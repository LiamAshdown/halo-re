// light_volume_render_procedure  (Ghidra: no function; only referenced as the immediate 0x4fea80 that
//   light_volume_render (0x4fe900) hands to rasterizer_lens_flare_occlusion_sample_add)
// address 0x4fea80, size 1019 bytes
// name confidence: 0.6   rewrite confidence: 0.8
// WRITTEN from objdump 0x4fea80..0x4fee7a. The transparent-geometry callback that draws one light volume
//   (stack: object, light volume datum, the ids light_volume_render queued; see
//   rasterizer_transparent_geometry_group_draw's group->index_buffer call).
//   The instance is resolved from the light volume table (0x6b8d70) exactly as light_volume_render does; tags
//   without a count (+0x6e) or frames (+0x120) draw nothing. The frame comes from
//   object_attachment_get_blended_marker (EAX object, ECX tag), the marker (tag +0x00) from
//   object_get_node_local_transform(object, marker, &marker, 1).
//   brightness = clamp01((1 - |marker forward . camera forward|) * perpendicular (+0x3c) + |..| * parallel
//   (+0x40)) * distance fade, where the fade (only with a far fade distance +0x38 > 0) is
//   clamp01((camera-forward distance - far) / (near +0x34 - far)); then scaled by the brightness function
//   (object_function_get_value, CX = tag +0x44 - 1) when it has a value. Nothing is drawn at brightness <= 0,
//   when both tint alphas (frame +0x68, +0x78) are <= 0, or both radii (+0x3c, +0x40) are <= 0.
//   Otherwise the lens flare batch is selected (AX 5, ECX 1) and, unless the bitmap key (EAX tag +0x68 map,
//   ECX 0, stack +0x6c sequence) is refused, count quads are added along the marker's forward axis:
//   t = curve(i / (count - 1), offset exponent +0x14) and, from t, radius (hither/yon +0x3c/+0x40, exponent
//   +0x44), alpha (+0x68/+0x78, brightness exponent +0x8c, times brightness), colour (color_interpolate of the
//   hither/yon rgb +0x6c/+0x7c with the tag's two interpolation flags +0x22, tint exponent +0x88) and position
//   marker + forward * (t * length +0x18 + offset +0x10); the effect slot is released afterwards.
// blam-cc: stack -> object_index, light_volume_handle

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"
#include "bitmaps.h"
#include "fn_rasterizer.h"

extern uint8_t *light_volume_instances; // 0x006b8d70
extern tag_instance *tag_instances;     // 0x0087bc14
extern float render_camera_global; // 0x007c3114
extern float camera_position_y; // 0x007c3118
extern float camera_position_z; // 0x007c311c
extern float camera_forward_x;  // 0x007c3120
extern float camera_forward_y;  // 0x007c3124
extern float camera_forward_z;  // 0x007c3128

extern uint8_t *object_attachment_get_blended_marker(uint32_t object_index, uint8_t *instance); // 0x4fe740, EAX, ECX
extern int32_t object_get_node_local_transform(uint32_t object_index, char *marker_name, object_marker *marker,
    uint32_t flags); // 0x4f6080
extern uint8_t object_function_get_value(uint32_t object_index, int16_t selector, float *out_value); // 0x4f6e70

extern uint8_t rasterizer_lens_flare_set_current_key(int32_t second_bitmap_tag_index, int16_t bitmap_tag_index,
    int16_t bitmap_index); // 0x5120f0, EAX, ECX, stack
extern float curve_apply_exponent(float value, float exponent); // 0x4fea50
extern ColorRGB *color_interpolate(ColorRGB *color1, ColorRGB *color0, ColorRGB *dest,
    color_interpolation_flags flags, float t); // 0x43f6a0, EAX, ECX, stack
extern uint32_t color_pack_argb_from_real(ColorARGB *color); // 0x497900
extern void rasterizer_lens_flare_quad_add(const float *scale, uint32_t diffuse, const real_point3d *position,
    float radius, float rotation_degrees); // 0x537550, EAX, EBX, stack
extern void rasterizer_effect_slot_release_active(void); // 0x512150

static float clamp01(float value)
{
    if (value < 0.0f) {
        return 0.0f;
    }
    if (value > 1.0f) {
        return 1.0f;
    }
    return value;
}

void light_volume_render_procedure(uint32_t object_index, datum_index light_volume_handle)
{
    uint8_t *instance = 0;
    uint8_t *tag;
    uint8_t *frame;
    object_marker marker;
    real_vector3d *forward;
    real_point3d *origin;
    float facing;
    float fade = 1.0f;
    float function_value = 1.0f;
    float brightness;

    if (object_index == 0xffffffff || light_volume_handle == k_datum_index_none) {
        return;
    }
    {
        int16_t index = (int16_t)light_volume_handle;
        int16_t salt = (int16_t)(light_volume_handle >> 16);

        if (index >= 0 && index < *(int16_t *)(light_volume_instances + 0x2e)) {
            uint8_t *slot = *(uint8_t **)(light_volume_instances + 0x34) +
                *(int16_t *)(light_volume_instances + 0x22) * index;

            if (*(int16_t *)slot != 0 && (salt == 0 || salt == *(int16_t *)slot)) {
                instance = slot;
            }
        }
    }
    tag = (uint8_t *)tag_instances[*(uint32_t *)(instance + 4) & 0xffff].data; // 0x4feae3: a bad handle reads 4
    if (*(int16_t *)(tag + 0x6e) <= 0 || *(int32_t *)(tag + 0x120) <= 0) {
        return;
    }
    frame = object_attachment_get_blended_marker(object_index, tag);
    object_get_node_local_transform(object_index, (char *)tag, &marker, 1);
    forward = &marker.node_transform.forward;
    origin = &marker.node_transform.position;

    facing = forward->k * camera_forward_z + forward->j * camera_forward_y + forward->i * camera_forward_x;
    if (facing < 0.0f) {
        facing = -facing;
    }
    if (*(float *)(tag + 0x38) > 0.0f) {
        float distance = (origin->z - camera_position_z) * camera_forward_z +
            (origin->x - render_camera_global) * camera_forward_x + camera_forward_y * (origin->y - camera_position_y);

        fade = clamp01((distance - *(float *)(tag + 0x38)) / (*(float *)(tag + 0x34) - *(float *)(tag + 0x38)));
    }
    brightness = clamp01((1.0f - facing) * *(float *)(tag + 0x3c) + facing * *(float *)(tag + 0x40)) * fade;
    if (object_function_get_value(object_index, (int16_t)(*(uint16_t *)(tag + 0x44) - 1), &function_value)) {
        brightness *= function_value;
    }
    if (brightness <= 0.0f) {
        return;
    }
    if (*(float *)(frame + 0x68) <= 0.0f && *(float *)(frame + 0x78) <= 0.0f) {
        return;
    }
    if (*(float *)(frame + 0x3c) <= 0.0f && *(float *)(frame + 0x40) <= 0.0f) {
        return;
    }

    rasterizer_lens_flare_batching_select_mode(5, 1);
    if (rasterizer_lens_flare_set_current_key(*(int32_t *)(tag + 0x68), 0, (int16_t)*(uint16_t *)(tag + 0x6c)) == 0 &&
        *(int16_t *)(tag + 0x6e) > 0) {
        int16_t count = *(int16_t *)(tag + 0x6e);
        float last = (float)(count - 1);
        int32_t i;

        for (i = 0; i < count; i++) {
            float t = curve_apply_exponent((float)i / last, *(float *)(frame + 0x14));
            float radius_t = curve_apply_exponent(t, *(float *)(frame + 0x44));
            float radius = (1.0f - radius_t) * *(float *)(frame + 0x3c) + radius_t * *(float *)(frame + 0x40);
            float alpha_t = curve_apply_exponent(t, *(float *)(frame + 0x8c));
            float along = t * *(float *)(frame + 0x18) + *(float *)(frame + 0x10);
            real_point3d point;
            ColorARGB color;
            float color_t;

            point.x = forward->i * along + origin->x;
            point.y = forward->j * along + origin->y;
            point.z = forward->k * along + origin->z;
            color_t = curve_apply_exponent(t, *(float *)(frame + 0x88));
            color_interpolate((ColorRGB *)(frame + 0x7c), (ColorRGB *)(frame + 0x6c), (ColorRGB *)&color.red,
                (color_interpolation_flags)(tag[0x22] & 3), color_t);
            color.alpha = ((1.0f - alpha_t) * *(float *)(frame + 0x68) + alpha_t * *(float *)(frame + 0x78)) *
                brightness;
            rasterizer_lens_flare_quad_add(0, color_pack_argb_from_real(&color), &point, radius, 0.0f);
        }
    }
    rasterizer_effect_slot_release_active();
}
