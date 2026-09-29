// unit_pick_random_spawned_actor_count  (Ghidra: FUN_00568540)
// address 0x568540, size 193 bytes
// name confidence: 0.35 (functions.md: "Lazily computes and caches a random variant/permutation
//   index for a unit, seeded from the global PRNG")   rewrite confidence: 0.4
// evidence: types/units.h unit_flags._unit_flag_permutation_chosen (0x20000, "0x568540 caches a
//   random variant once" -- this exact function); types/tags.h Unit.spawned_actor (TagDependency
//   at 0x24c, tag_id at 0x258) and Unit.spawned_actor_count[2] (int16 at 0x25c/0x25e).
// register convention: unit index arrives in EDI, unaffected by anything earlier in the caller.
//   // blam-cc: unaff_EDI -> unit_index
// UNSURE: the final CONCAT22 term reconstructs a 32-bit value from a 16-bit low half
//   (spawned_actor_count[0]) and an upper half taken from `DAT_0087bc14`'s own high 16 bits,
//   which is almost certainly decompiler noise (the real source likely just adds the two
//   int16s as plain ints); reproduced literally rather than simplified since it is bit-exact.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"
#include "units.h"
#include "fn_ai.h"

extern data_array *object_data;     // 0x008603b0
extern tag_instance *tag_instances; // 0x0087bc14
extern random_seed random_seed_global;   // 0x00719cd0


int32_t unit_pick_random_spawned_actor_count(uint32_t unit_index) // blam-cc: unaff_EDI -> unit_index
{
    object *unit_obj = ((object_header *)object_data->data)[unit_index & 0xffff].data;
    int32_t result = 0;

    unit_data *unit = (unit_data *)((uint8_t *)unit_obj + k_unit_data_offset);
    if ((unit->flags & _unit_flag_permutation_chosen) == 0) {
        Unit *unit_tag = (Unit *)tag_instances[unit_obj->definition_tag & 0xffff].data;
        if (*(int32_t *)&unit_tag->spawned_actor.tag_id != -1) {
            random_seed_global = random_seed_global * 0x19660d + 0x3c6ef35f;
            int32_t range = (int32_t)(int16_t)(unit_tag->spawned_actor_count[1] + 1) - (int32_t)unit_tag->spawned_actor_count[0];
            result = (range * (int32_t)(random_seed_global >> 0x10) >> 0x10) +
                     (int32_t)((((uint32_t)tag_instances >> 16) << 16) | (uint16_t)unit_tag->spawned_actor_count[0]);
            if (0 < (int16_t)result) {
                // FIXED (objdump 0x5685d2..0x5685eb): EBX = the tag's spawned actor (+0x258), DX = the count,
                //   stack = (this unit, tag +0x260 * 1/30). The draft passed only the count.
                result = actor_spawn_additional_units(*(datum_index *)&((struct Unit *)unit_tag)->spawned_actor.tag_id, (int16_t)result,
                    unit_index, ((struct Unit *)unit_tag)->spawned_velocity * 0.033333335f);
            }
            unit->flags |= _unit_flag_permutation_chosen;
        }
    }
    return result;
}

#if 0
Original Ghidra decompilation (0x568540):

int FUN_00568540(void)

{
  uint *puVar1;
  int iVar2;
  int iVar3;
  uint unaff_EDI;

  puVar1 = *(uint **)(*(int *)(DAT_008603b0 + 0x34) + 8 + (unaff_EDI & 0xffff) * 0xc);
  iVar3 = 0;
  if ((puVar1[0x81] & 0x20000) == 0) {
    iVar2 = *(int *)((*puVar1 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
    if (*(int *)(iVar2 + 600) != -1) {
      random_seed_global = random_seed_global * 0x19660d + 0x3c6ef35f;
      iVar3 = (((int)(short)(*(short *)(iVar2 + 0x25e) + 1) - (int)*(short *)(iVar2 + 0x25c)) *
               (random_seed_global >> 0x10) >> 0x10) +
              CONCAT22((short)((uint)DAT_0087bc14 >> 0x10),*(short *)(iVar2 + 0x25c));
      if (0 < (short)iVar3) {
        iVar3 = FUN_00427280();
      }
      puVar1[0x81] = puVar1[0x81] | 0x20000;
    }
  }
  return iVar3;
}
#endif
