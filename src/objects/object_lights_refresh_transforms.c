// object_lights_refresh_transforms
// address 0x4f2d50, size 157 bytes
// name confidence: 0.7 (still FUN_004f2d50 in Ghidra; types/objects.h's
//   _light_transform_dirty_bit comment names this function directly: "object_lights_refresh_
//   transforms recomputes the transform and clears it"; matches functions.md's summary)
// rewrite confidence: 0.6
// evidence: types/objects.h light (stride 0x7c, flags at 0x02, _light_transform_dirty_bit
//   0x0004); global 0x00860b14 light_data; callee FUN_004f2a00 (0x4f2a00, this batch). The
//   inlined datum-iteration tail is restored to a real datum_next call, following the precedent
//   in src/hs/hs_object_list_collect_player_units.c and objects_flush_dirty_state.c (this
//   batch), whose evidence establishes that this exact tail is datum_next's body re-inlined.
// register convention: none (void).

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern data_array *light_data; // 0x00860b14

extern datum_index datum_next(int16_t after_index, data_array *array); // 0x4d0630, memory module
extern void object_light_recompute_transform(uint32_t light_index); // 0x4f2a00, this batch

void object_lights_refresh_transforms(void)
{
    datum_index index = datum_next(-1, light_data);

    while (index != k_datum_index_none) {
        light *entry = (light *)light_data->data + (index & 0xffff);
        if ((entry->flags & _light_transform_dirty_bit) != 0) {
            entry->flags &= (uint16_t)~_light_transform_dirty_bit;
            object_light_recompute_transform(index);
        }
        index = datum_next((int16_t)index, light_data);
    }
}

#if 0
Original Ghidra decompilation (0x4f2d50):

void FUN_004f2d50(void)

{
  ushort uVar1;
  uint uVar2;
  int iVar3;
  short *psVar4;
  short sVar5;
  int iVar6;

  iVar6 = DAT_00860b14;
  uVar2 = FUN_004d0630();
  do {
    do {
      if (uVar2 == 0xffffffff) {
        return;
      }
      iVar3 = (uVar2 & 0xffff) * 0x7c + *(int *)(iVar6 + 0x34);
      uVar1 = *(ushort *)(iVar3 + 2);
      if ((uVar1 & 4) != 0) {
        *(ushort *)(iVar3 + 2) = uVar1 & 0xfffb;
        FUN_004f2a00(uVar2);
        iVar6 = DAT_00860b14;
      }
      iVar3 = uVar2 + 1;
      uVar2 = 0xffffffff;
      sVar5 = (short)iVar3;
    } while ((sVar5 < 0) || (*(short *)(iVar6 + 0x2e) <= sVar5));
    psVar4 = (short *)((int)sVar5 * (int)*(short *)(iVar6 + 0x22) + *(int *)(iVar6 + 0x34));
    do {
      if (*psVar4 != 0) {
        uVar2 = (int)*psVar4 << 0x10 | (int)(short)iVar3;
        break;
      }
      iVar3 = iVar3 + 1;
      psVar4 = (short *)((int)psVar4 + (int)*(short *)(iVar6 + 0x22));
    } while ((short)iVar3 < *(short *)(iVar6 + 0x2e));
  } while( true );
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
