/**
 * @file include/halo/platform/audio.hpp
 * Sound output, independent of the operating system. The engine's sound code drives DirectSound 8 through raw method
 * tables (halo/sound/directsound.hpp), so the platform hands it a software device with the same interface layout:
 * IDirectSound8, IDirectSoundBuffer, IDirectSound3DBuffer and IDirectSound3DListener at the slots the engine calls.
 * Buffers are memory rings mixed by src/platform/audio_sdl.cpp on SDL2; there is no EAX (asking a buffer for a property
 * set fails, which turns the engine's effects off).
 */
#pragma once

#include <cstdint>

namespace halo::platform {

/** DirectSoundCreate8 with the default device: opens the output and returns the device object, negative on failure. */
int32_t __stdcall audio_device_create(void *device_guid, void **direct_sound, void *outer);

}  // namespace halo::platform
