// cheat_make_selected_object_invincible  (Ghidra: FUN_0045a6c0; renamed per
// symbols/review_queue.txt)
// address 0x45a6c0, size 90 bytes
// name confidence: 0.45   rewrite confidence: 0.5
// evidence: types/game.h player::unit (0x34); types/units.h unit_data::unknown_37c (0x37c,
//   0..1 fraction) and unit_data::flags (0x204, unit_flags bits 0x10/0x20 -- unnamed in
//   types/units.h, kept as the numeric bits it documents).
// register convention: no arguments.
//
// UNSURE: relies on cheat_get_target_object_index (0x45a7a0), which this batch's own analysis
// shows always returns "not found" -- see that file's header. This cheat is therefore dead code
// as shipped; transcribed exactly regardless.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"
#include "units.h"
#include "game.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern data_array *player_data; // 0x0087a480
extern data_array *object_data; // 0x008603b0

extern uint32_t cheat_get_target_object_index(void); // this batch, 0x45a7a0

// Forces the debug-selected player's controlled unit's shield-like field to full and sets its
// invincibility-style flag bits.
void cheat_make_selected_object_invincible(void)
{
    uint32_t player_index;
    datum_index unit_index;
    object *unit_obj;
    unit_data *unit;

    player_index = cheat_get_target_object_index();
    if (player_index != 0xffffffff) {
        unit_index = ((player *)((uint8_t *)player_data->data + (player_index & 0xffff) * sizeof(player)))->unit;
        unit_obj = ((object_header *)object_data->data)[unit_index & 0xffff].data;
        unit = (unit_data *)((uint8_t *)unit_obj + k_unit_data_offset);
        unit->active_camouflage_power = 1.0f;
        if ((unit->flags & 0x10) != 0) {
            unit->flags = unit->flags | 0x20;
        }
        unit->flags = unit->flags | 0x10;
    }
}

#if 0
Original Ghidra decompilation (0x45a6c0), from tools/pack.py 0x45a6c0:

void FUN_0045a6c0(void)

{
  int iVar1;
  uint uVar2;

  uVar2 = FUN_0045a7a0();
  if (uVar2 != 0xffffffff) {
    iVar1 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 +
                    (*(uint *)((uVar2 & 0xffff) * 0x200 + 0x34 + *(int *)(DAT_0087a480 + 0x34)) &
                    0xffff) * 0xc);
    *(undefined4 *)(iVar1 + 0x37c) = 0x3f800000;
    if ((*(uint *)(iVar1 + 0x204) & 0x10) != 0) {
      *(uint *)(iVar1 + 0x204) = *(uint *)(iVar1 + 0x204) | 0x20;
    }
    *(uint *)(iVar1 + 0x204) = *(uint *)(iVar1 + 0x204) | 0x10;
  }
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
