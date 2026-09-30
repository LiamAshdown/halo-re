// unit_get_average_active_marker_direction  (Ghidra: FUN_00575e30; renamed from the phase2
//   proposal)
// address 0x575e30, size 365 bytes
// name confidence: 0.3 (phase2 proposal at 0.3, matches functions.md summary)
// rewrite confidence: 0.85 -- REWRITTEN from the disassembly. The old version passed a single
//   pointer as the 0x3c-byte object_physics_context (the build call wrote past it), transformed
//   through an all-zero matrix and discarded the result, and returned position - average (the
//   wrong sign). The binary averages the active mass points in physics space, transforms the
//   average by the context matrix (ctx + 8), and returns normalize(world average - unit
//   position).
// evidence: 0x575e68 object_physics_context_build (EBX index, EAX context at esp+0x28);
//   0x575e81 Physics tag (ctx + 4) +0x74 mass point count, +0x78 mass points, 0x80 stride,
//   position at +0x38; 0x575e75 sum starts from *global_zero_vector3d_pointer (0x6966f8);
//   0x575f00 inv = 1.0 / count (int16); 0x575f2c matrix4x3_transform_point(EAX out, EDX sum,
//   stack ctx + 8); 0x575f3c object_get_position; 0x575f41 out = transformed - position;
//   0x575f70 fails when the length compares equal to 0.0.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"
#include "physics.h"
#include "units.h"
#include "fn_math.h"
#include "fn_physics.h"

extern data_array *object_data;     // 0x008603b0
extern real_point3d *global_zero_vector3d_pointer; // 0x006966f8 -> 0x0065c230 {0,0,0}


extern void matrix4x3_transform_point(real_point3d *out, real_point3d *point, real_matrix4x3 *m); // 0x4cbde0
extern void object_get_position(real_point3d *out, uint32_t object_index); // 0x4f6900


// Direction from the unit's position to the world-space average of the mass points selected by
// vehicle_data.active_marker_mask (+0x520). Fails when no bit is set, when the object has no
// physics, when no selected mass point exists, or when the direction has zero length.
// blam-cc: stack -> unit_index, out_direction
uint8_t unit_get_average_active_marker_direction(uint32_t unit_index, real_vector3d *out_direction)
{
    uint8_t *obj = (uint8_t *)((object_header *)object_data->data)[unit_index & 0xffff].data;
    uint32_t mask = *(uint32_t *)(obj + 0x520);
    object_physics_context ctx;
    uint8_t *physics;
    int32_t count;
    int16_t active_count = 0;
    int16_t i;
    real_point3d sum;
    real_point3d world;
    real_point3d position;
    float inv;

    if (mask == 0) {
        return 0;
    }
    if (!object_physics_context_build(unit_index, &ctx)) {
        return 0;
    }

    physics = (uint8_t *)ctx.definition;
    count = *(int32_t *)(physics + 0x74);
    sum = *global_zero_vector3d_pointer;
    if (count <= 0) {
        return 0;
    }
    i = 0;
    do {
        if ((mask & (1u << (i & 0x1f))) != 0) {
            real_point3d *p = (real_point3d *)(*(uint8_t **)(physics + 0x78) + (int32_t)i * 0x80 + 0x38);
            sum.x += p->x;
            sum.y += p->y;
            sum.z += p->z;
            active_count++;
        }
        i++;
    } while ((int32_t)i < count);

    if (active_count <= 0) {
        return 0;
    }
    inv = 1.0f / (float)(int32_t)active_count;
    sum.x *= inv;
    sum.y *= inv;
    sum.z *= inv;
    matrix4x3_transform_point(&world, &sum, (real_matrix4x3 *)&ctx.scale);
    object_get_position(&position, unit_index);
    out_direction->i = world.x - position.x;
    out_direction->j = world.y - position.y;
    out_direction->k = world.z - position.z;
    if (vector3d_normalize_with_length(out_direction) == 0.0f) {
        return 0;
    }
    return 1;
}

#if 0
Original Ghidra decompilation (0x575e30):

uint FUN_00575e30(uint param_1,float *param_2)

{
  int iVar1;
  float fVar2;
  float fVar3;
  float fVar4;
  uint uVar5;
  float fVar6;
  undefined2 extraout_var;
  int iVar7;
  short sVar8;
  short sVar9;
  float10 fVar10;
  float10 fVar11;
  float local_48;
  float local_44;
  float local_40;
  int local_38;
  undefined1 local_34 [52];

  iVar1 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (param_1 & 0xffff) * 0xc);
  fVar6 = (float)((param_1 & 0xffff) * 3);
  if (*(int *)(iVar1 + 0x520) != 0) {
    uVar5 = FUN_005074b0();
    if ((char)uVar5 == '\0') {
      return uVar5 & 0xffffff00;
    }
    fVar2 = *(float *)PTR_DAT_006966f8;
    fVar3 = *(float *)(PTR_DAT_006966f8 + 4);
    fVar4 = *(float *)(PTR_DAT_006966f8 + 8);
    sVar8 = 0;
    sVar9 = 0;
    fVar6 = fVar4;
    if (0 < *(int *)(local_38 + 0x74)) {
      iVar7 = 0;
      do {
        fVar6 = (float)(1 << ((byte)iVar7 & 0x1f));
        if ((*(uint *)(iVar1 + 0x520) & (uint)fVar6) != 0) {
          fVar2 = fVar2 + *(float *)(iVar7 * 0x80 + 0x38 + *(int *)(local_38 + 0x78));
          fVar6 = (float)(iVar7 * 0x80 + 0x38 + *(int *)(local_38 + 0x78));
          sVar8 = sVar8 + 1;
          fVar3 = fVar3 + *(float *)((int)fVar6 + 4);
          fVar4 = fVar4 + *(float *)((int)fVar6 + 8);
        }
        sVar9 = sVar9 + 1;
        iVar7 = (int)sVar9;
      } while (iVar7 < *(int *)(local_38 + 0x74));
      if (0 < sVar8) {
        fVar6 = 1.0 / (float)(int)sVar8;
        matrix4x3_transform_point(local_34);
        object_get_position();
        *param_2 = local_48 - fVar2 * fVar6;
        param_2[1] = local_44 - fVar3 * fVar6;
        param_2[2] = local_40 - fVar4 * fVar6;
        fVar10 = (float10)vector3d_normalize_with_length();
        fVar11 = (float10)0.0;
        fVar6 = (float)CONCAT22(extraout_var,
                                (ushort)(fVar11 < fVar10) << 8 |
                                (ushort)(NAN(fVar11) || NAN(fVar10)) << 10 |
                                (ushort)(fVar11 == fVar10) << 0xe);
        if (fVar11 != fVar10) {
          return CONCAT31((int3)((uint)fVar6 >> 8),1);
        }
      }
    }
  }
  return (uint)fVar6 & 0xffffff00;
}
#endif
