# scenario module: type recovery notes

Header: `types/scenario.h`. Syntax gate: `out/phase4/scenario_smoke.c`
(`C:\msys64\ucrt64\bin\gcc.exe -fsyntax-only -I types out/phase4/scenario_smoke.c` passes. Every
size and offset below is also a compile-time check there. None of the new records holds a pointer,
so the checks fire on the 64-bit host compiler too.)

All the offsets come from the disassembly (objdump of 0x53e660..0x53f150), not just from Ghidra.
The register arguments below were read from the callee prologues and the call sites.

## What the module owns

Most of what these 17 functions touch is tag data, and `types/tags.h` already has the layout for
it. That layout is reused as-is. Every offset the code uses matches it (see the table below). The
header defines only the module's own records:

| Record | Size | Established by |
|---|---|---|
| `scenario_game_globals` | 0x7c | 0x45a9c0 (the game-state carve: `crc32_update` of 0x7c, pointer stored in 0x00746f94); 0x53eeb0 / 0x53efc0 (WORD at +0x00); game-state reset 0x45b050 (`rep stosd` of 0xb dwords from +0x04, byte clear at +0x30, 0x12-dword copy of `k_default_sound_environment` 0x0065e508 to +0x34); 0x53e8c0 (+0x04, stride 0x2c); sound 0x53f150 (+0x30 byte, +0x34 SoundEnvironment) |
| `scenario_sky_fog_state` | 0x2c | 0x53e8c0: snap path 0x53ea72 copies Sky fog +0x18/+0x1c/+0x14/+0x00..+0x08 into +0x10/+0x14/+0x18/+0x1c..+0x24 and the screen blend into +0x28, then sets +0x00 = 1. The blend path passes `&state+0x10/+0x14/+0x18/+0x28` to 0x470d40 and `&state+0x1c` to 0x50f520 (color). The distance test reads +0x04..+0x0c, and every exit rewrites them |
| `scenario_trigger_volume_box` (union of `_fixed_box` / `_rotational_box`) | 0x18 | 0x53f020. It overlays `ScenarioTriggerVolume` +0x48..+0x5f, and the tag struct itself is not redefined |
| `structure_bsp_procedure` | fn ptr | 0x53e660 / 0x53e680: `call [esi]` with no arguments, result ignored |
| enums `scenario_constants`, `scenario_cluster_fog_bits`, `scenario_tag_flag_bits` | | loop counts 10 / 13, retry limit 0x96, cluster fog 0x8000 / 0x7fff decode in 0x53ec30 / 0x53ee00, flag bit 0 tests |

Every byte is accounted for. The unknowns are only alignment bytes that nothing reads or writes:
`scenario_game_globals.unknown_02`, `.unknown_31[3]`, and `scenario_sky_fog_state.unknown_01[3]`.

### Tag offsets confirmed against types/tags.h (no change needed)

- **Scenario:** skies +0x30/+0x34 (0x53e8c0). object_names +0x204/+0x208, stride 0x24 (0x53ebb0,
  with the scenario in ECX). trigger_volumes.pointer +0x364, stride 0x60 (0x53f020).
  netgame_equipment +0x384/+0x388, stride 0x90: scenario_load stores -1 into +0x10 (`unknown_ffffffff`),
  which is the runtime item handle. structure_bsps +0x5a4/+0x5a8, stride 0x20, with
  structure_bsp.tag_id at +0x1c (0x53eeb0, 0x53efc0).
- **ScenarioStructureBSP:** collision_bsp.pointer +0xb4. leaves.pointer +0xe4 (cluster at +0x08,
  0x53e780). clusters +0x134/+0x138 (sky +0, fog +2, background_sound +4, weather +8).
  cluster_data.pointer +0x14c (PVS rows, 0x53eb60). fog_planes +0x17c. fog_regions +0x188
  (fog +0x24, weather_palette +0x26). fog_palette +0x194 (fog.tag_id +0x2c). background_sound_palette
  +0x1fc/+0x200 (tag_id +0x2c).
- **Globals:** materials +0x194/+0x198, stride 0x374 (0x53e7c0).
- **GlobalsMaterial:** melee_hit_sound.tag_id at +0x370. 0x53e7c0 stores -1 to 0x006e3578, which is
  0x006e3208 + 0x370.
- **Sky:** outdoor fog block +0x58, indoor fog block +0x78, indoor_fog_screen.tag_id +0xa4.
- **Fog:** flags bit 0 (is_water), distance_to_water_plane +0x74.
- **SoundLooping:** flags bit 0 (deafening_to_ais).

### Records reused from other headers

- `bsp_leaf_reference` (types/objects.h) is Blam's `scenario_location`. 0x53e780 fills it through
  ESI: `[esi] = leaf` and `[esi+4] = WORD cluster`. 0x53e810, 0x53ec30, 0x53ed60 and 0x53ee00 read
  `cluster_index` at +0x04 through EAX.
- `render_fog` (types/rasterizer.h) is filled by 0x53e8c0 through its third stack argument (the
  caller is 0x50ba80, which passes 0x007c32f4). It writes atmospheric_color +0x04, maximum_density
  +0x10, minimum_distance +0x14 and maximum_distance +0x18 (= max(start + 0.0001, opaque), or 0 when
  opaque is 0). It also writes **+0x4c**, the sky fog screen blend clamped to [0, 1]. So
  `render_fog.unknown_4c` can be renamed `sky_fog_screen_blend`. That header is not edited here.

## Register conventions (from call sites and prologues)

- **0x53e780:** ESI = `bsp_leaf_reference *out`, EDX = point (forwarded to bsp3d_node_find_leaf
  0x5013a0, which takes ECX = collision bsp, EAX = 0 root node, EDX = point).
- **0x53e7c0:** AX = material index. Returns `GlobalsMaterial *`.
- **0x53e810:** EAX = `bsp_leaf_reference *`. Returns a bool in AL.
- **0x53e870:** EDX = `real_point3d *point`. It moves point.z up 0.05 on each failed probe, 150 times at
  most. It returns true when the first probe already hit a leaf.
- **0x53e8c0:** AX = sky index (-1 means the cluster has no sky). Stack arguments: int16
  local_player_index, `real_point3d *camera_position`, `render_fog *out`.
- **0x53eb60:** CX = column cluster, stack int16 = row cluster.
- **0x53ebb0:** ECX = `Scenario *`, stack = name. Returns an int16 index, or -1.
- **0x53ec30:** EAX = `bsp_leaf_reference *`, EBX = point or NULL. Returns an int16 fog region.
- **0x53ed10:** AX = fog region. Returns a fog tag handle or -1.
- **0x53ed60:** stack arguments `(bsp_leaf_reference *, int16 *weather_index_out)`, with EBX = point
  passed through to 0x53ec30. Returns is_water.
- **0x53ee00:** EAX = `bsp_leaf_reference *`, EDI = point. Returns a float in ST0.
- **0x53eeb0:** SI = bsp index. Returns a bool.
- **0x53f020:** AX = trigger volume index, ECX = point. Returns a bool.

## Globals

The header owns 0x0069e8d4, 0x0069e8d8, 0x00746f8c, 0x00746f90, 0x00746f94, 0x00746f98, 0x00746f9c,
0x00746fa0, 0x0069e8dc (13 slots), 0x0069e910 (10 slots) and 0x006e3208 (0x374-byte default
material). The two tables are contiguous: 0x69e8dc + 13*4 == 0x69e910, and the smoke file checks
this.

Other headers name some of these differently, and some of those names are wrong:

- **0x00746f90:** structures.h / hs.h call it `global_globals`. It is the collision BSP: 0x53eeb0
  stores `structure_bsp +0xb4` there. Named `global_collision_bsp` here.
- **0x00746f98:** it holds the same value as 0x00746f90 (effects.h's `structure_collision_bsp`).
- **0x00746f9c:** ai.h line ~1057 / ~1151 calls it a `bsp_generation` int. It is the
  ScenarioStructureBSP tag-data pointer. hs.h calls it `global_matg_multiplayer`, which is also
  wrong.
- **0x00746f94:** structures.h attributes this pointer to the sound module. The block is scenario's:
  scenario_structure_bsp_switch writes its first word, and the sky fog state lives in it. Only
  +0x30..+0x7b is sound's.

These globals are not owned here:

- 0x00721e4c: the first-use latch of the default material. physics.h owns it, and 0x507cc0 inlines
  the same lookup.
- 0x00719769 / 0x0071976a: main-loop latches. The only other users are the main loop 0x4c7610,
  0x4c7f10 and 0x5381c0.
- 0x006a8958: owned by cache.
- 0x0065512c: the empty string.

## Unresolved

- **scenario_sky_fog_state.fog_screen_blend (+0x28):** the name comes from its source, the indoor
  fog screen dependency of sky[0]. Nothing in this module shows how render consumes render_fog +0x4c.
- **Array size of scenario_game_globals.sky_fog:** the block is 0x7c, so only index 0 fits, but the
  code multiplies by the local player index. An Xbox build with four players would have a larger
  block. `[1]` is an inference from the size.
- **Bytes +0x30 / +0x34 of scenario_game_globals:** they are typed from structures_types_notes.md
  (0x53f150), not re-derived in this pass.
- **The fixed trigger-volume type (0):** it reads +0x48..+0x5c as (lower, upper) pairs, not as
  corner + offset. The tag must be post-processed into that form at load time. The postprocess was
  not located.

## Misattributed / out-of-module functions

- **0x53e7c0 (material get):** not scenario-specific. It indexes `Globals.materials`, and physics
  (0x507cc0) inlines the same code with the same latch. It is kept in this module because it
  lives here, and scenario owns `global_game_globals`.
- **0x53e810, 0x53eb60, 0x53ec30, 0x53ed10, 0x53ed60, 0x53ee00:** these query the resident structure
  BSP (clusters, PVS, fog regions/planes). In Blam they belong to `scenario.c`
  (`scenario_location_*`, `scenario_cluster_*`), so they stay here. They introduce no new records.
- **0x53e8c0:** its output is the render module's `render_fog`, and its state lives in the scenario
  game-state block. It is kept here.
- No library code was found in the range.
