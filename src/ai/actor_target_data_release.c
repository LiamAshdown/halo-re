// actor_target_data_release  (Ghidra: actor_target_data_release; named from out/phase2/results/ai_02.json)
// address 0x41b980, size 355 bytes
// name confidence: 0.45   rewrite confidence: 0.8 (calls fixed against objdump 0x41b980..0x41bae2; was 0.3
// evidence: out/phase2/results/ai_02.json -- for a prop (target-data record) whose kind is
//   outside 2..3, copies summary fields (0x50-0x5c, 0x9c-0xa8) from a linked (paired) entry,
//   calls actor_replace_object_reference/actor_unlink_prop/datum_delete on the pair, clears the
//   link field, sets kind=3 and clears combat flags (0xb9/0xba/0xbb), matching
//   actor_target_reset_combat_flags's tail sequence.
// register convention: EBX -> target_prop_index (unaff_EBX); param_1 (opaque, forwarded
//   unexamined to actor_replace_object_reference and actor_queue_sighted_target_dialogue -- almost certainly an
//   actor_index) and param_2 (optional output byte pointer) are Ghidra's recognized stack
//   parameters.
//   // blam-cc: EBX -> target_prop_index, stack -> actor_index, out_conflict_flag
//
// UNSURE: actor_target_has_conflicting_neighbor (actor_target_has_conflicting_neighbor, this batch) and actor_unlink_prop
// (phase2 proposes actor_firing_position_node_unlink) are both called with no visible
// arguments; declared void-argument here per the codebase's established convention for
// unresolved register calls (see e.g. unit_add_marker_relative_offset.c).

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern data_array *prop_data; // 0x008802c0

extern uint8_t actor_target_has_conflicting_neighbor(datum_index actor_index, datum_index target_prop_index); // 0x41f410, EAX, stack
extern void actor_unlink_prop(datum_index actor_index, datum_index prop_to_remove); // 0x43ea20, EAX, EDI
extern void actor_replace_object_reference(datum_index actor_index, uint32_t new_reference, uint32_t old_reference); // 0x428470, stack, ESI, EDI
extern void actor_queue_sighted_target_dialogue(datum_index actor_index, datum_index target_prop_index, uint8_t already_noticed); // 0x421c20
extern void datum_delete(data_array *array, datum_index handle); // 0x4d0510

// blam-cc: EBX -> target_prop_index, stack -> actor_index, out_conflict_flag
// Releases a linked prop (target-data record) -- deleting its paired datum and clearing the
// link -- once the actor is done actively engaging it. Returns 1 if the prop's kind made it
// eligible for release (and was released), 0 otherwise. When out_conflict_flag is non-NULL,
// it receives actor_target_has_conflicting_neighbor's result.
uint32_t actor_target_data_release(datum_index target_prop_index, uint32_t actor_index, uint8_t *out_conflict_flag)
{
    prop *target;
    prop *paired;
    datum_index pair_index;
    uint32_t result;
    uint8_t conflict;

    target = (prop *)((uint8_t *)prop_data->data + (target_prop_index & 0xffff) * sizeof(prop));
    result = 0;
    conflict = 0;

    if (target->state < 2 || 3 < target->state) {
        pair_index = target->pair_index;
        conflict = actor_target_has_conflicting_neighbor(actor_index, target_prop_index); // 0x41b9c5: EAX actor, stack target

        if (pair_index != k_datum_index_none) {
            paired = (prop *)((uint8_t *)prop_data->data + (pair_index & 0xffff) * sizeof(prop));

            target->desirability = paired->desirability;
            target->interest = paired->interest;
            target->interest_satisfied = paired->interest_satisfied;
            target->last_attention_time = paired->last_attention_time;
            target->engaged_ticks = paired->engaged_ticks;
            target->last_engaged_time = paired->last_engaged_time;
            target->engaged = paired->engaged;
            target->friends_killed = paired->friends_killed;
            target->friends_killed_timer = paired->friends_killed_timer;

            // 0x41ba61: references to the paired prop now point at the target (ESI target, EDI pair)
            actor_replace_object_reference(actor_index, target_prop_index, pair_index);
            actor_unlink_prop(actor_index, pair_index);
            datum_delete(prop_data, pair_index);
            target->pair_index = k_datum_index_none;
        }

        target->state = 3;
        target->noticed_b = 0;
        target->noticed_a = 0;
        target->noticed_c = 0;
        target->combat_dirty = 1;
        actor_queue_sighted_target_dialogue(actor_index, target_prop_index, conflict); // 0x41baa6
        result = 1;
    }

    if (out_conflict_flag != (uint8_t *)0) {
        *out_conflict_flag = conflict;
    }
    return result;
}

#if 0
Original Ghidra decompilation (0x41b980):

undefined4 FUN_0041b980(undefined4 param_1,undefined1 *param_2)

{
  short *psVar1;
  uint *puVar2;
  short sVar3;
  int iVar4;
  uint uVar5;
  undefined1 uVar6;
  undefined4 uVar7;
  int iVar8;
  int iVar9;
  uint unaff_EBX;
  int iVar10;

  iVar10 = (unaff_EBX & 0xffff) * 0x138;
  iVar4 = *(int *)(DAT_008802c0 + 0x34);
  psVar1 = (short *)(iVar4 + 0x24 + iVar10);
  sVar3 = *psVar1;
  uVar7 = 0;
  uVar6 = 0;
  if ((sVar3 < 2) || (3 < sVar3)) {
    puVar2 = (uint *)(iVar4 + 0xc + iVar10);
    uVar5 = *puVar2;
    uVar6 = FUN_0041f410();
    if (uVar5 != 0xffffffff) {
      iVar8 = (uVar5 & 0xffff) * 0x138;
      iVar9 = iVar8 + iVar4;
      *(undefined4 *)(iVar4 + 0x50 + iVar10) = *(undefined4 *)(iVar8 + 0x50 + iVar4);
      *(undefined4 *)(iVar4 + 0x54 + iVar10) = *(undefined4 *)(iVar9 + 0x54);
      *(undefined4 *)(iVar4 + 0x58 + iVar10) = *(undefined4 *)(iVar9 + 0x58);
      *(undefined4 *)(iVar4 + 0x5c + iVar10) = *(undefined4 *)(iVar9 + 0x5c);
      *(undefined2 *)(iVar4 + 0x9c + iVar10) = *(undefined2 *)(iVar9 + 0x9c);
      *(undefined4 *)(iVar4 + 0xa0 + iVar10) = *(undefined4 *)(iVar9 + 0xa0);
      *(undefined1 *)(iVar4 + 0xa4 + iVar10) = *(undefined1 *)(iVar9 + 0xa4);
      *(undefined2 *)(iVar4 + 0xa6 + iVar10) = *(undefined2 *)(iVar9 + 0xa6);
      *(undefined2 *)(iVar4 + 0xa8 + iVar10) = *(undefined2 *)(iVar9 + 0xa8);
      actor_replace_object_reference(param_1);
      FUN_0043ea20();
      datum_delete();
      *puVar2 = 0xffffffff;
    }
    iVar4 = DAT_008802c0;
    *psVar1 = 3;
    iVar10 = *(int *)(iVar4 + 0x34) + iVar10;
    *(undefined1 *)(iVar10 + 0xba) = 0;
    *(undefined1 *)(iVar10 + 0xb9) = 0;
    *(undefined1 *)(iVar10 + 0xbb) = 0;
    *(undefined1 *)(iVar10 + 100) = 1;
    FUN_00421c20(param_1);
    uVar7 = 1;
  }
  if (param_2 != (undefined1 *)0x0) {
    *param_2 = uVar6;
  }
  return uVar7;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
