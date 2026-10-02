// unit_reset_orientation_and_find_position  (Ghidra: unit_reset_orientation_and_find_position, renamed)
// address 0x55add0, size 244 bytes
// name confidence: 0.35   rewrite confidence: 0.95
// evidence: object.forward/up at 0x074/0x080 (objects.h); biped_data.flags bit 0 "grounded"
//   (0x4cc, types/units.h); calls unit_find_placement_position (0x55a500) twice, matching this
//   batch's naming.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"
#include "units.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern data_array *object_data;     // 0x008603b0
extern real_vector3d *global_forward3d_pointer; // 0x00696718
extern real_vector3d *global_up3d_pointer;      // 0x00696720

extern real vector3d_normalize_with_length(real_vector3d *v); // 0x401990, ECX
extern uint32_t unit_find_placement_position(uint32_t anchor_object, uint32_t orientation_object,
                                              real_point3d *out_position, float radius, char grid_mode,
                                              char skip_reposition, char scale_radius,
                                              uint32_t object_index_a, real_vector3d *reference_direction); // 0x55a500

// REWRITTEN from objdump 0x55add0..0x55aec3. Stack: unit; EDI: the vehicle (or other object) it is leaving -- every
//   caller loads EDI with it. Flattens the unit's forward (world forward when degenerate), sets up to world up,
//   flags +0x4cc bit 0, then places it around the vehicle: first a 27-point grid at 2x the pill radius from the
//   unit, then around the vehicle's bounding centre (+0xa0) at its bounding radius (+0xac). The draft had no
//   vehicle and passed zeros (no radius, no grid) to both placement calls.
// blam-cc: stack -> object_index, EDI -> vehicle_index
void unit_reset_orientation_and_find_position(uint32_t object_index, uint32_t vehicle_index)
{
    uint8_t *obj = (uint8_t *)((object_header *)object_data->data)[object_index & 0xffff].data;
    real_vector3d *forward = (real_vector3d *)(obj + 0x74);

    forward->k = 0.0f;
    if (vector3d_normalize_with_length(forward) == 0.0f) {
        *forward = *global_forward3d_pointer;
    }
    *(real_vector3d *)&((unit_object *)obj)->base.up.i = *global_up3d_pointer;
    *(uint32_t *)(obj + 0x4cc) |= 1;
    if (!(uint8_t)unit_find_placement_position(object_index, vehicle_index, 0, 2.0f, 1, 0, 1, 0, 0)) {
        uint8_t *vehicle = (uint8_t *)((object_header *)object_data->data)[vehicle_index & 0xffff].data;
        real_point3d center = *(real_point3d *)&((unit_object *)vehicle)->base.bounding_center.x;    // [esp+0xc]

        unit_find_placement_position(object_index, vehicle_index, 0, ((unit_object *)vehicle)->base.bounding_radius, 1, 0, 0, 0,
            (real_vector3d *)&center);
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
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
