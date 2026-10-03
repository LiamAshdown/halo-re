#include "halo/objects/antenna.hpp"
#include "rasterizer.h"
#include "render.h"
#include <stdint.h>

extern "C" {
extern int32_t __ftol(double);
extern void antenna_apply_marker_delta(real_vector3d *out_forward, real_point3d *out_position, antenna *ant, Antenna *antenna_tag, bsp_leaf_reference *node_ref);
extern data_array *antenna_data;
extern void antenna_render_geometry(Antenna *antenna_tag, antenna *ant);
extern uint8_t antenna_sprite_shader[];
extern void antenna_update_physics(antenna *ant, Antenna *antenna_tag, float dt);
extern void *bitmap_group_get_bitmap_data(void);
extern uint32_t bsp3d_node_find_leaf(int32_t node_index, ModelCollisionGeometryBSP *bsp, real_point3d *point);
extern void build_sprite(build_sprite_data *data, int16_t sequence_index, int16_t sprite_index, int16_t mode, real_point3d *origin, real_vector3d *direction, float rotation, float scale, ColorARGB *color, float fade, uint32_t flags);
extern void build_sprites_end(build_sprite_data *data);
extern double cos(double x);
extern void data_delete_all(data_array *array);
extern void datum_delete(data_array *array, datum_index index);
extern datum_index datum_new(data_array *array);
extern datum_index datum_next(int16_t after_index, data_array *array);
extern uint32_t effect_random_seed;
extern data_array *game_state_new(char *name, int16_t maximum_count, int16_t element_size);
extern ModelCollisionGeometryBSP *global_collision_bsp;
extern real_vector3d *global_left3d_pointer;
extern ScenarioStructureBSP *global_structure_bsp;
extern real_point3d *global_zero_vector3d_pointer;
extern void matrix4x3_transform_vector(real_vector3d *out, real_vector3d *v, real_matrix4x3 *m);
extern int32_t object_get_node_local_transform(uint32_t object_index, char *marker_name, object_marker *marker, uint32_t flags);
extern uint32_t point_physics_tick(real_vector3d *velocity , uint32_t mode, void *physics_tag_data, bsp_leaf_reference *node_ref, uint32_t flags, real_point3d *position, real_vector3d *wind_direction, void *unused_c, void *unused_d, float damping_constant, float dt);
extern double sin(double x);
extern double sqrt(double x);
extern tag_instance *tag_instances;
extern real vector3d_angle_between_4cd4f0(real_vector3d *a, real_vector3d *b);
extern real vector3d_normalize_with_length(real_vector3d *v);
extern void vector3d_rotate_about_axis(real_vector3d *v, real_vector3d *axis, real sin_angle, real cos_angle);
}

/**
 * Creates the antenna data array with room for twelve antennas.
 *
 * Original register convention: no parameters.
 *
 * @address 0x004faa20
 */
void halo::objects::AntennaSystem::initialize()
{
    antenna_data = game_state_new((char *)"antenna", k_maximum_antennas, 0x2bc  );
}

/**
 * Disposes the antenna data array.
 *
 * Original register convention: no parameters.
 *
 * @address 0x004faa40
 */
void halo::objects::AntennaSystem::dispose()
{
    antenna_data->valid = 1;
    data_delete_all(antenna_data);
}

/**
 * Clears the disposing flag of the antenna data array after a dispose pass.
 *
 * Original register convention: no parameters.
 *
 * @address 0x004faa60
 */
void halo::objects::AntennaSystem::clear_disposing_flag()
{
    antenna_data->valid = 0;
}

/**
 * Re-reads the antenna data array pointer from the game state after a state restore.
 *
 * Original register convention: no parameters.
 *
 * @address 0x004faa70
 */
void halo::objects::AntennaSystem::reset_data_pointer()
{
    if (antenna_data != 0) {
        antenna_data = 0;
    }
}

/**
 * Allocates an antenna datum for an Antenna tag, initialising its vertex chain, and returns its handle.
 *
 * Original register convention: antenna tag as the sole parameter, per Ghidra's own "antenna_new(uint param_1)" with
 * no in_REG markers.
 *
 * @address 0x004faa90
 */
datum_index halo::objects::AntennaSystem::create(datum_index antenna_tag)
{
    datum_index handle = k_datum_index_none;

    if (antenna_tag != k_datum_index_none) {
        Antenna *tag = (Antenna *)tag_instances[antenna_tag & 0xffff].data;
        handle = datum_new(antenna_data);

        if (handle != k_datum_index_none) {
            antenna *ant = (antenna *)antenna_data->data + (handle & 0xffff);
            int32_t tag_vertex_count = tag->vertices.count;
            AntennaVertex *tag_vertices = (AntennaVertex *)tag->vertices.pointer;
            real_point3d position = { 0.0f, 0.0f, 0.0f };
            int32_t i;

            ant->unknown_04 = 0;
            ant->degenerate = tag_vertex_count < 2;
            ant->definition_tag = antenna_tag;
            ant->object_index = k_datum_index_none;
            ant->update_counter = 0;
            ant->previous_marker_position.x = 0.0f;
            ant->previous_marker_position.y = 0.0f;
            ant->previous_marker_position.z = 0.0f;

            for (i = 0; i < tag_vertex_count; i++) {
                antenna_vertex *vertex = &ant->vertices[i];
                AntennaVertex *tag_vertex = &tag_vertices[i];

                vertex->position = position;
                vertex->velocity.i = 0.0f;
                vertex->velocity.j = 0.0f;
                vertex->velocity.k = 0.0f;
                vertex->texture_scale = 0.0f;
                vertex->step_count = 0;

                if (tag->bitmaps.tag_id.index != 0xffff) {
                    Bitmap *bitmap = (Bitmap *)tag_instances[tag->bitmaps.tag_id.index & 0xffff].data;
                    int16_t sequence_index = tag_vertex->sequence_index;

                    if ((sequence_index >= 0) && (sequence_index < (int16_t)bitmap->bitmap_group_sequence.count)) {
                        BitmapGroupSequence *sequence = (BitmapGroupSequence *)bitmap->bitmap_group_sequence.pointer + sequence_index;
                        if (sequence->sprites.count != 0) {
                            BitmapGroupSprite *sprite = (BitmapGroupSprite *)sequence->sprites.pointer;
                            void *bitmap_data = bitmap_group_get_bitmap_data();
                            if (bitmap_data != 0) {

                                float width = (float)*(int16_t *)((uint8_t *)bitmap_data + 4);
                                float spacing = (float)*(int16_t *)&((struct Bitmap *)bitmap)->sprite_spacing;
                                vertex->texture_scale = tag_vertex->length /
                                    (((sprite->right - sprite->left) * width - (spacing + spacing)) - 1.0f);
                            }
                        }
                    }
                }

                position.x += tag_vertex->offset.x;
                position.y += tag_vertex->offset.y;
                position.z += tag_vertex->offset.z;
            }

            {
                antenna_vertex *tip = &ant->vertices[tag_vertex_count];
                tip->position = position;
                tip->velocity.i = 0.0f;
                tip->velocity.j = 0.0f;
                tip->velocity.k = 0.0f;
            }
        }
    }

    return handle;
}

/**
 * Frees an antenna datum from the antenna data array.
 *
 * Original register convention: stack -> antenna_index (cdecl).
 *
 * @address 0x004fac80
 */
void halo::objects::AntennaSystem::destroy(datum_index antenna_index)
{
    datum_delete(antenna_data, antenna_index);
}

/**
 * Widget render hook for an antenna: looks up the owning object and the antenna datum and submits its geometry.
 *
 * Original register convention: stack -> object_index, antenna_index (cdecl).
 *
 * @address 0x004fac90
 */
void halo::objects::AntennaSystem::render_callback(datum_index object_index, datum_index antenna_index)
{
    antenna *self = (antenna *)((uint8_t *)antenna_data->data + (antenna_index & 0xffff) * 0x2bc);
    Antenna *tag = (Antenna *)tag_instances[self->definition_tag & 0xffff].data;

    if (self->degenerate) {
        return;
    }
    self->object_index = object_index;
    if (self->update_counter > 5) {
        antenna_update_physics(self, tag, 0.05f);
        antenna_update_physics(self, tag, 0.05f);
        antenna_update_physics(self, tag, 0.05f);
    }
    self->update_counter = 0;
    antenna_render_geometry(tag, self);
}

/**
 * Runs the per-tick update of every live antenna.
 *
 * Original register convention: dt is the sole stack parameter (Ghidra's own "antennas_update(float param_1)").
 *
 * @address 0x004fad20
 */
void halo::objects::AntennaSystem::update(float dt)
{
    datum_index handle = datum_next(-1, antenna_data);

    while (handle != k_datum_index_none) {
        antenna *ant = (antenna *)antenna_data->data + (handle & 0xffff);

        if (ant->degenerate == 0) {
            ant->update_counter = ant->update_counter + 1;
            if ((ant->object_index != k_datum_index_none) && (ant->update_counter < 5)) {
                Antenna *tag = (Antenna *)tag_instances[ant->definition_tag & 0xffff].data;
                float clamped_dt = (dt <= 0.06666667f) ? dt : 0.06666667f;
                antenna_update_physics(ant, tag, clamped_dt);
            }
        }

        handle = datum_next((int16_t)handle, antenna_data);
    }
}

/**
 * Advances the damped spring simulation of the antenna vertices by dt seconds against the supplied Antenna tag.
 *
 * @address 0x004fae10
 */
void halo::objects::AntennaView::update_physics(Antenna *antenna_tag, float dt)
{
    antenna *ant = self;
    real_vector3d marker_forward;
    real_point3d marker_position;
    bsp_leaf_reference node_ref;

    antenna_apply_marker_delta(&marker_forward, &marker_position, ant, antenna_tag, &node_ref);

    if (ant->degenerate == 0 && dt > 0.0f) {
        int32_t vertex_count = antenna_tag->vertices.count;

        if (vertex_count != -1 && vertex_count + 1 >= 0) {
            AntennaVertex *tag_vertices = (AntennaVertex *)antenna_tag->vertices.pointer;
            float inv_dt = 1.0f / dt;
            real_point3d anchor = { 0.0f, 0.0f, 0.0f };

            int32_t runtime_index = 0;
            int16_t completed = 0;

            do {
                antenna_vertex *vertex = &ant->vertices[runtime_index];
                int32_t tag_index = runtime_index;
                AntennaVertex *tag_vertex;
                float blend, one_minus_blend;
                real_point3d new_position;
                real_vector3d bend_delta;

                if (tag_index == vertex_count) {

                    tag_index = vertex_count - 1;
                }
                tag_vertex = &tag_vertices[tag_index];
                blend = antenna_tag->spring_strength_coefficient * tag_vertex->spring_strength_coefficient;
                vertex->step_count += 1;

                if (completed == 0) {
                    new_position = marker_position;
                    bend_delta = marker_forward;
                } else {
                    float rest_scale;

                    new_position = vertex->position;

                    point_physics_tick(&vertex->velocity, 0,
                        tag_instances[antenna_tag->physics.tag_id.index].data,
                        &node_ref, 0xffffffff, &new_position, 0, 0, 0, 0.02f, dt);

                    {
                        float dx = new_position.x - anchor.x;
                        float dy = new_position.y - anchor.y;
                        float dz = new_position.z - anchor.z;
                        rest_scale = (float)(tag_vertex->length / sqrt(dx * dx + dy * dy + dz * dz));
                        one_minus_blend = 1.0f - blend;
                        new_position.x = bend_delta.i * blend + (rest_scale * dx + anchor.x) * one_minus_blend;

                        new_position.y = bend_delta.j * blend + one_minus_blend * (rest_scale * dy + anchor.y);
                        new_position.z = bend_delta.k * blend + one_minus_blend * (rest_scale * dz + anchor.z);
                    }
                    bend_delta.i = new_position.x - anchor.x;
                    bend_delta.j = new_position.y - anchor.y;
                    bend_delta.k = new_position.z - anchor.z;
                }

                {
                    real_vector3d axis;
                    real_vector3d offset;
                    float length;
                    float angle, s, c;

                    axis.i = bend_delta.k * 0.0f - bend_delta.j;
                    axis.j = bend_delta.i - bend_delta.k * 0.0f;
                    axis.k = bend_delta.j * 0.0f - bend_delta.i * 0.0f;

                    length = vector3d_normalize_with_length(&axis);
                    if (length == 0.0f) {
                        axis = *global_left3d_pointer;
                    }

                    offset.i = tag_vertex->offset.x;
                    offset.j = tag_vertex->offset.y;
                    offset.k = tag_vertex->offset.z;

                    {
                        real_vector3d world_up_z = { 0.0f, 0.0f, 1.0f };

                        angle = vector3d_angle_between_4cd4f0(&bend_delta, &world_up_z);
                    }
                    s = (real)sin((double)angle);
                    c = (real)cos((double)angle);
                    vector3d_rotate_about_axis(&offset, &axis, s, c);

                    anchor.x = new_position.x;
                    anchor.y = new_position.y;
                    anchor.z = new_position.z;

                    completed = completed + 1;
                    runtime_index = completed;

                    {
                        float old_x = vertex->position.x;
                        float old_y = vertex->position.y;
                        float old_z = vertex->position.z;

                        vertex->velocity.i = (new_position.x - old_x) * inv_dt;
                        vertex->velocity.j = (new_position.y - old_y) * inv_dt;
                        vertex->velocity.k = (new_position.z - old_z) * inv_dt;
                        vertex->position = new_position;
                    }

                    bend_delta.i = offset.i + new_position.x;
                    bend_delta.j = offset.j + new_position.y;
                    bend_delta.k = offset.k + new_position.z;
                }

                vertex_count = antenna_tag->vertices.count;
            } while (runtime_index < vertex_count + 1);
        }
    }
}

/**
 * Applies the movement of the antenna's attached marker since the last tick to the antenna chain, producing the
 * forward vector and base position the vertex simulation starts from.
 *
 * @address 0x004fb1c0
 */
void halo::objects::AntennaView::apply_marker_delta(real_vector3d *out_forward, real_point3d *out_position,
    Antenna *antenna_tag, bsp_leaf_reference *node_ref)
{
    antenna *ant = self;
    object_marker marker;

    object_get_node_local_transform(ant->object_index, (char *)antenna_tag, &marker, 1);
    *out_position = marker.node_transform.position;
    *out_forward = marker.node_transform.forward;

    {

        int32_t node_index = bsp3d_node_find_leaf(0, (ModelCollisionGeometryBSP *)global_collision_bsp, &marker.node_transform.position);

        node_ref->leaf_index = node_index;
        if (node_index == -1) {
            node_ref->cluster_index = -1;
        } else {
            node_ref->cluster_index = *(int16_t *)((uint8_t *)global_structure_bsp->leaves.pointer +
                                                 (uint32_t)(node_index & 0x7fffffff) * 0x10 + 8);
        }
    }

    {
        real_vector3d delta;
        delta.i = out_position->x - ant->previous_marker_position.x;
        delta.j = out_position->y - ant->previous_marker_position.y;
        delta.k = out_position->z - ant->previous_marker_position.z;

        int32_t tx = __ftol((double)delta.i);
        int skip = (tx < 0 ? -tx : tx) <= 1;
        if (skip) {
            int32_t ty = __ftol((double)delta.j);
            skip = (ty < 0 ? -ty : ty) <= 1;
            if (skip) {
                int32_t tz = __ftol((double)delta.k);
                skip = (tz < 0 ? -tz : tz) <= 1;
            }
        }

        if (!skip) {
            int32_t vertex_count = antenna_tag->vertices.count;
            int32_t i;

            for (i = 0; i <= vertex_count; i++) {
                ant->vertices[i].position.x += delta.i;
                ant->vertices[i].position.y += delta.j;
                ant->vertices[i].position.z += delta.k;
            }
        }
    }

    ant->previous_marker_position = *out_position;
}

/**
 * Builds the camera-facing sprite geometry for the antenna chain from its tag vertices and current simulated
 * positions.
 *
 * Original register convention: EDI -> antenna_tag, stack -> ant.
 *
 * @address 0x004fb340
 */
void halo::objects::AntennaView::render_geometry(Antenna *antenna_tag)
{
    antenna *ant = self;
    uint8_t *tag = (uint8_t *)antenna_tag;
    int32_t count = *(int32_t *)&((struct Antenna *)tag)->vertices.count;
    build_sprite_data data;
    float fade;
    int16_t i;

    if (count == 0) {
        return;
    }
    fade = (100.0f - ((struct Antenna *)tag)->cutoff_pixels) / (((struct Antenna *)tag)->falloff_pixels - ((struct Antenna *)tag)->cutoff_pixels);
    if (fade < 0.0f) {
        fade = 0.0f;
    } else if (fade > 1.0f) {
        fade = 1.0f;
    }
    data.bitmap_group_index = *(datum_index *)&((struct Antenna *)tag)->bitmaps.tag_id;
    data.maximum_sprite_count = (int16_t)count;
    data.shader = (uint32_t)(uintptr_t)antenna_sprite_shader;
    data.sprite_count = 0;
    data.flags = 4;
    data.centroid = *global_zero_vector3d_pointer;
    data.group_count = 0;

    for (i = 0; i < *(int32_t *)&((struct Antenna *)tag)->vertices.count; i++) {
        antenna_vertex *vertex = &ant->vertices[i];
        uint8_t *tag_vertex = *(uint8_t **)&((struct Antenna *)tag)->vertices.pointer + i * 0x80;
        real_vector3d direction;
        ColorARGB color;

        direction.i = ant->vertices[i + 1].position.x - vertex->position.x;
        direction.j = ant->vertices[i + 1].position.y - vertex->position.y;
        direction.k = ant->vertices[i + 1].position.z - vertex->position.z;
        color = *(ColorARGB *)(tag_vertex + 0x2c);
        if (vertex->texture_scale != 0.0f && fade > 0.0f) {
            build_sprite(&data, *(int16_t *)(tag_vertex + 0x28), 0, 1, &vertex->position, &direction, 0.0f,
                vertex->texture_scale, &color, fade, 0);
        }
    }
    build_sprites_end(&data);
}

namespace {
static void (*const build_sprites_end__as_antenna_render_wire)(void) = reinterpret_cast<void (*)(void)>(&build_sprites_end);
static void (*const build_sprite__as_antenna_render_wire)(int32_t, uint8_t *, real_vector3d *, int32_t, void *, ColorARGB *, float, int32_t) = reinterpret_cast<void (*)(int32_t, uint8_t *, real_vector3d *, int32_t, void *, ColorARGB *, float, int32_t)>(&build_sprite);
}

/**
 * Builds the wire segments between antenna vertices as sprites. Unreferenced in the retail binary and kept as a
 * literal transliteration; its signature is a best-effort reconstruction.
 *
 * Original register convention: not observable, because the retail binary never calls this function.
 *
 * @address 0x004fb3e0
 */
void halo::objects::AntennaView::render_wire(uint32_t widget_flags, float scale, Antenna *antenna_tag)
{
    antenna *ant = self;
    if ((widget_flags & 0x44) != 0) {
        AntennaVertex *tag_vertices = (AntennaVertex *)antenna_tag->vertices.pointer;
        int16_t i = 0;

        do {
            uint8_t *vertex = (uint8_t *)ant + i * 0x20;

            AntennaVertex *tag_vertex = &tag_vertices[i];
            real_vector3d segment;

            segment.i = *(float *)((uint8_t *)ant + i * 0x20 + 0x3c) - *(float *)(vertex + 0x1c);
            segment.j = *(float *)(vertex + 0x40) - *(float *)(vertex + 0x20);
            segment.k = *(float *)(vertex + 0x44) - *(float *)(vertex + 0x24);

            if (*(float *)(vertex + 0x34) != 0.0f && scale > 0.0f) {
                ColorARGB color = tag_vertex->color;
                build_sprite__as_antenna_render_wire(1, vertex + 0x1c, &segment, 0,
                                             *(void **)(vertex + 0x34), &color, scale, 0);

            }

            i = i + 1;
        } while (i < antenna_tag->vertices.count);
    }
    build_sprites_end__as_antenna_render_wire();
}

/**
 * Adds a randomised displacement scaled by the amplitude to the antenna tip position, transformed by the supplied
 * matrix.
 *
 * @address 0x004fef40
 */
void halo::objects::AntennaSystem::tip_jitter(real_vector3d *amplitude, real_point3d *position, real_matrix4x3 *m)
{
    uint32_t s1 = effect_random_seed * 0x19660dU + 0x3c6ef35fU;
    uint32_t s2 = s1 * 0x19660dU + 0x3c6ef35fU;
    float rz = (float)(s1 >> 16) * 1.5259022e-05f;
    float ry, rx;

    effect_random_seed = s2 * 0x19660dU + 0x3c6ef35fU;
    ry = (float)(s2 >> 16) * 1.5259022e-05f;
    rx = (float)(effect_random_seed >> 16) * 1.5259022e-05f;

    {
        real_vector3d jitter;

        jitter.i = (rx + rx - 1.0f) * amplitude->i;
        jitter.j = (ry + ry - 1.0f) * amplitude->j;
        jitter.k = (rz + rz - 1.0f) * amplitude->k;
        matrix4x3_transform_vector(&jitter, &jitter, m);
        position->x = jitter.i + position->x;
        position->y = jitter.j + position->y;
        position->z = jitter.k + position->z;
    }
}
