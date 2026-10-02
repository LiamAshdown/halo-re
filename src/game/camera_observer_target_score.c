// camera_observer_target_score  (Ghidra: FUN_00459b10; renamed per symbols/review_queue.txt)
// address 0x459b10, size 419 bytes
// name confidence: 0.3   rewrite confidence: 0.95
// evidence: types/game.h observer_target_candidate / observer_target_cone; the acos operand and
//   the EAX/EBX aliasing below were read directly out of the disassembly
//   (objdump -d -M intel --start-address=0x459b10 --stop-address=0x459cb0 bin/halo.exe), because
//   Ghidra elides both the vector3d_normalize_with_length and FUN_00628140 (acos) call
//   arguments here. Object tag flag bit 0x80000 = UnitFlags::inconsequential (types/tags.h).
// register convention: cone pointer in EAX (in_EAX, Ghidra's "param_1"), target object handle in
//   ECX (in_ECX), output candidate pointer in ESI (unaff_ESI); reference position is the
//   recognized stack parameter (param_2). A second stack slot the caller reserves is never
//   read by this function, so it is not a parameter here.
//   // blam-cc: EAX -> cone, ECX -> object, ESI -> out, stack -> reference_position
//
// The angle is acos(dot(direction, facing)), clamped to [-1, 1], where `facing` is the vector in EAX/EBX (0x459b77..0x459b79);
// the cone's angle_a/distance_a/angle_b/distance_b are the falloff bounds (offsets 0/4/8/0xc) fed to distance_falloff_fraction.
// The secondary-weight multiplier read through global_globals+0x114+8 is Globals::player_control.pointer ->
// GlobalsPlayerControl::inconsequential_target_scale.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"
#include "game.h"

extern data_array *object_data;         // 0x008603b0
extern tag_instance *tag_instances;     // 0x0087bc14
extern Globals *global_globals;         // 0x00746fa0
extern real vector3d_normalize_with_length(real_vector3d *v); // 0x401990, vector in ECX
extern double acos(double x);           // 0x628140, CRT/compiler helper; operand on the x87 stack
extern real distance_falloff_fraction(real value, real max_range); // this batch, 0x459360
extern void vector3d_closest_point_on_segment(datum_index unit_index, real_vector3d *aux_vector,
    real_point3d *reference_point, real_point3d *out_closest); // 0x45a280, ECX, EBX, stack

// Fills in one observer_target_candidate: the closest point on the target's look ray to
// `reference_position` (via vector3d_closest_point_on_segment), the offset/direction/distance
// from that point, the angle between that direction and `facing`, and the two falloff-weighted scores. Returns 1 when either weight is
// positive.
// REWRITTEN 2026-09-27 (static loop) from objdump 0x459b10..0x459cb2: EAX is the FACING vector (EBX: the
// closest-point aux vector and the angle's dot product), the stack carries (cone, reference_position). The draft
// merged facing and cone into one pointer and took the dot product against the cone's four falloff floats.
uint32_t camera_observer_target_score(real_vector3d *facing, observer_target_cone *cone, datum_index target,
                                       observer_target_candidate *out, real_point3d *reference_position)
    // blam-cc: EAX -> facing, ECX -> object, ESI -> out, stack -> cone, reference_position
{
    real dot;
    real angle;
    object *target_object;
    Unit *target_tag;

    out->object = target;
    // 0x459b21: ECX = target, EBX = facing, stack (reference_position, &out->point)
    vector3d_closest_point_on_segment(target, facing, reference_position, &out->point);

    out->offset.i = out->point.x - reference_position->x;
    out->offset.j = out->point.y - reference_position->y;
    out->offset.k = out->point.z - reference_position->z;
    out->direction = out->offset;
    out->distance = vector3d_normalize_with_length(&out->direction);

    dot = out->direction.k * facing->k + out->direction.j * facing->j + out->direction.i * facing->i;
    if (dot < -1.0f) {
        dot = -1.0f;
    } else if (1.0f < dot) {
        dot = 1.0f;
    }
    angle = (real)acos((double)dot);
    out->angle = angle;

    if (cone == (observer_target_cone *)0) {
        out->weight_primary = 0.0f;
        out->weight_secondary = 0.0f;
    } else {
        out->weight_primary = distance_falloff_fraction(angle, cone->angle_a) *
                               distance_falloff_fraction(out->distance, cone->distance_a);
        out->weight_secondary = distance_falloff_fraction(angle, cone->angle_b) *
                                 distance_falloff_fraction(out->distance, cone->distance_b);
        if (0.0f < out->weight_secondary) {
            target_object = ((object_header *)object_data->data)[target & 0xffff].data;
            target_tag = (Unit *)tag_instances[target_object->definition_tag & 0xffff].data;
            if ((target_tag->unit_flags & 0x80000) != 0) { // UnitFlags::inconsequential
                out->weight_secondary = out->weight_secondary *
                    ((GlobalsPlayerControl *)global_globals->player_control.pointer)
                        ->inconsequential_target_scale; // RESOLVED: globals+0x114 is
                        // player_control.pointer (Globals: 0xf8 + 2 reflexives), and +0x08 of
                        // GlobalsPlayerControl is inconsequential_target_scale
            }
        }
    }

    return (out->weight_primary > 0.0f || out->weight_secondary > 0.0f) ? 1 : 0;
}

#if 0
Original Ghidra decompilation (0x459b10), from tools/pack.py 0x459b10:

undefined4 FUN_00459b10(undefined4 *param_1,float *param_2)

{
  float fVar1;
  uint in_ECX;
  uint *unaff_ESI;
  float10 fVar2;
  float10 fVar3;
  float10 fVar4;
  float10 fVar5;

  *unaff_ESI = in_ECX;
  FUN_0045a280(param_2,unaff_ESI + 1);
  unaff_ESI[4] = (uint)((float)unaff_ESI[1] - *param_2);
  unaff_ESI[5] = (uint)((float)unaff_ESI[2] - param_2[1]);
  unaff_ESI[6] = (uint)((float)unaff_ESI[3] - param_2[2]);
  unaff_ESI[7] = unaff_ESI[4];
  unaff_ESI[8] = unaff_ESI[5];
  unaff_ESI[9] = unaff_ESI[6];
  fVar2 = (float10)vector3d_normalize_with_length();
  unaff_ESI[10] = (uint)(float)fVar2;
  fVar3 = (float10)FUN_00628140();
  unaff_ESI[0xb] = (uint)(float)fVar3;
  if (param_1 == (undefined4 *)0x0) {
    unaff_ESI[0xc] = 0;
    unaff_ESI[0xd] = 0;
  }
  else {
    fVar4 = (float10)FUN_00459360((float)fVar2,param_1[1]);
    fVar5 = (float10)FUN_00459360((float)fVar3,*param_1);
    unaff_ESI[0xc] = (uint)(float)(fVar5 * (float10)(float)fVar4);
    fVar3 = (float10)FUN_00459360((float)fVar3,param_1[2]);
    fVar2 = (float10)FUN_00459360((float)fVar2,param_1[3]);
    fVar1 = (float)(fVar2 * (float10)(float)fVar3);
    unaff_ESI[0xd] = (uint)fVar1;
    if ((0.0 < fVar1) &&
       ((*(uint *)(*(int *)((**(uint **)(*(int *)(DAT_008603b0 + 0x34) + 8 +
                                        (*unaff_ESI & 0xffff) * 0xc) & 0xffff) * 0x20 + 0x14 +
                           DAT_0087bc14) + 0x17c) & 0x80000) != 0)) {
      unaff_ESI[0xd] = (uint)(fVar1 * *(float *)(*(int *)(DAT_00746fa0 + 0x114) + 8));
    }
  }
  if (((float)unaff_ESI[0xc] <= 0.0) && ((float)unaff_ESI[0xd] <= 0.0)) {
    return 0;
  }
  return 1;
}
#endif
