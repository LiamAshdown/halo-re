// update_server_dispose  (Ghidra: FUN_00472b70; renamed to match its sibling update_client_dispose)
// address 0x472b70, size 280 bytes
// name confidence: 0.3   rewrite confidence: 0.3
// evidence: out/phase4/game_functions.md ("Tears down the server update queue's datum array and
// cascades into disposing the client update queue"); modules.json ("player action queues -
// 'update server queues'/'update client queues' ring buffer"); types/game.h update_server_queue,
// player_update_queue; the sibling update_client_dispose.c (0x472fa0), which this function calls
// at its own tail and which shares the identical re-synchronize-from-players idiom.
//
// CORRECTED (this rewrite) against
//   objdump -d -M intel --start-address=0x472b70 --stop-address=0x472c90 bin/halo.exe
// Ghidra prints 12 "WARNING: Removing unreachable block" lines and collapses the whole function
// to `data_delete_all(); data_delete_all(); while (data_iterator_next()) FUN_00479f40();`. As with
// update_client_dispose.c, the real behavior is a reset-and-rebuild, not a teardown: after
// resetting update_server_queues (data_delete_all, twice, both against the same array), it walks
// every live player_data entry and, for each one whose own index/salt fits a free
// update_server_queues slot, allocates that slot with the same handle, zero-fills the record and
// then calls player_update_queue_create on the embedded player_update_queue at +0x28 -- something
// Ghidra's truncated version does show as an unconditional per-iteration call, but with the wrong
// argument (it takes no visible parameter in the Ghidra text; the real call is
// player_update_queue_create(&update_server_queues[index].queue) via ESI, established already by
// types/game.h's own reading of this constructor). Finally, exactly as Ghidra shows, it tails into
// update_client_dispose() to resynchronize the client-side ring the same way.
// register convention: no visible parameters.
// UNSURE: given the corrected behavior, "dispose" is very likely the wrong name for this
// function too; it is kept in step with its already-named sibling update_client_dispose (0x472fa0,
// not a Ghidra FUN_ name, so left alone) rather than invented independently.
// reconciled: R16 data_iterator is 0x10 bytes (int16 next_index, +0x0c signature = data ^ 'iter'); the inline constructor now stores the signature like the original; the separate write-only iter_signature local is folded into it

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "fn_game.h"
#include <stdint.h>

extern data_array *update_server_queues; // 0x006f1d90
extern data_array *player_data;          // 0x0087a480

extern void data_delete_all(data_array *array);            // 0x4d0580, blam-cc: array in ESI
extern void *data_iterator_next(data_iterator *iterator);  // 0x4d05d0, blam-cc: iterator in EDI
extern void player_update_queue_create(player_update_queue *queue); // 0x479f40, this batch (0x472cc0 area), blam-cc: ESI -> queue


// Resets update_server_queues to empty, re-creates one update_server_queue slot per currently
// live player (reusing that player's own datum index and salt) and constructs its embedded
// player_update_queue, then does the same for the client-side ring via update_client_dispose.
// See the header comment: despite the name, nothing is freed here.
void update_server_dispose(void)
{
    data_iterator player_iter;
    void *player_element;

    update_server_queues->valid = 1;
    data_delete_all(update_server_queues);
    data_delete_all(update_server_queues);

    player_iter.data = player_data;
    player_iter.next_index = 0;
    player_iter.index = k_datum_index_none;
    player_iter.signature = (uint32_t)(uintptr_t)player_iter.data ^ k_data_iterator_signature;
    player_element = data_iterator_next(&player_iter);
    while (player_element != 0) {
        datum_index player_handle = player_iter.index;
        int16_t index = (int16_t)player_handle;
        int16_t salt = (int16_t)((uint32_t)player_handle >> 16);

        if (index >= 0 && index < update_server_queues->maximum_count && salt != 0) {
            update_server_queue *slot = (update_server_queue *)((uint8_t *)update_server_queues->data +
                (int32_t)update_server_queues->size * (int32_t)index);

            if (slot->identifier == 0) {
                int32_t i;
                uint8_t *raw = (uint8_t *)slot;

                update_server_queues->actual_count = update_server_queues->actual_count + 1;
                if (index >= update_server_queues->last_index) {
                    update_server_queues->last_index = (int16_t)(index + 1);
                }
                for (i = 0; i < update_server_queues->size; i++) {
                    raw[i] = 0;
                }
                update_server_queues->next_identifier = update_server_queues->next_identifier + 1;
                if (update_server_queues->next_identifier == 0) {
                    update_server_queues->next_identifier = (int16_t)k_datum_identifier_wrap;
                }
                slot->identifier = salt;

                player_update_queue_create(&slot->queue);
            }
        }
        player_element = data_iterator_next(&player_iter);
    }

    update_client_dispose();
}

#if 0
Original Ghidra decompilation (0x472b70) -- INCOMPLETE, see the header comment above.

/* WARNING: Removing unreachable block (ram,0x00472bdb) */
/* WARNING: Removing unreachable block (ram,0x00472be1) */
/* WARNING: Removing unreachable block (ram,0x00472be6) */
/* WARNING: Removing unreachable block (ram,0x00472c00) */
/* WARNING: Removing unreachable block (ram,0x00472c0a) */
/* WARNING: Removing unreachable block (ram,0x00472c0f) */
/* WARNING: Removing unreachable block (ram,0x00472c1b) */
/* WARNING: Removing unreachable block (ram,0x00472c1d) */
/* WARNING: Removing unreachable block (ram,0x00472c22) */
/* WARNING: Removing unreachable block (ram,0x00472c24) */
/* WARNING: Removing unreachable block (ram,0x00472c36) */
/* WARNING: Removing unreachable block (ram,0x00472c3c) */

void FUN_00472b70(void)

{
  int iVar1;

  *(undefined1 *)(DAT_006f1d90 + 0x24) = 1;
  data_delete_all();
  data_delete_all();
  iVar1 = data_iterator_next();
  while (iVar1 != 0) {
    FUN_00479f40();
    iVar1 = data_iterator_next();
  }
  update_client_dispose();
  return;
}

Reconstruction from objdump -d -M intel --start-address=0x472b70 --stop-address=0x472c90
bin/halo.exe of the loop body Ghidra discarded (esi=update_server_queues per iteration):

  472bc4: mov ebp,[esp+0x1c]     ; ebp = player_iter.index
  472bc8: mov esi,DAT_006f1d90
  472bce..472bd9: ebx=ebp ; ax=bp ; sar ebx,0x10 ; test ax,ax ; jl skip
  472bdb: cmp ax,[esi+0x20] ; jge skip           ; index >= maximum_count
  472be1: test bx,bx ; je skip                    ; salt == 0
  472be6..472bfa: edx = data + size*index ; cmp [edx],0 ; jne skip  ; slot occupied
  472c00: inc actual_count
  472c04..472c0b: if (index >= last_index) last_index = index + 1
  472c0f..472c22: zero `size` bytes at edx
  472c24..472c36: next_identifier++ (wrap to 0x8000 at 0)
  472c3c..472c46: *edx = bx (the player's own salt)
  472c49: reload esi
  472c56..472c65: esi = &update_server_queues.data[index] + 0x28    ; the embedded queue field
  472c65: call 0x479f40                                              ; player_update_queue_create(esi)
  472c6a: lea edi,[esp+0x14] ; call data_iterator_next ; loop
  472c7d: call update_client_dispose (0x472fa0)
#endif
