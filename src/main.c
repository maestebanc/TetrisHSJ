#include "game.h"
#include "render.h"
#include "audio.h"
#include "font.h"
#include <SDL2/SDL.h>
#include <stdio.h>
#include <stdbool.h>

int main(int argc, char *argv[]) {
    (void)argc;
    (void)argv;

    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "1");
    SDL_SetHint(SDL_HINT_RENDER_VSYNC, "1");

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER) != 0) {
        fprintf(stderr, "Error al inicializar SDL2: %s\n", SDL_GetError());
        return 1;
    }

    font_init();

    if (!audio_init()) {
        fprintf(stderr, "Aviso: No se pudo inicializar el dispositivo de audio: %s\n", SDL_GetError());
    }

    SDL_Window *window = NULL;
    SDL_Renderer *renderer = NULL;
    if (!render_init(&window, &renderer)) {
        fprintf(stderr, "Error al crear la ventana o renderer de SDL2.\n");
        audio_cleanup();
        SDL_Quit();
        return 1;
    }

    // Load window icon from multiple candidate paths
    SDL_Surface *icon = SDL_LoadBMP("assets/icon.bmp");
    if (!icon) icon = SDL_LoadBMP("icon.bmp");
    if (!icon) icon = SDL_LoadBMP("/home/maec/tetris3/assets/icon.bmp");
    if (icon) {
        SDL_SetWindowIcon(window, icon);
        SDL_FreeSurface(icon);
    }

    Game game;
    game_init(&game);
    if (argc > 1 && strcmp(argv[1], "--play") == 0) {
        game_reset(&game);
    }

    bool running = true;
    uint32_t last_time = SDL_GetTicks();
    const uint32_t target_frame_time = 1000 / 60; // 60 FPS cap

    while (running) {
        uint32_t current_time = SDL_GetTicks();
        uint32_t delta_ms = current_time - last_time;
        if (delta_ms > 100) delta_ms = 100;
        last_time = current_time;

        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            switch (event.type) {
                case SDL_QUIT:
                    running = false;
                    break;
                case SDL_KEYDOWN:
                    if (event.key.keysym.sym == SDLK_F11 ||
                        (event.key.keysym.sym == SDLK_RETURN && (event.key.keysym.mod & KMOD_ALT))) {
                        Uint32 flags = SDL_GetWindowFlags(window);
                        if (flags & SDL_WINDOW_FULLSCREEN_DESKTOP) {
                            SDL_SetWindowFullscreen(window, 0);
                        } else {
                            SDL_SetWindowFullscreen(window, SDL_WINDOW_FULLSCREEN_DESKTOP);
                        }
                        break;
                    }
                    if (event.key.repeat == 0) {
                        game_handle_key_down(&game, event.key.keysym.sym);
                    }
                    break;
                case SDL_KEYUP:
                    game_handle_key_up(&game, event.key.keysym.sym);
                    break;
                case SDL_WINDOWEVENT:
                    if (event.window.event == SDL_WINDOWEVENT_SIZE_CHANGED ||
                        event.window.event == SDL_WINDOWEVENT_RESIZED ||
                        event.window.event == SDL_WINDOWEVENT_EXPOSED ||
                        event.window.event == SDL_WINDOWEVENT_MAXIMIZED ||
                        event.window.event == SDL_WINDOWEVENT_RESTORED) {
                        render_frame(renderer, &game);
                    }
                    break;
                default:
                    break;
            }
        }

        game_update(&game, delta_ms);
        render_frame(renderer, &game);

        uint32_t frame_duration = SDL_GetTicks() - current_time;
        if (frame_duration < target_frame_time) {
            SDL_Delay(target_frame_time - frame_duration);
        }
    }

    game_save_highscore(&game);
    render_cleanup(window, renderer);
    audio_cleanup();
    SDL_Quit();

    printf("Sant Joan Tetris - Hospital Universitario de Sant Joan cerrado correctamente.\n");
    return 0;
}
