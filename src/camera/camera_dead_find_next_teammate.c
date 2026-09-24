// camera_dead_find_next_teammate  (Ghidra: FUN_004452c0; renamed, Blam-style, not previously named)
// address 0x4452c0, size 179 bytes (0x4452c0..0x445373). Ghidra reports this function as only
//   144 bytes and mis-splits its tail off as a separate function "physical_memory_map_predict_resources"
//   at 0x445350 -- that address is not a real function, it is this loop's continuation (the
//   `call 0x4d05d0` / `test eax,eax` / `jne 0x445310` at the very end of the loop body). See
//   out/phase4/camera_types_notes.md "Misattributed" section.
// name confidence: 0.4   rewrite confidence: 0.85 (was 0.55)
// evidence: camera_types_notes.md: "the target cycling in 0x445240 / 0x4452c0 walks the players
//   data_array comparing player+0x20 (team) and +0x34 (unit)". Ghidra's decompile for this
//   function is unreliable (both loop bodies collapse to "walk the iterator and discard the
//   result"), so this rewrite follows objdump instead.
// review fix (phase 4 gate): the scan now stops at the first later candidate; the earlier
//   rewrite kept scanning and returned the LAST one. Team compared as int32 (0x4452df).
// register convention: two cdecl stack parameters (reference_player, current_target) plus a
//   third argument in BL (require_same_team), confirmed at the only call site
//   (camera_track_compute_pov 0x445380: `mov bl, al` right after the
//   camera_dead_player_has_teammate call, then `call 0x4452c0`).
//   // blam-cc: BL -> require_same_team, stack -> (reference_player, current_target)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"

extern data_array *player_data; // 0x0087a480, stride 0x200 (no types/players.h yet)
extern void *data_iterator_next(data_iterator *iterator); // 0x4d05d0, memory module

// blam-cc: BL -> require_same_team, stack -> (reference_player, current_target)
// Cycles to the next eligible teammate after `current_target`: scans the players in ascending
// index order skipping `reference_player` itself, any player with no unit and, when
// `require_same_team` is set, any player not on reference_player's team. The first eligible
// player becomes the wrap-around fallback; the scan stops at the first LATER eligible player
// whose index is greater than current_target's (0x445345 jg 0x445356 leaves the loop). The first
// eligible player is never itself tested against current_target (faithful quirk). Returns
// current_target unchanged if no player at all is eligible.
datum_index camera_dead_find_next_teammate(datum_index reference_player, datum_index current_target,
                                            uint8_t require_same_team)
{
    data_iterator iterator;
    player *p;
    int32_t team;
    datum_index best;

    team = require_same_team
               ? ((player *)((uint8_t *)player_data->data +
                              (reference_player & 0xffff) * sizeof(player)))->team
               : -1;

    iterator.data = player_data;
    iterator.next_index = 0;
    iterator.index = k_datum_index_none;
    best = k_datum_index_none;

    p = (player *)data_iterator_next(&iterator);
    while (p != (player *)0) {
        if (iterator.index != reference_player && p->unit != k_datum_index_none &&
            (!require_same_team || p->team == team)) {
            if (best == k_datum_index_none) {
                best = iterator.index;
            } else if ((int32_t)(iterator.index & 0xffff) > (int32_t)(current_target & 0xffff)) {
                best = iterator.index;
                break;
            }
        }
        p = (player *)data_iterator_next(&iterator);
    }

    return (best == k_datum_index_none) ? current_target : best;
}

#if 0
Original Ghidra decompilation (0x4452c0) -- UNRELIABLE, see header note; kept for reference only:

undefined4 FUN_004452c0(undefined4 param_1,undefined4 param_2)

{
  int iVar1;

  iVar1 = data_iterator_next();
  while (iVar1 != 0) {
    iVar1 = data_iterator_next();
  }
  return param_2;
}

Disassembly (objdump -d -M intel, 0x4452c0..0x445370) -- the actual control flow this rewrite
follows:

0x4452c0: mov eax, dword ptr [0x87a480]
0x4452c8: test bl, bl                          ; require_same_team
0x4452cd: je 0x4452e5                           ; skip team lookup when clear
0x4452cf: mov ecx, dword ptr [esp + 0x20]       ; reference_player
0x4452d3: mov edx, dword ptr [eax + 0x34]
0x4452df: mov ebp, dword ptr [ecx + edx + 0x20] ; ebp = player[reference_player].team
0x4452e3: jmp 0x4452e8
0x4452e5: or ebp, 0xffffffff                    ; ebp = -1 (team check disabled)
0x4452f1: or esi, 0xffffffff                    ; esi = best = -1
0x4452ff: mov dword ptr [esp + 0x14], esi       ; iterator.index = -1
0x445307: call 0x4d05d0                          ; data_iterator_next(&iterator)
0x44530c: test eax, eax
0x44530e: je 0x44535d                            ; NULL -> return current_target
0x445310: mov ecx, dword ptr [esp + 0x14]        ; ecx = iterator.index
0x445314: cmp ecx, dword ptr [esp + 0x20]        ; == reference_player ?
0x445318: je 0x445347                            ; skip self
0x44531a: cmp dword ptr [eax + 0x34], -1         ; p->unit == -1 ?
0x44531e: je 0x445347                            ; skip, no unit
0x445320: test bl, bl
0x445322: je 0x445329                            ; team check disabled -> eligible
0x445324: cmp dword ptr [eax + 0x20], ebp        ; p->team == team ?
0x445327: jne 0x445347                           ; skip, wrong team
0x445329: cmp esi, -1
0x44532c: jne 0x445332
0x44532e: mov esi, ecx                           ; first eligible player -> fallback
0x445330: jmp 0x445347
0x445332: mov edx, dword ptr [esp + 0x24]        ; current_target
0x445338: and eax, 0xffff                        ; candidate index
0x44533d: and edx, 0xffff                        ; current_target index
0x445343: cmp eax, edx
0x445345: jg 0x445356                            ; candidate index > current -> take it
0x445347: lea edi, [esp + 0xc]
0x44534b: call 0x4d05d0
0x445350: test eax, eax                          ; (Ghidra's mis-split "physical_memory_map_predict_resources")
0x445352: jne 0x445310
0x445356: mov esi, ecx                           ; best = candidate
0x445358: cmp esi, -1
0x44535d: mov eax, dword ptr [esp + 0x24]        ; return current_target (esi == -1)
0x445368: mov eax, esi                           ; return best (esi != -1)
#endif
