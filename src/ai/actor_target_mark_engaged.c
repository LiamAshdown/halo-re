// actor_target_mark_engaged  (Ghidra: actor_target_mark_engaged; named from out/phase2/results/ai_02.json)
// address 0x41fa80, size 120 bytes
// name confidence: 0.45   rewrite confidence: 0.85 (VERIFIED 2026-09-28 against objdump 0x41fa80..0x41faf7 (EAX prop, EBX actor, stack flag).)
// evidence: out/phase2/results/ai_02.json -- sets or clears the engagement timestamp fields
//   at target-data+0x9c/+0xa0 (using the game-time global) based on param_1, then refreshes
//   derived fields via the functions now named actor_target_update_active_flag (0x41fc60) and
//   actor_rate_potential_target (0x41fd50), storing the latter into +0x50. Matches
//   prop.engaged_age/engaged_tick/engaged/desirability in types/ai.h.
// register convention: EAX -> target_prop_index; param_1 (char) is Ghidra's recognized stack
//   parameter, the mark/clear flag.
//   // blam-cc: EAX -> target_prop_index, EBX -> actor_index, stack -> mark_engaged
// FIXED (register inputs, objdump): EBX carries actor_index (read at 0x41fadb, `mov eax,ebx`,
//   passed on as EAX to actor_target_update_active_flag and then pushed as the first stack arg
//   to actor_rate_potential_target at 0x41fae2..0x41fae3); the UNSURE note already suspected
//   this but the rewrite still called both helpers with no arguments.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "ai.h"

extern data_array *prop_data;      // 0x008802c0
extern game_time_globals *game_time; // 0x006f1d6c

extern uint8_t actor_target_update_active_flag(datum_index actor_index, datum_index target_prop_index); // 0x41fc60, blam-cc: EAX -> actor_index, EDI -> target_prop_index
extern float actor_rate_potential_target(datum_index actor_index, datum_index target_prop_index);        // 0x41fd50

// blam-cc: EAX -> target_prop_index, EBX -> actor_index, stack -> mark_engaged
// Marks (or clears) the given prop (target-data record) as actively engaged and refreshes its
// derived combat timing fields.
void actor_target_mark_engaged(datum_index target_prop_index, datum_index actor_index, uint8_t mark_engaged)
{
    prop *target;

    target = (prop *)((uint8_t *)prop_data->data + (target_prop_index & 0xffff) * sizeof(prop));

    if (mark_engaged == 0) {
        target->engaged_age = 0;
        target->engaged_tick = -1;
    } else {
        if (target->engaged_age == 0) {
            target->engaged_age = 1;
        }
        target->engaged_tick = game_time->game_time;
    }

    target->engaged = actor_target_update_active_flag(actor_index, target_prop_index);
    target->desirability = actor_rate_potential_target(actor_index, target_prop_index);
}

#if 0
Original Ghidra decompilation (0x41fa80):

void FUN_0041fa80(char param_1)

{
  undefined1 uVar1;
  uint in_EAX;
  int iVar2;
  float10 fVar3;

  iVar2 = (in_EAX & 0xffff) * 0x138 + *(int *)(DAT_008802c0 + 0x34);
  if (param_1 == '\0') {
    *(undefined2 *)(iVar2 + 0x9c) = 0;
    *(undefined4 *)(iVar2 + 0xa0) = 0xffffffff;
  }
  else {
    if (*(short *)(iVar2 + 0x9c) == 0) {
      *(undefined2 *)(iVar2 + 0x9c) = 1;
    }
    *(undefined4 *)(iVar2 + 0xa0) = *(undefined4 *)(DAT_006f1d6c + 0xc);
  }
  uVar1 = FUN_0041fc60();
  *(undefined1 *)(iVar2 + 0xa4) = uVar1;
  fVar3 = (float10)actor_rate_potential_target();
  *(float *)(iVar2 + 0x50) = (float)fVar3;
  return;
}
#endif
