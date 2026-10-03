#pragma once

#include <stdint.h>
#include "crt.h"
#include "win32.h"
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "hs.h"
#include "networking.h"
#include "interface.h"
#include "saved_games.h"
#include "cache.h"

namespace halo::game {

/**
 * Save-game file operations: creation, deletion, enumeration and slot handle packing.
 */
class SaveGameFiles {
public:
    SaveGameFiles() = delete;

    static uint32_t create(const uint16_t *save_game_name, const char *root_path, int32_t mode, char *out_path, uint32_t out_path_size);
    static uint32_t remove_files(const uint16_t *save_game_name, const char *root_path);
    static int32_t find_first(char *root_path, win32_find_dataa *find_data);
    static uint32_t find_next(win32_find_dataa *find_data, uint32_t handle);
    static uint32_t slot_handle_pack(uint32_t slot_index, uint32_t type_nibble, uint8_t flag_bit_30, uint8_t flag_bit_31);
};

/**
 * The on-disk save-game slot index file.
 */
class SaveGameIndex {
public:
    SaveGameIndex() = delete;

    static uint8_t append_slot(const void *entry, uint32_t *out_slot_count);
    static uint8_t file_exists();
    static uint32_t get_slot_count();
    static uint8_t read_slot(uint16_t slot, void *out_entry);
    static uint8_t remove_slot(uint16_t slot);
    static uint8_t write_slot(uint16_t slot, const void *entry);
};

/**
 * Registry mapping a user id to its save-game directory.
 */
class UserSavePaths {
public:
    UserSavePaths() = delete;

    static char * lookup(uint32_t user_id);
    static int32_t user_save_path_register(uint32_t user_id, char *path);
    static uint8_t user_save_path_remove(uint32_t user_id);
};

/**
 * Lookup into unicode string-list tags.
 */
class UnicodeStringLists {
public:
    UnicodeStringLists() = delete;

    static wchar_t * get_string(char *path, int16_t index);
};

/**
 * User profile sign-in and cached player customization state.
 */
class UserProfile {
public:
    UserProfile() = delete;

    static uint32_t signin_state_is_valid();
    static void profile_cache_initialize();
    static uint8_t customization_slot_set(uint8_t *base, uint8_t new_value, uint32_t key);
};

}  // namespace halo::game
