// message_delta_sample_record_and_append  (Ghidra: FUN_004ed310; named per this rewrite)
// address 0x4ed310, size 54 bytes
// name confidence: 0.3   rewrite confidence: 0.9 (step 1: objdump -d 0x4ed310..0x4ed345 -- the ring is the one stack argument, not ESI)
// evidence: out/phase2/networking/07.md decompilation plus objdump -d -M intel bin/halo.exe
// @0x4ed310: builds a 20-byte (5-dword) sample record {a, b, c, (c-a)>>1, ((c-a)>>1)-c+b} on its
// own stack from three register inputs and forwards it to
// message_delta_sample_ring_buffer_append (0x4ed390) via ESI (the ring buffer) unchanged.
// register convention: a in EAX, c in ECX, b in EDX (recognized/register order), ring buffer
// pinned in ESI (unaff_ESI) for the call to message_delta_sample_ring_buffer_append.
// blam-cc: EAX -> a, ECX -> c, EDX -> b, stack -> ring
// UNSURE: a 4th value is loaded from the stack ([esp+0x1c] relative to this function's own
// frame) into a register that is never read again -- transcribed as an unused parameter rather
// than omitted, since it is a genuine load from the caller's stack.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"



#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern void message_delta_sample_ring_buffer_append(message_delta_sample_ring_buffer *ring, const int32_t *entry); // 0x4ed390, this module

// blam-cc: EAX -> a, ECX -> c, EDX -> b, stack -> ring
// Builds a 5-field sample record out of three inputs and appends it to the ring buffer.
void message_delta_sample_record_and_append(int32_t a, int32_t c, int32_t b,
                                             message_delta_sample_ring_buffer *ring)
{
    int32_t record[5];
    uint32_t half_span;

    half_span = ((uint32_t)c - (uint32_t)a) >> 1;
    record[0] = a;
    record[1] = b;
    record[2] = c;
    record[3] = (int32_t)half_span;
    record[4] = (int32_t)half_span - c + b;
    message_delta_sample_ring_buffer_append(ring, record);
}

#if 0
Original Ghidra decompilation (0x4ed310):

void FUN_004ed310(void)

{
  FUN_004ed390();
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
