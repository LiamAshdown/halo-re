// hs_object_list_test_trigger_volume  (Ghidra: FUN_00487820)
// address 0x487820, size 207 bytes
// name confidence: 0.5   rewrite confidence: 0.9
// REWRITTEN (objdump 0x487820..0x4878ee; the draft handed the object index to the containment test). ECX = the
//   object list, [esp+4] trigger volume, [esp+8] all_mode. Walks the list (header +0x08 first reference;
//   reference +0x04 object, +0x08 next) testing each object's centre (object +0xa0) with
//   scenario_trigger_volume_contains_point (EAX volume, ECX point): "any" returns 1 on the first inside, "all"
//   returns 0 on the first outside; an empty or exhausted list returns all_mode.
// blam-cc: ECX -> header_index, stack -> trigger_volume_index, all_mode

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "hs.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern uint8_t scenario_trigger_volume_contains_point(int16_t trigger_volume_index, real_point3d *point);
    // 0x53f020, blam-cc: EAX, ECX

extern data_array *object_list_header_data;    // 0x0087a464
extern data_array *object_list_reference_data; // 0x0087a468
extern data_array *object_data;                // 0x008603b0

char hs_object_list_test_trigger_volume(int32_t trigger_volume_index, datum_index header_index, char all_mode)
{
    datum_index object_index = k_datum_index_none;
    datum_index next = k_datum_index_none;

    if (header_index != k_datum_index_none) {
        datum_index first = *(datum_index *)((uint8_t *)object_list_header_data->data + (header_index & 0xffff) * 0xc + 8);

        if (first != k_datum_index_none) {
            uint8_t *reference = (uint8_t *)object_list_reference_data->data + (first & 0xffff) * 0xc;

            next = *(datum_index *)(reference + 8);
            object_index = *(datum_index *)(reference + 4);
        }
    }
    while (object_index != k_datum_index_none) {
        uint8_t *object = *(uint8_t **)((uint8_t *)object_data->data + (object_index & 0xffff) * 0xc + 8);

        if (scenario_trigger_volume_contains_point((int16_t)trigger_volume_index, (real_point3d *)(object + 0xa0))) {
            if (!all_mode) {
                return 1;
            }
        } else if (all_mode) {
            return 0;
        }
        if (next != k_datum_index_none) {
            uint8_t *reference = (uint8_t *)object_list_reference_data->data + (next & 0xffff) * 0xc;

            next = *(datum_index *)(reference + 8);
            object_index = *(datum_index *)(reference + 4);
        } else {
            object_index = k_datum_index_none;
        }
    }
    return all_mode;
}

#if 0
Original Ghidra decompilation (0x487820):

undefined4 FUN_00487820(undefined4 param_1,uint param_2)

{
  int iVar1;
  char cVar2;
  int iVar3;
  uint uVar4;
  uint uVar5;
  uint in_ECX;

  iVar1 = DAT_0087a468;
  iVar3 = -1;
  uVar4 = param_2;
  if (in_ECX != 0xffffffff) {
    uVar4 = *(uint *)(*(int *)(DAT_0087a464 + 0x34) + 8 + (in_ECX & 0xffff) * 0xc);
    if (uVar4 == 0xffffffff) {
      iVar3 = -1;
      uVar4 = 0xffffffff;
    }
    else {
      uVar4 = uVar4 & 0xffff;
      iVar3 = *(int *)(*(int *)(DAT_0087a468 + 0x34) + uVar4 * 0xc + 4);
      uVar4 = *(uint *)(*(int *)(DAT_0087a468 + 0x34) + 8 + uVar4 * 0xc);
    }
  }
  if (iVar3 == -1) {
    return CONCAT31(0xffffff,(char)param_2);
  }
  do {
    cVar2 = scenario_trigger_volume_contains_point();
    if (cVar2 == '\0') {
      if ((char)param_2 != '\0') {
        return 0;
      }
    }
    else if ((char)param_2 == '\0') {
      return 1;
    }
    if (uVar4 == 0xffffffff) {
      iVar3 = -1;
    }
    else {
      uVar5 = uVar4 & 0xffff;
      uVar4 = *(uint *)(*(int *)(iVar1 + 0x34) + 8 + uVar5 * 0xc);
      iVar3 = *(int *)(*(int *)(iVar1 + 0x34) + uVar5 * 0xc + 4);
    }
  } while (iVar3 != -1);
  return CONCAT31(0xffffff,(char)param_2);
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
