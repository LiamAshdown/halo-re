// update_client_dispose  (Ghidra: update_client_dispose, already named)
// address 0x472fa0, size 240 bytes
// name confidence: 0.55   rewrite confidence: 0.3
// evidence: out/phase4/game_functions.md ("Tears down the client update queue's datum array");
// types/game.h update_server_queue / player_update_queue / update_client_queues global; the
// sibling pair update_server_new (0x472aa0, allocates) / update_queues_dispose (0x472b00, the
// real GlobalFree teardown) for contrast.
//
// CORRECTED (this rewrite) against
//   objdump -d -M intel --start-address=0x472fa0 --stop-address=0x473090 bin/halo.exe
// Ghidra prints 12 "WARNING: Removing unreachable block" lines and, as a result, shows only
// `data_delete_all(); data_delete_all(); while (data_iterator_next()) {}` -- an empty-bodied
// loop. The real function is NOT a teardown: after resetting update_client_queues to empty
// (data_delete_all, called twice as the compiled code genuinely does -- both calls target the
// same array with nothing in between that could change it, so the second is a no-op replay of
// the first), it walks every live entry of player_data and, for each one whose own datum index
// fits inside update_client_queues (index < maximum_count, salt != 0) and whose corresponding
// slot is currently free, allocates that slot with the *same* index and salt as the player and
// zero-fills the record -- i.e. it re-synchronizes update_client_queues so every live player has
// a same-handle entry, it does not free anything. See update_server_dispose.c (0x472b70) for the
// same idiom with an added per-record constructor call.
// register convention: no visible parameters; unaff_ESI/EDI patterns are this function's own
// locals (the data_iterator lives on the stack), not caller-supplied registers.
// UNSURE: given the corrected behavior, "dispose" is very likely the wrong name for this
// function, but it is not a Ghidra-generated FUN_ name, so it is kept as-is per the task's
// naming rule; see the header note on update_server_dispose.c for the matching sibling.
// reconciled: R16 data_iterator is 0x10 bytes (int16 next_index, +0x0c signature = data ^ 'iter'); the inline constructor now stores the signature like the original; the separate write-only iter_signature local is folded into it

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern data_array *update_client_queues; // 0x006f7ed0
extern data_array *player_data;          // 0x0087a480

extern void data_delete_all(data_array *array);            // 0x4d0580, blam-cc: array in ESI
extern void *data_iterator_next(data_iterator *iterator);  // 0x4d05d0, blam-cc: iterator in EDI

// Resets update_client_queues to empty, then re-creates one zero-filled update_server_queue-sized
// slot per currently live player, reusing that player's own datum index and salt so the two
// arrays stay handle-compatible. See the header comment: despite the name, nothing is freed here.
void update_client_dispose(void)
{
    data_iterator player_iter;
    void *player_element;

    update_client_queues->valid = 1;
    data_delete_all(update_client_queues);
    data_delete_all(update_client_queues);

    player_iter.data = player_data;
    player_iter.next_index = 0;
    player_iter.index = k_datum_index_none;
    player_iter.signature = (uint32_t)(uintptr_t)player_iter.data ^ k_data_iterator_signature;
    player_element = data_iterator_next(&player_iter);
    while (player_element != 0) {
        datum_index player_handle = player_iter.index;
        int16_t index = (int16_t)player_handle;
        int16_t salt = (int16_t)((uint32_t)player_handle >> 16);

        if (index >= 0 && index < update_client_queues->maximum_count && salt != 0) {
            uint8_t *slot = (uint8_t *)update_client_queues->data +
                (int32_t)update_client_queues->size * (int32_t)index;

            if (*(int16_t *)slot == 0) {
                int32_t i;

                update_client_queues->actual_count = update_client_queues->actual_count + 1;
                if (index >= update_client_queues->last_index) {
                    update_client_queues->last_index = (int16_t)(index + 1);
                }
                for (i = 0; i < update_client_queues->size; i++) {
                    slot[i] = 0;
                }
                update_client_queues->next_identifier = update_client_queues->next_identifier + 1;
                if (update_client_queues->next_identifier == 0) {
                    update_client_queues->next_identifier = (int16_t)k_datum_identifier_wrap;
                }
                *(int16_t *)slot = salt;
            }
        }
        player_element = data_iterator_next(&player_iter);
    }
}

#if 0
Original Ghidra decompilation (0x472fa0) -- INCOMPLETE, see the header comment above.

/* WARNING: Removing unreachable block (ram,0x00473011) */
/* WARNING: Removing unreachable block (ram,0x00473017) */
/* WARNING: Removing unreachable block (ram,0x0047301c) */
/* WARNING: Removing unreachable block (ram,0x00473032) */
/* WARNING: Removing unreachable block (ram,0x0047303c) */
/* WARNING: Removing unreachable block (ram,0x00473041) */
/* WARNING: Removing unreachable block (ram,0x0047304d) */
/* WARNING: Removing unreachable block (ram,0x0047304f) */
/* WARNING: Removing unreachable block (ram,0x00473054) */
/* WARNING: Removing unreachable block (ram,0x00473056) */
/* WARNING: Removing unreachable block (ram,0x00473068) */
/* WARNING: Removing unreachable block (ram,0x0047306e) */

void update_client_dispose(void)

{
  int iVar1;

  *(undefined1 *)(DAT_006f7ed0 + 0x24) = 1;
  data_delete_all();
  data_delete_all();
  iVar1 = data_iterator_next();
  while (iVar1 != 0) {
    iVar1 = data_iterator_next();
  }
  return;
}

Reconstruction from objdump -d -M intel --start-address=0x472fa0 --stop-address=0x473090
bin/halo.exe of the loop body Ghidra discarded (per-player slot at esi=update_client_queues):

  472ff2: mov esi,DAT_006f7ed0
  473000: mov edi,[esp+0x18]      ; edi = player_iter.index (this iteration's player handle)
  473004..473009: ebx=edi ; ax=di ; sar ebx,0x10        ; ebx = signed salt, ax = index
  47300c: test ax,ax ; jl skip                          ; index < 0 -> skip
  473011: cmp ax,[esi+0x20] ; jge skip                   ; index >= maximum_count -> skip
  473017: test bx,bx ; je skip                            ; salt == 0 -> skip
  47301c..47302c: edx = data + size*index ; cmp [edx],0 ; jne skip   ; slot already occupied -> skip
  473032: inc actual_count
  473036..47303d: if (index >= last_index) last_index = index + 1
  473041..473054: zero `size` bytes at edx (dword-at-a-time then remainder bytes)
  473056..47306e: next_identifier++ (wrap to 0x8000 at 0); *edx = bx (the player's own salt)
  473071: reload esi ; loop to next player via data_iterator_next
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
