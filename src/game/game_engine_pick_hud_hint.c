// game_engine_pick_hud_hint  (Ghidra: FUN_00463150; renamed per its summary)
// address 0x463150, size 320 bytes
// name confidence: 0.35   rewrite confidence: 0.2
// evidence: out/phase4/game_functions.md ("Determines and dispatches which contextual HUD hint
// (for example leader, score-limit reached, eliminated) should currently be shown to a player");
// types/game.h player::unknown_74 (+0x74, "0x45c440 writes -1"), player::marked_for_deletion
// (+0xd5), player::unit (+0x34), player::respawn_timer (+0x2c), game_time_globals::game_time
// (+0x0c), game_variant::game_engine_index (+0x30, aliased 0x006f1cb8), game_variant::
// ctf_value_80 (+0x80, aliased 0x006f1d08); this batch's game_engine_player_is_eliminated
// (0x460f30), game_engine_player_has_respawn_priority (0x460e40) and
// game_engine_build_message_text (0x460890).
// register convention: ECX -> player_index, EAX -> maximum_length (0x400 from hud_update_interaction_prompt, forwarded as
//   the builders' buffer_size), stack -> out_text. (Ghidra took the EAX value for the output buffer; the buffer is the
//   one stack argument.)
//   // blam-cc: ECX -> player_index, EAX -> maximum_length, stack -> out_text
// VERIFIED against disassembly 0x463150..0x46328f (2026-09-30). Fixed: the +0x6c override gets five stack args
//   (player, type, extra, out, size); 0x45e680 gets EAX = the player, EBX = out and stack (type, extra, size); the
//   0x460890 calls get EAX = out, ESI = size, EDI = subject (-1, or player.hud_message_player) and stack (player, type).

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include <wchar.h>

extern game_engine_definition *current_game_engine; // 0x006f1d20
extern data_array *player_data;                     // 0x0087a480
extern game_time_globals *game_time;                 // 0x006f1d6c
extern game_variant game_engine_variant;            // 0x006f1c88 (game_engine_index aliased
                                                     // 0x006f1cb8, ctf_value_80 aliased 0x006f1d08)

extern uint8_t game_engine_player_is_eliminated(uint32_t player_index); // 0x460f30, this batch
extern uint8_t game_engine_player_has_respawn_priority(uint32_t player_index); // 0x460e40, this batch
// blam-cc: EAX -> out, ESI -> buffer_size, EDI -> subject, stack -> param_1, message_type
extern uint8_t game_engine_build_message_text(wchar_t *out, uint32_t buffer_size, datum_index subject,
    uint32_t param_1, uint32_t message_type); // 0x460890, this batch
// blam-cc: EAX -> recipient, EBX -> out, stack -> message_type, subject, buffer_size
extern uint8_t game_engine_build_kill_feed_message_text(datum_index recipient, wchar_t *out, uint32_t message_type,
    datum_index subject, size_t buffer_size); // 0x45e680

// blam-cc: ECX -> player_index, EAX -> maximum_length, stack -> out_text
uint8_t game_engine_pick_hud_hint(datum_index player_index, int32_t maximum_length, uint16_t *out_text)
{
    wchar_t *out = (wchar_t *)out_text;
    uint32_t buffer_size = (uint32_t)maximum_length;
    player *p = (player *)((uint8_t *)player_data->data + (player_index & 0xffff) * sizeof(player));

    if (current_game_engine == 0) {
        return 0;
    }

    if (0x16 < (int32_t)p->hud_message_index && (int32_t)p->hud_message_index < 0x1b) {
        p->hud_message_index = (datum_index)0xffffffff;
    }

    if (p->unit == (datum_index)0xffffffff) {
        uint32_t message_type;
        int32_t extra = 0;

        if (p->marked_for_deletion == 1) {
            message_type = 0x1b;
        } else if (game_engine_player_is_eliminated(player_index) != 0) {
            message_type = 0x18;
        } else if (game_engine_player_has_respawn_priority(player_index) != 0) {
            message_type = 0x17;
        } else if (p->respawn_timer < 1) {
            message_type = 0x1a;
        } else {
            extra = p->respawn_timer / 30;
            message_type = 0x19;
        }

        if (current_game_engine->build_message_text != 0) {
            // 0x463207..0x463215: push size, out, extra, type, player
            char handled = ((char (*)(datum_index, uint32_t, int32_t, wchar_t *, uint32_t))
                current_game_engine->build_message_text)(player_index, message_type, extra, out, buffer_size);
            if (handled != 0) {
                return (uint8_t)handled;
            }
        }
        return game_engine_build_kill_feed_message_text(player_index, out, message_type, (datum_index)extra, buffer_size);
    } else {
        if (game_time->game_time < 0x1c2) {
            if (p->hud_message_index == (datum_index)0xffffffff ||
                game_engine_variant.game_engine_index != _game_engine_ctf ||
                game_engine_variant.engine.ctf.single_flag_time < 1) {
                // 0x46325b: message type 0x1d, subject -1 (EDI is still the -1 loaded at 0x463182)
                return game_engine_build_message_text(out, buffer_size, (datum_index)0xffffffff, player_index, 0x1d);
            }
        } else if (p->hud_message_index == (datum_index)0xffffffff) {
            return 0;
        }
        // 0x463271/0x463278: message type = the pending hud message, subject = player.hud_message_player (+0x78)
        return game_engine_build_message_text(out, buffer_size, p->hud_message_player, player_index, p->hud_message_index);
    }
}

#if 0
Original Ghidra decompilation (0x463150), from tools/pack.py 0x463150:

uint FUN_00463150(void)

{
  char cVar1;
  uint in_EAX;
  uint uVar2;
  uint in_ECX;
  int iVar3;
  undefined4 uVar4;
  int iVar5;

  uVar2 = in_EAX & 0xffffff00;
  if (DAT_006f1d20 != 0) {
    iVar3 = (in_ECX & 0xffff) * 0x200;
    iVar5 = *(int *)(iVar3 + 0x74 + *(int *)(DAT_0087a480 + 0x34));
    iVar3 = iVar3 + *(int *)(DAT_0087a480 + 0x34);
    if ((0x16 < iVar5) && (iVar5 < 0x1b)) {
      *(undefined4 *)(iVar3 + 0x74) = 0xffffffff;
    }
    if (*(int *)(iVar3 + 0x34) == -1) {
      iVar5 = 0;
      if (*(char *)(iVar3 + 0xd5) == '\x01') {
        uVar4 = 0x1b;
      }
      else {
        cVar1 = FUN_00460f30();
        if (cVar1 == '\0') {
          cVar1 = FUN_00460e40();
          if (cVar1 == '\0') {
            if (*(int *)(iVar3 + 0x2c) < 1) {
              uVar4 = 0x1a;
            }
            else {
              iVar5 = *(int *)(iVar3 + 0x2c) / 0x1e;
              uVar4 = 0x19;
            }
          }
          else {
            uVar4 = 0x17;
          }
        }
        else {
          uVar4 = 0x18;
        }
      }
      if ((*(code **)(DAT_006f1d20 + 0x6c) == (code *)0x0) ||
         (uVar2 = (**(code **)(DAT_006f1d20 + 0x6c))(), (char)uVar2 == '\0')) {
        uVar2 = game_engine_build_kill_feed_message_text(uVar4,iVar5);
        return uVar2;
      }
    }
    else {
      if (*(int *)(DAT_006f1d6c + 0xc) < 0x1c2) {
        if (((*(int *)(iVar3 + 0x74) == -1) || (DAT_006f1cb8 != 1)) || (DAT_006f1d08 < 1)) {
          uVar2 = FUN_00460890();
          return uVar2;
        }
      }
      else if (*(int *)(iVar3 + 0x74) == -1) {
        return uVar2;
      }
      uVar2 = FUN_00460890();
    }
  }
  return uVar2;
}
#endif
