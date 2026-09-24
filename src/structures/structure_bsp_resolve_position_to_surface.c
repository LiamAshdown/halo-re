// structure_bsp_resolve_position_to_surface  (Ghidra: FUN_00555190, still unnamed there)
// address 0x555190, size 219 bytes
// name confidence: 0.6   rewrite confidence: 0.7
// evidence: full stack-frame reconstruction from disassembly (objdump -d -M intel bin/halo.exe,
//   0x555190..0x55526a). Note the first three instructions execute BEFORE the `sub esp,0x50`, so
//   EAX and ESI are register parameters; only ONE of Ghidra's "param_1" style slots is genuinely
//   at [esp+0x58] (the direction), the other three stack parameters are forwarded verbatim to
//   structure_bsp_leaf_find_material_surface and never read here.
//   The decompile's `FUN_00505880(0x21)` is 0x505880 == collision_test_movement_segment
//   (src/physics, canonical signature reused verbatim): the call site pushes
//   (0x21, position, direction, -1, &result) and pops 0x14, matching that function's five stack
//   arguments exactly. An earlier rewrite of this file invented a three-argument
//   "jitter generator" contract for it; that is corrected here.
//   The collision_result fields this function reads are pinned by the frame offsets
//   (result base == esp+0x04 at the top of the loop):
//     +0x18 point            -> copied over *position after every successful segment test
//     +0x0c leaf.leaf_index  -> the EAX argument of structure_bsp_leaf_find_material_surface
//     +0x48 (types/projectiles.h `unknown_48`) -> masked with 0x7fffffff and handed over as the
//           accepted plane; that callee compares it against
//           collision_bsp->bsp3d_nodes[reference.node].plane, so +0x48 is the BSP3D PLANE INDEX
//           of the hit with bit 31 as the flip bit. New fact for types/projectiles.h's
//           collision_result, recorded here rather than edited into that header from this module.
//     +0x4c surface_flags    -> bit 0 gates the retry; with it clear the walk gives up.
// register convention: EAX -> start_position, ESI -> position (in/out), EDI -> out_lightmap_index,
//   EBX -> param_7 (forwarded). Stack: direction, out_material_index, out_surface, param_6.
//   // blam-cc: EAX -> start_position, ESI -> position, EDI -> out_lightmap_index, EBX -> param_7
// UNSURE: the two forwarded opaque values (param_6 / param_7) end up as the last two arguments of
//   0x4ce8c0 inside structure_bsp_leaf_find_material_surface and are never inspected on this path.
// reconciled: R23 collision_result: normal -> plane.normal, unknown_30 -> plane.d, unknown_04 -> first_leaf/first_cluster, unknown_3c -> region_index, marker_index -> node_index, unknown_40 -> permutation_index (int16), unknown_48 -> plane_index, unknown_4d -> breakable_surface_index, unknown_4e -> collision_material_index

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"   // bsp_leaf_reference, needed by projectiles.h's collision_result
#include "projectiles.h"
#include "structures.h"

extern ScenarioStructureBSP *global_structure_bsp; // 0x00746f9c

// 0x0067 2db0, the 1/4096 step the walk nudges the position by on each retry.
extern float k_surface_resolve_step;   // 0x00672db0 == 0.00024414062

extern uint8_t collision_test_movement_segment(uint32_t flags, real_point3d *origin,
    real_vector3d *delta, uint32_t exclude_object_index, collision_result *result); // 0x505880

// blam-cc: EAX -> raw_child, stack -> the rest
extern uint8_t structure_bsp_leaf_find_material_surface(real_point3d *point, int32_t accepted_plane,
    int16_t *out_lightmap_index, int16_t *out_material_index, int32_t *out_surface, void *param_6,
    void *param_7, int32_t raw_child); // 0x554fa0, this module

// Walks forward from `start_position` along `direction`, casting a collision segment each step,
// until one of the surfaces it hits resolves to a BSP surface whose lightmap actually has a
// bitmap. `position` ends holding the contact point of the accepted hit. Each failed step nudges
// the position 1/4096 further along the direction and tries again, but only while the last hit
// surface's flags bit 0 allows it; otherwise the walk fails.
// blam-cc: EAX -> start_position, ESI -> position, EDI -> out_lightmap_index, EBX -> param_7
uint8_t structure_bsp_resolve_position_to_surface(real_point3d *start_position,
                                                   real_point3d *position,
                                                   int16_t *out_lightmap_index, void *param_7,
                                                   real_vector3d *direction,
                                                   int16_t *out_material_index,
                                                   int32_t *out_surface, void *param_6)
{
    collision_result result;

    *position = *start_position;

    for (;;) {
        ScenarioStructureBSPLightmap *lightmaps;

        if (!collision_test_movement_segment(0x21, position, direction, 0xffffffff, &result)) {
            return 0;
        }
        *position = result.point;

        if (structure_bsp_leaf_find_material_surface(position,
                (int32_t)(result.plane_index & 0x7fffffff), out_lightmap_index, out_material_index,
                out_surface, param_6, param_7, result.leaf.leaf_index)) {
            lightmaps = (ScenarioStructureBSPLightmap *)global_structure_bsp->lightmaps.pointer;
            if (lightmaps[*out_lightmap_index].bitmap != 0xffff) {
                return 1;
            }
        }

        if ((result.surface_flags & 1) == 0) {
            return 0;
        }
        position->x = direction->i * k_surface_resolve_step + position->x;
        position->y = direction->j * k_surface_resolve_step + position->y;
        position->z = direction->k * k_surface_resolve_step + position->z;
    }
}

#if 0
Original Ghidra decompilation (0x555190):

undefined4 FUN_00555190(float *param_1)

{
  char cVar1;
  float *in_EAX;
  float *unaff_ESI;
  short *unaff_EDI;
  float local_38;
  float local_34;
  float local_30;
  byte local_4;

  *unaff_ESI = *in_EAX;
  unaff_ESI[1] = in_EAX[1];
  unaff_ESI[2] = in_EAX[2];
  while( true ) {
    cVar1 = FUN_00505880(0x21);
    if (cVar1 == '\0') {
      return 0;
    }
    *unaff_ESI = local_38;
    unaff_ESI[1] = local_34;
    unaff_ESI[2] = local_30;
    cVar1 = FUN_00554fa0();
    if ((cVar1 != '\0') && (*(short *)(*unaff_EDI * 0x20 + *(int *)(DAT_00746f9c + 0x108)) != -1))
    break;
    if ((local_4 & 1) == 0) {
      return 0;
    }
    *unaff_ESI = *param_1 * 0.00024414062 + *unaff_ESI;
    unaff_ESI[1] = param_1[1] * 0.00024414062 + unaff_ESI[1];
    unaff_ESI[2] = param_1[2] * 0.00024414062 + unaff_ESI[2];
  }
  return 1;
}
#endif
