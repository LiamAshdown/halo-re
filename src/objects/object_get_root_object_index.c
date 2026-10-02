// object_get_root_object_index  (Ghidra: object_get_root_object_index, already named)
// address 0x4f6fb0, size 43 bytes
// name confidence: 0.85 (already carries this name from an earlier phase; matches
//   functions.md's summary: "Returns the index of the top-most object in an attachment chain
//   starting from the given object")
// rewrite confidence: 0.85
// evidence: types/objects.h object (parent_object 0x11c); global 0x008603b0 object_data.
// register convention: object index in ECX, returns the root index in EAX. Confirmed against
//   objdump -d -M intel bin/halo.exe: 0x4f6fb3 cmp ecx,0xffffffff at entry, no stack access.
//   // blam-cc: ECX -> object_index

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern data_array *object_data; // 0x008603b0

uint32_t object_get_root_object_index(uint32_t object_index) // blam-cc: ECX -> object_index
{
    uint32_t root = k_datum_index_none;

    if (object_index != k_datum_index_none) {
        uint32_t current = object_index;
        do {
            root = current;
            current = ((object_header *)object_data->data)[root & 0xffff].data->parent_object;
        } while (current != k_datum_index_none);
    }

    return root;
}

#if 0
Original Ghidra decompilation (0x4f6fb0):

uint object_get_root_object_index(void)

{
  uint uVar1;
  uint in_ECX;

  uVar1 = 0xffffffff;
  if (in_ECX != 0xffffffff) {
    do {
      uVar1 = in_ECX;
      in_ECX = *(uint *)(*(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (uVar1 & 0xffff) * 0xc) +
                        0x11c);
    } while (in_ECX != 0xffffffff);
  }
  return uVar1;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
