// camera_observer_generate_target_candidates  (Ghidra: FUN_00459f70; renamed per
// symbols/review_queue.txt)
// address 0x459f70, size 347 bytes
// name confidence: 0.3   rewrite confidence: 0.85 (VERIFIED 2026-09-27 static loop against objdump 0x459f70..0x45a0ca; flood fill start cluster restored)
// evidence: symbols/review_queue.txt 0x459f70 "samples directions with fsin/fcos across the
//   largest field-of-view axis, calls cluster_flood_fill_with_predicate then object_collect_in_clusters to build a
//   batch, and accumulates results from camera_observer_collect_target_candidates"; the cone
//   bounds picked here (max of angle_a/angle_b, max of distance_a/distance_b) match
//   observer_target_cone.
// register convention: cone pointer in EDI (unaff_EDI); everything else is a stack argument.
//   // blam-cc: EDI -> cone, stack -> (start_cluster, observer_position, facing,
//   //          exclude_object, team, capacity, out)
//
// UNSURE: cluster_flood_fill_with_predicate and object_collect_in_clusters are not in this batch, so their true
// signatures are modelled loosely (matching only the shapes this one call site shows).
// object_collect_in_clusters result is read here as a per-cluster array of "head object
// index" values, on the theory that camera_observer_collect_target_candidates (0x45a0e0) already
// walks an entire cluster sibling/child tree from one starting object, so one call per cluster
// (rather than per object) is what the loop bound implies -- not independently verified.
//
// RE-DERIVED (phase 4 review), not a transcription of Ghidra's C. Ghidra reports three
// parameters; the function really takes SEVEN stack arguments plus the cone in EDI. From
//   objdump -d -M intel --start-address=0x459f70 --stop-address=0x45a0e0 bin/halo.exe
// with E0 = the return-address slot and the 0x2410-byte alloca accounted for:
//   [E0+0x04] start_cluster  (passed on to 0x554e30 in EAX)
//   [E0+0x08] observer_position      [E0+0x0c] facing        [E0+0x10] exclude_object
//   [E0+0x14] team                   [E0+0x18] capacity (ebp, the loop bound)
//   [E0+0x1c] out
// and edi is dereferenced as the cone before anything is written ([edi+0x4] vs [edi+0xc],
// [edi] vs [edi+0x8] = distance_a/distance_b, angle_a/angle_b).
// This REPLACES the previous version's three invented globals
// (camera_observer_search_position / _exclude_object / _team): the observer position, the
// excluded object and the team were never globals, they are stack parameters that Ghidra lost.
// The three frustum scalars the collect call receives are likewise locals, not cone fields:
//   [E0-0x2410] max(distance_a, distance_b)   [E0-0x240c] sin(max_angle)
//   [E0-0x2408] cos(max_angle)
// and the 11 pushes at 0x45a066..0x45a09f give camera_observer_collect_target_candidates'
// full argument list (the previous version omitted `team` entirely).

#include "tags.h"
#include "math.h"
#include "memory.h"
#include "game.h"

extern double sin(double x); // x87 FSIN
extern double cos(double x); // x87 FCOS
extern int16_t cluster_flood_fill_with_predicate(real_point3d *position, real_vector3d *facing, real max_distance,
    real sin_angle, real cos_angle, int16_t max_count, int16_t *output, int16_t start_cluster); // 0x554e30, AX start
extern int16_t object_collect_in_clusters(uint32_t search_mask, int16_t cluster_count,
    int16_t *cluster_indices, int16_t max_output, datum_index *out_objects); // 0x4f7180,
    // all five are plain stack arguments; prototype taken from src/objects/object_collect_in_clusters.c.
    // UNSURE: that module reads argument 2 as a CLUSTER COUNT and argument 3 as the int16
    // cluster-index array; this call site's own evidence only shows that 0x554e30's return
    // value goes into argument 2 and the buffer 0x554e30 filled goes into argument 3, which
    // is consistent with it. The "frustum" reading the first pass used is not.
extern uint16_t camera_observer_collect_target_candidates(observer_target_cone *cone,
    datum_index start_object, real_point3d *observer_position, real_vector3d *facing,
    real max_distance, real sin_max_angle, real cos_max_angle, datum_index exclude_object,
    int16_t observer_team, int16_t capacity, observer_target_candidate *out); // 0x45a0e0

// Builds a detection frustum from `cone`'s widest angle/distance bounds, gathers the clusters it
// touches, then walks each cluster with camera_observer_collect_target_candidates, stopping once
// `capacity` candidates have been written to `out`.
int16_t camera_observer_generate_target_candidates(observer_target_cone *cone,
                                                   int16_t start_cluster,
                                                   real_point3d *observer_position,
                                                   real_vector3d *facing,
                                                   datum_index exclude_object, int16_t team,
                                                   int16_t capacity,
                                                   observer_target_candidate *out)
    // blam-cc: EDI -> cone, stack -> (start_cluster, observer_position, facing,
    //          exclude_object, team, capacity, out)
{
    real max_distance;
    real max_angle;
    real sin_max_angle;
    real cos_max_angle;
    int16_t cluster_count;
    int16_t i;
    int16_t total;
    int16_t cluster_indices[512];      // UNSURE size: matches Ghidra's local_2400 (esp+0x40)
    datum_index cluster_heads[2048];   // UNSURE size: matches Ghidra's local_2000 (esp+0x438)
    int16_t collected_clusters;

    max_distance = (cone->distance_a <= cone->distance_b) ? cone->distance_b : cone->distance_a;
    max_angle = (cone->angle_a <= cone->angle_b) ? cone->angle_b : cone->angle_a;
    if (max_distance <= 0.0f || max_angle <= 0.0f) {
        return 0;
    }

    sin_max_angle = (real)sin((double)max_angle);
    cos_max_angle = (real)cos((double)max_angle);
    // 0x459fdd..0x45a01a: AX = start_cluster (FIXED 2026-09-27: the draft dropped it), stack (observer_position,
    // facing, max_distance, sin, cos, 0x200, cluster_indices)
    collected_clusters = cluster_flood_fill_with_predicate(observer_position, facing, max_distance,
                                      sin_max_angle, cos_max_angle, 0x200, cluster_indices, start_cluster);
    cluster_count = object_collect_in_clusters(1, collected_clusters, cluster_indices, 0x800,
                                               cluster_heads);

    total = 0;
    if (0 < cluster_count) {
        for (i = 0; i < cluster_count; i = i + 1) {
            total = total + (int16_t)camera_observer_collect_target_candidates(
                cone, cluster_heads[i], observer_position, facing,
                max_distance, sin_max_angle, cos_max_angle,
                exclude_object, team, (int16_t)(capacity - total), out + total);
            if (capacity <= total) {
                return total;
            }
        }
    }
    return total;
}

#if 0
Original Ghidra decompilation (0x459f70), from tools/pack.py 0x459f70:

/* WARNING: Function: __chkstk replaced with injection: alloca_probe */

short FUN_00459f70(undefined4 param_1,undefined4 param_2,undefined4 param_3,undefined4 param_4,
                  undefined4 param_5,short param_6)

{
  float fVar1;
  short sVar2;
  short sVar3;
  undefined4 uVar4;
  short sVar5;
  short sVar6;
  float *unaff_EDI;
  float10 fVar7;
  float10 fVar8;
  float local_2410;
  undefined1 local_2400 [1024];
  undefined1 local_2000 [8188];
  undefined4 uStack_4;

  uStack_4 = 0x459f7a;
  sVar6 = 0;
  sVar3 = 0;
  if (unaff_EDI[1] <= unaff_EDI[3]) {
    local_2410 = unaff_EDI[3];
  }
  else {
    local_2410 = unaff_EDI[1];
  }
  if (*unaff_EDI <= unaff_EDI[2]) {
    fVar1 = unaff_EDI[2];
  }
  else {
    fVar1 = *unaff_EDI;
  }
  fVar7 = (float10)fVar1;
  if ((0.0 < local_2410) && ((float10)0.0 < fVar7)) {
    fVar8 = (float10)fsin(fVar7);
    fVar7 = (float10)fcos(fVar7);
    uVar4 = FUN_00554e30(param_2,param_3,local_2410,(float)fVar8,(float)fVar7,0x200,local_2400);
    sVar2 = object_collect_in_clusters(1,uVar4,local_2400,0x800,local_2000);
    sVar5 = 0;
    if (0 < sVar2) {
      do {
        sVar3 = FUN_0045a0e0();
        sVar3 = sVar6 + sVar3;
        if (param_6 <= sVar3) {
          return sVar3;
        }
        sVar5 = sVar5 + 1;
        sVar6 = sVar3;
      } while (sVar5 < sVar2);
    }
    return sVar3;
  }
  return 0;
}
#endif
