// game_engine_apply_player_interaction_message  (Ghidra: FUN_00478f10; renamed -- the network
// handler that applies an incoming pending-interaction message to a player)
// address 0x478f10, size 221 bytes
// name confidence: 0.3   rewrite confidence: 0.3
// evidence: the same "envelope" shape as game_engine_apply_player_join_message.c (this batch);
//   `machine_table` and `object_network_id_table` (both already named elsewhere in this
//   module) resolve a machine id and a pooled node id respectively; types/game.h
//   player::interaction_type/interaction_object/interaction_seat (0x28/0x24/0x2a) are written
//   directly here, matching player_set_pending_interaction_action's own field set (this batch).
// register convention: an envelope in EAX (in_EAX).
//   // blam-cc: EAX -> envelope
// UNSURE: datum_get's array argument (elided, presumably player_data); player_execute_pending_interaction/
//   player_swap_to_weapon's exact argument identity (each receives one of the two resolved handles).

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "objects.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern network_id_table *machine_table;
extern network_id_table *object_network_id_table; // 0x00687130
extern data_array *player_data;              // 0x0087a480

extern void message_delta_decode_compound_field_staged(void *event); // 0x4ec670
extern uint8_t message_delta_decode_compound_field(void *event, void *out_values); // 0x4ec590
extern void *datum_get(datum_index handle, data_array *array); // 0x4d0680; UNSURE array argument
extern uint8_t player_execute_pending_interaction(uint32_t handle); // this batch, 0x4793a0
extern uint8_t player_swap_to_weapon(uint32_t player_index, datum_index target_weapon); // 0x479240, EAX player, stack weapon

// blam-cc: EAX -> envelope
// Decodes an incoming interaction message, resolves the target player (join_key) and its
// interaction object (a pooled node id), stamps the player's interaction_type/_object/_seat
// fields, then dispatches to player_execute_pending_interaction or player_swap_to_weapon (selected by the message's mode flag)
// with a second, independently resolved handle. Returns 0 on any failure (invalid player,
// unresolved interaction object when one was expected, or the dispatched handler's own
// failure), otherwise the dispatched handler's result.
// FIXED 2026-09-28 (networking call audit, from the disassembly 0x478f10..0x478fe8): there is no join_key argument
// -- the player is the message's first field through the player key table (0x687558 +0x28, EBP); the key tables
// are indexed through the pointer at +0x28 (the C added 0x28 to the table's own address); and the swap path passes
// the player too (player_swap_to_weapon: EAX player, stack weapon).
uint8_t game_engine_apply_player_interaction_message(void **envelope)
{
    struct {
        int32_t machine_id;
        int32_t use_secondary_mode;
        int32_t interaction_pooled_id;
        int16_t interaction_type;
        int16_t interaction_seat;
        int32_t secondary_pooled_id;
    } message;

    if (*(int32_t *)*envelope != 0) {
        message_delta_decode_compound_field_staged(envelope);
        return 0;
    }
    if (!message_delta_decode_compound_field(envelope, &message)) {
        return 0;
    }

    {
        uint32_t primary_handle = 0xffffffff;
        if (message.machine_id != 0) {
            primary_handle = (uint32_t)(*(int32_t **)&machine_table->handles)[message.machine_id];
        }

        {
            player *p = (player *)datum_get((datum_index)primary_handle, player_data); // 0x478f4d: EDX handle, ESI players
            if (p == 0) {
                return 0;
            }

            {
                datum_index interaction_object = (datum_index)0xffffffff;
                uint32_t secondary_handle = 0xffffffff;

                if (message.interaction_pooled_id != 0) {
                    interaction_object = (datum_index)((int32_t *)object_network_id_table->handles)[
                        message.interaction_pooled_id];
                }
                if (message.secondary_pooled_id != 0) {
                    secondary_handle = (uint32_t)((int32_t *)object_network_id_table->handles)[
                        message.secondary_pooled_id];
                }

                if (interaction_object == (datum_index)0xffffffff && message.interaction_pooled_id != -1) {
                    // Matches the original exactly: interaction_pooled_id == 0 (lookup never
                    // attempted, interaction_object stays -1) also bails here -- only an
                    // explicit -1 pooled id skips this check.
                    return 0;
                }

                p->interaction_type = message.interaction_type;
                p->interaction_object = interaction_object;
                p->interaction_seat = message.interaction_seat;

                if (message.use_secondary_mode == 0) {
                    return player_execute_pending_interaction(primary_handle);
                }
                return player_swap_to_weapon(primary_handle, (datum_index)secondary_handle);
            }
        }
    }
}

#if 0
Original Ghidra decompilation (0x478f10), from tools/pack.py 0x478f10:

uint FUN_00478f10(void)

{
  undefined4 *in_EAX;
  uint uVar1;
  uint uVar2;
  int iVar3;
  undefined4 uVar4;
  undefined4 uVar5;
  int local_14;
  int local_10;
  int local_c;
  undefined2 local_8;
  undefined2 local_6;
  int local_4;

  if (*(int *)*in_EAX == 0) {
    uVar1 = FUN_004ec590();
    if ((char)uVar1 != '\0') {
      uVar4 = 0xffffffff;
      if (local_14 != 0) {
        uVar4 = *(undefined4 *)(*(int *)(PTR_DAT_00687558 + 0x28) + local_14 * 4);
      }
      uVar2 = datum_get();
      uVar1 = 0;
      if (uVar2 != 0) {
        iVar3 = -1;
        if (local_c != 0) {
          iVar3 = *(int *)(*(int *)(PTR_DAT_00687130 + 0x28) + local_c * 4);
        }
        uVar5 = 0xffffffff;
        if (local_4 != 0) {
          uVar5 = *(undefined4 *)(*(int *)(PTR_DAT_00687130 + 0x28) + local_4 * 4);
        }
        if ((iVar3 == -1) && (local_c != -1)) {
          return uVar2 & 0xffffff00;
        }
        *(undefined2 *)(uVar2 + 0x28) = local_8;
        *(int *)(uVar2 + 0x24) = iVar3;
        *(undefined2 *)(uVar2 + 0x2a) = local_6;
        if (local_10 == 0) {
          uVar1 = FUN_004793a0(uVar4);
          return uVar1;
        }
        uVar1 = FUN_00479240(uVar5);
        return uVar1;
      }
    }
  }
  else {
    uVar1 = FUN_004ec670();
  }
  return uVar1 & 0xffffff00;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
