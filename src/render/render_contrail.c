// render_contrail  (Ghidra: contrail_geometry_build_segment, phase-2 name; CEA
// render_contrail(contrail, contrail_definition, instance_index), hint only; renamed)
// address 0x50e090, size 2071 bytes
// name confidence: 0.7   rewrite confidence: 0.7
// evidence: objdump -d -M intel 0x50e090..0x50e8ae (jump table at 0x50e8b0: 0 -> 0x50e457,
//   1 / 2 -> 0x50e4e8, 3 -> 0x50e8a4, 4 -> 0x50e595), every stack slot followed. Types:
//   types/effects.h contrail (0x44) and contrail_point (0x38), types/tags.h Contrail (flags +0,
//   scale_flags +2, render_type +0x18, texture_repeats_u / v +0x1c / +0x20, bitmap id +0x3c,
//   the shader block +0x84 with framebuffer_fade_mode at +0xb0, point_states +0x138 / +0x13c)
//   and ContrailPointState (0x68: width +0x40, colour bounds +0x44 / +0x54, scale_flags +0x64).
//   - cdecl (contrail, definition, instance): the caller render_contrails 0x50df20 pushes them.
//   - bitmap_group_sequence_get_bitmap_data 0x43f290 (EAX bitmap id, DI frame +0x16, stack the
//     sequence +0x14), rasterizer_vertex_buffer_lock_state (0x0069c632) = 0xf, texture_cache_get
//     0x444550 (EAX bitmap, stack 0, 1); nothing is drawn when the texture is not resident.
//   - n = point_count[instance] - 1 segments: index reserve 0x51bd60 (EDX = 2n), vertex reserve
//     0x51bdd0 (AX = 6, ESI = 2n + 2), index lock 0x511e80 (ECX slot), vertex lock 0x51be40 (EAX
//     slot).
//   - per point, two vertices: width and colour from the point state (scaled by the point scale
//     when the state scale flags have 0x10 / 0x20), blended toward the next state by the point
//     age when point flag 0x02 is set; colour rgb times the owner object's change colour when
//     the Object tag attachment (+0x144 pointer, stride 0x48, +0x34 change colour index) names
//     one; u runs from texture_offset_u in steps of -texture_repeats_u (times scale for scale
//     flag 0x40), v is texture_offset_v + texture_repeats_v (scale flag 0x80) on the first
//     vertex and texture_offset_v on the second.
//   - render_type 0: vertical pair (z -+ half width); 1 / 2: horizontal pair across the 2D
//     segment normal (vector2d_normalize_with_length 0x4018e0); 4: viewer facing, across
//     normalize(cross(camera - point, segment)); 3 and anything above 4 return at once, with
//     both buffers still locked and 0x0069c632 left at 0xf (reproduced). The segment runs to
//     the previous point, or to the next point for the first one.
//   - when the shader fade mode is non zero a fade normal is also built (0: the 2D segment
//     normal, 1 / 2: *global_up3d_pointer, 4: normalize(cross(segment, side))) for
//     contrail_compute_edge_fade_factor 0x50e000 (EAX normal, ECX point, SI fade mode, stack the
//     definition flags); the alpha is clamped to [0, 1] and packed by 0x497900.
//   - after the points: the first pair gets alpha 0 unless flags bit 0 (first_point_unfaded),
//     the last pair unless bit 1 (last_point_unfaded); indices (a-1, a, a+1), (a+1, a, a+2) for
//     a = 1, 3, ...; the centroid is the mean point; both buffers are unlocked
//     (IDirect3DIndexBuffer9 / IDirect3DVertexBuffer9::Unlock, vtable +0x30) and
//     rasterizer_transparent_object_append 0x51c830 (EDX centroid, EDI shader, stack (bitmap,
//     index slot, vertex slot, 2n, 0)) queues the draw; 0x0069c632 = 0.
// register convention: cdecl, three stack arguments.
//   // blam-cc: cdecl
// UNSURE: the fade normal with the fade mode off is left uninitialized and still handed to
//   0x50e000 (which then does not read it). The vertex buffer unlock goes through the rasterizer
//   tables 0x006d99d8 (slot -> vertex type), 0x006d98f0 (dynamic cache buffer handle, stride
//   0xc) and 0x007bf04c (+ handle * 0x14), as build_sprites_end 0x511620 does.
// reconciled: R77 0x0069c632 extern uint16 -> int16 (rasterizer.h type)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "rasterizer.h"
#include "objects.h"
#include "effects.h"
#include <stdint.h> // uintptr_t

extern data_array *contrail_point_data;              // 0x0087abe8, effects module
extern data_array *object_data;                      // 0x008603b0, objects module
extern tag_instance *tag_instances;                  // 0x0087bc14
extern real_point3d *global_zero_vector3d_pointer;                // 0x006966f8 -> {0,0,0}
extern real_vector3d *global_up3d_pointer;                   // 0x00696720 -> {0,0,1}
extern render_camera render_camera_global;           // 0x007c3114, this module
extern int16_t rasterizer_vertex_buffer_lock_state; // 0x0069c632, rasterizer module
extern void *rasterizer_dynamic_index_buffer;        // 0x006e09e8 IDirect3DIndexBuffer9*
extern rasterizer_dynamic_vertex_slot rasterizer_dynamic_vertex_slots[k_rasterizer_dynamic_vertex_slots];
    // 0x006d99d8, rasterizer module
extern rasterizer_dynamic_vertex_cache rasterizer_dynamic_vertex_caches[k_rasterizer_vertex_type_count];
    // 0x006d98e8, rasterizer module
extern rasterizer_vertex_buffer_slot rasterizer_vertex_buffer_slots[k_rasterizer_vertex_buffer_slots];
    // 0x007bf060, rasterizer module (handle h addresses slot h - 1)

extern BitmapData *bitmap_group_sequence_get_bitmap_data(datum_index bitmap_tag, int16_t frame,
                                                        int16_t sequence);
    // 0x43f290, blam-cc: EAX tag, DI frame, stack sequence
extern void *texture_cache_get(BitmapData *bitmap, uint8_t wait, uint8_t allocate_if_missing);
    // 0x444550, cache module; blam-cc: EAX bitmap, stack (wait, allocate_if_missing)
extern int32_t rasterizer_dynamic_index_cache_reserve(int32_t count); // 0x51bd60; EDX = count
extern int32_t rasterizer_dynamic_vertex_cache_reserve(int16_t vertex_type, int32_t count);
    // 0x51bdd0; blam-cc: AX = vertex_type, ESI = count
extern void *rasterizer_dynamic_index_slot_lock(int32_t slot_index); // 0x511e80; ECX
extern void *rasterizer_dynamic_vertex_cache_lock(int32_t slot_index); // 0x51be40; EAX
extern real vector3d_normalize_with_length(real_vector3d *v); // 0x401990; ECX -> v
extern real vector2d_normalize_with_length(real_vector2d *v); // 0x4018e0; ECX -> v
extern void vector3d_cross_product(real_vector3d *out, const real_vector3d *a, const real_vector3d *b);
    // 0x4052c0, math module; blam-cc: EAX -> out, stack -> a, ECX -> b
extern real contrail_compute_edge_fade_factor(real_vector3d *direction, real_point3d *point,
    int16_t fade_mode, uint8_t *flags);
    // 0x50e000, this module; blam-cc: EAX=direction, ECX=point, ESI=fade_mode, stack=flags
extern uint32_t color_pack_argb_from_real(ColorARGB *color); // 0x497900 (cdecl)
extern void rasterizer_transparent_object_append(uint32_t lightmap_bitmap, int32_t dynamic_index_slot,
    int32_t dynamic_vertex_slot, int32_t primitive_count, uint32_t flags,
    real_point3d *world_position, Shader *shader);
    // 0x51c830, rasterizer module; blam-cc: stack params as declared; EDX = world_position,
    // EDI = shader

typedef int32_t (__stdcall *d3d_unlock_fn)(void *self);

static void point_state_width_and_color(ContrailPointState *state, contrail_point *point,
                                        float *width, ColorARGB *color)
{
    float t = 1.0f;

    if ((state->scale_flags & 0x20) != 0) {
        t = point->scale;
    }
    *width = state->width;
    if ((state->scale_flags & 0x10) != 0) {
        *width = *width * point->scale;
    }
    color->alpha = (state->color_upper_bound.alpha - state->color_lower_bound.alpha) * t +
                   state->color_lower_bound.alpha;
    color->red = (state->color_upper_bound.red - state->color_lower_bound.red) * t +
                 state->color_lower_bound.red;
    color->green = (state->color_upper_bound.green - state->color_lower_bound.green) * t +
                   state->color_lower_bound.green;
    color->blue = (state->color_upper_bound.blue - state->color_lower_bound.blue) * t +
                  state->color_lower_bound.blue;
}

// Builds the triangle list of one contrail point list (instance) as a strip of vertex pairs
// oriented by the Contrail render type, and queues it as a transparent draw.
void render_contrail(contrail *c, Contrail *definition, int16_t instance)
{
    BitmapData *bitmap;
    int16_t segment_count;
    int32_t primitive_count;
    int32_t vertex_count;
    int32_t index_slot;
    int32_t vertex_slot;
    uint16_t *indices;
    rasterizer_dynamic_screen_vertex *vertices;
    uint8_t has_fade;
    Shader *shader;
    real_point3d centroid;
    float u;
    float u_step;
    float v_top;
    float v_bottom;
    datum_index point_index;
    contrail_point *previous = 0;
    ContrailPointState *states;
    real_vector3d fade_normal;

    bitmap = bitmap_group_sequence_get_bitmap_data(*(datum_index *)&definition->bitmap.tag_id,
                                                   c->frame_index, c->sequence_index);
    rasterizer_vertex_buffer_lock_state = 0xf;
    if (texture_cache_get(bitmap, 0, 1) == 0) {
        rasterizer_vertex_buffer_lock_state = 0;
        return;
    }

    segment_count = (int16_t)(c->point_count[instance] - 1);
    primitive_count = (int32_t)(int16_t)(segment_count * 2);
    index_slot = rasterizer_dynamic_index_cache_reserve(primitive_count);
    vertex_count = (int32_t)(int16_t)(segment_count * 2 + 2);
    vertex_slot = rasterizer_dynamic_vertex_cache_reserve(_rasterizer_vertex_type_dynamic_unlit,
                                                          vertex_count);
    if (index_slot == -1 || vertex_slot == -1) {
        rasterizer_vertex_buffer_lock_state = 0;
        return;
    }
    indices = (uint16_t *)rasterizer_dynamic_index_slot_lock(index_slot);
    vertices = (rasterizer_dynamic_screen_vertex *)rasterizer_dynamic_vertex_cache_lock(vertex_slot);

    has_fade = definition->framebuffer_fade_mode != 0;
    centroid = *global_zero_vector3d_pointer;
    shader = (Shader *)((uint8_t *)definition + 0x84);
    u = c->texture_offset_u;
    u_step = definition->texture_repeats_u;
    if ((definition->scale_flags & 0x40) != 0) {
        u_step = u_step * c->scale;
    }
    u_step = -u_step;
    v_bottom = c->texture_offset_v;
    v_top = definition->texture_repeats_v;
    if ((definition->scale_flags & 0x80) != 0) {
        v_top = v_top * c->scale;
    }
    v_top = v_top + v_bottom;
    states = (ContrailPointState *)definition->point_states.pointer;

    for (point_index = c->first_point[instance]; point_index != 0xffffffff;
         point_index = previous->next_point) {
        contrail_point *point = &((contrail_point *)contrail_point_data->data)[(uint16_t)point_index];
        ContrailPointState *state = &states[point->state_index];
        float width;
        float half_width;
        ColorARGB color;
        contrail_point *next = 0;

        point_state_width_and_color(state, point, &width, &color);
        if ((point->flags & 0x02) != 0) {
            float next_width;
            ColorARGB next_color;
            float t = point->age;

            point_state_width_and_color(&states[point->state_index + 1], point, &next_width,
                                        &next_color);
            width = (next_width - width) * t + width;
            color.alpha = (next_color.alpha - color.alpha) * t + color.alpha;
            color.red = (next_color.red - color.red) * t + color.red;
            color.green = (next_color.green - color.green) * t + color.green;
            color.blue = (next_color.blue - color.blue) * t + color.blue;
        }
        if (c->object_index != 0xffffffff) {
            object *o = ((object_header *)object_data->data)[(uint16_t)c->object_index].data;
            Object *object_definition = (Object *)tag_instances[o->definition_tag & 0xffff].data;
            int16_t change_color = (int16_t)(*(int16_t *)((uint8_t *)object_definition->attachments.pointer +
                                             (int32_t)c->attachment_index * 0x48 + 0x34) - 1);

            if (change_color != -1) {
                color.red = color.red * o->change_colors[change_color].red;
                color.green = color.green * o->change_colors[change_color].green;
                color.blue = color.blue * o->change_colors[change_color].blue;
            }
        }
        half_width = width * 0.5f;

        vertices[0].u = u;
        vertices[0].v = v_top;
        vertices[1].u = u;
        vertices[1].v = v_bottom;
        centroid.x = centroid.x + point->position.x;
        centroid.y = centroid.y + point->position.y;
        centroid.z = centroid.z + point->position.z;
        if (previous == 0) {
            next = &((contrail_point *)contrail_point_data->data)[(uint16_t)point->next_point];
        }

        switch (definition->render_type) {
        case 0:
            vertices[0].x = point->position.x;
            vertices[0].y = point->position.y;
            vertices[0].z = point->position.z - half_width;
            vertices[1].x = point->position.x;
            vertices[1].y = point->position.y;
            vertices[1].z = half_width + point->position.z;
            if (has_fade) {
                if (previous != 0) {
                    fade_normal.k = 0.0f;
                    fade_normal.i = previous->position.y - point->position.y;
                    fade_normal.j = point->position.x - previous->position.x;
                } else {
                    fade_normal.i = point->position.y - next->position.y;
                    fade_normal.j = next->position.x - point->position.x;
                    fade_normal.k = 0.0f;
                }
                vector3d_normalize_with_length(&fade_normal);
            }
            break;
        case 1:
        case 2: {
            real_vector2d side;

            if (previous != 0) {
                side.i = previous->position.y - point->position.y;
                side.j = point->position.x - previous->position.x;
            } else {
                side.i = point->position.y - next->position.y;
                side.j = next->position.x - point->position.x;
            }
            vector2d_normalize_with_length(&side);
            vertices[0].x = point->position.x - side.i * half_width;
            vertices[0].y = point->position.y - side.j * half_width;
            vertices[0].z = point->position.z;
            vertices[1].x = side.i * half_width + point->position.x;
            vertices[1].y = side.j * half_width + point->position.y;
            vertices[1].z = point->position.z;
            if (has_fade) {
                fade_normal = *global_up3d_pointer;
            }
            break;
        }
        case 4: {
            real_vector3d to_camera;
            real_vector3d segment;
            real_vector3d side;
            real_point3d *from = previous != 0 ? &previous->position : &point->position;

            to_camera.i = render_camera_global.position.x - from->x;
            to_camera.j = render_camera_global.position.y - from->y;
            to_camera.k = render_camera_global.position.z - from->z;
            if (previous != 0) {
                segment.i = point->position.x - previous->position.x;
                segment.j = point->position.y - previous->position.y;
                segment.k = point->position.z - previous->position.z;
            } else {
                segment.i = next->position.x - point->position.x;
                segment.j = next->position.y - point->position.y;
                segment.k = next->position.z - point->position.z;
            }
            side.i = segment.k * to_camera.j - segment.j * to_camera.k;
            side.j = segment.i * to_camera.k - segment.k * to_camera.i;
            side.k = segment.j * to_camera.i - segment.i * to_camera.j;
            vector3d_normalize_with_length(&side);
            vertices[0].x = point->position.x - side.i * half_width;
            vertices[0].y = point->position.y - side.j * half_width;
            vertices[0].z = point->position.z - side.k * half_width;
            vertices[1].x = side.i * half_width + point->position.x;
            vertices[1].y = side.j * half_width + point->position.y;
            vertices[1].z = side.k * half_width + point->position.z;
            if (has_fade) {
                // VERIFIED against disassembly 0x50e6dc..0x50e6e9 (2026-09-30): EAX=fade_normal, ECX=side (esp+0x74), stack=segment (esp+0x44)
                vector3d_cross_product(&fade_normal, &side, &segment);
                vector3d_normalize_with_length(&fade_normal);
            }
            break;
        }
        default:
            // render_type 3 (ground mapped) and anything above 4 leave at once (0x50e8a4)
            return;
        }

        color.alpha = contrail_compute_edge_fade_factor(&fade_normal, &point->position,
                                                        definition->framebuffer_fade_mode,
                                                        (uint8_t *)&definition->flags) * color.alpha;
        if (color.alpha < 0.0f) {
            color.alpha = 0.0f;
        } else if (color.alpha > 1.0f) {
            color.alpha = 1.0f;
        }
        vertices[1].color = color_pack_argb_from_real(&color);
        vertices[0].color = vertices[1].color;
        u = u_step + u;

        previous = point;
        vertices += 2;
    }

    vertices -= vertex_count;
    if ((definition->flags & 1) == 0) {
        ((uint8_t *)&vertices[0].color)[3] = 0;
        ((uint8_t *)&vertices[1].color)[3] = 0;
    }
    if ((definition->flags & 2) == 0) {
        ((uint8_t *)&vertices[vertex_count - 1].color)[3] = 0;
        ((uint8_t *)&vertices[vertex_count - 2].color)[3] = 0;
    }
    if (segment_count > 0) {
        uint16_t a = 1;
        uint16_t k;

        for (k = 0; k < (uint16_t)segment_count; k++) {
            indices[0] = (uint16_t)(a - 1);
            indices[1] = a;
            indices[2] = (uint16_t)(a + 1);
            indices[4] = a;
            indices[3] = (uint16_t)(a + 1);
            indices[5] = (uint16_t)(a + 2);
            a = (uint16_t)(a + 2);
            indices += 6;
        }
    }

    {
        float inverse = 1.0f / (float)(int32_t)c->point_count[instance];
        void *vertex_buffer;
        int32_t buffer_handle;

        centroid.x = centroid.x * inverse;
        centroid.y = centroid.y * inverse;
        centroid.z = inverse * centroid.z;
        ((d3d_unlock_fn)(*(void ***)rasterizer_dynamic_index_buffer)[0x30 / 4])(
            rasterizer_dynamic_index_buffer);
        buffer_handle = rasterizer_dynamic_vertex_caches[
            rasterizer_dynamic_vertex_slots[vertex_slot].vertex_type].buffer_handle;
        if (buffer_handle != 0) {
            vertex_buffer = (void *)(uintptr_t)rasterizer_vertex_buffer_slots[buffer_handle - 1].hardware_buffer;
            ((d3d_unlock_fn)(*(void ***)vertex_buffer)[0x30 / 4])(vertex_buffer);
        }
        rasterizer_transparent_object_append((uint32_t)(uintptr_t)bitmap, index_slot, vertex_slot,
                                             primitive_count, 0, &centroid, shader);
    }
    rasterizer_vertex_buffer_lock_state = 0;
}

#if 0
Original Ghidra decompilation (0x50e090):

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void contrail_geometry_build_segment(int param_1,byte *param_2,short param_3)

{
  float *pfVar1;
  float fVar2;
  float fVar3;
  float fVar4;
  uint uVar5;
  uint *puVar6;
  float10 fVar7;
  short sVar8;
  short sVar9;
  int iVar10;
  short *psVar11;
  float *pfVar12;
  int iVar13;
  float fVar14;
  int iVar15;
  int iVar16;
  bool bVar17;
  float10 fVar18;
  float local_a0;
  float local_9c;
  float local_98;
  float local_94;
  float local_90;
  float local_8c;
  float local_88;
  float local_84;
  float local_80;
  float local_7c;
  float local_78;
  float local_74;
  float local_70;
  float local_6c;
  uint local_68;
  float local_64;
  float local_60;
  undefined4 local_5c;
  float local_58;
  float local_54;
  int local_50;
  byte *local_4c;
  float local_48;
  float local_44;
  float local_40;
  float local_3c;
  int local_38;
  int local_34;
  int local_30;
  float local_2c;
  float local_28;
  float local_24;
  float local_20;
  short *local_1c;
  undefined4 local_18;
  float local_14;
  float local_10;
  float local_c;
  float local_8;
  float local_4;
  
  local_18 = bitmap_group_sequence_get_bitmap_data(*(undefined2 *)(param_1 + 0x14));
  _DAT_0069c632 = 0xf;
  iVar10 = texture_cache_get(0,1);
  if (iVar10 != 0) {
    sVar9 = *(short *)(param_1 + 0x2c + param_3 * 2);
    sVar8 = sVar9 + -1;
    local_68 = CONCAT22(sVar9 >> 0xf,sVar8);
    sVar8 = sVar8 * 2;
    local_34 = (int)sVar8;
    iVar10 = FUN_0051bd60();
    local_38 = (int)(short)(sVar8 + 2);
    local_30 = iVar10;
    local_50 = FUN_0051bdd0();
    if ((iVar10 != -1) && (local_50 != -1)) {
      psVar11 = (short *)FUN_00511e80();
      local_1c = psVar11;
      pfVar12 = (float *)FUN_0051be40();
      local_14 = *(float *)(param_2 + 0x1c);
      local_84 = *(float *)PTR_DAT_006966f8;
      bVar17 = *(short *)(param_2 + 0xb0) != 0;
      local_80 = *(float *)(PTR_DAT_006966f8 + 4);
      local_7c = *(float *)(PTR_DAT_006966f8 + 8);
      local_4c = param_2 + 0x84;
      local_78 = *(float *)(param_1 + 0x18);
      if ((*(ushort *)(param_2 + 2) & 0x40) != 0) {
        local_14 = local_14 * *(float *)(param_1 + 0x10);
      }
      local_14 = -local_14;
      local_48 = *(float *)(param_1 + 0x1c);
      local_2c = *(float *)(param_2 + 0x20);
      if ((char)*(ushort *)(param_2 + 2) < '\0') {
        local_2c = local_2c * *(float *)(param_1 + 0x10);
      }
      local_2c = local_2c + local_48;
      uVar5 = *(uint *)(param_1 + 0x34 + param_3 * 4);
      iVar10 = 0;
      while (uVar5 != 0xffffffff) {
        iVar13 = *(int *)(param_2 + 0x13c);
        iVar15 = (uVar5 & 0xffff) * 0x38;
        iVar16 = iVar15 + *(int *)(DAT_0087abe8 + 0x34);
        iVar15 = *(char *)(iVar15 + 3 + *(int *)(DAT_0087abe8 + 0x34)) * 0x68;
        uVar5 = *(uint *)(iVar15 + 100 + iVar13);
        iVar15 = iVar15 + iVar13;
        local_8c = 1.0;
        if ((uVar5 & 0x20) != 0) {
          local_8c = *(float *)(iVar16 + 0xc);
        }
        fVar14 = *(float *)(iVar15 + 0x40);
        if ((uVar5 & 0x10) != 0) {
          fVar14 = fVar14 * *(float *)(iVar16 + 0xc);
        }
        local_a0 = (*(float *)(iVar15 + 0x54) - *(float *)(iVar15 + 0x44)) * local_8c +
                   *(float *)(iVar15 + 0x44);
        local_9c = (*(float *)(iVar15 + 0x58) - *(float *)(iVar15 + 0x48)) * local_8c +
                   *(float *)(iVar15 + 0x48);
        local_98 = (*(float *)(iVar15 + 0x5c) - *(float *)(iVar15 + 0x4c)) * local_8c +
                   *(float *)(iVar15 + 0x4c);
        local_94 = (*(float *)(iVar15 + 0x60) - *(float *)(iVar15 + 0x50)) * local_8c +
                   *(float *)(iVar15 + 0x50);
        if ((*(byte *)(iVar16 + 2) & 2) != 0) {
          local_88 = *(float *)(iVar16 + 4);
          iVar13 = (*(char *)(iVar16 + 3) + 1) * 0x68 + iVar13;
          local_90 = 1.0;
          if ((*(uint *)(iVar13 + 100) & 0x20) != 0) {
            local_90 = *(float *)(iVar16 + 0xc);
          }
          fVar2 = *(float *)(iVar13 + 0x40);
          if ((*(uint *)(iVar13 + 100) & 0x10) != 0) {
            fVar2 = fVar2 * *(float *)(iVar16 + 0xc);
          }
          local_10 = (*(float *)(iVar13 + 0x54) - *(float *)(iVar13 + 0x44)) * local_90 +
                     *(float *)(iVar13 + 0x44);
          local_c = (*(float *)(iVar13 + 0x58) - *(float *)(iVar13 + 0x48)) * local_90 +
                    *(float *)(iVar13 + 0x48);
          local_8 = (*(float *)(iVar13 + 0x5c) - *(float *)(iVar13 + 0x4c)) * local_90 +
                    *(float *)(iVar13 + 0x4c);
          local_4 = (*(float *)(iVar13 + 0x60) - *(float *)(iVar13 + 0x50)) * local_90 +
                    *(float *)(iVar13 + 0x50);
          fVar14 = (fVar2 - fVar14) * local_88 + fVar14;
          local_a0 = (local_10 - local_a0) * local_88 + local_a0;
          local_9c = (local_c - local_9c) * local_88 + local_9c;
          local_98 = (local_8 - local_98) * local_88 + local_98;
          local_94 = (local_4 - local_94) * local_88 + local_94;
        }
        if (*(uint *)(param_1 + 8) != 0xffffffff) {
          puVar6 = *(uint **)(*(int *)(DAT_008603b0 + 0x34) + 8 +
                             (*(uint *)(param_1 + 8) & 0xffff) * 0xc);
          sVar9 = *(short *)(*(int *)(*(int *)((*puVar6 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14) +
                                     0x144) + 0x34 + *(short *)(param_1 + 0xc) * 0x48) + -1;
          if (sVar9 != -1) {
            pfVar1 = (float *)(puVar6 + sVar9 * 3 + 0x6e);
            local_9c = local_9c * *pfVar1;
            local_98 = local_98 * pfVar1[1];
            local_94 = local_94 * pfVar1[2];
          }
        }
        fVar14 = fVar14 * 0.5;
        pfVar12[4] = local_78;
        pfVar12[5] = local_2c;
        pfVar12[10] = local_78;
        pfVar12[0xb] = local_48;
        local_84 = local_84 + *(float *)(iVar16 + 0x1c);
        pfVar1 = (float *)(iVar16 + 0x1c);
        local_80 = local_80 + *(float *)(iVar16 + 0x20);
        local_7c = local_7c + *(float *)(iVar16 + 0x24);
        switch(*(undefined2 *)(param_2 + 0x18)) {
        case 0:
          *pfVar12 = *pfVar1;
          pfVar12[1] = *(float *)(iVar16 + 0x20);
          pfVar12[2] = *(float *)(iVar16 + 0x24) - fVar14;
          pfVar12[6] = *pfVar1;
          pfVar12[7] = *(float *)(iVar16 + 0x20);
          pfVar12[8] = fVar14 + *(float *)(iVar16 + 0x24);
          if (bVar17) {
            if (iVar10 == 0) {
              iVar10 = (*(uint *)(iVar16 + 0x34) & 0xffff) * 0x38;
              local_64 = *(float *)(iVar16 + 0x20) -
                         *(float *)(iVar10 + 0x20 + *(int *)(DAT_0087abe8 + 0x34));
              local_60 = *(float *)(iVar10 + 0x1c + *(int *)(DAT_0087abe8 + 0x34)) - *pfVar1;
              local_5c = 0;
            }
            else {
              local_5c = 0;
              local_64 = *(float *)(iVar10 + 0x20) - *(float *)(iVar16 + 0x20);
              local_60 = *pfVar1 - *(float *)(iVar10 + 0x1c);
            }
LAB_0050e6f1:
            vector3d_normalize_with_length();
          }
          break;
        case 1:
        case 2:
          if (iVar10 == 0) {
            iVar10 = (*(uint *)(iVar16 + 0x34) & 0xffff) * 0x38;
            local_58 = *(float *)(iVar16 + 0x20) -
                       *(float *)(iVar10 + 0x20 + *(int *)(DAT_0087abe8 + 0x34));
            local_54 = *(float *)(iVar10 + 0x1c + *(int *)(DAT_0087abe8 + 0x34)) - *pfVar1;
          }
          else {
            local_58 = *(float *)(iVar10 + 0x20) - *(float *)(iVar16 + 0x20);
            local_54 = *pfVar1 - *(float *)(iVar10 + 0x1c);
          }
          vector2d_normalize_with_length();
          *pfVar12 = *pfVar1 - local_58 * fVar14;
          pfVar12[1] = *(float *)(iVar16 + 0x20) - local_54 * fVar14;
          pfVar12[2] = *(float *)(iVar16 + 0x24);
          pfVar12[6] = local_58 * fVar14 + *pfVar1;
          pfVar12[7] = local_54 * fVar14 + *(float *)(iVar16 + 0x20);
          pfVar12[8] = *(float *)(iVar16 + 0x24);
          if (bVar17) {
            local_64 = *(float *)PTR_DAT_00696720;
            local_60 = *(float *)(PTR_DAT_00696720 + 4);
            local_5c = *(undefined4 *)(PTR_DAT_00696720 + 8);
          }
          break;
        default:
          goto switchD_0050e450_caseD_3;
        case 4:
          if (iVar10 == 0) {
            fVar3 = *pfVar1;
            fVar2 = *(float *)(iVar16 + 0x20);
            iVar10 = *(int *)(DAT_0087abe8 + 0x34);
            fVar4 = *(float *)(iVar16 + 0x24);
            iVar13 = (*(uint *)(iVar16 + 0x34) & 0xffff) * 0x38;
            local_74 = *(float *)(iVar13 + 0x1c + iVar10) - *pfVar1;
            local_70 = *(float *)(iVar13 + 0x20 + iVar10) - *(float *)(iVar16 + 0x20);
            local_6c = *(float *)(iVar13 + iVar10 + 0x24) - *(float *)(iVar16 + 0x24);
          }
          else {
            fVar3 = *(float *)(iVar10 + 0x1c);
            fVar2 = *(float *)(iVar10 + 0x20);
            fVar4 = *(float *)(iVar10 + 0x24);
            local_74 = *pfVar1 - *(float *)(iVar10 + 0x1c);
            local_70 = *(float *)(iVar16 + 0x20) - *(float *)(iVar10 + 0x20);
            local_6c = *(float *)(iVar16 + 0x24) - *(float *)(iVar10 + 0x24);
          }
          local_44 = local_6c * (DAT_007c3118 - fVar2) - local_70 * (DAT_007c311c - fVar4);
          local_40 = local_74 * (DAT_007c311c - fVar4) - local_6c * (DAT_007c3114 - fVar3);
          local_3c = local_70 * (DAT_007c3114 - fVar3) - local_74 * (DAT_007c3118 - fVar2);
          local_28 = local_44;
          local_24 = local_40;
          local_20 = local_3c;
          vector3d_normalize_with_length();
          *pfVar12 = *pfVar1 - local_44 * fVar14;
          pfVar12[1] = *(float *)(iVar16 + 0x20) - local_40 * fVar14;
          pfVar12[2] = *(float *)(iVar16 + 0x24) - local_3c * fVar14;
          pfVar12[6] = local_44 * fVar14 + *pfVar1;
          pfVar12[7] = local_40 * fVar14 + *(float *)(iVar16 + 0x20);
          pfVar12[8] = local_3c * fVar14 + *(float *)(iVar16 + 0x24);
          if (bVar17) {
            vector3d_cross_product(&local_74);
            goto LAB_0050e6f1;
          }
        }
        fVar18 = (float10)contrail_compute_edge_fade_factor(param_2);
        fVar7 = (float10)local_a0;
        local_a0 = (float)(fVar18 * fVar7);
        if ((float10)0.0 <= fVar18 * fVar7) {
          if (1.0 < local_a0) {
            local_a0 = 1.0;
          }
        }
        else {
          local_a0 = 0.0;
        }
        fVar14 = (float)color_pack_argb_from_real(&local_a0);
        local_78 = local_14 + local_78;
        pfVar12[9] = fVar14;
        pfVar12[3] = fVar14;
        pfVar12 = pfVar12 + 0xc;
        iVar10 = iVar16;
        psVar11 = local_1c;
        uVar5 = *(uint *)(iVar16 + 0x34);
      }
      if ((*param_2 & 1) == 0) {
        *(undefined1 *)((int)pfVar12 + local_38 * -0x18 + 0xf) = 0;
        *(undefined1 *)((int)pfVar12 + local_38 * -0x18 + 0x27) = 0;
      }
      if ((*param_2 & 2) == 0) {
        *(undefined1 *)((int)pfVar12 + -9) = 0;
        *(undefined1 *)((int)pfVar12 + -0x21) = 0;
      }
      if (0 < (short)local_68) {
        sVar9 = 1;
        local_68 = local_68 & 0xffff;
        do {
          *psVar11 = sVar9 + -1;
          psVar11[1] = sVar9;
          sVar8 = sVar9 + 1;
          psVar11[2] = sVar8;
          psVar11[4] = sVar9;
          sVar9 = sVar9 + 2;
          psVar11[3] = sVar8;
          psVar11[5] = sVar9;
          psVar11 = psVar11 + 6;
          local_68 = local_68 - 1;
        } while (local_68 != 0);
      }
      local_68 = (uint)*(short *)(param_1 + 0x2c + param_3 * 2);
      fVar14 = 1.0 / (float)(int)local_68;
      local_84 = local_84 * fVar14;
      local_80 = local_80 * fVar14;
      local_7c = fVar14 * local_7c;
      (**(code **)(*DAT_006e09e8 + 0x30))(DAT_006e09e8);
      fVar14 = local_54;
      if ((&DAT_006d98f0)[*(short *)(&DAT_006d99d8 + (int)local_54 * 0x10) * 3] != 0) {
        (**(code **)(**(int **)(&DAT_007bf04c +
                               (&DAT_006d98f0)[*(short *)(&DAT_006d99d8 + (int)local_54 * 0x10) * 3]
                               * 10) + 0x30))
                  (*(int **)(&DAT_007bf04c +
                            (&DAT_006d98f0)[*(short *)(&DAT_006d99d8 + (int)local_54 * 0x10) * 3] *
                            10));
      }
      FUN_0051c830(local_1c,local_34,fVar14,local_38,0);
    }
  }
  _DAT_0069c632 = 0;
switchD_0050e450_caseD_3:
  return;
}
#endif
