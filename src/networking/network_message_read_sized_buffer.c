// network_message_read_sized_buffer  (Ghidra: FUN_004de420; named per this rewrite)
// address 0x4de420, size 68 bytes
// name confidence: 0.25   rewrite confidence: 0.25
// evidence: out/phase4/networking_functions.md: "Register-based helper (EDI) that validates a
// size/capacity value obtained twice from FUN_004cf950 against param_1 before returning the
// buffer pointer, otherwise returns null." Reads a 16-bit chunked header into `buffer` itself,
// checks its high 12 bits (a byte count) against the caller's capacity, then reads that many
// more bits and confirms the bit count consumed matches exactly, before returning `buffer`.
// UNSURE: the exact bit bookkeeping (why the second read's expected size is
// `(uVar1>>4)*8-0x10`) is preserved literally rather than re-derived; `stream` (bit_stream_read_
// bits_chunked's third argument) is entirely elided here and could not be reconstructed.
// register convention: EDI -> buffer, EBX -> stream. blam-cc: EDI -> buffer, EBX -> stream, stack -> capacity
// FIXED (register inputs, objdump): EBX (read at 0x4de420, "push ebx" as the very first
// instruction, before eax/ecx are even set up) is the stream pointer -- it is pushed as the
// third stack argument to both bit_stream_read_bits_chunked calls. `stream` was already a
// parameter of this rewrite but was wrongly modelled as a stack argument instead of EBX.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"

extern int32_t bit_stream_read_bits_chunked(int32_t total_bit_count, uint32_t *buffer, bit_stream *stream); // 0x4cf950, memory module

// blam-cc: EDI -> buffer, EBX -> stream
uint16_t *network_message_read_sized_buffer(uint16_t *buffer, int32_t capacity, bit_stream *stream)
{
    int32_t consumed;
    uint16_t header;

    consumed = bit_stream_read_bits_chunked(0x10, (uint32_t *)buffer, stream);
    if (consumed == 0x10) {
        header = *buffer;
        if ((int32_t)(uint32_t)(header >> 4) <= capacity) {
            consumed = bit_stream_read_bits_chunked((header >> 4) * 8 - 0x10, (uint32_t *)buffer, stream);
            if (consumed == (header >> 4) * 8 - 0x10) {
                return buffer;
            }
        }
    }
    return 0;
}

#if 0
Original Ghidra decompilation (0x4de420):

ushort * FUN_004de420(int param_1)

{
  ushort uVar1;
  int iVar2;
  ushort *unaff_EDI;

  iVar2 = bit_stream_read_bits_chunked();
  if ((iVar2 == 0x10) && (uVar1 = *unaff_EDI, (int)(uint)(uVar1 >> 4) <= param_1)) {
    iVar2 = bit_stream_read_bits_chunked();
    if (iVar2 == (uint)(uVar1 >> 4) * 8 + -0x10) {
      return unaff_EDI;
    }
  }
  return (ushort *)0x0;
}
#endif
