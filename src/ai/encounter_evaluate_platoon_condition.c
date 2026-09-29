// encounter_evaluate_platoon_condition  (Ghidra: encounter_evaluate_platoon_condition, renamed)
// address 0x439f20, size 285 bytes
// name confidence: 0.35  rewrite confidence: 0.85 (VERIFIED 2026-09-28 against objdump 0x439f20..0x43a03c (jump table 0x43a040 codes 0..9; 0.75/0.5/0.25).)
// evidence: types/ai.h encounter (platoon_count +0x0a, first_platoon +0x08, confirmed) /
//   encounter_platoon_state (member_count +0x04, confirmed by this function reading it right
//   after the (first_platoon+index)*0x10 address computation). phase-4 summary "evaluates
//   one of several scripted morale/count-based conditions used to decide a squad type's
//   behavior flag"; called (with no visible arguments -- pure register forwarding) from
//   encounter_update_platoon_defending_flag @0x4393b0 (this rewrite).
// register convention: EAX -> encounter_index, EDI -> condition (a small caller-owned
//   2-field record: [0] condition code 1..9, [1] platoon index or an out-of-range sentinel
//   meaning "use the encounter's own totals instead of one platoon's").
//   // blam-cc: EAX -> encounter_index, EDI -> condition
//
// UNSURE: every return path in Ghidra's decompile is wrapped in CONCAT31/CONCAT22 byte
// packing with garbage upper bytes (the same "& 0xffffff00 masks the low byte to a clean
// 0/1, upper bytes are leftover register content" pattern seen elsewhere in this cluster).
// Every call site this module has for this function truncates the result to a `byte`/`char`,
// so only the low byte is ever observed; this rewrite returns that clean uint8_t directly
// rather than reproducing the CONCAT noise. encounter.unknown_34 and
// encounter_platoon_state.unknown_0c are declared int32_t in types/ai.h but are read here as
// float (reinterpreted through a pointer cast, not converted) -- another same-offset
// type disagreement between functions, not resolved in the header. The 2-field condition
// record's type is not established elsewhere in this module; declared locally below with a
// TYPES-GAP marker.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"

// TYPES-GAP: not established anywhere else in this module; a 2-field caller-owned record
// (EDI) naming the scripted condition to test and, optionally, which platoon it applies to.
extern data_array *encounter_data;  // 0x008802c8
extern encounter_platoon_state *encounter_platoon_states; // 0x008802c4

// blam-cc: EAX -> encounter_index, EDI -> condition
uint8_t encounter_evaluate_platoon_condition(datum_index encounter_index, const ai_platoon_condition *condition)
{
    encounter *self;
    encounter_platoon_state *platoon_state;
    int16_t platoon_index;
    int16_t count_a; // sVar7
    int16_t count_b; // sVar8
    float threshold; // fVar6

    self = (encounter *)((uint8_t *)encounter_data->data + (encounter_index & 0xffff) * sizeof(encounter));

    platoon_index = condition->platoon_index;
    if ((platoon_index < 0) || (self->platoon_count <= platoon_index)) {
        count_b = self->member_count;
        count_a = self->weighted_actor_count;
        threshold = *(float *)&self->average_vitality;
    } else {
        platoon_state = &encounter_platoon_states[(int16_t)(self->first_platoon + platoon_index)];
        count_b = platoon_state->member_count;
        count_a = platoon_state->weighted_actor_count;
        threshold = *(float *)&platoon_state->average_vitality;
    }

    if (count_b <= 0) {
        return 0;
    }

    switch (condition->code) {
    case 1:
        return threshold < 0.75f;
    case 2:
        return threshold < 0.5f;
    case 3:
        return threshold < 0.25f;
    case 4:
        return count_a < count_b;
    case 5:
        return (((int32_t)count_a << 2) / 3) <= count_b;
    case 6:
        return (count_a * 2 - (int32_t)count_b == 0) || (count_a * 2 < (int32_t)count_b);
    case 7:
        return count_a * 4 <= (int32_t)count_b;
    case 8:
        return count_a < 2;
    case 9:
        return count_a == 0;
    default:
        return 0;
    }
}

#if 0
// ---- original Ghidra decompilation (FUN_00439f20 @ 0x439f20) ----
uint FUN_00439f20(void)

{
  short sVar1;
  uint in_EAX;
  uint uVar2;
  int3 iVar3;
  int iVar5;
  float fVar6;
  short sVar7;
  short sVar8;
  short *unaff_EDI;
  short sVar4;

  uVar2 = (in_EAX & 0xffff) * 0x6c + *(int *)(DAT_008802c8 + 0x34);
  sVar8 = unaff_EDI[1];
  if ((sVar8 < 0) || (*(short *)(uVar2 + 10) <= sVar8)) {
    sVar8 = *(short *)(uVar2 + 0x18);
    sVar7 = *(short *)(uVar2 + 0x2a);
    fVar6 = *(float *)(uVar2 + 0x34);
  }
  else {
    iVar5 = (short)(*(short *)(uVar2 + 8) + sVar8) * 0x10;
    sVar8 = *(short *)(iVar5 + 4 + DAT_008802c4);
    iVar5 = iVar5 + DAT_008802c4;
    sVar7 = *(short *)(iVar5 + 6);
    fVar6 = *(float *)(iVar5 + 0xc);
  }
  if (0 < sVar8) {
    sVar1 = *unaff_EDI;
    uVar2 = (uint)sVar1;
    if (uVar2 < 10) {
      sVar4 = sVar1 >> 0xf;
      iVar3 = (int3)(char)((ushort)sVar1 >> 8);
      switch(uVar2) {
      case 1:
        uVar2 = CONCAT22(sVar4,(ushort)(fVar6 < 0.75) << 8 | (ushort)NAN(fVar6) << 10 |
                               (ushort)(fVar6 == 0.75) << 0xe);
        if (fVar6 < 0.75) {
          return CONCAT31((int3)(uVar2 >> 8),1);
        }
        break;
      case 2:
        uVar2 = CONCAT22(sVar4,(ushort)(fVar6 < 0.5) << 8 | (ushort)NAN(fVar6) << 10 |
                               (ushort)(fVar6 == 0.5) << 0xe);
        if (fVar6 < 0.5) {
          return CONCAT31((int3)(uVar2 >> 8),1);
        }
        break;
      case 3:
        uVar2 = CONCAT22(sVar4,(ushort)(fVar6 < 0.25) << 8 | (ushort)NAN(fVar6) << 10 |
                               (ushort)(fVar6 == 0.25) << 0xe);
        if (fVar6 < 0.25) {
          return CONCAT31((int3)(uVar2 >> 8),1);
        }
        break;
      case 4:
        return CONCAT31(iVar3,sVar7 < sVar8);
      case 5:
        iVar5 = ((int)sVar7 << 2) / 3;
        return CONCAT31((int3)((uint)iVar5 >> 8),iVar5 <= sVar8);
      case 6:
        return CONCAT31((int3)(char)((ushort)sVar8 >> 8),
                        sVar7 * 2 - (int)sVar8 == 0 || sVar7 * 2 < (int)sVar8);
      case 7:
        return CONCAT31(iVar3,sVar7 * 4 <= (int)sVar8);
      case 8:
        return CONCAT31(iVar3,sVar7 < 2);
      case 9:
        return CONCAT31(iVar3,sVar7 == 0);
      }
    }
  }
  return uVar2 & 0xffffff00;
}
#endif
