/**
 * @file src/structures/detail_object_system.cpp
 * Detail object (grass, debris sprites) game state and render list.
 * The original author notes and decompiles are in docs/original/structures/.
 */

#include "halo/core/crt.hpp"
#include "halo/structures/structures.hpp"
#include "halo/memory/api.hpp"
#include "halo/structures/api.hpp"
#include "halo/scenario/api.hpp"
#include "halo/rasterizer/api.hpp"
#include "halo/saved_games/api.hpp"
#include "halo/structures/globals.hpp"

extern "C" {
extern real_point3d render_camera_global;
}

namespace halo::structures {

void detail_object_system::globals_allocate(void)
{
    uint8_t *region = halo::saved_games::globals().game_state_base + halo::saved_games::globals().game_state_cursor;
    int32_t size = sizeof(detail_object_globals);

    halo::saved_games::globals().game_state_cursor = halo::saved_games::globals().game_state_cursor + size;
    halo::memory::crc32_update(&halo::saved_games::globals().game_state_crc, (uint8_t *)&size, 4);
    globals().detail_objects = (detail_object_globals *)region;

    globals().detail_objects->default_z_reference.z_reference_i = 0.0f;
    globals().detail_objects->default_z_reference.z_reference_j = 0.0f;
    globals().detail_objects->default_z_reference.z_reference_k = 1.0f;
    globals().detail_objects->default_z_reference.z_reference_l = 0.0f;
}

void detail_object_system::invalidate(void)
{
    ((uint8_t *)halo::structures::globals().detail_objects)[k_detail_objects_valid_offset] = 0;
}

void detail_object_system::update_render_list(void)
{
    ScenarioStructureBSPDetailObjectData *detail_data;
    detail_object_frame *frame = &globals().detail_objects->frames[0];
    int16_t cell_x, cell_y, cell_z;

    if (globals().local_player_globals->local_player_count != 1 || globals().current_local_player_index == -1) {
        return;
    }

    detail_data = (halo::scenario::globals().structure_bsp->detail_objects.count == 0)
        ? (ScenarioStructureBSPDetailObjectData *)0
        : (ScenarioStructureBSPDetailObjectData *)halo::scenario::globals().structure_bsp->detail_objects.pointer;

    cell_x = (int16_t)(int32_t)lrint((double)(render_camera_global.x * 0.125f - 0.5f));
    cell_y = (int16_t)(int32_t)lrint((double)(render_camera_global.y * 0.125f - 0.5f));
    cell_z = (int16_t)(int32_t)lrint((double)(render_camera_global.z * 0.125f - 0.5f));

    if (detail_data->bullshit != 0) {
        halo::rasterizer::rasterizer_detail_objects_begin();

        if (cell_x != frame->cell_x || cell_y != frame->cell_y || cell_z != frame->cell_z ||
            frame->valid == 0 || (detail_data->bullshit & 2) != 0) {
            int16_t layer_batch_counts[k_maximum_detail_object_layers];
            uint32_t layers_used = 0;
            int32_t layer;
            int32_t x_scan, y_scan_base, y_scan;
            int32_t z_key = cell_z;
            int32_t dx, dy;

            for (layer = 0; layer < k_maximum_detail_object_layers; layer = layer + 1) {
                layer_batch_counts[layer] = 0;
            }

            detail_data->bullshit = 1;
            frame->cell_x = cell_x;
            frame->cell_y = cell_y;
            frame->cell_z = cell_z;
            frame->valid = 1;
            frame->unknown_520f = 0;

            x_scan = (int32_t)cell_x + 1;
            y_scan_base = (int32_t)cell_y + 1;

            for (dx = 0; dx < 3; dx = dx + 1) {
                y_scan = y_scan_base;
                for (dy = 0; dy < 3; dy = dy + 1) {
                    detail_object_cell_key key;
                    ScenarioStructureBSPGlobalDetailObjectCell *lo, *hi;

                    key.cell_x = (int16_t)x_scan;
                    key.cell_y = (int16_t)y_scan;
                    key.unknown_06 = 0;

                    key.cell_z = (int16_t)(z_key - 1);
                    lo = detail_object_system::cell_lower_bound((ScenarioStructureBSPGlobalDetailObjectCell *)detail_data->cells.pointer, (ScenarioStructureBSPGlobalDetailObjectCell *)detail_data->cells.pointer + detail_data->cells.count, &key);
                    key.cell_z = (int16_t)(z_key + 2);
                    hi = detail_object_system::cell_upper_bound((ScenarioStructureBSPGlobalDetailObjectCell *)detail_data->cells.pointer, (ScenarioStructureBSPGlobalDetailObjectCell *)detail_data->cells.pointer + detail_data->cells.count, &key);
                    z_key = cell_z;

                    if (lo->cell_x == (int16_t)x_scan && lo->cell_y == (int16_t)y_scan &&
                        hi[-1].cell_x == (int16_t)x_scan && hi[-1].cell_y == (int16_t)y_scan &&
                        lo < hi) {
                        ScenarioStructureBSPGlobalDetailObjectCell *cell;

                        for (cell = lo; cell < hi; cell = cell + 1) {
                            int32_t z_diff = (int32_t)cell_z - (int32_t)cell->cell_z;
                            if ((z_diff < 0 ? -z_diff : z_diff) < 2) {
                                int32_t running_instance_offset = 0;
                                int32_t sub_index = 0;
                                uint32_t bit;

                                layers_used = layers_used | cell->valid_layers_flags;

                                for (layer = 0, bit = 1; layer < k_maximum_detail_object_layers;
                                     layer = layer + 1, bit = bit << 1) {
                                    if ((cell->valid_layers_flags & bit) != 0) {
                                        int16_t batch_index_in_layer = layer_batch_counts[layer];
                                        detail_object_batch *batch = &frame->batches[layer][batch_index_in_layer];

                                        batch->cell_x = cell->cell_x;
                                        layer_batch_counts[layer] = batch_index_in_layer + 1;
                                        batch->cell_y = cell->cell_y;
                                        batch->cell_z = (float)cell->cell_z + (float)cell->offset_z * 0.003921569f;
                                        batch->first_instance = (int32_t)cell->start_index + running_instance_offset;

                                        {
                                            uint16_t *counts = (uint16_t *)detail_data->counts.pointer;
                                            batch->instance_count = counts[cell->count_index + sub_index];
                                        }

                                        if (detail_data->z_reference_vectors.count == 0) {
                                            batch->z_reference = &globals().detail_objects->default_z_reference;
                                        } else {
                                            batch->z_reference = (ScenarioStructureBSPGlobalZReferenceVector *)
                                                detail_data->z_reference_vectors.pointer + (cell->count_index + sub_index);
                                        }

                                        running_instance_offset = running_instance_offset + batch->instance_count;
                                        sub_index = sub_index + 1;
                                    }
                                }
                            }
                        }
                    }

                    y_scan = y_scan - 1;
                }
                x_scan = x_scan - 1;
            }

            frame->render_list.layers = &frame->layers[0];
            frame->render_list.layer_count = 0;
            for (layer = 0; layer < k_maximum_detail_object_layers; layer = layer + 1) {
                if ((layers_used & (1u << layer)) != 0 && layer_batch_counts[layer] != 0) {
                    int16_t out_index = frame->render_list.layer_count;
                    frame->layers[out_index].batches = &frame->batches[layer][0];
                    frame->layers[out_index].batch_count = layer_batch_counts[layer];
                    frame->layers[out_index].layer_index = (int16_t)layer;
                    frame->render_list.layer_count = frame->render_list.layer_count + 1;
                }
            }
            halo::rasterizer::rasterizer_detail_objects_vertex_buffer_fill((rasterizer_detail_object_batches *)(&frame->render_list));
        }

        halo::rasterizer::rasterizer_detail_objects_draw((const rasterizer_detail_object_batches *)(&frame->render_list));
    }
}

ScenarioStructureBSPGlobalDetailObjectCell * detail_object_system::cell_lower_bound(ScenarioStructureBSPGlobalDetailObjectCell *begin, ScenarioStructureBSPGlobalDetailObjectCell *end, detail_object_cell_key *key)
{
    int32_t count = (int32_t)(end - begin);

    while (count > 0) {
        int32_t half = count / 2;
        ScenarioStructureBSPGlobalDetailObjectCell *mid = begin + half;
        int less = (mid->cell_x < key->cell_x) ||
            (mid->cell_x == key->cell_x &&
             (mid->cell_y < key->cell_y ||
              (mid->cell_y == key->cell_y && mid->cell_z < key->cell_z)));

        if (less) {
            begin = mid + 1;
            count = count - half - 1;
        } else {
            count = half;
        }
    }
    return begin;
}

ScenarioStructureBSPGlobalDetailObjectCell * detail_object_system::cell_upper_bound(ScenarioStructureBSPGlobalDetailObjectCell *begin, ScenarioStructureBSPGlobalDetailObjectCell *end, detail_object_cell_key *key)
{
    int32_t count = (int32_t)(end - begin);

    while (count > 0) {
        int32_t half = count / 2;
        ScenarioStructureBSPGlobalDetailObjectCell *mid = begin + half;
        int not_greater = (mid->cell_x < key->cell_x) ||
            (mid->cell_x == key->cell_x &&
             (mid->cell_y < key->cell_y ||
              (mid->cell_y == key->cell_y && mid->cell_z <= key->cell_z)));

        if (not_greater) {
            begin = mid + 1;
            count = count - half - 1;
        } else {
            count = half;
        }
    }
    return begin;
}

}  // namespace halo::structures
