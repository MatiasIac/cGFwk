#include "retro2d_internal.h"

#include <limits.h>
#include <stdlib.h>
#include <string.h>

static void register_image(Image *image)
{
    image->next_resource = r2d.images;
    r2d.images = image;
}

static void unregister_image(Image *image)
{
    Image **link = &r2d.images;
    while (*link && *link != image) link = &(*link)->next_resource;
    if (*link) *link = image->next_resource;
}

Image *create_image(int width, int height, const uint8_t *pixels)
{
    Image *image;
    size_t size;
    if (width <= 0 || height <= 0 || width > 32768 || height > 32768 ||
        (size_t)width > SIZE_MAX / (size_t)height) {
        r2d_set_error("invalid image size %d x %d", width, height);
        return NULL;
    }
    size = (size_t)width * (size_t)height;
    image = (Image *)calloc(1, sizeof *image);
    if (!image || !(image->pixels = (uint8_t *)malloc(size))) {
        free(image);
        r2d_set_error("not enough memory for a %d x %d image", width, height);
        return NULL;
    }
    image->width = width;
    image->height = height;
    image->colour_key = -1;
    if (pixels) memcpy(image->pixels, pixels, size);
    else memset(image->pixels, 0, size);
    register_image(image);
    return image;
}

void free_image(Image *image)
{
    if (!image) return;
    unregister_image(image);
    free(image->pixels);
    free(image->mask);
    free(image);
}

int image_width(const Image *image) { return image ? image->width : 0; }
int image_height(const Image *image) { return image ? image->height : 0; }

void set_image_alpha_threshold(uint8_t threshold)
{
    r2d.alpha_threshold = threshold;
}

void image_set_colour_key(Image *image, int palette_index)
{
    if (!image || palette_index < 0 || palette_index >= RETRO2D_PALETTE_SIZE) {
        r2d_set_error("invalid image or colour-key palette index");
        return;
    }
    image->colour_key = palette_index;
}

void image_clear_colour_key(Image *image)
{
    if (image) image->colour_key = -1;
}

bool image_set_mask(Image *image, const uint8_t *bits, int bit_stride)
{
    uint8_t *copy;
    int required;
    if (!image || !bits) {
        r2d_set_error("an image mask needs an image and mask bits");
        return false;
    }
    required = (image->width + 7) / 8;
    if (bit_stride < required) {
        r2d_set_error("image mask stride %d is smaller than %d", bit_stride, required);
        return false;
    }
    copy = (uint8_t *)malloc((size_t)required * (size_t)image->height);
    if (!copy) {
        r2d_set_error("not enough memory for an image mask");
        return false;
    }
    for (int y = 0; y < image->height; ++y)
        memcpy(copy + (size_t)y * (size_t)required,
               bits + (size_t)y * (size_t)bit_stride, (size_t)required);
    free(image->mask);
    image->mask = copy;
    image->mask_stride = required;
    return true;
}

void image_clear_mask(Image *image)
{
    if (!image) return;
    free(image->mask);
    image->mask = NULL;
    image->mask_stride = 0;
}

static uint8_t nearest_colour(uint8_t red, uint8_t green, uint8_t blue)
{
    unsigned best_distance = UINT_MAX;
    int best = 0;
    for (int i = 0; i < RETRO2D_PALETTE_SIZE; ++i) {
        int dr = (int)red - r2d.palette[i].r;
        int dg = (int)green - r2d.palette[i].g;
        int db = (int)blue - r2d.palette[i].b;
        unsigned distance = (unsigned)(dr * dr + dg * dg + db * db);
        if (distance < best_distance) {
            best_distance = distance;
            best = i;
            if (!distance) break;
        }
    }
    return (uint8_t)best;
}

static bool skip_bytes(FILE *file, long count)
{
    return count >= 0 && fseek(file, count, SEEK_CUR) == 0;
}

Image *load_image(const char *path)
{
    FILE *file;
    bool ok = true;
    uint16_t signature, planes, bits;
    uint32_t pixel_offset, dib_size, compression, colours_used;
    int32_t width, signed_height;
    int height, top_down, bytes_per_pixel;
    size_t row_bytes, file_row_bytes;
    Image *image = NULL;
    uint8_t *row = NULL, *mask = NULL;
    int mask_stride = 0;
    bool any_alpha = false;

    if (!path || !(file = fopen(path, "rb"))) {
        r2d_set_error("could not open image '%s'", path ? path : "(null)");
        return NULL;
    }
    signature = r2d_read_u16(file, &ok);
    (void)r2d_read_u32(file, &ok);
    (void)r2d_read_u16(file, &ok);
    (void)r2d_read_u16(file, &ok);
    pixel_offset = r2d_read_u32(file, &ok);
    dib_size = r2d_read_u32(file, &ok);
    width = r2d_read_i32(file, &ok);
    signed_height = r2d_read_i32(file, &ok);
    planes = r2d_read_u16(file, &ok);
    bits = r2d_read_u16(file, &ok);
    compression = r2d_read_u32(file, &ok);
    (void)r2d_read_u32(file, &ok);
    (void)r2d_read_i32(file, &ok);
    (void)r2d_read_i32(file, &ok);
    colours_used = r2d_read_u32(file, &ok);
    (void)r2d_read_u32(file, &ok);
    if (!ok || signature != 0x4d42 || dib_size < 40 || planes != 1 ||
        width <= 0 || signed_height == 0 || signed_height == INT32_MIN ||
        (bits != 8 && bits != 24 && bits != 32) || compression != 0) {
        fclose(file);
        r2d_set_error("image '%s' is not an uncompressed 8, 24, or 32-bit BMP", path);
        return NULL;
    }
    height = signed_height < 0 ? -signed_height : signed_height;
    top_down = signed_height < 0;
    if (width > 32768 || height > 32768 || dib_size > (uint32_t)LONG_MAX ||
        pixel_offset > (uint32_t)LONG_MAX || !skip_bytes(file, (long)dib_size - 40L)) {
        fclose(file);
        r2d_set_error("image '%s' has unsupported dimensions or header", path);
        return NULL;
    }
    if (bits == 8) {
        uint32_t entries = colours_used ? colours_used : 256;
        if (entries > 256 || !skip_bytes(file, (long)entries * 4L)) ok = false;
    }
    if (!ok || fseek(file, (long)pixel_offset, SEEK_SET) != 0) {
        fclose(file);
        r2d_set_error("image '%s' has a truncated BMP header", path);
        return NULL;
    }
    image = create_image(width, height, NULL);
    if (!image) { fclose(file); return NULL; }
    bytes_per_pixel = bits / 8;
    row_bytes = (size_t)width * (size_t)bytes_per_pixel;
    file_row_bytes = (row_bytes + 3u) & ~(size_t)3u;
    row = (uint8_t *)malloc(file_row_bytes);
    if (bits == 32) {
        mask_stride = (width + 7) / 8;
        mask = (uint8_t *)calloc((size_t)mask_stride * (size_t)height, 1);
    }
    if (!row || (bits == 32 && !mask)) ok = false;
    for (int file_y = 0; ok && file_y < height; ++file_y) {
        int y = top_down ? file_y : height - file_y - 1;
        if (fread(row, 1, file_row_bytes, file) != file_row_bytes) { ok = false; break; }
        for (int x = 0; x < width; ++x) {
            uint8_t index;
            if (bits == 8) index = row[x];
            else {
                const uint8_t *pixel = &row[(size_t)x * (size_t)bytes_per_pixel];
                index = nearest_colour(pixel[2], pixel[1], pixel[0]);
                if (bits == 32 && pixel[3] != 0) any_alpha = true;
                if (bits == 32 && pixel[3] >= r2d.alpha_threshold)
                    mask[(size_t)y * (size_t)mask_stride + (size_t)(x / 8)] |=
                        (uint8_t)(0x80u >> (x & 7));
            }
            image->pixels[(size_t)y * (size_t)width + (size_t)x] = index;
        }
    }
    fclose(file);
    free(row);
    if (!ok) {
        free(mask);
        free_image(image);
        r2d_set_error("image '%s' contains truncated pixel data", path);
        return NULL;
    }
    if (bits == 32 && any_alpha) {
        image->mask = mask;
        image->mask_stride = mask_stride;
    } else free(mask);
    return image;
}

static bool image_pixel_opaque(const Image *image, int x, int y, uint8_t pixel)
{
    if (image->colour_key >= 0 && pixel == (uint8_t)image->colour_key) return false;
    return !image->mask || (image->mask[(size_t)y * (size_t)image->mask_stride +
           (size_t)(x / 8)] & (uint8_t)(0x80u >> (x & 7))) != 0;
}

void draw_image_part(const Image *image, Rect source, int x, int y, Flip flip)
{
    Rect clip;
    int64_t base_x, base_y, destination_x, destination_y;
    if (!image || !image->pixels || source.width <= 0 || source.height <= 0) return;
    base_x = (int64_t)x - r2d.camera.x;
    base_y = (int64_t)y - r2d.camera.y;
    clip = r2d_current_clip();
    for (int dy = 0; dy < source.height; ++dy) {
        int sy = (flip & FLIP_VERTICAL) ? source.y + source.height - 1 - dy : source.y + dy;
        destination_y = base_y + dy;
        if (destination_y < clip.y || destination_y >= clip.y + clip.height ||
            sy < 0 || sy >= image->height) continue;
        for (int dx = 0; dx < source.width; ++dx) {
            int sx = (flip & FLIP_HORIZONTAL) ? source.x + source.width - 1 - dx : source.x + dx;
            uint8_t pixel;
            destination_x = base_x + dx;
            if (destination_x < clip.x || destination_x >= clip.x + clip.width ||
                sx < 0 || sx >= image->width) continue;
            pixel = image->pixels[(size_t)sy * (size_t)image->width + (size_t)sx];
            if (image_pixel_opaque(image, sx, sy, pixel))
                r2d.screen.pixels[(size_t)destination_y * (size_t)r2d.screen.stride +
                                  (size_t)destination_x] = pixel;
        }
    }
}

void draw_image(const Image *image, int x, int y)
{
    if (image) draw_image_part(image, (Rect){ 0, 0, image->width, image->height },
                               x, y, FLIP_NONE);
}
