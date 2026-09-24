// player_update_history_find_and_prune  (Ghidra: player_update_history_find_and_prune, already named)
// address 0x4e6f60, size 132 bytes
// name confidence: 0.5   rewrite confidence: 0.85
// evidence: out/phase4/networking_functions.md; types/networking.h player_update_history /
// player_update_history_node.
// register convention: none; __cdecl, all three parameters on the stack.
// UNSURE: none.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"

extern void *GlobalFree(void *memory);

// Walks history's linked list looking for the node whose update_id equals target_id. If prune
// is set and a match is found, frees every node from the head through the match (inclusive) and
// makes the following node the new head (clearing tail too if the list becomes empty). Always
// returns the node that followed the match, or NULL if no match was found.
player_update_history_node *player_update_history_find_and_prune(player_update_history *history,
    int32_t target_id, uint8_t prune)
{
    player_update_history_node *node;
    player_update_history_node *after_match;
    player_update_history_node *walk;
    player_update_history_node *next;
    int32_t matched_id;

    node = history->head;
    after_match = 0;
    walk = node;
    if (node != 0) {
        for (;;) {
            after_match = walk->next;
            if (walk->update_id == target_id) {
                break;
            }
            walk = after_match;
            if (walk == 0) {
                return 0;
            }
        }
        if (prune == 1) {
            walk = 0;
            do {
                if (node == 0) {
                    break;
                }
                matched_id = node->update_id;
                walk = node->next;
                GlobalFree(node);
                node = walk;
            } while (matched_id != target_id);
            history->head = walk;
            if (walk == 0) {
                history->tail = 0;
                return after_match;
            }
        }
    }
    return after_match;
}

#if 0
Original Ghidra decompilation (0x4e6f60), from tools/pack.py 0x4e6f60:

int * __cdecl player_update_history_find_and_prune(int param_1,int param_2,char param_3)

{
  int *hMem;
  int iVar1;
  int *piVar2;
  int *piVar3;

  hMem = *(int **)(param_1 + 4);
  piVar3 = (int *)0x0;
  piVar2 = hMem;
  if (hMem != (int *)0x0) {
    while (piVar3 = (int *)piVar2[0x105], *piVar2 != param_2) {
      piVar2 = piVar3;
      if (piVar3 == (int *)0x0) {
        return (int *)0x0;
      }
    }
    if (param_3 == '\x01') {
      piVar2 = (int *)0x0;
      do {
        if (hMem == (int *)0x0) break;
        iVar1 = *hMem;
        piVar2 = (int *)hMem[0x105];
        GlobalFree(hMem);
        hMem = piVar2;
      } while (iVar1 != param_2);
      *(int **)(param_1 + 4) = piVar2;
      if (piVar2 == (int *)0x0) {
        *(undefined4 *)(param_1 + 8) = 0;
        return piVar3;
      }
    }
  }
  return piVar3;
}
#endif
