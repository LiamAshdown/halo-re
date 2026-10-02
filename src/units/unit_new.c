// unit_new  (Ghidra: no function created; the phase-4 types agent carved a stub
//   "missed_562180" from the object_type_definition vtable evidence)
// VERIFIED against disassembly 0x562180..0x56255f (2026-09-30): every store offset/size/value, the grenade seed, the feign-death
//   roll, the default-team test, the seat-label call and the vehicle-entry queue append were compared with the code and the
//   unit_data / Unit / UnitSeat / ai_globals field offsets were checked with static asserts.
// address 0x562180, size 987 bytes
// name confidence 0.5, rewrite confidence 0.9
// evidence: out/phase4/units_types_notes.md: "The unit row's other columns are 0x561fe0 (+0x14
//   initialize), 0x562020 (+0x1c reset), 0x562180 (+0x28) ..." This is that +0x28 column: a
//   one-time per-object spawn initializer, called for every freshly placed/created unit
//   (biped or vehicle). Cross-checked field by field against types/units.h's unit_data (every
//   offset below resolves to a named field) and types/tags.h's Unit tag (grenade_type +0x2c4,
//   grenade_count +0x2c6, feign_death_threshold/time/chance +0x22c/0x230/0x244, default_team
//   +0x180, seats +0x2e4/+0x2e8). The CEA prototype's leaked `unit_new` (vendor/halocea/src/
//   blam/units/unit_new.c, names only, not code -- see README's "hint-only" policy) matches this
//   function step for step: same field reset order, the same forward-vector seeding into all
//   five aim/look/facing vectors, the same feign-death random roll gated on the same three tag
//   scalars, the same "default team when not networked and no team yet" branch, and the same
//   "queue mounted-weapon creation when any seat declares one" scan -- which is strong
//   corroboration this is genuinely `unit_new`, including for two TYPES-GAP items below that
//   only the CEA read makes sense of.
// register convention: plain stack argument (mov eax,[esp+8] at 0x562181), returns AL;
//   blam-cc: stack -> object_index. Returns 1 normally, 0 when
//   the unit's animation graph tag reference (Unit tag Object.animation_graph, tag+0x44) is -1.
// NOTE: object.owner_team (types/objects.h +0xb8) is written here from Unit.default_team
//   (tag+0x180), which only makes sense if this field is actually the object's team index, not
//   its scenario name index -- matching the pre-existing TYPES-GAP flagged in PLAN.md ("objects.h
//   offset 0xb8 team index vs name_index conflict") and the CEA source's `owner_team_index` name
//   for the same slot. Written through the header's current `name_index` field here rather than
//   renaming the header from this pass; flagged again in the summary as a types-correction item.
// NOTE: object.flags (+0x10) gains raw bits 0x6000 with no object_flags enumerators defined
//   for them; the CEA source calls the equivalent pair "dynamic/static lighting recompute". Kept
//   as a raw hex OR since types/objects.h's `object_flags` enum doesn't yet name them.
// NOTE: unit_data.flags (+0x204) bit 0x100 (_unit_flag_permutation_dirty in types/units.h) and
//   bit 0x2000 (_unit_flag_unknown_2000) are set here in ways that better match "needs dialogue
//   setup" and "feign death allowed" respectively (per the CEA names); not renamed in the header
//   from this pass, flagged in the summary.
// NOTE: the two un-analyzed callees FUN_005618e0 (CEA: unit_dialogue_determine_variant) and
//   FUN_0056cf10 (CEA: unit_add_initial_weapons) are outside this pass's address range; objdump
//   shows them receiving object_index in EAX (0x562393) and ESI (0x5624e7) respectively.
// Cleanup-pass review (objdump 0x562180..0x56255f, field offsets checked with gcc -m32):
//   added the missing unknown_475 = 0 store (0x5621c6); the feign-death roll sets the bit only
//   when random < chance (fcomp / test ah,5 / jp at 0x562479, equality clears it; the draft used
//   <=); the default-team branch tests current_game_engine (0x006f1d20, dword), not
//   network_game_mode (0x00719720), which is only the client test before unit_add_initial_weapons.
// NOTE: the retail binary defers "create this unit's mounted mount weapons" by pushing
//   object_index onto ai_globals.vehicle_entry_queue (drained later by whatever processes it)
//   rather than calling a `ai_create_mounted_weapons_for_unit`-style helper directly, unlike the
//   CEA prototype read above -- kept exactly as the retail disassembly shows.
// reconciled: R29 object/object_placement_data.name_index -> owner_team (int16 team at 0xb8 / 0x14)
// reconciled: R04 0x006f1d20 void * current_game_engine -> game.h game_engine_definition *current_game_engine (all accesses are DWORD; non-NULL = multiplayer engine loaded)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "cache.h"
#include "ai.h"
#include "objects.h"
#include "units.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern data_array *object_data;     // 0x008603b0
extern tag_instance *tag_instances; // 0x0087bc14
extern int16_t network_game_mode; // 0x00719720, foreign; 1 = client (matches camera/ai modules' established name for this address)
extern game_engine_definition *current_game_engine;   // 0x006f1d20, game.h; non-NULL = multiplayer engine loaded (R04)
extern ai_globals *ai_globals_ptr;  // 0x00880354, foreign (ai)
extern char *s_stand; // 0x0069fdec "stand" (matches unit_update_stance_and_jump.c)

extern float random_real(void); // 0x4019f0
extern void unit_dialogue_determine_variant(uint32_t object_index); // 0x5618e0, blam-cc: EAX -> object_index
extern uint8_t unit_set_or_test_seat_and_weapon_label(uint32_t unit_index, const char *seat_label,
                                                       const char *weapon_label, uint8_t test_only); // 0x5651e0,
    // blam-cc: EAX -> unit_index, stack -> (seat_label, weapon_label, test_only)
extern void unit_add_initial_weapons(uint32_t object_index); // 0x56cf10, blam-cc: ESI -> object_index

// object_type_definition "unit" row, +0x28 column: one-time per-object spawn initializer for
// bipeds and vehicles alike. Resets the seat/weapon/grenade/animation/vehicle-link/AI-dialogue
// fields to their empty defaults, seeds the aim/look/facing vectors from the object's current
// forward direction, rolls the feign-death eligibility for this spawn, applies the tag's default
// team and initial grenade count, labels the default "stand" seat, gives non-client units their
// starting weapons, and queues mounted-weapon creation for any seat that declares one.
uint8_t unit_new(uint32_t object_index)
{
    object *obj = ((object_header *)object_data->data)[object_index & 0xffff].data;
    Unit *tag = (Unit *)tag_instances[obj->definition_tag & 0xffff].data;
    unit_data *unit = (unit_data *)((uint8_t *)obj + k_unit_data_offset);
    uint32_t *field;
    int32_t i;

    if (*(int32_t *)&tag->base.animation_graph.tag_id == -1) {
        return 0;
    }

    unit->unknown_475 = 0;
    unit->control_update_id = -1;
    unit->equipment_object_index = (datum_index)-1;
    unit->weapons[0] = (datum_index)-1;
    unit->weapons[1] = (datum_index)-1;
    unit->weapons[2] = (datum_index)-1;
    unit->weapons[3] = (datum_index)-1;
    unit->current_weapon_index = -1;
    unit->desired_weapon_index = -1;
    unit->current_grenade_index = -1;
    unit->desired_grenade_index = -1;
    unit->zoom_level = -1;
    unit->desired_zoom_level = -1;
    unit->controlling_player = (datum_index)-1;
    unit->actor_index = (datum_index)-1;
    unit->swarm_actor_index = (datum_index)-1;
    unit->swarm_next_unit_index = (datum_index)-1;
    unit->swarm_previous_unit_index = (uint32_t)-1;
    unit->vehicle_seat_index = -1;
    unit->driver_unit_index = (datum_index)-1;
    unit->gunner_unit_index = (datum_index)-1;
    unit->animation_state_flags = 0;
    unit->animation_definition_index = -1;
    unit->animation_weapon_index = -1;
    unit->animation_weapon_type_index = -1;
    unit->animation_state = -1;
    unit->replacement_animation_state = 0;
    unit->overlay_animation_state = 0;
    unit->aiming_animation_index = -1;
    unit->looking_animation_index = -1;
    unit->overlays[0].animation_index = -1;
    unit->overlays[1].animation_index = -1;
    unit->overlays[2].animation_index = -1;
    unit->base_animation_state = 2;
    unit->unknown_29e = -1;
    unit->emotion_animation_frame = -1;
    unit->emotion_animation_index = -1;
    unit->scripted_base_animation_state = -1;
    unit->aiming_bounds_valid = 0;
    unit->aiming_bounds[0] = 0.0f;
    unit->aiming_bounds[1] = 0.0f;
    unit->aiming_bounds[2] = 0.0f;
    unit->aiming_bounds[3] = 0.0f;
    unit->looking_bounds_valid = 0;
    unit->looking_bounds[0] = 0.0f;
    unit->looking_bounds[1] = 0.0f;
    unit->looking_bounds[2] = 0.0f;
    unit->looking_bounds[3] = 0.0f;

    // seed all five aim/look/facing vectors from the object's current forward direction
    unit->looking_vector = obj->forward;
    unit->desired_looking_vector = obj->forward;
    unit->aiming_vector = obj->forward;
    unit->desired_aiming_vector = obj->forward;
    unit->desired_facing_vector = obj->forward;

    unit->persistent_control_ticks = 0;
    unit->dialogue_tag_index = (datum_index)-1;
    unit->flags |= 0x100; // NOTE: CEA calls this "must set up dialogue"

    // zero current_speech, pending_speech and the surrounding speech-state fields in one block
    // (0x388..0x403, 0x1f dwords)
    field = (uint32_t *)&unit->current_speech;
    for (i = 0x1f; i != 0; i--) {
        *field++ = 0;
    }
    unit->unknown_3f0 = (uint32_t)-1;

    unit_dialogue_determine_variant(object_index);

    // invalidate all four recent_damage slots (every byte, matching the original's blanket -1
    // dword loop rather than field-by-field defaults)
    field = (uint32_t *)unit->recent_damage;
    for (i = 0x10; i != 0; i--) {
        *field++ = (uint32_t)-1;
    }

    unit->delayed_damage_category = 0;
    unit->delayed_damage_ticks = 0;
    unit->delayed_damage_amount = 0.0f;
    unit->delayed_damage_responsible_object = (datum_index)-1;
    unit->death_time = -1;
    unit->encounter_index = -1;
    unit->squad_index = -1;
    unit->integrated_light_energy = 1.0f;
    unit->flaming_ticks = 0;
    unit->flaming_responsible_object = -1;
    unit->ai_communication_count = 0;
    unit->ai_communication_tick = -1;

    // seed the initial grenade count for the tag's default grenade type, if configured
    if ((uint16_t)tag->grenade_type <= 1 && tag->grenade_count >= 0) {
        unit->grenade_counts[tag->grenade_type] = (int8_t)tag->grenade_count;
    }

    obj->flags |= 0x6000; // NOTE: "just spawned" lighting-recompute bits, not in object_flags

    // feign-death eligibility roll, gated on all three tag scalars being configured
    if (tag->feign_death_threshold > 0.0f && tag->feign_death_time > 0.0f && tag->feign_death_chance > 0.0f) {
        float roll = random_real();
        if (roll < tag->feign_death_chance) { // unordered or equal clears the bit
            unit->flags |= 0x2000; // NOTE: CEA calls this "feign death allowed"
        } else {
            unit->flags &= ~0x2000u;
        }
    }

    // default team, only outside a running game engine and only when not already assigned
    // NOTE: written through object.owner_team; see the header note above
    if (current_game_engine == 0 && (obj->owner_team == 0 || obj->owner_team == -1)) {
        obj->owner_team = tag->default_team;
    }

    unit_set_or_test_seat_and_weapon_label(object_index, s_stand, (const char *)0, 1);

    if (network_game_mode != 1) { // not a network client
        unit_add_initial_weapons(object_index);
    }

    // queue mounted-weapon creation for this unit if any tag seat declares a built-in one
    if (tag->seats.count > 0) {
        int32_t seat_index = 0;
        UnitSeat *seats = (UnitSeat *)tag->seats.pointer;
        while (*(int32_t *)&seats[seat_index].built_in_gunner.tag_id == -1) {
            seat_index = seat_index + 1;
            if (seat_index >= (int32_t)tag->seats.count) {
                goto done_seat_scan;
            }
        }
        if (ai_globals_ptr->actors_valid != 0 && ai_globals_ptr->vehicle_entry_count < 8) {
            ai_globals_ptr->vehicle_entry_queue[ai_globals_ptr->vehicle_entry_count] = (datum_index)object_index;
            ai_globals_ptr->vehicle_entry_count = ai_globals_ptr->vehicle_entry_count + 1;
        }
    }
done_seat_scan:
    return 1;
}

#if 0
Original Ghidra decompilation (0x562180):

undefined4 missed_562180(uint param_1)

{
  uint *puVar1;
  int iVar2;
  undefined4 uVar3;
  uint uVar4;
  int iVar5;
  short sVar6;
  int iVar7;
  uint *puVar8;
  float fVar9;

  puVar1 = *(uint **)(*(int *)(DAT_008603b0 + 0x34) + 8 + (param_1 & 0xffff) * 0xc);
  iVar2 = *(int *)((*puVar1 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
  uVar3 = 0;
  if (*(int *)(iVar2 + 0x44) != -1) {
    *(undefined1 *)((int)puVar1 + 0x475) = 0;
    puVar1[0x12f] = 0xffffffff;
    puVar1[0xc6] = 0xffffffff;
    puVar1[0xbe] = 0xffffffff;
    puVar1[0xbf] = 0xffffffff;
    puVar1[0xc0] = 0xffffffff;
    puVar1[0xc1] = 0xffffffff;
    *(undefined2 *)((int)puVar1 + 0x2f2) = 0xffff;
    *(undefined2 *)(puVar1 + 0xbd) = 0xffff;
    *(undefined1 *)(puVar1 + 199) = 0xff;
    *(undefined1 *)((int)puVar1 + 0x31d) = 0xff;
    *(undefined1 *)(puVar1 + 200) = 0xff;
    *(undefined1 *)((int)puVar1 + 0x321) = 0xff;
    puVar1[0x86] = 0xffffffff;
    puVar1[0x7d] = 0xffffffff;
    puVar1[0x7e] = 0xffffffff;
    puVar1[0x7f] = 0xffffffff;
    puVar1[0x80] = 0xffffffff;
    *(undefined2 *)(puVar1 + 0xbc) = 0xffff;
    puVar1[0xc9] = 0xffffffff;
    puVar1[0xca] = 0xffffffff;
    *(undefined2 *)(puVar1 + 0xa6) = 0;
    *(undefined1 *)(puVar1 + 0xa8) = 0xff;
    *(undefined1 *)((int)puVar1 + 0x2a1) = 0xff;
    *(undefined1 *)((int)puVar1 + 0x2a2) = 0xff;
    *(undefined1 *)((int)puVar1 + 0x2a3) = 0xff;
    *(undefined1 *)(puVar1 + 0xa9) = 0;
    *(undefined1 *)((int)puVar1 + 0x2a5) = 0;
    *(undefined2 *)((int)puVar1 + 0x29a) = 0xffff;
    *(undefined2 *)(puVar1 + 0xa7) = 0xffff;
    *(undefined2 *)((int)puVar1 + 0x2aa) = 0xffff;
    *(undefined2 *)((int)puVar1 + 0x2ae) = 0xffff;
    *(undefined2 *)((int)puVar1 + 0x2b2) = 0xffff;
    *(undefined1 *)((int)puVar1 + 0x2a7) = 2;
    *(undefined2 *)((int)puVar1 + 0x29e) = 0xffff;
    *(undefined1 *)(puVar1 + 0xaa) = 0xff;
    *(undefined2 *)((int)puVar1 + 0x21e) = 0xffff;
    *(undefined1 *)((int)puVar1 + 0x20f) = 0xff;
    *(undefined1 *)((int)puVar1 + 0x2b6) = 0;
    puVar1[0xae] = 0;
    puVar1[0xaf] = 0;
    puVar1[0xb0] = 0;
    puVar1[0xb1] = 0;
    *(undefined1 *)((int)puVar1 + 0x2b7) = 0;
    puVar1[0xb2] = 0;
    puVar1[0xb3] = 0;
    puVar1[0xb4] = 0;
    puVar1[0xb5] = 0;
    puVar8 = puVar1 + 0x1d;
    puVar1[0x98] = *puVar8;
    puVar1[0x99] = puVar1[0x1e];
    puVar1[0x9a] = puVar1[0x1f];
    puVar1[0x95] = *puVar8;
    puVar1[0x96] = puVar1[0x1e];
    puVar1[0x97] = puVar1[0x1f];
    puVar1[0x8f] = *puVar8;
    puVar1[0x90] = puVar1[0x1e];
    puVar1[0x91] = puVar1[0x1f];
    puVar1[0x8c] = *puVar8;
    puVar1[0x8d] = puVar1[0x1e];
    puVar1[0x8e] = puVar1[0x1f];
    puVar1[0x89] = *puVar8;
    puVar1[0x8a] = puVar1[0x1e];
    puVar1[0x8b] = puVar1[0x1f];
    puVar1[0x84] = 0;
    puVar1[0xe1] = 0xffffffff;
    puVar1[0x81] = puVar1[0x81] | 0x100;
    puVar8 = puVar1 + 0xe2;
    for (iVar7 = 0x1f; iVar7 != 0; iVar7 = iVar7 + -1) {
      *puVar8 = 0;
      puVar8 = puVar8 + 1;
    }
    puVar1[0xfc] = 0xffffffff;
    FUN_005618e0();
    puVar8 = puVar1 + 0x10c;
    for (iVar7 = 0x10; iVar7 != 0; iVar7 = iVar7 + -1) {
      *puVar8 = 0xffffffff;
      puVar8 = puVar8 + 1;
    }
    *(undefined2 *)(puVar1 + 0x101) = 0;
    *(undefined2 *)((int)puVar1 + 0x406) = 0;
    puVar1[0x102] = 0;
    puVar1[0x103] = 0xffffffff;
    puVar1[0x107] = 0xffffffff;
    *(undefined2 *)(puVar1 + 0xcd) = 0xffff;
    *(undefined2 *)((int)puVar1 + 0x336) = 0xffff;
    puVar1[0xd1] = 0x3f800000;
    *(undefined1 *)((int)puVar1 + 0x28b) = 0;
    puVar1[0x104] = 0xffffffff;
    *(undefined2 *)((int)puVar1 + 0x42a) = 0;
    puVar1[0x10b] = 0xffffffff;
    sVar6 = *(short *)(iVar2 + 0x2c4);
    if (((-1 < sVar6) && (sVar6 < 2)) && (-1 < *(short *)(iVar2 + 0x2c6))) {
      *(undefined1 *)(sVar6 + 0x31e + (int)puVar1) = *(undefined1 *)(iVar2 + 0x2c6);
    }
    puVar1[4] = puVar1[4] | 0x6000;
    if (((0.0 < *(float *)(iVar2 + 0x22c)) && (0.0 < *(float *)(iVar2 + 0x230))) &&
       (0.0 < *(float *)(iVar2 + 0x244))) {
      fVar9 = random_real();
      if (*(float *)(iVar2 + 0x244) <= fVar9) {
        uVar4 = puVar1[0x81] & 0xffffdfff;
      }
      else {
        uVar4 = puVar1[0x81] | 0x2000;
      }
      puVar1[0x81] = uVar4;
    }
    if ((DAT_006f1d20 == 0) && (((short)puVar1[0x2e] == 0 || ((short)puVar1[0x2e] == -1)))) {
      *(undefined2 *)(puVar1 + 0x2e) = *(undefined2 *)(iVar2 + 0x180);
    }
    unit_set_or_test_seat_and_weapon_label(PTR_s_stand_0069fdec,0,1);
    if (DAT_00719720 != 1) {
      FUN_0056cf10();
    }
    iVar7 = DAT_00880354;
    sVar6 = 0;
    if (0 < *(int *)(iVar2 + 0x2e4)) {
      iVar5 = 0;
      while (*(int *)(iVar5 * 0x11c + 0x104 + *(int *)(iVar2 + 0x2e8)) == -1) {
        sVar6 = sVar6 + 1;
        iVar5 = (int)sVar6;
        if (*(int *)(iVar2 + 0x2e4) <= iVar5) {
          return 1;
        }
      }
      if ((*(char *)(DAT_00880354 + 1) != '\0') && (*(short *)(DAT_00880354 + 0x8b8) < 8)) {
        *(uint *)(DAT_00880354 + 0x8bc + *(short *)(DAT_00880354 + 0x8b8) * 4) = param_1;
        *(short *)(iVar7 + 0x8b8) = *(short *)(iVar7 + 0x8b8) + 1;
      }
    }
    uVar3 = 1;
  }
  return uVar3;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
