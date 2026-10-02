// matrix4x3_multiply_sse  (not a Ghidra function; the SSE build of matrix4x3_multiply)
// address 0x4cc250, size 336 bytes
// name confidence: 0.85  rewrite confidence: 0.8
// evidence: math_initialize 0x4cd3f0 installs it in matrix4x3_multiply_procedure when cpu_get_type(0x1d) reports
//   SSE. objdump 0x4cc250..0x4cc39f (MOVSS/MOVHPS/SHUFPS/MULPS/ADDPS): each output row is the sum of a's three
//   basis rows scaled by the matching b components; the position is a's rotation applied to b's position, scaled by
//   a's scale, plus a's position; the output scale (x87 FMUL at the end) is a.scale * b.scale -- the same product
//   matrix4x3_multiply 0x4cc0d0 computes. Written, like matrix4x3_multiply_3dnow, as that scalar formula: the
//   SIMD adds round each float product in single precision, so the last bit can differ from x87 builds.
//   First-boot track: reached on the first object placement.
// blam-cc: stack -> a, b, out (cdecl)
#include "tags.h"
#include "memory.h"
#include "math.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern void matrix4x3_multiply(real_matrix4x3 *a, real_matrix4x3 *b, real_matrix4x3 *out); // 0x4cc0d0

void matrix4x3_multiply_sse(real_matrix4x3 *a, real_matrix4x3 *b, real_matrix4x3 *out)
{
    matrix4x3_multiply(a, b, out);
}
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
