// unit_choose_combat_reaction_animation  (Ghidra: unit_choose_combat_reaction_animation)
// address 0x561140, size 854 bytes
// name confidence: 0.35 (phase2 candidate)   rewrite confidence: 0.85
// VERIFIED against objdump 0x561140..0x561495: EAX -> reaction_source (the damage_data; its effect tag's +0x1c6
//   category and +0x1f4 force), stack -> (unit_index, is_scripted, allow_second_tier, distance_bias). Every branch,
//   constant (0.6, 2.0, 0.2, 0.4) and the speech record match; FIXED the closing 0x42c2a0 call, which gets EDX unit,
//   BX 1 / 4 and DI 2 (was called without arguments). The register notes below predate this check.
// evidence: types/units.h unit_data.dialogue_tag_index (0x384), .swarm_actor_index (0x1f8),
//   .actor_index (0x1f4), .unknown_3ee/.unknown_3ec/.unknown_3ea/.unknown_3e8 (0x3ee/0x3ec/
//   0x3ea/0x3e8), .current_speech.priority (0x388); types/objects.h object.recent_body_damage
//   (0xf8). unit_animation_change_priority_check (0x560d00), unit_commit_speech (0x560f20).
// register convention: unit index in EAX, a tag-record pointer in EAX register alias (Ghidra's
//   "in_EAX", distinct from the recognized param_1 -- see UNSURE), threat flag in DL, extra
//   flag in CL, threshold in the stack float.
//   // blam-cc: param_1 -> unit_index, in_EAX -> reaction_source (UNSURE identity), param_2 (DL)
//   //   -> is_scripted, param_3 (CL) -> allow_second_tier, param_4 (stack float) -> distance_bias
// UNSURE: `in_EAX` is a second, unrelated pointer register Ghidra could not attach to any
//   parameter; the field it dereferences (+0x1c6) and the tag field read at a tag pointer's
//   +0x1f4 (`*(float *)(tag_data + 500)`) have no named struct in this batch's headers, so both
//   stay as raw offsets. actor_data's +0x6e field (stride 0x724) likewise has no name here.
//   ai_refresh_unit_stimulus_and_alert and random_real's exact argument-free call sites are reproduced as shown.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"
#include "units.h"

extern data_array *object_data;     // 0x008603b0
extern tag_instance *tag_instances; // 0x0087bc14
extern data_array *actor_data;      // 0x00880360, stride 0x724 (ai module)

extern real random_real(void); // 0x4019f0
extern void ai_refresh_unit_stimulus_and_alert(datum_index object_index, int16_t priority,
                                              int16_t stimulus_value); // 0x42c2a0, EDX, BX, DI
extern int32_t unit_animation_change_priority_check(uint32_t unit_index, uint8_t follow_fallback,
    int16_t requested_priority, uint8_t allow_repeat, uint32_t *out_unknown_3f0, int16_t *dialogue_index,
    int32_t *chain_value); // 0x560d00, EAX, DL, stack
extern int32_t unit_commit_speech(uint32_t unit_index, const unit_speech *source, int16_t mode); // 0x560f20

uint8_t unit_choose_combat_reaction_animation(uint32_t unit_index, const datum_index *reaction_source,
                                               uint8_t is_scripted, uint8_t allow_second_tier,
                                               float distance_bias) // blam-cc: see file header
{
    object *obj = ((object_header *)object_data->data)[unit_index & 0xffff].data;
    unit_data *unit = (unit_data *)((uint8_t *)obj + k_unit_data_offset);

    if (unit->dialogue_tag_index == (datum_index)-1) {
        return 0;
    }

    float recent_damage = obj->recent_body_damage;
    int16_t source_category = 0;
    uint8_t low_damage = recent_damage < 0.6f;
    uint8_t use_second_tier = 0;
    int32_t chain = -1;
    int32_t out_unknown_3f0 = 0;
    int16_t reaction_id;
    uint8_t success = 0;

    if (reaction_source != 0 && *reaction_source != (datum_index)-1) {
        source_category = *(int16_t *)((uint8_t *)tag_instances[*reaction_source & 0xffff].data + 0x1c6); // UNSURE
    }

    if (!is_scripted) {
        if (unit->unknown_3ee != 0) {
            return 0;
        }
        if (source_category == 1) {
            use_second_tier = 1;
            reaction_id = 9;
        } else if (low_damage) {
            if (unit->unknown_3ec != 0) {
                return 0;
            }
            if (unit->unknown_3ea > 2) {
                return 0;
            }
            if (unit->current_speech.priority != 0 && random_real() >= 0.4f) {
                return 0;
            }
            reaction_id = (recent_damage <= 0.0f) * 2 + 6;
            goto have_reaction_id;
        } else {
            use_second_tier = 1;
            reaction_id = 7;
        }
    } else {
        datum_index actor = unit->swarm_actor_index;
        if (actor == (datum_index)-1) {
            actor = unit->actor_index;
        }
        uint8_t near_tag_detection = 0;
        uint8_t past_distance_bias = 0;

        if (reaction_source != 0 && *reaction_source != (datum_index)-1) {
            near_tag_detection = *(float *)((uint8_t *)tag_instances[*reaction_source & 0xffff].data + 500) >= 2.0f; // UNSURE
        }
        if (actor == (datum_index)-1) {
            past_distance_bias = (distance_bias + 0.2f) < recent_damage;
        } else {
            past_distance_bias = *(int16_t *)((uint8_t *)actor_data->data + (actor & 0xffff) * 0x724 + 0x6e) > 2; // UNSURE
        }

        if (source_category == 1) {
            reaction_id = 0x10;
        } else if (source_category == 7) {
            reaction_id = 0x11;
        } else if (near_tag_detection) {
            reaction_id = 0x13;
        } else if (past_distance_bias) {
            reaction_id = allow_second_tier ? 0x12 : 0xf;
        } else {
            reaction_id = 0xe;
        }
        use_second_tier = 1;
        if (reaction_id != 0xe) {
            chain = 2;
            out_unknown_3f0 = (reaction_id == 0x12) ? 4 : 1;
        }
have_reaction_id:
        if (reaction_id == -1) {
            goto done;
        }
    }

    {
        int16_t priority = is_scripted ? 10 : (use_second_tier ? 7 : 2);
        int32_t out3f0 = -1;
        int32_t commit_chain = -1;
        // 0x561390: EAX unit, DL 1, stack (priority, 0, 0, &index, &chain)
        int32_t result = unit_animation_change_priority_check(unit_index, 1, priority, 0, 0, &reaction_id,
                                                                &commit_chain);
        if (result > 0) {
            unit_speech line = {0};
            line.priority = priority;
            line.scream_type = reaction_id;
            line.sound_tag = (datum_index)commit_chain;
            line.tail_ticks = 7;
            line.unknown_10 = -1;
            line.unknown_14 = -1;
            line.ai_line_index = -1;
            line.unknown_18 = -1;

            unit_commit_speech(unit_index, &line, (int16_t)result); // 0x5613af: DX = the check result
            success = 1;

            if (low_damage) {
                unit->unknown_3ea = unit->unknown_3ea + 1;
                unit->unknown_3ec = 0x1e;
                unit->unknown_3e8 = 0x16;
            } else {
                unit->unknown_3ee = 0x3c;
            }
        }
    }

done:
    if (chain != -1) {
        // 0x56146b: EDX unit, BX [esp+0x24] (1, or 4 for reaction 0x12), DI [esp+0x20] (2)
        ai_refresh_unit_stimulus_and_alert(unit_index, (int16_t)out_unknown_3f0, (int16_t)chain);
    }
    return success;
}

#if 0
Original Ghidra decompilation (0x561140):

undefined1 FUN_00561140(uint param_1,char param_2,char param_3,float param_4)

{
  float fVar1;
  bool bVar2;
  bool bVar3;
  bool bVar4;
  byte bVar5;
  short sVar6;
  uint *in_EAX;
  uint uVar7;
  int iVar8;
  short sVar9;
  short *psVar10;
  float fVar11;
  undefined1 local_47;
  int local_44;
  int local_40;
  undefined4 local_3c;
  undefined4 local_38;
  int local_34;
  short local_30;
  undefined2 local_2e;
  undefined4 local_2c;
  undefined2 local_24;
  undefined4 local_20;
  undefined4 local_1c;
  undefined4 local_18;
  undefined4 local_14;
  undefined4 local_10;
  undefined4 local_c;
  undefined4 local_8;
  undefined4 local_4;

  local_40 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (param_1 & 0xffff) * 0xc);
  local_47 = 0;
  if (*(int *)(local_40 + 900) == -1) {
    return 0;
  }
  fVar1 = *(float *)(local_40 + 0xf8);
  sVar9 = 0;
  bVar3 = *(float *)(local_40 + 0xf8) < 0.6;
  bVar5 = 0;
  local_38 = 0xffffffff;
  local_34 = 0;
  if ((in_EAX != (uint *)0x0) && (*in_EAX != 0xffffffff)) {
    sVar9 = *(short *)(*(int *)((*in_EAX & 0xffff) * 0x20 + 0x14 + DAT_0087bc14) + 0x1c6);
  }
  if (param_2 == '\0') {
    if (*(short *)(local_40 + 0x3ee) != 0) {
      return 0;
    }
    if (sVar9 == 1) {
      bVar5 = 1;
      local_44 = 9;
    }
    else {
      if (bVar3) {
        if (*(short *)(local_40 + 0x3ec) != 0) {
          return 0;
        }
        if (2 < *(short *)(local_40 + 0x3ea)) {
          return 0;
        }
        if ((*(short *)(local_40 + 0x388) != 0) && (fVar11 = random_real(), 0.4 <= fVar11)) {
          return 0;
        }
        local_44 = (uint)(fVar1 <= 0.0) * 2 + 6;
        goto LAB_00561365;
      }
      bVar5 = 1;
      local_44 = 7;
    }
  }
  else {
    uVar7 = *(uint *)(local_40 + 0x1f8);
    if (uVar7 == 0xffffffff) {
      uVar7 = *(uint *)(local_40 + 500);
    }
    bVar2 = false;
    bVar4 = false;
    if (*in_EAX != 0xffffffff) {
      if (*(float *)(*(int *)((*in_EAX & 0xffff) * 0x20 + 0x14 + DAT_0087bc14) + 500) < 2.0) {
        bVar4 = false;
      }
      else {
        bVar4 = true;
      }
    }
    if (uVar7 == 0xffffffff) {
      if (param_4 + 0.2 < *(float *)(local_40 + 0xf8)) {
        bVar2 = true;
      }
    }
    else {
      bVar2 = 2 < *(short *)((uVar7 & 0xffff) * 0x724 + 0x6e + *(int *)(DAT_00880360 + 0x34));
    }
    if (sVar9 == 1) {
      local_44 = 0x10;
    }
    else if (sVar9 == 7) {
      local_44 = 0x11;
    }
    else if (bVar4) {
      local_44 = 0x13;
    }
    else if (bVar2) {
      local_44 = (-(uint)(param_3 != '\0') & 3) + 0xf;
    }
    else {
      local_44 = 0xe;
    }
    bVar5 = 1;
    if ((short)local_44 != 0xe) {
      local_38 = 2;
      local_34 = (((short)local_44 != 0x12) - 1 & 3) + 1;
    }
LAB_00561365:
    if ((short)local_44 == -1) goto LAB_00561461;
  }
  local_3c = 0xffffffff;
  if (param_2 == '\0') {
    sVar9 = (-(ushort)bVar5 & 5) + 2;
  }
  else {
    sVar9 = 10;
  }
  sVar6 = FUN_00560d00(sVar9,0,0,&local_44,&local_3c);
  if (0 < sVar6) {
    psVar10 = &local_30;
    for (iVar8 = 0xc; iVar8 != 0; iVar8 = iVar8 + -1) {
      psVar10[0] = 0;
      psVar10[1] = 0;
      psVar10 = psVar10 + 2;
    }
    local_2e = (undefined2)local_44;
    local_14 = 0;
    local_10 = 0;
    local_c = 0;
    local_8 = 0;
    local_4 = 0;
    local_2c = local_3c;
    local_20 = 0xffffffff;
    local_1c = 0xffffffff;
    local_18 = 0xffff;
    local_24 = 7;
    local_30 = sVar9;
    FUN_00560f20();
    local_47 = 1;
    if (bVar3) {
      *(short *)(local_40 + 0x3ea) = *(short *)(local_40 + 0x3ea) + 1;
      *(undefined2 *)(local_40 + 0x3ec) = 0x1e;
      *(undefined2 *)(local_40 + 1000) = 0x16;
    }
    else {
      *(undefined2 *)(local_40 + 0x3ee) = 0x3c;
    }
  }
LAB_00561461:
  if ((short)local_38 != -1) {
    FUN_0042c2a0();
  }
  return local_47;
}
#endif
