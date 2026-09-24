// game_engine_build_sorted_player_list  (Ghidra: game_engine_build_sorted_player_list,
// already named -- but see UNSURE below, this rewrite does NOT follow Ghidra's decompile)
// address 0x45cc90, size 303 bytes
// name confidence: 0.5   rewrite confidence: 0.55
// evidence: types/game.h scoreboard_entry (player 0x00, unknown_04 0x04, key_0 0x08,
//   key_1 0x0c, key_2 0x10, key_3 0x14, place 0x18) -- its own comment already documents this
//   exact function's tie-detection ("compares dwords 2..5 of adjacent entries... place... bit
//   0x80000000 set on a tie"), confirmed field-for-field below; player (kills 0x9c, deaths
//   0xae, assists 0xa4); current_game_engine (get_score at +0x4c); scoreboard_entry_compare
//   (0x45cbe0) and scoreboard_entry_compare_by_unknown_04 (0x45cbc0), both this batch;
//   game_engine_build_scoreboard_sort_key (0x45cc30, this batch).
// register convention: an incoming AL byte (a genuine 3rd argument Ghidra's decompile drops
//   entirely -- see UNSURE), then the two stack parameters Ghidra does recognize.
//   // blam-cc: AL -> invert_low_stat, stack -> (out_entries, mode)
//
// UNSURE, IMPORTANT: Ghidra's own decompilation of this function is corrupted -- it prints
// "WARNING: Could not recover jumptable at 0x0045cd3d. Too many branches" and then
// "Treating indirect jump as call", which invents a single bogus call/return in place of a
// real 5-way `switch (mode)` and drops the AL-register argument and the post-sort rank/tie
// loop entirely. src/game/game_engine_get_scoreboard_place.c (0x45d440, a caller, not in this
// batch) already flagged this exact corruption. Because of that, THIS rewrite is not a
// transcription of Ghidra's C -- it is reconstructed directly from
// `objdump -d -M intel --start-address=0x45cc90 --stop-address=0x45cf00 bin/halo.exe` and the
// raw jump-table bytes at 0x45ce7c (read with `objdump -s`), which decode cleanly with no
// further warnings. The jump table has exactly 5 entries (0x45cd44, 0x45cd8b, 0x45cdb3,
// 0x45cdbe, 0x45cdc9), matching `mode` 0..4, and every offset/field access below was read
// directly off that disassembly. The xor eax,0x69746572 dword is data_iterator.signature
// (+0x0c, types/memory.h); it is written like every other inline iterator constructor.
// The `next_index` field is written with a 16-bit store (`mov word ptr [esp+0x1c], bx`),
// which matches types/memory.h: data_iterator::next_index is an int16_t.
// reconciled: R16 data_iterator is 0x10 bytes: next_index is int16 (plain store now), the 'iter' dword is the +0x0c signature and is stored

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include <stdint.h>

extern data_array *player_data;                      // 0x0087a480
extern game_engine_definition *current_game_engine;  // 0x006f1d20

extern void *data_iterator_next(data_iterator *iterator); // 0x4d05d0, memory module
extern void qsort(void *base, uint32_t count, uint32_t size,
    uint32_t (*compare)(const void *, const void *)); // CRT
extern uint32_t scoreboard_entry_compare_by_unknown_04(const scoreboard_entry *a,
    const scoreboard_entry *b); // 0x45cbc0, this batch
extern int32_t scoreboard_entry_compare(const scoreboard_entry *a,
    const scoreboard_entry *b); // 0x45cbe0, this batch
extern uint32_t game_engine_build_scoreboard_sort_key(uint32_t player_index, int32_t score); // 0x45cc30, this batch

// Collects up to 16 active players into `out_entries`, fills each entry's sort fields
// according to `mode` (0: the full score/kills/deaths/assists key tuple used for the normal
// scoreboard sort; 1..4: a single stat -- score-key, kills, assists, deaths respectively --
// into `unknown_04`), sorts the result (the multi-field comparator for mode 0, the
// single-field one otherwise), and assigns each entry's 0-based `place`, OR-ing in
// 0x80000000 for entries that tie every sort key with the entry immediately before them.
// Returns the number of entries filled (0..16).
//
// `invert_low_stat` negates each filled `unknown_04` value (modes 1..4 only; mode 0 never
// touches unknown_04, matching types/game.h's note that this field is "never compared" by the
// live scoreboard path). When `mode == 4` the incoming flag is itself inverted first
// (`invert_low_stat == 0`), which -- given mode 4 is the deaths stat, where a lower number is
// better -- looks like it exists to keep "more negative won't sort first" consistent across
// stats; not confirmed against a caller that actually varies these two parameters together.
int32_t game_engine_build_sorted_player_list(uint8_t invert_low_stat,
    scoreboard_entry out_entries[16], int32_t mode)
    // blam-cc: AL -> invert_low_stat, stack -> (out_entries, mode)
{
    data_iterator iterator;
    player *p;
    int32_t count;
    scoreboard_entry *entry;
    uint8_t negate;
    int32_t i;

    negate = (mode == 4) ? (invert_low_stat == 0) : invert_low_stat;

    iterator.data = player_data;
    iterator.next_index = 0;
    iterator.index = k_datum_index_none;
    iterator.signature = (uint32_t)(uintptr_t)iterator.data ^ k_data_iterator_signature;

    count = 0;
    p = (player *)data_iterator_next(&iterator);
    entry = out_entries;
    if (p != (player *)0) {
        while (p != (player *)0) {
            if (count < 16) {
                entry->player = iterator.index;
                count = count + 1;
                entry = entry + 1;
            }
            p = data_iterator_next(&iterator);
        }
    }

    for (i = 0; i < count; i = i + 1) {
        entry = &out_entries[i];
        p = (player *)((uint8_t *)player_data->data + ((uint32_t)entry->player & 0xffff) * sizeof(player));

        switch (mode) {
        case 0: {
            int32_t score = 0;
            entry->key_0 = 0;
            if (current_game_engine->get_score != (void *)0) {
                score = ((int32_t (*)(datum_index, int32_t))current_game_engine->get_score)(
                    entry->player, 0);
                entry->key_0 = (int32_t)game_engine_build_scoreboard_sort_key(entry->player, score);
            }
            entry->key_1 = p->kills;
            entry->key_2 = p->deaths;
            entry->key_3 = p->assists;
            break;
        }
        case 1: {
            entry->unknown_04 = 0;
            if (current_game_engine->get_score != (void *)0) {
                int32_t score = ((int32_t (*)(datum_index, int32_t))current_game_engine->get_score)(
                    entry->player, 0);
                entry->unknown_04 = (int32_t)game_engine_build_scoreboard_sort_key(entry->player, score);
            }
            break;
        }
        case 2:
            entry->unknown_04 = p->kills;
            break;
        case 3:
            entry->unknown_04 = p->assists;
            break;
        case 4:
            entry->unknown_04 = p->deaths;
            break;
        default:
            break; // UNSURE: the real jump table has no entry past mode 4; unreachable in practice
        }

        if (negate) {
            entry->unknown_04 = -entry->unknown_04;
        }
    }

    qsort(out_entries, (uint32_t)count, sizeof(scoreboard_entry),
        (mode == 0) ? (uint32_t (*)(const void *, const void *))(void *)scoreboard_entry_compare
                    : (uint32_t (*)(const void *, const void *))(void *)scoreboard_entry_compare_by_unknown_04);

    for (i = 0; i < count; i = i + 1) {
        entry = &out_entries[i];
        if (i == 0) {
            entry->place = i;
        } else {
            scoreboard_entry *prev = &out_entries[i - 1];
            if (prev->key_0 != entry->key_0 || prev->key_1 != entry->key_1 ||
                prev->key_2 != entry->key_2 || prev->key_3 != entry->key_3) {
                entry->place = i;
            } else {
                int32_t tied_place = prev->place | (int32_t)0x80000000u;
                prev->place = tied_place;
                entry->place = tied_place;
            }
        }
    }

    return count;
}

#if 0
Ghidra's decompilation of 0x45cc90 is corrupted by a failed jump-table recovery and is not a
faithful representation of this function -- see the header UNSURE note. It is kept here only
for reference to what pack.py originally returned:

size_t game_engine_build_sorted_player_list(undefined4 *param_1,int param_2)

{
  uint uVar1;
  int iVar2;
  int *piVar3;
  size_t sVar4;
  undefined4 *puVar5;
  code *_PtFuncCompare;

  sVar4 = 0;
  iVar2 = data_iterator_next();
  puVar5 = param_1;
  while (iVar2 != 0) {
    if ((int)sVar4 < 0x10) {
      *puVar5 = 0xffffffff;
      sVar4 = sVar4 + 1;
      puVar5 = puVar5 + 7;
    }
    iVar2 = data_iterator_next();
  }
  if (0 < (int)sVar4) {
                    /* WARNING: Could not recover jumptable at 0x0045cd3d. Too many branches */
                    /* WARNING: Treating indirect jump as call */
    sVar4 = (*(code *)(&PTR_LAB_0045ce7c)[param_2])();
    return sVar4;
  }
  if (param_2 == 0) {
    _PtFuncCompare = FUN_0045cbe0;
  }
  else {
    _PtFuncCompare = FUN_0045cbc0;
  }
  _qsort(param_1,sVar4,0x1c,_PtFuncCompare);
  iVar2 = 0;
  if (0 < (int)sVar4) {
    piVar3 = param_1 + -4;
    do {
      if (iVar2 == 0) {
LAB_0045ce64:
        piVar3[10] = iVar2;
      }
      else {
        if ((piVar3[6] < piVar3[-1]) || (piVar3[-1] < piVar3[6])) goto LAB_0045ce64;
        if ((piVar3[7] < *piVar3) || (*piVar3 < piVar3[7])) goto LAB_0045ce64;
        if ((piVar3[8] < piVar3[1]) || (piVar3[1] < piVar3[8])) goto LAB_0045ce64;
        if ((piVar3[9] < piVar3[2]) || (piVar3[2] < piVar3[9])) goto LAB_0045ce64;
        uVar1 = piVar3[3];
        piVar3[3] = uVar1 | 0x80000000;
        piVar3[10] = uVar1 | 0x80000000;
      }
      iVar2 = iVar2 + 1;
      piVar3 = piVar3 + 7;
    } while (iVar2 < (int)sVar4);
  }
  return sVar4;
}

The reconstruction above is instead based on the disassembly:

0045cc90: sub esp,0x18
0045cc93: mov ecx,[esp+0x20]          ; ecx = mode
0045cc9b: cmp ecx,0x4
0045cca0: mov [esp+0x14],ebx          ; count = 0
0045cca4: jne 0x45ccaf
0045cca6: test al,al
0045cca8: sete [esp+0x13]             ; invert = (mode==4) ? (al==0) : al
0045ccaf: mov [esp+0x13],al
0045ccb3: mov eax,ds:0x87a480         ; player_data
0045ccb8: mov [esp+0x18],eax          ; iterator.data
0045ccc5: mov word ptr [esp+0x1c],bx  ; iterator.next_index (low 16 bits only) = 0
0045ccca: mov [esp+0x20],0xffffffff   ; iterator.index = -1
0045ccd6: call data_iterator_next
0045ccdd: mov ebp,[esp+0x2c]          ; ebp = out_entries
... fill loop (up to 16 entries, entry->player = iterator.index) ...
0045cd20: mov ebp,[esi-4]             ; player handle
0045cd29: mov eax,[player_data->data]
0045cd38: shl edi,9 ; add edi,eax     ; edi = &player[handle & 0xffff]
0045cd3d: jmp [edx*4 + 0x45ce7c]      ; switch (mode) -- table: 0x45cd44,0x45cd8b,0x45cdb3,
                                      ;   0x45cdbe, 0x45cdc9 for mode 0..4
0045cd44..: case 0: key_0/1/2/3 = score-key/kills/deaths/assists
0045cd8b..: case 1: unknown_04 = score-key
0045cdb3..: case 2: unknown_04 = kills (0x9c)
0045cdbe..: case 3: unknown_04 = assists (0xa4)
0045cdc9..: case 4: unknown_04 = deaths (0xae)
0045cdd2: if (invert) unknown_04 = -unknown_04
0045cdf6: qsort(out_entries, count, 0x1c, mode==0 ? scoreboard_entry_compare
                                                   : scoreboard_entry_compare_by_unknown_04)
0045ce1c..0045ce6f: rank/tie loop comparing key_0..key_3 of adjacent sorted entries, writing
  `place` (0x80000000 marks a tie), exactly matching types/game.h's own note on this field.
#endif
