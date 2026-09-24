// vector3d_magnitude_squared  (Ghidra: FUN_00401000, orphan-pass named from evidence)
// address 0x401000, size 31 bytes
// name confidence: 0.85   rewrite confidence: 0.9
// evidence: modules.json/task list already carries this name; objdump of 0x401000..0x40101f
//   confirms the squared-length pattern with no FSQRT (unlike vector3d_length @0x401960, which
//   is the same field layout plus a trailing sqrt). Orphaned from "unknown" because Ghidra never
//   split this address into its own function (empty decompile at 0x401000); no *_types_notes.md
//   entry exists for it.
// register convention: vector pointer in EAX, no stack arguments (matches vector3d_length).
//   // blam-cc: EAX -> v

#include "tags.h"
#include "math.h"

// Returns the squared Euclidean length of a 3D vector (no sqrt), i.e. the dot product of the
// vector with itself. Used where callers only need to compare magnitudes.
real vector3d_magnitude_squared(real_vector3d *v)
{
    return v->i * v->i + v->j * v->j + v->k * v->k;
}

#if 0
Original Ghidra decompilation (0x401000):

void vector3d_magnitude_squared(void)

{
  return;
}

-- Ghidra produced an empty body (it never split this address out as its own function). The real
-- code, from objdump -d -M intel --start-address=0x401000 --stop-address=0x40101f bin/halo.exe:

  401000:	d9 40 08             	fld    DWORD PTR [eax+0x8]
  401003:	d9 40 04             	fld    DWORD PTR [eax+0x4]
  401006:	d9 00                	fld    DWORD PTR [eax]
  401008:	d9 c0                	fld    st(0)
  40100a:	d8 c9                	fmul   st,st(1)
  40100c:	d9 c2                	fld    st(2)
  40100e:	d8 cb                	fmul   st,st(3)
  401010:	de c1                	faddp  st(1),st
  401012:	d9 c3                	fld    st(3)
  401014:	d8 cc                	fmul   st,st(4)
  401016:	de c1                	faddp  st(1),st
  401018:	dd db                	fstp   st(3)
  40101a:	dd d8                	fstp   st(0)
  40101c:	dd d8                	fstp   st(0)
  40101e:	c3                   	ret
  40101f:	cc                   	int3
#endif
