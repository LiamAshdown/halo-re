// contrails_initialize  (Ghidra: contrails_initialize, already named -- cea-pdb hint on the
// "contrail"/"contrail point" strings)
// address 0x44c8b0, size 89 bytes
// name confidence: 0.9   rewrite confidence: 0.85
// evidence: types/effects.h k_maximum_contrails (0x100), k_maximum_contrail_points (0x400);
// contrail is 0x44 bytes and contrail_point is 0x38 bytes per the same header. src/game/
// game_initialize.c already establishes game_state_new's real signature (element size in EBX,
// name and maximum_count on the stack); Ghidra elides the EBX argument here exactly as it does
// there.
// register convention: no arguments.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "effects.h"

extern data_array *contrail_data;       // 0x0087abec
extern data_array *contrail_point_data; // 0x0087abe8

extern void *game_state_new(int16_t element_size, char *name, int16_t maximum_count); // 0x5380d0
    // blam-cc: EBX -> element_size, stack -> (name, maximum_count)

// Registers the contrail and contrail-point datum tables. If only one of the two allocations
// succeeds, that handle is cleared too, so the module is left fully enabled or fully disabled.
void contrails_initialize(void)
{
    contrail_data = (data_array *)game_state_new(sizeof(contrail), "contrail", k_maximum_contrails);
    contrail_point_data = (data_array *)game_state_new(sizeof(contrail_point), "contrail point",
        k_maximum_contrail_points);

    if (contrail_data == 0) {
        if (contrail_point_data != 0) {
            contrail_point_data = 0;
        }
    } else if (contrail_point_data == 0) {
        contrail_data = 0;
    }
}

#if 0
Original Ghidra decompilation (0x44c8b0):

void __cdecl contrails_initialize(void)

{
  DAT_0087abec = game_state_new("contrail",0x100);
  DAT_0087abe8 = game_state_new("contrail point",0x400);
  if (DAT_0087abec == 0) {
    if (DAT_0087abe8 != 0) {
      DAT_0087abe8 = 0;
      return;
    }
  }
  else if (DAT_0087abe8 == 0) {
    DAT_0087abec = 0;
  }
  return;
}
#endif
