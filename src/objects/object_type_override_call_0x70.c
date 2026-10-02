// object_type_override_call_0x70
// address 0x4f4620, size 87 bytes
// name confidence: 0.6 (still FUN_004f4620 in Ghidra; named from types/objects.h's
//   object_type_definition.override_call_70 field comment)
// rewrite confidence: 0.65
// evidence: same as object_type_override_get_0x64; callee message_delta_decode_compound_field_staged is outside this batch's
//   address range and is left under its Ghidra name.
// register convention (read off the disassembly at 0x4f4620): object handle in ESI, a second
//   value in EDI (never assigned in the body, and moved into EAX for the fallback tail call at
//   0x4f4670 mov eax,edi / jmp 0x4ec670), plus ONE stack argument reloaded at
//   0x4f4661 mov ecx,[esp+0x8]. All three are pushed for the vtable slot
//   (0x4f4665 push ecx / push edi / push esi / call [eax+0x70] / add esp,0xc).
// note: the no-current-object path is a TAIL CALL, not a plain call, and the loop-exhausted
//   path returns WITHOUT taking it -- the two are not interchangeable.
// UNSURE: message_delta_decode_compound_field_staged's own signature/behavior was not examined, and this function has no
//   caller anywhere in the image, so the meaning of the EDI and stack values is unknown.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern object *object_try_and_get(datum_index object_index, uint32_t type_mask);
    // 0x4f6ec0; object handle in ECX, type mask on the stack. Verified against the body at
    // 0x4f6ec0 (cmp ecx,-1 / test cx,cx / and param_1 & 1 << header->type) and against the
    // call site in this file.
extern object_type_definition *object_type_definitions[k_maximum_object_types]; // 0x0069bfdc
extern uint8_t message_delta_decode_compound_field_staged(void **context); // 0x4ec670, value in EAX; tail-called when there is no current object

void object_type_override_call_0x70(uint32_t object_index, uint32_t edi_argument,
                                    uint32_t stack_argument)
    // blam-cc: ESI -> object_index, EDI -> edi_argument; stack -> stack_argument
{
    object *obj = object_try_and_get(object_index, _object_mask_all);
    object_type_definition *def;
    int16_t i;

    if (obj == 0) {
        message_delta_decode_compound_field_staged((void **)edi_argument); // tail call
        return;
    }
    def = object_type_definitions[obj->type];
    for (i = k_maximum_object_subdefinitions - 1; i >= 0; i--) {
        object_type_definition *sub = def->subdefinitions[i];
        if (sub != 0 && sub->override_call_70 != 0) {
            ((void (*)(uint32_t, uint32_t, uint32_t))sub->override_call_70)(
                object_index, edi_argument, stack_argument);
            return;
        }
    }
}

#if 0
Original Ghidra decompilation (0x4f4620):

void FUN_004f4620(void)

{
  int iVar1;
  int iVar2;
  short sVar3;

  iVar2 = object_try_and_get(0xffffffff);
  if (iVar2 == 0) {
    FUN_004ec670();
    return;
  }
  sVar3 = 0xf;
  while ((iVar1 = *(int *)((&PTR_PTR_0069bfdc)[*(short *)(iVar2 + 0xb4)] + sVar3 * 4 + 0x80),
         iVar1 == 0 || (*(int *)(iVar1 + 0x70) == 0))) {
    sVar3 = sVar3 + -1;
    if (sVar3 < 0) {
      return;
    }
  }
  (**(code **)(iVar1 + 0x70))();
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
