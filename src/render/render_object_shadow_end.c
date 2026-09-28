// render_object_shadow_end  (Ghidra: shadow_compute_bounding_box_and_register; CEA
// render_object_shadow_end(data), hint only; renamed)
// address 0x50f980, size 833 bytes
// name confidence: 0.6   rewrite confidence: 0.8
// evidence: objdump -d -M intel 0x50f980..0x50fcc0, re-traced in the phase-4 review with the
//   FPU stack and the push depth followed at every step.
//   - ECX = object_render_data; the shadow basis is data->shadow_matrix (forward +0x10, left
//     +0x1c, up +0x28, position +0x34) and the radius data->shadow_radius (+0x40).
//   - six bounding planes on the stack (esp+0x18 of the frame, 0x60 bytes): (up, up.p - r/2),
//     (-up, -up.p - 4r), (forward, f.p - r), (-forward, -f.p - r), (left, l.p - r),
//     (-left, -l.p - r) -- the shadow volume box, stretched 4r away from the light along -up.
//   - a world box (esp+0x00, a real_rectangle3d): for x and y the up component is scaled by
//     -0.5 on the side it points to and by 4 on the other (fcomp / test ah,0x41 / jp: up > 0
//     picks -0.5 for the lower bound; up <= 0 picks 4), plus / minus |forward| + |left|; for z
//     the lower bound always uses 4 up.k and the upper -0.5 up.k. Everything times r plus the
//     position.
//   - structure_debug_draw_surfaces_simple 0x552b40 (ECX = 6 planes, stack (&position, 4r,
//     &box, planes)) draws the BSP surfaces inside the volume with the shadow texture; then,
//     for the main window (rasterizer_window.type == 1, 0x007c1220) with 0x0069c689 clear,
//     object shadows on (0x006893f2) and 0x0069e550 not yet set,
//     rasterizer_render_target_set_active 0x52ccc0 (EAX = 1, stack (0, 0)) restores the window
//     target once per frame and latches 0x0069e550.
// review fix (phase-4 gate): the first draft passed the planes as an anonymous float[24] and
//   dropped the ECX plane count of 0x552b40 and the EAX target of 0x52ccc0; both are restored,
//   and the planes and box are typed.
// register convention: ECX = object_render_data*.
//   // blam-cc: ECX -> data
// UNSURE: the meaning of 0x0069c689 (a rasterizer state byte also read by 0x530ff0).

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "rasterizer.h"
#include "render.h"

extern rasterizer_window_parameters rasterizer_window;      // 0x007c1220, rasterizer module
extern uint8_t rasterizer_caps_flag_689;                            // 0x0069c689 UNSURE
extern uint8_t console_debug_toggle_6893f2;                 // 0x006893f2 object shadows enabled
extern uint8_t rasterizer_object_shadow_window_restored;    // 0x0069e550, rasterizer module

extern void structure_debug_draw_surfaces_simple(real_point3d *query_point, float radius,
    real_rectangle3d *query_box, real_plane3d *planes, int16_t plane_count);
    // 0x552b40, structures module; blam-cc: stack -> (query_point, radius, query_box, planes),
    // ECX -> plane_count
extern void rasterizer_render_target_set_active(int16_t target_index, uint32_t clear_color,
                                                uint8_t clear);
    // 0x52ccc0, rasterizer module; blam-cc: EAX -> target_index, stack -> (clear_color, clear)

// Draws the fake shadow of the object in data onto the BSP surfaces inside its shadow volume
// (a box around the shadow basis, 4 radii deep along the light) and restores the window render
// target the first time a shadow is finished in the main window.
void render_object_shadow_end(object_render_data *data) // blam-cc: ECX -> data
{
    real_vector3d *forward = &data->shadow_matrix.forward;
    real_vector3d *left = &data->shadow_matrix.left;
    real_vector3d *up = &data->shadow_matrix.up;
    real_point3d *position = &data->shadow_matrix.position;
    real_plane3d planes[6];
    real_rectangle3d box;
    float d;
    float sx;
    float sy;
    float sz;
    float lower;
    float upper;

    d = up->k * position->z + up->j * position->y + up->i * position->x;
    planes[0].normal = *up;
    planes[0].d = d - data->shadow_radius * 0.5f;
    planes[1].normal.i = -up->i;
    planes[1].normal.j = -up->j;
    planes[1].normal.k = -up->k;
    planes[1].d = -d - data->shadow_radius * 4.0f;

    d = forward->k * position->z + forward->j * position->y + forward->i * position->x;
    planes[2].normal = *forward;
    planes[2].d = d - data->shadow_radius;
    planes[3].normal.i = -forward->i;
    planes[3].normal.j = -forward->j;
    planes[3].normal.k = -forward->k;
    planes[3].d = -d - data->shadow_radius;

    d = position->z * left->k + position->y * left->j + position->x * left->i;
    planes[4].normal = *left;
    planes[4].d = d - data->shadow_radius;
    planes[5].normal.i = -left->i;
    planes[5].normal.j = -left->j;
    planes[5].normal.k = -left->k;
    planes[5].d = -d - data->shadow_radius;

    sx = (left->i < 0.0f ? -left->i : left->i) + (forward->i < 0.0f ? -forward->i : forward->i);
    sy = (left->j < 0.0f ? -left->j : left->j) + (forward->j < 0.0f ? -forward->j : forward->j);
    sz = (forward->k < 0.0f ? -forward->k : forward->k) + (left->k < 0.0f ? -left->k : left->k);

    lower = up->i * (up->i > 0.0f ? -0.5f : 4.0f) + -sx;
    upper = up->i * (up->i > 0.0f ? 4.0f : -0.5f) + sx;
    box.x.lower = lower * data->shadow_radius + position->x;
    box.x.upper = upper * data->shadow_radius + position->x;
    lower = up->j * (up->j > 0.0f ? -0.5f : 4.0f) + -sy;
    upper = up->j * (up->j > 0.0f ? 4.0f : -0.5f) + sy;
    box.y.lower = lower * data->shadow_radius + position->y;
    box.y.upper = upper * data->shadow_radius + position->y;
    lower = up->k * 4.0f + -sz;
    upper = sz - up->k * 0.5f;
    box.z.lower = lower * data->shadow_radius + position->z;
    box.z.upper = upper * data->shadow_radius + position->z;

    structure_debug_draw_surfaces_simple(position, data->shadow_radius * 4.0f, &box, planes, 6);

    if (rasterizer_window.type == 1 && rasterizer_caps_flag_689 == 0 && console_debug_toggle_6893f2 != 0 &&
        rasterizer_object_shadow_window_restored == 0) {
        rasterizer_render_target_set_active(1, 0, 0);
        rasterizer_object_shadow_window_restored = 1;
    }
}

#if 0
Original Ghidra decompilation (0x50f980):

void shadow_compute_bounding_box_and_register(void)

{
  float *pfVar1;
  float *pfVar2;
  float *pfVar3;
  float *pfVar4;
  float fVar5;
  float fVar6;
  float fVar7;
  float fVar8;
  float fVar9;
  float fVar10;
  float fVar11;
  float fVar12;
  float fVar13;
  float fVar14;
  int in_ECX;
  float local_78;
  float local_74;
  float local_70;
  float local_6c;
  float local_68;
  float local_64;
  float local_60;
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

  pfVar1 = (float *)(in_ECX + 0x34);
  pfVar2 = (float *)(in_ECX + 0x28);
  local_60 = *pfVar2;
  local_5c = *(float *)(in_ECX + 0x2c);
  local_58 = *(float *)(in_ECX + 0x30);
  fVar5 = *pfVar2 * *pfVar1 +
          *(float *)(in_ECX + 0x2c) * *(float *)(in_ECX + 0x38) +
          *(float *)(in_ECX + 0x30) * *(float *)(in_ECX + 0x3c);
  pfVar3 = (float *)(in_ECX + 0x10);
  local_50 = -local_60;
  local_40 = *pfVar3;
  local_4c = -local_5c;
  local_3c = *(float *)(in_ECX + 0x14);
  local_38 = *(float *)(in_ECX + 0x18);
  local_48 = -local_58;
  pfVar4 = (float *)(in_ECX + 0x1c);
  local_20 = *pfVar4;
  local_1c = *(float *)(in_ECX + 0x20);
  local_18 = *(float *)(in_ECX + 0x24);
  local_54 = fVar5 - *(float *)(in_ECX + 0x40) * 0.5;
  local_44 = -fVar5 - *(float *)(in_ECX + 0x40) * 4.0;
  fVar5 = *pfVar3 * *pfVar1 +
          *(float *)(in_ECX + 0x14) * *(float *)(in_ECX + 0x38) +
          *(float *)(in_ECX + 0x18) * *(float *)(in_ECX + 0x3c);
  local_30 = -local_40;
  local_2c = -local_3c;
  local_28 = -local_38;
  local_34 = fVar5 - *(float *)(in_ECX + 0x40);
  local_24 = -fVar5 - *(float *)(in_ECX + 0x40);
  fVar5 = *pfVar1 * *pfVar4 +
          *(float *)(in_ECX + 0x38) * *(float *)(in_ECX + 0x20) +
          *(float *)(in_ECX + 0x3c) * *(float *)(in_ECX + 0x24);
  local_10 = -local_20;
  local_c = -local_1c;
  local_8 = -local_18;
  local_14 = fVar5 - *(float *)(in_ECX + 0x40);
  local_4 = -fVar5 - *(float *)(in_ECX + 0x40);
  fVar5 = *pfVar3;
  if (*pfVar3 < 0.0) {
    fVar5 = -fVar5;
  }
  fVar6 = *pfVar4;
  if (*pfVar4 < 0.0) {
    fVar6 = -fVar6;
  }
  fVar7 = *(float *)(in_ECX + 0x14);
  if (*(float *)(in_ECX + 0x14) < 0.0) {
    fVar7 = -fVar7;
  }
  fVar8 = *(float *)(in_ECX + 0x20);
  if (*(float *)(in_ECX + 0x20) < 0.0) {
    fVar8 = -fVar8;
  }
  fVar9 = *(float *)(in_ECX + 0x18);
  if (*(float *)(in_ECX + 0x18) < 0.0) {
    fVar9 = -fVar9;
  }
  fVar10 = *(float *)(in_ECX + 0x24);
  if (*(float *)(in_ECX + 0x24) < 0.0) {
    fVar10 = -fVar10;
  }
  if (*pfVar2 < 0.0 == (*pfVar2 == 0.0)) {
    fVar11 = -0.5;
  }
  else {
    fVar11 = 4.0;
  }
  if (*pfVar2 <= 0.0) {
    fVar12 = -0.5;
  }
  else {
    fVar12 = 4.0;
  }
  if (*(float *)(in_ECX + 0x2c) < 0.0 == (*(float *)(in_ECX + 0x2c) == 0.0)) {
    fVar13 = -0.5;
  }
  else {
    fVar13 = 4.0;
  }
  if (*(float *)(in_ECX + 0x2c) <= 0.0) {
    fVar14 = -0.5;
  }
  else {
    fVar14 = 4.0;
  }
  local_78 = (*pfVar2 * fVar11 + -(fVar6 + fVar5)) * *(float *)(in_ECX + 0x40) + *pfVar1;
  local_74 = (*pfVar2 * fVar12 + fVar6 + fVar5) * *(float *)(in_ECX + 0x40) + *pfVar1;
  local_70 = (*(float *)(in_ECX + 0x2c) * fVar13 + -(fVar8 + fVar7)) * *(float *)(in_ECX + 0x40) +
             *(float *)(in_ECX + 0x38);
  local_6c = (*(float *)(in_ECX + 0x2c) * fVar14 + fVar8 + fVar7) * *(float *)(in_ECX + 0x40) +
             *(float *)(in_ECX + 0x38);
  local_68 = (*(float *)(in_ECX + 0x30) * 4.0 + -(fVar10 + fVar9)) * *(float *)(in_ECX + 0x40) +
             *(float *)(in_ECX + 0x3c);
  local_64 = ((fVar10 + fVar9) - *(float *)(in_ECX + 0x30) * 0.5) * *(float *)(in_ECX + 0x40) +
             *(float *)(in_ECX + 0x3c);
  FUN_00552b40(pfVar1,*(float *)(in_ECX + 0x40) * 4.0,&local_78,&local_60);
  if (((((short)DAT_007c1220 == 1) && (DAT_0069c689 == '\0')) && (DAT_006893f2 != '\0')) &&
     (DAT_0069e550 == '\0')) {
    FUN_0052ccc0(0,0);
    DAT_0069e550 = '\x01';
  }
  return;
}
#endif
