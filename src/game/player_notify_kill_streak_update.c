// player_notify_kill_streak_update  (Ghidra: FUN_00479aa0; renamed -- called from
// player_add_kill_streak.c (this batch) on a dedicated server to broadcast a streak change)
// address 0x479aa0, size 145 bytes
// name confidence: 0.3   rewrite confidence: 0.4
// evidence: VERIFIED against the disassembly of the caller (objdump -d -M intel
//   --start-address=0x479ba0 --stop-address=0x479ca0): `mov ecx,ebx; push eax(amount); push
//   ebp(slot); call 0x479aa0`; the already-rewritten game_engine_notify_kill_event.c /
//   game_engine_notify_object_value_event.c (this batch) establish the same hash_table_get /
//   message_delta_encode_message / network_session_broadcast_to_flagged broadcast shape (message type 0xe here).
// register convention: a hash-table key (the player handle) in ECX (in_ECX); `slot` and
//   `amount` are this function's own two stack parameters.
//   // blam-cc: ECX -> player_handle, stack -> slot, amount
// UNSURE: hash_table_get's real argument list (elided by Ghidra, same as its every other call
//   site in this module).

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

// blam-cc: ECX -> player_handle, stack -> slot, amount
// Broadcasts a networked event 0xe carrying a hash of `player_handle` (0 if -1 or unmapped),
// `slot` and `amount` (each truncated to 16 bits).
void player_notify_kill_streak_update(int32_t slot, int16_t amount, uint32_t player_handle)
{
    struct { int32_t hash_result; int16_t slot; int16_t amount; } fields;
    void *fields_ptr;
    int32_t encoded_bits;

    fields.hash_result = 0;
    if (player_handle != 0xffffffff) {
        fields.hash_result = hash_table_get((hash_table *)((uint8_t *)machine_table + 0xc), (int32_t)player_handle); // 0x479aa5..0x479ab4
        if (fields.hash_result == -1) {
            fields.hash_result = 0;
        }
    }
    fields.slot = (int16_t)slot;
    fields.amount = amount;
    fields_ptr = &fields;

    encoded_bits = message_delta_encode_message((int32_t)network_message_scratch, 0x7ff8, 0, 0xe, 0, &fields_ptr, 0, 1, 0);
    if (0 < encoded_bits) {
        network_session_broadcast_to_flagged(encoded_bits, network_server, 1, &shared_hud_text_draw_state, 1, 0, 0, 3);
    }
}

#if 0
Original Ghidra decompilation (0x479aa0), from tools/pack.py 0x479aa0:

void FUN_00479aa0(int *param_1,undefined4 param_2)

{
  int iVar1;
  int in_ECX;
  int local_8;
  undefined2 local_4;
  undefined2 local_2;

  local_8 = 0;
  if (in_ECX != -1) {
    local_8 = hash_table_get();
    if (local_8 == -1) {
      local_8 = 0;
    }
  }
  local_4 = param_1._0_2_;
  param_1 = &local_8;
  local_2 = (undefined2)param_2;
  param_2 = 0;
  iVar1 = message_delta_encode_message(0,0xe,0,&param_1,0,1,'\0');
  if (0 < iVar1) {
    FUN_004e1a80(1,&DAT_00871de0,1,0,0,3);
  }
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
