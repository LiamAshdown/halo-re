// message_delta_encode_message  (Ghidra: message_delta_encode_message, already named)
// address 0x4ec940, size 536 bytes
// name confidence: 0.6   rewrite confidence: 0.3
// evidence: objdump -d -M intel bin/halo.exe @0x4ec940..0x4ecb60 pins the encode context's base
// address (a ~0x94-byte scratch block built on this function's own stack) and its +0x00, +0x04,
// +0x08, +0x0c, +0x10, +0x18, +0x30, +0x34, +0x38, +0x3c fields, and confirms
// message_delta_encode_prepare_item (0x4ecb60) and message_delta_encode_message_header
// (0x4ecd00) both receive that same base, while message_delta_encode_all_fields (0x4ecc00)
// receives a pointer 0xc bytes into it.
// register convention: the __cdecl stack parameters Ghidra recognized (flag, message_type,
// changed_offset, items, type_offset, count, force_changed), plus two further hidden register
// parameters (in_EDX, and an EAX value saved into the context at +0x0c/+0x20) whose exact
// meaning is unresolved -- see UNSURE notes on the sibling files in this cluster.
// UNSURE: this is the least-resolved function in the message-delta cluster; the per-item loop's
// exact use of changed_offset/items/type_offset against the two hidden register inputs could not
// be fully pinned down from the decompilation and disassembly available here. Preserved as a
// direct, offset-for-offset transliteration of the Ghidra decompilation rather than a
// re-derivation, so control flow and arithmetic match exactly even where field names do not.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "fn_memory.h"

extern message_delta_definition *message_delta_definitions[56]; // 0x0065d440
extern uint8_t message_delta_item_count_bits[];                 // 0x0065d51f

extern uint8_t message_delta_encode_prepare_item(uint8_t *ctx); // 0x4ecb60, EAX ctx
extern uint8_t message_delta_encode_all_fields(uint8_t *ctx, int32_t static_base, int32_t item, int32_t type_base); // 0x4ecc00, EAX ctx
extern uint8_t message_delta_encode_message_header(uint8_t *ctx); // 0x4ecd00, ESI ctx


// REWRITTEN from objdump 0x4ec940..0x4ecb57 (EAX = output buffer, EDX = its size in bits; stack as declared).
// The context (0x94 bytes, zeroed) holds: +0 "started" byte, +4 message type, +8 flag, +0xc buffer, +0x10 size,
// +0x14 total item bits, +0x18 remaining budget, +0x1c an inline bit_stream {0, buffer, 0, 0, 0, header_bits - 1},
// +0x34 header bits, +0x38 item count, +0x3c running bit offset, then per-item blocks the helpers fill (+0x40 static
// bits, +0x44 field bits, +0x48.. and +0x64.. reset after every item). Each item passes (baseline, item, type) --
// baseline/type from the parallel arrays when given -- to message_delta_encode_all_fields; items that changed (or all,
// when force_changed) are counted. A multi-item message then writes (item count - 1) in the definition's count width.
// Returns header bits + item bits, or 0 when nothing was encoded.
// blam-cc: EAX -> extra_eax, EDX -> extra_edx, stack -> flag, message_type, changed_offset, items, type_offset, count, force_changed
int32_t message_delta_encode_message(int32_t extra_eax, int32_t extra_edx, int32_t flag, int32_t message_type,
                                      int32_t changed_offset, void **items, int32_t type_offset, int32_t count,
                                      char force_changed)
{
    uint32_t ctx_storage[0x98 / 4];
    uint8_t *ctx = (uint8_t *)ctx_storage;
    #define CTXD(off) (*(int32_t *)(ctx + (off)))
    message_delta_definition *definition;
    int32_t header_bits;
    int32_t i;

    for (i = 0; i < 0x98 / 4; i++) {
        ctx_storage[i] = 0;
    }
    header_bits = message_delta_definitions[message_type]->header_bits; // +0xc
    CTXD(4) = message_type;
    CTXD(8) = flag;
    CTXD(0xc) = extra_eax;
    CTXD(0x10) = extra_edx;
    CTXD(0x18) = extra_edx - header_bits;
    CTXD(0x20) = extra_eax;              // the inline stream's data
    CTXD(0x30) = header_bits - 1;        // ...its last bit
    CTXD(0x34) = header_bits;
    CTXD(0x3c) = header_bits;
    ctx[0] = 1;
    message_delta_encode_message_header(ctx);

    if (0 < count) {
        void **cursor = items;
        int32_t remaining = count;
        do {
            int32_t baseline = (changed_offset == 0) ? 0
                : *(int32_t *)((uint8_t *)cursor + (changed_offset - (int32_t)items));
            void *item = *cursor;
            int32_t type = (flag == 0) ? 0
                : *(int32_t *)((uint8_t *)cursor + (type_offset - (int32_t)items));

            message_delta_encode_prepare_item(ctx);
            message_delta_encode_all_fields(ctx, baseline, (int32_t)item, type);
            if (0 < CTXD(0x44) || force_changed != 0) {
                int32_t bits = CTXD(0x44) + CTXD(0x40);
                CTXD(0x14) = CTXD(0x14) + bits;
                CTXD(0x18) = CTXD(0x18) - bits;
                CTXD(0x3c) = CTXD(0x3c) + bits;
                CTXD(0x38) = CTXD(0x38) + 1;
            }
            if (CTXD(8) == 1) {
                CTXD(0x48) = -1;
                CTXD(0x4c) = 0; CTXD(0x50) = 0; CTXD(0x54) = 0; CTXD(0x58) = 0; CTXD(0x5c) = 0; CTXD(0x60) = 0;
            }
            CTXD(0x64) = -1;
            CTXD(0x68) = 0; CTXD(0x6c) = 0; CTXD(0x70) = 0; CTXD(0x74) = 0; CTXD(0x78) = 0; CTXD(0x7c) = 0;
            cursor = cursor + 1;
            remaining = remaining - 1;
        } while (remaining != 0);
    }

    definition = message_delta_definitions[CTXD(4)];
    if (CTXD(0x14) <= 0) {
        return 0;
    }
    if (1 < definition->maximum_items) { // +0x14
        int32_t count_bits = message_delta_item_count_bits[definition->maximum_items];
        uint32_t value = (uint32_t)(CTXD(0x38) - 1);
        bit_stream_write_bits_chunked((bit_stream *)(ctx + 0x1c), &value, count_bits);
        CTXD(0x88) = count_bits;
    }
    return definition->header_bits + CTXD(0x14);
    #undef CTXD
}

#if 0
Original Ghidra decompilation (0x4ec940):

int __cdecl
message_delta_encode_message
          (int flag,int message_type,int changed_offset,void **items,int type_offset,int count,
          char force_changed)

{
  void *pvVar1;
  undefined *puVar2;
  int iVar3;
  int in_EDX;
  undefined4 uVar4;
  int *piVar5;
  undefined4 uVar6;
  void **local_a4;
  int local_9c;
  int local_90 [4];
  int local_80;
  int local_7c;
  undefined4 local_78;
  undefined4 local_70;
  undefined4 local_6c;
  undefined4 local_68;
  int local_64;
  int local_60;
  int local_5c;
  int local_58;
  int local_54;
  int local_50;
  undefined4 local_4c;
  undefined4 local_48;
  undefined4 local_44;
  undefined4 local_40;
  undefined4 local_3c;
  undefined4 local_38;
  undefined4 local_34;
  undefined4 local_30;
  undefined4 local_2c;
  undefined4 local_28;
  undefined4 local_24;
  undefined4 local_20;
  undefined4 local_1c;
  undefined4 local_18;

  piVar5 = local_90;
  for (iVar3 = 0x24; iVar3 != 0; iVar3 = iVar3 + -1) {
    *piVar5 = 0;
    piVar5 = piVar5 + 1;
  }
  local_60 = *(int *)((&PTR_DAT_0065d440)[message_type] + 0xc);
  local_7c = in_EDX - local_60;
  local_90[0] = message_type;
  local_64 = local_60 + -1;
  local_90[1] = flag;
  local_80 = 0;
  local_5c = 0;
  local_68 = 0;
  local_6c = 0;
  local_78 = 0;
  local_70 = 0;
  local_58 = local_60;
  message_delta_encode_message_header();
  if (0 < count) {
    local_a4 = items;
    local_9c = count;
    do {
      if (changed_offset == 0) {
        uVar6 = 0;
      }
      else {
        uVar6 = *(undefined4 *)((changed_offset - (int)items) + (int)local_a4);
      }
      pvVar1 = *local_a4;
      if (flag == 0) {
        uVar4 = 0;
      }
      else {
        uVar4 = *(undefined4 *)((type_offset - (int)items) + (int)local_a4);
      }
      FUN_004ecb60();
      message_delta_encode_all_fields(uVar6,pvVar1,uVar4);
      if ((0 < local_50) || (force_changed != '\0')) {
        iVar3 = local_50 + local_54;
        local_80 = local_80 + iVar3;
        local_7c = local_7c - iVar3;
        local_58 = local_58 + iVar3;
        local_5c = local_5c + 1;
      }
      if (local_90[1] == 1) {
        local_4c = 0xffffffff;
        local_48 = 0;
        local_44 = 0;
        local_40 = 0;
        local_3c = 0;
        local_38 = 0;
        local_34 = 0;
      }
      local_a4 = local_a4 + 1;
      local_9c = local_9c + -1;
      local_30 = 0xffffffff;
      local_2c = 0;
      local_28 = 0;
      local_24 = 0;
      local_20 = 0;
      local_1c = 0;
      local_18 = 0;
    } while (local_9c != 0);
  }
  puVar2 = (&PTR_DAT_0065d440)[local_90[0]];
  iVar3 = 0;
  if (0 < local_80) {
    iVar3 = *(int *)(puVar2 + 0x14);
    if (1 < iVar3) {
      bit_stream_write_bits_chunked((&DAT_0065d51f)[iVar3]);
    }
    iVar3 = *(int *)(puVar2 + 0xc) + local_80;
  }
  return iVar3;
}
#endif
