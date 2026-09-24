// ai_communication_line_fade_multiplier  (Ghidra: ai_communication_line_fade_multiplier; named for this rewrite)
// address 0x42f8c0, size 220 bytes
// name confidence: 0.3   rewrite confidence: 0.2
// evidence: phase-4 summary ("computes a fade/volume multiplier for a currently-playing or
// about-to-play communication line based on its type and elapsed time").
// register convention: BX -> some short threshold (unaff_BX, unresolved in Ghidra's own
// decompile); the six recognized parameters are all genuine stack arguments.
// blam-cc: BX -> short_range_limit (UNSURE), stack -> param_1, kind, param_3, param_4,
// apply_fade_window, volume
//
// UNSURE, substantially: the float __ftol truncates has no visible FPU setup in Ghidra's own
// decompile (a hidden dataflow this project already documents elsewhere, e.g.
// actor_reseed_movement_pause_timer.c); modeled here as a zero elapsed-fraction placeholder
// so the file compiles and the surrounding control flow is preserved, but the actual
// fade-in ramp value is not reconstructed. Needs a disassembly pass to recover.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "ai.h"

extern game_time_globals *game_time; // 0x006f1d6c

extern int16_t unit_animation_change_priority_check(uint32_t param_1, uint32_t kind, int32_t *out_last_played_tick); // 0x560d00, UNSURE args (2-out-param form; see other call sites in this batch)

// blam-cc: BX -> short_range_limit (UNSURE), stack -> param_1, kind, param_3, param_4,
// apply_fade_window, volume
// Looks up the line's status via unit_animation_change_priority_check; if it reports "already playing" (1), halves
// volume to 0.3x. If apply_fade_window is set and short_range_limit is small and the line
// has a recorded last-played tick, computes how long ago it played and, within a 60-tick
// fade-in window, scales volume down proportionally (or to 0 once the window has fully
// elapsed).
int32_t ai_communication_line_fade_multiplier(uint32_t param_1, uint32_t kind, uint32_t param_3,
                                               uint32_t param_4, uint8_t apply_fade_window,
                                               float *volume, int16_t short_range_limit)
{
    int32_t last_played_tick;
    int16_t status;
    int16_t elapsed;

    status = unit_animation_change_priority_check(kind, 1, &last_played_tick);
    if (status != 0 && status == 1) {
        *volume = *volume * 0.3f;
    }

    if (apply_fade_window != 0 && short_range_limit < 5 && last_played_tick != -1) {
        // The original is `*(int *)(DAT_006f1d6c + 0xc)`, i.e. game_time->game_time, not the
        // pointer itself; the first rewrite of this file dropped the + 0xc.
        int32_t ticks_ago = game_time->game_time - last_played_tick;
        uint16_t clamped = (ticks_ago < 0) ? 0 : (uint16_t)ticks_ago;
        // UNSURE: elapsed's real source (an untraced __ftol result) is not reconstructed.
        elapsed = 0;
        if (clamped <= (uint16_t)elapsed) {
            *volume = 0.0f;
            return 0;
        }
        if (clamped < elapsed + 0x3c) {
            *volume = (float)(clamped - elapsed) * *volume * 0.016666668f;
        }
        return status;
    }
    return status;
}

#if 0
Original Ghidra decompilation (0x42f8c0):

undefined4
FUN_0042f8c0(undefined4 param_1,undefined4 param_2,undefined4 param_3,undefined4 param_4,
            char param_5,float *param_6)

{
  short sVar1;
  undefined4 uVar2;
  int iVar3;
  short unaff_BX;
  short sVar4;
  ushort uVar5;
  int local_4;

  uVar2 = FUN_00560d00(param_2,1,&local_4);
  sVar4 = (short)uVar2;
  if ((sVar4 != 0) && (sVar4 == 1)) {
    *param_6 = *param_6 * 0.3;
  }
  if (((param_5 != '\0') && (unaff_BX < 5)) && (local_4 != -1)) {
    local_4 = *(int *)(DAT_006f1d6c + 0xc) - local_4;
    uVar5 = (local_4 < 0) - 1 & (ushort)local_4;
    sVar1 = __ftol();
    if ((short)uVar5 <= sVar1) {
      *param_6 = 0.0;
      return 0;
    }
    iVar3 = (int)(short)uVar5;
    if (iVar3 < sVar1 + 0x3c) {
      iVar3 = iVar3 - sVar1;
      *param_6 = (float)iVar3 * *param_6 * 0.016666668;
    }
    return CONCAT22((short)((uint)iVar3 >> 0x10),sVar4);
  }
  return uVar2;
}
#endif
