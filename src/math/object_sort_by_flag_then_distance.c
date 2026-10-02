// object_sort_by_flag_then_distance  (Ghidra: object_sort_by_flag_then_distance, already named)
// address 0x433c70, size 71 bytes
// name confidence: 0.5   rewrite confidence: 0.85
// evidence: out/phase2/results/ai_06.json ("Pure two-int-pointer comparator (no implicit
//   registers) used directly as the qsort callback in FUN_00433cc0: orders by boolean field at
//   offset+8 first (false before true), then ascending by float distance at offset+4."); tools
//   pack.py currently attributes this address to module=math (0.8). out/phase4/ai_types_notes.md
//   groups it with the plain math/qsort-comparator functions living inside the ai address range
//   ("Library / other-module functions inside the address range ... qsort comparators"), and
//   the record it sorts is the AI-side ai_nearby_actor_candidate (see below). Kept in math
//   per the module list; it is a generic comparator, not AI logic.
// register convention: __cdecl with two stack pointer parameters (param_1, param_2), matching
//   the standard qsort comparator signature int(*)(const void*, const void*). No registers.
//   // blam-cc: stack -> (a, b)
//
// record type: ai_nearby_actor_candidate (types/ai.h, size 0xc). The only caller,
//   ai_object_process_nearby_actors @0x433cc0 (src/ai/ai_object_process_nearby_actors.c),
//   fills a 0x40-entry stack array of these and passes it to qsort with width 0xc. An
//   earlier draft of this file declared a private copy of the same layout as a TYPES-GAP;
//   it was dropped in favour of the ai.h struct so the record has one definition.
// Parameters are const void * (the qsort comparator type) so the definition agrees with the
//   extern in src/ai/ai_object_process_nearby_actors.c.

#include "crt.h"
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"
#include "ai.h"

// qsort comparator: records with is_type_9 clear sort before records with it set; within
// each group, ascending distance_squared. The flag test is a raw byte compare (any nonzero
// byte counts as set, and two different nonzero bytes are treated as unequal flags, exactly
// as the `cmp al,[edx+8]` / `setne` sequence does).
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
int object_sort_by_flag_then_distance(const void *a_record, const void *b_record)
{
    const ai_nearby_actor_candidate *a = (const ai_nearby_actor_candidate *)a_record;
    const ai_nearby_actor_candidate *b = (const ai_nearby_actor_candidate *)b_record;

    if (a->is_type_9 != b->is_type_9) {
        return (int)(a->is_type_9 != 0) * 2 - 1;
    }
    if (a->distance_squared < b->distance_squared) {
        return -1;
    }
    if (b->distance_squared < a->distance_squared) {
        return 1;
    }
    return 0;
}

#if 0
Original Ghidra decompilation (0x433c70):

int __cdecl object_sort_by_flag_then_distance(int param_1,int param_2)

{
  if (*(char *)(param_1 + 8) != *(char *)(param_2 + 8)) {
    return (uint)(*(char *)(param_1 + 8) != '\0') * 2 + -1;
  }
  if (*(float *)(param_1 + 4) < *(float *)(param_2 + 4)) {
    return -1;
  }
  if (*(float *)(param_2 + 4) < *(float *)(param_1 + 4)) {
    return 1;
  }
  return 0;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
