// projectile_new  (Ghidra: missed_4bd7c0, created by hand this pass -- Ghidra never recovered it
// as a function; only reachable through the projectile object_type_definition row)
// address 0x4bd7c0, size 652 bytes
// name confidence: 0.75   rewrite confidence: 0.55
// evidence: named already in out/phase4/projectiles_types_notes.md ("+0x28 projectile_new --
//   missed by Ghidra; the single best source in the module" -- most of types/projectiles.h's
//   projectile_data field descriptions cite this function). types/projectiles.h projectile_data
//   (flags 0x22c initialized to _projectile_tracer_bit, state 0x230, material_response_index
//   0x232, ignore_object_index 0x234 "projectile_new walks object.creating_object (0xc4) up the
//   parent chain and stores the root", tracked_object_index 0x238, contrail_attachment_index
//   0x23c "scans the Object tag attachments block ... for the first entry whose tag group is
//   'cont'", detonation_timer_rate 0x244, arming_timer_rate 0x24c "1/(Projectile.arming_time*30)
//   from projectile_new"); types/tags.h Projectile (projectile_flags 0x17c, timer[2] 0x1bc,
//   arming_time 0x1a4, initial_velocity 0x1e4, maximum_range 0x1c8), Object.attachments (0x140
//   TagReflexive), ObjectAttachment (type.tag_fourcc 0x00, size 0x48),
//   ScenarioStructureBSPFogRegion (fog 0x24), ScenarioStructureBSPFogPalette (fog.tag_id 0x2c,
//   size 0x88), Fog (flags bit 0 is_water); types/objects.h object (owner_linkage 0xc0,
//   unknown_0c4 0xc4 "the creating object", parent_object 0x11c, forward 0x074, velocity 0x068,
//   location_leaf_index/location_cluster_index 0x098/0x09c == bsp_leaf_reference,
//   bounding_center 0x0a0, object_flags _object_in_water_bit 0x10, _object_definition_flag0_bit
//   0x40000, _object_connected_to_map_bit 0x80000); global 0x00746f9c global_structure_bsp,
//   0x00719cd0 random_seed_global, 0x00719720 network_game_mode. Callees
//   projectile_compute_rotation / projectile_update_function_values /
//   projectile_compute_deceleration (all three this module, already written) and
//   scenario_location_fog_region (0x53ec30, src/scenario, EAX -> leaf, EBX -> point).
// register convention: object index is a plain stack cdecl parameter, matching the rest of this
//   directly-indexed (non object_try_and_get) family. The scenario_location_fog_region call's
//   register arguments are confirmed against objdump (`lea eax,[esi+0x98]` for leaf, `lea
//   ebx,[esi+0xa0]` for point).
// blam-cc: stack -> object_index
// UNSURE: object.flags bits 0x2000 (set unconditionally here) and, in the timer-rate branch, the
//   choice between the random-range and the fixed timer[0] path have no names in
//   types/objects.h's object_flags enum; the 0x2000 bit is written as a raw literal with a
//   comment, matching the projectiles notes' own "0x2000 and 0xc0000 are set by projectile_new".
// UNSURE: the detonation_timer_rate selection tests projectile_flags bit 0x04
//   (detonation_max_time_if_attached) and bit 0x40 (minimum_unattached_detonation_time) but, as
//   decompiled, both a set bit 0x04 AND a set bit 0x40 (with bit 0x04 clear) select the same
//   timer[0]*30 expression -- there is no code path that selects timer[1] alone. Reproduced
//   exactly rather than corrected; see the two nested branches below.
// UNSURE: the fog-region-to-Fog-tag walk (fog_region -> ScenarioStructureBSPFogRegion.fog ->
//   ScenarioStructureBSPFogPalette[fog].fog.tag_id -> Fog.flags bit 0) has no home in any header
//   yet; written here as raw offsets against the confirmed struct layouts rather than folded
//   into types/scenario.h.
// reconciled: R28 object.unknown_0c4 -> datum_index creator_object (same offset 0xc4)
// reconciled: R26 object +0x009 raw byte store -> network_state_009 (object.unknown_008 is now split into uint8 unknown_008 / network_state_009 / unknown_00a[2])

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "scenario.h"
#include "objects.h"
#include "projectiles.h"

extern data_array *object_data;            // 0x008603b0
extern tag_instance *tag_instances;        // 0x0087bc14
extern ScenarioStructureBSP *global_structure_bsp; // 0x00746f9c
extern uint32_t random_seed_global;        // 0x00719cd0
extern int16_t network_game_mode;          // 0x00719720, 0 local, 1 client, 2 host

extern int16_t scenario_location_fog_region(bsp_leaf_reference *leaf, real_point3d *point); // 0x53ec30
    // blam-cc: EAX -> leaf, EBX -> point
extern void projectile_compute_rotation(uint32_t object_index);            // 0x4c0180, blam-cc: EAX -> object_index
extern void projectile_update_function_values(uint32_t object_index);      // 0x4c0250, blam-cc: stack -> object_index
extern void projectile_compute_deceleration(uint32_t object_index);        // 0x4c0310, blam-cc: EAX -> object_index

// The projectile row's query_create hook (object_type_definition +0x28). Establishes a freshly
// created projectile's initial state: zeroes/resets flags, state, material_response_index,
// resolves ignore_object_index from the firing object's own root parent, finds its contrail
// attachment index, computes the detonation and arming timer rates from the tag, applies the
// tag's initial_velocity along the object's own forward vector, probes whether it starts inside
// water fog, runs the rotation/function-value/deceleration setup passes, and clears the network
// replication bytes when the game is networked. Always reports success.
uint8_t projectile_new(uint32_t object_index) // blam-cc: stack -> object_index
{
    object *obj = ((object_header *)object_data->data)[object_index & 0xffff].data;
    Projectile *tag = (Projectile *)tag_instances[(uint16_t)obj->definition_tag].data;
    projectile_data *proj = (projectile_data *)((uint8_t *)obj + k_projectile_data_offset);
    float rate;
    datum_index root;
    int16_t i;
    int16_t fog_region;
    uint8_t in_water;

    obj->flags |= 0x2000; // UNSURE: unnamed object_flags bit

    proj->flags = _projectile_tracer_bit;
    proj->thrown_grenade = 0;
    proj->tracked_object_index = (datum_index)k_datum_index_none;
    proj->state = 0;
    proj->material_response_index = -1;

    // ignore_object_index: walk object.creator_object (the creating object) up its own
    // parent_object chain to find the root -- i.e. the firing unit.
    root = (datum_index)k_datum_index_none;
    if (obj->creator_object != (uint32_t)k_datum_index_none) {
        datum_index cursor = (datum_index)obj->creator_object;
        do {
            root = cursor;
            cursor = ((object_header *)object_data->data)[cursor & 0xffff].data->parent_object;
        } while (cursor != (datum_index)k_datum_index_none);
    }
    proj->ignore_object_index = root;

    // detonation_timer_rate: 1/(t*30), t chosen from the tag's attach-related flags. See the
    // UNSURE note above -- both branches below that test projectile_flags bit 0x04 land on the
    // same timer[0]*30 expression as the bit-0x40 branch.
    if ((tag->projectile_flags & 0x04) == 0) {
        if ((tag->projectile_flags & 0x40) == 0) {
            random_seed_global = random_seed_global * 0x19660d + 0x3c6ef35f;
            rate = ((tag->timer[1] - tag->timer[0]) * (real)(random_seed_global >> 0x10) * 1.5259022e-05f +
                    tag->timer[0]) * 30.0f;
        } else {
            rate = tag->timer[0] * 30.0f;
        }
    } else {
        rate = tag->timer[0] * 30.0f;
    }
    if (!(rate < 1.0f)) { // fcom / test ah,1: an unordered rate still divides
        proj->detonation_timer_rate = 1.0f / rate;
    }

    rate = tag->arming_time * 30.0f;
    if (!(rate < 1.0f)) {
        proj->arming_timer_rate = 1.0f / rate;
    }

    // contrail_attachment_index: first attachment whose tag group is 'cont'.
    proj->contrail_attachment_index = -1;
    for (i = 0; i < (int32_t)tag->base.attachments.count; i++) {
        ObjectAttachment *attachment = &((ObjectAttachment *)tag->base.attachments.pointer)[i];
        if (attachment->type.tag_fourcc == 0x636f6e74) { // 'cont'
            proj->contrail_attachment_index = i;
            break;
        }
    }

    // launch along the object's own forward vector at the tag's initial_velocity
    obj->velocity.i = tag->initial_velocity * obj->forward.i + obj->velocity.i;
    obj->velocity.j = tag->initial_velocity * obj->forward.j + obj->velocity.j;
    obj->velocity.k = tag->initial_velocity * obj->forward.k + obj->velocity.k;

    // in-water probe: resolve the object's fog region, then its Fog tag's is_water bit
    in_water = 0;
    fog_region = scenario_location_fog_region((bsp_leaf_reference *)&obj->location_leaf_index,
                                               &obj->bounding_center);
    if (fog_region != -1) {
        ScenarioStructureBSPFogRegion *region =
            &((ScenarioStructureBSPFogRegion *)global_structure_bsp->fog_regions.pointer)[fog_region];
        if (region->fog != (uint16_t)-1) {
            ScenarioStructureBSPFogPalette *fog_entry =
                &((ScenarioStructureBSPFogPalette *)global_structure_bsp->fog_palette.pointer)[region->fog];
            datum_index fog_tag_id = *(datum_index *)&fog_entry->fog.tag_id;
            if (fog_tag_id != (datum_index)k_datum_index_none) {
                Fog *fog_tag = (Fog *)tag_instances[fog_tag_id & 0xffff].data;
                in_water = (*(uint8_t *)fog_tag & 1) != 0;
            }
        }
    }
    if (in_water) {
        obj->flags |= _object_in_water_bit;
    } else {
        obj->flags &= ~(uint32_t)_object_in_water_bit;
    }

    projectile_compute_rotation(object_index);
    projectile_update_function_values(object_index);
    projectile_compute_deceleration(object_index);

    obj->flags |= _object_definition_flag0_bit | _object_connected_to_map_bit;

    if (network_game_mode == 1 || network_game_mode == 2) {
        proj->network_state_valid = 0;
        proj->network_baseline_index = 0;
        proj->network_sequence = 0;
        obj->network_state_009 = 0; // 0x4bda48 (projectile_new)
    }

    return 1;
}

#if 0
Original Ghidra decompilation (0x4bd7c0):

undefined4 missed_4bd7c0(uint param_1)

{
  float fVar1;
  uint *puVar2;
  int iVar3;
  uint uVar4;
  short sVar5;
  int iVar6;
  uint uVar7;

  iVar6 = DAT_008603b0;
  puVar2 = *(uint **)(*(int *)(DAT_008603b0 + 0x34) + 8 + (param_1 & 0xffff) * 0xc);
  iVar3 = *(int *)((*puVar2 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
  puVar2[4] = puVar2[4] | 0x2000;
  uVar7 = 0xffffffff;
  puVar2[0x8b] = 2;
  *(undefined1 *)(puVar2 + 0x9e) = 0;
  puVar2[0x8e] = 0xffffffff;
  *(undefined2 *)(puVar2 + 0x8c) = 0;
  *(undefined2 *)((int)puVar2 + 0x232) = 0xffff;
  if (puVar2[0x31] != 0xffffffff) {
    uVar4 = puVar2[0x31];
    do {
      uVar7 = uVar4;
      uVar4 = *(uint *)(*(int *)(*(int *)(iVar6 + 0x34) + 8 + (uVar7 & 0xffff) * 0xc) + 0x11c);
    } while (uVar4 != 0xffffffff);
  }
  puVar2[0x8d] = uVar7;
  if ((*(uint *)(iVar3 + 0x17c) & 4) == 0) {
    if ((*(uint *)(iVar3 + 0x17c) & 0x40) == 0) {
      random_seed_global = random_seed_global * 0x19660d + 0x3c6ef35f;
      fVar1 = ((*(float *)(iVar3 + 0x1c0) - *(float *)(iVar3 + 0x1bc)) *
               (float)(random_seed_global >> 0x10) * 1.5259022e-05 + *(float *)(iVar3 + 0x1bc)) *
              30.0;
    }
    else {
      fVar1 = *(float *)(iVar3 + 0x1bc) * 30.0;
    }
  }
  else {
    fVar1 = *(float *)(iVar3 + 0x1bc) * 30.0;
  }
  if (1.0 <= fVar1) {
    puVar2[0x91] = (uint)(1.0 / fVar1);
  }
  fVar1 = *(float *)(iVar3 + 0x1a4) * 30.0;
  if (1.0 <= fVar1) {
    puVar2[0x93] = (uint)(1.0 / fVar1);
  }
  puVar2[0x8f] = 0xffffffff;
  sVar5 = 0;
  if (0 < *(int *)(iVar3 + 0x140)) {
    iVar6 = 0;
    do {
      if (*(int *)(*(int *)(iVar3 + 0x144) + iVar6 * 0x48) == 0x636f6e74) {
        puVar2[0x8f] = (int)sVar5;
        break;
      }
      sVar5 = sVar5 + 1;
      iVar6 = (int)sVar5;
    } while (iVar6 < *(int *)(iVar3 + 0x140));
  }
  fVar1 = *(float *)(iVar3 + 0x1e4);
  puVar2[0x1a] = (uint)(fVar1 * (float)puVar2[0x1d] + (float)puVar2[0x1a]);
  puVar2[0x1b] = (uint)(fVar1 * (float)puVar2[0x1e] + (float)puVar2[0x1b]);
  puVar2[0x1c] = (uint)(fVar1 * (float)puVar2[0x1f] + (float)puVar2[0x1c]);
  sVar5 = scenario_location_fog_region();
  if ((((sVar5 == -1) ||
       (sVar5 = *(short *)(*(int *)(DAT_00746f9c + 0x188) + sVar5 * 0x28 + 0x24), sVar5 == -1)) ||
      (uVar7 = *(uint *)(sVar5 * 0x88 + *(int *)(DAT_00746f9c + 0x194) + 0x2c), uVar7 == 0xffffffff)
      ) || ((**(byte **)((uVar7 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14) & 1) == 0)) {
    uVar7 = puVar2[4] & 0xffffffef;
  }
  else {
    uVar7 = puVar2[4] | 0x10;
  }
  puVar2[4] = uVar7;
  projectile_compute_rotation();
  projectile_update_function_values(param_1);
  projectile_compute_deceleration();
  puVar2[4] = puVar2[4] | 0xc0000;
  uVar7 = (uint)DAT_00719720;
  if ((uVar7 == 1) || (uVar7 == 2)) {
    uVar7 = uVar7 & 0xffffff00;
    *(undefined1 *)((int)puVar2 + 0x279) = 0;
    *(undefined1 *)((int)puVar2 + 0x27a) = 0;
    *(undefined1 *)((int)puVar2 + 0x27b) = 0;
    *(undefined1 *)((int)puVar2 + 9) = 0;
  }
  return CONCAT31((int3)(uVar7 >> 8),1);
}
#endif
