// ai_propagate_communication_reaction  (Ghidra: ai_propagate_communication_reaction; named for this rewrite)
// address 0x42e9c0, size 711 bytes
// name confidence: 0.3   rewrite confidence: 0.9
// REWRITTEN from objdump 0x42e9c0..0x42ec86: the draft never advanced its actor index (every prop lookup used
//   none), called the prop lookup without the object, and called the firing-position, hearing and dispatch helpers
//   without operands. Stack: (object, order). Friendly actors (team matrix +0xa4, or same team in multiplayer)
//   within 30 of the object's eye marker get a prop for the object, their firing positions toward the marker
//   (0x41c1e0), and if the hearing check (0x41c030, gate 3 for line classes >= 4, else 1) reaches 2 the squad
//   order (0x42a540) and the queued order (0x42f840) are dispatched.
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
// reconciled: R04 0x006f1d20 int32_t use_absolute_team_check -> game.h game_engine_definition *current_game_engine (all accesses are DWORD; non-NULL = multiplayer engine loaded)

// FIXED (0x42ea55: [esp+0xb4] = marker +0x60): the marker position is the WORLD position
//   node_transform.position, not the node-relative transform at +0x2c.
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "objects.h"
#include "ai.h"
#include <stdint.h>
#include <string.h>

extern data_array *object_data;    // 0x008603b0
extern data_array *encounter_data; // 0x008802c8
extern data_array *prop_data;      // 0x008802c0
extern ai_globals *ai_globals_ptr;
extern game_engine_definition *current_game_engine;
extern uint8_t *team_pair_data; // 0x006b0b84
extern uint8_t ai_communication_lines[]; // 0x00655aa0, 0x28-byte rows
extern char ai_marker_name_a[]; // 0x0066bfa0

extern void ai_mark_recognized_objects_for_reaction(int16_t team_a, int16_t team_b, uint8_t status); // 0x42ba80
extern int32_t object_get_node_local_transform(uint32_t object_index, char *marker_name, object_marker *marker,
    uint32_t param_4); // 0x4f6080
extern uint32_t object_get_root_object_index(uint32_t object_index); // 0x4f6fb0, ECX
extern actor *actor_iterator_next(actor_iterator_state *iterator); // 0x436a70, EAX
extern datum_index actor_find_or_create_shared_prop(datum_index object_index, datum_index actor_index,
    char create_if_missing, uint32_t flag); // 0x43eb30, EAX, stack
extern void actor_get_firing_positions(datum_index actor_index, uint32_t *out_block, real_point3d *query_point); // 0x41c1e0, EAX, ECX, EDX
extern uint16_t actor_target_hearing_check(void *record, int16_t stance, datum_index actor_index, void *target_ref,
    int16_t gate, real_point3d *listener_position); // 0x41c030, stack, EAX, ECX, EBX, ESI
extern void actor_dispatch_squad_order(datum_index prop_index, const actor_squad_order_header *order,
    datum_index actor_index); // 0x42a540, EAX, ECX, EDX
extern void ai_dispatch_queued_order(ai_queued_order *order, datum_index prop_index, datum_index actor_index); // 0x42f840, ECX, EDX, stack

#define OBJECT_DATA(h) ((uint8_t *)((object_header *)object_data->data)[(h) & 0xffff].data)

// blam-cc: stack -> object_index, order
void ai_propagate_communication_reaction(datum_index object_index, ai_communication_order *order)
{
    uint8_t *o = (uint8_t *)order;
    uint8_t *obj;
    int16_t object_team;
    object_marker marker;
    real_point3d position;   // [esp+0x1c]
    uint8_t *location;       // [esp+0x18]
    int16_t gate = 1;        // [esp+0x14]
    int16_t row = *(int16_t *)(o + 0x6);
    actor_iterator_state iterator;
    actor *a;

    if (*(int16_t *)(o + 0x14) == 1) {
        ai_mark_recognized_objects_for_reaction((int16_t)*(uint16_t *)(o + 0x18), (int16_t)*(uint16_t *)(o + 0x1a),
                                                o[0x1c]);
    }
    if (*(int16_t *)(o + 0x14) == 0 && *(int16_t *)(o + 0xc) <= 0) {
        return;
    }
    obj = OBJECT_DATA(object_index);
    object_team = *(int16_t *)(obj + 0xb8);
    location = obj + 0x98;
    object_get_node_local_transform(object_index, ai_marker_name_a, &marker, 1);
    position = marker.node_transform.position;
    if (row != -1 && *(int16_t *)(ai_communication_lines + row * 0x28 + 0x2) >= 4) {
        gate = 3;
    }
    if (*(datum_index *)(obj + 0x11c) != k_datum_index_none) {
        location = OBJECT_DATA(object_get_root_object_index(object_index)) + 0x98;
    }
    if (!ai_globals_ptr->actors_valid) {
        return; // the binary walks an uninitialised iterator here
    }
    memset(&iterator, 0, sizeof(iterator));
    iterator.filter_array = encounter_data;
    iterator.unknown_04 = 0;
    iterator.cursor = -1;
    iterator.signature = (uint32_t)(uintptr_t)encounter_data ^ 0x69746572;
    iterator.unknown_10 = 0;
    iterator.active = 1;
    iterator.actor_index = k_datum_index_none;
    iterator.unknown_18 = -1;

    for (a = actor_iterator_next(&iterator); a != 0; a = actor_iterator_next(&iterator)) {
        uint8_t *ap = (uint8_t *)a;
        int16_t team = *(int16_t *)(ap + 0x3e);
        datum_index actor_index;
        datum_index prop_index;

        if (*(datum_index *)(ap + 0x18) == object_index) {
            continue;
        }
        if (current_game_engine != 0) {
            if (team != object_team) {
                continue;
            }
        } else {
            int32_t bit;

            if (team < 0 || team >= 10 || object_team < 0 || object_team >= 10) {
                continue;
            }
            bit = team * 10 + object_team;
            if (!(*(uint32_t *)(team_pair_data + 0xa4 + (bit >> 5) * 4) & (1u << (bit & 0x1f)))) {
                continue;
            }
        }
        {
            float dx = position.x - *(float *)(ap + 0x120);
            float dy = position.y - *(float *)(ap + 0x124);
            float dz = position.z - *(float *)(ap + 0x128);

            if (!(dz * dz + dy * dy + dx * dx <= 900.0f)) {
                continue;
            }
        }
        actor_index = iterator.actor_index;
        prop_index = actor_find_or_create_shared_prop(object_index, actor_index, 1, 1);
        if (prop_index != k_datum_index_none) {
            prop *p = &((prop *)prop_data->data)[prop_index & 0xffff];
            uint32_t firing[0x18]; // [esp+0x44]

            actor_get_firing_positions(actor_index, firing, &position);
            if ((int16_t)actor_target_hearing_check(location, (int16_t)*(uint16_t *)((uint8_t *)p + 0x38), actor_index,
                                           firing, gate, &position) >= 2) {
                actor_dispatch_squad_order(prop_index, (const actor_squad_order_header *)order, actor_index);
                ai_dispatch_queued_order((ai_queued_order *)order, prop_index, actor_index);
            }
        }
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
