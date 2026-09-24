// object_placement_data_initialize
// address 0x4f53a0, size 190 bytes
// name confidence: 0.85 (types/objects.h names and cites this exact address as
//   "object_placement_data_initialize" in both the object_placement_data struct comment and the
//   "Structs recovered" evidence trail)
// rewrite confidence: 0.75
// evidence: types/objects.h object_placement_data (every field below matches its offset:
//   definition_tag 0x00, flags 0x04, owner_linkage 0x08, role 0x0c, name_index 0x14,
//   permutation_group 0x16, forward 0x34 defaulting to the constant at 0x00696718, up 0x40
//   defaulting to the constant at 0x00696720, network_vectors[4] 0x58 seeded from the constant
//   at 0x00686b04); callee object_try_and_get 0x4f6ec0.
// register convention: destination object_placement_data* in EAX (in_EAX, the "self" pointer of
//   this constructor-style function); the tag id and the role value are BOTH stack arguments --
//   0x4f53a7 mov ebx,[esp+0xc] and 0x4f53ba mov eax,[esp+0x10] resolve to stack slots 2 and 1.
//   An earlier draft claimed ECX/EDX for them.
// note: the value stored in placement->role is the same value the function hands to
//   object_try_and_get in ECX (0x4f53f3 mov ecx,ebx), so the field at 0x0c is an object HANDLE,
//   not a small role enum. types/objects.h still calls it `role`; flagged rather than renamed.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"

extern object *object_try_and_get(datum_index object_index, uint32_t type_mask);
    // 0x4f6ec0; object handle in ECX, type mask on the stack. Verified against the body at
    // 0x4f6ec0 (cmp ecx,-1 / test cx,cx / and param_1 & 1 << header->type) and against the
    // call site in this file.
extern real_vector3d object_placement_default_forward; // 0x00696718
extern real_vector3d object_placement_default_up; // 0x00696720
extern real_vector3d object_placement_default_network_vectors[4]; // 0x00686b04

void object_placement_data_initialize(object_placement_data *placement, datum_index definition_tag,
                                       datum_index role)
    // blam-cc: EAX -> placement; stack -> definition_tag, role
{
    object *current;
    int32_t *raw = (int32_t *)placement;
    int i;

    for (i = 0; i < 0x22; i++) {
        raw[i] = 0;
    }

    placement->definition_tag = definition_tag;
    placement->flags = 0;
    placement->forward = object_placement_default_forward;
    placement->up = object_placement_default_up;
    placement->permutation_group = 0;

    current = object_try_and_get(role, _object_mask_all); // 0x4f53f3 mov ecx,ebx
    if (current == 0) {
        placement->role = 0xffffffff;
        placement->owner_linkage = 0xffffffff;
        placement->name_index = -1;
    } else {
        placement->role = role;
        placement->owner_linkage = *(uint32_t *)((uint8_t *)current + 0xc0);
        placement->name_index = *(int16_t *)((uint8_t *)current + 0xb8);
    }

    for (i = 0; i < 4; i++) {
        placement->network_vectors[i] = object_placement_default_network_vectors[i];
    }
}

#if 0
Original Ghidra decompilation (0x4f53a0):

void FUN_004f53a0(undefined4 param_1,undefined4 param_2)

{
  undefined *puVar1;
  undefined4 *in_EAX;
  undefined4 *puVar2;
  int iVar3;

  puVar1 = PTR_DAT_00696718;
  puVar2 = in_EAX;
  for (iVar3 = 0x22; iVar3 != 0; iVar3 = iVar3 + -1) {
    *puVar2 = 0;
    puVar2 = puVar2 + 1;
  }
  *in_EAX = param_1;
  in_EAX[1] = 0;
  in_EAX[0xd] = *(undefined4 *)puVar1;
  in_EAX[0xe] = *(undefined4 *)(puVar1 + 4);
  in_EAX[0xf] = *(undefined4 *)(puVar1 + 8);
  puVar1 = PTR_DAT_00696720;
  in_EAX[0x10] = *(undefined4 *)PTR_DAT_00696720;
  in_EAX[0x11] = *(undefined4 *)(puVar1 + 4);
  in_EAX[0x12] = *(undefined4 *)(puVar1 + 8);
  *(undefined2 *)((int)in_EAX + 0x16) = 0;
  iVar3 = object_try_and_get(0xffffffff);
  if (iVar3 == 0) {
    in_EAX[3] = 0xffffffff;
    in_EAX[2] = 0xffffffff;
    *(undefined2 *)(in_EAX + 5) = 0xffff;
  }
  else {
    in_EAX[3] = param_2;
    in_EAX[2] = *(undefined4 *)(iVar3 + 0xc0);
    *(undefined2 *)(in_EAX + 5) = *(undefined2 *)(iVar3 + 0xb8);
  }
  puVar1 = PTR_DAT_00686b04;
  iVar3 = 4;
  puVar2 = in_EAX + 0x16;
  do {
    *puVar2 = *(undefined4 *)puVar1;
    puVar2[1] = *(undefined4 *)(puVar1 + 4);
    iVar3 = iVar3 + -1;
    puVar2[2] = *(undefined4 *)(puVar1 + 8);
    puVar2 = puVar2 + 3;
  } while (iVar3 != 0);
  return;
}
#endif
