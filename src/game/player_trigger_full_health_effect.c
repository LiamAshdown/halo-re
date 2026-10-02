// player_trigger_full_health_effect  (Ghidra: FUN_00479890; renamed -- called from
// player_apply_pickup_effect.c (this batch) after a full-health-restore pickup, single player
// only)
// address 0x479890, size 157 bytes
// name confidence: 0.3   rewrite confidence: 0.4
// evidence: identical shape to player_trigger_shield_recharge_effect.c (this batch), but with
//   every field a literal constant instead of a global read (see that file's header for the
//   full 0x38-byte buffer offset derivation). The float bit patterns are 2.0, 0.5, 1.0 and
//   0.9238795 (three times) respectively.
// register convention: a player index in EDX (in_EDX); no stack parameters.
//   // blam-cc: EDX -> player_index

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern data_array *player_data; // 0x0087a480

extern void player_effect_set_screen_flash_for_player(datum_index player_index, void *descriptor, float intensity_falloff); // 0x456980, EAX player, stack (descriptor, intensity_falloff)

// blam-cc: EDX -> player_index
void player_trigger_full_health_effect(uint32_t player_index)
{
    player *p = (player *)((uint8_t *)player_data->data + (player_index & 0xffff) * sizeof(player));

    if (player_index != 0xffffffff && p->local_player_index != -1) {
        uint8_t buffer[0x38];
        int32_t i;
        for (i = 0; i < 0x38; i++) {
            buffer[i] = 0;
        }

        *(uint16_t *)(buffer + 0x00) = 6;
        *(uint16_t *)(buffer + 0x02) = 2;
        *(uint32_t *)(buffer + 0x10) = 0x40000000; // 2.0f
        *(uint16_t *)(buffer + 0x14) = 1;
        *(uint32_t *)(buffer + 0x20) = 0x3f000000; // 0.5f
        *(uint32_t *)(buffer + 0x24) = 0;
        *(uint32_t *)(buffer + 0x28) = 0x3f800000; // 1.0f
        *(uint32_t *)(buffer + 0x2c) = 0x3f6aeaea; // 0.9238795f
        *(uint32_t *)(buffer + 0x30) = 0x3f6aeaea; // 0.9238795f
        *(uint32_t *)(buffer + 0x34) = 0x3f6aeaea; // 0.9238795f

        player_effect_set_screen_flash_for_player(player_index, buffer, 1.0f); // FIXED: EAX = the player (the draft dropped it and passed 1.0f as an integer) // 1.0f
    }
}

#if 0
Original Ghidra decompilation (0x479890), from tools/pack.py 0x479890:

void FUN_00479890(void)

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
    local_24 = 1;
    local_38 = 6;
    local_36[0]._0_2_ = 2;
    local_28 = 0x40000000;
    local_18 = 0x3f000000;
    local_14 = 0;
    local_10 = 0x3f800000;
    local_c = 0x3f6aeaea;
    local_8 = 0x3f6aeaea;
    local_4 = 0x3f6aeaea;
    FUN_00456980(&local_38,0x3f800000);
  }
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
