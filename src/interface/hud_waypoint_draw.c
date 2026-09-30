// hud_waypoint_draw  (Ghidra: hud_waypoint_draw, already named)
// address 0x4af5e0, size 1452 bytes (to 0x4afb8b)
// name confidence: 0.5   rewrite confidence: 0.75
// evidence: rewritten from objdump 0x4af5e0..0x4afb8b in the phase-4 review (the first rewrite
// was a 0.2 literal copy with most register arguments missing). EAX is the world position of
// the target; the stack gives the local player index, the HUDGlobals waypoint arrow index
// (stride 0x68: color +0x28, opacity +0x2c, translucency +0x30, sequences +0x34 on screen,
// +0x36 off screen, +0x38 occluded, flags +0x4c), the visibility (0 on screen, 1 off screen,
// 2 occluded) and a byte that asks for the distance readout.
// Flow: the arrow scale is 0.5 beyond 15 world units, else (1 - d/15)^0.7 + 0.5, d measured
// from the unit camera position (0x568f80, ECX unit, EDI out). The point goes to camera space
// (0x4cbde0 with the matrix at 0x007c3178) and to the screen (0x50de30: ECX out, EDX point,
// ESI 0x007c3168, EDI 0x007c3114); when that fails, or for an off screen waypoint, the camera
// space x and -y are used as a direction. Outside the ellipse of the HUD safe area (HUDGlobals
// +0x120..+0x12c offsets from 640x480) the point is pulled onto the ellipse, the waypoint
// becomes off screen and, unless arrow flags bit 0 is set, the arrow is rotated by
// -atan2(x, y). The arrow is drawn centered (anchor 4) with hud_draw_bitmap_at. For an on screen
// waypoint with the readout byte set, the distance in meters (world units * 3.048) is drawn
// with hud_draw_number below the arrow (three digits, one fraction digit, trailing m).
// Behaviour kept from the binary: the alpha is (uint8_t)-trunc(opacity) inside the 0..255
// check, so only an opacity of 1.0 gives 0xff; without the readout byte the color becomes
// (blue, 0, 0); hud_draw_number gets flags 0, flash start 0 and scale 0.0.
// register convention: EAX position; four stack arguments.
//   // blam-cc: position -> EAX

#include <string.h>
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "game.h"
#include "networking.h"
#include "interface.h"
#include "fn_bitmaps.h"

extern HUDGlobals *hud_globals_tag_data; // 0x0071941c
extern player_globals *local_player_globals;  // 0x0087a478
extern data_array *player_data;               // 0x0087a480
extern real_matrix4x3 render_camera_world_to_view; // 0x007c3178, UNSURE name
extern uint8_t render_frustum_global[];       // 0x007c3168, UNSURE name/type (ESI of 0x50de30)
extern uint8_t render_camera_global[];               // 0x007c3114, UNSURE name/type (EDI of 0x50de30)
extern int16_t render_viewport_top;           // 0x007c3140
extern int16_t render_viewport_left;          // 0x007c3142
extern Rectangle2D screen_safe_area_right;      // 0x007c3148, UNSURE name

extern float sqrtf(float x);
extern float atan2f(float y, float x);
extern double pow(double base, double exponent); // 0x6283c0, MSVC 7.1 CRT _CIpow
extern double fmod(double x, double y); // 0x628cca, MSVC 7.1 CRT _CIfmod
extern long lrint(double x); // x87 fistp under the default control word (round-half-to-even)
extern int32_t __ftol(double x); // 0x006391b4, MSVC 7.1 CRT float-to-int truncation
extern int32_t ui_real_to_int_truncate(float value); // 0x4ab590
extern void unit_get_camera_position(datum_index unit_index, real_point3d *out); // 0x568f80, blam-cc: ECX unit_index, EDI out
extern void matrix4x3_transform_point(real_point3d *out, real_point3d *point, real_matrix4x3 *m); // 0x4cbde0, blam-cc: EAX out, EDX point
extern uint8_t render_project_world_point_to_screen(real_point2d *out, const real_point3d *point, void *frustum,
                                                    void *camera); // 0x50de30, blam-cc: ECX out, EDX point, ESI frustum, EDI camera

extern uint32_t color_rgb_float_to_int(const float *rgb); // 0x4ab5d0
extern void hud_meter_resolve_bitmap_frame(datum_index bitmap_tag, int16_t sequence_index, uint16_t frame_index,
                                           void **out_data, int32_t *out_offset); // 0x4ab8d0, blam-cc: EAX frame_index
extern void *texture_cache_get(BitmapData *bitmap, uint8_t wait, uint8_t allocate_if_missing); // 0x444550, blam-cc: EAX bitmap
extern void hud_draw_bitmap_at(const float *uv, BitmapData *bitmap, uint8_t pixel_uvs, int16_t anchor,
                               const Point2DInt *screen_position, float scale, float rotation, uint32_t color); // 0x4acbb0, blam-cc: EAX uv, EDX bitmap, CL pixel_uvs
extern void hud_draw_number(void *unused, uint16_t *anchor, const hud_number_placement *placement, int16_t value,
                            int16_t fraction, uint32_t flags, int32_t flash_start_time, float scale); // 0x4ac0b0

static float hud_clamp01(float value)
{
    if (value < 0.0f) {
        return 0.0f;
    }
    if (value > 1.0f) {
        return 1.0f;
    }
    return value;
}

// blam-cc: position -> EAX
void hud_waypoint_draw(const real_point3d *position, int16_t local_player_index, int16_t arrow_index,
                       int16_t visibility, uint8_t show_distance)
{
    HUDGlobals *globals = hud_globals_tag_data;
    const HUDGlobalsWaypointArrow *arrow =
        (const HUDGlobalsWaypointArrow *)globals->waypoint_arrows.pointer + arrow_index;
    real_point3d point;
    real_point3d camera;
    real_point2d screen;
    datum_index unit_index;
    float distance;
    float scale;
    float x;
    float y;
    float half_width;
    float half_height;
    float rotation;
    BitmapData *bitmap;
    int32_t uv_offset;
    const float *uv;
    Point2DInt arrow_position;
    ColorRGB color;
    uint8_t alpha;
    int32_t whole;
    uint32_t packed;

    point = *position;
    unit_index = (datum_index)-1;
    if (local_player_index != -1 && local_player_index < 1 &&
        local_player_globals->local_players[local_player_index] != (datum_index)-1) {
        unit_index = ((player *)((uint8_t *)player_data->data +
                                 (local_player_globals->local_players[local_player_index] & 0xffff) * 0x200))->unit;
    }
    unit_get_camera_position(unit_index, &camera);
    {
        float dx = position->x - camera.x;
        float dy = position->y - camera.y;
        float dz = position->z - camera.z;
        distance = sqrtf(dz * dz + dy * dy + dx * dx);
    }
    if (distance > 15.0f) {
        scale = 0.5f;
    } else {
        scale = (float)(pow((double)(1.0f - distance * 0.06666667014360428f), 0.7) + 0.5);
    }

    matrix4x3_transform_point(&point, &point, &render_camera_world_to_view);
    if (visibility != 1 && render_project_world_point_to_screen(&screen, &point, render_frustum_global, render_camera_global) != 0) {
        x = screen.x - (float)(render_viewport_left + 0x140);
        y = screen.y - (float)(render_viewport_top + 0xf0);
    } else {
        x = point.x;
        visibility = 1;
        y = -point.y;
    }

    half_width = (640.0f - (globals->left_offset + globals->right_offset)) * 0.5f;
    half_height = (480.0f - (globals->top_offset + globals->bottom_offset)) * 0.5f;
    rotation = 0.0f;
    {
        float radius = half_height * half_width;
        float scaled_x = half_height * x;
        float scaled_y = half_width * y;

        if (visibility == 1 || !(scaled_y * scaled_y + scaled_x * scaled_x < radius * radius)) {
            float k = sqrtf((radius * radius) / (scaled_y * scaled_y + scaled_x * scaled_x));
            visibility = 1;
            x = x * k;
            y = y * k;
            if ((arrow->flags & 1) == 0) {
                rotation = -atan2f(x, y);
            }
        }
    }
    screen.x = x + 320.0f;
    screen.y = y + 240.0f;

    bitmap = 0;
    uv_offset = 0;
    hud_meter_resolve_bitmap_frame(*(datum_index *)&globals->arrow_bitmap.tag_id,
                                   (int16_t)(&arrow->on_screen_sequence_index)[visibility], 0, (void **)&bitmap,
                                   &uv_offset);
    if (bitmap == 0 || texture_cache_get(bitmap, 0, 1) == 0) {
        return;
    }
    uv = (const float *)uv_offset;
    arrow_position.x = (int16_t)__ftol((double)screen.x);
    arrow_position.y = (int16_t)__ftol((double)screen.y);

    whole = ui_real_to_int_truncate(arrow->opacity);
    if (whole * 0xff < 0) {
        alpha = 0;
    } else if (whole * 0xff > 0xff) {
        alpha = 0xff;
    } else {
        alpha = (uint8_t)-(int8_t)ui_real_to_int_truncate(arrow->opacity);
    }
    color_rgb_int_to_real(&color, *(const uint32_t *)&arrow->color);
    color.red = hud_clamp01(1.0f - arrow->translucency) * color.red;
    color.green = hud_clamp01(1.0f - arrow->translucency) * color.green;
    color.blue = hud_clamp01(1.0f - arrow->translucency) * color.blue;
    if (show_distance == 0) {
        color.red = color.blue;
        color.green = 0.0f;
        color.blue = 0.0f;
    }
    packed = color_rgb_float_to_int(&color.red) | ((uint32_t)alpha << 24);
    hud_draw_bitmap_at(uv, bitmap, 0, 4, &arrow_position, scale, rotation, packed);

    if (visibility == 1 || show_distance == 0) {
        return;
    }
    {
        uint16_t anchor[0x12];
        hud_number_placement placement;
        float meters = distance * 3.048f;
        float power;
        int16_t number_x;
        int16_t number_y;

        memset(anchor, 0, sizeof(anchor)); // anchor 0 (top left), followed by zeroes
        memset(&placement, 0, sizeof(placement));
        packed = color_rgb_float_to_int(&color.red) | ((uint32_t)alpha << 24);
        *(uint32_t *)&placement.flash.default_color = packed;
        packed = color_rgb_float_to_int(&color.red) | ((uint32_t)alpha << 24);
        *(uint32_t *)&placement.flash.flashing_color = packed;
        placement.maximum_number_of_digits = 3;
        placement.number_of_fractional_digits = 1;
        placement.flags = 5; // show leading zeros, trailing m
        number_x = (int16_t)__ftol((double)((uv[1] - uv[0]) * (float)(int16_t)bitmap->width * 0.5f * scale * 0.33f +
                                            (float)arrow_position.x));
        number_y = (int16_t)__ftol((double)((uv[3] - uv[2]) * (float)(int16_t)bitmap->height * 0.5f * scale * 0.66f +
                                            (float)arrow_position.y));
        placement.anchor_offset.x = (int16_t)(number_x + (int16_t)(render_viewport_left - screen_safe_area_right.left));
        placement.anchor_offset.y = (int16_t)(number_y + (int16_t)(render_viewport_top - screen_safe_area_right.top));
        power = (float)pow(10.0, 4.0);
        whole = (int32_t)lrint(fmod((double)(power * meters < 0.0f ? -(power * meters) : power * meters), (double)power));
        hud_draw_number((void *)(int32_t)local_player_index, anchor, &placement,
                        (int16_t)ui_real_to_int_truncate(meters), (int16_t)whole, 0, 0, 0.0f);
    }
}

#if 0
Original Ghidra decompilation (0x4af5e0):

void hud_waypoint_draw(undefined4 param_1,short param_2,short param_3,char param_4)

{
  float fVar1;
  float fVar2;
  float fVar3;
  char cVar4;
  undefined2 uVar5;
  short sVar6;
  float *in_EAX;
  int iVar7;
  uint uVar8;
  undefined4 uVar9;
  int iVar10;
  uint uVar11;
  short *psVar12;
  float10 fVar13;
  float10 fVar14;
  float10 fVar15;
  float10 fVar16;
  float local_a8;
  byte local_a1;
  undefined4 local_a0;
  float local_9c;
  float local_98;
  float local_94;
  undefined4 local_90;
  float local_8c;
  float local_88;
  float local_84;
  float local_80;
  float local_7c;
  undefined4 local_78;
  undefined4 local_74;
  undefined4 local_70;
  undefined4 local_6c;
  undefined4 local_68;
  undefined4 local_64;
  undefined4 local_60;
  undefined4 local_5c;
  undefined4 local_58;
  short local_54;
  short local_52;
  uint local_30;
  uint local_2c;
  undefined1 local_10;
  undefined1 local_f;
  undefined1 local_e;

  iVar10 = param_2 * 0x68 + *(int *)(DAT_0071941c + 0x164);
  local_84 = *in_EAX;
  local_80 = in_EAX[1];
  local_7c = in_EAX[2];
  unit_get_camera_position();
  local_88 = SQRT((*in_EAX - local_9c) * (*in_EAX - local_9c) +
                  (in_EAX[1] - local_98) * (in_EAX[1] - local_98) +
                  (in_EAX[2] - local_94) * (in_EAX[2] - local_94));
  if (local_88 <= 15.0) {
    fVar13 = (float10)FUN_006283c0();
    local_8c = (float)(fVar13 + (float10)0.5);
  }
  else {
    local_8c = 0.5;
  }
  matrix4x3_transform_point(&DAT_007c3178);
  if ((param_3 == 1) || (cVar4 = render_project_world_point_to_screen(), cVar4 == '\0')) {
    fVar13 = (float10)local_84;
    param_3 = 1;
    fVar14 = -(float10)local_80;
  }
  else {
    fVar13 = (float10)local_9c - (float10)(DAT_007c3140._2_2_ + 0x140);
    fVar14 = (float10)local_98 - (float10)((short)DAT_007c3140 + 0xf0);
  }
  local_a8 = 0.0;
  fVar15 = ((float10)640.0 -
           ((float10)*(float *)(DAT_0071941c + 300) + (float10)*(float *)(DAT_0071941c + 0x128))) *
           (float10)0.5;
  fVar16 = ((float10)480.0 -
           ((float10)*(float *)(DAT_0071941c + 0x124) + (float10)*(float *)(DAT_0071941c + 0x120)))
           * (float10)0.5;
  fVar1 = (float)(fVar16 * fVar15);
  fVar2 = (float)(fVar16 * fVar13);
  fVar15 = fVar15 * fVar14;
  if ((param_3 == 1) ||
     ((float10)fVar1 * (float10)fVar1 <= (float10)fVar2 * (float10)fVar2 + fVar15 * fVar15)) {
    param_3 = 1;
    fVar16 = SQRT(((float10)fVar1 * (float10)fVar1) /
                  ((float10)fVar2 * (float10)fVar2 + fVar15 * fVar15));
    fVar13 = fVar16 * fVar13;
    fVar14 = fVar16 * fVar14;
    if ((*(byte *)(iVar10 + 0x4c) & 1) == 0) {
      fVar16 = (float10)fpatan(fVar13,fVar14);
      local_a8 = (float)-fVar16;
    }
  }
  local_9c = (float)(fVar13 + (float10)320.0);
  local_98 = (float)(fVar14 + (float10)240.0);
  local_a0 = 0;
  local_90 = 0;
  FUN_004ab8d0(*(undefined4 *)(DAT_0071941c + 0x15c),*(undefined2 *)(iVar10 + 0x34 + param_3 * 2),
               &local_a0,&local_90);
  if ((local_a0 != 0) && (iVar7 = texture_cache_get(0,1), iVar7 != 0)) {
    uVar5 = __ftol();
    local_a0 = CONCAT22(local_a0._2_2_,uVar5);
    uVar5 = __ftol();
    local_a0 = CONCAT22(uVar5,(undefined2)local_a0);
    iVar7 = FUN_004ab590(*(undefined4 *)(iVar10 + 0x2c));
    if (iVar7 * 0xff < 0) {
      local_a1 = 0;
    }
    else {
      iVar7 = FUN_004ab590(*(undefined4 *)(iVar10 + 0x2c));
      if (iVar7 * 0xff < 0x100) {
        cVar4 = FUN_004ab590(*(undefined4 *)(iVar10 + 0x2c));
        local_a1 = -cVar4;
      }
      else {
        local_a1 = 0xff;
      }
    }
    color_rgb_int_to_real();
    fVar1 = 1.0 - *(float *)(iVar10 + 0x30);
    if (0.0 <= fVar1) {
      fVar2 = fVar1;
      if (1.0 < fVar1) {
        fVar2 = 1.0;
      }
    }
    else {
      fVar2 = 0.0;
    }
    if (0.0 <= fVar1) {
      fVar3 = fVar1;
      if (1.0 < fVar1) {
        fVar3 = 1.0;
      }
    }
    else {
      fVar3 = 0.0;
    }
    local_98 = fVar3 * local_98;
    if (0.0 <= fVar1) {
      if (1.0 < fVar1) {
        fVar1 = 1.0;
      }
    }
    else {
      fVar1 = 0.0;
    }
    fVar1 = fVar1 * local_94;
    local_9c = fVar2 * local_9c;
    local_94 = fVar1;
    if (param_4 == '\0') {
      local_98 = 0.0;
      local_94 = 0.0;
      local_9c = fVar1;
    }
    uVar11 = (uint)local_a1 << 0x18;
    uVar8 = color_rgb_float_to_int(&local_9c);
    FUN_004acbb0(4,&local_a0,local_8c,local_a8,uVar8 | uVar11);
    if ((param_3 != 1) && (param_4 != '\0')) {
      local_88 = local_88 * 3.048;
      local_74 = 0;
      local_70 = 0;
      local_6c = 0;
      local_68 = 0;
      local_64 = 0;
      local_60 = 0;
      local_5c = 0;
      local_58 = 0;
      psVar12 = &local_54;
      for (iVar10 = 0x15; iVar10 != 0; iVar10 = iVar10 + -1) {
        psVar12[0] = 0;
        psVar12[1] = 0;
        psVar12 = psVar12 + 2;
      }
      local_78 = 0;
      local_30 = color_rgb_float_to_int(&local_9c);
      local_30 = local_30 | uVar11;
      local_2c = color_rgb_float_to_int(&local_9c);
      local_2c = local_2c | uVar11;
      local_10 = 3;
      local_e = 1;
      local_f = 5;
      sVar6 = __ftol();
      local_52 = __ftol();
      local_54 = sVar6 + (DAT_007c3140._2_2_ - DAT_007c3148._2_2_);
      local_52 = local_52 + ((short)DAT_007c3140 - (short)DAT_007c3148);
      FUN_006283c0();
      fVar13 = (float10)FUN_00628cca();
      local_8c = (float)(int)ROUND((float)fVar13);
      uVar9 = FUN_004ab590(local_88,local_8c,0,0,0);
      FUN_004ac0b0(param_1,&local_78,&local_54,uVar9);
    }
  }
  return;
}
#endif
