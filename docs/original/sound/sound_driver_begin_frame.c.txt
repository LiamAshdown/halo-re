// sound_driver_begin_frame  (Ghidra: missed_546f90; 0 callers in this module -- reached only
// through the DirectSound driver's begin_frame vtable slot, sound_driver.begin_frame 0x0c)
// address 0x546f90, size 8 bytes
// name confidence: 0.6   rewrite confidence: 0.95
// evidence: out/phase4/sound_types_notes.md "sound_driver (.data 0x0069f4c8 ...) Slots: ...
//   0x10 0x546f90" and types/sound.h sound_driver.begin_frame ("0x10 0x546f90 clears the
//   deferred flag"); the single global it touches is directsound_deferred_dirty
//   (types/sound.h "0x00746132: uint8_t directsound_deferred_dirty CommitDeferredSettings
//   pending"), which sound_driver_end_frame.c (0x546b80, this module) reads and commits.
// register convention: plain __cdecl, no parameters.
// blam-cc: void
// Phase-4 review: checked instruction by instruction against the disassembly appended in the
// #if 0 block; no semantic difference found (two instructions: store 0, ret).

#include "tags.h"
#include "memory.h"
#include "sound.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern uint8_t directsound_deferred_dirty; // 0x00746132

// blam-cc: void
// Clears the "3D listener settings need CommitDeferredSettings" flag at the start of a frame's
// DirectSound work.
void sound_driver_begin_frame(void)
{
    directsound_deferred_dirty = 0;
}

#if 0
Original Ghidra decompilation (0x546f90):

void missed_546f90(void)

{
  DAT_00746132 = 0;
  return;
}

Disassembly (0x546f90..0x546f97; phase-4 review):

0x546f90: mov byte ptr [0x746132], 0
0x546f97: ret
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
