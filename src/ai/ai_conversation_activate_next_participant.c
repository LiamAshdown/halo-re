// ai_conversation_activate_next_participant  (Ghidra: ai_conversation_activate_next_participant; named for this rewrite)
// address 0x431d10, size 352 bytes
// name confidence: 0.4   rewrite confidence: 0.6
// evidence: out/phase4/ai_types_notes.md's misattribution table places this function on
// Scenario.ai_conversations, not squads: "activates the next resolved participant". Its
// only caller is ai_conversation_update (0x430a70, already rewritten), which calls it with
// the instance handle still live in EAX from the surrounding loop -- Ghidra's decompile of
// that call site shows no visible argument, but objdump (bin/halo.exe 0x431d10..0x431e70)
// confirms `mov esi,eax` is the very first instruction, so EAX is a genuine parameter.
// types/ai.h ai_conversation.current_line_index (current line index), .participant_mask,
// .participant_actor[8], .speaker_participant/_50/_54/_58/_60/_61/_62/_63 all match one-to-one;
// ScenarioAIConversation.participants/.lines (TagReflexive at +0x50/+0x5c) and
// ScenarioAIConversationLine's participant/addressee/addressee_participant/line_delay_time
// fields match exactly. The tail write to line_variant_tag_id, which Ghidra rendered as raw pointer
// arithmetic, resolves cleanly once line->variant_1..variant_6 (six contiguous 0x10-byte
// TagDependency slots) are read as an array: the selector is a per-participant int16 stored
// at ai_conversation+0x18 (declared uint32_t in the header; treated here as an int16[2]
// array, one slot per participant up to the 4 that fit), and the result is that variant's
// TagID. The ticks_per_second (0x00672ac8, 30.0) multiply on line_delay_time is also
// confirmed by objdump (`fld [edi+0xc]; fmul ds:0x672ac8; call 0x6391b4`).
// SIGNATURE-CONFLICT: src/ai/ai_conversation_update.c still declares this as
// `uint8_t ai_conversation_activate_next_participant(void)`; it should take the instance handle in EAX. Left for review.
// register convention: EAX -> instance_handle (the ai_conversation datum index; confirmed
// by objdump).
//   // blam-cc: EAX -> instance_handle
//
// UNSURE: ai_conversation.unknown_10 has no established meaning
// beyond "an object/unit reference the addressee logic resolves"; kept as raw header fields.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "ai.h"

extern data_array *ai_conversation_data; // 0x008802d4
extern data_array *actor_data;           // 0x00880360
extern Scenario *global_scenario;        // 0x00746f8c
extern float ticks_per_second;           // 0x00672ac8, 30.0

extern int32_t __ftol(double x); // FISTP-based float-to-int truncation

// blam-cc: EAX -> instance_handle
// Activates the participant referenced by the instance's current line (ai_conversation
// current_line_index), once that participant has already been resolved (its bit set in
// participant_mask). Records the participant's actor/unit on the instance, resolves the
// line's addressee (if any) to a unit as well, picks up the line's sound-variant TagID and
// delay (converted to ticks), and clears the per-line completion flags. Returns 0 without
// effect if the line's participant index is out of range or not yet resolved.
uint8_t ai_conversation_activate_next_participant(datum_index instance_handle)
{
    ai_conversation *instance = &((ai_conversation *)ai_conversation_data->data)[instance_handle & 0xffff];
    ScenarioAIConversation *definition =
        &((ScenarioAIConversation *)global_scenario->ai_conversations.pointer)[instance->definition_index];
    ScenarioAIConversationLine *line =
        &((ScenarioAIConversationLine *)definition->lines.pointer)[instance->current_line_index];
    int16_t participant_index = line->participant;

    if (participant_index < 0 || participant_index >= definition->participants.count ||
        (instance->participant_mask & (1u << (participant_index & 0x1f))) == 0) {
        return 0;
    }

    participant_index = line->participant; // reloaded, matches the original's redundant re-read
    {
        ScenarioAIConversationParticipant *participants =
            (ScenarioAIConversationParticipant *)definition->participants.pointer;
        datum_index participant_actor_handle = instance->participant_actor[participant_index];

        instance->speaker_participant = participant_index;

        if (participant_actor_handle == (datum_index)k_datum_index_none) {
            instance->speaker_actor = (uint32_t)k_datum_index_none;
            instance->speaker_unit = (uint32_t)k_datum_index_none;
            instance->addressee_unit = (uint32_t)k_datum_index_none;
            instance->unknown_60 = 1;
        } else {
            actor *participant_actor = &((actor *)actor_data->data)[participant_actor_handle & 0xffff];
            int16_t selection_type;

            instance->speaker_actor = participant_actor_handle;
            instance->speaker_unit = participant_actor->unit_index;
            instance->addressee_unit = (uint32_t)k_datum_index_none;

            if (line->addressee == 1) {
                instance->addressee_unit = instance->unknown_10;
            } else if (line->addressee == 2 && line->addressee_participant >= 0 &&
                       line->addressee_participant < definition->participants.count) {
                datum_index addressee_actor_handle = instance->participant_actor[line->addressee_participant];
                if (addressee_actor_handle != (datum_index)k_datum_index_none) {
                    instance->addressee_unit = ((actor *)actor_data->data)[addressee_actor_handle & 0xffff].unit_index;
                }
            }

            selection_type = participants[participant_index].selection_type;
            instance->unknown_60 = (selection_type == 6 || selection_type == 7) ? 1 : 0;
        }

        {
            TagDependency *variants = &line->variant_1; // variant_1..variant_6, six contiguous 0x10-byte slots
            int16_t variant_selector = ((int16_t *)&instance->unknown_18)[participant_index];
            instance->line_variant_tag_id = *(uint32_t *)&variants[variant_selector].tag_id;
        }

        instance->line_delay_ticks = (int16_t)__ftol((double)(line->line_delay_time * ticks_per_second));
        instance->line_flags = line->flags;
        instance->unknown_63 = 0;
        instance->unknown_62 = 0;
        instance->unknown_61 = 0;
    }

    return 1;
}

#if 0
Original Ghidra decompilation (0x431d10):

undefined4 FUN_00431d10(void)

{
  short sVar1;
  short sVar2;
  uint uVar3;
  int iVar4;
  undefined1 uVar5;
  undefined2 uVar6;
  uint in_EAX;
  undefined4 uVar7;
  int iVar8;
  int iVar9;
  int iVar10;
  int iVar11;
  undefined2 *puVar12;

  iVar9 = (in_EAX & 0xffff) * 100;
  iVar10 = iVar9 + *(int *)(DAT_008802d4 + 0x34);
  iVar8 = *(short *)(iVar9 + 2 + *(int *)(DAT_008802d4 + 0x34)) * 0x74;
  iVar9 = *(int *)(iVar8 + 0x60 + *(int *)(global_scenario + 0x46c));
  iVar11 = *(short *)(iVar10 + 0x48) * 0x7c;
  sVar1 = *(short *)(iVar11 + 2 + iVar9);
  iVar8 = iVar8 + *(int *)(global_scenario + 0x46c);
  puVar12 = (undefined2 *)(iVar11 + iVar9);
  uVar7 = 0;
  if (((-1 < sVar1) && ((int)sVar1 < *(int *)(iVar8 + 0x50))) &&
     ((*(uint *)(iVar10 + 0x14) & 1 << ((byte)sVar1 & 0x1f)) != 0)) {
    sVar1 = puVar12[1];
    iVar9 = *(int *)(iVar8 + 0x54);
    uVar3 = *(uint *)(iVar10 + 0x28 + sVar1 * 4);
    *(short *)(iVar10 + 0x4a) = sVar1;
    iVar11 = DAT_00880360;
    if (uVar3 == 0xffffffff) {
      *(undefined4 *)(iVar10 + 0x50) = 0xffffffff;
      *(undefined4 *)(iVar10 + 0x54) = 0xffffffff;
      *(undefined4 *)(iVar10 + 0x58) = 0xffffffff;
      *(undefined1 *)(iVar10 + 0x60) = 1;
    }
    else {
      iVar4 = *(int *)(DAT_00880360 + 0x34);
      *(uint *)(iVar10 + 0x50) = uVar3;
      *(undefined4 *)(iVar10 + 0x54) = *(undefined4 *)((uVar3 & 0xffff) * 0x724 + iVar4 + 0x18);
      *(undefined4 *)(iVar10 + 0x58) = 0xffffffff;
      if (puVar12[2] == 1) {
        *(undefined4 *)(iVar10 + 0x58) = *(undefined4 *)(iVar10 + 0x10);
      }
      else if (((puVar12[2] == 2) && (sVar2 = puVar12[3], -1 < sVar2)) &&
              (((int)sVar2 < *(int *)(iVar8 + 0x50) &&
               (uVar3 = *(uint *)(iVar10 + 0x28 + sVar2 * 4), uVar3 != 0xffffffff)))) {
        *(undefined4 *)(iVar10 + 0x58) =
             *(undefined4 *)((uVar3 & 0xffff) * 0x724 + 0x18 + *(int *)(iVar11 + 0x34));
      }
      sVar1 = *(short *)(sVar1 * 0x54 + iVar9 + 4);
      if ((sVar1 == 6) || (sVar1 == 7)) {
        uVar5 = 1;
      }
      else {
        uVar5 = 0;
      }
      *(undefined1 *)(iVar10 + 0x60) = uVar5;
    }
    *(undefined4 *)(iVar10 + 0x5c) =
         *(undefined4 *)(puVar12 + *(short *)(iVar10 + 0x18 + (short)puVar12[1] * 2) * 8 + 0x14);
    uVar6 = __ftol();
    *(undefined2 *)(iVar10 + 0x4c) = uVar6;
    *(undefined2 *)(iVar10 + 0x4e) = *puVar12;
    *(undefined1 *)(iVar10 + 99) = 0;
    *(undefined1 *)(iVar10 + 0x62) = 0;
    *(undefined1 *)(iVar10 + 0x61) = 0;
    uVar7 = 1;
  }
  return uVar7;
}

Real disassembly (0x431d10-0x431e6f), used to confirm the EAX parameter, the
ticks_per_second multiply, and the variant-array indexing:

00431d10: push   ebp
00431d11: push   esi
00431d12: mov    esi,eax                    ; instance_handle
00431d14: mov    eax,ds:0x8802d4
00431d19: mov    ecx,[eax+0x34]
00431d1c: and    esi,0xffff
00431d22: imul   esi,esi,0x64
00431d44: imul   edi,edi,0x7c               ; line index * sizeof(ScenarioAIConversationLine)
00431e31: movsx  ecx,WORD PTR [edi+0x2]
00431e35: movsx  edx,WORD PTR [esi+ecx*2+0x18]
00431e3a: shl    edx,0x4                    ; variant_selector * sizeof(TagDependency)
00431e3d: mov    eax,[edx+edi+0x28]         ; line->variant_1[selector].tag_id
00431e44: fld    DWORD PTR [edi+0xc]        ; line->line_delay_time
00431e47: fmul   DWORD PTR ds:0x672ac8      ; ticks_per_second (30.0)
00431e4d: call   0x6391b4                   ; __ftol
#endif
