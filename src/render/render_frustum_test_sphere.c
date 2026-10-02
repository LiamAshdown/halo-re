// render_frustum_test_sphere  (Ghidra: render_frustum_test_sphere, already named)
// address 0x50d890, size 555 bytes
// name confidence: 0.5   rewrite confidence: 0.85
// evidence: out/phase4/render_functions.md's phase-2 summary: "Classifies a sphere (center +
//   radius) against the camera frustum planes as outside, partially inside, or fully inside."
//   Disassembly (objdump -d -M intel, 0x50d890..0x50d8bf) confirms ECX=frustum, EDX=center
//   (real_point3d*), stack=radius.
// register convention: ECX = frustum (render_frustum*), EDX = center (real_point3d*),
//   stack = radius.
//   // blam-cc: ECX=frustum, EDX=center, stack=radius
// UNSURE: the final "fully inside" refinement only re-checks 5 of the 6 plane distances (the
// side plane at world_planes[4] is used for the initial outside gate but never retested here);
// reproduced exactly rather than treated as an oversight to fix.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "rasterizer.h"
#include "render.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

// Classifies a world-space sphere against the camera frustum: outside, partially inside (straddles
// at least one plane) or fully inside. First rejects spheres that do not even overlap the
// frustum's own overall AABB (world_bounds) or that are entirely outside any one of the six
// frustum planes by more than the radius; a sphere that passes is then fully inside only if it
// also sits more than a radius on the inside of five of those six planes (see the UNSURE note
// above for the sixth).
int16_t render_frustum_test_sphere(render_frustum *frustum, real_point3d *center,
                                     float radius) // blam-cc: ECX=frustum, EDX=center, stack=radius
{
    float distance0, distance1, distance2, distance3, distance5;
    float negative_radius;

    if (center->x - radius > frustum->world_bounds.x.upper ||
        center->y - radius > frustum->world_bounds.y.upper ||
        center->z - radius > frustum->world_bounds.z.upper ||
        frustum->world_bounds.x.lower > radius + center->x ||
        frustum->world_bounds.y.lower > radius + center->y ||
        frustum->world_bounds.z.lower > radius + center->z) {
        return _render_frustum_outside;
    }

    distance0 = (frustum->world_planes[0].normal.i * center->x + frustum->world_planes[0].normal.j * center->y +
                 frustum->world_planes[0].normal.k * center->z) - frustum->world_planes[0].d;
    if (distance0 > radius) {
        return _render_frustum_outside;
    }
    distance1 = (frustum->world_planes[1].normal.i * center->x + frustum->world_planes[1].normal.j * center->y +
                 frustum->world_planes[1].normal.k * center->z) - frustum->world_planes[1].d;
    if (distance1 > radius) {
        return _render_frustum_outside;
    }
    distance2 = (frustum->world_planes[2].normal.i * center->x + frustum->world_planes[2].normal.j * center->y +
                 frustum->world_planes[2].normal.k * center->z) - frustum->world_planes[2].d;
    if (distance2 > radius) {
        return _render_frustum_outside;
    }
    distance3 = (frustum->world_planes[3].normal.i * center->x + frustum->world_planes[3].normal.j * center->y +
                 frustum->world_planes[3].normal.k * center->z) - frustum->world_planes[3].d;
    if (distance3 > radius) {
        return _render_frustum_outside;
    }
    if ((frustum->world_planes[4].normal.i * center->x + frustum->world_planes[4].normal.j * center->y +
         frustum->world_planes[4].normal.k * center->z) - frustum->world_planes[4].d > radius) {
        return _render_frustum_outside;
    }
    distance5 = (frustum->world_planes[5].normal.i * center->x + frustum->world_planes[5].normal.j * center->y +
                 frustum->world_planes[5].normal.k * center->z) - frustum->world_planes[5].d;
    if (distance5 > radius) {
        return _render_frustum_outside;
    }

    negative_radius = -radius;
    if (negative_radius <= distance0 || negative_radius <= distance1 || negative_radius <= distance2 ||
        negative_radius <= distance3 || negative_radius <= distance5) {
        return _render_frustum_partial;
    }
    return _render_frustum_inside;
}

#if 0
Original Ghidra decompilation (0x50d890):

undefined2 render_frustum_test_sphere(float param_1)

{
  float fVar1;
  float fVar2;
  float fVar3;
  float fVar4;
  float fVar5;
  undefined2 uVar6;
  int in_ECX;
  float *in_EDX;

  if (((((((*in_EDX - param_1 <= *(float *)(in_ECX + 300)) &&
          (in_EDX[1] - param_1 <= *(float *)(in_ECX + 0x134))) &&
         (in_EDX[2] - param_1 <= *(float *)(in_ECX + 0x13c))) &&
        ((*(float *)(in_ECX + 0x128) <= param_1 + *in_EDX &&
         (*(float *)(in_ECX + 0x130) <= param_1 + in_EDX[1])))) &&
       ((*(float *)(in_ECX + 0x138) <= param_1 + in_EDX[2] &&
        ((fVar1 = (*(float *)(in_ECX + 0x78) * *in_EDX +
                  *(float *)(in_ECX + 0x7c) * in_EDX[1] + *(float *)(in_ECX + 0x80) * in_EDX[2]) -
                  *(float *)(in_ECX + 0x84), fVar1 <= param_1 &&
         (fVar2 = (*(float *)(in_ECX + 0x88) * *in_EDX +
                  *(float *)(in_ECX + 0x8c) * in_EDX[1] + *(float *)(in_ECX + 0x90) * in_EDX[2]) -
                  *(float *)(in_ECX + 0x94), fVar2 <= param_1)))))) &&
      (fVar3 = (*(float *)(in_ECX + 0x98) * *in_EDX +
               *(float *)(in_ECX + 0x9c) * in_EDX[1] + *(float *)(in_ECX + 0xa0) * in_EDX[2]) -
               *(float *)(in_ECX + 0xa4), fVar3 <= param_1)) &&
     (((fVar4 = (*in_EDX * *(float *)(in_ECX + 0xa8) +
                *(float *)(in_ECX + 0xac) * in_EDX[1] + *(float *)(in_ECX + 0xb0) * in_EDX[2]) -
                *(float *)(in_ECX + 0xb4), fVar4 <= param_1 &&
       ((*in_EDX * *(float *)(in_ECX + 0xb8) +
        *(float *)(in_ECX + 0xbc) * in_EDX[1] + *(float *)(in_ECX + 0xc0) * in_EDX[2]) -
        *(float *)(in_ECX + 0xc4) <= param_1)) &&
      (fVar5 = (*in_EDX * *(float *)(in_ECX + 200) +
               *(float *)(in_ECX + 0xcc) * in_EDX[1] + *(float *)(in_ECX + 0xd0) * in_EDX[2]) -
               *(float *)(in_ECX + 0xd4), fVar5 <= param_1)))) {
    param_1 = -param_1;
    if (((param_1 <= fVar1) || (param_1 <= fVar2)) ||
       ((param_1 <= fVar3 || ((param_1 <= fVar4 || (uVar6 = 2, param_1 <= fVar5)))))) {
      uVar6 = 1;
    }
    return uVar6;
  }
  return 0;
}

Note on control flow: due to short-circuit evaluation, uVar6 is only ever assigned via the
`(uVar6 = 2, param_1 <= fVar5)` comma expression, which runs only once the first four OR terms
have all failed; if that expression's own comparison is also false the enclosing `if` body never
runs and uVar6=2 (fully inside) survives to the return, otherwise the `if` body overwrites it with
1 (partial). The rewrite above expresses the same result directly as two returns instead of
reproducing the uninitialized-until-touched local.
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
