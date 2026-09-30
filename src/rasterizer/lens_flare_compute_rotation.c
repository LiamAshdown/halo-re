// lens_flare_compute_rotation  (Ghidra: FUN_00513540)
// address 0x513540, size 556 bytes
// name confidence: 0.5   rewrite confidence: 0.9
// evidence: raw disassembly 0x513540..0x51376b (phase 4 review). Only caller is
//   lens_flare_render_all 0x513cf0 (0x513f44), with ESI = the lens_flare_instance and DI = the
//   LensFlare rotation_function word (+0x80); the product is scaled by rotation_function_scale
//   (+0x84). The phase 3 file modelled four more register arguments; they are the function's
//   own stack locals: the unpacked direction and the cross product scratch.
// What it does: unpacks the instance direction (packed_direction, 11:11:10) and returns
//   atan2(y, x) / (2 pi), or 0 when mode is 0 or y is exactly 0:
//   mode 1: t = d x (F x d) with F = view_to_world.forward; y = t . camera.forward,
//           x = -(d . camera.forward)
//   mode 2: y = F . (-d), x = -(U . (-d)) with U = view_to_world.up
//   mode 3: as mode 1 against r = position - camera.position
//   mode 4: y = F . r, x = -(U . r)
//   other:  y = 0, x = 1 (so 0).
//   Jump table 0x0051376c.
// register convention: ESI -> flare, DI -> mode.
// blam-cc: ESI -> flare, DI -> mode

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "rasterizer.h"
#include "fn_rasterizer.h"

extern rasterizer_window_parameters rasterizer_window;              // 0x007c1220
// blam-cc: EAX -> out, ECX -> packed

extern void vector3d_cross_product(real_vector3d *out, const real_vector3d *a, const real_vector3d *b); // 0x004052c0
extern double atan2(double y, double x); // x87 FPATAN

static float dot3(const real_vector3d *a, const real_vector3d *b)
{
    return a->i * b->i + a->j * b->j + a->k * b->k;
}

float lens_flare_compute_rotation(lens_flare_instance *flare, int16_t mode)
{
    const real_vector3d *forward = &rasterizer_window.frustum.view_to_world.forward;  // 0x007c12c4
    const real_vector3d *up = &rasterizer_window.frustum.view_to_world.up;            // 0x007c12dc
    real_vector3d unpacked;
    real_vector3d direction;
    real_vector3d reference;
    real_vector3d basis;
    float y = 0.0f;
    float x = 1.0f;

    direction = *vector3d_unpack_normal_11_11_10(&unpacked, flare->packed_direction);
    switch (mode) {
    case 1:
    case 3:
        vector3d_cross_product(&basis, forward, &direction);    // EAX out, ECX 0x007c12c4, stack d
        vector3d_cross_product(&basis, &direction, &basis);     // in place
        if (mode == 1) {
            reference = rasterizer_window.camera.forward;       // 0x007c1234
        } else {
            reference.i = flare->position.x - rasterizer_window.camera.position.x;
            reference.j = flare->position.y - rasterizer_window.camera.position.y;
            reference.k = flare->position.z - rasterizer_window.camera.position.z;
        }
        y = dot3(&basis, &reference);
        x = -dot3(&direction, &reference);
        break;
    case 2:
        reference.i = -direction.i;
        reference.j = -direction.j;
        reference.k = -direction.k;
        y = dot3(forward, &reference);
        x = -dot3(up, &reference);
        break;
    case 4:
        reference.i = flare->position.x - rasterizer_window.camera.position.x;
        reference.j = flare->position.y - rasterizer_window.camera.position.y;
        reference.k = flare->position.z - rasterizer_window.camera.position.z;
        y = dot3(forward, &reference);
        x = -dot3(up, &reference);
        break;
    default:
        break;
    }
    if (mode != 0 && y != 0.0f) {   // fucompp / test ah,0x44 / jnp: NaN also takes fpatan
        return (float)(atan2((double)y, (double)x) * 0.15915493667125702);
    }
    return 0.0f;
}

#if 0
Original Ghidra decompilation (0x513540): phase 3 body replaced from raw disassembly

float10 FUN_00513540(void)

{
  float fVar1;
  float fVar2;
  float fVar3;
  float *pfVar4;
  int unaff_ESI;
  short unaff_DI;
  float10 fVar5;
  float10 fVar6;
  float local_24;
  float local_20;
  float local_1c;
  float local_18;
  float local_14;
  float local_10;
  float local_c;
  float local_8;
  float local_4;
  
  pfVar4 = (float *)vector3d_unpack_normal_11_11_10();
  local_18 = *pfVar4;
  local_14 = pfVar4[1];
  local_10 = pfVar4[2];
  switch(unaff_DI) {
  case 1:
    vector3d_cross_product(&local_18);
    vector3d_cross_product(&local_c);
    local_24 = DAT_007c1234;
    local_20 = DAT_007c1238;
    local_1c = DAT_007c123c;
    goto LAB_005135ce;
  case 2:
    fVar5 = (float10)DAT_007c12c4 * (float10)-local_18 +
            (float10)DAT_007c12c8 * (float10)-local_14 + (float10)DAT_007c12cc * (float10)-local_10;
    fVar6 = -((float10)DAT_007c12dc * (float10)-local_18 +
             (float10)DAT_007c12e0 * (float10)-local_14 + (float10)DAT_007c12e4 * (float10)-local_10
             );
    break;
  case 3:
    vector3d_cross_product(&local_18);
    vector3d_cross_product(&local_c);
    local_24 = *(float *)(unaff_ESI + 4) - DAT_007c1228;
    local_20 = *(float *)(unaff_ESI + 8) - DAT_007c122c;
    local_1c = *(float *)(unaff_ESI + 0xc) - DAT_007c1230;
LAB_005135ce:
    fVar5 = (float10)local_c * (float10)local_24 +
            (float10)local_8 * (float10)local_20 + (float10)local_4 * (float10)local_1c;
    fVar6 = -((float10)local_18 * (float10)local_24 +
             (float10)local_14 * (float10)local_20 + (float10)local_10 * (float10)local_1c);
    break;
  case 4:
    fVar1 = *(float *)(unaff_ESI + 4) - DAT_007c1228;
    fVar2 = *(float *)(unaff_ESI + 8) - DAT_007c122c;
    fVar3 = *(float *)(unaff_ESI + 0xc) - DAT_007c1230;
    fVar5 = (float10)DAT_007c12c4 * (float10)fVar1 +
            (float10)DAT_007c12c8 * (float10)fVar2 + (float10)DAT_007c12cc * (float10)fVar3;
    fVar6 = -((float10)DAT_007c12dc * (float10)fVar1 +
             (float10)DAT_007c12e0 * (float10)fVar2 + (float10)DAT_007c12e4 * (float10)fVar3);
    break;
  default:
    fVar5 = (float10)0.0;
    fVar6 = (float10)1.0;
  }
  if ((unaff_DI != 0) && (fVar5 != (float10)0.0)) {
    fVar5 = (float10)fpatan(fVar5,fVar6);
    return fVar5 * (float10)0.15915494;
  }
  return (float10)0.0;
}
#endif
