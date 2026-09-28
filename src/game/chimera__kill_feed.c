// chimera__kill_feed  (Ghidra: chimera__kill_feed, already named)
// address 0x460a30, size 316 bytes
// name confidence: 0.5   rewrite confidence: 0.35
// evidence: out/phase4/game_functions.md ("Central kill-feed entry point: validates the
// recipient, formats the message text, and routes it to the multiplayer-message or HUD-message
// display system"); types/game.h player (identifier +0x00, local_player_index +0x02);
// game_engine_definition::build_message_text (+0x6c); the identical
// "index/salt/local_player_index" recipient validation already committed verbatim (twice) in
// game_engine_on_player_death.c.
// register convention: the recipient handle is unaff_EDI (never loaded from any stack slot in
// this function, and tested first); hash_key..param_4 are this function's own stack parameters.
//   // blam-cc: unaff_EDI -> recipient, stack -> hash_key, message_type, subject, broadcast
// UNSURE: hash_key is passed only to the game-variant override callback (build_message_text) and
// is otherwise unused; its real meaning is not recoverable from this decompilation. The override
// callback's prototype (5 visible arguments here, none anywhere else in this batch) is likewise
// unrecoverable and is modeled literally from what Ghidra shows at this one call site.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include <wchar.h>

extern game_engine_definition *current_game_engine; // 0x006f1d20
extern data_array *player_data;                     // 0x0087a480
extern int16_t network_game_mode;                   // 0x00719720

extern uint8_t game_engine_build_kill_feed_message_text(wchar_t *out, uint32_t message_type,
    datum_index subject, size_t buffer_size); // 0x45e680
extern void game_engine_notify_kill_event(uint32_t player_index, int32_t hash_key, int32_t message_type, datum_index subject); // 0x4608d0, this batch
extern void chimera__multiplayer_message(wchar_t *text); // 0x4ab4b0
extern void chimera__hud_message(int16_t local_player_index, wchar_t *text); // 0x4ae180,
    // blam-cc: EAX -> local_player_index, stack -> text. objdump 0x460b23: `mov ax,[esi+0x2]`
    // loads the recipient's player::local_player_index into EAX immediately before the call.

// blam-cc: unaff_EDI -> recipient, stack -> hash_key, message_type, subject, broadcast
// Validates `recipient` against the live players array (index range, occupied slot, optional
// salt match), and -- only for a recipient that is a local player -- builds the message text
// (variant override first, default builder second) and routes it to either the multiplayer chat
// line or the HUD message line depending on `message_type`. Independently of the local-player
// check, when hosting and `broadcast` is set, forwards the event to other machines via
// FUN_004608d0.
void chimera__kill_feed(datum_index recipient, int32_t hash_key, uint32_t message_type,
    datum_index subject, char broadcast)
{
    int16_t index = (int16_t)recipient;

    if (recipient == (datum_index)0xffffffff || index < 0 || player_data->maximum_count <= index) {
        return;
    }
    {
        player *p = (player *)((uint8_t *)player_data->data + player_data->size * index);
        int16_t salt;

        if (p->identifier == 0) {
            return;
        }
        salt = (int16_t)((uint32_t)recipient >> 16);
        if (salt != 0 && p->identifier != salt) {
            return;
        }

        if (p->local_player_index != -1) {
            wchar_t message[1024]; // matches Ghidra's local_800 [2046 bytes] + uStack_2 terminator
            char built = 0;

            if (current_game_engine->build_message_text != 0) {
                built = ((char (*)(int32_t, uint32_t, datum_index, wchar_t *, size_t))
                    current_game_engine->build_message_text)
                    (hash_key, message_type, subject, message, 0x400); // 0x460aa7..0x460ac2: (hash_key, type, subject, text, 0x400)
            }
            if (built == 0) {
                built = game_engine_build_kill_feed_message_text(message, message_type, subject, 0x400);
            }
            if (built != 0) {
                message[1023] = 0;
                switch (message_type) {
                case 0: case 1: case 2: case 3: case 4: case 5: case 6: case 8:
                case 0x0d: case 0x13: case 0x1b: case 0x1c:
                    chimera__multiplayer_message(message);
                    break;
                default:
                    chimera__hud_message(p->local_player_index, message);
                    break;
                }
            }
        }

        if (network_game_mode == 2 && broadcast == 1) {
            // 0x460b48..0x460b5a: EAX = the recipient (EDI), ECX = hash_key, stack (message_type, subject)
            game_engine_notify_kill_event(recipient, hash_key, (int32_t)message_type, subject);
        }
    }
}

#if 0
Original Ghidra decompilation (0x460a30), from tools/pack.py 0x460a30:

void chimera__kill_feed(undefined4 param_1,undefined4 param_2,undefined4 param_3,char param_4)

{
  bool bVar1;
  char cVar2;
  short sVar3;
  int iVar4;
  short sVar5;
  int unaff_EDI;
  char local_800 [2046];
  undefined2 uStack_2;
  
  if (((unaff_EDI != -1) && (sVar5 = (short)unaff_EDI, -1 < sVar5)) &&
     (sVar5 < *(short *)(DAT_0087a480 + 0x20))) {
    iVar4 = (int)*(short *)(DAT_0087a480 + 0x22) * (int)sVar5;
    sVar5 = *(short *)(iVar4 + *(int *)(DAT_0087a480 + 0x34));
    if ((sVar5 != 0) && ((sVar3 = (short)((uint)unaff_EDI >> 0x10), sVar3 == 0 || (sVar5 == sVar3)))
       ) {
      if ((*(short *)(iVar4 + *(int *)(DAT_0087a480 + 0x34) + 2) != -1) &&
         (((*(code **)(DAT_006f1d20 + 0x6c) != (code *)0x0 &&
           (cVar2 = (**(code **)(DAT_006f1d20 + 0x6c))(param_1,param_2,param_3,local_800,0x400),
           cVar2 != '\0')) ||
          (cVar2 = game_engine_build_kill_feed_message_text(param_2,param_3,0x400), cVar2 != '\0')))
         ) {
        switch(param_2) {
        case 0:
        case 1:
        case 2:
        case 3:
        case 4:
        case 5:
        case 6:
        case 8:
        case 0xd:
        case 0x13:
        case 0x1b:
        case 0x1c:
          bVar1 = true;
          break;
        default:
          bVar1 = false;
        }
        uStack_2 = 0;
        if (bVar1) {
          chimera__multiplayer_message(local_800);
        }
        else {
          chimera__hud_message(local_800);
        }
      }
      if ((DAT_00719720 == 2) && (param_4 == '\x01')) {
        FUN_004608d0(param_2,param_3);
      }
    }
  }
  return;
}
#endif
