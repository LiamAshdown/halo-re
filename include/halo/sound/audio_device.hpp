/**
 * @file include/halo/sound/audio_device.hpp
 * The audio backend interface and its DirectSound implementation.
 */
#pragma once

#include "halo/sound/sound_types.hpp"

namespace halo::sound {

/**
 * Backend-neutral audio device. The engine drives sound through this interface (one hardware voice per logical
 * channel index, a 3D listener, a begin/end frame bracket), so a different backend can replace DirectSound
 * without touching the callers.
 */
class AudioDevice {
public:
    /**
     * Brings up DirectSound: creates the device, takes priority cooperative level, reads the caps, creates the
     * 3D primary buffer and sets its format (16-bit stereo at 44100 Hz on quality 2 when the hardware allows,
     * else 22050 or 11025 Hz), then decides how many channels of each type to create. With EAX enabled and a
     * healthy first probe, the counts come from an iterative search that grows one pool at a time and rolls
     * back when a pool stops growing or another pool shrinks;
     *
     * @address 0x00545e20
     */
    virtual uint8_t initialize(sound_driver_parameters *parameters) = 0;

    /**
     * Releases every directsound_channel's buffer and 3D-buffer interfaces, shuts down and frees the global EAX
     * sound effects object, releases the listener and primary buffer, restores the DirectSound device's
     * cooperative level before releasing it, and clears every associated global.
     *
     * @address 0x00546a60
     */
    virtual void dispose(void) = 0;

    /**
     * Pushes each part of `parameters` (position, orientation, velocity, EAX environment) to the DirectSound 3D
     * listener and, when supported, the active EAX effects object, but only when it has actually changed (by
     * more than 0.05 for position/orientation, 0.01 for velocity, or any difference for the environment) since
     * the last call, or on the very first call after (re)initialization.
     *
     * @address 0x00547070
     */
    virtual void set_listener(sound_listener_parameters *parameters) = 0;

    /**
     * Clears the "3D listener settings need CommitDeferredSettings" flag at the start of a frame's DirectSound
     * work.
     *
     * @address 0x00546f90
     */
    virtual void begin_frame(void) = 0;

    /**
     * Closes a frame of DirectSound work: commits deferred 3D listener settings, steps the pause fade toward
     * its target (stopping the buffers once a pause fade reaches zero), runs the streaming update and renders
     * the channel debug text when it is enabled.
     *
     * @address 0x00546b80
     */
    virtual void end_frame(void) = 0;

    /**
     * Binds a hardware channel to `channel_index` if it does not already have one, then queues `source` for
     * playback on that hardware channel. `unused` is never read.
     *
     * @address 0x00548380
     */
    virtual void channel_play(int16_t channel_index, SoundPermutation *source, int16_t unused, int16_t sound_class, uint8_t crosslap) = 0;

    /**
     * Binds a hardware channel to `channel_index` if it does not already have one, then refreshes that hardware
     * channel's streaming fill. `sound_class` is never read.
     *
     * @address 0x005483d0
     */
    virtual void channel_continue(int16_t channel_index, uint8_t unused, int16_t sound_class) = 0;

    /**
     * If `channel_index` has a bound hardware channel, resets that hardware channel and clears the logical
     * channel's state (idle) and its binding (unbound).
     *
     * @address 0x00548410
     */
    virtual void channel_stop(int16_t channel_index) = 0;

    /**
     * If `channel_index` has a bound hardware channel, returns its loop-boundary-checked state; otherwise
     * returns idle (0).
     *
     * @address 0x00548450
     */
    virtual directsound_channel_state channel_get_state(int16_t channel_index) = 0;

    /**
     * Only acts when the paused state actually changes. Going from paused to unpaused, every occupied hardware
     * channel has its cached gain zeroed and, if it still has a live buffer, is restarted looping (Play, flags
     * = DSBPLAY_LOOPING), then its streaming state is refreshed. Going the other way (or any other transition)
     * just records the new state.
     *
     * @address 0x00546fe0
     */
    virtual void set_paused(uint8_t paused) = 0;

    /**
     * Resets every hardware DirectSound channel (stopping its buffer and clearing its queued sources) and marks
     * each one not-free, overriding sound_channel_reset's own free = 1.
     *
     * @address 0x00546fa0
     */
    virtual void stop_all(void) = 0;

    /**
     * Binds a hardware channel to `channel_index` if it does not already have one, then forwards the spatial
     * parameters to that hardware channel.
     *
     * @address 0x00548470
     */
    virtual void channel_set_spatial(int16_t channel_index, uint8_t spatialized, sound_channel_spatial *spatial, float obstruction, float occlusion, uint8_t underwater, int16_t sound_class) = 0;

    /**
     * Binds a hardware channel to `channel_index` if it does not already have one, then forwards `parameters`
     * to that hardware channel.
     *
     * @address 0x005484d0
     */
    virtual void channel_set_parameters(int16_t channel_index, sound_channel_parameters *parameters, uint8_t unknown) = 0;

    /**
     * Clamps and stores the sound quality level [0, 2] (default 1), reinitializes the EAX effects object if the
     * requested eax_enabled state differs from whether EAX is actually active, then forwards the first argument
     * to sound_driver_set_eax_enabled, forcing the device reopen (CL = 1) when the quality changed or the
     * effects object was reinitialized.
     *
     * @address 0x005480f0
     */
    virtual void set_quality(int32_t unknown, uint8_t eax_enabled, int32_t quality) = 0;

    /**
     * True when EAX hardware support was detected, EAX is currently enabled, an effects object exists, and its
     * mode is one of EAX1/EAX2/EAX3.
     *
     * @address 0x005482a0
     */
    virtual uint8_t eax_available(void) = 0;

protected:
    constexpr AudioDevice() = default;
    ~AudioDevice() = default;
};

/**
 * The DirectSound / EAX backend: the only implementation today. Its state lives in the engine's fixed-address
 * globals (directsound_channels, directsound_bindings and friends), so the object itself carries no data.
 */
class DirectSoundDevice final : public AudioDevice {
public:
    uint8_t initialize(sound_driver_parameters *parameters) override;
    void dispose(void) override;
    void set_listener(sound_listener_parameters *parameters) override;
    void begin_frame(void) override;
    void end_frame(void) override;
    void channel_play(int16_t channel_index, SoundPermutation *source, int16_t unused, int16_t sound_class, uint8_t crosslap) override;
    void channel_continue(int16_t channel_index, uint8_t unused, int16_t sound_class) override;
    void channel_stop(int16_t channel_index) override;
    directsound_channel_state channel_get_state(int16_t channel_index) override;
    void set_paused(uint8_t paused) override;
    void stop_all(void) override;
    void channel_set_spatial(int16_t channel_index, uint8_t spatialized, sound_channel_spatial *spatial, float obstruction, float occlusion, uint8_t underwater, int16_t sound_class) override;
    void channel_set_parameters(int16_t channel_index, sound_channel_parameters *parameters, uint8_t unknown) override;
    void set_quality(int32_t unknown, uint8_t eax_enabled, int32_t quality) override;
    uint8_t eax_available(void) override;

    /**
     * Probes the DirectSound driver's support for up to four channel-buffer pools (mono 3D, mono, stereo, and
     * 44kHz stereo), selected by `pool_mask` bits 0x02/0x04/0x10/0x100, writing how many buffers of each type
     * could actually be created. Every probed interface is released before returning; nothing is kept.
     *
     * @address 0x00545a30
     */
    void probe_channel_pools(int32_t *mono3d_count, uint32_t mono3d_requested, int32_t *mono_count, uint32_t mono_requested, int32_t *stereo_count, uint32_t stereo_requested, int32_t *stereo44k_count, uint32_t stereo44k_requested, uint32_t pool_mask);

    /**
     * Creates the hardware buffer of one DirectSound channel of the given type (2D or 3D, mono or stereo, 22 or
     * 44 kHz), fills in its cached state and, for 3D channels, acquires the 3D buffer interface. Returns 1 on
     * success and 0 when the buffer cannot be created.
     *
     * @address 0x00546760
     */
    uint8_t create_channel(int16_t channel_index, uint16_t type_flags);

    /**
     * If the sound driver is initialized and either the requested EAX state differs from the current one or
     * `force` is set, updates directsound_eax_enabled, resets sound_driver_parameters and
     * directsound_hardware_mode to their defaults, and reopens the sound device with them (sound_reopen_device,
     * reached by `jmp` after overwriting the stack argument with 0x0069f514).
     *
     * @address 0x00548200
     */
    void set_eax_enabled(uint8_t eax_enabled, uint8_t force);

    /**
     * If `logical_channel_index` has no hardware channel bound yet, scans forward from that channel type's
     * first slot for one that is unbound and either already marked available or just finished playing
     * (sound_channel_claim_if_finished), stopping at the end of that type's block or the channel table. On
     * success, binds the hardware channel back to this logical channel.
     *
     * @address 0x005482e0
     */
    void bind_hardware(int16_t logical_channel_index);

    /**
     * When the channel has a pending source_end_cursor, checks whether the hardware play cursor has crossed
     * from before it to at-or-past write_cursor (wraparound-aware). If so: a queued (state 2) channel becomes
     * playing (1) and this returns 1 immediately; a playing (state 1) channel with no successor becomes idle
     * (0). Either way source_end_cursor is cleared. Returns the resulting state.
     *
     * @address 0x00548050
     */
    directsound_channel_state check_loop_boundary(int16_t channel_index);

    /**
     * Queries the channel's hardware playback status; fails (returns 0) if the query itself fails, or if the
     * channel is not marked streaming and is still reported playing/looping. Otherwise resets streaming_bytes
     * (for a streaming channel) and clears `free`, claiming the slot.
     *
     * @address 0x00547ff0
     */
    uint32_t claim_if_finished(int16_t channel_index);

    /**
     * Writes `byte_count` bytes of PCM into `destination` (one locked segment of the ring buffer that starts at
     * ring offset `base_position`), walking current source -> next source as each is exhausted and zero-filling
     * once no source is left.
     *
     * @address 0x00547ab0
     */
    void fill_pcm_data(int16_t channel_index, uint8_t *destination, int32_t base_position, uint8_t *crosslap, int32_t byte_count);

    /**
     * Locks `fill_size` bytes of the channel's ring buffer at its write_cursor, fills the one or two returned
     * segments from the channel's sources, and unlocks.
     *
     * @address 0x00547a00
     */
    uint8_t lock_and_fill(int16_t channel_index, uint32_t fill_size);

    /**
     * Idle channel: takes `source`, fills the ring buffer (all of it, or the part between the cursors if it is
     * still streaming silence) and, if the buffer was not already running, rewinds, restores and starts it
     * looping. Playing channel: the source becomes the queued next source (or the current one, if the channel
     * had run dry).
     *
     * @address 0x00547c80
     */
    void queue_source(int16_t channel_index, SoundPermutation *source, int16_t sound_class, uint8_t crosslap);

    /**
     * Returns the channel's current hardware play cursor (IDirectSoundBuffer::GetCurrentPosition, vtable +0x10;
     * the write cursor output is dropped). read ([esp] after the call), not the buffer interface pointer as the
     * draft had it.
     *
     * @address 0x00547890
     */
    uint32_t refresh_cursor(int16_t channel_index);

    /**
     * Clears the channel's queued sources; for a weapon-fire 3D channel not mid-stop-all, keeps it streaming
     * and re-triggers a refill instead of a hard stop. Otherwise stops the DirectSound buffer outright. Either
     * way resets the channel to idle/free.
     *
     * @address 0x00547f60
     */
    void reset_channel(int16_t channel_index);

    /**
     * Restores a lost DirectSound buffer, retrying while it stays lost. Reports through `was_restored_out`
     * whether a restore happened and returns the DirectSound result code.
     *
     * @address 0x00547c10
     */
    int32_t restore_buffer(void *buffer, uint8_t *was_restored_out);

    /**
     * Commits a channel's volume (directsound_fade * gain, converted to a clamped DirectSound attenuation), and
     * -- unless `update` is set -- its frequency and, for 3D channels, distance, cone-angle, cone-outside-gain
     * and EAX value parameters, each only when changed.
     *
     * @address 0x005475b0
     */
    void commit_parameters(int16_t channel_index, sound_channel_parameters *parameters, uint8_t update);

    /**
     * Updates one directsound_channel's 3D mode, position, cone orientation, velocity (all cached and only
     * re-committed to the buffer when they change by more than a small epsilon, or the mode itself just
     * changed, or this is the first call since initialization), and obstruction/ occlusion/underwater state,
     * then reapplies the EAX per-channel effect when supported.
     *
     * @address 0x005472d0
     */
    void commit_spatial(int16_t channel_index, uint8_t spatialized, sound_channel_spatial *spatial, float obstruction, float occlusion, uint8_t underwater, int16_t sound_class);

    /**
     * Refills the part of a playing (or silence-streaming) channel's ring buffer that the play cursor has
     * consumed since the last fill. A channel that just switched to streaming silence (streaming_bytes == -1)
     * starts from the hardware write cursor and records how much it wrote; later passes accumulate
     * streaming_bytes (reset to a full buffer on overflow).
     *
     * @address 0x005478c0
     */
    void stream_update(int16_t channel_index, uint8_t unused);
};

/** The device instance every caller uses. */
DirectSoundDevice &directsound_device();

/** The active audio device. */
AudioDevice &audio_device();

}  // namespace halo::sound
