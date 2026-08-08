#ifndef RETRO2D_INTERNAL_H
#define RETRO2D_INTERNAL_H

#include "retro2d.h"

#include <stdio.h>

#define R2D_MAX_CLIPS 16
#define R2D_MAX_EFFECTS 16
#define R2D_MAX_VOICES 64
#define R2D_AUDIO_RATE 44100

typedef struct EffectSlot {
    EffectHandle handle;
    PostEffect callback;
    void *user_data;
    bool enabled;
} EffectSlot;

typedef struct InputButtonState {
    bool down;
    bool pressed;
    bool released;
    bool pending_pressed;
    bool pending_released;
} InputButtonState;

struct Image {
    int width, height;
    uint8_t *pixels;
    uint8_t *mask;
    int mask_stride;
    int colour_key;
    struct Image *next_resource;
};

struct SpriteSheet {
    Image *image;
    int frame_width, frame_height;
    int columns, rows;
    struct SpriteSheet *next_resource;
};

struct Animation {
    SpriteSheet *sheet;
    int *frames;
    int frame_count;
    int current;
    float rate;
    float accumulator;
    bool loop, playing, finished;
    struct Animation *next_resource;
};

struct BitmapFont {
    Image *image;
    bool owns_image;
    int glyph_width, glyph_height;
    int first_character, glyph_count, columns;
    int character_spacing, line_spacing;
    struct BitmapFont *next_resource;
};

struct Sound {
    int16_t *samples;
    size_t frame_count;
    struct Sound *next_resource;
};

struct Music {
    int16_t *samples;
    size_t frame_count;
    struct Music *next_resource;
};

typedef struct AudioVoice {
    Sound *sound;
    size_t position;
    float volume, pan;
    Playback handle;
    bool active;
} AudioVoice;

typedef struct MusicVoice {
    Music *music;
    size_t position;
    float fade, fade_step;
    bool loop, active, paused, stop_after_fade;
} MusicVoice;

typedef struct R2D_State {
    Framebuffer screen;
    uint8_t *effect_a;
    uint8_t *effect_b;
    Color palette[RETRO2D_PALETTE_SIZE];
    Rect clips[R2D_MAX_CLIPS];
    int clip_count;
    Point camera;

    InputButtonState keys[KEY_COUNT];
    InputButtonState mouse_buttons[MOUSE_BUTTON_COUNT];
    InputButtonState controller_buttons[CONTROLLER_BUTTON_COUNT];
    float controller_axes[CONTROLLER_AXIS_COUNT];
    Point mouse;
    Point mouse_delta;
    Point pending_mouse_delta;
    int mouse_wheel;
    int pending_mouse_wheel;
    bool mouse_inside;
    bool controller_present;

    EffectSlot effects[R2D_MAX_EFFECTS];
    int effect_count;
    EffectHandle next_effect;

    Image *images;
    SpriteSheet *sheets;
    Animation *animations;
    BitmapFont *fonts;
    Sound *sounds;
    Music *music_resources;
    uint8_t alpha_threshold;

    AudioVoice voices[R2D_MAX_VOICES];
    MusicVoice music_voice;
    int voice_limit;
    Playback next_playback;
    float sound_volume, music_volume;
    bool audio_ready;

    bool running;
    double elapsed;
    uint64_t frames, updates;
    float fps, frame_seconds, update_seconds;
} R2D_State;

extern R2D_State r2d;

void r2d_set_error(const char *format, ...);
bool r2d_screen_init(int width, int height);
void r2d_screen_shutdown(void);
Rect r2d_current_clip(void);
const uint8_t *r2d_apply_effects(float elapsed);
void r2d_resources_shutdown(void);

void r2d_input_reset(void);
void r2d_input_begin_update(void);
void r2d_input_key(Key key, bool down);
void r2d_input_mouse_button(MouseButton button, bool down);
void r2d_input_mouse_motion(float x, float y, float dx, float dy);
void r2d_input_mouse_wheel(float amount);
bool r2d_map_window_point(float x, float y, int window_width, int window_height,
                          Point *result);
void r2d_input_controller_button(ControllerButton button, bool down);
void r2d_input_controller_axis(ControllerAxis axis, float value);
void r2d_input_controller_connected(bool connected);

bool r2d_audio_init(int channels);
void r2d_audio_shutdown(void);
void r2d_audio_mix(int16_t *output, int frames);

bool r2d_platform_init(const GameConfig *config);
void r2d_platform_shutdown(void);
void r2d_platform_poll_events(void);
void r2d_platform_present(const uint8_t *pixels, const Color *palette);
uint64_t r2d_platform_ticks_ns(void);
void r2d_platform_delay_ns(uint64_t nanoseconds);
void r2d_platform_show_cursor(bool visible);
void r2d_platform_confine_cursor(bool confined);
void r2d_platform_audio_lock(void);
void r2d_platform_audio_unlock(void);

uint16_t r2d_read_u16(FILE *file, bool *ok);
uint32_t r2d_read_u32(FILE *file, bool *ok);
int32_t r2d_read_i32(FILE *file, bool *ok);
float r2d_clamp01(float value);

#endif
