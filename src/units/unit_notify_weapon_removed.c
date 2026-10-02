// unit_notify_weapon_removed  (Ghidra: FUN_0056ab10)
// address 0x56ab10, size 17 bytes, name confidence 0.3, rewrite confidence 0.6
// functions.md: "Calls FUN_00565f90 when the implicit weapon/object index is valid, used as a
// small guard before weapon-related teardown."
// blam-cc: in_EAX -> object_index, remaining args of unit_try_set_animation_state forwarded
//   implicitly (not visible in this decompilation).

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "units.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern uint8_t unit_try_set_animation_state(uint32_t unit_index, int16_t new_state); // 0x565f90

void unit_notify_weapon_removed(int32_t object_index) // blam-cc: EAX -> object_index
{
    // objdump 0x56ab10: push 0x25; push eax; call 0x565f90 -- the state is always 0x25
    if (object_index != -1) {
        unit_try_set_animation_state((uint32_t)object_index, 0x25);
    }
}

#if 0
Original Ghidra decompilation (0x56ab10):

void FUN_0056ab10(void)

{
  int in_EAX;

  if (in_EAX != -1) {
    unit_try_set_animation_state();
  }
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
