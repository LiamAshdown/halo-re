// ai_conversation_current_line_is_ready  (Ghidra: ai_conversation_current_line_is_ready; named for this rewrite)
// address 0x431e70, size 646 bytes
// name confidence: 0.4   rewrite confidence: 0.9
// evidence: out/phase4/ai_types_notes.md's misattribution table: "whether the activated
// squad member is ready to be placed" is actually "whether the participant is ready to
// speak". Only caller is ai_conversation_update (0x430a70, already rewritten), which passes
// the instance handle through in EAX exactly like ai_conversation_activate_next_participant
// (0x431d10, this batch; confirmed by objdump, `mov esi,eax` is the first instruction here
// too). types/ai.h ai_conversation fields all match one-to-one. types/units.h unit_speech
// (0x30 bytes, "0x560f20 block-moves 0xc dwords of this into unit 0x388") is the local
// stack block this function builds byte-for-byte before calling unit_commit_speech -- confirmed
// by objdump (bin/halo.exe 0x431f60..0x432045): priority=6, scream_type=-1, sound_tag =
// ai_conversation.line_variant_tag_id, unknown_14=-1, ai_line_index=-1, weighted_actor_count=-1 all line up
// with the named fields, though this call also writes non-zero values into three dwords of
// the struct's documented-always-zero tail (see TYPES-GAP below). objdump also confirms
// unit_commit_speech is called with ECX -> &request and EAX still holding the unit index from the
// mov two instructions earlier (matching the EAX -> object_index role already declared for
// this callee in ai_communication_play_event_line.c); EDX carries only a stale value here
// and is not treated as a real argument.
// register convention: EAX -> instance_handle (confirmed by objdump).
//   // blam-cc: EAX -> instance_handle
//
// TYPES-GAP: the request this function builds writes real values (1, 1, and the unit index)
// into unit_speech's unknown_1c tail, which types/units.h documents as always zero for the
// other constructor (0x561030). Reproduced exactly via a local struct definition that
// extends unit_speech's tail with three named fields, flagged UNSURE rather than folded
// into units.h.
// UNSURE: actor.mode_data.raw sub-fields at +0x4/+0x5/+0xc (absolute actor+0xa0/+0xa1/+0xa8)
// have no established meaning; kept as raw offsets into mode_data. object+0x388 (unit_data
// current_speech.sound_tag, read back here as an int16 category compared against 6) and the
// DAT_00725204 tick threshold are likewise not otherwise attributed.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"
#include "units.h"
#include "game.h"
#include "ai.h"
#include <string.h>

// TYPES-GAP: mirrors types/units.h unit_speech (0x30 bytes) but names the tail three fields
// this call site actually writes (documented as always-zero padding for the other builder).
extern data_array *ai_conversation_data; // 0x008802d4
extern data_array *actor_data;           // 0x00880360
extern data_array *object_data;          // 0x008603b0
extern Scenario *global_scenario;        // 0x00746f8c
extern game_time_globals *game_time;     // 0x006f1d6c
extern int32_t ai_communication_quiet_until_tick; // 0x00725204

extern void sound_impulse_start(datum_index object_index, datum_index definition_index, float scale); // 0x543e10, EAX, ECX, stack
extern int32_t sound_impulse_time(datum_index sound_tag_handle); // 0x543fc0, ECX
extern int32_t unit_animation_change_priority_check(uint32_t unit_index, uint8_t follow_fallback, int16_t requested_priority,
    uint8_t allow_repeat, uint32_t *out_unknown_3f0, int16_t *dialogue_index, int32_t *chain_value); // 0x560d00, EAX, DL, stack
extern int32_t unit_commit_speech(uint32_t unit_index, const void *source, int16_t mode); // 0x560f20, EAX, ECX, DX

// REWRITTEN from objdump 0x431e70..0x4320f5. EAX: the conversation. Returns its +0x63 (the current line is done).
//   Start (once, +0x61): unless a participant (all with flag 0x20, the +0x50 one with 0x10) is mid-conversation
//   (mode 12 with +0xa8 and neither +0xa0 / +0xa1), the communication quiet period (0x725204) is over and there is a
//   sound (+0x5c): the speaker unit (+0x54, unless +0x60) claims speech priority 6 (0x560d00: 1 = wait, <= 0 = give
//   up) and commits a 0x30-byte speech record (0x560f20, mode = that result, listener +0x58); without a speaker the
//   sound just plays (0x543e10). Then: done speaking (+0x62: the unit's +0x388 no longer 6, or the sound finished),
//   the post-line delay (+0x4c), and with +0x4e bit 3 the +8 / +9 handshake. The draft called every helper without
//   operands.
uint8_t ai_conversation_current_line_is_ready(datum_index instance_handle)
{
    uint8_t *inst = (uint8_t *)ai_conversation_data->data + (instance_handle & 0xffff) * 0x64;     // esi
    uint8_t *definition = *(uint8_t **)((uint8_t *)global_scenario + 0x46c) + *(int16_t *)(inst + 0x2) * 0x74;

    if (inst[0x63]) {
        return inst[0x63];
    }
    if (!inst[0x61]) {
        uint8_t blocked = 0;                                                                        // [esp+0x13]
        datum_index sound = *(datum_index *)(inst + 0x5c);

        if (sound != k_datum_index_none) {
            uint16_t flags = *(uint16_t *)(inst + 0x4e);

            if (flags & 0x30) {
                int16_t i;

                for (i = 0; (int32_t)i < *(int32_t *)(definition + 0x50); i++) {
                    datum_index actor_index = *(datum_index *)(inst + 0x28 + i * 4);
                    uint8_t *a;

                    if (actor_index == k_datum_index_none) {
                        continue;
                    }
                    if (!(flags & 0x20) && !((flags & 0x10) && actor_index == *(datum_index *)(inst + 0x50))) {
                        continue;
                    }
                    a = (uint8_t *)actor_data->data + (actor_index & 0xffff) * 0x724;
                    if (((actor *)a)->mode == 0xc && *(datum_index *)(a + 0xa8) != k_datum_index_none &&
                        !a[0xa1] && !a[0xa0]) {
                        blocked = 1;
                    }
                }
            }
            if (game_time->game_time < ai_communication_quiet_until_tick || blocked) {
                return inst[0x63];
            }
            if (*(datum_index *)(inst + 0x54) != k_datum_index_none && !inst[0x60]) {
                int16_t dialogue_index = -1;                                                        // [esp+0x14]
                int32_t chain_value = (int32_t)sound;                                               // [esp+0x18]
                int16_t result = (int16_t)unit_animation_change_priority_check(*(datum_index *)(inst + 0x54), 0, 6, 1, 0,
                    &dialogue_index, &chain_value);

                if (result == 1) {
                    return inst[0x63];
                }
                if (result > 0) {
                    uint8_t speech[0x30];                                                           // [esp+0x1c]

                    memset(speech, 0, sizeof(speech));
                    *(int16_t *)(speech + 0x0) = 6;
                    *(int16_t *)(speech + 0x2) = -1;
                    *(datum_index *)(speech + 0x4) = sound;
                    *(datum_index *)(speech + 0x10) = *(datum_index *)(inst + 0x58);
                    *(int16_t *)(speech + 0x14) = -1;
                    *(int16_t *)(speech + 0x18) = -1;
                    *(int16_t *)(speech + 0x16) = -1;
                    *(int16_t *)(speech + 0x1c) = 1;
                    *(int16_t *)(speech + 0x1e) = 1;
                    *(datum_index *)(speech + 0x20) = *(datum_index *)(inst + 0x54);
                    *(int16_t *)(speech + 0x24) = 0;
                    unit_commit_speech(*(datum_index *)(inst + 0x54), speech, result);
                }
            } else {
                sound_impulse_start(k_datum_index_none, sound, 1.0f);
            }
        }
        inst[0x61] = 1;
        inst[0x5] = 1;
    }
    // 0x432046
    if (!inst[0x61]) {
        return inst[0x63];
    }
    if (!inst[0x62]) {
        uint8_t done;

        if (*(datum_index *)(inst + 0x54) == k_datum_index_none) {
            done = !(*(datum_index *)(inst + 0x5c) != k_datum_index_none &&
                     sound_impulse_time(*(datum_index *)(inst + 0x5c)) != 0);
        } else {
            uint8_t *unit = (uint8_t *)((object_header *)object_data->data)[*(datum_index *)(inst + 0x54) & 0xffff].data;

            done = ((unit_object *)unit)->unit.current_speech.priority != 6;
        }
        inst[0x62] = done;
        if (!done) {
            return inst[0x63];
        }
    }
    if (*(int16_t *)(inst + 0x4c) > 0) {
        *(int16_t *)(inst + 0x4c) = (int16_t)(*(int16_t *)(inst + 0x4c) - 1);
        return inst[0x63];
    }
    inst[0x63] = 1;
    if (*(uint16_t *)(inst + 0x4e) & 0x8) {
        if (!inst[0x8]) {
            inst[0x8] = 1;
            inst[0x9] = 0;
        }
        if (inst[0x9]) {
            inst[0x8] = 0;
            return inst[0x63];
        }
        inst[0x63] = 0;
    }
    return inst[0x63];
}

#if 0
Original Ghidra decompilation (0x431e70):

uint FUN_00431e70(void)

{
  ushort uVar1;
  undefined1 uVar2;
  uint in_EAX;
  uint uVar3;
  int iVar4;
  short sVar5;
  int iVar6;
  int iVar7;
  undefined4 *puVar8;
  char local_39;
  uint local_38;
  int local_34;
  undefined4 local_30;
  undefined4 local_2c;
  undefined4 local_20;
  undefined2 local_1c;
  undefined2 local_1a;
  undefined2 local_18;
  undefined2 local_14;
  undefined2 local_12;
  undefined4 local_10;
  undefined2 local_c;

  iVar6 = (in_EAX & 0xffff) * 100;
  iVar4 = *(int *)(DAT_008802d4 + 0x34);
  iVar7 = iVar6 + iVar4;
  uVar3 = *(short *)(iVar6 + 2 + iVar4) * 0x74 + *(int *)(global_scenario + 0x46c);
  if (*(char *)(iVar6 + 99 + iVar4) != '\0') goto LAB_004320ed;
  if (*(char *)(iVar7 + 0x61) == '\0') {
    local_39 = '\0';
    if (*(int *)(iVar7 + 0x5c) != -1) {
      uVar1 = *(ushort *)(iVar7 + 0x4e);
      local_38 = uVar1 & 0x10;
      if (((uVar1 & 0x10) != 0) || ((uVar1 & 0x20) != 0)) {
        local_34 = *(int *)(uVar3 + 0x50);
        sVar5 = 0;
        if (0 < local_34) {
          iVar4 = 0;
          do {
            uVar3 = *(uint *)(iVar7 + 0x28 + iVar4 * 4);
            if (((uVar3 != 0xffffffff) &&
                (((iVar4 = (uVar3 & 0xffff) * 0x724 + *(int *)(DAT_00880360 + 0x34),
                  (uVar1 & 0x20) != 0 ||
                  (((short)local_38 != 0 && (uVar3 == *(uint *)(iVar7 + 0x50))))) &&
                 (*(short *)(iVar4 + 0x6c) == 0xc)))) &&
               (((*(int *)(iVar4 + 0xa8) != -1 && (*(char *)(iVar4 + 0xa1) == '\0')) &&
                (*(char *)(iVar4 + 0xa0) == '\0')))) {
              local_39 = '\x01';
            }
            sVar5 = sVar5 + 1;
            iVar4 = (int)sVar5;
          } while (iVar4 < local_34);
        }
      }
      uVar3 = *(uint *)(DAT_006f1d6c + 0xc);
      if (((int)uVar3 < DAT_00725204) ||
         (uVar3 = CONCAT31((int3)(uVar3 >> 8),local_39), local_39 != '\0')) goto LAB_00432046;
      if ((*(int *)(iVar7 + 0x54) == -1) || (*(char *)(iVar7 + 0x60) != '\0')) {
        uVar3 = FUN_00543e10(0x3f800000);
      }
      else {
        local_34 = *(int *)(iVar7 + 0x5c);
        local_38 = 0xffffffff;
        uVar3 = FUN_00560d00(6,1,0,&local_38,&local_34);
        if ((short)uVar3 == 1) goto LAB_00432046;
        if (0 < (short)uVar3) {
          puVar8 = &local_30;
          for (iVar4 = 0xc; iVar4 != 0; iVar4 = iVar4 + -1) {
            *puVar8 = 0;
            puVar8 = puVar8 + 1;
          }
          local_2c = *(undefined4 *)(iVar7 + 0x5c);
          local_20 = *(undefined4 *)(iVar7 + 0x58);
          local_10 = *(undefined4 *)(iVar7 + 0x54);
          local_30._0_2_ = 6;
          local_30._2_2_ = 0xffff;
          local_1c = 0xffff;
          local_18 = 0xffff;
          local_1a = 0xffff;
          local_14 = 1;
          local_12 = 1;
          local_c = 0;
          uVar3 = FUN_00560f20();
        }
      }
    }
    *(undefined1 *)(iVar7 + 0x61) = 1;
    *(undefined1 *)(iVar7 + 5) = 1;
  }
LAB_00432046:
  uVar3 = CONCAT31((int3)(uVar3 >> 8),*(char *)(iVar7 + 0x61));
  if (*(char *)(iVar7 + 0x61) != '\0') {
    if (*(char *)(iVar7 + 0x62) == '\0') {
      if (*(uint *)(iVar7 + 0x54) == 0xffffffff) {
        if ((*(int *)(iVar7 + 0x5c) == -1) || (iVar4 = FUN_00543fc0(), iVar4 == 0)) {
          uVar3 = 1;
        }
        else {
          uVar3 = 0;
        }
      }
      else {
        uVar3 = CONCAT31((int3)((uint)DAT_008603b0 >> 8),
                         *(short *)(*(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 +
                                            (*(uint *)(iVar7 + 0x54) & 0xffff) * 0xc) + 0x388) != 6)
        ;
      }
      *(char *)(iVar7 + 0x62) = (char)uVar3;
      if ((char)uVar3 == '\0') goto LAB_004320ed;
    }
    uVar1 = *(ushort *)(iVar7 + 0x4c);
    uVar3 = (uint)uVar1;
    if (0 < (short)uVar1) {
      *(short *)(iVar7 + 0x4c) = (short)(uVar3 - 1);
      return CONCAT31((int3)(uVar3 - 1 >> 8),*(undefined1 *)(iVar7 + 99));
    }
    *(undefined1 *)(iVar7 + 99) = 1;
    if ((*(byte *)(iVar7 + 0x4e) & 8) != 0) {
      uVar2 = (undefined1)(uVar1 >> 8);
      if (*(char *)(iVar7 + 8) == '\0') {
        *(undefined1 *)(iVar7 + 8) = 1;
        *(undefined1 *)(iVar7 + 9) = 0;
      }
      uVar3 = (uint)CONCAT11(uVar2,*(char *)(iVar7 + 9));
      if (*(char *)(iVar7 + 9) != '\0') {
        *(undefined1 *)(iVar7 + 8) = 0;
        return (uint)CONCAT11(uVar2,*(undefined1 *)(iVar7 + 99));
      }
      *(undefined1 *)(iVar7 + 99) = 0;
    }
  }
LAB_004320ed:
  return CONCAT31((int3)(uVar3 >> 8),*(undefined1 *)(iVar7 + 99));
}
#endif
