// unit_set_throw_aim_direction  (Ghidra: FUN_005704d0; renamed from the phase2 proposal)
// address 0x5704d0, size 76 bytes
// name confidence: 0.4 (phase2 proposal at 0.4, matches functions.md summary)
// rewrite confidence: 0.5
// evidence: types/objects.h object.forward (0x074), .up (0x080), .parent_object (0x11c);
//   math.h global_up3d_pointer (0x00696720).
// register convention: unit object index in EAX (in_EAX); a 2-component direction pointer in
//   ECX (in_ECX).
//   // blam-cc: EAX -> object_index, ECX -> direction_xy
// UNSURE: the ECX operand's exact shape (only two dwords are read, at +0 and +4) is guessed as
//   {float x, float y} rather than a full vector.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "units.h"

extern data_array *object_data;         // 0x008603b0
extern real_vector3d *global_up3d_pointer;  // 0x00696720

// Records the unit's grenade-throw aim direction (object.forward.x/y from the caller-supplied
// direction, z zeroed) and a reference up-vector (the world-up constant), unless the unit is
// currently seated in something.
void unit_set_throw_aim_direction(uint32_t object_index, float direction_x, float direction_y)
{
    object *obj = ((object_header *)object_data->data)[object_index & 0xffff].data;

    if (obj->parent_object == k_datum_index_none) {
        obj->forward.i = direction_x;
        obj->forward.j = direction_y;
        obj->forward.k = 0.0f;
        obj->up = *global_up3d_pointer;
    }
}

#if 0
Original Ghidra decompilation (0x5704d0):

void FUN_005704d0(void)

{
  int iVar1;
  undefined4 uVar2;
  undefined *puVar3;
  uint in_EAX;
  undefined4 *in_ECX;

  puVar3 = PTR_DAT_00696720;
  iVar1 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (in_EAX & 0xffff) * 0xc);
  if (*(int *)(iVar1 + 0x11c) == -1) {
    uVar2 = *in_ECX;
    *(undefined4 *)(iVar1 + 0x78) = in_ECX[1];
    *(undefined4 *)(iVar1 + 0x74) = uVar2;
    *(undefined4 *)(iVar1 + 0x7c) = 0;
    *(undefined4 *)(iVar1 + 0x80) = *(undefined4 *)puVar3;
    *(undefined4 *)(iVar1 + 0x84) = *(undefined4 *)(puVar3 + 4);
    *(undefined4 *)(iVar1 + 0x88) = *(undefined4 *)(puVar3 + 8);
  }
  return;
}
#endif
