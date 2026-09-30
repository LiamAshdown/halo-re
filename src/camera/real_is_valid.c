// real_is_valid  (Ghidra: real_is_valid, already named)
// address 0x4476c0, size 24 bytes
// name confidence: 0.8   rewrite confidence: 0.9
// evidence: types/camera.h calls this and real_approximately_equal / vector3d_is_unit_length
// "observer assertion helpers"; a plain NaN guard.
// register convention: __cdecl, single stack parameter (Ghidra's recognized param_1).

#include "crt.h"
#include "tags.h"
#include "memory.h"
#include "camera.h"
#include "fn_camera.h"


// Returns whether value is a valid (non-NaN) float.
uint8_t real_is_valid(float value)
{
    return (uint8_t)(_isnan((double)value) == 0);
}

#if 0
Original Ghidra decompilation (0x4476c0):

int __cdecl real_is_valid(float param_1)

{
  int iVar1;

  iVar1 = __isnan((double)param_1);
  return (uint)(iVar1 == 0);
}
#endif
