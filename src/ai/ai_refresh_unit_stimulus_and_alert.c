// ai_refresh_unit_stimulus_and_alert  (Ghidra: ai_refresh_unit_stimulus_and_alert; named for this rewrite)
// address 0x42c2a0, size 202 bytes
// name confidence: 0.3   rewrite confidence: 0.95
// evidence: phase-4 summary ("refreshes a stimulus timestamp on a unit and forwards the
// notification to its vehicle passengers, or directly if it is a biped"); types/objects.h
// next_object (0x114) / first_child_object (0x118) account for the vehicle passenger walk.
// register convention: EDX -> object_index, BX -> priority, DI -> stimulus_value (all
// unresolved registers in Ghidra's own decompile, i.e. not shown as parameters at all).
// blam-cc: EDX -> object_index, BX -> priority, DI -> stimulus_value
//
// UNSURE: ai_alert_actors_in_grenade_radius (0x42a0e0, outside this batch) is shown taking a
// stack argument at one call site and none at the other; both are modeled here as passing
// the relevant object index, on the assumption that Ghidra simply failed to recognize the
// second site's argument the same way it has elsewhere in this module.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "units.h"
#include "game.h"
#include "ai.h"

extern ai_globals *ai_globals_ptr;
extern data_array *object_data;      // 0x008603b0
extern game_time_globals *game_time; // 0x006f1d6c

extern void ai_alert_actors_in_grenade_radius(datum_index source_unit_index, int16_t stimulus, int16_t gate); // 0x42a0e0

#define OBJECT_DATA(h) ((uint8_t *)((object_header *)object_data->data)[(h) & 0xffff].data)

// REWRITTEN from objdump 0x42c2a0..0x42c369. When the stimulus outranks the unit's (+0x21c) or that one is over
//   30 ticks old (+0x220), restamps it and alerts nearby actors (0x42a0e0 with (object, stimulus, priority)):
//   for a vehicle each biped among its children (+0x118, then +0x114), for a biped the unit itself.
// blam-cc: EDX -> object_index, BX -> priority, DI -> stimulus_value
void ai_refresh_unit_stimulus_and_alert(datum_index object_index, int16_t priority, int16_t stimulus_value)
{
    uint8_t *obj;
    int32_t now;

    if (!ai_globals_ptr->actors_valid || object_index == k_datum_index_none || priority <= 0) {
        return;
    }
    obj = OBJECT_DATA(object_index);
    now = game_time->game_time;
    if (!(stimulus_value > *(int16_t *)(obj + 0x21c)) && !(now > *(int32_t *)(obj + 0x220) + 0x1e)) {
        return;
    }
    *(int32_t *)(obj + 0x220) = now;
    *(int16_t *)(obj + 0x21c) = stimulus_value;
    if (((object *)obj)->type == 1) {
        datum_index child;

        for (child = ((object *)obj)->first_child_object; child != k_datum_index_none;) {
            uint8_t *c = OBJECT_DATA(child);

            if (((struct object *)c)->type == 0) {
                ai_alert_actors_in_grenade_radius(child, stimulus_value, priority);
            }
            child = ((struct object *)c)->next_object;
        }
    } else if (((object *)obj)->type == 0) {
        ai_alert_actors_in_grenade_radius(object_index, stimulus_value, priority);
    }
}

#if 0
Original Ghidra decompilation (0x42c2a0):

void FUN_0042c2a0(void)

{
  int iVar1;
  uint uVar2;
  uint in_EDX;
  short unaff_BX;
  short unaff_DI;

  if (((*(char *)(DAT_00880354 + 1) != '\0') && (in_EDX != 0xffffffff)) && (0 < unaff_BX)) {
    iVar1 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (in_EDX & 0xffff) * 0xc);
    if ((*(short *)(iVar1 + 0x21c) < unaff_DI) ||
       (*(int *)(iVar1 + 0x220) + 0x1e < *(int *)(DAT_006f1d6c + 0xc))) {
      *(int *)(iVar1 + 0x220) = *(int *)(DAT_006f1d6c + 0xc);
      *(short *)(iVar1 + 0x21c) = unaff_DI;
      if (*(short *)(iVar1 + 0xb4) == 1) {
        uVar2 = *(uint *)(iVar1 + 0x118);
        if (uVar2 != 0xffffffff) {
          do {
            iVar1 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (uVar2 & 0xffff) * 0xc);
            if (*(short *)(iVar1 + 0xb4) == 0) {
              ai_alert_actors_in_grenade_radius(uVar2);
            }
            uVar2 = *(uint *)(iVar1 + 0x114);
          } while (uVar2 != 0xffffffff);
          return;
        }
      }
      else if (*(short *)(iVar1 + 0xb4) == 0) {
        ai_alert_actors_in_grenade_radius();
      }
    }
  }
  return;
}
#endif
