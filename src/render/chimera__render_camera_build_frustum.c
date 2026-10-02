// chimera__render_camera_build_frustum  (Ghidra: chimera__render_camera_build_frustum, Chimera
// signature name, kept; CEA render_camera_build_frustum(camera, frustum_bounds, frustum,
// build_projection), hint only)
// address 0x50cc40, size 2169 bytes
// name confidence: 0.8   rewrite confidence: 0.75
// evidence: objdump -d -M intel 0x50cc40..0x50d4b8, traced instruction by instruction (the
//   Ghidra decompile has the right arithmetic but hides every math helper's operands).
//   - arguments: ECX = camera (moved to EDI), EAX = frustum_bounds or NULL, ESI = frustum out,
//     one stack byte build_projection; matches the render_types_notes register note and every
//     caller in this module.
//   - frustum_bounds = the argument or {-1, 1, -1, 1}; hw / hh = half extents, cx / cy =
//     -(sum / half extent) * 0.5 (0x672bf4 = -0.5), t = tan(fov / 2) (fptan),
//     sx = 1 / (t * hw * width / height) and sy = 1 / (t * hh) with width / height the
//     camera viewport rectangle (+0x2c..+0x32, right - left and bottom - top).
//   - view_to_world (+0x44): scale 1, forward = normalize(cross(camera.forward, camera.up)),
//     left = normalize(cross(that, camera.forward)), up = normalize(-camera.forward), position =
//     camera.position (the view basis is x left, y up, z backwards); world_to_view (+0x10) =
//     matrix4x3_inverse 0x4cb7a0 (EAX out, ECX in).
//   - the four side planes are built in view space as normalize(n) with d = n . *0x006966f8
//     (a pointer to the zero point 0x0065c230) and taken to world space by
//     matrix4x3_transform_plane 0x4cbf10 (EAX out, ECX view_to_world, EDX plane):
//     [0] (-sx, 0, 1 + cx), [1] (sx, 0, 1 - cx), [2] (0, -sy, 1 + cy), [3] (0, sy, 1 - cy);
//     near [4] ((0,0,1), -z_near) and far [5] ((0,0,-1), z_far). The two z planes reuse ECX
//     and EDX from the previous call (0x4cbf10 leaves them alone).
//   - world_vertices[0..3] are the far plane corners (x = (cx +- 1) * -z_far / sx, y = (cy +- 1)
//     * -z_far / sy, z = -z_far, in the order (+,+) (-,+) (+,-) (-,-)), [4] the camera position;
//     world_midpoint is the view point (-mid cx / sx, -mid cy / sy, -mid) with mid =
//     (z_near + z_far) / 2; all through matrix4x3_transform_point 0x4cbde0 (EAX out, EDX point,
//     stack matrix, caller cleans the five pushes at 0x50d202).
//   - world_bounds: seeded from vertex 0, then min / max over vertices 1..4 (the camera position
//     included).
//   - build_projection == 0 clears projection_valid (+0x140) and returns.
//   - projection: the clip plane P is the camera mirror plane in view space (0x4cbf10 with
//     world_to_view, camera + 0x44) when z_near is exactly 0, else ((0,0,1), -z_near).
//     With k' = 1 / P.k, e = -P.d * k' and q = z_far / ((z_far - e) * (1 + |P.i k'| + |P.j k'|))
//     (the 1.0 is the double at 0x672af8): proj[2][2] = -q, proj[3][2] = q * P.d * k' (flipped
//     with q, P.i, P.j when it is positive and z_near is 0), proj[0][2] = -q P.i k',
//     proj[1][2] = -q P.j k', proj[0][0] = sx, proj[1][1] = sy, proj[2][0] = -cx,
//     proj[2][1] = -cy, proj[2][3] = -1, everything else 0 (rep stosd of 0x10 dwords); the
//     oblique near plane of a mirror pass.
//   - projection_world_to_screen = (sx * width / 2, sy * height / 2).
// register convention: ECX = camera, EAX = frustum_bounds (may be NULL), ESI = frustum,
//   one stack byte.
//   // blam-cc: EAX=frustum_bounds, ECX=camera, ESI=frustum, stack=build_projection
// UNSURE: none of the arithmetic; the name of 0x006966f8 (global_zero_vector3d_pointer here) differs from
//   the provisional names src/ai gives the same pointer.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "rasterizer.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern real_point3d *global_zero_vector3d_pointer; // 0x006966f8 -> {0,0,0} at 0x0065c230 (types/math.h
                                                   // global_math_constant_pointers slot 10; slot 17,
                                                   // 0x00696714, global_origin3d_pointer, points at the
                                                   // same constant)

extern real vector3d_normalize_with_length(real_vector3d *v); // 0x401990, math; blam-cc: ECX -> v
extern void matrix4x3_inverse(real_matrix4x3 *out, real_matrix4x3 *in);
    // 0x4cb7a0, math module; blam-cc: EAX -> out, ECX -> in
extern void matrix4x3_transform_plane(real_plane3d *out, real_matrix4x3 *m, real_plane3d *plane);
    // 0x4cbf10, math module; blam-cc: EAX -> out, ECX -> m, EDX -> plane
extern void matrix4x3_transform_point(real_point3d *out, real_point3d *point, real_matrix4x3 *m);
    // 0x4cbde0, math module; blam-cc: EAX -> out, EDX -> point, stack -> m
extern double tan(double x); // x87 FPTAN
extern double fabs(double x); // x87 FABS

static void build_side_plane(render_frustum *frustum, int16_t index, real i, real j, real k)
{
    real_vector3d normal;
    real_plane3d plane;

    normal.i = i;
    normal.j = j;
    normal.k = k;
    vector3d_normalize_with_length(&normal);
    plane.normal = normal;
    plane.d = normal.j * global_zero_vector3d_pointer->y + normal.k * global_zero_vector3d_pointer->z +
              normal.i * global_zero_vector3d_pointer->x;
    matrix4x3_transform_plane(&frustum->world_planes[index], &frustum->view_to_world, &plane);
}

// Builds the culling frustum of `camera` (view basis, world planes, far corners, bounds) and,
// when build_projection is set, its projection matrix and world to screen scale.
void chimera__render_camera_build_frustum(float *frustum_bounds, render_camera *camera,
                                          render_frustum *frustum, uint8_t build_projection)
    // blam-cc: EAX=frustum_bounds, ECX=camera, ESI=frustum, stack=build_projection
{
    real width;
    real height;
    real half_width;
    real half_height;
    real cx;
    real cy;
    real t;
    real sx;
    real sy;
    real_vector3d left;
    real_vector3d up;
    real_vector3d backward;
    real_plane3d plane;
    real_point3d point;
    real inverse_sx;
    real inverse_sy;
    real far_x;
    real far_y;
    real mid;
    int16_t i;

    width = (real)((int32_t)camera->viewport_bounds.right - (int32_t)camera->viewport_bounds.left);
    height = (real)((int32_t)camera->viewport_bounds.bottom - (int32_t)camera->viewport_bounds.top);
    if (frustum_bounds != 0) {
        frustum->frustum_bounds[0] = frustum_bounds[0];
        frustum->frustum_bounds[1] = frustum_bounds[1];
        frustum->frustum_bounds[2] = frustum_bounds[2];
        frustum->frustum_bounds[3] = frustum_bounds[3];
    } else {
        frustum->frustum_bounds[2] = -1.0f;
        frustum->frustum_bounds[0] = -1.0f;
        frustum->frustum_bounds[3] = 1.0f;
        frustum->frustum_bounds[1] = 1.0f;
    }
    half_width = (frustum->frustum_bounds[1] - frustum->frustum_bounds[0]) * 0.5f;
    half_height = (frustum->frustum_bounds[3] - frustum->frustum_bounds[2]) * 0.5f;
    cx = (frustum->frustum_bounds[0] + frustum->frustum_bounds[1]) / half_width * -0.5f;
    cy = (frustum->frustum_bounds[2] + frustum->frustum_bounds[3]) / half_height * -0.5f;
    t = (real)tan(camera->vertical_field_of_view * 0.5f);
    sx = 1.0f / (half_width / height * width * t);
    sy = 1.0f / (t * half_height);

    // view basis
    left.i = camera->up.k * camera->forward.j - camera->up.j * camera->forward.k;
    left.j = camera->forward.k * camera->up.i - camera->up.k * camera->forward.i;
    left.k = camera->up.j * camera->forward.i - camera->forward.j * camera->up.i;
    up.i = left.j * camera->forward.k - left.k * camera->forward.j;
    up.j = left.k * camera->forward.i - left.i * camera->forward.k;
    up.k = left.i * camera->forward.j - left.j * camera->forward.i;
    backward.i = -camera->forward.i;
    backward.j = -camera->forward.j;
    backward.k = -camera->forward.k;
    vector3d_normalize_with_length(&left);
    vector3d_normalize_with_length(&up);
    vector3d_normalize_with_length(&backward);
    frustum->view_to_world.forward = left;
    frustum->view_to_world.left = up;
    frustum->view_to_world.up = backward;
    frustum->view_to_world.position = camera->position;
    frustum->view_to_world.scale = 1.0f;
    matrix4x3_inverse(&frustum->world_to_view, &frustum->view_to_world);

    // side planes, then near and far
    build_side_plane(frustum, 0, -sx, 0.0f, cx + 1.0f);
    build_side_plane(frustum, 1, sx, 0.0f, 1.0f - cx);
    build_side_plane(frustum, 2, 0.0f, -sy, cy + 1.0f);
    build_side_plane(frustum, 3, 0.0f, sy, 1.0f - cy);
    plane.normal.i = 0.0f;
    plane.normal.j = 0.0f;
    plane.normal.k = 1.0f;
    plane.d = -camera->z_near;
    matrix4x3_transform_plane(&frustum->world_planes[4], &frustum->view_to_world, &plane);
    plane.normal.i = 0.0f;
    plane.normal.j = 0.0f;
    plane.normal.k = -1.0f;
    plane.d = camera->z_far;
    matrix4x3_transform_plane(&frustum->world_planes[5], &frustum->view_to_world, &plane);
    frustum->z_near = camera->z_near;
    frustum->z_far = camera->z_far;

    // far corners, camera position, midpoint
    inverse_sx = 1.0f / sx;
    far_x = -(inverse_sx * camera->z_far);
    inverse_sy = 1.0f / sy;
    far_y = -(inverse_sy * camera->z_far);
    mid = (camera->z_far + camera->z_near) * 0.5f;
    point.x = (cx + 1.0f) * far_x;
    point.y = (cy + 1.0f) * far_y;
    point.z = -camera->z_far;
    matrix4x3_transform_point(&frustum->world_vertices[0], &point, &frustum->view_to_world);
    point.x = (cx - 1.0f) * far_x;
    point.y = (cy + 1.0f) * far_y;
    point.z = -camera->z_far;
    matrix4x3_transform_point(&frustum->world_vertices[1], &point, &frustum->view_to_world);
    point.x = (cx + 1.0f) * far_x;
    point.y = (cy - 1.0f) * far_y;
    point.z = -camera->z_far;
    matrix4x3_transform_point(&frustum->world_vertices[2], &point, &frustum->view_to_world);
    point.x = (cx - 1.0f) * far_x;
    point.y = (cy - 1.0f) * far_y;
    point.z = -camera->z_far;
    matrix4x3_transform_point(&frustum->world_vertices[3], &point, &frustum->view_to_world);
    frustum->world_vertices[4] = camera->position;
    point.x = -(inverse_sx * mid * cx);
    point.y = -(inverse_sy * mid * cy);
    point.z = -mid;
    matrix4x3_transform_point(&frustum->world_midpoint, &point, &frustum->view_to_world);

    frustum->world_bounds.x.lower = frustum->world_bounds.x.upper = frustum->world_vertices[0].x;
    frustum->world_bounds.y.lower = frustum->world_bounds.y.upper = frustum->world_vertices[0].y;
    frustum->world_bounds.z.lower = frustum->world_bounds.z.upper = frustum->world_vertices[0].z;
    for (i = 1; i < 5; i++) {
        real_point3d *v = &frustum->world_vertices[i];

        frustum->world_bounds.x.lower = frustum->world_bounds.x.lower <= v->x ? frustum->world_bounds.x.lower : v->x;
        frustum->world_bounds.y.lower = frustum->world_bounds.y.lower <= v->y ? frustum->world_bounds.y.lower : v->y;
        frustum->world_bounds.z.lower = frustum->world_bounds.z.lower <= v->z ? frustum->world_bounds.z.lower : v->z;
        frustum->world_bounds.x.upper = frustum->world_bounds.x.upper <= v->x ? v->x : frustum->world_bounds.x.upper;
        frustum->world_bounds.y.upper = frustum->world_bounds.y.upper <= v->y ? v->y : frustum->world_bounds.y.upper;
        frustum->world_bounds.z.upper = frustum->world_bounds.z.upper <= v->z ? v->z : frustum->world_bounds.z.upper;
    }

    if (!build_projection) {
        frustum->projection_valid = 0;
        return;
    }

    {
        real_plane3d clip;
        real inverse_k;
        real e;
        real q;
        real qi;
        real qj;
        real qd;
        int16_t row;
        int16_t column;

        if (camera->z_near == 0.0f) {
            matrix4x3_transform_plane(&clip, &frustum->world_to_view, &camera->mirror_plane);
        } else {
            clip.normal.i = 0.0f;
            clip.normal.j = 0.0f;
            clip.normal.k = 1.0f;
            clip.d = -camera->z_near;
        }
        inverse_k = 1.0f / clip.normal.k;
        e = -(clip.d * inverse_k);
        q = camera->z_far / ((camera->z_far - e) *
                             ((real)fabs(clip.normal.j * inverse_k) +
                              (real)fabs(clip.normal.i * inverse_k) + 1.0f));
        qi = q * clip.normal.i * inverse_k;
        qj = q * clip.normal.j * inverse_k;
        qd = -(q * e);
        if (qd > 0.0f && camera->z_near == 0.0f) {
            qi = -qi;
            qj = -qj;
            q = -q;
            qd = -qd;
        }

        for (row = 0; row < 4; row++) {
            for (column = 0; column < 4; column++) {
                frustum->projection[row][column] = 0.0f;
            }
        }
        frustum->projection[0][2] = -qi;
        frustum->projection[1][2] = -qj;
        frustum->projection[2][0] = -cx;
        frustum->projection[0][0] = sx;
        frustum->projection[2][1] = -cy;
        frustum->projection[1][1] = sy;
        frustum->projection[2][3] = -1.0f;
        frustum->projection_valid = 1;
        frustum->projection[2][2] = -q;
        frustum->projection[3][2] = qd;
        frustum->projection_world_to_screen.i = sx * width * 0.5f;
        frustum->projection_world_to_screen.j = sy * height * 0.5f;
    }
}

#if 0
Original Ghidra decompilation (0x50cc40):

void chimera__render_camera_build_frustum(char param_1)

{
  float fVar1;
  float fVar2;
  float fVar3;
  float fVar4;
  float fVar5;
  float fVar6;
  float fVar7;
  float fVar8;
  float fVar9;
  float fVar10;
  float fVar11;
  float fVar12;
  float fVar13;
  short sVar14;
  short sVar15;
  float fVar16;
  float fVar17;
  float fVar18;
  float *in_EAX;
  float *in_ECX;
  int iVar19;
  float *pfVar20;
  int iVar21;
  float *unaff_ESI;
  float10 fVar22;
  float10 fVar23;
  float10 fVar24;
  float local_c;
  
  iVar19 = (int)*(short *)((int)in_ECX + 0x32) - (int)*(short *)((int)in_ECX + 0x2e);
  sVar14 = *(short *)(in_ECX + 0xb);
  sVar15 = *(short *)(in_ECX + 0xc);
  if (in_EAX == (float *)0x0) {
    unaff_ESI[2] = -1.0;
    *unaff_ESI = -1.0;
    unaff_ESI[3] = 1.0;
    unaff_ESI[1] = 1.0;
  }
  else {
    *unaff_ESI = *in_EAX;
    unaff_ESI[1] = in_EAX[1];
    unaff_ESI[2] = in_EAX[2];
    unaff_ESI[3] = in_EAX[3];
  }
  fVar22 = ((float10)unaff_ESI[1] - (float10)*unaff_ESI) * (float10)0.5;
  fVar23 = ((float10)unaff_ESI[3] - (float10)unaff_ESI[2]) * (float10)0.5;
  fVar1 = (float)((((float10)*unaff_ESI + (float10)unaff_ESI[1]) / fVar22) * (float10)-0.5);
  fVar2 = (float)((((float10)unaff_ESI[2] + (float10)unaff_ESI[3]) / fVar23) * (float10)-0.5);
  fVar24 = (float10)fptan((float10)in_ECX[10] * (float10)0.5);
  fVar3 = (float)((float10)1.0 /
                 ((fVar22 / (float10)((int)sVar15 - (int)sVar14)) * (float10)iVar19 * fVar24));
  fVar4 = (float)((float10)1.0 / (fVar24 * fVar23));
  fVar16 = in_ECX[8] * in_ECX[4] - in_ECX[7] * in_ECX[5];
  fVar17 = in_ECX[5] * in_ECX[6] - in_ECX[8] * in_ECX[3];
  fVar18 = in_ECX[7] * in_ECX[3] - in_ECX[4] * in_ECX[6];
  fVar5 = in_ECX[5];
  fVar6 = in_ECX[4];
  fVar7 = in_ECX[3];
  fVar8 = in_ECX[5];
  fVar9 = in_ECX[4];
  fVar10 = in_ECX[3];
  fVar11 = in_ECX[3];
  fVar12 = in_ECX[4];
  fVar13 = in_ECX[5];
  vector3d_normalize_with_length();
  vector3d_normalize_with_length();
  vector3d_normalize_with_length();
  unaff_ESI[0x12] = fVar16;
  unaff_ESI[0x13] = fVar17;
  unaff_ESI[0x14] = fVar18;
  unaff_ESI[0x15] = fVar17 * fVar5 - fVar18 * fVar6;
  unaff_ESI[0x16] = fVar18 * fVar7 - fVar16 * fVar8;
  unaff_ESI[0x17] = fVar16 * fVar9 - fVar17 * fVar10;
  unaff_ESI[0x18] = -fVar11;
  unaff_ESI[0x19] = -fVar12;
  unaff_ESI[0x1a] = -fVar13;
  unaff_ESI[0x1b] = *in_ECX;
  unaff_ESI[0x1c] = in_ECX[1];
  pfVar20 = unaff_ESI + 0x11;
  unaff_ESI[0x1d] = in_ECX[2];
  *pfVar20 = 1.0;
  matrix4x3_inverse();
  vector3d_normalize_with_length();
  matrix4x3_transform_plane();
  vector3d_normalize_with_length();
  matrix4x3_transform_plane();
  vector3d_normalize_with_length();
  matrix4x3_transform_plane();
  vector3d_normalize_with_length();
  matrix4x3_transform_plane();
  matrix4x3_transform_plane();
  matrix4x3_transform_plane();
  unaff_ESI[0x36] = in_ECX[0xf];
  unaff_ESI[0x37] = in_ECX[0x10];
  fVar6 = (in_ECX[0x10] + in_ECX[0xf]) * 0.5;
  fVar5 = (fVar2 - 1.0) * -((1.0 / fVar4) * in_ECX[0x10]);
  matrix4x3_transform_point(pfVar20);
  matrix4x3_transform_point(pfVar20);
  matrix4x3_transform_point(pfVar20);
  matrix4x3_transform_point(pfVar20);
  unaff_ESI[0x44] = *in_ECX;
  unaff_ESI[0x45] = in_ECX[1];
  fVar7 = -((1.0 / fVar3) * fVar6 * fVar1);
  unaff_ESI[0x46] = in_ECX[2];
  local_c = -((1.0 / fVar4) * fVar6 * fVar2);
  fVar6 = -fVar6;
  matrix4x3_transform_point(pfVar20);
  unaff_ESI[0x4b] = unaff_ESI[0x38];
  pfVar20 = unaff_ESI + 0x3c;
  unaff_ESI[0x4a] = unaff_ESI[0x38];
  iVar21 = 4;
  unaff_ESI[0x4d] = unaff_ESI[0x39];
  unaff_ESI[0x4c] = unaff_ESI[0x39];
  unaff_ESI[0x4f] = unaff_ESI[0x3a];
  unaff_ESI[0x4e] = unaff_ESI[0x3a];
  do {
    if (unaff_ESI[0x4a] <= pfVar20[-1]) {
      fVar8 = unaff_ESI[0x4a];
    }
    else {
      fVar8 = pfVar20[-1];
    }
    unaff_ESI[0x4a] = fVar8;
    if (unaff_ESI[0x4c] <= *pfVar20) {
      fVar8 = unaff_ESI[0x4c];
    }
    else {
      fVar8 = *pfVar20;
    }
    unaff_ESI[0x4c] = fVar8;
    if (unaff_ESI[0x4e] <= pfVar20[1]) {
      fVar8 = unaff_ESI[0x4e];
    }
    else {
      fVar8 = pfVar20[1];
    }
    unaff_ESI[0x4e] = fVar8;
    if (unaff_ESI[0x4b] <= pfVar20[-1]) {
      fVar8 = pfVar20[-1];
    }
    else {
      fVar8 = unaff_ESI[0x4b];
    }
    unaff_ESI[0x4b] = fVar8;
    if (unaff_ESI[0x4d] <= *pfVar20) {
      fVar8 = *pfVar20;
    }
    else {
      fVar8 = unaff_ESI[0x4d];
    }
    unaff_ESI[0x4d] = fVar8;
    if (unaff_ESI[0x4f] <= pfVar20[1]) {
      fVar8 = pfVar20[1];
    }
    else {
      fVar8 = unaff_ESI[0x4f];
    }
    pfVar20 = pfVar20 + 3;
    unaff_ESI[0x4f] = fVar8;
    iVar21 = iVar21 + -1;
  } while (iVar21 != 0);
  if (param_1 == '\0') {
    *(undefined1 *)(unaff_ESI + 0x50) = 0;
    return;
  }
  if (in_ECX[0xf] == 0.0) {
    matrix4x3_transform_plane();
  }
  else {
    local_c = 0.0;
    fVar7 = 0.0;
    fVar6 = 1.0;
    fVar5 = -in_ECX[0xf];
  }
  fVar6 = 1.0 / fVar6;
  fVar8 = in_ECX[0x10] /
          ((in_ECX[0x10] - -(fVar5 * fVar6)) * (ABS(fVar7 * fVar6) + ABS(local_c * fVar6) + 1.0));
  fVar7 = fVar8 * fVar7 * fVar6;
  fVar9 = fVar8 * local_c * fVar6;
  fVar5 = -(fVar8 * -(fVar5 * fVar6));
  if ((0.0 < fVar5) && (in_ECX[0xf] == 0.0)) {
    fVar7 = -fVar7;
    fVar9 = -fVar9;
    fVar8 = -fVar8;
    fVar5 = -fVar5;
  }
  pfVar20 = unaff_ESI + 0x51;
  for (iVar21 = 0x10; iVar21 != 0; iVar21 = iVar21 + -1) {
    *pfVar20 = 0.0;
    pfVar20 = pfVar20 + 1;
  }
  unaff_ESI[0x53] = -fVar7;
  unaff_ESI[0x57] = -fVar9;
  unaff_ESI[0x59] = -fVar1;
  unaff_ESI[0x51] = fVar3;
  unaff_ESI[0x5a] = -fVar2;
  unaff_ESI[0x56] = fVar4;
  unaff_ESI[0x5c] = -1.0;
  *(undefined1 *)(unaff_ESI + 0x50) = 1;
  unaff_ESI[0x5b] = -fVar8;
  unaff_ESI[0x5f] = fVar5;
  unaff_ESI[0x61] = fVar3 * (float)iVar19 * 0.5;
  unaff_ESI[0x62] = fVar4 * (float)((int)sVar15 - (int)sVar14) * 0.5;
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
