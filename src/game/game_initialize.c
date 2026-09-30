// game_initialize  (Ghidra: particle_systems_initialize; RENAMED per symbols/review_queue.txt --
// the previous name was too narrow, see below)
// address 0x45a9c0, size 783 bytes
// name confidence: 0.4   rewrite confidence: 0.45
// evidence: symbols/review_queue.txt 0x45a9c0 "sequentially bump-allocates and zeroes memory
//   pools (crc32_update-tracked) for particles, effects, effect locations, weather particles,
//   particle systems, contra[ils]..."; out/phase4/game_types_notes.md notes this function is
//   the counterpart to game_dispose (0x45acd0, this batch), which tears down the same globals.
//   Every allocation here is one-time post-map-load setup (particle/effect pools, the object
//   render-state cache, players, sound, AI, scripts, save files), not specific to particle
//   systems, hence the rename.
// register convention: no arguments.
//
// UNSURE: most pool sizes/globals here belong to other modules (objects, sound, ai, hs,
// saved_games) and are not named in types/game.h; kept as raw globals with TYPES-GAP markers.
// _control87 is the MSVC CRT FPU-control-word setter.
// reconciled: R07 0x00746f94 tag_cache_render_states_* (TYPES-GAP) -> scenario.h scenario_game_globals *global_scenario_game_globals (0x7c-byte scenario game-state block); R79 0x006b8d78 effect_something_006b8d78 -> physics.h breakable_surface_globals *breakable_surface_state

#include "crt.h"
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "physics.h"
#include "scenario.h"
#include "objects.h"
#include "effects.h"
#include "fn_hs.h"
#include "fn_ai.h"
#include "fn_game.h"
#include "fn_sound.h"
#include "fn_objects.h"

extern int32_t game_state_cursor; // 0x006e2dcc
extern uint8_t *game_state_base;   // 0x006e2dc8
extern uint32_t game_state_crc;   // 0x006e2dd4
extern void *main_game_globals; // 0x006b0b80, TYPES-GAP
extern game_variant game_engine_active_variant; // 0x0087ab20, the 0x98-byte staging variant
    // (0x26 dwords) this function zeroes and then hands to game_engine_load_from_variant in EBX
extern scenario_game_globals *global_scenario_game_globals; // 0x00746f94, scenario.h
extern uint8_t *hs_camera_control_pointer;             // 0x0087bc0c, TYPES-GAP, single byte zeroed
extern data_array *object_render_state_cache;       // 0x007c30ec, TYPES-GAP
extern void *runtime_decals_suppressed;                 // 0x0072278c, TYPES-GAP
extern breakable_surface_globals *breakable_surface_state; // 0x006b8d78, physics.h
extern data_array *particle_data;               // 0x0087abd0, TYPES-GAP
extern data_array *effect_data;          // 0x0087abdc, TYPES-GAP
extern data_array *effect_location_data;        // 0x0087abe0, TYPES-GAP
extern data_array *weather_particle_data;       // 0x0087abcc, TYPES-GAP
extern void *particle_system_data;        // 0x0087abd4, TYPES-GAP
extern data_array *particle_system_particle_data; // 0x0087abd8, TYPES-GAP
extern void *sound_class_gains;        // 0x00746140, TYPES-GAP
extern player_effect_globals *player_effect_globals_pointer;
extern void *recorded_animations;    // 0x006b0a10, TYPES-GAP
extern uint32_t *cinematic_globals_ptr; // 0x006f187c, TYPES-GAP (7 dwords)


extern void contrails_initialize(void);                 // 0x44c8b0
extern void decals_initialize(void);                     // 0x44df90

extern void game_engine_load_from_variant(const game_variant *variant); // 0x45c2c0,
    // blam-cc: EBX -> variant (matches src/game/game_engine_load_from_variant.c)


extern void input_state_initialize(void);                     // 0x48b3e0
extern void input_queue_initialize(void);   // UNSURE module
extern void interface_globals_allocate(void);   // UNSURE module
extern void player_profile_subsystem_initialize(void);   // UNSURE module
extern void widget_memory_pool_initialize(void);              // 0x4979b0
extern void crc32_update(uint32_t *crc, uint8_t *data, int32_t length); // 0x4d02d0, memory module
extern data_array *data_new(int16_t element_size, char *name, int16_t maximum_count); // 0x4d0370,
    // memory module; blam-cc: element size in EBX, then the stack pair (name, maximum_count)

extern data_array *game_state_new(char *name, int16_t maximum_count, int16_t element_size); // 0x5380d0, blam-cc: EBX -> element_size, stack -> name, maximum_count
    // maximum_count. CORRECTED by review: the first pass dropped the EBX element size
    // (Ghidra never shows it), which is the same 3-argument form players_initialize.c
    // uses and which types/game.h's own header note derives. The sizes below come from
    // objdump -d --start-address=0x45a9c0 --stop-address=0x45b050.
    // NOTE: src/hs still declares the 2-argument view of this function.
extern void saved_game_files_initialize(void);                       // 0x53c260

extern void detail_objects_globals_allocate(void); // UNSURE module

// One-time post-map-load initialization that bump-allocates every particle/effect/render-state
// pool, sets the FPU control word, allocates the simulation tick record, loads the active game
// engine, allocates the team-pair table, and starts the object, player, sound, AI, script, input
// and save-file subsystems.
void game_initialize(void)
{
    uint32_t *cursor;
    int32_t i;
    uint32_t size;

    cursor = (uint32_t *)(game_state_cursor + game_state_base);
    game_state_cursor = game_state_cursor + 0x114;
    size = 0x114;
    crc32_update(&game_state_crc, (uint8_t *)&size, 4);
    main_game_globals = cursor;
    for (i = 0x45; i != 0; i = i - 1) {
        *cursor = 0;
        cursor = cursor + 1;
    }

    cursor = (uint32_t *)&game_engine_active_variant;
    for (i = 0x26; i != 0; i = i - 1) {
        *cursor = 0;
        cursor = cursor + 1;
    }

    _control87(0x9001f, 0xfffff);
    game_engine_allocate_tick_record();
    game_engine_load_from_variant(&game_engine_active_variant); // objdump 0x45aa24: EBX = 0x0087ab20
    team_pair_table_allocate();
    interface_globals_allocate();

    size = 0x7c;
    global_scenario_game_globals = (scenario_game_globals *)(game_state_cursor + game_state_base);
    game_state_cursor = game_state_cursor + 0x7c;
    crc32_update(&game_state_crc, (uint8_t *)&size, 4);

    hs_camera_control_pointer = (uint8_t *)(game_state_cursor + game_state_base);
    size = 4;
    game_state_cursor = game_state_cursor + 4;
    crc32_update(&game_state_crc, (uint8_t *)&size, 4);
    *hs_camera_control_pointer = 0;

    object_render_state_cache = (data_array *)game_state_new("cached object render states", 0x100, 0x100);
    objects_initialize();
    detail_objects_globals_allocate();

    size = 4;
    runtime_decals_suppressed = (void *)(game_state_cursor + game_state_base);
    game_state_cursor = game_state_cursor + 4;
    crc32_update(&game_state_crc, (uint8_t *)&size, 4);

    size = 0x4204;
    breakable_surface_state = (breakable_surface_globals *)(game_state_cursor + game_state_base);
    game_state_cursor = game_state_cursor + 0x4204;
    crc32_update(&game_state_crc, (uint8_t *)&size, 4);

    decals_initialize();
    players_initialize();
    contrails_initialize();

    particle_data = (data_array *)game_state_new("particle", 0x400, 0x70);
    effect_data = (data_array *)game_state_new("effect", 0x100, 0xfc);
    effect_location_data = (data_array *)game_state_new("effect location", 0x200, 0x3c);
    weather_particle_data = data_new(0x54, "weather particles", 0x200); // objdump 0x45ab96: EBX = 0x54
    particle_system_data = game_state_new("particle systems", 0x40, 0x158);
    particle_system_particle_data = (data_array *)game_state_new("particle system particles", 0x200, 0x80);

    size = 0x264;
    sound_class_gains = (void *)(game_state_cursor + game_state_base);
    game_state_cursor = game_state_cursor + 0x264;
    crc32_update(&game_state_crc, (uint8_t *)&size, 4);
    game_sound_initialize();

    size = 0x128;
    player_effect_globals_pointer = (player_effect_globals *)(game_state_cursor + game_state_base);
    game_state_cursor = game_state_cursor + 0x128;
    crc32_update(&game_state_crc, (uint8_t *)&size, 4);
    ai_initialize_for_new_map();

    widget_memory_pool_initialize();
    object_lists_initialize();
    hs_runtime_initialize();
    hs_scripts_reload();

    recorded_animations = game_state_new("recorded animations", 0x40, 0x64);

    size = 0x1c;
    cinematic_globals_ptr = (uint32_t *)(game_state_cursor + game_state_base);
    game_state_cursor = game_state_cursor + 0x1c;
    crc32_update(&game_state_crc, (uint8_t *)&size, 4);
    saved_game_files_initialize();

    input_queue_initialize();
    input_state_initialize();
    player_profile_subsystem_initialize();
}

#if 0
Original Ghidra decompilation (0x45a9c0), from tools/pack.py 0x45a9c0:

void particle_systems_initialize(void)

{
  undefined1 *puVar1;
  int iVar2;
  int iVar3;
  undefined4 *puVar4;
  undefined4 local_4;

  puVar4 = (undefined4 *)(DAT_006e2dcc + DAT_006e2dc8);
  DAT_006e2dcc = DAT_006e2dcc + 0x114;
  local_4 = 0x114;
  crc32_update(&DAT_006e2dd4,&local_4,4);
  DAT_006b0b80 = puVar4;
  for (iVar3 = 0x45; iVar3 != 0; iVar3 = iVar3 + -1) {
    *puVar4 = 0;
    puVar4 = puVar4 + 1;
  }
  puVar4 = &DAT_0087ab20;
  for (iVar3 = 0x26; iVar3 != 0; iVar3 = iVar3 + -1) {
    *puVar4 = 0;
    puVar4 = puVar4 + 1;
  }
  __control87(0x9001f,0xfffff);
  game_engine_allocate_tick_record();
  game_engine_load_from_variant();
  FUN_0045bc30();
  FUN_00494340();
  iVar3 = DAT_006e2dcc + DAT_006e2dc8;
  DAT_006e2dcc = DAT_006e2dcc + 0x7c;
  local_4 = 0x7c;
  crc32_update(&DAT_006e2dd4,&local_4,4);
  puVar1 = (undefined1 *)(DAT_006e2dcc + DAT_006e2dc8);
  DAT_006e2dcc = DAT_006e2dcc + 4;
  local_4 = 4;
  DAT_00746f94 = iVar3;
  crc32_update(&DAT_006e2dd4,&local_4,4);
  DAT_0087bc0c = puVar1;
  *puVar1 = 0;
  DAT_007c30ec = game_state_new("cached object render states",0x100);
  objects_initialize();
  FUN_00552260();
  iVar3 = DAT_006e2dcc + DAT_006e2dc8;
  DAT_006e2dcc = DAT_006e2dcc + 4;
  local_4 = 4;
  crc32_update(&DAT_006e2dd4,&local_4,4);
  iVar2 = DAT_006e2dcc + DAT_006e2dc8;
  DAT_006e2dcc = DAT_006e2dcc + 0x4204;
  local_4 = 0x4204;
  DAT_0072278c = iVar3;
  crc32_update(&DAT_006e2dd4,&local_4,4);
  DAT_006b8d78 = iVar2;
  decals_initialize();
  players_initialize();
  contrails_initialize();
  DAT_0087abd0 = game_state_new("particle",0x400);
  DAT_0087abdc = game_state_new("effect",0x100);
  DAT_0087abe0 = game_state_new("effect location",0x200);
  DAT_0087abcc = data_new("weather particles",0x200);
  DAT_0087abd4 = game_state_new("particle systems",0x40);
  DAT_0087abd8 = game_state_new("particle system particles",0x200);
  iVar3 = DAT_006e2dcc + DAT_006e2dc8;
  DAT_006e2dcc = DAT_006e2dcc + 0x264;
  local_4 = 0x264;
  crc32_update(&DAT_006e2dd4,&local_4,4);
  DAT_00746140 = iVar3;
  game_sound_initialize();
  iVar3 = DAT_006e2dcc + DAT_006e2dc8;
  DAT_006e2dcc = DAT_006e2dcc + 0x128;
  local_4 = 0x128;
  crc32_update(&DAT_006e2dd4,&local_4,4);
  DAT_006f1884 = iVar3;
  ai_initialize_for_new_map();
  widget_memory_pool_initialize();
  object_lists_initialize();
  hs_runtime_initialize();
  hs_scripts_reload();
  DAT_006b0a10 = game_state_new("recorded animations",0x40);
  iVar3 = DAT_006e2dcc + DAT_006e2dc8;
  DAT_006e2dcc = DAT_006e2dcc + 0x1c;
  local_4 = 0x1c;
  crc32_update(&DAT_006e2dd4,&local_4,4);
  DAT_006f187c = iVar3;
  saved_game_files_initialize();
  FUN_00492250();
  input_state_initialize();
  FUN_00495370();
  return;
}
#endif
