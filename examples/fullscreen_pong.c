#include "retro2d.h"

#include <stdio.h>

enum {
    SCREEN_WIDTH = 640,
    SCREEN_HEIGHT = 480,
    PADDLE_WIDTH = 10,
    PADDLE_HEIGHT = 72,
    BALL_RADIUS = 6
};

static const float player_x = 28.0f;
static const float opponent_x = 602.0f;
static const float paddle_speed = 300.0f;
static const float opponent_speed = 245.0f;
static const float initial_ball_speed = 280.0f;

static BitmapFont *font;
static Sound *bounce_sound;
static float player_y;
static float opponent_y;
static float ball_x, ball_y;
static float ball_velocity_x, ball_velocity_y;
static int player_score, opponent_score;
static bool waiting_to_serve = true;
static bool paused;

static float clamp_float(float value, float minimum, float maximum)
{
    if (value < minimum) return minimum;
    if (value > maximum) return maximum;
    return value;
}

static void reset_ball(int direction)
{
    int variation = (player_score * 37 + opponent_score * 53) % 5;
    ball_x = SCREEN_WIDTH * 0.5f;
    ball_y = SCREEN_HEIGHT * 0.5f;
    ball_velocity_x = initial_ball_speed * (float)direction;
    ball_velocity_y = (float)(variation - 2) * 42.0f;
    waiting_to_serve = true;
}

static void bounce_from_paddle(float paddle_y, int direction)
{
    float paddle_centre = paddle_y + PADDLE_HEIGHT * 0.5f;
    float relative_hit = (ball_y - paddle_centre) / (PADDLE_HEIGHT * 0.5f);
    float horizontal_speed = ball_velocity_x < 0.0f
                           ? -ball_velocity_x : ball_velocity_x;
    horizontal_speed *= 1.04f;
    if (horizontal_speed > 520.0f) horizontal_speed = 520.0f;
    ball_velocity_x = horizontal_speed * (float)direction;
    ball_velocity_y += relative_hit * 150.0f;
    ball_velocity_y = clamp_float(ball_velocity_y, -360.0f, 360.0f);
    (void)play_sound(bounce_sound);
}

void game_start(void)
{
    set_palette_colour(0, (Color){ 2, 4, 12, 255 });
    set_palette_colour(1, (Color){ 8, 20, 42, 255 });
    set_palette_colour(2, (Color){ 40, 220, 255, 255 });
    set_palette_colour(3, (Color){ 255, 70, 150, 255 });
    set_palette_colour(4, (Color){ 255, 230, 80, 255 });
    set_palette_colour(5, (Color){ 220, 240, 255, 255 });

    font = load_bitmap_font("assets/font.bmp", 8, 8, 32, 96, 16);
    font_set_colour_key(font, 0);
    bounce_sound = load_sound("assets/laser.wav");

    player_y = (SCREEN_HEIGHT - PADDLE_HEIGHT) * 0.5f;
    opponent_y = player_y;
    reset_ball(1);
    show_cursor(false);
}

void game_update(float delta)
{
    float player_input = 0.0f;
    float controller_input = controller_axis(CONTROLLER_LEFT_Y);
    Rect player_paddle;
    Rect opponent_paddle;
    Rect ball_bounds;

    if (key_pressed(KEY_ESCAPE)) request_quit();
    if (key_pressed(KEY_P)) paused = !paused;

    if (key_down(KEY_W) || key_down(KEY_UP)) player_input -= 1.0f;
    if (key_down(KEY_S) || key_down(KEY_DOWN)) player_input += 1.0f;
    if (controller_input < -0.15f || controller_input > 0.15f)
        player_input = controller_input;

    player_y += player_input * paddle_speed * delta;
    player_y = clamp_float(player_y, 0.0f,
                           (float)(SCREEN_HEIGHT - PADDLE_HEIGHT));

    if (paused) return;
    if (waiting_to_serve) {
        if (key_pressed(KEY_SPACE) || controller_pressed(CONTROLLER_A))
            waiting_to_serve = false;
        else
            return;
    }

    {
        float target_y = ball_y - PADDLE_HEIGHT * 0.5f;
        float maximum_step = opponent_speed * delta;
        float difference = target_y - opponent_y;
        if (difference > maximum_step) difference = maximum_step;
        if (difference < -maximum_step) difference = -maximum_step;
        opponent_y += difference;
        opponent_y = clamp_float(opponent_y, 0.0f,
                                 (float)(SCREEN_HEIGHT - PADDLE_HEIGHT));
    }

    ball_x += ball_velocity_x * delta;
    ball_y += ball_velocity_y * delta;

    if (ball_y - BALL_RADIUS <= 0.0f && ball_velocity_y < 0.0f) {
        ball_y = (float)BALL_RADIUS;
        ball_velocity_y = -ball_velocity_y;
        (void)play_sound(bounce_sound);
    }
    if (ball_y + BALL_RADIUS >= SCREEN_HEIGHT && ball_velocity_y > 0.0f) {
        ball_y = (float)(SCREEN_HEIGHT - BALL_RADIUS);
        ball_velocity_y = -ball_velocity_y;
        (void)play_sound(bounce_sound);
    }

    player_paddle = (Rect){ (int)player_x, (int)player_y,
                            PADDLE_WIDTH, PADDLE_HEIGHT };
    opponent_paddle = (Rect){ (int)opponent_x, (int)opponent_y,
                              PADDLE_WIDTH, PADDLE_HEIGHT };
    ball_bounds = (Rect){ (int)ball_x - BALL_RADIUS,
                          (int)ball_y - BALL_RADIUS,
                          BALL_RADIUS * 2, BALL_RADIUS * 2 };

    if (ball_velocity_x < 0.0f && rectangles_intersect(ball_bounds, player_paddle)) {
        ball_x = player_x + PADDLE_WIDTH + BALL_RADIUS;
        bounce_from_paddle(player_y, 1);
    }
    if (ball_velocity_x > 0.0f && rectangles_intersect(ball_bounds, opponent_paddle)) {
        ball_x = opponent_x - BALL_RADIUS;
        bounce_from_paddle(opponent_y, -1);
    }

    if (ball_x < -(float)BALL_RADIUS) {
        ++opponent_score;
        reset_ball(1);
    } else if (ball_x > SCREEN_WIDTH + BALL_RADIUS) {
        ++player_score;
        reset_ball(-1);
    }
}

void game_draw(void)
{
    char score[32];
    clear_screen(0);

    for (int i = 0; i < 90; ++i) {
        int x = (i * 193 + 17) % SCREEN_WIDTH;
        int y = (i * 79 + 31) % SCREEN_HEIGHT;
        put_pixel(x, y, (uint8_t)(i % 7 == 0 ? 4 : 1));
    }

    for (int y = 12; y < SCREEN_HEIGHT; y += 24)
        fill_rectangle((Rect){ SCREEN_WIDTH / 2 - 1, y, 3, 12 }, 1);

    fill_rectangle((Rect){ (int)player_x, (int)player_y,
                           PADDLE_WIDTH, PADDLE_HEIGHT }, 2);
    fill_rectangle((Rect){ (int)opponent_x, (int)opponent_y,
                           PADDLE_WIDTH, PADDLE_HEIGHT }, 3);
    fill_circle((Circle){ (int)ball_x, (int)ball_y, BALL_RADIUS }, 4);
    draw_circle((Circle){ (int)ball_x, (int)ball_y, BALL_RADIUS + 2 }, 5);

    if (font) {
        (void)snprintf(score, sizeof score, "%02d       %02d",
                       player_score % 100, opponent_score % 100);
        draw_text(font, score, SCREEN_WIDTH / 2, 24, ALIGN_CENTRE);
        draw_text(font, "NEON FULLSCREEN PONG", SCREEN_WIDTH / 2, 6, ALIGN_CENTRE);
        draw_text(font, "W S OR ARROWS MOVE   P PAUSES   ESC QUITS",
                  SCREEN_WIDTH / 2, SCREEN_HEIGHT - 16, ALIGN_CENTRE);
        if (waiting_to_serve)
            draw_text(font, "PRESS SPACE OR CONTROLLER A TO SERVE",
                      SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2 + 34, ALIGN_CENTRE);
        if (paused)
            draw_text(font, "PAUSED", SCREEN_WIDTH / 2,
                      SCREEN_HEIGHT / 2 - 4, ALIGN_CENTRE);
    }
}

void game_stop(void)
{
    show_cursor(true);
    free_sound(bounce_sound);
    free_bitmap_font(font);
}

int main(void)
{
    GameConfig config = {
        .title = "Retro2D Fullscreen Pong",
        .screen_width = SCREEN_WIDTH,
        .screen_height = SCREEN_HEIGHT,
        .window_scale = 1,
        .update_rate = 60,
        .vertical_sync = true,
        .fullscreen = true,
        .audio_channels = 16
    };
    return run_game(&config);
}
