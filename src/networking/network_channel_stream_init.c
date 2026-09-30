// network_channel_stream_init  (Ghidra: FUN_004dd980; named per this rewrite)
// address 0x4dd980, size 80 bytes
// name confidence: 0.35   rewrite confidence: 0.55
// evidence: out/phase4/networking_functions.md: "Initializes a small per-direction bookkeeping
// record (window sizes, flags) for a newly-created channel and primes the shared output queue
// with a default byte count." Every field written (data = this+0x1d, last_bit = 0x287f,
// capacity_bits = 0x2880, empty = 1) matches types/networking.h's network_channel_stream and
// its k_network_channel_stream_bits constant exactly; network_bit_chunk_size defaults to 11 (0xb)
// here, matching the header's own note.
// UNSURE: bit_stream_write_bits_chunked's value/stream arguments are elided at this call site;
// reconstructed as (0, &stream->stream) -- priming the stream with one chunk-sized zero value --
// matching the summary's "primes ... with a default byte count".
// register convention: stream in EAX (in_EAX). blam-cc: EAX -> stream

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "fn_memory.h"

extern int32_t network_bit_chunk_size; // 0x0071c2cc


// blam-cc: EAX -> stream
void network_channel_stream_init(network_channel_stream *stream)
{
    if (network_bit_chunk_size == 0) {
        network_bit_chunk_size = 0xb;
    }
    stream->stream.unknown_00 = 0;
    stream->stream.data = (uint8_t *)stream + 0x1d;
    stream->stream.first_bit = 0;
    stream->stream.byte_cursor = 0;
    stream->stream.bit_cursor = 0;
    stream->stream.last_bit = 0x287f;
    stream->capacity_bits = 0x2880;
    stream->empty = 1;
    {   // 0x4dd99d / 0x4dd9b7: ECX = &a local holding 0
        uint32_t zero = 0;
        bit_stream_write_bits_chunked(&stream->stream, &zero, network_bit_chunk_size);
    }
}

#if 0
Original Ghidra decompilation (0x4dd980):

void FUN_004dd980(void)

{
  int iVar1;
  undefined4 *in_EAX;

  if (DAT_0071c2cc == 0) {
    DAT_0071c2cc = 0xb;
  }
  iVar1 = DAT_0071c2cc;
  *in_EAX = 0;
  in_EAX[1] = (int)in_EAX + 0x1d;
  in_EAX[2] = 0;
  in_EAX[5] = 0x287f;
  in_EAX[6] = 0x2880;
  *(undefined1 *)(in_EAX + 7) = 1;
  in_EAX[4] = 0;
  in_EAX[3] = 0;
  bit_stream_write_bits_chunked(iVar1);
  return;
}
#endif
