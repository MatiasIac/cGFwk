#include "retro2d_internal.h"

bool point_in_rectangle(Point point, Rect rectangle)
{
    return rectangle.width > 0 && rectangle.height > 0 &&
           point.x >= rectangle.x && point.y >= rectangle.y &&
           (int64_t)point.x < (int64_t)rectangle.x + rectangle.width &&
           (int64_t)point.y < (int64_t)rectangle.y + rectangle.height;
}

bool rectangles_intersect(Rect a, Rect b)
{
    return a.width > 0 && a.height > 0 && b.width > 0 && b.height > 0 &&
           (int64_t)a.x < (int64_t)b.x + b.width &&
           (int64_t)a.x + a.width > b.x &&
           (int64_t)a.y < (int64_t)b.y + b.height &&
           (int64_t)a.y + a.height > b.y;
}

bool circles_intersect(Circle a, Circle b)
{
    int64_t dx = (int64_t)a.x - b.x;
    int64_t dy = (int64_t)a.y - b.y;
    int64_t radii = (int64_t)a.radius + b.radius;
    return a.radius >= 0 && b.radius >= 0 && dx * dx + dy * dy <= radii * radii;
}

static bool opaque_at(const Image *image, int x, int y)
{
    size_t offset;
    uint8_t pixel;
    if (!image || x < 0 || y < 0 || x >= image->width || y >= image->height) return false;
    offset = (size_t)y * (size_t)image->width + (size_t)x;
    pixel = image->pixels[offset];
    if (image->colour_key >= 0 && pixel == (uint8_t)image->colour_key) return false;
    if (image->mask && !(image->mask[(size_t)y * (size_t)image->mask_stride + (size_t)(x / 8)] &
                         (uint8_t)(0x80u >> (x & 7)))) return false;
    return true;
}

bool image_masks_intersect(const Image *a, Rect source_a, Point position_a,
                           const Image *b, Rect source_b, Point position_b)
{
    Rect bounds_a = { position_a.x, position_a.y, source_a.width, source_a.height };
    Rect bounds_b = { position_b.x, position_b.y, source_b.width, source_b.height };
    int left, top, right, bottom, x, y;
    if (!a || !b || !rectangles_intersect(bounds_a, bounds_b)) return false;
    left = bounds_a.x > bounds_b.x ? bounds_a.x : bounds_b.x;
    top = bounds_a.y > bounds_b.y ? bounds_a.y : bounds_b.y;
    right = bounds_a.x + bounds_a.width < bounds_b.x + bounds_b.width
          ? bounds_a.x + bounds_a.width : bounds_b.x + bounds_b.width;
    bottom = bounds_a.y + bounds_a.height < bounds_b.y + bounds_b.height
           ? bounds_a.y + bounds_a.height : bounds_b.y + bounds_b.height;
    for (y = top; y < bottom; ++y) {
        for (x = left; x < right; ++x) {
            if (opaque_at(a, source_a.x + x - position_a.x,
                          source_a.y + y - position_a.y) &&
                opaque_at(b, source_b.x + x - position_b.x,
                          source_b.y + y - position_b.y)) return true;
        }
    }
    return false;
}
