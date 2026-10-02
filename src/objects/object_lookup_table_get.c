// object_lookup_table_get  (named by types/objects.h's own global list: "0x006b8cb8
// datum_index *object_name_list // 0x200 entries, object_lookup_table_get")
// address 0x4f73c0, size 28 bytes
// name confidence: 0.8 (fixed by types/objects.h's own citation of this address by this name)
// rewrite confidence: 0.85
// evidence: types/objects.h k_maximum_object_names (0x200), global 0x006b8cb8 object_name_list.
// register convention: index in AX. Confirmed against objdump -d -M intel bin/halo.exe:
//   0x4f73c5 cmp ax,0x200 at entry, no stack access.
//   // blam-cc: AX -> name_index

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern datum_index *object_name_list; // 0x006b8cb8

datum_index object_lookup_table_get(int16_t name_index) // blam-cc: AX -> name_index
{
    if (name_index >= 0 && name_index < k_maximum_object_names) {
        return object_name_list[name_index];
    }
    return k_datum_index_none;
}

#if 0
Original Ghidra decompilation (0x4f73c0):

undefined4 FUN_004f73c0(void)

{
  short in_AX;

  if ((-1 < in_AX) && (in_AX < 0x200)) {
    return *(undefined4 *)(DAT_006b8cb8 + in_AX * 4);
  }
  return 0xffffffff;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
