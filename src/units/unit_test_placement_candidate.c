// unit_test_placement_candidate  (Ghidra: unit_test_placement_candidate, renamed)
// address 0x55aa20, size 267 bytes
// name confidence: 0.35   rewrite confidence: 0.9
// REWRITTEN from objdump 0x55aa20..0x55ab2a. ECX: unit, ESI: direction, EBX: out plane normal (may be 0), stack:
//   (distance, out_position (may be 0)). Casts direction * distance from 0.4 above the unit (along global up)
//   against the structure BSP (0x502060, flags 1). Returns the surface hit, or -1; on a hit writes the point
//   and the plane's normal. Every caller passes global_down3d (0x69672c): "is there ground within distance".
//   The draft took no unit, returned the hit flag instead of the surface, and had no result buffer.
// blam-cc: ECX -> unit_index, ESI -> direction, EBX -> out_normal, stack -> distance, out_position

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"
#include "units.h"
#include "physics.h"

extern ModelCollisionGeometryBSP *global_structure_collision_bsp; // 0x00746f98
extern real_vector3d *global_up3d_pointer; // 0x00696720

extern void object_get_position(real_point3d *out, uint32_t object_index); // 0x4f6900, EAX, ECX
extern uint8_t collision_bsp_query_segment_init(uint32_t flags, collision_bsp_segment_result *result,
    ModelCollisionGeometryBSP *bsp, int16_t breakable_surface_count, uint32_t *breakable_surfaces,
    real_point3d *origin, real_vector3d *delta, float max_fraction); // 0x502060

int32_t unit_test_placement_candidate(uint32_t unit_index, const real_vector3d *direction,
                                      real_vector3d *out_normal, float distance, real_point3d *out_position)
{
    static collision_bsp_segment_result result;
    real_point3d origin;
    real_vector3d delta;

    object_get_position(&origin, unit_index);
    origin.x += global_up3d_pointer->i * 0.4f;
    origin.y += global_up3d_pointer->j * 0.4f;
    origin.z += global_up3d_pointer->k * 0.4f;
    delta.i = distance * direction->i;
    delta.j = distance * direction->j;
    delta.k = distance * direction->k;
    if (!collision_bsp_query_segment_init(1, &result, global_structure_collision_bsp, 0, 0, &origin, &delta,
                                          3.4028235e+38f)) {
        return -1;
    }
    if (out_position != 0) {
        out_position->x = delta.i * result.t + origin.x;
        out_position->y = delta.j * result.t + origin.y;
        out_position->z = delta.k * result.t + origin.z;
    }
    if (out_normal != 0) {
        *out_normal = *(real_vector3d *)result.plane;
    }
    return result.surface_index;
}

#if 0
Original Ghidra decompilation (0x55aa20):

undefined4 FUN_0055aa20(float param_1,float *param_2)

{
  char cVar1;
  undefined4 *unaff_EBX;
  float *unaff_ESI;
  float local_430;
  float local_42c;
  float local_428;
  float local_424;
  float local_420;
  float local_41c;
  float local_418;
  undefined4 *local_414;
  undefined4 local_410;

  object_get_position();
  local_430 = *(float *)PTR_DAT_00696720 * 0.4 + local_430;
  local_42c = *(float *)(PTR_DAT_00696720 + 4) * 0.4 + local_42c;
  local_428 = *(float *)(PTR_DAT_00696720 + 8) * 0.4 + local_428;
  local_424 = param_1 * *unaff_ESI;
  local_420 = param_1 * unaff_ESI[1];
  local_41c = param_1 * unaff_ESI[2];
  cVar1 = FUN_00502060(DAT_00746f98,0,0,&local_430,&local_424,0x7f7fffff);
  if (cVar1 == '\0') {
    local_410 = 0xffffffff;
  }
  else {
    if (param_2 != (float *)0x0) {
      *param_2 = local_424 * local_418 + local_430;
      param_2[1] = local_420 * local_418 + local_42c;
      param_2[2] = local_41c * local_418 + local_428;
    }
    if (unaff_EBX != (undefined4 *)0x0) {
      *unaff_EBX = *local_414;
      unaff_EBX[1] = local_414[1];
      unaff_EBX[2] = local_414[2];
      return local_410;
    }
  }
  return local_410;
}
#endif
