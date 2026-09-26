// game_engine_notify_player_interaction  (Ghidra: FUN_00478ff0; renamed -- broadcasts the
// message game_engine_apply_player_interaction_message.c (this batch) decodes)
// address 0x478ff0, size 223 bytes
// name confidence: 0.3   rewrite confidence: 0.45
// evidence: the same hash_table_get / message_delta_encode_message / network_session_broadcast_to_flagged broadcast
//   shape already established repeatedly in this module (e.g.
//   game_engine_send_unit_weapon_loadout.c); message type 10 matches
//   game_engine_apply_player_interaction_message's own message shape (three hashed/looked-up
//   handles plus a type/seat pair).
// register convention: a hash-table key in ECX (in_ECX) and a second hash-table key in EDI
//   (unaff_EDI); `mode`, `interaction_type_and_seat` and `secondary_key` are this function's own
//   three stack parameters. Note this function always broadcasts (network_session_broadcast_to_flagged) -- there is no
//   unicast path, so EDI is NOT a machine index despite that being the usual role of an EDI
//   register argument elsewhere in this module.
//   // blam-cc: ECX -> primary_key, EDI -> edi_key, stack -> mode, interaction_type_and_seat, secondary_key
// UNSURE: the exact split of `interaction_type_and_seat` into its two 16-bit halves (modeled as
//   Ghidra's own `param_2._0_2_`/`(undefined2)param_3`, i.e. the low 16 bits of two separate
//   stack dwords); EDI's real identity beyond "a second hashed key". hash_table_get's arguments
//   are resolved (orphan pass 4 review): primary_key goes through the 0x00687558 table, edi_key and
//   the last key through the 0x00687130 object network-id table.
// UNSURE (found by the orphan pass 4 review, not fixed here): all three callers push FOUR stack
//   dwords (e.g. 0x478bc9..0x478bd1: -1, -1, 7, 1) and the body reads [entry+0xc] for the low word
//   stored at fields+0x0a but [entry+0x10] (EBP, loaded at 0x478ff5) as the last hashed key, so
//   `secondary_key` below is really two parameters; the callers' own externs disagree with this
//   signature as well.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "objects.h" // hash_table

extern uint8_t shared_hud_text_draw_state; // 0x00871de0

extern int32_t hash_table_get(hash_table *table, int32_t key); // 0x4f05e0, src/objects; blam-cc: ESI table, ECX key
extern uint8_t *object_pooled_node_globals; // 0x00687130, the object network-id hash_table sits at +0x0c
extern uint8_t *machine_table; // 0x00687558, its hash_table sits at +0x0c
extern uint8_t network_message_scratch[0x7ff8]; // 0x00871de0
extern int32_t message_delta_encode_message(int32_t extra_eax, int32_t extra_edx, int32_t flag, int32_t message_type,
    int32_t changed_offset, void **items, int32_t type_offset, int32_t count, char force_changed); // 0x4ec940, EAX buffer, EDX size
extern void *network_server_pointer; // 0x0071c2d4 (network_server_globals *)
extern void *network_server_pointer; // 0x0071c2d4 (network_server_globals *)
extern char network_session_broadcast_to_flagged(int32_t body_bit_count, void *server, int32_t param_1, void *data,
    int32_t param_3, int32_t param_4, int32_t force, int32_t param_6); // 0x4e1a80, EAX bits, ECX server

// blam-cc: ECX -> primary_key, EDI -> edi_key, stack -> mode, interaction_type_and_seat, secondary_key
// Broadcasts a networked event 10 carrying: a hash of `primary_key`, `mode` verbatim, a hash of
// `edi_key`, the low 16 bits of `interaction_type_and_seat`, the low 16 bits of `secondary_key`,
// and a hash of `secondary_key` itself.
void game_engine_notify_player_interaction(uint32_t primary_key, uint32_t edi_key,
    uint32_t mode, int32_t interaction_type_and_seat, int32_t secondary_key)
{
    struct {
        int32_t primary_hash;
        int32_t mode;
        int32_t edi_hash;
        int16_t interaction_type;
        int16_t low_secondary_key;
        int32_t secondary_hash;
    } fields;
    void *fields_ptr;
    int32_t encoded_bits;

    fields.primary_hash = 0;
    if (primary_key != 0xffffffff) {
        fields.primary_hash = hash_table_get((hash_table *)((uint8_t *)machine_table + 0xc), (int32_t)primary_key); // 0x478ffb..0x47900a
        if (fields.primary_hash == -1) {
            fields.primary_hash = 0;
        }
    }
    fields.mode = mode;
    fields.edi_hash = 0;
    if (edi_key != 0xffffffff) {
        fields.edi_hash = hash_table_get((hash_table *)(object_pooled_node_globals + 0xc), (int32_t)edi_key); // 0x47902a..0x479034
        if (fields.edi_hash == -1) {
            fields.edi_hash = 0;
        }
    }
    fields.interaction_type = (int16_t)interaction_type_and_seat;
    fields.low_secondary_key = (int16_t)secondary_key;
    fields.secondary_hash = 0;
    if (secondary_key != -1) {
        fields.secondary_hash = hash_table_get((hash_table *)(object_pooled_node_globals + 0xc), (int32_t)secondary_key); // 0x479050..0x479064, UNSURE: EBP is the 4th stack dword, see note
        if (fields.secondary_hash == -1) {
            fields.secondary_hash = 0;
        }
    }
    fields_ptr = &fields;

    encoded_bits = message_delta_encode_message((int32_t)network_message_scratch, 0x7ff8, 0, 10, 0, &fields_ptr, 0, 1, 0);
    if (0 < encoded_bits) {
        network_session_broadcast_to_flagged(encoded_bits, network_server_pointer, 1, &shared_hud_text_draw_state, 1, 0, 0, 3);
    }
}

#if 0
Original Ghidra decompilation (0x478ff0), from tools/pack.py 0x478ff0:

void FUN_00478ff0(undefined4 param_1,int *param_2,undefined4 param_3,int param_4)

{
  int iVar1;
  int in_ECX;
  int unaff_EDI;
  int local_14;
  undefined4 local_10;
  int local_c;
  undefined2 local_8;
  undefined2 local_6;
  int local_4;

  iVar1 = param_4;
  local_14 = 0;
  if (in_ECX != -1) {
    local_14 = hash_table_get();
    if (local_14 == -1) {
      local_14 = 0;
    }
  }
  local_10 = param_1;
  local_c = 0;
  if (unaff_EDI != -1) {
    local_c = hash_table_get();
    if (local_c == -1) {
      local_c = 0;
    }
  }
  local_4 = 0;
  local_8 = param_2._0_2_;
  local_6 = (undefined2)param_3;
  if (iVar1 != -1) {
    local_4 = hash_table_get();
    if (local_4 == -1) {
      local_4 = 0;
    }
  }
  param_2 = &local_14;
  param_3 = 0;
  iVar1 = message_delta_encode_message(0,10,0,&param_2,0,1,'\0');
  if (0 < iVar1) {
    FUN_004e1a80(1,&DAT_00871de0,1,0,0,3);
  }
  return;
}
#endif
