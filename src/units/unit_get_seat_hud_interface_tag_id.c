// unit_get_seat_hud_interface_tag_id  (Ghidra: unit_get_seat_hud_interface_tag_id)
// address 0x560cb0, size 73 bytes
// name confidence: 0.3 (renamed from phase2's "unit_get_seat_permutation_by_index"; see
//   unit_get_hud_interface_tag_id, 0x560c70, for the shared evidence)   rewrite confidence: 0.5
// evidence: types/tags.h Unit.seats (TagReflexive at 0x2e4/0x2e8, UnitSeat stride 0x11c),
//   UnitSeat.unit_hud_interface (TagReflexive at 0xdc/0xe0), UnitUnitHudInterface (0x30 bytes,
//   hud TagDependency at +0x0, tag_id at +0xc).
// register convention: unit tag data pointer in ECX, seat index in AX, "use the second entry"
//   flag in the stack param.
//   // blam-cc: in_ECX -> unit_tag, in_AX -> seat_index, stack param_1 -> use_second

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"
#include "units.h"

TagID unit_get_seat_hud_interface_tag_id(Unit *unit_tag, int16_t seat_index, uint8_t use_second) // blam-cc: see file header
{
    UnitSeat *seat = (UnitSeat *)((uint8_t *)unit_tag->seats.pointer + seat_index * 0x11c);
    int32_t index = (int32_t)seat->unit_hud_interface.count - 1;

    if (index > (use_second != 0)) {
        index = use_second != 0;
    }
    if (index < 0) {
        TagID none = {0xffff, 0xffff};
        return none;
    }
    UnitUnitHudInterface *entries = (UnitUnitHudInterface *)seat->unit_hud_interface.pointer;
    return entries[(int16_t)index].hud.tag_id;
}

#if 0
Original Ghidra decompilation (0x560cb0):

undefined4 FUN_00560cb0(char param_1)

{
  short in_AX;
  int iVar1;
  int in_ECX;
  int iVar2;

  iVar1 = in_AX * 0x11c + *(int *)(in_ECX + 0x2e8);
  iVar2 = *(int *)(iVar1 + 0xdc) + -1;
  if ((short)(ushort)(param_1 != '\0') <= iVar2) {
    iVar2 = (int)(short)(ushort)(param_1 != '\0');
  }
  if ((short)iVar2 < 0) {
    return 0xffffffff;
  }
  return *(undefined4 *)((short)iVar2 * 0x30 + 0xc + *(int *)(iVar1 + 0xe0));
}
#endif
