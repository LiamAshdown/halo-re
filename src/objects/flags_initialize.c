// flags_initialize
// address 0x4fb4d0, size 28 bytes
// name confidence: 0.7 (functions.md: "Creates the global CTF flag instance data array
//   (data_new(\"flag\", 2))"; matches types/objects.h globals list entry for flag_data)
// rewrite confidence: 0.8
// evidence: types/objects.h globals list (flag_data 0x008603a8, k_maximum_flags 2); identical
//   shape to lights_initialize.c's `game_state_new(name, count)` call, minus the checksum
//   registration lights_initialize also does.
// register convention: none (no parameters).

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"

extern data_array *flag_data; // 0x008603a8

extern data_array *game_state_new(char *name, int32_t maximum_count); // memory module, 0x5380d0

void flags_initialize(void)
{
    flag_data = game_state_new("flag", k_maximum_flags);
}

#if 0
Original Ghidra decompilation (0x4fb4d0):

void flags_initialize(void)

{
  DAT_008603a8 = game_state_new(&DAT_0066e99c,2);
  return;
}
#endif
