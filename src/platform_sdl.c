#include "retro2d_internal.h"

#include <SDL3/SDL.h>

#include <stdlib.h>

static SDL_Window *window;
static SDL_Renderer *renderer;
static SDL_Texture *texture;
static SDL_AudioStream *audio_stream;
static SDL_Gamepad *gamepad;
static uint8_t *rgba_pixels;
static int window_width, window_height;

static Key translate_key(SDL_Scancode key)
{
    switch (key) {
        case SDL_SCANCODE_A: return KEY_A; case SDL_SCANCODE_B: return KEY_B;
        case SDL_SCANCODE_C: return KEY_C; case SDL_SCANCODE_D: return KEY_D;
        case SDL_SCANCODE_E: return KEY_E; case SDL_SCANCODE_F: return KEY_F;
        case SDL_SCANCODE_G: return KEY_G; case SDL_SCANCODE_H: return KEY_H;
        case SDL_SCANCODE_I: return KEY_I; case SDL_SCANCODE_J: return KEY_J;
        case SDL_SCANCODE_K: return KEY_K; case SDL_SCANCODE_L: return KEY_L;
        case SDL_SCANCODE_M: return KEY_M; case SDL_SCANCODE_N: return KEY_N;
        case SDL_SCANCODE_O: return KEY_O; case SDL_SCANCODE_P: return KEY_P;
        case SDL_SCANCODE_Q: return KEY_Q; case SDL_SCANCODE_R: return KEY_R;
        case SDL_SCANCODE_S: return KEY_S; case SDL_SCANCODE_T: return KEY_T;
        case SDL_SCANCODE_U: return KEY_U; case SDL_SCANCODE_V: return KEY_V;
        case SDL_SCANCODE_W: return KEY_W; case SDL_SCANCODE_X: return KEY_X;
        case SDL_SCANCODE_Y: return KEY_Y; case SDL_SCANCODE_Z: return KEY_Z;
        case SDL_SCANCODE_0: return KEY_0; case SDL_SCANCODE_1: return KEY_1;
        case SDL_SCANCODE_2: return KEY_2; case SDL_SCANCODE_3: return KEY_3;
        case SDL_SCANCODE_4: return KEY_4; case SDL_SCANCODE_5: return KEY_5;
        case SDL_SCANCODE_6: return KEY_6; case SDL_SCANCODE_7: return KEY_7;
        case SDL_SCANCODE_8: return KEY_8; case SDL_SCANCODE_9: return KEY_9;
        case SDL_SCANCODE_ESCAPE: return KEY_ESCAPE;
        case SDL_SCANCODE_RETURN: return KEY_ENTER;
        case SDL_SCANCODE_SPACE: return KEY_SPACE;
        case SDL_SCANCODE_TAB: return KEY_TAB;
        case SDL_SCANCODE_BACKSPACE: return KEY_BACKSPACE;
        case SDL_SCANCODE_LEFT: return KEY_LEFT; case SDL_SCANCODE_RIGHT: return KEY_RIGHT;
        case SDL_SCANCODE_UP: return KEY_UP; case SDL_SCANCODE_DOWN: return KEY_DOWN;
        case SDL_SCANCODE_LSHIFT: return KEY_LSHIFT; case SDL_SCANCODE_RSHIFT: return KEY_RSHIFT;
        case SDL_SCANCODE_LCTRL: return KEY_LCTRL; case SDL_SCANCODE_RCTRL: return KEY_RCTRL;
        case SDL_SCANCODE_LALT: return KEY_LALT; case SDL_SCANCODE_RALT: return KEY_RALT;
        case SDL_SCANCODE_F1: return KEY_F1; case SDL_SCANCODE_F2: return KEY_F2;
        case SDL_SCANCODE_F3: return KEY_F3; case SDL_SCANCODE_F4: return KEY_F4;
        case SDL_SCANCODE_F5: return KEY_F5; case SDL_SCANCODE_F6: return KEY_F6;
        case SDL_SCANCODE_F7: return KEY_F7; case SDL_SCANCODE_F8: return KEY_F8;
        case SDL_SCANCODE_F9: return KEY_F9; case SDL_SCANCODE_F10: return KEY_F10;
        case SDL_SCANCODE_F11: return KEY_F11; case SDL_SCANCODE_F12: return KEY_F12;
        case SDL_SCANCODE_HOME: return KEY_HOME; case SDL_SCANCODE_END: return KEY_END;
        case SDL_SCANCODE_PAGEUP: return KEY_PAGE_UP; case SDL_SCANCODE_PAGEDOWN: return KEY_PAGE_DOWN;
        case SDL_SCANCODE_INSERT: return KEY_INSERT; case SDL_SCANCODE_DELETE: return KEY_DELETE;
        case SDL_SCANCODE_MINUS: return KEY_MINUS; case SDL_SCANCODE_EQUALS: return KEY_EQUALS;
        case SDL_SCANCODE_LEFTBRACKET: return KEY_LEFT_BRACKET;
        case SDL_SCANCODE_RIGHTBRACKET: return KEY_RIGHT_BRACKET;
        case SDL_SCANCODE_SEMICOLON: return KEY_SEMICOLON;
        case SDL_SCANCODE_APOSTROPHE: return KEY_APOSTROPHE;
        case SDL_SCANCODE_COMMA: return KEY_COMMA; case SDL_SCANCODE_PERIOD: return KEY_PERIOD;
        case SDL_SCANCODE_SLASH: return KEY_SLASH; case SDL_SCANCODE_BACKSLASH: return KEY_BACKSLASH;
        case SDL_SCANCODE_GRAVE: return KEY_GRAVE;
        default: return KEY_UNKNOWN;
    }
}

static MouseButton translate_mouse(uint8_t button)
{
    switch (button) {
        case SDL_BUTTON_LEFT: return MOUSE_LEFT;
        case SDL_BUTTON_MIDDLE: return MOUSE_MIDDLE;
        case SDL_BUTTON_RIGHT: return MOUSE_RIGHT;
        case SDL_BUTTON_X1: return MOUSE_X1;
        case SDL_BUTTON_X2: return MOUSE_X2;
        default: return MOUSE_BUTTON_COUNT;
    }
}

static ControllerButton translate_gamepad_button(uint8_t button)
{
    switch ((SDL_GamepadButton)button) {
        case SDL_GAMEPAD_BUTTON_SOUTH: return CONTROLLER_A;
        case SDL_GAMEPAD_BUTTON_EAST: return CONTROLLER_B;
        case SDL_GAMEPAD_BUTTON_WEST: return CONTROLLER_X;
        case SDL_GAMEPAD_BUTTON_NORTH: return CONTROLLER_Y;
        case SDL_GAMEPAD_BUTTON_BACK: return CONTROLLER_BACK;
        case SDL_GAMEPAD_BUTTON_GUIDE: return CONTROLLER_GUIDE;
        case SDL_GAMEPAD_BUTTON_START: return CONTROLLER_START;
        case SDL_GAMEPAD_BUTTON_LEFT_STICK: return CONTROLLER_LEFT_STICK;
        case SDL_GAMEPAD_BUTTON_RIGHT_STICK: return CONTROLLER_RIGHT_STICK;
        case SDL_GAMEPAD_BUTTON_LEFT_SHOULDER: return CONTROLLER_LEFT_SHOULDER;
        case SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER: return CONTROLLER_RIGHT_SHOULDER;
        case SDL_GAMEPAD_BUTTON_DPAD_UP: return CONTROLLER_DPAD_UP;
        case SDL_GAMEPAD_BUTTON_DPAD_DOWN: return CONTROLLER_DPAD_DOWN;
        case SDL_GAMEPAD_BUTTON_DPAD_LEFT: return CONTROLLER_DPAD_LEFT;
        case SDL_GAMEPAD_BUTTON_DPAD_RIGHT: return CONTROLLER_DPAD_RIGHT;
        default: return CONTROLLER_BUTTON_COUNT;
    }
}

static ControllerAxis translate_gamepad_axis(uint8_t axis)
{
    switch ((SDL_GamepadAxis)axis) {
        case SDL_GAMEPAD_AXIS_LEFTX: return CONTROLLER_LEFT_X;
        case SDL_GAMEPAD_AXIS_LEFTY: return CONTROLLER_LEFT_Y;
        case SDL_GAMEPAD_AXIS_RIGHTX: return CONTROLLER_RIGHT_X;
        case SDL_GAMEPAD_AXIS_RIGHTY: return CONTROLLER_RIGHT_Y;
        case SDL_GAMEPAD_AXIS_LEFT_TRIGGER: return CONTROLLER_LEFT_TRIGGER;
        case SDL_GAMEPAD_AXIS_RIGHT_TRIGGER: return CONTROLLER_RIGHT_TRIGGER;
        default: return CONTROLLER_AXIS_COUNT;
    }
}

static void SDLCALL fill_audio(void *user_data, SDL_AudioStream *stream,
                               int additional_amount, int total_amount)
{
    int16_t buffer[2048];
    (void)user_data;
    (void)total_amount;
    while (additional_amount > 0) {
        int frames = (additional_amount + 3) / 4;
        int bytes;
        if (frames > 1024) frames = 1024;
        bytes = frames * 4;
        r2d_audio_mix(buffer, frames);
        if (!SDL_PutAudioStreamData(stream, buffer, bytes)) return;
        additional_amount -= bytes;
    }
}

bool r2d_platform_init(const GameConfig *config)
{
    size_t rgba_size = (size_t)config->screen_width * (size_t)config->screen_height * 4u;
    SDL_WindowFlags flags = SDL_WINDOW_RESIZABLE;
    SDL_AudioSpec audio_spec;
    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMEPAD)) {
        r2d_set_error("SDL could not start video: %s", SDL_GetError());
        return false;
    }
    if (config->fullscreen) flags |= SDL_WINDOW_FULLSCREEN;
    window_width = config->screen_width * config->window_scale;
    window_height = config->screen_height * config->window_scale;
    if (!SDL_CreateWindowAndRenderer(config->title, window_width, window_height,
                                     flags, &window, &renderer)) {
        r2d_set_error("SDL could not create the window: %s", SDL_GetError());
        r2d_platform_shutdown();
        return false;
    }
    texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA32,
                                SDL_TEXTUREACCESS_STREAMING,
                                config->screen_width, config->screen_height);
    rgba_pixels = (uint8_t *)malloc(rgba_size);
    if (!texture || !rgba_pixels || !SDL_SetTextureScaleMode(texture, SDL_SCALEMODE_NEAREST) ||
        !SDL_SetRenderVSync(renderer, config->vertical_sync ? 1 : 0)) {
        r2d_set_error("SDL could not prepare indexed-screen presentation: %s", SDL_GetError());
        r2d_platform_shutdown();
        return false;
    }
    (void)SDL_GetWindowSizeInPixels(window, &window_width, &window_height);
    r2d.audio_ready = false;
    if (SDL_InitSubSystem(SDL_INIT_AUDIO)) {
        audio_spec.format = SDL_AUDIO_S16;
        audio_spec.channels = 2;
        audio_spec.freq = R2D_AUDIO_RATE;
        audio_stream = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK,
                                                 &audio_spec, fill_audio, NULL);
        if (audio_stream && SDL_ResumeAudioStreamDevice(audio_stream)) r2d.audio_ready = true;
        else log_message(LOG_WARNING, "SDL audio is unavailable: %s", SDL_GetError());
    } else log_message(LOG_WARNING, "SDL audio initialisation failed: %s", SDL_GetError());
    return true;
}

void r2d_platform_shutdown(void)
{
    if (gamepad) SDL_CloseGamepad(gamepad);
    gamepad = NULL;
    if (audio_stream) SDL_DestroyAudioStream(audio_stream);
    audio_stream = NULL;
    if (texture) SDL_DestroyTexture(texture);
    texture = NULL;
    if (renderer) SDL_DestroyRenderer(renderer);
    renderer = NULL;
    if (window) SDL_DestroyWindow(window);
    window = NULL;
    free(rgba_pixels);
    rgba_pixels = NULL;
    SDL_Quit();
}

static void update_mouse_motion(float x, float y, float x_relative, float y_relative)
{
    Point current, previous;
    bool current_inside = r2d_map_window_point(x, y, window_width, window_height, &current);
    bool previous_inside = r2d_map_window_point(x - x_relative, y - y_relative,
                                                window_width, window_height, &previous);
    r2d_input_mouse_motion(current_inside ? (float)current.x : -1.0f,
                           current_inside ? (float)current.y : -1.0f,
                           current_inside && previous_inside ? (float)(current.x - previous.x) : 0.0f,
                           current_inside && previous_inside ? (float)(current.y - previous.y) : 0.0f);
}

void r2d_platform_poll_events(void)
{
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        switch (event.type) {
            case SDL_EVENT_QUIT:
            case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
                request_quit();
                break;
            case SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED:
                window_width = event.window.data1;
                window_height = event.window.data2;
                break;
            case SDL_EVENT_WINDOW_MOUSE_LEAVE:
                r2d_input_mouse_motion(-1.0f, -1.0f, 0.0f, 0.0f);
                break;
            case SDL_EVENT_KEY_DOWN:
            case SDL_EVENT_KEY_UP:
                r2d_input_key(translate_key(event.key.scancode), event.type == SDL_EVENT_KEY_DOWN);
                break;
            case SDL_EVENT_MOUSE_MOTION:
                update_mouse_motion(event.motion.x, event.motion.y,
                                    event.motion.xrel, event.motion.yrel);
                break;
            case SDL_EVENT_MOUSE_BUTTON_DOWN:
            case SDL_EVENT_MOUSE_BUTTON_UP:
                r2d_input_mouse_button(translate_mouse(event.button.button),
                                       event.type == SDL_EVENT_MOUSE_BUTTON_DOWN);
                break;
            case SDL_EVENT_MOUSE_WHEEL:
                r2d_input_mouse_wheel(event.wheel.y);
                break;
            case SDL_EVENT_GAMEPAD_ADDED:
                if (!gamepad) {
                    gamepad = SDL_OpenGamepad(event.gdevice.which);
                    r2d_input_controller_connected(gamepad != NULL);
                }
                break;
            case SDL_EVENT_GAMEPAD_REMOVED:
                if (gamepad && SDL_GetGamepadID(gamepad) == event.gdevice.which) {
                    SDL_CloseGamepad(gamepad);
                    gamepad = NULL;
                    r2d_input_controller_connected(false);
                }
                break;
            case SDL_EVENT_GAMEPAD_BUTTON_DOWN:
            case SDL_EVENT_GAMEPAD_BUTTON_UP:
                if (gamepad && SDL_GetGamepadID(gamepad) == event.gbutton.which)
                    r2d_input_controller_button(translate_gamepad_button(event.gbutton.button),
                                                event.type == SDL_EVENT_GAMEPAD_BUTTON_DOWN);
                break;
            case SDL_EVENT_GAMEPAD_AXIS_MOTION:
                if (gamepad && SDL_GetGamepadID(gamepad) == event.gaxis.which) {
                    ControllerAxis axis = translate_gamepad_axis(event.gaxis.axis);
                    float value = event.gaxis.value < 0 ? event.gaxis.value / 32768.0f :
                                                         event.gaxis.value / 32767.0f;
                    r2d_input_controller_axis(axis, value);
                }
                break;
            default:
                break;
        }
    }
}

void r2d_platform_present(const uint8_t *pixels, const Color *palette)
{
    float scale_x, scale_y, scale;
    SDL_FRect destination;
    size_t pixel_count = (size_t)r2d.screen.width * (size_t)r2d.screen.height;
    if (!pixels || !rgba_pixels) return;
    for (size_t i = 0; i < pixel_count; ++i) {
        Color colour = palette[pixels[i]];
        rgba_pixels[i * 4u] = colour.r;
        rgba_pixels[i * 4u + 1u] = colour.g;
        rgba_pixels[i * 4u + 2u] = colour.b;
        rgba_pixels[i * 4u + 3u] = colour.a;
    }
    if (!SDL_UpdateTexture(texture, NULL, rgba_pixels, r2d.screen.width * 4)) {
        log_message(LOG_WARNING, "could not update the screen texture: %s", SDL_GetError());
        return;
    }
    scale_x = (float)window_width / (float)r2d.screen.width;
    scale_y = (float)window_height / (float)r2d.screen.height;
    scale = scale_x < scale_y ? scale_x : scale_y;
    destination.w = (float)r2d.screen.width * scale;
    destination.h = (float)r2d.screen.height * scale;
    destination.x = ((float)window_width - destination.w) * 0.5f;
    destination.y = ((float)window_height - destination.h) * 0.5f;
    (void)SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    (void)SDL_RenderClear(renderer);
    (void)SDL_RenderTexture(renderer, texture, NULL, &destination);
    SDL_RenderPresent(renderer);
}

uint64_t r2d_platform_ticks_ns(void) { return SDL_GetTicksNS(); }
void r2d_platform_delay_ns(uint64_t nanoseconds) { SDL_DelayNS(nanoseconds); }
void r2d_platform_show_cursor(bool visible) { if (visible) (void)SDL_ShowCursor(); else (void)SDL_HideCursor(); }
void r2d_platform_confine_cursor(bool confined) { if (window) (void)SDL_SetWindowMouseGrab(window, confined); }
void r2d_platform_audio_lock(void) { if (audio_stream) (void)SDL_LockAudioStream(audio_stream); }
void r2d_platform_audio_unlock(void) { if (audio_stream) SDL_UnlockAudioStream(audio_stream); }
