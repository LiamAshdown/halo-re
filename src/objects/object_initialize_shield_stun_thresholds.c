// object_initialize_shield_stun_thresholds
// address 0x4ed440, size 198 bytes
// name confidence: 0.7 (out/phase4/objects_types_notes.md names and cites this function as the
// proof for object.maximum_body_vitality/maximum_shield_vitality/body_vitality/shield_vitality)
// rewrite confidence: 0.8
// evidence: types/objects.h object (maximum_body_vitality 0xd8, maximum_shield_vitality 0xdc,
// body_vitality 0xe0, shield_vitality 0xe4); types/tags.h Object.collision_model (0x7c),
// ModelCollisionGeometry.maximum_body_vitality (0x08) / .maximum_shield_vitality (0xcc);
// types/cache.h tag_instance.data (0x14).
// register convention: uint32_t object_index in EAX (in_EAX); float *override_max_body_vitality
// in ESI (unaff_ESI, optional); float *override_max_shield_vitality in EDI (unaff_EDI,
// optional). ECX/EDX/EBX are unused by this function.
// blam-cc: EAX=object_index, ESI=override_max_body_vitality, EDI=override_max_shield_vitality

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"
#include "fn_objects.h"

extern data_array *object_data;     // 0x008603b0
extern tag_instance *tag_instances; // 0x0087bc14

void object_initialize_shield_stun_thresholds(uint32_t object_index, float *override_max_body_vitality,
                                               float *override_max_shield_vitality)
{
    object_header *headers = (object_header *)object_data->data;
    object *obj = headers[object_index & 0xffff].data;
    float max_body_vitality = 0.0f;
    float max_shield_vitality = 0.0f;
    Object *definition = (Object *)tag_instances[obj->definition_tag & 0xffff].data;
    TagID collision_model = definition->collision_model.tag_id;

    if (collision_model.index != 0xffff) {
        ModelCollisionGeometry *geometry = (ModelCollisionGeometry *)tag_instances[collision_model.index].data;
        if (geometry != 0) {
            max_body_vitality = geometry->maximum_body_vitality;
            max_shield_vitality = geometry->maximum_shield_vitality;
        }
    }

    if (override_max_body_vitality != 0) {
        max_body_vitality = *override_max_body_vitality;
    }
    if (override_max_shield_vitality != 0) {
        max_shield_vitality = *override_max_shield_vitality;
    }

    obj->maximum_shield_vitality = max_shield_vitality;
    obj->maximum_body_vitality = max_body_vitality;
    obj->body_vitality = (max_body_vitality > 0.0f) ? 1.0f : 0.0f;
    obj->shield_vitality = (max_shield_vitality > 0.0f) ? 1.0f : 0.0f;
}

#if 0
Original Ghidra decompilation (0x4ed440):

void FUN_004ed440(void)

{
  float fVar1;
  uint *puVar2;
  uint uVar3;
  int iVar4;
  uint in_EAX;
  float *unaff_ESI;
  float *unaff_EDI;
  float local_4;

  fVar1 = 0.0;
  puVar2 = *(uint **)(*(int *)(DAT_008603b0 + 0x34) + 8 + (in_EAX & 0xffff) * 0xc);
  uVar3 = *(uint *)(*(int *)((*puVar2 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14) + 0x7c);
  local_4 = 0.0;
  if ((uVar3 != 0xffffffff) &&
     (iVar4 = *(int *)((uVar3 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14), iVar4 != 0)) {
    local_4 = *(float *)(iVar4 + 8);
    fVar1 = *(float *)(iVar4 + 0xcc);
  }
  if (unaff_ESI != (float *)0x0) {
    local_4 = *unaff_ESI;
  }
  if (unaff_EDI != (float *)0x0) {
    fVar1 = *unaff_EDI;
  }
  puVar2[0x37] = (uint)fVar1;
  puVar2[0x36] = (uint)local_4;
  if (local_4 <= 0.0) {
    uVar3 = 0;
  }
  else {
    uVar3 = 0x3f800000;
  }
  puVar2[0x38] = uVar3;
  if (0.0 < fVar1) {
    puVar2[0x39] = 0x3f800000;
    return;
  }
  puVar2[0x39] = 0;
  return;
}
#endif
