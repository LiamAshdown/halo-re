// ui_profile_require_existing  (Ghidra: FUN_004a1c30, renamed)
// renamed from FUN_004a1c30 in the naming pass
// address 0x4a1c30, size 80 bytes, callers=0 in this build
// name confidence: 0.3   rewrite confidence: 0.8
// evidence: rewritten in the phase-4 review from objdump -d 0x4a1c30..0x4a1c7f. Ghidra dropped
// the else branch as unreachable because it lost the EBX count argument of 0x53c4e0: the
// routine asks for at most one saved profile (EBX points at a count of 1) and returns 1 when one
// exists; otherwise it forwards its three event arguments to ui_new_profile_name_entry_open (open the new profile
// name entry) and returns 0. A ui_event_function shape (widget, event, out_handled).
// register convention: cdecl, three stack parameters; returns a byte.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"

extern void saved_game_enumerate_by_type(int32_t type, int32_t *out_slots, int32_t unknown,
                                         int16_t *in_out_count); // 0x53c4e0, blam-cc: EBX in_out_count (capacity in, found out)
extern uint8_t ui_new_profile_name_entry_open(void *widget, int16_t *event, uint8_t *out_handled); // 0x4a1940, new profile name entry

uint8_t ui_profile_require_existing(void *widget, int16_t *event, uint8_t *out_handled)
{
    int16_t count = 1;
    int32_t slot;

    saved_game_enumerate_by_type(0, &slot, 0, &count);
    if (count > 0) {
        return 1;
    }
    ui_new_profile_name_entry_open(widget, event, out_handled);
    return 0;
}

#if 0
Original Ghidra decompilation (0x4a1c30):

/* WARNING: Removing unreachable block (ram,0x004a1c60) */

undefined4 FUN_004a1c30(void)

{
  undefined1 local_4 [4];

  saved_game_enumerate_by_type(0,local_4,0);
  return 1;
}
#endif
