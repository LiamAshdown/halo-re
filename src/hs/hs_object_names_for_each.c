// hs_object_names_for_each  (Ghidra: hs_object_names_for_each, already named)
// address 0x487ef0, size 79 bytes
// name confidence: 0.5 (out/phase4/hs_functions.md: "Enumerates every scenario object_names entry
//   and invokes a callback for each one that satisfies a predicate check")
// rewrite confidence: 0.7
// evidence: types/tags.h Scenario::object_names (TagReflexive, ScenarioObjectName size 0x24),
//   matching out/phase4/hs_types_notes.md's documented 0x204/0x208 offsets exactly.
// register convention: none (void); callback pointer is the recognized stack parameter (param_1).

#include "tags.h"
#include "memory.h"
#include "hs.h"

extern int32_t FUN_00625430(ScenarioObjectName *entry); // UNSURE: predicate, module unknown, 0x625430

extern Scenario *global_scenario; // 0x00746f8c

// Invokes `callback(index)` for every entry of Scenario::object_names that satisfies the
// FUN_00625430 predicate, in ascending index order.
void hs_object_names_for_each(void (*callback)(int32_t index))
{
    ScenarioObjectName *object_names;
    int32_t index;

    object_names = (ScenarioObjectName *)global_scenario->object_names.pointer;
    for (index = 0; index < (int32_t)global_scenario->object_names.count; index++) {
        if (FUN_00625430(&object_names[index]) != 0) {
            callback(index);
        }
    }
}

#if 0
Original Ghidra decompilation (0x487ef0):

void hs_object_names_for_each(code *param_1)

{
  int iVar1;
  int iVar2;
  int iVar3;

  iVar1 = DAT_00746f8c;
  iVar3 = 0;
  if (0 < *(int *)(DAT_00746f8c + 0x204)) {
    iVar2 = 0;
    do {
      iVar2 = FUN_00625430(*(int *)(iVar1 + 0x208) + iVar2 * 0x24);
      if (iVar2 != 0) {
        (*param_1)(iVar3);
      }
      iVar3 = iVar3 + 1;
      iVar2 = (int)(short)iVar3;
    } while (iVar2 < *(int *)(iVar1 + 0x204));
  }
  return;
}
#endif
