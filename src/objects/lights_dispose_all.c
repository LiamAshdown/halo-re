// lights_dispose_all
// address 0x4f0aa0, size 76 bytes
// name confidence: 0.55 (out/phase4/objects_functions.md)
// rewrite confidence: 0.5
// evidence: types/objects.h globals list (light_data 0x00860b14, light_cluster_first 0x00860b20,
// light_cluster_references 0x00860b24, light_object_references 0x00860b28, lights_enabled
// 0x0071cfb8, k_maximum_clusters); types/memory.h data_array.valid (0x24).
// register convention: none (no parameters).

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern data_array *light_data;                 // 0x00860b14
extern uint8_t *lights_enabled;                // 0x0071cfb8
extern datum_index *light_cluster_first;        // 0x00860b20
extern data_array *light_cluster_references;    // 0x00860b24
extern data_array *light_object_references;     // 0x00860b28

extern void data_delete_all(data_array *array); // UNSURE: zero visible args at each call site; memory module, 0x4d0580

void lights_dispose_all(void)
{
    int32_t i;

    light_data->valid = 1;
    data_delete_all(light_data); // UNSURE: Ghidra shows no visible arguments

    *lights_enabled = 1;

    for (i = 0; i < k_maximum_clusters; i++) {
        light_cluster_first[i] = (datum_index)0xffffffff;
    }

    light_object_references->valid = 1;
    data_delete_all(light_object_references); // UNSURE: Ghidra shows no visible arguments

    light_cluster_references->valid = 1;
    data_delete_all(light_cluster_references); // UNSURE: Ghidra shows no visible arguments
}

#if 0
Original Ghidra decompilation (0x4f0aa0):

void lights_dispose_all(void)

{
  int iVar1;
  undefined4 *puVar2;

  *(undefined1 *)(DAT_00860b14 + 0x24) = 1;
  data_delete_all();
  *DAT_0071cfb8 = 1;
  puVar2 = DAT_00860b20;
  for (iVar1 = 0x200; iVar1 != 0; iVar1 = iVar1 + -1) {
    *puVar2 = 0xffffffff;
    puVar2 = puVar2 + 1;
  }
  *(undefined1 *)(DAT_00860b28 + 0x24) = 1;
  data_delete_all();
  *(undefined1 *)(DAT_00860b24 + 0x24) = 1;
  data_delete_all();
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
