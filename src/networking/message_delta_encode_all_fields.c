// message_delta_encode_all_fields  (Ghidra: message_delta_encode_all_fields, already named)
// address 0x4ecc00, size 246 bytes
// name confidence: 0.5   rewrite confidence: 0.35
// evidence: out/phase4/networking_functions.md summary; walks a message type's static fields via
// each field type's encode callback at field_type+0x50 (the encode-side counterpart of the
// decode callback at +0x54 used throughout the decode half of this subsystem), then every
// top-level field via message_delta_encode_field.
// register convention: encode (sub-)context as the recognized parameter (in_EAX); two further
// __cdecl stack parameters (param_1, param_2).
// UNSURE: this receives a pointer 0xc bytes into the larger context
// message_delta_encode_message builds (see message_delta_encode_prepare_item.c), so its own +4
// here is NOT the same byte offset as that function's message_type field; transcribed using this
// function's own offsets exactly as decompiled, per networking_types_notes.md's acknowledgement
// that the message-delta encode/decode context layout is only partly resolved.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"

extern message_delta_definition *message_delta_definitions[56]; // 0x0065d440
extern uint8_t message_delta_field_changed_flags[0x40];      // 0x006b89c0, shared scratch (16 dwords)

typedef int32_t (*message_delta_field_encode_fn)(void *field_type, int32_t changed, int32_t offset, void *stream_or_ctx);
extern uint8_t message_delta_encode_field(int32_t changed_offset, uint8_t *ctx, int32_t field_index, int32_t type_offset); // 0x4ecde0, EAX changed_offset, ESI ctx

// Encodes all of one item's static fields (unconditionally, via each field type's own encode
// callback) and then every top-level field (via message_delta_encode_field), aggregating whether
// any field actually changed. Returns that combined changed flag in the low byte.
// REWRITTEN from objdump 0x4ecc00..0x4eccf5. EAX = the encoder context; stack (static_base, item, type_base) --
// the encoder pushes its per-item (baseline, item, type) values (0x4eca3b). Static fields: each binding's encode proc
// (field_type+0x50) gets (field_type, 0, static_base + binding->destination_offset, ctx+0x64); positive sizes add to
// ctx+0x40, a non-positive one marks failure but the loop goes on (the draft returned at once and used source_offset).
// A failure there returns 0. Then the 16 changed-flag dwords are cleared and every field is encoded with
// message_delta_encode_field(EAX = type_base, ESI = ctx, stack i, item); in flag mode (ctx+8 == 1) any change sets the
// result, otherwise all must succeed -- with no early exit in either case (the draft broke out of the loop). AL result.
// blam-cc: EAX -> ctx, stack -> static_base, item, type_base
uint8_t message_delta_encode_all_fields(uint8_t *ctx, int32_t static_base, int32_t item, int32_t type_base)
{
    #define CTXD(off) (*(int32_t *)(ctx + (off)))
    message_delta_definition *definition = message_delta_definitions[CTXD(4)];
    message_delta_static_fields *statics = definition->statics;
    int32_t field_count;
    int32_t i;
    uint8_t ok;

    if (0 < statics->count) {
        int32_t count = statics->count;
        ok = 1;
        for (i = 0; i < count; i++) {
            message_delta_field_binding *binding =
                &message_delta_definitions[CTXD(4)]->statics->fields[i];     // re-read each pass, as compiled
            message_delta_field_encode_fn encode =
                *(message_delta_field_encode_fn *)((uint8_t *)binding->field_type + 0x50);
            int32_t field_bits = encode(binding->field_type, 0, static_base + binding->destination_offset, ctx + 0x64);
            if (field_bits > 0) {
                CTXD(0x40) = CTXD(0x40) + field_bits;
                ok = ok ? 1 : 0;
            } else {
                ok = 0;
            }
        }
        if (!ok) {
            return 0;
        }
    }

    field_count = definition->field_count; // +0x20
    for (i = 0; i < 0x10; i++) {
        ((int32_t *)message_delta_field_changed_flags)[i] = 0;
    }
    ok = (uint8_t)(CTXD(8) != 1);
    for (i = 0; i < field_count; i++) {
        uint8_t changed = message_delta_encode_field(type_base, ctx, i, item);
        if (CTXD(8) == 1) {
            ok = (ok || changed) ? 1 : 0;
        } else {
            ok = (ok && changed) ? 1 : 0;
        }
    }
    return ok;
    #undef CTXD
}

#if 0
Original Ghidra decompilation (0x4ecc00):

undefined4 message_delta_encode_all_fields(int param_1,undefined4 param_2)

{
  undefined *puVar1;
  int *piVar2;
  bool bVar3;
  char cVar4;
  int in_EAX;
  int iVar5;
  int iVar6;
  undefined4 *puVar7;
  int iVar8;
  bool bVar9;
  int local_8;

  puVar1 = (&PTR_DAT_0065d440)[*(int *)(in_EAX + 4)];
  piVar2 = *(int **)(puVar1 + 0x1c);
  if (0 < *piVar2) {
    local_8 = *piVar2;
    bVar3 = true;
    if (0 < local_8) {
      iVar6 = 0;
      do {
        iVar5 = *(int *)(*(int *)((&PTR_DAT_0065d440)[*(int *)(in_EAX + 4)] + 0x1c) + 8 + iVar6);
        iVar5 = (**(code **)(iVar5 + 0x50))
                          (iVar5,0,*(int *)(*(int *)((&PTR_DAT_0065d440)[*(int *)(in_EAX + 4)] +
                                                    0x1c) + iVar6 + 0xc) + param_1,in_EAX + 100);
        if ((iVar5 < 1) || (*(int *)(in_EAX + 0x40) = *(int *)(in_EAX + 0x40) + iVar5, !bVar3)) {
          bVar3 = false;
        }
        else {
          bVar3 = true;
        }
        iVar6 = iVar6 + 0x10;
        local_8 = local_8 + -1;
      } while (local_8 != 0);
      bVar9 = false;
      iVar6 = 0;
      if (!bVar3) goto LAB_004eccec;
    }
  }
  iVar5 = *(int *)(puVar1 + 0x20);
  puVar7 = &DAT_006b89c0;
  for (iVar6 = 0x10; iVar6 != 0; iVar6 = iVar6 + -1) {
    *puVar7 = 0;
    puVar7 = puVar7 + 1;
  }
  bVar9 = *(int *)(in_EAX + 8) != 1;
  iVar8 = 0;
  iVar6 = 0;
  if (0 < iVar5) {
    do {
      cVar4 = message_delta_encode_field(iVar8,param_2);
      if (*(int *)(in_EAX + 8) == 1) {
        if ((bVar9 == false) && (cVar4 == '\0')) {
LAB_004ecce1:
          bVar9 = false;
        }
        else {
          bVar9 = true;
        }
      }
      else {
        if ((bVar9 == false) || (cVar4 == '\0')) goto LAB_004ecce1;
        bVar9 = true;
      }
      iVar8 = iVar8 + 1;
      iVar6 = iVar5;
    } while (iVar8 < iVar5);
  }
LAB_004eccec:
  return CONCAT31((int3)((uint)iVar6 >> 8),bVar9);
}
#endif
