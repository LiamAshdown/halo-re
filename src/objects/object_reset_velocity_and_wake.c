// object_reset_velocity_and_wake
// address 0x4f5160, size 92 bytes
// name confidence: 0.4 (still FUN_004f5160 in Ghidra; types/objects.h's
//   object_type_definition.notify_reset_scale field comment guesses "object_reset_default_
//   scale_and_color" for this caller, but the fields actually touched here -- object+0x68/0x6c/
//   0x70 and object+0x8c/0x90/0x94 -- land exactly on the documented velocity and
//   angular_velocity real_vector3d fields, not on scale or change_colors; renamed accordingly)
// rewrite confidence: 0.6
// evidence: types/objects.h object (velocity at 0x068, angular_velocity at 0x08c, flags at
//   0x10 with _object_at_rest_bit); global 0x00696714 "the shared constant vectors" (a single
//   real_vector3d the two fields are both set from); callee
//   object_type_definitions_notify_0x50 (0x4f4330, this batch).

// FIXED 2026-09-28: global_origin3d_pointer here is the global at its address comment, global_zero_vector3d_pointer (the name belonged to another
// global at a different address, so the link bound it there).

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern data_array *object_data; // 0x008603b0
// FIXED 2026-09-27 (static loop): 0x00696714 holds a POINTER to (0,0,0); 0x4f5169..0x4f51a3 copy [ptr]. The
// draft copied the pointer bits themselves into the velocities.
extern real_vector3d *global_origin3d_pointer; // 0x00696714
extern void object_type_definitions_notify_0x50(uint32_t object_index); // 0x4f4330, this batch

// FIXED (register inputs, objdump): the original never reads EAX as an input (it overwrites or only saves it); those parameters arrive on the stack (1 stack argument(s) read).
// blam-cc: stack -> object_index
void object_reset_velocity_and_wake(uint32_t object_index)
{
    object *obj = ((object_header *)object_data->data)[object_index & 0xffff].data;

    obj->velocity = *global_origin3d_pointer;
    obj->angular_velocity = *global_origin3d_pointer;
    obj->flags &= ~(uint32_t)_object_at_rest_bit;
    object_type_definitions_notify_0x50(object_index);
}

#if 0
Original Ghidra decompilation (0x4f5160):

void FUN_004f5160(uint param_1)

{
  int iVar1;
  undefined *puVar2;

  puVar2 = PTR_DAT_00696714;
  iVar1 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (param_1 & 0xffff) * 0xc);
  *(undefined4 *)(iVar1 + 0x68) = *(undefined4 *)PTR_DAT_00696714;
  *(undefined4 *)(iVar1 + 0x6c) = *(undefined4 *)(puVar2 + 4);
  *(undefined4 *)(iVar1 + 0x70) = *(undefined4 *)(puVar2 + 8);
  *(undefined4 *)(iVar1 + 0x8c) = *(undefined4 *)puVar2;
  *(undefined4 *)(iVar1 + 0x90) = *(undefined4 *)(puVar2 + 4);
  *(undefined4 *)(iVar1 + 0x94) = *(undefined4 *)(puVar2 + 8);
  *(uint *)(iVar1 + 0x10) = *(uint *)(iVar1 + 0x10) & 0xffffffdf;
  FUN_004f4330();
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
