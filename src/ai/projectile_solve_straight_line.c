// projectile_solve_straight_line  (Ghidra: FUN_004bee20; renamed)
// address 0x4bee20, size 147 bytes
// name confidence: 0.35   rewrite confidence: 0.6
// evidence: out/phase4/projectiles_types_notes.md "0x4beb30, 0x4bee20, 0x4beec0 are AI
//   ballistic-aiming helpers, not projectile state... 0x4bee20 just returns a direction, its
//   length and the length over the given speed"; src/projectiles/README.md "Misattributed
//   functions": "AI ballistic aiming: the straight-line solver". Its only caller is the
//   dispatcher projectile_get_aiming_vector (0x4beec0, this batch). cleanup pass 4 orphan pass:
//   neither projectiles nor items claimed this address; picked up here in ai, alongside its
//   sibling projectile_solve_ballistic_arc (0x4beb30).
// register convention: `objdump -d -M intel --start-address=0x4bee20 --stop-address=0x4bee55
//   bin/halo.exe` cross-checked against the caller (0x4beec0's straight-line branch): EAX =
//   target position (real_point3d *); three plain stack arguments follow (speed, origin
//   position, out_time_of_flight -- Ghidra recovered these three correctly as stack params,
//   confirmed by `mov ecx,[esp+0x14]` / `mov ebp,[esp+0x1c]` reading them before ESP is
//   otherwise disturbed); EBX = out_speed_echo (real *, nullable), ESI = out_direction
//   (real_vector3d *, unconditional), EDI = out_length (real *, nullable).
//   // blam-cc: EAX -> target, stack -> (speed, origin, out_time_of_flight),
//   //          EBX -> out_speed_echo, ESI -> out_direction, EDI -> out_length
// UNSURE: vector3d_normalize_with_length() is called with `lea ecx,[esp+0x4]`, the address of a
//   local scratch copy of (target - origin); its normalized result is discarded and only the
//   returned length is used -- *out_direction itself receives the UNNORMALIZED delta
//   (target - origin), recomputed straight from the two point arguments after the call. This
//   matches src/projectiles/README.md's "returns a direction[, not necessarily unit length,] its
//   length and the length over the given speed".
// UNSURE: the original's return value is built from `CONCAT31((int3)(fVar8>>8), 1)`, i.e. AL is
//   always 1 and the upper 24 bits are leftover float bits nothing reads (the sole caller,
//   projectile_get_aiming_vector, never inspects this function's return value); translated as
//   an unconditional `return 1`.

#include "tags.h"
#include "math.h"

extern real vector3d_normalize_with_length(real_vector3d *v); // 0x401990, math module

// Computes the straight-line direction (unnormalized, target - origin) from *origin to *target,
// its length, and the flight time at the given speed (0 when speed is not positive). Writes the
// direction to *out_direction unconditionally; out_speed_echo, out_length and out_time_of_flight
// may be NULL.
uint8_t projectile_solve_straight_line(real_point3d *target, real_point3d *origin, real speed,
    real *out_time_of_flight, real_vector3d *out_direction, real *out_speed_echo, real *out_length)
{
    real_vector3d scratch;
    real length, time_fraction;

    // Normalizes a scratch copy of the delta in place; only the returned length is kept.
    scratch.i = target->x - origin->x;
    scratch.j = target->y - origin->y;
    scratch.k = target->z - origin->z;
    length = vector3d_normalize_with_length(&scratch);

    if (speed <= 0.0f) {
        time_fraction = 0.0f;
    } else {
        time_fraction = length / speed;
    }

    out_direction->i = target->x - origin->x;
    out_direction->j = target->y - origin->y;
    out_direction->k = target->z - origin->z;

    if (out_length != (real *)0) {
        *out_length = length;
    }
    if (out_speed_echo != (real *)0) {
        *out_speed_echo = speed;
    }
    if (out_time_of_flight != (real *)0) {
        *out_time_of_flight = time_fraction;
    }
    return 1;
}

#if 0
Original Ghidra decompilation (0x4bee20):

undefined4 FUN_004bee20(float param_1,float *param_2,float *param_3)

{
  float fVar1;
  float fVar2;
  float fVar3;
  float fVar4;
  float fVar5;
  float fVar6;
  float fVar7;
  float *in_EAX;
  float fVar8;
  undefined4 uVar9;
  float *unaff_EBX;
  float *unaff_ESI;
  float *unaff_EDI;
  float10 fVar10;

  fVar8 = *in_EAX;
  fVar1 = *param_2;
  fVar2 = in_EAX[1];
  fVar3 = param_2[1];
  fVar4 = in_EAX[2];
  fVar5 = param_2[2];
  fVar10 = (float10)vector3d_normalize_with_length();
  fVar6 = (float)fVar10;
  if (param_1 <= 0.0) {
    fVar7 = 0.0;
  }
  else {
    fVar7 = fVar6 / param_1;
  }
  *unaff_ESI = fVar8 - fVar1;
  unaff_ESI[1] = fVar2 - fVar3;
  unaff_ESI[2] = fVar4 - fVar5;
  fVar8 = fVar8 - fVar1;
  if (unaff_EDI != (float *)0x0) {
    *unaff_EDI = fVar6;
    fVar8 = fVar6;
  }
  if (unaff_EBX != (float *)0x0) {
    *unaff_EBX = param_1;
  }
  uVar9 = CONCAT31((int3)((uint)fVar8 >> 8),1);
  if (param_3 != (float *)0x0) {
    *param_3 = fVar7;
    return uVar9;
  }
  return uVar9;
}
#endif
