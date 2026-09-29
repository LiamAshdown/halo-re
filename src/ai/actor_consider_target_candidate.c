// actor_consider_target_candidate  (Ghidra: actor_consider_target_candidate, already named)
// address 0x4208a0, size 196 bytes
// name confidence: 0.55   rewrite confidence: 0.85 (VERIFIED against objdump 0x4208a0..0x420963 (rate(stack actor, candidate); store at candidate +0x50; positive and not below the current target's rating -> take it, reset +0x268/+0x26c, update combat status and awareness (EAX actor)))
// evidence: out/phase2/results/ai_02.json -- rates a single candidate prop (unaff_EBX) with
//   actor_rate_potential_target, stores the score into the candidate's own desirability field,
//   and only replaces the actor's current target (actor.target_unit_index) if the new score
//   exceeds the existing target's cached score, then calls the same target-status/awareness
//   refresh pair as actor_choose_best_target. This call site is also the clearest evidence that
//   actor.target_unit_index (0x270) holds a PROP datum index, not a raw unit handle: it is
//   assigned directly from candidate_prop_index here.
// register convention: a single stack argument, actor_index (0x4208a6 reads [esp+8] straight
//   after one push), plus candidate_prop_index in EBX (unaff_EBX). Both call sites (0x4036af and
//   0x40ddcb) push exactly one dword and leave the candidate handle in EBX.
//   // blam-cc: stack -> actor_index, EBX -> candidate_prop_index
//
// UNSURE: the original computes the score in an 80-bit float10 and only narrows to float when
// storing it; this rewrite uses float throughout (actor_rate_potential_target's own return type
// is likewise narrowed to float here), which can differ in the last bit or two from the
// original's 80-bit intermediate comparisons.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"
#include "fn_ai.h"

extern data_array *actor_data; // 0x00880360
extern data_array *prop_data;  // 0x008802c0


// blam-cc: stack -> actor_index, EBX -> candidate_prop_index
// Evaluates one specific candidate prop against the actor's current target and swaps to it as
// the new target if it scores higher. Returns 1 if the candidate became the new target.
uint16_t actor_consider_target_candidate(datum_index actor_index, datum_index candidate_prop_index)
{
    actor *self;
    prop *candidate;
    prop *current_target;
    datum_index current_target_index;
    float score;

    self = (actor *)((uint8_t *)actor_data->data + (actor_index & 0xffff) * sizeof(actor));
    candidate = (prop *)((uint8_t *)prop_data->data + (candidate_prop_index & 0xffff) * sizeof(prop));

    current_target_index = self->target_unit_index;
    current_target = (current_target_index == k_datum_index_none) ? (prop *)0 :
                      (prop *)((uint8_t *)prop_data->data + (current_target_index & 0xffff) * sizeof(prop));

    score = actor_rate_potential_target(actor_index, candidate_prop_index);
    candidate->desirability = score;

    if (0.0f < score) {
        if (current_target != (prop *)0 && score < current_target->desirability) {
            return 0;
        }
        self->target_combat_status = 0;
        self->target_unit_index = candidate_prop_index;
        self->target_status_tick = k_datum_index_none;
        actor_update_target_combat_status(actor_index);
        actor_update_awareness_level(actor_index);
        return 1;
    }
    return 0;
}

#if 0
Original Ghidra decompilation (0x4208a0):

undefined2 actor_consider_target_candidate(uint param_1)

{
  int iVar1;
  uint uVar2;
  uint uVar3;
  uint unaff_EBX;
  int iVar4;
  float10 fVar5;

  uVar3 = param_1;
  iVar1 = *(int *)(DAT_008802c0 + 0x34);
  iVar4 = (param_1 & 0xffff) * 0x724;
  uVar2 = *(uint *)(iVar4 + 0x270 + *(int *)(DAT_00880360 + 0x34));
  iVar4 = iVar4 + *(int *)(DAT_00880360 + 0x34);
  if (uVar2 == 0xffffffff) {
    param_1 = 0;
  }
  else {
    param_1 = (uVar2 & 0xffff) * 0x138 + iVar1;
  }
  fVar5 = (float10)actor_rate_potential_target(uVar3);
  *(float *)((unaff_EBX & 0xffff) * 0x138 + iVar1 + 0x50) = (float)fVar5;
  if ((float10)0.0 < fVar5) {
    if ((param_1 != 0) && (fVar5 < (float10)*(float *)(param_1 + 0x50))) {
      return 0;
    }
    *(undefined2 *)(iVar4 + 0x268) = 0;
    *(uint *)(iVar4 + 0x270) = unaff_EBX;
    *(undefined4 *)(iVar4 + 0x26c) = 0xffffffff;
    actor_update_target_combat_status();
    actor_update_awareness_level();
    return 1;
  }
  return 0;
}
#endif
