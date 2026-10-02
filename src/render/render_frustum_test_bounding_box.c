// render_frustum_test_bounding_box  (Ghidra: render_frustum_test_bounding_box, already named)
// address 0x50d5b0, size 731 bytes
// name confidence: 0.5   rewrite confidence: 0.85
// evidence: out/phase4/render_functions.md's phase-2 summary: "Classifies an axis-aligned
// bounding box against the camera frustum as fully outside, intersecting, or fully inside."
// Disassembly (objdump -d -M intel, 0x50d5b0..0x50d619) confirms ECX=frustum, EDI=box
// (real_rectangle3d*: x.lower/upper, y.lower/upper, z.lower/upper at +0x00/04/08/0c/10/14) for
// the leading world_bounds overlap test. The per-corner classification against the four side
// planes reuses render_frustum_classify_point_side_planes's exact bit values (plane0 +0x78 ->
// 0x01, plane1 +0x88 -> 0x02, plane3 +0xa8 -> 0x04, plane2 +0x98 -> 0x08), so this rewrite calls
// that function instead of re-inlining the same four dot products. The optional validate pass
// (param_1 != 0) re-derives its own six bits per frustum world_vertex from a careful reading of
// the "(a<b)==(a==b)" ("a>b") and "(a<b)!=(a==b)" ("a<=b") idioms mixed with plain "<"/">="
// comparisons in the decompile -- see the inline note at that loop.
// register convention: ECX = frustum (render_frustum*), EDI = box (real_rectangle3d*),
//   stack = validate (whether to also run the near/far vertex-separation pass).
//   // blam-cc: ECX=frustum, EDI=box, stack=validate

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "rasterizer.h"
#include "render.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern uint8_t render_frustum_classify_point_side_planes(render_frustum *frustum,
                                                          real_point3d *point); // 0x50d4c0, this
                                                                               // module

// Classifies an axis-aligned world-space bounding box against the camera frustum: fully outside,
// fully inside, or intersecting. First rejects boxes that do not even overlap the frustum's own
// overall AABB (world_bounds), then classifies all 8 box corners against the four side planes
// (via render_frustum_classify_point_side_planes) to decide outside/inside/intersecting. When
// validate is set and the side-plane test says "intersecting", a further separating-axis check
// against the frustum's own extreme vertices (world_vertices) can still reject the box.
int16_t render_frustum_test_bounding_box(render_frustum *frustum, real_rectangle3d *box,
                                           uint8_t validate) // blam-cc: ECX=frustum, EDI=box, stack=validate
{
    real_point3d corners[8];
    uint8_t all_outside = 0x3f;
    uint8_t any_outside = 0;
    int i;

    if (frustum->world_bounds.x.upper < box->x.lower || frustum->world_bounds.y.upper < box->y.lower ||
        frustum->world_bounds.z.upper < box->z.lower || box->x.upper < frustum->world_bounds.x.lower ||
        box->y.upper < frustum->world_bounds.y.lower || box->z.upper < frustum->world_bounds.z.lower) {
        return _render_frustum_outside;
    }

    corners[0].x = box->x.lower; corners[0].y = box->y.lower; corners[0].z = box->z.lower;
    corners[1].x = box->x.upper; corners[1].y = box->y.lower; corners[1].z = box->z.lower;
    corners[2].x = box->x.lower; corners[2].y = box->y.upper; corners[2].z = box->z.lower;
    corners[3].x = box->x.upper; corners[3].y = box->y.upper; corners[3].z = box->z.lower;
    corners[4].x = box->x.lower; corners[4].y = box->y.lower; corners[4].z = box->z.upper;
    corners[5].x = box->x.upper; corners[5].y = box->y.lower; corners[5].z = box->z.upper;
    corners[6].x = box->x.lower; corners[6].y = box->y.upper; corners[6].z = box->z.upper;
    corners[7].x = box->x.upper; corners[7].y = box->y.upper; corners[7].z = box->z.upper;

    for (i = 0; i < 8; i++) {
        uint8_t flags = render_frustum_classify_point_side_planes(frustum, &corners[i]);
        all_outside &= flags;
        any_outside |= flags;
    }

    if (any_outside == 0) {
        return _render_frustum_inside;
    }
    if (all_outside != 0) {
        return _render_frustum_outside;
    }

    if (validate) {
        // Separating-axis check: for each of the frustum's 5 extreme vertices, set one bit per
        // box side (x/y/z, lower/upper) the vertex lies outside of. If every vertex shares a
        // common outside side, the box and frustum do not actually intersect.
        uint8_t vertex_separates = 0x3f;

        for (i = 0; i < 5; i++) {
            real_point3d *v = &frustum->world_vertices[i];
            uint8_t bits = 0;

            if (v->x <= box->x.lower) bits |= 0x01;
            if (v->x >= box->x.upper) bits |= 0x02;
            if (v->y >= box->y.upper) bits |= 0x04;
            if (v->y <= box->y.lower) bits |= 0x08;
            if (v->z <= box->z.lower) bits |= 0x10;
            if (v->z >= box->z.upper) bits |= 0x20;

            vertex_separates &= bits;
        }

        if (vertex_separates != 0) {
            return _render_frustum_outside;
        }
    }

    return _render_frustum_partial;
}

#if 0
Original Ghidra decompilation (0x50d5b0):

undefined4 render_frustum_test_bounding_box(char param_1)

{
  byte bVar1;
  byte bVar2;
  byte bVar3;
  byte bVar4;
  byte bVar5;
  int in_ECX;
  float *pfVar6;
  int iVar7;
  byte bVar8;
  float *unaff_EDI;
  int local_64;
  float local_5c;
  float local_58;
  float local_54;
  float local_50;
  float local_4c;
  float local_48;
  float local_44;
  float local_40;
  float local_3c;
  float local_38;
  float local_34;
  float local_30;
  float local_2c;
  float local_28;
  float local_24;
  float local_20;
  float local_1c;
  float local_18;
  float local_14;
  float local_10;
  float local_c;
  float local_8;
  float local_4;

  if ((((*(float *)(in_ECX + 300) < *unaff_EDI) || (*(float *)(in_ECX + 0x134) < unaff_EDI[2])) ||
      (*(float *)(in_ECX + 0x13c) < unaff_EDI[4])) ||
     (((unaff_EDI[1] < *(float *)(in_ECX + 0x128) || (unaff_EDI[3] < *(float *)(in_ECX + 0x130))) ||
      (unaff_EDI[5] < *(float *)(in_ECX + 0x138))))) {
    return 0;
  }
  local_18 = *unaff_EDI;
  local_30 = local_18;
  bVar1 = 0x3f;
  local_48 = local_18;
  bVar8 = 0;
  pfVar6 = &local_5c;
  local_c = unaff_EDI[1];
  local_64 = 8;
  local_24 = local_c;
  local_3c = local_c;
  local_54 = local_c;
  local_20 = unaff_EDI[2];
  local_2c = local_20;
  local_50 = local_20;
  local_5c = local_20;
  local_8 = unaff_EDI[3];
  local_14 = local_8;
  local_38 = local_8;
  local_44 = local_8;
  local_34 = unaff_EDI[4];
  local_40 = local_34;
  local_4c = local_34;
  local_58 = local_34;
  local_4 = unaff_EDI[5];
  local_10 = local_4;
  local_1c = local_4;
  local_28 = local_4;
  do {
    /* inlined classify against the four side planes, same bit values as
       render_frustum_classify_point_side_planes 0x50d4c0 */
    bVar1 = bVar1 & bVar4;
    bVar8 = bVar8 | bVar4;
    pfVar6 = pfVar6 + 3;
    local_64 = local_64 + -1;
  } while (local_64 != 0);
  if (bVar8 != 0) {
    if (bVar1 != 0) {
      return 0;
    }
    if (param_1 != '\0') {
      bVar8 = 0x3f;
      pfVar6 = (float *)(in_ECX + 0xe8);
      iVar7 = 5;
      do {
        if (pfVar6[-2] < unaff_EDI[1]) {
          bVar1 = 0;
        }
        else {
          bVar1 = 2;
        }
        if (pfVar6[-1] < unaff_EDI[2] == (pfVar6[-1] == unaff_EDI[2])) {
          bVar2 = 0;
        }
        else {
          bVar2 = 8;
        }
        if (pfVar6[-1] < unaff_EDI[3]) {
          bVar3 = 0;
        }
        else {
          bVar3 = 4;
        }
        if (*pfVar6 < unaff_EDI[4] == (*pfVar6 == unaff_EDI[4])) {
          bVar4 = 0;
        }
        else {
          bVar4 = 0x10;
        }
        if (*pfVar6 < unaff_EDI[5]) {
          bVar5 = 0;
        }
        else {
          bVar5 = 0x20;
        }
        bVar8 = bVar8 & (bVar5 | pfVar6[-2] < *unaff_EDI != (pfVar6[-2] == *unaff_EDI) | bVar1 |
                                 bVar2 | bVar3 | bVar4);
        pfVar6 = pfVar6 + 3;
        iVar7 = iVar7 + -1;
      } while (iVar7 != 0);
      if (bVar8 != 0) {
        return 0;
      }
    }
    return 1;
  }
  return 2;
}

Idiom key for the vertex loop (pfVar6[-2]=vertex.x, pfVar6[-1]=vertex.y, *pfVar6=vertex.z,
unaff_EDI[0..5]=box.x.lower/upper,y.lower/upper,z.lower/upper):
  bVar1 (plain "<"):            vertex.x <  box.x.upper ? 0 : 2      -> bit 0x02 = vertex.x >= box.x.upper
  bVar2 ("(a<b)==(a==b)"="a>b"): vertex.y >  box.y.lower ? 0 : 8      -> bit 0x08 = vertex.y <= box.y.lower
  bVar3 (plain "<"):            vertex.y <  box.y.upper ? 0 : 4      -> bit 0x04 = vertex.y >= box.y.upper
  bVar4 ("a>b" idiom):          vertex.z >  box.z.lower ? 0 : 0x10   -> bit 0x10 = vertex.z <= box.z.lower
  bVar5 (plain "<"):            vertex.z <  box.z.upper ? 0 : 0x20   -> bit 0x20 = vertex.z >= box.z.upper
  last term ("(a<b)!=(a==b)"="a<=b"): vertex.x <= box.x.lower ? 1 : 0 -> bit 0x01 = vertex.x <= box.x.lower
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
