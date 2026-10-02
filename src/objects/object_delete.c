// object_delete
// address 0x4f5bd0, size 58 bytes
// name confidence: 0.70 (still FUN_004f5bd0 in Ghidra; functions.md: "Top-level object
//   deletion entry point that dispatches to the immediate or recursive deletion path based on
//   the object's parent-role category"; 27 callers, by far the most of any deletion entry in
//   the module, which is what a public `object_delete` looks like. The name is already recorded
//   for this address in symbols/agent_phase4_objects.txt.)
// rewrite confidence: 0.75
// evidence: types/objects.h object_header (data at 0x08, stride 0x0c), object.network_role
//   (0x004, whose own header comment in types/objects.h already says "object_delete dispatches
//   on 0 versus 3"); global 0x008603b0 object_data; callees object_delete_unparented 0x4f5aa0
//   and object_delete_recursive 0x4f59d0.
// register convention: object index in EAX. Confirmed against
//   `objdump -d -M intel bin/halo.exe` 0x4f5bd0..0x4f5c09: the function loads ds:0x008603b0
//   before touching anything else, then `mov edi,eax` / `and eax,0xffff`, so EAX is a true
//   caller-supplied input and EDI is only its saved copy. object_delete_unparented is then
//   called with that copy still in EDI (its own documented input register) and
//   object_delete_recursive is called cdecl with `push 0 / push edi / add esp,8`.
// blam-cc: EAX -> object_index
//
// NOTE for out/phase4/objects_types_notes.md: that file lists this address under
// "object_type_definition ... category at +0x04". It is not the type definition that is read
// here -- the +0x04 dereference is off object_header.data, i.e. the object record itself, so
// the field is object.network_role. objects_delete_unparented_of_type_mask 0x4f47c0 is the
// function that reads a +0x04 off a definition.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern data_array *object_data; // 0x008603b0

extern void object_delete_unparented(uint32_t object_index);            // 0x4f5aa0, EDI -> object_index
extern void object_delete_recursive(uint32_t object_index, uint8_t recurse_siblings); // 0x4f59d0, cdecl

void object_delete(uint32_t object_index) // blam-cc: EAX -> object_index
{
    object *obj = ((object_header *)object_data->data + (object_index & 0xffff))->data;

    // Ghidra shows a redundant `test eax,eax / jne` between the `cmp eax,3` and the recursive
    // call (0x4f5bf4); role is already known to be 3 on that path, so it can never be taken.
    // It is a leftover of the compiler's switch lowering and is not reproduced.
    if (obj->network_role == 0) {
        object_delete_unparented(object_index);
    } else if (obj->network_role != 3) {
        return;
    }
    object_delete_recursive(object_index, 0);
}

#if 0
Original Ghidra decompilation (0x4f5bd0):

void FUN_004f5bd0(void)

{
  int iVar1;
  uint in_EAX;

  iVar1 = *(int *)(*(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (in_EAX & 0xffff) * 0xc) + 4);
  if (iVar1 == 0) {
    FUN_004f5aa0();
  }
  else if (iVar1 != 3) {
    return;
  }
  FUN_004f59d0();
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
