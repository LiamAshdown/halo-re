// actor_validate_grenade_impact_point  (Ghidra: actor_validate_grenade_impact_point, renamed)
// address 0x410710, size 112 bytes
// name confidence: 0.35   rewrite confidence: 0.4
// evidence: phase-4 summary "validates and records a candidate grenade impact point on the
// actor if the area is clear"; calls actor_score_blast_area_clear with the same ActorVariant blast/safety
// radii (0x188/0x19c) as actor_can_throw_grenade_at_target (0x40d9c0), confirming that
// reading. The candidate point (unaff_EDI) is passed to actor_score_blast_area_clear as its point argument
// and, on success, copied into actor.grenade_impact_point.
// register convention: actor_index in EAX, candidate point in EDI (Ghidra's unaff_EDI).
// blam-cc: EAX -> actor_index, EDI -> candidate_point

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "ai.h"

extern data_array *actor_data;      // 0x00880360
extern tag_instance *tag_instances; // 0x0087bc14
extern uint8_t actor_score_blast_area_clear(datum_index actor_index, float blast_radius, float safety_radius, real_point3d *point, int16_t *out_count); // this module, rewritten below

// blam-cc: EAX -> actor_index, EDI -> candidate_point
uint8_t actor_validate_grenade_impact_point(datum_index actor_index, real_point3d *candidate_point)
{
    actor *self;
    ActorVariant *variant;
    int16_t hostile_count;

    self = (actor *)((uint8_t *)actor_data->data + (actor_index & 0xffff) * sizeof(actor));
    variant = (ActorVariant *)tag_instances[self->actor_variant_tag & 0xffff].data;

    if (actor_score_blast_area_clear(actor_index, ((ActorVariant *)variant)->enemy_radius,
                      ((ActorVariant *)variant)->collateral_damage_radius, candidate_point, &hostile_count) != 0) {
        self->grenade_impact_point = *candidate_point;
        return 1;
    }
    return 0;
}

#if 0
Original Ghidra decompilation (0x410710):

uint FUN_00410710(void)

{
  int iVar1;
  undefined4 uVar2;
  uint in_EAX;
  uint uVar3;
  int iVar4;
  undefined4 *unaff_EDI;

  iVar4 = (in_EAX & 0xffff) * 0x724 + *(int *)(DAT_00880360 + 0x34);
  iVar1 = *(int *)((*(uint *)(iVar4 + 0x5c) & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
  uVar3 = actor_score_blast_area_clear(*(undefined4 *)(iVar1 + 0x188),*(undefined4 *)(iVar1 + 0x19c));
  if ((char)uVar3 != '\0') {
    *(undefined4 *)(iVar4 + 0x6a8) = *unaff_EDI;
    uVar2 = unaff_EDI[1];
    *(undefined4 *)(iVar4 + 0x6ac) = uVar2;
    *(undefined4 *)(iVar4 + 0x6b0) = unaff_EDI[2];
    return CONCAT31((int3)((uint)uVar2 >> 8),1);
  }
  return uVar3 & 0xffffff00;
}
#endif
