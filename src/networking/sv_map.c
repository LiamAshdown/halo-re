// sv_map  (Ghidra: sv_map, already named)
// address 0x4e2b20, size 234 bytes
// name confidence: 0.9   rewrite confidence: 0.4
// evidence: out/phase4/networking_functions.md; CEA-pdb match on both literal strings; the
// 0x26-dword (0x98-byte) copy into DAT_00714de0 matches types/game.h's own already-named
// `game_variant_saved_default` global exactly (k_game_variant_size == 0x98).
// register convention: EAX = argument_count, EBX = arguments (uint16_t **, the map/variant name
// pair), matching the EAX/EBX console-command convention sv_kick.c documents.
//   // blam-cc: EAX -> argument_count, EBX -> arguments
// UNSURE: FUN_00463920 (foreign, < this module) is called here with no visible arguments; its
// role is inferred only as "validates the map/variant pair" from the branch it guards.
// UNSURE: network_game_start_new_server_from_profile's own file declares it `void`; called here
// with a local `char`-returning prototype instead, matching this call site's literal use of its
// result, per the established precedent for this category of mismatch.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "fn_game.h"
#include <string.h>

extern int16_t network_game_mode; // 0x00719720, 2 == host, 0 == local/not in a game
extern int32_t game_variant_history_current; // 0x00687b18
extern game_variant game_variant_saved_default; // 0x00714de0
extern uint8_t game_variant_saved_default_valid; // 0x00714e78

extern char game_engine_is_map_and_variant_valid(void); // foreign, validates the requested map/variant pair (UNSURE)


extern void widget_close_all(void); // 0x498650, other module

extern void console_deactivate(void); // 0x4c64b0, other module
extern void main_queue_map_change_by_name_or_clear(void); // 0x4c87a0

extern char network_game_start_new_server_from_profile(uint32_t param_1); // 0x4e40f0, this
    // module, called here with a local char-returning prototype (UNSURE, see header)
extern void *global_white_argb; // 0x006851fc, a ColorARGB * the original loads into EAX
extern void *console_message_default_color; // 0x00685218, a ColorARGB * the original loads into EAX
extern void chimera__console_out(ColorARGB *color, char *format, ...); // 0x496b50, EAX color (NULL = default)

// Console command: validates the requested map/variant, then either restarts the current
// dedicated-server game with the new variant (when hosting) or starts a brand-new dedicated
// server with it (when not yet in a game).
void sv_map(uint32_t argument_count, uint16_t **arguments)
{
    if (argument_count == 0 || arguments == 0 || game_engine_is_map_and_variant_valid() == 0) {
        chimera__console_out((ColorARGB *)global_white_argb, "sv_map specified invalid map or game variant");
        return;
    }

    if (network_game_mode == 2) {
        game_engine_free_custom_variant_cache();
        game_engine_variant_add_to_history(0, 0, 0); // UNSURE: called argument-less in Ghidra's decompile
        game_variant_history_current = -1;
        widget_close_all();
        game_engine_begin_end_game_sequence();
        console_deactivate();
        return;
    }

    if (network_game_mode == 0) {
        game_variant new_variant;

        main_queue_map_change_by_name_or_clear();
        game_engine_get_variant_by_name(0, &new_variant); // UNSURE: called argument-less in Ghidra's decompile
        memcpy(&game_variant_saved_default, &new_variant, sizeof(game_variant));
        game_variant_saved_default_valid = 1;
        if (network_game_start_new_server_from_profile(0) == 0) {
            return;
        }
        console_deactivate();
        return;
    }

    chimera__console_out((ColorARGB *)console_message_default_color, "sv_map is a server-only function!");
}

#if 0
Original Ghidra decompilation (0x4e2b20), from tools/pack.py 0x4e2b20:

void sv_map(void)

{
  char cVar1;
  int in_EAX;
  int iVar2;
  int unaff_EBX;
  undefined4 *puVar3;
  undefined4 *puVar4;
  undefined4 local_98 [38];

  if (((in_EAX == 0) || (unaff_EBX == 0)) || (cVar1 = FUN_00463920(), cVar1 == '\0')) {
    chimera__console_out("sv_map specified invalid map or game variant");
    return;
  }
  if (DAT_00719720 == 2) {
    game_engine_free_custom_variant_cache();
    game_engine_variant_add_to_history();
    DAT_00687b18 = 0xffffffff;
    widget_close_all();
    game_engine_begin_end_game_sequence();
    console_deactivate();
    return;
  }
  if (DAT_00719720 == 0) {
    main_queue_map_change_by_name_or_clear();
    game_engine_get_variant_by_name(local_98);
    puVar3 = local_98;
    puVar4 = &DAT_00714de0;
    for (iVar2 = 0x26; iVar2 != 0; iVar2 = iVar2 + -1) {
      *puVar4 = *puVar3;
      puVar3 = puVar3 + 1;
      puVar4 = puVar4 + 1;
    }
    DAT_00714e78 = 1;
    cVar1 = network_game_start_new_server_from_profile(0);
    if (cVar1 == '\0') {
      return;
    }
    console_deactivate();
    return;
  }
  chimera__console_out("sv_map is a server-only function!");
  return;
}
#endif
