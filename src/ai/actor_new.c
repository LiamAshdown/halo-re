// actor_new  (Ghidra: actor_new, already named)
// address 0x426760, size 860 bytes
// name confidence: 0.75   rewrite confidence: 0.55
// evidence: types/ai.h names this the actor constructor and the primary evidence source for
//   the majority of the actor struct's fields; nearly every field this function writes is
//   already cited by address (0x426760) in the header's per-field comments. Uses
//   types/tags.h ActorVariant.actor_definition (TagDependency, tag_id at +0x10) and Actor
//   (flags bit 26 "swarm", bit 21 "flying", Actor+0x14 type). Calls datum_new (0x4d0480),
//   random_real (0x4019f0), actor_clear_recognition_history (0x414140, already rewritten)
//   and actor_dispatch_type_vtable_0x10 (0x426670, already rewritten).
//   UNSURE: several fields this function writes are not individually cited by 0x426760 in
//   types/ai.h (only by other functions), but the store shapes match the header's declared
//   types exactly in every case checked; used directly rather than re-deriving.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"
#include "cache.h"
#include <string.h>
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern data_array *actor_data;      // 0x00880360
extern tag_instance *tag_instances; // 0x0087bc14
extern const real_vector3d *global_forward3d_pointer; // 0x00696718, read here as a plain
                                                       // 12-byte block for all three caches

extern real random_real(void); // 0x4019f0
extern datum_index datum_new(data_array *array); // 0x4d0480, blam-cc: EDX -> array
extern void actor_clear_recognition_history(datum_index actor_index, uint8_t keep_when_typed); // 0x414140
extern void actor_dispatch_type_vtable_0x10(datum_index actor_index); // 0x426670

// Allocates and default-initializes a new actor record for the unit type referenced by the
// given ActorVariant tag: resolves the Actor tag it points at, allocates a datum, then writes
// every field whose default the engine cares about (the great majority to -1/none, 0, or a
// small constant), copies actor.type and the swarm/flying flags out of the Actor tag, rolls
// glass-ignorance once against Actor.glass_ignorance_chance, seeds the three position caches
// from the shared zero vector, and finally runs the per-type vtable-0x10 init callback.
// Returns the new actor's datum index, or k_datum_index_none on failure.
// FIXED (register inputs, objdump; one stack argument remains, so no ordering question): the original never reads EAX; actor_variant_tag arrive(s) on the stack (1 stack argument(s)).
// blam-cc: stack -> actor_variant_tag
datum_index actor_new(datum_index actor_variant_tag)
{
    ActorVariant *variant;
    datum_index actor_definition_tag;
    Actor *actor_tag;
    datum_index actor_index;
    actor *self;
    uint32_t flags;

    if (actor_variant_tag == (datum_index)k_datum_index_none) {
        return (datum_index)k_datum_index_none;
    }

    variant = (ActorVariant *)(tag_instances[actor_variant_tag & 0xffff].data);
    actor_definition_tag = *(datum_index *)&variant->actor_definition.tag_id;
    if (actor_definition_tag == (datum_index)k_datum_index_none) {
        return (datum_index)k_datum_index_none;
    }

    actor_tag = (Actor *)(tag_instances[actor_definition_tag & 0xffff].data);

    actor_index = datum_new(actor_data);
    if (actor_index == (datum_index)k_datum_index_none) {
        return (datum_index)k_datum_index_none;
    }

    self = &((actor *)actor_data->data)[actor_index & 0xffff];
    flags = *(uint32_t *)actor_tag; // Actor.flags

    self->actor_variant_tag = actor_variant_tag;
    self->swarm = (uint8_t)(flags >> 0x1a) & 1;               // ActorFlags bit 26 "swarm"
    self->actor_definition_tag = actor_definition_tag;
    self->type = actor_tag->type;
    self->unit_index = (datum_index)k_datum_index_none;
    self->counts_toward_encounter = 0;
    self->encounter_index = (datum_index)k_datum_index_none;
    self->squad_index = -1;
    self->platoon_index = -1;
    self->encounterless = 0;
    self->original_encounter_index = (datum_index)k_datum_index_none;
    self->original_squad_index = -1;
    self->cluster_count = 0;
    self->total_cluster_count = 0;
    self->cluster_unit_index = (datum_index)k_datum_index_none;
    self->swarm_index = (datum_index)k_datum_index_none;
    self->unit_control_pending = 1;
    self->active = 0;
    self->deactivation_time = (datum_index)k_datum_index_none;
    self->keep_unit_alive = 1;
    self->can_go_dormant = 1;
    self->idle_counter = 0;
    self->first_prop = (datum_index)k_datum_index_none;
    self->nearest_orphan_prop_index = (datum_index)k_datum_index_none;
    self->firing_position_index = -1;
    self->pending_order_request = -1;
    self->standing_order_request = -1;
    self->last_order_request_time = -1;
    self->unknown_8e = 0;
    self->pending_command_list = -1;
    self->command_list_finished_time = -1;
    self->mode = 0;
    self->awareness_level = 2;
    self->combat_status = 0;
    self->minimum_combat_status = 0;
    self->suspicion_status = 0;
    self->ticks_since_threatened = -1;
    self->search_firing_positions = 0;
    self->flying = (uint8_t)(flags >> 0x15) & 1;              // ActorFlags bit 21 "flying"
    self->pathfinding_surface_index = -1;
    self->active_unit_index = (datum_index)k_datum_index_none;
    self->platoon_defending = 0;
    self->unknown_1cc = 0;
    self->nearby_friend_prop_index = (datum_index)k_datum_index_none;
    self->try_to_fight_type = 0;
    self->conversation_index = (datum_index)k_datum_index_none;

    memset(&self->unknown_350, 0, 0x1a * sizeof(uint32_t)); // 0x350..0x3b7

    self->last_cover_attempt_time = (datum_index)k_datum_index_none;
    *(uint32_t *)&self->search_wait_time = 0xffffffff;
    *(uint32_t *)&self->last_melee_time = 0xffffffff;
    self->last_evasion_time = (datum_index)k_datum_index_none;
    self->last_vehicle_search_time = 0xffffffff;
    *(uint32_t *)&self->last_vehicle_charge_time = 0xffffffff;
    self->last_flee_abort_time = (datum_index)k_datum_index_none;
    self->found_body_time = (datum_index)k_datum_index_none;
    self->retreat_end_time = (datum_index)k_datum_index_none;
    self->retreat_prop_index = (datum_index)k_datum_index_none;
    self->retreat_start_time = (datum_index)k_datum_index_none;
    self->stood_down_body_vitality = 1.0f;
    self->exited_vehicle_index = (datum_index)k_datum_index_none;
    self->exited_vehicle_reentry_time = (datum_index)k_datum_index_none;
    self->panic_cooldown_time = (datum_index)k_datum_index_none;

    if (actor_tag->glass_ignorance_chance > 0.0f) {
        self->ignores_glass = random_real() < actor_tag->glass_ignorance_chance;
    }

    self->vocalization_unknown_3e8 = 0;
    self->secondary_action = 0;
    self->movement_completed = 0;
    self->destination_surface_index = 0xffffffff;
    self->destination_radius = 0xffffffff;

    memset(&self->movement_action_complete, 0, 0x17 * sizeof(uint32_t)); // 0x4a8..0x503

    self->moving = 0;
    self->forced_aim = 0;
    self->firing_state = 1;
    self->firing_state_timer = 0;
    self->firing_delay_timer = 0;
    self->refire_timer = 0;
    self->line_of_fire_blocked_ticks = 0;
    self->firing_target_ticks = 0;
    self->firing_target_prop_index = (datum_index)k_datum_index_none;
    self->last_grenade_check_time = 0xffffffff;
    self->grenade_target_prop_index = 0xffffffff;
    self->avoidance_ray_clear_ticks = (datum_index)k_datum_index_none;
    self->unknown_5cc = (datum_index)k_datum_index_none;
    self->unknown_5d0 = (datum_index)k_datum_index_none;
    self->unknown_5d4 = (datum_index)k_datum_index_none;
    self->avoidance_last_direction = -1;
    self->avoidance_turn_around_ticks = -1;
    self->vocalization_line = 0;
    self->vocalization_state = 0;
    self->desired_aiming_vector = *(const real_point3d *)global_forward3d_pointer;
    self->desired_facing_vector = *(const real_point3d *)global_forward3d_pointer;
    self->desired_looking_vector = *(const real_point3d *)global_forward3d_pointer;
    self->grenade_eligible = 0;
    self->grenade_recheck_ticks = 30;
    self->target_combat_status = 0;
    self->target_unit_index = (datum_index)k_datum_index_none;
    self->target_last_seen_time = (datum_index)k_datum_index_none;
    self->ticks_since_engaged = 0xffffffff;

    actor_clear_recognition_history(actor_index, 0);

    self->pursuit_target_prop_index = (datum_index)k_datum_index_none;
    actor_dispatch_type_vtable_0x10(actor_index);

    return actor_index;
}

#if 0
Original Ghidra decompilation (0x426760):

uint actor_new(uint param_1)

{
  uint uVar1;
  uint *puVar2;
  uint uVar3;
  undefined *puVar4;
  uint uVar5;
  int iVar6;
  int iVar7;
  undefined4 *puVar8;
  float fVar9;

  if ((param_1 != 0xffffffff) &&
     (uVar1 = *(uint *)(*(int *)((param_1 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14) + 0x10),
     uVar1 != 0xffffffff)) {
    puVar2 = *(uint **)((uVar1 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
    uVar5 = datum_new();
    if (uVar5 != 0xffffffff) {
      iVar7 = (uVar5 & 0xffff) * 0x724 + *(int *)(DAT_00880360 + 0x34);
      uVar3 = *puVar2;
      *(uint *)(iVar7 + 0x5c) = param_1;
      *(byte *)(iVar7 + 6) = (byte)(uVar3 >> 0x1a) & 1;
      *(uint *)(iVar7 + 0x58) = uVar1;
      *(short *)(iVar7 + 4) = (short)puVar2[5];
      *(undefined4 *)(iVar7 + 0x18) = 0xffffffff;
      *(undefined1 *)(iVar7 + 0x1c) = 0;
      *(undefined4 *)(iVar7 + 0x34) = 0xffffffff;
      *(undefined2 *)(iVar7 + 0x3a) = 0xffff;
      *(undefined2 *)(iVar7 + 0x3c) = 0xffff;
      *(undefined1 *)(iVar7 + 9) = 0;
      *(undefined4 *)(iVar7 + 0x30) = 0xffffffff;
      *(undefined2 *)(iVar7 + 0x38) = 0xffff;
      *(undefined2 *)(iVar7 + 0x1e) = 0;
      *(undefined2 *)(iVar7 + 0x20) = 0;
      *(undefined4 *)(iVar7 + 0x24) = 0xffffffff;
      *(undefined4 *)(iVar7 + 0x28) = 0xffffffff;
      *(undefined1 *)(iVar7 + 7) = 1;
      *(undefined1 *)(iVar7 + 8) = 0;
      *(undefined4 *)(iVar7 + 0xc) = 0xffffffff;
      *(undefined1 *)(iVar7 + 0x13) = 1;
      *(undefined1 *)(iVar7 + 0x12) = 1;
      *(undefined2 *)(iVar7 + 0x4a) = 0;
      *(undefined4 *)(iVar7 + 0x50) = 0xffffffff;
      *(undefined4 *)(iVar7 + 0x54) = 0xffffffff;
      *(undefined2 *)(iVar7 + 0x3b8) = 0xffff;
      *(undefined2 *)(iVar7 + 0x60) = 0xffff;
      *(undefined2 *)(iVar7 + 0x62) = 0xffff;
      *(undefined4 *)(iVar7 + 100) = 0xffffffff;
      *(undefined1 *)(iVar7 + 0x8e) = 0;
      *(undefined2 *)(iVar7 + 0x90) = 0xffff;
      *(undefined4 *)(iVar7 + 0x94) = 0xffffffff;
      *(undefined2 *)(iVar7 + 0x6c) = 0;
      *(undefined2 *)(iVar7 + 0x6a) = 2;
      *(undefined2 *)(iVar7 + 0x6e) = 0;
      *(undefined2 *)(iVar7 + 0x72) = 0;
      *(undefined2 *)(iVar7 + 0x74) = 0;
      *(undefined4 *)(iVar7 + 0x88) = 0xffffffff;
      *(undefined1 *)(iVar7 + 0x98) = 0;
      *(byte *)(iVar7 + 0x99) = (byte)(*puVar2 >> 0x15) & 1;
      *(undefined4 *)(iVar7 + 0x164) = 0xffffffff;
      *(undefined4 *)(iVar7 + 0x158) = 0xffffffff;
      *(undefined1 *)(iVar7 + 0x1c9) = 0;
      *(undefined1 *)(iVar7 + 0x1cc) = 0;
      *(undefined4 *)(iVar7 + 0x1d0) = 0xffffffff;
      *(undefined2 *)(iVar7 + 0x1d4) = 0;
      *(undefined4 *)(iVar7 + 0x1dc) = 0xffffffff;
      puVar8 = (undefined4 *)(iVar7 + 0x350);
      for (iVar6 = 0x1a; iVar6 != 0; iVar6 = iVar6 + -1) {
        *puVar8 = 0;
        puVar8 = puVar8 + 1;
      }
      *(undefined4 *)(iVar7 + 0x370) = 0xffffffff;
      *(undefined4 *)(iVar7 + 0x37c) = 0xffffffff;
      *(undefined4 *)(iVar7 + 0x380) = 0xffffffff;
      *(undefined4 *)(iVar7 + 0x36c) = 0xffffffff;
      *(undefined4 *)(iVar7 + 900) = 0xffffffff;
      *(undefined4 *)(iVar7 + 0x388) = 0xffffffff;
      *(undefined4 *)(iVar7 + 0x398) = 0xffffffff;
      *(undefined4 *)(iVar7 + 0x3a0) = 0xffffffff;
      *(undefined4 *)(iVar7 + 0x3a4) = 0xffffffff;
      *(undefined4 *)(iVar7 + 0x3ac) = 0xffffffff;
      *(undefined4 *)(iVar7 + 0x3b0) = 0xffffffff;
      *(undefined4 *)(iVar7 + 0x3b4) = 0x3f800000;
      *(undefined4 *)(iVar7 + 0x390) = 0xffffffff;
      *(undefined4 *)(iVar7 + 0x394) = 0xffffffff;
      *(undefined4 *)(iVar7 + 0x39c) = 0xffffffff;
      if (0.0 < (float)puVar2[0x24]) {
        fVar9 = random_real();
        *(bool *)(iVar7 + 0x376) = fVar9 < (float)puVar2[0x24];
      }
      *(undefined2 *)(iVar7 + 1000) = 0;
      *(undefined2 *)(iVar7 + 0x400) = 0;
      *(undefined2 *)(iVar7 + 0x46c) = 0;
      *(undefined4 *)(iVar7 + 0x480) = 0xffffffff;
      *(undefined4 *)(iVar7 + 0x494) = 0xffffffff;
      puVar8 = (undefined4 *)(iVar7 + 0x4a8);
      for (iVar6 = 0x17; iVar6 != 0; iVar6 = iVar6 + -1) {
        *puVar8 = 0;
        puVar8 = puVar8 + 1;
      }
      *(undefined1 *)(iVar7 + 0x504) = 0;
      *(undefined1 *)(iVar7 + 0x505) = 0;
      *(undefined2 *)(iVar7 + 0x5f2) = 1;
      *(undefined2 *)(iVar7 + 0x5f4) = 0;
      *(undefined2 *)(iVar7 + 0x5f6) = 0;
      *(undefined2 *)(iVar7 + 0x5f8) = 0;
      *(undefined2 *)(iVar7 + 0x5fa) = 0;
      *(undefined4 *)(iVar7 + 0x61c) = 0;
      *(undefined4 *)(iVar7 + 0x610) = 0xffffffff;
      *(undefined4 *)(iVar7 + 0x6a4) = 0xffffffff;
      *(undefined4 *)(iVar7 + 0x6b4) = 0xffffffff;
      *(undefined4 *)(iVar7 + 0x5c8) = 0xffffffff;
      *(undefined4 *)(iVar7 + 0x5cc) = 0xffffffff;
      *(undefined4 *)(iVar7 + 0x5d0) = 0xffffffff;
      *(undefined4 *)(iVar7 + 0x5d4) = 0xffffffff;
      puVar4 = PTR_DAT_00696718;
      *(undefined2 *)(iVar7 + 0x5d8) = 0xffff;
      *(undefined2 *)(iVar7 + 0x5f0) = 0xffff;
      *(undefined2 *)(iVar7 + 0x544) = 0;
      *(undefined2 *)(iVar7 + 0x548) = 0;
      *(undefined4 *)(iVar7 + 0x5b0) = *(undefined4 *)puVar4;
      *(undefined4 *)(iVar7 + 0x5b4) = *(undefined4 *)(puVar4 + 4);
      *(undefined4 *)(iVar7 + 0x5b8) = *(undefined4 *)(puVar4 + 8);
      puVar4 = PTR_DAT_00696718;
      *(undefined4 *)(iVar7 + 0x5a4) = *(undefined4 *)PTR_DAT_00696718;
      *(undefined4 *)(iVar7 + 0x5a8) = *(undefined4 *)(puVar4 + 4);
      *(undefined4 *)(iVar7 + 0x5ac) = *(undefined4 *)(puVar4 + 8);
      puVar4 = PTR_DAT_00696718;
      *(undefined4 *)(iVar7 + 0x5bc) = *(undefined4 *)PTR_DAT_00696718;
      *(undefined4 *)(iVar7 + 0x5c0) = *(undefined4 *)(puVar4 + 4);
      *(undefined4 *)(iVar7 + 0x5c4) = *(undefined4 *)(puVar4 + 8);
      *(undefined1 *)(iVar7 + 0x6cc) = 0;
      *(undefined2 *)(iVar7 + 0x6ce) = 0x1e;
      *(undefined2 *)(iVar7 + 0x268) = 0;
      *(undefined4 *)(iVar7 + 0x270) = 0xffffffff;
      *(undefined4 *)(iVar7 + 0x26c) = 0xffffffff;
      *(undefined4 *)(iVar7 + 0x278) = 0xffffffff;
      FUN_00414140(0);
      *(undefined4 *)(iVar7 + 0x3c0) = 0xffffffff;
      actor_dispatch_type_vtable_0x10();
      return uVar5;
    }
  }
  return 0xffffffff;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
