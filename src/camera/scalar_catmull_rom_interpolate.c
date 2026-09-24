// scalar_catmull_rom_interpolate  (Ghidra: scalar_catmull_rom_interpolate, already named)
// address 0x447000, size 121 bytes
// name confidence: 0.9   rewrite confidence: 0.95
// evidence: a Newton forward-difference cubic through four uniformly-spaced samples (value0 at
// time0, then time0+dt, +2dt, +3dt), evaluated at an arbitrary time; used by
// vector3d_catmull_rom_interpolate (0x447080) and, through it, by camera_evaluate_animation_offset
// (0x447190) to sample marker animation offsets between keyframes.
// register convention: __cdecl, all seven parameters on the stack.

#include "tags.h"
#include "memory.h"
#include "camera.h"

// blam-cc: __cdecl, all parameters on the stack
// Evaluates the cubic that passes through value0, value1, value2 and value3 at times time0,
// time0+dt, time0+2*dt and time0+3*dt (a Catmull-Rom style stencil), at the given time. Callers
// use it to interpolate between value0 and value1, with value2 and value3 shaping the curve's
// tangent.
double scalar_catmull_rom_interpolate(float value0, float value1, float value2, float value3,
    float time0, float dt, float time)
{
    float second_difference;

    second_difference = (value2 - value1) - (value1 - value0);
    return (double)(((time - time0) / dt) *
                     (((time - (time0 + dt)) *
                       ((((value3 - value2) - (value2 - value1)) - second_difference) *
                        (time - (time0 + dt + dt)) /
                        (dt * 3.0)
                        + second_difference)) /
                      (dt + dt)
                      + (value1 - value0))
                     + value0);
}

#if 0
Original Ghidra decompilation (0x447000):

double __cdecl
scalar_catmull_rom_interpolate
          (float param_1,float param_2,float param_3,float param_4,float param_5,float param_6,
          float param_7)

{
  float fVar1;

  fVar1 = (param_3 - param_2) - (param_2 - param_1);
  return (double)(((param_7 - param_5) / param_6) *
                  (((param_7 - (param_5 + param_6)) *
                   (((((param_4 - param_3) - (param_3 - param_2)) - fVar1) *
                    (param_7 - (param_5 + param_6 + param_6))) / (param_6 * 3.0) + fVar1)) /
                   (param_6 + param_6) + (param_2 - param_1)) + param_1);
}
#endif
