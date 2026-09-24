// tag_iterator_next
// address 0x4425d0, size 96 bytes
// name confidence: 0.8 (already named by Ghidra; matches types/cache.h's tag_iterator struct,
// whose field layout was recovered specifically from this function and its two callers)
// rewrite confidence: 0.75
// evidence: types/cache.h tag_iterator (next_index 0x04, group_tag filter 0x10) and tag_instance.
// register convention: tag_iterator *iterator in ESI (unaff_ESI).
// UNSURE: the `entry != NULL` check below (`piVar4 != (int *)0x0` in the original) can only be
// false if tag_instances itself were a null-ish address minus a small offset, which never
// happens once a map is loaded; preserved literally rather than dropped as dead code.

#include "tags.h"
#include "cache.h"

extern cache_file_tag_header *tag_header; // 0x006a8954
extern tag_instance *tag_instances;       // 0x0087bc14

// blam-cc: iterator in ESI
// Advances iterator->next_index through the resident tag table, returning the tag id of the next
// entry whose group, parent group or grandparent group matches iterator->group_tag (-1 matches
// everything), or k_datum_index_none once the table is exhausted.
datum_index tag_iterator_next(tag_iterator *iterator)
{
    tag_instance *entry;

    if (iterator->next_index >= tag_header->tag_count) {
        return (datum_index)0xffffffff;
    }

    for (;;) {
        entry = &tag_instances[iterator->next_index];
        iterator->next_index = iterator->next_index + 1;

        if (entry != 0 &&
            ((int32_t)iterator->group_tag == -1 ||
             (int32_t)iterator->group_tag == (int32_t)entry->group_tag ||
             (int32_t)iterator->group_tag == (int32_t)entry->parent_group_tag ||
             (int32_t)iterator->group_tag == (int32_t)entry->grandparent_group_tag)) {
            break;
        }
        if (iterator->next_index >= tag_header->tag_count) {
            return (datum_index)0xffffffff;
        }
    }
    return entry->tag_id;
}

#if 0
Original Ghidra decompilation (0x4425d0):

int tag_iterator_next(void)

{
  int iVar1;
  int iVar2;
  int iVar3;
  int *piVar4;
  int unaff_ESI;

  iVar2 = DAT_0087bc14;
  iVar3 = -1;
  if ((int)*(short *)(unaff_ESI + 4) < *(int *)(DAT_006a8954 + 0xc)) {
    while( true ) {
      piVar4 = (int *)(*(short *)(unaff_ESI + 4) * 0x20 + iVar2);
      *(short *)(unaff_ESI + 4) = *(short *)(unaff_ESI + 4) + 1;
      if ((piVar4 != (int *)0x0) &&
         ((((iVar1 = *(int *)(unaff_ESI + 0x10), iVar1 == -1 || (iVar1 == *piVar4)) ||
           (iVar1 == piVar4[1])) || (iVar1 == piVar4[2])))) break;
      if (*(int *)(DAT_006a8954 + 0xc) <= (int)*(short *)(unaff_ESI + 4)) {
        return iVar3;
      }
    }
    iVar3 = piVar4[3];
  }
  return iVar3;
}
#endif
