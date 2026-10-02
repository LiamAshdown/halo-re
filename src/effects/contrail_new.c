// contrail_new  (Ghidra: FUN_0044c910; named per out/phase2/results/effects_00.json "contrail_new",
// confidence 0.4)
// address 0x44c910, size 318 bytes
// name confidence: 0.4   rewrite confidence: 0.55
// evidence: types/effects.h contrail (every field written here matches its documented
// establishing function list), types/tags.h Object.attachments (TagReflexive at 0x140, verified
// by offsetof against types/tags.h) and ObjectAttachment.primary_scale (0x30, FunctionOut_t);
// types/objects.h object_header / object.definition_tag / object.function_out_values (0x134) /
// function_valid_flags (0x123); src/devices/device_can_change_position.c establishes the
// `((object_header *)object_data->data)[index & 0xffff].data` object lookup idiom this function
// also uses (twice: once for the owner object's tag, once -- after contrail_next_sequence -- to
// refetch the same owner object and read its function tables).
// register convention: attachment_index in AX (in_AX, low 16 bits of EAX), owner object_index in
// ECX (in_ECX), the Contrail tag definition_index on the stack (Ghidra's param_1).
//   // blam-cc: EAX(low16) -> attachment_index, ECX -> object_index, stack -> definition_index

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "cache.h"
#include "effects.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern data_array *contrail_data;   // 0x0087abec
extern data_array *object_data;     // 0x008603b0
extern tag_instance *tag_instances; // 0x0087bc14

extern datum_index datum_new(data_array *array); // 0x4d0480, memory module; blam-cc: array in EDX
extern void contrail_next_sequence(contrail *self); // 0x44ced0, this module; blam-cc: EAX -> self
extern void contrail_generate_points(datum_index contrail_handle, int16_t point_count,
    uint8_t force); // 0x44d020, this module;
    // blam-cc: EAX -> contrail_handle, stack -> (point_count, force)

// Creates a new contrail attached to `object_index` at attachment `attachment_index`, and
// immediately generates its first point.
datum_index contrail_new(int16_t attachment_index, datum_index object_index,
    datum_index definition_index)
{
    datum_index handle = k_datum_index_none;

    if (definition_index != k_datum_index_none) {
        handle = datum_new(contrail_data);
        if (handle != k_datum_index_none) {
            contrail *self = &((contrail *)contrail_data->data)[(uint16_t)handle];
            object *owner = ((object_header *)object_data->data)[(uint16_t)object_index].data;
            Object *owner_tag = (Object *)tag_instances[(uint16_t)owner->definition_tag].data;
            ObjectAttachment *attachment = (ObjectAttachment *)owner_tag->attachments.pointer +
                attachment_index;

            self->flags = 0;
            self->unknown_03 = 0;
            self->definition_index = definition_index;
            self->object_index = object_index;
            self->attachment_index = attachment_index;
            self->scale_function_index = attachment->primary_scale - 1;
            self->sequence_index = (int16_t)0xffff;
            contrail_next_sequence(self);

            self->texture_offset_u = 0.0f;
            self->texture_offset_v = 0.0f;
            {
                int i;
                for (i = 0; i < 4; i++) {
                    self->point_count[i] = 0;
                    self->first_point[i] = k_datum_index_none;
                }
            }

            {
                int16_t scale_function_index = self->scale_function_index;
                object *root = ((object_header *)object_data->data)[(uint16_t)self->object_index].data;

                if (scale_function_index == -1) {
                    self->scale = 1.0f;
                } else {
                    self->scale = root->function_out_values[scale_function_index];
                    if ((root->function_valid_flags & (1u << (scale_function_index & 0x1f))) == 0) {
                        return handle;
                    }
                }
            }

            self->flags |= _contrail_emitting_bit;
            contrail_generate_points(handle, 1, 1);
        }
    }

    return handle;
}

#if 0
Original Ghidra decompilation (0x44c910):

uint FUN_0044c910(int param_1)

{
  short sVar1;
  uint *puVar2;
  short in_AX;
  uint uVar3;
  int iVar4;
  int iVar5;
  uint in_ECX;
  undefined2 *puVar6;
  undefined4 *puVar7;
  undefined8 uVar8;
  
  uVar3 = 0xffffffff;
  if (param_1 != -1) {
    uVar8 = datum_new();
    iVar5 = DAT_0087bc14;
    uVar3 = (uint)uVar8;
    if (uVar3 != 0xffffffff) {
      iVar4 = (uVar3 & 0xffff) * 0x44 + *(int *)((int)((ulonglong)uVar8 >> 0x20) + 0x34);
      puVar2 = *(uint **)(*(int *)(DAT_008603b0 + 0x34) + 8 + (in_ECX & 0xffff) * 0xc);
      *(undefined2 *)(iVar4 + 2) = 0;
      *(int *)(iVar4 + 4) = param_1;
      *(uint *)(iVar4 + 8) = in_ECX;
      *(short *)(iVar4 + 0xc) = in_AX;
      *(short *)(iVar4 + 0xe) =
           *(short *)(*(int *)(*(int *)((*puVar2 & 0xffff) * 0x20 + 0x14 + iVar5) + 0x144) + 0x30 +
                     in_AX * 0x48) + -1;
      *(undefined2 *)(iVar4 + 0x14) = 0xffff;
      iVar5 = FUN_0044ced0();
      *(undefined4 *)(iVar5 + 0x18) = 0;
      *(undefined4 *)(iVar5 + 0x1c) = 0;
      puVar7 = (undefined4 *)(iVar5 + 0x34);
      puVar6 = (undefined2 *)(iVar5 + 0x2c);
      iVar4 = 4;
      do {
        *puVar6 = 0;
        *puVar7 = 0xffffffff;
        puVar6 = puVar6 + 1;
        puVar7 = puVar7 + 1;
        iVar4 = iVar4 + -1;
      } while (iVar4 != 0);
      sVar1 = *(short *)(iVar5 + 0xe);
      iVar4 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (*(uint *)(iVar5 + 8) & 0xffff) * 0xc);
      if (sVar1 == -1) {
        *(undefined4 *)(iVar5 + 0x10) = 0x3f800000;
      }
      else {
        *(undefined4 *)(iVar5 + 0x10) = *(undefined4 *)(iVar4 + 0x134 + sVar1 * 4);
        if ((*(byte *)(iVar4 + 0x123) & (byte)(1 << ((byte)sVar1 & 0x1f))) == 0) {
          return uVar3;
        }
      }
      *(byte *)(iVar5 + 2) = *(byte *)(iVar5 + 2) | 1;
      contrail_generate_points(1,1);
      return uVar3;
    }
  }
  return uVar3;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
