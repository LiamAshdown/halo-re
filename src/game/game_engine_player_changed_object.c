// game_engine_player_changed_object  (Ghidra: FUN_0045c570)
// address 0x45c570, size 286 bytes
// name confidence: 0.55 (still FUN_0045c570 in Ghidra; types/game.h's game_engine_definition
//   comment cites this exact address as the +0x18 "player_changed_object" vtable slot)
// rewrite confidence: 0.3
// evidence: types/game.h current_game_engine (0x006f1d20, player_changed_object at +0x18).
// register convention: __cdecl; the recognized stack parameter is forwarded verbatim to the
//   vtable slot without ever being read here.
//
// Ghidra reported eleven "Removing unreachable block" warnings and an empty loop body because it
// lost the stack data_iterator (passed in EDI). The loop body is recovered from objdump
// 0x45c570..0x45c68e: the inline data_iterator over player_data (0x45c577..0x45c5a4), then for
// every player: validate iterator.index against player_data (0x45c5c0..0x45c605, the same
// range/occupied/salt test chimera__kill_feed does), skip non-local players (WORD +0x02 == -1,
// 0x45c607), build message type 0x1c about `param` into a 0x400-wchar buffer through the
// variant's build_message_text (+0x6c, 0x45c60e) or game_engine_build_kill_feed_message_text
// (0x45e680, EAX = player, EBX = buffer), terminate it (0x45c650) and hand it to
// chimera__multiplayer_message (0x4ab4b0). This is chimera__kill_feed's local-player path with
// message type 0x1c inlined. Afterwards the +0x18 player_changed_object slot gets `param`.
// UNSURE: the build_message_text override signature (as in chimera__kill_feed.c).
// reconciled: R16 the elided iterator is the inline 0x10-byte data_iterator over player_data (0x45c577); with it the loop body Ghidra dropped is recovered from the disassembly

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include <stdint.h>
#include <wchar.h>

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern game_engine_definition *current_game_engine; // 0x006f1d20

extern data_array *player_data; // 0x0087a480

extern void *data_iterator_next(data_iterator *iterator); // 0x4d05d0, memory module; blam-cc: EDI -> iterator
extern uint8_t game_engine_build_kill_feed_message_text(datum_index recipient, wchar_t *out, uint32_t message_type,
    datum_index subject, size_t buffer_size); // 0x45e680, blam-cc: EAX -> recipient, EBX -> out
extern void chimera__multiplayer_message(wchar_t *text); // 0x4ab4b0

// Posts the "player changed object" (type 0x1c) message about `param` to every local player's
// chat line, then forwards `param` to the active game engine's player_changed_object slot.
void game_engine_player_changed_object(uint32_t param)
{
    data_iterator iterator;
    player *p;

    iterator.data = player_data;
    iterator.next_index = 0;
    iterator.index = k_datum_index_none;
    iterator.signature = (uint32_t)(uintptr_t)iterator.data ^ k_data_iterator_signature;
    p = (player *)data_iterator_next(&iterator);
    while (p != (player *)0) {
        datum_index recipient = iterator.index;
        int16_t index = (int16_t)recipient;

        if (recipient != k_datum_index_none && index >= 0 && index < player_data->maximum_count) {
            player *r = (player *)((uint8_t *)player_data->data + player_data->size * index);
            int16_t salt = (int16_t)((uint32_t)recipient >> 16);

            if (r->identifier != 0 && (salt == 0 || r->identifier == salt) &&
                r->local_player_index != -1) {
                wchar_t message[0x400];
                char built = 0;

                if (current_game_engine->build_message_text != 0) {
                    built = ((char (*)(datum_index, uint32_t, uint32_t, wchar_t *, size_t))
                        current_game_engine->build_message_text)
                        (recipient, 0x1c, param, message, 0x400); // UNSURE: override signature
                }
                if (built == 0) {
                    built = game_engine_build_kill_feed_message_text(recipient, message, 0x1c, param, 0x400);
                }
                if (built != 0) {
                    message[0x3ff] = 0;
                    chimera__multiplayer_message(message);
                }
            }
        }
        p = (player *)data_iterator_next(&iterator);
    }

    if (current_game_engine->player_changed_object != (void *)0) {
        ((void (*)(uint32_t))current_game_engine->player_changed_object)(param);
    }
}

#if 0
Original Ghidra decompilation (0x45c570), from tools/pack.py 0x45c570:

/* WARNING: Removing unreachable block (ram,0x0045c5cd) */
/* WARNING: Removing unreachable block (ram,0x0045c5de) */
/* WARNING: Removing unreachable block (ram,0x0045c5e8) */
/* WARNING: Removing unreachable block (ram,0x0045c5fd) */
/* WARNING: Removing unreachable block (ram,0x0045c602) */
/* WARNING: Removing unreachable block (ram,0x0045c607) */
/* WARNING: Removing unreachable block (ram,0x0045c60e) */
/* WARNING: Removing unreachable block (ram,0x0045c61a) */
/* WARNING: Removing unreachable block (ram,0x0045c631) */
/* WARNING: Removing unreachable block (ram,0x0045c64b) */
/* WARNING: Removing unreachable block (ram,0x0045c662) */

void FUN_0045c570(undefined4 param_1)

{
  int iVar1;

  iVar1 = data_iterator_next();
  while (iVar1 != 0) {
    iVar1 = data_iterator_next();
  }
  if (*(code **)(DAT_006f1d20 + 0x18) != (code *)0x0) {
    (**(code **)(DAT_006f1d20 + 0x18))(param_1);
  }
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
