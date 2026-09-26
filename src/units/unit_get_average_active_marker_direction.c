// unit_get_average_active_marker_direction  (Ghidra: FUN_00575e30; renamed from the phase2
//   proposal)
// address 0x575e30, size 365 bytes
// name confidence: 0.3 (phase2 proposal at 0.3, matches functions.md summary)
// rewrite confidence: 0.15 -- local_38 (the Physics tag data pointer this function indexes at
//   +0x74/+0x78) is never assigned anywhere in Ghidra's own decompile, which means it comes
//   from a hidden output of object_physics_context_build that could not be recovered; modeled here as an
//   explicit out-parameter of that call.
// evidence: types/units.h vehicle_data.active_marker_mask (0x520, "0x575e30 treats it as a
//   marker bitmask"); math.h global_up3d_and_neighbors_pointer (0x006966f8); callees
//   matrix4x3_transform_point, object_get_position, vector3d_normalize_with_length.
// UNSURE: object_physics_context_build's real signature/output and the marker-position field offset (+0x38
//   within an 0x80-byte physics contact record, by analogy with unit_update_marker_traction_effects.c's
//   own 0x80-stride record) are not confirmed.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"
#include "units.h"

extern data_array *object_data;     // 0x008603b0
extern real_vector3d *global_up3d_and_neighbors_pointer; // 0x006966f8, UNSURE identity

extern uint8_t object_physics_context_build(uint32_t unit_index, uint8_t **out_physics_tag); // 0x5074b0, UNSURE signature
extern void matrix4x3_transform_point(real_point3d *out, real_point3d *point, real_matrix4x3 *m); // 0x4cbde0
extern void object_get_position(real_point3d *out, uint32_t object_index); // 0x4f6900
extern real vector3d_normalize_with_length(real_vector3d *v); // 0x401990, in place

// Computes and returns the normalized direction from the unit toward the averaged position of
// its currently active markers (vehicle_data.active_marker_mask), or fails if no markers are
// active or object_physics_context_build fails.
// FIXED (register inputs, objdump: each stack slot's first use checked against the parameter): the original never reads EAX; unit_index arrive(s) on the stack (2 stack argument(s)).
// blam-cc: stack -> unit_index, out_direction
uint8_t unit_get_average_active_marker_direction(uint32_t unit_index, real_vector3d *out_direction)
{
    object *obj = ((object_header *)object_data->data)[unit_index & 0xffff].data;
    vehicle_data *vehicle = (vehicle_data *)((uint8_t *)obj + k_unit_object_size);

    if (vehicle->active_marker_mask == 0) {
        return 0;
    }

    {
        uint8_t *physics_tag = 0;
        uint8_t ok = object_physics_context_build(unit_index, &physics_tag);
        real_vector3d sum;
        int32_t active_count = 0;
        int32_t count;
        int32_t i;

        if (!ok) {
            return 0;
        }

        sum = *global_up3d_and_neighbors_pointer;
        count = *(int32_t *)(physics_tag + 0x74);

        for (i = 0; i < count; i++) {
            if ((vehicle->active_marker_mask & (1u << (i & 0x1f))) != 0) {
                real_point3d *marker_pos = (real_point3d *)(*(uint8_t **)(physics_tag + 0x78) + i * 0x80 + 0x38);
                sum.i += marker_pos->x;
                sum.j += marker_pos->y;
                sum.k += marker_pos->z;
                active_count++;
            }
        }

        if (active_count > 0) {
            float inv = 1.0f / (float)active_count;
            real_matrix4x3 basis = {0}; // UNSURE: local_34, never assigned in the decompile
            real_point3d transformed;
            real_point3d self_position;
            real length;

            matrix4x3_transform_point(&transformed, (real_point3d *)&sum, &basis);
            object_get_position(&self_position, unit_index);

            out_direction->i = self_position.x - sum.i * inv;
            out_direction->j = self_position.y - sum.j * inv;
            out_direction->k = self_position.z - sum.k * inv;
            length = vector3d_normalize_with_length(out_direction);
            if (length != 0.0f) {
                return 1;
            }
        }
    }
    return 0;
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
