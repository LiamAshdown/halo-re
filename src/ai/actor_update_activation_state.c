// actor_update_activation_state  (Ghidra: actor_update_activation_state, already named)
// address 0x429160, size 265 bytes
// name confidence: 0.5   rewrite confidence: 0.9 (checked against objdump 0x429160..0x429268)
// evidence: phase-4 summary "Per-tick actor combat-state update: validates via
// actor_update_squad_link_state then refreshes threat/perception/orientation sub-state and
// either continues an in-progress transition or performs a full combat activation." Calls
// many functions already rewritten either earlier in the module (actor_target_relationship_think,
// actor_update_crouch_state, actor_update_firing_state, actor_update_flee_response,
// actor_movement_advance_waypoint, actor_update_look_target,
// actor_update_grenade_eligibility_state, actor_choose_best_target, actor_invoke_type_handler,
// actor_schedule_grenade_throw, actor_dispatch_type_vtable_0x18) or in this rewrite
// (actor_update_squad_link_state, actor_refresh_combat_context, actor_run_mode_transition_loop,
// actor_snapshot_orientation, actor_apply_queued_look_to_unit -- the last not yet written when this file was
// authored). types/ai.h actor.secondary_action(0x418)/keep_unit_alive(0x13)/swarm(0x06);
// offsets 0x3e8..0x428 (33 dwords) are zeroed here, matching the vocalization/recognition
// scratch run types/ai.h already names individually. The mode-table call at 0x00655264 is
// actor_mode_definitions[mode]+0x10, i.e. exactly enter_proc as named in
// types/ai.h's actor_mode_definition.
// register convention: ESI -> actor_index (unaff_ESI).
//   // blam-cc: ESI -> actor_index

// VERIFIED against disassembly 0x429160..0x429268 (2026-09-30): the mode tick proc (row +0x10, 0x655264) is called with
//   the actor index pushed (push esi; call eax; add esp,4), every call/tail-call order and the 0x21-dword clear match.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"
#include <string.h>

extern data_array *actor_data; // 0x00880360
extern actor_mode_definition actor_mode_definitions[16]; // 0x00655254

extern uint8_t actor_update_squad_link_state(datum_index actor_index); // 0x429270
extern void actor_update_idle_stagger(datum_index actor_index); // 0x429430, in this rewrite range, not yet written when this file was authored
extern void actor_refresh_combat_context(datum_index actor_index); // 0x4297a0
extern void actor_target_relationship_think(datum_index actor_index); // 0x41abd0
extern void actor_choose_best_target(datum_index actor_index); // 0x4203a0
extern void actor_update_crouch_state(datum_index actor_index); // 0x4213b0
extern void actor_run_mode_transition_loop(datum_index actor_index); // 0x429ee0
extern void actor_dispatch_type_vtable_0x18(datum_index actor_index); // 0x4266a0
extern void actor_snapshot_orientation(datum_index actor_index); // 0x4294d0
extern void actor_invoke_type_handler(uint32_t actor_index); // 0x409e70
extern void actor_update_grenade_eligibility_state(datum_index actor_index); // 0x42f370
extern void actor_schedule_grenade_throw(uint32_t actor_index); // 0x402f80
extern void actor_movement_advance_waypoint(datum_index actor_index); // 0x4163e0
extern void actor_update_flee_response(datum_index actor_index); // 0x414250
extern void actor_movement_update(datum_index actor_index); // 0x416790
extern void actor_update_look_target(datum_index actor_index); // 0x415480
extern void actor_update_firing_state(datum_index actor_index); // 0x40e7b0
extern void actor_apply_queued_look_to_unit(datum_index actor_index); // 0x42a640, in this rewrite range, not yet written when this file was authored

// blam-cc: ESI -> actor_index
// Per-tick actor combat-state update: validates via actor_update_squad_link_state; if it
// says the link is still good, refreshes idle/combat context, target relationship, best
// target and crouch state, then clears the vocalization/recognition scratch run, and runs
// the mode-transition loop and its enter-proc callback. If keep_unit_alive is clear, either
// dispatches to the per-type vtable (for a swarm) or runs the full per-tick pipeline: snapshot
// orientation, per-type handler, grenade eligibility, scheduling, waypoint advance, flee
// response, movement, look target, firing state, and a final unestablished step.
void actor_update_activation_state(datum_index actor_index)
{
    actor *self = &((actor *)actor_data->data)[actor_index & 0xffff];

    if (actor_update_squad_link_state(actor_index) == 0) {
        return;
    }

    actor_update_idle_stagger(actor_index);
    actor_refresh_combat_context(actor_index);
    actor_target_relationship_think(actor_index);
    actor_choose_best_target(actor_index);
    actor_update_crouch_state(actor_index);

    memset(&self->vocalization_unknown_3e8, 0, 0x21 * sizeof(uint32_t)); // 0x3e8..0x46b
    self->secondary_action = -1;
    *(int16_t *)((uint8_t *)self + 0x42c) = -1;
    *(int16_t *)((uint8_t *)self + 0x42e) = -1;

    actor_run_mode_transition_loop(actor_index);

    // 0x4291e3: the new mode's +0x10 (tick) proc, stack actor index
    {
        uint32_t proc = actor_mode_definitions[self->mode].tick_proc;
        if (proc != 0) {
            ((void (*)(datum_index))proc)(actor_index);
        }
    }

    if (self->keep_unit_alive == 0) {
        if (self->swarm != 0) {
            actor_dispatch_type_vtable_0x18(actor_index);
            return;
        }
        actor_snapshot_orientation(actor_index);
        actor_invoke_type_handler(actor_index);
        actor_update_grenade_eligibility_state(actor_index);
        actor_schedule_grenade_throw(actor_index);
        actor_movement_advance_waypoint(actor_index);
        actor_update_flee_response(actor_index);
        actor_movement_update(actor_index);
        actor_update_look_target(actor_index);
        actor_update_firing_state(actor_index);
        actor_apply_queued_look_to_unit(actor_index);
    }
}

#if 0
Original Ghidra decompilation (0x429160):

void actor_update_activation_state(void)

{
  char cVar1;
  int iVar2;
  int iVar3;
  int iVar4;
  int iVar5;
  uint unaff_ESI;
  undefined4 *puVar6;

  iVar4 = (unaff_ESI & 0xffff) * 0x724;
  iVar5 = *(int *)(DAT_00880360 + 0x34) + iVar4;
  cVar1 = actor_update_squad_link_state();
  if (cVar1 != '\0') {
    FUN_00429430();
    actor_refresh_combat_context();
    FUN_0041abd0();
    actor_choose_best_target();
    FUN_004213b0();
    iVar3 = *(int *)(DAT_00880360 + 0x34) + iVar4;
    puVar6 = (undefined4 *)(iVar3 + 1000);
    for (iVar2 = 0x21; iVar2 != 0; iVar2 = iVar2 + -1) {
      *puVar6 = 0;
      puVar6 = puVar6 + 1;
    }
    *(undefined2 *)(iVar3 + 0x418) = 0xffff;
    *(undefined2 *)(iVar3 + 0x42c) = 0xffff;
    *(undefined2 *)(iVar3 + 0x42e) = 0xffff;
    actor_run_mode_transition_loop();
    if (*(code **)(&DAT_00655264 + *(short *)(*(int *)(DAT_00880360 + 0x34) + iVar4 + 0x6c) * 0x38)
        != (code *)0x0) {
      (**(code **)(&DAT_00655264 + *(short *)(*(int *)(DAT_00880360 + 0x34) + iVar4 + 0x6c) * 0x38))
                ();
    }
    if (*(char *)(iVar5 + 0x13) == '\0') {
      if (*(char *)(iVar5 + 6) != '\0') {
        actor_dispatch_type_vtable_0x18();
        return;
      }
      actor_snapshot_orientation();
      actor_invoke_type_handler();
      FUN_0042f370();
      actor_schedule_grenade_throw();
      FUN_004163e0();
      FUN_00414250();
      actor_movement_update();
      FUN_00415480();
      FUN_0040e7b0();
      FUN_0042a640();
      return;
    }
  }
  return;
}
#endif
