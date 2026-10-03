#include "crt.h"
#include "win32.h"
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"
#include "saved_games.h"
#include <string.h>
#include "halo/saved_games/saved_games.hpp"
#include "halo/saved_games/api.hpp"

extern "C" {
extern int32_t saved_player_profile_slots_handle;
extern uint8_t checkpoint_sort_newest_first;
extern char *campaign_level_paths[k_campaign_level_count];
extern ColorARGB *actor_mode_default_look_weights;
extern void chimera__console_out(ColorARGB *color, char *format, ...);
extern char network_ban_file_read_mode_string[];
extern uint8_t game_state_write_in_progress;
extern game_time_globals *game_time;
extern char network_summary_log_mode_string[];
extern int16_t campaign_level_find_index_for_path(char *scenario_name);
extern int16_t pending_difficulty;
extern void main_queue_map_change(char *map_name);
}

namespace halo::saved_games::checkpoint {

/**
 * Enumerates the checkpoint files of the current profile, optionally including autosaves. The
 * entries are read into a 0x66-entry array, sorted with saved_game_checkpoint_compare (newest
 * first on request) and passed to callback one by one. Returns the number of entries visited.
 *
 * @address 0x00538e70
 */
int32_t enumerate_files(uint8_t include_autosaves, uint8_t sort_newest_first, checkpoint_enumerate_proc callback,
    void *user_data)
{
    checkpoint_file_entry *entries;
    checkpoint_file_entry *entry;
    char directory[264];
    char search_path[264];
    win32_find_dataa find_data;
    void *find_handle;
    uint32_t found_count;
    char name[264];
    char *extension;
    char *basename;
    int32_t difficulty;
    int32_t game_time_ticks;
    win32_systemtime time;
    int16_t level;
    int32_t accepted;
    uint32_t remaining;

    entries = (checkpoint_file_entry *)GlobalAlloc(0, k_maximum_checkpoint_files * sizeof(checkpoint_file_entry));
    halo::saved_games::saved_game_get_directory_by_handle(saved_player_profile_slots_handle, directory);
    sprintf(search_path, "%s%s", directory, "checkpoints\\*.sav");

    found_count = 0;
    find_handle = FindFirstFileA(search_path, (LPWIN32_FIND_DATAA)&find_data);
    if (find_handle != (void *)0xffffffff) {
        entry = entries;
        do {
            sprintf(name, "%s%s", "checkpoints\\", find_data.cFileName);
            extension = strchr(name, '.');
            if (extension != 0) {
                *extension = 0;
            }

            level = halo::saved_games::game_checkpoint_read_stats_file(&difficulty, name, &game_time_ticks, &time);
            if (level != -1) {
                if (strstr(name, "autosave") == 0 || include_autosaves != 0) {
                    entry->level_index = level;
                    entry->game_time = game_time_ticks;
                    entry->difficulty = difficulty;
                    entry->time = time;
                    entry->last_write_time[0] = find_data.ftLastWriteTime[0];
                    entry->last_write_time[1] = find_data.ftLastWriteTime[1];
                    entry->kind = _checkpoint_kind_checkpoint;

                    basename = strchr(name, '\\') + 1;
                    sprintf(entry->name, "%s", basename);
                    if (strstr(basename, "autosave") == basename && basename[8] == 0) {
                        entry->kind = _checkpoint_kind_autosave;
                    } else if (strstr(basename, "autosave1") == basename && basename[9] == 0) {
                        entry->kind = _checkpoint_kind_autosave1;
                    }

                    found_count = found_count + 1;
                    entry = entry + 1;
                }
            }
        } while (FindNextFileA(find_handle, (LPWIN32_FIND_DATAA)&find_data) != 0 && found_count < k_maximum_checkpoint_files);
        FindClose(find_handle);
    }

    checkpoint_sort_newest_first = sort_newest_first;
    qsort(entries, found_count, sizeof(checkpoint_file_entry),
        (int32_t (*)(const void *, const void *))halo::saved_games::saved_game_checkpoint_compare);

    accepted = 0;
    remaining = found_count;
    entry = entries;
    if (0 < (int32_t)remaining) {
        do {
            if (callback == 0) {
                accepted = accepted + 1;
            } else if (callback(accepted, entry->name, (int32_t)(uint16_t)entry->level_index, entry->difficulty,
                           entry->game_time, &entry->time, user_data) != 0) {
                accepted = accepted + 1;
            }
            entry = entry + 1;
            remaining = remaining - 1;
        } while (remaining != 0);
    }

    GlobalFree(entries);
    return accepted;
}

/**
 * Finds the first unused "checkpoints\checkpointN" (N = 0..99) slot under directory and writes
 * it to out_name. If all 100 slots are taken, reclaims one instead via
 * game_checkpoint_enumerate_files (oldest/lowest-priority entry first, autosaves excluded).
 *
 * @address 0x00538ae0
 */
uint8_t get_next_filename(char *out_name, char *directory)
{
    int32_t index;
    char path[256];
    win32_find_dataa find_data;
    void *find_handle;

    for (index = 0; index <= 99; index = index + 1) {
        sprintf(out_name, "checkpoints\\checkpoint%d", index);
        sprintf(path, "%s%s.sav", directory, out_name);
        find_handle = FindFirstFileA(path, (LPWIN32_FIND_DATAA)&find_data);
        if (find_handle == (void *)0xffffffff) {
            return 1;
        }
        FindClose(find_handle);
    }

    *out_name = 0;
    halo::saved_games::game_checkpoint_enumerate_files(0, 0, halo::saved_games::game_checkpoint_reclaim_slot_callback, out_name);
    return *out_name != 0;
}

/**
 * Enumeration callback that prints one checkpoint list line: index, name, level, difficulty, the
 * played time as hours, minutes and seconds, and the save date.
 *
 * @address 0x00539110
 */
uint8_t print_list_entry(int32_t index, const char *name, int32_t level_index, int32_t difficulty,
    int32_t game_time_ticks, const win32_systemtime *time, void *user_data)
{
    int32_t hours;
    int32_t minutes;
    int32_t seconds;
    int32_t remainder;
    char *level_name;

    hours = game_time_ticks / 108000;
    remainder = game_time_ticks - hours * 108000;
    minutes = remainder / 1800;
    remainder = remainder - minutes * 1800;
    seconds = remainder / 30;

    level_name = 0;
    if (0 <= level_index && level_index < k_campaign_level_count) {
        level_name = campaign_level_paths[level_index];
    }

    chimera__console_out(actor_mode_default_look_weights, (char *)"%-15s %-20s %02d:%02d:%02d", name, level_name, hours, minutes, seconds);
    return 1;
}

/**
 * Reads back "checkpoints\<name>.sav" as written by game_checkpoint_write_stats_file: a first
 * line "<level>,<difficulty>,<tick>" and two more lines of local date and time. Any out pointer
 * may be NULL. Returns the level index (as an unsigned 16-bit value; -1/0xffff if the file
 * could not be opened or scanned).
 *
 * @address 0x00538c60
 */
int16_t read_stats_file(int32_t *out_difficulty, char *name, int32_t *out_game_time, win32_systemtime *out_time)
{
    char directory[264];
    char path[264];
    void *file;
    int32_t level;
    int32_t difficulty;
    int32_t game_time_ticks;
    win32_systemtime time;

    level = -1;
    halo::saved_games::saved_game_get_directory_by_handle(saved_player_profile_slots_handle, directory);
    strcpy(path, directory);
    strcpy(path + strlen(path), name);
    strcpy(path + strlen(path), ".sav");

    file = fopen(path, network_ban_file_read_mode_string);
    if (file != 0) {
        time.year = 0; time.month = 0; time.day_of_week = 0; time.day = 0;
        time.hour = 0; time.minute = 0; time.second = 0; time.milliseconds = 0;

        fscanf((FILE *)file, "%d,%d,%d", &level, &difficulty, &game_time_ticks);
        fscanf((FILE *)file, "%hu,%hu,%hu\n", &time.month, &time.day, &time.year);
        fscanf((FILE *)file, "%hu,%hu,%hu\n", &time.hour, &time.minute, &time.second);
        fclose((FILE *)file);

        if (out_difficulty != 0) {
            *out_difficulty = difficulty;
        }
        if (out_game_time != 0) {
            *out_game_time = game_time_ticks;
        }
        if (out_time != 0) {
            *out_time = time;
        }
    }
    return (int16_t)(level & 0xffff);
}

/**
 * Enumeration callback that copies the first checkpoint name it is given into user_data, which
 * lets the caller reuse the oldest slot name.
 *
 * @address 0x00538ac0
 */
uint8_t reclaim_slot_callback(int32_t index, const char *name, int32_t level_index, int32_t difficulty,
    int32_t game_time_ticks, const win32_systemtime *time, void *user_data)
{
    char *out_name = (char *)user_data;

    if (out_name[0] == 0) {
        sprintf(out_name, "checkpoints\\%s", name);
    }
    return 1;
}

/**
 * Saves the current game state as a new checkpoint. Waits for any write in progress, finds a free
 * checkpoint slot name in the profile directory and copies the savegame.bin and .sav pair onto it.
 *
 * @address 0x00538db0
 */
uint8_t save_new(void)
{
    char directory[264];
    char target_name[256];

    while (game_state_write_in_progress != 0) {
        Sleep(0);
    }

    halo::saved_games::saved_game_get_directory_by_handle(saved_player_profile_slots_handle, directory);
    if (halo::saved_games::game_checkpoint_get_next_filename(target_name, directory) == 0) {
        return 0;
    }
    return halo::saved_games::saved_game_copy_files_to_target(directory, (char *)"savegame", target_name);
}

/**
 * Writes <profile directory>savegame.sav, the companion stats file game_checkpoint_read_stats_file
 * reads back: a first line of "<level>,<difficulty>,<tick>" (level from campaign_level_find_index_for_path) followed
 * by the current local date and time as two more comma-separated lines.
 *
 * @address 0x00538b70
 */
void write_stats_file(char *scenario_name, int32_t difficulty)
{
    char directory[264];
    char path[264];
    void *file;
    win32_systemtime now;
    int16_t level;

    halo::saved_games::saved_game_get_directory_by_handle(saved_player_profile_slots_handle, directory);
    strcpy(path, directory);
    strcpy(path + strlen(path), "savegame.sav");

    file = fopen(path, network_summary_log_mode_string);
    if (file != 0) {
        GetLocalTime((LPSYSTEMTIME)&now);
        level = campaign_level_find_index_for_path(scenario_name);
        fprintf((FILE *)file, "%d,%d,%d\n", (int32_t)level, difficulty, game_time->game_time);
        fprintf((FILE *)file, "%hu,%hu,%hu\n", now.month, now.day, now.year);
        fprintf((FILE *)file, "%hu,%hu,%hu\n", now.hour, now.minute, now.second);
        fclose((FILE *)file);
    }
}

/**
 * qsort comparator for checkpoint_file_entry records: larger kind first, then by time in the
 * direction selected by checkpoint_sort_newest_first.
 *
 * @address 0x00538e30
 */
int32_t compare(const checkpoint_file_entry *a, const checkpoint_file_entry *b)
{
    int32_t result;
    int32_t time_result;

    if (a->kind == b->kind) {
        time_result = CompareFileTime((const FILETIME *)a->last_write_time, (const FILETIME *)b->last_write_time);
        result = -time_result;
        if (checkpoint_sort_newest_first == 0) {
            return time_result;
        }
    } else {
        result = (a->kind < b->kind) * 2 - 1;
    }
    return result;
}

}  // namespace halo::saved_games::checkpoint

namespace halo::saved_games::saved_game {

/**
 * Resolves a checkpoint name and loads it: NULL/empty means "autosave", "*" instead prints the
 * full checkpoint list and loads nothing, and any other name is loaded via
 * saved_game_load_checkpoint_by_name (adding the "checkpoints\\" prefix if not already present).
 *
 * @address 0x00539290
 */
uint8_t load_checkpoint(char *name)
{
    char directory[264];
    char full_name[264];

    halo::saved_games::saved_game_get_directory_by_handle(saved_player_profile_slots_handle, directory);

    if (name == 0 || *name == 0) {
        name = (char *)"autosave";
    } else if (*name == '*') {
        halo::saved_games::game_checkpoint_enumerate_files(1, 1, halo::saved_games::game_checkpoint_print_list_entry, 0);
        return 1;
    }

    if (strstr(name, "checkpoints\\") != 0) {
        return halo::saved_games::saved_game_load_checkpoint_by_name(name);
    }
    sprintf(full_name, "checkpoints\\%s", name);
    return halo::saved_games::saved_game_load_checkpoint_by_name(full_name);
}

/**
 * FIXED (verified against 0x5391a0..0x53928d): the only argument is the stack name; EDI is saved and restored,
 *   not an input. The stats file reader takes &difficulty in EBX. A difficulty 0..3 becomes pending_difficulty
 *   (0x696564, a word). main_queue_map_change gets EAX = campaign_level_paths[level] for a level 0..9,
 *   otherwise NULL (the draft passed nothing). When the name is not "savegame" (9-byte compare) the files are
 *   copied onto "savegame" (ESI = directory, EDI = name). Returns whether the stats file gave a level.
 * Loads the checkpoint "<current profile directory><name>.sav" as the active checkpoint.
 *
 * @address 0x005391a0
 */
uint8_t load_checkpoint_by_name(char *name)
{
    char directory[264];
    char path[264];
    win32_find_dataa find_data;
    void *find_handle;
    int32_t difficulty;
    int16_t level;
    char *map_path = 0;

    halo::saved_games::saved_game_get_directory_by_handle(saved_player_profile_slots_handle, directory);
    sprintf(path, "%s%s.sav", directory, name);
    find_handle = FindFirstFileA(path, (LPWIN32_FIND_DATAA)&find_data);
    if (find_handle == (void *)0xffffffff) {
        return 0;
    }
    FindClose(find_handle);

    level = halo::saved_games::game_checkpoint_read_stats_file(&difficulty, name, 0, 0);
    if (level == -1) {
        return 0;
    }
    if ((int16_t)difficulty >= 0 && (int16_t)difficulty < 4) {
        pending_difficulty = (int16_t)difficulty;
    }
    if (level >= 0 && level < 10) {
        map_path = campaign_level_paths[level];
    }
    main_queue_map_change(map_path);
    if (memcmp(name, "savegame", 9) != 0) {
        halo::saved_games::saved_game_copy_files_to_target(directory, name, (char *)"savegame");
    }
    return 1;
}

}  // namespace halo::saved_games::saved_game
