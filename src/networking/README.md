# src/networking

Rewritten C for the Blam networking module of retail `halo.exe` 1.0.10 (x86, MSVC 7.1).
One file per function, named after the function, with the original Ghidra decompilation kept
verbatim in a trailing `#if 0` block. Every persistent type lives in `types/networking.h`. Two
files still declare a struct of their own — `network_server_build_game_info_packet.c` and
`network_server_handle_rcon_request.c` — and both are stack scratch frames, not objects that
exist anywhere in the image, so they were deliberately not promoted into the header.

## What is covered

`out/phase4/networking_functions.md` lists 454 functions for this module, spread over
`0x440350..0x5781c0`. This directory now holds **448** of them — the whole module except the six
misattributed addresses listed under *Known gaps* — with **1166** `UNSURE` marks across all
files. `rc` below is the mean rewrite confidence of the band.

| range | what it is | files | UNSURE | mean rc |
| --- | --- | --- | --- | --- |
| `0x440350..0x442290` | transport: named mutexes, worker threads, UDP channels, the receive queue, the listen/accept path, the bandwidth and connection statistics logs | 36 | 61 | 0.55 |
| `0x4b5d70..0x4b6160` | master-server (GameSpy) connection: its mutex, its worker thread, the per-tick request pump | 4 | 16 | 0.30 |
| `0x4b61c0..0x4b80f0` | join-game server browser: filters, sorting, the row/player/variant panels, the per-frame tick | 23 | 97 | 0.32 |
| `0x4b8a00..0x4bab30` | join-game ticker text buffer, join-by-address, the server-list pointer array | 16 | 36 | 0.54 |
| `0x4d7980..0x4d8620` | the net_graph bandwidth overlay and the network statistics overlay | 14 | 15 | 0.57 |
| `0x4d8a20..0x4dc4b0` | session lifecycle, the join handshake, the per-state tick handlers, the incoming-message decode cluster | 62 | 228 | 0.39 |
| `0x4dc730..0x4ddca0` | channels: allocation, the reliable retransmission pool, the two bit streams, the service routines | 23 | 47 | 0.46 |
| `0x4ddcc0..0x4df790` | host/server globals, machines, the player table, channel keys, timers, broadcasts | 42 | 96 | 0.38 |
| `0x4df840..0x4e2990` | the rest of the host/server code: state snapshots, the message dispatcher, the per-message handlers | 62 | 203 | 0.39 |
| `0x4e2990..0x4e5390` | the sv_* console commands, rcon, the ban list and banned<x>.txt | 48 | 140 | 0.56 |
| `0x4e5390..0x4e6950` | the local- and remote-player update decode family and the update/position/vehicle queues | 16 | 46 | 0.55 |
| `0x4e6950..0x4e8a00` | the player-update history list, its replay/reconciliation path and the update builders | 21 | 48 | 0.53 |
| `0x4e9330..0x4ed3f0` | the message-delta protocol: the four array codecs, the vector quantizers, the field-binding tables, the message encoder and decoder, the parameters protocol | 52 | 80 | 0.51 |
| `0x575fa0..0x5781c0` | the autopatch client: the download pool, the bungie.net version check, the registry reads | 30 | 57 | 0.48 |
| `0x4dc560`, `0x4dc5e0` | two strays between the bands above | 2 | 1 | — |

Nothing in the module range is left unwritten except the six misattributed addresses.

## Gate

    python tools/build_check.py networking      -> 448 ok, 0 failed
    python tools/build_check.py                 -> 2505 ok, 0 failed   (whole tree)

    gcc      -fsyntax-only -I types out/phase4/networking_smoke.c      -> clean
    gcc -m32 -fsyntax-only -I types out/phase4/networking_smoke.c      -> clean

`build_check` is a 64-bit host `gcc -fsyntax-only` pass: it proves every identifier, field and
prototype resolves and agrees across files, not that the 32-bit layout is right. The smoke file
`out/phase4/networking_smoke.c` is what checks the layout, and the `-m32` run of it is the one
that means anything for a struct holding pointers.

Note for future sweeps: `sizeof` under the gate is **not** a stride check for any struct holding
a pointer, because the gate compiles 64-bit.

Under mingw in C mode `wchar_t` is `unsigned short`, so a `uint16_t *` field in the header and a
`wchar_t *` at a call site are the same type. The header keeps `uint16_t` for UTF-16 because
Ghidra's CParser has no `wchar_t`, exactly as `types/game.h` records. Files that use `wchar_t`
directly need `#include <wchar.h>` after `networking.h`; files that touch
`network_game_session::variant` need `math.h` and `game.h` **before** `networking.h`, because
`types/*.h` carry no includes of their own. The four `network_index_cache_*.c` files additionally
need `objects.h` before `networking.h`, for `hash_table`.

## Reading the file headers

Each file starts with the address and size, a **name confidence** and a **rewrite confidence**,
the evidence the name rests on, the register calling convention (`blam-cc:` lines), and an
`UNSURE:` block for every reconstruction that is not directly supported by the decompilation.
Where a review pass corrected an earlier draft the header says `REVIEW PASS <date>:` or
`FIXED in the review pass:` and states what was wrong and what evidence settled it.

## Struct layouts

Every struct below lives in `types/networking.h`; offsets are byte offsets from the struct base,
pinned from the pointer arithmetic in the decompiled module and, where a note says so, from
`objdump -d -M intel` over `bin/halo.exe`. Structs the written range does not touch
(`player_update_history*`, `ban_list_entry`, the `message_delta_*` family, `autopatch_download_slot`)
are described here for completeness but are exercised only by functions that are still unwritten.


### `s_network_address` — size 0x14

| offset | type | field | note |
| --- | --- | --- | --- |
| 0x00 | `uint32_t` | `ipv4` | a.b.c.d packed high byte first |
| 0x04 | `uint32_t` | `ipv6_1` | only used when size == 16 |
| 0x08 | `uint32_t` | `ipv6_2` |  |
| 0x0c | `uint32_t` | `ipv6_3` |  |
| 0x10 | `uint16_t` | `size` | 4 or 16, see network_address_size |
| 0x12 | `uint16_t` | `port` | host order |

### `network_mutex_record` — size 0x28

| offset | type | field | note |
| --- | --- | --- | --- |
| 0x00 | `void *` | `handle` | CreateMutexA result |
| 0x04 | `char` | `name[0x20]` | snprintf of "mutex_%ld" |
| 0x24 | `uint8_t` | `in_use` |  |
| 0x25 | `uint8_t` | `pad_25[3]` |  |

### `network_thread_record` — size 0x08

| offset | type | field | note |
| --- | --- | --- | --- |
| 0x00 | `void *` | `handle` | CreateThread result, 0x4000 byte stack |
| 0x04 | `uint8_t` | `in_use` |  |
| 0x05 | `uint8_t` | `pad_05[3]` |  |

### `network_handle_registry_slot` — size 0x08

| offset | type | field | note |
| --- | --- | --- | --- |
| 0x00 | `network_thread_record *` | `record` | NULL when the slot is free |
| 0x04 | `uint8_t` | `registered` | the in-use flag of this slot, independent of |
| 0x05 | `uint8_t` | `pad_05[3]` |  |

### `network_connection_statistics` — size 0x44

| offset | type | field | note |
| --- | --- | --- | --- |
| 0x00 | `int32_t` | `connected_duration_ms` | += now - active_since_ms when the link ends |
| 0x04 | `int32_t` | `active_since_ms` | 0 while the entry is not live |
| 0x08 | `uint8_t` | `active` |  |
| 0x09 | `uint8_t` | `pad_09[3]` |  |
| 0x0c | `int32_t` | `connection_id` | first half of the lookup key |
| 0x10 | `int16_t` | `connection_key` | second half of the lookup key |
| 0x12 | `int16_t` | `pad_12` |  |
| 0x14 | `int32_t` | `bytes_sent` | lifetime |
| 0x18 | `int32_t` | `bytes_received` | lifetime |
| 0x1c | `int32_t` | `reliable_bytes_sent` | lifetime |
| 0x20 | `int32_t` | `resend_bytes_sent` | lifetime |
| 0x24 | `int32_t` | `interval_bytes_sent` | the Total Sent column, cleared per row |
| 0x28 | `int32_t` | `interval_bytes_received` |  |
| 0x2c | `int32_t` | `interval_reliable_bytes_sent` | the Reliable Sent column |
| 0x30 | `int32_t` | `interval_resend_bytes_sent` | the Resends Sent column |
| 0x34 | `int32_t` | `packets_sent` | lifetime |
| 0x38 | `int32_t` | `packets_received` | lifetime |
| 0x3c | `int32_t` | `interval_packets_sent` | the Packets Sent column |
| 0x40 | `int32_t` | `interval_packets_received` |  |

### `network_summary_statistics` — size 0x1c

| offset | type | field | note |
| --- | --- | --- | --- |
| 0x00 | `int32_t` | `start_ms` | global 0x0087bea0 |
| 0x04 | `int32_t` | `bytes_sent` | global 0x0087bea4 |
| 0x08 | `int32_t` | `bytes_received` | global 0x0087bea8 |
| 0x0c | `int32_t` | `packets_sent` | global 0x0087beac |
| 0x10 | `int32_t` | `packets_received` | global 0x0087beb0 |
| 0x14 | `int32_t` | `player_count_total` | global 0x0087beb4, sum of session player counts |
| 0x18 | `int32_t` | `player_count_samples` | global 0x0087beb8, the Avg # Players divisor |

### `network_receive_queue` — size 0x1c

| offset | type | field | note |
| --- | --- | --- | --- |
| 0x00 | `int32_t` | `socket` | 0 until a connection is accepted or opened |
| 0x04 | `uint8_t` | `data_ready` | set by the transport when a read can succeed |
| 0x05 | `uint8_t` | `unknown_05` |  |
| 0x06 | `int16_t` | `pad_06` |  |
| 0x08 | `int32_t` | `socket_key` | -1 when unused; the fd_set entry value |
| 0x0c | `uint8_t` | `flags` | bit0 connection oriented (buffer the payload), |
| 0x0d | `uint8_t` | `unknown_0d` | constructed as 0x14 |
| 0x0e | `int16_t` | `last_error` | see network_error_code |
| 0x10 | `circular_buffer *` | `incoming` | 0x18 header plus 0x10001 bytes |
| 0x14 | `int32_t` | `unknown_14` | constructed as -1 |
| 0x18 | `int32_t` | `unknown_18` |  |

### `network_channel_list` — size 0x114

| offset | type | field | note |
| --- | --- | --- | --- |
| 0x000 | `uint32_t` | `fd_count` |  |
| 0x004 | `uint32_t` | `fd_array[64]` | the socket_key of each member queue |
| 0x104 | `network_receive_queue` | `**entries` | capacity pointers, GlobalAlloc backed |
| 0x108 | `int32_t` | `capacity` | the constructor refuses more than 0x40 |
| 0x10c | `int32_t` | `last_index` | -1 when empty, else the highest used index |
| 0x110 | `int32_t` | `unknown_110` |  |

### `network_channel_stream` — size 0x534

| offset | type | field | note |
| --- | --- | --- | --- |
| 0x000 | `bit_stream` | `stream` | data == (uint8_t *)this + 0x1d |
| 0x018 | `uint32_t` | `capacity_bits` | 0x2880 |
| 0x01c | `uint8_t` | `empty` | 1 while nothing is staged |
| 0x01d | `uint8_t` | `data[0x510]` | 0x2880 bits of staging buffer |
| 0x52d | `uint8_t` | `unknown_52d[7]` | never read by this module |

### `network_channel_reliable_slot` — size 0x20

| offset | type | field | note |
| --- | --- | --- | --- |
| 0x00 | `uint8_t` | `pending` | 1 once a message is staged, cleared after the scan |
| 0x01 | `uint8_t` | `pad_01[3]` |  |
| 0x04 | `int32_t` | `priority` | -1 when free; the scan sweeps priorities 0..9 |
| 0x08 | `int32_t` | `header_capacity` | byte capacity of header |
| 0x0c | `int32_t` | `body_capacity` | byte capacity of body |
| 0x10 | `uint32_t` | `header_bits` |  |
| 0x14 | `uint32_t` | `body_bits` |  |
| 0x18 | `uint8_t *` | `header` | header_capacity bytes |
| 0x1c | `uint8_t *` | `body` | body_capacity bytes |

### `network_channel` — size 0xae4

| offset | type | field | note |
| --- | --- | --- | --- |
| 0x000 | `network_receive_queue *` | `endpoint` |  |
| 0x004 | `int32_t` | `last_activity_ms` | network_channel_record_timestamp, QPC milliseconds |
| 0x008 | `int32_t` | `unknown_008` |  |
| 0x00c | `circular_buffer *` | `incoming` | named "transport-incoming" |
| 0x010 | `network_channel_stream` | `outgoing` | the message staging stream: every queue/send |
| 0x544 | `network_channel_stream` | `retransmit` | the retransmission staging stream: only |
| 0xa78 | `int32_t` | `reliable_count` | number of slots in reliable |
| 0xa7c | `network_channel_reliable_slot *` | `reliable` |  |
| 0xa80 | `int32_t` | `send_budget` | reset to 0xe0 on every service pass |
| 0xa84 | `uint32_t` | `budget_base_tick` | GetTickCount at the last budget reset |
| 0xa88 | `int32_t` | `rate_index` | index into the rate table at 0x00697edc |
| 0xa8c | `uint32_t` | `flags` | see network_channel_flags |
| 0xa90 | `int32_t` | `unknown_a90` |  |
| 0xa94 | `struct network_channel *` | `parent` | set on a channel accepted by a listener |
| 0xa98 | `uint8_t` | `connected` | the flag every send path tests |
| 0xa99 | `uint8_t` | `pad_a99[3]` |  |
| 0xa9c | `network_channel_list *` | `listen_list` | listening channels only, from here down |
| 0xaa0 | `struct network_channel *` | `children[16]` |  |
| 0xae0 | `uint8_t` | `listening` | set by network_listen_start |
| 0xae1 | `uint8_t` | `child_busy` | paired with a child channel connected flag |
| 0xae2 | `uint8_t` | `pad_ae2[2]` |  |

### `network_resolved_address` — size 0x18

| offset | type | field | note |
| --- | --- | --- | --- |
| 0x00 | `s_network_address` | `address` | ipv4 at +0x00, size at +0x10, port at +0x12 |
| 0x14 | `uint32_t` | `unknown_14` | only ever zeroed, by 0x4dd390's default path |

### `network_timer_pair` — size 0x08

| offset | type | field | note |
| --- | --- | --- | --- |
| 0x00 | `int32_t` | `remaining_ms` |  |
| 0x04 | `int32_t` | `last_tick_ms` |  |

### `network_map_cycle_entry` — size 0x08

| offset | type | field | note |
| --- | --- | --- | --- |
| 0x00 | `uint8_t` | `unknown_00` | from item+0x67 |
| 0x01 | `uint8_t` | `pad_01[3]` |  |
| 0x04 | `uint32_t` | `unknown_04` | from item+0xdc |

### `network_scenario_load_request` — size 0x10c

| offset | type | field | note |
| --- | --- | --- | --- |
| 0x00 | `uint32_t` | `unknown_00` | zeroed only |
| 0x04 | `int16_t` | `unknown_04` | zeroed only |
| 0x06 | `int16_t` | `seed` | set to 1, then overwritten from session->unknown_19e |
| 0x08 | `uint32_t` | `salt` | defaults to 0xdeadbeef, else session+0x3a4 |
| 0x0c | `char` | `map_name[0x100]` | strncpy of 0x7f from session+0x84 |

### `network_client_begin_connect_scratch` — size (see header)

| offset | type | field | note |
| --- | --- | --- | --- |
| 0x0000 | `uint16_t` | `pad_00` |  |
| 0x0002 | `uint16_t` | `name[8]` | UTF-16 |
| 0x0012 | `uint16_t` | `name_terminator` | forced NUL |
| 0x0014 | `uint32_t` | `config_template[1008]` |  |
| 0x0fd4 | `uint8_t` | `trailing_byte` |  |

### `network_pending_connection` — size 0x14

| offset | type | field | note |
| --- | --- | --- | --- |
| 0x00 | `int32_t` | `reply_socket` | the socket the accept or reject code is written to |
| 0x04 | `int32_t` | `transport_handle` |  |
| 0x08 | `uint32_t` | `remote_address` |  |
| 0x0c | `uint16_t` | `remote_port` |  |
| 0x0e | `uint16_t` | `pad_0e` |  |
| 0x10 | `uint32_t` | `first_payload_word` | first dword of the request, at least 4 bytes |

### `network_listen_accept_config` — size 0x10

| offset | type | field | note |
| --- | --- | --- | --- |
| 0x00 | `int32_t` | `result` | written 0 before the call |
| 0x04 | `void *` | `receive_callback` | network_channel_receive_callback (0x441ed0) |
| 0x08 | `void *` | `error_callback` | UNSURE: &LAB_00441f30, the disconnect path |
| 0x0c | `void *` | `connect_callback` | UNSURE: FUN_0044ad80 |

### `network_player_entry` — size 0x20

| offset | type | field | note |
| --- | --- | --- | --- |
| 0x00 | `uint16_t` | `name[12]` | UTF-16, NUL terminated inside the field |
| 0x18 | `int16_t` | `color_index` | 0xffff when unused; 0x4df790 picks a free one |
| 0x1a | `int16_t` | `unknown_1a` | reset to 0xffff alongside color_index |
| 0x1c | `int8_t` | `machine_index` | 0xff when the row is free |
| 0x1d | `int8_t` | `machine_player_index` | always 0 on the PC build |
| 0x1e | `int8_t` | `unknown_1e` | 0xff when free |
| 0x1f | `int8_t` | `slot_index` | 0xff when free, else the row index in players[] |

### `network_game_session` — size 0x3b0

| offset | type | field | note |
| --- | --- | --- | --- |
| 0x000 | `void *` | `message_callback` | 0x4dec40 stores 0x004e1410 here |
| 0x004 | `uint8_t` | `unknown_004[0x7a]` | zeroed at init, not resolved |
| 0x07e | `uint16_t` | `unknown_07e` | zeroed by 0x4e1820 |
| 0x080 | `int32_t` | `unknown_080` | zeroed by 0x4e1820 |
| 0x084 | `char` | `server_name[64]` | strncpy of 0x3f chars plus a forced NUL at 0x0c3 |
| 0x0c4 | `uint8_t` | `unknown_0c4[0x40]` |  |
| 0x104 | `game_variant` | `variant` | see types/game.h |
| 0x19c | `uint8_t` | `unknown_19c` |  |
| 0x19d | `uint8_t` | `maximum_players` | initialized to 16 |
| 0x19e | `int16_t` | `unknown_19e` | seeded from 0x00696564 |
| 0x1a0 | `int16_t` | `player_count` | the value the summary log averages |
| 0x1a2 | `network_player_entry` | `players[16]` |  |
| 0x3a2 | `uint8_t` | `unknown_3a2[10]` |  |
| 0x3ac | `uint8_t` | `unknown_3ac` | copied from 0x0071c2c1 |
| 0x3ad | `uint8_t` | `pad_3ad[3]` |  |

### `network_machine` — size 0x60

| offset | type | field | note |
| --- | --- | --- | --- |
| 0x00 | `network_channel *` | `channel` |  |
| 0x04 | `int32_t` | `unknown_04` |  |
| 0x08 | `int32_t` | `unknown_08` |  |
| 0x0c | `int16_t` | `machine_id` | 0xffff means the slot is free |
| 0x0e | `uint8_t` | `flags` | see network_machine_flags |
| 0x0f | `uint8_t` | `unknown_0f` |  |
| 0x10 | `uint8_t` | `unknown_10` | cleared by 0x4df690 |
| 0x11 | `uint8_t` | `pad_11[3]` |  |
| 0x14 | `int32_t` | `timer_14` | cleared by 0x4df690 |
| 0x18 | `int32_t` | `timer_18` | cleared by 0x4df690 |
| 0x1c | `uint8_t` | `connect_state[0x34]` | zeroed as one block by 0x4df690 |
| 0x50 | `uint8_t` | `unknown_50` | cleared by 0x4e0b90 |
| 0x51 | `uint8_t` | `unknown_51` |  |
| 0x52 | `int32_t` | `unknown_52` | unaligned in the original |
| 0x56 | `int32_t` | `unknown_56` | unaligned in the original |
| 0x5a | `int16_t` | `unknown_5a` |  |
| 0x5c | `int32_t` | `unknown_5c` | initialized to -1 |

### `network_server_globals` — size 0xa10

| offset | type | field | note |
| --- | --- | --- | --- |
| 0x000 | `network_channel *` | `listen_channel` | network_channel_new(1) |
| 0x004 | `int16_t` | `unknown_004` | tested against 0 and 2 by host_dispose |
| 0x006 | `uint16_t` | `flags` | bit0 session initialized, bit1 host, bit2 stats logging |
| 0x008 | `network_game_session` | `session` | everything shared but the password lives here |
| 0x3b8 | `network_machine` | `machines[16]` |  |
| 0x9b8 | `int32_t` | `unknown_9b8` | cleared by host_new, along with 0x9c4..0x9d4 |
| 0x9bc | `uint8_t` | `unknown_9bc[0x3c]` |  |
| 0x9f8 | `uint8_t` | `unknown_9f8` |  |
| 0x9f9 | `uint8_t` | `unknown_9f9` |  |
| 0x9fa | `uint8_t` | `unknown_9fa` |  |
| 0x9fb | `uint8_t` | `pad_9fb` |  |
| 0x9fc | `uint16_t` | `password[9]` | wcsncpy of 8 wide chars plus a forced NUL at 0xa0c |
| 0xa0e | `uint8_t` | `unknown_a0e` |  |
| 0xa0f | `uint8_t` | `game_over` | the end-of-game flag game.h records |

### `network_connection_endpoint` — size 0x28

| offset | type | field | note |
| --- | --- | --- | --- |
| 0x00 | `s_network_address` | `address` | (client+0xab4) size 4 and the port are written by 0x4d8cf0 |
| 0x14 | `uint32_t` | `unknown_14` | (0xac8) sixth dword of the six-dword blit 0x4d8c50 copies |
| 0x18 | `int32_t` | `last_send_ms` | (0xacc) last keepalive send, QPC milliseconds |
| 0x1c | `int16_t` | `message_count` | (0xad0) incremented once per keepalive sent |
| 0x1e | `int16_t` | `retry_count` | (0xad2) incremented once per overdue retransmit |
| 0x20 | `int16_t` | `unknown_20` | (0xad4) set to (0x4ed350's result << 1) on retransmit |
| 0x22 | `uint8_t` | `ready` | (0xad6) 0 while being rebuilt, 1 once populated |
| 0x23 | `uint8_t` | `unknown_23` | (0xad7) |
| 0x24 | `void *` | `control_block` | (0xad8) GlobalAlloc of 0x264, first two dwords zeroed |

### `network_connection_attempt_state` — size 0x34

| offset | type | field | note |
| --- | --- | --- | --- |
| 0x00 | `uint32_t` | `unknown_00` | (0xae0) cleared at the start of every attempt |
| 0x04 | `int32_t` | `started_ms` | (0xae4) QPC milliseconds; the progress bar's time base |
| 0x08 | `int32_t` | `elapsed_counter` | (0xae8) driven by the join status text animation |
| 0x0c | `uint8_t` | `unknown_0c` | (0xaec) cleared at the start of every attempt |
| 0x0d | `uint8_t` | `pad_0d` | (0xaed) |
| 0x0e | `uint32_t` | `session_info[9]` | (0xaee) nine dwords copied from the caller, unaligned |
| 0x32 | `uint8_t` | `unknown_32[2]` | (0xb12) not written by any function in this module |

### `network_client_timer_record` — size 0x14

| offset | type | field | note |
| --- | --- | --- | --- |
| 0x00 | `uint8_t` | `active` | (0xee4) 1 while a deadline is armed |
| 0x01 | `uint8_t` | `pad_01[3]` |  |
| 0x04 | `int32_t` | `deadline_ms` | (0xee8) QPC milliseconds |
| 0x08 | `uint8_t` | `triggered` | (0xeec) set when the deadline passes |
| 0x09 | `uint8_t` | `pad_09[3]` |  |
| 0x0c | `int32_t` | `context` | (0xef0) the value the scheduler was handed in EDX |
| 0x10 | `int32_t` | `retrigger_ms` | (0xef4) deadline_ms + context, the follow-up deadline |

### `network_client_globals` — size 0xf50

| offset | type | field | note |
| --- | --- | --- | --- |
| 0x000 | `uint16_t` | `unknown_000` | initialized to 0xffff |
| 0x002 | `uint8_t` | `unknown_002[0xab2]` |  |
| 0xab4 | `network_connection_endpoint` | `connection` | the server this client is talking to |
| 0xadc | `network_channel *` | `channel` | network_channel_new(2), deleted by destroy |
| 0xae0 | `network_connection_attempt_state` | `connect_attempt` |  |
| 0xb14 | `network_game_session` | `session` | the same block the server embeds at +0x008 |
| 0xec4 | `int32_t` | `unknown_ec4` |  |
| 0xec8 | `int32_t` | `unknown_ec8` |  |
| 0xecc | `int32_t` | `unknown_ecc` |  |
| 0xed0 | `int32_t` | `unknown_ed0` |  |
| 0xed4 | `int32_t` | `unknown_ed4` |  |
| 0xed8 | `uint16_t` | `unknown_ed8` | initialized to 0xffff |
| 0xeda | `uint16_t` | `state` | see network_client_state; NOT padding, see 0x4d8bb0 |
| 0xedc | `int16_t` | `unknown_edc` |  |
| 0xede | `uint16_t` | `unknown_ede` | bits 1 and 2 cleared at create |
| 0xee0 | `uint8_t` | `unknown_ee0` |  |
| 0xee1 | `uint8_t` | `unknown_ee1` |  |
| 0xee2 | `uint16_t` | `pad_ee2` |  |
| 0xee4 | `network_client_timer_record` | `timer` | the first five dwords of the zeroed run |
| 0xef8 | `network_resolved_address` | `server_address` | filled by 0x4dd390 from |
| 0xf10 | `int32_t` | `unknown_f10` | initialized to -1 |
| 0xf14 | `int32_t` | `unknown_f14[13]` | zeroed as one run at create |
| 0xf48 | `void *` | `update_history` | player_update_history *, GlobalAlloc of 0x2c |
| 0xf4c | `int32_t` | `connection_rate_index` | profile connection_type at begin_connect, 4 on host create; sent as the join request rate_index |

### `network_game_search_entry` — size 0x130

| offset | type | field | note |
| --- | --- | --- | --- |
| 0x000 | `uint32_t` | `identity[6]` | copied verbatim from the announcement header |
| 0x018 | `int32_t` | `received_ms` | QPC milliseconds, the freshness stamp |
| 0x01c | `uint16_t` | `name[64]` | UTF-16, wcsncpy of 0x3f then a forced NUL |
| 0x09c | `uint32_t` | `info[33]` | 0x21 dwords lifted from announcement+0xd0 |
| 0x120 | `int16_t` | `game_engine_index` |  |
| 0x122 | `int16_t` | `player_count` | from announcement+0x156 |
| 0x124 | `int16_t` | `unknown_124` |  |
| 0x126 | `int16_t` | `unknown_126` | from announcement+0x15a |
| 0x128 | `int16_t` | `unknown_128` |  |
| 0x12a | `int16_t` | `unknown_12a` |  |
| 0x12c | `uint8_t` | `joinable` | needs announcement flag bit1 and under 16 players |
| 0x12d | `uint8_t` | `in_use` |  |
| 0x12e | `uint8_t` | `stats_logging` | announcement flag bit2 |
| 0x12f | `uint8_t` | `unknown_12f` | set when engine 3 and announcement flag bit3 |

### `player_update_history_node` — size 0x418

| offset | type | field | note |
| --- | --- | --- | --- |
| 0x000 | `int32_t` | `update_id` | wraps modulo 0x40 |
| 0x004 | `int32_t` | `tick_count` | summed when the log reports "== %d ticks" |
| 0x008 | `uint32_t` | `control[8]` | the staged control record for this update |
| 0x028 | `uint8_t` | `has_vehicle` | set when the unit is in a seat |
| 0x029 | `uint8_t` | `pad_029[3]` |  |
| 0x02c | `datum_index` | `vehicle_object` | copied from unit+0x11c |
| 0x030 | `uint8_t` | `unit_state[0x0d0]` | unit fields 0x5c..0x520 |
| 0x100 | `uint8_t` | `vehicle_state[0x314]` | vehicle object fields, valid only with has_vehicle |
| 0x414 | `struct player_update_history_node *` | `next` |  |

### `player_update_history` — size 0x2c

| offset | type | field | note |
| --- | --- | --- | --- |
| 0x00 | `int32_t` | `next_update_id` | incremented modulo 0x40 per add |
| 0x04 | `player_update_history_node *` | `head` |  |
| 0x08 | `player_update_history_node *` | `tail` |  |
| 0x0c | `int32_t` | `unknown_0c[8]` | the rest of the 0x2c allocation |

### `ban_list_entry` — size 0x38

| offset | type | field | note |
| --- | --- | --- | --- |
| 0x00 | `char` | `name[13]` | strncpy of 12 then a forced NUL at 0x0c |
| 0x0d | `char` | `cd_key_hash[33]` | strncpy of 32 then a forced NUL at 0x2d |
| 0x2e | `int16_t` | `ban_count` | the offence tier, indexes the penalty table |
| 0x30 | `uint8_t` | `indefinite` | 1 when there is no expiry |
| 0x31 | `uint8_t` | `pad_31[3]` |  |
| 0x34 | `int32_t` | `expiry_time` | time_t, 0 when indefinite |

### `message_delta_field_type` — at least 0x68

The object a binding points at: one per field *type*, shared by every message that uses it.
Its layout is the union of every offset the module touches. Renamed and widened by the
whole-module pass, which is what let the four array codecs stop taking an `int32_t param_1`.

| offset | type | field | note |
| --- | --- | --- | --- |
| 0x00 | `int32_t` | `kind` | index into `message_delta_field_type_table`, stride 0x18 |
| 0x04 | `uint8_t` | `unknown_04[0x4c]` |  |
| 0x50 | fn ptr | `encode` | `(type, previous, destination, stream) -> bits`; called by 0x4e95e0, 0x4ec6a0 |
| 0x54 | fn ptr | `decode` | same shape; called by 0x4e9330 |
| 0x58 | `void *` | `array_descriptor` | the array codecs' `{count, ...}` block |
| 0x5c | `int32_t` | `size_bits` | cached by `message_delta_field_bindings_lazy_init` |
| 0x60 | `int32_t` | `reserved_bits` | flag bits the array codecs reserve ahead of the elements |
| 0x64 | `uint8_t` | `initialized` | set once by `_lazy_init`, cleared by `_teardown` |

### `message_delta_field_type_vtable` — size 0x18

The per-kind callback table at `0x0069a2f0`, 28 entries. Indexing it as `kind * 0x18 + 0x69a2f8 /
0x69a2fc / 0x69a300` is what 0x4ec840, 0x4ec700 and 0x4ec900 do. Folding this in also resolved
what the header used to record as a separate unnamed table of 0x18-byte records at `0x0069a304`:
those are this table's `registered` bytes, one per entry.

| offset | type | field | note |
| --- | --- | --- | --- |
| 0x00 | `uint8_t` | `unknown_00[8]` |  |
| 0x08 | fn ptr | `compute_size` | returns the field type's bit size |
| 0x0c | fn ptr | `initialize` |  |
| 0x10 | fn ptr | `teardown` |  |
| 0x14 | `uint8_t` | `registered` | set for all 28 by `message_delta_initialize` (0x4ec2f0) |

### `message_delta_field_binding` — size 0x10

The two offsets were `unknown_04` / `unknown_08` until the whole-module pass; five independent
call sites (0x4ec6a0, 0x4ec5d0, 0x4ed1d0, 0x4e95e0, 0x4ecc60) add `+0x04` to a *destination*
pointer and `+0x08` to a *previous-value* pointer, and 0x4ec840 / 0x4ec900 set and clear the
byte at `+0x0c`.

| offset | type | field | note |
| --- | --- | --- | --- |
| 0x00 | `message_delta_field_type *` | `field_type` | size in bits at field_type+0x5c |
| 0x04 | `int32_t` | `destination_offset` | byte offset of this field in the decoded body |
| 0x08 | `int32_t` | `source_offset` | byte offset of this field in the previous body |
| 0x0c | `uint8_t` | `initialized` | per-binding "its field type is live" flag |

### the message-delta field-type registry

The field-type instances are not built by any code: they sit in `.data`, one per named field
type, and the only way to see them is to read the image. Scanning for records whose `+0x50` and
`+0x54` both hold addresses inside `0x4e9000..0x4ed400` recovers the whole registry with Bungie's
own names inline at `+0x04`. That is what pins the layout above, and it is also why
`tools/pack.py` reports `callers=0` for every codec in `0x4e9130..0x4ebc20`: nothing calls them
directly, they are only reached through these records.

| kind | encode | decode | field types |
| --- | --- | --- | --- |
| 8 | `0x4e9130` | `0x4e9330` | `ctf_score_array`, `king_score_array`, `oddball_score_array`, `oddball_owner_array`, `race_score_array`, `slayer_score_array`, `network_game_players`, `object_change_colors`, `game_engine_variant`, `parameters_protocol_array` |
| 9 | `0x4e95e0` | `0x4e97e0` | `game_variant`, `universal_variant`, `network_map`, `network_player` |
| 10 | `0x4e9a30` | `0x4e9a60` | `hud_chat_message_ptr` |
| 13 | `0x4e9bc0` | `0x4e9bf0` | `object_index`, `player_index` |
| 14 | `0x4e9db0` | `0x4ea040` | `point2d`, `point3d` |
| 15 | `0x4ea240` | `0x4ea250` | `vector2d`, `vector3d` |
| 17 | `0x4ea2b0` | `0x4ea3b0` | `control_flags`, `damage_data_flags`, `game_variant_flags`, `universal_variant_flags` |
| 18 | `0x4ea430` | `0x4ea460` | `time` |
| 19 | `0x4ea610` | `0x4ea660` | `grenade_counts` |
| 20 | `0x4ea500` | `0x4ea5b0` | `fixed_width_1bit`, `fixed_width_3bits`, `fixed_width_6bits` |
| 21 | `0x4ea8b0` | `0x4eaa70` | `fixed_width_normal_4bit`, `_8bit`, `_16bit` |
| 22 | `0x4eabe0` | `0x4eaed0` | `locality_reference_position` |
| 23 | `0x4eb160` | `0x4eb1d0` | `digital_throttle` |
| 24 | `0x4eb220` | `0x4eb280` | `fixed_width_weapon_index` |
| 25 | `0x4eb2d0` | `0x4eb330` | `fixed_width_grenade_index` |
| 26 | `0x4eb680` | `0x4eb890` | `angular_velocity`, `translational_velocity` |
| 27 | `0x4ebab0` | `0x4ebc20` | `item_placement_position` |

Kinds 0-7, 11, 12 and 16 have no instance in `.data` with both pointers in that range; they are
presumably the primitive widths the field bindings encode inline.

Two names this directory inherited from the phase-2 pass are taxonomically wrong in light of the
registry, and are kept only because they describe the mechanism correctly and are already in
`symbols/functions.txt`:

* `message_delta_array_field_encode` (0x4e95e0) is **not** the encode half of
  `message_delta_array_field_decode` (0x4e9330). 0x4e9330 is kind 8 and its encoder is 0x4e9130;
  0x4e95e0 is kind 9, the compound-record codec, and its decoder is 0x4e97e0. That is exactly why
  the two descriptors have different shapes.
* `message_delta_float_array_encode` / `message_delta_dword_array_decode` (0x4e9db0 / 0x4ea040)
  are the `point2d` / `point3d` pair, and `message_delta_decode_vector3d_indexed` (0x4eb890) is
  the `angular_velocity` / `translational_velocity` decoder.

Each of those files carries a `REGISTRY (2026-09-20):` note saying so.

### the array-codec descriptors

`message_delta_field_type::array_descriptor` is read with three different shapes, one per codec
family. Nothing in the image shows them being the same object, so all three are declared.

| type | used by | layout |
| --- | --- | --- |
| `message_delta_array_descriptor` (0x0c) | 0x4e9330 decode | `{int32_t count; int32_t element_size; message_delta_field_type *field_type;}` — the decoder strides the elements itself |
| `message_delta_array_field_list` (0x04 + n*0x10) | 0x4e95e0 encode | `{int32_t count; message_delta_field_binding fields[];}` — note the fields start at **+0x04**, not the +0x08 of `message_delta_static_fields` |
| `message_delta_scalar_array_descriptor` (0x04) | 0x4e9db0 float encode, 0x4ea040 dword decode | `{int32_t count;}`, stride fixed at 4 bytes / 32 bits |

### `message_delta_sample_ring_buffer` — size 0x264

| offset | type | field | note |
| --- | --- | --- | --- |
| 0x00 | `int32_t` | `cached_average` | recomputed on append |
| 0x04 | `int32_t` | `count` | capped at 30 |
| 0x08 | `int32_t` | `write_cursor` | used once the ring is full |
| 0x0c | `int32_t` | `entries[30][5]` | 0x14 bytes per sample; the fields inside one sample are unnamed |

### `vector3d_lerp_table` — at least 0x18 / `waypoint_table` — 0x24 + n*0x0c

Both arrive as pointer arguments to the message-delta vector codecs (0x4eb370, 0x4eb4a0,
0x4eb560, 0x4eb890); neither is allocated in this module, so only the offsets that are read are
known.

| struct | offset | field | note |
| --- | --- | --- | --- |
| `vector3d_lerp_table` | 0x00, 0x04 | `minimum`, `maximum` | |
| | 0x0c, 0x14 | `denominator_mode1`, `denominator_mode0` | selected by the mode argument |
| `waypoint_table` | 0x00, 0x04 | `range_min`, `range_max` | |
| | 0x08, 0x10 | `bits_a`, `bits_b` | low byte used as a shift |
| | 0x0c, 0x14 | `max_level_a`, `max_level_b` | filled with `(1 << bits) - 1` when left zero |
| | 0x18 | `count_as_float` | read through an int conversion, not a bit reinterpretation |
| | 0x24 | `points[]` | `real_point3d`, NaN terminated |

### `network_index_cache` — size 0x2c

The object-to-network-index cache at `container+0x58` (0x4e9c20, 0x4e9cd0, 0x4e9d20, 0x4e9d40).

| offset | type | field | note |
| --- | --- | --- | --- |
| 0x00 | `int32_t` | `capacity` | entries in `slots` |
| 0x04 | `uint8_t` | `unknown_04[8]` |  |
| 0x0c | `uint8_t` | `table[0x18]` | a `types/objects.h` `hash_table`, key -> slot index; kept as bytes so `networking.h` does not need `objects.h`, and cast by the four users |
| 0x24 | `int32_t` | `cursor` | rotating allocation/eviction cursor |
| 0x28 | `int32_t *` | `slots` | `capacity` entries, -1 when free |

### `server_browser_custom_options` — at least 0x41

What the browser squeezes into the GameSpy key/value advertisement (0x576180 pack, 0x576460
unpack). It is narrower than `game_variant` and is not one; every field name except the type tag
is positional, and the low 3 bits of every packed code are the record's own tag.

| offset | type | field | note |
| --- | --- | --- | --- |
| 0x04 | `uint32_t` | `flags_a` |  |
| 0x1c | `int32_t` | `gametype_like` | 0, 1, 3 or 5; selects the sub-codec |
| 0x20 | `uint32_t` | `float_bits_20` | raw IEEE-754 bits, moved as an integer |
| 0x00, 0x08..0x18, 0x24..0x40 | | `unknown_*` | scalars the packer reads field by field |

### the sub-codec records

| type | size | note |
| --- | --- | --- |
| `server_browser_gametype1_options` | 0x08 | `{uint8_t flags[4]; int32_t time_limit;}` — the packer's view; `time_limit` is one of 0, 0x708, 0xe10, 0x1518, 9000, 18000 |
| `server_browser_gametype1_decoded` | 0x08 | `{uint8_t flags[4]; uint8_t low, high; uint8_t unknown_06, unknown_07;}` — the *unpacker's* view of the same bytes. The two halves genuinely disagree: the unpacker writes value pairs (8/7, 16/14, 24/21, 40/35, 80/70) that are nothing like the packer's time limits |
| `server_browser_gametype3_options` | 0x18 | `{uint8_t flag0, flag1; int32_t value_04, value_08, value_0c, value_10, value_14;}`, 2/2/2/2/5 bits on the wire |

### `message_delta_static_fields` — size 0x08

| offset | type | field | note |
| --- | --- | --- | --- |
| 0x00 | `int32_t` | `count` |  |
| 0x04 | `int32_t` | `size_bits` | written by message_delta_field_layout_compute_size |
| 0x08 | `message_delta_field_binding` | `fields[1]` | count entries |

### `message_delta_definition` — size 0x28

| offset | type | field | note |
| --- | --- | --- | --- |
| 0x00 | `int32_t` | `type_index` | index back into the definition pointer table |
| 0x04 | `int32_t` | `header_and_static_bits` |  |
| 0x08 | `int32_t` | `item_bits` | header_and_static_bits plus the per-item fields |
| 0x0c | `int32_t` | `header_bits` | 7, or 10 when the parameters protocol is on |
| 0x10 | `int32_t` | `maximum_bits` | header_bits + maximum_items * item_bits |
| 0x14 | `int32_t` | `maximum_items` |  |
| 0x18 | `uint8_t` | `initialized` |  |
| 0x19 | `uint8_t` | `pad_19[3]` |  |
| 0x1c | `message_delta_static_fields *` | `statics` |  |
| 0x20 | `int32_t` | `field_count` |  |
| 0x24 | `int32_t` | `field_bits` |  |
| 0x28 | `message_delta_field_binding` | `fields[1]` | field_count entries |

### `message_delta_parameter` — size 0x0c

| offset | type | field | note |
| --- | --- | --- | --- |
| 0x00 | `char *` | `name` | GlobalAlloc backed, "<scope>::<name>" when scoped |
| 0x04 | `int32_t` | `type` | 1 int, otherwise float |
| 0x08 | `void *` | `value` | the live variable |

### `network_graph_vertex` — size 0x18

| offset | type | field | note |
| --- | --- | --- | --- |
| 0x00 | `float` | `x` | spans left..right across 320 columns |
| 0x04 | `float` | `y` | the baseline until a sample raises it |
| 0x08 | `float` | `z` |  |
| 0x0c | `uint32_t` | `color` | initialized to 0xffffffff |
| 0x10 | `float` | `u` |  |
| 0x14 | `float` | `v` |  |

### `network_bandwidth_graph` — size 0x23e0

| offset | type | field | note |
| --- | --- | --- | --- |
| 0x0000 | `uint8_t` | `needs_layout` | forces the next update to recompute the layout |
| 0x0001 | `uint8_t` | `pad_0001[3]` |  |
| 0x0004 | `int32_t` | `last_sample_ms` |  |
| 0x0008 | `uint32_t` | `sample_interval_ms` | seeded from 0x006894b0 |
| 0x000c | `int32_t` | `units_index` | 0 bytes, 1 packets (0x4d8a20) |
| 0x0010 | `int32_t` | `direction_index` | 0 sent, 1 received (0x4d8a50) |
| 0x0014 | `uint8_t` | `unknown_0014[0x12]` |  |
| 0x0026 | `int16_t` | `left` | screen bounds, recomputed on a resize |
| 0x0028 | `int16_t` | `baseline` |  |
| 0x002a | `int16_t` | `right` |  |
| 0x002c | `uint8_t` | `unknown_002c[0x90]` | label text and layout scratch |
| 0x00bc | `int32_t` | `bits_sent` | accumulated by 0x4d79d0 |
| 0x00c0 | `int32_t` | `bits_received` | accumulated by 0x4d7a50 |
| 0x00c4 | `int32_t` | `rate_base_ms` |  |
| 0x00c8 | `float` | `rate_sent` | bits per second |
| 0x00cc | `float` | `rate_received` | bits per second |
| 0x00d0 | `int32_t` | `pending_sample` | folded into history on the next interval |
| 0x00d4 | `int32_t` | `unknown_00d4` |  |
| 0x00d8 | `int32_t` | `history[320]` |  |
| 0x05d8 | `network_graph_vertex` | `columns[320]` |  |
| 0x23d8 | `int32_t` | `peak_scale` | initialized to 1 |
| 0x23dc | `float` | `displayed_rate` | smoothed from the last five history entries |

### `autopatch_download_slot` — size 0x14

| offset | type | field | note |
| --- | --- | --- | --- |
| 0x00 | `int32_t` | `request_id` | -1 when the slot is free |
| 0x04 | `int32_t` | `state` | see autopatch_download_state |
| 0x08 | `void *` | `data` | GlobalAlloc of size bytes, NUL terminated |
| 0x0c | `int32_t` | `size` | payload length plus the terminator |
| 0x10 | `uint8_t` | `local_file` | 1 when the source was a file, not a URL |
| 0x11 | `uint8_t` | `pad_11[3]` |  |

### `server_list_globals` — size 0x10

| offset | type | field | note |
| --- | --- | --- | --- |
| 0x00 | `void` | `**list` | 0x007196bc, capacity GameSpy server-record pointers; |
| 0x04 | `int32_t` | `result_count` | 0x007196c0, live entry count |
| 0x08 | `int32_t` | `capacity` | 0x007196c4, allocated entry count |
| 0x0c | `int32_t` | `pending_count` | 0x007196c8, UNSURE: entries added since the last sort; |

### `server_browser_filters` — size 0x06

| offset | type | field | note |
| --- | --- | --- | --- |
| 0x0071948b | `uint8_t` | `dedicated_only` | requires the dedicated key to be 1 |
| 0x0071948c | `uint8_t` | `classic_only` | requires the game_classic key to be 1 |
| 0x0071948d | `uint8_t` | `allow_unknown_map` | skips the installed-map check when set |
| 0x0071948e | `uint8_t` | `gametype` | 0 any, 1 CTF, 2 Slayer, 3 Oddball, 4 King, 5 Race |
| 0x0071948f | `uint8_t` | `teamplay` | UNSURE polarity: 0x4b7300 rejects a server whose |
| 0x00719490 | `uint8_t` | `ping_limit_index` | 0 any, else indexes 0x00695400 |

### `network_screen_point` — size 0x04

| offset | type | field | note |
| --- | --- | --- | --- |
| 0x00 | `int16_t` | `x` |  |
| 0x02 | `int16_t` | `y` |  |

### `ticker_text_buffer` — size 0x18

| offset | type | field | note |
| --- | --- | --- | --- |
| 0x00 | `uint16_t *` | `text` | UTF-16, widget_memory_pool-backed, NULL when empty |
| 0x04 | `int32_t` | `start_column` | UNSURE: synced into an external widget field by |
| 0x08 | `int32_t` | `scroll_cursor` | character index of the current visible window start |
| 0x0c | `int32_t` | `length` | character count of text, excluding the NUL |
| 0x10 | `int32_t` | `capacity` | allocated capacity in characters |
| 0x14 | `int32_t` | `scroll_delay_ms` | per-character scroll delay, reset to 100 |


### `message_delta_decode_state` — size 0x20

The per-message decode record the message-delta protocol hands every client-side update
handler; `decode_context` slot 0 points at it. Added by the second review pass, which closed the
one TYPES-GAP this module carried (`network_queued_update_record`, previously a file-local
typedef in `network_client_drain_queued_updates.c`).

| off | field | notes |
| --- | --- | --- |
| 0x00 | `int32_t incremental` | 0 == stateless (baseline), non-zero == incremental |
| 0x04 | `int32_t message_type` | index into `message_delta_definitions[56]`; `0x4ed1d0` reads it first |
| 0x08 | `int32_t item_count` | total items; the drain loop compares `processed_count` against it |
| 0x0c | `int32_t bits_read` | accumulates `message_delta_read_changed_subfields`'s return |
| 0x10 | `void *stream` | bit cursor; `0x4ed1d0` reads +0x08/+0x0c/+0x10/+0x14 off it |
| 0x14 | `int32_t unknown_14` | |
| 0x18 | `int32_t processed_count` | items the drain loop has dispatched |
| 0x1c | `uint8_t more_items` | the drain loop stops when this clears |
| 0x1d | `uint8_t changed` | every delta handler stores 1 here after decoding |

Anchor: `message_delta_read_changed_subfields` (0x4ed1d0) takes this record in **EDI** and reads
`message_type` at +0x04 and the cursor at +0x10 in its first four instructions. `FUN_004ec590`
(0x4ec590) pulls the same record out of `decode_context` slot 0 and passes it straight through in
EDI, which is what proves the two are one type.

### `remote_player_update_header` — size 0x0c (UNSURE)

What `decode_context` slot 0x11 points at.

| off | field | notes |
| --- | --- | --- |
| 0x00 | `int32_t player_index` | wire index in, player `datum_index` out; the four "owning" handlers write the remap back, the two stand-alone position handlers deliberately do not |
| 0x04 | `uint8_t update_id` | wraps modulo 0x40; the `[%d]` in every log line |
| 0x05 | `uint8_t baseline_id` | which baseline a delta is relative to |
| 0x08 | `uint8_t control_sequence` | the second argument the two "total" handlers forward; the stand-alone handlers forward `baseline_id` there instead |

Size is a floor, not a fact: nothing in the image allocates one.

### `remote_player_action_state` — size 0x30

The 12-dword control record at `player+0x0f0` (`types/game.h` `player::unknown_f0 .. unknown_11c`),
staged on the stack by every remote-player handler.

| off | field | notes |
| --- | --- | --- |
| 0x00 | `uint32_t flags` | only the low byte is ever read |
| 0x04 | `int32_t unknown_04` | first of the eight dwords pushed onto the action queue |
| 0x08 | `real yaw` | `atan2(direction.j, direction.i)` |
| 0x0c | `real pitch` | `atan2(direction.k, sqrt(i*i + j*j))` |
| 0x10 | `int32_t unknown_10[4]` | |
| 0x20 | `uint16_t unknown_20` | `0x4e60c0` stores a leftover register here (see below) |
| 0x22 | `uint16_t unknown_22` | |
| 0x24 | `real_vector3d direction` | the aim direction yaw/pitch are derived from |

Named from `handle_remote_player_action_update` (0x4e60c0), which is the only function that
touches the individual fields.

### `remote_player_biped_update_state` — size 0x3c / `remote_player_vehicle_update_state` — size 0x70

The two combined ("total") payloads: a `remote_player_action_state` followed by the positional
half, staged as one block so one `message_delta_read_changed_subfields` call covers both.

| struct | off 0x00 | off 0x30 | fixed by |
| --- | --- | --- | --- |
| `remote_player_biped_update_state` | `action` -> `player+0x0f0` | `real_point3d position` -> `player+0x164` | `mov ecx,0xf ; rep stos` at 0x4e58fd |
| `remote_player_vehicle_update_state` | `action` -> `player+0x0f0` | `vehicle_update_body vehicle` -> `player+0x190` | `mov ecx,0x1c ; rep stos` at 0x4e5ac6 |

`vehicle_update_body` is `types/game.h`'s and is **not** redeclared here. Its field names are
independently confirmed by `player_update_client_remote_player_vehicle_update_from_network`
(0x4e6510), whose six field copies at 0x4e686f..0x4e68f2 land on `object::velocity` (0x68),
`object::angular_velocity` (0x8c), `object::forward` (0x74) and `object::up` (0x80) — exactly the
order `vehicle_update_body` declares them.

### `local_player_update_ack` — size 0x10 / `local_player_vehicle_update_ack` — size 0x44

The ECX destination `FUN_004ec590` decodes into for the two local-player ack handlers
(0x4e5390 on foot, 0x4e5490 in a vehicle).

| off | `local_player_update_ack` | `local_player_vehicle_update_ack` |
| --- | --- | --- |
| 0x00 | `uint8_t update_id` | `uint8_t update_id` |
| 0x01 | `uint8_t baseline_id` | `uint8_t baseline_id` |
| 0x04 | `real_point3d position` -> `player+0x0f0..0x0f8` | `vehicle_update_body vehicle`; its position at +0x08 is what lands in `player+0x0f0..0x0f8` |

### `network_map_list_entry` — size 0x0c / `network_buffer_pair` — size 0x08

Folded out of `map_list_matching_substring.c` and `network_buffer_pair_pool_clear.c` by the
review pass so that no file under `src/networking` declares a struct of its own any more.

| struct | off | field |
| --- | --- | --- |
| `network_map_list_entry` | 0x00 / 0x08 | `char *name` (NULL when the slot is free) / `uint8_t valid` |
| `network_buffer_pair` | 0x00 / 0x04 | `void *first` / `void *second`, both `GlobalAlloc`-backed |


## Known gaps

### The message-delta decoders are still black boxes

`FUN_004ec590` / `FUN_004ec600` / `FUN_004ec670` (0x4ec590, 0x4ec600, 0x4ec670) and
`message_delta_read_changed_subfields` (0x4ed1d0) now have pinned *calling conventions*
(EAX = decode context, ECX = destination; EDI = state for 0x4ed1d0) and the second review pass
rewired all 14 call sites to match. What they actually decode is still unrecovered: the
per-field work goes through the binding table at `message_delta_definition::fields`, and no
function in `src/networking` walks it.

### `network_session_broadcast_to_all` (0x4e19c0) has an undeclared EAX argument

Its disassembly reads **two** register arguments: `ECX` (the `network_server_globals *`, which
the declaration models) and `EAX` (`mov ebx,eax` at 0x4e19cd, which it does not). Every call site
sets EAX to the encoded size the preceding `message_delta_encode_message` returned
(0x4e118f in `network_server_check_machine_timeout`, 0x4df2b8..0x4df2c7 in
`network_server_advance_connect_state`). The declaration was left at seven parameters this pass
because adding an eighth touches ten files; see open question 2.

### `unit_snap_position_if_far` (0x4772e0) arity disagreement with `src/units`

`src/units` declares it with two parameters; the call at 0x4e649b genuinely pushes a third
(`player::unit`), so `player_update_client_remote_player_position_update_from_network.c`
declares three. One of the two is wrong and the units-module side was not re-derived here.

### 23 remaining `extern` arity disagreements inside the module

Every one is the same shape: Ghidra dropped a register argument in one file and a later rewriter
recovered it in another, so the two files declare the same callee with different arities. They
are not compile errors (each file is checked on its own), and the gate cannot see them. The
whole-module pass fixed the ones where the true signature was already known from `src/memory` or
from another file in this module; the rest need the real argument list first.

| function | arities seen | files |
| --- | --- | --- |
| `data_packet_group_decode_packet` | 6, 8 | 33 |
| `network_channel_remote_address_or_default` | 0, 2 | 21 |
| `network_prepare_challenge_packet` | 0, 2 | 16 |
| `network_player_entry_validate` | 0, 1 | 12 |
| `data_packet_group_encode_packet` | 4, 6 | 9 |
| `FUN_00557990`, `datum_get` | 0/2/3, 0/1/2 | 7 each |
| `network_disconnect_notify_dropped_machines` | 0, 1 | 5 |
| 15 more | | 1-4 each |

Fixed in this pass: `bit_stream_write_bit`, `bit_stream_write_bits_chunked`,
`circular_buffer_write` sense, `data_iterator_next`, `player_update_history_play` and
`player_update_history_log_write` now all match their definitions (the first four against
`src/memory`, the last two against their own files here).

### One global was carried under two different names

`0x006f1d6c` is `game_time_globals *game_time` everywhere else in the tree (93 files) but 14
files in the player-update family called it `void *network_update_ring_globals` and read
`+0x0c` by hand. All 14 now use the tree-wide name and `game_time->game_time`.

### Misattributed addresses in this module range

| address | what it really is |
| --- | --- |
| `0x4b8d30`, `0x4b8da0` | interface/localization text builders, not networking |
| `0x4e2d4b` | a mid-body address inside `sv_players` (0x4e2c70), not a function; folded into `sv_players.c` |
| `0x4e44e1` | a duplicate decompile of `map_list_matching_substring`, hook-named `client_machine_cleanup__hook_remove_player` |
| `0x4e3f30`, `0x4e4040`, `0x4e51c0`, `0x4e52c0`, `0x4e5320` | generic string/time helpers (`string_is_numeric`, `string_trim_whitespace`, `parse_time_duration_string`, `format_local_time_and_date`, `format_time_and_date_strings`) that belong to cseries, kept here because the module map put them here — these *are* written, under their own names |
| `0x576f40` `autopatch_get_proxy_settings`, `0x577310` `autopatch_launch_updater` | pure WinInet/WinHTTP and `CreateProcess` glue over OS structures, nothing Blam-shaped; deliberately not written |

Six of the 454 addresses therefore have no file: `0x4b8d30`, `0x4b8da0`, `0x4e2d4b`, `0x4e44e1`,
`0x576f40` and `0x577310`. The first four are not networking functions at all (two of them are
not functions); the last two are OS glue. Everything else in `0x440350..0x5781c0` is written.

## What the second review pass changed

Ordered by how much they would have mattered at a hook site.

1. **The whole client update-decode family had its arguments mis-wired.** `FUN_004ec590` takes
   the decode context in **EAX** and the destination buffer in **ECX**; eight files were passing
   it the `message_delta_decode_state` instead, or nothing at all, and none was passing a
   destination. That single error is why four files carried notes saying the decoded values
   "are never actually written by any traceable code path" — they are written, through the ECX
   pointer the call sites were not passing. Fixed in
   `player_update_client_local_player_update_from_network.c`,
   `player_update_client_local_player_vehicle_update_from_network.c`,
   `player_update_client_remote_player_action_update_from_network.c`,
   `player_update_client_remote_player_position_delta_from_network.c`,
   `player_update_remote_player_action_update_apply.c`,
   `network_channel_key_send_state.c`, `network_game_message_handle_ping_timestamp.c`,
   `network_game_client_apply_received_update.c` and
   `network_player_ping_field_update_and_report.c`.
2. **`handle_remote_player_action_update` (0x4e60c0) was being called without its EAX argument**
   in all three of its in-tree callers, so it would have applied whatever happened to be in EAX
   as the player's control record. Its third parameter is also a **byte**, not a dword: two
   callers pass a stack slot whose upper three bytes are stale (Ghidra renders this as
   `CONCAT31`), and the callee only tests AL.
3. **`player_update_client_remote_player_position_update_from_network` (0x4e6270) was being
   called without its EAX player index.** Its first two stack arguments are single bytes off
   the update header, not the dword at `header+4`.
4. **`player_update_client_local_player_update_from_network` (0x4e5390) latched the wrong three
   dwords into `player+0x0f0..0x0f8`** — it read them out of the `data_iterator` that happens to
   sit next to the decode scratch on the stack. They are the decoded position.
5. **`player_update_remote_player_action_update_apply` (0x4e5720) dropped the
   `local_player_index == -1` gate** from its delta-path inner validation.
6. **`network_server_check_machine_timeout` (0x4e0ef0)** passed the *live* player-table entry to
   `network_player_entry_is_valid` / `FUN_004df0e0` / `FUN_004de640` in the loop where the binary
   passes a **stack copy**, and cleared only `network_machine::flags` where the binary does a
   16-bit store over `flags` **and** `unknown_0f`.
7. **`network_machine_advance_connect_state` (0x4df290) was named and typed after the wrong
   object.** Its ESI is a `network_server_globals *`: the binary hands it to `0x4e19c0` as ECX,
   and that callee immediately computes `ecx + 0x3b8`, which is `network_server_globals::machines`.
   Renamed to `network_server_advance_connect_state` (row appended to
   `symbols/agent_phase4_networking.txt`). Its call to 0x4e19c0 also carried a phantom seventh
   stack argument that is really the EAX register argument.
8. **63 files did not compile** because they include `networking.h` without `math.h`/`game.h`
   (for `game_variant`) or without `<wchar.h>`. All fixed; the gate is clean for the first time
   since the `game_variant` field was added to `network_game_session`.
9. **Five deferred functions written** (0x4e5870, 0x4e5a30, 0x4e5d60, 0x4e6270, 0x4e6510), all
   from `objdump` disassembly rather than Ghidra's pseudo-C, which loses every register argument
   in that neighbourhood and splits the staging buffers into unrelated pseudo-locals.
10. **One TYPES-GAP closed and two more local typedefs folded** into `types/networking.h`
    (`message_delta_decode_state`, `network_map_list_entry`, `network_buffer_pair`), plus six new
    types recovered from the update-decode family.

Two quirks in the original that are preserved deliberately, not bugs in the rewrite:

* `player_update_client_remote_player_vehicle_update_from_network` (0x4e6510) counts
  `position_updates` (player+0x170) in its `***Ignoring` log line where the parallel
  `***Applying immediately` line counts `vehicle_updates` (player+0x1d0), and it reuses the two
  "Received pos update ..." format strings verbatim, so the vehicle path logs itself as a
  position update.
* `player_update_client_remote_player_vehicle_position_delta_from_network` (0x4e5d60) does not
  write its remapped index back into the header, has no `local_player_index == -1` gate, and does
  not zero its staging buffer before a baseline decode — all three differ from its "total" twin.

## What the third (whole-module) review pass changed

This pass covered the whole module for the first time: the two ranges above `0x4e6950` had only
just been written, and nothing had yet been checked across the module as a whole.

1. **The two array-of-structures codecs seeked from the wrong origin.**
   `message_delta_array_field_decode` (0x4e9330), `_encode` (0x4e95e0),
   `message_delta_float_array_encode` (0x4e9db0) and `message_delta_dword_array_decode`
   (0x4ea040) all alternate between a reserved block of per-element "changed" bits and the
   element payloads that follow it. Every seek in that loop is **absolute**, to
   `bit_stream::first_bit + a relative position` — Ghidra reloads `param_4+8` immediately before
   each one. All four files added those relative positions to the *current* cursor instead, so
   both regions walked forward and the flag bits landed on top of payload. Retranslated.
2. **`message_delta_array_field_encode` read every element offset one dword too high**, taking
   the destination from `fields[i]+0x08` and the source from `+0x0c` where the original takes
   them from `+0x04` and `+0x08`. Five other functions in the module already used `+0x04` and
   `+0x08` the right way round, which is what settled the naming of the two fields.
3. **`message_delta_float_array_encode` made a call the original never makes.** Its baseline
   branch is a goto chain: after a successful 32-bit write the remaining width is 0 and control
   jumps straight to the accumulate step. The draft fell through into the tail
   `bit_stream_write_bits` with a width of 0. Its trailing "nothing changed, rewind" seek had
   also been dropped entirely, so an unchanged array would not have been free.
4. **`player_update_history_play` (0x4e6ff0) restored two fields the original does not touch**:
   `object+0x11c` (`parent_object`) and `object+0x5c` (`position`). The restore starts at the
   unit's velocity; the position is deliberately left alone, because the non-vehicle branch
   immediately overwrites it with the server's position and the vehicle branch never writes it.
   Replaying a seated player would have teleported the unit to its own snapshot.
5. **`update_server_send_update` (0x4ddfb0) passed a format string as a trailing flag.**
   Its `player_update_history_log_write` call site had been transcribed as
   `(param_1, history_byte, tick, 0x66c404)`. `0x66c404` is the format string
   (`objdump -s` of `.rdata` reads `[%d]: Sent update [%d], [%d] ticks.`), pushed first;
   the declaration in that file was also the only one in the module that disagreed with the
   function's own definition.
6. **Eleven local `TYPES-GAP` typedefs folded into `types/networking.h`** and deleted from the
   `.c` files: `message_delta_field_type`, `message_delta_field_type_vtable`,
   `message_delta_array_descriptor`, `message_delta_array_field_list`,
   `message_delta_scalar_array_descriptor`, `message_delta_sample_ring_buffer`,
   `vector3d_lerp_table`, `waypoint_table`, `network_index_cache`,
   `server_browser_custom_options` and the three sub-codec records. Three files had declared
   three *different* partial versions of the field-type vtable; merging them showed that the
   header's "unnamed table of 0x18-byte records at 0x0069a304" was that same table's
   `registered` byte, one per entry, which closes that note.
7. **`message_delta_field_binding::unknown_04` / `_08` / `_0c` renamed** to
   `destination_offset`, `source_offset` and `initialized`, and `field_type` retyped from
   `void *` to `message_delta_field_type *`. Seven files updated.
8. **118 names appended to `symbols/agent_phase4_networking.txt`.** The file previously covered
   only `0x440350..0x4d8620`; every name this rewrite established above that, and that
   `symbols/functions.txt` does not already carry, is now recorded.
9. **13 new layout assertions** added to `out/phase4/networking_smoke.c`, one per folded type.
   Both the 64-bit and the `-m32` run are clean.

Verified correct and left alone (spot-checked line by line against `tools/pack.py`, and against
`objdump -d -M intel` where the decompile drops registers): `player_update_history_add`
(0x4e6b50, including the pre-incremented copy loop that starts at `vehicle_obj+0x04`),
`message_delta_encode_message` (0x4ec940 — the disassembly confirms the context block really is
`[esp+0x20 .. esp+0xb4)` and that `ctx[0] = 1` immediately before the header call is real),
`message_delta_encode_field` (0x4ec6a0), `network_index_cache_find_or_allocate_slot` (0x4e9c20),
`autopatch_version_string_is_outdated` (0x5781c0), `message_delta_quantize_float_to_int`
(0x4ea480) and `vector3d_from_yaw_pitch` (0x4ea7d0).

## Functions, with rewrite confidence

`nc` = name confidence, `rc` = rewrite confidence. All 448 files, in address order.

| address | function | bytes | nc | rc | UNSURE |
| --- | --- | --- | --- | --- | --- |
| `0x440350` | `network_message_block_build` | 86 | 0.3 | 0.5 | 2 |
| `0x4403b0` | `network_random_offset` | 99 | 0.3 | 0.4 | 2 |
| `0x440420` | `network_mutex_slot_allocate` | 59 | 0.6 | 0.85 | 0 |
| `0x440460` | `network_thread_create` | 163 | 0.55 | 0.8 | 0 |
| `0x440510` | `mutex_create` | 89 | 0.8 | 0.8 | 0 |
| `0x440570` | `network_address_to_string` | 149 | 0.75 | 0.85 | 0 |
| `0x440610` | `network_signal_quality_glyph` | 48 | 0.25 | 0.5 | 1 |
| `0x440670` | `network_stats_summary_log_open` | 417 | 0.55 | 0.55 | 5 |
| `0x440820` | `network_stats_summary_log_write` | 605 | 0.6 | 0.55 | 1 |
| `0x440a80` | `network_connection_stats_lookup_or_add` | 160 | 0.5 | 0.65 | 1 |
| `0x440b20` | `network_connection_stats_record_packet` | 505 | 0.45 | 0.4 | 3 |
| `0x440d20` | `network_connection_stats_end` | 82 | 0.5 | 0.5 | 0 |
| `0x440d80` | `network_connection_stats_log_tick` | 660 | 0.5 | 0.5 | 1 |
| `0x441300` | `network_channels_open` | 375 | 0.55 | 0.4 | 7 |
| `0x441480` | `network_channels_close` | 57 | 0.6 | 0.7 | 0 |
| `0x4414c0` | `network_dispatch_initialize` | 72 | 0.45 | 0.5 | 5 |
| `0x441510` | `network_hostname_thread_proc` | 34 | 0.55 | 0.8 | 0 |
| `0x441540` | `network_local_hostent_get` | 122 | 0.55 | 0.7 | 1 |
| `0x4415c0` | `network_initialize` | 281 | 0.55 | 0.5 | 8 |
| `0x4416e0` | `network_shutdown` | 493 | 0.6 | 0.5 | 1 |
| `0x4418d0` | `network_update` | 129 | 0.55 | 0.55 | 3 |
| `0x441960` | `network_channel_list_new` | 109 | 0.45 | 0.6 | 0 |
| `0x4419d0` | `network_channel_list_mark_readable` | 102 | 0.45 | 0.55 | 1 |
| `0x441a40` | `network_channel_list_add` | 184 | 0.55 | 0.55 | 1 |
| `0x441b00` | `network_channel_list_remove` | 175 | 0.55 | 0.55 | 1 |
| `0x441bb0` | `network_handle_registry_close_all` | 59 | 0.35 | 0.5 | 1 |
| `0x441bf0` | `network_receive_queue_new` | 136 | 0.55 | 0.65 | 1 |
| `0x441c80` | `network_receive_queue_free` | 88 | 0.5 | 0.4 | 2 |
| `0x441ce0` | `network_channel_get_remote_address` | 276 | 0.6 | 0.5 | 1 |
| `0x441ed0` | `network_channel_receive_callback` | 94 | 0.5 | 0.4 | 2 |
| `0x441f60` | `network_channel_attempt_connect` | 214 | 0.4 | 0.3 | 1 |
| `0x442040` | `network_receive_queue_close_socket` | 76 | 0.4 | 0.45 | 1 |
| `0x442090` | `network_listen_connection_request_handler` | 217 | 0.55 | 0.55 | 2 |
| `0x442170` | `network_listen_start` | 50 | 0.5 | 0.45 | 1 |
| `0x4421b0` | `network_listen_accept_pending_connection` | 149 | 0.5 | 0.35 | 4 |
| `0x442250` | `network_listen_reject_pending_connection` | 50 | 0.45 | 0.45 | 1 |
| `0x4b5d70` | `master_server_process_pending_requests` | 508 | 0.4 | 0.25 | 8 |
| `0x4b6000` | `master_server_connection_start` | 101 | 0.55 | 0.35 | 2 |
| `0x4b6070` | `master_server_connection_wait_thread` | 221 | 0.45 | 0.3 | 4 |
| `0x4b6160` | `join_game_ticker_string_copy` | 63 | 0.45 | 0.3 | 2 |
| `0x4b61c0` | `server_browser_filter_panel_set_mode` | 429 | 0.35 | 0.2 | 1 |
| `0x4b65f0` | `server_list_reset` | 103 | 0.5 | 0.35 | 8 |
| `0x4b6660` | `master_server_list_refresh_request` | 83 | 0.5 | 0.5 | 3 |
| `0x4b66c0` | `master_server_ensure_list_connection` | 103 | 0.4 | 0.35 | 3 |
| `0x4b6730` | `server_browser_latch_join_target` | 161 | 0.4 | 0.3 | 6 |
| `0x4b67e0` | `server_browser_list_row_populate` | 478 | 0.4 | 0.2 | 4 |
| `0x4b69c0` | `server_browser_list_row_gather` | 533 | 0.4 | 0.25 | 3 |
| `0x4b6be0` | `server_list_compare_by_string_key` | 59 | 0.35 | 0.35 | 1 |
| `0x4b6c20` | `server_list_compare_by_mapname` | 162 | 0.5 | 0.35 | 3 |
| `0x4b6da0` | `server_list_compare_by_ping_then_hostname` | 205 | 0.5 | 0.4 | 4 |
| `0x4b6f20` | `server_list_compare_by_mapname_then_hostname` | 135 | 0.5 | 0.4 | 1 |
| `0x4b7080` | `server_browser_server_passes_filter` | 716 | 0.5 | 0.3 | 3 |
| `0x4b7360` | `server_list_scroll_clamp` | 58 | 0.45 | 0.5 | 1 |
| `0x4b73a0` | `server_browser_ui_refresh` | 51 | 0.45 | 0.4 | 2 |
| `0x4b73e0` | `server_browser_player_list_populate` | 254 | 0.5 | 0.3 | 3 |
| `0x4b74e0` | `server_browser_selected_variant_description_build` | 207 | 0.35 | 0.2 | 3 |
| `0x4b75b0` | `server_browser_open` | 870 | 0.5 | 0.15 | 16 |
| `0x4b7b20` | `server_list_scroll_page_up` | 134 | 0.5 | 0.45 | 3 |
| `0x4b7bb0` | `server_list_scroll_page_down` | 138 | 0.5 | 0.45 | 3 |
| `0x4b7d80` | `server_browser_filter_widget_clicked` | 385 | 0.5 | 0.3 | 1 |
| `0x4b7f10` | `server_browser_column_header_update` | 89 | 0.4 | 0.4 | 0 |
| `0x4b7f70` | `server_browser_filter_headers_refresh` | 378 | 0.4 | 0.15 | 1 |
| `0x4b80f0` | `join_game_server_browser_tick` | 2261 | 0.5 | 0.15 | 24 |
| `0x4b8a00` | `ticker_text_buffer_reset` | 81 | 0.30 | 0.55 | 0 |
| `0x4b8a60` | `ticker_text_buffer_append` | 213 | 0.5 | 0.5 | 1 |
| `0x4b8b40` | `ticker_text_buffer_advance` | 489 | 0.4 | 0.3 | 6 |
| `0x4ba270` | `network_join_hostname_resolved_callback` | 174 | 0.35 | 0.35 | 4 |
| `0x4ba320` | `network_join_request_resolve_host` | 819 | 0.45 | 0.35 | 6 |
| `0x4ba760` | `server_list_mutex_try_lock` | 54 | 0.5 | 0.85 | 0 |
| `0x4ba7a0` | `server_list_mutex_unlock` | 31 | 0.5 | 0.7 | 0 |
| `0x4ba7c0` | `server_list_result_reset` | 87 | 0.4 | 0.6 | 3 |
| `0x4ba820` | `server_list_result_count_get` | 73 | 0.5 | 0.85 | 0 |
| `0x4ba870` | `dynamic_pointer_array_find_index` | 37 | 0.35 | 0.7 | 0 |
| `0x4ba8a0` | `dynamic_pointer_array_add_unique` | 150 | 0.35 | 0.55 | 5 |
| `0x4ba940` | `dynamic_pointer_array_remove_at` | 42 | 0.35 | 0.8 | 0 |
| `0x4ba970` | `server_browser_sort_comparator_select` | 50 | 0.5 | 0.75 | 5 |
| `0x4ba9c0` | `server_browser_result_array_sort` | 157 | 0.45 | 0.45 | 3 |
| `0x4baa60` | `server_browser_total_players_compute` | 117 | 0.5 | 0.55 | 1 |
| `0x4baae0` | `server_browser_query_results_ingest` | 104 | 0.5 | 0.4 | 2 |
| `0x4d7980` | `network_bandwidth_graph_reset` | 74 | 0.45 | 0.75 | 0 |
| `0x4d79d0` | `network_bandwidth_graph_accumulate_sent` | 116 | 0.4 | 0.55 | 0 |
| `0x4d7a50` | `network_bandwidth_graph_accumulate_received` | 116 | 0.4 | 0.55 | 0 |
| `0x4d7ad0` | `network_bandwidth_graph_update` | 701 | 0.4 | 0.4 | 0 |
| `0x4d7d90` | `network_bandwidth_graph_set_units_command` | 73 | 0.45 | 0.6 | 0 |
| `0x4d7de0` | `network_bandwidth_graph_instance_init` | 58 | 0.5 | 0.75 | 0 |
| `0x4d7e20` | `network_bandwidth_graph_instance_update_layout` | 596 | 0.4 | 0.4 | 4 |
| `0x4d8080` | `network_bandwidth_graph_instance_history_reset` | 183 | 0.4 | 0.55 | 0 |
| `0x4d8140` | `network_bandwidth_graph_find_peak_sample` | 114 | 0.4 | 0.75 | 0 |
| `0x4d81c0` | `network_bandwidth_graph_update_columns` | 605 | 0.35 | 0.45 | 0 |
| `0x4d8430` | `network_bandwidth_graph_new_sample` | 145 | 0.4 | 0.6 | 0 |
| `0x4d84d0` | `network_bandwidth_graph_tick` | 100 | 0.45 | 0.65 | 0 |
| `0x4d8540` | `network_bandwidth_rate_compute` | 212 | 0.5 | 0.6 | 2 |
| `0x4d8620` | `network_stats_overlay_draw` | 1021 | 0.6 | 0.35 | 9 |
| `0x4d8a20` | `network_bandwidth_unit_name_to_index` | 44 | 0.55 | 0.75 | 1 |
| `0x4d8a50` | `network_bandwidth_direction_name_to_index` | 44 | 0.45 | 0.75 | 1 |
| `0x4d8a80` | `network_session_create` | 236 | 0.5 | 0.55 | 4 |
| `0x4d8b70` | `network_session_destroy` | 64 | 0.5 | 0.6 | 1 |
| `0x4d8bb0` | `network_client_state_dispatch` | 66 | 0.4 | 0.6 | 0 |
| `0x4d8c10` | `network_client_connect_progress_percent` | 52 | 0.4 | 0.55 | 3 |
| `0x4d8c50` | `network_connection_endpoint_set` | 149 | 0.4 | 0.55 | 1 |
| `0x4d8cf0` | `network_connection_initiate` | 470 | 0.5 | 0.45 | 10 |
| `0x4d8ed0` | `chimera__on_connect` | 380 | 0.45 | 0.4 | 11 |
| `0x4d9050` | `network_session_info_packet_send` | 292 | 0.4 | 0.35 | 6 |
| `0x4d9190` | `network_session_player_table_index_apply` | 139 | 0.35 | 0.3 | 4 |
| `0x4d9220` | `network_send_join_request_packet` | 279 | 0.5 | 0.4 | 4 |
| `0x4d9340` | `network_disconnect_notify_dropped_machines` | 104 | 0.4 | 0.3 | 6 |
| `0x4d93b0` | `network_connection_retransmit_if_overdue` | 80 | 0.4 | 0.35 | 1 |
| `0x4d9400` | `network_connection_send_keepalive` | 178 | 0.4 | 0.4 | 3 |
| `0x4d94c0` | `network_game_settings_packet_send` | 568 | 0.4 | 0.2 | 11 |
| `0x4d9700` | `network_session_player_join_notify` | 218 | 0.4 | 0.4 | 5 |
| `0x4d97e0` | `network_session_disconnect_with_error` | 31 | 0.4 | 0.5 | 1 |
| `0x4d9800` | `network_game_settings_packet_receive` | 240 | 0.4 | 0.5 | 4 |
| `0x4d98f0` | `player_data_iterator_advance` | 110 | 0.3 | 0.2 | 3 |
| `0x4d9960` | `network_connection_finalize_join` | 892 | 0.5 | 0.25 | 13 |
| `0x4d9ce0` | `network_client_timer_default_or_disconnect` | 51 | 0.35 | 0.5 | 1 |
| `0x4d9d20` | `network_game_state_update_receive` | 261 | 0.4 | 0.3 | 10 |
| `0x4d9e30` | `network_player_join_finalize` | 156 | 0.55 | 0.3 | 11 |
| `0x4d9ed0` | `network_client_timer_schedule` | 122 | 0.4 | 0.5 | 1 |
| `0x4d9f50` | `network_game_settings_ack_send` | 455 | 0.4 | 0.25 | 4 |
| `0x4da130` | `network_game_record_message_send` | 288 | 0.4 | 0.4 | 3 |
| `0x4da250` | `network_staged_message_commit` | 199 | 0.4 | 0.5 | 3 |
| `0x4da320` | `network_game_action_apply` | 787 | 0.4 | 0.35 | 4 |
| `0x4da770` | `network_game_search_entry_is_fresh` | 94 | 0.45 | 0.55 | 1 |
| `0x4da7d0` | `network_game_search_results_add_or_update` | 582 | 0.5 | 0.45 | 2 |
| `0x4daa20` | `network_join_handshake_tick` | 348 | 0.4 | 0.25 | 6 |
| `0x4dab80` | `network_join_connect_retry_tick` | 550 | 0.4 | 0.3 | 3 |
| `0x4dadb0` | `network_host_presence_broadcast_tick` | 313 | 0.4 | 0.45 | 3 |
| `0x4daef0` | `network_host_lobby_tick` | 138 | 0.45 | 0.75 | 0 |
| `0x4daf80` | `network_game_client_update` | 377 | 0.55 | 0.35 | 8 |
| `0x4db100` | `network_host_channel_service_tick` | 120 | 0.4 | 0.4 | 1 |
| `0x4db180` | `network_game_process_incoming_messages` | 388 | 0.6 | 0.4 | 4 |
| `0x4db310` | `network_client_identity_tick` | 422 | 0.4 | 0.3 | 6 |
| `0x4db4c0` | `network_join_status_text_update` | 359 | 0.4 | 0.5 | 1 |
| `0x4db630` | `network_incoming_item_dispatch` | 121 | 0.4 | 0.4 | 5 |
| `0x4db6b0` | `network_game_message_decode_dispatch` | 336 | 0.5 | 0.3 | 1 |
| `0x4db870` | `network_game_action_queue_drain` | 303 | 0.4 | 0.2 | 7 |
| `0x4db9a0` | `network_game_client_decode_beacon_reply` | 119 | 0.5 | 0.3 | 4 |
| `0x4dba20` | `network_game_client_decode_pong_reply` | 114 | 0.55 | 0.35 | 4 |
| `0x4dbaa0` | `network_player_ping_field_update_and_report` | 342 | 0.35 | 0.25 | 6 |
| `0x4dbc00` | `network_game_decode_settings_request` | 177 | 0.35 | 0.3 | 5 |
| `0x4dbcc0` | `network_game_client_decode_join_accepted` | 127 | 0.55 | 0.3 | 4 |
| `0x4dbd40` | `network_game_client_decode_connect_rejected` | 124 | 0.4 | 0.3 | 3 |
| `0x4dbdc0` | `network_game_client_decode_join_complete` | 141 | 0.55 | 0.4 | 1 |
| `0x4dbe50` | `network_game_client_decode_settings_or_ack` | 218 | 0.35 | 0.3 | 3 |
| `0x4dbf30` | `network_game_client_decode_player_config_value` | 117 | 0.3 | 0.3 | 3 |
| `0x4dbfb0` | `network_game_client_decode_and_discard_join_message` | 101 | 0.3 | 0.4 | 2 |
| `0x4dc020` | `network_game_client_decode_and_discard_ingame_message` | 99 | 0.3 | 0.4 | 2 |
| `0x4dc090` | `network_game_client_decode_join_finalize_message` | 130 | 0.35 | 0.4 | 1 |
| `0x4dc120` | `network_game_client_decode_join_finalize_ack` | 112 | 0.35 | 0.4 | 4 |
| `0x4dc190` | `network_game_client_decode_state_update_chunk` | 172 | 0.3 | 0.3 | 3 |
| `0x4dc240` | `network_game_client_decode_player_join_chunk` | 156 | 0.3 | 0.3 | 2 |
| `0x4dc2e0` | `network_game_client_decode_player_slot_chunk` | 183 | 0.3 | 0.3 | 1 |
| `0x4dc3a0` | `network_game_client_decode_sync_complete` | 108 | 0.35 | 0.35 | 0 |
| `0x4dc410` | `network_game_message_decode_replicated_command` | 147 | 0.25 | 0.3 | 1 |
| `0x4dc4b0` | `network_game_message_decode_ingame_notification` | 166 | 0.3 | 0.3 | 1 |
| `0x4dc560` | `network_address_parse_port` | 119 | 0.5 | 0.55 | 0 |
| `0x4dc5e0` | `network_address_string_normalize` | 327 | 0.55 | 0.55 | 1 |
| `0x4dc730` | `network_address_string_is_valid` | 91 | 0.45 | 0.45 | 0 |
| `0x4dc790` | `network_game_client_connect_to_address` | 314 | 0.5 | 0.45 | 2 |
| `0x4dc8d0` | `network_client_begin_connect` | 210 | 0.4 | 0.3 | 5 |
| `0x4dc9b0` | `network_channel_new` | 295 | 0.5 | 0.55 | 5 |
| `0x4dcae0` | `network_channel_delete` | 329 | 0.55 | 0.5 | 2 |
| `0x4dcc30` | `network_channel_reliable_pool_ensure_capacity` | 367 | 0.4 | 0.6 | 0 |
| `0x4dcdb0` | `network_channel_reliable_pool_store` | 142 | 0.4 | 0.45 | 0 |
| `0x4dce40` | `network_channel_queue_message` | 194 | 0.35 | 0.3 | 4 |
| `0x4dcf10` | `network_channel_incoming_read_item` | 372 | 0.4 | 0.25 | 2 |
| `0x4dd090` | `network_channel_remove_child` | 123 | 0.4 | 0.45 | 2 |
| `0x4dd110` | `network_channel_service` | 292 | 0.4 | 0.35 | 2 |
| `0x4dd240` | `network_channel_service_light` | 225 | 0.35 | 0.4 | 1 |
| `0x4dd330` | `network_channel_service_retransmit_only` | 89 | 0.35 | 0.45 | 1 |
| `0x4dd390` | `network_channel_remote_address_or_default` | 84 | 0.6 | 0.75 | 0 |
| `0x4dd3f0` | `network_channel_service_close_if_disconnected` | 60 | 0.35 | 0.45 | 0 |
| `0x4dd430` | `network_channel_new_child` | 172 | 0.4 | 0.6 | 2 |
| `0x4dd4e0` | `network_channel_listen_service` | 591 | 0.3 | 0.2 | 4 |
| `0x4dd730` | `network_channel_transmit` | 504 | 0.5 | 0.35 | 7 |
| `0x4dd930` | `network_channel_record_timestamp` | 66 | 0.5 | 0.7 | 0 |
| `0x4dd980` | `network_channel_stream_init` | 80 | 0.35 | 0.55 | 2 |
| `0x4dd9d0` | `network_channel_scan_retransmit_timeouts` | 368 | 0.5 | 0.3 | 3 |
| `0x4ddb60` | `network_channel_stream_flush` | 318 | 0.4 | 0.35 | 3 |
| `0x4ddca0` | `network_game_is_active` | 27 | 0.5 | 0.85 | 0 |
| `0x4ddcc0` | `network_channel_key_resolve_target` | 81 | 0.3 | 0.25 | 1 |
| `0x4ddd20` | `network_channel_short_disconnect_timeout` | 27 | 0.4 | 0.7 | 1 |
| `0x4ddd40` | `network_game_server_host_create` | 73 | 0.5 | 0.4 | 3 |
| `0x4ddd90` | `network_host_shutdown_or_defer` | 184 | 0.35 | 0.3 | 5 |
| `0x4dde50` | `network_client_globals_create` | 31 | 0.4 | 0.8 | 0 |
| `0x4dde70` | `network_client_globals_dispose` | 89 | 0.45 | 0.45 | 1 |
| `0x4dded0` | `network_client_update_dispatch` | 224 | 0.35 | 0.35 | 5 |
| `0x4ddfb0` | `update_server_send_update` | 989 | 0.6 | 0.15 | 17 |
| `0x4de390` | `network_client_rejoin_check` | 132 | 0.3 | 0.3 | 0 |
| `0x4de420` | `network_message_read_sized_buffer` | 68 | 0.25 | 0.25 | 1 |
| `0x4de470` | `network_game_session_reset` | 105 | 0.6 | 0.65 | 1 |
| `0x4de4e0` | `network_player_entry_add` | 259 | 0.4 | 0.45 | 1 |
| `0x4de5f0` | `network_player_entry_update` | 75 | 0.4 | 0.3 | 1 |
| `0x4de640` | `network_player_entry_remove` | 130 | 0.4 | 0.35 | 0 |
| `0x4de6d0` | `network_game_scenario_load_request` | 405 | 0.55 | 0.25 | 5 |
| `0x4de870` | `network_channel_key_open` | 75 | 0.35 | 0.3 | 0 |
| `0x4de8c0` | `network_channel_key_close` | 55 | 0.35 | 0.3 | 2 |
| `0x4de900` | `network_player_entry_find` | 65 | 0.4 | 0.35 | 0 |
| `0x4de950` | `network_channel_key_send_state` | 148 | 0.3 | 0.2 | 1 |
| `0x4de9f0` | `network_player_entry_validate` | 138 | 0.4 | 0.55 | 0 |
| `0x4dea80` | `network_game_get_random_player_name` | 112 | 0.7 | 0.45 | 3 |
| `0x4deaf0` | `network_prepare_challenge_packet` | 85 | 0.5 | 0.6 | 2 |
| `0x4deb50` | `network_timer_advance` | 93 | 0.4 | 0.55 | 0 |
| `0x4debb0` | `network_timer_increment_clamped` | 32 | 0.35 | 0.4 | 0 |
| `0x4debd0` | `network_timer_decrement_floored` | 28 | 0.35 | 0.5 | 0 |
| `0x4debf0` | `network_timer_start` | 68 | 0.4 | 0.6 | 0 |
| `0x4dec40` | `network_game_server_host_new` | 338 | 0.55 | 0.45 | 9 |
| `0x4deda0` | `network_game_server_host_dispose` | 284 | 0.5 | 0.35 | 3 |
| `0x4deec0` | `network_map_cycle_list_broadcast` | 190 | 0.4 | 0.25 | 3 |
| `0x4def80` | `network_host_update_tick` | 239 | 0.3 | 0.15 | 1 |
| `0x4df070` | `network_password_field_set` | 27 | 0.25 | 0.2 | 0 |
| `0x4df090` | `network_machine_timer_start` | 75 | 0.4 | 0.5 | 0 |
| `0x4df0e0` | `network_game_settings_broadcast_send` | 209 | 0.35 | 0.25 | 3 |
| `0x4df1c0` | `network_host_send_scenario_announcement` | 204 | 0.35 | 0.2 | 3 |
| `0x4df290` | `network_server_advance_connect_state` | 72 | 0.35 | 0.3 | 2 |
| `0x4df2e0` | `network_game_client_game_settings_updated` | 541 | 0.6 | 0.15 | 15 |
| `0x4df510` | `network_host_full_state_broadcast` | 302 | 0.4 | 0.25 | 4 |
| `0x4df640` | `network_host_round_reset` | 65 | 0.35 | 0.4 | 2 |
| `0x4df690` | `network_machine_reset` | 82 | 0.4 | 0.4 | 1 |
| `0x4df6f0` | `network_player_name_collision_check` | 60 | 0.4 | 0.4 | 0 |
| `0x4df730` | `network_game_generate_unique_random_name` | 96 | 0.55 | 0.45 | 0 |
| `0x4df790` | `network_player_assign_random_color` | 165 | 0.45 | 0.4 | 0 |
| `0x4df840` | `network_game_session_finalize_and_add_player` | 185 | 0.4 | 0.45 | 6 |
| `0x4df900` | `network_object_record_last_sender` | 75 | 0.35 | 0.4 | 2 |
| `0x4df950` | `network_game_broadcast_team_object_updates` | 183 | 0.4 | 0.35 | 8 |
| `0x4dfa10` | `network_game_server_handoff_object_ownership` | 512 | 0.4 | 0.35 | 11 |
| `0x4dfc10` | `network_object_release_ownership_claim` | 119 | 0.4 | 0.45 | 5 |
| `0x4dfc90` | `network_game_server_handle_client_join` | 643 | 0.4 | 0.3 | 14 |
| `0x4dff20` | `network_machine_check_build_version` | 72 | 0.4 | 0.5 | 3 |
| `0x4dff70` | `network_game_client_apply_position_update` | 264 | 0.4 | 0.35 | 3 |
| `0x4e0080` | `network_client_check_connection_quality` | 504 | 0.4 | 0.4 | 4 |
| `0x4e0280` | `network_game_client_apply_received_update` | 308 | 0.5 | 0.3 | 6 |
| `0x4e03c0` | `network_game_server_per_frame_tick` | 183 | 0.4 | 0.35 | 6 |
| `0x4e0480` | `network_game_any_team_empty` | 104 | 0.35 | 0.4 | 2 |
| `0x4e04f0` | `network_game_all_machines_have_player` | 145 | 0.4 | 0.45 | 1 |
| `0x4e0590` | `network_client_connection_handshake_tick` | 383 | 0.35 | 0.3 | 8 |
| `0x4e0720` | `network_game_server_load_scenario` | 106 | 0.45 | 0.55 | 3 |
| `0x4e0790` | `network_debug_fill_canary_buffer` | 117 | 0.4 | 0.6 | 0 |
| `0x4e0810` | `network_machine_find_by_id` | 46 | 0.55 | 0.6 | 0 |
| `0x4e0850` | `network_server_validate_join_request` | 141 | 0.4 | 0.4 | 3 |
| `0x4e08e0` | `network_server_password_is_set` | 34 | 0.5 | 0.6 | 0 |
| `0x4e0910` | `network_server_password_set` | 28 | 0.5 | 0.6 | 0 |
| `0x4e0930` | `network_server_password_get` | 24 | 0.5 | 0.6 | 0 |
| `0x4e0950` | `network_server_build_game_info_packet` | 345 | 0.4 | 0.3 | 9 |
| `0x4e0ab0` | `network_join_request_reset_state` | 63 | 0.35 | 0.7 | 2 |
| `0x4e0af0` | `network_server_notify_or_resend_challenge` | 150 | 0.4 | 0.4 | 3 |
| `0x4e0b90` | `network_machine_clear_flag_by_id` | 63 | 0.4 | 0.35 | 5 |
| `0x4e0bd0` | `network_server_build_full_game_info_packet` | 281 | 0.4 | 0.35 | 5 |
| `0x4e0cf0` | `network_object_owner_team_index_desired` | 63 | 0.35 | 0.4 | 2 |
| `0x4e0d30` | `network_server_count_machines_and_resolve_address` | 438 | 0.35 | 0.25 | 2 |
| `0x4e0ef0` | `network_server_check_machine_timeout` | 727 | 0.4 | 0.25 | 10 |
| `0x4e11d0` | `network_server_service_machines_tick` | 191 | 0.4 | 0.3 | 2 |
| `0x4e1290` | `network_channel_drain_bitstream` | 375 | 0.4 | 0.25 | 3 |
| `0x4e1450` | `network_server_resend_challenge_periodic` | 142 | 0.4 | 0.5 | 0 |
| `0x4e14e0` | `network_server_any_machine_awaiting_flag` | 53 | 0.35 | 0.5 | 0 |
| `0x4e1520` | `network_server_status_periodic_print` | 120 | 0.45 | 0.6 | 2 |
| `0x4e15a0` | `network_server_heartbeat_tick` | 627 | 0.4 | 0.3 | 4 |
| `0x4e1820` | `network_game_session_reset_defaults` | 82 | 0.4 | 0.55 | 1 |
| `0x4e1880` | `network_server_count_connected_machines` | 38 | 0.45 | 0.55 | 0 |
| `0x4e18b0` | `network_channel_dispatch_bitstream_unit` | 119 | 0.4 | 0.45 | 3 |
| `0x4e1930` | `network_session_send_to_machine` | 137 | 0.5 | 0.3 | 3 |
| `0x4e19c0` | `network_session_broadcast_to_all` | 183 | 0.45 | 0.35 | 2 |
| `0x4e1a80` | `network_session_broadcast_to_flagged` | 193 | 0.45 | 0.35 | 1 |
| `0x4e1b50` | `network_game_broadcast_state_snapshot` | 156 | 0.4 | 0.55 | 2 |
| `0x4e1bf0` | `network_game_broadcast_player_set_changed` | 103 | 0.4 | 0.35 | 3 |
| `0x4e1c60` | `network_game_process_incoming_message` | 614 | 0.7 | 0.35 | 4 |
| `0x4e1f40` | `network_client_drain_queued_updates` | 288 | 0.4 | 0.25 | 5 |
| `0x4e20b0` | `network_game_message_handle_ping_timestamp` | 83 | 0.35 | 0.25 | 5 |
| `0x4e2110` | `network_game_message_handle_keepalive` | 177 | 0.4 | 0.5 | 1 |
| `0x4e21d0` | `network_game_server_handle_join_password` | 546 | 0.4 | 0.3 | 10 |
| `0x4e2400` | `network_game_server_handle_join_confirm` | 200 | 0.4 | 0.3 | 5 |
| `0x4e24d0` | `network_game_message_handle_settings_relay` | 86 | 0.35 | 0.3 | 2 |
| `0x4e2530` | `network_game_message_handle_player_count_broadcast` | 80 | 0.35 | 0.3 | 2 |
| `0x4e2580` | `network_game_message_handle_player_entry_update` | 88 | 0.35 | 0.3 | 3 |
| `0x4e25e0` | `network_game_message_handle_handshake_forward` | 74 | 0.35 | 0.3 | 2 |
| `0x4e2630` | `network_game_message_handle_build_version` | 100 | 0.35 | 0.3 | 2 |
| `0x4e26a0` | `network_game_message_handle_retry_schedule` | 83 | 0.35 | 0.35 | 1 |
| `0x4e2700` | `network_game_server_handle_info_request` | 136 | 0.35 | 0.3 | 3 |
| `0x4e2790` | `network_game_client_handle_map_data` | 126 | 0.35 | 0.25 | 3 |
| `0x4e2810` | `network_game_client_handle_settings_relay` | 88 | 0.35 | 0.3 | 2 |
| `0x4e2870` | `network_game_client_handle_retry_schedule` | 83 | 0.35 | 0.3 | 1 |
| `0x4e28d0` | `network_game_message_handle_settings_relay_role2` | 86 | 0.35 | 0.3 | 2 |
| `0x4e2930` | `network_game_message_handle_join_finalize_ack_role2` | 86 | 0.35 | 0.3 | 0 |
| `0x4e2990` | `console_command_bool_get_set` | 258 | 0.8 | 0.4 | 1 |
| `0x4e2aa0` | `sv_map_reset` | 125 | 0.8 | 0.6 | 2 |
| `0x4e2b20` | `sv_map` | 234 | 0.9 | 0.4 | 6 |
| `0x4e2c10` | `sv_players_find_by_team_index_desired` | 87 | 0.4 | 0.55 | 0 |
| `0x4e2c70` | `sv_players` | 244 | 0.75 | 0.3 | 12 |
| `0x4e2e50` | `sv_status` | 121 | 0.9 | 0.5 | 5 |
| `0x4e2ed0` | `sv_name` | 274 | 0.9 | 0.4 | 6 |
| `0x4e2ff0` | `sv_password` | 259 | 0.9 | 0.45 | 5 |
| `0x4e3100` | `sv_single_flag_force_reset` | 86 | 0.6 | 0.6 | 1 |
| `0x4e3160` | `network_banlist_load` | 538 | 0.8 | 0.75 | 6 |
| `0x4e3380` | `network_banlist_save` | 348 | 0.8 | 0.7 | 3 |
| `0x4e34e0` | `network_banlist_print` | 210 | 0.6 | 0.65 | 1 |
| `0x4e35c0` | `network_banlist_add_ban` | 276 | 0.5 | 0.6 | 2 |
| `0x4e36e0` | `network_session_autoban_player` | 233 | 0.45 | 0.65 | 5 |
| `0x4e37d0` | `ban_list_find_by_name` | 73 | 0.6 | 0.75 | 0 |
| `0x4e3820` | `ban_list_check_and_reject_player` | 98 | 0.6 | 0.7 | 1 |
| `0x4e3890` | `ban_list_get_or_add_entry` | 117 | 0.55 | 0.75 | 1 |
| `0x4e3910` | `sv_kick` | 115 | 0.9 | 0.6 | 1 |
| `0x4e3990` | `sv_ban` | 231 | 0.85 | 0.6 | 0 |
| `0x4e3a80` | `sv_ban_penalty` | 589 | 0.9 | 0.65 | 2 |
| `0x4e3cd0` | `sv_tk_grace` | 104 | 0.9 | 0.75 | 1 |
| `0x4e3d40` | `sv_tk_cooldown` | 104 | 0.9 | 0.75 | 1 |
| `0x4e3db0` | `sv_banlist_file` | 280 | 0.9 | 0.6 | 2 |
| `0x4e3ed0` | `network_buffer_pair_pool_clear` | 89 | 0.4 | 0.6 | 2 |
| `0x4e3f30` | `string_is_numeric` | 56 | 0.6 | 0.75 | 0 |
| `0x4e3f70` | `sv_find_client_by_name_or_index` | 194 | 0.6 | 0.6 | 2 |
| `0x4e4040` | `string_trim_whitespace` | 91 | 0.6 | 0.7 | 1 |
| `0x4e40a0` | `network_log_path_resolve` | 79 | 0.5 | 0.55 | 4 |
| `0x4e40f0` | `network_game_start_new_server_from_profile` | 88 | 0.5 | 0.4 | 3 |
| `0x4e4150` | `network_game_start_new_server_with_name_and_password` | 504 | 0.65 | 0.45 | 17 |
| `0x4e4350` | `network_name_string_is_valid_for_mode` | 276 | 0.45 | 0.45 | 7 |
| `0x4e4470` | `map_list_matching_substring` | 113 | 0.55 | 0.45 | 2 |
| `0x4e4600` | `game_variant_list_matching_substring` | 528 | 0.55 | 0.4 | 4 |
| `0x4e4810` | `sv_friendly_fire` | 378 | 0.9 | 0.65 | 1 |
| `0x4e49a0` | `sv_timelimit` | 278 | 0.9 | 0.65 | 0 |
| `0x4e4ac0` | `sv_maxplayers` | 130 | 0.9 | 0.7 | 0 |
| `0x4e4b50` | `sv_rcon_password` | 164 | 0.9 | 0.7 | 1 |
| `0x4e4c00` | `rcon` | 448 | 0.85 | 0.5 | 1 |
| `0x4e4dc0` | `rcon_send_request` | 112 | 0.6 | 0.45 | 5 |
| `0x4e4e30` | `network_game_server_send_message_to_all_machines` | 192 | 0.7 | 0.35 | 8 |
| `0x4e4ef0` | `network_game_server_send_message_to_all_machines_ingame` | 12 | 0.5 | 0.3 | 1 |
| `0x4e4f00` | `network_server_handle_rcon_request` | 438 | 0.75 | 0.35 | 4 |
| `0x4e50c0` | `chimera__rcon_out` | 119 | 0.6 | 0.45 | 4 |
| `0x4e5140` | `network_client_handle_server_text_message` | 116 | 0.5 | 0.4 | 1 |
| `0x4e51c0` | `parse_time_duration_string` | 213 | 0.7 | 0.6 | 2 |
| `0x4e52c0` | `format_local_time_and_date` | 86 | 0.55 | 0.7 | 1 |
| `0x4e5320` | `format_time_and_date_strings` | 98 | 0.6 | 0.75 | 1 |
| `0x4e5390` | `player_update_client_local_player_update_from_network` | 243 | 0.85 | 0.35 | 4 |
| `0x4e5490` | `player_update_client_local_player_vehicle_update_from_network` | 396 | 0.85 | 0.3 | 4 |
| `0x4e5620` | `player_update_client_remote_player_action_update_from_network` | 251 | 0.55 | 0.4 | 2 |
| `0x4e5720` | `player_update_remote_player_action_update_apply` | 336 | 0.4 | 0.35 | 3 |
| `0x4e5870` | `player_update_client_remote_player_total_biped_update_from_network` | 440 | 0.75 | 0.75 | 3 |
| `0x4e5a30` | `player_update_client_remote_player_total_vehicle_update_from_network` | 519 | 0.75 | 0.7 | 2 |
| `0x4e5c40` | `player_update_client_remote_player_position_delta_from_network` | 277 | 0.55 | 0.45 | 1 |
| `0x4e5d60` | `player_update_client_remote_player_vehicle_position_delta_from_network` | 312 | 0.55 | 0.7 | 3 |
| `0x4e5ea0` | `player_update_history_log_write` | 126 | 0.75 | 0.7 | 2 |
| `0x4e5f20` | `player_update_history_log_printf_filtered` | 85 | 0.55 | 0.55 | 3 |
| `0x4e5f80` | `player_update_history_log_set_name_filter` | 76 | 0.4 | 0.65 | 1 |
| `0x4e5fe0` | `player_update_queue_flush_by_name` | 209 | 0.35 | 0.5 | 4 |
| `0x4e60c0` | `handle_remote_player_action_update` | 420 | 0.9 | 0.35 | 7 |
| `0x4e6270` | `player_update_client_remote_player_position_update_from_network` | 657 | 0.85 | 0.75 | 3 |
| `0x4e6510` | `player_update_client_remote_player_vehicle_update_from_network` | 1074 | 0.8 | 0.7 | 3 |
| `0x4e6950` | `player_update_history_play_for_update_index` | 86 | 0.5 | 0.6 | 1 |
| `0x4e69b0` | `is_local_player_update_in_order` | 103 | 0.85 | 0.75 | 1 |
| `0x4e6a20` | `is_remote_player_update_in_order` | 121 | 0.85 | 0.65 | 1 |
| `0x4e6aa0` | `player_update_queue_offset_from_head` | 98 | 0.45 | 0.5 | 1 |
| `0x4e6b10` | `player_update_history_destroy` | 51 | 0.4 | 0.75 | 1 |
| `0x4e6b50` | `player_update_history_add` | 973 | 0.9 | 0.55 | 4 |
| `0x4e6f20` | `player_update_history_free_all` | 49 | 0.6 | 0.85 | 1 |
| `0x4e6f60` | `player_update_history_find_and_prune` | 132 | 0.5 | 0.85 | 1 |
| `0x4e6ff0` | `player_update_history_play` | 1846 | 0.9 | 0.4 | 13 |
| `0x4e7730` | `player_update_history_play_local_player` | 168 | 0.55 | 0.5 | 2 |
| `0x4e77e0` | `network_client_send_local_player_updates` | 174 | 0.7 | 0.5 | 1 |
| `0x4e7890` | `build_remote_player_action_update` | 693 | 0.4 | 0.3 | 7 |
| `0x4e7b50` | `build_remote_player_transform_update` | 565 | 0.4 | 0.25 | 1 |
| `0x4e7d90` | `build_player_full_resync_update` | 508 | 0.5 | 0.55 | 1 |
| `0x4e7f90` | `network_player_update_history_log_write` | 95 | 0.75 | 0.8 | 1 |
| `0x4e7ff0` | `network_event_feed_queue_append` | 68 | 0.5 | 0.45 | 2 |
| `0x4e8040` | `network_event_feed_flush` | 405 | 0.45 | 0.35 | 2 |
| `0x4e81e0` | `build_local_player_position_update` | 257 | 0.9 | 0.45 | 1 |
| `0x4e82f0` | `build_local_player_vehicle_update` | 467 | 0.9 | 0.4 | 2 |
| `0x4e84d0` | `build_remote_player_vehicle_update` | 538 | 0.5 | 0.3 | 3 |
| `0x4e86f0` | `build_remote_player_vehicle_attachment_update` | 709 | 0.45 | 0.3 | 1 |
| `0x4e9330` | `message_delta_array_field_decode` | 496 | 0.5 | 0.55 | 1 |
| `0x4e95e0` | `message_delta_array_field_encode` | 512 | 0.5 | 0.55 | 2 |
| `0x4e9c20` | `network_index_cache_find_or_allocate_slot` | 161 | 0.4 | 0.4 | 2 |
| `0x4e9cd0` | `network_index_cache_insert_if_free` | 74 | 0.4 | 0.35 | 2 |
| `0x4e9d20` | `network_index_cache_get` | 25 | 0.45 | 0.75 | 1 |
| `0x4e9d40` | `network_index_cache_remove` | 101 | 0.45 | 0.4 | 1 |
| `0x4e9db0` | `message_delta_float_array_encode` | 651 | 0.5 | 0.5 | 2 |
| `0x4ea040` | `message_delta_dword_array_decode` | 468 | 0.45 | 0.5 | 1 |
| `0x4ea480` | `message_delta_quantize_float_to_int` | 80 | 0.5 | 0.55 | 2 |
| `0x4ea7d0` | `vector3d_from_yaw_pitch` | 221 | 0.5 | 0.7 | 1 |
| `0x4eabe0` | `message_delta_encode_vector3d` | 744 | 0.5 | 0.3 | 4 |
| `0x4eb370` | `vector3d_lerp_by_mode_denominator` | 289 | 0.45 | 0.4 | 1 |
| `0x4eb4a0` | `vector3d_quantize` | 123 | 0.45 | 0.4 | 1 |
| `0x4eb560` | `waypoint_table_quantize_initialize` | 288 | 0.4 | 0.25 | 2 |
| `0x4eb890` | `message_delta_decode_vector3d_indexed` | 423 | 0.4 | 0.25 | 4 |
| `0x4ebd50` | `message_delta_parameters_protocol_free_registered` | 72 | 0.55 | 0.8 | 0 |
| `0x4ebda0` | `message_delta_parameters_protocol_reload_from_config_file` | 96 | 0.85 | 0.8 | 2 |
| `0x4ebe00` | `message_delta_parameters_protocol_register` | 328 | 0.6 | 0.55 | 2 |
| `0x4ebf50` | `message_delta_parameters_protocol_send_update` | 172 | 0.55 | 0.5 | 4 |
| `0x4ec000` | `message_delta_parameters_protocol_receive_update` | 78 | 0.5 | 0.4 | 0 |
| `0x4ec050` | `message_delta_parameters_protocol_format_registered_values` | 115 | 0.5 | 0.7 | 0 |
| `0x4ec0d0` | `message_delta_parameters_protocol_parse_value_from_config` | 62 | 0.5 | 0.45 | 3 |
| `0x4ec110` | `message_delta_parameters_protocol_find_registered` | 136 | 0.5 | 0.75 | 0 |
| `0x4ec1a0` | `message_delta_parameters_protocol_pack_values` | 138 | 0.5 | 0.6 | 0 |
| `0x4ec230` | `message_delta_parameters_protocol_format_received_values` | 177 | 0.45 | 0.55 | 1 |
| `0x4ec2f0` | `message_delta_protocol_initialize` | 60 | 0.5 | 0.7 | 1 |
| `0x4ec330` | `message_delta_parameters_protocol_dump_to_config_file` | 90 | 0.85 | 0.7 | 1 |
| `0x4ec390` | `message_delta_definitions_invoke_field_bindings` | 43 | 0.35 | 0.55 | 0 |
| `0x4ec3d0` | `message_delta_metrics_dump` | 120 | 0.55 | 0.35 | 5 |
| `0x4ec450` | `message_delta_encode_single_value` | 52 | 0.35 | 0.3 | 2 |
| `0x4ec490` | `message_delta_decode_begin` | 120 | 0.4 | 0.6 | 0 |
| `0x4ec510` | `message_delta_decode_array_field` | 121 | 0.4 | 0.5 | 0 |
| `0x4ec590` | `message_delta_decode_compound_field` | 102 | 0.4 | 0.5 | 0 |
| `0x4ec600` | `message_delta_decode_compound_field_forced` | 109 | 0.35 | 0.5 | 0 |
| `0x4ec670` | `message_delta_decode_compound_field_staged` | 142 | 0.35 | 0.45 | 1 |
| `0x4ec700` | `message_delta_field_bindings_invoke` | 66 | 0.3 | 0.55 | 1 |
| `0x4ec750` | `message_delta_definitions_teardown_field_bindings` | 45 | 0.3 | 0.6 | 0 |
| `0x4ec790` | `message_delta_field_layout_compute_size` | 165 | 0.5 | 0.6 | 2 |
| `0x4ec840` | `message_delta_field_bindings_lazy_init` | 184 | 0.4 | 0.55 | 1 |
| `0x4ec900` | `message_delta_field_bindings_teardown` | 63 | 0.35 | 0.6 | 1 |
| `0x4ec940` | `message_delta_encode_message` | 536 | 0.6 | 0.3 | 4 |
| `0x4ecb60` | `message_delta_encode_prepare_item` | 160 | 0.45 | 0.35 | 1 |
| `0x4ecc00` | `message_delta_encode_all_fields` | 246 | 0.5 | 0.35 | 1 |
| `0x4ecd00` | `message_delta_encode_message_header` | 210 | 0.5 | 0.4 | 6 |
| `0x4ecde0` | `message_delta_encode_field` | 140 | 0.5 | 0.35 | 3 |
| `0x4ece70` | `message_delta_decode_message_header` | 504 | 0.5 | 0.55 | 7 |
| `0x4ed070` | `message_delta_decode_field_changed_flags` | 341 | 0.4 | 0.45 | 1 |
| `0x4ed1d0` | `message_delta_read_changed_subfields` | 183 | 0.5 | 0.55 | 0 |
| `0x4ed290` | `message_delta_decode_static_fields` | 117 | 0.4 | 0.55 | 0 |
| `0x4ed310` | `message_delta_sample_record_and_append` | 54 | 0.3 | 0.45 | 2 |
| `0x4ed350` | `message_delta_sample_ring_buffer_average` | 55 | 0.35 | 0.6 | 1 |
| `0x4ed390` | `message_delta_sample_ring_buffer_append` | 169 | 0.35 | 0.55 | 0 |
| `0x575fa0` | `autopatch_temp_name_generate` | 68 | 0.45 | 0.55 | 1 |
| `0x575ff0` | `network_session_host_reject_or_cleanup_client` | 162 | 0.3 | 0.25 | 7 |
| `0x576100` | `network_session_host_start_info_set` | 126 | 0.45 | 0.4 | 0 |
| `0x576180` | `server_browser_custom_options_pack` | 788 | 0.4 | 0.3 | 1 |
| `0x5764a0` | `server_browser_custom_options_unpack` | 680 | 0.4 | 0.35 | 1 |
| `0x5767d0` | `server_browser_gametype1_flags_pack` | 179 | 0.35 | 0.5 | 1 |
| `0x576890` | `server_browser_gametype1_flags_unpack` | 109 | 0.35 | 0.45 | 1 |
| `0x576920` | `server_browser_gametype2_flags_pack` | 57 | 0.3 | 0.5 | 0 |
| `0x576960` | `server_browser_gametype5_flags_pack` | 41 | 0.3 | 0.5 | 0 |
| `0x576990` | `server_browser_gametype5_flags_unpack` | 48 | 0.3 | 0.5 | 0 |
| `0x5769c0` | `server_browser_gametype3_flags_pack` | 92 | 0.35 | 0.5 | 0 |
| `0x576a20` | `server_browser_gametype3_flags_unpack` | 75 | 0.35 | 0.5 | 0 |
| `0x576ad0` | `autopatch_download_complete_callback` | 165 | 0.5 | 0.6 | 0 |
| `0x576b80` | `autopatch_download_worker_thread` | 59 | 0.55 | 0.7 | 1 |
| `0x576bc0` | `autopatch_download_pool_tick` | 103 | 0.5 | 0.55 | 2 |
| `0x576c30` | `autopatch_download_pool_initialize` | 380 | 0.55 | 0.5 | 3 |
| `0x576db0` | `autopatch_download_pool_shutdown` | 164 | 0.55 | 0.45 | 3 |
| `0x576e60` | `autopatch_download_start` | 160 | 0.55 | 0.45 | 4 |
| `0x576f00` | `autopatch_download_get_result` | 61 | 0.55 | 0.55 | 0 |
| `0x5771c0` | `autopatch_proxy_initialize` | 26 | 0.5 | 0.6 | 2 |
| `0x5771e0` | `autopatch_version_check_request` | 96 | 0.5 | 0.45 | 3 |
| `0x577240` | `autopatch_check_for_update_start` | 205 | 0.5 | 0.4 | 3 |
| `0x5776d0` | `registry_get_halo_version` | 134 | 0.7 | 0.7 | 0 |
| `0x577760` | `registry_get_dist_id` | 108 | 0.7 | 0.7 | 0 |
| `0x577850` | `network_session_host_start` | 159 | 0.45 | 0.25 | 7 |
| `0x5778f0` | `network_session_host_dispose` | 74 | 0.4 | 0.35 | 4 |
| `0x577940` | `network_session_host_update` | 120 | 0.4 | 0.35 | 6 |
| `0x577e40` | `network_session_host_dispatch_message` | 241 | 0.4 | 0.3 | 6 |
| `0x578190` | `autopatch_current_version_string_get` | 38 | 0.6 | 0.7 | 0 |
| `0x5781c0` | `autopatch_version_string_is_outdated` | 238 | 0.5 | 0.55 | 1 |

## Open questions for hook verification

1. **Do the four array codecs really seek from `bit_stream::first_bit`, and is
   `message_delta_field_type::reserved_bits` the size of the flag block?** The whole-module pass
   rewrote 0x4e9330, 0x4e95e0, 0x4e9db0 and 0x4ea040 on that reading, and it is the reading the
   decompile supports, but nothing in the image calls these four directly — they are reached only
   through the `.data` field-type records — so no call site cross-checks it. Break on 0x4e9db0
   with a `point3d` field and watch `+0x0c`/`+0x10` of the stream across the element loop.
2. **How big is `message_delta_decode_state` really?** `message_delta_decode_message_header`
   (0x4ece70) writes `+0x20`, `+0x24`, `+0x28` and `+0x2c` of the record the header declares as
   0x20 bytes. Either the struct is at least 0x30, or the header is writing past it into the
   caller's frame. Read 0x30 bytes at the state pointer after a header decode.
3. **`network_session_broadcast_to_all` (0x4e19c0) has an undeclared EAX argument.** Its
   disassembly reads `ECX` (the `network_server_globals *`, which the declaration models) and
   `EAX` (`mov ebx,eax` at 0x4e19cd, which it does not). Every call site sets EAX to the encoded
   size the preceding `message_delta_encode_message` returned. Until it is confirmed, every one of
   the ten rewritten calls is one argument short.
4. **Does `FUN_004ec590` write its whole ECX destination, or only the subfields the message
   declares changed?** Every remote-player handler except 0x4e5d60 zeroes the buffer first, which
   suggests partial writes; 0x4e5d60 does not, and would then hand stale stack bytes to
   `player_update_client_remote_player_vehicle_update_from_network`. Breakpoint 0x4ec590 and diff
   the ECX block across a baseline vehicle update.
5. **`player_update_history_play` (0x4e6ff0) returns a pointer in three of its exit paths** — the
   vehicle object, the vehicle ack, or whatever `FUN_0056cd10` left in EAX — and an averaged tick
   count in the fourth. `player_update_history_play_for_update_index` and
   `player_update_history_play_local_player` both discard it. Confirm nothing downstream reads
   it, because if something does, the four paths are not interchangeable.

Still open from the second pass, and unchanged:

* **Which of `header->baseline_id` (+0x05) and `header->control_sequence` (+0x08) is
  `player+0x15c` meant to hold?** The two "total" handlers forward +0x08 and the two stand-alone
  position handlers forward +0x05 into the same slot, and `is_remote_player_update_in_order`
  (0x4e6a20) compares it against the previous value with a +/-3 window.
* **`handle_remote_player_action_update` (0x4e60c0) stores a leftover register into
  `control_source+0x20`** (`mov WORD PTR [ebx+0x20],ax`, with AX still holding the low half of
  `player_data->size * index`).
* **Is `player+0x190` really a `vehicle_update_body`, including `parent_or_tag` at +0x00?**
* **`unit_snap_position_if_far` (0x4772e0) arity disagreement with `src/units`**: `src/units`
  declares two parameters, the call at 0x4e649b pushes three.
* **The type-1 advertised-options sub-codec disagrees with itself**: 0x5767d0 packs an `int32_t`
  time limit at +0x04 while 0x576890 unpacks a byte pair into the same place, with values
  (8/7, 16/14, 24/21, 40/35, 80/70) that are nothing like the time limits.
