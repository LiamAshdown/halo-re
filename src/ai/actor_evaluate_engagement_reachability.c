// actor_evaluate_engagement_reachability  (Ghidra: FUN_0042b270)
// address 0x42b270, size 851 bytes
// name confidence: 0.4   rewrite confidence: 0.85
// REWRITTEN from objdump 0x42b270..0x42b5c2. The register arguments are two cluster indices (AX, CX; -1 skips the
//   visibility gate, scenario_cluster_visibility_test 0x53eb60 with CX column, stack row) and the two positions (ESI
//   target, EDI self); the draft took actor indices and its callers passed none of the four.
//   - blocked outright between the clusters: 4;
//   - a direct sweep self -> target (collision_test_movement_segment 0x505880, mask 0xc2a7, or 0xc2b3 with
//     allow_wide_mask, less 0x200 when flying) with the hit's +0x14 fraction kept;
//   - movement_mode 1 sidesteps: from self +/- 0.25 along the horizontal perpendicular of self->target (the forward
//     vector when degenerate) to the target. A clear direct line whose sidesteps are both clear gives 0, a blocked
//     sidestep 1; a blocked direct line with a clear sidestep gives 1;
//   - movement_mode 2 (only when the direct line is clear): from target +/- 0.1 along that perpendicular, and from
//     target + 0.1 up (0x69672c), back to self: any blocked gives 1, else 0;
//   - otherwise a clear direct line gives 0, and a blocked one is graded by its length d and hit fraction f: 4 under
//     1 unit, 2 when d*f < 1, 3 when d*(1-f) < 4, else 4.
// blam-cc: AX -> self_cluster, CX -> target_cluster, ESI -> target_position, EDI -> self_position,
//   stack -> movement_mode, allow_wide_mask, exclude_object_index, flying

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"

extern double sqrt(double x);

extern const real_vector3d *global_forward3d_pointer; // 0x00696718
// FIXED 2026-09-27 (static loop): 0x42b45e reads 0x0069672c, which points at (0, 0, -1); the name
// global_up3d_pointer is bound to 0x00696720 (0, 0, 1) by every other file, so the draft offset the point UP.
extern const real_vector3d *global_down3d_pointer;    // 0x0069672c -> 0x0065c25c (0, 0, -1)

extern uint8_t scenario_cluster_visibility_test(int16_t row_cluster, int16_t column_cluster); // 0x53eb60, stack, CX
extern real vector3d_normalize_with_length(real_vector3d *v); // 0x401990, ECX
extern void point3d_add_scaled(real_point3d *out, real_vector3d *direction, real_point3d *base, real scale); // 0x401930, EAX, ECX, stack
extern uint8_t collision_test_movement_segment(uint32_t flags, real_point3d *origin, real_vector3d *delta,
    uint32_t exclude_object, void *result); // 0x505880
extern uint8_t collision_test_movement_segment_between_points(real_point3d *origin, real_point3d *target,
    uint32_t flags, uint32_t exclude_object_index, void *result); // 0x401a20, EAX, ECX, stack

int32_t actor_evaluate_engagement_reachability(int16_t self_cluster, int16_t target_cluster,
    real_point3d *target_position, real_point3d *self_position, int16_t movement_mode, uint8_t allow_wide_mask,
    datum_index exclude_object_index, uint8_t flying)
{
    uint8_t result[0x50]; // [esp+0x3c] to the end of the frame
    uint32_t mask;
    real_vector3d delta;
    real_vector3d side;
    real_point3d a;
    real_point3d b;
    uint8_t direct_clear;
    float fraction = 0.0f;

    if (self_cluster != -1 && target_cluster != -1 && !scenario_cluster_visibility_test(self_cluster, target_cluster)) {
        return 4;
    }
    mask = allow_wide_mask ? 0xc2b3 : 0xc2a7;
    if (flying) {
        mask &= 0xfffffdff;
    }
    delta.i = target_position->x - self_position->x;
    delta.j = target_position->y - self_position->y;
    delta.k = target_position->z - self_position->z;
    if (!collision_test_movement_segment(mask, self_position, &delta, exclude_object_index, result)) {
        direct_clear = 1;
    } else {
        direct_clear = 0;
        fraction = *(float *)(result + 0x14);
    }

    if (movement_mode != 0) {
        side.i = self_position->y - target_position->y;
        side.j = target_position->x - self_position->x;
        side.k = 0.0f;
        if (vector3d_normalize_with_length(&side) == 0.0f) {
            side = *global_forward3d_pointer;
        }
        if (movement_mode == 1) {
            real_vector3d offset;

            offset.i = side.i * 0.25f;
            offset.j = side.j * 0.25f;
            offset.k = side.k * 0.25f;
            a.x = offset.i + self_position->x;
            a.y = offset.j + self_position->y;
            a.z = offset.k + self_position->z;
            b.x = self_position->x - offset.i;
            b.y = self_position->y - offset.j;
            b.z = self_position->z - offset.k;
            if (direct_clear) {
                if (collision_test_movement_segment_between_points(&a, target_position, mask, exclude_object_index, result) ||
                    collision_test_movement_segment_between_points(&b, target_position, mask, exclude_object_index, result)) {
                    return 1;
                }
                return 0;
            }
            if (!collision_test_movement_segment_between_points(&a, target_position, mask, exclude_object_index, result) ||
                !collision_test_movement_segment_between_points(&b, target_position, mask, exclude_object_index, result)) {
                return 1;
            }
        } else if (direct_clear) {
            real_vector3d offset;
            real_point3d raised;

            offset.i = side.i * 0.1f;
            offset.j = side.j * 0.1f;
            offset.k = side.k * 0.1f;
            a.x = offset.i + target_position->x;
            a.y = offset.j + target_position->y;
            a.z = offset.k + target_position->z;
            b.x = target_position->x - offset.i;
            b.y = target_position->y - offset.j;
            b.z = target_position->z - offset.k;
            point3d_add_scaled(&raised, (real_vector3d *)global_down3d_pointer, target_position, 0.1f);
            if (collision_test_movement_segment_between_points(&a, self_position, mask, exclude_object_index, result) ||
                collision_test_movement_segment_between_points(&b, self_position, mask, exclude_object_index, result) ||
                collision_test_movement_segment_between_points(&raised, self_position, mask, exclude_object_index, result)) {
                return 1;
            }
            return 0;
        }
    } else if (direct_clear) {
        return 0;
    }

    {
        float dx = target_position->x - self_position->x;
        float dy = target_position->y - self_position->y;
        float dz = target_position->z - self_position->z;
        float distance = (float)sqrt((double)(dz * dz + dy * dy + dx * dx));

        if (distance < 1.0f) {
            return 4;
        }
        if (distance * fraction < 1.0f) {
            return 2;
        }
        if ((1.0f - fraction) * distance < 4.0f) {
            return 3;
        }
        return 4;
    }
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
