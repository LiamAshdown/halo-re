# ai module type notes

Header: `types/ai.h`. Smoke test: `out/phase4/ai_smoke.c`, checked with

```
cd C:\Users\Liam-\halo-re
C:\msys64\ucrt64\bin\gcc.exe -fsyntax-only -I types out\phase4\ai_smoke.c
```

The header reuses `datum_index` / `data_array` from `types/memory.h` and
`real_point2d` / `real_point3d` / `real_vector2d` / `real_vector3d` from `types/math.h`;
neither is redefined. Everything else this module touches through a pointer lives in
`types/objects.h`, `types/units.h`, `types/cache.h` or `types/tags.h`.

## What the binary itself fixed (preferred over the decompiler)

* **Every element stride comes from a named allocation**, not from field arithmetic.
  `actors_initialize` (0x426710) calls `game_state_new("actor", 0x100)`,
  `game_state_new("swarm", 0x20)` and `game_state_new("swarm component", 0x100)`;
  `ai_initialize_for_new_map` (0x42a7c0) adds `game_state_new(<0x0065f008>, 0x300)`, and the
  literal at 0x0065f008 read straight out of `bin/halo.exe` is `"prop"`;
  `encounters_initialize` (0x435c00) adds `"encounter" 0x80` and `"ai pursuit" 0x100`;
  `ai_communication_initialize` (0x42cf20) builds an inline `data_array` header with
  `strncpy(dest, "ai conversation", 0x1f)`, `maximum_count = 8`, `size = 0x64`.
  The strides (0x724, 0x98, 0x40, 0x138, 0x6c, 0x28, 0x64) then come from the
  `(index & 0xffff) * stride + data_array->data` form every accessor uses.
* **ai_globals is 0x8dc bytes** because `ai_initialize_for_new_map` bumps the game-state
  cursor by 0x8dc and feeds that exact value to `crc32_update`.
* **path_find_context is 0x1008c bytes** because `path_find_context_init` (0x43a700) zeroes
  0x4023 dwords. The three internal arrays are pinned by the literal offsets the accessors
  use: nodes at +0x84, the heap count at +0xd084, the vertex hash at +0xe08a.
  `(0xd084 - 0x84) / 0x34 == 1024` and `(0xe08a - 0xd086) / 4 == 1025` both divide exactly,
  which is the cross-check that the node stride 0x34 and the heap-entry stride 4 are right.
* **The two flat sub-record tables** are reserved by `encounters_initialize` as raw game-state
  runs: 0x8000 bytes at 0x008802cc and 0x1000 bytes at 0x008802c4. `encounter_new` (0x437060,
  currently named `squad_create`) indexes them with `* 0x20` and `* 0x10`, so they hold 0x400
  `encounter_squad_state` and 0x100 `encounter_platoon_state` records.
* **Three actor fields come out of the actor tag**, via `actor_new` reading
  `ActorVariant.actor_definition.tag_id` at ActorVariant+0x10 and then the `Actor` tag data:
  `Actor+0x14 type` -> `actor.type`, `Actor.flags` bit 26 (`swarm` in the invader bitfield)
  -> `actor.swarm`, bit 21 (`flying`) -> `actor.flying`, and `Actor+0x90`
  (`glass_ignorance_chance`) rolled once against `random_real` -> `actor.ignores_glass`.
* **actor_movement_context is 0x6048** because `actor_movement_choose_avoidance_direction`
  (0x4193d0) passes `&local_6048` and the same frame's `local_8` / `local_4` are read back
  through that pointer at +0x6040 and +0x6044. The obstacle array then runs 0x40..0x603f,
  which is exactly 0x400 entries of 0x18.

## Struct by struct: which functions established which fields

### `actor` (0x724)

| fields | established by |
| --- | --- |
| 0x04 type, 0x06 swarm, 0x07, 0x08 active, 0x09, 0x0c, 0x12, 0x13, 0x1c, 0x1e, 0x20, 0x24, 0x28, 0x30, 0x38, 0x4a, 0x54, 0x58, 0x5c, 0x64, 0x6a, 0x6c, 0x6e, 0x72, 0x88, 0x8e, 0x90, 0x94, 0x98, 0x99, 0x1c9, 0x1cc, 0x1d0, 0x1d4, 0x1dc, 0x268, 0x26c, 0x270, 0x278, 0x350..0x3b7, 0x36c..0x3b4, 0x3b8, 0x3c0, 0x400, 0x46c, 0x494, 0x504, 0x505, 0x544..0x548, 0x5a4..0x5c7, 0x5c8..0x5d8, 0x5f0..0x5fa, 0x610, 0x61c, 0x6a4, 0x6b4, 0x6cc, 0x6ce | `actor_new` @0x426760 (the constructor; the single largest source) |
| 0x6a awareness_level, 0x6c mode, 0x70 mode_changed, 0x9c mode_data (0xbc bytes), 0x3c6, 0x3c8 recognition[4], 0x3d8 | `actor_set_mode` @0x40d8d0 (the memcpy length is `actor_mode_definition.data_size`, and `mode_data` cannot extend past 0x158, the next independently-written field) |
| 0x18 unit_index, 0x1c counts_toward_encounter | `actor_attach_to_unit` @0x427560, `actor_unlink_unit` @0x427bc0 |
| 0x24 cluster_unit_index, 0x1e cluster_count, 0x28 swarm_index | `actor_link_to_unit_cluster` @0x4279f0, `actor_remove_from_unit_cluster` @0x427c90, `actor_create_swarm` @0x427f40, `actor_delete_swarm` @0x4280b0 |
| 0x2c next_in_encounter, 0x34 encounter_index, 0x3a squad_index, 0x3c platoon_index, 0x3e team, 0x374 | `squad_remove_actor` @0x436620 and `encounter_add_actor` @0x436770 |
| 0x50 first_prop | `0x43e640`, `0x43ea20`, `actor_target_reset_seen_flags` @0x41f9d0, `actor_target_reset_shot_counters` @0x41fa20, `0x427e00` |
| 0x400 queued_movement, 0x46c active_movement, 0x484, 0x4a0, 0x4a8 | `actor_movement_set_destination_point` @0x417610 (writes the queued copy and then copies all six dwords to the active copy), `actor_movement_action_cancel` @0x417a30, `actor_movement_action_complete` @0x41a430, `actor_movement_action_is_complete` @0x41a960 |
| 0x3b8 firing_position_index, 0x3ba, 0x3bb | `0x414060`, `actor_movement_set_destination_firing_point` @0x417750 |
| 0x3c6 recognition_cursor, 0x3c8 recognition[4], 0x3d8, 0x3d9, 0x3dc recognition_position | `0x4141a0` (writes `entry[cursor].type` at +0x3c8+i*4 and `entry[cursor].firing_position_index` at +0x3ca+i*4, wraps the cursor modulo 4, and copies 12 bytes out of the encounter `ScenarioFiringPosition` block at stride 0x18) |
| 0x174 position, 0x180, 0x18c, 0x6d0 flags, 0x6e0, 0x6ec, 0x6fc, 0x708, 0x714, 0x720 | `actor_snapshot_orientation` @0x4294d0 and `0x42a5e0` (which pairs flags bit 0x800 with 0x720) |
| 0x4a idle_counter, 0x4c | `0x429430` |
| 0x544/0x546/0x548 vocalization | `actor_clear_vocalization` @0x414560 |
| 0x37c / 0x388 / 0x608 wait times | `0x4028e0` |
| 0x60, 0x62, 0x68, 0x8e, 0x90, 0x92, 0x6a | `0x435420` |

### `prop` (0x138)

`0x43e640` is the initializer and pins 0x04, 0x08, 0x0c, 0x10, 0x12, 0x14, 0x18, 0x1c, 0x20,
0x28, 0x4e, 0x60..0x63, 0x66, 0x6a, 0x70, 0x74, 0x76, 0x7c, 0x8c, 0xa0, 0xb0, 0xb4, 0xb8,
0x127, 0x128, 0x12e. `0x43e270` pins 0x08 (list walk), 0x0c, 0x24 kind, 0x11c distance and
the 0x4e-dword repurpose memset that fixes the 0x138 size. `actor_target_reset_combat_flags`
@0x41baf0 pins 0x64, 0xb9, 0xba, 0xbb; `actor_target_reset_seen_flags` @0x41f9d0 pins 0x6c
and 0x74; `actor_target_reset_shot_counters` @0x41fa20 pins 0xaa, 0xac, 0xae;
`ai_target_distance_qsort_compare` @0x41d7a0 pins 0x11c as the float sort key;
`actor_target_get_relationship_object` @0x41f3a0 pins 0x110. The heavy readers
`0x41abd0`, `0x41c4b0` and `0x41c8f0` fill in most of the rest.

### `encounter` (0x6c) / `encounter_squad_state` (0x20) / `encounter_platoon_state` (0x10)

`encounter_new` @0x437060 writes 0x02 team (from `ScenarioEncounter.team_index`), 0x04/0x06
(the squad run), 0x08/0x0a (the platoon run), 0x10, 0x14, 0x20, 0x3c/0x40/0x41 (from
`ScenarioEncounter.flags` bits 1..3), 0x3e, 0x42, 0x44, 0x45, 0x46, 0x50, 0x54, 0x58, 0x5c,
plus `encounter_squad_state` 0x0c, 0x10, 0x11, 0x12 and `encounter_platoon_state` 0x00.
`squad_remove_actor` @0x436620 and `encounter_add_actor` @0x436770 pin 0x14 first_actor,
0x18 member_count, 0x1c live_count, 0x28 dirty, and both sub-record member counters.
`squad_recent_object_get_or_create` @0x436c60 pins 0x38 first_pursuit.
`0x437940` is the main morale pass and touches nearly every remaining byte of all three.

### `swarm` (0x98) / `swarm_component` (0x40)

`actor_create_swarm` @0x427f40 and `swarm_add_component` @0x4279a0 fix the two parallel
arrays: the unit handles at +0x18+i*4 and the component datums at +0x58+i*4, with the count
at +0x02. `0x58 + 16*4 == 0x98` and `0x18 + 16*4 == 0x58`, so both arrays hold exactly 16
entries -- that is where `k_swarm_maximum_components` comes from.
`actor_remove_from_unit_cluster` @0x427c90 confirms the swap-with-last removal on both.
`0x428130` pins `swarm_component.position` (an `object_get_position` out-parameter) and
+0x10.

### `ai_globals` (0x8dc)

`ai_reset_for_new_map` @0x42a840 pins 0x00, 0x01, 0x02, 0x08, 0x10, 0x14..0x28, 0x130, 0x132,
the 0xa0-dword block at 0x134, and 0x3b4. `0x42d230` pins 0x10, 0x14..0x28, 0x2c, 0x2e and
zeroes the 0x40-dword ring at 0x30. `ai_conversation_stop` @0x430ea0 is what makes that ring
0x10-byte entries: it writes the definition index at `globals + (cursor + 3) * 0x10`, i.e.
`+0x30 + cursor*0x10`, then the two reason bytes at +0x02/+0x03 and the tick at +0x04, and
masks the cursor with 0xf. `0x429430` pins 0x03/0x04/0x06 (the update-stagger trio).
`ai_process_vehicle_entry_queue` @0x42bf90 pins 0x8b8 and the queue at 0x8bc.

### `path_find_context` (0x1008c) and `ai_search_context` (0x1532)

`path_find_push_start_node` @0x43a760 writes a whole node and so gives the 0x34 layout;
`path_find_heap_push` @0x43b0f0 and the two sift routines give the {node, key} 4-byte heap
entry and the one-based indexing; `0x43b2b0` gives the 512-bucket, 8-wide hash probe.
For the point search, `0x43b5a0` writes a whole `ai_search_node` (0x28) and both the node
array bound (0x80) and the heap bound (0x80); `0x43b790` initializes the context header and
`0x43bcb0` reads the same heap count both as `context+0x1430` and as dword index 0x50c of
the same base, which is the same byte and confirms the offset.

## Unresolved offsets

* **actor** -- roughly 55% of the 0x724 bytes are still `unknown_XX`. The largest opaque runs
  are 0x9c..0x157 (`mode_data`, a union whose per-mode shape is set by
  `actor_mode_definition.data_size` and which no single function reveals in full),
  0x1a0..0x237, 0x2bc..0x2ed, 0x419..0x46b and 0x5de..0x607. Specific conflicts I could not
  settle:
  * 0x68 -- written as a byte by 0x435420 but read as a float by 0x4112b0 and 0x4394a0. It
    cannot be a float because 0x6a is unambiguously the int16 awareness level, so the float
    reads are almost certainly a decompiler variable-reuse artefact. Declared `uint8_t`.
  * 0x74/0x78/0x7c/0x80/0x84 -- 0x420290 reads them as int32 and 0x4112b0 as float. Declared
    int32/int16 on the awareness-update reading, which is the better-understood function.
  * 0x594 -- five distinct float reads in 0x4180c0 but only four fit before 0x5a4. Declared
    `float[4]`; the fifth read is the first element of `position_cache_a`.
  * 0x158 `active_unit_index` -- 0x4193d0 prefers it over `unit_index` and everything else
    treats it as an object handle, but nothing proves it is the vehicle the actor occupies
    rather than, say, a possessed unit. The name is a description of the fallback, not a
    claim about meaning.
  * 0x1d8..0x1db -- read as one uint32 by 0x41c8f0 and as two bytes at 0x1da by the same
    function. Left as raw bytes.
  * 0x376 `ignores_glass` -- the roll source (`Actor+0x90`) is certain, the field name comes
    from the invader field name `glass_ignorance_chance` plus the fact that only the movement
    and step-test routines read it. Not independently confirmed.
* **prop** -- 0x120, 0x124, 0x130 and 0x134 are each read as a float by one function and as
  three separate bytes at +1/+2/+3 by another (0x41c8f0 versus 0x42fb90). Declared as the
  bytes, because the byte reads are the more numerous and the more consistent.
  0x040/0x044/0x048 and 0x090..0x098 are only ever touched by 0x412ba0, the 4754-byte
  context gatherer, which I did not fully read.
* **encounter** -- 0x2a..0x37 and 0x60..0x6b are only written by `0x437940` (the 1234-byte
  morale recompute) and `0x4394a0` (the 2235-byte redistribution pass); I did not read
  either far enough to name them.
* **encounter_squad_state / encounter_platoon_state** -- only about half the bytes are
  attributed. My offset miner over-reported here, because 0x437940 reuses the same
  decompiler variable for a squad-state pointer and for other bases inside the same
  function; every offset at or past the record size was discarded, and what remains below
  it should be treated as weaker than the rest of the header.
* **ai_globals** -- the 0x280-byte block at 0x134 and everything between 0x3fc and 0x8b7 is
  unattributed. 0x1f4 is used by 0x42ec90 and `ai_communication_record_line_played`
  @0x42f9e0 as the base of a timestamp table whose element size I did not pin.
* **ai_conversation** -- the participant array is at 0x28 with 8 entries because
  `ai_conversation_stop` iterates `participants.count` of the tag block against a bitmask at
  0x14 and reads handles at `+0x28 + i*4`, and the datum array caps the instance count at 8;
  the tag itself permits more participants than 8 handles would hold, so the array bound is
  inferred from the record size (0x48 - 0x28) / 4, not stated.
* **path_find_context** -- the leading 0x48 bytes are copied wholesale from a caller-owned
  request block, so only 0x14 (start position) and 0x20 (start vertex) inside that range are
  named. The vertex hash is zeroed by `path_find_context_init` but `0x43b2b0` terminates on
  -1, so something else must fill it with 0xff; I did not find that memset (it is most
  likely inside `path_find_run` @0x43a8b0, which I did not read in full).
* **ai_search_context** -- 0x00, 0x04, 0x0c, 0x18, 0x29 and 0x2a are constructor arguments
  whose meaning is not determined by `0x43b790` alone.
* **actor_mode_definition** -- the 0x38 stride is solid (four independent accessors index it
  with `* 0x38`), but only four of its fourteen dwords are used by this module: +0x00
  data_size, +0x04 an int16 grade, +0x08 enter, +0x14 update, +0x18 exit. The row count 16 is
  *not* proven; it is the largest mode index the code tests (0xc) rounded up. Likewise
  `actor_type_procs[16]` takes its bound from `ActorType` having 16 values, not from the
  module.

## Misattributed functions

### The 0x430830..0x431e70 block is the AI conversation system, not squads

`ai_communication_initialize` @0x42cf20 builds the data array at **0x008802d4** inline with
the name `"ai conversation"`, `maximum_count = 8` and `size = 0x64`. Every function in the
range below indexes that array with `* 100`, and resolves `record + 0x02` against
`Scenario.ai_conversations` (offset 0x468/0x46c, stride 0x74). So the phase-4 names that say
"squad" are wrong for all of these, and the "member slots" they describe are conversation
*participants*:

| address | phase-4 name / summary | what it actually operates on |
| --- | --- | --- |
| 0x430830 | "coarse status code for the squad(s) matching a squad definition index" | status of the conversation instances of one `ScenarioAIConversation` |
| 0x430960 | "formation/actor-type index on the live squad instance" | a field of the live conversation instance |
| 0x4309c0 | "destroys every live squad instance" | stops every instance of one conversation |
| 0x430a20 | "marks every live squad instance with a flag" | same, on conversation instances |
| 0x430a70 | "per-tick update of every squad instance" | the per-tick conversation update |
| 0x430c70 | "removes an object index from any squad member slot" | clears a participant reference |
| 0x430d30 | `squad_clear_unit_references` | `ai_conversation_clear_object_references` |
| 0x430ea0 | `squad_despawn` | `ai_conversation_stop`; it is also the writer of the ai_globals event ring |
| 0x430fc0 | "decides whether a squad should regroup/retreat" | a conversation-driven member mode change |
| 0x431590 | `squad_new_instance` | `ai_conversation_new` |
| 0x431680 | "resolves what object/unit should occupy a squad member slot" | resolves a conversation participant |
| 0x431d10 | "activates the next resolved squad member slot" | activates the next participant |
| 0x431e70 | "whether the activated squad member is ready to be placed" | whether the participant is ready to speak |

The functions that really do operate on the runtime squad grouping are the ones that touch
0x008802c8 (`encounter`, stride 0x6c), 0x008802cc (`encounter_squad_state`, 0x20) and
0x008802c4 (`encounter_platoon_state`, 0x10): 0x435c00..0x439f20 plus the actor-side
helpers. Two names in that group are also off by one level of the hierarchy:

* `squad_create` @0x437060 creates an **encounter** datum (one per `ScenarioEncounter`), not
  a squad. `encounters_reset` @0x435cb0 calls it once per `Scenario.encounters` entry.
* `squad_remove_actor` @0x436620 and its partner 0x436770 remove/add an actor from an
  **encounter** member list, adjusting the per-squad and per-platoon counters as a side
  effect.
* `squad_recent_object_*` @0x436b10 / 0x436b90 / 0x436c10 / 0x436c60 and
  `squad_despawn`-adjacent naming aside, these operate on the `"ai pursuit"` datum array at
  0x008802d0 hanging off `encounter.first_pursuit`.

### Library / other-module functions inside the address range

* 0x4052c0 `vector3d_cross_product`, 0x405320 `random_int_range`, 0x405360
  `float_compare_ascending`, 0x414910 (2D cone test), 0x43b2f0
  `path_find_closest_point_on_segment`, 0x43c340, 0x43c380, 0x43c400 -- plain math helpers
  that belong with `types/math.h`. No AI types defined for them.
* 0x41d7a0 `ai_target_distance_qsort_compare`, 0x42ac90 `ai_squad_priority_compare`,
  0x433c70 `object_sort_by_flag_then_distance` -- `qsort` comparators; the first two are the
  only evidence for the field they sort on and are cited above.
* 0x42c940 -- 1358 bytes, **zero callers** in this build. It bulk-resets fire-group
  bookkeeping across all actors and swarms. I used it for offset evidence only where another
  function agreed, since dead code can carry a stale layout.
* 0x41a2d0 is called once from `ai_initialize_for_new_map` and owns the three constant
  direction-sample tables at 0x00880380, 0x00880540 and 0x008805a0. Those are the only
  globals this module owns that are not datum arrays or `ai_globals`, and they are listed at
  the bottom of the header. The tables they are computed from (0x006556a0, 0x006556c8,
  0x006556ec, 0x00655714, 0x00655734, 0x0065573c) are `.rdata` constants and are *not*
  owned by this module.
* 0x40b080 (1767 bytes, vehicle seat detach/reseat) reads far more of `unit_data` than of
  `actor`; its unit-side offsets belong to `types/units.h` and were not re-derived here.
