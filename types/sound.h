// Blam sound module (halo.exe 1.0.10 retail, 0x543a30..0x5514d0, 134 Ghidra functions).
// Four layers live in this address range, each with its own records:
//
//   game sound       0x543a30..0x544c70   game-state looping sounds bound to objects, scripts
//                                         and the BSP background sound ("object looping sounds")
//   ogg / pcm feed   0x544e00..0x545920   libvorbisfile stream state and PCM copy into a channel
//   sound            0x545330..0x5454a0,  sound classes, gain sliders, the "sounds" and
//                    0x548590..0x54e8c0   "looping sounds" datum tables, logical channels, listener
//   directsound      0x545a30..0x548520   the DirectSound driver: its vtable, hardware channels,
//                                         3D listener and buffer parameter caches
//   eax effects      0x54ec40..0x5514d0   the EAX1/EAX2/EAX3 sound effect objects
//
// Offsets in comments are byte offsets from the struct base. Where the binary itself carries a
// layout it is preferred over the decompiler and the fact is called out:
//
//   - Every tag side layout already exists in types/tags.h and is reused, never redefined. The
//     arithmetic in this module agrees with it everywhere: Sound 0xa4 (sound_class 0x04,
//     sample_rate 0x06, minimum/maximum_distance 0x08/0x0c, random_pitch_bounds 0x14,
//     cone angles 0x1c/0x20, outer_cone_gain 0x24, random_gain_modifier 0x28,
//     maximum_bend_per_second 0x2c, zero_* 0x3c..0x44, one_* 0x54..0x5c, channel_count 0x6c,
//     format 0x6e, promotion_sound.tag_id 0x7c, promotion_count 0x80,
//     longest_permutation_length 0x84, promotion_counter 0x88, promotion_time 0x8c,
//     scripting_time 0x90, scripting_sound 0x94, pitch_ranges 0x98), SoundPitchRange 0x48
//     (bend_bounds 0x24/0x28, actual_permutation_count 0x2c, playback_rate 0x30,
//     permutation_flags 0x34, last/discarded_permutation_index 0x38/0x3a, permutations 0x3c),
//     SoundPermutation 0x7c (gain 0x24, format 0x28, next_permutation_index 0x2a; the runtime
//     words samples_pointer 0x2c and _pad_30 0x30 hold the sound cache datum_index and the
//     cached sample pointer, buffer_size 0x38, samples.size 0x40, mouth_data 0x54/0x60),
//     SoundLooping 0x54 (runtime_scripting_sound 0x1c, maximum_distance 0x20,
//     continuous_damage_effect.tag_id 0x38, tracks 0x3c, detail_sounds 0x48),
//     SoundLoopingTrack 0xa0 (start/loop/end/alternate_loop/alternate_end tag_id at
//     0x3c/0x4c/0x5c/0x8c/0x9c), SoundLoopingDetail 0x68 and SoundEnvironment 0x48.
//
//   - sound_class_definition is read straight out of .data at 0x0069eae0: 51 rows of 0x2c
//     bytes followed by the 51 class name pointers at 0x0069f3a8 (0x0069eae0 + 51*0x2c = 0x0069f3a4).
//
//   - sound_driver is read straight out of .data at 0x0069f4c8 (the only non-null entry of the
//     driver table at 0x0069f508): a 16-bit type word then 15 function pointers, 0x40 bytes.
//     Its slots were matched to code by disassembling each target (most are not Ghidra functions).
//
//   - sound_effect_object_vtable is read out of .rdata at 0x00671d04 (EAX2), 0x00671d28 (EAX3)
//     and 0x00671d4c (EAX1), nine slots each. operator_new(0xe8) / operator_new(0x1c) in
//     0x551270 and 0x5514d0 fix the two object sizes.
//
//   - The DirectSound structures (sound_wave_format, sound_buffer_description) are built field by
//     field on the stack by 0x545a30 and 0x546760; they are the Windows WAVEFORMATEX and
//     DSBUFFERDESC layouts and the dwSize fields (0x24) confirm them.
//
// Array bounds that are not carried by the binary are inferred and marked. The three per-channel
// arrays (0x00724a60 stride 0x18, 0x007252e4 stride 4, 0x00725430 stride 0x678) each fill the
// gap to the next referenced global with exactly 81 entries, so k_maximum_sound_channels is 81.
// The driver probe in the unnamed driver initialize (0x545e20) caps 3D channels at 0x33, which
// matches the 51 property-set slots of the EAX objects.
#pragma pack(push, 1)
typedef unsigned char uint8_t; typedef signed char int8_t; typedef unsigned short uint16_t; typedef short int16_t;
typedef unsigned int uint32_t; typedef int int32_t;

// ---------------------------------------------------------------------------
// constants
// ---------------------------------------------------------------------------
typedef enum sound_constants {
    k_maximum_sound_classes = 51,           // every sound_class table loops 0x33 times
    k_maximum_sounds = 0x200,               // data_new("sounds", 0x200) in sound_initialize
    k_maximum_looping_sounds = 0x80,        // data_new("looping sounds", 0x80)
    k_maximum_game_looping_sounds = 0x400,  // game_state_new("object looping sounds", 0x400)
    k_maximum_sound_channels = 81,          // UNSURE: inferred from three global array gaps
    k_maximum_eax_channels = 51,            // 0x33 property sets, 0x33 3D channel cap
    k_sound_channel_type_count = 4,         // mono 3D, mono, stereo, 44k stereo
    k_maximum_looping_sound_tracks = 4,     // looping_sound.track_sounds, 0xd4 + 4*4 == 0xe4
    k_maximum_looping_sound_details = 32,   // looping_sound.detail_next_time, 0x54 + 32*4 == 0xd4
    k_maximum_sound_callback_data = 0x30,   // sound.callback_data, 0x54..0x84
    k_sound_update_interval_ms = 0x20,      // sound_update runs its body when 0x20 ms have passed
    k_game_sound_update_interval_ms = 0x21, // game_sound_update full pass at >= 0x21 ms
    k_sound_cluster_bitmap_words = 16,      // UNSURE: 0x00746160..0x007461a0 gap
    k_sound_minimum_volume = -10000,        // DSBVOLUME_MIN, every gain to volume helper
    k_ogg_vorbis_file_size = 0x2d0          // ov_clear then 0xb4 dwords zeroed in 0x545760
} sound_constants;

typedef enum sound_signatures {
    k_sound_driver_type_directsound = 0     // sound_driver.type, compared to the parameter word
} sound_signatures;

// ---------------------------------------------------------------------------
// sound_location  (copied as 16 dwords into sound+0x14 by sound_play_new 0x549af0 and into
// looping_sound+0x0c by sound_looping_set_state 0x549fa0; filled on the stack by 0x543ce0,
// 0x544330, 0x54b970, 0x54d270; written by the location callbacks 0x5448c0 and 0x54dc70)
// ---------------------------------------------------------------------------
typedef enum sound_location_type {
    _sound_location_none = 0,               // unspatialized, distance 0 (0x54bbd0)
    _sound_location_absolute = 1,           // world position, transformed by the listener matrix
    _sound_location_listener_relative = 2   // position is already a listener space offset
} sound_location_type;

typedef struct sound_location {
    int16_t type;              // 0x00 sound_location_type
    int16_t unknown_02;        // 0x02 never read; a short store pads it
    float scale;               // 0x04 lerp factor between Sound.zero_* and Sound.one_* modifiers
    float gain;                // 0x08 1.0 from every builder; detail gain for detail sounds
    Point3D position;          // 0x0c
    Vector3D forward;          // 0x18 matrix4x3_transform_normal result
    Vector3D velocity;         // 0x24 world units per tick: object_get_root_object_velocities
                               //      (0x4f6aa0) in 0x544330 / 0x5448c0, zero for detail sounds
                               //      (0x54dc70); 0x54c900 turns it into the doppler velocity
    int32_t leaf_index;        // 0x30 object_get_root_location (0x4f6b10) pair, written by 0x5448c0
    int16_t cluster_index;     // 0x34 -1 skips the obstruction test in 0x544aa0
    int16_t unknown_36;        // 0x36 high half of the leaf reference dword, never read
    float obstruction;         // 0x38 0.6 / 0.45 / 0.0 from 0x544aa0; printed by render_debug_sound
    float occlusion;           // 0x3c 1.0 disables the channel (0x54bb20); printed by render_debug_sound
} sound_location;              // size 0x40

// sound.callback_data of object-marker sounds (0x1c bytes, built on the stack by
// sound_start_at_object_marker 0x543ce0, read back by sound_location_object_marker 0x5448c0)
typedef struct sound_object_marker_data {
    int16_t unknown_00;        // 0x00 never written (stack garbage copied along)
    int16_t node_index;        // 0x02 object node, -1 means node 0
    Point3D position;          // 0x04 node space, through matrix4x3_transform_point
    Vector3D forward;          // 0x10 node space, through matrix4x3_transform_normal
} sound_object_marker_data;    // size 0x1c

// the 11 dwords sound_start_at_location (0x543d80) copies into sound_location+0x0c
typedef struct sound_placement {
    Point3D position;          // 0x00 -> location 0x0c
    Vector3D forward;          // 0x0c -> location 0x18
    Vector3D velocity;         // 0x18 -> location 0x24
    int32_t leaf_index;        // 0x24 -> location 0x30
    int16_t cluster_index;     // 0x28 -> location 0x34
    int16_t unknown_2a;        // 0x2a -> location 0x36
} sound_placement;             // size 0x2c

// callback stored in sound.location_proc: (owner, callback_data, location) -> success.
// Known targets: 0x5448c0 (object marker), 0x54dc10 (looping track part: copies the owner
// looping_sound location) and 0x54dc70 (looping detail sound: owner location plus an offset).
typedef uint8_t (*sound_location_proc)(datum_index owner, void *callback_data, sound_location *location);

// ---------------------------------------------------------------------------
// sound_class_definition  (.data 0x0069eae0, 51 rows, see header note)
// Read by 0x545460, 0x54af10, 0x54bcd0, 0x54c1d0, 0x54c2f0, 0x54c5e0, 0x54c6b0, 0x54c750,
// 0x54c900, 0x54d9f0, 0x54deb0, 0x54e740; muted is written by the gain setters.
// ---------------------------------------------------------------------------
typedef struct sound_class_definition {
    int16_t maximum_sounds_per_tag;      // 0x00 3/4/2, candidate list limit in 0x54c1d0
    int16_t maximum_sounds_per_object;   // 0x02 2/1/4, second candidate limit in 0x54c1d0
    int32_t minimum_replace_time;        // 0x04 ms a channel must play before 0x54c5e0 steals it
    uint8_t dialog;                      // 0x08 1 for unit_dialog and the scripted_dialog classes:
                                         //      mouth_data lip sync (0x54c900), cache miss retry
                                         //      (0x54bcd0), shared channel per object (0x54c2f0)
    uint8_t unknown_09;                  // 0x09 zero in every row, never read
    int16_t priority;                    // 0x0a 1..6, compared by 0x54c6b0
    int16_t discard_on_cache_miss;       // 0x0c UNSURE name: 0 makes 0x54c020 stop a sound whose
                                         //      samples are not resident at its start time
    int16_t unknown_0e;                  // 0x0e zero in every row, never read
    float eax_value;                     // 0x10 0.5..1.0, eighth float of the channel parameters,
                                         //      lands in directsound_channel.eax_value (0x60)
    float unknown_14;                    // 0x14 0.0 in every row, never read
    float default_minimum_distance;      // 0x18 fallback when Sound.minimum_distance == 0
    float default_maximum_distance;      // 0x1c fallback when Sound.maximum_distance == 0
    float unknown_20;                    // 0x20 0.0 or 1.0, never read in this module
    float unknown_24;                    // 0x24 1.0 in every row, never read in this module
    uint8_t muted;                       // 0x28 set by the gain setters when their slider hits 0
    uint8_t unknown_29[3];               // 0x29 padding
} sound_class_definition;                // size 0x2c

// per-class gain fade, pointed to by the global at 0x00746140 (51 entries)
// sound_class_update_gain_fade 0x545330, sound_class_set_gain_by_name 0x545390,
// sound_compute_class_gain 0x54b100
typedef struct sound_class_gain {
    float target_gain;         // 0x00 clamped [0,1] by 0x545390
    float current_gain;        // 0x04 read by 0x54b100, interpolated toward target by 0x545330
    int16_t fade_ticks;        // 0x08 ticks left; 0 snaps current to target
    int16_t unknown_0a;        // 0x0a padding
} sound_class_gain;            // size 0x0c

// ---------------------------------------------------------------------------
// game_looping_sound  ("object looping sounds", game state datum, 0x400 x 0x34)
// game_sound_initialize 0x543a30, looping_sound_new 0x543c20, 0x543a90, 0x543b30, 0x544090,
// 0x544120..0x544330, game_sound_update 0x5445c0, 0x544c70.
// The looping sound definition keeps a back reference in SoundLooping.runtime_scripting_sound
// (tag +0x1c) while the scripted bit is set.
// ---------------------------------------------------------------------------
typedef enum game_looping_sound_state {
    _game_looping_sound_playing = 0,
    _game_looping_sound_stopping = 1,
    _game_looping_sound_stopped = 2            // looping_sound_new starts here
} game_looping_sound_state;

typedef enum game_looping_sound_flags {
    _game_looping_sound_script_gain_bit = 0x01,     // gain comes from .scale, not an object function
    _game_looping_sound_stop_requested_bit = 0x02,  // set by detach and by the background swap
    _game_looping_sound_stopped_by_music_bit = 0x04,// 0x544c70, the definition stops_music
    _game_looping_sound_alternate_bit = 0x08,       // 0x544200, forwarded to sound_looping_set_state
    _game_looping_sound_scripted_bit = 0x10         // definition runtime_scripting_sound points here
} game_looping_sound_flags;

typedef struct game_looping_sound {
    int16_t identifier;        // 0x00 datum salt
    int16_t state;             // 0x02 game_looping_sound_state
    uint32_t flags;            // 0x04 game_looping_sound_flags
    float scale;               // 0x08 script gain, clamped [0,1] by 0x544180
    datum_index definition_index; // 0x0c SoundLooping tag
    datum_index object_index;  // 0x10 -1 for script and background sounds
    int32_t last_update;       // 0x14 game_sound_globals.update_count at the last pass
    int16_t function_index;    // 0x18 object function_out_values index (object 0x134, valid 0x123), -1 none
    int16_t node_index;        // 0x1a object node matrix index (object nodes, stride 0x34)
    Point3D position;          // 0x1c node space position
    Vector3D forward;          // 0x28 node space direction
} game_looping_sound;          // size 0x34

// game state block registered by game_sound_initialize (crc32 of the 12-byte size)
typedef struct game_sound_globals {
    int32_t update_count;      // 0x00 bumped by each full game_sound_update pass
    datum_index background_sound_index; // 0x04 game_looping_sound for the cluster background sound
    int32_t last_update_time;  // 0x08 QueryPerformanceCounter ms at the last full pass
} game_sound_globals;          // size 0x0c

// ---------------------------------------------------------------------------
// sound  ("sounds", 0x200 x 0xb0, the global at 0x007252c0)
// sound_play_new 0x549af0 and 0x54d9f0 construct it; 0x549ee0, 0x54af60, 0x54b180, 0x54bcd0,
// 0x54bd60, 0x54c020..0x54c750, 0x54c900, 0x54dd90, 0x54ddc0, 0x54deb0, 0x54e3c0 read it.
// ---------------------------------------------------------------------------
typedef enum sound_play_state {
    _sound_play_impulse = 0,        // one shot, gain through 0x54c750
    _sound_play_loop_start = 1,     // looping track start part
    _sound_play_loop = 2,           // looping track loop part
    _sound_play_loop_stopping = 3,  // loop asked to finish its current permutation
    _sound_play_loop_end = 4        // looping track end part / finished
} sound_play_state;

typedef enum sound_flags {
    _sound_delayed_start_bit = 0x01,        // start time pushed out by 0x549af0, location proc skipped
    _sound_channel_requested_bit = 0x02,    // 0x54c020 asked for a channel; holds a cache reference
    _sound_out_of_range_bit = 0x04,         // 0x54bd60 faded it out for distance
    _sound_permutation_pending_bit = 0x08   // a permutation or definition switch is queued
} sound_flags;

typedef enum sound_fade_curve {
    _sound_fade_linear = 0,
    _sound_fade_power = 1                   // FUN_006283c0 with the 2.5 exponent at 0x0069f510
} sound_fade_curve;

typedef struct sound {
    int16_t identifier;        // 0x00 datum salt
    int16_t play_state;        // 0x02 sound_play_state
    uint16_t flags;            // 0x04 sound_flags (byte accesses)
    int16_t listener_index;    // 0x06 0x54bb20 result, stride 0x44 into sound_listeners
    datum_index definition_index; // 0x08 Sound tag
    datum_index owner_index;   // 0x0c object for impulses, looping_sound for detail/track sounds
    sound_location_proc location_proc; // 0x10 called by 0x54bcd0 with owner, callback_data, location
    sound_location location;   // 0x14
    uint8_t callback_data[0x30]; // 0x54 caller bytes (param_5/param_6 of 0x549af0)
    int32_t start_time;        // 0x84 sound clock ms; channel granted when reached (0x54c020)
    float pitch;               // 0x88 random pitch from 0x54aec0 / random_range_real
    int16_t channel_index;     // 0x8c sound_channel slot, -1 while waiting
    int16_t pitch_range_index; // 0x8e sound_permutation_pick_for_pitch
    int16_t permutation_index; // 0x90 sound_permutation_pick_random
    int16_t fade_curve;        // 0x92 sound_fade_curve
    int16_t track_index;       // 0x94 SoundLoopingTrack index, -1 for impulses
    int16_t unknown_96;        // 0x96 never read or written
    datum_index pending_definition_index; // 0x98 0x54dd90 queues a definition switch, -1 none
    float fade_start_gain;     // 0x9c written by 0x54af60
    float fade_end_gain;       // 0xa0 0 means the sound dies when the fade completes
    int32_t fade_start_time;   // 0xa4 sound clock ms
    int32_t fade_end_time;     // 0xa8 == start means no fade
    uint8_t first_person;      // 0xac weapon classes / player dialog; head relative in 0x54c900
    uint8_t unknown_ad[3];     // 0xad padding
} sound;                       // size 0xb0

// ---------------------------------------------------------------------------
// looping_sound  ("looping sounds", 0x80 x 0xe4, the global at 0x00724a50)
// sound_looping_set_state 0x549fa0, 0x549f50, 0x54d140, 0x54d270, 0x54d9f0, 0x54dc70,
// 0x54deb0, 0x54e5d0. A looping_sound not touched in the current frame (update_toggle !=
// the global toggle at 0x00725214) is torn down by 0x54d270.
// ---------------------------------------------------------------------------
typedef struct looping_sound {
    int16_t identifier;        // 0x00 datum salt
    int16_t unknown_02;        // 0x02 never read or written in this module
    datum_index definition_index; // 0x04 SoundLooping tag
    int32_t owner;             // 0x08 caller handle (game_looping_sound index), searched by 0x54e5d0
    sound_location location;   // 0x0c
    uint8_t update_toggle;     // 0x4c copy of the frame toggle, keep alive
    uint8_t alternate;         // 0x4d alternate loop / end selected
    uint8_t finished;          // 0x4e track ran out of permutations
    uint8_t unknown_4f;        // 0x4f padding
    int16_t active_sound_count;// 0x50 sounds created through 0x54d9f0 still alive
    int16_t state;             // 0x52 last state passed to sound_looping_set_state (2 = stopped)
    int32_t detail_next_time[32]; // 0x54 per SoundLoopingDetail, sound clock ms
    datum_index track_sounds[4]; // 0xd4 sound playing each SoundLoopingTrack
} looping_sound;               // size 0xe4

// ---------------------------------------------------------------------------
// sound_channel  (logical channel, 0x00724a60, count at 0x007252b4)
// sound_initialize 0x5492f0 and 0x5494a0 seed it; 0x54b180, 0x54c020..0x54c900, 0x54cd30,
// 0x54d020, 0x54d0d0, 0x54deb0. The same index is the slot passed to the driver.
// ---------------------------------------------------------------------------
typedef struct sound_channel {
    datum_index sound_index;   // 0x00 sound playing here, -1 free
    uint16_t type_flags;       // 0x04 sound_channel_type_flags of its driver channel type
    int16_t unknown_06;        // 0x06 never read
    float play_time;           // 0x08 accumulated time * pitch (0x54d020)
    float current_pitch;       // 0x0c scales pitch range natural_pitch in 0x54deb0
    SoundPermutation *current_permutation; // 0x10 holds a sound cache reference
    SoundPermutation *next_permutation;    // 0x14 queued behind current
} sound_channel;               // size 0x18

// channel candidate lists, a 0x48-byte stack block built by sound_build_channel_candidates
// (0x54c1d0, ESI = the block) and consumed by sound_pick_channel_for_instance (0x54c2f0) and
// sound_instance_apply_pending_definition_switch (0x54ddc0): logical channels already playing
// the same Sound tag, and among those the ones with the same owner, each with the class limit
typedef struct sound_channel_candidate_list {
    int16_t tag_match_count;      // 0x00
    int16_t tag_matches[16];      // 0x02 sound_channel indices
    int16_t tag_match_limit;      // 0x22 sound_class_definition.maximum_sounds_per_tag
    int16_t owner_match_count;    // 0x24
    int16_t owner_matches[16];    // 0x26 sound_channel indices
    int16_t owner_match_limit;    // 0x46 sound_class_definition.maximum_sounds_per_object
} sound_channel_candidate_list;   // size 0x48

typedef enum sound_channel_type_flags {
    _sound_channel_3d_bit = 0x01,
    _sound_channel_stereo_bit = 0x02,
    _sound_channel_44khz_bit = 0x04,        // selects 44100 from k_sound_sample_rates
    _sound_channel_compressed_bit = 0x08    // UNSURE: matched against Sound.format != 0 (0x548520)
} sound_channel_type_flags;

// ---------------------------------------------------------------------------
// sound_listener  (0x00725218, one entry, stride 0x44 by listener_index)
// sound_update_listener 0x54b970, 0x54bbd0, 0x54bc50, 0x54c900
// ---------------------------------------------------------------------------
typedef struct sound_listener {
    uint8_t valid;             // 0x00 cleared when there is no local player
    uint8_t underwater;        // 0x01 FUN_0053ed60; edges play the matg enter/exit water sounds
    int16_t unknown_02;        // 0x02 padding
    float scale;               // 0x04 real_matrix4x3 (types/math.h) from FUN_004cb970
    Vector3D forward;          // 0x08
    Vector3D left;             // 0x14
    Vector3D up;               // 0x20
    Point3D position;          // 0x2c
    Vector3D velocity;         // 0x38 subtracted from the 30 Hz source velocity in 0x54c900
} sound_listener;              // size 0x44

// observer camera rows (0x006ac6d0, one per local player, stride 0x29c). The observer/camera
// code owns this array and has no type for it yet (src/game/camera_observer_get_target_angles.c
// reads it raw); this is the part the sound module reads. It is an array in .bss, not a pointer:
// every access is add reg, 0x6ac6d0 or an absolute [0x6ac6xx] operand.
// sound_update_listener 0x54b970, sound_build_cluster_range_bitmap 0x544980,
// sound_compute_obstruction_occlusion 0x544aa0
typedef struct sound_observer_camera {
    Point3D position;          // 0x00 listener position
    int32_t leaf_index;        // 0x0c with cluster_index a {leaf, cluster} location; its address
                               //      goes to FUN_0053ed60 (underwater test)
    int16_t cluster_index;     // 0x10 listener cluster, -1 outside the bsp
    int16_t unknown_12;        // 0x12
    Vector3D velocity;         // 0x14 world units per tick, rotated into listener space
    Vector3D forward;          // 0x20
    Vector3D up;               // 0x2c
    uint8_t unknown_38[0x264]; // 0x38 not read by this module
} sound_observer_camera;       // size 0x29c

// argument of sound_driver.set_listener (sound_listener_update 0x547070, built by 0x54b970)
typedef struct sound_listener_parameters {
    Point3D position;          // 0x00 always global_zero: sources are moved into listener space
    Vector3D forward;          // 0x0c SetOrientation front
    Vector3D up;               // 0x18 SetOrientation top
    Vector3D velocity;         // 0x24 SetVelocity
    SoundEnvironment *environment; // 0x30 compared as 0x12 dwords against the cache
} sound_listener_parameters;   // size 0x34

// last values committed to the IDirectSound3DListener (0x00746030, sound_listener_update 0x547070)
typedef struct directsound_listener_cache {
    Point3D position;          // 0x00 SetPosition
    Vector3D forward;          // 0x0c SetOrientation front
    Vector3D up;               // 0x18 SetOrientation top
    Vector3D velocity;         // 0x24 SetVelocity
} directsound_listener_cache;  // size 0x30

// argument of sound_driver.set_channel_spatial (0x5472d0, built in 0x54c900)
typedef struct sound_channel_spatial {
    Point3D position;          // 0x00 SetPosition, y negated
    Vector3D forward;          // 0x0c SetConeOrientation, y negated
    Vector3D velocity;         // 0x18 SetVelocity, y negated
} sound_channel_spatial;       // size 0x24

// argument of the channel parameter proc at 0x006e36cc and of 0x5475b0, built in 0x54c750 / 0x54deb0
typedef struct sound_channel_parameters {
    float minimum_distance;    // 0x00 SetMinDistance
    float maximum_distance;    // 0x04 SetMaxDistance, always FLT_MAX
    float pitch;               // 0x08 converted to SetFrequency
    float gain;                // 0x0c SetVolume after the fade factor
    float inner_cone_angle;    // 0x10 SetConeAngles
    float outer_cone_angle;    // 0x14
    float outer_cone_gain;     // 0x18 SetConeOutsideVolume
    float eax_value;           // 0x1c sound_class_definition.eax_value
} sound_channel_parameters;    // size 0x20

typedef void (*sound_channel_parameters_proc)(int16_t channel_index, sound_channel_parameters *parameters,
    uint8_t update, int16_t sound_class);

// ---------------------------------------------------------------------------
// sound_driver  (.data 0x0069f4c8, see header note; pointer at 0x00725208)
// ---------------------------------------------------------------------------
// .data 0x0069f514; sound_initialize passes its address to sound_driver.initialize
typedef struct sound_driver_parameters {
    int16_t driver_index;      // 0x00 index into the driver table at 0x0069f508 (0..1)
    int16_t channel_counts[4]; // 0x02 per channel type, decremented when a buffer fails (0x545e20)
    int16_t slot_counts[4];    // 0x0a per channel type, summed into the sound_channel count
    int16_t unknown_12;        // 0x12 zero, never read
} sound_driver_parameters;     // size 0x14

typedef struct sound_driver {
    int16_t type;                                     // 0x00 must equal sound_driver_parameters.driver_index
    int16_t unknown_02;                               // 0x02 padding
    uint8_t (*initialize)(sound_driver_parameters *parameters); // 0x04 0x545e20
    void (*dispose)(void);                            // 0x08 0x546a60 (Ghidra: game_sound_dispose)
    void (*set_listener)(sound_listener_parameters *listener); // 0x0c 0x547070
    void (*begin_frame)(void);                        // 0x10 0x546f90 clears the deferred flag
    void (*end_frame)(void);                          // 0x14 0x546b80 fade, stream fill, debug text
    void (*channel_play)(int16_t channel_index, SoundPermutation *source, int16_t unused,
        int16_t sound_class, uint8_t crosslap);       // 0x18 0x548380 -> 0x547c80 (CL = crosslap;
                                                      //      the third argument is not read)
    void (*channel_continue)(int16_t channel_index, uint8_t unused, int16_t sound_class); // 0x1c 0x5483d0 -> 0x5478c0
                                                      //      (only the channel is used; the byte is
                                                      //      pushed to 0x5478c0, which ignores it)
    void (*channel_stop)(int16_t channel_index);      // 0x20 0x548410 -> 0x547f60
    int16_t (*channel_get_state)(int16_t channel_index); // 0x24 0x548450 -> 0x548050
    void (*set_paused)(uint8_t paused);               // 0x28 0x546fe0
    void (*stop_all)(void);                           // 0x2c 0x546fa0
    void (*channel_set_spatial)(int16_t channel_index, uint8_t spatialized, sound_channel_spatial *spatial,
        float obstruction, float occlusion, uint8_t underwater, int16_t sound_class); // 0x30 0x548470 -> 0x5472d0
    void (*channel_set_parameters)(int16_t channel_index, sound_channel_parameters *parameters,
        uint8_t unknown);                             // 0x34 0x5484d0 -> 0x5475b0
    void (*set_quality)(int32_t unknown, uint8_t eax_enabled, int32_t quality); // 0x38 0x5480f0
    uint8_t (*eax_available)(void);                   // 0x3c 0x5482a0
} sound_driver;                                       // size 0x40

// driver slot table 0x007252e4: logical sound_channel index -> directsound_channel
typedef struct sound_channel_binding {
    int16_t hardware_channel_index; // 0x00 -1 until 0x5482e0 assigns one
    int16_t channel_type;      // 0x02 0..3, index into 0x0069f528 and 0x00746028
} sound_channel_binding;       // size 0x04

// ---------------------------------------------------------------------------
// ogg / pcm feed state, embedded in directsound_channel at 0xa0
// sound_ogg_stream_open 0x544eb0, sound_ogg_stream_read 0x5451d0, 0x545760,
// sound_pcm_buffer_read 0x545860, sound_ogg_buffer_fill 0x545920, 0x547ab0
// ---------------------------------------------------------------------------
typedef struct sound_ogg_memory_file {
    int32_t position;          // 0x00 seek callback 0x544e00 bounds it by size
    void *data;                // 0x04 SoundPermutation cached sample pointer
    int32_t size;              // 0x08 SoundPermutation.samples.size
    uint8_t end_of_file;       // 0x0c cleared by every seek
    uint8_t unknown_0d[3];     // 0x0d padding
} sound_ogg_memory_file;       // size 0x10

typedef struct sound_stream_decoder {
    int32_t position;          // 0x000 PCM byte offset into the source (sound_pcm_buffer_read)
    int32_t decoded_bytes;     // 0x004 ogg bytes produced so far for the current source
    uint8_t ogg_vorbis_file[2][0x2d0]; // 0x008 two OggVorbis_File for crosslapped transitions
    uint8_t open;              // 0x5a8 a stream is open (also cleared by 0x547f60)
    uint8_t active_file;       // 0x5a9 0 selects file 1 / memory file 1, 1 selects file 0 / memory file 0
    int16_t unknown_5aa;       // 0x5aa padding
    sound_ogg_memory_file memory_files[2]; // 0x5ac datasource of each OggVorbis_File
} sound_stream_decoder;        // size 0x5cc

// ---------------------------------------------------------------------------
// directsound_channel  (0x00725430, stride 0x678, count at 0x00725428)
// sound_channel_create 0x546760, 0x546b40, 0x546b80, 0x5472d0, 0x5475b0, 0x547890..0x548050,
// 0x5482e0, sound_eax30_effect_apply_channel 0x550890, 0x551480. The EAX initializers take
// the array base and read channel 0 buffer_3d.
// ---------------------------------------------------------------------------
typedef enum directsound_channel_state {
    _directsound_channel_idle = 0,
    _directsound_channel_playing = 1,
    _directsound_channel_queued = 2          // a second source waits behind the current one
} directsound_channel_state;

typedef struct directsound_channel {
    int16_t state;             // 0x000 directsound_channel_state
    int16_t sound_channel_index; // 0x002 owning logical channel, -1 free
    int16_t sound_class;       // 0x004 SoundClass of the queued source, -1 idle
    uint8_t spatialized;       // 0x006 3D mode normal vs disabled (SetMode)
    uint8_t underwater;        // 0x007 low pass through the EAX direct path
    uint8_t free;              // 0x008 released and reusable
    uint8_t streaming;         // 0x009 keeps filling silence after the source ends
    int16_t unknown_00a;       // 0x00a never referenced
    Point3D position;          // 0x00c cached SetPosition
    Vector3D cone_orientation; // 0x018 cached SetConeOrientation
    Vector3D velocity;         // 0x024 cached SetVelocity
    uint8_t unknown_030[8];    // 0x030 never referenced
    uint16_t type_flags;       // 0x038 sound_channel_type_flags
    int16_t unknown_03a;       // 0x03a never referenced
    float gain;                // 0x03c cached gain (before the fade factor)
    float pitch;               // 0x040 cached pitch
    float obstruction;         // 0x044
    float occlusion;           // 0x048
    float minimum_distance;    // 0x04c
    float maximum_distance;    // 0x050
    float cone_outside_gain;   // 0x054
    float inner_cone_angle;    // 0x058 radians, compared with 2 degree tolerance
    float outer_cone_angle;    // 0x05c
    float eax_value;           // 0x060 sound_class_definition.eax_value
    int32_t unknown_064;       // 0x064 never referenced
    int32_t buffer_size;       // 0x068 3 * channels * 2 * rate bytes
    int16_t frequency;         // 0x06c last SetFrequency value
    int16_t unknown_06e;       // 0x06e never referenced
    int32_t volume;            // 0x070 last SetVolume value, hundredths of a decibel
    int32_t unknown_074;       // 0x074 never referenced
    int32_t write_cursor;      // 0x078 ring buffer position filled up to
    int32_t source_end_cursor; // 0x07c where the current source ends, -1 none
    int32_t unknown_080;       // 0x080 never referenced
    int32_t streaming_bytes;   // 0x084 silence bytes written after the source, -1 none
    SoundPermutation *source;  // 0x088 permutation being fed
    SoundPermutation *next_source; // 0x08c queued permutation
    uint8_t source_crosslap;   // 0x090 current source continues an ogg stream
    uint8_t next_source_crosslap; // 0x091
    uint8_t unknown_092[6];    // 0x092 never referenced
    uint8_t source_started;    // 0x098 first fill of the current source done
    uint8_t unknown_099[7];    // 0x099 never referenced
    sound_stream_decoder decoder; // 0x0a0
    int32_t unknown_66c;       // 0x66c never referenced
    void *buffer;              // 0x670 IDirectSoundBuffer *
    void *buffer_3d;           // 0x674 IDirectSound3DBuffer *, null for 2D channels
} directsound_channel;         // size 0x678

// DirectSound COM methods called through the interface vtables (__stdcall, this first), folded
// here from the driver files. IDirectSound: +0x0c CreateSoundBuffer. IDirectSoundBuffer: +0x10
// GetCurrentPosition, +0x24 GetStatus, +0x2c Lock, +0x30 Play, +0x34 SetCurrentPosition, +0x3c
// SetVolume, +0x48 Stop, +0x4c Unlock, +0x50 Restore. IDirectSound3DListener: +0x44
// CommitDeferredSettings. IUnknown: +0 QueryInterface.
typedef int32_t (*directsound_query_interface_proc)(void *self, uint8_t *iid, void **object);
typedef int32_t (*directsound_buffer_get_current_position_proc)(void *self, int32_t *play_cursor,
    int32_t *write_cursor);
typedef int32_t (*directsound_buffer_get_status_proc)(void *self, uint32_t *status);
typedef int32_t (*directsound_buffer_lock_proc)(void *self, int32_t offset, uint32_t bytes, void **ptr1,
    uint32_t *bytes1, void **ptr2, uint32_t *bytes2, uint32_t flags);
typedef int32_t (*directsound_buffer_play_proc)(void *self, uint32_t reserved, uint32_t priority, uint32_t flags);
typedef int32_t (*directsound_buffer_set_current_position_proc)(void *self, int32_t position);
typedef int32_t (*directsound_buffer_set_volume_proc)(void *self, int32_t volume);
typedef int32_t (*directsound_buffer_stop_proc)(void *self);
typedef int32_t (*directsound_buffer_unlock_proc)(void *self, void *ptr1, uint32_t bytes1, void *ptr2,
    uint32_t bytes2);
typedef int32_t (*directsound_buffer_restore_proc)(void *self);
typedef int32_t (*directsound_listener_commit_proc)(void *self);

// WAVEFORMATEX as built by 0x545a30 / 0x546760 (format tag 1, 16 bits)
typedef struct sound_wave_format {
    uint16_t format_tag;       // 0x00 1 = PCM
    uint16_t channels;         // 0x02
    uint32_t samples_per_second; // 0x04 22050 / 44100
    uint32_t average_bytes_per_second; // 0x08
    uint16_t block_align;      // 0x0c
    uint16_t bits_per_sample;  // 0x0e 16
    uint16_t extra_size;       // 0x10 0
} sound_wave_format;           // size 0x12

// DSBUFFERDESC as built by 0x545a30 / 0x546760
typedef struct sound_buffer_description {
    uint32_t size;             // 0x00 0x24
    uint32_t flags;            // 0x04 0x100a0 / 0x100a8 | 0x10 (3D) | 0x200
    uint32_t buffer_bytes;     // 0x08
    uint32_t reserved;         // 0x0c
    sound_wave_format *format; // 0x10
    uint32_t algorithm_3d[4];  // 0x14 GUID, DS3DALG_HRTF_FULL written when EAX is off
} sound_buffer_description;    // size 0x24

// DSBCAPS, filled by IDirectSoundBuffer::GetCaps in 0x546760 (DSBCAPS_LOCHARDWARE = 4)
typedef struct win32_dsbcaps {
    uint32_t size;             // 0x00 0x14
    uint32_t flags;            // 0x04
    uint32_t buffer_bytes;     // 0x08
    uint32_t unlock_transfer_rate; // 0x0c
    uint32_t play_cpu_overhead; // 0x10
} win32_dsbcaps;               // size 0x14

typedef int32_t (*directsound_create_sound_buffer_proc)(void *self, sound_buffer_description *description,
    void **buffer, void *outer);
typedef int32_t (*directsound_buffer_get_caps_proc)(void *self, win32_dsbcaps *caps);

// Xbox ADPCM block decoders (0x0065e640 table, 0x54e920 mono / 0x54ea60 stereo), called by
// sound_decode_dispatch 0x54e830 with the block layout it computes
typedef int32_t (*sound_decode_block_proc)(void *source, void *destination, int32_t block_count,
    int32_t block_size, int32_t samples_per_block, int32_t *out_0, int32_t *out_1);

// ---------------------------------------------------------------------------
// sound effect objects (EAX), pointer at 0x00721f24
// sound_effects_object_detect_mode 0x551270, sound_effects_object_reinitialize 0x5514d0,
// sound_eax20_* 0x54ef30..0x54fa80, sound_eax30_* 0x54fff0..0x550c70, EAX1 0x54ec40..0x54ed50
// ---------------------------------------------------------------------------
typedef enum sound_effect_object_mode {
    _sound_effect_object_none = -1,
    _sound_effect_object_eax1 = 0,
    _sound_effect_object_eax2 = 1,
    _sound_effect_object_eax3 = 2,
    _sound_effect_object_directx = 3        // only named by 0x551270, never constructed
} sound_effect_object_mode;

// IKsPropertySet methods used by the EAX objects (COM __stdcall, this first):
// Set is vtable +0x10, QuerySupport +0x14. Folded here from the EAX1/2/3 files.
typedef int32_t (*sound_property_set_fn)(void *this_ps, const uint8_t *guid, uint32_t id,
    void *instance_data, uint32_t instance_size, void *value, uint32_t value_size);
typedef int32_t (*sound_query_support_fn)(void *this_ps, const uint8_t *guid, uint32_t id, uint32_t *out);

typedef struct sound_effect_object_vtable {
    void (*shutdown)(void *this_object);                         // 0x00 0x54ef30 / 0x54fff0 / 0x54ec40
    int32_t (*initialize)(void *this_object, directsound_channel *channels, int32_t unknown); // 0x04
    int32_t (*initialize_channel)(void *this_object, int32_t channel_index); // 0x08 0x54f6e0 / 0x551240
    int32_t (*listener_supported)(void *this_object);            // 0x0c 0x54ef10 returns +0x10
    int32_t (*channel_supported)(void *this_object);             // 0x10 0x54ef20 returns +0x14
    void (*apply_channel)(void *this_object, int32_t channel_index); // 0x14
    void (*set_environment_index)(void *this_object, int32_t environment); // 0x18
    void (*apply_listener)(void *this_object, SoundEnvironment *environment); // 0x1c
    void (*set_room_gain)(void *this_object, float gain);        // 0x20 converted to millibels
} sound_effect_object_vtable;  // size 0x24

typedef struct sound_effect_object {
    sound_effect_object_vtable *vtable; // 0x00
    int32_t mode;              // 0x04 sound_effect_object_mode
    uint32_t supported_properties; // 0x08 one bit per property id that QuerySupport accepted;
                               //      bit 0 set means deferred (id | 0x80000000) Set calls
    int32_t unknown_0c;        // 0x0c never referenced
    int32_t listener_supported; // 0x10
    int32_t channel_supported; // 0x14
    void *property_set;        // 0x18 IKsPropertySet * from channel 0 buffer_3d
} sound_effect_object;         // size 0x1c (EAX1 object size, operator_new(0x1c))

typedef struct sound_eax_effect_object {
    sound_effect_object base;  // 0x00
    void *channel_property_sets[51]; // 0x1c IKsPropertySet * per 3D channel; slot 0 also carries
                               //      the listener properties
} sound_eax_effect_object;     // size 0xe8 (EAX2 and EAX3, operator_new(0xe8))

// ---------------------------------------------------------------------------
// globals
// ---------------------------------------------------------------------------
// game sound (game state)
// global 0x007461a0: data_array *game_looping_sound_data         "object looping sounds", 0x400 x 0x34
// global 0x007461a4: game_sound_globals *game_sound_globals_ptr   0x0c bytes of game state
// global 0x00746160: uint32_t sound_cluster_audible_bitmap[16]    rebuilt by 0x544980; UNSURE length
// global 0x00746140: sound_class_gain *sound_class_gains          51 entries; allocation not in module
//
// sound
// global 0x00725200: uint8_t sound_initialized
// global 0x00725201: uint8_t sound_enabled                        master gain above zero
// global 0x00725202: uint8_t sound_paused                         focus lost, driver set_paused(1)
// global 0x00725203: uint8_t sound_idle_update_active             reentrancy guard of 0x549960
// global 0x00725204: int32_t sound_dialog_suppress_until_tick     0x549af0, scripted dialog length
// global 0x00725208: sound_driver *current_sound_driver
// global 0x0072520c: int32_t sound_time                           ms, sound_update_clock 0x54ae60
// global 0x00725210: float sound_time_delta                       elapsed ms * 0.03
// global 0x00725214: uint8_t sound_update_toggle                  flipped each sound_update pass
// global 0x00725218: sound_listener sound_listeners[1]
// global 0x0072525c: SoundEnvironment sound_environment           default copied from 0x0065e508
// global 0x007252a4: float sound_ducking_gain                     ramps to 0.7 while dialog plays
// global 0x007252a8: float sound_music_gain
// global 0x007252ac: float sound_master_gain
// global 0x007252b0: float sound_effects_gain
// global 0x007252b4: int16_t sound_channel_count
// global 0x007252b6: uint8_t sound_disabled
// global 0x007252b7: uint8_t sound_stopping_all                   set around 0x54adb0
// global 0x007252b8: int16_t sound_permutation_limit              UNSURE: 0 first only, 1 first half
// global 0x007252bc: uint8_t sound_dialog_unspatialized           UNSURE: forces dialog location type 0
// global 0x007252c0: data_array *sound_data                       "sounds", 0x200 x 0xb0
// global 0x00724a48: decoder proc sound_decode_proc              set by 0x54e830 from 0x0065e640 (7 arguments, see src/sound/sound_decode_dispatch.c)
// global 0x00724a4c: uint8_t debug_sound_channels                 0x546b80 channel totals
// global 0x00724a4d: uint8_t debug_sound                          render_debug_sound 0x54e6d0
// global 0x00724a50: data_array *looping_sound_data               "looping sounds", 0x80 x 0xe4
// global 0x00724a54: uint8_t sound_looping_audibility_check       UNSURE: gates 0x54e740
// global 0x00724a60: sound_channel sound_channels[81]
// global 0x007251f8: uint8_t debug_sound_channel_details          0x546b80 per channel lines
// global 0x006e36cc: sound_channel_parameters_proc sound_channel_parameters_proc_ptr  0x54ce50 / 0x54cf80
// global 0x00721f14: int32_t sound_ogg_underrun_count             0x545920
//
// directsound driver
// global 0x007252e0: uint8_t directsound_initialized
// global 0x007252e2: int16_t directsound_binding_count
// global 0x007252e4: sound_channel_binding directsound_bindings[81]
// global 0x00725428: int16_t directsound_channel_count
// global 0x00725430: directsound_channel directsound_channels[81]
// global 0x00746028: int16_t directsound_first_channel_of_type[4]
// global 0x00746030: directsound_listener_cache directsound_listener_cached
// global 0x00746064: SoundEnvironment directsound_environment_cache
// global 0x007460ac: uint8_t directsound_caps[0x60]              DSCAPS
// global 0x0074610c: void *directsound                           IDirectSound *
// global 0x00746110: void *directsound_primary_buffer            IDirectSoundBuffer *
// global 0x00746114: void *directsound_listener                  IDirectSound3DListener *
// global 0x00746118: uint8_t directsound_paused
// global 0x0074611c: float directsound_fade                       0.05 per frame toward paused state
// global 0x00746120: uint8_t directsound_eax_available
// global 0x00746121: uint8_t directsound_eax_enabled
// global 0x00746124: int16_t directsound_hardware_3d_channel_count  3D buffers with DSBCAPS_LOCHARDWARE (0x546760); zeroed by 0x545e20
// global 0x00746128: int32_t directsound_quality                  0..2, 0x5480f0
// global 0x0074612c: int32_t directsound_hardware_mode            UNSURE: 3 adds DSBCAPS 0x200, -1 reset
// global 0x00746130: int16_t sound_effect_object_state            0 none, 2 disabled
// global 0x00746132: uint8_t directsound_deferred_dirty           CommitDeferredSettings pending
// global 0x00721f24: sound_effect_object *global_sound_effect_object
//
// .data / .rdata tables
// global 0x0069eae0: sound_class_definition sound_class_definitions[51]
// global 0x0069f3a8: char *sound_class_names[51]                  "projectile_impact", ...
// global 0x0069f4c8: sound_driver directsound_driver
// global 0x0069f508: sound_driver *sound_drivers[2]               { 0x0069f4c8, 0 }
// global 0x0069f510: float sound_fade_curve_exponent              2.5
// global 0x0069f514: sound_driver_parameters driver_parameters  { 0, {22,2,2,2}, {22,2,2,2}, 0 }
// global 0x0069f528: uint16_t sound_channel_type_flag_table[4]     { 9, 8, 0xa, 0xe }
// global 0x0069ff28: float sound_underwater_direct_gain           0.25
// global 0x006893d4: float sound_dialog_ducking_gain              0.7
// global 0x0065e4f8: int32_t k_sound_sample_rates[2]              { 22050, 44100 }
// global 0x0065e508: SoundEnvironment k_default_sound_environment
// global 0x0065e640: void *k_sound_decode_procs[3]                { 0x7fff, 0x54e920, 0x54ea60 } Xbox ADPCM decoders by channel count
// global 0x00671d04: sound_effect_object_vtable sound_eax2_vtable
// global 0x00671d28: sound_effect_object_vtable sound_eax3_vtable
// global 0x00671d4c: sound_effect_object_vtable sound_eax1_vtable
//
// referenced, owned elsewhere
// global 0x006ac6d0: sound_observer_camera observer_cameras[]     (stride 0x29c, see the struct)
// 0x0087bc14 tag_instances, 0x008603b0 object headers, 0x006f1d6c game time, 0x0087a478
// player globals, 0x00746f9c / 0x00746fa0 collision bsp and matg globals, 0x006ac528 /
// 0x006ac530 / 0x006ac554 / 0x006869c4 sound cache (types/cache.h), 0x00719cd4 random seed, 0x006e35c8 error text buffer, 0x00722b58 /
// 0x00722b5c unknown flags, 0x006ac8f8 performance counter frequency.

#pragma pack(pop)
