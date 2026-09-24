// recorded_animation_find_by_name  (Ghidra: FUN_00449f80; renamed per types notes)
// address 0x449f80, size 70 bytes
// name confidence: 0.8 (symbols/agent_phase4_cutscene.txt)   rewrite confidence: 0.85
// evidence: out/phase4/cutscene_types_notes.md "0x449f80: recorded_animation_find_by_name
// (ESI Scenario*, EBX name, returns int16). Its only caller is actor_squad_action_execute
// 0x405520." unaff_ESI+0x36c/+0x370 match Scenario.recorded_animations (a TagReflexive:
// count then pointer), stride 0x40 matches ScenarioRecordedAnimation, and the compared field
// is its leading TagString name.
// register convention: EBX = name (in_EBX), ESI = scenario (in_ESI); blam-cc: (EBX, ESI) ->
// (name, scenario), following the EAX/ECX/EDX/EBX/ESI/EDI register order (EAX/ECX/EDX unused).

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "units.h"
#include "cutscene.h"

extern int32_t __stricmp(const char *a, const char *b); // 0x628d8b

// blam-cc: EBX -> name, ESI -> scenario
// Case-insensitively searches scenario->recorded_animations for an entry whose name matches,
// returning its index or -1 if none matches.
int16_t recorded_animation_find_by_name(const char *name, Scenario *scenario)
{
    int16_t index;

    index = 0;
    if (0 < (int32_t)scenario->recorded_animations.count) {
        do {
            ScenarioRecordedAnimation *entries =
                (ScenarioRecordedAnimation *)scenario->recorded_animations.pointer;
            if (__stricmp(entries[index].name.string, name) == 0) {
                return index;
            }
            index += 1;
        } while ((int32_t)index < (int32_t)scenario->recorded_animations.count);
    }
    return -1;
}

#if 0
Original Ghidra decompilation (0x449f80):

short FUN_00449f80(void)

{
  int iVar1;
  char *unaff_EBX;
  int unaff_ESI;
  short sVar2;

  sVar2 = 0;
  if (0 < *(int *)(unaff_ESI + 0x36c)) {
    iVar1 = 0;
    do {
      iVar1 = __stricmp((char *)(iVar1 * 0x40 + *(int *)(unaff_ESI + 0x370)),unaff_EBX);
      if (iVar1 == 0) {
        return sVar2;
      }
      sVar2 = sVar2 + 1;
      iVar1 = (int)sVar2;
    } while (iVar1 < *(int *)(unaff_ESI + 0x36c));
  }
  return -1;
}
#endif
