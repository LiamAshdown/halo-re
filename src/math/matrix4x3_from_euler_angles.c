// matrix4x3_from_euler_angles  (Ghidra: matrix4x3_from_euler_angles, already named)
// address 0x4cba10, size 179 bytes
// name confidence: 0.55   rewrite confidence: 0.7
// evidence: types/math.h real_euler_angles3d section ("takes (yaw, pitch, roll) in that
//   argument order... param_2 drives in_EAX[7] = -sin(param_2), the pitch term of the up row").
//   Standard yaw-pitch-roll composed rotation matrix.
// register convention: output matrix in EAX (in_EAX); yaw, pitch, roll as the three recognized
//   stack parameters, in that order.
//   // blam-cc: EAX -> out, stack -> (yaw, pitch, roll)

#include "tags.h"
#include "math.h"

// cos/sin are single x87 FCOS/FSIN instructions in the original code (Ghidra's fcos()/fsin()
// pseudo-calls); declared locally instead of via <math.h> because -I types shadows that header
// name with types/math.h.
extern double cos(double x);
extern double sin(double x);

// Builds a matrix4x3 rotation from three Euler angles.
void matrix4x3_from_euler_angles(real_matrix4x3 *out, real yaw, real pitch, real roll)
{
    real cy, sy;   // cos/sin(roll)
    real cp, sp;   // cos/sin(pitch)
    real cyaw, syaw; // cos/sin(yaw)

    cy = (real)cos((double)roll);
    out->scale = 1.0f;
    out->position.x = 0.0f;
    out->position.y = 0.0f;
    out->position.z = 0.0f;
    sy = (real)sin((double)roll);
    cp = (real)cos((double)pitch);
    sp = (real)sin((double)pitch);
    cyaw = (real)cos((double)yaw);
    syaw = (real)sin((double)yaw);

    out->forward.i = cyaw * cp;
    out->forward.j = syaw * cy - sp * sy * cyaw;
    out->forward.k = syaw * sy + sp * cy * cyaw;
    out->left.i = -(syaw * cp);
    out->left.j = cyaw * cy + sp * sy * syaw;
    out->left.k = cyaw * sy - sp * cy * syaw;
    out->up.i = -sp;
    out->up.j = -(cp * sy);
    out->up.k = cp * cy;
}

#if 0
Original Ghidra decompilation (0x4cba10):

void matrix4x3_from_euler_angles(float param_1,float param_2,float param_3)

{
  float fVar1;
  undefined4 *in_EAX;
  float10 fVar2;
  float10 fVar3;
  float10 fVar4;
  float10 fVar5;
  float10 fVar6;

  fVar2 = (float10)fcos((float10)param_3);
  *in_EAX = 0x3f800000;
  in_EAX[10] = 0;
  in_EAX[0xb] = 0;
  in_EAX[0xc] = 0;
  fVar3 = (float10)fsin((float10)param_3);
  fVar4 = (float10)fcos((float10)param_2);
  fVar1 = (float)fVar4;
  fVar4 = (float10)fsin((float10)param_2);
  fVar5 = (float10)fcos((float10)param_1);
  fVar6 = (float10)fsin((float10)param_1);
  in_EAX[1] = (float)(fVar5 * (float10)fVar1);
  in_EAX[2] = (float)(fVar6 * fVar2 - (float10)(float)(fVar4 * fVar3) * fVar5);
  in_EAX[3] = (float)(fVar6 * fVar3 + fVar4 * fVar2 * fVar5);
  in_EAX[4] = (float)-(fVar6 * (float10)fVar1);
  in_EAX[5] = (float)(fVar5 * fVar2 + (float10)(float)(fVar4 * fVar3) * fVar6);
  in_EAX[6] = (float)(fVar5 * fVar3 - fVar4 * fVar2 * fVar6);
  in_EAX[7] = (float)-fVar4;
  in_EAX[8] = (float)-((float10)fVar1 * fVar3);
  in_EAX[9] = (float)((float10)fVar1 * fVar2);
  return;
}
#endif
