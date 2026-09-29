// actor_target_data_acquire  (Ghidra: actor_target_data_acquire; named from out/phase2/results/ai_02.json)
// address 0x41f7d0, size 512 bytes
// name confidence: 0.45   rewrite confidence: 0.9
// REWRITTEN from objdump 0x41f7d0..0x41f9cf: the second argument is the object (EAX of 0x43eb30, which the
//   draft passed as the actor); the copy is (EAX pair, ECX pair_reference); 0x41fc60 gets (EAX actor, EDI prop).
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

extern datum_index actor_find_or_create_shared_prop(datum_index object_index, datum_index actor_index,
    uint8_t create_if_missing, uint8_t flag); // 0x43eb30, EAX, stack
extern datum_index actor_allocate_paired_prop(datum_index actor_index, datum_index existing_prop); // 0x43e910
extern datum_index actor_allocate_paired_prop_with_kind(datum_index actor_index, datum_index existing_prop,
    datum_index reference_prop); // 0x43e980
extern void actor_copy_prop_and_reset(datum_index dest_prop, datum_index src_prop); // 0x43e840, EAX, ECX
extern void actor_target_data_refresh(uint32_t actor_index, uint32_t target_prop_index, void *reference, char force,
    char allow_reassign); // 0x41c4b0
extern void actor_target_update_tracking_speed(uint32_t actor_index, datum_index target_prop_index,
    void *scratch); // 0x41c8f0
extern uint8_t actor_target_update_active_flag(datum_index actor_index, datum_index target_prop_index); // 0x41fc60, EAX, EDI
extern float actor_rate_potential_target(datum_index actor_index, datum_index target_prop_index); // 0x41fd50

#define PROP(h) ((prop *)((uint8_t *)prop_data->data + ((h) & 0xffff) * sizeof(prop)))

// blam-cc: stack -> actor_index, object_index, owner_reference, pair_reference
uint8_t actor_target_data_acquire(datum_index actor_index, datum_index object_index, datum_index owner_reference,
                                  datum_index pair_reference)
{
    uint8_t result = 1;
    datum_index resolved;
    datum_index current;
    prop *target;
    uint8_t scratch[0x38]; // [esp+0x20] to the end of the frame: actor_get_firing_positions writes 0x38 bytes (was 0x30)

    resolved = actor_find_or_create_shared_prop(object_index, actor_index, 1, 0);
    if (resolved == k_datum_index_none) {
        return 1;
    }
    target = PROP(resolved);
    current = resolved;
    if (target->kind >= 2 && target->kind <= 3) {
        result = 0;
    } else if (target->pair_index != k_datum_index_none) {
        // 0x41f84c: already paired; refresh the pair
        datum_index pair = target->pair_index;
        prop *paired = PROP(pair);
        uint8_t fresh = 0;

        if (pair_reference != k_datum_index_none) {
            actor_copy_prop_and_reset(pair, pair_reference);
            target->object_index = paired->object_index;
        } else {
            paired->kind = 4;
            paired->unknown_3c = 0;
            fresh = 1;
        }
        actor_target_data_refresh(actor_index, pair, scratch, (char)fresh, 1);
        actor_target_update_tracking_speed(actor_index, pair, scratch);
        current = pair;
        target = PROP(pair);
    } else {
        datum_index created;

        if (pair_reference != k_datum_index_none) {
            created = actor_allocate_paired_prop_with_kind(actor_index, resolved, pair_reference);
            if (created != k_datum_index_none) {
                prop *copy = PROP(created);

                copy->object_index = target->object_index;
                copy->owner_actor_index = target->owner_actor_index;
                copy->has_parent = target->has_parent;
            }
        } else {
            actor_target_data_refresh(actor_index, resolved, scratch, 0, 0);
            created = actor_allocate_paired_prop(actor_index, resolved);
        }
        if (created == k_datum_index_none) {
            return 0;
        }
        current = created;
        target = PROP(created);
    }

    // 0x41f960
    if (owner_reference == k_datum_index_none ||
        (pair_reference != k_datum_index_none && PROP(pair_reference)->perception_grade >= 2)) {
        target->unknown_b8 = 1;
        target->unknown_b0 = 0;
        target->unknown_b4 = owner_reference;
    }
    target->engaged = actor_target_update_active_flag(actor_index, current);
    target->desirability = actor_rate_potential_target(actor_index, current);
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
