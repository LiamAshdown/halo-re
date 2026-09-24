// physics_sweep_capsule_step  (Ghidra: FUN_00506fb0; renamed)
// address 0x506fb0, size 277 bytes
// name confidence: 0.3   rewrite confidence: 0.3
// evidence: out/phase4/physics_functions.md summary ("Sweeps a capsule-approximated movement
//   step through the world, sliding along any surfaces it touches"); calls
//   physics_model_build_from_sphere_query (0x506440, this module, higher half, whose confirmed
//   signature is (flags, center, radius, x_offset, y_offset, exclude_object_index, model)) with
//   a sphere centred at the midpoint of the step and radius = half the step length plus the
//   caller's radius and margin -- the standard "swept sphere approximates a capsule" trick.
// register convention: Ghidra recognizes EBX -> out_velocity, ESI -> delta, EDI -> origin as
//   unaff_* (all real_vector3d/real_point3d *), plus its own four ordinary parameters (flags,
//   radius, margin, out_position). physics_model_build_from_sphere_query needs two more
//   arguments (exclude_object_index, physics_model *) that never appear in this function's own
//   body at all -- they must be additional hidden-register inputs, still live at the call site,
//   that this rewrite adds as leading parameters per the EAX/ECX/EDX/EBX/ESI/EDI ordering.
//   // blam-cc: EAX -> model, ECX -> exclude_object_index, EBX -> out_velocity, ESI -> delta,
//   //          EDI -> origin, stack -> flags, radius, margin, out_position
// UNSURE: which of EAX/ECX actually carries model vs exclude_object_index; Ghidra shows neither
//   register at all, so this is inferred purely from physics_model_build_from_sphere_query's own
//   parameter list and cannot be confirmed against the machine code here.
// UNSURE: the call to physics_model_slide_along_contacts() also has zero visible arguments;
//   this rewrite forwards the same start_position/delta/model/out_position/contact_scratch this
//   function itself would need, on the assumption they stay live in the same registers/stack
//   slots physics_model_build_from_sphere_query itself was just given.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "physics.h"

extern double sqrt(double x); // SQRT is a single x87 FSQRT instruction

extern uint8_t physics_model_build_from_sphere_query(uint32_t flags, real_point3d *center,
    float radius, float x_offset, float y_offset, uint32_t exclude_object_index,
    physics_model *model); // 0x506440, this module (higher half)
extern void physics_model_slide_along_contacts(real_point3d *start_position, real_vector3d *delta,
    physics_model *model, real_point3d *out_position, uint32_t param_4, int16_t max_iterations,
    physics_model_contact *contact_scratch); // 0x5067b0, this module (higher half)

// Approximates a capsule sweep (origin, delta, radius, margin) as a single swept-sphere query
// centred at the step's midpoint. If that query finds anything, resolves the step by sliding
// along the contacts (physics_model_slide_along_contacts) and returns its result. Otherwise the
// step is unobstructed: *out_position = origin + delta and *out_velocity = delta, unchanged.
uint8_t physics_sweep_capsule_step(physics_model *model, uint32_t exclude_object_index,
    real_vector3d *out_velocity, real_vector3d *delta, real_point3d *origin, uint32_t flags,
    float radius, float margin, real_point3d *out_position)
{
    real_point3d sweep_center;
    float delta_length;
    float sweep_radius;

    sweep_center.x = delta->i * 0.5f + origin->x;
    sweep_center.y = delta->j * 0.5f + origin->y;
    sweep_center.z = delta->k * 0.5f + origin->z + radius * 0.5f;

    delta_length = (float)sqrt((double)(delta->i * delta->i + delta->k * delta->k + delta->j * delta->j));
    sweep_radius = delta_length * 0.5f + radius * 0.5f + margin;

    if (physics_model_build_from_sphere_query(flags, &sweep_center, sweep_radius, radius, margin,
            exclude_object_index, model)) {
        physics_model_slide_along_contacts(origin, delta, model, out_position, 0, 3 /* UNSURE:
            max_iterations, presumably k_physics_collision_iterations */, (physics_model_contact *)0
            /* UNSURE: contact scratch buffer, not visible in this function's own frame */);
        return 1;
    }

    out_position->x = origin->x + delta->i;
    out_position->y = origin->y + delta->j;
    out_position->z = origin->z + delta->k;
    *out_velocity = *delta;
    return 0;
}

#if 0
Original Ghidra decompilation (0x506fb0):

/* WARNING: Function: __chkstk replaced with injection: alloca_probe */

undefined4 FUN_00506fb0(undefined4 param_1,float param_2,float param_3,float *param_4)

{
  char cVar1;
  undefined4 uVar2;
  float *unaff_EBX;
  float *unaff_ESI;
  float *unaff_EDI;
  float local_ac14;
  float local_ac10;
  float local_ac0c;
  undefined4 uStack_4;

  uStack_4 = 0x506fba;
  local_ac14 = *unaff_ESI * 0.5 + *unaff_EDI;
  local_ac10 = unaff_ESI[1] * 0.5 + unaff_EDI[1];
  local_ac0c = unaff_ESI[2] * 0.5 + unaff_EDI[2] + param_2 * 0.5;
  cVar1 = FUN_00506440(param_1,&local_ac14,
                       SQRT(*unaff_ESI * *unaff_ESI +
                            unaff_ESI[2] * unaff_ESI[2] + unaff_ESI[1] * unaff_ESI[1]) * 0.5 +
                       param_2 * 0.5 + param_3,param_2,param_3);
  if (cVar1 != '\0') {
    uVar2 = FUN_005067b0();
    return uVar2;
  }
  *param_4 = *unaff_EDI + *unaff_ESI;
  param_4[1] = unaff_EDI[1] + unaff_ESI[1];
  param_4[2] = unaff_EDI[2] + unaff_ESI[2];
  *unaff_EBX = *unaff_ESI;
  unaff_EBX[1] = unaff_ESI[1];
  unaff_EBX[2] = unaff_ESI[2];
  return 0;
}
#endif
