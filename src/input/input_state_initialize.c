// input_state_initialize  (Ghidra: input_state_initialize, already named)
// address 0x48b3e0, size 133 bytes
// name confidence: 0.6   rewrite confidence: 0.85
// evidence: zeroes exactly 0x97c dwords (0x25f0 bytes) at 0x00710328, which is
//   sizeof(input_abstraction_globals) per types/input.h; the individual field stores after the
//   clear (last_input_device, unknown_2214/2219, time_base, scan_result, mode_flags bit 0) match
//   the header's field map for the same block exactly (see types/input.h and
//   out/phase4/input_types_notes.md).
// register convention: no parameters, no return value.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"
#include "saved_games.h"
#include "input.h"
#include <string.h>

extern input_abstraction_globals input_globals; // 0x00710328
extern int32_t last_input_device;                            // 0x0087a460
extern int64_t performance_frequency;                         // 0x006ac8f8/0x006ac8fc
extern int32_t __stdcall QueryPerformanceCounter(large_integer *counter); // 0x0063a0ac IAT

// One-time input subsystem initializer: zeroes the whole input abstraction block (settings,
// states, binding tables, scan state), reseeds the millisecond time base from
// QueryPerformanceCounter, clears the cached last-used input device, and marks the subsystem as
// running in game mode.
void input_state_initialize(void)
{
    large_integer counter;

    memset(&input_globals, 0, sizeof(input_globals));

    last_input_device = 0;
    input_globals.unknown_2214 = 1;

    QueryPerformanceCounter(&counter);
    input_globals.time_base = (uint32_t)((counter.quad_part * 1000) / performance_frequency);

    input_globals.scan_result.device_type = 0;   // 0x007127c4 (dword with device_index)
    input_globals.scan_result.device_index = 0;
    input_globals.scan_result.input_kind = 0;    // 0x007127c8 (dword with input_index)
    input_globals.scan_result.input_index = 0;
    input_globals.scan_result.direction = 0;     // 0x007127cc

    input_globals.mode_flags = input_globals.mode_flags | _input_mode_game_bit;
    input_globals.unknown_2219 = 1;
}

#if 0
Original Ghidra decompilation (0x48b3e0), from tools/pack.py 0x48b3e0:

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void input_state_initialize(void)

{
  int iVar1;
  undefined4 *puVar2;
  undefined8 uVar3;
  LARGE_INTEGER local_8;

  puVar2 = &DAT_00710328;
  for (iVar1 = 0x97c; iVar1 != 0; iVar1 = iVar1 + -1) {
    *puVar2 = 0;
    puVar2 = puVar2 + 1;
  }
  DAT_0087a460 = 0;
  DAT_0071253c = 1;
  QueryPerformanceCounter(&local_8);
  uVar3 = __allmul(local_8.s.LowPart,local_8.s.HighPart,1000,0);
  _DAT_00712538 = __alldiv(uVar3,DAT_006ac8f8,DAT_006ac8fc);
  DAT_007127c4 = 0;
  DAT_007127c8 = 0;
  DAT_007127cc = 0;
  DAT_00712542 = DAT_00712542 | 1;
  DAT_00712541 = 1;
  return;
}
#endif
