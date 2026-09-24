// object_type_override_get_0x64
// address 0x4f44f0, size 97 bytes
// name confidence: 0.6 (still FUN_004f44f0 in Ghidra; named from types/objects.h's
//   object_type_definition.override_get_64 field comment, "0x64
//   object_type_override_get_0x64")
// rewrite confidence: 0.7
// evidence: types/objects.h object_type_definition (subdefinitions[16] at 0x80, override_get_64
//   at 0x64); global 0x0069bfdc object_type_definitions[12]; callee object_try_and_get 0x4f6ec0.
// register convention (read off the disassembly at 0x4f44f0): object handle in ESI, plus TWO
//   stack arguments that Ghidra did not model -- 0x4f4540 mov ecx,[esp+0x10] and
//   0x4f4544 mov edx,[esp+0xc] reload stack slots 2 and 1 and push them, with ESI, as the
//   three cdecl arguments of the type's 0x64 vtable slot (0x4f454b call [eax+0x64],
//   0x4f454e add esp,0xc). The single caller in this module passes the 0x00871de0 scratch
//   buffer and its 0x7ff8-byte size, so this is a "serialize the object into a buffer"
//   query that returns the number of bytes/records written (0 when no subdefinition
//   implements it -- 0x4f453c mov eax,edi with edi zeroed at 0x4f44f5).

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"

extern object *object_try_and_get(datum_index object_index, uint32_t type_mask);
    // 0x4f6ec0; object handle in ECX, type mask on the stack. Verified against the body at
    // 0x4f6ec0 (cmp ecx,-1 / test cx,cx / and param_1 & 1 << header->type) and against the
    // call site in this file.
extern object_type_definition *object_type_definitions[k_maximum_object_types]; // 0x0069bfdc

int object_type_override_get_0x64(uint32_t object_index, void *buffer, int32_t buffer_size)
    // blam-cc: ESI -> object_index; stack -> buffer, buffer_size
{
    object *obj = object_try_and_get(object_index, _object_mask_all);
    object_type_definition *def;
    int16_t i;

    if (obj == 0) {
        return 0;
    }
    def = object_type_definitions[obj->type];
    for (i = k_maximum_object_subdefinitions - 1; i >= 0; i--) {
        object_type_definition *sub = def->subdefinitions[i];
        if (sub != 0 && sub->override_get_64 != 0) {
            return ((int (*)(uint32_t, void *, int32_t))sub->override_get_64)(
                object_index, buffer, buffer_size);
        }
    }
    return 0;
}

#if 0
Original Ghidra decompilation (0x4f44f0):

undefined4 FUN_004f44f0(void)

{
  int iVar1;
  int iVar2;
  undefined4 uVar3;
  short sVar4;

  iVar2 = object_try_and_get(0xffffffff);
  if (iVar2 == 0) {
    return 0;
  }
  sVar4 = 0xf;
  while ((iVar1 = *(int *)((&PTR_PTR_0069bfdc)[*(short *)(iVar2 + 0xb4)] + sVar4 * 4 + 0x80),
         iVar1 == 0 || (*(int *)(iVar1 + 100) == 0))) {
    sVar4 = sVar4 + -1;
    if (sVar4 < 0) {
      return 0;
    }
  }
  uVar3 = (**(code **)(iVar1 + 100))();
  return uVar3;
}
#endif
