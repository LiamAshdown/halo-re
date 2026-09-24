// object_physics_tick  (Ghidra: FUN_00507840; renamed per out/phase4/physics_types_notes.md
//   section 5, which resolves this whole call family to the object Physics tag rather than the
//   phase2 antenna_* guess)
// address 0x507840, size 503 bytes
// name confidence: 0.4   rewrite confidence: 0.35
// evidence: types/physics.h section 5 ("0x00507840 picks [object_physics_tick_single_pass] when Physics.radius >
//   0.0; the multi-mass-point path runs when the radius is 0 or less") and its
//   powered_mass_point_state struct comment (matrix4x3_from_quaternion then the in-place 3x3
//   transpose swapping m01/m10, m02/m20, m12/m21, now written via named matrix[row][col] fields
//   instead of the +8/+0x10/+0xc/+0x1c/+0x18/+0x20 raw offsets); types/units.h vehicle_data
//   accumulated_force/accumulated_torque (object +0x508/+0x514, "zeroed by 0x570b00" -- this
//   function is the other zeroer); src/objects (object_data / tag_instances) established
//   elsewhere in this batch for the Object-tag-to-Physics-tag resolution this function repeats
//   inline instead of going through object_physics_context_build for the tag lookup itself
//   (though it does call it, via FUN_005074b0, for the context used later).
// register convention: none recognized as in_EAX etc; all five are Ghidra's own ordinary
//   parameters (`FUN_00507840(uint param_1, int param_2, undefined4 param_3, float *param_4,
//   float *param_5)`). extraout_EDX (matrix4x3_from_quaternion's implicit result register) is
//   the address of powered_mass_points[i].matrix_scale.
// UNSURE (major): matrix4x3_from_quaternion is called with zero visible arguments. param_2 is
//   confirmed as powered_mass_point_state* (not a quaternion array) by
//   object_physics_compute_mass_point_forces' own use of it at stride 0x60 =
//   sizeof(powered_mass_point_state), so the actual quaternion source here is some other,
//   entirely hidden input this function's decompile never shows -- this rewrite calls
//   matrix4x3_from_quaternion with a NULL placeholder rather than guess at it.
// UNSURE: the final antenna_object_integrate_and_test_rest (0x5097e0, renamed
//   object_physics_integrate_and_test_at_rest in this batch) call passes only &local_54 (the base
//   of the contiguous {torque, force} accumulator this function builds); whether that callee
//   really expects one combined 6-float pointer or something else is confirmed only by that
//   function's own rewrite.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"
#include "units.h"
#include "physics.h"

extern data_array *object_data;     // 0x008603b0
extern tag_instance *tag_instances; // 0x0087bc14

extern uint8_t object_physics_context_build(uint32_t object_index,
    object_physics_context *out_context); // 0x5074b0, this module
extern void matrix4x3_from_quaternion(void *quaternion, void *out_matrix); // 0x4cbad0, math
    // module; blam-cc UNSURE, see file header
extern void object_physics_compute_mass_point_forces(object_physics_context *context,
    powered_mass_point_state *powered_states, uint32_t param_3, real_vector3d *out_force,
    real_vector3d *out_torque); // 0x507cc0, this module (higher half)
extern void object_physics_integrate_and_test_at_rest(object_physics_context *context,
    uint32_t param_3, void *torque_and_force); // 0x5097e0, this module (higher half)
extern void object_physics_handle_nearby_object_impacts(uint32_t object_index); // 0x508a10, this module (higher half)
extern void object_physics_tick_single_pass(uint32_t object_index, powered_mass_point_state *powered_states, uint32_t param_3,
    real_vector3d *extra_force, real_vector3d *extra_torque); // 0x509e80, this module (higher half)

// object_physics_tick_accumulator now lives in types/physics.h.

// Dispatches one physics tick for object_index's Physics tag: object_physics_tick_single_pass (the single-mass-point
// path) when Physics.radius > 0, otherwise the general multi-mass-point path below. That path
// re-orients every powered mass point's driven matrix from powered_states (when powered_states !=
// NULL), computes the total mass-point force/torque via object_physics_compute_mass_point_forces,
// folds in the object's own accumulated_force/accumulated_torque (object +0x508/+0x514, zeroing
// them after), optionally adds extra_force/extra_torque when non-NULL, integrates via
// object_physics_integrate_and_test_at_rest, and finally runs object_physics_handle_nearby_object_impacts (nearby-object impact
// handling).
void object_physics_tick(uint32_t object_index, powered_mass_point_state *powered_states, uint32_t param_3,
    real_vector3d *extra_force, real_vector3d *extra_torque)
{
    object *obj = ((object_header *)object_data->data)[object_index & 0xffff].data;
    void *object_tag_data = tag_instances[obj->definition_tag & 0xffff].data;
    int32_t physics_tag_id = *(int32_t *)((uint8_t *)object_tag_data + 0x8c);
    Physics *definition = (Physics *)tag_instances[(uint16_t)physics_tag_id].data;

    if (definition->radius <= 0.0f) {
        object_physics_context context;
        object_physics_tick_accumulator accum;
        // types/units.h's vehicle_data still declares only uint32_t unknown_508/unknown_514
        // placeholders; types/physics.h's own cross-module note resolves them as
        // real_vector3d accumulated_force/accumulated_torque, not yet folded back into
        // units.h, so this rewrite accesses them by raw offset instead of through that struct.
        real_vector3d *accumulated_force = (real_vector3d *)((uint8_t *)obj + 0x508);
        real_vector3d *accumulated_torque = (real_vector3d *)((uint8_t *)obj + 0x514);

        object_physics_context_build(object_index, &context);

        if (powered_states != 0 && definition->powered_mass_points.count > 0) {
            int16_t i;
            for (i = 0; i < definition->powered_mass_points.count; i++) {
                powered_mass_point_state *powered = &((powered_mass_point_state *)
                    definition->powered_mass_points.pointer)[i];
                float t;

                // UNSURE: the quaternion source is not powered_states itself (that is the
                // OUTPUT array, confirmed by object_physics_compute_mass_point_forces' own use
                // of it at stride 0x60 = sizeof(powered_mass_point_state)); it must be some
                // other hidden-register input this function's own decompile never shows at all.
                matrix4x3_from_quaternion((void *)0 /* UNSURE quaternion source */,
                    &powered->matrix_scale);

                t = powered->matrix[0][1];
                powered->matrix[0][1] = powered->matrix[1][0];
                powered->matrix[1][0] = t;
                t = powered->matrix[0][2];
                powered->matrix[0][2] = powered->matrix[2][0];
                powered->matrix[2][0] = t;
                t = powered->matrix[1][2];
                powered->matrix[1][2] = powered->matrix[2][1];
                powered->matrix[2][1] = t;
            }
        }

        object_physics_compute_mass_point_forces(&context, powered_states, param_3, &accum.force,
            &accum.torque);

        accum.force.i += accumulated_force->i;
        accum.force.j += accumulated_force->j;
        accum.force.k += accumulated_force->k;
        accum.torque.i += accumulated_torque->i;
        accum.torque.j += accumulated_torque->j;
        accum.torque.k += accumulated_torque->k;
        accumulated_force->i = 0.0f;
        accumulated_force->j = 0.0f;
        accumulated_force->k = 0.0f;
        accumulated_torque->i = 0.0f;
        accumulated_torque->j = 0.0f;
        accumulated_torque->k = 0.0f;

        if (extra_force != (real_vector3d *)0) {
            accum.force.i += extra_force->i;
            accum.force.j += extra_force->j;
            accum.force.k += extra_force->k;
        }
        if (extra_torque != (real_vector3d *)0) {
            accum.torque.i += extra_torque->i;
            accum.torque.j += extra_torque->j;
            accum.torque.k += extra_torque->k;
        }

        object_physics_integrate_and_test_at_rest(&context, param_3, &accum);
        object_physics_handle_nearby_object_impacts(object_index);
        return;
    }

    object_physics_tick_single_pass(object_index, powered_states, param_3, extra_force, extra_torque);
}

#if 0
Original Ghidra decompilation (0x507840):

void FUN_00507840(uint param_1,int param_2,undefined4 param_3,float *param_4,float *param_5)

{
  float *pfVar1;
  undefined4 uVar2;
  undefined4 uVar3;
  int extraout_EDX;
  short sVar4;
  int iVar5;
  float local_54;
  float local_50;
  float local_4c;
  float local_48;
  float local_44;
  float local_40;
  undefined1 local_3c [60];

  iVar5 = (param_1 & 0xffff) * 0xc;
  pfVar1 = *(float **)
            ((*(uint *)(*(int *)((**(uint **)(*(int *)(DAT_008603b0 + 0x34) + 8 + iVar5) & 0xffff) *
                                 0x20 + 0x14 + DAT_0087bc14) + 0x8c) & 0xffff) * 0x20 + 0x14 +
            DAT_0087bc14);
  if (*pfVar1 <= 0.0) {
    FUN_005074b0();
    if ((param_2 != 0) && (sVar4 = 0, 0 < (int)pfVar1[0x1a])) {
      do {
        matrix4x3_from_quaternion();
        uVar2 = *(undefined4 *)(extraout_EDX + 8);
        *(undefined4 *)(extraout_EDX + 8) = *(undefined4 *)(extraout_EDX + 0x10);
        uVar3 = *(undefined4 *)(extraout_EDX + 0xc);
        *(undefined4 *)(extraout_EDX + 0x10) = uVar2;
        *(undefined4 *)(extraout_EDX + 0xc) = *(undefined4 *)(extraout_EDX + 0x1c);
        uVar2 = *(undefined4 *)(extraout_EDX + 0x18);
        sVar4 = sVar4 + 1;
        *(undefined4 *)(extraout_EDX + 0x18) = *(undefined4 *)(extraout_EDX + 0x20);
        *(undefined4 *)(extraout_EDX + 0x20) = uVar2;
        *(undefined4 *)(extraout_EDX + 0x1c) = uVar3;
      } while ((int)sVar4 < (int)pfVar1[0x1a]);
    }
    antenna_object_compute_vertex_forces(local_3c,param_2,param_3,&local_48,&local_54);
    iVar5 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + iVar5);
    local_48 = local_48 + *(float *)(iVar5 + 0x508);
    local_44 = local_44 + *(float *)(iVar5 + 0x50c);
    local_40 = local_40 + *(float *)(iVar5 + 0x510);
    local_54 = local_54 + *(float *)(iVar5 + 0x514);
    local_50 = local_50 + *(float *)(iVar5 + 0x518);
    local_4c = local_4c + *(float *)(iVar5 + 0x51c);
    *(undefined4 *)(iVar5 + 0x508) = 0;
    *(undefined4 *)(iVar5 + 0x50c) = 0;
    *(undefined4 *)(iVar5 + 0x510) = 0;
    *(undefined4 *)(iVar5 + 0x514) = 0;
    *(undefined4 *)(iVar5 + 0x518) = 0;
    *(undefined4 *)(iVar5 + 0x51c) = 0;
    if (param_4 != (float *)0x0) {
      local_48 = local_48 + *param_4;
      local_44 = local_44 + param_4[1];
      local_40 = local_40 + param_4[2];
    }
    if (param_5 != (float *)0x0) {
      local_54 = local_54 + *param_5;
      local_50 = local_50 + param_5[1];
      local_4c = local_4c + param_5[2];
    }
    antenna_object_integrate_and_test_rest(local_3c,param_3,&local_54);
    FUN_00508a10(param_1);
    return;
  }
  FUN_00509e80(param_1,param_2,param_3,param_4,param_5);
  return;
}
#endif
