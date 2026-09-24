# `sound` — Blam sound: object looping sounds, sound instances, DirectSound driver, EAX

Retail Halo PC `halo.exe` 1.0.10, `0x543a30 .. 0x5514d0` (134 Ghidra functions in
`out/phase4/sound_functions.md`), plain C / MSVC 7.1 / x86. Every file in this directory is one
function, rewritten against `types/sound.h` (plus `tags.h`, `memory.h`, `cache.h`, `math.h`,
`objects.h`, `game.h`), with the original Ghidra decompile, and since the phase-4 review the
capstone disassembly of the whole function, preserved at the bottom inside `#if 0 ... #endif`.

- 133 files: 132 of the 134 listed addresses (two are Ghidra fragments, see below) plus
  `sound_looping_track_location_proc` (`0x54dc10`), a real function Ghidra never made.
- Gate: `python tools/build_check.py sound` → **133 ok, 0 failed**; whole tree
  `python tools/build_check.py` → **3229 ok, 0 failed**.
- Every file carries a `Phase-4 review` note: each function was re-read against its disassembly.
  The Ghidra C of this module hides most register arguments (`EAX`/`ECX`/`EDX`/`EBX`/`ESI`/`EDI`
  and `AL`/`CL`/`DX`/`BX` inputs, x87 temporaries, out-parameters written into argument slots), and
  the first drafts guessed them; the review replaced those guesses with what the code does.

## What the module contains

| Layer | Range | What it is |
|---|---|---|
| game sound | `0x543a30`–`0x544d4e` | game-state "object looping sounds" (`game_looping_sound`, 0x400 × 0x34): sounds bound to object nodes, scripted looping sounds (hs `sound_looping_start/stop/set_scale/set_alternate/predict`), the BSP cluster background sound; audibility bitmap of BSP clusters |
| impulse API | `0x543ce0`–`0x543fff`, `0x549ee0` | starting one-shot sounds at an object marker, at a fixed placement or unspatialized; hs `sound_impulse_start/time` and the impulse fade-out |
| ogg / pcm feed | `0x544e00`–`0x545a2f` | libvorbisfile memory streams (two `OggVorbis_File` per channel for crosslapped loops), PCM copy out of the sound cache |
| sound classes | `0x545330`–`0x5454a0`, `0x548590`–`0x5492ee`, `0x54b100` | 51 `sound_class_definition` rows, per-class gain fades, the master / music / effects sliders (muting classes by name) |
| permutations | `0x5454a0`–`0x54570f` | pitch-range choice by pitch, random permutation choice with a no-repeat bitmask and skip fractions |
| directsound driver | `0x545a30`–`0x548587` | the only `sound_driver` (`0x0069f4c8`): hardware channels (`directsound_channel`, 0x678 bytes), ring-buffer streaming, 3D listener / buffer parameter caches, pausing fade |
| sound engine | `0x5492f0`–`0x54e6cf` | `sound_initialize/update/dispose`, the "sounds" (0x200 × 0xb0) and "looping sounds" (0x80 × 0xe4) datum tables, channel assignment and stealing, 3D spatialization, gain fades, dialog ducking, looping track state machine, detail sounds |
| codec | `0x54e830`–`0x54e91b` | Xbox ADPCM block layout and nibble decoder |
| EAX effects | `0x54ec40`–`0x551610` | the EAX1 / EAX2 / EAX3 `sound_effect_object`s (vtables at `0x00671d4c` / `0x00671d04` / `0x00671d28`): IKsPropertySet probing, listener reverb, per-channel obstruction / occlusion |

### How a sound plays

```
start: sound_play_new (0x549af0)                 <- impulse API, looping tracks (0x54d9f0), detail sounds
  skip-fraction roll, audibility (0x54bb20), promotion throttle (0x54b050), pitch range + permutation,
  start delay = distance * 8.96 ms (speed of sound), sound_cache_touch
per tick: sound_update (0x549810), every 32 ms
  sound_listener_update chain (0x54b970 -> driver set_listener 0x547070)
  sound_update_looping_states (0x54d270)     detail sounds due, stale looping_sounds freed
  sound_update_range_and_ducking (0x54bd60)  location procs, audibility fades, dialog ducking
  sound_assign_channels (0x54c020)           cache residency, channel choice / stealing
  sound_update_active_instances (0x54c900)   3D placement, then
    sound_update_instance_gain (0x54c750) or sound_update_looping_gain (0x54deb0)
      -> sound_driver.channel_play / channel_continue / channel_set_parameters
driver: sound_channel_queue_source (0x547c80) -> sound_channel_lock_and_fill (0x547a00)
        -> sound_channel_fill_pcm_data (0x547ab0) -> PCM copy (0x545860) or Ogg decode (0x545920)
game side: game_sound_update (0x5445c0) -> game_looping_sound_update (0x544330)
        -> sound_looping_set_state (0x549fa0) -> sound_looping_create_detail_sound (0x54d9f0)
```

## Struct layouts

All live in `types/sound.h` (pack 1). Offsets are byte offsets from the struct base. The tag-side
records (`Sound`, `SoundPitchRange`, `SoundPermutation`, `SoundLooping`, `SoundLoopingTrack`,
`SoundLoopingDetail`, `SoundEnvironment`) are the `types/tags.h` definitions and are not repeated
here; `SoundPermutation +0x2c` (`samples_pointer`) holds the sound cache datum and `+0x30` the
resident sample pointer at runtime.

### `sound_location` — size `0x40`

| Off | Type | Field | Notes |
|---|---|---|---|
| 0x00 | `int16_t` | `type` | sound_location_type |
| 0x02 | `int16_t` | `unknown_02` | never read; a short store pads it |
| 0x04 | `float` | `scale` | lerp factor between Sound.zero_* and Sound.one_* modifiers |
| 0x08 | `float` | `gain` | 1.0 from every builder; detail gain for detail sounds |
| 0x0c | `Point3D` | `position` |  |
| 0x18 | `Vector3D` | `forward` | matrix4x3_transform_normal result |
| 0x24 | `Vector3D` | `velocity` | world units per tick: object_get_root_object_velocities (0x4f6aa0) in 0x544330 / 0x5448c0, zero for detail sounds (0x54dc70); 0x54c900 turns it into the doppler velocity |
| 0x30 | `int32_t` | `leaf_index` | object_get_root_location (0x4f6b10) pair, written by 0x5448c0 |
| 0x34 | `int16_t` | `cluster_index` | -1 skips the obstruction test in 0x544aa0 |
| 0x36 | `int16_t` | `unknown_36` | high half of the leaf reference dword, never read |
| 0x38 | `float` | `obstruction` | 0.6 / 0.45 / 0.0 from 0x544aa0; printed by render_debug_sound |
| 0x3c | `float` | `occlusion` | 1.0 disables the channel (0x54bb20); printed by render_debug_sound |

### `sound_object_marker_data` — size `0x1c`

| Off | Type | Field | Notes |
|---|---|---|---|
| 0x00 | `int16_t` | `unknown_00` | never written (stack garbage copied along) |
| 0x02 | `int16_t` | `node_index` | object node, -1 means node 0 |
| 0x04 | `Point3D` | `position` | node space, through matrix4x3_transform_point |
| 0x10 | `Vector3D` | `forward` | node space, through matrix4x3_transform_normal |

### `sound_placement` — size `0x2c`

| Off | Type | Field | Notes |
|---|---|---|---|
| 0x00 | `Point3D` | `position` | -> location 0x0c |
| 0x0c | `Vector3D` | `forward` | -> location 0x18 |
| 0x18 | `Vector3D` | `velocity` | -> location 0x24 |
| 0x24 | `int32_t` | `leaf_index` | -> location 0x30 |
| 0x28 | `int16_t` | `cluster_index` | -> location 0x34 |
| 0x2a | `int16_t` | `unknown_2a` | -> location 0x36 |

### `sound_class_definition` — size `0x2c`

| Off | Type | Field | Notes |
|---|---|---|---|
| 0x00 | `int16_t` | `maximum_sounds_per_tag` | 3/4/2, candidate list limit in 0x54c1d0 |
| 0x02 | `int16_t` | `maximum_sounds_per_object` | 2/1/4, second candidate limit in 0x54c1d0 |
| 0x04 | `int32_t` | `minimum_replace_time` | ms a channel must play before 0x54c5e0 steals it |
| 0x08 | `uint8_t` | `dialog` | 1 for unit_dialog and the scripted_dialog classes: mouth_data lip sync (0x54c900), cache miss retry (0x54bcd0), shared channel per object (0x54c2f0) |
| 0x09 | `uint8_t` | `unknown_09` | zero in every row, never read |
| 0x0a | `int16_t` | `priority` | 1..6, compared by 0x54c6b0 |
| 0x0c | `int16_t` | `discard_on_cache_miss` | UNSURE name: 0 makes 0x54c020 stop a sound whose samples are not resident at its start time |
| 0x0e | `int16_t` | `unknown_0e` | zero in every row, never read |
| 0x10 | `float` | `eax_value` | 0.5..1.0, eighth float of the channel parameters, lands in directsound_channel.eax_value (0x60) |
| 0x14 | `float` | `unknown_14` | 0.0 in every row, never read |
| 0x18 | `float` | `default_minimum_distance` | fallback when Sound.minimum_distance == 0 |
| 0x1c | `float` | `default_maximum_distance` | fallback when Sound.maximum_distance == 0 |
| 0x20 | `float` | `unknown_20` | 0.0 or 1.0, never read in this module |
| 0x24 | `float` | `unknown_24` | 1.0 in every row, never read in this module |
| 0x28 | `uint8_t` | `muted` | set by the gain setters when their slider hits 0 |
| 0x29 | `uint8_t` | `unknown_29[3]` | padding |

### `sound_class_gain` — size `0x0c`

| Off | Type | Field | Notes |
|---|---|---|---|
| 0x00 | `float` | `target_gain` | clamped [0,1] by 0x545390 |
| 0x04 | `float` | `current_gain` | read by 0x54b100, interpolated toward target by 0x545330 |
| 0x08 | `int16_t` | `fade_ticks` | ticks left; 0 snaps current to target |
| 0x0a | `int16_t` | `unknown_0a` | padding |

### `game_looping_sound` — size `0x34`

| Off | Type | Field | Notes |
|---|---|---|---|
| 0x00 | `int16_t` | `identifier` | datum salt |
| 0x02 | `int16_t` | `state` | game_looping_sound_state |
| 0x04 | `uint32_t` | `flags` | game_looping_sound_flags |
| 0x08 | `float` | `scale` | script gain, clamped [0,1] by 0x544180 |
| 0x0c | `datum_index` | `definition_index` | SoundLooping tag |
| 0x10 | `datum_index` | `object_index` | -1 for script and background sounds |
| 0x14 | `int32_t` | `last_update` | game_sound_globals.update_count at the last pass |
| 0x18 | `int16_t` | `function_index` | object function_out_values index (object 0x134, valid 0x123), -1 none |
| 0x1a | `int16_t` | `node_index` | object node matrix index (object nodes, stride 0x34) |
| 0x1c | `Point3D` | `position` | node space position |
| 0x28 | `Vector3D` | `forward` | node space direction |

### `game_sound_globals` — size `0x0c`

| Off | Type | Field | Notes |
|---|---|---|---|
| 0x00 | `int32_t` | `update_count` | bumped by each full game_sound_update pass |
| 0x04 | `datum_index` | `background_sound_index` | game_looping_sound for the cluster background sound |
| 0x08 | `int32_t` | `last_update_time` | QueryPerformanceCounter ms at the last full pass |

### `sound` — size `0xb0`

| Off | Type | Field | Notes |
|---|---|---|---|
| 0x00 | `int16_t` | `identifier` | datum salt |
| 0x02 | `int16_t` | `play_state` | sound_play_state |
| 0x04 | `uint16_t` | `flags` | sound_flags (byte accesses) |
| 0x06 | `int16_t` | `listener_index` | 0x54bb20 result, stride 0x44 into sound_listeners |
| 0x08 | `datum_index` | `definition_index` | Sound tag |
| 0x0c | `datum_index` | `owner_index` | object for impulses, looping_sound for detail/track sounds |
| 0x10 | `sound_location_proc` | `location_proc` | called by 0x54bcd0 with owner, callback_data, location |
| 0x14 | `sound_location` | `location` |  |
| 0x54 | `uint8_t` | `callback_data[0x30]` | caller bytes (param_5/param_6 of 0x549af0) |
| 0x84 | `int32_t` | `start_time` | sound clock ms; channel granted when reached (0x54c020) |
| 0x88 | `float` | `pitch` | random pitch from 0x54aec0 / random_range_real |
| 0x8c | `int16_t` | `channel_index` | sound_channel slot, -1 while waiting |
| 0x8e | `int16_t` | `pitch_range_index` | sound_permutation_pick_for_pitch |
| 0x90 | `int16_t` | `permutation_index` | sound_permutation_pick_random |
| 0x92 | `int16_t` | `fade_curve` | sound_fade_curve |
| 0x94 | `int16_t` | `track_index` | SoundLoopingTrack index, -1 for impulses |
| 0x96 | `int16_t` | `unknown_96` | never read or written |
| 0x98 | `datum_index` | `pending_definition_index` | 0x54dd90 queues a definition switch, -1 none |
| 0x9c | `float` | `fade_start_gain` | written by 0x54af60 |
| 0xa0 | `float` | `fade_end_gain` | 0 means the sound dies when the fade completes |
| 0xa4 | `int32_t` | `fade_start_time` | sound clock ms |
| 0xa8 | `int32_t` | `fade_end_time` | == start means no fade |
| 0xac | `uint8_t` | `first_person` | weapon classes / player dialog; head relative in 0x54c900 |
| 0xad | `uint8_t` | `unknown_ad[3]` | padding |

### `looping_sound` — size `0xe4`

| Off | Type | Field | Notes |
|---|---|---|---|
| 0x00 | `int16_t` | `identifier` | datum salt |
| 0x02 | `int16_t` | `unknown_02` | never read or written in this module |
| 0x04 | `datum_index` | `definition_index` | SoundLooping tag |
| 0x08 | `int32_t` | `owner` | caller handle (game_looping_sound index), searched by 0x54e5d0 |
| 0x0c | `sound_location` | `location` |  |
| 0x4c | `uint8_t` | `update_toggle` | copy of the frame toggle, keep alive |
| 0x4d | `uint8_t` | `alternate` | alternate loop / end selected |
| 0x4e | `uint8_t` | `finished` | track ran out of permutations |
| 0x4f | `uint8_t` | `unknown_4f` | padding |
| 0x50 | `int16_t` | `active_sound_count` | sounds created through 0x54d9f0 still alive |
| 0x52 | `int16_t` | `state` | last state passed to sound_looping_set_state (2 = stopped) |
| 0x54 | `int32_t` | `detail_next_time[32]` | per SoundLoopingDetail, sound clock ms |
| 0xd4 | `datum_index` | `track_sounds[4]` | sound playing each SoundLoopingTrack |

### `sound_channel` — size `0x18`

| Off | Type | Field | Notes |
|---|---|---|---|
| 0x00 | `datum_index` | `sound_index` | sound playing here, -1 free |
| 0x04 | `uint16_t` | `type_flags` | sound_channel_type_flags of its driver channel type |
| 0x06 | `int16_t` | `unknown_06` | never read |
| 0x08 | `float` | `play_time` | accumulated time * pitch (0x54d020) |
| 0x0c | `float` | `current_pitch` | scales pitch range natural_pitch in 0x54deb0 |
| 0x10 | `SoundPermutation *` | `current_permutation` | holds a sound cache reference |
| 0x14 | `SoundPermutation *` | `next_permutation` | queued behind current |

### `sound_channel_candidate_list` — size `0x48`

| Off | Type | Field | Notes |
|---|---|---|---|
| 0x00 | `int16_t` | `tag_match_count` |  |
| 0x02 | `int16_t` | `tag_matches[16]` | sound_channel indices |
| 0x22 | `int16_t` | `tag_match_limit` | sound_class_definition.maximum_sounds_per_tag |
| 0x24 | `int16_t` | `owner_match_count` |  |
| 0x26 | `int16_t` | `owner_matches[16]` | sound_channel indices |
| 0x46 | `int16_t` | `owner_match_limit` | sound_class_definition.maximum_sounds_per_object |

### `sound_listener` — size `0x44`

| Off | Type | Field | Notes |
|---|---|---|---|
| 0x00 | `uint8_t` | `valid` | cleared when there is no local player |
| 0x01 | `uint8_t` | `underwater` | FUN_0053ed60; edges play the matg enter/exit water sounds |
| 0x02 | `int16_t` | `unknown_02` | padding |
| 0x04 | `float` | `scale` | real_matrix4x3 (types/math.h) from FUN_004cb970 |
| 0x08 | `Vector3D` | `forward` |  |
| 0x14 | `Vector3D` | `left` |  |
| 0x20 | `Vector3D` | `up` |  |
| 0x2c | `Point3D` | `position` |  |
| 0x38 | `Vector3D` | `velocity` | subtracted from the 30 Hz source velocity in 0x54c900 |

### `sound_observer_camera` — size `0x29c`

| Off | Type | Field | Notes |
|---|---|---|---|
| 0x00 | `Point3D` | `position` | listener position |
| 0x0c | `int32_t` | `leaf_index` | with cluster_index a {leaf, cluster} location; its address goes to FUN_0053ed60 (underwater test) |
| 0x10 | `int16_t` | `cluster_index` | listener cluster, -1 outside the bsp |
| 0x12 | `int16_t` | `unknown_12` |  |
| 0x14 | `Vector3D` | `velocity` | world units per tick, rotated into listener space |
| 0x20 | `Vector3D` | `forward` |  |
| 0x2c | `Vector3D` | `up` |  |
| 0x38 | `uint8_t` | `unknown_38[0x264]` | not read by this module |

### `sound_listener_parameters` — size `0x34`

| Off | Type | Field | Notes |
|---|---|---|---|
| 0x00 | `Point3D` | `position` | always global_zero: sources are moved into listener space |
| 0x0c | `Vector3D` | `forward` | SetOrientation front |
| 0x18 | `Vector3D` | `up` | SetOrientation top |
| 0x24 | `Vector3D` | `velocity` | SetVelocity |
| 0x30 | `SoundEnvironment *` | `environment` | compared as 0x12 dwords against the cache |

### `directsound_listener_cache` — size `0x30`

| Off | Type | Field | Notes |
|---|---|---|---|
| 0x00 | `Point3D` | `position` | SetPosition |
| 0x0c | `Vector3D` | `forward` | SetOrientation front |
| 0x18 | `Vector3D` | `up` | SetOrientation top |
| 0x24 | `Vector3D` | `velocity` | SetVelocity |

### `sound_channel_spatial` — size `0x24`

| Off | Type | Field | Notes |
|---|---|---|---|
| 0x00 | `Point3D` | `position` | SetPosition, y negated |
| 0x0c | `Vector3D` | `forward` | SetConeOrientation, y negated |
| 0x18 | `Vector3D` | `velocity` | SetVelocity, y negated |

### `sound_channel_parameters` — size `0x20`

| Off | Type | Field | Notes |
|---|---|---|---|
| 0x00 | `float` | `minimum_distance` | SetMinDistance |
| 0x04 | `float` | `maximum_distance` | SetMaxDistance, always FLT_MAX |
| 0x08 | `float` | `pitch` | converted to SetFrequency |
| 0x0c | `float` | `gain` | SetVolume after the fade factor |
| 0x10 | `float` | `inner_cone_angle` | SetConeAngles |
| 0x14 | `float` | `outer_cone_angle` |  |
| 0x18 | `float` | `outer_cone_gain` | SetConeOutsideVolume |
| 0x1c | `float` | `eax_value` | sound_class_definition.eax_value |

### `sound_driver_parameters` — size `0x14`

| Off | Type | Field | Notes |
|---|---|---|---|
| 0x00 | `int16_t` | `driver_index` | index into the driver table at 0x0069f508 (0..1) |
| 0x02 | `int16_t` | `channel_counts[4]` | per channel type, decremented when a buffer fails (0x545e20) |
| 0x0a | `int16_t` | `slot_counts[4]` | per channel type, summed into the sound_channel count |
| 0x12 | `int16_t` | `unknown_12` | zero, never read |

### `sound_driver` — size `0x40`

| Off | Type | Field | Notes |
|---|---|---|---|
| 0x00 | `int16_t` | `type` | must equal sound_driver_parameters.driver_index |
| 0x02 | `int16_t` | `unknown_02` | padding |
| 0x04 | `uint8_t (*)(sound_driver_parameters *parameters)` | `initialize` | 0x545e20 |
| 0x08 | `void (*)(void)` | `dispose` | 0x546a60 (Ghidra: game_sound_dispose) |
| 0x0c | `void (*)(sound_listener_parameters *listener)` | `set_listener` | 0x547070 |
| 0x10 | `void (*)(void)` | `begin_frame` | 0x546f90 clears the deferred flag |
| 0x14 | `void (*)(void)` | `end_frame` | 0x546b80 fade, stream fill, debug text |
| 0x18 | `void (*)(int16_t channel_index, SoundPermutation *source, int16_t unused, int16_t sound_class, uint8_t crosslap)` | `channel_play` | 0x548380 -> 0x547c80 (CL = crosslap; the third argument is not read) |
| 0x1c | `void (*)(int16_t channel_index, uint8_t unused, int16_t sound_class)` | `channel_continue` | 0x5483d0 -> 0x5478c0 (only the channel is used; the byte is pushed to 0x5478c0, which ignores it) |
| 0x20 | `void (*)(int16_t channel_index)` | `channel_stop` | 0x548410 -> 0x547f60 |
| 0x24 | `int16_t (*)(int16_t channel_index)` | `channel_get_state` | 0x548450 -> 0x548050 |
| 0x28 | `void (*)(uint8_t paused)` | `set_paused` | 0x546fe0 |
| 0x2c | `void (*)(void)` | `stop_all` | 0x546fa0 |
| 0x30 | `void (*)(int16_t channel_index, uint8_t spatialized, sound_channel_spatial *spatial, float obstruction, float occlusion, uint8_t underwater, int16_t sound_class)` | `channel_set_spatial` | 0x548470 -> 0x5472d0 |
| 0x34 | `void (*)(int16_t channel_index, sound_channel_parameters *parameters, uint8_t unknown)` | `channel_set_parameters` | 0x5484d0 -> 0x5475b0 |
| 0x38 | `void (*)(int32_t unknown, uint8_t eax_enabled, int32_t quality)` | `set_quality` | 0x5480f0 |
| 0x3c | `uint8_t (*)(void)` | `eax_available` | 0x5482a0 |

### `sound_channel_binding` — size `0x04`

| Off | Type | Field | Notes |
|---|---|---|---|
| 0x00 | `int16_t` | `hardware_channel_index` | -1 until 0x5482e0 assigns one |
| 0x02 | `int16_t` | `channel_type` | 0..3, index into 0x0069f528 and 0x00746028 |

### `sound_ogg_memory_file` — size `0x10`

| Off | Type | Field | Notes |
|---|---|---|---|
| 0x00 | `int32_t` | `position` | seek callback 0x544e00 bounds it by size |
| 0x04 | `void *` | `data` | SoundPermutation cached sample pointer |
| 0x08 | `int32_t` | `size` | SoundPermutation.samples.size |
| 0x0c | `uint8_t` | `end_of_file` | cleared by every seek |
| 0x0d | `uint8_t` | `unknown_0d[3]` | padding |

### `sound_stream_decoder` — size `0x5cc`

| Off | Type | Field | Notes |
|---|---|---|---|
| 0x000 | `int32_t` | `position` | PCM byte offset into the source (sound_pcm_buffer_read) |
| 0x004 | `int32_t` | `decoded_bytes` | ogg bytes produced so far for the current source |
| 0x008 | `uint8_t` | `ogg_vorbis_file[2][0x2d0]` | two OggVorbis_File for crosslapped transitions |
| 0x5a8 | `uint8_t` | `open` | a stream is open (also cleared by 0x547f60) |
| 0x5a9 | `uint8_t` | `active_file` | 0 selects file 1 / memory file 1, 1 selects file 0 / memory file 0 |
| 0x5aa | `int16_t` | `unknown_5aa` | padding |
| 0x5ac | `sound_ogg_memory_file` | `memory_files[2]` | datasource of each OggVorbis_File |

### `directsound_channel` — size `0x678`

| Off | Type | Field | Notes |
|---|---|---|---|
| 0x000 | `int16_t` | `state` | directsound_channel_state |
| 0x002 | `int16_t` | `sound_channel_index` | owning logical channel, -1 free |
| 0x004 | `int16_t` | `sound_class` | SoundClass of the queued source, -1 idle |
| 0x006 | `uint8_t` | `spatialized` | 3D mode normal vs disabled (SetMode) |
| 0x007 | `uint8_t` | `underwater` | low pass through the EAX direct path |
| 0x008 | `uint8_t` | `free` | released and reusable |
| 0x009 | `uint8_t` | `streaming` | keeps filling silence after the source ends |
| 0x00a | `int16_t` | `unknown_00a` | never referenced |
| 0x00c | `Point3D` | `position` | cached SetPosition |
| 0x018 | `Vector3D` | `cone_orientation` | cached SetConeOrientation |
| 0x024 | `Vector3D` | `velocity` | cached SetVelocity |
| 0x030 | `uint8_t` | `unknown_030[8]` | never referenced |
| 0x038 | `uint16_t` | `type_flags` | sound_channel_type_flags |
| 0x03a | `int16_t` | `unknown_03a` | never referenced |
| 0x03c | `float` | `gain` | cached gain (before the fade factor) |
| 0x040 | `float` | `pitch` | cached pitch |
| 0x044 | `float` | `obstruction` |  |
| 0x048 | `float` | `occlusion` |  |
| 0x04c | `float` | `minimum_distance` |  |
| 0x050 | `float` | `maximum_distance` |  |
| 0x054 | `float` | `cone_outside_gain` |  |
| 0x058 | `float` | `inner_cone_angle` | radians, compared with 2 degree tolerance |
| 0x05c | `float` | `outer_cone_angle` |  |
| 0x060 | `float` | `eax_value` | sound_class_definition.eax_value |
| 0x064 | `int32_t` | `unknown_064` | never referenced |
| 0x068 | `int32_t` | `buffer_size` | 3 * channels * 2 * rate bytes |
| 0x06c | `int16_t` | `frequency` | last SetFrequency value |
| 0x06e | `int16_t` | `unknown_06e` | never referenced |
| 0x070 | `int32_t` | `volume` | last SetVolume value, hundredths of a decibel |
| 0x074 | `int32_t` | `unknown_074` | never referenced |
| 0x078 | `int32_t` | `write_cursor` | ring buffer position filled up to |
| 0x07c | `int32_t` | `source_end_cursor` | where the current source ends, -1 none |
| 0x080 | `int32_t` | `unknown_080` | never referenced |
| 0x084 | `int32_t` | `streaming_bytes` | silence bytes written after the source, -1 none |
| 0x088 | `SoundPermutation *` | `source` | permutation being fed |
| 0x08c | `SoundPermutation *` | `next_source` | queued permutation |
| 0x090 | `uint8_t` | `source_crosslap` | current source continues an ogg stream |
| 0x091 | `uint8_t` | `next_source_crosslap` |  |
| 0x092 | `uint8_t` | `unknown_092[6]` | never referenced |
| 0x098 | `uint8_t` | `source_started` | first fill of the current source done |
| 0x099 | `uint8_t` | `unknown_099[7]` | never referenced |
| 0x0a0 | `sound_stream_decoder` | `decoder` |  |
| 0x66c | `int32_t` | `unknown_66c` | never referenced |
| 0x670 | `void *` | `buffer` | IDirectSoundBuffer * |
| 0x674 | `void *` | `buffer_3d` | IDirectSound3DBuffer *, null for 2D channels |

### `sound_wave_format` — size `0x12`

| Off | Type | Field | Notes |
|---|---|---|---|
| 0x00 | `uint16_t` | `format_tag` | 1 = PCM |
| 0x02 | `uint16_t` | `channels` |  |
| 0x04 | `uint32_t` | `samples_per_second` | 22050 / 44100 |
| 0x08 | `uint32_t` | `average_bytes_per_second` |  |
| 0x0c | `uint16_t` | `block_align` |  |
| 0x0e | `uint16_t` | `bits_per_sample` | 16 |
| 0x10 | `uint16_t` | `extra_size` | 0 |

### `sound_buffer_description` — size `0x24`

| Off | Type | Field | Notes |
|---|---|---|---|
| 0x00 | `uint32_t` | `size` | 0x24 |
| 0x04 | `uint32_t` | `flags` | 0x100a0 / 0x100a8 / 0x10 (3D) / 0x200 |
| 0x08 | `uint32_t` | `buffer_bytes` |  |
| 0x0c | `uint32_t` | `reserved` |  |
| 0x10 | `sound_wave_format *` | `format` |  |
| 0x14 | `uint32_t` | `algorithm_3d[4]` | GUID, DS3DALG_HRTF_FULL written when EAX is off |

### `win32_dsbcaps` — size `0x14`

| Off | Type | Field | Notes |
|---|---|---|---|
| 0x00 | `uint32_t` | `size` | 0x14 |
| 0x04 | `uint32_t` | `flags` |  |
| 0x08 | `uint32_t` | `buffer_bytes` |  |
| 0x0c | `uint32_t` | `unlock_transfer_rate` |  |
| 0x10 | `uint32_t` | `play_cpu_overhead` |  |

### `sound_effect_object_vtable` — size `0x24`

| Off | Type | Field | Notes |
|---|---|---|---|
| 0x00 | `void (*)(void *this_object)` | `shutdown` | 0x54ef30 / 0x54fff0 / 0x54ec40 |
| 0x04 | `int32_t (*)(void *this_object, directsound_channel *channels, int32_t unknown)` | `initialize` |  |
| 0x08 | `int32_t (*)(void *this_object, int32_t channel_index)` | `initialize_channel` | 0x54f6e0 / 0x551240 |
| 0x0c | `int32_t (*)(void *this_object)` | `listener_supported` | 0x54ef10 returns +0x10 |
| 0x10 | `int32_t (*)(void *this_object)` | `channel_supported` | 0x54ef20 returns +0x14 |
| 0x14 | `void (*)(void *this_object, int32_t channel_index)` | `apply_channel` |  |
| 0x18 | `void (*)(void *this_object, int32_t environment)` | `set_environment_index` |  |
| 0x1c | `void (*)(void *this_object, SoundEnvironment *environment)` | `apply_listener` |  |
| 0x20 | `void (*)(void *this_object, float gain)` | `set_room_gain` | converted to millibels |

### `sound_effect_object` — size `0x1c` (EAX1 object size, operator_new(0x1c))

| Off | Type | Field | Notes |
|---|---|---|---|
| 0x00 | `sound_effect_object_vtable *` | `vtable` |  |
| 0x04 | `int32_t` | `mode` | sound_effect_object_mode |
| 0x08 | `uint32_t` | `supported_properties` | one bit per property id that QuerySupport accepted; bit 0 set means deferred (id / 0x80000000) Set calls |
| 0x0c | `int32_t` | `unknown_0c` | never referenced |
| 0x10 | `int32_t` | `listener_supported` |  |
| 0x14 | `int32_t` | `channel_supported` |  |
| 0x18 | `void *` | `property_set` | IKsPropertySet * from channel 0 buffer_3d |

### `sound_eax_effect_object` — size `0xe8` (EAX2 and EAX3, operator_new(0xe8))

| Off | Type | Field | Notes |
|---|---|---|---|
| 0x00 | `sound_effect_object` | `base` |  |
| 0x1c | `void *` | `channel_property_sets[51]` | IKsPropertySet * per 3D channel; slot 0 also carries the listener properties |

## Globals

The full list (address, type, name, meaning) is the `globals` block at the end of
`types/sound.h`. The ones every file shares, with the names used consistently across
`src/sound`:

| Address | Declaration | Note |
|---|---|---|
| `0x007252c0` | `data_array *sound_data` | "sounds", 0x200 × 0xb0 |
| `0x00724a50` | `data_array *looping_sound_data` | "looping sounds", 0x80 × 0xe4 |
| `0x007461a0` | `data_array *game_looping_sound_data` | "object looping sounds", game state |
| `0x007461a4` | `game_sound_globals *game_sound_globals_ptr` | |
| `0x00724a60` | `sound_channel sound_channels[81]` | logical channels, count at `0x007252b4` |
| `0x00725430` | `directsound_channel directsound_channels[81]` | hardware channels, count at `0x00725428` |
| `0x007252e4` | `sound_channel_binding directsound_bindings[81]` | logical → hardware |
| `0x00725208` | `sound_driver *current_sound_driver` | always `0x0069f4c8` |
| `0x00721f24` | `sound_effect_object *global_sound_effect_object` | EAX object |
| `0x006e36cc` | `sound_channel_parameters_proc sound_channel_parameters_proc_ptr` | `0x54ce50`, or `0x54cf80` with EAX |
| `0x00746140` | `sound_class_gain *sound_class_gains` | a pointer to the 51-entry table |
| `0x0072520c` / `0x00725210` | `int32_t sound_time` / `float sound_time_delta` | ms clock; delta = elapsed ms × 0.03 |
| `0x00725218` | `sound_listener sound_listeners[1]` | |
| `0x006ac6d0` | `sound_observer_camera observer_cameras[]` | an array, owned by the observer code |

## Known gaps

- `0x545e20` (the DirectSound driver's `initialize`, `sound_driver +0x04`), the driver wrappers
  `0x546f90` / `0x546fa0` / `0x546fe0` / `0x548380` / `0x5483d0` / `0x548410` / `0x548450` /
  `0x548470` / `0x5484d0` / `0x5482a0`, the channel parameter procs `0x54ce50` / `0x54cf80`, the
  Ogg callback thunks `0x544d50` / `0x544da0` / `0x544dc0` / `0x544de0`, the ADPCM block decoders
  `0x54e920` / `0x54ea60`, and the small EAX vtable stubs (`0x54ef10`, `0x54ef20`, `0x54f6e0`,
  `0x54f720`, `0x54ff30`, `0x54ff70`, `0x551180`, `0x5511c0`, `0x551240`, `0x551250`,
  `0x551260`, `0x54edf0`, `0x54ee30`) are code in this range that Ghidra never made functions of.
  They are called from the rewritten code by address and described in `types/sound.h`. The driver
  methods and EAX vtable stubs are now rewritten (see "Cleanup pass 1" at the end); `0x54dc10`
  was added earlier (it is the looping track location proc).
- `0x00746124` is now known to be the count of hardware-mixed 3D buffers (incremented by
  `sound_channel_create`), zeroed by `sound_driver_initialize` (`0x545e20`, cleanup pass 1).
- `sound_class_definition` fields `+0x09`, `+0x0e`, `+0x14`, `+0x20`, `+0x24` are never read here;
  `discard_on_cache_miss` (`+0x0c`) is named by behaviour only.
- `0x006b7020` (pauses sound together with `current_game_engine`) and `0x00722b58` /
  `0x00722b5c` (weapon-fire channels keep streaming; dialog channels go head-relative) have no
  established names.
- `sound_observer_camera` covers only the 0x38 bytes this module reads; the owning observer code
  (`src/game`) has no struct for the 0x29c-byte row yet.
- `FUN_0053ed60` (the underwater test in `sound_update_listener`) is outside this module; whether
  it also takes `EBX` is not settled.
- `sound_driver_end_frame` builds its debug text into a stack buffer that nothing reads (the
  release build compiled the draw call out).
- Other modules still use pre-review names or signatures for sound symbols and globals (see the
  list at the end of this file).

## Misattributed entries in `out/phase4/sound_functions.md`

| Address | Listed as | What it is |
|---|---|---|
| `0x545b70` | `shell_get_command_line_argument` | the tail of `0x545a30` (same frame), folded into `sound_directsound_probe_channel_pools.c` |
| `0x54e200` | `sound_stop_all` | the tail of `0x54deb0` (shared `LAB_0054e2c8`, 0 callers), part of `sound_update_looping_gain.c`; the real `sound_stop_all` is `0x54adb0` |
| `0x546a60` | `game_sound_dispose` | the DirectSound driver's `dispose` (`sound_driver_dispose`) |
| `0x543a90` | `chimera__revert` | clears `SoundLooping.runtime_scripting_sound` back-references (`game_sound_revert_scripting_sounds`) |

## Phase-4 review: what changed

The review compared all 133 files with their disassembly. The main classes of error it removed:

- **Register arguments guessed wrongly or dropped** (most of the module): e.g.
  `sound_looping_set_state` (owner in `EAX`, definition in `ECX`, every callee argument),
  `sound_schedule_gain_fade` callers (the sound to fade in is `EBX`), `sound_start_at_object_marker`
  (`ESI` object, `ECX`/`EAX` node-space point and forward), `sound_decode_dispatch`,
  `sound_effects_object_detect_mode` / `_initialize_channel`, `sound_channel_type_flags_match` (`DX`),
  `sound_location_distance*` (listener on the stack), `sound_cache_touch` (`EDI` permutation) in five
  callers, `data_new` element size (`EBX`).
- **Wrong struct field or global**: `sound_location +0x24` is a velocity (was "up");
  `0x006ac6d0` is an array and `0x00746140` a pointer (both inverted); `sound_stop_all` clears the
  looping-sound table; `sound_dispose` clears `data_array.valid`; EAX2/EAX3 listener properties
  go through `channel_property_sets[0]`; EAX3 channel property 9 is the occlusion; EAX1
  `apply_listener` reads the `SoundEnvironment`.
- **Lost x87 arithmetic**: `2000·log10(g) + max` offsets in the millibel helpers and EAX
  REFLECTIONS / REVERB, the fade power curve (`t^(1/2.5)`), the 8.96 ms/unit start delay, the
  detail-sound period `× 1000 + longest permutation`, the frequency clamp `[188, 191983]`.
- **Control flow**: the fade-out-and-stop wait loop (`datum_next(-1)` every pass, inverted
  pause test), the channel queue state machine, the stream update (cursors come from
  `GetCurrentPosition`), `sound_channel_fill_pcm_data` (advances by what the reader produced), the
  dialog channel takeover returning the channel index, the int16 `== 0xffff` test that never fired.
- **Types**: `sound_channel_candidate_list`, `sound_object_marker_data`, `sound_placement`,
  `sound_observer_camera`, `directsound_listener_cache`, `win32_dsbcaps`, the IKsPropertySet and
  DirectSound method typedefs and the ADPCM decoder proc were folded into `types/sound.h`.

Renamed in the review (all registered in `symbols/agent_phase4_sound.txt`):

| Address | Earlier name | Name | Why |
|---|---|---|---|
| `0x543d80` | `sound_start_from_parameter_block` | `sound_start_at_location` | starts an ownerless sound at a copied placement |
| `0x543dd0` | `sound_start_trampoline` | `sound_start_unspatialized` | builds a type-0 location |
| `0x543e10` | `sound_refresh_scripted_sound` | `sound_impulse_start` | hs `sound_impulse_start <sound> <object> <real>` |
| `0x543fc0` | `sound_scripted_ticks_remaining` | `sound_impulse_time` | hs `sound_impulse_time` |
| `0x544000` | `sound_looping_preload_tracks` | `sound_looping_predict` | hs `sound_looping_predict` |
| `0x544090` | `sound_looping_start_scripted` | `sound_looping_start` | hs `sound_looping_start <looping_sound> <object> <real>` |
| `0x544120` | `looping_sound_object_detach` | `sound_looping_stop` | hs `sound_looping_stop` |
| `0x544180` | `looping_sound_object_set_gain` | `sound_looping_set_scale` | hs `sound_looping_set_scale` |
| `0x544200` | `looping_sound_object_set_alternate` | `sound_looping_set_alternate` | hs `sound_looping_set_alternate` |
| `0x5495f0` | `sound_resume` | `sound_fade_out_and_stop_all` | fades all sounds out over 0.3 s, then stops everything |
| `0x549ee0` | `sound_schedule_initial_fade_in` | `sound_impulse_fade_out` | fades one impulse out (hs `sound_impulse_stop` calls it) |
| `0x54ec40` | `sound_effect_object_release_property_set` | `sound_eax1_effect_shutdown` | EAX1 vtable slot 0 |
| `0x54ec60` | `sound_effect_object_query_listener_support` | `sound_eax1_effect_initialize` | EAX1 vtable slot 1 |
| `0x54ed50` | `sound_eax_apply_channel_obstruction` | `sound_eax1_effect_apply_listener` | EAX1 vtable slot 7 |
| `0x54dc10` | (no function) | `sound_looping_track_location_proc` | location proc of looping track sounds |

## Follow-ups outside this module

These declarations in other modules disagree with the verified sound code; they were left
untouched here:

- `src/effects/player_effect_apply_at_object.c` (`0x456900`): the tag reference is a stack
  argument and the local player index is forced to 0 inside; it is not `EAX` / `DX`.
- `src/objects/object_type_definitions_notify_0x58.c` (`0x4f4480`): forwards two stack words to
  every `notify_58(object, a, b)`; the rewrite drops them.
- `src/cache/sound_cache_decode_permutation.c` declares `FUN_0054e830(void *, uint32_t)`; the
  real call is `sound_decode_dispatch(ECX channels, EBX destination, source, size)`.
- `src/interface/main_menu_on_shown.c` calls `looping_sound_object_detach` (now
  `sound_looping_stop`), `src/objects/object_create_attachments.c` declares
  `looping_sound_new(int16_t)` (really `EAX` object, `EDI` definition, `ECX` marker name, stack
  function index).
- `src/game/camera_observer_get_target_angles.c` treats `0x006ac6d0` as a pointer (it is an array).
- Different names for sound-owned globals elsewhere: `0x0072520c` (`frame_watchdog_time` in
  `src/cache`), `0x007461a0` (`looping_sound_data` in `src/objects`, which is this module's name
  for `0x00724a50`), `0x00721f24` (`user_profile_signin_state` in `src/game`), `0x00746140`
  (`sound_something_00746140` in `src/game`), `0x007252e0` / `0x00746120` (in `src/interface`).

## Functions and rewrite confidence

"Name conf" is the header's name confidence, "Rewrite conf" its rewrite confidence after the
review, "UNSURE" the number of remaining `UNSURE` markers in the rewritten part.

| Address | Function | Size | Name conf | Rewrite conf | UNSURE |
|---|---|---|---|---|---|
| `0x543a30` | `game_sound_initialize` | 88 | 0.90 | 0.85 | 0 |
| `0x543a90` | `game_sound_revert_scripting_sounds` | 151 | 0.40 | 0.85 | 0 |
| `0x543b30` | `game_sound_reconcile_scripting_state` | 233 | 0.35 | 0.85 | 1 |
| `0x543c20` | `looping_sound_new` | 185 | 0.50 | 0.85 | 0 |
| `0x543ce0` | `sound_start_at_object_marker` | 156 | 0.60 | 0.85 | 0 |
| `0x543d80` | `sound_start_at_location` | 71 | 0.55 | 0.85 | 0 |
| `0x543dd0` | `sound_start_unspatialized` | 51 | 0.55 | 0.90 | 0 |
| `0x543e10` | `sound_impulse_start` | 427 | 0.70 | 0.85 | 2 |
| `0x543fc0` | `sound_impulse_time` | 56 | 0.60 | 0.90 | 0 |
| `0x544000` | `sound_looping_predict` | 130 | 0.80 | 0.85 | 0 |
| `0x544090` | `sound_looping_start` | 135 | 0.70 | 0.90 | 0 |
| `0x544120` | `sound_looping_stop` | 88 | 0.80 | 0.85 | 0 |
| `0x544180` | `sound_looping_set_scale` | 114 | 0.80 | 0.85 | 0 |
| `0x544200` | `sound_looping_set_alternate` | 75 | 0.80 | 0.85 | 0 |
| `0x544250` | `sound_looping_start_ambient` | 64 | 0.40 | 0.90 | 0 |
| `0x544290` | `game_looping_sound_touch_if_valid` | 160 | 0.35 | 0.85 | 0 |
| `0x544330` | `game_looping_sound_update` | 655 | 0.45 | 0.80 | 0 |
| `0x5445c0` | `game_sound_update` | 759 | 0.50 | 0.75 | 3 |
| `0x5448c0` | `sound_location_object_marker` | 183 | 0.60 | 0.85 | 0 |
| `0x544980` | `sound_build_cluster_range_bitmap` | 274 | 0.50 | 0.85 | 2 |
| `0x544aa0` | `sound_compute_obstruction_occlusion` | 360 | 0.40 | 0.80 | 1 |
| `0x544c10` | `sound_looping_definition_has_music_loop` | 87 | 0.40 | 0.85 | 0 |
| `0x544c70` | `game_sound_stop_loops_conflicting_with_music` | 223 | 0.35 | 0.85 | 0 |
| `0x544e00` | `sound_ogg_seek_callback` | 176 | 0.55 | 0.90 | 0 |
| `0x544eb0` | `sound_ogg_stream_open` | 192 | 0.50 | 0.85 | 1 |
| `0x544f70` | `sound_ogg_error_to_string` | 407 | 0.75 | 0.85 | 1 |
| `0x5451d0` | `sound_ogg_stream_read` | 340 | 0.60 | 0.85 | 1 |
| `0x545330` | `sound_class_update_gain_fade` | 88 | 0.50 | 0.85 | 0 |
| `0x545390` | `sound_class_set_gain_by_name` | 140 | 0.50 | 0.85 | 0 |
| `0x545420` | `sound_class_set_muted_by_name` | 63 | 0.40 | 0.85 | 0 |
| `0x545460` | `sound_definition_maximum_distance` | 54 | 0.30 | 0.85 | 0 |
| `0x5454a0` | `sound_permutation_pick_for_pitch` | 228 | 0.55 | 0.85 | 0 |
| `0x545590` | `sound_permutation_pick_random` | 384 | 0.50 | 0.85 | 1 |
| `0x545710` | `sound_linear_gain_to_attenuation` | 71 | 0.50 | 0.85 | 0 |
| `0x545760` | `sound_stream_decoder_close_slot` | 252 | 0.40 | 0.85 | 1 |
| `0x545860` | `sound_pcm_buffer_read` | 186 | 0.50 | 0.85 | 1 |
| `0x545920` | `sound_ogg_buffer_fill` | 264 | 0.50 | 0.85 | 0 |
| `0x545a30` | `sound_directsound_probe_channel_pools` | 320 | 0.50 | 0.85 | 1 |
| `0x546760` | `sound_channel_create` | 763 | 0.60 | 0.80 | 0 |
| `0x546a60` | `sound_driver_dispose` | 220 | 0.60 | 0.85 | 0 |
| `0x546b40` | `sound_update_streaming_channels` | 59 | 0.35 | 0.85 | 0 |
| `0x546b80` | `sound_driver_end_frame` | 1037 | 0.50 | 0.80 | 0 |
| `0x547070` | `sound_listener_update` | 595 | 0.55 | 0.85 | 0 |
| `0x5472d0` | `sound_channel_set_spatial` | 728 | 0.40 | 0.90 | 1 |
| `0x5475b0` | `sound_channel_set_parameters` | 723 | 0.40 | 0.85 | 0 |
| `0x547890` | `sound_channel_refresh_cursor` | 46 | 0.40 | 0.95 | 0 |
| `0x5478c0` | `sound_channel_stream_update` | 307 | 0.40 | 0.85 | 0 |
| `0x547a00` | `sound_channel_lock_and_fill` | 172 | 0.50 | 0.85 | 0 |
| `0x547ab0` | `sound_channel_fill_pcm_data` | 337 | 0.50 | 0.80 | 0 |
| `0x547c10` | `sound_channel_restore_buffer` | 97 | 0.35 | 0.90 | 0 |
| `0x547c80` | `sound_channel_queue_source` | 721 | 0.45 | 0.80 | 0 |
| `0x547f60` | `sound_channel_reset` | 130 | 0.35 | 0.85 | 1 |
| `0x547ff0` | `sound_channel_claim_if_finished` | 84 | 0.40 | 0.85 | 0 |
| `0x548050` | `sound_channel_check_loop_boundary` | 146 | 0.40 | 0.85 | 0 |
| `0x5480f0` | `sound_driver_set_quality` | 122 | 0.50 | 0.90 | 0 |
| `0x548170` | `sound_pause` | 46 | 0.40 | 0.85 | 0 |
| `0x548200` | `sound_driver_set_eax_enabled` | 148 | 0.40 | 0.85 | 0 |
| `0x5482e0` | `sound_channel_bind_hardware` | 150 | 0.35 | 0.85 | 0 |
| `0x548520` | `sound_channel_type_flags_match` | 104 | 0.40 | 0.85 | 0 |
| `0x548590` | `sound_set_master_gain` | 225 | 0.40 | 0.85 | 0 |
| `0x548680` | `sound_set_music_gain` | 300 | 0.60 | 0.85 | 0 |
| `0x5487b0` | `sound_set_effects_gain` | 2876 | 0.90 | 0.85 | 0 |
| `0x5492f0` | `sound_initialize` | 425 | 0.90 | 0.90 | 3 |
| `0x5494a0` | `sound_reopen_device` | 319 | 0.40 | 0.85 | 0 |
| `0x5495f0` | `sound_fade_out_and_stop_all` | 355 | 0.50 | 0.80 | 0 |
| `0x549760` | `sound_dispose` | 166 | 0.55 | 0.90 | 0 |
| `0x549810` | `sound_update` | 336 | 0.50 | 0.85 | 1 |
| `0x549960` | `sound_idle_update` | 151 | 0.40 | 0.85 | 0 |
| `0x549af0` | `sound_play_new` | 1003 | 0.60 | 0.75 | 0 |
| `0x549ee0` | `sound_impulse_fade_out` | 110 | 0.50 | 0.90 | 0 |
| `0x549f50` | `sound_looping_datum_touch` | 76 | 0.35 | 0.85 | 0 |
| `0x549fa0` | `sound_looping_set_state` | 3548 | 0.50 | 0.70 | 0 |
| `0x54adb0` | `sound_stop_all` | 166 | 0.70 | 0.95 | 0 |
| `0x54ae60` | `sound_update_clock` | 88 | 0.70 | 0.85 | 0 |
| `0x54aec0` | `sound_compute_random_pitch` | 73 | 0.55 | 0.85 | 0 |
| `0x54af10` | `sound_definition_has_audible_permutations` | 67 | 0.50 | 0.85 | 0 |
| `0x54af60` | `sound_schedule_gain_fade` | 233 | 0.50 | 0.85 | 0 |
| `0x54b050` | `sound_definition_check_promotion` | 176 | 0.50 | 0.85 | 0 |
| `0x54b100` | `sound_compute_class_gain` | 116 | 0.50 | 0.85 | 0 |
| `0x54b180` | `sound_instance_stop` | 2003 | 0.60 | 0.85 | 0 |
| `0x54b970` | `sound_update_listener` | 431 | 0.70 | 0.85 | 1 |
| `0x54bb20` | `sound_location_check_audibility` | 171 | 0.50 | 0.90 | 0 |
| `0x54bbd0` | `sound_location_distance_squared` | 115 | 0.50 | 0.85 | 1 |
| `0x54bc50` | `sound_location_distance` | 119 | 0.50 | 0.85 | 1 |
| `0x54bcd0` | `sound_instance_invoke_location_proc` | 133 | 0.50 | 0.85 | 0 |
| `0x54bd60` | `sound_update_range_and_ducking` | 692 | 0.50 | 0.90 | 0 |
| `0x54c020` | `sound_assign_channels` | 426 | 0.50 | 0.85 | 0 |
| `0x54c1d0` | `sound_build_channel_candidates` | 279 | 0.45 | 0.85 | 0 |
| `0x54c2f0` | `sound_pick_channel_for_instance` | 322 | 0.50 | 0.85 | 0 |
| `0x54c440` | `sound_find_lowest_priority_channel` | 410 | 0.50 | 0.85 | 0 |
| `0x54c5e0` | `sound_pick_replaceable_channel` | 207 | 0.50 | 0.90 | 0 |
| `0x54c6b0` | `sound_compare_priority` | 146 | 0.50 | 0.90 | 0 |
| `0x54c750` | `sound_update_instance_gain` | 426 | 0.50 | 0.85 | 0 |
| `0x54c900` | `sound_update_active_instances` | 1058 | 0.50 | 0.75 | 0 |
| `0x54cd30` | `sound_channel_set_next_permutation` | 107 | 0.45 | 0.85 | 0 |
| `0x54cda0` | `sound_linear_gain_to_millibels_clamped` | 65 | 0.40 | 0.85 | 0 |
| `0x54cdf0` | `sound_evaluate_volume_curve` | 83 | 0.50 | 0.85 | 0 |
| `0x54d020` | `sound_channel_release_detail_buffers` | 176 | 0.50 | 0.85 | 0 |
| `0x54d0d0` | `sound_channel_release_permutations` | 106 | 0.50 | 0.85 | 0 |
| `0x54d140` | `sound_looping_state_new` | 293 | 0.50 | 0.75 | 0 |
| `0x54d270` | `sound_update_looping_states` | 1886 | 0.55 | 0.80 | 0 |
| `0x54d9f0` | `sound_looping_create_detail_sound` | 541 | 0.50 | 0.85 | 0 |
| `0x54dc10` | `sound_looping_track_location_proc` | 91 | 0.60 | 0.90 | 0 |
| `0x54dc70` | `sound_looping_detail_location_proc` | 275 | 0.55 | 0.85 | 0 |
| `0x54dd90` | `sound_instance_queue_definition_switch` | 39 | 0.55 | 0.85 | 0 |
| `0x54ddc0` | `sound_instance_apply_pending_definition_switch` | 239 | 0.45 | 0.90 | 0 |
| `0x54deb0` | `sound_update_looping_gain` | 848 | 0.50 | 0.75 | 0 |
| `0x54e3c0` | `sound_evaluate_fade_gain` | 269 | 0.55 | 0.85 | 0 |
| `0x54e4d0` | `sound_random_detail_direction` | 251 | 0.50 | 0.85 | 0 |
| `0x54e5d0` | `sound_looping_find_by_owner` | 130 | 0.50 | 0.85 | 0 |
| `0x54e660` | `sound_clamp_gain_by_ratio` | 103 | 0.40 | 0.85 | 0 |
| `0x54e6d0` | `render_debug_sound` | 97 | 0.60 | 0.85 | 1 |
| `0x54e740` | `sound_looping_check_audibility_gate` | 178 | 0.40 | 0.80 | 0 |
| `0x54e830` | `sound_decode_dispatch` | 137 | 0.50 | 0.85 | 0 |
| `0x54e8c0` | `sound_adpcm_decode_sample` | 92 | 0.40 | 0.85 | 1 |
| `0x54ec40` | `sound_eax1_effect_shutdown` | 25 | 0.70 | 0.95 | 0 |
| `0x54ec60` | `sound_eax1_effect_initialize` | 238 | 0.70 | 0.90 | 0 |
| `0x54ed50` | `sound_eax1_effect_apply_listener` | 148 | 0.70 | 0.90 | 0 |
| `0x54ee70` | `sound_gain_to_directsound_volume` | 71 | 0.50 | 0.95 | 0 |
| `0x54eec0` | `sound_gain_to_millibels` | 66 | 0.60 | 0.85 | 0 |
| `0x54ef30` | `sound_eax20_effect_shutdown` | 825 | 0.55 | 0.85 | 0 |
| `0x54f270` | `sound_eax20_effect_initialize` | 1126 | 0.55 | 0.85 | 0 |
| `0x54fa80` | `sound_eax20_effect_apply_listener` | 1188 | 0.55 | 0.85 | 0 |
| `0x54fff0` | `sound_eax30_effect_shutdown` | 882 | 0.50 | 0.85 | 0 |
| `0x550370` | `sound_eax30_effect_initialize` | 1300 | 0.55 | 0.85 | 0 |
| `0x550890` | `sound_eax30_effect_apply_channel` | 884 | 0.60 | 0.85 | 0 |
| `0x550c10` | `sound_reverb_size_scale` | 91 | 0.50 | 0.85 | 0 |
| `0x550c70` | `sound_eax30_effect_apply_listener` | 1284 | 0.55 | 0.85 | 0 |
| `0x551270` | `sound_effects_object_detect_mode` | 406 | 0.70 | 0.85 | 0 |
| `0x551420` | `sound_effects_object_shutdown` | 49 | 0.60 | 0.85 | 0 |
| `0x551460` | `sound_effects_object_initialize_channel` | 19 | 0.35 | 0.90 | 0 |
| `0x551480` | `sound_effects_object_apply_all_channels` | 65 | 0.60 | 0.85 | 0 |
| `0x5514d0` | `sound_effects_object_reinitialize` | 321 | 0.60 | 0.85 | 0 |

Names that differ from `symbols/functions.txt` are registered in `symbols/agent_phase4_sound.txt`
(merged by `tools/merge_symbols.py`).

## Cleanup pass 1: functions Ghidra never created

Real functions reached only through vtables, dispatch tables or call sites, found by the phase-4
types agents, created in the Ghidra project as `missed_XXXXXX` and rewritten here under Blam
names (symbols in `symbols/agent_phase4_missed.txt`). Register conventions were taken from
objdump of the function and of the table or call site that reaches it.

| Address | Function | Size | Name conf. | Rewrite conf. | UNSURE | Note |
|---|---|---|---|---|---|---|
| `0x545e20` | `sound_driver_initialize` | 2283 | 0.8 | 0.75 | 1 | written in the cleanup review (the rewriter had left it unwritten); EAX channel-budget search rebuilt from objdump and the five jump tables |
| `0x546f90` | `sound_driver_begin_frame` | 8 | 0.6 | 0.95 | 0 |  |
| `0x546fa0` | `sound_driver_stop_all` | 51 | 0.6 | 0.9 | 0 |  |
| `0x546fe0` | `sound_driver_set_paused` | 131 | 0.55 | 0.85 | 0 |  |
| `0x5482a0` | `sound_driver_eax_available` | 53 | 0.6 | 0.95 | 0 |  |
| `0x548380` | `sound_driver_channel_play` | 69 | 0.6 | 0.85 | 0 |  |
| `0x5483d0` | `sound_driver_channel_continue` | 59 | 0.6 | 0.85 | 0 |  |
| `0x548410` | `sound_driver_channel_stop` | 54 | 0.6 | 0.9 | 0 |  |
| `0x548450` | `sound_driver_channel_get_state` | 32 | 0.6 | 0.9 | 0 |  |
| `0x548470` | `sound_driver_channel_set_spatial` | 88 | 0.6 | 0.85 | 0 |  |
| `0x5484d0` | `sound_driver_channel_set_parameters` | 66 | 0.6 | 0.85 | 0 | review: extern of sound_channel_set_parameters reordered to its definition (channel, parameters, update) |
| `0x54edf0` | `sound_eax1_effect_set_environment_index` | 53 | 0.5 | 0.85 | 0 |  |
| `0x54ee30` | `sound_eax1_effect_set_room_gain` | 53 | 0.5 | 0.85 | 0 |  |
| `0x54ef10` | `sound_effect_object_listener_supported` | 3 | 0.6 | 0.95 | 0 |  |
| `0x54ef20` | `sound_effect_object_channel_supported` | 4 | 0.6 | 0.95 | 0 |  |
| `0x54f6e0` | `sound_eax_effect_initialize_channel` | 63 | 0.55 | 0.85 | 0 |  |
| `0x54f720` | `sound_eax20_effect_apply_channel` | 860 | 0.55 | 0.8 | 0 | review: underwater gain read from 0x0069ff24 (the EAX2 copy), not 0x0069ff28 |
| `0x54ff30` | `sound_eax20_effect_set_environment_index` | 49 | 0.5 | 0.85 | 0 |  |
| `0x54ff70` | `sound_eax20_effect_set_room_gain` | 117 | 0.5 | 0.8 | 0 |  |
| `0x551180` | `sound_eax30_effect_set_environment_index` | 49 | 0.5 | 0.85 | 0 |  |
| `0x5511c0` | `sound_eax30_effect_set_room_gain` | 117 | 0.5 | 0.8 | 0 |  |
| `0x551240` | `sound_eax1_effect_initialize_channel` | 8 | 0.6 | 0.95 | 0 |  |
| `0x551250` | `sound_eax1_effect_channel_supported` | 3 | 0.6 | 0.95 | 0 |  |
| `0x551260` | `sound_eax1_effect_apply_channel` | 3 | 0.6 | 0.95 | 0 |  |

Gate: `python tools/build_check.py sound` clean after the cleanup review.
