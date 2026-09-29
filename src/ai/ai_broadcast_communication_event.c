// ai_broadcast_communication_event  (Ghidra: ai_broadcast_communication_event, renamed)
// address 0x429fc0, size 275 bytes
// name confidence: 0.35   rewrite confidence: 0.9
// evidence: phase-4 summary "Broadcasts an AI communication/order event (selected by
// event_type) to every eligible active actor, gating delivery per-actor through a relevance
// test." Calls actor_get_firing_positions (0x41c1e0, already established:
// actor_index/out_block/query_point) and actor_target_hearing_check (a firing-position-block query, not
// independently established), plus actor_queue_point_reaction_dialogue (0x422780),
// actor_react_to_registered_danger (0x422930) and actor_react_to_flee_point (0x422c00), all
// already rewritten in this module, and actor_iterator_next, whose dropped iterator-state
// argument is recovered in src/ai/ai_mark_recognized_objects_for_reaction.c (reused here).
//   UNSURE: this is one of the least-confident rewrites in this pass, given how many of its
//   register-carried arguments (the query point, the firing-position block) Ghidra drops
//   entirely; the local scratch block's exact field layout is guessed from actor_target_hearing_check's
//   own argument (&local_5c) alone.
// register convention: stack -> point, event_type.
//   // blam-cc: stack -> point, event_type

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"
#include "objects.h"
#include "physics.h"
#include <stdint.h>
#include "units.h"

extern ai_globals *ai_globals_ptr;
extern data_array *encounter_data; // 0x008802c8
extern ModelCollisionGeometryBSP *global_collision_bsp; // 0x00746f90
extern uint8_t *global_structure_bsp; // 0x00746f9c (+0xe4 leaves, 0x10 each, +0x8 cluster word)

extern actor *actor_iterator_next(actor_iterator_state *iterator); // 0x436a70, EAX
extern uint32_t bsp3d_node_find_leaf(int32_t node_index, ModelCollisionGeometryBSP *bsp, real_point3d *point); // 0x5013a0, EAX, ECX, EDX
extern void actor_get_firing_positions(datum_index actor_index, uint32_t *out_block, real_point3d *query_point); // 0x41c1e0, EAX, ECX, EDX
extern uint16_t actor_target_hearing_check(void *record, int16_t stance, datum_index actor_index, void *target_ref,
    int16_t gate, real_point3d *listener_position); // 0x41c030, stack, EAX, ECX, EBX, ESI
extern void actor_queue_point_reaction_dialogue(const real_point3d *point, datum_index actor_index); // 0x422780, EAX, ECX
extern void actor_react_to_registered_danger(const real_point3d *point, datum_index actor_index, int32_t danger_object_index); // 0x422930, EAX, stack
extern void actor_react_to_flee_point(datum_index actor_index, int32_t flee_source_object, const real_point3d *point); // 0x422c00, stack

// REWRITTEN from objdump 0x429fc0..0x42a0d2. EAX: the sound's gate (loudness); ECX: its point; stack: (source object,
//   event type, an unused word). Finds the point's leaf and cluster, then every actor under 7 (+0x6e) that hears it
//   (0x41c030 from its firing-position block, stance 0) reacts: type 0 queues the point dialogue, 1 the registered
//   danger reaction (with the source), 2 the flee reaction. The draft had neither the gate, the point nor the source.
// blam-cc: EAX -> gate, ECX -> point, stack -> source_object, event_type, unused
void ai_broadcast_communication_event(int16_t gate, real_point3d *point, int32_t source_object, int16_t event_type,
    int16_t unused)
{
    bsp_leaf_reference location;        // [esp+0xc]
    actor_iterator_state iterator;      // [esp+0x14]
    actor *a;
    int32_t leaf;

    (void)unused;
    leaf = (int32_t)bsp3d_node_find_leaf(0, global_collision_bsp, point);
    location.leaf_index = leaf;
    location.cluster_index = leaf == -1 ? -1 :
        *(int16_t *)(*(uint8_t **)(global_structure_bsp + 0xe4) + (leaf & 0x7fffffff) * 0x10 + 0x8);
    if (ai_globals_ptr->actors_valid) {
        iterator.filter_array = encounter_data;
        iterator.next_index = 0;
        iterator.cursor = -1;
        iterator.signature = (uint32_t)encounter_data ^ 0x69746572;
        iterator.encounterless_done = 0;
        iterator.active = 1;
        iterator.actor_index = k_datum_index_none;
        iterator.next_actor_index = -1;
    }
    for (a = actor_iterator_next(&iterator); a != 0; a = actor_iterator_next(&iterator)) {
        datum_index actor_index = iterator.actor_index;
        uint32_t block[14];             // [esp+0x30]

        if (((struct actor *)a)->combat_status >= 7) {
            continue;
        }
        actor_get_firing_positions(actor_index, block, point);
        if ((int16_t)actor_target_hearing_check(&location, 0, actor_index, block, gate, point) < 2) {
            continue;
        }
        if (event_type == 0) {
            actor_queue_point_reaction_dialogue(point, actor_index);
        } else if (event_type == 1) {
            actor_react_to_registered_danger(point, actor_index, source_object);
        } else if (event_type == 2) {
            actor_react_to_flee_point(actor_index, source_object, point);
        }
    }
}

#if 0
Original Ghidra decompilation (0x429fc0):

void FUN_00429fc0(undefined4 param_1,short param_2)

{
  undefined4 uVar1;
  short sVar2;
  int iVar3;
  int local_5c;
  undefined2 local_58;
  uint local_54;
  undefined2 local_50;
  undefined4 local_4c;
  uint local_48;
  undefined1 local_44;
  undefined1 local_43;
  undefined4 local_40;
  undefined4 local_3c;

  local_5c = FUN_005013a0();
  if (local_5c == -1) {
    local_58 = 0xffff;
  }
  else {
    local_58 = *(undefined2 *)(local_5c * 0x10 + 8 + *(int *)(DAT_00746f9c + 0xe4));
  }
  if (*(char *)(DAT_00880354 + 1) != '\0') {
    local_54 = DAT_008802c8;
    local_48 = DAT_008802c8 ^ 0x69746572;
    local_50 = 0;
    local_4c = 0xffffffff;
    local_44 = 0;
    local_3c = 0xffffffff;
    local_40 = 0xffffffff;
    local_43 = 1;
  }
  iVar3 = actor_iterator_next();
  uVar1 = local_40;
  while (iVar3 != 0) {
    local_40 = uVar1;
    if (*(short *)(iVar3 + 0x6e) < 7) {
      actor_get_firing_positions();
      sVar2 = FUN_0041c030(&local_5c,0);
      if (1 < sVar2) {
        if (param_2 == 0) {
          FUN_00422780();
        }
        else if (param_2 == 1) {
          FUN_00422930(uVar1,param_1);
        }
        else if (param_2 == 2) {
          FUN_00422c00(uVar1,param_1);
        }
      }
    }
    iVar3 = actor_iterator_next();
    uVar1 = local_40;
  }
  return;
}
#endif
