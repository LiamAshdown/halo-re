// actor_died_unit_grenade_count_mod  (Ghidra: actor_died_unit_grenade_count_mod, already named)
// address 0x428d35, size 281 bytes
// name confidence: 0.6   rewrite confidence: 1.0 (FRAGMENT: 0x428d35 is the tail of actor_attempt_grenade_throw 0x428ab0..0x428e4d, whose C covers it; only its jump reaches here)
// evidence: sole caller actor_attempt_grenade_throw @0x428ab0 (this rewrite), whose own tail
//   is byte-for-byte identical to this function's body (same ammo-randomization logic, same
//   final actor_delete + encounter_recompute_morale cleanup). Ghidra's own decompile of this function reads
//   every one of its "parameters" as unaff_EBX/unaff_EBP/unaff_ESI plus raw stack slots at
//   +0x10/+0x14/+0x18 -- it shares its caller's stack frame rather than having a clean
//   parameter list of its own, strongly suggesting these two addresses are two entry points
//   into what was really one function in the source. Modeled here with the four values both
//   call sites clearly need made explicit parameters; not independently confirmed with
//   objdump. Calls actor_delete (0x427e60, already rewritten in this module),
//   weapon_set_loaded_ammo_fraction / weapon_set_ammo_counts (established names, signatures
//   guessed from argument count) and encounter_recompute_morale (outside this rewrite's range).
// register convention: EBX -> unit_object, EBP -> actor_tag_data, ESI -> weapon_object_index,
//   stack -> actor_index, encounter_index (UNSURE, see above).
//   // blam-cc: EBX -> unit_object, EBP -> actor_tag_data, ESI -> weapon_object_index,
//   //   stack -> actor_index, encounter_index

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"
#include "cache.h"
#include "objects.h"
#include "units.h"
#include "fn_ai.h"

extern uint32_t random_seed_global; // 0x00719cd0


extern void weapon_set_loaded_ammo_fraction(float fraction); // 0x4c58c0, UNSURE signature
extern void weapon_set_ammo_counts(int16_t *counts); // 0x4c5820, UNSURE signature

// blam-cc: EBX -> unit_object, EBP -> actor_tag_data, ESI -> weapon_object_index,
//   stack -> actor_index, encounter_index
// Cleanup path invoked when the actor whose grenade decision was being processed has become
// invalid: clears the unit's cached grenade-count field, randomizes the current weapon's
// loaded ammo fraction and/or count within the Actor tag's configured ranges when it has one,
// then deletes the actor and releases it from its encounter.
void actor_died_unit_grenade_count_mod(object *unit_object, const uint8_t *actor_tag_data,
                                       datum_index weapon_object_index, datum_index actor_index,
                                       datum_index encounter_index)
{
    *(int16_t *)((uint8_t *)unit_object + 0x31e) = 0;

    if (weapon_object_index != (datum_index)k_datum_index_none) {
        float min_fraction = *(const float *)(actor_tag_data + 0x1d8); // UNSURE offset (Actor tag padding)
        float max_fraction = *(const float *)(actor_tag_data + 0x1dc); // UNSURE offset

        if (min_fraction > 0.0f || max_fraction > 0.0f) {
            random_seed_global = random_seed_global * 0x19660d + 0x3c6ef35f;
            weapon_set_loaded_ammo_fraction(
                (float)(int32_t)(random_seed_global >> 0x10) * 1.5259022e-05f * (max_fraction - min_fraction) + min_fraction);
        }

        {
            int16_t min_count = *(const int16_t *)(actor_tag_data + 0x1e0); // UNSURE offset
            int16_t max_count = *(const int16_t *)(actor_tag_data + 0x1e2); // UNSURE offset

            if (min_count > 0 || max_count > 0) {
                int16_t count;
                random_seed_global = random_seed_global * 0x19660d + 0x3c6ef35f;
                count = (int16_t)((((int32_t)(int16_t)(max_count + 1) - min_count) * (int32_t)(random_seed_global >> 0x10)) >> 0x10) + min_count;
                weapon_set_ammo_counts(&count);
            }
        }
    }

    actor_delete(actor_index, 1);
    if (encounter_index != (datum_index)k_datum_index_none) {
        encounter_recompute_morale(encounter_index);
    }
}

#if 0
Original Ghidra decompilation (0x428d35):

void actor_died_unit_grenade_count_mod(void)

{
  short sVar1;
  int unaff_EBX;
  int unaff_EBP;
  int unaff_ESI;
  uint in_stack_00000010;
  uint in_stack_00000014;
  int in_stack_00000018;

  *(undefined2 *)(unaff_EBX + 0x31e) = 0;
  if (unaff_ESI != -1) {
    if ((0.0 < *(float *)(unaff_EBP + 0x1d8)) || (0.0 < *(float *)(unaff_EBP + 0x1dc))) {
      random_seed_global = random_seed_global * 0x19660d + 0x3c6ef35f;
      in_stack_00000014 = random_seed_global >> 0x10;
      weapon_set_loaded_ammo_fraction
                ((float)in_stack_00000014 * 1.5259022e-05 *
                 (*(float *)(unaff_EBP + 0x1dc) - *(float *)(unaff_EBP + 0x1d8)) +
                 *(float *)(unaff_EBP + 0x1d8));
    }
    sVar1 = *(short *)(unaff_EBP + 0x1e0);
    if ((0 < sVar1) || (0 < *(short *)(unaff_EBP + 0x1e2))) {
      random_seed_global = random_seed_global * 0x19660d + 0x3c6ef35f;
      in_stack_00000010 =
           (uint)(ushort)((short)(((int)(short)(*(short *)(unaff_EBP + 0x1e2) + 1) - (int)sVar1) *
                                  (random_seed_global >> 0x10) >> 0x10) + sVar1);
      weapon_set_ammo_counts(&stack0x00000010);
    }
  }
  actor_delete(1);
  if (in_stack_00000018 != -1) {
    FUN_00437940();
  }
  return;
}
#endif
