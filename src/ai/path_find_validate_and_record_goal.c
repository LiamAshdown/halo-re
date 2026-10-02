// path_find_validate_and_record_goal  (Ghidra: path_find_validate_and_record_goal, renamed)
// address 0x43a190, size 136 bytes
// name confidence: 0.35  rewrite confidence: 0.9 (REWRITTEN: reachability call arguments from 0x43a1a5 (EAX goal, ECX point_b, ESI local out, stack context/&byte); success-only copy)
// evidence: phase-4 summary "attempts to validate and record a candidate pathfinding goal
// point". Calls path_find_test_direct_reachability @0x43a0a0 (this rewrite).
// register convention: EBX -> candidate record (output, zeroed then filled in); stack ->
//   context, two unused words, position.
//   // blam-cc: EBX -> candidate, stack -> context, point_b, unused_c, position
//
// UNSURE: param_2/param_3 (Ghidra's recognized stack parameters 2 and 3) are never read
// anywhere in this function's body; declared here to match the call site's arity but marked
// unused. path_find_test_direct_reachability's point_a/point_b (its register-passed
// EAX/ECX operands) are not set up anywhere visible in this function either -- they must
// already be live from this function's own caller and are simply forwarded untouched, which
// this rewrite cannot express in portable C; passed as NULL below, a deviation from strict
// register-preserving semantics that could not be resolved without a disassembly of this
// function's caller. Its third operand (out_position, ESI) is recovered here: Ghidra's
// `local_c/local_8/local_4` are read back into the candidate's alt_position immediately
// after the call with no other writer, so this rewrite passes `&candidate->alt_position`
// directly instead of reproducing that intermediate copy.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

// TYPES-GAP: a caller-owned 0x5c-byte scratch record, not established elsewhere in this
// module. Only the fields this function actually writes are named; the remainder (zeroed by
// the leading clear loop, per Ghidra's `for (iVar2 = 0x17; ...)`) is left as padding.
extern uint8_t path_find_test_direct_reachability(const real_point3d *point_a, const real_point3d *point_b,
                                                   real_point3d *out_position, void *context, uint8_t *out_success); // 0x43a0a0

// blam-cc: EBX -> candidate, stack -> context, point_b, unused_c, position
// REWRITTEN (0x43a190..0x43a217): the reachability test runs from the goal (EAX = position) to
//   the second argument (ECX, the flying actor's body position -- not unused) with ESI = a local
//   out point and stack (context, &local reachable byte); only on success are the locals copied
//   to +0x20 / +0x18 and the goal recorded. The draft passed two NULL points.
uint8_t path_find_validate_and_record_goal(ai_path_candidate_goal *candidate, void *context,
                                           uint32_t point_b, uint32_t unused_c, const real_point3d *position)
{
    uint32_t *clear;
    int32_t i;
    real_point3d reached;
    uint8_t reachable;

    (void)unused_c;

    clear = (uint32_t *)candidate;
    for (i = 0x17; i != 0; i = i - 1) {
        *clear = 0;
        clear = clear + 1;
    }

    if (path_find_test_direct_reachability(position, (const real_point3d *)point_b, &reached, context,
                                           &reachable) != 0) {
        candidate->alt_position = reached;
        candidate->flag_19 = 1;
        candidate->unknown_1c = 0xffffffff;
        candidate->flag_1a = 0;
        candidate->reachable = reachable;
        candidate->position = *position;
        candidate->unknown_10 = 0xffffffff;
        candidate->unknown_14 = 0;
        candidate->valid = 1;
    }
    return candidate->valid;
}

#if 0
// ---- original Ghidra decompilation (FUN_0043a190 @ 0x43a190) ----
undefined1
FUN_0043a190(undefined4 param_1,undefined4 param_2,undefined4 param_3,undefined4 *param_4)

{
  char cVar1;
  int iVar2;
  undefined4 *unaff_EBX;
  undefined4 *puVar3;
  undefined1 local_d;
  undefined4 local_c;
  undefined4 local_8;
  undefined4 local_4;

  puVar3 = unaff_EBX;
  for (iVar2 = 0x17; iVar2 != 0; iVar2 = iVar2 + -1) {
    *puVar3 = 0;
    puVar3 = puVar3 + 1;
  }
  cVar1 = FUN_0043a0a0(param_1,&local_d);
  if (cVar1 != '\0') {
    unaff_EBX[8] = local_c;
    unaff_EBX[9] = local_8;
    unaff_EBX[10] = local_4;
    *(undefined1 *)((int)unaff_EBX + 0x19) = 1;
    unaff_EBX[7] = 0xffffffff;
    *(undefined1 *)((int)unaff_EBX + 0x1a) = 0;
    *(undefined1 *)(unaff_EBX + 6) = local_d;
    unaff_EBX[1] = *param_4;
    unaff_EBX[2] = param_4[1];
    unaff_EBX[3] = param_4[2];
    unaff_EBX[4] = 0xffffffff;
    unaff_EBX[5] = 0;
    *(undefined1 *)unaff_EBX = 1;
  }
  return *(undefined1 *)unaff_EBX;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
