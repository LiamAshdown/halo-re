// game_engine_send_team_allegiance_message  (Ghidra: FUN_004704d0; renamed, no established name)
// address 0x4704d0, size 285 bytes
// name confidence: 0.25   rewrite confidence: 0.45
// evidence: types/game.h player (local_player_index +0x02, team_index_desired +0x67);
// src/game/game_engine_notify_kill_event.c for the established message_delta_encode_message /
// network_message_scratch (0x00871de0) convention, confirmed identical here by objdump
// (--start-address=0x4704d0 --stop-address=0x4705f0): `mov edx,0x7ff8 ; mov eax,0x871de0` right
// before the call, matching that file's "EAX -> destination buffer, EDX -> destination size"
// note; the &local_18-holds-&local_1c indirection Ghidra already renders is confirmed correct.
// register convention: single stack byte parameter (`mov al,[esp+0x30]` against the caller's
// pushed byte, before any of this function's own pushes).
//   // blam-cc: stack -> broadcast
// UNSURE: name and purpose are inferred from field access only (out/phase4/game_functions.md's
// own guess, "formats an announcer message... into the game's film/replay buffer", does not match
// what the disassembly actually does: it sends network event 0x1a carrying the first local
// player's team_index_desired byte, or 0xff if there is no local player).

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include <stdint.h>

extern game_engine_definition *current_game_engine; // 0x006f1d20
extern uint8_t game_engine_teams_enabled_flag;       // 0x006f1cbc; UNSURE raw name, see
                                                      // game_engine_get_teams_enabled (variant+0x34 alias)
extern data_array *player_data;                      // 0x0087a480
extern uint8_t network_session_ptr[];                // 0x0071c2d8, UNSURE: raw pointer, see below
extern uint8_t network_message_scratch[0x7ff8];      // 0x00871de0

extern void *data_iterator_next(data_iterator *iterator); // 0x4d05d0, blam-cc: iterator in EDI
extern int32_t message_delta_encode_message(uint32_t unknown_0, uint32_t message_type,
    uint32_t unknown_2, void **fields, uint32_t unknown_4, uint32_t unknown_5,
    uint8_t unknown_6); // 0x4ec940; blam-cc: EAX -> network_message_scratch, EDX -> 0x7ff8, then
    // the seven stack arguments (see game_engine_notify_kill_event.c)
extern char network_channel_stream_flush(uint8_t *session, int32_t unknown); // 0x4ddb60, not in this batch
extern int32_t bit_stream_write_bits_chunked(int32_t total_bit_count, uint32_t value,
    bit_stream *stream); // 0x4cf8f0, blam-cc: value in EDX, stream in ESI

// blam-cc: stack -> broadcast
// While a multiplayer engine is loaded and teams are enabled, encodes network event 0x1a with
// the first local player's team_index_desired byte (0xff if there is no local player) and
// `broadcast`, then, if there is room in the outgoing packet (or the session can be flushed to
// make room), commits it and writes it out in two chunked bit-stream calls.
void game_engine_send_team_allegiance_message(char broadcast)
{
    data_iterator player_iter;
    void *player_element;
    uint8_t team_index_desired = 0xff;
    int32_t iter_signature; // UNSURE: write-only "iter" scratch value, see
                             // game_engine_player_select_random_target.c; never read back.
    uint8_t local_team_byte;      // Ghidra's local_1c: the field message_delta_encode_message reads
    uint8_t local_broadcast_byte; // Ghidra's local_1b, immediately after it in memory
    uint8_t *fields_ptr;          // Ghidra's local_18 = &local_team_byte
    int32_t fields_pad;           // Ghidra's local_14 = 0
    int32_t encoded_bits;

    if (current_game_engine == 0 || !game_engine_teams_enabled_flag) {
        return;
    }

    iter_signature = (int32_t)(intptr_t)player_data ^ 0x69746572;
    player_iter.data = player_data;
    player_iter.next_index = 0;
    player_iter.index = k_datum_index_none;
    player_element = data_iterator_next(&player_iter);
    while (player_element != 0) {
        if (((player *)player_element)->local_player_index != -1) {
            team_index_desired = (uint8_t)((player *)player_element)->team_index_desired;
            break;
        }
        player_element = data_iterator_next(&player_iter);
    }
    (void)iter_signature;

    local_broadcast_byte = (uint8_t)broadcast;
    fields_ptr = &local_team_byte;
    fields_pad = 0;
    local_team_byte = team_index_desired;
    (void)local_broadcast_byte;
    (void)fields_pad;

    encoded_bits = message_delta_encode_message(0, 0x1a, 0, (void **)&fields_ptr, 0, 1, 0);
    if (encoded_bits > 0) {
        uint8_t *session = *(uint8_t **)(network_session_ptr + 0xadc);

        if ((*(uint8_t *)(session + 0xa8c) & 1) == 0 &&
            (encoded_bits + 1 <= (*(int32_t *)(session + 0x24) -
                *(int32_t *)(session + 0x1c) * 8 - *(int32_t *)(session + 0x20)) + 1 ||
             network_channel_stream_flush(session, 1) != 0)) {
            // UNSURE: bit_stream_write_bits_chunked's value/stream registers (EDX/ESI, per its own
            // established signature) are not reloaded anywhere in this function's own body
            // between the two calls or before them -- they must already be live from inside
            // message_delta_encode_message's own return sequence. Not independently recoverable
            // from this pack; left as unaff_-style uninitialized locals rather than invented.
            uint32_t unaff_write_value;
            bit_stream *unaff_write_stream;

            *(int32_t *)(session + 0xa80) = *(int32_t *)(session + 0xa80) + encoded_bits + 1;
            bit_stream_write_bits_chunked(1, unaff_write_value, unaff_write_stream);
            *(uint8_t *)(session + 0x2c) = 0;
            bit_stream_write_bits_chunked(encoded_bits, unaff_write_value, unaff_write_stream);
            *(uint8_t *)(session + 0x2c) = 0;
        }
    }
}

#if 0
Original Ghidra decompilation (0x4704d0), from tools/pack.py 0x4704d0:

void FUN_004704d0(undefined1 param_1)

{
  int iVar1;
  char cVar2;
  int iVar3;
  undefined1 uVar4;
  undefined1 local_1c;
  undefined1 local_1b;
  undefined1 *local_18;
  undefined4 local_14;
  uint local_10;
  undefined2 local_c;
  undefined4 local_8;
  uint local_4;

  if ((DAT_006f1d20 != 0) && (DAT_006f1cbc != '\0')) {
    local_10 = DAT_0087a480;
    local_4 = DAT_0087a480 ^ 0x69746572;
    uVar4 = 0xff;
    local_c = 0;
    local_8 = 0xffffffff;
    iVar3 = data_iterator_next();
    while (iVar3 != 0) {
      if (*(short *)(iVar3 + 2) != -1) {
        uVar4 = *(undefined1 *)(iVar3 + 0x67);
        break;
      }
      iVar3 = data_iterator_next();
    }
    local_1b = param_1;
    local_18 = &local_1c;
    local_14 = 0;
    local_1c = uVar4;
    iVar3 = message_delta_encode_message(0,0x1a,0,&local_18,0,1,'\0');
    if (0 < iVar3) {
      iVar1 = *(int *)(DAT_0071c2d8 + 0xadc);
      if (((*(byte *)(iVar1 + 0xa8c) & 1) == 0) &&
         ((iVar3 + 1 <=
           ((*(int *)(iVar1 + 0x24) + *(int *)(iVar1 + 0x1c) * -8) - *(int *)(iVar1 + 0x20)) + 1 ||
          (cVar2 = FUN_004ddb60(iVar1,1), cVar2 != '\0')))) {
        *(int *)(iVar1 + 0xa80) = *(int *)(iVar1 + 0xa80) + iVar3 + 1;
        bit_stream_write_bits_chunked(1);
        *(undefined1 *)(iVar1 + 0x2c) = 0;
        bit_stream_write_bits_chunked(iVar3);
        *(undefined1 *)(iVar1 + 0x2c) = 0;
      }
    }
  }
  return;
}
#endif
