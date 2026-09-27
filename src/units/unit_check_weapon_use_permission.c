// unit_check_weapon_use_permission  (Ghidra: FUN_0056da00)
// address 0x56da00, size 113 bytes, name confidence 0.35, rewrite confidence: 0.9 (VERIFIED against objdump; engine callback FIXED)
// functions.md: "Checks whether the unit's current seat allows using its equipped weapon,
// consulting an optional scripted permission callback."
// evidence: unit_get_seat_or_state_name.c (0x56c2f0, single-argument EAX form, confirmed here
// since this call site shows zero visible arguments too); unit_set_or_test_seat_and_weapon_label
// established signature.
// blam-cc: ESI -> unit_index, EDI -> weapon_index.
// reconciled: R04 0x006f1d20 int32_t network_predicted_state_flag -> game.h game_engine_definition *current_game_engine (all accesses are DWORD; non-NULL = multiplayer engine loaded)
// FIXED (register inputs, objdump): ESI (read at 0x56da00, "mov eax,esi" feeding the two callee
// calls) is the unit index; the register the rewrite called "unit_index" was actually EDI, an
// equipped-weapon object index (indexes object_data to reach a Weapon tag's label at +0x30c) and
// is renamed weapon_index. ESI was missing entirely.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "cache.h"
#include "objects.h"
#include "units.h"

extern data_array *object_data;     // 0x008603b0
extern tag_instance *tag_instances; // 0x0087bc14
extern char s_no_weapon_label[];     // 0x0065512c, shared with unit_pickup_weapon.c FIXED: an array (the binary pushes the ADDRESS as an immediate; a pointer declaration loaded the string bytes)
extern game_engine_definition *current_game_engine; // 0x006f1d20, game.h; non-NULL = multiplayer engine loaded (R04)

extern char * unit_get_seat_or_state_name(uint32_t unit_index);                     // 0x56c2f0
extern uint8_t unit_set_or_test_seat_and_weapon_label(uint32_t unit_index, char *seat_label, char *weapon_label, uint8_t test_only); // 0x5651e0

uint8_t unit_check_weapon_use_permission(uint32_t unit_index, uint32_t weapon_index) // blam-cc: ESI -> unit_index, EDI -> weapon_index
{
    char *seat_name = unit_get_seat_or_state_name(unit_index);
    char *weapon_label = s_no_weapon_label;
    if (weapon_index != 0xffffffff) {
        object *weapon_obj = ((object_header *)object_data->data)[weapon_index & 0xffff].data;
        weapon_label = (char *)(tag_instances[weapon_obj->definition_tag & 0xffff].data) + 0x30c;
    }

    if (unit_set_or_test_seat_and_weapon_label(unit_index, seat_name, weapon_label, 0) == 0) {
        return 0;
    }
    // FIXED (0x56da56..0x56da6b): the game engine's weapon permission callback (+0x60) decides when present;
    //   it is cdecl (unit, weapon) and its AL is the result
    if (current_game_engine != 0 && *(void **)((uint8_t *)current_game_engine + 0x60) != (void *)0) {
        uint8_t (*permission)(uint32_t, uint32_t) =
            *(uint8_t (**)(uint32_t, uint32_t))((uint8_t *)current_game_engine + 0x60);

        return permission(unit_index, weapon_index);
    }
    return 1;
}

#if 0
Original Ghidra decompilation (0x56da00):

undefined1 FUN_0056da00(void)

{
  char cVar1;
  undefined1 uVar2;
  undefined4 uVar3;
  undefined1 *puVar4;
  uint unaff_EDI;

  uVar3 = FUN_0056c2f0();
  puVar4 = &DAT_0065512c;
  if (unaff_EDI != 0xffffffff) {
    puVar4 = (undefined1 *)
             (*(int *)((**(uint **)(*(int *)(DAT_008603b0 + 0x34) + 8 + (unaff_EDI & 0xffff) * 0xc)
                       & 0xffff) * 0x20 + 0x14 + DAT_0087bc14) + 0x30c);
  }
  cVar1 = unit_set_or_test_seat_and_weapon_label(uVar3,puVar4,0);
  uVar2 = 0;
  if (cVar1 != '\0') {
    uVar2 = 1;
    if ((DAT_006f1d20 != 0) && (*(code **)(DAT_006f1d20 + 0x60) != (code *)0x0)) {
      uVar2 = (**(code **)(DAT_006f1d20 + 0x60))();
    }
  }
  return uVar2;
}
#endif
