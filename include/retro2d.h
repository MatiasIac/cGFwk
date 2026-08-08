#ifndef RETRO2D_H
#define RETRO2D_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifndef NDEBUG
#include <assert.h>
#define RETRO2D_ASSERT(condition) assert(condition)
#else
#define RETRO2D_ASSERT(condition) ((void)0)
#endif

#ifdef __cplusplus
extern "C" {
#endif

#define RETRO2D_PALETTE_SIZE 256
#define RETRO2D_NO_PLAYBACK 0u

typedef struct Color {
    uint8_t r, g, b, a;
} Color;

typedef struct Point {
    int x, y;
} Point;

typedef struct Rect {
    int x, y, width, height;
} Rect;

typedef struct Circle {
    int x, y, radius;
} Circle;

typedef struct Framebuffer {
    uint8_t *pixels;
    int width, height, stride;
} Framebuffer;

typedef struct GameConfig {
    const char *title;
    int screen_width;
    int screen_height;
    int window_scale;
    int update_rate;
    bool vertical_sync;
    bool fullscreen;
    int audio_channels;
} GameConfig;

typedef struct Image Image;
typedef struct SpriteSheet SpriteSheet;
typedef struct Animation Animation;
typedef struct BitmapFont BitmapFont;
typedef struct Sound Sound;
typedef struct Music Music;
typedef uint32_t Playback;
typedef int EffectHandle;

typedef enum Flip {
    FLIP_NONE = 0,
    FLIP_HORIZONTAL = 1,
    FLIP_VERTICAL = 2
} Flip;

typedef enum TextAlign {
    ALIGN_LEFT,
    ALIGN_CENTRE,
    ALIGN_RIGHT
} TextAlign;

typedef enum LogLevel {
    LOG_INFO,
    LOG_WARNING,
    LOG_ERROR
} LogLevel;

typedef enum Key {
    KEY_UNKNOWN = 0,
    KEY_A, KEY_B, KEY_C, KEY_D, KEY_E, KEY_F, KEY_G, KEY_H, KEY_I,
    KEY_J, KEY_K, KEY_L, KEY_M, KEY_N, KEY_O, KEY_P, KEY_Q, KEY_R,
    KEY_S, KEY_T, KEY_U, KEY_V, KEY_W, KEY_X, KEY_Y, KEY_Z,
    KEY_0, KEY_1, KEY_2, KEY_3, KEY_4,
    KEY_5, KEY_6, KEY_7, KEY_8, KEY_9,
    KEY_ESCAPE, KEY_ENTER, KEY_SPACE, KEY_TAB, KEY_BACKSPACE,
    KEY_LEFT, KEY_RIGHT, KEY_UP, KEY_DOWN,
    KEY_LSHIFT, KEY_RSHIFT, KEY_LCTRL, KEY_RCTRL, KEY_LALT, KEY_RALT,
    KEY_F1, KEY_F2, KEY_F3, KEY_F4, KEY_F5, KEY_F6,
    KEY_F7, KEY_F8, KEY_F9, KEY_F10, KEY_F11, KEY_F12,
    KEY_HOME, KEY_END, KEY_PAGE_UP, KEY_PAGE_DOWN, KEY_INSERT, KEY_DELETE,
    KEY_MINUS, KEY_EQUALS, KEY_LEFT_BRACKET, KEY_RIGHT_BRACKET,
    KEY_SEMICOLON, KEY_APOSTROPHE, KEY_COMMA, KEY_PERIOD, KEY_SLASH,
    KEY_BACKSLASH, KEY_GRAVE,
    KEY_COUNT
} Key;

typedef enum MouseButton {
    MOUSE_LEFT = 0,
    MOUSE_MIDDLE,
    MOUSE_RIGHT,
    MOUSE_X1,
    MOUSE_X2,
    MOUSE_BUTTON_COUNT
} MouseButton;

typedef enum ControllerButton {
    CONTROLLER_A = 0, CONTROLLER_B, CONTROLLER_X, CONTROLLER_Y,
    CONTROLLER_BACK, CONTROLLER_GUIDE, CONTROLLER_START,
    CONTROLLER_LEFT_STICK, CONTROLLER_RIGHT_STICK,
    CONTROLLER_LEFT_SHOULDER, CONTROLLER_RIGHT_SHOULDER,
    CONTROLLER_DPAD_UP, CONTROLLER_DPAD_DOWN,
    CONTROLLER_DPAD_LEFT, CONTROLLER_DPAD_RIGHT,
    CONTROLLER_BUTTON_COUNT
} ControllerButton;

typedef enum ControllerAxis {
    CONTROLLER_LEFT_X = 0, CONTROLLER_LEFT_Y,
    CONTROLLER_RIGHT_X, CONTROLLER_RIGHT_Y,
    CONTROLLER_LEFT_TRIGGER, CONTROLLER_RIGHT_TRIGGER,
    CONTROLLER_AXIS_COUNT
} ControllerAxis;

typedef void (*LogCallback)(LogLevel level, const char *message, void *user_data);
typedef void (*PostEffect)(const uint8_t *source, uint8_t *destination,
                           int width, int height, int source_stride,
                           int destination_stride, const Color *palette,
                           float elapsed, void *user_data);

/* The game supplies these callbacks. */
void game_start(void);
void game_update(float delta);
void game_draw(void);
void game_stop(void);

/* Lifecycle, timing, and diagnostics. */
int run_game(const GameConfig *config);
void request_quit(void);
double elapsed_time(void);
uint64_t frame_count(void);
uint64_t update_count(void);
float frames_per_second(void);
float frame_time(void);
float update_time(void);
const char *last_error(void);
void set_log_callback(LogCallback callback, void *user_data);
void log_message(LogLevel level, const char *format, ...);

/* Indexed screen and palette. Framebuffer access is valid in game callbacks. */
Framebuffer get_framebuffer(void);
void clear_screen(uint8_t colour);
void set_palette_colour(int index, Color colour);
Color get_palette_colour(int index);
void set_palette(const Color colours[RETRO2D_PALETTE_SIZE]);
void get_palette(Color colours[RETRO2D_PALETTE_SIZE]);
bool load_palette(const char *path);
bool save_palette(const char *path);
void rotate_palette(int first, int last, int amount);
void make_faded_palette(const Color source[RETRO2D_PALETTE_SIZE],
                         Color target, float amount,
                         Color result[RETRO2D_PALETTE_SIZE]);
void fade_palette_to(Color target, float amount);

/* Drawing, camera, and clipping. */
void put_pixel(int x, int y, uint8_t colour);
uint8_t get_pixel(int x, int y);
void draw_horizontal_line(int x, int y, int length, uint8_t colour);
void draw_vertical_line(int x, int y, int length, uint8_t colour);
void draw_line(int x0, int y0, int x1, int y1, uint8_t colour);
void draw_rectangle(Rect rectangle, uint8_t colour);
void fill_rectangle(Rect rectangle, uint8_t colour);
void draw_circle(Circle circle, uint8_t colour);
void fill_circle(Circle circle, uint8_t colour);
void set_camera(int x, int y);
Point get_camera(void);
bool push_clip(Rect rectangle);
void pop_clip(void);
void reset_clip(void);

/* Images and sprite regions. */
Image *load_image(const char *path);
Image *create_image(int width, int height, const uint8_t *pixels);
void set_image_alpha_threshold(uint8_t threshold);
void free_image(Image *image);
int image_width(const Image *image);
int image_height(const Image *image);
void image_set_colour_key(Image *image, int palette_index);
void image_clear_colour_key(Image *image);
bool image_set_mask(Image *image, const uint8_t *bits, int bit_stride);
void image_clear_mask(Image *image);
void draw_image(const Image *image, int x, int y);
void draw_image_part(const Image *image, Rect source, int x, int y, Flip flip);

/* Shared sprite sheet and independent animation state. */
SpriteSheet *create_sprite_sheet(Image *image, int frame_width, int frame_height);
void free_sprite_sheet(SpriteSheet *sheet);
int sprite_sheet_frame_count(const SpriteSheet *sheet);
Rect sprite_frame_source(const SpriteSheet *sheet, int frame);
void draw_sprite_frame(const SpriteSheet *sheet, int frame, int x, int y, Flip flip);
Animation *create_animation(SpriteSheet *sheet, int first_frame, int frame_count,
                            float frames_per_second, bool loop);
Animation *create_animation_frames(SpriteSheet *sheet, const int *frames,
                                   int frame_count, float frames_per_second,
                                   bool loop);
void free_animation(Animation *animation);
void animation_update(Animation *animation, float delta);
void animation_play(Animation *animation);
void animation_pause(Animation *animation);
void animation_reset(Animation *animation);
void animation_set_loop(Animation *animation, bool loop);
void animation_set_speed(Animation *animation, float frames_per_second);
bool animation_finished(const Animation *animation);
int animation_current_frame(const Animation *animation);
void draw_animation(const Animation *animation, int x, int y, Flip flip);

/* Bitmap fonts. The image remains owned by the caller. */
BitmapFont *create_bitmap_font(Image *image, int glyph_width, int glyph_height,
                               int first_character, int glyph_count,
                               int columns);
BitmapFont *load_bitmap_font(const char *path, int glyph_width, int glyph_height,
                             int first_character, int glyph_count, int columns);
void free_bitmap_font(BitmapFont *font);
void font_set_colour_key(BitmapFont *font, int palette_index);
void font_set_spacing(BitmapFont *font, int character_spacing, int line_spacing);
Point measure_text(const BitmapFont *font, const char *text);
void draw_text(const BitmapFont *font, const char *text, int x, int y,
               TextAlign alignment);

/* Input states are updated once per fixed game update. */
bool key_down(Key key);
bool key_pressed(Key key);
bool key_released(Key key);
Point mouse_position(void);
Point mouse_movement(void);
bool mouse_inside_screen(void);
bool mouse_down(MouseButton button);
bool mouse_pressed(MouseButton button);
bool mouse_released(MouseButton button);
int mouse_wheel(void);
void show_cursor(bool visible);
void confine_cursor(bool confined);
bool controller_connected(void);
bool controller_down(ControllerButton button);
bool controller_pressed(ControllerButton button);
bool controller_released(ControllerButton button);
float controller_axis(ControllerAxis axis);

/* PCM WAVE sound effects and music. */
Sound *load_sound(const char *path);
void free_sound(Sound *sound);
Playback play_sound(Sound *sound);
void stop_playback(Playback playback);
void stop_sound(Sound *sound);
void stop_all_sounds(void);
void set_playback_volume(Playback playback, float volume);
void set_playback_pan(Playback playback, float pan);
void set_sound_volume(float volume);
Music *load_music(const char *path);
void free_music(Music *music);
void play_music(Music *music, bool loop);
void pause_music(void);
void resume_music(void);
void stop_music(void);
void fade_music_in(Music *music, bool loop, float seconds);
void fade_music_out(float seconds);
void set_music_volume(float volume);
bool music_playing(void);

/* Ordered software post-processing. */
EffectHandle add_effect(PostEffect effect, void *user_data);
void remove_effect(EffectHandle handle);
void enable_effect(EffectHandle handle, bool enabled);
bool move_effect(EffectHandle handle, int new_index);
void clear_effects(void);

/* Collision helpers do not move game objects. */
bool point_in_rectangle(Point point, Rect rectangle);
bool rectangles_intersect(Rect a, Rect b);
bool circles_intersect(Circle a, Circle b);
bool image_masks_intersect(const Image *a, Rect source_a, Point position_a,
                           const Image *b, Rect source_b, Point position_b);

#ifdef __cplusplus
}
#endif

#endif
