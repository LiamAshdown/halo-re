#include "halo/objects/record_access.hpp"
#include "halo/objects/flag.hpp"
#include "halo/scenario/api.hpp"
#include "halo/math/api.hpp"
#include "halo/memory/api.hpp"
#include "halo/cache/api.hpp"
#include "halo/physics/api.hpp"
#include "halo/core/lcg.hpp"
#include "halo/render/api.hpp"
#include "halo/saved_games/api.hpp"
#include "halo/rasterizer/api.hpp"
#include "halo/objects/api.hpp"
#include "halo/core/link.hpp"
#include "halo/ai/vars.hpp"
#include "halo/objects/vars.hpp"
#include "halo/units/vars.hpp"
#include "halo/core/libm.hpp"
#include "halo/core/x87.hpp"
#include "halo/ai/api.hpp"
#include "halo/units/api.hpp"
#include "rasterizer.h"
#include "halo/rasterizer/globals.hpp"
#include "halo/rasterizer/render_device.hpp"
#include "halo/rasterizer/vars.hpp"
#include "halo/render/d3d9.hpp"
#include "halo/effects/vars.hpp"
#include "halo/game/vars.hpp"
#include "halo/effects/api.hpp"
#include "halo/game/api.hpp"

static auto &rasterizer_dynamic_vertex_slots = halo::link::ref<rasterizer_dynamic_vertex_slot [k_rasterizer_dynamic_vertex_slots]>(halo::game::vars().rasterizer_dynamic_vertex_slots);
static auto &rasterizer_dynamic_vertex_caches = halo::link::ref<rasterizer_dynamic_vertex_cache [k_rasterizer_vertex_type_count]>(halo::rasterizer::vars().rasterizer_dynamic_vertex_caches);
static auto &rasterizer_vertex_buffer_slots = halo::link::ref<rasterizer_vertex_buffer_slot [k_rasterizer_vertex_buffer_slots]>(halo::rasterizer::vars().rasterizer_vertex_buffer_slots);
static auto &k_render_identity_matrix_ptr = halo::link::ref<void *>(halo::effects::vars().k_render_identity_matrix_ptr);
static auto &flag_data = halo::link::ref<data_array *>(halo::objects::vars().flag_data);
static auto &global_origin3d_pointer = halo::link::ref<real_point3d *>(halo::ai::vars().global_origin3d_pointer);
static auto &global_zero_vector3d_pointer = halo::link::ref<real_point3d *>(halo::units::vars().global_zero_vector3d_pointer);

/**
 * Creates the flag data array with room for two flags.
 *
 * Original register convention: none (no parameters).
 *
 * @address 0x004fb4d0
 */
void halo::objects::FlagSystem::initialize()
{
    flag_data = halo::saved_games::game_state_new((char *)"flag", k_maximum_flags, 0x16bc  );
}

/**
 * Disposes the flag data array.
 *
 * Original register convention: none (no parameters).
 *
 * @address 0x004fb4f0
 */
void halo::objects::FlagSystem::dispose()
{
    flag_data->valid = 1;
    halo::memory::data_delete_all(flag_data);
}

/**
 * Clears the disposing flag of the flag data array after a dispose pass.
 *
 * Original register convention: none (no parameters).
 *
 * @address 0x004fb510
 */
void halo::objects::FlagSystem::clear_disposing_flag()
{
    flag_data->valid = 0;
}

/**
 * Re-reads the flag data array pointer from the game state after a state restore.
 *
 * Original register convention: none (no parameters).
 *
 * @address 0x004fb520
 */
void halo::objects::FlagSystem::reset_data_pointer()
{
    if (flag_data != 0) {
        flag_data = 0;
    }
}

/**
 * Allocates a flag datum for a Flag tag and returns its handle; refuses grids with too many vertices.
 *
 * Original register convention: stack -> flag_tag.
 *
 * @address 0x004fb540
 */
datum_index halo::objects::FlagSystem::create(datum_index flag_tag)
{
    datum_index handle = k_datum_index_none;

    if (flag_tag != k_datum_index_none) {
        Flag *tag = (Flag *)halo::cache::globals().tag_instances[halo::datum_slot(flag_tag)].data;

        handle = halo::memory::datum_new(flag_data);
        if (handle != k_datum_index_none) {
            flag *entry = &((flag *)flag_data->data)[halo::datum_slot(handle)];

            if (tag->height * tag->width < (int32_t)k_maximum_flag_cloth_vertices &&
                tag->width < 0x28 && (int32_t)halo::objects::tag_handle(tag->blue_flag_shader) != -1) {
                int16_t row;

                entry->definition_tag = flag_tag;
                entry->invalid = 0;
                entry->unknown_03 = 0;
                entry->object_index = k_datum_index_none;
                entry->previous_marker_position.x = 0.0f;
                entry->previous_marker_position.y = 0.0f;
                entry->previous_marker_position.z = 0.0f;

                for (row = 0; row < tag->width; row++) {
                    int16_t col;

                    for (col = 0; col < tag->height; col++) {
                        flag_vertex *vertex = &entry->vertices[tag->height * row + col];

                        vertex->position = *global_zero_vector3d_pointer;
                        vertex->previous_position = *global_origin3d_pointer;

                        if (row < tag->width - 1 && col < tag->height - 1) {
                            entry->cell_split_codes[(tag->height - 1) * row + col] = 0;
                        }
                    }
                }

                halo::objects::flag_cloth_mark_border_cells(entry, tag);
                halo::objects::flag_cloth_init_shape_constraints(entry, tag);
                return handle;
            }
            entry->invalid = 1;
        }
    }
    return handle;
}

/**
 * Marks the border cells of the flag's cloth grid so the simulation treats them specially.
 *
 * Original register convention: EDI -> tag, carried over unmodified from flag_new's own frame (this function never
 * reloads it); stack -> entry (the one value flag_new actually pushes).
 *
 * @address 0x004fb6d0
 */
void halo::objects::FlagView::cloth_mark_border_cells(Flag *tag)
{
    flag *entry = self;
    if (tag->attached_edge_shape != 0 && (int32_t)tag->attachment_points.count > 0) {
        int16_t col = 0;
        int32_t point_index = 0;

        do {
            int16_t raw, clamped, half, split;

            if (tag->height <= col) {
                return;
            }

            raw = halo::objects::block_element<FlagAttachmentPoint>(tag->attachment_points, point_index).height_to_next_attachment;
            if (raw < 0) {
                clamped = 0;
            } else {
                int16_t remaining = tag->height - col;
                clamped = (remaining < raw) ? remaining : raw;
            }
            half = clamped & ~1;
            split = half / 2;

            halo::objects::flag_cloth_stamp_region_split_flags(0, tag, entry, col, split, 4);
            halo::objects::flag_cloth_stamp_region_split_flags(0, tag, entry, col + split, split, 5);

            col = col + half;
            point_index = point_index + 1;
        } while (point_index < (int32_t)tag->attachment_points.count);
    }
}

/**
 * Initialises the rest lengths and shape constraints of the flag's cloth grid from the Flag tag.
 *
 * Original register convention: EDI -> tag; stack -> entry.
 *
 * @address 0x004fb770
 */
void halo::objects::FlagView::cloth_init_shape_constraints(Flag *tag)
{
    flag *entry = self;
    int16_t shape = (int16_t)tag->trailing_edge_shape;

    if (shape != 0) {
        int16_t si;
        int32_t edge_count;

        if (shape == 3 || shape == 4) {
            si = tag->height - 1;
        } else {
            si = tag->height / 2;
        }

        edge_count = tag->width + tag->trailing_edge_shape_offset - si - 1;
        if (edge_count < 0) {
            edge_count = 0;
        }

        if (shape == 3) {
            halo::objects::flag_cloth_stamp_region_split_flags((int16_t)edge_count, tag, entry, 0, si, 3);
        } else if (shape == 4) {
            halo::objects::flag_cloth_stamp_region_split_flags((int16_t)edge_count, tag, entry, 0, si, 2);
        } else if (shape == 1) {
            halo::objects::flag_cloth_stamp_region_split_flags((int16_t)edge_count, tag, entry, 0, si, 2);
            halo::objects::flag_cloth_stamp_region_split_flags((int16_t)edge_count, tag, entry, si, si, 3);
        } else if (shape == 2) {
            halo::objects::flag_cloth_stamp_region_split_flags((int16_t)edge_count, tag, entry, 0, si, 3);
            halo::objects::flag_cloth_stamp_region_split_flags((int16_t)edge_count, tag, entry, si, si, 2);
        }
    }
}

/**
 * Writes the split code into a rectangular region of the flag's per-cell split flag table.
 *
 * Original register convention: EAX -> outer_start (Ghidra's in_AX), stack -> tag, entry, inner_start, size,
 * split_code (in that order, matching both call sites' push order).
 *
 * @address 0x004fb840
 */
void halo::objects::FlagView::cloth_stamp_region_split_flags(int16_t outer_start, Flag *tag, int16_t inner_start,
    int16_t size, uint16_t split_code)
{
    flag *entry = self;
    int16_t outer;

    for (outer = outer_start; outer < size + outer_start; outer++) {
        int16_t inner;

        for (inner = inner_start; inner < size + inner_start; inner++) {
            if (outer >= 0 && inner >= 0 &&
                outer < tag->width - 1 && inner < tag->height - 1) {
                int16_t a, b;
                uint16_t *cell;

                if (split_code == 4 || split_code == 5) {
                    a = outer - outer_start;
                } else {
                    a = (size - outer) - 1 + outer_start;
                }
                if (split_code == 4 || split_code == 2) {
                    b = inner - inner_start;
                } else {
                    b = (size - inner) - 1 + inner_start;
                }

                cell = (uint16_t *)&entry->cell_split_codes[(tag->height - 1) * outer + inner];
                if (a == b) {
                    *cell = split_code;
                } else {
                    *cell = (uint16_t)(a <= b);
                }
            }
        }
    }
}

/**
 * Frees a flag datum from the flag data array.
 *
 * Original register convention: stack -> flag_index (cdecl).
 *
 * @address 0x004fb970
 */
void halo::objects::FlagSystem::destroy(datum_index flag_index)
{
    halo::memory::datum_delete(flag_data, flag_index);
}

/**
 * Widget render hook for a flag: resolves the owning object and the flag datum.
 *
 * Original register convention: stack -> object_index, flag_index, arg3, arg4 (cdecl).
 *
 * @address 0x004fb980
 */
void halo::objects::FlagSystem::render_callback(datum_index object_index, datum_index flag_index, uint32_t arg3,
    uint32_t arg4)
{
    flag *self = reinterpret_cast<flag *>(static_cast<uint8_t *>(flag_data->data) + halo::datum_slot(flag_index) * 0x16bc);
    Flag *tag = halo::objects::tag_as<Flag>(self->definition_tag);

    self->object_index = object_index;
    if (self->update_counter > 5 || self->unknown_03 == 0) {
        halo::objects::flag_cloth_update(self, tag, 5.0f);
        self->unknown_03 = 1;
    }
    self->update_counter = 0;
    if (self->invalid == 0) {
        halo::objects::flag_render(tag, (flag *)self, (const render_lighting *)(uintptr_t)arg3, (const uint32_t *)(uintptr_t)arg4);
    }
}

/**
 * Runs the per-tick update of every live flag.
 *
 * Original register convention: single float stack parameter (dt); Ghidra shows a clean param_1 with no in_REG
 * marker.
 *
 * @address 0x004fba00
 */
void halo::objects::FlagSystem::update(float dt)
{
    data_array *flags = flag_data;
    datum_index current = halo::memory::datum_next(-1, flags);

    for (;;) {
        int32_t next_index;

        if (current == k_datum_index_none) {
            return;
        }

        {
            flag *entry = (flag *)((uint8_t *)flags->data + halo::datum_slot(current) * flags->size);
            datum_index object_index = entry->object_index;
            void *tag_data = halo::cache::globals().tag_instances[halo::datum_slot(entry->definition_tag)].data;
            int16_t *update_counter = &entry->update_counter;

            *update_counter = *update_counter + 1;
            if (object_index != k_datum_index_none && *update_counter < 5 && dt != 0.0f) {
                halo::objects::flag_cloth_update(entry, (Flag *)tag_data, dt);
                flags = flag_data;
            }
        }

        next_index = (int32_t)(int16_t)((current & 0xffff) + 1);
        current = k_datum_index_none;
        if (next_index < 0 || flags->last_index <= next_index) {
            return;

        }

        {
            int16_t *identifier = (int16_t *)((uint8_t *)flags->data + next_index * flags->size);
            int32_t i = next_index;

            do {
                if (*identifier != 0) {
                    current = (datum_index)(((uint32_t)(uint16_t)*identifier << 16) | (uint16_t)i);
                    break;
                }
                i = i + 1;
                identifier = (int16_t *)((uint8_t *)identifier + flags->size);
            } while ((int16_t)i < flags->last_index);
        }
    }
}

/**
 * Advances the flag cloth simulation by dt seconds.
 *
 * Original register convention: three clean stack parameters (entry, tag data, dt); Ghidra shows no in_REG/unaff_
 * markers for this function's own parameters (unlike its callees).
 *
 * @address 0x004fbae0
 */
void halo::objects::FlagView::cloth_update(Flag *tag, float dt)
{
    flag *entry = self;
    int retracting = (entry->deployed == 0);
    bsp_leaf_reference node_ref;
    uint32_t physics_a = 0, physics_b = 0;

    real_point3d marker_positions[8];
    uint8_t row_table[488];
    int16_t row_start_scratch[8];
    int16_t column_marker_index[40];
    int8_t moving;

    halo::objects::flag_pole_get_marker_positions(entry, &node_ref, marker_positions, row_table,
                                    row_start_scratch, column_marker_index, tag);
    real_point3d water_probe_point = {0.0f, 0.0f, 0.0f};
    int16_t water_probe_weather = -1;
    moving = halo::scenario::scenario_location_get_water_and_weather(&water_probe_point, &node_ref, &water_probe_weather);

    if (entry->invalid == 0) {

        int16_t neighbour_dcol[3] = { -1, 0, 0 };
        int16_t neighbour_drow[3] = { 0, 1, -1 };
        float corner_length[3];
        int32_t n0;

        for (n0 = 0; n0 < 3; n0++) {
            float fx = (float)neighbour_dcol[n0] * tag->cell_width;
            float fy = (float)neighbour_drow[n0] * tag->cell_height;
            corner_length[n0] = (float)halo::libm::sqrt(fx * fx + fy * fy);
        }

        if (tag->width > 0) {
            int32_t row_step = ((int16_t)retracting != 0) ? 1 : -1;
            int16_t col = 0;

            do {
                int16_t row_cursor;
                int16_t bound;

                row_cursor = (col == 0) ? (int16_t)(tag->height - 1) : 0;

                for (;;) {
                    flag_vertex *vertex;
                    real_point3d target;
                    int32_t contributor_count;
                    uint32_t mode;
                    float wind_scale;
                    real_vector3d wind_dir;

                    if (col == 0) {
                        if (!(0 < row_cursor)) break;
                    } else {
                        if (!(row_cursor < tag->height)) break;
                    }

                    vertex = &entry->vertices[tag->height * col + row_cursor];
                    contributor_count = 0;
                    mode = 1;

                    if (!moving) {
                        wind_scale = *(float *)(halo::objects::tag_record_bytes(tag->physics.tag_id.index) + 0x24) *
                                     tag->wind_noise * 0.0004f;
                    } else {
                        mode = 3;
                        wind_scale = *(float *)(halo::objects::tag_record_bytes(tag->physics.tag_id.index) + 0x28) *
                                     tag->wind_noise * 0.00016f;
                    }

                    halo::math::globals().effect_random_seed = halo::advance_random_seed(halo::math::globals().effect_random_seed);
                    {
                        int16_t idx = (int16_t)(((halo::math::globals().effect_random_seed >> halo::k_random_high_shift) *
                                                  (uint32_t)halo::math::globals().sphere_point_table_count) >> 16);
                        real_point3d *dir = &halo::math::globals().sphere_point_table[idx];
                        wind_dir.i = dir->x * wind_scale;
                        wind_dir.j = dir->y * wind_scale;
                        wind_dir.k = dir->z * wind_scale;
                    }

                    target = vertex->position;

                    halo::physics::point_physics_tick(reinterpret_cast<real_vector3d *>(&vertex->previous_position), mode,
                        (PointPhysics *)halo::cache::globals().tag_instances[tag->physics.tag_id.index].data, &node_ref,
                        physics_b, &target, &wind_dir, 0, 0, 0.02f, dt);

                    if (col == 0 && column_marker_index[row_cursor] != -1) {
                        int32_t m = column_marker_index[row_cursor];
                        target = marker_positions[m];
                    } else {
                        real_point3d contributions[3];
                        int32_t n;

                        for (n = 0; n < 3; n++) {
                            int16_t ncol = neighbour_dcol[n] + col;
                            int16_t nrow = neighbour_drow[n] + row_cursor;

                            if (ncol >= 0 && ncol < tag->width && nrow >= 0 && nrow < tag->height) {
                                real_point3d *nvp = &entry->vertices[tag->height * ncol + nrow].position;
                                float dx = target.x - nvp->x;
                                float dy = target.y - nvp->y;
                                float dz = target.z - nvp->z;
                                float dist = (float)halo::libm::sqrt(dx * dx + dy * dy + dz * dz);

                                if (dist >= 0.0001f || dist <= -0.0001f) {
                                    float inv = 1.0f / dist;
                                    dx *= inv; dy *= inv; dz *= inv;
                                }

                                contributions[contributor_count].x = dx * corner_length[n] + nvp->x;
                                contributions[contributor_count].y = dy * corner_length[n] + nvp->y;
                                contributions[contributor_count].z = dz * corner_length[n] + nvp->z;
                                contributor_count++;
                            }
                        }

                        {
                            float sum_x = 0.0f, sum_y = 0.0f, sum_z = 0.0f, weight_total = 0.0f;
                            int32_t idx2;

                            for (idx2 = 0; idx2 < contributor_count; idx2++) {
                                float w = (col == 0 && idx2 == 0) ? 4.0f : 1.0f;
                                sum_x += w * contributions[idx2].x;
                                sum_y += w * contributions[idx2].y;
                                sum_z += w * contributions[idx2].z;
                                weight_total += w;
                            }
                            if (col == 0) {
                                float *row_positions = (float *)row_table;
                                sum_x += row_positions[row_cursor * 3 + 0] * 4.0f;
                                sum_y += row_positions[row_cursor * 3 + 1] * 4.0f;
                                sum_z += row_positions[row_cursor * 3 + 2] * 4.0f;
                                weight_total += 4.0f;
                            }
                            weight_total = 1.0f / weight_total;
                            target.x = weight_total * sum_x;
                            target.y = weight_total * sum_y;
                            target.z = weight_total * sum_z;
                        }
                    }

                    {
                        float inv_dt = 1.0f / dt;
                        real_vector3d *velocity_slot = reinterpret_cast<real_vector3d *>(&vertex->previous_position);
                        velocity_slot->i = (target.x - vertex->position.x) * inv_dt;
                        velocity_slot->j = (target.y - vertex->position.y) * inv_dt;
                        velocity_slot->k = (target.z - vertex->position.z) * inv_dt;
                        vertex->position = target;
                    }

                    row_cursor = (int16_t)(row_cursor + row_step);
                }

                col = col + 1;
            } while (col < tag->width);
        }
    }
}

/**
 * Gathers the pole marker positions that anchor the flag's cloth columns and rows.
 *
 * @address 0x004fc020
 */
void halo::objects::FlagView::pole_get_marker_positions(bsp_leaf_reference *node_ref, real_point3d *marker_positions,
    uint8_t *row_table, int16_t *row_start_scratch, int16_t *column_marker_index, Flag *tag)
{
    flag *entry = self;
    int32_t i;

    for (i = 0; i < (int32_t)tag->attachment_points.count; i++) {
        object_marker marker;
        halo::objects::object_get_node_local_transform(entry->object_index,
            halo::objects::block_element<FlagAttachmentPoint>(tag->attachment_points, i).marker_name.string,
            &marker, 1);
        marker_positions[i] = marker.node_transform.position;
    }

    {

        int32_t node_index = halo::physics::bsp3d_node_find_leaf(0, (ModelCollisionGeometryBSP *)halo::physics::globals().collision_bsp, &marker_positions[0]);

        node_ref->leaf_index = node_index;
        if (node_index == -1) {
            node_ref->cluster_index = -1;
        } else {
            node_ref->cluster_index = *(int16_t *)((uint8_t *)halo::scenario::globals().structure_bsp->leaves.pointer +
                                                 (uint32_t)(node_index & halo::k_leaf_index_mask) * 0x10 + 8);
        }
    }

    if (entry->invalid == 0) {
        int32_t row;

        for (row = 0; row < tag->height; row++) {
            column_marker_index[row] = -1;
        }

        {
            int32_t row_cursor = 0;
            int32_t point_index = 0;

            while (point_index < (int32_t)tag->attachment_points.count) {
                int16_t row16 = (int16_t)row_cursor;
                int16_t raw, span;
                int32_t row_end;

                if (tag->height <= row16) {
                    break;
                }

                raw = halo::objects::block_element<FlagAttachmentPoint>(tag->attachment_points, point_index).height_to_next_attachment;
                if (raw < 0) {
                    span = 0;
                } else {
                    int16_t remaining = tag->height - row16;
                    span = (remaining < raw) ? remaining : raw;
                }

                row_start_scratch[point_index] = row16;
                row_end = (span & ~1) + row_cursor;
                column_marker_index[row16] = (int16_t)point_index;

                if (row16 <= (int16_t)row_end) {
                    uint8_t *out = row_table + 8 + row16 * 0xc;
                    uint32_t count = (uint32_t)(uint16_t)((row_end - row_cursor) + 1);
                    int32_t interp_row = row16;

                    row_cursor = row_cursor + (int32_t)count;

                    do {
                        if ((int16_t)row_end != interp_row) {
                            real_point3d *base = &marker_positions[point_index];
                            float t = ((float)interp_row - (float)row16) /
                                      ((float)(int16_t)row_end - (float)row16);
                            float one_minus_t = 1.0f - t;

                            ((float *)out)[0] = one_minus_t * base[0].x + t * base[1].x;
                            ((float *)out)[1] = t * base[1].y + one_minus_t * base[0].y;
                            ((float *)out)[2] = t * base[1].z + one_minus_t * base[0].z;
                        }
                        interp_row = interp_row + 1;
                        out = out + 0xc;
                        count = count - 1;
                    } while (count != 0);
                }

                row_cursor = row_cursor - 1;
                point_index = point_index + 1;
            }
        }

        {
            real_point3d new_position = marker_positions[0];
            real_point3d old_position = entry->previous_marker_position;
            int32_t tx = halo::x87::__ftol((double)(new_position.x - old_position.x));
            int skip = (tx < 0 ? -tx : tx) <= 1;

            if (skip) {
                int32_t ty = halo::x87::__ftol((double)(new_position.y - old_position.y));
                skip = (ty < 0 ? -ty : ty) <= 1;
                if (skip) {
                    int32_t tz = halo::x87::__ftol((double)(new_position.z - old_position.z));
                    skip = (tz < 0 ? -tz : tz) <= 1;
                }
            }

            if (!skip && tag->width > 0) {
                real_vector3d delta;
                int16_t r;

                delta.i = new_position.x - old_position.x;
                delta.j = new_position.y - old_position.y;
                delta.k = new_position.z - old_position.z;

                for (r = 0; r < tag->width; r++) {
                    int16_t c;
                    for (c = 0; c < tag->height; c++) {
                        real_point3d *vertex_position =
                            &entry->vertices[tag->height * r + c].position;
                        vertex_position->x += delta.i;
                        vertex_position->y += delta.j;
                        vertex_position->z += delta.k;
                    }
                }
            }
        }
    }

    entry->previous_marker_position = marker_positions[0];
}

namespace {

/**
 * One 0x44-byte vertex of the uncompressed model vertex type (rasterizer vertex type 4), as written by the
 * flag cloth mesh builder. The binormal and tangent are left untouched in the dynamic vertex cache.
 */
struct flag_cloth_vertex {
    real_point3d position;
    real_vector3d normal;
    real_vector3d binormal;
    real_vector3d tangent;
    float u;
    float v;
    int16_t node_index[2];
    float node_weight[2];
};
static_assert(sizeof(flag_cloth_vertex) == 0x44, "flag cloth vertex must match the model_uncompressed vertex layout");

enum class flag_cell_split : int16_t {
    both_diagonals = 0,
    none = 1,
    first_triangle_lower_left = 2,
    first_triangle_lower_right = 3,
    first_triangle_upper_left = 4,
    first_triangle_upper_right = 5,
};

typedef int32_t (__stdcall *flag_d3d_unlock_fn)(void *self);

}  // namespace

/**
 * Builds the triangle mesh of one flag's cloth grid and draws it with the flag's red or blue shader. Every grid
 * point becomes a model vertex whose normal is the normalized cross product of the grid edges toward the next
 * column and the next row (the last column and row reuse their neighbours' edges), with texture coordinates
 * running 0..1 across the grid; each cell then emits zero, one or two triangles according to its split code.
 *
 * The mesh is written into the dynamic vertex and index caches, which are unlocked before the draw. The shader
 * is drawn through a model draw context that carries the supplied lighting and change colours, with the centre
 * of the four corner points as the sort position of the transparent shader types. The red shader is used unless
 * the owning object has a non-zero owner team, and the blue one is also the fallback when the chosen shader tag
 * is missing.
 *
 * @address 0x004fc350
 */
void halo::objects::FlagSystem::render(Flag *tag, flag *entry, const render_lighting *lighting,
    const uint32_t *animation)
{
    object *owner = ((object_header *)halo::objects::globals().object_data->data)[halo::datum_slot(entry->object_index)].data;
    datum_index shader_index = *(datum_index *)(owner->owner_team != 0 ? &tag->blue_flag_shader.tag_id : &tag->red_flag_shader.tag_id);
    int16_t width = tag->width;
    int16_t height = tag->height;
    int32_t triangle_total;
    int32_t index_slot;
    int32_t vertex_slot;
    flag_cloth_vertex *vertices;
    uint16_t *indices;
    float inverse_width;
    float inverse_height;
    int16_t vertex_number = 0;
    int16_t triangle_count = 0;
    int16_t column;
    int16_t row;

    if (shader_index == k_datum_index_none) {
        shader_index = *(datum_index *)&tag->blue_flag_shader.tag_id;
    }

    triangle_total = (int16_t)((uint16_t)(height * 2 - 2) * (uint16_t)(width - 1));
    halo::rasterizer::globals().vertex_buffer_lock_state = 0xb;
    index_slot = halo::rasterizer::rasterizer_dynamic_index_cache_reserve(triangle_total);
    vertex_slot = halo::rasterizer::rasterizer_dynamic_vertex_cache_reserve(_rasterizer_vertex_type_model_uncompressed,
        (int32_t)height * (int32_t)width);
    if (index_slot == -1 || vertex_slot == -1) {
        halo::rasterizer::globals().vertex_buffer_lock_state = 0;
        return;
    }

    vertices = (flag_cloth_vertex *)halo::rasterizer::rasterizer_dynamic_vertex_cache_lock(vertex_slot);
    indices = (uint16_t *)halo::render::rasterizer_dynamic_index_slot_lock(index_slot);
    inverse_width = 1.0f / (float)(width - 1);
    inverse_height = 1.0f / (float)(height - 1);

    for (column = 0; column < width; column++) {
        for (row = 0; row < height; row++) {
            int16_t edge_column = (column >= width - 1) ? (int16_t)(column - 1) : column;
            int16_t edge_row = (row >= height - 1) ? (int16_t)(row - 1) : row;
            const real_point3d &origin = entry->vertices[edge_column * height + edge_row].position;
            const real_point3d &next_column = entry->vertices[(edge_column + 1) * height + edge_row].position;
            const real_point3d &next_row = entry->vertices[edge_column * height + edge_row + 1].position;
            real_vector3d column_edge = {next_column.x - origin.x, next_column.y - origin.y, next_column.z - origin.z};
            real_vector3d row_edge = {next_row.x - origin.x, next_row.y - origin.y, next_row.z - origin.z};
            flag_cloth_vertex *out = &vertices[vertex_number];

            out->normal.i = column_edge.j * row_edge.k - column_edge.k * row_edge.j;
            out->normal.j = column_edge.k * row_edge.i - column_edge.i * row_edge.k;
            out->normal.k = column_edge.i * row_edge.j - column_edge.j * row_edge.i;
            halo::math::vector3d_normalize_with_length(out->normal);

            out->position = entry->vertices[column * height + row].position;
            out->u = (float)column * inverse_width;
            out->v = (float)row * inverse_height;
            out->node_index[0] = 0;
            out->node_index[1] = 0;
            out->node_weight[0] = 0.5f;
            out->node_weight[1] = 0.5f;
            vertex_number++;
        }
    }

    for (column = 0; column < width - 1; column++) {
        for (row = 0; row < height - 1; row++) {
            flag_cell_split split = (flag_cell_split)entry->cell_split_codes[column * (height - 1) + row];
            uint16_t corner_a = (uint16_t)(height * column + row);
            uint16_t corner_b = (uint16_t)(height * column + row + 1);
            uint16_t corner_c = (uint16_t)(height * (column + 1) + row);
            uint16_t corner_d = (uint16_t)(height * (column + 1) + row + 1);
            uint16_t *triangle = indices + triangle_count * 3;

            switch (split) {
            case flag_cell_split::both_diagonals:
                triangle[0] = corner_a;
                triangle[1] = corner_c;
                triangle[2] = corner_b;
                triangle[3] = corner_b;
                triangle[4] = corner_c;
                triangle[5] = corner_d;
                triangle_count += 2;
                break;
            case flag_cell_split::first_triangle_lower_left:
                triangle[0] = corner_a;
                triangle[1] = corner_b;
                triangle[2] = corner_c;
                triangle_count++;
                break;
            case flag_cell_split::first_triangle_lower_right:
                triangle[0] = corner_a;
                triangle[1] = corner_b;
                triangle[2] = corner_d;
                triangle_count++;
                break;
            case flag_cell_split::first_triangle_upper_left:
                triangle[0] = corner_a;
                triangle[1] = corner_d;
                triangle[2] = corner_c;
                triangle_count++;
                break;
            case flag_cell_split::first_triangle_upper_right:
                triangle[0] = corner_b;
                triangle[1] = corner_d;
                triangle[2] = corner_c;
                triangle_count++;
                break;
            default:
                break;
            }
        }
    }

    {
        rasterizer_dynamic_vertex_slot &slot = rasterizer_dynamic_vertex_slots[vertex_slot];
        int32_t buffer_handle = rasterizer_dynamic_vertex_caches[slot.vertex_type].buffer_handle;

        halo::rasterizer::render_device().buffer_unlock(halo::rasterizer::globals().dynamic_index_buffer);
        if (buffer_handle != 0) {
            halo::rasterizer::render_device().buffer_unlock(
                (void *)(uintptr_t)rasterizer_vertex_buffer_slots[buffer_handle - 1].hardware_buffer);
        }
    }

    {
        Shader *shader = (Shader *)halo::cache::globals().tag_instances[halo::datum_slot(shader_index)].data;
        const flag_vertex *grid = entry->vertices;
        const flag_vertex &far_corner = grid[width * height - 1];
        const flag_vertex &column_corner = grid[height * (width - 1)];
        const flag_vertex &row_corner = grid[height - 1];
        const flag_vertex &near_corner = grid[0];
        real_point3d center;
        rasterizer_model_draw_context context;
        uint32_t *context_words = (uint32_t *)&context;
        size_t word;

        center.x = (far_corner.position.x + row_corner.position.x + column_corner.position.x + near_corner.position.x) * 0.25f;
        center.y = (far_corner.position.y + row_corner.position.y + column_corner.position.y + near_corner.position.y) * 0.25f;
        center.z = (far_corner.position.z + row_corner.position.z + column_corner.position.z + near_corner.position.z) * 0.25f;

        for (word = 0; word < sizeof(context) / sizeof(uint32_t); word++) {
            context_words[word] = 0;
        }
        context.object_index = 1;
        context.node_matrices = (uint32_t)(uintptr_t)k_render_identity_matrix_ptr;
        context.node_count = 1;
        context.lighting = *lighting;
        context.change_colors = animation[0];
        context.function_values = animation[1];
        context.center = center;
        context.base_map_u_scale = 1.0f;
        context.base_map_v_scale = 1.0f;

        if (halo::rasterizer::fields::models_enabled != 0) {
            halo::rasterizer::globals().render_states_dirty = 1;
            halo::rasterizer::fields::sky_pass_active = 0;
            if ((uint32_t)halo::rasterizer::globals().device_version < halo::d3d9::k_pixel_shader_version_1_1) {
                halo::rasterizer::render_device().set_render_state((uint32_t)halo::d3d9::render_state::lighting, 1);
            }
        }

        halo::rasterizer::rasterizer_model_draw_prepare_states(&context, 0);
        if (shader->shader_type == 1 || (4 < shader->shader_type && shader->shader_type < 0xc)) {
            halo::rasterizer::rasterizer_transparent_geometry_group_build(nullptr, (uint8_t *)shader, 0, nullptr,
                index_slot, triangle_count, nullptr, vertex_slot, &center);
        } else {
            halo::rasterizer::rasterizer_shader_environment_draw_dispatch(vertex_slot, (uint8_t *)shader, 0, nullptr,
                index_slot, triangle_count, nullptr);
        }
        halo::rasterizer::rasterizer_model_draw_restore_states();

        if (halo::rasterizer::fields::models_enabled != 0 &&
            (uint32_t)halo::rasterizer::globals().device_version < halo::d3d9::k_pixel_shader_version_1_1) {
            halo::rasterizer::render_device().set_render_state((uint32_t)halo::d3d9::render_state::lighting, 0);
        }
    }
    halo::rasterizer::globals().vertex_buffer_lock_state = 0;
}
