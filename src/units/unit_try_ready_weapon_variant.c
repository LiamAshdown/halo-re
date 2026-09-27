// unit_try_ready_weapon_variant  (Ghidra: FUN_00569b30)
// address 0x569b30, size 116 bytes, name confidence 0.3, rewrite confidence 0.85
// REWRITTEN from objdump 0x569b30..0x569ba3: the draft asked for state 0x19; the original asks for 0x27 (a
//   biped in the air, object +0xb4 type 0 with +0x4cc bit 0, never gets it) and aims a non-null direction (EDI).
//   Returns whether the state was taken.
// blam-cc: ESI -> unit_index, EDI -> direction

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"
#include "units.h"

extern data_array *object_data;     // 0x008603b0
extern tag_instance *tag_instances; // 0x0087bc14

extern uint8_t unit_try_set_animation_state(uint32_t unit_index, int16_t new_state); // 0x565f90
extern void unit_set_throw_aim_direction(uint32_t object_index, const real_vector2d *direction_xy); // 0x5704d0, EAX, ECX

// 0x17..0x29 jump table (0x569b18 / 0x569bac): only 0x1c, 0x24, 0x25, 0x26 and 0x28 inside that range let it proceed.
static int unit_animation_state_allows_melee(int8_t state)
{
    switch (state) {
    case 0x17: case 0x18: case 0x19: case 0x1a: case 0x1b: case 0x1d: case 0x1e: case 0x1f:
    case 0x20: case 0x21: case 0x22: case 0x23: case 0x27: case 0x29:
        return 0;
    default:
        return 1;
    }
}

uint8_t unit_try_ready_weapon_variant(uint32_t unit_index, const real_vector2d *direction)
{
    uint8_t *unit = (uint8_t *)((object_header *)object_data->data)[unit_index & 0xffff].data;

    if (!unit_animation_state_allows_melee((int8_t)unit[0x2a3])) {
        return 0;
    }
    if (*(int16_t *)(unit + 0xb4) == 0 && (unit[0x4cc] & 1) != 0) {
        return 0;
    }
    if (!unit_try_set_animation_state(unit_index, 0x27)) {
        return 0;
    }
    if (direction != 0) {
        unit_set_throw_aim_direction(unit_index, direction);
    }
    return 1;
}

#if 0
Original Ghidra decompilation (0x569b30):

undefined1 FUN_00569b30(void)

{
  int iVar1;
  char cVar2;
  undefined1 uVar3;
  uint unaff_ESI;
  int unaff_EDI;

  iVar1 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (unaff_ESI & 0xffff) * 0xc);
  uVar3 = 0;
  switch(*(undefined1 *)(iVar1 + 0x2a3)) {
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
    break;
  default:
    if (((*(short *)(iVar1 + 0xb4) != 0) || ((*(byte *)(iVar1 + 0x4cc) & 1) == 0)) &&
       (cVar2 = unit_try_set_animation_state(), cVar2 != '\0')) {
      if (unaff_EDI != 0) {
        FUN_005704d0();
      }
      uVar3 = 1;
    }
  }
  return uVar3;
}
#endif
