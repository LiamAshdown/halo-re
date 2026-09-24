// unit_choose_dialogue_variant  (Ghidra: unit_choose_dialogue_variant)
// address 0x561990, size 104 bytes
// name confidence: 0.3 (renamed from phase2's "unit_choose_random_permutation"; see
//   unit_pick_random_dialogue_variant, 0x561a00, for the shared evidence)   rewrite
//   confidence: 0.4
// evidence: types/objects.h object_placement_data.permutation_group (0x16, "goes to object
//   0xbe" -- the field this function prefers); types/units.h unit_data.dialogue_tag_index
//   (0x384, the destination of the chosen tag id).
// register convention: unit index in EAX.
//   // blam-cc: in_EAX -> unit_index

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"
#include "units.h"

extern data_array *object_data;     // 0x008603b0
extern tag_instance *tag_instances; // 0x0087bc14

extern TagID unit_pick_random_dialogue_variant(Unit *unit_tag, int16_t variant_number); // 0x561a00

void unit_choose_dialogue_variant(uint32_t unit_index) // blam-cc: in_EAX -> unit_index
{
    object *obj = ((object_header *)object_data->data)[unit_index & 0xffff].data;
    unit_data *unit = (unit_data *)((uint8_t *)obj + k_unit_data_offset);
    Unit *unit_tag = (Unit *)tag_instances[obj->definition_tag & 0xffff].data;
    int16_t permutation_group = *(int16_t *)((uint8_t *)obj + 0xbe); // object_placement_data.permutation_group

    TagID chosen;
    if (permutation_group > 0) {
        chosen = unit_pick_random_dialogue_variant(unit_tag, permutation_group);
        if (*(uint32_t *)&chosen != (uint32_t)-1) {
            goto done;
        }
    }
    chosen = unit_pick_random_dialogue_variant(unit_tag, 0);
    if (*(uint32_t *)&chosen == (uint32_t)-1) {
        chosen = unit_pick_random_dialogue_variant(unit_tag, -1);
    }

done:
    unit->dialogue_tag_index = *(datum_index *)&chosen;
}

#if 0
Original Ghidra decompilation (0x561990):

void FUN_00561990(void)

{
  short sVar1;
  int iVar2;
  uint in_EAX;
  int iVar3;

  iVar2 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (in_EAX & 0xffff) * 0xc);
  sVar1 = *(short *)(iVar2 + 0xbe);
  if (0 < sVar1) {
    iVar3 = FUN_00561a00(sVar1);
    if (iVar3 != -1) goto LAB_005619ef;
  }
  iVar3 = FUN_00561a00(0);
  if (iVar3 == -1) {
    iVar3 = FUN_00561a00(0xffffffff);
  }
LAB_005619ef:
  *(int *)(iVar2 + 900) = iVar3;
  return;
}
#endif
