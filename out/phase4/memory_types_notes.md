# memory module: type recovery notes

Header: `types/memory.h`. Sources: `out/phase2/memory/00.md`, `out/phase2/memory/01.md`,
`out/phase2/results/memory_00.json`, full decompiles via `python tools/pack.py 0xADDR`, and
direct reads of `bin/halo.exe` .data/.rdata (PE VA -> file offset) for the static definition
tables. Everything below is evidence, not guesswork, unless it says "unresolved" or "guess".

## growable_array (0x0c)

- `growable_array_add_element` @0x4cf810: ESI[0] = element_size (multiplied by count+1 for the
  GlobalAlloc/GlobalReAlloc size), ESI[1] = count (refuses at 0x7fffffff, returns the old count as
  the new element index), ESI[2] = data pointer (GlobalAlloc when NULL, GlobalReAlloc otherwise).
  Only the newly appended element is zeroed.
- `growable_array_remove_element` @0x4cf890: same three fields; memmove of
  `(count-1-index)*element_size` and a shrinking GlobalReAlloc, GlobalFree when the new size is 0.
- Unresolved: nothing. All 12 bytes accounted for.

## bit_stream (0x18)

- `bit_stream_write_bit` @0x4cf9a0, `bit_stream_read_bit` @0x4cfb80: +0x04 data base, +0x08 lower
  bit bound, +0x0c byte cursor, +0x10 bit-in-byte cursor, +0x14 inclusive upper bit bound. The
  commit test is `first_bit <= pos && pos <= last_bit`, with `pos == last_bit+1` also accepted so
  the cursor can come to rest at the end.
- `bit_stream_write_bits` @0x4cfa20 / `bit_stream_read_bits` @0x4cfbf0: same fields plus the mask
  tables at 0x0065c2b4 and 0x0065c2c0 (see below).
- **Unresolved: +0x00.** No function in the module reads or writes it. Callers embed the stream
  inside larger objects (e.g. `chimera__chat_out` @0x4aab00 uses base `iVar1+0x10`, and
  `FUN_004ddb60` @0x4ddb60 uses the same object with a flag byte at stream+0x1c and a payload
  buffer at stream+0x1d), which bounds the struct at 0x18 bytes but does not name field 0.
  Best guess by Blam convention: a `char *name`. Named `unknown_00`.
- Mask tables (read directly out of the image at 0x0065c2b0):
  `0065c2b4: ff fe fc f8 f0 e0 c0 80` = `0xff << i`, and
  `0065c2c0: 00 01 03 07 0f 1f 3f 7f ff` = `(1 << i) - 1` for i = 0..8.
  `DAT_0065c2c8` is simply entry 8 of the second table; `bit_stream_write_bits` indexes it with a
  negative index (`(&DAT_0065c2c8)[-bit_cursor]`), so the two names are one 9-byte table.

## byte_stream (0x10)

- `FUN_004d0700` (byte_stream_write_ranged_integer), `byte_stream_write_string` @0x4d07e0,
  `byte_stream_read_long` @0x4d0850, `FUN_004d08a0`, `byte_stream_read_string` @0x4d0930 all use
  ESI/ECX[0] = buffer, [1] = cursor, [2] = size, and set the low byte of [3] to 1 on overflow.
  `struct_definition_encode`/`_decode` use the identical layout through `param_2`.
- The overflow flag is written as a single byte (`*(undefined1 *)(stream + 3) = 1`) and read as
  `(char)stream[3]`, so bytes 0x0d..0x0f are padding.
- Unresolved: nothing, though no constructor was located, so the struct could in principle be
  longer than 0x10 in its callers.

## circular_buffer (0x18 + capacity)

- `circular_buffer_new` @0x4d0170 writes every field: +0x00 = the constructor argument (a name
  pointer, never dereferenced here), +0x04 = 0x63697263 ('circ'), +0x08/+0x0c cleared,
  +0x10 = requested_size+1, +0x14 = this+0x18. Allocation is `requested_size + 0x19` bytes, i.e.
  0x18 of header plus `capacity` bytes of storage.
- `circular_buffer_write` @0x4d01c0 advances +0x0c and wraps at +0x10; `circular_buffer_read`
  @0x4d0240 advances +0x08 only when its third argument is nonzero (peek vs. read). Used bytes are
  computed as `write - read` corrected by `+ capacity` when negative, which fixes +0x08 as the read
  cursor and +0x0c as the write cursor.
- Unresolved: nothing.

## crc32

- `crc32_build_table` @0x4d0330 builds the standard reversed CRC-32 table (0xedb88320), 256 x
  uint32 = 0x400 bytes, into EDX.
- `crc32_update` @0x4d02d0 guards it with the byte at 0x00719cd8 and indexes the table at
  0x006b7b00.
- Globals: `0x006b7b00 crc32_table crc32_lookup_table`, `0x00719cd8 uint8_t crc32_lookup_table_initialized`.

## data_array (0x38 header) / datum_header (0x02) / data_iterator (0x0c)

- `data_new` @0x4d0370: allocates `maximum_count*size + 0x38`, zeroes 0x38 bytes, strncpy(name, 31)
  at +0x00, +0x20 = maximum_count, +0x22 = element size, +0x24 = 0, +0x28..0x2b = '@t@d' in memory
  order (dword 0x64407440, reads 'd@t@'), +0x34 = this+0x38.
- `datum_new` @0x4d0480: +0x2c = next free index cursor, +0x30 = actual count, +0x32 = next
  identifier (post-increment, reseeded to 0x8000 when it wraps to 0), +0x2e = high-water mark.
- `datum_delete` @0x4d0510: validates index against +0x2e (not +0x20), clears the element
  identifier, rewinds +0x2c, walks +0x2e back over trailing free slots, decrements +0x30.
- `datum_get` @0x4d0680, `FUN_004d0630` (datum_next), `FUN_004d06c0` (datum_element_initialize),
  `datum_new_at_index` @0x4d0430, `datum_new_at_index_with_salt` @0x4d03d0: all consistent.
- `data_delete_all` @0x4d0580 reseeds +0x32 with `strncpy(&next_identifier, name, 2)` then ORs in
  0x8000, and zeroes the identifier of every one of maximum_count elements.
- **Independent confirmation of the whole header**: the image contains a byte-swap definition named
  `data_array_header` at 0x0068e39c with declared size 56 and codes at 0x0068e364:
  `-100, 1, 32, -2, -2, 1, 3, -4, -2, -2, -2, -2, -4, -101`
  = skip 32 (name) / swap int16 (maximum_count) / swap int16 (size) / skip 1 (valid) / skip 3
  (pad) / swap int32 (signature) / swap 4 x int16 (next_index, last_index, actual_count,
  next_identifier) / swap int32 (data). 32+2+2+1+3+4+8+4 = 56 = 0x38.
- The 3 bytes at +0x25 are genuinely a single skip-3 in that table, so they are pad, not a named
  field in this build (later Blam engines put `identifier_zero_invalid` at +0x25).
- `data_iterator_next` @0x4d05d0 uses EDI[0] = data_array*, EDI[1] = resume index (written as an
  int16 but read as an int32), EDI[2] = the handle of the element returned. No 4th field is
  touched, and `cache_flush`/`cache_build_status_bitmap` build iterators on 0x0c-byte stack slots.
- Unresolved: nothing.

## struct_definition (0x14) and struct_definition_field (0x0a)

- `FUN_004d0980` (struct_definition_table_compute_sizes) reads `defn+0x0c` as the field list and
  `defn+0x10` as a one-shot "already computed" byte.
- `struct_definition_compute_size` @0x4d0d50 compares each field record's [2]/[3] against
  `*(int16 *)(defn + 0x0a)`, which is therefore the version, and writes the computed size back into
  each record's [4].
- `FUN_004d0bc0`/`FUN_004d0c70` (encode/decode one packet type) use `defn+0x0a` as the version that
  gets written as a leading byte when it is > 0, and refuse a decoded version above it.
- +0x00 and +0x08 came from the image, not the code: the 39 packet definitions at 0x00698820 and
  up are `{char *name; int32 0; int16 size; int16 version; field *fields; int32 0}`. Example:
  `message_client_ping_packet` has size 4 and a single field record `(3, 1, 0, 0, 0)` = one int32;
  `message_client_broadcast_game_search_packet` has size 12 and `(2,2),(1,8),(9,0)` = 4 + 8.
- **Unresolved: +0x04** (zero in every instance in the image, never touched by the module) and the
  3 bytes after the +0x10 flag. Named `unknown_04` / `pad_11`.
- Field type codes are established by `struct_definition_compute_size` (sizes) cross-checked with
  `struct_definition_encode` @0x4d0e80 (what actually goes into the stream):
  0 unused (counted in the size but nothing written), 1 raw bytes, 2/3/4 int16/int32/int64 arrays
  with byte swapping, 5 string (count+1), 6 variable-length byte array with an int16 count in the
  source and a ranged-integer count prefix in the stream (count+2), 7 array of sub-structures whose
  field records follow inline at field+1 (count*element_size + 2), 8 raw block, 9 terminator.

## byte_swap_definition (0x14) and its code tables

- `struct_definition_byte_swap` @0x4cfee0 reads `table[1]` as the number of structure records and
  starts at `table[2]`; positive codes skip N bytes, -2/-4/-8 swap in place, -100 begins a nested
  record (recursion with the same data base), -101 ends a record, and -102 is followed by a
  `byte_swap_definition *` whose `+0x08` is its code table.
- The struct itself was read out of the image: scanning .data for the 'bysw' dword turns up 31
  instances, all shaped `{char *name; int32 size; int32 *codes; uint32 'bysw'; int32 0}`, e.g.
  `byte`/`word`/`long`/`real_vector2d`/`real_vector3d`, `data_array_header` (0x38),
  `syntax_node` (0x14), `packet_header` (0x01), `aiff container chunk` (0x0c).
- **Unresolved: +0x10** (zero in all 31 instances). Named `unknown_10`.

## data_packet_group (0x30), data_packet_type (0x08), data_packet_header (0x01)

- `data_packet_group_decode_packet` @0x4d09d0: `group+0x04` int16 type count, `group+0x10` type
  table; each table entry is `{int16 packet_class; int16 pad; struct_definition *}` (stride 8,
  class at +0, definition at +4).
- `data_packet_group_append_packet_header` @0x4d0b60 bounds the output at `group+0x0c`, and
  `data_packet_group_encode_packet` @0x4d0ae0 passes `group+0x0c` (as an int16) to the body encoder
  as the size limit.
- The packet header is one byte (the type), byte-swapped through the `packet_header` definition at
  0x00696780 whose code table 0x00696770 is `{-100, 1, 1, -101}` (one structure, one 1-byte field).
- The remaining fields come from the only instance, `network_game_messages_group` at 0x006994f8:
  name 0x0066c488, type_count 0x27, +0x06 = 8, +0x08 = 0x600, +0x0c = 0x800, types 0x006993c0,
  then 0, 0x35, -1, -1, -1, -1, 0x10.
  +0x06 = 8 is called `class_count` because the 39 table entries use exactly classes 0..7.
  The table is 39*8 = 0x138 bytes and ends exactly where the group struct begins, which is what
  fixes the type count; the next unrelated global starts at 0x00699528, which is what fixes the
  group size at 0x30.
- **Unresolved: +0x14 (0), +0x18 (0x35 = 53), +0x1c..+0x28 (four 0xffffffff), +0x2c (0x10).** None
  of them is read by this module. 0x35 is not the type count and not any obvious size.
- Global `0x006b7f00 char *data_packet_group_error` holds the last failure string
  (got packet with no header / bad type / mismatched class / which would not decode /
  could not encode packet / could not append header to encoded packet) and is set to NULL on
  success.

## cache (0x7c + entries) and cache_entry (0x1c)

- `cache_new` @0x4d1750 writes the whole container: name[32] at +0x00 (strncpy 31), +0x20 and
  +0x24 the two callbacks, +0x28 block count, +0x2c block shift, +0x30 = 1 (age), +0x34/+0x38 =
  0xffffffff (list head/tail handles), +0x3c = this+0x44, +0x40 = 'weee'. The embedded data_array
  starts at +0x44: maximum_count at +0x64, element size 0x1c at +0x66, valid = 1 at +0x68,
  signature at +0x6c, data = this+0x7c at +0x78. That is what fixes both the container size (0x7c)
  and the entry size (0x1c).
- Constructor arguments are visible at the call sites: `sound_cache_new` @0x443ca0 calls
  `cache_new(pool, block_count, 0xc, 0x200, 0x4440a0, 0x444060)` and `texture_cache_new`
  @0x4444d0 calls `cache_new(pool, 0x1000, 2, 0x1000, 0x444730, 0x444700)`, i.e. +0x2c really is a
  shift (4 KiB and 4-byte blocks).
- `cache_evict_entry` @0x4d1c20 calls `cache+0x20` (release) and unlinks using entry +0x0c (next)
  and +0x10 (previous), patching `cache+0x34`/`cache+0x38` at the ends.
- `cache_allocate_block` @0x4d1840 (full decompile) sets entry +0x04 = size in blocks, +0x08 =
  offset in blocks, +0x14 = `cache+0x30` (age), and links +0x0c/+0x10. It also calls `cache+0x24`
  with a datum handle and treats a nonzero return as "cannot evict", which is what makes +0x24 the
  in-use predicate and +0x20 the release callback.
- `cache_build_status_bitmap` @0x4d1ca0 confirms +0x28 is a block count (it zero-fills that many
  bytes, one per block) and produces the status bits 1 allocated / 2 age == cache->age / 4
  age + 0x1e < cache->age / 8 in use.
- The address of the backing region is not in the struct; callers hold it (the sound cache keeps it
  in DAT_006ac52c and computes `(entry->offset << cache->block_shift) + DAT_006ac52c`).
- **Unresolved: cache_entry +0x18.** Nothing in the module writes it, and the two cache users
  (`FUN_004440e0`, `FUN_00444800`) store their per-block bookkeeping in a *different* 0x10-byte
  data_array, not in the cache entry. Named `unknown_18`.
- `cache_allocation_gap` (0x10) is not a heap object: it is the 256-entry stack ring buffer
  `local_1000` inside cache_allocate_block, laid out as
  `{datum_index previous_entry; uint32 newest_age; int32 offset; int32 size}`. Included because the
  function is unreadable without it.

## memory_pool / block list (0x38 header) and memory_pool_block (0x18 header)

- `block_list_allocate` @0x4d1d60 stamps +0x00 = 'head' (0x68656164), +0x04 = size rounded up to 4,
  +0x08 = the caller pointer cell (EDI), +0x0c = NULL, +0x10 = the old tail, +0x14 = 'tail'
  (0x7461696c), and returns `block+0x18` as the payload. Arena fields: +0x24 base, +0x28 size,
  +0x2c free bytes, +0x30 first block, +0x34 last block.
- `block_list_unlink` @0x4d1e70 and `block_list_reallocate` @0x4d1de0 address the same block
  through the payload pointer: payload-0x14 = size, payload-0x0c = next, payload-0x08 = previous,
  payload-0x10 = the back pointer cell. That is what fixes +0x0c as next and +0x10 as previous.
- `block_list_compact` @0x4d1eb0 memmoves each block down and fixes `**(block+8)`, i.e. the back
  pointer is a `void **` into the owning structure (confirmed by `FUN_004f7d50` @0x4f7d50, which
  reads the payload back out of its own datum field at +8).
- **The header below +0x24 came from the constructor**: `game_state_new_pool` @0x00538150 zeroes
  0x38 bytes, writes 0x706f6f6c ('pool') at +0x00, strncpy(name, 31) at +0x04, base = this+0x38 at
  +0x24, size at +0x28, free bytes at +0x2c, and NULL first/last at +0x30/+0x34. Nothing is
  unresolved in this struct.
- Global `0x006b8cb4 memory_pool *object_memory_pool` (created as `game_state_new_pool("objects")`).

## heap (0x34 + maximum_blocks*4) and heap_block (0x10 header)

- `heap_allocate` @0x4d1f10 and `heap_reallocate` @0x4d1f80: block +0x00 is size with bit 31 as the
  in-use flag, payload is block+0x10, and the statistics live at heap +0x14 bytes allocated,
  +0x18 peak bytes, +0x1c allocation count, +0x20 peak count, +0x24 largest single allocation.
- `heap_allocate_raw` @0x4d2180: heap +0x04 base, +0x08 size, +0x0c table length, +0x10 cached
  table cursor, +0x2c first (lowest-address) block, +0x30 last (highest-address) block. The
  address-ordered insert (`< first` -> push front, `<= last` -> splice, else append) is what
  distinguishes first from last, and matches `heap_unlink_block` @0x4d20a0 (+0x08 previous,
  +0x0c next).
- `heap_get_free_bytes` @0x4d20f0: `size - ((last->size & 0x7fffffff) + (int)last - base)`.
- `heap_compact` @0x4d2310 is gated on the byte at heap +0x28, hence `compaction_disabled`.
- **Correction to the phase-2 naming**: the trailing pointer array at +0x34 is *not* a set of
  size-class free lists. `heap_find_first_free_slot` @0x4d2110 returns the index of the first
  **NULL** entry (or -1), `heap_advance_free_slot` @0x4d2140 advances the +0x10 cursor to the next
  NULL entry, `heap_allocate_raw` stores the new block pointer into that slot and copies the slot
  index into the block header at +0x04, and `heap_unlink_block` clears the slot. So it is an
  allocation slot table with one entry per live block, +0x0c is the maximum live block count, and
  block +0x04 is the owning slot index. Suggested renames: FUN_004d2110 ->
  heap_find_first_free_slot, FUN_004d2140 -> heap_advance_free_slot.
- **Unresolved: heap +0x00.** Never touched by the module, and no constructor was found (the two
  heaps in play reach `heap_allocate` with the heap already in ECX from a caller two frames up).
  By analogy with memory_pool it is probably a signature or a name pointer. Named `unknown_00`.
  The 3 bytes after +0x28 are padding by inference, not by direct evidence.

## Not part of the memory module

- 0x4d3980 (currently labelled `data_delete_all`) and 0x4d3a30 (`data_next_index`) are zlib
  `trees.c` code: `send_tree`/`compress_block` writing through a deflate_state at EAX with
  pending_buf at +0x08, pending at +0x14, bl_tree at +0x0a74, bi_buf (uint16) at +0x16b0 and
  bi_valid at +0x16b4, using the REP_3_6 / REPZ_3_10 / REPZ_11_138 thresholds (3, 6/10, 138). They
  share a symbol name with the real `data_delete_all` at 0x4d0580 by accident. No struct for them is
  defined in `types/memory.h`; a `deflate_state` belongs in a compression header if it is ever
  needed.

## Globals worth labelling (also listed inline in types/memory.h)

| global | type |
| --- | --- |
| 0x0065c2b4 | `uint8_t bit_mask_clear[8]` |
| 0x0065c2c0 | `uint8_t bit_mask_keep[9]` (0x0065c2c8 is entry 8) |
| 0x0068e364 | `int32_t data_array_header_byte_swap_codes[14]` |
| 0x0068e39c | `byte_swap_definition data_array_header_byte_swap_definition` |
| 0x00696770 | `int32_t packet_header_byte_swap_codes[4]` |
| 0x00696780 | `byte_swap_definition packet_header_byte_swap_definition` |
| 0x006993c0 | `data_packet_type network_game_messages_types[39]` |
| 0x006994f8 | `data_packet_group network_game_messages_group` |
| 0x006ac530 | `cache *sound_cache` (region base in 0x006ac52c) |
| 0x006ac540 | `cache *texture_cache` |
| 0x006b7b00 | `crc32_table crc32_lookup_table` |
| 0x006b7f00 | `char *data_packet_group_error` |
| 0x006b8cb4 | `memory_pool *object_memory_pool` |
| 0x00719cd8 | `uint8_t crc32_lookup_table_initialized` |
