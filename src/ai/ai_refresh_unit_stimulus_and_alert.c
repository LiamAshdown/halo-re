// ai_refresh_unit_stimulus_and_alert  (Ghidra: ai_refresh_unit_stimulus_and_alert; named for this rewrite)
// address 0x42c2a0, size 202 bytes
// name confidence: 0.3   rewrite confidence: 0.4
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

extern ai_globals *ai_globals_ptr;   // 0x00880354
extern data_array *object_data;      // 0x008603b0
extern game_time_globals *game_time; // 0x006f1d6c

extern void ai_alert_actors_in_grenade_radius(datum_index object_index); // 0x42a0e0, not yet rewritten; UNSURE signature

// blam-cc: EDX -> object_index, BX -> priority, DI -> stimulus_value
// If priority is positive and stimulus_value outranks (or the previous stimulus has aged
// past 30 ticks) object_index's currently recorded stimulus, restamps it and forwards the
// alert: directly if object_index is a biped, or to every biped passenger if it is a
// vehicle.
void ai_refresh_unit_stimulus_and_alert(datum_index object_index, int16_t priority, int16_t stimulus_value)
{
    object *obj;
    object *passenger_obj;
    datum_index passenger_index;
    unit_data *unit;

    if (!ai_globals_ptr->actors_valid || object_index == (datum_index)k_datum_index_none || priority <= 0) {
        return;
    }
    obj = ((object_header *)object_data->data)[object_index & 0xffff].data;
    unit = (unit_data *)((uint8_t *)obj + k_unit_data_offset);

    if (unit->unknown_21c < stimulus_value ||
        (int32_t)unit->unknown_220 + 0x1e < game_time->game_time) {
        unit->unknown_220 = game_time->game_time;
        unit->unknown_21c = stimulus_value;

        if (obj->type == _object_type_vehicle) {
            passenger_index = obj->first_child_object;
            if (passenger_index != (datum_index)k_datum_index_none) {
                do {
                    passenger_obj = ((object_header *)object_data->data)[passenger_index & 0xffff].data;
                    if (passenger_obj->type == _object_type_biped) {
                        ai_alert_actors_in_grenade_radius(passenger_index);
                    }
                    passenger_index = passenger_obj->next_object;
                } while (passenger_index != (datum_index)k_datum_index_none);
            }
        } else if (obj->type == _object_type_biped) {
            ai_alert_actors_in_grenade_radius(object_index);
        }
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
