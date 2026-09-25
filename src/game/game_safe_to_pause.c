// game_safe_to_pause  (Ghidra: FUN_0045b9e0; renamed per symbols/review_queue.txt)
// address 0x45b9e0, size 108 bytes
// name confidence: 0.45   rewrite confidence: 0.6
// evidence: types/objects.h object_iterator / _object_mask_projectile (0x20); matches
//   game_safe_to_save.c's (0x45ba50, this batch) first four checks exactly (dangerous
//   projectiles near a player, then three more danger predicates), omitting its remaining four.
//   The local `0x86868686`-tagged stack dword this function also sets is not part of the
//   object_iterator struct (it sits past its 0xc-byte end) and is dropped here, matching the
//   precedent in src/devices/device_group_set_value.c's own object_iterator_next call.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"

extern object *object_iterator_next(object_iterator *iterator); // 0x4f6f20, objects module

extern uint8_t item_any_detonating(void); // UNSURE module: "dangerous_items_near_player"
extern uint8_t effect_check_object_collisions(void); // UNSURE module: "dangerous_effects_near_player"
extern uint8_t unit_any_dying_or_seat_transition(void); // UNSURE module: "any_unit_is_dangerous"
extern uint8_t ai_scan_for_recent_combat_activity(uint32_t param_1); // UNSURE module: "ai_enemies_can_see_player"

// Returns whether it is currently safe to pause: no nearby dangerous projectiles, items,
// effects or units. A quieter subset of game_safe_to_save's checks (no AI-visibility check, no
// console logging).
uint32_t game_safe_to_pause(void)
{
    object_iterator iterator;

    iterator.type_mask = _object_mask_projectile;
    iterator.flags_mask = 0;
    iterator.index = 0;
    iterator.handle = k_datum_index_none;

    if (object_iterator_next(&iterator) == (object *)0) {
        if (item_any_detonating() == 0 && effect_check_object_collisions() == 0 && unit_any_dying_or_seat_transition() == 0 && ai_scan_for_recent_combat_activity(0) == 0) {
            return 1;
        }
    }
    return 0;
}

#if 0
Original Ghidra decompilation (0x45b9e0), from tools/pack.py 0x45b9e0:

undefined4 FUN_0045b9e0(void)

{
  char cVar1;
  int iVar2;
  undefined4 local_10;
  undefined1 local_c;
  undefined2 local_a;
  undefined4 local_8;
  undefined4 local_4;

  local_4 = 0x86868686;
  local_10 = 0x20;
  local_c = 0;
  local_a = 0;
  local_8 = 0xffffffff;
  iVar2 = object_iterator_next(&local_10);
  if (iVar2 == 0) {
    cVar1 = FUN_004bcf50();
    if (cVar1 == '\0') {
      cVar1 = FUN_00450fa0();
      if (cVar1 == '\0') {
        cVar1 = FUN_0056c070();
        if (cVar1 == '\0') {
          cVar1 = FUN_0042c3e0(0);
          if (cVar1 == '\0') {
            return 1;
          }
        }
      }
    }
  }
  return 0;
}
#endif
