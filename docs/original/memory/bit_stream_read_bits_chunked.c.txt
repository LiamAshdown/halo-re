// bit_stream_read_bits_chunked  (Ghidra: FUN_004cf950)
// address 0x4cf950, size 78 bytes
// name confidence: 0.4   rewrite confidence: 0.6
// evidence: out/phase4/memory_functions.md summary "Reads in_EAX bits from a bit stream into a
// uint32 buffer, 32 bits per call to FUN_004cfbf0, returning the number of bits actually read.";
// 5 callers, callee bit_stream_read_bits (0x4cfbf0). Mirror of bit_stream_write_bits_chunked
// (0x4cf8f0), except this one visibly advances its destination-buffer register (in_ECX) by 4
// bytes after each full 32-bit chunk, confirming param_2 of bit_stream_read_bits really is a
// uint32_t* destination buffer, not a plain value.
// register convention: total bit count in EAX (in_EAX), destination buffer pointer in ECX
// (in_ECX), stream pointer forwarded through to bit_stream_read_bits without being read here
// (matches bit_stream_read_bits' own EDX stream parameter).

#include "tags.h"
#include "memory.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern uint32_t bit_stream_read_bits(uint32_t bit_count, uint32_t *out_value, bit_stream *stream);

// blam-cc: total bit count in EAX, destination buffer in ECX, stream forwarded (EDX)
// Reads total_bit_count bits out of stream into the uint32_t array at buffer, 32 bits (one
// uint32_t) per call to bit_stream_read_bits, advancing buffer between full chunks. Returns the
// number of bits actually read: total_bit_count on full success, or fewer if a chunk read came
// up short.
int32_t bit_stream_read_bits_chunked(int32_t total_bit_count, uint32_t *buffer, bit_stream *stream)
{
    int32_t remaining;
    int32_t total_read;
    uint32_t chunk_read;

    remaining = total_bit_count;
    total_read = 0;
    for (;;) {
        if (remaining < 1) {
            return total_read;
        }
        if (remaining < 0x20) {
            chunk_read = bit_stream_read_bits((uint32_t)remaining, buffer, stream);
            if (chunk_read != (uint32_t)remaining) {
                return total_read;
            }
        } else {
            chunk_read = bit_stream_read_bits(0x20, buffer, stream);
            if (chunk_read != 0x20) {
                return total_read;
            }
            buffer = buffer + 1;
            chunk_read = 0x20;
        }
        remaining = remaining - (int32_t)chunk_read;
        total_read = total_read + (int32_t)chunk_read;
    }
}

#if 0
Original Ghidra decompilation (0x4cf950):

int FUN_004cf950(void)

{
  int in_EAX;
  int iVar1;
  int in_ECX;
  int iVar2;

  iVar2 = 0;
  do {
    if (in_EAX < 1) {
      return iVar2;
    }
    if (in_EAX < 0x20) {
      iVar1 = bit_stream_read_bits(in_EAX,in_ECX);
      if (iVar1 != in_EAX) {
        return iVar2;
      }
    }
    else {
      iVar1 = bit_stream_read_bits(0x20,in_ECX);
      if (iVar1 != 0x20) {
        return iVar2;
      }
      in_ECX = in_ECX + 4;
      iVar1 = 0x20;
    }
    in_EAX = in_EAX - iVar1;
    iVar2 = iVar2 + iVar1;
  } while( true );
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
