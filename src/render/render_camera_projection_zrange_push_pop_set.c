// render_camera_projection_zrange_push_pop_set  (Ghidra: render_camera_projection_zrange_push_pop_set,
// already named)
// address 0x50c9a0, size 235 bytes
// name confidence: 0.55   rewrite confidence: 0.65
// evidence: types/render.h's globals list: "global 0x006b8d84: float render_saved_projection_z[4]
//   render_camera_hack_frustum_z 0x50c9a0 save / restore of projection[0..3][2]." The four
//   offsets read/written (+0x14c/0x15c/0x16c/0x17c) are render_frustum.projection[0][2],
//   [1][2], [2][2] and [3][2] (the 4x4 matrix is row-major from +0x144). Disassembly
//   (objdump -d -M intel, 0x50c9a0..0x50c9cb) confirms both floats are stack arguments and ECX is
//   the frustum.
// register convention: ECX = frustum (render_frustum*), stack = (z_near, z_far).
//   // blam-cc: ECX=frustum, stack=(z_near, z_far)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "rasterizer.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern float render_saved_projection_z[4]; // 0x006b8d84, this module

// Three-way depth-range helper for the projection matrix's z column (projection[0..3][2]):
// z_near == z_far == -1.0 saves the frustum's current z-column terms to render_saved_projection_z
// ("push"); z_near == z_far == 0.0 restores them ("pop"); any other pair recomputes the z-column
// terms for that near/far range (the standard perspective z-range terms, with this engine's own
// sign convention), used to temporarily bias the depth range (e.g. for overlays/decals) without
// touching the rest of the projection matrix.
void render_camera_projection_zrange_push_pop_set(render_frustum *frustum, float z_near,
                                                   float z_far) // blam-cc: ECX=frustum, stack=(z_near, z_far)
{
    if (z_near == -1.0f && z_far == -1.0f) {
        render_saved_projection_z[0] = frustum->projection[0][2];
        render_saved_projection_z[1] = frustum->projection[1][2];
        render_saved_projection_z[2] = frustum->projection[2][2];
        render_saved_projection_z[3] = frustum->projection[3][2];
        return;
    }
    if (z_near == 0.0f && z_far == 0.0f) {
        frustum->projection[0][2] = render_saved_projection_z[0];
        frustum->projection[1][2] = render_saved_projection_z[1];
        frustum->projection[2][2] = render_saved_projection_z[2];
        frustum->projection[3][2] = render_saved_projection_z[3];
        return;
    }
    frustum->projection[0][2] = 0.0f;
    frustum->projection[1][2] = 0.0f;
    frustum->projection[2][2] = -((z_near + z_far) / (z_far - z_near));
    frustum->projection[3][2] = (z_near * z_far * -2.0f) / (z_far - z_near);
}

#if 0
Original Ghidra decompilation (0x50c9a0):

void render_camera_projection_zrange_push_pop_set(float param_1,float param_2)

{
  int in_ECX;

  if ((param_1 == -1.0) && (param_2 == -1.0)) {
    DAT_006b8d84 = *(undefined4 *)(in_ECX + 0x14c);
    DAT_006b8d88 = *(undefined4 *)(in_ECX + 0x15c);
    DAT_006b8d8c = *(undefined4 *)(in_ECX + 0x16c);
    DAT_006b8d90 = *(undefined4 *)(in_ECX + 0x17c);
    return;
  }
  if ((param_1 == 0.0) && (param_2 == 0.0)) {
    *(undefined4 *)(in_ECX + 0x14c) = DAT_006b8d84;
    *(undefined4 *)(in_ECX + 0x15c) = DAT_006b8d88;
    *(undefined4 *)(in_ECX + 0x16c) = DAT_006b8d8c;
    *(undefined4 *)(in_ECX + 0x17c) = DAT_006b8d90;
    return;
  }
  *(undefined4 *)(in_ECX + 0x14c) = 0;
  *(undefined4 *)(in_ECX + 0x15c) = 0;
  *(float *)(in_ECX + 0x16c) = -((param_1 + param_2) / (param_2 - param_1));
  *(float *)(in_ECX + 0x17c) = (param_1 * param_2 * -2.0) / (param_2 - param_1);
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
