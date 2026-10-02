// object_dump_compare_by_total_size  (Ghidra: object_dump_compare_by_total_size, already named)
// address 0x4fa3a0, size 35 bytes
// name confidence: 0.6 (already carries this name from an earlier phase; matches
//   functions.md's summary: "qsort comparator that orders object/definition memory-dump
//   records by their accumulated total size field")
// rewrite confidence: 0.85
// evidence: types/objects.h object_memory_dump_record (total_size 0x08).
// register convention: __cdecl, both parameters on the stack (Ghidra already recovered this
//   fully, including the calling convention).

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

int object_dump_compare_by_total_size(const object_memory_dump_record *a, const object_memory_dump_record *b)
{
    if (a->total_size < b->total_size) {
        return 1;
    }
    return (a->total_size <= b->total_size) ? 0 : -1;
}

#if 0
Original Ghidra decompilation (0x4fa3a0):

int __cdecl object_dump_compare_by_total_size(int param_1,int param_2)

{
  if (*(int *)(param_1 + 8) < *(int *)(param_2 + 8)) {
    return 1;
  }
  return (*(int *)(param_1 + 8) <= *(int *)(param_2 + 8)) - 1;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
