// game_engine_notify_object_value_event  (Ghidra: FUN_004779d0; named per
// out/phase4/game_functions.md: "Sends a network notification carrying a single hashed object
// index and an associated value to observers.")
// address 0x4779d0, size 175 bytes
// name confidence: 0.25   rewrite confidence: 0.4
// evidence: out/phase4/game_functions.md; the near-identical, already-rewritten
//   game_engine_notify_kill_event.c (0x4608d0) and game_engine_koth_broadcast_hill_times.c
//   (0x46b7f0) establish the same hash_table_get / message_delta_encode_message /
//   network_session_send_to_machine-or-network_session_broadcast_to_flagged shape, including the "machine_index == -1
//   means broadcast" convention.
// register convention: a hash-table key in ECX (in_ECX) and a target machine index in EDI
//   (unaff_EDI); `subject` is this function's own recognized stack parameter.
//   // blam-cc: EAX -> value_byte, ECX -> hash_key, EDI -> machine_index, stack -> subject
// UNSURE: hash_table_get's real argument list (elided by Ghidra, same as every other call site
//   of it in this module); message type 7's real meaning.
// FIXED (register inputs, objdump): EAX/AL carries an extra byte value (read at 0x4779d3,
//   `mov BYTE PTR [esp+0x4],al`, before EAX is zeroed for the hash lookup). Address arithmetic
//   through the call at 0x4779d3..0x477a2f shows the encoder's fields pointer ends up pointing
//   exactly at this byte's stack slot (0x477a0d `lea edx,[esp+0x1c]` computes that same address
//   and 0x477a11 stores it as the fields block address), so it is the first field of the
//   message_delta_encode_message field block, ahead of hash_result/subject. Callers load it from
//   varying per-call byte sources ([esi+0x1f] at 0x4dfb66, BL at 0x4dfc66), confirming it is a
//   real, caller-supplied input rather than a spilled constant. UNSURE: its exact semantic
//   meaning (named `value_byte` pending a better name).

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "objects.h" // hash_table
#include "units.h"
#include "networking.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern uint8_t shared_hud_text_draw_state; // 0x00871de0

extern int32_t hash_table_get(hash_table *table, int32_t key); // 0x4f05e0, src/objects; blam-cc: ESI table, ECX key
extern uint8_t *machine_table; // 0x00687558, its hash_table sits at +0x0c
extern uint8_t network_message_scratch[0x7ff8]; // 0x00871de0
extern int32_t message_delta_encode_message(int32_t extra_eax, int32_t extra_edx, int32_t flag, int32_t message_type,
    int32_t changed_offset, void **items, int32_t type_offset, int32_t count, char force_changed); // 0x4ec940, EAX buffer, EDX size
extern network_server_globals *network_server; // 0x0071c2d4
extern char network_session_broadcast_to_flagged(int32_t body_bit_count, void *server, int32_t status_bit, void *data,
    int32_t immediate, int32_t flush_after, int32_t force, int32_t unused); // 0x4e1a80, EAX bits, ECX server
extern void network_session_send_to_machine(uint32_t unknown_0, void *unknown_1, int32_t length,
    uint32_t unknown_3, uint32_t unknown_4, uint32_t unknown_5, uint32_t unknown_6); // 0x4e1930

// blam-cc: EAX -> value_byte, ECX -> hash_key, EDI -> machine_index, stack -> subject
// Encodes and sends a networked event 7 carrying `value_byte`, a hash-table lookup of
// `hash_key` (0 if hash_key is -1 or the lookup misses) and `subject`, to `machine_index`, or
// broadcasts it via network_session_broadcast_to_flagged when machine_index is -1.
void game_engine_notify_object_value_event(uint8_t value_byte, int32_t hash_key, int32_t machine_index, void *subject)
{
    struct { uint8_t value_byte; int32_t hash_result; void *subject; } fields;
    void *fields_ptr;
    int32_t encoded_bits;

    fields.value_byte = value_byte;
    fields.hash_result = 0;
    if (hash_key != -1) {
        fields.hash_result = hash_table_get((hash_table *)((uint8_t *)machine_table + 0xc), (int32_t)hash_key); // 0x4779d9..0x4779e8
        if (fields.hash_result == -1) {
            fields.hash_result = 0;
        }
    }
    fields.subject = subject;
    fields_ptr = &fields;

    encoded_bits = message_delta_encode_message((int32_t)network_message_scratch, 0x7ff8, 0, 7, 0, &fields_ptr, 0, 1, 0);
    if (0 < encoded_bits) {
        if (machine_index == -1) {
            network_session_broadcast_to_flagged(encoded_bits, network_server, 1, &shared_hud_text_draw_state, 0, 0, 0, 0);
            return;
        }
        network_session_send_to_machine(1, &shared_hud_text_draw_state, encoded_bits, 1, 0, 0, 3);
    }
}

#if 0
Original Ghidra decompilation (0x4779d0), from tools/pack.py 0x4779d0:

void FUN_004779d0(undefined1 *param_1)

{
  int iVar1;
  int in_ECX;
  int unaff_EDI;
  undefined1 local_10 [8];
  int local_8;
  void *local_4;

  local_8 = 0;
  if (in_ECX != -1) {
    local_8 = hash_table_get();
    if (local_8 == -1) {
      local_8 = 0;
    }
  }
  local_4 = param_1;
  param_1 = local_10;
  iVar1 = message_delta_encode_message(0,7,0,&param_1,0,1,'\0');
  if (0 < iVar1) {
    if (unaff_EDI == -1) {
      FUN_004e1a80(1,&DAT_00871de0);
      return;
    }
    network_session_send_to_machine(1,&DAT_00871de0,iVar1,1,0,0,3);
  }
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
