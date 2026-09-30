// network_channel_queue_message  (Ghidra: FUN_004dce40; named per this rewrite)
// address 0x4dce40, size 194 bytes
// name confidence: 0.35   rewrite confidence: 0.3
// evidence: out/phase4/networking_functions.md: "Queues an outgoing message either for
// immediate transmission or, when not marked immediate, into the reliable retransmission buffer
// pool, depending on a caller-supplied mode flag." Matches: when immediate (param_4 == 1) and
// there is room, writes header then body bits directly into a channel bit_stream via
// bit_stream_write_bits_chunked and marks it non-empty; otherwise (or if there is no room even
// after one flush attempt via network_channel_stream_flush) falls back to
// network_channel_reliable_pool_store (network_channel_reliable_pool_store).
// UNSURE (significant): the free-space formula this function computes
// (last_bit - byte_cursor*8 - bit_cursor + 1) reads channel+0x1c/+0x20/+0x24, which land on
// types/networking.h's network_channel.in (the struct's own comment calls `in` "the
// receive-side stream" and `out`, at +0x544, "the send-side stream, drained by 0x4dd730"). But
// network_channel_transmit's own decompile (0x4dd730) drains channel->incoming (+0x00c) and
// channel->endpoint, never touching +0x544 at all -- so the header's out/in direction labels are
// not confirmed by that function either. This rewrite follows the concrete offsets (channel->outgoing)
// rather than the header's descriptive comment, which is the more likely source of error.
// UNSURE: param_1/param_2 are declared but never read anywhere in Ghidra's own decompile; this
// rewrite assumes they are the header/body bit VALUES that bit_stream_write_bits_chunked needs
// (register-forwarded past Ghidra's dead-parameter analysis, the same class of issue documented
// throughout this batch) rather than genuinely unused, since no other source for those two
// values exists in this function's visible body.
// register/parameter convention: EBX -> body_bit_count, EDI -> channel (both elided). blam-cc:
// EBX -> body_bit_count, stack -> header_value, body_value, header_bit_count, immediate, flush_after

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "fn_memory.h"
#include "fn_networking.h"


extern void network_channel_reliable_pool_store(network_channel *channel, uint8_t *body_data,
    uint8_t *header_data, int32_t priority, uint32_t header_bits, uint32_t body_bits); // 0x4dcdb0, this batch


// blam-cc: EBX -> body_bit_count, EDI -> channel
char network_channel_queue_message(network_channel *channel, uint32_t header_value, uint32_t body_value,
    int32_t header_bit_count, char immediate, char flush_after, int32_t body_bit_count)
{
    char result;
    int32_t free_bits;

    if (channel->flags & k_network_channel_listening) {
        return 1;
    }
    result = 1;
    if (immediate == 1) {
        free_bits = (channel->outgoing.stream.last_bit - channel->outgoing.stream.byte_cursor * 8) -
                    channel->outgoing.stream.bit_cursor + 1;
        if (free_bits < body_bit_count + header_bit_count) {
            result = network_channel_stream_flush(&channel->outgoing, channel, 1); // FIXED: 0x4dce78 pushes 1
            if (result == 0) {
                return 0;
            }
        }
        channel->send_budget = channel->send_budget + body_bit_count + header_bit_count; // UNSURE: see header
        bit_stream_write_bits_chunked(&channel->outgoing.stream, (const uint32_t *)header_value, header_bit_count); // 0x4dce9c: ECX = the header bits
        channel->outgoing.empty = 0;
        bit_stream_write_bits_chunked(&channel->outgoing.stream, (const uint32_t *)body_value, body_bit_count);
        channel->outgoing.empty = 0;
        if (flush_after != 1) {
            return result;
        }
        result = network_channel_stream_flush(&channel->outgoing, channel, 1); // FIXED: 0x4dcec7 pushes 1
        return result;
    }
    network_channel_reliable_pool_store(channel, (uint8_t *)&body_value, (uint8_t *)&header_value,
        0, header_bit_count, body_bit_count); // UNSURE: priority elided
    return 1;
}

#if 0
Original Ghidra decompilation (0x4dce40):

char FUN_004dce40(undefined4 param_1,undefined4 param_2,int param_3,char param_4,char param_5)

{
  char extraout_AL;
  char cVar1;
  int unaff_EBX;
  int unaff_EDI;

  if ((*(byte *)(unaff_EDI + 0xa8c) & 1) != 0) {
    return '\x01';
  }
  cVar1 = '\x01';
  if (param_4 == '\x01') {
    if ((((*(int *)(unaff_EDI + 0x24) + *(int *)(unaff_EDI + 0x1c) * -8) -
         *(int *)(unaff_EDI + 0x20)) + 1 < unaff_EBX + param_3) &&
       (FUN_004ddb60(), cVar1 = extraout_AL, extraout_AL == '\0')) {
      return '\0';
    }
    *(int *)(unaff_EDI + 0xa80) = *(int *)(unaff_EDI + 0xa80) + unaff_EBX + param_3;
    bit_stream_write_bits_chunked(param_3);
    *(undefined1 *)(unaff_EDI + 0x2c) = 0;
    bit_stream_write_bits_chunked();
    *(undefined1 *)(unaff_EDI + 0x2c) = 0;
    if (param_5 != '\x01') {
      return cVar1;
    }
    cVar1 = FUN_004ddb60();
    return cVar1;
  }
  FUN_004dcdb0();
  return '\x01';
}
#endif
