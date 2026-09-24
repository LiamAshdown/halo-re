# shaders module: type recovery notes

Header: `types/shaders.h`. Syntax gate: `out/phase4/shaders_smoke.c`
(`C:\msys64\ucrt64\bin\gcc.exe -fsyntax-only -I types out/phase4/shaders_smoke.c` passes). Every
size and offset below is also a compile-time check there. None of the records holds a pointer,
so the checks fire on the 64-bit host compiler too. Including every types/*.h together (tags.h
and memory.h first) adds no errors from shaders.h.

All offsets come from objdump of 0x53fd60..0x5402a0, not just from Ghidra. The jump tables
(0x53fdbc, 0x53fe10, 0x54020c) were decoded from the raw bytes, and every call site was found
by scanning .text for E8/E9 rel32.

## Register conventions (from prologues and call sites)

| Function | Inputs | Output |
|---|---|---|
| 0x53fd60 | ECX shader (tested against **-1**, not NULL) | AX |
| 0x53fde0 / 0x53fe30 | ECX shader (tested against NULL) | AL |
| 0x53fe50 | ESI anim block, ECX render_animation* or NULL, EBX / EDI output rows; stack u_scale, v_scale, u_offset, v_offset, rotation, time (6 floats, caller cleans) | none |
| 0x540060 | ESI ShaderEnvironment*; stack float* u_out, float* v_out, double time | none |
| 0x5400c0 | AX digit index (-1..8) | AX (one digit, not a pair) |
| 0x540240 | none | none |

periodic_function_evaluate 0x4cc9b0 takes the wave function in AX and a double on the stack. It
returns the value in ST0.

## What the module owns

| Record | Size | Established by |
|---|---|---|
| `shader_animation_channel` | 0x10 | 0x53fe50 reads source +0x00 (WORD, 0 means 1.0, otherwise `function_values[source-1]` via ECX+4), function +0x02 (into AX for 0x4cc9b0), period +0x04 (0.0 becomes 1.0), phase +0x08 (added to time), scale +0x0c. It does this per channel at +0x00/+0x10/+0x20 |
| `shader_texture_animation` | 0x38 | 0x53fe50: three channels, plus rotation_center at +0x30/+0x34 (subtracted before the rotation, added back after). The call sites load ESI with ShaderModel+0xfc (0x528eb3), ShaderTransparentChicagoMap+0xa4 (0x532419) and shader_effect+0x60 (0x534335). All of these match tags.h one for one, and ShaderTransparentGenericMap+0x2c has the same shape |
| `shader_texture_transform` | 0x20 | 0x53fe50 writes EBX[0..3] and EDI[0..3], with [2] = 0.0 in both. Every caller passes EDI = EBX + 0x10 inside one block (esp+0x3c/+0x4c at 0x528ec3; esp+esi*0x20+0x70/+0x80 at 0x532414, a per-stage array of stride 0x20). So this is one 2x4 matrix |
| `shader_effect` | 0xb4 | The layout comes from tags.h (LightningShader is exactly this block; Particle +0xb0, Contrail +0x84, ParticleSystemType, WeatherParticleSystem embed it). This module confirms +0x24 (type 1), +0x58 (secondary_map.tag_id vs -1) and +0x5c (anchor) in 0x53fd60, and +0x60 texture_animation at the 0x534335 call site. types/render.h already used the name `shader_effect*` without a definition |
| enums `shaders_constants`, `shader_flag_bits`, `shader_transparent_flag_bits`, `shader_decal_flag_bits`, `shader_vertex_permutation`, `numeric_countdown_timer_digit` | | Bit tests at 0x53fdad (flags & 4), 0x53fda5 (+0x29 & 8), 0x53fdf9 / 0x53fe01 / 0x53fe09 / 0x53fe45; timer divisors in 0x5400c0; *1000/30 in 0x540240 |

Globals owned (the numeric countdown timer; the names come from the hs function strings
`numeric_countdown_timer_set/_get/_stop/_restart`, whose definitions at 0x00657d74..0x00657dd0 give
the signatures set(long, boolean), get(short) -> short, stop(), restart()):

- 0x00721e50 int32_t numeric_countdown_timer_remaining_ms. Read by 0x5400c0 and 0x540240. Written by 0x540240 (counts down, clamps at 0) and by the hs set evaluator 0x47ae41.
- 0x00721e54 uint8_t numeric_countdown_timer_running. Read by 0x540240. Written by the hs evaluators set 0x47ae49, stop 0x47aec4 (0) and restart 0x47aee4 (1).
- 0x00721e58 int32_t numeric_countdown_timer_last_update_ms. Only 0x540240 touches it (game_time_globals+0x0c * 1000 / 30). It is not updated while the timer is stopped.
- 0x00721e55..57: alignment, no references.

These three are declared as separate globals rather than as a struct. Nothing addresses them
through a base pointer, and the only multi-field copy (0x47ae3c..0x47ae49) reads from the hs
argument block {long @+0, boolean @+4}, not into a struct.

## Tag offsets confirmed against types/tags.h (no change needed)

- Shader: +0x00 shader_flags bit 2 transparent_lit (0x53fdad), +0x24 shader_type (all three
  shader accessors).
- ShaderModel +0x38 translucency (`fcomp` against 0.0; permutation 1 when > 0).
- ShaderTransparentGeneric / Chicago / ChicagoExtended: +0x29 flags byte (bit 1 decal, bit 3
  first_map_is_in_screenspace, bit 4 draw_before_water), +0x2a first_map_type.
- ShaderTransparentChicagoMap: +0x5c/+0x60/+0x64 map_u_offset / map_v_offset / map_rotation
  are pushed as the 0x53fe50 offset and rotation arguments at 0x5323ec.
- ShaderTransparentGlass +0x28 bit 1 decal; ShaderTransparentMeter +0x28 bit 0 decal.
- ShaderEnvironment +0x150..+0x167: the u/v texture scrolling pairs (function, period, scale)
  read by 0x540060. The same callers read +0xc8 secondary_detail_map_scale and +0xf8
  micro_detail_map_scale (0x51e483 / 0x51e49f), which confirms that ESI is a ShaderEnvironment.

## Unresolved offsets

- shader_effect +0x00..+0x23, +0x26, +0x30..+0x4b and +0xa0..+0xb3 are tag padding, and nothing in
  this module reads them (named unknown_XX).
- shader_effect +0x98 is tag padding. types/render.h says render_particles stores an average radius there
  at runtime. It is not read in this module, so it stays unknown_98.
- The 0x53fd60 test `ecx == -1` (rather than NULL) is unexplained. The three callers (0x51eeda,
  0x531eeb, 0x532a5f) pass a tag data pointer, so -1 is presumably a "no shader" sentinel from
  the caller's tag_get path.

## Cross-module observations

- types/rasterizer.h `transparent_geometry_group.lighting_extra` (+0x74) and
  `rasterizer_model_draw_context.unknown_84[2]` are the `render_animation` block of
  types/render.h ({change_colors, function_values}). 0x53fe50 receives group+0x74 in ECX
  (0x53242a, 0x534347) and model context+0x84 (0x528eaa) and reads `function_values` at +4.

## Misattributed / misnamed functions

None belong to another module. The module-order check holds: scenario ends at 0x53f150, and
shell starts at 0x5402a0. Name corrections for Phase 4 rewriting:

- 0x53fde0 FUN_0053fde0 -> `shader_is_decal`. The bits it returns are the decal flag of every
  type it handles (transparent 5..7 bit 1, glass bit 1, meter bit 0). It is not two-sidedness.
- 0x53fe30 FUN_0053fe30 -> `shader_draw_before_water`. It returns bit 4 of the transparent flags,
  which is draw_before_water, not alpha testing.
- 0x540060 FUN_00540060 -> `shader_environment_texture_scrolling_evaluate` (ShaderEnvironment
  u/v scrolling at +0x150).
- 0x5400c0 game_timer_get_digit_pair -> `numeric_countdown_timer_get_digit`. It returns a single
  digit in AX. Ghidra's CONCAT22 "pair" is the dead upper half of EAX.
- 0x540240 game_timer_update -> `numeric_countdown_timer_update`. It is tail-called (E9) from
  game_effects_update 0x45b57f.
- 0x53fd60 chimera__shader_get_vertex_shader_permutation: the name is correct.
- 0x53fe50 shader_texture_animation_evaluate: the name is correct.
