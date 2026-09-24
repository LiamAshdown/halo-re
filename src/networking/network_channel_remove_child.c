// network_channel_remove_child  (Ghidra: FUN_004dd090; named per this rewrite)
// address 0x4dd090, size 123 bytes
// name confidence: 0.4   rewrite confidence: 0.45
// evidence: out/phase4/networking_functions.md: "Finds a specific child channel in the parent's
// child table, closes its socket, deletes the child object, and clears the table entry."
// children[] (0xaa0, 16 entries) and connected (0xa98) match types/networking.h's
// network_channel exactly.
// UNSURE: network_channel_list_remove's second argument (the list to remove from) is elided at
// this call site; reconstructed as parent->listen_list, matching the equivalent call in
// network_channel_delete.c.
// register convention: parent channel in EDI (unaff_EDI). blam-cc: EDI -> parent, stack -> child

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"

extern int32_t network_channel_list_remove(network_receive_queue *entry, network_channel_list *list); // 0x441b00, this module
extern void network_channel_delete(network_channel *channel); // 0x4dcae0, this batch

// blam-cc: EDI -> parent
int32_t network_channel_remove_child(network_channel *parent, network_channel *child)
{
    int32_t i;

    i = 0;
    while (parent->children[i] == 0 || parent->children[i] != child) {
        i = i + 1;
        if (i > 0x10) {
            return 0;
        }
    }
    if (child->endpoint != 0) {
        network_channel_list_remove(parent->children[i]->endpoint, parent->listen_list); // UNSURE: list arg
    }
    if (parent->children[i]->connected == 1) {
        parent->child_busy = 0;
    }
    network_channel_delete(parent->children[i]);
    parent->children[i] = 0;
    return 1;
}

#if 0
Original Ghidra decompilation (0x4dd090):

undefined4 FUN_004dd090(int *param_1)

{
  int *piVar1;
  int iVar2;
  int unaff_EDI;

  iVar2 = 0;
  piVar1 = (int *)(unaff_EDI + 0xaa0);
  while (((int *)*piVar1 == (int *)0x0 || ((int *)*piVar1 != param_1))) {
    iVar2 = iVar2 + 1;
    piVar1 = piVar1 + 1;
    if (0x10 < iVar2) {
      return 0;
    }
  }
  if (*param_1 != 0) {
    network_channel_list_remove(**(undefined4 **)(unaff_EDI + 0xaa0 + iVar2 * 4));
  }
  if (*(char *)(*(int *)(unaff_EDI + 0xaa0 + iVar2 * 4) + 0xa98) == '\x01') {
    *(undefined1 *)(unaff_EDI + 0xae1) = 0;
  }
  network_channel_delete(*(int **)(unaff_EDI + 0xaa0 + iVar2 * 4));
  *(undefined4 *)(unaff_EDI + 0xaa0 + iVar2 * 4) = 0;
  return 1;
}
#endif
