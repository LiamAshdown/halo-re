// Blam input module (halo.exe 1.0.10 retail, 0x48b3e0..0x492340, 80 Ghidra functions).
// Two layers:
//   - input abstraction: the four per-controller binding/sensitivity blocks, the four
//     per-controller action states the player-control code reads, the bind/unbind console
//     commands, the rebind capture (scan) machine, the menu navigation event generator and
//     the small UI event queue; everything from 0x00710328 to 0x00712918 is ONE object,
//     input_abstraction_globals below;
//   - the DirectInput 8 layer: device creation, acquire/unacquire, the per-frame poll and the
//     raw-to-engine conversion of keyboard, mouse and up to eight game controllers, all held in
//     the contiguous .bss run 0x006b15f8..0x006b2d98 listed at the bottom.
//
// Offsets in comments are byte offsets from the struct base. Where the binary itself carries a
// layout it is preferred over the decompiler, and the fact is called out:
//   - input_abstraction_globals (0x25f0): input_state_initialize 0x48b3e0 is
//     "mov ecx,0x97c ; mov edi,0x710328 ; rep stosd", 0x97c dwords.
//   - joystick_raw_state (0xe0): input_system_initialize 0x491a80 builds the 0x50-entry
//     DIOBJECTDATAFORMAT table at 0x00879f60 with offsets 0..0x7c (axes, type 0x80ffff03),
//     0x80..0xbc (POVs, 0x80ffff10) and 0xc0..0xdf (buttons, 0x80ffff0c); the DIDATAFORMAT
//     at 0x0068e51c reads {0x18, 0x10, 1, 0xe0, 0x50, 0x00879f60}, and the poll passes 0xe0 to
//     GetDeviceState.
//   - di_mouse_state2 (0x14): the poll passes 0x14 to GetDeviceState; the data format is
//     c_dfDIMouse2 at 0x0064e1e4 {0x18, 0x10, 2, 0x14, 0x0b, ...}.
//   - joystick_state (0xa0): every copy is rep movsd of 0x28 dwords (0x48b6b0, 0x490aa0,
//     0x490760), and input_joystick_state_process 0x491fd0 writes the three arrays.
//   - input_device (0x240): stride of every table walk (0x90 dwords); the fields are the
//     stores of the DirectInput device enumeration callback 0x491d70 (not a Ghidra function).
//   - input_event_queue (0x10c): input_queue_initialize 0x492250 zeroes 0x43 dwords.
//   - mouse_state (0x1c): the poll error path zeroes the seven dwords 0x006b180c..0x006b1827.
//
// Types this module operates on that already have a definition, and are therefore NOT
// redefined here:
//   types/game.h         local_player_input_state (the four action states at 0x00712498)
//   types/interface.h    player_control_settings (the four binding blocks at 0x00710328),
//                        controls_gamepad_record (the first 0x220 bytes of input_device),
//                        input_guid, ui_key_event (the key ring at 0x006b16fe),
//                        ui_input_event (the event queue records)
//   types/saved_games.h  control_binding_descriptor (the parsed bind string, the scan result
//                        and the last-used table), saved_player_profile (what 0x490280 copies
//                        and what the device-defaults tag profile block holds), the
//                        k_control_* counts and control_device_type / control_input_kind
//   types/tags.h         InputDeviceDefaults (tag data of 0x490110 / 0x4901b0),
//                        UnicodeStringList (the controls_*_names tags)
// So a translation unit needs tags.h, memory.h, math.h, game.h, networking.h, interface.h and
// saved_games.h before this header, as out/phase4/input_smoke.c does. interface.h and
// saved_games.h sort AFTER input.h, so these are forward references for an alphabetical
// CParser run (the same situation as interface.h using s_network_address); see the notes.
//
// Pointer convention: pointer fields inside structs are uint32_t with the pointee type written
// first in the comment (the rasterizer.h / shell.h convention), so the 32 bit sizes hold on
// any host compiler.
//
// Functions in this range that are not real functions or belong elsewhere are listed in
// out/phase4/input_types_notes.md; their types are not defined here.

#pragma pack(push, 1)
typedef unsigned char uint8_t; typedef signed char int8_t; typedef unsigned short uint16_t; typedef short int16_t;
typedef unsigned int uint32_t; typedef int int32_t;

// ---------------------------------------------------------------------------
// input actions (game controls)
// The index every binding slot holds. Names are the 0x10 byte strings of the table at
// 0x0065b730 (0x1b entries, ending at 0x0065b8e0) that input_action_name_to_index 0x48fe60
// scans. 0x00 .. 0x12 are digital: each owns one hold-count byte of
// local_player_input_state::buttons. 0x13 .. 0x1a are the eight half-axes that
// input_accumulate_axis_value 0x48ca10 folds into the four float axes of the same record.
// ---------------------------------------------------------------------------
typedef enum input_action {
    _input_action_jump = 0x00,
    _input_action_switch_grenade = 0x01,
    _input_action_action = 0x02,
    _input_action_switch_weapon = 0x03,
    _input_action_melee = 0x04,
    _input_action_flashlight = 0x05,
    _input_action_throw_grenade = 0x06,
    _input_action_fire = 0x07,
    _input_action_accept = 0x08,          // gamepads bind it through the per-pad button index
    _input_action_back = 0x09,            //   table, see player_control_settings +0x1fe
    _input_action_crouch = 0x0a,
    _input_action_zoom = 0x0b,
    _input_action_showscores = 0x0c,
    _input_action_reload = 0x0d,
    _input_action_exchange_weapon = 0x0e,
    _input_action_say = 0x0f,
    _input_action_sayteam = 0x10,
    _input_action_sayvehicle = 0x11,
    _input_action_screenshot = 0x12,
    _input_action_forward = 0x13,         // + throttle_x (+0x14), rate settings +0x810
    _input_action_backward = 0x14,        // - throttle_x
    _input_action_left = 0x15,            // + throttle_y (+0x18), rate settings +0x814
    _input_action_right = 0x16,           // - throttle_y
    _input_action_look_up = 0x17,         // + look_y (+0x20), rate settings +0x81c
    _input_action_look_down = 0x18,       // - look_y
    _input_action_look_left = 0x19,       // + look_x (+0x1c), rate settings +0x818
    _input_action_look_right = 0x1a       // - look_x
} input_action;

typedef enum input_action_constants {
    k_input_action_count = 0x1b,          // bounds of every last-used-binding write (0 .. 0x1a)
    k_input_digital_action_count = 0x13,  // hold-count bytes per local_player_input_state
    k_input_first_axis_action = 0x13
} input_action_constants;

// ---------------------------------------------------------------------------
// keyboard key indices
// The engine key index is not a DirectInput scan code: the poll maps DIK codes through the
// int16 table at 0x0065bd58, input_record_windows_key_message maps virtual keys through
// 0x0065ba58 and WM_CHAR characters through 0x0065bc58. 0x00 .. 0x6c are real keys
// (k_control_keyboard_key_count, saved_games.h). Only the indices the module code names are
// listed; each is given with the DIK code the 0x0065bd58 table maps onto it.
// 0x6e .. 0x71 are virtual either-side modifier keys that only input_get_key_state 0x490b50
// understands: it returns the larger hold count of the two physical keys.
// ---------------------------------------------------------------------------
typedef enum input_key {
    _input_key_escape = 0x00,             // DIK 0x01; also the back key of the menu generator
    _input_key_f1 = 0x01,                 // DIK 0x3b (f1 .. f10 are 0x01 .. 0x0a)
    _input_key_print_screen = 0x0d,       // DIK 0xb7
    _input_key_grave = 0x10,              // DIK 0x29 / 0x94, the console key
    _input_key_backspace = 0x1d,          // DIK 0x0e
    _input_key_tab = 0x1e,                // DIK 0x0f; the poll clears its hold count while
                                          //   GetAsyncKeyState(VK_MENU) reports alt down
    _input_key_enter = 0x38,              // DIK 0x1c; menu accept
    _input_key_left_shift = 0x39,         // DIK 0x2a
    _input_key_right_shift = 0x44,        // DIK 0x36
    _input_key_left_control = 0x45,       // DIK 0x1d
    _input_key_left_windows = 0x46,       // DIK 0xdb
    _input_key_left_alt = 0x47,           // DIK 0x38
    _input_key_space = 0x48,              // DIK 0x39
    _input_key_right_alt = 0x49,          // DIK 0xb8
    _input_key_right_windows = 0x4a,      // DIK 0xdc
    _input_key_right_control = 0x4c,      // DIK 0x9d
    _input_key_up = 0x4d,                 // DIK 0xc8; menu up
    _input_key_down = 0x4e,               // DIK 0xd0; menu down
    _input_key_left = 0x4f,               // DIK 0xcb; menu left
    _input_key_right = 0x50,              // DIK 0xcd; menu right
    _input_key_insert = 0x51,             // DIK 0xd2
    _input_key_home = 0x52,               // DIK 0xc7
    _input_key_page_up = 0x53,            // DIK 0xc9
    _input_key_delete = 0x54,             // DIK 0xd3
    _input_key_end = 0x55,                // DIK 0xcf
    _input_key_page_down = 0x56,          // DIK 0xd1
    _input_key_numpad_2 = 0x5c,           // DIK 0x50; menu down
    _input_key_numpad_4 = 0x5e,           // DIK 0x4b; menu left
    _input_key_numpad_6 = 0x60,           // DIK 0x4d; menu right
    _input_key_numpad_8 = 0x62,           // DIK 0x48; menu up
    _input_key_numpad_enter = 0x66,       // DIK 0x9c; menu accept
    _input_key_any_shift = 0x6e,          // max(left_shift, right_shift)
    _input_key_any_control = 0x6f,        // max(right_control, left_control)
    _input_key_any_windows = 0x70,        // left_windows / right_windows (compare is <=)
    _input_key_any_alt = 0x71             // left_alt / right_alt (compare is <=)
} input_key;

// ui_key_event::modifiers as input_record_windows_key_message 0x490d10 builds it from
// GetKeyState; ui_key_event::character is 0xff for a WM_KEYDOWN record and key_code is -1 for
// a WM_CHAR record whose character has no key index.
typedef enum input_key_modifier_flags {
    _input_modifier_shift_bit = 0x01,     // VK_SHIFT
    _input_modifier_control_bit = 0x02,   // VK_CONTROL
    _input_modifier_alt_bit = 0x04        // VK_MENU
} input_key_modifier_flags;

// input_abstraction_globals::mode_flags (0x00712542). input_update_tick 0x48b4b0 dispatches
// on it: exactly 0x01 runs the game action update (0x48cca0) once the menu exit deadline
// 0x00712918 has passed; otherwise bit 3 wins, then bit 2, then bit 1.
typedef enum input_mode_flags {
    _input_mode_game_bit = 0x01,          // set by input_state_initialize
    _input_mode_menu_bit = 0x02,          // 0x48ec50 menu navigation events; exit deadline is
                                          //   now + 200 ms. Also stops the poll from marking a
                                          //   same-frame press+release as release_pending
    _input_mode_keyboard_capture_bit = 0x04, // 0x48b650; the tick clears action state 0
    _input_mode_bind_scan_bit = 0x08      // 0x48b6b0; input_scan_any_bound_input 0x48f8c0
} input_mode_flags;

typedef enum input_constants {
    k_input_maximum_devices = 8,              // DirectInput game controllers; the enumeration
                                              //   callback stops at 8 (0x491f5b)
    k_input_mouse_axis_count = 3,             // x, y, wheel (mouse_state order)
    k_input_mouse_button_count = 8,
    k_input_joystick_axis_range = 0x1000,     // DIPROP_RANGE -0x1000 .. 0x1000 (0x491c50)
    k_input_joystick_axis_deadzone = 1000,    // DIPROP_DEADZONE, 10 percent (0x491c50)
    k_input_joystick_pov_none = -1,           // joystick_state::povs when centered
    k_input_joystick_pov_octant = 4500,       // hundredths of a degree per octant; 0x491fd0
                                              //   rounds with the 0x8ca .. 0x7241 thresholds
    k_input_scan_axis_threshold = 0x4cc,      // joystick axis delta the scan accepts
    k_input_scan_mouse_threshold = 0x46,      // mouse delta the scan accepts (> 0x46)
    k_input_menu_axis_threshold = 0x7ff,      // stick deflection the menu generator accepts
    k_input_menu_repeat_ms = 0x15d,           // 350 ms key repeat of the menu directions
    k_input_menu_exit_delay_ms = 200,         // the game update waits this long after the menu
    k_input_key_event_capacity = 0x40,        // ui_key_event ring at 0x006b16fe
    k_input_key_block_timer_count = 4,        // key_block_timer table at 0x006b1600
    k_input_system_key_count = 3,             // grave, escape, print screen (0x0068e40c)
    k_input_mouse_acceleration_point_count = 7,
    k_input_event_queue_count = 4,            // queues, one per controller index
    k_input_event_queue_depth = 8,
    k_input_joystick_object_count = 0x50,     // 0x20 axes + 0x10 POVs + 0x20 buttons
    k_input_unbound = 0x7fff                  // same value as k_control_binding_unbound
} input_constants;

// ---------------------------------------------------------------------------
// player_control_settings field map as this module reads it (the type itself is owned by
// types/interface.h and is NOT redefined here). Offsets are from the block base
// 0x00710328 + controller * 0x85c; the int16 tables hold input_action values or 0x7fff.
//   +0x000 float look_rate_80        copied to 0x006f1d74 every game update (0x48cca0)
//   +0x004 float look_rate_40        copied to 0x006f1d78
//   +0x008 int16 keyboard[0x6d]      0x00710330, key index
//   +0x0e2 int16 mouse_button[8]     0x0071040a
//   +0x0f2 int16 mouse_axis[3][2]    0x0071041a, [axis][0] direction 1 (positive delta),
//                                    [axis][1] direction 2 (negative delta)
//   +0x0fe int16 gamepad_button[4][0x20]        0x00710426
//   +0x1fe int16 gamepad_action_button[4][2]    0x00710526, the BUTTON index read for
//                                    accept ([0]) and back ([1]); -1 when unbound
//   +0x20e int16 gamepad_axis[4][0x20][2]       0x00710536, [0] direction 1, [1] direction 2
//   +0x40e int16 gamepad_pov[4][0x10][8]        0x00710736, one per octant
//   +0x810 float forward_rate        digital throttle_x step per tick
//   +0x814 float strafe_rate         digital throttle_y step
//   +0x818 float look_x_rate         digital look_x step
//   +0x81c float look_y_rate         digital look_y step
//   +0x820 float mouse_forward_scale mouse delta divisor for forward/backward bindings
//   +0x824 float mouse_strafe_scale  mouse delta divisor for left/right bindings
//   +0x828 float mouse_look_x_sensitivity   argument of the acceleration curve (0x48cb60)
//   +0x82c float mouse_look_y_sensitivity
//   +0x830 float gamepad_axis_scale_x  0..1, input_joystick_set_axis_scale_x 0x48c930
//   +0x834 float gamepad_axis_scale_y  0..1, input_joystick_set_axis_scale_y 0x48c9a0
//   +0x838 float gamepad_rate_80[4]    per pad, read by the gamepad axis path
//   +0x848 float gamepad_rate_40[4]
//   +0x858 uint8 look_inverted         nonzero negates look_y after every game update
//   +0x859 uint8 look_inverted_driving negates look_y when 0x48fd60 reports a driver seat
//                                      (seat flag bit 2) on a unit of type 3 or 5, UNSURE
// saved_player_profile (saved_games.h) holds the same tables at +0x12c higher.
// ---------------------------------------------------------------------------

// ---------------------------------------------------------------------------
// joystick_state  (0xa0 bytes; input_joystick_state_process 0x491fd0 fills it, the poll
// 0x490760 and input_reset_state_and_axis_configs 0x490aa0 copy it as 0x28 dwords)
// The engine view of one game controller slot: joystick_states[4] at 0x006b2a68, the neutral
// copy joystick_neutral_state at 0x006b2cf8 (all zero, POVs -1; read instead of the live
// state while input_suppressed is set), and the four scan baselines at 0x00712544.
// Earlier passes called these the axis-binding profile tables; they hold no bindings.
// ---------------------------------------------------------------------------
typedef struct joystick_state {
    uint8_t button_frames[0x20];   // 0x00 frames held, saturating at 0xff; 0 when up
    int16_t axes[0x20];            // 0x20 raw axis value, -0x1000 .. 0x1000
    int32_t povs[0x10];            // 0x60 octant 0 (north) .. 7 clockwise, -1 centered
} joystick_state;                  // size 0xa0

// ---------------------------------------------------------------------------
// joystick_raw_state  (0xe0 bytes; the custom DirectInput data format, see the header)
// The GetDeviceState buffer the poll hands input_joystick_state_process in its first
// argument. Only the first axis_count / pov_count / button_count entries are consumed.
// ---------------------------------------------------------------------------
typedef struct joystick_raw_state {
    int32_t axes[0x20];            // 0x00 low 16 bits copied to joystick_state::axes
    uint32_t povs[0x10];           // 0x80 hundredths of a degree; low word 0xffff centered
    uint8_t buttons[0x20];         // 0xc0 bit 7 set while pressed
} joystick_raw_state;              // size 0xe0

// ---------------------------------------------------------------------------
// di_mouse_state2  (0x14 bytes, DIMOUSESTATE2; the GetDeviceState buffer of the mouse)
// ---------------------------------------------------------------------------
typedef struct di_mouse_state2 {
    int32_t x;                     // 0x00 relative
    int32_t y;                     // 0x04 relative, down positive
    int32_t z;                     // 0x08 wheel, in granularity units
    uint8_t buttons[8];            // 0x0c bit 7 set while pressed
} di_mouse_state2;                 // size 0x14

// ---------------------------------------------------------------------------
// mouse_state  (0x1c bytes; input_mouse_state_process 0x491bc0 builds it from a
// di_mouse_state2 in ECX into the argument, the poll passes 0x006b180c)
// mouse_state at 0x006b180c is live, mouse_neutral_state at 0x006b1828 is never written and
// stays zero; the readers take the neutral one while input_suppressed is set.
// ---------------------------------------------------------------------------
typedef struct mouse_state {
    int32_t x;                     // 0x00 raw x
    int32_t y;                     // 0x04 negated raw y, up positive
    int32_t wheel;                 // 0x08 -(raw z / mouse_wheel_granularity)
    uint8_t button_frames[8];      // 0x0c frames held, saturating at 0xff; physical button i
                                   //      lands at mouse_button_map[i] (left/right swap)
    uint8_t button_pressed[8];     // 0x14 1 on the poll the button was RELEASED (held before,
                                   //      up now; 0x491c01..0x491c0d), else 0. The menu
                                   //      generator double click reads [0] (a completed click)
} mouse_state;                     // size 0x1c

// ---------------------------------------------------------------------------
// input_device  (0x240 bytes; 8 at 0x006b1868, count input_device_count)
// Written by the DirectInput EnumDevices callback 0x491d70 (not a Ghidra function) from its
// DIDEVICEINSTANCE and DIDEVCAPS; cleared by input_device_release 0x491f80 and
// input_directinput_release_devices 0x490580; +0x230 seeded -1 by input_system_initialize.
// The first 0x220 bytes are exactly a controls_gamepad_record (interface.h): the player
// profile keeps four of them, input_device_list_print copies 0x88 dwords out, and
// input_device_find_index_by_guid 0x4916e0 matches record.device_key (+0x20c product guid,
// then +0x21c) against it.
// ---------------------------------------------------------------------------
typedef struct input_device {
    controls_gamepad_record record;// 0x000 name: tszInstanceName widened (0x20a byte limit),
                                   //       plus a (N) suffix (format 0x006694fc) when
                                   //       instance_number is nonzero;
                                   //       device_key[0..3]: guidProduct (DIDEVICEINSTANCE
                                   //       +0x14); device_key[4]: instance_number, the count of
                                   //       already registered devices with the same product
                                   //       (input_device_count_by_guid 0x491d30)
    input_guid instance_guid;      // 0x220 guidInstance (DIDEVICEINSTANCE +0x04)
    int32_t slot;                  // 0x230 joystick slot 0..3 that reads this device, -1 none;
                                   //       the poll skips a device while this is -1
    int32_t axis_count;            // 0x234 DIDEVCAPS dwAxes clamped to 0x20
    int32_t button_count;          // 0x238 DIDEVCAPS dwButtons clamped to 0x20
    int32_t pov_count;             // 0x23c DIDEVCAPS dwPOVs clamped to 0x10
} input_device;                    // size 0x240

// ---------------------------------------------------------------------------
// key_block_timer  (8 bytes; 4 at 0x006b1600)
// input_key_block_timer_set 0x490bf0 takes a free entry (key -1) or the one with the
// earliest deadline, stores now + duration and the key, and clears the matching
// system_key_states byte. input_get_key_state 0x490b50 reports 0 for a key listed here, and
// input_key_block_timers_expire 0x490ca0 frees an entry once its deadline passes.
// input_keyboard_device_create 0x4918a0 seeds every entry to {-1, -1}.
// ---------------------------------------------------------------------------
typedef struct key_block_timer {
    uint32_t deadline;             // 0x00 milliseconds (QueryPerformanceCounter * 1000 / freq),
                                   //      0xffffffff free
    int16_t key;                   // 0x04 input_key, -1 free
    int16_t pad_06;                // 0x06 never written
} key_block_timer;                 // size 0x08

// ---------------------------------------------------------------------------
// menu_repeat_state  (8 bytes; 4 at 0x0068e4fc, .data, initialized {0, -1})
// input_menu_generate_events 0x48ec50 keeps one per menu direction: up (keys up / numpad 8),
// down (down / numpad 2), left (left / numpad 4), right (right / numpad 6). A held key fires
// again only after k_input_menu_repeat_ms and only for the key that started the repeat.
// ---------------------------------------------------------------------------
typedef struct menu_repeat_state {
    uint32_t last_event_time;      // 0x00 milliseconds of the last event, 0 when released
    int32_t key;                   // 0x04 key index + 1 of the owning key, -1 none
} menu_repeat_state;               // size 0x08

// ---------------------------------------------------------------------------
// mouse_acceleration_point  (0x10 bytes; 7 at 0x0068e41c (defaults, .data) and 7 at
// 0x0068e48c (working copy); input_mouse_acceleration_evaluate 0x48cb60)
// When mouse_acceleration (0x006894d0, clamped 0..1) differs from the cached value at
// 0x0068e418 the defaults are copied over the working table and every working
// magnitude is rebuilt as magnitude_slow - (magnitude_slow - magnitude_fast) * acceleration.
// A delta m then finds the first point i >= 1 with magnitude >= m and returns
// (lerp(rate[i-1], rate[i]) * (sensitivity * boost[i] + 1)) * m.
// The .data defaults: {0,0,.00075,.4} {2,16,.001,.4} {8,32,.001125,.5} {16,64,.001125,.6}
// {32,96,.00125,.8} {64,128,.0015,1} {10000,10000,.0015,1}.
// ---------------------------------------------------------------------------
typedef struct mouse_acceleration_point {
    int32_t magnitude;             // 0x00 working: the interpolated threshold;
                                   //      defaults: the threshold at full acceleration
    int32_t magnitude_slow;        // 0x04 the threshold at zero acceleration
    float rate;                    // 0x08 output per count at this threshold (radians, UNSURE)
    float boost;                   // 0x0c multiplied by the sensitivity argument
} mouse_acceleration_point;        // size 0x10

// ---------------------------------------------------------------------------
// input_event_queue  (0x10c bytes at 0x00712cc0; input_queue_initialize 0x492250,
// input_queue_pop_event 0x4922b0, input_queue_push_event 0x492340)
// Four queues of eight ui_input_event records (types/interface.h). The push takes the queue
// index in AX and the record in EDI, writes the queue index into record +0x02, moves slots
// 1..7 down onto 0..6 (memmove 0x38 bytes) and stores the record in slot 0. The pop returns the
// highest slot whose kind word is nonzero and clears that word; queue -1 tries 0..3.
// ---------------------------------------------------------------------------
typedef struct input_event_queue {
    uint8_t enabled;               // 0x00 set by the initializer; push and pop do nothing
                                   //      while clear. Also read by interface 0x4c7a85
    uint8_t push_disabled;         // 0x01 the push is a no-op while set; never written here
    uint8_t pad_02[2];             // 0x02
    uint32_t last_event_time;      // 0x04 milliseconds, set by a push whose kind is nonzero
    uint32_t start_time;           // 0x08 milliseconds at initialization; interface 0x4c7ac8
                                   //      rewrites it
    ui_input_event events[4][8];   // 0x0c [queue][slot]
} input_event_queue;               // size 0x10c

// ---------------------------------------------------------------------------
// input_abstraction_globals  (0x25f0 bytes at 0x00710328; zeroed as 0x97c dwords by
// input_state_initialize 0x48b3e0)
// ---------------------------------------------------------------------------
typedef struct input_abstraction_globals {
    player_control_settings settings[4];     // 0x0000 per controller, types/interface.h;
                                             //        filled by 0x496060, field map above
    local_player_input_state states[4];      // 0x2170 per controller, types/game.h; only [0]
                                             //        is written by this module (0x00712498)
    uint32_t time_base;                      // 0x2210 milliseconds at initialization,
                                             //        refreshed by 0x48b470; never read here
    uint8_t unknown_2214;                    // 0x2214 set to 1 by 0x48b3e0, never read
    uint8_t pad_2215[3];                     // 0x2215
    uint8_t idle;                            // 0x2218 set to 1 each tick; cleared when the
                                             //        game update produced a state that
                                             //        differs from the last (0x48fce0);
                                             //        read by the main loop at 0x4c7c53
    uint8_t unknown_2219;                    // 0x2219 set to 1 by 0x48b3e0, never read
    uint8_t mode_flags;                      // 0x221a input_mode_flags
    uint8_t pad_221b;                        // 0x221b
    joystick_state scan_baselines[4];        // 0x221c per slot, snapshot taken when the bind
                                             //        scan starts (0x48b6b0), zeroed when it
                                             //        ends; the scan compares axes against it
    control_binding_descriptor scan_result;  // 0x249c the first input the scan saw, zero none
    uint8_t system_key_states[3];            // 0x24a8 hold counts of grave, escape and print
                                             //        screen (system_keys at 0x0068e40c),
                                             //        refreshed every tick
    uint8_t pad_24ab;                        // 0x24ab
    control_binding_descriptor last_used_bindings[0x1b]; // 0x24ac per input_action, the
                                             //        binding that last drove it; device_type 0
                                             //        means not known yet
} input_abstraction_globals;                 // size 0x25f0

// ---------------------------------------------------------------------------
// DirectInput 8 SDK records the module builds or reads (dinput.h layouts; the offsets the
// code uses are quoted)
// ---------------------------------------------------------------------------

// DIOBJECTDATAFORMAT; 0x50 at 0x00879f60, built by input_system_initialize 0x491a80
typedef struct di_object_data_format {
    uint32_t guid;                 // 0x00 const GUID *, always NULL here
    uint32_t offset;               // 0x04 byte offset into joystick_raw_state
    uint32_t type;                 // 0x08 DIDFT_OPTIONAL | DIDFT_ANYINSTANCE | axis/pov/button
    uint32_t flags;                // 0x0c 0
} di_object_data_format;           // size 0x10

// DIDATAFORMAT; the joystick one at 0x0068e51c (.data), c_dfDIKeyboard at 0x0064dfdc and
// c_dfDIMouse2 at 0x0064e1e4 (.rdata)
typedef struct di_data_format {
    uint32_t size;                 // 0x00 0x18
    uint32_t object_size;          // 0x04 0x10
    uint32_t flags;                // 0x08 1 DIDF_ABSAXIS (joystick), 2 DIDF_RELAXIS
    uint32_t data_size;            // 0x0c 0xe0 joystick, 0x100 keyboard, 0x14 mouse
    uint32_t object_count;         // 0x10 0x50 joystick, 0x100 keyboard, 0x0b mouse
    uint32_t objects;              // 0x14 di_object_data_format *
} di_data_format;                  // size 0x18

// DIPROPHEADER; the argument head of Get/SetProperty
typedef struct di_property_header {
    uint32_t size;                 // 0x00 size of the whole property record
    uint32_t header_size;          // 0x04 0x10
    uint32_t object;               // 0x08 0 for the device, 8 DIMOFS_Z, or an object id
    uint32_t how;                  // 0x0c 0 DIPH_DEVICE, 1 DIPH_BYOFFSET, 2 DIPH_BYID
} di_property_header;              // size 0x10

// DIPROPDWORD; DIPROP_BUFFERSIZE (keyboard create), DIPROP_GRANULARITY of the mouse wheel
// (acquire, into mouse_wheel_granularity), DIPROP_DEADZONE 1000 (0x491c50)
typedef struct di_property_dword {
    di_property_header header;     // 0x00
    uint32_t data;                 // 0x10
} di_property_dword;               // size 0x14

// DIPROPRANGE; DIPROP_RANGE -0x1000 .. 0x1000 on every axis (0x491c50)
typedef struct di_property_range {
    di_property_header header;     // 0x00
    int32_t minimum;               // 0x10
    int32_t maximum;               // 0x14
} di_property_range;               // size 0x18

// DIDEVICEOBJECTDATA (DirectInput 8); the keyboard GetDeviceData record, cbObjectData 0x14
typedef struct di_device_object_data {
    uint32_t offset;               // 0x00 DIK scan code, mapped through 0x0065bd58
    uint32_t data;                 // 0x04 bit 7 set on a press
    uint32_t timestamp;            // 0x08
    uint32_t sequence;             // 0x0c
    uint32_t app_data;             // 0x10
} di_device_object_data;           // size 0x14

// DIDEVCAPS; read by the enumeration callback 0x491d70 (dwSize 0x2c)
typedef struct di_device_caps {
    uint32_t size;                 // 0x00 0x2c
    uint32_t flags;                // 0x04
    uint32_t device_type;          // 0x08
    uint32_t axis_count;           // 0x0c -> input_device::axis_count
    uint32_t button_count;         // 0x10 -> input_device::button_count
    uint32_t pov_count;            // 0x14 -> input_device::pov_count
    uint32_t ff_sample_period;     // 0x18
    uint32_t ff_min_time_resolution; // 0x1c
    uint32_t firmware_revision;    // 0x20
    uint32_t hardware_revision;    // 0x24
    uint32_t ff_driver_version;    // 0x28
} di_device_caps;                  // size 0x2c

// DIDEVICEINSTANCEA; the first argument of the enumeration callback 0x491d70
typedef struct di_device_instance {
    uint32_t size;                 // 0x000
    input_guid instance_guid;      // 0x004 CreateDevice argument, -> input_device +0x220
    input_guid product_guid;       // 0x014 -> input_device record.device_key[0..3]
    uint32_t device_type;          // 0x024
    char instance_name[0x104];     // 0x028 widened into input_device record.name
    char product_name[0x104];      // 0x12c
    input_guid ff_driver_guid;     // 0x230
    uint16_t usage_page;           // 0x240
    uint16_t usage;                // 0x242
} di_device_instance;              // size 0x244

// DIDEVICEOBJECTINSTANCEA (DirectInput 8); the first argument of the EnumObjects callback
// 0x491c50 (not a Ghidra function), which reads type (+0x18, axis when bits 0..1 are set and
// the instance number type >> 8 is below 0x20) and name (+0x20) for its error message
typedef struct di_device_object_instance {
    uint32_t size;                 // 0x000
    input_guid type_guid;          // 0x004
    uint32_t offset;               // 0x014
    uint32_t type;                 // 0x018
    uint32_t flags;                // 0x01c
    char name[0x104];              // 0x020
    uint32_t ff_max_force;         // 0x124
    uint32_t ff_force_resolution;  // 0x128
    uint16_t collection_number;    // 0x12c
    uint16_t designator_index;     // 0x12e
    uint16_t usage_page;           // 0x130
    uint16_t usage;                // 0x132
    uint32_t dimension;            // 0x134
    uint16_t exponent;             // 0x138
    uint16_t reserved;             // 0x13a
} di_device_object_instance;       // size 0x13c

// ---------------------------------------------------------------------------
// DirectInput 8 entry point and COM methods, called through the interface vtable
// (method = (*(void ***)object)[byte offset / 4]). All of them are __stdcall in the binary
// (the callee pops); the host-compiler form here carries no calling convention. The byte
// offset of each vtable slot is quoted; the callers are the files named in parentheses.
// ---------------------------------------------------------------------------
// DirectInput8Create, reached through the shell pointer at 0x00746268
// (input_directinput_initialize 0x490520)
typedef int32_t (*directinput8create_proc)(void *instance, uint32_t sdk_version,
    input_guid *iid, void **out_interface, void *outer);
// IDirectInput8A
typedef int32_t (*idirectinput8_release_proc)(void *self);             // +0x08 (0x490580)
typedef int32_t (*idirectinput8_createdevice_proc)(void *self, input_guid *guid,
    void **out_device, void *outer);                                   // +0x0c (0x4918a0, 0x4919c0)
typedef int32_t (*idirectinput8_enumdevices_proc)(void *self, uint32_t device_class,
    void *callback, void *callback_arg, uint32_t flags);               // +0x10 (0x491a80)
// IDirectInputDevice8A
typedef int32_t (*idirectinputdevice8_release_proc)(void *self);       // +0x08 (0x491f80, 0x490580,
                                                                       //   0x4918a0, 0x4919c0)
typedef int32_t (*idirectinputdevice8_getproperty_proc)(void *self, uint32_t property,
    di_property_dword *data);                                          // +0x14 (0x490620)
typedef int32_t (*idirectinputdevice8_setproperty_proc)(void *self, uint32_t property,
    di_property_dword *data);                                          // +0x18 (0x4918a0)
typedef int32_t (*idirectinputdevice8_acquire_proc)(void *self);       // +0x1c (0x490620, 0x490760)
typedef int32_t (*idirectinputdevice8_unacquire_proc)(void *self);     // +0x20 (0x4906e0, 0x491f80,
                                                                       //   0x490580, create failures)
typedef int32_t (*idirectinputdevice8_getdevicestate_proc)(void *self, uint32_t size,
    void *data);                                                       // +0x24 (0x490760)
// +0x28 GetDeviceData. input_keyboard_set_capture_mode 0x48b650 flushes the keyboard buffer
// with (0x14, NULL, &count = -1, 0); interface.h calls the same slot directinput_set_property_fn
typedef int32_t (*idirectinputdevice8_getdevicedata_proc)(void *self, uint32_t object_size,
    di_device_object_data *events, uint32_t *in_out_count, uint32_t flags); // (0x490760)
typedef int32_t (*idirectinputdevice8_setdataformat_proc)(void *self,
    di_data_format *format);                                           // +0x2c (0x4918a0, 0x4919c0)
typedef int32_t (*idirectinputdevice8_setcooplevel_proc)(void *self, void *hwnd,
    uint32_t flags);                                                   // +0x34 (0x4918a0, 0x4919c0)
typedef int32_t (*idirectinputdevice8_poll_proc)(void *self);          // +0x64 (0x490760)

// ---------------------------------------------------------------------------
// globals this module owns
// ---------------------------------------------------------------------------
// Global names: a global cannot share its identifier with its typedef in C (one ordinary
// identifier namespace), so the singletons are named input_globals, live_mouse_state and
// event_queue; every src/input file uses exactly the names below.
// input abstraction (.bss, one object; see input_abstraction_globals)
// global 0x00710328: input_abstraction_globals input_globals
//   0x00710328 settings, 0x00712498 states, 0x00712538 time_base, 0x00712540 idle,
//   0x00712542 mode_flags, 0x00712544 scan_baselines, 0x007127c4 scan_result,
//   0x007127d0 system_key_states, 0x007127d4 last_used_bindings
//   Binding-table aliases inside settings[0] (interface.h keeps those bytes opaque, so the
//   src/input files declare them as their own externs, typed like saved_player_profile):
//   0x00710330 int16_t keyboard_bindings[0x6d], 0x0071040a mouse_button_bindings[8],
//   0x0071041a mouse_axis_bindings[3][2], 0x00710426 gamepad_button_bindings[4][0x20],
//   0x00710526 gamepad_action_buttons[4][2], 0x00710536 gamepad_axis_bindings[4][0x20][2],
//   0x00710736 gamepad_pov_bindings[4][0x10][8]
// global 0x00712918: uint32_t input_menu_exit_deadline      now + 200 ms whenever the menu mode
//                    ticks; the game update waits for it (0x48b55b / 0x48b5c3)
// global 0x0071291c: uint8_t mouse_axis_frames[3][2]        per mouse axis and direction, frames
//                    the delta has kept that sign (0x48d794..0x48d7f6)
// global 0x00712928: uint8_t joystick_axis_frames[4][0x20][2]  same per slot, axis, direction
// global 0x00712a28: uint8_t joystick_pov_frames[4][0x10][8]   per slot, pov and octant
// global 0x00712c28: uint32_t mouse_double_click_time       milliseconds of the first left click,
//                    compared against GetDoubleClickTime by the menu generator
// global 0x00712c34: uint32_t input_queue_sample_time       milliseconds, written by 0x492210
//                    (called by interface 0x498a3d); never read in the image
// global 0x00712cc0: input_event_queue event_queue
// global 0x0087a460: int32_t last_input_device              0 keyboard/mouse, 1..4 slot + 1;
//                    written by the game action update 0x48cca0, the first class 0x48bde0
//                    asks 0x48bae0 about
// global 0x00879f60: di_object_data_format joystick_objects[0x50]   rgodf of the joystick format
//
// DirectInput layer (.bss, contiguous 0x006b15f8..0x006b2d98)
// global 0x006b15f8: uint8_t input_acquired                 set by acquire, cleared by unacquire;
//                    the poll and the key message handler do nothing while clear
// global 0x006b15f9: uint8_t input_suppressed               readers substitute the neutral
//                    mouse/joystick state and report no keys while set. Only ever written 0
//                    (poll, 0x490781), so it is always clear in this build
// global 0x006b15fc: void *direct_input                     IDirectInput8A, DirectInput8Create
// global 0x006b1600: key_block_timer key_block_timers[4]
// global 0x006b1620: uint8_t key_frames[0x6d]               per key index, frames held (sat. 0xff)
// global 0x006b168d: uint8_t key_release_pending[0x6d]      pressed and released inside one poll;
//                    the next poll clears key_frames for these keys
// global 0x006b16fa: int16_t key_event_read_index           consumed by console 0x496655 and the
//                    virtual keyboard 0x4a8c0d; cleared by the poll
// global 0x006b16fc: int16_t key_event_count                clamps at k_input_key_event_capacity
// global 0x006b16fe: ui_key_event key_events[0x40]          (the keyboard create and reset only
//                    zero the first 0x10 records)
// global 0x006b1800: void *keyboard_device                  IDirectInputDevice8A
// global 0x006b1804: void *mouse_device                     IDirectInputDevice8A
// global 0x006b1808: int32_t mouse_wheel_granularity        DIPROP_GRANULARITY of DIMOFS_Z
// global 0x006b180c: mouse_state live_mouse_state
// global 0x006b1828: mouse_state mouse_neutral_state        never written
// global 0x006b1844: int32_t input_device_count             0 .. 8 (some interface readers load
//                    only the low word)
// global 0x006b1848: void *joystick_devices[8]              IDirectInputDevice8A per input_device
// global 0x006b1868: input_device input_devices[8]
// global 0x006b2a68: joystick_state joystick_states[4]      per slot
// global 0x006b2ce8: int32_t joystick_slot_devices[4]       input_device index per slot, -1 none
//                    (types/game.h calls it team_slot_table; see the notes)
// global 0x006b2cf8: joystick_state joystick_neutral_state
//
// .data / .rdata tables
// global 0x00671fac: uint16_t missing_string_text[]         L"<missing string>", the fallback of
//                    every controls_*_names lookup (an array: the code loads its address)
// global 0x0065b730: char input_action_names[0x1b][0x10]    jump .. look_right
// global 0x0065b8e0: input_guid default_profile_guid        (types/interface.h)
// global 0x0065b8f0: char joystick_button_prefix[0x18]      button
// global 0x0065b908: char joystick_axis_prefix[0x18]        axis
// global 0x0065b920: char joystick_pov_prefix[0x18]         pov
// global 0x0065b938: char pov_direction_names[8][10]        north .. northwest
// global 0x0065b988: char decimal_suffixes[0x20][3]         0 .. 31
// global 0x0065ba58: int16_t virtual_key_to_key[0x100]      WM_KEYDOWN mapping, -1 none
// global 0x0065bc58: int16_t character_to_key[0x80]         WM_CHAR mapping, -1 none
// global 0x0065bd58: int16_t scan_code_to_key[0x100]        DIK mapping, -1 none
// global 0x0064dfdc: di_data_format c_dfDIKeyboard
// global 0x0064e1e4: di_data_format c_dfDIMouse2
// global 0x0064e24c: input_guid GUID_SysKeyboard
// global 0x0064e25c: input_guid GUID_SysMouse
// global 0x0064e2ac: input_guid IID_IDirectInput8A
// global 0x0068e40c: int16_t system_keys[3]                 grave, escape, print screen
// global 0x0068e418: float mouse_acceleration_cached        1.0 initially
// global 0x0068e41c: mouse_acceleration_point mouse_acceleration_defaults[7]
// global 0x0068e48c: mouse_acceleration_point mouse_acceleration_points[7]
// global 0x0068e4fc: menu_repeat_state menu_repeat_states[4]  up, down, left, right
// global 0x0068e51c: di_data_format joystick_data_format
// global 0x0068e534: int16_t mouse_button_map[8]            mouse_state slot per physical button,
//                    .data {0,2,1,3,4,5,6,7}; input_mouse_device_create sets entries 0 and 1 to
//                    {0,2}, or {2,0} when GetSystemMetrics(SM_SWAPBUTTON) is nonzero
// global 0x0068e544: int32_t input_last_error               last HRESULT input_error_log_once
//                    formatted, -1 initially
// global 0x006894d0: float mouse_acceleration               0.7; clamped to 0..1 on use
//
// ---------------------------------------------------------------------------
// globals this module reads or writes but does not own
// ---------------------------------------------------------------------------
// 0x006ac8f8  large_integer performance frequency (math); every millisecond clock here is
//             QueryPerformanceCounter * 1000 / this
// 0x006f1d74 / 0x006f1d78  player look yaw / pitch rate (game), written from settings[0]
// 0x007196d8  int32_t game_time_force_single_tick (game); the poll skips the mouse and every
//             game controller while it is set
// 0x00712c2c  int32_t nojoystick (shell); input_system_initialize skips EnumDevices when set
// 0x007461c0 / 0x007461c4 / 0x00746268  shell instance, window, DirectInput8Create (shell)
// 0x0087bc14  tag_instances (cache); 0x0087a478 / 0x0087a480 player globals / players (game);
// 0x008603b0  object headers (objects), read by the look inversion test 0x48fd60

#pragma pack(pop)
