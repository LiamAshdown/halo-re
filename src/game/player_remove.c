// player_remove  (Ghidra: player_remove, already named)
// address 0x473bb0, size 150 bytes
// name confidence: 0.5   rewrite confidence: 0.4
// evidence: out/phase4/game_functions.md ("Full removal of a player: frees network history
//   buffers, deletes the player datum, and clears its roster slot"); types/game.h
//   update_client_queues (0x006f7ed0) / update_server_queues (0x006f1d90) share the player
//   data_array's 16-slot index space, so this deletes the datum at the SAME index out of both
//   queue arrays as the player itself, not a datum keyed by its own handle; player_profile_cache
//   / player_profile_cache_count (game_engine_player_profile_cache_find.c, this module).
// objdump -d -M intel --start-address=0x473bb0 --stop-address=0x473c50 bin/halo.exe pins every
// register, including that player_delete's "machine_index" argument at this call site is
// actually the low byte of player::unknown_64, sign-extended -- not a real machine index.
// register convention: player_handle in EAX (unaff_EAX/in_EAX in Ghidra's read of it).
//   // blam-cc: EAX -> player_handle
//
// UNSURE: game_engine_player_profile_cache_find can return -1 (not found), and this function
// uses that result unchecked as `player_profile_cache[index].in_use = 0` -- with index -1 that
// is `*(uint8_t *)((uint8_t *)player_profile_cache - 0x30) = 0`, a write just before the array.
// Transcribed exactly; not "fixed".
// UNSURE: why player::unknown_64's low byte is passed to player_delete as its machine_index
// argument is not recoverable from this function alone.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"

extern data_array *player_data;               // 0x0087a480
extern data_array *update_client_queues;      // 0x006f7ed0
extern data_array *update_server_queues;      // 0x006f1d90
extern int16_t network_game_mode;             // 0x00719720
extern player_profile player_profile_cache[16]; // 0x006b0b88
extern int32_t player_profile_cache_count;    // 0x006f1d34

extern void datum_delete(data_array *array, datum_index handle); // 0x4d0510, blam-cc: EAX -> array,
    // EDX -> handle
extern void player_delete(uint32_t machine_index, datum_index player_handle); // this batch, 0x473ae0
extern void network_index_cache_remove(void *table, datum_index player_handle); // 0x4e9d40, networking module,
    // not in this batch; blam-cc: EAX -> table, ESI -> player_handle; UNSURE full behavior
extern int32_t game_engine_player_profile_cache_find(datum_index player_handle); // this module,
    // 0x466e80, blam-cc: ESI -> player_handle
extern void *GlobalFree(void *handle); // Win32

// Deletes this player's slot out of update_client_queues, and -- only while hosting -- also
// frees update_server_queues's matching slot's queue storage and deletes that slot too (both
// queue arrays are indexed the same way as player_data, so the player's own handle doubles as
// the datum handle into each). Passes the low byte of player::unknown_64 to player_delete as its
// machine_index (see UNSURE above), then runs the two networking-side cleanups and clears the
// player's player-profile-cache entry, decrementing the cache count.
void player_remove(datum_index player_handle)
    // blam-cc: EAX -> player_handle
{
    player *p;
    int32_t index;
    int32_t profile_index;
    update_server_queue *server_entry;

    index = (int32_t)(uint16_t)player_handle;
    p = (player *)((uint8_t *)player_data->data + (uint32_t)index * player_data->size);

    datum_delete(update_client_queues, player_handle);

    if (network_game_mode == 2) {
        server_entry = &((update_server_queue *)update_server_queues->data)[index];
        GlobalFree(server_entry->queue.queue.storage);
        server_entry->queue.queue.storage = (void *)0;
        datum_delete(update_server_queues, player_handle);
    }

    player_delete((uint32_t)(int8_t)*((uint8_t *)p + 0x64), player_handle); // UNSURE: see header

    network_index_cache_remove((void *)0x687500, player_handle);
    profile_index = game_engine_player_profile_cache_find(player_handle);
    player_profile_cache[profile_index].in_use = 0; // UNSURE: unchecked -1, see header
    player_profile_cache_count = player_profile_cache_count - 1;
}

#if 0
Original Ghidra decompilation (0x473bb0), from tools/pack.py 0x473bb0:

void player_remove(void)

{
  uint in_EAX;
  int iVar1;
  int iVar2;

  datum_delete();
  if (DAT_00719720 == 2) {
    iVar2 = (in_EAX & 0xffff) * 100;
    iVar1 = *(int *)(DAT_006f1d90 + 0x34);
    GlobalFree(*(HGLOBAL *)(iVar2 + 0x3c + iVar1));
    *(undefined4 *)(iVar2 + iVar1 + 0x3c) = 0;
    datum_delete();
  }
  player_delete();
  FUN_004e9d40();
  iVar1 = FUN_00466e80();
  *(undefined1 *)(&DAT_006b0b88 + iVar1 * 0xc) = 0;
  DAT_006f1d34 = DAT_006f1d34 + -1;
  return;
}
#endif
