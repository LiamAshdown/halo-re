// objects_get_ambient_cluster  (named by types/objects.h's own struct comment:
// "object_globals.ambient_cluster_mode, set by objects_set_ambient_cluster_override and read
// by objects_get_ambient_cluster")
// address 0x4f7a50, size 125 bytes
// name confidence: 0.85 (fixed by types/objects.h's own citation of this address by this name)
// rewrite confidence: 0.55
// evidence: types/objects.h object_globals (ambient_cluster_mode 0x90, ambient_cluster_index
//   0x94), object (flags 0x10 with _object_needs_cluster_update_bit, location_cluster_index
//   0x09c); global 0x006b8cbc object_globals_pointer, global 0x008603b0 object_data; callees
//   object_try_and_get (0x4f6ec0, established: ECX -> object_index, stack -> type_mask),
//   object_get_root_object_index (0x4f6fb0, established: ECX -> object_index).
// register convention: no parameters. Confirmed against objdump -d -M intel bin/halo.exe: the
//   whole function reads only object_globals_pointer before branching on ambient_cluster_mode.
// UNSURE: in _object_ambient_cluster_from_tracked_object mode, ambient_cluster_index (an
//   int16_t field) is reused here as the LOW 16 BITS of a datum_index object handle (the high
//   salt half is implicitly zero from the field's own zero-extension); this matches
//   objects_set_ambient_cluster_override never writing that mode/field pair together, so no
//   caller in this module actually exercises this path with a real salt.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "fn_objects.h"

extern object_globals *object_globals_pointer; // 0x006b8cbc
extern data_array *object_data; // 0x008603b0

extern object *object_try_and_get(datum_index object_index, uint32_t type_mask); // 0x4f6ec0
extern uint32_t object_get_root_object_index(uint32_t object_index); // 0x4f6fb0

int16_t objects_get_ambient_cluster(void)
{
    if (object_globals_pointer->ambient_cluster_mode == _object_ambient_cluster_from_tracked_object) {
        datum_index handle = (uint16_t)object_globals_pointer->ambient_cluster_index;
        if (object_try_and_get(handle, 0xffffffff) == 0) {
            object_globals_pointer->ambient_cluster_mode = _object_ambient_cluster_none;
        } else {
            uint32_t root_index = object_get_root_object_index(handle);
            object *root = ((object_header *)object_data->data)[root_index & 0xffff].data;
            if ((root->flags & _object_needs_cluster_update_bit) != 0) {
                if (root->location_cluster_index != -1) {
                    return root->location_cluster_index;
                }
            }
        }
        return -1;
    }

    if (object_globals_pointer->ambient_cluster_mode == _object_ambient_cluster_override) {
        return object_globals_pointer->ambient_cluster_index;
    }

    return -1;
}

#if 0
Original Ghidra decompilation (0x4f7a50):

uint FUN_004f7a50(void)

{
  short *psVar1;
  int iVar2;
  uint uVar3;
  int iVar4;

  iVar2 = DAT_006b8cbc;
  if (*(short *)(DAT_006b8cbc + 0x90) == 1) {
    iVar4 = object_try_and_get(0xffffffff);
    if (iVar4 == 0) {
      *(undefined2 *)(iVar2 + 0x90) = 0;
      uVar3 = 0;
    }
    else {
      uVar3 = object_get_root_object_index();
      uVar3 = *(uint *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (uVar3 & 0xffff) * 0xc);
      if (((*(uint *)(uVar3 + 0x10) & 0x800) != 0) &&
         (psVar1 = (short *)(uVar3 + 0x9c), uVar3 = CONCAT22((short)(uVar3 >> 0x10),*psVar1),
         *psVar1 != -1)) {
        return uVar3;
      }
    }
  }
  else {
    uVar3 = (int)*(short *)(DAT_006b8cbc + 0x90) - 2;
    if (uVar3 == 0) {
      return (uint)*(ushort *)(DAT_006b8cbc + 0x94);
    }
  }
  return CONCAT22((short)(uVar3 >> 0x10),0xffff);
}
#endif
