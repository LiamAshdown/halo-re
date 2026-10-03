// circular_buffer_write  (Ghidra: circular_buffer_write, already named)
// address 0x4d01c0, size 118 bytes
// name confidence: 0.8   rewrite confidence: 0.7
// evidence: out/phase4/memory_types_notes.md "circular_buffer"; field offsets +8 read_cursor,
// +0xc write_cursor, +0x10 capacity, +0x14 data match types/memory.h exactly.
// register convention: source buffer as the recognized parameter (param_1); byte count in EAX
// (in_EAX); circular_buffer* in EDX (in_EDX). Exposed in EAX,EDX,stack order: byte count, then
// the buffer, then the recognized stack parameter last.
// UNSURE: param_1's physical location (register vs. stack) is not stated by Ghidra beyond being
// its own recognized parameter; declared last here per the EAX,ECX,EDX,EBX,ESI,EDI,then-stack
// convention.

#include "tags.h"
#include "memory.h"

// blam-cc: byte count in EAX, stream in EDX, source buffer as the recognized parameter
// Writes byte_count bytes from source into the circular buffer, wrapping at capacity and
// advancing the write cursor. Fails without writing anything if there isn't enough free space
// for the whole write. Returns 1 on success, 0 on failure.
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
uint32_t circular_buffer_write(uint32_t byte_count, circular_buffer *stream, uint8_t *source)
{
    int32_t write_cursor;
    uint32_t result;
    int32_t used;
    uint32_t tail_room;
    uint8_t *src;
    uint8_t *dst;
    uint32_t words;
    uint32_t bytes;

    write_cursor = stream->write_cursor;
    result = 0;
    used = write_cursor - stream->read_cursor;
    if (used < 0) {
        used = used + stream->capacity;
    }
    if ((int32_t)((uint32_t)used + byte_count) < stream->capacity) {
        tail_room = (uint32_t)stream->capacity - (uint32_t)write_cursor;
        if (tail_room <= byte_count) {
            src = source;
            dst = stream->data + write_cursor;
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
            source = source + tail_room;
            stream->write_cursor = 0;
            byte_count = byte_count - tail_room;
        }
        if (0 < (int32_t)byte_count) {
            dst = stream->data + stream->write_cursor;
            src = source;
            for (words = byte_count >> 2; words != 0; words = words - 1) {
                *(uint32_t *)dst = *(uint32_t *)src;
                src = src + 4;
                dst = dst + 4;
            }
            for (bytes = byte_count & 3; bytes != 0; bytes = bytes - 1) {
                *dst = *src;
                src = src + 1;
                dst = dst + 1;
            }
            stream->write_cursor = stream->write_cursor + (int32_t)byte_count;
        }
        result = 1;
    }
    return result;
}

#if 0
Original Ghidra decompilation (0x4d01c0):

undefined4 circular_buffer_write(undefined4 *param_1)

{
  int iVar1;
  uint in_EAX;
  undefined4 uVar2;
  int iVar3;
  uint uVar4;
  int in_EDX;
  undefined4 *puVar5;
  uint uVar6;
  undefined4 *puVar7;

  iVar1 = *(int *)(in_EDX + 0xc);
  uVar2 = 0;
  iVar3 = iVar1 - *(int *)(in_EDX + 8);
  if (iVar3 < 0) {
    iVar3 = iVar3 + *(int *)(in_EDX + 0x10);
  }
  if ((int)(iVar3 + in_EAX) < *(int *)(in_EDX + 0x10)) {
    uVar6 = *(int *)(in_EDX + 0x10) - iVar1;
    if ((int)uVar6 <= (int)in_EAX) {
      puVar5 = param_1;
      puVar7 = (undefined4 *)(*(int *)(in_EDX + 0x14) + iVar1);
      for (uVar4 = uVar6 >> 2; uVar4 != 0; uVar4 = uVar4 - 1) {
        *puVar7 = *puVar5;
        puVar5 = puVar5 + 1;
        puVar7 = puVar7 + 1;
      }
      for (uVar4 = uVar6 & 3; uVar4 != 0; uVar4 = uVar4 - 1) {
        *(undefined1 *)puVar7 = *(undefined1 *)puVar5;
        puVar5 = (undefined4 *)((int)puVar5 + 1);
        puVar7 = (undefined4 *)((int)puVar7 + 1);
      }
      param_1 = (undefined4 *)((int)param_1 + uVar6);
      *(undefined4 *)(in_EDX + 0xc) = 0;
      in_EAX = in_EAX - uVar6;
    }
    if (0 < (int)in_EAX) {
      puVar5 = (undefined4 *)(*(int *)(in_EDX + 0x14) + *(int *)(in_EDX + 0xc));
      for (uVar6 = in_EAX >> 2; uVar6 != 0; uVar6 = uVar6 - 1) {
        *puVar5 = *param_1;
        param_1 = param_1 + 1;
        puVar5 = puVar5 + 1;
      }
      for (uVar6 = in_EAX & 3; uVar6 != 0; uVar6 = uVar6 - 1) {
        *(undefined1 *)puVar5 = *(undefined1 *)param_1;
        param_1 = (undefined4 *)((int)param_1 + 1);
        puVar5 = (undefined4 *)((int)puVar5 + 1);
      }
      *(int *)(in_EDX + 0xc) = *(int *)(in_EDX + 0xc) + in_EAX;
    }
    uVar2 = 1;
  }
  return uVar2;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
