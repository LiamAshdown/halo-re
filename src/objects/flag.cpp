#include "halo/objects/flag.hpp"
#include "halo/math/api.hpp"
#include "halo/memory/api.hpp"
#include "halo/cache/api.hpp"
#include "halo/physics/api.hpp"

extern "C" {
extern void *const flag_render_device_slot;
extern int32_t __ftol(double);
extern uint32_t bsp3d_node_find_leaf(int32_t node_index, ModelCollisionGeometryBSP *bsp, real_point3d *point);
extern void flag_cloth_init_shape_constraints(flag *entry);
extern void flag_cloth_mark_border_cells(flag *entry);
extern void flag_cloth_stamp_region_split_flags(int16_t outer_start, Flag *tag, flag *entry, int16_t inner_start, int16_t size, uint16_t split_code);
extern void flag_cloth_update(flag *entry, Flag *tag, float dt);
extern data_array *flag_data;
extern void flag_pole_get_marker_positions(flag *entry, bsp_leaf_reference *node_ref, real_point3d *marker_positions, uint8_t *row_table, int16_t *row_start_scratch, int16_t *column_marker_index, Flag *tag);
extern void flag_render(uint32_t *entry, uint32_t *submission_block, Flag *tag, uint8_t *second_geometry);
extern data_array *game_state_new(char *name, int16_t maximum_count, int16_t element_size);
extern real_point3d *global_origin3d_pointer;
extern ScenarioStructureBSP *global_structure_bsp;
extern real_point3d *global_zero_vector3d_pointer;
extern data_array *object_data;
extern int32_t object_get_node_local_transform(uint32_t object_index, char *marker_name, object_marker *marker, uint32_t flags);
extern int32_t rasterizer_dynamic_index_cache_reserve(void);
extern void *rasterizer_dynamic_index_slot_lock(void);
extern void *rasterizer_dynamic_vertex_cache_lock(void);
extern int32_t rasterizer_dynamic_vertex_cache_reserve(void);
extern void rasterizer_model_draw_prepare_states(uint32_t flag_arg);
extern void rasterizer_model_draw_restore_states(void);
extern void rasterizer_shader_environment_draw_dispatch(int32_t a, int32_t b, int32_t c, int32_t d, int32_t e, int32_t f);
extern void rasterizer_transparent_geometry_group_build(int32_t a, int32_t b, int32_t c, int32_t d, int32_t e, int32_t f, int32_t g, void *h);
extern int8_t scenario_location_get_water_and_weather(int32_t *a, void *b);
extern double sqrt(double x);
}

/**
 * Creates the flag data array with room for two flags.
 *
 * Original register convention: none (no parameters).
 *
 * @address 0x004fb4d0
 */
void halo::objects::FlagSystem::initialize()
{
    flag_data = game_state_new((char *)"flag", k_maximum_flags, 0x16bc  );
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
    datum_index handle = (datum_index)0xffffffff;

    if (flag_tag != (datum_index)0xffffffff) {
        Flag *tag = (Flag *)halo::cache::globals().tag_instances[flag_tag & 0xffff].data;

        handle = halo::memory::datum_new(flag_data);
        if (handle != (datum_index)0xffffffff) {
            flag *entry = &((flag *)flag_data->data)[handle & 0xffff];

            if (tag->height * tag->width < (int32_t)k_maximum_flag_cloth_vertices &&
                tag->width < 0x28 && *(int32_t *)&tag->blue_flag_shader.tag_id != -1) {
                int16_t row;

                entry->definition_tag = flag_tag;
                entry->invalid = 0;
                entry->unknown_03 = 0;
                entry->object_index = (datum_index)0xffffffff;
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

                flag_cloth_mark_border_cells(entry);
                flag_cloth_init_shape_constraints(entry);
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

            raw = *(int16_t *)((uint8_t *)tag->attachment_points.pointer + point_index * 0x34);
            if (raw < 0) {
                clamped = 0;
            } else {
                int16_t remaining = tag->height - col;
                clamped = (remaining < raw) ? remaining : raw;
            }
            half = clamped & ~1;
            split = half / 2;

            flag_cloth_stamp_region_split_flags(0, tag, entry, col, split, 4);
            flag_cloth_stamp_region_split_flags(0, tag, entry, col + split, split, 5);

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
            flag_cloth_stamp_region_split_flags((int16_t)edge_count, tag, entry, 0, si, 3);
        } else if (shape == 4) {
            flag_cloth_stamp_region_split_flags((int16_t)edge_count, tag, entry, 0, si, 2);
        } else if (shape == 1) {
            flag_cloth_stamp_region_split_flags((int16_t)edge_count, tag, entry, 0, si, 2);
            flag_cloth_stamp_region_split_flags((int16_t)edge_count, tag, entry, si, si, 3);
        } else if (shape == 2) {
            flag_cloth_stamp_region_split_flags((int16_t)edge_count, tag, entry, 0, si, 3);
            flag_cloth_stamp_region_split_flags((int16_t)edge_count, tag, entry, si, si, 2);
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
    uint8_t *self = (uint8_t *)flag_data->data + (flag_index & 0xffff) * 0x16bc;
    Flag *tag = (Flag *)halo::cache::globals().tag_instances[*(datum_index *)(self + 0xc) & 0xffff].data;

    *(datum_index *)(self + 8) = object_index;
    if (*(int16_t *)(self + 6) > 5 || self[3] == 0) {
        flag_cloth_update((flag *)self, tag, 5.0f);
        self[3] = 1;
    }
    *(int16_t *)(self + 6) = 0;
    if (self[2] == 0) {
        flag_render((uint32_t *)self, (uint32_t *)arg3, tag, (uint8_t *)arg4);
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

        if (current == (datum_index)0xffffffff) {
            return;
        }

        {
            flag *entry = (flag *)((uint8_t *)flags->data + (current & 0xffff) * flags->size);
            datum_index object_index = entry->object_index;
            void *tag_data = halo::cache::globals().tag_instances[entry->definition_tag & 0xffff].data;
            int16_t *update_counter = (int16_t *)((uint8_t *)entry + 6);

            *update_counter = *update_counter + 1;
            if (object_index != (datum_index)0xffffffff && *update_counter < 5 && dt != 0.0f) {
                flag_cloth_update(entry, (Flag *)tag_data, dt);
                flags = flag_data;
            }
        }

        next_index = (int32_t)(int16_t)((current & 0xffff) + 1);
        current = (datum_index)0xffffffff;
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
    int retracting = (*((uint8_t *)entry + 4) == 0);
    bsp_leaf_reference node_ref;
    uint32_t physics_a = 0, physics_b = 0;

    real_point3d marker_positions[8];
    uint8_t row_table[488];
    int16_t row_start_scratch[8];
    int16_t column_marker_index[40];
    int8_t moving;

    flag_pole_get_marker_positions(entry, &node_ref, marker_positions, row_table,
                                    row_start_scratch, column_marker_index, tag);
    moving = scenario_location_get_water_and_weather((int32_t *)&physics_a, (void *)&physics_b);

    if (entry->invalid == 0) {

        int16_t neighbour_dcol[3] = { -1, 0, 0 };
        int16_t neighbour_drow[3] = { 0, 1, -1 };
        float corner_length[3];
        int32_t n0;

        for (n0 = 0; n0 < 3; n0++) {
            float fx = (float)neighbour_dcol[n0] * tag->cell_width;
            float fy = (float)neighbour_drow[n0] * tag->cell_height;
            corner_length[n0] = (float)sqrt(fx * fx + fy * fy);
        }

        if (tag->width > 0) {
            int32_t row_step = ((int16_t)retracting != 0) ? 1 : -1;
            int16_t col = 0;

            do {
                int16_t row_cursor;
                int16_t bound;

                row_cursor = (col == 0) ? (int16_t)(tag->height - 1) : 0;

                for (;;) {
                    real_point3d *vertex;
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

                    vertex = (real_point3d *)((uint8_t *)entry + 0x1c + (tag->height * col + row_cursor) * 0x18);
                    contributor_count = 0;
                    mode = 1;

                    if (!moving) {
                        wind_scale = *(float *)((uint8_t *)halo::cache::globals().tag_instances[tag->physics.tag_id.index].data + 0x24) *
                                     tag->wind_noise * 0.0004f;
                    } else {
                        mode = 3;
                        wind_scale = *(float *)((uint8_t *)halo::cache::globals().tag_instances[tag->physics.tag_id.index].data + 0x28) *
                                     tag->wind_noise * 0.00016f;
                    }

                    halo::math::globals().effect_random_seed = halo::math::globals().effect_random_seed * 0x19660dU + 0x3c6ef35fU;
                    {
                        int16_t idx = (int16_t)(((halo::math::globals().effect_random_seed >> 16) *
                                                  (uint32_t)halo::math::globals().sphere_point_table_count) >> 16);
                        real_point3d *dir = &halo::math::globals().sphere_point_table[idx];
                        wind_dir.i = dir->x * wind_scale;
                        wind_dir.j = dir->y * wind_scale;
                        wind_dir.k = dir->z * wind_scale;
                    }

                    target = *vertex;

                    halo::physics::point_physics_tick((real_vector3d *)((uint8_t *)vertex + 0x0c)  , mode,
                        halo::cache::globals().tag_instances[tag->physics.tag_id.index].data, &node_ref,
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
                                real_point3d *nvp = (real_point3d *)((uint8_t *)entry + 0x1c +
                                                                      (tag->height * ncol + nrow) * 0x18);
                                float dx = target.x - nvp->x;
                                float dy = target.y - nvp->y;
                                float dz = target.z - nvp->z;
                                float dist = (float)sqrt(dx * dx + dy * dy + dz * dz);

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
                        real_vector3d *velocity_slot = (real_vector3d *)((uint8_t *)vertex + 0x0c);
                        velocity_slot->i = (target.x - vertex->x) * inv_dt;
                        velocity_slot->j = (target.y - vertex->y) * inv_dt;
                        velocity_slot->k = (target.z - vertex->z) * inv_dt;
                        *vertex = target;
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
        object_get_node_local_transform(entry->object_index,
            (char *)((uint8_t *)tag->attachment_points.pointer + i * 0x34 + 0x14),
            &marker, 1);
        marker_positions[i] = marker.node_transform.position;
    }

    {

        int32_t node_index = halo::physics::bsp3d_node_find_leaf(0, (ModelCollisionGeometryBSP *)halo::physics::globals().collision_bsp, &marker_positions[0]);

        node_ref->leaf_index = node_index;
        if (node_index == -1) {
            node_ref->cluster_index = -1;
        } else {
            node_ref->cluster_index = *(int16_t *)((uint8_t *)global_structure_bsp->leaves.pointer +
                                                 (uint32_t)(node_index & 0x7fffffff) * 0x10 + 8);
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

                raw = *(int16_t *)((uint8_t *)tag->attachment_points.pointer + point_index * 0x34);
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
            int32_t tx = __ftol((double)(new_position.x - old_position.x));
            int skip = (tx < 0 ? -tx : tx) <= 1;

            if (skip) {
                int32_t ty = __ftol((double)(new_position.y - old_position.y));
                skip = (ty < 0 ? -ty : ty) <= 1;
                if (skip) {
                    int32_t tz = __ftol((double)(new_position.z - old_position.z));
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
                            (real_point3d *)((uint8_t *)entry + 0x1c + (tag->height * r + c) * 0x18);
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

/**
 * Builds the flag's cloth mesh (vertex normals and index list) and submits it to the rasterizer. Unreferenced in the
 * retail binary; kept as a close transliteration with a guessed signature.
 *
 * Original register convention: UNSURE throughout, see file header; this signature is a guess with no verified.
 *
 * @address 0x004fc350
 */
void halo::objects::FlagSystem::render(uint32_t *entry, uint32_t *submission_block, Flag *tag,
    uint8_t *second_geometry)
{
    int16_t width = ((struct Flag *)tag)->width;
    int16_t height = ((struct Flag *)tag)->height;
    int32_t model_context;
    void *normal_buffer;
    void *index_buffer;
    int16_t col, row;
    int16_t triangle_count = 0;
    int16_t index_count = 0;
    float inv_width_minus1 = 1.0f / (float)(width - 1);
    float inv_height_minus1 = 1.0f / (float)(height - 1);

    model_context = rasterizer_dynamic_index_cache_reserve();
    if (model_context == -1 || rasterizer_dynamic_vertex_cache_reserve() == -1) {
        return;
    }
    normal_buffer = rasterizer_dynamic_vertex_cache_lock();
    index_buffer = rasterizer_dynamic_index_slot_lock();

    for (col = 0; col < width; col++) {
        for (row = 0; row < height; row++) {
            int16_t col_clamped = (col >= width - 1) ? col - 1 : col;
            int16_t row_clamped = (row >= height - 1) ? row - 1 : row;
            int32_t base_index = col_clamped * height + row_clamped;
            uint32_t *next_row_vertex = entry + ((col_clamped + 1) * height + row_clamped) * 6 + 7;
            uint32_t *base_vertex = entry + base_index * 6 + 7;
            float ex_x = *(float *)next_row_vertex - *(float *)(entry + base_index * 6 + 7);
            float ex_y = ((float *)next_row_vertex)[1] - *(float *)(entry + base_index * 6 + 8);
            float ex_z = ((float *)next_row_vertex)[2] - *(float *)(entry + base_index * 6 + 9);
            float ey_x = *(float *)(entry + base_index * 6 + 0xd) - *(float *)(entry + base_index * 6 + 7);
            float ey_y = *(float *)(entry + base_index * 6 + 0xe) - *(float *)(entry + base_index * 6 + 8);
            float ey_z = *(float *)(entry + base_index * 6 + 0xf) - *(float *)(entry + base_index * 6 + 9);
            uint32_t *out = (uint32_t *)((uint8_t *)normal_buffer + triangle_count * 0x44);
            float nx = ey_z * ex_y - ey_y * ex_z;
            float ny = ey_y * ex_x - ey_z * ex_x;
            float nz = ey_x * ex_x - ey_y * ex_x;

            float len;

            ((float *)out)[3] = nx;
            ((float *)out)[4] = ny;
            ((float *)out)[5] = nz;
            len = (float)sqrt(nz * nz + ny * ny + nx * nx);
            if (len >= 0.0001f || len <= -0.0001f) {
                float inv = 1.0f / len;
                ((float *)out)[3] = inv * nx;
                ((float *)out)[4] = inv * ny;
                ((float *)out)[5] = inv * nz;
            }

            *(float *)out = *(float *)base_vertex;
            ((float *)out)[1] = ((float *)base_vertex)[1];
            ((float *)out)[2] = ((float *)base_vertex)[2];
            ((float *)out)[0xc] = (float)col * inv_width_minus1;
            ((float *)out)[0xd] = (float)row * inv_height_minus1;
            *(int16_t *)(out + 0xe) = 0;
            *(int16_t *)((uint8_t *)out + 0x3a) = 0;
            out[0xf] = 0x3f000000;
            out[0x10] = 0x3f000000;

            triangle_count = triangle_count + 1;
        }
    }

    col = 0;
    row = 0;
    if (width != 1 && width - 1 >= 0) {
        int32_t r;
        for (r = 0; r < width - 1; r++) {
            int32_t c;
            for (c = 0; c < height - 1; c++) {
                uint16_t split = *(uint16_t *)((uint8_t *)entry + (r * (height - 1) + c) * 2 + 0x1534);
                int16_t *tri;

                switch (split) {
                case 0:
                    tri = (int16_t *)((uint8_t *)index_buffer + index_count * 6);
                    tri[0] = height * row + col;
                    tri[1] = height * (row + 1) + col;
                    index_count++;
                    tri[2] = height * row + 1 + col;
                    tri = (int16_t *)((uint8_t *)index_buffer + index_count * 6);
                    tri[0] = height * row + 1 + col;
                    tri[1] = height * (row + 1) + col;
                    tri[2] = height * (row + 1) + 1 + col;
                    index_count++;
                    col++;
                    continue;
                case 2:
                    tri = (int16_t *)((uint8_t *)index_buffer + index_count * 6);
                    tri[0] = height * row + col;
                    tri[1] = height * row + 1 + col;
                    tri[2] = (row + 1) * height + col;
                    break;
                case 3:
                    tri = (int16_t *)((uint8_t *)index_buffer + index_count * 6);
                    tri[0] = height * row + col;
                    tri[1] = height * row + 1 + col;
                    tri[2] = (row + 1) * height + 1 + col;
                    break;
                case 4:
                    tri = (int16_t *)((uint8_t *)index_buffer + index_count * 6);
                    tri[0] = height * row + col;
                    tri[1] = height * (row + 1) + 1 + col;
                    tri[2] = (row + 1) * height;
                    break;
                case 5:
                    tri = (int16_t *)((uint8_t *)index_buffer + index_count * 6);
                    tri[0] = height * row + 1 + col;
                    tri[1] = height * (row + 1) + 1 + col;
                    tri[2] = (row + 1) * height;
                    break;
                default:
                    col++;
                    continue;
                }
                index_count++;
                col++;
            }
            row++;
            col = 0;
        }
    }

    {
        void ***device = (void ***)flag_render_device_slot;
        (*(void (__stdcall **)(void *))((uint8_t *)(*device)[0] + 0x30 * 0))(device);
    }

    {
        Flag *fallback_tag = (Flag *)halo::cache::globals().tag_instances[  0].data;
        int32_t stride = height;
        float *v0 = (float *)(second_geometry + 4 + stride * 0x18);
        float *v1 = (float *)(second_geometry + 0x1c + stride * (width - 1) * 0x18);
        float *v2 = (float *)(second_geometry + 4 + width * stride * 0x18);
        float cx = (v2[0] + v0[0] + v1[0] + *(float *)(second_geometry + 0x1c)) * 0.25f;
        float cy = (v2[1] + v0[1] + v1[1] + *(float *)(second_geometry + 0x20)) * 0.25f;
        float cz = (v2[2] + v0[2] + v1[2] + *(float *)(second_geometry + 0x24)) * 0.25f;

        (void)fallback_tag;
        rasterizer_model_draw_prepare_states(0);
        rasterizer_shader_environment_draw_dispatch(0, 0, 0, row - 1, index_count, 0);

        rasterizer_model_draw_restore_states();
    }
}
