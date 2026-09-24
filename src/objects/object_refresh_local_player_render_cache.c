// object_refresh_local_player_render_cache  (Ghidra: FUN_004f7370; renamed, Blam-style, not
// previously named)
// address 0x4f7370, size 72 bytes
// name confidence: 0.2 (single caller, foreign-global-heavy; name is a guess from the shape of
//   the call: it resolves a per-something record to an object_type_definition, then forwards a
//   pointer inside that definition's +0xc field to FUN_004f9b70, "returns a cached render
//   permutation index for a tag/object combination, building and caching a new one on first
//   use" per functions.md)
// rewrite confidence: 0.3
// evidence: types/objects.h object_type_definition (unknown_0c 0x0c); global 0x0069bfdc
//   object_type_definitions (12 read-only pointers); global 0x00746f8c, UNSURE: close to but
//   distinct from the established 0x00746f90 global_globals/0x00746f9c player globals in
//   src/objects/object_set_cluster_and_parent.c, not otherwise examined here.
// register convention: selector in CX. Confirmed against objdump -d -M intel bin/halo.exe:
//   0x4f737b movsx ecx,cx at entry, no stack access at all.
//   // blam-cc: CX -> selector
// UNSURE: the 0x24-byte-stride array at (DAT_00746f8c + 0x208) and its +0x20 field (an
//   object-type-definition-table index) are not otherwise established anywhere in this module;
//   preserved as raw offsets.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"

extern void *local_player_render_globals; // 0x00746f8c, UNSURE: see file header
extern object_type_definition *object_type_definitions[k_maximum_object_types]; // 0x0069bfdc

extern int32_t object_get_or_build_render_permutation(int16_t *pair, uint8_t *table_owner); // 0x4f9b70, this batch

void object_refresh_local_player_render_cache(int16_t selector) // blam-cc: CX -> selector
{
    uint8_t *table = *(uint8_t **)((uint8_t *)local_player_render_globals + 0x208);
    int16_t definition_index = *(int16_t *)(table + selector * 0x24 + 0x20);
    int16_t field_0c = *(int16_t *)((uint8_t *)object_type_definitions[definition_index] + 0xc);

    // UNSURE: the original leaves the {tag_index, name_index} pair pointer FUN_004f9b70 reads
    // through EDI entirely to caller context this fragment does not show; not resolved here.
    object_get_or_build_render_permutation((int16_t *)0, (uint8_t *)local_player_render_globals + field_0c);
}

#if 0
Original Ghidra decompilation (0x4f7370):

void FUN_004f7370(void)

{
  short in_CX;

  FUN_004f9b70(*(short *)((&PTR_PTR_0069bfdc)
                          [*(short *)(*(int *)(DAT_00746f8c + 0x208) + in_CX * 0x24 + 0x20)] + 0xc)
               + DAT_00746f8c);
  return;
}
#endif
