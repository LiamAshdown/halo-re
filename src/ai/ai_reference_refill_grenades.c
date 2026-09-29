// ai_reference_refill_grenades  (Ghidra: ai_reference_refill_grenades; named for this rewrite)
// address 0x434af0, size 391 bytes
// name confidence: 0.35   rewrite confidence: 0.45
// evidence: phase-4 summary ("updates per-member derived combat booleans and randomizes a
//   grenade-count style counter using an LCG PRNG"). For every actor a packed ai reference
//   names it normalizes two unit floats to 0.0/1.0 (unit + 0xd8 / 0xdc into + 0xe0 / 0xe4),
//   then, when the actor variant tag has a grenade type at +0x180, rolls a count in
//   [+0x1d0, +0x1d2] and tops the unit's grenade counters (unit + 0x31c current type,
//   + 0x31d, + 0x31e + type the per-type count) up to it.
// register convention: EAX -> packed_reference; no stack arguments. The variant tag comes
//   from actor.actor_variant_tag (+0x5c).
//   // blam-cc: EAX -> packed_reference
//
// UNSURE: the unit-side offsets (0xd8/0xdc/0xe0/0xe4 and 0x31c..0x31e) belong to
// types/units.h and are not defined there yet, so they are accessed raw. The tag-side
// offsets 0x180 / 0x1d0 / 0x1d2 are ActorVariant fields that types/tags.h does not name.
// Signed-char handling is load-bearing here: the current grenade type at +0x31c and each
// per-type count at +0x31e are read with `movsx` in the original, so they are int8_t.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "cache.h"
#include "ai.h"
#include "units.h"
#include "fn_ai.h"

extern data_array *object_data;     // 0x008603b0
extern tag_instance *tag_instances; // 0x0087bc14
extern float k_real_zero;           // 0x00672ac0, 0.0
extern float k_real_one;            // 0x00672ac4, 1.0
extern uint32_t random_seed_global; // 0x00719cd0

extern void ai_reference_actor_iterator_new(uint32_t packed_reference,
    ai_reference_actor_iterator *out_iterator);                             // 0x432650
extern actor *ai_reference_actor_iterator_next(ai_reference_actor_iterator *iterator); // 0x4326d0

// blam-cc: EAX -> packed_reference
void ai_reference_refill_grenades(uint32_t packed_reference)
{
    ai_reference_actor_iterator iterator;
    actor *a;
    uint8_t *variant_data;
    uint8_t *unit;
    int32_t rolled;
    int16_t current;
    int16_t grenade_type;

    ai_reference_actor_iterator_new(packed_reference, &iterator);
    a = ai_reference_actor_iterator_next(&iterator);
    while (a != 0) {
        if (a->unit_index != (datum_index)k_datum_index_none) {
            variant_data = (uint8_t *)tag_instances[a->actor_variant_tag & 0xffff].data;
            unit = (uint8_t *)((object_header *)object_data->data)[a->unit_index & 0xffff].data;

            ((unit_object *)unit)->base.body_vitality = (((unit_object *)unit)->base.maximum_body_vitality <= 0.0f) ? k_real_zero : k_real_one;
            ((unit_object *)unit)->base.shield_vitality = (((unit_object *)unit)->base.maximum_shield_vitality <= 0.0f) ? k_real_zero : k_real_one;

            if (*(int16_t *)(variant_data + 0x180) != -1) {
                random_seed_global = random_seed_global * 0x19660d + 0x3c6ef35f;
                rolled = (int32_t)((((int32_t)(int16_t)(*(int16_t *)(variant_data + 0x1d2) + 1) -
                                     (int32_t)(int16_t)*(uint16_t *)(variant_data + 0x1d0)) *
                                    (int32_t)(random_seed_global >> 0x10)) >> 0x10) +
                         (int32_t)*(uint16_t *)(variant_data + 0x1d0);

                unit = (uint8_t *)((object_header *)object_data->data)
                    [a->unit_index & 0xffff].data;
                current = (int16_t)((unit_object *)unit)->unit.current_grenade_index;
                if (current == -1) {
                    current = 0;
                } else {
                    current = (int16_t)*(int8_t *)(unit + 0x31e + current);
                }

                if (current < (int16_t)rolled) {
                    grenade_type = *(int16_t *)(variant_data + 0x180);
                    unit = (uint8_t *)((object_header *)object_data->data)
                        [a->unit_index & 0xffff].data;
                    *(int8_t *)(unit + 0x31e + grenade_type) =
                        (int8_t)(*(int8_t *)(unit + 0x31e + grenade_type) +
                                 ((int8_t)rolled - (int8_t)current));
                    *(uint8_t *)&((unit_object *)unit)->unit.desired_grenade_index = (uint8_t)grenade_type;
                    *(uint8_t *)&((unit_object *)unit)->unit.current_grenade_index = (uint8_t)grenade_type;
                }
            }
        }
        a = ai_reference_actor_iterator_next(&iterator);
    }
}

#if 0
Original Ghidra decompilation (0x434af0):

void FUN_00434af0(void)

{
  char *pcVar1;
  short sVar2;
  int iVar3;
  int iVar4;
  undefined4 uVar5;
  int iVar6;
  int iVar7;
  int iVar8;
  short sVar9;
  undefined1 uVar10;

  FUN_00432650();
  iVar7 = FUN_004326d0();
  iVar6 = DAT_008603b0;
  while (iVar7 != 0) {
    if (*(uint *)(iVar7 + 0x18) != 0xffffffff) {
      iVar3 = *(int *)((*(uint *)(iVar7 + 0x5c) & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
      iVar4 = *(int *)(*(int *)(iVar6 + 0x34) + 8 + (*(uint *)(iVar7 + 0x18) & 0xffff) * 0xc);
      if (*(float *)(iVar4 + 0xd8) <= 0.0) {
        uVar5 = 0;
      }
      else {
        uVar5 = 0x3f800000;
      }
      *(undefined4 *)(iVar4 + 0xe0) = uVar5;
      if (*(float *)(iVar4 + 0xdc) <= 0.0) {
        uVar5 = 0;
      }
      else {
        uVar5 = 0x3f800000;
      }
      *(undefined4 *)(iVar4 + 0xe4) = uVar5;
      if (*(short *)(iVar3 + 0x180) != -1) {
        random_seed_global = random_seed_global * 0x19660d + 0x3c6ef35f;
        iVar8 = (((int)(short)(*(short *)(iVar3 + 0x1d2) + 1) -
                 (int)(short)*(ushort *)(iVar3 + 0x1d0)) * (random_seed_global >> 0x10) >> 0x10) +
                (uint)*(ushort *)(iVar3 + 0x1d0);
        iVar4 = *(int *)(*(int *)(iVar6 + 0x34) + 8 + (*(uint *)(iVar7 + 0x18) & 0xffff) * 0xc);
        sVar9 = (short)*(char *)(iVar4 + 0x31c);
        if (sVar9 == -1) {
          sVar9 = 0;
        }
        else {
          sVar9 = (short)*(char *)(sVar9 + 0x31e + iVar4);
        }
        if (sVar9 < (short)iVar8) {
          sVar2 = *(short *)(iVar3 + 0x180);
          iVar7 = *(int *)(*(int *)(iVar6 + 0x34) + 8 + (*(uint *)(iVar7 + 0x18) & 0xffff) * 0xc);
          pcVar1 = (char *)(sVar2 + 0x31e + iVar7);
          *pcVar1 = *pcVar1 + ((char)iVar8 - (char)sVar9);
          uVar10 = (undefined1)sVar2;
          *(undefined1 *)(iVar7 + 0x31d) = uVar10;
          *(undefined1 *)(iVar7 + 0x31c) = uVar10;
        }
      }
    }
    iVar7 = FUN_004326d0();
  }
  return;
}
#endif
