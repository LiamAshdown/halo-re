// player_kill_streak_continue  (Ghidra: FUN_00479de0; renamed -- called from
// player_add_kill_streak.c (this batch) when a streak slot already has an active count and no
// multiplayer engine is loaded)
// address 0x479de0, size 58 bytes
// name confidence: 0.3   rewrite confidence: 0.6
// FIXED 2026-09-28: unit_data begins at object +0x1f4 (k_unit_data_offset) and its field offsets are absolute; the draft cast the object pointer itself, so unit fields landed 0x1f4 bytes low (e.g. flags at object +0x10).
// evidence: VERIFIED against the disassembly (same call site as player_kill_streak_begin.c:
//   `mov eax,ebx; push ebp; call 0x479de0`); types/units.h unit_flags (bit 0x20, unnamed there
//   -- kept as a raw flag literal since no header names it).
// register convention: a player handle in EAX (in_EAX); `slot` is this function's own stack
//   parameter.
//   // blam-cc: EAX -> player_handle, stack -> slot
// UNSURE: unit_flags bit 0x20's identity (types/units.h leaves it unnamed).

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "units.h"
#include "game.h"

extern data_array *player_data;    // 0x0087a480
extern data_array *object_data; // 0x008603b0

// blam-cc: EAX -> player_handle, stack -> slot
// For slot 0 only, sets unit_flags bit 0x20 on the player's unit.
void player_kill_streak_continue(int16_t slot, uint32_t player_handle)
{
    if (slot == 0) {
        player *p = (player *)((uint8_t *)player_data->data + (player_handle & 0xffff) * sizeof(player));
        unit_data *unit = (unit_data *)((uint8_t *)((object_header *)object_data->data)[p->unit & 0xffff].data + k_unit_data_offset);
        unit->flags = unit->flags | 0x20; // UNSURE: bit identity, see header note
    }
}

#if 0
Original Ghidra decompilation (0x479de0), from tools/pack.py 0x479de0:

void FUN_00479de0(short param_1)

{
  uint *puVar1;
  uint in_EAX;

  if (param_1 == 0) {
    puVar1 = (uint *)(*(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 +
                              (*(uint *)((in_EAX & 0xffff) * 0x200 + 0x34 +
                                        *(int *)(DAT_0087a480 + 0x34)) & 0xffff) * 0xc) + 0x204);
    *puVar1 = *puVar1 | 0x20;
  }
  return;
}
#endif
