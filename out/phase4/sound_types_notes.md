# sound module: type recovery notes

Header: `types/sound.h`. Smoke test: `out/phase4/sound_smoke.c`, built with

```
cd C:\Users\Liam-\halo-re
C:\msys64\ucrt64\bin\gcc.exe -fsyntax-only -Wall -I types out/phase4/sound_smoke.c
C:\msys64\ucrt64\bin\gcc.exe -m32 -fsyntax-only -Wall -I types out/phase4/sound_smoke.c
```

Both pass, and so does `-std=gnu99 -m32`. The smoke file includes only `tags.h`, `memory.h`, `sound.h`.
ucrt64 gcc has 64-bit pointers, so every struct that holds a pointer is checked through the `_P`
macros, which only fire under `-m32` (the same PTRS32 gate `physics_smoke.c` and `networking_smoke.c` use).
It carries 23 size assertions, about 150 `offsetof` assertions (runtime records plus the tags.h
offsets the module reads), and span identities that pin the array bounds to the .data/.bss layout:
`0x00725430 + 81*0x678 == 0x00746028`, `0x007252e4 + 81*4 == 0x00725428`,
`0x00724a60 + 81*0x18 == 0x007251f8`, `0x00725218 + 0x44 == 0x0072525c`,
`0x0072525c + 0x48 == 0x007252a4`, `0x0069eae0 + 51*0x2c == 0x0069f3a4`,
`0x0069f4c8 + 0x40 == 0x0069f508`, `0x0069f514 + 0x14 == 0x0069f528`.
(`interface_smoke.c` fails independently of this module; it does not include sound.h.)

The header is self-contained on tags.h + memory.h. It does not pull in math.h or objects.h: the
listener matrix is spelled out as scale/forward/left/up/position (real_matrix4x3 in types/math.h)
and the leaf reference inside sound_location as two fields (bsp_leaf_reference in types/objects.h).
No memory.h or tags.h type is redefined.

## Evidence the binary carries (preferred over the decompiler)

- **sound_class_definition** (0x2c): read from .data 0x0069eae0 with a script over bin/halo.exe. 51 rows;
  the name table at 0x0069f3a8 follows 4 bytes after the last row. Values: max per tag 2..4,
  max per object 1..4, replace time 0..1000 ms, dialog byte 1 exactly for unit_dialog (19) and
  scripted dialog 44/46/47, priority 1..6, the 0x0c word 0/1, 0x10 float 0.5..1.0, default min/max
  distance pairs (e.g. projectile_impact 1.4 / 8.0, projectile_detonation 8 / 120), 0x20 float 0/1,
  0x24 float always 1.0, 0x28 muted byte 0.
- **sound_driver** (0x40): .data 0x0069f4c8, the only non-null entry of the table at 0x0069f508.
  Slots: 0x04 0x545e20 initialize, 0x08 0x546a60, 0x0c 0x547070, 0x10 0x546f90, 0x14 0x546b80,
  0x18 0x548380, 0x1c 0x5483d0, 0x20 0x548410, 0x24 0x548450, 0x28 0x546fe0, 0x2c 0x546fa0,
  0x30 0x548470, 0x34 0x5484d0, 0x38 0x5480f0, 0x3c 0x5482a0. The small wrappers were
  disassembled (capstone): each maps a logical channel through 0x007252e4 and tail-calls 0x547c80,
  0x5478c0, 0x547f60, 0x548050, 0x5472d0 or 0x5475b0. sound_initialize calls the table through a
  `short *`, which is why the decompiled offsets there read `+2`, `+0x1c`, `+0x1e` (bytes 4, 0x38, 0x3c).
- **sound_driver_parameters** (0x14): .data 0x0069f514 = `{0, {22,2,2,2}, {22,2,2,2}, 0}`;
  0x548200 rewrites both count arrays with the same defaults. The channel type flag table right after
  it (0x0069f528 = `{9, 8, 0xa, 0xe}`) is a separate global: 0x5494a0 reads counts through its
  parameter pointer but still reads the flags absolutely.
- **sound_effect_object_vtable** (0x24): .rdata 0x00671d04 (EAX2), 0x00671d28 (EAX3), 0x00671d4c (EAX1).
  Slots 3/4 are one-instruction getters of `this+0x10` / `this+0x14` (0x54ef10 / 0x54ef20); slot 2 is
  called per channel by 0x551480; slot 6 sets listener property 1 (EAX1) / 0xb (EAX2) / 2 (EAX3)
  from an int; slot 8 converts a float to millibels. Object sizes 0x1c and 0xe8 come from the
  operator_new calls in 0x551270 / 0x5514d0; 0xe8 = 0x1c + 51*4 matches the 0x33-entry loops in
  the EAX2/EAX3 initialize and shutdown.
- **sound_wave_format / sound_buffer_description**: the WAVEFORMATEX / DSBUFFERDESC blocks built on
  the stack in 0x545a30 and 0x546760 (dwSize 0x24, format tag 1, 16 bits, 0x5622/0xac44 Hz).
- Constant tables noted as globals: 0x0065e4f8 sample rates {22050, 44100} (indexed by the byte
  offset `flags & 4`), 0x0065e508 default SoundEnvironment, 0x0065e640 decode procs by SoundFormat,
  0x0069f510 fade exponent 2.5, 0x0069ff28 underwater direct gain 0.25, 0x006893d4 duck target 0.7.

## Per struct: which functions established which fields

### sound_location (0x40)
Copied as 16 dwords by sound_play_new 0x549af0 (into sound+0x14) and sound_looping_set_state
0x549fa0 (into looping_sound+0x0c). type 0x00 from 0x54bbd0/0x54bc50/0x54bb20 (0/1/2 cases) and
0x54c900 (1 transforms by the listener, 2 is already relative); scale 0x04 is the lerp factor between
Sound zero_/one_ modifiers in 0x549af0, 0x54c750, 0x54deb0; gain 0x08 set 1.0 by 0x543ce0/0x54b970 and
the detail gain by 0x54d270; position/forward/up 0x0c/0x18/0x24 from 0x54dc70 (default
global_forward 0x00696718 / global_up 0x00696720) and 0x54bbd0; leaf/cluster 0x30/0x34 written as
one dword by 0x5448c0 from FUN_004f6b10 and read as a short by 0x544aa0; obstruction/occlusion
0x38/0x3c written by 0x544aa0 and printed by render_debug_sound (sound+0x4c/+0x50).
Unresolved: 0x02 (never read), 0x36 (upper half of the leaf reference dword).

### sound_class_definition (0x2c)
0x00/0x02 0x54c1d0 (candidate list limits), 0x04 0x54c5e0, 0x08 0x54bcd0, 0x54c2f0, 0x54c900 (mouth
data lip sync through FUN_00570400), 0x0a 0x54c6b0, 0x0c 0x54c020, 0x10 0x54c750 / 0x54deb0
(eighth channel parameter), 0x18 0x54c900, 0x54c750, 0x54deb0, 0x54e740, 0x1c 0x545460, 0x54bd60,
0x54c900, 0x54d9f0, 0x28 the gain setters 0x545420 / 0x548680 / 0x5487b0 and readers 0x54af10, 0x54d9f0.
Unresolved: 0x09, 0x0e, 0x14, 0x20, 0x24 (never read in this module; values listed above).
Names dialog, discard_on_cache_miss and eax_value are behaviour names, not recovered originals.

### sound_class_gain (0x0c), pointer at 0x00746140
0x545330 walks 51 entries at stride 0xc (target 0x00, current 0x04, ticks 0x08), 0x545390 writes
target + ticks, 0x54b100 reads current. The allocation of the block is not in this module.

### game_looping_sound (0x34) and game_sound_globals (0x0c)
looping_sound_new 0x543c20 fills 0x02 (=2), 0x04 (=0), 0x0c, 0x10, 0x14 (=-1), 0x18 and, for object
sounds, 0x1a..0x30 from caller stack locals. Flags: bit0 0x544090/0x544250, bit1 0x544120 and the
background swap in 0x5445c0, bit2 0x544c70, bit3 0x544200, bit4 0x543b30/0x544090/0x544120.
scale 0x08 0x544180. last_update 0x14 compared with `update_count - 1` in 0x544290/0x544330.
function_index 0x18 indexes object 0x123 / 0x134, node_index 0x1a indexes the node matrices at
object nodes.offset (0x1f2) stride 0x34 (0x544330). game_sound_globals: [0] bumped at the end of the
full pass in 0x5445c0, [1] background sound index swapped against FUN_0053f150 output, [2] ms time
used for the 0x21 ms throttle.
Unresolved: none; every byte is written by looping_sound_new.

### sound (0xb0)
Constructors 0x549af0 and 0x54d9f0 write 0x02, 0x04, 0x06, 0x08, 0x0c, 0x10, 0x14..0x54, 0x54..,
0x84, 0x88, 0x8c, 0x8e, 0x90, 0x94, 0x98, 0xa4, 0xa8, 0xac. Fade block 0x92..0xa8 from 0x54af60 and
0x54e3c0. Flags bits from 0x54bcd0 (bit0), 0x54c020 (bit1), 0x54bd60 (bit2), 0x54ddc0/0x54deb0 (bit3).
listener_index 0x06 is multiplied by 0x44 into the listener array in 0x54c900 / 0x54c440.
location_proc 0x10 is compared with FUN_005448c0 in 0x54c900 and set to the 0x54dc10 thunk in 0x54d9f0.
play_state values 0..4 from 0x54d9f0 callers (1 start, 2 loop, 4 end), 0x549fa0 (3) and 0x54deb0 (1->2, 3->4).
Unresolved: 0x96 (never touched), 0xad..0xaf (padding), exact width of callback_data (0x30 is the
space up to 0x84; the largest observed copy is 0x0c).

### looping_sound (0xe4)
0x54d140 writes 0x04, 0x08, 0x4e, 0x50 and detail_next_time for each SoundLoopingDetail;
0x549fa0 writes 0x0c..0x4c, 0x4c, 0x52 (state) and 0x4d (alternate) and track_sounds (0xd4 + track*4);
0x549f50 refreshes 0x4c; 0x54d270 compares 0x4c against the frame toggle and reads 0x0c/0x10/0x54..;
0x54d9f0 bumps 0x50; 0x54deb0 writes 0x4e and 0xd4.. ; 0x54e5d0 searches 0x08.
Bounds: 32 details and 4 tracks exactly fill 0x54..0xe4.
Unresolved: 0x02 (never read or written), 0x4f (padding).

### sound_channel (0x18), 0x00724a60
sound_initialize/0x5494a0 seed 0x00 (-1), 0x04 (type flags), 0x10, 0x14. 0x54d020 accumulates 0x08 by
0x0c, 0x54deb0 reads 0x0c as the current pitch, 0x54cd30 / 0x54d020 / 0x54d0d0 move the permutation
pointers 0x10/0x14 and drop sound cache refcounts through permutation+0x2c. 0x54c900 reads
permutation+0x54/+0x60 (mouth_data) through 0x10.
Unresolved: 0x06 (the high half of the type word, never read).

### sound_listener (0x44), 0x00725218
0x54b970 writes 0x00, 0x01 and the matrix at 0x04 (FUN_004cb970 into 0x0072521c, then position
copied from the observer camera); 0x54bbd0/0x54bc50 read position 0x2c..0x34; 0x54c900 reads
velocity 0x38..0x40 and passes 0x01 as the underwater flag. The 0x01 edge plays matg sounds[0/1]
(enter/exit water) through 0x549af0.
Unresolved: 0x02 (padding). Nothing in the module writes velocity; it is presumably written by
FUN_004cb970 or stays zero (UNSURE).

### sound_listener_parameters (0x34), sound_channel_spatial (0x24), sound_channel_parameters (0x20)
Built on the stack by 0x54b970, 0x54c900, 0x54c750/0x54deb0 and consumed by 0x547070 (vtable
SetPosition 0x38 / SetOrientation 0x34 / SetVelocity 0x40, environment compared as 0x12 dwords),
0x5472d0 (SetPosition 0x4c, SetConeOrientation 0x38, SetVelocity 0x50) and 0x5475b0 (SetFrequency,
SetVolume, SetMaxDistance 0x40, SetMinDistance 0x44, SetConeAngles 0x34, SetConeOutsideVolume 0x3c).
The IDirectSound3DBuffer / IDirectSound3DListener vtable slot numbers were matched against the
DirectSound SDK order.

### sound_driver_parameters (0x14), sound_channel_binding (0x04), sound_driver (0x40)
See the binary evidence above. Binding table: 0x5482e0 and the 0x548380.. wrappers (entry = channel
index, entry+2 = channel type); 0x545e20 fills it and counts it into 0x007252e2.
Unresolved: channel_counts vs slot_counts naming is UNSURE (both are per-type counts; the first is
decremented when a hardware buffer fails to create, the second sums into the logical channel count);
the two `unknown` arguments of channel_play / channel_continue / channel_set_parameters.

### sound_stream_decoder (0x5cc) and sound_ogg_memory_file (0x10)
0x545760 zeroes 0xb4 dwords at +0x08 and +0x2d8 (two 0x2d0 OggVorbis_File) and the dword groups at
0x5ac and 0x5bc; 0x544eb0 fills a memory file (+0 position, +4 data, +8 size, +0xc byte) and opens
the matching file, setting 0x5a8; 0x544e00 (seek) bounds +0 by +8 and clears +0xc; 0x5451d0 crosslaps
between the two files by 0x5a9 and adds to +0x04; 0x545920 compares +0x04 with buffer_size.
The decoder sits at directsound_channel+0xa0: confirmed by disassembly of 0x547ab0
(`lea ecx, [ebp+0xa0]` before 0x545920, `lea esi, [ebp+0xa0]` before 0x545760, and the same
address passed as the PCM position pointer to 0x545860). That also explains 0x00725a78 in 0x547f60:
it is channel 0 decoder.open (0xa0 + 0x5a8 = 0x648).
Unresolved: 0x5aa (padding).

### directsound_channel (0x678), 0x00725430
0x546760 (create) writes 0x02, 0x04, 0x08, 0x09, 0x38, 0x68, 0x84, 0x88, 0x8c, 0x670, 0x674;
0x5472d0 caches 0x06, 0x07, 0x0c..0x2c, 0x44, 0x48; 0x5475b0 caches 0x3c, 0x40, 0x4c..0x60, 0x6c, 0x70;
0x547c80/0x5478c0/0x547a00/0x547ab0/0x548050 drive 0x00, 0x78, 0x7c, 0x84, 0x88, 0x8c, 0x90, 0x91, 0x98;
0x547f60 resets 0x00, 0x04, 0x08, 0x09, 0x84, 0x88, 0x8c, 0x98 and decoder.open. The base, stride and
0x3c/0x670 were confirmed by disassembly of 0x546fe0 (`[esi+0x725430]`, `[esi+0x3c]`, `[esi+0x670]`).
Unresolved (never referenced by any function in the module): 0x0a, 0x30..0x37, 0x3a, 0x64, 0x6e,
0x74, 0x80, 0x92..0x97, 0x99..0x9f, 0x66c.

### sound_effect_object (0x1c) and sound_eax_effect_object (0xe8)
0x54ec60 / 0x54f270 / 0x550370 write 0x08, 0x10, 0x14, 0x18 and zero the 51 channel property sets;
0x551270 / 0x5514d0 write 0x00 (vtable) and 0x04 (mode 2/1/0); 0x5480f0 and 0x5482a0 test 0x04.
The EAX listener Set calls go through channel_property_sets[0] (`this+0x1c`), the per-channel ones
through `this + 0x1c + index*4`.
Unresolved: 0x0c (never referenced).

## Globals

Listed at the end of types/sound.h with one `// global` line each. Inferred bounds (UNSURE):
k_maximum_sound_channels = 81 (three array gaps agree exactly, the driver probe caps at
51 + 10 + 8 + 8 = 77), sound_cluster_audible_bitmap[16] (gap 0x00746160..0x007461a0),
sound_permutation_limit at 0x007252b8 (0 keeps only permutation 0, 1 picks from the first half,
for non-dialog, non-music classes, in 0x545590), sound_dialog_unspatialized at 0x007252bc,
directsound_hardware_mode at 0x0074612c.

## Misattributed or misnamed functions

- **0x545b70 shell_get_command_line_argument**: not a shell function and not a real entry. It is the
  tail of FUN_00545a30 (0x545a30 + 320 = 0x545b70) that Ghidra split off: it continues the same
  EBP frame, the same DSBUFFERDESC builds and releases the same local interface arrays. Types skipped
  as a separate function; covered by sound_buffer_description.
- **0x54e200 sound_stop_all**: not a function. It is a fragment of FUN_0054deb0 (shares
  LAB_0054e2c8, runs on its stack frame through in_stack_ pseudo-variables, 0 callers). Nothing to
  do with stopping all sounds (that is the driver stop_all slot 0x546fa0 and 0x54adb0).
- **0x546a60 game_sound_dispose**: this is the DirectSound driver dispose (sound_driver slot 0x08),
  not game sound. game_sound has no dispose in this range.
- **0x543a90 chimera__revert**: the Chimera hint prefix; the code is the game looping sound
  back-reference clear (SoundLooping.runtime_scripting_sound).
- **0x5480f0 FUN_005480f0**: the driver set_quality slot (0x38), not a general environment setter.
- **0x544000 / 0x544c10**: the Phase 2 evidence describes an object attachment table; the
  arithmetic is SoundLooping.tracks (count 0x3c, pointer 0x40, stride 0xa0, loop.tag_id at +0x4c)
  and Sound.pitch_ranges, so these are looping sound definition helpers (music class test, preload).
- **0x5448c0**: a sound_location_proc (object marker transform), called through sound.location_proc.
- **0x54e8c0 / 0x54e830**: ADPCM decode step and codec dispatch; codec code with no module structs
  beyond the decode proc table at 0x0065e640.
- **0x544f70, 0x5451d0, 0x545760, 0x544eb0**: thin wrappers around libvorbisfile (ov_* are DLL
  imports); OggVorbis_File is kept opaque (0x2d0 bytes).
- Driver methods 0x545e20, 0x546f90, 0x546fa0, 0x546fe0, 0x548380, 0x5483d0, 0x548410, 0x548450,
  0x548470, 0x5484d0, 0x5482a0 and the EAX vtable slots 0x54ef10, 0x54ef20, 0x54f6e0, 0x54f720,
  0x54ff30, 0x54ff70, 0x551180, 0x5511c0, 0x551240..0x551260, 0x54edf0, 0x54ee30 are not Ghidra
  functions (only reached through tables) and are not in the 134-function list.
