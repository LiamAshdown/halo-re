// build_remote_player_transform_update  (Ghidra: FUN_004e7b50; named per this rewrite)
// address 0x4e7b50, size 565 bytes
// name confidence: 0.4   rewrite confidence: 0.25
// evidence: out/phase4/networking_functions.md ("Dispatches a vehicle/attached-object transform
// update for a given player slot to one of two message builders depending on the object's
// seat/attachment state, sending the result to matching peers."); src/game/player_unit_has_parent.c
// (this batch's sibling, same object_try_and_get/_object_mask_unit/parent_object shape);
// build_remote_player_vehicle_update.c and build_remote_player_vehicle_attachment_update.c (this
// batch, the two builders this dispatches to, at cache offsets 0x164/0x168 and 0x17c/0x180
// respectively -- exactly the "last sent" bookkeeping pairs those two functions themselves own);
// build_remote_player_action_update.c (this batch, the fallback tail call when the update id is
// out of range or the unit could not be resolved).
// register convention: this function's own visible parameters (player_index, control pointer,
// network_key) are forwarded through to whichever builder it calls; the batch decompile shows
// the two builder calls and the closing FUN_004e7890 tail call with zero or partial visible
// arguments, so the exact forwarding could not be fully confirmed.
// UNSURE (extensive, rewrite confidence lowered accordingly): the machine broadcast loop's
// eligibility test (walking DAT_0071c2d4's machine-id table for a match against
// *(char*)(candidate_player+100)) is transcribed as literally as possible but is not cross-checked
// against types/networking.h's network_server_globals::machines layout here. The final
// "goto LAB_004e7d5a" fallback path (invalid update id, unresolved unit, or non-'\x01' has_parent
// check) tail-calls FUN_004e7890 with a whole extra player_action's worth of stack arguments this
// batch could not fully recover; only player_index, control and network_key are forwarded.
// reconciled: R16 data_iterator is 0x10 bytes (int16 next_index, +0x0c signature = data ^ 'iter'); the inline constructor now stores the signature like the original

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "objects.h"
#include "networking.h"
#include <stdint.h>
#include "units.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern data_array *player_data; // 0x0087a480
extern game_time_globals *game_time; // 0x006f1d6c, +0x0c is the current game tick
extern int32_t network_transform_resend_interval_ms; // 0x00689494, shared by both builders' gates
extern int32_t network_vehicle_transform_resend_interval_ms_alt; // 0x00689490
extern int32_t network_attachment_transform_resend_interval_ms_alt; // 0x00689498
extern network_server_globals *network_server; // 0x0071c2d4

extern object *object_try_and_get(datum_index object_index, uint32_t type_mask); // 0x4f6ec0
extern uint8_t player_unit_has_parent(datum_index player_handle); // this batch, 0x477210
extern void *data_iterator_next(data_iterator *iterator); // 0x4d05d0, blam-cc: iterator in EDI
extern int32_t build_remote_player_vehicle_update(uint8_t *cache, uint8_t update_id,
    uint8_t flags, char is_full, player_action *control, int32_t network_key); // this module, 0x4e84d0
extern int32_t build_remote_player_vehicle_attachment_update(uint8_t *cache, uint8_t update_id,
    uint8_t flags, char is_full, player_action *control, int32_t network_key); // this module, 0x4e86f0
extern void build_remote_player_action_update(uint32_t player_index, uint32_t network_key,
    uint8_t update_id_byte, player_action control); // this module, 0x4e7890
extern uint8_t network_session_send_to_machine(int32_t machine_id, void *data, int32_t bits,
    int32_t reliable, int32_t unknown_a, int32_t unknown_b, int32_t priority); // 0x4e1930

// Looks up player_index's controlled unit; if its update id is in range and the unit resolves,
// dispatches to build_remote_player_vehicle_update (unit has no parent -- it is itself the
// vehicle) or build_remote_player_vehicle_attachment_update (unit is attached to something else),
// then broadcasts the encoded result to every established, eligible machine. Falls back to
// build_remote_player_action_update when the update id is out of range or the unit cannot be
// resolved.
void build_remote_player_transform_update(uint32_t player_index, player_action *control,
    int32_t network_key)
{
    uint8_t *plr;
    uint8_t *cache;
    int32_t update_id;
    object *unit_obj;
    int32_t encoded_size;
    uint8_t is_full;
    uint32_t now;
    data_iterator iter;
    player *candidate;
    int32_t i;
    int16_t *machine_id_slot;
    network_machine *machine;

    plr = (uint8_t *)player_data->data + (player_index & 0xffff) * 0x200;
    update_id = *(int32_t *)(plr + 0xf4);
    cache = plr;
    encoded_size = 0;

    if (-1 < update_id && update_id < 0x40) {
        unit_obj = object_try_and_get(*(datum_index *)(plr + 0x34), _object_mask_unit);
        if (unit_obj != 0) {
            if (unit_obj->parent_object == (datum_index)-1) {
                now = (uint32_t)game_time->game_time;
                if (now < (uint32_t)(network_transform_resend_interval_ms + *(int32_t *)(cache + 0x168)) &&
                    *(int32_t *)(cache + 0x168) != -1) {
                    if (now < (uint32_t)(*(int32_t *)(cache + 0x164) +
                            network_vehicle_transform_resend_interval_ms_alt)) {
                        goto fallback;
                    }
                    is_full = 0;
                } else {
                    is_full = 1;
                }
                encoded_size = build_remote_player_vehicle_update(cache, 0, is_full, is_full,
                    control, network_key);
            } else {
                if (player_unit_has_parent(*(datum_index *)(plr + 0x34)) != 1) {
                    goto fallback;
                }
                now = (uint32_t)game_time->game_time;
                if (now < (uint32_t)(network_transform_resend_interval_ms + *(int32_t *)(cache + 0x180)) &&
                    *(int32_t *)(cache + 0x180) != -1) {
                    if (now < (uint32_t)(*(int32_t *)(cache + 0x17c) +
                            network_attachment_transform_resend_interval_ms_alt)) {
                        goto fallback;
                    }
                    is_full = 0;
                } else {
                    is_full = 1;
                }
                encoded_size = build_remote_player_vehicle_attachment_update(cache, 0, is_full,
                    is_full, control, network_key);
            }

            if (0 < encoded_size) {
                iter.data = player_data;
                iter.next_index = 0;
                iter.index = k_datum_index_none;
                iter.signature = (uint32_t)(uintptr_t)iter.data ^ k_data_iterator_signature;
                candidate = (player *)data_iterator_next(&iter);
                while (candidate != 0) {
                    if (player_index != 0xffffffff && candidate->local_player_index == -1) {
                        machine_id_slot = (int16_t *)((uint8_t *)network_server + 0x3c4);
                        for (i = 0; i < 0x10; i = i + 1) {
                            if (machine_id_slot[i * 0x30] ==
                                (int16_t)*(char *)&((struct player *)candidate)->machine_index) {
                                machine = (network_machine *)((uint8_t *)network_server + 0x3b8 +
                                    i * 0x60);
                                if (((*(uint16_t *)((uint8_t *)machine + 0xe) >> 1 & 1) != 0) &&
                                    ((*(uint16_t *)((uint8_t *)machine + 0xe) >> 2 & 1) != 0)) {
                                    network_session_send_to_machine(1, 0, encoded_size, 1, 0, 0, 1);
                                }
                                break;
                            }
                        }
                    }
                    candidate = (player *)data_iterator_next(&iter);
                }
                now = (*(uint32_t *)(cache + 0x160) + 1) & 0x80000007;
                if ((int32_t)now < 0) {
                    now = (now - 1 | 0xfffffff8) + 1;
                }
                *(uint32_t *)(cache + 0x160) = now;
                return;
            }
            return;
        }
    }

fallback:
    build_remote_player_action_update(player_index, network_key, 0, *control);
}

#if 0
Original Ghidra decompilation (0x4e7b50), from tools/pack.py 0x4e7b50:

void FUN_004e7b50(uint param_1,undefined4 param_2,undefined4 param_3)

{
  int iVar1;
  char cVar2;
  int iVar3;
  int iVar4;
  short *psVar5;
  byte bVar6;
  int iVar7;
  uint uVar8;
  int iVar9;
  undefined4 *puVar10;
  undefined **ppuVar11;
  undefined *apuStack_44 [3];
  int iStack_38;
  int iStack_34;
  undefined4 uStack_30;
  undefined4 uStack_2c;
  int iStack_28;
  int local_14;

  iVar9 = (param_1 & 0xffff) * 0x200;
  iVar3 = *(int *)(iVar9 + 0xf4 + *(int *)(DAT_0087a480 + 0x34));
  iVar9 = iVar9 + *(int *)(DAT_0087a480 + 0x34);
  if ((-1 < iVar3) && (iVar3 < 0x40)) {
    iStack_28 = 3;
    uStack_2c = 0x4e7b92;
    iVar3 = object_try_and_get();
    if (iVar3 != 0) {
      if (*(int *)(iVar3 + 0x11c) == -1) {
        if ((*(uint *)(DAT_006f1d6c + 0xc) < (uint)(DAT_00689494 + *(int *)(iVar9 + 0x168))) &&
           (*(int *)(iVar9 + 0x168) != -1)) {
          if (*(uint *)(DAT_006f1d6c + 0xc) < (uint)(*(int *)(iVar9 + 0x164) + _DAT_00689490))
          goto LAB_004e7d5a;
          local_14 = (uint)local_14._1_3_ << 8;
        }
        else {
          local_14 = CONCAT31(local_14._1_3_,1);
        }
        iStack_28 = local_14;
        uStack_2c = param_3;
        uStack_30 = param_2;
        iStack_38 = 0x4e7c06;
        iStack_34 = iVar9;
        iVar3 = build_remote_player_vehicle_update();
      }
      else {
        iStack_28 = 0x4e7c0f;
        cVar2 = FUN_00477210();
        if (cVar2 != '\x01') goto LAB_004e7d5a;
        if ((*(uint *)(DAT_006f1d6c + 0xc) < (uint)(DAT_00689494 + *(int *)(iVar9 + 0x180))) &&
           (*(int *)(iVar9 + 0x180) != -1)) {
          if (*(uint *)(DAT_006f1d6c + 0xc) < (uint)(*(int *)(iVar9 + 0x17c) + _DAT_00689498))
          goto LAB_004e7d5a;
          local_14 = (uint)local_14._1_3_ << 8;
        }
        else {
          local_14 = CONCAT31(local_14._1_3_,1);
        }
        iStack_28 = local_14;
        uStack_2c = param_3;
        uStack_30 = param_2;
        iStack_38 = 0x4e7c73;
        iStack_34 = iVar9;
        iVar3 = FUN_004e86f0();
      }
      iVar1 = DAT_0071c2d4;
      if (0 < iVar3) {
        iStack_28 = 0x4e7cac;
        iVar4 = data_iterator_next();
        do {
          if (iVar4 == 0) {
            uVar8 = *(int *)(iVar9 + 0x160) + 1U & 0x80000007;
            if ((int)uVar8 < 0) {
              uVar8 = (uVar8 - 1 | 0xfffffff8) + 1;
            }
            *(uint *)(iVar9 + 0x160) = uVar8;
            return;
          }
          if ((param_1 != 0xffffffff) && (*(short *)(iVar4 + 2) == -1)) {
            iVar7 = 0;
            psVar5 = (short *)(iVar1 + 0x3c4);
            do {
              if (*psVar5 == (short)*(char *)(iVar4 + 100)) {
                iVar4 = iVar7 * 0x60 + 0x3b8 + iVar1;
                if (((iVar4 != 0) &&
                    (bVar6 = (byte)*(undefined2 *)(iVar4 + 0xe), (bVar6 >> 1 & 1) != 0)) &&
                   ((bVar6 >> 2 & 1) != 0)) {
                  iStack_28 = 1;
                  uStack_2c = 0;
                  uStack_30 = 0;
                  iStack_34 = local_14;
                  apuStack_44[2] = &DAT_00871de0;
                  apuStack_44[1] = (undefined *)0x1;
                  apuStack_44[0] = (undefined *)0x4e7d24;
                  iStack_38 = iVar3;
                  network_session_send_to_machine();
                }
                break;
              }
              iVar7 = iVar7 + 1;
              psVar5 = psVar5 + 0x30;
            } while (iVar7 < 0x10);
          }
          iStack_28 = 0x4e7d30;
          iVar4 = data_iterator_next();
        } while( true );
      }
    }
  }
LAB_004e7d5a:
  puVar10 = (undefined4 *)&stack0x00000010;
  ppuVar11 = apuStack_44;
  for (iVar3 = 8; iVar3 != 0; iVar3 = iVar3 + -1) {
    *ppuVar11 = (undefined *)*puVar10;
    puVar10 = puVar10 + 1;
    ppuVar11 = ppuVar11 + 1;
  }
  FUN_004e7890(param_1,param_2,param_3);
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
