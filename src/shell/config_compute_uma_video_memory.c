// config_compute_uma_video_memory  (Ghidra: no function created; the phase-4 types agent carved a
//   placeholder "missed_57d250" from the config-property-table evidence)
// address 0x57d250, size 67 bytes
// name confidence 0.5, rewrite confidence 0.7
// evidence: out/phase4/shell_types_notes.md: "The remaining 22 setters (0x57d0c0,
//   0x57d110..0x57d230, 0x57d250 UMA, 0x57d2a0) are not Ghidra functions" -- explicitly called
//   out as the one entry in that run of setters that is not a config.txt property setter (it
//   takes no value string). Reads physical_memory (0x00722ba8, MB) and derives a tiered UMA
//   video-memory default in bytes (8/16/32/64 MB) into video_memory (0x00722bb0); the header's
//   own comment on that field ("or the UMA config value (8/16/32 MB by physical memory)") is the
//   3-tier summary of this function's real 4-tier ladder.
// register convention: no parameters; reads/writes only the two named globals. UNSURE: no caller
//   was found in this pass's range (out/phase4/shell_functions.md does not list one); kept plain
//   __cdecl to match every other setter in this file group, since it shares their bare `uint8_t`
//   return shape.
// blam-cc: (no arguments).
// UNSURE: the original's return value is `CONCAT31((int3)(DAT_00722ba8 >> 8),1)` -- Ghidra
//   showing that the upper three bytes of EAX are leftover from a shift used to compute one of
//   the thresholds, with only the low byte (1) meaningful to any caller treating this as the
//   `uint8_t (*)(const char *)` setter shape. Reproduced as a plain `return 1;`.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "rasterizer.h"
#include "shell.h"

extern uint32_t physical_memory; // 0x00722ba8, MB
extern uint32_t video_memory;    // 0x00722bb0, bytes

// Derives the default UMA (shared/integrated graphics) video memory size from installed physical
// memory, used when no dedicated adapter memory figure is available: 8 MB below 64 MB of RAM,
// 16 MB below 128 MB, 32 MB below 256 MB, else 64 MB.
uint8_t config_compute_uma_video_memory(void)
{
    video_memory = 0x800000; // 8 MB
    if (physical_memory > 0x3f) {
        video_memory = 0x1000000; // 16 MB
    }
    if (physical_memory > 0x7f) {
        video_memory = 0x2000000; // 32 MB
    }
    if (physical_memory > 0xff) {
        video_memory = 0x4000000; // 64 MB
    }
    return 1;
}

#if 0
Original Ghidra decompilation (0x57d250):

undefined4 missed_57d250(void)

{
  DAT_00722bb0 = 0x800000;
  if (0x3f < DAT_00722ba8) {
    DAT_00722bb0 = 0x1000000;
  }
  if (0x7f < DAT_00722ba8) {
    DAT_00722bb0 = 0x2000000;
  }
  if (0xff < DAT_00722ba8) {
    DAT_00722bb0 = 0x4000000;
  }
  return CONCAT31((int3)(DAT_00722ba8 >> 8),1);
}
#endif
