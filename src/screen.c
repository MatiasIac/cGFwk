#include "retro2d_internal.h"

#include <stdlib.h>
#include <string.h>
#include <limits.h>

static Rect intersect_rects(Rect a, Rect b)
{
    int64_t left = a.x > b.x ? a.x : b.x;
    int64_t top = a.y > b.y ? a.y : b.y;
    int64_t right_a = (int64_t)a.x + (a.width > 0 ? a.width : 0);
    int64_t right_b = (int64_t)b.x + (b.width > 0 ? b.width : 0);
    int64_t bottom_a = (int64_t)a.y + (a.height > 0 ? a.height : 0);
    int64_t bottom_b = (int64_t)b.y + (b.height > 0 ? b.height : 0);
    int64_t right = right_a < right_b ? right_a : right_b;
    int64_t bottom = bottom_a < bottom_b ? bottom_a : bottom_b;
    Rect result;
    if (right <= left || bottom <= top) return (Rect){ 0, 0, 0, 0 };
    result = (Rect){ (int)left, (int)top, (int)(right - left), (int)(bottom - top) };
    return result;
}

static int saturated_subtract(int a, int b)
{
    int64_t result = (int64_t)a - b;
    if (result < INT_MIN) return INT_MIN;
    if (result > INT_MAX) return INT_MAX;
    return (int)result;
}

static void make_default_palette(void)
{
    int index = 0;
    int r, g, b;
    for (r = 0; r < 6; ++r) {
        for (g = 0; g < 6; ++g) {
            for (b = 0; b < 6; ++b) {
                r2d.palette[index++] = (Color){ (uint8_t)(r * 51),
                                                (uint8_t)(g * 51),
                                                (uint8_t)(b * 51), 255 };
            }
        }
    }
    while (index < RETRO2D_PALETTE_SIZE) {
        uint8_t grey = (uint8_t)(((index - 216) * 255) / 39);
        r2d.palette[index++] = (Color){ grey, grey, grey, 255 };
    }
}

bool r2d_screen_init(int width, int height)
{
    size_t size;
    if (width <= 0 || height <= 0 || width > 8192 || height > 8192) {
        r2d_set_error("invalid virtual screen size %d x %d", width, height);
        return false;
    }
    size = (size_t)width * (size_t)height;
    if (size / (size_t)width != (size_t)height) {
        r2d_set_error("virtual screen size is too large");
        return false;
    }
    r2d.screen.pixels = (uint8_t *)calloc(size, 1);
    r2d.effect_a = (uint8_t *)malloc(size);
    r2d.effect_b = (uint8_t *)malloc(size);
    if (!r2d.screen.pixels || !r2d.effect_a || !r2d.effect_b) {
        r2d_screen_shutdown();
        r2d_set_error("not enough memory for the virtual screen");
        return false;
    }
    r2d.screen.width = width;
    r2d.screen.height = height;
    r2d.screen.stride = width;
    r2d.camera = (Point){ 0, 0 };
    r2d.clip_count = 1;
    r2d.clips[0] = (Rect){ 0, 0, width, height };
    make_default_palette();
    return true;
}

void r2d_screen_shutdown(void)
{
    free(r2d.screen.pixels);
    free(r2d.effect_a);
    free(r2d.effect_b);
    r2d.screen = (Framebuffer){ 0 };
    r2d.effect_a = NULL;
    r2d.effect_b = NULL;
    r2d.clip_count = 0;
}

Framebuffer get_framebuffer(void)
{
    return r2d.screen;
}

void clear_screen(uint8_t colour)
{
    if (r2d.screen.pixels) {
        memset(r2d.screen.pixels, colour,
               (size_t)r2d.screen.stride * (size_t)r2d.screen.height);
    }
}

void set_palette_colour(int index, Color colour)
{
    if (index < 0 || index >= RETRO2D_PALETTE_SIZE) {
        r2d_set_error("palette index %d is outside 0..255", index);
        return;
    }
    r2d.palette[index] = colour;
}

Color get_palette_colour(int index)
{
    if (index < 0 || index >= RETRO2D_PALETTE_SIZE) {
        r2d_set_error("palette index %d is outside 0..255", index);
        return (Color){ 0, 0, 0, 0 };
    }
    return r2d.palette[index];
}

void set_palette(const Color colours[RETRO2D_PALETTE_SIZE])
{
    if (!colours) {
        r2d_set_error("cannot set a palette from NULL");
        return;
    }
    memcpy(r2d.palette, colours, sizeof r2d.palette);
}

void get_palette(Color colours[RETRO2D_PALETTE_SIZE])
{
    if (!colours) {
        r2d_set_error("cannot copy the palette to NULL");
        return;
    }
    memcpy(colours, r2d.palette, sizeof r2d.palette);
}

bool load_palette(const char *path)
{
    FILE *file;
    long size;
    uint8_t bytes[RETRO2D_PALETTE_SIZE * 4];
    int i, components;
    if (!path || !(file = fopen(path, "rb"))) {
        r2d_set_error("could not open palette '%s'", path ? path : "(null)");
        return false;
    }
    if (fseek(file, 0, SEEK_END) != 0 || (size = ftell(file)) < 0 ||
        fseek(file, 0, SEEK_SET) != 0) {
        fclose(file);
        r2d_set_error("could not inspect palette '%s'", path);
        return false;
    }
    components = size == 768 ? 3 : (size == 1024 ? 4 : 0);
    if (!components || fread(bytes, 1, (size_t)size, file) != (size_t)size) {
        fclose(file);
        r2d_set_error("palette '%s' must contain 768 RGB or 1024 RGBA bytes", path);
        return false;
    }
    fclose(file);
    for (i = 0; i < RETRO2D_PALETTE_SIZE; ++i) {
        r2d.palette[i] = (Color){ bytes[i * components], bytes[i * components + 1],
                                  bytes[i * components + 2],
                                  components == 4 ? bytes[i * 4 + 3] : 255 };
    }
    return true;
}

bool save_palette(const char *path)
{
    FILE *file;
    uint8_t bytes[RETRO2D_PALETTE_SIZE * 4];
    int i;
    if (!path || !(file = fopen(path, "wb"))) {
        r2d_set_error("could not create palette '%s'", path ? path : "(null)");
        return false;
    }
    for (i = 0; i < RETRO2D_PALETTE_SIZE; ++i) {
        bytes[i * 4] = r2d.palette[i].r;
        bytes[i * 4 + 1] = r2d.palette[i].g;
        bytes[i * 4 + 2] = r2d.palette[i].b;
        bytes[i * 4 + 3] = r2d.palette[i].a;
    }
    if (fwrite(bytes, 1, sizeof bytes, file) != sizeof bytes) {
        fclose(file);
        r2d_set_error("could not write palette '%s'", path);
        return false;
    }
    fclose(file);
    return true;
}

void rotate_palette(int first, int last, int amount)
{
    Color temporary[RETRO2D_PALETTE_SIZE];
    int length, i, shift;
    if (first < 0 || last >= RETRO2D_PALETTE_SIZE || first > last) {
        r2d_set_error("invalid palette rotation range %d..%d", first, last);
        return;
    }
    length = last - first + 1;
    shift = amount % length;
    if (shift < 0) shift += length;
    memcpy(temporary, &r2d.palette[first], (size_t)length * sizeof(Color));
    for (i = 0; i < length; ++i) {
        r2d.palette[first + (i + shift) % length] = temporary[i];
    }
}

void make_faded_palette(const Color source[RETRO2D_PALETTE_SIZE], Color target,
                         float amount, Color result[RETRO2D_PALETTE_SIZE])
{
    int i;
    float t = r2d_clamp01(amount);
    if (!source || !result) {
        r2d_set_error("palette fade needs source and destination palettes");
        return;
    }
    for (i = 0; i < RETRO2D_PALETTE_SIZE; ++i) {
        result[i].r = (uint8_t)(source[i].r + (target.r - source[i].r) * t + 0.5f);
        result[i].g = (uint8_t)(source[i].g + (target.g - source[i].g) * t + 0.5f);
        result[i].b = (uint8_t)(source[i].b + (target.b - source[i].b) * t + 0.5f);
        result[i].a = (uint8_t)(source[i].a + (target.a - source[i].a) * t + 0.5f);
    }
}

void fade_palette_to(Color target, float amount)
{
    Color faded[RETRO2D_PALETTE_SIZE];
    make_faded_palette(r2d.palette, target, amount, faded);
    set_palette(faded);
}

void set_camera(int x, int y)
{
    r2d.camera = (Point){ x, y };
}

Point get_camera(void)
{
    return r2d.camera;
}

Rect r2d_current_clip(void)
{
    if (r2d.clip_count <= 0) return (Rect){ 0, 0, 0, 0 };
    return r2d.clips[r2d.clip_count - 1];
}

bool push_clip(Rect rectangle)
{
    Rect screen = { 0, 0, r2d.screen.width, r2d.screen.height };
    if (r2d.clip_count >= R2D_MAX_CLIPS) {
        r2d_set_error("clip stack is full");
        return false;
    }
    rectangle.x = saturated_subtract(rectangle.x, r2d.camera.x);
    rectangle.y = saturated_subtract(rectangle.y, r2d.camera.y);
    rectangle = intersect_rects(rectangle, screen);
    if (r2d.clip_count > 0) rectangle = intersect_rects(rectangle, r2d_current_clip());
    r2d.clips[r2d.clip_count++] = rectangle;
    return true;
}

void pop_clip(void)
{
    if (r2d.clip_count > 1) --r2d.clip_count;
}

void reset_clip(void)
{
    r2d.clip_count = 1;
    r2d.clips[0] = (Rect){ 0, 0, r2d.screen.width, r2d.screen.height };
}
