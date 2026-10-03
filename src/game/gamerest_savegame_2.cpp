#include "halo/game/gamerest_savegame.hpp"
#include "halo/saved_games/api.hpp"
#include "halo/game/api.hpp"

extern "C" {
extern char saved_game_root_path[];
extern file_reference_record savegame_index_file;
extern network_mutex_record *savegame_index_mutex;
}

namespace halo::game {

/**
 * Removes save-slot `slot` from the index file: every 0x206-byte record after it is read and written back one record
 * earlier, then the file is truncated by one record. Returns 1 on success, 0 otherwise.
 *
 * @address 0x53e4a0
 */
uint8_t SaveGameIndex::remove_slot(uint16_t slot)
{
    file_reference_record *ref = &savegame_index_file;
    uint8_t record[0x206];
    uint32_t size;
    uint32_t read_offset, write_offset;
    uint8_t result = 0;
    uint32_t wait_result = WaitForSingleObject(savegame_index_mutex->handle, 5000);

    if (wait_result != 0 && wait_result != 0x80) {
        return 0;
    }

    {
        uint32_t *raw = (uint32_t *)ref;
        int32_t i;
        for (i = 0; i < 0x43; i++) {
            raw[i] = 0;
        }
        raw[0] = 0x66696c6f;
        *(uint16_t *)((uint8_t *)ref + 6) = 2;
        if ((*((uint8_t *)ref + 4) & 1) != 0) {
            halo::saved_games::path_remove_last_component((char *)ref + 8);
        }
        halo::saved_games::path_append_component((char *)ref + 8, saved_game_root_path);
        *((uint8_t *)ref + 4) |= 1;
    }

    if (halo::saved_games::file_reference_get_size_by_path(ref, &size) != 0) {
        write_offset = (uint32_t)slot * 0x206;
        read_offset = write_offset + 0x206;
        if (read_offset <= size && halo::saved_games::file_reference_open(ref, 3) != 0) {
            result = halo::saved_games::file_reference_seek((int32_t)write_offset, ref);
            if (result == 1) {
                for (; read_offset < size; read_offset += 0x206, write_offset += 0x206) {
                    if (halo::saved_games::file_reference_seek((int32_t)read_offset, ref) == 0 ||
                        halo::saved_games::file_reference_read(ref, record, 0x206) == 0 ||
                        halo::saved_games::file_reference_seek((int32_t)write_offset, ref) == 0 ||
                        halo::saved_games::file_reference_write(ref, record, 0x206) == 0) {
                        result = 0;
                        goto close_file;
                    }
                }
                result = halo::saved_games::file_reference_set_length((int32_t)(size - 0x206), ref);
            } else if (result != 0) {
                result = halo::saved_games::file_reference_set_length((int32_t)(size - 0x206), ref);
            }
        close_file:
            if (halo::saved_games::file_reference_close(ref) == 0) {
                result = 0;
            }
        }
    }

    ReleaseMutex(savegame_index_mutex->handle);
    return result;
}

}  // namespace halo::game

namespace halo::game {

/**
 * C entry point for halo::game::SaveGameIndex::remove_slot; forwards to the C++ implementation.
 * register convention: a slot index in the low 16 bits of the incoming value (Ghidra's own
 * `ushort param_1`, a genuine stack parameter).
 *
 * @address 0x53e4a0
 */
uint8_t savegame_index_remove_slot(uint16_t slot)
{
    return halo::game::SaveGameIndex::remove_slot(slot);
}

}
