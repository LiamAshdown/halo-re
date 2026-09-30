// render_object  (Ghidra: FUN_0050eba0; renamed per out/phase4/render_types_notes.md's
// misattributed-functions table: "render_objects, render_object_shadows, render_object,
// render_object_list, render_object_shadow_begin, render_object_shadow_end")
// address 0x50eba0, size 634 bytes
// name confidence: 0.5   rewrite confidence: 0.8
// evidence: types/render.h object_render_data's own doc names every callee here by address
//   (object_get_cached_render_lighting 0x50ea00, object_compute_level_of_detail_pixels 0x50f740,
//   render_object_shadow_begin 0x50f830, render_object_shadow_end/
//   shadow_compute_bounding_box_and_register 0x50f980) and every field this function writes
//   (+0x00 object_index by the caller, +0x04 lighting, +0x08 shadow_pass selects this branch,
//   +0x09 outside_fog_plane from the fog plane test). Object tag fields resolved from
//   types/tags.h Object (object_type int16 @0, flags uint16 @2, bounding_radius float @4) and
//   types/objects.h object (flags @0x10, first_widget @0x16c, first_child_object @0x118,
//   bounding_center @0xa0); the fog test reuses render_fog's aliased planar_mode/plane fields
//   (see src/render/render_player_frame.c's note on that aliasing).
// register convention: EDI = data (object_render_data*).
//   // blam-cc: EDI=data
// UNSURE: FUN_0050f830's second (stack) argument is passed here as the computed shadow intensity
//   (fade1 * fade2), not the "level of detail value" out/phase4/render_types_notes.md's register-
//   conventions section describes for that address in general; named for what this call site
//   actually passes.
// CORRECTION: render_object_list (0x50ee20) takes three arguments (data, parent_effect,
//   object_index), not one -- confirmed from its own disassembly after Ghidra's decompile of this
//   function (0x50eba0) showed every call to it with no visible arguments at all. Each top-level
//   call site builds its own parent_effect (0 for the shadow pass, a locally zeroed
//   render_model_effect of type none otherwise); see render_object_list.c for the callee side.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "cache.h"
#include "rasterizer.h"
#include "render.h"
#include "structures.h"
#include "fn_render.h"
#include <stdint.h> // uintptr_t

extern data_array *object_data;   // 0x008603b0
extern tag_instance *tag_instances;  // 0x0087bc14
extern render_fog render_fog_state;  // 0x007c32f4, this module (named render_fog_state; a
                                     // variable cannot share the render_fog typedef's own name)

extern uint8_t render_object_is_camera_unit(datum_index object); // 0x50ea50, this module

    // module (below this batch); blam-cc: EAX=object_index
extern render_lighting *object_get_cached_render_lighting(datum_index object_index,
                                                          real level_of_detail_pixels); // 0x50ea00,
    // this module; blam-cc: ESI=object_index, stack=level_of_detail_pixels
extern int8_t widget_list_has_flag(datum_index first_widget); // 0x4ffc60, objects module;
                                                               // blam-cc: ECX=first_widget
extern uint8_t render_object_shadow_begin(object_render_data *data, real shadow_intensity); // 0x50f830,
    // this module (below this batch); blam-cc: EAX=data, stack=shadow_intensity
extern void render_object_list(object_render_data *data, render_model_effect *parent_effect,
                               datum_index object_index); // 0x50ee20, this module; blam-cc:
                                                          // stack=(data, parent_effect, object_index)
extern void render_object_shadow_end(object_render_data *data); // 0x50f980, this module (below
    // this batch); blam-cc: ECX=data

// Applies either the fake object "blob" shadow effect (data->shadow_pass != 0) or ordinary cached
// ambient lighting plus a fog-plane test (data->shadow_pass == 0) to one object, then always hands
// it to render_object_list.
void render_object(object_render_data *data) // blam-cc: EDI=data
{
    object_header *header = &((object_header *)object_data->data)[(uint16_t)data->object_index];
    object *obj = header->data;

    if (data->shadow_pass != 0) {
        real level_of_detail_pixels;
        real luminance_deficit;

        if (render_object_is_camera_unit(data->object_index)) {
            return;
        }
        if ((obj->flags & _object_definition_flag0_bit) != 0) {
            return;
        }
        if ((obj->flags & _object_no_collision_bit) != 0 && obj->first_child_object == k_datum_index_none) {
            return;
        }

        level_of_detail_pixels = object_compute_level_of_detail_pixels(data->object_index);
        {
            render_lighting *lighting = object_get_cached_render_lighting(data->object_index, level_of_detail_pixels);
            data->lighting = (uint32_t)(uintptr_t)lighting;
            level_of_detail_pixels = object_compute_level_of_detail_pixels(data->object_index);
            luminance_deficit = 1.0f - (lighting->shadow_color.red * 0.299f + lighting->shadow_color.green * 0.587f +
                                        lighting->shadow_color.blue * 0.114f);
        }

        if (level_of_detail_pixels <= 30.0f) {
            return;
        }
        if (luminance_deficit <= 0.19f) {
            return;
        }

        {
            float distance_fade = (level_of_detail_pixels - 30.0f) * 0.06666667f;
            float darkness_fade = (luminance_deficit - 0.19f) * 9.090908f;

            if (distance_fade < 0.0f) {
                distance_fade = 0.0f;
            } else if (distance_fade > 1.0f) {
                distance_fade = 1.0f;
            }
            if (darkness_fade < 0.0f) {
                darkness_fade = 0.0f;
            } else if (darkness_fade > 1.0f) {
                darkness_fade = 1.0f;
            }

            if (!render_object_shadow_begin(data, darkness_fade * distance_fade)) {
                return;
            }
        }

        // Disassembly (objdump -d -M intel, 0x50ed0c..0x50ed12) shows this top-level call passes
        // a literal 0 (no parent effect) for the shadow pass, not the object's own data pointer.
        render_object_list(data, 0, data->object_index);
        render_object_shadow_end(data);
        return;
    }

    {
        uint8_t sample_full_lighting;
        Object *definition;

        if ((obj->flags & _object_no_collision_bit) != 0 && obj->first_child_object == k_datum_index_none) {
            datum_index first_widget = obj->first_widget;
            if (!widget_list_has_flag(first_widget)) {
                if (first_widget == k_datum_index_none) {
                    return;
                }
                sample_full_lighting = 0;
                goto sampled;
            }
        }
        sample_full_lighting = 1;

    sampled:
        definition = (Object *)tag_instances[(uint16_t)obj->definition_tag].data;

        if (sample_full_lighting) {
            real level_of_detail_pixels = object_compute_level_of_detail_pixels(data->object_index);
            render_lighting *lighting = object_get_cached_render_lighting(data->object_index, level_of_detail_pixels);
            data->lighting = (uint32_t)(uintptr_t)lighting;
        } else {
            data->lighting = 0;
        }

        if (render_fog_state.planar_mode != _structure_fog_plane_bounded ||
            definition->bounding_radius <
                (render_fog_state.plane.normal.i * obj->bounding_center.x +
                 render_fog_state.plane.normal.k * obj->bounding_center.z +
                 render_fog_state.plane.normal.j * obj->bounding_center.y) -
                    render_fog_state.plane.d) {
            data->outside_fog_plane = 1;
        } else {
            data->outside_fog_plane = 0;
        }

        // Disassembly (objdump -d -M intel, 0x50edff..0x50ee0b) shows this top-level call builds
        // a local render_model_effect with type/modifier_shader/change_colors/function_values all
        // zeroed (the other fields are left at whatever the stack already held, but they are only
        // ever read by render_object_list when type is not "none", so zero-initializing the whole
        // struct is exactly equivalent and avoids reproducing genuinely uninitialized memory).
        render_model_effect top_level_effect = {0};
        top_level_effect.type = _render_model_effect_none;
        render_object_list(data, &top_level_effect, data->object_index);
    }
}

#if 0
Original Ghidra decompilation (0x50eba0):

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void FUN_0050eba0(void)

{
  int iVar1;
  uint *puVar2;
  float fVar3;
  bool bVar4;
  char cVar5;
  undefined1 uVar6;
  uint uVar7;
  uint *unaff_EDI;
  float10 fVar8;
  float local_34;

  if ((char)unaff_EDI[2] != '\0') {
    iVar1 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (*unaff_EDI & 0xffff) * 0xc);
    cVar5 = FUN_0050ea50();
    if (cVar5 != '\0') {
      return;
    }
    uVar7 = *(uint *)(iVar1 + 0x10);
    if ((uVar7 & 0x40000) != 0) {
      return;
    }
    if (((uVar7 & 1) != 0) && (*(int *)(iVar1 + 0x118) == -1)) {
      return;
    }
    fVar8 = (float10)FUN_0050f740();
    uVar7 = render_get_cluster_ambient_light_sample((float)fVar8);
    unaff_EDI[1] = uVar7;
    fVar8 = (float10)FUN_0050f740();
    fVar3 = 1.0 - (*(float *)(uVar7 + 0x68) * 0.299 +
                  *(float *)(uVar7 + 0x6c) * 0.587 + *(float *)(uVar7 + 0x70) * 0.114);
    if ((float)fVar8 <= 30.0) {
      return;
    }
    if (fVar3 <= 0.19) {
      return;
    }
    local_34 = ((float)fVar8 - 30.0) * 0.06666667;
    fVar3 = (fVar3 - 0.19) * 9.090908;
    if (0.0 <= local_34) {
      if (1.0 < local_34) {
        local_34 = 1.0;
      }
    }
    else {
      local_34 = 0.0;
    }
    if (0.0 <= fVar3) {
      if (1.0 < fVar3) {
        fVar3 = 1.0;
      }
    }
    else {
      fVar3 = 0.0;
    }
    cVar5 = FUN_0050f830(fVar3 * local_34);
    if (cVar5 == '\0') {
      return;
    }
    FUN_0050ee20();
    shadow_compute_bounding_box_and_register();
    return;
  }
  puVar2 = *(uint **)(*(int *)(DAT_008603b0 + 0x34) + 8 + (*unaff_EDI & 0xffff) * 0xc);
  if (((puVar2[4] & 1) != 0) && (puVar2[0x46] == 0xffffffff)) {
    uVar7 = puVar2[0x5b];
    cVar5 = FUN_004ffc60();
    if (cVar5 == '\0') {
      bVar4 = false;
      if (uVar7 == 0xffffffff) {
        return;
      }
      goto LAB_0050ed66;
    }
  }
  bVar4 = true;
LAB_0050ed66:
  iVar1 = *(int *)((*puVar2 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
  if (bVar4) {
    fVar8 = (float10)FUN_0050f740();
    uVar7 = render_get_cluster_ambient_light_sample((float)fVar8);
    unaff_EDI[1] = uVar7;
  }
  else {
    unaff_EDI[1] = 0;
  }
  if ((DAT_007c3310 != 1) ||
     (*(float *)(iVar1 + 4) <
      (_DAT_007c3314 * (float)puVar2[0x28] +
      _DAT_007c331c * (float)puVar2[0x2a] + _DAT_007c3318 * (float)puVar2[0x29]) - _DAT_007c3320)) {
    uVar6 = 1;
  }
  else {
    uVar6 = 0;
  }
  *(undefined1 *)((int)unaff_EDI + 9) = uVar6;
  FUN_0050ee20();
  return;
}
#endif
