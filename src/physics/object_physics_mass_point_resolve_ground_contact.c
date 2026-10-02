// object_physics_mass_point_resolve_ground_contact  (Ghidra: FUN_00507ac0; renamed per
//   out/phase4/physics_functions.md summary and out/phase4/physics_types_notes.md section 5)
// address 0x507ac0, size 306 bytes
// name confidence: 0.4   rewrite confidence: 0.35
// evidence: types/physics.h mass_point_state (resting_plane seeded from k_default_resting_plane
//   0x0069c53c/40/44/48, ground_depth, material_type, flags bit 0x04
//   _mass_point_on_ground_surface_bit -- its own comment names this exact function),
//   physics_model_contact (0x2c layout matching every local_ac.. offset here byte for byte),
//   PhysicsMassPoint.radius (+0x68); types/objects.h object_header.type (+0x03, tested against
//   bit 6 = _object_type_scenery).
// register convention: param_1 -> exclude_object_index (forwarded to
//   physics_model_build_from_sphere_query), param_2 -> mass_point (mass_point_state *, Ghidra's
//   own recognized parameter), param_3 -> definition (PhysicsMassPoint *, this mass point's tag
//   definition, for its radius).
// UNSURE: physics_resolve_material_type is called with zero visible arguments here; this
//   rewrite supplies (contact.object_index, contact.material_type) on the assumption the
//   registers set up for the immediately preceding physics_shape_test_point call are still live, matching
//   physics_resolve_material_type's own two-parameter signature. object_set_shield_depleted_flag
//   is called the same way, with contact.object_index assumed still live in EAX.
// UNSURE: the mass_point_flags bit 0x04 (_mass_point_on_ground_surface_bit) is CLEARED when
//   "not breakable and (world or scenery)" and SET otherwise -- preserved exactly as decompiled,
//   even though the flag's own name/comment (written elsewhere) does not specify which sense is
//   the "set" one.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "physics.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern data_array *object_data; // 0x008603b0
extern float k_default_resting_plane[4]; // 0x0069c53c

extern uint8_t physics_shape_test_point(physics_model *model, real_point3d *point,
    physics_model_contact *out_contact); // 0x504260, this module (higher half)
extern uint8_t physics_model_build_from_sphere_query(uint32_t flags, real_point3d *center,
    float radius, float x_offset, float y_offset, uint32_t exclude_object_index,
    physics_model *model); // 0x506440, this module (higher half)
extern int16_t physics_resolve_material_type(uint32_t object_index, int16_t vertex_slot); // 0x507a40
extern void object_set_shield_depleted_flag(uint32_t object_index); // 0x4edb10, objects module,
    // UNSURE args

// Seeds mass_point's resting plane to k_default_resting_plane and computes its ground_depth
// against that default plane, then runs a sphere query (physics_model_build_from_sphere_query,
// flags 0xc0a0: structure BSP + nearby objects) around mass_point->position at definition->radius.
// If that finds anything and a point test against the resulting model (physics_shape_test_point) also hits,
// overwrites resting_plane/ground_depth/material_type from the contact, updates
// _mass_point_on_ground_surface_bit (see UNSURE above), and depletes the hit object's shield
// when the contact was against an object rather than the world.
void object_physics_mass_point_resolve_ground_contact(uint32_t exclude_object_index,
    mass_point_state *mass_point, PhysicsMassPoint *definition)
{
    physics_model model;
    physics_model_contact contact;

    mass_point->resting_plane_i = k_default_resting_plane[0];
    mass_point->resting_plane_j = k_default_resting_plane[1];
    mass_point->resting_plane_k = k_default_resting_plane[2];
    mass_point->resting_plane_d = k_default_resting_plane[3];
    mass_point->material_type = -1;

    mass_point->ground_depth = definition->radius -
        ((mass_point->resting_plane_i * mass_point->position_x +
          mass_point->resting_plane_j * mass_point->position_y +
          mass_point->resting_plane_k * mass_point->position_z) - mass_point->resting_plane_d);

    if (physics_model_build_from_sphere_query(0xc0a0, (real_point3d *)&mass_point->position_x,
            definition->radius, 0.0f, definition->radius, exclude_object_index, &model)) {
        if (physics_shape_test_point(&model, (real_point3d *)&mass_point->position_x, &contact)) {
            uint8_t is_scenery;

            mass_point->resting_plane_i = contact.plane_i;
            mass_point->resting_plane_j = contact.plane_j;
            mass_point->resting_plane_k = contact.plane_k;
            mass_point->ground_depth = contact.t;
            mass_point->resting_plane_d = contact.plane_d;
            mass_point->material_type =
                physics_resolve_material_type(contact.object_index, contact.material_type);

            is_scenery = contact.object_index != 0xffffffff &&
                (1u << (((object_header *)object_data->data)[contact.object_index & 0xffff].type &
                        0x1f) & 0x40) != 0;

            if ((contact.surface_flags & 8) == 0 &&
                (contact.object_index == 0xffffffff || is_scenery)) {
                mass_point->flags &= ~(uint32_t)_mass_point_on_ground_surface_bit;
            } else {
                mass_point->flags |= _mass_point_on_ground_surface_bit;
            }

            if (contact.object_index != 0xffffffff) {
                object_set_shield_depleted_flag(contact.object_index);
            }
        }
    }
}

#if 0
Original Ghidra decompilation (0x507ac0):

/* WARNING: Function: __chkstk replaced with injection: alloca_probe */

void FUN_00507ac0(undefined4 param_1,uint *param_2,int param_3)

{
  float *pfVar1;
  float *pfVar2;
  char cVar3;
  undefined2 uVar4;
  uint uVar5;
  uint local_ac38 [4];
  float local_ac28;
  uint local_ac24;
  uint local_ac20;
  uint local_ac1c;
  uint local_ac18;
  byte local_ac10;
  undefined1 local_ac08 [44036];
  undefined4 uStack_4;

  uStack_4 = 0x507aca;
  pfVar1 = (float *)(param_2 + 0x18);
  *pfVar1 = DAT_0069c53c;
  param_2[0x19] = DAT_0069c540;
  param_2[0x1a] = DAT_0069c544;
  param_2[0x1b] = DAT_0069c548;
  *(undefined2 *)(param_2 + 0x1c) = 0xffff;
  pfVar2 = (float *)(param_2 + 1);
  param_2[0x1d] =
       (uint)(*(float *)(param_3 + 0x68) -
             ((*pfVar1 * *pfVar2 +
              (float)param_2[0x19] * (float)param_2[2] + (float)param_2[0x1a] * (float)param_2[3]) -
             (float)param_2[0x1b]));
  cVar3 = FUN_00506440(0xc0a0,pfVar2,*(undefined4 *)(param_3 + 0x68),0,
                       *(undefined4 *)(param_3 + 0x68),param_1,local_ac08);
  if (cVar3 != '\0') {
    cVar3 = FUN_00504260(local_ac08,pfVar2,local_ac38);
    if (cVar3 != '\0') {
      *pfVar1 = local_ac28;
      param_2[0x19] = local_ac24;
      param_2[0x1a] = local_ac20;
      param_2[0x1d] = local_ac38[0];
      param_2[0x1b] = local_ac1c;
      uVar4 = FUN_00507a40();
      *(undefined2 *)(param_2 + 0x1c) = uVar4;
      if (((local_ac10 & 8) == 0) &&
         ((local_ac18 == 0xffffffff ||
          ((1 << (*(byte *)(*(int *)(DAT_008603b0 + 0x34) + 3 + (local_ac18 & 0xffff) * 0xc) & 0x1f)
           & 0x40U) != 0)))) {
        uVar5 = *param_2 & 0xfffffffb;
      }
      else {
        uVar5 = *param_2 | 4;
      }
      *param_2 = uVar5;
      if (local_ac18 != 0xffffffff) {
        object_set_shield_depleted_flag();
      }
    }
  }
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
