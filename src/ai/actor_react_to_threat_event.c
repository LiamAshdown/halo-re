// actor_react_to_threat_event  (Ghidra: actor_react_to_threat_event; named for this rewrite)
// address 0x42be40, size 324 bytes
// name confidence: 0.4   rewrite confidence: 0.6
// evidence: phase-4 summary ("handles a unit reacting to being hit or threatened by a given
// amount/type, choosing and broadcasting an appropriate AI communication (pain/notice)
// event"); ai_communication_broadcast's 7-argument shape already established elsewhere in
// this module matches this call site exactly.
// register convention: plain __cdecl, all six arguments on the stack.
// blam-cc: stack -> self_object_index, other_object_index, event_kind, magnitude,
// extra_param, suppress_vehicle_relay

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "units.h"
#include "ai.h"

extern data_array *object_data; // 0x008603b0

extern void *object_try_and_get(datum_index object_index, int32_t kind); // 0x4f6ec0
extern void actor_mark_prop_seen_with_delta(datum_index squad_index, uint32_t key, float delta); // 0x428840, not yet rewritten
extern uint8_t teams_are_enemies(int16_t team_a, int16_t team_b); // 0x45bd50, CX, DX
extern void ai_communication_broadcast(int32_t event_code, datum_index unit_index, datum_index object_a, int32_t reason, datum_index object_b, datum_index object_c, uint32_t *extra_data); // 0x42d340
extern void team_pair_override_refresh(void); // 0x45c090, not yet rewritten (game module), UNSURE args

// blam-cc: stack -> self_object_index, other_object_index, event_kind, magnitude,
// extra_param, suppress_vehicle_relay
// Resolves other_object_index to a vehicle occupant (its gunner unless event_kind == 9, else
// its driver, falling back to other_object_index itself) to get a relationship object, then
// relays a pain/notice tick to self_object_index's controlling actor (via actor_mark_prop_seen_with_delta)
// unless suppressed or event_kind == 1. Broadcasts a communication event graded by whether
// the two objects are the same, hostile, or friendly, and by whether magnitude is small.
void actor_react_to_threat_event(datum_index self_object_index, datum_index other_object_index,
                                  int32_t event_kind, real magnitude, uint32_t extra_param,
                                  uint8_t suppress_vehicle_relay)
{
    object *self_obj;
    object *relationship_obj;
    datum_index relationship_object_index;
    object *vehicle_obj;
    unit_data *vehicle_unit;
    datum_index actor_object_index;
    int32_t reason;
    int32_t event_code;

    self_obj = ((object_header *)object_data->data)[self_object_index & 0xffff].data;
    relationship_object_index = (datum_index)k_datum_index_none;
    relationship_obj = 0;

    if (other_object_index != (datum_index)k_datum_index_none) {
        vehicle_obj = object_try_and_get(other_object_index, 3);
        if (vehicle_obj != 0) {
            vehicle_unit = (unit_data *)((uint8_t *)vehicle_obj + k_unit_data_offset);
            relationship_object_index = (datum_index)k_datum_index_none;
            if ((int16_t)event_kind != 9) {
                relationship_object_index = vehicle_unit->gunner_unit_index;
            }
            if (relationship_object_index == (datum_index)k_datum_index_none) {
                relationship_object_index = vehicle_unit->driver_unit_index;
                if (relationship_object_index == (datum_index)k_datum_index_none) {
                    relationship_object_index = other_object_index;
                }
                if (relationship_object_index == (datum_index)k_datum_index_none) {
                    goto no_relationship_object;
                }
            }
            relationship_obj = ((object_header *)object_data->data)[relationship_object_index & 0xffff].data;
        }
    }
no_relationship_object:

    if (suppress_vehicle_relay == 0 && (int16_t)event_kind != 1) {
        actor_object_index = ((unit_data *)((uint8_t *)self_obj + k_unit_data_offset))->actor_index;
        if (actor_object_index != (datum_index)k_datum_index_none) {
            actor_mark_prop_seen_with_delta(actor_object_index, magnitude, extra_param);
        }
    }

    reason = 0;
    if (self_object_index == relationship_object_index) {
        reason = 1;
    } else if (relationship_obj != 0) {
        reason = (teams_are_enemies(*(int16_t *)((uint8_t *)relationship_obj + 0xb8), *(int16_t *)((uint8_t *)self_obj + 0xb8)) != 0) + 2; // 0x42bf00
    }

    if (suppress_vehicle_relay == 0 && reason == 2) {
        reason = 2;
        event_code = 3;
    } else {
        if (magnitude < 0.3f) {
            goto skip_broadcast;
        }
        event_code = 2;
    }
    ai_communication_broadcast(event_code, self_object_index, relationship_object_index, reason,
                                event_kind, (datum_index)k_datum_index_none, 0);
skip_broadcast:
    if (relationship_obj != 0) {
        team_pair_override_refresh();
    }
}

#if 0
Original Ghidra decompilation (0x42be40):

void FUN_0042be40(uint param_1,uint param_2,undefined4 param_3,float param_4,undefined4 param_5,
                 char param_6)

{
  int iVar1;
  char cVar2;
  int iVar3;
  uint uVar4;
  int iVar5;
  undefined4 uVar6;

  iVar5 = *(int *)(DAT_008603b0 + 0x34);
  iVar1 = *(int *)(iVar5 + 8 + (param_1 & 0xffff) * 0xc);
  uVar4 = 0xffffffff;
  if (param_2 == 0xffffffff) {
LAB_0042bea5:
    iVar5 = 0;
  }
  else {
    iVar3 = object_try_and_get(3);
    if (iVar3 == 0) goto LAB_0042bea5;
    if (((short)param_3 == 9) || (uVar4 = *(uint *)(iVar3 + 0x328), uVar4 == 0xffffffff)) {
      uVar4 = *(uint *)(iVar3 + 0x324);
      if (*(uint *)(iVar3 + 0x324) == 0xffffffff) {
        uVar4 = param_2;
      }
      if (uVar4 == 0xffffffff) goto LAB_0042bea5;
    }
    iVar5 = *(int *)(iVar5 + 8 + (uVar4 & 0xffff) * 0xc);
  }
  if (((param_6 == '\0') && ((short)param_3 != 1)) && (iVar1 = *(int *)(iVar1 + 500), iVar1 != -1))
  {
    FUN_00428840(iVar1,param_4,param_5);
  }
  cVar2 = '\0';
  if (param_1 == uVar4) {
    cVar2 = '\x01';
  }
  else if (iVar5 != 0) {
    cVar2 = FUN_0045bd50();
    cVar2 = (cVar2 != '\0') + '\x02';
  }
  if ((param_6 == '\0') && (cVar2 == '\x02')) {
    cVar2 = '\x02';
    uVar6 = 3;
  }
  else {
    if (param_4 < 0.3) goto LAB_0042bf64;
    uVar6 = 2;
  }
  ai_communication_broadcast(uVar6,param_1,uVar4,cVar2,param_3,0xffffffff,0);
LAB_0042bf64:
  if (iVar5 != 0) {
    FUN_0045c090();
  }
  return;
}
#endif
