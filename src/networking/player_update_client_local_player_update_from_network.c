// player_update_client_local_player_update_from_network  (Ghidra:
// player_update_client_local_player_update_from_network, already named)
// address 0x4e5390, size 243 bytes
// name confidence: 0.85   rewrite confidence: 0.35
// evidence: out/phase4/networking_functions.md; out/phase4/networking_types_notes.md's "player
// update" and "message delta protocol" sections (the field-type descriptor and its per-field
// callbacks are explicitly documented there as unresolved); types/game.h player (identifier,
// local_player_index, unknown_e8, unknown_ec, unknown_f0, unknown_f4, unknown_f8);
// types/networking.h/memory.h data_iterator; disassembly (objdump -d -M intel) for the register
// convention and for recovering the arguments Ghidra's decompile dropped entirely.
// register convention: EAX -> decode_context (a message-delta decode block this batch does not
// have a struct for: decode_context[0] is a second pointer whose first dword is a 0/non-0 mode
// selector, and decode_context+4 is handed straight through to the baseline/delta decoders).
//   // blam-cc: EAX -> decode_context
// UNSURE (load-bearing): Ghidra's decompile declares `local_24`, `local_23`, `local_20`, `local_1c`,
// `local_18` and reads every one of them without ever assigning them, i.e. Ghidra itself could not
// find where their values come from. Disassembly resolves *some* of this precisely and confirms
// the rest really is uninitialized:
//   - `FUN_004ec590` (the baseline decoder) is called with ECX = &scratch (a `lea ecx,[esp+8]`
//     right before the call) but never itself reads ECX -- it only `push`es it once, purely to
//     reserve 4 bytes of its own stack, and that memory is popped again 12 bytes later without
//     ever being read. So the caller's scratch buffer at [esp+8] is genuinely never written by
//     anything in this call chain; `local_24` (scratch+0) and `local_23` (scratch+1), read after
//     the search loop below, are uninitialized stack bytes in the real binary, not just in
//     Ghidra's view of it.
//   - `local_20`/`local_1c`/`local_18`, by contrast, ARE well-defined: they are the SAME three
//     stack dwords used moments earlier to build the on-stack `data_iterator` (data/next_index/
//     index), reused in place once the search loop is done. So the three dwords stored into
//     unknown_f0/unknown_f4/unknown_f8 are, respectively: the `player_data` pointer, the
//     iterator's final `next_index`, and the found player's own datum_index handle (which is
//     what makes unknown_f8 a sensible self-reference rather than noise).
// REVIEW PASS 2026-09-20: the "unresolved scratch bytes" are resolved. FUN_004ec590 takes the
// decode context in EAX and the DESTINATION in ECX (0x4e53a1 `lea ecx,[esp+0x08]`); that block is
// types/networking.h local_player_update_ack, and the binary reads its two id bytes back at
// [esp+0x08] / [esp+0x09] and its three position dwords at [esp+0x0c] / [esp+0x10] / [esp+0x14].
// An earlier rewrite sourced those three dwords from the data_iterator at [esp+0x18] instead, a
// neighbouring and unrelated stack object. The paragraph below is kept only as history.
// The unresolved scratch bytes are preserved as genuinely-uninitialized locals below (matching
// player_data_iterator_advance.c's precedent for a case decompilation cannot recover), not
// invented, per the task's "no invented behaviour" rule.
// UNSURE: decode_context's exact shape; see the "message delta protocol" section of
// out/phase4/networking_types_notes.md for why no struct is declared for it here.
// reconciled: R16 data_iterator is 0x10 bytes (int16 next_index, +0x0c signature = data ^ 'iter'); the inline constructor now stores the signature like the original

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include <stdint.h>

extern data_array *player_data; // 0x0087a480
extern game_time_globals *game_time; // 0x006f1d6c, +0x0c is the current game tick
    // tick counter here (see disassembly note below); close to game.h's server update ring at
    // 0x006f1d94, but the two addresses are not resolved as the same object by this batch.

extern uint8_t message_delta_decode_compound_field(void *decode_context, void *destination);
    // blam-cc: EAX -> decode_context, ECX -> destination; 0x4ec590, message-delta stateless
    // (baseline) decode. It forwards to message_delta_read_changed_subfields with a NULL
    // previous-state pointer and the caller destination (0x4ec591..0x4ec59a).
extern void message_delta_decode_compound_field_staged(void *decode_context);
    // blam-cc: EAX -> decode_context; 0x4ec670, the message-delta skip/drop path
extern void *data_iterator_next(data_iterator *iterator); // 0x4d05d0, blam-cc: iterator in EDI
extern uint8_t is_local_player_update_in_order(int32_t current_update_id, int32_t new_update_id);
    // foreign (< this batch), 0x4e69b0, blam-cc: EAX -> current_update_id, ECX -> new_update_id
extern void player_update_history_log_write(uint32_t category_flags, int32_t use_filtered_mask,
    const char *format, ...); // this module, 0x4e5ea0
extern void player_update_history_play_for_update_index(void *update_history, int32_t update_id);
    // foreign (< this batch), 0x4e6950, UNSURE: exact register convention not re-derived here

// Handles an incoming acknowledgement of the local player's own position update: if
// decode_context selects the baseline path and decodes successfully, finds the (single) local
// player, checks the new id is in order, logs it, latches the decoded control fields into the
// player object, and replays pending updates on top of it. The delta path is decoded but its
// result is not otherwise used by this function.
void player_update_client_local_player_update_from_network(int32_t *decode_context) // blam-cc: EAX -> decode_context
{
    int32_t mode;
    int32_t *record_ctx;
    local_player_update_ack ack; // [esp+0x08], the ECX destination FUN_004ec590 fills
    data_iterator iter;          // [esp+0x18], the neighbouring stack object
    player *candidate;

    record_ctx = (int32_t *)(uintptr_t)decode_context[0];
    mode = record_ctx[0];
    if (mode != 0) {
        message_delta_decode_compound_field_staged(decode_context);
        return;
    }
    if (message_delta_decode_compound_field(decode_context, &ack) != 1) {
        return;
    }
    iter.data = player_data;
    iter.next_index = 0;
    iter.index = k_datum_index_none;
    iter.signature = (uint32_t)(uintptr_t)iter.data ^ k_data_iterator_signature;
    candidate = (player *)data_iterator_next(&iter);
    if (candidate == 0) {
        return;
    }
    while (candidate->local_player_index == -1) {
        candidate = (player *)data_iterator_next(&iter);
        if (candidate == 0) {
            return;
        }
    }
    if (is_local_player_update_in_order(candidate->unknown_e8, ack.update_id) != 1) {
        return;
    }
    player_update_history_log_write(1, 0, "[%d]: Received ack for update [%d].\n",
        game_time->game_time, ack.baseline_id);
    candidate->unknown_e8 = ack.update_id;
    candidate->unknown_ec = ack.baseline_id;
    candidate->unknown_f0 = *(int32_t *)&ack.position.x;
    candidate->unknown_f4 = *(int32_t *)&ack.position.y;
    candidate->unknown_f8 = *(int32_t *)&ack.position.z;
    player_update_history_play_for_update_index(0, 0); // UNSURE: arguments not re-derived, see file header
}

#if 0
Original Ghidra decompilation (0x4e5390), from tools/pack.py 0x4e5390:

void player_update_client_local_player_update_from_network(void)

{
  char cVar1;
  undefined4 *in_EAX;
  int iVar2;
  byte local_24;
  byte local_23;
  undefined4 local_20;
  undefined4 local_1c;
  undefined4 local_18;

  if (*(int *)*in_EAX == 0) {
    cVar1 = FUN_004ec590();
    if (cVar1 == '\x01') {
      iVar2 = data_iterator_next();
      if (iVar2 != 0) {
        while (*(short *)(iVar2 + 2) == -1) {
          iVar2 = data_iterator_next();
          if (iVar2 == 0) {
            return;
          }
        }
        cVar1 = is_local_player_update_in_order();
        if (cVar1 == '\x01') {
          player_update_history_log_write
                    ("[%d]: Received ack for update [%d].\n",*(undefined4 *)(DAT_006f1d6c + 0xc),
                     local_23);
          *(uint *)(iVar2 + 0xe8) = (uint)local_24;
          *(uint *)(iVar2 + 0xec) = (uint)local_23;
          *(undefined4 *)(iVar2 + 0xf0) = local_20;
          *(undefined4 *)(iVar2 + 0xf4) = local_1c;
          *(undefined4 *)(iVar2 + 0xf8) = local_18;
          player_update_history_play_for_update_index();
          return;
        }
      }
    }
  }
  else {
    FUN_004ec670();
  }
  return;
}

Disassembly (objdump -d -M intel, bin/halo.exe) for the register convention and the scratch/
iterator memory reuse described in the header:
  4e5390: mov ecx,[eax]     ; ecx = *decode_context
  4e5392: mov edx,[ecx]     ; edx = mode = **decode_context
  4e539b: jne 0x4e5478      ; mode != 0 -> FUN_004ec670 (delta), no return-value check
  4e53a1: lea ecx,[esp+0x8] ; scratch address, passed to FUN_004ec590 but never read by it
  4e53a5: call 0x4ec590
  4e53b2..4e53d7: build the on-stack data_iterator at [esp+0x18..0x24)
  4e5402: movzx ecx,[esp+0x8]   ; is_local_player_update_in_order new_update_id (uninitialized)
  4e5407: mov eax,[esi+0xe8]    ; candidate->unknown_e8 as current_update_id
  4e5416: movzx edx,[esp+0x9]   ; log_write new-id vararg (uninitialized)
  4e541b: mov eax,[0x6f1d6c] / 4e5420: mov ecx,[eax+0xc]   ; network_client_current_tick
  4e5436: movzx edx,[esp+0x14]  ; -> candidate->unknown_e8 (uninitialized)
  4e5441: movzx eax,[esp+0x15]  ; -> candidate->unknown_ec (uninitialized)
  4e544c..4e5463: [esp+0x18],[esp+0x1c],[esp+0x20] (the iterator own fields, well-defined) ->
                  candidate->unknown_f0/f4/f8
#endif
