# `interface`: menus, widgets, console, HUD, chat, controls, video modes and the small UI lists

Retail Halo PC `halo.exe` 1.0.10, `0x44c290 .. 0x4c9c80` (376 Ghidra functions), plain C / MSVC 7.1 /
x86. Each file in this directory holds one function, rewritten from its Ghidra decompilation against
`types/interface.h`. The original decompile is kept at the bottom of each file inside `#if 0 ... #endif`.
Wherever Ghidra lost register arguments or whole blocks, the rewrite follows `objdump -d bin/halo.exe`,
and the file header gives the address range it was checked against.

Gate: `python tools/build_check.py interface` gives **365 ok, 0 failed** after the s2 part 2 review. Every
other module still passes with the updated header (whole tree 2872 ok, 0 failed).

365 source files plus this README: 376 listed addresses, less 11 that are not functions (table below).

## Coverage

| Range | Session | Listed | Written | Not written |
|---|---|---|---|---|
| `0x44c290 .. 0x49b55f` | s1 | 138 | 131 | `0x495190`, `0x4951f0`, `0x4974f0`, `0x498160`, `0x49a2e0`, `0x49aa00`, `0x49b450` |
| `0x49b560 .. 0x4ac0b0` | s2 part 1 | 148 | 146 | `0x49c369`, `0x49d850` |
| `0x4ac0b1 .. 0x4b031f` | s2 part 2, lower | 42 | 41 | `0x4add40` |
| `0x4b0320 .. 0x4c9c80` | s2 part 2, upper | 48 | 47 | `0x4b2f8a` |

Misattributed or not-a-function addresses. None of these has its own file.

| Address | Stale name | What it is | Where the logic lives |
|---|---|---|---|
| `0x495190` | `first_person_weapons_update` | Ghidra mis-split of the tail of `map_list_add_entry` (0x4950c0), `unaff_*` registers, no callers | `map_list_add_entry.c` |
| `0x4951f0` | `first_person_weapon_render_update` | the same mis-split tail as 0x495190 | `map_list_add_entry.c` |
| `0x4974f0` | `interface_draw_screen` | a `je` inside `chimera__do_show_loading_screen` | `chimera__do_show_loading_screen.c` |
| `0x498160` | `multiplayer_map_list_dispose` | the tail of `interface_tick` (same frame, no prologue) | `interface_tick.c` |
| `0x49a2e0` | `render_ui_cursor` | the child-dispatch tail of `widget_instance_handle_input_event` | `widget_instance_handle_input_event.c` |
| `0x49aa00` | `ui_widget_load_by_name_or_tag` | the tail of `widget_instance_render` | `widget_instance_render.c` |
| `0x49b450` | `render_ui_widgets` | bytes inside `widget_instance_render_text_box`, no prologue | none needed |
| `0x49c369` | `display_scenario_help_fail` | a compiler-duplicated tail fragment of `ui_check_for_pause_game` (0x49c1a0), no callers | `ui_check_for_pause_game.c` |
| `0x49d850` | `render_widget_recursive` | the map path scan tail of the unlisted function 0x49d7c0 | not in this module list |
| `0x4add40` | `hud_chat_to_network` | an address inside `hud_timer_draw` (0x4add10..0x4adf9f), between its second and third `hud_draw_number` calls | `hud_timer_draw.c` (the old `hud_chat_to_network.c` was deleted) |
| `0x4b2f8a` | `FUN_004b2f8a` | the crosshair type switch head inside `hud_weapon_crosshairs_draw` (target of the `jmp` at 0x4b2f80; byte map 0x4b3434, jump table 0x4b3424). The Ghidra decompile failure came from starting mid-function. | `hud_weapon_crosshairs_draw.c` (0x4b2cf0..0x4b3420) |

`0x4ac0b0` (`hud_draw_number`) is listed at 402 bytes but runs to 0x4ac6ce, and `0x4b2cf0` is listed short
but runs to 0x4b3420: Ghidra stopped at their switches. `0x4a47c0` owns the six entry jump table at 0x4a4850.

Unlisted functions found inside the module range (they are in no module list and have no file):

| Address | What it is |
|---|---|
| `0x49d7c0` | map list event handler that owns the 0x49d850 fragment (copies the selected map path, loads it, records it through 0x53d5e0) |
| `0x4ae530` | clears the active flag of all four HUD message slots of local player 0 (the same loop is inlined in the loading and checkpoint message functions) |
| `0x4b52f0`, `0x4b5350`, `0x4b54a0`, `0x4b54c0` | controls setup event handlers: start a binding capture on the focused row, leave the list mode, two small list handlers |
| `0x4bb290`, `0x4bb2e0`, `0x4bb300`, `0x4bb360`, `0x4bb3d0` | video options screen handlers between `video_options_menu_populate` and `video_options_reset_to_defaults` (0x4bb3d0 is the large one, about 0x200 bytes) |

## What the module contains

| Family | Range | Files | What it is |
|---|---|---|---|
| text edit | `0x44c290..0x44c7ff` | 5 | cursor and selection editing over a caller-owned 8-bit string (`text_edit_state`) |
| first-person weapon | `0x4923d0..0x49442f` | 27 | the `first_person_weapon_interface` record: attach, animation state, pose snapshots, HUD element matching, lighting, local player index helpers, interface game-state blocks |
| HUD draw helpers | `0x494430..0x494f4f` | 7 | text-draw configuration, weapon HUD lookup, first-person screen effects and zoom tint, split-screen dividers, rotated screen quad |
| map list | `0x494f50..0x4952bf` | 4 | GlobalAlloc vector of `map_list_entry`, friendly level names |
| player profile, saved items, net game | `0x4952c0..0x4963cf` | 18 | profile auto-select, video and audio options, settings cache, saved-item name editing, host start and teardown |
| terminal / console | `0x4963d0..0x49724f` | 19 | `console_message` data_array, `terminal_console`, win32 console mirror |
| widget system | `0x497250..0x49b55f` | 51 | loading screen, cursor, `widget_instance` tree, open/close/history, input dispatch, rendering, list navigation, button-prompt text |
| widget helpers | `0x49b560..0x49c67f` | 16 | list head and column list renderers, list selection restore, wide search-and-replace, focus next / previous child, pause check, history pop, reopen-as-root |
| profile name, network wait | `0x49c680..0x49c8ef` | 5 | profile name label, network wait timeout, trouble-brewing indicator |
| level select, host start | `0x49c8f0..0x49dd6f` | 6 | campaign level list, level carousel rows, multiplayer host start |
| profile list | `0x49dd70..0x4a004f` | 4 | saved profile enumeration into list ids, profile screens |
| options screens | `0x4a0050..0x4a3cdf` | 21 | controls, video, audio and network setup screens, event handlers that open the name entry |
| HUD text message queue | `0x4a3ce0..0x4a464f` | 3 | `hud_text_message` growable array: add, expire, draw |
| network fields, server browser | `0x4a4650..0x4a637f` | 19 | adapter and port fields, autopatch status, host name fields, server list page, game variant settings lists |
| carousels and UI lists | `0x4a6380..0x4a878f` | 26 | profile and variant carousels, the three `ui_lists` groups, the list widget row builder, the search-and-replace dispatcher |
| registry, virtual keyboard | `0x4a8790..0x4a977f` | 12 | product id, the on-screen keyboard (`virtual_keyboard_globals`) |
| HUD update | `0x4a9780..0x4aa6af` | 11 | HUD game-state blocks, per player HUD update, interaction prompt, loading and checkpoint messages, teammate waypoints |
| chat | `0x4aa6b0..0x4ab58f` | 15 | chat dialog and listbox through the embedded GUI library, network chat messages, the vehicle of a player |
| HUD numeric helpers | `0x4ab590..0x4ac6ef` | 8 | float truncation, anchor to screen, meter bitmap frame, flash color, meter fill, numbers |
| HUD element drawing | `0x4ac6f0..0x4ad8df` | 8 | static elements, overlays, bitmap placement and anchor extents, rotated quads, weapon ammo state, multitexture overlays (`ui_quad_render_state`) |
| HUD messaging | `0x4ad8e0..0x4af06f` | 19 | message text spans and icons, help and objective text, the mission timer, per player messages and arguments, network item messages, `hud_messaging_update` |
| HUD waypoints | `0x4af070..0x4afd2f` | 10 | waypoint arrows, activation per player and team, target and visibility update, arrow drawing |
| unit HUD | `0x4afd30..0x4b173f` | 7 | unit HUD sounds, shield and health meters, `hud_render_unit_interface`, damage indicators, predictive shield damage |
| weapon HUD | `0x4b1740..0x4b344f` | 6 | `hud_weapon_interface_state`, meter evaluation, weapon and grenade interfaces, crosshairs |
| motion sensor | `0x4b3450..0x4b43bf` | 8 | blip types, sampling, plotting, rendering (`motion_sensor_globals`) |
| controls setup | `0x4b43c0..0x4b555f` | 12 | binding rows, action names, device labels, presets, binding capture, device mode panels |
| gamepad assignment | `0x4b5560..0x4b5cdf` | 8 | assigned (4, profile +0x1108) and available (8) gamepad lists: find, add, remove, load, refresh, move, restore bindings (`controls_gamepad_record`) |
| video modes | `0x4bab50..0x4bb7d9` | 9 | D3D9 display mode enumeration into `video_resolution`, the video options screen |
| command line | `0x4c9c80..0x4c9dcd` | 1 | `-connect` / `-name` / `-password` autojoin |

Call flow for one UI frame:

```
interface_tick @0x497e80
  loading-thread poll, pending dialogs (0x00718fb2, 0x00718fac)
  0x4922b0 builds a ui_input_event per controller
    widget_instance_handle_input_event @0x499d00       close requests, auto close, list/tab navigation
      ui_widget_list_item_activate @0x49a430           event handler actions (script, function, open, close, back, focus)
        chimera__load_ui_widget @0x497a70              open a widget; pushes the go-back record
          widget_initialize_from_tag @0x499780
            widget_create_children_from_tag @0x499540
  history pop: chimera__load_ui_widget + 0x49bd00 (restore the list selection)
widget_draw_fullscreen_region @0x4984c0 / widget_draw_split_screen_region @0x498330
  widget_instance_render @0x49a8c0                     background quad, game data inputs, per-type renderer
    widget_instance_render_text_box @0x49b1d0          text, search-and-replace, button prompts
      ui_widget_draw_formatted_prompt_string @0x49ade0
        ui_widget_draw_prompt_span @0x49ad30, ui_button_prompt_draw_icon @0x49ac80
    widget_instance_render_list_head @0x49b560         scroll arrows and caption
```

## Calling conventions recovered from the disassembly

Ghidra drops most register arguments in this module. These are the ones the rewrites depend on:

| Callee | Convention |
|---|---|
| `chimera__load_ui_widget` 0x497a70 | cdecl, **7** stack args: path, tag index, parent widget (NULL = install as root and push history), controller, history definition, history list definition, history selection |
| `widget_initialize_from_tag` 0x499780 | ECX widget, EAX tag index, EDX parent; stack controller, tag data |
| `tag_lookup` 0x442550 | EDI group tag (`'DeLa'`, `'bitm'`, `'snd!'`, `'ustr'`, `'lsnd'`); stack path |
| `bitmap_group_sequence_get_bitmap_data` 0x43f290 | EAX bitmap tag, DI frame; stack sequence |
| `ui_draw_screen_quad` 0x498b20 | EAX source rect, ECX dest rect; stack bitmap data, clip rect, **packed ARGB vertex color** (not a float) |
| `FUN_00449780` | EAX packed color, ECX rect: solid rectangle fill |
| `FUN_005562d0` | EBX origin rect, ESI cursor rect (in/out), EDI output rect; stack text: measures a text span |
| `chimera__draw_16_bit_text` 0x514ab0 | EAX clip rect (may be NULL), ECX bounds rect; stack 0, 0, text |
| `text_set_render_context` 0x5563b0 | ECX font, EAX ColorARGB; stack -1, justification, 0 |
| `text_string_list_get_string` 0x5578c0 | ECX tag, DX index |
| `FUN_00557990` | EAX wide dest, EDI dest bytes, EBX 8-bit source: 8-bit to wide copy |
| `ui_string_replace_all` 0x49be10 | cdecl: search, replacement, &text |
| `FUN_0049bba0` / `FUN_0049bb60` | EAX widget; ECX child (bba0) or stack child definition (bb60): focus changes |
| `FUN_0049c080` / `FUN_0049c0f0` | EDX widget: focus previous / next child |
| `widget_instance_close_and_restore_previous` 0x49c3e0 | EAX widget |
| `chimera__console_out` 0x496b50 | EAX ColorARGB* (NULL = default gray); stack format, varargs |
| `FUN_00543ce0` (sound) | ESI object, ECX position, EAX forward; stack sound, marker, gain, flag |
| `widget_play_sound_effect` 0x498e90 | AX one-based effect: 1 cursor, 2 forward (keyboard open), 3 back (keyboard close and commit), 4 rejected |
| `heap_reallocate` 0x4d1f80 | EAX old block, ESI heap (`widget_memory_pool`), stack new size; every s2 text slot is reallocated in place |
| `set_profile_name` 0x49c710 | EBX widget (usually `extended_description` or its first child), stack wide name |
| `FUN_0049bac0` column list renderer | EDI widget, stack tag, dest, offset, flags |
| `FUN_0049c4c0` reopen as root | cdecl (widget, tag); returns the new widget from `chimera__load_ui_widget` |
| `widget_list_scroll_window` 0x4a7400 | EAX out int32[3], ECX widget |
| `FUN_004a4e20` level carousel row | ECX row widget, EAX level index |
| `ui_list_widget_rebuild_rows` 0x4a7db0 | cdecl (widget, row formatter); the formatter returns a byte |
| `ui_profile_carousel_slot_cache_populate` 0x4a74b0 | stack count, EBX id array |
| `ui_variant_carousel_slot_cache_populate` 0x4a7570 | EBX id array, stack count (the same shape; the first rewrite had the count in EAX) |
| `ui_search_replace_function_call` 0x4a8730 | AX function index, ECX widget (forwarded as the stack argument of the table function) |
| `player_profile_details_widget_refresh` 0x4a6100 | EAX widget, stack profile record |
| `virtual_keyboard_open` 0x4a89a0 | ESI destination, stack maximum length and field kind (words); returns a byte |
| `virtual_keyboard_character_is_legal` 0x4a8b80 | EAX validation mode, CL character |
| `ui_variant_name_is_available` 0x4a8b50 | EDI wide name |
| `FUN_00557950` wide to 8-bit copy | ESI out, EDI wide source, stack length |
| `hud_update_interaction_prompt` 0x4a9b80 | EDX player datum |
| HUD message setters 0x4adfc0, 0x4ae050, 0x4ae0b0, 0x4ae110 | 0x4adfc0: EAX message index, stack local player; the others: EAX local player, ESI argument slot (0x4ae110: BL shown) |
| `hud_waypoint_draw_one` 0x4aa440 | EAX player datum |
| `ui_draw_rotated_screen_quad` 0x494d70 | EAX origin, stack bitmap, uvs, scale, rotation, **alpha** (five stack arguments) |
| `FUN_0051c9a0` quad submit | EAX `ui_quad_render_state`, stack vertices |
| `hud_meter_flash_color_blend` 0x4ab980 | ESI `hud_flash_parameters`, EDI start time |
| `hud_meter_draw_fill` 0x4abbc0 | ESI `hud_meter_placement`, six stack arguments |
| `hud_meter_resolve_bitmap_frame` 0x4ab8d0 | EAX frame, stack tag, sequence, out bitmap data (preset by the caller, not cleared), out sprite rect |
| `hud_anchor_offset_to_screen_position` 0x4ab690 | AL has scale, EDX offset, ECX child placement or NULL, stack anchor, scale, out |
| `chat_dispatch_incoming` 0x4aaf70 | EAX network event; the decoder 0x4ec590 takes EAX event, ECX `chat_incoming_record` |
| `saved_game_enumerate_by_type` 0x53c4e0 | EBX in/out int16 count, stack type, out slots, flag |
| `color_interpolate` 0x43f6a0 | ECX a, EAX b, stack out, flags, t (the other modules declare it without a and b) |

### Register conventions from 0x4ab170 on (from the file headers)

Every function from `0x4ab170` on whose file declares a `blam-cc` line. Stack arguments follow in C order.

| Function | Address | Register convention |
|---|---|---|
| `player_get_vehicle` | `0x4ab170` | ECX player datum; returns EAX. |
| `bitmap_group_sequence_get_bitmap_offset` | `0x4ab630` | bitmap tag in ECX (in_ECX), sequence_index in AX (in_AX), frame_index in DI (unaff_DI), all unresolved register reads. |
| `hud_anchor_offset_to_screen_position` | `0x4ab690` | anchor pointer in ECX->... |
| `hud_meter_resolve_bitmap_frame` | `0x4ab8d0` | EAX frame; stack bitmap_tag, sequence_index, out_data, out_offset (the C parameter order below follows the callers; frame_index is the EAX argument). |
| `hud_meter_flash_color_blend` | `0x4ab980` | flash parameters in ESI, start time in EDI. |
| `hud_meter_draw_fill` | `0x4abbc0` | placement in ESI; six stack arguments. |
| `hud_draw_bitmap_element` | `0x4acad0` | EAX uv (NULL for the whole bitmap), EDX placement, BL pixel_uvs; seven stack arguments, the last one a byte. |
| `hud_draw_bitmap_at` | `0x4acbb0` | EAX uv (NULL for the whole bitmap), EDX bitmap, CL pixel_uvs; five stack arguments. |
| `hud_bitmap_anchor_extents` | `0x4acc50` | CL pixel_uvs, ESI bitmap, EDX uv rect, EAX out; anchor on the stack. |
| `hud_draw_rotated_bitmap_quad` | `0x4acd50` | EAX screen position (Point2DInt), ESI scale (float[2]); six stack args. |
| `hud_player_weapon_ammo_state` | `0x4acef0` | EAX player; one stack argument; returns a byte in AL. |
| `hud_draw_multitexture_overlay` | `0x4acfe0` | EAX scale (float[2]); seven stack arguments. |
| `hud_draw_message_text_span` | `0x4ad8e0` | EAX cursor, ECX origin; two stack arguments. |
| `hud_draw_message_icon` | `0x4ad970` | ESI information block; two stack arguments. |
| `hud_set_timer_time` | `0x4adbf0` | ECX minutes, EAX seconds. |
| `hud_pause_timer` | `0x4adc60` | DL paused. |
| `hud_set_player_message` | `0x4adfc0` | EAX message index; one stack argument (local player index). |
| `hud_set_message_icon_argument` | `0x4ae050` | EAX local player index, ESI slot; one stack argument. |
| `hud_set_message_string_argument` | `0x4ae0b0` | EAX local player index, ESI slot; two stack arguments. |
| `hud_set_action_text_shown` | `0x4ae110` | EAX local player index, BL shown. |
| `chimera__hud_message` | `0x4ae180` | local player index in AX. |
| `hud_receive_item_message` | `0x4ae200` | EAX message. |
| `hud_post_item_message` | `0x4ae350` | EAX count, ECX source, DL kind; two stack arguments. |
| `hud_add_item_message` | `0x4ae400` | EAX local player index, ECX source, BL kind; one stack argument. |
| `hud_message_find_slot` | `0x4ae480` | ESI source; two stack arguments. |
| `hud_messaging_update` | `0x4ae550` | AX local player index. |
| `hud_waypoint_arrow_find` | `0x4af070` | EDI name; the result is in AX. |
| `hud_waypoint_activate_for_player` | `0x4af0d0` | EAX player, EBX target, DX kind; two stack arguments. |
| `hud_waypoint_activate_for_team` | `0x4af1b0` | EAX target; four stack arguments. |
| `hud_waypoint_deactivate_for_player` | `0x4af230` | EAX player, EDI target, SI kind. |
| `hud_waypoint_deactivate_for_team` | `0x4af2b0` | EAX kind; two stack arguments. |
| `hud_waypoint_visibility` | `0x4af540` | AX local player index, ECX eye, EDX target; one stack argument; the result is in EAX (0 visible, 2 occluded), stored into bits 4..7 of hud_waypoint::type. |
| `hud_waypoint_draw` | `0x4af5e0` | EAX position; four stack arguments. |
| `hud_unit_sounds_update` | `0x4afee0` | EAX player; one stack argument (a byte). |
| `hud_unit_meters_update_for_player` | `0x4b0160` | DI local player index. |
| `hud_draw_damage_indicators` | `0x4b14c0` | local player index in EAX, read only as its low 16 bits. |
| `hud_unit_meter_apply_predictive_damage` | `0x4b16e0` | player datum_index in ECX (in_ECX, index low 16 / salt high 16); damage amount as the one recovered stack parameter. |
| `hud_weapon_interface_meters_evaluate` | `0x4b1970` | EAX a datum_index (the hud_interface tag id to chain from, or the caller's hud_globals+0x2cc default when there is no weapon); stack: local_player_index, weapon_or_vehicle_index, and a pointer to a 32-byte scratch record the caller either zeroed or filled via FUN_004c29d0. |
| `hud_weapon_crosshairs_draw` | `0x4b2cf0` | EAX hud interface tag id, ECX player record; one stack argument. |
| `blip_type_get` | `0x4b3450` | EBX object; one stack argument (a short); result in AL. |
| `motion_sensor_blip_fill` | `0x4b35f0` | EAX local player index, ECX object, ESI blip. |
| `motion_sensor_plot_blip` | `0x4b37a0` | EAX position, BL type; five stack arguments. |
| `motion_sensor_render` | `0x4b4120` | EAX screen center, CX local player index; one stack argument. |
| `controls_key_is_bindable` | `0x4b43c0` | action value in EDX (in_EDX, unresolved register). |
| `controls_enumerate_next_assignable_action` | `0x4b43e0` | EAX device, ECX record, EDI action name; one stack argument (a byte). |
| `controls_action_display_name` | `0x4b44c0` | EAX device, EDI action name. |
| `controls_binding_row_widget_update` | `0x4b4520` | EAX action index; two stack arguments. |
| `controls_binding_list_refresh_rows` | `0x4b4790` | EAX widget; one stack argument. |
| `controls_action_column_is_bindable` | `0x4b4df0` | ECX which slot (0 primary, 1 secondary), EDX action index. |
| `controls_binding_clear` | `0x4b4e20` | EAX action index; one stack argument. |
| `controls_binding_rows_toggle_device_mode` | `0x4b53a0` | ESI widget; one stack byte. |
| `controls_gamepad_widget_nodes_collect` | `0x4b5560` | EAX out array, ECX screen widget; no stack arguments. |
| `controls_gamepad_lists_refresh` | `0x4b55d0` | ECX screen widget (passed straight to 0x4b5560); no stack arguments. |
| `controls_gamepad_list_find` | `0x4b5760` | EDX the list base (controls_assigned_gamepads or controls_available_gamepads); the record to match in the one recovered stack parameter (param_1). |
| `controls_gamepad_list_add` | `0x4b5800` | EAX the list base; the record to copy in the one recovered stack parameter (param_1). |
| `controls_gamepad_list_remove` | `0x4b5850` | EDI the list base (unaff_EDI -- never assigned in this function, so it must be a register argument its caller sets up, exactly like controls_gamepad_list_find's own EDX argument); the record to match in the one recovered stack parameter (param_1). |
| `video_display_modes_enumerate` | `0x4baba0` | EBX the D3DFORMAT; no stack arguments. |
| `video_resolution_add` | `0x4badc0` | EAX height; stack width, refresh rate. |
| `video_refresh_rate_find_index` | `0x4bae80` | ECX resolution index, EDI refresh rate value (both registers, per Ghidra's unresolved in_ECX/unaff_EDI). |

## Struct layouts

All of these live in `types/interface.h`, generated from the header by the phase-4 s2 part 2 review. Offsets are byte offsets and `#pragma pack(push,1)` is in force. The win32 console, rect and point copies at the end of the header are omitted. Enums (`ui_edit_key_code`, `ui_sound_effect`, `blip_type`, `progress_screen_state`) and function pointer types are in the header only.

### `ui_key_event` (size 0x04)

input records shared by the text editor and the console (widget_text_edit_process_key @0x44c290, console_process_queued_input @0x4965e0) console_process_queued_input copies one 4-byte record out of the buffered key ring at 0x006b16fe and both stores it in the console and passes it to the editor.

| Offset | Type | Field | Notes |
|---|---|---|---|
| `0x00` | `uint8_t` | `modifiers` | bit 0 tested by 0x44c290 to extend the selection |
| `0x01` | `uint8_t` | `character` | 0xff means no character; below 0x20 is not inserted |
| `0x02` | `int16_t` | `key_code` | 0x1d, 0x4f, 0x50 and 0x54 are special-cased |

### `text_edit_state` (size 0x0a)

text_edit_state (widget_text_edit_process_key @0x44c290, widget_text_edit_reset_length @0x44c5b0, widget_text_edit_get_selection @0x44c5e0, widget_text_edit_insert_string @0x44c640, widget_text_edit_clamp_selection @0x44c780) A cursor/selection pair over a caller-owned NUL-terminated 8-bit string. The selection runs between anchor and cursor; anchor -1 means no selection, and clamp_selection collapses it to -1 whenever the anchor lands on the cursor. The console embeds one of these at terminal_console::edit.

| Offset | Type | Field | Notes |
|---|---|---|---|
| `0x00` | `char *` | `text` | not owned; strlen is recomputed on every call |
| `0x04` | `int16_t` | `maximum_length` | insertion stops once strlen reaches this |
| `0x06` | `int16_t` | `cursor` | clamped into 0 .. strlen |
| `0x08` | `int16_t` | `selection_anchor` | clamped into -1 .. strlen, -1 means no selection |

### `widget_instance` (size 0x60)

widget_instance (widget_open @0x497a70 allocates it from the widget heap, widget_initialize_from_tag @0x499780 fills it, widget_close @0x497c00 frees it, widget_create_children_from_tag @0x499540 links the tree, widget_instance_point_in_bounds @0x4999f0, widget_get_sibling_index @0x498e30, widget_instance_get_cumulative_scale @0x499c20) widget_initialize_from_tag zeroes exactly 0x18 dwords before writing any field, which is what fixes the size at 0x60. Everything the instance copies comes out of the matching UIWidgetDefinition in types/tags.h, so the tag offsets are quoted in the comments rather than duplicated as fields. The tree is a parent pointer plus a doubly linked sibling list. The focus stack that widget_instance_is_top_of_stack @0x499cb0 walks is the same tree read upward, checking at each step that parent->focused_child is the node it came from.

| Offset | Type | Field | Notes |
|---|---|---|---|
| `0x00` | `datum_index` | `definition` | ui_widget_definition tag index |
| `0x04` | `char *` | `name` | points at definition->name, tag data + 0x04 |
| `0x08` | `int16_t` | `controller_index` | from definition->controller_index, 4 becomes -1 |
| `0x0a` | `int16_t` | `local_x` | child_widget_reference + 0x38 plus the parent value |
| `0x0c` | `int16_t` | `local_y` | child_widget_reference + 0x36 plus the parent value |
| `0x0e` | `int16_t` | `widget_type` | UIWidgetType_t copied from definition + 0x00 |
| `0x10` | `uint8_t` | `state` | set to 1 at creation, rewritten by 0x498e60 |
| `0x11` | `uint8_t` | `render_always` | definition flags bit 9 |
| `0x12` | `uint8_t` | `hidden` | skipped by the focus and hit-test walks |
| `0x13` | `uint8_t` | `pauses_game_time` | definition flags bit 1, drives ui_pause_depth |
| `0x14` | `uint8_t` | `closing` | widget_close latches this to stay reentrant |
| `0x15` | `uint8_t` | `unknown_15` | tested by main_menu_on_shown @0x498ab0 |
| `0x16` | `uint8_t` | `unknown_16[2]` |  |
| `0x18` | `int32_t` | `creation_time` | copied from ui_time_milliseconds |
| `0x1c` | `int32_t` | `milliseconds_to_auto_close` | definition + 0x30, negatives clamped to 0 |
| `0x20` | `int32_t` | `milliseconds_auto_close_fade` | definition + 0x34, negatives clamped to 0 |
| `0x24` | `float` | `scale` | 1.0 at creation, multiplied up the parent chain |
| `0x28` | `struct widget_instance *` | `previous_sibling` |  |
| `0x2c` | `struct widget_instance *` | `next_sibling` |  |
| `0x30` | `struct widget_instance *` | `parent` |  |
| `0x34` | `struct widget_instance *` | `first_child` |  |
| `0x38` | `struct widget_instance *` | `focused_child` |  |
| `0x3c` | `void *` | `text` | text_box: heap block freed by widget_close; list: see below |
| `0x40` | `int16_t` | `selection_index` | -1 for a text_box, 0 for a list |
| `0x42` | `int16_t` | `scroll_blink` | advanced by widget_instance_render_list_head |
| `0x44` | `void *` | `list_items` | built by ui_build_profile_list, freed by 0x49df70; text_box: see below |
| `0x48` | `uint16_t` | `item_count` | incremented once per child created from the tag |
| `0x4a` | `int16_t` | `unknown_4a` |  |
| `0x4c` | `struct widget_instance *` | `extended_description` | definition + 0x1b0, closed recursively |
| `0x50` | `void *` | `list_render_data` | list types only, freed by widget_close |
| `0x54` | `int16_t` | `scroll_direction` | zeroed for spinner_list and column_list |
| `0x56` | `int16_t` | `unknown_56` |  |
| `0x58` | `int16_t` | `background_bitmap_frame` | 0 or 1, 1 marks the selected list_head child |
| `0x5a` | `uint8_t` | `unknown_5a[4]` |  |
| `0x5e` | `int16_t` | `background_bitmap_frames` | sequence 0 frame count of the background bitmap |

### `widget_history_node` (size 0x10)

widget_history_node (list_node_prepend @0x499430, list_node_pop @0x499460, widget_pool_list_free_all @0x4994b0, pushed by chimera__load_ui_widget @0x497a70, popped by interface_tick @0x497e80 and 0x49c3e0) A record pushed on the per-controller go-back stack. The nodes are allocated from the same widget heap as the widgets themselves. chimera__load_ui_widget builds the template from its own arguments 5, 6 and 7 plus the controller_index of the root widget it replaces (-1 when the slot was empty). On a pop the definition is reopened with controller_index as argument 4, then 0x49bd00 gets list_definition in EAX and the dword at 0x08 on the stack (it reads only the low word) to restore the selection.

| Offset | Type | Field | Notes |
|---|---|---|---|
| `0x00` | `datum_index` | `definition` | widget tag to reload |
| `0x04` | `datum_index` | `list_definition` | child located by 0x499950 on reopen, -1 for none |
| `0x08` | `int16_t` | `selection` | list index restored by 0x49bd00 |
| `0x0a` | `int16_t` | `controller_index` | controller of the replaced root widget, -1 for none |
| `0x0c` | `struct widget_history_node *` | `next` |  |

### `ui_pending_error` (size 0x04)

ui_pending_error (display_error @0x498f20, interface_handle_quit_request @0x499170) While the interface is not ready to open a dialog, display_error parks the request here and interface_tick opens it later. The record is indexed by player slot with a 4-byte stride: display_error writes the index as a short at +0x00 and the two flags as bytes at +0x02 and +0x03 of the same element.

| Offset | Type | Field | Notes |
|---|---|---|---|
| `0x00` | `int16_t` | `error_string_index` | -1 means the slot is empty |
| `0x02` | `uint8_t` | `modal` |  |
| `0x03` | `uint8_t` | `is_error` | picks the error_ rather than the warning_ dialog tag |

### `console_message` (size 0x124)

console_message (terminal_initialize @0x4963d0 creates the data_array, console_message_new @0x496420, console_message_delete @0x496490, console_message_expire_old @0x4966e0, chimera__console_out @0x496b50, console_draw_overlay @0x496730) Datums in the terminal output data_array, capacity 0x20, element size 0x124, kept in a doubly linked list ordered newest first. The element size is what pins the layout: the text runs from 0x0d up to the color at 0x110, and vsnprintf is handed a 0xfe byte limit inside it.

| Offset | Type | Field | Notes |
|---|---|---|---|
| `0x00` | `int16_t` | `identifier` | datum_header |
| `0x02` | `int16_t` | `pad_02` |  |
| `0x04` | `datum_index` | `previous` | toward the newest message |
| `0x08` | `datum_index` | `next` | toward the oldest message |
| `0x0c` | `uint8_t` | `is_command_echo` | set when the text contains the prefix at 0x00669140 |
| `0x0d` | `char` | `text[0x103]` | vsnprintf with a 0xfe byte limit |
| `0x110` | `ColorARGB` | `color` | defaults to 1.0, 0.7, 0.7, 0.7 |
| `0x120` | `int32_t` | `age` | frames since creation, deleted once it passes 150 |

### `terminal_console` (size 0x1be)

terminal_console (console_open @0x496510, console_close @0x496580, console_process_queued_input @0x4965e0, console_draw_overlay @0x496730, console_restore_cursor @0x496c20, console_update_display @0x496d40, console_draw_input_line @0x4970a0, console_position_cursor @0x4971a0) Owned by the caller of console_open, which installs it as console_active. console_open builds the edit state in place: edit.text is set to the address of input[0] and edit.maximum_length to 0xff, which is what fixes input at 0x100 bytes and places edit at 0x1b4. console_draw_overlay terminates prompt at 0xb3 and input at 0x1b3, giving both buffers their exact lengths.

| Offset | Type | Field | Notes |
|---|---|---|---|
| `0x00` | `int16_t` | `key_event_count` | cleared every frame, capped at 0x20 |
| `0x02` | `ui_key_event` | `key_events[0x20]` |  |
| `0x82` | `uint8_t` | `unknown_82[2]` |  |
| `0x84` | `ColorARGB` | `color` | used for the input line |
| `0x94` | `char` | `prompt[0x20]` | window title, also copied to console_window_title |
| `0xb4` | `char` | `input[0x100]` | the line being typed |
| `0x1b4` | `text_edit_state` | `edit` | edit.text is input, maximum_length 0xff |

### `map_list_entry` (size 0x0c)

map_list_entry (map_list_add_entry @0x4950c0, map_list_append_entry_variant @0x495190, map_list_finalize_entry @0x4951f0, map_list_free_all @0x495260, map_list_find_known_map_index @0x494ff0, map_list_get_friendly_level_name @0x494f50) GlobalAlloc-backed vector grown 0x13 entries at a time. The 0xc stride is the GlobalReAlloc size computation itself, capacity times 0xc.

| Offset | Type | Field | Notes |
|---|---|---|---|
| `0x00` | `char *` | `path` | GlobalAlloc, lowercased, extension stripped |
| `0x04` | `int32_t` | `map_id` |  |
| `0x08` | `uint8_t` | `cache_file_exists` | return of cache_file_exists on the stripped path |
| `0x09` | `uint8_t` | `pad_09[3]` |  |

### `video_resolution` (size 0x4c)

video_resolution (video_resolution_add @0x4badc0, video_resolution_list_build @0x4bad40, video_refresh_rate_find_index @0x4bae80, video_resolution_compare @0x4bab50, video_display_modes_enumerate @0x4baba0) Fixed table of 0x20 entries. The 0x4c stride comes from the refresh_rate_count access, which indexes a dword array with a 0x13 dword stride; the refresh rates are capped at 8 per resolution by an explicit compare.

| Offset | Type | Field | Notes |
|---|---|---|---|
| `0x00` | `int32_t` | `width` |  |
| `0x04` | `int32_t` | `height` |  |
| `0x08` | `uint16_t` | `name[16]` | wide string built from the %d x %d pattern; name[15] (0x26) forced to 0 |
| `0x28` | `uint32_t` | `refresh_rate_count` | never allowed past 8 |
| `0x2c` | `int32_t` | `refresh_rates[8]` |  |

### `d3d_display_mode` (size 0x10)

d3d_display_mode, d3d9_interface (video_display_modes_enumerate @0x4baba0) D3DDISPLAYMODE and the two IDirect3D9 vtable slots the mode enumeration calls; the interface pointer is the global at 0x0071d178.

| Offset | Type | Field | Notes |
|---|---|---|---|
| `0x00` | `uint32_t` | `width` |  |
| `0x04` | `uint32_t` | `height` |  |
| `0x08` | `uint32_t` | `refresh_rate` |  |
| `0x0c` | `uint32_t` | `format` | D3DFORMAT |

### `d3d9_interface_vtable` (size not stated)

| Offset | Type | Field | Notes |
|---|---|---|---|
| `0x00` | `void *` | `unknown_00[6]` | IUnknown, RegisterSoftwareDevice, GetAdapterCount, GetAdapterIdentifier |
| `0x18` | `function pointer` | `get_adapter_mode_count` |  |
| `0x1c` | `function pointer` | `enum_adapter_modes` |  |

### `d3d9_interface` (size not stated)

| Offset | Type | Field | Notes |
|---|---|---|---|
| `0x00` | `d3d9_interface_vtable *` | `vtable` |  |

### `ui_list_item` (size 0x10)

ui_list_item (ui_list_add_entry @0x4a7ba0, ui_list_free_all @0x4a7b20, ui_list_get_data @0x4a7c50, ui_list_get_id @0x4a7c80, ui_list_find_default @0x4a7cb0, consumed by the list-widget builder at 0x4a7db0) Elements of three growable_array instances from types/memory.h laid end to end at 0x006b3830. ui_list_free_all walks them with a 0xc stride and stops at 0x006b385c, which is what fixes the group at three arrays; growable_array_add_element is called with element_size 0x10.

| Offset | Type | Field | Notes |
|---|---|---|---|
| `0x00` | `uint16_t *` | `name` | GlobalAlloc wide string, freed by ui_list_free_all |
| `0x04` | `void *` | `data` | optional GlobalAlloc blob, freed with the name |
| `0x08` | `int32_t` | `id` |  |
| `0x0c` | `uint8_t` | `is_default` | nonzero raises ui_list_has_default |
| `0x0d` | `uint8_t` | `pad_0d[3]` |  |

### `controls_gamepad_record` (size 0x220)

controls_gamepad_record (controls_gamepad_list_find @0x4b5760, controls_gamepad_list_add @0x4b5800, controls_gamepad_list_remove @0x4b5850, controls_gamepad_lists_load @0x4b58d0, controls_gamepad_toggle_assignment @0x4b5b20, controls_build_device_label_table @0x4b4890) One gamepad (input device) record. The profile keeps four at +0x1108 (the assigned gamepads, labelled as controls devices 2..5 by controls_build_device_label_table); the connected devices are the 0x240 stride table at 0x006b1868 (count int16 0x006b1844), of which the first 0x220 bytes are copied. Phase 4 review: formerly server_browser_entry, but no caller is on a server screen; the 0x14 byte key at 0x20c was only assumed to be an s_network_address and is probably the device instance GUID plus one dword (UNSURE). The size is exact: the add routine copies 0x88 dwords and the remove routine compacts with a 0x220 byte stride; the find routine compares the key as five dwords.

| Offset | Type | Field | Notes |
|---|---|---|---|
| `0x000` | `uint16_t` | `name[0x106]` | wide device name, the first 0x3f characters are shown |
| `0x20c` | `uint32_t` | `device_key[5]` | match key, UNSURE: GUID plus one dword |

### `controls_device_label` (size 0x210)

controls_device_label (controls_device_label_add @0x4b4830, controls_build_device_label_table @0x4b4890) Fixed table of 16. The add routine walks it with a 0x210 byte stride and stops at 0x006953e8, and the build routine zeroes exactly 0x840 dwords, 0x2100 bytes.

| Offset | Type | Field | Notes |
|---|---|---|---|
| `0x000` | `uint16_t` | `name[0x105]` | wide string, wcsncpy with a 0x104 limit then terminated |
| `0x20a` | `uint16_t` | `pad_20a` |  |
| `0x20c` | `int32_t` | `device_type` | 0 for the tag-supplied label, 2 plus slot for a device |

### `input_guid` (size 0x10)

input_guid (controls_apply_preset @0x4b4c50 passes the default profile GUID at 0x0065b8e0 by value to input_device_default_profile_tag_find @0x490110)

| Offset | Type | Field | Notes |
|---|---|---|---|
| `0x00` | `uint32_t` | `words[4]` |  |

### `first_person_weapon_interface` (size 0x1ea0)

first_person_weapon_interface (interface_globals_allocate @0x494340 reserves it, interface_local_player_state_reset @0x494390 clears it, first_person_weapon_interface_initialize @0x493c60, first_person_weapon_set_state @0x492e60, first_person_weapon_snapshot_pose @0x4930b0, first_person_weapon_update @0x493150, first_person_weapon_update_animation_controls @0x493740, first_person_weapon_process_action @0x4940f0, local_player_index_for_object @0x4926f0) The size is exact on both sides: 0x494340 bumps the game-state cursor by 0x1ea0 and folds that constant into the state checksum, and 0x494390 zeroes 0x7a8 dwords. Every accessor indexes first_person_weapon_interfaces with a 0x1ea0 stride, and local_player_index_for_object stops after index 0, so retail PC has one entry.

| Offset | Type | Field | Notes |
|---|---|---|---|
| `0x0000` | `uint8_t` | `attached` | 1 while the first person model is attached |
| `0x0001` | `uint8_t` | `unknown_01[3]` |  |
| `0x0004` | `datum_index` | `unit_index` | the controlled unit object |
| `0x0008` | `datum_index` | `weapon_index` | the unit current weapon object |
| `0x000c` | `int16_t` | `state` | animation state, driven by 0x492d20 and 0x492e60 |
| `0x000e` | `int16_t` | `unknown_0e` |  |
| `0x0010` | `int16_t` | `unknown_10` |  |
| `0x0012` | `int16_t` | `shutdown_countdown` | reseeded to 0x1e by 0x4942e0 |
| `0x0014` | `int16_t` | `animation_index` | first person animation index, -1 when none |
| `0x0016` | `int16_t` | `unknown_16` |  |
| `0x0018` | `uint8_t` | `unknown_18[2]` |  |
| `0x001a` | `int16_t` | `unknown_1a` |  |
| `0x001c` | `uint8_t` | `unknown_1c[4]` |  |
| `0x0020` | `int16_t` | `unknown_20` |  |
| `0x0022` | `uint8_t` | `unknown_22[6]` |  |
| `0x0028` | `float` | `unknown_28` | cleared together with the next field by 0x493c60 |
| `0x002c` | `float` | `charge` | nudged by action code 0 in 0x4940f0 |
| `0x0030` | `uint8_t` | `unknown_30[0x58]` | aim sway and idle timers written by 0x493150 |
| `0x0088` | `int16_t` | `blend_start` | written by 0x4930b0 when a blended change starts |
| `0x008a` | `int16_t` | `blend_end` |  |
| `0x008c` | `uint8_t` | `animation_control[0x800]` | node control block fed to the animation system |
| `0x088c` | `uint8_t` | `previous_pose[0x800]` | copied from animation_control by 0x4930b0 |
| `0x108c` | `uint8_t` | `unknown_108c[0xd00]` | node scratch gathered by 0x493ea0 and 0x4924b0 |
| `0x1d8c` | `uint8_t` | `weapon_hud_valid` | result of hud_meter_find_matching_element |
| `0x1d8d` | `uint8_t` | `pad_1d8d` |  |
| `0x1d8e` | `int16_t` | `weapon_hud_element[0x40]` | match table filled by 0x493f00 |
| `0x1e0e` | `uint8_t` | `device_hud_valid` | second hud_meter_find_matching_element result |
| `0x1e0f` | `uint8_t` | `pad_1e0f` |  |
| `0x1e10` | `int16_t` | `device_hud_element[0x44]` | second match table |
| `0x1e98` | `int32_t` | `frame_sound_impulse` | reset to -1 |
| `0x1e9c` | `int16_t` | `frame_sound_state` | reset to -1 |
| `0x1e9e` | `int16_t` | `unknown_1e9e` |  |

### `hud_message_slot` (size 0x8c)

hud_message_slot (chimera__hud_message @0x4ae180, hud_add_item_message @0x4ae400, hud_message_find_slot @0x4ae480, hud_message_compare @0x4ae500, drawn by hud_messaging_update @0x4ae550) Four slots per local player. The 0x8c stride is the loop bound in hud_message_find_slot, and the text length is the wcsncpy limit of 0x3f.

| Offset | Type | Field | Notes |
|---|---|---|---|
| `0x00` | `int32_t` | `timestamp` | game time when the message was posted |
| `0x04` | `uint16_t` | `text[0x3f]` | wcsncpy with a 0x3f limit |
| `0x82` | `uint8_t` | `active` |  |
| `0x83` | `uint8_t` | `sequence` | from the rolling counter in hud_messaging_globals |
| `0x84` | `int32_t` | `source` | -1 for a plain text message, else the item tag id whose pickup_text_index string is drawn; matched by find_slot |
| `0x88` | `int16_t` | `count` | item message count, summed by hud_add_item_message and divided by the item hud_message_value_scale when drawn |
| `0x8a` | `uint8_t` | `source_kind` | second half of the find_slot match key; added to the item pickup_text_index (0xff: add 1 when count > 1) |
| `0x8b` | `uint8_t` | `pad_8b` |  |

### `hud_player_messaging_state` (size 0x460)

hud_player_messaging_state (hud_set_player_message @0x4adfc0, hud_set_message_icon_argument @0x4ae050, hud_set_message_string_argument @0x4ae0b0, hud_set_action_text_shown @0x4ae110, chimera__hud_message @0x4ae180, hud_add_item_message @0x4ae400, drawn by hud_messaging_update @0x4ae550) One per local player, 0x460 bytes; every accessor multiplies the local player index by 0x460 against hud_messaging. The "action message" is either a message of the HUDGlobals hud_messages tag (message, with up to 8 substitution arguments that the message elements 0x20.. reference) or, when message is NULL, the wide action_text.

| Offset | Type | Field | Notes |
|---|---|---|---|
| `0x000` | `hud_message_slot` | `messages[4]` | sorted with hud_message_compare every draw |
| `0x230` | `uint16_t` | `action_text[0x100]` | wcsncpy with a 0xff limit; drawn when message is NULL |
| `0x430` | `uint8_t` | `unknown_430[4]` |  |
| `0x434` | `int32_t` | `arguments[8]` | per argument: a hud_messaging_information pointer (an icon, argument_is_string bit clear) or a string reference {int16 index, uint8 from_scenario_names} |
| `0x454` | `struct HUDMessageTextMessage *` | `message` | HUDGlobals hud_messages message set by 0x4adfc0; NULL blocks every argument store |
| `0x458` | `uint8_t` | `message_shown` | the action message line is drawn while set |
| `0x459` | `uint8_t` | `argument_is_string` | bit i set when arguments[i] is a string reference |
| `0x45a` | `uint8_t` | `unknown_45a[4]` |  |
| `0x45e` | `uint8_t` | `prompt_changed` | raised by 0x4ae110 when message_shown flips, cleared when a slot message is posted; keeps the line reserved |
| `0x45f` | `uint8_t` | `message_shown_copy` | written with message_shown by 0x4ae110 |

### `hud_messaging_information` (size 0x10)

hud_messaging_information (ESI of hud_draw_message_icon @0x4ad970, stored as an icon argument by hud_set_message_icon_argument @0x4ae050) The messaging_information block of WeaponHUDInterface (+0x13c in types/tags.h); the unit and grenade HUD interfaces repeat it.

| Offset | Type | Field | Notes |
|---|---|---|---|
| `0x00` | `uint16_t` | `sequence_index` | sequence of the HUDGlobals icon_bitmap |
| `0x02` | `int16_t` | `width_offset` | added to the icon width, or the whole advance (flag 2) |
| `0x04` | `Point2DInt` | `offset` | from the text cursor (x) and its bottom (y) |
| `0x08` | `ColorARGBInt` | `override_icon_color` | used when flags bit 1 is set |
| `0x0c` | `int8_t` | `frame_rate` | game ticks per frame divisor, 0 for a still icon |
| `0x0d` | `uint8_t` | `flags` | HUDInterfaceMessagingFlags: bit 1 override color, bit 2 width offset is the absolute icon width |
| `0x0e` | `uint16_t` | `text_index` |  |

### `hud_item_message` (size 0x08)

hud_item_message (payload of network message type 6: encoded by hud_post_item_message @0x4ae350 on a networked game, decoded by hud_receive_item_message @0x4ae200)

| Offset | Type | Field | Notes |
|---|---|---|---|
| `0x00` | `datum_index` | `item_definition` | item tag (weapon or equipment) whose pickup text is used |
| `0x04` | `uint8_t` | `kind` | hud_message_slot::source_kind; 0xff means plural by count |
| `0x05` | `uint8_t` | `pad_05` |  |
| `0x06` | `int16_t` | `count` | added to hud_message_slot::count |

### `hud_messaging_globals` (size 0x488)

hud_messaging_globals (game-state block reserved by hud_state_allocate @0x4a9780 with an exact size of 0x488 and cleared by hud_state_reset @0x4a98d0 with a 0x122 dword fill; the timer fields are read by hud_counter_get_value @0x4adcc0 and drawn by hud_timer_draw @0x4add10). One player record plus a 0x28 byte tail holding the script-set help text, objective text and the countdown timer (hud_set_timer_time 0x4adbf0, pause_hud_timer 0x4adc60 are reached through the hs function table, no direct callers).

| Offset | Type | Field | Notes |
|---|---|---|---|
| `0x000` | `hud_player_messaging_state` | `players[1]` |  |
| `0x460` | `int32_t` | `help_text_flash_start_time` | start time of the help text flash |
| `0x464` | `uint8_t` | `help_text_flashing` | help text flashes (HUDGlobals hud_help flash) while set |
| `0x465` | `uint8_t` | `next_sequence` | post-incremented by every message post |
| `0x466` | `uint8_t` | `unknown_466[6]` |  |
| `0x46c` | `struct HUDMessageTextMessage *` | `help_text` | hud_set_help_text (0x4adb30), stored only while hud_globals_flags::help_text_shown is set |
| `0x470` | `struct HUDMessageTextMessage *` | `objective_text` | hud_set_objective_text (0x4adb80) |
| `0x474` | `int16_t` | `objective_text_ticks` | HUDGlobals objective uptime + fade ticks |
| `0x476` | `int16_t` | `unknown_476` |  |
| `0x478` | `int32_t` | `timer_start_time` | game time the timer was (re)armed; hud_timer_draw rebases it when the warning time is reached |
| `0x47c` | `uint16_t` | `timer_ticks` | ticks on the clock; 0xffff once it ran out |
| `0x47e` | `int16_t` | `timer_warning_ticks` | the flashing not-much-time-left threshold |
| `0x480` | `Point2DInt` | `timer_offset` | anchor offset of the three timer numbers |
| `0x484` | `int16_t` | `timer_anchor` | HUD corner 0..4, clamped by hud_set_timer_time |
| `0x486` | `uint8_t` | `timer_paused` | pause_hud_timer; timer_ticks then holds the remaining time |
| `0x487` | `uint8_t` | `timer_active` | set by hud_set_timer_time, gates hud_timer_draw |

### `hud_text_message` (size 0x14)

hud_text_message (hud_text_message_queue_init @0x4a3ce0, hud_text_message_queue_add @0x4a3d90, hud_text_message_queue_update_and_draw @0x4a3e30) Elements of a plain growable_array from types/memory.h whose header sits at 0x006b37e8; the init routine writes element_size 0x14, count 0 and data NULL straight into it. The leading escapes recognised in the text are backslash s, whose digits give a delay in units of 0x10, and backslash h.

| Offset | Type | Field | Notes |
|---|---|---|---|
| `0x00` | `uint16_t *` | `text` | points past any recognised escape marker |
| `0x04` | `int32_t` | `unknown_04` | caller-supplied tag or index |
| `0x08` | `int32_t` | `hold` | 1 when the text started with the hold escape |
| `0x0c` | `int32_t` | `start_time` |  |
| `0x10` | `int32_t` | `end_time` | start_time plus the computed duration |

### `hud_waypoint` (size 0x0c)

hud_waypoint (hud_waypoint_activate_for_player @0x4af0d0, hud_waypoint_deactivate_for_player @0x4af230, hud_waypoints_update_for_player @0x4af370, hud_waypoints_draw_for_player @0x4afb90 -> hud_waypoint_draw @0x4af5e0) Four slots per local player (the activate_nav_point_* script functions). Both the set and the clear routine index the slots with a 0xc stride inside a 0x30 byte per-player record, and the containing game-state block is exactly 0x30 bytes, so retail PC has one player record.

| Offset | Type | Field | Notes |
|---|---|---|---|
| `0x00` | `int16_t` | `arrow_index` | HUDGlobals waypoint_arrows index (found by name through 0x4af070); 0xffff marks the slot empty |
| `0x02` | `int16_t` | `type` | bits 0..3 the target kind (0 scenario cutscene flag, 1 object, 2 custom waypoint), 0xf marks the slot free; bits 4..7 the visibility written by the update (0 in view, 2 occluded) |
| `0x04` | `float` | `vertical_offset` | added to the target z |
| `0x08` | `datum_index` | `object_index` | flag, object or custom waypoint index; -1 marks the slot free |

### `hud_waypoint_state` (size 0x30)

| Offset | Type | Field | Notes |
|---|---|---|---|
| `0x00` | `hud_waypoint` | `waypoints[4]` |  |

### `motion_sensor_blip` (size 0x04)

motion sensor (motion_sensor_reset @0x4b3660, blip_fill @0x4b35f0, motion_sensor_update @0x4b3920, motion_sensor_update_for_player @0x4b3e10, motion_sensor_plot_blip @0x4b37a0, motion_sensor_render @0x4b4120) motion_sensor_reset gives the whole layout away: it zeroes 0x15c dwords, 0x570 bytes, then writes the empty marker into 10 groups of 16 records, stepping the record by 4 bytes and the group by 0x84. The per-player stride of 0x568 and the frame cursor at 0x56c come from the render and update routines.

| Offset | Type | Field | Notes |
|---|---|---|---|
| `0x00` | `int8_t` | `x` | sensor-space offset, ftol of the relative position |
| `0x01` | `int8_t` | `y` |  |
| `0x02` | `uint8_t` | `type` | blip_type, 6 means the slot is empty |
| `0x03` | `uint8_t` | `subtype` | 0 to 2, picks the icon variant |

### `motion_sensor_frame` (size 0x84)

| Offset | Type | Field | Notes |
|---|---|---|---|
| `0x00` | `motion_sensor_blip` | `blips[0x10]` |  |
| `0x40` | `int8_t` | `extra_blips[0x20]` | x and y pairs for the sound-driven blips |
| `0x60` | `uint8_t` | `extra_sources[0x10]` | source id per extra blip |
| `0x70` | `float` | `viewer_x` |  |
| `0x74` | `float` | `viewer_y` |  |
| `0x78` | `int32_t` | `blip_count` |  |
| `0x7c` | `float` | `viewer_facing` | camera yaw plus half pi |
| `0x80` | `uint8_t` | `extra_blip_count` |  |
| `0x81` | `uint8_t` | `pad_81[3]` |  |

### `motion_sensor_player_state` (size 0x568)

| Offset | Type | Field | Notes |
|---|---|---|---|
| `0x000` | `motion_sensor_frame` | `history[10]` | ring buffer, oldest fades out |
| `0x528` | `datum_index` | `tracked_objects[0x10]` | one per blip slot, -1 when unused |

### `motion_sensor_globals` (size 0x570)

| Offset | Type | Field | Notes |
|---|---|---|---|
| `0x000` | `motion_sensor_player_state` | `players[1]` |  |
| `0x568` | `int32_t` | `update_time` | game time of the last motion_sensor_update |
| `0x56c` | `int16_t` | `frame_index` | current ring slot, stepped modulo 10 |
| `0x56e` | `uint8_t` | `enabled` | set by motion_sensor_update; update_for_player needs it |
| `0x56f` | `uint8_t` | `pad_56f` |  |

### `hud_unit_meter_state` (size 0x58)

hud_unit_meter_state / hud_unit_meter_globals (game-state block of exactly 0x5c bytes reserved by hud_state_allocate @0x4a9780 at 0x0071942c; hud_state_reset @0x4a98d0 clears 0x17 dwords and then writes the sentinels below; used by hud_meter_update_value @0x4b0160, hud_unit_sounds_update @0x4afee0, hud_render_unit_interface @0x4b0320 and the predictive damage hook at 0x4b16e0) One 0x58 byte record per local player (every accessor multiplies the local player index by 0x58) followed by the script flags dword at +0x58. The displayed shield and health lag behind the unit values, so they start at -1.0 to force a snap on the first frame.

| Offset | Type | Field | Notes |
|---|---|---|---|
| `0x00` | `float` | `displayed_shield` | -1.0 until the first update; held for 15 ticks when the shield drops, then follows it (0x4b0160) |
| `0x04` | `float` | `displayed_health` | -1.0 until the first update; the unit health of the last draw |
| `0x08` | `float` | `shield_drain_time` | seconds (1/30 per tick) the shield has been dropping, -1.0 while idle or recharging; flash time of the meter |
| `0x0c` | `int32_t` | `shield_update_time` | game time of the last 0x08 step |
| `0x10` | `int32_t` | `shield_flash_start_time` | set while the shield panel flashes, else -1 |
| `0x14` | `int32_t` | `health_flash_start_time` | set while the health panel flashes, else -1 |
| `0x18` | `int32_t` | `motion_sensor_flash_start_time` | set while the motion sensor flashes, else -1 |
| `0x1c` | `datum_index` | `last_unit` | the unit of the last draw; -1 resets the record |
| `0x20` | `uint16_t` | `auxiliary_meters_shown` | bit per UnitHUDInterfaceMeterPanelType shown last draw |
| `0x22` | `int16_t` | `auxiliary_meter_timers[1]` | per meter type (integrated light only): ticks of the background flash, -1 when idle |
| `0x24` | `uint16_t` | `sounds_playing` | bit per UnitHUDInterface sound (hud_unit_sounds_play 0x4afd30) |
| `0x26` | `uint8_t` | `pad_26[2]` |  |
| `0x28` | `int32_t` | `sound_handles[12]` | impulse sound or looping sound datum per HUD sound, -1 when stopped |

### `hud_unit_meter_globals` (size 0x5c)

| Offset | Type | Field | Notes |
|---|---|---|---|
| `0x00` | `hud_unit_meter_state` | `players[1]` |  |
| `0x58` | `uint32_t` | `flags` | hud_show_* / hud_blink_* script flags: bit 0 hide health, bit 1 blink health, bit 2 hide shield, bit 3 blink shield, bit 4 hide motion sensor, bit 5 blink motion sensor; bits 0 and 2 also silence the health and shield warning sounds |

### `hud_sound_start_parameters` (size 0x0c)

hud_sound_start_parameters (hud_unit_sounds_play @0x4afd30 builds it on the stack) UNSURE: probably owned by the sound module.

| Offset | Type | Field | Notes |
|---|---|---|---|
| `0x00` | `int16_t` | `unknown_00` | written 0 |
| `0x02` | `int16_t` | `pad_02` |  |
| `0x04` | `float` | `scale` | the HUD sound scale |
| `0x08` | `float` | `gain` | 1.0 |

### `hud_weapon_interface_player` (size 0x28)

hud_weapon_interface_state (game-state block of exactly 0x7c bytes reserved by hud_state_allocate @0x4a9780 and filled with -1 by hud_state_reset @0x4a98d0, 0x1f dwords; written by 0x4b1740, 0x4b1970, 0x4b1e20, 0x4b1ff0, 0x4b2ac0 and 0x4b2cf0) Phase 4 review: the former flat entries[0x1f] view is split by use. Each local player has a 0x28 byte block at player * 0x28 (flash start times of the weapon HUD elements, the weapon the cache belongs to, the grenade flash start time) and a 0x50 byte meter / crosshair state at 0x28 + player * 0x50 (19 evaluated values and the active mask at entry 0x13, filled by hud_weapon_interface_meters_evaluate). With one local player the two arrays and the flags dword end exactly at 0x7c. The flags dword at 0x78 bit 0 gates every crosshair draw (hud_weapon_crosshairs_draw @0x4b2cfd) and is set or cleared by a script function at 0x480c4e (show_hud_crosshair, UNSURE name).

| Offset | Type | Field | Notes |
|---|---|---|---|
| `0x00` | `int32_t` | `flash_start_times[8]` | per weapon HUD element flash, -1 when idle |
| `0x20` | `datum_index` | `weapon` | weapon or vehicle the cached state belongs to |
| `0x24` | `int32_t` | `grenade_flash_start_time` |  |

### `hud_weapon_meter_state` (size 0x50)

| Offset | Type | Field | Notes |
|---|---|---|---|
| `0x00` | `int32_t` | `values[0x13]` | evaluated meter and crosshair inputs |
| `0x4c` | `uint32_t` | `active_mask` | entry 0x13, bit per evaluated state |

### `hud_weapon_interface_state` (size 0x7c)

| Offset | Type | Field | Notes |
|---|---|---|---|
| `0x00` | `hud_weapon_interface_player` | `players[1]` | indexed by local player |
| `0x28` | `hud_weapon_meter_state` | `meters[1]` | indexed by local player |
| `0x78` | `uint32_t` | `flags` | bit 0 crosshairs shown |

### `hud_globals_flags` (size 0x04)

hud_globals_flags (game-state block of exactly 4 bytes reserved by hud_state_allocate @0x4a9780; byte 0 is set to 1 by hud_state_reset @0x4a98d0 and byte 1 is tested as a gate by 0x4adfc0, 0x4ae050 and 0x4ae0b0)

| Offset | Type | Field | Notes |
|---|---|---|---|
| `0x00` | `uint8_t` | `hud_enabled` |  |
| `0x01` | `uint8_t` | `help_text_shown` | show_hud_help_text: while set every message and argument store is blocked and hud_set_help_text (0x4adb30) is taken |
| `0x02` | `uint8_t` | `unknown_02[2]` |  |

### `virtual_keyboard_globals` (size 0x74)

virtual_keyboard_globals (virtual_keyboard_initialize @0x4a88f0, virtual_keyboard_open @0x4a89a0, virtual_keyboard_process_input @0x4a8be0, virtual_keyboard_close @0x4a9250, virtual_keyboard_backspace @0x4a96f0, virtual_keyboard_draw_text @0x4a9300, virtual_keyboard_render @0x4a9510) Not a passed-around structure: this is the contiguous global block starting at 0x007193a8, written field by field. The text buffer length is the wcsncpy limit of 0x20 wide characters used on open, and maximum_length is clamped to 0x40. The open routine takes the destination buffer in ESI and two stack words, maximum_length and field_kind, and returns a byte. The UI sounds it plays are 2 on open, 3 on close and commit, 1 on every edit and cursor move, 4 on a rejected character.

| Offset | Type | Field | Notes |
|---|---|---|---|
| `0x00` | `uint8_t` | `active` | at 0x007193a8 |
| `0x01` | `uint8_t` | `unknown_01` |  |
| `0x02` | `uint8_t` | `unknown_02` |  |
| `0x03` | `uint8_t` | `unknown_03` |  |
| `0x04` | `void *` | `strings_tag_data` | tag data of ui\english, NULL disables the keyboard |
| `0x08` | `int16_t` | `caret` |  |
| `0x0a` | `int16_t` | `unknown_0a` |  |
| `0x0c` | `int16_t` | `maximum_length` | bytes, clamped to 0x40, over 0x32 selects the small ui |
| `0x0e` | `int16_t` | `selection_start` | reset to -1 |
| `0x10` | `int16_t` | `selection_end` | reset to -1 |
| `0x12` | `int16_t` | `unknown_12` |  |
| `0x14` | `int16_t` | `field_kind` | checked by the character filter at 0x4a8b80 |
| `0x16` | `uint8_t` | `committed` | cleared on open and on close |
| `0x17` | `uint8_t` | `opened` | set to 1 on open |
| `0x18` | `uint16_t *` | `destination` | caller buffer, written back by virtual_keyboard_close |
| `0x1c` | `uint16_t *` | `destination_end` | destination plus its current length |
| `0x20` | `int32_t` | `open_time` |  |
| `0x24` | `datum_index` | `white_bitmap` | ui\shell\bitmaps\white |
| `0x28` | `uint16_t` | `text[0x20]` | the line being edited |
| `0x68` | `int32_t` | `validation_mode` | at 0x00719410, a dword written after every open: 1 by the open itself (profile name), 2 by the variant and profile editors, 3 by 0x4a2c50 (an empty entry restores the text), 4 and 5 by the address and port fields (0x4a3a8f, 0x4a4b1f), 0 by 0x4a2c80 and 0x4b67a7. Commit in virtual_keyboard_process_input branches on 1..3 and the character filter 0x4a8b80 (EAX) on 3..5 |
| `0x6c` | `datum_index` | `large_ui_tag` |  |
| `0x70` | `datum_index` | `small_ui_tag` |  |

### `player_control_settings` (size 0x85c)

player_control_settings (player_profile_refresh_settings_cache @0x496060) The live per-controller settings block the input and UI code reads. 0x496060 builds it on the stack from the 0x2004 byte saved profile record at 0x00712dd8 + slot * 0x2004 (owned by the profile module around 0x0053a000) and copies it out with rep movsd, ecx 0x217 dwords, to 0x00710328 + slot * 0x85c (imul edi, edi, 0x85c at 0x4963ba). The earlier 0x217 byte size was the Ghidra dword-pointer index read as bytes. Most ranges are verbatim copies; the profile offset each came from is quoted (profile+X is relative to the 0x2004 byte record). The float fields are the 0 to 9 slider bytes remapped through three hard-coded tables: table 80: 80, 100, 120, 140, 160, 180, 200, 220, 240, 260 table 40: 40, 50, 60, 70, 80, 90, 100, 110, 120, 130 table 01: 0.1, 0.25, 0.5, 0.75, 1, 1.25, 1.5, 2, 3, 4

| Offset | Type | Field | Notes |
|---|---|---|---|
| `0x000` | `float` | `look_rate_80` | table 80 at clamp(profile+0x12e minus 1, 0, 9) |
| `0x004` | `float` | `look_rate_40` | table 40 at the same index |
| `0x008` | `uint8_t` | `unknown_008[0xda]` | profile+0x134 verbatim |
| `0x0e2` | `uint32_t` | `unknown_0e2[7]` | profile+0x20e verbatim |
| `0x0fe` | `uint8_t` | `unknown_0fe[0x100]` | profile+0x22a verbatim |
| `0x1fe` | `uint32_t` | `unknown_1fe[4]` | profile+0x32a verbatim |
| `0x20e` | `uint8_t` | `unknown_20e[0x200]` | profile+0x33a verbatim |
| `0x40e` | `uint8_t` | `unknown_40e[0x400]` | profile+0x53a verbatim |
| `0x80e` | `uint8_t` | `pad_80e[2]` | always zero |
| `0x810` | `uint32_t` | `unknown_810[6]` | profile+0x93c verbatim |
| `0x828` | `float` | `sensitivity_01_a` | table 01 at min(profile+0x954, 9) |
| `0x82c` | `float` | `sensitivity_01_b` | table 01 at min(profile+0x955, 9) |
| `0x830` | `uint32_t` | `unknown_830[2]` | profile+0x960 verbatim |
| `0x838` | `float` | `rate_80[4]` | table 80 at min(profile+0x956 + i, 9) |
| `0x848` | `float` | `rate_40[4]` | table 40 at min(profile+0x95a + i, 9) |
| `0x858` | `uint8_t` | `unknown_858` | profile+0x12f |
| `0x859` | `uint8_t` | `unknown_859` | profile+0x131 |
| `0x85a` | `uint8_t` | `pad_85a[2]` | always zero |

### `loading_thread_record` (size 0x05 as read (true allocation unknown))

loading_thread_record (interface_tick @0x497e80, chimera__load_main_menu @0x4989f0) Pointed to by the global at 0x00718fbc while the background map loading thread runs. Only the win32 thread handle and one status byte are ever read.

| Offset | Type | Field | Notes |
|---|---|---|---|
| `0x00` | `void *` | `handle` | win32 thread handle, waited on and closed |
| `0x04` | `uint8_t` | `unknown_04` | UNSURE, set by the thread when it finishes |

### `weapon_screen_effect_parameters` (size 0x38)

weapon_screen_effect_parameters (first_person_weapon_update_screen_effects @0x494730) The 0x38 byte block 0x494730 zeroes on its stack (rep stosd, 0x0e dwords), fills from the first WeaponHUDInterfaceScreenEffect of the local player weapon HUD interface and hands to the rasterizer at 0x52d8a0 (or, in EAX, to 0x52e2d0 on older hardware). The rasterizer side is not rewritten, so only the fields 0x494730 writes are named.

| Offset | Type | Field | Notes |
|---|---|---|---|
| `0x00` | `int16_t` | `unknown_00` | never written by 0x494730 |
| `0x02` | `int16_t` | `convolution_type` | 2 when a convolution amount is set |
| `0x04` | `float` | `convolution_amount` | radius interpolated over the fov bounds |
| `0x08` | `uint32_t` | `mask_bitmap_data` | mask bitmap tag +0x64, bitmaps.pointer |
| `0x0c` | `float` | `night_vision_intensity` |  |
| `0x10` | `float` | `desaturation_intensity` |  |
| `0x14` | `float` | `desaturation_tint[3]` | ColorRGB effect_tint |
| `0x20` | `uint8_t` | `desaturation_additive` | desaturation_flags bit 2 |
| `0x21` | `uint8_t` | `night_vision_masked` | night vision flags bit 2 |
| `0x22` | `uint8_t` | `desaturation_masked` | desaturation_flags bit 3 |
| `0x23` | `uint8_t` | `unknown_23[0x15]` | never written by 0x494730 |

### `first_person_light_parameters` (size 0x20)

first_person_light_parameters (first_person_weapon_update_lighting @0x4924b0) The 0x20 byte stack block handed by pointer to the render routine at 0x4d6fc0 for both first person models. Armed while the unit has flag 0x10 at +0x204 or a positive float at +0x37c; otherwise only armed and zero are written.

| Offset | Type | Field | Notes |
|---|---|---|---|
| `0x00` | `uint16_t` | `armed` | 1 when the unit light is on |
| `0x02` | `uint16_t` | `pad_02` | never written |
| `0x04` | `float` | `unknown_37c` | unit +0x37c |
| `0x08` | `float` | `unknown_380` | unit +0x380 |
| `0x0c` | `uint32_t` | `unit_handle` | the controlled unit datum |
| `0x10` | `float` | `camera_x` | copied from 0x007c3114 |
| `0x14` | `float` | `camera_y` | 0x007c3118 |
| `0x18` | `float` | `camera_z` | 0x007c311c |
| `0x1c` | `uint32_t` | `zero` | always 0 |

### `ui_input_event` (size 0x08 as read (the producer reserves 0x10))

ui_input_event (built by 0x4922b0 into the interface_tick 16 byte stack scratch, consumed by widget_instance_handle_input_event @0x499d00 and passed on to ui_widget_list_item_activate @0x49a430 and the event functions below) Kinds, from the jump table at 0x49a3e8: 1 dpad or first stick, 2 second stick, 3 button, 4 mouse button, 5 custom activation (matched against event type 0x20). For kinds 1 and 2 the word at 0x04 is the horizontal axis and the word at 0x06 the vertical one; only the extremes -0x8000 and 0x7fff are ever compared. For kinds 3 and 4 the byte at 0x04 is the button code and the byte at 0x05 is 1 on a press. The src files index this record as int16_t event[4] with the same offsets.

| Offset | Type | Field | Notes |
|---|---|---|---|
| `0x00` | `int16_t` | `kind` | 1 to 5, see above |
| `0x02` | `int16_t` | `controller_index` | compared against widget_instance::controller_index |
| `0x04` | `uint8_t` | `code` | button code, also the low byte of the horizontal axis |
| `0x05` | `uint8_t` | `pressed` | 1 on a press, also the high byte of the horizontal axis |
| `0x06` | `int16_t` | `axis_y` | vertical axis extreme for kinds 1 and 2 |

### `ui_quad_render_state` (size 0x8c)

ui_quad_render_state (built on the stack by ui_draw_screen_quad @0x498b20, ui_draw_rotated_screen_quad @0x494d70, hud_draw_rotated_bitmap_quad @0x4acd50 and hud_draw_multitexture_overlay @0x4acfe0, handed in EAX to the rasterizer quad submitter 0x51c9a0 together with the four hud_quad_vertex records that follow it on the stack) Every builder zeroes the whole 0x23 dwords first. The single-bitmap builders then write only meter_parameters, maps[0], map_scales[0], map_texel_scales[0] (all 1.0) and the blend function; hud_draw_multitexture_overlay (0x4ad20e..0x4ad86e) fills all three map slots and the effector outputs, which is where the per-map field meanings below come from.

| Offset | Type | Field | Notes |
|---|---|---|---|
| `0x00` | `void *` | `meter_parameters` | hud_meter_color_block* from hud_meter_draw_fill through 0x4acad0 and 0x4acd50, else NULL |
| `0x04` | `float *` | `geometry_offset` | float[2] x, y written by the geometry_offset effectors of a multitexture overlay, else NULL |
| `0x08` | `uint32_t` | `unknown_08` | never written by the interface builders |
| `0x0c` | `BitmapData *` | `maps[3]` | primary, secondary, tertiary bitmap data |
| `0x18` | `uint8_t` | `wrap_modes[3]` | HUDInterfaceWrapMode low byte per map |
| `0x1b` | `uint8_t` | `pad_1b` |  |
| `0x1c` | `Point2D *` | `map_offsets[3]` | the copy owned by the caller of primary/secondary/tertiary offset; effectors add to it through this pointer |
| `0x28` | `Point2D` | `map_scales[3]` | 1 / tag scale per axis, 1.0 where the tag scale is 0 |
| `0x40` | `Point2D` | `map_texel_scales[3]` | 1/width, 1/height for a non power of two map, else 1.0 |
| `0x58` | `ColorRGB *` | `map_tints[3]` | tint_0_1 effector output per map, else NULL |
| `0x64` | `uint8_t` | `unknown_64[0x14]` | never written by the interface builders |
| `0x78` | `float *` | `map_fades[3]` | fade_0_1 effector output per map, else NULL |
| `0x84` | `int16_t` | `zero_to_one_blend` | HUDInterfaceZeroToOneBlendFunction remapped: |
| `0x86` | `int16_t` | `one_to_two_blend` | tag 0,1,2,3,4 -> 0,2,1,3,4 (jump table 0x4ad890) |
| `0x88` | `int16_t` | `framebuffer_blend_function` | 0 for ui_draw_screen_quad, 7 (alpha multiply add) for the rotated quads, the tag value for an overlay |
| `0x8a` | `uint8_t` | `single_local_player` | player_globals::unknown_0c == 1 for a multitexture overlay, 0 elsewhere |
| `0x8b` | `uint8_t` | `pad_8b` |  |

### `hud_quad_vertex` (size 0x18)

hud_quad_vertex (the four records that follow ui_quad_render_state on the stack of every builder above; 0x51c9a0 takes a pointer to the first one as its only stack argument) Vertex i uses u[(i+1)&2 ? 1 : 0] and v[i > 1 ? 1 : 0], i.e. (u0,v0) (u1,v0) (u1,v1) (u0,v1).

| Offset | Type | Field | Notes |
|---|---|---|---|
| `0x00` | `float` | `x` | screen pixels |
| `0x04` | `float` | `y` |  |
| `0x08` | `float` | `z` | always 0.0 |
| `0x0c` | `uint32_t` | `color` | packed ARGB |
| `0x10` | `float` | `u` |  |
| `0x14` | `float` | `v` |  |

### `hud_flash_parameters` (size 0x18)

hud_flash_parameters (hud_meter_flash_color_blend @0x4ab980, which gets a pointer to one in ESI; its 8 callers in 0x4ac000..0x4b4000 point it into HUD interface tag elements) The 0x18 byte run default_color .. flash_length that every HUD interface element of types/tags.h repeats inline (GrenadeHUDInterfaceOverlay +0x24, the unit and weapon HUD statics, meters and overlays). tags.h has no struct of its own for it.

| Offset | Type | Field | Notes |
|---|---|---|---|
| `0x00` | `ColorARGBInt` | `default_color` |  |
| `0x04` | `ColorARGBInt` | `flashing_color` |  |
| `0x08` | `float` | `flash_period` | seconds; 0.0 means never flash |
| `0x0c` | `float` | `flash_delay` | seconds between flashes |
| `0x10` | `int16_t` | `number_of_flashes` |  |
| `0x12` | `uint16_t` | `flash_flags` | bit 0 reverse_default_flashing_colors |
| `0x14` | `float` | `flash_length` | seconds; 0.0 means never flash |

### `hud_meter_placement` (size 0x5c (tag element bytes 0x24 .. 0x80))

hud_meter_placement (hud_meter_draw_fill @0x4abbc0 takes a pointer to one in ESI) A view of a HUD meter element of types/tags.h starting at its anchor_offset field: element +0x24 in WeaponHUDInterfaceMeter (and the grenade and unit meters, which share the layout). Every offset below was matched against WeaponHUDInterfaceMeter + 0x24.

| Offset | Type | Field | Notes |
|---|---|---|---|
| `0x00` | `Point2DInt` | `anchor_offset` |  |
| `0x04` | `float` | `width_scale` |  |
| `0x08` | `float` | `height_scale` |  |
| `0x0c` | `uint16_t` | `scaling_flags` | bit 2 halves the draw alpha in 0x4abbc0 |
| `0x0e` | `uint8_t` | `pad_0e[0x16]` |  |
| `0x24` | `TagDependency` | `meter_bitmap` | tag_id at 0x30 |
| `0x34` | `ColorARGBInt` | `color_at_meter_minimum` |  |
| `0x38` | `ColorARGBInt` | `color_at_meter_maximum` |  |
| `0x3c` | `ColorARGBInt` | `flash_color` |  |
| `0x40` | `ColorARGBInt` | `empty_color` | drawn with its alpha inverted |
| `0x44` | `uint8_t` | `flags` | HUDInterfaceMeterFlags: bit 0 use min/max, bit 1 interpolate, bit 4 invert the interpolation |
| `0x45` | `uint8_t` | `minimum_meter_value` | lower bound of both computed alphas |
| `0x46` | `uint16_t` | `sequence_index` |  |
| `0x48` | `uint8_t` | `alpha_multiplier` | alpha = round(value * multiplier + bias) |
| `0x49` | `uint8_t` | `alpha_bias` |  |
| `0x4a` | `int16_t` | `value_scale` |  |
| `0x4c` | `float` | `opacity` |  |
| `0x50` | `float` | `translucency` |  |
| `0x54` | `ColorARGBInt` | `disabled_color` |  |
| `0x58` | `float` | `min_alpha` |  |

### `hud_number_placement` (size 0x48 as read)

hud_number_placement (hud_draw_number @0x4ac0b0 takes a pointer to one as its third stack argument) -- a HUD number element of types/tags.h viewed from its anchor_offset: WeaponHUDInterfaceNumber + 0x24, whose layout the unit and grenade numbers repeat.

| Offset | Type | Field | Notes |
|---|---|---|---|
| `0x00` | `Point2DInt` | `anchor_offset` | passed in EDX to 0x4ab690 |
| `0x04` | `float` | `width_scale` |  |
| `0x08` | `float` | `height_scale` |  |
| `0x0c` | `uint16_t` | `scaling_flags` | bit 2 halves the digit scale |
| `0x0e` | `uint8_t` | `pad_0e[0x16]` |  |
| `0x24` | `hud_flash_parameters` | `flash` | default color first |
| `0x3c` | `ColorARGBInt` | `disabled_color` |  |
| `0x40` | `uint8_t` | `pad_40[4]` |  |
| `0x44` | `int8_t` | `maximum_number_of_digits` |  |
| `0x45` | `uint8_t` | `flags` | HUDInterfaceNumberFlags: bit 0 show leading zeros, bit 1 only when zoomed, bit 2 trailing m (km over 999) |
| `0x46` | `int8_t` | `number_of_fractional_digits` | at most 4 are drawn |
| `0x47` | `uint8_t` | `pad_47` |  |

### `hud_element_placement` (size 0x0e)

hud_element_placement (EDX of hud_draw_bitmap_element @0x4acad0 and of hud_anchor_offset_to_screen_position @0x4ab690) The first 0x0e bytes of every HUD interface element of types/tags.h viewed from its anchor_offset field (static elements, meters and numbers at element +0x24, overlays and crosshair overlays at +0x00). hud_meter_placement, hud_number_placement and hud_static_element_placement all start with it.

| Offset | Type | Field | Notes |
|---|---|---|---|
| `0x00` | `Point2DInt` | `anchor_offset` |  |
| `0x04` | `float` | `width_scale` |  |
| `0x08` | `float` | `height_scale` |  |
| `0x0c` | `uint16_t` | `scaling_flags` | HUDInterfaceScalingFlags: bit 0 do not scale offset (clears the split screen flag of 0x4ab690), bit 2 use high res scale (halves the draw scale) |

### `hud_static_element_placement` (size 0x64 (tag element bytes 0x24 .. 0x88))

hud_static_element_placement (third stack argument of hud_draw_static_element @0x4ac6f0) A HUD static element of types/tags.h viewed from its anchor_offset field: WeaponHUDInterfaceStaticElement + 0x24, which the unit and grenade HUD statics repeat. Replaces the file-local hud_static_icon_element TYPES-GAP typedefs of the first rewrite.

| Offset | Type | Field | Notes |
|---|---|---|---|
| `0x00` | `Point2DInt` | `anchor_offset` |  |
| `0x04` | `float` | `width_scale` |  |
| `0x08` | `float` | `height_scale` |  |
| `0x0c` | `uint16_t` | `scaling_flags` | see hud_element_placement |
| `0x0e` | `uint8_t` | `pad_0e[0x16]` |  |
| `0x24` | `TagDependency` | `interface_bitmap` | tag_id at 0x30 |
| `0x34` | `hud_flash_parameters` | `flash` | default color first |
| `0x4c` | `ColorARGBInt` | `disabled_color` | drawn when draw flags bit 1 is set |
| `0x50` | `uint8_t` | `pad_50[4]` |  |
| `0x54` | `uint16_t` | `sequence_index` |  |
| `0x56` | `uint8_t` | `pad_56[2]` |  |
| `0x58` | `TagReflexive` | `multitexture_overlays` | HUDInterfaceMultitextureOverlay, stride 0x1e0 |

### `hud_overlay_list` (size 0x1c)

hud_overlay_list (second stack argument of hud_draw_overlays @0x4ac950) WeaponHUDInterfaceOverlayElement + 0x24: the overlay bitmap dependency followed by the overlays block (WeaponHUDInterfaceOverlay, stride 0x88; the grenade HUD overlay block has the same layout). Replaces the file-local hud_overlay_icon_list TYPES-GAP typedef.

| Offset | Type | Field | Notes |
|---|---|---|---|
| `0x00` | `TagDependency` | `overlay_bitmap` | tag_id at 0x0c |
| `0x10` | `TagReflexive` | `overlays` | count at 0x10, pointer at 0x14 |

### `level_select_entry` (size 0x08)

campaign level select list (ui_build_level_select_list @0x49c8f0, 0x49cc80, 0x49ce00, 0x4a4e20) level_select_entries at 0x00719018 is one array of 10 eight byte records: the rewrites first read it as two arrays (paths at 0x00719018, flags at 0x0071901c), which are the two halves of the same record. The builder copies the path from known_campaign_levels at 0x00692acc and the unlock bits from the saved profile. widget_instance::list_items points at the array.

| Offset | Type | Field | Notes |
|---|---|---|---|
| `0x00` | `char *` | `path` | from known_campaign_levels |
| `0x04` | `uint8_t` | `valid` | set to 1 once the level is unlocked |
| `0x05` | `uint8_t` | `flag_bit1` | profile unlock flags bit 1 |
| `0x06` | `uint8_t` | `flag_bit2` | bit 2 |
| `0x07` | `uint8_t` | `flag_bit3` | bit 3 |

### `campaign_level_entry` (size 0x08, table at 0x00692acc)

| Offset | Type | Field | Notes |
|---|---|---|---|
| `0x00` | `char *` | `path` | levels1010 and so on |
| `0x04` | `int32_t` | `unknown_04` |  |

### `profile_carousel_slot` (size 0x2000)

profile_carousel_slot (ui_profile_carousel_slot_cache_populate @0x4a74b0, player_profile_1wide_list_update @0x4a6380, ui_build_profile_list @0x49dd70) Three 0x2000 byte slots at 0x00873d60 (the scans stop at 0x00879d60); ui_build_profile_list fills all 0x1800 dwords with 0xff. player_profile_get @0x53a770 loads a saved profile into profile (ECX) when a slot is assigned. Offsets of the profile body as the 1-wide list reads them: +0x002 name (11 wide chars), +0x11a player color (clamped to 0..0x11), +0x11c flags (bit 0 default profile, high byte the default name index), +0x12c button set, +0x12d joystick set.

| Offset | Type | Field | Notes |
|---|---|---|---|
| `0x0000` | `int32_t` | `profile_id` | -1 when empty |
| `0x0004` | `uint8_t` | `profile[0x1ffc]` | the saved profile body |

### `variant_carousel_slot` (size 0x9c)

variant_carousel_slot (ui_variant_carousel_slot_cache_populate @0x4a7570) Three cached game variant headers at 0x00879d60, 0x9c byte stride; the body is filled by the saved game module (0x53bee0).

| Offset | Type | Field | Notes |
|---|---|---|---|
| `0x00` | `int32_t` | `id` |  |
| `0x04` | `uint8_t` | `unknown[0x98]` | filled by 0x53bee0 |

### `chat_incoming_record` (size 0x0c)

chat_incoming_record (chat_dispatch_incoming @0x4aaf70) The stack record the network decoder 0x4ec590 fills from a chat event.

| Offset | Type | Field | Notes |
|---|---|---|---|
| `0x00` | `int32_t` | `kind` | 0 all, 1 team, 2 vehicle, 3 server text, 4 localized string id |
| `0x04` | `uint8_t` | `player_index` | 0xff when the line has no sending player |
| `0x05` | `uint8_t` | `pad_05[3]` |  |
| `0x08` | `uint16_t *` | `text` | caller-provided 0x100 wide character buffer |

### `hud_meter_color_block` (size 0x1c)

hud_meter_color_block (built on the stack by hud_meter_draw_fill @0x4abbc0 and handed to the meter renderer 0x4acad0)

| Offset | Type | Field | Notes |
|---|---|---|---|
| `0x00` | `uint32_t` | `primary` | packed ARGB |
| `0x04` | `uint32_t` | `secondary` |  |
| `0x08` | `uint32_t` | `empty` | empty_color with its alpha inverted |
| `0x0c` | `uint32_t` | `tint` |  |
| `0x10` | `uint8_t` | `flag_10` | always 0 here |
| `0x11` | `uint8_t` | `flag_11` | always 1 here |
| `0x12` | `uint8_t` | `pad_12[2]` |  |
| `0x14` | `uint32_t` | `opacity` | packed {translucency, 1 - opacity three times} |
| `0x18` | `float` | `scale` | always 1.0 here |

## Phase-4 review of s1 (Opus): what changed

The two Sonnet rewrite passes compiled cleanly but carried behaviour errors, mostly register arguments that
Ghidra dropped. The review checked the following files line by line against objdump. Each fixed file
says so in its header.

| Area | Fix |
|---|---|
| `chimera__load_ui_widget` and its 7 callers | 7 stack arguments, not 5. Argument 3 is a parent widget, not a replace flag. The go-back record is built from arguments 5 to 7 plus the replaced root controller, not invented. `tag_lookup` takes its group in EDI. `widget_initialize_from_tag` now gets its three register arguments. |
| `widget_history_node` | +0x04 is a list child definition (`list_definition`) and +0x0a the controller word, not +0x04. `interface_tick` reopened with the wrong field. |
| `widget_create_children_from_tag` | the extended description is created with this widget as parent and controller (was 0, 0) |
| `widget_list_select_next` | list types other than 2 and 3 skip the `selection_index` store; focus helpers take register arguments |
| `ui_widget_list_item_activate` | a successful run_function no longer suppresses the other actions; the script expression index; the final sound is the action kind (was always 0) |
| `ui_sound_effect` enum | one-based: 0 none, 1 cursor, 2 forward, 3 back, 4 flag_failure |
| `widget_instance_handle_input_event` | the close-request table is `int32_t[4]` (`team_slot_table`), not `int16_t`; the out-of-range test was inverted (any slot set closes); the 0x40 list navigation also raises the sound to 1; four helper signatures |
| `ui_widget_draw_formatted_prompt_string`, `ui_widget_draw_prompt_span` | rewritten. The running state is a Rectangle2D, not `{color, x}` (`prompt_draw_state` removed from the header). The text comes in through EDX. The literal strings were being read as pointers. The quoted-name and `???` branches were swapped. The icon override table is the wrong one (0x006926f4). There is a second stack parameter. |
| `widget_instance_render_text_box` | rewritten: string index, `heap_reallocate` arguments, the search-and-replace calls (ASCII to wide into a 0x40 byte local, then a three-argument replace), the draw rect and clip rect, and the register arguments of the text calls. Also resolves the widget+0x44 "collision": a text box reuses 0x44..0x53 as a color override. |
| `widget_instance_render_list_head` | same string, search-and-replace and draw fixes; scroll arrow frames, source rect and clip |
| `widget_instance_render` | background quad: source = dest = offset tag bounds, clip = offset *dest, packed color |
| `ui_draw_screen_quad` and callers | the fifth argument is a packed ARGB vertex color; callers were converting the integer to float |
| `interface_draw_cursor` | Rectangle2D order (x and y + size were swapped); bitmap lookup arguments |
| `ui_error_modal_update` | restored the two split-screen divider fills that the first rewrite dropped |
| `interface_tick` | the pending message is the 6 byte record at 0x00718fac, forwarded in full (was 0x00718fa4 with constants) |
| `network_game_host_start` | params 1 and 2 are the map name (EDI) and variant name (ECX); the first rewrite said they were unused |
| `first_person_weapon_update_lighting` | node permute and render use weapon tag +0x468 and first person interface +0x0c, not +0x478; `first_person_light_parameters` moved to the header |
| `hud_play_pickup_notification` | object index passed to `object_try_and_get`; unified sound prototype |
| `console_draw_overlay`, `hud_text_draw_configure`, `interface_local_player_state_reset` | 0x006e4738 is the float text alpha; the int conversion corrupted it |
| `map_list_get_friendly_level_name` | the string index is `map_list[index].map_id` |
| `widget_close`, `ui_error_modal_update`, `player_profile_get_flag_by_id`, quit prompt users | globals that are pointers or have other widths: 0x006b145c, 0x006f1d6c, 0x006f187c are pointers; 0x0071c2d4/d8 are pointers; 0x0071973c is a byte; 0x00719754 is a word plus 0x00719757; 0x00719720 is `int16_t network_game_mode` |
| `ui_button_prompt_draw_icon` (renamed from `ui_button_prompt_queue_icon_sound`) | draws an animated icon frame; nothing here plays a sound |
| `console_printf.c` renamed to `chimera__console_out.c` | 30 files in other modules already call 0x496b50 by this name |

Header changes: `widget_history_node` relaid, `ui_sound_effect` made one-based, `prompt_draw_state` replaced by a
note, `first_person_light_parameters`, `ui_input_event` and three function-pointer typedefs added
(`ui_event_function`, `ui_game_data_input_function`, `ui_search_replace_function`; the local copies in
`widget_close.c`, `widget_instance_render.c`, `widget_instance_render_text_box.c` and
`ui_widget_list_item_activate.c` were deleted). The widget_instance text box overlay is documented. The
global list now describes 0x00718fac, 0x00692b18 and 0x00692c08 correctly.

Names: `symbols/agent_phase4_interface.txt` has one row per file (132 rows). Five of them are renames at
0.80: four Chimera signature labels replaced by descriptive names, plus 0x49ac80.

## Phase-4 review of s2 part 1 (Opus): what changed

Range `0x49bac0 .. 0x4ac0b0`, written by two Sonnet agents (146 files). The gate was clean on arrival,
so every fix below is a behaviour fix found by reading the function against objdump. Each fixed file
says so in its header.

| Area | Fix |
|---|---|
| `chat_dispatch_incoming` 0x4aaf70 | rewritten: Ghidra removed 21 blocks as unreachable and left only the gate. The real body formats player, team and vehicle chat lines (strings 0xbb and 0xbc of `ui\multiplayer_game_text`), server text and localized string ids into the chat listbox. `chimera__multiplayer_message` takes a wide string. |
| `hud_update_interaction_prompt` 0x4a9b80 (was `hud_weapon_message_state_update`) | rewritten from both jump tables: message index per interaction type, argument slots, the weapon HUD icon argument, the empty-weapon switch hint; every register argument restored |
| `object_get_hud_text_message_index` 0x4a9b40, `weapon_hud_ammo_state_is_empty` 0x4a9750 | renamed: +0x13c of an object tag is `Object::hud_text_message_index`; the record tested is `weapon_hud_ammo_state` |
| `hud_draw_number` 0x4ac0b0 (was `hud_draw_ammo_digit`) | rewritten: the function runs to 0x4ac6ce and draws a whole number (digits, fraction, decimal point, minus, trailing m or km) |
| `hud_meter_flash_color_blend`, `hud_meter_draw_fill` | rewritten against `hud_flash_parameters` and `hud_meter_placement`: fmod time base, branch targets, color sources, sprite rect and renderer arguments |
| `hud_waypoint_draw_all_for_player`, `hud_waypoint_draw_one` | self exclusion, teammate handle array and the per-call handle; float fade bounds; uv crop; five-argument quad call |
| `hud_update_player` | register arguments of 0x4afee0, 0x4b14c0 and `hud_messaging_update`; 0x006f1d6c is `game_time_globals` (also fixed in five s1 files) |
| `server_list_menu_update` | `FUN_00625430` is strstr (every map test was inverted); the tenth details widget; signed and unsigned compares; the `%d` literal; `FUN_004da770` takes the entry in ESI |
| `virtual_keyboard_*` | 0x00719410 is the dword `validation_mode`, not `name_is_valid`; sound ids; byte returns; open takes the destination in ESI; the DirectInput buffers are 0x6d bytes; the `.fortune` easter egg modulo is unsigned; int16 key ring indices (also in `console_process_queued_input`) |
| `ui_list_widget_rebuild_rows` | the row scale constant is 0.333 (0x3eaa7efa), not 1/3 (eight more files had it); highlight color call |
| `heap_reallocate` in 12 files | the old block (EAX) and the heap (ESI) were dropped at every call |
| `set_profile_name`, `FUN_0049bac0`, `player_profile_details_widget_refresh`, `widget_list_scroll_window`, `FUN_004a4e20` callers | the widget register argument was missing or wrong at 11 call sites |
| `FUN_0049c4c0` | tag controller 4 maps to `widget->controller_index` (+0x08, was +0x02); returns the new widget |
| `FUN_0049c620` | 0x006851fc is a pointer to a ColorARGB, only its alpha is used |
| `FUN_004a0050`, `audio_options_apply_from_profile`, `FUN_004a0b50`, `FUN_004a4a30` | the working copy at 0x00714e80 is a buffer, and the sbb/not/and select was read inverted in all four |
| `FUN_004a2ad0`, `FUN_004a1c30`, `FUN_004a29a0`, `FUN_004a47c0`, `FUN_004a4ee0` | rewritten: bodies Ghidra dropped (EBX count of 0x53c4e0, profile bytes read as uninitialized locals, an unrecovered jump table, register arguments) |
| `ui_profile_carousel_slot_cache_populate`, `player_profile_1wide_list_update` | the carousel is three 0x2000 byte `profile_carousel_slot` records (a dword stride was used); the 1-wide list loops back after compacting its ids and keeps the name at widget +0x50 |
| `ui_draw_screen_quad`, `ui_draw_rotated_screen_quad` (s1) | the 0x8c byte `ui_quad_render_state` is the EAX argument of 0x51c9a0 (its floats were at the wrong offsets); the rotated quad takes a fifth alpha argument |
| `player_profile_apply_audio_options` (s1) | the extern addresses of the music and effects gain setters were swapped (the calls were right) |
| `widget_focus_next_child` 0x49c080, `widget_focus_previous_child` 0x49c0f0 | named; their labels in `widget_instance_handle_input_event.c` were swapped |
| `map_list_find_index_by_path.c` | deleted: 0x49d850 is a mid-function fragment (see the misattributed table) |

Header changes: `virtual_keyboard_globals` +0x68 is the dword `validation_mode`; `widget_instance` documents the
list use of +0x3c/+0x3e; the 0x00718fac pending message is slot 0 of a per local player array. New types:
`hud_flash_parameters`, `hud_meter_placement`, `hud_number_placement`, `ui_quad_render_state`,
`level_select_entry`, `campaign_level_entry`, `profile_carousel_slot`, `variant_carousel_slot`,
`chat_incoming_record`, `hud_meter_color_block`, and the function pointer types of the embedded GUI chat
table, DirectInput SetProperty, the list row formatter, the network group name getters and the child anchor
handlers. Every TYPES-GAP typedef copy in the src files was folded in and deleted (the level select paths
and flags were one array read twice).

Names: `symbols/agent_phase4_interface.txt` gained 95 rows for s2 part 1; six are renames at 0.80
(0x4a9b80, 0x4a9b40, 0x4a9750, 0x4ac0b0, 0x49c080, 0x49c0f0). `symbols/functions.txt` has not been
regenerated (`python tools/merge_symbols.py`).

## Phase-4 review of s2 part 2 (Opus): what changed

Range `0x4ac6f0 .. 0x4c9c80`, written by two Sonnet agents (88 files). The review compared every file in the
range against objdump: 49 were rewritten, 39 were checked and either fixed or confirmed (all 88 now carry
a `phase-4 review` or `objdump` note). The cross-range and consistency passes below also touched
files in s1 and s2 part 1. Each fixed file says so in its header.

The defects were of the same kinds as in the earlier reviews:
- register arguments dropped at call sites (`heap_reallocate`, 0x43f290, 0x4ab8d0, `texture_cache_get`,
  the flash blend, the string list lookup, the D3D enumerator);
- pointer globals treated as arrays (0x873d40, 0x6f187c);
- jump tables modelled as function pointer tables;
- wrong struct offsets and wrong stack slots, including a loop that read the wrong list.

| Area | Fix |
|---|---|
| HUD element drawing 0x4ac6f0..0x4acfe0 | Eight functions were renamed and checked: `hud_draw_static_element`, `hud_draw_overlays`, `hud_draw_bitmap_element`, `hud_draw_bitmap_at`, `hud_bitmap_anchor_extents`, `hud_draw_rotated_bitmap_quad`, `hud_player_weapon_ammo_state` and `hud_draw_multitexture_overlay`. Their register arguments were restored: EAX uv, EDX placement or bitmap, BL/CL pixel uvs, ESI scale. `ui_quad_render_state` was relaid from the 0x51c9a0 consumers, and `hud_quad_vertex`, `hud_element_placement`, `hud_static_element_placement` and `hud_overlay_list` were added. The last two replace the TYPES-GAP `hud_static_icon_element` and `hud_overlay_icon_list`. |
| HUD messaging 0x4ad8e0..0x4ae550 | 20 functions were renamed and rewritten: text span and icon, help and objective text, the timer (set, pause, ticks, draw), player message and arguments, action text, and item messages (network message 6, slot find, compare). `hud_messaging_update` was rewritten in full; the no-line path keeps y. `chimera__hud_message` takes the local player in AX, and three callers were fixed. `hud_message_broadcast_to_local_players` takes the text in ESI. `hud_chat_to_network.c` was deleted (0x4add40 is inside `hud_timer_draw`). |
| HUD waypoints 0x4af070..0x4afb90 | Nine functions were renamed and checked. `hud_waypoint_draw` was rewritten. `hud_waypoint` has kind and visibility nibbles, a float vertical offset and the object. |
| Unit HUD 0x4afd30..0x4b16e0 | `hud_render_unit_interface` (4468 bytes) was rewritten in full. The sounds, meters and damage indicators were checked. `hud_unit_meter_state` is 0x58 bytes and is followed by the script flags dword in `hud_unit_meter_globals`. |
| Weapon HUD 0x4b1740..0x4b3420 | `hud_weapon_interface_state_update` was rewritten, and `hud_weapon_interface_meters_evaluate` got fixes in cases 0, 1, 6, 9, 18 and the accumulator. `hud_draw_weapon_interface`, `hud_weapon_interface_draw_elements` (seven cdecl arguments; the meter flags are state flags), `hud_draw_grenade_interface` and `hud_weapon_crosshairs_draw` were renamed from misleading names. The crosshair type range check is unsigned. `hud_weapon_interface_state` is now per-player 0x28 blocks, per-player 0x50 meter states and a crosshair flags dword. |
| Motion sensor 0x4b3450..0x4b4120 | Six functions were rewritten. `motion_sensor_blip_fill` takes EAX local player, ECX object and ESI blip. `motion_sensor_object_is_detected` has an EDI=NULL path. `motion_sensor_render` calls the camera type function once. The blip count is at 0x78 and the update time at 0x568. |
| Controls setup 0x4b43c0..0x4b53a0 | Seven functions were rewritten; every register argument had been dropped. Device labels use tag group `'ustr'`, not `'DeLa'`, and are terminated at name[0x104]. Two helpers were renamed because their old names read inverted: `controls_key_is_bindable` and `controls_action_column_is_bindable`. The second spinner in the device mode toggle is one child level shallower than the first rewrite had it, and 0x6953ec is `controls_selected_device`. |
| Gamepad assignment 0x4b5560..0x4b5b20 | Eight functions were renamed from a server history / favorites reading that no caller supports. The lists are the four gamepads assigned in the profile (+0x1108, labelled as devices 2..5 by the device label table) and up to eight connected ones (0x6b1868, stride 0x240). `server_browser_entry` became `controls_gamepad_record`. Five of the eight were rewritten: the node array read row i instead of row i + 1; ECX screen widgets were dropped; the built-in entries went to the wrong list with a 0x90 stride; `server_list_reset` collided with networking 0x4b65f0; the restore dropped the register arguments of the control profile calls; an index of -1 is not guarded in the binary. |
| Video modes 0x4bab50..0x4bb640 | The enumerator is IDirect3D9 (GetAdapterModeCount / EnumAdapterModes with the EBX format 0x16), walked from the last mode down. The memory filter had been inverted. `video_resolution_add` formats the name through EDX, and `name[15]` is the terminator; the old `selected_rate` field is gone. In `video_options_menu_populate` the fallback refresh index is looked up for 60 Hz and the refresh item count comes from entry 0. `video_options_menu_open` was renamed `video_options_reset_to_defaults`: it restores defaults, keeps the gamma, uses sound 2 and a 0x1ffc buffer. `video_options_menu_update` was rewritten: the spinners had been swapped, the gamma row was wrong and it plays sound 4. |
| `network_autojoin_from_command_line` 0x4c9c80 | Rewritten: `command_line_check_flag` output in EDI, the EBX in/out profile count, the name compare at profile +2, the default profile copy for slot -1, and the load arguments. |
| Cross-range, s1 and s2 part 1 | `ui_round_half_away_from_zero` was renamed `ui_real_to_int_truncate` (it is truncation toward zero). `ui_network_group_name_get` was renamed `ui_search_replace_function_call` (AX index, ECX widget). `player_index_from_unit_index` was renamed `player_get_vehicle` (ECX player; returns the unit parent when seated). `hud_meter_resolve_bitmap_frame` (EAX frame; it never clears the output), `virtual_keyboard_draw_text` (it takes the bounds rect) and `virtual_keyboard_render` were rewritten. Call sites were fixed in `ui_draw_trouble_brewing_indicator`, `multiplayer_settings_select_list_refresh_3wide`, `ui_variant_carousel_slot_cache_populate` (count on the stack), `hud_get_message_string` and `hud_display_checkpoint_message` (ECX/DX string lookup, sound parameters) and `ui_button_prompt_draw_icon`. |
| Consistency | The 33 `FUN_` (and one stale) extern names of functions defined in this module were renamed to the defining file name at every call site in src/interface. 20 globals now have one name and one type everywhere: `hud_globals_tag_data` (HUDGlobals*), `network_server`, `network_client`, `global_scenario`, `current_game_engine`, `cinematic_globals`, `object_data`, `saved_item_working_copy`, `current_local_player_index` (0x7c3108), the controls capture globals, the rasterizer and gamma globals, and others. Four byte-offset expressions that became struct pointer arithmetic were cast back to `uint8_t *`. Every in-module extern now has the same arity as its definition. Local typedefs (`d3d_display_mode`, `d3d9_interface*`, `input_guid`, `hud_sound_start_parameters`) were folded into the header. `hud_number_pen` stays local to `hud_draw_number.c`: it is a C-only helper, not a binary layout. |

Header changes:
- relaid: `ui_quad_render_state`, `hud_messaging_globals` (tail 0x460..0x487), `hud_player_messaging_state`, `hud_message_slot`, `hud_waypoint`, `hud_unit_meter_state`, `motion_sensor_frame`, `motion_sensor_globals`, `hud_weapon_interface_state` (split into `hud_weapon_interface_player` and `hud_weapon_meter_state`), `video_resolution`;
- renamed: `server_browser_entry` became `controls_gamepad_record`;
- `hud_globals_flags` byte 1 is `help_text_shown`;
- new: `hud_quad_vertex`, `hud_element_placement`, `hud_static_element_placement`, `hud_overlay_list`, `hud_messaging_information`, `hud_item_message`, `hud_unit_meter_globals`, `hud_sound_start_parameters`, `input_guid`, `d3d_display_mode`, `d3d9_interface_vtable`, `d3d9_interface`;
- removed: `network_group_name_function`;
- two apostrophes in comments were reworded for the Ghidra C parser.

Names: `symbols/agent_phase4_interface.txt` gained 91 rows. 57 are renames at 0.80, with the old name in the evidence; the other 34 cover second-range files that had no row. `symbols/functions.txt` has not been regenerated (`python tools/merge_symbols.py`).

## Known gaps

- **Unlisted functions** in the range (table above): 0x49d7c0, 0x4ae530, 0x4b52f0, 0x4b5350, 0x4b54a0, 0x4b54c0, 0x4bb290, 0x4bb2e0, 0x4bb300, 0x4bb360, 0x4bb3d0.
- **Callers outside this module still use `FUN_` names and old prototypes**; only src/interface was edited:
  - game: `FUN_004ae350` / `FUN_004ae400` (item messages), `FUN_004af070` / `FUN_004af540` (waypoint arrow and visibility), `FUN_004a9990`, `FUN_004aa2a0`, `FUN_004aade0`, `FUN_00498b20`, `FUN_00498330`, `FUN_00495a60`;
  - items and objects: `FUN_00492990`, `FUN_00494010`, `FUN_00492c30`, `FUN_004b16e0`;
  - units: `FUN_00492730`;
  - networking: `FUN_004ae200`, `FUN_004aaf70`, `FUN_004aabd0`, `FUN_0049c7b0` / `FUN_0049c810`;
  - their register arguments (EAX count / ECX source / DL kind of 0x4ae350, EDI name of 0x4af070, AX/ECX/EDX of 0x4af540, ECX player and stack damage of 0x4b16e0) need checking at each call site.
- **Earlier cross-module gaps still open**:
  - the 30 `chimera__console_out` externs in game, hs and networking omit the EAX color;
  - `color_interpolate` is declared without its ECX/EAX colors in ai, effects and objects;
  - `saved_game_enumerate_by_type` is declared without the EBX count in game.
- **Networking prototypes used by the chat files**: `message_delta_encode_message` and `network_session_send_to_machine` are declared here with seven stack arguments. The networking definitions lack the EAX/EDX and EAX/ESI register arguments that the call sites load. `bit_stream_write_bits_chunked`, `network_channel_stream_flush`, `network_object_owner_team_index_desired`, `message_delta_decode_compound_field`, `network_debug_fill_canary_buffer` and `network_player_entry_validate` also have a different arity from their definitions, in low-confidence chat and network files (`chat_queue_team_message`, `chat_server_relay_incoming_message`, `chimera__chat_out`, `FUN_0049d2e0`, `FUN_004a5ad0`).
- **Remaining extern conflicts inside the module**: 82 addresses are declared differently in different files. Most are string literals and anonymous globals named per file (0x660c34, 0x671fac, 0x6e4730..0x6e4738, 0x7c3140..0x7c3178, 0x71c2dc..0x71c2de), foreign callee parameter names, and a few real arity or type differences in low-confidence s1 and s2 part 1 files:
  - `widget_list_adjust_rect_for_scroll_arrows(void)` in `FUN_004a1ff0.c`;
  - `string_format_wide_va` without its EDX destination in `chat_queue_team_message.c`;
  - `text_string_list_get_string(void)` in `chimera__chat_open.c`, `multiplayer_settings_select_list_update_item.c` and `player_profile_details_widget_refresh.c`;
  - `object_try_and_get` and `saved_game_enumerate_by_type` without the EBX count in `FUN_004a2cb0.c`, `FUN_004a4650.c` and `ui_build_profile_list.c`;
  - `FUN_00557990(void)`;
  - `d3d_device` 0x71d174 is typed three ways (`void *`, `void **`, `uint32_t`);
  - the capture buffer 0x712544 is 0x280 bytes in one file and 0x290 in another.
- **Named elsewhere, used otherwise here**:
  - `types/objects.h` calls object +0xb8 `name_index`, but the waypoint, motion sensor and damage indicator code use it as the owner team;
  - 0x7461a0 is `network_predicted_globals` elsewhere but `hud_unit_sounds_play` uses it as the looping sound datum array;
  - 0x4635e0 has a name that does not match its use as the motion sensor gate;
  - 0x6b1844 is the connected gamepad count (`input_gamepad_count`) here.
- `hud_anchor_offset_to_screen_position` models its ECX child placement as a selector (0.3). `hud_weapon_interface_meters_evaluate` (0.6) was fixed case by case, not rewritten.

- **Files still below 0.45 rewrite confidence**: 75, all in s1 and s2 part 1. They were not re-verified in this review; expect the dropped register arguments and inverted selects found in the files that were: `first_person_weapon_update_state`, `first_person_weapon_set_state`, `first_person_weapon_snapshot_pose`, `first_person_weapon_interface_initialize`, `hud_meter_permute_node_records`, `hud_meter_find_matching_elements`, `first_person_weapon_process_action`, `first_person_weapon_interface_tick_reset`, `ui_draw_rotated_screen_quad`, `map_list_get_friendly_level_name`, `map_list_add_entry`, `player_profile_apply_video_options`, `player_profile_get_flag_by_id`, `saved_item_select`, `hud_message_broadcast_to_local_players`, `player_profile_save_495fb0`, `interface_tick`, `widget_draw_split_screen_region`, `ui_draw_screen_quad`, `widget_instance_find_root`, `interface_handle_quit_request`, `widget_instance_is_input_eligible`, `FUN_0049bac0`, `FUN_0049bba0`, `widget_instance_select_list_index`, `FUN_0049bdd0`, `ui_string_replace_all`, `ui_check_for_pause_game`, `set_profile_name`, `ui_build_level_select_list`, `FUN_0049cc80`, `FUN_0049ce00`, `FUN_0049cfd0`, `multiplayer_host_session_start`, `FUN_0049d2e0`, `ui_build_profile_list`, `FUN_0049dfc0`, `FUN_0049e090`, `FUN_004a0050`, `FUN_004a1110`, `FUN_004a19a0`, `FUN_004a1ff0`, `FUN_004a2070`, `FUN_004a20f0`, `FUN_004a2270`, `FUN_004a22e0`, `audio_options_apply_from_profile`, `FUN_004a2cb0`, `restart_map_dialog_choice_handler`, `FUN_004a3180`, `FUN_004a3960`, `FUN_004a3b30`, `hud_text_message_queue_update_and_draw`, `FUN_004a4650`, `autopatch_status_widget_update`, `FUN_004a49c0`, `FUN_004a4b60`, `FUN_004a4d30`, `FUN_004a4f60`, `FUN_004a5ad0`, `multiplayer_settings_select_list_update_item`, `player_profile_details_widget_refresh`, `FUN_004a62d0`, `ui_profile_carousel_fetch_sensitivity`, `ui_audio_options_apply_volume_sliders`, `ui_game_variant_flag_list_widget_build`, `hud_update_dispatch`, `ui_chat_window_reset_position`, `chimera__chat_open`, `chimera__chat_out`, `chat_server_relay_incoming_message`, `chat_queue_team_message`, `chat_default_team_channel`, `hud_chat_listbox_update`, `hud_anchor_offset_to_screen_position`.

## Open questions for hook verification

Top five from the s2 part 2 review:

1. **`ui_quad_render_state` (EAX of 0x51c9a0)**. Break in 0x51c9a0 from `hud_draw_static_element` (one map) and from `hud_draw_multitexture_overlay` (three maps), and dump the 0x8c bytes. Check:
   - `meter_parameters` at 0x00 and `geometry_offset` at 0x04;
   - the map pointers 0x0c..0x14, wrap modes 0x18, offset pointers 0x1c, scales 0x28, texel scales 0x40, tint pointers 0x58 and fade pointers 0x78;
   - the blend words 0x84..0x88 and the `single_local_player` byte 0x8a.
   Only the multitexture path has been seen writing all three maps.
2. **HUD messaging element bytes**. Hook `hud_messaging_update` (0x4ae550, AX local player) while a pickup message with an icon argument and a help text with a string argument are on screen. Log the argument slots (`argument_is_string` bits at +0x459), the per-element data byte that selects text or icon, and the `prompt_changed` / `message_shown_copy` pair at +0x45e/+0x45f.
3. **`hud_weapon_interface_state` split (0x719430, 0x7c bytes)**. Watch the block while switching weapons, firing to empty, reloading and throwing a grenade. Confirm:
   - flash start times at +0x00..+0x1c;
   - the cached weapon at +0x20 and the grenade flash at +0x24;
   - 19 meter values from +0x28 and the active mask at +0x74;
   - flags bit 0 (+0x78): toggle `show_hud_crosshair` through the script function at 0x480c4e and check that the crosshairs disappear.
4. **Object +0xb8 as the owner team**. In a team game, log `*(int16 *)(object + 0xb8)` for friendly and enemy bipeds, vehicles and a flag. Run it as the waypoint (`hud_waypoints_update_for_player`), motion sensor (`blip_type_get`) and damage indicator code reads it, and compare with `player.team`. `types/objects.h` calls the field `name_index`.
5. **Gamepad records**. With two gamepads connected, open controls setup, assign one, and dump:
   - the profile at 0x714e80 + 0x1108 (four 0x220 records);
   - the connected table at 0x6b1868 (stride 0x240, count int16 at 0x6b1844).
   Confirm the wide device name at +0 and that the 0x14 byte key at +0x20c is the DirectInput instance GUID plus one dword. Then check that `controls_gamepad_toggle_assignment` (0x4b5b20) moves a record between the two lists.

Also open, lower priority:
- the binding record layout of 0x53aa20 / 0x53ae10 ({int16 kind, index, subtype, control} as `controls_binding_row_handle_input` reads it);
- whether 0x7c3108 (`current_local_player_index`, passed by `hud_timer_draw`) is really the rendering local player index;
- whether 0x4bb3d0 is the video options apply handler.

### Carried over from the s1 and s2 part 1 reviews

1. `hud_update_interaction_prompt` (0x4a9b80): hook 0x4adfc0 and log EAX (message index) against
   `player::interaction_type` while walking up to a weapon, a vehicle seat and a teammate vehicle. This
   pins the eleven cases and the slot 0 / slot 1 argument order.
2. `hud_draw_number` (0x4ac0b0): log the value, fraction and flags arguments for the ammo counter and for a
   navpoint distance. Check that a value above 999 with the trailing m flag draws glyph 0xe plus a fraction,
   and that the anchor jump table only ever sees 0..4.
3. `virtual_keyboard.validation_mode` (0x00719410): the binary writes 0..5 after each open (static
   stores at 0x495d1e, 0x4a2c72, 0x4a2c9f, 0x4a3aba, 0x4a3b0c, 0x4a4b1f, 0x4b67a7). Break on those while
   editing a profile name, a variant name, the server address and the port, and check which field each
   mode belongs to: 1..3 are commit rules, 3..5 character filters in 0x4a8b80.
4. `ui_quad_render_state` (EAX of 0x51c9a0): dump the 0x8c bytes for one widget background quad and one
   waypoint arrow. The rewrite only knows the bitmap at +0x0c, the four 1.0 floats and `mode` (0 or 7).
5. The list widget overlay at `widget_instance` +0x3c/+0x3e: break on writes for a 3-wide list and a text
   box on the same screen, and confirm the text pointer and the two words never meet in one widget.

## Functions and rewrite confidence

`Name conf` is the confidence in the symbol name and `Rewrite conf` the confidence in the C rewrite, both from the file header. `UNSURE` and `TYPES-GAP` count the markers in the file (the Ghidra block is not counted). `Phase-4 check` is `rewritten` when the review rewrote the body from objdump, `checked` when it was compared against objdump and fixed or confirmed, blank when the file was not re-verified.

| Address | Function | Bytes | Name conf | Rewrite conf | UNSURE | TYPES-GAP | Phase-4 check |
|---|---|---|---|---|---|---|---|
| `0x44c290` | `widget_text_edit_process_key` | 785 | 0.35 | 0.55 | 2 | 0 |  |
| `0x44c5b0` | `widget_text_edit_reset_length` | 36 | 0.3 | 0.7 | 0 | 0 |  |
| `0x44c5e0` | `widget_text_edit_get_selection` | 85 | 0.35 | 0.7 | 0 | 0 |  |
| `0x44c640` | `widget_text_edit_insert_string` | 310 | 0.35 | 0.65 | 1 | 0 |  |
| `0x44c780` | `widget_text_edit_clamp_selection` | 123 | 0.35 | 0.7 | 1 | 0 |  |
| `0x4923d0` | `first_person_weapon_interface_tick` | 96 | 0.4 | 0.55 | 0 | 0 |  |
| `0x492430` | `first_person_weapon_update_active_state` | 123 | 0.4 | 0.5 | 0 | 0 |  |
| `0x4924b0` | `first_person_weapon_update_lighting` | 568 | 0.3 | 0.55 | 2 | 0 | checked |
| `0x4926f0` | `local_player_index_for_object` | 62 | 0.4 | 0.7 | 0 | 0 |  |
| `0x492730` | `weapon_action_notify_for_unit` | 82 | 0.35 | 0.55 | 1 | 0 |  |
| `0x492790` | `weapon_action_notify_for_weapon` | 38 | 0.35 | 0.6 | 0 | 0 |  |
| `0x4927c0` | `item_type_to_message_stage` | 109 | 0.3 | 0.6 | 0 | 0 |  |
| `0x492880` | `item_type_to_animation_stage` | 164 | 0.3 | 0.6 | 0 | 0 |  |
| `0x492990` | `hud_play_pickup_notification` | 320 | 0.3 | 0.7 | 2 | 2 | checked |
| `0x492ad0` | `first_person_weapon_get_marker_data` | 176 | 0.6 | 0.6 | 1 | 0 |  |
| `0x492b80` | `first_person_weapon_center_flashlight` | 172 | 0.75 | 0.6 | 0 | 0 |  |
| `0x492c30` | `unit_get_first_person_marker_transform` | 225 | 0.35 | 0.5 | 0 | 0 |  |
| `0x492d20` | `first_person_weapon_update_state` | 269 | 0.45 | 0.35 | 4 | 0 |  |
| `0x492e60` | `first_person_weapon_set_state` | 524 | 0.55 | 0.3 | 4 | 1 |  |
| `0x4930b0` | `first_person_weapon_snapshot_pose` | 160 | 0.4 | 0.4 | 1 | 1 |  |
| `0x493150` | `first_person_weapon_update` | 1510 | 0.5 | 0.6 | 1 | 0 |  |
| `0x493740` | `first_person_weapon_update_animation_controls` | 1299 | 0.5 | 0.6 | 2 | 0 |  |
| `0x493c60` | `first_person_weapon_interface_initialize` | 487 | 0.5 | 0.25 | 9 | 1 |  |
| `0x493e50` | `first_person_weapon_set_attached` | 77 | 0.6 | 0.8 | 0 | 0 |  |
| `0x493ea0` | `hud_meter_permute_node_records` | 84 | 0.3 | 0.35 | 0 | 1 |  |
| `0x493f00` | `hud_meter_find_matching_elements` | 258 | 0.35 | 0.2 | 2 | 1 |  |
| `0x494010` | `local_player_index_for_weapon` | 135 | 0.4 | 0.55 | 0 | 0 |  |
| `0x4940a0` | `local_player_index_for_unit` | 72 | 0.4 | 0.55 | 0 | 0 |  |
| `0x4940f0` | `first_person_weapon_process_action` | 453 | 0.5 | 0.25 | 4 | 1 |  |
| `0x4942e0` | `first_person_weapon_interface_tick_reset` | 90 | 0.35 | 0.3 | 2 | 0 |  |
| `0x494340` | `interface_globals_allocate` | 73 | 0.5 | 0.6 | 0 | 0 |  |
| `0x494390` | `interface_local_player_state_reset` | 145 | 0.5 | 0.6 | 0 | 0 |  |
| `0x494430` | `globals_color_table_get_cyclic_color` | 140 | 0.35 | 0.55 | 0 | 0 |  |
| `0x4944c0` | `hud_text_draw_configure` | 145 | 0.3 | 0.45 | 1 | 0 |  |
| `0x494560` | `local_player_get_weapon_hud_interface` | 457 | 0.7 | 0.65 | 1 | 0 |  |
| `0x494730` | `first_person_weapon_update_screen_effects` | 946 | 0.4 | 0.65 | 5 | 0 |  |
| `0x494af0` | `first_person_weapon_update_zoom_static_tint` | 425 | 0.3 | 0.65 | 2 | 0 |  |
| `0x494ca0` | `ui_error_modal_update` | 207 | 0.3 | 0.75 | 2 | 1 | checked |
| `0x494d70` | `ui_draw_rotated_screen_quad` | 475 | 0.3 | 0.35 | 1 | 1 | checked |
| `0x494f50` | `map_list_get_friendly_level_name` | 145 | 0.75 | 0.4 | 1 | 0 | checked |
| `0x494ff0` | `map_list_find_known_map_index` | 195 | 0.5 | 0.45 | 3 | 0 |  |
| `0x4950c0` | `map_list_add_entry` | 208 | 0.55 | 0.4 | 4 | 0 |  |
| `0x495260` | `map_list_free_all` | 89 | 0.6 | 0.85 | 0 | 0 |  |
| `0x4952c0` | `player_profile_auto_select` | 174 | 0.4 | 0.65 | 1 | 0 |  |
| `0x495370` | `player_profile_subsystem_initialize` | 372 | 0.4 | 0.6 | 7 | 0 |  |
| `0x4954f0` | `player_profile_find_index_by_id` | 33 | 0.5 | 0.6 | 0 | 0 |  |
| `0x495520` | `network_game_setup_teardown` | 87 | 0.35 | 0.6 | 0 | 0 |  |
| `0x495580` | `player_profile_apply_video_options` | 584 | 0.4 | 0.3 | 27 | 0 |  |
| `0x4957d0` | `player_profile_apply_audio_options` | 409 | 0.4 | 0.45 | 6 | 0 |  |
| `0x495970` | `player_profile_load` | 238 | 0.55 | 0.65 | 5 | 0 |  |
| `0x495a60` | `player_profile_get_flag_by_id` | 84 | 0.35 | 0.4 | 2 | 0 |  |
| `0x495ac0` | `network_game_host_start` | 288 | 0.55 | 0.6 | 12 | 0 | checked |
| `0x495be0` | `saved_item_select` | 169 | 0.4 | 0.3 | 2 | 0 |  |
| `0x495c90` | `saved_item_name_changed` | 82 | 0.35 | 0.55 | 1 | 0 |  |
| `0x495cf0` | `saved_item_name_edit_begin` | 77 | 0.35 | 0.75 | 1 | 0 | checked |
| `0x495d40` | `player_profile_save` | 290 | 0.7 | 0.55 | 4 | 0 |  |
| `0x495e70` | `saved_item_name_matches` | 38 | 0.35 | 0.5 | 1 | 0 |  |
| `0x495ea0` | `saved_item_has_unsaved_changes` | 163 | 0.4 | 0.6 | 1 | 0 |  |
| `0x495f50` | `hud_message_broadcast_to_local_players` | 86 | 0.35 | 0.4 | 0 | 0 | checked |
| `0x495fb0` | `player_profile_save_495fb0` | 171 | 0.5 | 0.25 | 2 | 0 | checked |
| `0x496060` | `player_profile_refresh_settings_cache` | 878 | 0.4 | 0.6 | 1 | 0 |  |
| `0x4963d0` | `terminal_initialize` | 80 | 0.9 | 0.85 | 0 | 0 |  |
| `0x496420` | `console_message_new` | 110 | 0.7 | 0.55 | 1 | 0 |  |
| `0x496490` | `console_message_delete` | 117 | 0.7 | 0.6 | 0 | 0 |  |
| `0x496510` | `console_open` | 100 | 0.7 | 0.55 | 1 | 0 |  |
| `0x496580` | `console_close` | 82 | 0.7 | 0.7 | 0 | 0 |  |
| `0x4965e0` | `console_process_queued_input` | 248 | 0.7 | 0.45 | 6 | 0 |  |
| `0x4966e0` | `console_message_expire_old` | 79 | 0.55 | 0.75 | 0 | 0 |  |
| `0x496730` | `console_draw_overlay` | 835 | 0.7 | 0.45 | 5 | 0 | checked |
| `0x496a80` | `console_printf_verbose` | 206 | 0.4 | 0.5 | 0 | 0 |  |
| `0x496b50` | `chimera__console_out` | 193 | 0.55 | 0.55 | 0 | 0 |  |
| `0x496c20` | `console_restore_cursor` | 94 | 0.45 | 0.6 | 0 | 0 |  |
| `0x496c80` | `console_process_input_events` | 178 | 0.6 | 0.7 | 0 | 0 |  |
| `0x496d40` | `console_update_display` | 167 | 0.6 | 0.7 | 0 | 0 |  |
| `0x496df0` | `string_replace_all_in_place` | 158 | 0.4 | 0.55 | 1 | 0 |  |
| `0x496e90` | `chimera__console_out_copy` | 242 | 0.7 | 0.55 | 5 | 0 |  |
| `0x496f90` | `console_clear_screen` | 124 | 0.7 | 0.8 | 0 | 1 |  |
| `0x497010` | `console_clear_bottom_line` | 140 | 0.7 | 0.8 | 0 | 0 |  |
| `0x4970a0` | `console_draw_input_line` | 254 | 0.7 | 0.55 | 1 | 1 |  |
| `0x4971a0` | `console_position_cursor` | 164 | 0.7 | 0.6 | 0 | 1 |  |
| `0x497250` | `interface_update_for_resolution_change` | 105 | 0.5 | 0.7 | 0 | 0 |  |
| `0x4972c0` | `ui_cursor_update` | 189 | 0.5 | 0.45 | 10 | 0 | checked |
| `0x497380` | `interface_draw_cursor` | 132 | 0.6 | 0.85 | 0 | 0 | checked |
| `0x497410` | `chimera__do_show_loading_screen` | 224 | 0.5 | 0.6 | 7 | 0 |  |
| `0x4978a0` | `interface_loading_screen_set_text` | 34 | 0.5 | 0.45 | 1 | 0 | checked |
| `0x4978d0` | `interface_loading_screen_reset` | 46 | 0.6 | 0.85 | 1 | 0 |  |
| `0x497900` | `color_pack_argb_from_real` | 99 | 0.8 | 0.9 | 0 | 0 |  |
| `0x497970` | `color_argb_scale_alpha` | 52 | 0.55 | 0.7 | 0 | 0 |  |
| `0x4979b0` | `widget_memory_pool_initialize` | 181 | 0.8 | 0.65 | 2 | 0 |  |
| `0x497a70` | `chimera__load_ui_widget` | 365 | 0.7 | 0.8 | 0 | 0 | checked |
| `0x497c00` | `widget_close` | 627 | 0.6 | 0.45 | 4 | 0 | checked |
| `0x497e80` | `interface_tick` | 736 | 0.8 | 0.4 | 11 | 2 | checked |
| `0x498330` | `widget_draw_split_screen_region` | 379 | 0.35 | 0.4 | 5 | 1 |  |
| `0x4984c0` | `widget_draw_fullscreen_region` | 368 | 0.4 | 0.5 | 2 | 2 |  |
| `0x498630` | `widget_list_get_child_by_index` | 29 | 0.5 | 0.6 | 0 | 0 |  |
| `0x498650` | `widget_close_all` | 88 | 0.5 | 0.6 | 4 | 0 |  |
| `0x4986b0` | `widget_list_select_next` | 366 | 0.55 | 0.7 | 1 | 0 | checked |
| `0x498820` | `widget_list_select_previous` | 449 | 0.55 | 0.7 | 1 | 0 | checked |
| `0x4989f0` | `chimera__load_main_menu` | 184 | 0.7 | 0.5 | 6 | 3 | checked |
| `0x498ab0` | `main_menu_on_shown` | 111 | 0.55 | 0.55 | 2 | 0 |  |
| `0x498b20` | `ui_draw_screen_quad` | 737 | 0.35 | 0.3 | 0 | 2 | checked |
| `0x498e10` | `widget_instance_find_root` | 17 | 0.4 | 0.4 | 1 | 0 |  |
| `0x498e30` | `widget_get_sibling_index` | 35 | 0.55 | 0.55 | 0 | 0 |  |
| `0x498e60` | `widget_instance_set_state_recursive` | 40 | 0.5 | 0.6 | 0 | 0 |  |
| `0x498e90` | `widget_play_sound_effect` | 114 | 0.65 | 0.6 | 2 | 0 |  |
| `0x498f20` | `display_error` | 559 | 0.9 | 0.45 | 3 | 3 | checked |
| `0x499170` | `interface_handle_quit_request` | 125 | 0.5 | 0.4 | 5 | 1 |  |
| `0x4991f0` | `player_help_screen_select_by_name` | 496 | 0.5 | 0.5 | 3 | 2 |  |
| `0x4993e0` | `main_menu_play_title_music` | 66 | 0.55 | 0.6 | 3 | 0 |  |
| `0x499430` | `list_node_prepend` | 46 | 0.6 | 0.5 | 1 | 0 |  |
| `0x499460` | `list_node_pop` | 68 | 0.6 | 0.45 | 1 | 0 |  |
| `0x4994b0` | `widget_pool_list_free_all` | 129 | 0.55 | 0.45 | 0 | 0 |  |
| `0x499540` | `widget_create_children_from_tag` | 569 | 0.55 | 0.45 | 1 | 0 | checked |
| `0x499780` | `widget_initialize_from_tag` | 456 | 0.6 | 0.45 | 3 | 0 | checked |
| `0x499950` | `widget_find_by_tag_id` | 64 | 0.5 | 0.65 | 0 | 0 |  |
| `0x499990` | `widget_list_adjust_rect_for_scroll_arrows` | 95 | 0.45 | 0.55 | 0 | 0 |  |
| `0x4999f0` | `widget_instance_point_in_bounds` | 161 | 0.5 | 0.6 | 0 | 0 |  |
| `0x499aa0` | `widget_instance_verify_stack_chain` | 33 | 0.3 | 0.55 | 1 | 0 |  |
| `0x499ad0` | `widget_instance_find_at_point` | 331 | 0.55 | 0.45 | 0 | 0 |  |
| `0x499c20` | `widget_instance_get_cumulative_scale` | 27 | 0.6 | 0.75 | 0 | 0 |  |
| `0x499c40` | `widget_instance_is_input_eligible` | 109 | 0.35 | 0.35 | 1 | 0 |  |
| `0x499cb0` | `widget_instance_is_top_of_stack` | 69 | 0.5 | 0.55 | 0 | 0 |  |
| `0x499d00` | `widget_instance_handle_input_event` | 1504 | 0.5 | 0.6 | 3 | 1 | checked |
| `0x49a430` | `ui_widget_list_item_activate` | 1148 | 0.5 | 0.65 | 4 | 0 | checked |
| `0x49a8c0` | `widget_instance_render` | 320 | 0.6 | 0.45 | 5 | 0 | checked |
| `0x49ac30` | `ui_button_prompt_index_from_string` | 66 | 0.5 | 0.65 | 1 | 0 |  |
| `0x49ac80` | `ui_button_prompt_draw_icon` | 163 | 0.5 | 0.75 | 1 | 0 | rewritten |
| `0x49ad30` | `ui_widget_draw_prompt_span` | 106 | 0.35 | 0.8 | 0 | 0 |  |
| `0x49ada0` | `ui_string_has_button_prompt_token` | 50 | 0.5 | 0.55 | 0 | 0 |  |
| `0x49ade0` | `ui_widget_draw_formatted_prompt_string` | 983 | 0.5 | 0.7 | 1 | 0 | rewritten |
| `0x49b1d0` | `widget_instance_render_text_box` | 845 | 0.8 | 0.7 | 1 | 0 | rewritten |
| `0x49b560` | `widget_instance_render_list_head` | 1372 | 0.5 | 0.45 | 2 | 1 | checked |
| `0x49bac0` | `FUN_0049bac0` | 153 | 0.3 | 0.4 | 2 | 0 |  |
| `0x49bb60` | `FUN_0049bb60` | 55 | 0.4 | 0.55 | 1 | 0 |  |
| `0x49bba0` | `FUN_0049bba0` | 324 | 0.35 | 0.4 | 1 | 0 |  |
| `0x49bd00` | `widget_instance_select_list_index` | 200 | 0.6 | 0.4 | 1 | 0 |  |
| `0x49bdd0` | `FUN_0049bdd0` | 59 | 0.35 | 0.4 | 3 | 0 |  |
| `0x49be10` | `ui_string_replace_all` | 489 | 0.55 | 0.4 | 1 | 1 |  |
| `0x49c000` | `FUN_0049c000` | 63 | 0.35 | 0.55 | 0 | 0 |  |
| `0x49c040` | `FUN_0049c040` | 55 | 0.35 | 0.55 | 0 | 0 |  |
| `0x49c080` | `widget_focus_next_child` | 112 | 0.6 | 0.7 | 0 | 0 | checked |
| `0x49c0f0` | `widget_focus_previous_child` | 168 | 0.6 | 0.7 | 0 | 0 | checked |
| `0x49c1a0` | `ui_check_for_pause_game` | 489 | 0.8 | 0.35 | 1 | 5 |  |
| `0x49c3e0` | `widget_instance_close_and_restore_previous` | 214 | 0.55 | 0.55 | 1 | 0 |  |
| `0x49c4c0` | `FUN_0049c4c0` | 202 | 0.35 | 0.75 | 0 | 0 | checked |
| `0x49c5c0` | `FUN_0049c5c0` | 82 | 0.35 | 0.8 | 1 | 0 |  |
| `0x49c620` | `FUN_0049c620` | 93 | 0.3 | 0.9 | 0 | 0 | checked |
| `0x49c680` | `FUN_0049c680` | 136 | 0.35 | 0.45 | 2 | 2 |  |
| `0x49c710` | `set_profile_name` | 149 | 0.8 | 0.4 | 2 | 0 |  |
| `0x49c7b0` | `ui_network_wait_timeout_check` | 86 | 0.45 | 0.6 | 0 | 0 |  |
| `0x49c810` | `ui_network_wait_timeout_start` | 81 | 0.45 | 0.55 | 0 | 0 |  |
| `0x49c870` | `ui_draw_trouble_brewing_indicator` | 128 | 0.5 | 0.85 | 2 | 2 | checked |
| `0x49c8f0` | `ui_build_level_select_list` | 892 | 0.75 | 0.2 | 5 | 14 |  |
| `0x49cc80` | `FUN_0049cc80` | 318 | 0.4 | 0.25 | 2 | 5 |  |
| `0x49ce00` | `FUN_0049ce00` | 409 | 0.3 | 0.2 | 4 | 7 |  |
| `0x49cfd0` | `FUN_0049cfd0` | 248 | 0.3 | 0.2 | 2 | 9 |  |
| `0x49d210` | `multiplayer_host_session_start` | 201 | 0.55 | 0.35 | 5 | 2 |  |
| `0x49d2e0` | `FUN_0049d2e0` | 351 | 0.3 | 0.2 | 5 | 1 |  |
| `0x49dd70` | `ui_build_profile_list` | 492 | 0.7 | 0.3 | 2 | 1 |  |
| `0x49df70` | `FUN_0049df70` | 74 | 0.4 | 0.5 | 0 | 0 |  |
| `0x49dfc0` | `FUN_0049dfc0` | 206 | 0.35 | 0.25 | 3 | 0 |  |
| `0x49e090` | `FUN_0049e090` | 212 | 0.35 | 0.25 | 2 | 0 |  |
| `0x4a0050` | `FUN_004a0050` | 492 | 0.3 | 0.3 | 3 | 2 |  |
| `0x4a0a30` | `FUN_004a0a30` | 68 | 0.35 | 0.55 | 0 | 0 |  |
| `0x4a0b50` | `FUN_004a0b50` | 109 | 0.35 | 0.7 | 0 | 1 |  |
| `0x4a1110` | `FUN_004a1110` | 109 | 0.35 | 0.4 | 1 | 1 |  |
| `0x4a1670` | `FUN_004a1670` | 38 | 0.4 | 0.55 | 1 | 0 |  |
| `0x4a1940` | `FUN_004a1940` | 91 | 0.35 | 0.7 | 1 | 2 |  |
| `0x4a19a0` | `FUN_004a19a0` | 342 | 0.35 | 0.25 | 5 | 2 |  |
| `0x4a1c30` | `FUN_004a1c30` | 80 | 0.3 | 0.8 | 0 | 0 | rewritten |
| `0x4a1ff0` | `FUN_004a1ff0` | 126 | 0.4 | 0.4 | 2 | 0 |  |
| `0x4a2070` | `FUN_004a2070` | 116 | 0.3 | 0.35 | 3 | 0 |  |
| `0x4a20f0` | `FUN_004a20f0` | 160 | 0.3 | 0.35 | 0 | 0 |  |
| `0x4a2270` | `FUN_004a2270` | 99 | 0.3 | 0.4 | 1 | 0 |  |
| `0x4a22e0` | `FUN_004a22e0` | 424 | 0.3 | 0.2 | 1 | 3 |  |
| `0x4a26a0` | `audio_options_apply_from_profile` | 688 | 0.5 | 0.4 | 2 | 0 |  |
| `0x4a29a0` | `FUN_004a29a0` | 96 | 0.25 | 0.8 | 0 | 0 | rewritten |
| `0x4a2ad0` | `FUN_004a2ad0` | 375 | 0.2 | 0.75 | 1 | 0 | checked |
| `0x4a2cb0` | `FUN_004a2cb0` | 605 | 0.3 | 0.15 | 3 | 4 |  |
| `0x4a30e0` | `restart_map_dialog_choice_handler` | 103 | 0.5 | 0.4 | 4 | 0 |  |
| `0x4a3180` | `FUN_004a3180` | 544 | 0.3 | 0.25 | 2 | 0 |  |
| `0x4a3960` | `FUN_004a3960` | 89 | 0.3 | 0.4 | 0 | 2 |  |
| `0x4a3b30` | `FUN_004a3b30` | 56 | 0.3 | 0.35 | 2 | 0 |  |
| `0x4a3ce0` | `hud_text_message_queue_init` | 96 | 0.5 | 0.7 | 0 | 0 |  |
| `0x4a3d90` | `hud_text_message_queue_add` | 145 | 0.55 | 0.45 | 1 | 0 |  |
| `0x4a3e30` | `hud_text_message_queue_update_and_draw` | 716 | 0.5 | 0.15 | 3 | 7 |  |
| `0x4a4650` | `FUN_004a4650` | 338 | 0.3 | 0.15 | 2 | 1 |  |
| `0x4a47c0` | `FUN_004a47c0` | 73 | 0.3 | 0.7 | 1 | 0 | rewritten |
| `0x4a4880` | `autopatch_status_widget_update` | 288 | 0.6 | 0.2 | 3 | 4 |  |
| `0x4a49c0` | `FUN_004a49c0` | 99 | 0.3 | 0.25 | 1 | 2 |  |
| `0x4a4a30` | `FUN_004a4a30` | 177 | 0.3 | 0.75 | 0 | 0 | checked |
| `0x4a4b60` | `FUN_004a4b60` | 272 | 0.3 | 0.2 | 1 | 0 |  |
| `0x4a4cb0` | `FUN_004a4cb0` | 60 | 0.3 | 0.5 | 0 | 0 |  |
| `0x4a4cf0` | `FUN_004a4cf0` | 60 | 0.3 | 0.5 | 0 | 0 |  |
| `0x4a4d30` | `FUN_004a4d30` | 204 | 0.3 | 0.2 | 1 | 0 |  |
| `0x4a4e20` | `FUN_004a4e20` | 185 | 0.3 | 0.7 | 0 | 0 |  |
| `0x4a4ee0` | `FUN_004a4ee0` | 115 | 0.3 | 0.75 | 0 | 0 | rewritten |
| `0x4a4f60` | `FUN_004a4f60` | 126 | 0.3 | 0.3 | 1 | 0 |  |
| `0x4a4fe0` | `FUN_004a4fe0` | 89 | 0.3 | 0.55 | 0 | 1 |  |
| `0x4a5040` | `server_list_menu_update` | 1724 | 0.7 | 0.55 | 1 | 1 | checked |
| `0x4a5ad0` | `FUN_004a5ad0` | 239 | 0.25 | 0.15 | 3 | 1 |  |
| `0x4a5bc0` | `multiplayer_settings_select_list_update_item` | 1018 | 0.85 | 0.15 | 4 | 0 |  |
| `0x4a5ff0` | `multiplayer_settings_select_list_refresh_3wide` | 263 | 0.5 | 0.85 | 0 | 0 | rewritten |
| `0x4a6100` | `player_profile_details_widget_refresh` | 434 | 0.5 | 0.15 | 4 | 0 |  |
| `0x4a62d0` | `FUN_004a62d0` | 143 | 0.3 | 0.3 | 1 | 0 |  |
| `0x4a6380` | `player_profile_1wide_list_update` | 802 | 0.85 | 0.65 | 1 | 0 | rewritten |
| `0x4a66b0` | `widget_extended_description_sync_selection` | 182 | 0.3 | 0.55 | 1 | 0 |  |
| `0x4a6810` | `ui_selection_list_mirror_value_build` | 105 | 0.3 | 0.55 | 4 | 0 |  |
| `0x4a6940` | `ui_map_list_carousel_refresh_window` | 160 | 0.3 | 0.5 | 1 | 0 |  |
| `0x4a69f0` | `ui_profile_carousel_fetch_name` | 107 | 0.3 | 0.55 | 0 | 0 |  |
| `0x4a6b00` | `ui_profile_carousel_fetch_sensitivity` | 107 | 0.3 | 0.4 | 3 | 0 |  |
| `0x4a7360` | `ui_widget_sync_profile_status_flag` | 106 | 0.3 | 0.55 | 1 | 0 |  |
| `0x4a7400` | `widget_list_scroll_window` | 163 | 0.45 | 0.7 | 0 | 0 |  |
| `0x4a74b0` | `ui_profile_carousel_slot_cache_populate` | 185 | 0.35 | 0.75 | 1 | 0 | checked |
| `0x4a7570` | `ui_variant_carousel_slot_cache_populate` | 189 | 0.35 | 0.8 | 3 | 0 | checked |
| `0x4a7630` | `ui_carousel_slot_compare_valid_first` | 38 | 0.35 | 0.8 | 0 | 0 |  |
| `0x4a76d0` | `ui_audio_options_apply_volume_sliders` | 426 | 0.35 | 0.4 | 3 | 0 |  |
| `0x4a7b20` | `ui_list_free_all` | 116 | 0.45 | 0.6 | 1 | 0 |  |
| `0x4a7ba0` | `ui_list_add_entry` | 172 | 0.45 | 0.55 | 2 | 0 |  |
| `0x4a7c50` | `ui_list_get_data` | 40 | 0.45 | 0.75 | 0 | 0 |  |
| `0x4a7c80` | `ui_list_get_id` | 41 | 0.45 | 0.75 | 0 | 0 |  |
| `0x4a7cb0` | `ui_list_find_default` | 67 | 0.45 | 0.75 | 0 | 0 |  |
| `0x4a7d00` | `ui_list_widget_compute_scroll_start` | 174 | 0.35 | 0.5 | 3 | 0 |  |
| `0x4a7db0` | `ui_list_widget_rebuild_rows` | 1365 | 0.35 | 0.7 | 1 | 0 | checked |
| `0x4a8360` | `ui_profile_details_list_widget_build` | 102 | 0.3 | 0.7 | 3 | 0 |  |
| `0x4a83d0` | `ui_list_item_format_name_and_cache_flag` | 111 | 0.35 | 0.5 | 1 | 0 |  |
| `0x4a8440` | `ui_network_adapter_list_widget_build` | 134 | 0.3 | 0.55 | 2 | 0 |  |
| `0x4a84d0` | `ui_game_variant_list_widget_build` | 140 | 0.3 | 0.55 | 3 | 0 |  |
| `0x4a8560` | `ui_game_variant_flag_list_widget_build` | 140 | 0.3 | 0.4 | 4 | 0 |  |
| `0x4a85f0` | `player_profile_select_list_widget_build` | 307 | 0.55 | 0.55 | 3 | 0 |  |
| `0x4a8730` | `ui_search_replace_function_call` | 32 | 0.6 | 0.95 | 0 | 0 | rewritten |
| `0x4a8790` | `registry_get_product_id` | 115 | 0.55 | 0.85 | 0 | 0 |  |
| `0x4a88f0` | `virtual_keyboard_initialize` | 175 | 0.9 | 0.85 | 0 | 0 |  |
| `0x4a89a0` | `virtual_keyboard_open` | 362 | 0.55 | 0.75 | 8 | 0 |  |
| `0x4a8b10` | `ui_wide_string_has_non_whitespace` | 55 | 0.35 | 0.65 | 0 | 0 |  |
| `0x4a8b50` | `ui_variant_name_is_available` | 37 | 0.45 | 0.85 | 0 | 0 | rewritten |
| `0x4a8b80` | `virtual_keyboard_character_is_legal` | 87 | 0.4 | 0.85 | 3 | 0 |  |
| `0x4a8be0` | `virtual_keyboard_process_input` | 1498 | 0.5 | 0.65 | 1 | 0 | checked |
| `0x4a9250` | `virtual_keyboard_close` | 175 | 0.55 | 0.8 | 2 | 0 |  |
| `0x4a9300` | `virtual_keyboard_draw_text` | 521 | 0.6 | 0.8 | 1 | 0 | rewritten |
| `0x4a9510` | `virtual_keyboard_render` | 472 | 0.6 | 0.8 | 0 | 0 | rewritten |
| `0x4a96f0` | `virtual_keyboard_backspace` | 88 | 0.6 | 0.85 | 0 | 0 |  |
| `0x4a9750` | `weapon_hud_ammo_state_is_empty` | 39 | 0.6 | 0.9 | 0 | 0 | checked |
| `0x4a9780` | `hud_state_allocate` | 330 | 0.5 | 0.6 | 0 | 0 |  |
| `0x4a98d0` | `hud_state_reset` | 178 | 0.5 | 0.55 | 2 | 0 |  |
| `0x4a9990` | `hud_update_dispatch` | 93 | 0.3 | 0.35 | 3 | 0 | checked |
| `0x4a99f0` | `hud_update_player` | 321 | 0.5 | 0.6 | 7 | 0 | checked |
| `0x4a9b40` | `object_get_hud_text_message_index` | 58 | 0.7 | 0.9 | 0 | 0 | checked |
| `0x4a9b80` | `hud_update_interaction_prompt` | 1632 | 0.5 | 0.6 | 1 | 0 | rewritten |
| `0x4aa2a0` | `hud_display_loading_message` | 101 | 0.35 | 0.5 | 0 | 0 | checked |
| `0x4aa310` | `hud_display_checkpoint_message` | 219 | 0.35 | 0.85 | 2 | 0 | checked |
| `0x4aa3f0` | `hud_get_message_string` | 67 | 0.5 | 0.9 | 1 | 0 | checked |
| `0x4aa440` | `hud_waypoint_draw_one` | 419 | 0.45 | 0.7 | 3 | 0 | rewritten |
| `0x4aa5f0` | `hud_waypoint_draw_all_for_player` | 179 | 0.45 | 0.85 | 0 | 0 | rewritten |
| `0x4aa6b0` | `ui_chat_window_reset_position` | 80 | 0.3 | 0.4 | 9 | 0 |  |
| `0x4aa700` | `chimera__chat_open` | 512 | 0.6 | 0.35 | 8 | 0 |  |
| `0x4aa900` | `chat_close` | 169 | 0.5 | 0.5 | 1 | 0 |  |
| `0x4aa9b0` | `chat_submit_input` | 210 | 0.5 | 0.5 | 1 | 0 |  |
| `0x4aaa90` | `chat_poll_hotkeys` | 102 | 0.3 | 0.45 | 5 | 0 |  |
| `0x4aab00` | `chimera__chat_out` | 197 | 0.55 | 0.15 | 4 | 0 |  |
| `0x4aabd0` | `chat_server_relay_incoming_message` | 512 | 0.3 | 0.15 | 11 | 0 |  |
| `0x4aade0` | `chat_queue_team_message` | 398 | 0.3 | 0.15 | 8 | 0 |  |
| `0x4aaf70` | `chat_dispatch_incoming` | 502 | 0.45 | 0.6 | 2 | 0 | rewritten |
| `0x4ab170` | `player_get_vehicle` | 105 | 0.6 | 0.9 | 0 | 0 | rewritten |
| `0x4ab1e0` | `chat_default_team_channel` | 87 | 0.35 | 0.3 | 3 | 0 |  |
| `0x4ab240` | `hud_chat_listbox_remove_oldest` | 177 | 0.5 | 0.45 | 0 | 0 |  |
| `0x4ab300` | `hud_chat_listbox_update` | 252 | 0.5 | 0.4 | 4 | 0 |  |
| `0x4ab400` | `hud_chat_listbox_clear` | 172 | 0.55 | 0.5 | 0 | 0 |  |
| `0x4ab4b0` | `chimera__multiplayer_message` | 214 | 0.55 | 0.6 | 1 | 0 |  |
| `0x4ab590` | `ui_real_to_int_truncate` | 56 | 0.7 | 0.9 | 0 | 0 | rewritten |
| `0x4ab5d0` | `color_rgb_float_to_int` | 90 | 0.55 | 0.85 | 0 | 0 |  |
| `0x4ab630` | `bitmap_group_sequence_get_bitmap_offset` | 83 | 0.5 | 0.5 | 1 | 0 |  |
| `0x4ab690` | `hud_anchor_offset_to_screen_position` | 315 | 0.35 | 0.3 | 3 | 0 |  |
| `0x4ab8d0` | `hud_meter_resolve_bitmap_frame` | 166 | 0.35 | 0.85 | 0 | 0 | rewritten |
| `0x4ab980` | `hud_meter_flash_color_blend` | 573 | 0.45 | 0.75 | 1 | 0 | rewritten |
| `0x4abbc0` | `hud_meter_draw_fill` | 1254 | 0.4 | 0.65 | 0 | 0 | rewritten |
| `0x4ac0b0` | `hud_draw_number` | 402 | 0.55 | 0.6 | 1 | 0 | rewritten |
| `0x4ac6f0` | `hud_draw_static_element` | 595 | 0.6 | 0.8 | 0 | 1 | rewritten |
| `0x4ac950` | `hud_draw_overlays` | 372 | 0.55 | 0.8 | 0 | 0 | rewritten |
| `0x4acad0` | `hud_draw_bitmap_element` | 216 | 0.6 | 0.85 | 0 | 0 | rewritten |
| `0x4acbb0` | `hud_draw_bitmap_at` | 152 | 0.55 | 0.85 | 0 | 0 | rewritten |
| `0x4acc50` | `hud_bitmap_anchor_extents` | 87 | 0.6 | 0.85 | 0 | 0 | rewritten |
| `0x4acd50` | `hud_draw_rotated_bitmap_quad` | 410 | 0.6 | 0.85 | 0 | 0 | rewritten |
| `0x4acef0` | `hud_player_weapon_ammo_state` | 226 | 0.55 | 0.85 | 0 | 0 | rewritten |
| `0x4acfe0` | `hud_draw_multitexture_overlay` | 2224 | 0.7 | 0.75 | 0 | 0 | rewritten |
| `0x4ad8e0` | `hud_draw_message_text_span` | 144 | 0.55 | 0.85 | 0 | 0 | checked |
| `0x4ad970` | `hud_draw_message_icon` | 447 | 0.55 | 0.85 | 0 | 0 | checked |
| `0x4adb30` | `hud_set_help_text` | 73 | 0.6 | 0.9 | 0 | 0 | checked |
| `0x4adb80` | `hud_set_objective_text` | 106 | 0.6 | 0.9 | 0 | 0 | checked |
| `0x4adbf0` | `hud_set_timer_time` | 101 | 0.6 | 0.9 | 0 | 0 | checked |
| `0x4adc60` | `hud_pause_timer` | 85 | 0.55 | 0.9 | 0 | 0 | checked |
| `0x4adcc0` | `hud_timer_get_ticks` | 65 | 0.6 | 0.9 | 0 | 0 | checked |
| `0x4add10` | `hud_timer_draw` | 56 | 0.7 | 0.8 | 0 | 0 | rewritten |
| `0x4adfc0` | `hud_set_player_message` | 140 | 0.55 | 0.85 | 0 | 0 | checked |
| `0x4ae050` | `hud_set_message_icon_argument` | 88 | 0.55 | 0.9 | 0 | 0 | checked |
| `0x4ae0b0` | `hud_set_message_string_argument` | 91 | 0.55 | 0.9 | 0 | 0 | checked |
| `0x4ae110` | `hud_set_action_text_shown` | 102 | 0.5 | 0.9 | 0 | 0 | checked |
| `0x4ae180` | `chimera__hud_message` | 126 | 0.65 | 0.6 | 1 | 0 | checked |
| `0x4ae200` | `hud_receive_item_message` | 332 | 0.5 | 0.75 | 0 | 0 | rewritten |
| `0x4ae350` | `hud_post_item_message` | 166 | 0.5 | 0.8 | 1 | 0 | rewritten |
| `0x4ae400` | `hud_add_item_message` | 127 | 0.55 | 0.9 | 0 | 0 | checked |
| `0x4ae480` | `hud_message_find_slot` | 128 | 0.6 | 0.9 | 0 | 0 | checked |
| `0x4ae500` | `hud_message_compare` | 45 | 0.65 | 0.95 | 0 | 0 | checked |
| `0x4ae550` | `hud_messaging_update` | 2826 | 0.65 | 0.75 | 5 | 0 | rewritten |
| `0x4af070` | `hud_waypoint_arrow_find` | 94 | 0.6 | 0.9 | 0 | 0 | checked |
| `0x4af0d0` | `hud_waypoint_activate_for_player` | 206 | 0.55 | 0.9 | 0 | 0 | checked |
| `0x4af1b0` | `hud_waypoint_activate_for_team` | 122 | 0.55 | 0.9 | 0 | 0 | rewritten |
| `0x4af230` | `hud_waypoint_deactivate_for_player` | 123 | 0.55 | 0.9 | 0 | 0 | checked |
| `0x4af2b0` | `hud_waypoint_deactivate_for_team` | 112 | 0.55 | 0.9 | 0 | 0 | rewritten |
| `0x4af320` | `hud_waypoints_update` | 75 | 0.55 | 0.9 | 0 | 0 | checked |
| `0x4af370` | `hud_waypoints_update_for_player` | 462 | 0.55 | 0.8 | 0 | 0 | rewritten |
| `0x4af540` | `hud_waypoint_visibility` | 151 | 0.55 | 0.85 | 0 | 0 | rewritten |
| `0x4af5e0` | `hud_waypoint_draw` | 1452 | 0.5 | 0.75 | 4 | 0 | rewritten |
| `0x4afb90` | `hud_waypoints_draw_for_player` | 397 | 0.55 | 0.85 | 0 | 0 | rewritten |
| `0x4afd30` | `hud_unit_sounds_play` | 422 | 0.55 | 0.8 | 1 | 0 | rewritten |
| `0x4afee0` | `hud_unit_sounds_update` | 557 | 0.55 | 0.8 | 2 | 0 | rewritten |
| `0x4b0110` | `hud_unit_meters_update` | 71 | 0.55 | 0.9 | 0 | 0 | checked |
| `0x4b0160` | `hud_unit_meters_update_for_player` | 442 | 0.55 | 0.85 | 1 | 0 | rewritten |
| `0x4b0320` | `hud_render_unit_interface` | 4468 | 0.85 | 0.7 | 1 | 0 | rewritten |
| `0x4b14c0` | `hud_draw_damage_indicators` | 520 | 0.55 | 0.65 | 2 | 0 | checked |
| `0x4b16e0` | `hud_unit_meter_apply_predictive_damage` | 95 | 0.5 | 0.9 | 1 | 0 | checked |
| `0x4b1740` | `hud_weapon_interface_state_update` | 557 | 0.5 | 0.85 | 0 | 0 | rewritten |
| `0x4b1970` | `hud_weapon_interface_meters_evaluate` | 373 | 0.5 | 0.6 | 4 | 0 | checked |
| `0x4b1e20` | `hud_draw_weapon_interface` | 453 | 0.6 | 0.85 | 0 | 0 | rewritten |
| `0x4b1ff0` | `hud_weapon_interface_draw_elements` | 2747 | 0.6 | 0.75 | 0 | 0 | rewritten |
| `0x4b2ac0` | `hud_draw_grenade_interface` | 552 | 0.6 | 0.85 | 0 | 0 | rewritten |
| `0x4b2cf0` | `hud_weapon_crosshairs_draw` | 1841 | 0.6 | 0.75 | 0 | 0 | rewritten |
| `0x4b3450` | `blip_type_get` | 415 | 0.6 | 0.85 | 1 | 0 | rewritten |
| `0x4b35f0` | `motion_sensor_blip_fill` | 110 | 0.5 | 0.9 | 0 | 0 | checked |
| `0x4b3660` | `motion_sensor_reset` | 57 | 0.55 | 0.85 | 0 | 0 | checked |
| `0x4b36a0` | `motion_sensor_object_is_detected` | 244 | 0.55 | 0.8 | 4 | 0 | checked |
| `0x4b37a0` | `motion_sensor_plot_blip` | 370 | 0.5 | 0.8 | 0 | 0 | rewritten |
| `0x4b3920` | `chimera__motion_sensor_update` | 1238 | 0.55 | 0.75 | 0 | 0 | rewritten |
| `0x4b3e10` | `motion_sensor_update_for_player` | 773 | 0.5 | 0.8 | 0 | 0 | rewritten |
| `0x4b4120` | `motion_sensor_render` | 663 | 0.5 | 0.8 | 1 | 0 | rewritten |
| `0x4b43c0` | `controls_key_is_bindable` | 26 | 0.5 | 0.95 | 1 | 0 | checked |
| `0x4b43e0` | `controls_enumerate_next_assignable_action` | 214 | 0.45 | 0.8 | 0 | 0 | rewritten |
| `0x4b44c0` | `controls_action_display_name` | 90 | 0.5 | 0.85 | 0 | 0 | rewritten |
| `0x4b4520` | `controls_binding_row_widget_update` | 622 | 0.4 | 0.75 | 0 | 0 | rewritten |
| `0x4b4790` | `controls_binding_list_refresh_rows` | 148 | 0.4 | 0.85 | 0 | 0 | rewritten |
| `0x4b4830` | `controls_device_label_add` | 92 | 0.6 | 0.7 | 0 | 0 | checked |
| `0x4b4890` | `controls_build_device_label_table` | 227 | 0.5 | 0.85 | 4 | 0 | checked |
| `0x4b4c50` | `controls_apply_preset` | 414 | 0.4 | 0.75 | 1 | 0 | rewritten |
| `0x4b4df0` | `controls_action_column_is_bindable` | 41 | 0.5 | 0.95 | 0 | 0 | checked |
| `0x4b4e20` | `controls_binding_clear` | 259 | 0.45 | 0.8 | 0 | 0 | rewritten |
| `0x4b4f30` | `controls_binding_row_handle_input` | 943 | 0.4 | 0.75 | 3 | 0 | rewritten |
| `0x4b53a0` | `controls_binding_rows_toggle_device_mode` | 243 | 0.4 | 0.85 | 2 | 0 | rewritten |
| `0x4b5560` | `controls_gamepad_widget_nodes_collect` | 107 | 0.45 | 0.9 | 0 | 0 | checked |
| `0x4b55d0` | `controls_gamepad_lists_refresh` | 394 | 0.45 | 0.85 | 0 | 0 | rewritten |
| `0x4b5760` | `controls_gamepad_list_find` | 146 | 0.6 | 0.85 | 0 | 0 | checked |
| `0x4b5800` | `controls_gamepad_list_add` | 80 | 0.6 | 0.85 | 0 | 0 | checked |
| `0x4b5850` | `controls_gamepad_list_remove` | 127 | 0.6 | 0.85 | 0 | 0 | checked |
| `0x4b58d0` | `controls_gamepad_lists_load` | 405 | 0.35 | 0.8 | 3 | 0 | rewritten |
| `0x4b5a70` | `controls_gamepad_bindings_restore` | 170 | 0.4 | 0.75 | 0 | 0 | rewritten |
| `0x4b5b20` | `controls_gamepad_toggle_assignment` | 447 | 0.45 | 0.8 | 0 | 0 | rewritten |
| `0x4bab50` | `video_resolution_compare` | 48 | 0.6 | 0.9 | 0 | 0 | checked |
| `0x4bab80` | `video_refresh_rate_compare` | 30 | 0.6 | 0.9 | 0 | 0 | checked |
| `0x4baba0` | `video_display_modes_enumerate` | 414 | 0.55 | 0.8 | 4 | 0 | rewritten |
| `0x4bad40` | `video_resolution_list_build` | 117 | 0.6 | 0.9 | 0 | 0 | checked |
| `0x4badc0` | `video_resolution_add` | 189 | 0.85 | 0.9 | 0 | 0 | rewritten |
| `0x4bae80` | `video_refresh_rate_find_index` | 59 | 0.5 | 0.75 | 0 | 0 | checked |
| `0x4baec0` | `video_options_menu_populate` | 975 | 0.4 | 0.75 | 10 | 0 | checked |
| `0x4bb5e0` | `video_options_reset_to_defaults` | 92 | 0.45 | 0.85 | 1 | 0 | rewritten |
| `0x4bb640` | `video_options_menu_update` | 410 | 0.75 | 0.85 | 0 | 0 | rewritten |
| `0x4c9c80` | `network_autojoin_from_command_line` | 334 | 0.5 | 0.8 | 1 | 0 | rewritten |
