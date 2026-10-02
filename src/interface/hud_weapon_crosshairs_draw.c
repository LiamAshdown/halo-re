// hud_weapon_crosshairs_draw  (Ghidra: FUN_004b2cf0; the first rewrite called it
// hud_weapon_crosshair_elements_build; renamed in the phase-4 review)
// address 0x4b2cf0, size 1841 bytes; real extent 0x4b2cf0..0x4b3420 (1841 bytes). Ghidra stopped at the
// crosshair type switch; the listed function 0x4b2f8a ("FUN_004b2f8a", decompile failure) is
// the switch head inside this function (the target of the jmp at 0x4b2f80), not a function.
// Jump tables: byte map 0x4b3434 (19 types) -> 4 cases at 0x4b3424.
// name confidence: 0.6 (chosen)   rewrite confidence: 0.75
// evidence: rewritten from objdump 0x4b2cf0..0x4b3420 in the phase-4 review; the first rewrite
// covered only the part before the switch and guessed the rest. Draws the crosshairs of a
// weapon HUD interface and of its child_hud chain (up to 16 tags): every crosshair whose type
// bit is set in the crosshair state mask (hud_weapon_state + index * 0x50 + 0x28, entry 0x13,
// written by hud_weapon_interface_meters_evaluate) and whose view type is allowed, and every
// crosshair overlay (WeaponHUDInterfaceCrosshairOverlay, stride 0x6c) of it:
//   aim (0): frame = the aim state (0/1), or flashing from time 0 while it is on target;
//   zoom overlay (1): frame = zoom level (minus 1 with "show only when zoomed"), one zoom
//     level overlays only while zoomed; flashing while the aim state is set;
//   charge .. secondary ready (2..7, 10..13, 15..17): frame from (game time - state) /
//     frame_rate / 30 modulo the sprite count, flashing from the state time;
//   firing with no ammo (8), throwing with no grenade (9), secondary no ammo (14), depleted
//     battery (18): drawn while the trigger condition holds or until flash_period seconds
//     after the state time, then the state is reset to -1.
// Overlays with flags bit 7 are skipped, "don't show when zoomed" ones while zoomed. A "hide
// area outside reticle" overlay stretches its uv rectangle to cover the whole viewport
// (0x007c3140..0x007c3146). Each is drawn with hud_draw_bitmap_element at anchor 4 (center),
// scale 0.5 in split screen unless scaling flags bit 1 (don't scale size) is set.
// Behaviour kept from the binary: type 14 reuses the trigger byte left by the previous
// overlay (the binary never computes it for 14); a type above 18 draws with the previous
// color; without a sprite sequence the overlay sequence index is used as the bitmap index.
// register convention: EAX hud interface tag id, ECX player record; one stack argument.
//   // blam-cc: EAX -> hud_tag, ECX -> p, stack -> ammo
// reconciled: R34 player_globals.unknown_0c -> local_player_count (int16 at +0x0c, same width)

#include <string.h>
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "game.h"
#include "networking.h"
#include "objects.h"
#include "items.h"
#include "interface.h"
#include "units.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern tag_instance *tag_instances;                  // 0x0087bc14
extern data_array *object_data; // 0x008603b0
extern player_globals *local_player_globals;         // 0x0087a478
extern game_time_globals *game_time;                 // 0x006f1d6c
extern Scenario *global_scenario; // 0x00746f8c
extern hud_weapon_interface_state *hud_weapon_state; // 0x00719430
extern int16_t render_viewport_top;                  // 0x007c3140
extern int16_t render_viewport_left;                 // 0x007c3142
extern int16_t render_viewport_bottom;               // 0x007c3144
extern int16_t render_viewport_right;                // 0x007c3146

extern long lrint(double x); // x87 fistp under the default control word (round-half-to-even)
extern void *texture_cache_get(BitmapData *bitmap, uint8_t wait, uint8_t allocate_if_missing); // 0x444550, blam-cc: EAX bitmap
extern uint32_t hud_meter_flash_color_blend(const hud_flash_parameters *flash, int32_t start_time); // 0x4ab980, blam-cc: ESI flash, EDI start_time
extern void hud_draw_bitmap_element(const float *uv, const hud_element_placement *placement, uint8_t pixel_uvs,
                                    void *meter_parameters, BitmapData *bitmap, uint16_t *anchor,
                                    float scale, float rotation, uint32_t color, uint8_t split_screen); // 0x4acad0, blam-cc: EAX uv, EDX placement, BL pixel_uvs

// blam-cc: EAX -> hud_tag, ECX -> p, stack -> ammo
void hud_weapon_crosshairs_draw(datum_index hud_tag, const player *p, const weapon_hud_ammo_state *ammo)
{
    int32_t *crosshair_state;
    uint32_t view_mask;
    uint32_t active_mask;
    uint8_t *unit;
    WeaponHUDInterface *chain[16];
    int16_t chain_count;
    int16_t chain_index;
    uint8_t triggered = 0; // byte at esp+0x13, carried across overlays
    uint32_t color = 0;    // esp+0x1c, carried across overlays

    if ((hud_weapon_state->flags & 1) == 0 || hud_tag == (datum_index)-1) {
        return;
    }
    crosshair_state = (int32_t *)((uint8_t *)hud_weapon_state + p->local_player_index * 0x50 + 0x28);
    view_mask = (*(int16_t *)((uint8_t *)global_scenario + 0x3c) != 2 ? 1 : 0) |
                (local_player_globals->local_player_count == 1 ? 2 : 0) | (local_player_globals->local_player_count > 1 ? 4 : 0);
    if (p->unit == (datum_index)-1) {
        return;
    }
    unit = (uint8_t *)((object_header *)object_data->data)[p->unit & 0xffff].data;

    memset(chain, 0, sizeof(chain));
    chain[0] = (WeaponHUDInterface *)tag_instances[hud_tag & 0xffff].data;
    active_mask = (uint32_t)crosshair_state[0x13];
    chain_count = 1;
    do {
        datum_index child = *(datum_index *)&chain[chain_count - 1]->child_hud.tag_id;
        if (child == (datum_index)-1) {
            break;
        }
        chain[chain_count] = (WeaponHUDInterface *)tag_instances[child & 0xffff].data;
        chain_count++;
    } while (chain_count < 0x10);

    for (chain_index = 0; chain_index < chain_count; chain_index++) {
        WeaponHUDInterface *hud = chain[chain_index];
        uint16_t anchor[0x12];
        uint8_t split_screen = local_player_globals->local_player_count > 1;
        int16_t crosshair_index;

        memset(anchor, 0, sizeof(anchor));
        anchor[0] = 4; // center
        for (crosshair_index = 0; (int32_t)crosshair_index < (int32_t)hud->crosshairs.count; crosshair_index++) {
            WeaponHUDInterfaceCrosshair *crosshair = (WeaponHUDInterfaceCrosshair *)hud->crosshairs.pointer + crosshair_index;
            int16_t type = crosshair->crosshair_type;
            int32_t *state;
            int16_t overlay_index;

            if ((active_mask & (1u << type)) == 0 ||
                ((int32_t)(int16_t)view_mask & (1 << *(uint8_t *)&crosshair->allowed_view_type)) == 0) {
                continue;
            }
            state = &crosshair_state[type];
            for (overlay_index = 0; (int32_t)overlay_index < (int32_t)crosshair->crosshair_overlays.count; overlay_index++) {
                WeaponHUDInterfaceCrosshairOverlay *overlay =
                    (WeaponHUDInterfaceCrosshairOverlay *)crosshair->crosshair_overlays.pointer + overlay_index;
                uint32_t flags = *(uint32_t *)&overlay->flags;
                Bitmap *bitmap_tag;
                BitmapGroupSequence *sequence;
                BitmapData *bitmap;
                float scale;
                int16_t frame;
                const float *uv;
                float stretched_uv[4];
                uint8_t pixel_uvs;

                if ((int8_t)flags < 0) {
                    continue;
                }
                if ((flags & 4) != 0 && crosshair_state[1] <= 0) { // show only when zoomed
                    continue;
                }
                if ((flags & 0x40) != 0 && crosshair_state[1] != 0) { // don't show when zoomed
                    continue;
                }
                scale = 1.0f;
                if (local_player_globals->local_player_count > 1 && (*(uint8_t *)&overlay->scaling_flags & 2) == 0) {
                    scale = 0.5f;
                }
                sequence = 0;
                if ((flags & 2) == 0) { // a sprite
                    Bitmap *tag = (Bitmap *)tag_instances[*(datum_index *)&crosshair->crosshair_bitmap.tag_id & 0xffff].data;
                    sequence = (BitmapGroupSequence *)tag->bitmap_group_sequence.pointer + (int16_t)overlay->sequence_index;
                }

                frame = 0;
                switch ((uint32_t)(int32_t)type > 0x12 ? -1 : type) { // ja 0x4b3198 on an unsigned compare
                case 0: // aim
                    if ((flags & 1) != 0) {
                        frame = 0;
                        color = *state > 0 ? hud_meter_flash_color_blend((const hud_flash_parameters *)&overlay->default_color, 0)
                                           : *(uint32_t *)&overlay->default_color;
                    } else {
                        frame = (int16_t)*state;
                        color = *(uint32_t *)&overlay->default_color;
                    }
                    break;
                case 1: // zoom overlay
                    if ((flags & 0x20) != 0) { // one zoom level
                        if (*state == 0) {
                            continue;
                        }
                        frame = 0;
                    } else {
                        frame = (int16_t)((int16_t)*state - ((flags >> 2) & 1));
                    }
                    if ((flags & 1) != 0 && crosshair_state[0] > 0) {
                        color = hud_meter_flash_color_blend((const hud_flash_parameters *)&overlay->default_color, 0);
                    } else {
                        color = *(uint32_t *)&overlay->default_color;
                    }
                    break;
                case 8: case 9: case 14: case 18: // triggered
                    if (type == 18) {
                        triggered = ammo->age == 0.0f && (((unit_object *)unit)->unit.control_flags & 0x800) != 0;
                    } else if (type == 8) {
                        triggered = ammo->magazines[0].rounds_loaded == 0 && ammo->magazines[0].rounds_unloaded == 0 &&
                                    (((unit_object *)unit)->unit.control_flags & 0x800) != 0; // test dl,ch with dl = 8
                    } else if (type == 9) {
                        triggered = unit[0x31e] == 0 && unit[0x31f] == 0 && unit[0x28d] == 0 &&
                                    (((unit_object *)unit)->unit.control_flags & 0x2000) != 0;
                    }
                    if (!triggered) {
                        int32_t duration = (int32_t)lrint((double)(overlay->flash_period * 30.0f));
                        if (game_time->game_time - *state >= duration) {
                            *state = -1;
                            continue;
                        }
                    }
                    if (*state == -1) {
                        *state = -1;
                        continue;
                    }
                    /* fall through */
                default:
                    if ((uint32_t)(int32_t)type > 0x12) {
                        break; // frame 0, the color of the previous overlay
                    }
                    if (overlay->frame_rate > 0) {
                        frame = (int16_t)((((game_time->game_time - *state) / overlay->frame_rate) / 30) %
                                          (int32_t)sequence->sprites.count);
                    } else {
                        frame = 0;
                    }
                    if ((*(uint8_t *)&overlay->flags & 1) != 0 && *state != -1) {
                        color = hud_meter_flash_color_blend((const hud_flash_parameters *)&overlay->default_color, *state);
                    } else {
                        color = *(uint32_t *)&overlay->default_color;
                    }
                    break;
                }

                bitmap_tag = (Bitmap *)tag_instances[*(datum_index *)&crosshair->crosshair_bitmap.tag_id & 0xffff].data;
                {
                    int32_t bitmap_index = sequence != 0
                        ? (int16_t)((BitmapGroupSprite *)sequence->sprites.pointer)[frame].bitmap_index
                        : (int16_t)overlay->sequence_index;
                    bitmap = (BitmapData *)bitmap_tag->bitmap_data.pointer + bitmap_index;
                }
                if (texture_cache_get(bitmap, 0, 1) == 0) {
                    continue;
                }
                pixel_uvs = bitmap_tag->type == 4;
                if ((flags & 0x10) != 0) { // hide area outside reticle
                    float u_factor = 1.0f;
                    float v_factor = 1.0f;
                    float dx;
                    float dy;

                    if (sequence != 0) {
                        BitmapGroupSprite *sprite = (BitmapGroupSprite *)sequence->sprites.pointer + frame;
                        stretched_uv[0] = sprite->left;
                        stretched_uv[1] = sprite->right;
                        stretched_uv[2] = sprite->top;
                        stretched_uv[3] = sprite->bottom;
                    } else {
                        stretched_uv[0] = 0.0f;
                        stretched_uv[1] = (float)(pixel_uvs ? (int32_t)(int16_t)bitmap->width : 1);
                        stretched_uv[2] = 0.0f;
                        stretched_uv[3] = (float)(pixel_uvs ? (int32_t)(int16_t)bitmap->height : 1);
                        u_factor = pixel_uvs ? 1.0f : (float)((double)(1.0f / (float)(int16_t)bitmap->width) * 1.25);
                        v_factor = pixel_uvs ? 1.0f : (float)((double)(1.0f / (float)(int16_t)bitmap->height) * 1.25);
                    }
                    dx = ((float)(int16_t)bitmap->width - (float)(render_viewport_right - render_viewport_left) * (1.0f / scale)) *
                         u_factor * -0.5f;
                    dy = ((float)(int16_t)bitmap->height - (float)(render_viewport_bottom - render_viewport_top) * (1.0f / scale)) *
                         v_factor * -0.5f;
                    stretched_uv[0] = stretched_uv[0] - dx;
                    stretched_uv[1] = dx + stretched_uv[1];
                    stretched_uv[2] = stretched_uv[2] - dy;
                    stretched_uv[3] = dy + stretched_uv[3];
                    uv = stretched_uv;
                } else {
                    uv = sequence != 0 ? &((BitmapGroupSprite *)sequence->sprites.pointer)[frame].left : (const float *)0;
                }
                hud_draw_bitmap_element(uv, (const hud_element_placement *)overlay, pixel_uvs, 0, bitmap, anchor, scale, 0.0f,
                                        color, split_screen);
            }
        }
    }
}

#if 0
Original Ghidra decompilation (0x4b2cf0) -- the rewrite above is a close, mechanical
translation of this (folding the `local_46`/`aiStackY_20044` stack-aliasing artifact back
into ordinary `chain[]` indexing, exactly as in hud_weapon_interface_meters_evaluate.c); see
the header comment for exactly which parts are and are not independently verified:

void FUN_004b2cf0(int param_1)

{
  int *piVar1;
  short sVar2;
  short sVar3;
  int iVar4;
  uint uVar5;
  uint uVar6;
  bool bVar7;
  short sVar8;
  short sVar9;
  short sVar10;
  short sVar11;
  uint in_EAX;
  int iVar12;
  short *psVar13;
  int iVar14;
  ushort uVar15;
  int in_ECX;
  int iVar16;
  char *pcVar17;
  short sVar18;
  short sVar19;
  int *piVar20;
  int aiStackY_20044 [32725];
  char cStack_c1;
  int local_c0;
  undefined4 uStack_b8;
  undefined4 local_a8;
  undefined2 local_64;
  undefined4 local_62;
  undefined4 local_5e;
  undefined4 local_5a;
  undefined4 local_56;
  undefined4 local_52;
  undefined4 local_4e;
  undefined4 local_4a;
  undefined1 local_46 [4];
  undefined2 local_42;
  int local_40 [16];

  if (((*(byte *)(DAT_00719430 + 0x78) & 1) != 0) && (in_EAX != 0xffffffff)) {
    piVar1 = (int *)(*(short *)(in_ECX + 2) * 0x50 + 0x28 + DAT_00719430);
    uVar15 = (ushort)(*(short *)(global_scenario + 0x3c) != 2);
    if (*(short *)(DAT_0087a478 + 0xc) == 1) {
      uVar15 = uVar15 | 2;
    }
    if (1 < *(short *)(DAT_0087a478 + 0xc)) {
      uVar15 = uVar15 | 4;
    }
    if (*(uint *)(in_ECX + 0x34) != 0xffffffff) {
      iVar4 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (*(uint *)(in_ECX + 0x34) & 0xffff) * 0xc
                      );
      local_40[0] = *(int *)((in_EAX & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
      piVar20 = local_40;
      for (iVar16 = 0xf; piVar20 = piVar20 + 1, iVar16 != 0; iVar16 = iVar16 + -1) {
        *piVar20 = 0;
      }
      uVar5 = piVar1[0x13];
      sVar18 = 1;
      do {
        iVar16 = (int)sVar18;
        if (*(uint *)(*(int *)(local_46 + iVar16 * 4 + 2) + 0xc) == 0xffffffff) break;
        sVar18 = sVar18 + 1;
        local_40[iVar16] =
             *(int *)((*(uint *)(*(int *)(local_46 + iVar16 * 4 + 2) + 0xc) & 0xffff) * 0x20 + 0x14
                     + DAT_0087bc14);
      } while (sVar18 < 0x10);
      sVar11 = 0;
      if (0 < sVar18) {
        do {
          iVar16 = local_40[sVar11];
          sVar2 = *(short *)(DAT_0087a478 + 0xc);
          local_62 = 0;
          local_5e = 0;
          local_5a = 0;
          local_56 = 0;
          local_52 = 0;
          local_4e = 0;
          local_4a = 0;
          local_46 = (undefined1  [4])0x0;
          local_42 = 0;
          local_64 = 4;
          sVar10 = 0;
          if (0 < *(int *)(iVar16 + 0x84)) {
            iVar12 = 0;
            do {
              psVar13 = (short *)(iVar12 * 0x68 + *(int *)(iVar16 + 0x88));
              sVar3 = *psVar13;
              if (((uVar5 & 1 << ((byte)sVar3 & 0x1f)) != 0) &&
                 (((int)(short)uVar15 & 1 << (*(byte *)(psVar13 + 2) & 0x1f)) != 0)) {
                piVar20 = piVar1 + sVar3;
                sVar9 = 0;
                if (0 < *(int *)(psVar13 + 0x1a)) {
                  iVar12 = 0;
                  do {
                    iVar12 = iVar12 * 0x6c + *(int *)(psVar13 + 0x1c);
                    uVar6 = *(uint *)(iVar12 + 0x48);
                    if (((-1 < (char)uVar6) && (((uVar6 & 4) == 0 || (0 < piVar1[1])))) &&
                       (((uVar6 & 0x40) == 0 || (piVar1[1] == 0)))) {
                      if ((*(short *)(DAT_0087a478 + 0xc) < 2) ||
                         (local_a8 = 0x3f000000, (*(byte *)(iVar12 + 0xc) & 2) != 0)) {
                        local_a8 = 0x3f800000;
                      }
                      if ((uVar6 & 2) == 0) {
                        local_c0 = *(short *)(iVar12 + 0x46) * 0x40 +
                                   *(int *)(*(int *)((*(uint *)(psVar13 + 0x18) & 0xffff) * 0x20 +
                                                     0x14 + DAT_0087bc14) + 0x58);
                      }
                      else {
                        local_c0 = 0;
                      }
                      sVar19 = 0;
                      switch(sVar3) {
                      case 0:
                        if ((uVar6 & 1) == 0) {
                          uStack_b8 = *(undefined4 *)(iVar12 + 0x24);
                          sVar19 = (short)*piVar20;
                        }
                        else {
                          sVar19 = 0;
                          sVar8 = 0;
                          if (*piVar20 < 1) goto LAB_004b3191;
                          uStack_b8 = FUN_004ab980();
                        }
                        break;
                      case 1:
                        if ((uVar6 & 0x20) == 0) {
                          sVar19 = (short)*piVar20 - ((ushort)(uVar6 >> 2) & 1);
                        }
                        else {
                          if (*piVar20 == 0) goto LAB_004b33c9;
                          sVar19 = 0;
                        }
                        sVar8 = sVar19;
                        if (((uVar6 & 1) == 0) || (*piVar1 < 1)) {
LAB_004b3191:
                          sVar19 = sVar8;
                          uStack_b8 = *(undefined4 *)(iVar12 + 0x24);
                        }
                        else {
                          uStack_b8 = FUN_004ab980();
                        }
                        break;
                      case 2:
                      case 3:
                      case 4:
                      case 5:
                      case 6:
                      case 7:
                      case 10:
                      case 0xb:
                      case 0xc:
                      case 0xd:
                      case 0xf:
                      case 0x10:
                      case 0x11:
LAB_004b3135:
                        if (*(short *)(iVar12 + 0x44) < 1) {
                          sVar19 = 0;
                        }
                        else {
                          sVar19 = (short)((((*(int *)(DAT_006f1d6c + 0xc) - *piVar20) /
                                            (int)*(short *)(iVar12 + 0x44)) / 0x1e) %
                                          *(int *)(local_c0 + 0x34));
                        }
                        sVar8 = sVar19;
                        if (((*(byte *)(iVar12 + 0x48) & 1) == 0) || (*piVar20 == -1))
                        goto LAB_004b3191;
                        uStack_b8 = FUN_004ab980();
                        break;
                      case 8:
                      case 9:
                      case 0xe:
                      case 0x12:
                        if (sVar3 == 0x12) {
                          if ((*(float *)(param_1 + 4) == 0.0) &&
                             ((*(uint *)(iVar4 + 0x208) & 0x800) != 0)) {
                            cStack_c1 = '\x01';
                            goto LAB_004b3121;
                          }
LAB_004b30e4:
                          cStack_c1 = '\0';
LAB_004b30f3:
                          if (*(int *)(DAT_006f1d6c + 0xc) - *piVar20 <
                              (int)ROUND(*(float *)(iVar12 + 0x2c) * 30.0)) goto LAB_004b3121;
                        }
                        else {
                          if (sVar3 == 8) {
                            if (((*(short *)(param_1 + 0xe) != 0) ||
                                (*(short *)(param_1 + 0x12) != 0)) ||
                               ((*(uint *)(iVar4 + 0x208) & 0x800) == 0)) goto LAB_004b30e4;
                            cStack_c1 = '\x01';
                          }
                          else if (sVar3 == 9) {
                            bVar7 = true;
                            pcVar17 = (char *)(iVar4 + 0x31e);
                            iVar14 = 2;
                            do {
                              if ((bVar7) && (*pcVar17 == '\0')) {
                                bVar7 = true;
                              }
                              else {
                                bVar7 = false;
                              }
                              pcVar17 = pcVar17 + 1;
                              iVar14 = iVar14 + -1;
                            } while (iVar14 != 0);
                            if (((!bVar7) || (*(char *)(iVar4 + 0x28d) != '\0')) ||
                               ((*(uint *)(iVar4 + 0x208) & 0x2000) == 0)) goto LAB_004b30e4;
                            cStack_c1 = '\x01';
                          }
                          else if (cStack_c1 == '\0') goto LAB_004b30f3;
LAB_004b3121:
                          if (*piVar20 != -1) goto LAB_004b3135;
                        }
                        *piVar20 = -1;
                        goto LAB_004b33c9;
                      }
                      if (local_c0 == 0) {
                        sVar19 = *(short *)(iVar12 + 0x46);
                      }
                      else {
                        sVar19 = *(short *)(sVar19 * 0x20 + *(int *)(local_c0 + 0x38));
                      }
                      iVar12 = *(int *)(*(int *)((*(uint *)(psVar13 + 0x18) & 0xffff) * 0x20 + 0x14
                                                + DAT_0087bc14) + 100);
                      iVar14 = texture_cache_get(0,1);
                      if (iVar14 != 0) {
                        FUN_004acad0(0,sVar19 * 0x30 + iVar12,&local_64,local_a8,0,uStack_b8,
                                     1 < sVar2);
                      }
                    }
LAB_004b33c9:
                    sVar9 = sVar9 + 1;
                    iVar12 = (int)sVar9;
                  } while (iVar12 < *(int *)(psVar13 + 0x1a));
                }
              }
              sVar10 = sVar10 + 1;
              iVar12 = (int)sVar10;
            } while (iVar12 < *(int *)(iVar16 + 0x84));
          }
          sVar11 = sVar11 + 1;
        } while (sVar11 < sVar18);
      }
    }
  }
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
