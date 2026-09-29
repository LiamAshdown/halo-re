// unit_reset_gauge_if_flagged  (Ghidra: FUN_004633a0; renamed -- the decompilation operates on a
// single unit, not a per-team gauge)
// address 0x4633a0, size 72 bytes
// name confidence: 0.25   rewrite confidence: 0.35
// evidence: out/phase4/game_functions.md's summary ("Resets a per-team floating-point
// gauge/timer field to 0.5 when a particular team status bit is set") does not match the body,
// which reads a single unit's own unit_flags (types/units.h, bit 0x10 = _unit_flag_unknown_10)
// and, if set, writes 0.5 into that same unit's active_camo_amount gauge; types/game.h player::unit
// (+0x34).
// register convention: a player index in EAX (in_EAX).
//   // blam-cc: EAX -> player_index

// CORRECTED (phase 4 review): types/units.h unit_data starts at object + k_unit_data_offset
// (0x1f4), so a unit_data * built straight from the object pointer reads every field 0x1f4
// bytes too low. The cast below adds the extension offset.
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "units.h"
#include "game.h"

extern data_array *player_data;    // 0x0087a480
extern data_array *object_data; // 0x008603b0

// blam-cc: EAX -> player_index
void unit_reset_gauge_if_flagged(uint32_t player_index)
{
    player *p;
    unit_data *unit;

    if (player_index == 0xffffffff) {
        return;
    }
    p = (player *)((uint8_t *)player_data->data + (player_index & 0xffff) * sizeof(player));
    if (p->unit == (datum_index)0xffffffff) {
        return;
    }

    unit = (unit_data *)((uint8_t *)
        ((object_header *)object_data->data)[p->unit & 0xffff].data + k_unit_data_offset);
    if ((unit->flags & _unit_flag_unknown_10) != 0) {
        unit->active_camo_amount = 0.5f;
    }
}

#if 0
Original Ghidra decompilation (0x4633a0), from tools/pack.py 0x4633a0:

void FUN_004633a0(void)

{
  uint uVar1;
  int iVar2;
  uint in_EAX;

  if (((in_EAX != 0xffffffff) &&
      (uVar1 = *(uint *)((in_EAX & 0xffff) * 0x200 + *(int *)(DAT_0087a480 + 0x34) + 0x34),
      uVar1 != 0xffffffff)) &&
     (iVar2 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (uVar1 & 0xffff) * 0xc),
     (*(byte *)(iVar2 + 0x204) & 0x10) != 0)) {
    *(undefined4 *)(iVar2 + 0x37c) = 0x3f000000;
  }
  return;
}
#endif
