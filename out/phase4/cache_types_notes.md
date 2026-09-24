# cache module: type recovery notes

Header: `types/cache.h`. Smoke gate: `out/phase4/cache_smoke.c`, checked with
`C:\msys64\ucrt64\bin\gcc.exe -fsyntax-only -std=gnu99 -Wall -I types out/phase4/cache_smoke.c`
(passes; it also parses standalone, with no other header included).

Sources: `out/phase4/cache_functions.md`, `out/phase2/cache/00.md`,
`out/phase2/results/cache_00.json`, `types/tags.h`, `types/memory.h`, full decompiles via
`python tools/pack.py 0xADDR`, and — where Ghidra dropped a register argument or invented a
function boundary — raw disassembly of `bin/halo.exe` with
`objdump -d -M intel --start-address=... --stop-address=...` plus direct reads of `.data`
(PE VA -> file offset: `va - 0x676000 + 0x276000`).

Everything below is evidence, not guesswork, unless it says "unresolved" or "inferred".

## Types NOT redefined: the tag structures already in `types/tags.h`

Most of this module is a thin runtime layer over structures whose layout `types/tags.h` already
carries. In every case the module reads them at exactly the offsets `tags.h` predicts, which is
strong mutual confirmation. Listing them because the alternative — re-deriving them as new
`cache`-module structs — would have duplicated `tags.h` badly.

| Tag struct | Offsets this module touches | Function |
|---|---|---|
| `ScenarioBSP` (0x20) | `bsp_start` 0x00, `bsp_size` 0x04, `bsp_address` 0x08, `structure_bsp.tag_id` 0x1c | `structure_bsp_load` @0x4424b0, `structure_bsp_dispose` @0x442520 |
| `ScenarioStructureBSPCompiledHeader` (0x18) | `pointer` 0x00 | @0x4424b0 stores it into `tag_instance::data`; @0x443020 dereferences it |
| `ScenarioStructureBSP` (0x288) | `lightmaps` reflexive at 0x104/0x108 | @0x443020, @0x4431a0 |
| `ScenarioStructureBSPLightmap` (0x20) | `materials` reflexive at 0x14/0x18 | @0x443020, @0x4431a0 |
| `ScenarioStructureBSPMaterial` (0x100) | `shader.tag_fourcc` 0x00, `rendered_vertices_type` 0xb0, `rendered_vertices_count` 0xb4, `rendered_vertices_index_pointer` 0xc0, `lightmap_vertices_type` 0xc4, `lightmap_vertices_count` 0xc8, `lightmap_vertices_index_pointer` 0xd4, `uncompressed_vertices.pointer` 0xe4 | @0x443020 (create), @0x4431a0 (release) |
| `GBXModel` (0xe8) | `geometries` 0xd0/0xd4, `shaders` 0xdc/0xe0 | @0x442d10, @0x442f00 |
| `GBXModelGeometry` (0x30) | `parts` 0x24/0x28 | same |
| `GBXModelGeometryPart` (0x84) | `shader_index` 0x04, `triangle_buffer_type` 0x44, `triangle_offset_2` 0x50, `vertex_type` 0x54, `vertex_count` 0x58, vertex offset 0x64 | same |
| `ModelShaderReference` (0x20) | `shader.tag_fourcc` 0x00 | @0x442d10 |
| `BitmapData` (0x30) | `type` 0x0a, `flags` 0x0e, `pixel_data_offset` 0x18, `pixel_data_size` 0x1c, `bitmap_tag_id` 0x20, `pointer` 0x24, 0x28, 0x2c | `texture_cache_get` @0x444550, `texture_cache_page_allocate` @0x444800, release proc @0x444730 |
| `SoundPermutation` (0x7c) | `format` 0x28, `samples_pointer` 0x2c, 0x30, `buffer_size` 0x38, `tag_id_1` 0x3c, `samples.size` 0x40, `samples.flags` 0x44, `samples.file_offset` 0x48 | @0x443d60, @0x443e10, @0x4440e0, release proc @0x4440a0 |
| `SoundPitchRange` (0x48) | `actual_permutation_count` 0x2c, permutations reflexive | @0x444a60 |
| `Sound` (0xa4) | `pitch_ranges` 0x98/0x9c | @0x444a60 |
| `PredictedResource` (0x08) | `type` 0x00 (0 = bitmap, 1 = sound) | @0x4449f0 |
| `ModelAnimationsAnimation` (0xb4) | name 0x00, `frame_count` 0x22 | @0x444b30 |

### Tag fields the engine repurposes at runtime

Four `tags.h` fields hold something different once the map is resident. Named in the notes rather
than changed in `tags.h`, since the on-disk meaning is what the tag definitions describe.

- `BitmapData::pointer` (0x24) is the texture cache handle, `-1` when the bitmap is not resident.
  `_pad_28` is the D3D texture object; `_pad_2c` is the GlobalAlloc staging buffer the read fills.
  All three proved by the release procedure at `0x00444730`: it sets 0x24 to -1, `GlobalFree`s
  0x2c, and calls vtable slot +8 on 0x28.
- `BitmapData::flags` bit 7 (`make_it_actually_work`) gates the whole streaming path:
  `texture_cache_get` reads `pointer` directly when it is clear. Bit 8 (`external`) selects
  `bitmaps.map` as the source file.
- `SoundPermutation::samples_pointer` (0x2c) is the sound cache handle, `-1` when not resident,
  and `_pad_30` is the address of the resident page. Proved by the release procedure at
  `0x004440a0`, which sets 0x2c to -1 and 0x30 to 0.
- `GBXModelGeometryPart::triangle_offset_2` (0x50) and the vertex offset at 0x64 hold the
  rasterizer index/vertex buffer objects after `model_load_vertex_buffers` runs;
  `model_dispose_vertex_buffers` @0x442f00 releases them through the same offsets.

## `cache_file_header` (0x800)

- The same five-part validation appears three times, once per reader, and is what fixes the
  layout: `cache_file_load` @0x442290 on the copy at `0x006a8154`, `cache_file_exists` @0x442bb0
  on a stack buffer, `cache_file_slot_read_header` @0x4435e0 on the slot copy. All three check
  `[0] == 0x68656164` ("head"), `[0x1ff] == 0x666f6f74` ("foot"), `-1 < [2] && [2] < 0x18000001`,
  `strlen(&[8]) < 0x20` (spelled `piVar3 - (base + 0x21) < 0x20` after the inline `strlen`), and
  `[1] == 7`. That gives `head` 0x00, `version` 0x04, `file_size` 0x08, `name[32]` 0x20,
  `foot` 0x7fc, and the 0x800 size.
- `tag_data_offset` 0x10 and `tag_data_size` 0x14: `cache_file_load` passes `DAT_006a8164` and
  `DAT_006a8168` (header+0x10, +0x14) to `cache_io_request_new` as the file offset and byte count
  for the read into `DAT_006ac54c` (0x40440000).
- `crc32` 0x64 is established **outside** this module. `DAT_006a81b8` is header+0x64, and the two
  image-wide references to it are both in the game-state save/load code: the validator whose
  Ghidra label is `LAB_00538569` compares a saved header field `header[0x4a]` against it, and the
  writer just above it (`DAT_006e2de0[0x4a] = DAT_006a81b8`) stores it into that slot. So it is a
  per-map value that a saved game pins to.
- **Unresolved: 0x0c, 0x18, 0x1c, 0x40..0x5f, 0x60, 0x62, 0x68..0x7fb.** No function in the image
  reads any of them. The published retail layout puts `build[32]` at 0x40 and a map-type int16 at
  0x60; that is recorded in a comment but the fields are named `unknown_XX`.

## `cache_file_tag_header` (0x28)

- `tags` 0x00 and `scenario_tag` 0x04: `cache_file_load` does `DAT_0087bc14 = *puVar1;` and
  `return DAT_006a8954[1];` where `puVar1 == DAT_006ac54c`, the tag data base.
- `tag_count` 0x0c: both `tag_lookup` and `tag_iterator_next` bound their scan with
  `*(int *)(DAT_006a8954 + 0xc)`.
- `model_data_file_offset` 0x14, `model_index_data_offset` 0x1c, `model_data_size` 0x20:
  `model_load_vertex_buffers` @0x442d10 does `GlobalAlloc(0, header[0x20])`,
  `cache_io_request_new(header[0x14], header[0x20], buffer, 1, 0)`, then
  `index_base = header[0x1c] + buffer`.
- **Unresolved: 0x08, 0x10, 0x18, 0x24.** Never touched. The published layout has a checksum at
  0x08, two model part counts at 0x10 / 0x18 and a `"tags"` signature at 0x24; the signature is
  also the only thing that fixes the size at 0x28, so **the 0x28 total is inferred**, not proved
  by this module. The one in-module consistency check is that the tag array begins immediately
  after the header in the mapped block.

## `tag_instance` (0x20)

- Stride 0x20 from `index * 0x20 + DAT_0087bc14` in ten functions.
- 0x00/0x04/0x08 are the three group ids: `tag_iterator_next` matches its single filter against
  `piVar4[0]`, `piVar4[1]` and `piVar4[2]`; `tag_lookup` matches only `piVar4[0]`.
- `tag_id` 0x0c: the return value of both `tag_lookup` (`*(sVar3 * 0x20 + 0xc + base)`) and
  `tag_iterator_next` (`piVar4[3]`).
- `path` 0x10: `__stricmp(param_1, (char *)piVar2[4])` in `tag_lookup`, and printed with `%s` in
  both `sound_cache_dump_to_file` and the console spew in `texture_cache_get`.
- `data` 0x14: every consumer. Written by `structure_bsp_load` / `structure_bsp_dispose`.
- **Unresolved: 0x18, 0x1c.** Untouched by the whole image as far as this module can see.

## `tag_iterator` (0x14)

- Both callers were read from raw disassembly because Ghidra renders the setup as loose locals.
  `model_dispose_vertex_buffers` @0x442f00: `lea esi,[esp+0x14]`, `mov WORD [esp+0x18],bp` (zero),
  `mov DWORD [esp+0x24],0x6d6f6432`. `model_load_vertex_buffers` @0x442d10: `lea esi,[esp+0x28]`,
  `mov WORD [esp+0x2c],di` (zero), `mov DWORD [esp+0x38],0x6d6f6432`. Both put the index at
  base+0x04 and the group filter at base+0x10, and both reserve 0x14 bytes.
- `tag_iterator_next` reads exactly those two fields; a filter of `-1` matches every tag.
- **Unresolved: 0x00, 0x06, 0x08, 0x0c.** Neither caller initializes them, so a fuller
  `tag_iterator_new` must exist outside this range (the four other callers image-wide are in the
  `game`/`cheats` code and were not read). Size 0x14 is from the stack reservation, so the real
  struct could be longer in a caller that fills the rest.

## `cache_file_slot` (0x80c), array of 6 at 0x006a9428

- Stride 0x80c from `&DAT_006a9428 + index * 0x80c` in seven functions;
  `cache_file_unload` zero-fills `0x203` dwords, i.e. exactly 0x80c bytes.
- 0x00 is the `HANDLE`: `CloseHandle(*(HANDLE *)(&DAT_006a9428 + i*0x80c))`, and
  `cache_file_download_status` @0x4434a0 pokes it to `-1` on a download reset.
- 0x04 is a `FILETIME`: `SystemTimeToFileTime(&local, &DAT_006a942c + i*0x80c)` then
  `SetFileTime` in `cache_file_download_finish` @0x443540, and
  `CompareFileTime((FILETIME *)(slot + 4), ...)` in `cache_file_find_oldest_slot`. Zeroed through
  an `LPFILETIME` (`dwLowDateTime` / `dwHighDateTime`) when the header fails validation.
- 0x0c is the header: `DAT_006a9434` is slot+0x0c, `DAT_006a9438` is header+0x04 (version),
  `DAT_006a943c` is header+0x08, `DAT_006a9454` is header+0x20 (the name
  `cache_file_find_slot_by_name` compares), `DAT_006a9c30` is header+0x7fc (the footer). The
  0x200-dword zero fills in `cache_file_open_by_name` and `cache_file_slot_read_header` cover
  exactly the 0x800-byte header.
- Slot count 6 from `while (sVar2 < 6)` in `cache_file_find_slot_by_name`.
- The per-slot size limits in `cache_file_find_oldest_slot` decompile as
  `slot < 2 ? 0x18000000 : ((2 < slot) - 1 & 0xfa300000) + 0x8000000`, which is 0x18000000 for
  slots 0-1, 0x02300000 for slot 2 (the wrap of `0xfa300000 + 0x08000000`), and 0x08000000 for
  slots 3-5. Category 0 scans slots 0..1, category 1 scans 3..5, category 2 is slot 2 only. The
  category *names* in `cache_file_slot_category` are inferred from those ranges and sizes.

## `cache_io_request` (0x30) and `cache_io_completion` (0x0c)

- 0x200 entries: `cache_io_request_find_free_slot` and `cache_io_wait_all_requests` both loop 0x200
  times at stride 0x30, and `data_file_open` does `GlobalAlloc(0, 0x6000)` == `0x200 * 0x30`.
- The first 0x14 bytes are a Win32 `OVERLAPPED`. `cache_io_request_new` zeroes `puVar2[0..4]` then
  writes the file offset into `puVar2[2]` (0x08, `OVERLAPPED::Offset`), and the async worker hands
  the request itself to `ReadFileEx`. `cache_io_completion_routine` @0x443b00 is the APC and reads
  the embedded completion record back at `overlapped + 0x24` / `+ 0x28`, which is only coherent if
  the request *is* the OVERLAPPED.
- `size` 0x14 and `destination` 0x18: `ReadFile(hFile, *(LPVOID *)(req + 0x18),
  *(DWORD *)(req + 0x14), ...)` in `cache_io_thread_proc_sync`, preceded by
  `SetFilePointer(hFile, *(LONG *)(req + 8), 0, 0)`.
- `priority` 0x1c, `pending` 0x1d, `started` 0x1e: the worker scan tests `req[0x1d] != 0 &&
  req[0x1e] == 0` and then prefers the candidate with the lower `(req[0x1c], req[0x08])` pair.
  `cache_io_request_new` sets `pending = 1`, `started = 0`. Both blocking waiters raise
  `priority` in place afterwards: `sound_cache_touch` does
  `*(index * 0x30 + 0x1c + DAT_006ac4a0) = 1`, `texture_cache_get` the same.
- `data_file_index` 0x20: `req[0x20]` selects the handle in both worker procedures — 0 keeps the
  active map slot handle, 1 reads `&DAT_006ac4e8` (bitmaps), 2 reads `&DAT_006ac4a8` (sounds), and
  in both cases takes field `[0xf]`, i.e. `data_file + 0x3c`.
- **`cache_io_completion` was recovered from disassembly**, because Ghidra models
  `cache_io_request_new`'s third source as an unmodelled `unaff_ESI`. At `0x004441de..0x00444211`
  in `sound_cache_page_allocate`: `lea eax,[edi+2]` / `mov [esp+0x18],eax` (the flag pointer,
  `&entry->loaded`), `mov [esp+0x20],ecx` (the procedure), `mov [esp+0x30],edi` (the entry), and
  `lea esi,[esp+0x28]` resolving to the base of those three slots. `texture_cache_page_allocate`
  does the same at `0x00444879` with `{&entry->loaded, 0, 0}`.
- The procedure value is computed branchlessly:
  `mov cx,[esi+0x28]; dec cx; neg cx; sbb ecx,ecx; and ecx,0x443e00` — i.e. the callback is
  `0x00443e00` for every `SoundPermutation::format` except `xbox_adpcm`, and NULL for that one.
- `0x00443e00` is a 15-byte thunk Ghidra never made a function:
  `mov eax,[esp+4]; mov ecx,[eax+8]; mov eax,[ecx+0xc]; jmp 0x443d60`. It takes the completion
  record, walks `record->data` (the `sound_cache_entry`) to `entry->permutation`, and tail-calls
  the decoder. That is what pins `cache_io_completion::data` to +0x08.
- **Unresolved: 0x1f, 0x21..0x23.** Padding around the two byte fields; never written.

## `data_file` (0x40) and `data_file_reference` (0x0c)

- Two instances, both zero-filled 0x40 bytes by `data_file_open` @0x442840:
  `sounds_data_file` at 0x006ac4a8 and `bitmaps_data_file` at 0x006ac4e8. They are adjacent
  (`0x6ac4a8 + 0x40 == 0x6ac4e8`) but are separate named globals, since the worker maps the index
  to a pointer with an explicit if-chain rather than indexing.
- The on-disk header is the first 0x10 bytes: `data_file_read_header` @0x443b30 does
  `ReadFile(handle, base, 0x10, ...)` and then `if (*base != expected_id)` with 1 passed for
  bitmaps and 2 for sounds, zeroing `[0..3]` on mismatch.
- `data_file_read_data_block` @0x443ba0: seeks to `+0x04`, allocates `[0x08] - [0x04]` bytes into
  `+0x20`, and on success writes that size to both `+0x1c` and `+0x18`. The duplicate is in the
  original; `data_size` / `data_capacity` are the names given here.
- `data_file_read_offset_table` @0x443c20: seeks to `+0x08`, allocates `[0x0c] * 0xc` into `+0x10`
  and copies `[0x0c]` to `+0x14`. The `* 0xc` is the only evidence for the 0x0c reference stride.
- `name` 0x38: `_DAT_006ac520 = "bitmaps"` (0x6ac4e8+0x38) and `_DAT_006ac4e0 = "sounds"`
  (0x6ac4a8+0x38); it is the `%s` in the three `data file %s` error strings.
- `file` 0x3c: `DAT_006ac524` and `DAT_006ac4e4`, both +0x3c, and the `[0xf]` the worker reads.
- **Unresolved: 0x24 (explicitly zeroed by `data_file_open`, never read again) and 0x28..0x37.**
- **Unresolved: the whole `data_file_reference` layout.** Nothing in this module indexes the
  table; consumers live in the bitmap/sound modules. Only the 0x0c stride is established.

## `sound_cache_entry` (0x10) and `texture_cache_entry` (0x10)

Both are datums of a `data_array` whose `cache` (from `types/memory.h`) rations the backing
region. The two constructors confirm `types/memory.h` byte-for-byte:

- `sound_cache_new` @0x443ca0: `data_new("pc sound", 0x200)` then
  `GlobalAlloc(0, 0x387c)` == `0x7c + 0x200 * 0x1c` == `sizeof(cache) + maximum_count *
  sizeof(cache_entry)`, then `cache_new(that, page_count, 0xc, 0x200, 0x4440a0, 0x444060)`.
  Page count is `(sound_cache_size_megabytes << 20) >> 12`, i.e. 4096-byte pages.
- `texture_cache_new` @0x4444d0: `data_new("pc texture", 0x1000)` then
  `GlobalAlloc(0, 0x1c07c)` == `0x7c + 0x1000 * 0x1c`, then
  `cache_new(that, 0x1000, 2, 0x1000, 0x444730, 0x444700)`. Note `block_shift == 2`: the texture
  cache hands out 4-byte blocks (`cache_allocate_block(texture_cache, 4)`) over a 0x4000-byte
  `VirtualAlloc`, so it rations *slots*, not pixel storage; the pixels go in a per-bitmap
  `GlobalAlloc` at `BitmapData + 0x2c`.
- Element size 0x10 for both, from `(datum & 0xffff) * 0x10 + entries->data`.

`sound_cache_entry` fields, all from code that Ghidra did render:

- +0x02 `loaded` — the `cache_io_completion::flag` target (`lea eax,[edi+2]` above), polled by
  `sound_cache_touch` @0x443e10.
- +0x03 `decoded` — set once, guarding the call to the decoder.
- +0x05 `lock_count`, +0x06 `playing` — the in-use predicate at `0x00444060` returns "busy" when
  `!loaded || lock_count || playing`, and `sound_cache_release_unused` @0x443fd0 evicts exactly
  the entries with `entry[5] == 0 && entry[6] == 0`.
- +0x08 `io_request_index` — `*(short *)(entry + 8) * 0x30 + 0x1c + cache_io_requests` is how the
  waiter raises the request priority.
- +0x0c `permutation` — written by `sound_cache_page_allocate`, read by the release procedure.
- **Unresolved: 0x04, 0x07, 0x0a.**

`texture_cache_entry` has the two roles at swapped offsets: request index at +0x02, completion
flag at +0x04, `converted` at +0x05, `bitmap` at +0x08, `texture` at +0x0c. Read out of the
release procedure at `0x00444730` (`mov al,[ecx+esi*1+4]` spin, then `mov eax,[edi+8]`) and the
in-use predicate at `0x00444700` (`in_use == (loaded == 0)`), plus the writes in
`texture_cache_page_allocate`. `texture_cache_get` returns `&entry->texture`, so its 25 callers
hold a `void **`. **Unresolved: 0x06, 0x07.**

## `map_download_state` (0xac8) — foreign object, partial

- `PTR_DAT_006869c0` resolves to `0x006a8960`: the four bytes at file offset `0x2869c0` are
  `60 89 6a 00`. Every field offset in the four download functions then lands on the globals
  Ghidra listed for them (`+0x110 -> 0x6a8a70`, `+0x908 -> 0x6a9268`, `+0x954..0x960 ->
  0x6a930c..0x6a9318`, `+0x98c -> 0x6a92ec`, `+0xaa4 -> 0x6a9404`).
- Size 0xac8 is **inferred**: `0x6a8960 + 0xac8 == 0x6a9428`, exactly where `cache_file_slots`
  begins, and nothing in the image references any address inside that span directly (a grep for
  `DAT_006a89xx` / `DAT_006a9[0-3]xx` over `out/halo_decompiled.c` returns nothing).
- Known fields: `queued_file_count` 0x110 (`< 1` reports idle), `status_flags` 0x908 (nonzero
  ends the download, bit 1 means cancelled), four `HANDLE`s at 0x954/0x958/0x95c/0x960,
  `thread_busy` 0x98c, `progress` 0xaa4 (clamped to `[0, 1]` by the poller).
- **Everything else is unresolved**, filled with `unknown_XXX[n]` byte arrays. The object is not
  owned by this module: only the four glue functions listed under "misattributed" touch it.

## Globals

The block from 0x006a8150 to 0x006ac554 is contiguous and chains exactly — 0x006a8154 + 0x800 =
0x006a8954, 0x006a8960 + 0xac8 = 0x006a9428, 0x006a9428 + 6*0x80c = 0x006ac470, and 0x006ac470 +
1 + 2 + 0x20 = 0x006ac494. The download name buffer at 0x006ac474 is therefore exactly 0x20
bytes, matching the header name length. Full list in the trailing comment block of
`types/cache.h`.

Two gaps in that block, 0x006ac4a4 and 0x006ac544, are never touched.

Globals read but **not** owned here are listed separately in the header: the D3D device
(`0x0071d174`), the rasterizer vertex-size table (`0x0065de00`), the OS platform id
(`0x00721ef0`), the profile directory (`0x006ac900`, written by the shell startup code), the map
path prefix (`0x006f16d8`), the frame watchdog (`0x0072520c`) and the shell fatal-error argument
(`0x00722bbc`).

## Misattributed functions and wrong one-line summaries

1. **`data_file_read` @0x444420 is not a function.** `sound_cache_dump_to_file` starts at
   0x444240 with size 480, i.e. it ends at 0x444420, and the last instruction before that
   boundary (`0x0044441c: mov [esp+0x4c],eax`) falls straight through with no `ret`. Ghidra split
   the tail block of the dump routine into a second "function" and named it `data_file_read`;
   that is why its decompile is nonsense (`in_EAX ^ 0x69746572`, stack args out of nowhere) and
   why `sound_cache_dump_to_file`'s own decompile already contains the `_fclose` / `GlobalFree`
   that supposedly live in `data_file_read`. **No types taken from it.** The name is also wrong
   on its face: it never touches a `data_file`.
2. **`chimera__on_map_load_client` @0x442d10 and `FUN_00442f00` are model code, not BSP code.**
   Both `cache_functions.md` summaries say "walks every structure_bsp cluster". They do not: the
   iterator filter is the literal `0x6d6f6432` (`"mod2"`), and the walk is
   `GBXModel::geometries -> GBXModelGeometry::parts` at strides 0x30 and 0x84, with shader group
   ids fetched through `GBXModel::shaders` at stride 0x20. Reasonable names would be
   `model_load_vertex_buffers` / `model_dispose_vertex_buffers`. The `chimera__` prefix is a
   Chimera-derived label, not a Bungie symbol.
   The BSP counterparts are the *other* pair, `FUN_00443020` / `FUN_004431a0`, which do walk
   `ScenarioStructureBSP::lightmaps -> materials`; the summary for `0x4431a0` calls those
   "clusters", but 0x104/0x108 is `lightmaps`, and `ScenarioStructureBSPCluster` (0x68) is never
   touched by this module.
3. **`random_range_real` @0x444af0 belongs to the math/random module.** It is the global LCG
   (`seed = seed * 0x19660d + 0x3c6ef35f`) on `0x00719cd4`, with the `1.5259022e-05` constant at
   `0x00672b84`. `modules.json` already assigns the identical inlined body at `0x401050` to
   `math`, and `types/math.h` already owns the performance-counter frequency at `0x006ac8f8` that
   the neighbouring cache functions use. No cache type comes from it.
4. **`FUN_00444b30` is animation playback, not sound playback.** The `cache_functions.md` summary
   says "starts a sound permutation playing". It walks a tag with an int32 at +0x68 equal to 1 and
   a reflexive at +0x74/+0x78 of stride **0xb4**, matches a name with `__stricmp` at block+0x00,
   and computes `frame_count / 30.0` from the int16 at block+0x22 — that is
   `ModelAnimationsAnimation`, not `SoundPermutation`. The globals it writes
   (0x006869d1..0x006869d8, 0x00686a00..0x00686a0c) are a single first-person animation playback
   record and belong to whichever module owns that state, not to cache. **No types taken from it.**
5. **Four functions are multiplayer-download glue over a foreign object**: `FUN_00442640`,
   `FUN_00442720`, `FUN_00443510`, `FUN_00443540`. They sit in the cache address run and are
   reached from the cache slot code, so the header keeps a partial `map_download_state`, but the
   object is constructed and filled elsewhere (see above).
6. **`FUN_00442c70` and `FUN_00442ce0` are Win32 retry helpers**, not cache types. The first
   re-arms an `OVERLAPPED` and spins on a `ReadFileEx`-shaped function pointer with
   `SleepEx(0, TRUE)`; the second is an alertable wait on a completion byte. `FUN_00442c70`
   zeroing `unaff_ESI[0..4]` is just the OVERLAPPED reset, already covered by
   `cache_io_request`.
7. **Four small procedures in the range are not in `cache_functions.md`** because Ghidra left them
   as `LAB_`: `0x00444060` (sound cache in-use predicate), `0x004440a0` (sound cache release),
   `0x00444700` (texture cache in-use predicate), `0x00444730` (texture cache release), plus the
   15-byte thunk at `0x00443e00`. All five were disassembled for this header and carry the best
   evidence for the two entry structs.

## One observation for another module

The tail of `sound_cache_dump_to_file` builds its `data_iterator` on the stack and writes **four**
fields, not three:

```
444417: mov  eax, ds:0x6ac528      ; sound_cache_entries, a data_array *
44441c: mov  [esp+0x4c], eax
444420: xor  eax, 0x69746572       ; ^ "iter"
444425: add  esp, 0x20             ; [esp+0x4c] is now [esp+0x2c]
444428: lea  edi, [esp+0x2c]       ; the iterator
44442c: mov  WORD [esp+0x30], 0    ; iterator+0x04
444433: mov  [esp+0x34], ebp       ; iterator+0x08
444437: mov  [esp+0x38], eax       ; iterator+0x0c = data_array ^ "iter"
44443b: call data_iterator_next
```

So iterator+0x00 is the `data_array *` and iterator+0x04 / +0x08 match `types/memory.h`, but there
is a fourth word at +0x0c holding the array pointer XORed with the literal `"iter"` — a
self-check cookie. That makes `data_iterator` 0x10 bytes, not 0x0c. Not changed here; flagged for
the memory module.
