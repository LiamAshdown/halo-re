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

#include "crt.h"
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "game.h"
#include "scenario.h"
#include "camera.h"
#include "fn_hs.h"
#include "fn_ai.h"
#include "fn_game.h"
#include "fn_rasterizer.h"

extern game_main_globals *main_game_globals; // 0x006b0b80
extern random_seed random_seed_global; // 0x00719cd0
extern game_engine_definition *current_game_engine; // 0x006f1d20
extern uint8_t player_profile_cache_initialized;     // 0x006f1d38
extern uint32_t player_profile_cache[0xc0];    // 0x006b0b88, TYPES-GAP
extern game_variant game_engine_active_variant;             // 0x0087ab20 (NOT 0x006f1c88, which is the live copy)
extern game_time_globals *game_time;                 // 0x006f1d6c
extern uint32_t unknown_00746280_block[0x343];         // 0x00746280, a block of 0x343 dwords (not a pointer)
extern scenario_game_globals *global_scenario_game_globals; // 0x00746f94, scenario.h
extern uint32_t k_default_sound_environment[0x12];        // TYPES-GAP, copied into the block above
extern data_array *object_render_state_cache;              // 0x007c30ec, TYPES-GAP (data_array*)
extern uint32_t *detail_objects;              // TYPES-GAP, 0x290c dwords + trailing byte
extern void *runtime_decals_suppressed;                         // TYPES-GAP
extern void *decal_grid_block;      // 0x006b0ad8, TYPES-GAP (0xa00+3 dwords)
extern data_array *decal_data;                          // TYPES-GAP (a data_array*)
extern data_array *contrail_data;      // TYPES-GAP
extern data_array *contrail_point_data;      // TYPES-GAP
extern data_array *particle_data;     // 0x0087abd0, TYPES-GAP
extern data_array *effect_data;// 0x0087abdc, TYPES-GAP
extern data_array *effect_location_data; // 0x0087abe0, TYPES-GAP
extern void *particle_system_data; // 0x0087abd4, TYPES-GAP
extern data_array *particle_system_particle_data; // 0x0087abd8, TYPES-GAP
extern uint8_t sound_disabled; // TYPES-GAP
extern void *sound_data;   // TYPES-GAP
extern void *looping_sound_data;   // TYPES-GAP
extern void *sound_class_gains; // 0x00746140, TYPES-GAP (0x33 records, stride 6 shorts)
extern data_array *game_looping_sound_data; // 0x007461a0, TYPES-GAP
extern uint32_t *game_sound_globals_ptr;      // TYPES-GAP
extern int32_t weather_instances; // TYPES-GAP
extern int32_t weather_instance_count; // TYPES-GAP
extern data_array *weather_particle_data; // 0x0087abcc, TYPES-GAP
extern real unknown_0069c534; // TYPES-GAP
extern real k_air_density; // TYPES-GAP
extern real unknown_0069c530; // TYPES-GAP
extern real k_water_density; // TYPES-GAP
extern uint32_t game_engine_attribute_enabled; // TYPES-GAP
extern uint32_t *player_effect_globals_pointer; // 0x006f1884, TYPES-GAP
extern void *recorded_animations; // 0x006b0a10, TYPES-GAP
extern uint32_t cinematic_saved_music_gain; // TYPES-GAP
extern uint32_t *cinematic_globals_ptr; // 0x006f187c, TYPES-GAP (7 dwords)
extern Scenario *global_scenario; // 0x00746f8c
extern uint8_t *object_globals_pointer; // TYPES-GAP


extern void camera_initialize(void);                       // 0x445580
extern void observer_new(observer *this);                    // 0x447740, blam-cc: EDX -> this
extern observer observers[];                                 // 0x006ac65c, one per local player (0x29c each)

extern void game_engine_load_from_variant(const game_variant *variant); // 0x45c2c0,
    // blam-cc: EBX -> variant (matches src/game/game_engine_load_from_variant.c)                // this batch, 0x45c2c0


extern uint8_t update_server_new(void);                             // 0x472aa0


extern void interface_local_player_state_reset(void);                                        // UNSURE module
extern void data_delete_all(data_array *array); // blam-cc: ESI -> array (src/memory/data_delete_all.c)                                // 0x4d0580, memory module, UNSURE arg
extern void scenario_objects_place(Scenario *scenario); // 0x4f3ba0. One plain stack
    // argument -- objdump shows 0x4f3ba0 reading [esp+0x24] twice and taking no register input,
    // so src/objects/scenario_objects_place.c's "blam-cc: EAX -> param_1" is wrong.          // 0x4f3ba0, UNSURE arg
extern void objects_reset(void);                                           // 0x4f4bb0
extern void breakable_surfaces_reset(void); // 0x4ffd40, objects (breakable_surface_globals reset, R79)

extern void game_state_build_header(void);                                     // 0x538000
extern void ambient_color_randomize(void);                                                  // UNSURE module

// Resets game state (objects, scripts, particle/effect pools, network server) to begin a new
// game on the currently loaded map.
void game_start_new_map(void)
{
    uint32_t i;
    uint8_t *tag_cache_bytes;
    uint32_t *cursor;
    uint32_t *dst;
    uint8_t *record;

    random_seed_global = main_game_globals->random_seed;

    if (current_game_engine != (game_engine_definition *)0) {
        if (current_game_engine->dispose != (void *)0) {
            ((void (*)(void))current_game_engine->dispose)();
        }
        current_game_engine = (game_engine_definition *)0;
    }
    if (player_profile_cache_initialized == 1) {
        for (i = 0; i < 0xc0; i = i + 1) {
            player_profile_cache[i] = 0;
        }
        player_profile_cache_initialized = 0;
    }

    game_engine_load_from_variant(&game_engine_active_variant); // objdump 0x45b09a: EBX = 0x0087ab20
    _control87(0x9001f, 0xfffff);
    decal_and_font_system_reset();
    game_state_build_header();

    {
        uint32_t *game_time_dwords = (uint32_t *)game_time;
        for (i = 0; i < 8; i = i + 1) {
            game_time_dwords[i] = 0;
        }
    }
    ((uint8_t *)game_time)[0] = 1;

    interface_local_player_state_reset();
    team_pair_table_init_defaults();
    players_dispose();

    cursor = unknown_00746280_block;
    for (i = 0x343; i != 0; i = i - 1) {
        *cursor = 0;
        cursor = cursor + 1;
    }
    *(uint8_t *)unknown_00746280_block = 1;
    ambient_color_randomize();

    tag_cache_bytes = (uint8_t *)global_scenario_game_globals;
    cursor = (uint32_t *)tag_cache_bytes;
    for (i = 1; i <= 0xb; i = i + 1) {
        cursor[i] = 0; // dword 0 is deliberately left untouched, matching the original's loop
    }
    dst = (uint32_t *)tag_cache_bytes + 0xd;
    for (i = 0x12; i != 0; i = i - 1) {
        *dst = k_default_sound_environment[0x12 - i];
        dst = dst + 1;
    }
    *((uint8_t *)((uint32_t *)tag_cache_bytes + 0xc)) = 0;

    objects_reset();
    object_render_state_cache->valid = 1;
    data_delete_all(object_render_state_cache);

    dst = detail_objects;
    for (i = 0x290c; i != 0; i = i - 1) {
        *dst = 0;
        dst = dst + 1;
    }
    *((uint8_t *)detail_objects + 0x520e) = 0;
    *(uint32_t *)runtime_decals_suppressed = 0;
    breakable_surfaces_reset();

    dst = (uint32_t *)decal_grid_block;
    for (i = 0xa00; i != 0; i = i - 1) {
        *dst = 0xffffffff;
        dst = dst + 1;
    }
    dst[0] = 0xffffffff;
    dst[1] = 0;
    dst[2] = 0;
    decal_data->valid = 1;
    data_delete_all(decal_data);

    camera_initialize();
    observer_new(&observers[0]); // 0x45b1a2: mov edx,0x6ac65c

    contrail_data->valid = 1;
    data_delete_all(contrail_data);
    contrail_point_data->valid = 1;
    data_delete_all(contrail_point_data);
    particle_data->valid = 1;
    data_delete_all(particle_data);
    effect_data->valid = 1;
    data_delete_all(effect_data);
    effect_location_data->valid = 1;
    data_delete_all(effect_location_data);
    *((uint8_t *)particle_system_data + 0x24) = 1;
    data_delete_all(particle_system_data);
    particle_system_particle_data->valid = 1;
    data_delete_all(particle_system_particle_data);

    if (sound_disabled == 0) {
        *((uint8_t *)sound_data + 0x24) = 1;
        data_delete_all(sound_data);
        *((uint8_t *)looping_sound_data + 0x24) = 1;
        data_delete_all(looping_sound_data);
    }

    // 0x45b233..0x45b252: 0x33 records of 12 bytes: two 1.0 floats then a zero word (byte offsets -8, -4, 0)
    record = (uint8_t *)sound_class_gains + 8;
    i = 0x33;
    do {
        *(uint32_t *)(record - 4) = 0x3f800000;
        *(uint32_t *)(record - 8) = 0x3f800000;
        *(uint16_t *)record = 0;
        record = record + 0xc;
        i = i - 1;
    } while (i != 0);

    if (game_looping_sound_data != (data_array *)0) {
        game_looping_sound_data->valid = 1;
        data_delete_all(game_looping_sound_data);
        game_sound_globals_ptr[1] = 0xffffffff;
        game_sound_globals_ptr[0] = 0;
        game_sound_globals_ptr[2] = 0;
    }

    weather_instances = -1;
    weather_instance_count = 0;
    weather_particle_data->valid = 1;
    data_delete_all(weather_particle_data);

    k_air_density = unknown_0069c534 * 118613.34f;
    k_water_density = unknown_0069c530 * 118613.34f;

    game_engine_initialize_for_new_game();
    game_engine_attribute_enabled = 1;
    update_server_new();
    game_engine_reset_player_look_state();

    dst = player_effect_globals_pointer;
    for (i = 0x4a; i != 0; i = i - 1) {
        *dst = 0;
        dst = dst + 1;
    }
    *(uint16_t *)((uint8_t *)player_effect_globals_pointer + 0x3f * 4) = 0xffff;
    player_effect_globals_pointer[0x49] = ((uint32_t *)game_time)[3];
    ai_reset_for_new_map();

    dst = cinematic_globals_ptr;
    dst[0] = 0;
    dst[1] = 0;
    dst[2] = 0;
    dst[3] = 0xffffffff;
    dst[4] = 0xffffffff;
    dst[5] = 0xffffffff;
    dst[6] = 0xffffffff;

    cinematic_saved_music_gain = 0xbf800000;
    hs_scripts_reload();
    *((uint8_t *)recorded_animations + 0x24) = 1;
    data_delete_all(recorded_animations);

    main_game_globals->active = 1;
    *object_globals_pointer = 1;
    scenario_objects_place(global_scenario);
    *object_globals_pointer = 0;
    encounters_spawn_initial();
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
  scenario_objects_place(uVar2);
  *DAT_006b8cbc = 0;
  FUN_00435d50();
  return;
}
#endif
