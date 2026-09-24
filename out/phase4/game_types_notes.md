# game module type notes (types/game.h)

Module: `game`, 428 functions, 0x459300..0x551d30. Header checked with
`C:\msys64\ucrt64\bin\gcc.exe -fsyntax-only -I types out/phase4/game_smoke.c` (clean), and
again with every other header in `types/` included alongside it (clean, no name collisions).

The smoke file states each size for a 32-bit image and corrects it by
`(sizeof(void *) - 4) * <pointer count>`, because the only compiler available here is 64-bit
and several of these structs hold real pointers.

---

## Anchors taken from the binary rather than the decompiler

| What | Where | Fixes |
|---|---|---|
| `players_initialize` disassembly, 0x4735b3..0x47365b | `objdump -d bin/halo.exe` | `mov ebx,0x200` / `mov ebx,0x40` before the two `game_state_new` calls give the `player` and `team` element sizes Ghidra drops (the stride is a register argument). The two following raw allocations give `player_globals` = 0x98 and `player_control_globals` = 0x50. |
| gametype pointer table at 0x00688308 | `objdump -s -j .data` | Rows at 0x00687d20 "ctf" (1), 0x006880f8 "slayer" (2), 0x00688258 "oddball" (3), 0x00687ec8 "king" (4), 0x006881a8 "race" (5), plus 0x00688048 "stub" (6). Rows are 0xb0 apart, which is the size of `game_engine_definition`; each row's own `+0x04` word equals its table index, which is what confirms the `game_engine_index` enum values. |
| `types/tags.h` | existing header | `ScenarioNetgameFlags` 0x94 (type +0x10, usage_id +0x12), `ScenarioNetgameEquipment` 0x90, `ScenarioPlayerStartingLocation` 0x34, `ScenarioPlayerStartingProfile` 0x68, `GlobalsMultiplayerInformation` sounds reflexive at +0x5c/+0x60 with `GlobalsSound` stride 0x10. Every stride the module multiplies by matches one of these, so no scenario-side struct is re-derived here. |
| `types/units.h` `unit_control_data` | existing header | The bit-to-field mapping of `player_action_flags`. |

---

## Struct by struct: which functions established which fields

### `game_time_globals` (0x20, pointer at 0x006f1d6c)
- `game_engine_allocate_tick_record` 0x470a80 — allocates exactly 0x20 and zeroes all eight dwords, so the size is exact.
- 0x470ae0 — writes `speed` (+0x18) = 1.0, `leftover_time` (+0x1c) = 0, `active` (+0x01) = 1.
- `game_engine_advance_simulation_ticks` 0x470bf0 — increments `game_time` (+0x0c) and `elapsed_ticks` (+0x14) once per tick, stores `ticks_this_frame` (+0x10), gates on `active`.
- 0x470b30 — the fixed-timestep accumulator: `(delta + leftover) * speed * 30`, remainder back into +0x1c.
- `game_engine_get_current_tick` 0x470cd0 / `game_engine_get_time_scale` 0x470ce0.
- A grep of the whole decompiled image shows only 0x01, 0x02, 0x0c, 0x10, 0x14, 0x18, 0x1c are ever touched. **Unresolved: 0x00 and 0x04..0x0b.**

### `game_variant` (0x98)
- Size: every `game_engine_variant_defaults_*` function (0x463c40..0x467cf0, 33 of them) builds one on a 76-uint16 stack buffer and copies `0x26` dwords out; `game_engine_get_variant_by_name` 0x4622d0 declares the same buffer; the network session state reserves 0x98 at +0x10c.
- `game_variant_sanitize_options` 0x466730 is the field dictionary: 0x30 clamped 1..5, 0x34/0x40 normalized to bool, 0x44/0x48/0x4c/0x50 clamped >= 0, 0x54 clamped 0.25..4.0, 0x5c clamped 0..0xd, 0x60 low nibble clamped 0..8, and 0x7c..0x80 only touched when the engine index is 1 (ctf) or 2 (slayer).
- `game_engine_load_from_variant` 0x45c2c0 block-copies the variant into the live copy at 0x006f1c88 and then uses `variant[0xc]` (i.e. +0x30) to index the gametype table.
- Use-site names: +0x34 is what `game_engine_get_teams_enabled` returns (0x006f1cbc); +0x44/+0x48/+0x4c/+0x70 are the respawn-growth / respawn-time / suicide-penalty / betrayal-penalty values `game_engine_on_player_death` 0x460200 applies; +0x50 is the lives-per-round value 0x460f30 and 0x45cc30 compare `player::deaths` against.
- The variant name at 0x00..0x2f is zero in all 33 built-ins and is never read by this module. **Unresolved: 0x3c, 0x40, 0x58, 0x64, 0x6c, 0x74, 0x78, 0x84, 0x88, 0x8c, 0x90, 0x94.** 0x60 and 0x64 are packed 3-bit-per-slot tables (every built-in stores 0x249240, i.e. slots 2..7 set to 1); the meaning of the slots is not recovered.

### `game_engine_definition` (0xb0)
- Size and the first two fields come from .data (above).
- The callback slot names were produced mechanically: a script scanned every
  `(**(code **)(DAT_006f1d20 + N))()` in `out/halo_decompiled.c` and attributed it to its
  enclosing function. Slots with a single caller get that caller's role as the name.
- **Unresolved slots (no call site anywhere in the image): 0x1c, 0x24, 0x28, 0x2c, 0x30, 0x40, 0x48, 0x70, 0x7c, 0x9c, 0xa4, 0xa8.** Slots 0x60, 0x64 and 0xa0 are called only from the units module (0x56da00, 0x5674a0, 0x577e40) and 0x68 only from the head of `game_engine_on_player_death`; those four are named `unknown_XX` because their argument lists are not recoverable from one call site.

### `player` (0x200)
- The two constructors are the backbone: 0x473780 (network-replicated player, the fuller of the two) and 0x473940 (locally created player). Between them they write 0x02, 0x04, 0x1a, 0x1c, 0x20, 0x24, 0x28, 0x34, 0x38, 0x3c, 0x40, 0x48 (a `rep movsd` of 8 dwords from the caller's identifier record), 0x6c, 0x8c, 0xd0, 0xd5, 0xdc, 0xe0, 0xe4, 0xe8, 0xec, 0xf0..0x120, 0x104..0x12c, 0x15c, 0x160, 0x164..0x16c, 0x188, 0x18c, 0x190..0x1d0, 0x1e8..0x1f8.
- The three embedded queues are pinned by `lea esi,[ebp+N]` in the disassembly of 0x473940:
  `+0x120` player_update_queue_create, `+0x170` position_update_queue_create,
  `+0x1d0` vehicle_update_queue_create. Each queue's own size makes it end exactly where the
  next initialized field starts (0x15c, 0x188, 0x1e8), which is strong corroboration.
- `player_set_pending_interaction_action` 0x478e00 — 0x24 interaction object, 0x28 priority type (0xb clears), 0x2a seat.
- `game_engine_on_player_death` 0x460200 — 0x2c respawn timer, 0x30 respawn growth, 0x84 last death tick, 0xc0 betrayal penalty count.
- `game_engine_attribute_player_death` 0x46ff00 — the whole statistics block. The victim gets `deaths` (0xae) and, when the killer is itself, `suicides` (0xb0), and has `killing_spree_count` (0x96), `multikill_count` (0x98) and `last_kill_tick` (0x9a) reset. The killer gets `kills` (0x9c) on the enemy path and `betrayals` (0xac) plus 0xc0 on the friendly path (the branch is `teams_are_enemies`, 0x45bd50). Every surviving entry of the victim's four-slot recent-damager list (unit+0x43c, `unit_recent_damage` in types/units.h) gets `assists` (0xa4).
- `player_profile_cache` capture/apply pair 0x466ee0 / 0x466d00 — proves 0x9c..0xb2 is one contiguous statistics run and adds 0x6c, 0x88, 0x8c, 0xc4, 0xc8.
- `local_player_set_controlled_unit` 0x474fc0 / 0x474e10 — 0x34 unit, 0x38 previous unit, 0xd5 marked-for-deletion.
- 0x479ba0 / 0x479ca0 / 0x479d10 — 0x68 and 0x6a are a two-entry kill-streak countdown; slot 0 also sets object flag 0x10 and stamps the streak method into unit+0x422.
- 0x479eb0 — 0xe0/0xe4 are the multikill medal counter and its window timer (thresholds at 0x006894a4 and 0x0069956c).
- **Unresolved player offsets:** 0x1c, 0x3e, 0x44 (paired with 0x40 by the camera-observer code), 0x60, 0x64, 0x70/0x74/0x78/0x7c (all four seeded to -1 by 0x45c440 and only read again inside the HUD nameplate and objective code), 0x80, 0x88, 0x8d..0x95, 0x9e, 0xa0, 0xa6, 0xa8, 0xb2, 0xb4 (teleporter), 0xb8..0xbf, 0xc2, 0xca..0xd4, 0xd6..0xdb, 0xe8, 0xec, 0xf0..0x11f (the network state the client/server update code owns), 0x15c, 0x160, 0x164..0x16c, 0x188, 0x18c, 0x190..0x1cf, 0x1e8..0x1ff.
- The 0x20-byte block at 0x48 is copied wholesale from the caller's player-identifier record. The first 0x18 of it is a second copy of the UTF-16 name; `team_index` (0x66) and `team_index_desired` (0x67) live in its tail and are the two bytes 0x45c440 reads and rewrites. **The remaining 6 bytes (0x60..0x66) are unresolved.**

### `team` (0x40)
Allocated by `players_initialize` and emptied by `players_dispose`, and those are the only two references to 0x0087a47c in the entire image. **No function in this build ever creates or reads a team datum**, so only the size is known. It is declared opaque on purpose.

### `player_globals` (0x98)
- `players_initialize` seeds +0x00/+0x04 = -1 and +0x0c = 0; `players_dispose` zeroes all 0x26 dwords and re-seeds +0x00/+0x04/+0x08 = -1, +0x0e = 0, +0x10 = 0, +0x11 = 0, +0x12 = -1, +0x14 = 0.
- `local_player_to_player_index` 0x474d30 and `game_set_local_player` 0x474d50 index `base + 4 + local * 4` with a `local < 1` bound — hence one local-player slot.
- +0x08 is indexed the same way and is written with `player->unit`, then dereferenced through the object header array, so it is the local player's controlled unit.
- 0x474e10 sets +0x10 to 1 and clears it again if any player still has a unit.
- **Unresolved: 0x0c, 0x11, 0x12, 0x16 and the whole 0x17..0x98 tail** (zeroed on dispose, never read).

### `local_player_control` (0x40) and `player_control_globals` (0x50)
- `game_engine_init_player_look_state_from_object` 0x470e80 zeroes 0x10 dwords and then writes every default, which is what fixes the record size; `game_engine_reset_player_look_state` 0x470de0 does the same through the header, which is what fixes the 0x10-byte header.
- 0x0c/0x10 are yaw/pitch, built with `fpatan` from the unit's facing vector; 0x38/0x3c are the pitch clamps (-/+1.4906585, i.e. +/-85.4 degrees).
- 0x20/0x22/0x24 are seeded from unit+0x2f4 / +0x31d / +0x321, which `types/units.h` already names `desired_weapon_index` / `desired_grenade_index` / `desired_zoom_level`.
- `player_control_globals::action_flags` and the two shadow words are built by 0x472760 out of a `unit_control_data`; the bit table in the header is that function read straight through.
- **Unresolved: record 0x04, 0x08, 0x0a, 0x14, 0x18, 0x1c, 0x26, 0x28, 0x2c, 0x30, 0x34; header 0x0c.** The exact roles of `action_flags_latched` (0x04) and `action_flags_edge` (0x08) are inferred from how 0x472760 tests one and rewrites the other, and are marked as such.

### `player_profile` (0x30) and the 16-slot cache
`player_profile_cache_initialize` 0x466c20 zeroes 0xc0 dwords at 0x006b0b88 and then walks the same region with a 0x30 stride up to 0x006b0e88, which gives 16 x 0x30 exactly. The field list is the capture/apply pair 0x466ee0 / 0x466d00. Note that +0x1e is genuinely misaligned (an int32 packed straight after three int16s) — that is what the binary does, and the smoke file asserts it.

### `team_pair_override` (0x12) / `team_pair_globals` (0xb4)
0x45bc30 allocates 0x2d dwords. 0x45be50 writes every field of one entry, 0x45bcf0 ticks 0x0e/0x10, 0x45be00 reads 0x0b. The two 100-bit maps are indexed `(a * 10 + b)` and `(b * 10 + a)` and therefore occupy four uint32 each; 0x02 + 8*0x12 + 2 pad + 0x10 + 0x10 = 0xb4 accounts for every byte. **Unresolved: entry 0x08, 0x09, 0x0c, and what distinguishes the two bitmaps** (0xa4 is the one `teams_are_enemies` 0x45bd50 inverts; 0x94 is a second per-pair flag read by 0x45bdb0).

### `scoreboard_entry` (0x1c)
`game_engine_build_sorted_player_list` 0x45cc90 hands qsort an element width of 0x1c and then compares dwords 2..5 of adjacent entries to detect ties, writing the place into dword 6 with bit 0x80000000 set on a tie. The key builder is 0x45cc30 (score clamped at -1000, biased +1000, plus bit 0x40000000 for "still has lives" and 0x20000000 for "not marked for deletion"). **Unresolved: dword 1 (0x04), which nothing compares.**

### `custom_waypoint` (0x20)
0x462260 (register), 0x462230 (read position), 0x4620c0 (filter) and 0x462190 (collect) all index `slot * 8` over a uint32 array. The 32 slots run 0x006f1888..0x006f1c88 and butt directly against the live `game_variant` copy, which corroborates both the count and the stride. -1 in `player`, `team` or `owner` is a wildcard.

### `multiplayer_sound_request` (0x10)
`game_engine_queue_multiplayer_sound` 0x46be40 refuses a sixth entry and writes four fields at `count * 0x10`; the tick handler 0x46bd80 shifts the array down by one record. 0x006b10f0 + 5*0x10 == 0x006b1140, the count global, which closes the block.

### `circular_queue` (0x18), `player_update_queue` (0x3c), `update_record` (0x308), `update_server_queue` (0x64)
- The three constructors 0x47a020 (30 x 0x14), 0x47a250 (30 x 0x48) and 0x479f40 (120 x 0x2c) write the same six fields through `unaff_ESI`, and `circular_queue_push/pop/count` (0x47a1a0/0x47a200/0x47a230) read them back. The pointer table at +0x08 holds one pointer per record into the +0x14 storage, so records never move.
- The larger 0x479f40 form adds a latched-record tail; 0x479fb0 decrements the record's refcount at +0x04, pops it and copies its dwords 3..10 into the tail. Placed at player+0x120 it ends exactly at player+0x15c.
- Both update rings use a 0xc2-dword (0x308) stride: `update_client_queue_get_slot` 0x473500 computes `&DAT_006f7ed4 + (tick & 0x7f) * 0xc2`, and 0x472cc0 computes `&DAT_006f1d94 + (tick & 0x1f) * 0xc2`. 32 * 0x308 and 128 * 0x308 match the 0x6100/0x18400-byte memsets in `update_server_new`/`update_client_new` exactly. **The record body is unresolved** — it is the packed per-player payload the network module encodes.
- `update_server_queue` is the element of the "update server queues" data_array (16 x 0x64, stride read off `(index * 100) + data`); 0x472c90 creates the datum and immediately calls the 0x3c-byte queue constructor on it, and every ring access in 0x472cc0 is at base+0x28..+0x3c, which places the queue at 0x28 and fills the record. **Unresolved: 0x02..0x27.**

### `observer_target_candidate` (0x38) / `observer_target_cone` (0x10)
`camera_observer_find_best_target` 0x459a00 declares a 0xe00-byte stack array and qsorts it with an element width of 0x38 (so 64 slots); 0x459b10 fills one slot field by field and 0x45a4a0 is the comparator. The cone is the four floats 0x459b10 reads out of its `param_1`; passing NULL zeroes both weights.

### `ctf_globals` (0x148) / `king_globals` / `king_hill_marker_history`
- `game_engine_ctf_initialize_flags` 0x46d890 zeroes 0x52 dwords at both 0x006b1290 (live) and 0x0087a520 (replicated), which is what gives 0x148. The first dword is a bitmask over netgame-flag usage ids (assigned uniquely in 0..31 by 0x46d800), and the sixteen dwords after it are seeded either with the lowest usage id on the map or with -1 depending on `game_variant::ctf_option_7c`.
- **Everything past 0x44 in `ctf_globals` is unresolved.** 0x46ec10 serializes per-team assignments and captured bitmasks out of it and 0x006b1314 is the neutral-flag id, so at least those two live somewhere in the tail, but the individual offsets were not pinned.
- `king_globals` is the contiguous triple 0x006b1050/0x54/0x58 written as a set by `game_engine_koth_update_hill_occupancy_state` 0x46acb0; the state values are the five constants that function stores.
- `king_hill_marker_history` is the 0x40 bytes 0x46b250 reseeds with four copies of the current hill position plus four zeroed state slots.

### save games
- Record size 0x206 is proved three ways: 0x53e0e0 seeks to `slot * 0x206`, 0x53e300 compares against `slot * 0x206 + 0x206`, and 0x53e420 divides the file size by 0x206. **The record body is entirely unresolved** — no function in this module reads a field inside it.
- All four index functions rebuild the same `file_reference` in place at 0x00721330 with a 0x43-dword memset, which matches the 0x10c-byte `file_reference` already declared in `types/hs.h`.
- `user_save_path_table` comes from `user_save_path_register` 0x551650 / `_lookup` 0x5516a0 / `_remove` 0x5516d0; 0x00721f30 + 8 * 0x105 == 0x00722758 closes the pair of arrays.

---

## Misattributed / mis-named functions

Nothing in the 428 turned out to belong to a different module in a way that made its types
un-writable, but several names in `out/phase4/game_functions.md` are wrong and the header
does not follow them:

1. **0x4726b0 `unit_get_local_player_weapon_index`** — it is *not* a weapon lookup. The
   disassembly is `[unit+0x218] -> player handle`, then `mov ax, WORD [player + 2]`, i.e. it
   returns `player::local_player_index`. The correct name is roughly
   `unit_get_local_player_index`.
2. **0x4726f0 / 0x472740** — described as reading and invalidating a "weapon-index salt".
   Both touch `local_player_control + 0x24`, which
   `game_engine_init_player_look_state_from_object` seeds from `unit+0x321`
   (`desired_zoom_level`). They are the zoom-level accessor pair, not weapon bookkeeping.
   0x472100 is the weapon one (it writes `+0x20`).
3. **0x461080 `game_engine_find_valid_starting_locations`** — it walks
   `scenario + 0x378 / 0x37c` with a 0x94 stride, which is `netgame_flags`, not
   `player_starting_locations` (those are at +0x354 with a 0x34 stride, and 0x477640 is the
   function that indexes them). Its two "team"/"type" parameters actually compare against
   `ScenarioNetgameFlags::type` (+0x10) and `::usage_id` (+0x12) respectively — i.e. they are
   swapped relative to the current signature.
4. **0x006f1d20** is named `game_is_server` in `types/objects.h`/`types/units.h` and
   "the network / predicted-state flag" in several notes files. It is
   `game_engine_definition *current_game_engine` — `game_engine_load_from_variant` assigns it
   from the 0x00688308 gametype table and `game_engine_unload` calls its `+0x08` dispose slot.
   The existing tests still work because "non-NULL" does mean "a multiplayer engine is
   loaded", but the type is wrong and any decompilation that treats it as an int will hide
   the ~30 vtable dispatches through it.
5. **`types/hs.h` `hs_game_time_globals::seconds_per_tick` (+0x1c)** is very likely wrong.
   0x470b30 uses +0x1c as the fractional-tick accumulator carried between frames and +0x18 as
   the time scale (`game_engine_get_time_scale` returns +0x18). `types/units.h` says 0x55a170
   multiplies +0x1c by 29.999998 to turn a per-second rate into a per-tick step, which is the
   wrong direction for a seconds-per-tick value. Left alone here; `types/game.h` declares the
   same 0x20 bytes under the name `game_time_globals` with the roles the game module's own
   code demonstrates, and says so in its header comment.
6. **0x45b4cb / 0x45b4e4** (`game_initialize_mod_per_map_upgrade_effects` /
   `..._locations`) are a 22-byte fragment and a 1-byte stub that overlap the end of
   0x45b370. They carry no types.
7. **0x45fc50 `hud_render_scoreboard_ingame`** (45 bytes) is a tail fragment of the
   netgame-equipment respawn tick, and **0x476847
   `player_add_equipment_unit_grenade_count_mod`** is a duplicated tail of
   0x476760. Neither introduces a type.
8. **0x53e060..0x53e630 and 0x551620..0x551d30** are the save-game directory / index helpers.
   They are assigned to `game` in `modules.json` on CEA string evidence
   (`XCreateSaveGame`, `XDeleteSaveGame`, `find_files_start`), but they are pure file-system
   plumbing with no game-layer state and would sit at least as comfortably in
   `saved_games`. Their types are included here because they are in this module's function
   list; if the module boundary is ever redrawn, `savegame_index_record` and
   `user_save_path_table` should move with them.

## Globals this module owns but whose type is still unknown

`0x00871de0` (43 references, always as a bare value — most likely the shared 16-bit HUD text
drawing state), `0x006b0f50` (starting-location result count), `0x006b1458` (a gate
`game_engine_attribute_player_death` returns on), `0x0087abe0`/`0x0087abdc`, and the
0x006b0e88..0x006b1050 run between the profile cache and the King globals.
