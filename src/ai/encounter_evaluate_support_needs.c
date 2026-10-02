// encounter_evaluate_support_needs  (Ghidra: encounter_evaluate_support_needs; named for this rewrite)
// address 0x436dc0, size 451 bytes
// name confidence: 0.35   rewrite confidence: 0.5
// evidence: phase-4 summary ("evaluates a squad's current combat-state counts to produce
//   boolean flags describing whether it needs reinforcement or additional support"). It
//   walks the encounter member list counting members by actor.mode (5 and 7) and by
//   actor.awareness_level (3), then reports five booleans through caller-owned out pointers.
//   actor_update_melee_combat_action @0x413d80 (already rewritten) is the only caller and
//   already passes the five out pointers.
// register convention: EAX -> encounter_index, ten stack arguments.
//   // blam-cc: EAX -> encounter_index, stack -> (self_actor_index, mode, phase,
//   //   out_crowded, out_flanked, out_a, out_b, out_reachable_a, out_reachable_b, out_any)
//
// UNSURE (verified against the disassembly at 0x436f2a, kept because it is what the binary
// does): out_any is set from whether the two out POINTERS are non-null, not from the values
// they hold -- `mov eax,[esp+0x48] / test eax,eax / jne ... / mov eax,[esp+0x4c] /
// test eax,eax / jne ... / xor eax,eax` then `mov [ecx],al`. For a fixed call site that
// makes out_any a compile-time constant. Flagged for hook verification.
//
// UNSURE: only one of out_reachable_a / out_reachable_b is written per call (whichever
// matches `phase`), so the other keeps whatever the caller left in it.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern ai_globals *ai_globals_ptr; // 0x00880354
extern data_array *encounter_data; // 0x008802c8
extern data_array *actor_data;     // 0x00880360

extern int32_t actor_find_nearest_grenade_ally(datum_index actor_index, int32_t mode); // 0x40e540, not yet rewritten

// blam-cc: EAX -> encounter_index, stack -> (self_actor_index, mode, phase, out_crowded,
//   out_flanked, out_a, out_b, out_reachable_a, out_reachable_b, out_any)
void encounter_evaluate_support_needs(datum_index encounter_index, datum_index self_actor_index,
    int16_t mode, uint8_t phase, uint8_t *out_crowded, uint8_t *out_flanked,
    uint8_t *out_a, uint8_t *out_b, uint8_t *out_reachable_a, uint8_t *out_reachable_b,
    uint8_t *out_any)
{
    encounter *enc;
    actor *a;
    actor *self;
    datum_index actor_index;
    datum_index current;
    int16_t engaged_count;
    int16_t mode7_count;
    int16_t alert_count;
    int16_t mode5_count;
    int32_t threshold;
    int32_t reachable;
    uint8_t any;
    uint8_t not_self;

    actor_index = (datum_index)k_datum_index_none;
    if (ai_globals_ptr->actors_valid != 0) {
        if (encounter_index == (datum_index)k_datum_index_none) {
            actor_index = ai_globals_ptr->first_encounterless_actor;
        } else {
            enc = &((encounter *)encounter_data->data)[encounter_index & 0xffff];
            actor_index = enc->first_actor;
        }
    }

    engaged_count = 0;
    mode7_count = 0;
    alert_count = 0;
    mode5_count = 0;

    for (;;) {
        if (ai_globals_ptr->actors_valid == 0 || actor_index == (datum_index)k_datum_index_none) {
            break;
        }
        current = actor_index;
        a = &((actor *)actor_data->data)[current & 0xffff];
        not_self = (uint8_t)(current != self_actor_index);
        actor_index = a->next_in_encounter;

        if (not_self != 0 && a->unknown_1cc == phase) {
            if (a->mode == 5) {
                if (*(int16_t *)(a->mode_data.raw + 8) == 0) { // actor + 0xa4, inside mode_data
                    if (a->combat_status < 3) {
                        mode5_count = mode5_count + 1;
                    }
                } else {
                    engaged_count = engaged_count + 1;
                }
            } else if (a->mode == 7) {
                if (*(int16_t *)(a->mode_data.raw + 8) != 0) { // actor + 0xa4, inside mode_data
                    engaged_count = engaged_count + 1;
                } else {
                    mode7_count = mode7_count + 1;
                }
            }
        }
        if (a->awareness_level == 3) {
            alert_count = alert_count + 1;
        }
    }

    if (mode == 1) {
        threshold = 0;
    } else if (mode == 2) {
        threshold = 999;
    } else {
        threshold = (int32_t)alert_count / 3;
        if (threshold < 3) {
            threshold = 3;
        }
    }

    self = &((actor *)actor_data->data)[self_actor_index & 0xffff];
    if (phase == 0) {
        actor_find_nearest_grenade_ally(self_actor_index, 0);
        *out_reachable_b = (uint8_t)(self->nearby_friend_prop_index != (datum_index)k_datum_index_none);
        self->unknown_1cc = 0;
    } else {
        reachable = actor_find_nearest_grenade_ally(self_actor_index, 1);
        *out_reachable_a = (uint8_t)(1 < reachable);
        self->unknown_1cc = (uint8_t)(1 < reachable);
    }

    // see the file header: this tests the POINTERS, not the values behind them
    any = (uint8_t)(out_reachable_a != 0 || out_reachable_b != 0);
    *out_any = any;

    *out_crowded = (uint8_t)(mode5_count < 6);
    *out_flanked = (uint8_t)(mode7_count < 4);
    *out_b = (uint8_t)(engaged_count < (int16_t)threshold);
    *out_a = (uint8_t)(engaged_count < (int16_t)threshold);
}

#if 0
Original Ghidra decompilation (0x436dc0):

void FUN_00436dc0(uint param_1,short param_2,char param_3,undefined4 param_4,undefined4 param_5,
                 undefined4 param_6,undefined4 param_7,int param_8,int param_9,undefined1 *param_10)

{
  short sVar1;
  short sVar2;
  undefined1 uVar3;
  uint in_EAX;
  int iVar4;
  int iVar5;
  short sVar6;
  int iVar7;
  short sVar8;
  bool bVar9;
  uint local_4;

  if (*(char *)(DAT_00880354 + 1) != '\0') {
    if (in_EAX == 0xffffffff) {
      local_4 = *(uint *)(DAT_00880354 + 8);
    }
    else {
      local_4 = *(uint *)((in_EAX & 0xffff) * 0x6c + 0x14 + *(int *)(DAT_008802c8 + 0x34));
    }
  }
  sVar6 = 0;
  sVar8 = 0;
  sVar1 = 0;
  sVar2 = 0;
  do {
    if ((*(char *)(DAT_00880354 + 1) == '\0') || (local_4 == 0xffffffff)) {
      if (param_2 == 1) {
        iVar4 = 0;
      }
      else if (param_2 == 2) {
        iVar4 = 999;
      }
      else {
        iVar4 = (int)sVar1 / 3;
        if (iVar4 < 3) {
          iVar4 = 3;
        }
      }
      iVar7 = (param_1 & 0xffff) * 0x724 + *(int *)(DAT_00880360 + 0x34);
      if (param_3 == '\0') {
        FUN_0040e540(param_1,0);
        *(bool *)param_9 = *(int *)(iVar7 + 0x1d0) != -1;
        *(undefined1 *)(iVar7 + 0x1cc) = 0;
      }
      else {
        iVar5 = FUN_0040e540(param_1,1);
        *(bool *)param_8 = 1 < iVar5;
        *(bool *)(iVar7 + 0x1cc) = 1 < iVar5;
      }
      if ((param_8 == 0) && (param_9 == 0)) {
        uVar3 = 0;
      }
      else {
        uVar3 = 1;
      }
      *param_10 = uVar3;
      *(bool *)param_4 = sVar2 < 6;
      *(bool *)param_5 = sVar8 < 4;
      bVar9 = sVar6 < (short)iVar4;
      *(bool *)param_7 = bVar9;
      *(bool *)param_6 = bVar9;
      return;
    }
    iVar4 = (local_4 & 0xffff) * 0x724 + *(int *)(DAT_00880360 + 0x34);
    bVar9 = local_4 != param_1;
    local_4 = *(uint *)(iVar4 + 0x2c);
    if ((bVar9) && (*(char *)(iVar4 + 0x1cc) == param_3)) {
      if (*(short *)(iVar4 + 0x6c) == 5) {
        if (*(short *)(iVar4 + 0xa4) == 0) {
          if (*(short *)(iVar4 + 0x6e) < 3) {
            sVar2 = sVar2 + 1;
          }
        }
        else {
LAB_00436e79:
          sVar6 = sVar6 + 1;
        }
      }
      else if (*(short *)(iVar4 + 0x6c) == 7) {
        if (*(short *)(iVar4 + 0xa4) != 0) goto LAB_00436e79;
        sVar8 = sVar8 + 1;
      }
    }
    if (*(short *)(iVar4 + 0x6a) == 3) {
      sVar1 = sVar1 + 1;
    }
  } while( true );
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
