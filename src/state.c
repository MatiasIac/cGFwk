#include "retro2d_internal.h"

R2D_State r2d;

float r2d_clamp01(float value)
{
    if (value < 0.0f) return 0.0f;
    if (value > 1.0f) return 1.0f;
    return value;
}

uint16_t r2d_read_u16(FILE *file, bool *ok)
{
    uint8_t bytes[2];
    if (!*ok || fread(bytes, 1, sizeof bytes, file) != sizeof bytes) {
        *ok = false;
        return 0;
    }
    return (uint16_t)((uint16_t)bytes[0] | ((uint16_t)bytes[1] << 8));
}

uint32_t r2d_read_u32(FILE *file, bool *ok)
{
    uint8_t bytes[4];
    if (!*ok || fread(bytes, 1, sizeof bytes, file) != sizeof bytes) {
        *ok = false;
        return 0;
    }
    return (uint32_t)bytes[0] | ((uint32_t)bytes[1] << 8) |
           ((uint32_t)bytes[2] << 16) | ((uint32_t)bytes[3] << 24);
}

int32_t r2d_read_i32(FILE *file, bool *ok)
{
    return (int32_t)r2d_read_u32(file, ok);
}
