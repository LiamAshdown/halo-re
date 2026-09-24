// object_clear_references_to_object  (named by out/phase4/objects_types_notes.md: "0x0f0 |
// object_clear_references_to_object 0x4f73e0 clears any object whose 0xf0 points at the dying
// object, notifying another subsystem for each object visited")
// address 0x4f73e0, size 112 bytes
// name confidence: 0.85 (fixed by the types notes' own citation of this address by this name)
// rewrite confidence: 0.45
// evidence: types/objects.h object (damage_owner 0x0f0), object_iterator; callees
//   object_iterator_next (0x4f6f20, established), object_type_definitions_notify_0x3c
//   (0x4f40f0, outside this batch).
// register convention: the dying object index is the sole, genuinely-stack, parameter (Ghidra's
//   own "FUN_004f73e0(int param_1)"), confirmed against objdump 0x4f7415 mov esi,[esp+0x20].
// UNSURE: 0x4f40f0 is called here with BOTH the iterated object's handle in EBX (matching that
//   file's own declared convention) AND the dying object index pushed on the stack
//   (0x4f7432 push esi; call 0x4f40f0, with no stack cleanup after the call, implying the
//   callee pops its own argument). src/objects/object_type_definitions_notify_0x3c.c (written
//   outside this batch) declares only the EBX parameter and is not modified here; this file's
//   own extern below adds the stack argument that this call site actually passes.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"

extern object *object_iterator_next(object_iterator *iterator); // 0x4f6f20, this batch
extern void object_type_definitions_notify_0x3c(uint32_t object_index, uint32_t dying_object_index);
    // 0x4f40f0, outside this batch; see UNSURE above about the second, stack-passed argument

void object_clear_references_to_object(uint32_t dying_object_index)
{
    object_iterator iterator;
    object *obj;

    iterator.type_mask = 0xffffffff;
    iterator.flags_mask = 0;
    iterator.index = 0;
    iterator.handle = k_datum_index_none;

    obj = object_iterator_next(&iterator);
    while (obj != (object *)0) {
        if (obj->damage_owner == dying_object_index) {
            obj->damage_owner = k_datum_index_none;
        }
        object_type_definitions_notify_0x3c(iterator.handle, dying_object_index);
        obj = object_iterator_next(&iterator);
    }
}

#if 0
Original Ghidra decompilation (0x4f73e0):

void FUN_004f73e0(int param_1)

{
  int iVar1;
  undefined4 local_10;
  undefined1 local_c;
  undefined2 local_a;
  undefined4 local_8;
  undefined4 local_4;

  local_4 = 0x86868686;
  local_10 = 0xffffffff;
  local_c = 0;
  local_a = 0;
  local_8 = 0xffffffff;
  iVar1 = object_iterator_next(&local_10);
  while (iVar1 != 0) {
    if (*(int *)(iVar1 + 0xf0) == param_1) {
      *(undefined4 *)(iVar1 + 0xf0) = 0xffffffff;
    }
    FUN_004f40f0(param_1);
    iVar1 = object_iterator_next(&local_10);
  }
  return;
}
#endif
