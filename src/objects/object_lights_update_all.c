// object_lights_update_all
// address 0x4f0cf0, size 2306 bytes (0x4f0cf0..0x4f15f1; Ghidra split it at 0x4f133c into a second "function",
//   formerly src/objects/object_lights_update_all_continued.c, which is the tail of this loop)
// name confidence: 0.5 (out/phase4/objects_functions.md)
// rewrite confidence: 0.8
// REWRITTEN (first-boot track, objdump 0x4f0cf0..0x4f15f1) -- the old transcription called the render callbacks
//   with no arguments, passed zeros for the five collection callbacks, returned out of the loop into its own tail
//   and never filled the light record it queued. Per frame:
//   1. every light: flag bit 3 cleared, queue slot (+8) = -1. A transient light (creation tick at +0x58) older than
//      its tag's duration (+0xf4) leaves the clusters and is deleted; a persistent one whose owner object (+0x2c)
//      still exists leaves the clusters when flag bit 1 is set (and clears bit 2), then object_light_recompute_transform.
//   2. light_frame_counter++, the visible lights are collected (structure_bsp_collect_visible_objects with the five
//      light callbacks at 0x4f34c0..0x4f3650), the rasterizer lights are cleared and each visible cluster adds
//      its BSP lens flares.
//   3. each collected light: its colour (+0x14) is interpolated -- persistent: between the tag's two colours by the
//      owner's function output (+0x134 + 4 * secondary index at +0x5e, else 1) tinted by the owner's change colour
//      (+0x1b8 + 12 * index at +0x60) or the default colour; transient: by 1 - transition(age / duration) times
//      +0x78. A light on a unit root whose +0x37c is positive is dimmed by 1 - that (unless tag flag bit 5). A black
//      light is skipped. With flag bit 0 the radius (+0x54) is the intensity blend of tag +0x08/+0x0c times +0x04
//      and a non-zero one is queued as a rasterizer_light (first-person flashlights and weapon markers move it
//      first and set flag bit 3) into the 0x80-entry table at 0x007c1484 and rasterizer_light_set; otherwise the
//      radius is tag +0x04.
//   4. a light whose tag has a lens flare (+0xb8) adds one flare instance -- at the light for a transient light,
//      at each marker (first-person weapon markers first, else the owner's markers) for a persistent one.
//   5. the frame's transient flares (0x008609cc, count 0x00860b0c) are added and the count cleared.
//   The flare candidate's sample_count (+0x24) is never written by the original (stack garbage); it is zeroed here.
// register convention: none (no parameters).

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"
#include "structures.h"
#include "rasterizer.h"

extern uint8_t *game_time; // 0x006f1d6c, tick count at +0xc
extern data_array *light_data; // 0x00860b14
extern data_array *object_data; // 0x008603b0
extern tag_instance *tag_instances; // 0x0087bc14
extern cluster_reference_group light_cluster_group; // 0x00860b20
extern int16_t light_transient_count_or_queue; // 0x00860b10
extern int32_t light_frame_counter; // 0x008607c4
extern uint8_t light_render_unknown_7c0; // 0x008607c0
extern datum_index light_active_list[0x80]; // 0x008607cc
extern int16_t light_active_list_count; // 0x008607c8
extern int32_t g_007c1480; // 0x007c1480, rasterizer light queue count
extern rasterizer_light rasterizer_light_queue[0x80]; // 0x007c1484
extern int16_t g_007d0390; // 0x007d0390, visible cluster count
extern uint8_t visible_clusters_base[]; // 0x007c3390, 0x1a0-byte records, cluster index at +0
extern float *default_effect_color_pointer; // 0x00686b04
extern uint8_t render_window_index; // 0x007c310a
extern int16_t current_local_player_index; // 0x007c3108
extern int16_t light_transient_count; // 0x00860b0c
extern lens_flare_instance light_transient_table[]; // 0x008609cc

extern datum_index datum_next(int16_t after_index, data_array *array); // 0x4d0630, DX after_index, EDI array
extern void datum_delete(data_array *array, datum_index handle); // 0x4d0510, EAX array, EDX handle
extern void cluster_reference_remove_all(uint32_t handle, datum_index *link, cluster_reference_group *cluster_list); // 0x552020
extern object *object_try_and_get(datum_index object_index, uint32_t type_mask); // 0x4f6ec0
extern void object_light_recompute_transform(uint32_t light_index); // 0x4f2a00
extern int16_t structure_bsp_collect_visible_objects(datum_index *out_list, int32_t max_count, void *iterate_begin,
    void *iterate_next, void *get_bounds, void *predicate, void *accept); // 0x554420
extern datum_index light_cluster_iterate_begin(datum_index *cursor, int16_t cluster_index); // 0x4f34c0
extern datum_index light_cluster_iterate_next(datum_index *cursor); // 0x4f3500
extern void light_get_render_bounds(datum_index handle, real_point3d *center_out, float *radius_out); // 0x4f3530
extern uint8_t light_not_marked_this_frame(datum_index handle); // 0x4f3620
extern uint8_t light_mark_this_frame(datum_index handle); // 0x4f3650
extern void rasterizer_light_disable_all(void); // 0x526700
extern void structure_cluster_add_lens_flares(int16_t cluster_index); // 0x513a00, CX cluster
extern void *color_interpolate_argb_with_tint(uint32_t flags, void *color1, void *dest, void *tint, void *color0,
    float t); // 0x43f7d0, EDX flags, EBX color1, ESI dest, EDI tint, stack color0, t
extern void *color_interpolate(void *color1, void *color0, void *dest, uint32_t flags, float t); // 0x43f6a0,
    // EAX color1, ECX color0, stack dest, flags, t
extern real transition_function_evaluate(int16_t type, real phase); // 0x4ccac0, CX type, stack phase
extern void first_person_weapon_center_flashlight(datum_index unit_index, real_point3d *out_origin,
    real_vector3d *out_extents, real_vector3d *out_direction); // 0x492b80
extern uint8_t unit_get_first_person_marker_transform(datum_index object_index, const char *marker_name,
    real_point3d *out_position, real_vector3d *out_extents, real_vector3d *out_direction); // 0x492c30
extern void rasterizer_light_set(rasterizer_light *light); // 0x526760, ESI light
extern int32_t local_player_index_for_weapon(datum_index weapon_index); // 0x494010
extern uint32_t first_person_weapon_get_marker_data(datum_index weapon_index, const char *marker_name,
    object_marker *out, uint32_t maximum); // 0x492ad0
extern int32_t object_get_node_local_transform(uint32_t object_index, char *marker_name, object_marker *marker,
    uint32_t maximum); // 0x4f6080
extern uint32_t vector3d_pack_normal_11_11_10(real_vector3d *direction); // 0x5132d0, ESI direction
extern void lens_flare_add_instance(lens_flare_instance *candidate); // 0x5138a0, EBX candidate
extern int32_t fistp_round(float x); // FISTP, round to nearest

static uint8_t *object_data_get(datum_index handle)
{
    return *(uint8_t **)((uint8_t *)object_data->data + (handle & 0xffff) * 0xc + 8);
}

static uint8_t *tag_data(datum_index tag)
{
    return (uint8_t *)tag_instances[tag & 0xffff].data;
}

// the owner's marker name for a light (+0x5c indexes the owner tag's attachment block: +0x140 count, +0x144
// elements of 0x48 with the marker name at +0x10), or NULL
static const char *light_owner_marker_name(uint8_t *light)
{
    uint8_t *owner_tag = tag_data(*(datum_index *)object_data_get(*(datum_index *)(light + 0x2c)));
    int16_t index = *(int16_t *)(light + 0x5c);

    if (index >= 0 && index < *(int32_t *)(owner_tag + 0x140)) {
        return (const char *)(*(uint8_t **)(owner_tag + 0x144) + index * 0x48 + 0x10);
    }
    return 0;
}

void object_lights_update_all(void)
{
    int32_t tick = *(int32_t *)(game_time + 0xc);
    datum_index handle;
    int16_t i;

    // 1. age and re-place every light
    light_transient_count_or_queue = 0;
    for (handle = datum_next(-1, light_data); handle != k_datum_index_none;
         handle = datum_next((int16_t)handle, light_data)) {
        uint8_t *light = (uint8_t *)light_data->data + (handle & 0xffff) * 0x7c;

        light[2] &= ~8;
        *(int32_t *)(light + 8) = -1;
        if (*(int32_t *)(light + 0x58) != -1) {
            float age = (float)(tick - *(int32_t *)(light + 0x58));
            if (!(age <= *(float *)(tag_data(*(datum_index *)(light + 4)) + 0xf4))) {
                cluster_reference_remove_all(handle, (datum_index *)(light + 0x10), &light_cluster_group);
                datum_delete(light_data, handle);
            }
        } else if (object_try_and_get(*(datum_index *)(light + 0x2c), 0xffffffff) != 0) {
            if ((light[2] & 2) != 0) {
                cluster_reference_remove_all(handle, (datum_index *)(light + 0x10), &light_cluster_group);
                light[2] &= ~4;
            }
            object_light_recompute_transform(handle);
        }
    }

    // 2. collect the visible lights; BSP lens flares of the visible clusters
    light_frame_counter++;
    light_render_unknown_7c0 = 1;
    light_active_list_count = structure_bsp_collect_visible_objects(light_active_list, 0x80,
        (void *)light_cluster_iterate_begin, (void *)light_cluster_iterate_next, (void *)light_get_render_bounds,
        (void *)light_not_marked_this_frame, (void *)light_mark_this_frame);
    light_render_unknown_7c0 = 0;
    g_007c1480 = 0;
    rasterizer_light_disable_all();
    for (i = 0; i < g_007d0390; i++) {
        structure_cluster_add_lens_flares(*(int16_t *)(visible_clusters_base + i * 0x1a0));
    }

    // 3 and 4. each visible light
    for (i = 0; i < light_active_list_count; i++) {
        datum_index light_handle = light_active_list[i];
        uint8_t *light = (uint8_t *)light_data->data + (light_handle & 0xffff) * 0x7c;
        uint8_t *tag = tag_data(*(datum_index *)(light + 4));
        datum_index owner_handle = *(datum_index *)(light + 0x2c);
        uint8_t *owner = 0;
        float dim = 1.0f;
        float t;
        float *color = (float *)(light + 0x14);

        if (owner_handle != k_datum_index_none) {
            int16_t index = (int16_t)owner_handle;
            int16_t salt = (int16_t)(owner_handle >> 16);
            if (index >= 0 && index < object_data->maximum_count) {
                uint8_t *header = (uint8_t *)object_data->data + index * object_data->size;
                int16_t identifier = *(int16_t *)header;
                if (identifier != 0 && (salt == 0 || identifier == salt)) {
                    owner = *(uint8_t **)(header + 8);
                }
            }
        }

        if (*(int32_t *)(light + 0x58) == -1) {
            int16_t function_index = *(int16_t *)(light + 0x5e);
            int16_t color_index = *(int16_t *)(light + 0x60);
            void *tint;

            t = function_index == -1 ? 1.0f : *(float *)(object_data_get(owner_handle) + 0x134 + function_index * 4);
            tint = color_index == -1 ? (void *)default_effect_color_pointer : (void *)(owner + 0x1b8 + color_index * 12);
            color_interpolate_argb_with_tint(*(uint32_t *)(tag + 0x34), tag + 0x48, color, tint, tag + 0x38, t);
        } else {
            float phase = (float)(tick - *(int32_t *)(light + 0x58)) / *(float *)(tag + 0xf4);
            t = (1.0f - transition_function_evaluate(*(int16_t *)(tag + 0xfa), phase)) * *(float *)(light + 0x78);
            color_interpolate(tag + 0x4c, tag + 0x3c, color, *(uint32_t *)(tag + 0x34), t);
        }

        if (owner != 0) {
            datum_index root = k_datum_index_none;
            datum_index walk = owner_handle;
            uint8_t *root_object;

            while (walk != k_datum_index_none) {
                root = walk;
                walk = *(datum_index *)(object_data_get(walk) + 0x11c);
            }
            root_object = object_data_get(root);
            if (((1 << root_object[0xb4]) & 3) != 0 && !(*(float *)(root_object + 0x37c) <= 0.0f) &&
                (tag_data(*(datum_index *)(light + 4))[0] & 0x20) == 0) {
                dim = 1.0f - *(float *)(root_object + 0x37c);
                color[0] *= dim;
                color[1] *= dim;
                color[2] *= dim;
            }
        }
        if (color[0] == 0.0f && color[1] == 0.0f && color[2] == 0.0f) {
            continue;
        }

        if ((light[2] & 1) != 0) {
            float intensity = ((1.0f - t) * *(float *)(tag + 8) + t * *(float *)(tag + 0xc)) * *(float *)(tag + 4);

            *(float *)(light + 0x54) = intensity;
            if (intensity != 0.0f) {
                rasterizer_light record;
                int32_t slot = -1;

                record.definition = (uint32_t)tag_data(*(datum_index *)(light + 4));
                record.position = *(real_point3d *)(light + 0x30);
                record.forward = *(real_vector3d *)(light + 0x3c);
                record.up = *(real_vector3d *)(light + 0x48);
                record.color = *(ColorRGB *)color;
                record.radius = intensity;
                if (*(int32_t *)(light + 0x58) == -1) {
                    if ((tag[0] & 0x10) != 0) {
                        first_person_weapon_center_flashlight(owner_handle, &record.position, &record.forward, &record.up);
                        light[2] |= 8;
                    } else if (*(int16_t *)(owner + 0xb4) == 2 && *(datum_index *)(owner + 0x11c) != k_datum_index_none &&
                               unit_get_first_person_marker_transform(owner_handle, light_owner_marker_name(light),
                                                                      &record.position, &record.forward, &record.up)) {
                        light[2] |= 8;
                    }
                }
                if (g_007c1480 < 0x80) {
                    slot = g_007c1480;
                    rasterizer_light_queue[slot] = record;
                    g_007c1480 = slot + 1;
                    rasterizer_light_set(&record);
                }
                *(int32_t *)(light + 8) = slot;
                light_transient_count_or_queue = (int16_t)(slot + 1);
            }
        } else {
            *(float *)(light + 0x54) = *(float *)(tag + 4);
        }

        if (*(datum_index *)(tag + 0xb8) != k_datum_index_none) {
            lens_flare_instance flare;
            int16_t salt = (int16_t)(light_handle >> 16);

            flare.definition = (uint32_t)tag_data(*(datum_index *)(tag + 0xb8));
            flare.color = (uint32_t)(fistp_round(color[2] * 255.0f) & 0xff) |
                          (uint32_t)(fistp_round(color[1] * 255.0f) & 0xff) << 8 |
                          (uint32_t)(fistp_round(color[0] * 255.0f) & 0xff) << 16 |
                          (uint32_t)fistp_round(dim * 255.0f) << 24;
            flare.intensity = (uint8_t)fistp_round(t * 255.0f);
            flare.window_flags = render_window_index;
            flare.visibility_high = (int16_t)light_handle;
            flare.object_index = salt == -1 ? 0 : salt;
            flare.sample_count = 0;
            if (*(int32_t *)(light + 0x58) == -1) {
                object_marker markers[8];
                const char *marker_name = light_owner_marker_name(light);
                int16_t count = 0;
                int16_t j;

                if (*(int16_t *)(owner + 0xb4) == 2 && *(datum_index *)(owner + 0x11c) != k_datum_index_none &&
                    (int16_t)local_player_index_for_weapon(owner_handle) == current_local_player_index) {
                    count = (int16_t)first_person_weapon_get_marker_data(owner_handle, marker_name, markers, 8);
                    if (count > 0) {
                        flare.window_flags |= 0x80;
                    }
                }
                if (count == 0) {
                    count = (int16_t)object_get_node_local_transform(owner_handle, (char *)marker_name, markers, 8);
                }
                for (j = 0; j < count; j++) {
                    flare.position = markers[j].node_transform.position;
                    flare.packed_direction = vector3d_pack_normal_11_11_10(&markers[j].node_transform.forward);
                    flare.packed_up = vector3d_pack_normal_11_11_10(&markers[j].node_transform.up);
                    flare.visibility_low = j;
                    lens_flare_add_instance(&flare);
                }
            } else {
                flare.position = *(real_point3d *)(light + 0x30);
                flare.packed_direction = vector3d_pack_normal_11_11_10((real_vector3d *)(light + 0x3c));
                flare.packed_up = vector3d_pack_normal_11_11_10((real_vector3d *)(light + 0x48));
                flare.visibility_low = 0;
                lens_flare_add_instance(&flare);
            }
        }
    }

    // 5. transient flares
    for (i = 0; i < light_transient_count; i++) {
        lens_flare_add_instance(&light_transient_table[i]);
    }
    light_transient_count = 0;
}

#if 0
Original Ghidra decompilation (0x4f0cf0): see out/functions.json / tools/pack.py 0x4f0cf0 for the
full 1591-byte listing (omitted here for length; every field access and call above was
transcribed directly from it, with the render-queue record copy and several jump-table arguments
left unmodeled as noted in the header comment).
#endif

#if 0
Original Ghidra decompilation (0x4f133c): see out/functions.json / tools/pack.py 0x4f133c for the
full 694-byte listing (omitted here for length). This is the direct continuation of
object_lights_update_all's own per-light loop; every field access and call above was transcribed from
it, with the same render-queue record copy left unmodeled as in object_lights_update_all.c.
#endif
