// actor_target_update_active_flag  (Ghidra: actor_target_update_active_flag; renamed, described as
//   unit_update_active_combat_flag in out/phase2/results/ai_02.json but the record it reads
//   is prop, addressed through prop_data with the module's 0x138 stride, not a unit record)
// address 0x41fc60, size 240 bytes
// name confidence: 0.35   rewrite confidence: 0.85 (VERIFIED 2026-09-28 against objdump 0x41fc60..0x41fd4f (EAX actor, EDI prop).)
// evidence: out/phase2/results/ai_02.json (offsets reinterpreted against types/ai.h's prop):
//   computes whether the prop (target-data record) counts as currently 'active' in combat --
//   kind 2-3, exposed (is_unit) and not a vault, and either seen by the actor's current
//   target, aiming, or actor_type 0xf -- stores it to prop.engaged, and on losing active
//   status clears prop.shots_fired/shots_hit/shots_unknown_ae and, for a helper-target actor
//   record, clears actor.unknown_3a8/unknown_3ac if this prop was the recorded helper.
// register convention: EAX -> actor_index, EDI -> target_prop_index (unaff_EDI).
//   // blam-cc: EAX -> actor_index, EDI -> target_prop_index
//
// UNSURE: actor+0x270 (types/ai.h's target_unit_index, "the unit the actor is fighting") is
// compared here directly against a PROP index (target_prop_index), not a unit object handle.
// The same pattern recurs in actor_rate_potential_target (0x41fd50), which compares its own
// prop-index parameter against the same field. This is reasonably strong evidence that
// actor+0x270 actually holds the actor's current target *prop* datum, not a raw unit handle --
// but the header's stronger citation (actor_choose_best_target) is left in place per the
// module rules, and this discrepancy is flagged for the hook-verification pass instead of
// resolved here.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"

extern data_array *actor_data; // 0x00880360
extern data_array *prop_data;  // 0x008802c0

// blam-cc: EAX -> actor_index, EDI -> target_prop_index
// Recomputes whether a prop (target-data record) counts as actively engaged in combat this
// tick and clears stale shot-counter and backup-request bookkeeping when it stops being
// active. Returns the new active flag.
uint8_t actor_target_update_active_flag(datum_index actor_index, datum_index target_prop_index)
{
    actor *self;
    prop *target;
    int16_t kind;
    uint8_t active;

    self = (actor *)((uint8_t *)actor_data->data + (actor_index & 0xffff) * sizeof(actor));
    target = (prop *)((uint8_t *)prop_data->data + (target_prop_index & 0xffff) * sizeof(prop));

    kind = target->state;
    active = 0;

    if ((((1 < kind && kind < 4) && target->enemy != 0) && target->dead == 0) &&
        (((target->engaged_ticks != 0 &&
           (self->target_unit_index == target_prop_index || self->tally.unit_props_unseen == 0)) ||
          ((target->is_vehicle_gunner != 0 || target->is_vehicle_driver != 0) &&
           (self->vehicle_gunner == 0 && self->tally.group_a_marked_135 == 0))) ||
         (target->actor_type == 0xf))) {
        active = 1;
    }

    if (target->engaged != 0 && active == 0) {
        target->shots_fired = 0;
        target->shots_unknown_ae = 0;
        target->shots_hit = 0;
    }

    if (((1 < kind && kind < 4) && active == 0) &&
        (0 < self->unknown_3a8 && self->unknown_3ac == target_prop_index)) {
        self->unknown_3a8 = 0;
        self->unknown_3ac = k_datum_index_none;
    }

    target->engaged = active;
    return active;
}

#if 0
Original Ghidra decompilation (0x41fc60):

undefined4 FUN_0041fc60(void)

{
  short sVar1;
  uint in_EAX;
  int iVar2;
  int iVar3;
  char cVar4;
  uint unaff_EDI;

  iVar2 = (in_EAX & 0xffff) * 0x724 + *(int *)(DAT_00880360 + 0x34);
  iVar3 = (unaff_EDI & 0xffff) * 0x138 + *(int *)(DAT_008802c0 + 0x34);
  sVar1 = *(short *)(iVar3 + 0x24);
  cVar4 = '\0';
  if (((((1 < sVar1) && (sVar1 < 4)) && (*(char *)(iVar3 + 0x60) != '\0')) &&
      (*(char *)(iVar3 + 0x127) == '\0')) &&
     ((((*(short *)(iVar3 + 0x9c) != 0 &&
        ((*(uint *)(iVar2 + 0x270) == unaff_EDI || (*(char *)(iVar2 + 0x1ed) == '\0')))) ||
       (((*(char *)(iVar3 + 0x135) != '\0' || (*(char *)(iVar3 + 0x136) != '\0')) &&
        ((*(char *)(iVar2 + 0x161) == '\0' && (*(char *)(iVar2 + 0x202) == '\0')))))) ||
      (*(short *)(iVar3 + 0x10) == 0xf)))) {
    cVar4 = '\x01';
  }
  if ((*(char *)(iVar3 + 0xa4) != '\0') && (cVar4 == '\0')) {
    *(undefined2 *)(iVar3 + 0xaa) = 0;
    *(undefined2 *)(iVar3 + 0xae) = 0;
    *(undefined2 *)(iVar3 + 0xac) = 0;
  }
  if ((((1 < sVar1) && (sVar1 < 4)) && (cVar4 == '\0')) &&
     ((0 < *(short *)(iVar2 + 0x3a8) && (*(uint *)(iVar2 + 0x3ac) == unaff_EDI)))) {
    *(undefined2 *)(iVar2 + 0x3a8) = 0;
    *(undefined4 *)(iVar2 + 0x3ac) = 0xffffffff;
  }
  *(char *)(iVar3 + 0xa4) = cVar4;
  return CONCAT31((int3)((uint)iVar2 >> 8),cVar4);
}
#endif
