// hs_object_name_cache_validate  (Ghidra: FUN_00487d20)
// address 0x487d20, size 98 bytes
// name confidence: 0.25 (out/phase4/hs_functions.md: "Validates a cached object-slot reference
//   against a type-mask test, resetting it via a fallback path when invalid or absent")
// rewrite confidence: 0.9 (VERIFIED against objdump)
// evidence: types/hs.h globals list (object_name_list, 0x006b8cb8, objects module, 0x200
//   entries); this module's hs_object_hierarchy_test (0x487c10) as the "type-mask test".
// register convention: none (void); object_names index is the recognized stack parameter
//   (param_1).
// UNSURE: object_delete and object_new_from_scenario_name are called with zero visible arguments; `param_1` is
//   forwarded to both here on the assumption they operate on the same object_names slot, but
//   this is inferred, not observed.

#include "tags.h"
#include "memory.h"
#include "hs.h"
#include "fn_hs.h"


extern void object_delete(datum_index object_index); // 0x4f5bd0, blam-cc: EAX (0x487d56: mov eax,edi = the cached object)
extern void object_new_from_scenario_name(int16_t object_name_index); // objects module, 0x4f7370, UNSURE args

extern datum_index *object_name_list; // 0x006b8cb8, 0x200 entries

// If `object_name_index` currently caches a resolved object that fails
// hs_object_hierarchy_test, invalidates it via object_delete. Either way, if the slot is still
// out of range or unresolved afterward, falls through to the object_new_from_scenario_name resolve path.
void hs_object_name_cache_validate(int16_t object_name_index)
{
    datum_index cached;

    if (object_name_index == -1) {
        return;
    }
    if (-1 < object_name_index) {
        if (object_name_index < 0x200) {
            cached = object_name_list[object_name_index];
            if (cached != k_datum_index_none && hs_object_hierarchy_test(cached) == 0) {
                object_delete(cached); // FIXED: the draft passed the name index
            }
        }
        if (-1 < object_name_index && object_name_index < 0x200 &&
            object_name_list[object_name_index] != k_datum_index_none) {
            return;
        }
    }
    object_new_from_scenario_name(object_name_index);
}

#if 0
Original Ghidra decompilation (0x487d20):

void FUN_00487d20(short param_1)

{
  int iVar1;
  char cVar2;

  if (param_1 == -1) {
    return;
  }
  if (-1 < param_1) {
    if (((param_1 < 0x200) && (iVar1 = *(int *)(DAT_006b8cb8 + param_1 * 4), iVar1 != -1)) &&
       (cVar2 = FUN_00487c10(iVar1), cVar2 == '\0')) {
      FUN_004f5bd0();
    }
    if (((-1 < param_1) && (param_1 < 0x200)) && (*(int *)(DAT_006b8cb8 + param_1 * 4) != -1)) {
      return;
    }
  }
  FUN_004f7370();
  return;
}
#endif
