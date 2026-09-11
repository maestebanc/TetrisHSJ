#include "game.h"
#include "render.h"
#include "audio.h"
#include "font.h"
#include <SDL2/SDL.h>
#include <stdio.h>
#include <stdbool.h>

#ifdef _WIN32
#include <windows.h>
static void init_windows_dpi_awareness(void) {
    // 1. Try Windows 10 1703+ PerMonitorV2
    HMODULE user32 = LoadLibraryA("user32.dll");
    if (user32) {
        typedef BOOL (WINAPI *SetProcessDpiAwarenessContextFunc)(void*);
        SetProcessDpiAwarenessContextFunc setContext = 
            (SetProcessDpiAwarenessContextFunc)GetProcAddress(user32, "SetProcessDpiAwarenessContext");
        if (setContext) {
            setContext((void*)-4); // DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2
        } else {
            typedef BOOL (WINAPI *SetProcessDPIAwareFunc)(void);
            SetProcessDPIAwareFunc setDPIAware = 
                (SetProcessDPIAwareFunc)GetProcAddress(user32, "SetProcessDPIAware");
            if (setDPIAware) setDPIAware();
        }
    }
    // 2. Try Windows 8.1+ Per-Monitor
    HMODULE shcore = LoadLibraryA("Shcore.dll");
    if (shcore) {
        typedef HRESULT (WINAPI *SetProcessDpiAwarenessFunc)(int);
        SetProcessDpiAwarenessFunc setAwareness = 
            (SetProcessDpiAwarenessFunc)GetProcAddress(shcore, "SetProcessDpiAwareness");
        if (setAwareness) {
            setAwareness(2); // PROCESS_PER_MONITOR_DPI_AWARE
        }
    }
}
#endif

#ifdef __APPLE__
#include <unistd.h>
#include <libgen.h>
#include <mach-o/dyld.h>
static void init_macos_environment(void) {
    char path[1024];
    uint32_t size = sizeof(path);
    if (_NSGetExecutablePath(path, &size) == 0) {
        char *dir = dirname(path);
        if (dir) {
            chdir(dir);
        }
    }
}
#endif

int main(int argc, char *argv[]) {
    (void)argc;
    (void)argv;

#ifdef _WIN32
    init_windows_dpi_awareness();
#endif
#ifdef __APPLE__
    init_macos_environment();
#endif

    // Set DPI and scaling hints BEFORE SDL_Init so video subsystem takes them into account
    SDL_SetHint("SDL_WINDOWS_DPI_AWARENESS", "permonitorv2");
    SDL_SetHint("SDL_WINDOWS_DPI_SCALING", "1");
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
    if (!icon) icon = SDL_LoadBMP("../Resources/assets/icon.bmp");
    if (!icon) icon = SDL_LoadBMP("/home/maec/tetris3/assets/icon.bmp");
    if (icon) {
        SDL_SetWindowIcon(window, icon);
        SDL_FreeSurface(icon);
    }

    Game game;
    game_init(&game);
    if (argc > 1 && strcmp(argv[1], "--play") == 0) {
        game_reset(&game);
    } else if (argc > 1 && strcmp(argv[1], "--generate-screenshots") == 0) {
        // 1. Pantalla de Inicio (Title Screen)
        render_take_screenshot(renderer, &game, "docs/screenshots/inicio.bmp");

        // 2. Pantalla de Partida en Vivo (Gameplay)
        game_reset(&game);
        game.state = STATE_PLAY;
        game.toast.alpha = 0.0f;
        game.toast.text[0] = '\0';
        game.score = 24850;
        game.lines = 28;
        game.level = 3;
        game.pieces_dropped = 29; // Turno de Marichu (25..49), 21 piezas para relevo
        game.highscore = 45000;
        for (int c = 0; c < BOARD_WIDTH; c++) {
            if (c != 3 && c != 4) game.board[19][c] = PIECE_I;
            if (c != 0 && c != 8) game.board[18][c] = PIECE_J;
            if (c > 1 && c < 9)   game.board[17][c] = PIECE_T;
            if (c >= 3 && c <= 7) game.board[16][c] = PIECE_O;
            if (c == 2 || c == 5) game.board[15][c] = PIECE_S;
        }
        game.current_piece = PIECE_Z;
        game.piece_x = 4;
        game.piece_y = 11;
        render_take_screenshot(renderer, &game, "docs/screenshots/gameplay.bmp");

        // 3. Manual de Guardia (F1 / About)
        game.state = STATE_ABOUT;
        render_take_screenshot(renderer, &game, "docs/screenshots/manual_guardia.bmp");

        // 4. Pausa de Guardia (Pausa Café)
        game.state = STATE_PAUSE;
        render_take_screenshot(renderer, &game, "docs/screenshots/pausa_cafe.bmp");

        render_cleanup(window, renderer);
        audio_cleanup();
        SDL_Quit();
        return 0;
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
                    if (event.key.keysym.sym == SDLK_F12) {
                        char fname[64];
                        snprintf(fname, sizeof(fname), "screenshot_%u.bmp", (unsigned int)SDL_GetTicks());
                        render_take_screenshot(renderer, &game, fname);
                        printf("Captura guardada en %s\n", fname);
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
