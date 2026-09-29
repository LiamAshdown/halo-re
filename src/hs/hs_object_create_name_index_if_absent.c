// hs_object_create_name_index_if_absent  (not a Ghidra function; the per-name callback object_create_containing
//   hands to hs_object_names_for_each)
// address 0x487cf0, size 43 bytes
// name confidence: 0.8  rewrite confidence: 0.9
// evidence: pushed as the callback at 0x47a6ef (object_create_containing); it had no C.
// objdump 0x487cf0..0x487d1a: a name index other than -1 whose slot is out of range or empty (object_name_list
//   0x006b8cb8) is created by a tail jump to object_new_from_scenario_name (CX = the index) -- object_create's
//   own test.
// blam-cc: stack -> name_index (cdecl)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "hs.h"
#include "fn_hs.h"

extern datum_index *object_name_list; // 0x006b8cb8
extern datum_index object_new_from_scenario_name(int16_t name_index); // 0x4f7370, blam-cc: CX

void hs_object_create_name_index_if_absent(int32_t name_index)
{
    int16_t name = (int16_t)name_index;

    if (name != -1 && (name < 0 || name >= 0x200 || object_name_list[name] == k_datum_index_none)) {
        object_new_from_scenario_name(name);
    }
}
