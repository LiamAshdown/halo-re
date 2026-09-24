// unit_notify_weapon_removed_dup  (Ghidra: FUN_0056ab30)
// address 0x56ab30, size 17 bytes, name confidence 0.25, rewrite confidence 0.6
// functions.md: "Duplicate of unit_notify_weapon_removed; calls FUN_00565f90 when the implicit
// index is valid."
// blam-cc: in_EAX -> object_index, remaining args of unit_try_set_animation_state forwarded
//   implicitly (not visible in this decompilation).

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "units.h"

extern uint8_t unit_try_set_animation_state(uint32_t unit_index, int16_t new_state); // 0x565f90

void unit_notify_weapon_removed_dup(int32_t object_index, int16_t new_state) // blam-cc: in_EAX, UNSURE 2nd arg
{
    if (object_index != -1) {
        unit_try_set_animation_state((uint32_t)object_index, new_state);
    }
    return;
}

#if 0
Original Ghidra decompilation (0x56ab30):

void FUN_0056ab30(void)

{
  int in_EAX;

  if (in_EAX != -1) {
    unit_try_set_animation_state();
  }
  return;
}
#endif
