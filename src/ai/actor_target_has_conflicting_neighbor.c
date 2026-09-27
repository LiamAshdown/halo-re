// actor_target_has_conflicting_neighbor  (Ghidra: actor_target_has_conflicting_neighbor; named from out/phase2/results/ai_02.json)
// address 0x41f410, size 311 bytes
// name confidence: 0.4   rewrite confidence: 0.85 (VERIFIED 2026-09-28 against objdump 0x41f410..0x41f546 (6.25 horizontal, |dz| < 1.5 double, facing dot > 0.5 summed z,y,x; kinds 2..5; returns BL).)
// evidence: out/phase2/results/ai_02.json -- walks the actor's own prop (target-data) linked
//   list looking for another entry that shares the same tracked object or owning actor, or
//   whose position is within 2.5 units horizontally / 1.5 vertically and facing similarly (dot
//   product > 0.5), to avoid duplicate firing-position/object assignment. Matches
//   prop.object_index/owner_actor_index/is_unit/kind/last_known_position/unknown_e0 in
//   types/ai.h.
// register convention: EAX -> actor_index; target_prop_index is Ghidra's recognized parameter.
//   // blam-cc: EAX -> actor_index, stack -> target_prop_index
//
// UNSURE: the original's return packs a literal 0xffffff into the upper 24 bits
// (CONCAT31(0xffffff, uVar8)), which looks like leftover EAX content rather than a second
// value; only the low byte (0 or 1) is meaningful and is what this rewrite returns.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"

// sqrt/fabs are single x87 instructions (FSQRT/FABS) in the original code, which
// Ghidra renders as the pseudo-functions SQRT()/ABS(); declared locally instead of via
// <math.h> because -I types shadows that header name with types/math.h.
extern double fabs(double x); // FABS
static float fabs_f(float x) { return (float)fabs((double)x); }

extern data_array *actor_data; // 0x00880360
extern data_array *prop_data;  // 0x008802c0

// blam-cc: EAX -> actor_index, stack -> target_prop_index
// Checks whether another of the actor's tracked props occupies a conflicting firing position
// or object, to avoid duplicate assignment.
uint8_t actor_target_has_conflicting_neighbor(datum_index actor_index, datum_index target_prop_index)
{
    actor *self;
    prop *target;
    prop *other;
    datum_index prop_index;
    uint8_t conflict;
    float dx, dy;
    int16_t other_kind;

    self = (actor *)((uint8_t *)actor_data->data + (actor_index & 0xffff) * sizeof(actor));
    target = (prop *)((uint8_t *)prop_data->data + (target_prop_index & 0xffff) * sizeof(prop));
    conflict = 0;

    prop_index = self->first_prop;
    while (prop_index != k_datum_index_none) {
        other = (prop *)((uint8_t *)prop_data->data + (prop_index & 0xffff) * sizeof(prop));
        if (prop_index != target_prop_index) {
            other_kind = other->kind;
            if (((other->object_index == target->object_index ||
                  other->owner_actor_index == target->owner_actor_index) ||
                 ((target->is_unit != 0 && other->is_unit != 0) &&
                  ((3 < other_kind && other_kind < 6) || (1 < other_kind && other_kind < 4)))) &&
                (((dx = target->last_known_position.x - other->last_known_position.x,
                   dy = target->last_known_position.y - other->last_known_position.y,
                   dx * dx + dy * dy < 6.25f) &&
                  fabs_f(other->last_known_position.z - target->last_known_position.z) < 1.5f) &&
                 (0.5f < other->unknown_e0.z * target->unknown_e0.z +
                         other->unknown_e0.y * target->unknown_e0.y +
                         other->unknown_e0.x * target->unknown_e0.x))) { // 0x41f503: summed z, y, x
                conflict = 1;
            }
        }
        prop_index = other->next_in_actor;
    }

    return conflict;
}

#if 0
Original Ghidra decompilation (0x41f410):

undefined4 FUN_0041f410(uint param_1)

{
  short sVar1;
  int iVar2;
  uint uVar3;
  float fVar4;
  float fVar5;
  uint in_EAX;
  int iVar6;
  int iVar7;
  undefined1 uVar8;
  uint uVar9;
  bool bVar10;

  iVar2 = *(int *)(DAT_008802c0 + 0x34);
  iVar7 = (param_1 & 0xffff) * 0x138 + iVar2;
  uVar8 = 0;
  uVar9 = *(uint *)((in_EAX & 0xffff) * 0x724 + 0x50 + *(int *)(DAT_00880360 + 0x34));
  while (uVar9 != 0xffffffff) {
    iVar6 = (uVar9 & 0xffff) * 0x138;
    uVar3 = *(uint *)(iVar6 + 8 + iVar2);
    iVar6 = iVar6 + iVar2;
    bVar10 = uVar9 != param_1;
    uVar9 = uVar3;
    if ((bVar10) &&
       ((((*(int *)(iVar6 + 0x18) == *(int *)(iVar7 + 0x18) ||
          (*(int *)(iVar6 + 0x1c) == *(int *)(iVar7 + 0x1c))) ||
         (((*(char *)(iVar7 + 0x60) != '\0' && (*(char *)(iVar6 + 0x60) != '\0')) &&
          (((sVar1 = *(short *)(iVar6 + 0x24), 3 < sVar1 && (sVar1 < 6)) ||
           ((1 < sVar1 && (sVar1 < 4)))))))) &&
        (((fVar4 = *(float *)(iVar7 + 0xbc) - *(float *)(iVar6 + 0xbc),
          fVar5 = *(float *)(iVar7 + 0xc0) - *(float *)(iVar6 + 0xc0),
          fVar4 * fVar4 + fVar5 * fVar5 < 6.25 &&
          (ABS(*(float *)(iVar6 + 0xc4) - *(float *)(iVar7 + 0xc4)) < 1.5)) &&
         (0.5 < *(float *)(iVar6 + 0xe0) * *(float *)(iVar7 + 0xe0) +
                *(float *)(iVar6 + 0xe4) * *(float *)(iVar7 + 0xe4) +
                *(float *)(iVar6 + 0xe8) * *(float *)(iVar7 + 0xe8))))))) {
      uVar8 = 1;
    }
  }
  return CONCAT31(0xffffff,uVar8);
}
#endif
