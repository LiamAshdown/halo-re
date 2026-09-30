// bit_stream_write_bits_chunked  (Ghidra: FUN_004cf8f0)
// address 0x4cf8f0, size 82 bytes
// name confidence: 0.4   rewrite confidence: 0.55
// evidence: out/phase4/memory_functions.md summary "Writes param_1 bits to a bit stream by
// issuing 32-bit-chunk writes through FUN_004cfa20 (bit_stream_write_bits), stopping early and
// reporting the bit count if the stream runs out of room."; 24 callers, callee
// bit_stream_write_bits (0x4cfa20).
// register convention: total bit count as the recognized parameter (param_1); the value and
// bit_stream* arguments to bit_stream_write_bits are never read or reassigned by this function's
// own body, so Ghidra never materializes them as locals -- they are register values (value in
// EDX, stream in ESI, matching bit_stream_write_bits' own convention) that simply flow through
// this function unchanged into every call. Exposed here as ordinary pass-through parameters.
// UNSURE: because value is never advanced between chunk calls, every 32-bit chunk after the
// first writes the same low bits of value again (bit_stream_write_bits does not return its
// shifted-down remainder to the caller, and this function never reloads EDX from a source
// buffer the way the read counterpart, FUN_004cf950, advances its ECX buffer pointer between
// chunks). This is transcribed exactly as decompiled; whether real callers ever pass a total
// bit count above 32 (making this asymmetry observable) is not established from this pack alone.
// A plausible reading is that this helper is only exercised for counts <= 32 in practice.

#include "tags.h"
#include "memory.h"
#include "fn_memory.h"


// (superseded, see FIXED below) total bit count as the recognized parameter, value in EDX, stream in ESI (both
// pass-through, unread by this function)
// Writes total_bit_count bits to stream, split into 32-bit chunks via bit_stream_write_bits.
// Returns the number of bits actually written: total_bit_count on full success, or fewer if the
// stream ran out of room partway through.
// FIXED (objdump 0x4cf8f0..0x4cf941): EAX is the stream (forwarded to bit_stream_write_bits in ESI), ECX points at an
// ARRAY of 32-bit values -- one per 32-bit chunk, advanced by 4 each time -- and the total bit count is the one stack
// argument. The draft wrote the same value into every chunk and took the stream on the stack.
// blam-cc: EAX -> stream, ECX -> values, stack -> total_bit_count
int32_t bit_stream_write_bits_chunked(bit_stream *stream, const uint32_t *values, int32_t total_bit_count)
{
    int32_t remaining = total_bit_count;

    if (0 < total_bit_count) {
        while (0x1f < remaining) {
            if (bit_stream_write_bits(0x20, *values, stream) == 0) {
                return total_bit_count - remaining;
            }
            remaining = remaining - 0x20;
            values = values + 1;
            if (remaining < 1) {
                return total_bit_count - remaining;
            }
        }
        if (bit_stream_write_bits((uint32_t)remaining, *values, stream) != 0) {
            remaining = 0;
        }
    }
    return total_bit_count - remaining;
}

#if 0
Original Ghidra decompilation (0x4cf8f0):

int FUN_004cf8f0(int param_1)

{
  char cVar1;
  int iVar2;

  iVar2 = param_1;
  if (0 < param_1) {
    while (0x1f < iVar2) {
      cVar1 = bit_stream_write_bits(0x20);
      if (cVar1 == '\0') goto LAB_004cf939;
      iVar2 = iVar2 + -0x20;
      if (iVar2 < 1) {
        return param_1 - iVar2;
      }
    }
    cVar1 = bit_stream_write_bits(iVar2);
    if (cVar1 != '\0') {
      iVar2 = 0;
    }
  }
LAB_004cf939:
  return param_1 - iVar2;
}
#endif
