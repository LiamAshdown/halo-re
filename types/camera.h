// Blam camera module (halo.exe 1.0.10 retail, 0x444c00..0x449170, 55 Ghidra functions).
// Three layers of the Blam camera share this range, and the header follows them:
//
//   camera script   0x444b30..0x4450e0   the hs driven cinematic camera (camera_set to a
//                                        cutscene camera point, camera animations,
//                                        camera_set_first_person, camera_set_dead) and its
//                                        globals block at 0x006869d0
//   director        0x4450e0..0x447370   per local player camera mode selection: the pov
//                                        procedure pointer, the per-mode data union, the look
//                                        input smoothing, first person / third person / dead /
//                                        flying / editor cameras. Array at 0x006ac560, 0xf8 each
//   observer        0x447680..0x449170   the final camera: takes the observer_command a
//                                        director pov procedure produced and eases every
//                                        parameter towards it with a quintic spline, clamps it,
//                                        pushes it out of geometry and publishes the
//                                        observer_camera the renderer and sound read.
//                                        Array at 0x006ac65c, 0x29c each
//
// Offsets in comments are byte offsets from the struct base. Where the binary itself carries a
// layout it is preferred over the decompiler and the fact is called out:
//
//   - observer (0x29c) is fixed by its constructor observer_new (0x447740, reached through the
//     thunk 0x447870 with EDX = 0x006ac65c and from 0x45b1a2 / 0x478282 with EDX = base + i*0x29c):
//     it writes the signature 0x72616421 at +0x000 AND at +0x298 (in_EDX[0xa6]), i.e. a header
//     and a trailer word 0x298 apart, which with the 0x29c stride used by every indexed access
//     (imul reg,reg,0x29c) pins the size and both ends.
//   - The 14-float and 11-float parameter vectors are carried by two int16 tables in .data:
//     0x00686ae0 = {3,3,1,1,6} (parameter floats per channel, sum 14) and
//     0x00686aec = {3,3,1,1,3} (derivative floats per channel, sum 11). Every spline routine
//     (0x447be0, 0x447e40, 0x448010, 0x448210) walks its arrays with those counts, so the
//     observer arrays are 14 floats (0x38) or 11 floats (0x2c) wide and the gaps between the
//     global addresses 0x006ac70c/744/77c (0x38 apart) and 0x006ac7b4/7e0/80c/838/864/890
//     (0x2c apart) confirm it.
//   - observer_command (0x68) is copied as one block of 0x1a dwords three times: camera_update
//     (0x445640) zeroes a stack record with rep stos 0x1a and copies it into the director at
//     +0x58, observer_set_command (0x447ab0) copies it into the observer at +0x08, and the
//     observer constructor zeroes the observer copy with a 0x1a loop.
//   - director (0xf8): the base is 0x006ac560 (FUN_004455f0 NULL-tests i*0xf8 + 0x006ac560,
//     i.e. the returned struct pointer) and the last field, the fourth look axis at +0xec,
//     ends exactly at 0x006ac658 where observer_dt starts.
//   - camera_input_axis_definition (0x1c) is read out of .data at 0x00686a28 (four rows; the
//     smoothing loop 0x446170 steps esi by 0x1c, camera_initialize steps the reset value by 7
//     dwords).
//   - The mode name table at 0x00686a10 ("following", "orbiting", "flying", "editor",
//     "first person") matches the switch on director_globals.mode in camera_update, and the
//     flying sub-mode names at 0x00686ac0 ("flying camera", "orbiting camera") match the two
//     entry update table at 0x00686aa8 (0x4465d0 flying, 0x446870 orbiting).
//
// Types this module operates on that already have a definition, and are therefore NOT
// redefined here:
//   types/tags.h     ScenarioCutsceneCameraPoint (0x68: position 0x28, orientation 0x34,
//                    field_of_view 0x40; Scenario.cutscene_camera_points.pointer is +0x4f4),
//                    ScenarioPlayerStartingLocation (0x34, position + facing; the flying camera
//                    home comes from Scenario +0x354/+0x358), UnitSeat (0x11c; flags 0x00,
//                    camera fields from 0x84), Unit (camera fields from 0x1a8),
//                    UnitCameraTrack (0x1c), CameraTrack (0x30), CameraTrackControlPoint (0x3c),
//                    ModelAnimationsAnimation (0xb4; frame_count +0x22 is the camera animation
//                    length)
//   types/memory.h   datum_index, data_iterator (0x445240 / 0x4452c0 walk the players)
//   types/game.h     player (0x200: team 0x20, unit 0x34), player_globals (local_players at
//                    +0x04), camera_basis_out (0x18, filled by 0x472020 for the first person,
//                    third person and orbiting cameras)
//   types/input.h    mouse_state (0x006b180c live / 0x006b1828 neutral; the look input reads
//                    its first three dwords and the byte at +0x0d)
//   types/rasterizer.h render_camera (0x686aa4 points at 0x007c3100, whose +0x14 is the
//                    render_camera; the flying camera copies position +0x00 and forward +0x0c)
//
// Two other headers already carry partial views of the observer and director arrays; they are
// consistent with this header and are not changed:
//   types/sound.h  sound_observer_camera is observer_camera (+0x74 of observer) followed by the
//                  rest of the 0x29c stride, read from 0x006ac6d0.
//   types/game.h   lists 0x006ac5b0 as a byte array with stride 0xf8: that is director +0x50,
//                  so its bytes +1 and +2 are director.suppress_look_update and
//                  director.look_input_consumed.
#pragma pack(push, 1)
typedef unsigned char uint8_t; typedef signed char int8_t; typedef unsigned short uint16_t; typedef short int16_t;
typedef unsigned int uint32_t; typedef int int32_t;

// ---------------------------------------------------------------------------
// constants
// ---------------------------------------------------------------------------
typedef enum camera_constants {
    k_director_size = 0xf8,                      // imul reg,reg,0xf8 on every director access
    k_observer_size = 0x29c,                     // imul reg,reg,0x29c; header/trailer 0x298 apart
    k_observer_command_size = 0x68,              // the 0x1a dword block moves
    k_camera_input_size = 0x24,                  // 0x445f90 zeroes 0x12 int16s
    k_camera_local_player_count = 1,             // every per player loop and bound test here is
                                                 // "< 1" (same bound as k_maximum_local_players
                                                 // in types/game.h)
    k_director_camera_mode_count = 5,            // the name table at 0x00686a10
    k_camera_input_axis_count = 4,               // 0x446170 loop count, camera_initialize
    k_observer_parameter_count = 5,              // position, focus offset, distance, fov,
                                                 // orientation: every spline loop counts to 5
    k_observer_parameter_float_count = 14,       // sum of 0x00686ae0 {3,3,1,1,6}
    k_observer_derivative_float_count = 11,      // sum of 0x00686aec {3,3,1,1,3}
    k_observer_signature = 0x72616421,           // bytes 21 64 61 72; written at +0x000 and
                                                 // +0x298 by observer_new
    k_flying_camera_mode_count = 2,              // 0x00686aa8 update table
    k_camera_script_ticks_per_second = 30        // camera_set divides its tick count by 0x1e
    // Float constants (not expressible as enum values):
    //   70 degrees 1.2217305 (0x3f9c61aa)  default field of view of every director pov
    //   50 degrees 0.8726646 (0x3f5f66f3)  observer_new default field of view
    //   0.0001                              observer dt camera_set uses for its forced update
    //   5000.0                              observer position / distance clamp (0x448900)
    //   0.001 .. pi/2                       observer field of view clamp (0x448900)
    //   0x006572c4 float[5] {1500, 1500, 1e5, 1e5, 1e5}  per channel acceleration limit that
    //                                       0x447e40 treats as a spline blow-up
    //   0x006572dc 0.174                    collision probe radius factor (0x448d40)
} camera_constants;

// director_globals.mode (int16 at 0x006ac55c): indexes the name table at 0x00686a10; the
// switch in camera_update (jump table 0x0044586c) sends 0 and 1 to the gameplay chooser
// 0x445dc0, 2 to the flying camera 0x445f40 and 4 back to first person; 3 has no case.
typedef enum director_camera_mode {
    _director_camera_mode_following = 0,
    _director_camera_mode_orbiting = 1,
    _director_camera_mode_flying = 2,
    _director_camera_mode_editor = 3,
    _director_camera_mode_first_person = 4
} director_camera_mode;

// camera_script_globals.mode (int16 at 0x006869d2), the switch in the scripted camera pov
// procedure 0x444d50. Written by 0x444c00 (0), 0x444b30 (1), 0x47ef40 (2), 0x47ef90 (3).
typedef enum camera_script_mode {
    _camera_script_mode_none = -1,               // .data initial value
    _camera_script_mode_point = 0,               // cutscene camera point, optionally relative to
                                                 // camera_script_globals.object
    _camera_script_mode_animation = 1,           // camera animation (0x686a08 / 0x686a0c)
    _camera_script_mode_first_person = 2,        // first person through camera_script_globals.object
    _camera_script_mode_dead = 3                 // dead camera orbiting camera_script_globals.object
} camera_script_mode;

// director.camera_type (int16 at director +0x56), recomputed by camera_get_type_for_player
// (0x445ac0) from the pov procedure. A first person procedure with a transition still running
// keeps the previous value.
typedef enum director_camera_type {
    _director_camera_type_first_person = 0,      // pov == 0x446d60
    _director_camera_type_third_person = 1,      // pov == 0x447370
    _director_camera_type_scripted = 2,          // pov == 0x444d50
    _director_camera_type_other = 3              // any other pov (dead, flying, editor)
} director_camera_type;

// director.seat_camera_state (int16 at director +0x54), the out value of 0x445b20. Set only
// when the unit is seated (object +0x11c parent valid) in a parent whose seat has flag 0x40;
// 1 and 3 mirror unit animation states 0x1a / 0x1b (types/units.h seat_enter / seat_exit).
typedef enum director_seat_camera_state {
    _director_seat_camera_none = 0,
    _director_seat_camera_entering = 1,          // unit +0x2a3 == 0x1a
    _director_seat_camera_seated = 2,
    _director_seat_camera_exiting = 3            // unit +0x2a3 == 0x1b
} director_seat_camera_state;

// flying_camera_mode (int16 at 0x006f1828): indexes 0x00686aa8 and 0x00686ac0.
typedef enum flying_camera_mode {
    _flying_camera_mode_flying = 0,              // 0x4465d0
    _flying_camera_mode_orbiting = 1             // 0x446870
} flying_camera_mode;

// observer parameter channels: the five entries of the 0x00686ae0 / 0x00686aec tables, of
// observer_command.interpolation_flags / channel_times and of the observer timer array.
typedef enum observer_parameter {
    _observer_parameter_position = 0,            // 3 floats
    _observer_parameter_focus_offset = 1,        // 3 floats
    _observer_parameter_distance = 2,            // 1 float
    _observer_parameter_field_of_view = 3,       // 1 float
    _observer_parameter_orientation = 4          // 6 floats (forward, up) as a parameter,
                                                 // 3 floats (axis * angle) as a derivative
} observer_parameter;

// observer_command.flags
typedef enum observer_command_flags {
    _observer_command_valid_bit = 0x01,          // the command carries a camera; camera_update
                                                 // copies it into the director only when set and
                                                 // observer_set_command ignores it otherwise
    _observer_command_snap_bit = 0x08,           // expired channels stop dead (velocity zeroed,
                                                 // 0x448010); camera_update sets it for the first
                                                 // command an observer ever gets, the scripted
                                                 // camera always sets it, the flying camera ORs 9
    _observer_command_no_collision_bit = 0x10,   // 0x448900 skips the 0x448d40 geometry pushout
    _observer_command_frozen_bit = 0x20          // 0x447b50 does not advance the spline; the
                                                 // scripted camera sets it while the game is
                                                 // paused (game time globals +2)
} observer_command_flags;

// observer_command.interpolation_flags[]
typedef enum observer_interpolation_flags {
    _observer_interpolation_own_time_bit = 0x01, // use channel_times[i]; clear = use timer
    _observer_interpolation_exact_bit = 0x02     // keep channel_times[i] even when shorter than
                                                 // the running timer; with snap, stop dead
                                                 // (values 1 and 3 are the only ones written)
} observer_interpolation_flags;

// camera_input_axis_definition key bits: the byte 0x445f90 builds from input_get_key_state
// (0x490b50), one bit per engine key index, while the debug look is active (mouse device,
// director_camera_switching, a pov other than first / third person, mouse button_frames[1]
// held). Key 0x1d is read first but only feeds the return value of 0x445f90, it gates nothing.
typedef enum camera_input_key_bits {
    _camera_input_key_0_bit = 0x01,              // key 0x20
    _camera_input_key_1_bit = 0x02,              // key 0x2e
    _camera_input_key_2_bit = 0x04,              // key 0x2d
    _camera_input_key_3_bit = 0x08,              // key 0x2f
    _camera_input_key_4_bit = 0x10,              // key 0x22
    _camera_input_key_5_bit = 0x20,              // key 0x30
    _camera_input_key_6_bit = 0x40,              // key 0x23
    _camera_input_key_7_bit = 0x80               // key 0x31
    // UNSURE: on the engine key numbering of types/input.h (tab 0x1e) these would be
    // w, s, a, d, r, f, t, g. Not confirmed against the 0x0065bd58 table.
} camera_input_key_bits;

// ---------------------------------------------------------------------------
// camera_script_globals  (0x40 bytes, .data 0x006869d0)
// The hs camera state. Writers: camera_set 0x444c00 (point, position/forward/up/fov from
// ScenarioCutsceneCameraPoint +0x28/+0x34/+0x40 through matrix4x3_from_euler_angles), the
// camera animation setter 0x444b30, camera_set_first_person 0x47ef40, camera_set_dead
// 0x47ef90, camera_control 0x445cc0 (byte 0x00). Reader: the scripted pov procedure 0x444d50.
// Widths from the stores: 0x02 / 0x04 / 0x3c are WORD stores, 0x00 / 0x01 BYTE stores.
// ---------------------------------------------------------------------------
typedef struct camera_script_globals {
    uint8_t camera_control;          // 0x00 hs camera_control; 0x445cc0 also stores it through
                                     //      the byte pointer at 0x0087bc0c. Read by 0x50ea50
    uint8_t changed;                 // 0x01 set by every setter, cleared by 0x444d50 after
                                     //      use; mode 3 reinitialises the dead camera while set
    int16_t mode;                    // 0x02 camera_script_mode, .data -1
    int16_t camera_point_index;      // 0x04 ScenarioCutsceneCameraPoint index, -1 for animations
    int16_t unknown_06;              // 0x06 never referenced
    float time_remaining;            // 0x08 seconds: ticks / 30 on set, decremented by
                                     //      dt * time scale per update, floored at 0; becomes
                                     //      the observer_command timer
    Point3D position;                // 0x0c
    Vector3D forward;                // 0x18 matrix4x3 forward row of the point orientation
    Vector3D up;                     // 0x24 matrix4x3 up row
    float field_of_view;             // 0x30 the point fov, 70 degrees when the point has 0
    datum_index object;              // 0x34 object the point is relative to, or the unit of
                                     //      first person / dead modes; -1 none
    datum_index animation_tag;       // 0x38 model_animations tag handle (animation mode)
    int16_t animation_index;         // 0x3c ModelAnimationsAnimation index (stride 0xb4)
    int16_t unknown_3e;              // 0x3e never referenced
} camera_script_globals;             // size 0x40

// ---------------------------------------------------------------------------
// camera_input_axis_definition  (0x1c bytes, .data 0x00686a28, four rows)
// One smoothed debug camera axis. FUN_00446170 (with its split tail 0x4462a0) reads the three
// key bit indices at +0x00/+0x02/+0x04 as int16 (-1 = none), the floats at +0x08..+0x14 as
// [esi+0x4] / [esi+0x8] / [esi+0xc] / [esi+0x10] with esi = row + 4; camera_initialize reads
// +0x0c as the reset value. The .data rows are
//   0: keys 5/4, accel 0.15,  axis feeds camera_input.move_up
//   1: keys 6/7, accel 0.075, axis feeds camera_input.roll_delta
//   2: keys 1/0, accel 0.075, axis feeds camera_input.move_forward
//   3: keys 3/2, accel 0.075, axis feeds camera_input.move_left
// all with reset value 0 and bounds -FLT_MAX..FLT_MAX.
// ---------------------------------------------------------------------------
typedef struct camera_input_axis_definition {
    int16_t decrease_key_bit;        // 0x00 bit index into camera_input_key_bits; -1 none
    int16_t increase_key_bit;        // 0x02
    int16_t reset_key_bit;           // 0x04 when held the value is forced to reset_value
    int16_t unknown_06;              // 0x06 .data 0 in every row
    float acceleration;              // 0x08 velocity step = dt * acceleration * scale * 25
    float reset_value;               // 0x0c
    float minimum_value;             // 0x10
    float maximum_value;             // 0x14
    uint8_t scale_by_zoom;           // 0x18 multiply the step by director.look_scale. Only row
                                     //      0 is ever read: the loop loads the absolute byte
                                     //      0x00686a40 on every iteration
    uint8_t unknown_19[3];           // 0x19
} camera_input_axis_definition;      // size 0x1c

// ---------------------------------------------------------------------------
// camera_input_axis_state  (0x0c bytes, director +0xc8, four of them)
// camera_initialize writes reset_value / 0 / 0 in steps of 3 dwords from 0x006ac628;
// 0x446170 treats [edx] / [edx+4] / [edx+8] as value, velocity and per tick delta.
// ---------------------------------------------------------------------------
typedef struct camera_input_axis_state {
    float value;                     // 0x00 clamped to minimum_value..maximum_value
    float velocity;                  // 0x04 damped by (1 - clamp(dt*5, 0, 1)) each tick
    float delta;                     // 0x08 dt * velocity; added into camera_input
} camera_input_axis_state;           // size 0x0c

// ---------------------------------------------------------------------------
// camera_input  (0x24 bytes, camera_update stack local_24, second argument of every director
// pov procedure). Built by 0x445f90 (ESI): zeroes 0x12 int16s, stores the local player index
// at +0x00 and dt at +0x04, and when backspace debug look is active fills the rest.
// Readers: 0x446870 / 0x447370 (+0x02, +0x08, +0x0c, +0x20), 0x4465d0 and 0x446e90
// (+0x02, +0x08..+0x1c), 0x446d60 (+0x00), 0x445380 / 0x444d50 (+0x04).
// ---------------------------------------------------------------------------
typedef struct camera_input {
    int16_t local_player_index;      // 0x00
    uint8_t has_look_input;          // 0x02 set only when the debug look branch ran
    uint8_t unknown_03;              // 0x03 zeroed, never read
    float dt;                        // 0x04 seconds, director_globals.dt
    float yaw_delta;                 // 0x08 mouse x * -0.0031415927
    float pitch_delta;               // 0x0c mouse y * 0.0031415927
    float roll_delta;                // 0x10 += axis 1 delta; the flying camera applies it only
                                     //      while flying_camera_allow_roll is set
    float move_forward;              // 0x14 += axis 2 delta
    float move_left;                 // 0x18 += axis 3 delta
    float move_up;                   // 0x1c += axis 0 delta
    float zoom_delta;                // 0x20 mouse_state.wheel (+0x08, int32 converted); third person
                                     //      distance -= zoom * 0.05, orbiting -= zoom / 3
} camera_input;                      // size 0x24

// ---------------------------------------------------------------------------
// observer_command  (0x68 bytes)
// What a director pov procedure produces and the observer eases towards. Lives at director
// +0x58 (committed copy) and observer +0x08 (the copy the spline runs against); built on the
// stack by camera_update. Every field below is written by name by at least one pov procedure:
// flags/position/focus_offset/distance/field_of_view/forward/up by all of them (0x444d50,
// 0x445380, 0x446b70, 0x446870, 0x447370, 0x4465d0, 0x446e90), velocity by 0x445380 (zero),
// timer and the interpolation arrays by 0x445640, 0x446d60, 0x447370, 0x445380.
// The 14 floats 0x04..0x3b are exactly the observer_parameters layout (observer_new copies
// forward/up/fov between them; 0x448210 copies them 1:1 when a channel expires).
// ---------------------------------------------------------------------------
typedef struct observer_parameters {
    Point3D position;                // 0x00 focus point
    Vector3D focus_offset;           // 0x0c camera relative: x along the horizontal forward,
                                     //      y along its perpendicular, z world up (0x448900)
    float distance;                  // 0x18 pull back along -forward, clamped 0..5000
    float field_of_view;             // 0x1c radians, clamped 0.001..pi/2 by the observer
    Vector3D forward;                // 0x20 unit length (0x448210 re-orthonormalises)
    Vector3D up;                     // 0x2c
} observer_parameters;               // size 0x38 (14 floats)

typedef struct observer_command {
    uint32_t flags;                  // 0x00 observer_command_flags
    observer_parameters parameters;  // 0x04
    Vector3D velocity;               // 0x3c world units per tick; the spline adds 30 * it to
                                     //      the linear position coefficient (0x447be0)
    float timer;                     // 0x48 seconds to reach the command, for channels whose
                                     //      interpolation flag bit 0 is clear
    uint8_t interpolation_flags[5];  // 0x4c observer_interpolation_flags per observer_parameter
    uint8_t unknown_51[3];           // 0x51 never referenced
    float channel_times[5];          // 0x54 per observer_parameter seconds; in the observer copy
                                     //      these are the running countdowns
} observer_command;                  // size 0x68

// ---------------------------------------------------------------------------
// observer_parameter_derivatives  (0x2c bytes = 11 floats)
// The derivative-space layout of observer_parameters: orientation collapses to one axis*angle
// vector (0x448710 builds it with matrix4x3_inverse / quaternion_from_matrix4x3, 0x448880
// applies it with two vector3d_rotate_about_axis calls).
// ---------------------------------------------------------------------------
typedef struct observer_parameter_derivatives {
    Vector3D position;               // 0x00
    Vector3D focus_offset;           // 0x0c
    float distance;                  // 0x18
    float field_of_view;             // 0x1c
    Vector3D rotation;               // 0x20 axis * angle, angle folded into -pi..pi
} observer_parameter_derivatives;    // size 0x2c

// ---------------------------------------------------------------------------
// observer_camera  (0x3c bytes, observer +0x74; camera_get_globals_for_player 0x4479a0
// returns its address, 0x006ac6d0 + i*0x29c). Written only by 0x448900 (position, velocity,
// forward, up, fov), 0x447a60 and 0x448900 (leaf / cluster) and observer_new (defaults:
// origin, -1, -1, zero, global forward, global up, 50 degrees).
// Readers outside the module: sound (types/sound.h sound_observer_camera), 0x453330,
// 0x4596f0, 0x459900, cheat_teleport_to_camera 0x45a630, 0x53f150.
// ---------------------------------------------------------------------------
typedef struct observer_camera {
    Point3D position;                // 0x00 focus - distance * forward, clamped to +-5000
    int32_t leaf_index;              // 0x0c 0x5013a0 result for position, -1 outside
    int16_t cluster_index;           // 0x10 WORD store in 0x447a60; 0x448900 stores the dword
                                     //      0x10..0x13 in one move
    int16_t unknown_12;              // 0x12 upper half of that dword store, never read
    Vector3D velocity;               // 0x14 minus the position velocity of the spline
    Vector3D forward;                // 0x20
    Vector3D up;                     // 0x2c
    float field_of_view;             // 0x38 radians
} observer_camera;                   // size 0x3c

// ---------------------------------------------------------------------------
// observer  (0x29c bytes, .bss 0x006ac65c, one per local player)
// Offsets pinned by the base-register accesses of 0x448900 (ebx = 0x006ac65c + i*0x29c:
// +0x08, +0x74..+0xb0, +0xb0..+0xe8, +0xe8..+0xf0), by 0x447b50 (+0x04, +0x0c, +0x5c, +0xb0,
// +0x260) and by the absolute addresses in the spline routines.
// Spline: for each channel with time T = timer remaining, 0x447be0 fits a quintic from the
// current velocity (+0xe8), acceleration (+0x120) and remaining offset (+0x260) and stores its
// coefficients highest power first at +0x158..+0x260; 0x448210 evaluates value, 0x448010
// velocity (first derivative) and 0x447e40 acceleration (second derivative) at the new T.
// The coefficients and derivatives are in "remaining offset" space, so they run towards zero
// and have the opposite sign of the world motion (0x448900 publishes -velocity).
// ---------------------------------------------------------------------------
typedef struct observer {
    uint32_t header_signature;                       // 0x000 k_observer_signature
    observer_command *command;                       // 0x004 camera_update points it at the
                                                     //       director +0x58 copy every tick
    observer_command current_command;                // 0x008 observer_set_command copies the
                                                     //       command here; +0x5c (its
                                                     //       channel_times) are the running
                                                     //       countdowns decremented by
                                                     //       observer_dt in 0x447b50
    uint8_t updated;                                 // 0x070 set by the observer update entries
                                                     //       (0x444d12, 0x4478ae), cleared by
                                                     //       camera_update
    uint8_t has_command;                             // 0x071 0 from observer_new; camera_update
                                                     //       sets it after forcing the snap bit
                                                     //       on the first command
    int16_t unknown_072;                             // 0x072 never referenced
    observer_camera camera;                          // 0x074 the published result
    observer_parameters parameters;                  // 0x0b0 current eased state (0x006ac70c)
    observer_parameter_derivatives velocity;         // 0x0e8 first derivative (0x006ac744)
    float unknown_114[3];                            // 0x114 the 0x38 wide slot has only 11
                                                     //       live floats; never referenced
    observer_parameter_derivatives acceleration;     // 0x120 second derivative (0x006ac77c)
    float unknown_14c[3];                            // 0x14c never referenced
    observer_parameter_derivatives coefficient_t5;   // 0x158 (0x006ac7b4)
    observer_parameter_derivatives coefficient_t4;   // 0x184 (0x006ac7e0)
    observer_parameter_derivatives coefficient_t3;   // 0x1b0 (0x006ac80c)
    observer_parameter_derivatives coefficient_t2;   // 0x1dc (0x006ac838)
    observer_parameter_derivatives coefficient_t1;   // 0x208 (0x006ac864) initial velocity term
    observer_parameter_derivatives coefficient_t0;   // 0x234 (0x006ac890) = remaining offset
    observer_parameter_derivatives remaining_offset; // 0x260 target - current, 0x448710
    float unknown_28c[3];                            // 0x28c never referenced
    uint32_t trailer_signature;                      // 0x298 k_observer_signature
} observer;                                          // size 0x29c

// ---------------------------------------------------------------------------
// director camera mode data  (the union at director +0x0c, first argument of every pov
// procedure; camera_update pushes 0x006ac56c). The largest member seen is 0x30 bytes; bytes
// 0x30..0x3f of the union are never referenced, and the union is closed at 0x40 because
// camera_initialize zeroes director +0x4c as a separate dword.
// ---------------------------------------------------------------------------

// first person (0x446d60): the only field it touches.
typedef struct first_person_camera_data {
    float field_of_view;             // 0x00 last fov from 0x471f90; a change sets the fov
                                     //      channel to 0.18 s. 0x445c00 / 0x445dc0 zero it
} first_person_camera_data;          // size 0x04

// third person (0x447370). 0x445c00 / 0x445dc0 / 0x445cc0 initialise it: bytes 0..3 = 0,
// word 4 = 0, +8 = -1, word +0xc = -1, +0x10 = +0x14 = 0, +0x18 = 1.0.
typedef struct third_person_camera_data {
    uint8_t initialized;             // 0x00 set to 1 at the end of every compute
    uint8_t unknown_01;              // 0x01 zeroed only
    uint8_t crouch_or_jump;          // 0x02 last (unit control flags +0x208 & 3) != 0, i.e.
                                     //      crouch or jump (types/units.h); a change eases the
                                     //      focus offset channel over at least 0.5 s
    uint8_t unknown_03;              // 0x03 zeroed only
    int16_t unknown_04;              // 0x04 zeroed only (WORD store)
    int16_t unknown_06;              // 0x06 never referenced
    datum_index unit;                // 0x08 camera_basis_out.unit of the last compute
    int16_t seat_index;              // 0x0c camera_basis_out.seat_index of the last compute
    int16_t unknown_0e;              // 0x0e never referenced
    float yaw_offset;                // 0x10 accumulated look input
    float pitch_offset;              // 0x14 accumulated look input
    float distance_scale;            // 0x18 1.0 initially, zoom adjusts, clamped 0..5
} third_person_camera_data;          // size 0x1c

// dead camera: initialised by dead_camera_new (0x4450e0, Ghidra camera_shake_initialize,
// EAX = this, DX = local player, arg = unit or -1), computed by 0x445380 (Ghidra
// camera_track_compute_pov). The scripted camera mode 3 reuses it.
typedef struct dead_camera_data {
    Point3D focus;                   // 0x00 observer_camera.position at initialisation; used
                                     //      when target_unit is not an object
    float yaw;                       // 0x0c random 0..2pi
    float pitch;                     // 0x10 -(0.4712 + random * 0.6283)
    float distance;                  // 0x14 2 + random * 4
    float field_of_view;             // 0x18 70 degrees
    float transition_time;           // 0x1c 3.0 on a new target; while exactly 3.0 the distance
                                     //      channel snaps; decremented by dt
    datum_index local_player;        // 0x20 the viewing player
    datum_index target_player;       // 0x24 player being watched; cycles through players of
                                     //      the same team (0x445240 / 0x4452c0)
    datum_index target_unit;         // 0x28 unit of target_player, or the scripted object;
                                     //      dead_camera_new seeds it with previous_unit of the
                                     //      viewing player (player +0x38), the body just lost
    float retarget_time;             // 0x2c 3.0 (15.0 in multiplayer) before looking for the
                                     //      next teammate; FLT_MAX for scripted targets
} dead_camera_data;                  // size 0x30

// editor camera (0x446e90, installed by camera_debug_load_from_file 0x445940) and the flying
// camera (0x4465d0, initialised by 0x446350). The saved copies at 0x006f1830 / 0x006f1850 are
// 7 dword block moves of this record.
typedef struct editor_camera_data {
    Point3D position;                // 0x00
    float yaw;                       // 0x0c
    float pitch;                     // 0x10 clamped to +-1.5676548 (0x00672f58 / 0x00672f5c)
    float roll;                      // 0x14
    float field_of_view;             // 0x18 70 degrees from 0x446350; loaded from camera.txt
} editor_camera_data;                // size 0x1c

// orbiting sub-mode of the flying camera (0x446870): the same 0x1c record read with a
// different meaning; 0x446a10 seeds it as {0, 1.0, 0, yaw, pitch} when nothing was saved.
typedef struct orbiting_camera_data {
    float unknown_00;                // 0x00 seeded 0, never read
    float distance;                  // 0x04 -= zoom / 3, floored at 0.6
    float unknown_08;                // 0x08 seeded 0, never read
    float yaw;                       // 0x0c
    float pitch;                     // 0x10 clamped to +-1.2566371 (0x00673010 / 0x00673014)
    float unknown_14;                // 0x14 never read
    float unknown_18;                // 0x18 never read
} orbiting_camera_data;              // size 0x1c

typedef union director_camera_data {
    first_person_camera_data first_person;
    third_person_camera_data third_person;
    dead_camera_data dead;
    editor_camera_data editor;       // also the flying camera
    orbiting_camera_data orbiting;
    uint8_t raw[0x40];
} director_camera_data;              // size 0x40

// pov procedure: (mode data, input, out command), cdecl, caller pops (camera_update 0x4456f8)
typedef void (*director_pov_proc)(director_camera_data *data, camera_input *input,
    observer_command *command);

// ---------------------------------------------------------------------------
// director  (0xf8 bytes, .bss 0x006ac560, one per local player)
// ---------------------------------------------------------------------------
typedef struct director {
    int16_t unknown_00;                      // 0x00 WORD; only store is 2 by
                                             //      camera_debug_load_from_file, never read
    int16_t unknown_02;                      // 0x02 never referenced
    float transition_time;                   // 0x04 seconds; 1.0 on first/third person and dead
                                             //      camera switches, decays by dt; raises the
                                             //      command timer, and a first person switch
                                             //      under 0.2 s snaps position and distance
    director_pov_proc pov_proc;              // 0x08 0x446d60 first person, 0x447370 third
                                             //      person, 0x445380 dead, 0x444d50 scripted,
                                             //      0x4464f0 flying, 0x446e90 editor
    director_camera_data data;               // 0x0c
    int32_t unknown_4c;                      // 0x4c zeroed by camera_initialize, never read
    uint8_t unknown_50;                      // 0x50 zeroed by camera_initialize, never read
    uint8_t suppress_look_update;            // 0x51 set by 0x445f90 with debug look input;
                                             //      game_engine 0x471ae0 reads it
    uint8_t look_input_consumed;             // 0x52 set by 0x445f90 / 0x446870; 0x471ae0 and
                                             //      timedemo 0x4c6f30 read it
    uint8_t unknown_53;                      // 0x53 never referenced
    int16_t seat_camera_state;               // 0x54 director_seat_camera_state (0x445c00)
    int16_t camera_type;                     // 0x56 director_camera_type (0x445ac0)
    observer_command command;                // 0x58 last command with the valid bit; the
                                             //      observer points at it
    uint8_t unknown_c0;                      // 0x0c0 zeroed on every pov switch, never read
    uint8_t unknown_c1[3];                   // 0x0c1 never referenced
    float look_scale;                        // 0x0c4 1.0 on every pov switch; 0x446170
                                             //       multiplies it by 0x6283c0 and clamps it to
                                             //       0.01..50
    camera_input_axis_state axes[4];         // 0x0c8 camera_input_axis_definition rows 0..3
} director;                                  // size 0xf8

// ---------------------------------------------------------------------------
// director_globals  (8 bytes, .bss 0x006ac558)
// ---------------------------------------------------------------------------
typedef struct director_globals {
    float dt;                        // 0x00 camera_update argument; the look input and the
                                     //      axis smoothing read it
    int16_t mode;                    // 0x04 director_camera_mode (WORD 0x006ac55c)
    uint8_t mode_changed;            // 0x06 BYTE 0x006ac55e; passed to 0x445dc0 (resets to
                                     //      first person) and cleared every update
    uint8_t unknown_07;              // 0x07 never referenced
} director_globals;                  // size 0x08

// ---------------------------------------------------------------------------
// flying_camera_home  (0x14 bytes, .bss 0x006f1800)
// 0x446350 fills it once (guarded by flying_camera_home_initialized) from the first
// ScenarioPlayerStartingLocation (position, facing) or zero, pitch 0.
// ---------------------------------------------------------------------------
typedef struct flying_camera_home {
    Point3D position;                // 0x00
    float yaw;                       // 0x0c
    float pitch;                     // 0x10
} flying_camera_home;                // size 0x14

// flying camera transition procedure (0x4469a0 enter flying, 0x446a10 enter orbiting):
// cdecl, one argument, the director mode data.
typedef void (*flying_camera_transition_proc)(editor_camera_data *data);

// ---------------------------------------------------------------------------
// unit_camera_properties  (0x58 bytes, a view into tag data, not a separate tag block)
// FUN_00447110 (EAX = unit handle) returns a pointer to either UnitSeat +0x84 (when the unit
// is seated and the seat flags & 0x15) or Unit +0x1a8, and FUN_00447190 (ECX = that pointer)
// reads +0x4c / +0x50 as the camera_tracks count / pointer. types/tags.h lays both copies out
// field for field (camera_marker_name 0x84 / 0x1a8 .. camera_tracks 0xd0 / 0x1f4) but has no
// named struct for the shared run, so it is named here. Unit additionally has
// camera_field_of_view and camera_stiffness in front of it, which the seat lacks.
// ---------------------------------------------------------------------------
typedef struct unit_camera_properties {
    TagString camera_marker_name;            // 0x00
    TagString camera_submerged_marker_name;  // 0x20
    float pitch_auto_level;                  // 0x40
    float pitch_range[2];                    // 0x44
    TagReflexive camera_tracks;              // 0x4c UnitCameraTrack; when empty (or its first
                                             //      track handle is -1) 0x447190 falls back to
                                             //      the globals default_unit_camera_track
} unit_camera_properties;                    // size 0x58

// ---------------------------------------------------------------------------
// globals this module owns
// ---------------------------------------------------------------------------
// .data
// global 0x006869d0: camera_script_globals camera_script
// global 0x00686a10: char *director_camera_mode_names[5]      following, orbiting, flying,
//                    editor, first person; no code reference in this build
// global 0x00686a24: uint32_t unknown_686a24                  .data 0, unreferenced
// global 0x00686a28: camera_input_axis_definition camera_input_axes[4]
// global 0x00686a98: uint8_t director_camera_switching        hs global (the hs table row at
//                    0x0068b1fc names it); camera_update overwrites it every tick with
//                    input mode_flags (0x00712542) == 1, 0x445f90 gates the debug look on it
// global 0x00686a99: uint8_t unknown_686a99[3]                unreferenced
// global 0x00686a9c: float flying_camera_speed                1.0; scales the flying move
// global 0x00686aa0: datum_index flying_camera_attached_object -1; set by 0x446470, the
//                    flying camera moves with it
// global 0x00686aa4: void *flying_camera_render_frame         .data 0x007c3100 (render frame
//                    index; +0x14 is the render_camera the flying camera copies)
// global 0x00686aa8: director_pov_proc flying_camera_update_procs[2]   0x4465d0, 0x446870
// global 0x00686ab0: flying_camera_transition_proc flying_camera_transition_procs[2][2]
//                    {0, 0}, {0x4469a0, 0x446a10}; only [mode][1] is called
//                    (jmp / call [edx*8+0x686ab4])
// global 0x00686ac0: char *flying_camera_mode_names[2]        flying camera, orbiting camera
// global 0x00686ac8: char *flying_camera_transition_names[2]  exiting, entering
// global 0x00686ad0: float unknown_686ad0[4]                  31.29, 12.78, 5.13, 2.05;
//                    unreferenced
// global 0x00686ae0: int16_t observer_parameter_float_counts[5]       {3,3,1,1,6}
// global 0x00686aea: int16_t unknown_686aea                   padding
// global 0x00686aec: int16_t observer_derivative_float_counts[5]      {3,3,1,1,3}
// global 0x00686af6: int16_t unknown_686af6                   padding; 0x00686af8 belongs to
//                    another module
// .bss
// global 0x006ac558: director_globals camera_director_globals
// global 0x006ac560: director directors[1]
// global 0x006ac658: float observer_dt                        observer update argument
//                    (0x447880); camera_set forces 0.0001
// global 0x006ac65c: observer observers[1]
// global 0x006f17f8: director_pov_proc director_last_pov_proc  pov of the previous update; a
//                    first person compute right after another pov zeroes the fov ease time
// global 0x006f17fc: uint8_t unknown_6f17fc                   unreferenced
// global 0x006f17fd: uint8_t flying_camera_follow_script      UNSURE name: while set and
//                    there is no look input the flying pov defers to the scripted pov; with
//                    input it resumes from the render camera and ORs the command flags with 9
// global 0x006f17fe: uint8_t flying_camera_allow_roll         roll input is ignored while 0
// global 0x006f17ff: uint8_t flying_camera_home_initialized
// global 0x006f1800: flying_camera_home flying_camera_home_location
// global 0x006f1814: editor_camera_data *flying_camera_data   player 0 mode data, set by 0x446350
// global 0x006f1818: uint32_t unknown_6f1818                  unreferenced
// global 0x006f181c: Vector3D flying_camera_attached_offset   position - attached object origin
// global 0x006f1828: int16_t flying_camera_current_mode       flying_camera_mode; no writer in
//                    code (debug only)
// global 0x006f182a: uint8_t unknown_6f182a[6]                unreferenced
// global 0x006f1830: editor_camera_data flying_camera_saved_flying     0x446a10
// global 0x006f184c: uint32_t unknown_6f184c                  unreferenced
// global 0x006f1850: orbiting_camera_data flying_camera_saved_orbiting  0x4469a0
// global 0x006f186c: uint8_t flying_camera_saved_orbiting_valid         0x4469a0 sets it
//
// ---------------------------------------------------------------------------
// globals this module reads or writes but does not own
// ---------------------------------------------------------------------------
// 0x00746f8c  Scenario *global_scenario (cache)   +0x354/+0x358 player_starting_locations,
//             +0x4f4 cutscene_camera_points.pointer
// 0x00746fa0  Globals *global_globals (cache)     +0x108 camera block pointer; its +0x0c is the
//             default_unit_camera_track tag handle
// 0x00746f90 / 0x00746f9c  global_collision_bsp / global_structure_bsp (scenario)  leaf lookup
//             0x5013a0 on the collision BSP, cluster table at structure bsp +0xe4
// 0x0087a478  player_globals (game)   +0x04 local_players[0]
// 0x0087a480  data_array *player_data (game)   stride 0x200; +0x20 team, +0x34 unit,
//             +0x38 previous_unit (dead camera target), +0xae deaths
// 0x006b145c  player control array (game) stride 0x40, +0x10 unit, +0x1c/+0x20 desired yaw /
//             pitch (third person base angles)
// 0x008603b0  object headers (objects)     unit +0x11c parent, +0x208 control flags,
//             +0x23c aiming vector, +0x2a3 animation state, +0x2f0 seat index
// 0x0087bc14  tag_instances (cache)
// 0x0087bc0c  uint8_t *hs_camera_control_pointer (hs)   camera_control mirror; saved with the
//             game state and re-applied by director_game_state_loaded (0x445560)
// 0x006f1d6c  game_time_globals *game_time (game)   +0x02 paused, +0x18 speed, +0x1c
//             leftover_time (observer_update passes it as the movement prediction fraction)
// 0x006f1d20  game_engine_definition *current_game_engine (game)   non-NULL (multiplayer)
//             selects the 15 s dead camera retarget time over 3 s
// 0x00719720  int16_t network_game_mode (network)
// 0x00719cd4  uint32_t random_seed (types/cache.h)  linear congruential 0x19660d / 0x3c6ef35f
// 0x006b180c / 0x006b1828 / 0x006b15f9 / 0x006b1804  mouse state (input)
// 0x00712542  input mode_flags (input)
// 0x00696714 global_origin3d_pointer, 0x006966f8 global_zero_vector3d_pointer (both point at
//             0x0065c230 {0,0,0}), 0x00696718 forward, 0x00696720 up (math)
// 0x00746f90  ModelCollisionGeometryBSP *global_collision_bsp (scenario)   bsp3d_node_find_leaf root
// 0x0069e7c0 / 0x0069e7e0  game_state_after_load_procs[5] / [12] (saved_games): observer_initialize
//             0x447870 and director_game_state_loaded 0x445560
// 0x0069e900  a procedure table slot (0x0069e8dc..) holding observer_update_location 0x447a60

#pragma pack(pop)
