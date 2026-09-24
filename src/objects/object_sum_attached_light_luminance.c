// object_sum_attached_light_luminance
// address 0x4f1b30, size 226 bytes
// name confidence: 0.45 (still FUN_004f1b30 in Ghidra; named from out/phase4/objects_functions.md's
// summary: "Recursively computes an object's total perceptual-luminance across its own visible
// regions plus every attached child/sibling object")
// rewrite confidence: 0.45
// evidence: types/objects.h object.attachment_types (0x144), object.attachment_handles (0x14c),
// object.first_child_object (0x118), object.next_object (0x114); types/tags.h Object.attachments
// (TagReflexive count at 0x140). The per-light accumulated colour at light+0x14/0x18/0x1c is the
// same unnamed ColorRGB block object_lights_update_all.c writes into.
// register convention: uint32_t object_index, passed in a float-typed register per Ghidra's own
// signature (a common decompiler ambiguity for values that arrive in a register also used for
// floats); treated here as the plain integer index it clearly is from its use.
// blam-cc: EAX=object_index (typed float by Ghidra, used as uint)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"

extern data_array *object_data; // 0x008603b0
extern tag_instance *tag_instances; // 0x0087bc14
extern data_array *light_data;  // 0x00860b14

real object_sum_attached_light_luminance(uint32_t object_index)
{
    object_header *headers = (object_header *)object_data->data;
    object *obj = headers[object_index & 0xffff].data;
    Object *definition = (Object *)tag_instances[obj->definition_tag & 0xffff].data;
    int32_t attachment_count = definition->attachments.count;
    real total = 0.0f;
    int32_t i;

    for (i = 0; i < attachment_count; i++) {
        if (obj->attachment_types[i] == _object_attachment_type_light &&
            obj->attachment_handles[i] != (datum_index)0xffffffff) {
            light *l = &((light *)light_data->data)[obj->attachment_handles[i] & 0xffff];

            total = *(float *)((uint8_t *)l + 0x14) * 0.299f +
                    *(float *)((uint8_t *)l + 0x18) * 0.587f +
                    *(float *)((uint8_t *)l + 0x1c) * 0.114f + total;
        }
    }

    if (obj->first_child_object != (datum_index)0xffffffff) {
        total = object_sum_attached_light_luminance(obj->first_child_object) + total;
    }
    if (obj->next_object != (datum_index)0xffffffff) {
        total = object_sum_attached_light_luminance(obj->next_object) + total;
    }

    return total;
}

#if 0
Original Ghidra decompilation (0x4f1b30):

float10 FUN_004f1b30(float param_1)

{
  uint *puVar1;
  int iVar2;
  int iVar3;
  int iVar4;
  short sVar5;
  float10 fVar6;

  fVar6 = (float10)0.0;
  puVar1 = *(uint **)(*(int *)(DAT_008603b0 + 0x34) + 8 + ((uint)param_1 & 0xffff) * 0xc);
  iVar2 = *(int *)(*(int *)((*puVar1 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14) + 0x140);
  sVar5 = 0;
  if (0 < iVar2) {
    iVar4 = 0;
    do {
      if ((*(char *)(iVar4 + 0x144 + (int)puVar1) == '\0') && (puVar1[iVar4 + 0x53] != 0xffffffff))
      {
        iVar3 = *(int *)(DAT_00860b14 + 0x34);
        iVar4 = (puVar1[iVar4 + 0x53] & 0xffff) * 0x7c;
        fVar6 = (float10)*(float *)(iVar4 + 0x14 + iVar3) * (float10)0.299 +
                (float10)*(float *)(iVar4 + 0x18 + iVar3) * (float10)0.587 +
                (float10)*(float *)(iVar4 + 0x1c + iVar3) * (float10)0.114 + fVar6;
      }
      sVar5 = sVar5 + 1;
      iVar4 = (int)sVar5;
    } while (iVar4 < iVar2);
  }
  param_1 = (float)fVar6;
  if (puVar1[0x46] != 0xffffffff) {
    fVar6 = (float10)FUN_004f1b30(puVar1[0x46]);
    fVar6 = fVar6 + (float10)param_1;
    param_1 = (float)fVar6;
  }
  if (puVar1[0x45] != 0xffffffff) {
    fVar6 = (float10)FUN_004f1b30(puVar1[0x45]);
    fVar6 = fVar6 + (float10)param_1;
  }
  return fVar6;
}
#endif
