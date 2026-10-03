// object_reserve_render_cache_slot  (named by types/objects.h's own object.render_cache_slot
// comment: "0x0ba render cache slot | object_reserve_render_cache_slot 0x4f9ac0,
// object_release_render_cache_slot 0x4f9b00")
// address 0x4f9ac0, size 54 bytes
// name confidence: 0.85 (fixed by the types notes' own citation of this address by this name)
// rewrite confidence: 0.9 (VERIFIED against objdump)
// evidence: types/objects.h object (render_cache_slot 0x0ba, k_maximum_object_names 0x200);
//   global 0x008603b0 object_data, global 0x006b8cb8 object_name_list (reused here as a
//   render-cache slot table).
// register convention: object index in EDX, slot index in CX. Confirmed against objdump
//   -d -M intel bin/halo.exe: 0x4f9aca mov eax,edx at entry, 0x4f9adf movsx eax,cx.
//   // blam-cc: EDX -> object_index, CX -> slot

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern data_array *object_data; // 0x008603b0
extern datum_index *object_name_list; // 0x006b8cb8, reused here as a render-cache slot table

void object_reserve_render_cache_slot(uint32_t object_index, int16_t slot) // blam-cc: EDX -> object_index, CX -> slot
{
    object *obj = ((object_header *)object_data->data)[object_index & 0xffff].data;

    if (object_name_list[slot] == k_datum_index_none) {
        object_name_list[slot] = object_index;
        obj->render_cache_slot = slot;
    }
}

#if 0
Original Ghidra decompilation (0x4f9ac0):

void FUN_004f9ac0(void)

{
  uint *puVar1;
  int iVar2;
  short in_CX;
  uint in_EDX;

  iVar2 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (in_EDX & 0xffff) * 0xc);
  puVar1 = (uint *)(DAT_006b8cb8 + in_CX * 4);
  if (*puVar1 == 0xffffffff) {
    *puVar1 = in_EDX;
    *(short *)(iVar2 + 0xba) = in_CX;
  }
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
