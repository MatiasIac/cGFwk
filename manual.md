# Retro2D Framework Manual

This manual describes every public type, feature, callback, and function in
`include/retro2d.h`. It complements the shorter setup guide in `README.md`.

All examples assume:

```c
#include "retro2d.h"
```

Unless stated otherwise, call framework functions from `game_start`,
`game_update`, `game_draw`, or `game_stop`. Retro2D owns the window, indexed
framebuffer, SDL backend, fixed-step loop, and audio device.

## 1. Core concepts

### Virtual screen

The game draws into a configurable virtual screen that defaults to 320 x 200.
Each pixel is one byte and contains a palette index from 0 to 255. The palette
turns that index into an RGBA colour when the frame is presented.

```c
void game_draw(void)
{
    clear_screen(0);       /* Fill every pixel with palette index 0. */
    put_pixel(10, 10, 4);  /* Pixel (10, 10) now uses palette index 4. */
}
```

Changing a palette colour immediately changes every displayed pixel using that
index; objects do not need to be redrawn into the framebuffer.

### Coordinates and clipping

The origin is the top-left. X increases rightward and Y increases downward.
Every normal drawing operation is safely clipped to the screen and the current
clip rectangle. The camera is subtracted from drawing coordinates.

The virtual resolution is configurable; 320 x 200 is only the default. Values
such as 640 x 480 and 800 x 600 use the same indexed drawing API. In windowed or
borderless fullscreen mode, Retro2D uses nearest-neighbour sampling and the
largest aspect-preserving scale. Exact multiples use integer scaling; other
display sizes use a fractional scale so the rendered game occupies as much of
the available area as possible. Any remaining area becomes a black letterbox or
pillarbox border.

### Fixed updates and independent drawing

`game_update` runs at the configured fixed rate, normally 60 times per second.
`game_draw` may run at a different rate. Put game rules and movement in
`game_update`; use `game_draw` only to produce the current picture.

### Resource ownership

Images, sprite sheets, animations, fonts, sounds, and music are opaque handles.
Free them explicitly when practical. Framework shutdown also releases resources
that remain registered.

The usual dependency order is:

```text
Image -> SpriteSheet -> Animation
Image -> BitmapFont created from an existing image
```

Free dependants first:

```c
free_animation(walk);
free_sprite_sheet(characters);
free_image(character_image);
```

## 2. Constants, types, and enumerations

### Constants

| Name | Meaning | Example |
|---|---|---|
| `RETRO2D_PALETTE_SIZE` | Number of palette entries; always 256. | `Color saved[RETRO2D_PALETTE_SIZE];` |
| `RETRO2D_NO_PLAYBACK` | Invalid sound playback handle; always zero. | `if (shot == RETRO2D_NO_PLAYBACK) { /* audio unavailable */ }` |
| `RETRO2D_ASSERT(condition)` | Debug assertion. It becomes a no-op when `NDEBUG` is defined. | `RETRO2D_ASSERT(player != NULL);` |

### Value types

| Type | Fields and purpose | Example |
|---|---|---|
| `Color` | Four 8-bit fields: `r`, `g`, `b`, and `a`. | `Color orange = {255, 128, 0, 255};` |
| `Point` | Integer `x` and `y`. Used for positions and measured sizes. | `Point spawn = {160, 100};` |
| `Rect` | Integer `x`, `y`, `width`, and `height`. Right and bottom edges are exclusive for collision. | `Rect button = {20, 30, 80, 16};` |
| `Circle` | Integer centre `x`, `y`, and `radius`. | `Circle shield = {160, 100, 12};` |
| `Framebuffer` | Framework-owned `pixels`, `width`, `height`, and row `stride`. | `Framebuffer fb = get_framebuffer();` |
| `GameConfig` | Startup title, virtual size, window scale, update rate, VSync, fullscreen mode, and audio channel count. | `GameConfig c = {"Game", 320, 200, 3, 60, true, false, 32};` |
| `Playback` | Lightweight handle for one playing sound instance. | `Playback p = play_sound(laser);` |
| `EffectHandle` | Handle returned for one registered post effect. | `EffectHandle water = add_effect(water_effect, NULL);` |

`Image`, `SpriteSheet`, `Animation`, `BitmapFont`, `Sound`, and `Music` are
opaque types. Game code stores pointers to them but cannot access their fields.

```c
static Image *hero;
static Sound *jump_sound;
```

### Drawing and text enumerations

`Flip` controls sprite-region drawing. Horizontal and vertical flags may be
combined with a cast.

```c
draw_image_part(hero, source, 20, 30, FLIP_NONE);
draw_image_part(hero, source, 40, 30, FLIP_HORIZONTAL);
draw_image_part(hero, source, 60, 30, FLIP_VERTICAL);
draw_image_part(hero, source, 80, 30,
                (Flip)(FLIP_HORIZONTAL | FLIP_VERTICAL));
```

`TextAlign` contains `ALIGN_LEFT`, `ALIGN_CENTRE`, and `ALIGN_RIGHT`.

```c
draw_text(font, "LEFT", 4, 4, ALIGN_LEFT);
draw_text(font, "CENTRE", 160, 4, ALIGN_CENTRE);
draw_text(font, "RIGHT", 316, 4, ALIGN_RIGHT);
```

`LogLevel` contains `LOG_INFO`, `LOG_WARNING`, and `LOG_ERROR`.

```c
log_message(LOG_WARNING, "Player has only %d energy", energy);
```

### Input enumerations

`Key` provides letters `KEY_A` through `KEY_Z`, digits `KEY_0` through
`KEY_9`, arrows, modifiers, function keys, navigation keys, and common
punctuation. `KEY_UNKNOWN` is not a usable key and `KEY_COUNT` is the internal
array limit.

```c
if (key_down(KEY_LEFT) || key_down(KEY_A)) player_x -= 2;
if (key_pressed(KEY_F1)) show_help = !show_help;
```

`MouseButton` provides `MOUSE_LEFT`, `MOUSE_MIDDLE`, `MOUSE_RIGHT`, `MOUSE_X1`,
and `MOUSE_X2`.

```c
if (mouse_pressed(MOUSE_RIGHT)) open_context_menu();
```

`ControllerButton` provides the four face buttons, Back, Guide, Start, stick
buttons, shoulders, and directional pad. `ControllerAxis` provides both stick
axes and both triggers.

```c
float horizontal = controller_axis(CONTROLLER_LEFT_X);
if (controller_pressed(CONTROLLER_A)) jump();
```

## 3. Game lifecycle

The game supplies four callbacks. A minimal complete lifecycle is:

```c
static int player_x;

void game_start(void)
{
    player_x = 152; /* Load resources and create initial state here. */
}

void game_update(float delta)
{
    if (key_down(KEY_RIGHT)) player_x += (int)(100.0f * delta);
    if (key_pressed(KEY_ESCAPE)) request_quit();
}

void game_draw(void)
{
    clear_screen(0);
    fill_rectangle((Rect){player_x, 92, 16, 16}, 1);
}

void game_stop(void)
{
    /* Free resources and save state here. */
}

int main(void)
{
    GameConfig config = {
        "Lifecycle Example", 320, 200, 3, 60, true, false, 32
    };
    return run_game(&config);
}
```

### Lifecycle functions

| Function | Purpose | Short example |
|---|---|---|
| `game_start()` | Supplied by the game. Called once after the screen, platform, input, and audio are ready. | `void game_start(void) { hero = load_image("assets/hero.bmp"); }` |
| `game_update(delta)` | Supplied by the game. Called at the fixed update rate; `delta` is seconds. | `void game_update(float delta) { x += speed * delta; }` |
| `game_draw()` | Supplied by the game. Called for each rendered frame. | `void game_draw(void) { clear_screen(0); draw_image(hero, x, y); }` |
| `game_stop()` | Supplied by the game. Called once during clean shutdown. | `void game_stop(void) { free_image(hero); }` |
| `run_game(config)` | Starts the framework and returns zero after a clean exit. Positive config fields set screen/timing values. | `return run_game(&config);` |
| `request_quit()` | Requests a clean exit after the current callback. | `if (key_pressed(KEY_ESCAPE)) request_quit();` |

Fully initialize `GameConfig`, especially its Boolean fields. A standard
windowed configuration is:

```c
GameConfig config = {
    .title = "My Game",
    .screen_width = 320,
    .screen_height = 200,
    .window_scale = 3,
    .update_rate = 60,
    .vertical_sync = true,
    .fullscreen = false,
    .audio_channels = 32
};
```

## 4. Timing, errors, and logging

### Timing queries

| Function | Result | Short example |
|---|---|---|
| `elapsed_time()` | Seconds since the game loop started. | `double seconds = elapsed_time();` |
| `frame_count()` | Number of frames presented. | `uint64_t frames = frame_count();` |
| `update_count()` | Number of fixed updates completed. | `uint64_t ticks = update_count();` |
| `frames_per_second()` | Recently measured presentation rate. It is initially zero until a sample is available. | `float fps = frames_per_second();` |
| `frame_time()` | Seconds spent on the most recently measured frame. | `float draw_ms = frame_time() * 1000.0f;` |
| `update_time()` | Seconds spent in the latest `game_update` call. | `float update_ms = update_time() * 1000.0f;` |

Timing queries are useful for diagnostics, not for game movement. Use the fixed
`delta` supplied to `game_update` for gameplay.

### Errors and logs

Loading functions return `NULL` or `false` on failure and set a readable error.

```c
Image *map = load_image("assets/map.bmp");
if (!map) {
    log_message(LOG_ERROR, "Map failed: %s", last_error());
    request_quit();
}
```

| Function | Purpose | Short example |
|---|---|---|
| `last_error()` | Returns the framework's most recent readable error string. The framework owns it. | `fprintf(stderr, "%s\n", last_error());` |
| `log_message(level, format, ...)` | Formats an informational, warning, or error message. | `log_message(LOG_INFO, "Level %d loaded", level);` |
| `set_log_callback(callback, user_data)` | Replaces default stdout/stderr logging. Pass `NULL` to restore default logging. | `set_log_callback(my_logger, log_file);` |

A custom logger receives the configured user pointer:

```c
static void my_logger(LogLevel level, const char *message, void *user_data)
{
    FILE *file = (FILE *)user_data;
    fprintf(file, "[%d] %s\n", (int)level, message);
}

/* After opening log_file: */
set_log_callback(my_logger, log_file);
```

Do not call `log_message` from inside the custom callback; doing so would invoke
the callback recursively.

## 5. Framebuffer and palette

### Direct framebuffer access

`get_framebuffer()` returns a copy of the current framebuffer description. The
pixel memory remains owned by Retro2D and is valid for direct use only during
game callbacks.

```c
void game_draw(void)
{
    Framebuffer fb = get_framebuffer();
    for (int y = 0; y < fb.height; ++y) {
        for (int x = 0; x < fb.width; ++x) {
            fb.pixels[y * fb.stride + x] = (uint8_t)((x + y) & 255);
        }
    }
}
```

`clear_screen(colour)` fills the entire framebuffer with one palette index and
ignores the camera and clip stack.

```c
clear_screen(0);
```

### Individual and whole-palette access

| Function | Purpose | Short example |
|---|---|---|
| `set_palette_colour(index, colour)` | Changes one entry from 0 through 255. Invalid indices set an error. | `set_palette_colour(4, (Color){255, 0, 0, 255});` |
| `get_palette_colour(index)` | Reads one active palette entry. | `Color old = get_palette_colour(4);` |
| `set_palette(colours)` | Replaces all 256 active entries from a caller array. | `set_palette(level_palette);` |
| `get_palette(colours)` | Copies all 256 active entries to a caller array. | `get_palette(saved_palette);` |

```c
static Color original[RETRO2D_PALETTE_SIZE];

void game_start(void)
{
    get_palette(original);
    set_palette_colour(10, (Color){255, 220, 40, 255});
}
```

### Palette files

`load_palette(path)` accepts a raw 768-byte RGB palette or a raw 1024-byte RGBA
palette. `save_palette(path)` writes the active palette as 1024 RGBA bytes.

```c
if (!load_palette("assets/night.pal"))
    log_message(LOG_WARNING, "%s", last_error());

if (!save_palette("saved/custom.pal"))
    log_message(LOG_WARNING, "%s", last_error());
```

Both functions return `true` on success and `false` on error.

### Palette rotation and fading

`rotate_palette(first, last, amount)` rotates an inclusive palette range.
Positive amounts move entries toward higher indices; negative amounts move them
toward lower indices.

```c
/* Cycle four water colours once to the right. */
rotate_palette(18, 21, 1);
```

`make_faded_palette(source, target, amount, result)` calculates a new palette
without changing the active one. `amount` is clamped between 0 and 1.

```c
Color base[RETRO2D_PALETTE_SIZE];
Color faded[RETRO2D_PALETTE_SIZE];

get_palette(base);
make_faded_palette(base, (Color){0, 0, 0, 255}, 0.5f, faded);
set_palette(faded);
```

`fade_palette_to(target, amount)` fades the current active palette in place.
It is convenient for a single operation, but repeated calls compound the fade.
For a time-based fade, retain a base palette and use `make_faded_palette` each
update.

```c
fade_palette_to((Color){255, 255, 255, 255}, 0.25f);
```

## 6. Primitive drawing, camera, and clips

All drawing colours are palette indices, not RGB values.

### Pixels and lines

| Function | Purpose | Short example |
|---|---|---|
| `put_pixel(x, y, colour)` | Sets one visible pixel after camera and clip processing. | `put_pixel(10, 20, 7);` |
| `get_pixel(x, y)` | Reads one screen pixel after applying the camera. It is bounded by the screen, not the draw clip. | `uint8_t under_player = get_pixel(x, y);` |
| `draw_horizontal_line(x, y, length, colour)` | Draws a horizontal run. Negative length draws leftward. | `draw_horizontal_line(10, 20, 50, 12);` |
| `draw_vertical_line(x, y, length, colour)` | Draws a vertical run. Negative length draws upward. | `draw_vertical_line(10, 20, 30, 12);` |
| `draw_line(x0, y0, x1, y1, colour)` | Draws a general Bresenham line including both endpoints. | `draw_line(0, 0, 319, 199, 15);` |

### Rectangles and circles

| Function | Purpose | Short example |
|---|---|---|
| `draw_rectangle(rectangle, colour)` | Draws a one-pixel rectangle outline. | `draw_rectangle((Rect){10, 10, 40, 20}, 8);` |
| `fill_rectangle(rectangle, colour)` | Fills a rectangle. | `fill_rectangle((Rect){10, 10, 40, 20}, 8);` |
| `draw_circle(circle, colour)` | Draws a one-pixel circle outline. | `draw_circle((Circle){160, 100, 24}, 9);` |
| `fill_circle(circle, colour)` | Fills a circle. | `fill_circle((Circle){160, 100, 24}, 9);` |

```c
fill_rectangle((Rect){20, 20, 80, 30}, 4);
draw_rectangle((Rect){20, 20, 80, 30}, 15);
fill_circle((Circle){160, 100, 12}, 6);
```

### Camera

`set_camera(x, y)` defines a global world-space offset. Drawing at `(x, y)` is
presented at `(x - camera.x, y - camera.y)`. `get_camera()` returns the current
offset.

```c
set_camera(player_x - 160, player_y - 100);
draw_image(world, 0, 0);

Point camera = get_camera();
log_message(LOG_INFO, "Camera: %d,%d", camera.x, camera.y);
```

Screen-space user-interface drawing can temporarily reset the camera:

```c
Point old_camera = get_camera();
set_camera(0, 0);
draw_text(font, "SCORE", 4, 4, ALIGN_LEFT);
set_camera(old_camera.x, old_camera.y);
```

### Nested clipping

`push_clip(rectangle)` intersects a world-space rectangle with the current clip
and pushes it. It returns `false` if the fixed clip stack is full.
`pop_clip()` restores the previous clip. `reset_clip()` restores the whole
screen regardless of stack depth.

```c
if (push_clip((Rect){40, 30, 120, 80})) {
    draw_image(large_map, 0, 0); /* Only the clipped window is changed. */
    pop_clip();
}

reset_clip();
```

Always pair successful pushes with pops. The implementation supports up to 16
clip levels, including the base screen clip.

## 7. Images and sprite drawing

### Loading and creating images

`load_image(path)` loads an uncompressed 8-bit, 24-bit, or 32-bit BMP. Indexed
BMP pixels keep their indices. True-colour BMP pixels are converted to the
nearest active palette colour at load time.

```c
Image *hero = load_image("assets/hero.bmp");
if (!hero) log_message(LOG_ERROR, "%s", last_error());
```

`create_image(width, height, pixels)` creates an indexed image from
`width * height` tightly packed bytes. Pass `NULL` for a zero-filled image. The
framework copies supplied pixels.

```c
uint8_t checker[4] = {1, 2, 2, 1};
Image *tile = create_image(2, 2, checker);
Image *blank = create_image(32, 32, NULL);
```

`set_image_alpha_threshold(threshold)` changes the threshold used when loading
subsequent 32-bit BMP alpha channels. The default is 128.

```c
set_image_alpha_threshold(96);
Image *soft_source = load_image("assets/source32.bmp");
```

### Image information and lifetime

| Function | Purpose | Short example |
|---|---|---|
| `image_width(image)` | Returns the image width, or zero for `NULL`. | `int w = image_width(hero);` |
| `image_height(image)` | Returns the image height, or zero for `NULL`. | `int h = image_height(hero);` |
| `free_image(image)` | Releases an image; accepting `NULL` makes conditional cleanup easy. | `free_image(hero); hero = NULL;` |

Do not free an image while a sprite sheet or borrowed bitmap font still uses it.

### Colour-key transparency

`image_set_colour_key(image, palette_index)` makes matching pixels transparent.
`image_clear_colour_key(image)` returns them to normal opaque drawing.

```c
image_set_colour_key(hero, 0);
draw_image(hero, 50, 60);

image_clear_colour_key(hero);
draw_image(hero, 80, 60);
```

### Explicit one-bit masks

`image_set_mask(image, bits, bit_stride)` copies a one-bit-per-pixel mask. Each
row begins at `bit_stride` bytes. The most-significant bit represents the
leftmost pixel; set bits draw and clear bits are transparent. It returns `false`
for invalid inputs or allocation failure. `image_clear_mask(image)` removes it.

```c
/* Two pixels: first opaque, second transparent. */
uint8_t mask[] = {0x80};
Image *two_pixels = create_image(2, 1, (uint8_t[]){4, 5});

if (image_set_mask(two_pixels, mask, 1))
    draw_image(two_pixels, 10, 10);

image_clear_mask(two_pixels);
```

An image may use both a mask and a colour key; a pixel draws only when both
tests say it is opaque.

### Drawing images

`draw_image(image, x, y)` draws the complete image at a world position.

```c
draw_image(hero, player_x, player_y);
```

`draw_image_part(image, source, x, y, flip)` draws one source rectangle with an
optional horizontal or vertical flip. It does not scale or rotate.

```c
Rect standing = {0, 0, 16, 24};
draw_image_part(hero, standing, 100, 80, FLIP_HORIZONTAL);
```

Source regions and destinations are safely clipped to the image and screen.

## 8. Sprite sheets and animations

A `SpriteSheet` divides an existing image into equal-sized frames. It borrows
the image and does not free it.

```c
Image *characters = load_image("assets/characters.bmp");
image_set_colour_key(characters, 0);
SpriteSheet *sheet = create_sprite_sheet(characters, 16, 24);
```

### Sprite-sheet functions

| Function | Purpose | Short example |
|---|---|---|
| `create_sprite_sheet(image, frame_width, frame_height)` | Creates an equal-cell sheet, or returns `NULL` for an invalid layout. | `SpriteSheet *s = create_sprite_sheet(image, 16, 16);` |
| `sprite_sheet_frame_count(sheet)` | Returns complete cells in the sheet. Partial edge cells are ignored. | `int count = sprite_sheet_frame_count(sheet);` |
| `sprite_frame_source(sheet, frame)` | Returns the source `Rect` for a zero-based frame. Invalid frames return an empty rectangle and set an error. | `Rect src = sprite_frame_source(sheet, 3);` |
| `draw_sprite_frame(sheet, frame, x, y, flip)` | Draws one frame using normal image transparency. | `draw_sprite_frame(sheet, 3, x, y, FLIP_NONE);` |
| `free_sprite_sheet(sheet)` | Releases the sheet but not its image. | `free_sprite_sheet(sheet);` |

### Creating animations

`create_animation(sheet, first_frame, frame_count, frames_per_second, loop)`
creates a consecutive frame range.

```c
Animation *walk = create_animation(sheet, 0, 6, 10.0f, true);
```

`create_animation_frames(sheet, frames, frame_count, frames_per_second, loop)`
copies an explicit frame sequence.

```c
int bounce_frames[] = {0, 1, 2, 3, 2, 1};
Animation *bounce = create_animation_frames(
    sheet, bounce_frames, 6, 12.0f, true);
```

Each animation owns independent playback state but borrows its sheet, allowing
many objects to animate from one loaded image.

### Animation control and drawing

| Function | Purpose | Short example |
|---|---|---|
| `animation_update(animation, delta)` | Advances playback. Normally call it once per fixed update. | `animation_update(walk, delta);` |
| `animation_play(animation)` | Resumes a paused animation unless a non-looping animation has finished. | `animation_play(walk);` |
| `animation_pause(animation)` | Stops time advancement while retaining the current frame. | `animation_pause(walk);` |
| `animation_reset(animation)` | Returns to the first sequence frame, clears finished state, and starts playback. | `animation_reset(attack);` |
| `animation_set_loop(animation, loop)` | Changes whether the end wraps. | `animation_set_loop(attack, false);` |
| `animation_set_speed(animation, frames_per_second)` | Changes speed; zero freezes advancement. | `animation_set_speed(walk, 8.0f);` |
| `animation_finished(animation)` | Reports whether a non-looping animation reached its end. | `if (animation_finished(attack)) state = IDLE;` |
| `animation_current_frame(animation)` | Returns the underlying sheet frame index, or `-1` for `NULL`. | `int frame = animation_current_frame(walk);` |
| `draw_animation(animation, x, y, flip)` | Draws the current frame. | `draw_animation(walk, player_x, player_y, FLIP_NONE);` |
| `free_animation(animation)` | Releases playback state and copied frame list, not the sheet. | `free_animation(walk);` |

A typical pair of callbacks is:

```c
void game_update(float delta)
{
    animation_update(walk, delta);
}

void game_draw(void)
{
    draw_animation(walk, player_x, player_y, FLIP_NONE);
}
```

## 9. Bitmap fonts and text

Glyphs occupy equal cells in a row-major indexed image.

`load_bitmap_font(path, glyph_width, glyph_height, first_character,
glyph_count, columns)` loads an image and creates a font that owns it.

```c
BitmapFont *font = load_bitmap_font(
    "assets/font.bmp", 8, 8, 32, 96, 16);
font_set_colour_key(font, 0);
```

`create_bitmap_font(image, ...)` creates the same layout from an existing image
but borrows the image.

```c
Image *font_image = load_image("assets/font.bmp");
BitmapFont *font = create_bitmap_font(font_image, 8, 8, 32, 96, 16);
```

Here, character code 32 is the first cell, 96 glyphs are available, and each row
contains 16 cells.

### Font functions

| Function | Purpose | Short example |
|---|---|---|
| `font_set_colour_key(font, palette_index)` | Applies a transparent colour key to the font's image. | `font_set_colour_key(font, 0);` |
| `font_set_spacing(font, character_spacing, line_spacing)` | Sets extra pixels between characters and lines. Negative spacing is allowed. | `font_set_spacing(font, 1, 2);` |
| `measure_text(font, text)` | Returns maximum line width in `x` and total multiline height in `y`. | `Point size = measure_text(font, "GAME OVER");` |
| `draw_text(font, text, x, y, alignment)` | Draws text, processing newline characters and aligning each line. | `draw_text(font, "GAME\nOVER", 160, 80, ALIGN_CENTRE);` |
| `free_bitmap_font(font)` | Releases the font. It also frees the image only when created by `load_bitmap_font`. | `free_bitmap_font(font);` |

Right-aligned status text can use the screen edge as its anchor:

```c
draw_text(font, "LIVES 3", 316, 4, ALIGN_RIGHT);
```

When using `create_bitmap_font`, free the font before its borrowed image:

```c
free_bitmap_font(font);
free_image(font_image);
```

## 10. Keyboard input

Input edges are delivered once per fixed update:

- Down means the key is currently held.
- Pressed means it changed from up to down for this update.
- Released means it changed from down to up for this update.

| Function | Purpose | Short example |
|---|---|---|
| `key_down(key)` | Tests the held state. Use for continuous movement. | `if (key_down(KEY_LEFT)) x -= 2;` |
| `key_pressed(key)` | Tests the one-update press edge. Use for actions and toggles. | `if (key_pressed(KEY_SPACE)) fire();` |
| `key_released(key)` | Tests the one-update release edge. | `if (key_released(KEY_SPACE)) stop_charging();` |

```c
void game_update(float delta)
{
    if (key_down(KEY_LEFT)) player_x -= (int)(90.0f * delta);
    if (key_pressed(KEY_P)) paused = !paused;
    if (key_released(KEY_SPACE)) launch_charged_shot();
}
```

## 11. Mouse input and cursor control

Mouse positions are converted from the scaled, letterboxed window into virtual
screen coordinates.

| Function | Purpose | Short example |
|---|---|---|
| `mouse_position()` | Returns virtual coordinates, or `{-1, -1}` over a black border/outside the screen. | `Point mouse = mouse_position();` |
| `mouse_movement()` | Returns accumulated virtual-pixel movement delivered to the current update. | `camera_x += mouse_movement().x;` |
| `mouse_inside_screen()` | Reports whether the pointer maps to a virtual pixel. | `if (!mouse_inside_screen()) return;` |
| `mouse_down(button)` | Tests whether a mouse button is held. | `if (mouse_down(MOUSE_LEFT)) paint(mouse_position());` |
| `mouse_pressed(button)` | Tests the one-update press edge. | `if (mouse_pressed(MOUSE_LEFT)) select_at(mouse_position());` |
| `mouse_released(button)` | Tests the one-update release edge. | `if (mouse_released(MOUSE_LEFT)) finish_drag();` |
| `mouse_wheel()` | Returns accumulated whole wheel steps for this update; zero means no movement. | `zoom += mouse_wheel();` |
| `show_cursor(visible)` | Shows or hides the operating-system cursor. | `show_cursor(menu_open);` |
| `confine_cursor(confined)` | Requests that the cursor stay inside the game window. | `confine_cursor(mouse_look);` |

Always test `mouse_inside_screen()` before using the position for a clickable
area:

```c
Point mouse = mouse_position();
if (mouse_inside_screen() && mouse_pressed(MOUSE_LEFT) &&
    point_in_rectangle(mouse, start_button)) {
    start_level();
}
```

## 12. Controller input

Retro2D exposes the first active SDL game controller.

| Function | Purpose | Short example |
|---|---|---|
| `controller_connected()` | Reports whether the framework currently has a controller. | `if (controller_connected()) draw_text(font, "PAD", 4, 4, ALIGN_LEFT);` |
| `controller_down(button)` | Tests a held controller button. | `if (controller_down(CONTROLLER_DPAD_LEFT)) x -= 2;` |
| `controller_pressed(button)` | Tests the one-update press edge. | `if (controller_pressed(CONTROLLER_A)) jump();` |
| `controller_released(button)` | Tests the one-update release edge. | `if (controller_released(CONTROLLER_A)) end_jump();` |
| `controller_axis(axis)` | Returns an axis value. Stick axes use approximately -1 to 1; triggers normally use 0 to 1. | `float move = controller_axis(CONTROLLER_LEFT_X);` |

Analogue movement commonly uses a dead zone:

```c
float move = controller_axis(CONTROLLER_LEFT_X);
if (move > -0.15f && move < 0.15f) move = 0.0f;
player_x += move * 100.0f * delta;
```

## 13. Sound effects

The baseline audio loader accepts uncompressed PCM WAVE files with 8-bit or
16-bit samples, mono or stereo. Audio is converted to 44.1 kHz stereo at load
time. If no audio device is available, resources can still load and playback
calls safely return an invalid handle.

### Loading, playing, and freeing sound

| Function | Purpose | Short example |
|---|---|---|
| `load_sound(path)` | Loads a PCM WAVE sound, returning `NULL` on failure. | `Sound *laser = load_sound("assets/laser.wav");` |
| `play_sound(sound)` | Starts one instance and returns its handle, or `RETRO2D_NO_PLAYBACK`. The handle may be ignored. | `Playback shot = play_sound(laser);` |
| `free_sound(sound)` | Stops all its instances and releases sample memory. | `free_sound(laser);` |

```c
Sound *laser = load_sound("assets/laser.wav");
if (!laser) log_message(LOG_ERROR, "%s", last_error());

/* Later: */
(void)play_sound(laser);
```

### Stopping and modifying playback

| Function | Purpose | Short example |
|---|---|---|
| `stop_playback(playback)` | Stops one specific instance by handle. | `stop_playback(engine_instance);` |
| `stop_sound(sound)` | Stops every active instance of one loaded sound. | `stop_sound(engine_sound);` |
| `stop_all_sounds()` | Stops all sound-effect voices but not music. | `stop_all_sounds();` |
| `set_playback_volume(playback, volume)` | Sets one instance's volume, clamped from 0 to 1. | `set_playback_volume(engine_instance, 0.4f);` |
| `set_playback_pan(playback, pan)` | Sets one instance from -1 (left) through 0 (centre) to 1 (right). | `set_playback_pan(shot, -0.6f);` |
| `set_sound_volume(volume)` | Sets global sound-effect volume from 0 to 1. It does not affect music volume. | `set_sound_volume(0.75f);` |

```c
Playback engine_instance = play_sound(engine_sound);
set_playback_volume(engine_instance, 0.35f);
set_playback_pan(engine_instance, player_x < 160 ? -0.5f : 0.5f);

/* When the engine stops: */
stop_playback(engine_instance);
```

The configured audio channel count controls simultaneous sound voices, up to
64. When all voices are occupied, a new sound replaces an existing voice.

## 14. Music

Music uses the same baseline PCM WAVE decoder but has a dedicated playback voice
and volume independent of sound effects.

| Function | Purpose | Short example |
|---|---|---|
| `load_music(path)` | Loads PCM WAVE music and returns `NULL` on failure. | `Music *theme = load_music("assets/theme.wav");` |
| `play_music(music, loop)` | Starts immediately from the beginning; `loop` chooses repetition. | `play_music(theme, true);` |
| `pause_music()` | Pauses the current position. | `pause_music();` |
| `resume_music()` | Resumes a paused active track. | `resume_music();` |
| `stop_music()` | Stops and clears the current music voice. | `stop_music();` |
| `fade_music_in(music, loop, seconds)` | Starts at zero gain and reaches full music gain over the duration. | `fade_music_in(theme, true, 1.5f);` |
| `fade_music_out(seconds)` | Fades the current track to silence and stops it. Zero stops immediately. | `fade_music_out(0.75f);` |
| `set_music_volume(volume)` | Sets music volume independently, clamped from 0 to 1. | `set_music_volume(0.6f);` |
| `music_playing()` | Reports whether a music track is active. A paused track remains active. | `if (music_playing()) pause_music();` |
| `free_music(music)` | Stops it if active and releases its samples. | `free_music(theme);` |

Because a paused track still counts as active, keep an explicit pause flag for a
toggle:

```c
static bool music_paused;

if (key_pressed(KEY_M)) {
    music_paused = !music_paused;
    if (music_paused) pause_music();
    else resume_music();
}
```

## 15. Software post-processing

A post effect runs after `game_draw` and before indexed pixels are converted to
the final window texture. Effects operate on palette indices.

The callback type is:

```c
typedef void (*PostEffect)(const uint8_t *source, uint8_t *destination,
                           int width, int height, int source_stride,
                           int destination_stride, const Color *palette,
                           float elapsed, void *user_data);
```

`source` and `destination` are always different. Before each callback, Retro2D
copies the source into the destination, so an effect may change only the pixels
it cares about. Do not retain either pointer after the callback.

### Complete horizontal-displacement effect

```c
typedef struct WaterSettings {
    int first_row;
    int strength;
} WaterSettings;

static void water_effect(const uint8_t *source, uint8_t *destination,
                         int width, int height,
                         int source_stride, int destination_stride,
                         const Color *palette, float elapsed, void *user_data)
{
    WaterSettings *settings = (WaterSettings *)user_data;
    int phase = (int)(elapsed * 20.0f);
    (void)palette;

    for (int y = settings->first_row; y < height; ++y) {
        int wave = ((y + phase) & 7) - 3;
        int offset = wave * settings->strength;

        for (int x = 0; x < width; ++x) {
            int sample_x = x + offset;
            if (sample_x < 0) sample_x = 0;
            if (sample_x >= width) sample_x = width - 1;
            destination[y * destination_stride + x] =
                source[y * source_stride + sample_x];
        }
    }
}

static WaterSettings water_settings = {100, 1};
static EffectHandle water;

void game_start(void)
{
    water = add_effect(water_effect, &water_settings);
}

void game_stop(void)
{
    remove_effect(water);
}
```

The `user_data` object must remain alive while the effect is registered.

### Effect-list functions

| Function | Purpose | Short example |
|---|---|---|
| `add_effect(effect, user_data)` | Appends an enabled effect and returns its nonzero handle; zero means failure. | `EffectHandle h = add_effect(water_effect, &settings);` |
| `remove_effect(handle)` | Removes one effect. Unknown handles are ignored. | `remove_effect(h);` |
| `enable_effect(handle, enabled)` | Enables or bypasses an effect without losing its position. | `enable_effect(h, water_visible);` |
| `move_effect(handle, new_index)` | Moves an effect to a zero-based position and returns whether the request was valid. | `move_effect(h, 0);` |
| `clear_effects()` | Removes every registered effect. | `clear_effects();` |

Effects run in list order. The output of one is the source of the next:

```c
EffectHandle water = add_effect(water_effect, &water_settings);
EffectHandle wipe = add_effect(wipe_effect, &wipe_amount);
move_effect(wipe, 0); /* Wipe now runs before water. */
```

With no enabled effects, Retro2D presents the framebuffer directly. It allocates
two scratch framebuffers once and reuses them rather than allocating each frame.
For fades, flashes, and colour cycling, prefer palette operations because they
do not copy the framebuffer.

## 16. Collision helpers

Collision functions answer geometric questions only. They never move objects or
apply physics.

| Function | Purpose | Short example |
|---|---|---|
| `point_in_rectangle(point, rectangle)` | Tests a point against a positive-sized rectangle. Left/top are inclusive; right/bottom are exclusive. | `if (point_in_rectangle(mouse, button)) click();` |
| `rectangles_intersect(a, b)` | Tests overlapping positive-sized rectangles. Merely touching edges do not intersect. | `if (rectangles_intersect(player_box, enemy_box)) hit();` |
| `circles_intersect(a, b)` | Tests whether two circles overlap or touch. | `if (circles_intersect(player_shield, projectile)) block();` |
| `image_masks_intersect(...)` | Tests overlapping opaque pixels in two image source regions at two destination positions. | See the mask example below. |

```c
Rect player_box = {player_x, player_y, 16, 24};
Rect coin_box = {coin_x, coin_y, 8, 8};

if (rectangles_intersect(player_box, coin_box)) collect_coin();
```

`image_masks_intersect` combines each image's mask and colour key. The source
rectangles select regions in the images; positions are the world-independent
top-left positions of those selected regions. This helper does not apply camera
offsets or sprite flips.

```c
Rect hero_frame = sprite_frame_source(hero_sheet,
                                      animation_current_frame(hero_animation));
Rect enemy_frame = sprite_frame_source(enemy_sheet, enemy_frame_index);

bool pixel_hit = image_masks_intersect(
    hero_image, hero_frame, (Point){player_x, player_y},
    enemy_image, enemy_frame, (Point){enemy_x, enemy_y});

if (pixel_hit) damage_player();
```

Use rectangle collision first as a cheap broad-phase check when testing many
objects, then call mask collision only for overlapping candidates.

## 17. A complete resource example

This condensed example shows correct loading, use, failure handling, and cleanup
for related resources:

```c
static Image *hero_image;
static SpriteSheet *hero_sheet;
static Animation *hero_walk;
static BitmapFont *font;
static Sound *step_sound;

void game_start(void)
{
    hero_image = load_image("assets/hero.bmp");
    if (hero_image) {
        image_set_colour_key(hero_image, 0);
        hero_sheet = create_sprite_sheet(hero_image, 16, 24);
    }
    if (hero_sheet)
        hero_walk = create_animation(hero_sheet, 0, 6, 10.0f, true);

    font = load_bitmap_font("assets/font.bmp", 8, 8, 32, 96, 16);
    font_set_colour_key(font, 0);
    step_sound = load_sound("assets/step.wav");

    if (!hero_walk || !font)
        log_message(LOG_ERROR, "Required asset failed: %s", last_error());
}

void game_update(float delta)
{
    animation_update(hero_walk, delta);
    if (key_pressed(KEY_SPACE)) (void)play_sound(step_sound);
}

void game_draw(void)
{
    clear_screen(0);
    draw_animation(hero_walk, 152, 88, FLIP_NONE);
    draw_text(font, "READY", 160, 4, ALIGN_CENTRE);
}

void game_stop(void)
{
    free_sound(step_sound);
    free_bitmap_font(font);
    free_animation(hero_walk);
    free_sprite_sheet(hero_sheet);
    free_image(hero_image);
}
```

Most draw, update, and free functions safely accept `NULL`, but constructors and
loaders should still be checked so the game can report missing required assets
and choose whether to continue.

## 18. Feature boundaries and current limitations

The core intentionally does not provide entities, scenes, physics, tile maps,
pathfinding, or genre-specific rules. These belong in game code or optional
modules.

The current baseline supports BMP images and PCM WAVE audio. PNG/JPEG,
OGG/MP3, TrueType fonts, partial alpha blending, arbitrary sprite scaling and
rotation, resource path caching, and GPU shaders are deferred. The software
post-effect API and direct framebuffer access remain available for custom visual
work without exposing SDL.

For build commands, example controls, Windows packaging, and deployment, see
`README.md`.
