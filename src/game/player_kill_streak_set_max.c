// player_kill_streak_set_max  (Ghidra: FUN_00479ca0; renamed per the same kill-streak trio
// described in out/phase4/game_types_notes.md as 0x479ba0/0x479ca0/0x479d10)
// address 0x479ca0, size 99 bytes
// name confidence: 0.4   rewrite confidence: 0.6
// FIXED 2026-09-28: unit_data begins at object +0x1f4 (k_unit_data_offset) and its field offsets are absolute; the draft cast the object pointer itself, so unit fields landed 0x1f4 bytes low (e.g. flags at object +0x10).
// evidence: VERIFIED against the disassembly (objdump -d -M intel --start-address=0x479ca0
//   --stop-address=0x479d10): EAX is the player index, ESI (Ghidra's `unaff_SI`) the candidate
//   value, and `slot` the one stack parameter. types/game.h player::kill_streak (0x68);
//   types/units.h unit_flags::_unit_flag_unknown_10, unit::unknown_422.
// register convention: player index in EAX (in_EAX), candidate value in SI (unaff_SI); `slot`
//   is this function's own stack parameter.
//   // blam-cc: EAX -> player_index, ESI -> value, stack -> slot

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "units.h"
#include "game.h"

extern data_array *player_data;    // 0x0087a480
extern data_array *object_data; // 0x008603b0

// blam-cc: EAX -> player_index, ESI -> value, stack -> slot
// The first time slot 0's streak is touched (it reads 0), marks the player's unit with
// unit_flags bit 0x10 and stamps `slot` into unit::unknown_422. Either way, raises
// player::kill_streak[slot] to `value` if `value` is larger.
void player_kill_streak_set_max(int16_t slot, uint32_t player_index, int16_t value)
{
    player *p = (player *)((uint8_t *)player_data->data + (player_index & 0xffff) * sizeof(player));
    int16_t *streak = &p->kill_streak[slot];

    if (*streak == 0 && slot == 0) {
        unit_data *unit = (unit_data *)((uint8_t *)((object_header *)object_data->data)[p->unit & 0xffff].data + k_unit_data_offset);
        unit->flags = unit->flags | _unit_flag_unknown_10;
        unit->unknown_422 = slot;
    }

    if (*streak <= value) {
        *streak = value;
    }
}

#if 0
Original Ghidra decompilation (0x479ca0), from tools/pack.py 0x479ca0:

void FUN_00479ca0(short param_1)

{
  uint *puVar1;
  int iVar2;
  uint in_EAX;
  int iVar3;
  int iVar4;
  short sVar5;
  short unaff_SI;

  iVar4 = (int)param_1;
  iVar3 = (in_EAX & 0xffff) * 0x200 + *(int *)(DAT_0087a480 + 0x34);
  if ((*(short *)(iVar3 + 0x68 + iVar4 * 2) == 0) &&
     (iVar2 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (*(uint *)(iVar3 + 0x34) & 0xffff) * 0xc)
     , iVar4 == 0)) {
    puVar1 = (uint *)(iVar2 + 0x204);
    *puVar1 = *puVar1 | 0x10;
    *(short *)(iVar2 + 0x422) = param_1;
  }
  sVar5 = *(short *)(iVar3 + 0x68 + iVar4 * 2);
  if (sVar5 <= unaff_SI) {
    sVar5 = unaff_SI;
  }
  *(short *)(iVar3 + 0x68 + iVar4 * 2) = sVar5;
  return;
}
#endif
