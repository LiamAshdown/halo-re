# shaders module: 7 functions (address, current Ghidra name, size, agent confidence, summary)

- 0x53fd60 chimera__shader_get_vertex_shader_permutation size=91 conf=0.7 :: Selects the vertex shader permutation index for a shader tag instance (in_ECX) based on its shader type and per-type flags/fields.
- 0x53fde0 FUN_0053fde0 size=47 conf=0.3 :: Returns a per-shader-type boolean flag bit from a shader tag instance, likely a rendering property such as two-sidedness.
- 0x53fe30 FUN_0053fe30 size=30 conf=0.3 :: Returns another per-shader-type boolean flag bit (distinct from FUN_0053fde0) from a shader tag instance, likely a rendering property such as alpha testing.
- 0x53fe50 shader_texture_animation_evaluate size=517 conf=0.75 :: Evaluates a texture UV animation (scroll u/v and rotation) at the given time, producing two transform rows (in unaff_EBX/unaff_EDI) used to animate texture coordinates.
- 0x540060 FUN_00540060 size=83 conf=0.35 :: Evaluates two independent periodic animation channels (e.g. animated shader properties) at the given time and writes their scaled results to the two output floats.
- 0x5400c0 game_timer_get_digit_pair size=329 conf=0.5 :: Extracts a pair of display digits (tens/ones) for a given position of a millisecond-based game timer, for rendering a clock/stopwatch HUD.
- 0x540240 game_timer_update size=86 conf=0.5 :: Advances/counts down the global millisecond game timer (DAT_00721e50) based on elapsed ticks since the last update, when the timer is running.
