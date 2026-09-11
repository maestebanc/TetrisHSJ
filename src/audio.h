#ifndef AUDIO_H
#define AUDIO_H

#include <stdbool.h>

typedef enum {
    SFX_MOVE = 0,
    SFX_ROTATE,
    SFX_SOFT_DROP,
    SFX_HARD_DROP,
    SFX_HOLD,
    SFX_LINE_CLEAR,
    SFX_TETRIS,
    SFX_LEVELUP,
    SFX_GAMEOVER,
    SFX_COUNT
} SoundEffect;

#define MAX_TRACKS 3

bool audio_init(void);
void audio_cleanup(void);

void audio_play_sfx(SoundEffect sfx);

void audio_toggle_music(void);
bool audio_is_music_enabled(void);

void audio_next_track(void);
int audio_get_track_index(void);
const char *audio_get_track_name(void);

void audio_volume_up(void);
void audio_volume_down(void);
float audio_get_volume(void);

// Set tempo multiplier based on game level (e.g. 1.0 to 1.35)
void audio_set_level_tempo(int level);

// Get real-time audio visualizer frequency levels (0.0 to 1.0)
void audio_get_visualizer(float bars[8]);

#endif // AUDIO_H
