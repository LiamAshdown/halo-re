// game_no_player_is_dead  (Ghidra: FUN_0045bbe0; renamed per symbols/review_queue.txt)
// address 0x45bbe0, size 75 bytes
// name confidence: 0.45   rewrite confidence: 0.55
// evidence: same object_iterator / players_any_without_unit checks as
//   game_safe_to_save.c's "dangerous_projectiles_near_player" and "any_player_is_dead" cases.
//   The review_queue's one-line description calls this "the single predicate that corresponds
//   to any_player_is_dead", but the decompiled code also repeats the projectile-iterator check;
//   kept as observed.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "fn_game.h"

extern object *object_iterator_next(object_iterator *iterator); // 0x4f6f20, objects module


// Returns true only when there are no nearby dangerous projectiles and no player is currently
// without a controlled unit (dead/respawning).
uint32_t game_no_player_is_dead(void)
{
    object_iterator iterator;

    iterator.type_mask = _object_mask_projectile;
    iterator.flags_mask = 0;
    iterator.index = 0;
    iterator.handle = k_datum_index_none;

    if (object_iterator_next(&iterator) == (object *)0) {
        if (players_any_without_unit() == 0) {
            return 1;
        }
    }
    return 0;
}

#if 0
Original Ghidra decompilation (0x45bbe0), from tools/pack.py 0x45bbe0:

undefined4 FUN_0045bbe0(void)

{
  char cVar1;
  int iVar2;
  undefined4 local_10;
  undefined1 local_c;
  undefined2 local_a;
  undefined4 local_8;
  undefined4 local_4;

  local_c = 0;
  local_a = 0;
  local_4 = 0x86868686;
  local_10 = 0x20;
  local_8 = 0xffffffff;
  iVar2 = object_iterator_next(&local_10);
  if (iVar2 == 0) {
    cVar1 = players_any_without_unit();
    if (cVar1 == '\0') {
      return 1;
    }
  }
  return 0;
}
#endif
