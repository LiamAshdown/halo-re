// collision_gather_nearby_object_shapes  (Ghidra: FUN_005061c0; renamed)
// address 0x5061c0, size 613 bytes
// name confidence: 0.35   rewrite confidence: 0.35
// evidence: types/objects.h object (flags 0x010 with _object_no_collision_bit, bounding_center
//   0x0a0, bounding_radius 0x0ac, type 0x0b4, vitality_flags 0x106 with
//   _object_health_frozen_bit, next_object 0x114, first_child_object 0x118); types/units.h
//   unit_data.vehicle_seat_index (0x2f0) and biped_data.flags (0x4cc); types/physics.h "the two
//   provenance words" note (physics_shape_vertex_to_sphere(..., object_index, 0xffffffff, 0, 0xff)) confirming
//   the third physics_shape_vertex_to_sphere argument here is an object datum index, not a float; the same
//   object.next_object / first_child_object recursive-chain-walk idiom as FUN_005055b0
//   (out/phase2/physics/00.md), which this function mirrors for a different purpose
//   (physics_model proxy collection rather than a ray/sphere hit test); out/phase4/
//   physics_types_notes.md section 5 ("0x00507790 ... never the antenna widget ... resolve the
//   object Physics tag"), which is why the vehicle-vs-node dispatch below calls the
//   object-physics mass-point path rather than a debug-draw routine as out/phase4/
//   physics_functions.md's own (0.3-confidence) summary guessed.
// register convention: none recognized as "in_EAX" etc by Ghidra -- all eight are its own
//   ordinary parameters. The comparisons `fVar9 != param_7` and `fVar9 != -NAN` only make sense
//   as a raw 32-bit equality test (a float compare against NaN is never true, which would make
//   the loop never terminate), so param_2, param_7 and the fVar9 loop variable are corrected to
//   uint32_t datum indices here instead of Ghidra's float typing.
// param_8 is the physics_model* being built: FUN_00506440 (this module, higher half) passes its
//   own physics_model out-parameter down through this function's param_8, and that same pointer
//   checks its first three int16 fields (sphere_count/pill_count/shape_count) for
//   "was anything added" at the end of FUN_00506440, matching object_physics_add_mass_point_shapes'
//   (0x507790) identical check on its own equivalent parameter.
// UNSURE: unit_get_crouch_height_offset's exact signature; it clobbers the (already-consumed) param_2 stack slot
//   and the adjacent local_5c, which this rewrite models as writing a 2-float array through one
//   pointer. UNSURE: object flag bit 0x01000000 (tested alongside _object_no_collision_bit) is
//   not named in types/objects.h.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "units.h"
#include "physics.h"

extern data_array *object_data; // 0x008603b0

extern void unit_get_crouch_height_offset(float *out); // 0x55a2e0, foreign module, UNSURE signature
// blam-cc: UNSURE which registers besides the visible ones carry the rest of the physics_model
// pointer this ultimately writes into (see collision_test_movement_pill's own note on
// FUN_00502730 for the same pattern of registers still live from an outer caller).
extern void physics_shape_vertex_to_sphere(float center_x, float center_y, uint32_t object_index,
    uint32_t surface_index, int32_t margin, uint32_t flags); // 0x503360, this module (higher half)
extern uint8_t object_collision_context_build(uint32_t object_index, object_collision_context *out_context);
    // 0x504e10, this module (lower half); UNSURE exact register convention, called with zero
    // visible arguments in the original -- object_index is assumed still live from this loop
extern uint8_t object_collision_context_gather_sphere_shapes(void *context,
    real_point3d *origin, float radius_scale, float margin, float thickness,
    physics_model *model); // 0x505200, this module. Declaration shared with every other caller;
                           // param_4/param_5 are named margin/thickness after
                           // physics_shape_build_proxies_from_query, which they are forwarded to
                           // unmodified, rather than the x_offset/y_offset guess used here before. // 0x505200, this module (lower half)
extern uint8_t object_physics_context_build(uint32_t object_index,
    object_physics_context *out_context); // 0x5074b0, this module; same UNSURE note as
                                           // FUN_00504e10 above
extern uint8_t object_physics_add_mass_point_shapes(float x_offset, float y_offset,
    object_physics_context *context, int16_t *model_counts); // 0x507790, this module (higher half)

// Walks the object chain starting at start_object_index (following object.next_object) and,
// for every object matching flags' type mask that overlaps the query sphere (origin, radius) and
// is not itself excluded (exclude_object_index) or no-collision/frozen-dead, either appends it to
// the physics_model as a single sphere proxy (bipeds, via unit_get_crouch_height_offset + physics_shape_vertex_to_sphere) or builds
// its detailed collision-node or mass-point shape proxies (every other type, via FUN_00505200 or
// object_physics_add_mass_point_shapes depending on flags bit 0x400000). Recurses into
// first_child_object for every object visited, and into next_object for the walk itself.
void collision_gather_nearby_object_shapes(uint32_t flags, uint32_t start_object_index,
    real_point3d *origin, float radius, float x_offset, float y_offset,
    uint32_t exclude_object_index, physics_model *model)
{
    uint32_t object_index = start_object_index;

    do {
        object *obj = ((object_header *)object_data->data)[object_index & 0xffff].data;

        if (object_index != exclude_object_index &&
            (obj->flags & _object_no_collision_bit) == 0 &&
            (obj->flags & 0x01000000) == 0 && // UNSURE: unnamed object flag
            ((obj->vitality_flags & _object_health_frozen_bit) == 0 || obj->type != 0) &&
            (radius + obj->bounding_radius) * (radius + obj->bounding_radius) >=
                (obj->bounding_center.x - origin->x) * (obj->bounding_center.x - origin->x) +
                (obj->bounding_center.y - origin->y) * (obj->bounding_center.y - origin->y) +
                (obj->bounding_center.z - origin->z) * (obj->bounding_center.z - origin->z)) {

            if ((flags & (1u << ((obj->type + 8) & 0x1f))) != 0) {
                switch (obj->type) {
                case _object_type_biped: {
                    biped_data *biped = (biped_data *)((uint8_t *)obj + 0x4cc);
                    unit_data *unit = (unit_data *)((uint8_t *)obj + 0x1f4);
                    if (((flags & 0x200000) == 0 || (biped->flags & 0x10) == 0) &&
                        (obj->first_child_object == k_datum_index_none ||
                         unit->vehicle_seat_index == -1)) {
                        float sample[2];
                        unit_get_crouch_height_offset(sample);
                        physics_shape_vertex_to_sphere(sample[0] + x_offset, sample[1] + y_offset, object_index,
                            0xffffffff, 0, 0xff);
                    }
                    break;
                }
                case _object_type_vehicle:
                case _object_type_scenery:
                case _object_type_device_machine:
                case _object_type_device_control: {
                    // Mass-point path only for vehicles (type 1) with flag 0x400000 set;
                    // every other type (or vehicles without that flag) uses the node path.
                    if (obj->type != _object_type_vehicle || (flags & 0x400000) == 0) {
                        object_collision_context node_ctx;
                        if (object_collision_context_build(object_index, &node_ctx)) {
                            uint8_t node_context[76];
                            object_collision_context_gather_sphere_shapes(node_context, origin, radius, x_offset,
                                                         y_offset, model);
                        }
                    } else {
                        object_physics_context physics_ctx;
                        if (object_physics_context_build(object_index, &physics_ctx)) {
                            object_physics_add_mass_point_shapes(x_offset, y_offset, &physics_ctx,
                                (int16_t *)model);
                        }
                    }
                    break;
                }
                }
            }

            if (obj->first_child_object != k_datum_index_none) {
                collision_gather_nearby_object_shapes(flags, obj->first_child_object, origin,
                    radius, x_offset, y_offset, exclude_object_index, model);
            }
        }

        object_index = obj->next_object;
    } while (object_index != k_datum_index_none);
}

#if 0
Original Ghidra decompilation (0x5061c0):

void FUN_005061c0(uint param_1,float param_2,float *param_3,float param_4,float param_5,
                 float param_6,float param_7,undefined4 param_8)

{
  int iVar1;
  float fVar2;
  float fVar3;
  float fVar4;
  float fVar5;
  byte bVar6;
  char cVar7;
  float *pfVar8;
  float fVar9;
  float local_5c;
  undefined1 local_4c [76];

  fVar9 = param_2;
  do {
    pfVar8 = param_3;
    iVar1 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + ((uint)fVar9 & 0xffff) * 0xc);
    if (((((fVar9 != param_7) && ((*(uint *)(iVar1 + 0x10) & 1) == 0)) &&
         ((*(uint *)(iVar1 + 0x10) & 0x1000000) == 0)) &&
        (((*(byte *)(iVar1 + 0x106) & 4) == 0 || (*(short *)(iVar1 + 0xb4) != 0)))) &&
       (fVar2 = param_4 + *(float *)(iVar1 + 0xac), fVar3 = *(float *)(iVar1 + 0xa0) - *param_3,
       fVar5 = *(float *)(iVar1 + 0xa4) - param_3[1], fVar4 = *(float *)(iVar1 + 0xa8) - param_3[2],
       fVar5 * fVar5 + fVar4 * fVar4 + fVar3 * fVar3 <= fVar2 * fVar2)) {
      bVar6 = (byte)*(undefined2 *)(iVar1 + 0xb4);
      if ((param_1 & 1 << (bVar6 + 8 & 0x1f)) != 0) {
        switch(*(undefined2 *)(iVar1 + 0xb4)) {
        case 0:
          if ((((param_1 & 0x200000) == 0) || ((*(byte *)(iVar1 + 0x4cc) & 0x10) == 0)) &&
             ((*(int *)(iVar1 + 0x11c) == -1 || (*(short *)(iVar1 + 0x2f0) == -1)))) {
            FUN_0055a2e0(&param_2);
            FUN_00503360(param_2 + param_5,local_5c + param_6,fVar9,0xffffffff,0,0xff);
            pfVar8 = param_3;
          }
          break;
        case 1:
        case 6:
        case 7:
        case 8:
          if (((1 << (bVar6 & 0x1f) & 2U) == 0) || ((param_1 & 0x400000) == 0)) {
            cVar7 = FUN_00504e10();
            if (cVar7 != '\0') {
              FUN_00505200(local_4c,pfVar8,param_4,param_5,param_6,param_8);
            }
          }
          else {
            cVar7 = FUN_005074b0();
            if (cVar7 != '\0') {
              FUN_00507790(param_5,param_6,param_8);
            }
          }
        }
      }
      if (*(int *)(iVar1 + 0x118) != -1) {
        FUN_005061c0(param_1,*(int *)(iVar1 + 0x118),pfVar8,param_4,param_5,param_6,param_7,param_8)
        ;
      }
    }
    fVar9 = *(float *)(iVar1 + 0x114);
  } while (fVar9 != -NAN);
  return;
}
#endif
