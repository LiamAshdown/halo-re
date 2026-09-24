// actor_target_data_acquire  (Ghidra: actor_target_data_acquire; named from out/phase2/results/ai_02.json)
// address 0x41f7d0, size 512 bytes
// name confidence: 0.45   rewrite confidence: 0.2
// evidence: out/phase2/results/ai_02.json -- resolves or creates the prop (target-data record)
//   for an object via actor_find_or_create_shared_prop/actor_allocate_paired_prop/actor_allocate_paired_prop_with_kind (out/phase4/ai_functions.md calls
//   these "firing-position node" helpers, but they operate on prop_data's 0x138 stride, so they
//   are really prop allocators), merging fields when the target moves between squad-member
//   owners, then refreshes it via actor_target_data_refresh (0x41c4b0) and
//   actor_target_update_tracking_speed (0x41c8f0), both this batch.
// register convention: all four parameters are Ghidra's recognized stack parameters; param_2 is
//   never read by this function's own body (kept for signature fidelity, unused).
//   // blam-cc: stack -> actor_index, unused_param, owner_reference, pair_reference
//
// UNSURE, substantially: actor_find_or_create_shared_prop/0x43e910/0x43e980/0x43e840 are outside this batch's
// rewrite range and their real parameter meanings are not established; declared with the
// literal argument shapes Ghidra shows at these call sites. actor_rate_potential_target's
// result here is read from `extraout_ST0`, i.e. its x87 return, narrowed to float exactly as
// the original does when storing it.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"

extern data_array *prop_data; // 0x008802c0

extern datum_index actor_find_or_create_shared_prop(datum_index actor_index, uint32_t flag_a, uint32_t flag_b); // 0x43eb30, UNSURE signature
extern datum_index actor_allocate_paired_prop(uint32_t actor_index, datum_index prop_index);        // 0x43e910, UNSURE signature
extern datum_index actor_allocate_paired_prop_with_kind(uint32_t actor_index, datum_index prop_index, datum_index pair_reference); // 0x43e980, UNSURE signature
extern void actor_copy_prop_and_reset(void); // 0x43e840, UNSURE signature, no traced args

extern void actor_target_data_refresh(uint32_t actor_index, uint32_t target_prop_index, void *reference, char force, char allow_reassign); // 0x41c4b0, this batch, UNSURE signature
extern void actor_target_update_tracking_speed(uint32_t actor_index, datum_index target_prop_index, void *scratch); // 0x41c8f0, this batch, UNSURE signature
extern uint8_t actor_target_update_active_flag(void); // 0x41fc60, UNSURE signature
extern float actor_rate_potential_target(datum_index actor_index, datum_index target_prop_index); // 0x41fd50

// blam-cc: stack -> actor_index, unused_param, owner_reference, pair_reference
// Finds (creating if necessary) the prop (target-data record) for a given object and refreshes
// its tracking information, transferring ownership between squad members as needed. Returns 1
// on most paths; 0 only when allocation genuinely failed.
uint8_t actor_target_data_acquire(uint32_t actor_index, uint32_t unused_param, int32_t owner_reference, uint32_t pair_reference)
{
    uint8_t result;
    datum_index resolved;
    prop *target;
    prop *paired;
    datum_index new_prop;
    uint8_t scratch[56];

    (void)unused_param;
    result = 1;

    resolved = actor_find_or_create_shared_prop(actor_index, 1, 0);
    if (resolved == k_datum_index_none) {
        return 1;
    }

    target = (prop *)((uint8_t *)prop_data->data + (resolved & 0xffff) * sizeof(prop));

    if (target->kind < 2 || 3 < target->kind) {
        if (target->pair_index == k_datum_index_none) {
            if (pair_reference == 0xffffffff) {
                actor_target_data_refresh(actor_index, resolved, scratch, 0, 0);
                new_prop = actor_allocate_paired_prop(actor_index, resolved);
            } else {
                new_prop = actor_allocate_paired_prop_with_kind(actor_index, resolved, pair_reference);
                if (new_prop != k_datum_index_none) {
                    paired = (prop *)((uint8_t *)prop_data->data + (new_prop & 0xffff) * sizeof(prop));
                    paired->object_index = target->object_index;
                    paired->owner_actor_index = target->owner_actor_index;
                    paired->has_parent = target->has_parent;
                }
            }
            if (new_prop == k_datum_index_none) {
                return 0;
            }
            target = (prop *)((uint8_t *)prop_data->data + (new_prop & 0xffff) * sizeof(prop));
        } else {
            paired = (prop *)((uint8_t *)prop_data->data + (target->pair_index & 0xffff) * sizeof(prop));
            new_prop = target->pair_index;
            if (pair_reference == 0xffffffff) {
                paired->kind = 4;
                paired->unknown_3c = 0;
            } else {
                actor_copy_prop_and_reset();
                target->object_index = paired->object_index;
            }
            actor_target_data_refresh(actor_index, new_prop, scratch, (pair_reference == 0xffffffff), 1);
            actor_target_update_tracking_speed(actor_index, new_prop, scratch);
            target = paired;
        }
    } else {
        result = 0;
        new_prop = resolved;
    }

    if (target != (prop *)0) {
        if (owner_reference == -1 ||
            (pair_reference != 0xffffffff &&
             1 < ((prop *)((uint8_t *)prop_data->data + (pair_reference & 0xffff) * sizeof(prop)))->unknown_32)) {
            target->unknown_b8 = 1;
            target->unknown_b0 = 0;
            target->unknown_b4 = owner_reference;
        }
        target->engaged = actor_target_update_active_flag();
        target->desirability = actor_rate_potential_target(actor_index, new_prop);
    }

    return result;
}

#if 0
Original Ghidra decompilation (0x41f7d0):

undefined1 FUN_0041f7d0(undefined4 param_1,undefined4 param_2,int param_3,uint param_4)

{
  short sVar1;
  undefined1 uVar2;
  uint uVar3;
  uint uVar4;
  int iVar5;
  int *piVar6;
  int iVar7;
  int iVar8;
  float10 extraout_ST0;
  undefined1 local_45;
  undefined1 local_38 [56];

  local_45 = 1;
  uVar3 = FUN_0043eb30(param_1,1,0);
  if (uVar3 == 0xffffffff) {
    return 1;
  }
  iVar5 = *(int *)(DAT_008802c0 + 0x34);
  iVar8 = (uVar3 & 0xffff) * 0x138;
  sVar1 = *(short *)(iVar8 + 0x24 + iVar5);
  piVar6 = (int *)(DAT_008802c0 + 0x34);
  iVar8 = iVar8 + iVar5;
  if ((sVar1 < 2) || (3 < sVar1)) {
    uVar4 = *(uint *)(iVar8 + 0xc);
    if (uVar4 == 0xffffffff) {
      if (param_4 == 0xffffffff) {
        FUN_0041c4b0(param_1,uVar3,local_38,0,0);
        uVar4 = FUN_0043e910(param_1,uVar3);
      }
      else {
        uVar4 = FUN_0043e980(param_1,uVar3,param_4);
        if (uVar4 != 0xffffffff) {
          iVar5 = (uVar4 & 0xffff) * 0x138 + *piVar6;
          *(undefined4 *)(iVar5 + 0x18) = *(undefined4 *)(iVar8 + 0x18);
          *(undefined4 *)(iVar5 + 0x1c) = *(undefined4 *)(iVar8 + 0x1c);
          *(undefined1 *)(iVar5 + 0x14) = *(undefined1 *)(iVar8 + 0x14);
        }
      }
      if (uVar4 == 0xffffffff) {
        return 0;
      }
      piVar6 = (int *)(DAT_008802c0 + 0x34);
      iVar8 = (uVar4 & 0xffff) * 0x138 + *(int *)(DAT_008802c0 + 0x34);
    }
    else {
      iVar7 = (uVar4 & 0xffff) * 0x138;
      if (param_4 == 0xffffffff) {
        *(undefined2 *)(iVar5 + 0x24 + iVar7) = 4;
        *(undefined2 *)(iVar5 + 0x3c + iVar7) = 0;
      }
      else {
        FUN_0043e840();
        *(undefined4 *)(iVar8 + 0x18) = *(undefined4 *)(iVar5 + 0x18 + iVar7);
      }
      FUN_0041c4b0(param_1,uVar4,local_38,param_4 == 0xffffffff,1);
      FUN_0041c8f0(param_1,uVar4,local_38);
      piVar6 = (int *)(DAT_008802c0 + 0x34);
      iVar8 = *(int *)(DAT_008802c0 + 0x34) + iVar7;
    }
  }
  else {
    local_45 = 0;
    uVar4 = uVar3;
  }
  if (iVar8 != 0) {
    if ((param_3 == -1) ||
       ((param_4 != 0xffffffff && (1 < *(short *)(*piVar6 + 0x32 + (param_4 & 0xffff) * 0x138))))) {
      *(undefined1 *)(iVar8 + 0xb8) = 1;
      *(undefined2 *)(iVar8 + 0xb0) = 0;
      *(int *)(iVar8 + 0xb4) = param_3;
    }
    uVar2 = FUN_0041fc60();
    *(undefined1 *)(iVar8 + 0xa4) = uVar2;
    actor_rate_potential_target(param_1,uVar4);
    *(float *)(iVar8 + 0x50) = (float)extraout_ST0;
  }
  return local_45;
}
#endif
