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
//   stack dwords) and hash_table_get's real argument list (elided, same as its every other call
//   site in this module); EDI's real identity beyond "a second hashed key".

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"

extern uint8_t shared_hud_text_draw_state; // 0x00871de0

extern int32_t hash_table_get(void); // 0x4f05e0
extern int32_t message_delta_encode_message(uint32_t unknown_0, uint32_t message_type,
    uint32_t unknown_2, void **fields, uint32_t unknown_4, uint32_t unknown_5,
    uint8_t unknown_6); // 0x4ec940
extern void network_session_broadcast_to_flagged(uint32_t unknown_0, void *unknown_1, uint32_t unknown_2, uint32_t unknown_3,
    uint32_t unknown_4, uint32_t unknown_5); // 0x4e1a80

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
        fields.primary_hash = hash_table_get();
        if (fields.primary_hash == -1) {
            fields.primary_hash = 0;
        }
    }
    fields.mode = mode;
    fields.edi_hash = 0;
    if (edi_key != 0xffffffff) {
        fields.edi_hash = hash_table_get();
        if (fields.edi_hash == -1) {
            fields.edi_hash = 0;
        }
    }
    fields.interaction_type = (int16_t)interaction_type_and_seat;
    fields.low_secondary_key = (int16_t)secondary_key;
    fields.secondary_hash = 0;
    if (secondary_key != -1) {
        fields.secondary_hash = hash_table_get();
        if (fields.secondary_hash == -1) {
            fields.secondary_hash = 0;
        }
    }
    fields_ptr = &fields;

    encoded_bits = message_delta_encode_message(0, 10, 0, &fields_ptr, 0, 1, 0);
    if (0 < encoded_bits) {
        network_session_broadcast_to_flagged(1, &shared_hud_text_draw_state, 1, 0, 0, 3);
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
