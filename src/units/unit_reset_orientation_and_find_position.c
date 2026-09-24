// unit_reset_orientation_and_find_position  (Ghidra: unit_reset_orientation_and_find_position, renamed)
// address 0x55add0, size 244 bytes
// name confidence: 0.35   rewrite confidence: 0.5
// evidence: object.forward/up at 0x074/0x080 (objects.h); biped_data.flags bit 0 "grounded"
//   (0x4cc, types/units.h); calls unit_find_placement_position (0x55a500) twice, matching this
//   batch's naming.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"
#include "units.h"

extern data_array *object_data;     // 0x008603b0
extern real_vector3d *global_forward3d_pointer; // 0x00696718
extern real_vector3d *global_up3d_pointer;      // 0x00696720

extern real vector3d_normalize_with_length(real_vector3d *v); // 0x401990
extern uint32_t unit_find_placement_position(uint32_t anchor_object, uint32_t orientation_object,
                                              real_point3d *out_position, float radius, char grid_mode,
                                              char skip_reposition, char scale_radius,
                                              uint32_t object_index_a, real_vector3d *reference_direction); // 0x55a500, this batch

// Resets a unit's forward vector to world-forward (falling back to global_forward3d if it's
// already degenerate) and its up vector unconditionally to world-up, marks it grounded, and
// tries twice to find a valid (collision-free) placement position via
// unit_find_placement_position.
void unit_reset_orientation_and_find_position(uint32_t object_index)
{
    object *obj = ((object_header *)object_data->data)[object_index & 0xffff].data;
    biped_data *biped = (biped_data *)((uint8_t *)obj + k_unit_object_size);

    obj->forward.k = 0.0f;
    if (vector3d_normalize_with_length(&obj->forward) == 0.0f) {
        obj->forward = *global_forward3d_pointer;
    }
    obj->up = *global_up3d_pointer;
    biped->flags |= 1;

    if (!unit_find_placement_position(object_index, k_datum_index_none, 0, 0.0f, 0, 0, 0, 0, 0)) {
        unit_find_placement_position(object_index, k_datum_index_none, 0, 0.0f, 0, 0, 0, 0, 0);
    }
}

#if 0
Original Ghidra decompilation (0x55add0):

void FUN_0055add0(uint param_1)

{
  int iVar1;
  undefined *puVar2;
  char cVar3;
  float10 fVar4;

  iVar1 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (param_1 & 0xffff) * 0xc);
  *(undefined4 *)(iVar1 + 0x7c) = 0;
  fVar4 = (float10)vector3d_normalize_with_length();
  puVar2 = PTR_DAT_00696718;
  if ((float10)0.0 == fVar4) {
    *(undefined4 *)(iVar1 + 0x74) = *(undefined4 *)PTR_DAT_00696718;
    *(undefined4 *)(iVar1 + 0x78) = *(undefined4 *)(puVar2 + 4);
    *(undefined4 *)(iVar1 + 0x7c) = *(undefined4 *)(puVar2 + 8);
  }
  puVar2 = PTR_DAT_00696720;
  *(undefined4 *)(iVar1 + 0x80) = *(undefined4 *)PTR_DAT_00696720;
  *(undefined4 *)(iVar1 + 0x84) = *(undefined4 *)(puVar2 + 4);
  *(undefined4 *)(iVar1 + 0x88) = *(undefined4 *)(puVar2 + 8);
  *(uint *)(iVar1 + 0x4cc) = *(uint *)(iVar1 + 0x4cc) | 1;
  cVar3 = FUN_0055a500(param_1);
  if (cVar3 == '\0') {
    FUN_0055a500(param_1);
  }
  return;
}
#endif
