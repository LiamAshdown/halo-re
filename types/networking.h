// Blam networking module (halo.exe 1.0.10 retail, 0x440350..0x5781c0, 454 functions).
// Offsets in comments are byte offsets from the struct base and were pinned from the
// pointer arithmetic in the decompiled module unless a comment says otherwise.
//
// The module covers seven fairly separate subsystems:
//   transport         0x440350..0x442290 and 0x4dc9b0..0x4ddca0  sockets, channels, bit streams
//   session/handshake 0x4d8a80..0x4dcae0 and 0x4ddca0..0x4e2990  server, client, machines
//   console/ban list  0x4e2990..0x4e5390                         sv_* commands, banned<x>.txt
//   player update     0x4e5390..0x4e8a00                         update history and replay
//   message delta     0x4e9330..0x4ed3f0                         the delta-compression protocol
//   server browser    0x4b5d70..0x4bab30                         GameSpy front end
//   autopatch         0x575fa0..0x5781c0                         bungie.net version check
//
// This header depends on types/memory.h (bit_stream, circular_buffer, datum_index) and on
// types/game.h (game_variant). Types another module already owns are never redefined here.
#pragma pack(push, 1)
typedef unsigned char uint8_t; typedef signed char int8_t; typedef unsigned short uint16_t; typedef short int16_t;
typedef unsigned int uint32_t; typedef int int32_t;

// ---------------------------------------------------------------------------
// constants
// ---------------------------------------------------------------------------
typedef enum network_constants {
    k_network_maximum_machines = 16,         // network_server_globals::machines
    k_network_maximum_players = 16,          // network_game_session::players
    k_network_maximum_children = 16,         // network_channel::children
    k_network_mutex_table_count = 32,        // 0x440420 scans 32 slots of 0x28
    k_network_thread_table_count = 32,       // 0x440460 scans 32 slots of 0x08
    k_network_channel_list_capacity = 64,    // fd_set FD_SETSIZE
    k_network_pending_connection_count = 30, // 0x442090 refuses the 31st
    k_network_connection_stats_count = 255,  // 0x440a80 stops growing at 0xff
    k_network_game_search_result_count = 9,  // 0x4da7d0 walks nine records
    k_network_game_search_expiry_ms = 6000,  // 0x4da7d0 and 0x4da770
    k_network_channel_stream_bits = 0x2880,  // 0x4dd980 writes last_bit 0x287f
    k_network_update_history_maximum = 64,   // 0x4e6b50 refuses past 63 nodes
    k_network_autopatch_download_slots = 2,  // 0x006ef93c holds two 0x14 records
    k_network_message_definition_count = 56, // 0x0065d440..0x0065d520, 4 bytes each
    k_network_ban_penalty_tiers = 4          // sv_ban_penalty, 0x00699574
} network_constants;

// s_network_address::size. The only two values the module ever writes.
typedef enum network_address_size {
    k_network_address_size_ipv4 = 4,
    k_network_address_size_ipv6 = 16
} network_address_size;

// The codes network_channel_get_remote_address and the listen handler report.
typedef enum network_error_code {
    k_network_error_none = 0,
    k_network_listen_error_queue_full = 2,   // 0x442090, more than 30 pending
    k_network_listen_error_bad_payload = 3,  // 0x442090, fewer than 4 payload bytes
    k_network_error_no_address = -15,        // 0x441ce0 stores 0xfff1
    k_network_error_connect_failed = -16     // 0x441f60 stores 0xfff0 and returns it in AX
} network_error_code;

// ---------------------------------------------------------------------------
// s_network_address
// Layout comes straight out of network_address_to_string (0x440570): it reads
// halves [0]..[7] as the address, [8] as the size and [9] as the port, and the
// IPv4 print order is byte 3, 2, 1, 0 -- so an IPv4 address is a host-order
// uint32 whose high byte is the first dotted quad.
// ---------------------------------------------------------------------------
typedef struct s_network_address {
    uint32_t ipv4;             // 0x00 a.b.c.d packed high byte first
    uint32_t ipv6_1;           // 0x04 only used when size == 16
    uint32_t ipv6_2;           // 0x08
    uint32_t ipv6_3;           // 0x0c
    uint16_t size;             // 0x10 4 or 16, see network_address_size
    uint16_t port;             // 0x12 host order
} s_network_address;           // size 0x14
// global 0x006a3f38: char network_address_string[0x100]  the shared format buffer

// ---------------------------------------------------------------------------
// named mutex and worker thread tables  (0x440420 network_mutex_slot_allocate,
// 0x440510 mutex_create, 0x440460 network_thread_create)
// Both are static arrays with an in-use byte; the allocator returns a pointer to
// the slot, and that pointer is what callers keep as the handle.
// ---------------------------------------------------------------------------
typedef struct network_mutex_record {
    void *handle;              // 0x00 CreateMutexA result
    char name[0x20];           // 0x04 snprintf of "mutex_%ld"
    uint8_t in_use;            // 0x24
    uint8_t pad_25[3];         // 0x25
} network_mutex_record;        // size 0x28
// global 0x006f0db0: network_mutex_record network_mutex_table[32]
// global 0x006f0cac: int32_t network_mutex_name_counter

typedef struct network_thread_record {
    void *handle;              // 0x00 CreateThread result, 0x4000 byte stack
    uint8_t in_use;            // 0x04
    uint8_t pad_05[3];         // 0x05
} network_thread_record;       // size 0x08
// global 0x006f0cb0: network_thread_record network_thread_table[32]
// network_thread_create maps its flag byte to a priority: bit1 -> -1, bit2 -> +1, else 0.

// The registry 0x441bb0 sweeps on shutdown: a second, parallel 32-slot table whose
// entries point back into network_thread_table and carry their own in-use byte, so a
// slot can be registered here while the thread record itself is already free. Folded
// here from src/networking/network_handle_registry_close_all.c during the review pass.
typedef struct network_handle_registry_slot {
    network_thread_record *record; // 0x00 NULL when the slot is free
    uint8_t registered;            // 0x04 the in-use flag of this slot, independent of
                                   //      record->in_use
    uint8_t pad_05[3];             // 0x05
} network_handle_registry_slot;    // size 0x08

// ---------------------------------------------------------------------------
// bandwidth statistics  (0x440670 / 0x440820 the summary log, 0x440a80 / 0x440b20 /
// 0x440d20 / 0x440d80 the per-connection log)
// Field names come from the two tab-separated log headers the module writes:
//   Game Summary: Map, Length, Avg # Players, Packets Sent/Received, ... Bytes/packet
//   gamespy:      Time, Packets Sent, Total Sent, Reliable Sent, Unreliable Sent,
//                 Resends Sent   printed from +0x3c, +0x24, +0x2c, +0x24 minus +0x2c, +0x30
// The counters at 0x14..0x20 and 0x34..0x38 are never reset; the ones at
// 0x24..0x30 and 0x3c..0x40 are zeroed after every log row.
// ---------------------------------------------------------------------------
typedef struct network_connection_statistics {
    int32_t connected_duration_ms;   // 0x00 += now - active_since_ms when the link ends
    int32_t active_since_ms;         // 0x04 0 while the entry is not live
    uint8_t active;                  // 0x08
    uint8_t pad_09[3];               // 0x09
    int32_t connection_id;           // 0x0c first half of the lookup key
    int16_t connection_key;          // 0x10 second half of the lookup key
    int16_t pad_12;                  // 0x12
    int32_t bytes_sent;              // 0x14 lifetime
    int32_t bytes_received;          // 0x18 lifetime
    int32_t reliable_bytes_sent;     // 0x1c lifetime
    int32_t resend_bytes_sent;       // 0x20 lifetime
    int32_t interval_bytes_sent;     // 0x24 the Total Sent column, cleared per row
    int32_t interval_bytes_received; // 0x28
    int32_t interval_reliable_bytes_sent; // 0x2c the Reliable Sent column
    int32_t interval_resend_bytes_sent;   // 0x30 the Resends Sent column
    int32_t packets_sent;            // 0x34 lifetime
    int32_t packets_received;        // 0x38 lifetime
    int32_t interval_packets_sent;   // 0x3c the Packets Sent column
    int32_t interval_packets_received; // 0x40
} network_connection_statistics;     // size 0x44
// global 0x0087bec0: network_connection_statistics network_connection_stats[255]
// global 0x006f14bc: int32_t network_connection_stats_count
// global 0x006f14b8: void *network_connection_stats_log_file  FILE *, "gamespy <date>.xls"
// global 0x006a6140: void *network_summary_log_file           FILE *, "Game Summary <date>.xls"
// global 0x006f14b4: uint8_t network_statistics_logging_enabled
// global 0x0087ac06: uint8_t debug_log_level  -- ONE declaration shared by interface.h and
//   networking.h (R01; cseries.h calls it the shell debug level). All 11 .text accesses are
//   byte-wide: cmp BYTE ...,0x3 (0x440829, 0x440b24, 0x440d20, 0x4e0756), cmp BYTE ...,0x4
//   (0x489c4d, 0x496a86), mov al (0x440670, 0x440d80, 0x449450, 0x4d9960) and the shell's
//   mov BYTE PTR ds:0x87ac06,bl (0x540fac). Console output (0x496a80) needs > 3, network
//   statistics logging needs > 2. Formerly interface.h "int32_t console_verbosity" and
//   networking.h "int16_t network_statistics_level".
// global 0x006869bc: uint8_t network_summary_log_needs_open
// global 0x006869bd: uint8_t network_connection_log_needs_open
// global 0x006a4038: int32_t network_connection_log_last_row_ms  one row per 100 ms
// global 0x006a8148: int32_t network_connection_log_start_ms

// The whole-session accumulators the Game Summary row is computed from.
typedef struct network_summary_statistics {
    int32_t start_ms;             // 0x00 global 0x0087bea0
    int32_t bytes_sent;           // 0x04 global 0x0087bea4
    int32_t bytes_received;       // 0x08 global 0x0087bea8
    int32_t packets_sent;         // 0x0c global 0x0087beac
    int32_t packets_received;     // 0x10 global 0x0087beb0
    int32_t player_count_total;   // 0x14 global 0x0087beb4, sum of session player counts
    int32_t player_count_samples; // 0x18 global 0x0087beb8, the Avg # Players divisor
} network_summary_statistics;     // size 0x1c
// global 0x0087bea0: network_summary_statistics network_summary_stats

// ---------------------------------------------------------------------------
// network_receive_queue  (0x441bf0 new, 0x441c80 free, 0x441ed0 receive callback)
// The socket-side half of a channel: a socket plus a 0x10001-byte circular buffer
// named "received_data_queue". network_channel_list_add uses +0x08 as the fd_set
// value and as the dedup key, so the two socket-shaped fields are distinct.
// ---------------------------------------------------------------------------
typedef struct network_receive_queue {
    int32_t socket;            // 0x00 0 until a connection is accepted or opened
    uint8_t data_ready;        // 0x04 set by the transport when a read can succeed
    uint8_t unknown_05;        // 0x05
    int16_t pad_06;            // 0x06
    int32_t socket_key;        // 0x08 -1 when unused; the fd_set entry value
    uint8_t flags;             // 0x0c bit0 connection oriented (buffer the payload),
                               //      bit1 alternate dedup path, bit2 readable,
                               //      bit3 currently in a network_channel_list
    uint8_t unknown_0d;        // 0x0d constructed as 0x14
    int16_t last_error;        // 0x0e see network_error_code
    circular_buffer *incoming; // 0x10 0x18 header plus 0x10001 bytes
    int32_t unknown_14;        // 0x14 constructed as -1
    int32_t unknown_18;        // 0x18
} network_receive_queue;       // size 0x1c

// ---------------------------------------------------------------------------
// network_channel_list  (0x441960 allocate, 0x441a40 add, 0x441b00 remove,
// 0x4419d0 the mark-readable pass)
// A Winsock fd_set plus a parallel array of the queues those sockets belong to.
// The fd_set part is byte for byte struct fd_set, so it has to stay first.
// ---------------------------------------------------------------------------
typedef struct network_channel_list {
    uint32_t fd_count;         // 0x000
    uint32_t fd_array[64];     // 0x004 the socket_key of each member queue
    network_receive_queue **entries; // 0x104 capacity pointers, GlobalAlloc backed
    int32_t capacity;          // 0x108 the constructor refuses more than 0x40
    int32_t last_index;        // 0x10c -1 when empty, else the highest used index
    int32_t unknown_110;       // 0x110
} network_channel_list;        // size 0x114

// ---------------------------------------------------------------------------
// network_channel_stream
// One direction of a channel. 0x4dd980 builds it: a bit_stream whose data points
// at the bytes immediately after the header, a 0x2880-bit capacity and an empty
// flag the transmit path clears once it has drained the stream.
// The base offsets are confirmed twice: network_channel_delete writes the same
// seven dwords plus a byte at channel+0x010 and at channel+0x544, and 0x4dd9d0
// computes the free space as (last_bit - byte_cursor*8 - bit_cursor) + 1.
// ---------------------------------------------------------------------------
typedef struct network_channel_stream {
    bit_stream stream;         // 0x000 data == (uint8_t *)this + 0x1d
    uint32_t capacity_bits;    // 0x018 0x2880
    uint8_t empty;             // 0x01c 1 while nothing is staged
    uint8_t data[0x510];       // 0x01d 0x2880 bits of staging buffer
    uint8_t unknown_52d[7];    // 0x52d never read by this module
} network_channel_stream;      // size 0x534

// ---------------------------------------------------------------------------
// network_channel_reliable_slot  (0x4dcc30 find-or-grow, 0x4dcdb0 store,
// 0x4dd9d0 the retransmit scan)
// The retransmission pool grows ten slots at a time; each slot owns two
// GlobalAlloc buffers whose capacities never fall below 100 bytes.
// ---------------------------------------------------------------------------
typedef struct network_channel_reliable_slot {
    uint8_t pending;           // 0x00 1 once a message is staged, cleared after the scan
    uint8_t pad_01[3];         // 0x01
    int32_t priority;          // 0x04 -1 when free; the scan sweeps priorities 0..9
    int32_t header_capacity;   // 0x08 byte capacity of header
    int32_t body_capacity;     // 0x0c byte capacity of body
    uint32_t header_bits;      // 0x10
    uint32_t body_bits;        // 0x14
    uint8_t *header;           // 0x18 header_capacity bytes
    uint8_t *body;             // 0x1c body_capacity bytes
} network_channel_reliable_slot; // size 0x20

// ---------------------------------------------------------------------------
// network_channel  (0x4dc9b0 new, 0x4dcae0 delete, 0x4dd110 service,
// 0x4dd730 transmit, 0x4dd9d0 retransmit scan)
// network_channel_new allocates 0xae4 bytes for a listening channel (flag bit0)
// and 0xa9c for a plain one (flag bit1), so everything from listen_list down is
// present only on a listening channel.
// ---------------------------------------------------------------------------
typedef enum network_channel_flags {
    k_network_channel_listening = 0x01,   // allocate the 0xae4 variant
    k_network_channel_client = 0x02,      // allocate the 0xa9c variant
    k_network_channel_transmit_pending = 0x04,
    k_network_channel_dead = 0x10,        // host_dispose skips a channel with this set
    k_network_channel_timed_out = 0x20    // 0x4dd110 sets it after 5000 ms of silence
} network_channel_flags;

typedef struct network_channel {
    network_receive_queue *endpoint; // 0x000
    int32_t last_activity_ms;  // 0x004 network_channel_record_timestamp, QPC milliseconds
    int32_t unknown_008;       // 0x008
    circular_buffer *incoming; // 0x00c named "transport-incoming"
    network_channel_stream outgoing;    // 0x010 the message staging stream: every queue/send
                               //       path does `lea esi,[channel+0x10]` before calling
                               //       0x4ddb60 (0x4d9108, 0x4d9698, 0x4d9791, 0x4d9bcd,
                               //       0x4da0af, 0x4da2b4, 0x4dae96, 0x4dce68, 0x4de254), and
                               //       the service routines flush it with mode 1
    network_channel_stream retransmit;  // 0x544 the retransmission staging stream: only
                               //       network_channel_scan_retransmit_timeouts writes it
                               //       (`lea esi,[ebp+0x544]` at 0x4dda95), and the service
                               //       routines flush it with mode 0
                               // Review-pass correction: these were named in/out on the
                               // assumption that +0x010 was the receive side. Nothing receives
                               // through either stream -- incoming bytes land in the
                               // `incoming` circular buffer at +0x00c -- so both are outgoing.
    int32_t reliable_count;    // 0xa78 number of slots in reliable
    network_channel_reliable_slot *reliable; // 0xa7c
    int32_t send_budget;       // 0xa80 reset to 0xe0 on every service pass
    uint32_t budget_base_tick; // 0xa84 GetTickCount at the last budget reset
    int32_t rate_index;        // 0xa88 index into the rate table at 0x00697edc
    uint32_t flags;            // 0xa8c see network_channel_flags
    int32_t unknown_a90;       // 0xa90
    struct network_channel *parent; // 0xa94 set on a channel accepted by a listener
    uint8_t connected;         // 0xa98 the flag every send path tests
    uint8_t pad_a99[3];        // 0xa99
    network_channel_list *listen_list;    // 0xa9c listening channels only, from here down
    struct network_channel *children[16]; // 0xaa0
    uint8_t listening;         // 0xae0 set by network_listen_start
    uint8_t child_busy;        // 0xae1 paired with a child channel connected flag
    uint8_t pad_ae2[2];        // 0xae2
} network_channel;             // size 0xae4 listening, 0xa9c otherwise
// global 0x006f14c4: int32_t network_game_socket   the main UDP socket, port at 0x00698208
// global 0x006f14c8: int32_t network_query_socket  the query socket, port at 0x0069820c
// global 0x006869b0: uint32_t network_local_address  byte swapped before binding
// global 0x006869be: uint8_t network_channels_open_ok
// global 0x00697edc: int32_t network_rate_table[]   bits per second, indexed by rate_index
// global 0x00710308: int32_t network_rate_override  non-zero replaces the table lookup
// global 0x0071c2cc: int32_t network_bit_chunk_size  defaulted to 11 by 0x4dd980

// ---------------------------------------------------------------------------
// network_resolved_address  (0x4dd390 network_channel_remote_address_or_default
// and the whole 0x4dbc00..0x4dc4b0 decode-handler cluster)
// The six-dword scratch every decode handler declares on its own stack and hands
// to 0x4dd390, which fills it via network_channel_get_remote_address. The first
// five dwords are a plain s_network_address; 0x4dd390's failure path also zeroes
// the sixth, which nothing in the module reads back.
// The review pass renamed this from the rewriters' network_message_decode_result:
// 0x4dd390 writes WORD [record+0x10] = 4, which is k_network_address_size_ipv4 and
// not a "state", and every decode handler compares dword [record+0x00] against an
// expected peer address (0x4dbcc0: `mov eax,[esp+0x10] / cmp eax,[ecx]`), i.e. the
// handlers are checking that the message came from the peer they expect.
// ---------------------------------------------------------------------------
typedef struct network_resolved_address {
    s_network_address address; // 0x00 ipv4 at +0x00, size at +0x10, port at +0x12
    uint32_t unknown_14;       // 0x14 only ever zeroed, by 0x4dd390's default path
} network_resolved_address;    // size 0x18

// ---------------------------------------------------------------------------
// network_timer_pair  (0x4deb50 advance, 0x4debb0 start, 0x4debd0 increment,
// 0x4debf0 decrement)
// A [remaining, last tick] millisecond pair. The four helpers above take a pointer
// to one and nothing else, so the pair is the whole type; callers embed it at
// various offsets inside network_machine and network_client_globals.
// ---------------------------------------------------------------------------
typedef struct network_timer_pair {
    int32_t remaining_ms;      // 0x00
    int32_t last_tick_ms;      // 0x04
} network_timer_pair;          // size 0x08

// ---------------------------------------------------------------------------
// network_map_cycle_entry  (0x4deec0 network_map_cycle_list_broadcast)
// The 8-byte record staged per iterated item into the scratch array at 0x00861d60
// before the type 0x35 broadcast. The source item type is not established.
// ---------------------------------------------------------------------------
typedef struct network_map_cycle_entry {
    uint8_t unknown_00;        // 0x00 from item+0x67
    uint8_t pad_01[3];         // 0x01
    uint32_t unknown_04;       // 0x04 from item+0xdc
} network_map_cycle_entry;     // size 0x08

// ---------------------------------------------------------------------------
// network_scenario_load_request  (0x4de6d0 network_game_scenario_load_request)
// Staged on the caller's stack, then copied to scenario_load_staging+8 and handed
// to scenario_load. The size is pinned by the two 0x43-dword loops (0x10c bytes)
// that zero and then copy it; the field offsets come from Ghidra's own locals
// (local_110, local_10c, local_10a, local_108, local_104[260]).
// ---------------------------------------------------------------------------
typedef struct network_scenario_load_request {
    uint32_t unknown_00;       // 0x00 zeroed only
    int16_t unknown_04;        // 0x04 zeroed only
    int16_t difficulty;        // 0x06 the campaign difficulty (formerly "seed"): the single
                               //      player path stores the pending difficulty 0x00696564
                               //      here (0x4c9973/0x4c9984); 0x4c95f0 copies the request
                               //      to game globals +0x08, so this lands at game globals
                               //      +0x0e, which the checkpoint loader compares with
                               //      0x00696564 (0x5382c7). The network path sets it to 1,
                               //      then overwrites it from session->unknown_19e.
    uint32_t salt;             // 0x08 defaults to 0xdeadbeef, else session+0x3a4
    char map_name[0x100];      // 0x0c strncpy of 0x7f from session+0x84
} network_scenario_load_request; // size 0x10c

// ---------------------------------------------------------------------------
// network_client_begin_connect_scratch  (0x4dbb10 network_client_begin_connect)
// The stack block whose address is handed to chimera__on_connect as session_info.
// Kept as a named type because the two functions have to agree on its shape; the
// memcpy that fills config_template writes 0x7ff dwords, which overruns the array
// as the binary has it. See src/networking/network_client_begin_connect.c.
// ---------------------------------------------------------------------------
typedef struct network_client_begin_connect_scratch {
    uint16_t pad_00;           // 0x0000
    uint16_t name[8];          // 0x0002 UTF-16
    uint16_t name_terminator;  // 0x0012 forced NUL
    uint32_t config_template[1008]; // 0x0014
    uint8_t trailing_byte;     // 0x0fd4
} network_client_begin_connect_scratch;

// Callback shapes the transport calls through. Folded from the per-file copies.
typedef void (*network_channel_accept_callback)(network_receive_queue *entry);
typedef char (*network_game_message_handler)(void);

// ---------------------------------------------------------------------------
// network_pending_connection  (0x442090 queue, 0x4421b0 accept, 0x442250 reject)
// The listening socket parks up to 30 incoming requests here; accept pops the
// most recent one, so the queue behaves as a stack.
// ---------------------------------------------------------------------------
typedef struct network_pending_connection {
    int32_t reply_socket;        // 0x00 the socket the accept or reject code is written to
    int32_t transport_handle;    // 0x04
    uint32_t remote_address;     // 0x08
    uint16_t remote_port;        // 0x0c
    uint16_t pad_0e;             // 0x0e
    uint32_t first_payload_word; // 0x10 first dword of the request, at least 4 bytes
} network_pending_connection;    // size 0x14
// global 0x0087bc20: network_pending_connection network_pending_connections[30]
// global 0x006f16d0: int32_t network_pending_connection_count

// The scratch block 0x4421b0 builds on its own stack and hands to the transport-layer
// accept thunk; it is never stored anywhere. Folded here from
// src/networking/network_listen_accept_pending_connection.c during the review pass.
typedef struct network_listen_accept_config {
    int32_t result;             // 0x00 written 0 before the call
    void *receive_callback;     // 0x04 network_channel_receive_callback (0x441ed0)
    void *error_callback;       // 0x08 UNSURE: &LAB_00441f30, the disconnect path
    void *connect_callback;     // 0x0c UNSURE: FUN_0044ad80
} network_listen_accept_config; // size 0x10

// ---------------------------------------------------------------------------
// network_player_entry  (0x4de4e0 add, 0x4de5f0 update, 0x4de640 remove,
// 0x4de900 find, 0x4de9f0 validate, 0x4df790 colour assignment)
// One row of the session player table. The (machine_index, machine_player_index)
// pair is the lookup key and slot_index is the row the entry occupies.
// The validator accepts a name of at most 12 wide characters and requires
// machine_index in 0..15 and machine_player_index == 0, which is the one player
// per machine the PC build allows.
// ---------------------------------------------------------------------------
typedef struct network_player_entry {
    uint16_t name[12];           // 0x00 UTF-16, NUL terminated inside the field
    int16_t color_index;         // 0x18 0xffff when unused; 0x4df790 picks a free one
    int16_t icon_index;          // 0x1a reset to -1 with color_index in session reset/remove/settings_updated; no
                                 //    reader; OpenSauce/Chimera network player layout {primary_color_index,
                                 //    icon_index} at 0x18/0x1a
    int8_t machine_index;        // 0x1c 0xff when the row is free
    int8_t machine_player_index; // 0x1d always 0 on the PC build
    int8_t team_index;           // 0x1e network_game_any_team_empty counts entries per team 0/1; sv_players prints
                                 //    red/blue from it; finalize_and_add_player assigns a team when -1
    int8_t slot_index;           // 0x1f 0xff when free, else the row index in players[]
} network_player_entry;          // size 0x20

// ---------------------------------------------------------------------------
// network_game_session  (0x4de470 network_channel_table_initialize, 0x4e1820 defaults)
// The block both the server and the client embed: the advertised server name, the
// live game variant and the 16-row player table. The variant offset is confirmed
// twice -- 0x4e1820 copies 0x98 bytes of game_engine_pending_variant here, and
// game.h already records session+0x10c as the variant with its game_engine_index
// at +0x13c, which is exactly +0x30 into game_variant.
// ---------------------------------------------------------------------------
typedef struct network_game_session {
    void *message_callback;    // 0x000 0x4dec40 stores 0x004e1410 here
    uint8_t unknown_004[0x7a]; // 0x004 zeroed at init, not resolved
    uint16_t unknown_07e;      // 0x07e zeroed by 0x4e1820
    int32_t unknown_080;       // 0x080 zeroed by 0x4e1820
    char server_name[64];      // 0x084 strncpy of 0x3f chars plus a forced NUL at 0x0c3
    uint8_t unknown_0c4[0x40]; // 0x0c4
    game_variant variant;      // 0x104 see types/game.h
    uint8_t unknown_19c;       // 0x19c
    uint8_t maximum_players;   // 0x19d initialized to 16
    int16_t difficulty;        // 0x19e network_game_scenario_load_request 0x4de6d0 request.difficulty =
                               //    session+0x19e; host_new seeds it from pending_difficulty 0x696564
    int16_t player_count;      // 0x1a0 the value the summary log averages
    network_player_entry players[16]; // 0x1a2
    uint8_t unknown_3a2[10];   // 0x3a2
    uint8_t map_loaded;        // 0x3ac 0x4de6d0 sets 1 after scenario_load/game_start_new_map (0 on key-open failure)
                               //    and returns it; dispatch/shutdown load the UI map when set then clear
    uint8_t pad_3ad[3];        // 0x3ad
} network_game_session;        // size 0x3b0

// ---------------------------------------------------------------------------
// network_machine  (0x4dec40 init, 0x4df690 reset, 0x4e0810 find by id,
// 0x4e0b90 clear flag, 0x4e0ef0 timeout, 0x4e11d0 the per-frame pass)
// One connected peer. The table base and stride are pinned by 0x4e0810, which
// walks machine_id from server+0x3c4 with a stride of 0x60 and returns
// server + 0x3b8 + i*0x60.
// ---------------------------------------------------------------------------
typedef enum network_machine_flags {
    k_network_machine_established = 0x01,
    k_network_machine_pending = 0x02,         // set by 0x4df690
    k_network_machine_version_mismatch = 0x08 // set by 0x4dff20
} network_machine_flags;

typedef struct network_machine {
    network_channel *channel;    // 0x00
    int32_t unknown_04;          // 0x04
    int32_t unknown_08;          // 0x08
    int16_t machine_id;          // 0x0c 0xffff means the slot is free
    uint8_t flags;               // 0x0e see network_machine_flags
    uint8_t unknown_0f;          // 0x0f
    uint8_t disconnect_timer_active; // 0x10 network_machine_timer_start 0x4df090 sets 1 with timer_14 start /
                                     //    timer_18 deadline; 0x4e11d0 services the channel only while 0; reset clears
                                     //    it
    uint8_t pad_11[3];           // 0x11
    int32_t timer_14;            // 0x14 cleared by 0x4df690
    int32_t timer_18;            // 0x18 cleared by 0x4df690
    uint8_t connect_state[0x34]; // 0x1c zeroed as one block by 0x4df690
    uint8_t player_joined;       // 0x50 handle_client_join 0x4dfc90 sets 1 after the player is created; player_delete
                                 //    clears via 0x4e0b90; timeout 0x4e0ef0 frees the slot only when 0
    uint8_t players_removed_broadcast; // 0x51 0x4e0ef0: on a timed-out machine that still has players, broadcasts
                                       //    each player's removal once, then sets 1 and returns early thereafter
    int32_t unknown_52;          // 0x52 unaligned in the original
    int32_t unknown_56;          // 0x56 unaligned in the original
    int16_t unknown_5a;          // 0x5a
    int32_t gcd_user_id;         // 0x5c 0x4e0ef0 gcd_disconnect_user(network_console_connection_id, it) (-1 ->
                                 //    gcd_disconnect_all); sv_ban/autoban pass it to network_banlist_add_ban; -1 at
                                 //    init
} network_machine;               // size 0x60

// ---------------------------------------------------------------------------
// network_server_globals  (0x4dec40 network_game_server_host_new,
// 0x4deda0 network_game_server_host_dispose)
// The host side block. host_new zeroes 0x284 dwords, so the block is 0xa10 bytes.
// ---------------------------------------------------------------------------
typedef struct network_server_globals {
    network_channel *listen_channel; // 0x000 network_channel_new(1)
    int16_t state;             // 0x004 0 pregame (heartbeat_tick runs, it moves to 1 when the game starts;
                               //       host_dispose sends message 0x0b), 1 in game (the per-frame tick drains
                               //       updates; joins are only finalized now), 2 postgame (game_engine_tick
                               //       only; host_dispose sends 0x22)
    uint16_t flags;            // 0x006 bit0 session initialized, bit1 host, bit2 stats logging
    network_game_session session; // 0x008 everything shared but the password lives here
                               //       session+0x3a8 (server+0x3b0) is the int32 host_new
                               //       sets to -1 and then increments to 0
    network_machine machines[16]; // 0x3b8
    int32_t update_tick;       // 0x9b8 +1 per server update drained in state 1; cleared by host_new, the round
                               //       reset, a settings update and the scenario announcement
    uint8_t unknown_9bc[0x3c]; // 0x9bc
    uint8_t join_finalize_pending; // 0x9f8 the per-frame tick finalizes the join of the machine whose id is
                               //       at 0x9f4 and clears it
    uint8_t scenario_announced; // 0x9f9 network_host_send_scenario_announcement sends once per round
    uint8_t new_server_pending; // 0x9fa set by network_game_start_new_server_with_name_and_password, cleared
                               //       when the first game settings update has loaded the scenario
    uint8_t pad_9fb;           // 0x9fb
    uint16_t password[9];      // 0x9fc wcsncpy of 8 wide chars plus a forced NUL at 0xa0c
    uint8_t full_state_broadcast_pending; // 0xa0e set by a settings update, consumed by
                               //       network_host_full_state_broadcast
    uint8_t game_over;         // 0xa0f the end-of-game flag game.h records
} network_server_globals;      // size 0xa10
// global 0x0071c2d4: network_server_globals *network_server   points at 0x00861340
// global 0x00861340: network_server_globals network_server_storage
// global 0x0071c2ec: uint8_t network_server_active

// ---------------------------------------------------------------------------
// network_connection_endpoint  (0x4d8c50 set, 0x4d8cf0 initiate, 0x4d8ed0 connect,
// 0x4d93b0 retransmit, 0x4d9400 keepalive, 0x4daf80 the client tick)
// The client's view of the server it is connected to. It lives inline in
// network_client_globals at +0xab4; every function above reaches it through that
// offset, and 0x4d8cf0 zeroes exactly ten dwords there, which is the size.
// Folded in from the per-file TYPES-GAP copies during the review pass; the field
// widths at +0x1c..+0x21 come from the disassembly, which touches 0xad0, 0xad2 and
// 0xad4 as three separate 16-bit counters (`inc WORD PTR [esi+0xad0]` at 0x4d94a0,
// `inc WORD PTR [esi+0xad2]` at 0x4d93d3, `mov WORD PTR [esi+0xad4],ax` at 0x4d93f8).
// ---------------------------------------------------------------------------
typedef struct network_connection_endpoint {
    s_network_address address;   // 0x00 (client+0xab4) size 4 and the port are written by 0x4d8cf0
    uint32_t unknown_14;         // 0x14 (0xac8) sixth dword of the six-dword blit 0x4d8c50 copies
    int32_t last_send_ms;        // 0x18 (0xacc) last keepalive send, QPC milliseconds
    int16_t message_count;       // 0x1c (0xad0) incremented once per keepalive sent
    int16_t retry_count;         // 0x1e (0xad2) incremented once per overdue retransmit
    int16_t unknown_20;          // 0x20 (0xad4) set to (0x4ed350's result << 1) on retransmit
    uint8_t ready;               // 0x22 (0xad6) 0 while being rebuilt, 1 once populated
    uint8_t unknown_23;          // 0x23 (0xad7)
    void *control_block;         // 0x24 (0xad8) GlobalAlloc of 0x264, first two dwords zeroed
} network_connection_endpoint;   // size 0x28

// ---------------------------------------------------------------------------
// network_connection_attempt_state  (0x4d8cf0 initiate, 0x4d8ed0 connect,
// 0x4d8c10 the progress percentage, 0x4dab80 the retry tick, 0x4db4c0 the status text)
// Inline in network_client_globals at +0xae0, filling the 0x34 bytes between the
// channel pointer and the session block. 0x4d8cf0 writes it field by field:
// dword +0xae0 = 0, dword +0xae4 = the QPC millisecond clock, dword +0xae8 = 0,
// byte +0xaec = 0, then `rep movsd` of nine dwords into +0xaee -- which is why
// session_info is unaligned and why this struct only works under pack(1).
// ---------------------------------------------------------------------------
typedef struct network_connection_attempt_state {
    uint32_t unknown_00;         // 0x00 (0xae0) cleared at the start of every attempt
    int32_t started_ms;          // 0x04 (0xae4) QPC milliseconds; the progress bar's time base
    int32_t elapsed_counter;     // 0x08 (0xae8) driven by the join status text animation
    uint8_t unknown_0c;          // 0x0c (0xaec) cleared at the start of every attempt
    uint8_t pad_0d;              // 0x0d (0xaed)
    uint32_t session_info[9];    // 0x0e (0xaee) nine dwords copied from the caller, unaligned
    uint8_t unknown_32[2];       // 0x32 (0xb12) not written by any function in this module
} network_connection_attempt_state; // size 0x34, spans 0xae0..0xb13

// ---------------------------------------------------------------------------
// network_client_timer_record  (0x4dbf50 schedule, 0x4db3a0 the identity tick)
// The one-shot timer network_client_timer_schedule arms and the identity tick
// fires; it overlays the first five dwords of the zeroed run at
// network_client_globals+0xee4.
// ---------------------------------------------------------------------------
typedef struct network_client_timer_record {
    uint8_t active;              // 0x00 (0xee4) 1 while a deadline is armed
    uint8_t pad_01[3];           // 0x01
    int32_t deadline_ms;         // 0x04 (0xee8) QPC milliseconds
    uint8_t triggered;           // 0x08 (0xeec) set when the deadline passes
    uint8_t pad_09[3];           // 0x09
    int32_t context;             // 0x0c (0xef0) the value the scheduler was handed in EDX
    int32_t retrigger_ms;        // 0x10 (0xef4) deadline_ms + context, the follow-up deadline
} network_client_timer_record;   // size 0x14

// ---------------------------------------------------------------------------
// network_client_state  -- the values network_client_globals::state takes.
// network_client_state_dispatch (0x4d8bb0) switches on it to pick the per-tick
// handler, and every decode handler in 0x4dbc00..0x4dc4b0 gates on it. The labels
// are inferred from which handler each value selects plus which messages each
// state accepts; the numbers themselves are certain, the names are not.
// ---------------------------------------------------------------------------
typedef enum network_client_state {
    k_network_client_state_idle = 0,        // network_join_handshake_tick        (0x4daa20)
    k_network_client_state_connecting = 1,  // network_join_connect_retry_tick    (0x4dab80)
    k_network_client_state_joining = 2,     // network_host_lobby_tick            (0x4daef0)
    k_network_client_state_playing = 3,     // network_game_client_update         (0x4daf80)
    k_network_client_state_established = 4  // network_host_channel_service_tick  (0x4db100)
} network_client_state;

// ---------------------------------------------------------------------------
// network_client_globals  (0x4d8a80 network_session_create, 0x4d8b70 destroy)
// The client and shared-game side block. create writes the storage at 0x00872de0
// directly, so every offset below is that global minus 0x00872de0.
// ---------------------------------------------------------------------------
typedef struct network_client_globals {
    uint16_t machine_index;    // 0x000 0x4d94c0 stores join-accept +0xc; network_client_rejoin_check and ui
                               //    0x49dca0/0x4a5740 compare *(int16*)network_client to player.machine_index; 0xffff
                               //    at create
    uint8_t unknown_002[0xab2];// 0x002
    network_connection_endpoint connection; // 0xab4 the server this client is talking to
    network_channel *channel;  // 0xadc network_channel_new(2), deleted by destroy
    network_connection_attempt_state connect_attempt; // 0xae0
    network_game_session session; // 0xb14 the same block the server embeds at +0x008
    int32_t unknown_ec4;       // 0xec4
    int32_t unknown_ec8;       // 0xec8
    int32_t last_update_id;    // 0xecc network_game_state_update_receive 0x4d9d20: record id <= it -> network error;
                               //    stored after apply; update_server_send_update reads &0x7fffffff as ack
    int32_t last_update_received_ms; // 0xed0 0x4d9d20 stores QPC ms after applying a game state update; zeroed at
                                     //    create
    int32_t last_presence_broadcast_ms; // 0xed4 network_host_presence_broadcast_tick: sends when +1000 < now and
                                        //    restamps
    uint16_t game_start_countdown_seconds; // 0xed8 0x4dbf30 decodes a class-2 16-bit value into it; ui 0x4a5740
                                           //    formats it as 0:%02d / %02d:%02d countdown, 0 = starting; 0xffff at
                                           //    create
    uint16_t state;            // 0xeda see network_client_state; NOT padding, see 0x4d8bb0
    int16_t disconnect_reason; // 0xedc 0 at create; 0x4d9ce0 / 0x4dc4b0 (server notification) default it to 8;
                               //    main_loop maps 8 to join error 4 else 6; dispatch only continues while 0
    uint16_t flags;            // 0xede bit 0x2 set once the join/settings packet (0x4d94c0) was sent and gates
                               //    resending; bit 0x4 gates the 3 s join-status text in
                               //    network_join_connect_retry_tick 0x4dab80; create clears 0x6
    uint8_t network_error_displayed; // 0xee0 network_disconnect_notify_dropped_machines 0x4d9340 shows
                                     //    display_error(8) only while 0 then sets 1; called on every decode/sequence
                                     //    failure
    uint8_t connection_stalled; // 0xee1 network_game_client_update: channel flags bit 5, also starts
                                //    ui_network_wait_timeout; zeroed at create/finalize_join
    uint16_t pad_ee2;          // 0xee2
    network_client_timer_record timer; // 0xee4 the first five dwords of the zeroed run
    network_resolved_address server_address; // 0xef8 filled by 0x4dd390 from
                               //       client->channel; 0x4d9f23 is `lea ecx,[esi+0xef8]`
    int32_t team_index;        // 0xf10 identity_tick keeps player->team (+0x20) across reconnect;
                               //    game_settings_updated copies player+0x20; 0x4d94c0 sends it as join frame byte
                               //    0x8c; -1 at create
    int32_t unknown_f14[13];   // 0xf14 zeroed as one run at create
    void *update_history;      // 0xf48 player_update_history *, GlobalAlloc of 0x2c
} network_client_globals;      // size 0xf4c
// global 0x0071c2d8: network_client_globals *network_client   points at 0x00872de0
// global 0x00872de0: network_client_globals network_client_storage
// global 0x0071c2c2: uint8_t network_session_active
// global 0x0071c2c1: uint8_t copied into network_game_session::unknown_3ac
// global 0x0071c2c8: uint8_t bypasses the channel service back-off in 0x4dd110
// global 0x0071c2dc: uint8_t shortens the disconnect timeout when clear (0x4ddd20)
// global 0x0071c2de: uint8_t host-handoff request flag (0x4de390, 0x4dded0)

// ---------------------------------------------------------------------------
// network_game_search_entry  (0x4da7d0 add-or-update, 0x4da770 freshness test,
// 0x4db9a0 beacon decode)
// The LAN browse list. Nine fixed slots; an entry older than six seconds is wiped
// before the incoming announcement is matched against the list.
// ---------------------------------------------------------------------------
typedef struct network_game_search_entry {
    uint32_t identity[6];      // 0x000 copied verbatim from the announcement header
    int32_t received_ms;       // 0x018 QPC milliseconds, the freshness stamp
    uint16_t name[64];         // 0x01c UTF-16, wcsncpy of 0x3f then a forced NUL
    uint32_t info[33];         // 0x09c 0x21 dwords lifted from announcement+0xd0
    int16_t game_engine_index; // 0x120
    int16_t player_count;      // 0x122 from announcement+0x156
    int16_t unknown_124;       // 0x124
    int16_t unknown_126;       // 0x126 from announcement+0x15a
    int16_t unknown_128;       // 0x128
    int16_t unknown_12a;       // 0x12a
    uint8_t joinable;          // 0x12c needs announcement flag bit1 and under 16 players
    uint8_t in_use;            // 0x12d
    uint8_t stats_logging;     // 0x12e announcement flag bit2
    uint8_t unknown_12f;       // 0x12f set when engine 3 and announcement flag bit3
} network_game_search_entry;   // size 0x130

// ---------------------------------------------------------------------------
// player update history  (0x4e6b50 add, 0x4e6f20 free all, 0x4e6f60 find and
// prune, 0x4e6ff0 play, 0x4e6b10 destroy)
// The client keeps a singly linked list of snapshots of the locally controlled
// unit so it can replay prediction on top of a server acknowledgement. The list
// is capped at 64 nodes; past that the add refuses and reports -1.
// The node body is a straight copy of unit and vehicle object fields, so it is
// left opaque; only the fields this module itself reads are named.
// ---------------------------------------------------------------------------
typedef struct player_update_history_node {
    int32_t update_id;            // 0x000 wraps modulo 0x40
    int32_t tick_count;           // 0x004 summed when the log reports "== %d ticks"
    uint32_t control[8];          // 0x008 the staged control record for this update
    uint8_t has_vehicle;          // 0x028 set when the unit is in a seat
    uint8_t pad_029[3];           // 0x029
    datum_index vehicle_object;   // 0x02c copied from unit+0x11c
    uint8_t unit_state[0x0d0];    // 0x030 unit fields 0x5c..0x520
    uint8_t vehicle_state[0x314]; // 0x100 vehicle object fields, valid only with has_vehicle
    struct player_update_history_node *next; // 0x414
} player_update_history_node;     // size 0x418

typedef struct player_update_history {
    int32_t next_update_id;           // 0x00 incremented modulo 0x40 per add
    player_update_history_node *head; // 0x04
    player_update_history_node *tail; // 0x08
    int32_t statistics[8];            // 0x0c player_update_history_play 0x4e6ff0: [0] calls,[1] total updates,[2]
                                      //    total ticks,[3]/[4] last updates/ticks,[5] total distance f,[6] avg
                                      //    distance f,[7] avg ticks f
} player_update_history;              // size 0x2c
// Ordering windows: a local player acknowledgement is accepted while it is within
// 15 of the current id (0x4e69b0), a remote player update while it is within 3 of
// the previous one (0x4e6a20, whose previous id lives at player+0x15c).

// ---------------------------------------------------------------------------
// ban list  (0x4e3160 load, 0x4e3380 save, 0x4e3890 get or add, 0x4e37d0 find,
// 0x4e35c0 add ban, 0x4e3820 reject)
// banned<suffix>.txt is CSV: name, cd key hash, ban count, expiry date or "--".
// The entries live in a growable_array whose element size is 0x38.
// ---------------------------------------------------------------------------
typedef struct ban_list_entry {
    char name[13];             // 0x00 strncpy of 12 then a forced NUL at 0x0c
    char cd_key_hash[33];      // 0x0d strncpy of 32 then a forced NUL at 0x2d
    int16_t ban_count;         // 0x2e the offence tier, indexes the penalty table
    uint8_t indefinite;        // 0x30 1 when there is no expiry
    uint8_t pad_31[3];         // 0x31
    int32_t expiry_time;       // 0x34 time_t, 0 when indefinite
} ban_list_entry;              // size 0x38
// global 0x006b85a0: growable_array ban_list   element_size 0x38, data pointer at 0x006b85a4
// global 0x00699574: int32_t sv_ban_penalty_seconds[4]   -1 means ban indefinitely

// ---------------------------------------------------------------------------
// installed map list  (0x4e4290 map_list_matching_substring)
// The table of maps found on disk, scanned linearly by the sv_map console command
// and by the map-name completion helper. Folded here out of
// src/networking/map_list_matching_substring.c by the review pass.
// UNSURE: everything except name and valid; only those two are read.
// ---------------------------------------------------------------------------
// The element type is types/interface.h map_list_entry (0x0c: +0x00 path, NULL when the slot
// is unused, +0x04 map_id, +0x08 cache_file_exists): the old network_map_list_entry here was a
// second, less complete layout of the same table and has been dropped (interface.h sorts
// before networking.h, so Ghidra already has the type).
// (0x00712dcc is interface.h map_list_entry *map_list; this module only scans it)

// ---------------------------------------------------------------------------
// network_buffer_pair  (0x4e3ed0 network_buffer_pair_pool_clear)
// The pool element the autopatch / map-download path allocates in pairs through
// GlobalAlloc; the clear path frees both halves of every element and then the
// array itself. Folded here out of src/networking/network_buffer_pair_pool_clear.c
// by the review pass.
// ---------------------------------------------------------------------------
typedef struct network_buffer_pair {
    void *first;               // 0x00
    void *second;              // 0x04
} network_buffer_pair;         // size 0x08
// global 0x006b85a8: int32_t network_buffer_pair_pool_unknown_element_flag
// global 0x006b85ac: int32_t network_buffer_pair_pool_count
// global 0x006b85b0: network_buffer_pair *network_buffer_pair_pool_data

// ---------------------------------------------------------------------------
// message delta protocol  (0x4ec2f0 initialize, 0x4ec790 layout size, 0x4ec940
// encode, 0x4ece70 decode header, 0x4ebe00 parameter register)
// Each network message type has a definition record that caches the bit sizes the
// encoder and decoder need. 0x4ec790 fills +0x04, +0x08, +0x0c, +0x10, +0x18 and
// +0x24 plus the static field list size; a field binding is 4 dwords whose first
// dword is a field type descriptor carrying its own bit size at +0x5c.
// ---------------------------------------------------------------------------
// The per-field-type record a binding points at. Its layout is the union of every offset the
// module touches: message_delta_field_bindings_invoke (0x4ec700) and _lazy_init (0x4ec840)
// dispatch on the kind at +0x00 through message_delta_field_type_table, _lazy_init caches the
// bit size at +0x5c and the once-only flag at +0x64, and the four array codecs
// (0x4e9330, 0x4e95e0, 0x4e9db0, 0x4ea040) call the instance function pointers at +0x50 and
// +0x54 and read the array descriptor at +0x58 and the reserved-flag-bit count at +0x60.
// UNSURE: total size. 0x68 is a floor; nothing in this module allocates one.
typedef struct message_delta_field_type {
    int32_t kind;              // 0x00 index into message_delta_field_type_table, stride 0x18
    char name[0x4c];           // 0x04 the field type's own name, NUL terminated ("point3d",
                               //      "game_variant", "translational_velocity", ...).
                               //      UNSURE: the 0x4c bound is the gap to +0x50, not a proven
                               //      buffer size; every name seen is far shorter.
    int32_t (*encode)(void *field_type, void *previous, void *destination, void *stream); // 0x50
    int32_t (*decode)(void *field_type, void *previous, void *destination, void *stream); // 0x54
    void *array_descriptor;    // 0x58 message_delta_array_descriptor or
                               //      message_delta_array_field_list, per codec
    int32_t size_bits;         // 0x5c cached by message_delta_field_bindings_lazy_init
    int32_t reserved_bits;     // 0x60 flag bits the array codecs reserve before the elements
    uint8_t initialized;       // 0x64 set once by message_delta_field_bindings_lazy_init
    uint8_t pad_65[3];         // 0x65
} message_delta_field_type;    // at least 0x68

// The per-kind callback table the two dispatchers index. The base is 0x0069a2f0: the init slot
// is called as `[kind * 0x18 + 0x69a2fc]` (0x4ec700, 0x4ec840), the size slot as
// `[kind * 0x18 + 0x69a2f8]` (0x4ec840) and the teardown slot as `[kind * 0x18 + 0x69a300]`
// (0x4ec900). That also resolves what used to be recorded as a separate unnamed table of 0x18
// records at 0x0069a304: those are this table's `registered` bytes, one per record.
typedef struct message_delta_field_type_vtable {
    uint8_t unknown_00[8];                                   // 0x00
    int32_t (*compute_size)(message_delta_field_type *type); // 0x08 0x0069a2f8
    void (*initialize)(message_delta_field_type *type);      // 0x0c 0x0069a2fc
    void (*teardown)(message_delta_field_type *type);        // 0x10 0x0069a300
    uint8_t registered;                                      // 0x14 0x0069a304, set by 0x4ec2f0
    uint8_t pad_15[3];                                       // 0x15
} message_delta_field_type_vtable; // size 0x18
// global 0x0069a2f0: message_delta_field_type_vtable message_delta_field_type_table[28]
//
// The field-type instances themselves live in .data and are not built by any code in this
// module, so the only way to see them is to read the image. Scanning for records whose +0x50 and
// +0x54 are both addresses inside 0x4e9000..0x4ed400 recovers the whole registry, with Bungie's
// own names inline at +0x04. That is what pins name, encode, decode, array_descriptor,
// size_bits (-1 until lazy_init caches it), reserved_bits and initialized all at once:
//
//   kind  encode    decode    field types
//    8    0x4e9130  0x4e9330  ctf_score_array, king_score_array, oddball_score_array,
//                             oddball_owner_array, race_score_array, slayer_score_array,
//                             network_game_players, object_change_colors, game_engine_variant,
//                             parameters_protocol_array      (array of structures)
//    9    0x4e95e0  0x4e97e0  game_variant, universal_variant, network_map, network_player
//                             (compound record: {count, field bindings})
//   10    0x4e9a30  0x4e9a60  hud_chat_message_ptr
//   13    0x4e9bc0  0x4e9bf0  object_index, player_index
//   14    0x4e9db0  0x4ea040  point2d, point3d                 ({count} scalars, 32 bits each)
//   15    0x4ea240  0x4ea250  vector2d, vector3d
//   17    0x4ea2b0  0x4ea3b0  control_flags, damage_data_flags, game_variant_flags,
//                             universal_variant_flags
//   18    0x4ea430  0x4ea460  time
//   19    0x4ea610  0x4ea660  grenade_counts
//   20    0x4ea500  0x4ea5b0  fixed_width_1bit, fixed_width_3bits, fixed_width_6bits
//   21    0x4ea8b0  0x4eaa70  fixed_width_normal_4bit, _8bit, _16bit
//   22    0x4eabe0  0x4eaed0  locality_reference_position
//   23    0x4eb160  0x4eb1d0  digital_throttle
//   24    0x4eb220  0x4eb280  fixed_width_weapon_index
//   25    0x4eb2d0  0x4eb330  fixed_width_grenade_index
//   26    0x4eb680  0x4eb890  angular_velocity, translational_velocity
//   27    0x4ebab0  0x4ebc20  item_placement_position
//
// Kinds 0..7, 11, 12 and 16 have no instance in .data with both pointers in that range; they are
// presumably the primitive widths the field bindings encode inline.
// This also explains why tools/pack.py reports callers=0 for every one of those codecs: they are
// only ever reached through these records, never by a direct call.

// One field of a message: which field type codes it, and where the value sits inside the
// message body. The two offsets are named from five independent call sites that add them to a
// destination or a source pointer (0x4ec6a0, 0x4ec5d0, 0x4ed1d0, 0x4e95e0, 0x4ecc60); the flag
// byte at +0x0c is what message_delta_field_bindings_lazy_init sets and _teardown clears, and
// the all-zero sentinel both dispatchers stop on tests only +0x00, +0x04 and +0x08.
typedef struct message_delta_field_binding {
    message_delta_field_type *field_type; // 0x00 size in bits at field_type+0x5c
    int32_t destination_offset;           // 0x04 byte offset of the field in the decoded body
    int32_t source_offset;                // 0x08 byte offset of the field in the previous body
    uint8_t initialized;                  // 0x0c per-binding "its field type is live" flag
    uint8_t pad_0d[3];                    // 0x0d
} message_delta_field_binding;            // size 0x10

typedef struct message_delta_static_fields {
    int32_t count;             // 0x00
    int32_t size_bits;         // 0x04 written by message_delta_field_layout_compute_size
    message_delta_field_binding fields[1]; // 0x08 count entries
} message_delta_static_fields; // size 0x08 plus count * 0x10

typedef struct message_delta_definition {
    int32_t type_index;             // 0x00 index back into the definition pointer table
    int32_t header_and_static_bits; // 0x04
    int32_t item_bits;              // 0x08 header_and_static_bits plus the per-item fields
    int32_t header_bits;            // 0x0c 7, or 10 when the parameters protocol is on
    int32_t maximum_bits;           // 0x10 header_bits + maximum_items * item_bits
    int32_t maximum_items;          // 0x14
    uint8_t initialized;            // 0x18
    uint8_t pad_19[3];              // 0x19
    message_delta_static_fields *statics; // 0x1c
    int32_t field_count;            // 0x20
    int32_t field_bits;             // 0x24
    message_delta_field_binding fields[1]; // 0x28 field_count entries
} message_delta_definition;         // size 0x28 plus field_count * 0x10
// global 0x0065d440: message_delta_definition *message_delta_definitions[56]
// global 0x0065d51f: uint8_t message_delta_item_count_bits[]  extra header bits per item count
// global 0x0071cfa8: uint8_t message_delta_parameters_enabled

typedef struct message_delta_parameter {
    char *name;                // 0x00 GlobalAlloc backed, "<scope>::<name>" when scoped
    int32_t type;              // 0x04 1 int, otherwise float
    void *value;               // 0x08 the live variable
} message_delta_parameter;     // size 0x0c
// global 0x006b86c0: message_delta_parameter message_delta_parameters[]
// global 0x0071cfb0: int32_t message_delta_parameter_count
// parameters.cfg is read whole into a shared text buffer by 0x4ebda0 and written
// back out by 0x4ec330.

// ---------------------------------------------------------------------------
// the four generic array codecs  (0x4e9330 decode, 0x4e95e0 encode, 0x4e9db0 float
// encode, 0x4ea040 dword decode)
// All four read message_delta_field_type::array_descriptor, and all four write the
// per-element "changed" bits into a block of field_type->reserved_bits reserved at the
// head of the array and the element payloads after it, seeking back and forth between
// the two regions. Every seek in them is absolute and measured from bit_stream::first_bit,
// never from the current cursor.
// The array-of-structures decoder and encoder disagree about the descriptor's shape: the
// decoder reads {count, element_size, field_type} and strides the elements itself, the
// encoder reads {count, fields[]} and takes each element's offsets from its own record.
// Both readings are literal; nothing in the image shows the two descriptors being the same
// object, so they are declared separately.
// ---------------------------------------------------------------------------
typedef struct message_delta_array_descriptor {
    int32_t count;                        // 0x00
    int32_t element_size;                 // 0x04 stride in bytes
    message_delta_field_type *field_type; // 0x08 decodes one element
} message_delta_array_descriptor;         // size 0x0c

typedef struct message_delta_array_field_list {
    int32_t count;                        // 0x00
    message_delta_field_binding fields[1]; // 0x04 count entries; note the fields start at +0x04
                                           //      here, not at +0x08 as in
                                           //      message_delta_static_fields
} message_delta_array_field_list;          // size 0x04 plus count * 0x10

// What the float and dword array codecs read: only the element count. Their stride is the
// fixed 4 bytes / 32 bits both of them hard-code.
typedef struct message_delta_scalar_array_descriptor {
    int32_t count;             // 0x00
} message_delta_scalar_array_descriptor; // size 0x04

// ---------------------------------------------------------------------------
// message_delta_sample_ring_buffer  (0x4ed350 average, 0x4ed390 append,
// 0x4ed310 record and append)
// A fixed 30-entry ring of 5-dword samples with a cached mean. Nothing names the fields of
// one entry; only the ring around them is pinned, by the 0x1e cap and the 0x14 stride.
// ---------------------------------------------------------------------------
typedef struct message_delta_sample_ring_buffer {
    int32_t cached_average;    // 0x00 recomputed by the append path
    int32_t count;             // 0x04 capped at 30
    int32_t write_cursor;      // 0x08 used once the ring is full
    int32_t entries[30][5];    // 0x0c 0x14 bytes per entry
} message_delta_sample_ring_buffer; // size 0x264

// ---------------------------------------------------------------------------
// the vector quantization tables  (0x4eb370 lerp, 0x4eb4a0 quantize,
// 0x4eb560 waypoint table init, 0x4eb890 indexed decode)
// Two small descriptor blocks the message-delta vector codecs read. Neither is allocated in
// this module; both arrive as a pointer argument, so only the offsets that are read are known.
// ---------------------------------------------------------------------------
typedef struct vector3d_lerp_table {
    real minimum;              // 0x00
    real maximum;              // 0x04
    int32_t unknown_08;        // 0x08
    int32_t denominator_mode1; // 0x0c
    int32_t unknown_10;        // 0x10
    int32_t denominator_mode0; // 0x14
} vector3d_lerp_table;         // at least 0x18

typedef struct waypoint_table {
    real range_min;            // 0x00
    real range_max;            // 0x04
    int32_t bits_a;            // 0x08 low byte used as a shift amount
    real max_level_a;          // 0x0c (1 << bits_a) - 1 when left zero
    int32_t bits_b;            // 0x10
    real max_level_b;          // 0x14 (1 << bits_b) - 1 when left zero
    real count_as_float;       // 0x18 read through an int conversion, not a bit reinterpretation
    uint8_t unknown_1c[8];     // 0x1c
    real_point3d points[1];    // 0x24 count_as_float entries, NaN terminated
} waypoint_table;              // size 0x24 plus count * 0x0c

// ---------------------------------------------------------------------------
// network_index_cache  (0x4e9c20 find or allocate, 0x4e9cd0 insert if free,
// 0x4e9d20 get, 0x4e9d40 remove)
// The object-to-network-index cache hanging off an object container at container+0x58: a
// bounded slot array plus a hash table mapping a key to its slot, with a rotating eviction
// cursor. The embedded table is types/objects.h's hash_table, kept here as a byte block
// because types/*.h carry no includes and every other consumer of this header would then
// need objects.h parsed first; the four network_index_cache_*.c files cast it.
// ---------------------------------------------------------------------------
typedef struct network_index_cache {
    int32_t capacity;          // 0x00 entries in slots
    uint8_t unknown_04[8];     // 0x04
    uint8_t table[0x18];       // 0x0c a types/objects.h hash_table, key -> slot index
    int32_t cursor;            // 0x24 rotating allocation/eviction cursor
    int32_t *slots;            // 0x28 capacity entries, -1 when free
} network_index_cache;         // size 0x2c

// ---------------------------------------------------------------------------
// message_delta_decode_state  (0x20 bytes)
// The per-message decode record the message-delta protocol hands every client-side
// update handler. message_delta_read_changed_subfields (0x4ed1d0) takes it in EDI
// and is the anchor for the layout: it reads message_type at +0x04 to index
// message_delta_definitions, walks the bit cursor object at +0x10, and accumulates
// the bits it consumed into +0x0c. FUN_004ec590 (0x4ec590) reads *this* record out
// of decode_context slot 0 and passes it straight through in EDI, which is what ties
// the two together. The remaining three fields are named from
// network_client_drain_queued_updates (0x4e1f40), the only function that reads them;
// it was the sole owner of this layout before (as a file-local
// network_queued_update_record) and now shares this declaration.
// ---------------------------------------------------------------------------
typedef struct message_delta_decode_state {
    int32_t incremental;       // 0x00 0 == stateless (baseline), non-zero == incremental
    int32_t message_type;      // 0x04 index into message_delta_definitions[56]
    int32_t item_count;        // 0x08 total items in this message
    int32_t bits_read;         // 0x0c accumulates what message_delta_read_changed_subfields returns
    void *stream;              // 0x10 bit cursor; 0x4ed1d0 reads +0x08/+0x0c/+0x10/+0x14
    int32_t unknown_14;        // 0x14
    int32_t processed_count;   // 0x18 items the drain loop has already dispatched
    uint8_t more_items;        // 0x1c the drain loop stops when this clears
    uint8_t changed;           // 0x1d every delta handler stores 1 here after decoding
    uint8_t pad_1e[2];         // 0x1e
} message_delta_decode_state;  // size 0x20
// The decode context itself (EAX or EDX in the 0x4e5390..0x4e6510 handlers) is an
// array of pointers: slot 0 is this record, slot 1 onward is the field-binding list
// handed to message_delta_read_changed_subfields as (context + 1), and slot 0x11 is
// a remote_player_update_header *. It is left untyped because only those three slots
// are ever touched and its size is not pinned by anything in the image.

// ---------------------------------------------------------------------------
// remote_player_update_header
// What decode_context slot 0x11 points at: the on-the-wire player identity plus the
// two sequence bytes every remote-player handler in 0x4e5620..0x4e5d60 reads.
// player_index arrives as a raw wire index and is remapped in place through
// remote_player_index_remap_table (0x00687558, table pointer at +0x28) by the four
// handlers that own the message; the two stand-alone "position delta" handlers
// (0x4e5c40, 0x4e5d60) deliberately do not write the remap back.
// UNSURE: total size. Only +0x00, +0x04, +0x05 and +0x08 are ever read, and no
// allocation of this record is visible in this module, so 0x0c is a floor, not a fact.
// ---------------------------------------------------------------------------
typedef struct remote_player_update_header {
    int32_t player_index;      // 0x00 wire index in, player datum_index out
    uint8_t update_id;         // 0x04 wraps modulo 0x40; the "[%d]" in every log line
    uint8_t baseline_id;       // 0x05 which baseline a delta is relative to
    uint8_t pad_06[2];         // 0x06
    uint8_t control_sequence;  // 0x08 the second argument the two "total" handlers pass
                               //      on; the stand-alone handlers pass baseline_id there
    uint8_t pad_09[3];         // 0x09
} remote_player_update_header; // size 0x0c, UNSURE

// ---------------------------------------------------------------------------
// remote_player_action_state  (0x30 bytes)
// The 12-dword control record that lives at player+0xf0 (types/game.h
// player::unknown_f0 .. unknown_11c) and is staged on the stack by every remote
// player handler. handle_remote_player_action_update (0x4e60c0) is what names the
// fields: it computes yaw/pitch from direction with two FPATANs and copies
// unknown_04..+0x23 (eight dwords) into the action queue record.
// UNSURE: every field except direction, yaw and pitch.
// ---------------------------------------------------------------------------
typedef struct remote_player_action_state {
    uint32_t flags;            // 0x00 only the low byte is ever read
    int32_t unknown_04;        // 0x04 first of the eight dwords pushed on the queue
    real yaw;                  // 0x08 atan2(direction.y, direction.x)
    real pitch;                // 0x0c atan2(direction.z, sqrt(x*x + y*y))
    int32_t unknown_10[4];     // 0x10
    uint16_t unknown_20;       // 0x20 0x4e60c0 stores a leftover register here
    uint16_t unknown_22;       // 0x22
    real_vector3d direction;   // 0x24 aim direction yaw/pitch are derived from
} remote_player_action_state;  // size 0x30

// The two combined ("total") update payloads. Both are a remote_player_action_state
// followed by the positional half, staged as one block so that one
// message_delta_read_changed_subfields call covers the whole thing:
// player_update_client_remote_player_total_biped_update_from_network (0x4e5870)
// seeds it from player+0xf0 plus player+0x164, and
// player_update_client_remote_player_total_vehicle_update_from_network (0x4e5a30)
// from player+0xf0 plus player+0x190. The 0xf rep-stosd / 0x1c rep-stosd counts in
// those two functions are what fix the sizes at 0x3c and 0x70.
typedef struct remote_player_biped_update_state {
    remote_player_action_state action; // 0x00 -> player+0x0f0
    real_point3d position;             // 0x30 -> player+0x164
} remote_player_biped_update_state;    // size 0x3c

typedef struct remote_player_vehicle_update_state {
    remote_player_action_state action; // 0x00 -> player+0x0f0
    vehicle_update_body vehicle;       // 0x30 -> player+0x190, see types/game.h
} remote_player_vehicle_update_state;  // size 0x70

// ---------------------------------------------------------------------------
// local player update acknowledgements  (0x4e5390 on foot, 0x4e5490 in a vehicle)
// The ECX destination FUN_004ec590 decodes into for the two local-player ack
// handlers. Both were modelled as a handful of "uninitialized" scalar locals until
// the review pass disassembled the two functions: the decoder writes the whole
// block through the ECX pointer the call sites hand it, and the three dwords the
// handlers then latch into player+0x0f0..0x0f8 are its position, not (as an earlier
// rewrite had it) fields of the data_iterator that happens to live next to it on
// the stack.
// Offsets: 0x4e5390 reads the block at [esp+0x08] and the two ids at +0x00/+0x01,
// the three dwords at +0x04/+0x08/+0x0c. 0x4e5490 reads its block at [esp+0x10],
// the same two ids, the remapped vehicle datum at +0x04 and the position at +0x08.
// UNSURE: the two ids are the only named bytes; nothing reads +0x02/+0x03.
// ---------------------------------------------------------------------------
typedef struct local_player_update_ack {
    uint8_t update_id;         // 0x00 compared by is_local_player_update_in_order
    uint8_t baseline_id;       // 0x01 the "[%d]" of the ack log line
    uint8_t pad_02[2];         // 0x02
    real_point3d position;     // 0x04 latched into player+0x0f0..0x0f8
} local_player_update_ack;     // size 0x10

typedef struct local_player_vehicle_update_ack {
    uint8_t update_id;         // 0x00
    uint8_t baseline_id;       // 0x01
    uint8_t pad_02[2];         // 0x02
    vehicle_update_body vehicle; // 0x04 see types/game.h; its position at +0x08 is
                                 //      what lands in player+0x0f0..0x0f8
} local_player_vehicle_update_ack; // size 0x44

// ---------------------------------------------------------------------------
// network bandwidth debug graph  (0x4d7980 reset, 0x4d79d0 and 0x4d7a50
// accumulate, 0x4d7de0 instance init, 0x4d8080 clear, 0x4d8140 find peak,
// 0x4d81c0 rebuild, 0x4d8430 sample, 0x4d84d0 catch up, 0x4d8540 rate, 0x4d8620 draw)
// The per-field offsets of the singleton at 0x00719ce0 line up one for one with
// the instance 0x4d7de0 fills, which is what pins the layout.
// ---------------------------------------------------------------------------
typedef struct network_graph_vertex {
    float x;                   // 0x00 spans left..right across 320 columns
    float y;                   // 0x04 the baseline until a sample raises it
    float z;                   // 0x08
    uint32_t color;            // 0x0c initialized to 0xffffffff
    float u;                   // 0x10
    float v;                   // 0x14
} network_graph_vertex;        // size 0x18

typedef struct network_bandwidth_graph {
    uint8_t needs_layout;        // 0x0000 forces the next update to recompute the layout
    uint8_t pad_0001[3];         // 0x0001
    int32_t last_sample_ms;      // 0x0004
    uint32_t sample_interval_ms; // 0x0008 seeded from 0x006894b0
    int32_t units_index;         // 0x000c 0 bytes, 1 packets (0x4d8a20)
    int32_t direction_index;     // 0x0010 0 sent, 1 received (0x4d8a50)
    uint8_t unknown_0014[0x12];  // 0x0014
    int16_t left;                // 0x0026 screen bounds, recomputed on a resize
    int16_t baseline;            // 0x0028
    int16_t right;               // 0x002a
    uint8_t unknown_002c[0x90];  // 0x002c label text and layout scratch
    int32_t bits_sent;           // 0x00bc accumulated by 0x4d79d0
    int32_t bits_received;       // 0x00c0 accumulated by 0x4d7a50
    int32_t rate_base_ms;        // 0x00c4
    float rate_sent;             // 0x00c8 bits per second
    float rate_received;         // 0x00cc bits per second
    int32_t pending_sample;      // 0x00d0 folded into history on the next interval
    int32_t unknown_00d4;        // 0x00d4
    int32_t history[320];        // 0x00d8
    network_graph_vertex columns[320]; // 0x05d8
    int32_t peak_scale;          // 0x23d8 initialized to 1
    float displayed_rate;        // 0x23dc smoothed from the last five history entries
} network_bandwidth_graph;       // size 0x23e0
// global 0x00719ce0: network_bandwidth_graph network_bandwidth_graph_globals
// global 0x006894b0: uint32_t network_bandwidth_graph_default_interval_ms

// ---------------------------------------------------------------------------
// autopatch  (0x576ad0 completion, 0x576bc0 tick, 0x576c30 init, 0x576db0
// shutdown, 0x576e60 start, 0x576f00 result, 0x576f40 proxy, 0x577310 launch)
// Two asynchronous download slots feeding the bungie.net version check.
// ---------------------------------------------------------------------------
typedef enum autopatch_download_state {
    k_autopatch_download_idle = 0,
    k_autopatch_download_active = 1,
    k_autopatch_download_ready = 4,
    k_autopatch_download_error = 5
} autopatch_download_state;

typedef struct autopatch_download_slot {
    int32_t request_id;        // 0x00 -1 when the slot is free
    int32_t state;             // 0x04 see autopatch_download_state
    void *data;                // 0x08 GlobalAlloc of size bytes, NUL terminated
    int32_t size;              // 0x0c payload length plus the terminator
    uint8_t local_file;        // 0x10 1 when the source was a file, not a URL
    uint8_t pad_11[3];         // 0x11
} autopatch_download_slot;     // size 0x14
// global 0x006ef93c: autopatch_download_slot autopatch_download_slots[2]
// global 0x007227c0: network_mutex_record *autopatch_download_mutex
// global 0x007227c4: network_thread_record *autopatch_download_thread
// global 0x007227bc: uint8_t autopatch_download_pool_stop
// global 0x007227c8: int32_t autopatch_download_active_count
// The build string 0x577310 and 0x578190 hand out is 01.00.10.0621.

// ---------------------------------------------------------------------------
// server browser  (0x4b7080 filter, 0x4ba760 lock, 0x4ba820 count,
// 0x4ba970 comparator select, 0x4baa60 total players, 0x4baae0 ingest)
// The queried server records themselves are GameSpy SDK objects owned by the
// 0x00614000..0x00618000 library code, not by this module: every field access
// goes through the key/value accessors at 0x00617490, 0x006174d0, 0x00617aa0 and
// 0x00617c10 with string keys such as numplayers, maxplayers, password,
// dedicated, gametype, mapname, teamplay and gamever, so no layout for them is
// recoverable here and none is declared.
// ---------------------------------------------------------------------------
// The list itself is a GlobalAlloc-backed pointer array grown 0x20 entries at a time by
// 0x4ba8a0, linear-searched by 0x4ba870, compacted by 0x4ba940 and sorted by 0x4ba9c0 (qsort).
// Those four helpers were named dynamic_pointer_array_* before the array was pinned to this
// global; they all operate on this struct and nothing else in the image allocates one.
// The last two fields were folded in from the per-file TYPES-GAP typedefs in
// src/networking/dynamic_pointer_array_*.c during the review pass.
typedef struct server_list_globals {
    void **list;               // 0x00 0x007196bc, capacity GameSpy server-record pointers;
                               //      also what the &server_list returned by server_list_mutex_try_lock
                               //      result is indexed through
    int32_t result_count;      // 0x04 0x007196c0, live entry count
    int32_t capacity;          // 0x08 0x007196c4, allocated entry count
    int32_t pending_count;     // 0x0c 0x007196c8, UNSURE: entries added since the last sort;
                               //      0x4ba8a0 increments it, 0x4ba9c0 clears it
} server_list_globals;         // size 0x10
// global 0x007196bc: server_list_globals server_list
// global 0x007196a8: network_mutex_record *server_list_mutex
// global 0x007196ac: int32_t server_list_mutex_valid
// global 0x00719474: int32_t server_browser_total_players
// global 0x00719489: uint8_t server_browser_sort_column  UNSURE name, selects the comparator
// global 0x00719480: uint8_t server_browser_skip_reselect
// global 0x00719478: int32_t server_browser_scroll_offset   visible-page window start, 16 rows
// global 0x006953f4: int32_t server_browser_selected_index  -1 when nothing is selected

// The qsort comparator 0x4ba970 hands 0x4ba9c0 for the active sort column. Only two of the five
// it can return are recognized functions in the image; 0x4b6cd0, 0x4b6e70 and 0x4b6fb0 are still
// bare LAB_ labels Ghidra never promoted, so they have no rewrite (see src/networking/README.md).
typedef int32_t (*server_browser_sort_comparator)(const void *, const void *);

// The browser filter state, six adjacent bytes the filter function reads directly.
typedef struct server_browser_filters {
    uint8_t dedicated_only;    // 0x0071948b requires the dedicated key to be 1
    uint8_t classic_only;      // 0x0071948c requires the game_classic key to be 1
    uint8_t allow_unknown_map; // 0x0071948d skips the installed-map check when set
    uint8_t gametype;          // 0x0071948e 0 any, 1 CTF, 2 Slayer, 3 Oddball, 4 King, 5 Race
    uint8_t teamplay;          // 0x0071948f UNSURE polarity: 0x4b7300 rejects a server whose
                               //   teamplay key is 1 when this byte is 1, and rejects one
                               //   whose key is not 1 when this byte is 2, which reads as
                               //   1 = free for all only, 2 = team games only. The opposite
                               //   reading (matching the menu label order) is what an earlier
                               //   pass recorded; only the code above is verified.
    uint8_t ping_limit_index;  // 0x00719490 0 any, else indexes 0x00695400
} server_browser_filters;      // size 0x06
// global 0x0071948b: server_browser_filters server_browser_filter_state
// Every function reaches these six bytes as separate scalars, never through the struct, so
// src/networking uses one extern per byte. The canonical names are
//   0x0071948b uint8_t server_browser_filter_dedicated_only
//   0x0071948c uint8_t server_browser_filter_classic_only
//   0x0071948d uint8_t server_browser_filter_allow_unknown_map
//   0x0071948e uint8_t server_browser_filter_gametype
//   0x0071948f uint8_t server_browser_filter_teamplay
//   0x00719490 uint8_t server_browser_filter_ping_limit_index
// global 0x006953f0: uint8_t server_browser_require_valid_entry
// global 0x006953f9: uint8_t server_browser_allow_password
// global 0x006953fa: uint8_t server_browser_allow_empty
// global 0x006953fb: uint8_t server_browser_allow_full
// global 0x00695400: int32_t server_browser_ping_limits[]   milliseconds per index

// ---------------------------------------------------------------------------
// the advertised custom-game option codecs  (0x576180 pack, 0x576460 unpack,
// 0x5767d0 / 0x576890 the type-1 pair, 0x576900 / 0x576a10 the type-3 pair)
// What the browser squeezes into the GameSpy key/value advertisement. The wide record IS
// game_variant seen from +0x34: network_session_host_qr2_server_key passes
// server_browser_custom_options_pack(game_engine_variant + 0x34), and every field lines up with
// the game_variant field at +0x34 (the value sets the packer accepts are the options screens'
// value sets, and multiplayer_game_variant_description_generate labels each one from the
// options' own string lists). Kept as its own type because the codecs are compiled against the
// +0x34 base. The low 3 bits of every packed code are the record's own type tag.
// ---------------------------------------------------------------------------
typedef struct server_browser_custom_options {
    uint8_t teams;                 // 0x00 game_variant+0x34
    uint8_t pad_01[3];             // 0x01
    uint32_t flags;                // 0x04 game_variant+0x38 option bits
    uint32_t objective_indicator;  // 0x08 game_variant+0x3c, 2 bits (0..2)
    uint8_t odd_man_out;           // 0x0c game_variant+0x40
    uint8_t pad_0d[3];             // 0x0d
    int32_t respawn_time_growth;   // 0x10 game_variant+0x44, ticks: 0 / 150 / 300 / 450
    int32_t respawn_time;          // 0x14 game_variant+0x48, same encoding; shown as "respawn+growth" seconds
    int32_t suicide_penalty;       // 0x18 game_variant+0x4c, same encoding
    int32_t lives_per_round;       // 0x1c game_variant+0x50: 0, 1, 3 or 5
    uint32_t health_bits;          // 0x20 game_variant+0x54 health, the raw IEEE-754 bits (compared as integers)
    int32_t score_limit;           // 0x24 game_variant+0x58
    uint32_t weapon_set;           // 0x28 game_variant+0x5c, < 0xe (var_weapon_set)
    uint32_t red_vehicle_set;      // 0x2c game_variant+0x60, low nibble < 9 (var_vehicle_set)
    uint32_t blue_vehicle_set;     // 0x30 game_variant+0x64, low nibble < 9
    int32_t vehicle_respawn_time;  // 0x34 game_variant+0x68, ticks: 0 / 30 / 60 / 90 / 120 / 180 / 300 s
    uint8_t friendly_fire;         // 0x38 game_variant+0x6c, < 4 (var_friendly_fire)
    uint8_t pad_39[3];             // 0x39
    int32_t betrayal_penalty;      // 0x3c game_variant+0x70, ticks 0 / 150 / 300 / 450 (var_friendly_fire_penalty)
    uint8_t team_autobalance;      // 0x40 game_variant+0x74, one packed bit
} server_browser_custom_options; // at least 0x41

// The type-1 sub-codec. Its two halves are not the same record: the packer reads four
// booleans and one int32 time limit, the unpacker writes four booleans and a byte pair whose
// values (8/7, 16/14, 24/21, 40/35, 80/70) are nothing like the time limits the packer maps.
typedef struct server_browser_gametype1_options {
    uint8_t flags[4];          // 0x00
    int32_t time_limit;        // 0x04 0, 0x708, 0xe10, 0x1518, 9000 or 18000
} server_browser_gametype1_options; // size 0x08

typedef struct server_browser_gametype1_decoded {
    uint8_t flags[4];          // 0x00
    uint8_t low;               // 0x04
    uint8_t high;              // 0x05
    uint8_t unknown_06;        // 0x06 always cleared
    uint8_t unknown_07;        // 0x07 always cleared
} server_browser_gametype1_decoded; // size 0x08

typedef struct server_browser_gametype3_options {
    uint8_t flag0;             // 0x00
    uint8_t flag1;             // 0x01
    uint8_t pad_02[2];         // 0x02
    int32_t value_04;          // 0x04 2 bits on the wire
    int32_t value_08;          // 0x08 2 bits
    int32_t value_0c;          // 0x0c 2 bits
    int32_t value_10;          // 0x10 2 bits
    int32_t value_14;          // 0x14 5 bits
} server_browser_gametype3_options; // size 0x18

// ---------------------------------------------------------------------------
// join-game screen scratch types
//
// The join-game / server-browser screens are driven from this module but draw through the
// interface module's widget tree and through the rasterizer's client rectangle. Neither has
// a header yet, so the two foreign types below are declared here under a network_ prefix,
// exactly as types/hs.h declares hs_player_record for the player of the game module. Each
// layout is the union of the offsets the functions of this module touch, folded in from the
// per-file TYPES-GAP typedefs in src/networking/server_browser_*.c during the review pass.
// When a types/interface.h appears these move there and lose the prefix.
// ---------------------------------------------------------------------------

// A node of the menu widget tree of the interface module, as the server browser reads it.
// Only the fields the browser touches are modeled; the real node is larger than 0x5a.
typedef struct network_ui_widget network_ui_widget;
struct network_ui_widget {
    uint8_t unknown_00[0x0e];          // 0x00
    int16_t type;                      // 0x0e control-type discriminant; 2 is the list row
    uint8_t visible;                   // 0x10
    uint8_t unknown_11;                // 0x11
    uint8_t hidden;                    // 0x12 UNSURE: toggled opposite of visible everywhere
    uint8_t unknown_13[0x11];          // 0x13
    float alpha;                       // 0x24
    uint8_t unknown_28[4];             // 0x28
    network_ui_widget *next_sibling;   // 0x2c
    network_ui_widget *parent;         // 0x30
    network_ui_widget *first_child;    // 0x34
    network_ui_widget *selected_child; // 0x38 UNSURE
    uint16_t *label_text;              // 0x3c UTF-16, reallocated out of
                                       //      widget_memory_pool (see types/game.h on UTF-16)
    int16_t value;                     // 0x40
    uint8_t unknown_42[6];             // 0x42
    uint16_t max_value;                // 0x48
    uint8_t unknown_4a[2];             // 0x4a
    network_ui_widget *status_root;    // 0x4c root only: the numplayers/maxplayers/page labels
    uint8_t unknown_50[8];             // 0x50
    uint16_t highlight_flag;           // 0x58
};                                     // partial: at least 0x5a

// The client-area corner pair of the rasterizer: a packed pair of 16-bit screen coordinates.
// Read-only here: the bandwidth graph lays itself out inside the window rectangle and the
// stats overlay keeps its own text rectangle in the same shape.
typedef struct network_screen_point {
    int16_t x;                 // 0x00
    int16_t y;                 // 0x02
} network_screen_point;        // size 0x04
// global 0x0069c634: network_screen_point game_window_top_left      (owned by the renderer)
// global 0x0069c638: network_screen_point game_window_bottom_right  (owned by the renderer)
// global 0x007c1254: network_screen_point network_stats_overlay_text_rect_min  UNSURE owner
// global 0x007c1258: network_screen_point network_stats_overlay_text_rect_max  UNSURE owner

// The scrolling one-line status ticker on the join-game screen (0x4b8a00 reset,
// 0x4b8a60 append, 0x4b8b40 advance). Its string is reallocated out of
// widget_memory_pool, so the reset path patches the totals of that heap directly.
typedef struct ticker_text_buffer {
    uint16_t *text;            // 0x00 UTF-16, widget_memory_pool-backed, NULL when empty
    int32_t start_column;      // 0x04 UNSURE: synced into an external widget field by
                               //      ticker_text_buffer_advance; exact meaning unconfirmed
    int32_t scroll_cursor;     // 0x08 character index of the current visible window start
    int32_t length;            // 0x0c character count of text, excluding the NUL
    int32_t capacity;          // 0x10 allocated capacity in characters
    int32_t scroll_delay_ms;   // 0x14 per-character scroll delay, reset to 100
} ticker_text_buffer;          // size 0x18
// global 0x006b5e58: ticker_text_buffer server_browser_player_ticker
// global 0x006b5e74: ticker_text_buffer server_browser_variant_ticker
// There are exactly two instances, and every call site loads one of these two addresses into
// EDI (0x4b662f/0x4b663c/0x4b664c, 0x4b73aa/0x4b73b8/0x4b73c9, 0x4b7440, 0x4b74bc, 0x4b74f7,
// 0x4b798c/0x4b7996, 0x4b8818/0x4b8826). The names follow what feeds each one:
// server_browser_player_list_populate writes 0x006b5e58 and
// server_browser_selected_variant_description_build writes 0x006b5e74 (name confidence 0.45).
// UNSURE: the two are 0x1c apart, so four bytes past scroll_delay_ms are unaccounted for;
// nothing in this module reads them.

// ---------------------------------------------------------------------------
// other globals this module owns
// ---------------------------------------------------------------------------
// global 0x00719879: char network_build_string[]   compared by 0x4dff20
// global 0x0069fdfc: the rcon/console connection id 0x4e35c0 and 0x4deda0 use
// global 0x00722a18: int32_t master_server_state
// global 0x00722a20: void *master_server_object
// global 0x006f1d20: game_engine_definition *current_game_engine (game.h, R04; not owned
//                    here). Non-NULL means a multiplayer engine is loaded; its +0x90 slot is
//                    the ownership-handoff completion callback 0x4dfa10 invokes
// global 0x0065fd30: the "wt" fopen mode string both log files are opened with
// global 0x006ac8f8 / 0x006ac8fc: the QueryPerformanceFrequency pair every
//                    millisecond timestamp in this module divides by (owned by
//                    the cseries timer code, read here)
// global 0x00719720: int16_t network_game_mode  (types/game.h) 0 local, 1 client,
//                    2 host, 3 replay
//
// Types deliberately NOT declared here because another module already owns them:
//   game_variant, player, player_globals, update_record, position_update_record,
//   vehicle_update_record, player_update_queue, circular_queue   -- types/game.h
//   bit_stream, circular_buffer, growable_array, data_array, datum_index,
//   data_packet_group, data_packet_type                          -- types/memory.h
// The network game message table itself is the data_packet_group in memory.h at
// 0x006994f8 (39 types, maximum decoded size 0x600): 0x4414c0 registers it and
// 0x4db6b0 and 0x4e1c60 dispatch on its type byte.
#pragma pack(pop)
