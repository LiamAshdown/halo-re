# `shell` — the Win32 host around the engine

Retail Halo PC `halo.exe` 1.0.10, `0x5402a0 .. 0x57f52c` (the function list also carries five
bogus `.bss` entries at `0x6bd180 .. 0x6bd1b8`), plain C / MSVC 7.1 (cl 13.10.3077, LTCG) / x86.
Every `.c` file here is one function, rewritten against `types/shell.h` from its Ghidra
decompile and, where the decompile was wrong or incomplete, from `objdump -d` of `bin/halo.exe`.
Each file keeps the original Ghidra output at the bottom inside `#if 0 ... #endif`.

Gate: `python tools/build_check.py shell` gives **65 ok, 0 failed** (65 files).

Of the 115 entries in `out/phase4/shell_functions.md`, 65 are real module functions and all 65
are written. The other 50 are library code, catch funclets or not functions (see
[Misattributed entries](#misattributed-entries)).

## What the module contains

| Family | Range | What it is |
|---|---|---|
| start-up / teardown | `0x5402a0`–`0x541a1f` | CPUID decoder (`cpu_get_type`), `engine_initialize_subsystems` / `engine_shutdown_subsystems`, `shell_winmain` (EULA, command line, DLL loading, requirement checks, main loop call) |
| Win32 plumbing | `0x541a20`–`0x542f9f` | message pump, clipboard, argv builder and `-flag` lookup, OS family, NT write-access probe, Keystone UI DLL loader, single-instance mutexes, dialog centering |
| crash reporter | `0x542fa0`–`0x543a2a` | the `__except` filter of `shell_winmain`: "Gathering Exception Data" window, `dxdiag.exe` run, Dr. Watson (`dw15.exe`) shared-memory hand-off, `ReportFault` fallback |
| hardware requirements parser | `0x578410`–`0x57b51f` | a C++ component (vtable `0x006721e8`, MSVC 7.1 `std::string` / `std::vector` / `std::map` members) that reads the config.txt requirements script: tokenizer, `if` conditions over D3DCAPS9 / adapter / sound / OS values, `vendor` / `audiovendor` / `propertyset` blocks |
| config.txt and hardware detection | `0x57cfe0`–`0x57d87f`, `0x57d880`–`0x57e10d` | property setters, `shell_parse_config_txt`, memory / rdtsc CPU clock / DirectDraw video memory / DxDiag sound device detection |
| strings, errors, product id | `0x57e110`–`0x57f52c` | language-aware string-table loader, previous-run crash flag, fatal error dialog, localized string init, CryptoAPI SHA-1, DigitalProductID string |

Control flow at start-up:

```
shell_winmain
  shell_init_localization_strings        strings.dll, LangID, fallback texts
  game_single_instance_check(0)
  __try {                                 filter: exception_filter_crash_reporter
    stack guard byte, EULA (eula.dll EBUEula), command_line_parse_to_argv, -flags
    shell_detect_hardware_specs           memory, CPU MHz, adapters, sound devices
    Direct3DCreate9 -> shell_parse_config_txt(0, d3d)
        hwreq_parser_create -> vtable parse 0x579bd0 -> find_requirements_section / scan_* /
        parse_block -> parse_flag_assignment / d3dcaps_field_resolve -> evaluate_condition
    dsound / dinput8 / shfolder, shell_check_previous_run_crash, requirement checks
    shell_build_product_id_string, keystone_library_load
    engine_initialize_subsystems -> network_session_host_start_info_set -> main_loop 0x4c7610
    engine_shutdown_subsystems
  }
  shell_registry_set_exit_flag_clean, release the instance mutex
```

## Struct layouts

All live in `types/shell.h` (`#pragma pack(push,1)`). Offsets are bytes from the struct base.
Pointer fields inside structs are `uint32_t` with the pointee in the comment.

### `hwreq_parser` — size `0x6b8` (operator new in `hwreq_parser_create` 0x57b4c0)

| Off | Type | Field |
|---|---|---|
| `0x000` | `uint32_t` | `vtable` (`hwreq_parser_vtable *`, 0x006721e8) |
| `0x004` | `uint32_t` | `file_buffer` (`char *`, file size + 0x10, CR stored at the end) |
| `0x008` | `uint32_t` | `cursor` (`char *`) |
| `0x00c` | `uint32_t` | `end` (`char *`) |
| `0x010` | `uint32_t` | `line_start` (`char *`, error context) |
| `0x014` | `int32_t` | `line_number` |
| `0x018` | `uint32_t` | `flags` (`hwreq_property_set *`, flags applied to this machine) |
| `0x01c` | `uint32_t` | `requirements` (`hwreq_property_set *`, the Requirements section) |
| `0x020` | `uint8_t` | `error_reported` |
| `0x021` | `uint8_t[3]` | `unknown_21` |
| `0x024` | `msvc_std_string` | `error_message` |
| `0x040` | `msvc_std_string` | `graphics_device_name` |
| `0x05c` | `msvc_std_string` | `graphics_vendor_name` |
| `0x078` | `msvc_std_string` | `sound_device_name` |
| `0x094` | `msvc_std_string` | `sound_vendor_name` |
| `0x0b0` | `uint32_t` | `cpu_speed` (MHz, "cpuspeed") |
| `0x0b4` | `uint32_t` | `memory` (MB, "ram") |
| `0x0b8` | `uint32_t` | `video_memory` (bytes, "videoram") |
| `0x0bc` | `d3d_adapter_identifier9` | `adapter` |
| `0x508` | `d3d_caps9` | `caps` (types/rasterizer.h) |
| `0x638` | `shell_sound_device` | `sound_device` |
| `0x6a0` | `msvc_std_map` | `property_sets` (name to set, owns the sets) |
| `0x6ac` | `msvc_std_map` | `graphic_detail_sets` (OverallGraphicDetail value to set) |

### `hwreq_parser_vtable` — size `0x40` (0x006721e8, every slot `__thiscall`)

| Off | Target | Slot |
|---|---|---|
| `0x00` | 0x579bd0 | `parse(name, sound_device, adapter, caps, memory, video_memory, cpu_speed)`, ret 0x1c |
| `0x04` | 0x5786a0 | `hwreq_parser_scalar_deleting_destructor` |
| `0x08` | 0x578860 | `get_flags` |
| `0x0c` | 0x5788f0 | `hwreq_parser_find_property_set(name)` |
| `0x10` / `0x14` / `0x18` | 0x5786c0 / 0x5786f0 / 0x578740 | flag count / name / value |
| `0x1c` / `0x20` / `0x24` | 0x578790 / 0x5787c0 / 0x578810 | requirement count / name / value |
| `0x28` / `0x2c` | 0x578870 / 0x578880 | graphics device / vendor name |
| `0x30` / `0x34` | 0x578890 / 0x5788b0 | sound device / vendor name |
| `0x38` / `0x3c` | 0x5788d0 / 0x5788e0 | `has_error` (AL) / `get_error_message` (no argument) |

### `hwreq_property_set` — size `0x14`

| Off | Type | Field |
|---|---|---|
| `0x00` | `msvc_std_vector` | `flags` (`hwreq_string_pair` elements, stride 0x38) |
| `0x10` | `uint32_t` | `owner` (`hwreq_parser *`) |

### `hwreq_string_pair` — size `0x38`

| Off | Type | Field |
|---|---|---|
| `0x00` | `msvc_std_string` | `first` (flag name, compared with `_stricmp`) |
| `0x1c` | `msvc_std_string` | `second` (value) |

### `hwreq_map_node` — size `0x30`

| Off | Type | Field |
|---|---|---|
| `0x00` / `0x04` / `0x08` | `uint32_t` | `left` / `parent` / `right` |
| `0x0c` | `msvc_std_string` | `key` |
| `0x28` | `uint32_t` | `value` (`hwreq_property_set *`) |
| `0x2c` | `uint8_t` | `color` (1 black) |
| `0x2d` | `uint8_t` | `is_nil` (1 on the head sentinel) |

### `msvc_std_string` — size `0x1c` (MSVC 7.1 `std::basic_string<char>`)

| Off | Type | Field |
|---|---|---|
| `0x00` | `uint8_t` | allocator |
| `0x04` | union | `inline_buffer[16]` while capacity is 0xf, else `heap_buffer` |
| `0x14` | `uint32_t` | `size` |
| `0x18` | `uint32_t` | `capacity` |

### `msvc_std_vector` — size `0x10`, `msvc_std_map` — size `0x0c`

| Off | vector | map |
|---|---|---|
| `0x00` | allocator | comparator / allocator |
| `0x04` | `first` | `head` (`hwreq_map_node *` sentinel) |
| `0x08` | `last` | `size` |
| `0x0c` | `end` | |

### `shell_sound_device` — size `0x68` (DxDiag sound device record, 10 at 0x006ef9b0)

| Off | Type | Field |
|---|---|---|
| `0x00` | `char[0x20]` | `description` (szDescription, 31 characters) |
| `0x20` | `uint8_t[0x20]` | `unknown_20` |
| `0x40` | `uint32_t[4]` | `guid` (szGuidDeviceID; compared with DSDEVID_DefaultPlayback) |
| `0x50` | `large_integer` | `driver_version` (low `c<<16\|d`, high `a<<16\|b`) |
| `0x58` / `0x5c` / `0x60` / `0x64` | `uint32_t` | `vendor_id` / `device_id` / `subsystem_id` / `revision` (from szHardwareID) |

### `shell_display_adapter` — size `0x34` (10 at 0x006efdc0)

| Off | Type | Field |
|---|---|---|
| `0x00` | `uint32_t[4]` | `guid` (zero for entry 0) |
| `0x10` | `char[0x20]` | `driver_name` |
| `0x30` | `uint32_t` | `video_memory` (bytes, rounded to 8 / 32 / 64 MB) |

### `d3d_adapter_identifier9` — size `0x44c` (SDK `D3DADAPTER_IDENTIFIER9`)

| Off | Type | Field |
|---|---|---|
| `0x000` | `char[0x200]` | `driver` |
| `0x200` | `char[0x200]` | `description` |
| `0x400` | `char[0x20]` | `device_name` |
| `0x420` | `large_integer` | `driver_version` |
| `0x428` / `0x42c` / `0x430` / `0x434` | `uint32_t` | `vendor_id` / `device_id` / `subsystem_id` / `revision` |
| `0x438` | `uint32_t[4]` | `device_identifier` |
| `0x448` | `uint32_t` | `whql_level` |

### `shell_config_property` — size `0x08` (28 at 0x0069fe40)

| Off | Type | Field |
|---|---|---|
| `0x00` | `uint32_t` | `name` (`const char *`) |
| `0x04` | `uint32_t` | `setter` (`shell_config_property_setter`, returns AL) |

### `crash_dialog_template` — size `0x16` (DLGTEMPLATE head)

| Off | Type | Field |
|---|---|---|
| `0x00` | `uint32_t` | `style` (0x80c800c0) |
| `0x04` | `uint32_t` | `extended_style` |
| `0x08` | `uint16_t` | `item_count` (1) |
| `0x0a` / `0x0c` / `0x0e` / `0x10` | `int16_t` | `x` / `y` / `cx` (0xa3) / `cy` (0x37) |
| `0x12` / `0x14` | `uint16_t` | `menu` / `window_class` |

### `crash_dialog_item_template` — size `0x16` (DLGITEMTEMPLATE head)

| Off | Type | Field |
|---|---|---|
| `0x00` | `uint32_t` | `style` (0x50020000) |
| `0x04` | `uint32_t` | `extended_style` |
| `0x08` / `0x0a` / `0x0c` / `0x0e` | `int16_t` | `x` 0x1c / `y` 0x17 / `cx` 0x6c / `cy` 8 |
| `0x10` | `uint16_t` | `id` (0xffff) |
| `0x12` / `0x14` | `uint16_t` | class ordinal marker 0xffff / 0x82 (STATIC) |

### `dw_shared_memory` — size `0x1c50` (Dr. Watson block)

| Off | Type | Field |
|---|---|---|
| `0x0000` | `uint32_t` | `size` (0x1c50) |
| `0x0004` / `0x0008` | `uint32_t` | `process_id` / `thread_id` |
| `0x000c` | `uint32_t` | `exception_address` |
| `0x0010` | `uint32_t` | `exception_pointers` |
| `0x0014` / `0x0018` / `0x001c` | `uint32_t` | `event_done` / `event_notify_done` / `event_alive` |
| `0x0020` / `0x0024` | `uint32_t` | `mutex` / `process` |
| `0x0028` | `uint32_t` | `behavior_flags` (0x100) |
| `0x002c` | `uint32_t` | `result` (bit 0 set: ExitProcess(0)) |
| `0x0034` / `0x003c` | `uint32_t` | `offer_flags` (1) / `let_run_flags` (0x11) |
| `0x0048` | `uint16_t[0x38]` | `application_name` (L"Halo") |
| `0x00b8` | `uint16_t[0x104]` | `module_file_name` |
| `0x0e38` | `char[0x310]` | `server` ("watson.microsoft.com") |
| `0x1148` | `char[0x2d0]` | `registry_subpath` |
| `0x1418` | `uint16_t[0x400]` | `additional_files` ("\|"-separated, lower-cased) |

### `digital_product_id` — size `0xa4`

| Off | Type | Field |
|---|---|---|
| `0x00` | `uint32_t` | `size` (must be 0xa4) |
| `0x04` / `0x06` | `uint16_t` | `major_version` (3) / `minor_version` (0) |
| `0x08` | `char[0x18]` | `product_id` |
| `0x20` | `uint32_t` | `unknown_20` (first `%05d` field) |
| `0x38` | `uint8_t[0xf]` | `hashed_key` (SHA-1 input) |

### Win32 / COM records (public SDK layouts)

Folded into `types/shell.h` by the review (they were local typedefs in several files):
`os_version_info_a` 0x94, `window_placement` 0x2c, `sid_identifier_authority` 6,
`generic_mapping` 0x10, `win32_memory_status` 0x20, `win32_security_attributes` 0xc,
`win32_startup_info_a` 0x44, `win32_process_information` 0x10, `win32_msg` 0x1c,
`win32_exception_record` (head, 0x10), `win32_exception_pointers` 8, `ddscaps2` 0x10,
`dxdiag_init_params` 0x10, `win32_variant` 0x10, and the vtables `direct_draw7_vtable`
(+0x08 Release, +0x50 SetCooperativeLevel, +0x5c GetAvailableVidMem), `dxdiag_provider_vtable`
(+0x0c Initialize, +0x10 GetRootContainer) and `dxdiag_container_vtable` (+0x0c child count,
+0x14 GetChildContainer, +0x20 GetProp). Function pointer types: `hwreq_*_fn` (vtable slots),
`d3d9_get_adapter_identifier_fn` (+0x14), `d3d9_get_device_caps_fn` (+0x38),
`direct_draw_create_ex_fn`, `direct_draw_enumerate_ex_fn`, `report_fault_fn`. `win32_rect` comes
from types/interface.h.

## Register conventions worth knowing

LTCG gave the small helpers custom conventions; each file header documents its own. The ones
callers most often get wrong:

| Function | Convention |
|---|---|
| `command_line_check_flag` 0x542760 | flag on the stack, out value in EDI; result in **AL only** |
| `command_line_parse_to_argv` 0x5425f0 | command line in EDI, count pointer on the stack |
| `shell_parse_config_txt` 0x57d410 | adapter index in ECX, IDirect3D9 in EDX; returns an error string or NULL |
| `shell_load_string_resource` 0x57e110 | id EAX, language ECX (full dword), capacity EBX, module EDI, buffer on the stack |
| `shell_load_localized_string` 0x57e1a0 | capacity EAX, module ECX, buffer ESI, id on the stack; returns the length |
| `hwreq_token_*` / `d3dcaps_field_resolve` | parser in EAX (`skip_whitespace`: EDX; `match_keyword`: keyword EDX, parser EDI) |
| `hwreq_parser_report_error` 0x578a20 | parser in ESI, message on the stack |
| `hwreq_parser_parse_flag_assignment` 0x578cf0 | parser in ECX, set on the stack, ret 4 |
| `hwreq_parser_parse_vendor_block` / `_audiovendor_block` | parser in EAX |
| `dialog_center_on_screen` 0x542f00 | `__stdcall` DLGPROC, ret 0x10 |
| `exception_filter_crash_reporter` 0x542fa0 | `__stdcall`, EXCEPTION_POINTERS on the stack |
| library helpers (not module code) | `std::string::assign` 0x57bc90 (ECX), `map::find` 0x57b7a0 (EDI map, ESI key, EBX iterator slot), `map::operator[]` 0x57b6e0 (EDI key, map on the stack) |

## Functions and rewrite confidence

| Address | Function | Size | Confidence |
|---|---|---|---|
| 0x5402a0 | `cpu_get_type` | 2353 | 0.85 |
| 0x540da0 | `cpu_query_identification` | 310 | 0.55 |
| 0x540ee0 | `engine_initialize_subsystems` | 297 | 0.75 |
| 0x541010 | `engine_shutdown_subsystems` | 177 | 0.75 |
| 0x5411e0 | `shell_winmain` | 2077 | 0.7 |
| 0x541a20 | `shell_pump_windows_messages` | 146 | 0.85 |
| 0x541ac0 | `clipboard_get_text` | 109 | 0.85 |
| 0x5425f0 | `command_line_parse_to_argv` | 352 | 0.55 |
| 0x542760 | `command_line_check_flag` | 114 | 0.8 |
| 0x5427e0 | `os_platform_identify` | 89 | 0.85 |
| 0x542840 | `security_check_write_access` | 568 | 0.55 |
| 0x542a80 | `security_check_cleanup` | 65 | 0.55 |
| 0x542ad0 | `keystone_library_load` | 536 | 0.7 |
| 0x542cf0 | `keystone_library_unload` | 123 | 0.9 |
| 0x542d70 | `game_single_instance_check` | 382 | 0.7 |
| 0x542f00 | `dialog_center_on_screen` | 160 | 0.85 |
| 0x542fa0 | `exception_filter_crash_reporter` | 2581 | 0.65 |
| 0x578410 | `hwreq_property_set_upsert` | 411 | 0.75 |
| 0x5785b0 | `hwreq_string_pair_destruct` | 122 | 0.8 |
| 0x578630 | `hwreq_device_override_list_find` | 98 | 0.7 |
| 0x5786a0 | `hwreq_parser_scalar_deleting_destructor` | 24 | 0.85 |
| 0x5788f0 | `hwreq_parser_find_property_set` | 222 | 0.8 |
| 0x5789d0 | `hwreq_token_skip_line` | 43 | 0.85 |
| 0x578a00 | `hwreq_token_skip_whitespace` | 20 | 0.9 |
| 0x578a20 | `hwreq_parser_report_error` | 161 | 0.8 |
| 0x578ad0 | `hwreq_token_parse_hex_digit` | 71 | 0.85 |
| 0x578b20 | `hwreq_token_parse_number` | 308 | 0.65 |
| 0x578c60 | `hwreq_token_parse_quoted_string` | 139 | 0.8 |
| 0x578cf0 | `hwreq_parser_parse_flag_assignment` | 498 | 0.7 |
| 0x578ef0 | `hwreq_token_parse_hex_id` | 129 | 0.85 |
| 0x578f80 | `hwreq_token_parse_hex_id_byteswap` | 31 | 0.9 |
| 0x578fa0 | `hwreq_token_match_keyword` | 80 | 0.85 |
| 0x578ff0 | `hwreq_d3dcaps_field_resolve` | 1712 | 0.75 |
| 0x579690 | `hwreq_parser_evaluate_condition` | 1261 | 0.75 |
| 0x579ef0 | `hwreq_parser_construct` | 235 | 0.85 |
| 0x57a010 | `hwreq_parser_destruct` | 525 | 0.8 |
| 0x57a220 | `hwreq_parser_scan_for_applytoall_or_vendor` | 243 | 0.75 |
| 0x57a320 | `hwreq_parser_scan_for_applytoall` | 192 | 0.75 |
| 0x57a3e0 | `hwreq_parser_parse_propertyset_directive` | 665 | 0.7 |
| 0x57a680 | `hwreq_parser_parse_vendor_block` | 951 | 0.75 |
| 0x57aa40 | `hwreq_parser_parse_audiovendor_block` | 1032 | 0.75 |
| 0x57ae50 | `hwreq_parser_find_requirements_section` | 187 | 0.8 |
| 0x57af10 | `hwreq_parser_parse_block` | 1372 | 0.75 |
| 0x57b4c0 | `hwreq_parser_create` | 88 | 0.85 |
| 0x57cfe0 | `config_reset_system_requirements` | 153 | 0.85 |
| 0x57d080 | `config_set_maximum_resolution` | 54 | 0.85 |
| 0x57d240 | `config_set_disable_driver_management` | 11 | 0.9 |
| 0x57d2b0 | `config_set_decal_z_bias` | 36 | 0.85 |
| 0x57d2e0 | `config_set_decal_slope_z_bias` | 36 | 0.85 |
| 0x57d310 | `config_set_transparent_decal_z_bias` | 36 | 0.85 |
| 0x57d340 | `config_set_transparent_decal_slope_z_bias` | 36 | 0.85 |
| 0x57d410 | `shell_parse_config_txt` | 969 | 0.75 |
| 0x57d7f0 | `hex_string_to_uint` | 58 | 0.8 |
| 0x57d830 | `hex_string_to_bytes` | 75 | 0.8 |
| 0x57d880 | `shell_detect_hardware_specs` | 2190 | 0.7 |
| 0x57e110 | `shell_load_string_resource` | 130 | 0.75 |
| 0x57e1a0 | `shell_load_localized_string` | 75 | 0.75 |
| 0x57e850 | `shell_check_previous_run_crash` | 439 | 0.8 |
| 0x57ea10 | `shell_registry_set_exit_flag_clean` | 95 | 0.9 |
| 0x57ea70 | `shell_display_fatal_error_dialog` | 1300 | 0.75 |
| 0x57efa0 | `shell_init_localization_strings` | 765 | 0.75 |
| 0x57f2a0 | `compute_sha1_hash` | 140 | 0.9 |
| 0x57f330 | `compute_sha1_hash_first_qword` | 44 | 0.8 |
| 0x57f360 | `extract_product_id_digits` | 136 | 0.8 |
| 0x57f3f0 | `shell_build_product_id_string` | 317 | 0.6 |

Renames are recorded in `symbols/agent_phase4_shell.txt` (including `main_loop` 0x4c7610, a
`main`-module function the Ghidra database calls `game_state_save_core`).

## Misattributed entries

Not written, by design (details in `out/phase4/shell_types_notes.md`):

- **MSVC 7.1 library code** (38 entries): the `std::logic_error` / `length_error` /
  `out_of_range` constructors and destructors 0x5782b0, 0x578310, 0x578390, 0x5783b0, 0x5783c0,
  0x5783e0, 0x5783f0, 0x57bc20; string / vector / pair helpers 0x579fe0, 0x57b590, 0x57b5b0,
  0x57b5e0, 0x57b670, 0x57b800, 0x57bd80, 0x57be00, 0x57c640, 0x57c6d0, 0x57c76c, 0x57cda0,
  0x57cde0, 0x57ce10, 0x57ce80, 0x57cf50; red-black tree code 0x57ba50, 0x57bbd0, 0x57c1a0,
  0x57c310, 0x57c530, 0x57c5e0, 0x57cb10, 0x57cb70, 0x57cb90, 0x57cbf0, 0x57cc30, 0x57cce0,
  0x57cd20, 0x57cd40.
- **Catch funclets** (6): 0x57bfec, 0x57c0a8, 0x57c743, 0x57c7e2, 0x57ccc4, 0x57ced1.
- **Not a function entry**: 0x542380 (`scenario_trigger_volume_test_point`) is a tail of the
  window procedure 0x541b30, which has no Ghidra function.
- **Not code**: 0x6bd180, 0x6bd188, 0x6bd198, 0x6bd19c, 0x6bd1b8 (`ks_*` / `kw_*` / `kc_*`) are
  `.bss` addresses nothing references.

## Known gaps

- **Window procedure 0x541b30** is now rewritten as `shell_window_procedure.c` (cleanup pass 1;
  the 0x542380 and 0x542141 chunks are part of it).
- **Parse method 0x579bd0** (vtable slot 0, opens and reads the script, copies the adapter /
  caps / sound records, runs the passes) and most vtable accessors 0x5786c0..0x5788e0 are not
  Ghidra functions and are not rewritten (0x578870 and 0x5788e0 were written in cleanup pass 1); their behaviour is documented in the notes.
- **Stack-overflow frame move** in the crash reporter (0x54302a..0x54303f) cannot be written in
  C; it is marked in the body.
- **`[ebp-0x19]` in `shell_winmain`** is written 1 and never cleared, which makes the
  "Corrupted Halo.exe" / `-testcrash` block dead code. Either a removed integrity check or code
  that the protection layer of other builds patches.
- **Cross-module names**: `engine_shutdown_subsystems` clears nine globals whose owners the
  other headers disagree on; they are declared by address only.
- Other modules still declare some shell-owned or shell-written globals with the wrong width or
  name (see the review notes below); those files were left untouched.

## Review notes (phase-4 review pass)

Fixes against the binary, beyond the three new files:

- `shell_winmain`: restored the outer `__try` / `__except(exception_filter_crash_reporter(...))`;
  flags are 32-bit BOOLs, the sound cache size is 16-bit; `memory_global_alloc` / `_free` take
  EAX; `shell_parse_config_txt(0, d3d)`; the EULA gets `eula_file_name`; `-?` and `-ip`
  literals recovered; `network_session_host_start_info_set("halor", "e4Rd9J", ip, port)`;
  `main_loop` call; `-testcrash` null write kept.
- `hwreq_parser_parse_block`: a `propertyset = "name"` reference merges into the caller's target,
  not `this->flags`.
- `hwreq_parser_evaluate_condition`: the whitespace skip before the operator was missing.
- `hwreq_parser_report_error`: "..." is also added for a line of exactly 36 characters;
  `std::string::assign` instead of a private re-initialising helper (also in
  `hwreq_property_set_upsert` and `hwreq_parser_parse_flag_assignment`, which also leaked its
  temporary string).
- `shell_parse_config_txt`: `get_error_message` takes no argument; the setter call is inside
  `__try`.
- `shell_display_fatal_error_dialog`: the exit-path shutdown calls are inside `__try`; callee
  names match the rasterizer and sound modules.
- `command_line_check_flag` returns AL only (`uint8_t`); `shell_load_localized_string` returns the
  length; `shell_check_previous_run_crash` returns full EAX; `dialog_center_on_screen` is a
  `__stdcall` DLGPROC.
- Width fixes: `0x00686b50` is a byte, `0x0087ac08` is 16-bit, `safe_mode` / `nosound` are
  32-bit.

## Cleanup pass 1: functions Ghidra never created

Real functions reached only through vtables, dispatch tables or call sites, found by the phase-4
types agents, created in the Ghidra project as `missed_XXXXXX` and rewritten here under Blam
names (symbols in `symbols/agent_phase4_missed.txt`). Register conventions were taken from
objdump of the function and of the table or call site that reaches it.

| Address | Function | Size | Name conf. | Rewrite conf. | UNSURE | Note |
|---|---|---|---|---|---|---|
| `0x541b30` | `shell_window_procedure` | 2544 | 0.85 | 0.55 | 6 | review: WM_ENTERSIZEMOVE no longer sets shell_window_minimized; FUN_005410d0 gets BL; input_key_block_timer_set gets its EDI key; Win32 message names fixed |
| `0x578870` | `hwreq_parser_get_graphics_device_name` | 14 | 0.8 | 0.85 | 0 |  |
| `0x5788e0` | `hwreq_parser_get_error_message` | 14 | 0.8 | 0.85 | 0 |  |
| `0x57d0c0` | `config_set_force_shader` | 77 | 0.7 | 0.85 | 1 |  |
| `0x57d110` | `config_set_disable_buffering` | 11 | 0.7 | 0.9 | 0 |  |
| `0x57d230` | `config_set_safe_mode` | 11 | 0.7 | 0.9 | 0 |  |
| `0x57d250` | `config_compute_uma_video_memory` | 67 | 0.5 | 0.7 | 2 |  |
| `0x57d2a0` | `config_set_min_max_blend_op_is_broken` | 11 | 0.7 | 0.9 | 0 |  |

Not written (not real functions; internal chunks of another function, folded into its owner):

- `0x542141`: part of shell_window_procedure (0x541b30), its DefWindowProcA tail.
- `0x578aaf`: part of hwreq_parser_report_error (0x578a20), its tail.
- `0x57a74c`: part of hwreq_parser_parse_vendor_block (0x57a680).
- `0x57a801`: part of hwreq_parser_parse_vendor_block (0x57a680).
- `0x57a989`: part of hwreq_parser_parse_vendor_block (0x57a680).
- `0x57aa19`: part of hwreq_parser_parse_vendor_block (0x57a680).

Gate: `python tools/build_check.py shell` clean after the cleanup review.
