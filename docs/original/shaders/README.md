# `shaders` — shader tag helpers and the numeric countdown timer

Retail Halo PC `halo.exe` 1.0.10, `0x53fd60 .. 0x540295` (7 functions, ~1,150 bytes of code),
plain C / MSVC 7.1 / x86 with LTCG. Every file in this directory is one function, rewritten from
its Ghidra decompilation against `types/shaders.h` (plus the tag layouts in `types/tags.h`), with
the original decompile preserved verbatim at the bottom of the file inside `#if 0 ... #endif`.

Gate: `python tools/build_check.py shaders` → **7 ok, 0 failed** (also clean under `-Wextra`).

## What the module contains

Two unrelated groups that happen to sit next to each other in the image:

| Group | Range | What it is |
|---|---|---|
| shader tag queries | `0x53fd60`–`0x53fe4d` | small switch-on-`shader_type` helpers the rasterizer calls: vertex shader permutation, decal flag, draw-before-water flag |
| texture animation | `0x53fe50`–`0x5400b2` | evaluate the periodic u/v/rotation texture animation of a shader into a 2x4 texture matrix (general form), or the u/v scroll pair of `shader_environment` |
| numeric countdown timer | `0x5400c0`–`0x540295` | the millisecond countdown that numeric `transparent_chicago` shaders display: per-tick countdown, and one decimal digit on demand |

The timer is driven by the script functions `numeric_countdown_timer_set / _get / _stop /
_restart` (hs evaluators `0x47ae10 .. 0x47aee0`), counted down by `numeric_countdown_timer_update`
from `game_effects_update` (tail call at `0x45b57f`), and read digit by digit by the chicago draws
`0x531ed0` / `0x532a40` as the bitmap frame index.

Every function takes register arguments invented by LTCG; the C parameter order is a
convention, the register mapping is in each file header (`blam-cc:` line). For `0x53fe50` and
`0x540060` the C order matches the externs the rasterizer module already declares.

## Struct layouts

All in `types/shaders.h`; tag structs (`Shader`, `ShaderModel`, `ShaderTransparentGeneric`,
`ShaderTransparentGlass`, `ShaderTransparentMeter`, `ShaderEnvironment`) are reused from
`types/tags.h` unchanged. `#pragma pack(push,1)` is in force.

### `shader_animation_channel` — size `0x10`

| Off | Type | Field |
|---|---|---|
| `0x00` | `int16_t` | `source` (0 = none → 1.0, else `render_animation.function_values[source-1]`) |
| `0x02` | `int16_t` | `function` (periodic function type, passed in AX to `0x4cc9b0`) |
| `0x04` | `float` | `period` (0.0 treated as 1.0) |
| `0x08` | `float` | `phase` (added to the time) |
| `0x0c` | `float` | `scale` |

### `shader_texture_animation` — size `0x38`

Embedded at `ShaderModel +0xfc`, `ShaderTransparentChicagoMap +0xa4`,
`ShaderTransparentGenericMap +0x2c`, `shader_effect +0x60`.

| Off | Type | Field |
|---|---|---|
| `0x00` | `shader_animation_channel` | `u` |
| `0x10` | `shader_animation_channel` | `v` |
| `0x20` | `shader_animation_channel` | `rotation` (degrees) |
| `0x30` | `Point2D` | `rotation_center` |

### `shader_texture_transform` — size `0x20` (output of `0x53fe50`)

| Off | Type | Field |
|---|---|---|
| `0x00` | `float[4]` | `u_row` (EBX) = `{ c*u_scale, -s*v_scale, 0, c*du - s*dv + center.x }` |
| `0x10` | `float[4]` | `v_row` (EDI) = `{ s*u_scale, c*v_scale, 0, s*du + c*dv + center.y }` |

### `shader_effect` — size `0xb4` (shader_type 1, the particle shader block)

| Off | Type | Field |
|---|---|---|
| `0x00` | `uint8_t[0x24]` | tag padding |
| `0x24` | `int16_t` | `shader_type` (1) |
| `0x28` | `uint16_t` | `flags` |
| `0x2a` | `int16_t` | `framebuffer_blend_function` |
| `0x2c` | `int16_t` | `framebuffer_fade_mode` |
| `0x2e` | `uint16_t` | `map_flags` |
| `0x4c` | `TagDependency` | `secondary_map` (tag_id at `0x58`, -1 = none) |
| `0x5c` | `int16_t` | `anchor` (ParticleAnchor) |
| `0x5e` | `uint16_t` | `secondary_map_flags` |
| `0x60` | `shader_texture_animation` | `texture_animation` |
| `0x98` | `float` | runtime average particle radius (tag padding) |
| `0x9c` | `float` | `zsprite_radius_scale` |

### Numeric countdown timer globals

| Address | Type | Name |
|---|---|---|
| `0x00721e50` | `int32_t` | `numeric_countdown_timer_remaining_ms` |
| `0x00721e54` | `uint8_t` | `numeric_countdown_timer_running` |
| `0x00721e58` | `int32_t` | `numeric_countdown_timer_last_update_ms` (game ticks * 1000 / 30) |

### Tag offsets read (from `types/tags.h`)

| Struct | Offset | Use |
|---|---|---|
| `Shader` | `+0x00` bit 2 | `transparent_lit` → permutation 5 |
| `Shader` | `+0x24` | `shader_type` |
| `ShaderModel` | `+0x38` | `translucency` > 0 → permutation 1 |
| `ShaderTransparentGeneric/Chicago/ChicagoExtended` | `+0x29` | flags byte: bit 1 decal, bit 3 first map in screenspace, bit 4 draw before water |
| same | `+0x2a` | `first_map_type` |
| `ShaderTransparentGlass` | `+0x28` bit 1 | decal |
| `ShaderTransparentMeter` | `+0x28` bit 0 | decal |
| `ShaderEnvironment` | `+0x150..+0x164` | u/v animation function, period, scale |

## Functions

| Address | Name (Ghidra name) | Registers | Rewrite conf. |
|---|---|---|---|
| `0x53fd60` | `chimera__shader_get_vertex_shader_permutation` | ECX shader (-1 = none) → AX; preserves EDX | 0.85 |
| `0x53fde0` | `shader_is_decal` (`FUN_0053fde0`) | ECX shader (NULL ok) → AL | 0.85 |
| `0x53fe30` | `shader_draw_before_water` (`FUN_0053fe30`) | ECX shader (NULL ok) → AL | 0.9 |
| `0x53fe50` | `shader_texture_animation_evaluate` | ECX render_animation* (may be NULL), ESI animation, EBX/EDI output rows, 6 stack floats, caller pops | 0.85 |
| `0x540060` | `shader_environment_texture_scrolling_evaluate` (`FUN_00540060`) | ESI ShaderEnvironment*, stack u_out, v_out, double time | 0.9 |
| `0x5400c0` | `numeric_countdown_timer_get_digit` (`game_timer_get_digit_pair`) | AX digit index (-1..8) → AX | 0.9 |
| `0x540240` | `numeric_countdown_timer_update` (`game_timer_update`) | none | 0.9 |

Renames are recorded in `symbols/agent_phase4_shaders.txt`.

## Review fixes (phase 4)

- `0x53fe50`: periodic function arguments now computed in double (the x87 code never rounds
  `time + phase` to float before the divide); `fcos` takes the unrounded `rotation * pi/180`,
  `fsin` the float-rounded one, and sine/cosine stay unrounded through the row products, as in
  the original. Parameter order changed to match the rasterizer externs.
- `0x540060`: parameter order changed to match the rasterizer externs; comment corrected
  (the divide is `fdivr qword`, i.e. a double divide; `push ecx / pop ecx` preserves ECX).
- `0x5400c0`: comments corrected (three callers, not one; upper half of EAX is leftover
  quotient, not sign extension).
- `types/shaders.h`: function list updated to the final names.

## Known gaps

- Foreign modules still declare these functions under their old names: `FUN_0053fde0`,
  `FUN_0053fe30`, `FUN_00540060` (src/rasterizer), `game_timer_get_digit_pair`
  (src/rasterizer chicago draws), `game_timer_update` (src/game/game_effects_update.c). Their
  prototypes agree with the definitions here; only the names are stale.
- The rasterizer externs of `0x53fe50` call the stack floats 3..5 `unused_z / unused_w /
  unused_5`; they are `u_offset`, `v_offset`, `rotation_degrees`.
- Why the vertex permutation helper tests its shader pointer against -1 rather than NULL is not
  explained by this module (callers pass the shader pointer loaded from `[edx+0xc]`).
- `0x53fe50` does not bounds-check `source` against the `function_values` array; neither does
  the original.
