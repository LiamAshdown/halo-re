# `memory` — Blam memory, serialization and container primitives

Retail Halo PC `halo.exe` 1.0.10, `0x4cf810 .. 0x4d2370` (10,409 bytes of code, 59 functions),
plain C / MSVC 7.1 / x86. Every file in this directory is one function, rewritten from its Ghidra
decompilation against `types/memory.h`, with the original decompile preserved verbatim at the
bottom of the file inside `#if 0 ... #endif` for diffing.

Gate: `python tools/build_check.py memory` → **59 ok, 0 failed**.

## What the module contains

Seven independent families that happen to be adjacent in the image. Nothing here touches the tag
system, the game state, or rendering; these are the leaf utilities everything else is built on.

| Family | Range | What it is |
|---|---|---|
| `growable_array` | `0x4cf810`–`0x4cf8ef` | `GlobalAlloc`-backed vector of fixed-size elements |
| `bit_stream` | `0x4cf8f0`–`0x4cfd8f` | LSB-first bit reader/writer with absolute `[first_bit, last_bit]` bounds |
| byte swapping | `0x4cfd90`–`0x4d016f` | endian conversion driven by `byte_swap_definition` code tables |
| `circular_buffer`, `crc32` | `0x4d0170`–`0x4d036f` | ring buffer (`circ`), standard CRC-32 (`0xedb88320`) |
| `data_array` / datums | `0x4d0370`–`0x4d06ff` | the Blam handle-table container; datum handles are `salt<<16 \| index` |
| `byte_stream`, `struct_definition`, `data_packet_group` | `0x4d0700`–`0x4d174f` | the versioned wire-serialization stack used by network game messages |
| `cache`, `memory_pool`, `heap` | `0x4d1750`–`0x4d2370` | three unrelated allocators: LRU block cache, compactable arena, address-ordered heap with a slot table |

The serialization stack layers like this:

```
data_packet_group_{encode,decode}_packet      header: [type][optional version]
  data_packet_group_{encode,decode}_packet_body
    struct_definition_compute_size            caches each field's encoded size once
    struct_definition_{encode,decode}         walks the field list, recursing on struct arrays
      byte_stream_{read,write}_*              bounds-checked cursor over a flat buffer
      byte_swap_array                         big-endian on the wire
```

## Struct layouts

All of these live in `types/memory.h`; the tables below are the summary. Offsets are byte offsets
from the struct base. `#pragma pack(push,1)` is in force.

### `datum_index` (typedef `uint32_t`)

| Bits | Meaning |
|---|---|
| 0..15 | index into the `data_array` |
| 16..31 | identifier / salt; `0` means "empty slot", and `0` in a *handle* is a wildcard that matches any live slot |

`k_datum_index_none` = `0xffffffff`. `next_identifier` reseeds to `0x8000` on overflow.

### `growable_array` — size `0x0c`

| Off | Type | Field |
|---|---|---|
| `0x00` | `int32_t` | `element_size` |
| `0x04` | `int32_t` | `count` (add refuses at `0x7fffffff`) |
| `0x08` | `void *` | `data` (NULL when `count == 0`) |

### `bit_stream` — size `0x18`

| Off | Type | Field |
|---|---|---|
| `0x00` | `uint32_t` | `unknown_00` — never read or written by this module |
| `0x04` | `uint8_t *` | `data` |
| `0x08` | `uint32_t` | `first_bit` (inclusive, absolute bit index) |
| `0x0c` | `uint32_t` | `byte_cursor` |
| `0x10` | `uint32_t` | `bit_cursor` (0..7) |
| `0x14` | `uint32_t` | `last_bit` (inclusive) |

The cursor is committed only when `byte_cursor*8 + bit_cursor` stays inside `[first_bit, last_bit]`
or is exactly `last_bit + 1`. Mask tables: `bit_mask_clear[8]` at `0x0065c2b4`
(`(uint8_t)(0xff << i)`) and `bit_mask_keep[9]` at `0x0065c2c0` (`(uint8_t)((1 << i) - 1)`, entry 8
= `0xff`; `write_bits` indexes it with a negative offset from entry 8).

### `byte_stream` — size `0x10`

| Off | Type | Field |
|---|---|---|
| `0x00` | `uint8_t *` | `data` |
| `0x04` | `int32_t` | `cursor` |
| `0x08` | `int32_t` | `size` |
| `0x0c` | `uint8_t` | `overflow` — sticky; once set, every later field short-circuits |

### `circular_buffer` — size `0x18` + `capacity` bytes of storage

| Off | Type | Field |
|---|---|---|
| `0x00` | `char *` | `name` (not copied) |
| `0x04` | `uint32_t` | `signature`, ASCII `circ` |
| `0x08` | `int32_t` | `read_cursor` |
| `0x0c` | `int32_t` | `write_cursor` |
| `0x10` | `int32_t` | `capacity` = requested size + 1 (one slot kept empty) |
| `0x14` | `uint8_t *` | `data` == `this + 0x18` |

### `data_array` — size `0x38`, allocation is `0x38 + maximum_count*size`

| Off | Type | Field |
|---|---|---|
| `0x00` | `char[32]` | `name` (`strncpy` of 31 chars + NUL) |
| `0x20` | `int16_t` | `maximum_count` |
| `0x22` | `int16_t` | `size` (element size) |
| `0x24` | `uint8_t` | `valid` (`data_new` leaves 0; `cache_new` sets 1) |
| `0x28` | `uint32_t` | `signature`, ASCII `d@t@` |
| `0x2c` | `int16_t` | `next_index` (first index `datum_new` tries) |
| `0x2e` | `int16_t` | `last_index` (high-water mark, one past the last live slot) |
| `0x30` | `int16_t` | `actual_count` |
| `0x32` | `int16_t` | `next_identifier` (salt counter) |
| `0x34` | `void *` | `data` |

Every element starts with `datum_header { int16_t identifier; }`; identifier `0` means free.
Layout confirmed by the `data_array_header` byte-swap definition at `0x0068e39c` (codes at
`0x0068e364`, declared size 56). `data_iterator` is `{ data_array *data; int32_t next_index;
datum_index index; }`, size `0x0c`.

### `struct_definition` — size `0x14`; `struct_definition_field` — size `0x0a`

| Off | Type | Field (`struct_definition`) |
|---|---|---|
| `0x00` | `char *` | `name` |
| `0x04` | `int32_t` | `unknown_04` — zero in every instance in the image |
| `0x08` | `int16_t` | `size` (encoded size in bytes) |
| `0x0a` | `int16_t` | `version` (written as a byte when > 0) |
| `0x0c` | `struct_definition_field *` | `fields` |
| `0x10` | `uint8_t` | `size_computed` |

| Off | Type | Field (`struct_definition_field`) |
|---|---|---|
| `0x00` | `int16_t` | `type` (see below) |
| `0x02` | `int16_t` | `count` |
| `0x04` | `int16_t` | `minimum_version` |
| `0x06` | `int16_t` | `maximum_version` (0 = no upper bound) |
| `0x08` | `int16_t` | `computed_size` — filled in by `struct_definition_compute_size` |

Field types: 0 `unused`, 1 `data`, 2 `int16_array`, 3 `int32_array`, 4 `int64_array`, 5 `string`
(count chars + NUL), 6 `variable_data` (int16 count then that many bytes), 7 `struct_array`
(int16 count then that many sub-structures; the sub-structure's fields follow **inline** at
`field + 1`), 8 `block` (count raw bytes, never swapped), 9 `terminator`.

A field applies when `minimum_version <= version && (version <= maximum_version ||
maximum_version == 0)`; otherwise the encoder still reserves and zeroes its wire bytes and the
decoder zeroes the destination, so offsets stay aligned across versions.

**Field-record stride.** The field list is a flat array of 5-`int16` records. Ghidra renders the
"skip past a nested struct's fields" step as `psVar7 + count * 5` on a `short *`, which is
`field + count` on a `struct_definition_field *`. Getting that wrong by a factor of 5 is the
easiest mistake to make in this family.

### `byte_swap_definition` — size `0x14`

| Off | Type | Field |
|---|---|---|
| `0x00` | `char *` | `name` |
| `0x04` | `int32_t` | `size` |
| `0x08` | `int32_t *` | `codes` |
| `0x0c` | `uint32_t` | `signature`, ASCII `bysw` |
| `0x10` | `int32_t` | `unknown_10` — zero in every instance |

`codes[0]` is `_byte_swap_begin_struct`, `codes[1]` is the number of structure records, field codes
start at `codes[2]`: `n > 0` skip n bytes; `-2`/`-4`/`-8` swap a 16/32/64-bit value; `-100` begin a
nested record (count in the next int32); `-101` end of record; `-102` the next int32 is a
`byte_swap_definition *` to recurse into. A code of `0`, or any negative code not in that set,
spins forever — the interpreter only advances on `code > 0` or a recognized code. Instances:
`packet_header` at `0x00696780` (codes `0x00696770`), `data_array_header` at `0x0068e39c`
(codes `0x0068e364`).

### `data_packet_group` — size `0x30`; `data_packet_type` — size `0x08`

| Off | Type | Field (`data_packet_group`) |
|---|---|---|
| `0x00` | `char *` | `name` |
| `0x04` | `int16_t` | `type_count` (39 for network game messages) |
| `0x06` | `int16_t` | `class_count` (8; classes seen are 0..7) |
| `0x08` | `int32_t` | `maximum_decoded_size` (`0x600`) |
| `0x0c` | `int32_t` | `maximum_encoded_size` (`0x800`) |
| `0x10` | `data_packet_type *` | `types` |
| `0x14`..`0x2c` | `int32_t` | `unknown_14`=0, `unknown_18`=`0x35`, `unknown_1c`..`unknown_28`=`0xffffffff`, `unknown_2c`=`0x10` |

`data_packet_type` is `{ int16_t packet_class; int16_t pad; struct_definition *definition; }`;
a NULL `definition` means a body-less packet. Globals: group at `0x006994f8`, types at
`0x006993c0`, `char *data_packet_group_error` at `0x006b7f00` (last failure string, NULL on
success). A packet is `[1 byte type][optional 1 byte version][body]`, and the type byte is run
through the `packet_header` byte-swap definition before use.

### `cache` — size `0x7c` + `maximum_count*0x1c`; `cache_entry` — size `0x1c`

| Off | Type | Field (`cache`) |
|---|---|---|
| `0x00` | `char[32]` | `name` |
| `0x20` | `void *` | `release_procedure` — called by `cache_evict_entry` before the datum is freed |
| `0x24` | `void *` | `in_use_procedure` — predicate; nonzero means "must not evict" |
| `0x28` | `int32_t` | `block_count` (region capacity, in blocks) |
| `0x2c` | `int32_t` | `block_shift` (log2 of block size in bytes) |
| `0x30` | `uint32_t` | `age` (monotonic use counter, starts at 1) |
| `0x34` | `datum_index` | `first` (head of the by-offset entry list) |
| `0x38` | `datum_index` | `last` |
| `0x3c` | `data_array *` | `entries` == `&entry_data` |
| `0x40` | `uint32_t` | `signature`, ASCII `weee` |
| `0x44` | `data_array` | `entry_data` (element size `0x1c`, storage starts at `0x7c`) |

| Off | Type | Field (`cache_entry`) |
|---|---|---|
| `0x00` | `int16_t` | `identifier` (the `datum_header`) |
| `0x04` | `int32_t` | `size` (in blocks) |
| `0x08` | `int32_t` | `offset` (in blocks from the start of the region) |
| `0x0c` | `datum_index` | `next` (by offset) |
| `0x10` | `datum_index` | `previous` |
| `0x14` | `uint32_t` | `age` (`cache->age` when last touched) |
| `0x18` | `uint32_t` | `unknown_18` — never written by this module |

The address of the backing region is **not** stored in `cache`; each user keeps it separately
(e.g. the sound cache keeps it in `DAT_006ac52c`). `cache_allocate_block` walks a 256-slot ring of
`cache_allocation_gap { datum_index previous_entry; uint32_t newest_age; int32_t offset; int32_t
size; }` (size `0x10`) on the stack. `cache_build_status_bitmap` emits per-block flags: 1
allocated, 2 current (`age == cache->age`), 4 stale (`age + 30 < cache->age`), 8 locked.

### `memory_pool` — size `0x38` + storage; `memory_pool_block` — header `0x18`

| Off | Type | Field (`memory_pool`) |
|---|---|---|
| `0x00` | `uint32_t` | `signature`, ASCII `pool` |
| `0x04` | `char[32]` | `name` |
| `0x24` | `void *` | `base` == `this + 0x38` |
| `0x28` | `int32_t` | `size` |
| `0x2c` | `int32_t` | `free_bytes` |
| `0x30` | `memory_pool_block *` | `first_block` (lowest address) |
| `0x34` | `memory_pool_block *` | `last_block` |

| Off | Type | Field (`memory_pool_block`) |
|---|---|---|
| `0x00` | `uint32_t` | `head_signature`, ASCII `head` |
| `0x04` | `int32_t` | `size` (whole block incl. header, rounded up to 4) |
| `0x08` | `void **` | `address` — back pointer to the *owner's* pointer variable, so compaction can relocate the block |
| `0x0c` | `memory_pool_block *` | `next` (higher address) |
| `0x10` | `memory_pool_block *` | `previous` |
| `0x14` | `uint32_t` | `tail_signature`, ASCII `tail` |

Payload follows at `+0x18`. Global `memory_pool *object_memory_pool` at `0x006b8cb4`.

### `heap` — size `0x34 + maximum_blocks*4`; `heap_block` — header `0x10`

| Off | Type | Field (`heap`) |
|---|---|---|
| `0x00` | `uint32_t` | `unknown_00` — never touched |
| `0x04` | `uint8_t *` | `base` |
| `0x08` | `int32_t` | `size` |
| `0x0c` | `int32_t` | `maximum_blocks` |
| `0x10` | `int32_t` | `next_free_slot` (cached; `-1` means unknown or full) |
| `0x14` | `int32_t` | `bytes_allocated` (headers included) |
| `0x18` | `int32_t` | `peak_bytes_allocated` |
| `0x1c` | `int32_t` | `allocation_count` |
| `0x20` | `int32_t` | `peak_allocation_count` |
| `0x24` | `int32_t` | `peak_allocation_size` |
| `0x28` | `uint8_t` | `compaction_disabled` |
| `0x2c` | `heap_block *` | `first_block` (lowest address) |
| `0x30` | `heap_block *` | `last_block` |
| `0x34` | `heap_block *[]` | `blocks[maximum_blocks]`, a NULL slot is free |

| Off | Type | Field (`heap_block`) |
|---|---|---|
| `0x00` | `uint32_t` | `size` — total incl. header; **bit 31 set while in use**, mask with `k_heap_block_size_mask` (`0x7fffffff`) |
| `0x04` | `int32_t` | `slot` (index into `heap::blocks`) |
| `0x08` | `heap_block *` | `previous` (lower address) |
| `0x0c` | `heap_block *` | `next` (higher address) |

Payload follows at `+0x10`; `heap_allocate_raw` rounds `size + 0x10` up to a multiple of 4. The
trailing table has one slot per live block and a NULL slot means free, so it is an allocation slot
table, **not** size buckets — several earlier one-line summaries call it "bucketed"; that is wrong.

## Known gaps

1. **`0x4d3980` and `0x4d3a30` are not in this module.** `out/phase4/memory_functions.md` lists
   them as `data_delete_all` / `data_next_index`, but they are zlib `trees.c` code (`send_tree`
   and `compress_block`). `symbols/functions.txt` already carries `deflate_compress_block` for
   `0x4d3a30`; the `data_delete_all` label on `0x4d3980` comes from `opensauce-ce-exact-entry`
   and collides by accident with the real `data_delete_all` at `0x4d0580`. No files were written
   for them, so the module's true last function is `0x4d2370`.
2. **No `memory_pool` or `heap` constructor here.** The pool is built by `game_state_new_pool` at
   `0x00538150`, outside this range; `block_list_*` only operate on an already-initialized pool.
   Likewise nothing in this range initializes `heap::base` / `size` / `blocks`.
3. **Unused or unknown fields**, all confirmed untouched by every function in the range:
   `bit_stream::unknown_00`, `struct_definition::unknown_04`, `byte_swap_definition::unknown_10`,
   `cache_entry::unknown_18`, `heap::unknown_00`, and `data_packet_group::unknown_14`..`unknown_2c`.
4. **Elided callee arguments.** Ghidra renders several calls with no argument list at all
   (`FUN_004d0700();`, `struct_definition_encode();`, `struct_definition_decode();`,
   `byte_swap_array();`, `data_iterator_next();`) because the callee reads registers the caller
   holds live across its whole body. Those argument lists are reconstructed from the callee's own
   register convention and marked `// UNSURE:` at each site. 43 such markers remain module-wide,
   concentrated in the `data_packet_group` / `struct_definition` cluster and `cache_allocate_block`.
5. **`bit_stream_write_bits_chunked` (`0x4cf8f0`) never advances its value between chunks.** The
   read counterpart (`0x4cf950`) visibly advances its `uint32_t *` destination by 4 per chunk, but
   the writer re-writes the same low bits for every chunk past the first. Transcribed as-is; it
   suggests the helper is only ever called with counts of 32 or fewer.
6. **`datum_delete` on an invalid handle writes through NULL.** Both the validity test and the
   unconditional `*element = 0` that follows it are preserved, so a bad handle crashes rather
   than returning early. Not "fixed".
7. **`struct_definition_decode`'s `_struct_field_variable_data` case never clamps the wire count.**
   The count is sign-extended (`(uint)(short)`) and added to a signed cursor, so a wire value of
   `0x8000` or more makes the bounds test pass and the following copy run away. The
   `_struct_field_struct_array` case *does* clamp. Preserved verbatim; see the inline note.
8. **`struct_definition_byte_swap`'s `out_record_count` is uninitialized** in the original when
   `codes[1] < 1` (Ghidra's `local_1c` is never assigned on that path). The rewrite writes 0
   instead, the one place in the module where reproducing the original was not possible.

## Functions and rewrite confidence

`name` is confidence in the symbol name, `rw` is confidence in the C rewrite, `U` counts `UNSURE`
markers in the file.

| Address | Function | Bytes | name | rw | U |
|---|---|---|---|---|---|
| `0x4cf810` | `growable_array_add_element` | 124 | 0.60 | 0.85 | 0 |
| `0x4cf890` | `growable_array_remove_element` | 96 | 0.60 | 0.85 | 0 |
| `0x4cf8f0` | `bit_stream_write_bits_chunked` | 82 | 0.40 | 0.55 | 1 |
| `0x4cf950` | `bit_stream_read_bits_chunked` | 78 | 0.40 | 0.60 | 0 |
| `0x4cf9a0` | `bit_stream_write_bit` | 117 | 0.60 | 0.75 | 2 |
| `0x4cfa20` | `bit_stream_write_bits` | 348 | 0.60 | 0.70 | 0 |
| `0x4cfb80` | `bit_stream_read_bit` | 103 | 0.60 | 0.80 | 0 |
| `0x4cfbf0` | `bit_stream_read_bits` | 341 | 0.60 | 0.70 | 0 |
| `0x4cfd90` | `byte_swap_array` | 331 | 0.70 | 0.60 | 1 |
| `0x4cfee0` | `struct_definition_byte_swap` | 512 | 0.60 | 0.55 | 1 |
| `0x4d0170` | `circular_buffer_new` | 65 | 0.80 | 0.85 | 1 |
| `0x4d01c0` | `circular_buffer_write` | 118 | 0.80 | 0.70 | 1 |
| `0x4d0240` | `circular_buffer_read` | 124 | 0.80 | 0.70 | 0 |
| `0x4d02c0` | `datum_index_invalidate` | 11 | 0.30 | 0.85 | 0 |
| `0x4d02d0` | `crc32_update` | 81 | 0.85 | 0.90 | 0 |
| `0x4d0330` | `crc32_build_table` | 53 | 0.90 | 0.90 | 0 |
| `0x4d0370` | `data_new` | 93 | 0.70 | 0.80 | 0 |
| `0x4d03d0` | `datum_new_at_index_with_salt` | 92 | 0.50 | 0.75 | 0 |
| `0x4d0430` | `datum_new_at_index` | 75 | 0.50 | 0.75 | 0 |
| `0x4d0480` | `datum_new` | 131 | 0.65 | 0.75 | 0 |
| `0x4d0510` | `datum_delete` | 108 | 0.70 | 0.65 | 2 |
| `0x4d0580` | `data_delete_all` | 75 | 0.75 | 0.80 | 0 |
| `0x4d05d0` | `data_iterator_next` | 85 | 0.75 | 0.75 | 0 |
| `0x4d0630` | `datum_next` | 69 | 0.45 | 0.75 | 0 |
| `0x4d0680` | `datum_get` | 59 | 0.65 | 0.80 | 0 |
| `0x4d06c0` | `datum_element_initialize` | 51 | 0.45 | 0.75 | 0 |
| `0x4d0700` | `byte_stream_write_ranged_integer` | 217 | 0.45 | 0.55 | 0 |
| `0x4d07e0` | `byte_stream_write_string` | 112 | 0.55 | 0.70 | 0 |
| `0x4d0850` | `byte_stream_read_long` | 67 | 0.50 | 0.70 | 0 |
| `0x4d08a0` | `byte_stream_read_ranged_integer` | 133 | 0.45 | 0.55 | 1 |
| `0x4d0930` | `byte_stream_read_string` | 70 | 0.55 | 0.75 | 0 |
| `0x4d0980` | `struct_definition_table_compute_sizes` | 79 | 0.60 | 0.75 | 1 |
| `0x4d09d0` | `data_packet_group_decode_packet` | 270 | 0.85 | 0.55 | 4 |
| `0x4d0ae0` | `data_packet_group_encode_packet` | 125 | 0.85 | 0.35 | 2 |
| `0x4d0b60` | `data_packet_group_append_packet_header` | 86 | 0.85 | 0.60 | 0 |
| `0x4d0bc0` | `data_packet_group_encode_packet_body` | 175 | 0.55 | 0.35 | 2 |
| `0x4d0c70` | `data_packet_group_decode_packet_body` | 210 | 0.55 | 0.35 | 3 |
| `0x4d0d50` | `struct_definition_compute_size` | 249 | 0.85 | 0.80 | 1 |
| `0x4d0e80` | `struct_definition_encode` | 1258 | 0.85 | 0.55 | 8 |
| `0x4d13c0` | `struct_definition_decode` | 869 | 0.85 | 0.60 | 2 |
| `0x4d1750` | `cache_new` | 152 | 0.80 | 0.75 | 0 |
| `0x4d17f0` | `cache_flush` | 78 | 0.75 | 0.40 | 1 |
| `0x4d1840` | `cache_allocate_block` | 984 | 0.80 | 0.35 | 2 |
| `0x4d1c20` | `cache_evict_entry` | 127 | 0.70 | 0.60 | 2 |
| `0x4d1ca0` | `cache_build_status_bitmap` | 190 | 0.80 | 0.60 | 1 |
| `0x4d1d60` | `block_list_allocate` | 128 | 0.80 | 0.70 | 0 |
| `0x4d1de0` | `block_list_reallocate` | 144 | 0.80 | 0.45 | 1 |
| `0x4d1e70` | `block_list_unlink` | 55 | 0.80 | 0.60 | 0 |
| `0x4d1eb0` | `block_list_compact` | 82 | 0.85 | 0.65 | 0 |
| `0x4d1f10` | `heap_allocate` | 103 | 0.85 | 0.60 | 1 |
| `0x4d1f80` | `heap_reallocate` | 145 | 0.85 | 0.55 | 1 |
| `0x4d2020` | `heap_resize_block` | 114 | 0.70 | 0.45 | 1 |
| `0x4d20a0` | `heap_unlink_block` | 76 | 0.85 | 0.75 | 0 |
| `0x4d20f0` | `heap_get_free_bytes` | 31 | 0.85 | 0.85 | 0 |
| `0x4d2110` | `heap_find_first_free_slot` | 37 | 0.90 | 0.85 | 0 |
| `0x4d2140` | `heap_advance_free_slot` | 55 | 0.90 | 0.85 | 0 |
| `0x4d2180` | `heap_allocate_raw` | 387 | 0.75 | 0.50 | 0 |
| `0x4d2310` | `heap_compact` | 94 | 0.80 | 0.55 | 0 |
| `0x4d2370` | `heap_find_free_block` | 85 | 0.85 | 0.65 | 0 |

The 20 names first established by this rewrite are registered in
`symbols/agent_phase4_memory.txt` and merged into `symbols/functions.txt` by
`tools/merge_symbols.py`. The other 39 already matched `symbols/functions.txt` exactly.
