// render_object_list  (Ghidra: FUN_0050ee20; CEA render_object_list(data, parent_model_effect,
// object_index), hint only; renamed. Ghidra's decompile of this address is materially wrong --
// it drops the camera-unit/mirror gate and shows every callee with no arguments -- so this
// rewrite is built from the disassembly (objdump -d -M intel, 0x50ee20..0x50f141).)
// address 0x50ee20, size 802 bytes
// name confidence: 0.6   rewrite confidence: 0.75
// evidence: disassembly, with the push depth followed at both render_model calls:
//   - cdecl (data, parent_effect, object_index); the loop walks object.next_object (+0x114).
//   - render_object_is_camera_unit 0x50ea50 (ESI object) skips the object and its children
//     unless render_camera_global.mirrored (0x007c3138).
//   - outside the shadow pass the parent block is copied (10 dword rep movsd) and, for a parent
//     of type 2, reset (type 0, +0x1c / +0x20 / +0x24 cleared).
//   - objects without object flag 0x01 are drawn through render_model 0x4d6fc0 (EAX =
//     Object.model tag id, ECX = object + object.nodes.offset (+0x1f2), 11 stack arguments in
//     the CEA order (level_of_detail_pixels, region_permutations +0x180, change_colors +0x1b8,
//     function_out_values +0x134, lighting, bounding_center +0xa0, bounding_radius +0xac,
//     effect, object index, forced_shader_permutation +0x176, flags)). The first stack argument
//     is object_compute_level_of_detail_pixels 0x50f740 (the float stored at esp+0x10, re-read
//     as [esp+0x34] after nine pushes) in the normal pass and that value times 0.3 (0x672c94) in
//     the shadow pass; flags are 4 when outside_fog_plane, and 2 with a NULL effect in the
//     shadow pass.
//   - the effect: modifier shader = the Shader tag of Object.modifier_shader (+0x9c) when it is
//     of shader_type 1 or 5..11, which also sets change_colors / function_values, else NULL (an
//     unset modifier shader keeps the inherited value); type 1 (unit +0x37c > 0 on a biped or
//     vehicle, copying +0x37c / +0x380, the object and its centre) and type 2 (Object flags bit 1)
//     as types/render.h describes.
//   - render_debug_objects (0x007c30e8) calls object_type_definitions_notify_0x5c 0x4f4410
//     (EBX = object index).
//   - widgets: widget_list_notify 0x4ffca0 (EDI = object index, EBX = a render_animation
//     {change_colors, function_out_values} on the stack, stack = lighting), which forwards all
//     three to every widget type render callback.
//   - children: render_object_list(data, shadow pass ? NULL : &effect, first_child (+0x118)).
// review fix (phase-4 gate): the first draft passed effect.modifier_shader as the first
//   render_model stack argument (it is the level of detail, CEA render_model(model_index,
//   level_of_detail_pixels, node_matrices, ...)) and dropped the lighting and render_animation
//   arguments of widget_list_notify; both are corrected.
// register convention: stack = (data, parent_effect, object_index).
//   // blam-cc: stack=(data, parent_effect, object_index)
// UNSURE: unit +0x37c / +0x380 (unit_data.active_camo_amount / unknown_380, most likely the active
//   camouflage amount and its fade).

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "units.h"
#include "cache.h"
#include "rasterizer.h"
#include "render.h"
#include "fn_render.h"
#include <stdint.h> // uintptr_t

extern data_array *object_data;            // 0x008603b0, objects module
extern tag_instance *tag_instances;        // 0x0087bc14
extern render_camera render_camera_global; // 0x007c3114, this module
extern uint8_t render_debug_objects;       // 0x007c30e8, this module

extern uint8_t render_object_is_camera_unit(datum_index object); // 0x50ea50, this module; ESI

    // module; blam-cc: EAX=object_index
extern void object_type_definitions_notify_0x5c(uint32_t object_index); // 0x4f4410, objects
                                                                        // module; blam-cc: EBX=object_index
extern void widget_list_notify(uint32_t object_index, render_lighting *lighting,
                               render_animation *animation);
    // 0x4ffca0, objects module; blam-cc: EDI -> object_index, stack -> lighting,
    // EBX -> animation (src/objects models only the EDI argument)
extern void render_model(TagID model_tag_id, void *node_matrices, float level_of_detail_pixels,
                         uint8_t *region_permutations, ColorRGB *change_colors, float *function_out_values,
                         render_lighting *lighting, real_point3d *bounding_center, float bounding_radius,
                         render_model_effect *effect, datum_index object_index,
                         uint16_t forced_shader_permutation, uint32_t flags);
    // 0x4d6fc0, models; blam-cc: EAX -> model_tag_id, ECX -> node_matrices, stack -> the rest

// Recursively walks an object and its sibling/child chain, drawing each one's model (skipping the
// local player's own camera unit unless viewing through a mirror), building a render_model_effect
// per object (inherited from the parent, reset below a self-occlusion parent, and re-derived for
// active camouflage or self-occlusion on the object itself) and notifying its widget list.
void render_object_list(object_render_data *data, render_model_effect *parent_effect,
                        datum_index object_index) // blam-cc: stack=(data, parent_effect, object_index)
{
    while (object_index != k_datum_index_none) {
        object *obj = ((object_header *)object_data->data)[(uint16_t)object_index].data;
        render_model_effect effect;

        if (render_object_is_camera_unit(object_index) && !render_camera_global.mirrored) {
            goto next_sibling;
        }

        if (data->shadow_pass == 0) {
            effect = *parent_effect;
            if (parent_effect->type == _render_model_effect_self_occlusion) {
                effect.type = _render_model_effect_none;
                effect.modifier_shader = 0;
                effect.function_values = 0;
                effect.change_colors = 0;
            }
        }

        if ((obj->flags & _object_no_collision_bit) == 0) {
            Object *tag_data = (Object *)tag_instances[(uint16_t)obj->definition_tag].data;
            real lod = object_compute_level_of_detail_pixels(object_index);

            if (data->shadow_pass == 0) {
                if (*(uint32_t *)&tag_data->modifier_shader.tag_id != 0xffffffffu) {
                    Shader *shader_data =
                        (Shader *)tag_instances[tag_data->modifier_shader.tag_id.index].data;

                    effect.modifier_shader = (uint32_t)(uintptr_t)shader_data;
                    if (shader_data->shader_type == 1 ||
                        (shader_data->shader_type > 4 && shader_data->shader_type <= 0xb)) {
                        effect.change_colors = (uint32_t)(uintptr_t)obj->change_colors;
                        effect.function_values = (uint32_t)(uintptr_t)obj->function_out_values;
                    } else {
                        effect.modifier_shader = 0;
                    }
                }

                if (((1 << (obj->type & 0x1f)) & 3) != 0) {
                    object *unit_object =
                        ((object_header *)object_data->data)[(uint16_t)object_index].data;
                    unit_data *unit = (unit_data *)((uint8_t *)unit_object + k_unit_data_offset);

                    if (unit->active_camo_amount > 0.0f) {
                        effect.centroid = obj->bounding_center;
                        effect.type = _render_model_effect_active_camouflage;
                        effect.object_index = object_index;
                        effect.unit_37c = unit->active_camo_amount;
                        effect.unit_380 = unit->unknown_380;
                    }
                }
                if ((tag_data->flags & 0x02) != 0) { // ObjectFlags transparent_self_occlusion
                    effect.centroid = obj->bounding_center;
                    effect.type = _render_model_effect_self_occlusion;
                    effect.object_index = object_index;
                }

                render_model(tag_data->model.tag_id,
                             (uint8_t *)obj + obj->nodes.offset, lod,
                             obj->region_permutations, obj->change_colors, obj->function_out_values,
                             (render_lighting *)(uintptr_t)data->lighting, &obj->bounding_center,
                             obj->bounding_radius, &effect, object_index,
                             obj->forced_shader_permutation,
                             (data->outside_fog_plane != 0) ? 4u : 0u);

                if (render_debug_objects != 0) {
                    object_type_definitions_notify_0x5c(object_index);
                }
            } else {
                render_model(tag_data->model.tag_id,
                             (uint8_t *)obj + obj->nodes.offset, lod * 0.3f,
                             obj->region_permutations, obj->change_colors, obj->function_out_values,
                             (render_lighting *)(uintptr_t)data->lighting, &obj->bounding_center,
                             obj->bounding_radius, 0, object_index,
                             obj->forced_shader_permutation, 2);
            }
        }

        if (data->shadow_pass == 0 && obj->first_widget != k_datum_index_none) {
            render_animation animation;

            animation.change_colors = (uint32_t)(uintptr_t)obj->change_colors;
            animation.function_values = (uint32_t)(uintptr_t)obj->function_out_values;
            widget_list_notify(object_index, (render_lighting *)(uintptr_t)data->lighting,
                               &animation);
        }

        if (obj->first_child_object != k_datum_index_none) {
            render_object_list(data, data->shadow_pass != 0 ? 0 : &effect,
                               obj->first_child_object);
        }

    next_sibling:
        object_index = obj->next_object;
    }
}

#if 0
Original Ghidra decompilation (0x50ee20) -- MATERIALLY WRONG, see the evidence note above; kept
only for reference to what it did get approximately right (the field offsets it does show):

void FUN_0050ee20(int param_1,uint *param_2,uint param_3)

{
  short sVar1;
  uint *puVar2;
  uint uVar3;
  char cVar4;
  int iVar5;
  uint *puVar6;
  int iVar7;
  uint *puVar8;
  float10 fVar9;
  uint local_28 [4];
  uint local_18;
  uint local_14;
  uint local_10;
  int local_c;
  uint *local_8;
  uint *local_4;

  while (param_3 != 0xffffffff) {
    iVar7 = (param_3 & 0xffff) * 0xc;
    puVar2 = *(uint **)(*(int *)(DAT_008603b0 + 0x34) + 8 + iVar7);
    cVar4 = FUN_0050ea50();
    if ((cVar4 == '\0') || (DAT_007c3138 != '\0')) {
      /* ... effect-block copy, modifier shader lookup, active camouflage / self occlusion
         checks, render_model call, widget notify, recursion into first_child_object ... */
    }
    param_3 = puVar2[0x45];
  }
  return;
}

Disassembly (objdump -d -M intel, 0x50ee20..0x50f141) -- this is what the rewrite above actually
follows; key excerpts:

0050ee20: sub esp,0x38; push ebx,ebp,esi,edi
0050ee24: mov ebx,[esp+0x48]              ; ebx = object_index (arg3)
0050ee36: mov ebx,[esp+0x54]              ; loop head: reload object_index for this iteration
0050ee40: ... mov ebp,[object_headers[object_index].data]   ; ebp = obj
0050ee59: mov esi,ebx; call 0x50ea50      ; render_object_is_camera_unit(ESI=object_index)
0050ee66: je 0x50ee75                     ; not camera unit -> proceed
0050ee68: mov al,ds:0x7c3138              ; render_camera_global.mirrored
0050ee6f: je 0x50f127                     ; camera unit AND not mirrored -> skip to next sibling
0050ee75: mov edx,[esp+0x4c]              ; edx = data (arg1)
0050ee79: mov al,[edx+8]                  ; al = data->shadow_pass
0050ee7e: jne 0x50eebd                    ; shadow pass: skip the effect-block copy
0050ee80: mov eax,[esp+0x50]              ; eax = parent_effect (arg2)
0050ee84: cmp WORD PTR [eax],0x2          ; parent_effect->type == 2
0050ee93: rep movs ...                    ; copy 10 dwords (render_model_effect) into the local
0050ee95: jne 0x50eeac                    ; not self-occlusion parent -> keep the copy as is
0050ee97..50eeaa: zero local.type/.modifier_shader/.change_colors/.function_values
0050eebd: test BYTE PTR [ebp+0x10],0x1; jne 0x50f0c0   ; no_collision -> skip model draw
0050eec7: tag_data = tag_instances[obj->definition_tag].data
0050eedc: eax=object_index; call 0x50f740               ; object_compute_level_of_detail_pixels
0050eeeb: al = data->shadow_pass; jne 0x50f05c           ; shadow pass -> the reduced-LOD call
0050eef6: eax = tag_data->modifier_shader.tag_id (Object +0x9c); cmp eax,-1; je 0x50ef48
0050ef01..0f: shader_data = tag_instances[eax].data; store into effect.modifier_shader (esp+0x3c)
0050ef17: shader_type = shader_data->shader_type (Shader +0x24, types/tags.h)
0050ef1b..28: shader_type==1 || (4<shader_type<=0xb) -> keep; else clear effect.modifier_shader
0050ef2a..3a: effect.change_colors=&obj->change_colors(+0x1b8), effect.function_values=&obj->
             function_out_values(+0x134)
0050ef48: object_type byte at obj+0xb4; (1<<type)&3 gate (biped/vehicle)
0050ef66: fld [obj+0x37c]                                 ; UNSURE fields
0050ef7d..b6: effect.centroid=obj->bounding_center(+0xa0), effect.type=1, effect.object_index=
             object_index, effect.unit_37c/.unit_380 = obj+0x37c/+0x380
0050efba: test BYTE PTR [tag_data+2],2 (ObjectFlags bit 1); effect.centroid/.type=2/.object_index
0050eff1..f044: builds the render_model call: EAX=tag_data->model.tag_id (+0x34 of the
             TagDependency, i.e. Object+0x28+0xc), ECX=obj + *(int16*)(obj+0x1f2), stack args
             (modifier_shader/shader ptr, &region_permutations(+0x180), &change_colors(+0x1b8),
             &function_out_values(+0x134), data->lighting, &bounding_center(+0xa0),
             bounding_radius(+0xac), &effect, object_index, forced_shader_permutation(+0x176),
             fog flag (data->outside_fog_plane ? 4 : 0))
0050f04d: if (render_debug_objects) object_type_definitions_notify_0x5c(EBX=object_index)
0050f05c..b8: the shadow-pass render_model call: same shape, leftmost stack slot overwritten with
             lod*0.3 (a raw float, not a pointer), &effect replaced with 0, flags literal 2
0050f0c0: if (data->shadow_pass==0 && obj->first_widget(+0x16c)!=-1) widget_list_notify(EDI=
             object_index)
0050f0fc: if (obj->first_child_object(+0x118)!=-1) recurse with parent_effect = (data->
             shadow_pass==0) ? &effect : 0
0050f127: ebp = obj->next_object(+0x114); loop while ebp != -1
#endif
