// hud_draw_multitexture_overlay  (Ghidra: FUN_004acfe0, renamed in the phase-4 review)
// address 0x4acfe0, size 2224 bytes (to 0x4ad88f; jump tables 0x4ad890 blend remap,
// 0x4ad8a4 effector source, 0x4ad8c4 effector destination)
// name confidence: 0.7 (chosen)   rewrite confidence: 0.75
// evidence: rewritten from objdump 0x4acfe0..0x4ad88f in the phase-4 review; the only caller is
// hud_draw_static_element (0x4ac6f0). Argument 1 is a HUDInterfaceMultitextureOverlay
// (types/tags.h, stride 0x1e0): offsets +0x4c..+0x63 copied to the stack, maps +0x70/+0x80/+0x90,
// scales +0x34.., wrap modes +0x94.., blend functions +0x2e/+0x30 and +0x04, effectors
// +0x154/+0x158 (HUDInterfaceMultitextureOverlayEffector, stride 0xdc: destination type +0x40,
// destination +0x42, source +0x44, in bounds +0x48, out bounds +0x50, tints +0x98/+0xa4). The
// ui_quad_render_state fields documented in types/interface.h come from this function. The
// first rewrite had no EAX scale, placed the vertex loop on the wrong buffer, read the local
// player index as a unit and guessed the effector cases.
// Behaviour kept from the binary: the player record is computed from index 0xffff when the
// local player index is -1 (the ammo state call then reads a stale record); an effector whose
// source is out of range reuses the previous value (initially the extents[0] temporary of the
// vertex loop); an offset effector on a map whose bitmap is missing writes through a NULL
// map_offsets slot; the float at 0x00719428 grows by 0.05 per effector and is never read here.
// register convention: EAX scale (float[2]); seven stack arguments.
//   // blam-cc: scale -> EAX
// reconciled: R34 player_globals.unknown_0c -> local_player_count (int16 at +0x0c, same width)

#include <string.h>
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "game.h"
#include "networking.h"
#include "objects.h"
#include "units.h"
#include "items.h"
#include "interface.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern float sinf(float x);
extern float cosf(float x);
extern float sqrtf(float x);
extern float atan2f(float y, float x);
extern long lrint(double x); // x87 fistp under the default control word (round-half-to-even)

extern data_array *object_data; // 0x008603b0
extern player_globals *local_player_globals;                 // 0x0087a478
extern data_array *player_data;                              // 0x0087a480
extern player_control_globals *player_control_globals_ptr;   // 0x006b145c
extern float hud_multitexture_effector_counter;              // 0x00719428, += 0.05 per effector, never read here

extern BitmapData *bitmap_group_sequence_get_bitmap_data(datum_index bitmap_tag, int16_t frame, int16_t sequence); // 0x43f290, blam-cc: EAX tag, DI frame
extern ColorRGB *color_interpolate(ColorRGB *color1, ColorRGB *color0, ColorRGB *dest, uint32_t flags, float t); // 0x43f6a0, EAX color1, ECX color0
extern uint8_t hud_player_weapon_ammo_state(const player *p, weapon_hud_ammo_state *out); // 0x4acef0, blam-cc: EAX player
extern void rasterizer_ui_quad_draw(ui_quad_render_state *state, hud_quad_vertex *vertices); // 0x51c9a0, rasterizer quad submitter, blam-cc: EAX state

static datum_index hud_local_player_index_to_player(int16_t local_player_index)
{
    if (local_player_index != -1 && local_player_index < 1) {
        return local_player_globals->local_players[local_player_index];
    }
    return (datum_index)-1;
}

// blam-cc: scale -> EAX
// Draws one multitexture overlay of a HUD static element: up to three bitmaps combined by the
// overlay blend functions, animated by its effectors (tint, horizontal and vertical offset,
// fade, geometry offset) from the local player aim pitch, weapon ammo, heat or zoom level.
void hud_draw_multitexture_overlay(const float *scale, const HUDInterfaceMultitextureOverlay *overlay,
                                   int16_t local_player_index, const Point2DInt *screen_position,
                                   const float *uv, const float *extents, float rotation, uint32_t color)
{
    ui_quad_render_state state;
    hud_quad_vertex vertices[4];
    weapon_hud_ammo_state ammo;
    Point2D offsets[3];
    ColorRGB tints[3];
    ColorRGB tint;
    float fades[3];
    float geometry_offset[2];
    float sine;
    float cosine;
    float value;
    float output;
    float x;
    int16_t i;

    offsets[0] = overlay->primary_offset;
    offsets[1] = overlay->secondary_offset;
    offsets[2] = overlay->tertiary_offset;
    memset(tints, 0, sizeof(tints));
    fades[0] = 1.0f;
    fades[1] = 1.0f;
    fades[2] = 1.0f;
    geometry_offset[0] = 0.0f;
    geometry_offset[1] = 0.0f;
    sine = sinf(rotation);
    cosine = cosf(rotation);

    hud_player_weapon_ammo_state(
        (const player *)((uint8_t *)player_data->data + (hud_local_player_index_to_player(local_player_index) & 0xffff) * 0x200),
        &ammo); // the result is not tested

    x = 0.0f;
    for (i = 0; i < 4; i++) {
        int32_t corner = i + 1;
        float u = (corner & 2) != 0 ? uv[1] : uv[0];
        float v = i > 1 ? uv[3] : uv[2];
        float y = i > 1 ? extents[3] : extents[2];
        float rotated;

        x = (corner & 2) != 0 ? extents[1] : extents[0];
        rotated = (x * cosine - y * sine) * scale[0];
        vertices[i].x = (float)(screen_position->x + (int32_t)lrint(rotated));
        rotated = (y * cosine + x * sine) * scale[1];
        vertices[i].y = (float)(screen_position->y + (int32_t)lrint(rotated));
        vertices[i].z = 0.0f;
        vertices[i].color = color;
        vertices[i].u = u;
        vertices[i].v = v;
    }
    value = x; // stack slot shared with the loop temporary, see the header

    memset(&state, 0, sizeof(state));
    state.map_texel_scales[0].y = 1.0f;
    state.map_texel_scales[0].x = 1.0f;
    state.map_scales[0].y = 1.0f;
    state.map_scales[0].x = 1.0f;
    state.meter_parameters = 0;
    state.single_local_player = local_player_globals->local_player_count == 1;
    state.maps[0] = bitmap_group_sequence_get_bitmap_data(*(const datum_index *)&overlay->primary.tag_id, 0, 0);
    state.maps[1] = bitmap_group_sequence_get_bitmap_data(*(const datum_index *)&overlay->secondary.tag_id, 0, 0);
    state.maps[2] = bitmap_group_sequence_get_bitmap_data(*(const datum_index *)&overlay->tertiary.tag_id, 0, 0);

    for (i = 0; i < 3; i++) {
        const BitmapData *map = state.maps[i];

        if (map != 0) {
            const Point2D *map_scale = &(&overlay->primary_scale)[i];
            int32_t width = (int16_t)map->width;
            int32_t height = (int16_t)map->height;
            float u_scale = 1.0f;
            float v_scale = 1.0f;

            if (map_scale->x != 0.0f) {
                u_scale = 1.0f / map_scale->x;
            }
            if (map_scale->y != 0.0f) {
                v_scale = 1.0f / map_scale->y;
            }
            if ((width & (width - 1)) != 0 || (height & (height - 1)) != 0) {
                state.map_texel_scales[i].x = 1.0f / (float)(int16_t)map->width;
                state.map_texel_scales[i].y = 1.0f / (float)(int16_t)map->height;
            } else {
                state.map_texel_scales[i].x = 1.0f;
                state.map_texel_scales[i].y = 1.0f;
            }
            state.map_offsets[i] = &offsets[i];
            state.map_scales[i].x = u_scale;
            state.map_scales[i].y = v_scale;
            state.wrap_modes[i] = (uint8_t)(&overlay->primary_wrap_mode)[i];
        }
        if (i < 2) {
            int16_t *blend = i == 0 ? &state.zero_to_one_blend : &state.one_to_two_blend;

            switch ((&overlay->zero_to_one_blend_function)[i]) {
            case 0: *blend = 0; break; // add
            case 1: *blend = 2; break; // subtract
            case 2: *blend = 1; break; // multiply
            case 3: *blend = 3; break; // multiply2x
            case 4: *blend = 4; break; // dot
            }
        } else {
            state.framebuffer_blend_function = overlay->framebuffer_blend_function;
        }
    }

    for (i = 0; (int32_t)i < (int32_t)overlay->effectors.count; i++) {
        const HUDInterfaceMultitextureOverlayEffector *effector =
            (const HUDInterfaceMultitextureOverlayEffector *)overlay->effectors.pointer + i;

        hud_multitexture_effector_counter += 0.05f;
        switch (effector->source) {
        case 0: { // player pitch
            datum_index player_index = hud_local_player_index_to_player(local_player_index);
            datum_index unit_index = (datum_index)-1;
            const float *aim;

            if (player_index != (datum_index)-1) {
                unit_index = ((player *)((uint8_t *)player_data->data + (player_index & 0xffff) * 0x200))->unit;
            }
            aim = (const float *)((uint8_t *)((object_header *)object_data->data)[unit_index & 0xffff].data + 0x23c); // unit aiming_vector
            value = atan2f(aim[2], sqrtf(aim[0] * aim[0] + aim[1] * aim[1]));
            break;
        }
        case 1: // player pitch tangent
        case 2: // player yaw
            value = 0.0f;
            break;
        case 3: // weapon ammo total
            value = (float)ammo.magazines[0].rounds_loaded;
            break;
        case 4: // weapon ammo loaded
            value = (float)ammo.magazines[0].rounds_unloaded;
            break;
        case 5: // weapon heat
            value = ammo.heat;
            break;
        case 6: // explicit, uses the low bound
            value = effector->in_bounds[0];
            break;
        case 7: { // weapon zoom level
            int16_t zoom = -1;
            if (local_player_index != -1) {
                zoom = player_control_globals_ptr->local_players[local_player_index].desired_zoom_level;
            }
            value = (float)zoom;
            break;
        }
        }

        if (effector->in_bounds[0] != effector->in_bounds[1] && effector->out_bounds[0] != effector->out_bounds[1]) {
            float t = (value - effector->in_bounds[0]) / (effector->in_bounds[1] - effector->in_bounds[0]);
            if (t < 0.0f) {
                t = 0.0f;
            } else if (t > 1.0f) {
                t = 1.0f;
            }
            output = (1.0f - t) * effector->out_bounds[0] + t * effector->out_bounds[1];
            // 0x4ad638: EAX = upper bound (+0xa4) = color1, ECX = lower bound (+0x98) = color0
            color_interpolate((ColorRGB *)&effector->tint_color_upper_bound, (ColorRGB *)&effector->tint_color_lower_bound, &tint, 0, t);
        } else {
            output = effector->out_bounds[0];
            tint = effector->tint_color_lower_bound;
        }

        switch (effector->destination) {
        case 0: // geometry offset
            geometry_offset[0] = effector->destination_type == 1 ? output : 0.0f;
            geometry_offset[1] = effector->destination_type == 2 ? output : 0.0f;
            state.geometry_offset = geometry_offset;
            break;
        case 1: // primary map
        case 2: // secondary map
        case 3: { // tertiary map
            int32_t map = effector->destination - 1;
            switch (effector->destination_type) {
            case 0: // tint 0-1
                tints[map] = tint;
                state.map_tints[map] = &tints[map];
                break;
            case 1: // horizontal offset
                state.map_offsets[map]->x += output;
                break;
            case 2: // vertical offset
                state.map_offsets[map]->y += output;
                break;
            case 3: // fade 0-1
                fades[map] = output;
                state.map_fades[map] = &fades[map];
                break;
            }
            break;
        }
        }
    }

    rasterizer_ui_quad_draw(&state, vertices);
}

#if 0
Original Ghidra decompilation (0x4acfe0):

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void FUN_004acfe0(int param_1,short param_2,short *param_3,float *param_4,float *param_5,
                 float param_6,float param_7)

{
  float fVar1;
  undefined1 uVar2;
  int iVar3;
  short sVar4;
  float *in_EAX;
  float *pfVar5;
  undefined2 *puVar6;
  short sVar7;
  int iVar8;
  uint uVar9;
  int iVar10;
  float fVar11;
  float fVar12;
  float fVar13;
  float **ppfVar14;
  int iVar15;
  float10 fVar16;
  float10 fVar17;
  float local_194;
  float local_190;
  undefined1 *local_18c;
  undefined1 *local_174 [2];
  float local_16c;
  float local_168;
  float local_164;
  float local_160;
  float local_15c;
  float local_158;
  undefined4 local_154;
  undefined4 local_150;
  undefined4 local_14c;
  undefined4 local_148;
  undefined4 local_144;
  undefined4 local_140;
  undefined4 local_13c;
  undefined4 local_138;
  undefined4 local_134;
  undefined4 local_130;
  undefined4 local_12c;
  undefined4 local_128;
  undefined4 local_124;
  undefined4 local_120;
  undefined4 local_11c;
  undefined4 local_118;
  undefined4 local_114;
  undefined4 local_110;
  float *local_10c [2];
  float local_104 [4];
  undefined1 local_f4 [4];
  float *local_f0 [3];
  float local_e4 [6];
  undefined4 local_cc;
  undefined4 local_c8;
  undefined4 *local_b4;
  undefined4 *local_b0;
  undefined4 *local_ac;
  float *local_94;
  float *local_90;
  float *local_8c;
  undefined1 local_88 [2];
  undefined1 local_86 [2];
  undefined2 local_84;
  undefined1 local_82;
  float local_80 [3];
  short local_72;
  short local_6e;
  undefined1 local_60 [4];
  float local_5c [23];

  fVar16 = (float10)fsin((float10)param_6);
  local_124 = *(undefined4 *)(param_1 + 0x4c);
  local_120 = *(undefined4 *)(param_1 + 0x50);
  local_11c = *(undefined4 *)(param_1 + 0x54);
  local_118 = *(undefined4 *)(param_1 + 0x58);
  local_114 = *(undefined4 *)(param_1 + 0x5c);
  local_110 = *(undefined4 *)(param_1 + 0x60);
  local_154 = 0;
  local_150 = 0;
  local_14c = 0;
  local_148 = 0;
  local_144 = 0;
  local_140 = 0;
  local_13c = 0;
  local_138 = 0;
  local_134 = 0;
  local_160 = 1.0;
  local_15c = 1.0;
  local_158 = 1.0;
  local_168 = 0.0;
  local_164 = 0.0;
  fVar17 = (float10)fcos((float10)param_6);
  FUN_004acef0(local_80);
  sVar7 = 0;
  uVar9 = 1;
  pfVar5 = local_5c;
  do {
    if ((uVar9 & 2) == 0) {
      fVar11 = *param_4;
    }
    else {
      fVar11 = param_4[1];
    }
    if (sVar7 < 2) {
      fVar12 = param_4[2];
    }
    else {
      fVar12 = param_4[3];
    }
    if ((uVar9 & 2) == 0) {
      local_190 = *param_5;
    }
    else {
      local_190 = param_5[1];
    }
    if (sVar7 < 2) {
      fVar13 = param_5[2];
    }
    else {
      fVar13 = param_5[3];
    }
    local_174[0] = (undefined1 *)
                   (int)ROUND((local_190 * (float)fVar17 - fVar13 * (float)fVar16) * *in_EAX);
    pfVar5[-1] = (float)(int)(local_174[0] + *param_3);
    fVar1 = in_EAX[1];
    sVar4 = param_3[1];
    pfVar5[3] = fVar11;
    pfVar5[4] = fVar12;
    *pfVar5 = (float)((int)sVar4 +
                     (int)ROUND((local_190 * (float)fVar16 + fVar13 * (float)fVar17) * fVar1));
    sVar7 = sVar7 + 1;
    pfVar5[1] = 0.0;
    pfVar5[2] = param_7;
    uVar9 = uVar9 + 1;
    pfVar5 = pfVar5 + 6;
  } while (sVar7 < 4);
  ppfVar14 = local_10c;
  for (iVar8 = 0x23; iVar8 != 0; iVar8 = iVar8 + -1) {
    *ppfVar14 = (float *)0x0;
    ppfVar14 = ppfVar14 + 1;
  }
  sVar7 = 0;
  local_c8 = 0x3f800000;
  local_cc = 0x3f800000;
  local_e4[1] = 1.0;
  local_e4[0] = 1.0;
  local_10c[0] = (float *)0x0;
  local_82 = *(short *)(DAT_0087a478 + 0xc) == 1;
  iVar15 = 0;
  local_104[1] = (float)bitmap_group_sequence_get_bitmap_data(0);
  local_104[2] = (float)bitmap_group_sequence_get_bitmap_data(0);
  local_104[3] = (float)bitmap_group_sequence_get_bitmap_data(0);
  local_18c = local_f4;
  iVar8 = (int)local_104 + (4 - param_1);
  puVar6 = (undefined2 *)(param_1 + 0x2e);
  pfVar5 = (float *)(param_1 + 0x38);
  iVar10 = 0;
  do {
    iVar3 = *(int *)((int)local_104 + iVar10 + 4);
    if (iVar3 != 0) {
      fVar11 = 1.0;
      if (pfVar5[-1] != 0.0) {
        fVar11 = 1.0 / pfVar5[-1];
      }
      fVar12 = 1.0;
      if (*pfVar5 != 0.0) {
        fVar12 = 1.0 / *pfVar5;
      }
      if ((((int)*(short *)(iVar3 + 4) & (int)*(short *)(iVar3 + 4) - 1U) == 0) &&
         (((int)*(short *)(iVar3 + 6) & (int)*(short *)(iVar3 + 6) - 1U) == 0)) {
        *(undefined4 *)(((int)local_104 - param_1) + (int)pfVar5) = 0x3f800000;
        *(undefined4 *)(iVar8 + (int)pfVar5) = 0x3f800000;
      }
      else {
        *(float *)(((int)local_104 - param_1) + (int)pfVar5) =
             1.0 / (float)(int)*(short *)(iVar3 + 4);
        *(float *)(iVar8 + (int)pfVar5) =
             1.0 / (float)(int)*(short *)(*(int *)((int)local_104 + iVar10 + 4) + 6);
      }
      *(int *)((int)local_f0 + iVar10) = (int)&local_124 + iVar15;
      *(float *)((int)local_e4 + iVar15) = fVar11;
      uVar2 = *(undefined1 *)(puVar6 + 0x33);
      *(float *)((int)local_e4 + iVar15 + 4) = fVar12;
      *local_18c = uVar2;
    }
    if (sVar7 < 2) {
      local_174[0] = local_88;
      local_174[1] = local_86;
      switch(*puVar6) {
      case 0:
        **(undefined2 **)((int)local_174 + iVar10) = 0;
        break;
      case 1:
        **(undefined2 **)((int)local_174 + iVar10) = 2;
        break;
      case 2:
        **(undefined2 **)((int)local_174 + iVar10) = 1;
        break;
      case 3:
        **(undefined2 **)((int)local_174 + iVar10) = 3;
        break;
      case 4:
        **(undefined2 **)((int)local_174 + iVar10) = 4;
      }
    }
    else {
      local_84 = *(undefined2 *)(param_1 + 4);
    }
    puVar6 = puVar6 + 1;
    sVar7 = sVar7 + 1;
    pfVar5 = pfVar5 + 2;
    iVar15 = iVar15 + 8;
    local_18c = local_18c + 1;
    iVar10 = iVar10 + 4;
  } while (sVar7 < 3);
  sVar7 = 0;
  if (0 < *(int *)(param_1 + 0x154)) {
    iVar8 = 0;
    do {
      _DAT_00719428 = _DAT_00719428 + 0.05;
      iVar10 = iVar8 * 0xdc + *(int *)(param_1 + 0x158);
      switch(*(undefined2 *)(iVar8 * 0xdc + 0x44 + *(int *)(param_1 + 0x158))) {
      case 0:
        if (((param_2 == -1) || (0 < param_2)) ||
           (uVar9 = *(uint *)(DAT_0087a478 + 4 + param_2 * 4), uVar9 == 0xffffffff)) {
          uVar9 = 0xffffffff;
        }
        else {
          uVar9 = *(uint *)((uVar9 & 0xffff) * 0x200 + 0x34 + *(int *)(DAT_0087a480 + 0x34));
        }
        iVar8 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (uVar9 & 0xffff) * 0xc);
        local_174[0] = *(undefined1 **)(iVar8 + 0x23c);
        local_174[1] = *(undefined1 **)(iVar8 + 0x240);
        local_16c = *(float *)(iVar8 + 0x244);
        fVar16 = (float10)fpatan((float10)local_16c,
                                 SQRT((float10)(float)local_174[1] * (float10)(float)local_174[1] +
                                      (float10)(float)local_174[0] * (float10)(float)local_174[0]));
        goto LAB_004ad593;
      case 1:
      case 2:
        local_190 = 0.0;
        break;
      case 3:
        fVar16 = (float10)(int)local_72;
        goto LAB_004ad593;
      case 4:
        fVar16 = (float10)(int)local_6e;
        goto LAB_004ad593;
      case 5:
        local_190 = local_80[0];
        break;
      case 6:
        local_190 = *(float *)(iVar10 + 0x48);
        break;
      case 7:
        sVar4 = -1;
        if (param_2 != -1) {
          sVar4 = *(short *)(param_2 * 0x40 + 0x34 + DAT_006b145c);
        }
        fVar16 = (float10)(int)sVar4;
LAB_004ad593:
        local_190 = (float)fVar16;
      }
      if ((*(float *)(iVar10 + 0x4c) == *(float *)(iVar10 + 0x48)) ||
         (*(float *)(iVar10 + 0x54) == *(float *)(iVar10 + 0x50))) {
        local_194 = *(float *)(iVar10 + 0x50);
        local_130 = *(undefined4 *)(iVar10 + 0x98);
        local_12c = *(undefined4 *)(iVar10 + 0x9c);
        local_128 = *(undefined4 *)(iVar10 + 0xa0);
      }
      else {
        if (0.0 <= (local_190 - *(float *)(iVar10 + 0x48)) /
                   (*(float *)(iVar10 + 0x4c) - *(float *)(iVar10 + 0x48))) {
          if ((local_190 - *(float *)(iVar10 + 0x48)) /
              (*(float *)(iVar10 + 0x4c) - *(float *)(iVar10 + 0x48)) <= 1.0) {
            local_18c = (undefined1 *)
                        ((local_190 - *(float *)(iVar10 + 0x48)) /
                        (*(float *)(iVar10 + 0x4c) - *(float *)(iVar10 + 0x48)));
          }
          else {
            local_18c = (undefined1 *)0x3f800000;
          }
        }
        else {
          local_18c = (undefined1 *)0x0;
        }
        local_194 = (float)local_18c * *(float *)(iVar10 + 0x54) +
                    (1.0 - (float)local_18c) * *(float *)(iVar10 + 0x50);
        color_interpolate(&local_130,0,local_18c);
      }
      switch(*(undefined2 *)(iVar10 + 0x42)) {
      case 0:
        if (*(short *)(iVar10 + 0x40) == 1) {
          local_168 = local_194;
        }
        else {
          local_168 = 0.0;
        }
        if (*(short *)(iVar10 + 0x40) == 2) {
          local_10c[1] = &local_168;
          local_164 = local_194;
        }
        else {
          local_10c[1] = &local_168;
          local_164 = 0.0;
        }
        break;
      case 1:
        sVar4 = *(short *)(iVar10 + 0x40);
        if (sVar4 == 0) {
          local_b4 = &local_154;
          local_154 = local_130;
          local_150 = local_12c;
          local_14c = local_128;
        }
        else if (sVar4 == 1) {
          *local_f0[0] = local_194 + *local_f0[0];
        }
        else if (sVar4 == 2) {
          local_f0[0][1] = local_194 + local_f0[0][1];
        }
        else if (sVar4 == 3) {
          local_94 = &local_160;
          local_160 = local_194;
        }
        break;
      case 2:
        sVar4 = *(short *)(iVar10 + 0x40);
        if (sVar4 == 0) {
          local_b0 = &local_148;
          local_148 = local_130;
          local_144 = local_12c;
          local_140 = local_128;
        }
        else if (sVar4 == 1) {
          *local_f0[1] = local_194 + *local_f0[1];
        }
        else if (sVar4 == 2) {
          local_f0[1][1] = local_194 + local_f0[1][1];
        }
        else if (sVar4 == 3) {
          local_90 = &local_15c;
          local_15c = local_194;
        }
        break;
      case 3:
        sVar4 = *(short *)(iVar10 + 0x40);
        if (sVar4 == 0) {
          local_ac = &local_13c;
          local_13c = local_130;
          local_138 = local_12c;
          local_134 = local_128;
        }
        else if (sVar4 == 1) {
          *local_f0[2] = local_194 + *local_f0[2];
        }
        else if (sVar4 == 2) {
          local_f0[2][1] = local_194 + local_f0[2][1];
        }
        else if (sVar4 == 3) {
          local_8c = &local_158;
          local_158 = local_194;
        }
      }
      sVar7 = sVar7 + 1;
      iVar8 = (int)sVar7;
    } while (iVar8 < *(int *)(param_1 + 0x154));
  }
  FUN_0051c9a0(local_60);
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
