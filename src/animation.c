#include "retro2d_internal.h"

#include <stdlib.h>
#include <string.h>

SpriteSheet *create_sprite_sheet(Image *image, int frame_width, int frame_height)
{
    SpriteSheet *sheet;
    if (!image || frame_width <= 0 || frame_height <= 0 ||
        frame_width > image->width || frame_height > image->height) {
        r2d_set_error("invalid image or frame size for sprite sheet");
        return NULL;
    }
    sheet = (SpriteSheet *)calloc(1, sizeof *sheet);
    if (!sheet) { r2d_set_error("not enough memory for a sprite sheet"); return NULL; }
    sheet->image = image;
    sheet->frame_width = frame_width;
    sheet->frame_height = frame_height;
    sheet->columns = image->width / frame_width;
    sheet->rows = image->height / frame_height;
    sheet->next_resource = r2d.sheets;
    r2d.sheets = sheet;
    return sheet;
}

void free_sprite_sheet(SpriteSheet *sheet)
{
    SpriteSheet **link = &r2d.sheets;
    if (!sheet) return;
    while (*link && *link != sheet) link = &(*link)->next_resource;
    if (*link) *link = sheet->next_resource;
    free(sheet);
}

int sprite_sheet_frame_count(const SpriteSheet *sheet)
{
    return sheet ? sheet->columns * sheet->rows : 0;
}

Rect sprite_frame_source(const SpriteSheet *sheet, int frame)
{
    if (!sheet || frame < 0 || frame >= sprite_sheet_frame_count(sheet)) {
        r2d_set_error("sprite frame %d is out of range", frame);
        return (Rect){ 0, 0, 0, 0 };
    }
    return (Rect){ (frame % sheet->columns) * sheet->frame_width,
                   (frame / sheet->columns) * sheet->frame_height,
                   sheet->frame_width, sheet->frame_height };
}

void draw_sprite_frame(const SpriteSheet *sheet, int frame, int x, int y, Flip flip)
{
    Rect source = sprite_frame_source(sheet, frame);
    if (source.width) draw_image_part(sheet->image, source, x, y, flip);
}

Animation *create_animation_frames(SpriteSheet *sheet, const int *frames,
                                   int frame_count, float frames_per_second,
                                   bool loop)
{
    Animation *animation;
    if (!sheet || !frames || frame_count <= 0 || frames_per_second < 0.0f) {
        r2d_set_error("invalid animation definition");
        return NULL;
    }
    for (int i = 0; i < frame_count; ++i) {
        if (frames[i] < 0 || frames[i] >= sprite_sheet_frame_count(sheet)) {
            r2d_set_error("animation frame %d is out of range", frames[i]);
            return NULL;
        }
    }
    if ((size_t)frame_count > SIZE_MAX / sizeof(int)) {
        r2d_set_error("animation frame list is too large");
        return NULL;
    }
    animation = (Animation *)calloc(1, sizeof *animation);
    if (!animation || !(animation->frames = (int *)malloc((size_t)frame_count * sizeof(int)))) {
        free(animation);
        r2d_set_error("not enough memory for an animation");
        return NULL;
    }
    memcpy(animation->frames, frames, (size_t)frame_count * sizeof(int));
    animation->sheet = sheet;
    animation->frame_count = frame_count;
    animation->rate = frames_per_second;
    animation->loop = loop;
    animation->playing = true;
    animation->next_resource = r2d.animations;
    r2d.animations = animation;
    return animation;
}

Animation *create_animation(SpriteSheet *sheet, int first_frame, int frame_count,
                            float frames_per_second, bool loop)
{
    Animation *result;
    int *frames;
    if (!sheet || first_frame < 0 || frame_count <= 0 ||
        first_frame > sprite_sheet_frame_count(sheet) - frame_count ||
        (size_t)frame_count > SIZE_MAX / sizeof(int)) {
        r2d_set_error("invalid animation frame range");
        return NULL;
    }
    frames = (int *)malloc((size_t)frame_count * sizeof(int));
    if (!frames) { r2d_set_error("not enough memory for animation frames"); return NULL; }
    for (int i = 0; i < frame_count; ++i) frames[i] = first_frame + i;
    result = create_animation_frames(sheet, frames, frame_count, frames_per_second, loop);
    free(frames);
    return result;
}

void free_animation(Animation *animation)
{
    Animation **link = &r2d.animations;
    if (!animation) return;
    while (*link && *link != animation) link = &(*link)->next_resource;
    if (*link) *link = animation->next_resource;
    free(animation->frames);
    free(animation);
}

void animation_update(Animation *animation, float delta)
{
    float frame_duration;
    if (!animation || !animation->playing || animation->finished ||
        animation->rate <= 0.0f || delta <= 0.0f) return;
    frame_duration = 1.0f / animation->rate;
    animation->accumulator += delta;
    while (animation->accumulator >= frame_duration) {
        animation->accumulator -= frame_duration;
        if (animation->current + 1 < animation->frame_count) ++animation->current;
        else if (animation->loop) animation->current = 0;
        else { animation->finished = true; animation->playing = false; break; }
    }
}

void animation_play(Animation *animation) { if (animation && !animation->finished) animation->playing = true; }
void animation_pause(Animation *animation) { if (animation) animation->playing = false; }
void animation_reset(Animation *animation) { if (animation) { animation->current = 0; animation->accumulator = 0; animation->finished = false; animation->playing = true; } }
void animation_set_loop(Animation *animation, bool loop) { if (animation) animation->loop = loop; }
void animation_set_speed(Animation *animation, float speed) { if (animation && speed >= 0.0f) animation->rate = speed; }
bool animation_finished(const Animation *animation) { return animation && animation->finished; }
int animation_current_frame(const Animation *animation) { return animation ? animation->frames[animation->current] : -1; }
void draw_animation(const Animation *animation, int x, int y, Flip flip) { if (animation) draw_sprite_frame(animation->sheet, animation_current_frame(animation), x, y, flip); }
