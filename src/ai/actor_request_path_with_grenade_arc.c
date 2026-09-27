// actor_request_path_with_grenade_arc  (Ghidra: actor_request_path_with_grenade_arc, renamed)
// address 0x408300, size 358 bytes
// name confidence: 0.4   rewrite confidence: 0.2
// evidence: types/ai.h actor.needs_new_path (0x4c)/order_committed (0x160); phase-4 summary
//   "issues a pathfinding query that optionally includes grenade-arc parameters and records
//   whether the requested destination changed".
// register convention: actor index in EAX, the sole real parameter.
//   // blam-cc: EAX -> actor_index
// UNSURE: actor+0x9d/0xa0/0xa4/0xa8/0xac/0xb0/0xb4/0xb8/0xbc/0x9e all fall inside
//   actor.mode_data (a per-mode union, see types/ai.h); read/written here as a small
//   pending-path record (a "kind" selector at +0xa4, a point-or-arc payload, and result
//   flags at +0xa0/0x9e/0xbc).
//   UNSURE: the stack layout Ghidra produced splits the request into a small buffer
//   (`auStack_10740`, passed by pointer to actor_select_firing_position) and a separately-named, larger
//   zeroed region (`uStack_10700`..`uStack_106bf`) that is never itself passed to anything;
//   the two are only ~0x40 bytes apart on the stack and every other function in this session
//   that calls actor_select_firing_position treats its second argument as one small request buffer, so this
//   rewrite folds them into one `request` buffer instead of reproducing two disconnected
//   ones. The two post-call output reads (a int16 and a float, `sStack_10736`/
//   `fStack_10734`) are placed at the front of that same buffer for the same reason.
//   actor_select_firing_position/actor_claim_firing_position's scratch buffer mirrors actor_update_path_if_needed.c's
//   treatment.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"
#include <string.h>

extern data_array *actor_data; // 0x00880360

extern int16_t actor_select_firing_position(); // SIGNATURE-CONFLICT: this call site and the rewrite of actor_select_firing_position at 0x413e50
                 // disagree on the argument list; Ghidra drops the register arguments
                 // here. Left unprototyped so the conflict is visible. See src/ai/README.md.
extern int16_t actor_claim_firing_position(); // SIGNATURE-CONFLICT: this call site and the rewrite of actor_claim_firing_position at 0x414060
                 // disagree on the argument list; Ghidra drops the register arguments
                 // here. Left unprototyped so the conflict is visible. See src/ai/README.md.
extern float actor_compute_accuracy_scale(datum_index actor_index); // 0x429620, not yet rewritten

uint8_t actor_request_path_with_grenade_arc(uint32_t actor_index)
{
    actor *a = &((actor *)actor_data->data)[actor_index & 0xffff];
    uint8_t *m = a->mode_data;
    uint8_t result = 0;

    if (a->needs_new_path != 0 && a->order_committed == 0 && m[0x9d - 0x9c] == 0) {
        uint32_t request[18];
        uint8_t scratch_context[65684];
        uint8_t reached_exactly;
        int16_t result_code;
        int16_t have_arc = *(int16_t *)(m + 0xa4 - 0x9c);

        memset(request, 0, sizeof(request));
        request[1] = 3;

        if (have_arc == 1) {
            *((uint8_t *)request + 0x20) = 1;
            *(uint32_t *)((uint8_t *)request + 0x24) = *(uint32_t *)(m + 0xb0 - 0x9c);
            *(uint32_t *)((uint8_t *)request + 0x28) = *(uint32_t *)(m + 0xb4 - 0x9c);
            *(uint32_t *)((uint8_t *)request + 0x2c) = *(uint32_t *)(m + 0xb8 - 0x9c);
            *(uint32_t *)((uint8_t *)request + 0x30) = *(uint32_t *)(m + 0xac - 0x9c);
            *(uint16_t *)((uint8_t *)request + 0x34) = *(uint16_t *)(m + 0xa8 - 0x9c);
        } else {
            *((uint8_t *)request + 0x41) = *(uint8_t *)(m + 0xa0 - 0x9c);
        }

        result_code = actor_select_firing_position(actor_index, request, scratch_context, &reached_exactly);
        if (result_code != -1) {
            int16_t out_kind = *(int16_t *)((uint8_t *)request + 8);

            if (have_arc == 0) {
                if (out_kind != 0 && out_kind != 1) {
                    m[0xa0 - 0x9c] = 1;
                }
            } else if (out_kind == 0) {
                float out_distance = *(float *)((uint8_t *)request + 0xc);
                if (out_distance < actor_compute_accuracy_scale(actor_index)) {
                    m[0xbc - 0x9c] = 1;
                }
            }
        }

        {
            int16_t waypoint = actor_claim_firing_position(actor_index, request[0], scratch_context);
            if (waypoint == -1) {
                m[0x9e - 0x9c] = 1;
            }
            result = (uint8_t)waypoint;
        }
    }
    return result;
}

#if 0
Original Ghidra decompilation (0x408300):

/* WARNING: Function: __chkstk replaced with injection: alloca_probe */

uint FUN_00408300(uint param_1)

{
  char cVar1;
  short sVar2;
  undefined3 uVar4;
  uint uVar3;
  int iVar5;
  int iVar6;
  undefined4 *puVar7;
  float10 fVar8;
  undefined1 uStack_10741;
  undefined4 auStack_10740 [2];
  short sStack_10736;
  float fStack_10734;
  undefined4 uStack_10700;
  undefined2 uStack_106fc;
  undefined1 uStack_106e0;
  undefined4 uStack_106dc;
  undefined4 uStack_106d8;
  undefined4 uStack_106d4;
  undefined4 uStack_106d0;
  undefined2 uStack_106cc;
  undefined1 uStack_106bf;
  undefined1 auStack_10098 [65684];

  iVar6 = (param_1 & 0xffff) * 0x724;
  cVar1 = *(char *)(iVar6 + 0x4c + *(int *)(DAT_00880360 + 0x34));
  uVar4 = (undefined3)((uint)DAT_00880360 >> 8);
  uVar3 = CONCAT31(uVar4,cVar1);
  iVar6 = iVar6 + *(int *)(DAT_00880360 + 0x34);
  if (((cVar1 != '\0') &&
      (uVar3 = CONCAT31(uVar4,*(char *)(iVar6 + 0x160)), *(char *)(iVar6 + 0x160) == '\0')) &&
     (uVar3 = CONCAT31(uVar4,*(char *)(iVar6 + 0x9d)), *(char *)(iVar6 + 0x9d) == '\0')) {
    puVar7 = &uStack_10700;
    for (iVar5 = 0x199; iVar5 != 0; iVar5 = iVar5 + -1) {
      *puVar7 = 0;
      puVar7 = puVar7 + 1;
    }
    uStack_106fc = 3;
    if (*(short *)(iVar6 + 0xa4) == 1) {
      uStack_106e0 = 1;
      uStack_106dc = *(undefined4 *)(iVar6 + 0xb0);
      uStack_106d8 = *(undefined4 *)(iVar6 + 0xb4);
      uStack_106d4 = *(undefined4 *)(iVar6 + 0xb8);
      uStack_106d0 = *(undefined4 *)(iVar6 + 0xac);
      uStack_106cc = *(undefined2 *)(iVar6 + 0xa8);
    }
    else {
      uStack_106bf = *(undefined1 *)(iVar6 + 0xa0);
    }
    sVar2 = FUN_00413e50(param_1,auStack_10740,auStack_10098,&uStack_10741);
    if (sVar2 != -1) {
      if (*(short *)(iVar6 + 0xa4) == 0) {
        if ((sStack_10736 != 0) && (sStack_10736 != 1)) {
          *(undefined1 *)(iVar6 + 0xa0) = 1;
        }
      }
      else if (sStack_10736 == 0) {
        fVar8 = (float10)FUN_00429620();
        if ((float10)fStack_10734 < fVar8) {
          *(undefined1 *)(iVar6 + 0xbc) = 1;
        }
      }
    }
    uVar3 = FUN_00414060(param_1,auStack_10740[0],auStack_10098);
    if ((short)uVar3 == -1) {
      *(undefined1 *)(iVar6 + 0x9e) = 1;
    }
  }
  return uVar3 & 0xffffff00;
}
#endif
