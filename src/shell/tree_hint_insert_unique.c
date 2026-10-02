// tree_hint_insert_unique  (orphan pass 4: FUN_0057ba50, no Ghidra name)
// address 0x57ba50, size 369 bytes
// name confidence: 0.4 (the standard MSVC 7.1 `_Tree::insert(iterator hint, value)`
//   hinted-insert optimization: tries to confirm the hint position is correct in O(1) via a
//   handful of key comparisons against the hint's neighbors, and only falls back to the full
//   tree_insert_unique 0x57c1a0 descent when the hint doesn't pan out)
// rewrite confidence: 0.3 (control flow and every register role are confirmed against objdump;
//   tree_splice_insert 0x57c390's exact contract -- specifically, that it writes the resulting
//   node through `result_holder` as a side effect, since every call site here discards its EAX
//   return and reads `*result_holder` afterward instead -- is inferred, not directly evidenced,
//   see UNSURE)
// evidence: types/shell.h hwreq_map_node, msvc_std_map. Every call site confirmed via objdump
//   --start-address=0x57ba50 --stop-address=0x57bbd0.
// register convention (confirmed via objdump): EAX = tree (msvc_std_map *), stack argument =
//   hint (hwreq_map_node *), EBX = value (const hwreq_map_value_type *, doubling as
//   `&value->key` since key is value's first member), ESI = result_holder
//   (hwreq_map_node ** -- both this function's own effective return slot, `mov eax,esi` before
//   every `ret`, and the argument threaded through to tree_splice_insert / tree_insert_unique).
// UNSURE: tree_splice_insert (0x57c390) is opaque, not in this pass's address list; every call
//   site here discards its EAX return, implying it writes the new node into `*result_holder`
//   directly. tree_insert_unique.c and its own callers instead read tree_splice_insert's EAX
//   return; both usages are modeled as compatible (it does both).

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "rasterizer.h"
#include "shell.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
typedef struct hwreq_map_value_type {
    msvc_std_string key;
    uint32_t value;
} hwreq_map_value_type; // size 0x20, see tree_node_allocate.c

extern hwreq_map_node **tree_splice_insert(msvc_std_map *tree, hwreq_map_node *parent, hwreq_map_node **result_holder,
    uint8_t insert_as_left, const hwreq_map_value_type *value); // 0x57c390, blam-cc: EDI tree, ECX parent, stack rest
extern uint8_t hwreq_map_key_less_than(const msvc_std_string *self, const msvc_std_string *other); // 0x57bbd0, same pass
extern void tree_iterator_decrement(hwreq_map_node **iterator); // 0x57cd40, same pass
extern void tree_iterator_increment(hwreq_map_node **iterator); // 0x57c5e0, same pass
extern int32_t string_compare(const msvc_std_string *self, uint32_t n1, uint32_t pos,
    const char *s, uint32_t n2); // 0x57ce10, same pass
extern void tree_insert_unique(msvc_std_map *tree, void *result, const hwreq_map_value_type *value); // 0x57c1a0, same pass

// blam-cc: EAX -> tree, ESI -> result_holder, EBX -> value, stack -> hint
// FIXED 2026-09-28 (retail-independence loop): tree_splice_insert also takes the tree (EDI) and the parent (ECX); per
//   objdump 0x57ba50..0x57bbbe the parent is the head for an empty tree, the hint before begin(), the rightmost node
//   after the end, and in the neighbour cases the predecessor (when ITS right child is nil, as a right child) or the
//   hint (left), resp. the hint (when its right child is nil, as a right child) or the successor (left). The old
//   predecessor case tested hint->left instead of predecessor->right.
hwreq_map_node *tree_hint_insert_unique(msvc_std_map *tree, hwreq_map_node **result_holder,
                                         hwreq_map_node *hint, const hwreq_map_value_type *value)
{
    hwreq_map_node *head = (hwreq_map_node *)tree->head;
    const msvc_std_string *value_key = &value->key;

    if (tree->size == 0) {
        return *tree_splice_insert(tree, head, result_holder, 1, value);
    }

    if (hint == (hwreq_map_node *)head->left) { // hint == begin(): try inserting before the first element
        const char *hint_data = (hint->key.capacity < 0x10) ? hint->key.buffer.inline_buffer : (const char *)hint->key.buffer.heap_buffer;
        if (string_compare(value_key, value_key->size, 0, hint_data, hint->key.size) < 0) {
            return *tree_splice_insert(tree, hint, result_holder, 1, value);
        }
        goto full_search;
    }

    if (hint == head) { // hint == end(): try inserting after the last element
        if (hwreq_map_key_less_than(&((hwreq_map_node *)head->right)->key, value_key)) {
            return *tree_splice_insert(tree, (hwreq_map_node *)head->right, result_holder, 0, value);
        }
        goto full_search;
    }

    if (hwreq_map_key_less_than(value_key, &hint->key)) {
        // value < *hint: try just after the predecessor of hint
        hwreq_map_node *predecessor = hint;
        tree_iterator_decrement(&predecessor);
        if (hwreq_map_key_less_than(&predecessor->key, value_key)) {
            if (((hwreq_map_node *)predecessor->right)->is_nil != 0) {
                return *tree_splice_insert(tree, predecessor, result_holder, 0, value);
            }
            return *tree_splice_insert(tree, hint, result_holder, 1, value);
        }
    } else if (hwreq_map_key_less_than(&hint->key, value_key)) {
        // *hint < value: try just before the successor of hint
        hwreq_map_node *successor = hint;
        tree_iterator_increment(&successor);
        if (successor == head || hwreq_map_key_less_than(value_key, &successor->key)) {
            if (((hwreq_map_node *)hint->right)->is_nil != 0) {
                return *tree_splice_insert(tree, hint, result_holder, 0, value);
            }
            return *tree_splice_insert(tree, successor, result_holder, 1, value);
        }
    }

full_search:
    {
        uint8_t local_result[8];
        tree_insert_unique(tree, local_result, value);
        *result_holder = *(hwreq_map_node **)local_result;
        return *result_holder;
    }
}

#if 0
Original Ghidra decompilation (0x57ba50):

void FUN_0057ba50(int *param_1)

{
  char cVar1;
  int in_EAX;
  int *piVar2;
  int iVar3;
  undefined4 *puVar4;
  undefined4 unaff_EBX;
  undefined4 *unaff_ESI;
  undefined1 local_8 [8];

  if (*(int *)(in_EAX + 8) == 0) {
    FUN_0057c390(unaff_ESI,1,unaff_EBX);
    return;
  }
  if (param_1 == (int *)**(int **)(in_EAX + 4)) {
    if ((uint)param_1[9] < 0x10) {
      piVar2 = param_1 + 4;
    }
    else {
      piVar2 = (int *)param_1[4];
    }
    iVar3 = string_compare(0,piVar2,param_1[8]);
    if (iVar3 < 0) {
LAB_0057baa8:
      FUN_0057c390(unaff_ESI,1,unaff_EBX);
      return;
    }
  }
  else if (param_1 == *(int **)(in_EAX + 4)) {
    cVar1 = hwreq_map_key_less_than();
    if (cVar1 != '\0') {
      FUN_0057c390(unaff_ESI,0,unaff_EBX);
      return;
    }
  }
  else {
    cVar1 = hwreq_map_key_less_than();
    if (cVar1 != '\0') {
      tree_iterator_decrement();
      cVar1 = hwreq_map_key_less_than();
      if (cVar1 != '\0') {
        if (*(char *)(param_1[2] + 0x2d) != '\0') {
          FUN_0057c390(unaff_ESI,0,unaff_EBX);
          return;
        }
        goto LAB_0057baa8;
      }
    }
    cVar1 = hwreq_map_key_less_than();
    if ((cVar1 != '\0') &&
       ((tree_iterator_increment(), param_1 == *(int **)(in_EAX + 4) ||
        (cVar1 = hwreq_map_key_less_than(), cVar1 != '\0')))) {
      if (*(char *)(param_1[2] + 0x2d) == '\0') {
        FUN_0057c390(unaff_ESI,1,unaff_EBX);
        return;
      }
      FUN_0057c390(unaff_ESI,0,unaff_EBX);
      return;
    }
  }
  puVar4 = (undefined4 *)tree_insert_unique(in_EAX,local_8,unaff_EBX);
  *unaff_ESI = *puVar4;
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
