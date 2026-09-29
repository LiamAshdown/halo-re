// actor_probe_step_direction  (Ghidra: actor_probe_step_direction, renamed)
// address 0x417e50, size 304 bytes
// name confidence: 0.3   rewrite confidence: 0.9 (VERIFIED against 0x417e50 (jump table 0x417f80, argument slots, flip loop); fixed the swapped random side in variant 4)
// evidence: dispatches *variant on a 2D base direction (ECX, a register argument Ghidra
// dropped along with the whole body of cases 0-3, which its own decompilation rendered as
// empty `break`s) to one of five 2D transforms -- left-perpendicular (0), right-perpendicular
// (1), unchanged (2), reversed (3), or a coin-flip between left/right-perpendicular (4, with
// two attempts, retrying the other perpendicular and its complement if the first is
// obstructed) -- and calls actor_check_step_obstruction with the transformed direction until
// one attempt succeeds or all have been tried; on success records which variant worked back
// through *variant, on total failure sets *variant to -1.
// register convention: reconstructed from objdump -d -M intel over 0x417e50..0x417f7f
// (Ghidra dropped the ECX register argument and the bodies of switch cases 0-3 entirely,
// showing only empty `break` statements -- this rewrite fills them in from the disassembly).
// actor_index, step_distance, variant, step_up, out_flag and extra_param are genuine stack
// parameters; direction is ECX.
// blam-cc: stack -> actor_index, stack -> step_distance, ECX -> direction, stack -> variant,
//   stack -> step_up, stack -> out_flag, stack -> extra_param

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"
#include "fn_ai.h"

extern uint32_t random_seed_global; // 0x00719cd0


// blam-cc: stack -> actor_index, stack -> step_distance, ECX -> direction, stack -> variant,
//   stack -> step_up, stack -> out_flag, stack -> extra_param
uint8_t actor_probe_step_direction(datum_index actor_index, float step_distance, real_vector2d *direction,
                                    uint16_t *variant, float step_up, uint8_t *out_flag, void *extra_param)
{
    uint16_t index;
    int16_t attempts;
    real_vector2d probe;
    int16_t tried;

    index = *variant;
    attempts = 1;

    switch (index) {
    case 0:
        probe.i = -direction->j;
        probe.j = direction->i;
        break;
    case 1:
        probe.i = direction->j;
        probe.j = -direction->i;
        break;
    case 2:
        probe.i = direction->i;
        probe.j = direction->j;
        break;
    case 3:
        probe.i = -direction->i;
        probe.j = -direction->j;
        break;
    case 4: {
        uint32_t rng;
        random_seed_global = random_seed_global * 0x19660d + 0x3c6ef35f;
        rng = (uint32_t)random_seed_global;
        // FIXED (0x417ed4 jbe 0x417ef3): a roll <= 0x8000 takes the right-hand side (index 1),
        //   a larger roll the left-hand side (index 0); the draft had them swapped.
        if ((uint16_t)(rng >> 0x10) <= 0x8000) {
            probe.i = direction->j;
            probe.j = -direction->i;
            index = 1;
        } else {
            probe.i = -direction->j;
            probe.j = direction->i;
            index = 0;
        }
        attempts = 2;
        break;
    }
    default:
        probe.i = 0.0f;
        probe.j = 0.0f;
        break;
    }

    for (tried = 0; tried < attempts; tried++) {
        if (actor_check_step_obstruction(actor_index, &probe, step_distance, step_up, out_flag, extra_param)) {
            *variant = index;
            return 1;
        }
        probe.i = -probe.i;
        probe.j = -probe.j;
        index ^= 1;
    }

    *variant = 0xffff;
    return 0;
}

#if 0
Original Ghidra decompilation (0x417e50):

undefined4
FUN_00417e50(undefined4 param_1,undefined4 param_2,ushort *param_3,undefined4 param_4,
            undefined4 param_5,undefined4 param_6)

{
  char cVar1;
  short sVar2;
  short sVar3;
  ushort uVar4;

  uVar4 = *param_3;
  sVar3 = 1;
  switch(uVar4) {
  case 0:
    break;
  case 1:
    break;
  case 2:
    break;
  case 3:
    break;
  case 4:
    random_seed_global = random_seed_global * 0x19660d + 0x3c6ef35f;
    uVar4 = (ushort)((ushort)((uint)random_seed_global >> 0x10) < 0x8001);
    sVar3 = 2;
  }
  sVar2 = 0;
  if (sVar3 != 0) {
    do {
      cVar1 = FUN_00417bb0(param_2,param_4,param_5,param_6);
      if (cVar1 != '\0') {
        *param_3 = uVar4;
        return 1;
      }
      sVar2 = sVar2 + 1;
      uVar4 = uVar4 ^ 1;
    } while (sVar2 < sVar3);
  }
  *param_3 = 0xffff;
  return 0;
}

Disassembly cross-check (objdump -d -M intel bin/halo.exe, 0x417e50..0x417f7f): the jump table at
0x417f80 (417e76 417e8a 417e9b 417eaa 417ebd) gives the five case bodies Ghidra omitted; ECX
holds the 2-float direction input throughout, confirmed never assigned inside this function.
#endif
