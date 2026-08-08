#include "retro2d.h"

typedef struct Shot { int x, y; bool active; } Shot;

static Image *ship_image;
static SpriteSheet *ship_sheet;
static Animation *ship_animation;
static BitmapFont *font;
static Sound *laser;
static Shot shots[12];
static int ship_x = 152, ship_y = 170;
static Rect target = { 140, 25, 40, 16 };
static int score;

void game_start(void)
{
    ship_image = load_image("assets/sprites.bmp");
    if (ship_image) {
        image_set_colour_key(ship_image, 0);
        ship_sheet = create_sprite_sheet(ship_image, 16, 16);
        ship_animation = create_animation(ship_sheet, 0, 4, 10.0f, true);
    }
    font = load_bitmap_font("assets/font.bmp", 8, 8, 32, 96, 16);
    font_set_colour_key(font, 0);
    laser = load_sound("assets/laser.wav");
}

void game_update(float delta)
{
    int speed = (int)(110.0f * delta + 0.5f);
    if (key_down(KEY_LEFT)) ship_x -= speed;
    if (key_down(KEY_RIGHT)) ship_x += speed;
    if (key_down(KEY_UP)) ship_y -= speed;
    if (key_down(KEY_DOWN)) ship_y += speed;
    if (ship_x < 0) ship_x = 0;
    if (ship_x > 304) ship_x = 304;
    if (ship_y < 80) ship_y = 80;
    if (ship_y > 184) ship_y = 184;
    if (key_pressed(KEY_SPACE)) {
        for (int i = 0; i < 12; ++i) if (!shots[i].active) {
            shots[i] = (Shot){ ship_x + 7, ship_y - 2, true };
            (void)play_sound(laser);
            break;
        }
    }
    for (int i = 0; i < 12; ++i) if (shots[i].active) {
        shots[i].y -= 3;
        if (rectangles_intersect((Rect){ shots[i].x, shots[i].y, 2, 5 }, target)) {
            shots[i].active = false;
            ++score;
            target.x = 20 + (score * 47) % 260;
        } else if (shots[i].y < 0) shots[i].active = false;
    }
    animation_update(ship_animation, delta);
    if (key_pressed(KEY_ESCAPE)) request_quit();
}

void game_draw(void)
{
    clear_screen(1);
    for (int i = 0; i < 45; ++i) put_pixel((i * 73) % 320, (i * 31) % 200, 215);
    fill_rectangle(target, 180);
    draw_rectangle((Rect){ target.x - 2, target.y - 2, target.width + 4, target.height + 4 }, 210);
    for (int i = 0; i < 12; ++i) if (shots[i].active)
        fill_rectangle((Rect){ shots[i].x, shots[i].y, 2, 5 }, 210);
    if (ship_animation) draw_animation(ship_animation, ship_x, ship_y, FLIP_NONE);
    else fill_rectangle((Rect){ ship_x, ship_y, 16, 16 }, 35);
    if (font) {
        char score_text[] = "SCORE 000";
        score_text[7] = (char)('0' + (score / 10) % 10);
        score_text[8] = (char)('0' + score % 10);
        draw_text(font, "ARROWS MOVE  SPACE FIRES", 160, 4, ALIGN_CENTRE);
        draw_text(font, score_text, 4, 15, ALIGN_LEFT);
    }
}

void game_stop(void)
{
    free_animation(ship_animation);
    free_sprite_sheet(ship_sheet);
    free_image(ship_image);
    free_bitmap_font(font);
    free_sound(laser);
}

int main(void)
{
    GameConfig config = { "Retro2D Sprite Demo", 320, 200, 3, 60, true, false, 32 };
    return run_game(&config);
}
