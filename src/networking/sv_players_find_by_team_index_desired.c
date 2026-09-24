// sv_players_find_by_team_index_desired  (Ghidra: FUN_004e2c10; named per this rewrite)
// address 0x4e2c10, size 87 bytes
// name confidence: 0.4   rewrite confidence: 0.55
// evidence: out/phase4/networking_functions.md: "Iterates player data looking for an entry
// whose team/slot byte matches a caller value, used as a helper by the sv_players scoreboard
// printer." The iterated data_array is types/game.h's `player_data` (0x0087a480), and the
// matched byte at +0x67 is exactly `player::team_index_desired`. The two-return-path shape
// (`or eax,0xffffffff` vs `mov eax,[esp+0xc]`) is confirmed by disassembly (objdump -d -M intel,
// bin/halo.exe): the "not found" path returns a literal -1, while the "found" path reads back
// the iterator's own `index` field, which data_iterator_next has by then updated to the
// matching player's datum_index -- i.e. this function returns the found player's datum_index,
// or k_datum_index_none if none matched. Modelled on the identical data_iterator idiom already
// established in src/ai/actor_delete.c (data_array/next_index=0/index=k_datum_index_none, one
// data_iterator_next per iteration).
// register convention: ESI = team_index_desired (int8_t, implicit).
//   // blam-cc: ESI -> team_index_desired

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"

extern data_array *player_data; // 0x0087a480, stride 0x200 (types/game.h `player`)
extern void *data_iterator_next(data_iterator *iterator); // 0x4d05d0, memory module

// blam-cc: ESI -> team_index_desired
// Walks every live player looking for one whose team_index_desired matches, returning its
// datum_index, or k_datum_index_none if no player matches.
uint32_t sv_players_find_by_team_index_desired(int8_t team_index_desired)
{
    data_iterator iterator;
    player *p;

    iterator.data = player_data;
    iterator.next_index = 0;
    iterator.index = (datum_index)k_datum_index_none;

    p = (player *)data_iterator_next(&iterator);
    while (p != 0) {
        if (team_index_desired == p->team_index_desired) {
            return (uint32_t)(datum_index)iterator.index;
        }
        p = (player *)data_iterator_next(&iterator);
    }
    return 0xffffffff;
}

#if 0
Original Ghidra decompilation (0x4e2c10), from tools/pack.py 0x4e2c10:

undefined4 FUN_004e2c10(void)

{
  int iVar1;
  int unaff_ESI;

  iVar1 = data_iterator_next();
  while( true ) {
    if (iVar1 == 0) {
      return 0xffffffff;
    }
    if (unaff_ESI == *(char *)(iVar1 + 0x67)) break;
    iVar1 = data_iterator_next();
  }
  return 0xffffffff;
}

Disassembly (objdump -d -M intel, bin/halo.exe) recovering the true return values Ghidra's
pseudo-C collapsed to a literal -1 on both paths:
  4e2c2c: mov DWORD PTR [esp+0xc],0xffffffff  ; iterator.index = k_datum_index_none (init)
  4e2c38: call 0x4d05d0                        ; data_iterator_next(&iterator)
  4e2c56: or eax,0xffffffff                    ; not-found path: return -1
  4e2c5e: mov eax,DWORD PTR [esp+0xc]          ; found path: return iterator.index (updated by
                                                ; the data_iterator_next call that found it)
#endif
