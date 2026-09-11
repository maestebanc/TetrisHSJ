#ifndef FONT_H
#define FONT_H

#include <SDL2/SDL.h>
#include <stdbool.h>

// Initialize font subsystem
void font_init(void);

// Get width of a string at given scale
int font_text_width(const char *text, int scale);

// Get height of a string at given scale
int font_text_height(int scale);

// Draw plain text
void font_draw_text(SDL_Renderer *renderer, const char *text, int x, int y, int scale, SDL_Color color);

// Draw text with a drop shadow (offset by scale pixels)
void font_draw_text_shadow(SDL_Renderer *renderer, const char *text, int x, int y, int scale, SDL_Color color, SDL_Color shadow_color);

// Draw centered text horizontally around cx
void font_draw_text_centered(SDL_Renderer *renderer, const char *text, int cx, int y, int scale, SDL_Color color);

// Draw centered text with shadow
void font_draw_text_centered_shadow(SDL_Renderer *renderer, const char *text, int cx, int y, int scale, SDL_Color color, SDL_Color shadow_color);

// Draw right-aligned text ending at rx
void font_draw_text_right(SDL_Renderer *renderer, const char *text, int rx, int y, int scale, SDL_Color color);

// Draw right-aligned text with shadow
void font_draw_text_right_shadow(SDL_Renderer *renderer, const char *text, int rx, int y, int scale, SDL_Color color, SDL_Color shadow_color);

#endif // FONT_H
