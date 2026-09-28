// saved_game_files_initialize  (Ghidra: saved_game_files_initialize, already named)
// address 0x53c260, size 535 bytes
// name confidence: 0.9   rewrite confidence: 0.6
// evidence: already named by Ghidra/CEA (cea-pdb via strings 'hdmu.map', 'saved', 'savegames').
// out/phase4/saved_games_functions.md summary "One-time module init: builds all saved-game/
// profile/playlist path strings, creates the module's directories and synchronization mutexes,
// and resets the in-memory save index." types/saved_games.h globals list documents the two bulk
// zero ranges this function performs: the 0x2c7-dword block 0x00721330..0x00721e4c (savegame_
// index_file through the eleven path buffers this function is about to fill) and the 0x1001-dword
// block from default_profile_data (0x0071d280) through unknown_0071f27c, which runs past both
// of those into default_player_profile_initialized (0x00721280, immediately set back to 1 right
// after) -- kept as a raw pointer-cursor zero rather than forced into any one struct's sizeof,
// since it deliberately spans three separate globals.
// register convention: __cdecl, no parameters.
// reconciled: R10 profile_directory is char[0x105] (k_profile_directory_storage_size; shell zeroes 0x41 dwords + 1 byte at 0x540ef9)

#include "crt.h"
#include <string.h>
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"
#include "saved_games.h"

extern char profile_directory[0x105]; // 0x006ac900 (cache module)
extern file_reference_record savegame_index_file; // 0x00721330
extern char saved_game_root_directory[0x100]; // 0x00721449
extern char saved_game_root_path[0x100]; // 0x006e3108
extern char savegames_directory[0x100]; // 0x00721549
extern char saved_directory[0x100]; // 0x00721649
extern char player_profiles_directory[0x100]; // 0x00721749
extern char default_player_profiles_directory[0x100]; // 0x00721849
extern char playlists_directory[0x100]; // 0x00721949
extern char default_playlists_directory[0x100]; // 0x00721a49
extern char last_profile_path[0x100]; // 0x00721b49
extern char last_game_variant_path[0x100]; // 0x00721c49
extern char last_multiplayer_map_path[0x100]; // 0x00721d49
extern network_mutex_record *saved_game_files_mutex; // 0x0072143c
extern uint8_t savegame_index_dirty; // 0x00721447
extern network_mutex_record *savegame_index_mutex; // 0x00721440
extern uint8_t saved_game_files_initialized; // 0x00721446
extern saved_player_profile default_profile_data; // 0x0071d280
extern uint8_t default_player_profile_initialized; // 0x00721280
extern variant_write_request variant_write_request_state; // 0x00721288
extern int16_t default_game_variant_count; // 0x00721328
extern uint8_t unknown_0072132a; // 0x0072132a

extern int32_t mutex_create(network_mutex_record **out_handle); // 0x440510
extern char directory_create_recursive(char *path); // 0x449250, foreign module
extern void player_profile_initialize(saved_player_profile *profile, int32_t local_player_index,
    uint8_t merge_existing); // 0x53a1c0, this module
extern void player_profile_write_default_files(void); // 0x53a610, this module

// blam-cc: __cdecl, no parameters
// One-time module init: zeroes the saved-game-files globals block, copies profile_directory as
// saved_game_root_directory and builds every path buffer under it, creates the saved/
// player_profiles/default_profile/playlists/default_playlist directory tree, marks the index
// dirty, creates the files and index mutexes (leaving saved_game_files_initialized false if
// either fails), builds and marks-initialized the default (unsaved) player profile, writes the
// two default profile files to disk, and resets the game-variant write-request block.
void saved_game_files_initialize(void)
{
    uint8_t *zero_cursor;
    int32_t i;
    int32_t mutex1_ok;
    int32_t mutex2_ok;

    zero_cursor = (uint8_t *)&savegame_index_file;
    for (i = 0x2c7; i != 0; i--) {
        *(uint32_t *)zero_cursor = 0;
        zero_cursor += 4;
    }

    strncpy(saved_game_root_directory, profile_directory, 0xff);
    _snprintf(saved_game_root_path, 0xff, "%s\\%s\\%s", saved_game_root_directory, "saved", "hdmu.map");
    _snprintf(savegames_directory, 0xff, "%s\\%s", saved_game_root_directory, "savegames");
    _snprintf(saved_directory, 0xff, "%s\\%s", saved_game_root_directory, "saved");
    directory_create_recursive(saved_directory);
    _snprintf(player_profiles_directory, 0xff, "%s\\%s", saved_game_root_directory, "saved\\player_profiles");
    directory_create_recursive(player_profiles_directory);
    _snprintf(default_player_profiles_directory, 0xff, "%s\\%s", saved_game_root_directory,
        "saved\\player_profiles\\default_profile");
    directory_create_recursive(default_player_profiles_directory);
    _snprintf(playlists_directory, 0xff, "%s\\%s", saved_game_root_directory, "saved\\playlists");
    directory_create_recursive(playlists_directory);
    _snprintf(default_playlists_directory, 0xff, "%s\\%s", saved_game_root_directory,
        "saved\\playlists\\default_playlist");
    directory_create_recursive(default_playlists_directory);
    _snprintf(last_profile_path, 0xff, "%s\\%s", saved_game_root_directory, "lastprof.txt");
    _snprintf(last_game_variant_path, 0xff, "%s\\%s", saved_game_root_directory, "lastmpvr.txt");
    _snprintf(last_multiplayer_map_path, 0xff, "%s\\%s", saved_game_root_directory, "lastmpmp.txt");

    savegame_index_dirty = 1;
    saved_game_files_mutex = 0;
    savegame_index_mutex = 0;
    mutex1_ok = mutex_create(&saved_game_files_mutex);
    if (mutex1_ok != 0) {
        mutex2_ok = mutex_create(&savegame_index_mutex);
        saved_game_files_initialized = 1;
        if (mutex2_ok != 0) {
            goto default_profile;
        }
    }
    saved_game_files_initialized = 0;

default_profile:
    zero_cursor = (uint8_t *)&default_profile_data;
    for (i = 0x1001; i != 0; i--) {
        *(uint32_t *)zero_cursor = 0;
        zero_cursor += 4;
    }
    player_profile_initialize(&default_profile_data, 0, 0);
    default_player_profile_initialized = 1;
    player_profile_write_default_files();

    zero_cursor = (uint8_t *)&variant_write_request_state;
    for (i = 0x29; i != 0; i--) {
        *(uint32_t *)zero_cursor = 0;
        zero_cursor += 4;
    }
    unknown_0072132a = 1;
}

#if 0
Original Ghidra decompilation (0x53c260):

void __cdecl saved_game_files_initialize(void)

{
  char cVar1;
  int iVar2;
  undefined4 *puVar3;

  puVar3 = &DAT_00721330;
  for (iVar2 = 0x2c7; iVar2 != 0; iVar2 = iVar2 + -1) {
    *puVar3 = 0;
    puVar3 = puVar3 + 1;
  }
  _strncpy(&DAT_00721449,(char *)&DAT_006ac900,0xff);
  __snprintf(&DAT_006e3108,0xff,"%s\\%s\\%s",&DAT_00721449,"saved","hdmu.map");
  __snprintf(&DAT_00721549,0xff,"%s\\%s",&DAT_00721449,"savegames");
  __snprintf(&DAT_00721649,0xff,"%s\\%s",&DAT_00721449,"saved");
  directory_create_recursive(&DAT_00721649);
  __snprintf(&DAT_00721749,0xff,"%s\\%s",&DAT_00721449,"saved\\player_profiles");
  directory_create_recursive(&DAT_00721749);
  __snprintf(&DAT_00721849,0xff,"%s\\%s",&DAT_00721449,"saved\\player_profiles\\default_profile");
  directory_create_recursive(&DAT_00721849);
  __snprintf(&DAT_00721949,0xff,"%s\\%s",&DAT_00721449,"saved\\playlists");
  directory_create_recursive(&DAT_00721949);
  __snprintf(&DAT_00721a49,0xff,"%s\\%s",&DAT_00721449,"saved\\playlists\\default_playlist");
  directory_create_recursive(&DAT_00721a49);
  __snprintf(&DAT_00721b49,0xff,"%s\\%s",&DAT_00721449,"lastprof.txt");
  __snprintf(&DAT_00721c49,0xff,"%s\\%s",&DAT_00721449,"lastmpvr.txt");
  __snprintf(&DAT_00721d49,0xff,"%s\\%s",&DAT_00721449,"lastmpmp.txt");
  DAT_00721447 = 1;
  DAT_0072143c = 0;
  DAT_00721440 = 0;
  cVar1 = mutex_create();
  if (cVar1 != '\0') {
    cVar1 = mutex_create();
    DAT_00721446 = 1;
    if (cVar1 != '\0') goto LAB_0053c437;
  }
  DAT_00721446 = 0;
LAB_0053c437:
  puVar3 = &DAT_0071d280;
  for (iVar2 = 0x1001; iVar2 != 0; iVar2 = iVar2 + -1) {
    *puVar3 = 0;
    puVar3 = puVar3 + 1;
  }
  player_profile_initialize(&DAT_0071d280,0,0);
  DAT_00721280 = 1;
  FUN_0053a610();
  puVar3 = &DAT_00721288;
  for (iVar2 = 0x29; iVar2 != 0; iVar2 = iVar2 + -1) {
    *puVar3 = 0;
    puVar3 = puVar3 + 1;
  }
  DAT_00721328._2_1_ = 1;
  return;
}
#endif
