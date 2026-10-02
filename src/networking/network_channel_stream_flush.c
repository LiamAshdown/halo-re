// network_channel_stream_flush  (Ghidra: FUN_004ddb60; named per this rewrite)
// address 0x4ddb60, size 318 bytes
// name confidence: 0.4   rewrite confidence: 0.35
// evidence: out/phase4/networking_functions.md: "Register-based (ESI=connection object) helper
// that computes an unsent byte length from a bit-position ring buffer, flushes it through
// gt2Send (a socket/channel send), then advances the connection." The `stream+8/+0xc/+0x10/
// +0x14` fields match bit_stream's first_bit/byte_cursor/bit_cursor/last_bit, and `stream+0x1c`/
// `stream+0x1d` match network_channel_stream's empty flag and inline data buffer exactly, so ESI
// is a network_channel_stream* -- one of channel->in or channel->out depending on the caller
// (unresolvable from this function's own body; see each caller for which one it intends).
// channel->send_budget (+0xa80) and budget_base_tick (+0xa84) match types/networking.h.
// UNSURE: gt2Send's own signature is inferred purely from this call site: (socket,
// buffer, byte_count, mode). `local_c[2]`/the final `0 < local_c[2]` return is preserved as a
// success flag set only when gt2Send reports a positive byte count.
// FIXED: Ghidra's decompile never captures gt2Send's return value (`iVar3`, the loop's own
// exit condition, is left literally unmodified inside the loop body), which would make the
// `while (iVar3 == -4)` retry either never repeat or loop forever depending on the initial byte
// count -- clearly not the intended "retry on a transient socket error" behavior the summary
// describes. This rewrite assigns gt2Send's result back to `byte_count` each iteration,
// which is the only reading that makes the loop terminate sensibly.
// register/parameter convention: ESI -> stream (elided). blam-cc: ESI -> stream, stack ->
// channel, mode
// NOTE for future reconciliation: several already-written files outside this batch's assigned
// range (network_connection_finalize_join.c, network_game_record_message_send.c,
// network_session_info_packet_send.c, network_staged_message_commit.c, and others) declare this
// function as `extern char FUN_004ddb60(network_channel *channel, int32_t unknown)` -- a 2-arg
// guess made before this address was analyzed in depth ("not in this batch"). That guess omits
// the stream argument this rewrite's own analysis of the function body shows is required (ESI
// selects channel->in vs channel->retransmit); those callers' behavior for whichever stream they
// implicitly intended was not re-verified here and their extern declarations were not changed,
// per the rule against editing files outside this batch's assigned range.

#include "win32.h"
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern int32_t network_bit_chunk_size; // 0x0071c2cc
extern int32_t bit_stream_write_bits_chunked(bit_stream *stream, const uint32_t *values, int32_t total_bit_count); // 0x4cf8f0, EAX stream, ECX values
extern int32_t gt2Send(int32_t socket, uint8_t *buffer, int32_t byte_count, int32_t mode); // foreign, GameSpy/transport library

// blam-cc: ESI -> stream
char network_channel_stream_flush(network_channel_stream *stream, network_channel *channel, char mode)
{
    uint32_t first_bit;
    uint32_t used_bits;
    uint32_t rem;
    int32_t byte_count;
    int32_t send_result;
    int32_t success;
    int32_t send_mode;

    success = 0;
    first_bit = stream->stream.first_bit;
    // FIXED (0x4ddb63..0x4ddb75): the position is bit_cursor + byte_cursor * 8 (the draft used last_bit)
    used_bits = (stream->stream.bit_cursor + stream->stream.byte_cursor * 8) - first_bit;
    rem = used_bits & 0x80000007;
    if ((int32_t)rem < 0) {
        rem = (rem - 1 | 0xfffffff8) + 1;
    }
    byte_count = (rem != 0) + ((int32_t)(used_bits + ((int32_t)used_bits >> 31 & 7)) >> 3);
    if (first_bit <= stream->stream.last_bit || first_bit == stream->stream.last_bit + 1) {
        stream->stream.byte_cursor = first_bit >> 3;
        stream->stream.bit_cursor = first_bit & 7;
        // 0x4ddbcc: the packet's byte count goes into the header chunk (ECX = &byte_count)
        send_result = bit_stream_write_bits_chunked(&stream->stream, (const uint32_t *)&byte_count, network_bit_chunk_size);
        if (send_result == network_bit_chunk_size) {
            do {
                if (channel->endpoint->connection_failed == 1) {
                    break;
                }
                send_mode = mode != 0;
                byte_count = gt2Send(channel->endpoint->socket, (uint8_t *)stream + 0x1d,
                    byte_count, send_mode);
                if (byte_count > 0) {
                    success = 1;
                    break;
                }
            } while (byte_count == -4);
        }
    }
    first_bit = stream->stream.first_bit;
    stream->empty = 1;
    if (first_bit <= stream->stream.last_bit || first_bit == stream->stream.last_bit + 1) {
        stream->stream.bit_cursor = first_bit & 7;
        stream->stream.byte_cursor = first_bit >> 3;
    }
    {   // 0x4ddc3b..0x4ddc62: the header chunk is reset to 0
        uint32_t zero = 0;
        bit_stream_write_bits_chunked(&stream->stream, &zero, network_bit_chunk_size);
    }
    channel->send_budget = channel->send_budget + 0xe0;
    channel->budget_base_tick = GetTickCount();
    return success > 0;
}

#if 0
Original Ghidra decompilation (0x4ddb60):

bool FUN_004ddb60(int *param_1,char param_2)

{
  uint uVar1;
  int iVar2;
  int iVar3;
  uint uVar4;
  int iVar5;
  DWORD DVar6;
  int unaff_ESI;
  uint uVar7;
  int local_c [3];

  iVar2 = DAT_0071c2cc;
  uVar1 = *(uint *)(unaff_ESI + 8);
  uVar4 = (*(int *)(unaff_ESI + 0x10) + *(int *)(unaff_ESI + 0xc) * 8) - uVar1;
  uVar7 = uVar4 & 0x80000007;
  local_c[2] = 0;
  if ((int)uVar7 < 0) {
    uVar7 = (uVar7 - 1 | 0xfffffff8) + 1;
  }
  local_c[0] = (uint)(uVar7 != 0) + ((int)(uVar4 + ((int)uVar4 >> 0x1f & 7U)) >> 3);
  if ((uVar1 <= *(uint *)(unaff_ESI + 0x14)) || (uVar1 == *(uint *)(unaff_ESI + 0x14) + 1)) {
    *(uint *)(unaff_ESI + 0xc) = uVar1 >> 3;
    *(uint *)(unaff_ESI + 0x10) = uVar1 & 7;
    iVar5 = bit_stream_write_bits_chunked(iVar2);
    iVar3 = local_c[0];
    if (iVar5 == iVar2) {
      do {
        if (*(char *)(*param_1 + 5) == '\x01') break;
        local_c[0] = 0;
        local_c[1] = 1;
        FUN_006146b0(*(undefined4 *)*param_1,unaff_ESI + 0x1d,iVar3,local_c[param_2 != '\0']);
        if (0 < iVar3) {
          local_c[2] = 1;
          break;
        }
      } while (iVar3 == -4);
    }
  }
  uVar1 = *(uint *)(unaff_ESI + 8);
  local_c[0] = 0;
  *(undefined1 *)(unaff_ESI + 0x1c) = 1;
  if ((uVar1 <= *(uint *)(unaff_ESI + 0x14)) || (uVar1 == *(uint *)(unaff_ESI + 0x14) + 1)) {
    *(uint *)(unaff_ESI + 0x10) = uVar1 & 7;
    *(uint *)(unaff_ESI + 0xc) = uVar1 >> 3;
  }
  bit_stream_write_bits_chunked(DAT_0071c2cc);
  param_1[0x2a0] = param_1[0x2a0] + 0xe0;
  DVar6 = GetTickCount();
  param_1[0x2a1] = DVar6;
  return 0 < local_c[2];
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
