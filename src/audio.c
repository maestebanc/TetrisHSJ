#include "audio.h"
#include <SDL2/SDL.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

#define SAMPLE_RATE 44100
#define PI 3.14159265358979323846f
#define TWO_PI (2.0f * PI)

static SDL_AudioDeviceID audio_device = 0;
static bool music_enabled = true;
static float master_volume = 0.85f;
static int current_track = 0;
static float level_tempo_mult = 1.0f;

// Visualizer levels (8 bands)
static float vis_levels[8] = {0};
static float vis_targets[8] = {0};

// Track metadata
static const char *TRACK_NAMES[MAX_TRACKS] = {
    "Cyber Sant Joan (Synthwave 124)",
    "Guardia Nocturna CPD (Techno 136)",
    "Ritmo Cardiaco UCI (House 120)"
};

static const float BASE_BPM[MAX_TRACKS] = { 124.0f, 136.0f, 120.0f };

// Sound Effects State
typedef struct {
    bool active;
    SoundEffect type;
    int sample_pos;
    int total_samples;
    float param1;
    float param2;
} ActiveSFX;

#define MAX_ACTIVE_SFX 8
static ActiveSFX active_sfx[MAX_ACTIVE_SFX];

// Synth state
static uint64_t sample_counter = 0;
static float bass_phase = 0.0f;
static float lead_phase = 0.0f;
static float pad_phases[4] = {0.0f, 0.0f, 0.0f, 0.0f};
static float ecg_phase = 0.0f;

// Random noise generator for percussion
static uint32_t noise_seed = 0x12345678;
static inline float next_noise(void) {
    noise_seed = noise_seed * 1664525u + 1013904223u;
    return ((float)(noise_seed >> 16) / 32768.0f) - 1.0f;
}

// Lowpass filter state for bass
static float lp_out1 = 0.0f;
static float lp_out2 = 0.0f;

static inline float soft_clip(float x) {
    if (x > 1.2f) return 0.9f;
    if (x < -1.2f) return -0.9f;
    return x - (x * x * x) * 0.18f;
}

// Complete note frequencies (Hz)
#define NOTE_C2   65.41f
#define NOTE_D2   73.42f
#define NOTE_Eb2  77.78f
#define NOTE_E2   82.41f
#define NOTE_F2   87.31f
#define NOTE_G2   98.00f
#define NOTE_Ab2 103.83f
#define NOTE_A2  110.00f
#define NOTE_Bb2 116.54f
#define NOTE_B2  123.47f

#define NOTE_C3  130.81f
#define NOTE_Db3 138.59f
#define NOTE_D3  146.83f
#define NOTE_Eb3 155.56f
#define NOTE_E3  164.81f
#define NOTE_F3  174.61f
#define NOTE_G3  196.00f
#define NOTE_Ab3 207.65f
#define NOTE_A3  220.00f
#define NOTE_Bb3 233.08f
#define NOTE_B3  246.94f

#define NOTE_C4  261.63f
#define NOTE_Db4 277.18f
#define NOTE_D4  293.66f
#define NOTE_Eb4 311.13f
#define NOTE_E4  329.63f
#define NOTE_F4  349.23f
#define NOTE_G4  392.00f
#define NOTE_Ab4 415.30f
#define NOTE_A4  440.00f
#define NOTE_Bb4 466.16f
#define NOTE_B4  493.88f

#define NOTE_C5  523.25f
#define NOTE_Db5 554.37f
#define NOTE_D5  587.33f
#define NOTE_Eb5 622.25f
#define NOTE_E5  659.25f
#define NOTE_F5  698.46f
#define NOTE_G5  783.99f
#define NOTE_A5  880.00f

// Lead melodies across 64 16th steps
static const float TRACK0_LEAD[64] = {
    // Bar 1 (Dm)
    NOTE_D4, 0, NOTE_F4, 0, NOTE_A4, 0, NOTE_D5, NOTE_C5,
    NOTE_A4, 0, NOTE_F4, 0, NOTE_G4, 0, NOTE_F4, NOTE_E4,
    // Bar 2 (Bb)
    NOTE_D4, 0, NOTE_F4, 0, NOTE_Bb4, 0, NOTE_D5, 0,
    NOTE_C5, 0, NOTE_Bb4, 0, NOTE_A4, 0, NOTE_F4, 0,
    // Bar 3 (F)
    NOTE_C4, 0, NOTE_F4, 0, NOTE_A4, 0, NOTE_C5, 0,
    NOTE_D5, 0, NOTE_C5, 0, NOTE_A4, 0, NOTE_F4, 0,
    // Bar 4 (C)
    NOTE_G4, 0, NOTE_C5, 0, NOTE_E5, 0, NOTE_D5, NOTE_C5,
    NOTE_B4, 0, NOTE_G4, 0, NOTE_E4, 0, NOTE_F4, NOTE_E4
};

static const float TRACK1_LEAD[64] = {
    // Techno / Cyberpunk stabs (Fm - Db - Ab - Eb)
    NOTE_F4, NOTE_F4, 0, NOTE_Ab4, NOTE_C5, 0, NOTE_Ab4, 0,
    NOTE_F4, 0, NOTE_C5, 0, NOTE_Eb5, 0, NOTE_C5, 0,
    NOTE_Db4, NOTE_Db4, 0, NOTE_F4, NOTE_Ab4, 0, NOTE_F4, 0,
    NOTE_Db5, 0, NOTE_C5, 0, NOTE_Ab4, 0, NOTE_F4, 0,
    NOTE_Ab4, NOTE_Ab4, 0, NOTE_C5, NOTE_Eb5, 0, NOTE_C5, 0,
    NOTE_Eb5, 0, NOTE_C5, 0, NOTE_Ab4, 0, NOTE_Eb4, 0,
    NOTE_Eb4, NOTE_Eb4, 0, NOTE_G4, NOTE_Bb4, 0, NOTE_G4, 0,
    NOTE_Eb5, 0, NOTE_D5, 0, NOTE_Bb4, 0, NOTE_G4, 0
};

static const float TRACK2_LEAD[64] = {
    // Tech-House / ECG motif (Am - Em - F - G)
    NOTE_A4, 0, 0, NOTE_C5, 0, NOTE_E5, 0, NOTE_D5,
    0, NOTE_C5, 0, NOTE_A4, 0, 0, NOTE_G4, 0,
    NOTE_E4, 0, 0, NOTE_G4, 0, NOTE_B4, 0, NOTE_A4,
    0, NOTE_G4, 0, NOTE_E4, 0, 0, NOTE_F4, 0,
    NOTE_F4, 0, 0, NOTE_A4, 0, NOTE_C5, 0, NOTE_D5,
    0, NOTE_C5, 0, NOTE_A4, 0, NOTE_C5, 0, 0,
    NOTE_G4, 0, 0, NOTE_B4, 0, NOTE_D5, 0, NOTE_C5,
    0, NOTE_B4, 0, NOTE_G4, 0, NOTE_E4, 0, 0
};

// Procedural audio generation callback
static void audio_callback(void *userdata, Uint8 *stream, int len) {
    (void)userdata;
    int16_t *output = (int16_t *)stream;
    int num_samples = len / 4; // 16-bit stereo

    float bpm = BASE_BPM[current_track] * level_tempo_mult;
    float spb = 60.0f / bpm;
    float sp16 = spb / 4.0f;
    int samples_per_16th = (int)(sp16 * SAMPLE_RATE);
    if (samples_per_16th < 100) samples_per_16th = 100;

    float vis_energy[8] = {0};

    for (int s = 0; s < num_samples; s++) {
        float mix_l = 0.0f;
        float mix_r = 0.0f;

        if (music_enabled) {
            uint64_t cur_sample = sample_counter;
            int step_idx = (int)((cur_sample / samples_per_16th) % 64);
            int step_sample = (int)(cur_sample % samples_per_16th);
            float step_t = (float)step_sample / SAMPLE_RATE;

            int beat_sample = (int)(cur_sample % (samples_per_16th * 4));
            float beat_t = (float)beat_sample / SAMPLE_RATE;

            // 1. KICK DRUM (Electronic 808 pitch-drop punch)
            float kick = 0.0f;
            if (beat_t < 0.22f) {
                float k_env = expf(-beat_t * 22.0f);
                float k_freq = 160.0f * expf(-beat_t * 32.0f) + 40.0f;
                float k_click = (beat_t < 0.005f) ? (next_noise() * 0.4f) : 0.0f;
                kick = (sinf(TWO_PI * k_freq * beat_t) + k_click) * k_env * 0.85f;
            }

            // 2. SNARE / CLAP on beats 2 and 4 (steps 4, 12, 20, 28...)
            float snare = 0.0f;
            int beat_in_bar = step_idx % 16;
            if ((beat_in_bar >= 4 && beat_in_bar < 8) || (beat_in_bar >= 12 && beat_in_bar < 16)) {
                int s_step = (beat_in_bar < 8) ? (beat_in_bar - 4) : (beat_in_bar - 12);
                float s_t = (float)(s_step * samples_per_16th + step_sample) / SAMPLE_RATE;
                if (s_t < 0.20f) {
                    float s_env = expf(-s_t * 24.0f);
                    float s_noise = next_noise();
                    float s_body = sinf(TWO_PI * 190.0f * s_t) * expf(-s_t * 35.0f);
                    snare = (s_noise * 0.65f + s_body * 0.35f) * s_env * 0.60f;
                }
            }

            // 3. HI-HATS (Crisp 16th and open offbeats)
            float hihat = 0.0f;
            if (step_t < 0.08f) {
                bool is_open = (step_idx % 4 == 2); // open hat on offbeat
                float h_decay = is_open ? 18.0f : 75.0f;
                float h_env = expf(-step_t * h_decay);
                float h_vol = is_open ? 0.30f : ((step_idx % 2 == 1) ? 0.22f : 0.12f);
                hihat = next_noise() * h_env * h_vol;
            }

            // 4. SIDECHAIN ENVELOPE (Pump effect ducking on kick)
            float sidechain = 0.20f + 0.80f * fminf(1.0f, beat_t * 8.0f);

            // 5. BASSLINE (Rolling 16th synthwave / cyber bass)
            float bass_freq = NOTE_D2;
            int bar = step_idx / 16;
            if (current_track == 0) {
                // Dm -> Bb -> F -> C
                switch (bar) {
                    case 0: bass_freq = NOTE_D2; break;
                    case 1: bass_freq = NOTE_Bb2; break;
                    case 2: bass_freq = NOTE_F2; break;
                    case 3: bass_freq = NOTE_C2; break;
                }
            } else if (current_track == 1) {
                // Fm -> Db -> Ab -> Eb
                switch (bar) {
                    case 0: bass_freq = NOTE_F2; break;
                    case 1: bass_freq = NOTE_Db3; break;
                    case 2: bass_freq = NOTE_Ab2; break;
                    case 3: bass_freq = NOTE_Eb2; break;
                }
            } else {
                // Am -> Em -> F -> G
                switch (bar) {
                    case 0: bass_freq = NOTE_A2; break;
                    case 1: bass_freq = NOTE_E2; break;
                    case 2: bass_freq = NOTE_F2; break;
                    case 3: bass_freq = NOTE_G2; break;
                }
            }

            // Octave rolling groove on 16th notes
            if (step_idx % 4 == 2 || step_idx % 4 == 3) {
                bass_freq *= 2.0f;
            }

            bass_phase += bass_freq / SAMPLE_RATE;
            if (bass_phase > 1.0f) bass_phase -= 1.0f;
            float saw = (2.0f * bass_phase - 1.0f);
            float sub = (fmodf(bass_phase * 0.5f, 1.0f) < 0.5f) ? 0.7f : -0.7f;
            float bass_raw = (saw * 0.6f + sub * 0.4f);

            // Lowpass filter with filter envelope
            float filter_env = 0.08f + 0.18f * expf(-step_t * 14.0f);
            lp_out1 += filter_env * (bass_raw - lp_out1);
            lp_out2 += filter_env * (lp_out1 - lp_out2);
            float bass = lp_out2 * sidechain * 0.45f;

            // 6. POLYPHONIC CHORD PADS (Warm detuned analog pad)
            float pad_l = 0.0f;
            float pad_r = 0.0f;
            float chord_notes[3] = { NOTE_D3, NOTE_F3, NOTE_A3 };
            if (current_track == 0) {
                if (bar == 0) { chord_notes[0] = NOTE_D3; chord_notes[1] = NOTE_F3; chord_notes[2] = NOTE_A3; }
                else if (bar == 1) { chord_notes[0] = NOTE_Bb2; chord_notes[1] = NOTE_D3; chord_notes[2] = NOTE_F3; }
                else if (bar == 2) { chord_notes[0] = NOTE_F2; chord_notes[1] = NOTE_A3; chord_notes[2] = NOTE_C4; }
                else { chord_notes[0] = NOTE_C3; chord_notes[1] = NOTE_E3; chord_notes[2] = NOTE_G3; }
            } else if (current_track == 1) {
                if (bar == 0) { chord_notes[0] = NOTE_F3; chord_notes[1] = NOTE_Ab3; chord_notes[2] = NOTE_C4; }
                else if (bar == 1) { chord_notes[0] = NOTE_Db3; chord_notes[1] = NOTE_F3; chord_notes[2] = NOTE_Ab3; }
                else if (bar == 2) { chord_notes[0] = NOTE_Ab2; chord_notes[1] = NOTE_C3; chord_notes[2] = NOTE_Eb3; }
                else { chord_notes[0] = NOTE_Eb3; chord_notes[1] = NOTE_G3; chord_notes[2] = NOTE_Bb3; }
            } else {
                if (bar == 0) { chord_notes[0] = NOTE_A3; chord_notes[1] = NOTE_C4; chord_notes[2] = NOTE_E4; }
                else if (bar == 1) { chord_notes[0] = NOTE_E3; chord_notes[1] = NOTE_G3; chord_notes[2] = NOTE_B3; }
                else if (bar == 2) { chord_notes[0] = NOTE_F3; chord_notes[1] = NOTE_A3; chord_notes[2] = NOTE_C4; }
                else { chord_notes[0] = NOTE_G3; chord_notes[1] = NOTE_B3; chord_notes[2] = NOTE_D4; }
            }

            for (int ch = 0; ch < 3; ch++) {
                pad_phases[ch] += chord_notes[ch] / SAMPLE_RATE;
                if (pad_phases[ch] > 1.0f) pad_phases[ch] -= 1.0f;
                float p_saw = (2.0f * pad_phases[ch] - 1.0f);
                float p_pan = (ch == 0) ? 0.3f : ((ch == 1) ? 0.5f : 0.7f);
                pad_l += p_saw * (1.0f - p_pan);
                pad_r += p_saw * p_pan;
            }
            pad_l *= 0.08f * sidechain;
            pad_r *= 0.08f * sidechain;

            // 7. LEAD SYNTH MELODY
            float lead = 0.0f;
            float note_f = 0.0f;
            if (current_track == 0) note_f = TRACK0_LEAD[step_idx];
            else if (current_track == 1) note_f = TRACK1_LEAD[step_idx];
            else note_f = TRACK2_LEAD[step_idx];

            if (note_f > 20.0f) {
                float lead_env = expf(-step_t * 6.0f);
                lead_phase += note_f / SAMPLE_RATE;
                if (lead_phase > 1.0f) lead_phase -= 1.0f;
                float l_saw = (2.0f * lead_phase - 1.0f);
                float l_pulse = (lead_phase < 0.4f) ? 0.6f : -0.6f;
                lead = (l_saw * 0.6f + l_pulse * 0.4f) * lead_env * 0.22f;
            }

            // 8. HOSPITAL ECG TELEMETRY MOTIF (Pulse beep synchronized with music)
            float ecg_sound = 0.0f;
            // Trigger ECG pulse beep on step 0 and step 32
            if (step_idx == 0 || step_idx == 32) {
                if (step_t < 0.06f) {
                    ecg_phase += 1000.0f / SAMPLE_RATE;
                    if (ecg_phase > 1.0f) ecg_phase -= 1.0f;
                    float e_env = expf(-step_t * 45.0f);
                    ecg_sound = sinf(TWO_PI * ecg_phase) * e_env * 0.18f;
                }
            }

            // Combine music channels
            mix_l = kick + snare * 0.8f + hihat * 0.7f + bass + pad_l + lead * 0.7f + ecg_sound;
            mix_r = kick + snare * 0.8f + hihat * 0.9f + bass + pad_r + lead * 0.7f + ecg_sound;

            // Update visualizer energy
            vis_energy[0] += fabsf(kick);
            vis_energy[1] += fabsf(bass);
            vis_energy[2] += fabsf(snare);
            vis_energy[3] += fabsf(pad_l + pad_r);
            vis_energy[4] += fabsf(lead);
            vis_energy[5] += fabsf(hihat);
            vis_energy[6] += fabsf(ecg_sound);
            vis_energy[7] += fabsf(mix_l + mix_r) * 0.3f;

            sample_counter++;
        }

        // 9. SOUND EFFECTS (SFX)
        float sfx_l = 0.0f;
        float sfx_r = 0.0f;
        for (int i = 0; i < MAX_ACTIVE_SFX; i++) {
            if (!active_sfx[i].active) continue;
            ActiveSFX *asfx = &active_sfx[i];
            float t = (float)asfx->sample_pos / SAMPLE_RATE;
            float s_val = 0.0f;

            switch (asfx->type) {
                case SFX_MOVE: { // Clean UI blip
                    float env = expf(-t * 90.0f);
                    s_val = sinf(TWO_PI * 850.0f * t) * env * 0.35f;
                    break;
                }
                case SFX_ROTATE: { // Tech click
                    float freq = 1400.0f - t * 14000.0f;
                    if (freq < 400.0f) freq = 400.0f;
                    float env = expf(-t * 110.0f);
                    s_val = sinf(TWO_PI * freq * t) * env * 0.40f;
                    break;
                }
                case SFX_SOFT_DROP: { // Soft tick
                    float env = expf(-t * 120.0f);
                    s_val = sinf(TWO_PI * 130.0f * t) * env * 0.30f;
                    break;
                }
                case SFX_HARD_DROP: { // Deep impact thump
                    float env = expf(-t * 30.0f);
                    float freq = 90.0f * expf(-t * 25.0f) + 35.0f;
                    float click = (t < 0.006f) ? next_noise() * 0.5f : 0.0f;
                    s_val = (sinf(TWO_PI * freq * t) + click) * env * 0.80f;
                    break;
                }
                case SFX_HOLD: { // Cyber whoosh
                    float freq = 350.0f + t * 5000.0f;
                    float env = expf(-t * 20.0f);
                    s_val = sinf(TWO_PI * freq * t) * env * 0.35f;
                    break;
                }
                case SFX_LINE_CLEAR: { // Hospital laser sweep / chime
                    float freq = 600.0f + t * 3500.0f;
                    float env = expf(-t * 10.0f);
                    s_val = (sinf(TWO_PI * freq * t) + 0.5f * sinf(TWO_PI * (freq * 1.5f) * t)) * env * 0.45f;
                    break;
                }
                case SFX_TETRIS: { // Epic chord fanfare
                    float env = expf(-t * 4.0f);
                    float f1 = sinf(TWO_PI * 523.25f * t); // C5
                    float f2 = sinf(TWO_PI * 659.25f * t); // E5
                    float f3 = sinf(TWO_PI * 783.99f * t); // G5
                    float f4 = sinf(TWO_PI * 1046.50f * t); // C6
                    s_val = (f1 + f2 + f3 + f4) * 0.25f * env * 0.70f;
                    break;
                }
                case SFX_LEVELUP: { // Hospital announcement priority chime
                    float env = expf(-t * 5.0f);
                    float freq = 880.0f;
                    if (t > 0.15f) freq = 1174.66f;
                    if (t > 0.30f) freq = 1760.00f;
                    s_val = sinf(TWO_PI * freq * t) * env * 0.50f;
                    break;
                }
                case SFX_GAMEOVER: { // Flatline continuous tone then buzzer
                    if (t < 0.60f) {
                        s_val = sinf(TWO_PI * 880.0f * t) * 0.55f; // Flatline ECG
                    } else {
                        float t_buzz = t - 0.60f;
                        float buzz = (fmodf(t_buzz * 160.0f, 1.0f) < 0.5f) ? 0.7f : -0.7f;
                        s_val = buzz * expf(-t_buzz * 4.0f) * 0.60f;
                    }
                    break;
                }
                default: break;
            }

            sfx_l += s_val;
            sfx_r += s_val;

            asfx->sample_pos++;
            if (asfx->sample_pos >= asfx->total_samples) {
                asfx->active = false;
            }
        }

        float total_l = (mix_l + sfx_l) * master_volume;
        float total_r = (mix_r + sfx_r) * master_volume;

        total_l = soft_clip(total_l);
        total_r = soft_clip(total_r);

        output[s * 2]     = (int16_t)(total_l * 32767.0f);
        output[s * 2 + 1] = (int16_t)(total_r * 32767.0f);
    }

    // Smooth visualizer levels
    for (int b = 0; b < 8; b++) {
        float avg = (vis_energy[b] / (float)num_samples) * 3.5f;
        if (avg > 1.0f) avg = 1.0f;
        vis_targets[b] = avg;
        // Attack/decay filter
        if (vis_targets[b] > vis_levels[b]) {
            vis_levels[b] = vis_targets[b];
        } else {
            vis_levels[b] -= 0.08f;
            if (vis_levels[b] < 0.0f) vis_levels[b] = 0.0f;
        }
    }
}

bool audio_init(void) {
    if (SDL_InitSubSystem(SDL_INIT_AUDIO) != 0) {
        return false;
    }

    SDL_AudioSpec want, have;
    SDL_zero(want);
    want.freq = SAMPLE_RATE;
    want.format = AUDIO_S16SYS;
    want.channels = 2;
    want.samples = 1024;
    want.callback = audio_callback;
    want.userdata = NULL;

    audio_device = SDL_OpenAudioDevice(NULL, 0, &want, &have, 0);
    if (!audio_device) {
        return false;
    }

    memset(active_sfx, 0, sizeof(active_sfx));
    SDL_PauseAudioDevice(audio_device, 0);
    return true;
}

void audio_cleanup(void) {
    if (audio_device) {
        SDL_CloseAudioDevice(audio_device);
        audio_device = 0;
    }
    SDL_QuitSubSystem(SDL_INIT_AUDIO);
}

void audio_play_sfx(SoundEffect sfx) {
    if (!audio_device) return;

    int total = 0;
    switch (sfx) {
        case SFX_MOVE:      total = (int)(SAMPLE_RATE * 0.04f); break;
        case SFX_ROTATE:    total = (int)(SAMPLE_RATE * 0.05f); break;
        case SFX_SOFT_DROP: total = (int)(SAMPLE_RATE * 0.04f); break;
        case SFX_HARD_DROP: total = (int)(SAMPLE_RATE * 0.12f); break;
        case SFX_HOLD:      total = (int)(SAMPLE_RATE * 0.10f); break;
        case SFX_LINE_CLEAR:total = (int)(SAMPLE_RATE * 0.35f); break;
        case SFX_TETRIS:    total = (int)(SAMPLE_RATE * 0.70f); break;
        case SFX_LEVELUP:   total = (int)(SAMPLE_RATE * 0.60f); break;
        case SFX_GAMEOVER:  total = (int)(SAMPLE_RATE * 1.50f); break;
        default: return;
    }

    SDL_LockAudioDevice(audio_device);
    int slot = -1;
    for (int i = 0; i < MAX_ACTIVE_SFX; i++) {
        if (!active_sfx[i].active) {
            slot = i;
            break;
        }
    }
    if (slot == -1) slot = 0;

    active_sfx[slot].active = true;
    active_sfx[slot].type = sfx;
    active_sfx[slot].sample_pos = 0;
    active_sfx[slot].total_samples = total;
    SDL_UnlockAudioDevice(audio_device);
}

void audio_toggle_music(void) {
    music_enabled = !music_enabled;
}

bool audio_is_music_enabled(void) {
    return music_enabled;
}

void audio_next_track(void) {
    current_track = (current_track + 1) % MAX_TRACKS;
}

int audio_get_track_index(void) {
    return current_track;
}

const char *audio_get_track_name(void) {
    return TRACK_NAMES[current_track];
}

void audio_volume_up(void) {
    master_volume += 0.05f;
    if (master_volume > 1.0f) master_volume = 1.0f;
}

void audio_volume_down(void) {
    master_volume -= 0.05f;
    if (master_volume < 0.0f) master_volume = 0.0f;
}

float audio_get_volume(void) {
    return master_volume;
}

void audio_set_level_tempo(int level) {
    float factor = 1.0f + (float)(level - 1) * 0.032f;
    if (factor > 1.35f) factor = 1.35f;
    level_tempo_mult = factor;
}

void audio_get_visualizer(float bars[8]) {
    for (int i = 0; i < 8; i++) {
        bars[i] = vis_levels[i];
    }
}
