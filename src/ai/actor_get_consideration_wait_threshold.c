// actor_get_consideration_wait_threshold  (Ghidra: actor_get_consideration_wait_threshold, renamed)
// address 0x4028e0, size 243 bytes
// name confidence: 0.45   rewrite confidence: 0.45
// evidence: types/ai.h actor.actor_definition_tag/target_combat_status/vitality_wait_time;
//   types/tags.h Actor.melee_fudge_factor/melee_leap_range (already-named fields). The
//   caller's "out" pointer offsets 0x30/0x34 match actor_consider_combat_mode's
//   actor_combat_consideration TYPES (folded into types/ai.h by the review pass) struct (suicidal, distance_delta) exactly, which is
//   this function's strongest piece of corroborating evidence.
// register convention: actor index in EAX, consideration mode in CX, and the caller's
//   in-progress actor_combat_consideration record in EDI (unaff_EDI -- inherited unchanged
//   from the caller, which keeps it live in EDI across the call).
//   // blam-cc: EAX -> actor_index, ECX -> mode, EDI -> consideration
// TYPES (folded into types/ai.h by the review pass): actor_combat_consideration, defined identically to the copy in
//   actor_consider_combat_mode.c (this function only reads it, at the same two offsets).

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"
#include "cache.h"

// TYPES (folded into types/ai.h by the review pass): see actor_consider_combat_mode.c for the full note; only the two fields this
// function reads need to be correctly placed.

extern data_array *actor_data;      // 0x00880360
extern tag_instance *tag_instances; // 0x0087bc14

extern uint8_t actor_has_unshielded_threat_weapon(void); // 0x428370, not yet rewritten (outside this session's range)

// Computes the wait/reaction-time threshold the actor should use for consideration mode:
//   2 or 3 - a melee-timing based search wait (mode 3 additionally floors it at the tag's
//            melee_leap_range[1]); combined with the suicidal flag exactly as
//            actor_consider_combat_mode set it.
//   0 or 4 - the vitality-based delay (actor.vitality_wait_time), gated on actor_has_unshielded_threat_weapon and
//            the actor's target_combat_status exceeding 6.
//   anything else - 0.0
float actor_get_consideration_wait_threshold(uint32_t actor_index, int16_t mode, actor_combat_consideration *consideration)
{
    actor *a = &((actor *)actor_data->data)[actor_index & 0xffff];
    float result = 0.0f;

    if (mode == 2 || mode == 3) {
        Actor *actor_def = (Actor *)tag_instances[a->actor_definition_tag & 0xffff].data;

        if (mode == 3 && actor_def->melee_leap_range[1] >= 0.0f) {
            result = actor_def->melee_leap_range[1];
        }
        if (consideration->suicidal == 0) {
            float candidate = actor_def->melee_fudge_factor + consideration->distance_delta;
            if (result <= candidate) {
                result = candidate;
            }
        } else if (result <= actor_def->melee_fudge_factor) {
            return actor_def->melee_fudge_factor;
        }
    } else if (mode == 4 || mode == 0) {
        if (actor_has_unshielded_threat_weapon() != 0 && a->target_combat_status > 6 && a->vitality_wait_time >= 0.0f) {
            return a->vitality_wait_time;
        }
    }
    return result;
}

#if 0
Original Ghidra decompilation (0x4028e0):

float10 FUN_004028e0(void)

{
  float fVar1;
  char cVar2;
  uint in_EAX;
  short in_CX;
  int iVar3;
  int unaff_EDI;
  float10 fVar4;
  float10 extraout_ST0;

  fVar4 = (float10)0.0;
  iVar3 = (in_EAX & 0xffff) * 0x724 + *(int *)(DAT_00880360 + 0x34);
  if ((in_CX == 2) || (in_CX == 3)) {
    iVar3 = *(int *)((*(uint *)(iVar3 + 0x58) & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
    if ((in_CX == 3) && (0.0 <= *(float *)(iVar3 + 0x388))) {
      fVar4 = (float10)*(float *)(iVar3 + 0x388);
    }
    if (*(char *)(unaff_EDI + 0x30) == '\0') {
      fVar1 = *(float *)(iVar3 + 0x37c) + *(float *)(unaff_EDI + 0x34);
      if (fVar4 <= (float10)fVar1) {
        fVar4 = (float10)fVar1;
      }
    }
    else if (fVar4 <= (float10)*(float *)(iVar3 + 0x37c)) {
      return (float10)*(float *)(iVar3 + 0x37c);
    }
  }
  else if ((in_CX == 4) || (in_CX == 0)) {
    cVar2 = FUN_00428370();
    fVar4 = extraout_ST0;
    if ((cVar2 != '\0') && ((6 < *(short *)(iVar3 + 0x268) && (0.0 <= *(float *)(iVar3 + 0x608)))))
    {
      return (float10)*(float *)(iVar3 + 0x608);
    }
  }
  return fVar4;
}
#endif
