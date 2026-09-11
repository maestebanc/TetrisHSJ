#include "render.h"
#include "font.h"
#include "audio.h"
#include <stdio.h>
#include <string.h>
#include <math.h>

// Exact 1920x1080 Full HD Layout Coordinates
#define WIN_W          1920
#define WIN_H          1080

#define HEADER_X       20
#define HEADER_Y       14
#define HEADER_W       1880
#define HEADER_H       84

#define BLOCK_SIZE     42
#define BOARD_PIX_W    (BOARD_WIDTH * BLOCK_SIZE)   // 420
#define BOARD_PIX_H    (BOARD_HEIGHT * BLOCK_SIZE)  // 840
#define BOARD_X        750
#define BOARD_Y        118

#define LEFT_X         20
#define LEFT_Y         118
#define LEFT_W         710
#define LEFT_H         840

#define RIGHT_X        1190
#define RIGHT_Y        118
#define RIGHT_W        710
#define RIGHT_H        840

#define FOOTER_X       20
#define FOOTER_Y       972
#define FOOTER_W       1880
#define FOOTER_H       94

// Tetromino palette (Cyber-Hospital Theme)
static const SDL_Color PIECE_COLORS[8] = {
    {0, 0, 0, 0},            // 0: None
    {0, 235, 255, 255},      // 1: I (Cyan - Fibra Urgencias)
    {45, 120, 255, 255},     // 2: J (Blue - Servidor HIS)
    {255, 145, 20, 255},     // 3: L (Orange - Radiología PACS)
    {255, 220, 20, 255},     // 4: O (Yellow - Telemetría UCI)
    {25, 230, 90, 255},      // 5: S (Green - Farmacia y Lab)
    {180, 50, 250, 255},     // 6: T (Purple - Switch Central)
    {255, 45, 55, 255}       // 7: Z (Red - Firewall Perimetral)
};

// UI Palette
static const SDL_Color BG_DARK      = { 6, 12, 20, 255 };
static const SDL_Color BG_PANEL     = { 14, 24, 38, 245 };
static const SDL_Color PANEL_BORDER = { 34, 65, 105, 255 };
static const SDL_Color ACCENT_CYAN  = { 0, 235, 255, 255 };
static const SDL_Color ACCENT_GREEN = { 25, 235, 95, 255 };
static const SDL_Color ACCENT_AMBER = { 255, 190, 0, 255 };
static const SDL_Color ACCENT_RED   = { 255, 45, 55, 255 };
static const SDL_Color TEXT_WHITE   = { 245, 250, 255, 255 };
static const SDL_Color TEXT_MUTED   = { 135, 175, 210, 255 };
static const SDL_Color TEXT_DARK    = { 60, 90, 120, 255 };
static const SDL_Color TERMINAL_BG  = { 4, 8, 14, 252 };

bool render_init(SDL_Window **out_window, SDL_Renderer **out_renderer) {
    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "1");
    SDL_SetHint(SDL_HINT_RENDER_VSYNC, "1");

    SDL_Window *win = SDL_CreateWindow(
        "Sant Joan Tetris - Hospital Universitario de Sant Joan d'Alacant (1920x1080 Full HD)",
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        WIN_W,
        WIN_H,
        SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE
    );
    if (!win) {
        printf("SDL_CreateWindow Error: %s\n", SDL_GetError());
        return false;
    }

    SDL_Renderer *ren = SDL_CreateRenderer(
        win,
        -1,
        SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC
    );
    if (!ren) {
        printf("SDL_CreateRenderer Error: %s\n", SDL_GetError());
        SDL_DestroyWindow(win);
        return false;
    }

    // Lock internal coordinate space to 1920x1080
    SDL_RenderSetLogicalSize(ren, WIN_W, WIN_H);
    SDL_SetRenderDrawBlendMode(ren, SDL_BLENDMODE_BLEND);

    *out_window = win;
    *out_renderer = ren;
    return true;
}

void render_cleanup(SDL_Window *window, SDL_Renderer *renderer) {
    if (renderer) SDL_DestroyRenderer(renderer);
    if (window) SDL_DestroyWindow(window);
}

static void draw_rect_outline(SDL_Renderer *ren, int x, int y, int w, int h, SDL_Color col) {
    SDL_SetRenderDrawColor(ren, col.r, col.g, col.b, col.a);
    SDL_Rect r = { x, y, w, h };
    SDL_RenderDrawRect(ren, &r);
}

static void draw_panel(SDL_Renderer *ren, int x, int y, int w, int h, SDL_Color bg, SDL_Color border) {
    if (w <= 0 || h <= 0) return;
    SDL_SetRenderDrawColor(ren, bg.r, bg.g, bg.b, bg.a);
    SDL_Rect r = { x, y, w, h };
    SDL_RenderFillRect(ren, &r);

    draw_rect_outline(ren, x, y, w, h, border);

    // Glowing corner accent brackets
    SDL_SetRenderDrawColor(ren, ACCENT_CYAN.r, ACCENT_CYAN.g, ACCENT_CYAN.b, 210);
    int c = 8;
    SDL_RenderDrawLine(ren, x, y, x + c, y);
    SDL_RenderDrawLine(ren, x, y, x, y + c);
    SDL_RenderDrawLine(ren, x + w - 1, y, x + w - 1 - c, y);
    SDL_RenderDrawLine(ren, x + w - 1, y, x + w - 1, y + c);
    SDL_RenderDrawLine(ren, x, y + h - 1, x + c, y + h - 1);
    SDL_RenderDrawLine(ren, x, y + h - 1, x, y + h - 1 - c);
    SDL_RenderDrawLine(ren, x + w - 1, y + h - 1, x + w - 1 - c, y + h - 1);
    SDL_RenderDrawLine(ren, x + w - 1, y + h - 1, x + w - 1, y + h - 1 - c);
}

static void draw_block(SDL_Renderer *ren, int x, int y, int size, PieceType piece, bool is_ghost, bool is_clearing) {
    if (piece < 1 || piece > 7) return;
    SDL_Color c = PIECE_COLORS[piece];

    if (is_clearing) {
        SDL_SetRenderDrawColor(ren, 255, 255, 255, 245);
        SDL_Rect r = { x + 1, y + 1, size - 2, size - 2 };
        SDL_RenderFillRect(ren, &r);
        return;
    }

    if (is_ghost) {
        SDL_SetRenderDrawColor(ren, c.r, c.g, c.b, 40);
        SDL_Rect fill = { x + 2, y + 2, size - 4, size - 4 };
        SDL_RenderFillRect(ren, &fill);

        SDL_SetRenderDrawColor(ren, c.r, c.g, c.b, 170);
        SDL_Rect border = { x + 1, y + 1, size - 2, size - 2 };
        SDL_RenderDrawRect(ren, &border);
        return;
    }

    // Base block
    SDL_SetRenderDrawColor(ren, c.r, c.g, c.b, 255);
    SDL_Rect r = { x + 1, y + 1, size - 2, size - 2 };
    SDL_RenderFillRect(ren, &r);

    // Bevel highlights
    Uint8 lr = (Uint8)fminf(255.0f, c.r + 75);
    Uint8 lg = (Uint8)fminf(255.0f, c.g + 75);
    Uint8 lb = (Uint8)fminf(255.0f, c.b + 75);
    SDL_SetRenderDrawColor(ren, lr, lg, lb, 240);
    SDL_RenderDrawLine(ren, x + 1, y + 1, x + size - 2, y + 1);
    SDL_RenderDrawLine(ren, x + 1, y + 1, x + 1, y + size - 2);

    // Dark edges
    Uint8 dr = (Uint8)(c.r * 0.50f);
    Uint8 dg = (Uint8)(c.g * 0.50f);
    Uint8 db = (Uint8)(c.b * 0.50f);
    SDL_SetRenderDrawColor(ren, dr, dg, db, 240);
    SDL_RenderDrawLine(ren, x + 1, y + size - 2, x + size - 2, y + size - 2);
    SDL_RenderDrawLine(ren, x + size - 2, y + 1, x + size - 2, y + size - 2);

    // Glowing LED core
    if (size >= 16) {
        SDL_SetRenderDrawColor(ren, 255, 255, 255, 140);
        int core_pad = size / 3;
        SDL_Rect core = { x + core_pad, y + core_pad, size - core_pad * 2, size - core_pad * 2 };
        SDL_RenderFillRect(ren, &core);
    }
}

static void draw_hospital_crest(SDL_Renderer *ren, int cx, int cy, int size) {
    if (size < 16) size = 16;
    SDL_SetRenderDrawColor(ren, 18, 32, 50, 255);
    SDL_Rect bg = { cx - size / 2, cy - size / 2, size, size };
    SDL_RenderFillRect(ren, &bg);
    draw_rect_outline(ren, cx - size / 2, cy - size / 2, size, size, ACCENT_CYAN);

    int arm_w = size / 4;
    int arm_l = (size * 7) / 10;
    SDL_SetRenderDrawColor(ren, ACCENT_RED.r, ACCENT_RED.g, ACCENT_RED.b, 255);

    SDL_Rect h_arm = { cx - arm_l / 2, cy - arm_w / 2, arm_l, arm_w };
    SDL_RenderFillRect(ren, &h_arm);

    SDL_Rect v_arm = { cx - arm_w / 2, cy - arm_l / 2, arm_w, arm_l };
    SDL_RenderFillRect(ren, &v_arm);

    SDL_SetRenderDrawColor(ren, 0, 240, 255, 255);
    int dot_s = size / 6;
    if (dot_s < 2) dot_s = 2;
    SDL_Rect center_dot = { cx - dot_s / 2, cy - dot_s / 2, dot_s, dot_s };
    SDL_RenderFillRect(ren, &center_dot);
}

static void draw_ecg_monitor(SDL_Renderer *ren, int x, int y, int w, int h, float phase, float bpm) {
    if (w <= 10 || h <= 10) return;
    SDL_SetRenderDrawColor(ren, 4, 8, 14, 255);
    SDL_Rect r = { x, y, w, h };
    SDL_RenderFillRect(ren, &r);
    draw_rect_outline(ren, x, y, w, h, PANEL_BORDER);

    // Grid lines
    SDL_SetRenderDrawColor(ren, 12, 24, 38, 180);
    for (int gx = x; gx < x + w; gx += 20) {
        SDL_RenderDrawLine(ren, gx, y, gx, y + h);
    }
    for (int gy = y; gy < y + h; gy += 14) {
        SDL_RenderDrawLine(ren, x, gy, x + w, gy);
    }

    // ECG waveform synthesis
    int mid_y = y + h / 2;
    int prev_x = x;
    int prev_y = mid_y;

    SDL_SetRenderDrawColor(ren, ACCENT_GREEN.r, ACCENT_GREEN.g, ACCENT_GREEN.b, 255);

    for (int px = 0; px < w; px += 2) {
        float sample_phase = fmodf(phase * 12.0f + (float)px * 0.045f, 6.283185f);
        float wave_val = 0.0f;

        if (sample_phase > 1.2f && sample_phase < 1.6f) {
            wave_val = 0.20f * sinf((sample_phase - 1.2f) / 0.4f * 3.14159f);
        } else if (sample_phase >= 2.0f && sample_phase < 2.15f) {
            wave_val = -0.22f * sinf((sample_phase - 2.0f) / 0.15f * 3.14159f);
        } else if (sample_phase >= 2.15f && sample_phase < 2.35f) {
            wave_val = 1.0f * sinf((sample_phase - 2.15f) / 0.20f * 3.14159f);
        } else if (sample_phase >= 2.35f && sample_phase < 2.50f) {
            wave_val = -0.35f * sinf((sample_phase - 2.35f) / 0.15f * 3.14159f);
        } else if (sample_phase >= 2.8f && sample_phase < 3.4f) {
            wave_val = 0.30f * sinf((sample_phase - 2.8f) / 0.6f * 3.14159f);
        }

        int cur_x = x + px;
        int cur_y = mid_y - (int)(wave_val * (float)(h / 2 - 6));

        if (px > 0) {
            SDL_RenderDrawLine(ren, prev_x, prev_y, cur_x, cur_y);
        }
        prev_x = cur_x;
        prev_y = cur_y;
    }

    char buf[64];
    snprintf(buf, sizeof(buf), "ECG: %d BPM | FIBRA: 10G", (int)bpm);
    font_draw_text(ren, buf, x + 12, y + 6, 2, ACCENT_GREEN);
}

static void draw_visualizer_bars(SDL_Renderer *ren, int x, int y, int w, int h) {
    if (w <= 10 || h <= 6) return;
    float bars[8];
    audio_get_visualizer(bars);

    int bar_w = (w - 18) / 8;
    if (bar_w < 6) bar_w = 6;

    for (int i = 0; i < 8; i++) {
        int bx = x + i * (bar_w + 2);
        int bar_h = (int)(bars[i] * (float)h);
        if (bar_h > h) bar_h = h;
        if (bar_h < 4) bar_h = 4;

        int by = y + h - bar_h;

        SDL_Color col = ACCENT_CYAN;
        if (i < 2) col = ACCENT_RED;
        else if (i < 5) col = ACCENT_AMBER;
        else col = ACCENT_GREEN;

        SDL_SetRenderDrawColor(ren, col.r, col.g, col.b, 230);
        SDL_Rect r = { bx, by, bar_w, bar_h };
        SDL_RenderFillRect(ren, &r);
    }
}

static void draw_preview_piece(SDL_Renderer *ren, PieceType piece, int cx, int cy, int size) {
    if (piece == PIECE_NONE) return;
    int p_size = game_get_piece_size(piece);

    int start_x = cx - (p_size * size) / 2;
    int start_y = cy - (p_size * size) / 2;

    for (int r = 0; r < p_size; r++) {
        for (int c = 0; c < p_size; c++) {
            if (game_get_piece_cell(piece, 0, r, c)) {
                draw_block(ren, start_x + c * size, start_y + r * size, size, piece, false, false);
            }
        }
    }
}

static void render_header(SDL_Renderer *ren, const Game *g) {
    draw_panel(ren, HEADER_X, HEADER_Y, HEADER_W, HEADER_H, BG_PANEL, PANEL_BORDER);

    // Hospital Crest
    draw_hospital_crest(ren, 64, HEADER_Y + HEADER_H / 2, 56);

    // Large Header Hospital Titles (Scale 3 and Scale 2)
    font_draw_text_shadow(ren, "HOSPITAL UNIVERSITARIO DE SANT JOAN D'ALACANT", 110, HEADER_Y + 14, 3, TEXT_WHITE, BG_DARK);
    font_draw_text(ren, "UNIDAD DE INFORMATICA Y TIC  |  GUARDIA 24H  -  CENTRO DE PROCESO DE DATOS", 110, HEADER_Y + 48, 2, ACCENT_CYAN);

    // Live ECG Strip (Expanded to 440px to ensure text and graph fit comfortably)
    draw_ecg_monitor(ren, 1430, HEADER_Y + 10, 440, 64, g->ecg_phase, g->ecg_bpm);
}

static void render_left_panel(SDL_Renderer *ren, const Game *g) {
    draw_panel(ren, LEFT_X, LEFT_Y, LEFT_W, LEFT_H, BG_PANEL, PANEL_BORDER);

    // 1. Box: Identificación del Puesto TIC (Scale 4 y Scale 2)
    draw_panel(ren, LEFT_X + 16, LEFT_Y + 14, LEFT_W - 32, 106, TERMINAL_BG, PANEL_BORDER);
    font_draw_text_centered_shadow(ren, "SANT JOAN TETRIS", LEFT_X + LEFT_W / 2, LEFT_Y + 24, 4, ACCENT_CYAN, BG_DARK);
    char guard_line[64];
    snprintf(guard_line, sizeof(guard_line), "GUARDIA DE HOY: %s", game_get_current_guardia(g));
    font_draw_text_centered(ren, guard_line, LEFT_X + LEFT_W / 2, LEFT_Y + 62, 2, ACCENT_GREEN);
    font_draw_text_centered(ren, "CPD Sotano 1  |  Relevo cada 25 piezas", LEFT_X + LEFT_W / 2, LEFT_Y + 86, 2, TEXT_MUTED);

    // 2. Box: HOLD / Memoria Caché (Tecla C)
    int hold_y = LEFT_Y + 130;
    draw_panel(ren, LEFT_X + 16, hold_y, LEFT_W - 32, 175, TERMINAL_BG, PANEL_BORDER);
    font_draw_text_centered(ren, "[ HOLD / CACHE ]  (Tecla C / Shift)", LEFT_X + LEFT_W / 2, hold_y + 12, 2, ACCENT_CYAN);
    if (g->hold_piece != PIECE_NONE) {
        draw_preview_piece(ren, g->hold_piece, LEFT_X + LEFT_W / 2, hold_y + 88, 30);
    } else {
        font_draw_text_centered(ren, "- VACIO -", LEFT_X + LEFT_W / 2, hold_y + 80, 3, TEXT_DARK);
    }
    font_draw_text_centered(ren, "Guarda un bloque de emergencia...", LEFT_X + LEFT_W / 2, hold_y + 146, 2, TEXT_MUTED);

    // 3. Box: Incidencias de la Guardia (Rotativas segun el turno activo)
    int sys_y = hold_y + 188;
    draw_panel(ren, LEFT_X + 16, sys_y, LEFT_W - 32, 255, TERMINAL_BG, PANEL_BORDER);
    font_draw_text(ren, "INCIDENCIAS DE LA GUARDIA:", LEFT_X + 32, sys_y + 14, 2, ACCENT_AMBER);

    int cur_shift = game_get_current_guardia_index(g);
    const char *status_sets[4][5][2] = {
        // Turno 0 (Jose Maria)
        {
            { "Urgencias: Atasco papel",   "[DE SIEMPRE]" },
            { "Planta 3: Raton suelto",    "[DESENCHUFADO]" },
            { "UCI: Cable red quitado",    "[CARGAR MOVIL]" },
            { "Consultas: 'Borre todo'",   "[MINIMIZADO]" },
            { "Cafetera CPD: Presion",     "[100% OK]" }
        },
        // Turno 1 (Marichu)
        {
            { "Quirofano: Pantalla girada","[90 GRADOS]" },
            { "Farmacia: Teclado cafe",    "[PEGAJOSO]" },
            { "Admision: Pulsera rota",    "[DOBLE TICKET]" },
            { "Planta 4: 'No hay internet'","[SIN WIFI]" },
            { "Router CPD: Temperatura",   "[16C FRESQUITO]" }
        },
        // Turno 2 (Diego)
        {
            { "Radiologia: TAC sin red",   "[ENCHUFE FLOJO]" },
            { "Triaje: Impresora huelga",  "[FALTA PAPEL]" },
            { "Archivos: Archivo borrado", "[EN PAPELERA]" },
            { "Planta 2: Regleta apagada", "[DADA CON PIE]" },
            { "Fibra Optica: Troncal",     "[10 GBPS OK]" }
        },
        // Turno 3 (Ernesto)
        {
            { "Direccion: 'Se cayo red'",  "[CABLE SUELTO]" },
            { "Laboratorio: Centrifuga",   "[NO ES IMPRESORA]" },
            { "UCI: Clave olvidada",       "[EN POST-IT]" },
            { "Consultas: Raton sin pila", "[CAMBIADA 04AM]" },
            { "SAI Central: Baterias",     "[CARGADAS 100%]" }
        }
    };

    for (int i = 0; i < 5; i++) {
        int ry = sys_y + 44 + i * 38;
        font_draw_text(ren, status_sets[cur_shift][i][0], LEFT_X + 32, ry, 2, TEXT_WHITE);

        SDL_Color tag_col = ACCENT_GREEN;
        if (i == 0) tag_col = ACCENT_AMBER;
        else if (i == 2) tag_col = ACCENT_RED;
        else if (i == 3) tag_col = ACCENT_CYAN;

        font_draw_text_right(ren, status_sets[cur_shift][i][1], LEFT_X + LEFT_W - 32, ry, 2, tag_col);
    }

    // 4. Box: Hilo Musical del CPD
    int music_y = sys_y + 268;
    draw_panel(ren, LEFT_X + 16, music_y, LEFT_W - 32, 238, TERMINAL_BG, PANEL_BORDER);
    font_draw_text(ren, "HILO MUSICAL DEL CPD (SANT JOAN):", LEFT_X + 32, music_y + 14, 2, ACCENT_CYAN);

    draw_visualizer_bars(ren, LEFT_X + 32, music_y + 42, LEFT_W - 64, 65);

    font_draw_text(ren, audio_get_track_name(), LEFT_X + 32, music_y + 120, 2, TEXT_WHITE);
    font_draw_text(ren, "[T] Pista   [M] Mute   [+/-] Volumen", LEFT_X + 32, music_y + 152, 2, ACCENT_AMBER);
    font_draw_text(ren, "Sintesis para no dormirse en guardia", LEFT_X + 32, music_y + 182, 2, TEXT_MUTED);
}

static void render_playfield(SDL_Renderer *ren, const Game *g) {
    // Cyber border around the board
    draw_panel(ren, BOARD_X - 8, BOARD_Y - 8, BOARD_PIX_W + 16, BOARD_PIX_H + 16, BG_DARK, PANEL_BORDER);

    SDL_SetRenderDrawColor(ren, 8, 14, 22, 255);
    SDL_Rect board_rect = { BOARD_X, BOARD_Y, BOARD_PIX_W, BOARD_PIX_H };
    SDL_RenderFillRect(ren, &board_rect);

    // Grid lines (every 42 px)
    SDL_SetRenderDrawColor(ren, 18, 30, 46, 180);
    for (int c = 1; c < BOARD_WIDTH; c++) {
        SDL_RenderDrawLine(ren, BOARD_X + c * BLOCK_SIZE, BOARD_Y, BOARD_X + c * BLOCK_SIZE, BOARD_Y + BOARD_PIX_H);
    }
    for (int r = 1; r < BOARD_HEIGHT; r++) {
        SDL_RenderDrawLine(ren, BOARD_X, BOARD_Y + r * BLOCK_SIZE, BOARD_X + BOARD_PIX_W, BOARD_Y + r * BLOCK_SIZE);
    }

    // Draw locked blocks
    for (int r = 0; r < BOARD_HEIGHT; r++) {
        int board_row = r + BUFFER_HEIGHT;
        bool is_clearing = false;
        for (int k = 0; k < g->num_clearing_rows; k++) {
            if (g->clearing_rows[k] == board_row) {
                is_clearing = true;
                break;
            }
        }

        for (int c = 0; c < BOARD_WIDTH; c++) {
            PieceType p = (PieceType)g->board[board_row][c];
            if (p != PIECE_NONE) {
                draw_block(ren, BOARD_X + c * BLOCK_SIZE, BOARD_Y + r * BLOCK_SIZE, BLOCK_SIZE, p, false, is_clearing);
            }
        }
    }

    // Draw ghost piece and active piece
    if (g->state == STATE_PLAY && g->current_piece != PIECE_NONE && !g->clearing_lines) {
        int ghost_y = game_get_ghost_y(g);
        int size = game_get_piece_size(g->current_piece);

        // Ghost piece
        for (int r = 0; r < size; r++) {
            for (int c = 0; c < size; c++) {
                if (game_get_piece_cell(g->current_piece, g->piece_rot, r, c)) {
                    int by = ghost_y + r;
                    int bx = g->piece_x + c;
                    if (by >= BUFFER_HEIGHT && by < TOTAL_ROWS && bx >= 0 && bx < BOARD_WIDTH) {
                        int scr_y = BOARD_Y + (by - BUFFER_HEIGHT) * BLOCK_SIZE;
                        int scr_x = BOARD_X + bx * BLOCK_SIZE;
                        draw_block(ren, scr_x, scr_y, BLOCK_SIZE, g->current_piece, true, false);
                    }
                }
            }
        }

        // Active falling piece
        for (int r = 0; r < size; r++) {
            for (int c = 0; c < size; c++) {
                if (game_get_piece_cell(g->current_piece, g->piece_rot, r, c)) {
                    int by = g->piece_y + r;
                    int bx = g->piece_x + c;
                    if (by >= BUFFER_HEIGHT && by < TOTAL_ROWS && bx >= 0 && bx < BOARD_WIDTH) {
                        int scr_y = BOARD_Y + (by - BUFFER_HEIGHT) * BLOCK_SIZE;
                        int scr_x = BOARD_X + bx * BLOCK_SIZE;
                        draw_block(ren, scr_x, scr_y, BLOCK_SIZE, g->current_piece, false, false);
                    }
                }
            }
        }
    }

    // Particles
    for (int i = 0; i < MAX_PARTICLES; i++) {
        if (g->particles[i].life > 0.0f) {
            const Particle *p = &g->particles[i];
            int px = BOARD_X + (int)(p->x * (float)BLOCK_SIZE);
            int py = BOARD_Y + (int)(p->y * (float)BLOCK_SIZE);
            int ps = (int)(p->size * 5.0f);
            if (ps < 3) ps = 3;

            SDL_SetRenderDrawColor(ren, p->r, p->g, p->b, (Uint8)(p->a * p->life));
            SDL_Rect pr = { px, py, ps, ps };
            SDL_RenderFillRect(ren, &pr);
        }
    }

    // Floating Toast Notification in Huge Scale 4 (32px!)
    if (g->toast.alpha > 0.0f) {
        SDL_Color tc = { g->toast.r, g->toast.g, g->toast.b, (Uint8)(255 * g->toast.alpha) };
        SDL_Color sc = { 0, 0, 0, (Uint8)(230 * g->toast.alpha) };
        int toast_y = BOARD_Y + BOARD_PIX_H / 2 - 25;
        font_draw_text_centered_shadow(ren, g->toast.text, BOARD_X + BOARD_PIX_W / 2, toast_y, 4, tc, sc);
    }
}

static void render_ticker_entry(SDL_Renderer *ren, const char *msg, int x, int y) {
    if (!msg || !msg[0]) return;
    const char *nl = strchr(msg, '\n');
    if (nl) {
        char line1[64];
        char line2[64];
        size_t len1 = (size_t)(nl - msg);
        if (len1 >= sizeof(line1)) len1 = sizeof(line1) - 1;
        strncpy(line1, msg, len1);
        line1[len1] = '\0';
        if (strlen(line1) > 37) line1[37] = '\0';

        const char *rest = nl + 1;
        while (*rest == ' ' && *(rest + 1) == ' ') rest++;
        strncpy(line2, rest, sizeof(line2) - 1);
        line2[sizeof(line2) - 1] = '\0';
        if (strlen(line2) > 37) line2[37] = '\0';

        font_draw_text(ren, line1, x, y, 2, ACCENT_CYAN);
        font_draw_text(ren, line2, x, y + 22, 2, TEXT_MUTED);
    } else {
        int len = (int)strlen(msg);
        if (len <= 37) {
            font_draw_text(ren, msg, x, y + 10, 2, TEXT_MUTED);
        } else {
            int split = 36;
            while (split > 15 && msg[split] != ' ') {
                split--;
            }
            if (split <= 15) split = 36;

            char line1[64];
            char line2[64];
            strncpy(line1, msg, split);
            line1[split] = '\0';
            if (strlen(line1) > 37) line1[37] = '\0';

            const char *rest = msg + split;
            if (*rest == ' ') rest++;
            strncpy(line2, rest, sizeof(line2) - 1);
            line2[sizeof(line2) - 1] = '\0';
            if (strlen(line2) > 37) line2[37] = '\0';

            font_draw_text(ren, line1, x, y, 2, ACCENT_CYAN);
            font_draw_text(ren, line2, x, y + 22, 2, TEXT_MUTED);
        }
    }
}

static void render_right_panel(SDL_Renderer *ren, const Game *g) {
    draw_panel(ren, RIGHT_X, RIGHT_Y, RIGHT_W, RIGHT_H, BG_PANEL, PANEL_BORDER);

    // 1. Box: Siguientes Peticiones (NEXT)
    draw_panel(ren, RIGHT_X + 16, RIGHT_Y + 14, RIGHT_W - 32, 175, TERMINAL_BG, PANEL_BORDER);
    font_draw_text_centered(ren, "[ PROXIMAS INCIDENCIAS ENTRANTES ]", RIGHT_X + RIGHT_W / 2, RIGHT_Y + 14, 2, ACCENT_CYAN);

    int col_w = (RIGHT_W - 32) / 3;
    for (int i = 0; i < 3; i++) {
        int preview_cx = RIGHT_X + 16 + i * col_w + col_w / 2;
        int preview_cy = RIGHT_Y + 88;
        draw_preview_piece(ren, g->next_queue[i], preview_cx, preview_cy, 28);
    }
    font_draw_text_centered(ren, "3 marrones mas entrando por Urgencias", RIGHT_X + RIGHT_W / 2, RIGHT_Y + 148, 2, TEXT_MUTED);

    // 2. Box: Estadísticas del Turno (Score en Scale 6!)
    int stats_y = RIGHT_Y + 200;
    draw_panel(ren, RIGHT_X + 16, stats_y, RIGHT_W - 32, 280, TERMINAL_BG, PANEL_BORDER);

    font_draw_text(ren, "INCIDENCIAS RESUELTAS (PUNTOS):", RIGHT_X + 32, stats_y + 14, 2, ACCENT_CYAN);

    // Giant Score in Scale 6 (48px tall!)
    char score_buf[32];
    snprintf(score_buf, sizeof(score_buf), "%07u", g->score);
    font_draw_text_shadow(ren, score_buf, RIGHT_X + 32, stats_y + 40, 6, TEXT_WHITE, BG_DARK);

    char hs_buf[48];
    snprintf(hs_buf, sizeof(hs_buf), "Record de la Guardia: %07u", g->highscore);
    font_draw_text(ren, hs_buf, RIGHT_X + 32, stats_y + 102, 2, ACCENT_AMBER);

    char lines_buf[48];
    snprintf(lines_buf, sizeof(lines_buf), "Marrones esquivados (Lineas): %d", g->lines);
    font_draw_text(ren, lines_buf, RIGHT_X + 32, stats_y + 130, 2, TEXT_WHITE);

    char lvl_buf[64];
    snprintf(lvl_buf, sizeof(lvl_buf), "Nivel %d - Servicio:", g->level);
    font_draw_text(ren, lvl_buf, RIGHT_X + 32, stats_y + 158, 2, ACCENT_GREEN);
    font_draw_text_shadow(ren, game_get_department_name(g->level), RIGHT_X + 32, stats_y + 184, 3, TEXT_WHITE, BG_DARK);

    char time_buf[64];
    int mins = g->play_duration_sec / 60;
    int secs = g->play_duration_sec % 60;
    snprintf(time_buf, sizeof(time_buf), "Guardia: %s  (%02d:%02d min)", game_get_current_guardia(g), mins, secs);
    font_draw_text(ren, time_buf, RIGHT_X + 32, stats_y + 222, 2, ACCENT_CYAN);

    char pieces_buf[64];
    int p_in_shift = g->pieces_dropped % 25;
    snprintf(pieces_buf, sizeof(pieces_buf), "Turno activo: %d/25 piezas para relevo", p_in_shift);
    font_draw_text(ren, pieces_buf, RIGHT_X + 32, stats_y + 248, 2, TEXT_MUTED);

    // 3. Box: Terminal de Incidencias en Vivo (Scale 2, 5 tickets completos con tag y detalle)
    int tick_y = stats_y + 295;
    draw_panel(ren, RIGHT_X + 16, tick_y, RIGHT_W - 32, 335, TERMINAL_BG, PANEL_BORDER);
    font_draw_text(ren, ">_ LOG DE INCIDENCIAS (SANT JOAN 24H):", RIGHT_X + 32, tick_y + 14, 2, ACCENT_GREEN);

    int max_visible = 5;
    int start_msg = (g->ticker_count > max_visible) ? (g->ticker_count - max_visible) : 0;
    for (int i = start_msg; i < g->ticker_count; i++) {
        int entry_y = tick_y + 46 + (i - start_msg) * 54;
        if (entry_y + 44 < tick_y + 335) {
            render_ticker_entry(ren, g->ticker_msgs[i], RIGHT_X + 32, entry_y);
        }
    }
}

static void render_footer(SDL_Renderer *ren) {
    draw_panel(ren, FOOTER_X, FOOTER_Y, FOOTER_W, FOOTER_H, BG_PANEL, PANEL_BORDER);

    // Subtle divider separating controls and hospital info
    SDL_SetRenderDrawColor(ren, PANEL_BORDER.r, PANEL_BORDER.g, PANEL_BORDER.b, 180);
    SDL_RenderDrawLine(ren, 1180, FOOTER_Y + 12, 1180, FOOTER_Y + FOOTER_H - 12);

    // Left Section: Controls (Scale 2)
    font_draw_text(ren, "[ESPACIO] Caida rapida   [C/Shift] Hold   [P] Pausa   [R] Reiniciar", FOOTER_X + 28, FOOTER_Y + 18, 2, TEXT_WHITE);
    font_draw_text(ren, "[Flechas / WASD] Movimiento y Giro   [T] Musica   [M] Mute   [F1] Manual", FOOTER_X + 28, FOOTER_Y + 54, 2, TEXT_MUTED);

    // Right Section: Hospital info & on-call quote (Scale 2)
    font_draw_text_right(ren, "Hospital Univ. Sant Joan d'Alacant", FOOTER_X + FOOTER_W - 28, FOOTER_Y + 18, 2, ACCENT_CYAN);
    font_draw_text_right(ren, "\"¿Ha probado a apagarlo y encenderlo?\"", FOOTER_X + FOOTER_W - 28, FOOTER_Y + 54, 2, ACCENT_AMBER);
}

static void render_title_screen(SDL_Renderer *ren, const Game *g) {
    SDL_SetRenderDrawColor(ren, 6, 12, 20, 250);
    SDL_Rect bg = { 0, 0, WIN_W, WIN_H };
    SDL_RenderFillRect(ren, &bg);

    int mid_x = WIN_W / 2;

    // Big Hospital Crest
    draw_hospital_crest(ren, mid_x, 110, 84);

    // Main Title in Scale 6 (48px tall)
    font_draw_text_centered_shadow(ren, "S A N T   J O A N   T E T R I S", mid_x, 175, 6, ACCENT_CYAN, BG_DARK);

    // Subtitles in Scale 3 and Scale 2
    font_draw_text_centered_shadow(ren, "HOSPITAL UNIVERSITARIO DE SANT JOAN D'ALACANT", mid_x, 245, 3, TEXT_WHITE, BG_DARK);
    font_draw_text_centered(ren, "UNIDAD DE INFORMATICA Y TECNOLOGIAS DE LA INFORMACION (TIC)", mid_x, 282, 2, ACCENT_GREEN);
    font_draw_text_centered(ren, "Simulador de Guardia Nocturna: Sobrevive a las impresoras y al cafe frio", mid_x, 312, 2, ACCENT_AMBER);

    // Menu Buttons in Scale 3
    int menu_y = 380;
    const char *options[3] = {
        "1. ENTRAR DE GUARDIA (COMENZAR TURNO)",
        "2. MANUAL DE SUPERVIVENCIA DEL HOSPITAL",
        "3. SALIR CORRIENDO DEL CPD"
    };

    int opt_w = 980;
    for (int i = 0; i < 3; i++) {
        int oy = menu_y + i * 74;
        bool selected = (g->menu_selected == i);

        if (selected) {
            draw_panel(ren, mid_x - opt_w / 2, oy - 8, opt_w, 62, BG_PANEL, ACCENT_CYAN);
            font_draw_text_centered_shadow(ren, options[i], mid_x, oy + 12, 3, ACCENT_CYAN, BG_DARK);
        } else {
            font_draw_text_centered(ren, options[i], mid_x, oy + 12, 3, TEXT_MUTED);
        }
    }

    int foot_y = 650;
    font_draw_text_centered(ren, "Usa Flechas o W/S para seleccionar  |  ENTER o ESPACIO para confirmar", mid_x, foot_y, 2, TEXT_WHITE);
    font_draw_text_centered(ren, "[T] Pista Musical   [M] Silenciar Musica   [+/-] Regular Volumen", mid_x, foot_y + 38, 2, ACCENT_AMBER);

    // Tiny discreet reference ONLY here at the bottom of the title screen with year 2026
    font_draw_text_centered(ren, "Concepto original y prompt: Miguel Angel Esteban - aka MA (2026)", mid_x, 1045, 1, TEXT_MUTED);
}

static void render_pause_screen(SDL_Renderer *ren) {
    SDL_SetRenderDrawColor(ren, 6, 12, 20, 235);
    SDL_Rect bg = { 0, 0, WIN_W, WIN_H };
    SDL_RenderFillRect(ren, &bg);

    int pw = 1050;
    int ph = 380;
    int px = (WIN_W - pw) / 2;
    int py = (WIN_H - ph) / 2;

    draw_panel(ren, px, py, pw, ph, BG_PANEL, ACCENT_CYAN);

    font_draw_text_centered_shadow(ren, "GUARDIA EN PAUSA", WIN_W / 2, py + 35, 5, ACCENT_CYAN, BG_DARK);
    font_draw_text_centered(ren, "El informatico de guardia ha bajado a por un cafe a Urgencias.", WIN_W / 2, py + 115, 2, TEXT_WHITE);
    font_draw_text_centered(ren, "Reza para que ningun switch eche humo mientras tanto...", WIN_W / 2, py + 155, 2, ACCENT_AMBER);

    font_draw_text_centered(ren, "[P / ESC] Reanudar Turno", WIN_W / 2, py + 225, 3, ACCENT_GREEN);
    font_draw_text_centered(ren, "[R] Reiniciar Guardia   |   [Q] Salir al Menu", WIN_W / 2, py + 290, 2, TEXT_MUTED);
}

static void render_gameover_screen(SDL_Renderer *ren, const Game *g) {
    SDL_SetRenderDrawColor(ren, 14, 6, 10, 245);
    SDL_Rect bg = { 0, 0, WIN_W, WIN_H };
    SDL_RenderFillRect(ren, &bg);

    int pw = 1150;
    int ph = 540;
    int px = (WIN_W - pw) / 2;
    int py = (WIN_H - ph) / 2;

    draw_panel(ren, px, py, pw, ph, BG_PANEL, ACCENT_RED);

    font_draw_text_centered_shadow(ren, "¡¡COLAPSO TOTAL DEL HOSPITAL!!", WIN_W / 2, py + 35, 4, ACCENT_RED, BG_DARK);
    font_draw_text_centered(ren, "Se ha caido la red. Los medicos sacan papel y boligrafo BIC.", WIN_W / 2, py + 105, 2, TEXT_WHITE);

    char buf[64];
    snprintf(buf, sizeof(buf), "Tickets Resueltos: %u", g->score);
    font_draw_text_centered_shadow(ren, buf, WIN_W / 2, py + 155, 4, ACCENT_CYAN, BG_DARK);

    snprintf(buf, sizeof(buf), "Marrones Esquivados: %d lineas", g->lines);
    font_draw_text_centered(ren, buf, WIN_W / 2, py + 210, 3, TEXT_WHITE);

    snprintf(buf, sizeof(buf), "Servicio de la catastrofe: %s", game_get_department_name(g->level));
    font_draw_text_centered(ren, buf, WIN_W / 2, py + 255, 3, ACCENT_AMBER);

    int mins = g->play_duration_sec / 60;
    int secs = g->play_duration_sec % 60;
    snprintf(buf, sizeof(buf), "Tiempo resistiendo en la trinchera: %02d:%02d min", mins, secs);
    font_draw_text_centered(ren, buf, WIN_W / 2, py + 300, 2, TEXT_MUTED);

    font_draw_text_centered_shadow(ren, "[R / ESPACIO] Fichar para una Nueva Guardia", WIN_W / 2, py + 380, 3, ACCENT_GREEN, BG_DARK);
    font_draw_text_centered(ren, "[ESC] Volver al Menu Principal", WIN_W / 2, py + 445, 2, TEXT_MUTED);
}

static void render_about_screen(SDL_Renderer *ren) {
    SDL_SetRenderDrawColor(ren, 6, 12, 20, 252);
    SDL_Rect bg = { 0, 0, WIN_W, WIN_H };
    SDL_RenderFillRect(ren, &bg);

    int pw = 1550;
    int ph = 920;
    int px = (WIN_W - pw) / 2;
    int py = (WIN_H - ph) / 2;

    draw_panel(ren, px, py, pw, ph, BG_PANEL, ACCENT_CYAN);

    draw_hospital_crest(ren, px + 70, py + 60, 60);
    font_draw_text_shadow(ren, "MANUAL DE SUPERVIVENCIA: EL INFORMATICO DE GUARDIA", px + 120, py + 35, 3, ACCENT_CYAN, BG_DARK);
    font_draw_text(ren, "HOSPITAL UNIVERSITARIO DE SANT JOAN D'ALACANT  -  CPD SOTANO 1", px + 120, py + 72, 2, TEXT_WHITE);

    int ty = py + 120;

    // Reglas de oro del turno (Humor de guardias reales)
    draw_panel(ren, px + 35, ty, pw - 70, 480, TERMINAL_BG, PANEL_BORDER);
    font_draw_text(ren, "PROTOCOLO OFICIAL DE ACTUACION ANTE INCIDENCIAS:", px + 60, ty + 18, 3, ACCENT_AMBER);

    const char *rules[] = {
        "1. Pregunta siempre: '¿Ha probado a apagarlo y volverlo a encender?' (Soluciona el 90%).",
        "2. Si el usuario jura que 'no toco nada', ha tocado todos los cables del despacho.",
        "3. La cafetera del CPD es soporte vital basico: Uptime requerido del 100.00%.",
        "4. La impresora de Urgencias no esta rota: solo tiene hambre voraz de folios.",
        "5. Si te llaman a las 4 AM por la pila del raton, respira hondo y cuenta hasta diez.",
        "6. Cada linea que completas en el juego es un ticket absurdo que consigues cerrar.",
        "7. Si parpadea una luz roja en el rack a las 5:30 AM, tapala con un post-it.",
        "8. Turno rotativo cada 25 piezas: Jose Maria, Marichu, Diego y Ernesto."
    };

    for (int i = 0; i < 8; i++) {
        font_draw_text(ren, rules[i], px + 60, ty + 64 + i * 50, 2, TEXT_WHITE);
    }

    // Controles
    int c_y = ty + 505;
    draw_panel(ren, px + 35, c_y, pw - 70, 180, TERMINAL_BG, PANEL_BORDER);
    font_draw_text(ren, "CONTROLES DEL TURNO DE GUARDIA:", px + 60, c_y + 16, 3, ACCENT_CYAN);
    font_draw_text(ren, "Mover: Flechas Izq/Der o A/D   |   Rotar: Flecha Arriba o W / Z / Q", px + 60, c_y + 58, 2, TEXT_WHITE);
    font_draw_text(ren, "Caida Rapida: Barra Espaciadora   |   Caida Suave: Flecha Abajo o S", px + 60, c_y + 92, 2, TEXT_WHITE);
    font_draw_text(ren, "Guardar (Hold): Tecla C o Shift   |   Musica: T (Pista), M (Mute), +/- (Vol)", px + 60, c_y + 126, 2, ACCENT_AMBER);

    font_draw_text_centered_shadow(ren, "Presiona [ESC] o [F1 / H] para volver al Menu Principal", WIN_W / 2, py + ph - 40, 3, ACCENT_GREEN, BG_DARK);
}

void render_frame(SDL_Renderer *renderer, const Game *g) {
    SDL_SetRenderDrawColor(renderer, BG_DARK.r, BG_DARK.g, BG_DARK.b, 255);
    SDL_RenderClear(renderer);
    SDL_RenderSetViewport(renderer, NULL); // Sin vibracion de pantalla

    render_header(renderer, g);
    render_left_panel(renderer, g);
    render_playfield(renderer, g);
    render_right_panel(renderer, g);
    render_footer(renderer);

    if (g->state == STATE_TITLE) {
        render_title_screen(renderer, g);
    } else if (g->state == STATE_PAUSE) {
        render_pause_screen(renderer);
    } else if (g->state == STATE_GAMEOVER) {
        render_gameover_screen(renderer, g);
    } else if (g->state == STATE_ABOUT) {
        render_about_screen(renderer);
    }

    SDL_RenderPresent(renderer);
}

void render_take_screenshot(SDL_Renderer *renderer, const Game *g, const char *bmp_path) {
    SDL_SetRenderDrawColor(renderer, BG_DARK.r, BG_DARK.g, BG_DARK.b, 255);
    SDL_RenderClear(renderer);
    SDL_RenderSetViewport(renderer, NULL);

    render_header(renderer, g);
    render_left_panel(renderer, g);
    render_playfield(renderer, g);
    render_right_panel(renderer, g);
    render_footer(renderer);

    if (g->state == STATE_TITLE) {
        render_title_screen(renderer, g);
    } else if (g->state == STATE_PAUSE) {
        render_pause_screen(renderer);
    } else if (g->state == STATE_GAMEOVER) {
        render_gameover_screen(renderer, g);
    } else if (g->state == STATE_ABOUT) {
        render_about_screen(renderer);
    }

    SDL_Surface *surface = SDL_CreateRGBSurfaceWithFormat(0, WIN_W, WIN_H, 32, SDL_PIXELFORMAT_ARGB8888);
    if (surface) {
        SDL_RenderReadPixels(renderer, NULL, SDL_PIXELFORMAT_ARGB8888, surface->pixels, surface->pitch);
        SDL_SaveBMP(surface, bmp_path);
        SDL_FreeSurface(surface);
    }
    SDL_RenderPresent(renderer);
}
