// game_safe_to_save  (Ghidra: game_safe_to_save, already named)
// address 0x45ba50, size 395 bytes
// name confidence: 0.9   rewrite confidence: 0.6
// evidence: the eight "not safe to save: ..." strings, one per check, printed only when the
//   debug flag at 0x00719aa9 is set; types/objects.h object_iterator / _object_mask_projectile.
//   Shares its first four checks with game_safe_to_pause.c (0x45b9e0, this batch).
// register convention: __cdecl, no arguments.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "fn_game.h"
#include "fn_units.h"
#include "fn_main.h"

extern uint8_t debug_print_safety_checks; // 0x00719aa9, TYPES-GAP

extern object *object_iterator_next(object_iterator *iterator); // 0x4f6f20, objects module


extern uint8_t ai_scan_for_recent_combat_activity(uint32_t hard_difficulty);   // UNSURE module: "ai_enemies_can_see_player"
extern uint8_t item_any_detonating(void);               // UNSURE module: "dangerous_items_near_player"
extern uint8_t effect_check_object_collisions(void);               // UNSURE module: "dangerous_effects_near_player"


// Returns whether it is currently safe to auto/quick-save, checking for nearby AI threats,
// dangerous projectiles/items/effects, dangerous units, airborne/dead players, and moving
// vehicles, logging the specific reason to the console when debug_print_safety_checks is set.
uint8_t game_safe_to_save(void)
{
    object_iterator iterator;

    if (ai_scan_for_recent_combat_activity(0) != 0) {
        if (debug_print_safety_checks != 0) {
            console_print_va("not safe to save: ai_enemies_can_see_player");
        }
        return 0;
    }

    iterator.type_mask = _object_mask_projectile;
    iterator.flags_mask = 0;
    iterator.index = 0;
    iterator.handle = k_datum_index_none;

    if (object_iterator_next(&iterator) != (object *)0) {
        if (debug_print_safety_checks != 0) {
            console_print_va("not safe to save: dangerous_projectiles_near_player");
        }
        return 0;
    }
    if (item_any_detonating() != 0) {
        if (debug_print_safety_checks != 0) {
            console_print_va("not safe to save: dangerous_items_near_player");
        }
        return 0;
    }
    if (effect_check_object_collisions() != 0) {
        if (debug_print_safety_checks != 0) {
            console_print_va("not safe to save: dangerous_effects_near_player");
        }
        return 0;
    }
    if (unit_any_dying_or_seat_transition() != 0) {
        if (debug_print_safety_checks != 0) {
            console_print_va("not safe to save: any_unit_is_dangerous");
        }
        return 0;
    }
    if (players_any_pending_seat_or_respawn() != 0) {
        if (debug_print_safety_checks != 0) {
            console_print_va("not safe to save: any_player_is_in_the_air");
        }
        return 0;
    }
    if (players_any_without_unit() != 0) {
        if (debug_print_safety_checks != 0) {
            console_print_va("not safe to save: any_player_is_dead");
        }
        return 0;
    }
    if (unit_is_area_clear_of_fast_objects() != 0) {
        if (debug_print_safety_checks != 0) {
            console_print_va("not safe to save: vehicle_moving_near_any_player");
        }
        return 0;
    }
    return 1;
}

#if 0
Original Ghidra decompilation (0x45ba50), from tools/pack.py 0x45ba50:

char __cdecl game_safe_to_save(void)

{
  char cVar1;
  int iVar2;
  undefined4 local_10;
  undefined1 local_c;
  undefined2 local_a;
  undefined4 local_8;
  undefined4 local_4;

  cVar1 = FUN_0042c3e0(0);
  if (cVar1 == '\0') {
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
            cVar1 = FUN_00475090();
            if (cVar1 == '\0') {
              cVar1 = players_any_without_unit();
              if (cVar1 == '\0') {
                cVar1 = FUN_00575c50();
                if (cVar1 == '\0') {
                  return '\x01';
                }
                if (DAT_00719aa9 != '\0') {
                  console_print_va("not safe to save: vehicle_moving_near_any_player");
                }
              }
              else if (DAT_00719aa9 != '\0') {
                console_print_va("not safe to save: any_player_is_dead");
                return '\0';
              }
            }
            else if (DAT_00719aa9 != '\0') {
              console_print_va("not safe to save: any_player_is_in_the_air");
              return '\0';
            }
          }
          else if (DAT_00719aa9 != '\0') {
            console_print_va("not safe to save: any_unit_is_dangerous");
            return '\0';
          }
        }
        else if (DAT_00719aa9 != '\0') {
          console_print_va("not safe to save: dangerous_effects_near_player");
          return '\0';
        }
      }
      else if (DAT_00719aa9 != '\0') {
        console_print_va("not safe to save: dangerous_items_near_player");
        return '\0';
      }
    }
    else if (DAT_00719aa9 != '\0') {
      console_print_va("not safe to save: dangerous_projectiles_near_player");
      return '\0';
    }
  }
  else if (DAT_00719aa9 != '\0') {
    console_print_va("not safe to save: ai_enemies_can_see_player");
    return '\0';
  }
  return '\0';
}
#endif
