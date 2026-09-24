// player_update_history_free_all  (Ghidra: player_update_history_free_all, already named)
// address 0x4e6f20, size 49 bytes
// name confidence: 0.6   rewrite confidence: 0.85
// evidence: out/phase4/networking_functions.md; types/networking.h player_update_history /
// player_update_history_node (head at +0x04, tail at +0x08, node next pointer at +0x414).
// register convention: none; param_1 is a plain stack argument (__cdecl, one pointer param).
// UNSURE: none.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"

extern void *GlobalFree(void *memory);

// Frees every node of a player_update_history's linked list and clears its head/tail.
void player_update_history_free_all(player_update_history *history)
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
}

#if 0
Original Ghidra decompilation (0x4e6f20), from tools/pack.py 0x4e6f20:

void player_update_history_free_all(int param_1)

{
  HGLOBAL pvVar1;
  HGLOBAL hMem;

  hMem = *(HGLOBAL *)(param_1 + 4);
  while (hMem != (HGLOBAL)0x0) {
    pvVar1 = *(HGLOBAL *)((int)hMem + 0x414);
    GlobalFree(hMem);
    hMem = pvVar1;
  }
  *(undefined4 *)(param_1 + 4) = 0;
  *(undefined4 *)(param_1 + 8) = 0;
  return;
}
#endif
