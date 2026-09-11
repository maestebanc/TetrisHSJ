#ifndef RENDER_H
#define RENDER_H

#include <SDL2/SDL.h>
#include <stdbool.h>
#include "game.h"

#define SCREEN_WIDTH  1920
#define SCREEN_HEIGHT 1080

bool render_init(SDL_Window **out_window, SDL_Renderer **out_renderer);
void render_cleanup(SDL_Window *window, SDL_Renderer *renderer);
void render_frame(SDL_Renderer *renderer, const Game *game);
void render_take_screenshot(SDL_Renderer *renderer, const Game *g, const char *bmp_path);

#endif // RENDER_H
