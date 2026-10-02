// hwreq_map_destruct  (orphan pass 4: FUN_00579fe0, no Ghidra name)
// address 0x579fe0, size 44 bytes
// name confidence: 0.5 (MSVC 7.1 std::map<string, hwreq_property_set*>::~map(): erases the
//   whole tree via erase(begin(), end()), frees the head sentinel, and clears the map's
//   head/size fields; out/phase4/shell_types_notes.md's "map _Tidy")
// rewrite confidence: 0.5 (standard library code, confirmed against the decompilation)
// evidence: types/shell.h msvc_std_map (head 0x04, size 0x08).
// register convention: EAX = this (msvc_std_map *, in_EAX).
// blam-cc: hwreq_map_destruct(msvc_std_map *this /*EAX*/)

#include "crt.h"
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "rasterizer.h"
#include "shell.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern hwreq_map_node **tree_erase_range(hwreq_map_node **out, hwreq_map_node *first,
    hwreq_map_node *last, msvc_std_map *tree); // 0x57c310, same pass

void hwreq_map_destruct(msvc_std_map *self)
{
    hwreq_map_node *dummy_out;
    hwreq_map_node *head = (hwreq_map_node *)self->head;

    tree_erase_range(&dummy_out, (hwreq_map_node *)head->left, head, self);

    free((void *)self->head);
    self->head = 0;
    self->size = 0;
}

#if 0
Original Ghidra decompilation (0x579fe0):

void FUN_00579fe0(void)

{
  int in_EAX;
  undefined1 local_4 [4];

  tree_erase_range(local_4,**(undefined4 **)(in_EAX + 4),*(undefined4 **)(in_EAX + 4));
  _free(*(void **)(in_EAX + 4));
  *(undefined4 *)(in_EAX + 4) = 0;
  *(undefined4 *)(in_EAX + 8) = 0;
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
