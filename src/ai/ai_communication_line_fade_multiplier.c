// ai_communication_line_fade_multiplier  (Ghidra: ai_communication_line_fade_multiplier; named for this rewrite)
// address 0x42f8c0, size 220 bytes
// name confidence: 0.3   rewrite confidence: 0.9
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
extern float ai_communication_class_repeat_delay[]; // 0x00655930, stride 0x28 (10 floats) per class

extern int32_t unit_animation_change_priority_check(uint32_t unit_index, uint8_t follow_fallback, int16_t requested_priority,
    uint8_t allow_repeat, uint32_t *out_unknown_3f0, int16_t *dialogue_index, int32_t *chain_value); // 0x560d00, EAX, DL, stack

// REWRITTEN from objdump 0x42f8c0..0x42f99b. Stack: (unit, priority, extra delay ticks, follow_fallback, apply_fade,
//   volume *); EAX: the chain value (in/out); ECX: the dialogue index (in/out); BX: the line class. Asks the unit's
//   speech priority (0x560d00, allow_repeat 1; the tick it last spoke comes back in the slot that held ECX) and scales
//   *volume by 0.3 when it answers 1. With apply_fade, a class under 5 and a known tick: inside the class repeat delay
//   (0x655930[class * 10] * 30 + extra) the volume becomes 0 and 0 is returned; over the next 60 ticks it fades in.
//   The draft called the priority check with 3 guessed arguments and had no chain / index / class inputs.
int16_t ai_communication_line_fade_multiplier(uint32_t unit_index, int16_t priority, int16_t extra_delay,
    uint8_t follow_fallback, uint8_t apply_fade, float *volume, int32_t *chain_value, int16_t *dialogue_index,
    int16_t line_class)
{
    uint32_t last_spoke = (uint32_t)dialogue_index;          // [esp+0x8]: the pushed ECX, reused as the out slot
    int16_t status;

    status = (int16_t)unit_animation_change_priority_check(unit_index, follow_fallback, priority, 1, &last_spoke,
        dialogue_index, chain_value);
    if (status == 1) {
        *volume = *volume * 0.3f;
    }
    if (apply_fade && line_class < 5 && last_spoke != 0xffffffff) {
        int32_t elapsed = game_time->game_time - (int32_t)last_spoke;
        int16_t limit;

        if (elapsed < 0) {
            elapsed = 0;
        }
        limit = (int16_t)(int32_t)(ai_communication_class_repeat_delay[line_class * 10] * 30.0f + (float)(int32_t)extra_delay);
        if ((int16_t)elapsed <= limit) {
            *volume = 0.0f;
            return 0;
        }
        if ((int32_t)(int16_t)elapsed < (int32_t)limit + 0x3c) {
            *volume = (float)((int32_t)(int16_t)elapsed - (int32_t)limit) * *volume * (1.0f / 60.0f);
        }
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
