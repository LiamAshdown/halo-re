# cutscene module: type recovery notes

Header: `types/cutscene.h`. Smoke test: `out/phase4/cutscene_smoke.c`. It passes both
`gcc -fsyntax-only -I types` and `gcc -m32 -fsyntax-only -I types`. The pointer-holding structs
only check their sizes under `-m32`.
Sources: `out/phase2/cutscene/00.md`, `out/phase2/results/cutscene_00.json`, `python tools/pack.py`,
a full `objdump -d -M intel` of `bin/halo.exe`, and direct reads of `.data`/`.rdata` (the codec
tables and the `bysw` records). The hs function definitions were found by searching `.data`
for each evaluator pointer. The name string sits 8 bytes before it.

The run holds three unrelated things: a sort routine, the hs `cinematic_*` layer, and the
recorded animation (hs `cutscene_recording`) codec.

## cinematic_globals (0x1c) and cinematic_title_slot (0x04)

- **Size** comes from the allocation in 0x45a9c0 (Ghidra calls it `particle_systems_initialize`,
  but it is the new-map initializer). It does `game state cursor += 0x1c` and a crc32 over 0x1c,
  then stores the base at 0x006f187c. The reset inside 0x45b050 (at 0x45b2f2) zeroes dwords 0..6
  and then writes -1 to dwords 3..6.
- **+0x00 letterbox_scale**: `chimera__letterbox` 0x4499c0 reads it as a float. It steps it by
  `(now - last) * 1/30`, clamps it to 0..1, and draws the two bars with 0x449780 while it is
  above 0. Each bar is `scale * 0.125 * 480` tall.
- **+0x04 letterbox_last_tick**: int32. 0x4499c0 does `sub esi,edi` / `fild` on it and on
  `game_time`. `cutscene_start` 0x449720 and `cinematic_show_letterbox` 0x47f8b0 seed it from
  `game_time_globals +0x0c`.
- **+0x08 show_letterbox**: set by 0x449720 and by 0x47f8b0 (from its bool argument). Cleared by
  0x449eb0 and by the map dispose 0x45b370.
- **+0x09 in_progress**: set by 0x449720, cleared by 0x449eb0 and 0x45b370. About twenty
  readers across the engine (0x472760, 0x4c9238, 0x4c99ef, 0x50bcbe, 0x4f4860, ...).
- **+0x0a skip_in_progress**: `cinematic_skip_start_internal` 0x47f840 writes 1 and
  `cinematic_skip_stop_internal` 0x47f860 writes 0. Two paths read it before reverting to the
  skip checkpoint: the skip-key path 0x472779 and `game_state_save_core` 0x4c78ce.
- **+0x0b suppress_bsp_object_creation**: written only by
  `cinematic_suppress_bsp_object_creation` 0x47f9b0. Read by 0x4f4860, which skips 0x4f4880
  while `in_progress && suppress`.
- **+0x0c titles[4]**: `cutscene_title_queue` 0x449960 scans `+0xc + i*4` for -1, then writes
  the index and `-(short)round(delay * 30.0)` at +0xe. Its call sites are cdecl:
  `cinematic_set_title` 0x47f910 pushes (index, 0.0) and `cinematic_set_title_delayed` pushes
  (index, delay). 0x4499c0 walks `edi = 0x0c .. 0x1c` in steps of 4. It adds `ticks_this_frame`
  (0 while `game_time.paused`) and writes -1/-1 once `ticks > up_time + fade_out_time`.
- **Unresolved:** none. Every byte of the 0x1c is accounted for.
- **Semantics caveat:** the title fade compares `ticks` directly against the ScenarioCutsceneTitle
  floats. The in-phase test is `fade_in_time <= t <= up_time`, so `up_time` works as an absolute
  end time and not a duration. As a result the tag values behave as ticks. This header does not
  settle whether tag postprocess converts seconds to ticks.

## recorded_animation_angles / _decoder_state / char and short differences

- 0x44a110 and 0x44a150 wrap only the yaw. Register arguments come from the call sites
  0x44a2a8/0x44a365 (`lea eax,[esi+4]` / `mov eax,esi`) and from the helper bodies: EAX is the
  pair, EDX the delta. 0x44a190 takes ECX = pair and EAX = float[3], with the scale
  `0x00672dd8 = pi/1000`.
- The decoder state is 3 angle pairs. The compressed handlers 0x44a1d0/0x44a390 index them as
  `param_1`, `param_1+2`, `param_1+4` (shorts). The compressed begin 0x44a550 copies 0x0c bytes
  from the stream into arg1 and advances the cursor by 0x0c.
- The payload sizes are the `bysw` records `vector_char_difference_data` (2) and
  `vector_short_difference_data` (4). The cursor advances by 2 in 0x44a1d0 and by 4 in 0x44a390.

## v1 (uncompressed) events

- Layouts come from the `bysw` records 0x686f0c..0x686fc4: `animation_event_v1` 4 (-2,-2), the
  set events 6/6/6/6, throttle 0x0c, multi_vector 0x10, angle_vector 0x0c. The handler bodies
  agree: 0x44a650 byte, 0x44a670 byte, 0x44a690 word, 0x44a6b0 word, 0x44a6d0 two floats plus
  k=0, 0x44a700/730/760 a single vector, 0x44a820 multi, 0x44a790 angles. Each one's cursor
  advance equals the record size.
- The dispatch is 0x44a8b0: `event.type == 1` means end, the handler comes from
  `0x686ea8[type]`, and a NULL handler means the cursor moves past the 4-byte header. The loop
  runs while `*event_ticks >= delay`.
- **Unresolved:** v1 types 7 and 8 have no handler. The angle types 0x10, 0x11, 0x12 and 0x16
  write all three vectors, and 0x13/0x14/0x15 each skip one. The binary does exactly this, but
  the intent (a mask?) is unknown.

## compressed events (version 4)

- There is no fixed record. The event byte holds the type in bits 2..7 and the delay encoding in
  bits 0..1 (jump table at 0x44a63c: 0 ticks/1 byte, 1 tick/1 byte, u8/2 bytes, u16/3 bytes).
  End is `(byte & 0xfc) == 4`. Handlers live in `0x686d98[type]` and take
  (state, control, header, cursor), per the push order at 0x44a601..0x44a60d.
- Handlers: 2..6 are 0x44a060/080/0a0/0c0/0e0, 7..14 are 0x44a1d0 (mask = type-7) and
  15..22 are 0x44a390 (mask = type-15).

## unit_control_data_field_layout (0x0c) and the version tables

- Dumped from 0x686cc8/0x686d40/0x686d58/0x686d70 through the pointer array 0x686d88. 0x449fd0
  reads `+4` (size) and `+8` (offset), steps by 0x0c and stops on size -1. The `+0` pointer
  always names a `bysw` record (byte/word/long/real_vector2d/real_vector3d) and is NULL in the
  terminator. 0x449fd0 never reads it.
- Register convention: EBX is the unit_control_data*. The stack holds (cursor **, version byte),
  seen at call sites 0x44a55e/0x44a89d.
- **Unresolved:** the v1 table drops 2 bytes (`size 2, offset -1`) between weapon_index and
  throttle. It is probably the recorded grenade_index or `unit_control_data.unknown_0a`, but
  nothing pins which.

## recorded_animation (0x64) and recorded_animation_codec (0x08)

- The size comes from `index * 100` in 0x44aa90. The codec argument addresses come from 0x44a930
  (begin: +0x54, +0x14, +0x10, version byte) and 0x44aa90 (update: +0x54, +0x14, +0x0c, +0x10).
  Also used: +0x04 unit handle, +0x08 decremented per tick, +0x0a flags, +0x60 = version-1.
- The codec table 0x686fe8 is {0x686fe0, 0x686fe0, 0x686fe0, 0x686fd8}, with 0x686fd8 =
  {0x44a550, 0x44a590} and 0x686fe0 = {0x44a890, 0x44a8b0}.
- **Correction to out/phase4/devices_types_notes.md:** +0x0c is not a frame index. It counts
  pending ticks: the update routines subtract each fired event's delay from it, and 0x44aa90
  adds 1 after each update.
- **Unresolved:** +0x02 and +0x62 are never touched (padding). Flag bit 1 is never tested. The
  meaning of the object bits behind flags 0x4/0x10 (object +0x204 bit 6, object +0x4cc bit 1)
  belongs to the objects and units headers.

## Misattributed / library functions

- **0x449590 `qsort_dword_array`, 0x4496d0 shortsort**: a library-style sort with no cutscene
  state. Callers are 0x412ba0 (ai, comparator 0x4127b0) and 0x552cf0 (comparator 0x552c00). It
  takes EAX = count and ECX = base; the comparator is on the stack and returns nonzero when
  a > b, so the sort is ascending. The shortsort takes (lo, cmp) on the stack with EAX = hi.
  It belongs to cseries, so I skipped its types.
- **0x449780**: a generic filled screen rectangle (ECX Rectangle2D*, EAX packed ARGB). It builds
  ui_quad_render_state + 4 hud_quad_vertex for 0x51c9a0, with maps[0] = default_2d bitmap data
  element 1. It has 7 callers: letterbox x2, 0x465690, 0x494ca0, `interface_draw_cursor`,
  0x4984c0, the trouble-brewing indicator and 0x517b90. It sits in this object but is really
  an interface/rasterizer helper, and has no types of its own.
- **0x449f80**: `recorded_animation_find_by_name` (ESI Scenario*, EBX name, returns int16). Its
  only caller is `actor_squad_action_execute` 0x405520.
- **Functions Ghidra never created**, all reached only through tables: 0x44a060, 0x44a080,
  0x44a0a0, 0x44a0c0, 0x44a0e0, 0x44a550, 0x44a590, 0x44a650, 0x44a670, 0x44a690, 0x44a6b0,
  0x44a6d0, 0x44a700, 0x44a730, 0x44a760, 0x44a820, 0x44a890, 0x44a8b0. They should be created
  and assigned to this module before phase 5.
- **Suggested renames**:
  - 0x4499c0: `cinematic_render` (Chimera calls it letterbox)
  - 0x449fd0: `unit_control_data_unpack`
  - 0x44a110 / 0x44a150: `recorded_animation_apply_char_difference` / `_short_difference`
  - 0x44a1d0 / 0x44a390: compressed difference event handlers
  - 0x44a790: the v1 angle vector event handler (it is not a keyframe)

## Header ingest caveat

`scripts/ApplySymbols.java` parses `types/*.h` one file at a time, in alphabetical order.
`cutscene.h` uses types from `math.h`, `memory.h` and `units.h`. `camera.h` and `effects.h`
already depend on later-sorted headers in the same way, so on an empty data type manager
`cutscene.h` resolves only once those headers are loaded (a second run).
