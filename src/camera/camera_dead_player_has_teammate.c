// camera_dead_player_has_teammate  (Ghidra: FUN_00445240; renamed, Blam-style, not previously named)
// address 0x445240, size 121 bytes
// name confidence: 0.4   rewrite confidence: 0.85
// evidence: out/phase4/camera_types_notes.md "the target cycling in 0x445240 / 0x4452c0 walks
//   the players data_array comparing player+0x20 (team) and +0x34 (unit)"; the Ghidra decompile
//   for this function is unreliable (it invents a nonexistent `-1` sentinel comparison), so this
//   rewrite follows objdump instead: it walks every player with a fresh data_iterator and
//   returns true the first time it finds one (other than `reference_player` itself) whose team
//   (types/game.h player::team, +0x20) matches reference_player's team.
// review fixes (phase 4 gate): the result is a byte (0x4452ab mov al,bl / 0x4452b5 mov al,1;
//   the only caller 0x4454eb uses AL alone), and the team is compared as the full int32 at
//   player +0x20 (0x445262 / 0x445296), not truncated to int16. The binary also stores the
//   iterator self-check word data ^ 0x69746572 at iterator +0x0c (0x44527e), which is
//   types/memory.h data_iterator.signature; data_iterator_next never reads it.
// register convention: single cdecl stack parameter (confirmed with objdump: loaded from
//   [esp+0x1c] right after the prologue, and the function returns with a bare `ret`).
//   // blam-cc: stack -> reference_player
// reconciled: R16 data_iterator is 0x10 bytes (int16 next_index, +0x0c signature = data ^ 'iter'); the inline constructor now stores the signature like the original

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include <stdint.h>
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern data_array *player_data; // 0x0087a480, stride 0x200 (no types/players.h yet)
extern void *data_iterator_next(data_iterator *iterator); // 0x4d05d0, memory module

// blam-cc: stack -> reference_player; result in AL
// True if some player other than `reference_player` shares its team.
uint8_t camera_dead_player_has_teammate(datum_index reference_player)
{
    data_iterator iterator;
    player *p;
    int32_t team;

    team = ((player *)((uint8_t *)player_data->data +
                        (reference_player & 0xffff) * sizeof(player)))->team;

    iterator.data = player_data;
    iterator.next_index = 0;
    iterator.index = k_datum_index_none;
    iterator.signature = (uint32_t)(uintptr_t)iterator.data ^ k_data_iterator_signature;

    p = (player *)data_iterator_next(&iterator);
    while (p != (player *)0) {
        if (iterator.index != reference_player && p->team == team) {
            return 1;
        }
        p = (player *)data_iterator_next(&iterator);
    }
    return 0;
}

#if 0
Original Ghidra decompilation (0x445240) -- UNRELIABLE, see header note; kept for reference only:

undefined4 FUN_00445240(uint param_1)

{
  int iVar1;
  int iVar2;

  iVar1 = *(int *)((param_1 & 0xffff) * 0x200 + 0x20 + *(int *)(DAT_0087a480 + 0x34));
  iVar2 = data_iterator_next();
  while( true ) {
    if (iVar2 == 0) {
      return 0;
    }
    if ((param_1 != 0xffffffff) && (*(int *)(iVar2 + 0x20) == iVar1)) break;
    iVar2 = data_iterator_next();
  }
  return 1;
}

Disassembly (objdump -d -M intel, 0x445240..0x4452b8) -- the actual control flow this rewrite
follows (the iterator's returned handle is compared against reference_player, NOT against -1):

0x445240: sub esp, 0x10
0x445243: mov eax, dword ptr [0x87a480]
0x445248: mov edx, dword ptr [eax + 0x34]
0x44524b: push ebx
0x44524c: push ebp
0x44524d: mov ebp, dword ptr [esp + 0x1c]      ; reference_player (single stack arg)
0x445251: mov ecx, ebp
0x445253: push esi
0x445254: and ecx, 0xffff
0x44525a: mov dword ptr [esp + 0xc], eax       ; iterator.data = player_data
0x44525e: push edi
0x44525f: shl ecx, 9                            ; index * sizeof(player)
0x445262: mov esi, dword ptr [ecx + edx + 0x20] ; esi = player[index].team
0x445271: mov word ptr [esp + 0x14], bx         ; iterator.next_index = 0 (bx == 0)
0x445276: mov dword ptr [esp + 0x18], 0xffffffff ; iterator.index = -1
0x445282: call 0x4d05d0                          ; data_iterator_next(&iterator)   (EDI -> &iterator)
0x445287: cmp eax, ebx
0x445289: je 0x4452a8                            ; NULL -> return 0
0x445290: cmp dword ptr [esp + 0x18], ebp        ; iterator.index == reference_player ?
0x445294: je 0x44529b                            ; skip self, keep scanning
0x445296: cmp dword ptr [eax + 0x20], esi        ; p->team == team ?
0x445299: je 0x4452b2                            ; match -> return 1
0x44529b: call 0x4d05d0
0x4452a4: cmp eax, ebx
0x4452a6: jne 0x445290
0x4452a8: mov al, bl                             ; return 0
0x4452b2: mov al, 1                              ; return 1
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
