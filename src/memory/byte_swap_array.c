// byte_swap_array  (Ghidra: byte_swap_array, already named)
// address 0x4cfd90, size 331 bytes
// name confidence: 0.7   rewrite confidence: 0.6
// evidence: out/phase4/memory_types_notes.md; called by FUN_004d0700/byte_stream_read_long with
// a -4 (or -8/-2) size code, matching struct_definition_byte_swap's byte_swap_code enum in
// types/memory.h (_byte_swap_int64/_byte_swap_int32/_byte_swap_int16 = -8/-4/-2).
// register convention: element size code in EAX (in_EAX), array pointer in ECX (in_ECX),
// element count in EDX (in_EDX).
// UNSURE: the arithmetic below is transcribed literally from Ghidra rather than simplified to a
// textbook bswap, to avoid silently changing behavior; it has been hand-checked to be a byte
// reversal for the -4 and -2 cases but not independently re-derived from the disassembly.

#include "tags.h"
#include "memory.h"

// blam-cc: size code in EAX, array in ECX, count in EDX
// In-place byte-swaps `count` elements of size 8, 4, or 2 bytes (selected by `size_code`, one of
// _byte_swap_int64/_byte_swap_int32/_byte_swap_int16) starting at `array`, for endian conversion.
// Any other size_code is a no-op.
void byte_swap_array(int32_t size_code, uint32_t *array, int32_t count)
{
    uint32_t low;
    uint32_t high;
    int32_t remaining;

    if (size_code == -8) {
        remaining = count;
        if (0 < count) {
            do {
                low = array[0];
                high = array[1];
                array[0] = (high >> 0x10 | ((high & 0xff0000) >> 0x10 | high & 0xff00) << 0x10) >> 8 |
                           high << 0x18;
                array[1] = (low << 0x10 | ((low & 0xff00) << 0x10 | low & 0xff0000) >> 0x10) << 8 |
                           low >> 0x18;
                array = array + 2;
                remaining = remaining - 1;
            } while (remaining != 0);
        }
    } else if (size_code == -4) {
        if (0 < count) {
            do {
                low = *array;
                *array = (low & 0xff0000 | low >> 0x10) >> 8 | (low << 0x10 | low & 0xff00) << 8;
                array = array + 1;
                count = count - 1;
            } while (count != 0);
        }
    } else if (size_code == -2 && 0 < count) {
        uint16_t *array16 = (uint16_t *)array;
        do {
            count = count - 1;
            *array16 = (uint16_t)(((*array16 & 0xff) << 8) | ((*array16 >> 8) & 0xff));
            array16 = array16 + 1;
        } while (count != 0);
    }
}

#if 0
Original Ghidra decompilation (0x4cfd90):

void byte_swap_array(void)

{
  uint uVar1;
  uint uVar2;
  int in_EAX;
  uint *in_ECX;
  int in_EDX;
  int local_4;
  
  if (in_EAX == -8) {
    local_4 = in_EDX;
    if (0 < in_EDX) {
      do {
        uVar1 = *in_ECX;
        uVar2 = in_ECX[1];
        *in_ECX = (uVar2 >> 0x10 | ((uVar2 & 0xff0000) >> 0x10 | uVar2 & 0xff00) << 0x10) >> 8 |
                  uVar2 << 0x18;
        in_ECX[1] = (uVar1 << 0x10 | ((uVar1 & 0xff00) << 0x10 | uVar1 & 0xff0000) >> 0x10) << 8 |
                    uVar1 >> 0x18;
        in_ECX = in_ECX + 2;
        local_4 = local_4 + -1;
      } while (local_4 != 0);
    }
  }
  else if (in_EAX == -4) {
    if (0 < in_EDX) {
      do {
        uVar1 = *in_ECX;
        *in_ECX = (uVar1 & 0xff0000 | uVar1 >> 0x10) >> 8 | (uVar1 << 0x10 | uVar1 & 0xff00) << 8;
        in_ECX = in_ECX + 1;
        in_EDX = in_EDX + -1;
      } while (in_EDX != 0);
      return;
    }
  }
  else if ((in_EAX == -2) && (0 < in_EDX)) {
    do {
      in_EDX = in_EDX + -1;
      *(ushort *)in_ECX = CONCAT11((char)(short)*in_ECX,(char)((ushort)(short)*in_ECX >> 8));
      in_ECX = (uint *)((int)in_ECX + 2);
    } while (in_EDX != 0);
    return;
  }
  return;
}
#endif
