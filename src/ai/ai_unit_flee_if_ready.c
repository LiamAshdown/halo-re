// ai_unit_flee_if_ready  (Ghidra: ai_unit_flee_if_ready; named for this rewrite)
// address 0x434df0, size 93 bytes
// name confidence: 0.35   rewrite confidence: 0.9 (VERIFIED against objdump; record argument FIXED)
// evidence: for one unit's controlling actor (unit_data.actor_index, object+0x1f4), switches
// it into mode 0xb (types/ai.h _actor_mode_flee) via actor_set_mode if the readiness
// predicate actor_squad_action_status_broadcast (outside this rewrite's range) is satisfied. Sibling of
// ai_reference_flee_if_ready (0x434d90, this batch), which does the same for every actor of
// a packed reference.
// register convention: Ghidra recognized param_1 as an ordinary parameter and left the unit
// index unresolved, in ECX.
//   // blam-cc: ECX -> unit_index, stack -> readiness_param

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "units.h"
#include "ai.h"
#include "fn_ai.h"

extern object *object_try_and_get(datum_index object_index, uint32_t type_mask); // 0x4f6ec0


// blam-cc: ECX -> unit_index, stack -> readiness_param
void ai_unit_flee_if_ready(datum_index unit_index, uint32_t readiness_param)
{
    object *unit_object = object_try_and_get(unit_index, 3); // biped or vehicle

    if (unit_object != 0) {
        unit_data *unit = (unit_data *)((uint8_t *)unit_object + k_unit_data_offset);
        uint8_t mode_data[0x84]; // FIXED (0x434e25 / 0x434e3c): the record ESI fills is the flee mode data

        if (unit->actor_index != (datum_index)k_datum_index_none &&
            (uint8_t)actor_squad_action_status_broadcast(unit->actor_index, (int16_t)readiness_param,
                (int16_t *)mode_data) != 0) {
            actor_set_mode(unit->actor_index, _actor_mode_flee, mode_data);
        }
    }
}

#if 0
Original Ghidra decompilation (0x434df0):

void FUN_00434df0(undefined4 param_1)

{
  char cVar1;
  int iVar2;
  int in_ECX;
  undefined1 local_84 [132];

  if ((((in_ECX != -1) && (iVar2 = object_try_and_get(3), iVar2 != 0)) &&
      (*(int *)(iVar2 + 500) != -1)) &&
     (cVar1 = FUN_00407140(*(undefined4 *)(iVar2 + 500),param_1), cVar1 != '\0')) {
    actor_set_mode(*(undefined4 *)(iVar2 + 500),0xb,local_84);
  }
  return;
}
#endif
