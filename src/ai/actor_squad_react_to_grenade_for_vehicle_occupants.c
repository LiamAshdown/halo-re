// actor_squad_react_to_grenade_for_vehicle_occupants  (Ghidra: actor_squad_react_to_grenade_for_vehicle_occupants; named for this rewrite)
// address 0x42bd70, size 201 bytes
// name confidence: 0.3   rewrite confidence: 0.4
// evidence: phase-4 summary ("when an object resolves to a biped, removes the current-weapon
// object of it and of another (register-passed) unit from the AI's recognized-object
// cache"); actor_find_or_create_shared_prop(actor_index, 1, 0) matches the established "resolve or create this
// actor's prop" call shape already used identically in actor_target_data_acquire.c and
// actor_find_nearest_grenade_ally.c.
// register convention: EAX -> vehicle_object_index, EBX -> other_object_index.
// blam-cc: EAX -> vehicle_object_index, EBX -> other_object_index
//
// UNSURE: actor_squad_react_to_grenade (0x42a3a0, outside this batch) may be misnamed by an
// earlier heuristic pass -- nothing in this function's own body is grenade-specific -- but
// it is used here under its current name per the module's naming convention. UNSURE: the
// field this function reads at object+500 (0x1f4) is types/units.h's unit_data.actor_index,
// which requires the object to actually have a unit extension; not re-verified here.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "units.h"
#include "ai.h"

extern data_array *object_data; // 0x008603b0

extern void *object_try_and_get(datum_index object_index, int32_t kind); // 0x4f6ec0
extern datum_index actor_find_or_create_shared_prop(datum_index actor_index, uint32_t flag_a, uint32_t flag_b); // 0x43eb30, UNSURE signature
extern void actor_squad_react_to_grenade(datum_index prop_index); // 0x42a3a0, not yet rewritten; UNSURE name

// blam-cc: EAX -> vehicle_object_index, EBX -> other_object_index
// Resolves vehicle_object_index to a vehicle object (falling back to vehicle_object_index
// itself as a plain object if its driver seat is empty or it isn't a vehicle), and if that
// object is a biped, resolves both other_object_index's and the vehicle occupant's own
// controlling actor to a prop via actor_find_or_create_shared_prop and reacts each one to the grenade.
void actor_squad_react_to_grenade_for_vehicle_occupants(datum_index vehicle_object_index,
                                                          datum_index other_object_index)
{
    object *vehicle_obj;
    datum_index occupant_index;
    object *occupant_obj;
    datum_index occupant_actor_index;
    datum_index other_actor_index;
    datum_index resolved;

    if (vehicle_object_index == (datum_index)k_datum_index_none) {
        return;
    }
    vehicle_obj = object_try_and_get(vehicle_object_index, 3);
    if (vehicle_obj == 0) {
        return;
    }

    occupant_index = ((unit_data *)((uint8_t *)vehicle_obj + k_unit_data_offset))->driver_unit_index;
    if (occupant_index == (datum_index)k_datum_index_none) {
        occupant_index = vehicle_object_index;
    }

    occupant_obj = ((object_header *)object_data->data)[occupant_index & 0xffff].data;
    if (occupant_obj->type != _object_type_biped) {
        return;
    }

    other_actor_index = ((unit_data *)((uint8_t *)((object_header *)object_data->data)[other_object_index & 0xffff].data
                                        + k_unit_data_offset))->actor_index;
    if (other_actor_index != (datum_index)k_datum_index_none) {
        resolved = actor_find_or_create_shared_prop(other_actor_index, 1, 0);
        if (resolved != (datum_index)k_datum_index_none) {
            actor_squad_react_to_grenade(resolved);
        }
    }

    occupant_actor_index = ((unit_data *)((uint8_t *)occupant_obj + k_unit_data_offset))->actor_index;
    if (occupant_actor_index != (datum_index)k_datum_index_none) {
        resolved = actor_find_or_create_shared_prop(occupant_actor_index, 1, 0);
        if (resolved != (datum_index)k_datum_index_none) {
            actor_squad_react_to_grenade(resolved);
        }
    }
}

#if 0
Original Ghidra decompilation (0x42bd70):

void FUN_0042bd70(void)

{
  uint in_EAX;
  int iVar1;
  uint uVar2;
  int iVar3;
  uint unaff_EBX;

  if (((in_EAX != 0xffffffff) && (iVar1 = object_try_and_get(3), iVar1 != 0)) &&
     ((uVar2 = *(uint *)(iVar1 + 0x324), *(uint *)(iVar1 + 0x324) != 0xffffffff ||
      (uVar2 = in_EAX, in_EAX != 0xffffffff)))) {
    iVar1 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (uVar2 & 0xffff) * 0xc);
    if (*(short *)(iVar1 + 0xb4) == 0) {
      iVar3 = *(int *)(*(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (unaff_EBX & 0xffff) * 0xc) +
                      500);
      if ((iVar3 != -1) && (iVar3 = FUN_0043eb30(iVar3,1,0), iVar3 != -1)) {
        actor_squad_react_to_grenade(iVar3);
      }
      iVar1 = *(int *)(iVar1 + 500);
      if ((iVar1 != -1) && (iVar1 = FUN_0043eb30(iVar1,1,0), iVar1 != -1)) {
        actor_squad_react_to_grenade(iVar1);
      }
    }
  }
  return;
}
#endif
