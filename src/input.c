#include "retro2d_internal.h"

#include <math.h>
#include <string.h>

static void change_button(InputButtonState *state, bool down)
{
    if (down && !state->down) state->pending_pressed = true;
    if (!down && state->down) state->pending_released = true;
    state->down = down;
}

void r2d_input_reset(void)
{
    memset(r2d.keys, 0, sizeof r2d.keys);
    memset(r2d.mouse_buttons, 0, sizeof r2d.mouse_buttons);
    memset(r2d.controller_buttons, 0, sizeof r2d.controller_buttons);
    memset(r2d.controller_axes, 0, sizeof r2d.controller_axes);
    r2d.mouse = (Point){ -1, -1 };
    r2d.mouse_delta = (Point){ 0, 0 };
    r2d.pending_mouse_delta = (Point){ 0, 0 };
    r2d.mouse_wheel = 0;
    r2d.pending_mouse_wheel = 0;
    r2d.mouse_inside = false;
    r2d.controller_present = false;
}

static void begin_buttons(InputButtonState *states, int count)
{
    int i;
    for (i = 0; i < count; ++i) {
        states[i].pressed = states[i].pending_pressed;
        states[i].released = states[i].pending_released;
        states[i].pending_pressed = false;
        states[i].pending_released = false;
    }
}

void r2d_input_begin_update(void)
{
    begin_buttons(r2d.keys, KEY_COUNT);
    begin_buttons(r2d.mouse_buttons, MOUSE_BUTTON_COUNT);
    begin_buttons(r2d.controller_buttons, CONTROLLER_BUTTON_COUNT);
    r2d.mouse_delta = r2d.pending_mouse_delta;
    r2d.pending_mouse_delta = (Point){ 0, 0 };
    r2d.mouse_wheel = r2d.pending_mouse_wheel;
    r2d.pending_mouse_wheel = 0;
}

void r2d_input_key(Key key, bool down)
{
    if (key > KEY_UNKNOWN && key < KEY_COUNT) change_button(&r2d.keys[key], down);
}

void r2d_input_mouse_button(MouseButton button, bool down)
{
    if (button >= 0 && button < MOUSE_BUTTON_COUNT)
        change_button(&r2d.mouse_buttons[button], down);
}

void r2d_input_mouse_motion(float x, float y, float dx, float dy)
{
    r2d.mouse.x = (int)floorf(x);
    r2d.mouse.y = (int)floorf(y);
    r2d.pending_mouse_delta.x += (int)lroundf(dx);
    r2d.pending_mouse_delta.y += (int)lroundf(dy);
    r2d.mouse_inside = x >= 0.0f && y >= 0.0f &&
                       x < (float)r2d.screen.width && y < (float)r2d.screen.height;
    if (!r2d.mouse_inside) r2d.mouse = (Point){ -1, -1 };
}

void r2d_input_mouse_wheel(float amount)
{
    r2d.pending_mouse_wheel += (int)lroundf(amount);
}

bool r2d_map_window_point(float x, float y, int window_width, int window_height,
                          Point *result)
{
    float scale, scale_x, scale_y, left, top;
    if (!result || window_width <= 0 || window_height <= 0 ||
        r2d.screen.width <= 0 || r2d.screen.height <= 0) return false;
    scale_x = (float)window_width / (float)r2d.screen.width;
    scale_y = (float)window_height / (float)r2d.screen.height;
    scale = scale_x < scale_y ? scale_x : scale_y;
    left = ((float)window_width - (float)r2d.screen.width * scale) * 0.5f;
    top = ((float)window_height - (float)r2d.screen.height * scale) * 0.5f;
    if (x < left || y < top || x >= left + (float)r2d.screen.width * scale ||
        y >= top + (float)r2d.screen.height * scale) {
        *result = (Point){ -1, -1 };
        return false;
    }
    result->x = (int)((x - left) / scale);
    result->y = (int)((y - top) / scale);
    return true;
}

void r2d_input_controller_button(ControllerButton button, bool down)
{
    if (button >= 0 && button < CONTROLLER_BUTTON_COUNT)
        change_button(&r2d.controller_buttons[button], down);
}

void r2d_input_controller_axis(ControllerAxis axis, float value)
{
    if (axis >= 0 && axis < CONTROLLER_AXIS_COUNT)
        r2d.controller_axes[axis] = value < -1.0f ? -1.0f : (value > 1.0f ? 1.0f : value);
}

void r2d_input_controller_connected(bool connected)
{
    r2d.controller_present = connected;
    if (!connected) {
        memset(r2d.controller_buttons, 0, sizeof r2d.controller_buttons);
        memset(r2d.controller_axes, 0, sizeof r2d.controller_axes);
    }
}

bool key_down(Key key) { return key > KEY_UNKNOWN && key < KEY_COUNT && r2d.keys[key].down; }
bool key_pressed(Key key) { return key > KEY_UNKNOWN && key < KEY_COUNT && r2d.keys[key].pressed; }
bool key_released(Key key) { return key > KEY_UNKNOWN && key < KEY_COUNT && r2d.keys[key].released; }
Point mouse_position(void) { return r2d.mouse; }
Point mouse_movement(void) { return r2d.mouse_delta; }
bool mouse_inside_screen(void) { return r2d.mouse_inside; }
bool mouse_down(MouseButton button) { return button >= 0 && button < MOUSE_BUTTON_COUNT && r2d.mouse_buttons[button].down; }
bool mouse_pressed(MouseButton button) { return button >= 0 && button < MOUSE_BUTTON_COUNT && r2d.mouse_buttons[button].pressed; }
bool mouse_released(MouseButton button) { return button >= 0 && button < MOUSE_BUTTON_COUNT && r2d.mouse_buttons[button].released; }
int mouse_wheel(void) { return r2d.mouse_wheel; }
void show_cursor(bool visible) { r2d_platform_show_cursor(visible); }
void confine_cursor(bool confined) { r2d_platform_confine_cursor(confined); }
bool controller_connected(void) { return r2d.controller_present; }
bool controller_down(ControllerButton button) { return button >= 0 && button < CONTROLLER_BUTTON_COUNT && r2d.controller_buttons[button].down; }
bool controller_pressed(ControllerButton button) { return button >= 0 && button < CONTROLLER_BUTTON_COUNT && r2d.controller_buttons[button].pressed; }
bool controller_released(ControllerButton button) { return button >= 0 && button < CONTROLLER_BUTTON_COUNT && r2d.controller_buttons[button].released; }
float controller_axis(ControllerAxis axis) { return axis >= 0 && axis < CONTROLLER_AXIS_COUNT ? r2d.controller_axes[axis] : 0.0f; }
