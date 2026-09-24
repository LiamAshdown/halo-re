# camera module: type recovery notes

Header: `types/camera.h`. Smoke test: `out/phase4/camera_smoke.c` (includes only `tags.h`,
`memory.h`, `camera.h`). Both of these pass with `-Wall`:

```
C:\msys64\ucrt64\bin\gcc.exe -fsyntax-only -Wall -I types out/phase4/camera_smoke.c
C:\msys64\ucrt64\bin\gcc.exe -m32 -fsyntax-only -Wall -I types out/phase4/camera_smoke.c
```

The smoke file checks the size of all 20 structs and about 75 field offsets. Most offsets are
checked as `global address - struct base`, so they compare against the absolute addresses the
code uses and not against my own arithmetic. `director` holds a function pointer and
`observer` holds a command pointer. ucrt64 gcc uses 64-bit pointers, so the checks on those two
structs are gated on `PTRS32`, the same gate `sound_smoke.c` uses, and only run under `-m32`.
No identifier in camera.h collides with another header under `types/`. I checked the enum
constants, typedef names and function pointer typedefs.

Evidence came from `objdump -d -M intel` of `bin/halo.exe` (whole .text, scratch copy),
`objdump -s` of .data, `tools/pack.py`, and the phase 2 packs.

## The module is three Blam files

| range | layer | globals |
|---|---|---|
| 0x444b30..0x4450e0 | camera script (hs cinematic camera) | .data 0x006869d0..0x00686a0f |
| 0x4450e0..0x447370 | director (per player mode selection, pov procedures, debug look input, flying/editor cameras) | .data 0x00686a10..0x00686ad0, .bss 0x006ac558..0x006ac657, 0x006f17f8..0x006f186c |
| 0x447680..0x449170 | observer (final camera, quintic spline easing, collision) | .data 0x00686ae0..0x00686af7, .bss 0x006ac658..0x006ac8f7 |

The .bss block is contiguous. `0x006ac54c` is the last cache global (tag_data_base) and
`0x006ac550` / `0x006ac554` are cache too (texture / sound cache). Camera starts at 0x006ac558.
`observers[1]` ends at 0x006ac8f8, which is the first address of the next module (performance
frequency, math). The .data block ends at 0x00686af8, the first address another module uses
(90+ references).

## Binary-carried layouts (preferred over the decompiler)

* **observer = 0x29c.** `observer_new` (0x447740, EDX = this) writes 0x72616421 at in_EDX[0]
  and in_EDX[0xa6] = +0x298, which gives a header and a trailer signature. Every indexed access
  is `imul reg,reg,0x29c`. Its callers pass EDX = 0x006ac65c (the thunk 0x447870, and 0x45b1a2
  right after `camera_initialize`) or 0x006ac65c + i*0x29c (0x478282). So the observer base is
  **0x006ac65c**, not 0x006ac6d0. 0x006ac6d0 is `observer + 0x74` (observer_camera), which is
  what `camera_get_globals_for_player` (0x4479a0) returns. 0x448900 addresses the whole struct
  from `ebx = 0x006ac65c + i*0x29c` with +0x08, +0x74..+0xac, +0xb0..+0xe4 and +0xe8..+0xf0,
  which confirms the base.
* **Parameter vector widths.** These come from .data tables. 0x00686ae0 = {3,3,1,1,6} and
  0x00686aec = {3,3,1,1,3} (dumped bytes `03000300 01000100 06000000 03000300 01000100 0300`).
  0x448210 copies `[esi+0x686ae0]` floats per channel from the command into the state, and
  every other spline loop uses `[..+0x686aec]`. So the state is 14 floats and each
  derivative/coefficient vector is 11 floats.
* **observer_command = 0x68.** It is moved as 0x1a dwords in camera_update (rep stos / rep movs
  into 0x006ac5b8), in observer_set_command 0x447ab0 (into observer+0x08), and in observer_new
  (zero loop over in_EDX[2..0x1b]).
* **camera_input_axis_definition rows** are dumped from .data 0x00686a28 (see header). The
  stride of 0x1c comes from `add esi,0x1c` in 0x446170 and from the 7-dword step in
  camera_initialize.
* **Name tables.** 0x00686a10 holds 5 string pointers: following, orbiting, flying, editor, first
  person. 0x00686ac0 holds flying camera / orbiting camera, and 0x00686ac8 holds exiting /
  entering.
* **director_camera_switching.** The hs global table row at 0x0068b1fc points at 0x00686a98, so
  this is the only hs-exposed camera global.

## Structs, and which functions established which fields

### camera_script_globals (0x40, .data 0x006869d0)
* The 0x00 byte is camera_control: 0x445cc0 writes it (and mirrors it through `*0x0087bc0c`).
  0x4c8930 clears it and 0x50ea50 reads it.
* 0x01 changed: set by 0x444c00, 0x444b30, 0x47ef40 and 0x47ef90, and cleared by 0x444d50.
* The mode at 0x02 is written as a WORD by all four setters (0 / 1 / 2 / 3). The .data value is
  0xffff.
* 0x04 point index: WORD, from 0x444c00 (`di`) and 0x444b30 (-1).
* 0x08 time: `ticks / 0x1e`, decayed in 0x444d50 by `dt * time_scale`. hs 0x47eff0 reads it.
* 0x0c / 0x18 / 0x24 / 0x30: 0x444c00 copies CutsceneCameraPoint +0x28 into position, the
  matrix4x3_from_euler_angles forward and up rows into forward and up, and fov into +0x40 (70
  degrees when it is 0). The .data defaults are forward (0,0,1), up (0,1,0) and fov 70.
* 0x34 object is written by 0x444c00 (arg), 0x47ef40 and 0x47ef90, and read by 0x444d50 and
  0x50ea50. 0x38 / 0x3c (DWORD / WORD) are written by 0x444b30 and read by 0x444d50 case 1
  (`imul edi,edi,0xb4` into the animation block, frame_count +0x22).
* Unresolved: 0x06 and 0x3e are never referenced.

### camera_input_axis_definition (0x1c), camera_input_axis_state (0x0c)
* Definition fields come from the 0x446170 loop, where esi = row + 4:
  `[esi-4]` / `[esi-2]` / `[esi]` are key bits, and `[esi+4]` / `[esi+8]` / `[esi+0xc]` /
  `[esi+0x10]` are acceleration, reset, min and max. The byte at +0x18 is read **only as the
  absolute 0x00686a40** (row 0) on every iteration. Either the compiler hoisted a
  loop-invariant expression that was written against row 0, or the source has that bug.
  Either way, the field exists per row and the .data values are 1, 0, 1, 1.
* State fields come from the same loop (`[edx]`, `[edx+4]`, `[edx+8]`, `add edx,0xc`) and from
  camera_initialize (reset / 0 / 0). 0x445f90 reads the deltas at director +0xd0 / +0xdc /
  +0xe8 / +0xf4. The absolute addresses are 0x006ac630 / 0x006ac63c / 0x006ac648 / 0x006ac654,
  and they map rows 0 to 3 onto camera_input +0x1c / +0x10 / +0x14 / +0x18.
* Unresolved: +0x06 (0 in every row) and +0x19..+0x1b.

### camera_input (0x24)
* 0x445f90 (ESI) zeroes 0x12 int16 and writes +0x00, +0x02, +0x04, +0x08, +0x0c, +0x10..+0x20.
* The field names come from the consumers. 0x4465d0 and 0x446e90 use +0x08 yaw, +0x0c pitch
  and +0x10 roll, and use +0x14 / +0x18 / +0x1c as the forward / left / up move, rotated by yaw.
  0x447370 and 0x446870 use +0x20 as zoom. The frame buffer in camera_update is `local_24[36]`,
  which is exactly 0x24 bytes.
* Unresolved: +0x03.

### observer_parameters (0x38), observer_parameter_derivatives (0x2c), observer_command (0x68)
* The command field order comes from every pov writer (param_3 indices):
  * [0] flags
  * [1..3] position
  * [4..6] focus offset (global zero vector in most writers, computed in the scripted
    relative-point case)
  * [7] distance (0x445380 dead camera distance; 0x447370 sets `|v| * scale`, min 0.6)
  * [8] fov
  * [9..0xb] forward
  * [0xc..0xe] up (always the output of 0x4479c0)
  * [0xf..0x11] velocity
  * [0x12] timer
  * bytes 0x4c..0x50 are the five interpolation bytes, and [0x15..0x19] the five channel times
* The channel-to-parameter mapping is confirmed three ways:
  * 0x447370 sets byte 0x4d/[0x16] when focus offset changes, 0x4e/[0x17] for zoom (distance)
    and 0x50/[0x19] for look (orientation).
  * 0x446d60 sets 0x4f/[0x18] for fov.
  * camera_update snaps channels 0 and 2 (position and distance) on a short first person
    transition.
* How 0x448900 consumes the parameters: final = position + rotate(focus_offset, horizontal
  forward) - distance * forward. It clamps distance to 0..FLT_MAX and then to 5000, and fov to
  0.001..pi/2.
* Flag bits: 0x1 is tested by 447ab0 and camera_update, 0x8 by 448010 and 447ab0, 0x10 by
  448900 and 0x20 by 447b50. Interpolation bits 0x1 and 0x2 are tested by 447ab0 and 448010.
* Unresolved: +0x51..+0x53 are never referenced. Command flag bits 0x02 and 0x04 are never
  set or tested.

### observer_camera (0x3c)
* Writers:
  * 0x448900: position, velocity (`-observer.velocity.position`), forward, up and fov from the
    state. It also writes the location words through `[ebx+0x80]` / `[ebx+0x84]`.
  * 0x447a60: leaf as a DWORD and cluster as a WORD (`mov ds:0x6ac6e0,ax`).
  * observer_new: the defaults.
* Readers: camera_debug_save_to_file writes position, forward, up and fov to camera.txt. That
  is +0x00, +0x20, +0x2c and +0x38, the same offsets types/sound.h found independently.
* Unresolved: +0x12. 0x448900 stores the whole dword +0x10..+0x13 from a register whose high
  half is stale stack, so the field is padding in effect.

### observer (0x29c)
* +0x04 command pointer: camera_update stores 0x006ac5b8 at 0x006ac660.
* +0x08 current_command: 447ab0.
* +0x5c are the command copy's channel times. They are the running countdowns, decremented by
  observer_dt in 447b50 and zeroed by 447e40 when a channel blows its acceleration limit
  (.rdata 0x006572c4).
* +0x70 updated: set by 0x444d12 and 0x4478ae, cleared by camera_update.
* +0x71 has_command: camera_update tests it, forces the snap bit once, then sets it.
  observer_new clears it.
* +0x74 is observer_camera.
* +0xb0 parameters: 448210 writes it and 448900 reads it. 448710 reads it (ECX) against the
  command (EAX = +0x0c) into +0x260 (EDX).
* +0xe8 velocity is written by 448010. +0x120 acceleration is written by 447e40.
* +0x158..+0x234 are the six coefficients, written by 447be0. The formulae make the powers
  clear: 448210 evaluates `t*c1 + t^2*c2 + t^3*c3 + t^4*c4 + t^5*c5 + c0`, where the addresses
  of c1..c5 and c0 are 0x864, 0x838, 0x80c, 0x7e0, 0x7b4 and 0x890.
* +0x260 remaining_offset: 448710.
* +0x298 trailer: observer_new.
* Unresolved: +0x72 (int16), +0x114, +0x14c and +0x28c. The last three are the three unused
  floats of each 14-float slot that holds an 11-float derivative. They are never referenced.

### director (0xf8, .bss 0x006ac560)
* +0x00: its only store is `mov WORD PTR ds:0x6ac560,0x2` in camera_debug_load_from_file.
  Nothing reads it. It is **unresolved**, left as `unknown_00`.
* +0x04 transition_time is read or written by camera_update, 0x445ac0, 0x445c00, 0x445dc0 and
  camera_shake_tick.
* +0x08 pov_proc: all mode switches, camera_update, 0x4455f0 and 0x445ac0.
* +0x0c union: see below.
* +0x4c and +0x50 are zeroed by camera_initialize (DWORD and BYTE) and never read. Unresolved.
* +0x51 / +0x52 are written by 0x445f90 and 0x446870, and read by game_engine 0x471ae0 and
  timedemo 0x4c6f30. game.h already calls these byte +1 and byte +2 of 0x006ac5b0.
* +0x54 seat_camera_state: 0x445c00 and 0x445dc0 (WORD). +0x56 camera_type: 0x445ac0.
* +0x58 command: camera_update.
* +0xc0 is zeroed on every pov switch (445c00, 445dc0, 445f40, 445cc0, 445940, camera_update,
  45b43e, 4c8a0f) and never read. It is unresolved, named `unknown_c0`.
* +0xc4 look_scale: same writers, and 0x446170 reads it.
* +0xc8 axes.
* Unresolved: +0x02, +0x53 and +0xc1..+0xc3 are never referenced.

### director_camera_data union (0x40, director +0x0c)
* **first_person** (+0 float): 0x446d60 (param_1[0]), 445c00 and 445dc0 (zero).
* **third_person** (0x1c): initialised by 445c00, 445dc0 and 445cc0 with BYTE stores at
  +0..+3, a WORD at +4, a DWORD -1 at +8, a WORD -1 at +0xc, 0 at +0x10 and +0x14, and 1.0 at
  +0x18. 0x447370 uses +0 (initialized), +2, +8 and +0xc against the camera_basis_out unit and
  seat, and +0x10 / +0x14 / +0x18. Unresolved: +1, +3, +4, +6 and +0xe.
* **dead** (0x30): 0x4450e0 writes every field (EAX = this). 0x445380 reads and writes [0..0xb]
  (positions and angles, then [7] timer, [8] and [9] players, [10] unit, [0xb] retarget). The
  target cycling in 0x445240 / 0x4452c0 walks the players data_array comparing player +0x20
  (team) and +0x34 (unit).
* **editor** (0x1c): 0x446e90 reads and writes [0..6]. 0x446350 initialises it. 0x445940 fills
  it through 0x446e30 and then sets roll (+0x14 = 0x006ac580) and fov (+0x18 = 0x006ac584).
  0x4465d0 uses [0..5].
* **orbiting** (0x1c): 0x446870 uses +4, +0xc and +0x10. 0x446a10 seeds [0..4].
* The union ends at +0x40 (director +0x4c), because camera_initialize zeroes +0x4c separately.
  Bytes 0x30..0x3f of the union are never touched by any member. That is unresolved.

### director_globals (8, .bss 0x006ac558)
* dt: written by camera_update, read by 445f90 and 446170.
* mode: a WORD, read in the camera_update switch (jump table 0x0044586c, 5 entries) and zeroed
  by camera_initialize.
* mode_changed: a BYTE, passed to 0x445dc0 and cleared.
* No code writes a nonzero mode or mode_changed in this build.
* Unresolved: +0x07.

### flying_camera_home (0x14, .bss 0x006f1800)
0x446350 writes it from Scenario +0x358 (the first player starting location: position and
facing, with pitch 0) or zeroes it.

### unit_camera_properties (0x58)
FUN_00447110 returns `UnitSeat + 0x84` or `Unit + 0x1a8`, and FUN_00447190 reads +0x4c / +0x50.
The smoke file checks both anchors against types/tags.h offsets.

## Flying / editor camera globals (.bss 0x006f17f8..0x006f186c)
* 0x006f17f8 is written by camera_update and read by 0x446d60.
* 0x006f17fd / 0x006f17fe / 0x006f1828 are **never written by code**. There is no hs or console
  table entry for them either. They are debug knobs, which leaves `flying_camera_follow_script`
  as an UNSURE name.
* 0x006f1830 / 0x006f1850 are 7-dword `rep movs` copies made by 0x446a10 and 0x4469a0.
  0x006f186c is the byte flag 0x4469a0 sets.
* Unreferenced gaps: 0x006f17fc, 0x006f1818, 0x006f182a..0x006f182f and 0x006f184c.
* 0x00686aa4 = 0x007c3100, which is `render_frame_index` in types/render.h. The flying camera
  reads `[eax+0x14]` (position) and `[eax+0x20]` / `[eax+0x24]` / `[eax+0x28]` (forward) from
  it. So this pointer is the base of a render block whose +0x14 is the render_camera at
  0x007c3114. It stays `void *`.
* 0x00686ab0 is called as `[edx*8+0x686ab4]` with edx = flying_camera_mode != 0, so only
  0x686abc (0x446a10) is reachable. 0x686ab8 (0x4469a0) is never an indexed target in this
  build. No instruction references 0x4469a0 directly either, so it is reachable only through a table slot this build never indexes. The 2x2 shape is a best reading.
* 0x00686ad0 float[4] {31.29, 12.78, 5.13, 2.05} is never referenced.

## Unresolved offsets (summary)
* director: +0x00 (write-only WORD), +0x02, +0x4c, +0x50, +0x53, +0xc0 (write-only), +0xc1..+0xc3
* director_camera_data: +0x30..+0x3f. third_person: +0x01, +0x03, +0x04, +0x06, +0x0e.
  orbiting: +0x00, +0x08, +0x14, +0x18.
* director_globals +0x07
* camera_input +0x03
* camera_input_axis_definition +0x06, +0x19..+0x1b
* camera_script_globals +0x06, +0x3e
* observer_command +0x51..+0x53
* observer_camera +0x12
* observer +0x72, +0x114, +0x14c, +0x28c
* Globals: 0x00686a24, 0x00686a99..0x00686a9b, 0x00686ad0[4], 0x006f17fc, 0x006f1818,
  0x006f182a..0x006f182f, 0x006f184c
* The key letters behind camera_input_key_bits are UNSURE. The engine key indices themselves
  (0x20, 0x2e, 0x2d, 0x2f, 0x22, 0x30, 0x23, 0x31, gated on 0x1d backspace) are certain.

## Misattributed / mis-split functions
* **0x445230 `physical_memory_initialize`** is not a function. It is `pop esi; ret` at the
  end of 0x4450e0 (344 bytes, which ends at 0x445238). It is the alternate epilogue of the
  `param != -1` path, and 0x445232..0x445239 is the other path. Its name comes from an
  unrelated library hint. No types.
* **0x445350 `physical_memory_map_predict_resources`** is not a function. It is the loop tail
  of FUN_004452c0 (0x4452c0 + 144 = 0x445350): the player scan that picks the next teammate
  for the dead camera. No types.
* **0x4462a0 `director_load_camera`** is not a function. It is the second half of the
  FUN_00446170 axis smoothing loop, entered through the loop back-edge `jne 0x4461f0`. No
  director or loading logic.
* Ghidra names that describe a different function, for the rename pass. The names I propose
  are my suggestions and are not confirmed by any symbol:
  * 0x444c00 `camera_debug_start`: hs camera_set (cutscene point index in AX, ticks, relative
    object)
  * 0x444d50 `camera_debug_compute_pov`: the scripted camera pov procedure
  * 0x4450e0 `camera_shake_initialize`: dead_camera_new
  * 0x445380 `camera_track_compute_pov`: the dead camera pov
  * 0x445cc0: camera_control
  * 0x445f40: switch to the flying camera
  * 0x446350: flying camera initialize (not a device camera)
  * 0x446470: flying camera attach to object
  * 0x4465d0 `camera_debug_update_transform`: the flying sub-mode update
  * 0x446870: the orbiting sub-mode update (not a spectator camera)
  * 0x447740: observer_new
  * 0x447880 `camera_shake_tick`: observer_update. Nothing in it is shake. It sets the command,
    advances the spline, commits, and adds a first person bob via 0x55cca0.
  * 0x4479a0 `camera_get_globals_for_player`: observer_get_camera
  * 0x447ab0: observer_set_command
  * 0x447b50: observer advance
  * 0x447be0 / 0x447e40 / 0x448010 / 0x448210: spline coefficients, acceleration, velocity,
    and value plus orthonormalise
  * 0x448710: remaining offset
  * 0x448900: observer commit
  * 0x448d40 / 0x449170: observer collision pushout / single ray
* Real functions in the range that Ghidra has **no function for**. They are listed so the
  function list can be fixed:
  * 0x4464f0: flying pov
  * 0x446e90: editor pov
  * 0x4469a0 / 0x446a10: flying transitions
  * 0x447870: observer initialize thunk (`mov edx,0x6ac65c; jmp 0x447740`)
  * 0x447a60: observer update location
  * 0x444b30 lies just below the range. It is the camera animation setter, which writes
    camera_script_globals.
* **Math helpers compiled into these files.** They own no types:
  * real_approximately_equal 0x447680, real_is_valid 0x4476c0 and vector3d_is_unit_length
    0x4476e0 are observer assertion helpers.
  * scalar / vector3d catmull-rom 0x447000 / 0x447080 are used by the first person camera
    track offset 0x447190.
  * 0x4479c0 is the up vector from forward, and 0x446e30 is position + forward to yaw / pitch.
* Types the module uses but other modules own, which are therefore not redefined:
  * player (game)
  * player control (game, stride 0x40)
  * mouse_state (input)
  * render_camera (rasterizer)
  * Scenario / Unit / UnitSeat / CameraTrack tags (tags.h)

## Consistency notes for other headers (not edited)
* types/sound.h `sound_observer_camera` (0x29c from 0x006ac6d0) is observer_camera plus the rest
  of one stride. It crosses into the following observer, which is harmless with one local
  player. Retyping it as `observer_camera` would be exact.
* types/game.h, "0x006ac5b0 uint8_t[] (camera) stride 0xf8": this is director +0x50. Bytes +1
  and +2 are director.suppress_look_update and director.look_input_consumed.
