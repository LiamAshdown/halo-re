// ai_reference_get_stat_pair  (Ghidra: ai_reference_get_stat_pair; named for this rewrite)
// address 0x432f90, size 491 bytes
// name confidence: 0.35   rewrite confidence: 0.45
// evidence: for a plain encounter, platoon or squad reference, reads a pair of int16
// counters (or their non-negative difference, selected by stat_kind) from the matching
// types/ai.h record: encounter.unknown_2a/unknown_2c, encounter_platoon_state.unknown_06/
// unknown_08, or encounter_squad_state.unknown_18/unknown_1a, plus a member-count field
// (encounter.member_count via +0x18, platoon/squad's own member_count) and a trailing
// dword (encounter.unknown_34, encounter_platoon_state.unknown_0c, or
// encounter_squad_state.grenade_cooldown). All offsets match the existing header fields
// exactly.
// register convention: Ghidra recognized param_1..param_3 as ordinary parameters and left
// only the stat-kind selector unresolved, in DI.
//   // blam-cc: EAX -> packed_reference, EDI -> stat_kind, stack -> out_member_count, out_extra

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "ai.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern Scenario *global_scenario;                              // 0x00746f8c
extern data_array *encounter_data;                             // 0x008802c8
extern encounter_squad_state *encounter_squad_states;          // 0x008802cc
extern encounter_platoon_state *encounter_platoon_states;      // 0x008802c4

// blam-cc: EAX -> packed_reference, EDI -> stat_kind, stack -> out_member_count, out_extra
// Returns one of a pair of stats (stat_kind 0 or 1), or their non-negative difference
// (stat_kind anything else) from the encounter/platoon/squad a packed ai reference names,
// and writes that record's member count and trailing dword through the two out-parameters
// (either of which may be NULL). Returns 0, and leaves the out-parameters at their default
// (0 / untouched), for a reference this function cannot resolve.
uint32_t ai_reference_get_stat_pair(uint32_t packed_reference, int16_t stat_kind, int32_t *out_member_count,
                                     uint32_t *out_extra)
{
    uint32_t extra = 0;
    uint32_t result = 0;
    int32_t member_count = 0;

    if (packed_reference != (uint32_t)k_datum_index_none) {
        uint32_t kind = packed_reference >> 0x1e;
        uint32_t encounter_index = packed_reference & 0xffff;

        if (kind == 0) {
            if ((int32_t)encounter_index < global_scenario->encounters.count) {
                encounter *enc = &((encounter *)encounter_data->data)[encounter_index];
                if (stat_kind == 0) {
                    result = (uint32_t)enc->living_count;
                } else if (stat_kind == 1) {
                    result = (uint32_t)enc->swarm_count;
                } else {
                    int32_t diff = (int32_t)enc->living_count - (int32_t)enc->swarm_count;
                    result = (uint32_t)(diff & ~(diff >> 31));
                }
                member_count = enc->member_count;
                extra = *(uint32_t *)&enc->average_vitality;
            }
        } else if (kind == 1) {
            if ((int32_t)encounter_index < global_scenario->encounters.count) {
                encounter *enc = &((encounter *)encounter_data->data)[encounter_index];
                int16_t platoon_sub_index = (int8_t)(packed_reference >> 0x10);
                if (platoon_sub_index < enc->platoon_count) {
                    encounter_platoon_state *state =
                        &encounter_platoon_states[enc->first_platoon + platoon_sub_index];
                    if (stat_kind == 0) {
                        result = (uint32_t)state->living_count;
                        extra = *(uint32_t *)&state->average_vitality;
                        member_count = state->member_count;
                    } else if (stat_kind == 1) {
                        result = (uint32_t)state->swarm_count;
                        extra = *(uint32_t *)&state->average_vitality;
                        member_count = state->member_count;
                    } else {
                        int32_t diff;
                        extra = *(uint32_t *)&state->average_vitality;
                        member_count = state->member_count;
                        diff = (int32_t)state->living_count - (int32_t)state->swarm_count;
                        result = (uint32_t)(diff & ~(diff >> 31));
                    }
                }
            }
        } else if ((int32_t)encounter_index < global_scenario->encounters.count) {
            encounter *enc = &((encounter *)encounter_data->data)[encounter_index];
            int16_t squad_sub_index = (int8_t)(packed_reference >> 0x10);
            if (squad_sub_index < enc->squad_count) {
                encounter_squad_state *state = &encounter_squad_states[enc->first_squad + squad_sub_index];
                if (stat_kind == 0) {
                    result = (uint32_t)state->living_count;
                    extra = (uint32_t)state->average_vitality;
                    member_count = state->member_count;
                } else if (stat_kind == 1) {
                    result = (uint32_t)state->swarm_count;
                    extra = (uint32_t)state->average_vitality;
                    member_count = state->member_count;
                } else {
                    int32_t diff;
                    extra = (uint32_t)state->average_vitality;
                    member_count = state->member_count;
                    diff = (int32_t)state->living_count - (int32_t)state->swarm_count;
                    result = (uint32_t)(diff & ~(diff >> 31));
                }
            }
        }
    }

    if (out_member_count != 0) {
        *out_member_count = member_count;
    }
    if (out_extra != 0) {
        *out_extra = extra;
    }
    return result;
}

#if 0
Original Ghidra decompilation (0x432f90):

uint FUN_00432f90(uint param_1,int *param_2,undefined4 *param_3)

{
  undefined4 uVar1;
  uint uVar2;
  int iVar3;
  int iVar4;
  short unaff_DI;

  uVar1 = 0;
  uVar2 = 0;
  iVar4 = 0;
  if (param_1 != 0xffffffff) {
    if (param_1 >> 0x1e == 0) {
      if ((int)(param_1 & 0xffff) < *(int *)(global_scenario + 0x42c)) {
        iVar3 = (param_1 & 0xffff) * 0x6c + *(int *)(DAT_008802c8 + 0x34);
        if (unaff_DI == 0) {
          uVar2 = (uint)*(short *)(iVar3 + 0x2a);
        }
        else if (unaff_DI == 1) {
          uVar2 = (uint)*(short *)(iVar3 + 0x2c);
        }
        else {
          uVar2 = (int)*(short *)(iVar3 + 0x2a) - (int)*(short *)(iVar3 + 0x2c);
          uVar2 = uVar2 & ((int)uVar2 < 0) - 1;
        }
        iVar4 = (int)*(short *)(iVar3 + 0x18);
        uVar1 = *(undefined4 *)(iVar3 + 0x34);
      }
    }
    else if (param_1 >> 0x1e == 1) {
      if ((int)(param_1 & 0xffff) < *(int *)(global_scenario + 0x42c)) {
        iVar3 = (param_1 & 0xffff) * 0x6c + *(int *)(DAT_008802c8 + 0x34);
        if ((short)(ushort)param_1._2_1_ < *(short *)(iVar3 + 10)) {
          iVar3 = (short)(*(short *)(iVar3 + 8) + (ushort)param_1._2_1_) * 0x10 + DAT_008802c4;
          if (unaff_DI == 0) {
            uVar2 = (uint)*(short *)(iVar3 + 6);
            uVar1 = *(undefined4 *)(iVar3 + 0xc);
            iVar4 = (int)*(short *)(iVar3 + 4);
          }
          else if (unaff_DI == 1) {
            uVar2 = (uint)*(short *)(iVar3 + 8);
            uVar1 = *(undefined4 *)(iVar3 + 0xc);
            iVar4 = (int)*(short *)(iVar3 + 4);
          }
          else {
            uVar1 = *(undefined4 *)(iVar3 + 0xc);
            iVar4 = (int)*(short *)(iVar3 + 4);
            uVar2 = (int)*(short *)(iVar3 + 6) - (int)*(short *)(iVar3 + 8);
            uVar2 = uVar2 & ((int)uVar2 < 0) - 1;
          }
        }
      }
    }
    else if ((int)(param_1 & 0xffff) < *(int *)(global_scenario + 0x42c)) {
      iVar3 = (param_1 & 0xffff) * 0x6c + *(int *)(DAT_008802c8 + 0x34);
      if ((short)(ushort)param_1._2_1_ < *(short *)(iVar3 + 6)) {
        iVar4 = (short)(*(short *)(iVar3 + 4) + (ushort)param_1._2_1_) * 0x20 + DAT_008802cc;
        if (unaff_DI == 0) {
          uVar2 = (uint)*(short *)(iVar4 + 0x18);
          uVar1 = *(undefined4 *)(iVar4 + 0x1c);
          iVar4 = (int)*(short *)(iVar4 + 0x16);
        }
        else if (unaff_DI == 1) {
          uVar2 = (uint)*(short *)(iVar4 + 0x1a);
          uVar1 = *(undefined4 *)(iVar4 + 0x1c);
          iVar4 = (int)*(short *)(iVar4 + 0x16);
        }
        else {
          uVar1 = *(undefined4 *)(iVar4 + 0x1c);
          uVar2 = (int)*(short *)(iVar4 + 0x18) - (int)*(short *)(iVar4 + 0x1a);
          iVar4 = (int)*(short *)(iVar4 + 0x16);
          uVar2 = uVar2 & ((int)uVar2 < 0) - 1;
        }
      }
    }
  }
  if (param_2 != (int *)0x0) {
    *param_2 = iVar4;
  }
  if (param_3 != (undefined4 *)0x0) {
    *param_3 = uVar1;
    return uVar2;
  }
  return uVar2;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
