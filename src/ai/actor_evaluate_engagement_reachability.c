// actor_evaluate_engagement_reachability  (Ghidra: actor_evaluate_engagement_reachability; named for this rewrite)
// address 0x42b270, size 851 bytes
// name confidence: 0.4   rewrite confidence: 0.3
// evidence: phase-4 summary ("determines whether and how an actor can reach or engage a
// target, combining a pathfinding/LOS query with distance and speed checks, returning a
// graded result code from clear (0) to unreachable (4)"). Ghidra's own decompile drops
// every register argument to collision_test_movement_segment and FUN_00401a20 and calls the latter four times
// with what looks like the same three arguments -- it is not. Partially re-derived from the
// real disassembly (objdump -d -M intel --start-address=0x42b270 --stop-address=0x42b5c3
// bin/halo.exe): the entry gate, the collision-mask computation and the two callee's real
// register mapping are confirmed byte-for-byte; the exact interpolated points fed to the
// four FUN_00401a20 calls in the middle section are reconstructed from the surrounding FPU
// arithmetic but not independently cross-checked against a second source, so the point
// names there (near_point / far_point) describe the arithmetic, not a confirmed meaning.
// register convention: EAX -> self_index, ECX -> target_index, ESI -> target_position
// (unaff_ESI), EDI -> self_position (unaff_EDI); stack -> movement_mode, allow_wide_mask,
// exclude_object_index, flying.
// blam-cc: EAX -> self_index, ECX -> target_index, ESI -> target_position, EDI ->
// self_position, stack -> movement_mode, allow_wide_mask, exclude_object_index, flying
//
// UNSURE: scenario_cluster_visibility_test's role (a relationship/compatibility gate between self_index and
// target_index, guessed from context) is not established anywhere else in this repo.
// UNSURE: the collision_mask base 0xc2a7 (widened to 0xc2b3 when allow_wide_mask, and with
// bit 0x200 cleared when flying) is reproduced verbatim; no bit of it is named.
// UNSURE: mask/exclude_object_index are ebp/ebx respectively at every call site (confirmed
// for the one fully-traced collision_test_movement_segment call and consistent at every FUN_00401a20 call
// site's push order), but the two interpolation fractions (0x672b8c, 0x672bac) and which of
// self_position/target_position plays "target" vs "origin" in the second (0x672bac) block
// versus the first could not be fully disambiguated from the interleaved integer/FPU
// instruction stream in the time available. Re-run `python tools/pack.py 0x42b270` and
// compare against objdump before trusting the exact grading thresholds.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"

extern double sqrt(double x); // SQRT is a single x87 FSQRT instruction

extern real DAT_00672b8c; // 0x00672b8c, mode-1 origin-point interpolation fraction
extern real DAT_00672bac; // 0x00672bac, mode-2 origin-point interpolation fraction

extern uint8_t scenario_cluster_visibility_test(datum_index self_or_target_index); // 0x0053eb60, not yet rewritten (units/objects module)
extern real vector3d_normalize_with_length(real_vector3d *v); // 0x401990, vector in ECX, length returned via a caller-owned float slot
extern void point3d_add_scaled(void); // 0x401930, called here with no visible arguments -- UNSURE
extern uint8_t collision_test_movement_segment(uint32_t mask, real_point3d *origin, real_vector3d *delta,
                             uint32_t exclude_object, void *scratch); // 0x505880
extern uint8_t FUN_00401a20(real_point3d *target, real_point3d *origin, uint32_t collision_mask,
                             uint32_t ignore_object_index, void *out_record); // 0x401a20

// blam-cc: EAX -> self_index, ECX -> target_index, ESI -> target_position, EDI ->
// self_position, stack -> movement_mode, allow_wide_mask, exclude_object_index, flying
// Grades how reachable target_position is from self_position: 4 (unreachable) if a
// relationship gate fails outright; otherwise runs a direct collision sweep and, if
// movement_mode requests it, up to three more sweeps against a waypoint offset toward the
// target (mode 1) or back toward self (mode 2). Returns 1 if any sweep hits something, 0 if
// the direct sweep was clear and no further sweep was needed, or a distance/closing-speed
// grade (2, 3 or 4) when the direct sweep was blocked but movement_mode was 0.
int32_t actor_evaluate_engagement_reachability(datum_index self_index, datum_index target_index,
                                                real_point3d *target_position, real_point3d *self_position,
                                                int16_t movement_mode, uint8_t allow_wide_mask,
                                                datum_index exclude_object_index, uint8_t flying)
{
    uint32_t collision_mask;
    real_vector3d delta;
    real_vector3d direction;
    real distance;
    uint8_t direct_clear;
    uint8_t hit;
    ai_reachability_scratch scratch;
    real closing_speed;
    real_point3d candidate_point;

    if (self_index != (datum_index)k_datum_index_none && target_index != (datum_index)k_datum_index_none &&
        !scenario_cluster_visibility_test(self_index)) {
        return 4;
    }

    collision_mask = (allow_wide_mask ? 0xc2b3u : 0xc2a7u);
    if (flying) {
        collision_mask &= 0xfffffdff;
    }

    delta.i = target_position->x - self_position->x;
    delta.j = target_position->y - self_position->y;
    delta.k = target_position->z - self_position->z;
    hit = collision_test_movement_segment(collision_mask, self_position, &delta, exclude_object_index, &scratch);
    closing_speed = 0.0f;
    if (hit != 0) {
        closing_speed = scratch.closing_speed;
    }
    direct_clear = (hit == 0);

    if (movement_mode != 0) {
        direction = delta;
        vector3d_normalize_with_length(&direction);

        if (movement_mode == 1) {
            // UNSURE: Ghidra shows the same three-call shape for both the direct_clear and
            // !direct_clear paths, differing only in which of two nearby origin points (both
            // computed from DAT_00672b8c) each call uses; collapsed here to one point.
            candidate_point.x = direction.i * DAT_00672b8c + self_position->x;
            candidate_point.y = direction.j * DAT_00672b8c + self_position->y;
            candidate_point.z = direction.k * DAT_00672b8c + self_position->z;

            hit = FUN_00401a20(target_position, &candidate_point, collision_mask, exclude_object_index, &scratch);
            if (!direct_clear && hit == 0) {
                return 1;
            }
            hit = FUN_00401a20(target_position, &candidate_point, collision_mask, exclude_object_index, &scratch);
            if (hit != 0) {
                return 1;
            }
            hit = FUN_00401a20(target_position, &candidate_point, collision_mask, exclude_object_index, &scratch);
        } else {
            if (!direct_clear) {
                goto low_speed_grading;
            }
            point3d_add_scaled();
            candidate_point.x = direction.i * DAT_00672bac + self_position->x;
            candidate_point.y = direction.j * DAT_00672bac + self_position->y;
            candidate_point.z = direction.k * DAT_00672bac + self_position->z;

            hit = FUN_00401a20(self_position, &candidate_point, collision_mask, exclude_object_index, &scratch);
            if (hit != 0) {
                return 1;
            }
            hit = FUN_00401a20(self_position, &candidate_point, collision_mask, exclude_object_index, &scratch);
            if (hit != 0) {
                return 1;
            }
            hit = FUN_00401a20(self_position, &candidate_point, collision_mask, exclude_object_index, &scratch);
        }
        if (hit != 0) {
            return 1;
        }
    }

    if (direct_clear) {
        return 0;
    }

low_speed_grading:
    distance = (real)sqrt((double)(delta.i * delta.i + delta.j * delta.j + delta.k * delta.k));
    if (1.0f <= distance) {
        if (distance * closing_speed < 1.0f) {
            return 2;
        }
        if ((1.0f - closing_speed) * distance < 4.0f) {
            return 3;
        }
    }
    return 4;
}

#if 0
Original Ghidra decompilation (0x42b270):

undefined4 FUN_0042b270(short param_1,char param_2,undefined4 param_3,char param_4)

{
  float fVar1;
  char cVar2;
  short in_AX;
  short in_CX;
  uint uVar3;
  float *unaff_ESI;
  float *unaff_EDI;
  bool local_81;
  float local_60;
  undefined1 local_50 [20];
  float local_3c;

  if (((in_AX != -1) && (in_CX != -1)) && (cVar2 = FUN_0053eb60(), cVar2 == '\0')) {
    return 4;
  }
  uVar3 = (-(uint)(param_2 != '\0') & 0xc) + 0xc2a7;
  if (param_4 != '\0') {
    uVar3 = uVar3 & 0xfffffdff;
  }
  cVar2 = FUN_00505880(uVar3);
  if (cVar2 != '\0') {
    local_60 = local_3c;
  }
  local_81 = cVar2 == '\0';
  if (param_1 != 0) {
    vector3d_normalize_with_length();
    if (param_1 == 1) {
      if (local_81) {
        cVar2 = FUN_00401a20(uVar3,param_3,local_50);
        goto joined_r0x0042b3f7;
      }
      cVar2 = FUN_00401a20(uVar3,param_3,local_50);
      if (cVar2 == '\0') {
        return 1;
      }
      cVar2 = FUN_00401a20(uVar3,param_3,local_50);
      cVar2 = '\x01' - (cVar2 != '\0');
    }
    else {
      if (!local_81) goto LAB_0042b535;
      point3d_add_scaled();
      cVar2 = FUN_00401a20(uVar3,param_3,local_50);
      if (cVar2 != '\0') {
        return 1;
      }
      cVar2 = FUN_00401a20(uVar3,param_3,local_50);
joined_r0x0042b3f7:
      if (cVar2 != '\0') {
        return 1;
      }
      cVar2 = FUN_00401a20(uVar3,param_3,local_50);
    }
    if (cVar2 != '\0') {
      return 1;
    }
  }
  if (local_81) {
    return 0;
  }
LAB_0042b535:
  fVar1 = SQRT((*unaff_ESI - *unaff_EDI) * (*unaff_ESI - *unaff_EDI) +
               (unaff_ESI[1] - unaff_EDI[1]) * (unaff_ESI[1] - unaff_EDI[1]) +
               (unaff_ESI[2] - unaff_EDI[2]) * (unaff_ESI[2] - unaff_EDI[2]));
  if (1.0 <= fVar1) {
    if (fVar1 * local_60 < 1.0) {
      return 2;
    }
    if ((1.0 - local_60) * fVar1 < 4.0) {
      return 3;
    }
  }
  return 4;
}

Key facts recovered from the real disassembly (0x42b270-0x42b5c2), used because Ghidra
dropped register arguments to FUN_00505880 and FUN_00401a20 entirely:

0042b2c7: mov ebx,[esp+0x98]     ; ebx = exclude_object_index (param_3), held for the rest
                                  ; of the function -- the pre-adjustment mask value it
                                  ; overwrites is never used again.
0042b2e6: push ebp                ; the FUN_00505880 call pushes, in order: scratch, ebx
0042b2e5: push edi                ;   (exclude_object_index), &delta, edi (origin =
0042b2d5: push ebx                ;   self_position), ebp (mask) -- i.e.
0042b2d4: push eax                ;   FUN_00505880(ebp, edi, &delta, ebx, &scratch), matching
0042b2f5: call 0x505880           ;   this project already established FUN_00505880 signature.
0042b3e4: push ecx (&scratch2)    ; every FUN_00401a20 call site pushes (out_record,
0042b3e5: push ebx (exclude)      ;   exclude_object_index, mask) in that push order --
0042b3e6: push ebp (mask)         ;   i.e. FUN_00401a20(target, origin, mask,
0042b3e7: mov ecx,esi (target)    ;   exclude_object_index, out_record) with ECX/EAX supplying
0042b3e9: lea eax,... (origin)    ;   target/origin, matching the signature this project
0042b3ed: call 0x401a20           ;   already established in src/projectiles/projectile_collision_test.c.
#endif
