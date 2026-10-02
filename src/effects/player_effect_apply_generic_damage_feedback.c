// player_effect_apply_generic_damage_feedback  (Ghidra: FUN_004569d0, still unnamed there)
// address 0x4569d0, size 251 bytes
// name confidence: 0.3   rewrite confidence: 0.9 (VERIFIED against objdump; descriptor fields FIXED)
// evidence: types/effects.h player_screen_flash and player_camera_shake; this module's
//   player_effect_set_screen_flash_for_player (0x456980) and player_effect_set_camera_shake
//   (0x457d50). out/phase4/effects_types_notes.md's misattribution table places this address in
//   the player_effect group, not the "contrail" framing functions.md guessed at phase 2.
// reconciled: 0x006851fc is a pointer to the opaque-white ColorARGB (0x00655138); one name global_white_argb: a player datum_index in EDX (in_EDX); duration as the recognized stack
//   parameter (param_1).
//   // blam-cc: in_EDX -> player_index, stack -> duration
// UNSURE (extensive): both local descriptors are built on the stack and Ghidra tracked only
//   fragments of each (a 14-dword zeroed block plus one explicit short and four dwords for the
//   screen flash descriptor; a 17-float zeroed block plus one explicit float for the shake
//   descriptor, one float short of player_camera_shake's real 18). The screen flash descriptor's
//   type is set to 2 with a default white color and full intensity; the shake descriptor's
//   second field (index 1) is set to duration*0.01 and everything else left at zero. The two
//   trailing calls are reconstructed with self/descriptor passed through registers Ghidra does
//   not show, by analogy with the two callees' own established signatures.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "cache.h"
#include "effects.h"
#include "game.h"
#include <string.h>
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern data_array *player_data;                               // 0x0087a480
extern player_effect_globals *player_effect_globals_pointer;  // 0x006f1884
extern const ColorARGB *global_white_argb;                          // 0x006851fc

extern void player_effect_set_screen_flash(player_effect *self, player_screen_flash *descriptor,
    float intensity_falloff, float duration_scale); // 0x4578a0, this module
extern void player_effect_set_camera_shake(player_effect *self, player_camera_shake *descriptor,
    float intensity_falloff, float duration_scale); // 0x457d50, this module

// FIXED (objdump 0x4569d0..0x456aca): the flash is {type 1, +0x02 = 2, duration 1.0, maximum (+0x20) = fraction,
//   weight (+0x24) 0, colour opaque white} and the shake {duration 1.0, +0x08 = fraction * 0.01}; both are applied
//   with (fraction, 1.0). The draft used type 2, the fraction as the duration, weight 1.0 and the shake's +0x04.
// blam-cc: EDX -> player_index, stack -> fraction
void player_effect_apply_generic_damage_feedback(datum_index player_index, float fraction)
{
    player_screen_flash flash_descriptor;
    player_camera_shake shake_descriptor;
    int16_t local_player_index;

    memset(&flash_descriptor, 0, sizeof(flash_descriptor));
    memset(&shake_descriptor, 0, sizeof(shake_descriptor));

    local_player_index = ((player *)player_data->data)[player_index & 0xffff].local_player_index;
    if (local_player_index != -1) {
        player_effect *self = &player_effect_globals_pointer->players[local_player_index];

        *(float *)&shake_descriptor.random_translation = (float)((double)fraction * 0.01);
        shake_descriptor.duration = 1.0f;
        flash_descriptor.type = 1;
        flash_descriptor.priority = 2;
        flash_descriptor.duration = 1.0f;
        *(float *)&flash_descriptor.maximum_intensity = fraction;
        flash_descriptor.intensity = 0.0f;
        flash_descriptor.color = *global_white_argb;

        player_effect_set_screen_flash(self, &flash_descriptor, fraction, 1.0f);
        player_effect_set_camera_shake(self, &shake_descriptor, fraction, 1.0f);
    }
}

#if 0
Original Ghidra decompilation (0x4569d0):

void FUN_004569d0(float param_1)

{
  short sVar1;
  int iVar2;
  uint in_EDX;
  undefined4 *puVar3;
  float *pfVar4;
  undefined4 local_7e [3];
  undefined4 local_70;
  float local_60;
  undefined4 local_5c;
  undefined4 local_58;
  undefined4 local_54;
  undefined4 local_50;
  undefined4 local_4c;
  undefined4 local_48;
  float local_44 [17];

  puVar3 = local_7e;
  for (iVar2 = 0xd; iVar2 != 0; iVar2 = iVar2 + -1) {
    *puVar3 = 0;
    puVar3 = puVar3 + 1;
  }
  *(undefined2 *)puVar3 = 0;
  pfVar4 = local_44;
  for (iVar2 = 0x11; iVar2 != 0; iVar2 = iVar2 + -1) {
    *pfVar4 = 0.0;
    pfVar4 = pfVar4 + 1;
  }
  sVar1 = *(short *)((in_EDX & 0xffff) * 0x200 + 2 + *(int *)(DAT_0087a480 + 0x34));
  if (sVar1 != -1) {
    local_44[1] = param_1 * 0.01;
    local_58 = *(undefined4 *)PTR_DAT_006851fc;
    local_60 = param_1;
    local_54 = *(undefined4 *)(PTR_DAT_006851fc + 4);
    local_50 = *(undefined4 *)(PTR_DAT_006851fc + 8);
    local_4c = *(undefined4 *)(PTR_DAT_006851fc + 0xc);
    local_48 = 0x3f800000;
    local_7e[0]._0_2_ = 2;
    local_70 = 0x3f800000;
    local_5c = 0;
    FUN_004578a0(sVar1 * 0xec + DAT_006f1884,param_1,0x3f800000);
    FUN_00457d50(param_1,0x3f800000);
  }
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
