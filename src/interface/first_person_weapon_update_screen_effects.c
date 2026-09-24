// first_person_weapon_update_screen_effects  (Ghidra: FUN_00494730, unnamed)
// address 0x494730, size 946 bytes
// name confidence: 0.4   rewrite confidence: 0.65
// evidence: out/phase4/interface_functions.md "Per-frame computation of the local player's
// screen tint/flash and widescreen letterbox effect, followed by HUD post-render and post-game
// overlay handling."; the tag it reads is the weapon HUD interface returned by
// local_player_get_weapon_hud_interface (0x494560), and +0xac/+0xb0 is
// WeaponHUDInterface.screen_effect, so every field offset below is a named
// WeaponHUDInterfaceScreenEffect field (types/tags.h).
// register convention: none (void).
// Review pass (phase 4): rebuilt from the disassembly (0x494730..0x494ae1). The goto graph
// Ghidra showed reduces to three independent gates of the same shape: a screen effect part
// runs when the player is zoomed (desired_zoom_level != -1) or its only_when_zoomed flag
// (bit 0) is clear. The convolution part additionally needs render_local_view_count <= 1 (it also picks
// the fullscreen or splitscreen mask). The connect_to_flashlight flag (bit 1) scales by the
// intensity out-value of 0x494560. The script-source values come from cinematic_screen_effect_get_script_value (source in
// EAX); the original clamps the value with up to three separate calls, kept as is.
// The rasterizer path needs rasterizer_device_version >= 0xffff0101 and byte 0x69c68a clear;
// otherwise 0x52e2d0 gets the parameter block (or NULL) in EAX.
// UNSURE: render_local_view_count (returns an int16 compared with 1, likely the local player count),
// cinematic_screen_effect_get_script_value (script-source value), 0x52d8a0, 0x52e2d0, 0x4a99f0, 0x45d700 and 0x45f220 are
// foreign; names kept from symbols/functions.txt where they exist. 0x7c313c is read as the
// camera field of view.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "game.h"
#include "networking.h"
#include "interface.h"
#include <string.h>

extern int16_t current_local_player_index; // 0x007c3108
extern tag_instance *tag_instances;        // 0x0087bc14
extern player_control_globals *player_control_globals_ptr; // 0x006b145c
extern float camera_field_of_view;          // 0x007c313c, UNSURE name
extern uint32_t rasterizer_device_version; // 0x007c118c
extern uint8_t unknown_0069c68a;            // 0x0069c68a, UNSURE: disables the technique path
extern game_engine_definition *current_game_engine; // 0x006f1d20
extern game_engine_state game_engine_state_value; // 0x0087aa10

extern int32_t local_player_get_weapon_hud_interface(float *out_intensity); // 0x494560
extern int16_t render_local_view_count(void);                       // 0x4c9220, UNSURE
extern float cinematic_screen_effect_get_script_value(uint16_t source);               // 0x5121a0; blam-cc: EAX -> source
extern void rasterizer_screen_effect_render(weapon_screen_effect_parameters *parameters); // 0x52d8a0
extern void rasterizer_screen_effect_render_fixed_function(weapon_screen_effect_parameters *parameters); // 0x52e2d0; blam-cc: EAX -> parameters
extern void hud_update_player(void);                      // 0x4a99f0
extern void game_engine_post_rasterize_post_game(void);   // 0x45d700
extern void hud_update_teammate_nameplate_fade(void);                            // 0x45f220, UNSURE

static float clamp_unit(float value)
{
    if (value < 0.0f) {
        return 0.0f;
    }
    if (value > 1.0f) {
        return 1.0f;
    }
    return value;
}

// The original calls the source function once per comparison and once more for the value.
static float script_source_value(uint16_t source)
{
    if (cinematic_screen_effect_get_script_value(source) < 0.0f) {
        return 0.0f;
    }
    if (cinematic_screen_effect_get_script_value(source) > 1.0f) {
        return 1.0f;
    }
    return cinematic_screen_effect_get_script_value(source);
}

void first_person_weapon_update_screen_effects(void)
{
    float intensity;                               // esp+0x00, out of 0x494560
    weapon_screen_effect_parameters parameters;    // esp+0x0c after the pushes
    int32_t hud_interface;
    WeaponHUDInterfaceScreenEffect *effect;
    int16_t desired_zoom_level;
    uint8_t zoomed;
    float amount;

    if (current_local_player_index == -1) {
        return;
    }

    hud_interface = local_player_get_weapon_hud_interface(&intensity);
    if (hud_interface == -1 ||
        (int32_t)((WeaponHUDInterface *)tag_instances[hud_interface & 0xffff].data)->screen_effect.count < 1) {
        if (rasterizer_device_version >= 0xffff0101u && unknown_0069c68a == 0) {
            rasterizer_screen_effect_render((weapon_screen_effect_parameters *)0);
        } else {
            rasterizer_screen_effect_render_fixed_function((weapon_screen_effect_parameters *)0);
        }
        goto post_hud;
    }

    effect = (WeaponHUDInterfaceScreenEffect *)
        ((WeaponHUDInterface *)tag_instances[hud_interface & 0xffff].data)->screen_effect.pointer;
    desired_zoom_level = -1;
    if (current_local_player_index != -1) {
        desired_zoom_level =
            player_control_globals_ptr->local_players[current_local_player_index].desired_zoom_level;
    }
    zoomed = (desired_zoom_level != -1);
    memset(&parameters, 0, sizeof(parameters));

    // mask
    if (zoomed || (effect->mask_flags & 1) == 0) {
        datum_index mask = (render_local_view_count() > 1) ? *(datum_index *)&effect->mask_splitscreen.tag_id
                                                : *(datum_index *)&effect->mask_fullscreen.tag_id;
        if (mask != (datum_index)-1) {
            parameters.mask_bitmap_data =
                *(uint32_t *)((uint8_t *)tag_instances[mask & 0xffff].data + 0x64);
            parameters.night_vision_masked = (uint8_t)((effect->even_more_flags >> 2) & 1);
            parameters.desaturation_masked = (uint8_t)((effect->desaturation_flags >> 3) & 1);
        }
    }

    // convolution
    if (render_local_view_count() <= 1 && (zoomed || (effect->convolution_flags & 1) == 0)) {
        if (effect->convolution_fov_in_bounds[0] == effect->convolution_fov_in_bounds[1]) {
            amount = effect->convolution_radius_out_bounds[1];
        } else {
            float t = clamp_unit((camera_field_of_view - effect->convolution_fov_in_bounds[0]) /
                                 (effect->convolution_fov_in_bounds[1] -
                                  effect->convolution_fov_in_bounds[0]));
            amount = (1.0f - t) * effect->convolution_radius_out_bounds[0] +
                     t * effect->convolution_radius_out_bounds[1];
        }
        if (amount > 0.0f) {
            parameters.convolution_amount = amount;
            parameters.convolution_type = 2;
        }
    }

    // night vision
    if (zoomed || (effect->even_more_flags & 1) == 0) {
        float value = effect->night_vision_intensity;
        if ((effect->even_more_flags & 2) != 0) {
            value = clamp_unit(intensity) * value;
        }
        value = script_source_value((uint16_t)effect->night_vision_script_source) * value;
        if (value > 0.0f) {
            parameters.night_vision_intensity = value;
        }
    }

    // desaturation
    if (zoomed || (effect->desaturation_flags & 1) == 0) {
        float value = effect->desaturation_intensity;
        if ((effect->desaturation_flags & 2) != 0) {
            value = clamp_unit(intensity) * value;
        }
        value = script_source_value((uint16_t)effect->desaturation_script_source) * value;
        if (value > 0.0f) {
            parameters.desaturation_intensity = value;
            parameters.desaturation_additive = (uint8_t)((effect->desaturation_flags >> 2) & 1);
            parameters.desaturation_tint[0] = effect->effect_tint.red;
            parameters.desaturation_tint[1] = effect->effect_tint.green;
            parameters.desaturation_tint[2] = effect->effect_tint.blue;
        }
    }

    if (rasterizer_device_version >= 0xffff0101u && unknown_0069c68a == 0) {
        rasterizer_screen_effect_render(&parameters);
    } else {
        rasterizer_screen_effect_render_fixed_function(&parameters);
    }

post_hud:
    hud_update_player();
    if (current_game_engine != (void *)0) {
        if ((int32_t)game_engine_state_value > 1) {
            game_engine_post_rasterize_post_game();
            return;
        }
        hud_update_teammate_nameplate_fade();
    }
}

#if 0
Original Ghidra decompilation (0x494730):

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void FUN_00494730(void)

{
  float fVar1;
  int iVar2;
  short sVar3;
  uint uVar4;
  int iVar5;
  int extraout_EDX;
  int extraout_EDX_00;
  int extraout_EDX_01;
  int extraout_EDX_02;
  int extraout_EDX_03;
  int extraout_EDX_04;
  int extraout_EDX_05;
  int extraout_EDX_06;
  int iVar6;
  undefined4 *puVar7;
  bool bVar8;
  float10 fVar9;
  float local_40;
  float local_3c;
  undefined4 local_38;
  float local_34;
  undefined4 local_30;
  float local_2c;
  float local_28;
  undefined4 local_24;
  undefined4 local_20;
  undefined4 local_1c;
  byte local_18;
  byte local_17;
  byte local_16;

  if (DAT_007c3108 == -1) {
    return;
  }
  uVar4 = FUN_00494560(&local_40);
  iVar6 = DAT_0087bc14;
  if ((uVar4 == 0xffffffff) ||
     (iVar2 = *(int *)((uVar4 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14), *(int *)(iVar2 + 0xac) < 1))
  {
    if ((0xffff0100 < DAT_007c118c) && (DAT_0069c68a == '\0')) {
      rasterizer_screen_effect_video_technique_select(0);
      goto LAB_00494ab8;
    }
  }
  else {
    iVar2 = *(int *)(iVar2 + 0xb0);
    sVar3 = -1;
    if (DAT_007c3108 != -1) {
      sVar3 = *(short *)(DAT_007c3108 * 0x40 + 0x34 + DAT_006b145c);
    }
    bVar8 = sVar3 != -1;
    puVar7 = &local_38;
    for (iVar5 = 0xe; iVar5 != 0; iVar5 = iVar5 + -1) {
      *puVar7 = 0;
      puVar7 = puVar7 + 1;
    }
    if ((bVar8) || ((*(byte *)(iVar2 + 4) & 1) == 0)) {
      sVar3 = FUN_004c9220();
      if (sVar3 < 2) {
        uVar4 = *(uint *)(extraout_EDX + 0x24);
      }
      else {
        uVar4 = *(uint *)(extraout_EDX + 0x34);
      }
      if (uVar4 != 0xffffffff) {
        local_30 = *(undefined4 *)(*(int *)((uVar4 & 0xffff) * 0x20 + 0x14 + iVar6) + 100);
        local_17 = *(byte *)(extraout_EDX + 0x6c) >> 2 & 1;
        local_16 = *(byte *)(extraout_EDX + 0x8c) >> 3 & 1;
      }
    }
    sVar3 = FUN_004c9220();
    if (sVar3 < 2) {
      if ((bVar8) || ((*(byte *)(extraout_EDX_00 + 0x40) & 1) == 0)) {
        if (*(float *)(extraout_EDX_00 + 0x44) == *(float *)(extraout_EDX_00 + 0x48)) {
          fVar1 = *(float *)(extraout_EDX_00 + 0x50);
        }
        else {
          if (0.0 <= (_DAT_007c313c - *(float *)(extraout_EDX_00 + 0x44)) /
                     (*(float *)(extraout_EDX_00 + 0x48) - *(float *)(extraout_EDX_00 + 0x44))) {
            if ((_DAT_007c313c - *(float *)(extraout_EDX_00 + 0x44)) /
                (*(float *)(extraout_EDX_00 + 0x48) - *(float *)(extraout_EDX_00 + 0x44)) <= 1.0) {
              fVar1 = (_DAT_007c313c - *(float *)(extraout_EDX_00 + 0x44)) /
                      (*(float *)(extraout_EDX_00 + 0x48) - *(float *)(extraout_EDX_00 + 0x44));
            }
            else {
              fVar1 = 1.0;
            }
          }
          else {
            fVar1 = 0.0;
          }
          fVar1 = fVar1 * *(float *)(extraout_EDX_00 + 0x50) +
                  (1.0 - fVar1) * *(float *)(extraout_EDX_00 + 0x4c);
        }
        if (0.0 < fVar1) {
          local_38._2_2_ = 2;
          local_34 = fVar1;
        }
        goto LAB_004948c0;
      }
LAB_004948c4:
      iVar6 = extraout_EDX_00;
      if ((*(byte *)(extraout_EDX_00 + 0x6c) & 1) == 0) goto LAB_004948ce;
LAB_0049497b:
      if ((*(byte *)(iVar6 + 0x8c) & 1) == 0) goto LAB_00494988;
    }
    else {
LAB_004948c0:
      if (!bVar8) goto LAB_004948c4;
LAB_004948ce:
      local_3c = *(float *)(extraout_EDX_00 + 0x70);
      if ((*(byte *)(extraout_EDX_00 + 0x6c) & 2) != 0) {
        if (0.0 <= local_40) {
          fVar1 = local_40;
          if (1.0 < local_40) {
            fVar1 = 1.0;
          }
        }
        else {
          fVar1 = 0.0;
        }
        local_3c = fVar1 * local_3c;
      }
      fVar9 = (float10)FUN_005121a0();
      if ((float10)0.0 <= fVar9) {
        fVar9 = (float10)FUN_005121a0();
        if (fVar9 <= (float10)1.0) {
          fVar9 = (float10)FUN_005121a0();
          iVar6 = extraout_EDX_03;
        }
        else {
          fVar9 = (float10)1.0;
          iVar6 = extraout_EDX_02;
        }
      }
      else {
        fVar9 = (float10)0.0;
        iVar6 = extraout_EDX_01;
      }
      if ((float10)0.0 < fVar9 * (float10)local_3c) {
        local_2c = (float)(fVar9 * (float10)local_3c);
      }
      if (!bVar8) goto LAB_0049497b;
LAB_00494988:
      local_3c = *(float *)(iVar6 + 0x90);
      if ((*(byte *)(iVar6 + 0x8c) & 2) != 0) {
        if (0.0 <= local_40) {
          if (1.0 < local_40) {
            local_40 = 1.0;
          }
        }
        else {
          local_40 = 0.0;
        }
        local_3c = local_40 * local_3c;
      }
      fVar9 = (float10)FUN_005121a0();
      if ((float10)0.0 <= fVar9) {
        fVar9 = (float10)FUN_005121a0();
        if (fVar9 <= (float10)1.0) {
          fVar9 = (float10)FUN_005121a0();
          iVar6 = extraout_EDX_06;
        }
        else {
          fVar9 = (float10)1.0;
          iVar6 = extraout_EDX_05;
        }
      }
      else {
        fVar9 = (float10)0.0;
        iVar6 = extraout_EDX_04;
      }
      if ((float10)0.0 < fVar9 * (float10)local_3c) {
        local_28 = (float)(fVar9 * (float10)local_3c);
        local_18 = *(byte *)(iVar6 + 0x8c) >> 2 & 1;
        local_24 = *(undefined4 *)(iVar6 + 0x94);
        local_20 = *(undefined4 *)(iVar6 + 0x98);
        local_1c = *(undefined4 *)(iVar6 + 0x9c);
      }
    }
    if ((0xffff0100 < DAT_007c118c) && (DAT_0069c68a == '\0')) {
      rasterizer_screen_effect_video_technique_select(&local_38);
      goto LAB_00494ab8;
    }
  }
  chimera__widescreen_screen_effect();
LAB_00494ab8:
  hud_update_player();
  if (DAT_006f1d20 != 0) {
    if (1 < DAT_0087aa10) {
      game_engine_post_rasterize_post_game();
      return;
    }
    FUN_0045f220();
  }
  return;
}
#endif
