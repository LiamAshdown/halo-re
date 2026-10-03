// player_trigger_kill_streak_effect  (Ghidra: FUN_004797d0; renamed -- called from
// player_apply_pickup_effect.c (this batch) after a slot-0 kill-streak pickup, single player
// only)
// address 0x4797d0, size 179 bytes
// name confidence: 0.3   rewrite confidence: 0.35
// evidence: identical shape to the sibling player_trigger_shield_recharge_effect.c (this
//   batch), reading a different set of six globals into the same 0x38-byte effect-parameter
//   buffer layout (see that file's header for the full offset derivation).
// register convention: a player index in EDX (in_EDX); no stack parameters.
//   // blam-cc: EDX -> player_index
// UNSURE: same as the sibling file.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern data_array *player_data; // 0x0087a480

extern uint16_t global_006889e4; // 0x006889e4
extern uint16_t global_007102f0; // 0x007102f0
extern uint32_t global_006889f4; // 0x006889f4
extern uint32_t global_007102f4; // 0x007102f4
extern uint32_t global_006889e8; // 0x006889e8
extern uint32_t global_006889f0; // 0x006889f0
extern uint32_t global_006889ec; // 0x006889ec
extern uint32_t global_007102f8; // 0x007102f8

extern void player_effect_set_screen_flash_for_player(datum_index player_index, void *descriptor, float intensity_falloff); // 0x456980, EAX player, stack (descriptor, intensity_falloff)

// blam-cc: EDX -> player_index
void player_trigger_kill_streak_effect(uint32_t player_index)
{
    player *p = (player *)((uint8_t *)player_data->data + (player_index & 0xffff) * sizeof(player));

    if (player_index != 0xffffffff && p->local_player_index != -1) {
        uint8_t buffer[0x38];
        int32_t i;
        for (i = 0; i < 0x38; i++) {
            buffer[i] = 0;
        }

        *(uint16_t *)(buffer + 0x00) = global_006889e4;
        *(uint16_t *)(buffer + 0x02) = 2;
        *(uint32_t *)(buffer + 0x10) = global_006889f4;
        *(uint16_t *)(buffer + 0x14) = global_007102f0;
        *(uint32_t *)(buffer + 0x20) = global_006889e8;
        *(uint32_t *)(buffer + 0x24) = 0;
        *(uint32_t *)(buffer + 0x28) = global_007102f4;
        *(uint32_t *)(buffer + 0x2c) = global_006889ec;
        *(uint32_t *)(buffer + 0x30) = global_006889f0;
        *(uint32_t *)(buffer + 0x34) = global_007102f8;

        player_effect_set_screen_flash_for_player(player_index, buffer, 1.0f); // FIXED: EAX = the player (the draft dropped it and passed 1.0f as an integer) // 1.0f
    }
}

#if 0
Original Ghidra decompilation (0x4797d0), from tools/pack.py 0x4797d0:

void FUN_004797d0(void)

{
  int iVar1;
  uint in_EDX;
  undefined4 *puVar2;
  undefined2 local_38;
  undefined4 local_36 [3];
  undefined4 local_28;
  undefined2 local_24;
  undefined4 local_18;
  undefined4 local_14;
  undefined4 local_10;
  undefined4 local_c;
  undefined4 local_8;
  undefined4 local_4;

  if ((in_EDX != 0xffffffff) &&
     (*(short *)((in_EDX & 0xffff) * 0x200 + 2 + *(int *)(DAT_0087a480 + 0x34)) != -1)) {
    puVar2 = local_36;
    for (iVar1 = 0xd; iVar1 != 0; iVar1 = iVar1 + -1) {
      *puVar2 = 0;
      puVar2 = puVar2 + 1;
    }
    *(undefined2 *)puVar2 = 0;
    local_24 = DAT_007102f0;
    local_28 = DAT_006889f4;
    local_38 = DAT_006889e4;
    local_10 = DAT_007102f4;
    local_18 = DAT_006889e8;
    local_8 = DAT_006889f0;
    local_c = DAT_006889ec;
    local_36[0]._0_2_ = 2;
    local_14 = 0;
    local_4 = DAT_007102f8;
    FUN_00456980(&local_38,0x3f800000);
  }
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
