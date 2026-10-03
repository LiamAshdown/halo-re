// object_get_root_location  (Ghidra: FUN_004f6b10; renamed, Blam-style, not previously named)
// address 0x4f6b10, size 81 bytes
// name confidence: 0.4 (matches functions.md's summary: "Finds the root of an object's
//   attachment chain and copies two unidentified 32-bit fields from it"; the two fields are
//   object.location_leaf_index and location_cluster_index, the companion pair
//   object_set_cluster_and_parent writes)
// rewrite confidence: 0.65
// evidence: types/objects.h object (parent_object 0x11c, location_leaf_index 0x098,
//   location_cluster_index 0x09c); global 0x008603b0 object_data.
// register convention: out pointer in EAX, object index in ECX. Confirmed against
//   objdump -d -M intel bin/halo.exe: 0x4f6b54 mov [eax],ecx writes the result, ecx is masked
//   as the walk index throughout.
//   // blam-cc: EAX -> out, ECX -> object_index
// UNSURE: the second dword written (object+0x9c) straddles the documented int16
//   location_cluster_index and the undocumented unknown_09e that immediately follows it,
//   copied here as one raw 32-bit read exactly like the compiled code.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern data_array *object_data; // 0x008603b0

void object_get_root_location(int32_t *out, uint32_t object_index) // blam-cc: EAX -> out, ECX -> object_index
{
    // Mirrors the compiled control flow exactly: when object_index is already
    // k_datum_index_none, the walk never runs and root_index stays k_datum_index_none, so the
    // final lookup below reads object_header[0xffff] (an out-of-range slot) just like the
    // original -- restructuring this into a plain "start at object_index" loop would add an
    // extra ->parent_object dereference for that case that the compiled code never performs.
    uint32_t root_index = k_datum_index_none;
    if (object_index != k_datum_index_none) {
        uint32_t current = object_index;
        do {
            root_index = current;
            current = ((object_header *)object_data->data)[root_index & 0xffff].data->parent_object;
        } while (current != k_datum_index_none);
    }

    {
        object *root = ((object_header *)object_data->data)[root_index & 0xffff].data;
        out[0] = root->location_leaf_index;
        out[1] = *(int32_t *)&root->location_cluster_index; // location_cluster_index + unknown_09e
    }
}

#if 0
Original Ghidra decompilation (0x4f6b10):

void FUN_004f6b10(void)

{
  int iVar1;
  undefined4 *in_EAX;
  uint in_ECX;
  uint uVar2;

  uVar2 = 0xffffffff;
  if (in_ECX != 0xffffffff) {
    do {
      uVar2 = in_ECX;
      in_ECX = *(uint *)(*(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (uVar2 & 0xffff) * 0xc) +
                        0x11c);
    } while (in_ECX != 0xffffffff);
  }
  iVar1 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (uVar2 & 0xffff) * 0xc);
  *in_EAX = *(undefined4 *)(iVar1 + 0x98);
  in_EAX[1] = *(undefined4 *)(iVar1 + 0x9c);
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
