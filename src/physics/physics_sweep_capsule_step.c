// physics_sweep_capsule_step  (Ghidra: FUN_00506fb0; renamed)
// address 0x506fb0, size 277 bytes
// name confidence: 0.4   rewrite confidence: 0.9 (rewritten from objdump -d 0x506fb0..0x5070c4
//   in step 1; the earlier draft guessed the registers and had them wrong)
// evidence: the only caller is the biped movement solver 0x55efd0 (0x55f766). It approximates
//   a capsule move as one sphere query centred on the midpoint of the step, lifted by half the
//   pill height: radius = |delta| / 2 + pill_height / 2 + pill_radius (0x672abc is 0.5).
//   physics_model_build_from_sphere_query (0x506440) gets (flags, &center, radius, pill_height,
//   pill_radius, exclude_object_index, &model). When it finds anything, the result and the
//   contact count come from physics_model_slide_along_contacts (0x5067b0: EAX = origin, stack
//   delta, &model, out_position, out_velocity, max_contacts, contacts). Otherwise the move is
//   free: out_position = origin + delta, out_velocity = delta, and 0 contacts.
// register convention: EDI origin, ESI delta, EBX out_velocity, ECX the object to leave out,
//   then six stack arguments; the contact count comes back in AX.
//   // blam-cc: EDI -> origin, ESI -> delta, EBX -> out_velocity, ECX -> exclude_object_index,
//   //          stack -> flags, pill_height, pill_radius, out_position, max_contacts, contacts

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "physics.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern double sqrt(double x); // SQRT is a single x87 FSQRT instruction

extern uint8_t physics_model_build_from_sphere_query(uint32_t flags, real_point3d *center,
    float radius, float x_offset, float y_offset, uint32_t exclude_object_index,
    physics_model *model); // 0x506440
extern int16_t physics_model_slide_along_contacts(real_point3d *start_position, real_vector3d *delta,
    physics_model *model, real_point3d *out_position, real_vector3d *out_velocity, int16_t max_contacts,
    physics_model_contact *contacts); // 0x5067b0, blam-cc: EAX -> start_position, stack -> the rest

// blam-cc: EDI -> origin, ESI -> delta, EBX -> out_velocity, ECX -> exclude_object_index,
//          stack -> flags, pill_height, pill_radius, out_position, max_contacts, contacts
// Moves a pill of the given height and radius from origin by delta through the world,
// sliding along what it touches; returns how many contacts were recorded.
int16_t physics_sweep_capsule_step(real_point3d *origin, real_vector3d *delta, real_vector3d *out_velocity,
    uint32_t exclude_object_index, uint32_t flags, float pill_height, float pill_radius,
    real_point3d *out_position, int16_t max_contacts, physics_model_contact *contacts)
{
    physics_model model;
    real_point3d center;
    float radius;

    center.x = delta->i * 0.5f + origin->x;
    center.y = delta->j * 0.5f + origin->y;
    center.z = delta->k * 0.5f + origin->z + pill_height * 0.5f;
    radius = (float)sqrt((double)(delta->i * delta->i + delta->j * delta->j + delta->k * delta->k)) * 0.5f +
             pill_height * 0.5f + pill_radius;

    if (physics_model_build_from_sphere_query(flags, &center, radius, pill_height, pill_radius,
                                              exclude_object_index, &model)) {
        return physics_model_slide_along_contacts(origin, delta, &model, out_position, out_velocity,
                                                  max_contacts, contacts);
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
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
