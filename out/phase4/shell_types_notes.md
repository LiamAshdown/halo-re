# shell module: type recovery notes

Header: `types/shell.h`. Smoke test: `out/phase4/shell_smoke.c`, built with

```
C:\msys64\ucrt64\bin\gcc.exe -fsyntax-only -I types out/phase4/shell_smoke.c
```

It passes with `-Wall -Wextra`, and with `-m32`. It checks every struct size, about 110 field
offsets, the block-copy sizes of the parser parse method, the array ends against the next
global, and the dword indices the decompile uses for the Dr. Watson block. A translation unit that
includes every header in `types/` plus `shell.h` also compiles, so no name collides.

The header is not standalone. It embeds `d3d_caps9` (types/rasterizer.h) and `large_integer`
(types/math.h) instead of redefining them, so the smoke file includes
`tags.h memory.h math.h rasterizer.h` before `shell.h`. Ghidra reads `types/*.h` in alphabetical
order, and both of those sort before `shell.h`. A file that includes only `tags.h memory.h shell.h`
fails on `large_integer`. That is expected.

Nothing already defined elsewhere is redefined. The Keystone slots that the chat code calls
already have `chat_gui_*` typedefs in types/interface.h. They are named in the global comments
only, so `shell.h` does not depend on interface.h. The 16-byte GUIDs are held as
`uint32_t[4]`. That is the shape the requirements parser builds (four dwords, `iStack_a4..98`),
and it has the same bytes as `input_guid` in interface.h.

Pointer fields inside structs are `uint32_t`, with the pointee type written first in the comment
(the rasterizer.h convention).

## Binary evidence the layouts hang on

- **hwreq_parser 0x6b8**: `operator_new(0x6b8)` in `hwreq_parser_create` 0x57b4c0. The parse
  method 0x579bd0 (vtable slot 0, not a Ghidra function) copies its arguments with
  `rep movsd`: 0x1a dwords from arg 2 to +0x638, 0x113 dwords from arg 3 to +0xbc, 0x4c dwords
  from arg 4 to +0x508. It also stores arg 5 at +0xb4, arg 6 at +0xb8 and arg 7 at +0xb0
  (0x579c57..0x579cc0; `ret 0x1c` means seven arguments). Those three sizes are exactly
  `D3DADAPTER_IDENTIFIER9` (0x44c), `D3DCAPS9` (0x130) and the DxDiag sound record (0x68).
- **caps / identifier field names**: in `hwreq_d3dcaps_field_resolve` 0x578ff0 every keyword
  comparison passes the keyword in EDX (`mov edx,<string>` just before `call 0x578fa0`). It is
  followed by `mov esi,[edi+off]`, which reads the field. The 44 pairs (script 0x578ff0..0x579603)
  run from `Caps` at +0x510 to `PixelShaderVersion` at +0x5d4. That is `d3d_caps9` at +0x508 with
  the SDK offsets, including the holes where `MaxTextureHeight`, the guard band floats and
  `MaxPointSize` are not keywords. `subsysid` is at +0x4ec, `revision` at +0x4f0, `guid` at +0x4f4
  (16-byte compare) and `driver` at +0x4dc/+0x4e0. Those are the identifier at +0xbc, whose
  `VendorId` +0x428 is the +0x4e4 that the vendor block compares.
- **the five strings, two maps and two set pointers**: `hwreq_parser_construct` 0x579ef0 sets
  capacity 0xf / size 0 / buffer byte on +0x3c/+0x38/+0x28, +0x58/+0x54/+0x44, +0x74/+0x70/+0x60,
  +0x90/+0x8c/+0x7c and +0xac/+0xa8/+0x98. It builds head nodes at +0x6a4 and +0x6b0 with size
  0 at +0x6a8 / +0x6b4, and clears +0x18 / +0x1c. `hwreq_parser_destruct` 0x57a010 undoes all of
  them in reverse order.
- **string meanings** come from the writers and the vtable accessors 0x578870..0x5788e0
  (disassembled; they are not Ghidra functions). The vendor block assigns +0x5c after
  `cmp [esi+0x4e4]` (0x57a74c / 0x57a801). The vendor device line assigns +0x40 after
  `cmp [esi+0x4e8]` (0x57a989 / 0x57aa19). The audiovendor block assigns +0x94 after
  `cmp [esi+0x690]` and +0x78 after `cmp [esi+0x694]`. Report_error assigns +0x24
  (`lea ecx,[esi+0x24]` at 0x578aaf). `shell_parse_config_txt` stores vtable 0x2c (+0x5c) in
  0x00722b90 and vtable 0x28 (+0x40) in 0x00722b94. The fatal error dialog prints them as
  "vendor device (0xdevice id)".
- **MSVC 7.1 containers**: string capacity tests `< 0x10` with buffer at +4, size at +0x14 and
  capacity at +0x18 in every reader. The pair stride is 0x38 (0x578410, 0x578630, 0x57b5b0).
  The node is 0x30 (`operator_new(0x30)` in 0x57cbf0 / 0x57cc30), with color at +0x2c, is_nil at
  +0x2d, value at +0x28 (`piVar2[10]` in the destructor, `+0x28` in 0x5788f0) and the key string
  at +0xc (size +0x20, capacity +0x24, buffer +0x10 in 0x57cc30 and 0x57cde0).
- **property set 0x14**: `operator_new(0x14)` in 0x579bd0 (twice) and in 0x57a3e0. It clears
  +4/+8/+0xc and stores the parser at +0x10. The destructor loop frees exactly those three
  vector words.
- **shell_sound_device 0x68**: the DxDiag loop in `shell_detect_hardware_specs` 0x57d880 steps
  `puVar8 += 0x34` words from 0x006ef9f6 (= base + 0x46). The array memset is 0x104 dwords
  (10 records). The driver version halves are pinned by the disassembly at 0x57e04e..0x57e0a7:
  the sscanf fields a.b.c.d give `a<<16|b` stored at +0x54 and `c<<16|d` stored at +0x50.
- **shell_display_adapter 0x34**: callback 0x57d370 (`imul eax,eax,0x34`, base 0x006efdc0
  for the GUID, 0x006efdd0 for the name; it stops at 0xa entries). The video memory at
  0x006efdf0 comes from the detector, and the config.txt lookup walks name at -0x20 and value
  with stride 0xd dwords.
- **dw_shared_memory 0x1c50**: `CreateFileMappingA(.., 0x1c50)`, a 0x714-dword memset and
  `*puVar5 = 0x1c50`.
- **digital_product_id 0xa4**: 0x57f471..0x57f488 compare `[buf] == 0xa4`,
  `word [buf+4] == 3` and `word [buf+6] == 0`. EBX = buf+8 goes to 0x57f360 and EDI =
  `[buf+0x20]`. SHA-1 runs over 0xf bytes at buf+0x38 (`lea ecx,[esp+0x64]` with buf at
  esp+0x2c).
- **config property table**: `(&PTR_s_ForceShader_0069fe40)[i * 2]` names, and the indirect
  call through `i * 8 + 0x0069fe44`, loop bound 0x1c. All 28 name / setter pairs were dumped from
  .data and each setter disassembled to its target global.

## Structs, and which functions established which fields

### `hwreq_parser` (0x6b8)
| offset | field | established by |
|---|---|---|
| 0x000 | vtable 0x006721e8 | 0x579ef0, 0x57a010 |
| 0x004 | file_buffer | 0x579bd0 (`malloc(size + 0x10)`, CR at [size], freed on every exit) |
| 0x008 / 0x00c / 0x010 / 0x014 | cursor, end, line_start, line_number | 0x5789d0, 0x578a00, 0x578b20, 0x578c60, 0x578cf0, 0x578ef0, 0x578fa0, 0x57a220, 0x57a320, 0x57a3e0, 0x57af10, 0x579bd0 (reset before each pass) |
| 0x018 | flags (property set *) | 0x579bd0 (allocates), 0x57a220 / 0x57a320 (parse_block target), 0x57af10 (MaxOverallGraphicDetail merges into it), vtable 0x08 / 0x10 / 0x14 / 0x18 |
| 0x01c | requirements (property set *) | 0x579bd0, 0x57ae50 (parse_block on +0x1c), vtable 0x1c / 0x20 / 0x24 |
| 0x020 | error_reported | 0x578a20 (latch), 0x579bd0 (clears), vtable 0x38 |
| 0x021 | unknown_21[3] | never referenced |
| 0x024 .. 0x0ac | five msvc_std_string | 0x579ef0, 0x57a010, 0x578a20, 0x579bd0 ("Cannot find"), 0x57a680, 0x57aa40, vtable 0x28..0x3c |
| 0x0b0 / 0x0b4 / 0x0b8 | cpu_speed, memory, video_memory | 0x579bd0 stores, 0x578ff0 keywords cpuspeed / ram / videoram (0x579029, 0x579048, 0x57957b) |
| 0x0bc | adapter (d3d_adapter_identifier9) | 0x579bd0 copy, then GetFileVersionInfo on `adapter.driver` when the driver version is 0; 0x578ff0 / 0x579690 subsysid, revision, guid, driver; 0x57a680 vendor / device id |
| 0x508 | caps (d3d_caps9) | 0x579bd0 copy, 0x578ff0 (44 keywords) |
| 0x638 | sound_device (shell_sound_device) | 0x579bd0 copy, 0x57aa40 (+0x690 / +0x694) |
| 0x6a0 | property_sets map | 0x579ef0, 0x57a010 (frees the values), 0x57a3e0 (`map[name] = set` via 0x57b6e0), 0x5788f0 / vtable 0x0c (find), 0x57af10 (apply a named set) |
| 0x6ac | graphic_detail_sets map | 0x579ef0, 0x57a010, 0x578cf0 (`map[OverallGraphicDetail value] = current set`), 0x57af10 (MaxOverallGraphicDetail lookup against head +0x6b0) |

### `hwreq_parser_vtable` (0x40)
Dumped from 0x006721e8. Slot targets were disassembled: 0x5786c0 / 0x578790 are
`(end - begin) / 0x38` on +0x18 / +0x1c. 0x5786f0 / 0x578740 / 0x5787c0 / 0x578810 index the
pair and return `.first` / `.second` (range checked by 0x57b9e0). The name accessors are listed
above. There is no RTTI locator (the dword before the vtable is 0).

### `hwreq_property_set` (0x14), `hwreq_string_pair` (0x38)
0x578410 (upsert: walks begin +4 .. end +8, stricmp on the key, re-assigns the value or
push_back), 0x578630 (find, strncpy of `.second` into the caller buffer), 0x57b5b0 / 0x57b5e0
(size / push_back), 0x57b670 / 0x5785b0 (pair construct / destruct), 0x57a010 (destroys sets).

### `msvc_std_string` (0x1c), `msvc_std_vector` (0x10), `msvc_std_map` (0x0c), `hwreq_map_node` (0x30)
See above. These are the library layouts. They are defined only because module code
(0x578410, 0x578630, 0x5788f0, the parser constructor and destructor, and the vtable accessors)
reads their fields directly.

### `d3d_adapter_identifier9` (0x44c)
SDK layout, size-pinned by the 0x113-dword copy. The offsets are confirmed by the parser (above)
and by `shell_parse_config_txt` 0x57d410. That function receives the IDirect3D9 in EDX, calls
vtable +0x14 (GetAdapterIdentifier) into `local_5b0`, compares `local_1b0` (= +0x400 DeviceName)
with the adapter names, and copies `local_190 / 18c` (+0x420/+0x424 DriverVersion) to
0x00722ba0/4, `local_188` (+0x428 VendorId) to 0x00722b9c and `local_184` (+0x42c DeviceId) to
0x00722b98. `description` (+0x200) and `whql_level` (+0x448) are named from the SDK only. The
module never reads them.

### `shell_sound_device` (0x68)
0x57d880: szDescription strncpy (0x1f, terminator at +0x1f), GUID from the hex string (Data1 at
+0x40, Data2 +0x44, Data3 +0x46, Data4 by `hex_string_to_bytes`) compared with the default
device GUID (`GetDeviceID`), which selects 0x006effd0. Also the driver version (+0x50/+0x54) and
ven_ / dev_ / subsys_ / rev_ at +0x58 / +0x5c / +0x60 / +0x64. **Unresolved: +0x20..+0x3f** is
never written or read in this module.

### `shell_display_adapter` (0x34)
0x57d370 (GUID for entries past 0, lpDriverName when shorter than 0x1f), 0x57d880 (video memory
at +0x30, minimum of four GetAvailableVidMem results, rounded), 0x57d410 (name match selects
0x00722bb0).

### `shell_config_property` (8)
0x57d410. Table contents, setter targets and meanings are in the header global list.
`FUN_0057d080` is the MaximumResolution setter (0x280..0x1000). `FUN_0057d2b0` / `2e0` / `310` /
`340` are the DecalZBias / DecalSlopeZBias / TransparentDecalZBias / TransparentDecalSlopeZBias
setters (floats at 0x00722b80 / 88 / 84 / 8c). The remaining 22 setters (0x57d0c0,
0x57d110..0x57d230, 0x57d250 UMA, 0x57d2a0) are not Ghidra functions.

### `crash_dialog_template` / `crash_dialog_item_template` (0x16 each)
0x542fa0: `*puVar5 = 0x80c800c0`, count 1 at +8, cx/cy 0xa3/0x37 at +0xe/+0x10, then
menu / class 0 at +0x12/+0x14 and the title at +0x16. After the font (size 8, "MS Sans Serif"),
the pointer is rounded up to a dword and the item gets style 0x50020000, x/y/cx/cy
0x1c/0x17/0x6c/8, id 0xffff, class 0xffff 0x0082 and its text at +0x16.

### `dw_shared_memory` (0x1c50)
0x542fa0, dword indices: [0] size, [1] pid, [2] tid, [3] = `*(*pep + 0xc)` exception address,
[4] pep, [5] first event (polled for done after the alive event), [7] second event (waited
600000 ms, "alive"), [8] mutex (waited 20000 ms), [9] DuplicateHandle target, [10] = 0x100,
[0xb] result (bit 0 chooses return 0 over ExitProcess(0)), [0xd] = 1, [0xf] = 0x11. Also
wchar +0x48 L"Halo", wchar +0xb8 module file name, char +0xe38 "watson.microsoft.com",
char +0x1148 the PCHealth ErrorReporting DW registry subpath, and wchar +0x1418 the
additional-file list (limit 0x400).
**Unresolved**: +0x18 (event_notify_done is named from the protocol, never written), +0x30,
+0x38, +0x40, +0x44, +0x2c0..+0xe37, and the true lengths of `server` (0xe38..0x1147) and
`registry_subpath` (0x1148..0x1417). The header gives each the whole gap up to the next written
field. +0x1c18..+0x1c4f is unknown.

### `digital_product_id` (0xa4)
0x57f3f0 / 0x57f360 (see the evidence above). The digit tables at 0x00672a68 (OEM:
12..15, 18..22) and 0x00672a90 (retail: 6..8, 10..15) index `product_id`.
**Unresolved**: +0x20 (printed as the first `%05d` field), +0x24..+0x37, +0x47..+0xa3.

## Globals

Every owned global is listed at the end of `types/shell.h`. Points worth repeating:
- The window block 0x007461c0..0x00746277 and 0x007461a8 are written only by `shell_winmain` /
  `engine_initialize_subsystems`. They are read by rasterizer, input and the fatal error
  dialog. They lie outside the shell .bss clusters (0x00721e5c..0x00721f13 and
  0x00722b28..0x007231db), so the defining object is uncertain. They are kept as globals rather
  than a struct, because nothing addresses them relative to a base. The meaning of 0x00746254 /
  0x00746255 (minimized / maximized) comes from the WM_SIZE arm of the window proc
  (0x541d00..0x541f35).
- The CPUID cache, the Keystone slots and the config flags are likewise independent globals.
  Every access is absolute, so no struct is claimed.
- **Unreferenced gaps**: 0x006ef9ac, 0x006effe4, 0x00721ef4..0x00721eff, 0x00722bc4 and
  0x00722bd4 (0x00722bc0 / 0x00722bc8 / 0x00722c58 belong to the three "dialogs" functions,
  see below).
- 0x0069e8d4 / 0x0069e8d8 / 0x00746f8c..0x00746fa0 / 0x00686b4c..0x00686b5c, which
  `engine_shutdown_subsystems` resets, belong to hs / objects / other modules and are not
  listed.
- `engine_initialize_subsystems` zeroes 0x006ac900 as 0x41 dwords plus one byte (0x105 bytes).
  types/cache.h has it as `char profile_directory[0x104]`, so the memset runs one byte past
  that array (not changed here).

## Misattributed, library, or not-a-function entries in the 115-function list

Library code (MSVC 7.1 standard library instantiations; types skipped, not defined):
- 0x5782b0 `std::logic_error::logic_error(const string &)`, 0x578310 `~logic_error`, 0x578390
  its scalar deleting destructor. 0x5783b0 / 0x5783c0 are `length_error` and 0x5783e0 /
  0x5783f0 are `out_of_range` (destructor and scalar deleting destructor). 0x57bc20 is the
  logic_error copy constructor. RTTI confirms the classes: the vtables 0x00655080 / 0x0065508c /
  0x00655098 resolve to `.?AVlogic_error@std@@`, `.?AVlength_error@std@@` and
  `.?AVout_of_range@std@@`. The layout is exception (0xc) plus one std::string at +0xc
  (0x28 total). The pass-1 evidence of two strings at 0x10 / 0x20 is wrong. It is one string:
  buffer +0x10, size +0x20, capacity +0x24.
- Strings and containers: 0x57b590 string::assign(const char *), 0x57b5b0 vector::size,
  0x57b5e0 vector::push_back, 0x57b670 pair constructor, 0x5785b0 pair destructor, 0x57b800
  string _Tidy, 0x57bd80 string::erase, 0x57be00 _Destroy_range, 0x57c640 pair default
  construct, 0x57c6d0 / 0x57c76c string _Grow / _Copy, 0x57cda0, 0x57cde0 (node key _Tidy),
  0x57ce10 string::compare, 0x57ce80 _Uninit_fill_n, 0x57cf50 _Uninit_copy, and 0x579fe0 / map
  _Tidy. Tree code: 0x57ba50, 0x57bbd0, 0x57c1a0, 0x57c310, 0x57c530, 0x57c5e0, 0x57cb10,
  0x57cb70, 0x57cb90, 0x57cbf0, 0x57cc30, 0x57cce0, 0x57cd20, 0x57cd40. Catch funclets:
  0x57bfec, 0x57c0a8, 0x57c743, 0x57c7e2, 0x57ccc4, 0x57ced1. modules.json already marks their
  neighbours (0x57b6e0, 0x57b7a0, 0x57b830, 0x57b920, 0x57b990, 0x57bc90, 0x57c390, 0x57c820)
  as lib:crt.

Not functions, or misnamed:
- **0x542380** `scenario_trigger_volume_test_point`: this is not a function entry. It is a tail
  of the window procedure at **0x541b30**, which has no Ghidra function (only
  `&DAT_00541b30` in winmain). The chunk begins with `ja 0x542141` and reads the proc frame
  (`[esp+0x40..0x48]`). The Win32 reading in the phase 2 evidence is right; the entry point is
  wrong.
- **0x579690** `gamespy_get_client_key_hash`: this is the condition evaluator. It handles
  operators, the GUID / driver / os terms and the numeric compare, and continues
  `hwreq_d3dcaps_field_resolve`. It has nothing to do with GameSpy.
- **0x57e850** `chimera__registry_check_2`: this is a previous-run crash check. It tests for
  a .pdb beside the path held at 0x006a32e8, reads ExitFlag and rewrites "bad 1" / "bad 2". It is not a Chimera
  hook.
- **0x6bd180, 0x6bd188, 0x6bd198, 0x6bd19c, 0x6bd1b8** (`ks_*`, `kw_*`, `kc_*`): these are
  .bss addresses, past the 0x2b000 raw bytes of .data (0x006a1000). Nothing in the image
  references them. They are not code. The real Keystone entry points are the GetProcAddress
  results in 0x00721ea0..0x00721ee8.
- Misleading pass-1 names: `hwreq_device_override_list_*` (0x578410 / 0x578630) operate on a
  property set's flag vector. `hwreq_symbol_table_find` (0x5788f0) is vtable slot 3,
  find_property_set. `FUN_0057f3f0` builds the product id string. `FUN_0057f360` extracts the
  product id digits. `FUN_0057f330` hashes the digest again and keeps 8 bytes.

In-range code missing from the list (types covered here): the window proc 0x541b30 and
0x5410d0 (called from one of its message arms, 0x541f66); the DirectDraw enum callback 0x57d370;
22 config.txt setters; the parser vtable methods 0x579bd0, 0x578860, 0x5786c0..0x578810 and
0x578870..0x5788e0. Also, 0x57e1f0 / 0x57e350 / 0x57e4c0 are assigned to module "dialogs" but
sit between shell functions (0x57e1a0 .. 0x57e850). With the alphabetical link order that
cannot be a separate module, so they are most likely shell (they use 0x00722bc0 / 0x00722bc8 /
0x00722c58 and the fatal error strings).

## Register conventions seen (confirm per function before rewriting)

Checked in objdump:
- `hwreq_token_match_keyword` 0x578fa0: keyword in EDX, parser in EDI.
- `extract_product_id_digits` 0x57f360: product id string in EBX.
- 0x57f330: data in EDX, length in ECX, 8-byte output in ESI.
- The parser `parse` method: thiscall with 7 stack arguments.
- `keystone_dispatch_message`: cdecl `(root, msg, wparam, lparam, &handled)`.
- `shell_parse_config_txt`: IDirect3D9 * in EDX.

From Ghidra only (the parser register varies per helper):
- EAX: 0x5789d0, 0x578b20, 0x578c60, 0x578ef0, 0x578ff0 (moved to EDI in the body), 0x57a680,
  0x57aa40.
- EDX: 0x578a00.
- ESI: 0x578a20, 0x578ad0, 0x57a220, 0x57a320, 0x57ae50.
- ECX: 0x578cf0 (property set on the stack), 0x57a3e0.

Other helpers:
- `command_line_parse_to_argv` 0x5425f0: command line in EDI, count out-pointer on the stack.
- `command_line_check_flag` 0x542760: value out-pointer in EDI.
- `shell_load_string_resource` 0x57e110: id in EAX, language in CX, buffer size in EBX, module in
  EDI, buffer on the stack.
- `hex_string_to_uint` 0x57d7f0: string in EDX.
- `hex_string_to_bytes` 0x57d830: EDX source, ECX destination.
