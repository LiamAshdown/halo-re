// game_engine_animate_hill_pulse_icons  (Ghidra: FUN_0046f450; renamed, no established name)
// address 0x46f450, size 239 bytes
// name confidence: 0.3   rewrite confidence: 0.4
// evidence: out/phase4/game_functions.md ("Animates a pair of HUD icon scale/alpha values, one
// fading down and one growing up over time, consistent with a hill-control pulse indicator");
// types/game.h player::speed (+0x6c, "constructors write 1.0; part of the profile").
// register convention: two player handles, EAX and ECX (Ghidra's `in_EAX`/`in_ECX`).
//   // blam-cc: EAX -> fading_player, ECX -> growing_player

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern uint8_t hill_pulse_fade_done;  // 0x006f1d05, UNSURE exact meaning ("done fading down" latch)
extern uint8_t hill_pulse_grow_done;   // 0x006f1d04, UNSURE exact meaning ("done growing up" latch)
extern data_array *player_data;         // 0x0087a480

// blam-cc: EAX -> fading_player, ECX -> growing_player
// Once per call: while hill_pulse_fade_done is clear, decays fading_player->speed toward 0.9 (a
// fast -0.15 step kicks in once the slow -0.02 decay is still >= 1.0, itself clamped back up to
// 1.0 if it undershoots); while hill_pulse_grow_done is clear, grows growing_player->speed toward
// 1.5 (a second +0.1 step kicks in while the first is still <= 1.0, clamped back down to 1.0 if
// it overshoots), snapping to exactly 1.5 once it would exceed it.
void game_engine_animate_hill_pulse_icons(datum_index fading_player, datum_index growing_player)
{
    player *fading = (player *)((uint8_t *)player_data->data + (uint32_t)(uint16_t)fading_player * sizeof(player));
    player *growing = (player *)((uint8_t *)player_data->data + (uint32_t)(uint16_t)growing_player * sizeof(player));

    if (!hill_pulse_fade_done) {
        float speed = fading->speed - 0.02f;

        fading->speed = speed;
        if (speed >= 1.0f) {
            speed = speed - 0.15f;
            fading->speed = speed;
            if (speed <= 1.0f) {
                speed = 1.0f;
            }
            fading->speed = speed;
        }
        fading->speed = (fading->speed <= 0.9f) ? 0.9f : fading->speed;
    }

    if (!hill_pulse_grow_done) {
        float speed = growing->speed + 0.1f;

        growing->speed = speed;
        if (speed <= 1.0f) { // Ghidra's `(a < b) != (a == b)` idiom on `speed < 1.0`/`speed == 1.0`
                              // reduces to `speed <= 1.0` (see angle_delta_wrapped.c for the
                              // sibling derivation)
            speed = speed + 0.1f;
            growing->speed = speed;
            if (speed > 1.0f) {
                speed = 1.0f;
            }
            growing->speed = speed;
        }
        if (growing->speed > 1.5f) {
            growing->speed = 1.5f;
        }
    }
}

#if 0
Original Ghidra decompilation (0x46f450), from tools/pack.py 0x46f450:

void FUN_0046f450(void)

{
  undefined4 uVar1;
  float fVar2;
  uint in_EAX;
  int iVar3;
  uint in_ECX;
  int iVar4;

  iVar3 = (in_EAX & 0xffff) * 0x200 + *(int *)(DAT_0087a480 + 0x34);
  iVar4 = (in_ECX & 0xffff) * 0x200 + *(int *)(DAT_0087a480 + 0x34);
  if (DAT_006f1d05 == '\0') {
    fVar2 = *(float *)(iVar3 + 0x6c) - 0.02;
    *(float *)(iVar3 + 0x6c) = fVar2;
    if (1.0 <= fVar2) {
      fVar2 = fVar2 - 0.15;
      *(float *)(iVar3 + 0x6c) = fVar2;
      if (fVar2 <= 1.0) {
        fVar2 = 1.0;
      }
      *(float *)(iVar3 + 0x6c) = fVar2;
    }
    if (*(float *)(iVar3 + 0x6c) <= 0.9) {
      uVar1 = 0x3f666666;
    }
    else {
      uVar1 = *(undefined4 *)(iVar3 + 0x6c);
    }
    *(undefined4 *)(iVar3 + 0x6c) = uVar1;
  }
  if (DAT_006f1d04 == '\0') {
    fVar2 = *(float *)(iVar4 + 0x6c) + 0.1;
    *(float *)(iVar4 + 0x6c) = fVar2;
    if (fVar2 < 1.0 != (fVar2 == 1.0)) {
      fVar2 = fVar2 + 0.1;
      *(float *)(iVar4 + 0x6c) = fVar2;
      if (1.0 < fVar2) {
        fVar2 = 1.0;
      }
      *(float *)(iVar4 + 0x6c) = fVar2;
    }
    if (1.5 < *(float *)(iVar4 + 0x6c)) {
      *(undefined4 *)(iVar4 + 0x6c) = 0x3fc00000;
      return;
    }
    *(undefined4 *)(iVar4 + 0x6c) = *(undefined4 *)(iVar4 + 0x6c);
  }
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
