#define _USE_MATH_DEFINES
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h> 
#include <time.h>

extern "C" {
#include"./SDL2-2.0.10/include/SDL.h"
#include"./SDL2-2.0.10/include/SDL_main.h"
}

#define SCREEN_WIDTH	730		// szerokosc ekranu
#define SCREEN_HEIGHT	600		// wysokosc ekranu

#define TEXTURE_SIZE	18		// rozmiar tekstury
#define PLATFORMS_COUNT	9		// maksymalna liczba platform

#define MAX_SBARREL 10			// maksymalna liczba beczek na ekranie
#define BARREL_SPAWN_TIME 1.5	// czestotliwosc tworzenia beczek
#define BARREL_SPEED 250		// predkosc beczek
#define BARREL_SIZE 16			// rozmiar beczek

#define HERO_SIZE 16			// rozmiar postaci
#define ANTAGONIST_SIZE 32		// rozmiar antagonisty

#define DEFAULT_Y_SPEED 200		// grawitacja
#define DEFAULT_X_SPEED	200		// predkosc postaci x

#define START_JUMP_SPEED 300	//V0y skoku postaci
#define G 5000					//g


struct s_game {
	int startPosX;
	int startPosY;
	int readyToClimb;
	int readyToDown;

	int jumped;
	int ceilingCollision;
	int touchedPlatform;
	int wasCollisionState3;
	int hJump;

	double speedJump;
	double heroSpeedX;
	double heroSpeedY;
	double climbingSpeed;

	double timeGone;
	int state;			//0 - standing, 1 - jumping, 2 - clumbing, 3 - down
	int animationFrame;
	int menu;
	int lives;
	int phase;
	int quit;
};
typedef struct s_game game;

struct s_sdls {
	SDL_Rect platforms[PLATFORMS_COUNT];
	SDL_Rect stairs[PLATFORMS_COUNT];
	SDL_Rect hero;
};
typedef struct s_sdls sdls;

struct s_barrelsInfo {
	double lastBarrelSpawn;
	int startPosX;
	int startPosY;
};
typedef struct s_barrelsInfo sBarrelsInfo;

struct s_barrel {
	int active = 0;
	double distanceX;
	double distanceY;
	int falling;
	int direction;
	int currentPlatform;
	SDL_Rect barrelRect;
};
typedef struct s_barrel sBarrel;


void DrawString(SDL_Surface* screen, int x, int y, const char* text, SDL_Surface* charset) {
	int px, py, c;
	SDL_Rect s, d;
	s.w = 8;
	s.h = 8;
	d.w = 8;
	d.h = 8;
	while (*text) {
		c = *text & 255;
		px = (c % 16) * 8;
		py = (c / 16) * 8;
		s.x = px;
		s.y = py;
		d.x = x;
		d.y = y;
		SDL_BlitSurface(charset, &s, screen, &d);
		x += 8;
		text++;
	};
};

void DrawPixel(SDL_Surface* surface, int x, int y, Uint32 color) {
	int bpp = surface->format->BytesPerPixel;
	Uint8* p = (Uint8*)surface->pixels + y * surface->pitch + x * bpp;
	*(Uint32*)p = color;
};

void DrawLine(SDL_Surface* screen, int x, int y, int l, int dx, int dy, Uint32 color) {
	for (int i = 0; i < l; i++) {
		DrawPixel(screen, x, y, color);
		x += dx;
		y += dy;
	};
};

void drawRectangle(SDL_Surface* screen, int x, int y, int l, int k, Uint32 outlineColor, Uint32 fillColor) {
	int i;
	DrawLine(screen, x, y, k, 0, 1, outlineColor);
	DrawLine(screen, x + l - 1, y, k, 0, 1, outlineColor);
	DrawLine(screen, x, y, l, 1, 0, outlineColor);
	DrawLine(screen, x, y + k - 1, l, 1, 0, outlineColor);
	for (i = y + 1; i < y + k - 1; i++)
		DrawLine(screen, x + 1, i, l - 2, 1, 0, fillColor);
};

int checkCollision(SDL_Rect first, SDL_Rect second) {
	int topFirst, topSecond;
	int leftFirst, leftSecond;
	int rightFirst, rightSecond;
	int bottomFirst, bottomSecond;

	topFirst = first.y;
	leftFirst = first.x;
	rightFirst = first.x + first.w;
	bottomFirst = first.y + first.h;

	topSecond = second.y;
	leftSecond = second.x;
	rightSecond = second.x + second.w;
	bottomSecond = second.y + second.h;


	if (topFirst >= bottomSecond) {
		return 0;
	}
	else if (leftFirst >= rightSecond) {
		return 0;
	}
	else if (rightFirst <= leftSecond) {
		return 0;
	}
	else if (bottomFirst <= topSecond) {
		return 0;
	}
	return 1;
}

int checkCollisionStairs(SDL_Rect hero, SDL_Rect stairs) {
	int x = hero.x + hero.w / 2;
	int y = hero.y + hero.h / 2;


	if (x < stairs.x) {
		return 0;
	}
	else if (x > (stairs.x + stairs.w)) {
		return 0;
	}
	else if (y < stairs.y) {
		return 0;
	}
	else if (y > (stairs.y + stairs.h)) {
		return 0;
	}

	return 1;
}

void deleteBarrels(sBarrelsInfo* sBarrelsInfo, sBarrel(*sBarrel)[MAX_SBARREL], int* number) {
	sBarrelsInfo->lastBarrelSpawn = 0;
	for (int i = 0; i < *number; i++) {
		(*sBarrel)[i] = {};
		(*sBarrel)[i].active = 0;
	}
	*number = 0;
}

void checkEvent(SDL_Event& event, game* game, sBarrelsInfo* sBarrelsInfo, sBarrel(*sBarrel)[MAX_SBARREL], double* dX, double* dY, int* num) {
	if (event.type == SDL_KEYDOWN && event.key.repeat == 0) {
		switch (event.key.keysym.sym) {
		case SDLK_LEFT: game->heroSpeedX -= DEFAULT_X_SPEED; break;
		case SDLK_RIGHT: game->heroSpeedX += DEFAULT_X_SPEED; break;
		case SDLK_UP:
			if (game->readyToClimb == 1 && game->state != 2 && game->state != 1) {
				game->state = 2;
			}

			if (game->state == 2) {
				game->climbingSpeed -= DEFAULT_Y_SPEED;
			}
			break;
		case SDLK_DOWN:
			if (game->state == 2) {
				game->climbingSpeed += DEFAULT_Y_SPEED;
			}
			else if (game->state == 0 && game->readyToDown) {
				game->climbingSpeed += DEFAULT_Y_SPEED;
				game->state = 3;
			}
			break;
		case SDLK_SPACE:
			if (game->state == 2) break;
			game->state = 1;
			if (game->heroSpeedX != 0) {
				game->hJump = 1;
				game->speedJump = game->heroSpeedX;
			}
			break;

		case SDLK_1:
			deleteBarrels(sBarrelsInfo, sBarrel, num);
			game->state = 0;
			game->timeGone = 0;
			game->phase = 1;
			game->animationFrame = 0;
			*dX = 0;
			*dY = 0;
			break;
		case SDLK_2:
			deleteBarrels(sBarrelsInfo, sBarrel, num);
			game->state = 0;
			game->timeGone = 0;
			game->phase = 2;
			game->animationFrame = 0;
			*dX = 0;
			*dY = 0;
			break;
		case SDLK_3:
			deleteBarrels(sBarrelsInfo, sBarrel, num);
			game->state = 0;
			game->timeGone = 0;
			game->phase = 3;
			game->animationFrame = 0;
			*dX = 0;
			*dY = 0;
			break;
		case SDLK_n:
			deleteBarrels(sBarrelsInfo, sBarrel, num);
			game->state = 0;
			game->timeGone = 0;
			game->animationFrame = 0;
			*dX = 0;
			*dY = 0;
			break;
		case SDLK_ESCAPE: game->quit = 1; break;
		}
	}
	else if (event.type == SDL_KEYUP && event.key.repeat == 0) {
		switch (event.key.keysym.sym) {
		case SDLK_LEFT: game->heroSpeedX += DEFAULT_X_SPEED; break;
		case SDLK_RIGHT: game->heroSpeedX -= DEFAULT_X_SPEED; break;
		case SDLK_UP:
			if (game->state == 2) {
				game->climbingSpeed += DEFAULT_Y_SPEED;
			}
			break;
		case SDLK_DOWN:
			if (game->state == 2) {
				game->climbingSpeed -= DEFAULT_Y_SPEED;
			}
			break;
		}
	}
	else if (event.type == SDL_QUIT) {
		game->quit = 1;
	}
}

void checkEventMenu(SDL_Event& event, game* game, int* notAvailable) {
	if (event.type == SDL_KEYDOWN && event.key.repeat == 0) {
		switch (event.key.keysym.sym) {
		case SDLK_1:
			if (game->menu == 1) {
				game->menu = 0;
				game->animationFrame = 0;
				*notAvailable = 0;
				game->phase = 1;
				game->timeGone = 0;
			}
			break;
		case SDLK_2:
			if (game->menu == 1) {
				game->menu = 0;
				game->animationFrame = 0;
				*notAvailable = 0;
				game->phase = 2;
				game->timeGone = 0;
			}
			break;
		case SDLK_3:
			if (game->menu == 1) {
				game->menu = 0;
				game->animationFrame = 0;
				*notAvailable = 0;
				game->phase = 3;
				game->timeGone = 0;
			}
			break;
		case SDLK_t:
			if (game->menu == 2) {
				game->menu = 0;
				game->animationFrame = 0;
				*notAvailable = 0;
				game->timeGone = 0;
			}
			break;
		case SDLK_w: game->quit = 1; break;
		case SDLK_s:
			*notAvailable = 1;
			break;
		default:
			*notAvailable = 0;
			break;
		}
	}
	else if (event.type == SDL_KEYUP && event.key.repeat == 0) {
		switch (event.key.keysym.sym) {
		case SDLK_LEFT: if (game->heroSpeedX != 0) game->heroSpeedX += DEFAULT_X_SPEED; break;
		case SDLK_RIGHT: if (game->heroSpeedX != 0) game->heroSpeedX -= DEFAULT_X_SPEED; break;
		}
	}
	else if (event.type == SDL_QUIT) {
		game->quit = 1;
	}
}

SDL_Rect createPlatform(SDL_Surface* screen, int x, int y, int l, SDL_Surface* textures) {
	SDL_Rect s, d, collision;
	s.w = TEXTURE_SIZE;
	s.h = TEXTURE_SIZE;
	d.w = TEXTURE_SIZE;
	d.h = TEXTURE_SIZE;

	s.x = 5 * TEXTURE_SIZE;
	s.y = 0 * TEXTURE_SIZE;
	d.y = y;
	for (int i = 0; i < l; i++) {
		d.x = x + (i * TEXTURE_SIZE);
		SDL_BlitSurface(textures, &s, screen, &d);
	}
	collision.x = x;
	collision.y = y;
	collision.h = TEXTURE_SIZE;
	collision.w = TEXTURE_SIZE * l;

	return collision;
}

SDL_Rect createStairs(SDL_Surface* screen, SDL_Rect platform, int n, SDL_Surface* textures) {
	int x = platform.x + (TEXTURE_SIZE * n);
	int y = platform.y - TEXTURE_SIZE;
	SDL_Rect s, d, collision;

	s.w = TEXTURE_SIZE;
	s.h = TEXTURE_SIZE;
	d.w = TEXTURE_SIZE;
	d.h = TEXTURE_SIZE;

	s.x = 4 * TEXTURE_SIZE;
	s.y = 3 * TEXTURE_SIZE;

	for (int i = 0; i < 3; i++) {
		d.x = x;
		d.y = y - (i * TEXTURE_SIZE);
		SDL_BlitSurface(textures, &s, screen, &d);
	}

	//change mb
	collision.x = x;
	collision.y = y - 3 * TEXTURE_SIZE;
	collision.h = 4 * TEXTURE_SIZE;
	collision.w = TEXTURE_SIZE;

	return collision;
}

void checkBarrelsCollisions(sdls* sdls, sBarrelsInfo* sBarrelsInfo, sBarrel(*sBarrel)[MAX_SBARREL], int number, double delta) {
	for (int i = 0; i < number; i++) {
		for (int j = 0; j < PLATFORMS_COUNT; j++) {
			if (checkCollision(sdls->platforms[j], (*sBarrel)[i].barrelRect)) {
				(*sBarrel)[i].currentPlatform = j;
				(*sBarrel)[i].distanceY -= BARREL_SPEED * delta;
				(*sBarrel)[i].barrelRect.y = sBarrelsInfo->startPosY + (*sBarrel)[i].distanceY;
				if ((*sBarrel)[i].falling == 1) {
					(*sBarrel)[i].falling = 0;
					if ((rand() % 5) != 2) {
						(*sBarrel)[i].direction = ((*sBarrel)[i].direction + 1) % 2;
					}
				}
				break;
			}
		}

		if ((*sBarrel)[i].barrelRect.x < 4 || ((*sBarrel)[i].barrelRect.x + (*sBarrel)[i].barrelRect.w) > SCREEN_WIDTH - 4) {
			(*sBarrel)[i] = {};
			(*sBarrel)[i].active = 0;
		}

		if ((*sBarrel)[i].falling == 0) {
			if ((*sBarrel)[i].direction == 0) {
				if (((*sBarrel)[i].barrelRect.x + (*sBarrel)[i].barrelRect.w) < sdls->platforms[(*sBarrel)[i].currentPlatform].x) {
					(*sBarrel)[i].falling = 1;
				}
			}
			else {
				if ((*sBarrel)[i].barrelRect.x > (sdls->platforms[(*sBarrel)[i].currentPlatform].x + sdls->platforms[(*sBarrel)[i].currentPlatform].w)) {
					(*sBarrel)[i].falling = 1;
				}
			}
		}

	}
}

void moveBarrels(game* game, sdls* sdls, sBarrelsInfo* sBarrelsInfo, sBarrel(*sBarrel)[MAX_SBARREL], int number, double delta) {
	for (int i = 0; i < number; i++) {
		if ((*sBarrel)[i].falling == 0) {
			if ((*sBarrel)[i].direction == 1) {
				(*sBarrel)[i].distanceX += BARREL_SPEED * delta;
			}
			else {
				(*sBarrel)[i].distanceX += -BARREL_SPEED * delta;
			}
			(*sBarrel)[i].barrelRect.x = sBarrelsInfo->startPosX + (*sBarrel)[i].distanceX;
		}

		(*sBarrel)[i].distanceY += BARREL_SPEED * delta;
		(*sBarrel)[i].barrelRect.y = sBarrelsInfo->startPosY + (*sBarrel)[i].distanceY;

	}
}

void createBarrels(game* game, sdls* sdls, sBarrelsInfo* sBarrelsInfo, sBarrel(*sBarrel)[MAX_SBARREL], int* number) {
	if ((game->timeGone - sBarrelsInfo->lastBarrelSpawn) > BARREL_SPAWN_TIME) {
		SDL_Rect barrel;
		barrel.x = sBarrelsInfo->startPosX;
		barrel.y = sBarrelsInfo->startPosY;
		barrel.w = BARREL_SIZE;
		barrel.h = BARREL_SIZE;

		(*sBarrel)[*number].barrelRect = barrel;
		(*sBarrel)[*number].active = 1;
		(*sBarrel)[*number].distanceX = 0;
		(*sBarrel)[*number].distanceY = 0;
		(*sBarrel)[*number].falling = 1;
		(*sBarrel)[*number].direction = 0;
		*number += 1;
		sBarrelsInfo->lastBarrelSpawn = game->timeGone;
	}
}

void createPlayground(SDL_Surface* screen, SDL_Surface* textures, sdls* sdls, int phase) {
	if (phase == 1) {
		sdls->platforms[0] = createPlatform(screen, 5, SCREEN_HEIGHT - 50, 40, textures);
		sdls->platforms[1] = createPlatform(screen, 118, SCREEN_HEIGHT - 50 - 1 * 72, 25, textures);
		sdls->platforms[2] = createPlatform(screen, 190, SCREEN_HEIGHT - 50 - 2 * 72, 25, textures);
		sdls->platforms[3] = createPlatform(screen, 118, SCREEN_HEIGHT - 50 - 3 * 72, 25, textures);
		sdls->platforms[4] = createPlatform(screen, 190, SCREEN_HEIGHT - 50 - 4 * 72, 25, textures);
		sdls->platforms[5] = createPlatform(screen, 118, SCREEN_HEIGHT - 50 - 5 * 72, 25, textures);
		sdls->platforms[6] = createPlatform(screen, 222, SCREEN_HEIGHT - 50 - 6 * 72, 5, textures);

		sdls->stairs[0] = createStairs(screen, sdls->platforms[0], 10, textures);
		sdls->stairs[1] = createStairs(screen, sdls->platforms[1], 20, textures);
		sdls->stairs[2] = createStairs(screen, sdls->platforms[2], 5, textures);
		sdls->stairs[3] = createStairs(screen, sdls->platforms[3], 24, textures);
		sdls->stairs[4] = createStairs(screen, sdls->platforms[4], 10, textures);
		sdls->stairs[5] = createStairs(screen, sdls->platforms[5], 8, textures);
	}
	else if (phase == 2) {
		sdls->platforms[0] = createPlatform(screen, 5, SCREEN_HEIGHT - 50, 40, textures);
		sdls->platforms[1] = createPlatform(screen, 100, SCREEN_HEIGHT - 50 - 1 * 72, 10, textures);
		sdls->platforms[2] = createPlatform(screen, 350, SCREEN_HEIGHT - 50 - 1 * 72, 10, textures);
		sdls->platforms[3] = createPlatform(screen, 150, SCREEN_HEIGHT - 50 - 2 * 72, 25, textures);
		sdls->platforms[4] = createPlatform(screen, 50, SCREEN_HEIGHT - 50 - 3 * 72, 15, textures);
		sdls->platforms[5] = createPlatform(screen, 190, SCREEN_HEIGHT - 50 - 4 * 72, 25, textures);
		sdls->platforms[6] = createPlatform(screen, 118, SCREEN_HEIGHT - 50 - 5 * 72, 25, textures);
		sdls->platforms[7] = createPlatform(screen, 222, SCREEN_HEIGHT - 50 - 6 * 72, 5, textures);

		sdls->stairs[0] = createStairs(screen, sdls->platforms[0], 10, textures);
		sdls->stairs[1] = createStairs(screen, sdls->platforms[1], 20, textures);
		sdls->stairs[2] = createStairs(screen, sdls->platforms[3], 3, textures);
		sdls->stairs[3] = createStairs(screen, sdls->platforms[4], 13, textures);
		sdls->stairs[4] = createStairs(screen, sdls->platforms[5], 19, textures);
		sdls->stairs[5] = createStairs(screen, sdls->platforms[6], 8, textures);

	}
	else if (phase == 3) {
		sdls->platforms[0] = createPlatform(screen, 5, SCREEN_HEIGHT - 50, 40, textures);
		sdls->platforms[1] = createPlatform(screen, 300, SCREEN_HEIGHT - 50 - 1 * 72, 20, textures);
		sdls->platforms[2] = createPlatform(screen, 270, SCREEN_HEIGHT - 50 - 2 * 72, 21, textures);
		sdls->platforms[3] = createPlatform(screen, 100, SCREEN_HEIGHT - 50 - 2 * 72, 5, textures);
		sdls->platforms[4] = createPlatform(screen, 118, SCREEN_HEIGHT - 50 - 3 * 72, 25, textures);
		sdls->platforms[5] = createPlatform(screen, 190, SCREEN_HEIGHT - 50 - 4 * 72, 25, textures);
		sdls->platforms[6] = createPlatform(screen, 118, SCREEN_HEIGHT - 50 - 5 * 72, 25, textures);
		sdls->platforms[7] = createPlatform(screen, 222, SCREEN_HEIGHT - 50 - 6 * 72, 5, textures);
		sdls->platforms[8] = createPlatform(screen, 630, SCREEN_HEIGHT - 50 - 2 * 72 - 40, 5, textures);

		sdls->stairs[0] = createStairs(screen, sdls->platforms[0], 20, textures);
		sdls->stairs[1] = createStairs(screen, sdls->platforms[0], 35, textures);
		sdls->stairs[2] = createStairs(screen, sdls->platforms[1], 11, textures);
		sdls->stairs[3] = createStairs(screen, sdls->platforms[3], 3, textures);
		sdls->stairs[4] = createStairs(screen, sdls->platforms[4], 10, textures);
		sdls->stairs[5] = createStairs(screen, sdls->platforms[5], 19, textures);
		sdls->stairs[6] = createStairs(screen, sdls->platforms[6], 8, textures);
	}
}

void cleanEverything(SDL_Surface* charset, SDL_Surface* screen, SDL_Texture* scrtex, SDL_Renderer* renderer, SDL_Window* window) {
	SDL_FreeSurface(charset);
	SDL_FreeSurface(screen);
	SDL_DestroyTexture(scrtex);
	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);
}

void moving(game* game, double* distanceX, double* distanceY, double* jumpTime, double delta) {
	//X moving
	if (game->state == 0) {
		*distanceX += game->heroSpeedX * delta;
	}
	else if (game->hJump == 1 && game->state == 1) {
		*distanceX += game->speedJump * delta;
	}

	//Y moving for different phases
	if (game->state == 0 || game->ceilingCollision == 1) {
		*distanceY += game->heroSpeedY * delta;
	}
	else if (game->state == 1) {
		if (game->jumped == 0) {
			game->jumped = 1;
			*jumpTime = 0;
		}
		*jumpTime += delta;
		game->heroSpeedY = -START_JUMP_SPEED + (G * *jumpTime * *jumpTime);
		*distanceY += game->heroSpeedY * delta;
	}
	else if (game->state == 2) {
		*distanceY += game->climbingSpeed * delta;
	}
	else if (game->state == 3) {
		*distanceY += game->climbingSpeed * delta;
	}
}

void drawTab(SDL_Surface* charset, SDL_Surface* screen, game* game, char* text, double* fps, int color1, int color2) {
	drawRectangle(screen, 4, 4, SCREEN_WIDTH - 8, 36, color1, color2);
	sprintf(text, "Czas, ktory minal od poczatku etapu = %.1lfs  [FPS: %.0lf [VSYNC]", game->timeGone, *fps);
	DrawString(screen, screen->w / 2 - strlen(text) * 8 / 2, 10, text, charset);
	sprintf(text, "Esc - wyjscie, [1-3] - etap, n - nowa rozgrywka");
	DrawString(screen, screen->w / 2 - strlen(text) * 8 / 2, 26, text, charset);
}

void drawLives(SDL_Surface* charset, SDL_Surface* screen, game* game, char* text, int color1, int color2) {
	sprintf(text, "Zycia:");
	DrawString(screen, 9, 54, text, charset);
	for (int i = 0; i < game->lives; i++) {
		drawRectangle(screen, 60 + i * TEXTURE_SIZE, 49, TEXTURE_SIZE, TEXTURE_SIZE, color2, color1);
	}
}

void drawHero(SDL_Surface* screen, SDL_Surface* sprite, game* game, SDL_Rect hero, int jumpFrame) {
	SDL_Rect s, d;
	s.w = HERO_SIZE;
	s.h = HERO_SIZE;

	d.w = HERO_SIZE;
	d.h = HERO_SIZE;
	d.y = hero.y;
	d.x = hero.x;

	if (game->state == 1) {
		switch (jumpFrame) {
		case 0:
			s.x = 4 * HERO_SIZE;
			s.y = 4 * HERO_SIZE;
			break;
		case 1:
			s.x = 4 * HERO_SIZE;
			s.y = 5 * HERO_SIZE;
			break;
		case 2:
			s.x = 4 * HERO_SIZE;
			s.y = 7 * HERO_SIZE;
			break;
		case 3:
			s.x = 4 * HERO_SIZE;
			s.y = 6 * HERO_SIZE;
			break;
		}
	}
	else if (game->state == 2 || game->state == 3) {
		s.x = 3 * HERO_SIZE + (game->animationFrame * HERO_SIZE);
		s.y = 7 * HERO_SIZE;
	}
	else if (game->heroSpeedX > 0) {
		s.x = 3 * HERO_SIZE + (game->animationFrame * HERO_SIZE);
		s.y = 6 * HERO_SIZE;
	}
	else if (game->heroSpeedX < 0) {
		s.x = 3 * HERO_SIZE + (game->animationFrame * HERO_SIZE);
		s.y = 5 * HERO_SIZE;
	}
	else if (game->heroSpeedX == 0) {
		s.x = 4 * HERO_SIZE;
		s.y = 4 * HERO_SIZE;
	}

	SDL_BlitSurface(sprite, &s, screen, &d);
}

void drawBarrel(SDL_Surface* screen, SDL_Surface* sprite, game* game, sBarrel barrel) {
	SDL_Rect s, d;
	s.w = BARREL_SIZE;
	s.h = BARREL_SIZE;

	d.w = BARREL_SIZE;
	d.h = BARREL_SIZE;
	d.y = barrel.barrelRect.y;
	d.x = barrel.barrelRect.x;

	if (barrel.falling == 1) {
		s.x = 120;
		s.y = 112;
	}
	else {
		s.x = 3 * BARREL_SIZE + (game->animationFrame * BARREL_SIZE);
		s.y = 3 * BARREL_SIZE;
	}

	SDL_BlitSurface(sprite, &s, screen, &d);
}

void drawAntagonist(SDL_Surface* screen, game* game, SDL_Surface* sprite, int x, int y) {
	SDL_Rect s, d;
	s.w = ANTAGONIST_SIZE;
	s.h = ANTAGONIST_SIZE;

	d.w = ANTAGONIST_SIZE;
	d.h = ANTAGONIST_SIZE;
	d.y = y;
	d.x = x;

	s.x = game->animationFrame * ANTAGONIST_SIZE;
	s.y = 0;

	SDL_BlitSurface(sprite, &s, screen, &d);
}

void drawMenu(SDL_Surface* charset, SDL_Surface* screen, game* game, char* text, int notAvailable, int color1, int color2) {
	drawRectangle(screen, 4, 4, SCREEN_WIDTH - 8, 36, color1, color2);
	sprintf(text, "Menu");
	DrawString(screen, screen->w / 2 - strlen(text) * 8 / 2, 20, text, charset);

	if (game->menu == 1) {
		sprintf(text, "[Zrealizowane: obowiazkowe + A B C D E] (11 pkt.)");
		DrawString(screen, screen->w / 2 - strlen(text) * 8 / 2, 50, text, charset);
		sprintf(text, "[1-3]Wybor etapu");
		DrawString(screen, screen->w / 2 - strlen(text) * 8 / 2, SCREEN_HEIGHT / 2, text, charset);
		if (notAvailable == 1) {
			sprintf(text, "[S]prawdzenie wynikow (Niedostepno)");
			DrawString(screen, screen->w / 2 - strlen(text) * 8 / 2, SCREEN_HEIGHT / 2 + 20, text, charset);
		}
		else {
			sprintf(text, "[S]prawdzenie wynikow");
			DrawString(screen, screen->w / 2 - strlen(text) * 8 / 2, SCREEN_HEIGHT / 2 + 20, text, charset);
		}
		sprintf(text, "[W]yjscie");
		DrawString(screen, screen->w / 2 - strlen(text) * 8 / 2, SCREEN_HEIGHT / 2 + 2 * 20, text, charset);
	}
	else if (game->menu == 2) {
		sprintf(text, "Straciles zycie. Chcesz kontynuowac?");
		DrawString(screen, screen->w / 2 - strlen(text) * 8 / 2, SCREEN_HEIGHT / 2, text, charset);
		sprintf(text, "[T]ak");
		DrawString(screen, screen->w / 2 - strlen(text) * 8 / 2, SCREEN_HEIGHT / 2 + 20, text, charset);
		sprintf(text, "[W]yjscie");
		DrawString(screen, screen->w / 2 - strlen(text) * 8 / 2, SCREEN_HEIGHT / 2 + 2 * 20, text, charset);
	}
}

void checkHeroCollisions(game* game, sdls* sdls, SDL_Rect checkRect, double delta, double* distanceX, double* distanceY) {
	sdls->hero.x = game->startPosX + *distanceX;
	for (int i = 0; i < PLATFORMS_COUNT; i++) {
		if (checkCollision(sdls->platforms[i], sdls->hero) || sdls->hero.x < 4 || (sdls->hero.x + sdls->hero.w) > SCREEN_WIDTH - 4) {
			if (game->hJump == 0) {
				*distanceX -= game->heroSpeedX * delta;
			}
			else {
				*distanceX -= game->speedJump * delta;
			}
			sdls->hero.x = game->startPosX + *distanceX;
		}

		if (checkCollisionStairs(sdls->hero, sdls->stairs[i])) {
			game->readyToClimb = 1;
		}

		checkRect.x = sdls->hero.x + sdls->hero.w / 2;
		checkRect.y = sdls->hero.y + (sdls->hero.h * 3) / 2;
		checkRect.h = 2;
		checkRect.w = 2;
		if (checkCollisionStairs(checkRect, sdls->stairs[i]) && game->state == 0) {
			game->readyToDown = 1;
		}
	}
	sdls->hero.y = game->startPosY + *distanceY;
	for (int i = 0; i < PLATFORMS_COUNT; i++) {
		if (checkCollision(sdls->platforms[i], sdls->hero)) {
			if (game->state != 3) {
				*distanceY -= game->heroSpeedY * delta;
			}
			else {
				*distanceY += game->heroSpeedY * delta;
			}
			sdls->hero.y = game->startPosY + *distanceY;

			if (game->state == 1 && sdls->hero.y < sdls->platforms[i].y) {
				game->state = 0;
				game->jumped = 0;
				game->hJump = 0;
				game->speedJump = 0;
				game->ceilingCollision = 0;
				game->heroSpeedY = DEFAULT_Y_SPEED;

			}
			else if (game->state == 1 && sdls->hero.y > sdls->platforms[i].y && game->ceilingCollision != 1) {
				game->ceilingCollision = 1;
				game->heroSpeedY = DEFAULT_Y_SPEED;
			}
			else if (game->state == 2) {
				game->touchedPlatform = i;
			}
			else if (game->state == 3) {
				game->wasCollisionState3 = 1;
			}
		}
	}

	if (game->state == 2 && game->touchedPlatform != -1) {
		if (!checkCollision(sdls->platforms[game->touchedPlatform], sdls->hero)) {
			game->touchedPlatform = -1;
			game->state = 0;
			game->readyToClimb = 0;
			game->climbingSpeed = 0;
		}
	}
	else if (game->state == 3 && game->wasCollisionState3 != 0) {
		game->wasCollisionState3 = 0;
	}
	else if (game->state == 3 && game->wasCollisionState3 == 0) {
		game->state = 2;
	}
}

void checkFinish(game* game, sdls* sdls, sBarrelsInfo* sBarrelsInfo, sBarrel(*sBarrel)[MAX_SBARREL], double* dX, double* dY, int* barNum) {
	if (sdls->hero.y < 105) {
		deleteBarrels(sBarrelsInfo, sBarrel, barNum);
		game->state = 0;
		game->timeGone = 0;
		game->phase++;
		if (game->phase > 3) {
			game->menu = 1;
		}
		game->animationFrame = 0;
		*dX = 0;
		*dY = 0;
		game->climbingSpeed = 0;
	}
}

void checkHeroAndBarrels(game* game, sdls* sdls, sBarrelsInfo* sBarrelsInfo, sBarrel(*sBarrel)[MAX_SBARREL], double* dX, double* dY, int* barNum) {
	for (int i = 0; i < *barNum; i++) {
		if ((*sBarrel)[i].active == 1) {} {
			if (checkCollision((*sBarrel)[i].barrelRect, sdls->hero)) {
				deleteBarrels(sBarrelsInfo, sBarrel, barNum);
				game->lives--;
				if (game->lives > 0) {
					game->menu = 2;
				}
				else if (game->lives == 0) {
					game->menu = 1;
					game->lives = 3;
				}
				game->state = 0;
				game->timeGone = 0;
				*dX = 0;
				*dY = 0;
				game->heroSpeedY = DEFAULT_Y_SPEED;
				game->climbingSpeed = 0;

				if (game->jumped != 0) {
					game->jumped = 0;
					game->hJump = 0;
					game->speedJump = 0;
					game->ceilingCollision = 0;
				}
			}
		}
	}
}

void cleanAndDrawBarrels(SDL_Surface* screen, game* game, sBarrel(*sBarrel)[MAX_SBARREL], SDL_Surface* sprite, int* barNum) {
	if (*barNum > 1) {
		for (int i = 0; i < *barNum; i++) {
			if ((*sBarrel)[i].active == 0 && i < *barNum - 1) {
				(*sBarrel)[i] = (*sBarrel)[*barNum - 1];
				(*sBarrel)[i].active = 1;
				(*sBarrel)[*barNum - 1] = {};
				(*sBarrel)[*barNum - 1].active = 0;
				*barNum -= 1;
			}
			else if ((*sBarrel)[i].active == 0 && i == *barNum - 1) {
				*barNum -= 1;
			}
		}
	}

	for (int i = 0; i < *barNum; i++) {
		if ((*sBarrel)[i].active == 1) {} {
			drawBarrel(screen, sprite, game, (*sBarrel)[i]);
		}
	}
}

#ifdef __cplusplus
extern "C"
#endif

int main(int argc, char** argv) {
	int tick1, tick2, frames, rc, barrelsNumber, jumpFrame;
	int notAvailable;
	double delta, fpsTimer, fps, distanceX, distanceY, jumpTime;

	SDL_Rect checkRect;
	checkRect.x = 0;
	checkRect.y = 0;
	checkRect.h = 0;
	checkRect.w = 0;

	SDL_Event event;
	SDL_Surface* screen, * charset;
	SDL_Surface* textures, * heroAnimSprite, * barrelSprite, * antagonistSprite;
	SDL_Texture* scrtex;
	SDL_Window* window;
	SDL_Renderer* renderer;

	game game = { 0, 0, 0, 0, 0, 0, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 3, 1, 0 };
	sdls sdls;
	sBarrelsInfo sBarrelsInfo = { 0, SCREEN_WIDTH / 2 - 170, SCREEN_HEIGHT / 2 - 150 };
	sBarrel sBarrel[MAX_SBARREL];

	game.startPosX = 60;
	game.startPosY = SCREEN_HEIGHT - 150;

	sdls.hero.x = game.startPosX;
	sdls.hero.y = game.startPosY;
	sdls.hero.w = HERO_SIZE;
	sdls.hero.h = HERO_SIZE;

	frames = 0;
	jumpFrame = 0;
	fpsTimer = 0;
	fps = 0;
	distanceX = 0;
	distanceY = 0;
	game.heroSpeedY = DEFAULT_Y_SPEED;

	barrelsNumber = 0;
	notAvailable = 0;

	srand(time(NULL));


	if (SDL_Init(SDL_INIT_EVERYTHING) != 0) {
		printf("SDL_Init error: %s\n", SDL_GetError());
		return 1;
	}

	window = SDL_CreateWindow("Text", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_OPENGL);
	if (window == NULL) {
		SDL_Quit();
		printf("SDL_CreateWindow error: %s\n", SDL_GetError());
		return 1;
	};
	renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
	if (renderer == NULL) {
		SDL_Quit();
		printf("SDL_CreateRenderer error: %s\n", SDL_GetError());
		return 1;
	};

	SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "linear");
	SDL_RenderSetLogicalSize(renderer, SCREEN_WIDTH, SCREEN_HEIGHT);
	SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
	SDL_SetWindowTitle(window, "Klim, Kaliasniou, 201250");

	screen = SDL_CreateRGBSurface(0, SCREEN_WIDTH, SCREEN_HEIGHT, 32, 0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000);
	scrtex = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, SCREEN_WIDTH, SCREEN_HEIGHT);

	SDL_ShowCursor(SDL_DISABLE);


	charset = SDL_LoadBMP("./cs8x8.bmp");
	if (charset == NULL) {
		printf("SDL_LoadBMP(cs8x8.bmp) error: %s\n", SDL_GetError());
		SDL_FreeSurface(screen);
		SDL_DestroyTexture(scrtex);
		SDL_DestroyWindow(window);
		SDL_DestroyRenderer(renderer);
		SDL_Quit();
		return 1;
	};
	SDL_SetColorKey(charset, true, 0x000000);

	textures = SDL_LoadBMP("./textures.bmp");
	if (textures == NULL) {
		printf("SDL_LoadBMP(textures.bmp) error: %s\n", SDL_GetError());
		cleanEverything(charset, screen, scrtex, renderer, window);
		SDL_Quit();
		return 1;
	};

	heroAnimSprite = SDL_LoadBMP("./heroAnimations.bmp");
	if (heroAnimSprite == NULL) {
		printf("SDL_LoadBMP(heroAnimations.bmp) error: %s\n", SDL_GetError());
		cleanEverything(charset, screen, scrtex, renderer, window);
		SDL_Quit();
		return 1;
	};

	barrelSprite = SDL_LoadBMP("./barrel.bmp");
	if (barrelSprite == NULL) {
		printf("SDL_LoadBMP(barrel.bmp) error: %s\n", SDL_GetError());
		cleanEverything(charset, screen, scrtex, renderer, window);
		SDL_Quit();
		return 1;
	};

	antagonistSprite = SDL_LoadBMP("./antagonist.bmp");
	if (antagonistSprite == NULL) {
		printf("SDL_LoadBMP(antagonist.bmp) error: %s\n", SDL_GetError());
		cleanEverything(charset, screen, scrtex, renderer, window);
		SDL_Quit();
		return 1;
	};


	char text[128];
	int czarny = SDL_MapRGB(screen->format, 0x00, 0x00, 0x00);
	int bialy = SDL_MapRGB(screen->format, 255, 255, 255);
	int zielony = SDL_MapRGB(screen->format, 0x00, 0xFF, 0x00);


	tick1 = SDL_GetTicks();


	while (!game.quit) {
		tick2 = SDL_GetTicks();
		delta = (tick2 - tick1) * 0.001;
		tick1 = tick2;

		game.timeGone += delta;

		//fps counter
		fpsTimer += delta;
		if (fpsTimer > 0.5) {
			fps = frames * 2;
			frames = 0;
			fpsTimer -= 0.5;

			game.animationFrame++;
			if (game.animationFrame > 2) {
				game.animationFrame = 0;
			}
			jumpFrame++;
			if (jumpFrame > 3) {
				jumpFrame = 0;
			}
		};


		//drawing interface
		SDL_FillRect(screen, NULL, czarny);
		drawRectangle(screen, 4, 44, SCREEN_WIDTH - 8, SCREEN_HEIGHT - 48, bialy, czarny);


		if (game.menu == 0) {
			moving(&game, &distanceX, &distanceY, &jumpTime, delta);


			//drawing game
			drawTab(charset, screen, &game, text, &fps, bialy, czarny);
			drawAntagonist(screen, &game, antagonistSprite, SCREEN_WIDTH / 2 - 220, SCREEN_HEIGHT / 2 - 135);
			createPlayground(screen, textures, &sdls, game.phase);
			drawLives(charset, screen, &game, text, zielony, czarny);
			drawHero(screen, heroAnimSprite, &game, sdls.hero, jumpFrame);


			//reset
			game.readyToClimb = 0;
			game.readyToDown = 0;


			checkHeroCollisions(&game, &sdls, checkRect, delta, &distanceX, &distanceY);		/*
			sdls.hero.x = startPosX + distanceX;
			for (int i = 0; i < PLATFORMS_COUNT; i++) {
				if (checkCollision(sdls.platforms[i], sdls.hero) || sdls.hero.x < 4 || (sdls.hero.x + sdls.hero.w) > SCREEN_WIDTH - 4) {
					if (game.hJump == 0) {
						distanceX -= game.heroSpeedX * delta;
					}
					else {
						distanceX -= game.speedJump * delta;
					}
					sdls.hero.x = startPosX + distanceX;
				}

				if (checkCollisionStairs(sdls.hero, sdls.stairs[i])) {
					game.readyToClimb = 1;
				}

				checkRect.x = sdls.hero.x + sdls.hero.w / 2;
				checkRect.y = sdls.hero.y + (sdls.hero.h*3)/2;
				checkRect.h = 2;
				checkRect.w = 2;
				if (checkCollisionStairs(checkRect, sdls.stairs[i]) && game.state == 0) {
					game.readyToDown = 1;
				}
			}
			sdls.hero.y = startPosY + distanceY;
			for (int i = 0; i < PLATFORMS_COUNT; i++) {
				if (checkCollision(sdls.platforms[i], sdls.hero)) {
					if (game.state != 3) {
						distanceY -= game.heroSpeedY * delta;
					}
					else {
						distanceY += game.heroSpeedY * delta;
					}
					sdls.hero.y = startPosY + distanceY;

					if (game.state == 1 && sdls.hero.y < sdls.platforms[i].y) {
						game.state = 0;
						jumped = 0;
						game.hJump = 0;
						game.speedJump = 0;
						ceilingCollision = 0;
						game.heroSpeedY = DEFAULT_Y_SPEED;

					}
					else if (game.state == 1 && sdls.hero.y > sdls.platforms[i].y && ceilingCollision != 1) {
						ceilingCollision = 1;
						game.heroSpeedY = DEFAULT_Y_SPEED;
					}
					else if (game.state == 2) {
						touchedPlatform = i;
					}
					else if (game.state == 3) {
						wasCollisionState3 = 1;
					}
				}
			}

			if (game.state == 2 && touchedPlatform != -1) {
				if (!checkCollision(sdls.platforms[touchedPlatform], sdls.hero)) {
					touchedPlatform = -1;
					game.state = 0;
					game.readyToClimb = 0;
					game.climbingSpeed = 0;
				}
			}
			else if (game.state == 3 && wasCollisionState3 != 0) {
				wasCollisionState3 = 0;
			}
			else if (game.state == 3 && wasCollisionState3 == 0) {
				game.state = 2;
			}
			*/

			checkFinish(&game, &sdls, &sBarrelsInfo, &sBarrel, &distanceX, &distanceY, &barrelsNumber);

			checkHeroAndBarrels(&game, &sdls, &sBarrelsInfo, &sBarrel, &distanceX, &distanceY, &barrelsNumber);


			//create, move and draw barrels
			createBarrels(&game, &sdls, &sBarrelsInfo, &sBarrel, &barrelsNumber);
			moveBarrels(&game, &sdls, &sBarrelsInfo, &sBarrel, barrelsNumber, delta);
			checkBarrelsCollisions(&sdls, &sBarrelsInfo, &sBarrel, barrelsNumber, delta); // barrels moving


			cleanAndDrawBarrels(screen, &game, &sBarrel, barrelSprite, &barrelsNumber);


			//update everything
			SDL_UpdateTexture(scrtex, NULL, screen->pixels, screen->pitch);
			SDL_RenderCopy(renderer, scrtex, NULL, NULL);
			SDL_RenderPresent(renderer);


			while (SDL_PollEvent(&event)) {
				checkEvent(event, &game, &sBarrelsInfo, &sBarrel, &distanceX, &distanceY, &barrelsNumber);
			};

			frames++;
		}
		else {
			drawMenu(charset, screen, &game, text, notAvailable, bialy, czarny);

			SDL_UpdateTexture(scrtex, NULL, screen->pixels, screen->pitch);
			SDL_RenderCopy(renderer, scrtex, NULL, NULL);
			SDL_RenderPresent(renderer);

			while (SDL_PollEvent(&event)) {
				checkEventMenu(event, &game, &notAvailable);
			};
		}
	};


	//freeing all surfaces
	cleanEverything(charset, screen, scrtex, renderer, window);
	SDL_Quit();


	return 0;
};
