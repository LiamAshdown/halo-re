// unit_dialogue_determine_variant  (Ghidra: FUN_005618e0; named in unit_new.c)
// address 0x5618e0, size 164 bytes
// name confidence: 0.5   rewrite confidence: 0.85
// evidence: objdump -d 0x5618e0..0x561983. unit_new (0x56239d) calls it with the new unit's
//   datum index in EAX. When object +0xbe (the permutation group copied from
//   object_placement_data.permutation_group) is still 0, it collects up to 16 variant numbers
//   below 100 from the unit tag's dialogue_variants (Unit +0x2b4, 0x18-byte
//   UnitDialogueVariant), picks one with a round-robin counter (0x00722794, incremented on
//   every pick) and stores it back at +0xbe.
// register convention: object index in EAX (only the low 16 bits are used).
//   // blam-cc: EAX -> object_index
// UNSURE: the counter's name; it is only touched here.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "cache.h"
#include "objects.h"
#include "units.h"

extern data_array *object_data;     // 0x008603b0
extern tag_instance *tag_instances; // 0x0087bc14
extern int32_t unit_dialogue_variant_counter; // 0x00722794 UNSURE name, only used here

// blam-cc: EAX -> object_index
// Gives a new unit a dialogue variant when its placement did not choose one.
void unit_dialogue_determine_variant(uint32_t object_index)
{
    object *obj = ((object_header *)object_data->data)[object_index & 0xffff].data;
    Unit *tag = (Unit *)tag_instances[obj->definition_tag & 0xffff].data;
    int16_t *variant = (int16_t *)((uint8_t *)obj + 0xbe);
    int16_t candidates[16];
    uint16_t count = 0;
    int16_t i;

    if (*variant != 0) {
        return;
    }
    for (i = 0; (int32_t)i < (int32_t)tag->dialogue_variants.count; i++) {
        int16_t number = ((UnitDialogueVariant *)tag->dialogue_variants.pointer)[i].variant_number;
        if (number < 100) {
            if (count >= 16) {
                break;
            }
            candidates[(int16_t)count] = number;
            count++;
        }
    }
    if ((int16_t)count > 0) {
        int32_t pick = unit_dialogue_variant_counter % (int32_t)(int16_t)count;
        unit_dialogue_variant_counter++;
        *variant = candidates[pick];
    }
}

#if 0
Original Ghidra decompilation (0x5618e0):

int FUN_005618e0(void)
{
  short *psVar1; short sVar2; uint *puVar3; int iVar4; uint in_EAX; int iVar5;
  ushort uVar6; int iVar7; short sVar8; short asStack_20 [16];
  puVar3 = *(uint **)(*(int *)(DAT_008603b0 + 0x34) + 8 + (in_EAX & 0xffff) * 0xc);
  iVar5 = (*puVar3 & 0xffff) * 0x20;
  iVar7 = *(int *)(iVar5 + 0x14 + DAT_0087bc14);
  if (*(short *)((int)puVar3 + 0xbe) == 0) {
    iVar4 = *(int *)(iVar7 + 0x2b4);
    uVar6 = 0; sVar8 = 0;
    if (0 < iVar4) {
      iVar7 = *(int *)(iVar7 + 0x2b8);
      iVar5 = 0;
      do {
        psVar1 = (short *)(iVar7 + iVar5 * 0x18);
        sVar2 = *psVar1;
        if (sVar2 < 100) {
          if (0xf < uVar6) break;
          asStack_20[(short)uVar6] = sVar2;
          uVar6 = uVar6 + 1;
        }
        sVar8 = sVar8 + 1;
        iVar5 = (int)sVar8;
      } while (iVar5 < iVar4);
      if (0 < (short)uVar6) {
        iVar7 = DAT_00722794 % (int)(short)uVar6;
        DAT_00722794 = DAT_00722794 + 1;
        *(short *)((int)puVar3 + 0xbe) = asStack_20[iVar7];
      }
    }
  }
  return iVar5;
}
#endif
