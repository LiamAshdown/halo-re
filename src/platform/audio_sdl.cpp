/**
 * @file src/platform/audio_sdl.cpp
 * Software DirectSound 8 on SDL2 (see halo/platform/audio.hpp). Secondary buffers are looping rings the engine fills
 * through Lock/Unlock behind the play cursor; the SDL audio callback mixes every playing buffer into 44.1 kHz stereo,
 * resampling by the buffer frequency. 3D buffers get DirectSound's default model: inverse-distance rolloff from the
 * minimum distance (clamped at the maximum), the sound cone, and a pan from the listener-relative direction. Doppler
 * and deferred settings are not modelled (the engine sets doppler 0; settings apply at once).
 */

#include "halo/platform/audio.hpp"

#include <SDL.h>
#if defined(__EMSCRIPTEN__)
#include <emscripten/threading.h>
#endif

#include "halo/core/libm.hpp"
#include <cstdlib>
#include <cstring>

namespace halo::platform {

namespace {

constexpr int32_t k_ok = 0;
constexpr int32_t k_no_interface = (int32_t)0x80004002;
constexpr int32_t k_invalid_call = (int32_t)0x88780032;
constexpr uint32_t k_output_rate = 44100;
constexpr uint32_t k_flag_primary = 0x1;
constexpr uint32_t k_iid_3d_listener = 0x279afa84;  // Data1 of IID_IDirectSound3DListener
constexpr uint32_t k_iid_3d_buffer = 0x279afa86;    // Data1 of IID_IDirectSound3DBuffer

struct sound_buffer;

/** A COM interface pointer: the 3D buffer (or, on the primary buffer, the listener) face of a buffer. */
struct face {
    void **vtable;
    sound_buffer *owner;
};

struct sound_buffer {
    void **vtable;
    face face_3d;
    int32_t refs;
    bool primary;
    uint8_t *data;
    uint32_t size;
    uint32_t block;  // bytes per frame
    uint32_t channels;
    uint32_t rate;   // the format's rate, the default frequency
    uint32_t flags;
    // mixer state, changed under the audio device lock
    bool playing;
    bool looping;
    double position;  // frames
    float frequency;
    float volume;     // linear
    // 3D
    uint32_t mode;    // 0 normal, 1 head-relative, 2 disabled
    float source[3];
    float cone[3];
    float min_distance, max_distance;
    float inner_cos, outer_cos;
    float outside_volume;  // linear
};

struct listener_state {
    float position[3];
    float front[3];
    float top[3];
    float rolloff;
};

SDL_AudioDeviceID g_device;
int32_t g_device_refs;
sound_buffer *g_buffers[512];  // ponytail: fixed table; the engine creates about 80 at once (the probe), creation fails past 512
uint32_t g_buffer_count;
listener_state g_listener = { { 0, 0, 0 }, { 0, 0, 1 }, { 0, 1, 0 }, 1.0f };
constexpr uint32_t k_mix_frames = 1024;
float g_mix[k_mix_frames * 2];

/** Same layout as the engine's sound_wave_format (WAVEFORMATEX). */
#pragma pack(push, 1)
struct wave_format {
    uint16_t format_tag;
    uint16_t channels;
    uint32_t samples_per_second;
    uint32_t average_bytes_per_second;
    uint16_t block_align;
    uint16_t bits_per_sample;
    uint16_t extra_size;
};
#pragma pack(pop)

/** DSBUFFERDESC. */
struct buffer_description {
    uint32_t size;
    uint32_t flags;
    uint32_t buffer_bytes;
    uint32_t reserved;
    wave_format *format;
    uint32_t algorithm_3d[4];
};

float millibels_to_linear(int32_t millibels)
{
    return millibels <= -10000 ? 0.0f : (float)halo::libm::pow(10.0, millibels / 2000.0);
}

float dot(const float *a, const float *b) { return a[0] * b[0] + a[1] * b[1] + a[2] * b[2]; }

void lock() { SDL_LockAudioDevice(g_device); }
void unlock() { SDL_UnlockAudioDevice(g_device); }

/** Left and right gains of a buffer for this mix. */
void buffer_gains(const sound_buffer *buffer, float *left, float *right)
{
    float gain = buffer->volume;
    float relative[3];
    float distance;
    float pan = 0.0f;

    *left = *right = gain;
    if ((buffer->flags & 0x10) == 0 || buffer->mode == 2) {
        return;
    }

    if (buffer->mode == 1) {
        memcpy(relative, buffer->source, sizeof(relative));
    } else {
        float offset[3] = { buffer->source[0] - g_listener.position[0], buffer->source[1] - g_listener.position[1],
            buffer->source[2] - g_listener.position[2] };
        float right_axis[3] = { g_listener.top[1] * g_listener.front[2] - g_listener.top[2] * g_listener.front[1],
            g_listener.top[2] * g_listener.front[0] - g_listener.top[0] * g_listener.front[2],
            g_listener.top[0] * g_listener.front[1] - g_listener.top[1] * g_listener.front[0] };

        relative[0] = dot(offset, right_axis);
        relative[1] = dot(offset, g_listener.top);
        relative[2] = dot(offset, g_listener.front);
        if (dot(buffer->cone, buffer->cone) > 0.0f && buffer->outer_cos < 1.0f) {
            // angle between the cone and the direction from the source to the listener
            float length = halo::libm::sqrtf(dot(offset, offset) * dot(buffer->cone, buffer->cone));
            float c = length > 0.0f ? -dot(offset, buffer->cone) / length : 1.0f;
            float cone_gain = 1.0f;

            if (c <= buffer->outer_cos) {
                cone_gain = buffer->outside_volume;
            } else if (c < buffer->inner_cos) {
                float t = (c - buffer->outer_cos) / (buffer->inner_cos - buffer->outer_cos);
                cone_gain = buffer->outside_volume + (1.0f - buffer->outside_volume) * t;
            }
            gain *= cone_gain;
        }
    }

    distance = halo::libm::sqrtf(dot(relative, relative));
    if (distance > buffer->max_distance) {
        distance = buffer->max_distance;
    }
    if (distance > buffer->min_distance && buffer->min_distance > 0.0f) {
        gain *= buffer->min_distance / (buffer->min_distance + g_listener.rolloff * (distance - buffer->min_distance));
    }
    if (distance > 0.0001f) {
        pan = relative[0] / distance;
    }
    // DirectSound's pan law: the far side drops, the near side stays at full level
    *left = gain * (pan > 0.0f ? 1.0f - pan : 1.0f);
    *right = gain * (pan < 0.0f ? 1.0f + pan : 1.0f);
}

/** Mixes up to k_mix_frames output frames. */
void mix_block(int16_t *out, uint32_t frames)
{
    uint32_t i;

    memset(g_mix, 0, frames * 2 * sizeof(float));
    for (uint32_t n = 0; n < g_buffer_count; n++) {
        sound_buffer *buffer = g_buffers[n];
        const int16_t *samples = reinterpret_cast<const int16_t *>(buffer->data);
        uint32_t total;
        double step = (double)buffer->frequency / k_output_rate;
        float left, right;

        if (!buffer->playing || buffer->primary || buffer->size == 0) {
            continue;
        }
        total = buffer->size / buffer->block;
        buffer_gains(buffer, &left, &right);
        for (i = 0; i < frames; i++) {
            uint32_t a = (uint32_t)buffer->position;
            uint32_t b = a + 1 < total ? a + 1 : 0;
            float t = (float)(buffer->position - a);

            if (buffer->channels == 1) {
                float s = samples[a] + (samples[b] - samples[a]) * t;
                g_mix[i * 2] += s * left;
                g_mix[i * 2 + 1] += s * right;
            } else {
                g_mix[i * 2] += (samples[a * 2] + (samples[b * 2] - samples[a * 2]) * t) * left;
                g_mix[i * 2 + 1] += (samples[a * 2 + 1] + (samples[b * 2 + 1] - samples[a * 2 + 1]) * t) * right;
            }
            buffer->position += step;
            if (buffer->position >= total) {
                if (!buffer->looping) {
                    buffer->playing = false;
                    buffer->position = 0.0;
                    break;
                }
                buffer->position -= total;
            }
        }
    }

    for (i = 0; i < frames * 2; i++) {
        float s = g_mix[i];
        out[i] = (int16_t)(s > 32767.0f ? 32767.0f : s < -32768.0f ? -32768.0f : s);
    }
}

void SDLCALL mix(void *, Uint8 *stream, int length)
{
    int16_t *out = reinterpret_cast<int16_t *>(stream);
    uint32_t frames = (uint32_t)length / 4;

    while (frames > 0) {
        uint32_t block = frames < k_mix_frames ? frames : k_mix_frames;

        mix_block(out, block);
        out += block * 2;
        frames -= block;
    }
}

/* IUnknown, shared by every object: the device has no owner, a buffer is its own, a face forwards to its buffer */

int32_t __stdcall device_query_interface(void *, const uint8_t *, void **out) { *out = nullptr; return k_no_interface; }
uint32_t __stdcall device_add_ref(void *) { return (uint32_t)++g_device_refs; }

uint32_t __stdcall device_release(void *)
{
    if (--g_device_refs == 0) {
#if !defined(__EMSCRIPTEN__)
        // In the browser the device stays open: closing it from the game's worker frees the stream the page thread's
        // callback is mixing into. The next audio_device_create reuses it.
        SDL_CloseAudioDevice(g_device);
        g_device = 0;
#endif
    }
    return (uint32_t)g_device_refs;
}

uint32_t __stdcall buffer_add_ref(sound_buffer *buffer) { return (uint32_t)++buffer->refs; }

uint32_t __stdcall buffer_release(sound_buffer *buffer)
{
    int32_t refs = --buffer->refs;

    if (refs == 0) {
        lock();
        for (uint32_t i = 0; i < g_buffer_count; i++) {
            if (g_buffers[i] == buffer) {
                g_buffers[i] = g_buffers[--g_buffer_count];
                break;
            }
        }
        unlock();
        free(buffer->data);
        free(buffer);
    }
    return (uint32_t)refs;
}

int32_t __stdcall buffer_query_interface(sound_buffer *buffer, const uint8_t *iid, void **out)
{
    uint32_t data1;

    memcpy(&data1, iid, 4);
    if (data1 == (buffer->primary ? k_iid_3d_listener : k_iid_3d_buffer) && (buffer->flags & 0x10) != 0) {
        buffer->refs++;
        *out = &buffer->face_3d;
        return k_ok;
    }
    *out = nullptr;
    return k_no_interface;
}

int32_t __stdcall face_query_interface(face *self, const uint8_t *iid, void **out) { return buffer_query_interface(self->owner, iid, out); }
uint32_t __stdcall face_add_ref(face *self) { return buffer_add_ref(self->owner); }
uint32_t __stdcall face_release(face *self) { return buffer_release(self->owner); }

/* IDirectSoundBuffer */

int32_t __stdcall buffer_get_caps(sound_buffer *buffer, uint32_t *caps)
{
    caps[1] = (buffer->flags & ~4u) | 8u;  // DSBCAPS_LOCSOFTWARE, never LOCHARDWARE
    caps[2] = buffer->size;
    caps[3] = 0;
    caps[4] = 0;
    return k_ok;
}

int32_t __stdcall buffer_get_current_position(sound_buffer *buffer, uint32_t *play, uint32_t *write)
{
    uint32_t cursor;
    uint32_t lead;

    if (buffer->size == 0) {
        return k_invalid_call;
    }
    lock();
    cursor = (uint32_t)buffer->position * buffer->block;
    unlock();
    // DirectSound keeps a few milliseconds between the cursors; the engine's streaming refill depends on the gap
    lead = ((uint32_t)buffer->frequency / 100 + 1) * buffer->block;
    if (play != nullptr) {
        *play = cursor;
    }
    if (write != nullptr) {
        *write = (cursor + lead) % buffer->size;
    }
    return k_ok;
}

int32_t __stdcall buffer_get_status(sound_buffer *buffer, uint32_t *status)
{
    *status = buffer->playing ? (buffer->looping ? 5u : 1u) : 0u;
    return k_ok;
}

int32_t __stdcall buffer_lock(sound_buffer *buffer, uint32_t offset, uint32_t bytes, void **pointer1, uint32_t *bytes1, void **pointer2,
    uint32_t *bytes2, uint32_t)
{
    if (buffer->size == 0 || offset >= buffer->size) {
        return k_invalid_call;
    }
    if (bytes > buffer->size) {
        bytes = buffer->size;
    }
    *pointer1 = buffer->data + offset;
    if (offset + bytes > buffer->size) {
        *bytes1 = buffer->size - offset;
        if (pointer2 != nullptr) {
            *pointer2 = buffer->data;
            *bytes2 = bytes - *bytes1;
        }
    } else {
        *bytes1 = bytes;
        if (pointer2 != nullptr) {
            *pointer2 = nullptr;
            *bytes2 = 0;
        }
    }
    return k_ok;
}

int32_t __stdcall buffer_unlock(sound_buffer *, void *, uint32_t, void *, uint32_t) { return k_ok; }

int32_t __stdcall buffer_play(sound_buffer *buffer, uint32_t, uint32_t, uint32_t flags)
{
    lock();
    buffer->looping = (flags & 1) != 0;
    buffer->playing = true;
    unlock();
    return k_ok;
}

int32_t __stdcall buffer_stop(sound_buffer *buffer)
{
    lock();
    buffer->playing = false;
    unlock();
    return k_ok;
}

int32_t __stdcall buffer_set_current_position(sound_buffer *buffer, uint32_t offset)
{
    lock();
    buffer->position = buffer->block != 0 ? (double)(offset / buffer->block) : 0.0;
    unlock();
    return k_ok;
}

int32_t __stdcall buffer_set_format(sound_buffer *, const wave_format *) { return k_ok; }  // the output is always 44.1 kHz stereo

int32_t __stdcall buffer_set_volume(sound_buffer *buffer, int32_t volume)
{
    buffer->volume = millibels_to_linear(volume);
    return k_ok;
}

int32_t __stdcall buffer_set_frequency(sound_buffer *buffer, uint32_t frequency)
{
    buffer->frequency = (float)(frequency != 0 ? frequency : buffer->rate);
    return k_ok;
}

int32_t __stdcall buffer_restore(sound_buffer *) { return k_ok; }

/* IDirectSound3DBuffer */

int32_t __stdcall b3d_set_cone_angles(face *self, uint32_t inside, uint32_t outside, uint32_t)
{
    self->owner->inner_cos = halo::libm::cosf((float)inside * 0.5f * 0.017453292f);
    self->owner->outer_cos = halo::libm::cosf((float)outside * 0.5f * 0.017453292f);
    return k_ok;
}

int32_t __stdcall b3d_set_cone_orientation(face *self, float x, float y, float z, uint32_t)
{
    self->owner->cone[0] = x;
    self->owner->cone[1] = y;
    self->owner->cone[2] = z;
    return k_ok;
}

int32_t __stdcall b3d_set_cone_outside_volume(face *self, int32_t volume, uint32_t)
{
    self->owner->outside_volume = millibels_to_linear(volume);
    return k_ok;
}

int32_t __stdcall b3d_set_max_distance(face *self, float distance, uint32_t) { self->owner->max_distance = distance; return k_ok; }
int32_t __stdcall b3d_set_min_distance(face *self, float distance, uint32_t) { self->owner->min_distance = distance; return k_ok; }
int32_t __stdcall b3d_set_mode(face *self, uint32_t mode, uint32_t) { self->owner->mode = mode; return k_ok; }

int32_t __stdcall b3d_set_position(face *self, float x, float y, float z, uint32_t)
{
    self->owner->source[0] = x;
    self->owner->source[1] = y;
    self->owner->source[2] = z;
    return k_ok;
}

int32_t __stdcall b3d_set_velocity(face *, float, float, float, uint32_t) { return k_ok; }

/* IDirectSound3DListener */

int32_t __stdcall lst_set_factor(face *, float, uint32_t) { return k_ok; }  // distance and doppler factors
int32_t __stdcall lst_set_rolloff_factor(face *, float rolloff, uint32_t) { g_listener.rolloff = rolloff; return k_ok; }

int32_t __stdcall lst_set_orientation(face *, float fx, float fy, float fz, float tx, float ty, float tz, uint32_t)
{
    g_listener.front[0] = fx;
    g_listener.front[1] = fy;
    g_listener.front[2] = fz;
    g_listener.top[0] = tx;
    g_listener.top[1] = ty;
    g_listener.top[2] = tz;
    return k_ok;
}

int32_t __stdcall lst_set_position(face *, float x, float y, float z, uint32_t)
{
    g_listener.position[0] = x;
    g_listener.position[1] = y;
    g_listener.position[2] = z;
    return k_ok;
}

int32_t __stdcall lst_set_velocity(face *, float, float, float, uint32_t) { return k_ok; }
int32_t __stdcall lst_commit_deferred_settings(face *) { return k_ok; }

/* IDirectSound8 */

int32_t __stdcall device_create_sound_buffer(void *, const buffer_description *, sound_buffer **, void *);

int32_t __stdcall device_get_caps(void *, uint32_t *caps)
{
    uint32_t size = caps[0];

    memset(caps, 0, size);
    caps[0] = size;
    caps[1] = 0x10 | 0x100 | 0x200 | 0x400 | 0x800;  // continuous rate, mono, stereo, 8 and 16 bit secondaries
    caps[2] = 100;      // minimum secondary rate
    caps[3] = 100000;   // maximum secondary rate
    return k_ok;
}

int32_t __stdcall device_set_cooperative_level(void *, void *, uint32_t) { return k_ok; }

/* method tables; unused slots stay null */

void *g_device_vtable[12] = { (void *)device_query_interface, (void *)device_add_ref, (void *)device_release,
    (void *)device_create_sound_buffer, (void *)device_get_caps, nullptr, (void *)device_set_cooperative_level };

void *g_buffer_vtable[21] = { (void *)buffer_query_interface, (void *)buffer_add_ref, (void *)buffer_release,
    (void *)buffer_get_caps, (void *)buffer_get_current_position, nullptr, nullptr, nullptr, nullptr,
    (void *)buffer_get_status, nullptr, (void *)buffer_lock, (void *)buffer_play, (void *)buffer_set_current_position,
    (void *)buffer_set_format, (void *)buffer_set_volume, nullptr, (void *)buffer_set_frequency, (void *)buffer_stop,
    (void *)buffer_unlock, (void *)buffer_restore };

void *g_buffer_3d_vtable[21] = { (void *)face_query_interface, (void *)face_add_ref, (void *)face_release,
    nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
    (void *)b3d_set_cone_angles, (void *)b3d_set_cone_orientation, (void *)b3d_set_cone_outside_volume,
    (void *)b3d_set_max_distance, (void *)b3d_set_min_distance, (void *)b3d_set_mode, (void *)b3d_set_position,
    (void *)b3d_set_velocity };

void *g_listener_vtable[18] = { (void *)face_query_interface, (void *)face_add_ref, (void *)face_release,
    nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
    (void *)lst_set_factor, (void *)lst_set_factor, (void *)lst_set_orientation, (void *)lst_set_position,
    (void *)lst_set_rolloff_factor, (void *)lst_set_velocity, (void *)lst_commit_deferred_settings };

void *g_device_object = g_device_vtable;  // the device is a singleton: its "this" is just its vtable pointer

int32_t __stdcall device_create_sound_buffer(void *, const buffer_description *description, sound_buffer **out, void *)
{
    sound_buffer *buffer = static_cast<sound_buffer *>(calloc(1, sizeof(sound_buffer)));

    buffer->vtable = g_buffer_vtable;
    buffer->refs = 1;
    buffer->flags = description->flags;
    buffer->primary = (description->flags & k_flag_primary) != 0;
    buffer->face_3d.vtable = buffer->primary ? g_listener_vtable : g_buffer_3d_vtable;
    buffer->face_3d.owner = buffer;
    buffer->volume = 1.0f;
    buffer->max_distance = 1e9f;
    buffer->min_distance = 1.0f;
    buffer->inner_cos = buffer->outer_cos = -1.0f;  // 360 degree cones
    buffer->outside_volume = 1.0f;
    if (!buffer->primary) {
        const wave_format *format = description->format;

        if (format == nullptr || format->bits_per_sample != 16 || format->channels == 0 || format->channels > 2 ||
            description->buffer_bytes < format->block_align) {
            free(buffer);
            *out = nullptr;
            return k_invalid_call;
        }
        buffer->channels = format->channels;
        buffer->block = format->block_align;
        buffer->rate = format->samples_per_second;
        buffer->frequency = (float)buffer->rate;
        buffer->size = description->buffer_bytes - description->buffer_bytes % buffer->block;
        buffer->data = static_cast<uint8_t *>(calloc(1, buffer->size));
    }

    lock();
    if (g_buffer_count == sizeof(g_buffers) / sizeof(g_buffers[0])) {
        unlock();
        free(buffer->data);
        free(buffer);
        *out = nullptr;
        return k_invalid_call;
    }
    g_buffers[g_buffer_count++] = buffer;
    unlock();
    *out = buffer;
    return k_ok;
}

}  // namespace

namespace {

/** Opens the output (44.1 kHz stereo, mixed by mix()); 0 on failure. */
int open_output()
{
    SDL_AudioSpec want;
    SDL_AudioSpec have;

    SDL_SetMainReady();
    if (SDL_InitSubSystem(SDL_INIT_AUDIO) != 0) {
        return 0;
    }
    memset(&want, 0, sizeof(want));
    want.freq = (int)k_output_rate;
    want.format = AUDIO_S16SYS;
    want.channels = 2;
    want.samples = 1024;
    want.callback = mix;
    g_device = SDL_OpenAudioDevice(nullptr, 0, &want, &have, 0);
    if (g_device != 0) {
        SDL_PauseAudioDevice(g_device, 0);
    }
    return g_device != 0;
}

}  // namespace

int32_t __stdcall audio_device_create(void *, void **direct_sound, void *)
{
    *direct_sound = nullptr;
    if (g_device == 0) {
#if defined(__EMSCRIPTEN__)
        // Web Audio lives on the page thread; the game runs on a worker. The mixer callback then runs on the page
        // thread too, reading the buffers through the shared heap.
        if (!emscripten_sync_run_in_main_runtime_thread(EM_FUNC_SIG_I, open_output)) {
            return k_invalid_call;
        }
#else
        if (!open_output()) {
            return k_invalid_call;
        }
#endif
    }
    g_device_refs++;
    *direct_sound = &g_device_object;
    return k_ok;
}

}  // namespace halo::platform
