// damage_data_initialize
// address 0x4ed990, size 52 bytes
// name confidence: 0.6 (Ghidra-recovered name, matches out/phase4/objects_types_notes.md's
// "damage_data_initialize 0x4ed990 zeroes 0x15 dwords and writes the sentinels...")
// rewrite confidence: 0.9
// evidence: types/objects.h damage_data (0x54 bytes / 0x15 dwords, sentinel fields at 0x08,
// 0x0c, 0x10, 0x18, 0x4c and the 1.0 multipliers at 0x40/0x44).
// register convention: damage_data *dd in EDX (in_EDX); datum_index damage_effect_tag on the
// stack (param_1).
// blam-cc: EDX=dd, stack=damage_effect_tag
// reconciled: R25 damage_data.unknown_4c -> material_type (int16 collision material of the damaged surface, 0xffff = none; indexes DamageEffect +0x200)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

void damage_data_initialize(damage_data *dd, datum_index damage_effect_tag)
{
    uint32_t *words = (uint32_t *)dd;
    int i;

    for (i = 0; i < 0x15; i++) {
        words[i] = 0;
    }

    dd->damage_effect_tag = damage_effect_tag;
    dd->material_type = (int16_t)0xffff;
    dd->responsible_player = (datum_index)0xffffffff;
    dd->responsible_object = (datum_index)0xffffffff;
    dd->team_index = (int16_t)0xffff;
    dd->location_cluster_index = (int16_t)0xffff;
    dd->random_blend = 1.0f;
    dd->multiplier = 1.0f;
}

#if 0
Original Ghidra decompilation (0x4ed990):

void damage_data_initialize(undefined4 param_1)

{
  int iVar1;
  undefined4 *in_EDX;
  undefined4 *puVar2;

  puVar2 = in_EDX;
  for (iVar1 = 0x15; iVar1 != 0; iVar1 = iVar1 + -1) {
    *puVar2 = 0;
    puVar2 = puVar2 + 1;
  }
  *in_EDX = param_1;
  *(undefined2 *)(in_EDX + 0x13) = 0xffff;
  in_EDX[2] = 0xffffffff;
  in_EDX[3] = 0xffffffff;
  *(undefined2 *)(in_EDX + 4) = 0xffff;
  *(undefined2 *)(in_EDX + 6) = 0xffff;
  in_EDX[0x10] = 0x3f800000;
  in_EDX[0x11] = 0x3f800000;
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
