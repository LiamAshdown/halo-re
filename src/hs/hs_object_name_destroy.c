// hs_object_name_destroy  (not a Ghidra function; the object_destroy_containing callback)
// address 0x487d90, size 63 bytes
// name confidence: 0.6  rewrite confidence: 0.95
// evidence: pushed as the hs_object_names_for_each callback by the object_destroy_containing evaluator
//   (0x47a78f push 0x487d90).
// objdump 0x487d90..0x487dce: a name index in [0, 0x200) whose object (object_name_list 0x006b8cb8) exists and
//   is not held by hs_object_hierarchy_test is deleted (tail jmp object_delete, EAX = object).
// blam-cc: stack -> object_name_index (cdecl)

#include "tags.h"
#include "memory.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern datum_index *object_name_list; // 0x006b8cb8
extern char hs_object_hierarchy_test(datum_index object_index); // 0x487c10
extern void object_delete(datum_index object_index); // 0x4f5bd0, blam-cc: EAX

void hs_object_name_destroy(int32_t object_name_index)
{
    int16_t name = (int16_t)object_name_index;
    datum_index object_index;

    if (name == -1 || name < 0 || name >= 0x200) {
        return;
    }
    object_index = object_name_list[name];
    if (object_index != k_datum_index_none && !hs_object_hierarchy_test(object_index)) {
        object_delete(object_index);
    }
}
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
