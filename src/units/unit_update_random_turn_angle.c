// unit_update_random_turn_angle  (Ghidra: FUN_00570840; renamed from the phase2 proposal)
// address 0x570840, size 644 bytes
// name confidence: 0.45 (phase2 proposal at 0.45, matches functions.md summary)
// rewrite confidence: 0.9 (checked against objdump 0x570840..0x570ac3)
// evidence: types/units.h unit_data.idle_turn_angle (0x414), .idle_turn_offset (0x418),
//   .actor_index (0x1f4, decimal 500); math.h global_forward3d_pointer (0x00696718); companion
//   function unit_initialize_random_turn_angle (0x570650, this batch) uses the same fields and
//   bound (0.43633232 rad = 25 deg there, matching the 0.7853982/4 quadrant math here).
// register convention: unit object index in EAX (in_EAX); an output vector pointer in EDI
//   (unaff_EDI).
//   // blam-cc: EAX -> object_index, EDI -> out_axis
// UNSURE: vector3d_rotate_about_axis's v and axis pointers are register-only at this call site;
//   v is guessed as the unit's own forward vector and axis as out_axis (the world-forward
//   constant for a non-actor unit, or whatever the caller already had in *out_axis otherwise).

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "units.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern data_array *object_data; // 0x008603b0
extern random_seed random_seed_global;   // 0x00719cd0
extern real_vector3d *global_forward3d_pointer; // 0x00696718

extern uint8_t actor_resolve_wander_or_look_direction(datum_index actor_index, real_vector3d *out_direction); // 0x4287a0, EAX, ECX
extern real_vector3d *global_up3d_pointer; // 0x00696720
extern double cos(double x); // fcos
extern double sin(double x); // fsin
extern void vector3d_rotate_about_axis(real_vector3d *v, real_vector3d *axis, real sin_angle,
                                        real cos_angle); // 0x4cd820

// Randomly wanders the unit's idle look/turn angle (idle_turn_angle) each tick within clamped
// bounds around idle_turn_offset, then applies the resulting rotation.
void unit_update_random_turn_angle(uint32_t object_index, real_vector3d *out_axis)
{
    object *obj = ((object_header *)object_data->data)[object_index & 0xffff].data;
    unit_data *unit = (unit_data *)((uint8_t *)obj + k_unit_data_offset);
    uint8_t is_actor_controlled = 0;
    float yaw_low, yaw_high;
    float pitch_low, pitch_high;
    float delta;

    // 0x570867: EAX = the unit's actor, ECX = out (the unit's desired facing, EDI)
    if (unit->actor_index == k_datum_index_none || actor_resolve_wander_or_look_direction(unit->actor_index, out_axis) == 0) {
        *out_axis = *global_forward3d_pointer;
    } else {
        is_actor_controlled = 1;
    }

    yaw_low = 1.0f;
    yaw_high = 1.0f;
    if (is_actor_controlled) {
        float a = (0.7853982f - unit->idle_turn_angle) * 4.2441316f;
        yaw_low = (a < 1.0f) ? a : 1.0f;
        float b = (unit->idle_turn_angle + 0.7853982f) * 4.2441316f;
        yaw_high = (b < 1.0f) ? b : 1.0f;
    }

    {
        float a = (0.20943952f - unit->idle_turn_offset) * 15.915494f;
        if (a < yaw_low) yaw_low = a;
        float b = (unit->idle_turn_offset + 0.20943952f) * 15.915494f;
        if (b < yaw_high) yaw_high = b;
    }
    pitch_low = yaw_low;
    pitch_high = yaw_high;

    if (pitch_high <= pitch_low) {
        if (pitch_high >= -1.0f) {
            float clamped = (pitch_high < 1.0f) ? pitch_high : 1.0f;
            random_seed_global = random_seed_global * 0x19660d + 0x3c6ef35f;
            delta = (0.02094395f - clamped * -0.02094395f) *
                    (float)(random_seed_global >> 0x10) * 1.5259022e-05f + clamped * -0.02094395f;
        } else {
            delta = 0.02094395f;
        }
    } else if (pitch_low >= -1.0f) {
        float clamped = (pitch_low < 1.0f) ? pitch_low : 1.0f;
        random_seed_global = random_seed_global * 0x19660d + 0x3c6ef35f;
        delta = (float)(random_seed_global >> 0x10) * 1.5259022e-05f *
                (clamped * 0.02094395f - -0.02094395f) - 0.02094395f;
    } else {
        delta = -0.02094395f;
    }

    delta += unit->idle_turn_offset;
    unit->idle_turn_offset = delta;
    delta += unit->idle_turn_angle;
    unit->idle_turn_angle = delta;

    if (delta < -3.1415927f) {
        unit->idle_turn_angle = delta + 6.2831855f;
    } else if (delta > 3.1415927f) {
        unit->idle_turn_angle = delta - 6.2831855f;
    }

    {
        float c = (float)cos((double)unit->idle_turn_angle);
        float s = (float)sin((double)unit->idle_turn_angle);
        // 0x570a94: EAX = out (EDI), ECX = *global_up3d_pointer -- the draft rotated the OBJECT's forward vector
        //   about out, bending the unit's orientation every tick
        vector3d_rotate_about_axis(out_axis, global_up3d_pointer, s, c);
    }
}

#if 0
Original Ghidra decompilation (0x570840):

void FUN_00570840(void)

{
  int iVar1;
  float fVar2;
  float fVar3;
  bool bVar4;
  undefined *puVar5;
  char cVar6;
  uint in_EAX;
  undefined4 *unaff_EDI;
  float10 fVar7;
  float10 fVar8;
  float local_8;

  iVar1 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (in_EAX & 0xffff) * 0xc);
  bVar4 = false;
  if ((*(int *)(iVar1 + 500) == -1) || (cVar6 = FUN_004287a0(), cVar6 == '\0')) {
    puVar5 = PTR_DAT_00696718;
    *unaff_EDI = *(undefined4 *)PTR_DAT_00696718;
    unaff_EDI[1] = *(undefined4 *)(puVar5 + 4);
    unaff_EDI[2] = *(undefined4 *)(puVar5 + 8);
  }
  else {
    bVar4 = true;
  }
  local_8 = 1.0;
  fVar2 = 1.0;
  if (bVar4) {
    fVar2 = (0.7853982 - *(float *)(iVar1 + 0x414)) * 4.2441316;
    if (fVar2 < 1.0) {
      local_8 = fVar2;
    }
    fVar3 = (*(float *)(iVar1 + 0x414) + 0.7853982) * 4.2441316;
    fVar2 = 1.0;
    if (fVar3 < 1.0) {
      fVar2 = fVar3;
    }
  }
  fVar3 = (0.20943952 - *(float *)(iVar1 + 0x418)) * 15.915494;
  if (fVar3 < local_8) {
    local_8 = fVar3;
  }
  fVar3 = (*(float *)(iVar1 + 0x418) + 0.20943952) * 15.915494;
  if (fVar3 < fVar2) {
    fVar2 = fVar3;
  }
  if (fVar2 <= local_8) {
    if (-1.0 <= fVar2) {
      if (1.0 <= fVar2) {
        fVar2 = 1.0;
      }
      random_seed_global = random_seed_global * 0x19660d + 0x3c6ef35f;
      fVar2 = (0.02094395 - fVar2 * -0.02094395) *
              (float)(random_seed_global >> 0x10) * 1.5259022e-05 + fVar2 * -0.02094395;
    }
    else {
      fVar2 = 0.02094395;
    }
  }
  else if (-1.0 <= local_8) {
    if (1.0 <= local_8) {
      local_8 = 1.0;
    }
    random_seed_global = random_seed_global * 0x19660d + 0x3c6ef35f;
    fVar2 = (float)(random_seed_global >> 0x10) * 1.5259022e-05 *
            (local_8 * 0.02094395 - -0.02094395) - 0.02094395;
  }
  else {
    fVar2 = -0.02094395;
  }
  fVar2 = fVar2 + *(float *)(iVar1 + 0x418);
  *(float *)(iVar1 + 0x418) = fVar2;
  fVar2 = fVar2 + *(float *)(iVar1 + 0x414);
  *(float *)(iVar1 + 0x414) = fVar2;
  if (-3.1415927 <= fVar2) {
    if (3.1415927 < fVar2) {
      *(float *)(iVar1 + 0x414) = fVar2 - 6.2831855;
    }
  }
  else {
    *(float *)(iVar1 + 0x414) = fVar2 + 6.2831855;
  }
  fVar7 = (float10)fcos((float10)*(float *)(iVar1 + 0x414));
  fVar8 = (float10)fsin((float10)*(float *)(iVar1 + 0x414));
  vector3d_rotate_about_axis((float)fVar8,(float)fVar7);
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
