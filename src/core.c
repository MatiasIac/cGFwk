#include "retro2d_internal.h"

#include <string.h>

static GameConfig normalise_config(const GameConfig *provided)
{
    GameConfig config = { "Retro2D Game", 320, 200, 3, 60, true, false, 32 };
    if (!provided) return config;
    if (provided->title) config.title = provided->title;
    if (provided->screen_width > 0) config.screen_width = provided->screen_width;
    if (provided->screen_height > 0) config.screen_height = provided->screen_height;
    if (provided->window_scale > 0) config.window_scale = provided->window_scale;
    if (provided->update_rate > 0) config.update_rate = provided->update_rate;
    config.vertical_sync = provided->vertical_sync;
    config.fullscreen = provided->fullscreen;
    if (provided->audio_channels > 0) config.audio_channels = provided->audio_channels;
    return config;
}

int run_game(const GameConfig *provided)
{
    GameConfig config = normalise_config(provided);
    const double step = 1.0 / config.update_rate;
    uint64_t last_ticks, fps_ticks;
    double accumulator = 0.0;
    uint64_t fps_frames = 0;
    int result = 1;
    memset(&r2d, 0, sizeof r2d);
    r2d.alpha_threshold = 128;
    if (!r2d_screen_init(config.screen_width, config.screen_height)) return 1;
    r2d_input_reset();
    if (!r2d_platform_init(&config)) goto shutdown_screen;
    (void)r2d_audio_init(config.audio_channels);
    r2d.running = true;
    last_ticks = fps_ticks = r2d_platform_ticks_ns();
    game_start();
    while (r2d.running) {
        uint64_t loop_start = r2d_platform_ticks_ns();
        uint64_t now;
        double elapsed;
        int catch_up = 0;
        r2d_platform_poll_events();
        now = r2d_platform_ticks_ns();
        elapsed = (double)(now - last_ticks) / 1000000000.0;
        last_ticks = now;
        if (elapsed > 0.25) elapsed = 0.25;
        accumulator += elapsed;
        r2d.elapsed += elapsed;
        while (accumulator >= step && catch_up < 5 && r2d.running) {
            uint64_t update_start = r2d_platform_ticks_ns();
            r2d_input_begin_update();
            game_update((float)step);
            r2d.update_seconds = (float)((double)(r2d_platform_ticks_ns() - update_start) /
                                         1000000000.0);
            ++r2d.updates;
            accumulator -= step;
            ++catch_up;
        }
        if (catch_up == 5 && accumulator >= step) accumulator = 0.0;
        if (!r2d.running) break;
        game_draw();
        r2d_platform_present(r2d_apply_effects((float)r2d.elapsed), r2d.palette);
        ++r2d.frames;
        ++fps_frames;
        now = r2d_platform_ticks_ns();
        r2d.frame_seconds = (float)((double)(now - loop_start) / 1000000000.0);
        if (now - fps_ticks >= 1000000000ull) {
            r2d.fps = (float)((double)fps_frames * 1000000000.0 / (double)(now - fps_ticks));
            fps_frames = 0;
            fps_ticks = now;
        }
        if (!config.vertical_sync && r2d.frame_seconds < 1.0f / 240.0f)
            r2d_platform_delay_ns((uint64_t)((1.0 / 240.0 - r2d.frame_seconds) * 1000000000.0));
    }
    game_stop();
    r2d_audio_shutdown();
    r2d_resources_shutdown();
    clear_effects();
    r2d_platform_shutdown();
    result = 0;
shutdown_screen:
    r2d_screen_shutdown();
    return result;
}

void request_quit(void) { r2d.running = false; }
double elapsed_time(void) { return r2d.elapsed; }
uint64_t frame_count(void) { return r2d.frames; }
uint64_t update_count(void) { return r2d.updates; }
float frames_per_second(void) { return r2d.fps; }
float frame_time(void) { return r2d.frame_seconds; }
float update_time(void) { return r2d.update_seconds; }
