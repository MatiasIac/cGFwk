#include "retro2d_internal.h"

#include <stdio.h>
#include <string.h>

static int failures;

#define CHECK(condition) do { \
    if (!(condition)) { \
        fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #condition); \
        ++failures; \
    } \
} while (0)

static void test_drawing_and_clipping(void)
{
    Framebuffer framebuffer = get_framebuffer();
    clear_screen(0);
    put_pixel(0, 0, 1);
    put_pixel(319, 0, 2);
    put_pixel(0, 199, 3);
    put_pixel(319, 199, 4);
    put_pixel(-1, 0, 9);
    put_pixel(320, 199, 9);
    CHECK(framebuffer.pixels[0] == 1);
    CHECK(framebuffer.pixels[319] == 2);
    CHECK(framebuffer.pixels[199 * framebuffer.stride] == 3);
    CHECK(framebuffer.pixels[199 * framebuffer.stride + 319] == 4);
    draw_line(-20, 3, 340, 3, 5);
    for (int x = 0; x < 320; ++x) CHECK(framebuffer.pixels[3 * framebuffer.stride + x] == 5);
    CHECK(push_clip((Rect){ 2, 1, 3, 3 }));
    fill_rectangle((Rect){ 0, 0, 320, 200 }, 6);
    CHECK(framebuffer.pixels[1 * framebuffer.stride + 2] == 6);
    CHECK(framebuffer.pixels[3 * framebuffer.stride + 4] == 6);
    CHECK(framebuffer.pixels[1 * framebuffer.stride + 1] == 0);
    pop_clip();
    draw_rectangle((Rect){ 0, 0, 320, 200 }, 7);
    CHECK(get_pixel(0, 0) == 7 && get_pixel(319, 199) == 7);
}

static void test_palette(void)
{
    Color palette[RETRO2D_PALETTE_SIZE] = { 0 };
    Color faded[RETRO2D_PALETTE_SIZE];
    palette[1] = (Color){ 10, 20, 30, 255 };
    palette[2] = (Color){ 40, 50, 60, 255 };
    palette[3] = (Color){ 70, 80, 90, 255 };
    set_palette(palette);
    CHECK(get_palette_colour(2).g == 50);
    rotate_palette(1, 3, 1);
    CHECK(get_palette_colour(2).r == 10);
    CHECK(get_palette_colour(1).r == 70);
    get_palette(palette);
    make_faded_palette(palette, (Color){ 0, 0, 0, 255 }, 0.5f, faded);
    CHECK(faded[2].r == 5 && faded[2].g == 10 && faded[2].b == 15);
    set_palette_colour(255, (Color){ 1, 2, 3, 4 });
    CHECK(get_palette_colour(255).a == 4);
}

static void test_images_and_frames(void)
{
    const uint8_t pixels[] = { 1, 2, 3, 4, 5, 6, 7, 8 };
    const uint8_t mask[] = { 0x80, 0x40, 0xc0, 0x00 };
    Image *image = create_image(2, 4, pixels);
    SpriteSheet *sheet;
    Rect source;
    clear_screen(9);
    image_set_colour_key(image, 1);
    draw_image_part(image, (Rect){ 0, 0, 2, 1 }, 0, 0, FLIP_NONE);
    CHECK(get_pixel(0, 0) == 9);
    CHECK(get_pixel(1, 0) == 2);
    image_clear_colour_key(image);
    CHECK(image_set_mask(image, mask, 1));
    draw_image_part(image, (Rect){ 0, 0, 2, 1 }, 2, 0, FLIP_NONE);
    CHECK(get_pixel(2, 0) == 1);
    CHECK(get_pixel(3, 0) == 9);
    sheet = create_sprite_sheet(image, 1, 2);
    source = sprite_frame_source(sheet, 3);
    CHECK(source.x == 1 && source.y == 2 && source.width == 1 && source.height == 2);
    CHECK(image_masks_intersect(image, (Rect){ 0, 0, 1, 1 }, (Point){ 0, 0 },
                                image, (Rect){ 0, 0, 1, 1 }, (Point){ 0, 0 }));
    free_sprite_sheet(sheet);
    free_image(image);
}

static void test_input(void)
{
    Point mapped;
    r2d_input_reset();
    r2d_input_key(KEY_A, true);
    CHECK(key_down(KEY_A) && !key_pressed(KEY_A));
    r2d_input_begin_update();
    CHECK(key_pressed(KEY_A) && !key_released(KEY_A));
    r2d_input_begin_update();
    CHECK(!key_pressed(KEY_A));
    r2d_input_key(KEY_A, false);
    r2d_input_begin_update();
    CHECK(!key_down(KEY_A) && key_released(KEY_A));
    r2d_input_mouse_button(MOUSE_LEFT, true);
    r2d_input_mouse_motion(4.0f, 2.0f, 2.0f, -1.0f);
    r2d_input_mouse_wheel(1.0f);
    r2d_input_begin_update();
    CHECK(mouse_pressed(MOUSE_LEFT) && mouse_inside_screen());
    CHECK(mouse_position().x == 4 && mouse_movement().x == 2 && mouse_wheel() == 1);
    CHECK(!r2d_map_window_point(0.0f, 39.0f, 640, 480, &mapped));
    CHECK(r2d_map_window_point(0.0f, 40.0f, 640, 480, &mapped));
    CHECK(mapped.x == 0 && mapped.y == 0);
    CHECK(r2d_map_window_point(639.0f, 439.0f, 640, 480, &mapped));
    CHECK(mapped.x == 319 && mapped.y == 199);
    /* 1920 x 1080 uses a 5.4x fit: 1728 x 1080 with 96-pixel side borders. */
    CHECK(!r2d_map_window_point(95.0f, 540.0f, 1920, 1080, &mapped));
    CHECK(r2d_map_window_point(96.0f, 0.0f, 1920, 1080, &mapped));
    CHECK(mapped.x == 0 && mapped.y == 0);
    CHECK(r2d_map_window_point(1823.0f, 1079.0f, 1920, 1080, &mapped));
    CHECK(mapped.x == 319 && mapped.y == 199);
    CHECK(!r2d_map_window_point(1824.0f, 540.0f, 1920, 1080, &mapped));
}

static int effect_checks;

static void add_one(const uint8_t *source, uint8_t *destination,
                    int width, int height, int source_stride,
                    int destination_stride, const Color *palette,
                    float elapsed, void *user_data)
{
    int expected = *(int *)user_data;
    (void)width; (void)height; (void)source_stride; (void)destination_stride;
    (void)palette; (void)elapsed;
    CHECK(source != destination);
    CHECK(source[0] == expected);
    destination[0] = (uint8_t)(source[0] + 1);
    ++effect_checks;
}

static void test_effects_and_collision(void)
{
    int first = 1, second = 2;
    const uint8_t *result;
    clear_effects();
    clear_screen(1);
    CHECK(add_effect(add_one, &first) != 0);
    CHECK(add_effect(add_one, &second) != 0);
    result = r2d_apply_effects(0.0f);
    CHECK(effect_checks == 2 && result[0] == 3);
    clear_effects();
    CHECK(point_in_rectangle((Point){ 2, 2 }, (Rect){ 1, 1, 2, 2 }));
    CHECK(rectangles_intersect((Rect){ 0, 0, 2, 2 }, (Rect){ 1, 1, 2, 2 }));
    CHECK(!rectangles_intersect((Rect){ 0, 0, 1, 1 }, (Rect){ 1, 0, 1, 1 }));
    CHECK(circles_intersect((Circle){ 0, 0, 2 }, (Circle){ 3, 0, 1 }));
}

static void test_loading_failure(void)
{
    Image *missing = load_image("this/file/does/not/exist.bmp");
    CHECK(missing == NULL);
    CHECK(strstr(last_error(), "could not open image") != NULL);
}

int main(void)
{
    if (!r2d_screen_init(320, 200)) {
        fprintf(stderr, "screen setup failed: %s\n", last_error());
        return 1;
    }
    r2d.alpha_threshold = 128;
    test_drawing_and_clipping();
    test_palette();
    test_images_and_frames();
    test_input();
    test_effects_and_collision();
    test_loading_failure();
    r2d_resources_shutdown();
    r2d_screen_shutdown();
    if (failures) {
        fprintf(stderr, "%d test failure(s)\n", failures);
        return 1;
    }
    puts("All Retro2D tests passed.");
    return 0;
}
