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
// VERIFIED against disassembly 0x45acd0..0x45ae9c (2026-09-30): the indirect calls are engine->dispose (+8, no
//   args: `call eax`) and the decal vertex buffer's vtable Release (+8, `push obj; call [ecx+8]`), both as written.
//   Fixed: terminal_initialized (0x6b2efc), game_state_write_buffer_allocated (0x6e2de8) and
//   game_state_persistent_storage_created (0x6e2df4) are byte stores; as uint32_t they zeroed the following bytes
//   (terminal_messages at 0x6b2f00 for the first).
// Note: most of the buffers here (network session state, hs dynamic globals, the widget
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
extern void *weather_particle_data;        // 0x0087abcc, TYPES-GAP (weather particles data_new pool)
extern uint32_t effect_data;         // 0x0087abdc, TYPES-GAP
extern uint32_t effect_location_data;// 0x0087abe0, TYPES-GAP
extern uint32_t particle_data;       // 0x0087abd0, TYPES-GAP
extern data_array *contrail_point_data;    // TYPES-GAP
extern data_array *contrail_data;    // TYPES-GAP
extern data_array *player_data;      // 0x0087a480
extern data_array *team_data;        // 0x0087a47c
extern player_globals *local_player_globals; // 0x0087a478
extern data_array *decal_data;    // TYPES-GAP

extern uint8_t *widget_memory_pool; // 0x006926c4 area, TYPES-GAP (widget module)
extern uint32_t ui_root_widget[13]; // TYPES-GAP, 13 dwords zeroed together
extern data_array *game_looping_sound_data; // 0x007461a0, TYPES-GAP
extern uint32_t sound_class_gains; // 0x00746140, TYPES-GAP
extern uint32_t rasterizer_device; // TYPES-GAP
extern void **rasterizer_decal_vertex_cache;   // TYPES-GAP, a vtable-dispatched object (dispose slot +8)
extern uint32_t object_render_state_cache; // 0x007c30ec, TYPES-GAP
extern uint8_t console_win32_attached;  // TYPES-GAP
extern uint32_t *terminal_messages; // TYPES-GAP, GlobalAlloc-ed, 0xe dwords, byte flag at +9
extern uint8_t terminal_initialized;  // 0x006b2efc, a BYTE (the draft declared it uint32_t and its 4-byte store clobbered the neighbours); // TYPES-GAP
extern void *game_state_write_buffer;   // 0x006e2de4, TYPES-GAP
extern uint32_t input_event_queue_active[0x43];   // TYPES-GAP
extern uint32_t input_globals[0x97c];  // TYPES-GAP
extern uint32_t profile_globals_block[0x1829]; // TYPES-GAP
extern uint8_t game_state_write_buffer_allocated;  // 0x006e2de8, a BYTE (the draft declared it uint32_t and its 4-byte store clobbered the neighbours); // TYPES-GAP
extern void *game_state_persistent_storage; // TYPES-GAP (Win32 handle)
extern uint8_t game_state_persistent_storage_created;  // 0x006e2df4, a BYTE (the draft declared it uint32_t and its 4-byte store clobbered the neighbours); // TYPES-GAP

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
    if (*(void **)(widget_memory_pool + 4) != (void *)0) {
        GlobalFree(*(void **)(widget_memory_pool + 4));
    }
    *(uint32_t *)(widget_memory_pool + 4) = 0;
    *(uint32_t *)(widget_memory_pool + 8) = 0;

    for (i = 0; i < 13; i = i + 1) {
        ui_root_widget[i] = 0;
    }
    game_looping_sound_data = (data_array *)0;
    sound_class_gains = 0;

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

    if (weather_particle_data != (void *)0) {
        cursor = (uint32_t *)weather_particle_data;
        for (i = 0; i < 14; i = i + 1) {
            cursor[i] = 0;
        }
        GlobalFree(weather_particle_data);
        weather_particle_data = (void *)0;
    }
    effect_data = 0;
    effect_location_data = 0;
    particle_data = 0;
    contrail_point_data = (data_array *)0;
    contrail_data = (data_array *)0;
    player_data = (data_array *)0;
    team_data = (data_array *)0;
    local_player_globals = (player_globals *)0;
    decal_data = (data_array *)0;

    if (rasterizer_device != 0 && rasterizer_decal_vertex_cache != (void **)0) {
        ((void (__stdcall *)(void **))(*(void ***)((uint8_t *)*rasterizer_decal_vertex_cache + 8)))(rasterizer_decal_vertex_cache); // TYPES-GAP vtable call
        rasterizer_decal_vertex_cache = (void **)0;
    }
    object_render_state_cache = 0;
    objects_dispose();

    if (console_win32_attached != 0) {
        console_win32_attached = 0;
    }
    if (terminal_messages != (uint32_t *)0) {
        if (*((uint8_t *)terminal_messages + 9 * 4) != 0) { // UNSURE: preserved as-is, see below
            *((uint8_t *)terminal_messages + 9 * 4) = 0;
        }
        for (i = 0; i < 14; i = i + 1) {
            terminal_messages[i] = 0;
        }
        GlobalFree(terminal_messages);
    }
    terminal_initialized = 0;
    saved_game_files_dispose();

    for (i = 0; i < 0x43; i = i + 1) {
        input_event_queue_active[i] = 0;
    }
    for (i = 0; i < 0x97c; i = i + 1) {
        input_globals[i] = 0;
    }
    for (i = 0; i < 0x1829; i = i + 1) {
        profile_globals_block[i] = 0;
    }
    GlobalFree(game_state_write_buffer);
    game_state_write_buffer_allocated = 0;
    CloseHandle(game_state_persistent_storage);
    game_state_persistent_storage_created = 0;

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
