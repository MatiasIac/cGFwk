#include "retro2d.h"

static Image *room;
static BitmapFont *font;
static Music *music;
static int scene;
static bool music_paused;

void game_start(void)
{
    room = load_image("assets/room.bmp");
    font = load_bitmap_font("assets/font.bmp", 8, 8, 32, 96, 16);
    font_set_colour_key(font, 0);
    music = load_music("assets/music.wav");
    play_music(music, true);
}

void game_update(float delta)
{
    Point mouse = mouse_position();
    (void)delta;
    if (mouse_pressed(MOUSE_LEFT) && mouse_inside_screen()) {
        if (point_in_rectangle(mouse, (Rect){ 205, 18, 80, 92 })) scene = 1;
        else if (point_in_rectangle(mouse, (Rect){ 30, 25, 90, 65 })) scene = 2;
        else scene = 0;
    }
    if (key_pressed(KEY_M)) {
        music_paused = !music_paused;
        if (music_paused) pause_music(); else resume_music();
    }
    if (key_pressed(KEY_ESCAPE)) request_quit();
}

void game_draw(void)
{
    Point mouse = mouse_position();
    if (room) draw_image(room, 0, 0); else clear_screen(97);
    if (mouse_inside_screen()) draw_rectangle((Rect){ mouse.x - 3, mouse.y - 3, 7, 7 }, 215);
    fill_rectangle((Rect){ 0, 174, 320, 26 }, 0);
    if (font) {
        const char *message = scene == 1 ? "THE DOOR LEADS OUTSIDE" :
                              scene == 2 ? "A QUIET BLUE WINDOW" :
                                           "CLICK THE WINDOW OR DOOR";
        draw_text(font, message, 160, 180, ALIGN_CENTRE);
        draw_text(font, "M PAUSES MUSIC", 316, 4, ALIGN_RIGHT);
    }
}

void game_stop(void)
{
    free_bitmap_font(font);
    free_image(room);
    free_music(music);
}

int main(void)
{
    GameConfig config = { "Retro2D Point and Click", 320, 200, 3, 60, true, false, 32 };
    return run_game(&config);
}
