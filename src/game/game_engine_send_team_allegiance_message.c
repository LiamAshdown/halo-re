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
// reconciled: R16 data_iterator is 0x10 bytes (int16 next_index, +0x0c signature = data ^ 'iter'); the inline constructor now stores the signature like the original; the separate write-only iter_signature local is folded into it

// FIXED 2026-09-28 (send-path audit, from the disassembly): network_channel_stream_flush takes the channel's
// stream (ESI, channel +0x10), the channel and the mode (the C passed the channel as the stream). The two
// bit_stream_write_bits_chunked calls write into
// the channel's outgoing bit stream (channel +0x10, EAX): first the 1-bit item flag (1: a game action) from a local, then
// the encoded bits from network_message_scratch 0x871de0; the C passed placeholders or dropped the arguments.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include <stdint.h>
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern game_engine_definition *current_game_engine; // 0x006f1d20
extern uint8_t game_engine_teams_enabled_flag;       // 0x006f1cbc; UNSURE raw name, see
                                                      // game_engine_get_teams_enabled (variant+0x34 alias)
extern data_array *player_data;                      // 0x0087a480
extern uint8_t network_client[];                // 0x0071c2d8, UNSURE: raw pointer, see below
extern uint8_t network_message_scratch[0x7ff8];      // 0x00871de0

extern void *data_iterator_next(data_iterator *iterator); // 0x4d05d0, blam-cc: iterator in EDI
extern uint8_t network_message_scratch[0x7ff8]; // 0x00871de0
extern int32_t message_delta_encode_message(int32_t extra_eax, int32_t extra_edx, int32_t flag, int32_t message_type,
    int32_t changed_offset, void **items, int32_t type_offset, int32_t count, char force_changed); // 0x4ec940, EAX buffer, EDX size
    // the seven stack arguments (see game_engine_notify_kill_event.c)
extern char network_channel_stream_flush(network_channel_stream *stream, network_channel *channel, char mode); // 0x4ddb60, ESI stream (channel +0x10), stack channel, mode
extern int32_t bit_stream_write_bits_chunked(bit_stream *stream, const uint32_t *values, int32_t total_bit_count); // 0x4cf8f0, EAX stream, ECX values, stack bits

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
    struct {
        uint8_t team;             // 0x00 the local player's desired team (0x47055d: BL)
        uint8_t broadcast;        // 0x01 the argument (0x470548: AL)
    } record;                     // one 2-byte record: the encoder reads both from one address
    void *fields_ptr[2];          // [0] = &record, [1] = 0
    int32_t encoded_bits;

    if (current_game_engine == 0 || !game_engine_teams_enabled_flag) {
        return;
    }

    player_iter.data = player_data;
    player_iter.next_index = 0;
    player_iter.index = k_datum_index_none;
    player_iter.signature = (uint32_t)(uintptr_t)player_iter.data ^ k_data_iterator_signature;
    player_element = data_iterator_next(&player_iter);
    while (player_element != 0) {
        if (((player *)player_element)->local_player_index != -1) {
            team_index_desired = (uint8_t)((player *)player_element)->team_index_desired;
            break;
        }
        player_element = data_iterator_next(&player_iter);
    }

    record.broadcast = (uint8_t)broadcast;
    record.team = team_index_desired;
    fields_ptr[0] = &record;
    fields_ptr[1] = 0;

    encoded_bits = message_delta_encode_message((int32_t)network_message_scratch, 0x7ff8, 0, 0x1a, 0, fields_ptr, 0, 1, 0);
    if (encoded_bits > 0) {
        uint8_t *session = *(uint8_t **)(network_client + 0xadc);

        if ((*(uint8_t *)(session + 0xa8c) & 1) == 0 &&
            (encoded_bits + 1 <= (*(int32_t *)(session + 0x24) -
                *(int32_t *)(session + 0x1c) * 8 - *(int32_t *)(session + 0x20)) + 1 ||
             network_channel_stream_flush((network_channel_stream *)((uint8_t *)session + 0x10), (network_channel *)session, 1) != 0)) {
            // UNSURE: bit_stream_write_bits_chunked's value/stream registers (EDX/ESI, per its own
            // established signature) are not reloaded anywhere in this function's own body
            // between the two calls or before them -- they must already be live from inside
            // message_delta_encode_message's own return sequence. Not independently recoverable
            // from this pack; left as unaff_-style uninitialized locals rather than invented.

            *(int32_t *)(session + 0xa80) = *(int32_t *)(session + 0xa80) + encoded_bits + 1;
            { uint32_t item_flag = 1; bit_stream_write_bits_chunked((bit_stream *)((uint8_t *)session + 0x10), &item_flag, 1); }
            *(uint8_t *)(session + 0x2c) = 0;
            bit_stream_write_bits_chunked((bit_stream *)((uint8_t *)session + 0x10), (const uint32_t *)(network_message_scratch), encoded_bits);
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
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
