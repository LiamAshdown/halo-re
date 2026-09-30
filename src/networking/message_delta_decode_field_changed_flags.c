// message_delta_decode_field_changed_flags  (Ghidra: FUN_004ed070; named per this rewrite)
// address 0x4ed070, size 341 bytes
// name confidence: 0.4   rewrite confidence: 0.45
// evidence: objdump -d -M intel bin/halo.exe @0x4ed070: reads a per-field "changed" byte array
// into the decode context's slot-1 region (context+4, up to 0x40 bytes -- matching
// types/networking.h's note that "context slot 1 onward is the field-binding list handed to
// message_delta_read_changed_subfields as (context + 1)"), then decodes the definition's static
// fields into context[0x11] (a remote_player_update_header*, per the same header note) via
// message_delta_decode_static_fields, and finally copies the changed-flags buffer into the
// shared scratch array at 0x006b89c0.
// register convention: the decode context as the recognized parameter (param_1); definition
// bounds/offsets computed from *context (message_delta_decode_state*).
// UNSURE: why the baseline (incremental == 0) path checks room for definition->statics's own
// size_bits while the incremental path checks definition->header_and_static_bits instead -- both
// are transcribed exactly as decompiled/disassembled.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "fn_memory.h"
#include "fn_networking.h"

extern message_delta_definition *message_delta_definitions[56]; // 0x0065d440
extern uint8_t message_delta_field_changed_flags[0x40];         // 0x006b89c0, shared scratch copy


// Decodes the per-field "changed" flags for a message's array/compound field: on a baseline
// (non-incremental) message every field is simply marked changed; otherwise each flag is read
// bit by bit. Either way, if the definition has static fields it decodes them immediately after
// into context[0x11], and always leaves a full 0x40-byte changed-flags snapshot in the shared
// scratch buffer. Returns the number of bits consumed, or 0 on failure.
int32_t message_delta_decode_field_changed_flags(void **context)
{
    message_delta_decode_state *state;
    message_delta_definition *definition;
    bit_stream *stream;
    int32_t field_count;
    uint8_t *changed_flags;
    int32_t bits_consumed;
    uint8_t ok;
    int32_t i;

    state = (message_delta_decode_state *)context[0];
    definition = message_delta_definitions[state->message_type];
    field_count = definition->field_count;
    stream = state->stream;
    changed_flags = (uint8_t *)context + 4;
    bits_consumed = 0;

    if (state->incremental == 0) {
        if (definition->statics->count < 1) {
            ok = 1;
        } else {
            uint32_t last = (uint32_t)(stream->bit_cursor + stream->byte_cursor * 8) - 1 +
                             definition->statics->size_bits;
            ok = (uint8_t)!(last < stream->first_bit || stream->last_bit < last);
        }
        if (ok) {
            for (i = 0; i < field_count; i++) {
                changed_flags[i] = 1;
            }
        }
    } else {
        uint32_t last = (uint32_t)(stream->bit_cursor + stream->byte_cursor * 8) - 1 +
                         definition->header_and_static_bits;
        ok = (uint8_t)!(last < stream->first_bit || stream->last_bit < last);
        if (ok) {
            for (i = 0; i < field_count; i++) {
                if (bit_stream_read_bit(&changed_flags[i], stream) != 1) {
                    ok = 0;
                    break;
                }
                bits_consumed = bits_consumed + 1;
            }
        }
    }

    if (ok && 0 < definition->statics->count) {
        int32_t static_bits = message_delta_decode_static_fields(
            state->message_type, stream, (int32_t)(int32_t)context[0x11]);
        if (static_bits < 1) {
            ok = 0;
        } else {
            bits_consumed = bits_consumed + static_bits;
        }
    }

    for (i = field_count; i < 0x40; i++) {
        changed_flags[i] = 0;
    }
    if (!ok) {
        bits_consumed = 0;
    }
    for (i = 0; i < 0x40; i++) {
        message_delta_field_changed_flags[i] = changed_flags[i];
    }
    return bits_consumed;
}

#if 0
Original Ghidra decompilation (0x4ed070):

int FUN_004ed070(int *param_1)

{
  undefined *puVar1;
  int iVar2;
  uint uVar3;
  uint uVar4;
  char cVar5;
  int *piVar6;
  int iVar7;
  int *piVar8;
  undefined4 *puVar9;
  int *piVar10;

  piVar6 = param_1;
  piVar10 = (int *)*param_1;
  puVar1 = (&PTR_DAT_0065d440)[piVar10[1]];
  uVar4 = *(uint *)(puVar1 + 0x20);
  param_1 = (int *)0x0;
  if (*piVar10 == 0) {
    if (**(int **)(puVar1 + 0x1c) < 1) {
      cVar5 = '\x01';
    }
    else {
      iVar7 = piVar10[4];
      uVar3 = *(int *)(iVar7 + 0x10) + *(int *)(iVar7 + 0xc) * 8 + -1 +
              *(int *)(*(int *)(puVar1 + 0x1c) + 4);
      if ((uVar3 < *(uint *)(iVar7 + 8)) || (*(uint *)(iVar7 + 0x14) < uVar3)) {
        cVar5 = '\0';
      }
      else {
        cVar5 = '\x01';
      }
      if (cVar5 == '\0') goto LAB_004ed185;
    }
    piVar8 = piVar6;
    for (uVar3 = uVar4 >> 2; piVar8 = piVar8 + 1, uVar3 != 0; uVar3 = uVar3 - 1) {
      *piVar8 = 0x1010101;
    }
    for (uVar3 = uVar4 & 3; uVar3 != 0; uVar3 = uVar3 - 1) {
      *(undefined1 *)piVar8 = 1;
      piVar8 = (int *)((int)piVar8 + 1);
    }
  }
  else {
    iVar7 = piVar10[4];
    uVar3 = *(int *)(iVar7 + 0x10) + *(int *)(iVar7 + 0xc) * 8 + -1 + *(int *)(puVar1 + 4);
    if ((uVar3 < *(uint *)(iVar7 + 8)) || (*(uint *)(iVar7 + 0x14) < uVar3)) {
      cVar5 = '\0';
    }
    else {
      cVar5 = '\x01';
    }
    if (cVar5 == '\0') goto LAB_004ed185;
    iVar7 = 0;
    if (0 < (int)uVar4) {
      do {
        iVar2 = bit_stream_read_bit((undefined1 *)(iVar7 + 4 + (int)piVar6));
        cVar5 = '\x01' - (iVar2 != 1);
        if (cVar5 == '\0') goto LAB_004ed185;
        param_1 = (int *)((int)param_1 + 1);
        iVar7 = iVar7 + 1;
      } while (iVar7 < (int)uVar4);
    }
  }
  if (0 < **(int **)((&PTR_DAT_0065d440)[piVar10[1]] + 0x1c)) {
    iVar7 = FUN_004ed290(piVar10[4],piVar6[0x11]);
    if (iVar7 < 1) {
      cVar5 = '\0';
    }
    else {
      param_1 = (int *)((int)param_1 + iVar7);
      cVar5 = '\x01';
    }
  }
LAB_004ed185:
  puVar9 = (undefined4 *)(uVar4 + 4 + (int)piVar6);
  for (uVar3 = 0x40 - uVar4 >> 2; uVar3 != 0; uVar3 = uVar3 - 1) {
    *puVar9 = 0;
    puVar9 = puVar9 + 1;
  }
  for (uVar4 = 0x40 - uVar4 & 3; uVar4 != 0; uVar4 = uVar4 - 1) {
    *(undefined1 *)puVar9 = 0;
    puVar9 = (undefined4 *)((int)puVar9 + 1);
  }
  if (cVar5 == '\0') {
    param_1 = (int *)0x0;
  }
  piVar10 = &DAT_006b89c0;
  for (iVar7 = 0x10; piVar6 = piVar6 + 1, iVar7 != 0; iVar7 = iVar7 + -1) {
    *piVar10 = *piVar6;
    piVar10 = piVar10 + 1;
  }
  return (int)param_1;
}
#endif
