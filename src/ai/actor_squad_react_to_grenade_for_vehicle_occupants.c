// actor_squad_react_to_grenade_for_vehicle_occupants  (Ghidra: actor_squad_react_to_grenade_for_vehicle_occupants; named for this rewrite)
// address 0x42bd70, size 201 bytes
// name confidence: 0.3   rewrite confidence: 0.9
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
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern data_array *object_data; // 0x008603b0

extern void *object_try_and_get(datum_index object_index, uint32_t type_mask); // 0x4f6ec0, ECX, stack
extern datum_index actor_find_or_create_shared_prop(datum_index object_index, datum_index actor_index,
    char create_if_missing, uint32_t flag); // 0x43eb30, EAX, stack
extern void actor_squad_react_to_grenade(datum_index actor_index, datum_index target_prop_index,
    int16_t grenade_type); // 0x42a3a0, ESI, stack, EAX

#define OBJECT_DATA(h) ((uint8_t *)((object_header *)object_data->data)[(h) & 0xffff].data)

// REWRITTEN from objdump 0x42bd70..0x42be38. EAX: a unit, EBX: the other object. The unit's driver (or the unit
//   itself) must be a biped; then the other object's actor gets its prop for that biped (0x43eb30, create 1,
//   flag 0) and reacts to it (0x42a3a0, type 0), and the biped's actor does the same for the other object. The
//   draft looked props up by actor alone and called the reaction with the prop as its actor.
// blam-cc: EAX -> vehicle_object_index, EBX -> other_object_index
void actor_squad_react_to_grenade_for_vehicle_occupants(datum_index vehicle_object_index,
                                                          datum_index other_object_index)
{
    uint8_t *vehicle;
    uint8_t *occupant;
    datum_index occupant_index;
    datum_index actor;
    datum_index prop;

    if (vehicle_object_index == k_datum_index_none) {
        return;
    }
    vehicle = (uint8_t *)object_try_and_get(vehicle_object_index, 3);
    if (vehicle == 0) {
        return;
    }
    occupant_index = ((vehicle_object *)vehicle)->unit.driver_unit_index;
    if (occupant_index == k_datum_index_none) {
        occupant_index = vehicle_object_index;
    }
    occupant = OBJECT_DATA(occupant_index);
    if (*(int16_t *)(occupant + 0xb4) != 0) {
        return;
    }
    // 0x42bdd8: the other object's actor about the biped
    actor = *(datum_index *)(OBJECT_DATA(other_object_index) + 0x1f4);
    if (actor != k_datum_index_none) {
        prop = actor_find_or_create_shared_prop(occupant_index, actor, 1, 0);
        if (prop != k_datum_index_none) {
            actor_squad_react_to_grenade(actor, prop, 0);
        }
    }
    // 0x42be06: the biped's actor about the other object
    actor = *(datum_index *)(occupant + 0x1f4);
    if (actor != k_datum_index_none) {
        prop = actor_find_or_create_shared_prop(other_object_index, actor, 1, 0);
        if (prop != k_datum_index_none) {
            actor_squad_react_to_grenade(actor, prop, 0);
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
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
