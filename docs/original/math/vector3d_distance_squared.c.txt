// vector3d_distance_squared  (Ghidra: FUN_00401020, orphan-pass named from evidence)
// address 0x401020, size 39 bytes
// name confidence: 0.85   rewrite confidence: 0.9
// evidence: modules.json/task list already carries this name; objdump of 0x401020..0x401046
//   confirms the squared-distance pattern (subtract, then the same fld/fmul/faddp chain as
//   vector3d_magnitude_squared @0x401000, with no trailing FSQRT — vector3d_distance @0x4088b0
//   is the same layout plus sqrt). Orphaned from "unknown" because Ghidra never split this
//   address into its own function; no *_types_notes.md entry exists for it.
// register convention: first point in EAX, second point in ECX (matches vector3d_distance).
//   // blam-cc: EAX -> a, ECX -> b

#include "tags.h"
#include "math.h"

// Returns the squared Euclidean distance between two 3D points (no sqrt). Used where callers
// only need to compare distances (e.g. against a squared threshold).
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
real vector3d_distance_squared(real_point3d *a, real_point3d *b)
{
    return (a->z - b->z) * (a->z - b->z) +
           (a->x - b->x) * (a->x - b->x) +
           (a->y - b->y) * (a->y - b->y);
}

#if 0
Original Ghidra decompilation (0x401020):

-- Ghidra never split this address into its own function. From objdump -d -M intel
-- --start-address=0x401020 --stop-address=0x401047 bin/halo.exe:

  401020:	d9 00                	fld    DWORD PTR [eax]
  401022:	d8 21                	fsub   DWORD PTR [ecx]
  401024:	d9 40 04             	fld    DWORD PTR [eax+0x4]
  401027:	d8 61 04             	fsub   DWORD PTR [ecx+0x4]
  40102a:	d9 40 08             	fld    DWORD PTR [eax+0x8]
  40102d:	d8 61 08             	fsub   DWORD PTR [ecx+0x8]
  401030:	d9 c0                	fld    st(0)
  401032:	d8 c9                	fmul   st,st(1)
  401034:	d9 c3                	fld    st(3)
  401036:	d8 cc                	fmul   st,st(4)
  401038:	de c1                	faddp  st(1),st
  40103a:	d9 c2                	fld    st(2)
  40103c:	d8 cb                	fmul   st,st(3)
  40103e:	de c1                	faddp  st(1),st
  401040:	dd db                	fstp   st(3)
  401042:	dd d8                	fstp   st(0)
  401044:	dd d8                	fstp   st(0)
  401046:	c3                   	ret
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
