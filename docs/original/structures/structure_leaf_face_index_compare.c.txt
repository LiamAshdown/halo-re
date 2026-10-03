// structure_leaf_face_index_compare  (not a Ghidra function; the comparator structure_leaf_faces_gather_list sorts with)
// address 0x552c00, size 19 bytes
// name confidence: 0.5  rewrite confidence: 0.95
// evidence: pushed as the qsort_dword_array comparator at 0x552d04.
// objdump 0x552c00..0x552c12: returns element > other (signed), i.e. an ascending sort.
// blam-cc: stack -> element, other

#include "tags.h"
#include "cseries.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
uint8_t structure_leaf_face_index_compare(int32_t element, int32_t other)
{
    return element > other;
}
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
