// tree_lower_bound  (already named; task-provided)
// address 0x57c530, size 165 bytes
// name confidence: 0.5 (already carries this name; the textbook MSVC 7.1
//   `_Tree::lower_bound` binary-search-down-the-tree walk, returning the first node whose key
//   is not less than the search key, or `end()` (the head sentinel) if none)
// rewrite confidence: 0.5 (tree traversal confirmed against the decompilation; the inlined
//   byte-by-byte string comparison is replaced with a call to string_compare 0x57ce10, this
//   pass, which implements the identical algorithm -- see hwreq_map_key_less_than.c for the
//   same substitution)
// evidence: types/shell.h hwreq_map_node, msvc_std_map (head 0x04).
// register convention: EAX = tree (msvc_std_map *), ECX = const msvc_std_string *search_key.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "rasterizer.h"
#include "shell.h"

extern int32_t string_compare(const msvc_std_string *self, uint32_t n1, uint32_t pos,
    const char *s, uint32_t n2); // 0x57ce10, same pass

// blam-cc: EAX -> tree, ECX -> search_key
hwreq_map_node *tree_lower_bound(msvc_std_map *tree, const msvc_std_string *search_key)
{
    hwreq_map_node *head = (hwreq_map_node *)tree->head;
    hwreq_map_node *best = head;
    hwreq_map_node *candidate = (hwreq_map_node *)head->parent; // root

    const char *search_data = (search_key->capacity < 0x10) ? search_key->buffer.inline_buffer : (const char *)search_key->buffer.heap_buffer;

    if (candidate->is_nil == 0) {
        do {
            /* candidate->key < *search_key ? */
            if (string_compare(&candidate->key, candidate->key.size, 0, search_data, search_key->size) < 0) {
                candidate = (hwreq_map_node *)candidate->right;
            } else {
                best = candidate;
                candidate = (hwreq_map_node *)candidate->left;
            }
        } while (candidate->is_nil == 0);
    }

    return best;
}

#if 0
Original Ghidra decompilation (0x57c530):

undefined4 * tree_lower_bound(void)

{
  uint uVar1;
  uint uVar2;
  int in_EAX;
  int in_ECX;
  uint uVar3;
  uint uVar4;
  undefined4 *puVar5;
  undefined4 *puVar6;
  byte *pbVar7;
  byte *pbVar8;
  bool bVar9;
  bool bVar10;
  undefined4 *local_c;

  local_c = *(undefined4 **)(in_EAX + 4);
  if (*(char *)((int)local_c[1] + 0x2d) == '\0') {
    uVar1 = *(uint *)(in_ECX + 0x14);
    puVar5 = (undefined4 *)local_c[1];
    do {
      pbVar8 = (byte *)(in_ECX + 4);
      if (0xf < *(uint *)(in_ECX + 0x18)) {
        pbVar8 = *(byte **)(in_ECX + 4);
      }
      uVar2 = puVar5[8];
      if (uVar2 == 0) {
LAB_0057c5a7:
        if (uVar1 <= uVar2) {
          uVar4 = (uint)(uVar2 != uVar1);
          goto LAB_0057c5b4;
        }
LAB_0057c5b6:
        puVar6 = (undefined4 *)puVar5[2];
      }
      else {
        uVar3 = uVar2;
        if (uVar1 <= uVar2) {
          uVar3 = uVar1;
        }
        if ((uint)puVar5[9] < 0x10) {
          pbVar7 = (byte *)(puVar5 + 4);
        }
        else {
          pbVar7 = (byte *)puVar5[4];
        }
        bVar9 = false;
        uVar4 = 0;
        bVar10 = true;
        do {
          if (uVar3 == 0) break;
          uVar3 = uVar3 - 1;
          bVar9 = *pbVar7 < *pbVar8;
          bVar10 = *pbVar7 == *pbVar8;
          pbVar7 = pbVar7 + 1;
          pbVar8 = pbVar8 + 1;
        } while (bVar10);
        if (!bVar10) {
          uVar4 = (1 - (uint)bVar9) - (uint)(bVar9 != 0);
        }
        if (uVar4 == 0) goto LAB_0057c5a7;
LAB_0057c5b4:
        if ((int)uVar4 < 0) goto LAB_0057c5b6;
        puVar6 = (undefined4 *)*puVar5;
        local_c = puVar5;
      }
      puVar5 = puVar6;
    } while (*(char *)((int)puVar6 + 0x2d) == '\0');
  }
  return local_c;
}
#endif
