// object_get_root_object_velocities  (named by out/phase4/objects_types_notes.md: "0x068
// velocity, 0x08c angular velocity | object_get_root_object_velocities 0x4f6aa0 returns
// exactly these two triples")
// address 0x4f6aa0, size 105 bytes
// name confidence: 0.85 (fixed by the types notes' own citation of this address by this name)
// rewrite confidence: 0.75
// evidence: types/objects.h object (parent_object 0x11c, velocity 0x068, angular_velocity
//   0x08c); global 0x008603b0 object_data.
// register convention: object index in EAX, out_velocity in ESI, out_angular_velocity in EDI
//   (Ghidra's unaff_ESI/unaff_EDI). Confirmed against objdump -d -M intel bin/halo.exe:
//   0x4f6ad7 test esi,esi and 0x4f6aee test edi,edi guard the two output writes, neither
//   register is ever assigned inside the function.
//   // blam-cc: EAX -> object_index, ESI -> out_velocity, EDI -> out_angular_velocity

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"

extern data_array *object_data; // 0x008603b0

void object_get_root_object_velocities(uint32_t object_index, real_vector3d *out_velocity,
    real_vector3d *out_angular_velocity) // blam-cc: EAX -> object_index, ESI -> out_velocity, EDI -> out_angular_velocity
{
    object *root = ((object_header *)object_data->data)[object_index & 0xffff].data;

    while (root->parent_object != k_datum_index_none) {
        root = ((object_header *)object_data->data)[root->parent_object & 0xffff].data;
    }

    if (out_velocity != (real_vector3d *)0) {
        *out_velocity = root->velocity;
    }
    if (out_angular_velocity != (real_vector3d *)0) {
        *out_angular_velocity = root->angular_velocity;
    }
}

#if 0
Original Ghidra decompilation (0x4f6aa0):

void FUN_004f6aa0(void)

{
  int iVar1;
  uint uVar2;
  uint in_EAX;
  undefined4 *unaff_ESI;
  undefined4 *unaff_EDI;

  iVar1 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (in_EAX & 0xffff) * 0xc);
  uVar2 = *(uint *)(iVar1 + 0x11c);
  while (uVar2 != 0xffffffff) {
    iVar1 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (uVar2 & 0xffff) * 0xc);
    uVar2 = *(uint *)(iVar1 + 0x11c);
  }
  if (unaff_ESI != (undefined4 *)0x0) {
    *unaff_ESI = *(undefined4 *)(iVar1 + 0x68);
    unaff_ESI[1] = *(undefined4 *)(iVar1 + 0x6c);
    unaff_ESI[2] = *(undefined4 *)(iVar1 + 0x70);
  }
  if (unaff_EDI != (undefined4 *)0x0) {
    *unaff_EDI = *(undefined4 *)(iVar1 + 0x8c);
    unaff_EDI[1] = *(undefined4 *)(iVar1 + 0x90);
    unaff_EDI[2] = *(undefined4 *)(iVar1 + 0x94);
  }
  return;
}
#endif
