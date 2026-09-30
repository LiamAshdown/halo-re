// biped_get_cached_look_at_position  (Ghidra: biped_get_cached_look_at_position, renamed)
// address 0x55ab30, size 464 bytes
// name confidence: 0.35   rewrite confidence: 0.7
// evidence: types/units.h biped_data fields cached_surface_index ("the cached look-at result"),
//   cached_position ("the cached look-at point 0x55ab30 refreshes"), cached_tick ("game tick that
//   cache was last refreshed"), previous_cached_surface_index ("the previous value of cached_surface_index"), tracked_target
//   ("the target 0x55e0a0 is tracking") -- all already attributed to this function by name in
//   the header, so used directly rather than re-derived.
// reconciled: R32 hs_game_time_globals -> game.h game_time_globals (current_tick->game_time, budget_flag_1/2->active/paused, seconds_per_tick->leftover_time; same offsets)

#include "tags.h"
#include "memory.h"
#include "hs.h"
#include "math.h"
#include "game.h"
#include "cache.h"
#include "objects.h"
#include "units.h"
#include "fn_units.h"

extern data_array *object_data;     // 0x008603b0
extern tag_instance *tag_instances; // 0x0087bc14
extern game_time_globals *game_time; // 0x006f1d6c, the game time globals (types/game.h)

extern uint8_t collision_bsp_surface_test_point_side_2d(ModelCollisionGeometryBSP *bsp,
    real_point2d *point, int32_t surface_index, int16_t axis, uint8_t sign);
    // 0x5014a0, src/physics; blam-cc: EAX bsp, EDI point, stack (surface_index, axis, sign)
extern uint32_t collision_bsp_surface_closest_edge_point_2d(ModelCollisionGeometryBSP *bsp,
    int32_t surface_index, uint16_t axis, uint8_t sign, real_point2d *point, real_point2d *out_point);
    // 0x5015a0, src/physics; blam-cc: EAX bsp, stack (surface_index, axis, sign, point, out_point)
extern real_point3d *collision_bsp_surface_solve_third_axis(ModelCollisionGeometryBSP *collision_bsp,
    int32_t surface_index, uint8_t component_sign, real_point3d *out, int32_t dominant_axis,
    const real_point2d *known); // 0x501470, src/physics
extern real_point3d *decal_plane_solve_third_axis(real_point3d *out, uint32_t component_sign,
    int32_t dominant_axis, const real_plane3d *plane, const real_point2d *known);
    // 0x44d860, src/math; blam-cc: stack out, AL component_sign, SI dominant_axis, EBX plane, EDI known
extern void object_get_position(real_point3d *out, uint32_t object_index); // 0x4f6900; blam-cc: EAX out, ECX object_index

extern ModelCollisionGeometryBSP *global_structure_collision_bsp; // 0x00746f98
extern real_vector3d *global_down3d_pointer;           // 0x0069672c

// Periodically refreshes and returns the biped's cached look-at surface (biped_data.cached_surface_index)
// and writes the cached point (cached_position) through out_position.
// objdump 0x55ab30..0x55acff (orphan pass 4 review rewrite; the earlier version swapped which
// fields feed which branch and passed a 1-argument form of the 0x501470 / 0x44d860 helpers):
//   * a biped whose tag has flag 0x4 while object byte 0x106 bit 0x4 is clear drops the cache:
//     cached_surface_index = -1, object_get_position(object_index, out_position) (ECX, EAX), and the
//     shared tail then overwrites *out_position with cached_position.
//   * otherwise, once per tick while cached_surface_index is -1 (game_time > cached_tick, signed):
//       - standing on a surface (ground_surface_index != -1): the closest point on that surface's
//         boundary to cached_position, projected along axis 2 (z) with sign 1, is lifted back onto
//         the surface plane (plane index & 0x7fffffff), and cached_surface_index = ground_surface_index;
//       - else when previous_cached_surface_index (the last result) is valid and cached_position still projects inside
//         that surface, cached_position is lifted onto it and cached_surface_index = previous_cached_surface_index;
//       - if that left cached_surface_index == -1, unit_test_placement_candidate(2.0, &point) with ESI =
//         global_down3d_pointer, EBX = 0 and ECX = object_index;
//       - a valid result stores point into cached_position and previous_cached_surface_index.
datum_index biped_get_cached_look_at_position(uint32_t object_index, real_point3d *out_position)
{
    object *obj = ((object_header *)object_data->data)[object_index & 0xffff].data;
    Biped *tag = (Biped *)tag_instances[obj->definition_tag & 0xffff].data;
    biped_data *biped = (biped_data *)((uint8_t *)obj + k_unit_object_size);

    if ((tag->biped_flags & 4) != 0 && (*((uint8_t *)obj + 0x106) & 4) == 0) {
        biped->cached_surface_index = k_datum_index_none;
        object_get_position(out_position, object_index);
    } else if (biped->cached_surface_index == k_datum_index_none && game_time->game_time > (int32_t)biped->cached_tick) {
        ModelCollisionGeometryBSP *bsp = global_structure_collision_bsp;
        int32_t surface = (int32_t)biped->ground_surface_index;
        real_point3d point = biped->cached_position;
        real_point2d closest; // [esp+0x10], the 2D result handed to the solver in EDI

        biped->cached_tick = game_time->game_time;
        if (surface != -1) {
            ModelCollisionGeometryBSPSurface *surfaces =
                (ModelCollisionGeometryBSPSurface *)bsp->surfaces.pointer;
            const real_plane3d *plane = (const real_plane3d *)((uint8_t *)bsp->planes.pointer +
                (surfaces[surface].plane & 0x7fffffff) * 0x10);

            collision_bsp_surface_closest_edge_point_2d(bsp, surface, 2, 1,
                (real_point2d *)&biped->cached_position, &closest);
            decal_plane_solve_third_axis(&point, 1, 2, plane, &closest);
            biped->cached_surface_index = biped->ground_surface_index;
        } else {
            int32_t previous = (int32_t)biped->previous_cached_surface_index;
            if (previous != -1 &&
                collision_bsp_surface_test_point_side_2d(bsp, (real_point2d *)&biped->cached_position,
                    previous, 2, 1)) {
                biped->cached_surface_index = (datum_index)previous;
                collision_bsp_surface_solve_third_axis(bsp, previous, 1, &point, 2,
                    (const real_point2d *)&biped->cached_position);
                biped->cached_surface_index = (datum_index)previous;
            }
        }

        if (biped->cached_surface_index == k_datum_index_none) {
            // 0x55ac85: the ground surface within 2 below (ECX unit, ESI global down, EBX 0)
            biped->cached_surface_index = (datum_index)unit_test_placement_candidate(object_index, global_down3d_pointer, 0,
                                                                            2.0f, &point);
        }
        if (biped->cached_surface_index != k_datum_index_none) {
            biped->cached_position = point;
            biped->previous_cached_surface_index = biped->cached_surface_index;
        }
    }

    *out_position = biped->cached_position;
    return biped->cached_surface_index;
}

#if 0
Original Ghidra decompilation (0x55ab30):

uint FUN_0055ab30(uint param_1,uint *param_2)

{
  uint *puVar1;
  char cVar2;
  uint uVar3;
  undefined1 local_14 [8];
  uint local_c;
  uint local_8;
  uint local_4;

  puVar1 = *(uint **)(*(int *)(DAT_008603b0 + 0x34) + 8 + (param_1 & 0xffff) * 0xc);
  if (((*(byte *)(*(int *)((*puVar1 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14) + 0x2f4) & 4) == 0) ||
     ((*(byte *)((int)puVar1 + 0x106) & 4) != 0)) {
    if ((puVar1[0x137] == 0xffffffff) && ((int)puVar1[0x13b] < (int)*(uint *)(DAT_006f1d6c + 0xc)))
    {
      local_c = puVar1[0x138];
      local_8 = puVar1[0x139];
      local_4 = puVar1[0x13a];
      puVar1[0x13b] = *(uint *)(DAT_006f1d6c + 0xc);
      if (puVar1[0x136] == 0xffffffff) {
        uVar3 = puVar1[0x13c];
        if ((uVar3 != 0xffffffff) && (cVar2 = FUN_005014a0(uVar3,2,1), cVar2 != '\0')) {
          puVar1[0x137] = uVar3;
          FUN_00501470(1,&local_c);
          puVar1[0x137] = uVar3;
        }
      }
      else {
        FUN_005015a0(puVar1[0x136],2,1,puVar1 + 0x138,local_14);
        FUN_0044d860(&local_c);
        puVar1[0x137] = puVar1[0x136];
      }
      if (puVar1[0x137] == 0xffffffff) {
        uVar3 = FUN_0055aa20(0x40000000,&local_c);
        puVar1[0x137] = uVar3;
      }
      if (puVar1[0x137] != 0xffffffff) {
        puVar1[0x138] = local_c;
        puVar1[0x139] = local_8;
        puVar1[0x13a] = local_4;
        puVar1[0x13c] = puVar1[0x137];
      }
    }
  }
  else {
    puVar1[0x137] = 0xffffffff;
    param_2 = (uint *)object_get_position();
  }
  *param_2 = puVar1[0x138];
  param_2[1] = puVar1[0x139];
  param_2[2] = puVar1[0x13a];
  return puVar1[0x137];
}
#endif
