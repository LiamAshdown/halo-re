#pragma once
#include <stddef.h>
// Blam interface module (halo.exe 1.0.10 retail, 0x44c290..0x4c9c80, 376 functions).
// Menu/widget system, developer console, HUD runtime state, motion sensor, virtual
// keyboard, video mode table and the small UI-owned lists. Offsets in comments are
// byte offsets from the struct base and were read off the pointer arithmetic of the
// functions named above each block. Tag layouts are NOT repeated here: the module
// reads ui_widget_definition, hud_globals, weapon_hud_interface, unit_hud_interface,
// grenade_hud_interface, meter and virtual_keyboard straight out of types/tags.h.
// data_array / datum_index / growable_array / heap / heap_block come from
// types/memory.h, ColorARGB from types/tags.h and s_network_address from
// types/networking.h.
#pragma pack(push, 1)
typedef unsigned char uint8_t; typedef signed char int8_t; typedef unsigned short uint16_t; typedef short int16_t;
typedef unsigned int uint32_t; typedef int int32_t;

// ---------------------------------------------------------------------------
// input records shared by the text editor and the console
// (widget_text_edit_process_key @0x44c290, console_process_queued_input @0x4965e0)
// console_process_queued_input copies one 4-byte record out of the buffered key
// ring at 0x006b16fe and both stores it in the console and passes it to the editor.
// ---------------------------------------------------------------------------
typedef struct ui_key_event {
    uint8_t modifiers;         // 0x00 bit 0 tested by 0x44c290 to extend the selection
    uint8_t character;         // 0x01 0xff means no character; below 0x20 is not inserted
    int16_t key_code;          // 0x02 0x1d, 0x4f, 0x50 and 0x54 are special-cased
} ui_key_event;                // size 0x04

// Key codes 0x44c290 handles explicitly. Any other code carrying a printable
// character byte is treated as an insertion.
// The values are the engine key codes of the DIK-to-key table at 0x0065bd58 (int16 per DIK
// code): DIK_BACK 0x0e -> 0x1d, DIK_LEFT 0xcb -> 0x4f, DIK_RIGHT 0xcd -> 0x50, DIK_HOME 0xc7 ->
// 0x52, DIK_DELETE 0xd3 -> 0x54, DIK_END 0xcf -> 0x55. 0x44c290 special-cases 0x1d, 0x4f, 0x50
// and 0x54 only; home and end are listed for reference.
typedef enum ui_edit_key_code {
    _ui_edit_key_backspace = 0x1d,
    _ui_edit_key_left_arrow = 0x4f,
    _ui_edit_key_right_arrow = 0x50,
    _ui_edit_key_home = 0x52,
    _ui_edit_key_delete = 0x54,
    _ui_edit_key_end = 0x55
} ui_edit_key_code;

// ---------------------------------------------------------------------------
// text_edit_state  (widget_text_edit_process_key @0x44c290,
// widget_text_edit_reset_length @0x44c5b0, widget_text_edit_get_selection @0x44c5e0,
// widget_text_edit_insert_string @0x44c640, widget_text_edit_clamp_selection @0x44c780)
// A cursor/selection pair over a caller-owned NUL-terminated 8-bit string. The
// selection runs between anchor and cursor; anchor -1 means no selection, and
// clamp_selection collapses it to -1 whenever the anchor lands on the cursor.
// The console embeds one of these at terminal_console::edit.
// ---------------------------------------------------------------------------
typedef struct text_edit_state {
    char *text;                // 0x00 not owned; strlen is recomputed on every call
    int16_t maximum_length;    // 0x04 insertion stops once strlen reaches this
    int16_t cursor;            // 0x06 clamped into 0 .. strlen
    int16_t selection_anchor;  // 0x08 clamped into -1 .. strlen, -1 means no selection
} text_edit_state;             // size 0x0a

// ---------------------------------------------------------------------------
// widget_instance  (widget_open @0x497a70 allocates it from the widget heap,
// widget_initialize_from_tag @0x499780 fills it, widget_close @0x497c00 frees it,
// widget_create_children_from_tag @0x499540 links the tree,
// widget_instance_point_in_bounds @0x4999f0, widget_get_sibling_index @0x498e30,
// widget_instance_get_cumulative_scale @0x499c20)
// widget_initialize_from_tag zeroes exactly 0x18 dwords before writing any field,
// which is what fixes the size at 0x60. Everything the instance copies comes out of
// the matching UIWidgetDefinition in types/tags.h, so the tag offsets are quoted in
// the comments rather than duplicated as fields.
// The tree is a parent pointer plus a doubly linked sibling list. The focus stack
// that widget_instance_is_top_of_stack @0x499cb0 walks is the same tree read
// upward, checking at each step that parent->focused_child is the node it came from.
// ---------------------------------------------------------------------------
typedef struct widget_instance {
    datum_index definition;            // 0x00 ui_widget_definition tag index
    char *name;                        // 0x04 points at definition->name, tag data + 0x04
    int16_t controller_index;          // 0x08 from definition->controller_index, 4 becomes -1
    int16_t local_x;                   // 0x0a child_widget_reference + 0x38 plus the parent value
    int16_t local_y;                   // 0x0c child_widget_reference + 0x36 plus the parent value
    int16_t widget_type;               // 0x0e UIWidgetType_t copied from definition + 0x00
    uint8_t state;                     // 0x10 set to 1 at creation, rewritten by 0x498e60
    uint8_t render_always;             // 0x11 definition flags bit 9
    uint8_t hidden;                    // 0x12 skipped by the focus and hit-test walks
    uint8_t pauses_game_time;          // 0x13 definition flags bit 1, drives ui_pause_depth
    uint8_t closing;                   // 0x14 widget_close latches this to stay reentrant
    uint8_t is_error_dialog;           // 0x15 display_error sets 1 on the dialog and refuses a new one while root has
                                       //    it; main_menu_on_shown skips auto-close when set; region draws use it for
                                       //    controller filtering
    uint8_t close_on_controller_connected[2]; // 0x16 display_error sets [0]=1 for error 0xd;
                                              //    widget_instance_handle_input_event closes the root once
                                              //    joystick_slot_devices[controller] != -1
    int32_t creation_time;             // 0x18 copied from ui_time_milliseconds
    int32_t milliseconds_to_auto_close;      // 0x1c definition + 0x30, negatives clamped to 0
    int32_t milliseconds_auto_close_fade;    // 0x20 definition + 0x34, negatives clamped to 0
    float scale;                       // 0x24 1.0 at creation, multiplied up the parent chain
    struct widget_instance *previous_sibling; // 0x28
    struct widget_instance *next_sibling;     // 0x2c
    struct widget_instance *parent;           // 0x30
    struct widget_instance *first_child;      // 0x34
    struct widget_instance *focused_child;    // 0x38
    void *text;                        // 0x3c text_box: heap block freed by widget_close; list: see below
    int16_t selection_index;           // 0x40 -1 for a text_box, 0 for a list
    int16_t scroll_blink;              // 0x42 advanced by widget_instance_render_list_head
    union {
        struct {
            void *list_items;          // 0x44 built by ui_build_profile_list, freed by 0x49df70
            uint16_t item_count;       // 0x48 incremented once per child created from the tag
            int16_t unknown_4a;        // 0x4a
            struct widget_instance *extended_description; // 0x4c definition + 0x1b0, closed recursively
            void *list_render_data;    // 0x50 list types only, freed by widget_close
        };
        ColorARGB text_color_override; // 0x44 text_box: the color override, alpha 0.0 means use the tag color
    };
    int16_t selection_direction;       // 0x54 -1/+1 set by widget_list_select_previous/next, cyclable_list_nudge,
                                       //    ui_event_4a1dc0; video_options_menu_update steps gamma by it;
                                       //    render_list_head clears it
    int16_t unknown_56;                // 0x56
    int16_t background_bitmap_frame;   // 0x58 0 or 1, 1 marks the selected list_head child
    uint8_t unknown_5a[4];             // 0x5a
    int16_t background_bitmap_frames;  // 0x5e sequence 0 frame count of the background bitmap
} widget_instance;                     // size 0x60

// A text_box reuses 0x44..0x53 as a ColorARGB text color override instead of the list
// fields: widget_instance_render_text_box @0x49b1d0 reads four floats there and takes the
// tag text color when the alpha at 0x44 is 0.0. The byte at 0x54 (low byte of unknown_54)
// makes the text pulse in both text renderers.
// A list widget driven by the ui_lists builders (ui_list_widget_rebuild_rows @0x4a7db0,
// ui_list_widget_compute_scroll_start @0x4a7d00) reuses the text slot at 0x3c as two int16
// words: 0x3c the committed selection (compared against each row index) and 0x3e the first
// visible item. Both are read with movsx and reset to 0 when the list group changes. They are
// accessed as raw offsets because a C99 header cannot carry an anonymous union here.
// widget_instance::widget_type holds a UIWidgetType_t from types/tags.h: 0 container,
// 1 text_box, 2 spinner_list, 3 column_list. widget_close and the selection walks
// treat 2 and 3 together as the list types.

// ---------------------------------------------------------------------------
// widget_history_node  (list_node_prepend @0x499430, list_node_pop @0x499460,
// widget_pool_list_free_all @0x4994b0, pushed by chimera__load_ui_widget @0x497a70,
// popped by interface_tick @0x497e80 and 0x49c3e0)
// A record pushed on the per-controller go-back stack. The nodes are allocated from
// the same widget heap as the widgets themselves. chimera__load_ui_widget builds the
// template from its own arguments 5, 6 and 7 plus the controller_index of the root
// widget it replaces (-1 when the slot was empty). On a pop the definition is reopened
// with controller_index as argument 4, then 0x49bd00 gets list_definition in EAX and
// the dword at 0x08 on the stack (it reads only the low word) to restore the selection.
// ---------------------------------------------------------------------------
typedef struct widget_history_node {
    datum_index definition;           // 0x00 widget tag to reload
    datum_index list_definition;      // 0x04 child located by 0x499950 on reopen, -1 for none
    int16_t selection;                // 0x08 list index restored by 0x49bd00
    int16_t controller_index;         // 0x0a controller of the replaced root widget, -1 for none
    struct widget_history_node *next; // 0x0c
} widget_history_node;                // size 0x10

// ---------------------------------------------------------------------------
// ui_pending_error  (display_error @0x498f20, interface_handle_quit_request @0x499170)
// While the interface is not ready to open a dialog, display_error parks the request
// here and interface_tick opens it later. The record is indexed by player slot with a
// 4-byte stride: display_error writes the index as a short at +0x00 and the two flags
// as bytes at +0x02 and +0x03 of the same element.
// ---------------------------------------------------------------------------
typedef struct ui_pending_error {
    int16_t error_string_index; // 0x00 -1 means the slot is empty
    uint8_t modal;              // 0x02
    uint8_t is_error;           // 0x03 picks the error_ rather than the warning_ dialog tag
} ui_pending_error;             // size 0x04

// widget_play_sound_effect @0x498e90 selects one of four sound\sfx\ui tags. The id is
// one-based: the switch is dec eax, cmp eax 3, and 0 or anything above 4 plays nothing.
// ui_widget_list_item_activate @0x49a430 passes its action kind straight through (1 after a
// focus change, 2 after opening or replacing a widget, 3 after going back, 0 for none).
typedef enum ui_sound_effect {
    _ui_sound_none = 0,
    _ui_sound_cursor = 1,
    _ui_sound_forward = 2,
    _ui_sound_back = 3,
    _ui_sound_flag_failure = 4
} ui_sound_effect;

// ---------------------------------------------------------------------------
// console_message  (terminal_initialize @0x4963d0 creates the data_array,
// console_message_new @0x496420, console_message_delete @0x496490,
// console_message_expire_old @0x4966e0, chimera__console_out @0x496b50,
// console_draw_overlay @0x496730)
// Datums in the terminal output data_array, capacity 0x20, element size 0x124, kept
// in a doubly linked list ordered newest first. The element size is what pins the
// layout: the text runs from 0x0d up to the color at 0x110, and vsnprintf is handed
// a 0xfe byte limit inside it.
// ---------------------------------------------------------------------------
typedef struct console_message {
    int16_t identifier;        // 0x00 datum_header
    int16_t pad_02;            // 0x02
    datum_index previous;      // 0x04 toward the newest message
    datum_index next;          // 0x08 toward the oldest message
    uint8_t is_command_echo;   // 0x0c set when the text contains the prefix at 0x00669140
    char text[0x103];          // 0x0d vsnprintf with a 0xfe byte limit
    ColorARGB color;           // 0x110 defaults to 1.0, 0.7, 0.7, 0.7
    int32_t age;               // 0x120 frames since creation, deleted once it passes 150
} console_message;             // size 0x124

// ---------------------------------------------------------------------------
// terminal_console  (console_open @0x496510, console_close @0x496580,
// console_process_queued_input @0x4965e0, console_draw_overlay @0x496730,
// console_restore_cursor @0x496c20, console_update_display @0x496d40,
// console_draw_input_line @0x4970a0, console_position_cursor @0x4971a0)
// Owned by the caller of console_open, which installs it as console_active.
// console_open builds the edit state in place: edit.text is set to the address of
// input[0] and edit.maximum_length to 0xff, which is what fixes input at 0x100 bytes
// and places edit at 0x1b4. console_draw_overlay terminates prompt at 0xb3 and input
// at 0x1b3, giving both buffers their exact lengths.
// ---------------------------------------------------------------------------
typedef struct terminal_console {
    int16_t key_event_count;       // 0x00 cleared every frame, capped at 0x20
    ui_key_event key_events[0x20]; // 0x02
    uint8_t unknown_82[2];         // 0x82
    ColorARGB color;               // 0x84 used for the input line
    char prompt[0x20];             // 0x94 window title, also copied to console_window_title
    char input[0x100];             // 0xb4 the line being typed
    text_edit_state edit;          // 0x1b4 edit.text is input, maximum_length 0xff
} terminal_console;                // size 0x1be

// ---------------------------------------------------------------------------
// map_list_entry  (map_list_add_entry @0x4950c0, map_list_append_entry_variant
// @0x495190, map_list_finalize_entry @0x4951f0, map_list_free_all @0x495260,
// map_list_find_known_map_index @0x494ff0, map_list_get_friendly_level_name @0x494f50)
// GlobalAlloc-backed vector grown 0x13 entries at a time. The 0xc stride is the
// GlobalReAlloc size computation itself, capacity times 0xc.
// ---------------------------------------------------------------------------
typedef struct map_list_entry {
    char *path;                // 0x00 GlobalAlloc, lowercased, extension stripped
    int32_t map_id;            // 0x04
    uint8_t cache_file_exists; // 0x08 return of cache_file_exists on the stripped path
    uint8_t pad_09[3];         // 0x09
} map_list_entry;              // size 0x0c

// ---------------------------------------------------------------------------
// video_resolution  (video_resolution_add @0x4badc0, video_resolution_list_build
// @0x4bad40, video_refresh_rate_find_index @0x4bae80, video_resolution_compare
// @0x4bab50, video_display_modes_enumerate @0x4baba0)
// Fixed table of 0x20 entries. The 0x4c stride comes from the refresh_rate_count
// access, which indexes a dword array with a 0x13 dword stride; the refresh rates
// are capped at 8 per resolution by an explicit compare.
// ---------------------------------------------------------------------------
typedef struct video_resolution {
    int32_t width;               // 0x00
    int32_t height;              // 0x04
    uint16_t name[16];           // 0x08 wide string built from the %d x %d pattern; name[15] (0x26) forced to 0
    uint32_t refresh_rate_count; // 0x28 never allowed past 8
    int32_t refresh_rates[8];    // 0x2c
} video_resolution;              // size 0x4c


// ---------------------------------------------------------------------------
// d3d_display_mode, d3d9_interface  (video_display_modes_enumerate @0x4baba0)
// D3DDISPLAYMODE and the two IDirect3D9 vtable slots the mode enumeration calls;
// the interface pointer is the global at 0x0071d178.
// ---------------------------------------------------------------------------
typedef struct d3d_display_mode {
    uint32_t width;        // 0x00
    uint32_t height;       // 0x04
    uint32_t refresh_rate; // 0x08
    uint32_t format;       // 0x0c D3DFORMAT
} d3d_display_mode;        // size 0x10

typedef struct d3d9_interface_vtable {
    void *unknown_00[6];   // 0x00 IUnknown, RegisterSoftwareDevice, GetAdapterCount, GetAdapterIdentifier
    uint32_t (__stdcall *get_adapter_mode_count)(void *self, uint32_t adapter, uint32_t format); // 0x18
    int32_t (__stdcall *enum_adapter_modes)(void *self, uint32_t adapter, uint32_t format, uint32_t mode,
                                            d3d_display_mode *out_mode);                       // 0x1c
} d3d9_interface_vtable;

typedef struct d3d9_interface {
    d3d9_interface_vtable *vtable; // 0x00
} d3d9_interface;

// ---------------------------------------------------------------------------
// ui_list_item  (ui_list_add_entry @0x4a7ba0, ui_list_free_all @0x4a7b20,
// ui_list_get_data @0x4a7c50, ui_list_get_id @0x4a7c80, ui_list_find_default
// @0x4a7cb0, consumed by the list-widget builder at 0x4a7db0)
// Elements of three growable_array instances from types/memory.h laid end to end at
// 0x006b3830. ui_list_free_all walks them with a 0xc stride and stops at 0x006b385c,
// which is what fixes the group at three arrays; growable_array_add_element is called
// with element_size 0x10.
// ---------------------------------------------------------------------------
typedef struct ui_list_item {
    uint16_t *name;            // 0x00 GlobalAlloc wide string, freed by ui_list_free_all
    void *data;                // 0x04 optional GlobalAlloc blob, freed with the name
    int32_t id;                // 0x08
    uint8_t is_default;        // 0x0c nonzero raises ui_list_has_default
    uint8_t pad_0d[3];         // 0x0d
} ui_list_item;                // size 0x10

// ---------------------------------------------------------------------------
// controls_gamepad_record  (controls_gamepad_list_find @0x4b5760, controls_gamepad_list_add
// @0x4b5800, controls_gamepad_list_remove @0x4b5850, controls_gamepad_lists_load @0x4b58d0,
// controls_gamepad_toggle_assignment @0x4b5b20, controls_build_device_label_table @0x4b4890)
// One gamepad (input device) record. The profile keeps four at +0x1108 (the assigned
// gamepads, labelled as controls devices 2..5 by controls_build_device_label_table); the
// connected devices are the 0x240 stride table at 0x006b1868 (count int32 0x006b1844), of
// which the first 0x220 bytes are copied. Phase 4 review: formerly server_browser_entry, but
// no caller is on a server screen; the 0x14 byte key at 0x20c was only assumed to be an
// s_network_address. It is the DirectInput product GUID plus the instance number among
// devices of the same product: input_device_find_index_by_guid 0x4916e0 pre-checks +0x21c and
// then compares 16 bytes at +0x20c (repz cmpsd, ecx 4), and input_device_list_print 0x491750
// hands +0x20c to StringFromGUID2.
// The size is exact: the add routine copies 0x88 dwords and the remove routine compacts
// with a 0x220 byte stride; the find routine compares the key as five dwords.
// ---------------------------------------------------------------------------

// input_guid  (controls_apply_preset @0x4b4c50 passes the default profile GUID at
// 0x0065b8e0 by value to input_device_default_profile_tag_find @0x490110). Declared ahead of
// controls_gamepad_record, which embeds it.
typedef struct input_guid {
    uint32_t words[4];     // 0x00
} input_guid;              // size 0x10

typedef struct controls_gamepad_record {
    uint16_t name[0x106];      // 0x000 wide device name, the first 0x3f characters are shown
    input_guid product_guid;   // 0x20c DirectInput guidProduct
    int32_t product_instance;  // 0x21c index among the connected devices with the same product
} controls_gamepad_record;     // size 0x220

// ---------------------------------------------------------------------------
// controls_device_label  (controls_device_label_add @0x4b4830,
// controls_build_device_label_table @0x4b4890)
// Fixed table of 16. The add routine walks it with a 0x210 byte stride and stops at
// 0x006953e8, and the build routine zeroes exactly 0x840 dwords, 0x2100 bytes.
// ---------------------------------------------------------------------------
typedef struct controls_device_label {
    uint16_t name[0x105];      // 0x000 wide string, wcsncpy with a 0x104 limit then terminated
    uint16_t pad_20a;          // 0x20a
    int32_t device_type;       // 0x20c 0 for the tag-supplied label, 2 plus slot for a device
} controls_device_label;       // size 0x210


// (input_guid is declared above controls_gamepad_record.)

// ---------------------------------------------------------------------------
// first_person_weapon_interface  (interface_globals_allocate @0x494340 reserves it,
// interface_local_player_state_reset @0x494390 clears it,
// first_person_weapon_interface_initialize @0x493c60,
// first_person_weapon_set_state @0x492e60, first_person_weapon_snapshot_pose @0x4930b0,
// first_person_weapon_update @0x493150,
// first_person_weapon_update_animation_controls @0x493740,
// first_person_weapon_process_action @0x4940f0,
// local_player_index_for_object @0x4926f0)
// The size is exact on both sides: 0x494340 bumps the game-state cursor by 0x1ea0 and
// folds that constant into the state checksum, and 0x494390 zeroes 0x7a8 dwords.
// Every accessor indexes first_person_weapon_interfaces with a 0x1ea0 stride, and
// local_player_index_for_object stops after index 0, so retail PC has one entry.
// ---------------------------------------------------------------------------
typedef struct first_person_weapon_interface {
    uint8_t attached;          // 0x0000 1 while the first person model is attached
    uint8_t unknown_01[3];     // 0x0001
    datum_index unit_index;    // 0x0004 the controlled unit object
    datum_index weapon_index;  // 0x0008 the unit current weapon object
    int16_t state;             // 0x000c animation state, driven by 0x492d20 and 0x492e60
    int16_t idle_delay_ticks;  // 0x000e first_person_weapon_update 0x493150: seeded random(first_person_idle_time)*30
                               //    in state 0; when idle_ticks exceeds it, state 5 (idle anim) unless skip_fraction
    int16_t idle_ticks;        // 0x0010 0x493150 increments while idle in state 0, zeroed otherwise; compared against
                               //    unknown_0e; 0x493c60 zeroes it
    int16_t shutdown_countdown; // 0x0012 reseeded to 0x1e by 0x4942e0
    int16_t animation_index;   // 0x0014 first person animation index, -1 when none
    int16_t current_animation; // 0x0016 0x492e60 stores the resolved animation index for new state; 0x493150
                               //    animation_state_advance(&+0x16) ; 0x493740
                               //    animation_get_frame_orientations(&animations[+0x16], frame +0x18)
    uint8_t current_animation_frame[2]; // 0x0018 0x492e60 zeroes the word with the new animation; 0x493740 frame arg
                                        //    for +0x16; type should be int16_t, not uint8_t[2]
    int16_t moving_animation;  // 0x001a 0x493150 set to fp-weapon list[3] ('moving') while unit throttle>0.1, -1 when
                               //    stopped; 0x493740 overlays it with frame +0x1c
    uint8_t unknown_1c[4];     // 0x001c
    int16_t overcharged_animation; // 0x0020 0x493150 set to fp list[15] (overcharged-jitter overlay) in state 4,
                                   //    frame float +0x24 advanced by weapon +0x244; 0x493740 weighted overlay
    uint8_t pad_22[2];         // 0x0022
    float overcharge_frame;    // 0x0024 advanced by weapon +0x244 in state 4; frame of the overcharged overlay
    float recoil;              // 0x0028 0x493150 real_seek_toward_clamped(velocity=+0x2c 'charge', value=+0x28);
                               //    action 0 (primary fire, weapon_fire_trigger) kicks +0x2c; overlays frame 8
                               //    weighted by it
    float charge;              // 0x002c nudged by action code 0 in 0x4940f0
    float move_sway_x;         // 0x0030 seeks the unit throttle.i (0x493150)
    float move_sway_y;         // 0x0034 seeks the unit throttle.j
    float move_sway_x_velocity; // 0x0038
    float move_sway_y_velocity; // 0x003c
    float aim_sway_yaw;        // 0x0040 seeks the clamped change of aim_yaw
    float aim_sway_pitch;      // 0x0044
    float aim_sway_yaw_velocity;   // 0x0048
    float aim_sway_pitch_velocity; // 0x004c
    uint8_t aim_seeded;        // 0x0050 set once seed_aim has run
    uint8_t unknown_51[0xf];   // 0x0051
    float aim_yaw;             // 0x0060 camera forward yaw, set by seed_aim
    float aim_pitch;           // 0x0064
    float previous_aim_yaw;    // 0x0068
    float previous_aim_pitch;  // 0x006c
    float previous_camera_x;   // 0x0070 camera position seeded by seed_aim
    float previous_camera_y;   // 0x0074
    float previous_camera_z;   // 0x0078
    uint8_t unknown_7c[0xc];   // 0x007c
    int16_t blend_start;       // 0x0088 written by 0x4930b0 when a blended change starts
    int16_t blend_end;         // 0x008a
    uint8_t animation_control[0x800]; // 0x008c node control block fed to the animation system
    uint8_t previous_pose[0x800];     // 0x088c copied from animation_control by 0x4930b0
    uint8_t node_matrices[0xd00];     // 0x108c 0x108c 0xd00 bytes = 64 real_matrix4x3 (0x34 each);
                                      //    first_person_weapon_get_marker_data casts it to real_matrix4x3* and
                                      //    update_lighting feeds it to hud_meter_permute_node_records as the node
                                      //    scratch
    uint8_t weapon_hud_valid;         // 0x1d8c result of hud_meter_find_matching_element
    uint8_t pad_1d8d;                 // 0x1d8d
    int16_t weapon_hud_element[0x40]; // 0x1d8e match table filled by 0x493f00
    uint8_t device_hud_valid;         // 0x1e0e second hud_meter_find_matching_element result
    uint8_t pad_1e0f;                 // 0x1e0f
    int16_t device_hud_element[0x40]; // 0x1e10 second match table
    uint8_t device_magazine_empty;    // 0x1e90 1 when the reload animation started on an empty magazine
    uint8_t pad_1e91;                 // 0x1e91
    int16_t device_reload_rounds;     // 0x1e92 rounds the reload will add, clamped to the reserve
    int16_t device_reload_marker;     // 0x1e94 -1 none, 0 reload not allowed, 1 single round, 2 magazine reload
    int16_t unknown_1e96;             // 0x1e96
    int32_t frame_sound_index;        // 0x1e98 0x493150 = sound_start_at_object_marker(weapon, frame sound from
                                      //    animation_state_advance); 0x492e60 sound_impulse_fade_out(it) on forced
                                      //    state change
    int16_t frame_sound_state;        // 0x1e9c 0x493150 records fp->state when the frame sound starts; 0x492e60 skips
                                      //    the fade when it is 1; reset -1 with the sound
    int16_t unknown_1e9e;             // 0x1e9e
} first_person_weapon_interface;      // size 0x1ea0
static_assert(sizeof(first_person_weapon_interface) == 0x1ea0, "first_person_weapon_interface layout");
static_assert(offsetof(first_person_weapon_interface, overcharge_frame) == 0x24, "first_person_weapon_interface layout");
static_assert(offsetof(first_person_weapon_interface, aim_yaw) == 0x60, "first_person_weapon_interface layout");
static_assert(offsetof(first_person_weapon_interface, previous_camera_x) == 0x70, "first_person_weapon_interface layout");
static_assert(offsetof(first_person_weapon_interface, blend_start) == 0x88, "first_person_weapon_interface layout");
static_assert(offsetof(first_person_weapon_interface, device_reload_marker) == 0x1e94, "first_person_weapon_interface layout");

// ---------------------------------------------------------------------------
// hud_message_slot  (chimera__hud_message @0x4ae180, hud_add_item_message @0x4ae400,
// hud_message_find_slot @0x4ae480, hud_message_compare @0x4ae500, drawn by
// hud_messaging_update @0x4ae550)
// Four slots per local player. The 0x8c stride is the loop bound in
// hud_message_find_slot, and the text length is the wcsncpy limit of 0x3f.
// ---------------------------------------------------------------------------
typedef struct hud_message_slot {
    int32_t timestamp;         // 0x00 game time when the message was posted
    uint16_t text[0x3f];       // 0x04 wcsncpy with a 0x3f limit
    uint8_t active;            // 0x82
    uint8_t sequence;          // 0x83 from the rolling counter in hud_messaging_globals
    int32_t source;            // 0x84 -1 for a plain text message, else the item tag id whose
                               //      pickup_text_index string is drawn; matched by find_slot
    int16_t count;             // 0x88 item message count, summed by hud_add_item_message and
                               //      divided by the item hud_message_value_scale when drawn
    uint8_t source_kind;       // 0x8a second half of the find_slot match key; added to the item
                               //      pickup_text_index (0xff: add 1 when count > 1)
    uint8_t pad_8b;            // 0x8b
} hud_message_slot;            // size 0x8c

// ---------------------------------------------------------------------------
// hud_player_messaging_state  (hud_set_player_message @0x4adfc0,
// hud_set_message_icon_argument @0x4ae050, hud_set_message_string_argument @0x4ae0b0,
// hud_set_action_text_shown @0x4ae110, chimera__hud_message @0x4ae180,
// hud_add_item_message @0x4ae400, drawn by hud_messaging_update @0x4ae550)
// One per local player, 0x460 bytes; every accessor multiplies the local player index
// by 0x460 against hud_messaging. The "action message" is either a message of the
// HUDGlobals hud_messages tag (message, with up to 8 substitution arguments that the
// message elements 0x20.. reference) or, when message is NULL, the wide action_text.
// ---------------------------------------------------------------------------
typedef struct hud_player_messaging_state {
    hud_message_slot messages[4]; // 0x000 sorted with hud_message_compare every draw
    uint16_t action_text[0x100];  // 0x230 wcsncpy with a 0xff limit; drawn when message is NULL
    uint8_t unknown_430[4];       // 0x430
    int32_t arguments[8];         // 0x434 per argument: a hud_messaging_information pointer (an
                                  //       icon, argument_is_string bit clear) or a string
                                  //       reference {int16 index, uint8 from_scenario_names}
    struct HUDMessageTextMessage *message; // 0x454 HUDGlobals hud_messages message set by
                                  //       0x4adfc0; NULL blocks every argument store
    uint8_t message_shown;        // 0x458 the action message line is drawn while set
    uint8_t argument_is_string;   // 0x459 bit i set when arguments[i] is a string reference
    uint8_t unknown_45a[4];       // 0x45a
    uint8_t prompt_changed;       // 0x45e raised by 0x4ae110 when message_shown flips, cleared
                                  //       when a slot message is posted; keeps the line reserved
    uint8_t message_shown_copy;   // 0x45f written with message_shown by 0x4ae110
} hud_player_messaging_state;     // size 0x460

// ---------------------------------------------------------------------------
// hud_messaging_information  (ESI of hud_draw_message_icon @0x4ad970, stored as an icon
// argument by hud_set_message_icon_argument @0x4ae050)
// The messaging_information block of WeaponHUDInterface (+0x13c in types/tags.h); the unit
// and grenade HUD interfaces repeat it.
// ---------------------------------------------------------------------------
typedef struct hud_messaging_information {
    uint16_t sequence_index;      // 0x00 sequence of the HUDGlobals icon_bitmap
    int16_t width_offset;         // 0x02 added to the icon width, or the whole advance (flag 2)
    Point2DInt offset;            // 0x04 from the text cursor (x) and its bottom (y)
    ColorARGBInt override_icon_color; // 0x08 used when flags bit 1 is set
    int8_t frame_rate;            // 0x0c game ticks per frame divisor, 0 for a still icon
    uint8_t flags;                // 0x0d HUDInterfaceMessagingFlags: bit 1 override color, bit 2
                                  //      width offset is the absolute icon width
    uint16_t text_index;          // 0x0e
} hud_messaging_information;      // size 0x10

// ---------------------------------------------------------------------------
// hud_item_message  (payload of network message type 6: encoded by hud_post_item_message
// @0x4ae350 on a networked game, decoded by hud_receive_item_message @0x4ae200)
// ---------------------------------------------------------------------------
typedef struct hud_item_message {
    datum_index item_definition;  // 0x00 item tag (weapon or equipment) whose pickup text is used
    uint8_t kind;                 // 0x04 hud_message_slot::source_kind; 0xff means plural by count
    uint8_t pad_05;               // 0x05
    int16_t count;                // 0x06 added to hud_message_slot::count
} hud_item_message;               // size 0x08

// ---------------------------------------------------------------------------
// hud_messaging_globals  (game-state block reserved by hud_state_allocate @0x4a9780
// with an exact size of 0x488 and cleared by hud_state_reset @0x4a98d0 with a 0x122
// dword fill; the timer fields are read by hud_counter_get_value @0x4adcc0 and drawn by
// hud_timer_draw @0x4add10). One player record plus a 0x28 byte tail holding the script-set
// help text, objective text and the countdown timer (hud_set_timer_time 0x4adbf0,
// pause_hud_timer 0x4adc60 are reached through the hs function table, no direct callers).
// ---------------------------------------------------------------------------
typedef struct hud_messaging_globals {
    hud_player_messaging_state players[1]; // 0x000
    int32_t help_text_flash_start_time; // 0x460 start time of the help text flash
    uint8_t help_text_flashing; // 0x464 help text flashes (HUDGlobals hud_help flash) while set
    uint8_t next_sequence;     // 0x465 post-incremented by every message post
    uint8_t unknown_466[6];    // 0x466
    struct HUDMessageTextMessage *help_text; // 0x46c hud_set_help_text (0x4adb30), stored only while
                               //       hud_globals_flags::help_text_shown is set
    struct HUDMessageTextMessage *objective_text; // 0x470 hud_set_objective_text (0x4adb80)
    int16_t objective_text_ticks; // 0x474 HUDGlobals objective uptime + fade ticks
    int16_t unknown_476;       // 0x476
    int32_t timer_start_time;  // 0x478 game time the timer was (re)armed; hud_timer_draw
                               //       rebases it when the warning time is reached
    uint16_t timer_ticks;      // 0x47c ticks on the clock; 0xffff once it ran out
    int16_t timer_warning_ticks; // 0x47e the flashing not-much-time-left threshold
    Point2DInt timer_offset;   // 0x480 anchor offset of the three timer numbers
    int16_t timer_anchor;      // 0x484 HUD corner 0..4, clamped by hud_set_timer_time
    uint8_t timer_paused;      // 0x486 pause_hud_timer; timer_ticks then holds the remaining time
    uint8_t timer_active;      // 0x487 set by hud_set_timer_time, gates hud_timer_draw
} hud_messaging_globals;       // size 0x488

// ---------------------------------------------------------------------------
// hud_text_message  (hud_text_message_queue_init @0x4a3ce0,
// hud_text_message_queue_add @0x4a3d90, hud_text_message_queue_update_and_draw
// @0x4a3e30)
// Elements of a plain growable_array from types/memory.h whose header sits at
// 0x006b37e8; the init routine writes element_size 0x14, count 0 and data NULL
// straight into it. The leading escapes recognised in the text are backslash s,
// whose digits give a delay in units of 0x10, and backslash h.
// ---------------------------------------------------------------------------
typedef struct hud_text_message {
    uint16_t *text;            // 0x00 points past any recognised escape marker
    int32_t unknown_04;        // 0x04 caller-supplied tag or index
    int32_t hold;              // 0x08 1 when the text started with the hold escape
    int32_t start_time;        // 0x0c
    int32_t end_time;          // 0x10 start_time plus the computed duration
} hud_text_message;            // size 0x14

// ---------------------------------------------------------------------------
// hud_waypoint  (hud_waypoint_activate_for_player @0x4af0d0,
// hud_waypoint_deactivate_for_player @0x4af230, hud_waypoints_update_for_player @0x4af370,
// hud_waypoints_draw_for_player @0x4afb90 -> hud_waypoint_draw @0x4af5e0)
// Four slots per local player (the activate_nav_point_* script functions). Both the set and
// the clear routine index the slots with a 0xc stride inside a 0x30 byte per-player record,
// and the containing game-state block is exactly 0x30 bytes, so retail PC has one player record.
// ---------------------------------------------------------------------------
typedef struct hud_waypoint {
    int16_t arrow_index;       // 0x00 HUDGlobals waypoint_arrows index (found by name through
                               //      0x4af070); 0xffff marks the slot empty
    int16_t type;              // 0x02 bits 0..3 the target kind (0 scenario cutscene flag,
                               //      1 object, 2 custom waypoint), 0xf marks the slot free;
                               //      bits 4..7 the visibility written by the update (0 in
                               //      view, 2 occluded)
    float vertical_offset;     // 0x04 added to the target z
    datum_index object_index;  // 0x08 flag, object or custom waypoint index; -1 marks the slot free
} hud_waypoint;                // size 0x0c

typedef struct hud_waypoint_state {
    hud_waypoint waypoints[4]; // 0x00
} hud_waypoint_state;          // size 0x30

// ---------------------------------------------------------------------------
// motion sensor  (motion_sensor_reset @0x4b3660, blip_fill @0x4b35f0,
// motion_sensor_update @0x4b3920, motion_sensor_update_for_player @0x4b3e10,
// motion_sensor_plot_blip @0x4b37a0, motion_sensor_render @0x4b4120)
// motion_sensor_reset gives the whole layout away: it zeroes 0x15c dwords, 0x570
// bytes, then writes the empty marker into 10 groups of 16 records, stepping the
// record by 4 bytes and the group by 0x84. The per-player stride of 0x568 and the
// frame cursor at 0x56c come from the render and update routines.
// ---------------------------------------------------------------------------
typedef struct motion_sensor_blip {
    int8_t x;                  // 0x00 sensor-space offset, ftol of the relative position
    int8_t y;                  // 0x01
    uint8_t type;              // 0x02 blip_type, 6 means the slot is empty
    uint8_t subtype;           // 0x03 0 to 2, picks the icon variant
} motion_sensor_blip;          // size 0x04

typedef struct motion_sensor_frame {
    motion_sensor_blip blips[0x10]; // 0x00
    int8_t extra_blips[0x20];       // 0x40 x and y pairs for the sound-driven blips
    uint8_t extra_sources[0x10];    // 0x60 source id per extra blip
    float viewer_x;            // 0x70
    float viewer_y;            // 0x74
    int32_t blip_count;        // 0x78
    float viewer_facing;       // 0x7c camera yaw plus half pi
    uint8_t extra_blip_count;  // 0x80
    uint8_t pad_81[3];         // 0x81
} motion_sensor_frame;         // size 0x84

typedef struct motion_sensor_player_state {
    motion_sensor_frame history[10];   // 0x000 ring buffer, oldest fades out
    datum_index tracked_objects[0x10]; // 0x528 one per blip slot, -1 when unused
} motion_sensor_player_state;          // size 0x568

typedef struct motion_sensor_globals {
    motion_sensor_player_state players[1]; // 0x000
    int32_t update_time;       // 0x568 game time of the last motion_sensor_update
    int16_t frame_index;       // 0x56c current ring slot, stepped modulo 10
    uint8_t enabled;           // 0x56e set by motion_sensor_update; update_for_player needs it
    uint8_t pad_56f;           // 0x56f
} motion_sensor_globals;       // size 0x570

// blip_type_get @0x4b3450 classifies an object relative to the viewing player. The
// 1/2 and 3/4 pairs differ only by the flag that 0x45bd50 returns, so the second
// member of each pair is the special variant of the first.
typedef enum blip_type {
    _blip_type_friendly = 0,
    _blip_type_enemy = 1,
    _blip_type_enemy_special = 2,
    _blip_type_vehicle = 3,
    _blip_type_vehicle_special = 4,   // also returned for the c_dropship model
    _blip_type_unavailable = 5,
    _blip_type_empty = 6              // written by motion_sensor_reset
} blip_type;

// ---------------------------------------------------------------------------
// hud_unit_meter_state / hud_unit_meter_globals  (game-state block of exactly 0x5c bytes
// reserved by hud_state_allocate @0x4a9780 at 0x0071942c; hud_state_reset @0x4a98d0 clears
// 0x17 dwords and then writes the sentinels below; used by hud_meter_update_value @0x4b0160,
// hud_unit_sounds_update @0x4afee0, hud_render_unit_interface @0x4b0320 and the predictive
// damage hook at 0x4b16e0)
// One 0x58 byte record per local player (every accessor multiplies the local player index by
// 0x58) followed by the script flags dword at +0x58. The displayed shield and health lag
// behind the unit values, so they start at -1.0 to force a snap on the first frame.
// ---------------------------------------------------------------------------
typedef struct hud_unit_meter_state {
    float displayed_shield;    // 0x00 -1.0 until the first update; held for 15 ticks when the
                               //      shield drops, then follows it (0x4b0160)
    float displayed_health;    // 0x04 -1.0 until the first update; the unit health of the
                               //      last draw
    float shield_drain_time;   // 0x08 seconds (1/30 per tick) the shield has been dropping,
                               //      -1.0 while idle or recharging; flash time of the meter
    int32_t shield_update_time; // 0x0c game time of the last 0x08 step
    int32_t shield_flash_start_time; // 0x10 set while the shield panel flashes, else -1
    int32_t health_flash_start_time; // 0x14 set while the health panel flashes, else -1
    int32_t motion_sensor_flash_start_time; // 0x18 set while the motion sensor flashes, else -1
    datum_index last_unit;     // 0x1c the unit of the last draw; -1 resets the record
    uint16_t auxiliary_meters_shown; // 0x20 bit per UnitHUDInterfaceMeterPanelType shown last draw
    int16_t auxiliary_meter_timers[1]; // 0x22 per meter type (integrated light only): ticks of
                               //      the background flash, -1 when idle
    uint16_t sounds_playing;   // 0x24 bit per UnitHUDInterface sound (hud_unit_sounds_play 0x4afd30)
    uint8_t pad_26[2];         // 0x26
    int32_t sound_handles[12]; // 0x28 impulse sound or looping sound datum per HUD sound, -1 when stopped
} hud_unit_meter_state;        // size 0x58

typedef struct hud_unit_meter_globals {
    hud_unit_meter_state players[1]; // 0x00
    uint32_t flags;            // 0x58 hud_show_* / hud_blink_* script flags: bit 0 hide health,
                               //      bit 1 blink health, bit 2 hide shield, bit 3 blink shield,
                               //      bit 4 hide motion sensor, bit 5 blink motion sensor; bits 0
                               //      and 2 also silence the health and shield warning sounds
} hud_unit_meter_globals;      // size 0x5c


// ---------------------------------------------------------------------------
// hud_sound_start_parameters  (hud_unit_sounds_play @0x4afd30 builds it on the stack)
// UNSURE: probably owned by the sound module.
// ---------------------------------------------------------------------------
typedef struct hud_sound_start_parameters {
    int16_t unknown_00;        // 0x00 written 0
    int16_t pad_02;            // 0x02
    float scale;               // 0x04 the HUD sound scale
    float gain;                // 0x08 1.0
} hud_sound_start_parameters;  // size 0x0c

// ---------------------------------------------------------------------------
// hud_weapon_interface_state  (game-state block of exactly 0x7c bytes reserved by
// hud_state_allocate @0x4a9780 and filled with -1 by hud_state_reset @0x4a98d0,
// 0x1f dwords; written by 0x4b1740, 0x4b1970, 0x4b1e20, 0x4b1ff0, 0x4b2ac0 and
// 0x4b2cf0)
// Phase 4 review: the former flat entries[0x1f] view is split by use. Each local
// player has a 0x28 byte block at player * 0x28 (flash start times of the weapon HUD
// elements, the weapon the cache belongs to, the grenade flash start time) and a 0x50
// byte meter / crosshair state at 0x28 + player * 0x50 (19 evaluated values and the
// active mask at entry 0x13, filled by hud_weapon_interface_meters_evaluate). With one
// local player the two arrays and the flags dword end exactly at 0x7c. The flags dword
// at 0x78 bit 0 gates every crosshair draw (hud_weapon_crosshairs_draw @0x4b2cfd) and is
// set or cleared by a script function at 0x480c4e (show_hud_crosshair, UNSURE name).
// ---------------------------------------------------------------------------
typedef struct hud_weapon_interface_player {
    int32_t flash_start_times[8];   // 0x00 per weapon HUD element flash, -1 when idle
    datum_index weapon;             // 0x20 weapon or vehicle the cached state belongs to
    int32_t grenade_flash_start_time; // 0x24
} hud_weapon_interface_player;      // size 0x28

typedef struct hud_weapon_meter_state {
    int32_t values[0x13];           // 0x00 evaluated meter and crosshair inputs
    uint32_t active_mask;           // 0x4c entry 0x13, bit per evaluated state
} hud_weapon_meter_state;           // size 0x50

typedef struct hud_weapon_interface_state {
    hud_weapon_interface_player players[1]; // 0x00 indexed by local player
    hud_weapon_meter_state meters[1];       // 0x28 indexed by local player
    uint32_t flags;                         // 0x78 bit 0 crosshairs shown
} hud_weapon_interface_state;  // size 0x7c

// ---------------------------------------------------------------------------
// hud_globals_flags  (game-state block of exactly 4 bytes reserved by
// hud_state_allocate @0x4a9780; byte 0 is set to 1 by hud_state_reset @0x4a98d0 and
// byte 1 is tested as a gate by 0x4adfc0, 0x4ae050 and 0x4ae0b0)
// ---------------------------------------------------------------------------
typedef struct hud_globals_flags {
    uint8_t hud_enabled;       // 0x00
    uint8_t help_text_shown;   // 0x01 show_hud_help_text: while set every message and argument
                               //      store is blocked and hud_set_help_text (0x4adb30) is taken
    uint8_t unknown_02[2];     // 0x02
} hud_globals_flags;           // size 0x04

// ---------------------------------------------------------------------------
// virtual_keyboard_globals  (virtual_keyboard_initialize @0x4a88f0,
// virtual_keyboard_open @0x4a89a0, virtual_keyboard_process_input @0x4a8be0,
// virtual_keyboard_close @0x4a9250, virtual_keyboard_backspace @0x4a96f0,
// virtual_keyboard_draw_text @0x4a9300, virtual_keyboard_render @0x4a9510)
// Not a passed-around structure: this is the contiguous global block starting at
// 0x007193a8, written field by field. The text buffer length is the wcsncpy limit of
// 0x20 wide characters used on open, and maximum_length is clamped to 0x40.
// The open routine takes the destination buffer in ESI and two stack words, maximum_length
// and field_kind, and returns a byte. The UI sounds it plays are 2 on open, 3 on close and
// commit, 1 on every edit and cursor move, 4 on a rejected character.
// ---------------------------------------------------------------------------
typedef struct virtual_keyboard_globals {
    uint8_t active;            // 0x00 at 0x007193a8
    uint8_t unknown_01;        // 0x01
    uint8_t unknown_02;        // 0x02
    uint8_t unknown_03;        // 0x03
    void *strings_tag_data;    // 0x04 tag data of ui\english, NULL disables the keyboard
    int16_t caret;             // 0x08
    int16_t unknown_0a;        // 0x0a
    int16_t maximum_length;    // 0x0c bytes, clamped to 0x40, over 0x32 selects the small ui
    int16_t selection_start;   // 0x0e reset to -1
    int16_t selection_end;     // 0x10 reset to -1
    int16_t unknown_12;        // 0x12
    int16_t field_kind;        // 0x14 checked by the character filter at 0x4a8b80
    uint8_t committed;         // 0x16 cleared on open and on close
    uint8_t opened;            // 0x17 set to 1 on open
    uint16_t *destination;     // 0x18 caller buffer, written back by virtual_keyboard_close
    uint16_t *destination_end; // 0x1c destination plus its current length
    int32_t open_time;         // 0x20
    datum_index white_bitmap;  // 0x24 ui\shell\bitmaps\white
    uint16_t text[0x20];       // 0x28 the line being edited
    int32_t validation_mode;   // 0x68 at 0x00719410, a dword written after every open: 1 by the
                               //      open itself (profile name), 2 by the variant and profile
                               //      editors, 3 by 0x4a2c50 (an empty entry restores the text),
                               //      4 and 5 by the address and port fields (0x4a3a8f, 0x4a4b1f),
                               //      0 by 0x4a2c80 and 0x4b67a7. Commit in
                               //      virtual_keyboard_process_input branches on 1..3 and the
                               //      character filter 0x4a8b80 (EAX) on 3..5
    datum_index large_ui_tag;  // 0x6c
    datum_index small_ui_tag;  // 0x70
} virtual_keyboard_globals;    // size 0x74

// ---------------------------------------------------------------------------
// progress screen  (chimera__do_show_loading_screen @0x497410, which also covers the
// catalogued interface_draw_screen @0x4974f0 (a split of the same body),
// interface_loading_screen_set_text @0x4978a0, interface_loading_screen_reset @0x4978d0)
// The state machine has no structure of its own; it is the set of globals listed at
// the bottom of this header. State 0 means inactive. 0x497410 draws states 2 to 9 with
// ui\shell\strings\loading entries 1, 2, 0, 2, 3, 4, 5 or 6, and 8 (in state order).
// ---------------------------------------------------------------------------
typedef enum progress_screen_state {
    _progress_screen_inactive = 0,
    _progress_screen_loading = 2,
    _progress_screen_saving = 3,
    _progress_screen_state_4 = 4,      // cleanup releases progress_screen_tag through 0x614fc0
    _progress_screen_state_5 = 5,
    _progress_screen_state_6 = 6,
    _progress_screen_state_7 = 7,
    _progress_screen_state_8 = 8,      // string 6 when hosting, else 5; no cleanup
    _progress_screen_state_9 = 9
} progress_screen_state;

// ---------------------------------------------------------------------------
// player_control_settings  (player_profile_refresh_settings_cache @0x496060)
// The live per-controller settings block the input and UI code reads. 0x496060 builds
// it on the stack from the 0x2004 byte saved profile record at 0x00712dd8 + slot * 0x2004
// (owned by the profile module around 0x0053a000) and copies it out with rep movsd,
// ecx 0x217 dwords, to 0x00710328 + slot * 0x85c (imul edi, edi, 0x85c at 0x4963ba).
// The earlier 0x217 byte size was the Ghidra dword-pointer index read as bytes.
// Most ranges are verbatim copies; the profile offset each came from is quoted
// (profile+X is relative to the 0x2004 byte record). The float fields are the 0 to 9
// slider bytes remapped through three hard-coded tables:
//   table 80: 80, 100, 120, 140, 160, 180, 200, 220, 240, 260
//   table 40: 40, 50, 60, 70, 80, 90, 100, 110, 120, 130
//   table 01: 0.1, 0.25, 0.5, 0.75, 1, 1.25, 1.5, 2, 3, 4
// ---------------------------------------------------------------------------
typedef struct player_control_settings {
    float look_rate_80;                // 0x000 table 80 at clamp(profile+0x12e minus 1, 0, 9)
    float look_rate_40;                // 0x004 table 40 at the same index
    // The binding tables hold input_action values or 0x7fff (types/input.h field map;
    // readers 0x48b7b0, 0x48b9b0, 0x48bae0 and 0x48bea0).
    int16_t keyboard[0x6d];            // 0x008 profile+0x134 verbatim, by key index
    int16_t mouse_button[8];           // 0x0e2 profile+0x20e verbatim (0x0e2..0x0fd is one
    int16_t mouse_axis[3][2];          // 0x0f2   7-dword copy); [axis][0] positive delta,
                                       //         [axis][1] negative delta
    int16_t gamepad_button[4][0x20];   // 0x0fe profile+0x22a verbatim
    int16_t gamepad_action_button[4][2]; // 0x1fe profile+0x32a verbatim; the BUTTON index for
                                       //       accept ([0]) and back ([1]), -1 when unbound
    int16_t gamepad_axis[4][0x20][2];  // 0x20e profile+0x33a verbatim, [0]/[1] direction 1/2
    int16_t gamepad_pov[4][0x10][8];   // 0x40e profile+0x53a verbatim, one per octant
    uint8_t pad_80e[2];                // 0x80e always zero
    float forward_rate;                // 0x810 profile+0x93c verbatim (6 floats): digital
    float strafe_rate;                 // 0x814   throttle_x/throttle_y/look_x/look_y steps
    float look_x_rate;                 // 0x818   per tick, then the mouse delta divisors for
    float look_y_rate;                 // 0x81c   forward/backward and left/right bindings
    float mouse_forward_scale;         // 0x820
    float mouse_strafe_scale;          // 0x824
    float mouse_look_x_sensitivity;    // 0x828 table 01 at min(profile+0x954, 9); argument of
                                       //       the acceleration curve (0x48cb60)
    float mouse_look_y_sensitivity;    // 0x82c table 01 at min(profile+0x955, 9)
    float gamepad_axis_scale_x;        // 0x830 profile+0x960 verbatim, 0..1 (0x48c930)
    float gamepad_axis_scale_y;        // 0x834 profile+0x964 verbatim, 0..1 (0x48c9a0)
    float gamepad_rate_80[4];          // 0x838 table 80 at min(profile+0x956 + i, 9), per pad
    float gamepad_rate_40[4];          // 0x848 table 40 at min(profile+0x95a + i, 9)
    uint8_t look_inverted;             // 0x858 profile+0x12f; nonzero negates look_y
                                       //       (0x48ea21..0x48ea46)
    uint8_t look_inverted_driving;     // 0x859 profile+0x131; negates look_y in a driver seat
    uint8_t pad_85a[2];                // 0x85a always zero
} player_control_settings;     // size 0x85c

// ---------------------------------------------------------------------------
// controls_edit_buffer  (input_controls_live_006b3a48, 0x890 bytes)
// The working copy of the binding tables the controls menu edits: the open handler 0x4b4a30
// copies the tables out of the selected saved_player_profile into it and the close handler
// 0x4b4af0 copies them back. The members are the same tables as player_control_settings and
// saved_player_profile hold, for four gamepads, in a different order.
// ---------------------------------------------------------------------------
typedef struct controls_edit_buffer {
    int16_t gamepad_action_button[4][2];  // 0x000 saved_player_profile::gamepad_action_buttons
    int16_t mouse_button[8];              // 0x010 saved_player_profile::mouse_button_bindings
    int16_t gamepad_axis[4][0x20][2];     // 0x020 saved_player_profile::gamepad_axis_bindings
    int16_t keyboard[0x6d];               // 0x220 saved_player_profile::keyboard_bindings
    uint8_t pad_2fa[2];                   // 0x2fa
    uint8_t gamepad_rate_a[4];            // 0x2fc saved_player_profile::gamepad_rate_a
    uint8_t unknown_300[0x80];            // 0x300
    int16_t gamepad_button[4][0x20];      // 0x380 saved_player_profile::gamepad_button_bindings
    int16_t gamepad_pov[4][0x10][8];      // 0x480 saved_player_profile::gamepad_pov_bindings
    int16_t mouse_axis[3][2];             // 0x880 saved_player_profile::mouse_axis_bindings
    uint8_t gamepad_rate_b[4];            // 0x88c saved_player_profile::gamepad_rate_b
} controls_edit_buffer;                   // size 0x890
static_assert(sizeof(controls_edit_buffer) == 0x890, "controls_edit_buffer layout");
static_assert(offsetof(controls_edit_buffer, gamepad_pov) == 0x480, "controls_edit_buffer layout");
static_assert(offsetof(controls_edit_buffer, gamepad_rate_b) == 0x88c, "controls_edit_buffer layout");

// ---------------------------------------------------------------------------
// loading_thread_record  (interface_tick @0x497e80, chimera__load_main_menu @0x4989f0)
// Pointed to by the global at 0x00718fbc while the background map loading thread runs.
// Only the win32 thread handle and one status byte are ever read.
// ---------------------------------------------------------------------------
typedef struct loading_thread_record {
    void *handle;              // 0x00 win32 thread handle, waited on and closed
    uint8_t unknown_04;        // 0x04 UNSURE, set by the thread when it finishes
} loading_thread_record;       // size 0x05 as read (true allocation size unknown)

// ---------------------------------------------------------------------------
// button prompt text layout  (ui_widget_draw_prompt_span @0x49ad30,
// ui_widget_draw_formatted_prompt_string @0x49ade0, widget_instance_render_text_box
// @0x49b1d0)
// No struct of its own. The drawing helpers pass Rectangle2D pointers from types/tags.h:
// an origin rect (the caller bounds) and a running cursor rect that the measuring routine
// 0x5562d0 rewrites (origin in EBX, cursor in ESI, output rect in EDI). After each span
// the cursor top is copied back into the origin top. An earlier reading of this pair as a
// 4 byte {color, x} record was the top and left halves of these rects.
// ---------------------------------------------------------------------------

// ---------------------------------------------------------------------------
// weapon_screen_effect_parameters  (first_person_weapon_update_screen_effects @0x494730)
// The 0x38 byte block 0x494730 zeroes on its stack (rep stosd, 0x0e dwords), fills from
// the first WeaponHUDInterfaceScreenEffect of the local player weapon HUD interface and
// hands to the rasterizer at 0x52d8a0 (or, in EAX, to 0x52e2d0 on older hardware).
// The rasterizer side is not rewritten, so only the fields 0x494730 writes are named.
// ---------------------------------------------------------------------------
typedef struct weapon_screen_effect_parameters {
    int16_t convolution_extra_passes;  // 0x00 0x00 shared layout with
                                       //    cinematic_screen_effect_globals.convolution_extra_passes;
                                       //    rasterizer_screen_effect_render computes pass_count = (value + 1) << 1
                                       //    from it
    int16_t convolution_type;          // 0x02 2 when a convolution amount is set
    float convolution_amount;          // 0x04 radius interpolated over the fov bounds
    uint32_t mask_bitmap_data;         // 0x08 mask bitmap tag +0x64, bitmaps.pointer
    float night_vision_intensity;      // 0x0c
    float desaturation_intensity;      // 0x10
    float desaturation_tint[3];        // 0x14 ColorRGB effect_tint
    uint8_t desaturation_additive;     // 0x20 desaturation_flags bit 2
    uint8_t night_vision_masked;       // 0x21 night vision flags bit 2
    uint8_t desaturation_masked;       // 0x22 desaturation_flags bit 3
    uint8_t has_extra_maps;            // 0x23 read by the rasterizer: a second and third map follow
    int16_t noise_type;                // 0x24 index into the noise scale table (1, 2, 4)
    uint16_t unknown_26;               // 0x26 never written by 0x494730
    uint32_t extra_map_b;              // 0x28 BitmapData * second map
    float noise_amount;                // 0x2c noise alpha, clamped to 0..1 by the rasterizer
    uint32_t unknown_30;               // 0x30 never written by 0x494730
    uint32_t extra_map_c;              // 0x34 BitmapData * third map
} weapon_screen_effect_parameters;     // size 0x38

// ---------------------------------------------------------------------------
// first_person_light_parameters  (first_person_weapon_update_lighting @0x4924b0)
// The stack block handed by pointer to the render routine at 0x4d6fc0 for both first person
// models. It is the first 0x20 bytes of types/render.h render_model_effect (0x28), and the
// fields follow that layout. Written with type 1 while the unit has flag 0x10 at +0x204 or a
// positive float at +0x37c; otherwise only type (0) and modifier_shader (0) are written.
// The callee copies a full 0x28 bytes (0x4d6fc0: mov ecx,0xa; rep movsd at 0x4d715f/0x4d7167;
// the same copy is at 0x50ee88), but the first-person caller (call 0x4d6fc0 at 0x49266a and
// 0x4926d5) only fills esp+0x20..0x3f, so render_model_effect.change_colors (+0x20) and
// function_values (+0x24) come from whatever the caller's next 8 stack bytes hold.
// render_model_effect cannot be embedded here: interface.h is parsed before render.h.
// ---------------------------------------------------------------------------
typedef struct first_person_light_parameters {
    int16_t type;              // 0x00 render_model_effect_type: 1 when the unit light is on, else 0
    int16_t unknown_02;        // 0x02 never written
    float unit_37c;            // 0x04 unit +0x37c
    float unit_380;            // 0x08 unit +0x380
    datum_index object_index;  // 0x0c the controlled unit datum
    float centroid[3];         // 0x10 real_point3d in render_model_effect: the camera position,
                               //      copied from 0x007c3114..0x007c311c (float[3] here so the
                               //      header does not need types/math.h in every includer)
    uint32_t modifier_shader;  // 0x1c always 0
} first_person_light_parameters; // size 0x20 (the callee reads 0x28, see above)

// ---------------------------------------------------------------------------
// ui_input_event  (built by 0x4922b0 into the interface_tick 16 byte stack scratch,
// consumed by widget_instance_handle_input_event @0x499d00 and passed on to
// ui_widget_list_item_activate @0x49a430 and the event functions below)
// Kinds, from the jump table at 0x49a3e8: 1 dpad or first stick, 2 second stick,
// 3 button, 4 mouse button, 5 custom activation (matched against event type 0x20).
// For kinds 1 and 2 the word at 0x04 is the horizontal axis and the word at 0x06 the
// vertical one; only the extremes -0x8000 and 0x7fff are ever compared. For kinds 3
// and 4 the byte at 0x04 is the button code and the byte at 0x05 is 1 on a press.
// The src files index this record as int16_t event[4] with the same offsets.
// ---------------------------------------------------------------------------
typedef struct ui_input_event {
    int16_t kind;              // 0x00 1 to 5, see above
    int16_t controller_index;  // 0x02 compared against widget_instance::controller_index
    uint8_t code;              // 0x04 button code, also the low byte of the horizontal axis
    uint8_t pressed;           // 0x05 1 on a press, also the high byte of the horizontal axis
    int16_t axis_y;            // 0x06 vertical axis extreme for kinds 1 and 2
} ui_input_event;              // size 0x08 as read (the producer reserves 0x10)

// Function tables indexed by ids stored in ui_widget_definition tags.
// ui_event_function_table @0x006927d0 (0xbe entries): called by widget_close @0x497c00
// with a NULL event and by ui_widget_list_item_activate @0x49a430 with the live event;
// both pass a byte they initialize to 0 as out_handled and test the return for 1.
typedef uint8_t (*ui_event_function)(struct widget_instance *widget, int16_t *event, uint8_t *out_handled);
// game_data_input_function_table (0x3b entries): run once per frame by
// widget_instance_render @0x49a8c0 for every game_data_inputs entry of the tag.
typedef void (*ui_game_data_input_function)(struct widget_instance *widget);
// ui_replace_function_table (4 entries): search and replace callbacks of a text_box,
// used by widget_instance_render_text_box @0x49b1d0 and widget_instance_render_list_head.
typedef void *(*ui_search_replace_function)(struct widget_instance *widget);

// ---------------------------------------------------------------------------
// ui_quad_render_state  (built on the stack by ui_draw_screen_quad @0x498b20,
// ui_draw_rotated_screen_quad @0x494d70, hud_draw_rotated_bitmap_quad @0x4acd50 and
// hud_draw_multitexture_overlay @0x4acfe0, handed in EAX to the rasterizer quad submitter
// 0x51c9a0 together with the four hud_quad_vertex records that follow it on the stack)
// Every builder zeroes the whole 0x23 dwords first. The single-bitmap builders then write
// only meter_parameters, maps[0], map_scales[0], map_texel_scales[0] (all 1.0) and the blend
// function; hud_draw_multitexture_overlay (0x4ad20e..0x4ad86e) fills all three map slots and
// the effector outputs, which is where the per-map field meanings below come from.
// ---------------------------------------------------------------------------
typedef struct ui_quad_render_state {
    void *meter_parameters;          // 0x00 hud_meter_color_block* from hud_meter_draw_fill
                                     //      through 0x4acad0 and 0x4acd50, else NULL
    float *geometry_offset;          // 0x04 float[2] x, y written by the geometry_offset
                                     //      effectors of a multitexture overlay, else NULL
    uint32_t unknown_08;             // 0x08 never written by the interface builders
    BitmapData *maps[3];             // 0x0c primary, secondary, tertiary bitmap data
    uint8_t wrap_modes[3];           // 0x18 HUDInterfaceWrapMode low byte per map
    uint8_t pad_1b;                  // 0x1b
    Point2D *map_offsets[3];         // 0x1c the copy owned by the caller of primary/secondary/tertiary
                                     //      offset; effectors add to it through this pointer
    Point2D map_scales[3];           // 0x28 1 / tag scale per axis, 1.0 where the tag scale is 0
    Point2D map_texel_scales[3];     // 0x40 1/width, 1/height for a non power of two map, else 1.0
    ColorRGB *map_tints[3];          // 0x58 tint_0_1 effector output per map, else NULL
    uint8_t unknown_64[0x14];        // 0x64 never written by the interface builders
    float *map_fades[3];             // 0x78 fade_0_1 effector output per map, else NULL
    int16_t zero_to_one_blend;       // 0x84 HUDInterfaceZeroToOneBlendFunction remapped:
    int16_t one_to_two_blend;        // 0x86 tag 0,1,2,3,4 -> 0,2,1,3,4 (jump table 0x4ad890)
    int16_t framebuffer_blend_function; // 0x88 0 for ui_draw_screen_quad, 7 (alpha multiply add)
                                     //      for the rotated quads, the tag value for an overlay
    uint8_t single_local_player;     // 0x8a player_globals::unknown_0c == 1 for a multitexture
                                     //      overlay, 0 elsewhere
    uint8_t pad_8b;                  // 0x8b
} ui_quad_render_state;              // size 0x8c

// ---------------------------------------------------------------------------
// hud_quad_vertex  (the four records that follow ui_quad_render_state on the stack of every
// builder above; 0x51c9a0 takes a pointer to the first one as its only stack argument)
// Vertex i uses u[(i+1)&2 ? 1 : 0] and v[i > 1 ? 1 : 0], i.e. (u0,v0) (u1,v0) (u1,v1) (u0,v1).
// ---------------------------------------------------------------------------
typedef struct hud_quad_vertex {
    float x;                         // 0x00 screen pixels
    float y;                         // 0x04
    float z;                         // 0x08 always 0.0
    uint32_t color;                  // 0x0c packed ARGB
    float u;                         // 0x10
    float v;                         // 0x14
} hud_quad_vertex;                   // size 0x18


// ---------------------------------------------------------------------------
// hud_flash_parameters  (hud_meter_flash_color_blend @0x4ab980, which gets a pointer to one in
// ESI; its 8 callers in 0x4ac000..0x4b4000 point it into HUD interface tag elements)
// The 0x18 byte run default_color .. flash_length that every HUD interface element of
// types/tags.h repeats inline (GrenadeHUDInterfaceOverlay +0x24, the unit and weapon HUD
// statics, meters and overlays). tags.h has no struct of its own for it.
// ---------------------------------------------------------------------------
typedef struct hud_flash_parameters {
    ColorARGBInt default_color;  // 0x00
    ColorARGBInt flashing_color; // 0x04
    float flash_period;          // 0x08 seconds; 0.0 means never flash
    float flash_delay;           // 0x0c seconds between flashes
    int16_t number_of_flashes;   // 0x10
    uint16_t flash_flags;        // 0x12 bit 0 reverse_default_flashing_colors
    float flash_length;          // 0x14 seconds; 0.0 means never flash
} hud_flash_parameters;          // size 0x18

// ---------------------------------------------------------------------------
// hud_meter_placement  (hud_meter_draw_fill @0x4abbc0 takes a pointer to one in ESI)
// A view of a HUD meter element of types/tags.h starting at its anchor_offset field: element
// +0x24 in WeaponHUDInterfaceMeter (and the grenade and unit meters, which share the layout).
// Every offset below was matched against WeaponHUDInterfaceMeter + 0x24.
// ---------------------------------------------------------------------------
typedef struct hud_meter_placement {
    Point2DInt anchor_offset;            // 0x00
    float width_scale;                   // 0x04
    float height_scale;                  // 0x08
    uint16_t scaling_flags;              // 0x0c bit 2 halves the draw alpha in 0x4abbc0
    uint8_t pad_0e[0x16];                // 0x0e
    TagDependency meter_bitmap;          // 0x24 tag_id at 0x30
    ColorARGBInt color_at_meter_minimum; // 0x34
    ColorARGBInt color_at_meter_maximum; // 0x38
    ColorARGBInt flash_color;            // 0x3c
    ColorARGBInt empty_color;            // 0x40 drawn with its alpha inverted
    uint8_t flags;                       // 0x44 HUDInterfaceMeterFlags: bit 0 use min/max, bit 1
                                         //      interpolate, bit 4 invert the interpolation
    uint8_t minimum_meter_value;         // 0x45 lower bound of both computed alphas
    uint16_t sequence_index;             // 0x46
    uint8_t alpha_multiplier;            // 0x48 alpha = round(value * multiplier + bias)
    uint8_t alpha_bias;                  // 0x49
    int16_t value_scale;                 // 0x4a
    float opacity;                       // 0x4c
    float translucency;                  // 0x50
    ColorARGBInt disabled_color;         // 0x54
    float min_alpha;                     // 0x58
} hud_meter_placement;                   // size 0x5c (tag element bytes 0x24 .. 0x80)

// ---------------------------------------------------------------------------
// hud_number_placement  (hud_draw_number @0x4ac0b0 takes a pointer to one as its third stack
// argument) -- a HUD number element of types/tags.h viewed from its anchor_offset:
// WeaponHUDInterfaceNumber + 0x24, whose layout the unit and grenade numbers repeat.
// ---------------------------------------------------------------------------
typedef struct hud_number_placement {
    Point2DInt anchor_offset;            // 0x00 passed in EDX to 0x4ab690
    float width_scale;                   // 0x04
    float height_scale;                  // 0x08
    uint16_t scaling_flags;              // 0x0c bit 2 halves the digit scale
    uint8_t pad_0e[0x16];                // 0x0e
    hud_flash_parameters flash;          // 0x24 default color first
    ColorARGBInt disabled_color;         // 0x3c
    uint8_t pad_40[4];                   // 0x40
    int8_t maximum_number_of_digits;     // 0x44
    uint8_t flags;                       // 0x45 HUDInterfaceNumberFlags: bit 0 show leading zeros,
                                         //      bit 1 only when zoomed, bit 2 trailing m (km over 999)
    int8_t number_of_fractional_digits;  // 0x46 at most 4 are drawn
    uint8_t pad_47;                      // 0x47
} hud_number_placement;                  // size 0x48 as read

// ---------------------------------------------------------------------------
// hud_element_placement  (EDX of hud_draw_bitmap_element @0x4acad0 and of
// hud_anchor_offset_to_screen_position @0x4ab690)
// The first 0x0e bytes of every HUD interface element of types/tags.h viewed from its
// anchor_offset field (static elements, meters and numbers at element +0x24, overlays and
// crosshair overlays at +0x00). hud_meter_placement, hud_number_placement and
// hud_static_element_placement all start with it.
// ---------------------------------------------------------------------------
typedef struct hud_element_placement {
    Point2DInt anchor_offset;        // 0x00
    float width_scale;               // 0x04
    float height_scale;              // 0x08
    uint16_t scaling_flags;          // 0x0c HUDInterfaceScalingFlags: bit 0 do not scale offset
                                     //      (clears the split screen flag of 0x4ab690), bit 2
                                     //      use high res scale (halves the draw scale)
} hud_element_placement;             // size 0x0e

// ---------------------------------------------------------------------------
// hud_static_element_placement  (third stack argument of hud_draw_static_element @0x4ac6f0)
// A HUD static element of types/tags.h viewed from its anchor_offset field:
// WeaponHUDInterfaceStaticElement + 0x24, which the unit and grenade HUD statics repeat.
// Replaces the file-local hud_static_icon_element TYPES-GAP typedefs of the first rewrite.
// ---------------------------------------------------------------------------
typedef struct hud_static_element_placement {
    Point2DInt anchor_offset;        // 0x00
    float width_scale;               // 0x04
    float height_scale;              // 0x08
    uint16_t scaling_flags;          // 0x0c see hud_element_placement
    uint8_t pad_0e[0x16];            // 0x0e
    TagDependency interface_bitmap;  // 0x24 tag_id at 0x30
    hud_flash_parameters flash;      // 0x34 default color first
    ColorARGBInt disabled_color;     // 0x4c drawn when draw flags bit 1 is set
    uint8_t pad_50[4];               // 0x50
    uint16_t sequence_index;         // 0x54
    uint8_t pad_56[2];               // 0x56
    TagReflexive multitexture_overlays; // 0x58 HUDInterfaceMultitextureOverlay, stride 0x1e0
} hud_static_element_placement;      // size 0x64 (tag element bytes 0x24 .. 0x88)

// ---------------------------------------------------------------------------
// hud_overlay_list  (second stack argument of hud_draw_overlays @0x4ac950)
// WeaponHUDInterfaceOverlayElement + 0x24: the overlay bitmap dependency followed by the
// overlays block (WeaponHUDInterfaceOverlay, stride 0x88; the grenade HUD overlay block has
// the same layout). Replaces the file-local hud_overlay_icon_list TYPES-GAP typedef.
// ---------------------------------------------------------------------------
typedef struct hud_overlay_list {
    TagDependency overlay_bitmap;    // 0x00 tag_id at 0x0c
    TagReflexive overlays;           // 0x10 count at 0x10, pointer at 0x14
} hud_overlay_list;                  // size 0x1c

// ---------------------------------------------------------------------------
// campaign level select list  (ui_build_level_select_list @0x49c8f0, 0x49cc80, 0x49ce00,
// 0x4a4e20)
// level_select_entries at 0x00719018 is one array of 10 eight byte records: the rewrites first
// read it as two arrays (paths at 0x00719018, flags at 0x0071901c), which are the two halves of
// the same record. The builder copies the path from known_campaign_levels at 0x00692acc and the
// unlock bits from the saved profile. widget_instance::list_items points at the array.
// ---------------------------------------------------------------------------
typedef struct level_select_entry {
    char *path;                // 0x00 from known_campaign_levels
    uint8_t valid;             // 0x04 set to 1 once the level is unlocked
    uint8_t flag_bit1;         // 0x05 profile unlock flags bit 1
    uint8_t flag_bit2;         // 0x06 bit 2
    uint8_t flag_bit3;         // 0x07 bit 3
} level_select_entry;          // size 0x08

typedef struct campaign_level_entry {
    char *path;                // 0x00 "levels\a10\a10" and so on
} campaign_level_entry;        // size 0x04, table of 10 at 0x00692acc: every access is [reg*4+0x692acc]
                               // (0x49ca50, 0x49cbb0, 0x49cd09, 0x49cf58), so there is no second field

// ---------------------------------------------------------------------------
// profile_carousel_slot  (ui_profile_carousel_slot_cache_populate @0x4a74b0,
// player_profile_1wide_list_update @0x4a6380, ui_build_profile_list @0x49dd70)
// Three 0x2000 byte slots at 0x00873d60 (the scans stop at 0x00879d60); ui_build_profile_list
// fills all 0x1800 dwords with 0xff. player_profile_get @0x53a770 loads a saved profile into
// profile (ECX) when a slot is assigned. Offsets of the profile body as the 1-wide list reads
// them: +0x002 name (11 wide chars), +0x11a player color (clamped to 0..0x11), +0x11c flags (bit
// 0 default profile, high byte the default name index), +0x12c button set, +0x12d joystick set.
// ---------------------------------------------------------------------------
typedef struct profile_carousel_slot {
    int32_t profile_id;        // 0x0000 -1 when empty
    uint8_t profile[0x1ffc];   // 0x0004 the saved profile body
} profile_carousel_slot;       // size 0x2000

// ---------------------------------------------------------------------------
// variant_carousel_slot  (ui_variant_carousel_slot_cache_populate @0x4a7570)
// Three cached game variant headers at 0x00879d60, 0x9c byte stride; the body is filled by
// the saved game module (0x53bee0).
// ---------------------------------------------------------------------------
typedef struct variant_carousel_slot {
    int32_t id;                // 0x00
    uint8_t unknown[0x98];     // 0x04 filled by 0x53bee0
} variant_carousel_slot;       // size 0x9c

// ---------------------------------------------------------------------------
// chat_incoming_record  (chat_dispatch_incoming @0x4aaf70)
// The stack record the network decoder 0x4ec590 fills from a chat event.
// ---------------------------------------------------------------------------
typedef struct chat_incoming_record {
    int32_t kind;              // 0x00 0 all, 1 team, 2 vehicle, 3 server text, 4 localized string id
    uint8_t player_index;      // 0x04 0xff when the line has no sending player
    uint8_t pad_05[3];         // 0x05
    uint16_t *text;            // 0x08 caller-provided 0x100 wide character buffer
} chat_incoming_record;        // size 0x0c

// ---------------------------------------------------------------------------
// hud_meter_color_block  (built on the stack by hud_meter_draw_fill @0x4abbc0 and handed to
// the meter renderer 0x4acad0)
// ---------------------------------------------------------------------------
typedef struct hud_meter_color_block {
    uint32_t primary;          // 0x00 packed ARGB
    uint32_t secondary;        // 0x04
    uint32_t empty;            // 0x08 empty_color with its alpha inverted
    uint32_t tint;             // 0x0c
    uint8_t flag_10;           // 0x10 always 0 here
    uint8_t flag_11;           // 0x11 always 1 here
    uint8_t pad_12[2];         // 0x12
    uint32_t opacity;          // 0x14 packed {translucency, 1 - opacity three times}
    float scale;               // 0x18 always 1.0 here
} hud_meter_color_block;       // size 0x1c

// ---------------------------------------------------------------------------
// function pointer types
// The chat dialog and the chat listbox are controls of the embedded GUI library, reached
// through the function pointer table at 0x00721eb8..0x00721ee8 (root handle 0x00721ea4,
// active flag 0x00721eec). Only the slots the interface calls are typed; the property ids
// seen are 0x115, 0x180 (append row) and 0x182 (remove row 0).
// ---------------------------------------------------------------------------
typedef void *(*chat_gui_find_object_fn)(void *root, void *key);               // 0x00721eb8
typedef void (*chat_gui_release_fn)(void *object);                             // 0x00721ec8
typedef void *(*chat_gui_find_child_fn)(void *object, const uint16_t *name);    // 0x00721ecc
typedef void (*chat_gui_finalize_fn)(void *object);                            // 0x00721ed0
typedef void (*chat_gui_set_focus_fn)(void *object, void *child);              // 0x00721ed4
typedef void (*chat_gui_set_state_fn)(void *object, int32_t state);            // 0x00721edc
typedef const uint16_t *(*chat_gui_get_property_string_fn)(void *child, const uint16_t *property); // 0x00721ee0
typedef void (*chat_gui_set_property_string_fn)(void *child, const uint16_t *property, const void *value); // 0x00721ee4
typedef uint32_t (*chat_gui_set_property_int_fn)(void *child, int32_t property, int32_t unknown, const void *value); // 0x00721ee8
// IDirectInputDevice8 vtable slot 0x28 (SetProperty style), called on the keyboard device at
// 0x006b1800 by the virtual keyboard and chat code with property 0x14 and value -1.
typedef int32_t (*directinput_set_property_fn)(void *device, uint32_t property, uint32_t flags,
                                                const int32_t *value, uint32_t reserved);
// Row formatter passed to ui_list_widget_rebuild_rows @0x4a7db0: fills the 0x80 byte row text
// buffer for item_index of list_items and returns nonzero on success (tested in AL).
typedef uint8_t (*ui_list_item_format_function)(void *item_buffer, int32_t item_index, void *list_items);
// (Phase 4 review: the former network_group_name_function type is gone. The four entry
// table at 0x00692c08 that ui_search_replace_function_call @0x4a8730 indexes holds
// ui_search_replace_function pointers, called with the widget.)
// Child anchor handler table of hud_anchor_offset_to_screen_position @0x4ab690 (jump table
// at 0x4ab8b8, used when ECX is not NULL).
typedef void (*hud_anchor_offset_handler)(void);

// ---------------------------------------------------------------------------
// win32 API structs used by the developer console, cursor and video code.
// Plain copies of COORD, SMALL_RECT, CONSOLE_SCREEN_BUFFER_INFO, CONSOLE_CURSOR_INFO,
// KEY_EVENT_RECORD, INPUT_RECORD (key member only), RECT and POINT with the ANSI layout.
// ---------------------------------------------------------------------------
#if defined(HALO_WIN32_H) && defined(_WIN32)   /* the SDK's own type where windows.h is in (same layout and field names) */
typedef COORD win32_coord;
#else
typedef struct win32_coord {
    int16_t X;                 // 0x00
    int16_t Y;                 // 0x02
} win32_coord;                 // size 0x04
#endif

#if defined(HALO_WIN32_H) && defined(_WIN32)   /* the SDK's own type where windows.h is in (same layout and field names) */
typedef SMALL_RECT win32_small_rect;
#else
typedef struct win32_small_rect {
    int16_t Left;              // 0x00
    int16_t Top;               // 0x02
    int16_t Right;             // 0x04
    int16_t Bottom;            // 0x06
} win32_small_rect;            // size 0x08
#endif

#if defined(HALO_WIN32_H) && defined(_WIN32)   /* the SDK's own type where windows.h is in (same layout and field names) */
typedef CONSOLE_SCREEN_BUFFER_INFO win32_console_screen_buffer_info;
#else
typedef struct win32_console_screen_buffer_info {
    win32_coord dwSize;               // 0x00
    win32_coord dwCursorPosition;     // 0x04
    uint16_t wAttributes;             // 0x08
    win32_small_rect srWindow;        // 0x0a
    win32_coord dwMaximumWindowSize;  // 0x12
} win32_console_screen_buffer_info;   // size 0x16
#endif

#if defined(HALO_WIN32_H) && defined(_WIN32)   /* the SDK's own type where windows.h is in (same layout and field names) */
typedef CONSOLE_CURSOR_INFO win32_console_cursor_info;
#else
typedef struct win32_console_cursor_info {
    int32_t bSize;             // 0x00
    int32_t bVisible;          // 0x04
} win32_console_cursor_info;   // size 0x08
#endif

typedef struct win32_key_event_record {
    int32_t bKeyDown;          // 0x00
    uint16_t wRepeatCount;     // 0x04
    uint16_t wVirtualKeyCode;  // 0x06
    uint16_t wVirtualScanCode; // 0x08
    uint16_t uChar;            // 0x0a AsciiChar in the low byte, the console is opened non Unicode
    uint32_t dwControlKeyState; // 0x0c
} win32_key_event_record;      // size 0x10

typedef struct win32_input_record {
    uint16_t EventType;               // 0x00 1 is KEY_EVENT
    uint16_t pad_02;                  // 0x02
    win32_key_event_record KeyEvent;  // 0x04 only the union member the console reads
} win32_input_record;                 // size 0x14

#if defined(HALO_WIN32_H) && defined(_WIN32)   /* the SDK's own type where windows.h is in (same layout and field names) */
typedef RECT win32_rect;
#else
typedef struct win32_rect {
    int32_t left;              // 0x00
    int32_t top;               // 0x04
    int32_t right;             // 0x08
    int32_t bottom;            // 0x0c
} win32_rect;                  // size 0x10
#endif

#if defined(HALO_WIN32_H) && defined(_WIN32)   /* the SDK's own type where windows.h is in (same layout and field names) */
typedef POINT win32_point;
#else
typedef struct win32_point {
    int32_t x;                 // 0x00
    int32_t y;                 // 0x04
} win32_point;                 // size 0x08
#endif

// ===========================================================================
// globals owned by this module
// ===========================================================================

// widget system
// global 0x006926c4: heap *widget_memory_pool           0x20000 byte GlobalAlloc arena
// global 0x0068e690: char *widget_memory_pool_name      the literal widget_memory_pool
// global 0x0068e67c: datum_index ui_cursor_bitmap       ui\shell\bitmaps\cursor
// global 0x00718f81: uint8_t ui_use_os_cursor
// global 0x00718f82: uint8_t ui_cursor_changed
// global 0x00718f84: int32_t ui_cursor_x                clamped into 0 .. 0x280
// global 0x00718f88: int32_t ui_cursor_y                clamped into 0 .. 0x1e0
// global 0x00718f8c: int32_t progress_screen_state      see progress_screen_state
// global 0x00718f90: int32_t progress_screen_progress
// global 0x00718f94: widget_instance *ui_root_widget[1]
// global 0x00718f98: widget_history_node *ui_widget_history[3]  one stack per controller slot
// global 0x00718f9c: int32_t ui_time_milliseconds       stamped into widget creation_time
// global 0x00718fa4: int16_t ui_unknown_718fa4          reset to -1
// global 0x00718fa6: int16_t ui_pause_depth             live widgets with pauses_game_time
// global 0x00718fa8: float ui_unknown_718fa8            reset to -1.0
// global 0x00718fac: pending message, 6 bytes: int16 error string (-1 none), uint16 player index,
//                    uint8 modal, uint8 is_error. Armed with 0x1f, 0x23 or 0x27 by quit and
//                    network code; interface_tick @0x497e80 forwards all four fields to
//                    display_error and resets the string to -1. src names: quit_confirm_error_*.
//                    0x49e090 indexes it by local player index times 6 (-1 becomes 0), so it is
//                    the slot 0 element of a per local player array; retail PC has one local
//                    player and nothing else reaches past slot 0 (0x00718fb2 is read as a lone
//                    int16 by interface_tick, see the next line).
// global 0x00718fb2: ui_pending_error ui_pending_error_alternate
// global 0x00718fb6: ui_pending_error ui_pending_errors[4]
// global 0x00718fc2: uint8_t widget_memory_pool_valid
// global 0x00718fc3: uint8_t widget_creating_children   suppresses recursion in 0x499780
// global 0x00718fc6: uint8_t main_menu_music_pending
// global 0x00718fc8: uint8_t ui_widget_opened
// global 0x00718fc9: uint8_t ui_split_screen
// global 0x00718fca: uint8_t ui_force_quit
// global 0x00718fcb: uint8_t ui_restoring_previous_widget
// global 0x00718fcc: uint8_t ui_network_wait_timed_out
// global 0x00718fcd: uint8_t ui_network_wait_active
// global 0x006927c4: int32_t ui_network_wait_start_time  -1 when no wait is running
// global 0x006927b8: float ui_saved_color[3]             read by 0x49c5c0 and 0x49c620
// global 0x006927d0: void *ui_event_function_table[0xbe] indexed by event handler function id
// global 0x00692708: uint16_t *ui_button_caption[0x28]   first entry is the a-button token
// global 0x00692b18: void *game_data_input_function_table[0x3b]  ui_game_data_input_function, called by 0x49a8c0 (0x49a956)
// global 0x00692c08: void *ui_replace_function_table[4]         ui_search_replace_function, called at 0x49b2ab and 0x4a873f

// developer console and terminal
// global 0x006b2efc: uint8_t terminal_initialized
// global 0x006b2f00: data_array *terminal_messages       terminal output, 0x20 console_message
// global 0x006b2f04: datum_index console_message_head    newest
// global 0x006b2f08: datum_index console_message_tail    oldest
// global 0x006b2f0c: terminal_console *console_active
// global 0x006b2f10: uint8_t console_caret_visible
// global 0x006b2f14: int32_t console_caret_blink_time
// global 0x006b2f18: uint8_t console_win32_attached
// global 0x006b2f1c: int32_t console_rcon_handle         -1 routes output to the win32 console
// global 0x006b2dd0: void *console_output_handle         win32 console output handle
// global 0x006b2dd8: char console_window_title[0x20]
// global 0x006b2df8: char console_last_line[0x100]
// global 0x006b2ef8: int32_t console_last_cursor_column
// global 0x0068e670: uint8_t console_show_messages
// global 0x00669140: char console_echo_prefix[]          matched by chimera__console_out
// global 0x0087ac06: uint8_t debug_log_level  -- ONE declaration shared by interface.h and
//   networking.h (R01; cseries.h calls it the shell debug level). All 11 .text accesses are
//   byte-wide: cmp BYTE ...,0x3 (0x440829, 0x440b24, 0x440d20, 0x4e0756), cmp BYTE ...,0x4
//   (0x489c4d, 0x496a86), mov al (0x440670, 0x440d80, 0x449450, 0x4d9960) and the shell's
//   mov BYTE PTR ds:0x87ac06,bl (0x540fac). Console output (0x496a80) needs > 3, network
//   statistics logging needs > 2. Formerly interface.h "int32_t console_verbosity" and
//   networking.h "int16_t network_statistics_level".

// loading and saving progress screen
// global 0x0068e680: uint32_t progress_screen_fade_end_time  milliseconds (0x449210 clock), -1 when none
// global 0x0068e684: int32_t progress_screen_start_time  -1 until the screen first draws
// global 0x0068e688: datum_index progress_screen_tag     -1 when none
// global 0x006b2f28: uint16_t progress_screen_text[0x20]
// global 0x006b2f68: uint16_t progress_screen_subtext[0x20]

// multiplayer map list
// global 0x00712dcc: map_list_entry *map_list
// global 0x00712dd0: int32_t map_list_count
// global 0x00712dd4: int32_t map_list_capacity           grown 0x13 entries at a time

// UI selection lists
// global 0x006b3830: growable_array ui_lists[3]          element size 0x10, ui_list_item
// global 0x00692c04: int32_t ui_list_current             index into ui_lists
// global 0x007192f8: uint8_t ui_list_has_default

// gamepad assignment lists (controls setup)
// global 0x006b42d8: controls_gamepad_record controls_available_gamepads[8]
// global 0x006b53d8: controls_gamepad_record controls_assigned_gamepads[4]
// global 0x00719448: int32_t controls_assigned_gamepad_count
// global 0x0071944c: int32_t controls_available_gamepad_count
// (0x006b1844 is input.h int32_t input_device_count: 17 DWORD accesses including the store
//  mov ds:0x6b1844,eax; the 4 WORD readers only need the low half. Connected devices, stride
//  0x240 at 0x006b1868.)

// video mode table
// global 0x006b6690: video_resolution video_resolutions[0x20]
// global 0x007196cc: int32_t video_resolution_count

// controls menu device labels
// global 0x006932e8: controls_device_label controls_device_labels[0x10]
// global 0x00719440: int32_t controls_device_label_count

// first person weapon interface
// global 0x006b2d98: first_person_weapon_interface *first_person_weapon_interfaces

// HUD runtime state, all six blocks reserved in one pass by hud_state_allocate @0x4a9780
// global 0x00719420: hud_globals_flags *hud_flags                 block size 0x004
// global 0x006b3a40: hud_messaging_globals *hud_messaging         block size 0x488
// global 0x0071942c: hud_unit_meter_state *hud_unit_meters        block size 0x05c
// global 0x00719430: hud_weapon_interface_state *hud_weapon_state block size 0x07c
// global 0x006b3a44: hud_waypoint_state *hud_waypoints            block size 0x030
// global 0x00719438: motion_sensor_globals *motion_sensor         block size 0x570
// global 0x0071941c: void *hud_globals_tag_data                   hud_globals tag data
// global 0x0071943c: int32_t motion_sensor_render_state

// HUD text message queue and chat listbox
// global 0x006b37e8: growable_array hud_text_message_queue        element size 0x14
// global 0x0071922c: int32_t hud_text_message_time_base
// global 0x006b3a20: int32_t hud_chat_message_expiry[8]           8 seconds past post time
// global 0x00719424: int32_t hud_chat_message_count               capped at 8
// global 0x00692ed8: uint8_t hud_chat_listbox_visible

// virtual keyboard
// global 0x007193a8: virtual_keyboard_globals virtual_keyboard

// player profile working state, owned jointly with the profile module at 0x0053a000
// global 0x00710328: player_control_settings player_control_settings_cache[]  stride 0x85c
// global 0x00714dd4: int32_t current_profile_index                -1 means the default profile
// global 0x00714dde: int16_t profile_slot_id[]                    scanned by 0x4954f0, indexed by player in 0x496060
// global 0x00714e7c: int32_t selected_saved_item                  low nibble 0 player profile, 1 variant
// global 0x00714e80: void *saved_item_working_copy
// global 0x00716e7c: void *saved_item_disk_copy
// global 0x00719410: int32_t virtual_keyboard.validation_mode (inside the block above, not a separate flag)

#pragma pack(pop)
