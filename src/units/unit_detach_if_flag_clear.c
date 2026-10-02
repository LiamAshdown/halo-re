// unit_detach_if_flag_clear  (Ghidra: unit_get_camera_position -- misattributed, see
// out/phase4/units_types_notes.md: "conditionally calls 0x56c640 to detach the unit from its
// seat; the real camera-position function is 0x568f80")
// address 0x56c440, size 29 bytes, name confidence 0.3, rewrite confidence 0.7
// blam-cc: in_AL -> skip_flag, remaining args of unit_detach_from_seat forwarded implicitly
//   (not visible in this decompilation).

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "units.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern void unit_detach_from_seat(uint32_t unit_index, uint8_t suppress_trigger, uint8_t require_client_flag, uint8_t fire_trigger_event); // 0x56c640

void unit_detach_if_flag_clear(uint8_t skip_flag, uint32_t unit_index, uint8_t suppress_trigger,
                               uint8_t require_client_flag, uint8_t fire_trigger_event) // blam-cc: in_AL, UNSURE rest
{
    if (!skip_flag) {
        unit_detach_from_seat(unit_index, suppress_trigger, require_client_flag, fire_trigger_event);
    }
    return;
}

#if 0
Original Ghidra decompilation (0x56c440):

void unit_get_camera_position(void)

{
  char in_AL;

  if (in_AL == '\0') {
    FUN_0056c640();
    return;
  }
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
