# networking module type notes

Header: `types/networking.h`. Smoke gate: `out/phase4/networking_smoke.c`, checked with

    C:\msys64\ucrt64\bin\gcc.exe -fsyntax-only -I types out/phase4/networking_smoke.c
    C:\msys64\ucrt64\bin\gcc.exe -m32 -fsyntax-only -I types out/phase4/networking_smoke.c

Both pass. The `-m32` run is the meaningful one: every size and offset assertion for a
struct containing pointers is gated on `PTRS32`, so the 32-bit run actually validates the
0xae4 channel, the 0xa10 server block, the 0xf4c client block and the rest.

`networking.h` needs `types/game.h` in addition to `tags.h` and `memory.h`, because
`network_game_session` embeds a real `game_variant` at +0x104 (see below). Nothing that
memory.h or game.h already owns is redefined.

---

## s_network_address (0x14)

Entirely from `network_address_to_string` (0x440570). It treats the object as `ushort *`
and prints `in_EAX[8] == 4` as `%hd.%hd.%hd.%hd:%hu` from bytes +3, +2, +1, +0 and
`in_EAX[9]` as the port, or `in_EAX[8] == 0x10` as eight hex groups. That pins

* 0x00..0x0f address (IPv4 is a host-order dword whose high byte prints first)
* 0x10 `size` (4 or 16)
* 0x12 `port`

`network_channel_get_remote_address` (0x441ce0) writes exactly those three fields
(`*ESI`, `ESI+0x12`, `ESI+0x10 = 4`) and nothing else, so 0x14 is the full size.

Unresolved: nothing. The 12 bytes at 0x04..0x0f are only ever touched on the IPv6 path,
which retail never takes.

## network_mutex_record (0x28) / network_thread_record (0x08)

`network_mutex_slot_allocate` (0x440420) strides the table by 0x28, zeroes +0x00 and +0x04
and sets +0x24 to 1; `mutex_create` (0x440510) snprintfs `"mutex_%ld"` into slot+0x04 with
a 0x20 limit and stores the `CreateMutexA` handle at +0x00. The scan bound `0x6f12d4`
against a start of `0x6f0dd4` gives exactly 32 slots at 0x006f0db0.

`network_thread_create` (0x440460) strides by 8 (`(&DAT_006f0cb4)[i*8]`), handle at +0x00,
in-use byte at +0x04, 32 slots at 0x006f0cb0.

`autopatch_download_pool_initialize` (0x576c30) inlines both allocators and confirms the
same offsets independently.

Unresolved: nothing.

## network_connection_statistics (0x44) and network_summary_statistics (0x1c)

The stride is pinned three ways in `network_connection_stats_lookup_or_add` (0x440a80):
`(&DAT_0087becc)[i*0x11]` (dword array), `(&DAT_0087bed0)[i*0x22]` (word array) and
`(&DAT_0087bec8)[i*0x44]` (byte array) all resolve to the same element.

Field names come from the log headers rather than guesswork:

* `network_connection_stats_log_tick` (0x440d80) prints
  `"%d\t%d\t%d\t%d\t%d\t%c"` from `piVar9[6], *piVar9, piVar9[2], *piVar9 - piVar9[2], piVar9[3]`
  with `piVar9 = record + 0x24`, under the header
  `Time / Packets Sent / Total Sent / Reliable Sent / Unreliable Sent / Resends Sent`.
  That maps +0x3c, +0x24, +0x2c, +0x30 directly.
* `FUN_00440b20` (the per-packet accumulator) adds the same length to +0x14 and +0x24, and
  to +0x1c/+0x2c when its second flag is set and +0x20/+0x30 when its third is; it bumps
  +0x34/+0x3c on send and +0x38/+0x40 on receive. Because log_tick zeroes only the +0x24
  set, the +0x24 set is per-row and the +0x14 set is lifetime.
* `network_connection_stats_end` (0x440d20) does `+0x00 += now - +0x04` and clears +0x04
  and the +0x08 byte, which names the first three fields.

The session-wide counters 0x0087bea0..0x0087beb8 are named from the
`Game Summary` header written by `network_stats_summary_log_open` (0x440670) and the
arithmetic in `network_stats_summary_log_write` (0x440820): `+0x14 / +0x18` is the
`Avg # Players` quotient, `+0x04 / +0x0c` is `Bytes Sent/packet`, and so on.

Unresolved: +0x12 is assumed padding (the key is only ever compared as a short at +0x10
plus a dword at +0x0c). `0x0087ac06` gates every statistics path with `2 < value` and is
declared as an int16 without further evidence.

## network_receive_queue (0x1c)

`network_receive_queue_new` (0x441bf0) writes every field: +0x00, byte +0x04, byte +0x05,
+0x08 = -1, byte +0x0c, byte +0x0d = 0x14, word +0x0e, +0x10 = a `circular_buffer` of
0x10019 bytes named `received_data_queue`, +0x14 = -1, +0x18 = 0, then stops. The free
function (0x441c80) frees +0x10 and the object.

* +0x04 is the readable flag: `FUN_004419d0` tests `*(char *)(q + 4) == 1`.
* +0x0c is a flag byte: `FUN_004419d0` ORs in 4, `network_channel_list_add` (0x441a40) ORs
  in 8 and branches on bit1, `network_listen_accept_pending_connection` (0x4421b0) ORs in 1,
  and `network_channel_receive_callback` (0x441ed0) uses bit0 to choose between recording
  the sender address and appending to the circular buffer.
* +0x08 is the value pushed into the `fd_set` (`network_channel_list_add` reads
  `*(uint *)(entry + 8)`), while +0x00 is the handle passed to the winsock wrappers.
* +0x0e is the error word: 0x441ce0 stores 0xfff1 there when no address is available.

Unresolved: +0x05, +0x0d (constructed as 0x14, never read in this module), +0x14, +0x18.

## network_channel_list (0x114)

`FUN_00441960` allocates 0x114 and fills +0x104 (a `GlobalAlloc` of `AX*4`), +0x108 = AX,
+0x10c = -1, +0x110 = 0 after zeroing +0x00; `network_channel_list_add` uses +0x00 as a
count, +0x04.. as a 64-entry dedup array and +0x10c as the write cursor. That is exactly
Winsock's `fd_set` (4-byte count plus 64 sockets = 0x104), which is why the constructor
refuses a capacity above 0x40.

Unresolved: +0x110.

## network_channel_stream (0x534)

`FUN_004dd980` initialises one: `[0]=0`, `[1] = this + 0x1d`, `[2]=[3]=[4]=0`,
`[5]=0x287f`, `[6]=0x2880`, byte `[7]=1`. Laid against memory.h's `bit_stream` that is
`unknown_00 / data / first_bit / byte_cursor / bit_cursor / last_bit`, followed by a
capacity dword and an "empty" byte, then 0x2880 bits (0x510 bytes) of buffer.

The stride 0x534 is pinned by `network_channel_delete`, which writes the identical
seven-dword-plus-byte pattern at channel+0x010 and at channel+0x544 (0x544 - 0x010 = 0x534),
and by `network_channel_scan_retransmit_timeouts`, which computes free space as
`(*(int*)(ch+0x558) - *(int*)(ch+0x550)*8) - *(int*)(ch+0x554) + 1` — i.e. last_bit,
byte_cursor and bit_cursor of the stream based at 0x544 — and clears the byte at ch+0x560
(0x544 + 0x1c) after each write.

Unresolved: the 7 bytes at +0x52d..+0x533. 0x1d + 0x510 = 0x52d and the next record starts
at +0x534, so they exist but this module never reads them.

## network_channel_reliable_slot (0x20)

`FUN_004dcc30` grows the pool ten slots at a time and initialises byte +0x00 = 0,
+0x04 = -1, +0x08 = max(body_arg, 100), +0x0c = max(header_arg, 100), +0x10 = +0x14 = 0,
+0x1c = GlobalAlloc(+0x0c), +0x18 = GlobalAlloc(+0x08). `FUN_004dcdb0` stores a message:
+0x04 = the priority argument, +0x10 and +0x14 = the two bit counts, byte +0x00 = 1, then
copies the two payloads into +0x18 and +0x1c. `network_channel_scan_retransmit_timeouts`
sweeps priority 0..9 against +0x04, treats `+0x10 + +0x14` as the total bit cost and clears
every +0x00 at the end. `network_channel_delete` frees +0x18 and +0x1c per 0x20-byte slot.

Unresolved: nothing, but note the naming asymmetry — the buffer at +0x18 is written first
by the retransmitter, so it is called `header`; the argument order in 0x4dcdb0 is the
opposite of the capacity order in 0x4dcc30, which is why the capacities are cross-named.

## network_channel (0xae4 listening / 0xa9c plain)

`network_channel_new` (0x4dc9b0) allocates 0xae4 for flag bit0 and 0xa9c for flag bit1,
and writes +0xae0 = 1, +0xae1 = 0, +0xa9c = `FUN_00441960()`, +0xa8c = flags,
+0xa78 = 0, +0xa7c = 0, +0xa80 = 0xe0, +0xa88 = 0, +0xa84 = GetTickCount, +0x00 = the
receive queue, +0x0c = a circular buffer named `transport-incoming`, and calls
`FUN_004dd980` twice (the two streams at +0x010 and +0x544).

`network_channel_delete` (0x4dcae0) walks `channel + 0x2a8` for 16 entries when flag bit0
is set, freeing each child and then `*(void **)(child_list + 0x104)` and the list itself —
which is what pins children[16] at 0xaa0 and listen_list at 0xa9c.

`network_channel_record_timestamp` (0x4dd930) writes the QPC-derived millisecond value to
+0x04. `FUN_004dd110` reads +0x04 for the 5000 ms silence test, clears/sets flag bit5,
resets +0xa80 to 0xe0, and dispatches on flags bit0 (accept path) vs bits1-2 (transmit).
`network_channel_scan_retransmit_timeouts` indexes `0x00697edc` with +0xa88, so that field
is a bandwidth-rate table index.

+0xa94 and +0xa98 come from the accept path in `FUN_004dd4e0` (pack 02.md around line 3556):
`*(int **)(child + 0xa94) = parent` and `*(byte *)(child + 0xa98) = 0/1` paired with
`parent + 0xae1`. Dozens of send paths test `*(char *)(*ptr + 0xa98)` before transmitting.

Unresolved: +0x008 and +0xa90. Neither is written by the constructor or the destructor and
neither shows up in an arithmetic form that names it.

## network_pending_connection (0x14)

`network_listen_connection_request_handler` (0x442090) refuses past 30 entries
(`0x1e < DAT_006f16d0`) and writes five dwords at `0x0087bc20 + count*0x14` from its
`param_2` (reply socket), `param_5`, `param_3` (address), `(short)param_4` (port) and the
first dword of the payload. `network_listen_accept_pending_connection` (0x4421b0) reads
`(&DAT_0087bc0c)[count*5]`, which is the same address for `count - 1`, so the queue is
popped from the top.

Unresolved: +0x04. The handler writes `param_5` there and nothing in this module reads it.

## network_player_entry (0x20)

Four functions agree on the layout:

* `network_channel_table_initialize` (0x4de470) initialises 16 entries of stride 0x20
  starting at table+0x1a2: word +0x00 = 0, word +0x18 = 0xffff, word +0x1a = 0xffff,
  bytes +0x1c..+0x1f = 0xff.
* `FUN_004de4e0` (add) keys on the byte pair (+0x1c, +0x1d), finds a free row by scanning
  +0x1f for 0xff, and copies 8 dwords (0x20 bytes) into the row.
* `FUN_004de640` (remove) restores exactly the initialiser pattern and decrements the count.
* `FUN_004de9f0` (validate) requires +0x1d in 0..0, +0x1c in 0..15 and a UTF-16 name at
  +0x00 that terminates within 12 characters.
* `FUN_004df790` stores the chosen colour index at record+0x18.

Unresolved: +0x1a (reset to 0xffff with the colour but never read here) and +0x1e (reset to
0xff). The 0..0 range check on +0x1d is the reason it is named `machine_player_index` —
the PC build only ever has one local player per machine.

## network_game_session (0x3b0)

`network_channel_table_initialize` zeroes 0xec dwords (0x3b0), sets byte +0x19d = 0x10,
word +0x1a0 = 0, the 16 player rows at +0x1a2, and the bool at +0x3ac from `0x0071c2c1`.

The variant offset is the strongest piece of evidence in the whole module:
`FUN_004e1820` copies 0x26 dwords (0x98 bytes) of `0x0087aa80`
(game.h's `game_engine_pending_variant`) to `server + 0x10c`, i.e. session+0x104, and
game.h independently records `network_session + 0x10c` as the variant with its
`game_engine_index` at +0x13c — which is +0x30 into `game_variant`. `sizeof(game_variant)`
is 0x98 and 0x104 + 0x98 = 0x19c, landing exactly before `maximum_players`.

The same function does `strncpy(server + 0x8c, 0x0087aa40, 0x3f)` with a forced NUL at
server+0xcb, giving `char server_name[64]` at session+0x084, and zeroes word session+0x07e
and dword session+0x080.

The player count at +0x1a0 is confirmed from the other end: `network_connection_stats_log_tick`
adds `*(short *)(x + 0x1a0)` to the summary player accumulator where
`x = network_client + 0xb14` or `network_server + 8` — i.e. the session block in either
container.

Unresolved: +0x004..+0x07d (0x7a bytes), +0x0c4..+0x103 (0x40 bytes, very likely the map
name and a second string), +0x19c, +0x19e (seeded from `0x00696564`), +0x3a2..+0x3ab
(one dword of which, session+0x3a8, host_new sets to -1 then increments to 0), +0x3ac.
`message_callback` at +0x000 is only known to be a code pointer because host_new stores the
address `0x004e1410` into it.

## network_machine (0x60)

`FUN_004e0810` walks `server + 0x3c4` with a stride of 0x60 for 16 iterations and returns
`server + 0x3b8 + i*0x60`, which pins both the base and the stride and makes +0x0c the
machine id. `FUN_004e1880` counts slots where `*(int *)(id - 0xc) != 0 && id != -1`, naming
+0x00 as the channel pointer.

`network_game_server_host_new` initialises +0x00, +0x04, +0x08 = 0, +0x0c = 0xffff,
+0x0e = 0, +0x50 = 0, +0x51 = 0, +0x52 = 0, +0x56 = 0, +0x5c = -1.
`FUN_004df690` resets a slot: `+0x0e |= 2`, byte +0x10 = 0, +0x14 = 0, +0x18 = 0,
byte +0x50 = 0 and a 0xd-dword (0x34 byte) block from +0x1c.
`FUN_004dff20` sets bit3 of +0x0e on a build-string mismatch.
`network_game_server_host_dispose` tests `(short)slot[3] != -1` and reads
`*(uint *)(slot[0] + 0xa8c)`, confirming both +0x0c and +0x00 again.

Unresolved: +0x04, +0x08, +0x0f, +0x10, the whole 0x34-byte `connect_state`, +0x50, +0x51,
+0x52, +0x56, +0x5a, +0x5c. The two unaligned dwords at +0x52 and +0x56 are as the binary
has them (`*(undefined4 *)((int)puVar2 + 0x4e)` with `puVar2 = base + 4`).

## network_server_globals (0xa10)

`network_game_server_host_new` (0x4dec40) zeroes 0x284 dwords of `0x00861340`, which is the
size. It then sets +0x000 = `network_channel_new(1)`, word +0x004 = 0, byte +0x006 |= 2, and
`DAT_00861340[2] = 0x004e1410` (session+0x000). `network_game_server_host_dispose`
(0x4deda0) tests `(*(byte *)(p + 6) >> 2) & 1` to decide whether to flush the statistics
logs, which is where flags bit2 comes from, and `FUN_004e1820` sets bit0.

The password field is exact: `FUN_004e0910` does `wcsncpy(server + 0x9fc, src, 8)` and then
`*(undefined2 *)(server + 0xa0c) = 0`, so it is a 9-wide-character field ending at 0xa0d.
`FUN_004e08e0` compares it against the empty string. game.h already records server+0xa0f as
the end-of-game flag, and host_new writes `DAT_00861d4e` (+0xa0e) and `DAT_00861d4f` (+0xa0f).

Unresolved: +0x9b8..+0x9f7 (0x40 bytes; host_new clears dwords at +0x9b8, +0x9c4, +0x9c8,
+0x9cc, +0x9d0, +0x9d4 and bytes at +0x9f8..+0x9fa), +0x9f8..+0x9fb, +0xa0e.

## network_client_globals (0xf4c)

`network_session_create` (0x4d8a80) writes the storage at `0x00872de0` through named
globals, so every offset is that address minus 0x00872de0. It sets word +0x000 = 0xffff,
+0xadc = `network_channel_new(2)`, calls `network_channel_table_initialize` (the session at
+0xb14), then +0xec8, +0xecc, +0xed0, word +0xed8 = 0xffff, word +0xeda, word +0xedc,
`+0xede &= 0xfff9`, bytes +0xee0/+0xee1, twelve dwords from +0xee4, +0xf10 = -1, thirteen
dwords from +0xf14, and +0xf48 = a 0x2c-byte `GlobalAlloc`. `network_session_destroy`
(0x4d8b70) frees the +0xf48 list and deletes the +0xadc channel.

The session offset +0xb14 is confirmed independently by
`network_connection_stats_log_tick`, which reads `network_client + 0xb14 + 0x1a0` as a
player count.

Unresolved: +0x002..+0xadb (0xada bytes) and +0xae0..+0xb13 (0x34 bytes) are completely
unresolved; they are the biggest hole in the header. Every consumer of them is a
register-based handshake function (0x4d9050, 0x4daa20, 0x4dab80, 0x4db310, 0x4e0590) whose
`this` pointer Ghidra did not recover, so the offsets cannot be attributed with confidence
from the decompilation alone. Also unresolved: +0xec4, +0xec8..+0xed7, +0xed8..+0xee3, the
+0xee4 and +0xf14 runs, and +0xf10.

## network_game_search_entry (0x130)

`network_game_search_results_add_or_update` (0x4da7d0) walks nine records of 0x4c dwords
and writes: six dwords at +0x000, the QPC timestamp at +0x018,
`wcsncpy(entry + 0x1c, name, 0x3f)` with a forced word NUL at +0x09a, 0x21 dwords at +0x09c,
words at +0x120, +0x122, +0x124, +0x126, +0x128, +0x12a, and bytes at +0x12c, +0x12d,
+0x12e, +0x12f. The freshness rule (`6000 < now - entry[6]` wipes the record) also appears
in `FUN_004da770`, and the joinable byte requires announcement flag bit1 plus fewer than 16
players.

Unresolved: what the six `identity` dwords and the 33 `info` dwords actually are — they are
bulk copies out of the announcement message, which is itself decoded by the message-delta
layer. The words at +0x124, +0x128 and +0x12a are likewise only known by their source
offsets in the announcement (+0x158-ish, +0x15c, +0x1c).

## player_update_history / player_update_history_node (0x2c / 0x418)

`player_update_history_add` (0x4e6b50) allocates 0x418, sets +0x000 to the container's
rolling id, +0x004 to the tick argument, copies 8 dwords of stack arguments to +0x008,
+0x414 = 0, links via `container[2]` (tail) and `container[1]` (head), and wraps the id
modulo 0x40. It refuses once the list is 64 nodes long. `player_update_history_free_all`
(0x4e6f20) walks +0x414 and clears container +0x04 and +0x08.
`player_update_history_find_and_prune` (0x4e6f60) matches node +0x000 against an id.
`FUN_004e6b10` is the same free loop plus a `GlobalFree` of the container, called from
`network_session_destroy` on `network_client + 0xf48`, which is the 0x2c allocation.

The node body from +0x028 to +0x413 is a field-by-field copy of the unit object
(+0x5c..+0x520) and, when the unit is seated, of the vehicle object; it is declared as two
opaque byte arrays because nothing in this module reads an individual field back out — only
`player_update_history_play` (0x4e6ff0) does, and it does so by re-writing the same object
offsets. Byte +0x028 is the has-vehicle flag and +0x02c the vehicle object handle
(unit+0x11c).

Unresolved: container +0x0c..+0x2b (8 dwords). The ordering windows live elsewhere:
`is_local_player_update_in_order` (0x4e69b0) accepts within 15 of the current id, and
`is_remote_player_update_in_order` (0x4e6a20) within 3 of `player + 0x15c`, so +0x15c in
game.h's `player` is the last remote update id.

## ban_list_entry (0x38)

`ban_list_get_or_add_entry` (0x4e3890) computes `entry = i * 0x38 + DAT_006b85a4`,
`strncpy(entry, arg1, 0xc)` + NUL at +0x0c, `strncpy(entry + 0xd, arg2, 0x20)` + NUL at
+0x2d, and zeroes +0x2e, +0x2f, +0x30 and the dword at +0x34.
`network_banlist_load` (0x4e3160) parses the CSV as `atol -> (short *)(entry + 0x2e)`, then
either the literal `--` (which leaves the expiry zero) or `%04d-%02d-%02d %02d:%02d:%02d`
into `mktime -> *(int *)(entry + 0x34)`, then sets byte +0x30.
`network_banlist_add_ban` (0x4e35c0) indexes `DAT_00699574` with +0x2e when the tier is
below 4, increments +0x2e, and either sets +0x30 with a zero expiry ("indefinitely") or
stores `time() + penalty` at +0x34.

Note the field naming: `ban_list_find_by_name` actually searches the +0x0d field, and
0x4e35c0 passes the 12-character name from `FUN_00557950` as the first argument and the
32-character identifier from `FUN_0061aa50` as the second, so +0x00 is the display name and
+0x0d is the CD-key hash. The existing function name is slightly misleading.

Unresolved: +0x31..+0x33 (assumed padding).

## message delta protocol

Only partly resolved, and the header says so.

`message_delta_field_layout_compute_size` (0x4ec790) is the source for
`message_delta_definition`: it reads `ESI[8]` as a field count and `ESI + 10` as a
4-dword-stride binding array whose first dword is a descriptor carrying its size in bits at
+0x5c; it reads `ESI[7]` as a pointer to a second list whose count is at +0x00 and whose
size it writes back to +0x04; it reads `ESI[5]` as the maximum item count and writes
`ESI[9]`, `ESI[2]`, `ESI[1]`, `ESI[3]`, `ESI[4]` and byte `ESI[6]`. The header bit count is
7, or 10 when `DAT_0071cfa8` (the parameters protocol) is on, plus
`(&DAT_0065d51f)[max_items]` for more than one item.

`message_delta_protocol_initialize` (0x4ec2f0) iterates the definition pointer table
`0x0065d440 .. 0x0065d520`, i.e. 56 entries.

`message_delta_parameters_protocol_register` (0x4ebe00) and `_free_registered` (0x4ebd50)
and `_pack_values` (0x4ec1a0) all stride `0x006b86c0` by 3 dwords: name, type (1 = int),
value pointer.

Unresolved:
* The field-type descriptor itself. Everything known is that it is at least 0x60 bytes and
  carries a bit size at +0x5c. `FUN_004ec840`, `FUN_004ec900`, `FUN_004ec700` and
  `FUN_004ec390` call through per-descriptor callbacks whose offsets are not recoverable
  from register-based decompilation. No struct is declared for it; the header uses `void *`.
* `0x0069a304`: 28 records of 0x18 bytes whose first byte `message_delta_protocol_initialize`
  sets to 1. The stride and count are certain, the contents are not. Left as a global
  comment, not a struct.
* The three trailing dwords of `message_delta_field_binding`.

## network_bandwidth_graph (0x23e0)

The layout is pinned by running the singleton globals against the constructor.
`network_bandwidth_graph_instance_init` (0x4d7de0) writes byte +0x000, +0x004, +0x008,
+0x00c, +0x010, +0x0bc, +0x0c0, +0x0c4. `FUN_004d7980` writes exactly the same fields at
`0x00719ce0, 0x00719ce4, 0x00719ce8, 0x00719cec, 0x00719cf0, 0x00719d9c, 0x00719da0,
0x00719da4`, and the deltas are identical, so the singleton base is 0x00719ce0.

`FUN_004d8080` builds the 320-column vertex array: `puVar5 = base + 0x5e4` stepping by 6
dwords with `puVar5[-3]` the x coordinate (spanning `*(short *)(base+0x26)` to
`*(short *)(base+0x2a)`), `puVar5[-2]` the y coordinate (`*(short *)(base+0x28)`) and
`*puVar5` = 0xffffffff, and clears 320 dwords at +0x0d8. That gives history at 0x0d8..0x5d7
and vertices at 0x5d8..0x23d7, immediately followed by +0x23d8 = 1 and +0x23dc.
`FUN_004d8140` scans 320 history entries from +0xd8 for the peak, and `FUN_004d8430`
averages `+0x5c4, +0x5c8, +0x5cc, +0x5d0, +0x5d4` (the last five history entries) into
+0x23dc. `network_bandwidth_rate_compute` (0x4d8540) divides +0x0bc and +0x0c0 by the
elapsed time since +0x0c4 into +0x0c8 and +0x0cc.

Unresolved: +0x014..+0x025 and +0x02c..+0x0bb (label text and layout scratch built by
`FUN_004d7e20`, 596 bytes of string formatting that was not traced), +0x0d4. The vertex
field order beyond x, y and the 0xffffffff at +0x0c is inferred from the 0x18 stride; z, u
and v are plausible names for a D3D vertex but are not individually witnessed.

## autopatch_download_slot (0x14)

`autopatch_download_pool_initialize` (0x576c30) clears five dwords per slot from
`0x006ef93c` to `0x006ef964` (two slots) and sets +0x00 = -1.
`autopatch_download_start` (0x576e60) finds a slot with +0x00 == -1, sets +0x04 = 1, stores
the request id at +0x00 and the local-file flag at +0x10.
`autopatch_download_complete_callback` (0x576ad0) matches on +0x00, sets +0x04 = 5 on error
or allocates `size + 1` into +0x08, records `size + 1` at +0x0c and sets +0x04 = 4.
`autopatch_download_get_result` (0x576f00) requires index in 0..1, +0x04 == 4 and +0x10 == 0.

Unresolved: nothing.

## server browser

`server_list_mutex_try_lock` (0x4ba760) returns `&DAT_007196bc` and
`server_list_result_count_get` (0x4ba820) reads `DAT_007196c0`, so the shared list object
has its count at +0x04. That is all the structure there is on the Blam side.

`server_browser_server_passes_filter` (0x4b7080) reads every server field through the
GameSpy key/value accessors `FUN_00617490` (string), `FUN_006174d0` (bool),
`FUN_00617aa0` (ping) and `FUN_00617c10` (int) with the literal keys `numplayers`,
`maxplayers`, `password`, `dedicated`, `game_classic`, `gametype`, `mapname`, `teamplay`
and `gamever`. Nothing indexes a server record by offset anywhere in the module, so the
queried-server type is not recoverable here and none is declared — see "misattributed"
below.

The filter state is six adjacent bytes at `0x0071948b..0x00719490`; the gametype byte maps
1..5 to `CTF`, `Slayer`, `Oddball`, `King`, `Race` in the switch at 0x4b72c7, and the ping
byte indexes `0x00695400`. Declared as `server_browser_filters` because the six bytes are
contiguous and only ever read together, but the binary does not group them.

Unresolved: the sort-column index, scroll offset and selection that the
`server_list_*` functions manipulate. They are separate scalar globals with no shared base
that the decompilation exposes.

---

## Functions that belong elsewhere, or that carry a wrong name

* **0x4e2d4b `network_game_server_add_player_to_game__hook_add_player`** — not a function.
  It is a mid-body address inside `sv_players` (0x4e2c70, 244 bytes, so it ends at 0x4e2d64
  — 0x4e2d4b is inside it) that Ghidra disassembled separately. It is the scoreboard
  row-formatting loop. The pre-existing hook name is wrong; no types come from it.
* **0x4e44e1 `client_machine_cleanup__hook_remove_player`** — same problem. It overlaps
  `map_list_matching_substring` (0x4e4470, 113 bytes ending at 0x4e44e1) and is a duplicate
  decompilation of the map-listing loop. The hook name is wrong.
* **0x4b8d30 `unicode_string_list_get_string`** — a tag accessor for the
  `unicode_string_list` tag, which `types/tags.h` already defines. Belongs to the cache/text
  module; no networking type comes from it.
* **0x4b8da0 `multiplayer_game_variant_description_generate`** (5301 bytes) — UI text
  generation over `game_variant`, owned by `types/game.h`. Skipped.
* **0x4e5390..0x4e8a00, the player update handlers** — these operate on game.h's
  `update_record`, `position_update_record`, `vehicle_update_record`, `player_update_queue`
  and `circular_queue`. Only the history list (which game.h does not have) is declared here.
* **0x4e3f30 `string_is_numeric`, 0x4e4040 `string_trim_whitespace`,
  0x4e51c0 `parse_time_duration_string`, 0x4e52c0 / 0x4e5320 the time formatters,
  0x575fa0** — cseries string and time helpers with no structures of their own.
* **0x5776d0 `registry_get_halo_version`, 0x577760 `registry_get_dist_id`** — plain
  registry reads, no types.
* **0x4e9c20, 0x4e9cd0, 0x4e9d20, 0x4e9d40** — a hash-indexed object-to-network-index cache.
  They call `hash_table_get`, so the container is a generic hash table owned by another
  module; only the eviction cursor behaviour is local. No type declared.
* **0x576f40 `autopatch_get_proxy_settings`, 0x577310 `autopatch_launch_updater`** — pure
  WinInet/WinHTTP and `CreateProcess` glue over OS structures; nothing Blam-shaped.
* **0x4b5d70..0x4bab30 generally** — the server browser drives GameSpy SDK objects living in
  the vendored library at roughly 0x00614000..0x00618000. Those objects (the query engine,
  the server record, the key/value store) are library types, not Blam types, and are
  deliberately not declared in `types/networking.h`.
* **0x440b20, 0x440d80** reference `0x0071c2d8 + 0xb14` and `0x0071c2d4 + 8` for a player
  count. That is the statistics code reaching into the session block; it is what let the
  session offsets be cross-checked, and it is the only place the two containers are proven
  to hold the same sub-object.
