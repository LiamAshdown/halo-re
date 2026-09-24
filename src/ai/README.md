# src/ai -- the Blam actor / encounter / navigation module

halo.exe 1.0.10 retail, address range `0x401090..0x43ecf0`, 519 functions in
`modules.json`. **510 of them are rewritten here, one C file per function**; the other 9
are library or other-module code and are listed under [Misattributed
functions](#misattributed-functions). Together the rewritten files cover 210,918 bytes of
`.text`. There are no holes: every address in `out/phase4/ai_functions.md` is either a file
here or a row in the misattributed table.

Every file follows the house style set by `src/memory/`:

* a header comment with the address, size, name confidence, rewrite confidence, the
  evidence the name rests on, and the register convention (`blam-cc:` lines, because this
  binary passes a lot of arguments in registers that Ghidra could not attribute);
* `extern` declarations for every global and callee, each carrying its address;
* the C body, with every uncertainty marked `// UNSURE:`;
* the original Ghidra decompilation in a trailing `#if 0` block for diffing.

Nothing in this directory is compiled into the game. The gate is
`python tools/build_check.py ai`, a gcc `-fsyntax-only` type-check against `types/`:

```
build_check: 510 ok, 0 failed        # this module
build_check: 2057 ok, 0 failed       # whole repo
```

## 1. What the module contains

The AI module is five layers that share one header (`types/ai.h`):

| layer | what it owns | entry points |
|---|---|---|
| **actor** | one `actor` record per AI-controlled unit: its mode, order, aim, morale, movement and perception | `actor_new` `0x426760`, `actor_update` and the per-mode dispatch through `actor_type_procs` |
| **encounter / squad / platoon** | the runtime grouping built from `Scenario.encounters`: which actors belong together, whether they are awake, and their aggregate morale | `encounters_initialize` `0x435c00`, `encounters_reset` `0x435cb0`, `encounters_update` `0x435e00` |
| **prop** | one record per object an actor currently perceives; the input to target selection | `actor_find_or_allocate_prop` `0x43e270` |
| **path_find** | an A\*-style search over the structure BSP's navigation mesh, with its own 64 KB context | `path_find_run` `0x43a8b0` |
| **ai_search** | a separate, smaller search over a graph of circular obstacles, used for local steering around other units | `ai_search_run` `0x43be20`, `ai_navigate_around_obstacles` `0x43be90` |

plus two cross-cutting services:

* **AI communication / conversations** -- the chatter system (`ai_communication_initialize`
  `0x42cf20`, the `"ai conversation"` datum array, the event table at `0x00656b08`);
* **packed AI references** -- the 32-bit `(encounter, platoon-or-squad, kind)` handle the
  scripting layer passes around, decoded by `ai_reference_parse` `0x432320` and expanded by
  the `ai_reference_*` / `ai_object_list_*` families.

### Globals this module owns

| address | C name | contents |
|---|---|---|
| `0x00880354` | `ai_globals *ai_global_data` | 0x8dc bytes of game state: the valid flags, the unassigned-actor list head, the conversation event ring, the per-object attention table, the vehicle-entry queue |
| `0x00880360` | `data_array *actor_data` | `"actor"`, stride 0x724, capacity 0x100 |
| `0x0088035c` | `data_array *swarm_data` | `"swarm"`, stride 0x98, capacity 0x20 |
| `0x00880358` | `data_array *swarm_component_data` | `"swarm component"`, stride 0x40, capacity 0x100 |
| `0x008802c0` | `data_array *prop_data` | `"prop"`, stride 0x138, capacity 0x300 |
| `0x008802c8` | `data_array *encounter_data` | `"encounter"`, stride 0x6c, capacity 0x80 |
| `0x008802cc` | `encounter_squad_state *encounter_squad_states` | flat 0x8000-byte table, 1024 records of 0x20 |
| `0x008802c4` | `encounter_platoon_state *encounter_platoon_states` | flat 0x1000-byte table, 256 records of 0x10 |
| `0x008802d0` | `data_array *ai_pursuit_data` | `"ai pursuit"`, stride 0x28, capacity 0x100 |
| `0x008802d4` | `data_array *ai_conversation_data` | `"ai conversation"`, stride 0x64, capacity 8 |
| `0x00880380` / `0x00880540` / `0x008805a0` | the three obstacle-avoidance direction tables | precomputed once by `0x41a2d0` |

### The encounter hierarchy, in the order it is built

```
encounters_initialize   0x435c00   creates encounter_data, ai_pursuit_data and the two flat tables
encounters_reset        0x435cb0   wipes them, then for each Scenario.encounters entry:
  encounter_new         0x437060     one encounter datum + a run of squad and platoon records
    encounter_squad_reset_starting_location_mask 0x436f90   per squad
encounters_spawn_initial 0x435d50   for each encounter not flagged "deferred":
  encounter_spawn_squads 0x437510     difficulty-scaled actor count per squad
    encounter_squad_spawn_actor 0x438e20
      squad_pick_random_starting_location 0x437220
      actor_place_new_unit 0x427080 -> actor_new 0x426760 -> encounter_add_actor 0x436770
encounters_update        0x435e00   per tick: morale, timers, reinforcements, targets
  encounters_recompute_dirty 0x435f00 / encounter_recompute_morale 0x437940
  encounters_update_activation 0x437e20 -> encounter_activate 0x437710 / _deactivate 0x437870
```

## 2. Struct layouts

`types/ai.h` is the source of truth; these tables are generated from it. Sizes marked
"verified" are pinned by an allocation, a `memset`/`crc32` length, or a stride the code
divides by -- not by guesswork. A compile-time check of all 68 sized structs against their
documented sizes passes (the five apparent mismatches are all pointer-width inflation on the
64-bit check host, and are exact on 32-bit).
#### `actor` // size 0x724

| offset | field | type | note |
|---|---|---|---|
| `0x00` | `identifier` | `int16_t` | datum_header |
| `0x04` | `type` | `int16_t` | ActorType, copied from the actor tag type at Actor+0x14 by actor_new; indexes actor_type_procs |
| `0x06` | `swarm` | `uint8_t` | Actor.flags bit 26 "swarm"; the order builders refuse to act while it is set |
| `0x08` | `active` | `uint8_t` | actor_set_units_active and squad_activate gate on this |
| `0x0b` | `swarm_pending` | `uint8_t` | encounter_activate sets it when a swarm actor could not get a swarm |
| `0x13` | `keep_unit_alive` | `uint8_t` | actor_attach_to_unit marks the unit pending-delete when this is clear |
| `0x18` | `unit_index` | `datum_index` | the one unit object this actor controls; the unit points back at 0x1f4 |
| `0x1c` | `counts_toward_encounter` | `uint8_t` | actor_unlink_unit decrements encounter+0x1c only when set |
| `0x1e` | `cluster_count` | `int16_t` | actor_link_to_unit_cluster increments, actor_remove_from_unit_cluster decrements |
| `0x24` | `cluster_unit_index` | `datum_index` | head of the unit cluster list, chained through object+0x1fc |
| `0x28` | `swarm_index` | `datum_index` | actor_create_swarm / actor_delete_swarm |
| `0x2c` | `next_in_encounter` | `datum_index` | next actor in the encounter member list, or in the unassigned list |
| `0x34` | `encounter_index` | `datum_index` | owning encounter datum, none while unassigned |
| `0x3a` | `squad_index` | `int16_t` | index of this actor encounter_squad_state, relative to encounter.first_squad |
| `0x3c` | `platoon_index` | `int16_t` | index of this actor encounter_platoon_state, or -1 |
| `0x3e` | `team` | `int16_t` | kept in sync with object+0xb8 and encounter.team |
| `0x4a` | `idle_counter` | `int16_t` | 0x429430 advances it and trips the global update stagger past 15 |
| `0x4c` | `needs_new_path` | `uint8_t` | 0x4017b0 issues a fresh path request while set; 0x429430 also writes it |
| `0x50` | `first_prop` | `datum_index` | head of the prop list, chained through prop.next_in_actor at +0x08 |
| `0x58` | `actor_definition_tag` | `datum_index` | the actor tag index; actor_get_actor_definition can override it per unit |
| `0x5c` | `actor_variant_tag` | `datum_index` | the actor_variant tag index actor_new was called with |
| `0x6a` | `awareness_level` | `int16_t` | 0..3; actor_set_mode clamps it to 2 or 3 by mode, actor_update_awareness_level drives it |
| `0x6c` | `mode` | `int16_t` | actor_set_mode writes it; indexes actor_mode_definitions |
| `0x70` | `mode_changed` | `uint8_t` | actor_set_mode sets 1 |
| `0x99` | `flying` | `uint8_t` | Actor.flags bit 21 "flying"; read by every steering and step-test routine |
| `0x9c` | `mode_data[0x84]` | `uint8_t` | actor_set_mode memcpys actor_mode_definition.data_size bytes here. |
| `0x120` | `aim_origin` | `real_point3d` | the firing / eye origin; 0x40e7b0 traces from it and 0x40fcb0 |
| `0x12c` | `body_position` | `real_point3d` | the position every range and scoring routine uses; read by |
| `0x158` | `active_unit_index` | `datum_index` | preferred unit object for movement; 0x4193d0 falls back to unit_index |
| `0x160` | `order_committed` | `uint8_t` | the order builders set it once the actor commits to the order they built |
| `0x174` | `facing` | `real_vector3d` | the actor unit forward vector, NOT a position: all 35 arithmetic |
| `0x180` | `facing_unknown_180` | `real_vector3d` | snapshotted to 0x708 |
| `0x18c` | `facing_unknown_18c` | `real_vector3d` | snapshotted to 0x714 |
| `0x1dc` | `conversation_index` | `datum_index` | ai_conversation_stop clears this and conversation_participant |
| `0x1e0` | `conversation_participant` | `datum_index` |  |
| `0x1ec` | `tally` | `actor_target_tally` | the 0x7b-byte perception tally actor_choose_best_target |
| `0x268` | `target_combat_status` | `int16_t` | actor_update_target_combat_status writes it, actor_update_awareness_level reads it |
| `0x270` | `target_unit_index` | `datum_index` | the unit the actor is fighting; actor_choose_best_target writes it |
| `0x280` | `danger_type` | `int16_t` | 0x41ea60 and 0x41ec90 only register a danger that outranks this |
| `0x282` | `danger_unknown_282` | `int16_t` |  |
| `0x284` | `danger_unknown_284` | `int16_t` |  |
| `0x286` | `danger_unknown_286` | `uint8_t` |  |
| `0x28c` | `danger_object_index` | `datum_index` |  |
| `0x290` | `danger_unknown_290` | `uint32_t` |  |
| `0x294` | `danger_unknown_294` | `float` |  |
| `0x298` | `danger_unknown_298` | `float` |  |
| `0x29c` | `danger_unknown_29c` | `float` |  |
| `0x2a0` | `danger_unknown_2a0` | `float` |  |
| `0x2a4` | `danger_unknown_2a4` | `uint32_t` |  |
| `0x2a8` | `danger_unknown_2a8` | `uint32_t` |  |
| `0x2ac` | `danger_unknown_2ac` | `uint32_t` |  |
| `0x2b0` | `flee_from_point` | `real_point3d` | 0x4146c0 resolves the point the actor flees away from; the |
| `0x2c8` | `danger_segment_end` | `real_point3d` | 0x4112b0 builds the segment flee_from_point -> here |
| `0x2d4` | `danger_unknown_2d4` | `float` |  |
| `0x2d8` | `danger_radius` | `float` | the sphere around danger_center a candidate has to be inside |
| `0x2dc` | `danger_center` | `real_point3d` |  |
| `0x2ee` | `look_at_priority` | `int16_t` | 0x421bc0 keeps only the highest-priority look-at point |
| `0x2f4` | `look_at_unknown_2f4` | `uint32_t` |  |
| `0x2f8` | `look_at_unknown_2f8` | `float` |  |
| `0x2fc` | `look_at_unknown_2fc` | `uint32_t` |  |
| `0x300` | `look_at_unknown_300` | `float` |  |
| `0x304` | `look_at_unknown_304` | `uint32_t` |  |
| `0x312` | `search_priority` | `int16_t` | 0x421af0 keeps only the highest-priority search position |
| `0x314` | `search_unknown_314` | `uint8_t` |  |
| `0x318` | `search_unknown_318` | `uint32_t` |  |
| `0x31c` | `search_unknown_31c` | `uint32_t` |  |
| `0x320` | `search_unknown_320` | `uint32_t` |  |
| `0x324` | `search_unknown_324` | `uint32_t` |  |
| `0x328` | `search_unknown_328` | `uint32_t` |  |
| `0x32c` | `search_unknown_32c` | `uint8_t` |  |
| `0x330` | `search_unknown_330` | `uint32_t` |  |
| `0x338` | `search_unknown_338` | `uint32_t` |  |
| `0x33c` | `search_unknown_33c` | `uint32_t` |  |
| `0x340` | `search_unknown_340` | `uint32_t` |  |
| `0x344` | `search_unknown_344` | `uint32_t` |  |
| `0x348` | `search_unknown_348` | `uint8_t` |  |
| `0x34a` | `perception_event` | `int16_t` | 0x422070 records the highest-priority pending perception event |
| `0x34c` | `perception_event_data` | `int32_t` |  |
| `0x376` | `ignores_glass` | `uint8_t` | actor_new rolls Actor.glass_ignorance_chance at Actor+0x90 once into this |
| `0x37c` | `search_wait_time` | `float` | 0x4028e0 reads this and unknown_388 as reaction wait thresholds |
| `0x3b8` | `firing_position_index` | `int16_t` | the encounter firing position this actor has claimed, or -1 |
| `0x3c6` | `recognition_cursor` | `int16_t` | ring cursor, advanced modulo 4 by 0x4141a0 |
| `0x3c8` | `recognition[4]` | `actor_recognition_entry` | actor_set_mode and 0x414140 reset all four firing_position_index to -1 |
| `0x3d8` | `recognition_valid` | `uint8_t` | 0x4141a0 sets it, 0x414140 and actor_set_mode clear it |
| `0x3d9` | `recognition_type` | `uint8_t` |  |
| `0x3dc` | `recognition_position` | `real_point3d` | copied out of the encounter ScenarioFiringPosition block (stride 0x18) |
| `0x3e8` | `vocalization_unknown_3e8` | `int16_t` |  |
| `0x3ec` | `vocalization_unknown_3ec` | `int16_t` |  |
| `0x400` | `queued_movement` | `actor_movement_action` | the action the setters at 0x417610..0x417910 write |
| `0x418` | `secondary_action` | `int16_t` | 0x417a60 queues it, actor_action_has_queued_secondary reads it |
| `0x46c` | `active_movement` | `actor_movement_action` | the six dwords the setters copy over from queued_movement |
| `0x484` | `movement_completed` | `uint8_t` | actor_movement_action_complete sets it |
| `0x4a0` | `movement_timer` | `int32_t` | actor_movement_action_complete zeroes it |
| `0x4a8` | `movement_action_complete` | `uint8_t` | actor_movement_action_is_complete returns it |
| `0x544` | `vocalization_line` | `int16_t` | actor_clear_vocalization zeroes 0x544, 0x546 and 0x548 |
| `0x546` | `vocalization_variant` | `int16_t` |  |
| `0x548` | `vocalization_state` | `int16_t` |  |
| `0x54c` | `vocalization_unknown_54c` | `uint32_t` |  |
| `0x550` | `vocalization_unknown_550` | `uint32_t` |  |
| `0x554` | `vocalization_unknown_554` | `uint32_t` |  |
| `0x558` | `vocalization_unknown_558` | `uint32_t` |  |
| `0x5a4` | `position_cache_a` | `real_point3d` | actor_movement_update copies position here every tick |
| `0x5b0` | `position_cache_b` | `real_point3d` | actor_new seeds all three caches from the zero vector at 0x00696718 |
| `0x5bc` | `position_cache_c` | `real_point3d` |  |
| `0x5dc` | `avoidance_direction` | `real_vector3d` | actor_movement_update low-pass filters the avoidance |
| `0x5e8` | `avoidance_scale` | `float` | the matching low-pass filtered magnitude, snapped to |
| `0x608` | `vitality_wait_time` | `float` | 0x4028e0 uses it as the vitality-based reaction delay |
| `0x62c` | `wander_unknown_62c` | `float` | 0x40fcb0 destination and velocity scratch |
| `0x630` | `wander_unknown_630` | `float` |  |
| `0x634` | `wander_unknown_634` | `float` |  |
| `0x638` | `wander_unknown_638` | `float` |  |
| `0x64c` | `wander_unknown_64c` | `real_vector3d` |  |
| `0x664` | `wander_unknown_664` | `real_vector3d` |  |
| `0x670` | `wander_unknown_670` | `real_vector3d` |  |
| `0x67c` | `grenade_aim_direction` | `real_vector3d` | written by 0x40f7e0 and 0x40fcb0 |
| `0x69c` | `perception_scale` | `float` | 0x42aa90 scales hearing and awareness ranges by this |
| `0x6a8` | `grenade_impact_point` | `real_point3d` | 0x410710 records a validated landing point |
| `0x6bc` | `grenade_unknown_6bc` | `float` | throw-direction scratch written by 0x410a60 |
| `0x6c0` | `grenade_unknown_6c0` | `float` |  |
| `0x6c4` | `grenade_unknown_6c4` | `float` |  |
| `0x6c8` | `grenade_unknown_6c8` | `float` |  |
| `0x6cc` | `grenade_eligible` | `uint8_t` | 0x42f260 caches the eligibility test here |
| `0x6ce` | `grenade_recheck_ticks` | `int16_t` | actor_new sets 30 |
| `0x6d0` | `flags` | `uint32_t` | bit 0x2 set by 0x42a5b0, bit 0x400 by 0x4347b0, bit 0x800 by 0x42a5e0 (override target) |
| `0x6e0` | `queued_look_vector` | `real_vector3d` | actor_snapshot_orientation seeds it from the zero vector at 0x00696714 |
| `0x6fc` | `snapshot_facing` | `real_vector3d` | copy of facing taken by actor_snapshot_orientation |
| `0x708` | `snapshot_unknown_708` | `real_vector3d` | copy of facing_unknown_180 |
| `0x714` | `snapshot_unknown_714` | `real_vector3d` | copy of facing_unknown_18c |
| `0x720` | `override_target` | `datum_index` | 0x42a5e0 writes it together with flags bit 0x800 |

*(only the named fields; the 233 `unknown_*` slots that fill the rest of the actor are in `types/ai.h`)*

#### `prop` // size 0x138

| offset | field | type | note |
|---|---|---|---|
| `0x00` | `identifier` | `int16_t` | datum_header |
| `0x04` | `actor_index` | `datum_index` | the actor whose prop list this is on |
| `0x08` | `next_in_actor` | `datum_index` | next prop in actor.first_prop list |
| `0x0c` | `pair_index` | `datum_index` | the paired prop allocated by 0x43e910 / 0x43e980 |
| `0x10` | `actor_type` | `int16_t` | the owning actor type, or 6 for a swarm prop, or -1 |
| `0x12` | `object_type` | `int16_t` | object+0xb8 of the tracked object |
| `0x14` | `has_parent` | `uint8_t` | set when the tracked object has a parent unit (object+0x1f8) |
| `0x18` | `object_index` | `datum_index` | the tracked object |
| `0x1c` | `owner_actor_index` | `datum_index` | the actor that currently owns the tracked object, or none |
| `0x24` | `kind` | `int16_t` | 0..1 are reserved kinds, 4..5 the shared / vault kinds, 6 the parented kind |
| `0x50` | `desirability` | `float` | actor_rate_potential_target writes the score here |
| `0x60` | `is_unit` | `uint8_t` | 0x45bd50 classifies the tracked object; 46 functions branch on it |
| `0x64` | `combat_dirty` | `uint8_t` | actor_target_reset_combat_flags sets it |
| `0x6c` | `seen_state` | `int16_t` | actor_target_reset_seen_flags sets 0xffff |
| `0x74` | `seen` | `uint8_t` | actor_target_reset_seen_flags clears it |
| `0xa4` | `engaged` | `uint8_t` | 0x41fa80 marks the target actively engaged |
| `0xaa` | `shots_fired` | `int16_t` | actor_target_reset_shot_counters zeroes 0xaa, 0xac and 0xae |
| `0xac` | `shots_hit` | `int16_t` |  |
| `0xae` | `shots_unknown_ae` | `int16_t` |  |
| `0xb9` | `noticed_a` | `uint8_t` | set by 0x41fb00 (unit+0xb9), cleared by actor_target_reset_combat_flags |
| `0xba` | `noticed_b` | `uint8_t` | set by 0x41fb60 (unit+0xba) |
| `0xbb` | `noticed_c` | `uint8_t` | set by 0x41fbc0 |
| `0xbc` | `last_known_position` | `real_point3d` |  |
| `0xc8` | `aim_offset` | `real_point3d` | 0x41c4b0 refreshes the aim marker offsets |
| `0xec` | `path_surface_index` | `int32_t` | actor_target_data_refresh @0x41c4b0 resets it to -1 |
| `0xf0` | `ground_position` | `real_point3d` | the destination a non-flying actor steers to for a |
| `0x100` | `cluster_index` | `int16_t` | the BSP cluster the tracked object was last seen in, or -1 |
| `0x110` | `relationship_object_index` | `int32_t` | actor_target_get_relationship_object caches it lazily |
| `0x11c` | `distance` | `float` | the ascending sort key of ai_target_distance_qsort_compare |
| `0x127` | `is_vault` | `uint8_t` | Unit type definition byte +0x106 bit 2; 29 functions branch on it |
| `0x12e` | `is_parented` | `uint8_t` | 0x43e640 sets it when the tracked object has a parent (object+0x30) |

*(only the named fields; the 83 `unknown_*` slots that fill the rest of the prop are in `types/ai.h`)*

#### `encounter` // size 0x6c

| offset | field | type | note |
|---|---|---|---|
| `0x00` | `identifier` | `int16_t` | datum_header |
| `0x02` | `team` | `int16_t` | ScenarioEncounter.team_index |
| `0x04` | `first_squad` | `int16_t` | index of this encounter first encounter_squad_state |
| `0x06` | `squad_count` | `int16_t` | ScenarioEncounter.squads.count |
| `0x08` | `first_platoon` | `int16_t` | index of this encounter first encounter_platoon_state |
| `0x0a` | `platoon_count` | `int16_t` | ScenarioEncounter.platoons.count |
| `0x0c` | `unknown_0c` | `uint8_t` |  |
| `0x0d` | `units_active` | `uint8_t` | encounter_add_actor calls actor_set_units_active when set |
| `0x0e` | `activation_delay` | `int16_t` | ticks remaining before encounters_update_activation re-evaluates this encounter; encounter_add_actor sets 0x96 |
| `0x10` | `activation_tick` | `int32_t` | encounter_new sets -1; encounter_activate stamps the current game tick |
| `0x14` | `first_actor` | `datum_index` | head of the member list, chained through actor.next_in_encounter |
| `0x18` | `member_count` | `int16_t` | encounter_add_actor increments, squad_remove_actor decrements |
| `0x1a` | `unknown_1a` | `int16_t` | encounter_recompute_morale snapshots unknown_2a here when the retreat latch clears |
| `0x1c` | `live_count` | `int16_t` | only actors with counts_toward_encounter set are counted |
| `0x1e` | `unknown_1e[2]` | `uint8_t` |  |
| `0x20` | `unknown_20` | `int16_t` | squad_create zeroes it; 0x437820 records recent zone ids near here |
| `0x22` | `unknown_22` | `int16_t` |  |
| `0x24` | `unknown_24` | `int16_t` |  |
| `0x26` | `unknown_26[2]` | `uint8_t` |  |
| `0x28` | `dirty` | `uint8_t` | set by every member add / remove; 0x435f00 re-runs morale for dirty encounters |
| `0x29` | `unknown_29` | `uint8_t` |  |
| `0x2a` | `unknown_2a` | `int16_t` |  |
| `0x2c` | `unknown_2c` | `int16_t` |  |
| `0x2e` | `unknown_2e` | `int16_t` |  |
| `0x30` | `unknown_30` | `int16_t` |  |
| `0x32` | `unknown_32[2]` | `uint8_t` |  |
| `0x34` | `average_vitality` | `float` | encounter_recompute_morale sums one vitality sample per live member here and then divides by member_count |
| `0x38` | `first_pursuit` | `datum_index` | head of the ai_pursuit ("recently seen object") list |
| `0x3c` | `unknown_3c` | `uint8_t` | ScenarioEncounter.flags bit 1 |
| `0x3d` | `unknown_3d` | `uint8_t` |  |
| `0x3e` | `unknown_3e` | `int16_t` | squad_create zeroes it |
| `0x40` | `unknown_40` | `uint8_t` | ScenarioEncounter.flags bit 2 |
| `0x41` | `unknown_41` | `uint8_t` | ScenarioEncounter.flags bit 3 |
| `0x42` | `unknown_42` | `uint8_t` | squad_create sets 1 |
| `0x43` | `unknown_43` | `uint8_t` |  |
| `0x44` | `unknown_44` | `uint8_t` | squad_create zeroes it |
| `0x45` | `unknown_45` | `uint8_t` | squad_create zeroes it |
| `0x46` | `unknown_46` | `uint8_t` | squad_create zeroes it |
| `0x47` | `unknown_47` | `uint8_t` |  |
| `0x48` | `unknown_48` | `uint8_t` |  |
| `0x49` | `unknown_49` | `uint8_t` |  |
| `0x4a` | `unknown_4a` | `int16_t` |  |
| `0x4c` | `unknown_4c` | `int16_t` |  |
| `0x4e` | `unknown_4e[2]` | `uint8_t` |  |
| `0x50` | `unknown_50` | `datum_index` | squad_create sets -1 |
| `0x54` | `unknown_54` | `datum_index` | squad_create sets -1 |
| `0x58` | `unknown_58` | `int32_t` | squad_create sets -1; 0x43e270 compares it against actor+0x3a0 |
| `0x5c` | `unknown_5c` | `datum_index` | squad_create sets -1 |
| `0x60` | `unknown_60` | `uint8_t` |  |
| `0x61` | `unknown_61` | `uint8_t` |  |
| `0x62` | `unknown_62` | `int16_t` |  |
| `0x64` | `unknown_64` | `int32_t` |  |
| `0x68` | `unknown_68` | `int16_t` |  |
| `0x6a` | `unknown_6a` | `int16_t` |  |

#### `encounter_squad_state` // size 0x20

| offset | field | type | note |
|---|---|---|---|
| `0x00` | `starting_location_mask` | `uint32_t` | locations this squad is allowed to use |
| `0x04` | `starting_location_free` | `uint32_t` | locations not yet handed out this round |
| `0x08` | `unknown_08` | `float` |  |
| `0x0c` | `respawn_budget` | `int16_t` | ScenarioSquad.respawn_total (999 when that is 0), only set when the squad has a respawn range; the reinforcement spawner decrements it |
| `0x0e` | `unknown_0e` | `int16_t` |  |
| `0x10` | `unknown_10` | `uint8_t` | ScenarioSquad.flags bit 5 |
| `0x11` | `unknown_11` | `uint8_t` | encounter_new zeroes it |
| `0x12` | `squad_delay_ticks` | `int16_t` | ftol(ScenarioSquad.squad_delay_time * 30), or 999 when ScenarioSquad.flags bit 3 is set |
| `0x14` | `unknown_14` | `uint8_t` | read as a flag by encounter_gather_occupied_bsp_clusters |
| `0x15` | `unknown_15` | `uint8_t` |  |
| `0x16` | `member_count` | `int16_t` | encounter_add_actor increments, squad_remove_actor decrements |
| `0x18` | `unknown_18` | `int16_t` |  |
| `0x1a` | `unknown_1a` | `int16_t` |  |
| `0x1c` | `average_vitality` | `float` | encounter_recompute_morale @0x437940 sums one vitality sample per live member here and then divides by member_count |

#### `encounter_platoon_state` // size 0x10

| offset | field | type | note |
|---|---|---|---|
| `0x00` | `unknown_00` | `uint8_t` | ScenarioPlatoon.flags bit 2 |
| `0x01` | `unknown_01[3]` | `uint8_t` |  |
| `0x04` | `member_count` | `int16_t` | encounter_add_actor increments, squad_remove_actor decrements |
| `0x06` | `unknown_06` | `int16_t` |  |
| `0x08` | `unknown_08` | `int16_t` |  |
| `0x0a` | `unknown_0a` | `int16_t` |  |
| `0x0c` | `average_vitality` | `float` | same running sum as encounter_squad_state.average_vitality, divided by member_count at 0x04 |

#### `encounter_iterator` // size 0x18

| offset | field | type | note |
|---|---|---|---|
| `0x04` | `next_index` | `int16_t` | written as an int16, read as an int32 by data_iterator_next |
| `0x06` | `pad_06[2]` | `uint8_t` |  |
| `0x08` | `index` | `datum_index` | handle of the encounter the last _next returned |
| `0x0c` | `signature` | `uint32_t` | data XOR 0x69746572 ('iter'), the data_iterator self-check |
| `0x10` | `encounter_index` | `datum_index` | the loop body copy of index |
| `0x14` | `active_only` | `uint8_t` | skip encounters whose units_active is clear |
| `0x15` | `pad_15[3]` | `uint8_t` |  |

#### `ai_pursuit` // size 0x28

| offset | field | type | note |
|---|---|---|---|
| `0x00` | `identifier` | `int16_t` | datum_header |
| `0x02` | `type` | `int16_t` | the per-type key squad_recent_object_get_or_create matches on |
| `0x04` | `last_tick` | `int32_t` | the tick of the last sighting, or -1 while empty |
| `0x08` | `count` | `int16_t` | total sightings recorded |
| `0x0a` | `cursor` | `int16_t` | next slot to overwrite, modulo 6 |
| `0x0c` | `object_index[6]` | `datum_index` |  |
| `0x24` | `next` | `datum_index` | next pursuit record in the encounter list |

#### `ai_conversation` // size 0x64

| offset | field | type | note |
|---|---|---|---|
| `0x00` | `identifier` | `int16_t` | datum_header |
| `0x02` | `definition_index` | `int16_t` | index into Scenario.ai_conversations (stride 0x74) |
| `0x04` | `priority` | `uint8_t` | ai_conversation_new stores its allow_eviction argument here |
| `0x05` | `unknown_05` | `uint8_t` |  |
| `0x06` | `unknown_06` | `uint8_t` |  |
| `0x07` | `unknown_07[5]` | `uint8_t` |  |
| `0x0c` | `start_tick` | `int32_t` | the game tick the instance was created |
| `0x10` | `unknown_10` | `int32_t` |  |
| `0x14` | `participant_mask` | `uint32_t` | bit i set once participant i has been resolved |
| `0x18` | `unknown_18` | `uint32_t` |  |
| `0x1c` | `unknown_1c` | `uint32_t` |  |
| `0x20` | `unknown_20` | `int16_t` |  |
| `0x22` | `unknown_22` | `int16_t` |  |
| `0x24` | `unknown_24` | `uint32_t` |  |
| `0x28` | `participant_actor[8]` | `datum_index` | one actor datum per resolved participant |
| `0x48` | `unknown_48` | `int16_t` | ai_conversation_new sets 0xffff |
| `0x4a` | `unknown_4a` | `int16_t` |  |
| `0x4c` | `unknown_4c` | `int16_t` |  |
| `0x4e` | `unknown_4e` | `int16_t` |  |
| `0x50` | `unknown_50` | `int32_t` |  |
| `0x54` | `unknown_54` | `int32_t` |  |
| `0x58` | `unknown_58` | `uint32_t` |  |
| `0x5c` | `unknown_5c` | `uint32_t` |  |
| `0x60` | `unknown_60` | `uint8_t` |  |
| `0x61` | `unknown_61` | `uint8_t` |  |
| `0x62` | `unknown_62` | `uint8_t` |  |
| `0x63` | `unknown_63` | `uint8_t` |  |

#### `ai_conversation_event` // size 0x10

| offset | field | type | note |
|---|---|---|---|
| `0x00` | `definition_index` | `int16_t` | the ai_conversation definition that stopped |
| `0x02` | `reason_a` | `uint8_t` |  |
| `0x03` | `reason_b` | `uint8_t` |  |
| `0x04` | `tick` | `int32_t` | game time at 0x006f1d6c+0x0c |
| `0x08` | `unknown_08[8]` | `uint8_t` |  |

#### `ai_globals` // size 0x8dc

| offset | field | type | note |
|---|---|---|---|
| `0x00` | `initialized` | `uint8_t` | ai_reset_for_new_map sets it |
| `0x01` | `actors_valid` | `uint8_t` | every actor and encounter entry point returns early when this is clear |
| `0x02` | `unknown_02` | `uint8_t` | ai_reset_for_new_map sets it |
| `0x03` | `stagger_claimed` | `uint8_t` | 0x429430 claims the per-tick idle slot |
| `0x04` | `stagger_threshold` | `int16_t` |  |
| `0x06` | `stagger_highest` | `int16_t` | 0x429430 tracks the highest idle counter seen |
| `0x08` | `unknown_08` | `datum_index` | ai_reset_for_new_map sets none; also the head of the unassigned actor list |
| `0x0c` | `unknown_0c` | `float` |  |
| `0x10` | `communication_valid` | `uint8_t` | 0x42d230 sets it |
| `0x11` | `unknown_11` | `uint8_t` |  |
| `0x12` | `unknown_12` | `int16_t` |  |
| `0x14` | `unknown_14` | `datum_index` | 0x42d230 zeroes 0x14..0x2b, ai_reset_for_new_map sets them all to none |
| `0x18` | `unknown_18` | `datum_index` |  |
| `0x1c` | `unknown_1c` | `datum_index` |  |
| `0x20` | `unknown_20` | `datum_index` |  |
| `0x24` | `unknown_24` | `datum_index` |  |
| `0x28` | `unknown_28` | `datum_index` |  |
| `0x2c` | `conversation_event_count` | `int16_t` | high-water mark, capped at 16 |
| `0x2e` | `conversation_event_cursor` | `int16_t` | next ring slot, modulo 16 |
| `0x30` | `conversation_events[16]` | `ai_conversation_event` | 0x42d230 zeroes the whole 0x100-byte ring |
| `0x130` | `unknown_130` | `int16_t` | ai_reset_for_new_map zeroes it |
| `0x132` | `unknown_132` | `int16_t` | ai_reset_for_new_map zeroes it |
| `0x134` | `unknown_134[0x280]` | `uint8_t` | ai_reset_for_new_map zeroes 0xa0 dwords from here |
| `0x3b4` | `unknown_3b4` | `uint8_t` | ai_reset_for_new_map sets it |
| `0x3b5` | `unknown_3b5` | `uint8_t` |  |
| `0x3b6` | `unknown_3b6` | `int16_t` | 0x435900 uses it as the per-object record table count |
| `0x3b8` | `unknown_3b8[56]` | `uint8_t` |  |
| `0x3f0` | `unknown_3f0` | `int32_t` | ai_communication_record_line_played |
| `0x3f4` | `unknown_3f4[6]` | `uint8_t` |  |
| `0x3fa` | `unknown_3fa` | `int16_t` |  |
| `0x3fc` | `unknown_3fc[1212]` | `uint8_t` |  |
| `0x8b8` | `vehicle_entry_count` | `int16_t` | ai_process_vehicle_entry_queue drains the queue and zeroes this |
| `0x8ba` | `unknown_8ba[2]` | `uint8_t` |  |
| `0x8bc` | `vehicle_entry_queue[8]` | `datum_index` | unit object indices waiting for a seat |

#### `ai_object_attention_record` // size 0x28

| offset | field | type | note |
|---|---|---|---|
| `0x00` | `object_index` | `datum_index` | the key; the search compares the whole 32-bit handle |
| `0x04` | `weight` | `float` | seeded to 8.0 on creation |
| `0x08` | `unknown_08[0x20]` | `uint8_t` | zeroed on creation, never read inside this module |

#### `ai_scored_candidate` // size 0x10

| offset | field | type | note |
|---|---|---|---|
| `0x00` | `handle` | `datum_index` | the actor the candidate belongs to; none marks an empty slot |
| `0x04` | `score` | `float` | the weight, always positive for a live slot |
| `0x08` | `payload` | `datum_index` | the prop the score came from, or none |
| `0x0c` | `key` | `datum_index` | the object that prop tracks, or none |

#### `swarm` // size 0x98

| offset | field | type | note |
|---|---|---|---|
| `0x00` | `identifier` | `int16_t` | datum_header |
| `0x02` | `component_count` | `int16_t` | 0..16 |
| `0x04` | `actor_index` | `datum_index` | the actor that owns this swarm |
| `0x08` | `unknown_08[4]` | `uint8_t` |  |
| `0x0c` | `aggregate_position` | `real_point3d` | the mean of every component's swarm_component.position, recomputed each tick by actor_refresh_combat_context @0x4297a0 |
| `0x18` | `unit_index[16]` | `datum_index` | one unit object per component |
| `0x58` | `component_index[16]` | `datum_index` | the matching swarm_component datums |

#### `swarm_component` // size 0x40

| offset | field | type | note |
|---|---|---|---|
| `0x00` | `identifier` | `int16_t` | datum_header |
| `0x02` | `flags` | `uint8_t` | bit 3 read by ai_object_list_max_flee_grade |
| `0x03` | `unknown_03` | `uint8_t` |  |
| `0x04` | `position` | `real_point3d` | object_get_position of the component unit |
| `0x10` | `marker_index` | `datum_index` | object+0x4d8 when object+0xb4 is 0, otherwise none |
| `0x14` | `unknown_14` | `uint32_t` | swarm_add_component sets -1 |
| `0x18` | `unknown_18[40]` | `uint8_t` |  |

#### `actor_mode_definition` // size 0x38

| offset | field | type | note |
|---|---|---|---|
| `0x00` | `data_size` | `uint32_t` | how many bytes actor_set_mode copies into actor.mode_data |
| `0x04` | `combat_grade` | `int16_t` | nonzero raises actor.awareness_level to 3, zero clamps it to 2 |
| `0x06` | `unknown_06[2]` | `uint8_t` |  |
| `0x08` | `enter_proc` | `uint32_t` | actor_set_mode calls it after switching in |
| `0x0c` | `unknown_0c[8]` | `uint8_t` |  |
| `0x14` | `update_proc` | `uint32_t` | actor_invoke_type_handler calls it |
| `0x18` | `exit_proc` | `uint32_t` | actor_set_mode calls the outgoing mode proc first |
| `0x1c` | `unknown_1c[28]` | `uint8_t` |  |

#### `actor_order` // size 0x5c

| offset | field | type | note |
|---|---|---|---|
| `0x00` | `order_code` | `int16_t` | the requested order; forced to 0 when actor.swarm is set |
| `0x02` | `unknown_02` | `int16_t` | zeroed |
| `0x04` | `valid` | `uint8_t` | set to 1 by every builder that succeeds |
| `0x05` | `unknown_05` | `uint8_t` |  |
| `0x06` | `target_index` | `int16_t` | 0xffff sentinel in the default order |
| `0x08` | `parameter` | `int16_t` | the caller-supplied word |
| `0x0a` | `unknown_0a` | `uint8_t` | zeroed |
| `0x0b` | `unknown_0b[81]` | `uint8_t` |  |

#### `actor_movement_action` // size 0x18

| offset | field | type | note |
|---|---|---|---|
| `0x00` | `type` | `int16_t` | 0 stop, 2 explicit point, and the firing-position / formation / near-target kinds |
| `0x02` | `cancelled` | `uint8_t` | actor_movement_action_cancel sets it |
| `0x03` | `unknown_03` | `uint8_t` |  |
| `0x04` | `destination` | `real_point3d` |  |
| `0x10` | `parameter` | `int32_t` | object index, firing-position index or formation slot depending on type |
| `0x14` | `extra` | `uint32_t` |  |

#### `actor_iterator_state` // size 0x20

| offset | field | type | note |
|---|---|---|---|
| `0x04` | `unknown_04` | `int16_t` | zeroed |
| `0x06` | `unknown_06[2]` | `uint8_t` |  |
| `0x08` | `cursor` | `int32_t` | -1 (not yet started) |
| `0x0c` | `signature` | `uint32_t` | filter_array XOR 0x69746572 |
| `0x10` | `unknown_10` | `uint8_t` | zeroed |
| `0x11` | `active` | `uint8_t` | 1 |
| `0x12` | `unknown_12[2]` | `uint8_t` |  |
| `0x14` | `actor_index` | `datum_index` | handle of the actor the last _next returned, else none |
| `0x18` | `unknown_18` | `int32_t` | -1 |

#### `ai_reference_actor_iterator` // size 0x18

| offset | field | type | note |
|---|---|---|---|
| `0x00` | `unknown_00[0x10]` | `uint8_t` |  |
| `0x10` | `actor_index` | `datum_index` | handle of the actor the last _next returned |
| `0x14` | `unknown_14[4]` | `uint8_t` |  |

#### `ai_reference_squad_iterator` // size 0x14

| offset | field | type | note |
|---|---|---|---|
| `0x00` | `encounter_index` | `int32_t` |  |
| `0x04` | `platoon_filter` | `int32_t` |  |
| `0x08` | `cursor` | `int32_t` |  |
| `0x0c` | `squad_start` | `int32_t` |  |
| `0x10` | `squad_end` | `int32_t` |  |

#### `path_find_node` // size 0x34

| offset | field | type | note |
|---|---|---|---|
| `0x00` | `unknown_00` | `int16_t` |  |
| `0x02` | `parent` | `int16_t` | 0xffff on the start node; the reconstruction walks this chain |
| `0x04` | `unknown_04` | `int32_t` | path_find_push_start_node sets -1 |
| `0x08` | `vertex_id` | `uint32_t` | hashed as (vertex_id & 0x1ff) into the 512-bucket table |
| `0x0c` | `position` | `real_point3d` |  |
| `0x18` | `cost` | `float` | g, zero on the start node |
| `0x1c` | `unknown_1c` | `float` | path_find_push_start_node sets FLT_MAX |
| `0x20` | `unknown_20` | `float` |  |
| `0x24` | `unknown_24` | `float` |  |
| `0x28` | `distance` | `float` | the heuristic distance to the goal |
| `0x2c` | `key` | `int16_t` | the heap ordering key |
| `0x2e` | `waypoint` | `int16_t` | index into the caller waypoint array, must stay below 0x40 |
| `0x30` | `heap_index` | `int16_t` | the heap keeps this in sync as it sifts |
| `0x32` | `unknown_32[2]` | `uint8_t` |  |

#### `path_find_context` // size 0x1008c

| offset | field | type | note |
|---|---|---|---|
| `0x00` | `unknown_00[0x14]` | `uint8_t` | the first 0x48 bytes are copied wholesale from the callers request block |
| `0x14` | `start_position` | `real_point3d` | path_find_push_start_node rejects a z below -1000.0 |
| `0x20` | `start_vertex_id` | `uint32_t` | none means there is nothing to search from |
| `0x24` | `unknown_24[36]` | `uint8_t` |  |
| `0x48` | `unknown_48` | `uint32_t` | path_find_context_init stores its second argument here |
| `0x4c` | `have_goal` | `uint8_t` | the whole search and the reconstruction are gated on this |
| `0x4d` | `unknown_4d[3]` | `uint8_t` |  |
| `0x50` | `goal_position` | `real_point3d` |  |
| `0x5c` | `goal_vertex_id` | `uint32_t` |  |
| `0x60` | `goal_cost` | `float` |  |
| `0x64` | `bsp_generation` | `int32_t` | path_find_context_init copies the global at 0x00746f9c |
| `0x68` | `best_node` | `int16_t` |  |
| `0x6a` | `unknown_6a[2]` | `uint8_t` |  |
| `0x6c` | `best_cost` | `float` |  |
| `0x70` | `unknown_70` | `float` |  |
| `0x74` | `best_position` | `real_point3d` |  |
| `0x80` | `node_count` | `int16_t` | capped at 1024 by the array below |
| `0x82` | `unknown_82[2]` | `uint8_t` |  |
| `0x84` | `nodes[1024]` | `path_find_node` |  |
| `0xd084` | `heap_count` | `int16_t` | path_find_heap_push refuses past 0x400 |
| `0xd086` | `heap[1025]` | `path_find_heap_entry` | one-based, slot 0 unused |
| `0xe08a` | `vertex_hash[4096]` | `int16_t` | 512 buckets of 8 entries, probed linearly modulo 0x1000 |
| `0x1008a` | `unknown_1008a[2]` | `uint8_t` |  |

#### `ai_search_obstacle` // size 0x14

| offset | field | type | note |
|---|---|---|---|
| `0x00` | `flags` | `uint16_t` | bit 0 counts toward the flagged total |
| `0x02` | `link` | `int16_t` | the paired obstacle entry, or -1 |
| `0x04` | `object_index` | `uint32_t` |  |
| `0x08` | `position` | `real_point2d` |  |
| `0x10` | `radius` | `float` |  |

#### `ai_search_node` // size 0x28

| offset | field | type | note |
|---|---|---|---|
| `0x00` | `position` | `real_point2d` |  |
| `0x08` | `z` | `float` |  |
| `0x0c` | `direction` | `real_vector2d` | normalized delta from the node to the search origin |
| `0x14` | `length` | `float` | the length the normalize returned |
| `0x18` | `point_id` | `int16_t` | index into the obstacle list, or -1 for a free point |
| `0x1a` | `side` | `uint8_t` | which tangent side this node bends around |
| `0x1b` | `unknown_1b` | `uint8_t` |  |
| `0x1c` | `side_link` | `int16_t` | two child links, one per side; initialized to -1 |
| `0x1e` | `unknown_1e[2]` | `uint8_t` |  |
| `0x20` | `cost` | `float` | length plus the inherited cost |
| `0x24` | `parent` | `int16_t` | the node this one was expanded from |
| `0x26` | `unknown_26[2]` | `uint8_t` |  |

#### `ai_search_context` // size 0x1532

| offset | field | type | note |
|---|---|---|---|
| `0x00` | `unknown_00` | `uint32_t` |  |
| `0x04` | `unknown_04` | `uint8_t` |  |
| `0x05` | `unknown_05[3]` | `uint8_t` |  |
| `0x08` | `obstacles` | `uint32_t` | pointer to the ai_search_obstacle_list this search reads |
| `0x0c` | `unknown_0c` | `uint32_t` |  |
| `0x10` | `origin` | `real_point2d` |  |
| `0x18` | `unknown_18` | `uint32_t` |  |
| `0x1c` | `goal_point_id` | `int16_t` | taken from obstacle[goal].link, or -1 |
| `0x1e` | `result_node` | `int16_t` | -1 until a node reaches the goal |
| `0x20` | `best_node` | `int16_t` | the fallback best-effort node |
| `0x22` | `unknown_22[2]` | `uint8_t` |  |
| `0x24` | `best_cost` | `float` | FLT_MAX until best_node is set |
| `0x28` | `complete` | `uint8_t` | set when result_node is valid |
| `0x29` | `unknown_29` | `uint8_t` |  |
| `0x2a` | `unknown_2a` | `uint8_t` |  |
| `0x2b` | `unknown_2b` | `uint8_t` |  |
| `0x2c` | `node_count` | `int16_t` | 0x43b5a0 refuses past 0x80 |
| `0x2e` | `unknown_2e[2]` | `uint8_t` |  |
| `0x30` | `nodes[128]` | `ai_search_node` |  |
| `0x1430` | `heap_count` | `int16_t` | capped at 0x80 |
| `0x1432` | `heap[128]` | `int16_t` | node indices, ordered by ai_search_node.cost |
### Other types in `types/ai.h`

These are caller-owned scratch and request records rather than datum-array elements. All of
them used to be duplicated as local `typedef`s inside individual .c files; this review
folded every one of them into the header, so there is now exactly one definition per shape.

| type | size | what it is |
|---|---|---|
| `actor_target_tally` | 0x7b | the per-actor-type perception tally `actor_choose_best_target` rebuilds each pass |
| `actor_recognition_entry` | 0x4 | one slot of the 4-entry recognition ring at `actor + 0x3c8` |
| `actor_firing_position_candidate` | 0x3c | one scored firing position inside `actor_firing_position_query` |
| `actor_firing_position_danger_sphere` | 0x10 | one avoid-sphere fed to the firing-position scorer |
| `actor_firing_position_hazard` | 0x1c | one hazard the scorer penalizes |
| `actor_firing_position_query` | 0x664 | the whole firing-position query block (`actor_find_best_firing_position`) |
| `actor_firing_position_rule` | 0x8 | one row of the per-mode firing-position rule table |
| `path_find_request` | 0x48 | the request `actor_build_path_find_request` fills and `path_find_run` consumes |
| `actor_look_request` | 0x30 | the look-at request the order builders fill |
| `actor_axis_request` | 0x18 | the body-axis query at `0x405390` |
| `actor_combat_consideration` | 0x38 | `actor_consider_combat_mode`'s working record |
| `actor_vocalization_context` | 0x10 | the vocalization request block |
| `grenade_solution` | 0x18 | the grenade trajectory result |
| `ai_conversation_range_lookup` | 0x14 | one row of the conversation distance table |
| `actor_type_table_entry` | 0x20 | one row of the per-actor-type constant table |
| `actor_recognition_scan_result` | 0x08 | the `{flag, datum}` pair the recognition scan returns |
| `actor_flee_source_reason` | 0x10 | the `{code, payload}` record `actor_resolve_flee_source_point` builds |
| `ai_group_bucket_entry` | 0x1c | one bucket of `actor_scan_allies_for_backup_request` |
| `ai_target_candidate` / `ai_target_candidate_list` | 0x0c / 0x604 | the target scan's candidate array |
| `ai_grenade_avoidance_entry` | 0x28 | one candidate of the grenade-avoidance scan |
| `ai_priority_target_record` / `ai_priority_target_list` | 0x0c / 0xc04 | `ai_build_priority_target_list`'s sorted output |
| `ai_recent_event_record` | 0x14 | one slot of the 32-entry repeated-event ring at `ai_globals + 0x134` |
| `ai_reachability_scratch` | 0x18 | the trace scratch `actor_evaluate_engagement_reachability` uses |
| `ai_queued_order` / `ai_communication_order` | 0x20 | the same 0x20-byte communication record seen by two different readers |
| `ai_communication_target_result` | 0x20 | the block `ai_communication_target_result_reset` clears |
| `ai_communication_record` | 0x0c | the record `ai_communication_gate_line_played` inspects |
| `ai_communication_event_definition` | 0x24 | one row of the event table at `0x00656b08` |
| `ai_communication_line_definition` | 0x28 | one row of the larger "ai conversation" line table at `0x00655aa0`, which `ai_communication_broadcast` @`0x42d340` walks. A chain of rows shares one `event_id` and ends at the first row whose `event_id` stops matching; the starting row index comes from the int16 index table at `0x008802e0`. Several field *names* are UNSURE; the offsets are not |
| `ai_communication_candidate` | 0x38 | the per-candidate scratch record `ai_communication_broadcast` builds on its stack, one per surviving line row (up to 16), then scores and hands to four readers that each use a different overlapping field set. Offsets recovered by stack-offset arithmetic against `objdump -d 0x42d340..0x42e930`; `entry_index` is independently confirmed as `ai_communication_record_line_played`'s `communication_line_id` |
| `bool_float_return` | 5 | **not a game structure** -- the shape this module uses to model one MSVC calling-convention artifact, a helper that returns a boolean in AL and a float in ST0 at the same time. `actor_rate_potential_target` (through `0x41d800`) and `actor_target_hearing_check` (through `0x53e810`) both used to carry their own identical local copy |
| `ai_conversation_speech_request` | 0x30 | the speech request `ai_conversation_current_line_is_ready` builds (a `unit_speech` whose documented-as-padding tail it actually writes) |
| `actor_unit_position_context` | 0x2c | `actor_fill_unit_position_context`'s output |
| `actor_placement_request` | 0x1e | the spawn request `actor_place_new_unit` reads |
| `actor_perception_request` | 0x16 | the block `actor_reset_perception_scratch` zeroes |
| `actor_squad_order_header` | 0x16 | the header `actor_dispatch_squad_order` reads |
| `ai_reference_platoon_range` | 0x0c | the 3-dword output of `ai_reference_expand_to_platoon_range` |
| `ai_platoon_condition` | 0x04 | the `{code, platoon_index}` pair `encounter_evaluate_platoon_condition` takes |
| `ai_nearby_actor_candidate` | 0x0c | `ai_object_process_nearby_actors`'s sort array (matches `object_sort_by_flag_then_distance`'s stride) |
| `actor_prop_iterator` | 0x08 | the `{current, next}` cursor of the prop iterator pair |
| `actor_movement_obstacle` / `actor_movement_context` | 0x18 / 0x6048 | the local steering obstacle list |
| `path_find_heap_entry` | 0x04 | one slot of the A\* open heap |
| `path_find_adjacent_edge` | 0x20 | one output record of `path_find_gather_adjacent_edges` |
| `path_find_boundary_crossing` | 0x20 | the crossing record the BSP boundary tracers fill |
| `path_find_simplify_scratch` | 0x5c | `path_find_simplify_waypoints`'s working record |
| `ai_path_candidate_goal` | 0x5c | the goal record `path_find_validate_and_record_goal` fills |
| `ai_search_obstacle` / `ai_search_obstacle_list` | 0x14 / 0xa08 | the obstacle graph |
| `ai_search_nearest_point_result` | 0x08 | `ai_search_find_nearest_visible_point`'s output |
| `ai_search_edge_result` | 0x10 | `ai_search_evaluate_edge_cost`'s output |
| `ai_reference_squad_iterator` | 0x14 | the 5-dword state of the squad iterator pair |

No local `typedef` remains in any file in this directory. The two that used to be left out --
`fun_0053e810_result` in `actor_target_hearing_check.c` and `dual_return_result` in
`actor_rate_potential_target.c` -- were byte-for-byte identical descriptions of the same
calling-convention artifact, so the review folded them into one shared `bool_float_return`
in the header rather than keeping two names for one shape.

## 3. Known gaps

### Misattributed functions

Nine addresses inside `0x401090..0x43ecf0` are assigned to `ai` in `modules.json` but do
not belong to this module. They have no file here; `out/phase4/ai_types_notes.md` has the
full argument for each.

| address | name | size | why it is not AI |
|---|---|---|---|
| `0x4052c0` | `vector3d_cross_product` | 81 | plain math helper, belongs with `types/math.h` |
| `0x405320` | `random_int_range` | 49 | plain math helper |
| `0x405360` | `float_compare_ascending` | 45 | `qsort` comparator |
| `0x414910` | (unnamed) | 128 | a 2D cone containment test, no actor state at all |
| `0x433c70` | `object_sort_by_flag_then_distance` | 71 | `qsort` comparator |
| `0x43b2f0` | `path_find_closest_point_on_segment` | 192 | plain math helper |
| `0x43c340` / `0x43c380` / `0x43c400` | (unnamed) | 64 / 125 / 163 | plain math helpers |

Three addresses that earlier revisions of this table listed as misattributed have since been
rewritten and are **no longer** in it: `0x40b080` (`actor_process_vehicle_seat_exit`, vehicle
seat detach / reseat -- it reads far more of `unit_data` than of `actor`, but it is reached
only from the actor layer), `0x42c940` (`ai_reset_fire_group_assignments`, **zero callers in
this build**, so its layout may be stale and it is deliberately not used as offset evidence
anywhere), and `0x417a30` (`actor_movement_actions_cancel`, which was only ever
"misattributed" because of the name collision described below).

### Functions that are AI but were not rewritten

**None.** Every one of the 510 in-module addresses has a file. The two that held out longest
are worth naming, because their confidence is correspondingly low:

* **`ai_communication_broadcast` @`0x42d340`, 5603 bytes**, the module's largest function, was
  deferred by four earlier sessions because it builds a 0x38-byte candidate record on its stack
  and hands it to four different readers that each interpret a different, overlapping field set.
  Both of its records -- that candidate record and the 0x28-byte row of the static table at
  `0x00655aa0` -- are now typed in `types/ai.h` as `ai_communication_candidate` and
  `ai_communication_line_definition`. Their byte offsets come from stack-offset arithmetic
  against `objdump -d 0x42d340..0x42e930`, not from guesswork, but several field *meanings*
  remain UNSURE. Rewrite confidence 0.30.
* **`actor_movement_actions_cancel` @`0x417a30`, 37 bytes** -- trivial in itself (it sets
  `actor.queued_movement.cancelled` and `actor.active_movement.cancelled`), but it sits on a
  live name collision. `out/functions.json` marks `actor_movement_action_cancel` as Ghidra's
  own IMPORTED name for `0x417a30`, while an earlier session had already written
  `actor_movement_action_cancel.c` for the unrelated `0x428650`. The new file is named in the
  plural to avoid overwriting that work; **the collision is not resolved**, and several files
  still call the singular symbol while citing whichever of the two addresses their author
  believed in. See the open questions at the end of this section.

### The phase-4 review pass (2026-09-20)

A review pass folded the module's last local `typedef`s into `types/ai.h`, spot-checked eight
of the largest and most-UNSURE functions line by line against `python tools/pack.py 0xADDR`,
and fixed ten defects. Each is recorded in the file where it was found, together with the
Ghidra or `objdump` evidence that settles it.

| where | defect | what settles it |
|---|---|---|
| `types/ai.h` `ai_communication_candidate` | the local copy in `ai_communication_broadcast.c` put an int16 at +0x10 directly before the dword documented at +0x14 (landing it at +0x12), and the same again at +0x28 / +0x2c -- under `pack(1)` every offset past +0x10 was two bytes short | two pad bytes added; `sizeof` is now exactly 0x38 |
| `types/ai.h` `ai_communication_line_definition` | `unknown_19[3]` pushed `capability_index` from +0x1a to +0x1c and the row size from 0x28 to 0x2a | `0x42d340` references `DAT_00655aba`, i.e. row +0x1a, directly; the pad is one byte |
| `types/ai.h` `swarm` | `unknown_08[8]` hid the first float of the aggregate position, so `actor_refresh_combat_context` averaged only two of its three components | `0x4297a0` seeds all three of swarm +0x0c/+0x10/+0x14 from the vector at `0x006966f8` and sums component +0x04/+0x08/+0x0c into them; the field is now `swarm.aggregate_position` |
| `actor_refresh_combat_context.c` | the `vector2d_normalize_with_length` branch tested a vector *component* rather than the returned *length*, inverting the branch whenever that component is negative | Ghidra tests `fVar17 <= 0.0`, where `fVar17` is the call's `float10` result |
| `actor_refresh_combat_context.c` | `actor + 0x162` was computed from the tag resolved at the top of the function instead of from `actor_get_actor_definition`'s return value | `*(bool *)(iVar14 + 0x162) = 0.0 < *(float *)(iVar10 + 0x14c)` with `iVar10` = the call result |
| `actor_evaluate_custom_charge_trigger.c` | the random gate was inverted -- the actor charges when the roll comes out **below** the base chance | `if (fVar9 <= rand) false; else true` |
| `actor_evaluate_custom_charge_trigger.c` | the prop iterator was read from its first dword and initialized with no actor index | `actor_prop_iterator_init` (0x43ecd0) writes only the record's +4 dword, from `actor.first_prop`, with the actor index in EAX |
| `actor_squad_action_execute.c` | the whole `atom_type == 0x18` branch: it compared iterator results against an `Actor` tag pointer, tested distance against 0.0 instead of a running minimum, stored the winner in the object slot instead of the delay slot, and read player records as props | rewritten against the Ghidra listing; all four substitutions are documented inline |
| `ai_reset_fire_group_assignments.c` | a swarm actor with no swarm record fell into the re-link tail instead of advancing to the next actor | `if (*(uint *)(iVar15 + 0x28) == 0xffffffff) goto LAB_0042c9c6;` |
| `ai_scan_for_recent_combat_activity.c` | two distance gates used `<=` where the original is strictly `<` | the NAN-packed form is `!(d < k) && !(d == k)` |

Two further defects were **found but not fixed**, because settling them needs a full
esp-tracked trace rather than a reading of the decompiler's output. Both are in
`actor_process_vehicle_seat_exit.c` (`0x40b080`) and both are recorded in that file's header:
Ghidra drops the `fsub` pair at `0x40b278`/`0x40b293`/`0x40b2b7` (the values stored into the
rider's node array are `rider_node0.position - marker.position`, a delta, not a plain copy),
and it drops one further `fsub [esp+0x3c]` at `0x40b379` on the third component of the point
handed to `object_set_position_and_orientation`. The `fadd [ebx+0x5c|0x60|0x64]` sequence at
`0x40b353..0x40b376` does confirm that the out-point is `model_node0.position +
rider->position`, so only the subtractions are known-missing.

Two mechanical sweeps over all 510 files came back clean and are worth recording as negative
results: no file widens a narrow (`char`/`short`) Ghidra return into a 32-bit C return, and no
file drops a signed-char comparison into an unsigned one. The pointer-stride sweep produced
only false positives -- note that `sizeof` under the gate is **not** a stride check for any
struct containing a pointer, because the gate compiles 64-bit while the target is 32-bit
(`object_header` measures 0x10 under the gate and 0x0c in the binary).

### Layout questions still open

* **`object + 0xb8` is the team, not `name_index`.** `types/objects.h` calls that offset
  `name_index`; `types/ai.h` has always said "`actor.team` ... kept in sync with
  `object+0xb8`", and `encounters_note_hostile_object` @`0x435f90` feeds it into
  `team_pair_globals.enemy_bits` as `object_team + encounter_team * 10` with both operands
  range-checked against 0..9, which only makes sense for a team. `prop.object_type` (+0x12)
  is documented as a copy of the same field and is used the same way by
  `encounter_recompute_morale`. The two headers disagree and this module did not resolve it;
  `encounters_note_hostile_object.c` reads the offset raw with a comment.
* **`encounter_squad_state` starting-location masks.**
  `encounter_squad_reset_starting_location_mask` @`0x436f90` fills the *second* dword
  (`starting_location_free`, at +0x04) with 0xff via `rep stosd` from `lea edi,[esi+0x4]`,
  but sets the per-location bits in `mask[index >> 5]`, i.e. the *first* dword (+0x00) for
  the first 32 locations. `squad_pick_random_starting_location` @`0x437220` then reads +0x00
  as "allowed" and +0x04 as "still free", which makes the pair coherent -- but the fill
  length is `((count + 31) >> 5)` dwords starting at +0x04, so a squad with more than 32
  starting locations would spill past +0x07. Verified against the disassembly; flagged for
  hook verification.
* **`ai_globals + 0x3b8`** holds 32 records of 0x28 bytes (`ai_object_attention_record`),
  which runs exactly to `ai_globals.vehicle_entry_count` at +0x8b8. But the communication
  code reads `ai_globals.unknown_3f0` and `unknown_3fa`, which fall inside record 1.
  `ai_globals` was therefore left laid out as before and the table is addressed by casting
  `ai_globals.unknown_3b8`.
* **`actor + 0x9c` is a 0x84-byte union (`actor.mode_data`)**, so several functions read
  `actor + 0xa4` / `+0xa8` / `+0xac` as typed fields of whatever mode is current. Those
  accesses are written as `*(int16_t *)(a->mode_data + N)` with the absolute offset in a
  comment. Which mode owns which slot is not established.
* **`ScenarioStructureBSP.clusters.count` at +0x134** is read raw; `types/tags.h` carries no
  offset comments for that struct.

### Extern prototype disagreements

`extern` declarations are now name-consistent across the module: every callee that has a
file in this directory is referred to by that file's name, and the foreign callees that the
wider repo has already named (`teams_are_enemies`, `vector3d_magnitude_squared`,
`object_get_root_object_velocities`, `datum_get`, ...) use those names here too.

What is **not** reconciled is the *signature* of the callee addresses that are declared with
different parameter lists in different files. Measured across the whole tree as of this review:
305 `extern` declarations inside `src/ai` disagree on arity with the rewritten definition of
the same function. 145 of those declare `(void)` where the definition takes arguments -- that
is the honest case, a call site that could not see register-passed operands. The other 160
disagree with a *specific* count on both sides and are the ones worth re-deriving; the most
repeated are `actor_find_prop_for_object` (13 call sites declare 1 argument, the definition
takes 2), `actor_set_units_active` (12 sites, 1 vs 2 -- the missing second argument is `BL`),
`actor_find_or_create_shared_prop` (8 sites, 3 vs 4), `actor_replace_object_reference`
(6 sites, 1 vs 3) and `actor_evaluate_engagement_reachability` (5 sites, 4 vs 8). In almost every case that is Ghidra showing a
different number of operands at different call sites for the same register-heavy function,
and each file honestly declares what its own call site shows. Collapsing them would mean
inventing the missing operands. Two addresses are declared under deliberately suffixed
aliases for exactly this reason:

* `0x43d790` `path_find_trace_cluster_boundary_from_vertex_3` / `_6` in
  `ai_search_evaluate_edge_cost.c` -- three operands at some call sites, six at others;
* `0x502060` `FUN_00502060` / `FUN_00502060_query` -- four operands vs six.

These are the single biggest source of remaining risk in the module and they resolve the
moment the functions concerned are read from disassembly.

### Lowest-confidence rewrites

62 files carry a rewrite confidence of 0.20 or below. The ten worst are structural
translations only and should be redone from a disassembly before anyone trusts them:

| address | file | rewrite conf | why |
|---|---|---|---|
| `0x43d240` | `ai_search_choose_shorter_corner` | 0.05 | all four `vector2d_angle_between` calls show zero operands |
| `0x43de90` | `path_find_test_segment_unobstructed` | 0.05 | nine calls to `path_find_trace_bsp_boundary`, none with visible arguments |
| `0x43b830` | `ai_search_evaluate_edge_cost` | 0.10 | mutually inconsistent argument counts across call sites |
| `0x43ba60` | `ai_search_expand_point_neighbors` | 0.10 | same |
| `0x43be90` | `ai_navigate_around_obstacles` | 0.10 | same |
| `0x43c510` | `ai_search_gather_obstacles` | 0.10 | same |
| `0x43cc00` | `path_find_simplify_waypoints` | 0.10 | same |
| `0x43d4b0` | `path_find_trace_cluster_boundary` | 0.10 | same |
| `0x43d790` | `path_find_trace_cluster_boundary_from_vertex` | 0.10 | same |
| `0x43d9b0` | `path_find_trace_bsp_boundary` | 0.10 | same |
| `0x41e320` | `actor_target_evaluate_squad_link` | 0.10 | register-passed operands lost |

There are 560 `UNSURE:` markers across the 510 files; mean rewrite confidence is 0.42,
median 0.40, and 62 files sit at 0.20 or below.

### The five open questions that most need hook verification

In priority order. Each is a question a few minutes of running the game with a hook would
settle, and each currently blocks a class of files rather than one function.

1. **Is `actor_movement_action_cancel` `0x417a30` or `0x428650`?** `out/functions.json` marks
   `0x417a30` IMPORTED (Ghidra's own name) and `0x428650` DEFAULT, yet
   `actor_movement_action_cancel.c` in this directory documents `0x428650`, and its callers are
   split: `actor_consider_combat_mode.c` and `actor_squad_action_execute.c` cite `0x417a30`,
   while `actor_reset_squad_link_for_type_change.c`, `ai_actor_link_to_unassigned_list.c` and
   `ai_reference_detach_actors_from_encounters.c` cite `0x428650` for the same symbol name.
   The two functions do different things (`0x417a30` sets two cancel bytes; `0x428650` clears
   the firing position and dispatches a per-mode proc), so one group of call sites is calling
   the wrong function. Breakpoint both and see which one an order cancellation hits.
2. **`actor + 0x270`: `target_unit_index` or a prop index?** `types/ai.h` documents it as a
   unit handle on the authority of `actor_choose_best_target`, which is live code. But
   `ai_reset_fire_group_assignments` (`0x42c940`) indexes `prop_data` with it at stride 0x138
   and then reads `prop.kind` / `pair_index` / `is_parented` / `distance` off the result, and
   `actor_evaluate_custom_charge_trigger` (`0x424090`) does the same. `0x42c940` is dead code
   in this build, so it may carry a stale layout -- but `0x424090` is not. Around 20 files
   depend on the answer. Log the value and see which array it indexes.
3. **`actor_set_units_active`'s second argument, and the register-argument gap generally.**
   12 call sites declare one argument where the definition takes two; the missing operand is
   `BL`. That single function is the most-repeated instance of the 160 arity disagreements
   described above. A hook that records EAX/EBX/ECX/EDX at entry for the two dozen most-called
   register-heavy callees would close most of that class at once.
4. **`object + 0xb8`: team or `name_index`?** `types/objects.h` says `name_index`;
   `types/ai.h` and `encounters_note_hostile_object` (`0x435f90`) both treat it as a team,
   range-checked 0..9 and folded into `team_pair_globals.enemy_bits` as
   `object_team + encounter_team * 10`. The same disagreement reaches `prop.object_type`
   (+0x12) and every relationship test in the module. Read the field for a known object.
5. **`actor_process_vehicle_seat_exit` (`0x40b080`) -- the two `fsub`s Ghidra drops.**
   `objdump` shows `fld [edi+0x28|0x2c|0x30]` followed by `fsub [esp+0xf0|0xf4|0xf8]` at
   `0x40b275..0x40b2c6`, and one further `fsub [esp+0x3c]` at `0x40b379`, none of which the
   decompiler (or therefore this rewrite) reproduces. This function has zero callers in this
   build, so a hook cannot reach it from normal play: settling it means an esp-tracked manual
   trace, not instrumentation. It is listed here because it is the only known *arithmetic*
   defect left in the module, as opposed to a naming or signature one.

## 4. Function list

Addresses are the retail `halo.exe` 1.0.10 entry points. "name conf" is how sure the
*name* is, "rewrite conf" how sure the *translation* is; both are copied from each file's
own header. "UNSURE" is the count of `UNSURE:` markers in that file's header and body
(excluding the `#if 0` Ghidra dump).

### Actor order builders and combat considerations  (52 functions, 26664 bytes)

| address | function | size | name conf | rewrite conf | UNSURE |
|---|---|---|---|---|---|
| `0x401090` | `actor_build_order_default` | 80 | 0.40 | 0.60 |  |
| `0x4014c0` | `actor_select_move_position` | 744 | 0.40 | 0.35 | 3 |
| `0x4017b0` | `actor_update_path_if_needed` | 156 | 0.40 | 0.30 | 3 |
| `0x401a60` | `actor_consider_combat_mode` | 737 | 0.50 | 0.30 | 3 |
| `0x4028e0` | `actor_get_consideration_wait_threshold` | 243 | 0.45 | 0.45 |  |
| `0x4029e0` | `actor_grenade_trace_from_source` | 184 | 0.40 | 0.30 | 3 |
| `0x402cf0` | `ai_conversation_get_run_to_player_range` | 128 | 0.35 | 0.40 | 1 |
| `0x402f80` | `actor_schedule_grenade_throw` | 499 | 0.50 | 0.30 | 7 |
| `0x403180` | `actor_update_movement_destination` | 946 | 0.40 | 0.20 | 4 |
| `0x403630` | `actor_build_order_grenade_or_melee` | 264 | 0.40 | 0.30 | 1 |
| `0x403dc0` | `actor_is_target_within_engagement_range` | 305 | 0.40 | 0.30 | 1 |
| `0x403f00` | `actor_check_melee_target_reachable` | 717 | 0.40 | 0.20 | 5 |
| `0x4041d0` | `actor_check_weapon_pickup_reachable` | 296 | 0.40 | 0.30 |  |
| `0x404340` | `actor_order_code_is_grenade_throw` | 21 | 0.60 | 0.90 |  |
| `0x4044b0` | `actor_build_order_return_to_anchor` | 82 | 0.40 | 0.35 | 1 |
| `0x404510` | `actor_build_order_guard` | 143 | 0.40 | 0.35 | 1 |
| `0x4045a0` | `actor_build_order_search_wait` | 260 | 0.40 | 0.30 | 3 |
| `0x4046c0` | `actor_build_order_look` | 345 | 0.40 | 0.30 |  |
| `0x4048b0` | `actor_report_command_status` | 243 | 0.40 | 0.30 | 1 |
| `0x4049d0` | `actor_request_move_and_face` | 439 | 0.40 | 0.25 | 4 |
| `0x405390` | `actor_get_body_axis_vector` | 376 | 0.40 | 0.30 | 2 |
| `0x405520` | `actor_squad_action_execute` | 4338 | 0.50 | 0.20 | 10 |
| `0x4066d0` | `actor_squad_action_is_complete` | 1318 | 0.50 | 0.20 | 3 |
| `0x406c50` | `actor_squad_action_reset_entry` | 312 | 0.40 | 0.25 |  |
| `0x406e30` | `actor_squad_action_list_process` | 246 | 0.50 | 0.40 |  |
| `0x407040` | `actor_swarm_for_each_component` | 244 | 0.35 | 0.30 | 2 |
| `0x407140` | `actor_squad_action_status_broadcast` | 249 | 0.40 | 0.25 | 4 |
| `0x407240` | `actor_swarm_for_each_component_thunk` | 51 | 0.35 | 0.50 |  |
| `0x4077d0` | `actor_build_order_flee` | 77 | 0.40 | 0.40 |  |
| `0x407820` | `actor_build_order_face_seat_marker_committed` | 193 | 0.40 | 0.35 |  |
| `0x4078f0` | `actor_build_order_minimal_stop` | 68 | 0.35 | 0.40 |  |
| `0x4080c0` | `actor_build_order_wait_byte` | 75 | 0.35 | 0.40 |  |
| `0x408110` | `actor_build_order_face_seat_marker` | 193 | 0.40 | 0.35 |  |
| `0x408300` | `actor_request_path_with_grenade_arc` | 358 | 0.40 | 0.20 | 2 |
| `0x408920` | `actor_build_order_search_object` | 267 | 0.50 | 0.30 | 1 |
| `0x408a30` | `actor_build_order_investigate_encounter_point` | 287 | 0.50 | 0.35 |  |
| `0x408ba0` | `actor_investigate_disturbance_update` | 732 | 0.40 | 0.20 |  |
| `0x408f30` | `actor_is_within_alert_range` | 319 | 0.50 | 0.35 | 1 |
| `0x409070` | `actor_find_best_search_node` | 342 | 0.40 | 0.20 | 2 |
| `0x4091d0` | `actor_evaluate_search_node` | 1003 | 0.40 | 0.15 | 5 |
| `0x4095c0` | `actor_avoid_obstacle_and_project` | 1219 | 0.40 | 0.15 | 3 |
| `0x409a90` | `actor_build_order_random_wait` | 153 | 0.40 | 0.40 |  |
| `0x409e70` | `actor_invoke_type_handler` | 44 | 0.50 | 0.50 | 1 |
| `0x409ea0` | `actor_process_order_request` | 621 | 0.50 | 0.20 | 7 |
| `0x40a700` | `actor_gate_jump_traversal` | 235 | 0.40 | 0.35 | 1 |
| `0x40ab80` | `actor_wants_reload_or_swap` | 161 | 0.40 | 0.35 | 1 |
| `0x40b080` | `actor_process_vehicle_seat_exit` | 1767 | 0.35 | 0.35 | 10 |
| `0x40b840` | `actor_should_throw_grenade` | 224 | 0.50 | 0.35 | 1 |
| `0x40b920` | `actor_update_grenade_and_morale_reactions` | 800 | 0.40 | 0.20 | 4 |
| `0x40c620` | `actor_evaluate_combat_state_transition` | 1443 | 0.50 | 0.15 | 6 |
| `0x40cc70` | `actor_get_target_state_flags` | 375 | 0.50 | 0.35 |  |
| `0x40cdf0` | `actor_update_melee_combat_action` | 1742 | 0.35 | 0.30 | 8 |

### Actor modes, aiming, grenades and movement  (100 functions, 55171 bytes)

| address | function | size | name conf | rewrite conf | UNSURE |
|---|---|---|---|---|---|
| `0x40d4c0` | `actor_flee_look_away` | 91 | 0.40 | 0.40 | 3 |
| `0x40d520` | `actor_combat_status_should_hold` | 93 | 0.40 | 0.40 | 1 |
| `0x40d580` | `actor_command_list_permits_escalation` | 132 | 0.40 | 0.45 |  |
| `0x40d610` | `actor_update_combat_behavior` | 374 | 0.45 | 0.35 | 1 |
| `0x40d7a0` | `actor_conditional_state_transition_check` | 114 | 0.45 | 0.40 | 1 |
| `0x40d820` | `actor_update_special_mode` | 162 | 0.30 | 0.35 | 4 |
| `0x40d8d0` | `actor_set_mode` | 236 | 0.50 | 0.70 |  |
| `0x40d9c0` | `actor_can_throw_grenade_at_target` | 318 | 0.40 | 0.30 | 3 |
| `0x40db00` | `actor_check_grenade_facing_and_commit` | 300 | 0.40 | 0.30 | 3 |
| `0x40dc30` | `actor_consider_grenade_throw` | 287 | 0.40 | 0.35 | 2 |
| `0x40dd50` | `actor_handle_death` | 198 | 0.30 | 0.25 | 2 |
| `0x40de20` | `actor_check_pain_reaction` | 75 | 0.30 | 0.30 | 3 |
| `0x40de70` | `actor_evaluate_grenade_target_position` | 485 | 0.35 | 0.25 | 6 |
| `0x40e260` | `actor_play_first_valid_vocalization` | 275 | 0.45 | 0.60 | 2 |
| `0x40e380` | `actor_targets_share_descriptor` | 273 | 0.40 | 0.25 | 5 |
| `0x40e4a0` | `actor_validate_grenade_ally_candidate` | 159 | 0.40 | 0.30 | 1 |
| `0x40e540` | `actor_find_nearest_grenade_ally` | 530 | 0.35 | 0.25 | 4 |
| `0x40e760` | `actor_get_current_mode_combat_grade` | 36 | 0.40 | 0.60 |  |
| `0x40e790` | `actor_lookup_small_table_entry` | 25 | 0.30 | 0.50 | 2 |
| `0x40e7b0` | `actor_update_firing_state` | 3752 | 0.50 | 0.30 | 1 |
| `0x40f670` | `actor_grenade_behavior_kind_allowed` | 139 | 0.35 | 0.45 | 1 |
| `0x40f700` | `actor_target_is_visible_or_object_count_ok` | 145 | 0.35 | 0.40 |  |
| `0x40f7e0` | `actor_compute_grenade_aim_direction` | 391 | 0.35 | 0.20 | 2 |
| `0x40f970` | `actor_get_threat_weapon_definition` | 61 | 0.40 | 0.50 | 1 |
| `0x40f9b0` | `actor_get_aim_from_position` | 177 | 0.40 | 0.25 | 3 |
| `0x40fa70` | `actor_get_actor_definition` | 127 | 0.50 | 0.50 | 4 |
| `0x40faf0` | `actor_choose_random_point_near` | 436 | 0.45 | 0.20 | 3 |
| `0x40fcb0` | `actor_update_aim_wander` | 2092 | 0.45 | 0.20 | 5 |
| `0x4104e0` | `actor_reseed_movement_pause_timer` | 213 | 0.40 | 0.15 | 3 |
| `0x4105c0` | `actor_should_hold_position` | 237 | 0.35 | 0.20 | 2 |
| `0x4106b0` | `actor_select_stance_offset_pair` | 95 | 0.30 | 0.40 |  |
| `0x410710` | `actor_validate_grenade_impact_point` | 112 | 0.35 | 0.40 |  |
| `0x410780` | `actor_solve_grenade_lob` | 499 | 0.50 | 0.45 | 1 |
| `0x410980` | `actor_get_grenade_launch_velocity` | 221 | 0.40 | 0.45 | 1 |
| `0x410a60` | `actor_compute_grenade_throw_vector` | 560 | 0.50 | 0.35 | 3 |
| `0x410c90` | `actor_find_grenade_landing_spot` | 263 | 0.40 | 0.25 | 1 |
| `0x410da0` | `actor_score_blast_area_clear` | 979 | 0.35 | 0.20 | 2 |
| `0x411180` | `actor_commit_grenade_toss` | 295 | 0.40 | 0.15 | 1 |
| `0x4112b0` | `actor_score_firing_positions_by_threat` | 1405 | 0.50 | 0.45 | 2 |
| `0x411bf0` | `actor_score_firing_positions_by_range` | 737 | 0.60 | 0.55 | 1 |
| `0x411ee0` | `actor_score_firing_positions_by_history` | 508 | 0.45 | 0.60 | 1 |
| `0x4120f0` | `actor_report_firing_position_request` | 415 | 0.50 | 0.55 |  |
| `0x412290` | `actor_reject_firing_position_unreachable` | 184 | 0.50 | 0.70 | 1 |
| `0x412350` | `actor_reject_firing_position_by_pursuit` | 365 | 0.50 | 0.65 | 2 |
| `0x412620` | `actor_reject_firing_position_by_request_result` | 198 | 0.50 | 0.80 |  |
| `0x4126f0` | `actor_firing_position_run_score_rules` | 64 | 0.60 | 0.80 |  |
| `0x412730` | `actor_firing_position_run_reject_rules` | 62 | 0.60 | 0.80 |  |
| `0x412770` | `actor_firing_position_probe_reject_rules` | 62 | 0.60 | 0.75 |  |
| `0x412820` | `actor_firing_position_evaluate` | 82 | 0.60 | 0.80 |  |
| `0x412880` | `actor_get_firing_position_group_mask` | 224 | 0.70 | 0.85 | 1 |
| `0x412960` | `actor_firing_position_near_point` | 554 | 0.55 | 0.60 | 1 |
| `0x412ba0` | `actor_find_best_firing_position` | 4754 | 0.55 | 0.35 | 4 |
| `0x413e50` | `actor_select_firing_position` | 516 | 0.55 | 0.50 | 3 |
| `0x414060` | `actor_claim_firing_position` | 214 | 0.55 | 0.70 | 2 |
| `0x414140` | `actor_clear_recognition_history` | 95 | 0.60 | 0.85 |  |
| `0x4141a0` | `actor_push_recognition_entry` | 173 | 0.60 | 0.80 |  |
| `0x414250` | `actor_update_flee_response` | 113 | 0.45 | 0.60 | 2 |
| `0x4142d0` | `actor_begin_vocalization` | 647 | 0.55 | 0.55 | 3 |
| `0x414560` | `actor_clear_vocalization` | 46 | 0.50 | 0.90 |  |
| `0x414590` | `actor_compute_target_priority_weight` | 301 | 0.40 | 0.40 | 2 |
| `0x4146c0` | `actor_resolve_flee_source_point` | 555 | 0.40 | 0.55 | 4 |
| `0x414990` | `actor_point_in_directional_lane` | 255 | 0.35 | 0.40 | 3 |
| `0x414a90` | `actor_select_facing_target_prop` | 613 | 0.35 | 0.35 | 2 |
| `0x414d00` | `actor_resolve_look_target` | 580 | 0.35 | 0.35 | 3 |
| `0x414f50` | `actor_look_randomize_direction` | 401 | 0.35 | 0.35 | 1 |
| `0x4150f0` | `actor_get_idle_facing_range` | 84 | 0.40 | 0.55 | 1 |
| `0x415150` | `actor_look_get_wait_ticks` | 267 | 0.50 | 0.50 | 3 |
| `0x415260` | `actor_look_pick_random_point_in_cone` | 540 | 0.55 | 0.40 | 4 |
| `0x415480` | `actor_update_look_target` | 3896 | 0.30 | 0.25 | 6 |
| `0x4163e0` | `actor_movement_advance_waypoint` | 796 | 0.30 | 0.30 | 1 |
| `0x416700` | `actor_movement_check_arrival` | 138 | 0.35 | 0.50 |  |
| `0x416790` | `actor_movement_update` | 3074 | 0.50 | 0.75 | 3 |
| `0x4173a0` | `actor_movement_get_stopping_distances` | 460 | 0.45 | 0.70 | 3 |
| `0x417570` | `actor_movement_action_stop` | 152 | 0.55 | 0.50 | 1 |
| `0x417610` | `actor_movement_set_destination_point` | 308 | 0.60 | 0.50 | 1 |
| `0x417750` | `actor_movement_set_destination_move_position` | 212 | 0.55 | 0.60 |  |
| `0x417830` | `actor_movement_set_destination_firing_position` | 220 | 0.55 | 0.60 | 1 |
| `0x417910` | `actor_movement_set_destination_near_target` | 284 | 0.50 | 0.55 | 1 |
| `0x417a30` | `actor_movement_actions_cancel` | 37 | 0.55 | 0.85 |  |
| `0x417a60` | `actor_queue_secondary_action` | 127 | 0.40 | 0.45 | 1 |
| `0x417ae0` | `actor_reset_queued_look_vector` | 139 | 0.35 | 0.45 | 1 |
| `0x417b70` | `actor_action_has_queued_secondary` | 57 | 0.50 | 0.60 |  |
| `0x417bb0` | `actor_check_step_obstruction` | 661 | 0.25 | 0.25 | 7 |
| `0x417e50` | `actor_probe_step_direction` | 304 | 0.30 | 0.40 |  |
| `0x417fa0` | `actor_get_requested_velocity` | 275 | 0.40 | 0.80 | 2 |
| `0x4180c0` | `actor_movement_apply_steering` | 2239 | 0.45 | 0.30 | 8 |
| `0x418a40` | `actor_movement_choose_strafe_axis` | 477 | 0.40 | 0.70 | 2 |
| `0x418c20` | `actor_movement_project_into_frame` | 189 | 0.45 | 0.85 | 1 |
| `0x418ce0` | `actor_movement_collect_obstacle_candidates` | 656 | 0.60 | 0.65 | 3 |
| `0x418f70` | `actor_movement_test_obstacle_ray` | 710 | 0.60 | 0.60 | 1 |
| `0x419240` | `actor_avoidance_interpolate_sample` | 396 | 0.40 | 0.75 | 1 |
| `0x4193d0` | `actor_movement_choose_avoidance_direction` | 3829 | 0.55 | 0.20 | 7 |
| `0x41a2d0` | `actor_avoidance_build_direction_tables` | 325 | 0.55 | 0.75 | 2 |
| `0x41a430` | `actor_movement_action_complete` | 44 | 0.60 | 0.70 |  |
| `0x41a460` | `actor_movement_action_resolve` | 1253 | 0.60 | 0.75 | 2 |
| `0x41a960` | `actor_movement_action_is_complete` | 28 | 0.60 | 0.70 |  |
| `0x41a980` | `actor_movement_action_in_progress` | 52 | 0.55 | 0.70 |  |
| `0x41a9c0` | `actor_build_path_find_request` | 236 | 0.60 | 0.80 | 1 |
| `0x41aab0` | `actor_movement_flying_needs_steering` | 275 | 0.45 | 0.80 | 2 |
| `0x41abd0` | `actor_target_relationship_think` | 3351 | 0.45 | 0.35 | 18 |

### Actor perception, targets, props and danger  (67 functions, 33608 bytes)

| address | function | size | name conf | rewrite conf | UNSURE |
|---|---|---|---|---|---|
| `0x41b980` | `actor_target_data_release` | 355 | 0.45 | 0.30 | 3 |
| `0x41baf0` | `actor_target_reset_combat_flags` | 55 | 0.50 | 0.60 | 2 |
| `0x41bb30` | `actor_dispatch_look_handler_by_posture` | 133 | 0.30 | 0.30 | 3 |
| `0x41be10` | `actor_target_get_priority_class` | 189 | 0.45 | 0.50 | 1 |
| `0x41bed0` | `unit_get_move_speed_for_range` | 348 | 0.45 | 0.30 | 2 |
| `0x41c030` | `actor_target_hearing_check` | 429 | 0.35 | 0.20 | 4 |
| `0x41c1e0` | `actor_get_firing_positions` | 213 | 0.55 | 0.40 | 3 |
| `0x41c2c0` | `object_find_nearest_squad_member` | 492 | 0.45 | 0.30 | 1 |
| `0x41c4b0` | `actor_target_data_refresh` | 1075 | 0.40 | 0.15 | 8 |
| `0x41c8f0` | `actor_target_update_tracking_speed` | 3753 | 0.50 | 0.45 | 11 |
| `0x41d7a0` | `ai_target_distance_qsort_compare` | 57 | 0.60 | 0.70 |  |
| `0x41d7e0` | `actor_target_scan_potential_targets` | 2847 | 0.50 | 0.30 | 10 |
| `0x41e320` | `actor_target_evaluate_squad_link` | 1849 | 0.40 | 0.10 | 8 |
| `0x41ea60` | `actor_danger_register_stationary_object` | 557 | 0.45 | 0.20 | 5 |
| `0x41ec90` | `actor_danger_register_point` | 259 | 0.45 | 0.35 | 2 |
| `0x41eda0` | `actor_danger_update_reaction` | 1533 | 0.45 | 0.60 | 4 |
| `0x41f3a0` | `actor_target_get_relationship_object` | 112 | 0.50 | 0.40 | 2 |
| `0x41f410` | `actor_target_has_conflicting_neighbor` | 311 | 0.40 | 0.40 | 1 |
| `0x41f550` | `actor_get_relevant_squad_member_target` | 345 | 0.40 | 0.25 | 3 |
| `0x41f6b0` | `actor_get_squad_recent_attacker_target` | 274 | 0.40 | 0.20 | 2 |
| `0x41f7d0` | `actor_target_data_acquire` | 512 | 0.45 | 0.20 | 8 |
| `0x41f9d0` | `actor_target_reset_seen_flags` | 71 | 0.55 | 0.65 |  |
| `0x41fa20` | `actor_target_reset_shot_counters` | 84 | 0.55 | 0.65 |  |
| `0x41fa80` | `actor_target_mark_engaged` | 120 | 0.45 | 0.35 | 3 |
| `0x41fb00` | `actor_set_target_alert_stage1` | 85 | 0.30 | 0.40 | 4 |
| `0x41fb60` | `actor_set_target_alert_stage2` | 85 | 0.30 | 0.40 | 2 |
| `0x41fbc0` | `actor_set_target_alert_stage3` | 158 | 0.30 | 0.40 | 2 |
| `0x41fc60` | `actor_target_update_active_flag` | 240 | 0.35 | 0.35 | 1 |
| `0x41fd50` | `actor_rate_potential_target` | 888 | 0.60 | 0.25 | 7 |
| `0x4200d0` | `actor_update_target_combat_status` | 415 | 0.55 | 0.45 | 1 |
| `0x420290` | `actor_update_awareness_level` | 258 | 0.50 | 0.45 |  |
| `0x4203a0` | `actor_choose_best_target` | 1266 | 0.60 | 0.60 | 4 |
| `0x4208a0` | `actor_consider_target_candidate` | 196 | 0.55 | 0.50 | 3 |
| `0x420970` | `actor_get_ranged_attack_vector` | 404 | 0.40 | 0.30 | 3 |
| `0x420b10` | `actor_evaluate_flank_offset` | 376 | 0.35 | 0.35 | 2 |
| `0x420c90` | `actor_scale_value_by_ally_exposure` | 322 | 0.40 | 0.35 | 2 |
| `0x420de0` | `ai_group_bucket_find_or_add` | 103 | 0.30 | 0.30 | 1 |
| `0x420e50` | `actor_target_get_backup_priority` | 106 | 0.45 | 0.40 | 1 |
| `0x420ec0` | `actor_scan_allies_for_backup_request` | 1256 | 0.50 | 0.45 | 5 |
| `0x4213b0` | `actor_update_crouch_state` | 1653 | 0.50 | 0.60 | 2 |
| `0x421a40` | `actor_set_combat_alert_flag` | 163 | 0.50 | 0.45 | 2 |
| `0x421af0` | `actor_queue_search_position` | 199 | 0.40 | 0.40 |  |
| `0x421bc0` | `actor_record_look_at_point` | 88 | 0.60 | 0.60 | 1 |
| `0x421c20` | `actor_queue_sighted_target_dialogue` | 1104 | 0.40 | 0.40 | 4 |
| `0x422070` | `actor_record_perception_event` | 72 | 0.60 | 0.65 |  |
| `0x4220c0` | `actor_notify_target_engaged` | 107 | 0.55 | 0.55 |  |
| `0x422130` | `actor_start_search_timer` | 115 | 0.40 | 0.55 | 1 |
| `0x4221b0` | `actor_queue_velocity_search_from_prop` | 63 | 0.35 | 0.50 | 1 |
| `0x4221f0` | `actor_queue_search_and_relay_perception` | 121 | 0.35 | 0.50 |  |
| `0x422270` | `actor_queue_directional_reaction_event` | 725 | 0.40 | 0.50 | 1 |
| `0x422550` | `actor_queue_recognized_target_dialogue` | 556 | 0.40 | 0.50 |  |
| `0x422780` | `actor_queue_point_reaction_dialogue` | 432 | 0.40 | 0.50 |  |
| `0x422930` | `actor_react_to_registered_danger` | 707 | 0.35 | 0.40 |  |
| `0x422c00` | `actor_react_to_flee_point` | 696 | 0.35 | 0.40 | 3 |
| `0x422ec0` | `actor_react_to_seen_target` | 858 | 0.35 | 0.40 | 5 |
| `0x423220` | `actor_scan_backup_and_panic_reaction` | 423 | 0.35 | 0.35 | 1 |
| `0x4233d0` | `actor_scan_ally_death_panic_reaction` | 275 | 0.35 | 0.35 | 2 |
| `0x4234f0` | `actor_notify_squad_of_threat_direction` | 269 | 0.35 | 0.40 |  |
| `0x423600` | `actor_notify_squad_and_flag_danger` | 99 | 0.40 | 0.55 |  |
| `0x423670` | `actor_update_facing_change_timer` | 208 | 0.30 | 0.40 | 2 |
| `0x424090` | `actor_evaluate_custom_charge_trigger` | 1267 | 0.40 | 0.15 | 23 |
| `0x424aa0` | `actor_pick_dialogue_variant_a` | 212 | 0.35 | 0.35 | 1 |
| `0x424b80` | `actor_pick_dialogue_variant_b` | 160 | 0.35 | 0.35 |  |
| `0x425c70` | `actor_compute_swarm_avoidance_offset` | 755 | 0.35 | 0.25 | 9 |
| `0x426670` | `actor_dispatch_type_vtable_0x10` | 46 | 0.55 | 0.45 | 1 |
| `0x4266a0` | `actor_dispatch_type_vtable_0x18` | 41 | 0.55 | 0.45 | 1 |
| `0x4266d0` | `actor_dispatch_type_vtable_0x1c` | 63 | 0.55 | 0.45 | 1 |

### Actor lifecycle, swarms and unit binding  (82 functions, 22367 bytes)

| address | function | size | name conf | rewrite conf | UNSURE |
|---|---|---|---|---|---|
| `0x426710` | `actors_initialize` | 78 | 0.90 | 0.85 |  |
| `0x426760` | `actor_new` | 860 | 0.75 | 0.55 | 1 |
| `0x426ac0` | `actor_new_and_attach_to_unit` | 553 | 0.55 | 0.30 | 11 |
| `0x426cf0` | `actor_apply_unit_definition_properties` | 901 | 0.40 | 0.15 | 18 |
| `0x427080` | `actor_place_new_unit` | 500 | 0.55 | 0.15 | 8 |
| `0x427280` | `actor_spawn_additional_units` | 716 | 0.45 | 0.15 | 9 |
| `0x427560` | `actor_attach_to_unit` | 382 | 0.55 | 0.35 | 7 |
| `0x4276e0` | `actor_propagate_unit_field` | 214 | 0.45 | 0.40 | 2 |
| `0x4277c0` | `actor_toggle_active_state` | 155 | 0.45 | 0.40 | 3 |
| `0x427860` | `actor_set_units_active` | 305 | 0.50 | 0.40 | 6 |
| `0x4279a0` | `swarm_add_component` | 78 | 0.50 | 0.55 |  |
| `0x4279f0` | `actor_link_to_unit_cluster` | 457 | 0.45 | 0.35 | 7 |
| `0x427bc0` | `actor_unlink_unit` | 197 | 0.50 | 0.45 | 2 |
| `0x427c90` | `actor_remove_from_unit_cluster` | 360 | 0.50 | 0.45 | 4 |
| `0x427e00` | `actor_clear_perceived_props` | 89 | 0.45 | 0.40 | 3 |
| `0x427e60` | `actor_delete` | 216 | 0.60 | 0.40 | 3 |
| `0x427f40` | `actor_create_swarm` | 355 | 0.50 | 0.40 |  |
| `0x4280b0` | `actor_delete_swarm` | 121 | 0.55 | 0.55 | 1 |
| `0x428130` | `actor_update_swarm_component_position` | 80 | 0.40 | 0.50 | 1 |
| `0x428180` | `actor_is_burst_pending` | 45 | 0.35 | 0.40 |  |
| `0x4281b0` | `actor_check_burst_length_exceeded` | 56 | 0.30 | 0.40 |  |
| `0x4281f0` | `actor_get_cached_wander_position` | 126 | 0.35 | 0.40 | 2 |
| `0x428270` | `actor_check_vehicle_mode_timeout` | 76 | 0.35 | 0.40 |  |
| `0x4282c0` | `actor_get_threat_weapon_object_index` | 176 | 0.50 | 0.40 | 5 |
| `0x428370` | `actor_has_unshielded_threat_weapon` | 84 | 0.40 | 0.50 |  |
| `0x4283d0` | `actor_get_target_prop_object_index` | 66 | 0.30 | 0.40 | 1 |
| `0x428420` | `actor_forward_target_object_reference` | 75 | 0.30 | 0.40 | 2 |
| `0x428470` | `actor_replace_object_reference` | 469 | 0.55 | 0.30 | 9 |
| `0x428650` | `actor_movement_action_cancel` | 101 | 0.40 | 0.30 | 1 |
| `0x4286c0` | `actor_clear_target_state` | 212 | 0.40 | 0.30 | 3 |
| `0x4287a0` | `actor_resolve_wander_or_look_direction` | 155 | 0.40 | 0.45 |  |
| `0x428840` | `actor_mark_prop_seen_with_delta` | 153 | 0.35 | 0.20 | 4 |
| `0x4288e0` | `actor_delete_or_release_unit` | 215 | 0.50 | 0.25 | 5 |
| `0x4289c0` | `actor_mark_units_and_release` | 235 | 0.35 | 0.30 | 2 |
| `0x428ab0` | `actor_attempt_grenade_throw` | 645 | 0.55 | 0.20 | 16 |
| `0x428d35` | `actor_died_unit_grenade_count_mod` | 281 | 0.60 | 0.20 | 8 |
| `0x428e50` | `actor_release_from_cluster_or_delete` | 80 | 0.40 | 0.40 | 2 |
| `0x428ea0` | `ai_release_actors_and_swarms` | 152 | 0.50 | 0.35 | 2 |
| `0x428f40` | `actor_reset_perception_scratch` | 190 | 0.40 | 0.20 | 4 |
| `0x429000` | `actor_dispatch_perception_reset` | 120 | 0.40 | 0.45 | 2 |
| `0x429080` | `ai_reset_all_actors_perception` | 105 | 0.50 | 0.50 |  |
| `0x4290f0` | `actor_reset_squad_link_for_type_change` | 98 | 0.30 | 0.20 | 4 |
| `0x429160` | `actor_update_activation_state` | 265 | 0.50 | 0.35 |  |
| `0x429270` | `actor_update_squad_link_state` | 438 | 0.50 | 0.25 | 3 |
| `0x429430` | `actor_update_idle_stagger` | 145 | 0.40 | 0.45 | 1 |
| `0x4294d0` | `actor_snapshot_orientation` | 160 | 0.50 | 0.70 |  |
| `0x429570` | `actor_update_target_lead_position` | 161 | 0.50 | 0.35 | 1 |
| `0x429620` | `actor_compute_accuracy_scale` | 156 | 0.40 | 0.35 | 4 |
| `0x4296c0` | `actor_fill_unit_position_context` | 211 | 0.35 | 0.20 | 5 |
| `0x4297a0` | `actor_refresh_combat_context` | 1845 | 0.50 | 0.20 | 30 |
| `0x429ee0` | `actor_run_mode_transition_loop` | 216 | 0.50 | 0.35 | 2 |
| `0x429fc0` | `ai_broadcast_communication_event` | 275 | 0.35 | 0.20 | 7 |
| `0x42a0e0` | `ai_alert_actors_in_grenade_radius` | 699 | 0.90 | 0.20 | 9 |
| `0x42a3a0` | `actor_squad_react_to_grenade` | 388 | 0.50 | 0.25 | 1 |
| `0x42a540` | `actor_dispatch_squad_order` | 104 | 0.40 | 0.25 | 4 |
| `0x42a5b0` | `actor_set_flag_bit1` | 39 | 0.30 | 0.55 |  |
| `0x42a5e0` | `actor_set_override_target` | 82 | 0.35 | 0.55 |  |
| `0x42a640` | `actor_apply_queued_look_to_unit` | 384 | 0.40 | 0.35 | 5 |
| `0x42a7c0` | `ai_initialize_for_new_map` | 119 | 0.55 | 0.60 |  |
| `0x42a840` | `ai_reset_for_new_map` | 184 | 0.50 | 0.55 |  |
| `0x42a900` | `ai_tick_dispatcher` | 74 | 0.40 | 0.50 | 3 |
| `0x42a950` | `ai_get_difficulty_request` | 107 | 0.40 | 0.45 |  |
| `0x42a9d0` | `ai_drift_zone_bias` | 180 | 0.35 | 0.45 |  |
| `0x42aa90` | `actor_apply_perception_scale` | 99 | 0.40 | 0.45 | 1 |
| `0x42ab00` | `ai_release_actors_filtered` | 203 | 0.40 | 0.20 | 3 |
| `0x42abd0` | `ai_release_inactive_swarms` | 178 | 0.90 | 0.50 | 2 |
| `0x42ac90` | `ai_squad_priority_compare` | 54 | 0.55 | 0.70 |  |
| `0x42acd0` | `ai_build_priority_target_list` | 372 | 0.90 | 0.45 | 1 |
| `0x42ae50` | `ai_release_inactive_encounters` | 241 | 0.90 | 0.75 | 1 |
| `0x42af50` | `actor_grenade_avoidance_entry_init` | 98 | 0.40 | 0.55 |  |
| `0x42afc0` | `actor_gather_nearby_grenade_targets` | 456 | 0.80 | 0.55 |  |
| `0x42b190` | `actor_grenade_trajectory_blocked` | 218 | 0.40 | 0.35 | 1 |
| `0x42b270` | `actor_evaluate_engagement_reachability` | 851 | 0.40 | 0.30 | 5 |
| `0x42b5d0` | `actor_grenade_parabolic_path_clear` | 565 | 0.50 | 0.40 |  |
| `0x42b810` | `actor_check_vehicle_target_available` | 111 | 0.40 | 0.50 | 3 |
| `0x42b880` | `actor_reassign_vehicle_seat` | 179 | 0.40 | 0.50 | 4 |
| `0x42b940` | `ai_notify_actors_of_encounter_state_change` | 304 | 0.80 | 0.50 | 4 |
| `0x42ba80` | `ai_mark_recognized_objects_for_reaction` | 287 | 0.35 | 0.50 | 2 |
| `0x42bbb0` | `ai_recompute_all_relationship_flags` | 439 | 0.40 | 0.45 | 2 |
| `0x42bd70` | `actor_squad_react_to_grenade_for_vehicle_occupants` | 201 | 0.30 | 0.40 | 4 |
| `0x42be40` | `actor_react_to_threat_event` | 324 | 0.40 | 0.60 | 2 |
| `0x42bf90` | `ai_process_vehicle_entry_queue` | 423 | 0.90 | 0.40 | 4 |

### AI communication and conversations  (41 functions, 23054 bytes)

| address | function | size | name conf | rewrite conf | UNSURE |
|---|---|---|---|---|---|
| `0x42c140` | `ai_clear_object_references` | 351 | 0.40 | 0.55 | 2 |
| `0x42c2a0` | `ai_refresh_unit_stimulus_and_alert` | 202 | 0.30 | 0.40 | 2 |
| `0x42c370` | `actor_notify_weapon_pickup_once` | 97 | 0.30 | 0.45 | 2 |
| `0x42c3e0` | `ai_scan_for_recent_combat_activity` | 547 | 0.35 | 0.45 | 3 |
| `0x42c610` | `ai_accumulate_repeated_event` | 813 | 0.40 | 0.40 | 3 |
| `0x42c940` | `ai_reset_fire_group_assignments` | 1358 | 0.3 | 0.3 | 3 |
| `0x42cf20` | `ai_communication_initialize` | 775 | 0.90 | 0.60 | 6 |
| `0x42d230` | `ai_communication_reset` | 221 | 0.40 | 0.60 |  |
| `0x42d310` | `ai_communication_target_result_reset` | 45 | 0.30 | 0.50 |  |
| `0x42d340` | `ai_communication_broadcast` | 5603 | 0.55 | 0.3 | 9 |
| `0x42e970` | `ai_communication_gate_line_played` | 50 | 0.30 | 0.40 | 1 |
| `0x42e9c0` | `ai_propagate_communication_reaction` | 711 | 0.30 | 0.25 | 6 |
| `0x42ec90` | `ai_select_communication_target` | 585 | 0.30 | 0.20 | 2 |
| `0x42eee0` | `ai_communication_play_event_line` | 888 | 0.40 | 0.35 | 3 |
| `0x42f260` | `actor_recompute_grenade_eligibility` | 268 | 0.35 | 0.35 | 2 |
| `0x42f370` | `actor_update_grenade_eligibility_state` | 261 | 0.35 | 0.30 | 3 |
| `0x42f480` | `actor_target_is_close_and_recognized` | 100 | 0.30 | 0.60 | 1 |
| `0x42f840` | `ai_dispatch_queued_order` | 126 | 0.40 | 0.40 | 2 |
| `0x42f8c0` | `ai_communication_line_fade_multiplier` | 220 | 0.30 | 0.20 | 5 |
| `0x42f9a0` | `actor_classify_communication_object_type` | 58 | 0.30 | 0.40 | 1 |
| `0x42f9e0` | `ai_communication_record_line_played` | 421 | 0.50 | 0.45 | 3 |
| `0x42fb90` | `ai_communication_rate_speaker` | 1002 | 0.40 | 0.35 | 6 |
| `0x42ff80` | `ai_communication_select_speaker_in_reference` | 326 | 0.45 | 0.55 | 2 |
| `0x4300d0` | `ai_communication_select_speaker_by_team` | 522 | 0.45 | 0.50 | 2 |
| `0x4302e0` | `actor_issue_order_or_vocalize` | 180 | 0.35 | 0.30 | 2 |
| `0x4303a0` | `actor_issue_multi_target_vocalization` | 69 | 0.30 | 0.25 | 1 |
| `0x4303f0` | `ai_communication_rate_player_proximity` | 965 | 0.45 | 0.45 | 2 |
| `0x4307c0` | `ai_conversation_activate` | 105 | 0.40 | 0.50 | 1 |
| `0x430830` | `ai_conversation_get_status` | 282 | 0.50 | 0.55 |  |
| `0x430960` | `ai_conversation_get_unknown_48` | 94 | 0.40 | 0.50 |  |
| `0x4309c0` | `ai_conversation_stop_all` | 90 | 0.40 | 0.40 | 1 |
| `0x430a20` | `ai_conversation_mark_all` | 77 | 0.40 | 0.50 |  |
| `0x430a70` | `ai_conversation_update` | 490 | 0.40 | 0.25 | 4 |
| `0x430c70` | `ai_conversation_clear_participant` | 185 | 0.40 | 0.50 |  |
| `0x430d30` | `ai_conversation_clear_object_references` | 354 | 0.50 | 0.45 | 1 |
| `0x430ea0` | `ai_conversation_stop` | 279 | 0.50 | 0.55 |  |
| `0x430fc0` | `ai_conversation_resolve_participants` | 1471 | 0.40 | 0.30 | 4 |
| `0x431590` | `ai_conversation_new` | 233 | 0.50 | 0.55 |  |
| `0x431680` | `ai_conversation_resolve_participant` | 1632 | 0.40 | 0.30 | 2 |
| `0x431d10` | `ai_conversation_activate_next_participant` | 352 | 0.40 | 0.60 | 1 |
| `0x431e70` | `ai_conversation_current_line_is_ready` | 646 | 0.40 | 0.30 | 7 |

### Packed AI references and object lists  (70 functions, 14404 bytes)

| address | function | size | name conf | rewrite conf | UNSURE |
|---|---|---|---|---|---|
| `0x432100` | `ai_weighted_random_index` | 246 | 0.40 | 0.50 |  |
| `0x432200` | `scenario_find_encounter_index_by_name` | 80 | 0.50 | 0.60 |  |
| `0x432260` | `encounter_definition_find_squad_index_by_name` | 80 | 0.50 | 0.60 |  |
| `0x4322c0` | `encounter_definition_find_platoon_index_by_name` | 80 | 0.50 | 0.60 |  |
| `0x432320` | `ai_reference_parse` | 254 | 0.50 | 0.60 |  |
| `0x432420` | `ai_reference_expand_to_platoon_range` | 197 | 0.40 | 0.40 | 1 |
| `0x4324f0` | `ai_reference_squad_iterator_new` | 179 | 0.40 | 0.40 |  |
| `0x4325b0` | `ai_reference_squad_iterator_next` | 151 | 0.40 | 0.40 |  |
| `0x432650` | `ai_reference_actor_iterator_new` | 121 | 0.50 | 0.50 |  |
| `0x4326d0` | `ai_reference_actor_iterator_next` | 97 | 0.50 | 0.50 |  |
| `0x432740` | `ai_reference_build_object_list` | 371 | 0.40 | 0.55 | 1 |
| `0x4328c0` | `ai_reference_spawn_starting_location_object` | 383 | 0.40 | 0.45 |  |
| `0x432a40` | `ai_object_list_spawn_members` | 143 | 0.35 | 0.45 |  |
| `0x432ad0` | `ai_object_list_clear_orders_with_weapon` | 173 | 0.30 | 0.50 |  |
| `0x432b80` | `ai_reference_activate_squads` | 79 | 0.40 | 0.50 |  |
| `0x432bd0` | `ai_reference_notify_actors` | 66 | 0.30 | 0.45 |  |
| `0x432c20` | `ai_reference_notify_squad_index` | 82 | 0.30 | 0.50 | 1 |
| `0x432c80` | `ai_reference_resolve_squad_datum` | 171 | 0.30 | 0.40 | 3 |
| `0x432d30` | `ai_reference_respawn_placed_members` | 81 | 0.35 | 0.45 |  |
| `0x432d90` | `ai_reference_respawn_all_players` | 82 | 0.35 | 0.45 |  |
| `0x432df0` | `ai_reference_respawn_member` | 134 | 0.35 | 0.40 | 4 |
| `0x432e80` | `ai_object_list_respawn_members` | 142 | 0.35 | 0.45 |  |
| `0x432f10` | `ai_reference_mark_squads_unknown_11` | 52 | 0.30 | 0.50 |  |
| `0x432f50` | `ai_reference_for_each_squad` | 60 | 0.30 | 0.45 | 1 |
| `0x432f90` | `ai_reference_get_stat_pair` | 491 | 0.35 | 0.45 |  |
| `0x433180` | `ai_platoon_range_has_available` | 125 | 0.30 | 0.45 |  |
| `0x433200` | `ai_platoon_range_clear_unknown_00` | 105 | 0.30 | 0.45 |  |
| `0x433270` | `ai_platoon_range_set_unknown_00` | 105 | 0.30 | 0.45 |  |
| `0x4332e0` | `ai_platoon_range_set_unknown_01` | 106 | 0.30 | 0.45 |  |
| `0x433350` | `ai_platoon_range_set_unknown_02` | 114 | 0.30 | 0.45 |  |
| `0x4333d0` | `ai_squad_find_best_matching_member` | 437 | 0.40 | 0.40 |  |
| `0x433590` | `ai_squads_merge` | 982 | 0.35 | 0.20 | 12 |
| `0x433970` | `ai_unit_remap_actor_to_squad` | 241 | 0.40 | 0.35 | 2 |
| `0x433a70` | `ai_object_list_remap_units_and_children` | 296 | 0.35 | 0.30 | 2 |
| `0x433ba0` | `ai_category_matches_wildcard` | 199 | 0.30 | 0.40 | 1 |
| `0x433cc0` | `ai_object_process_nearby_actors` | 335 | 0.35 | 0.30 | 1 |
| `0x433e20` | `ai_count_actors_in_mode9_group` | 124 | 0.30 | 0.50 |  |
| `0x433ea0` | `ai_reference_units_exit_vehicles` | 1627 | 0.30 | 0.30 | 1 |
| `0x434500` | `ai_reference_reset_or_wake_awareness` | 139 | 0.30 | 0.35 | 2 |
| `0x434590` | `ai_object_list_reset_or_wake_awareness` | 537 | 0.30 | 0.30 | 2 |
| `0x4347b0` | `ai_object_list_set_unit_flag_400` | 259 | 0.30 | 0.35 |  |
| `0x4348c0` | `ai_object_list_set_unit_flag_800` | 259 | 0.30 | 0.35 |  |
| `0x4349d0` | `ai_reference_face_starting_location` | 276 | 0.35 | 0.50 | 1 |
| `0x434af0` | `ai_reference_refill_grenades` | 391 | 0.35 | 0.45 | 1 |
| `0x434c80` | `ai_reference_clear_search_target` | 57 | 0.40 | 0.50 |  |
| `0x434cc0` | `ai_reference_set_search_target_point` | 63 | 0.40 | 0.40 |  |
| `0x434d00` | `ai_reference_set_search_target_area` | 57 | 0.40 | 0.50 |  |
| `0x434d40` | `ai_reference_set_unknown_1cb` | 64 | 0.30 | 0.45 |  |
| `0x434d90` | `ai_reference_flee_if_ready` | 91 | 0.35 | 0.30 | 2 |
| `0x434df0` | `ai_unit_flee_if_ready` | 93 | 0.35 | 0.40 | 1 |
| `0x434e60` | `ai_reference_invoke_squad_callback_406f80` | 102 | 0.30 | 0.40 | 2 |
| `0x434ed0` | `ai_actor_type_get_morale_grade` | 75 | 0.40 | 0.45 |  |
| `0x434f20` | `ai_object_list_max_flee_grade` | 666 | 0.35 | 0.45 | 1 |
| `0x4351c0` | `ai_reference_detach_actors_from_encounters` | 153 | 0.40 | 0.55 | 1 |
| `0x435260` | `ai_object_list_detach_actors_from_encounters` | 433 | 0.40 | 0.55 | 1 |
| `0x435420` | `ai_unit_create_actor` | 276 | 0.40 | 0.55 | 1 |
| `0x435540` | `ai_unit_set_actor_unknown_0a` | 75 | 0.25 | 0.40 |  |
| `0x435590` | `squad_members_assign_team_and_request_order` | 147 | 0.50 | 0.60 | 1 |
| `0x435630` | `squad_members_request_order` | 69 | 0.50 | 0.45 |  |
| `0x435680` | `ai_actor_get_activity_stage` | 121 | 0.40 | 0.45 | 1 |
| `0x435700` | `ai_reference_max_activity_stage` | 69 | 0.40 | 0.50 |  |
| `0x435750` | `ai_unit_set_squad_reference` | 431 | 0.35 | 0.45 | 1 |
| `0x435900` | `ai_object_attention_find_or_create` | 133 | 0.35 | 0.65 | 1 |
| `0x435990` | `ai_object_attention_remove` | 111 | 0.40 | 0.70 |  |
| `0x435a00` | `ai_unit_dispatch_actor_event_d` | 76 | 0.30 | 0.35 | 1 |
| `0x435a50` | `ai_unit_clear_actor_vocalization` | 83 | 0.35 | 0.45 |  |
| `0x435ab0` | `ai_reference_squad_set_unknown_10` | 51 | 0.25 | 0.40 |  |
| `0x435af0` | `ai_reference_set_combat_alert_flag` | 64 | 0.35 | 0.40 |  |
| `0x435b30` | `encounter_set_team` | 136 | 0.40 | 0.50 | 1 |
| `0x435bc0` | `ai_reference_set_squads_unknown_14` | 56 | 0.25 | 0.40 |  |

### Encounters, squads and platoons  (46 functions, 16976 bytes)

| address | function | size | name conf | rewrite conf | UNSURE |
|---|---|---|---|---|---|
| `0x435c00` | `encounters_initialize` | 170 | 0.90 | 0.85 | 1 |
| `0x435cb0` | `encounters_reset` | 149 | 0.85 | 0.85 |  |
| `0x435d50` | `encounters_spawn_initial` | 172 | 0.45 | 0.60 | 1 |
| `0x435e00` | `encounters_update` | 246 | 0.50 | 0.70 | 1 |
| `0x435f00` | `encounters_recompute_dirty` | 129 | 0.55 | 0.75 |  |
| `0x435f90` | `encounters_note_hostile_object` | 306 | 0.35 | 0.55 | 1 |
| `0x4360d0` | `encounter_build_firing_position_claims` | 178 | 0.50 | 0.60 | 1 |
| `0x436190` | `encounter_gather_occupied_clusters` | 1147 | 0.45 | 0.45 | 1 |
| `0x436620` | `encounter_remove_actor` | 229 | 0.60 | 0.75 | 1 |
| `0x436710` | `ai_encounter_stamp_team_from_unit` | 83 | 0.35 | 0.40 | 4 |
| `0x436770` | `encounter_add_actor` | 452 | 0.60 | 0.60 | 1 |
| `0x436940` | `ai_actor_link_to_unassigned_list` | 76 | 0.40 | 0.40 | 1 |
| `0x436990` | `ai_actor_unlink_from_unassigned_list` | 92 | 0.40 | 0.40 |  |
| `0x4369f0` | `ai_reference_actor_iterator_init_cursor` | 61 | 0.40 | 0.50 | 2 |
| `0x436a30` | `actor_iterator_new` | 62 | 0.50 | 0.55 |  |
| `0x436a70` | `actor_iterator_next` | 154 | 0.50 | 0.50 |  |
| `0x436b10` | `ai_pursuit_note_object` | 120 | 0.35 | 0.30 |  |
| `0x436b90` | `ai_pursuit_check_object` | 122 | 0.35 | 0.30 |  |
| `0x436c10` | `squad_recent_object_list_clear` | 73 | 0.50 | 0.50 |  |
| `0x436c60` | `squad_recent_object_get_or_create` | 223 | 0.50 | 0.40 | 1 |
| `0x436d40` | `ai_starting_location_derive_placement_flags` | 128 | 0.30 | 0.20 | 2 |
| `0x436dc0` | `encounter_evaluate_support_needs` | 451 | 0.35 | 0.50 | 2 |
| `0x436f90` | `encounter_squad_reset_starting_location_mask` | 196 | 0.45 | 0.60 | 1 |
| `0x437060` | `encounter_new` | 435 | 0.70 | 0.80 | 1 |
| `0x437220` | `squad_pick_random_starting_location` | 640 | 0.55 | 0.55 | 1 |
| `0x4374a0` | `ai_squad_resolve_actor_type` | 97 | 0.35 | 0.45 |  |
| `0x437510` | `encounter_spawn_squads` | 468 | 0.45 | 0.50 | 1 |
| `0x437710` | `encounter_activate` | 263 | 0.60 | 0.70 | 1 |
| `0x437820` | `ai_encounter_record_recent_zone` | 80 | 0.35 | 0.35 |  |
| `0x437870` | `encounter_deactivate` | 204 | 0.60 | 0.75 | 1 |
| `0x437940` | `encounter_recompute_morale` | 1234 | 0.50 | 0.50 | 1 |
| `0x437e20` | `encounters_update_activation` | 1139 | 0.50 | 0.45 | 1 |
| `0x4382b0` | `encounter_release_stale_props` | 303 | 0.40 | 0.60 |  |
| `0x4383f0` | `ai_insert_scored_candidate_pair` | 130 | 0.30 | 0.40 |  |
| `0x438480` | `ai_pick_weighted_candidate` | 242 | 0.40 | 0.50 | 1 |
| `0x438580` | `encounter_choose_vocalizations` | 2079 | 0.35 | 0.35 | 2 |
| `0x438db0` | `encounter_advance_grenade_timers` | 107 | 0.30 | 0.75 | 2 |
| `0x438e20` | `encounter_squad_spawn_actor` | 316 | 0.40 | 0.35 | 3 |
| `0x438f60` | `encounter_squad_spawn_reinforcement` | 317 | 0.40 | 0.30 | 4 |
| `0x4390a0` | `encounter_process_squad_reinforcements` | 454 | 0.45 | 0.40 | 1 |
| `0x439270` | `encounter_squad_clear_spawn_delay` | 123 | 0.40 | 0.40 | 2 |
| `0x4392f0` | `encounter_decay_squad_spawn_delays` | 184 | 0.45 | 0.40 | 1 |
| `0x4393b0` | `encounter_update_platoon_defending_flag` | 225 | 0.35 | 0.35 | 1 |
| `0x4394a0` | `encounter_redistribute_squads_toward_targets` | 2235 | 0.35 | 0.15 | 7 |
| `0x439d80` | `encounter_propagate_platoon_state_to_actors` | 397 | 0.40 | 0.30 | 3 |
| `0x439f20` | `encounter_evaluate_platoon_condition` | 285 | 0.35 | 0.45 | 1 |

### path_find: the navigation-mesh A* search  (17 functions, 4761 bytes)

| address | function | size | name conf | rewrite conf | UNSURE |
|---|---|---|---|---|---|
| `0x43a070` | `path_find_set_avoid_sphere` | 47 | 0.55 | 0.55 |  |
| `0x43a0a0` | `path_find_test_direct_reachability` | 234 | 0.40 | 0.30 | 1 |
| `0x43a190` | `path_find_validate_and_record_goal` | 136 | 0.35 | 0.30 | 2 |
| `0x43a220` | `path_find_find_unobstructed_ancestor` | 236 | 0.35 | 0.35 | 2 |
| `0x43a310` | `path_find_compute_heuristic` | 439 | 0.45 | 0.25 | 4 |
| `0x43a4d0` | `path_find_reconstruct_path` | 551 | 0.40 | 0.15 | 4 |
| `0x43a700` | `path_find_context_init` | 44 | 0.50 | 0.70 |  |
| `0x43a730` | `path_find_set_goal` | 40 | 0.55 | 0.60 |  |
| `0x43a760` | `path_find_push_start_node` | 321 | 0.50 | 0.35 | 3 |
| `0x43a8b0` | `path_find_run` | 1716 | 0.55 | 0.20 | 2 |
| `0x43af70` | `path_find_heap_sift_up` | 150 | 0.60 | 0.65 |  |
| `0x43b010` | `path_find_heap_sift_down` | 213 | 0.60 | 0.65 |  |
| `0x43b0f0` | `path_find_heap_push` | 63 | 0.60 | 0.65 |  |
| `0x43b130` | `path_find_vertex_distance` | 137 | 0.35 | 0.30 | 2 |
| `0x43b1c0` | `path_find_gather_adjacent_edges` | 227 | 0.45 | 0.20 | 2 |
| `0x43b2b0` | `path_find_hash_lookup_vertex` | 57 | 0.45 | 0.55 | 1 |
| `0x43b3b0` | `path_find_score_avoidance_penalty` | 150 | 0.50 | 0.50 | 1 |

### ai_search: the obstacle-graph point search  (25 functions, 11239 bytes)

| address | function | size | name conf | rewrite conf | UNSURE |
|---|---|---|---|---|---|
| `0x43b450` | `ai_search_heap_sift_up` | 119 | 0.55 | 0.50 |  |
| `0x43b4d0` | `ai_search_heap_sift_down` | 202 | 0.55 | 0.55 |  |
| `0x43b5a0` | `ai_search_add_node` | 483 | 0.40 | 0.25 | 2 |
| `0x43b790` | `ai_search_context_init` | 157 | 0.50 | 0.40 | 1 |
| `0x43b830` | `ai_search_evaluate_edge_cost` | 560 | 0.30 | 0.10 | 3 |
| `0x43ba60` | `ai_search_expand_point_neighbors` | 579 | 0.35 | 0.10 | 6 |
| `0x43bcb0` | `ai_search_step` | 354 | 0.40 | 0.15 | 2 |
| `0x43be20` | `ai_search_run` | 105 | 0.40 | 0.40 | 2 |
| `0x43be90` | `ai_navigate_around_obstacles` | 1196 | 0.35 | 0.10 | 7 |
| `0x43c4b0` | `ai_search_append_obstacle` | 83 | 0.50 | 0.55 |  |
| `0x43c510` | `ai_search_gather_obstacles` | 883 | 0.50 | 0.10 | 4 |
| `0x43c890` | `ai_search_find_covering_point` | 95 | 0.40 | 0.40 |  |
| `0x43c8f0` | `ai_search_find_nearest_visible_point` | 163 | 0.40 | 0.15 | 2 |
| `0x43c9a0` | `ai_search_compute_point_tangents` | 149 | 0.40 | 0.40 |  |
| `0x43ca40` | `ai_search_flood_fill_group` | 285 | 0.40 | 0.40 | 1 |
| `0x43cb60` | `ai_search_partition_into_groups` | 159 | 0.45 | 0.30 | 1 |
| `0x43cc00` | `path_find_simplify_waypoints` | 846 | 0.40 | 0.10 | 1 |
| `0x43cf60` | `ai_search_find_circle_tangent_point` | 415 | 0.40 | 0.35 | 1 |
| `0x43d100` | `ai_search_find_circle_portal_crossing` | 308 | 0.35 | 0.30 | 1 |
| `0x43d240` | `ai_search_choose_shorter_corner` | 614 | 0.35 | 0.05 | 1 |
| `0x43d4b0` | `path_find_trace_cluster_boundary` | 716 | 0.30 | 0.10 | 1 |
| `0x43d790` | `path_find_trace_cluster_boundary_from_vertex` | 383 | 0.30 | 0.10 | 1 |
| `0x43d910` | `path_find_heights_are_close` | 160 | 0.40 | 0.30 | 2 |
| `0x43d9b0` | `path_find_trace_bsp_boundary` | 1237 | 0.35 | 0.10 | 2 |
| `0x43de90` | `path_find_test_segment_unobstructed` | 988 | 0.35 | 0.05 | 1 |

### Props: per-actor perception records  (10 functions, 2674 bytes)

| address | function | size | name conf | rewrite conf | UNSURE |
|---|---|---|---|---|---|
| `0x43e270` | `actor_find_or_allocate_prop` | 973 | 0.40 | 0.20 | 2 |
| `0x43e640` | `actor_init_prop_from_object` | 504 | 0.40 | 0.25 | 1 |
| `0x43e840` | `actor_copy_prop_and_reset` | 205 | 0.40 | 0.35 | 1 |
| `0x43e910` | `actor_allocate_paired_prop` | 101 | 0.40 | 0.30 | 2 |
| `0x43e980` | `actor_allocate_paired_prop_with_kind` | 146 | 0.40 | 0.30 |  |
| `0x43ea20` | `actor_unlink_prop` | 95 | 0.45 | 0.40 |  |
| `0x43ea80` | `actor_find_prop_for_object` | 166 | 0.40 | 0.30 |  |
| `0x43eb30` | `actor_find_or_create_shared_prop` | 411 | 0.40 | 0.20 | 1 |
| `0x43ecd0` | `actor_prop_iterator_init` | 32 | 0.40 | 0.40 | 1 |
| `0x43ecf0` | `actor_prop_iterator_next` | 41 | 0.40 | 0.45 |  |
