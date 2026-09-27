// camera_observer_find_best_target  (Ghidra: FUN_00459a00, already named per symbols/functions.txt
// via a later merge; kept)
// address 0x459a00, size 270 bytes
// name confidence: 0.35   rewrite confidence: 0.85 (VERIFIED 2026-09-27 static loop against objdump 0x459a00..0x459b0d: generate / qsort (0x45a4a0) / validity loop)
// evidence: types/game.h observer_target_candidate (0x38 bytes, 64-slot 0xe00-byte stack array,
//   matching the qsort element width and array size here exactly); camera_observer_target_compare
//   (0x45a4a0) is the qsort comparator; camera_observer_target_is_valid (0x459dd0) is the final
//   acceptance filter.
// register convention: RE-DERIVED from
//   objdump -d -M intel --start-address=0x459a00 --stop-address=0x459b10 bin/halo.exe
//   and from the call site inside camera_observer_update (0x459472..0x459496). Ghidra reports
//   zero parameters for this function; in fact it takes FIVE stack arguments plus EBX:
//     mov edx,[esp+0xe10] / mov edi,[esp+0xe0c] / mov ecx,[esp+0xe18] / mov edx,[esp+0xe1c]
//       -> E0+0x10, E0+0x04, E0+0x0c, E0+0x08, and E0+0x14 (read at 0x459ad6 for the rep movsd
//          of 0xe dwords == one observer_target_candidate) = arg0..arg4
//     the caller pushes, in order, out / team / player->unit / facing / cone, so the stack
//       arguments are (cone, facing, exclude_object, team, out) -- the same five values the
//       previous version of this file guessed, but they are stack arguments, not registers.
//     ebx is NEVER written before use here (`mov edx,ebx` at 0x459a0c, `mov ecx,ebx` at
//       0x459ab6, `push ebx` at 0x459a69) and the callee it reaches, 0x459dd0, dereferences it
//       as three floats, so EBX is a real_point3d * -- the observer position. The caller sets it
//       with `lea ebx,[esp+0x40]` immediately before the call, pointing at the position the
//       camera routine (0x446a90 / 0x447290) just produced.
//   // blam-cc: EBX -> observer_position, stack -> (cone, facing, exclude_object, team, out)
//
// UNSURE: bsp3d_node_find_leaf (finds a camera cluster/leaf index) and the scenario cluster table at
// global_scenario+0xe4 are not in this batch; modelled with the minimum shape this function's
// own decompile shows. The cluster word that 0x459f70 receives is read out of that table at
// +0x08 with a 0x10 stride.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "game.h"

extern uint8_t *global_structure_bsp; // 0x00746f9c, the leaves block pointer is at +0xe4 (0x459a1e)
extern ModelCollisionGeometryBSP *global_collision_bsp; // 0x00746f90
extern uint32_t bsp3d_node_find_leaf(int32_t node_index, ModelCollisionGeometryBSP *bsp, real_point3d *point); // 0x5013a0, EAX node, ECX bsp, EDX point
    // EAX/ECX/EDX register arguments and does float math (objdump 0x5013a0), none of which
    // Ghidra surfaces. Declared as returning EDX:EAX because game_engine_location_blocked_by_vehicle
    // splits the result into a leaf index (EAX) and a real_point3d * (EDX);
    // camera_observer_find_best_target uses only the low half.

extern int16_t camera_observer_generate_target_candidates(observer_target_cone *cone,
    int16_t start_cluster, real_point3d *observer_position, real_vector3d *facing,
    datum_index exclude_object, int16_t team, int16_t capacity,
    observer_target_candidate *out); // this batch, 0x459f70; cone travels in EDI
extern int32_t camera_observer_target_compare(const observer_target_candidate *a, const observer_target_candidate *b); // this batch, 0x45a4a0
extern char camera_observer_target_is_valid(datum_index exclude_object,
    real_point3d *observer_position, real_point3d *target_position,
    datum_index target_object); // this batch, 0x459dd0
extern void qsort(void *base, uint32_t count, uint32_t size, uint32_t (*compare)(const void *, const void *)); // CRT

// Generates up to 64 candidates for the current camera cluster, sorts them by
// camera_observer_target_compare, and returns the first one that still passes
// camera_observer_target_is_valid.
char camera_observer_find_best_target(real_point3d *observer_position,
                                      observer_target_cone *cone, real_vector3d *facing,
                                      datum_index exclude_object, int16_t team,
                                      observer_target_candidate *out)
    // blam-cc: EBX -> observer_position, stack -> (cone, facing, exclude_object, team, out)
{
    observer_target_candidate candidates[64];
    int32_t cluster;
    int16_t start_cluster;
    int16_t candidate_count;
    int16_t i;

    // 0x459a00..0x459a10: EAX = 0, ECX = the global collision bsp, EDX = the observer position (EBX). The draft
    // called it with no arguments (crash).
    cluster = (int32_t)bsp3d_node_find_leaf(0, global_collision_bsp, observer_position);
    if (cluster != -1) {
        // UNSURE: TYPES-GAP raw offset into the scenario's cluster table (0x10 stride, +0x08)
        // 0x459a1e..0x459a32: the leaf's cluster (+0x08) in the STRUCTURE bsp leaves (+0xe4, 0x10 each); the
        // draft read the table through the scenario pointer.
        start_cluster = *(int16_t *)((cluster & 0x7fffffff) * 0x10 + 8 +
                                     *(int32_t *)(global_structure_bsp + 0xe4));
        if (start_cluster != -1) {
            candidate_count = camera_observer_generate_target_candidates(
                cone, start_cluster, observer_position, facing, exclude_object, team, 64, candidates);
            if (candidate_count < 1) {
                return 0;
            }
            qsort(candidates, (uint32_t)candidate_count, sizeof(observer_target_candidate),
                  (uint32_t (*)(const void *, const void *))camera_observer_target_compare);
            for (i = 0; i < candidate_count; i = i + 1) {
                if (camera_observer_target_is_valid(exclude_object, observer_position,
                                                    &candidates[i].point, candidates[i].object) != 0) {
                    *out = candidates[i];
                    return 1;
                }
            }
        }
    }
    return 0;
}

#if 0
Original Ghidra decompilation (0x459a00), from tools/pack.py 0x459a00:

undefined4 FUN_00459a00(void)

{
  char cVar1;
  short sVar2;
  int iVar3;
  short sVar4;
  undefined4 *puVar5;
  undefined4 *in_stack_00000014;
  undefined4 local_e00 [896];

  iVar3 = FUN_005013a0();
  if ((iVar3 != -1) &&
     (sVar2 = *(short *)(iVar3 * 0x10 + 8 + *(int *)(DAT_00746f9c + 0xe4)), sVar2 != -1)) {
    sVar2 = FUN_00459f70(CONCAT22((short)((uint)(iVar3 * 0x10) >> 0x10),sVar2));
    if (sVar2 < 1) {
      return 0;
    }
    _qsort(local_e00,(int)sVar2,0x38,FUN_0045a4a0);
    sVar4 = 0;
    if (0 < sVar2) {
      do {
        cVar1 = FUN_00459dd0(local_e00[sVar4 * 0xe]);
        if (cVar1 != '\0') {
          puVar5 = local_e00 + sVar4 * 0xe;
          for (iVar3 = 0xe; iVar3 != 0; iVar3 = iVar3 + -1) {
            *in_stack_00000014 = *puVar5;
            puVar5 = puVar5 + 1;
            in_stack_00000014 = in_stack_00000014 + 1;
          }
          return 1;
        }
        sVar4 = sVar4 + 1;
      } while (sVar4 < sVar2);
    }
    return 0;
  }
  return 0;
}
#endif
