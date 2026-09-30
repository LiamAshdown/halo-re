// control_profile_copy_gamepad_bindings_by_key  (Ghidra: FUN_0053b700, renamed)
// address 0x53b700, size 227 bytes
// name confidence: 0.5   rewrite confidence: 0.75
// evidence: out/phase4/saved_games_types_notes.md 0x53b700 summary "Looks up matching profile
// slots in two profile tables and, if valid, copies that slot's per-controller data blocks from
// one table to the other." Confirmed against objdump 0x53b700..0x53b7e2: the incoming EAX
// register is passed unchanged as EBX (the key argument) to both
// control_profile_gamepad_slot_find calls, once with EDX = the first stack argument (dest) and
// once with EDX = the second stack argument (source); the copy loops match
// gamepad_button_bindings (0x22a, 0x10 dwords), gamepad_action_buttons (0x32a, 1 dword),
// gamepad_axis_bindings (0x33a, 0x20 dwords), gamepad_pov_bindings (0x53a, 0x40 dwords) and the
// two per-gamepad rate bytes (0x956, 0x95a) -- exactly the controls_gamepad_record-indexed
// fields of saved_player_profile. control_profile_is_customized is called with EDI = source base
// and EBX = the source slot index, gating the whole copy on the source slot being customized.
// register convention: key record in EAX; dest profile as the first stack argument, source
// profile as the second (confirmed by objdump: the first stack slot feeds the EDX of the first
// control_profile_gamepad_slot_find call and every destination-side array access, the second
// feeds the EDX of the second call, the control_profile_is_customized EDI argument and every
// source-side array access).

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"
#include "saved_games.h"
#include "fn_saved_games.h"


extern void *memcpy(void *dest, const void *src, uint32_t count); // CRT

// blam-cc: key record in EAX; dest profile and source profile as ordinary stack arguments
// (dest first, source second)
// Finds the gamepad slot matching key in both dest and source. If both are found and the
// source slot has customized bindings, copies that slot's button, action-button, axis and pov
// bindings plus its two rate bytes from source to dest. Returns 1 if the copy was made, 0
// otherwise.
// FIXED (objdump): every ret sets only AL; the upper bits of EAX are left as they were
uint8_t control_profile_copy_gamepad_bindings_by_key(controls_gamepad_record *key,
    saved_player_profile *dest, saved_player_profile *source)
{
    int32_t dest_slot;
    int32_t source_slot;

    dest_slot = control_profile_gamepad_slot_find(dest, key);
    source_slot = control_profile_gamepad_slot_find(source, key);
    if (dest_slot != -1 && source_slot != -1) {
        if (control_profile_is_customized(source, source_slot)) {
            memcpy(dest->gamepad_button_bindings[dest_slot], source->gamepad_button_bindings[source_slot],
                   sizeof(dest->gamepad_button_bindings[dest_slot]));
            memcpy(dest->gamepad_action_buttons[dest_slot], source->gamepad_action_buttons[source_slot],
                   sizeof(dest->gamepad_action_buttons[dest_slot]));
            memcpy(dest->gamepad_axis_bindings[dest_slot], source->gamepad_axis_bindings[source_slot],
                   sizeof(dest->gamepad_axis_bindings[dest_slot]));
            memcpy(dest->gamepad_pov_bindings[dest_slot], source->gamepad_pov_bindings[source_slot],
                   sizeof(dest->gamepad_pov_bindings[dest_slot]));
            dest->gamepad_rate_a[dest_slot] = source->gamepad_rate_a[source_slot];
            dest->gamepad_rate_b[dest_slot] = source->gamepad_rate_b[source_slot];
            return 1;
        }
    }
    return 0;
}

#if 0
Original Ghidra decompilation (0x53b700):

undefined4 FUN_0053b700(int param_1,int param_2)

{
  char cVar1;
  int iVar2;
  int iVar3;
  int iVar4;
  undefined4 *puVar5;
  undefined4 *puVar6;

  iVar2 = FUN_0053b6b0();
  iVar3 = FUN_0053b6b0();
  if ((iVar2 != -1) && (iVar3 != -1)) {
    cVar1 = control_profile_is_customized();
    if (cVar1 != '\0') {
      puVar5 = (undefined4 *)(iVar3 * 0x40 + 0x22a + param_2);
      puVar6 = (undefined4 *)(iVar2 * 0x40 + 0x22a + param_1);
      for (iVar4 = 0x10; iVar4 != 0; iVar4 = iVar4 + -1) {
        *puVar6 = *puVar5;
        puVar5 = puVar5 + 1;
        puVar6 = puVar6 + 1;
      }
      *(undefined4 *)(param_1 + 0x32a + iVar2 * 4) = *(undefined4 *)(param_2 + 0x32a + iVar3 * 4);
      puVar5 = (undefined4 *)(iVar3 * 0x80 + 0x33a + param_2);
      puVar6 = (undefined4 *)(iVar2 * 0x80 + 0x33a + param_1);
      for (iVar4 = 0x20; iVar4 != 0; iVar4 = iVar4 + -1) {
        *puVar6 = *puVar5;
        puVar5 = puVar5 + 1;
        puVar6 = puVar6 + 1;
      }
      puVar5 = (undefined4 *)(iVar3 * 0x100 + 0x53a + param_2);
      puVar6 = (undefined4 *)(iVar2 * 0x100 + 0x53a + param_1);
      for (iVar4 = 0x40; iVar4 != 0; iVar4 = iVar4 + -1) {
        *puVar6 = *puVar5;
        puVar5 = puVar5 + 1;
        puVar6 = puVar6 + 1;
      }
      *(undefined1 *)(param_1 + 0x956 + iVar2) = *(undefined1 *)(iVar3 + 0x956 + param_2);
      *(undefined1 *)(param_1 + 0x95a + iVar2) = *(undefined1 *)(iVar3 + 0x95a + param_2);
      return 1;
    }
  }
  return 0;
}
#endif
