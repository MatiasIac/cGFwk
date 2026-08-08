#include "retro2d_internal.h"

void r2d_resources_shutdown(void)
{
    while (r2d.animations) free_animation(r2d.animations);
    while (r2d.sheets) free_sprite_sheet(r2d.sheets);
    while (r2d.fonts) free_bitmap_font(r2d.fonts);
    while (r2d.images) free_image(r2d.images);
    while (r2d.sounds) free_sound(r2d.sounds);
    while (r2d.music_resources) free_music(r2d.music_resources);
}
