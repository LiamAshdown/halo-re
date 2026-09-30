// ai_alert_actors_in_grenade_radius  (Ghidra: ai_alert_actors_in_grenade_radius, already named)
// address 0x42a0e0, size 699 bytes
// name confidence: 0.9   rewrite confidence: 0.9
// evidence: types/ai.h actor.unknown_138[0x20] (the raw offset 0x148 used here falls inside
//   it, at index 0x10 -- a cached BSP cluster index, matching the same array
//   actor_clear_target_state @0x4286c0 also reaches into). Calls actor_get_firing_positions
//   (0x41c1e0), actor_squad_react_to_grenade (0x42a3a0), object_get_position (0x4f6900) and
//   object_get_root_object_index (0x4f6fb0), all already established/rewritten, plus
//   actor_target_hearing_check and actor_find_or_create_shared_prop, neither established elsewhere in this repo, and
//   actor_iterator_next (dropped iterator-state argument recovered in
//   src/ai/ai_mark_recognized_objects_for_reaction.c).
//   VERIFIED against disassembly 0x42a0e0..0x42a39a (2026-09-30): the PAS triangle index (16-bit truncated), the audible test
//   ((byte & 0x7f) * 2.015748 < 40.0, bit 7 skips), the iterator initialisation, both hearing-check register/stack argument
//   sets and the react call all agree. The BSP cluster-visibility bitset keeps the raw pointer arithmetic (no BSP header).
// reconciled: R27 object.unknown_00c (datum_index) -> int32_t network_update_tick (game tick stamp, -1 = never)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"
#include "cache.h"
#include "objects.h"
#include <string.h>

extern ai_globals *ai_globals_ptr;
extern data_array *object_data;    // 0x008603b0
extern data_array *prop_data;      // 0x008802c0
extern data_array *encounter_data; // 0x008802c8
extern uint8_t *global_structure_bsp; // 0x00746f9c (clusters.count +0x134, sound PAS +0x220)

extern actor *actor_iterator_next(actor_iterator_state *iterator); // 0x436a70, EAX
extern void object_get_position(real_point3d *out, uint32_t object_index); // 0x4f6900, EAX, ECX
extern uint32_t object_get_root_object_index(uint32_t object_index); // 0x4f6fb0, ECX
extern void actor_get_firing_positions(datum_index actor_index, uint32_t *out_block,
    real_point3d *query_point); // 0x41c1e0, EAX, ECX, EDX
extern uint16_t actor_target_hearing_check(void *record, int16_t stance, datum_index actor_index, void *target_ref,
    int16_t gate, real_point3d *listener_position); // 0x41c030, stack, stack, EAX, ECX, EBX, ESI
extern datum_index actor_find_or_create_shared_prop(datum_index object_index, datum_index actor_index,
    char create_if_missing, uint32_t flag); // 0x43eb30, EAX, stack
extern void actor_squad_react_to_grenade(datum_index actor_index, datum_index target_prop_index,
    int16_t grenade_type); // 0x42a3a0, ESI, stack, EAX

#define OBJECT_DATA(h) ((uint8_t *)((object_header *)object_data->data)[(h) & 0xffff].data)
#define PROP(h) ((uint8_t *)prop_data->data + ((h) & 0xffff) * 0x138)

// Rewritten from the disassembly 0x42a0e0..0x42a39a. Stack: (object, stimulus, gate) -- ai_refresh_unit_stimulus_and_alert
//   passes (unit, DI, BX). The clusters whose sound PAS byte from the object's (root's) cluster is audible and
//   under 40 units (byte * 2.015748) are marked; every actor in a marked cluster (+0x148) other than the object's
//   own (+0x1f8, else +0x1f4) that hears the object's location (0x41c030, stance 0) gets its prop for the object
//   (created, flag 1) and, if it also hears that prop (+0xfc record, +0x38 stance, +0xbc position), reacts to it
//   (0x42a3a0) with the stimulus. The draft took one argument, looked the prop up by the owner actor alone and
//   ran both hearing checks with 2 of 6 arguments.
// blam-cc: stack -> source_unit_index, stimulus, gate
void ai_alert_actors_in_grenade_radius(datum_index source_unit_index, int16_t stimulus, int16_t gate)
{
    uint8_t *source = OBJECT_DATA(source_unit_index);
    uint8_t *location = source + 0x98;                 // [esp+0x10]: leaf, then the cluster word at +0x4
    datum_index owner_actor = *(datum_index *)(source + 0x1f8); // [esp+0x14]
    uint32_t cluster_bits[16];                         // [esp+0x80]
    uint32_t firing_block[14];                         // [esp+0x48]
    real_point3d source_position;                      // [esp+0x3c]
    actor_iterator_state iterator;                     // [esp+0x20]
    int32_t cluster_count;
    int16_t source_cluster;
    actor *a;

    if (owner_actor == k_datum_index_none) {
        owner_actor = *(datum_index *)(source + 0x1f4);
    }
    if (((struct object *)source)->parent_object != k_datum_index_none) {
        location = OBJECT_DATA(object_get_root_object_index(source_unit_index)) + 0x98;
    }
    cluster_count = *(int32_t *)(global_structure_bsp + 0x134);
    memset(cluster_bits, 0, sizeof(cluster_bits));
    source_cluster = *(int16_t *)(location + 0x4);
    if (source_cluster != -1) {
        int16_t i;

        for (i = 0; (int32_t)i < cluster_count; i++) {
            uint8_t pas = 0;

            if (source_cluster != i) {
                int32_t lo = source_cluster < i ? source_cluster : i;
                int32_t hi = source_cluster < i ? i : source_cluster;
                int32_t row = (uint16_t)(*(uint16_t *)(global_structure_bsp + 0x134) - 1) * lo - ((lo + 1) * lo) / 2;

                pas = (*(uint8_t **)(global_structure_bsp + 0x220))[(int16_t)(row + hi - 1)];
            }
            if (!(pas & 0x80) && (float)(int32_t)(pas & 0x7f) * 2.015748f < 40.0f) {
                cluster_bits[i >> 5] |= 1u << (i & 0x1f);
            }
        }
    }
    object_get_position(&source_position, source_unit_index);
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
        int16_t actor_cluster = *(int16_t *)((uint8_t *)a + 0x148);
        datum_index prop_index;
        uint8_t *p;

        if (actor_index == owner_actor || actor_cluster == -1 ||
            !(cluster_bits[actor_cluster >> 5] & (1u << (actor_cluster & 0x1f)))) {
            continue;
        }
        actor_get_firing_positions(actor_index, firing_block, &source_position);
        if ((int16_t)actor_target_hearing_check(location, 0, actor_index, firing_block, gate, &source_position) < 2) {
            continue;
        }
        prop_index = actor_find_or_create_shared_prop(source_unit_index, actor_index, 1, 1);
        if (prop_index == k_datum_index_none) {
            continue;
        }
        p = PROP(prop_index);
        if ((int16_t)actor_target_hearing_check(p + 0xfc, (int16_t)*(uint16_t *)(p + 0x38), actor_index, firing_block,
                gate, (real_point3d *)(p + 0xbc)) < 2) {
            continue;
        }
        actor_squad_react_to_grenade(actor_index, prop_index, stimulus);
    }
}

#if 0
Original Ghidra decompilation (0x42a0e0):

void ai_alert_actors_in_grenade_radius(uint param_1)

{
  int iVar1;
  byte bVar2;
  short sVar3;
  uint uVar4;
  short sVar5;
  int iVar6;
  int iVar7;
  int iVar8;
  short sVar9;
  short sVar10;
  uint *puVar11;
  uint auStackY_1040 [989];
  int local_b0;
  int local_8c;
  uint local_40 [16];

  iVar8 = *(int *)(DAT_008603b0 + 0x34);
  iVar1 = *(int *)(iVar8 + 8 + (param_1 & 0xffff) * 0xc);
  iVar6 = *(int *)(iVar1 + 0x1f8);
  if (iVar6 == -1) {
    iVar6 = *(int *)(iVar1 + 500);
  }
  if (*(int *)(iVar1 + 0x11c) != -1) {
    uVar4 = object_get_root_object_index();
    iVar1 = *(int *)(iVar8 + 8 + (uVar4 & 0xffff) * 0xc);
  }
  local_b0 = iVar1 + 0x98;
  iVar8 = *(int *)(DAT_00746f9c + 0x134);
  puVar11 = local_40;
  for (uVar4 = iVar8 + 0x1f >> 5 & 0x3fffffff; uVar4 != 0; uVar4 = uVar4 - 1) {
    *puVar11 = 0;
    puVar11 = puVar11 + 1;
  }
  for (iVar7 = 0; iVar7 != 0; iVar7 = iVar7 + -1) {
    *(undefined1 *)puVar11 = 0;
    puVar11 = (uint *)((int)puVar11 + 1);
  }
  sVar3 = *(short *)(iVar1 + 0x9c);
  if ((sVar3 != -1) && (sVar10 = 0, 0 < iVar8)) {
    iVar8 = 0;
    do {
      if (sVar3 == sVar10) {
        bVar2 = 0;
      }
      else {
        sVar5 = sVar10;
        sVar9 = sVar3;
        if (sVar10 < sVar3) {
          sVar5 = sVar3;
          sVar9 = sVar10;
        }
        bVar2 = *(byte *)((int)(short)(((*(short *)(DAT_00746f9c + 0x134) + -1) * sVar9 -
                                       (short)(((sVar9 + 1) * (int)sVar9) / 2)) + -1 + sVar5) +
                         *(int *)(DAT_00746f9c + 0x220));
      }
      if ((-1 < (char)bVar2) && ((float)(bVar2 & 0x7f) * 2.015748 < 40.0)) {
        local_40[iVar8 >> 5] = local_40[iVar8 >> 5] | 1 << ((byte)iVar8 & 0x1f);
      }
      sVar10 = sVar10 + 1;
      iVar8 = (int)sVar10;
    } while (iVar8 < *(int *)(DAT_00746f9c + 0x134));
  }
  object_get_position();
  if (*(char *)(DAT_00880354 + 1) != '\0') {
    local_8c = -1;
  }
  iVar8 = actor_iterator_next();
  while (iVar8 != 0) {
    if (((local_8c != iVar6) && (*(short *)(iVar8 + 0x148) != -1)) &&
       ((local_40[(int)*(short *)(iVar8 + 0x148) >> 5] &
        1 << ((byte)*(short *)(iVar8 + 0x148) & 0x1f)) != 0)) {
      actor_get_firing_positions();
      sVar3 = FUN_0041c030(local_b0,0);
      if (((1 < sVar3) && (uVar4 = FUN_0043eb30(local_8c,1,1), uVar4 != 0xffffffff)) &&
         (iVar8 = (uVar4 & 0xffff) * 0x138 + *(int *)(DAT_008802c0 + 0x34),
         sVar3 = FUN_0041c030(iVar8 + 0xfc,*(undefined2 *)(iVar8 + 0x38)), 1 < sVar3)) {
        actor_squad_react_to_grenade(uVar4);
      }
    }
    iVar8 = actor_iterator_next();
  }
  return;
}
#endif
