// unit_apply_impulse_to_seat  (Ghidra: FUN_00571cb0; kept the phase2 proposal's name, though
//   the rewrite below suggests it is not actually seat-specific -- see UNSURE)
// address 0x571cb0, size 286 bytes
// name confidence: 0.4 (phase2 proposal at 0.4)
// rewrite confidence: 0.4
// evidence: types/objects.h object.velocity (0x068), .angular_velocity (0x08c), .flags (0x010);
//   the physics.tag_id-at-0x8c idiom matches every other use in this module.
// register convention: unit object index in ECX (in_ECX); an impulse vector pointer in EAX
//   (in_EAX).
//   // blam-cc: ECX -> unit_index, EAX -> impulse
// UNSURE: the phase2 proposal read the guard condition as "is seated in something", but it is
//   the same tag->physics.tag_id != -1 check used everywhere else in this module for "does this
//   object's tag define a physics reference" -- kept the established name since it is already
//   used at this function's call site in unit_melee_attack_scan.c, but the "seat" framing may be
//   wrong.
// UNSURE: the byte written at object+0x524 (vehicle_data.unknown_524 for a vehicle) has no
//   confirmed meaning in this module beyond "cleared by 0x5724d0 after a film snapshot".

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"

extern data_array *object_data;         // 0x008603b0
extern tag_instance *tag_instances;     // 0x0087bc14
extern real_vector3d *global_up3d_pointer;  // 0x00696720

extern real vector3d_normalize_with_length(real_vector3d *v); // 0x401990, in place

// Applies a linear impulse to the unit's velocity and, if the resulting up-cross-impulse axis is
// non-degenerate, an angular impulse (scaled by pi times its length) to its angular velocity --
// but only when the unit's own tag defines a physics reference. Also clears the
// extension_of_parent flag and marks object+0x524 dirty.
void unit_apply_impulse_to_seat(uint32_t unit_index, real_vector3d *impulse)
{
    object *obj = ((object_header *)object_data->data)[unit_index & 0xffff].data;
    uint8_t *tag = (uint8_t *)tag_instances[obj->definition_tag & 0xffff].data;

    if (*(int32_t *)&((Unit *)tag)->base.physics.tag_id == -1) {
        return;
    }

    obj->velocity.i += impulse->i;
    obj->velocity.j += impulse->j;
    obj->velocity.k += impulse->k;

    {
        real_vector3d axis;
        real length;

        axis.i = global_up3d_pointer->j * impulse->k - impulse->j * global_up3d_pointer->k;
        axis.j = impulse->i * global_up3d_pointer->k - impulse->k * global_up3d_pointer->i;
        axis.k = impulse->j * global_up3d_pointer->i - impulse->i * global_up3d_pointer->j;
        length = vector3d_normalize_with_length(&axis);

        if (length > 0.0f) {
            float scale = length * 3.1415927f;
            obj->angular_velocity.i += axis.i * scale;
            obj->angular_velocity.j += axis.j * scale;
            obj->angular_velocity.k += axis.k * scale;
        }
    }

    obj->flags &= ~0x20u;
    *((uint8_t *)obj + 0x524) = 1;
}

#if 0
Original Ghidra decompilation (0x571cb0):

void FUN_00571cb0(void)

{
  float fVar1;
  float fVar2;
  float fVar3;
  float fVar4;
  float fVar5;
  float fVar6;
  float fVar7;
  float fVar8;
  float fVar9;
  float fVar10;
  float fVar11;
  float fVar12;
  uint *puVar13;
  undefined *puVar14;
  float *in_EAX;
  uint in_ECX;
  float10 fVar15;

  puVar14 = PTR_DAT_00696720;
  puVar13 = *(uint **)(*(int *)(DAT_008603b0 + 0x34) + 8 + (in_ECX & 0xffff) * 0xc);
  if (*(int *)(*(int *)((*puVar13 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14) + 0x8c) != -1) {
    puVar13[0x1a] = (uint)((float)puVar13[0x1a] + *in_EAX);
    puVar13[0x1b] = (uint)((float)puVar13[0x1b] + in_EAX[1]);
    puVar13[0x1c] = (uint)((float)puVar13[0x1c] + in_EAX[2]);
    fVar1 = *(float *)(puVar14 + 4);
    fVar2 = in_EAX[2];
    fVar3 = in_EAX[1];
    fVar4 = *(float *)(puVar14 + 8);
    fVar5 = *in_EAX;
    fVar6 = *(float *)(puVar14 + 8);
    fVar7 = in_EAX[2];
    fVar8 = *(float *)puVar14;
    fVar9 = in_EAX[1];
    fVar10 = *(float *)puVar14;
    fVar11 = *(float *)(puVar14 + 4);
    fVar12 = *in_EAX;
    fVar15 = (float10)vector3d_normalize_with_length();
    if ((float10)0.0 < fVar15) {
      fVar15 = fVar15 * (float10)3.1415927;
      puVar13[0x23] =
           (uint)(float)((float10)(fVar1 * fVar2 - fVar3 * fVar4) * fVar15 +
                        (float10)(float)puVar13[0x23]);
      puVar13[0x24] =
           (uint)((float)((float10)(fVar5 * fVar6 - fVar7 * fVar8) * fVar15) + (float)puVar13[0x24])
      ;
      puVar13[0x25] =
           (uint)((float)((float10)(fVar9 * fVar10 - fVar11 * fVar12) * fVar15) +
                 (float)puVar13[0x25]);
    }
    puVar13[4] = puVar13[4] & 0xffffffdf;
    *(undefined1 *)(puVar13 + 0x149) = 1;
  }
  return;
}
#endif
