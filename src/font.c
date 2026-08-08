#include "retro2d_internal.h"

#include <stdlib.h>

static BitmapFont *new_font(Image *image, int glyph_width, int glyph_height,
                            int first_character, int glyph_count, int columns,
                            bool owns_image)
{
    BitmapFont *font;
    int rows;
    if (!image || glyph_width <= 0 || glyph_height <= 0 || glyph_count <= 0 ||
        columns <= 0 || columns > image->width / glyph_width) {
        r2d_set_error("invalid bitmap font layout");
        return NULL;
    }
    rows = (glyph_count - 1) / columns + 1;
    if (rows > image->height / glyph_height) {
        r2d_set_error("bitmap font glyphs do not fit in the image");
        return NULL;
    }
    font = (BitmapFont *)calloc(1, sizeof *font);
    if (!font) { r2d_set_error("not enough memory for a bitmap font"); return NULL; }
    font->image = image;
    font->owns_image = owns_image;
    font->glyph_width = glyph_width;
    font->glyph_height = glyph_height;
    font->first_character = first_character;
    font->glyph_count = glyph_count;
    font->columns = columns;
    font->next_resource = r2d.fonts;
    r2d.fonts = font;
    return font;
}

BitmapFont *create_bitmap_font(Image *image, int glyph_width, int glyph_height,
                               int first_character, int glyph_count, int columns)
{
    return new_font(image, glyph_width, glyph_height, first_character, glyph_count,
                    columns, false);
}

BitmapFont *load_bitmap_font(const char *path, int glyph_width, int glyph_height,
                             int first_character, int glyph_count, int columns)
{
    Image *image = load_image(path);
    BitmapFont *font;
    if (!image) return NULL;
    font = new_font(image, glyph_width, glyph_height, first_character, glyph_count,
                    columns, true);
    if (!font) free_image(image);
    return font;
}

void free_bitmap_font(BitmapFont *font)
{
    BitmapFont **link = &r2d.fonts;
    if (!font) return;
    while (*link && *link != font) link = &(*link)->next_resource;
    if (*link) *link = font->next_resource;
    if (font->owns_image) free_image(font->image);
    free(font);
}

void font_set_colour_key(BitmapFont *font, int palette_index)
{
    if (font) image_set_colour_key(font->image, palette_index);
}

void font_set_spacing(BitmapFont *font, int character_spacing, int line_spacing)
{
    if (!font) return;
    font->character_spacing = character_spacing;
    font->line_spacing = line_spacing;
}

static int line_width(const BitmapFont *font, const char *text)
{
    int count = 0;
    while (text[count] && text[count] != '\n') ++count;
    return count ? count * font->glyph_width + (count - 1) * font->character_spacing : 0;
}

Point measure_text(const BitmapFont *font, const char *text)
{
    Point size = { 0, 0 };
    int current = 0, lines = 1;
    if (!font || !text || !*text) return size;
    for (const char *p = text; ; ++p) {
        if (*p == '\n' || *p == '\0') {
            int width = current ? current * font->glyph_width +
                                  (current - 1) * font->character_spacing : 0;
            if (width > size.x) size.x = width;
            current = 0;
            if (*p == '\0') break;
            ++lines;
        } else ++current;
    }
    size.y = lines * font->glyph_height + (lines - 1) * font->line_spacing;
    return size;
}

void draw_text(const BitmapFont *font, const char *text, int x, int y,
               TextAlign alignment)
{
    int pen_x, pen_y;
    const char *p;
    if (!font || !text) return;
    p = text;
    pen_y = y;
    while (*p) {
        int width = line_width(font, p);
        const char *line_end = p;
        pen_x = x - (alignment == ALIGN_CENTRE ? width / 2 :
                     (alignment == ALIGN_RIGHT ? width : 0));
        while (*line_end && *line_end != '\n') {
            int glyph = (int)(unsigned char)*line_end - font->first_character;
            if (glyph >= 0 && glyph < font->glyph_count) {
                Rect source = { (glyph % font->columns) * font->glyph_width,
                                (glyph / font->columns) * font->glyph_height,
                                font->glyph_width, font->glyph_height };
                draw_image_part(font->image, source, pen_x, pen_y, FLIP_NONE);
            }
            pen_x += font->glyph_width + font->character_spacing;
            ++line_end;
        }
        if (!*line_end) break;
        p = line_end + 1;
        pen_y += font->glyph_height + font->line_spacing;
    }
}
