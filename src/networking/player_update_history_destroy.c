// player_update_history_destroy  (Ghidra: FUN_004e6b10; named per this rewrite)
// address 0x4e6b10, size 51 bytes
// name confidence: 0.4   rewrite confidence: 0.75
// evidence: out/phase4/networking_functions.md ("Frees every node in a player's update-history
// linked list plus the history container itself, likely invoked when a player structure is
// destroyed."); types/networking.h player_update_history / player_update_history_node; identical
// free-loop shape to player_update_history_free_all.c (0x4e6f20), which stops short of freeing
// the container.
// register convention: EBX -> history (unaff_EBX in the decompile).
//   // blam-cc: EBX -> history
// UNSURE: none.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"

extern void *GlobalFree(void *memory);

// Frees every node of history's linked list and then the history container itself.
void player_update_history_destroy(player_update_history *history)
    // blam-cc: EBX -> history
{
    player_update_history_node *node;
    player_update_history_node *next;

    node = history->head;
    while (node != 0) {
        next = node->next;
        GlobalFree(node);
        node = next;
    }
    history->head = 0;
    history->tail = 0;
    GlobalFree(history);
}

#if 0
Original Ghidra decompilation (0x4e6b10), from tools/pack.py 0x4e6b10:

void FUN_004e6b10(void)

{
  HGLOBAL pvVar1;
  HGLOBAL hMem;
  HGLOBAL unaff_EBX;

  hMem = *(HGLOBAL *)((int)unaff_EBX + 4);
  while (hMem != (HGLOBAL)0x0) {
    pvVar1 = *(HGLOBAL *)((int)hMem + 0x414);
    GlobalFree(hMem);
    hMem = pvVar1;
  }
  *(undefined4 *)((int)unaff_EBX + 4) = 0;
  *(undefined4 *)((int)unaff_EBX + 8) = 0;
  GlobalFree(unaff_EBX);
  return;
}
#endif
