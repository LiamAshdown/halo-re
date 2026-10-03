// ai_unit_remap_actor_to_squad  (Ghidra: ai_unit_remap_actor_to_squad; named for this rewrite)
// address 0x433970, size 241 bytes
// name confidence: 0.4   rewrite confidence: 0.9 (VERIFIED against objdump; call arguments FIXED)
// evidence: resolves a unit's controlling actor (unit_data.actor_index at object+0x1f4,
// falling back to unit_data.swarm_actor_index at +0x1f8), then finds that actor's best
// matching squad slot within a target encounter via ai_squad_find_best_matching_member
// (0x4333d0, this batch), using the actor's own actor_definition_tag/actor_variant_tag
// (types/ai.h, +0x58/+0x5c) as the match criteria and "is the actor already in the target
// encounter" as match_by_index. On a genuine change, reassigns it (actor_reset_squad_link_for_type_change) and
// optionally notifies (actor_notify_squad_and_flag_danger).
// register convention: Ghidra recognized param_1/param_2 as ordinary parameters and left
// the unit index unresolved, in ECX. The target-encounter reference (param_1) is also
// implicitly the EAX ai_squad_find_best_matching_member needs, per that function's own
// signature (not independently confirmed with objdump for this call site).
//   // blam-cc: ECX -> unit_index, stack -> packed_reference, notify

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"
#include "units.h"
#include "ai.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern data_array *object_data;     // 0x008603b0
extern data_array *actor_data;      // 0x00880360
extern tag_instance *tag_instances; // 0x0087bc14

extern int32_t ai_squad_find_best_matching_member(uint32_t packed_reference, int16_t requested_squad_index,
    uint8_t *requested_actor_data, uint8_t *requested_actor_variant_data, char match_by_index); // 0x4333d0, this batch
extern void actor_reset_squad_link_for_type_change(datum_index actor_index, datum_index encounter_index,
    int16_t squad_index); // 0x4290f0, EAX, EBX, stack
extern void actor_notify_squad_and_flag_danger(datum_index actor_index, uint8_t alternate_event,
    uint8_t raise_danger_flag); // 0x423600, EAX, ECX, stack

// blam-cc: ECX -> unit_index, stack -> packed_reference, notify
void ai_unit_remap_actor_to_squad(datum_index unit_index, uint32_t packed_reference, char notify)
{
    object_header *header = &((object_header *)object_data->data)[unit_index & 0xffff];
    unit_data *unit = (unit_data *)((uint8_t *)header->data + k_unit_data_offset);
    datum_index actor_index = unit->actor_index;

    if (actor_index == (datum_index)k_datum_index_none) {
        actor_index = unit->swarm_actor_index;
    }

    if (actor_index != (datum_index)k_datum_index_none && packed_reference != (uint32_t)k_datum_index_none) {
        actor *a = &((actor *)actor_data->data)[actor_index & 0xffff];
        char already_in_target = (a->encounter_index & 0xffff) == (packed_reference & 0xffff);
        uint8_t *actor_tag_data = (uint8_t *)tag_instances[a->actor_definition_tag & 0xffff].data;
        uint8_t *actor_variant_data = (uint8_t *)tag_instances[a->actor_variant_tag & 0xffff].data;
        int32_t best_squad = ai_squad_find_best_matching_member(packed_reference, a->squad_index, actor_tag_data,
                                                                  actor_variant_data, already_in_target);

        if ((int16_t)best_squad != -1 && (already_in_target == 0 || (int16_t)best_squad != a->squad_index)) {
            // FIXED (objdump 0x433a36..0x433a53): EAX = the actor, EBX = the target encounter (ref & 0xffff),
            //   stack = the squad; the notify gets (actor, CL = notify, 0). The draft passed only the squad.
            actor_reset_squad_link_for_type_change(actor_index, (datum_index)(packed_reference & 0xffff),
                (int16_t)best_squad);
            if (notify != 0) {
                actor_notify_squad_and_flag_danger(actor_index, (uint8_t)notify, 0);
            }
        }
    }
}

#if 0
Original Ghidra decompilation (0x433970):

void FUN_00433970(uint param_1,char param_2)

{
  int iVar1;
  undefined4 uVar2;
  uint in_ECX;
  char cVar3;
  uint uVar4;
  int iVar5;
  int iVar6;

  iVar1 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (in_ECX & 0xffff) * 0xc);
  uVar4 = *(uint *)(iVar1 + 500);
  if (((uVar4 != 0xffffffff) || (uVar4 = *(uint *)(iVar1 + 0x1f8), uVar4 != 0xffffffff)) &&
     (param_1 != 0xffffffff)) {
    iVar1 = *(int *)(DAT_00880360 + 0x34);
    iVar5 = (uVar4 & 0xffff) * 0x724;
    iVar6 = iVar5 + iVar1;
    cVar3 = '\x01' - (((*(uint *)(iVar5 + 0x34 + iVar1) ^ param_1 & 0xffff) & 0xffff) != 0);
    uVar2 = FUN_004333d0(*(undefined2 *)(iVar6 + 0x3a),
                         *(undefined4 *)
                          ((*(uint *)(iVar6 + 0x58) & 0xffff) * 0x20 + 0x14 + DAT_0087bc14),
                         *(undefined4 *)
                          ((*(uint *)(iVar5 + 0x5c + iVar1) & 0xffff) * 0x20 + 0x14 + DAT_0087bc14),
                         cVar3);
    if ((((short)uVar2 != -1) && ((cVar3 == '\0' || ((short)uVar2 != *(short *)(iVar6 + 0x3a))))) &&
       (FUN_004290f0(uVar2), param_2 != '\0')) {
      FUN_00423600(0);
    }
  }
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
