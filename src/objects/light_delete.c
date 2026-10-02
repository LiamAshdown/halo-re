// light_delete
// address 0x4f0bd0, size 51 bytes
// name confidence: 0.35 (still FUN_004f0bd0 in Ghidra; named from out/phase4/objects_functions.md's
// summary, "Deregisters and frees a light datum, undoing light_new_attached/light_new_positioned's
// allocation")
// rewrite confidence: 0.75
// evidence: resolved from the disassembly at 0x4f0bd0 (objdump -d -M intel bin/halo.exe).
//   ESI holds the light handle and is never assigned, so it is the incoming argument;
//   0x4f0bd2 and esi,0xffff / imul eax,eax,0x7c indexes the 0x7c-stride light table, and
//   0x4f0be5 lea edx,[eax+ecx+0x10] is &light->next_light. EBX is loaded with 0x00860b20,
//   the per-cluster light list descriptor. The trailing datum_delete gets the light
//   data_array in EAX and the handle in EDX (0x4f0bf8 mov eax,edi / mov edx,esi).
// register convention: light handle in ESI.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern data_array *light_data; // 0x00860b14
extern datum_index *light_cluster_first; // 0x00860b20, the per-cluster list descriptor
extern void cluster_reference_remove_all(uint32_t handle, datum_index *link, void *cluster_list);
    // 0x552020; handle and link on the stack, the list descriptor in EBX. Unlinks one entry
    // from its cluster list.
extern void datum_delete(data_array *array, datum_index handle); // memory module, 0x4d0510

void light_delete(datum_index light_handle) // blam-cc: ESI -> light_handle
{
    light *entry = (light *)((uint8_t *)light_data->data + (light_handle & 0xffff) * 0x7c);

    cluster_reference_remove_all(light_handle, &entry->next_light, &light_cluster_first);
    datum_delete(light_data, light_handle);
}

#if 0
Original Ghidra decompilation (0x4f0bd0):

void FUN_004f0bd0(void)

{
  FUN_00552020();
  datum_delete();
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
