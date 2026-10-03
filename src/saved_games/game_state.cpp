#include "halo/hs/script_globals.hpp"
#include "halo/main/main_globals_fields.hpp"
#include "crt.h"
#include "win32.h"
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "game.h"
#include "networking.h"
#include "interface.h"
#include "saved_games.h"
#include "halo/saved_games/saved_games.hpp"
#include "halo/saved_games/layout.hpp"
#include "halo/cache/api.hpp"
#include "halo/memory/api.hpp"
#include "halo/scenario/api.hpp"
#include "halo/main/api.hpp"
#include "halo/saved_games/api.hpp"
#include "halo/shell/api.hpp"
#include "halo/hs/api.hpp"

static_assert(sizeof(data_array) == halo::saved_games::k_game_state_block_header_size);
static_assert(sizeof(memory_pool) == halo::saved_games::k_game_state_block_header_size);
static_assert(sizeof(game_state_header) == k_game_state_header_size);

extern "C" {
extern game_time_globals *game_time;
extern int32_t game_state_revert_time;
extern uint8_t *game_state_snapshot_source;
extern uint8_t game_state_write_buffer_allocated;
extern uint32_t game_state_size;
extern uint8_t *game_state_write_buffer;
extern char profile_directory[0x105];
extern char game_state_persistent_storage_path[0x100];
extern char game_state_core_directory[0x100];
extern uint8_t game_state_write_in_progress;
extern void *game_state_write_event;
extern game_state_header *game_state_header_ptr;
extern uint8_t game_state_header_valid;
extern uint8_t game_state_revert_available;
extern int16_t local_player_count;
extern uint32_t cache_file_current_header_crc32;
extern uint32_t game_state_crc;
extern void *game_state_persistent_storage;
extern uint8_t game_state_persistent_storage_created;
extern game_state_proc game_state_after_load_procs[k_game_state_after_load_proc_count];
extern uint16_t game_time_force_single_tick;
extern int16_t pending_difficulty;
extern uint8_t *game_state_base;
extern game_state_proc game_state_revert_proc;
extern int32_t game_state_cursor;
extern int32_t saved_player_profile_slots_handle;
extern char *strcpy(char *dest, const char *source);
extern uint32_t strlen(const char *str);
extern void *memset(void *dest, int32_t value, uint32_t count);
extern uint16_t split_screen_quit_prompt_string;
extern uint8_t network_join_error_reason;
extern game_state_proc game_state_before_save_proc;
extern uint8_t game_state_write_is_checkpoint;
extern void *memcpy(void *dest, const void *src, uint32_t count);
extern uint8_t game_state_write_completed;
}

namespace halo::saved_games::game_state {

/**
 * After-load callback (entry 10 of the 13 game-state callbacks). Stores the current game tick in
 * game_state_revert_time so a later revert can restore the time.
 *
 * @address 0x005385d0
 */
void after_load_restore_time(void)
{
    game_state_revert_time = game_time->game_time;
    game_time->paused = 0;
    if (game_time->initialized != 0) {
        game_time->active = 1;
    }
}

/**
 * Allocates the game-state working buffer (cpu_size + extra_size bytes), builds the
 * savegame.bin and core-directory path strings under the active profile directory, and
 * starts the background save-writer thread. Returns map_memory, the source the buffer is
 * snapshotted from.
 *
 * @address 0x005385f0
 */
void *allocate_buffer(int32_t cpu_size, int32_t extra_size)
{
    void *base;

    base = halo::cache::globals().map_memory;
    game_state_snapshot_source = (uint8_t *)halo::cache::globals().map_memory;
    game_state_size = cpu_size + extra_size;
    game_state_write_buffer_allocated = 1;
    game_state_write_buffer = (uint8_t *)GlobalAlloc(0, game_state_size);
    _snprintf(game_state_persistent_storage_path, k_path_maximum_length, "%s\\%s", profile_directory, k_game_state_file_name);
    _snprintf(game_state_core_directory, k_path_maximum_length, "%s\\%s", profile_directory, "core");
    game_state_write_in_progress = 0;
    game_state_write_event = CreateEventA(0, 0, 0, 0);
    _beginthread((void (*)(void *))halo::saved_games::game_state_save_thread_proc, 0x1000, 0);
    return base;
}

/**
 * Fills the 0x14c-byte game-state header: allocation checksum, running scenario path, build
 * version string, local player count, difficulty and map checksum. Marks the header valid and
 * resets the revert time.
 *
 * @address 0x00538000
 */
void build_header(void)
{
    game_state_header *header;
    uint32_t *zero;
    int32_t count;
    char *src;
    char *dst;

    game_state_header_valid = 1;
    game_state_revert_available = 0;
    game_state_revert_time = -1;
    header = game_state_header_ptr;
    zero = (uint32_t *)header;
    for (count = k_game_state_header_size / sizeof(uint32_t); count != 0; count = count - 1) {
        *zero = 0;
        zero = zero + 1;
    }

    src = halo::cache::globals().tag_instances[(int16_t)halo::scenario::globals().scenario_index].path;
    dst = header->scenario_name;
    do {
        *dst = *src;
        src = src + 1;
        dst = dst + 1;
    } while (*(dst - 1) != '\0');

    header->build_version[0] = '0'; header->build_version[1] = '1'; header->build_version[2] = '.';
    header->build_version[3] = '0'; header->build_version[4] = '0'; header->build_version[5] = '.';
    header->build_version[6] = '1'; header->build_version[7] = '0'; header->build_version[8] = '.';
    header->build_version[9] = '0'; header->build_version[10] = '6'; header->build_version[11] = '2';
    header->build_version[12] = '1'; header->build_version[13] = '\0';

    header->local_player_count = local_player_count;
    header->difficulty = halo::main::globals().game_globals->difficulty;
    header->map_checksum = cache_file_current_header_crc32;
    header->allocation_checksum = game_state_crc;
}

/**
 * Creates savegame.bin at game_state_persistent_storage_path and pre-sizes it to 0x480000 bytes.
 * Called once while the game state is being set up.
 *
 * @address 0x00538690
 */
void create_persistent_storage_file(void)
{
    game_state_persistent_storage = CreateFileA(game_state_persistent_storage_path, win32::k_generic_read_write, win32::k_file_share_none, 0,
        win32::k_open_always, win32::k_file_flag_sequential_scan, 0);
    if (game_state_persistent_storage != win32::invalid_handle() &&
        SetFilePointer(game_state_persistent_storage, k_game_state_file_size, 0, win32::k_file_begin) != win32::k_invalid_set_file_pointer &&
        SetEndOfFile(game_state_persistent_storage) != 0) {
        game_state_persistent_storage_created = 1;
        return;
    }
    halo::shell::shell_display_fatal_error_dialog(k_error_game_state_io_string, k_error_game_state_io_title, 1);
}

/**
 * Calls each of the 13 registered game-state after-load callbacks in table order. Shared by the
 * load and revert paths.
 *
 * @address 0x00537f70
 */
void dispatch_load_callbacks(void)
{
    game_state_proc *proc;
    int32_t count;

    proc = game_state_after_load_procs;
    count = k_game_state_after_load_proc_count;
    do {
        (*proc)();
        proc = proc + 1;
        count = count - 1;
    } while (count != 0);
}

/**
 * Loads the saved game state into the arena. Reads the header into a local buffer, validates the
 * crc and the build and scenario fields, and on success applies the pending difficulty and
 * dispatches the after-load callbacks.
 *
 * @address 0x00538280
 */
void load_checkpoint(void)
{
    game_state_header header;

    if (game_time_force_single_tick != 0) {
        return;
    }
    if (halo::saved_games::saved_game_validate_crc(k_game_state_size, k_game_state_header_size, (uint8_t *)&header,
            &header.file_checksum, 0) == 0) {
        return;
    }
    if (halo::saved_games::saved_game_verify_version_and_checksum(&header, 0) == 0) {
        return;
    }
    if (pending_difficulty != header.difficulty) {
        return;
    }

    game_state_revert_proc();
    halo::saved_games::game_state_read_persistent_storage_block(k_game_state_size, game_state_base);
    halo::main::globals().game_globals->difficulty = pending_difficulty;
    halo::saved_games::game_state_dispatch_load_callbacks();
    halo::saved_games::game_state_perform_save(0);
}

/**
 * Reads and validates the profile file "<core directory>\<name>" as a game state header; on a
 * good header (version, checksum, scenario and player-count match), reverts to it, reads the
 * full body into the game-state arena and dispatches the load callbacks, logging success. On
 * any failure, logs that the file couldn't be opened.
 *
 * @address 0x00538390
 */
void load_core(char *name)
{
    game_state_header header;

    if (halo::saved_games::game_state_read_profile_header(name, k_game_state_header_size, &header) != 0 &&
        halo::saved_games::saved_game_verify_version_and_checksum(&header, 1) != 0) {
        game_state_revert_proc();
        halo::saved_games::game_state_read_profile_file(name, k_game_state_size, game_state_base);
        halo::main::console_print_error_va(0, "loaded '%s'", name);
        halo::saved_games::game_state_dispatch_load_callbacks();
        return;
    }
    halo::main::console_print_error_va(0, "couldn't open '%s'", name);
}

/**
 * Carves a new data_array header + storage block out of the game-state arena, folding the
 * block's total size into the running game-state crc, and initializes the header exactly as
 * data_new does. Returns a pointer to the new header.
 *
 * @address 0x005380d0
 */
data_array *make(char *name, int16_t maximum_count, int16_t element_size)
{
    data_array *array;
    int32_t block_size;
    uint8_t *zero;
    int32_t i;

    block_size = (int32_t)maximum_count * (int32_t)element_size + k_game_state_block_header_size;
    array = (data_array *)(game_state_cursor + game_state_base);
    game_state_cursor = game_state_cursor + block_size;
    halo::memory::crc32_update(&game_state_crc, (uint8_t *)&block_size, 4);

    zero = (uint8_t *)array;
    for (i = k_game_state_block_header_size / 4; i != 0; i = i - 1) {
        zero[0] = 0;
        zero[1] = 0;
        zero[2] = 0;
        zero[3] = 0;
        zero = zero + 4;
    }
    strncpy(array->name, name, sizeof(array->name) - 1);
    array->maximum_count = maximum_count;
    array->size = element_size;
    array->signature = k_data_array_signature;
    array->data = (uint8_t *)array + k_game_state_block_header_size;
    array->valid = 0;
    return array;
}

/**
 * Carves a new memory_pool header + storage block out of the game-state arena, folding the
 * block's total size into the running game-state crc. Returns a pointer to the new pool.
 *
 * @address 0x00538150
 */
memory_pool *new_pool(char *name, int32_t pool_size)
{
    memory_pool *pool;
    int32_t block_size;
    uint32_t *zero;
    int32_t i;

    block_size = pool_size + k_game_state_block_header_size;
    pool = (memory_pool *)(game_state_cursor + game_state_base);
    game_state_cursor = game_state_cursor + block_size;
    halo::memory::crc32_update(&game_state_crc, (uint8_t *)&block_size, 4);

    zero = (uint32_t *)pool;
    for (i = k_game_state_block_header_size / 4; i != 0; i = i - 1) {
        *zero = 0;
        zero = zero + 1;
    }
    pool->signature = k_memory_pool_signature;
    strncpy(pool->name, name, sizeof(pool->name) - 1);
    pool->base = (uint8_t *)pool + k_game_state_block_header_size;
    pool->first_block = 0;
    pool->last_block = 0;
    pool->size = pool_size;
    pool->free_bytes = pool_size;
    return pool;
}

/**
 * Opens (creating and pre-sizing to k_game_state_file_size if necessary) the savegame.bin file
 * under either the caller-supplied directory (name) or, when name is NULL, the current player
 * profile's own directory. Returns the file handle, or (void *)-1 on failure.
 *
 * @address 0x005398e0
 */
void *open_persistent_storage(char *name)
{
    char path[0x120];
    char *end;
    void *file;
    uint32_t file_size;
    uint32_t bytes_written;
    uint32_t zero_block[k_game_state_file_initial_block / 4];
    char delete_path[0x120];

    if (name == 0) {
        if (halo::saved_games::saved_game_get_directory_by_handle(saved_player_profile_slots_handle, path) == 0) {
            return win32::invalid_handle();
        }
        halo::saved_games::saved_game_get_directory_by_handle(saved_player_profile_slots_handle, path);
    } else {
        strcpy(path, name);
    }

    end = path + strlen(path);
    strcpy(end, "savegame.bin");

    file = CreateFileA(path, win32::k_generic_read_write, win32::k_file_share_none, 0, win32::k_open_always, 0, 0);
    if (file == win32::invalid_handle()) {
        return win32::invalid_handle();
    }

    file_size = GetFileSize(file, 0);
    if (file_size != k_game_state_file_size) {
        memset(zero_block, 0, sizeof(zero_block));
        if (WriteFile(file, zero_block, k_game_state_file_initial_block, (LPDWORD)&bytes_written, 0) == 0 ||
            bytes_written != k_game_state_file_initial_block ||
            SetFilePointer(file, k_game_state_file_size, 0, win32::k_file_begin) == win32::k_invalid_set_file_pointer ||
            SetEndOfFile(file) == 0) {
            halo::shell::shell_display_fatal_error_dialog(k_error_game_state_io_string, k_error_game_state_io_title, 1);
            if (halo::saved_games::saved_game_get_directory_by_handle(saved_player_profile_slots_handle, delete_path) != 0) {
                DeleteFileA(delete_path);
            }
            CloseHandle(file);
            return win32::invalid_handle();
        }
    }
    return file;
}

/**
 * Reverts to the last queued snapshot by calling the revert procedure and then the after-load
 * callbacks.
 *
 * @address 0x00538200
 */
void perform_revert(void)
{
    if (game_state_revert_available == 0 && halo::hs::fields::recover_saved_games_hack == 0) {
        split_screen_quit_prompt_string = k_word_none;
        network_join_error_reason = 0;
        halo::main::fields::reset_map = 1;
        halo::main::fields::lost_map = 0;
        return;
    }

    while (game_state_write_in_progress != 0) {
        Sleep(0);
    }

    game_state_revert_proc();
    halo::saved_games::game_state_read_persistent_storage();
    halo::saved_games::game_state_dispatch_load_callbacks();
}

/**
 * Top-level save entry point. Runs the before-save callback, queues the asynchronous write and
 * records whether it succeeded as the revert-available flag.
 *
 * @address 0x005381c0
 */
void perform_save(uint8_t is_checkpoint)
{
    uint8_t result;

    game_state_before_save_proc();
    halo::main::fields::time_is_running = 0;
    halo::main::fields::reset_frame_timers = 0;
    result = halo::saved_games::game_state_queue_write(is_checkpoint);
    game_state_revert_available = result != 0;
    halo::main::fields::reset_frame_timers = 1;
}

/**
 * Copies the game-state arena into the write buffer and wakes the save thread. final_flag marks
 * a checkpoint write, which also rotates the autosave and writes the stats file. Returns 1 when the
 * write was queued.
 *
 * @address 0x00538700
 */
uint8_t queue_write(uint8_t final_flag)
{
    while (game_state_write_in_progress != 0) {
        Sleep(0);
    }

    memcpy(game_state_write_buffer, game_state_snapshot_source, game_state_size);

    game_state_write_is_checkpoint = final_flag;
    SetEvent(game_state_write_event);
    return 1;
}

/**
 * Reads and crc-validates the persistent checkpoint (savegame.bin) header without applying it,
 * and returns its difficulty and scenario name for display. On failure, returns a default
 * difficulty of 1 and an empty name.
 *
 * @address 0x00538320
 */
uint8_t read_checkpoint_summary(uint8_t *corrupt_flag, int16_t *out_difficulty, char *out_scenario_name)
{
    game_state_header header;

    if (halo::saved_games::saved_game_validate_crc(k_game_state_size, k_game_state_header_size, (uint8_t *)&header,
            &header.file_checksum, corrupt_flag) != 0) {
        *out_difficulty = header.difficulty;
        strcpy(out_scenario_name, header.scenario_name);
        return 1;
    }
    *out_difficulty = 1;
    *out_scenario_name = 0;
    return 0;
}

/**
 * Waits for any write in progress, then reads game_state_size bytes from the open persistent
 * storage file into the game-state arena. A failed read raises a fatal error dialog.
 *
 * @address 0x00539330
 */
uint8_t read_persistent_storage(void)
{
    uint32_t bytes_read;

    while (game_state_write_in_progress != 0) {
        Sleep(0);
    }

    if (SetFilePointer(game_state_persistent_storage, 0, 0, win32::k_file_begin) != win32::k_invalid_set_file_pointer &&
        ReadFile(game_state_persistent_storage, game_state_snapshot_source, game_state_size, (LPDWORD)&bytes_read, 0) != 0 &&
        bytes_read == game_state_size) {
        return 1;
    }
    halo::shell::shell_display_fatal_error_dialog(k_error_game_state_io_string, k_error_game_state_io_title, 1);
    return 0;
}

/**
 * Opens the current profile's savegame.bin and reads size bytes from its start into buffer.
 * On any failure, raises a fatal error dialog and attempts to delete the current profile's
 * save file.
 *
 * @address 0x00539850
 */
void read_persistent_storage_block(int32_t size, void *buffer)
{
    void *file;
    uint32_t bytes_read;
    char directory[264];

    file = halo::saved_games::game_state_open_persistent_storage(0);
    if (file == win32::invalid_handle()) {
        return;
    }

    if (SetFilePointer(file, 0, 0, win32::k_file_begin) == win32::k_invalid_set_file_pointer ||
        ReadFile(file, buffer, size, (LPDWORD)&bytes_read, 0) == 0 ||
        bytes_read != (uint32_t)size) {
        halo::shell::shell_display_fatal_error_dialog(k_error_game_state_io_string, k_error_game_state_io_title, 1);
        if (halo::saved_games::saved_game_get_directory_by_handle(saved_player_profile_slots_handle, directory) != 0) {
            DeleteFileA(directory);
        }
    }
    CloseHandle(file);
}

/**
 * Opens "<game_state_core_directory>\<name>" and reads exactly size bytes into buffer,
 * raising a fatal error dialog (string 0x8b, title 0x8c) if the open or the read fails.
 *
 * @address 0x005394e0
 */
void read_profile_file(char *name, int32_t size, void *buffer)
{
    char path[1024];
    void *file;
    uint32_t bytes_read;

    sprintf(path, "%s\\%s", game_state_core_directory, name);
    file = CreateFileA(path, win32::k_generic_read, win32::k_file_share_none, 0, win32::k_open_existing, win32::k_file_attribute_normal, 0);
    if (file == win32::invalid_handle() ||
        ReadFile(file, buffer, size, (LPDWORD)&bytes_read, 0) == 0 ||
        bytes_read != (uint32_t)size) {
        halo::shell::shell_display_fatal_error_dialog(k_error_game_state_io_string, k_error_game_state_io_title, 1);
    }
    CloseHandle(file);
}

/**
 * Opens "<game_state_core_directory>\<name>" and reads exactly size bytes into buffer,
 * returning success without raising a fatal error.
 *
 * @address 0x00539460
 */
uint8_t read_profile_header(char *name, int32_t size, void *buffer)
{
    char path[1024];
    void *file;
    uint32_t bytes_read;
    uint8_t result;

    result = 0;
    sprintf(path, "%s\\%s", game_state_core_directory, name);
    file = CreateFileA(path, win32::k_generic_read, win32::k_file_share_none, 0, win32::k_open_existing, win32::k_file_attribute_normal, 0);
    if (file != win32::invalid_handle()) {
        if (ReadFile(file, buffer, size, (LPDWORD)&bytes_read, 0) != 0 && bytes_read == (uint32_t)size) {
            result = 1;
        }
    }
    CloseHandle(file);
    return result;
}

/**
 * Body of the asynchronous game-state writer thread. Writes the snapshot in 0x4000-byte chunks
 * with its crc, then completes the checkpoint bookkeeping when the write was a checkpoint.
 *
 * @address 0x00538980
 */
void save_thread_proc(void)
{
    uint8_t is_checkpoint;
    int32_t remaining;
    uint32_t chunk;
    uint32_t bytes_written;
    char directory[264];

    while (1) {
        WaitForSingleObject(game_state_write_event, win32::k_infinite);
        is_checkpoint = game_state_write_is_checkpoint;
        remaining = game_state_size;
        game_state_write_in_progress = 1;
        game_state_write_is_checkpoint = 0;

        if (SetFilePointer(game_state_persistent_storage, 0, 0, 0) != k_datum_index_none) {
            while (0 < remaining) {
                chunk = remaining;
                if (k_game_state_write_chunk_size - 1 < remaining) {
                    chunk = k_game_state_write_chunk_size;
                }
                WriteFile(game_state_persistent_storage, game_state_write_buffer + (game_state_size - remaining),
                    chunk, (LPDWORD)&bytes_written, 0);
                remaining = remaining - bytes_written;
                Sleep(0);
            }
        }

        if (remaining != 0) {
            halo::shell::shell_display_fatal_error_dialog(k_error_game_state_io_string, k_error_game_state_io_title, 1);
            game_state_write_in_progress = 0;
            continue;
        }

        if (is_checkpoint != 0) {
            halo::saved_games::saved_game_get_directory_by_handle(saved_player_profile_slots_handle, directory);
            halo::saved_games::game_state_write_persistent_storage(&((game_state_header *)game_state_write_buffer)->file_checksum,
                game_state_write_buffer, k_game_state_header_size, k_game_state_size);
            halo::saved_games::game_checkpoint_write_stats_file(((game_state_header *)game_state_write_buffer)->scenario_name,
                ((game_state_header *)game_state_write_buffer)->difficulty);
            halo::saved_games::saved_game_copy_files_to_target(directory, (char *)"checkpoints\\autosave", (char *)"checkpoints\\autosave1");
            halo::saved_games::saved_game_copy_files_to_target(directory, (char *)"savegame", (char *)"checkpoints\\autosave");
        }

        game_state_write_completed = 1;
        game_state_write_in_progress = 0;
    }
}

/**
 * Reserves the game-state arena and its 0x14c-byte header. Allocates the buffer with the fixed
 * CPU and extra sizes, folds the header size into the allocation crc and advances the cursor.
 *
 * @address 0x00537f90
 */
void startup(void)
{
    int32_t header_size;
    uint8_t *header_base;

    game_state_crc = k_crc32_seed;
    game_state_base = (uint8_t *)halo::saved_games::game_state_allocate_buffer(k_game_state_cpu_size, k_game_state_extra_size);
    halo::saved_games::game_state_create_persistent_storage_file();
    header_base = game_state_cursor + game_state_base;
    game_state_cursor = game_state_cursor + k_game_state_header_size;
    header_size = k_game_state_header_size;
    halo::memory::crc32_update(&game_state_crc, (uint8_t *)&header_size, 4);
    game_state_header_ptr = (game_state_header *)header_base;
}

/**
 * Crcs the whole total_size-byte image into *crc_slot (a field inside the header at the start
 * of buffer, zeroed first so it doesn't crc its own old value), then writes the file in two
 * passes: the full image with the header portion zeroed, then a second, header-only write that
 * patches the real header back in -- so a write interrupted after the first pass leaves an
 * on-disk header that still fails validation rather than a half-written one. Restores buffer's
 * header bytes in memory afterward regardless of success. Raises a fatal error dialog and
 * attempts to delete the (default profile) save file if either write fails.
 *
 * @address 0x00539710
 */
void write_persistent_storage(uint32_t *crc_slot, uint8_t *buffer, int32_t header_size, int32_t total_size)
{
    void *file;
    uint32_t running_crc;
    uint8_t header_backup[2052];
    uint32_t bytes_written;
    char directory[264];

    file = halo::saved_games::game_state_open_persistent_storage(0);
    if (file == win32::invalid_handle()) {
        return;
    }

    *crc_slot = 0;
    running_crc = k_crc32_seed;
    halo::memory::crc32_update(&running_crc, buffer, total_size);
    *crc_slot = running_crc;

    memcpy(header_backup, buffer, header_size);
    memset(buffer, 0, header_size);

    if (SetFilePointer(file, 0, 0, win32::k_file_begin) == win32::k_invalid_set_file_pointer ||
        WriteFile(file, buffer, total_size, (LPDWORD)&bytes_written, 0) == 0 ||
        bytes_written != (uint32_t)total_size ||
        SetFilePointer(file, 0, 0, win32::k_file_begin) == win32::k_invalid_set_file_pointer ||
        WriteFile(file, header_backup, header_size, (LPDWORD)&bytes_written, 0) == 0 ||
        bytes_written != (uint32_t)header_size) {
        halo::shell::shell_display_fatal_error_dialog(k_error_game_state_io_string, k_error_game_state_io_title, 1);
        if (halo::saved_games::saved_game_get_directory_by_handle(saved_player_profile_slots_handle, directory) != 0) {
            DeleteFileA(directory);
        }
    }

    memcpy(buffer, header_backup, header_size);
    CloseHandle(file);
}

/**
 * Ensures game_state_core_directory exists, then writes size bytes of buffer to
 * "<game_state_core_directory>\<name>", creating or truncating the file. Returns success.
 *
 * @address 0x005393d0
 */
uint8_t write_profile_file(int32_t size, char *name, const void *buffer)
{
    char path[1024];
    void *file;
    uint32_t bytes_written;
    uint8_t result;

    result = 0;
    CreateDirectoryA(game_state_core_directory, 0);
    sprintf(path, "%s\\%s", game_state_core_directory, name);
    file = CreateFileA(path, win32::k_generic_write, win32::k_file_share_none, 0, win32::k_create_always, win32::k_file_attribute_normal, 0);
    if (file != win32::invalid_handle()) {
        if (WriteFile(file, buffer, size, (LPDWORD)&bytes_written, 0) != 0 && bytes_written == (uint32_t)size) {
            result = 1;
        }
    }
    CloseHandle(file);
    return result;
}

}  // namespace halo::saved_games::game_state
