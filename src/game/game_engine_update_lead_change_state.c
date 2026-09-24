// game_engine_update_lead_change_state  (Ghidra: FUN_00470810; renamed, no established name)
// address 0x470810, size 508 bytes
// name confidence: 0.25   rewrite confidence: 0.3
// evidence: out/phase4/game_functions.md ("Detects a lead change or close-game condition for a
// given team color, updates the tracked state and HUD indicator, and broadcasts the notification
// to all clients"); types/game.h player (team_index_desired +0x67); the sibling helpers
// player_customization_slot_set.c, player_set_team_by_color.c, game_engine_team_is_leading.c,
// game_engine_team_close_game_check.c (this batch), all of which this function calls; the
// message_delta_encode_message / network_message_scratch convention established in
// game_engine_send_team_allegiance_message.c; game_engine_player_round_reset.c (0x463620,
// already rewritten, EBX -> player_handle).
//
// Fully reconstructed against
//   objdump -d -M intel --start-address=0x470810 --stop-address=0x470a10 bin/halo.exe
// because Ghidra materializes almost every register argument as an untyped local (`local_18` is
// reused across three different meanings) and gets one global wrong outright: the "iVar3+6" bit
// test and both nearby player_customization_slot_set calls actually read `network_client`
// (ds:0x71c2d8), not `network_session` (ds:0x71c2d4) as Ghidra's text claims (confirmed at
// 0x4708c3: `mov edi,[0x71c2d8]`).
// register convention: EAX carries a "message envelope" (`**envelope`), used only for the very
// first validity check; the stack parameter is a second, distinct pointer (`message`) whose +0xc
// field is compared against the per-color network_session table.
//   // blam-cc: EAX -> envelope, stack -> message
// UNSURE: message_delta_decode_compound_field's two output bytes (color, side_selector) and chat_queue_team_message's constant
// 0x91 argument are not attested in any header; kept as raw/UNSURE. The final
// game_engine_player_round_reset() call passes the matched player's handle in EAX where that
// function's own file documents EBX as the input register -- transcribed literally (EBX is
// standard-cdecl callee-saved and was last set to `leading_or_side` several calls earlier, so it
// is very unlikely to actually be the intended argument there); flagged rather than "corrected"
// since round_reset's own register attribution was made from a different call site.
// reconciled: R16 data_iterator is 0x10 bytes (int16 next_index, +0x0c signature = data ^ 'iter'); the inline constructor now stores the signature like the original; the separate write-only iter_signature local is folded into it

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include <stdint.h>

extern game_engine_definition *current_game_engine; // 0x006f1d20
extern uint8_t game_engine_teams_enabled_flag;       // 0x006f1cbc
extern uint8_t *network_session;                     // 0x0071c2d4, pointer variable
extern uint8_t *network_client;                      // 0x0071c2d8, pointer variable
extern data_array *player_data;                      // 0x0087a480
extern uint8_t game_engine_unknown_1cfc;             // 0x006f1cfc, UNSURE raw flag
extern uint8_t network_message_scratch[0x7ff8];      // 0x00871de0

extern void game_engine_player_round_reset(void); // 0x463620; blam-cc: EBX -> player_handle (see header UNSURE)
extern void *data_iterator_next(data_iterator *iterator); // 0x4d05d0, blam-cc: iterator in EDI
extern void chat_queue_team_message(int32_t color, int32_t message_id); // 0x4aade0, not in this batch;
    // blam-cc: ECX -> message_id (always 0x91 at both call sites here), stack -> color
extern uint8_t message_delta_decode_compound_field(void *event, void *out_values); // 0x4ec590; blam-cc:
    // EAX -> event, ECX -> out_values (canonical form; here out_values is the 2-byte pair)
extern void message_delta_decode_compound_field_staged(void *event); // 0x4ec670; blam-cc: EAX -> event
    // (canonical form, per game_engine_handle_kill_feed_network_event.c)
extern uint8_t game_engine_team_close_game_check(int32_t side, int32_t filter_value); // this batch, 0x470790
extern uint8_t game_engine_team_is_leading(int32_t filter_value); // this batch, 0x470720
extern uint8_t player_customization_slot_set(uint8_t *base, uint8_t new_value, int8_t key); // this batch, 0x4705f0
extern void player_set_team_by_color(uint8_t new_team, int8_t target_team_index_desired); // this batch, 0x470630
extern int32_t message_delta_encode_message(uint32_t unknown_0, uint32_t message_type,
    uint32_t unknown_2, void **fields, uint32_t unknown_4, uint32_t unknown_5,
    uint8_t unknown_6); // 0x4ec940
extern void network_session_broadcast_to_flagged(uint32_t unknown_0, void *unknown_1, uint32_t unknown_2,
    uint32_t unknown_3, uint32_t unknown_4, uint32_t unknown_5); // 0x4e1a80, not in this module

// blam-cc: EAX -> envelope, stack -> message
void game_engine_update_lead_change_state(void **envelope, uint8_t *message)
{
    uint8_t out_pair[2] = { 0xff, 0xff };
    uint8_t color, side_selector;

    if (*(int32_t *)*envelope != 0 || current_game_engine == 0 || !game_engine_teams_enabled_flag) {
        message_delta_decode_compound_field_staged(envelope);
        return;
    }

    if (!message_delta_decode_compound_field(envelope, out_pair)) {
        return;
    }
    color = out_pair[0];
    side_selector = out_pair[1];

    if (color > 0xf ||
        (int16_t)*(int8_t *)(network_session + (uint32_t)color * 0x20 + 0x1c6) != *(int16_t *)(message + 0xc)) {
        chat_queue_team_message(color, 0x91);
        return;
    }

    {
        int32_t leading_or_side;

        if (side_selector == 0 || side_selector == 1) {
            if (game_engine_unknown_1cfc != 0 && !game_engine_team_close_game_check(color, 0)) {
                chat_queue_team_message(color, 0x91);
                return;
            }
            leading_or_side = side_selector;
        } else {
            leading_or_side = game_engine_team_is_leading(color) & 0xff;
        }

        if (leading_or_side == -1) {
            return;
        }

        if (!player_customization_slot_set(network_client + 8, (uint8_t)leading_or_side, (int8_t)color)) {
            return;
        }

        if ((network_client[6] >> 2 & 1) == 0) {
            player_customization_slot_set(network_client + 0xb14, (uint8_t)leading_or_side, (int8_t)color);
        }
        player_set_team_by_color((uint8_t)leading_or_side, (int8_t)color);

        {
            data_iterator player_iter;
            void *player_element;

            player_iter.data = player_data;
            player_iter.next_index = 0;
            player_iter.index = k_datum_index_none;
            player_iter.signature = (uint32_t)(uintptr_t)player_iter.data ^ k_data_iterator_signature;
            player_element = data_iterator_next(&player_iter);
            while (player_element != 0) {
                if (((player *)player_element)->team_index_desired == (int8_t)color) {
                    game_engine_player_round_reset(); // UNSURE: see header -- the matched player
                        // handle (player_iter.index) is loaded into EAX right before this call,
                        // not EBX, contradicting that function's own documented register.
                    break;
                }
                player_element = data_iterator_next(&player_iter);
            }
        }

        {
            uint8_t local_team_byte = color;
            uint8_t *fields_ptr = &local_team_byte;

            message_delta_encode_message(0, 0x1a, 0, (void **)&fields_ptr, 0, 1, 0);
        }
        network_session_broadcast_to_flagged(1, network_message_scratch, 1, 0, 1, 3);
    }
}

#if 0
Original Ghidra decompilation (0x470810), from tools/pack.py 0x470810 -- see the header comment
for the confirmed network_session/network_client mixup and the elided-register arguments.

void FUN_00470810(int param_1)

{
  char cVar1;
  undefined4 *in_EAX;
  uint uVar2;
  int iVar3;
  byte local_1c;
  byte local_1b;
  byte *local_18;
  undefined4 local_14;
  uint local_10;
  undefined2 local_c;
  undefined4 local_8;
  uint local_4;

  if (((*(int *)*in_EAX != 0) || (DAT_006f1d20 == 0)) || (DAT_006f1cbc == '\0')) {
    FUN_004ec670();
    return;
  }
  local_1c = 0xff;
  local_1b = 0xff;
  cVar1 = FUN_004ec590();
  if (cVar1 != '\0') {
    local_18 = (byte *)(uint)local_1b;
    if ((0xf < local_1c) ||
       (uVar2 = (uint)local_1c,
       (short)*(char *)(uVar2 * 0x20 + 0x1c6 + DAT_0071c2d4) != *(short *)(param_1 + 0xc))) {
      FUN_004aade0(local_1c);
      return;
    }
    if ((local_18 == (byte *)0x0) || (local_18 == (byte *)0x1)) {
      if ((DAT_006f1cfc != '\0') && (cVar1 = FUN_00470790(uVar2), cVar1 == '\0')) {
        FUN_004aade0(local_1c);
        return;
      }
    }
    else {
      uVar2 = FUN_00470720(uVar2);
      local_18 = (byte *)(uVar2 & 0xff);
    }
    iVar3 = DAT_0071c2d4;
    if ((local_18 != (byte *)0xffffffff) && (cVar1 = FUN_004705f0(), cVar1 != '\0')) {
      if ((*(byte *)(iVar3 + 6) >> 2 & 1) == 0) {
        FUN_004705f0();
      }
      FUN_00470630();
      local_10 = DAT_0087a480;
      local_4 = DAT_0087a480 ^ 0x69746572;
      local_c = 0;
      local_8 = 0xffffffff;
      iVar3 = data_iterator_next();
      if (iVar3 != 0) {
        uVar2 = (uint)local_1c;
        do {
          if ((int)*(char *)(iVar3 + 0x67) == uVar2) {
            FUN_00463620(local_18);
            break;
          }
          iVar3 = data_iterator_next();
        } while (iVar3 != 0);
      }
      local_18 = &local_1c;
      local_14 = 0;
      message_delta_encode_message(0,0x1a,0,&local_18,0,1,'\0');
      FUN_004e1a80(1,&DAT_00871de0,1,0,1,3);
    }
  }
  return;
}

Corrected control flow and register bindings, from objdump -d -M intel
--start-address=0x470810 --stop-address=0x470a10 bin/halo.exe:

  4708a1: push eax (=color) ; call 0x470720           ; game_engine_team_is_leading(color)
  4708c3: mov edi,[0x71c2d8]                            ; edi = network_client (NOT 0x71c2d4)
  4708cc: lea ecx,[edi+0x8] ; esi=color ; call 0x4705f0  ; player_customization_slot_set(client+8, ebx=leading_or_side, color)
  4708dc: mov al,[edi+6] ; shr al,2 ; test al,1 ; jne 0x470900
  4708eb: mov ecx,[0x71c2d8] ; add ecx,0xb14 ; call 0x4705f0  ; ...(client+0xb14, leading_or_side, color)
  470909: call 0x470630                                   ; player_set_team_by_color(leading_or_side, color)
  4709a7: mov edx,[esp+0x10] ; mov eax,[esp+0x20] ; push edx ; call 0x463620  ; game_engine_player_round_reset()
#endif
