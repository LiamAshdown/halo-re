// actor_gate_jump_traversal  (Ghidra: actor_gate_jump_traversal, renamed)
// address 0x40a700, size 235 bytes
// name confidence: 0.4   rewrite confidence: 0.35
// evidence: types/ai.h actor.unknown_308/unknown_30c/order_committed/mode/unknown_398/
//   unit_index; phase-4 summary "gates whether the actor may perform a jump/climb-style
//   traversal action, applying a per-actor cooldown and a scripted animation trigger".
// register convention: actor index in EAX, a height/distance threshold and two flags/values
//   as the recognized stack parameters.
//   // blam-cc: EAX -> actor_index, stack -> threshold/allow_broadcast/broadcast_threshold
// UNSURE: actor+0xa8 falls inside actor.mode_data (a per-mode union); read/written here as
//   an int16 "climb ready" counter while actor.mode==4.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"
#include "game.h"

extern data_array *actor_data;       // 0x00880360
extern game_time_globals *game_time; // 0x006f1d6c

extern void ai_communication_broadcast(int32_t event_code, datum_index unit_index,
                                       datum_index object_a, int32_t param_d,
                                       datum_index object_b, datum_index object_c,
                                       uint32_t param_g);
// 0x42d340, not yet rewritten (this module). Always seven stack arguments: every call
// site in the binary cleans up 0x1c bytes, so the shorter forms Ghidra recovers at some
// sites are artefacts, not a reduced-arity overload.
extern uint8_t actor_check_pain_reaction(datum_index actor_index); // 0x40de20, this session (later)

uint8_t actor_gate_jump_traversal(uint32_t actor_index, int16_t threshold, char allow_broadcast, int16_t broadcast_threshold)
{
    actor *a = &((actor *)actor_data->data)[actor_index & 0xffff];
    uint8_t result = 0;

    if (threshold > a->unknown_308 || a->order_committed != 0) {
        a->unknown_308 = 0;
        return result;
    }

    if (a->mode == 4) {
        int16_t climb = *(int16_t *)(a->mode_data + (0xa8 - 0x9c));
        if (climb > 0) {
            if (climb <= a->unknown_308) {
                climb = a->unknown_308;
            }
            *(int16_t *)(a->mode_data + (0xa8 - 0x9c)) = climb;
            a->unknown_308 = 0;
            return 0;
        }
    }

    if (a->unknown_398 == -1 || game_time->game_time > a->unknown_398 + 7) {
        if (allow_broadcast != 0 && a->unknown_308 < broadcast_threshold) {
            ai_communication_broadcast(0x22, a->unit_index, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0);
            a->unknown_308 = 0;
            return 0;
        }
        result = actor_check_pain_reaction(a->unknown_30c);
    }

    a->unknown_308 = 0;
    return result;
}

#if 0
Original Ghidra decompilation (0x40a700):

undefined1 FUN_0040a700(short param_1,char param_2,short param_3)

{
  short sVar1;
  short sVar2;
  undefined1 uVar3;
  uint in_EAX;
  int iVar4;

  iVar4 = (in_EAX & 0xffff) * 0x724 + *(int *)(DAT_00880360 + 0x34);
  sVar1 = *(short *)(iVar4 + 0x308);
  uVar3 = 0;
  if ((param_1 <= sVar1) && (*(char *)(iVar4 + 0x160) == '\0')) {
    if ((*(short *)(iVar4 + 0x6c) == 4) && (sVar2 = *(short *)(iVar4 + 0xa8), 0 < sVar2)) {
      if (sVar2 <= sVar1) {
        sVar2 = sVar1;
      }
      *(short *)(iVar4 + 0xa8) = sVar2;
      *(undefined2 *)(iVar4 + 0x308) = 0;
      return 0;
    }
    if (*(int *)(iVar4 + 0x398) != -1) {
      if (*(int *)(DAT_006f1d6c + 0xc) <= *(int *)(iVar4 + 0x398) + 7) goto LAB_0040a7dc;
    }
    if ((param_2 != '\0') && (sVar1 < param_3)) {
      ai_communication_broadcast
                (0x22,*(undefined4 *)(iVar4 + 0x18),0xffffffff,0xffffffff,0xffffffff,0xffffffff,0);
      *(undefined2 *)(iVar4 + 0x308) = 0;
      return 0;
    }
    uVar3 = FUN_0040de20(*(undefined4 *)(iVar4 + 0x30c));
  }
LAB_0040a7dc:
  *(undefined2 *)(iVar4 + 0x308) = 0;
  return uVar3;
}
#endif
