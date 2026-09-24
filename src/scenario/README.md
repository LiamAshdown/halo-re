# `scenario`: loading the map, switching the structure BSP, per-location queries

Retail Halo PC `halo.exe` 1.0.10, `0x53e660 .. 0x53f150` (17 functions, 2,630 bytes of code),
plain C / MSVC 7.1 (cl 13.10.3077, LTCG) / x86. Each file in this directory holds one function,
rewritten from its Ghidra decompilation against `types/scenario.h`. The original decompile is kept
verbatim at the bottom of each file inside `#if 0 ... #endif` so the two can be diffed. Every
register convention was confirmed from the callee prologue and the call sites in
`objdump -d -M intel`, since LTCG invents a different convention for each function.

Gate: `python tools/build_check.py scenario` gives **17 ok, 0 failed**. The header syntax gate
`gcc -fsyntax-only -I types out/phase4/scenario_smoke.c` also passes. It checks every size and
offset below at compile time.

## What the module contains

This is a thin layer between the cache file and everything that walks the loaded map. It does
three things:

| Group | Functions | What it does |
|---|---|---|
| Load and switch | `scenario_load`, `scenario_structure_bsp_switch`, `scenario_structure_bsp_switch_after_load`, `scenario_structure_bsp_{de,}activate_callbacks` | Loads the map through `cache_file_load` and resolves `global_scenario` / `global_game_globals`. Keeps one structure BSP resident, running the 10-slot deactivate table and the 13-slot activate table around each switch. After a saved game is loaded, it re-syncs the resident BSP to the game state (slot 0 of `game_state_after_load_procs`, 0x0069e7b4). |
| Location queries | `scenario_location_from_point`, `scenario_location_fog_region`, `scenario_fog_region_resolve_tag`, `scenario_location_get_water_and_weather`, `scenario_location_water_surface_distance`, `scenario_location_background_sound_is_deafening_to_ais`, `scenario_structure_bsp_locate_point_nudge_up`, `scenario_cluster_visibility_test` | Answers questions about a point in the resident BSP: which leaf and cluster it is in, which fog region applies, whether that fog is water, the weather, the signed distance to the water surface, whether the background sound deafens AIs, and whether one cluster can see another. |
| Scenario data | `scenario_trigger_volume_contains_point`, `scenario_object_name_find_index`, `globals_material_get`, `scenario_sky_fog_state_update` | Tests trigger volumes, looks up object names (for `hs_parse_object_name`), fetches a material with a static fallback, and blends the per-player sky fog that `render_player_frame` passes to the rasterizer as `render_fog`. |

A scenario location is the 8-byte `bsp_leaf_reference` from `types/objects.h` (`leaf_index` at
+0x00, `cluster_index` at +0x04). `scenario_location_from_point` is the only function that builds
one. The other `scenario_location_*` functions take it in EAX (or on the stack for 0x53ed60).

The cluster fog word, `ScenarioStructureBSPCluster.fog` at +0x02, decodes like this:

- `-1`: no fog.
- Top bit clear: a fog region index.
- Top bit set: a fog plane index. The plane's `front_region` names the region, and a point on or
  in front of the plane (distance, plus the water depth for water fog, is ≥ 0) is outside the
  region.

## Struct layouts

The tag layouts come from `types/tags.h` and are reused unchanged. The module defines only the
records below. `#pragma pack(push,1)` is in force.

### `scenario_game_globals`, size `0x7c` (game state, pointer at 0x00746f94)

| Off | Type | Field | Established by |
|---|---|---|---|
| `0x00` | `int16_t` | `structure_bsp_index` | 0x53eeb0 writes it, 0x53efc0 compares it; -1 when unloaded |
| `0x02` | `uint16_t` | `unknown_02` | never touched |
| `0x04` | `scenario_sky_fog_state[1]` | `sky_fog` | 0x53e8c0, stride 0x2c; the reset 0x45b050 zeroes 0xb dwords |
| `0x30` | `uint8_t` | `sound_environment_is_water` | sound module 0x53f150 |
| `0x31` | `uint8_t[3]` | `unknown_31` | never touched |
| `0x34` | `SoundEnvironment` | `sound_environment` | sound module 0x53f150; reset from 0x0065e508 |

### `scenario_sky_fog_state`, size `0x2c`

| Off | Type | Field | Sky source (outdoor / indoor) |
|---|---|---|---|
| `0x00` | `uint8_t` | `valid` | set by the snap path |
| `0x01` | `uint8_t[3]` | `unknown_01` | never touched |
| `0x04` | `Point3D` | `camera_position` | rewritten on every call that has a sky |
| `0x10` | `float` | `start_distance` | +0x70 / +0x90 |
| `0x14` | `float` | `opaque_distance` | +0x74 / +0x94 |
| `0x18` | `float` | `maximum_density` | +0x6c / +0x8c |
| `0x1c` | `ColorRGB` | `color` | +0x58 / +0x78 |
| `0x28` | `float` | `fog_screen_blend` | target 1.0 only on the indoor path when sky 0 has an `indoor_fog_screen` |

### `sky_fog_block`, size `0x20` (overlay for Sky +0x58 and Sky +0x78)

This type was folded in from a local typedef during the review.

| Off | Type | Field |
|---|---|---|
| `0x00` | `ColorRGB` | `color` |
| `0x0c` | `uint8_t[8]` | `unknown_0c` (tag padding) |
| `0x14` | `float` | `maximum_density` |
| `0x18` | `float` | `start_distance` |
| `0x1c` | `float` | `opaque_distance` |

### `scenario_trigger_volume_box`, size `0x18` (a union over `ScenarioTriggerVolume` +0x48..+0x5f)

| Off | `fixed` (type 0) | `rotational` (type 1) |
|---|---|---|
| `0x00` | `x_bounds[0]` | `origin.x` (`starting_corner`) |
| `0x04` | `x_bounds[1]` | `origin.y` |
| `0x08` | `y_bounds[0]` | `origin.z` |
| `0x0c` | `y_bounds[1]` | `extents.i` (`ending_corner_offset`) |
| `0x10` | `z_bounds[0]` | `extents.j` |
| `0x14` | `z_bounds[1]` | `extents.k` |

`structure_bsp_procedure` is `void (*)(void)`. The activate table has 13 entries at 0x0069e8dc,
and the deactivate table has 10 entries at 0x0069e910.

## Globals

| Address | Declaration | Note |
|---|---|---|
| 0x0069e8d4 | `datum_index global_scenario_index` | the `cache_file_load` result |
| 0x0069e8d8 | `int16_t global_structure_bsp_index` | the resident BSP, -1 when none |
| 0x00746f8c | `Scenario *global_scenario` | |
| 0x00746f90 | `ModelCollisionGeometryBSP *global_collision_bsp` | other modules call this `global_globals` (camera, hs, structures) |
| 0x00746f94 | `scenario_game_globals *global_scenario_game_globals` | |
| 0x00746f98 | `ModelCollisionGeometryBSP *global_structure_collision_bsp` | same value as 0x00746f90; elsewhere called `structure_collision_bsp` |
| 0x00746f9c | `ScenarioStructureBSP *global_structure_bsp` | elsewhere `structure_bsp`, `structure_bsp_globals`, `structure_bsp_tag_data` |
| 0x00746fa0 | `Globals *global_game_globals` | 44 files elsewhere call it `global_globals` |
| 0x006e3208 | `GlobalsMaterial k_default_global_material` | the fallback returned by `globals_material_get` |

## Functions

| Address | Name | Size | Convention | Rewrite conf. |
|---|---|---|---|---|
| 0x53e660 | `scenario_structure_bsp_deactivate_callbacks` | 27 | none | 0.9 |
| 0x53e680 | `scenario_structure_bsp_activate_callbacks` | 27 | none | 0.9 |
| 0x53e6a0 | `scenario_load` | 215 | EAX path, bool in AL | 0.75 |
| 0x53e780 | `scenario_location_from_point` | 57 | ESI out, EDX point | 0.8 |
| 0x53e7c0 | `globals_material_get` | 71 | AX index, pointer in EAX | 0.8 |
| 0x53e810 | `scenario_location_background_sound_is_deafening_to_ais` | 87 | EAX location, bool in AL | 0.7 |
| 0x53e870 | `scenario_structure_bsp_locate_point_nudge_up` | 71 | EDX point, bool in AL | 0.75 |
| 0x53e8c0 | `scenario_sky_fog_state_update` | 665 | AX sky, stack (player, camera, out) | 0.75 |
| 0x53eb60 | `scenario_cluster_visibility_test` | 65 | CX column, stack row; bool in EAX | 0.85 |
| 0x53ebb0 | `scenario_object_name_find_index` | 109 | ECX scenario, stack name; int16 in AX | 0.8 |
| 0x53ec30 | `scenario_location_fog_region` | 209 | EAX location, EBX point or NULL; int16 in AX | 0.85 |
| 0x53ed10 | `scenario_fog_region_resolve_tag` | 66 | AX region; tag id in EAX | 0.9 |
| 0x53ed60 | `scenario_location_get_water_and_weather` | 147 | EBX point (passed through), stack (location, weather out); bool in AL | 0.8 |
| 0x53ee00 | `scenario_location_water_surface_distance` | 163 | EAX location, EDI point; float in ST0 | 0.85 |
| 0x53eeb0 | `scenario_structure_bsp_switch` | 259 | SI index, bool in AL | 0.8 |
| 0x53efc0 | `scenario_structure_bsp_switch_after_load` | 96 | none | 0.85 |
| 0x53f020 | `scenario_trigger_volume_contains_point` | 296 | AX volume, ECX point, bool in AL | 0.8 |

No function in the range is library code, and none is misattributed. `0x53f150` belongs to the
sound module.

## Changes from the review pass

- The local typedef `sky_fog_block` was moved into `types/scenario.h`, and a layout check for it
  was added to `out/phase4/scenario_smoke.c`.
- `TagReflexive.count` is `uint32_t`, but every count comparison in the binary is signed
  (`jle` / `jge`). An `(int32_t)` cast was added to every one of them in 7 files.
- Several x87 comparisons also reject NaN (`test ah,5 / jp`, `test ah,0x41 / jne`). They are now
  written as negated strict tests. This affects the trigger volume box tests, the fog plane side
  test in 0x53ec30, and the 15-unit snap test in 0x53e8c0. Dot products and the distance sum now
  follow the order the FPU uses.
- Functions whose callers only test AL now return `uint8_t`: `scenario_load`,
  `scenario_location_get_water_and_weather` and `scenario_trigger_volume_contains_point` (whose
  unknown-type exit clears only AL).
- `scenario_location_from_point` now masks the leaf index with `0x7fffffff` before indexing, as
  the code at 0x53e7a7 does. `scenario_structure_bsp_switch_after_load` now reads the tag index
  sign-extended (`movsx` at 0x53eff0).
- The `matrix4x3_inverse_transform_point` extern now uses the parameter order `src/math` defines
  (m, out, point).
- 0x53e780 was renamed from `scenario_structure_bsp_find_leaf` to `scenario_location_from_point`.
  The 14 names new to the module are in `symbols/agent_phase4_scenario.txt`.

## Known gaps

- Callers outside this module still declare these functions under their `FUN_0053xxxx` names,
  often with wrong signatures:
  - `src/ai/actor_evaluate_engagement_reachability.c` passes a `datum_index` to 0x53eb60, which
    actually takes two cluster indices (CX and a stack int16).
  - `src/effects/ambient_color_marker_visible.c` passes an int16 cluster to 0x53ec30, which takes
    a location in EAX and a point in EBX.
  - `src/ai/actor_target_hearing_check.c` expects a float in ST0 from 0x53e810, which never
    touches the FPU.
  - `src/camera/observer_avoid_collision.c` says EBX is unused by 0x53ed60. It is used: 0x53ec30
    reads it.
- The rest of the codebase uses several names for 0x00746f90 / 0x00746f9c / 0x00746fa0 (see
  Globals). `global_globals` is used for two different addresses.
- `render_fog.unknown_4c` (`types/rasterizer.h`) is the sky fog screen blend. It has not been
  renamed there.
- 0x53e8c0 with `local_player_index == -1` and no sky tag reads an uninitialized stack
  `scenario_sky_fog_state`. This is kept as the original has it.
