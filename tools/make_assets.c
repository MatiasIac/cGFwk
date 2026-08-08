#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef uint8_t (*PixelFunction)(int x, int y);

static void write_u16(FILE *file, uint16_t value)
{
    fputc(value & 255, file);
    fputc((value >> 8) & 255, file);
}

static void write_u32(FILE *file, uint32_t value)
{
    fputc(value & 255, file);
    fputc((value >> 8) & 255, file);
    fputc((value >> 16) & 255, file);
    fputc((value >> 24) & 255, file);
}

static void palette_colour(int index, uint8_t *red, uint8_t *green, uint8_t *blue)
{
    if (index < 216) {
        *red = (uint8_t)((index / 36) * 51);
        *green = (uint8_t)(((index / 6) % 6) * 51);
        *blue = (uint8_t)((index % 6) * 51);
    } else {
        *red = *green = *blue = (uint8_t)(((index - 216) * 255) / 39);
    }
}

static int write_bmp(const char *path, int width, int height, PixelFunction pixel)
{
    FILE *file = fopen(path, "wb");
    int stride = (width + 3) & ~3;
    uint32_t pixel_offset = 14u + 40u + 1024u;
    uint32_t file_size = pixel_offset + (uint32_t)(stride * height);
    if (!file) return 0;
    write_u16(file, 0x4d42);
    write_u32(file, file_size);
    write_u16(file, 0); write_u16(file, 0); write_u32(file, pixel_offset);
    write_u32(file, 40); write_u32(file, (uint32_t)width); write_u32(file, (uint32_t)height);
    write_u16(file, 1); write_u16(file, 8); write_u32(file, 0);
    write_u32(file, (uint32_t)(stride * height));
    write_u32(file, 2835); write_u32(file, 2835); write_u32(file, 256); write_u32(file, 256);
    for (int i = 0; i < 256; ++i) {
        uint8_t r, g, b;
        palette_colour(i, &r, &g, &b);
        fputc(b, file); fputc(g, file); fputc(r, file); fputc(0, file);
    }
    for (int y = height - 1; y >= 0; --y) {
        for (int x = 0; x < width; ++x) fputc(pixel(x, y), file);
        for (int x = width; x < stride; ++x) fputc(0, file);
    }
    fclose(file);
    return 1;
}

static uint8_t sprite_pixel(int x, int y)
{
    int frame = x / 16;
    int local_x = x % 16;
    int centre = 7 + (frame & 1);
    if (y >= 3 && y <= 12 && local_x >= centre - (y - 3) / 2 &&
        local_x <= centre + (y - 3) / 2) return 35;
    if (y >= 9 && y <= 13 && (local_x == 3 || local_x == 12)) return 180;
    if (y >= 13 && (local_x == centre - 2 || local_x == centre + 2))
        return frame & 1 ? 210 : 180;
    if (y >= 5 && y <= 8 && local_x >= centre - 1 && local_x <= centre + 1) return 215;
    return 0;
}

static const char glyph_chars[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
static const uint8_t glyph_rows[][7] = {
    {14,17,17,31,17,17,17},{30,17,17,30,17,17,30},{14,17,16,16,16,17,14},
    {30,17,17,17,17,17,30},{31,16,16,30,16,16,31},{31,16,16,30,16,16,16},
    {14,17,16,23,17,17,15},{17,17,17,31,17,17,17},{14,4,4,4,4,4,14},
    {7,2,2,2,18,18,12},{17,18,20,24,20,18,17},{16,16,16,16,16,16,31},
    {17,27,21,21,17,17,17},{17,25,21,19,17,17,17},{14,17,17,17,17,17,14},
    {30,17,17,30,16,16,16},{14,17,17,17,21,18,13},{30,17,17,30,20,18,17},
    {15,16,16,14,1,1,30},{31,4,4,4,4,4,4},{17,17,17,17,17,17,14},
    {17,17,17,17,17,10,4},{17,17,17,21,21,21,10},{17,17,10,4,10,17,17},
    {17,17,10,4,4,4,4},{31,1,2,4,8,16,31},
    {14,17,19,21,25,17,14},{4,12,4,4,4,4,14},{14,17,1,2,4,8,31},
    {30,1,1,14,1,1,30},{2,6,10,18,31,2,2},{31,16,16,30,1,1,30},
    {14,16,16,30,17,17,14},{31,1,2,4,8,8,8},{14,17,17,14,17,17,14},
    {14,17,17,15,1,1,14}
};

static uint8_t font_pixel(int x, int y)
{
    int column = x / 8, row = y / 8;
    int character = 32 + row * 16 + column;
    int local_x = x % 8, local_y = y % 8;
    const char *found;
    if (local_x < 1 || local_x > 5 || local_y >= 7) return 0;
    found = strchr(glyph_chars, character);
    if (!found) return character == '.' && local_y == 6 && local_x == 3 ? 215 : 0;
    return (glyph_rows[found - glyph_chars][local_y] & (1u << (5 - local_x))) ? 215 : 0;
}

static uint8_t room_pixel(int x, int y)
{
    if (y < 110) {
        if (x > 30 && x < 120 && y > 25 && y < 90) return 104;
        if (x > 38 && x < 112 && y > 33 && y < 82) return 28;
        if (x > 205 && x < 285 && y > 18 && y < 110) return 74;
        if (x > 214 && x < 276 && y > 29 && y < 110) return 38;
        return 97;
    }
    if (((x / 20) + (y / 12)) & 1) return 55;
    return 49;
}

static uint8_t water_pixel(int x, int y)
{
    if (y < 92) {
        if ((x * 3 + y * 5) % 97 == 0) return 215;
        return 2 + (uint8_t)((y / 24) * 6);
    }
    if (y == 92 || y == 93) return 210;
    return (uint8_t)(18 + ((x / 12 + y / 5) & 3));
}

static int write_wave(const char *path, int music)
{
    const int rate = 22050;
    int frames = music ? rate * 4 : rate / 8;
    uint32_t data_size = (uint32_t)frames * 2u;
    FILE *file = fopen(path, "wb");
    static const int notes[] = { 262, 330, 392, 523, 392, 330, 294, 392 };
    if (!file) return 0;
    fwrite("RIFF", 1, 4, file); write_u32(file, 36u + data_size); fwrite("WAVE", 1, 4, file);
    fwrite("fmt ", 1, 4, file); write_u32(file, 16); write_u16(file, 1); write_u16(file, 1);
    write_u32(file, rate); write_u32(file, rate * 2); write_u16(file, 2); write_u16(file, 16);
    fwrite("data", 1, 4, file); write_u32(file, data_size);
    for (int i = 0; i < frames; ++i) {
        int sample;
        if (music) {
            int note = notes[(i / (rate / 2)) % 8];
            sample = ((i % (rate / note)) < (rate / note) / 2 ? 1 : -1) * 3500;
        } else {
            int amplitude = 12000 * (frames - i) / frames;
            sample = ((i / 9) & 1 ? amplitude : -amplitude);
        }
        write_u16(file, (uint16_t)(int16_t)sample);
    }
    fclose(file);
    return 1;
}

static void make_path(char *result, size_t size, const char *directory, const char *name)
{
    size_t length = strlen(directory);
    snprintf(result, size, "%s%s%s", directory,
             length && (directory[length - 1] == '/' || directory[length - 1] == '\\') ? "" : "/",
             name);
}

int main(int argc, char **argv)
{
    char path[1024];
    const char *directory = argc > 1 ? argv[1] : "assets";
    int ok = 1;
    make_path(path, sizeof path, directory, "sprites.bmp"); ok &= write_bmp(path, 64, 16, sprite_pixel);
    make_path(path, sizeof path, directory, "font.bmp"); ok &= write_bmp(path, 128, 48, font_pixel);
    make_path(path, sizeof path, directory, "room.bmp"); ok &= write_bmp(path, 320, 200, room_pixel);
    make_path(path, sizeof path, directory, "water.bmp"); ok &= write_bmp(path, 320, 200, water_pixel);
    make_path(path, sizeof path, directory, "laser.wav"); ok &= write_wave(path, 0);
    make_path(path, sizeof path, directory, "music.wav"); ok &= write_wave(path, 1);
    if (!ok) fprintf(stderr, "Could not create one or more Retro2D example assets.\n");
    return ok ? 0 : 1;
}
