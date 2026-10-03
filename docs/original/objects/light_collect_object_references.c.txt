// light_collect_object_references
// address 0x4f1700, size 125 bytes
// name confidence: 0.3 (still FUN_004f1700 in Ghidra; named from out/phase4/objects_functions.md's
// summary, "Collects up to N light-datum indices from a hash-chain into a caller-provided array")
// rewrite confidence: 0.4
// evidence: types/objects.h light.next_light (0x10), light_data, light_object_references
// (data_array of object_cluster_reference-shaped 0x0c entries per the objects.h globals note).
// register convention: uint32_t light_handle in ECX (in_ECX); int16_t max_count in SI
// (unaff_SI); int16_t *out_buffer in EDI (unaff_EDI).
// blam-cc: ECX=light_handle, SI=max_count, EDI=out_buffer

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern data_array *light_data;              // 0x00860b14
extern data_array *light_object_references; // 0x00860b28

// FIXED (objdump 0x4f1700..0x4f177c): returns the count in AX (lights_apply_spot_falloff reads it at 0x4f1850).
int16_t light_collect_object_references(uint32_t light_handle, int16_t max_count, int16_t *out_buffer)
{
    light *l = &((light *)light_data->data)[light_handle & 0xffff];
    uint32_t node = (uint32_t)l->next_light;
    uint32_t next_node;
    int16_t count = 0;

    if (node == 0xffffffff) {
        next_node = 0xffffffff;
        node = 0xffffffff;
    } else {
        uint8_t *ref = (uint8_t *)light_object_references->data + (node & 0xffff) * 0xc;
        next_node = *(uint32_t *)(ref + 8);
        node = *(uint32_t *)(ref + 4);
    }

    if (0 < max_count) {
        do {
            if ((int16_t)node == -1) {
                return count;
            }
            out_buffer[count] = (int16_t)node;
            count = count + 1;

            if (next_node == 0xffffffff) {
                node = 0xffffffff;
            } else {
                uint8_t *ref = (uint8_t *)light_object_references->data + (next_node & 0xffff) * 0xc;
                next_node = *(uint32_t *)(ref + 8);
                node = *(uint32_t *)(ref + 4);
            }
        } while (count < max_count);
    }
    return count;
}

#if 0
Original Ghidra decompilation (0x4f1700):

void FUN_004f1700(void)

{
  short sVar1;
  uint in_ECX;
  uint uVar2;
  undefined4 uVar3;
  int iVar4;
  short unaff_SI;
  int unaff_EDI;

  uVar2 = *(uint *)((in_ECX & 0xffff) * 0x7c + 0x10 + *(int *)(DAT_00860b14 + 0x34));
  sVar1 = 0;
  if (uVar2 == 0xffffffff) {
    uVar3 = 0xffffffff;
    uVar2 = 0xffffffff;
  }
  else {
    iVar4 = *(int *)(DAT_00860b28 + 0x34) + (uVar2 & 0xffff) * 0xc;
    uVar2 = *(uint *)(iVar4 + 8);
    uVar3 = *(undefined4 *)(iVar4 + 4);
  }
  if (0 < unaff_SI) {
    do {
      if ((short)uVar3 == -1) {
        return;
      }
      iVar4 = (int)sVar1;
      sVar1 = sVar1 + 1;
      *(short *)(unaff_EDI + iVar4 * 2) = (short)uVar3;
      if (uVar2 == 0xffffffff) {
        uVar3 = 0xffffffff;
      }
      else {
        iVar4 = *(int *)(DAT_00860b28 + 0x34) + (uVar2 & 0xffff) * 0xc;
        uVar2 = *(uint *)(iVar4 + 8);
        uVar3 = *(undefined4 *)(iVar4 + 4);
      }
    } while (sVar1 < unaff_SI);
  }
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
