// light_new_attached
// address 0x4f0af0, size 217 bytes
// name confidence: 0.75 (out/phase4/objects_types_notes.md names this function directly: "Stride
// proved by (index & 0xffff) * 0x7c in light_new_attached @0x4f0af0")
// rewrite confidence: 0.55
// evidence: types/objects.h light (owner_object 0x2c, marker_index 0x5c, marker_index_secondary
// 0x5e, change_color_index 0x60, definition_tag 0x04, flags 0x02 with light_flags, next_light
// 0x10, marker_link 0x58, creation_tick 0x0c); types/tags.h Light.flags bit 0 ==
// _light_always_visible_bit.
// UNSURE: the Light tag field at +0xb8 (tested for validity alongside the always-visible flag)
// has no established name; FUN_004f2a00 (the transform recompute this calls) is out of this
// module's address range.
// register convention: datum_index light_tag in EAX (param_1); datum_index owner_object on the
// stack (param_2); int16_t marker_index, marker_index_secondary, change_color_index on the stack
// (param_3/4/5).
// blam-cc: EAX=light_tag, stack=(owner_object, marker_index, marker_index_secondary,
//   change_color_index)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern data_array *light_data;      // 0x00860b14
extern tag_instance *tag_instances; // 0x0087bc14
extern int32_t light_frame_counter; // 0x008607c4

extern datum_index datum_new(data_array *array); // UNSURE: returns {handle, data_array*} as a
    // 64-bit pair in the original; only the handle is modeled here, using light_data directly
    // for the element base rather than the returned pointer. memory module, 0x4d0480
extern void object_light_recompute_transform(uint32_t light_index); // this module, 0x4f2a00 (out of range)

datum_index light_new_attached(datum_index light_tag, datum_index owner_object, int16_t marker_index,
    int16_t marker_index_secondary, int16_t change_color_index)
{
    Light *tag = (Light *)tag_instances[light_tag & 0xffff].data;
    datum_index handle = (datum_index)0xffffffff;

    if ((tag->flags & 1) != 0 || *(int32_t *)&((struct Light *)tag)->lens_flare.tag_id != -1) { // UNSURE: +0xb8
        handle = datum_new(light_data);

        if (handle != (datum_index)0xffffffff) {
            light *entry = &((light *)light_data->data)[handle & 0xffff];
            uint8_t always_visible = (uint8_t)(tag->flags & 1);

            entry->owner_object = owner_object;
            entry->marker_index = marker_index;
            entry->flags = 0;
            entry->marker_index_secondary = marker_index_secondary;
            entry->definition_tag = light_tag;
            // light+0x60 is the attached form's change_color_index; the positioned form uses the
            // same offset as local_position (see the light struct in types/objects.h).
            *(int16_t *)&((struct light *)entry)->local_position.x = change_color_index;

            if (always_visible == 0 && *(int32_t *)&((struct Light *)tag)->lens_flare.tag_id == -1) {
                entry->flags = 0;
            } else {
                entry->flags = always_visible | _light_attached_bit;
            }

            entry->next_light = (datum_index)0xffffffff;
            entry->marker_link = -1;
            object_light_recompute_transform(handle);
            entry->creation_tick = light_frame_counter - 1;
        }
    }

    return handle;
}

#if 0
Original Ghidra decompilation (0x4f0af0):

uint FUN_004f0af0(uint param_1,undefined4 param_2,undefined2 param_3,undefined2 param_4,
                 undefined2 param_5)

{
  byte *pbVar1;
  byte bVar2;
  ushort uVar3;
  uint uVar4;
  int iVar5;
  undefined8 uVar6;

  pbVar1 = *(byte **)((param_1 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
  uVar4 = 0xffffffff;
  if (((*pbVar1 & 1) != 0) || (*(int *)(pbVar1 + 0xb8) != -1)) {
    uVar6 = datum_new();
    uVar4 = (uint)uVar6;
    if (uVar4 != 0xffffffff) {
      iVar5 = (uVar4 & 0xffff) * 0x7c + *(int *)((int)((ulonglong)uVar6 >> 0x20) + 0x34);
      *(undefined4 *)(iVar5 + 0x2c) = param_2;
      *(undefined2 *)(iVar5 + 0x5c) = param_3;
      *(undefined2 *)(iVar5 + 2) = 0;
      *(undefined2 *)(iVar5 + 0x5e) = param_4;
      *(uint *)(iVar5 + 4) = param_1;
      *(undefined2 *)(iVar5 + 0x60) = param_5;
      bVar2 = *pbVar1 & 1;
      *(ushort *)(iVar5 + 2) = (ushort)bVar2;
      if ((bVar2 == 0) && (*(int *)(pbVar1 + 0xb8) == -1)) {
        uVar3 = 0;
      }
      else {
        uVar3 = bVar2 | 2;
      }
      *(ushort *)(iVar5 + 2) = uVar3;
      *(undefined4 *)(iVar5 + 0x10) = 0xffffffff;
      *(undefined4 *)(iVar5 + 0x58) = 0xffffffff;
      FUN_004f2a00(uVar4);
      *(int *)(iVar5 + 0xc) = DAT_008607c4 + -1;
    }
  }
  return uVar4;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
