// tree_insert_unique  (already named; task-provided)
// address 0x57c1a0, size 362 bytes
// name confidence: 0.5 (already carries this name; the textbook MSVC 7.1
//   `_Tree::insert(value, unique)` algorithm: descend to find the insertion parent, verify the
//   key is not already present by comparing against the adjacent node, then delegate to an
//   internal splice/rebalance helper)
// rewrite confidence: 0.4 (the descent and uniqueness-check comparisons are confirmed against
//   the decompilation, replacing the inlined byte-compare with calls to string_compare 0x57ce10
//   -- this pass -- in the direction the inlined code actually computes (search.compare(node),
//   not node.compare(search), see UNSURE); the splice/rebalance helper FUN_0057c390 is opaque,
//   not in this pass's address list)
// evidence: types/shell.h hwreq_map_node, msvc_std_map.
// register convention: stack arguments (all three recognized by Ghidra): tree (param_1,
//   msvc_std_map *), result (param_2, an out {hwreq_map_node *node; uint8_t inserted;} pair),
//   value (param_3, const hwreq_map_value_type * -- see tree_node_allocate.c for that type).
// blam-cc: tree_insert_unique(msvc_std_map *tree, hwreq_tree_insert_result *result,
//   const hwreq_map_value_type *value)
// UNSURE: the inlined comparison in the descent loop computes `search.compare(candidate)`
//   (search is "this", candidate is "s"), the opposite operand order from
//   tree_lower_bound.c's inlined compare (which computes `candidate.compare(search)`); this
//   rewrite calls string_compare with that same, verified-from-the-disassembly operand order
//   rather than assuming symmetry with tree_lower_bound.c.
// UNSURE: FUN_0057c390 (the actual node-splice-and-rebalance helper) is an opaque extern, not
//   rewritten here -- it is not in this pass's address list. Its signature is inferred as
//   (parent_holder, insert_as_left, value) -> new hwreq_map_node*.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "rasterizer.h"
#include "shell.h"

typedef struct hwreq_map_value_type {
    msvc_std_string key;
    uint32_t value;
} hwreq_map_value_type; // size 0x20, see tree_node_allocate.c

typedef struct hwreq_tree_insert_result {
    hwreq_map_node *node;
    uint8_t inserted;
} hwreq_tree_insert_result;

extern int32_t string_compare(const msvc_std_string *self, uint32_t n1, uint32_t pos,
    const char *s, uint32_t n2); // 0x57ce10, same pass
extern void tree_iterator_decrement(hwreq_map_node **iterator); // 0x57cd40, same pass
extern hwreq_map_node **tree_splice_insert(msvc_std_map *tree, hwreq_map_node *parent, hwreq_map_node **result_holder,
    uint8_t insert_as_left, const hwreq_map_value_type *value); // 0x57c390, blam-cc: EDI tree, ECX parent, stack rest

static int32_t compare_key_to_node(const msvc_std_string *search_key, const hwreq_map_node *node)
{
    const char *node_data = (node->key.capacity < 0x10) ? node->key.buffer.inline_buffer : (const char *)node->key.buffer.heap_buffer;
    return string_compare(search_key, search_key->size, 0, node_data, node->key.size);
}

void tree_insert_unique(msvc_std_map *tree, hwreq_tree_insert_result *result, const hwreq_map_value_type *value)
{
    hwreq_map_node *head = (hwreq_map_node *)tree->head;
    hwreq_map_node *parent = head;
    hwreq_map_node *where;
    uint8_t went_left = 1;

    {
        hwreq_map_node *candidate = (hwreq_map_node *)head->parent; // root
        if (candidate->is_nil == 0) {
            do {
                parent = candidate;
                went_left = compare_key_to_node(&value->key, candidate) < 0;
                candidate = went_left ? (hwreq_map_node *)candidate->left : (hwreq_map_node *)candidate->right;
            } while (candidate->is_nil == 0);
        }
    }

    where = parent; // FIXED 2026-09-28: the insertion parent is the descent node (EBX at 0x57c283 / 0x57c2da);
                    //   only the comparison uses the decremented copy
    if (went_left) {
        if (parent == (hwreq_map_node *)head->left) {
            hwreq_map_node *holder;
            hwreq_map_node *inserted = *tree_splice_insert(tree, where, &holder, 1, value);
            result->node = inserted;
            result->inserted = 1;
            return;
        }
        tree_iterator_decrement(&parent);
    }

    if (compare_key_to_node(&value->key, parent) > 0) { // node.compare(search) reused; see UNSURE below
        // UNSURE: the original compares candidate(now `parent`, possibly decremented) against
        // the search key with candidate as "this" (opposite of the descent loop's operand
        // order); string_compare(node, ..., search) < 0 means node < search, i.e.
        // compare_key_to_node(search, node) > 0. Preserved with that equivalence.
        hwreq_map_node *holder;
        hwreq_map_node *inserted = *tree_splice_insert(tree, where, &holder, went_left, value);
        result->node = inserted;
        result->inserted = 1;
        return;
    }

    result->node = parent;
    result->inserted = 0;
}

#if 0
Original Ghidra decompilation (0x57c1a0):

/* WARNING: Removing unreachable block (ram,0x0057c203) */

void tree_insert_unique(int param_1,undefined4 *param_2,undefined4 *param_3)

{
  uint uVar1;
  uint uVar2;
  int iVar3;
  byte *pbVar4;
  uint uVar5;
  undefined4 *puVar6;
  int iVar7;
  uint uVar8;
  undefined4 *puVar9;
  byte *pbVar10;
  bool bVar11;
  bool bVar12;

  iVar3 = (int)param_3;
  puVar6 = *(undefined4 **)(param_1 + 4);
  bVar11 = true;
  if (*(char *)((int)puVar6[1] + 0x2d) == '\0') {
    uVar1 = *(uint *)((int)param_3 + 0x14);
    puVar9 = (undefined4 *)puVar6[1];
    do {
      puVar6 = puVar9;
      uVar2 = puVar6[8];
      if ((uint)puVar6[9] < 0x10) {
        pbVar4 = (byte *)(puVar6 + 4);
      }
      else {
        pbVar4 = (byte *)puVar6[4];
      }
      if (uVar1 == 0) {
LAB_0057c233:
        if (uVar1 < uVar2) {
          uVar5 = 0xffffffff;
        }
        else {
          uVar5 = (uint)(uVar1 != uVar2);
        }
      }
      else {
        uVar8 = uVar1;
        if (uVar2 <= uVar1) {
          uVar8 = uVar2;
        }
        if (*(uint *)((int)param_3 + 0x18) < 0x10) {
          pbVar10 = (byte *)((int)param_3 + 4);
        }
        else {
          pbVar10 = *(byte **)((int)param_3 + 4);
        }
        bVar11 = false;
        uVar5 = 0;
        bVar12 = true;
        do {
          if (uVar8 == 0) break;
          uVar8 = uVar8 - 1;
          bVar11 = *pbVar10 < *pbVar4;
          bVar12 = *pbVar10 == *pbVar4;
          pbVar10 = pbVar10 + 1;
          pbVar4 = pbVar4 + 1;
        } while (bVar12);
        if (!bVar12) {
          uVar5 = (1 - (uint)bVar11) - (uint)(bVar11 != 0);
        }
        if (uVar5 == 0) goto LAB_0057c233;
      }
      bVar11 = (int)uVar5 < 0;
      if (bVar11) {
        puVar9 = (undefined4 *)*puVar6;
      }
      else {
        puVar9 = (undefined4 *)puVar6[2];
      }
    } while (*(char *)((int)puVar9 + 0x2d) == '\0');
  }
  param_3 = puVar6;
  if (bVar11) {
    if (puVar6 == (undefined4 *)**(int **)(param_1 + 4)) {
      puVar6 = (undefined4 *)FUN_0057c390(&param_3,1,iVar3);
      *param_2 = *puVar6;
      *(undefined1 *)(param_2 + 1) = 1;
      return;
    }
    tree_iterator_decrement();
  }
  puVar6 = param_3;
  if (*(uint *)(iVar3 + 0x18) < 0x10) {
    iVar7 = iVar3 + 4;
  }
  else {
    iVar7 = *(int *)(iVar3 + 4);
  }
  iVar7 = string_compare(0,iVar7,*(undefined4 *)(iVar3 + 0x14));
  if (iVar7 < 0) {
    puVar6 = (undefined4 *)FUN_0057c390(&param_3,bVar11,iVar3);
    *param_2 = *puVar6;
    *(undefined1 *)(param_2 + 1) = 1;
    return;
  }
  *param_2 = puVar6;
  *(undefined1 *)(param_2 + 1) = 0;
  return;
}
#endif
