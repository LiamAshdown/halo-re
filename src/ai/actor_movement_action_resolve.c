// actor_movement_action_resolve  (Ghidra: actor_movement_action_resolve, already named)
// address 0x41a460, size 1253 bytes
// name confidence: 0.6   rewrite confidence: 0.75
// evidence: the four movement setters at 0x417610..0x417910 and actor_movement_action_stop
// all tail-call it after writing active_movement; it turns the action's type-specific
// parameters into a concrete world destination (actor.unknown_488), a destination surface
// index (unknown_494) and a destination radius (unknown_498), then asks the pathfinder
// whether that destination is reachable and stores the answer in movement_action_complete.
// register convention: all three arguments are genuine stack parameters
// (objdump: [esp+0x100f0 / 0x100f4 / 0x100f8] behind the 0x100ec frame).
// blam-cc: stack -> actor_index, record_distance, context
//
// Facts recovered from the disassembly that the Ghidra listing below hides:
//  - the jump table at 0x41a948 maps active_movement.type 2/3/4/5 to 0x41a528 / 0x41a611 /
//    0x41a57b / 0x41a670, i.e. type 3 reads the encounter ScenarioFiringPosition block and
//    type 4 reads the squad ScenarioMovePosition block (see the note in src/ai/README.md).
//  - result is the byte at [esp+0xe] and is preloaded with 1 at function entry, which is
//    why Ghidra can print return 1 on the two early-out paths.
//  - previous_destination is the 12 bytes at [esp+0x18], copied out of unknown_488 before
//    it is overwritten, and is only used for the 0.01 squared-distance early-out.
//  - param_3 is a path_find_context pointer, not an int: when it is non-null the function
//    reuses the caller context instead of building a request and running a fresh search
//    (objdump 0x41a7b8: the same EAX that held param_3 is handed to 0x43a730, which is the
//    slot the other branch fills with lea eax,[esp+0x80]).
// UNSURE: the register arguments of 0x43a070 / 0x43a190 / 0x43a4d0 / 0x43a730 /
// path_find_context_init are read off this call site only; those five functions have not
// been rewritten yet.
// UNSURE: the call to actor_target_get_relationship_object in the type-5 case drops every
// argument in Ghidra and sets none in the disassembly either -- it is invoked purely for its
// side effect on the prop, with whatever EAX/ECX/EDX the prop lookup left behind.
// reconciled: R06 0x00746f9c is ScenarioStructureBSP *global_structure_bsp (was extern int32_t bsp_generation); ai.h path_find_context/actor_movement_context bsp_generation -> structure_bsp, bsp_index -> collision_bsp

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "ai.h"

extern data_array *actor_data;       // 0x00880360
extern data_array *prop_data;        // 0x008802c0
extern tag_instance *tag_instances;  // 0x0087bc14
extern Scenario *global_scenario;    // 0x00746f8c
extern ScenarioStructureBSP *global_structure_bsp; // 0x00746f9c, scenario.h (formerly bsp_generation)

extern float vector3d_distance_squared(const real_point3d *a, const real_point3d *b); // 0x401020, not yet rewritten: squared distance, EAX -> a, ECX -> b
extern real vector3d_distance(const real_point3d *a, const real_point3d *b); // 0x4088b0, EAX -> a, ECX -> b
extern uint8_t actor_movement_check_arrival(datum_index actor_index);   // 0x416700, this module, EAX -> actor_index
extern void actor_movement_action_complete(datum_index actor_index);    // 0x41a430, this module, EAX -> actor_index
extern void actor_build_path_find_request(datum_index actor_index, path_find_request *request); // 0x41a9c0, this module, EAX/EBX
extern uint8_t actor_movement_flying_needs_steering(datum_index actor_index, const real_point3d *destination,
                                                   float *out_avoidance_distance); // 0x41aab0, this module, EAX/ECX/EDI
extern void actor_target_get_relationship_object(datum_index target_prop_index); // 0x41f3a0, this module,
                                                 // blam-cc: EAX -> target_prop_index
extern void path_find_set_avoid_sphere(path_find_request *request, const real_point3d *center, float radius,
                         datum_index object_index, float weight);       // 0x43a070, not yet rewritten, EAX -> request, ECX -> center
extern uint8_t path_find_validate_and_record_goal(int32_t generation, const real_point3d *from, int32_t unknown,
                            const real_point3d *to);                    // 0x43a190, not yet rewritten, EBX -> out_reachable
extern uint8_t path_find_reconstruct_path(path_find_context *context, uint8_t *out_reachable); // 0x43a4d0, not yet rewritten, EBX -> context
extern void path_find_context_init(path_find_context *context, const path_find_request *request,
                                   int32_t flags);                      // 0x43a700, EDX -> context
extern void path_find_set_goal(path_find_context *context, const real_point3d *destination,
                         uint32_t surface_index, uint32_t radius);      // 0x43a730, not yet rewritten, EAX -> context, ECX -> destination
extern uint8_t path_find_run(path_find_context *context);               // 0x43a8b0, not yet rewritten

// blam-cc: stack -> actor_index, record_distance, context
uint8_t actor_movement_action_resolve(datum_index actor_index, uint8_t record_distance, path_find_context *context)
{
    actor *self;
    Actor *actor_definition;
    ScenarioEncounter *encounter_definition;
    ScenarioSquad *squad_definition;
    ScenarioMovePosition *move_position;
    ScenarioFiringPosition *firing_position;
    prop *target;
    int16_t type;
    int16_t index;
    uint8_t result;
    uint8_t have_previous;
    real_point3d previous_destination;
    float distance;
    float avoidance_distance;
    path_find_request request;
    path_find_context local_context;

    self = (actor *)((uint8_t *)actor_data->data + (actor_index & 0xffff) * sizeof(actor));
    type = self->active_movement.type;
    result = 1;
    have_previous = 0;
    previous_destination.x = 0.0f;
    previous_destination.y = 0.0f;
    previous_destination.z = 0.0f;
    if (type != 0 && type != 1) {
        previous_destination = self->unknown_488;
        have_previous = 1;
    }

    if (self->order_committed != 0 || type == 0 || type == 1 ||
        (type == 3 && self->unknown_3bb != 0)) {
        self->movement_action_complete = 0;
        self->movement_timer = 0;
        self->movement_completed = 1;
        return 1;
    }

    self->movement_action_complete = 0;
    self->movement_completed = 0;
    self->movement_timer = 0;
    self->unknown_506 = 0;

    switch (type) {
    case 2:
        self->unknown_488 = self->active_movement.destination;
        self->unknown_494 = (uint32_t)self->active_movement.parameter;
        self->unknown_498 = 0;
        break;

    case 3:
        if (self->encounter_index == (datum_index)k_datum_index_none) {
            result = 0;
            actor_movement_action_complete(actor_index);
            return result;
        }
        encounter_definition = &((ScenarioEncounter *)global_scenario->encounters.pointer)
                                   [self->encounter_index & 0xffff];
        firing_position = &((ScenarioFiringPosition *)encounter_definition->firing_positions.pointer)
                              [*(int16_t *)&self->active_movement.destination];
        self->unknown_488 = *(real_point3d *)&firing_position->position;
        self->unknown_494 = firing_position->surface_index;
        self->unknown_498 = 0;
        break;

    case 4:
        result = 0;
        if (self->encounter_index == (datum_index)k_datum_index_none) {
            actor_movement_action_complete(actor_index);
            return result;
        }
        encounter_definition = &((ScenarioEncounter *)global_scenario->encounters.pointer)
                                   [self->encounter_index & 0xffff];
        squad_definition = &((ScenarioSquad *)encounter_definition->squads.pointer)
                               [self->squad_index];
        index = *(int16_t *)&self->active_movement.destination;
        if (index < 0 || (int32_t)index >= (int32_t)squad_definition->move_positions.count) {
            actor_movement_action_complete(actor_index);
            return result;
        }
        move_position = &((ScenarioMovePosition *)squad_definition->move_positions.pointer)[index];
        self->unknown_488 = *(real_point3d *)&move_position->position;
        self->unknown_494 = move_position->surface_index;
        result = 1;
        self->unknown_498 = 0;
        break;

    case 5:
        target = &((prop *)prop_data->data)[*(uint32_t *)&self->active_movement.destination & 0xffff];
        if (target->kind < 4 || target->kind > 5) {
            // 0x41a670 leaves EAX holding active_movement.destination, the prop handle.
            actor_target_get_relationship_object(*(datum_index *)&self->active_movement.destination);
        }
        if (self->flying != 0) {
            self->unknown_488 = *(real_point3d *)((uint8_t *)target + 0xc8);
        } else {
            self->unknown_488 = *(real_point3d *)((uint8_t *)target + 0xf0);
        }
        self->unknown_494 = *(uint32_t *)((uint8_t *)target + 0xec);
        self->unknown_498 = *(uint32_t *)&self->active_movement.destination.y;
        break;

    default:
        result = 0;
        actor_movement_action_complete(actor_index);
        return result;
    }

    // LAB_0041a558: decide whether the destination is worth steering to at all.
    if (self->flying == 0) {
        if (*(float *)&self->unknown_498 == 0.0f &&
            self->unknown_494 == (uint32_t)-1) {
            result = 0;
            actor_movement_action_complete(actor_index);
            return result;
        }
    } else {
        if (actor_movement_flying_needs_steering(actor_index, &self->unknown_488,
                                                 &avoidance_distance) == 0) {
            result = 0;
            actor_movement_action_complete(actor_index);
            return result;
        }
    }

    if (actor_movement_check_arrival(actor_index) != 0) {
        if (have_previous == 0) {
            return result;
        }
        if (vector3d_distance_squared(&self->unknown_488, &previous_destination) <= 0.010000001f) {
            return result;
        }
    }

    actor_definition = (Actor *)tag_instances[self->actor_definition_tag & 0xffff].data;
    distance = vector3d_distance(&self->unknown_488, &self->body_position);

    if (self->flying != 0) {
        // EBX = &self->movement_action_complete, the out-parameter this variant writes.
        result = path_find_validate_and_record_goal((uint32_t)global_structure_bsp, &self->body_position, 0, &self->unknown_488);
    } else if (context != (path_find_context *)0) {
        path_find_set_goal(context, &self->unknown_488, self->unknown_494, self->unknown_498);
        result = path_find_reconstruct_path(context, &self->movement_action_complete);
    } else {
        actor_build_path_find_request(actor_index, &request);
        if (self->active_movement.extra != (uint32_t)-1) {
            request.unknown_0c = (datum_index)self->active_movement.extra;
        }
        if (self->danger_type > 0 && self->unknown_28a == 0 &&
            (((uint8_t *)actor_definition)[4] & 0x10) == 0) {
            path_find_set_avoid_sphere(&request, &self->flee_from_point, self->danger_unknown_294,
                         self->danger_object_index, 10.0f);
        }
        path_find_context_init(&local_context, &request, 0);
        path_find_set_goal(&local_context, &self->unknown_488, self->unknown_494, self->unknown_498);
        result = 0;
        if (path_find_run(&local_context) != 0 &&
            path_find_reconstruct_path(&local_context, &self->movement_action_complete) != 0) {
            result = 1;
        }
    }

    self->unknown_4a4 = 1;
    if (record_distance != 0) {
        *(float *)&self->movement_timer = distance;
    }

    if (result != 0) {
        if (self->unknown_4bc <= 0.0f) {
            return result;
        }
        if (*(float *)&self->unknown_498 <= distance) {
            return result;
        }
        if (distance - self->unknown_4bc >= 0.5f) {
            return result;
        }
    }

    actor_movement_action_complete(actor_index);
    return result;
}

#if 0
Original Ghidra decompilation (0x41a460):

/* WARNING: Function: __chkstk replaced with injection: alloca_probe */

char actor_movement_action_resolve(uint param_1,char param_2,int param_3)

{
  int iVar1;
  float fVar2;
  short sVar3;
  bool bVar4;
  char cVar5;
  undefined4 *puVar6;
  int iVar7;
  float10 fVar8;
  char cStack_100ea;
  undefined1 auStack_100d4 [12];
  int iStack_100c8;

  iVar1 = (param_1 & 0xffff) * 0x724 + *(int *)(DAT_00880360 + 0x34);
  sVar3 = *(short *)(iVar1 + 0x46c);
  bVar4 = false;
  if ((sVar3 != 0) && (sVar3 != 1)) {
    bVar4 = true;
  }
  if ((((*(char *)(iVar1 + 0x160) != '\0') || (sVar3 == 0)) || (sVar3 == 1)) ||
     ((sVar3 == 3 && (*(char *)(iVar1 + 0x3bb) != '\0')))) {
    *(undefined1 *)(iVar1 + 0x4a8) = 0;
    *(undefined4 *)(iVar1 + 0x4a0) = 0;
    *(undefined1 *)(iVar1 + 0x484) = 1;
    return '\x01';
  }
  *(undefined1 *)(iVar1 + 0x4a8) = 0;
  *(undefined1 *)(iVar1 + 0x484) = 0;
  *(undefined4 *)(iVar1 + 0x4a0) = 0;
  *(undefined1 *)(iVar1 + 0x506) = 0;
  switch(sVar3) {
  case 2:
    *(undefined4 *)(iVar1 + 0x488) = *(undefined4 *)(iVar1 + 0x470);
    *(undefined4 *)(iVar1 + 0x48c) = *(undefined4 *)(iVar1 + 0x474);
    *(undefined4 *)(iVar1 + 0x490) = *(undefined4 *)(iVar1 + 0x478);
    *(undefined4 *)(iVar1 + 0x494) = *(undefined4 *)(iVar1 + 0x47c);
    goto LAB_0041a552;
  case 3:
    if (*(uint *)(iVar1 + 0x34) != 0xffffffff) {
      puVar6 = (undefined4 *)
               (*(int *)((*(uint *)(iVar1 + 0x34) & 0xffff) * 0xb0 + 0x9c +
                        *(int *)(global_scenario + 0x430)) + *(short *)(iVar1 + 0x470) * 0x18);
      *(undefined4 *)(iVar1 + 0x488) = *puVar6;
      *(undefined4 *)(iVar1 + 0x48c) = puVar6[1];
      *(undefined4 *)(iVar1 + 0x490) = puVar6[2];
      *(undefined4 *)(iVar1 + 0x494) = puVar6[5];
      goto LAB_0041a552;
    }
  default:
switchD_0041a521_default:
    cStack_100ea = '\0';
    break;
  case 4:
    cStack_100ea = '\0';
    if (*(uint *)(iVar1 + 0x34) == 0xffffffff) break;
    sVar3 = *(short *)(iVar1 + 0x470);
    iVar7 = *(short *)(iVar1 + 0x3a) * 0xe8 +
            *(int *)((*(uint *)(iVar1 + 0x34) & 0xffff) * 0xb0 + 0x84 +
                    *(int *)(global_scenario + 0x430));
    if ((sVar3 < 0) || (*(int *)(iVar7 + 0xc4) <= (int)sVar3)) break;
    puVar6 = (undefined4 *)(sVar3 * 0x50 + *(int *)(iVar7 + 200));
    *(undefined4 *)(iVar1 + 0x488) = *puVar6;
    *(undefined4 *)(iVar1 + 0x48c) = puVar6[1];
    *(undefined4 *)(iVar1 + 0x490) = puVar6[2];
    *(undefined4 *)(iVar1 + 0x494) = puVar6[0x13];
LAB_0041a552:
    *(undefined4 *)(iVar1 + 0x498) = 0;
LAB_0041a558:
    if (*(char *)(iVar1 + 0x99) == '\0') {
      if (*(float *)(iVar1 + 0x498) == 0.0) {
        cVar5 = *(int *)(iVar1 + 0x494) != -1;
        goto LAB_0041a721;
      }
    }
    else {
      cVar5 = FUN_0041aab0();
LAB_0041a721:
      if (cVar5 == '\0') goto switchD_0041a521_default;
    }
    cVar5 = FUN_00416700();
    if (cVar5 != '\0') {
      if (!bVar4) {
        return '\x01';
      }
      fVar8 = (float10)FUN_00401020();
      if (fVar8 <= (float10)0.010000001) {
        return '\x01';
      }
    }
    iVar7 = *(int *)((*(uint *)(iVar1 + 0x58) & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
    fVar8 = (float10)vector3d_distance();
    fVar2 = (float)fVar8;
    if (*(char *)(iVar1 + 0x99) == '\0') {
      if (param_3 == 0) {
        FUN_0041a9c0();
        if (*(int *)(iVar1 + 0x480) != -1) {
          iStack_100c8 = *(int *)(iVar1 + 0x480);
        }
        if (((0 < *(short *)(iVar1 + 0x280)) && (*(char *)(iVar1 + 0x28a) == '\0')) &&
           ((*(byte *)(iVar7 + 4) & 0x10) == 0)) {
          FUN_0043a070(*(undefined4 *)(iVar1 + 0x294),*(undefined4 *)(iVar1 + 0x28c),0x41200000);
        }
        path_find_context_init(auStack_100d4,0);
        FUN_0043a730(*(undefined4 *)(iVar1 + 0x494),*(undefined4 *)(iVar1 + 0x498));
        cStack_100ea = '\0';
        cVar5 = path_find_run();
        if ((cVar5 != '\0') && (cVar5 = FUN_0043a4d0(iVar1 + 0x4a8), cVar5 != '\0')) {
          cStack_100ea = '\x01';
        }
      }
      else {
        FUN_0043a730(*(undefined4 *)(iVar1 + 0x494),*(undefined4 *)(iVar1 + 0x498));
        cStack_100ea = FUN_0043a4d0(iVar1 + 0x4a8);
      }
    }
    else {
      cStack_100ea = FUN_0043a190(DAT_00746f9c,iVar1 + 300,0,iVar1 + 0x488);
    }
    *(undefined1 *)(iVar1 + 0x4a4) = 1;
    if (param_2 != '\0') {
      *(float *)(iVar1 + 0x4a0) = fVar2;
    }
    if (cStack_100ea != '\0') {
      if (*(float *)(iVar1 + 0x4bc) <= 0.0) {
        return cStack_100ea;
      }
      if (*(float *)(iVar1 + 0x498) <= fVar2) {
        return cStack_100ea;
      }
      if (0.5 <= fVar2 - *(float *)(iVar1 + 0x4bc)) {
        return cStack_100ea;
      }
    }
    break;
  case 5:
    iVar7 = (*(uint *)(iVar1 + 0x470) & 0xffff) * 0x138;
    sVar3 = *(short *)(iVar7 + 0x24 + *(int *)(DAT_008802c0 + 0x34));
    iVar7 = iVar7 + *(int *)(DAT_008802c0 + 0x34);
    if ((sVar3 < 4) || (5 < sVar3)) {
      actor_target_get_relationship_object();
    }
    if (*(char *)(iVar1 + 0x99) == '\0') {
      *(undefined4 *)(iVar1 + 0x488) = *(undefined4 *)(iVar7 + 0xf0);
      *(undefined4 *)(iVar1 + 0x48c) = *(undefined4 *)(iVar7 + 0xf4);
      *(undefined4 *)(iVar1 + 0x490) = *(undefined4 *)(iVar7 + 0xf8);
    }
    else {
      *(undefined4 *)(iVar1 + 0x488) = *(undefined4 *)(iVar7 + 200);
      *(undefined4 *)(iVar1 + 0x48c) = *(undefined4 *)(iVar7 + 0xcc);
      *(undefined4 *)(iVar1 + 0x490) = *(undefined4 *)(iVar7 + 0xd0);
    }
    *(undefined4 *)(iVar1 + 0x494) = *(undefined4 *)(iVar7 + 0xec);
    *(undefined4 *)(iVar1 + 0x498) = *(undefined4 *)(iVar1 + 0x474);
    goto LAB_0041a558;
  }
  actor_movement_action_complete();
  return cStack_100ea;
}
#endif
