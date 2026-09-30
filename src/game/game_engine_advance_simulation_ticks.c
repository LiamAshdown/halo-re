// game_engine_advance_simulation_ticks  (Ghidra: game_engine_advance_simulation_ticks, already
// named)
// address 0x470bf0, size 223 bytes, cc=__cdecl
// name confidence: 0.5   rewrite confidence: 0.45
// evidence: out/phase4/game_functions.md ("Advances the game simulation by the appropriate
// number of fixed 30Hz ticks for this frame, running per-tick hooks and updating game effects");
// types/game.h game_time_globals (active +0x01, game_time +0x0c, ticks_this_frame +0x10,
// elapsed_ticks +0x14, speed +0x18); game_time_force_single_tick (0x007196d8);
// game_engine_accumulate_simulation_ticks.c (this batch, 0x470b30); src/game/game_simulate_tick.c
// (0x45b780, already named, `void game_simulate_tick(uint32_t predict_pass)`).
// FIXED (verified against 0x470c13..0x470c45): update_run_catchup_ticks takes the tick count in BX, and
//   network_game_server_per_frame_tick the tick count in CX with network_server in ESI; both were called
//   without arguments.
// UNSURE: network_game_server_per_frame_tick.c models an EAX `entry` argument that 0x4e03c0 overwrites
//   at its first instruction; host path only, not reached on the first-boot track.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "objects.h"
#include "units.h"
#include "networking.h"
#include "fn_game.h"

extern int32_t game_time_force_single_tick; // 0x007196d8
extern int16_t network_game_mode;            // 0x00719720
extern game_time_globals *game_time;          // 0x006f1d6c


extern network_server_globals *network_server;
extern void network_game_server_per_frame_tick(void *entry, int16_t update_count, uint8_t *server);
    // 0x4e03c0, blam-cc: EAX -> entry, CX -> update_count, ESI -> server
extern void game_effects_update(float delta_time); // 0x45b4f0


// Computes how many fixed ticks this frame should run (forcing exactly 1 when
// game_time_force_single_tick is set), runs any host/client catch-up hook for the connection
// role, simulates that many ticks (game_simulate_tick, counting each one into game_time), then
// updates game effects scaled by the resulting time-scaled delta (or by the raw delta when
// networked).
void game_engine_advance_simulation_ticks(float delta_time)
{
    int32_t tick_count = game_engine_accumulate_simulation_ticks(delta_time, 0);
    int32_t i;

    if (game_time_force_single_tick != 0) {
        tick_count = 1;
    }

    if (network_game_mode == 0) {
        if (!game_time->active) {
            game_time->ticks_this_frame = 0;
            return;
        }
        update_run_catchup_ticks((int16_t)tick_count); // 0x470c45: BX = the tick count
    } else if (network_game_mode == 2) {
        // 0x470c24..0x470c2c: ESI = network_server, CX = the tick count; EAX is 0 here (mode - 2) and
        // 0x4e03c0 overwrites it first thing (movzx eax,[esi+4])
        network_game_server_per_frame_tick(0, (int16_t)tick_count, (uint8_t *)network_server);
    }

    for (i = tick_count; i > 0; i--) {
        game_simulate_tick((uint32_t)(i - 1));
        game_time->elapsed_ticks = game_time->elapsed_ticks + 1;
        game_time->game_time = game_time->game_time + 1;
    }
    game_time->ticks_this_frame = (int16_t)tick_count;

    if (network_game_mode != 1 && network_game_mode != 2) {
        game_effects_update(game_time->speed * delta_time);
    } else {
        game_effects_update(delta_time * 1.0f);
    }
}

#if 0
Original Ghidra decompilation (0x470bf0), from tools/pack.py 0x470bf0:

void __cdecl game_engine_advance_simulation_ticks(float delta_time)

{
  int *piVar1;
  int iVar2;
  int iVar3;
  int iVar4;
  int iVar5;

  iVar3 = FUN_00470b30(delta_time,0);
  if (DAT_007196d8 != 0) {
    iVar3 = 1;
  }
  if (DAT_00719720 == 0) {
    if (*(char *)(DAT_006f1d6c + 1) == '\0') {
      *(undefined2 *)(DAT_006f1d6c + 0x10) = 0;
      return;
    }
    FUN_00473310();
  }
  else if (DAT_00719720 == 2) {
    FUN_004e03c0();
  }
  iVar4 = DAT_006f1d6c;
  iVar5 = iVar3;
  iVar2 = iVar3;
  if (0 < iVar3) {
    do {
      FUN_0045b780(iVar2 + -1);
      iVar4 = DAT_006f1d6c;
      piVar1 = (int *)(DAT_006f1d6c + 0xc);
      iVar5 = iVar5 + -1;
      *(int *)(DAT_006f1d6c + 0x14) = *(int *)(DAT_006f1d6c + 0x14) + 1;
      *(int *)(iVar4 + 0xc) = *piVar1 + 1;
      iVar2 = iVar2 + -1;
    } while (iVar5 != 0);
  }
  *(short *)(iVar4 + 0x10) = (short)iVar3;
  if ((DAT_00719720 != 1) && (DAT_00719720 != 2)) {
    game_effects_update(*(float *)(iVar4 + 0x18) * delta_time);
    return;
  }
  game_effects_update(delta_time * 1.0);
  return;
}
#endif
