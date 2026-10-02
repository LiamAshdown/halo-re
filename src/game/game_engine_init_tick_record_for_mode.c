// game_engine_init_tick_record_for_mode  (Ghidra: FUN_00470ae0; renamed, no established name)
// address 0x470ae0, size 64 bytes
// name confidence: 0.35   rewrite confidence: 0.6
// evidence: out/phase4/game_functions.md ("Initializes the current tick record's time-scale and
// accumulator fields and performs game-mode-specific setup"); types/game.h game_time_globals
// (speed +0x18, leftover_time +0x1c, active +0x01); network_game_mode (0 local, 1 client,
// 2 host, 3 replay); update_server_dispose.c / update_client_dispose.c (this batch), called here
// by mode.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern game_time_globals *game_time; // 0x006f1d6c
extern int16_t network_game_mode;     // 0x00719720
extern uint8_t game_time_unknown_49;  // 0x006f1d49
extern int32_t game_time_unknown_48;  // 0x006f1d48

extern void update_server_dispose(void); // this batch, 0x472b70
extern void update_client_dispose(void); // this batch, 0x472fa0

// Sets the current tick record's speed to 1.0 and clears its leftover_time accumulator, marks it
// active, resets the two game_time bookkeeping globals, then re-synchronizes the server or client
// update queue depending on the connection role (local/host use the server path, client/replay
// use the client path).
void game_engine_init_tick_record_for_mode(void)
{
    game_time->speed = 1.0f;
    game_time->leftover_time = 0.0f;
    game_time->active = 1;
    game_time_unknown_49 = 1;
    game_time_unknown_48 = 0;

    switch (network_game_mode) {
    case 0:
    case 2:
        update_server_dispose();
        return;
    case 1:
    case 3:
        update_client_dispose();
        return;
    default:
        return;
    }
}

#if 0
Original Ghidra decompilation (0x470ae0), from tools/pack.py 0x470ae0:

void FUN_00470ae0(void)

{
  int iVar1;

  iVar1 = DAT_006f1d6c;
  *(undefined4 *)(DAT_006f1d6c + 0x18) = 0x3f800000;
  *(undefined4 *)(iVar1 + 0x1c) = 0;
  *(undefined1 *)(iVar1 + 1) = 1;
  DAT_006f1d49 = 1;
  DAT_006f1d48 = 0;
  switch(DAT_00719720) {
  case 0:
  case 2:
    FUN_00472b70();
    return;
  case 1:
  case 3:
    update_client_dispose();
    return;
  default:
    return;
  }
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
