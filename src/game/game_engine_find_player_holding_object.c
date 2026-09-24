// game_engine_find_player_holding_object  (Ghidra: FUN_00468b50; named per this rewrite)
// address 0x468b50, size 150 bytes
// name confidence: 0.4   rewrite confidence: 0.6
// evidence: CORRECTED against the disassembly (objdump -d -M intel --start-address=0x468b50
//   --stop-address=0x468be6): out/phase4/game_functions.md's own note flags this correctly --
//   "the decompiled result is always -1, indicating an unmodeled output register". The match
//   path (`je 0x468bd3`) returns `mov eax,[esp+0x14]`, which at that stack depth is the local
//   data_iterator's own `index` field (the current player's datum_index that
//   data_iterator_next just returned), NOT the literal constant -1 Ghidra's decompile shows on
//   that branch; only the "list exhausted, nothing matched" path genuinely returns -1. types/
//   game.h player::unit (0x34), player_data (0x0087a480); types/units.h unit_data.weapons[4]
//   (0x2f8, k_maximum_weapons_per_unit).
// register convention: target object handle in EBX (unaff_EBX).
//   // blam-cc: EBX -> target_object

// CORRECTED (phase 4 review): types/units.h unit_data starts at object + k_unit_data_offset
// (0x1f4), so a unit_data * built straight from the object pointer reads every field 0x1f4
// bytes too low. The cast below adds the extension offset.
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "units.h"
#include "game.h"

extern data_array *player_data;    // 0x0087a480
extern data_array *object_headers; // 0x008603b0

extern void *data_iterator_next(data_iterator *iterator); // 0x4d05d0

// blam-cc: EBX -> target_object
// Walks every in-use player with a valid controlled unit and returns the handle of the first
// one whose unit has `target_object` in one of its four weapon slots; returns k_datum_index_none
// if no player is holding it.
datum_index game_engine_find_player_holding_object(datum_index target_object)
{
    data_iterator iter;
    player *p;

    iter.data = player_data;
    iter.next_index = 0;
    iter.index = (datum_index)0xffffffff;

    p = (player *)data_iterator_next(&iter);
    while (p != (player *)0) {
        if (p->unit != (datum_index)0xffffffff) {
            object *unit_obj = ((object_header *)object_headers->data)[(uint32_t)p->unit & 0xffff].data;
            unit_data *unit = (unit_data *)((uint8_t *)unit_obj + k_unit_data_offset);
            int32_t i;
            for (i = 0; i < k_maximum_weapons_per_unit; i++) {
                if (unit->weapons[i] == target_object) {
                    return iter.index;
                }
            }
        }
        p = (player *)data_iterator_next(&iter);
    }
    return (datum_index)0xffffffff;
}

#if 0
Original Ghidra decompilation (0x468b50), from tools/pack.py 0x468b50:

undefined4 FUN_00468b50(void)

{
  int iVar1;
  int iVar2;
  int iVar3;
  int *piVar4;
  int unaff_EBX;

  iVar2 = data_iterator_next();
  iVar1 = DAT_008603b0;
  if (iVar2 == 0) {
    return 0xffffffff;
  }
  do {
    if (*(uint *)(iVar2 + 0x34) != 0xffffffff) {
      iVar3 = 0;
      piVar4 = (int *)(*(int *)(*(int *)(iVar1 + 0x34) + 8 +
                               (*(uint *)(iVar2 + 0x34) & 0xffff) * 0xc) + 0x2f8);
      do {
        if (*piVar4 == unaff_EBX) {
          return 0xffffffff;
        }
        iVar3 = iVar3 + 1;
        piVar4 = piVar4 + 1;
      } while (iVar3 < 4);
    }
    iVar2 = data_iterator_next();
  } while (iVar2 != 0);
  return 0xffffffff;
}
#endif

// Raw disassembly (objdump -d -M intel --start-address=0x468b50 --stop-address=0x468be6),
// showing the match-path return that Ghidra rendered incorrectly as a literal -1:
//
//   468bb0: cmp DWORD PTR [ecx],ebx
//   468bb2: je 0x468bd3
//   ...
//   468bd3: mov eax,[esp+0x14]      ; == iterator.index at this stack depth, not a constant
//   468bd7: pop esi
//   468bd8: pop edi
//   468bd9: pop ebp
//   468bda: add esp,0x10
//   468bdd: ret
