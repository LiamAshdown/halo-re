// player_kill_streak_begin  (Ghidra: FUN_00479d90; renamed -- called from
// player_add_kill_streak.c (this batch) the first time a streak slot is touched)
// address 0x479d90, size 67 bytes
// name confidence: 0.3   rewrite confidence: 0.6
// FIXED 2026-09-28: unit_data begins at object +0x1f4 (k_unit_data_offset) and its field offsets are absolute; the draft cast the object pointer itself, so unit fields landed 0x1f4 bytes low (e.g. flags at object +0x10).
// evidence: VERIFIED against the disassembly (objdump -d -M intel --start-address=0x479ba0
//   --stop-address=0x479ca0), which shows the caller doing `mov eax,ebx; push ebp; call
//   0x479d90` (EAX = the same player handle player_add_kill_streak.c received, `ebp` = slot);
//   types/units.h unit_flags::_unit_flag_unknown_10, unit::unknown_422.
// register convention: a player handle in EAX (in_EAX); `slot` is this function's own stack
//   parameter.
//   // blam-cc: EAX -> player_handle, stack -> slot

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "units.h"
#include "game.h"

extern data_array *player_data;    // 0x0087a480
extern data_array *object_data; // 0x008603b0

// blam-cc: EAX -> player_handle, stack -> slot
// For slot 0 only, sets unit_flags bit 0x10 on the player's unit and resets unit::unknown_422
// to 0.
void player_kill_streak_begin(int16_t slot, uint32_t player_handle)
{
    player *p = (player *)((uint8_t *)player_data->data + (player_handle & 0xffff) * sizeof(player));
    unit_data *unit = (unit_data *)((uint8_t *)((object_header *)object_data->data)[p->unit & 0xffff].data + k_unit_data_offset);

    if (slot == 0) {
        unit->flags = unit->flags | _unit_flag_unknown_10;
        unit->unknown_422 = 0;
    }
}

#if 0
Original Ghidra decompilation (0x479d90), from tools/pack.py 0x479d90:

void FUN_00479d90(short param_1)

{
  uint *puVar1;
  int iVar2;
  uint in_EAX;

  iVar2 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 +
                  (*(uint *)((in_EAX & 0xffff) * 0x200 + 0x34 + *(int *)(DAT_0087a480 + 0x34)) &
                  0xffff) * 0xc);
  if (param_1 == 0) {
    puVar1 = (uint *)(iVar2 + 0x204);
    *puVar1 = *puVar1 | 0x10;
    *(undefined2 *)(iVar2 + 0x422) = 0;
  }
  return;
}
#endif
