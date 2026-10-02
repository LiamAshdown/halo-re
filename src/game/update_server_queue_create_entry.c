// update_server_queue_create_entry  (Ghidra: FUN_00472c90; renamed, no established name)
// address 0x472c90, size 40 bytes
// name confidence: 0.4   rewrite confidence: 0.75
// evidence: out/phase4/game_functions.md ("Creates (or reinitializes) a single entry in the
// server update-queue datum array"); update_server_dispose.c for the sibling
// datum_new_at_index_with_salt / player_update_queue_create pairing.
// CORRECTED by review: the first pass left both calls argument-less. objdump
// (--start-address=0x472c90 --stop-address=0x472cb8) shows exactly which registers carry what:
//     mov edx,ds:0x6f1d90 ; push esi ; call 0x4d03d0     <- EDX = the array; EAX is NOT set,
//                                                           so this function forwards its own
//                                                           incoming EAX as the requested handle
//                                                           (datum_new_at_index_with_salt's
//                                                           blam-cc is EAX -> handle, EDX -> array)
//     and eax,0xffff ; imul eax,eax,0x64 ; lea esi,[eax+edx+0x28] ; call 0x479f40
//                                                        <- ESI = &entry->queue, matching
//                                                           player_update_queue_create's ESI
//   ("push esi" at 0x472c96 is the callee-save pair for the "pop esi" at 0x472cb6, not an
//    argument.) The 0x64 stride and the +0x28 offset are types/game.h's update_server_queue.
// register convention: EAX -> requested_handle.
//   // blam-cc: EAX -> requested_handle

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern data_array *update_server_queues; // 0x006f1d90

extern datum_index datum_new_at_index_with_salt(datum_index requested_handle,
    data_array *array); // 0x4d03d0, memory module; blam-cc: EAX -> handle, EDX -> array
extern void player_update_queue_create(player_update_queue *queue); // 0x479f40, this module;
    // blam-cc: ESI -> queue

// blam-cc: EAX -> requested_handle
// Creates the update_server_queue datum at `requested_handle`'s index/salt and constructs its
// embedded player_update_queue in place.
void update_server_queue_create_entry(datum_index requested_handle)
{
    datum_index handle = datum_new_at_index_with_salt(requested_handle, update_server_queues);
    update_server_queue *entry = (update_server_queue *)
        ((uint8_t *)update_server_queues->data + ((uint32_t)handle & 0xffff) * sizeof(update_server_queue));
    player_update_queue_create(&entry->queue);
}

#if 0
Original Ghidra decompilation (0x472c90), from tools/pack.py 0x472c90:

void FUN_00472c90(void)

{
  datum_new_at_index_with_salt();
  FUN_00479f40();
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
