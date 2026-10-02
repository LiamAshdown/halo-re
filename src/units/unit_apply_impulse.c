// unit_apply_impulse  (Ghidra: unit_apply_impulse, renamed)
// address 0x559fa0, size 455 bytes
// name confidence: 0.35   rewrite confidence: 0.35
// evidence: object.velocity at 0x068 (objects.h), object.angular_velocity at 0x08c (objects.h),
//   biped_data.flags at 0x4cc, object.flags at 0x010 (objects.h); tag offset 0x2f4 matches
//   Biped.biped_flags per types/tags.h; Unit tag offset 0x17c is UNSURE (not yet named).
// register convention: object index in EAX, the impulse vector in EDI.
//   // blam-cc: EAX -> object_index, EDI -> impulse
// UNSURE: vector3d_cross_product's implicit ECX operand (guessed here as the impulse itself);
//   the exact meaning of Unit-tag offset 0x17c bit 0x100000 (an "immovable" style gate) and of
//   biped_data.flags bits 1/2 being OR'd in wholesale (preserved as `|= 3`, not decoded further).

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"
#include "units.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern data_array *object_data;     // 0x008603b0
extern tag_instance *tag_instances; // 0x0087bc14
extern random_seed random_seed_global;   // 0x00719cd0, types/math.h
extern real_vector3d *global_up3d_pointer; // 0x00696720, indirect pointer to math.h global_up3d (0x0065c224)

extern real vector3d_normalize_with_length(real_vector3d *v); // 0x401990, in place, returns length, ECX
extern void vector3d_cross_product(real_vector3d *out, real_vector3d *ecx_operand, real_vector3d *stack_operand); // 0x4052c0
extern void unit_clear_ground_adjust_dirty(uint32_t object_index); // 0x55ad70, this batch: clears ground-adjust dirty flags
extern void unit_update_up_vector(Biped *biped_tag, object *obj); // 0x560800, next batch: levels the up-vector toward target
extern double sqrt(double x); // a single x87 FSQRT instruction in the original (Ghidra's SQRT())

// Adds a positional impulse to a unit: halves it first if the unit is attached to a parent
// (running the ground-adjust dirty-flag clear either way), accumulates it into velocity, marks
// the biped grounded/jumping flags, applies a small random angular jitter scaled by the impulse
// magnitude when the unit is unattended or its weapon-mode tag flag is set, and -- for an
// unattached unit only -- reorients its forward vector toward the impulse direction when it has
// a nonzero length.
void unit_apply_impulse(uint32_t object_index, real_vector3d *impulse)
{
    object *obj = ((object_header *)object_data->data)[object_index & 0xffff].data;
    void *tag_data = tag_instances[obj->definition_tag & 0xffff].data;
    unit_data *unit = (unit_data *)((uint8_t *)obj + k_unit_data_offset);
    biped_data *biped = (biped_data *)((uint8_t *)obj + k_unit_object_size);

    if ((*(uint32_t *)((uint8_t *)tag_data + 0x17c) & 0x100000) != 0) { // UNSURE offset, see file header
        return;
    }

    if ((obj->vitality_flags & 4) == 0) {
        impulse->i *= 0.5f;
        impulse->j *= 0.5f;
        impulse->k *= 0.5f;
    }
    unit_clear_ground_adjust_dirty(object_index);

    obj->velocity.i += impulse->i;
    obj->velocity.j += impulse->j;
    obj->velocity.k += impulse->k;
    obj->flags &= ~0x20u;
    biped->flags |= 3;

    if ((obj->vitality_flags & 4) != 0 || (*(uint8_t *)((uint8_t *)tag_data + 0x2f4) & 0x44) != 0) {
        real_vector3d jitter_axis;
        float length;
        // VERIFIED against disassembly 0x55a066..0x55a072 (2026-09-30): EAX=jitter_axis, ECX=edi=impulse, stack=[0x696720] global up
        vector3d_cross_product(&jitter_axis, impulse, global_up3d_pointer);
        length = vector3d_normalize_with_length(&jitter_axis);
        random_seed_global = random_seed_global * 0x19660d + 0x3c6ef35f;
        {
            float magnitude = (float)sqrt((double)(impulse->i * impulse->i + impulse->j * impulse->j +
                                                     impulse->k * impulse->k));
            float angle = (float)(int32_t)(random_seed_global >> 0x10) * 1.5259022e-05f * magnitude * 1.5707964f;
            obj->angular_velocity.i += jitter_axis.i * angle;
            obj->angular_velocity.j += jitter_axis.j * angle;
            obj->angular_velocity.k += jitter_axis.k * angle;
        }
    }

    if (obj->parent_object == k_datum_index_none) {
        real_vector3d direction = *impulse;
        float length = vector3d_normalize_with_length(&direction);
        if (length > 0.0f) {
            obj->forward = direction;
            // EAX -> the Biped tag, ECX -> the object (both register-carried)
            unit_update_up_vector((Biped *)tag_data, obj);
        }
    }
}

#if 0
Original Ghidra decompilation (0x559fa0):

void FUN_00559fa0(void)

{
  uint *puVar1;
  int iVar2;
  float fVar3;
  float fVar4;
  float fVar5;
  uint in_EAX;
  float *unaff_EDI;
  float10 fVar6;
  float local_c;
  float local_8;
  float local_4;

  puVar1 = *(uint **)(*(int *)(DAT_008603b0 + 0x34) + 8 + (in_EAX & 0xffff) * 0xc);
  iVar2 = *(int *)((*puVar1 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
  if ((*(uint *)(iVar2 + 0x17c) & 0x100000) != 0) {
    return;
  }
  if ((*(byte *)((int)puVar1 + 0x106) & 4) == 0) {
    *unaff_EDI = *unaff_EDI * 0.5;
    unaff_EDI[1] = unaff_EDI[1] * 0.5;
    unaff_EDI[2] = unaff_EDI[2] * 0.5;
    if ((*(byte *)((int)puVar1 + 0x106) & 4) == 0) goto LAB_0055a01d;
  }
  FUN_0055ad70();
LAB_0055a01d:
  puVar1[0x1a] = (uint)((float)puVar1[0x1a] + *unaff_EDI);
  puVar1[0x1b] = (uint)((float)puVar1[0x1b] + unaff_EDI[1]);
  puVar1[0x1c] = (uint)((float)puVar1[0x1c] + unaff_EDI[2]);
  puVar1[4] = puVar1[4] & 0xffffffdf;
  puVar1[0x133] = puVar1[0x133] | 3;
  if (((*(byte *)((int)puVar1 + 0x106) & 4) != 0) || ((*(byte *)(iVar2 + 0x2f4) & 0x44) != 0)) {
    vector3d_cross_product(PTR_DAT_00696720);
    vector3d_normalize_with_length();
    random_seed_global = random_seed_global * 0x19660d + 0x3c6ef35f;
    fVar3 = (float)(random_seed_global >> 0x10) * 1.5259022e-05 *
            SQRT(*unaff_EDI * *unaff_EDI + unaff_EDI[1] * unaff_EDI[1] + unaff_EDI[2] * unaff_EDI[2]
                ) * 1.5707964;
    puVar1[0x23] = (uint)(local_c * fVar3 + (float)puVar1[0x23]);
    puVar1[0x24] = (uint)(local_8 * fVar3 + (float)puVar1[0x24]);
    puVar1[0x25] = (uint)(local_4 * fVar3 + (float)puVar1[0x25]);
  }
  if (puVar1[0x47] == 0xffffffff) {
    fVar3 = unaff_EDI[1];
    fVar4 = *unaff_EDI;
    fVar5 = unaff_EDI[2];
    fVar6 = (float10)vector3d_normalize_with_length();
    if ((float10)0.0 < fVar6) {
      puVar1[0x1d] = (uint)fVar4;
      puVar1[0x1e] = (uint)fVar3;
      puVar1[0x1f] = (uint)fVar5;
      FUN_00560800();
    }
  }
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
