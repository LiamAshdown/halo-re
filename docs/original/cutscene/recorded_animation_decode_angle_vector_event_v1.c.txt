// recorded_animation_decode_angle_vector_event_v1  (Ghidra: FUN_0044a790; renamed per types
// notes: "the v1 angle vector event handler (it is not a keyframe)")
// address 0x44a790, size 133 bytes
// name confidence: 0.8 (symbols/agent_phase4_cutscene.txt)   rewrite confidence: 0.85
// evidence: out/phase4/cutscene_types_notes.md "0x44a790 angles" among the v1 handler bodies,
// and "The angle types 0x10, 0x11, 0x12 and 0x16 write all three vectors, and 0x13/0x14/0x15
// each skip one." types/cutscene.h recorded_animation_angle_vector_set_event_v1 (header +
// real_euler_angles2d angles at 0x04) and recorded_animation_v1_event_proc(control, event,
// cursor). Unlike the compressed codec this reads real radians directly, no +-1000 fixed-point
// scale.
// register convention: cdecl stack parameters (control, event, cursor), matching Ghidra's own
// param_1..param_3 recovery -- no register-passed (in_EAX-style) arguments appear in the body.
// blam-cc: stack -> (control, event, cursor).

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "units.h"
#include "cutscene.h"

// cos/sin are single x87 FCOS/FSIN instructions in the original code (Ghidra's fcos()/fsin()
// pseudo-calls); declared locally instead of via <math.h> because -I types shadows that header
// name with types/math.h.
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern double cos(double x);
extern double sin(double x);

// blam-cc: stack -> (control, event, cursor)
// Builds a unit direction vector from an uncompressed (radians) yaw/pitch pair and writes it
// into every direction vector of control the event type doesn't explicitly exclude: type 0x15
// skips facing, 0x14 skips aiming, 0x13 skips looking, every other type writes all three.
// Advances *cursor by the fixed 0x0c event size.
void recorded_animation_decode_angle_vector_event_v1(unit_control_data *control,
    recorded_animation_angle_vector_set_event_v1 *event, uint8_t **cursor)
{
    double cos_pitch;
    double cos_yaw;
    float i;
    float j;
    float k;

    cos_pitch = cos((double)event->angles.pitch);
    cos_yaw = cos((double)event->angles.yaw);
    i = (float)(cos_yaw * cos_pitch);
    j = (float)(sin((double)event->angles.yaw) * cos_pitch);
    k = (float)sin((double)event->angles.pitch);

    if (event->header.type != 0x15) {
        control->facing_vector.i = i;
        control->facing_vector.j = j;
        control->facing_vector.k = k;
    }
    if (event->header.type != 0x14) {
        control->aiming_vector.i = i;
        control->aiming_vector.j = j;
        control->aiming_vector.k = k;
    }
    if (event->header.type != 0x13) {
        control->looking_vector.i = i;
        control->looking_vector.j = j;
        control->looking_vector.k = k;
    }
    *cursor += 0xc;
}

#if 0
Original Ghidra decompilation (0x44a790):

void FUN_0044a790(int param_1,short *param_2,int *param_3)

{
  float fVar1;
  float fVar2;
  float fVar3;
  float10 fVar4;
  float10 fVar5;

  fVar4 = (float10)fcos((float10)*(float *)(param_2 + 4));
  fVar5 = (float10)fcos((float10)*(float *)(param_2 + 2));
  fVar1 = (float)(fVar5 * fVar4);
  fVar5 = (float10)fsin((float10)*(float *)(param_2 + 2));
  fVar2 = (float)(fVar5 * fVar4);
  fVar4 = (float10)fsin((float10)*(float *)(param_2 + 4));
  fVar3 = (float)fVar4;
  if (*param_2 != 0x15) {
    *(float *)(param_1 + 0x1c) = fVar1;
    *(float *)(param_1 + 0x20) = fVar2;
    *(float *)(param_1 + 0x24) = fVar3;
  }
  if (*param_2 != 0x14) {
    *(float *)(param_1 + 0x28) = fVar1;
    *(float *)(param_1 + 0x2c) = fVar2;
    *(float *)(param_1 + 0x30) = fVar3;
  }
  if (*param_2 != 0x13) {
    *(float *)(param_1 + 0x34) = fVar1;
    *(float *)(param_1 + 0x38) = fVar2;
    *(float *)(param_1 + 0x3c) = fVar3;
  }
  *param_3 = *param_3 + 0xc;
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
