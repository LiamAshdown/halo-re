// game_stop_current_map  (Ghidra: FUN_0045b370; renamed per symbols/review_queue.txt)
// address 0x45b370, size 369 bytes (0x45b370..0x45b4e0; Ghidra's metadata said 347 and catalogued the
//   last 22 bytes as the bogus function 0x45b4cb "game_initialize_mod_per_map_upgrade_effects" --
//   the game_time reset, widget_close_all and the slot-table clear at the end of the body below;
//   re-checked against objdump by orphan pass 4)
// name confidence: 0.45   rewrite confidence: 0.35
// evidence: symbols/review_queue.txt 0x45b370 "mirrors game_start_new_map's globals but clears
//   their 'active' flags to 0 instead of setting them, and calls hs_scripts_free, cache_flush,
//   and game_sound_revert_scripting_sounds"; types/memory.h data_array::valid (0x24, "data_new leaves 0; cache_new
//   sets 1"), matching every `*(byte*)(X+0x24) = 0` write here to a data_array pointed to by X.
// register convention: no arguments.
//
// UNSURE: most of the individual data_array pointers are pools owned by other modules
// (fonts, hud, widgets, the map font cache at 0x008802XX) and are not named in types/game.h;
// declared as raw `data_array *` TYPES-GAP externs.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"

extern void *recorded_animations_pool_ptr; // 0x006b0a10, TYPES-GAP (data_array*)
extern uint32_t unknown_0071d164; // TYPES-GAP
extern uint32_t *saved_games_something_006f187c; // 0x006f187c, TYPES-GAP
extern data_array *unknown_008802d4; // TYPES-GAP
extern data_array *unknown_008802c8; // TYPES-GAP
extern data_array *unknown_008802d0; // TYPES-GAP
extern data_array *unknown_008802c0; // TYPES-GAP
extern data_array *unknown_00880360; // TYPES-GAP
extern data_array *unknown_0088035c; // TYPES-GAP
extern data_array *unknown_00880358; // TYPES-GAP
extern uint8_t *unknown_00880354;    // TYPES-GAP, byte+1 cleared (not a data_array valid flag)
extern data_array *weather_particle_pool_ptr; // 0x0087abcc
extern uint32_t unknown_0071d1c0; // TYPES-GAP
extern data_array *unknown_0087abe4; // TYPES-GAP
extern data_array *object_render_state_cache; // 0x007c30ec
extern data_array *player_data; // 0x0087a480
extern data_array *team_data;   // 0x0087a47c
extern data_array *network_predicted_globals; // 0x007461a0
extern uint32_t unknown_006ac568; // TYPES-GAP
extern real unknown_006ac624;     // TYPES-GAP
extern uint32_t unknown_006ac620; // TYPES-GAP
extern uint8_t *unknown_0087bc0c; // 0x0087bc0c
extern data_array *unknown_0087abe8; // TYPES-GAP
extern uint32_t unknown_006e4728; // TYPES-GAP
extern data_array *unknown_0087abec; // TYPES-GAP
extern data_array *particle_pool_ptr;      // 0x0087abd0
extern data_array *effect_object_pool_ptr; // 0x0087abdc
extern data_array *effect_location_pool_ptr; // 0x0087abe0
extern game_engine_definition *current_game_engine; // 0x006f1d20
extern game_time_globals *game_time; // 0x006f1d6c
extern uint32_t unknown_00746280_block[0x343]; // 0x00746280, a block (mov edi,0x746280; rep stos), not a pointer
extern game_main_globals *main_game_globals; // 0x006b0b80

extern void decal_clear_flags(uint8_t clear_object_attached); // 0x44e220, BL
extern void particle_systems_delete_all(void);              // UNSURE module
extern void update_queues_dispose(void);      // 0x472b00
extern void hs_scripts_free(void);             // 0x4832b0
extern void cache_flush(cache *self); // 0x4d17f0, blam-cc: ESI -> self (src/memory/cache_flush.c)                  // 0x4d17f0, memory module
extern void objects_flush_dirty_state(void);     // 0x4f4cc0
extern void font_glyph_cache_clear_all(void);     // 0x514cb0
extern void game_sound_revert_scripting_sounds(void);                 // 0x543a90
extern void sound_fade_out_and_stop_all(void);                     // UNSURE module
extern void widget_close_all(void);                  // 0x498650

// Shuts down the currently running game (frees scripts, flushes caches, reverts modded state)
// as the counterpart to game_start_new_map.
void game_stop_current_map(void)
{
    uint8_t had_network_predicted_globals;

    font_glyph_cache_clear_all();
    unknown_0071d164 = 0;
    ((data_array *)recorded_animations_pool_ptr)->valid = 0;
    hs_scripts_free();

    ((uint8_t *)saved_games_something_006f187c)[8] = 0;
    ((uint8_t *)saved_games_something_006f187c)[9] = 0;
    unknown_008802d4->valid = 0;
    unknown_008802c8->valid = 0;
    unknown_008802d0->valid = 0;
    unknown_008802c0->valid = 0;
    unknown_00880360->valid = 0;
    unknown_0088035c->valid = 0;
    unknown_00880358->valid = 0;
    unknown_00880354[1] = 0;
    particle_systems_delete_all();

    if (weather_particle_pool_ptr->valid != 0) {
        weather_particle_pool_ptr->valid = 0;
    }
    if (unknown_0071d1c0 != 0) {
        decal_clear_flags(1); // FIXED: BL = 1 (0x45b3f9)
        cache_flush((cache *)unknown_0071d1c0); // objdump 0x45b3ef: ESI = DAT_0071d1c0
    }
    unknown_0087abe4->valid = 0;
    if (object_render_state_cache != (data_array *)0 && object_render_state_cache->valid != 0) {
        object_render_state_cache->valid = 0;
    }
    objects_flush_dirty_state();

    had_network_predicted_globals = network_predicted_globals != (data_array *)0;
    unknown_006ac568 = 0;
    unknown_006ac624 = 1.0f;
    unknown_006ac620 = 0;
    *unknown_0087bc0c = 0;
    unknown_006e4728 = 0xffffffff;
    player_data->valid = 0;
    team_data->valid = 0;
    unknown_0087abe8->valid = 0;
    unknown_0087abec->valid = 0;
    particle_pool_ptr->valid = 0;
    effect_object_pool_ptr->valid = 0;
    effect_location_pool_ptr->valid = 0;

    if (had_network_predicted_globals && network_predicted_globals->valid != 0) {
        game_sound_revert_scripting_sounds();
        network_predicted_globals->valid = 0;
    }

    sound_fade_out_and_stop_all();
    update_queues_dispose();

    if (current_game_engine != (game_engine_definition *)0 && current_game_engine->dispose_from_old_game != (void *)0) {
        ((void (*)(void))current_game_engine->dispose_from_old_game)();
    }

    *(uint8_t *)unknown_00746280_block = 0;
    if (game_time != (game_time_globals *)0) {
        ((uint8_t *)game_time)[0] = 0;
        ((uint8_t *)game_time)[1] = 0;
    }

    widget_close_all();
    main_game_globals->active = 0;
}

#if 0
Original Ghidra decompilation (0x45b370), from tools/pack.py 0x45b370:

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void FUN_0045b370(void)

{
  int iVar1;
  undefined1 *puVar2;
  int iVar3;
  int iVar4;
  int iVar5;
  int iVar6;
  bool bVar7;

  font_glyph_cache_clear_all();
  DAT_0071d164 = 0;
  *(undefined1 *)(DAT_006b0a10 + 0x24) = 0;
  hs_scripts_free();
  iVar5 = DAT_008802d4;
  iVar3 = DAT_008802c8;
  iVar1 = DAT_006f187c;
  *(undefined1 *)(DAT_006f187c + 8) = 0;
  *(undefined1 *)(iVar1 + 9) = 0;
  iVar4 = DAT_008802d0;
  *(undefined1 *)(iVar5 + 0x24) = 0;
  iVar1 = DAT_008802c0;
  *(undefined1 *)(iVar3 + 0x24) = 0;
  iVar5 = DAT_00880360;
  *(undefined1 *)(iVar4 + 0x24) = 0;
  iVar4 = DAT_0088035c;
  *(undefined1 *)(iVar1 + 0x24) = 0;
  iVar3 = DAT_00880358;
  *(undefined1 *)(iVar5 + 0x24) = 0;
  iVar1 = DAT_00880354;
  *(undefined1 *)(iVar4 + 0x24) = 0;
  *(undefined1 *)(iVar3 + 0x24) = 0;
  *(undefined1 *)(iVar1 + 1) = 0;
  FUN_004535b0();
  if (*(char *)(DAT_0087abcc + 0x24) != '\0') {
    *(undefined1 *)(DAT_0087abcc + 0x24) = 0;
  }
  if (DAT_0071d1c0 != 0) {
    FUN_0044e220();
    cache_flush();
  }
  *(undefined1 *)(DAT_0087abe4 + 0x24) = 0;
  if ((DAT_007c30ec != 0) && (*(char *)(DAT_007c30ec + 0x24) != '\0')) {
    *(undefined1 *)(DAT_007c30ec + 0x24) = 0;
  }
  objects_flush_dirty_state();
  iVar4 = DAT_0087a480;
  iVar3 = DAT_0087a47c;
  iVar1 = DAT_007461a0;
  bVar7 = DAT_007461a0 != 0;
  DAT_006ac568 = 0;
  _DAT_006ac624 = 0x3f800000;
  DAT_006ac620 = 0;
  *DAT_0087bc0c = 0;
  iVar5 = DAT_0087abe8;
  DAT_006e4728 = 0xffffffff;
  *(undefined1 *)(iVar4 + 0x24) = 0;
  iVar6 = DAT_0087abec;
  *(undefined1 *)(iVar3 + 0x24) = 0;
  iVar3 = DAT_0087abd0;
  *(undefined1 *)(iVar5 + 0x24) = 0;
  iVar4 = DAT_0087abdc;
  *(undefined1 *)(iVar6 + 0x24) = 0;
  iVar5 = DAT_0087abe0;
  *(undefined1 *)(iVar3 + 0x24) = 0;
  *(undefined1 *)(iVar4 + 0x24) = 0;
  *(undefined1 *)(iVar5 + 0x24) = 0;
  if ((bVar7) && (*(char *)(iVar1 + 0x24) != '\0')) {
    chimera__revert();
    *(undefined1 *)(iVar1 + 0x24) = 0;
  }
  FUN_005495f0();
  update_queues_dispose();
  if ((DAT_006f1d20 != 0) && (*(code **)(DAT_006f1d20 + 0x10) != (code *)0x0)) {
    (**(code **)(DAT_006f1d20 + 0x10))();
  }
  puVar2 = DAT_006f1d6c;
  DAT_00746280._0_1_ = 0;
  if (DAT_006f1d6c != (undefined1 *)0x0) {
    *DAT_006f1d6c = 0;
    puVar2[1] = 0;
  }
  widget_close_all();
  *(undefined1 *)(DAT_006b0b80 + 1) = 0;
  return;
}
#endif
