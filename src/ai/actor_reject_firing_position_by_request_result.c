// actor_reject_firing_position_by_request_result  (Ghidra: actor_reject_firing_position_by_request_result, renamed)
// address 0x412620, size 198 bytes
// name confidence: 0.5   rewrite confidence: 0.8
// evidence: row 1 of the rejection table at 0x006555f8 with the kinds mask 0x51, i.e. goal
//   kinds 0, 4 and 6. It reads nothing but candidate.request_result (0x06), which
//   actor_report_firing_position_request @0x4120f0 fills in immediately before the
//   rejection pass, plus query.target_is_large (0x628) and query.collect_all (0x14).
// register convention: actor_index, the query and the candidate are the three
//   Ghidra-recognized stack parameters.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"

// blam-cc: stack -> actor_index, query, candidate
// Turns the perception result code the movement request came back with into a desirability
// term. Code 0 is the best outcome and scores 15.0, code 1 scores 5.0, and anything else
// scores nothing and is rejected unless the target is large. A large target roughly halves
// both bonuses. The probe call with no candidate gives the baseline the code 0 value.
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
uint8_t actor_reject_firing_position_by_request_result(datum_index actor_index,
                                                       actor_firing_position_query *query,
                                                       actor_firing_position_candidate *candidate)
{
    float bonus;

    if (query->have_target != 0) {
        if (candidate == (actor_firing_position_candidate *)0) {
            if (query->target_is_large != 0) {
                query->baseline_penalty = query->baseline_penalty + 6.0f;
            } else {
                query->baseline_penalty = query->baseline_penalty + 15.0f;
            }
            return 1;
        }

        bonus = 0.0f;
        if (candidate->request_result == 0) {
            bonus = (query->target_is_large != 0) ? 6.0f : 15.0f;
        } else if (candidate->request_result == 1) {
            bonus = (query->target_is_large != 0) ? 2.5f : 5.0f;
        } else if (query->target_is_large == 0) {
            candidate->rejected = 1;
            if (query->collect_all == 0) {
                candidate->valid = 0;
            }
        }
        candidate->score = bonus + candidate->score;
    }

    if (candidate == (actor_firing_position_candidate *)0) {
        return 1;
    }
    return candidate->valid;
}

#if 0
Original Ghidra decompilation (0x412620):

undefined1 FUN_00412620(undefined4 param_1,int param_2,int param_3)

{
  float fVar1;

  if (*(char *)(param_2 + 0x5fc) != '\0') {
    if (param_3 == 0) {
      if (*(char *)(param_2 + 0x628) != '\0') {
        *(float *)(param_2 + 0x660) = *(float *)(param_2 + 0x660) + 6.0;
        return 1;
      }
      *(float *)(param_2 + 0x660) = *(float *)(param_2 + 0x660) + 15.0;
      return 1;
    }
    fVar1 = 0.0;
    if (*(short *)(param_3 + 6) == 0) {
      if (*(char *)(param_2 + 0x628) == '\0') {
        fVar1 = 15.0;
      }
      else {
        fVar1 = 6.0;
      }
    }
    else if (*(short *)(param_3 + 6) == 1) {
      if (*(char *)(param_2 + 0x628) == '\0') {
        fVar1 = 5.0;
      }
      else {
        fVar1 = 2.5;
      }
    }
    else if ((*(char *)(param_2 + 0x628) == '\0') &&
            (*(undefined1 *)(param_3 + 0x31) = 1, *(char *)(param_2 + 0x14) == '\0')) {
      *(undefined1 *)(param_3 + 0x30) = 0;
    }
    *(float *)(param_3 + 0x38) = fVar1 + *(float *)(param_3 + 0x38);
  }
  if (param_3 == 0) {
    return 1;
  }
  return *(undefined1 *)(param_3 + 0x30);
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
