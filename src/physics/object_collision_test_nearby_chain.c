// object_collision_test_nearby_chain  (Ghidra: FUN_00505350, still unnamed there; phase-2
// guessed object_test_collision_recursive)
// address 0x505350, size 305 bytes
// name confidence: 0.4   rewrite confidence: 0.3
// evidence: out/phase4/physics_functions.md ("Recursively searches a chain of nearby objects
//   for one whose collision geometry overlaps a given position, excluding a specified object.");
//   types/objects.h object (flags 0x010, bounding_center 0x0a0, bounding_radius 0x0ac, type
//   0x0b4, next_object 0x114, first_child_object 0x118 -- the same next_object/first_child_object
//   recursive-chain-walk idiom as collision_gather_nearby_object_shapes, this module); this
//   function's own two dispatch branches match object_collision_context_build/
//   object_collision_context_test_point (0x504e10/0x504e90, this batch) for the node path and
//   object_physics_context_build/object_physics_test_point_against_mass_points (0x5074b0/
//   0x507590, this module) for the vehicle mass-point path, the same type==vehicle-and-flag-
//   0x400000 split documented for collision_gather_nearby_object_shapes.
// register convention: param_1..param_4 are Ghidra's own recognized parameters (start_object_index,
//   type_mask, position, exclude_object_index).
//   // blam-cc: stack -> start_object_index, type_mask, position, exclude_object_index

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "physics.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern data_array *object_data; // 0x008603b0

extern uint8_t object_collision_context_build(uint32_t object_index,
    object_collision_context *out_context); // 0x504e10, this module
extern uint32_t object_collision_context_test_point(object_collision_context *context,
    real_point3d *point); // 0x504e90, this module
extern uint8_t object_physics_context_build(uint32_t object_index,
    object_physics_context *out_context); // 0x5074b0, this module
extern uint8_t object_physics_test_point_against_mass_points(object_physics_context *context,
    real_point3d *world_point, int16_t *out_index); // 0x507590, this module

// Walks the object chain starting at start_object_index (following object.next_object), testing
// each object matching type_mask that overlaps position's bounding sphere and is not
// exclude_object_index: vehicles with flag bit 0x400000 set are tested against their
// physics mass points, every other matching object against its collision-node geometry.
// Recurses into first_child_object for every object visited, and into next_object for the walk
// itself. Returns as soon as any object reports a hit.
// blam-cc: stack -> start_object_index, type_mask, position, exclude_object_index
uint8_t object_collision_test_nearby_chain(uint32_t start_object_index, uint32_t type_mask,
                                            real_point3d *position, uint32_t exclude_object_index)
{
    uint32_t object_index = start_object_index;

    do {
        object *obj = ((object_header *)object_data->data)[object_index & 0xffff].data;

        if (object_index != exclude_object_index && (obj->flags & 1) == 0) {
            uint8_t type = (uint8_t)obj->type;

            if ((type_mask & (1u << ((type + 8) & 0x1f))) != 0) {
                float dx = obj->bounding_center.x - position->x;
                float dy = obj->bounding_center.y - position->y;
                float dz = obj->bounding_center.z - position->z;

                if (dy * dy + dz * dz + dx * dx <= obj->bounding_radius * obj->bounding_radius) {
                    if (((1 << (type & 0x1f)) & 2) == 0 || (type_mask & 0x400000) == 0) {
                        object_collision_context node_ctx;

                        if (object_collision_context_build(object_index, &node_ctx) &&
                            object_collision_context_test_point(&node_ctx, position)) {
                            return 1;
                        }
                    } else {
                        object_physics_context phys_ctx;
                        int16_t hit_index;

                        if (object_physics_context_build(object_index, &phys_ctx) &&
                            object_physics_test_point_against_mass_points(&phys_ctx, position,
                                                                           &hit_index)) {
                            return 1;
                        }
                    }

                    if (obj->first_child_object != k_datum_index_none) {
                        if (object_collision_test_nearby_chain(obj->first_child_object, type_mask,
                                                                 position, exclude_object_index)) {
                            return 1;
                        }
                    }
                }
            }
        }
        object_index = obj->next_object;
    } while (object_index != k_datum_index_none);

    return 0;
}

#if 0
Original Ghidra decompilation (0x505350):

undefined4 FUN_00505350(uint param_1,uint param_2,float *param_3,uint param_4)

{
  int iVar1;
  float fVar2;
  float fVar3;
  float fVar4;
  byte bVar5;
  char cVar6;

  do {
    iVar1 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (param_1 & 0xffff) * 0xc);
    if ((((param_1 != param_4) && ((*(byte *)(iVar1 + 0x10) & 1) == 0)) &&
        (bVar5 = (byte)*(undefined2 *)(iVar1 + 0xb4), (param_2 & 1 << (bVar5 + 8 & 0x1f)) != 0)) &&
       (fVar2 = *(float *)(iVar1 + 0xa0) - *param_3, fVar4 = *(float *)(iVar1 + 0xa4) - param_3[1],
       fVar3 = *(float *)(iVar1 + 0xa8) - param_3[2],
       fVar4 * fVar4 + fVar3 * fVar3 + fVar2 * fVar2 <=
       *(float *)(iVar1 + 0xac) * *(float *)(iVar1 + 0xac))) {
      if (((1 << (bVar5 & 0x1f) & 2U) == 0) || ((param_2 & 0x400000) == 0)) {
        cVar6 = FUN_00504e10();
        if (cVar6 != '\0') {
          cVar6 = FUN_00504e90(param_3);
          goto LAB_00505437;
        }
      }
      else {
        cVar6 = FUN_005074b0();
        if (cVar6 != '\0') {
          cVar6 = FUN_00507590();
LAB_00505437:
          if (cVar6 != '\0') {
            return 1;
          }
        }
      }
      if ((*(int *)(iVar1 + 0x118) != -1) &&
         (cVar6 = FUN_00505350(*(int *)(iVar1 + 0x118),param_2,param_3,param_4), cVar6 != '\0')) {
        return 1;
      }
    }
    param_1 = *(uint *)(iVar1 + 0x114);
    if (param_1 == 0xffffffff) {
      return 0;
    }
  } while( true );
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
