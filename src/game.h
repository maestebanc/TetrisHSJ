#ifndef GAME_H
#define GAME_H

#include <SDL2/SDL.h>
#include <stdbool.h>
#include <stdint.h>

#define BOARD_WIDTH 10
#define BOARD_HEIGHT 20
#define BUFFER_HEIGHT 4
#define TOTAL_ROWS (BOARD_HEIGHT + BUFFER_HEIGHT)

#define MAX_PARTICLES 160
#define MAX_TICKER_MSGS 8
#define TICKER_MSG_LEN 128

typedef enum {
    PIECE_NONE = 0,
    PIECE_I = 1,
    PIECE_J = 2,
    PIECE_L = 3,
    PIECE_O = 4,
    PIECE_S = 5,
    PIECE_T = 6,
    PIECE_Z = 7
} PieceType;

typedef enum {
    STATE_TITLE,
    STATE_PLAY,
    STATE_PAUSE,
    STATE_GAMEOVER,
    STATE_ABOUT
} GameState;

typedef struct {
    float x, y;     // Coordinates in grid space (col 0..10, row 0..20)
    float vx, vy;   // Velocities in grid units per second
    float life;     // 1.0 -> 0.0
    float decay;
    uint8_t r, g, b, a;
    float size;     // Scale multiplier
} Particle;

typedef struct {
    char text[64];
    float alpha;    // 1.0 -> 0.0
    float scale;
    uint8_t r, g, b;
} ToastBanner;

typedef struct {
    int board[TOTAL_ROWS][BOARD_WIDTH];

    PieceType current_piece;
    int piece_x;
    int piece_y;
    int piece_rot;

    PieceType hold_piece;
    bool can_hold;

    PieceType next_queue[5];
    int bag[7];
    int bag_idx;

    uint32_t score;
    uint32_t highscore;
    int lines;
    int level;
    int combo;
    bool back_to_back;
    int pieces_dropped;
    int guardia_offset; // desplaza qué informático empieza el turno (aleatorio por partida)

    // Timing
    uint32_t last_fall_time;
    uint32_t lock_timer;
    bool is_locking;
    int lock_resets;

    // Line clearing animation
    bool clearing_lines;
    int clearing_rows[4];
    int num_clearing_rows;
    uint32_t clear_anim_start;

    // Particles & FX
    Particle particles[MAX_PARTICLES];
    ToastBanner toast;
    float screen_shake;

    // Hospital IT Telemetry
    float ecg_phase;
    float ecg_bpm;
    float cpu_load;
    float net_traffic;
    char ticker_msgs[MAX_TICKER_MSGS][TICKER_MSG_LEN];
    int ticker_count;
    uint32_t last_ticker_time;

    // Game states
    GameState state;
    GameState prev_state;
    int menu_selected;
    uint32_t shift_start_time;
    uint32_t play_duration_sec;

    // Controls state for DAS (Delayed Auto Shift)
    bool key_left_held;
    bool key_right_held;
    bool key_down_held;
    uint32_t key_left_timer;
    uint32_t key_right_timer;
    uint32_t key_down_timer;
} Game;

void game_init(Game *g);
void game_reset(Game *g);
void game_update(Game *g, uint32_t delta_ms);

// Input handling
void game_handle_key_down(Game *g, int keycode);
void game_handle_key_up(Game *g, int keycode);

// Movement & Actions
bool game_move(Game *g, int dx, int dy);
void game_rotate(Game *g, int dir); // 1 = CW, -1 = CCW
void game_hard_drop(Game *g);
void game_hold(Game *g);

// Ghost piece calculation
int game_get_ghost_y(const Game *g);

// Particles & Hospital ticker
void game_spawn_particles(Game *g, float grid_x, float grid_y, int count, uint8_t r, uint8_t g_col, uint8_t b);
void game_add_ticker(Game *g, const char *msg);
void game_set_toast(Game *g, const char *text, uint8_t r, uint8_t g_col, uint8_t b);

// High score
void game_load_highscore(Game *g);
void game_save_highscore(const Game *g);

// Hospital department name by level
const char *game_get_department_name(int level);
int game_get_piece_cell(PieceType piece, int rot, int r, int c);
int game_get_piece_size(PieceType piece);

// Hospital guardia team
const char *game_get_current_guardia(const Game *g);
int game_get_current_guardia_index(const Game *g);
const char *game_get_guardia_name(int idx);

#endif // GAME_H
