// light_volume_render
// address 0x4fe900, size 326 bytes, zero recorded callers (the widget_type_definition render
//   column for light volume (row 3) calls this indirectly)
// name confidence: 0.7 (out/phase4/objects_types_notes.md misattribution table: "0x4fe900 |
//   light_volume_new | light_volume_render (widget type 3 render column)")
// rewrite confidence: 0.8 (queue call fixed from objdump 0x4fea2c: it passes the 0x4fea80 procedure and the marker
//   position; the draft passed only the two ids)
// evidence: types/tags.h LightVolume (count 0x6e, frames TagReflexive 0x120, far_fade_distance
//   0x38, brightness_scale_source 0x44); global 0x006b8d70 light_volume_instances (table shape
//   matches the same data_array validation idiom as glow_render_dispatch.c, glow_render.c);
//   tag_instances 0x0087bc14; object_get_node_local_transform 0x4f6080 (established, all-stack).
// register convention: Ghidra shows a clean (param_1, param_2, param_3, param_4); param_3 is
//   never read anywhere in the body. Follows the same handle-validation shape as
//   glow_render_dispatch.c's param_2.
// blam-cc: stack -> object_index, light_volume_handle, unused, function_context
// UNSURE: `function_context` (param_4)'s layout is not identified; the brightness-scale gate it
//   feeds (`*(int *)(function_context + 4) + selector*4 - 4`) is kept as raw offsets.
// UNSURE: the fallback path when the handle validation fails (instance pointer left as the
//   literal integer 0) then unconditionally reads through it at +4; preserved literally, as in
//   glow_render_dispatch.c's identical pattern.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"

extern uint8_t *light_volume_instances; // 0x006b8d70, UNSURE: raw table, no struct defined
extern tag_instance *tag_instances;     // 0x0087bc14
extern float render_camera_global; // 0x007c3114
extern float camera_position_y; // 0x007c3118
extern float camera_position_z; // 0x007c311c (UNSURE: foreign module)
extern float camera_forward_x; // 0x007c3120
extern float camera_forward_y; // 0x007c3124
extern float camera_forward_z; // 0x007c3128 (UNSURE: foreign module)

extern int32_t object_get_node_local_transform(uint32_t object_index, char *marker_name,
                                                object_marker *marker, uint32_t flags); // 0x4f6080
extern void rasterizer_lens_flare_occlusion_sample_add(void *procedure, const real_point3d *position, uint32_t id_1,
    uint32_t id_2); // 0x536ff0, EAX procedure, EDX position, stack ids
extern void light_volume_render_procedure(uint32_t object_index, datum_index light_volume_handle); // 0x4fea80

void light_volume_render(uint32_t object_index, datum_index light_volume_handle, uint32_t unused,
                          uint8_t *function_context)
    // blam-cc: stack -> object_index, light_volume_handle, unused, function_context
{
    uint8_t *instance;

    if (object_index == 0xffffffff || light_volume_handle == (datum_index)0xffffffff) {
        return;
    }

    {
        int16_t index = (int16_t)light_volume_handle;
        instance = 0;

        // UNSURE: light_volume_instances's own data_array header layout (size 0x22,
        // last_index 0x2e, data 0x34) is assumed identical to glow_data / flag_data's.
        if (index >= 0 && index < *(int16_t *)(light_volume_instances + 0x2e)) {
            int32_t off = *(int16_t *)(light_volume_instances + 0x22) * index;
            int16_t identifier = *(int16_t *)(off + *(int32_t *)(light_volume_instances + 0x34));
            int16_t salt = (int16_t)(light_volume_handle >> 16);

            off = off + *(int32_t *)(light_volume_instances + 0x34);
            if (identifier != 0 && (salt == 0 || salt == identifier)) {
                instance = (uint8_t *)off;
            }
        }
    }

    {
        uint8_t *tag = (uint8_t *)tag_instances[*(uint32_t *)(instance + 4) & 0xffff].data;

        if (*(int16_t *)(tag + 0x6e) > 0 && *(int32_t *)(tag + 0x120) > 0 &&
            (*(int16_t *)(tag + 0x44) == 0 || function_context == 0 ||
             *(float *)(*(int32_t *)(function_context + 4) - 4 + *(int16_t *)(tag + 0x44) * 4) > 0.0f)) {
            object_marker marker;

            object_get_node_local_transform(object_index, (char *)tag, &marker, 1);

            if (*(float *)(tag + 0x38) == 0.0f ||
                camera_forward_y * (marker.node_transform.position.y - camera_position_y) +
                camera_forward_x * (marker.node_transform.position.x - render_camera_global) +
                camera_forward_z * (marker.node_transform.position.z - camera_position_z) <
                *(float *)(tag + 0x38)) {
                // 0x4fea2c: EAX = 0x4fea80, EDX = the marker position, stack (object, light volume)
                rasterizer_lens_flare_occlusion_sample_add((void *)light_volume_render_procedure,
                    &marker.node_transform.position, object_index, light_volume_handle);
            }
        }
    }
}

#if 0
Original Ghidra decompilation (0x4fe900):

void FUN_004fe900(int param_1,int param_2,undefined4 param_3,int param_4)

{
  int iVar1;
  short sVar2;
  short sVar3;
  undefined1 local_6c [96];
  float local_c;
  float local_8;
  float local_4;

  if (param_1 == -1) {
    return;
  }
  if (param_2 == -1) {
    return;
  }
  sVar3 = (short)param_2;
  if ((-1 < sVar3) && (sVar3 < *(short *)(DAT_006b8d70 + 0x2e))) {
    iVar1 = (int)*(short *)(DAT_006b8d70 + 0x22) * (int)sVar3;
    sVar3 = *(short *)(iVar1 + *(int *)(DAT_006b8d70 + 0x34));
    iVar1 = iVar1 + *(int *)(DAT_006b8d70 + 0x34);
    if ((sVar3 != 0) && ((sVar2 = (short)((uint)param_2 >> 0x10), sVar2 == 0 || (sVar2 == sVar3))))
    goto LAB_004fe95a;
  }
  iVar1 = 0;
LAB_004fe95a:
  iVar1 = *(int *)((*(uint *)(iVar1 + 4) & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
  if (((0 < *(short *)(iVar1 + 0x6e)) && (0 < *(int *)(iVar1 + 0x120))) &&
     ((*(short *)(iVar1 + 0x44) == 0 ||
      ((param_4 == 0 ||
       (0.0 < *(float *)(*(int *)(param_4 + 4) + -4 + *(short *)(iVar1 + 0x44) * 4))))))) {
    FUN_004f6080(param_1,iVar1,local_6c,1);
    if ((*(float *)(iVar1 + 0x38) == 0.0) ||
       (DAT_007c3124 * (local_8 - DAT_007c3118) +
        DAT_007c3120 * (local_c - DAT_007c3114) + DAT_007c3128 * (local_4 - DAT_007c311c) <
        *(float *)(iVar1 + 0x38))) {
      FUN_00536ff0(param_1,param_2);
    }
  }
  return;
}
#endif
