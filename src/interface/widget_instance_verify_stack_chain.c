// widget_instance_verify_stack_chain  (Ghidra: FUN_00499aa0, unnamed;
// out/phase2/results/interface_01.json names it widget_instance_verify_stack_chain, conf=0.3)
// address 0x499aa0, size 33 bytes
// name confidence: 0.3   rewrite confidence: 0.55
// evidence: phase-2 evidence; walks widget_instance::parent (0x30) upward from a starting node,
// checking at each step that the ancestor's focused_child (0x38) is the node just left --
// exactly the parent-chain consistency check types/interface.h attributes to the sibling
// function widget_instance_is_top_of_stack @0x499cb0, but without that function's list-type
// special case, and starting one level higher (the caller passes the CHILD already found under
// the cursor as the loop's first "child").
// register convention: starting node in ECX (in_ECX), unresolved register read.
// blam-cc: ECX -> node
// UNSURE: the return value is a full EAX/uint per Ghidra, not the bool widget_instance_is_top_of
// _stack uses; its own caller in interface_tick only ever tests the low byte (`cVar7 == '\0'`),
// so the failure path's upper 24 bits (leftover ancestor-pointer bits, masked with 0xffffff00 in
// the original) are dead and are not reproduced here -- returning a plain uint8_t is behaviourally
// identical to every observed caller.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"
#include "fn_interface.h"

// blam-cc: ECX -> node
// True if every ancestor from `node` up to the tree root has its focused_child pointing back at
// the node it was reached from (i.e. the whole chain above `node` is internally consistent).
uint8_t widget_instance_verify_stack_chain(widget_instance *node)
{
    widget_instance *ancestor;

    if (node == (widget_instance *)0) {
        return 0;
    }
    ancestor = node->parent;
    while (ancestor != (widget_instance *)0) {
        if (ancestor->focused_child != node) {
            return 0;
        }
        node = ancestor;
        ancestor = node->parent;
    }
    return 1;
}

#if 0
Original Ghidra decompilation (0x499aa0):

uint FUN_00499aa0(void)

{
  uint uVar1;
  uint in_EAX;
  uint in_ECX;

  if (in_ECX != 0) {
    uVar1 = *(uint *)(in_ECX + 0x30);
    while( true ) {
      in_EAX = uVar1;
      if (in_EAX == 0) {
        return 1;
      }
      if (*(uint *)(in_EAX + 0x38) != in_ECX) break;
      uVar1 = *(uint *)(in_EAX + 0x30);
      in_ECX = in_EAX;
    }
  }
  return in_EAX & 0xffffff00;
}
#endif
