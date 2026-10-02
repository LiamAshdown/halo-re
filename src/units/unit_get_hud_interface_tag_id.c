// unit_get_hud_interface_tag_id  (Ghidra: unit_get_hud_interface_tag_id)
// address 0x560c70, size 52 bytes
// name confidence: 0.3 (renamed from phase2's "unit_get_permutation_by_index"; see evidence)
//   rewrite confidence: 0.5
// evidence: types/tags.h Unit.new_hud_interfaces (TagReflexive at 0x2a8, computed: Unit struct
//   layout puts it 8 bytes after the _pad_2a0[8] that follows metagame_class), stride 0x30
//   matches UnitUnitHudInterface (0x30 bytes, TagDependency hud at +0x0, so +0xc is hud.tag_id).
//   Confirmed against unit_get_seat_hud_interface_tag_id (0x560cb0), which reads the same
//   +0xc-of-a-0x30-record pattern out of UnitSeat.unit_hud_interface (offset 0xdc/0xe0) instead.
// register convention: unit tag data pointer in EDX, "use the second entry" flag in AL.
//   // blam-cc: in_EDX -> unit_tag, in_AL -> use_second
// UNSURE: functions.md's summary ("permutation record from a unit's variant table") does not
//   match this decompilation; renamed to reflect what the offsets actually resolve to.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"
#include "units.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
TagID unit_get_hud_interface_tag_id(Unit *unit_tag, uint8_t use_second) // blam-cc: in_EDX -> unit_tag, in_AL -> use_second
{
    int32_t index = (int32_t)unit_tag->new_hud_interfaces.count - 1;
    if (index > (use_second != 0)) {
        index = use_second != 0;
    }
    if (index < 0) {
        TagID none = {0xffff, 0xffff};
        return none;
    }
    UnitUnitHudInterface *entries = (UnitUnitHudInterface *)unit_tag->new_hud_interfaces.pointer;
    return entries[(int16_t)index].hud.tag_id;
}

#if 0
Original Ghidra decompilation (0x560c70):

undefined4 FUN_00560c70(void)

{
  char in_AL;
  int iVar1;
  int in_EDX;

  iVar1 = *(int *)(in_EDX + 0x2a8) + -1;
  if ((short)(ushort)(in_AL != '\0') <= iVar1) {
    iVar1 = (int)(short)(ushort)(in_AL != '\0');
  }
  if ((short)iVar1 < 0) {
    return 0xffffffff;
  }
  return *(undefined4 *)((short)iVar1 * 0x30 + 0xc + *(int *)(in_EDX + 0x2ac));
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
