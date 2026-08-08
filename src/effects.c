#include "retro2d_internal.h"

#include <limits.h>
#include <string.h>

EffectHandle add_effect(PostEffect effect, void *user_data)
{
    EffectSlot *slot;
    if (!effect) {
        r2d_set_error("cannot add a NULL post-processing effect");
        return 0;
    }
    if (r2d.effect_count >= R2D_MAX_EFFECTS) {
        r2d_set_error("the post-processing effect list is full");
        return 0;
    }
    if (r2d.next_effect == INT_MAX) r2d.next_effect = 1;
    else ++r2d.next_effect;
    slot = &r2d.effects[r2d.effect_count++];
    slot->handle = r2d.next_effect;
    slot->callback = effect;
    slot->user_data = user_data;
    slot->enabled = true;
    return slot->handle;
}

static int find_effect(EffectHandle handle)
{
    int i;
    for (i = 0; i < r2d.effect_count; ++i)
        if (r2d.effects[i].handle == handle) return i;
    return -1;
}

void remove_effect(EffectHandle handle)
{
    int index = find_effect(handle);
    if (index < 0) return;
    if (index + 1 < r2d.effect_count)
        memmove(&r2d.effects[index], &r2d.effects[index + 1],
                (size_t)(r2d.effect_count - index - 1) * sizeof(EffectSlot));
    --r2d.effect_count;
}

void enable_effect(EffectHandle handle, bool enabled)
{
    int index = find_effect(handle);
    if (index >= 0) r2d.effects[index].enabled = enabled;
}

bool move_effect(EffectHandle handle, int new_index)
{
    int old_index = find_effect(handle);
    EffectSlot saved;
    if (old_index < 0 || new_index < 0 || new_index >= r2d.effect_count) return false;
    if (old_index == new_index) return true;
    saved = r2d.effects[old_index];
    if (old_index < new_index)
        memmove(&r2d.effects[old_index], &r2d.effects[old_index + 1],
                (size_t)(new_index - old_index) * sizeof(EffectSlot));
    else
        memmove(&r2d.effects[new_index + 1], &r2d.effects[new_index],
                (size_t)(old_index - new_index) * sizeof(EffectSlot));
    r2d.effects[new_index] = saved;
    return true;
}

void clear_effects(void)
{
    r2d.effect_count = 0;
}

const uint8_t *r2d_apply_effects(float elapsed)
{
    const uint8_t *source = r2d.screen.pixels;
    uint8_t *destination = r2d.effect_a;
    size_t size = (size_t)r2d.screen.stride * (size_t)r2d.screen.height;
    int i;
    for (i = 0; i < r2d.effect_count; ++i) {
        EffectSlot *effect = &r2d.effects[i];
        if (!effect->enabled) continue;
        memcpy(destination, source, size);
        effect->callback(source, destination, r2d.screen.width, r2d.screen.height,
                         r2d.screen.stride, r2d.screen.stride, r2d.palette,
                         elapsed, effect->user_data);
        source = destination;
        destination = destination == r2d.effect_a ? r2d.effect_b : r2d.effect_a;
    }
    return source;
}
