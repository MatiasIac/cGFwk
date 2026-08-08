#include "retro2d_internal.h"

bool r2d_platform_init(const GameConfig *config) { (void)config; return true; }
void r2d_platform_shutdown(void) {}
void r2d_platform_poll_events(void) {}
void r2d_platform_present(const uint8_t *pixels, const Color *palette)
{ (void)pixels; (void)palette; }
uint64_t r2d_platform_ticks_ns(void) { return 0; }
void r2d_platform_delay_ns(uint64_t nanoseconds) { (void)nanoseconds; }
void r2d_platform_show_cursor(bool visible) { (void)visible; }
void r2d_platform_confine_cursor(bool confined) { (void)confined; }
void r2d_platform_audio_lock(void) {}
void r2d_platform_audio_unlock(void) {}
