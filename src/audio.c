#include "retro2d_internal.h"

#include <limits.h>
#include <stdlib.h>
#include <string.h>

typedef struct WaveData {
    int16_t *samples;
    size_t frames;
} WaveData;

static bool id_equals(const uint8_t id[4], const char *text)
{
    return id[0] == (uint8_t)text[0] && id[1] == (uint8_t)text[1] &&
           id[2] == (uint8_t)text[2] && id[3] == (uint8_t)text[3];
}

static bool read_id(FILE *file, uint8_t id[4])
{
    return fread(id, 1, 4, file) == 4;
}

static bool load_wave_data(const char *path, WaveData *result)
{
    FILE *file;
    uint8_t id[4], *source = NULL;
    bool ok = true, have_format = false;
    uint16_t format = 0, channels = 0, bits = 0;
    uint32_t sample_rate = 0, data_size = 0;
    size_t source_frames, destination_frames;
    int bytes_per_sample;
    result->samples = NULL;
    result->frames = 0;
    if (!path || !(file = fopen(path, "rb"))) {
        r2d_set_error("could not open WAVE file '%s'", path ? path : "(null)");
        return false;
    }
    if (!read_id(file, id) || !id_equals(id, "RIFF")) ok = false;
    (void)r2d_read_u32(file, &ok);
    if (!read_id(file, id) || !id_equals(id, "WAVE")) ok = false;
    while (ok && read_id(file, id)) {
        uint32_t chunk_size = r2d_read_u32(file, &ok);
        long chunk_start = ftell(file);
        if (!ok || chunk_start < 0) break;
        if (id_equals(id, "fmt ")) {
            if (chunk_size < 16) { ok = false; break; }
            format = r2d_read_u16(file, &ok);
            channels = r2d_read_u16(file, &ok);
            sample_rate = r2d_read_u32(file, &ok);
            (void)r2d_read_u32(file, &ok);
            (void)r2d_read_u16(file, &ok);
            bits = r2d_read_u16(file, &ok);
            have_format = ok;
        } else if (id_equals(id, "data")) {
            if (chunk_size == 0 || chunk_size > 256u * 1024u * 1024u) {
                ok = false;
                break;
            }
            source = (uint8_t *)malloc(chunk_size);
            if (!source || fread(source, 1, chunk_size, file) != chunk_size) {
                ok = false;
                break;
            }
            data_size = chunk_size;
        }
        if ((uint64_t)chunk_start + chunk_size + (chunk_size & 1u) > (uint64_t)LONG_MAX ||
            fseek(file, chunk_start + (long)chunk_size + (long)(chunk_size & 1u),
                  SEEK_SET) != 0) ok = false;
        if (have_format && source) break;
    }
    fclose(file);
    if (!ok || !have_format || !source || format != 1 ||
        (channels != 1 && channels != 2) ||
        (bits != 8 && bits != 16) || sample_rate < 1000 || sample_rate > 384000) {
        free(source);
        r2d_set_error("'%s' must be an 8/16-bit mono/stereo PCM WAVE file", path);
        return false;
    }
    bytes_per_sample = bits / 8;
    source_frames = data_size / ((size_t)channels * (size_t)bytes_per_sample);
    if (!source_frames || source_frames > SIZE_MAX / R2D_AUDIO_RATE) {
        free(source);
        r2d_set_error("WAVE file '%s' contains no usable samples", path);
        return false;
    }
    destination_frames = (source_frames * R2D_AUDIO_RATE + sample_rate - 1u) / sample_rate;
    if (destination_frames > SIZE_MAX / (2u * sizeof(int16_t)) ||
        !(result->samples = (int16_t *)malloc(destination_frames * 2u * sizeof(int16_t)))) {
        free(source);
        r2d_set_error("not enough memory for WAVE file '%s'", path);
        return false;
    }
    for (size_t frame = 0; frame < destination_frames; ++frame) {
        size_t source_frame = (size_t)(((uint64_t)frame * sample_rate) / R2D_AUDIO_RATE);
        if (source_frame >= source_frames) source_frame = source_frames - 1;
        for (int channel = 0; channel < 2; ++channel) {
            int source_channel = channels == 1 ? 0 : channel;
            size_t offset = (source_frame * channels + (size_t)source_channel) *
                            (size_t)bytes_per_sample;
            int sample = bits == 8 ? ((int)source[offset] - 128) * 256 :
                         (int)(int16_t)((uint16_t)source[offset] |
                                       ((uint16_t)source[offset + 1] << 8));
            result->samples[frame * 2u + (size_t)channel] = (int16_t)sample;
        }
    }
    free(source);
    result->frames = destination_frames;
    return true;
}

bool r2d_audio_init(int channels)
{
    r2d.voice_limit = channels > 0 && channels <= R2D_MAX_VOICES ? channels : 32;
    r2d.next_playback = 1;
    r2d.sound_volume = 1.0f;
    r2d.music_volume = 1.0f;
    memset(r2d.voices, 0, sizeof r2d.voices);
    memset(&r2d.music_voice, 0, sizeof r2d.music_voice);
    if (!r2d.audio_ready)
        log_message(LOG_WARNING, "audio is unavailable; playback calls will be ignored");
    return r2d.audio_ready;
}

void r2d_audio_shutdown(void)
{
    r2d_platform_audio_lock();
    memset(r2d.voices, 0, sizeof r2d.voices);
    memset(&r2d.music_voice, 0, sizeof r2d.music_voice);
    r2d_platform_audio_unlock();
}

Sound *load_sound(const char *path)
{
    WaveData data;
    Sound *sound;
    if (!load_wave_data(path, &data)) return NULL;
    sound = (Sound *)calloc(1, sizeof *sound);
    if (!sound) {
        free(data.samples);
        r2d_set_error("not enough memory for sound '%s'", path);
        return NULL;
    }
    sound->samples = data.samples;
    sound->frame_count = data.frames;
    sound->next_resource = r2d.sounds;
    r2d.sounds = sound;
    return sound;
}

void free_sound(Sound *sound)
{
    Sound **link = &r2d.sounds;
    if (!sound) return;
    stop_sound(sound);
    while (*link && *link != sound) link = &(*link)->next_resource;
    if (*link) *link = sound->next_resource;
    free(sound->samples);
    free(sound);
}

Playback play_sound(Sound *sound)
{
    AudioVoice *voice = NULL;
    if (!sound || !r2d.audio_ready) return RETRO2D_NO_PLAYBACK;
    r2d_platform_audio_lock();
    for (int i = 0; i < r2d.voice_limit; ++i) {
        if (!r2d.voices[i].active) { voice = &r2d.voices[i]; break; }
    }
    if (!voice) voice = &r2d.voices[0];
    if (++r2d.next_playback == RETRO2D_NO_PLAYBACK) ++r2d.next_playback;
    *voice = (AudioVoice){ sound, 0, 1.0f, 0.0f, r2d.next_playback, true };
    r2d_platform_audio_unlock();
    return voice->handle;
}

void stop_playback(Playback playback)
{
    if (playback == RETRO2D_NO_PLAYBACK) return;
    r2d_platform_audio_lock();
    for (int i = 0; i < r2d.voice_limit; ++i)
        if (r2d.voices[i].handle == playback) r2d.voices[i].active = false;
    r2d_platform_audio_unlock();
}

void stop_sound(Sound *sound)
{
    if (!sound) return;
    r2d_platform_audio_lock();
    for (int i = 0; i < r2d.voice_limit; ++i)
        if (r2d.voices[i].sound == sound) r2d.voices[i].active = false;
    r2d_platform_audio_unlock();
}

void stop_all_sounds(void)
{
    r2d_platform_audio_lock();
    for (int i = 0; i < r2d.voice_limit; ++i) r2d.voices[i].active = false;
    r2d_platform_audio_unlock();
}

void set_playback_volume(Playback playback, float volume)
{
    r2d_platform_audio_lock();
    for (int i = 0; i < r2d.voice_limit; ++i)
        if (r2d.voices[i].handle == playback) r2d.voices[i].volume = r2d_clamp01(volume);
    r2d_platform_audio_unlock();
}

void set_playback_pan(Playback playback, float pan)
{
    if (pan < -1.0f) pan = -1.0f;
    if (pan > 1.0f) pan = 1.0f;
    r2d_platform_audio_lock();
    for (int i = 0; i < r2d.voice_limit; ++i)
        if (r2d.voices[i].handle == playback) r2d.voices[i].pan = pan;
    r2d_platform_audio_unlock();
}

void set_sound_volume(float volume)
{
    r2d_platform_audio_lock();
    r2d.sound_volume = r2d_clamp01(volume);
    r2d_platform_audio_unlock();
}

Music *load_music(const char *path)
{
    WaveData data;
    Music *music;
    if (!load_wave_data(path, &data)) return NULL;
    music = (Music *)calloc(1, sizeof *music);
    if (!music) {
        free(data.samples);
        r2d_set_error("not enough memory for music '%s'", path);
        return NULL;
    }
    music->samples = data.samples;
    music->frame_count = data.frames;
    music->next_resource = r2d.music_resources;
    r2d.music_resources = music;
    return music;
}

void free_music(Music *music)
{
    Music **link = &r2d.music_resources;
    if (!music) return;
    r2d_platform_audio_lock();
    if (r2d.music_voice.music == music) memset(&r2d.music_voice, 0, sizeof r2d.music_voice);
    r2d_platform_audio_unlock();
    while (*link && *link != music) link = &(*link)->next_resource;
    if (*link) *link = music->next_resource;
    free(music->samples);
    free(music);
}

static void start_music(Music *music, bool loop, float fade_seconds)
{
    if (!music || !r2d.audio_ready) return;
    r2d_platform_audio_lock();
    r2d.music_voice = (MusicVoice){ music, 0,
        fade_seconds > 0.0f ? 0.0f : 1.0f,
        fade_seconds > 0.0f ? 1.0f / (fade_seconds * R2D_AUDIO_RATE) : 0.0f,
        loop, true, false, false };
    r2d_platform_audio_unlock();
}

void play_music(Music *music, bool loop) { start_music(music, loop, 0.0f); }
void fade_music_in(Music *music, bool loop, float seconds) { start_music(music, loop, seconds); }
void pause_music(void) { r2d_platform_audio_lock(); r2d.music_voice.paused = true; r2d_platform_audio_unlock(); }
void resume_music(void) { r2d_platform_audio_lock(); if (r2d.music_voice.active) r2d.music_voice.paused = false; r2d_platform_audio_unlock(); }
void stop_music(void) { r2d_platform_audio_lock(); memset(&r2d.music_voice, 0, sizeof r2d.music_voice); r2d_platform_audio_unlock(); }

void fade_music_out(float seconds)
{
    r2d_platform_audio_lock();
    if (seconds <= 0.0f) memset(&r2d.music_voice, 0, sizeof r2d.music_voice);
    else if (r2d.music_voice.active) {
        r2d.music_voice.fade_step = -r2d.music_voice.fade / (seconds * R2D_AUDIO_RATE);
        r2d.music_voice.stop_after_fade = true;
    }
    r2d_platform_audio_unlock();
}

void set_music_volume(float volume)
{
    r2d_platform_audio_lock();
    r2d.music_volume = r2d_clamp01(volume);
    r2d_platform_audio_unlock();
}

bool music_playing(void)
{
    bool playing;
    r2d_platform_audio_lock();
    playing = r2d.music_voice.active;
    r2d_platform_audio_unlock();
    return playing;
}

static int clamp_sample(int value)
{
    if (value < INT16_MIN) return INT16_MIN;
    if (value > INT16_MAX) return INT16_MAX;
    return value;
}

void r2d_audio_mix(int16_t *output, int frames)
{
    if (!output || frames <= 0) return;
    memset(output, 0, (size_t)frames * 2u * sizeof(int16_t));
    for (int frame = 0; frame < frames; ++frame) {
        int left = 0, right = 0;
        for (int i = 0; i < r2d.voice_limit; ++i) {
            AudioVoice *voice = &r2d.voices[i];
            float left_volume, right_volume, volume;
            if (!voice->active) continue;
            if (voice->position >= voice->sound->frame_count) {
                voice->active = false;
                continue;
            }
            volume = voice->volume * r2d.sound_volume;
            left_volume = volume * (voice->pan > 0.0f ? 1.0f - voice->pan : 1.0f);
            right_volume = volume * (voice->pan < 0.0f ? 1.0f + voice->pan : 1.0f);
            left += (int)(voice->sound->samples[voice->position * 2u] * left_volume);
            right += (int)(voice->sound->samples[voice->position * 2u + 1u] * right_volume);
            if (++voice->position >= voice->sound->frame_count) voice->active = false;
        }
        if (r2d.music_voice.active && !r2d.music_voice.paused) {
            MusicVoice *music = &r2d.music_voice;
            if (music->position >= music->music->frame_count) {
                if (music->loop) music->position = 0;
                else music->active = false;
            }
            if (music->active) {
                float volume = r2d.music_volume * music->fade;
                left += (int)(music->music->samples[music->position * 2u] * volume);
                right += (int)(music->music->samples[music->position * 2u + 1u] * volume);
                ++music->position;
                if (music->fade_step != 0.0f) {
                    music->fade += music->fade_step;
                    if (music->fade >= 1.0f) { music->fade = 1.0f; music->fade_step = 0.0f; }
                    if (music->fade <= 0.0f) {
                        music->fade = 0.0f;
                        music->fade_step = 0.0f;
                        if (music->stop_after_fade) music->active = false;
                    }
                }
            }
        }
        output[frame * 2] = (int16_t)clamp_sample(left);
        output[frame * 2 + 1] = (int16_t)clamp_sample(right);
    }
}
