// game_start_new_map  (Ghidra: FUN_0045b050; renamed per symbols/review_queue.txt)
// address 0x45b050, size 797 bytes
// name confidence: 0.4   rewrite confidence: 0.35
// evidence: symbols/review_queue.txt 0x45b050 "resets/re-zeroes numerous per-map globals, calls
//   objects_reset and data_delete_all for each effect/particle/weather pool, hs_scripts_reload,
//   update_server_new, ..."; types/game.h game_time_globals (0x006f1d6c), current_game_engine
//   (0x006f1d20), player_profile_cache (0x006b0b88), global_scenario (0x00746f8c). Counterpart
//   to game_stop_current_map (0x45b370, this batch), which mirrors most of these resets.
// register convention: no arguments.
//
// UNSURE: the great majority of the data_array pointers here (0x0087abXX and friends) are
// object/effect/particle/sound pools this module reads but does not own; each is written
// through raw TYPES-GAP externs at the same "+0x24 = 1 (mark valid), then data_delete_all"
// pattern the header comments on data_array::valid describe.
// reconciled: R07 0x00746f94 tag_cache_render_states_* (TYPES-GAP) -> scenario.h scenario_game_globals *global_scenario_game_globals (0x7c-byte scenario game-state block)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "game.h"
#include "scenario.h"

extern uint8_t *cache_file_slot_table; // 0x006b0b80, TYPES-GAP (dword+0x10 seeds the RNG)
extern random_seed random_seed_global; // 0x00719cd0
extern game_engine_definition *current_game_engine; // 0x006f1d20
extern uint8_t player_profile_cache_initialized;     // 0x006f1d38
extern uint32_t player_profile_cache_block[0xc0];    // 0x006b0b88, TYPES-GAP
extern game_variant game_engine_active_variant;             // 0x0087ab20 (NOT 0x006f1c88, which is the live copy)
extern game_time_globals *game_time;                 // 0x006f1d6c
extern uint32_t *unknown_00746280_block;             // TYPES-GAP, 0x343 dwords + a trailing byte
extern scenario_game_globals *global_scenario_game_globals; // 0x00746f94, scenario.h
extern uint32_t unknown_0065e508_block[0x12];        // TYPES-GAP, copied into the block above
extern data_array *object_render_state_cache;              // 0x007c30ec, TYPES-GAP (data_array*)
extern uint32_t *unknown_0072277c_block;              // TYPES-GAP, 0x290c dwords + trailing byte
extern void *unknown_0072278c;                         // TYPES-GAP
extern void *custom_waypoints_or_similar_006b0ad8;      // 0x006b0ad8, TYPES-GAP (0xa00+3 dwords)
extern data_array *unknown_0087abe4;                          // TYPES-GAP (a data_array*)
extern data_array *unknown_0087abec;      // TYPES-GAP
extern data_array *unknown_0087abe8;      // TYPES-GAP
extern data_array *particle_pool_ptr;     // 0x0087abd0, TYPES-GAP
extern data_array *effect_object_pool_ptr;// 0x0087abdc, TYPES-GAP
extern data_array *effect_location_pool_ptr; // 0x0087abe0, TYPES-GAP
extern void *particle_system_pool_ptr; // 0x0087abd4, TYPES-GAP
extern data_array *particle_system_particle_pool_ptr; // 0x0087abd8, TYPES-GAP
extern uint8_t unknown_007252b6; // TYPES-GAP
extern void *unknown_007252c0;   // TYPES-GAP
extern void *unknown_00724a50;   // TYPES-GAP
extern void *sound_something_00746140; // 0x00746140, TYPES-GAP (0x33 records, stride 6 shorts)
extern data_array *network_predicted_globals; // 0x007461a0, TYPES-GAP
extern uint32_t *unknown_007461a4;      // TYPES-GAP
extern int32_t unknown_006b0ae4; // TYPES-GAP
extern int32_t unknown_006b0ae0; // TYPES-GAP
extern data_array *weather_particle_pool_ptr; // 0x0087abcc, TYPES-GAP
extern real unknown_0069c534; // TYPES-GAP
extern real unknown_006b8d80; // TYPES-GAP
extern real unknown_0069c530; // TYPES-GAP
extern real unknown_006b8d7c; // TYPES-GAP
extern uint32_t unknown_006b1458; // TYPES-GAP
extern uint32_t *ai_something_006f1884; // 0x006f1884, TYPES-GAP
extern void *recorded_animations_pool_ptr; // 0x006b0a10, TYPES-GAP
extern uint32_t unknown_00686b60; // TYPES-GAP
extern uint32_t *saved_games_something_006f187c; // 0x006f187c, TYPES-GAP (7 dwords)
extern Scenario *global_scenario; // 0x00746f8c
extern uint8_t *unknown_006b8cbc; // TYPES-GAP

extern void ai_reset_for_new_map(void);                 // 0x42a840
extern void FUN_00435d50(void);                          // UNSURE module
extern void camera_initialize(void);                       // 0x445580
extern void FUN_00447740(void);                              // UNSURE module
extern void team_pair_table_init_defaults(void);               // this batch, 0x45bc80
extern void game_engine_load_from_variant(const game_variant *variant); // 0x45c2c0,
    // blam-cc: EBX -> variant (matches src/game/game_engine_load_from_variant.c)                // this batch, 0x45c2c0
extern void game_engine_initialize_for_new_game(void);                              // this batch, 0x45c370
extern void game_engine_reset_player_look_state(void);              // 0x470de0
extern uint8_t update_server_new(void);                             // 0x472aa0
extern void players_dispose(void);                                   // 0x473670
extern void hs_scripts_reload(void);                                  // 0x483250
extern void FUN_00494390(void);                                        // UNSURE module
extern void data_delete_all(data_array *array); // blam-cc: ESI -> array (src/memory/data_delete_all.c)                                // 0x4d0580, memory module, UNSURE arg
extern void objects_update_control_bindings(Scenario *scenario); // 0x4f3ba0. One plain stack
    // argument -- objdump shows 0x4f3ba0 reading [esp+0x24] twice and taking no register input,
    // so src/objects/objects_update_control_bindings.c's "blam-cc: EAX -> param_1" is wrong.          // 0x4f3ba0, UNSURE arg
extern void objects_reset(void);                                           // 0x4f4bb0
extern void breakable_surfaces_reset(void); // 0x4ffd40, objects (breakable_surface_globals reset, R79)
extern void FUN_00515740(void);                                               // UNSURE module
extern void game_state_build_header(void);                                     // 0x538000
extern void FUN_0053fa70(void);                                                  // UNSURE module
extern void __control87(uint32_t new_word, uint32_t mask); // MSVC CRT

// Resets game state (objects, scripts, particle/effect pools, network server) to begin a new
// game on the currently loaded map.
void game_start_new_map(void)
{
    uint32_t i;
    uint8_t *tag_cache_bytes;
    uint32_t *cursor;
    uint32_t *dst;
    uint16_t *record;

    random_seed_global = *(random_seed *)(cache_file_slot_table + 0x10);

    if (current_game_engine != (game_engine_definition *)0) {
        if (current_game_engine->dispose != (void *)0) {
            ((void (*)(void))current_game_engine->dispose)();
        }
        current_game_engine = (game_engine_definition *)0;
    }
    if (player_profile_cache_initialized == 1) {
        for (i = 0; i < 0xc0; i = i + 1) {
            player_profile_cache_block[i] = 0;
        }
        player_profile_cache_initialized = 0;
    }

    game_engine_load_from_variant(&game_engine_active_variant); // objdump 0x45b09a: EBX = 0x0087ab20
    __control87(0x9001f, 0xfffff);
    FUN_00515740();
    game_state_build_header();

    {
        uint32_t *game_time_dwords = (uint32_t *)game_time;
        for (i = 0; i < 8; i = i + 1) {
            game_time_dwords[i] = 0;
        }
    }
    ((uint8_t *)game_time)[0] = 1;

    FUN_00494390();
    team_pair_table_init_defaults();
    players_dispose();

    cursor = unknown_00746280_block;
    for (i = 0x343; i != 0; i = i - 1) {
        *cursor = 0;
        cursor = cursor + 1;
    }
    *(uint8_t *)unknown_00746280_block = 1;
    FUN_0053fa70();

    tag_cache_bytes = (uint8_t *)global_scenario_game_globals;
    cursor = (uint32_t *)tag_cache_bytes;
    for (i = 1; i <= 0xb; i = i + 1) {
        cursor[i] = 0; // dword 0 is deliberately left untouched, matching the original's loop
    }
    dst = (uint32_t *)tag_cache_bytes + 0xd;
    for (i = 0x12; i != 0; i = i - 1) {
        *dst = unknown_0065e508_block[0x12 - i];
        dst = dst + 1;
    }
    *((uint8_t *)((uint32_t *)tag_cache_bytes + 0xc)) = 0;

    objects_reset();
    object_render_state_cache->valid = 1;
    data_delete_all(object_render_state_cache);

    dst = unknown_0072277c_block;
    for (i = 0x290c; i != 0; i = i - 1) {
        *dst = 0;
        dst = dst + 1;
    }
    *((uint8_t *)unknown_0072277c_block + 0x520e) = 0;
    *(uint32_t *)unknown_0072278c = 0;
    breakable_surfaces_reset();

    dst = (uint32_t *)custom_waypoints_or_similar_006b0ad8;
    for (i = 0xa00; i != 0; i = i - 1) {
        *dst = 0xffffffff;
        dst = dst + 1;
    }
    dst[0] = 0xffffffff;
    dst[1] = 0;
    dst[2] = 0;
    unknown_0087abe4->valid = 1;
    data_delete_all(unknown_0087abe4);

    camera_initialize();
    FUN_00447740();

    unknown_0087abec->valid = 1;
    data_delete_all(unknown_0087abec);
    unknown_0087abe8->valid = 1;
    data_delete_all(unknown_0087abe8);
    particle_pool_ptr->valid = 1;
    data_delete_all(particle_pool_ptr);
    effect_object_pool_ptr->valid = 1;
    data_delete_all(effect_object_pool_ptr);
    effect_location_pool_ptr->valid = 1;
    data_delete_all(effect_location_pool_ptr);
    *((uint8_t *)particle_system_pool_ptr + 0x24) = 1;
    data_delete_all(particle_system_pool_ptr);
    particle_system_particle_pool_ptr->valid = 1;
    data_delete_all(particle_system_particle_pool_ptr);

    if (unknown_007252b6 == 0) {
        *((uint8_t *)unknown_007252c0 + 0x24) = 1;
        data_delete_all(unknown_007252c0);
        *((uint8_t *)unknown_00724a50 + 0x24) = 1;
        data_delete_all(unknown_00724a50);
    }

    record = (uint16_t *)((uint8_t *)sound_something_00746140 + 8);
    i = 0x33;
    do {
        *(uint32_t *)(record - 4) = 0x3f800000;
        *(uint32_t *)(record - 8) = 0x3f800000;
        *record = 0;
        record = record + 12;
        i = i - 1;
    } while (i != 0);

    if (network_predicted_globals != (data_array *)0) {
        network_predicted_globals->valid = 1;
        data_delete_all(network_predicted_globals);
        unknown_007461a4[1] = 0xffffffff;
        unknown_007461a4[0] = 0;
        unknown_007461a4[2] = 0;
    }

    unknown_006b0ae4 = -1;
    unknown_006b0ae0 = 0;
    weather_particle_pool_ptr->valid = 1;
    data_delete_all(weather_particle_pool_ptr);

    unknown_006b8d80 = unknown_0069c534 * 118613.34f;
    unknown_006b8d7c = unknown_0069c530 * 118613.34f;

    game_engine_initialize_for_new_game();
    unknown_006b1458 = 1;
    update_server_new();
    game_engine_reset_player_look_state();

    dst = ai_something_006f1884;
    for (i = 0x4a; i != 0; i = i - 1) {
        *dst = 0;
        dst = dst + 1;
    }
    *(uint16_t *)((uint8_t *)ai_something_006f1884 + 0x3f * 4) = 0xffff;
    ai_something_006f1884[0x49] = ((uint32_t *)game_time)[3];
    ai_reset_for_new_map();

    dst = saved_games_something_006f187c;
    dst[0] = 0;
    dst[1] = 0;
    dst[2] = 0;
    dst[3] = 0xffffffff;
    dst[4] = 0xffffffff;
    dst[5] = 0xffffffff;
    dst[6] = 0xffffffff;

    unknown_00686b60 = 0xbf800000;
    hs_scripts_reload();
    *((uint8_t *)recorded_animations_pool_ptr + 0x24) = 1;
    data_delete_all(recorded_animations_pool_ptr);

    *(cache_file_slot_table + 1) = 1;
    *unknown_006b8cbc = 1;
    objects_update_control_bindings(global_scenario);
    *unknown_006b8cbc = 0;
    FUN_00435d50();
}

#if 0
Original Ghidra decompilation (0x45b050), from tools/pack.py 0x45b050:

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void FUN_0045b050(void)

{
  undefined1 *puVar1;
  undefined4 uVar2;
  undefined2 *puVar3;
  int iVar4;
  int iVar5;
  undefined4 *puVar6;
  undefined4 *puVar7;
  undefined4 *puVar8;

  random_seed_global = *(undefined4 *)(DAT_006b0b80 + 0x10);
  if (DAT_006f1d20 != 0) {
    if (*(code **)(DAT_006f1d20 + 8) != (code *)0x0) {
      (**(code **)(DAT_006f1d20 + 8))();
    }
    DAT_006f1d20 = 0;
  }
  if (DAT_006f1d38 == '\x01') {
    puVar6 = &DAT_006b0b88;
    for (iVar4 = 0xc0; iVar4 != 0; iVar4 = iVar4 + -1) {
      *puVar6 = 0;
      puVar6 = puVar6 + 1;
    }
    DAT_006f1d38 = '\0';
  }
  game_engine_load_from_variant();
  __control87(0x9001f,0xfffff);
  FUN_00515740();
  game_state_build_header();
  puVar6 = DAT_006f1d6c;
  *DAT_006f1d6c = 0;
  puVar6[1] = 0;
  puVar6[2] = 0;
  puVar6[3] = 0;
  puVar6[4] = 0;
  puVar6[5] = 0;
  puVar6[6] = 0;
  puVar6[7] = 0;
  *(undefined1 *)puVar6 = 1;
  FUN_00494390();
  FUN_0045bc80();
  players_dispose();
  puVar6 = &DAT_00746280;
  for (iVar4 = 0x343; iVar4 != 0; iVar4 = iVar4 + -1) {
    *puVar6 = 0;
    puVar6 = puVar6 + 1;
  }
  DAT_00746280._0_1_ = 1;
  FUN_0053fa70();
  puVar8 = DAT_00746f94;
  puVar6 = DAT_00746f94;
  for (iVar4 = 0xb; puVar6 = puVar6 + 1, iVar4 != 0; iVar4 = iVar4 + -1) {
    *puVar6 = 0;
  }
  puVar6 = &DAT_0065e508;
  puVar7 = puVar8 + 0xd;
  for (iVar4 = 0x12; iVar4 != 0; iVar4 = iVar4 + -1) {
    *puVar7 = *puVar6;
    puVar6 = puVar6 + 1;
    puVar7 = puVar7 + 1;
  }
  *(undefined1 *)(puVar8 + 0xc) = 0;
  objects_reset();
  *(undefined1 *)(DAT_007c30ec + 0x24) = 1;
  data_delete_all();
  puVar6 = DAT_0072277c;
  puVar8 = DAT_0072277c;
  for (iVar4 = 0x290c; iVar4 != 0; iVar4 = iVar4 + -1) {
    *puVar8 = 0;
    puVar8 = puVar8 + 1;
  }
  *(undefined1 *)((int)puVar6 + 0x520e) = 0;
  *DAT_0072278c = 0;
  FUN_004ffd40();
  iVar4 = DAT_0087abe4;
  puVar6 = DAT_006b0ad8;
  puVar8 = DAT_006b0ad8;
  for (iVar5 = 0xa00; iVar5 != 0; iVar5 = iVar5 + -1) {
    *puVar8 = 0xffffffff;
    puVar8 = puVar8 + 1;
  }
  puVar6[0xa00] = 0xffffffff;
  puVar6[0xa01] = 0;
  puVar6[0xa02] = 0;
  *(undefined1 *)(iVar4 + 0x24) = 1;
  data_delete_all();
  camera_initialize();
  FUN_00447740();
  *(undefined1 *)(DAT_0087abec + 0x24) = 1;
  data_delete_all();
  *(undefined1 *)(DAT_0087abe8 + 0x24) = 1;
  data_delete_all();
  *(undefined1 *)(DAT_0087abd0 + 0x24) = 1;
  data_delete_all();
  *(undefined1 *)(DAT_0087abdc + 0x24) = 1;
  data_delete_all();
  *(undefined1 *)(DAT_0087abe0 + 0x24) = 1;
  data_delete_all();
  *(undefined1 *)(DAT_0087abd4 + 0x24) = 1;
  data_delete_all();
  *(undefined1 *)(DAT_0087abd8 + 0x24) = 1;
  data_delete_all();
  if (DAT_007252b6 == '\0') {
    *(undefined1 *)(DAT_007252c0 + 0x24) = 1;
    data_delete_all();
    *(undefined1 *)(DAT_00724a50 + 0x24) = 1;
    data_delete_all();
  }
  puVar3 = (undefined2 *)(DAT_00746140 + 8);
  iVar4 = 0x33;
  do {
    *(undefined4 *)(puVar3 + -2) = 0x3f800000;
    *(undefined4 *)(puVar3 + -4) = 0x3f800000;
    *puVar3 = 0;
    puVar3 = puVar3 + 6;
    iVar4 = iVar4 + -1;
  } while (iVar4 != 0);
  if (DAT_007461a0 != 0) {
    *(undefined1 *)(DAT_007461a0 + 0x24) = 1;
    data_delete_all();
    puVar6 = DAT_007461a4;
    DAT_007461a4[1] = 0xffffffff;
    *puVar6 = 0;
    puVar6[2] = 0;
  }
  _DAT_006b0ae4 = 0xffffffff;
  DAT_006b0ae0 = 0;
  *(undefined1 *)(DAT_0087abcc + 0x24) = 1;
  data_delete_all();
  _DAT_006b8d80 = _DAT_0069c534 * 118613.34;
  _DAT_006b8d7c = _DAT_0069c530 * 118613.34;
  FUN_0045c370();
  DAT_006b1458 = 1;
  update_server_new();
  game_engine_reset_player_look_state();
  puVar6 = DAT_006f1884;
  puVar8 = DAT_006f1884;
  for (iVar4 = 0x4a; puVar7 = DAT_006f1d6c, iVar4 != 0; iVar4 = iVar4 + -1) {
    *puVar8 = 0;
    puVar8 = puVar8 + 1;
  }
  *(undefined2 *)(puVar6 + 0x3f) = 0xffff;
  puVar6[0x49] = puVar7[3];
  ai_reset_for_new_map();
  puVar6 = DAT_006f187c;
  *DAT_006f187c = 0;
  puVar6[1] = 0;
  puVar6[2] = 0;
  puVar6[3] = 0;
  puVar6[4] = 0;
  puVar6[5] = 0;
  puVar6[6] = 0;
  puVar6[3] = 0xffffffff;
  puVar6[4] = 0xffffffff;
  puVar6[5] = 0xffffffff;
  puVar6[6] = 0xffffffff;
  DAT_00686b60 = 0xbf800000;
  hs_scripts_reload();
  *(undefined1 *)(DAT_006b0a10 + 0x24) = 1;
  data_delete_all();
  uVar2 = global_scenario;
  puVar1 = DAT_006b8cbc;
  *(undefined1 *)(DAT_006b0b80 + 1) = 1;
  *puVar1 = 1;
  objects_update_control_bindings(uVar2);
  *DAT_006b8cbc = 0;
  FUN_00435d50();
  return;
}
#endif
