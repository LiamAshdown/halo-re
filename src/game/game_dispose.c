// game_dispose  (Ghidra: FUN_0045acd0; renamed per symbols/review_queue.txt)
// address 0x45acd0, size 460 bytes
// name confidence: 0.4   rewrite confidence: 0.45
// evidence: symbols/review_queue.txt 0x45acd0 "frees/zeroes the same set of global buffers set
//   up in game_initialize (0x87abcc/0x87a480/etc.), calls objects_dispose, and releases Win32
//   handles via GlobalFree"; types/game.h globals list (player_globals/player_control_globals
//   pointers at 0x0087a478/0x0087a47c/0x0087a480, player_profile_cache at 0x006b0b88,
//   player_profile_cache_initialized at 0x006f1d38, current_game_engine at 0x006f1d20). Mirrors
//   game_initialize (0x45a9c0) field-for-field, tearing the same allocations back down.
// register convention: no arguments.
//
// UNSURE: most of the buffers here (network session state, hs dynamic globals, the widget
// memory pool header, the save-game arena) belong to other modules and are not named in
// types/game.h; kept as raw global writes with TYPES-GAP markers rather than guessed names.

#include "win32.h"
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"

extern game_engine_definition *current_game_engine; // 0x006f1d20
extern uint8_t player_profile_cache_initialized;     // 0x006f1d38
extern player_profile player_profile_cache[16];         // 0x006b0b88 (16 entries, 0xc0 dwords)
extern void *ctf_globals_live_or_weather_pool;        // 0x0087abcc, TYPES-GAP (weather particles data_new pool)
extern uint32_t effect_pool;         // 0x0087abdc, TYPES-GAP
extern uint32_t effect_location_pool;// 0x0087abe0, TYPES-GAP
extern uint32_t particle_pool;       // 0x0087abd0, TYPES-GAP
extern data_array *unknown_0087abe8;    // TYPES-GAP
extern data_array *unknown_0087abec;    // TYPES-GAP
extern data_array *player_data;      // 0x0087a480
extern data_array *team_data;        // 0x0087a47c
extern player_globals *local_player_globals; // 0x0087a478
extern data_array *unknown_0087abe4;    // TYPES-GAP

extern uint8_t *widget_close_all_state; // 0x006926c4 area, TYPES-GAP (widget module)
extern uint32_t unknown_00718f94_block[13]; // TYPES-GAP, 13 dwords zeroed together
extern data_array *network_predicted_globals; // 0x007461a0, TYPES-GAP
extern uint32_t network_something_00746140; // 0x00746140, TYPES-GAP
extern uint32_t unknown_0071d174; // TYPES-GAP
extern void **unknown_0071d1bc;   // TYPES-GAP, a vtable-dispatched object (dispose slot +8)
extern uint32_t tag_cache_render_states; // 0x007c30ec, TYPES-GAP
extern uint8_t unknown_006b2f18;  // TYPES-GAP
extern uint32_t *unknown_006b2f00; // TYPES-GAP, GlobalAlloc-ed, 0xe dwords, byte flag at +9
extern uint32_t unknown_006b2efc; // TYPES-GAP
extern void *saved_games_arena;   // 0x006e2de4, TYPES-GAP
extern uint32_t unknown_00712cc0_block[0x43];   // TYPES-GAP
extern uint32_t unknown_00710328_block[0x97c];  // TYPES-GAP
extern uint32_t unknown_00712dd8_block[0x1829]; // TYPES-GAP
extern uint32_t unknown_006e2de8; // TYPES-GAP
extern void *unknown_006e2df8_handle; // TYPES-GAP (Win32 handle)
extern uint32_t unknown_006e2df4; // TYPES-GAP

extern void hs_dispose_dynamic_globals(void); // 0x48a130, hs module
extern void widget_close_all(void);           // 0x498650, widget module
extern void objects_dispose(void);            // 0x4f4db0, objects module
extern void saved_game_files_dispose(void);   // 0x53c480, saved_games module
extern void network_shutdown(void);           // 0x4416e0, network module

// Tears down the memory pools and Win32 resources allocated by game_initialize (0x45a9c0).
void game_dispose(void)
{
    uint32_t i;
    uint32_t *cursor;

    hs_dispose_dynamic_globals();
    widget_close_all();
    if (*(void **)(widget_close_all_state + 4) != (void *)0) {
        GlobalFree(*(void **)(widget_close_all_state + 4));
    }
    *(uint32_t *)(widget_close_all_state + 4) = 0;
    *(uint32_t *)(widget_close_all_state + 8) = 0;

    for (i = 0; i < 13; i = i + 1) {
        unknown_00718f94_block[i] = 0;
    }
    network_predicted_globals = (data_array *)0;
    network_something_00746140 = 0;

    if (current_game_engine != (game_engine_definition *)0) {
        if (current_game_engine->dispose != (void *)0) {
            ((void (*)(void))current_game_engine->dispose)();
        }
        current_game_engine = (game_engine_definition *)0;
    }

    if (player_profile_cache_initialized == 1) {
        cursor = (uint32_t *)player_profile_cache;
        for (i = 0; i < 0xc0; i = i + 1) {
            cursor[i] = 0;
        }
        player_profile_cache_initialized = 0;
    }

    if (ctf_globals_live_or_weather_pool != (void *)0) {
        cursor = (uint32_t *)ctf_globals_live_or_weather_pool;
        for (i = 0; i < 14; i = i + 1) {
            cursor[i] = 0;
        }
        GlobalFree(ctf_globals_live_or_weather_pool);
        ctf_globals_live_or_weather_pool = (void *)0;
    }
    effect_pool = 0;
    effect_location_pool = 0;
    particle_pool = 0;
    unknown_0087abe8 = (data_array *)0;
    unknown_0087abec = (data_array *)0;
    player_data = (data_array *)0;
    team_data = (data_array *)0;
    local_player_globals = (player_globals *)0;
    unknown_0087abe4 = (data_array *)0;

    if (unknown_0071d174 != 0 && unknown_0071d1bc != (void **)0) {
        ((void (__stdcall *)(void **))(*(void ***)((uint8_t *)*unknown_0071d1bc + 8)))(unknown_0071d1bc); // TYPES-GAP vtable call
        unknown_0071d1bc = (void **)0;
    }
    tag_cache_render_states = 0;
    objects_dispose();

    if (unknown_006b2f18 != 0) {
        unknown_006b2f18 = 0;
    }
    if (unknown_006b2f00 != (uint32_t *)0) {
        if (*((uint8_t *)unknown_006b2f00 + 9 * 4) != 0) { // UNSURE: preserved as-is, see below
            *((uint8_t *)unknown_006b2f00 + 9 * 4) = 0;
        }
        for (i = 0; i < 14; i = i + 1) {
            unknown_006b2f00[i] = 0;
        }
        GlobalFree(unknown_006b2f00);
    }
    unknown_006b2efc = 0;
    saved_game_files_dispose();

    for (i = 0; i < 0x43; i = i + 1) {
        unknown_00712cc0_block[i] = 0;
    }
    for (i = 0; i < 0x97c; i = i + 1) {
        unknown_00710328_block[i] = 0;
    }
    for (i = 0; i < 0x1829; i = i + 1) {
        unknown_00712dd8_block[i] = 0;
    }
    GlobalFree(saved_games_arena);
    unknown_006e2de8 = 0;
    CloseHandle(unknown_006e2df8_handle);
    unknown_006e2df4 = 0;

    network_shutdown();
}

#if 0
Original Ghidra decompilation (0x45acd0), from tools/pack.py 0x45acd0:

void FUN_0045acd0(void)

{
  undefined *puVar1;
  HGLOBAL hMem;
  int iVar2;
  undefined4 *puVar3;
  undefined4 *puVar4;

  hs_dispose_dynamic_globals();
  widget_close_all();
  if (*(HGLOBAL *)(PTR_PTR_006926c4 + 4) != (HGLOBAL)0x0) {
    GlobalFree(*(HGLOBAL *)(PTR_PTR_006926c4 + 4));
  }
  puVar1 = PTR_PTR_006926c4;
  *(undefined4 *)(PTR_PTR_006926c4 + 4) = 0;
  *(undefined4 *)(puVar1 + 8) = 0;
  puVar4 = &DAT_00718f94;
  for (iVar2 = 0xd; iVar2 != 0; iVar2 = iVar2 + -1) {
    *puVar4 = 0;
    puVar4 = puVar4 + 1;
  }
  if (DAT_007461a0 != 0) {
    DAT_007461a0 = 0;
  }
  DAT_00746140 = 0;
  if (DAT_006f1d20 != 0) {
    if (*(code **)(DAT_006f1d20 + 8) != (code *)0x0) {
      (**(code **)(DAT_006f1d20 + 8))();
    }
    DAT_006f1d20 = 0;
  }
  if (DAT_006f1d38 == '\x01') {
    puVar4 = &DAT_006b0b88;
    for (iVar2 = 0xc0; iVar2 != 0; iVar2 = iVar2 + -1) {
      *puVar4 = 0;
      puVar4 = puVar4 + 1;
    }
    DAT_006f1d38 = '\0';
  }
  puVar4 = DAT_0087abcc;
  if (DAT_0087abcc != (undefined4 *)0x0) {
    puVar3 = DAT_0087abcc;
    for (iVar2 = 0xe; iVar2 != 0; iVar2 = iVar2 + -1) {
      *puVar3 = 0;
      puVar3 = puVar3 + 1;
    }
    GlobalFree(puVar4);
    DAT_0087abcc = (undefined4 *)0x0;
  }
  if (DAT_0087abdc != 0) {
    DAT_0087abdc = 0;
  }
  if (DAT_0087abe0 != 0) {
    DAT_0087abe0 = 0;
  }
  if (DAT_0087abd0 != 0) {
    DAT_0087abd0 = 0;
  }
  if (DAT_0087abe8 != 0) {
    DAT_0087abe8 = 0;
  }
  if (DAT_0087abec != 0) {
    DAT_0087abec = 0;
  }
  if (DAT_0087a480 != 0) {
    DAT_0087a480 = 0;
  }
  if (DAT_0087a47c != 0) {
    DAT_0087a47c = 0;
  }
  if (DAT_0087a478 != 0) {
    DAT_0087a478 = 0;
  }
  DAT_0087abe4 = 0;
  if ((DAT_0071d174 != 0) && (DAT_0071d1bc != (int *)0x0)) {
    (**(code **)(*DAT_0071d1bc + 8))(DAT_0071d1bc);
    DAT_0071d1bc = (int *)0x0;
  }
  DAT_007c30ec = 0;
  objects_dispose();
  puVar4 = DAT_006b2f00;
  if (DAT_006b2f18 != '\0') {
    DAT_006b2f18 = '\0';
  }
  if (DAT_006b2f00 != (undefined4 *)0x0) {
    if (*(char *)(DAT_006b2f00 + 9) != '\0') {
      *(undefined1 *)(DAT_006b2f00 + 9) = 0;
    }
    puVar3 = puVar4;
    for (iVar2 = 0xe; iVar2 != 0; iVar2 = iVar2 + -1) {
      *puVar3 = 0;
      puVar3 = puVar3 + 1;
    }
    GlobalFree(puVar4);
  }
  DAT_006b2efc = 0;
  saved_game_files_dispose();
  hMem = DAT_006e2de4;
  puVar4 = (undefined4 *)&DAT_00712cc0;
  for (iVar2 = 0x43; iVar2 != 0; iVar2 = iVar2 + -1) {
    *puVar4 = 0;
    puVar4 = puVar4 + 1;
  }
  puVar4 = &DAT_00710328;
  for (iVar2 = 0x97c; iVar2 != 0; iVar2 = iVar2 + -1) {
    *puVar4 = 0;
    puVar4 = puVar4 + 1;
  }
  puVar4 = &DAT_00712dd8;
  for (iVar2 = 0x1829; iVar2 != 0; iVar2 = iVar2 + -1) {
    *puVar4 = 0;
    puVar4 = puVar4 + 1;
  }
  GlobalFree(hMem);
  DAT_006e2de8 = 0;
  CloseHandle(DAT_006e2df8);
  DAT_006e2df4 = 0;
  network_shutdown();
  return;
}
#endif
