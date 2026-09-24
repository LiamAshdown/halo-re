# `game` — players, the simulation clock, and the multiplayer game engine

Retail Halo PC `halo.exe` 1.0.10, module range `0x459300 .. 0x551d30` (428 functions),
plain C / MSVC 7.1 / x86. Every file in this directory is one function, rewritten from its
Ghidra decompilation against `types/game.h`, with the original decompile preserved verbatim at
the bottom of the file inside `#if 0 ... #endif` for diffing.

**This directory covers the whole module: 422 files for the 428 addresses `modules.json`
assigns to `game`.** The six without a file are all decompiler fragments, not functions — see
**Misattributed / fragment functions** below.

Gate: `python tools/build_check.py game` → **422 ok, 0 failed**
(full repo: `python tools/build_check.py` → **1389 ok, 0 failed**).

Mean self-reported rewrite confidence is **0.48**; 107 of the 422 files sit below 0.35 and should
be hooked before they are trusted. Every file's number is in the **Functions** table at the end.

## What the module contains

| Family | Range | What it is |
|---|---|---|
| camera observer / autoaim | `0x459300`–`0x45a4ff` | picks the object the camera or the aim assist should track: build a cone, collect candidates per BSP cluster, score, sort, line-of-sight filter |
| debug spawn cheats | `0x45a530`–`0x45a8ff` | `cheat_all_weapons`, `cheat_spawn_warthog`, `cheat_teleport_to_camera`, invincibility |
| game lifecycle | `0x45a9c0`–`0x45b4ff` | `game_initialize` / `game_dispose`, map load and unload, `game_start_new_map` / `game_stop_current_map`, per-tick effect updates |
| tick + network dispatch | `0x45b590`–`0x45b7ff` | the per-tick driver (`game_simulate_tick`) and the server object-type change broadcast |
| variants and teams | `0x45b8b0`–`0x45c2ff` | install a `game_variant`, reset players, the `team_pair_globals` enemy matrix, save/pause safety checks |
| game engine plumbing | `0x45c300`–`0x45cc8f` | load/unload a gametype, per-player life and object-change notifications, BSP-switch readiness, the time-remaining announcer |
| scoreboard | `0x45cbc0`–`0x45d6ff` | sort keys, comparators, the sorted player list, place/rank, on-screen selection, post-game result text |
| HUD nameplates and kill feed | `0x45d700`–`0x45f2ff` | post-game rasterize, teammate nameplates, kill-feed message text |
| netgame equipment | `0x45f320`–`0x45ff2f` | dropped-object cleanup, item scale/pickup, weighted respawn of scenario netgame equipment, end-of-game sequencing |
| engine tick / death | `0x45ff30`–`0x4607ff` | `game_engine_tick` and `game_engine_on_player_death` |
| kill-feed dispatch | `0x460890`–`0x460fff` | `chimera__kill_feed`, the four broadcast helpers, the network event handler, the message-delta encoders |
| respawn / starting locations | `0x461000`–`0x4614ff` | elimination and respawn-priority gates, netgame-flag starting-location search |
| teleporters and waypoints | `0x461630`–`0x4622cf` | `game_engine_update_teleporter`, the 32-slot custom-waypoint table, navpoint collection |
| variant lookup and history | `0x4622d0`–`0x463bff` | `game_engine_get_variant_by_name` (a 38-way name dispatch), the variant history cache, the placement-validation no-ops |
| built-in gametype defaults | `0x463c40`–`0x4652ff` | the 27 `game_engine_variant_defaults_classic_*` option blocks |
| in-game scoreboard rasterizer | `0x465300`–`0x466bff` | `game_engine_rasterize_in_game_score` (the largest function in the module), the world-relative text wrapper, the multiplayer string table |
| player profile cache | `0x466c20`–`0x466fff` | the 16-slot `player_profile` cache and its capture/apply pair |
| profile / end-game messages | `0x467010`–`0x46726f` | the `message_delta_encode_message` senders for profile updates and end-of-game notifications |
| non-classic gametype defaults | `0x467270`–`0x467f70` | the 11 plain `game_engine_variant_defaults_*` builtins plus `game_variant_sanitize_options` |
| stray-object cleanup and CTF flags | `0x468010`–`0x469fff` | per-tick removal of dropped items and projectiles, `ctf_engine_flag_tick`, flag creation / return / capture, the per-team flag state at `0x006b0e88` and `ctf_globals` |
| king of the hill | `0x46a240`–`0x46bdff` | hill placement and relocation, the occupancy state machine, the hill boundary fence and marker geometry the rasterizer draws, the pulse icons, per-player scoring |
| multiplayer sound queue | `0x46bd80`–`0x46bfff` | the five-slot `multiplayer_sound_request` ring, the announcement duration lookup, the KOTH marker-position search |
| oddball / race / bucket scoring | `0x46c000`–`0x46efdf` | ball idle and carrier ticks, race checkpoint order, the per-bucket score aggregation and end-of-round trigger, CTF broadcast and serialization |
| death attribution and statistics | `0x46ef70`–`0x470700` | `game_engine_attribute_player_death` (kills, assists, betrayals, suicides, sprees, multikills), team allegiance messages, the score totals and team-leading queries |
| the simulation clock | `0x470a80`–`0x470d40` | `game_time_globals`: the tick-record allocator, the fixed-timestep accumulator, `game_engine_advance_simulation_ticks`, `game_engine_get_current_tick` / `_get_time_scale` |
| player control input | `0x470d40`–`0x472760` | the response curve, `local_player_control` init and reset, `game_engine_build_local_player_control_input`, `game_engine_update_local_player_control`, the look update and its vehicle-seat clamps, the zoom / weapon / grenade accessors, `game_engine_digitize_control_input` |
| client / server update queues | `0x472aa0`–`0x473560` | `update_server_*` and `update_client_*`: the 16 per-machine `update_server_queue` data_array, the 32- and 128-entry `update_record` rings, tick staging, catch-up and distribution, `game_engine_find_player_by_name`, `random_get_table_point` |
| players and the player datum | `0x4735b0`–`0x474580` | `players_initialize` / `players_dispose`, the two `player` constructors (network and local), `player_delete` / `player_remove`, the starting-profile applier, the view-forward vector, BSP reattachment |
| the per-tick players update | `0x4740a0`–`0x474f40` | `game_engine_players_update_server` / `_client` (the two biggest and least-verified functions in the range), `main_switch_structure_bsp`, `local_player_to_player_index`, `game_set_local_player` |
| unit binding and respawn | `0x474fc0`–`0x477810` | `local_player_set_controlled_unit`, `player_reset_after_unit_change`, `player_attach_unit_to_parent`, `player_kill_and_release_unit`, `player_find_placement_position`, `player_respawn`, and the starting-location scorer chain (`0x461ad0`/`0x461c60`/`0x461d90`, renamed by this pass) |
| remote position updates | `0x476cf0`–`0x4776a0` | the server-side position broadcast, the client catch-up walk, and the two appliers for remote on-foot and remote vehicle position records |
| netgame message handlers | `0x4778c0`–`0x477e90` | join / team, spawn loadout, object-value, interaction and kill-streak message decoders, all built on the `0x4ec590` / `0x4ec670` message-delta envelope pair |
| nearby interactions | `0x477ea0`–`0x479930` | the two per-player interaction scans, vehicle boarding and assassination checks, the pending-action queue and its executor, weapon swap and drop |
| screen effects and streaks | `0x479940`–`0x479f30` | the shield-recharge / kill-streak / full-health / camo / health-pack screen effects, the kill-streak trio and the multikill medal advance |
| network update queues | `0x479f40`–`0x47a2c0` | `circular_queue` and `player_update_queue` construction, push / pop / count, and the position and vehicle record rings embedded in every `player` |
| save games | `0x53e060`–`0x551d30` | the flat 0x206-byte save index file (exists / read / write / append / count / remove), the signed-in-profile check, the 8-slot `user_save_path` table, `XCreateSaveGame` / `XDeleteSaveGame`, and the save-directory enumeration pair |

Everything here depends on `types/memory.h` (`data_array`, `datum_index`, `data_iterator`),
`types/math.h`, `types/objects.h`, `types/units.h`, `types/items.h`, `types/cache.h` and
`types/tags.h`; none of those types is redefined.

## Struct layouts

Every struct `types/game.h` declares, field by field, generated from that header so the two cannot
drift. Offsets are byte offsets from the struct base and are the layout the binary itself carries
(see the header's own preamble for which facts came from `objdump` rather than from the
decompiler). The `game_constants`, `game_engine_index`, `game_engine_state`, `player_action_flags`
and `king_hill_state` enums are not repeated here; read them in the header.

#### `game_time_globals` size 0x20

| off | field | notes |
|---|---|---|
| `0x00` | `uint8_t unknown_00` | never read or written |
| `0x01` | `uint8_t active` | 0x470ae0 sets it; advance_simulation_ticks refuses to run any tick while it is zero |
| `0x02` | `uint8_t paused` | units / hs read it as a "time is running" gate, and both game_engine_build_local_player_control_input (0x4710b0) and game_engine_update_local_player_control (0x471ae0) refuse to turn stick input into look deltas, or to cycle the weapon / grenade / zoom level, while it is set |
| `0x03` | `uint8_t unknown_03[0x0c - 0x03]` |  |
| `0x0c` | `int32_t game_time` | the simulation tick counter, incremented once per tick |
| `0x10` | `int16_t ticks_this_frame` | how many ticks advance_simulation_ticks just ran |
| `0x12` | `int16_t unknown_12` |  |
| `0x14` | `int32_t elapsed_ticks` | second, never-reset counter bumped beside game_time |
| `0x18` | `float speed` | time scale; forced to 1.0 in any networked game |
| `0x1c` | `float leftover_time` | fractional-tick accumulator carried between frames |

#### `game_variant` size 0x98

| off | field | notes |
|---|---|---|
| `0x00` | `uint16_t name[24]` | UTF-16 display name; all of it zero in the built-ins |
| `0x30` | `int32_t game_engine_index` | sanitize clamps to 1..5, see game_engine_index |
| `0x34` | `uint8_t teams` | sanitize normalizes to 0/1; game_engine_get_teams_enabled |
| `0x35` | `uint8_t pad_35[3]` |  |
| `0x38` | `uint32_t flags` | option bitfield; slayer forces bits 0 and 8 on |
| `0x3c` | `int32_t unknown_3c` |  |
| `0x40` | `uint8_t unknown_40` | sanitize normalizes to 0/1 |
| `0x41` | `uint8_t pad_41[3]` |  |
| `0x44` | `int32_t respawn_time_growth` | clamped >= 0; on_player_death adds it to 0x30 and caps the total at 5x this value |
| `0x48` | `int32_t respawn_time` | clamped >= 0; the base respawn countdown in ticks |
| `0x4c` | `int32_t suicide_penalty` | clamped >= 0; added when the killer is the victim |
| `0x50` | `int32_t lives_per_round` | clamped >= 0; 0 means unlimited. A player whose death count (player+0xae) reaches it is eliminated |
| `0x54` | `float speed_scale` | clamped to 0.25 .. 4.0 |
| `0x58` | `int32_t score_limit` |  |
| `0x5c` | `int32_t starting_equipment` | clamped to 0 .. 0xd |
| `0x60` | `uint32_t vehicle_set` | low nibble clamped to 0 .. 8; the upper bits are a packed 3-bit-per-slot table (the built-ins store , i.e. every slot from index 2 up set to 1) |
| `0x64` | `uint32_t unknown_64` | same packed 3-bit encoding as 0x60 |
| `0x68` | `int32_t time_limit` | in ticks (slayer default 0x708 == 60 s * 30) |
| `0x6c` | `uint8_t unknown_6c` |  |
| `0x6d` | `uint8_t pad_6d[3]` |  |
| `0x70` | `int32_t betrayal_penalty` | on_player_death multiplies it by player+0xc0 |
| `0x74` | `uint8_t unknown_74` |  |
| `0x75` | `uint8_t pad_75[3]` |  |
| `0x78` | `int32_t unknown_78` | slayer default 36000 ticks (20 minutes) |
| `0x7c` | `uint8_t ctf_option_7c` | the four bytes 0x7c..0x7f are only normalized when |
| `0x7d` | `uint8_t ctf_option_7d` | game_engine_index is 1 (ctf); 0x7f is skipped when the |
| `0x7e` | `uint8_t ctf_option_7e` | index is 2 (slayer), which also normalizes 0x7c..0x7e |
| `0x7f` | `uint8_t ctf_option_7f` |  |
| `0x80` | `int32_t ctf_value_80` | clamped >= 0 for game_engine_index 1 |
| `0x84` | `int32_t unknown_84` |  |
| `0x88` | `int32_t unknown_88` |  |
| `0x8c` | `int32_t unknown_8c` |  |
| `0x90` | `int32_t unknown_90` |  |
| `0x94` | `int16_t unknown_94` | every built-in writes 1 |
| `0x96` | `int16_t unknown_96` |  |

#### `game_engine_definition` size 0xb0

| off | field | notes |
|---|---|---|
| `0x00` | `char *name` | "ctf" / "slayer" / "oddball" / "king" / "race" |
| `0x04` | `int32_t index` | matches its slot in the 0x00688308 table |
| `0x08` | `void *dispose` | game_engine_unload |
| `0x0c` | `void *initialize_for_new_game` | 0x45c370; returns false to abort the start |
| `0x10` | `void *dispose_from_old_game` | 0x45b370 (game shutdown) |
| `0x14` | `void *player_new_life` | 0x45c440, after the per-life reset |
| `0x18` | `void *player_changed_object` | 0x45c570 |
| `0x1c` | `void *unknown_1c` |  |
| `0x20` | `void *reset_round` | 0x45b8b0 |
| `0x24` | `void *unknown_24` |  |
| `0x28` | `void *unknown_28` |  |
| `0x2c` | `void *unknown_2c` |  |
| `0x30` | `void *unknown_30` |  |
| `0x34` | `void *post_rasterize` | render_scene_draw (0x50bfb0) |
| `0x38` | `void *update` | game_engine_tick |
| `0x3c` | `void *object_in_play_update` | 0x45f560 (per-tick pickup bookkeeping) |
| `0x40` | `void *unknown_40` |  |
| `0x44` | `void *object_expired` | 0x45f510 (unclaimed item about to despawn) |
| `0x48` | `void *unknown_48` |  |
| `0x4c` | `void *get_score` | 0x463480 / kill-feed builder; takes a player handle (or -1) and returns its score |
| `0x50` | `void *get_team_score` | called with 0 and 1 |
| `0x54` | `void *unknown_54_build_player_text` | (player, wchar buffer) -> scoreboard row text |
| `0x58` | `void *build_score_header_text` | (wchar buffer) |
| `0x5c` | `void *build_team_score_text` | (team, wchar buffer) |
| `0x60` | `void *unknown_60` | reached from the units module (0x56da00) |
| `0x64` | `void *unknown_64` | reached from the units module (0x5674a0) |
| `0x68` | `void *unknown_68` | fired first thing in game_engine_on_player_death |
| `0x6c` | `void *build_message_text` | variant override for the kill-feed text builder |
| `0x70` | `void *unknown_70` |  |
| `0x74` | `void *player_team_changed` | 0x4611b0 |
| `0x78` | `void *allow_grenade_counts` | game_engine_apply_player_grenade_counts |
| `0x7c` | `void *unknown_7c` |  |
| `0x80` | `void *waypoint_filter` | 0x4620c0, per custom-waypoint slot |
| `0x84` | `void *unknown_84` | called with 0 or 1; gates damage scaling and one kill-feed message class |
| `0x88` | `void *time_scale_override` | 0x461550 |
| `0x8c` | `void *is_winner` | 0x463730 / 0x45cf30 |
| `0x90` | `void *profiles_updated` | 0x466cb0 and the network layer (0x4dfa10) |
| `0x94` | `void *profile_post_update` | 0x466e60 |
| `0x98` | `void *player_round_reset` | 0x463620 |
| `0x9c` | `void *unknown_9c` |  |
| `0xa0` | `void *unknown_a0` | reached from the units module (0x577e40) |
| `0xa4` | `void *unknown_a4` |  |
| `0xa8` | `void *unknown_a8` |  |
| `0xac` | `void *reset_objects` | 0x468260 / 0x468320 |

#### `game_variant_history_entry` size 0xa4

| off | field | notes |
|---|---|---|
| `0x00` | `char *path` | freed alongside name; UNSURE which of the two is which |
| `0x04` | `char *name` | GlobalAlloc-ed copy of the variant name |
| `0x08` | `int32_t unknown_08` |  |
| `0x0c` | `game_variant options` |  |

#### `circular_queue` size 0x18

| off | field | notes |
|---|---|---|
| `0x00` | `int32_t capacity` | record count |
| `0x04` | `int32_t record_size` | bytes per record |
| `0x08` | `void **records` | capacity pointers into storage |
| `0x0c` | `int32_t write_index` |  |
| `0x10` | `int32_t read_index` | equal to write_index means empty |
| `0x14` | `void *storage` | capacity * record_size bytes |

#### `player_update_queue` size 0x3c

| off | field | notes |
|---|---|---|
| `0x00` | `circular_queue queue` | 120 records of 0x2c |
| `0x18` | `uint8_t has_current` | set once a record has been latched |
| `0x19` | `uint8_t pad_19[3]` |  |
| `0x1c` | `int32_t current[8]` | dwords 3..10 of the latched record |

#### `update_record` size 0x308 == k_update_record_size

| off | field | notes |
|---|---|---|
| `0x00` | `int32_t tick` |  |
| `0x04` | `uint16_t player_count` |  |
| `0x06` | `uint8_t body[0x308 - 0x06]` |  |

#### `update_server_queue` size 0x64

| off | field | notes |
|---|---|---|
| `0x00` | `int16_t identifier` | datum_header |
| `0x02` | `uint8_t unknown_02[0x28 - 0x02]` |  |
| `0x28` | `player_update_queue queue` |  |

#### `player` size 0x200 == k_player_size

| off | field | notes |
|---|---|---|
| `0x00` | `int16_t identifier` | datum_header salt |
| `0x02` | `int16_t local_player_index` | -1 unless this player is driven locally; game_set_local_player keeps it in sync with player_globals::local_players |
| `0x04` | `uint16_t name[12]` | UTF-16, _wcsncpy of 11 chars plus the NUL the constructors write at 0x1a |
| `0x1c` | `int32_t unknown_1c` | both constructors write -1 |
| `0x20` | `int32_t team` | 0..k_team_pair_index_count-1; both constructors seed it with 1, 0x45c440 recomputes it |
| `0x24` | `datum_index interaction_object` | board / swap / assassinate target |
| `0x28` | `int16_t interaction_type` | player_set_pending_interaction_action keeps the highest value; 0xb clears the slot |
| `0x2a` | `int16_t interaction_seat` | vehicle seat the pending action would use |
| `0x2c` | `int32_t respawn_timer` | ticks left; clamped to 0x5a..9000 on death |
| `0x30` | `int32_t respawn_time_growth` | accumulates game_variant::respawn_time_growth, capped at 5x it, and is refunded to the killer |
| `0x34` | `datum_index unit` | the object this player drives, -1 when dead |
| `0x38` | `datum_index previous_unit` | 0x474e10 rolls unit into it on every change |
| `0x3c` | `int16_t bsp_cluster` | constructors write -1 |
| `0x3e` | `int16_t unknown_3e` |  |
| `0x40` | `datum_index observer_target` | camera_observer_update (0x4593b0) result |
| `0x44` | `int32_t observer_state` | written beside 0x40 by the same function |
| `0x48` | `uint16_t identifier_name[12]` | second copy of the name: 0x473940 does a rep movsd of 8 dwords from the caller player-identifier record into 0x48 |
| `0x60` | `int32_t unknown_60` | tail of that same 0x20-byte copy |
| `0x64` | `int16_t unknown_64` |  |
| `0x66` | `int8_t team_index` | 0x45c440 assigns it round-robin in team games |
| `0x67` | `int8_t team_index_desired` | the requested team it is derived from |
| `0x68` | `int16_t kill_streak[2]` | slot 0 also sets object flag 0x10 and stamps the streak method into unit+0x422; 0x479d10 counts both down once per tick |
| `0x6c` | `float speed` | constructors write 1.0; part of the profile |
| `0x70` | `datum_index unknown_70` | 0x45c440 writes -1 |
| `0x74` | `datum_index unknown_74` | 0x45c440 writes -1 |
| `0x78` | `datum_index unknown_78` | 0x45c440 writes -1 |
| `0x7c` | `datum_index unknown_7c` | 0x45c440 writes -1 |
| `0x80` | `int32_t unknown_80` | read by the nameplate HUD (0x45e520) |
| `0x84` | `int32_t last_death_tick` | game_time when this player last died; the odd-man-out test orders players by it |
| `0x88` | `int32_t unknown_88` | part of the profile block |
| `0x8c` | `uint8_t odd_man_out` | cached result of 0x460e40 |
| `0x8d` | `uint8_t unknown_8d[0x96 - 0x8d]` | The statistics block. game_engine_attribute_player_death (0x46ff00) is the one function that writes all of it, and it separates the three roles cleanly: the victim gets deaths (and suicides when the killer is itself) and has its three streak fields reset, the killer gets kills or -- when teams_are_enemies says the pair is friendly -- betrayals, and every surviving recent damager gets assists. The profile cache mirrors ..0xb2 verbatim. |
| `0x96` | `int16_t killing_spree_count` | +1 per kill, zeroed on death |
| `0x98` | `int16_t multikill_count` | reset to 1 when the previous kill is more than ticks (4 s) old, else incremented |
| `0x9a` | `int16_t last_kill_tick` | game_time of the last kill, -1 on death |
| `0x9c` | `int16_t kills` |  |
| `0x9e` | `int16_t unknown_9e` |  |
| `0xa0` | `int32_t unknown_a0` |  |
| `0xa4` | `int16_t assists` | credited to each of the four recent damagers |
| `0xa6` | `int16_t unknown_a6` |  |
| `0xa8` | `int32_t unknown_a8` |  |
| `0xac` | `int16_t betrayals` | +1 when killer and victim are not enemies |
| `0xae` | `int16_t deaths` | compared against game_variant::lives_per_round to decide elimination |
| `0xb0` | `int16_t suicides` | +1 when the killer is the victim |
| `0xb2` | `int16_t unknown_b2` |  |
| `0xb4` | `int32_t unknown_b4` | game_engine_update_teleporter |
| `0xb8` | `uint8_t unknown_b8[0xc0 - 0xb8]` |  |
| `0xc0` | `int16_t betrayal_penalty_count` | +1 per betrayal; on_player_death scales it by game_variant::betrayal_penalty and clears it |
| `0xc2` | `int16_t unknown_c2` |  |
| `0xc4` | `int32_t objective_time` | hill / ball time in ticks. The profile cache divides it by 30 on the way out and multiplies it back on the way in when the engine is king |
| `0xc8` | `int16_t unknown_c8` | flag touches; also mirrored by the profile |
| `0xca` | `uint8_t unknown_ca[0xd0 - 0xca]` |  |
| `0xd0` | `datum_index unknown_d0` | constructors write -1 |
| `0xd4` | `uint8_t unknown_d4` |  |
| `0xd5` | `uint8_t marked_for_deletion` | 1 makes 0x474e10 call player_remove; every respawn / scoreboard path skips such a player |
| `0xd6` | `uint8_t unknown_d6[0xdc - 0xd6]` |  |
| `0xdc` | `int32_t unknown_dc` |  |
| `0xe0` | `int32_t medal_streak_count` | 0x479eb0 bumps it and fires the medal event once it reaches the threshold at 0x006894a4 |
| `0xe4` | `int32_t medal_streak_timer` | seeded negative from 0x0069956c; the streak is only extended while it is >= 0 |
| `0xe8` | `int32_t unknown_e8` |  |
| `0xec` | `datum_index unknown_ec` | 0x473940 writes -1 |
| `0xf0` | `int32_t unknown_f0` | start of a 0xc-dword run the local constructor |
| `0xf4` | `datum_index unknown_f4` | zeroes; the network constructor writes -1 here |
| `0xf8` | `int32_t unknown_f8` |  |
| `0xfc` | `uint8_t unknown_fc[0x104 - 0xfc]` |  |
| `0x104` | `int32_t unknown_104` | network constructor writes -1 |
| `0x108` | `uint8_t unknown_108` |  |
| `0x109` | `uint8_t pad_109[3]` |  |
| `0x10c` | `int32_t unknown_10c` |  |
| `0x110` | `int32_t unknown_110` |  |
| `0x114` | `int32_t unknown_114` |  |
| `0x118` | `int32_t unknown_118` |  |
| `0x11c` | `int32_t unknown_11c` |  |
| `0x120` | `player_update_queue update_history` | 120 records of 0x2c |
| `0x15c` | `datum_index unknown_15c` | local constructor writes -1 |
| `0x160` | `datum_index unknown_160` | local constructor writes -1 |
| `0x164` | `int32_t unknown_164` |  |
| `0x168` | `int32_t unknown_168` |  |
| `0x16c` | `int32_t unknown_16c` |  |
| `0x170` | `circular_queue position_updates` | 30 records of 0x14 |
| `0x188` | `int32_t unknown_188` |  |
| `0x18c` | `datum_index unknown_18c` | local constructor writes -1 |
| `0x190` | `uint8_t unknown_190[0x1d0 - 0x190]` | 0x10 dwords the local constructor zeroes |
| `0x1d0` | `circular_queue vehicle_updates` | 30 records of 0x48 |
| `0x1e8` | `int32_t unknown_1e8` |  |
| `0x1ec` | `int32_t unknown_1ec` |  |
| `0x1f0` | `int32_t unknown_1f0` |  |
| `0x1f4` | `int32_t unknown_1f4` |  |
| `0x1f8` | `int32_t unknown_1f8` |  |
| `0x1fc` | `int32_t unknown_1fc` |  |

#### `team` size 0x40 == k_team_size

| off | field | notes |
|---|---|---|
| `0x00` | `int16_t identifier` | datum_header |
| `0x02` | `uint8_t unknown_02[0x3e]` | never written in this build |

#### `player_globals` size 0x98

| off | field | notes |
|---|---|---|
| `0x00` | `datum_index unknown_00` | seeded to -1, never read |
| `0x04` | `datum_index local_players[1]` | local_player_to_player_index / game_set_local_player |
| `0x08` | `datum_index local_player_units[1]` | the unit each local player drives; indexed by player::local_player_index |
| `0x0c` | `int16_t unknown_0c` |  |
| `0x0e` | `int16_t respawn_stagger` | bumped and decremented while respawns are spread across frames |
| `0x10` | `uint8_t no_player_has_a_unit` | 0x474e10 sets 1 then clears it if any player still has a unit |
| `0x11` | `uint8_t unknown_11` |  |
| `0x12` | `int16_t unknown_12` | seeded to -1 |
| `0x14` | `int16_t mode` | written with 0 and with 3 |
| `0x16` | `uint8_t unknown_16` |  |
| `0x17` | `uint8_t unknown_17[0x98 - 0x17]` | zeroed by players_dispose, never read |

#### `local_player_control` size 0x40

| off | field | notes |
|---|---|---|
| `0x00` | `datum_index unit` | the controlled object, -1 when there is none |
| `0x04` | `uint32_t input_control_flags` | player_control_input::control_flags, stored by after it builds this tick's input |
| `0x08` | `uint16_t suppressed_buttons` | bit per digital button index (0..18): while set, the button reads as "not pressed". 0x4710b0's mask loop is a 16-bit AND/ANDN pair, so buttons 16..18 can never be suppressed even though the loop counts to 19. |
| `0x0a` | `uint16_t suppressed_until_released` | the subset of suppressed_buttons that is released again (both bits cleared) as soon as the physical button reads zero |
| `0x0c` | `float yaw` | atan2(facing.j, facing.i), wrapped into [0, 2pi) |
| `0x10` | `float pitch` | atan2(facing.k, \|facing.ij\|) |
| `0x14` | `float input_throttle_x` | player_control_input::throttle_x, stored by 0x471ae0 |
| `0x18` | `float input_throttle_y` | player_control_input::throttle_y |
| `0x1c` | `float input_primary_trigger` | player_control_input::primary_trigger |
| `0x20` | `int16_t desired_weapon_index` | seeded from unit+0x2f4; 0x472100 rewrites it |
| `0x22` | `int16_t desired_grenade_index` | seeded from unit+0x31d |
| `0x24` | `int16_t desired_zoom_level` | seeded from unit+0x321; 0x4726f0 invalidates it with 0xffff and 0x472740 reads it back |
| `0x26` | `uint8_t autolevelling_active` | set once autolevelling_ticks passes GlobalsPlayerControl::minimum_autolevelling_ticks; game_engine_update_local_player_look reads it as the "keep steering the pitch toward level" gate |
| `0x27` | `int8_t autolevelling_ticks` | clamped to 0..0x7f; 0x471ae0 counts up while the player is on foot, running (\|throttle_x\| > 0.5), not looking up or down and has no aim-assist target, and resets it to 0 otherwise |
| `0x28` | `datum_index nameplate_target` | written -1 by both initializers, and read back by hud_find_nearby_teammate_for_nameplate (0x45e340) as the teammate whose nameplate is currently drawn. Ghidra shows that read as "local_player_index * 0x40 + DAT_006b145c + 0x38", i.e. the compiler folded the 0x10-byte header into the field offset: 0x10 + 0x28 == 0x38. |
| `0x2c` | `float nameplate_weight` | the same read's gate, "0.0 < weight" means the track above is live; left 0.0 by both initializers. It is also camera_observer_get_target_angles' out_weight_primary -- 0x4710b0 calls that function as get_target_angles(&nameplate_weight, &aim_assist_weight, ..., slot) so 0x28/0x2c/0x30 are one aim-assist tracker block that the HUD nameplate code reuses. |
| `0x30` | `float aim_assist_weight` | out_weight_secondary of the same call; the magnetism strength, 0.0 when there is no target |
| `0x34` | `float look_acceleration_timer` | seconds the look stick has been held past GlobalsPlayerControl::look_peg_threshold; drives the look_acceleration_time / look_acceleration_scale ramp and is zeroed the moment the stick falls back inside the threshold |
| `0x38` | `float pitch_minimum` | -1.4906585 (-85.4 degrees) |
| `0x3c` | `float pitch_maximum` | +1.4906585 |

#### `player_control_globals` size 0x50

| off | field | notes |
|---|---|---|
| `0x00` | `uint32_t action_flags` | player_action_flags, rebuilt every tick by 0x472760 |
| `0x04` | `uint32_t action_flags_latched` | bits whose edge has already been consumed |
| `0x08` | `uint32_t action_flags_edge` | bits 0x472760 arms and clears again |
| `0x0c` | `uint32_t flags` | bit 0 suppresses player input: both game_engine_build_local_player_control_input (look deltas) and game_engine_update_local_player_control (zoom cycling) short-circuit while it is set |
| `0x10` | `local_player_control local_players[1]` |  |

#### `player_control_input` size 0x20

| off | field | notes |
|---|---|---|
| `0x00` | `float throttle_x` | forward / back; +0x00 and +0x04 are normalized as a 2D pair so the magnitude never exceeds 1 |
| `0x04` | `float throttle_y` | left / right |
| `0x08` | `float primary_trigger` | 1.0 when control_flags bit 0x800 is set, else 0.0 |
| `0x0c` | `float yaw_delta` | radians to turn this tick (already * dt * 30) |
| `0x10` | `float pitch_delta` | radians to pitch this tick |
| `0x14` | `int8_t action` | raw hold count of digital button 2, unless that button is in local_player_control::suppressed_buttons |
| `0x15` | `int8_t melee` | raw hold count of digital button 9, same condition |
| `0x16` | `int16_t pad_16` | never written (the record is zeroed first) |
| `0x18` | `uint32_t control_flags` | the digitized button set; the bit names are the unit_control_flags this eventually feeds |
| `0x1c` | `uint32_t button_flags` | a second digitized set. CORRECTED (phase 4 review, the disassembly): 0x01 next weapon (from digital button 3), 0x02 next grenade (button 1), 0x04 zoom (button 11); 0x08 and 0x10 are the single-player debug "possess the nearest / next unit" pair and 0x20 the debug camera-shake sample. The earlier note had 0x01 and 0x02 the other way round. |

#### `player_action` size 0x20

| off | field | notes |
|---|---|---|
| `0x00` | `uint32_t control_flags` | player_control_input::control_flags verbatim; 0x473270 masks it with 0x4d0 before re-staging |
| `0x04` | `float desired_yaw` | local_player_control::yaw (absolute, radians) |
| `0x08` | `float desired_pitch` | local_player_control::pitch |
| `0x0c` | `float throttle_x` | player_control_input::throttle_x |
| `0x10` | `float throttle_y` | player_control_input::throttle_y |
| `0x14` | `float primary_trigger` | player_control_input::primary_trigger |
| `0x18` | `int16_t weapon_index` | local_player_control::desired_weapon_index |
| `0x1a` | `int16_t grenade_index` | local_player_control::desired_grenade_index |
| `0x1c` | `int16_t zoom_level` | local_player_control::desired_zoom_level |
| `0x1e` | `int16_t pad_1e` | left uninitialized by 0x471ae0 |

#### `local_player_input_state` size 0x28

| off | field | notes |
|---|---|---|
| `0x00` | `int8_t buttons[0x13]` | one hold count per digital button, 19 of them. The control_flags bits 0x4710b0 builds out of them are button 10 -> 0x0001, 0 -> 0x0002, 2 -> 0x0040, 5 -> 0x0010, 13 -> 0x0400, 7 -> 0x0800, 6 -> 0x1000 and 0x2000 together, 4 -> 0x0080, and from button 14 or from button 2 held at least GlobalsPlayerControl::minimum_weapon_swap_ticks ticks. The button_flags bits are button 3 -> 0x01, 1 -> 0x02, 11 -> 0x04. Buttons 2 and 9 are also copied raw into player_control_input::action / ::melee. |
| `0x13` | `int8_t pad_13` |  |
| `0x14` | `float throttle_x` | quantized to -1/0/+1 in any networked game |
| `0x18` | `float throttle_y` | same |
| `0x1c` | `float look_x` | raw look stick, before the square-to-circle scaling |
| `0x20` | `float look_y` |  |
| `0x24` | `uint8_t look_is_analog` | zero selects the digital look path (raw rates, no response curve, no aim assist) |
| `0x25` | `uint8_t pad_25[3]` |  |

#### `player_profile` size 0x30

| off | field | notes |
|---|---|---|
| `0x00` | `uint8_t in_use` | the only field the free-slot scan looks at |
| `0x01` | `uint8_t pad_01[3]` |  |
| `0x04` | `datum_index player` | the handle 0x466e80 matches against |
| `0x08` | `int16_t kills` | <- player + 0x9c   (the cache copies 0x9c..0xb2 as |
| `0x0a` | `int16_t unknown_0a` | <- player + 0x9e    four dwords and three words, so |
| `0x0c` | `int32_t unknown_0c` | <- player + 0xa0    the field split here is the |
| `0x10` | `int16_t assists` | <- player + 0xa4    player's, not the copy's) |
| `0x12` | `int16_t unknown_12` | <- player + 0xa6 |
| `0x14` | `int32_t unknown_14` | <- player + 0xa8 |
| `0x18` | `int16_t betrayals` | <- player + 0xac |
| `0x1a` | `int16_t deaths` | <- player + 0xae |
| `0x1c` | `int16_t suicides` | <- player + 0xb0 |
| `0x1e` | `int32_t objective_time` | <- player + 0xc4 (seconds here, ticks in the player; the king engine rescales by 30 across the copy) |
| `0x22` | `int16_t unknown_22` | <- player + 0xc8 |
| `0x24` | `int32_t unknown_24` | <- player + 0x88 |
| `0x28` | `uint8_t odd_man_out` | <- player + 0x8c |
| `0x29` | `uint8_t pad_29[3]` |  |
| `0x2c` | `float speed` | <- player + 0x6c |

#### `team_pair_override` size 0x12

| off | field | notes |
|---|---|---|
| `0x00` | `int16_t index_a` |  |
| `0x02` | `int16_t index_b` | either ordering matches in every lookup |
| `0x04` | `int16_t threshold` | 0x45bfc0 fires team_pair_set once the counter reaches it |
| `0x06` | `int16_t timer_reset` | value the countdown is refreshed to |
| `0x08` | `uint8_t unknown_08` |  |
| `0x09` | `uint8_t unknown_09` |  |
| `0x0a` | `uint8_t active` | set to 1 when the entry is inserted |
| `0x0b` | `uint8_t status` | the extra byte 0x45be00 returns and 0x45c0f0 clears |
| `0x0c` | `uint8_t unknown_0c` |  |
| `0x0d` | `uint8_t pad_0d` |  |
| `0x0e` | `int16_t refcount` | decremented when the countdown expires; at 0 the entry is torn down |
| `0x10` | `int16_t timer` | ticks down once per call to 0x45bcf0 |

#### `team_pair_globals` size 0xb4

| off | field | notes |
|---|---|---|
| `0x00` | `int16_t override_count` | at most 8 |
| `0x02` | `team_pair_override overrides[8]` |  |
| `0x92` | `uint8_t pad_92[2]` |  |
| `0x94` | `uint32_t secondary_bits[4]` | indexed (b * 10 + a) |
| `0xa4` | `uint32_t enemy_bits[4]` | indexed (a * 10 + b); teams_are_enemies inverts it |

#### `scoreboard_entry` size 0x1c == k_scoreboard_entry_size

| off | field | notes |
|---|---|---|
| `0x00` | `datum_index player` |  |
| `0x04` | `int32_t unknown_04` | never compared |
| `0x08` | `int32_t key_0` | primary sort key, built by 0x45cc30: a clamped score |
| `0x0c` | `int32_t key_1` | biased by +1000 plus bit 0x40000000 when the player |
| `0x10` | `int32_t key_2` | still has lives left and bit 0x20000000 when the |
| `0x14` | `int32_t key_3` | player is not marked for deletion |
| `0x18` | `int32_t place` | 0-based rank; bit 0x80000000 marks a tie with the entry above |

#### `custom_waypoint` size 0x20

| off | field | notes |
|---|---|---|
| `0x00` | `real_point3d position` | the caller point, raised by 0.63 world units |
| `0x0c` | `uint8_t active` |  |
| `0x0d` | `uint8_t pad_0d[3]` |  |
| `0x10` | `datum_index player` | -1 = every player |
| `0x14` | `int16_t team` | -1 = every team; otherwise compared to player::team |
| `0x16` | `int16_t pad_16` |  |
| `0x18` | `datum_index owner` | -1 = no owner filter |
| `0x1c` | `int16_t icon` | resolved by name through 0x4af070 |
| `0x1e` | `int16_t pad_1e` |  |

#### `multiplayer_sound_request` size 0x10

| off | field | notes |
|---|---|---|
| `0x00` | `datum_index player` | recipient, -1 for everyone |
| `0x04` | `int32_t sound_index` | index into GlobalsMultiplayerInformation::sounds |
| `0x08` | `int32_t remaining_ticks` | duration + 5, counted down once per tick |
| `0x0c` | `uint8_t broadcast` | only ever 1 while hosting |
| `0x0d` | `uint8_t pad_0d[3]` |  |

#### `observer_target_candidate` size 0x38

| off | field | notes |
|---|---|---|
| `0x00` | `datum_index object` |  |
| `0x04` | `real_point3d point` | closest point on the target segment to the observer |
| `0x10` | `real_vector3d offset` | point - observer position |
| `0x1c` | `real_vector3d direction` | normalized copy of offset |
| `0x28` | `float distance` | length of offset |
| `0x2c` | `float angle` | angle between direction and the observer facing |
| `0x30` | `float weight_primary` | falloff(distance, cone.distance_a) * falloff(angle, cone.angle_a) |
| `0x34` | `float weight_secondary` | the same product against the b pair, boosted by the globals tag player information +0x08 when the target tag carries flag 0x80000 |

#### `observer_target_cone` size 0x10

| off | field | notes |
|---|---|---|
| `0x00` | `float angle_a` |  |
| `0x04` | `float distance_a` |  |
| `0x08` | `float angle_b` |  |
| `0x0c` | `float distance_b` |  |

#### `ctf_globals` size 0x148

| off | field | notes |
|---|---|---|
| `0x00` | `uint32_t flag_id_mask` | bit i set means usage_id i exists on this map |
| `0x04` | `int32_t team_flag_id[16]` |  |
| `0x44` | `uint8_t unknown_44[0x148 - 0x44]` | UNRESOLVED: per-team capture counters, the captured bitmasks 0x46ec10 sends, and the neutral-flag slot at 0x006b1314 |

#### `king_globals` size 0x0c

| off | field | notes |
|---|---|---|
| `0x006b1050` | `int32_t hill_state` | see king_hill_state |
| `0x006b1054` | `int32_t hill_ticks` | how long the current holder has held it; the "contested" cue only plays past 0x12d ticks |
| `0x006b1058` | `datum_index occupant` | the single holder, -1 when empty or contested |

#### `king_hill_marker_history` size 0x40

| off | field | notes |
|---|---|---|
| `0x00` | `real_point3d position[4]` |  |
| `0x30` | `int32_t state[4]` |  |

#### `savegame_index_record` size 0x206 == k_savegame_index_record_size

| off | field | notes |
|---|---|---|
| `` | `uint8_t data[0x206]` |  |

#### `user_save_path_table` size 0x848

| off | field | notes |
|---|---|---|
| `0x00721f30` | `char paths[8][0x105]` | , _strncpy of 0x104 chars |
| `0x00722758` | `uint32_t handles[8]` | , 0 marks a free slot |

#### `hud_text_bounds` size 0x08

| off | field | notes |
|---|---|---|
| `0x00` | `int16_t top` |  |
| `0x02` | `int16_t left` |  |
| `0x04` | `int16_t bottom` |  |
| `0x06` | `int16_t right` |  |

#### `hud_world_text_params` size 0x10

| off | field | notes |
|---|---|---|
| `0x00` | `int32_t unknown_00` | copied verbatim into the 0x006e4738 draw-state slot |
| `0x04` | `real color_r` |  |
| `0x08` | `real color_g` |  |
| `0x0c` | `real color_b` |  |

#### `camera_basis_out` size 0x18

| off | field | notes |
|---|---|---|
| `0x00` | `datum_index unit` | the unit the camera is attached to |
| `0x04` | `int16_t seat_index` | -1 when the unit is on foot |
| `0x06` | `int16_t pad_06` |  |
| `0x08` | `uint8_t *marker_offset` | UNSURE: the seat camera marker, NULL when on foot |
| `0x0c` | `real_point3d position` |  |

#### `koth_fence_corner` size 0x44

| off | field | notes |
|---|---|---|
| `0x00` | `float position[3]` |  |
| `0x0c` | `float normal[3]` | shared outward face normal for the whole edge |
| `0x18` | `float unused_18[6]` |  |
| `0x30` | `float u` | runs with the accumulated edge length |
| `0x34` | `float v` | fixed per corner (ground vs top) |
| `0x38` | `float unused_38[3]` |  |

#### `position_update_record` size 0x14

| off | field | notes |
|---|---|---|
| `0x00` | `int32_t tick` |  |
| `0x04` | `int32_t sequence` | wrapping distance, compared mod 0x40 |
| `0x08` | `real_point3d position` | copied out as three raw dwords by the queue functions |

#### `vehicle_update_body` size 0x40

| off | field | notes |
|---|---|---|
| `0x00` | `int32_t parent_or_tag` | UNSURE |
| `0x04` | `real_point3d position` |  |
| `0x10` | `real_vector3d velocity` |  |
| `0x1c` | `real_vector3d angular_velocity` |  |
| `0x28` | `real_vector3d forward` |  |
| `0x34` | `real_vector3d up` |  |

#### `vehicle_update_record` size 0x48

| off | field | notes |
|---|---|---|
| `0x00` | `int32_t tick` |  |
| `0x04` | `int32_t sequence` | same wrapping distance as position_update_record |
| `0x08` | `vehicle_update_body body` |  |

#### `client_update_carry` size 0x10

| off | field | notes |
|---|---|---|
| `0x00` | `uint8_t flag_a` | gates the 0x4e7b50 / grenade-value block |
| `0x01` | `uint8_t flag_b` | gates whether field1 becomes this tick's grenade value |
| `0x02` | `uint8_t pad_02[2]` |  |
| `0x04` | `int32_t field1` |  |
| `0x08` | `int32_t field2` |  |
| `0x0c` | `int32_t field3` |  |

#### `player_update_record` size 0x2c

| off | field | notes |
|---|---|---|
| `0x00` | `uint32_t field0` | UNSURE: forwarded whole to 0x476cf0 |
| `0x04` | `int32_t references_remaining` | decremented by every pop; 0 retires the record |
| `0x08` | `int32_t reference_count` | how many consumers the record was queued for |
| `0x0c` | `player_action action` | the staged control record for this tick |

#### `win32_find_dataa` size 0x140

| off | field | notes |
|---|---|---|
| `0x00` | `uint32_t dwFileAttributes` | bit 0x10 == directory |
| `0x04` | `uint32_t ftCreationTime[2]` | FILETIME |
| `0x0c` | `uint32_t ftLastAccessTime[2]` |  |
| `0x14` | `uint32_t ftLastWriteTime[2]` |  |
| `0x1c` | `uint32_t nFileSizeHigh` |  |
| `0x20` | `uint32_t nFileSizeLow` |  |
| `0x24` | `uint32_t dwReserved0` |  |
| `0x28` | `uint32_t dwReserved1` |  |
| `0x2c` | `char cFileName[260]` | MAX_PATH |
| `0x130` | `char cAlternateFileName[14]` |  |
| `0x13e` | `uint8_t pad_13e[2]` |  |

#### `user_save_path_table` size 0x848

| off | field | notes |
|---|---|---|
| `` | `char paths[k_maximum_user_save_paths][k_user_save_path_slot_stride]` |  |
| `` | `uint32_t keys[k_maximum_user_save_paths]` | 0x00721f30, _strncpy of k_user_save_path_length chars |

## What the phase-4 review pass changed

Two rewriter agents produced `0x460890..0x467f70`; an earlier session produced
`0x459300..0x460890`. A review pass then re-derived the low-confidence parts from
`objdump -d -M intel bin/halo.exe` and fixed what the decompiler had lost. Grouped by kind:

### Pointer-stride errors (the memory pilot's failure mode #1)

Eight expressions across seven files read a player through the **object-header** indirection
`((object_header *)player_data->data)[i].data` — stride `0x0c` plus a dereference. `player_data`
is a `data_array` of 0x200-byte `player` elements, and every one of those sites disassembles to
`shl eax,9` (× 0x200) plus the array's own `data` base, with no second load. All eight now use
`(player *)((uint8_t *)player_data->data + (i & 0xffff) * sizeof(player))`, which is what the
other 27 player accesses in the module already used.

### Register arguments Ghidra dropped at the call site

- `camera_observer_get_target_angles` / `camera_observer_get_target_id` printed
  `exclude_object = FUN_00569670()`. `0x569670` is a verified no-op; EAX is loaded with
  `player->unit` immediately before the call and survives it. The value is the unit handle.
- `game_engine_announce_time_remaining` printed `chimera__kill_feed(0xffffffff, ...)` because
  Ghidra constant-folded the iterator's `index` field to its initial `-1`. The loop reloads EDI
  from `iterator + 0x08` every pass; the recipient is the player just returned.
- `game_engine_get_scoreboard_place` was called four times with no arguments. `EAX` is 1, 2, 3
  and 4 at the four sites (the score / kills / assists / deaths column) and `invert_low_stat` is
  the pushed 0.
- `chimera__hud_message` (`0x4ae180`) takes the recipient's `local_player_index` in EAX. Both of
  its call sites in this module load it right before the call; the teleporter's site was also
  passing the wide string that belongs to `chimera__hud_message` itself into `0x4726b0`.
- `cheat_teleport_to_camera` pushes only `(object_index, 0, 0)`; EDI still holds the camera-state
  row, which is `object_set_position_and_orientation`'s `position` argument.
- `game_engine_handle_kill_feed_network_event` had four invented parameters. Its only argument is
  the message pointer in EAX: the other three values are *decoded* into a stack block by
  `FUN_004ec590(message, decoded)`, and the recipient is `player_globals->local_players[0]`.
- `game_variant_sanitize_options` (ECX), `game_engine_load_from_variant` (EBX = the
  `0x0087ab20` staging variant), `data_new` (EBX = element size), `cache_flush` (ESI),
  `data_delete_all` (ESI), `tag_iterator_next` (ESI), `predicted_resource_list_touch` (ESI),
  `unit_get_camera_position` (ECX/EDI), `cache_file_download_status_get` (EAX),
  `cache_file_open_by_name` (EAX + a `report_fatal_error` stack argument) and
  `saved_game_enumerate_by_type` (EBX = an in/out capacity-then-count) all now agree with the
  module that owns them.

### Dropped control flow and dropped calls

- `hud_draw_world_relative_text` (`0x4653f0`) takes a **row index in EDX**. Row 0 means "no
  background box"; the first pass armed the box unconditionally. It also threw away the 8-byte
  row rectangle the function builds and passes to the rasterizer in ECX, and returned 0 instead
  of the rasterizer's own result.
- `chimera__draw_16_bit_text` (`0x514ab0`) takes three stack arguments plus `EAX` and a
  `hud_text_bounds *` in `ECX`. Three of this module's four callers had declared it with the
  rectangle's packed halves as extra *stack* arguments (four, five and six arguments
  respectively). All four now agree.
- `game_engine_rasterize_in_game_score` was missing its first of three
  `hud_draw_world_relative_text` calls (the end-game result line, row 0), and passed no row to
  the other two.
- `game_engine_get_variant_by_name`'s custom-variant scan was bounded at a fixed 100 iterations;
  the real bound is the count `saved_game_enumerate_by_type` writes back through EBX. Its
  `requested_name_wide` parameter was invented — the buffer is a local, filled by
  `FUN_00557990(&buffer, 0x30)` (EAX/EDI). And its 36th comparison operand, which Ghidra prints
  as `PTR_s_g_objects_equipment_...`, is the string literal `"ctf"`.
- `game_engine_update_teleporter` used string-list index 0 where the disassembly uses `0x65`.

### Structure and type corrections

- `cheat_tag_record` **deleted** from `types/game.h`: it is `types/tags.h`'s `TagDependency`.
  `cheat_all_weapons` passes `Globals::weapon_list.pointer` (`GlobalsWeapon`, one TagDependency
  each) straight into `cheat_spawn_objects_near_camera`, and `cheat_spawn_warthog` passes
  `&GlobalsMultiplayerInformation::vehicles[i].vehicle`.
- `hud_text_bounds` and `hud_world_text_params` **added** to `types/game.h`, folding in the one
  local `typedef` the rewriters had left in `src/game/hud_draw_world_relative_text.c`.
- `message_delta_encode_message`'s 4th argument is a pointer **to** a pointer to the field
  block, not the block itself — MSVC reuses the dead incoming parameter slots as the scratch for
  that outer pointer, which is what produced Ghidra's `&param_N` confusion in
  `game_engine_notify_kill_event` and `game_engine_dispatch_item_pickup_event`. The two
  already-correct senders (`0x467010`, `0x4671d0`) pass `&payload_ptr` and corroborate it.
- Four raw-offset reads through `global_globals` resolved to named `Globals` fields:
  `+0x114` is `player_control.pointer` → `GlobalsPlayerControl::inconsequential_target_scale`
  (the autoaim secondary-weight multiplier), `+0x140/+0x144` is `interface_bitmaps`,
  `+0x14c/+0x150` is `weapon_list`, `+0x164/+0x168` is `multiplayer_information`. `global_globals`
  is now `Globals *` in all eleven files that use it (it was `uint8_t *` in four and
  `void *unknown_00746fa0` in one).
- `0x006e4734` and `0x006e4736` are two independent `int16` draw-state slots, not one dword;
  the five files that touch them now declare and write them the same way.
- `0x625b7a` is `wcslen` (it walks 16-bit units and divides the byte distance by two).

### Renames recorded in `symbols/agent_phase4_game.txt`

| addr | new name | was |
|---|---|---|
| `0x45a530` | `cheat_all_weapons` | `cheat_spawn_all_object_tags` |
| `0x467450` | `game_engine_variant_defaults_ctf` | `FUN_00467450` |
| `0x4726b0` | `unit_get_local_player_index` | `unit_get_local_player_weapon_index` |
| `0x625b7a` | `wcslen` | `FUN_00625b7a` |
| `0x4710b0` | `game_engine_build_local_player_control_input` | `FUN_004710b0` (no file) |
| `0x471ae0` | `game_engine_update_local_player_control` | `FUN_00471ae0` (no file) |
| `0x470790` | `game_engine_team_close_game_check` | same name, return type corrected to `uint8_t` |

## What the second phase-4 review pass changed (`0x468010 .. 0x473560`)

Two rewriter agents produced this range and left two functions unwritten for budget; a review pass
wrote those two from `objdump`, re-derived the low-confidence parts of the rest, and folded the
remaining local types back into the headers. Grouped by kind:

### Functions written in the review pass

- **`0x4710b0 game_engine_build_local_player_control_input`** (2595 bytes) and
  **`0x471ae0 game_engine_update_local_player_control`** (1108 bytes), the two the rewriters
  deferred. Both are transcribed from `objdump -M intel` rather than from Ghidra, whose decompile
  of `0x4710b0` renders the axis quantization as `extraout_EDX`/`extraout_ECX`, drops the
  local-player-index argument of both `camera_observer_*` calls, drops the EDI response-curve
  table and the EAX unit index of `unit_get_active_weapon_scale`, and whose decompile of
  `0x471ae0` mangles the two-entry grenade-cycling loop. `types/game.h` had already been written
  *from* these two functions during type recovery, so every struct they need already existed:
  `player_control_input`, `player_action`, `local_player_input_state`, `local_player_control`.

### Behaviour bugs fixed

- **`game_engine_digitize_control_input` (`0x472760`), two dropped `goto`s.** In each of the two
  branches of the edge-detection tail, the second block clears a bit of
  `player_control_input::control_flags` — but only when `action_flags_edge` still has its bit set.
  `objdump 0x4728f1 "je 0x472907"` and `0x472940 "je 0x472956"` both jump *past* the shared
  `and DWORD PTR [edx+0x18],esi`. The first rewrite hoisted that clear out of the `if`, so the
  flashlight (branch A) or jump (branch B) bit was cleared on every tick where the edge had
  already been consumed. This is the memory pilot's "goto target that skips a shared step"
  failure mode, exactly.
- **Eight `unit_data *` casts built straight from the object pointer.** `types/units.h`
  `unit_data` starts at `object + k_unit_data_offset` (`0x1f4`), so `(unit_data *)obj` reads every
  field `0x1f4` bytes too low. Fixed in `ctf_engine_flag_tick`,
  `game_engine_find_player_holding_object`, `game_engine_get_max_look_pitch`,
  `game_engine_koth_dispatch_player_scoring`, `game_engine_koth_update_occupant_table`,
  `game_engine_update_teleporter`, `unit_has_must_be_readied_weapon` and
  `unit_reset_gauge_if_flagged` — each one had been reaching for `0x2f2`, `0x2f8`, `0x218` or
  `0x37c` and landing in the object header instead.
- **Four copies of the Blam random-index idiom sign-extended the seed.** The real sequence is
  `movsx ecx,<count> ; shr eax,0x10 ; imul eax,ecx ; shr eax,0x10 ; movsx <idx>,ax`: the seed's
  high half is used **zero**-extended. Casting `(seed >> 16)` to `int16_t` first, which
  `game_engine_ctf_pick_random_flag`, `game_engine_koth_find_marker_position`,
  `game_engine_player_select_random_target` and `random_get_table_point` all did, makes the index
  negative for half of all seeds and silently disables the pick.
- **`game_engine_koth_find_marker_position` skipped a write the original always performs.** Every
  failure path falls through to the same three-dword copy at `0x46bfb6`, so `*out_position` is
  written even when nothing matched — with whatever was on the stack. The rewrite had guarded it
  with `if (index != -1)`. The staging copy is now deliberately uninitialized so the original
  behaviour stays visible.
- **`game_engine_team_close_game_check` returned a 32-bit packed word.** Its only call site is
  `objdump 0x47096f "test al,al"`, and the caller's own extern already said `uint8_t`. The return
  type is now `uint8_t` in both places.

### Structure and type corrections

- `game_engine_digitize_control_input` took a `uint8_t *` and raw offsets, with an `UNSURE` note
  saying the record was not attested by any header. It **is** `types/game.h`
  `player_control_input` — the record `0x4710b0` builds and passes in EDX — so the whole function
  is now field accesses. That was the last local layout in the range.
- `types/game.h` `player_control_input::button_flags` had `0x01` and `0x02` the wrong way round:
  `0x471ae0` tests `0x01` for the weapon cycle (digital button 3) and `0x02` for the grenade cycle
  (button 1). `local_player_input_state::buttons` now carries the full button-index-to-bit table,
  and the `player_action_flags` comments name `player_control_input` fields instead of
  `unit_control_data` fields (the two layouts differ: `unit_control_data::throttle` is at `+0x0c`,
  `player_control_input::throttle_x` at `+0x00`).
- `Globals + 0x114` is `player_control.pointer`, not `player_information` (that is `+0x174`).
  `game_engine_update_local_player_look` was reading `+0x54` through a variable called
  `player_information` — it is `GlobalsPlayerControl::look_autolevelling_scale` — and
  `game_engine_reset_player_look_state` was reading `+0x4c`/`+0x50`, which are
  `look_default_pitch_rate` / `look_default_yaw_rate`. That last pair is what fixes the two
  per-local-player look-rate globals: `0x006f1d74` is **yaw**, `0x006f1d78` is **pitch**. They
  were called `default_look_rate_b` / `default_look_rate_a`; both are now
  `look_yaw_rate_setting[]` / `look_pitch_rate_setting[]`.
- `tag_reflexive_pick_weighted_random_index` declared a local `{ int32_t count; uint8_t *elements; }`
  for the reflexive header it reads out of a tag. That is `types/tags.h` `TagReflexive`; the local
  copy is gone.
- `network_game_mode` (`0x00719720`) is an `int16_t` — every access in the image is a 16-bit one,
  which is what `types/game.h`'s own note already said. 41 files declared it `int32_t` and 4
  declared it `int16_t`; all 45 now agree with the header.
- One global, one name: `0x0087a478` was `the_player_globals` in `ctf_engine_flag_tick.c` and
  `local_player_globals` in the other nine files that use it.

### Left for the owning module (found, not fixed here)

- `src/units/unit_get_active_weapon_scale.c` (`0x565ab0`) models its single stack argument as the
  unit index. The disassembly takes the **unit index in EAX** and forwards the stack argument in
  EDX to `0x4c2d70`; `0x4710b0` passes the zoom level there. The call sites in this module pass
  the zoom level and document the mismatch; the units-module file needs the real signature.
- `src/game/camera_observer_get_target_id.c`'s stack parameter is named `out_id`, but `0x4710b0`
  passes `&local_player_control::nameplate_weight` and stores the **return value** into
  `nameplate_target`. The parameter is an out-weight and the return is the handle, exactly as in
  its sibling `camera_observer_get_target_angles`. The prototype was left alone (it is that
  file's, and nothing in the range depends on the name) and the call site casts.

## What the third phase-4 review pass changed (`0x4735b0 .. 0x551d30`)

Two rewriter agents produced this range (99 files: 37 in `0x4735b0..0x4776d0`, 62 above it) and
skipped three addresses as fragments. This review pass verified those three, resolved a duplicate,
folded every remaining local `typedef` into `types/game.h`, spot-checked the largest and most
`UNSURE`-marked files line by line against `python tools/pack.py` plus `objdump -d -M intel`, and
made the declarations agree across the whole directory. Grouped by kind:

### A duplicate and a misidentified struct

- **`0x4776d0` had two files.** The first agent's range ended at it inclusively and the second
  agent's began at it, so both wrote it: `game_pick_weighted_starting_location.c` (rewrite
  confidence 0.3) and `player_pick_random_starting_location.c` (0.55). The second is kept - it
  identifies `0x6283c0` as the MSVC 7.1 CRT `_CIpow` with the `0x00672cf0` exponent `0.5` (so the
  weight is `sqrt(random) * suitability`) and `0x45f7c0` as `netgame_equipment_game_type_matches`.
  The first is deleted.
- **`position_history_reset_all_slots` was not a position history.** Its single caller,
  `player_respawn`, reaches it (objdump `0x4780a0..0x478124`) with `ECX` = the
  `object_placement_data` block `object_placement_data_initialize` just built at `esp+0x30` - the
  caller's own writes at `+0x18` (position), `+0x34` (forward) and `+0x40` (up) pin that - and
  `EAX` = the three-float RGB `game_engine_get_player_color` produced. So the four 3-float slots it
  fills at `+0x58` are that struct's four **change colors**. The file is now
  `object_placement_data_set_change_colors.c` and the invented local `position_history` struct is
  gone.

### The starting-location scorer chain was named as damage code

`0x461ad0` / `0x461c60` / `0x461d90` were `game_engine_compute_proximity_damage_scale`,
`_compute_nearby_ally_bonus` and `_compute_damage_scale`, and `0x461d90` took a `damage_data *` in
`EAX`. All four names were wrong, and the proof is in this range:
`player_pick_random_starting_location` (`0x4776d0`) is `0x461d90`'s **only** caller, and at
`0x477770` it does `mov eax,[esp+0x2c] ; push eax ; mov eax,edi ; call 0x461d90` with
`EDI = scenario->player_starting_locations.pointer + i * 0x34`. That makes `EAX` a
`ScenarioPlayerStartingLocation *`, and the `+0x10` word `0x461d90` compares against `player::team`
is that record's `team_index` (`types/tags.h`), not `damage_data::team_index`. The two callees
confirm it from the other side - `0x461ad0` penalizes a point for being within 0.25 / 1.0 / 2.0 /
5.0 world units of another player's unit and `0x461c60` pays a bonus for same-team units 1.0..6.0
away. That is Halo's spawn weighting. Renamed to `game_engine_rate_location_crowding`,
`game_engine_rate_location_ally_bonus` and `game_engine_rate_player_starting_location`, and
`0x461d90`'s signature retyped.

`0x461e60`, in the same chain, was `game_engine_find_nearby_vehicle(void)`. objdump `0x461eaa`
shows the `push edx` that feeds `object_find_in_sphere`'s `center` is the **incoming** `EDX` - the
caller sets `mov edx,edi` - not a second return value out of `0x5013a0` as the first pass modeled.
With radius `0.1f` and an `object::type == 1` test, it is `game_engine_location_blocked_by_vehicle`.
Its cluster-table read also gained the missing indirection (`*(uint8_t **)(matg + 0xe4)`; the
first pass used `matg + 0xe4` as the table itself).

### Behaviour bugs fixed

- **`main_switch_structure_bsp` (`0x4749a0`) skipped the whole per-player tail.** Ghidra's
  `goto LAB_00474c59` lands on the **inner** trigger-volume loop's own increment and only skips the
  two `console_print_va` calls. The first pass turned it into a jump out to the player loop's
  advance, so every player who successfully triggered a BSP switch never had
  `interaction_object` / `interaction_type` reset and never got its `0x478400` / `0x478500`
  interaction scan. This is the memory pilot's "goto target that skips a shared advance step",
  exactly. The same function was also missing the three-step read-modify-write that stamps the
  player's `local_player_index` into the low nibble of `player_globals + 0x17`.
- **`game_engine_apply_player_spawn_loadout_message` (`0x477c70`) cleared half a field.** The
  original zeroes a full dword at `player + 0x68`, which is **both** `kill_streak` entries; the
  rewrite cleared only slot 0.
- **`player_update_queue_pop_current` (`0x479fb0`) returned the wrong type.** `0x479fb4` is
  `or eax,0xffffffff` and `0x479fcd` is `mov al,0x1`, so on success `EAX` is `0xffffff01`. Both
  callers use `cmp al,0x1`. Declared `uint8_t`, and its parameters swapped to `(out, queue)` to
  match its own stated `blam-cc: EAX -> out, EBX -> queue` (house order is register order).
- **`savegame_index_file_exists` (`0x53e060`) returned the wrong type** for the same reason: only
  `AL` is written, over an `EAX` still holding `0x00721330`.
- **`game_engine_get_player_color` (`0x463290`) dropped its return value.** `0x4632ee` is
  `mov eax,esi ; ret` - it returns `out_rgb`, and `player_respawn` immediately dereferences that
  pointer. Its no-teams branch also passes `player + 0x60` (a **word** colour index) plus a 12-byte
  stack scratch to `0x539c20`, which Ghidra rendered as a call taking the player index. And
  `0x00686b10` / `0x00686b18` hold **pointers** to the default colours (`mov eax,ds:0x686b10` then
  `mov ecx,[eax]`), not the colours themselves.
- **`game_engine_koth_player_tick` (`0x46ab00`) lost its announcer selection.** Three sites compute
  a sound index into `ESI` - `teams ? 5 + 2*(team != 0) : 3` at `0x46abf5`,
  `teams ? 4 + 2*(team != 0) : 2` at `0x46ac27`, and the constant `0x2a` at `0x46ac6c` - and Ghidra
  keeps only the literal `1` that is really `multiplayer_sound_request::broadcast`. All three calls
  passed `1`.
- **`player_attach_unit_to_parent` (`0x475c60`) threw away the position it computes.** objdump
  `0x475e2e..0x475e4e` is `push 0 ; push 0 ; push esi` plus `lea edi,[ebp-0x38]`, i.e.
  `object_set_position_and_orientation(object_index, NULL, NULL, position)`; the first pass called
  the 3-argument form and marked the three summed floats as a dead computation. The sums are also
  `fadd`, not the subtraction the first pass wrote.

### Register arguments Ghidra dropped at the call site

Each of these was read off the disassembly at the call, not guessed:

| call site | callee | what was elided |
|---|---|---|
| `0x4784c0`, `0x4785be` | `player_check_assassination_opportunity` (`0x478770`) | `push ecx ; push edi` - the player handle and the candidate object; the callee reads `[esp+0x4]` as a player handle |
| `0x4784b7`, `0x4785b7` | `FUN_004788a0` / `FUN_00478c40` | the same two arguments |
| `0x47799a` | `game_set_local_player` (`0x474d50`) | `movsx si,BYTE PTR [ebp+0x1d]` - a **signed** byte out of the sender's identifier record, plus `ECX` = the message's `join_key` |
| `0x477945` | `FUN_004e9cd0` | `mov eax,0x687500` and `mov ecx,[esp+0x14]` on top of the one visible stack argument |
| `0x4619fb` | `game_engine_compute_look_angles_from_vector` (`0x470d80`) | `mov cx,WORD PTR [ebp+0x2]` (the player's `local_player_index`) and `lea eax,[esp+0x1c]` (the forward vector) |
| `0x472c97`, `0x472cb1` | `datum_new_at_index_with_salt`, `player_update_queue_create` | `EAX` (forwarded, so `update_server_queue_create_entry` itself takes a handle in `EAX`), `EDX` = the array, and `ESI` = `&entry->queue` at stride `0x64` offset `0x28` |
| `0x53e08d`-`0x53e0c3` | `path_remove_last_component`, `path_append_component`, `file_reference_open`, `file_reference_get_size` | `EBX` / `EBX`+`ESI` / `ESI` / `EAX` - pinned once and applied to all six `savegame_index_*` files |
| `0x45c541` | `chimera__kill_feed` (`0x460a30`) | `EDI` = the iterator's current handle (the recipient); the declaration in `game_engine_player_new_life.c` was a 4-parameter form that did not match the definition's 5 |
| `0x45aab4`ff | `game_state_new` (`0x5380d0`) | `EBX` = the element size at all seven `game_initialize` sites (`0x100`, `0x70`, `0xfc`, `0x3c`, `0x158`, `0x80`, `0x64`) |

### Types folded out of the source files and into `types/game.h`

Every `TYPES-GAP` local `typedef` in the directory is gone. `types/game.h` gained:

- `position_update_record` (0x14) and `vehicle_update_body` / `vehicle_update_record` (0x40 / 0x48)
  - the record shapes of the two `circular_queue`s embedded in every `player`, previously
  duplicated across four files with three different field spellings.
- `player_update_record` (0x2c), which replaces the local `queue_peek_result`. Its two counters are
  now named from both ends: `0x479fb0` does `dec [record+0x04]` and retires the record at zero, and
  its callers test `out[1] == out[2] - 1` (objdump `0x4746af`) to decide whether **this** consumer
  is the first to see it - which is exactly when they call `player_apply_first_position_update`.
  So `+0x04` is `references_remaining` and `+0x08` is the total `reference_count`.
- `client_update_carry` (0x10), the per-player side-band record `update_client_queue_apply_tick`
  writes and `game_engine_players_update_server` consumes. Still UNSURE field by field.
- `win32_find_dataa` (0x140), the real Win32 `WIN32_FIND_DATAA` layout, replacing three different
  partial local copies in `XDeleteSaveGame.c`, `savegame_find_first.c` and `savegame_find_next.c`.
- `k_user_save_path_slot_stride = 0x105` in `game_constants`. Three files had
  `#define k_user_save_path_length 0x105` **after** including a header whose enum already defines
  the same name as `0x104` - a latent collision. `user_save_path_table`'s `handles` field is also
  renamed `keys`, because `savegame_find_first` registers a Win32 find handle there, not a user id.

### Declarations made to agree

`extern` declarations of the same symbol disagreed in 31 places, and definitions disagreed with
their callers' declarations in 14. All 14 definition mismatches and 19 of the 31 declaration
disagreements are resolved. Notable ones beyond the bugs above:

- `local_player_set_controlled_unit` was **defined** `(new_unit, local_player_index)` and
  **declared** `(local_player_index, new_unit)` by both its callers - the arguments were swapped at
  every call site. Unified on the definition, which is the one that did the objdump pass.
- `game_state_base` was `int32_t` in three files and `uint8_t *` in a fourth (and in
  `src/objects`); `random_point_table` was `uint8_t *` with a hand-computed `* 0xc` stride in one
  file and `real_point3d []` in another; `random_point_table_count` was `int16_t` in one and
  `int32_t` in another (the binary reads it with `movsx` from a word); `random_seed_global` was
  `uint32_t` in eight files and `random_seed` in three.
- `0x0071973c` was `int32_t` in one file and `uint8_t` in another. It is written as a **byte**
  (`mov ds:0x71973c,al`), as is its neighbour `0x00719738`.
- `datum_get`, `datum_new_at_index_with_salt` and `object_set_position_and_orientation` were
  declared `(void)` / 3-argument in the two weakest files; they now carry `src/memory`'s and
  `src/objects`' canonical signatures.
- `FUN_004ec590` / `FUN_004ec670`, the message-delta envelope pair, had a one-argument reading in
  `game_engine_client_apply_team_assignment.c` and `game_engine_update_lead_change_state.c` and the
  established `(event, out_values)` / `(event)` reading in ten others. Unified on the latter.
- `CloseHandle`, `GlobalFree` and `__ftol` were unified on the Win32 / CRT truth (`__ftol` on
  `(void)`, since its argument really is on the x87 stack and no call site can name it).

### Confirmed non-functions (verified, not written)

`0x476847` sits inside an `fstp` run whose frame was already adjusted at `0x47683d`; `0x47bf23` is
the `je 0x47bf4c` inside the function that starts at `0x47bef0`; `0x47c3d0` is the
`call 0x4f6ec0` inside the function that starts at `0x47c3a0`. All three are mid-body addresses,
confirmed by `objdump`, and correctly left unwritten. Note that **`0x47bef0` and `0x47c3a0`, the
two real function entries those fragments live inside, are not in `modules.json` at all** - the
module assignment carries the fragment instead of the function, so two real functions in this
address range have no file and no module.

## Known gaps

**Struct fields still unresolved.** `types/game.h` marks these inline; the full derivation is in
`out/phase4/game_types_notes.md`. The largest holes are `player` (0xb8..0xbf, 0xca..0xd4,
0xf0..0x11f — the network state the client/server update code owns — and 0x190..0x1cf),
`ctf_globals` (everything past 0x44), `update_record` (the entire 0x308 body: it is the packed
per-player payload the network module encodes), `savegame_index_record` (the entire 0x206 body),
and `team` (no function in this build ever creates or reads a team datum, so only the size is
known and it is declared opaque on purpose).

**Globals this module owns whose type is unknown.** `0x00871de0` (the 0x7ff8-byte network message
scratch buffer the message-delta encoders write into — its record structure is the network
module's), `0x006b0f50` (starting-location result count), `0x006b1458`, `0x0087abe0`/`0x0087abdc`,
and the `0x006b0e88..0x006b1050` run between the profile cache and the King globals.

**`TYPES-GAP` markers: 146 occurrences across 22 files**, concentrated in the four lifecycle
functions (`game_start_new_map` 37, `game_dispose` 26, `game_stop_current_map` 21,
`game_initialize` 17). Those are all *other* modules' pool pointers - the particle, effect,
effect-location, weather and render-state `data_array`s and the Win32 handles - which this module
only allocates, zeroes and frees. **No `TYPES-GAP` is a local `typedef`**: every remaining one is an
unnamed `extern` on a foreign module's global, which is what the header rules ask for.

**`UNSURE` markers: 1054 across 284 files**, 259 of them in the 98 files of the
`0x4735b0 .. 0x551d30` range. The densest single files are
`game_engine_rasterize_in_game_score` (30), `game_engine_koth_submit_hill_marker_geometry` (23, and
the lowest rewrite confidence in the directory at 0.1 - a near-literal transcription of a
rasterizer-heavy function, and the render module has had no type recovery yet),
`game_engine_build_kill_feed_message_text` and `game_simulate_tick` (18 each),
`game_engine_reattach_player_unit_unused` (17) and `game_engine_update_teleporter` (16).

**The `0x4735b0 .. 0x551d30` range's own weak spots.** `game_engine_players_update_server`
(`0x4740a0`), `game_engine_players_update_client` (`0x474590`) and
`players_client_catchup_on_server_updates` (`0x476d40`) are the three least-verified functions in
the module: Ghidra mis-tracks their large ESP-relative frames, rendering CALL return addresses as
local-variable assignments and merging one 16-byte and one 32-byte per-player record into three
overlapping pseudo-locals. They read and write `client_update_carry` and `player_update_record`,
both of which are only attested by that one pair of producer/consumer functions.
`game_engine_reattach_player_unit_unused` (`0x475270`) is dead code (`callers=0`, superseded by
`player_attach_unit_to_parent` at `0x475c60`) and got a deliberately lower-rigor pass;
`update_client_queue_apply_tick` (`0x4730d0`, from the previous pass) still calls
`update_client_queue_get_slot(0)` because its real tick argument arrives in `EAX` and was never
recovered - that is a placeholder, not a transcription, and it should be the first thing hooked in
the update-queue cluster.

**Cross-module callee conventions still in disagreement** (two rewrites read different registers;
each needs a hook or an objdump pass on the callee itself, which is in another module):

| callee | the disagreement |
|---|---|
| `0x56d400` | five different declared forms across five files: `(U32,U8)`, `(U32)`, `(U8)`, `U8(U8)`, `U8(U32,U8)`. The entry does `mov edi,eax`, so `EAX` is certainly an argument |
| `0x56d990` | `int16_t (void)` in two files, `int16_t (uint32_t)` in a third. The entry masks `EAX` to 16 bits and indexes `object_headers`, so the 1-argument form is right, but the two `(void)` call sites have no visible handle to pass |
| `0x569bf0` | `(unit)` in two files, `(unit, attaching)` in `local_player_set_controlled_unit.c`, which claims a `CL` flag |
| `0x56d6e0 unit_ready_desired_weapon` | `(unit)` vs `(unit, flag)` |
| `0x566970 unit_enter_vehicle_seat` | `(vehicle, seat)` vs `(vehicle, seat, unit)` |
| `0x557950` | `void (char *, uint32_t)` with the string in `EDI` vs `char * (uint32_t)`. The entry passes `EDI` to `0x625b7a` (`wcslen`) and then reads one stack argument, so the first reading is closer |
| `0x557990` | four readings across four files: `(char *, uint32_t)`, `(wchar_t *)`, `(wchar_t *, int32_t)`, `(void)`. The entry walks `EBX` to a NUL byte |
| `0x447740` | `(void)` vs `(uint32_t)`. Its entry reads only a global |
| `0x466cb0`, `0x478ff0`, `0x4e1a80`, `0x4e9cd0` | resolved to one spelling each in this pass; `0x4e9cd0` still has a 1-argument reading in `game_engine_spawn_or_replay_netgame_equipment.c` against the 3-argument one the join handler's objdump pass established |

**`game_state_new` (`0x5380d0`) has two views in the repo.** `src/game` now uses the 3-argument
form with the `EBX` element size at every site. `src/hs/hs_runtime_initialize.c` and
`src/hs/object_lists_initialize.c` still declare the 2-argument view. Whichever module ends up
owning the function should reconcile them; the sizes are recoverable the same way
(`mov ebx,<size>` immediately before each call).

**`types/objects.h` calls `object_placement_data + 0x58` `network_vectors[4]` and types it
`real_vector3d`.** It is Blam's `real_rgb_color change_colors[4]`:
`object_placement_data_set_change_colors` (`0x477670`) fills all four slots with one player colour,
and `0x4f8b70` - the function `object_new_with_datum_role_control` hands them to - copies them into
`object + 0x188` with count 4 immediately before `object_update_change_colors`. `src/objects`'
`object_set_position_network` (in `object_apply_network_placement.c`) is almost certainly
`object_set_change_colors` for the same reason. Not renamed here, because the field and the function
belong to `types/objects.h` and `src/objects`.

**The update-queue cluster is the weakest part of the new range.** `update_record` is a 0x308-byte
packed payload with no recovered body, `update_server_queue` is unresolved from `0x02` to `0x27`,
and `0x006f7ea4` / `0x006f7ec4` / `0x006f7ecc` are declared as raw staging globals even though
`types/game.h` now types the record itself as `player_action`.
`update_client_queue_apply_tick`, `update_server_push_player_tick_history`,
`update_client_advance_read_cursor` and `update_run_catchup_ticks` all sit at 0.15–0.2 rewrite
confidence and should be hooked before they are trusted.

**Cross-module discrepancies found but not fixed here** (they belong to the owning module):

- `src/objects/objects_update_control_bindings.c` claims `blam-cc: EAX -> param_1`. `0x4f3ba0`
  loads EAX from `0x0071c2d4` as its first instruction and reads its only parameter twice from
  `[esp+0x24]`, so the argument is a plain stack argument (this module passes `global_scenario`).
- `types/memory.h`'s `data_iterator` is 0x0c bytes. Every caller in this module reserves 0x10 and
  writes a fourth dword — a `(data_array * ^ 'iter')` signature — at `+0x0c`. The rewrites model
  that fourth dword as a separate `unused_checksum` local so the shared header stays untouched.
- `types/hs.h` calls `hs_game_time_globals + 0x1c` `seconds_per_tick`. `0x470b30` uses it as the
  fractional-tick accumulator carried between frames, and `+0x18` as the time scale.
- `types/objects.h` / `types/units.h` call `0x006f1d20` `game_is_server`. It is
  `game_engine_definition *current_game_engine`; about 30 vtable dispatches go through it.

## Misattributed / fragment functions

| addr | what `out/phase4/game_functions.md` calls it | reality |
|---|---|---|
| `0x45b4cb` | `game_initialize_mod_per_map_upgrade_effects` | 22-byte fragment overlapping the end of `0x45b370`; **no file written** |
| `0x45b4e4` | `game_initialize_mod_per_map_upgrade_locations` | 1-byte stub, same overlap; **no file written** |
| `0x45fc50` | `hud_render_scoreboard_ingame` | 45-byte tail fragment of the netgame-equipment respawn tick; **no file written** |
| `0x476847` | `player_add_equipment_unit_grenade_count_mod` | an `fstp` mid-body of `0x476760`, whose frame is already adjusted at `0x47683d`; **no file written** |
| `0x47bf23` | `player_handle_action_jmp_table_adjust_size` | the `je 0x47bf4c` inside the function that really starts at `0x47bef0`; **no file written**, and `0x47bef0` itself is missing from `modules.json` |
| `0x47c3d0` | `player_health_pack_screen_effect` | the `call 0x4f6ec0` inside the function that really starts at `0x47c3a0`; **no file written**, and `0x47c3a0` itself is missing from `modules.json` |
| `0x461ad0` / `0x461c60` / `0x461d90` / `0x461e60` | the "damage scale" chain | the player starting-location suitability scorer; all four **renamed** (see the third review pass above) |
| `0x477670` | `position_history_reset_all_slots` (this repo's own earlier name) | `object_placement_data_set_change_colors`; **renamed** |
| `0x4726b0` | `unit_get_local_player_weapon_index` | returns `player::local_player_index`; **renamed** |
| `0x4726f0` / `0x472740` | a "weapon-index salt" accessor pair | the `local_player_control + 0x24` zoom-level pair (`0x472100` is the weapon one) |
| `0x461080` | `game_engine_find_valid_starting_locations` | walks `netgame_flags` (stride 0x94), not `player_starting_locations`; its "team"/"type" parameters are swapped relative to the inherited signature. The rewrite follows the corrected reading |
| `0x53e060`–`0x53e630`, `0x551620`–`0x551d30` | save-game directory / index helpers | assigned to `game` in `modules.json` on CEA string evidence, but pure file-system plumbing; if the module boundary is redrawn, `savegame_index_record`, `user_save_path_table` and `win32_find_dataa` move with them |

`0x4637c0` and `0x463810` are transcribed as literal empty scan loops. They are real code that
walks `netgame_flags` / `netgame_equipment` comparing a register argument against each entry and
does nothing with the result — the release build's dead error-reporting bodies. Nothing was
invented to fill them.

## Functions

All 422 files, in address order. `rewrite conf` is the file's own self-assessment;
`UNSURE` is the number of `UNSURE` markers in its header and body. The six addresses
`modules.json` assigns to `game` that have no row are the fragments listed above.

| addr | function | bytes | rewrite conf | UNSURE |
|---|---|---|---|---|
| `0x459300` | `vector3d_clamp_length` | 83 | 0.75 | 1 |
| `0x459360` | `distance_falloff_fraction` | 78 | 0.7 | 0 |
| `0x4593b0` | `camera_observer_update` | 821 | 0.15 | 14 |
| `0x4596f0` | `camera_observer_get_target_angles` | 517 | 0.2 | 7 |
| `0x459900` | `camera_observer_get_target_id` | 248 | 0.2 | 3 |
| `0x459a00` | `camera_observer_find_best_target` | 270 | 0.6 | 4 |
| `0x459b10` | `camera_observer_target_score` | 419 | 0.4 | 4 |
| `0x459cc0` | `camera_observer_target_direction` | 272 | 0.3 | 1 |
| `0x459dd0` | `camera_observer_target_is_valid` | 168 | 0.75 | 1 |
| `0x459e80` | `unit_get_current_weapon_autoaim_cone` | 240 | 0.45 | 2 |
| `0x459f70` | `camera_observer_generate_target_candidates` | 347 | 0.2 | 5 |
| `0x45a0e0` | `camera_observer_collect_target_candidates` | 401 | 0.25 | 5 |
| `0x45a280` | `vector3d_closest_point_on_segment` | 533 | 0.3 | 3 |
| `0x45a4a0` | `camera_observer_target_compare` | 142 | 0.8 | 0 |
| `0x45a530` | `cheat_all_weapons` | 141 | 0.45 | 0 |
| `0x45a5c0` | `cheat_spawn_warthog` | 97 | 0.6 | 2 |
| `0x45a630` | `cheat_teleport_to_camera` | 136 | 0.55 | 5 |
| `0x45a6c0` | `cheat_make_selected_object_invincible` | 90 | 0.5 | 1 |
| `0x45a720` | `cheat_make_player_invincible` | 114 | 0.55 | 0 |
| `0x45a7a0` | `cheat_get_target_object_index` | 86 | 0.5 | 5 |
| `0x45a800` | `cheat_spawn_objects_near_camera` | 441 | 0.25 | 8 |
| `0x45a9c0` | `game_initialize` | 783 | 0.45 | 5 |
| `0x45acd0` | `game_dispose` | 460 | 0.45 | 2 |
| `0x45aea0` | `cache_file_switch_map_by_path` | 260 | 0.3 | 7 |
| `0x45afb0` | `game_unload_map` | 152 | 0.4 | 1 |
| `0x45b050` | `game_start_new_map` | 797 | 0.35 | 9 |
| `0x45b370` | `game_stop_current_map` | 347 | 0.35 | 4 |
| `0x45b4f0` | `game_effects_update` | 148 | 0.65 | 2 |
| `0x45b590` | `game_engine_flag_local_player_units` | 225 | 0.45 | 4 |
| `0x45b680` | `network_server_broadcast_object_type_changes` | 256 | 0.35 | 2 |
| `0x45b780` | `game_simulate_tick` | 300 | 0.35 | 18 |
| `0x45b8b0` | `game_engine_reset_all_players` | 98 | 0.3 | 5 |
| `0x45b920` | `game_engine_set_variant_by_name` | 112 | 0.3 | 4 |
| `0x45b990` | `game_engine_apply_variant` | 73 | 0.55 | 1 |
| `0x45b9e0` | `game_safe_to_pause` | 108 | 0.6 | 4 |
| `0x45ba50` | `game_safe_to_save` | 395 | 0.6 | 6 |
| `0x45bbe0` | `game_no_player_is_dead` | 75 | 0.55 | 0 |
| `0x45bc30` | `team_pair_table_allocate` | 72 | 0.75 | 0 |
| `0x45bc80` | `team_pair_table_init_defaults` | 101 | 0.75 | 0 |
| `0x45bcf0` | `team_pair_overrides_tick` | 91 | 0.55 | 2 |
| `0x45bd50` | `teams_are_enemies` | 91 | 0.75 | 0 |
| `0x45bdb0` | `team_pair_flag_test` | 68 | 0.6 | 1 |
| `0x45be00` | `team_pair_override_get_flag` | 77 | 0.6 | 0 |
| `0x45be50` | `team_pair_override_add` | 179 | 0.45 | 2 |
| `0x45bf10` | `team_pair_override_remove` | 162 | 0.5 | 2 |
| `0x45bfc0` | `team_pair_override_adjust_counter` | 198 | 0.4 | 2 |
| `0x45c090` | `team_pair_override_refresh` | 92 | 0.4 | 0 |
| `0x45c0f0` | `team_pair_override_clear_flag` | 62 | 0.55 | 0 |
| `0x45c130` | `team_pair_set` | 400 | 0.35 | 1 |
| `0x45c2c0` | `game_engine_load_from_variant` | 112 | 0.55 | 1 |
| `0x45c330` | `game_engine_unload` | 59 | 0.7 | 0 |
| `0x45c370` | `game_engine_initialize_for_new_game` | 207 | 0.4 | 5 |
| `0x45c440` | `game_engine_player_new_life` | 304 | 0.45 | 2 |
| `0x45c570` | `game_engine_player_changed_object` | 286 | 0.3 | 3 |
| `0x45c6a0` | `players_active_count` | 73 | 0.6 | 3 |
| `0x45c6f0` | `players_get_active_by_index` | 91 | 0.4 | 5 |
| `0x45c750` | `game_engine_players_ready_for_bsp_switch` | 214 | 0.3 | 5 |
| `0x45c830` | `game_engine_players_ready_for_bsp_switch_strict` | 426 | 0.25 | 8 |
| `0x45c9e0` | `game_engine_find_first_eligible_player_on_team` | 207 | 0.3 | 4 |
| `0x45cab0` | `game_engine_get_time_remaining` | 37 | 0.6 | 1 |
| `0x45cae0` | `game_engine_announce_time_remaining` | 217 | 0.45 | 2 |
| `0x45cbc0` | `scoreboard_entry_compare_by_unknown_04` | 32 | 0.55 | 0 |
| `0x45cbe0` | `scoreboard_entry_compare` | 74 | 0.6 | 1 |
| `0x45cc30` | `game_engine_build_scoreboard_sort_key` | 87 | 0.7 | 0 |
| `0x45cc90` | `game_engine_build_sorted_player_list` | 303 | 0.55 | 5 |
| `0x45ce90` | `game_engine_get_default_multiplayer_string` | 68 | 0.55 | 2 |
| `0x45cee0` | `game_engine_get_player_scoreboard_entry` | 78 | 0.5 | 1 |
| `0x45cf30` | `game_engine_build_end_game_result_text` | 1286 | 0.2 | 15 |
| `0x45d440` | `game_engine_get_scoreboard_place` | 84 | 0.4 | 1 |
| `0x45d4a0` | `select_players_to_display` | 453 | 0.35 | 4 |
| `0x45d670` | `hud_draw_scoreboard_row_text` | 135 | 0.35 | 2 |
| `0x45d700` | `game_engine_post_rasterize_post_game` | 3024 | 0.2 | 15 |
| `0x45e340` | `hud_find_nearby_teammate_for_nameplate` | 474 | 0.3 | 4 |
| `0x45e520` | `hud_draw_teammate_nameplate` | 343 | 0.35 | 2 |
| `0x45e680` | `game_engine_build_kill_feed_message_text` | 2815 | 0.15 | 18 |
| `0x45f220` | `hud_update_teammate_nameplate_fade` | 251 | 0.3 | 6 |
| `0x45f320` | `game_engine_cleanup_dropped_objects` | 472 | 0.45 | 4 |
| `0x45f510` | `game_engine_notify_item_expired` | 77 | 0.4 | 1 |
| `0x45f560` | `game_engine_update_item_scale_and_pickup` | 374 | 0.35 | 1 |
| `0x45f6e0` | `random_advance_draws` | 53 | 0.25 | 2 |
| `0x45f720` | `tag_reflexive_pick_weighted_random_index` | 145 | 0.3 | 3 |
| `0x45f7c0` | `netgame_equipment_game_type_matches` | 128 | 0.35 | 1 |
| `0x45f850` | `game_engine_dispatch_item_pickup_event` | 147 | 0.2 | 5 |
| `0x45f8f0` | `game_engine_spawn_or_replay_netgame_equipment` | 248 | 0.2 | 7 |
| `0x45f9f0` | `game_engine_update_netgame_equipment` | 598 | 0.25 | 10 |
| `0x45fc80` | `game_engine_sync_variant_defaults` | 145 | 0.35 | 6 |
| `0x45fd20` | `game_engine_clear_unit_shields_when_disabled` | 102 | 0.4 | 2 |
| `0x45fd90` | `game_engine_begin_end_game_sequence` | 86 | 0.6 | 1 |
| `0x45fdf0` | `game_engine_update_end_game_sequence` | 308 | 0.3 | 4 |
| `0x45ff30` | `game_engine_tick` | 706 | 0.3 | 2 |
| `0x460200` | `game_engine_on_player_death` | 1672 | 0.3 | 4 |
| `0x460890` | `game_engine_build_message_text` | 56 | 0.2 | 2 |
| `0x4608d0` | `game_engine_notify_kill_event` | 242 | 0.25 | 6 |
| `0x4609d0` | `game_engine_handle_kill_feed_network_event` | 89 | 0.55 | 4 |
| `0x460a30` | `chimera__kill_feed` | 316 | 0.35 | 3 |
| `0x460ba0` | `game_engine_broadcast_kill_feed_to_team` | 101 | 0.2 | 2 |
| `0x460c10` | `game_engine_broadcast_kill_feed_by_relationship` | 245 | 0.25 | 3 |
| `0x460d10` | `game_engine_broadcast_kill_feed_or_direct` | 159 | 0.2 | 2 |
| `0x460db0` | `game_engine_broadcast_kill_feed_gated` | 122 | 0.2 | 3 |
| `0x460e40` | `game_engine_player_has_respawn_priority` | 234 | 0.4 | 0 |
| `0x460f30` | `game_engine_player_is_eliminated` | 59 | 0.5 | 0 |
| `0x460f70` | `game_engine_player_ready_to_respawn` | 265 | 0.35 | 1 |
| `0x461080` | `game_engine_find_valid_starting_locations` | 249 | 0.55 | 3 |
| `0x461180` | `game_engine_find_one_valid_starting_location` | 40 | 0.4 | 0 |
| `0x4611b0` | `game_engine_resolve_player_team` | 64 | 0.4 | 2 |
| `0x4611f0` | `game_engine_spawn_player_starting_loadout` | 450 | 0.3 | 4 |
| `0x4613c0` | `game_engine_apply_player_grenade_counts` | 366 | 0.25 | 4 |
| `0x461550` | `game_engine_compute_time_scale` | 192 | 0.3 | 3 |
| `0x461610` | `game_engine_is_inactive` | 23 | 0.9 | 0 |
| `0x461630` | `game_engine_update_teleporter` | 1096 | 0.2 | 16 |
| `0x461a80` | `game_engine_maybe_render_post_game` | 69 | 0.6 | 0 |
| `0x461ad0` | `game_engine_rate_location_crowding` | 388 | 0.4 | 0 |
| `0x461c60` | `game_engine_rate_location_ally_bonus` | 300 | 0.4 | 2 |
| `0x461d90` | `game_engine_rate_player_starting_location` | 194 | 0.55 | 0 |
| `0x461e60` | `game_engine_location_blocked_by_vehicle` | 186 | 0.6 | 1 |
| `0x461f20` | `hud_draw_teammate_nameplate_text` | 220 | 0.3 | 1 |
| `0x462000` | `game_engine_notify_weapon_ready_state_change` | 191 | 0.3 | 2 |
| `0x4620c0` | `custom_waypoint_matches_filter` | 204 | 0.2 | 2 |
| `0x462190` | `game_engine_collect_matching_waypoints` | 150 | 0.25 | 2 |
| `0x462230` | `custom_waypoint_get_position` | 33 | 0.6 | 0 |
| `0x462260` | `custom_waypoint_register` | 100 | 0.35 | 2 |
| `0x4622d0` | `game_engine_get_variant_by_name` | 1968 | 0.35 | 2 |
| `0x462a80` | `game_engine_rasterize_message` | 11 | 0.9 | 0 |
| `0x462a90` | `game_engine_update_custom_waypoint_navpoints` | 313 | 0.2 | 4 |
| `0x462bd0` | `game_engine_pack_object_flags_or_passthrough` | 24 | 0.2 | 1 |
| `0x462bf0` | `game_engine_get_teams_enabled` | 18 | 0.9 | 0 |
| `0x462c10` | `game_engine_object_flag_bit3_clear` | 32 | 0.4 | 0 |
| `0x462c30` | `game_engine_resolve_multiplayer_placement` | 405 | 0.15 | 1 |
| `0x462df0` | `game_engine_resolve_netgame_flag_role` | 475 | 0.75 | 2 |
| `0x4630b0` | `game_engine_remap_placement_by_type` | 68 | 0.2 | 1 |
| `0x463100` | `game_engine_player_respawn_priority_gate` | 75 | 0.4 | 1 |
| `0x463150` | `game_engine_pick_hud_hint` | 320 | 0.2 | 5 |
| `0x463290` | `game_engine_get_player_color` | 101 | 0.4 | 2 |
| `0x463300` | `unit_has_must_be_readied_weapon` | 148 | 0.3 | 0 |
| `0x4633a0` | `unit_reset_gauge_if_flagged` | 72 | 0.35 | 0 |
| `0x4633f0` | `game_engine_get_multiplayer_text_list` | 130 | 0.5 | 2 |
| `0x463480` | `game_engine_compare_score_to_others` | 347 | 0.15 | 2 |
| `0x4635e0` | `game_engine_scores_tracked_individually` | 56 | 0.5 | 0 |
| `0x463620` | `game_engine_player_round_reset` | 50 | 0.4 | 1 |
| `0x463660` | `game_engine_is_object_winning` | 195 | 0.2 | 1 |
| `0x463730` | `game_engine_is_tracked_object_winner` | 130 | 0.3 | 1 |
| `0x4637c0` | `game_engine_scan_netgame_flags_noop` | 76 | 0.2 | 2 |
| `0x463810` | `game_engine_validate_scenario_placements_noop` | 156 | 0.2 | 3 |
| `0x4638b0` | `game_engine_free_custom_variant_cache` | 111 | 0.8 | 0 |
| `0x463920` | `game_engine_is_map_and_variant_valid` | 81 | 0.2 | 4 |
| `0x463980` | `game_engine_variant_add_to_history` | 397 | 0.2 | 5 |
| `0x463b20` | `game_engine_ensure_variant_history_has_entry` | 108 | 0.35 | 2 |
| `0x463b90` | `game_engine_apply_current_custom_variant` | 176 | 0.3 | 6 |
| `0x463c40` | `game_engine_variant_defaults_classic_slayer` | 216 | 0.6 | 0 |
| `0x463d20` | `game_engine_variant_defaults_classic_slayer_pro` | 216 | 0.6 | 0 |
| `0x463e00` | `game_engine_variant_defaults_classic_elimination` | 216 | 0.6 | 0 |
| `0x463ee0` | `game_engine_variant_defaults_classic_phantoms` | 217 | 0.6 | 0 |
| `0x463fc0` | `game_engine_variant_defaults_classic_endurance` | 221 | 0.6 | 0 |
| `0x4640a0` | `game_engine_variant_defaults_classic_rockets` | 216 | 0.6 | 0 |
| `0x464180` | `game_engine_variant_defaults_classic_snipers` | 220 | 0.6 | 0 |
| `0x464260` | `game_engine_variant_defaults_classic_team_slayer` | 217 | 0.6 | 0 |
| `0x464340` | `game_engine_variant_defaults_classic_oddball` | 240 | 0.6 | 0 |
| `0x464430` | `game_engine_variant_defaults_classic_team_oddball` | 224 | 0.55 | 0 |
| `0x464510` | `game_engine_variant_defaults_classic_reverse_tag` | 240 | 0.85 | 0 |
| `0x464600` | `game_engine_variant_defaults_classic_accumulation` | 241 | 0.85 | 0 |
| `0x464700` | `game_engine_variant_defaults_classic_juggernaut` | 243 | 0.85 | 0 |
| `0x464800` | `game_engine_variant_defaults_classic_stalker` | 250 | 0.85 | 0 |
| `0x464900` | `game_engine_variant_defaults_classic_king` | 203 | 0.85 | 0 |
| `0x4649d0` | `game_engine_variant_defaults_classic_king_pro` | 206 | 0.85 | 0 |
| `0x464aa0` | `game_engine_variant_defaults_classic_crazy_king` | 202 | 0.85 | 0 |
| `0x464b70` | `game_engine_variant_defaults_classic_team_king` | 206 | 0.85 | 0 |
| `0x464c40` | `game_engine_variant_defaults_classic_ctf` | 230 | 0.85 | 0 |
| `0x464d30` | `game_engine_variant_defaults_classic_ctf_pro` | 230 | 0.85 | 0 |
| `0x464e20` | `game_engine_variant_defaults_classic_invasion` | 230 | 0.85 | 0 |
| `0x464f10` | `game_engine_variant_defaults_classic_iron_ctf` | 230 | 0.85 | 0 |
| `0x465000` | `game_engine_variant_defaults_classic_race` | 209 | 0.85 | 0 |
| `0x4650e0` | `game_engine_variant_defaults_classic_rally` | 213 | 0.85 | 0 |
| `0x4651c0` | `game_engine_variant_defaults_classic_team_race` | 209 | 0.85 | 0 |
| `0x4652a0` | `game_engine_variant_defaults_classic_team_rally` | 210 | 0.85 | 0 |
| `0x465380` | `game_variant_option_default_by_index` | 66 | 0.6 | 3 |
| `0x4653f0` | `hud_draw_world_relative_text` | 474 | 0.45 | 6 |
| `0x4655d0` | `game_engine_multiplayer_ui_state_id` | 167 | 0.55 | 4 |
| `0x465690` | `game_engine_rasterize_in_game_score` | 3226 | 0.15 | 30 |
| `0x466340` | `game_engine_local_player_score_is_nonpositive` | 77 | 0.45 | 2 |
| `0x466390` | `unit_current_weapon_prevents_camo_depower` | 133 | 0.75 | 0 |
| `0x466420` | `unit_update_active_camouflage_depower` | 272 | 0.7 | 1 |
| `0x466530` | `game_time_format_minutes_seconds` | 198 | 0.7 | 0 |
| `0x4666c0` | `ctf_flag_object_clear_carrier` | 99 | 0.7 | 3 |
| `0x466730` | `game_variant_sanitize_options` | 339 | 0.8 | 0 |
| `0x466890` | `game_engine_touch_multiplayer_predicted_resources` | 688 | 0.6 | 2 |
| `0x466b60` | `game_engine_is_valid_team_player` | 94 | 0.65 | 4 |
| `0x466bc0` | `game_engine_ctf_unit_weapon_must_be_readied` | 81 | 0.7 | 0 |
| `0x466c20` | `player_profile_cache_initialize` | 63 | 0.8 | 0 |
| `0x466c60` | `game_engine_player_profile_cache_add` | 65 | 0.7 | 0 |
| `0x466cb0` | `game_engine_player_profile_cache_sync_all` | 73 | 0.55 | 2 |
| `0x466d00` | `game_engine_apply_player_profile_entry` | 344 | 0.5 | 6 |
| `0x466e60` | `game_engine_invoke_profile_post_update_callback` | 23 | 0.7 | 0 |
| `0x466e80` | `game_engine_player_profile_cache_find` | 93 | 0.75 | 0 |
| `0x466ee0` | `game_engine_capture_player_profile` | 291 | 0.45 | 4 |
| `0x467010` | `game_engine_send_player_profile_update` | 175 | 0.3 | 3 |
| `0x4670c0` | `game_engine_end_game_sequence_stage1` | 43 | 0.75 | 0 |
| `0x4670f0` | `game_engine_end_game_sequence_stage2` | 135 | 0.55 | 3 |
| `0x467180` | `game_engine_end_game_sequence_stage3` | 68 | 0.55 | 3 |
| `0x4671d0` | `game_engine_send_end_game_notification` | 95 | 0.55 | 0 |
| `0x467230` | `game_engine_dispatch_end_game_notification` | 60 | 0.55 | 4 |
| `0x467270` | `game_engine_variant_defaults_assault` | 244 | 0.85 | 0 |
| `0x467370` | `game_engine_variant_defaults_crazy_king` | 219 | 0.85 | 0 |
| `0x467450` | `game_engine_variant_defaults_stalker` | 237 | 0.8 | 1 |
| `0x467540` | `game_engine_variant_defaults_juggernaut` | 259 | 0.85 | 0 |
| `0x467650` | `game_engine_variant_defaults_king` | 219 | 0.85 | 0 |
| `0x467730` | `game_engine_variant_defaults_oddball` | 254 | 0.85 | 0 |
| `0x467830` | `game_engine_variant_defaults_race` | 225 | 0.85 | 0 |
| `0x467920` | `game_engine_variant_defaults_slayer` | 232 | 0.85 | 0 |
| `0x467a10` | `game_engine_variant_defaults_team_king` | 223 | 0.85 | 0 |
| `0x467af0` | `game_engine_variant_defaults_team_oddball` | 258 | 0.85 | 0 |
| `0x467c00` | `game_engine_variant_defaults_team_race` | 226 | 0.85 | 0 |
| `0x467cf0` | `game_engine_variant_defaults_team_slayer` | 234 | 0.85 | 0 |
| `0x467de0` | `game_engine_reset_all_unit_grenade_counts` | 119 | 0.65 | 1 |
| `0x467e60` | `game_engine_reset_respawns_and_cleanup_bipeds` | 267 | 0.55 | 3 |
| `0x467f70` | `game_engine_cleanup_stray_projectiles` | 139 | 0.6 | 1 |
| `0x468010` | `game_engine_cleanup_stray_items` | 308 | 0.45 | 3 |
| `0x468150` | `game_engine_reset_player_profile_stats` | 67 | 0.6 | 1 |
| `0x4681a0` | `game_engine_reset_vehicles_or_race_cleanup` | 192 | 0.5 | 0 |
| `0x468260` | `game_engine_reset_round_objects` | 82 | 0.7 | 2 |
| `0x4682c0` | `game_engine_send_round_reset_message` | 96 | 0.55 | 2 |
| `0x468320` | `game_engine_apply_partial_round_reset_message` | 62 | 0.5 | 1 |
| `0x468360` | `game_engine_ctf_create_flag_object` | 196 | 0.4 | 1 |
| `0x468430` | `game_engine_ctf_respawn_team_flag` | 35 | 0.45 | 3 |
| `0x468460` | `game_engine_ctf_notify_both_teams` | 64 | 0.35 | 1 |
| `0x468840` | `game_engine_ctf_reset_team_return_credit` | 101 | 0.4 | 4 |
| `0x4688b0` | `game_engine_ctf_player_drop_flag` | 85 | 0.55 | 2 |
| `0x468910` | `game_engine_ctf_player_touch_flag` | 121 | 0.5 | 1 |
| `0x468990` | `game_engine_ctf_point_within_team_flag_radius` | 77 | 0.5 | 0 |
| `0x4689e0` | `game_engine_ctf_notify_flag_carried_throttled` | 51 | 0.75 | 0 |
| `0x468b50` | `game_engine_find_player_holding_object` | 150 | 0.6 | 0 |
| `0x468bf0` | `ctf_engine_flag_tick` | 1415 | 0.25 | 12 |
| `0x469780` | `game_engine_ctf_unit_is_flag_holder` | 96 | 0.45 | 2 |
| `0x4697e0` | `game_engine_ctf_player_flag_tick` | 374 | 0.3 | 6 |
| `0x46a130` | `point3d_array_project_to_xy_plane` | 125 | 0.55 | 0 |
| `0x46a1b0` | `game_engine_pick_random_recent_location` | 126 | 0.55 | 2 |
| `0x46a240` | `game_engine_koth_build_hill_boundary` | 709 | 0.25 | 8 |
| `0x46a670` | `game_engine_koth_build_hill_boundary_fence` | 1001 | 0.35 | 2 |
| `0x46aa60` | `game_engine_koth_player_in_hill_bounds` | 157 | 0.6 | 0 |
| `0x46ab00` | `game_engine_koth_player_tick` | 424 | 0.35 | 3 |
| `0x46acb0` | `game_engine_koth_update_hill_occupancy_state` | 546 | 0.25 | 4 |
| `0x46b250` | `game_engine_koth_reset_hill_marker_history` | 156 | 0.7 | 0 |
| `0x46b2f0` | `game_engine_koth_submit_hill_marker_geometry` | 1008 | 0.1 | 23 |
| `0x46b7f0` | `game_engine_koth_broadcast_hill_times` | 297 | 0.3 | 6 |
| `0x46bbd0` | `game_engine_queue_status_sound_message` | 199 | 0.4 | 2 |
| `0x46bca0` | `game_engine_handle_sound_status_event` | 84 | 0.45 | 2 |
| `0x46bd00` | `game_engine_play_multiplayer_sound` | 115 | 0.35 | 2 |
| `0x46bd80` | `game_engine_multiplayer_sound_queue_tick` | 84 | 0.6 | 1 |
| `0x46bde0` | `game_engine_get_multiplayer_sound_duration_ticks` | 87 | 0.5 | 0 |
| `0x46be40` | `game_engine_queue_multiplayer_sound` | 101 | 0.25 | 4 |
| `0x46beb0` | `game_engine_koth_find_marker_position` | 296 | 0.4 | 0 |
| `0x46bfe0` | `game_engine_koth_relocate_hill_marker` | 149 | 0.3 | 3 |
| `0x46c1a0` | `game_engine_koth_relocate_object_hill` | 143 | 0.35 | 1 |
| `0x46c230` | `game_engine_koth_alt_scorer_tick` | 231 | 0.5 | 1 |
| `0x46c320` | `game_engine_koth_update_occupant_table` | 185 | 0.4 | 0 |
| `0x46c3e0` | `game_engine_koth_dispatch_player_scoring` | 484 | 0.25 | 4 |
| `0x46c5d0` | `game_engine_koth_ball_idle_tick` | 382 | 0.3 | 5 |
| `0x46ce10` | `game_engine_koth_player_eligible_to_score` | 143 | 0.35 | 1 |
| `0x46d060` | `game_engine_koth_broadcast_team_scores` | 354 | 0.25 | 3 |
| `0x46d520` | `game_engine_find_nearest_unused_type4_location` | 167 | 0.5 | 0 |
| `0x46d800` | `game_engine_ctf_assign_flag_ids` | 133 | 0.6 | 0 |
| `0x46d890` | `game_engine_ctf_initialize_flags` | 528 | 0.4 | 1 |
| `0x46db70` | `game_engine_check_bucket_scores_and_end_round` | 615 | 0.2 | 10 |
| `0x46dde0` | `game_engine_ctf_on_flag_captured` | 324 | 0.35 | 3 |
| `0x46df30` | `game_engine_ctf_is_flag_eligible_for_capture` | 175 | 0.35 | 1 |
| `0x46dfe0` | `game_engine_ctf_pick_random_flag` | 160 | 0.6 | 0 |
| `0x46e080` | `game_engine_ctf_score_flag` | 213 | 0.35 | 1 |
| `0x46e250` | `game_engine_team_has_scoring_capacity` | 186 | 0.4 | 4 |
| `0x46e310` | `game_engine_apply_catchup_speed_boost` | 227 | 0.4 | 2 |
| `0x46ec10` | `game_engine_ctf_broadcast_state` | 275 | 0.35 | 0 |
| `0x46efe0` | `game_engine_ctf_return_all_flags` | 438 | 0.4 | 2 |
| `0x46f1a0` | `game_engine_player_select_random_target` | 475 | 0.35 | 5 |
| `0x46f450` | `game_engine_animate_hill_pulse_icons` | 239 | 0.4 | 2 |
| `0x46fe10` | `weapon_get_zoom_fov` | 85 | 0.6 | 2 |
| `0x46fe70` | `weapon_get_zoom_fov_resolved` | 136 | 0.35 | 3 |
| `0x46ff00` | `game_engine_attribute_player_death` | 1483 | 0.35 | 9 |
| `0x4704d0` | `game_engine_send_team_allegiance_message` | 285 | 0.45 | 5 |
| `0x4705f0` | `player_customization_slot_set` | 49 | 0.4 | 1 |
| `0x470630` | `player_set_team_by_color` | 89 | 0.45 | 2 |
| `0x470690` | `game_engine_gather_team_score_totals` | 136 | 0.4 | 2 |
| `0x470720` | `game_engine_team_is_leading` | 109 | 0.5 | 0 |
| `0x470790` | `game_engine_team_close_game_check` | 114 | 0.3 | 3 |
| `0x470810` | `game_engine_update_lead_change_state` | 508 | 0.3 | 6 |
| `0x470a10` | `game_engine_client_apply_team_assignment` | 112 | 0.5 | 0 |
| `0x470a80` | `game_engine_allocate_tick_record` | 86 | 0.75 | 0 |
| `0x470ae0` | `game_engine_init_tick_record_for_mode` | 64 | 0.6 | 0 |
| `0x470b30` | `game_engine_accumulate_simulation_ticks` | 189 | 0.55 | 2 |
| `0x470bf0` | `game_engine_advance_simulation_ticks` | 223 | 0.45 | 1 |
| `0x470cd0` | `game_engine_get_current_tick` | 9 | 0.9 | 0 |
| `0x470ce0` | `game_engine_get_time_scale` | 34 | 0.85 | 0 |
| `0x470d10` | `angle_delta_wrapped` | 47 | 0.85 | 0 |
| `0x470d40` | `value_step_toward_target` | 64 | 0.6 | 0 |
| `0x470d80` | `game_engine_compute_look_angles_from_vector` | 90 | 0.6 | 0 |
| `0x470de0` | `game_engine_reset_player_look_state` | 155 | 0.5 | 1 |
| `0x470e80` | `game_engine_init_player_look_state_from_object` | 303 | 0.5 | 2 |
| `0x470fb0` | `response_curve_evaluate` | 179 | 0.35 | 2 |
| `0x471070` | `control_axis_sign` | 57 | 0.7 | 0 |
| `0x4710b0` | `game_engine_build_local_player_control_input` | 2595 | 0.55 | 8 |
| `0x471ae0` | `game_engine_update_local_player_control` | 1108 | 0.6 | 8 |
| `0x471f40` | `game_engine_compute_local_player_look_vector` | 73 | 0.4 | 0 |
| `0x471f90` | `game_engine_get_max_look_pitch` | 131 | 0.3 | 2 |
| `0x472020` | `chimera__spectate_fp_camera_position` | 222 | 0.35 | 4 |
| `0x472100` | `unit_set_local_player_weapon_index` | 83 | 0.55 | 0 |
| `0x472160` | `game_engine_update_local_player_look` | 1346 | 0.15 | 10 |
| `0x4726b0` | `unit_get_local_player_weapon_index` | 60 | 0.5 | 0 |
| `0x4726f0` | `unit_invalidate_local_player_zoom_level` | 80 | 0.55 | 0 |
| `0x472740` | `local_player_get_zoom_level` | 27 | 0.6 | 0 |
| `0x472760` | `game_engine_digitize_control_input` | 543 | 0.3 | 7 |
| `0x472aa0` | `update_server_new` | 86 | 0.6 | 0 |
| `0x472b00` | `update_queues_dispose` | 109 | 0.75 | 0 |
| `0x472b70` | `update_server_dispose` | 280 | 0.3 | 2 |
| `0x472c90` | `update_server_queue_create_entry` | 40 | 0.75 | 0 |
| `0x472cc0` | `update_server_push_player_tick_history` | 480 | 0.15 | 5 |
| `0x472ea0` | `update_server_queue_get_history_entry` | 145 | 0.2 | 1 |
| `0x472f40` | `update_client_new` | 93 | 0.6 | 0 |
| `0x472fa0` | `update_client_dispose` | 240 | 0.3 | 2 |
| `0x473090` | `update_client_stage_entry` | 59 | 0.35 | 4 |
| `0x4730d0` | `update_client_queue_apply_tick` | 416 | 0.2 | 5 |
| `0x473270` | `update_client_distribute_staged_entry` | 152 | 0.25 | 5 |
| `0x473310` | `update_run_catchup_ticks` | 116 | 0.2 | 6 |
| `0x473390` | `update_server_queue_push_history` | 160 | 0.35 | 0 |
| `0x473430` | `game_engine_find_player_by_name` | 118 | 0.15 | 3 |
| `0x4734b0` | `update_client_advance_read_cursor` | 76 | 0.15 | 5 |
| `0x473500` | `update_client_queue_get_slot` | 86 | 0.45 | 0 |
| `0x473560` | `random_get_table_point` | 78 | 0.6 | 0 |
| `0x4735b0` | `players_initialize` | 181 | 0.9 | 0 |
| `0x473670` | `players_dispose` | 95 | 0.85 | 0 |
| `0x4736d0` | `players_any_with_local_player_index` | 88 | 0.7 | 0 |
| `0x473730` | `local_player_find_free_slot_index` | 73 | 0.6 | 2 |
| `0x473780` | `player_new_network` | 438 | 0.45 | 0 |
| `0x473940` | `player_new_local` | 412 | 0.45 | 1 |
| `0x473ae0` | `player_delete` | 204 | 0.45 | 3 |
| `0x473bb0` | `player_remove` | 150 | 0.4 | 6 |
| `0x473c50` | `unit_apply_starting_profile` | 278 | 0.4 | 2 |
| `0x473d70` | `player_compute_view_forward_vector` | 282 | 0.35 | 3 |
| `0x473e90` | `game_engine_attach_players_to_new_bsp` | 517 | 0.3 | 4 |
| `0x4740a0` | `game_engine_players_update_server` | 1262 | 0.2 | 9 |
| `0x474590` | `game_engine_players_update_client` | 1034 | 0.25 | 6 |
| `0x4749a0` | `main_switch_structure_bsp` | 902 | 0.3 | 10 |
| `0x474d30` | `local_player_to_player_index` | 30 | 0.8 | 0 |
| `0x474d50` | `game_set_local_player` | 84 | 0.75 | 0 |
| `0x474db0` | `player_index_from_unit_index` | 86 | 0.5 | 1 |
| `0x474e10` | `player_reset_after_unit_change` | 414 | 0.45 | 4 |
| `0x474fc0` | `local_player_set_controlled_unit` | 201 | 0.65 | 0 |
| `0x475090` | `players_any_pending_seat_or_respawn` | 354 | 0.3 | 3 |
| `0x475210` | `players_any_without_unit` | 82 | 0.85 | 0 |
| `0x475270` | `game_engine_reattach_player_unit_unused` | 1331 | 0.2 | 17 |
| `0x4757b0` | `player_find_placement_position` | 1181 | 0.25 | 10 |
| `0x475c60` | `player_attach_unit_to_parent` | 1096 | 0.25 | 14 |
| `0x4760b0` | `player_release_unit_and_reset` | 408 | 0.35 | 4 |
| `0x476250` | `player_kill_and_release_unit` | 157 | 0.5 | 2 |
| `0x476760` | `game_engine_server_update_player_positions` | 231 | 0.4 | 2 |
| `0x476cf0` | `player_apply_first_position_update` | 79 | 0.55 | 0 |
| `0x476d40` | `players_client_catchup_on_server_updates` | 1224 | 0.2 | 9 |
| `0x477210` | `player_unit_has_parent` | 111 | 0.55 | 0 |
| `0x477280` | `players_find_local_owned_unclear` | 87 | 0.5 | 0 |
| `0x4772e0` | `unit_snap_position_if_far` | 99 | 0.4 | 3 |
| `0x477350` | `apply_remote_player_position_update` | 318 | 0.35 | 3 |
| `0x477490` | `apply_remote_player_vehicle_position_update` | 428 | 0.25 | 7 |
| `0x477640` | `game_get_player_starting_location` | 42 | 0.75 | 0 |
| `0x477670` | `object_placement_data_set_change_colors` | 87 | 0.9 | 0 |
| `0x4776d0` | `player_pick_random_starting_location` | 312 | 0.55 | 2 |
| `0x477810` | `player_spawn_starting_profile_weapon` | 169 | 0.5 | 1 |
| `0x4778c0` | `game_engine_apply_player_join_message` | 265 | 0.3 | 7 |
| `0x4779d0` | `game_engine_notify_object_value_event` | 175 | 0.4 | 3 |
| `0x477a80` | `game_engine_send_unit_weapon_loadout` | 481 | 0.4 | 6 |
| `0x477c70` | `game_engine_apply_player_spawn_loadout_message` | 555 | 0.3 | 11 |
| `0x477ea0` | `player_respawn` | 1010 | 0.35 | 8 |
| `0x4782a0` | `game_engine_build_visible_cluster_bitmask` | 347 | 0.25 | 6 |
| `0x478400` | `player_update_nearby_interactions_primary` | 212 | 0.45 | 1 |
| `0x478500` | `player_update_nearby_interactions_secondary` | 212 | 0.45 | 0 |
| `0x478600` | `player_check_vehicle_interaction` | 366 | 0.4 | 3 |
| `0x478770` | `player_check_assassination_opportunity` | 172 | 0.5 | 1 |
| `0x478820` | `player_is_busy_with_interaction` | 114 | 0.3 | 2 |
| `0x4788a0` | `player_check_vehicle_boarding_interaction` | 926 | 0.2 | 5 |
| `0x478c40` | `player_check_vehicle_boarding_interaction_lightweight` | 444 | 0.25 | 3 |
| `0x478e00` | `player_set_pending_interaction_action` | 262 | 0.5 | 1 |
| `0x478f10` | `game_engine_apply_player_interaction_message` | 221 | 0.3 | 3 |
| `0x478ff0` | `game_engine_notify_player_interaction` | 223 | 0.45 | 1 |
| `0x4790d0` | `player_execute_weapon_drop_interaction` | 353 | 0.25 | 1 |
| `0x479240` | `player_swap_to_weapon` | 351 | 0.3 | 1 |
| `0x4793a0` | `player_execute_pending_interaction` | 837 | 0.2 | 4 |
| `0x479710` | `player_trigger_shield_recharge_effect` | 179 | 0.35 | 1 |
| `0x4797d0` | `player_trigger_kill_streak_effect` | 179 | 0.35 | 1 |
| `0x479890` | `player_trigger_full_health_effect` | 157 | 0.4 | 0 |
| `0x479930` | `player_apply_pickup_effect` | 361 | 0.25 | 6 |
| `0x479aa0` | `player_notify_kill_streak_update` | 145 | 0.4 | 2 |
| `0x479b40` | `game_engine_apply_kill_streak_message` | 82 | 0.5 | 1 |
| `0x479ba0` | `player_add_kill_streak` | 255 | 0.4 | 1 |
| `0x479ca0` | `player_kill_streak_set_max` | 99 | 0.6 | 0 |
| `0x479d10` | `player_kill_streak_tick` | 121 | 0.55 | 1 |
| `0x479d90` | `player_kill_streak_begin` | 67 | 0.6 | 0 |
| `0x479de0` | `player_kill_streak_continue` | 58 | 0.6 | 2 |
| `0x479eb0` | `player_advance_multikill_medal` | 141 | 0.45 | 2 |
| `0x479f40` | `player_update_queue_create` | 108 | 0.6 | 0 |
| `0x479fb0` | `player_update_queue_pop_current` | 108 | 0.8 | 2 |
| `0x47a020` | `position_update_queue_create` | 99 | 0.7 | 0 |
| `0x47a090` | `network_queue_destroy` | 35 | 0.7 | 0 |
| `0x47a0c0` | `position_update_queue_push` | 56 | 0.5 | 1 |
| `0x47a100` | `position_update_queue_find_and_remove` | 150 | 0.4 | 1 |
| `0x47a1a0` | `circular_queue_push` | 82 | 0.75 | 0 |
| `0x47a200` | `circular_queue_pop` | 45 | 0.75 | 0 |
| `0x47a230` | `circular_queue_count` | 23 | 0.8 | 0 |
| `0x47a250` | `vehicle_update_queue_create` | 99 | 0.7 | 0 |
| `0x47a2c0` | `vehicle_update_queue_find_and_remove` | 145 | 0.4 | 1 |
| `0x47b140` | `player_examine_nearby_vehicle` | 100 | 0.45 | 1 |
| `0x47b940` | `player_set_action_result` | 86 | 0.55 | 0 |
| `0x47c310` | `player_camo_screen_effect` | 45 | 0.45 | 2 |
| `0x53e060` | `savegame_index_file_exists` | 115 | 0.4 | 2 |
| `0x53e0e0` | `savegame_index_read_slot` | 266 | 0.4 | 5 |
| `0x53e1f0` | `savegame_index_write_slot` | 267 | 0.4 | 3 |
| `0x53e300` | `savegame_index_append_slot` | 276 | 0.4 | 3 |
| `0x53e420` | `savegame_index_get_slot_count` | 122 | 0.45 | 2 |
| `0x53e4a0` | `savegame_index_remove_slot` | 400 | 0.35 | 5 |
| `0x53e630` | `savegame_slot_handle_pack` | 36 | 0.4 | 1 |
| `0x551620` | `user_profile_signin_state_is_valid` | 35 | 0.4 | 2 |
| `0x551650` | `user_save_path_register` | 73 | 0.7 | 0 |
| `0x5516a0` | `user_save_path_lookup` | 40 | 0.6 | 2 |
| `0x5516d0` | `user_save_path_remove` | 64 | 0.65 | 2 |
| `0x551710` | `XCreateSaveGame` | 653 | 0.35 | 4 |
| `0x5519a0` | `XDeleteSaveGame` | 528 | 0.35 | 2 |
| `0x551bc0` | `savegame_find_first` | 356 | 0.3 | 3 |
| `0x551d30` | `savegame_find_next` | 251 | 0.3 | 4 |
