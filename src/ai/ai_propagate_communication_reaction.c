// ai_propagate_communication_reaction  (Ghidra: ai_propagate_communication_reaction; named for this rewrite)
// address 0x42e9c0, size 711 bytes
// name confidence: 0.3   rewrite confidence: 0.25
// evidence: phase-4 summary ("propagates an AI communication event's effect (marking/
// acknowledging it) to nearby actors of the appropriate team within a fixed radius").
// Several callees (actor_iterator_next, object_get_root_object_index,
// object_get_node_local_transform) are called with fewer visible arguments than their
// established signatures elsewhere in this repo take; modeled here with the same
// signatures, but the exact values threaded through actor_target_hearing_check / actor_dispatch_squad_order /
// ai_dispatch_queued_order / actor_get_firing_positions are not independently verified.
// register convention: plain __cdecl, two stack arguments.
// blam-cc: stack -> object_index, order
//
// UNSURE, substantially: this is one of the module's more heavily register-aliased
// functions (Ghidra loses the world position local_c/local_8/local_4's source entirely).
// Modeled as the node-local transform's translation component, which is the only value
// object_get_node_local_transform produces that fits, but not independently confirmed. The
// actor-index local (this function's "local_74") is uninitialized when ai_globals_ptr is not
// actors_valid, matching the same pattern already reproduced faithfully elsewhere in this
// batch. Treat this file as a starting point for a follow-up disassembly pass, not a
// finished rewrite.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "ai.h"
#include <stdint.h>

extern data_array *object_data;    // 0x008603b0
extern data_array *encounter_data; // 0x008802c8
extern data_array *prop_data;      // 0x008802c0
extern ai_globals *ai_globals_ptr; // 0x00880354
extern int32_t use_absolute_team_check; // 0x006f1d20, nonzero (not multiplayer) means "different team" is enough
extern uint8_t team_relationship_flags; // 0x006b0b84, base of the 0x2d-dword team-relationship block
extern char ai_marker_name_a[]; // 0x0066bfa0, the AI marker name string

extern void ai_mark_recognized_objects_for_reaction(int16_t team_a, int16_t team_b, uint8_t status); // 0x42ba80
extern int32_t object_get_node_local_transform(datum_index object_index, char *marker_name, object_marker *marker, uint32_t flags); // 0x4f6080
extern datum_index object_get_root_object_index(void); // 0x4f6fb0, UNSURE: no traced args here either
extern void actor_get_firing_positions(void); // 0x41c1e0, not yet rewritten; UNSURE args
extern int16_t actor_target_hearing_check(uint32_t *block, uint32_t param); // 0x41c030, not yet rewritten; UNSURE args
extern void actor_dispatch_squad_order(void); // 0x42a540, not yet rewritten; UNSURE args
extern void ai_dispatch_queued_order(datum_index actor_index); // 0x42f840, this batch
extern datum_index actor_find_or_create_shared_prop(datum_index actor_index, uint32_t flag_a, uint32_t flag_b); // 0x43eb30, UNSURE signature
extern actor *actor_iterator_next(actor_iterator_state *iterator); // 0x436a70

// blam-cc: stack -> object_index, order
// If order requests it, first relays a team-vs-team reaction mark. Then, for either an
// order-type-1 or a counted order, resolves object_index's world marker position (via its
// root object if it has a parent) and scans every OTHER actor within 30 world units whose
// team is hostile (or, in multiplayer, simply different) to object_index's own team,
// resolving each one to a prop and dispatching a squad-order handler when its firing
// position rating exceeds 1.
void ai_propagate_communication_reaction(datum_index object_index, ai_communication_order *order)
{
    object *obj;
    int16_t object_team;
    object_marker marker;
    datum_index root_index;
    void *firing_position_list_base;
    datum_index actor_index;
    actor_iterator_state iterator;
    actor *a;
    int16_t other_team;
    uint8_t hostile;
    prop *p;
    datum_index resolved_actor;
    int16_t rating;

    if (order->order_type == 1) {
        ai_mark_recognized_objects_for_reaction(order->team_a, order->team_b, order->status);
    }

    if (order->order_type == 0 && order->count <= 0) {
        return;
    }

    obj = ((object_header *)object_data->data)[object_index & 0xffff].data;
    object_team = *(int16_t *)((uint8_t *)obj + 0xb8);
    object_get_node_local_transform(object_index, ai_marker_name_a, &marker, 1);

    if (obj->parent_object != (datum_index)k_datum_index_none) {
        root_index = object_get_root_object_index();
        obj = ((object_header *)object_data->data)[root_index & 0xffff].data;
    }
    firing_position_list_base = (void *)((uint8_t *)obj + 0x98);

    if (ai_globals_ptr->actors_valid) {
        iterator.filter_array = encounter_data;
        iterator.unknown_04 = 0;
        iterator.cursor = -1;
        iterator.signature = (uint32_t)(uintptr_t)encounter_data ^ 0x69746572;
        iterator.unknown_10 = 0;
        iterator.active = 1;
        iterator.actor_index = -1;
        iterator.unknown_18 = -1;
        actor_index = (datum_index)k_datum_index_none;
    }

    a = actor_iterator_next(&iterator);
    while (a != 0) {
        if (a->unit_index != object_index) {
            other_team = a->team;
            if (use_absolute_team_check == 0) {
                if (other_team < 0 || 9 < other_team || object_team < 0 || 9 < object_team) {
                    goto next_actor;
                }
                {
                    int32_t pair = (int32_t)object_team + other_team * 10;
                    uint32_t bit = *(uint32_t *)(&team_relationship_flags + 0xa4 + (pair >> 5) * 4);
                    hostile = 1 - ((bit & (1u << (pair & 0x1f))) != 0);
                }
            } else {
                hostile = (other_team != object_team);
            }

            if (hostile == 0) {
                float dx = marker.transform.position.x - a->aim_origin.x;
                float dy = marker.transform.position.y - a->aim_origin.y;
                float dz = marker.transform.position.z - a->aim_origin.z;
                if (dx * dx + dy * dy + dz * dz <= 900.0f) {
                    resolved_actor = actor_find_or_create_shared_prop(actor_index, 1, 1);
                    if (resolved_actor != (datum_index)k_datum_index_none) {
                        p = &((prop *)prop_data->data)[resolved_actor & 0xffff];
                        actor_get_firing_positions();
                        rating = actor_target_hearing_check(firing_position_list_base, p->unknown_38);
                        if (1 < rating) {
                            actor_dispatch_squad_order();
                            ai_dispatch_queued_order(actor_index);
                        }
                    }
                }
            }
        }
next_actor:
        a = actor_iterator_next(&iterator);
    }
}

#if 0
Original Ghidra decompilation (0x42e9c0):

void FUN_0042e9c0(uint param_1,int param_2)

{
  int iVar1;
  short sVar2;
  float fVar3;
  float fVar4;
  float fVar5;
  short sVar6;
  uint uVar7;
  int iVar8;
  char cVar9;
  int local_98;
  undefined4 local_74;
  undefined1 local_6c [96];
  float local_c;
  float local_8;
  float local_4;

  if (*(short *)(param_2 + 0x14) == 1) {
    FUN_0042ba80(*(undefined2 *)(param_2 + 0x18),*(undefined2 *)(param_2 + 0x1a),
                 *(undefined1 *)(param_2 + 0x1c));
  }
  if ((*(short *)(param_2 + 0x14) != 0) || (0 < *(short *)(param_2 + 0xc))) {
    local_98 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (param_1 & 0xffff) * 0xc);
    sVar2 = *(short *)(local_98 + 0xb8);
    object_get_node_local_transform(param_1,&DAT_0066bfa0,local_6c,1);
    if (*(int *)(local_98 + 0x11c) != -1) {
      uVar7 = object_get_root_object_index();
      local_98 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (uVar7 & 0xffff) * 0xc);
    }
    local_98 = local_98 + 0x98;
    if (*(char *)(DAT_00880354 + 1) != '\0') {
      local_74 = 0xffffffff;
    }
    iVar8 = actor_iterator_next();
    while (iVar8 != 0) {
      if (*(uint *)(iVar8 + 0x18) != param_1) {
        sVar6 = *(short *)(iVar8 + 0x3e);
        if (DAT_006f1d20 == 0) {
          if ((((sVar6 < 0) || (9 < sVar6)) || (sVar2 < 0)) || (9 < sVar2)) goto LAB_0042ec6b;
          iVar1 = (int)sVar2 + sVar6 * 10;
          cVar9 = '\x01' - ((1 << ((byte)iVar1 & 0x1f) &
                            *(uint *)(DAT_006b0b84 + 0xa4 + (iVar1 >> 5) * 4)) != 0);
        }
        else {
          cVar9 = sVar6 != sVar2;
        }
        if (((cVar9 == '\0') &&
            (fVar3 = local_c - *(float *)(iVar8 + 0x120),
            fVar5 = local_8 - *(float *)(iVar8 + 0x124), fVar4 = local_4 - *(float *)(iVar8 + 0x128)
            , fVar3 * fVar3 + fVar5 * fVar5 + fVar4 * fVar4 <= 900.0)) &&
           (uVar7 = FUN_0043eb30(local_74,1,1), uVar7 != 0xffffffff)) {
          iVar8 = *(int *)(DAT_008802c0 + 0x34);
          actor_get_firing_positions();
          sVar6 = FUN_0041c030(local_98,*(undefined2 *)((uVar7 & 0xffff) * 0x138 + iVar8 + 0x38));
          if (1 < sVar6) {
            FUN_0042a540();
            FUN_0042f840(local_74);
          }
        }
      }
LAB_0042ec6b:
      iVar8 = actor_iterator_next();
    }
  }
  return;
}
#endif
