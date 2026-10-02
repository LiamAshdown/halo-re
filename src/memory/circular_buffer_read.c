// circular_buffer_read  (Ghidra: circular_buffer_read, already named)
// address 0x4d0240, size 124 bytes
// name confidence: 0.8   rewrite confidence: 0.7
// evidence: out/phase4/memory_types_notes.md "circular_buffer"; mirror of circular_buffer_write
// (0x4d01c0), advancing +0x08 (read_cursor) only when told to.
// register convention: destination buffer, byte count and peek flag are all Ghidra-recognized
// parameters (param_1, param_2, param_3); circular_buffer* is in EDX (in_EDX).

#include "tags.h"
#include "memory.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

// blam-cc: stream in EDX; destination, count, consume flag are the recognized parameters
// Reads byte_count bytes from stream into destination, with wraparound. Advances the read
// cursor only when `consume` is nonzero; a zero `consume` leaves the cursor untouched, i.e. this
// call is a non-consuming peek. Fails without reading anything if fewer than byte_count bytes
// are available. Returns 1 on success, 0 on failure.
uint32_t circular_buffer_read(uint8_t *destination, uint32_t byte_count, char consume,
    circular_buffer *stream)
{
    int32_t read_cursor;
    uint32_t result;
    int32_t available;
    uint32_t tail_room;
    uint8_t *src;
    uint8_t *dst;
    uint32_t words;
    uint32_t bytes;

    read_cursor = stream->read_cursor;
    result = 0;
    available = stream->write_cursor - read_cursor;
    if (available < 0) {
        available = available + stream->capacity;
    }
    if ((int32_t)byte_count <= available) {
        tail_room = (uint32_t)stream->capacity - (uint32_t)read_cursor;
        if (tail_room <= byte_count) {
            src = stream->data + read_cursor;
            dst = destination;
            for (words = tail_room >> 2; words != 0; words = words - 1) {
                *(uint32_t *)dst = *(uint32_t *)src;
                src = src + 4;
                dst = dst + 4;
            }
            for (bytes = tail_room & 3; bytes != 0; bytes = bytes - 1) {
                *dst = *src;
                src = src + 1;
                dst = dst + 1;
            }
            destination = destination + tail_room;
            read_cursor = 0;
            byte_count = byte_count - tail_room;
        }
        if (0 < (int32_t)byte_count) {
            src = stream->data + read_cursor;
            for (words = byte_count >> 2; words != 0; words = words - 1) {
                *(uint32_t *)destination = *(uint32_t *)src;
                src = src + 4;
                destination = destination + 4;
            }
            for (bytes = byte_count & 3; bytes != 0; bytes = bytes - 1) {
                *destination = *src;
                src = src + 1;
                destination = destination + 1;
            }
            read_cursor = read_cursor + (int32_t)byte_count;
        }
        if (consume != 0) {
            stream->read_cursor = read_cursor;
        }
        result = 1;
    }
    return result;
}

#if 0
Original Ghidra decompilation (0x4d0240):

undefined4 circular_buffer_read(undefined4 *param_1,uint param_2,char param_3)

{
  undefined4 uVar1;
  uint uVar2;
  int iVar3;
  uint uVar4;
  int in_EDX;
  int iVar5;
  undefined4 *puVar6;
  undefined4 *puVar7;

  iVar5 = *(int *)(in_EDX + 8);
  uVar1 = 0;
  iVar3 = *(int *)(in_EDX + 0xc) - iVar5;
  if (iVar3 < 0) {
    iVar3 = iVar3 + *(int *)(in_EDX + 0x10);
  }
  if ((int)param_2 <= iVar3) {
    uVar2 = *(int *)(in_EDX + 0x10) - iVar5;
    if ((int)uVar2 <= (int)param_2) {
      puVar6 = (undefined4 *)(*(int *)(in_EDX + 0x14) + iVar5);
      puVar7 = param_1;
      for (uVar4 = uVar2 >> 2; uVar4 != 0; uVar4 = uVar4 - 1) {
        *puVar7 = *puVar6;
        puVar6 = puVar6 + 1;
        puVar7 = puVar7 + 1;
      }
      for (uVar4 = uVar2 & 3; uVar4 != 0; uVar4 = uVar4 - 1) {
        *(undefined1 *)puVar7 = *(undefined1 *)puVar6;
        puVar6 = (undefined4 *)((int)puVar6 + 1);
        puVar7 = (undefined4 *)((int)puVar7 + 1);
      }
      param_1 = (undefined4 *)((int)param_1 + uVar2);
      iVar5 = 0;
      param_2 = param_2 - uVar2;
    }
    if (0 < (int)param_2) {
      puVar6 = (undefined4 *)(*(int *)(in_EDX + 0x14) + iVar5);
      for (uVar2 = param_2 >> 2; uVar2 != 0; uVar2 = uVar2 - 1) {
        *param_1 = *puVar6;
        puVar6 = puVar6 + 1;
        param_1 = param_1 + 1;
      }
      for (uVar2 = param_2 & 3; uVar2 != 0; uVar2 = uVar2 - 1) {
        *(undefined1 *)param_1 = *(undefined1 *)puVar6;
        puVar6 = (undefined4 *)((int)puVar6 + 1);
        param_1 = (undefined4 *)((int)param_1 + 1);
      }
      iVar5 = iVar5 + param_2;
    }
    if (param_3 != '\0') {
      *(int *)(in_EDX + 8) = iVar5;
    }
    uVar1 = 1;
  }
  return uVar1;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
