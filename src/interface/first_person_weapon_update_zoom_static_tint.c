// first_person_weapon_update_zoom_static_tint  (Ghidra: FUN_00494af0, unnamed)
// address 0x494af0, size 425 bytes
// name confidence: 0.3   rewrite confidence: 0.95
// evidence: out/phase4/interface_functions.md "Computes a randomized static/flicker color tint
// applied while the local player is zoomed in"; the tag is the weapon HUD interface from
// local_player_get_weapon_hud_interface (0x494560) and +0xac/+0xb0 is its screen_effect block,
// so the fields read are named WeaponHUDInterfaceScreenEffect fields.
// register convention: a bool flag in AL. // blam-cc: AL -> enabled
// Review pass (phase 4): rebuilt from the disassembly (0x494af0..0x494c98).
//  - FUN_00472740 is local_player_get_zoom_level (src/game, CX -> local player index) and the
//    early-out is "not zoomed AND mask only_when_zoomed", the first rewrite had it inverted.
//  - render_local_view_count returns 1 during the post game and otherwise a local player count (reads
//    player_globals +0x0c); the tint needs it <= 1.
//  - The tint needs mask_fullscreen assigned and desaturation connect_to_flashlight (bit 1):
//    it is the desaturation intensity scaled by the clamped flashlight intensity and the
//    clamped desaturation script-source value (cinematic_screen_effect_get_script_value, source in EAX, called up to three
//    times as the original does), capped at 0.75 per channel.
// Verified against the disassembly 0x494af0..0x494c98: the clamps, the three script-value calls, the 0.75 cap and the offsets
// (effect +4 mask_flags, +0x24 mask_fullscreen tag id, +0x8c/+0x8e/+0x90 desaturation flags/source/intensity) match. The three
// 0x71d190 floats are named for their use as a tint; the rasterizer reader is not rewritten.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "game.h"
#include "networking.h"
#include "interface.h"

extern tag_instance *tag_instances; // 0x0087bc14
extern int16_t current_local_player_index; // 0x007c3108
extern float zoom_static_tint_r; // 0x0071d190
extern float zoom_static_tint_g; // 0x0071d194
extern float zoom_static_tint_b; // 0x0071d198

extern int32_t local_player_get_weapon_hud_interface(float *out_intensity); // 0x494560
extern int32_t local_player_get_zoom_level(int16_t local_player_index); // 0x472740, src/game; blam-cc: CX
extern int16_t render_local_view_count(void);                  // 0x4c9220, UNSURE: effective local player count
extern float cinematic_screen_effect_get_script_value(int16_t index);          // 0x5121a0; blam-cc: EAX -> index (signed 16-bit: `movsx edx,[+0x8e]`)

// blam-cc: AL -> enabled
void first_person_weapon_update_zoom_static_tint(uint8_t enabled)
{
    float intensity;
    int32_t hud_interface;
    WeaponHUDInterfaceScreenEffect *effect;
    float scale;
    float source_value;
    float product;
    int16_t source;

    zoom_static_tint_r = 0.0f;
    zoom_static_tint_g = 0.0f;
    zoom_static_tint_b = 0.0f;

    if (enabled == 0 || current_local_player_index == -1) {
        return;
    }
    hud_interface = local_player_get_weapon_hud_interface(&intensity);
    if (hud_interface == -1) {
        return;
    }
    if ((int32_t)((WeaponHUDInterface *)tag_instances[hud_interface & 0xffff].data)->screen_effect.count <= 0) {
        return;
    }
    effect = (WeaponHUDInterfaceScreenEffect *)
        ((WeaponHUDInterface *)tag_instances[hud_interface & 0xffff].data)->screen_effect.pointer;

    if ((int16_t)local_player_get_zoom_level(current_local_player_index) == -1 &&
        (effect->mask_flags & 1) != 0) {
        return;
    }
    if (render_local_view_count() > 1) {
        return;
    }
    if (*(datum_index *)&effect->mask_fullscreen.tag_id == (datum_index)-1) {
        return;
    }
    if ((effect->desaturation_flags & 2) == 0) {
        return;
    }

    if (intensity < 0.0f) {
        scale = 0.0f;
    } else if (intensity > 1.0f) {
        scale = 1.0f;
    } else {
        scale = intensity;
    }
    scale = scale * effect->desaturation_intensity;

    source = effect->desaturation_script_source;
    if (cinematic_screen_effect_get_script_value(source) < 0.0f) {
        source_value = 0.0f;
    } else if (cinematic_screen_effect_get_script_value(source) > 1.0f) {
        source_value = 1.0f;
    } else {
        source_value = cinematic_screen_effect_get_script_value(source);
    }

    product = source_value * scale;
    if (product > 0.0f) {
        zoom_static_tint_r = (product < 0.75f) ? product : 0.75f;
        zoom_static_tint_g = (product < 0.75f) ? product : 0.75f;
        zoom_static_tint_b = (product < 0.75f) ? product : 0.75f;
    }
}

#if 0
Original Ghidra decompilation (0x494af0):

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void FUN_00494af0(void)

{
  char in_AL;
  short sVar1;
  uint uVar2;
  int extraout_EDX;
  int extraout_EDX_00;
  float10 fVar3;
  float local_4;

  _DAT_0071d190 = 0.0;
  _DAT_0071d194 = 0.0;
  _DAT_0071d198 = 0.0;
  if (((((in_AL != '\0') && (DAT_007c3108 != -1)) &&
       (uVar2 = FUN_00494560(&local_4), uVar2 != 0xffffffff)) &&
      (0 < *(int *)(*(int *)((uVar2 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14) + 0xac))) &&
     (((sVar1 = FUN_00472740(), sVar1 != -1 || ((*(byte *)(extraout_EDX + 4) & 1) == 0)) &&
      ((sVar1 = FUN_004c9220(), sVar1 < 2 &&
       ((*(int *)(extraout_EDX_00 + 0x24) != -1 && ((*(byte *)(extraout_EDX_00 + 0x8c) & 2) != 0))))
      )))) {
    if (0.0 <= local_4) {
      if (1.0 < local_4) {
        local_4 = 1.0;
      }
    }
    else {
      local_4 = 0.0;
    }
    local_4 = local_4 * *(float *)(extraout_EDX_00 + 0x90);
    fVar3 = (float10)FUN_005121a0();
    if ((float10)0.0 <= fVar3) {
      fVar3 = (float10)FUN_005121a0();
      if (fVar3 <= (float10)1.0) {
        fVar3 = (float10)FUN_005121a0();
      }
      else {
        fVar3 = (float10)1.0;
      }
    }
    else {
      fVar3 = (float10)0.0;
    }
    fVar3 = fVar3 * (float10)local_4;
    if ((float10)0.0 < fVar3) {
      if ((float10)0.75 <= fVar3) {
        _DAT_0071d190 = 0.75;
      }
      else {
        _DAT_0071d190 = (float)fVar3;
      }
      if ((float10)0.75 <= fVar3) {
        _DAT_0071d194 = 0.75;
      }
      else {
        _DAT_0071d194 = (float)fVar3;
      }
      if (fVar3 < (float10)0.75) {
        _DAT_0071d198 = (float)fVar3;
        return;
      }
      _DAT_0071d198 = 0.75;
    }
  }
  return;
}
#endif
