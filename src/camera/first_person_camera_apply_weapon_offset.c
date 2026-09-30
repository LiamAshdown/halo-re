// first_person_camera_apply_weapon_offset  (Ghidra: FUN_00447290; renamed for this rewrite)
// address 0x447290, size 210 bytes
// name confidence: 0.5   rewrite confidence: 0.85
// evidence: confirmed instruction-by-instruction against objdump. Calls unit_get_camera_position
// (0x568f80, ECX -> unit, EDI -> out; established by src/game/chimera__spectate_fp_camera_position.c)
// with EDI aliased to this function's own output pointer, so that call seeds *position with the
// unit's base camera position before this function adds the track offset on top. Calls
// unit_get_camera_properties (0x447110) and first_person_camera_track_offset (0x447190), both
// declared in this module. The pitch angle feeding the track comes from asin(aiming_vector.k):
// 0x628630 is the CRT _CIasin (review fix, phase 4 gate: it was declared with a spurious
// out pointer that is really the early push of the next call's argument).
// register convention: EAX -> position (also the seed/output of unit_get_camera_position), EBX
// -> unit, ESI -> aiming_direction (caller-owned scratch this function fills and then reads back).

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "units.h"
#include "camera.h"
#include "fn_camera.h"

extern data_array *object_data; // 0x008603b0

// sqrt/fabs are single x87 instructions (FSQRT/FABS) in the original code, which Ghidra
// renders as the pseudo-functions SQRT()/ABS(); declared locally instead of via <math.h>
// because -I types shadows that header name with types/math.h.
extern double sqrt(double x); // FSQRT
extern double fabs(double x); // FABS

extern void unit_get_camera_position(datum_index unit, real_point3d *out); // 0x568f80
    // blam-cc: ECX -> unit, EDI -> out

// blam-cc: EAX -> unit (0x447110, this module)


// blam-cc: ECX -> properties; angle, out = stack (0x447190, this module)


// MSVC 7.1 CRT _CIasin: operand and result on the x87 stack, no stack arguments (0x628630 is
// sub esp,0xc / fst QWORD [esp] / call / call / ret). The `push edx` at 0x4472e0 right before
// this call is the out pointer of first_person_camera_track_offset, pushed early.
extern double asin(double x); // 0x628630

// blam-cc: EAX -> position, EBX -> unit, ESI -> aiming_direction
// Advances the first person camera position by the unit's camera-track offset: samples the
// track at the pitch angle implied by the unit's current aiming direction, rotates the sampled
// x/y offset into the aiming direction's horizontal frame, and adds it (plus the unrotated z) to
// *position, which unit_get_camera_position has already seeded with the unit's base camera
// position.
void first_person_camera_apply_weapon_offset(real_point3d *position, datum_index unit,
    real_vector3d *aiming_direction)
{
    object *unit_object;
    unit_camera_properties *properties;
    unit_data *unit_extension;
    Vector3D track_offset;
    double pitch_angle;
    float horizontal_i, horizontal_j;
    float magnitude;

    unit_object = ((object_header *)object_data->data)[(uint16_t)unit].data;
    properties = unit_get_camera_properties(unit);
    unit_get_camera_position(unit, position);

    unit_extension = (unit_data *)((uint8_t *)unit_object + k_unit_data_offset);
    *aiming_direction = unit_extension->aiming_vector;

    pitch_angle = asin((double)aiming_direction->k);
    first_person_camera_track_offset(properties, (float)pitch_angle, &track_offset);

    horizontal_i = aiming_direction->i;
    horizontal_j = aiming_direction->j;
    magnitude = (float)sqrt((double)(horizontal_i * horizontal_i + horizontal_j * horizontal_j));
    if (0.0001 <= fabs((double)magnitude)) {
        magnitude = 1.0f / magnitude;
        horizontal_i = magnitude * horizontal_i;
        horizontal_j = magnitude * horizontal_j;
    }

    position->x = track_offset.i * horizontal_i + track_offset.j * horizontal_j + position->x;
    position->y = (track_offset.i * horizontal_j - track_offset.j * horizontal_i) + position->y;
    position->z = track_offset.k + position->z;
}

#if 0
Original Ghidra decompilation (0x447290):

void FUN_00447290(void)

{
  float fVar1;
  float fVar2;
  int iVar3;
  float fVar4;
  float *in_EAX;
  uint unaff_EBX;
  float *unaff_ESI;
  float10 fVar5;
  float local_c;
  float local_8;
  float local_4;

  iVar3 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (unaff_EBX & 0xffff) * 0xc);
  FUN_00447110();
  unit_get_camera_position();
  *unaff_ESI = *(float *)(iVar3 + 0x23c);
  unaff_ESI[1] = *(float *)(iVar3 + 0x240);
  unaff_ESI[2] = *(float *)(iVar3 + 0x244);
  fVar5 = (float10)FUN_00628630(&local_c);
  FUN_00447190((float)fVar5);
  fVar1 = *unaff_ESI;
  fVar2 = unaff_ESI[1];
  fVar4 = SQRT(fVar1 * fVar1 + fVar2 * fVar2);
  if (0.0001 <= ABS(fVar4)) {
    fVar4 = 1.0 / fVar4;
    fVar1 = fVar4 * fVar1;
    fVar2 = fVar4 * fVar2;
  }
  *in_EAX = local_c * fVar1 + local_8 * fVar2 + *in_EAX;
  in_EAX[1] = (local_c * fVar2 - local_8 * fVar1) + in_EAX[1];
  in_EAX[2] = local_4 + in_EAX[2];
  return;
}
#endif
