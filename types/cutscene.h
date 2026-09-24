// Blam cutscene module (halo.exe 1.0.10 retail, 0x449590..0x44a790, 15 Ghidra functions).
// Three things share this address run, and the header follows them:
//
//   sort            0x449590..0x449720   a dword-array quicksort (EAX count, ECX base, stack
//                                        comparator) and its small-partition fallback. Library
//                                        style code with callers in ai (0x412ba0) and sound
//                                        (0x552cf0); it owns no types. See the notes file.
//   cinematics      0x449720..0x449f80   the hs cinematic_* layer: cinematic_start /
//                                        cinematic_stop (0x449720 / 0x449eb0), the letterbox
//                                        fade and draw plus the queued cutscene titles
//                                        (0x4499c0, 0x449960) and the shared filled screen
//                                        rectangle routine 0x449780 (ECX Rectangle2D *, EAX
//                                        packed ARGB) that the letterbox and six interface /
//                                        rasterizer callers use. State block at *0x006f187c.
//   recorded        0x449f80..0x44a930   the recorded animation (hs cutscene_recording) codec:
//   animations                           Scenario.recorded_animations lookup by name, the
//                                        versioned unit_control_data unpacker, the compressed
//                                        (version 4) and uncompressed (versions 1..3) event
//                                        stream decoders and their per event handlers. The
//                                        playback records they fill live in the data_array at
//                                        0x006b0a10, driven by 0x44a930 / 0x44aa90, which the
//                                        address run heuristic handed to devices (see
//                                        out/phase4/devices_types_notes.md); devices.h leaves
//                                        that record undefined and points here, so it is
//                                        defined below.
//
// Offsets in comments are byte offsets from the struct base. Where the binary itself carries a
// layout it is preferred over the decompiler and the fact is called out:
//
//   - cinematic_globals (0x1c) is sized by its allocation in the new map initializer 0x45a9c0
//     (Ghidra name particle_systems_initialize): game state cursor += 0x1c, crc32 over the
//     size, then the base goes to 0x006f187c, right after data_new("recorded animations",
//     0x40). The reset at 0x45b2f2 (inside 0x45b050) zeroes dwords 0..6 and stores -1 into
//     dwords 3..6, i.e. the four title slots.
//   - Every event payload of both recorded animation codecs is described by a
//     byte_swap_definition ('bysw') in .data between 0x686c2c and 0x686fc4, each named and
//     sized: byte 1, word 2, long 4, real_vector2d 8, real_vector3d 0x0c,
//     animation_state_event_data 1, aiming_speed_event_data 1, control_flags_event_data 2,
//     weapon_index_event_data 2, throttle_event_data 8, vector_char_difference_data 2 (codes
//     1,1), vector_short_difference_data 4 (codes -2,-2), animation_event_v1 4 (codes -2,-2),
//     animation_state_set_event_v1 6, aiming_speed_set_event_v1 6, control_flags_set_event_v1
//     6, weapon_index_set_event_v1 6, throttle_set_event_v1 0x0c, multi_vector_set_event_v1
//     0x10, angle_vector_set_event_v1 0x0c. The struct names below follow those strings.
//   - The unit_control_data field layouts per unit_control_data_version are four tables at
//     0x686cc8 / 0x686d40 / 0x686d58 / 0x686d70 (pointer array 0x686d88) of
//     {byte_swap_definition *, size, offset} records ending in {NULL, -1, -1}; 0x449fd0
//     applies tables 0..version-1 in turn. Their offsets agree with types/units.h.
//   - The recorded animation playback record (0x64) is fixed by datum_get arithmetic in
//     0x44aa90 (index * 100 + data) and by the argument addresses both codec entry points
//     receive: +0x54 decoder state, +0x14 unit_control_data, +0x0c event ticks, +0x10 cursor.
//
// Types this module operates on that already have a definition, and are therefore NOT
// redefined here:
//   types/tags.h     Scenario (recorded_animations 0x36c, cutscene_titles 0x4fc with its
//                    pointer at 0x500, ingame_help_text tag id at 0x590 is the string list the
//                    titles draw from), ScenarioRecordedAnimation (0x40: name 0x00, version
//                    0x20, unit_control_data_version 0x22, length_of_animation 0x24, event
//                    stream pointer 0x38), ScenarioCutsceneTitle (0x60: string_index 0x30,
//                    text_style 0x32, justification 0x34, text_flags 0x38, text_color 0x3c,
//                    shadow_color 0x40, fade_in_time 0x44, up_time 0x48, fade_out_time 0x4c),
//                    HUDGlobals (fullscreen_font tag id 0x54), Globals (rasterizer_data 0x134,
//                    GlobalsRasterizerData default_2d tag id 0xb8), Rectangle2D, ColorARGBInt
//   types/memory.h   datum_index, data_array, byte_swap_definition
//   types/math.h     real_vector2d, real_vector3d, real_euler_angles2d
//   types/units.h    unit_control_data (0x40), which both codecs decode into
//   types/game.h     game_time_globals (game_time 0x0c, ticks_this_frame 0x10, paused 0x02),
//                    player_globals (+0x11 set by cinematic_start, cleared by cinematic_stop)
//   types/ai.h       ai_globals (+0x10 communication_valid, cleared by cinematic_start)
//   types/render.h   cinematic_screen_effect_globals (0x78, reset by cinematic_stop through
//                    0x0071cfc4) and the 0x10 byte model tint at 0x0071cfc0
//   types/interface.h ui_pending_error (0x00718fb6, reported and cleared by cinematic_stop),
//                    ui_quad_render_state and hud_quad_vertex (the stack blocks 0x449780 builds
//                    for the quad submitter 0x51c9a0)
//
// Note on sizes: like types/memory.h, structs holding pointers only measure to the
// documented size under a 32-bit data organization.

#pragma pack(push, 1)
typedef unsigned char uint8_t; typedef signed char int8_t; typedef unsigned short uint16_t; typedef short int16_t;
typedef unsigned int uint32_t; typedef int int32_t;

// ---------------------------------------------------------------------------
// constants
// ---------------------------------------------------------------------------
typedef enum cutscene_constants {
    k_cinematic_title_slot_count = 4,            // cutscene_title_queue 0x449960 and the draw
                                                 // loop in 0x4499c0 both stop at 4
    k_cinematic_title_none = -1,                 // empty slot marker, index and ticks
    k_cinematic_ticks_per_second = 30,           // 0x00672ac8 30.0 turns the queue delay into
                                                 // ticks; 0x00672acc 1/30 turns ticks into the
                                                 // letterbox fade step (a one second fade)
    k_cinematic_letterbox_screen_width = 640,    // right edge of both bars (0x44204000)
    k_cinematic_letterbox_screen_height = 480,   // 0x00672b9c 480.0; bar height is
                                                 // letterbox_scale * 0.125 (0x00672cbc) * 480
    k_cinematic_letterbox_bottom_edge = 0x1e1,   // bottom of the lower bar (481)
    k_cinematic_letterbox_color = -0x1000000,    // 0xff000000 opaque black, the EAX colour of
                                                 // both 0x449780 calls
    k_recorded_animation_maximum_count = 0x40,   // data_new("recorded animations", 0x40)
    k_recorded_animation_datum_size = 0x64,      // the stride datum_get uses in 0x44aa90
    k_recorded_animation_version_count = 4,      // ScenarioRecordedAnimation.version 1..4,
                                                 // minus one indexes 0x00686fe8
    k_recorded_animation_event_type_count = 0x17, // both handler tables hold 0x17 entries;
                                                 // the next dword is already a bysw record
    k_unit_control_data_version_count = 4,       // pointer array 0x00686d88
    k_recorded_animation_angle_half_turn = 1000, // compressed angles wrap at +-1000, and
                                                 // 0x00672dd8 pi / 1000 converts to radians
    k_recorded_animation_compressed_type_shift = 2, // compressed event byte: type in bits 2..7
    k_recorded_animation_compressed_delay_mask = 3  // and the delay encoding in bits 0..1
} cutscene_constants;

// ---------------------------------------------------------------------------
// recorded animation event types
// One numbering, two codecs. 0..6 mean the same in both. Above 6 the compressed codec
// (0x686d98 handlers) and the uncompressed v1 codec (0x686ea8 handlers) diverge; a type with a
// NULL handler is skipped (v1: the 4 byte header only; compressed: the header bytes only).
// ---------------------------------------------------------------------------
typedef enum recorded_animation_event_type {
    _recorded_animation_event_none = 0,            // no handler in either table
    _recorded_animation_event_end = 1,             // both update routines stop on it and report
                                                   // the animation finished once it is due
    _recorded_animation_event_animation_state = 2, // unit_control_data.animation_state
    _recorded_animation_event_aiming_speed = 3,    // unit_control_data.aiming_speed
    _recorded_animation_event_control_flags = 4,   // unit_control_data.control_flags
    _recorded_animation_event_weapon_index = 5,    // unit_control_data.weapon_index
    _recorded_animation_event_throttle = 6,        // unit_control_data.throttle i, j (k := 0)

    // compressed (version 4): type - 7 or type - 15 is a recorded_animation_vector_mask
    _recorded_animation_event_char_difference_first = 7,   // 0x44a1d0, delta is 2 x int8
    _recorded_animation_event_char_difference_last = 14,
    _recorded_animation_event_short_difference_first = 15, // 0x44a390, delta is 2 x int16
    _recorded_animation_event_short_difference_last = 22,

    // uncompressed (versions 1..3)
    _recorded_animation_event_v1_facing_vector = 9,        // 0x44a700
    _recorded_animation_event_v1_aiming_vector = 10,       // 0x44a730
    _recorded_animation_event_v1_looking_vector = 11,      // 0x44a760
    _recorded_animation_event_v1_multi_vector_first = 12,  // 0x44a820: 12 skips looking,
    _recorded_animation_event_v1_multi_vector_last = 15,   // 13 aiming, 14 facing, 15 none
    _recorded_animation_event_v1_angle_vector_first = 16,  // 0x44a790: 0x13 skips looking,
    _recorded_animation_event_v1_angle_vector_last = 22    // 0x14 aiming, 0x15 facing; 0x10,
                                                           // 0x11, 0x12, 0x16 write all three
} recorded_animation_event_type;

// Which of the three unit_control_data direction vectors a compressed difference event moves.
// The first set bit gets the delta; the later set bits copy the vector just produced (and its
// compressed angles) instead of decoding their own, so one delta pair is consumed per event.
typedef enum recorded_animation_vector_mask {
    _recorded_animation_vector_facing = 0x1,       // unit_control_data +0x1c, state angles +0x0
    _recorded_animation_vector_aiming = 0x2,       // unit_control_data +0x28, state angles +0x4
    _recorded_animation_vector_looking = 0x4       // unit_control_data +0x34, state angles +0x8
} recorded_animation_vector_mask;

// Bits 0..1 of a compressed event byte: how many ticks after the previous event it fires and
// how many header bytes 0x44a590 steps over before the payload.
typedef enum recorded_animation_compressed_delay {
    _recorded_animation_delay_none = 0,            // 0 ticks, 1 header byte
    _recorded_animation_delay_one_tick = 1,        // 1 tick, 1 header byte
    _recorded_animation_delay_byte = 2,            // ticks in the next uint8, 2 header bytes
    _recorded_animation_delay_word = 3             // ticks in the next unaligned uint16, 3 bytes
} recorded_animation_compressed_delay;

// recorded_animation.flags. Bit 0 is owned by the playback loop; the rest are chosen by the
// caller of 0x44a930 (ORed in from its stack argument) except bit 2, which 0x44a930 derives.
typedef enum recorded_animation_flags {
    _recorded_animation_flag_finished = 0x1,       // set when the codec update returns false;
                                                   // the next tick restores the unit and deletes
                                                   // the record. 0x44acc0 skips finished ones
    _recorded_animation_flag_unknown_2 = 0x2,      // never tested by the playback functions
    _recorded_animation_flag_restore_object_flag_40 = 0x4, // 0x569bc0 result at start; on
                                                   // finish object +0x204 bit 6 is set from it
    _recorded_animation_flag_delete_object_when_finished = 0x8, // unless
                                                   // hs_object_hierarchy_test says keep it
    _recorded_animation_flag_mark_object_when_finished = 0x10   // sets object +0x4cc bit 1
} recorded_animation_flags;

// ---------------------------------------------------------------------------
// cinematic_title_slot  (cinematic_globals +0x0c, four of them)
// cutscene_title_queue 0x449960 fills the first slot whose index is -1 with the
// Scenario.cutscene_titles index and -(delay * 30); the letterbox update 0x4499c0 adds
// game_time_globals.ticks_this_frame to ticks every frame (0 while paused), fades the title by
// ticks against ScenarioCutsceneTitle fade_in_time / up_time / fade_out_time, and writes -1 to
// both halves once ticks passes up_time + fade_out_time.
// ---------------------------------------------------------------------------
typedef struct cinematic_title_slot {
    int16_t title_index;            // 0x00 Scenario.cutscene_titles element, -1 when empty
    int16_t ticks;                  // 0x02 negative while the start delay runs, then the ticks
                                    //      since the title appeared
} cinematic_title_slot;             // size 0x04

// ---------------------------------------------------------------------------
// cinematic_globals  (the 0x1c byte game state block at *0x006f187c)
// Field names follow the hs functions that write them, located through their hs function
// definitions (name string 8 bytes before the evaluator pointer): cinematic_start 0x47f7f0
// (calls 0x449720), cinematic_stop 0x47f800 (calls 0x449eb0), cinematic_skip_start_internal
// 0x47f840, cinematic_skip_stop_internal 0x47f860, cinematic_show_letterbox 0x47f8b0,
// cinematic_set_title 0x47f910 / cinematic_set_title_delayed 0x47f960 (call 0x449960),
// cinematic_suppress_bsp_object_creation 0x47f9b0.
// ---------------------------------------------------------------------------
typedef struct cinematic_globals {
    float letterbox_scale;          // 0x00 0..1; 0x4499c0 moves it by elapsed ticks / 30 towards
                                    //      show_letterbox, clamps it, and draws both bars while
                                    //      it is above 0
    int32_t letterbox_last_tick;    // 0x04 game_time_globals.game_time of the last letterbox
                                    //      update; seeded by cinematic_start and by
                                    //      cinematic_show_letterbox(true)
    uint8_t show_letterbox;         // 0x08 fade target: cinematic_start and
                                    //      cinematic_show_letterbox set it, cinematic_stop and
                                    //      the map dispose 0x45b370 clear it
    uint8_t in_progress;            // 0x09 set by cinematic_start, cleared by cinematic_stop;
                                    //      read by some twenty gates across the engine (input,
                                    //      hud, game state save, 0x4f4860 bsp objects)
    uint8_t skip_in_progress;       // 0x0a cinematic_skip_start_internal / _stop_internal; when
                                    //      set, the skip key path 0x472760 and the main loop
                                    //      0x4c78ce revert to the skip checkpoint
    uint8_t suppress_bsp_object_creation; // 0x0b cinematic_suppress_bsp_object_creation;
                                    //      0x4f4860 skips bsp object creation while this and
                                    //      in_progress are both set
    cinematic_title_slot titles[4]; // 0x0c k_cinematic_title_slot_count
} cinematic_globals;                // size 0x1c

// ---------------------------------------------------------------------------
// recorded_animation_angles
// A direction compressed to two int16s in units of pi / 1000 (0x00672dd8). The delta helpers
// 0x44a110 (int8 delta) and 0x44a150 (int16 delta) take the pair in EAX and the delta in EDX,
// add the delta, wrap yaw back by 1000 when it leaves -1000..1000 (pitch is not wrapped) and
// return EAX; 0x44a190 takes the pair in ECX and writes the unit vector
// (cos yaw cos pitch, sin yaw cos pitch, sin pitch) to the float[3] in EAX.
// ---------------------------------------------------------------------------
typedef struct recorded_animation_angles {
    int16_t yaw;                    // 0x00
    int16_t pitch;                  // 0x02
} recorded_animation_angles;        // size 0x04

// The decoder state the compressed codec carries between events (first argument of both codec
// entry points, recorded_animation +0x54). The compressed begin 0x44a550 copies it verbatim
// from the stream right after the initial unit_control_data; the v1 codec never touches it.
typedef struct recorded_animation_decoder_state {
    recorded_animation_angles facing;  // 0x00 mirrors unit_control_data.facing_vector
    recorded_animation_angles aiming;  // 0x04 mirrors unit_control_data.aiming_vector
    recorded_animation_angles looking; // 0x08 mirrors unit_control_data.looking_vector
} recorded_animation_decoder_state;    // size 0x0c

// Compressed difference payloads (bysw vector_char_difference_data / _short_difference_data).
typedef struct recorded_animation_char_difference {
    int8_t yaw;                     // 0x00
    int8_t pitch;                   // 0x01
} recorded_animation_char_difference; // size 0x02

typedef struct recorded_animation_short_difference {
    int16_t yaw;                    // 0x00
    int16_t pitch;                  // 0x02
} recorded_animation_short_difference; // size 0x04

// ---------------------------------------------------------------------------
// uncompressed (version 1..3) events. Every event starts with the 4 byte header; the handler
// advances the cursor by the full event size, and a type without a handler by 4.
// ---------------------------------------------------------------------------
typedef struct recorded_animation_event_v1 {
    int16_t type;                   // 0x00 recorded_animation_event_type
    uint16_t delay_ticks;           // 0x02 ticks after the previous event; 0x44a8b0 fires the
                                    //      event once the pending tick count reaches it
} recorded_animation_event_v1;      // size 0x04, bysw animation_event_v1

typedef struct recorded_animation_animation_state_set_event_v1 {
    recorded_animation_event_v1 header; // 0x00 type 2
    int8_t animation_state;         // 0x04 0x44a650
    uint8_t pad_05;                 // 0x05
} recorded_animation_animation_state_set_event_v1; // size 0x06

typedef struct recorded_animation_aiming_speed_set_event_v1 {
    recorded_animation_event_v1 header; // 0x00 type 3
    int8_t aiming_speed;            // 0x04 0x44a670
    uint8_t pad_05;                 // 0x05
} recorded_animation_aiming_speed_set_event_v1; // size 0x06

typedef struct recorded_animation_control_flags_set_event_v1 {
    recorded_animation_event_v1 header; // 0x00 type 4
    uint16_t control_flags;         // 0x04 0x44a690, unit_control_flags
} recorded_animation_control_flags_set_event_v1; // size 0x06

typedef struct recorded_animation_weapon_index_set_event_v1 {
    recorded_animation_event_v1 header; // 0x00 type 5
    int16_t weapon_index;           // 0x04 0x44a6b0
} recorded_animation_weapon_index_set_event_v1; // size 0x06

typedef struct recorded_animation_throttle_set_event_v1 {
    recorded_animation_event_v1 header; // 0x00 type 6
    real_vector2d throttle;         // 0x04 0x44a6d0 stores i, j and zeroes k
} recorded_animation_throttle_set_event_v1; // size 0x0c

typedef struct recorded_animation_multi_vector_set_event_v1 {
    recorded_animation_event_v1 header; // 0x00 types 9..15
    real_vector3d vector;           // 0x04 copied as is into the selected direction vectors
} recorded_animation_multi_vector_set_event_v1; // size 0x10

typedef struct recorded_animation_angle_vector_set_event_v1 {
    recorded_animation_event_v1 header; // 0x00 types 16..22
    real_euler_angles2d angles;     // 0x04 radians; 0x44a790 builds the unit vector from them
} recorded_animation_angle_vector_set_event_v1; // size 0x0c

// ---------------------------------------------------------------------------
// unit_control_data_field_layout  (records of the four tables behind 0x00686d88)
// 0x449fd0 (EBX = unit_control_data *, stack: cursor **, uint8 version) zeroes the 0x40 bytes,
// sets zoom_level to -1, then for each table 0..max(version,1)-1 copies size bytes from the
// cursor to offset (skipping the copy when offset is -1) and advances the cursor by size.
//   v1  animation_state 1@0x00, aiming_speed 1@0x01, control_flags 2@0x02, weapon_index 2@0x04,
//       2 bytes dropped, throttle 8@0x0c, facing 0x0c@0x1c, aiming 0x0c@0x28, looking 0x0c@0x34
//   v2  primary_trigger 4@0x18      v3  grenade_index 2@0x06      v4  zoom_level 2@0x08
// ---------------------------------------------------------------------------
typedef struct unit_control_data_field_layout {
    byte_swap_definition *type;     // 0x00 byte 0x686c2c, word 0x686c50, long 0x686c74,
                                    //      real_vector2d 0x686c90, real_vector3d 0x686cb0;
                                    //      NULL in the terminator; not read by 0x449fd0
    int32_t size;                   // 0x04 bytes taken from the stream, -1 ends the table
    int32_t offset;                 // 0x08 destination offset in unit_control_data, -1 skips
} unit_control_data_field_layout;   // size 0x0c

// ---------------------------------------------------------------------------
// codec entry points (all cdecl) and the two-entry codec table
// begin:  0x44a550 compressed (unpack control data, then copy the 0x0c byte decoder state),
//         0x44a890 v1 (unpack control data only)
// update: 0x44a590 compressed, 0x44a8b0 v1. Both fire every event whose delay fits in
//         *event_ticks, subtracting each delay, and return 0 only when the end event is due
//         exactly now (the playback loop then sets the finished flag), 1 otherwise.
// ---------------------------------------------------------------------------
typedef void (*recorded_animation_begin_proc)(recorded_animation_decoder_state *state,
    unit_control_data *control, uint8_t **cursor, uint8_t unit_control_data_version);
typedef uint8_t (*recorded_animation_update_proc)(recorded_animation_decoder_state *state,
    unit_control_data *control, int32_t *event_ticks, uint8_t **cursor);

// Compressed handler (0x686d98[type]): header points at the event byte, *cursor already sits
// past the delay bytes, and the handler advances it past its payload.
typedef void (*recorded_animation_compressed_event_proc)(recorded_animation_decoder_state *state,
    unit_control_data *control, uint8_t *header, uint8_t **cursor);
// Uncompressed handler (0x686ea8[type]): event is the full event record, *cursor still points
// at it and the handler advances it by the event size.
typedef void (*recorded_animation_v1_event_proc)(unit_control_data *control,
    recorded_animation_event_v1 *event, uint8_t **cursor);

typedef struct recorded_animation_codec {
    recorded_animation_begin_proc begin;   // 0x00
    recorded_animation_update_proc update; // 0x04
} recorded_animation_codec;         // size 0x08

// ---------------------------------------------------------------------------
// recorded_animation  (the 0x64 byte datum of the data_array at 0x006b0a10)
// Created by 0x44a930 (EAX unit, CX Scenario.recorded_animations index, stack extra flags),
// ticked by 0x44aa90, searched by 0x44acc0 / 0x44ad20.
// ---------------------------------------------------------------------------
typedef struct recorded_animation {
    int16_t identifier;             // 0x00 datum_header
    int16_t unknown_02;             // 0x02 never touched
    datum_index unit_index;         // 0x04 the biped or vehicle being driven (0x44aa90 drops
                                    //      the record for any other object type)
    int16_t ticks_remaining;        // 0x08 ScenarioRecordedAnimation.length_of_animation,
                                    //      decremented once per tick
    uint16_t flags;                 // 0x0a recorded_animation_flags
    int32_t event_ticks;            // 0x0c ticks not yet consumed by events: 0 at start, +1
                                    //      after each update, minus each fired event delay
    uint8_t *event_cursor;          // 0x10 starts at the recorded_animation_event_stream data
                                    //      (ScenarioRecordedAnimation +0x38)
    unit_control_data control_data; // 0x14 the decoded input handed to 0x5639f0 every tick
    recorded_animation_decoder_state decoder_state; // 0x54
    int16_t codec_index;            // 0x60 ScenarioRecordedAnimation.version - 1, indexes
                                    //      recorded_animation_codecs_by_version
    int16_t unknown_62;             // 0x62 never touched
} recorded_animation;               // size 0x64

// ---------------------------------------------------------------------------
// globals owned by this module
// ---------------------------------------------------------------------------
// global 0x006f187c: cinematic_globals *cinematic_globals      0x1c bytes of game state
// global 0x00686b60: float cinematic_saved_music_gain         -1.0 when nothing is saved;
//                                                             cinematic_start saves
//                                                             sound_music_gain here, stop and
//                                                             skip_stop restore it
// global 0x006b0a10: data_array *recorded_animations          "recorded animations", 0x40 x 0x64
// global 0x00686fe8: recorded_animation_codec *recorded_animation_codecs_by_version[4]
//                                                             {v1, v1, v1, compressed}
// global 0x00686fd8: recorded_animation_codec recorded_animation_compressed_codec
// global 0x00686fe0: recorded_animation_codec recorded_animation_v1_codec
// global 0x00686d98: recorded_animation_compressed_event_proc recorded_animation_compressed_event_handlers[0x17]
// global 0x00686ea8: recorded_animation_v1_event_proc recorded_animation_v1_event_handlers[0x17]
// global 0x00686d88: unit_control_data_field_layout *unit_control_data_version_layouts[4]
// global 0x00686cc8: unit_control_data_field_layout unit_control_data_layout_v1[10]
// global 0x00686d40: unit_control_data_field_layout unit_control_data_layout_v2[2]
// global 0x00686d58: unit_control_data_field_layout unit_control_data_layout_v3[2]
// global 0x00686d70: unit_control_data_field_layout unit_control_data_layout_v4[2]
// global 0x00686c2c: byte_swap_definition recorded_animation_byte_swap_definitions[]  the
//                                                             run of bysw records listed in
//                                                             the file header, 0x686c2c..
//                                                             0x686fc4
//
// Read here but owned elsewhere: 0x006f1d6c game_time_globals, 0x0087a478 player_globals,
// 0x00880354 ai_globals, 0x0071cfc4 / 0x0071cfc0 (render.h), 0x00718fb6 ui_pending_errors,
// 0x00718fc2 / 0x00718f94 (interface.h; the letterbox is not drawn while a ui widget is up),
// 0x007252a8 sound_music_gain, 0x00746f8c global_scenario, 0x00746fa0 global_globals,
// 0x0071941c hud_globals tag data, 0x0087bc14 tag instances, 0x0069c632 rasterizer vertex
// buffer lock state, 0x007c3140 (Rectangle2D, top and left read as the bar origin) and
// 0x0071d144 (the text shadow colour the title draw sets and clears).
// .rdata literals, not globals: 0x00672ac0 0.0, 0x00672ac4 1.0, 0x00672ac8 30.0, 0x00672acc
// 1/30, 0x00672b9c 480.0, 0x00672ba8 -1.0, 0x00672bd8 double 0.0001, 0x00672ca0 0.8 (white
// title text is dimmed to 0.8), 0x00672cbc 0.125, 0x00672dd8 pi / 1000.

#pragma pack(pop)
