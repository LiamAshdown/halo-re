// object_type_override_call_0x70_release_node
// address 0x4f4680, size 46 bytes
// name confidence: 0.3 (still FUN_004f4680 in Ghidra; functions.md's summary -- "Releases one
//   node back to a pooled free-list before dispatching the +0x70 object-type override for
//   param_1" -- is conf=0.25 and not corroborated by types/objects.h, which does not document
//   0x00687130 or the +0x44/+0x28 fields touched here)
// rewrite confidence: 0.4
// evidence: none beyond the raw decompilation; 0x00687130 and 0x006870d0 are not object-module
//   globals documented in types/objects.h, so this is likely a thin caller-supplied wrapper
//   around some other subsystem's node table.
// UNSURE: the exact struct at in_EAX and the table at 0x00687130 are unidentified; every offset
//   below is kept as a raw byte offset rather than a named field. UNSURE: object_type_override_
//   call_0x70 (0x4f4620) takes no parameters in this batch's rewrite (it works off the "current
//   object" global), so param_1 is passed through here but has no visible effect; preserved
//   exactly as the original does.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern network_id_table *object_network_id_table; // 0x00687130
extern void object_type_override_call_0x70(uint32_t object_index, uint32_t edi_argument,
    uint32_t stack_argument); // this module, 0x4f4620; UNSURE: none of the three are visible here // 0x4f4620

// FIXED (register inputs, objdump): the original never reads ECX as an input (it overwrites or only saves it); those parameters arrive on the stack (1 stack argument(s) read).
// blam-cc: EAX -> record, stack -> param_1
void object_type_override_call_0x70_release_node(int32_t *record, uint32_t client)
{
    int32_t **slot = (int32_t **)((uint8_t *)record + 0x44);
    int32_t node = **slot;
    int32_t next = -1;

    if (node != 0) {
        next = ((int32_t *)object_network_id_table->handles)[node];
    }
    **slot = next;
    object_type_override_call_0x70(0, 0, 0); // UNSURE: original passes param_1 through, unused by the callee
}

#if 0
Original Ghidra decompilation (0x4f4680):

void FUN_004f4680(undefined4 param_1)

{
  int iVar1;
  int in_EAX;
  int iVar2;

  iVar1 = **(int **)(in_EAX + 0x44);
  iVar2 = -1;
  if (iVar1 != 0) {
    iVar2 = *(int *)(*(int *)(PTR_DAT_00687130 + 0x28) + iVar1 * 4);
  }
  **(int **)(in_EAX + 0x44) = iVar2;
  FUN_004f4620(param_1);
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
