# cache — Blam map-file and streaming caches

`halo.exe` 1.0.10 retail, `0x442290..0x444b30`. 49 rewritten functions in `src/cache/*.c`,
typed against `types/cache.h` (module-owned types) plus `types/memory.h` (`data_array`,
`datum_index`, `cache`, `cache_entry`) and `types/tags.h` (every tag structure this module
reads). Gate: `python tools/build_check.py cache` → **49 ok, 0 failed**; the whole repo is
198 ok, 0 failed. The standalone header parse gate `out/phase4/cache_smoke.c` also passes.

## What the module does

Five things sit in this address range, in dependency order:

1. **The map (cache) file.** Six `cache_file_slot`s, each an open `.map` handle plus a copy of
   its 0x800-byte header. `cache_file_open_by_name` picks an LRU slot by category and size,
   `cache_file_slot_read_header` reads and validates the header, `cache_file_load` copies the
   active slot's header into the current-map global, reads the tag data block to `0x40440000`
   and publishes `tag_header` / `tag_instances`. `tag_lookup` and `tag_iterator_next` are the
   two readers of the tag address table.
2. **The multiplayer map downloader glue.** Four functions (`cache_file_request_map`,
   `cache_file_download_poll`/`_status_get`, `_stop`, `_finish`) over the foreign
   `map_download_state` object reached through the pointer at `0x006869c0`. The object itself
   is owned elsewhere in the image and is mostly unresolved.
3. **The two external data files**, `maps\bitmaps.map` and `maps\sounds.map`
   (`data_file_open`/`_close` and the three `data_file_read_*` readers).
4. **The asynchronous IO queue.** A 0x200-entry `cache_io_request` array whose first 0x14 bytes
   are a Win32 `OVERLAPPED`, a worker thread (`_async` via `ReadFileEx` + APC, or `_sync` via
   blocking `ReadFile`, chosen by `os_platform`), and two completion APCs.
5. **The two runtime streaming caches**, sound pages and texture handles, built on the generic
   `cache` container from `types/memory.h`. `sound_cache_touch` / `texture_cache_get` are the
   hot entry points; `*_page_allocate` submits the read; the eviction hooks live at
   `0x00444060`/`0x004440a0` (sound) and `0x00444700`/`0x00444730` (texture).

Loading a map's model and BSP vertex buffers also lands here
(`model_load_vertex_buffers`, `structure_bsp_load_material_vertex_buffers` and their disposal
counterparts), because both are driven straight off the freshly-read cache file.

## Struct layouts

All structs are declared in `types/cache.h` under `#pragma pack(push, 1)`.

### `cache_file_header` — 0x800

| off | type | field | note |
|---|---|---|---|
| 0x000 | `uint32_t` | `head` | `"head"` = 0x68656164 |
| 0x004 | `int32_t` | `version` | must be 7 |
| 0x008 | `int32_t` | `file_size` | must be in [0, 0x18000000] |
| 0x00c | `uint32_t` | `unknown_00c` | never read |
| 0x010 | `uint32_t` | `tag_data_offset` | file offset of the tag data block |
| 0x014 | `uint32_t` | `tag_data_size` | bytes read to `0x40440000` |
| 0x018 | `uint32_t` | `unknown_018` | never read |
| 0x01c | `uint32_t` | `unknown_01c` | never read |
| 0x020 | `char[32]` | `name` | `strlen < 0x20` |
| 0x040 | `uint8_t[32]` | `unknown_040` | published layout says `build[32]` |
| 0x060 | `int16_t` | `map_type` | the slot *category* argument; published layout says map type |
| 0x062 | `int16_t` | `unknown_062` | |
| 0x064 | `uint32_t` | `crc32` | read only by the game-state save/load code at 0x00538569 |
| 0x068 | `uint8_t[0x794]` | `unknown_068` | |
| 0x7fc | `uint32_t` | `foot` | `"foot"` = 0x666f6f74 |

Validated identically in three places (`cache_file_load`, `cache_file_exists`,
`cache_file_slot_read_header`); the five checked fields plus the two offsets the loader reads
are the only ones anything in the image touches.

### `cache_file_slot` — 0x80c, six of them at `0x006a9428`

| off | type | field |
|---|---|---|
| 0x000 | `void *` | `file` (HANDLE, -1 when invalid) |
| 0x004 | `file_time` | `last_write_time` |
| 0x00c | `cache_file_header` | `header` |

### `cache_file_tag_header` — 0x28, at `0x40440000`

| off | type | field |
|---|---|---|
| 0x00 | `tag_instance *` | `tags` (== `tag_instances`) |
| 0x04 | `datum_index` | `scenario_tag` (`cache_file_load`'s return) |
| 0x08 | `uint32_t` | `unknown_08` (published: checksum) |
| 0x0c | `int32_t` | `tag_count` |
| 0x10 | `uint32_t` | `unknown_10` |
| 0x14 | `uint32_t` | `model_data_file_offset` |
| 0x18 | `uint32_t` | `unknown_18` |
| 0x1c | `uint32_t` | `model_index_data_offset` |
| 0x20 | `uint32_t` | `model_data_size` |
| 0x24 | `uint32_t` | `unknown_24` (published: `"tags"`) |

### `tag_instance` — 0x20

| off | type | field |
|---|---|---|
| 0x00 | `tag_group` | `group_tag` |
| 0x04 | `tag_group` | `parent_group_tag` (-1 = none) |
| 0x08 | `tag_group` | `grandparent_group_tag` (-1 = none) |
| 0x0c | `datum_index` | `tag_id` |
| 0x10 | `char *` | `path` |
| 0x14 | `void *` | `data` |
| 0x18 | `uint32_t` | `unknown_18` |
| 0x1c | `uint32_t` | `unknown_1c` |

### `tag_iterator` — 0x14 (stack object)

| off | type | field |
|---|---|---|
| 0x00 | `uint32_t` | `unknown_00` (left uninitialized by both callers) |
| 0x04 | `int16_t` | `next_index` |
| 0x06 | `int16_t` | `unknown_06` |
| 0x08 | `uint32_t` | `unknown_08` |
| 0x0c | `uint32_t` | `unknown_0c` |
| 0x10 | `tag_group` | `group_tag` (-1 matches every tag) |

### `cache_io_request` — 0x30, 0x200 of them

| off | type | field |
|---|---|---|
| 0x00 | `uint32_t` | `internal` (OVERLAPPED) |
| 0x04 | `uint32_t` | `internal_high` |
| 0x08 | `uint32_t` | `offset` |
| 0x0c | `uint32_t` | `offset_high` (always 0) |
| 0x10 | `void *` | `event` (always 0) |
| 0x14 | `uint32_t` | `size` |
| 0x18 | `void *` | `destination` |
| 0x1c | `uint8_t` | `priority` (raised to 1 in place by blocking waiters) |
| 0x1d | `uint8_t` | `pending` |
| 0x1e | `uint8_t` | `started` |
| 0x1f | `uint8_t` | `pad_1f` |
| 0x20 | `uint8_t` | `data_file_index` (`cache_io_data_file`) |
| 0x21 | `uint8_t[3]` | `pad_21` |
| 0x24 | `cache_io_completion` | `completion` |

### `cache_io_completion` — 0x0c

| off | type | field |
|---|---|---|
| 0x00 | `uint8_t *` | `flag` (cleared on submit, set to 1 when the read finishes) |
| 0x04 | `void (*)(cache_io_completion *)` | `procedure` |
| 0x08 | `void *` | `data` |

### `data_file` — 0x40 (`bitmaps_data_file`, `sounds_data_file`)

| off | type | field |
|---|---|---|
| 0x00 | `int32_t` | `file_id` (1 bitmaps, 2 sounds) |
| 0x04 | `int32_t` | `data_offset` |
| 0x08 | `int32_t` | `table_offset` |
| 0x0c | `int32_t` | `entry_count` |
| 0x10 | `data_file_reference *` | `references` (`entry_count` * 0x0c) |
| 0x14 | `int32_t` | `reference_count` |
| 0x18 | `int32_t` | `data_size` |
| 0x1c | `int32_t` | `data_capacity` |
| 0x20 | `void *` | `data` |
| 0x24 | `uint32_t` | `unknown_24` |
| 0x28 | `uint8_t[16]` | `unknown_28` |
| 0x38 | `char *` | `name` (a literal, not copied) |
| 0x3c | `void *` | `file` (HANDLE) |

`data_file_reference` is 0x0c; only the stride is established, by the allocation size.

### `sound_cache_entry` — 0x10 ("pc sound", 0x200 entries)

| off | type | field |
|---|---|---|
| 0x00 | `int16_t` | `identifier` (datum header) |
| 0x02 | `uint8_t` | `loaded` |
| 0x03 | `uint8_t` | `decoded` |
| 0x04 | `uint8_t` | `unknown_04` |
| 0x05 | `uint8_t` | `lock_count` |
| 0x06 | `uint8_t` | `playing` |
| 0x07 | `uint8_t` | `unknown_07` |
| 0x08 | `int16_t` | `io_request_index` |
| 0x0a | `int16_t` | `unknown_0a` |
| 0x0c | `SoundPermutation *` | `permutation` |

### `texture_cache_entry` — 0x10 ("pc texture", 0x1000 entries)

Note the field order differs from `sound_cache_entry`.

| off | type | field |
|---|---|---|
| 0x00 | `int16_t` | `identifier` |
| 0x02 | `int16_t` | `io_request_index` |
| 0x04 | `uint8_t` | `loaded` |
| 0x05 | `uint8_t` | `converted` |
| 0x06 | `uint8_t` | `unknown_06` |
| 0x07 | `uint8_t` | `unknown_07` |
| 0x08 | `BitmapData *` | `bitmap` |
| 0x0c | `void *` | `texture` (`texture_cache_get` returns this field's *address*) |

### `map_download_state` — 0xac8, at `0x006a8960` via the pointer at `0x006869c0`

Only eight fields are established; everything else is padding named `unknown_XXX`.

| off | type | field |
|---|---|---|
| 0x110 | `int32_t` | `queued_file_count` |
| 0x908 | `uint32_t` | `status_flags` (bit 1 = cancelled) |
| 0x954 | `void *` | `stop_event` |
| 0x958 | `void *` | `finished_event` |
| 0x95c | `void *` | `progress_event` |
| 0x960 | `void *` | `thread` |
| 0x98c | `uint8_t` | `thread_busy` |
| 0xaa4 | `float` | `progress` |

### Small helper types

`file_time` (0x08: `low_date_time`, `high_date_time`) and `system_time` (0x10, the Win32
`SYSTEMTIME` layout) are both declared in `types/cache.h` because no `windows.h` is available
to Ghidra's CParser.

## Known gaps

- **`map_download_state` is ~97% unresolved.** Nothing else in the image references the object
  by pointer or by address, so the four glue functions here are all the evidence there is. Its
  extent is only bounded above, by the next global at `0x006a9428`. `cache_file_request_map`
  (7 `UNSURE` markers, rewrite confidence 0.60) is the weakest file in the module for this
  reason, along with the four globals at `0x00718fac..0x00718fb1` it pokes, which belong to no
  module this project has mapped.
- **`cache_file_header` bytes 0x0c, 0x018, 0x01c, 0x040-0x05f, 0x060-0x063, 0x068-0x7fb.**
  No function in the image reads them. 0x060 is used as a slot-category selector by
  `cache_file_open_by_name`, which corroborates the published layout's "map type", but the
  field is still named `unknown_060` rather than renamed on inference.
- **Eviction hooks not rewritten.** `0x00444060`, `0x004440a0`, `0x00444700`, `0x00444730` and
  the 15-byte thunk at `0x00443e00` all have no Ghidra function boundary and are not listed in
  `out/phase4/cache_functions.md`. Their behaviour is documented in `types/cache.h` from the
  field offsets they touch, but there is no `.c` file for any of them.
- **`cache_io_request_completion_routine @0x443ae0`** is likewise a real function with no
  Ghidra boundary. It is the APC `cache_io_thread_proc_async` installs, and it is *not* the
  same as `cache_io_completion_routine @0x443b00`: it sets the completion flag and clears
  `pending`/`started`, but never runs `completion.procedure`. It has no file; it is declared
  where it is used and recorded in `symbols/agent_phase4_cache.txt`.
- **Callees outside the module** are declared as opaque externs with whatever calling
  convention their call site proves: `bitmap_compute_texture_data_size` (0x5146c0),
  `rasterizer_vertex_buffer_create` (0x524980), `rasterizer_index_buffer_create` (0x525030),
  `FUN_00523fa0` / `FUN_00524100` / `FUN_00524270` / `FUN_005243c0` (D3D texture conversion,
  three *different* conventions), `FUN_00549960` (frame-watchdog pump) and `FUN_00515c30`
  ("no texture available" fallback).
- **Tag fields the engine repurposes at runtime** are used through their `tags.h` padding
  names rather than renamed: `BitmapData::pointer`/`_pad_28`/`_pad_2c` (cache handle, D3D
  texture, staging buffer), `SoundPermutation::samples_pointer`/`_pad_30` (cache handle, page
  address), `GBXModelGeometryPart::triangle_offset_2` and the vertex offset at 0x64
  (rasterizer buffer objects). See `out/phase4/cache_types_notes.md`.
- **Two tag-index reads sign-extend in the machine code** (`movsx` on
  `BitmapData::bitmap_tag_id.index` at 0x004445d2 and on `ModelGeometryPart::shader_index`)
  where `types/tags.h` types the field `uint16_t`. Reproduced with an explicit `(int16_t)`
  cast at the `texture_cache_get` site; identical for any index below 0x8000.

### Misattributed addresses

| addr | Ghidra name | reality |
|---|---|---|
| `0x443410` | `cache_file_read_request` | Not a function. Bytes 0x443410-0x443463 are the `CreateFileA` success tail of `cache_file_open_by_name`, whose true extent is 0x443360-0x443496 (310 bytes, not the 228 Ghidra reports). Folded into `cache_file_open_by_name.c`. |
| `0x444420` | `data_file_read` | Not a function. Tail block of `sound_cache_dump_to_file @0x444240`, mis-split the same way. Folded into `sound_cache_dump_to_file.c`. |
| `0x442d10` | `chimera__on_map_load_client` | A Chimera *signature* label, not a Bungie name. The iterator filter is the literal `"mod2"` and the walk is `GBXModel::geometries -> parts`; rewritten as `model_load_vertex_buffers`. |
| `0x444af0` | `random_range_real` | Belongs to math/random (global LCG on `0x00719cd4`), not cache. |
| `0x444b30` | — | Animation playback (`ModelAnimationsAnimation`), not sound playback as the batch summary claimed. Belongs to whichever module owns first-person animation state. |
| `0x443ae0` | — | A real function Ghidra gave no boundary: see above. |

## Functions

`n` = `UNSURE` marker count in the file. `blam-cc` lists only the non-stack arguments.
Mean rewrite confidence 0.73; 38 of 49 files are at 0.70 or above.

| addr | name | bytes | name conf | rewrite conf | n | blam-cc |
|---|---|---|---|---|---|---|
| `0x442290` | `cache_file_load` | 401 | 0.85 | 0.90 | | |
| `0x442430` | `cache_file_unload` | 121 | 0.80 | 0.80 | | |
| `0x4424b0` | `structure_bsp_load` | 108 | 0.65 | 0.70 | 3 | bsp in EDI |
| `0x442520` | `structure_bsp_dispose` | 39 | 0.70 | 0.75 | | |
| `0x442550` | `tag_lookup` | 114 | 0.80 | 0.85 | | group in EDI |
| `0x4425d0` | `tag_iterator_next` | 96 | 0.80 | 0.75 | 1 | iterator in ESI |
| `0x442640` | `cache_file_request_map` | 212 | 0.55 | 0.60 | 7 | name in ESI |
| `0x442720` | `cache_file_download_poll` | 273 | 0.45 | 0.70 | 2 | — |
| `0x442840` | `data_file_open` | 528 | 0.80 | 0.85 | 1 | |
| `0x442a50` | `data_file_close` | 198 | 0.70 | 0.75 | 1 | |
| `0x442b20` | `cache_io_request_new` | 135 | 0.75 | 0.90 | | completion in ESI |
| `0x442bb0` | `cache_file_exists` | 186 | 0.75 | 0.75 | | name in EAX, header_out in ESI |
| `0x442c70` | `cache_io_read_file_ex_retry` | 109 | 0.35 | 0.80 | 1 | request ESI, size EBX, offset EDX, routine EDI |
| `0x442ce0` | `cache_io_wait_for_flag` | 44 | 0.35 | 0.85 | 1 | flag in ESI |
| `0x442d10` | `model_load_vertex_buffers` | 475 | 0.50 | 0.75 | 4 | header in EAX |
| `0x442f00` | `model_dispose_vertex_buffers` | 266 | 0.50 | 0.45 | 2 | |
| `0x443020` | `structure_bsp_load_material_vertex_buffers` | 384 | 0.55 | 0.70 | 2 | compiled_header in EAX |
| `0x4431a0` | `structure_bsp_dispose_material_vertex_buffers` | 196 | 0.55 | 0.60 | 1 | compiled_header in EAX |
| `0x443270` | `cache_io_request_find_free_slot` | 51 | 0.70 | 0.75 | 2 | |
| `0x4432b0` | `cache_io_wait_all_requests` | 52 | 0.70 | 0.85 | | |
| `0x4432f0` | `cache_file_download_matches` | 106 | 0.30 | 0.55 | | name in EAX |
| `0x443360` | `cache_file_open_by_name` | 310 | 0.45 | 0.70 | | name in EAX |
| `0x4434a0` | `cache_file_download_status_get` | 78 | 0.40 | 0.75 | 2 | progress_out in EAX |
| `0x443510` | `cache_file_download_stop` | 44 | 0.35 | 0.75 | | |
| `0x443540` | `cache_file_download_finish` | 159 | 0.40 | 0.70 | | |
| `0x4435e0` | `cache_file_slot_read_header` | 385 | 0.60 | 0.70 | | slot_index in EAX |
| `0x443770` | `cache_file_find_slot_by_name` | 51 | 0.65 | 0.85 | | filename in EDI |
| `0x4437b0` | `cache_file_find_oldest_slot` | 275 | 0.65 | 0.80 | 1 | slot_category in EAX |
| `0x4438d0` | `cache_io_thread_start` | 107 | 0.70 | 0.85 | | |
| `0x443940` | `cache_io_thread_proc_async` | 188 | 0.60 | 0.75 | 1 | — |
| `0x443a10` | `cache_io_thread_proc_sync` | 194 | 0.60 | 0.85 | | — |
| `0x443b00` | `cache_io_completion_routine` | 41 | 0.75 | 0.90 | | __stdcall APC |
| `0x443b30` | `data_file_read_header` | 100 | 0.80 | 0.70 | 1 | file in ESI |
| `0x443ba0` | `data_file_read_data_block` | 115 | 0.80 | 0.75 | | file in ESI |
| `0x443c20` | `data_file_read_offset_table` | 118 | 0.80 | 0.75 | | file in EDI |
| `0x443ca0` | `sound_cache_new` | 138 | 0.85 | 0.75 | 1 | |
| `0x443d30` | `sound_permutation_release_page` | 38 | 0.45 | 0.65 | 1 | permutation in ESI |
| `0x443d60` | `sound_cache_decode_permutation` | 158 | 0.40 | 0.45 | 2 | permutation in EAX |
| `0x443e10` | `sound_cache_touch` | 284 | 0.60 | 0.60 | 2 | wait_until_loaded in EBX, permutation in EDI |
| `0x443f30` | `sound_cache_dispose` | 148 | 0.85 | 0.65 | | |
| `0x443fd0` | `sound_cache_release_unused` | 134 | 0.75 | 0.70 | | |
| `0x4440e0` | `sound_cache_page_allocate` | 344 | 0.70 | 0.55 | 1 | permutation in EAX |
| `0x444240` | `sound_cache_dump_to_file` | 480 | 0.85 | 0.55 | 2 | |
| `0x4444d0` | `texture_cache_new` | 125 | 0.80 | 0.80 | 1 | |
| `0x444550` | `texture_cache_get` | 424 | 0.75 | 0.70 | 6 | bitmap in EAX |
| `0x444800` | `texture_cache_page_allocate` | 194 | 0.75 | 0.80 | 2 | bitmap in EAX |
| `0x4448d0` | `cache_reserve_map_memory` | 274 | 0.80 | 0.80 | 1 | |
| `0x4449f0` | `predicted_resource_list_touch` | 101 | 0.40 | 0.75 | 1 | resources in ESI |
| `0x444a60` | `sound_tag_touch_permutations` | 134 | 0.40 | 0.60 | | tag in EAX |

Names are recorded in `symbols/agent_phase4_cache.txt`; each file's own header comment carries
the address, the register convention, the evidence and the original Ghidra decompilation in a
trailing `#if 0` block.

## Cleanup pass 1: functions Ghidra never created

Real functions reached only through vtables, dispatch tables or call sites, found by the phase-4
types agents, created in the Ghidra project as `missed_XXXXXX` and rewritten here under Blam
names (symbols in `symbols/agent_phase4_missed.txt`). Register conventions were taken from
objdump of the function and of the table or call site that reaches it.

| Address | Function | Size | Name conf. | Rewrite conf. | UNSURE | Note |
|---|---|---|---|---|---|---|
| `0x443e00` | `cache_io_sound_decode_thunk` | 15 | 0.8 | 0.9 | 0 |  |

Gate: `python tools/build_check.py cache` clean after the cleanup review.
