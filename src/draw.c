#include "retro2d_internal.h"

#include <stdlib.h>

static bool visible(int64_t x, int64_t y)
{
    Rect clip = r2d_current_clip();
    return r2d.screen.pixels && x >= clip.x && y >= clip.y &&
           x < clip.x + clip.width && y < clip.y + clip.height;
}

static void raw_pixel(int64_t x, int64_t y, uint8_t colour)
{
    if (visible(x, y)) r2d.screen.pixels[(size_t)y * (size_t)r2d.screen.stride + (size_t)x] = colour;
}

void put_pixel(int x, int y, uint8_t colour)
{
    raw_pixel((int64_t)x - r2d.camera.x, (int64_t)y - r2d.camera.y, colour);
}

uint8_t get_pixel(int x, int y)
{
    int64_t screen_x = (int64_t)x - r2d.camera.x;
    int64_t screen_y = (int64_t)y - r2d.camera.y;
    if (!r2d.screen.pixels || screen_x < 0 || screen_y < 0 ||
        screen_x >= r2d.screen.width || screen_y >= r2d.screen.height) return 0;
    return r2d.screen.pixels[(size_t)screen_y * (size_t)r2d.screen.stride +
                             (size_t)screen_x];
}

void draw_horizontal_line(int x, int y, int length, uint8_t colour)
{
    int i;
    if (length < 0) { x += length + 1; length = -length; }
    for (i = 0; i < length; ++i) put_pixel(x + i, y, colour);
}

void draw_vertical_line(int x, int y, int length, uint8_t colour)
{
    int i;
    if (length < 0) { y += length + 1; length = -length; }
    for (i = 0; i < length; ++i) put_pixel(x, y + i, colour);
}

void draw_line(int x0, int y0, int x1, int y1, uint8_t colour)
{
    int dx = abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
    int dy = -abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
    int error = dx + dy;
    for (;;) {
        put_pixel(x0, y0, colour);
        if (x0 == x1 && y0 == y1) break;
        if (2 * error >= dy) { error += dy; x0 += sx; }
        if (2 * error <= dx) { error += dx; y0 += sy; }
    }
}

static Rect normalise(Rect rectangle)
{
    if (rectangle.width < 0) {
        rectangle.x += rectangle.width + 1;
        rectangle.width = -rectangle.width;
    }
    if (rectangle.height < 0) {
        rectangle.y += rectangle.height + 1;
        rectangle.height = -rectangle.height;
    }
    return rectangle;
}

void draw_rectangle(Rect rectangle, uint8_t colour)
{
    rectangle = normalise(rectangle);
    if (rectangle.width <= 0 || rectangle.height <= 0) return;
    draw_horizontal_line(rectangle.x, rectangle.y, rectangle.width, colour);
    if (rectangle.height > 1)
        draw_horizontal_line(rectangle.x, rectangle.y + rectangle.height - 1,
                             rectangle.width, colour);
    if (rectangle.height > 2) {
        draw_vertical_line(rectangle.x, rectangle.y + 1, rectangle.height - 2, colour);
        if (rectangle.width > 1)
            draw_vertical_line(rectangle.x + rectangle.width - 1, rectangle.y + 1,
                               rectangle.height - 2, colour);
    }
}

void fill_rectangle(Rect rectangle, uint8_t colour)
{
    int row;
    rectangle = normalise(rectangle);
    for (row = 0; row < rectangle.height; ++row)
        draw_horizontal_line(rectangle.x, rectangle.y + row, rectangle.width, colour);
}

void draw_circle(Circle circle, uint8_t colour)
{
    int x = circle.radius, y = 0, error = 1 - circle.radius;
    if (circle.radius < 0) return;
    while (x >= y) {
        put_pixel(circle.x + x, circle.y + y, colour);
        put_pixel(circle.x + y, circle.y + x, colour);
        put_pixel(circle.x - y, circle.y + x, colour);
        put_pixel(circle.x - x, circle.y + y, colour);
        put_pixel(circle.x - x, circle.y - y, colour);
        put_pixel(circle.x - y, circle.y - x, colour);
        put_pixel(circle.x + y, circle.y - x, colour);
        put_pixel(circle.x + x, circle.y - y, colour);
        ++y;
        if (error < 0) error += 2 * y + 1;
        else { --x; error += 2 * (y - x + 1); }
    }
}

void fill_circle(Circle circle, uint8_t colour)
{
    int y;
    if (circle.radius < 0) return;
    for (y = -circle.radius; y <= circle.radius; ++y) {
        int x = circle.radius;
        while (x > 0 && x * x + y * y > circle.radius * circle.radius) --x;
        draw_horizontal_line(circle.x - x, circle.y + y, x * 2 + 1, colour);
    }
}
