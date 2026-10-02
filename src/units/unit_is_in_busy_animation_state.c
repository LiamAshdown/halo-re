// unit_is_in_busy_animation_state  (Ghidra: FUN_00569c90)
// address 0x569c90, size 56 bytes, name confidence 0.4, rewrite confidence 0.6
// functions.md: "Returns whether the unit's current weapon/vehicle-transition mode byte is one
// of the reserved 'busy' state values."
// evidence: types/units.h unit_data.animation_state (0x2a3); the same case list appears in
//   unit_is_seat_control_available (0x5693a0), unit_try_ready_weapon (0x569a20) and
//   unit_try_ready_weapon_variant (0x569b30).
// blam-cc: in_ECX -> unit_index.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "units.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern data_array *object_data; // 0x008603b0

uint8_t unit_is_in_busy_animation_state(uint32_t unit_index) // blam-cc: in_ECX
{
    object *unit_obj = ((object_header *)object_data->data)[unit_index & 0xffff].data;
    unit_data *unit = (unit_data *)((uint8_t *)unit_obj + k_unit_data_offset);

    switch (unit->animation_state) {
    case 0x17: case 0x18: case 0x19: case 0x1a: case 0x1b: case 0x1d: case 0x1e: case 0x1f:
    case 0x20: case 0x21: case 0x22: case 0x23: case 0x27: case 0x29:
        return 1;
    }
    return 0;
}

#if 0
Original Ghidra decompilation (0x569c90):

undefined1 FUN_00569c90(void)

{
  undefined1 uVar1;
  uint in_ECX;

  uVar1 = 0;
  switch(*(undefined1 *)
          (*(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (in_ECX & 0xffff) * 0xc) + 0x2a3)) {
  case 0x17:
  case 0x18:
  case 0x19:
  case 0x1a:
  case 0x1b:
  case 0x1d:
  case 0x1e:
  case 0x1f:
  case 0x20:
  case 0x21:
  case 0x22:
  case 0x23:
  case 0x27:
  case 0x29:
    uVar1 = 1;
  }
  return uVar1;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
