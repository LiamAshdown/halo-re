// map_list_free_all  (Ghidra: chimera__free_map_index, renamed per types/interface.h)
// address 0x495260, size 89 bytes
// name confidence: 0.6   rewrite confidence: 0.85
// evidence: types/interface.h map_list_entry struct comment lists this address as
// map_list_free_all; out/phase4/interface_functions.md "Frees every map-list entry's allocated
// path buffer plus the map-list array itself and resets the map list to empty."
// register convention: none (void).

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"

extern map_list_entry *map_list;  // 0x00712dcc
extern int32_t map_list_count;    // 0x00712dd0
extern int32_t map_list_capacity; // 0x00712dd4

extern void *GlobalFree(void *mem);

// Frees every entry's path buffer, then the map_list array itself, and resets the list to empty.
void map_list_free_all(void)
{
    int32_t i;

    for (i = 0; i < map_list_count; i++) {
        GlobalFree(map_list[i].path);
    }
    GlobalFree(map_list);
    map_list = (map_list_entry *)0;
    map_list_count = 0;
    map_list_capacity = 0;
}

#if 0
Original Ghidra decompilation (0x495260):

void chimera__free_map_index(void)

{
  int iVar1;
  int iVar2;

  iVar1 = 0;
  if (0 < DAT_00712dd0) {
    iVar2 = 0;
    do {
      GlobalFree(*(HGLOBAL *)(iVar2 + (int)DAT_00712dcc));
      iVar1 = iVar1 + 1;
      iVar2 = iVar2 + 0xc;
    } while (iVar1 < DAT_00712dd0);
  }
  GlobalFree(DAT_00712dcc);
  DAT_00712dcc = (HGLOBAL)0x0;
  DAT_00712dd0 = 0;
  DAT_00712dd4 = 0;
  return;
}
#endif
