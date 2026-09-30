// game_engine_client_apply_team_assignment  (Ghidra: FUN_00470a10; renamed, no established name)
// address 0x470a10, size 112 bytes
// name confidence: 0.3   rewrite confidence: 0.5
// evidence: types/game.h network_game_mode (0x00719720, "1 client"); the sibling
// game_engine_update_lead_change_state.c (0x470810), which shares the exact same
// message_delta_decode_compound_field(envelope, out_pair) / player_customization_slot_set(network_client+0xb14, ...) /
// player_set_team_by_color(...) sequence for a client-only path.
// register convention: confirmed by objdump (--start-address=0x470a10 --stop-address=0x470a80)
// against Ghidra's `in_EAX`: this function takes only the register envelope, no stack parameter.
//   // blam-cc: EAX -> envelope

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "fn_game.h"
#include <stdint.h>

extern game_engine_definition *current_game_engine; // 0x006f1d20
extern uint8_t game_engine_teams_enabled_flag;       // 0x006f1cbc
extern int16_t network_game_mode;                    // 0x00719720
extern uint8_t *network_client;                      // 0x0071c2d8, pointer variable

extern void message_delta_decode_compound_field_staged(void *event); // 0x4ec670; blam-cc: EAX -> event
    // (canonical form, per game_engine_handle_kill_feed_network_event.c)
extern uint8_t message_delta_decode_compound_field(void *event, void *out_values); // 0x4ec590; blam-cc:
    // EAX -> event, ECX -> out_values (canonical form; here out_values is the 2-byte pair)


// blam-cc: EAX -> envelope
// While a multiplayer engine with teams is loaded, and only while this machine is the network
// client, reads a (color, side) pair and applies it to network_client+0xb14's customization slot
// and to the matching player's team.
void game_engine_client_apply_team_assignment(void **envelope)
{
    uint8_t out_pair[2] = { 0xff, 0xff };

    if (*(int32_t *)*envelope != 0 || current_game_engine == 0 || !game_engine_teams_enabled_flag) {
        message_delta_decode_compound_field_staged(envelope);
        return;
    }
    if (!message_delta_decode_compound_field(envelope, out_pair) || network_game_mode != 1) {
        return;
    }

    player_customization_slot_set(network_client + 0xb14, out_pair[1], (int8_t)out_pair[0]);
    player_set_team_by_color(out_pair[1], (int8_t)out_pair[0]);
}

#if 0
Original Ghidra decompilation (0x470a10), from tools/pack.py 0x470a10:

void FUN_00470a10(void)

{
  char cVar1;
  undefined4 *in_EAX;

  if (((*(int *)*in_EAX == 0) && (DAT_006f1d20 != 0)) && (DAT_006f1cbc != '\0')) {
    cVar1 = FUN_004ec590(0xffff);
    if ((cVar1 != '\0') && (DAT_00719720 == 1)) {
      FUN_004705f0();
      FUN_00470630();
      return;
    }
  }
  else {
    FUN_004ec670();
  }
  return;
}
#endif
