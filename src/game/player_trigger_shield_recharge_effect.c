// player_trigger_shield_recharge_effect  (Ghidra: FUN_00479710; renamed -- called from
// player_apply_pickup_effect.c (this batch) on a shield-recharge pickup, single player only)
// address 0x479710, size 179 bytes
// name confidence: 0.3   rewrite confidence: 0.35
// evidence: shares its exact shape with the siblings player_trigger_kill_streak_effect.c and
//   player_trigger_full_health_effect.c (this batch), differing only in which constants are
//   copied into the 0x38-byte effect-parameter buffer; player_effect_set_screen_flash_for_player (already established,
//   src/game/game_engine_update_teleporter.c) takes that buffer plus a 1.0f scale. Ghidra's own
//   local variable names encode their exact stack offsets, which is what proves local_38
//   through local_4 are one contiguous 0x38-byte buffer that this rewrite keeps as a byte array
//   with explicit offset writes rather than a named struct (its field meanings are not
//   otherwise documented anywhere in this batch's evidence).
// register convention: a player index in EDX (in_EDX); no stack parameters.
//   // blam-cc: EDX -> player_index
// UNSURE: the effect-parameter buffer's own field layout/meaning (six of its dwords come
//   straight from six otherwise-undocumented globals); player_effect_set_screen_flash_for_player's full effect.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"

extern data_array *player_data; // 0x0087a480

extern uint16_t global_006889d0; // 0x006889d0
extern uint16_t global_007102e4; // 0x007102e4 (upper half of buffer+0x14, see below)
extern uint32_t global_006889e0; // 0x006889e0
extern uint32_t global_007102e8; // 0x007102e8
extern uint32_t global_006889d4; // 0x006889d4
extern uint32_t global_007102ec; // 0x007102ec
extern uint32_t global_006889d8; // 0x006889d8
extern uint32_t global_006889dc; // 0x006889dc

extern void player_effect_set_screen_flash_for_player(void *effect_struct, uint32_t one_point_zero); // 0x456980, established

// blam-cc: EDX -> player_index
// For a local player only, builds a zeroed 0x38-byte effect-parameter buffer, fills it from six
// module globals, and fires it via player_effect_set_screen_flash_for_player with a 1.0f scale.
void player_trigger_shield_recharge_effect(uint32_t player_index)
{
    player *p = (player *)((uint8_t *)player_data->data + (player_index & 0xffff) * sizeof(player));

    if (player_index != 0xffffffff && p->local_player_index != -1) {
        uint8_t buffer[0x38];
        int32_t i;
        for (i = 0; i < 0x38; i++) {
            buffer[i] = 0;
        }

        *(uint16_t *)(buffer + 0x00) = global_006889d0;
        *(uint16_t *)(buffer + 0x02) = 2;
        *(uint32_t *)(buffer + 0x10) = global_006889e0;
        *(uint16_t *)(buffer + 0x14) = global_007102e4;
        *(uint32_t *)(buffer + 0x20) = global_006889d4;
        *(uint32_t *)(buffer + 0x24) = 0;
        *(uint32_t *)(buffer + 0x28) = global_007102e8;
        *(uint32_t *)(buffer + 0x2c) = global_006889d8;
        *(uint32_t *)(buffer + 0x30) = global_007102ec;
        *(uint32_t *)(buffer + 0x34) = global_006889dc;

        player_effect_set_screen_flash_for_player(buffer, 0x3f800000); // 1.0f
    }
}

#if 0
Original Ghidra decompilation (0x479710), from tools/pack.py 0x479710:

void FUN_00479710(void)

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
    local_24 = DAT_007102e4;
    local_28 = DAT_006889e0;
    local_38 = DAT_006889d0;
    local_10 = DAT_007102e8;
    local_18 = DAT_006889d4;
    local_8 = DAT_007102ec;
    local_c = DAT_006889d8;
    local_36[0]._0_2_ = 2;
    local_14 = 0;
    local_4 = DAT_006889dc;
    FUN_00456980(&local_38,0x3f800000);
  }
  return;
}
#endif
