#include "halo/objects/record_access.hpp"
#include "halo/objects/light_system.hpp"
#include "halo/tags/flags.hpp"
#include "halo/core/flag_bits.hpp"
#include "halo/bitmaps/api.hpp"
#include "game.h"
#include "units.h"
#include "structures.h"
#include "rasterizer.h"
#include "hs.h"
#include <string.h>
#include "halo/math/api.hpp"
#include "halo/memory/api.hpp"
#include "halo/cache/api.hpp"
#include "halo/structures/api.hpp"
#include "halo/physics/api.hpp"
#include "halo/scenario/api.hpp"
#include "halo/saved_games/api.hpp"
#include "halo/rasterizer/api.hpp"
#include "halo/objects/api.hpp"
#include "halo/bitmaps/bitmaps.hpp"
#include "halo/interface/api.hpp"
#include "halo/game/api.hpp"
#include "halo/core/link.hpp"
#include "halo/effects/vars.hpp"
#include "halo/game/vars.hpp"
#include "halo/objects/vars.hpp"
#include "halo/render/vars.hpp"
#include "halo/core/libm.hpp"
#include "halo/core/x87.hpp"
#include "halo/effects/api.hpp"
#include "halo/render/api.hpp"

static auto &game_engine_unknown_aa00 = halo::link::ref<uint8_t>(halo::game::vars().game_engine_unknown_aa00);
static auto &global_white_color = halo::link::ref<float *>(halo::effects::vars().global_white_color);
static auto &light_active_list = halo::link::ref<datum_index [0x80]>(halo::objects::vars().light_active_list);
static auto &light_active_list_count = halo::link::ref<int16_t>(halo::objects::vars().light_active_list_count);
static auto &light_cluster_first = halo::link::ref<datum_index *>(halo::objects::vars().light_cluster_first);
static auto &light_cluster_references = halo::link::ref<data_array *>(halo::objects::vars().light_cluster_references);
static auto &light_count_enabled = halo::link::ref<int16_t>(halo::effects::vars().light_count_enabled);
static auto &light_data = halo::link::ref<data_array *>(halo::objects::vars().light_data);
static auto &light_frame_counter = halo::link::ref<int32_t>(halo::objects::vars().light_frame_counter);
static auto &light_object_references = halo::link::ref<data_array *>(halo::objects::vars().light_object_references);
static auto &light_render_unknown_7c0 = halo::link::ref<uint8_t>(halo::objects::vars().light_render_unknown_7c0);
static auto &light_transient_count = halo::link::ref<int16_t>(halo::objects::vars().light_transient_count);
static auto &light_transient_count_or_queue = halo::link::ref<int16_t>(halo::objects::vars().light_transient_count_or_queue);
static auto &light_transient_table = halo::link::ref<light_transient [k_maximum_transient_lights]>(halo::objects::vars().light_transient_table);
static auto &lights_enabled = halo::link::ref<uint8_t *>(halo::objects::vars().lights_enabled);
static auto &object_data = halo::link::ref<data_array *>(halo::objects::vars().object_data);
static auto &rasterizer_light_count = halo::link::ref<int32_t>(halo::objects::vars().rasterizer_light_count);
static auto &rasterizer_lights = halo::link::ref<rasterizer_light [0x80]>(halo::objects::vars().rasterizer_lights);
static auto &render_window_index = halo::link::ref<uint8_t>(halo::render::vars().render_window_index);

namespace {
static cluster_reference_group &light_cluster_first__as_lights_initialize = reinterpret_cast<cluster_reference_group &>(light_cluster_first);
}

/**
 * Creates the light data array, the cluster tables and the light reference arrays.
 *
 * Original register convention: none (no parameters).
 *
 * @address 0x004f0a20
 */
void halo::objects::LightSystem::initialize()
{
    data_array *new_light_data = halo::saved_games::game_state_new((char *)"lights", k_maximum_lights, 0x7c  );
    uint8_t *checksum_slot = halo::saved_games::globals().game_state_base + halo::saved_games::globals().game_state_cursor;
    uint32_t size_marker = 4;

    halo::saved_games::globals().game_state_cursor = halo::saved_games::globals().game_state_cursor + 4;
    light_data = new_light_data;
    halo::memory::crc32_update(&halo::saved_games::globals().game_state_crc, (uint8_t *)&size_marker, 4);
    lights_enabled = checksum_slot;
    *checksum_slot = 1;

    if (new_light_data != 0) {
        halo::structures::cluster_partition_new(&light_cluster_first__as_lights_initialize, (char *)"light");
    }
}

/**
 * Disposes the light data array and the per-cluster light tables.
 *
 * Original register convention: none (no parameters).
 *
 * @address 0x004f0aa0
 */
void halo::objects::LightSystem::dispose_all()
{
    int32_t i;

    light_data->valid = 1;
    halo::memory::data_delete_all(light_data);

    *lights_enabled = 1;

    for (i = 0; i < k_maximum_clusters; i++) {
        light_cluster_first[i] = k_datum_index_none;
    }

    light_object_references->valid = 1;
    halo::memory::data_delete_all(light_object_references);

    light_cluster_references->valid = 1;
    halo::memory::data_delete_all(light_cluster_references);
}

/**
 * Creates a light attached to an object marker and returns its handle.
 *
 * Original register convention: datum_index light_tag in EAX (param_1); datum_index owner_object on the.
 *
 * @address 0x004f0af0
 */
datum_index halo::objects::LightSystem::new_attached(datum_index light_tag, datum_index owner_object,
    int16_t marker_index, int16_t marker_index_secondary, int16_t change_color_index)
{
    Light *tag = (Light *)halo::cache::globals().tag_instances[halo::datum_slot(light_tag)].data;
    datum_index handle = k_datum_index_none;

    if (test_flag(tag->flags, tags::light_tag_flag::dynamic) || (int32_t)halo::objects::tag_handle(((struct Light *)tag)->lens_flare) != -1) {
        handle = halo::memory::datum_new(light_data);

        if (handle != k_datum_index_none) {
            light *entry = &((light *)light_data->data)[halo::datum_slot(handle)];
            uint8_t always_visible = (uint8_t)(tag->flags & 1);

            entry->owner_object = owner_object;
            entry->marker_index = marker_index;
            entry->flags = 0;
            entry->marker_index_secondary = marker_index_secondary;
            entry->definition_tag = light_tag;

            *(int16_t *)&((struct light *)entry)->local_position.x = change_color_index;

            if (always_visible == 0 && (int32_t)halo::objects::tag_handle(((struct Light *)tag)->lens_flare) == -1) {
                entry->flags = 0;
            } else {
                entry->flags = always_visible | _light_attached_bit;
            }

            entry->next_light = k_datum_index_none;
            entry->marker_link = -1;
            halo::objects::object_light_recompute_transform(handle);
            entry->creation_tick = light_frame_counter - 1;
        }
    }

    return handle;
}

/**
 * Removes a light from its cluster lists and frees its datum.
 *
 * Original register convention: light handle in ESI.
 *
 * @address 0x004f0bd0
 */
void halo::objects::LightSystem::destroy(datum_index light_handle)
{
    light *entry = (light *)((uint8_t *)light_data->data + halo::datum_slot(light_handle) * 0x7c);

    halo::structures::cluster_reference_remove_all(light_handle, &entry->next_light, (cluster_reference_group *)&light_cluster_first);
    halo::memory::datum_delete(light_data, light_handle);
}

/**
 * Creates a light at a fixed position and direction and returns its handle.
 *
 * Original register convention: datum_index light_tag on the stack (param_1); int32_t marker_index (-1.
 *
 * @address 0x004f0c10
 */
datum_index halo::objects::LightSystem::new_positioned(datum_index light_tag, int32_t marker_index,
    int16_t marker_sub_index, real_point3d *position, uint32_t param_5, real_vector3d *direction)
{
    datum_index handle = halo::memory::datum_new(light_data);

    if (handle != k_datum_index_none) {
        light *entry = &((light *)light_data->data)[halo::datum_slot(handle)];
        uint8_t *raw = (uint8_t *)entry;

        entry->flags = 0;
        entry->marker_link = halo::game::globals().game_time->game_time;
        entry->definition_tag = light_tag;
        *(int32_t *)&((struct light *)raw)->owner_object = marker_index;
        entry->transient_color_scale = param_5;
        entry->flags = 3;

        entry->next_light = k_datum_index_none;

        if (marker_index == -1) {
            *(real_point3d *)&((struct light *)raw)->position.x = *position;
            *(real_vector3d *)&((struct light *)raw)->direction.i = *direction;
        } else {
            ((struct light *)raw)->marker_index = marker_sub_index;
            *(real_point3d *)&((struct light *)raw)->local_position.x = *position;
            *(real_vector3d *)&((struct light *)raw)->local_direction.i = *direction;
        }

        halo::objects::object_light_recompute_transform(handle);
        entry->creation_tick = light_frame_counter - 1;
    }

    return handle;
}

namespace {
static cluster_reference_group &light_cluster_first__as_object_lights_update_all = reinterpret_cast<cluster_reference_group &>(light_cluster_first);
static lens_flare_instance *const light_transient_table__as_object_lights_update_all = reinterpret_cast<lens_flare_instance *>(light_transient_table);
static void (*const cluster_reference_remove_all__as_object_lights_update_all)(uint32_t handle, datum_index *link, cluster_reference_group *cluster_list) = reinterpret_cast<void (*)(uint32_t handle, datum_index *link, cluster_reference_group *cluster_list)>(&halo::structures::cluster_reference_remove_all);
static uint8_t *object_data_get(datum_index handle)
{
    return *(uint8_t **)((uint8_t *)object_data->data + halo::datum_slot(handle) * 0xc + 8);
}

static uint8_t *tag_data(datum_index tag)
{
    return halo::objects::tag_record_bytes(tag);
}

static const char *light_owner_marker_name(uint8_t *light)
{
    uint8_t *owner_tag = tag_data(*(datum_index *)object_data_get(((struct light *)light)->owner_object));
    int16_t index = ((struct light *)light)->marker_index;

    if (index >= 0 && index < *(int32_t *)(owner_tag + 0x140)) {
        return (const char *)(*(uint8_t **)(owner_tag + 0x144) + index * 0x48 + 0x10);
    }
    return 0;
}
}

/**
 * Per-frame update of all lights: lifetimes, owner transforms and cluster membership.
 *
 * Original register convention: none (no parameters).
 *
 * @address 0x004f0cf0
 */
void halo::objects::LightSystem::update_all()
{
    int32_t tick = halo::game::globals().game_time->game_time;
    datum_index handle;
    int16_t i;

    light_transient_count_or_queue = 0;
    for (handle = halo::memory::datum_next(-1, light_data); handle != k_datum_index_none;
         handle = halo::memory::datum_next((int16_t)handle, light_data)) {
        uint8_t *light = (uint8_t *)light_data->data + halo::datum_slot(handle) * 0x7c;

        light[2] &= ~8;
        *(int32_t *)&((struct rasterizer_light *)light)->position.y = -1;

        if (((struct light *)light)->marker_link != -1) {
            float age = (float)(tick - ((struct light *)light)->marker_link);
            if (!(age <= *(float *)(tag_data(((struct light *)light)->definition_tag) + 0xf4))) {
                cluster_reference_remove_all__as_object_lights_update_all(handle, (datum_index *)(light + 0x10), &light_cluster_first__as_object_lights_update_all);
                halo::memory::datum_delete(light_data, handle);
            } else if (halo::objects::object_try_and_get(((struct light *)light)->owner_object, k_datum_index_none) != 0) {
                light = (uint8_t *)light_data->data + halo::datum_slot(handle) * 0x7c;
                if ((light[2] & 2) != 0) {
                    cluster_reference_remove_all__as_object_lights_update_all(handle, (datum_index *)(light + 0x10), &light_cluster_first__as_object_lights_update_all);
                    light[2] &= ~4;
                }
                halo::objects::object_light_recompute_transform(handle);
            }
        }
    }

    light_frame_counter++;
    light_render_unknown_7c0 = 1;
    light_active_list_count = halo::structures::structure_bsp_collect_visible_objects((int32_t *)light_active_list, 0x80,
        (structure_bsp_object_iterate_begin_fn)halo::objects::light_cluster_iterate_begin, (structure_bsp_object_iterate_next_fn)halo::objects::light_cluster_iterate_next, (structure_bsp_object_get_bounds_fn)halo::objects::light_get_render_bounds,
        (structure_bsp_object_predicate_fn)halo::objects::light_not_marked_this_frame, (structure_bsp_object_accept_fn)halo::objects::light_mark_this_frame);
    light_render_unknown_7c0 = 0;
    rasterizer_light_count = 0;
    halo::rasterizer::rasterizer_light_disable_all();
    for (i = 0; i < halo::structures::globals().visible_cluster_count; i++) {
        halo::rasterizer::structure_cluster_add_lens_flares(*(int16_t *)(halo::structures::globals().visible_clusters + i * 0x1a0));
    }

    for (i = 0; i < light_active_list_count; i++) {
        datum_index light_handle = light_active_list[i];
        uint8_t *light = (uint8_t *)light_data->data + halo::datum_slot(light_handle) * 0x7c;
        uint8_t *tag = tag_data(((struct light *)light)->definition_tag);
        datum_index owner_handle = ((struct light *)light)->owner_object;
        uint8_t *owner = 0;
        float dim = 1.0f;
        float t;
        float blend;

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

        if (((struct light *)light)->marker_link == -1) {
            int16_t function_index = ((struct light *)light)->marker_index_secondary;
            int16_t color_index = *(int16_t *)&((struct light *)light)->local_position.x;
            void *tint;

            t = function_index == -1 ? 1.0f : *(float *)(object_data_get(owner_handle) + 0x134 + function_index * 4);
            tint = color_index == -1 ? (void *)global_white_color : (void *)(owner + 0x1b8 + color_index * 12);
            halo::bitmaps::color_codec::interpolate_argb_with_tint(static_cast<color_interpolation_flags>(*(uint32_t *)(tag + 0x34)), reinterpret_cast<ColorARGB *>(tag + 0x48), reinterpret_cast<ColorRGB *>(color), reinterpret_cast<ColorRGB *>(tint), reinterpret_cast<ColorARGB *>(tag + 0x38), t);
            blend = t;
        } else {
            int32_t age_ticks = tick - ((struct light *)light)->marker_link;
            float phase = (float)age_ticks / *(float *)(tag + 0xf4);
            t = (1.0f - halo::math::transition_function_evaluate(*(int16_t *)(tag + 0xfa), phase)) * *(float *)&((struct light *)light)->transient_color_scale;
            halo::bitmaps::color_interpolate(reinterpret_cast<ColorRGB *>(tag + 0x4c), reinterpret_cast<ColorRGB *>(tag + 0x3c), reinterpret_cast<ColorRGB *>(color), static_cast<color_interpolation_flags>(*(uint32_t *)(tag + 0x34)), t);

            memcpy(&blend, &age_ticks, sizeof(blend));
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
                (tag_data(((struct light *)light)->definition_tag)[0] & 0x20) == 0) {
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
            float intensity = ((1.0f - blend) * *(float *)(tag + 8) + blend * *(float *)(tag + 0xc)) * *(float *)(tag + 4);

            ((struct light *)light)->radius = intensity;
            if (intensity != 0.0f) {
                rasterizer_light record;
                int32_t slot = -1;

                record.definition = (uint32_t)tag_data(((struct light *)light)->definition_tag);
                record.position = *(real_point3d *)&((struct light *)light)->position.x;
                record.forward = *(real_vector3d *)&((struct light *)light)->direction.i;
                record.up = *(real_vector3d *)&((struct light *)light)->up.i;
                record.color = *(ColorRGB *)color;
                record.radius = intensity;
                if (((struct light *)light)->marker_link == -1) {
                    if ((tag[0] & 0x10) != 0) {
                        halo::interface::first_person_weapon_center_flashlight(owner_handle, &record.position, &record.forward, &record.up);
                        light[2] |= 8;
                    } else if (*(int16_t *)(owner + 0xb4) == 2 && *(datum_index *)(owner + 0x11c) != k_datum_index_none &&
                               halo::interface::unit_get_first_person_marker_transform(owner_handle, light_owner_marker_name(light),
                                                                      &record.position, &record.forward, &record.up)) {
                        light[2] |= 8;
                    }
                }
                if (rasterizer_light_count < 0x80) {
                    slot = rasterizer_light_count;
                    rasterizer_lights[slot] = record;
                    rasterizer_light_count = slot + 1;
                    halo::rasterizer::rasterizer_light_set(&record);
                }
                *(int32_t *)&((struct rasterizer_light *)light)->position.y = slot;
                light_transient_count_or_queue = (int16_t)(slot + 1);
            }
        } else {
            ((struct light *)light)->radius = *(float *)(tag + 4);
        }

        if (*(datum_index *)(tag + 0xb8) != k_datum_index_none) {
            lens_flare_instance flare;
            int16_t salt = (int16_t)(light_handle >> 16);

            flare.definition = (uint32_t)tag_data(*(datum_index *)(tag + 0xb8));
            flare.color = (uint32_t)(halo::x87::fistp_round(color[2] * 255.0f) & 0xff) |
                          (uint32_t)(halo::x87::fistp_round(color[1] * 255.0f) & 0xff) << 8 |
                          (uint32_t)(halo::x87::fistp_round(color[0] * 255.0f) & 0xff) << 16 |
                          (uint32_t)halo::x87::fistp_round(dim * 255.0f) << 24;
            flare.intensity = (uint8_t)halo::x87::fistp_round(blend * 255.0f);
            flare.window_flags = render_window_index;
            flare.visibility_high = (int16_t)light_handle;
            flare.object_index = salt == -1 ? 0 : salt;
            flare.sample_count = 0;
            if (((struct light *)light)->marker_link == -1) {
                object_marker markers[8];
                const char *marker_name = light_owner_marker_name(light);
                int16_t count = 0;
                int16_t j;

                if (*(int16_t *)(owner + 0xb4) == 2 && *(datum_index *)(owner + 0x11c) != k_datum_index_none &&
                    (int16_t)halo::interface::local_player_index_for_weapon(owner_handle) == halo::interface::globals().current_local_player_index) {
                    count = (int16_t)halo::interface::first_person_weapon_get_marker_data(owner_handle, marker_name, markers, 8);
                    if (count > 0) {
                        flare.window_flags |= 0x80;
                    }
                }
                if (count == 0) {
                    count = (int16_t)halo::objects::object_get_node_local_transform(owner_handle, (char *)marker_name, markers, 8);
                }
                for (j = 0; j < count; j++) {
                    flare.position = markers[j].node_transform.position;
                    flare.packed_direction = halo::rasterizer::vector3d_pack_normal_11_11_10(&markers[j].node_transform.forward);
                    flare.packed_up = halo::rasterizer::vector3d_pack_normal_11_11_10(&markers[j].node_transform.up);
                    flare.visibility_low = j;
                    halo::rasterizer::lens_flare_add_instance(&flare);
                }
            } else {
                flare.position = *(real_point3d *)&((struct light *)light)->position.x;
                flare.packed_direction = halo::rasterizer::vector3d_pack_normal_11_11_10((real_vector3d *)(light + 0x3c));
                flare.packed_up = halo::rasterizer::vector3d_pack_normal_11_11_10((real_vector3d *)(light + 0x48));
                flare.visibility_low = 0;
                halo::rasterizer::lens_flare_add_instance(&flare);
            }
        }
    }

    for (i = 0; i < light_transient_count; i++) {
        halo::rasterizer::lens_flare_add_instance(&light_transient_table__as_object_lights_update_all[i]);
    }
    light_transient_count = 0;
}

/**
 * Adds a one-frame light with the given colour, position and intensity to the transient light table; refuses when it
 * is full.
 *
 * Original register convention: datum_index light_tag in EAX (in_EAX); real_vector3d *color in EDX.
 *
 * @address 0x004f1600
 */
void halo::objects::LightSystem::transient_add(datum_index light_tag, real_vector3d *color, real_point3d *position,
    uint32_t direction, uint32_t param_3, float intensity)
{
    if (light_transient_count < k_maximum_transient_lights &&
        (color->i != 0.0f || color->j != 0.0f || color->k != 0.0f)) {
        light_transient *slot = &light_transient_table[light_transient_count];

        slot->color = halo::math::color_real_to_argb_pack(1.0f, reinterpret_cast<float *>(color));
        slot->intensity = (uint8_t)halo::x87::fistp_round(intensity * 255.0f);
        slot->definition = halo::cache::globals().tag_instances[halo::datum_slot(light_tag)].data;
        slot->position = *position;

        slot->packed_forward = halo::rasterizer::vector3d_pack_normal_11_11_10((real_vector3d *)direction);
        slot->packed_up = halo::rasterizer::vector3d_pack_normal_11_11_10((real_vector3d *)param_3);
        slot->render_window_index = render_window_index;
        slot->unknown_1e = -1;
        slot->unknown_1c = -1;
        slot->slot_index = light_transient_count;

        light_transient_count = light_transient_count + 1;
    }
}

/**
 * Copies the handles of the objects a light references into out_buffer, up to max_count, and returns the count.
 *
 * Original register convention: uint32_t light_handle in ECX (in_ECX); int16_t max_count in SI.
 *
 * @address 0x004f1700
 */
int16_t halo::objects::LightSystem::collect_object_references(uint32_t light_handle, int16_t max_count,
    int16_t *out_buffer)
{
    light *l = &((light *)light_data->data)[halo::datum_slot(light_handle)];
    uint32_t node = (uint32_t)l->next_light;
    uint32_t next_node;
    int16_t count = 0;

    if (node == k_datum_index_none) {
        next_node = k_datum_index_none;
        node = k_datum_index_none;
    } else {
        uint8_t *ref = (uint8_t *)light_object_references->data + halo::datum_slot(node) * 0xc;
        next_node = *(uint32_t *)(ref + 8);
        node = *(uint32_t *)(ref + 4);
    }

    if (0 < max_count) {
        do {
            if ((int16_t)node == -1) {
                return count;
            }
            out_buffer[count] = (int16_t)node;
            count = count + 1;

            if (next_node == k_datum_index_none) {
                node = k_datum_index_none;
            } else {
                uint8_t *ref = (uint8_t *)light_object_references->data + halo::datum_slot(next_node) * 0xc;
                next_node = *(uint32_t *)(ref + 8);
                node = *(uint32_t *)(ref + 4);
            }
        } while (count < max_count);
    }
    return count;
}

/**
 * Applies spot cone falloff to the active light list.
 *
 * Original register convention: none (no parameters).
 *
 * @address 0x004f1780
 */
void halo::objects::LightSystem::apply_spot_falloff()
{
    int16_t references[0x200];

    halo::rasterizer::rasterizer_light_cone_set_texture_stage_states();

    if (*lights_enabled != 0 &&
        (halo::game::globals().current_engine == 0 || ((game_engine_unknown_aa00 & 1) == 0 && 1 < light_count_enabled))) {
        int16_t i;

        for (i = 0; i < light_active_list_count; i++) {
            light *l = &((light *)light_data->data)[light_active_list[i] & 0xffff];

            if ((l->flags & _light_always_visible_bit) != 0) {
                int32_t queue_slot = *(int32_t *)&((struct light *)l)->queue_slot;

                if (queue_slot != -1) {
                    Light *tag = (Light *)halo::cache::globals().tag_instances[halo::datum_slot(l->definition_tag)].data;
                    int8_t is_cone = (l->flags & _light_needs_cone_update_bit) != 0 &&
                        test_flag(tag->flags, tags::light_tag_flag::supersize_in_first_person);
                    int16_t marker_count = 0;
                    float radius = l->radius;
                    real_point3d position;

                    if (!is_cone) {
                        marker_count = halo::objects::light_collect_object_references(light_active_list[i], 0x200, references);
                    }

                    if (1.5707964f <= tag->cutoff_angle) {
                        position = l->position;
                    } else if (0.7853982f <= tag->cutoff_angle) {
                        float scale = radius * tag->cos_cutoff_angle;

                        position.x = scale * l->direction.i + l->position.x;
                        position.y = scale * l->direction.j + l->position.y;
                        position.z = scale * l->direction.k + l->position.z;
                        radius = radius * tag->sin_cutoff_angle;
                    } else {
                        radius = radius / tag->cos_cutoff_angle;
                        position.x = radius * l->direction.i + l->position.x;
                        position.y = radius * l->direction.j + l->position.y;
                        position.z = radius * l->direction.k + l->position.z;
                    }

                    halo::structures::structure_debug_draw_surfaces_in_box_alt((void *)queue_slot, &position, radius, marker_count,
                        is_cone ? (int16_t *)0 : references);

                }
            }
        }
    }
}

/**
 * Applies spot cone falloff to the active light list for the specular pass.
 *
 * Original register convention: none (no parameters).
 *
 * @address 0x004f1950
 */
void halo::objects::LightSystem::apply_spot_falloff_specular()
{
    int16_t references[0x200];

    halo::rasterizer::rasterizer_shader_environment_technique_ps2_set_states();

    if (*lights_enabled != 0 &&
        (halo::game::globals().current_engine == 0 || ((game_engine_unknown_aa00 & 1) == 0 && 1 < light_count_enabled))) {
        int16_t i;

        for (i = 0; i < light_active_list_count; i++) {
            light *l = &((light *)light_data->data)[light_active_list[i] & 0xffff];

            if ((l->flags & _light_always_visible_bit) != 0) {
                int32_t queue_slot = *(int32_t *)&((struct light *)l)->queue_slot;

                if (queue_slot != -1) {
                    Light *tag = (Light *)halo::cache::globals().tag_instances[halo::datum_slot(l->definition_tag)].data;

                    if (!test_flag(tag->flags, tags::light_tag_flag::no_specular)) {
                        int8_t is_cone = (l->flags & _light_needs_cone_update_bit) != 0 &&
                            test_flag(tag->flags, tags::light_tag_flag::supersize_in_first_person);
                        int16_t marker_count = 0;
                        float radius = l->radius;
                        real_point3d position;

                        if (!is_cone) {
                            marker_count = halo::objects::light_collect_object_references(light_active_list[i], 0x200, references);
                        }

                        if (!test_flag(tag->flags, tags::light_tag_flag::no_specular)) {
                            radius = radius * tag->specular_radius_multiplier;
                        }

                        if (1.5707964f <= tag->cutoff_angle) {
                            position = l->position;
                        } else if (0.7853982f <= tag->cutoff_angle) {
                            float scale = radius * tag->cos_cutoff_angle;

                            position.x = scale * l->direction.i + l->position.x;
                            position.y = scale * l->direction.j + l->position.y;
                            position.z = scale * l->direction.k + l->position.z;
                            radius = radius * tag->sin_cutoff_angle;
                        } else {
                            radius = radius / tag->cos_cutoff_angle;
                            position.x = radius * l->direction.i + l->position.x;
                            position.y = radius * l->direction.j + l->position.y;
                            position.z = radius * l->direction.k + l->position.z;
                        }

                        halo::structures::structure_debug_draw_surfaces_in_box((void *)queue_slot, &position, radius, marker_count,
                            is_cone ? (int16_t *)0 : references);

                    }
                }
            }
        }
    }
}

/**
 * Clears the dirty flag of an attached light.
 *
 * Original register convention: light index in EAX (in_EAX).
 *
 * @address 0x004f29c0
 */
void halo::objects::LightSystem::clear_dirty_flag(uint32_t light_index)
{
    light *entry = (light *)light_data->data + halo::datum_slot(light_index);

    if ((entry->flags & _light_attached_bit) != 0) {
        halo::structures::cluster_reference_remove_all(light_index, &entry->next_light, (cluster_reference_group *)&light_cluster_first);

        entry->flags &= (uint16_t)~_light_transform_dirty_bit;
    }
}

/**
 * Recomputes the world transform of an attached light from its owner's marker.
 *
 * Original register convention: light index as the single stack argument ([esp+0x8c] after the prologue's sub
 * esp,0x84 and three pushes).
 *
 * @address 0x004f2a00
 */
void halo::objects::LightSystem::recompute_transform(uint32_t light_index)
{
    light *entry = (light *)((uint8_t *)light_data->data + halo::datum_slot(light_index) * 0x7c);
    datum_index owner_object = entry->owner_object;

    if (entry->marker_link == -1) {
        object_marker marker;
        char *marker_name = halo::objects::object_get_attachment_marker_name(owner_object, entry->marker_index);

        halo::objects::object_get_node_local_transform(owner_object, marker_name, &marker, 1);

        entry->position = marker.node_transform.position;
        entry->direction = marker.node_transform.forward;
        *(real_vector3d *)&((struct light *)entry)->up.i = marker.node_transform.up;
    } else if (halo::objects::object_try_and_get(owner_object, _object_mask_all) != 0) {
        object *owner = ((object_header *)object_data->data)[halo::datum_slot(owner_object)].data;
        uint8_t *node = (uint8_t *)owner + owner->nodes.offset + entry->marker_index * 0x34;

        real_point3d *local_position = (real_point3d *)&entry->local_position;
        real_vector3d *local_direction = (real_vector3d *)&entry->local_direction;
        real_vector3d *up = (real_vector3d *)&entry->up;
        real_vector3d *world_direction;

        halo::math::matrix4x3_transform_point(entry->position, *local_position, *(real_matrix4x3 *)node);
        halo::math::matrix4x3_transform_normal(entry->direction, *local_direction, *(real_matrix4x3 *)node);
        world_direction = &entry->direction;
        halo::math::vector3d_build_perpendicular(*up, *world_direction);
        halo::math::vector3d_normalize_with_length(*up);
    }

    if ((entry->flags & _light_attached_bit) != 0) {
        uint8_t *light_tag = halo::objects::tag_record_bytes(entry->definition_tag);
        float attenuation = ((struct Light *)light_tag)->radius_modifer[1] * *(float *)(light_tag + 4);
        bsp_leaf_reference leaf_reference;
        real_point3d position;
        float radius;

        if ((*light_tag & 2) == 0) {
            attenuation = attenuation * ((struct Light *)light_tag)->specular_radius_multiplier;
        }

        if (((struct Light *)light_tag)->lens_flare_only_radius <= attenuation) {
            if (((struct Light *)light_tag)->cutoff_angle >= 1.5707964f) {
                position = entry->position;
            } else if (((struct Light *)light_tag)->cutoff_angle >= 0.7853982f) {
                float offset = attenuation * ((struct Light *)light_tag)->cos_cutoff_angle;
                position.x = offset * entry->direction.i + entry->position.x;
                position.y = offset * entry->direction.j + entry->position.y;
                position.z = offset * entry->direction.k + entry->position.z;
                attenuation = attenuation * ((struct Light *)light_tag)->sin_cutoff_angle;
            } else {
                attenuation = attenuation / ((struct Light *)light_tag)->cos_cutoff_angle;
                position.x = attenuation * entry->direction.i + entry->position.x;
                position.y = attenuation * entry->direction.j + entry->position.y;
                position.z = attenuation * entry->direction.k + entry->position.z;
            }
        } else {
            position = entry->position;
            attenuation = ((struct Light *)light_tag)->lens_flare_only_radius;
        }
        radius = attenuation;

        if (entry->owner_object == k_datum_index_none ||
            halo::objects::object_try_and_get(entry->owner_object, _object_mask_all) == 0) {
            leaf_reference.leaf_index = halo::physics::bsp3d_node_find_leaf(0, (ModelCollisionGeometryBSP *)halo::physics::globals().collision_bsp, &position);
            if (leaf_reference.leaf_index == -1) {
                leaf_reference.cluster_index = -1;
            } else {
                uint8_t *leaves = (uint8_t *)halo::scenario::globals().structure_bsp->leaves.pointer;
                leaf_reference.cluster_index =
                    *(int16_t *)(leaves + (leaf_reference.leaf_index & halo::k_leaf_index_mask) * 0x10 + 8);
            }
        } else {

            halo::objects::object_get_root_location((int32_t *)&leaf_reference, entry->owner_object);
        }

        halo::structures::cluster_reference_add_within_radius(light_index, &entry->next_light, &position, radius, &leaf_reference,
                     (cluster_reference_group *)&light_cluster_first);
        entry->flags |= _light_transform_dirty_bit;
    }
}

namespace {
static cluster_reference_group &light_cluster_first__as_object_lights_detach_from_structure_bsp = reinterpret_cast<cluster_reference_group &>(light_cluster_first);
static void (*const cluster_reference_remove_all__as_object_lights_detach_from_structure_bsp)(uint32_t handle, datum_index *link, cluster_reference_group *cluster_list) = reinterpret_cast<void (*)(uint32_t handle, datum_index *link, cluster_reference_group *cluster_list)>(&halo::structures::cluster_reference_remove_all);
}

/**
 * Removes every light from the structure BSP cluster lists.
 *
 * Original register convention: no arguments.
 *
 * @address 0x004f2cb0
 */
void halo::objects::LightSystem::detach_from_structure_bsp()
{
    datum_index handle;

    for (handle = halo::memory::datum_next(-1, light_data); handle != k_datum_index_none;
         handle = halo::memory::datum_next((int16_t)handle, light_data)) {
        uint8_t *light = (uint8_t *)light_data->data + halo::datum_slot(handle) * 0x7c;

        if (light[2] & 4) {
            if (light[2] & 2) {
                cluster_reference_remove_all__as_object_lights_detach_from_structure_bsp(handle, (datum_index *)(light + 0x10), &light_cluster_first__as_object_lights_detach_from_structure_bsp);
                light[2] &= 0xfb;
            }
            light[2] |= 4;
        }
    }
}

/**
 * Refreshes the transforms of every attached light.
 *
 * Original register convention: none (void).
 *
 * @address 0x004f2d50
 */
void halo::objects::LightSystem::refresh_transforms()
{
    datum_index index = halo::memory::datum_next(-1, light_data);

    while (index != k_datum_index_none) {
        light *entry = (light *)light_data->data + halo::datum_slot(index);
        if ((entry->flags & _light_transform_dirty_bit) != 0) {
            entry->flags &= (uint16_t)~_light_transform_dirty_bit;
            halo::objects::object_light_recompute_transform(index);
        }
        index = halo::memory::datum_next((int16_t)index, light_data);
    }
}

/**
 * Gathers the nearest lights to a probe point in a cluster with their intensities and falloffs.
 *
 * Original register convention: cluster/hash index in AX (in_AX), then the eight stack parameters shown by Ghidra
 * unchanged.
 *
 * @address 0x004f2df0
 */
void halo::objects::LightSystem::gather_nearest(int16_t cluster_index, uint32_t self_object_index,
    real_point3d *probe_point, float search_margin, uint32_t *out_indices, float *out_intensities,
    uint32_t out_falloffs, int16_t *count, int16_t max_count)
{
    datum_index next_ref = light_cluster_first[cluster_index];
    uint32_t chain_index;

    if (next_ref == k_datum_index_none) {
        chain_index = k_datum_index_none;
    } else {
        object_cluster_reference *ref =
            (object_cluster_reference *)light_cluster_references->data + halo::datum_slot(next_ref);
        chain_index = ref->object_index;
        next_ref = ref->next_reference;
    }

    while (chain_index != k_datum_index_none) {
        light *entry = (light *)light_data->data + halo::datum_slot(chain_index);

        if (entry->creation_tick != light_frame_counter) {
            if (*(int32_t *)&((struct light *)entry)->queue_slot != -1) {
                int eligible;
                if (entry->owner_object != self_object_index) {
                    eligible = 1;
                } else {

                    uint8_t *light_tag = halo::objects::tag_record_bytes(*(uint32_t *)&((struct light *)entry)->definition_tag);
                    eligible = (light_tag[0] & 4) == 0;
                }

                if (eligible) {
                    float dx = probe_point->x - entry->position.x;
                    float dy = probe_point->y - entry->position.y;
                    float dz = probe_point->z - entry->position.z;
                    float distance = (float)halo::libm::sqrt((double)(dz * dz + dx * dx + dy * dy));
                    if (distance < search_margin + entry->radius) {
                        int16_t used = *count;
                        float attenuation = 1.0f - (distance * distance) / (entry->radius * entry->radius);
                        float luminance = (*(float *)((uint8_t *)entry + 0x1c) * 0.114f +
                                           *(float *)((uint8_t *)entry + 0x18) * 0.587f +
                                           *(float *)((uint8_t *)entry + 0x14) * 0.299f) * attenuation;
                        int16_t slot;

                        if (used < max_count) {
                            *count = used + 1;
                            slot = used;
                        } else {

                            float weakest = 3.4028235e+38f;
                            int16_t weakest_slot = -1;
                            slot = 0;
                            if (used > 0) {
                                int16_t j;
                                for (j = 0; j < *count; j++) {
                                    if (out_intensities[j] < weakest) {
                                        weakest = out_intensities[j];
                                        weakest_slot = j;
                                    }
                                }
                                slot = j;
                            }
                            if (weakest < luminance) {
                                slot = weakest_slot;
                            }
                        }

                        if (slot < max_count) {
                            out_indices[slot] = chain_index;
                            out_intensities[slot] = luminance;
                            *(float *)((uint8_t *)out_falloffs + slot * 4) = attenuation;
                        }
                    }
                }
            }
            if (entry->creation_tick != light_frame_counter) {
                entry->creation_tick = light_frame_counter;
            }
        }

        if (next_ref == k_datum_index_none) {
            chain_index = k_datum_index_none;
        } else {
            object_cluster_reference *ref =
                (object_cluster_reference *)light_cluster_references->data + halo::datum_slot(next_ref);
            chain_index = ref->object_index;
            next_ref = ref->next_reference;
        }
    }
}

/**
 * Starts iterating the lights registered in a BSP cluster and returns the first light handle.
 *
 * Original register convention: stack -> cursor, cluster_index (cdecl).
 *
 * @address 0x004f34c0
 */
datum_index halo::objects::LightSystem::cluster_iterate_begin(datum_index *cursor, int16_t cluster_index)
{
    datum_index reference = light_cluster_first[cluster_index];
    uint8_t *element;

    *cursor = reference;
    if (reference == k_datum_index_none) {
        return k_datum_index_none;
    }
    element = (uint8_t *)light_cluster_references->data + halo::datum_slot(reference) * 0xc;
    *cursor = *(datum_index *)(element + 8);
    return *(datum_index *)(element + 4);
}

/**
 * Advances the light cluster cursor and returns the next light handle.
 *
 * Original register convention: stack -> cursor (cdecl).
 *
 * @address 0x004f3500
 */
datum_index halo::objects::LightSystem::cluster_iterate_next(datum_index *cursor)
{
    uint8_t *element;

    if (*cursor == k_datum_index_none) {
        return k_datum_index_none;
    }
    element = (uint8_t *)light_cluster_references->data + halo::datum_slot(*cursor) * 0xc;
    *cursor = *(datum_index *)(element + 8);
    return *(datum_index *)(element + 4);
}

namespace {
static uint8_t *light_get(datum_index handle)
{
    return (uint8_t *)light_data->data + halo::datum_slot(handle) * 0x7c;
}

static void offset_center(real_point3d *out, uint8_t *light, float distance)
{
    out->x = distance * *(float *)(light + 0x3c) + *(float *)(light + 0x30);
    out->y = distance * *(float *)(light + 0x40) + *(float *)(light + 0x34);
    out->z = distance * *(float *)(light + 0x44) + *(float *)(light + 0x38);
}
}

/**
 * Returns the centre and radius of the bounding sphere used when rendering a light.
 *
 * Original register convention: stack -> handle, center_out, radius_out (cdecl).
 *
 * @address 0x004f3530
 */
void halo::objects::LightSystem::get_render_bounds(datum_index handle, real_point3d *center_out, float *radius_out)
{
    uint8_t *light = light_get(handle);
    uint8_t *definition = halo::objects::tag_record_bytes(*(datum_index *)(light + 4));
    float reach = *(float *)(definition + 0xc) * *(float *)(definition + 4);
    float angle = *(float *)(definition + 0x14);

    if ((definition[0] & 2) == 0) {
        reach = reach * *(float *)(definition + 0x24);
    }
    if (reach < *(float *)(definition + 0x18)) {
        *center_out = *(real_point3d *)(light + 0x30);
        *radius_out = *(float *)(definition + 0x18);
    } else if (!(angle < 1.5707964f)) {
        *center_out = *(real_point3d *)(light + 0x30);
        *radius_out = reach;
    } else if (!(angle < 0.78539819f)) {
        *radius_out = reach * *(float *)(definition + 0x28);
        offset_center(center_out, light, reach * *(float *)(definition + 0x20));
    } else {
        float distance = reach / *(float *)(definition + 0x20);
        *radius_out = distance;
        offset_center(center_out, light, distance);
    }
}

/**
 * Returns nonzero when the light has not been marked this frame.
 *
 * Original register convention: stack -> handle (cdecl); returns AL.
 *
 * @address 0x004f3620
 */
uint8_t halo::objects::LightSystem::not_marked_this_frame(datum_index handle)
{
    return *(int32_t *)(light_get(handle) + 0xc) != light_frame_counter;
}

/**
 * Marks a light as seen this frame and returns its previous mark state.
 *
 * Original register convention: stack -> handle (cdecl); returns AL.
 *
 * @address 0x004f3650
 */
uint8_t halo::objects::LightSystem::mark_this_frame(datum_index handle)
{
    uint8_t *light = light_get(handle);

    if (*(int32_t *)(light + 0xc) == light_frame_counter) {
        return 0;
    }
    *(int32_t *)(light + 0xc) = light_frame_counter;
    return 1;
}
