/**
 * @file include/halo/sound/effects_backend.hpp
 * EAX effect backends behind one interface.
 */
#pragma once

#include "halo/sound/sound_types.hpp"

namespace halo::sound {

/**
 * Interface of one environmental-audio backend (EAX 1, 2 or 3). The effect object is the C record of
 * types/sound.h; the backend supplies the operations the object's vtable slots used to point at.
 */
class EffectsBackend {
public:
    virtual void shutdown(sound_effect_object * this_object) = 0;

    virtual int32_t initialize(sound_effect_object * this_object, directsound_channel * channel, int32_t unused) = 0;

    virtual int32_t initialize_channel(sound_effect_object * this_object, int32_t channel_index) = 0;

    /**
     * Default listener-support query shared by the EAX backends: returns the flag the backend's initialization
     * stored in the effect object.
     *
     * @address 0x0054ef10
     */
    virtual int32_t listener_supported(sound_effect_object * this_object);

    /**
     * Default per-channel support query shared by the EAX backends: returns the flag the backend's
     * initialization stored in the effect object.
     *
     * @address 0x0054ef20
     */
    virtual int32_t channel_supported(sound_effect_object * this_object);

    virtual void apply_channel(sound_effect_object * this_object, int32_t channel_index) = 0;

    virtual void set_environment_index(sound_effect_object * this_object, int32_t environment) = 0;

    virtual void apply_listener(sound_effect_object * this_object, const SoundEnvironment * environment) = 0;

    virtual void set_room_gain(sound_effect_object * this_object, float gain) = 0;

protected:
    constexpr EffectsBackend() = default;
    ~EffectsBackend() = default;
};

/**
 * EAX 1.0: listener environment and volume only, no per-channel effects.
 */
class Eax1Backend final : public EffectsBackend {
public:
    void apply_channel(sound_effect_object * this_object, int32_t channel_index) override;
    void apply_listener(sound_effect_object * this_object, const SoundEnvironment * environment) override;
    int32_t channel_supported(sound_effect_object * this_object) override;
    int32_t initialize(sound_effect_object * this_object, directsound_channel * channel, int32_t unused) override;
    int32_t initialize_channel(sound_effect_object * this_object, int32_t channel_index) override;
    void set_environment_index(sound_effect_object * this_object, int32_t environment) override;
    void set_room_gain(sound_effect_object * this_object, float gain) override;
    void shutdown(sound_effect_object * this_object) override;
};

/**
 * Shared base of the EAX 2.0 and 3.0 backends: per-channel property sets are queried from each 3D buffer.
 */
class EaxBackend : public EffectsBackend {
public:
    int32_t initialize_channel(sound_effect_object * this_object, int32_t channel_index) override;

protected:
    constexpr EaxBackend() = default;
    ~EaxBackend() = default;
};

/**
 * EAX 2.0 listener and channel properties. The object is a sound_eax_effect_object.
 */
class Eax2Backend final : public EaxBackend {
public:
    void apply_channel(sound_effect_object * this_object, int32_t channel_index) override;
    void apply_listener(sound_effect_object * this_object, const SoundEnvironment * environment) override;
    int32_t initialize(sound_effect_object * this_object, directsound_channel * channel, int32_t unused) override;
    void set_environment_index(sound_effect_object * this_object, int32_t environment) override;
    void set_room_gain(sound_effect_object * this_object, float gain) override;
    void shutdown(sound_effect_object * this_object) override;
};

/**
 * EAX 3.0 listener and channel properties. The object is a sound_eax_effect_object.
 */
class Eax3Backend final : public EaxBackend {
public:
    void apply_channel(sound_effect_object * this_object, int32_t channel_index) override;
    void apply_listener(sound_effect_object * this_object, const SoundEnvironment * environment) override;
    int32_t initialize(sound_effect_object * this_object, directsound_channel * channel, int32_t unused) override;
    void set_environment_index(sound_effect_object * this_object, int32_t environment) override;
    void set_room_gain(sound_effect_object * this_object, float gain) override;
    void shutdown(sound_effect_object * this_object) override;
};

/** The EAX 1.0 backend instance. */
Eax1Backend &eax1_backend();

/** The EAX 2.0 backend instance. */
Eax2Backend &eax2_backend();

/** The EAX 3.0 backend instance. */
Eax3Backend &eax3_backend();

}  // namespace halo::sound
