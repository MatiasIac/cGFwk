#include "retro2d.h"

#include <string.h>

static Image *scene;
static BitmapFont *font;
static Color original_palette[RETRO2D_PALETTE_SIZE];
static float fade_amount;
static float cycle_timer;
static bool water_enabled = true;
static EffectHandle water_handle;

static void water_effect(const uint8_t *source, uint8_t *destination,
                         int width, int height, int source_stride,
                         int destination_stride, const Color *palette,
                         float elapsed, void *user_data)
{
    int phase = (int)(elapsed * 20.0f);
    (void)palette; (void)user_data;
    for (int y = 94; y < height; ++y) {
        int wave = (y + phase) & 15;
        int offset = wave < 8 ? wave - 4 : 12 - wave;
        for (int x = 0; x < width; ++x) {
            int sample_x = x + offset;
            if (sample_x < 0) sample_x = 0;
            if (sample_x >= width) sample_x = width - 1;
            destination[y * destination_stride + x] = source[y * source_stride + sample_x];
        }
    }
}

void game_start(void)
{
    scene = load_image("assets/water.bmp");
    font = load_bitmap_font("assets/font.bmp", 8, 8, 32, 96, 16);
    font_set_colour_key(font, 0);
    get_palette(original_palette);
    water_handle = add_effect(water_effect, NULL);
}

void game_update(float delta)
{
    Color faded[RETRO2D_PALETTE_SIZE];
    cycle_timer += delta;
    if (cycle_timer >= 0.12f) {
        set_palette(original_palette);
        rotate_palette(18, 21, 1);
        cycle_timer = 0.0f;
        get_palette(original_palette);
    }
    if (key_down(KEY_DOWN)) fade_amount += delta;
    if (key_down(KEY_UP)) fade_amount -= delta;
    if (fade_amount < 0.0f) fade_amount = 0.0f;
    if (fade_amount > 1.0f) fade_amount = 1.0f;
    make_faded_palette(original_palette, (Color){ 0, 0, 0, 255 }, fade_amount, faded);
    set_palette(faded);
    if (key_pressed(KEY_SPACE)) {
        water_enabled = !water_enabled;
        enable_effect(water_handle, water_enabled);
    }
    if (key_pressed(KEY_ESCAPE)) request_quit();
}

void game_draw(void)
{
    if (scene) draw_image(scene, 0, 0); else clear_screen(18);
    fill_rectangle((Rect){ 0, 0, 320, 13 }, 0);
    if (font) draw_text(font, "UP DOWN FADE  SPACE WATER", 160, 3, ALIGN_CENTRE);
}

void game_stop(void)
{
    remove_effect(water_handle);
    free_bitmap_font(font);
    free_image(scene);
}

int main(void)
{
    GameConfig config = { "Retro2D Effects", 320, 200, 3, 60, true, false, 32 };
    return run_game(&config);
}
