// point_physics_tick  (Ghidra: FUN_0050b530, still unnamed there; named "point_physics" by
//   types/physics.h's own section header for this exact address)
// address 0x50b530, size 1195 bytes
// name confidence: 0.45   rewrite confidence: 0.85 (VERIFIED 2026-09-27 static loop against objdump 0x50b530..0x50b9d9 section by section (arguments, medium, wind nudge, gravity, drag clamp, collision flags, 3-bounce loop); offsets probed) (raised by the phase-4 integration pass after
//   the bounce-loop exits were corrected; see the note below)
// evidence: types/physics.h's dedicated "PointPhysics tag drives 0x0050b530 entirely through
//   named fields" note gives every param_2[N] field name directly (flags +0x00, mass_scale
//   +0x04, water_gravity_scale +0x08, air_gravity_scale +0x0c, air_friction +0x24, water_friction
//   +0x28, surface_friction +0x2c, elasticity +0x30) and the point_physics_result_flags enum
//   (in_air=1, in_water=2, collided=4, hit_water_surface=8) this function returns; the "what this
//   module adds to collision_result" section confirms 0x0050b530 reads collision_result type 0 as
//   "hit the water surface"; collision_test_movement_segment.c's (0x505880, this module) own
//   collision_test_movement_segment_flags enum (_structure_bsp=0x20, _water_surface=0x40,
//   _nearby_objects=0x80) names the three flag bits this function ORs into its own call;
//   k_physics_collision_iterations (3, "0x0050b530 resolves at most three bounces");
//   types/projectiles.h collision_result -- every local_50/local_44../local_1c offset below
//   (0x50 down to 0x1a) is exactly one collision_result struct starting at &local_50, closed by
//   local_1c landing precisely on material_type (+0x34).
// register convention: unaff_ESI -> velocity (real_vector3d *, read-modify-write; the caller's
//   point's own velocity). param_1..param_10 are Ghidra's own recognized stack parameters.
//   // blam-cc: ESI -> velocity, stack -> flags_arg, definition, out_leaf, unused_param_4,
//   //          position, wind, out_normal, out_material_type, radius, dt
// UNSURE: param_4 is never referenced anywhere in this function's own decompile at all; kept as
//   an unused parameter for stack-layout fidelity rather than dropped.
// UNSURE: ambient_color_marker_visible / ambient_color_for_marker (0x53f860 / 0x53f940) are outside this module's slice
//   (scenario/weather wind probes); declared opaque. ambient_color_for_marker is called with only the
//   position argument visible, yet its result feeds the same wind vector ambient_color_marker_visible fills
//   explicitly -- read here as also writing through a hidden out_wind pointer, by analogy.
// The bounce loop has TWO distinct exits and they do different things, which an earlier rewrite
//   of this file collapsed into one: `break` (collision_test_movement_segment found nothing) runs
//   the post-loop block, which copies collision_result.leaf and collision_result.point into
//   *out_leaf and *position -- that is the ordinary "the mover travelled the whole tick freely"
//   advance. Loop exhaustion (`if (2 < sVar5) return local_88`) returns without it, because the
//   third bounce already positioned the mover. Restored by the phase-4 integration pass; the
//   earlier version dropped the post-loop block entirely, so a tick with no collision never
//   moved the point.
// The original does not null-check param_3 (out_leaf) before writing through it, on either
//   path, although it does null-check param_7 (out_normal) and param_8 (out_material_type).
//   Preserved as decompiled.
// reconciled: R23 collision_result: normal -> plane.normal, unknown_30 -> plane.d, unknown_04 -> first_leaf/first_cluster, unknown_3c -> region_index, marker_index -> node_index, unknown_40 -> permutation_index (int16), unknown_48 -> plane_index, unknown_4d -> breakable_surface_index, unknown_4e -> collision_material_index

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "projectiles.h"
#include "physics.h"
#include "fn_effects.h"

extern float k_physics_gravity; // 0x0069c52c
extern float k_water_density;   // 0x006b8d7c
extern float k_air_density;     // 0x006b8d80


    // module unresolved (scenario/weather); returns medium (0 air, 1 water); UNSURE args

    // file header
extern uint8_t collision_test_movement_segment(uint32_t flags, real_point3d *origin,
    real_vector3d *delta, uint32_t exclude_object_index,
    collision_result *result); // 0x505880, this module

// One tick of a massless point mover driven by a PointPhysics tag: probes wind (full or "cheap"
// depending on flags_arg bit 0), applies medium-dependent gravity and drag, nudges velocity
// toward the wind vector, then resolves up to k_physics_collision_iterations bounces against the
// world -- each bounce scales the tangential velocity by (1 - surface_friction) and the normal
// component by -elasticity, and reports the closest hit's leaf/normal/material_type through the
// optional out parameters. Returns a point_physics_result_flags bit field.
uint32_t point_physics_tick(real_vector3d *velocity, uint32_t flags_arg, PointPhysics *definition,
    bsp_leaf_reference *out_leaf, uint32_t unused_param_4, real_point3d *position, real_vector3d *wind,
    real_vector3d *out_normal, int16_t *out_material_type, real radius, real dt)
{
    uint32_t result_flags;
    real gravity_scale, friction, density, radius_cubed;
    real_vector3d probed_wind;
    uint8_t in_water;
    uint32_t collision_flags;
    int16_t bounce;


    if (dt == 0.0f) {
        return 0;
    }

    radius_cubed = radius * radius * radius;

    {
        uint32_t wind_mode_mask = ((definition->flags & 0xff) >> 3 & 1);
        if ((definition->flags & 0x10) != 0) {
            wind_mode_mask |= 2;
        }

        if ((flags_arg & 1) == 0) {
            // FIXED (0x50b5cf..0x50b5e3): EAX = out_leaf (stack arg 2), stack (position, &probed, mask).
            in_water = ambient_color_marker_visible(out_leaf, position, &probed_wind, wind_mode_mask);
        } else {
            in_water = (uint8_t)(flags_arg >> 1) & 1;
            // FIXED (0x50b5a6..0x50b5be): AX = stack arg 3 (a weather row, the parameter the draft thought
            // unused), EDI = &probed, stack (position, mask).
            ambient_color_for_marker((int16_t)unused_param_4, position, (uint8_t)wind_mode_mask, &probed_wind);
        }
    }

    if (in_water == 0) {
        result_flags = _point_physics_in_air_bit;
        gravity_scale = definition->air_gravity_scale;
        friction = definition->air_friction;
        density = k_air_density;
    } else {
        result_flags = _point_physics_in_water_bit;
        gravity_scale = definition->water_gravity_scale;
        friction = definition->water_friction;
        density = k_water_density;
    }

    density += definition->mass_scale;
    friction = radius * radius * friction;
    density *= radius_cubed;

    {
        real wind_nudge_fraction = dt / density;

        if ((definition->flags & 0x20) != 0) {
            gravity_scale = 0.0f;
        }

        if (wind != (real_vector3d *)0 && density != 0.0f) {
            velocity->i += wind_nudge_fraction * wind->i;
            velocity->j += wind_nudge_fraction * wind->j;
            velocity->k += wind_nudge_fraction * wind->k;
        }

        velocity->k += k_physics_gravity * 30.0f * 30.0f * gravity_scale * dt;

        {
            real drag_lerp;
            if (density == 0.0f) {
                drag_lerp = (friction == 0.0f) ? 0.0f : 1.0f;
            } else {
                drag_lerp = wind_nudge_fraction * friction;
                if (drag_lerp < 0.0f) {
                    drag_lerp = 0.0f;
                } else if (drag_lerp > 1.0f) {
                    drag_lerp = 1.0f;
                }
            }
            velocity->i += (probed_wind.i - velocity->i) * drag_lerp;
            velocity->j += (probed_wind.j - velocity->j) * drag_lerp;
            velocity->k += (probed_wind.k - velocity->k) * drag_lerp;
        }
    }

    {
        // collision_test_movement_segment_flags (src/physics/collision_test_movement_segment.c,
        // this module): 0x20 structure_bsp, 0x40 water_surface, 0x80 nearby_objects. Not
        // reusable as named constants here (that enum is file-local), so kept as raw hex.
        uint32_t flags = definition->flags;
        if ((flags & 4) == 0) {
            collision_flags = 1;
        } else if ((flags_arg & 4) != 0) {
            collision_flags = 1;
        } else {
            collision_flags = 0x40 | 1; // water_surface + front-face test
        }
        if ((flags & 2) != 0 && (flags_arg & 4) == 0) {
            collision_flags |= 0x20; // structure_bsp
        }
        if ((flags & 1) != 0) {
            collision_flags |= 0x80 | 0x4200; // nearby_objects + UNSURE raw object-type-mask bits
        }
    }

    for (bounce = 0; bounce <= k_physics_collision_iterations - 1; bounce++) {
        real_vector3d delta;
        collision_result hit; // UNSURE: uninitialized on a first-iteration miss, see file header
        uint8_t collided;

        delta.i = dt * velocity->i;
        delta.j = dt * velocity->j;
        delta.k = dt * velocity->k;

        collided = collision_test_movement_segment(collision_flags, position, &delta, 0xffffffff, &hit);
        if (collided == 0) {
            // The no-collision exit. collision_test_movement_segment still filled hit.point with
            // the segment endpoint (see physics_model_contact's own "writes t = 1 with point =
            // origin + delta when nothing was hit" note), so this IS the ordinary position
            // advance for a tick that touched nothing -- the original's post-loop block, reached
            // only through this break.
            if (hit.leaf.leaf_index != -1) {
                *out_leaf = hit.leaf;
            }
            position->x = hit.point.x;
            position->y = hit.point.y;
            position->z = hit.point.z;
            return result_flags;
        }

        {
            real step_dt = (radius < 0.005f) ? radius : 0.005f;
            real dot_nv, tangential_j, tangential_k, normal_j_term, normal_k_term;

            if (hit.type == 0) {
                result_flags |= _point_physics_hit_water_surface_bit;
            } else if (hit.type == 2 || (hit.type == 3 && (definition->flags & 1) != 0)) {
                result_flags |= _point_physics_collided_bit;
            }

            if (out_normal != (real_vector3d *)0) {
                *out_normal = hit.plane.normal;
            }
            if (out_material_type != (int16_t *)0) {
                *out_material_type = hit.material_type;
            }

            dot_nv = hit.plane.normal.j * velocity->j + hit.plane.normal.k * velocity->k + hit.plane.normal.i * velocity->i;
            normal_j_term = dot_nv * hit.plane.normal.j;
            normal_k_term = dot_nv * hit.plane.normal.k;
            tangential_j = velocity->j - normal_j_term;
            tangential_k = velocity->k - normal_k_term;

            velocity->i = (1.0f - definition->surface_friction) * (velocity->i - hit.plane.normal.i * dot_nv) -
                hit.plane.normal.i * dot_nv * definition->elasticity;
            velocity->j = (1.0f - definition->surface_friction) * tangential_j - normal_j_term * definition->elasticity;
            velocity->k = (1.0f - definition->surface_friction) * tangential_k - normal_k_term * definition->elasticity;

            if (hit.leaf.leaf_index != -1) {
                *out_leaf = hit.leaf; // the original does NOT null-check param_3 here
            }

            position->x = hit.plane.normal.i * step_dt + hit.point.x;
            position->y = hit.plane.normal.j * step_dt + hit.point.y;
            position->z = hit.plane.normal.k * step_dt + hit.point.z;

            dt -= hit.t * dt;
            if (dt == 0.0f) {
                return result_flags;
            }
        }
    }

    // Loop exhausted after k_physics_collision_iterations bounces. The original returns straight
    // out of the `if (2 < sVar5)` test at the top of the while(true), so it does NOT re-run the
    // post-loop position write -- the last bounce already left *position at
    // hit.point + hit.plane.normal * step_dt.
    return result_flags;
}

#if 0
Original Ghidra decompilation (0x50b530):

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

uint FUN_0050b530(uint param_1,uint *param_2,int *param_3,undefined4 param_4,float *param_5,
                 float *param_6,float *param_7,undefined2 *param_8,float param_9,float param_10)

{
  float fVar1;
  float fVar2;
  float fVar3;
  char cVar4;
  short sVar5;
  uint uVar6;
  byte bVar7;
  uint uVar8;
  float *unaff_ESI;
  uint local_88;
  float local_84;
  float local_78;
  float local_74;
  float local_70;
  float local_6c;
  float local_64;
  float local_60;
  float local_5c;
  float local_58;
  float local_54;
  short local_50 [6];
  int local_44;
  int local_40;
  float local_3c;
  float local_38;
  float local_34;
  float local_30;
  float local_2c;
  float local_28;
  float local_24;
  undefined2 local_1c;

  if (param_10 == 0.0) {
    return 0;
  }
  fVar3 = (float)param_2[1];
  local_6c = param_9 * param_9 * param_9;
  uVar6 = (*param_2 & 0xff) >> 3 & 1;
  if ((*param_2 & 0x10) != 0) {
    uVar6 = uVar6 | 2;
  }
  if ((param_1 & 1) == 0) {
    bVar7 = FUN_0053f860(param_5,&local_78,uVar6);
  }
  else {
    bVar7 = (byte)(param_1 >> 1) & 1;
    FUN_0053f940(param_5);
  }
  if (bVar7 == 0) {
    local_88 = 1;
    fVar1 = (float)param_2[3];
    fVar2 = (float)param_2[9];
    local_84 = _DAT_006b8d80;
  }
  else {
    local_88 = 2;
    fVar1 = (float)param_2[2];
    fVar2 = (float)param_2[10];
    local_84 = _DAT_006b8d7c;
  }
  local_84 = local_84 + fVar3;
  fVar2 = param_9 * param_9 * fVar2;
  local_84 = local_84 * local_6c;
  fVar3 = param_10 / local_84;
  if ((*param_2 & 0x20) != 0) {
    fVar1 = 0.0;
  }
  if ((param_6 != (float *)0x0) && (local_84 != 0.0)) {
    *unaff_ESI = fVar3 * *param_6 + *unaff_ESI;
    unaff_ESI[1] = fVar3 * param_6[1] + unaff_ESI[1];
    unaff_ESI[2] = fVar3 * param_6[2] + unaff_ESI[2];
  }
  fVar1 = _DAT_0069c52c * 30.0 * 30.0 * fVar1 * param_10 + unaff_ESI[2];
  unaff_ESI[2] = fVar1;
  if (local_84 == 0.0) {
    if (fVar2 == 0.0) {
      fVar3 = 0.0;
      goto LAB_0050b726;
    }
  }
  else {
    fVar3 = fVar3 * fVar2;
    if (fVar3 < 0.0) {
      fVar3 = 0.0;
      goto LAB_0050b726;
    }
    if (fVar3 <= 1.0) goto LAB_0050b726;
  }
  fVar3 = 1.0;
LAB_0050b726:
  *unaff_ESI = (local_78 - *unaff_ESI) * fVar3 + *unaff_ESI;
  unaff_ESI[1] = (local_74 - unaff_ESI[1]) * fVar3 + unaff_ESI[1];
  unaff_ESI[2] = (local_70 - fVar1) * fVar3 + fVar1;
  uVar6 = *param_2;
  if (((uVar6 & 4) == 0) || (uVar8 = 0x41, (param_1 & 4) != 0)) {
    uVar8 = 1;
  }
  if (((uVar6 & 2) != 0) && ((param_1 & 4) == 0)) {
    uVar8 = uVar8 | 0x20;
  }
  if ((uVar6 & 1) != 0) {
    uVar8 = uVar8 | 0x4280;
  }
  sVar5 = 0;
  while( true ) {
    if (2 < sVar5) {
      return local_88;
    }
    local_5c = param_10 * *unaff_ESI;
    local_58 = param_10 * unaff_ESI[1];
    local_54 = param_10 * unaff_ESI[2];
    cVar4 = FUN_00505880(uVar8,param_5,&local_5c,0xffffffff,local_50);
    if (cVar4 == '\0') break;
    fVar3 = param_9;
    if (0.005 < param_9) {
      fVar3 = 0.005;
    }
    if (local_50[0] == 0) {
      local_88 = local_88 | 8;
    }
    else if ((local_50[0] == 2) || ((local_50[0] == 3 && ((*param_2 & 1) != 0)))) {
      local_88 = local_88 | 4;
    }
    if (param_7 != (float *)0x0) {
      *param_7 = local_2c;
      param_7[1] = local_28;
      param_7[2] = local_24;
    }
    if (param_8 != (undefined2 *)0x0) {
      *param_8 = local_1c;
    }
    fVar1 = local_28 * unaff_ESI[1] + local_24 * unaff_ESI[2] + local_2c * *unaff_ESI;
    local_74 = fVar1 * local_28;
    local_70 = fVar1 * local_24;
    local_64 = unaff_ESI[1] - local_74;
    local_60 = unaff_ESI[2] - local_70;
    *unaff_ESI = (1.0 - (float)param_2[0xb]) * (*unaff_ESI - local_2c * fVar1) -
                 local_2c * fVar1 * (float)param_2[0xc];
    unaff_ESI[1] = (1.0 - (float)param_2[0xb]) * local_64 - local_74 * (float)param_2[0xc];
    unaff_ESI[2] = (1.0 - (float)param_2[0xb]) * local_60 - local_70 * (float)param_2[0xc];
    if (local_44 != -1) {
      *param_3 = local_44;
      param_3[1] = local_40;
    }
    sVar5 = sVar5 + 1;
    *param_5 = local_2c * fVar3 + local_38;
    param_5[1] = local_28 * fVar3 + local_34;
    param_5[2] = local_24 * fVar3 + local_30;
    param_10 = param_10 - local_3c * param_10;
    if (param_10 == 0.0) {
      return local_88;
    }
  }
  if (local_44 != -1) {
    *param_3 = local_44;
    param_3[1] = local_40;
  }
  *param_5 = local_38;
  param_5[1] = local_34;
  param_5[2] = local_30;
  return local_88;
}
#endif
