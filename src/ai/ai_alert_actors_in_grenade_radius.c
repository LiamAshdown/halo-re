// ai_alert_actors_in_grenade_radius  (Ghidra: ai_alert_actors_in_grenade_radius, already named)
// address 0x42a0e0, size 699 bytes
// name confidence: 0.9   rewrite confidence: 0.2
// evidence: types/ai.h actor.unknown_138[0x20] (the raw offset 0x148 used here falls inside
//   it, at index 0x10 -- a cached BSP cluster index, matching the same array
//   actor_clear_target_state @0x4286c0 also reaches into). Calls actor_get_firing_positions
//   (0x41c1e0), actor_squad_react_to_grenade (0x42a3a0), object_get_position (0x4f6900) and
//   object_get_root_object_index (0x4f6fb0), all already established/rewritten, plus
//   actor_target_hearing_check and actor_find_or_create_shared_prop, neither established elsewhere in this repo, and
//   actor_iterator_next (dropped iterator-state argument recovered in
//   src/ai/ai_mark_recognized_objects_for_reaction.c).
//   UNSURE: this is one of the least-confident rewrites in this pass. The BSP
//   cluster-visibility bitset this function builds (from a triangular PVS byte table at the
//   collision BSP's own data, DAT_00746f9c+0x134/+0x220) is preserved close to the original
//   pointer arithmetic rather than modeled with named types, since no BSP/collision header
//   exists in this repo. The large unused stack array the original declares
//   (auStackY_1040[989]) is dead and is not reproduced.
// reconciled: R27 object.unknown_00c (datum_index) -> int32_t network_update_tick (game tick stamp, -1 = never)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"
#include "cache.h"
#include "objects.h"
#include <string.h>

extern ai_globals *ai_globals_ptr; // 0x00880354
extern data_array *actor_data;     // 0x00880360
extern data_array *object_data;    // 0x008603b0
extern data_array *prop_data;      // 0x008802c0
extern data_array *encounter_data; // 0x008802c8

// 0x00746f9c holds a POINTER to the structure BSP tag data; Ghidra's `DAT_00746f9c + 0x134`
// is ScenarioStructureBSP.clusters.count and `+ 0x220` is .sound_pas_data.pointer (the
// potentially-audible-set table, which is what this sound-propagation test walks).
extern ScenarioStructureBSP *global_structure_bsp; // 0x00746f9c

extern actor *actor_iterator_next(actor_iterator_state *iterator); // 0x436a70
extern void object_get_position(real_point3d *out_position, datum_index object_index); // 0x4f6900
extern datum_index object_get_root_object_index(datum_index object_index); // 0x4f6fb0, UNSURE signature
extern void actor_get_firing_positions(datum_index actor_index, uint32_t *out_block, real_point3d *query_point); // 0x41c1e0
extern int16_t actor_target_hearing_check(uint32_t *block, uint32_t param); // 0x41c030, UNSURE signature
extern datum_index actor_find_or_create_shared_prop(datum_index actor_index, uint32_t flag_a, uint32_t flag_b); // 0x43eb30, UNSURE signature
extern void actor_squad_react_to_grenade(datum_index actor_index, datum_index target_prop_index, int16_t grenade_type); // 0x42a3a0, already rewritten in this module

// Given a grenade/threat's originating object, marks nearby pathfinding-visible clusters
// within a ~40-unit radius and triggers a squad reaction for every actor whose cached BSP
// cluster sits inside that set.
// FIXED (register inputs, objdump; one stack argument remains, so no ordering question): the original never reads EAX; source_unit_index arrive(s) on the stack (1 stack argument(s)).
// blam-cc: stack -> source_unit_index
void ai_alert_actors_in_grenade_radius(datum_index source_unit_index)
{
    object *source_object = ((object_header *)object_data->data)[source_unit_index & 0xffff].data;
    datum_index owner_actor = (datum_index)source_object->network_update_tick; // UNSURE: swarm_actor_index-style field, offset 0x1f8 in original
    real_point3d source_position;
    uint32_t cluster_bits[16];
    int16_t source_cluster;
    actor_iterator_state iterator;
    actor *a;

    owner_actor = *(datum_index *)((uint8_t *)source_object + 0x1f8);
    if (owner_actor == (datum_index)k_datum_index_none) {
        owner_actor = *(datum_index *)((uint8_t *)source_object + 500); // unit_data.actor_index
    }

    if (source_object->parent_object != (datum_index)k_datum_index_none) {
        datum_index root_index = object_get_root_object_index(source_object->parent_object);
        source_object = ((object_header *)object_data->data)[root_index & 0xffff].data;
    }

    memset(cluster_bits, 0, sizeof(cluster_bits));

    source_cluster = source_object->location_cluster_index;
    if (source_cluster != -1 && (int32_t)global_structure_bsp->clusters.count > 0) {
        int32_t i;
        for (i = 0; i < (int32_t)global_structure_bsp->clusters.count; i++) {
            uint8_t pvs_byte;
            if (source_cluster == (int16_t)i) {
                pvs_byte = 0;
            } else {
                int16_t hi = (int16_t)i, lo = source_cluster;
                if (source_cluster < (int16_t)i) {
                    hi = source_cluster;
                    lo = (int16_t)i;
                }
                // UNSURE: triangular PVS table index, preserved from the original arithmetic.
                pvs_byte = ((uint8_t *)(uintptr_t)global_structure_bsp->sound_pas_data.pointer)[
                    (int16_t)(((int32_t)global_structure_bsp->clusters.count - 1) * lo - (int16_t)(((lo + 1) * (int32_t)lo) / 2)) - 1 + hi];
            }
            if ((int8_t)pvs_byte >= 0 && (float)(pvs_byte & 0x7f) * 2.015748f < 40.0f) {
                cluster_bits[i >> 5] |= 1u << (i & 0x1f);
            }
        }
    }

    object_get_position(&source_position, source_unit_index);

    iterator.filter_array = encounter_data;
    iterator.unknown_04 = 0;
    iterator.cursor = -1;
    iterator.signature = (uint32_t)(uintptr_t)encounter_data ^ 0x69746572;
    iterator.unknown_10 = 0;
    iterator.active = 1;
    iterator.actor_index = -1;
    iterator.unknown_18 = -1;

    a = actor_iterator_next(&iterator);
    while (a != 0) {
        datum_index actor_index = iterator.actor_index /* the full handle, salt included */;
        int16_t actor_cluster = *(int16_t *)((uint8_t *)a + 0x148); // UNSURE offset (unknown_138 array)

        if (actor_index != owner_actor && actor_cluster != -1 &&
            (cluster_bits[actor_cluster >> 5] & (1u << (actor_cluster & 0x1f))) != 0) {
            uint32_t firing_block[64]; // UNSURE size/layout, see file header
            int16_t count;

            actor_get_firing_positions(actor_index, firing_block, &source_position);
            count = actor_target_hearing_check(firing_block, 0);
            if (count > 1) {
                datum_index target_prop = actor_find_or_create_shared_prop(owner_actor, 1, 1);
                if (target_prop != (datum_index)k_datum_index_none) {
                    prop *p = &((prop *)prop_data->data)[target_prop & 0xffff];
                    count = actor_target_hearing_check((uint32_t *)((uint8_t *)p + 0xfc), *(uint16_t *)((uint8_t *)p + 0x38));
                    if (count > 1) {
                        // UNSURE: the original calls this with only one visible argument
                        // (the resolved prop handle); actor_index and grenade_type are
                        // guessed here (the acting actor and grenade type 0) rather than
                        // independently confirmed with objdump.
                        actor_squad_react_to_grenade(actor_index, target_prop, 0);
                    }
                }
            }
        }
        a = actor_iterator_next(&iterator);
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
