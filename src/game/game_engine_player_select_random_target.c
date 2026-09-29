// game_engine_player_select_random_target  (Ghidra: FUN_0046f1a0; renamed, no established name)
// address 0x46f1a0, size 475 bytes (Ghidra's size; the real function runs to 0x46f37e -- see below)
// name confidence: 0.2   rewrite confidence: 0.35
// evidence: out/phase4/game_functions.md ("Clears a player's pending reservation flag and
// randomly selects another active player/object outside the player's own team or bucket");
// types/game.h player (team +0x20, unit +0x34, unknown_88 "part of the profile block");
// src/game/chimera__kill_feed.c for the (recipient, param_1, message_type, subject, broadcast)
// signature and its EDI-register recipient; src/memory/data_iterator_next.c for the
// data_iterator idiom. callers=0 in this build -- this function is not reached from anywhere
// Ghidra's call-graph export can see (possibly a leftover/unused engine hook).
//
// CORRECTED (this rewrite) against
//   objdump -d -M intel --start-address=0x46f1a0 --stop-address=0x46f380 bin/halo.exe
// Ghidra's own decompile for this function is WRONG, not just truncated: it prints four
// "WARNING: Removing unreachable block" lines and, as a direct result, (a) drops the entire
// second half of the function (the winner is actually stored and a kill-feed message is
// dispatched; Ghidra's version ends with an unconditional `*(int*)(player+0x88) = -1`), and
// (b) mis-attributes the first loop's filter -- Ghidra shows `param_1 != 0xffffffff` and
// `iVar2 != -1` as the guards, but the real comparisons (objdump 0x46f224/0x46f228) are
// `candidate != player_or_all` and `candidate != previous_target`. The whole function below is
// therefore transcribed from the disassembly, not from Ghidra's C.
// register convention: single __cdecl stack parameter (objdump 0x46f1af: `mov esi,[esp+0x34]`
// against the caller's pushed dword, before this function's own four register pushes).
//   // blam-cc: stack -> player_or_all
// UNSURE: `previous_target` (player::unknown_88) is only ever seen written by this function (to
// the newly picked target, or to -1 when none is found) and read back as an exclusion value here;
// its true purpose ("reservation", "last target", something else) is not otherwise attested.
// UNSURE: the kill-feed message id 0x20 is out of the 0..0x1f range game_engine_build_kill_feed_
// message_text's own switch recognizes, so on every built-in gametype `chimera__kill_feed` will
// fail to build any text for it (only a custom variant's build_message_text override could ever
// render it). What this function is *for* is therefore not recoverable from this build alone.
// UNSURE: if `player_or_all` is passed as k_datum_index_none (0xffffffff), `self` below is
// computed from index 0xffff, which is out of player_data's 16-slot range. No caller exists in
// this build to say whether that path is actually reachable.
// reconciled: R16 data_iterator is 0x10 bytes (int16 next_index, +0x0c signature = data ^ 'iter'); the inline constructor now stores the signature like the original; the separate write-only iter_signature local is folded into it

// CORRECTED (phase 4 review): the Blam random-index idiom is
//   movsx ecx,<count> ; shr eax,0x10 ; imul eax,ecx ; shr eax,0x10 ; movsx <idx>,ax
// so the seed's high half is used ZERO-extended (shr, no movsx) and the product is shifted
// down logically. Casting (seed >> 16) to int16_t first, as this file did, makes the index
// negative for half of all seeds, which silently disables the pick.
#include "tags.h"
#include "memory.h"
#include "math.h"
#include <stdint.h>
#include "game.h"

extern data_array *player_data;        // 0x0087a480
extern random_seed random_seed_global;    // 0x00719cd0

extern void chimera__kill_feed(datum_index recipient, int32_t hash_key, uint32_t message_type,
    datum_index subject, char broadcast); // 0x460a30
extern void *data_iterator_next(data_iterator *iterator); // 0x4d05d0, blam-cc: iterator in EDI

// Picks a random live player who is not `player_or_all` itself, not the player's previous pick,
// on the opposing team and currently driving a unit; stores the pick (or k_datum_index_none) into
// the player's own unknown_88 field, then dispatches a kill-feed message (id 0x20) about it --
// either to `player_or_all` alone, or, when `player_or_all` is k_datum_index_none, to every
// player in turn.
void game_engine_player_select_random_target(datum_index player_or_all)
{
    uint16_t self_index = (uint16_t)player_or_all;
    player *self = (player *)((uint8_t *)player_data->data + (uint32_t)self_index * sizeof(player));
    int32_t previous_target = self->slayer_target; // UNSURE: see header
    int32_t match_count = 0;
    datum_index winner = k_datum_index_none;
    data_iterator iter;
    void *element;

    iter.data = player_data;
    iter.next_index = 0;
    iter.index = k_datum_index_none;
    iter.signature = (uint32_t)(uintptr_t)iter.data ^ k_data_iterator_signature;
    element = data_iterator_next(&iter);
    while (element != 0) {
        datum_index candidate = iter.index;
        player *candidate_player =
            (player *)((uint8_t *)player_data->data + (uint32_t)(uint16_t)candidate * sizeof(player));

        if (candidate != player_or_all && (int32_t)candidate != previous_target &&
            candidate_player->team != self->team && candidate_player->unit != k_datum_index_none) {
            match_count = match_count + 1;
        }
        element = data_iterator_next(&iter);
    }

    if (match_count > 0) {
        int16_t pick;

        random_seed_global = random_seed_global * 0x19660d + 0x3c6ef35f;
        pick = (int16_t)(((random_seed_global >> 16) *
                          (uint32_t)(int32_t)(int16_t)match_count) >> 16);

        iter.data = player_data;
        iter.next_index = 0;
        iter.index = k_datum_index_none;
        iter.signature = (uint32_t)(uintptr_t)iter.data ^ k_data_iterator_signature;
        element = data_iterator_next(&iter);
        while (element != 0) {
            datum_index candidate = iter.index;
            player *candidate_player =
                (player *)((uint8_t *)player_data->data + (uint32_t)(uint16_t)candidate * sizeof(player));

            if (candidate != player_or_all && (int32_t)candidate != previous_target &&
                candidate_player->team != self->team && candidate_player->unit != k_datum_index_none) {
                if (pick == 0) {
                    winner = candidate;
                    break;
                }
                pick = pick - 1;
            }
            element = data_iterator_next(&iter);
        }
    }

    self->slayer_target = (int32_t)winner;
    if (winner == k_datum_index_none) {
        return;
    }

    if (player_or_all != k_datum_index_none) {
        chimera__kill_feed(player_or_all, (int32_t)player_or_all, 0x20, winner, 1);
        return;
    }

    {
        data_iterator broadcast_iter;
        void *broadcast_element;

        broadcast_iter.data = player_data;
        broadcast_iter.next_index = 0;
        broadcast_iter.index = k_datum_index_none;
        broadcast_iter.signature = (uint32_t)(uintptr_t)broadcast_iter.data ^ k_data_iterator_signature;
        broadcast_element = data_iterator_next(&broadcast_iter);
        while (broadcast_element != 0) {
            chimera__kill_feed(broadcast_iter.index, (int32_t)broadcast_iter.index, 0x20, winner, 1);
            broadcast_element = data_iterator_next(&broadcast_iter);
        }
    }
}

#if 0
Original Ghidra decompilation (0x46f1a0), from tools/pack.py 0x46f1a0 -- INCOMPLETE/WRONG, see the
header comment above. Reproduced verbatim for diffing purposes only.

/* WARNING: Removing unreachable block (ram,0x0046f310) */
/* WARNING: Removing unreachable block (ram,0x0046f319) */
/* WARNING: Removing unreachable block (ram,0x0046f32f) */
/* WARNING: Removing unreachable block (ram,0x0046f357) */

void FUN_0046f1a0(uint param_1)

{
  int iVar1;
  int iVar2;
  int iVar3;
  int iVar4;
  int iVar5;
  int iVar6;
  int local_24;

  iVar3 = DAT_0087a480;
  iVar1 = *(int *)(DAT_0087a480 + 0x34);
  iVar6 = (param_1 & 0xffff) * 0x200;
  iVar2 = *(int *)(iVar1 + 0x88 + iVar6);
  local_24 = 0;
  iVar4 = data_iterator_next();
  if (iVar4 != 0) {
    iVar4 = *(int *)(iVar3 + 0x34);
    do {
      if ((((param_1 != 0xffffffff) && (iVar2 != -1)) &&
          (*(int *)(iVar4 + 0x1fffe20) != *(int *)(iVar6 + 0x20 + iVar4))) &&
         (*(int *)(iVar4 + 0x1fffe34) != -1)) {
        local_24 = local_24 + 1;
      }
      iVar5 = data_iterator_next();
    } while (iVar5 != 0);
    if (0 < local_24) {
      random_seed_global = random_seed_global * 0x19660d + 0x3c6ef35f;
      local_24 = (int)(short)((random_seed_global >> 0x10) * (int)(short)local_24 >> 0x10);
      iVar4 = data_iterator_next();
      if (iVar4 != 0) {
        iVar3 = *(int *)(iVar3 + 0x34);
        do {
          if (((param_1 != 0xffffffff) && (iVar2 != -1)) &&
             ((*(int *)(iVar3 + 0x1fffe20) != *(int *)(iVar6 + 0x20 + iVar3) &&
              (*(int *)(iVar3 + 0x1fffe34) != -1)))) {
            if (local_24 == 0) break;
            local_24 = local_24 + -1;
          }
          iVar4 = data_iterator_next();
        } while (iVar4 != 0);
      }
    }
  }
  *(undefined4 *)(iVar1 + iVar6 + 0x88) = 0xffffffff;
  return;
}

Full reconstruction from objdump -d -M intel --start-address=0x46f1a0 --stop-address=0x46f380
bin/halo.exe (the part Ghidra's own decompiler discarded):

  46f2fd: mov ebp,[esp+0x14]        ; ebp = &player_at(self_index)
  46f301: mov esi,[esp+0x1c]        ; esi = winner (or -1)
  46f305: cmp esi,0xffffffff
  46f308: mov [ebp+0x88],esi        ; self->slayer_target = winner
  46f30e: je 0x46f376               ; winner == -1 -> return
  46f310: mov edi,[esp+0x38]        ; edi = player_or_all (the original argument)
  46f314: cmp edi,0xffffffff
  46f317: je 0x46f32f               ; player_or_all == -1 -> broadcast-all path
  46f319: push 0x1 ; push esi ; push 0x20 ; push edi
  46f31f: call chimera__kill_feed   ; (recipient=edi, param_1=edi, type=0x20, subject=esi, bcast=1)
  46f324..46f32e: epilogue, ret
  46f32f: mov eax,[esp+0x20]        ; eax = player_data ^ 'iter' (dead scratch value)
  46f333..46f34a: rebuild the same data_iterator over player_data in edi
  46f34e: call data_iterator_next
  46f353: test eax,eax ; je 0x46f376
  46f357: mov edi,[esp+0x2c]        ; edi = this iteration's player handle
  46f35b: push 0x1 ; push esi ; push 0x20 ; push edi
  46f361: call chimera__kill_feed   ; (recipient=edi, param_1=edi, type=0x20, subject=esi, bcast=1)
  46f366..46f374: lea edi,[esp+0x24] ; call data_iterator_next ; loop while more elements
  46f376: pop edi ; pop esi ; pop ebp ; pop ebx ; add esp,0x24 ; ret
#endif
