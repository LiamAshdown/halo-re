// unit_weapon_is_best_of_type  (Ghidra: FUN_0056dae0)
// address 0x56dae0, size 189 bytes, name confidence 0.4, rewrite confidence: 0.85 (VERIFIED 2026-09-27 static loop against objdump 0x56dae0..0x56db9c; unit_data offsets probed)
// functions.md: "Compares a reference weapon against every weapon the unit carries of the same
// type, checking whether it is the current weapon and has the lowest score field."
// evidence: types/units.h unit_data.weapons[4] (0x2f8), .current_weapon_index (0x2f2);
//   types/objects.h object.definition_tag. The weapon "score" field at object+0x240 (word 0x90)
//   is not named by this module's header.
// blam-cc: in_EAX -> reference_weapon_index, in_ECX -> unit_index.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "units.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern data_array *object_data; // 0x008603b0

uint8_t unit_weapon_is_best_of_type(uint32_t reference_weapon_index, uint32_t unit_index) // blam-cc: in_EAX, in_ECX
{
    object *unit_obj = ((object_header *)object_data->data)[unit_index & 0xffff].data;
    unit_data *unit = (unit_data *)((uint8_t *)unit_obj + k_unit_data_offset);
    object *reference_obj = ((object_header *)object_data->data)[reference_weapon_index & 0xffff].data;

    // FIXED 2026-09-27 (static loop): 0x56db05..0x56db27 / 0x56db97: no current weapon (index -1 or an empty slot)
    // returns 0 (xor al,al); the draft returned 1.
    if (unit->current_weapon_index == -1) {
        return 0;
    }
    datum_index current_weapon = unit->weapons[unit->current_weapon_index];
    if (current_weapon == k_datum_index_none) {
        return 0;
    }

    uint8_t result = 1;
    for (int16_t slot = 0; slot < k_maximum_weapons_per_unit; slot++) {
        datum_index weapon_index = unit->weapons[slot];
        if (weapon_index == k_datum_index_none) {
            continue;
        }
        object *slot_weapon = ((object_header *)object_data->data)[weapon_index & 0xffff].data;
        if (reference_obj->definition_tag != slot_weapon->definition_tag) {
            continue;
        }
        if (slot == unit->current_weapon_index) {
            float slot_score = *(float *)((uint8_t *)slot_weapon + 0x240);
            if (!(slot_score < 0.0f) && (slot_score != 0.0f)) {
                float ref_score = *(float *)((uint8_t *)reference_obj + 0x240);
                if (ref_score < slot_score) {
                    continue; // keep result == 1
                }
            }
        }
        result = 0;
    }
    return result;
}

#if 0
Original Ghidra decompilation (0x56dae0):

uint FUN_0056dae0(void)

{
  float fVar1;
  float fVar2;
  int iVar3;
  uint *puVar4;
  uint *puVar5;
  undefined1 uVar6;
  uint in_EAX;
  uint uVar7;
  uint in_ECX;
  int iVar9;
  uint *puVar10;
  short sVar11;
  undefined2 uVar8;

  iVar3 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (in_ECX & 0xffff) * 0xc);
  puVar4 = *(uint **)(*(int *)(DAT_008603b0 + 0x34) + 8 + (in_EAX & 0xffff) * 0xc);
  sVar11 = *(short *)(iVar3 + 0x2f2);
  iVar9 = -1;
  if (sVar11 != -1) {
    iVar9 = *(int *)(iVar3 + 0x2f8 + sVar11 * 4);
  }
  uVar6 = 1;
  if (iVar9 == -1) {
    return CONCAT22((short)((in_EAX & 0xffff) * 3 >> 0x10),sVar11) & 0xffffff00;
  }
  sVar11 = 0;
  puVar10 = (uint *)(iVar3 + 0x2f8);
  do {
    uVar7 = *puVar10;
    if (uVar7 != 0xffffffff) {
      puVar5 = *(uint **)(*(int *)(DAT_008603b0 + 0x34) + 8 + (uVar7 & 0xffff) * 0xc);
      uVar7 = *puVar4;
      if (uVar7 == *puVar5) {
        if (sVar11 == *(short *)(iVar3 + 0x2f2)) {
          fVar1 = (float)puVar5[0x90];
          uVar8 = (undefined2)(uVar7 >> 0x10);
          uVar7 = CONCAT22(uVar8,(ushort)(fVar1 < 0.0) << 8 | (ushort)NAN(fVar1) << 10 |
                                 (ushort)(fVar1 == 0.0) << 0xe);
          if (fVar1 < 0.0 == 0 && (fVar1 == 0.0) == 0) {
            fVar1 = (float)puVar4[0x90];
            fVar2 = (float)puVar5[0x90];
            uVar7 = CONCAT22(uVar8,(ushort)(fVar1 < fVar2) << 8 |
                                   (ushort)(NAN(fVar1) || NAN(fVar2)) << 10 |
                                   (ushort)(fVar1 == fVar2) << 0xe);
            if (fVar1 < fVar2) goto LAB_0056db83;
          }
        }
        uVar6 = 0;
      }
    }
LAB_0056db83:
    sVar11 = sVar11 + 1;
    puVar10 = puVar10 + 1;
    if (3 < sVar11) {
      return CONCAT31((int3)(uVar7 >> 8),uVar6);
    }
  } while( true );
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
