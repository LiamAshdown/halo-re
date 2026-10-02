// vector3d_major_axis_index  (Ghidra: FUN_0044d820; name from decal_place.c / structure_bsp_portal_sphere_test.c)
// address 0x44d820, size 63 bytes
// name confidence: 0.6   rewrite confidence: 0.9
// evidence: objdump 0x44d820..0x44d85e: fabs of x, y, z; returns 0, 1 or 2 in AX for the component with the largest
//   magnitude, ties going to the higher index (z over y over x). An unordered compare (NaN) sets C0 and so counts
//   as "less" -- written below as !(a >= b), which is also true for NaN.
// blam-cc: EAX -> v

#include "tags.h"
#include "math.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern double fabs(double x);

int16_t vector3d_major_axis_index(real_vector3d *v)
{
    real x = (real)fabs(v->i);
    real y = (real)fabs(v->j);
    real z = (real)fabs(v->k);

    if (!(z >= y) || !(z >= x)) {        // 0x44d82e fcom st(1) / 0x44d837 fcomp st(2)
        return !(y >= x) ? 0 : 1;        // 0x44d84b fcomp st(1)
    }
    return 2;
}
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
