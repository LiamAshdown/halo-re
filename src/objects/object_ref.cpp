#include "halo/objects/object_ref.hpp"
#include "halo/objects/flags.hpp"
#include "halo/core/flag_bits.hpp"
#include "halo/core/collision_flags.hpp"
#include "halo/core/lcg.hpp"
#include "halo/core/network_constants.hpp"
#include "halo/projectiles/api.hpp"
#include "halo/models/api.hpp"
#include "halo/scenario/api.hpp"
#include "game.h"
#include "units.h"
#include "networking.h"
#include "projectiles.h"
#include "halo/math/api.hpp"
#include "halo/memory/api.hpp"
#include "halo/cache/api.hpp"
#include "halo/structures/api.hpp"
#include "halo/physics/api.hpp"
#include "halo/cseries/api.hpp"
#include "halo/objects/api.hpp"

extern "C" {
extern int32_t __ftol();
extern char ai_marker_name_a[];
extern double atan2(double y, double x);
extern uint32_t bsp3d_node_find_leaf(int32_t node_index, ModelCollisionGeometryBSP *bsp, real_point3d *point);
extern datum_index *collideable_cluster_first;
extern void *collideable_cluster_partition;
extern void console_print_va(const char *format, ...);
extern double cos(double x);
extern ModelCollisionGeometryBSP *global_collision_bsp;
extern real_vector3d *global_origin3d_pointer;
extern player_globals *local_player_globals;
extern int32_t message_delta_encode_message(int32_t extra_eax, int32_t extra_edx, int32_t flag, int32_t message_type, int32_t changed_offset, void **items, int32_t type_offset, int32_t count, char force_changed);
extern int32_t network_index_cache_get(hash_table *table, int32_t key);
extern uint8_t network_message_scratch[halo::k_network_message_scratch_size];
extern void *network_object_index_cache;
extern network_server_globals *network_server;
extern uint8_t network_session_send_to_machine(int32_t machine_id, void *server, uint32_t status_bit, void *data, uint32_t body_bit_count, uint32_t reliable, uint32_t unknown_a, char force, uint32_t priority);
extern datum_index *noncollideable_cluster_first;
extern void *noncollideable_cluster_partition;
extern data_array *noncollideable_object_references;
extern data_array *object_data;
extern object_globals *object_globals_pointer;
extern uint8_t object_marker_scratch[0x6c];
extern datum_index *object_name_list;
extern data_array *player_data;
extern int32_t player_index_from_unit_index(datum_index object_index);
extern double sqrt(double x);
}

/**
 * Returns the object's bounding centre and bounding radius.
 *
 * Original register convention: real_point3d *out_center in EAX (in_EAX), object index in ECX.
 *
 * @address 0x004088e0
 */
void halo::objects::ObjectRef::get_center_of_mass_and_scale(real_point3d *out_center, float *out_radius)
{
    uint32_t object_index = handle;
    object_header *headers = (object_header *)object_data->data;
    object *obj = headers[halo::datum_slot(object_index)].data;

    *out_center = obj->bounding_center;
    *out_radius = obj->bounding_radius;
}

/**
 * Adds a velocity change to a parentless object and gives it a randomised angular velocity proportional to the
 * impulse.
 *
 * @address 0x004bef80
 */
void halo::objects::ObjectRef::apply_impulse_and_spin(real_vector3d *delta_velocity)
{
    uint32_t object_index = handle;
    object *obj = ((object_header *)object_data->data)[halo::datum_slot(object_index)].data;
    uint32_t draw;
    int16_t index;
    real_point3d sample;
    real magnitude, spin_scale;

    if (obj->parent_object != k_datum_index_none) {
        return;
    }

    obj->velocity.i = delta_velocity->i + obj->velocity.i;
    obj->velocity.j = obj->velocity.j + delta_velocity->j;
    obj->velocity.k = obj->velocity.k + delta_velocity->k;

    draw = halo::math::globals().random_seed_global * k_random_multiplier + k_random_increment;
    index = (int16_t)(((draw >> k_random_value_shift) * halo::math::globals().sphere_point_table_count) >> 16);
    halo::math::globals().random_seed_global = draw;
    sample = halo::math::globals().sphere_point_table[index];
    halo::math::globals().random_seed_global = halo::math::globals().random_seed_global * k_random_multiplier + k_random_increment;

    magnitude = (real)sqrt((double)(delta_velocity->j * delta_velocity->j +
        delta_velocity->k * delta_velocity->k + delta_velocity->i * delta_velocity->i));
    spin_scale = (real)(halo::math::globals().random_seed_global >> k_random_value_shift) * halo::k_unit_word_scale * magnitude * 1.5707964f;

    obj->angular_velocity.i = sample.x * spin_scale + obj->angular_velocity.i;
    obj->angular_velocity.j = sample.y * spin_scale + obj->angular_velocity.j;
    obj->angular_velocity.k = sample.z * spin_scale + obj->angular_velocity.k;

    halo::projectiles::projectile_compute_rotation(object_index);
    obj->flags = obj->flags & ~(uint32_t)_object_at_rest_bit;
}

/**
 * Walks the child chain of an object and prunes children recursively.
 *
 * Original register convention: datum_index object_index on the stack (param_1).
 *
 * @address 0x004edc10
 */
void halo::objects::ObjectRef::children_recurse_prune()
{
    uint32_t object_index = handle;
    object_header *headers = (object_header *)object_data->data;
    object *obj = headers[halo::datum_slot(object_index)].data;
    datum_index child_index = obj->first_child_object;

    while (child_index != k_datum_index_none) {
        object *child = headers[halo::datum_slot(child_index)].data;
        datum_index next_index = child->next_object;

        if (halo::objects::object_type_definitions_query_0x44(child_index) == 0) {
            halo::objects::object_children_recurse_prune(child_index);
        }

        child_index = next_index;
    }
}

/**
 * Returns the index of the player controlling the object or its occupant, or none.
 *
 * Original register convention: datum_index object_index in EAX (in_EAX).
 *
 * @address 0x004ee2e0
 */
int32_t halo::objects::ObjectRef::get_controlling_player_index()
{
    datum_index object_index = handle;
    object_header *headers = (object_header *)object_data->data;

    if (object_index == k_datum_index_none) {
        return -1;
    }

    for (;;) {
        int16_t index = (int16_t)object_index;

        if (-1 < index && index < object_data->maximum_count) {
            object_header *header = &headers[(uint16_t)index];

            if (header->identifier != 0) {
                int16_t salt = (int16_t)(object_index >> 0x10);

                if ((salt == 0 || header->identifier == salt) &&
                    (1 << (header->type & 0x1f) & _object_mask_unit) != 0 &&
                    header->data != 0) {
                    return player_index_from_unit_index(object_index);
                }
            }
        }

        object_index = headers[halo::datum_slot(object_index)].data->parent_object;
        if (object_index == k_datum_index_none) {
            return -1;
        }
    }
}

/**
 * Notifies an object that a player picked it up or refreshes its probe.
 *
 * Original register convention: datum_index player_index in EDI (unaff_EDI); the single stack argument is the object
 * handle (0x4ee415 mov ebx,[esp+0x14] / mov ecx,ebx into object_try_and_get).
 *
 * @address 0x004ee3c0
 */
void halo::objects::ObjectRef::notify_pickup_or_refresh_probe(datum_index player_index)
{
    uint32_t object_index = handle;
    int16_t index = (int16_t)player_index;

    if (player_index == k_datum_index_none || index < 0 || index >= player_data->maximum_count) {
        return;
    }

    {
        uint8_t *record = (uint8_t *)player_data->data + (int32_t)player_data->size * index;
        int16_t identifier = *(int16_t *)record;

        if (identifier == 0) {
            return;
        }

        {
            int16_t salt = (int16_t)(player_index >> 0x10);

            if (salt != 0 && identifier != salt) {
                return;
            }
        }

        {
            object *unit = halo::objects::object_try_and_get(object_index, _object_mask_unit);

            if (unit == 0 || unit->type != _object_type_biped || unit->network_role != 0 ||
                *(int32_t *)&((unit_object *)unit)->unit.controlling_player == (int32_t)player_index) {
                return;
            }

            if (*(int16_t *)(record + 2) == -1) {
                int32_t encoded_value;
                void *field_list[2];
                int32_t encoded;

                encoded_value = network_index_cache_get((hash_table *)&network_object_index_cache, (int32_t)object_index);
                field_list[0] = &encoded_value;
                field_list[1] = 0;
                encoded = message_delta_encode_message((int32_t)network_message_scratch, halo::k_network_message_scratch_size, 0, 0x32, 0, field_list, 0, 1, 0);
                network_session_send_to_machine((int8_t)record[0x64], network_server, 1, network_message_scratch,
                    (uint32_t)encoded, 0, 0, 0, 9);
                return;
            }

            halo::objects::object_throttled_multiplayer_sound_event();
        }
    }
}

/**
 * Zeroes the object's velocities and wakes it from rest.
 *
 * Original register convention: stack -> object_index.
 *
 * @address 0x004f5160
 */
void halo::objects::ObjectRef::reset_velocity_and_wake()
{
    uint32_t object_index = handle;
    object *obj = ((object_header *)object_data->data)[halo::datum_slot(object_index)].data;

    obj->velocity = *global_origin3d_pointer;
    obj->angular_velocity = *global_origin3d_pointer;
    obj->flags &= ~(uint32_t)_object_at_rest_bit;
    halo::objects::object_type_definitions_notify_0x50(object_index);
}

/**
 * Sets the object's forward, up and position and relinks it.
 *
 * @address 0x004f51c0
 */
void halo::objects::ObjectRef::set_position_and_orientation(real_vector3d *forward, real_vector3d *up,
    real_point3d *position)
{
    uint32_t object_index = handle;
    object *obj = ((object_header *)object_data->data)[halo::datum_slot(object_index)].data;

    halo::objects::object_unlink_cluster_or_notify_parent(object_index);

    if (position != 0) {
        obj->position = *position;
    }

    if (forward != 0) {
        obj->forward = *forward;

        if (up == 0) {
            real_vector3d perpendicular;

            perpendicular.i = forward->j;
            perpendicular.j = -forward->i;
            perpendicular.k = 0.0f;

            if (halo::math::vector3d_normalize_with_length(perpendicular) == 0.0f) {
                perpendicular.i = 1.0f;
                perpendicular.k = 0.0f;
                perpendicular.j = 0.0f;
            }

            halo::math::vector3d_cross_product(obj->up, *forward, perpendicular);
        } else {
            obj->up = *up;
        }
    }

    halo::objects::object_recalculate_bounding_radius(object_index);
    halo::objects::object_set_cluster_and_parent(object_index, 0);
}

/**
 * Sets the object's position and recalculates its derived state.
 *
 * Original register convention: position vector pointer in ESI (unaff_ESI), object index in EDI (unaff_EDI).
 *
 * @address 0x004f52c0
 */
void halo::objects::ObjectRef::set_position_and_recalculate(real_point3d *position)
{
    uint32_t object_index = handle;
    object *obj;
    bsp_leaf_reference location;
    int32_t leaf;

    leaf = (int32_t)halo::physics::bsp3d_node_find_leaf(0, (ModelCollisionGeometryBSP *)halo::physics::globals().collision_bsp, position);
    location.leaf_index = leaf;
    if (leaf == -1) {
        location.cluster_index = -1;
    } else {
        location.cluster_index =
            (int16_t)((ScenarioStructureBSPLeaf *)halo::scenario::globals().structure_bsp->leaves.pointer)[leaf & 0x7fffffff].cluster;
    }
    obj = ((object_header *)object_data->data)[halo::datum_slot(object_index)].data;
    halo::objects::object_unlink_cluster_or_notify_parent(object_index);
    *(real_point3d *)&((object *)obj)->position.x = *position;
    halo::objects::object_set_cluster_and_parent(object_index, &location);
    halo::objects::object_recalculate_bounding_radius(object_index);
}

/**
 * Sets the object's position and relinks it into the given BSP location.
 *
 * Original register convention: position vector pointer in ESI (unaff_ESI), object index in EDI (unaff_EDI).
 *
 * @address 0x004f5350
 */
void halo::objects::ObjectRef::set_position_and_relink(real_point3d *position, bsp_leaf_reference *location)
{
    uint32_t object_index = handle;
    object *obj = ((object_header *)object_data->data)[halo::datum_slot(object_index)].data;

    halo::objects::object_unlink_cluster_or_notify_parent(object_index);
    *(real_point3d *)&((object *)obj)->position.x = *position;
    halo::objects::object_set_cluster_and_parent(object_index, location);
}

/**
 * Links the object into the cluster of a BSP location and its parent's lists.
 *
 * Original register convention: stack -> object_index, location.
 *
 * @address 0x004f5c30
 */
void halo::objects::ObjectRef::set_cluster_and_parent(bsp_leaf_reference *location)
{
    uint32_t object_index = handle;
    object_header *header = (object_header *)object_data->data + halo::datum_slot(object_index);
    object *obj = header->data;

    if (obj->parent_object == k_datum_index_none) {
        bsp_leaf_reference local_location;

        if (location == 0) {
            int32_t leaf = halo::physics::bsp3d_node_find_leaf(0, (ModelCollisionGeometryBSP *)halo::physics::globals().collision_bsp, &obj->bounding_center);
            if (leaf == -1) {
                local_location.cluster_index = -1;
            } else {

                local_location.cluster_index = *(int16_t *)((uint8_t *)halo::scenario::globals().structure_bsp->leaves.pointer +
                                                            (uint32_t)(leaf & halo::k_leaf_index_mask) * 0x10 + 8);
            }
            local_location.leaf_index = leaf;
            location = &local_location;
            if (local_location.cluster_index == -1) {

                halo::scenario::scenario_location_from_point(&local_location, (real_point3d *)&((struct object *)obj)->position);
            }
        }

        if (location->cluster_index == -1) {
            obj->flags |= _object_outside_map_bit;
        } else {
            obj->location_leaf_index = location->leaf_index;

            *(int32_t *)&((object *)obj)->location_cluster_index = *(int32_t *)&location->cluster_index;
            header->cluster_index = location->cluster_index;
            obj->flags &= ~(uint32_t)_object_outside_map_bit;
        }

        header->flags &= (uint8_t)~_object_header_unknown_80_bit;

        halo::structures::cluster_reference_add_within_radius(object_index, &obj->placement_id, &obj->bounding_center, obj->bounding_radius,
                     (bsp_leaf_reference *)(&obj->location_leaf_index),
                     (cluster_reference_group *)(test_flag(obj->flags, objects::object_flag::has_collision_model) ? (void *)&collideable_cluster_first : (void *)&noncollideable_cluster_first));

        if ((header->flags & _object_header_in_pvs_pass_bit) != 0) {
            int16_t cluster = header->cluster_index;
            if (cluster == -1 ||
                (*(uint32_t *)&local_player_globals->cluster_pvs[(cluster >> 5)] &
                 (1u << (cluster & 0x1f))) == 0) {
                if ((obj->flags & _object_connected_to_map_bit) != 0) {
                    halo::objects::object_delete(object_index);
                }
            } else {
                halo::objects::object_mark_pending_delete(object_index);
            }
        }
    } else {
        object_header *parent_header =
            (object_header *)object_data->data + halo::datum_slot(obj->parent_object);
        object *parent = parent_header->data;

        obj->next_object = parent->first_child_object;
        parent->first_child_object = object_index;
        header->flags |= _object_header_unknown_80_bit;
        obj->location_cluster_index = -1;
    }

    obj->flags |= _object_needs_cluster_update_bit;
    header->flags |= _object_header_connected_bit;
}

/**
 * Unlinks the object from its cluster, or notifies its parent when it is attached.
 *
 * Original register convention: object index in EAX (in_EAX).
 *
 * @address 0x004f5de0
 */
void halo::objects::ObjectRef::unlink_cluster_or_notify_parent()
{
    uint32_t object_index = handle;
    object_header *header = (object_header *)object_data->data + halo::datum_slot(object_index);
    object *obj = header->data;

    if (obj->parent_object == k_datum_index_none) {

        halo::structures::cluster_reference_remove_all(object_index, (datum_index *)&((struct object *)obj)->placement_id,
                     (cluster_reference_group *)(test_flag(obj->flags, objects::object_flag::has_collision_model) ? (void *)&collideable_cluster_first
                                                   : (void *)&noncollideable_cluster_first));
        if ((header->flags & _object_header_in_pvs_pass_bit) != 0) {
            header = (object_header *)object_data->data + halo::datum_slot(object_index);
            if ((header->flags & _object_header_active_bit) != 0) {
                header->flags &= (uint8_t)~_object_header_active_bit;
            }
        }
    } else {
        object *parent = halo::objects::object_try_and_get(obj->parent_object, _object_mask_all);
        if (parent != 0) {

            halo::objects::object_remove_from_sibling_list((datum_index *)((uint8_t *)parent + 0x118), object_index);
        }
    }

    obj->flags &= ~(uint32_t)_object_needs_cluster_update_bit;
    header->flags &= (uint8_t)~_object_header_connected_bit;
}

/**
 * Fills a placement cursor from the root parent of the object.
 *
 * @address 0x004f5f70
 */
int16_t halo::objects::ObjectRef::get_root_parent_placement(object_placement_cursor *out_cursor)
{
    uint32_t object_index = handle;
    object_header *header;
    object *obj;
    int32_t *family;
    data_array *reference_table;
    datum_index placement_id;

    {

        uint32_t root = k_datum_index_none;
        if (object_index != k_datum_index_none) {
            do {
                root = object_index;
                header = (object_header *)object_data->data + halo::datum_slot(root);
                obj = header->data;
                object_index = obj->parent_object;
            } while (object_index != k_datum_index_none);
        }
        header = (object_header *)object_data->data + halo::datum_slot(root);
        obj = header->data;
    }

    if ((obj->flags & _object_has_collision_model_bit) == 0) {
        family = (int32_t *)&noncollideable_cluster_first;
        reference_table = (data_array *)noncollideable_cluster_partition;
    } else {
        family = (int32_t *)&halo::physics::globals().collideable_cluster_first;
        reference_table = (data_array *)collideable_cluster_partition;
    }
    out_cursor->cluster_globals = family;

    placement_id = obj->placement_id;
    out_cursor->next_reference = placement_id;
    if (placement_id != k_datum_index_none) {
        object_cluster_reference *ref =
            (object_cluster_reference *)reference_table->data + halo::datum_slot(placement_id);
        out_cursor->next_reference = ref->next_reference;
        return (int16_t)ref->object_index;
    }
    return -1;
}

/**
 * Returns the address of a node's matrix in the object's node array.
 *
 * Original register convention: EAX -> object_index, stack -> node_index.
 *
 * @address 0x004f6000
 */
real_matrix4x3 * halo::objects::ObjectRef::get_node_marker_address(int16_t node_index)
{
    uint32_t object_index = handle;
    object *obj = ((object_header *)object_data->data)[halo::datum_slot(object_index)].data;
    return (real_matrix4x3 *)((uint8_t *)obj + obj->nodes.offset + node_index * 0x34);
}

/**
 * Returns the marker name of one of the object's attachments.
 *
 * Original register convention: object index in EAX (in_EAX), attachment index in EDX (in_DX).
 *
 * @address 0x004f6030
 */
char * halo::objects::ObjectRef::get_attachment_marker_name(int16_t attachment_index)
{
    uint32_t object_index = handle;
    object *obj = ((object_header *)object_data->data)[halo::datum_slot(object_index)].data;
    Object *object_tag = (Object *)halo::cache::globals().tag_instances[halo::datum_slot(obj->definition_tag)].data;

    if (attachment_index >= 0 && attachment_index < (int32_t)object_tag->attachments.count) {
        return (char *)object_tag->attachments.pointer + 0x10 + attachment_index * 0x48;
    }
    return 0;
}

/**
 * Looks up a named marker on the object and writes its local transforms; returns the number found.
 *
 * Original register convention: stack -> object_index, marker_name, marker, maximum_markers.
 *
 * @address 0x004f6080
 */
int32_t halo::objects::ObjectRef::get_node_local_transform(char *marker_name, object_marker *marker,
    uint32_t maximum_markers)
{
    uint32_t object_index = handle;
    object *obj = ((object_header *)object_data->data)[halo::datum_slot(object_index)].data;
    void *node_array = (uint8_t *)obj + obj->nodes.offset;

    int32_t result = halo::models::model_markers_get_by_name(
        *(datum_index *)((uint8_t *)halo::cache::globals().tag_instances[halo::datum_slot(obj->definition_tag)].data + 0x34), marker_name,
        (uint8_t *)obj + 0x180, (int16_t *)0, (real_matrix4x3 *)node_array, (uint8_t)((obj->flags >> 0xc) & 1),
        marker, (int16_t)maximum_markers);

    if ((int16_t)result == 0) {
        marker->node_index = 0;
        marker->transform.scale = 1.0f;
        marker->transform.forward.i = 1.0f;
        marker->transform.forward.j = 0.0f;
        marker->transform.forward.k = 0.0f;
        marker->transform.left.i = 0.0f;
        marker->transform.left.j = 1.0f;
        marker->transform.left.k = 0.0f;
        marker->transform.up.i = 0.0f;
        marker->transform.up.j = 0.0f;
        marker->transform.up.k = 1.0f;
        marker->transform.position.x = 0.0f;
        marker->transform.position.y = 0.0f;
        marker->transform.position.z = 0.0f;

        obj = ((object_header *)object_data->data)[halo::datum_slot(object_index)].data;
        marker->node_transform = *(real_matrix4x3 *)((uint8_t *)obj + obj->nodes.offset);

        if ((obj->flags & _object_mirrored_geometry_bit) != 0) {
            marker->node_transform.left.i = -marker->node_transform.left.i;
            marker->node_transform.left.j = -marker->node_transform.left.j;
            marker->node_transform.left.k = -marker->node_transform.left.k;
        }

        if (marker_name != 0 && *marker_name == '\0') {
            result = 1;
        }
    }
    return result;
}

/**
 * Reorients an object relative to a named marker of its parent.
 *
 * Original register convention: parent object index and parent marker name are the two stack arguments ([ebp+0x8],
 * [ebp+0xc]); the object being reoriented is in ESI and its own marker name in EDI. Both registers are read before
 * ever being written.
 *
 * @address 0x004f6180
 */
void halo::objects::ObjectRef::reorient_relative_to_marker(uint32_t parent_index, char *parent_marker_name,
    char *object_marker_name)
{
    uint32_t object_index = handle;
    object *obj = ((object_header *)object_data->data)[halo::datum_slot(object_index)].data;
    object_marker own_marker;
    object_marker parent_marker;

    halo::objects::object_get_node_local_transform(parent_index, parent_marker_name, &parent_marker, 1);
    halo::objects::object_get_node_local_transform(object_index, object_marker_name, &own_marker, 1);
    halo::objects::object_unlink_cluster_or_notify_parent(object_index);

    if (object_marker_name != 0 && *object_marker_name != '\0') {
        halo::objects::object_recompute_basis_from_marker_delta(obj, &own_marker, &parent_marker.node_transform);
    } else {
        real_matrix4x3 inverse;
        real_vector3d *forward = &parent_marker.node_transform.forward;
        real_vector3d *up = &parent_marker.node_transform.up;

        halo::math::matrix4x3_inverse(&inverse, own_marker.transform);
        halo::math::matrix4x3_transform_point(obj->position, parent_marker.node_transform.position, inverse);

        obj->forward.i = inverse.up.i * forward->k + inverse.forward.i * forward->i +
                         inverse.left.i * forward->j;
        obj->forward.j = inverse.up.j * forward->k + inverse.left.j * forward->j +
                         inverse.forward.j * forward->i;
        obj->forward.k = inverse.up.k * forward->k + inverse.left.k * forward->j +
                         inverse.forward.k * forward->i;

        obj->up.i = inverse.up.i * up->k + inverse.left.i * up->j + inverse.forward.i * up->i;
        obj->up.j = inverse.up.j * up->k + inverse.left.j * up->j + inverse.forward.j * up->i;
        obj->up.k = inverse.up.k * up->k + inverse.left.k * up->j + inverse.forward.k * up->i;
    }

    halo::objects::object_set_cluster_and_parent(object_index, 0);
    halo::objects::object_attach_to_object(parent_index, object_index,
                            *(uint32_t *)&parent_marker.node_index);
}

/**
 * Rebuilds an orientation matrix from the difference between the object and a marker.
 *
 * Original register convention: object* in EAX (in_EAX), object_marker* on the stack ([ebp+0x8]), output matrix
 * pointer on the stack ([ebp+0xc]). // blam-cc: EAX -> obj; stack -> marker, output_matrix.
 *
 * @address 0x004f62f0
 */
void halo::objects::ObjectView::recompute_basis_from_marker_delta(object_marker *marker,
    real_matrix4x3 *output_matrix)
{
    object *obj = self;
    real_matrix4x3 relative;
    real_matrix4x3 basis;
    real_vector3d cross;

    halo::math::matrix4x3_from_forward_up(obj->up, obj->forward, basis);
    basis.position = obj->position;

    halo::math::matrix4x3_inverse(&relative, basis);
    halo::math::globals().matrix4x3_multiply_procedure(&relative, &marker->node_transform, &relative);
    halo::math::matrix4x3_inverse(&relative, relative);
    halo::math::globals().matrix4x3_multiply_procedure(output_matrix, &relative, &basis);

    obj->position = basis.position;
    obj->forward = basis.forward;

    cross.i = basis.up.k * basis.forward.j - basis.up.j * basis.forward.k;
    cross.j = basis.forward.k * basis.up.i - basis.up.k * basis.forward.i;
    cross.k = basis.up.j * basis.forward.i - basis.up.i * basis.forward.j;

    obj->up.i = cross.j * basis.forward.k - cross.k * basis.forward.j;
    obj->up.j = cross.k * basis.forward.i - basis.forward.k * cross.i;
    obj->up.k = cross.i * basis.forward.j - cross.j * basis.forward.i;

    halo::math::vector3d_normalize_with_length(obj->forward);
    halo::math::vector3d_normalize_with_length(obj->up);
}

/**
 * Attaches a child object to a marker of a parent object.
 *
 * @address 0x004f6440
 */
void halo::objects::ObjectRef::attach_to_object(uint32_t child_index, int16_t marker_index)
{
    uint32_t parent_index = handle;
    uint32_t ancestor = parent_index;

    while (ancestor != k_datum_index_none) {
        object *ancestor_obj;
        if (ancestor == child_index) {
            return;
        }
        ancestor_obj = ((object_header *)object_data->data)[halo::datum_slot(ancestor)].data;
        ancestor = ancestor_obj->parent_object;
    }

    {
        object_header *child_header = (object_header *)object_data->data + halo::datum_slot(child_index);
        object *child = child_header->data;
        int needs_cluster_update = (child->flags >> 0xb) & 1;
        object *parent;
        real_matrix4x3 *parent_node;
        real_matrix4x3 inverse;
        real_vector3d v;

        if (needs_cluster_update) {
            halo::objects::object_unlink_cluster_or_notify_parent(child_index);
        }

        parent = ((object_header *)object_data->data)[halo::datum_slot(parent_index)].data;
        parent_node = (real_matrix4x3 *)((uint8_t *)parent + parent->nodes.offset +
                                         marker_index * 0x34);

        halo::math::matrix4x3_inverse(&inverse, *parent_node);
        halo::math::matrix4x3_transform_point(child->position, child->position, inverse);

        v = child->forward;
        child->forward.i = inverse.forward.i * v.i + inverse.left.i * v.j + inverse.up.i * v.k;
        child->forward.j = inverse.forward.j * v.i + inverse.left.j * v.j + inverse.up.j * v.k;
        child->forward.k = inverse.forward.k * v.i + inverse.left.k * v.j + inverse.up.k * v.k;

        v = child->up;
        child->up.i = inverse.forward.i * v.i + inverse.left.i * v.j + inverse.up.i * v.k;
        child->up.j = inverse.forward.j * v.i + inverse.left.j * v.j + inverse.up.j * v.k;
        child->up.k = inverse.forward.k * v.i + inverse.left.k * v.j + inverse.up.k * v.k;

        child->parent_object = parent_index;
        child->parent_marker_index = (uint8_t)marker_index;

        if (needs_cluster_update) {
            halo::objects::object_set_cluster_and_parent(child_index, 0);
            child_header = (object_header *)object_data->data + halo::datum_slot(child_index);
        }

        if ((child_header->flags & _object_header_active_bit) != 0) {
            child_header->flags &= (uint8_t)~_object_header_active_bit;
        }
        child_header->flags |= _object_header_just_created_bit;

        halo::objects::object_recalculate_bounding_radius(child_index);
    }
}

/**
 * Snaps the object to its parent's marker and detaches it.
 *
 * Original register convention: the object index is a plain STACK argument (0x4f6619 mov edx,[ebp+0x8]), not a
 * register parameter, as the original prototype -- confirmed against objdump -d -M intel
 * bin/halo.exe at 0x4f6610.
 *
 * @address 0x004f6610
 */
void halo::objects::ObjectRef::snap_to_parent_marker_and_detach()
{
    uint32_t object_index = handle;
    object_header *headers = (object_header *)object_data->data;
    object *child = headers[halo::datum_slot(object_index)].data;
    object *old_parent = headers[halo::datum_slot(child->parent_object)].data;

    halo::objects::object_unlink_cluster_or_notify_parent(object_index);

    {
        object *parent_node_owner = ((object_header *)object_data->data)[halo::datum_slot(child->parent_object)].data;
        real_matrix4x3 *parent_node = (real_matrix4x3 *)((uint8_t *)parent_node_owner +
            parent_node_owner->nodes.offset + (int8_t)child->parent_marker_index * 0x34);

        real_matrix4x3 own_rotation;
        real_matrix4x3 local_transform;
        real_matrix4x3 world;

        halo::math::matrix4x3_from_forward_up(child->up, child->forward, own_rotation);

        local_transform.scale = 1.0f;
        local_transform.forward.i = 1.0f; local_transform.forward.j = 0.0f; local_transform.forward.k = 0.0f;
        local_transform.left.i = 0.0f;    local_transform.left.j = 1.0f;    local_transform.left.k = 0.0f;
        local_transform.up.i = 0.0f;      local_transform.up.j = 0.0f;      local_transform.up.k = 1.0f;
        local_transform.position = child->position;

        halo::math::globals().matrix4x3_multiply_procedure(parent_node, &local_transform, &world);
        halo::math::globals().matrix4x3_multiply_procedure(&world, &own_rotation, &world);

        child->forward = world.forward;
        child->up = world.up;
        child->position = world.position;
    }

    child->velocity = old_parent->velocity;
    child->angular_velocity = old_parent->angular_velocity;

    child->parent_marker_index = 0xff;
    child->parent_object = k_datum_index_none;

    halo::objects::object_set_cluster_and_parent(object_index, 0);

    {
        object_header *header = (object_header *)object_data->data + halo::datum_slot(object_index);
        object *obj = header->data;
        if (((header->flags & _object_header_active_bit) == 0) &&
            ((obj->flags & _object_do_not_delete_bit) == 0) &&
            (obj->parent_object == k_datum_index_none)) {
            header->flags |= _object_header_active_bit;
        }
    }
}

/**
 * Sets or clears the flag marking the object as inside the potentially visible set.
 *
 * Original register convention: object index in EAX (in_EAX), boolean in BL (unaff_BL). Confirmed against objdump -d
 * -M intel bin/halo.exe: 0x4f67e1 mov ecx,eax / and ecx,0xffff, 0x4f67ff test bl,bl. // blam-cc: EAX -> object_index,
 * BL -> in_pvs.
 *
 * @address 0x004f67e0
 */
void halo::objects::ObjectRef::set_in_pvs_pass_flag(uint8_t in_pvs)
{
    uint32_t object_index = handle;
    object_header *header = (object_header *)object_data->data + halo::datum_slot(object_index);
    object *obj = header->data;

    if (in_pvs != 0) {
        header->flags |= _object_header_in_pvs_pass_bit;
        if ((obj->parent_object == k_datum_index_none) && (obj->location_cluster_index == -1)) {
            if ((header->flags & _object_header_active_bit) != 0) {
                header->flags &= (uint8_t)~_object_header_active_bit;
            }
        }
    } else {
        uint8_t flags = header->flags & (uint8_t)~_object_header_in_pvs_pass_bit;
        header->flags = flags;
        if ((flags & _object_header_active_bit) == 0) {
            halo::objects::object_mark_pending_delete(object_index);
        }
    }
}

/**
 * Enables or disables collision for an object.
 *
 * @address 0x004f6850
 */
void halo::objects::ObjectRef::set_collision_enabled(uint8_t enable)
{
    uint32_t object_index = handle;
    object_header *header = (object_header *)object_data->data + halo::datum_slot(object_index);
    object *obj = header->data;
    Object *definition = (Object *)halo::cache::globals().tag_instances[halo::datum_slot(obj->definition_tag)].data;
    int has_model = (definition->model.tag_id.index != halo::k_word_none);
    int currently_disabled = (obj->flags & _object_no_collision_bit) != 0;
    int requesting_disabled = (enable == 0);

    if (has_model) {

        if (currently_disabled != requesting_disabled) {
            if (enable != 0) {
                halo::objects::object_for_each_light_attachment(object_index, 0, 1);
            } else {
                halo::objects::object_for_each_light_attachment(object_index, 1, 0);
            }
        }
    } else if (enable != 0) {
        return;
    }

    if (enable == 0) {
        obj->flags |= _object_no_collision_bit;
        header->flags &= (uint8_t)~0x02;
    } else {
        obj->flags &= ~(uint32_t)_object_no_collision_bit;
        header->flags |= 0x02;
    }
}

/**
 * Writes the object's world position, transforming through the parent's marker node when the object is attached.
 *
 * @address 0x004f6900
 */
void halo::objects::ObjectRef::get_position(real_point3d *out)
{
    uint32_t object_index = handle;
    object *obj = ((object_header *)object_data->data)[halo::datum_slot(object_index)].data;

    if (obj->parent_object == k_datum_index_none) {
        *out = obj->position;
        return;
    }

    {
        object *parent = ((object_header *)object_data->data)[halo::datum_slot(obj->parent_object)].data;
        real_matrix4x3 *parent_node = (real_matrix4x3 *)((uint8_t *)parent + parent->nodes.offset +
            (int8_t)obj->parent_marker_index * 0x34);
        halo::math::matrix4x3_transform_point(*out, obj->position, *parent_node);
    }
}

/**
 * Writes the object's forward and up vectors, transformed through its parent when attached.
 *
 * @address 0x004f6970
 */
void halo::objects::ObjectRef::get_orientation(real_vector3d *out_forward, real_vector3d *out_up)
{
    uint32_t object_index = handle;
    object *obj = ((object_header *)object_data->data)[halo::datum_slot(object_index)].data;

    if (obj->parent_object == k_datum_index_none) {
        if (out_forward != (real_vector3d *)0) {
            *out_forward = obj->forward;
        }
        if (out_up != (real_vector3d *)0) {
            *out_up = obj->up;
        }
        return;
    }

    {
        object *parent = ((object_header *)object_data->data)[halo::datum_slot(obj->parent_object)].data;
        real_matrix4x3 *parent_node = (real_matrix4x3 *)((uint8_t *)parent + parent->nodes.offset +
            (int8_t)obj->parent_marker_index * 0x34);

        if (out_forward != (real_vector3d *)0) {
            halo::math::matrix4x3_transform_normal(*out_forward, obj->forward, *parent_node);
        }
        if (out_up != (real_vector3d *)0) {
            halo::math::matrix4x3_transform_normal(*out_up, obj->up, *parent_node);
        }
    }
}

/**
 * Writes the object's world matrix and returns it.
 *
 * @address 0x004f6a20
 */
real_matrix4x3 * halo::objects::ObjectRef::get_world_matrix(real_matrix4x3 *out)
{
    uint32_t object_index = handle;
    object *obj = ((object_header *)object_data->data)[halo::datum_slot(object_index)].data;

    halo::math::matrix4x3_from_forward_up(obj->up, obj->forward, *out);
    out->position = obj->position;

    if (obj->parent_object != k_datum_index_none) {
        object *parent = ((object_header *)object_data->data)[halo::datum_slot(obj->parent_object)].data;
        real_matrix4x3 *parent_node = (real_matrix4x3 *)((uint8_t *)parent + parent->nodes.offset +
            (int8_t)obj->parent_marker_index * 0x34);
        halo::math::globals().matrix4x3_multiply_procedure(parent_node, out, out);
    }

    return out;
}

/**
 * Returns the linear and angular velocity of the root of the parent chain.
 *
 * @address 0x004f6aa0
 */
void halo::objects::ObjectRef::get_root_object_velocities(real_vector3d *out_velocity,
    real_vector3d *out_angular_velocity)
{
    uint32_t object_index = handle;
    object *root = ((object_header *)object_data->data)[halo::datum_slot(object_index)].data;

    while (root->parent_object != k_datum_index_none) {
        root = ((object_header *)object_data->data)[halo::datum_slot(root->parent_object)].data;
    }

    if (out_velocity != (real_vector3d *)0) {
        *out_velocity = root->velocity;
    }
    if (out_angular_velocity != (real_vector3d *)0) {
        *out_angular_velocity = root->angular_velocity;
    }
}

/**
 * Writes the BSP location of the root object of the parent chain.
 *
 * @address 0x004f6b10
 */
void halo::objects::ObjectRef::get_root_location(int32_t *out)
{
    uint32_t object_index = handle;

    uint32_t root_index = k_datum_index_none;
    if (object_index != k_datum_index_none) {
        uint32_t current = object_index;
        do {
            root_index = current;
            current = ((object_header *)object_data->data)[halo::datum_slot(root_index)].data->parent_object;
        } while (current != k_datum_index_none);
    }

    {
        object *root = ((object_header *)object_data->data)[halo::datum_slot(root_index)].data;
        out[0] = root->location_leaf_index;
        out[1] = *(int32_t *)&root->location_cluster_index;
    }
}

/**
 * Copies the default node transforms of the object's model into its node array.
 *
 * @address 0x004f6b70
 */
void halo::objects::ObjectRef::copy_default_node_transforms(int16_t requested_count)
{
    uint32_t object_index = handle;
    object *obj = ((object_header *)object_data->data)[halo::datum_slot(object_index)].data;
    Object *definition = (Object *)halo::cache::globals().tag_instances[halo::datum_slot(obj->definition_tag)].data;
    GBXModel *model = (GBXModel *)halo::cache::globals().tag_instances[halo::datum_slot(definition->model.tag_id.index)].data;
    int32_t byte_count = (int16_t)model->nodes.count << 5;

    uint8_t *src = (uint8_t *)obj + obj->node_function_defaults.offset;
    uint8_t *dst = (uint8_t *)obj + obj->node_function_values.offset;
    int32_t i;
    for (i = 0; i < byte_count; i++) {
        dst[i] = src[i];
    }

    if (requested_count >= (int16_t)(obj->node_function_count - obj->interpolation_frame_index)) {
        obj->interpolation_frame_index = 0;
        obj->node_function_count = requested_count;
    }
}

/**
 * Adds a translation to every node of the object.
 *
 * Original register convention: object index in EAX, delta vector in EDX. Confirmed against objdump -d -M intel
 * bin/halo.exe: 0x4f6c36 fld [eax+ecx+0x10] then 0x4f6c3c fadd [edx]. // blam-cc: EAX -> object_index, EDX -> delta.
 *
 * @address 0x004f6c10
 */
void halo::objects::ObjectRef::offset_node_translation(real_vector3d *delta)
{
    uint32_t object_index = handle;
    object *obj = ((object_header *)object_data->data)[halo::datum_slot(object_index)].data;

    if (obj->node_function_count != 0) {
        real_vector3d *translation = (real_vector3d *)
            ((uint8_t *)obj + obj->node_function_values.offset + 0x10);
        translation->i += delta->i;
        translation->j += delta->j;
        translation->k += delta->k;
    }
}

/**
 * Solves a two bone inverse kinematics chain so a node reaches a marker of another object.
 *
 * @address 0x004f6d60
 */
void halo::objects::ObjectRef::solve_two_bone_ik_to_marker(char *marker_a_name, uint32_t marker_b_object_index,
    char *marker_b_name, uint8_t *node_base)
{
    uint32_t object_index = handle;
    Object *definition = (Object *)halo::cache::globals().tag_instances[
        ((object_header *)object_data->data)[halo::datum_slot(object_index)].data->definition_tag & 0xffff].data;
    GBXModel *model = (GBXModel *)halo::cache::globals().tag_instances[halo::datum_slot(definition->model.tag_id.index)].data;
    uint8_t *nodes = (uint8_t *)model->nodes.pointer;

    object_marker marker_a;
    object_marker marker_b;

    if (halo::objects::object_get_node_local_transform(object_index, marker_a_name, &marker_a, 1) == 0) {
        return;
    }

    if (halo::objects::object_get_node_local_transform(marker_b_object_index, marker_b_name, &marker_b, 1) == 0) {
        return;
    }

    {
        int16_t node_b = *(int16_t *)(nodes + marker_a.node_index * 0x9c + 0x24);
        if (node_b != -1) {
            int16_t node_c = *(int16_t *)(nodes + node_b * 0x9c + 0x24);
            if (node_c != -1) {
                real_matrix4x3 inverse;

                halo::math::matrix4x3_inverse(&inverse, marker_a.transform);
                halo::math::globals().matrix4x3_multiply_procedure(&marker_b.node_transform, &inverse, &inverse);

                halo::models::model_ik_solve_two_bone(&inverse,
                    reinterpret_cast<real_matrix4x3 *>(node_base + node_c * 0x34),
                    reinterpret_cast<real_matrix4x3 *>(node_base + node_b * 0x34),
                    reinterpret_cast<real_matrix4x3 *>(node_base + marker_a.node_index * 0x34));
            }
        }
    }
}

/**
 * Reads the value of an object function selector for an object; returns whether it is valid.
 *
 * @address 0x004f6e70
 */
uint8_t halo::objects::ObjectRef::function_get_value(int16_t selector, float *out_value)
{
    uint32_t object_index = handle;
    object *obj = ((object_header *)object_data->data)[halo::datum_slot(object_index)].data;

    if (selector == -1) {
        *out_value = 1.0f;
        return 1;
    }

    *out_value = obj->function_out_values[selector];
    return (obj->function_valid_flags & (1 << (selector & 0x1f))) != 0;
}

/**
 * Returns the handle of the root of the object's parent chain.
 *
 * Original register convention: object index in ECX, returns the root index in EAX. Confirmed against objdump -d -M
 * intel bin/halo.exe: 0x4f6fb3 cmp ecx,0xffffffff at entry, no stack access. // blam-cc: ECX -> object_index.
 *
 * @address 0x004f6fb0
 */
uint32_t halo::objects::ObjectRef::get_root_object_index()
{
    uint32_t object_index = handle;
    uint32_t root = k_datum_index_none;

    if (object_index != k_datum_index_none) {
        uint32_t current = object_index;
        do {
            root = current;
            current = ((object_header *)object_data->data)[halo::datum_slot(root)].data->parent_object;
        } while (current != k_datum_index_none);
    }

    return root;
}

/**
 * Adds an object to or removes it from the global object list.
 *
 * Original register convention: object index in ECX, add/remove flag as the sole stack parameter. Confirmed against
 * objdump -d -M intel bin/halo.exe: 0x4f7458 mov eax,ecx / and eax,0xffff at entry. // blam-cc: ECX -> object_index,
 * stack -> add.
 *
 * @address 0x004f7450
 */
void halo::objects::ObjectRef::list_membership_set(char add)
{
    uint32_t object_index = handle;
    object *obj = ((object_header *)object_data->data)[halo::datum_slot(object_index)].data;

    if (add == 0) {
        if ((obj->flags & _object_in_tracked_list_bit) != 0) {
            datum_index *slot = &object_globals_pointer->first_tracked_object;
            while (*slot != object_index) {
                object *node = ((object_header *)object_data->data)[halo::datum_slot(*slot)].data;
                slot = &node->next_tracked_object;
            }
            *slot = obj->next_tracked_object;
            obj->next_tracked_object = k_datum_index_none;
            obj->flags &= ~(uint32_t)_object_in_tracked_list_bit;
        }
    } else if ((obj->flags & (_object_in_tracked_list_bit | _object_unknown_20000_bit)) == 0) {
        obj->next_tracked_object = object_globals_pointer->first_tracked_object;
        object_globals_pointer->first_tracked_object = object_index;
        obj->flags |= _object_in_tracked_list_bit;
    }
}

namespace {
static uint8_t * &local_player_globals__as_object_test_in_atmosphere_zone = reinterpret_cast<uint8_t * &>(local_player_globals);
}

/**
 * Returns whether the object lies inside an atmosphere zone.
 *
 * Original register convention: object index in EAX. Confirmed against objdump -d -M intel bin/halo.exe: 0x4f76ee mov
 * esi,eax at entry, then esi is masked as the object index immediately. // blam-cc: EAX -> object_index.
 *
 * @address 0x004f76e0
 */
uint8_t halo::objects::ObjectRef::test_in_atmosphere_zone()
{
    uint32_t object_index = handle;
    object_header *header = (object_header *)object_data->data + halo::datum_slot(object_index);
    object *obj = header->data;
    uint8_t result = 0;

    if (((header->flags & _object_header_active_bit) != 0) &&
        ((obj->flags & _object_needs_cluster_update_bit) != 0) &&
        ((obj->flags & _object_outside_map_bit) == 0)) {

        object_placement_cursor cursor;
        int16_t placement = halo::objects::object_get_root_parent_placement(object_index, &cursor);

        if (placement != -1) {
            uint32_t ref = (uint32_t)(uint16_t)placement;
            uint32_t ref_index = cursor.next_reference;

            data_array *references = (data_array *)cursor.cluster_globals[2];

            while ((*(uint32_t *)(local_player_globals__as_object_test_in_atmosphere_zone + 0x18 + ((int16_t)ref >> 5) * 4) &
                    (1u << ((uint8_t)ref & 0x1f))) == 0) {
                if (ref_index == k_datum_index_none) {
                    ref = k_datum_index_none;
                } else {

                    object_cluster_reference *node =
                        (object_cluster_reference *)references->data + halo::datum_slot(ref_index);
                    ref_index = node->next_reference;
                    ref = node->object_index;
                }
                if ((int16_t)ref == -1) {
                    return 0;
                }
            }

            if ((int16_t)ref != -1) {
                float search_radius = obj->bounding_radius;

                uint32_t zone_index = halo::memory::datum_next(-1, player_data);

                while (zone_index != k_datum_index_none) {
                    uint8_t *zone_table = (uint8_t *)player_data->data;
                    int32_t zone_offset = (int32_t)halo::datum_slot(zone_index) * 0x200;
                    int32_t zone_cluster_head = *(int32_t *)(zone_table + zone_offset + 0x34);

                    if (zone_cluster_head != -1) {
                        object_marker marker;
                        float dx, dy, dz;

                        halo::objects::object_get_node_local_transform(zone_cluster_head, ai_marker_name_a, &marker, 1);
                        dx = obj->bounding_center.x - marker.node_transform.position.x;
                        dy = obj->bounding_center.y - marker.node_transform.position.y;
                        dz = obj->bounding_center.z - marker.node_transform.position.z;

                        if (search_radius * search_radius <= dx * dx + dy * dy + dz * dz) {

                            uint8_t *extended = (uint8_t *)((object_header *)object_data->data)[halo::datum_slot(*(uint16_t *)(zone_table + zone_offset + 0x34))].data;
                            real_vector3d delta;
                            float length;
                            double angle;
                            double c;

                            delta.i = dx;
                            delta.j = dy;
                            delta.k = dz;
                            length = halo::math::vector3d_normalize_with_length(delta);

                            angle = atan2((double)search_radius, (double)length);

                            c = cos(angle + 0.7853982);

                            if (delta.i * *(float *)(extended + 0x230) +
                                delta.j * *(float *)(extended + 0x234) +
                                delta.k * *(float *)(extended + 0x238) <= c) {
                                zone_index = halo::memory::datum_next((int16_t)zone_index, player_data);
                                continue;
                            }
                        }
                    } else {
                        zone_index = halo::memory::datum_next((int16_t)zone_index, player_data);
                        continue;
                    }

                    result = 1;
                    break;
                }
            }
        }
    }

    return result;
}

/**
 * Notifies each child of an object, recursively.
 *
 * Original register convention: object index is the sole, genuinely-stack, parameter (the original prototype
 * param_1)"). Confirmed against objdump -d -M intel bin/halo.exe: 0x4f7b00 mov eax,[esp+0x4] at entry.
 *
 * @address 0x004f7b00
 */
void halo::objects::ObjectRef::notify_children_recursive()
{
    uint32_t object_index = handle;
    while (object_index != k_datum_index_none) {
        object *obj = ((object_header *)object_data->data)[halo::datum_slot(object_index)].data;

        if (obj->definition_tag != k_datum_index_none) {
            uint8_t *tag_data = (uint8_t *)halo::cache::globals().tag_instances[halo::datum_slot(obj->definition_tag)].data;
            halo::cache::predicted_resource_list_touch((TagReflexive *)(tag_data + 0x170));
        }

        halo::objects::object_notify_children_recursive(obj->first_child_object);
        object_index = obj->next_object;
    }
}

/**
 * Moves the object to a spawn position, avoiding collisions with another object; returns success.
 *
 * @address 0x004f7b70
 */
uint8_t halo::objects::ObjectRef::reposition_to_spawn_location(real_point3d *target_position,
    uint32_t ignore_object_index)
{
    uint32_t object_index = handle;
    uint8_t *obj = (uint8_t *)((object_header *)object_data->data)[halo::datum_slot(object_index)].data;
    real_vector3d delta;
    collision_result hit;

    delta.i = ((object *)obj)->position.x - target_position->x;
    delta.j = ((object *)obj)->position.y - target_position->y;
    delta.k = ((object *)obj)->position.z - target_position->z;
    if (!halo::physics::collision_test_movement_segment(halo::to_bits(halo::collision_test_flag::front_face | halo::collision_test_flag::ignore_invisible | halo::collision_test_flag::structure_bsp | halo::collision_test_flag::water_surface | halo::collision_test_flag::nearby_objects | halo::collision_test_flag::unstick), target_position, &delta, ignore_object_index, &hit) &&
        ((object *)obj)->location_cluster_index != -1) {
        return 1;
    }
    if (hit.leaf.cluster_index == -1) {
        return 0;
    }
    halo::objects::object_unlink_cluster_or_notify_parent(object_index);
    obj = (uint8_t *)((object_header *)object_data->data)[halo::datum_slot(object_index)].data;
    *(real_point3d *)&((object *)obj)->position.x = hit.point;
    halo::objects::object_set_cluster_and_parent(object_index, &hit.leaf);
    halo::objects::object_recalculate_bounding_radius(object_index);
    return 1;
}

/**
 * Moves the object by its velocity and writes the new position; returns whether it moved.
 *
 * Original register convention: object index in EAX, out point on the stack. Confirmed against objdump -d -M intel
 * bin/halo.exe: 0x4f7c4c and eax,0xffff at entry, no stack access before that. // blam-cc: EAX -> object_index.
 *
 * @address 0x004f7c40
 */
uint8_t halo::objects::ObjectRef::nudge_position_by_velocity(real_point3d *out)
{
    uint32_t object_index = handle;
    object *obj = ((object_header *)object_data->data)[halo::datum_slot(object_index)].data;

    if (obj->network_position_valid == 1 && obj->network_velocity_valid == 1 &&
        obj->network_timestamp_valid == 1) {

        uint32_t elapsed_ms = (uint32_t)halo::cseries::time_query_performance_counter_ms() - obj->network_timestamp;

        if (elapsed_ms != 0) {
            real_vector3d velocity = obj->velocity;
            float speed = (float)sqrt(velocity.i * velocity.i + velocity.j * velocity.j +
                                      velocity.k * velocity.k);
            if (speed > 0.05f) {
                real_point3d base = obj->network_position;
                halo::math::point3d_add_scaled(*out, velocity, base, (float)elapsed_ms * 0.001f * 30.0f * speed);
                return 1;
            }
        }
    }
    return 0;
}

/**
 * Notifies the type definition that the node array changed when the object is animated.
 *
 * Original register convention: object index in EAX. Confirmed against objdump -d -M intel bin/halo.exe: 0x4f8b1a mov
 * ebx,eax at entry (ebx then carries the object index across the call). // blam-cc: EAX -> object_index.
 *
 * @address 0x004f8b10
 */
void halo::objects::ObjectRef::notify_node_array_if_animated()
{
    uint32_t object_index = handle;
    object *obj = ((object_header *)object_data->data)[halo::datum_slot(object_index)].data;
    Object *definition = (Object *)halo::cache::globals().tag_instances[halo::datum_slot(obj->definition_tag)].data;

    if ((definition->model.tag_id.index != halo::k_word_none) && (definition->animation_graph.tag_id.index != halo::k_word_none)) {
        halo::objects::object_type_definitions_notify_0x4c(object_index, (uint32_t)((uint8_t *)obj + obj->nodes.offset));
    }
}

/**
 * Removes the target object from a sibling chain starting at the given slot.
 *
 * @address 0x004f8fe0
 */
void halo::objects::ObjectRef::remove_from_sibling_list(datum_index *slot)
{
    uint32_t target_object_index = handle;
    if (*slot != k_datum_index_none) {
        object *node;
        do {
            node = ((object_header *)object_data->data)[halo::datum_slot(*slot)].data;
            if (*slot == target_object_index) {
                break;
            }
            slot = &node->next_object;
        } while (*slot != k_datum_index_none);

        if (*slot == target_object_index) {
            *slot = node->next_object;
            node->next_object = k_datum_index_none;
        }
    }
}

/**
 * Sets the object's scale over a number of ticks and refreshes its nodes.
 *
 * Original register convention: object index in EAX, new scale as the sole stack parameter. Confirmed against the disassembly: EAX -> object_index, stack -> scale.
 *
 * @address 0x004f96a0
 */
void halo::objects::ObjectRef::set_scale_and_refresh_nodes(float scale, int16_t ticks)
{
    uint32_t object_index = handle;
    if (object_index != k_datum_index_none) {
        object *obj = ((object_header *)object_data->data)[halo::datum_slot(object_index)].data;
        obj->scale = scale;
        if (((1u << (obj->type & 0x1f)) & _object_mask_no_node_functions) == 0) {
            halo::objects::object_copy_default_node_transforms(object_index, ticks);
        }
    }
}

/**
 * Detaches an object from the map's cluster lists; returns whether it was connected.
 *
 * Original register convention: object index is the sole, genuinely-stack, parameter (Ghidra's own
 * "object_disconnect_from_map(uint param_1)").
 *
 * @address 0x004f96f0
 */
uint8_t halo::objects::ObjectRef::disconnect_from_map()
{
    uint32_t object_index = handle;
    object *obj = ((object_header *)object_data->data)[halo::datum_slot(object_index)].data;
    return obj->cluster_stamp != halo::physics::globals().object_cluster_stamp;
}

/**
 * Reserves a render cache slot for the object.
 *
 * Original register convention: object index in EDX, slot index in CX. Confirmed against objdump -d -M intel
 * bin/halo.exe: 0x4f9aca mov eax,edx at entry, 0x4f9adf movsx eax,cx. // blam-cc: EDX -> object_index, CX -> slot.
 *
 * @address 0x004f9ac0
 */
void halo::objects::ObjectRef::reserve_render_cache_slot(int16_t slot)
{
    uint32_t object_index = handle;
    object *obj = ((object_header *)object_data->data)[halo::datum_slot(object_index)].data;

    if (object_name_list[slot] == k_datum_index_none) {
        object_name_list[slot] = object_index;
        obj->render_cache_slot = slot;
    }
}

/**
 * Releases the object's render cache slot.
 *
 * Original register convention: object index in EDI. Confirmed against objdump-consistent pattern: Ghidra shows only
 * "unaff_EDI", no stack access. // blam-cc: EDI -> object_index.
 *
 * @address 0x004f9b00
 */
void halo::objects::ObjectRef::release_render_cache_slot()
{
    uint32_t object_index = handle;
    object *obj = ((object_header *)object_data->data)[halo::datum_slot(object_index)].data;

    if (obj->render_cache_slot != -1) {
        int32_t count = *(int32_t *)(halo::scenario::globals().scenario + 0x204);
        int16_t i;

        obj->render_cache_slot = -1;

        for (i = 0; i < count; i++) {
            if (object_name_list[i] == object_index) {
                object_name_list[i] = k_datum_index_none;
            }
        }
    }
}

/**
 * Starts a named animation from a graph tag at the requested frame.
 *
 * @address 0x004fa8d0
 */
void halo::objects::ObjectRef::start_animation(datum_index graph_tag, char *name, int16_t requested_frame)
{
    uint32_t object_index = handle;
    if ((object_index != k_datum_index_none) && (graph_tag != k_datum_index_none)) {
        object *obj = ((object_header *)object_data->data)[halo::datum_slot(object_index)].data;
        void *graph = halo::cache::globals().tag_instances[halo::datum_slot(graph_tag)].data;

        int16_t animation_index = halo::models::animation_graph_find_animation_by_name(graph_tag, name);

        if (animation_index != -1) {
            uint8_t *nodes = *(uint8_t **)((uint8_t *)graph + 0x78);
            uint8_t *extended_flags = (uint8_t *)obj + 0x1f4;

            clear_flag(obj->flags, objects::object_flag::unknown_80);
            *extended_flags |= 1;
            obj->animation_index = animation_index;

            if (requested_frame < 0) {
                obj->animation_frame = 0;
                obj->animation_graph = graph_tag;
                return;
            }

            {
                int16_t frame_count = *(int16_t *)(nodes + animation_index * 0xb4 + 0x22) - 1;
                int16_t frame = (requested_frame <= frame_count) ? requested_frame : frame_count;
                obj->animation_frame = frame;
                obj->animation_graph = graph_tag;
            }
            return;
        }

        console_print_va("the animation '%s' doesn't exist in the graph '%s'", name,
            *(char **)((uint8_t *)&halo::cache::globals().tag_instances[halo::datum_slot(graph_tag)] + 0x10));
    }
}

/**
 * Returns the number of animation frames left in the object's current animation.
 *
 * Original register convention: object index in EAX. Consistent with Ghidra's own "in_EAX" and no other input. //
 * blam-cc: EAX -> object_index.
 *
 * @address 0x004fa9b0
 */
uint32_t halo::objects::ObjectRef::animation_get_frames_remaining()
{
    uint32_t object_index = handle;
    object *obj = ((object_header *)object_data->data)[halo::datum_slot(object_index)].data;
    uint8_t *extended_flags = (uint8_t *)obj + 0x1f4;

    if ((*extended_flags & 1) != 0) {
        void *graph = halo::cache::globals().tag_instances[halo::datum_slot(obj->animation_graph)].data;
        uint8_t *nodes = *(uint8_t **)((uint8_t *)graph + 0x78);
        int32_t frame_count = *(int16_t *)(nodes + obj->animation_index * 0xb4 + 0x22);
        int32_t remaining = (frame_count - obj->animation_frame) - 2;
        return (remaining < 1) ? 0 : (uint32_t)remaining;
    }

    return (halo::datum_slot(object_index) * 3) & 0xffff0000;
}

/**
 * Returns the blended marker transform of an attachment instance, stored in the module's scratch marker.
 *
 * @address 0x004fe740
 */
uint8_t * halo::objects::ObjectRef::attachment_get_blended_marker(uint8_t *instance)
{
    uint32_t object_index = handle;
    uint8_t *source = *(uint8_t **)(instance + 0x124);

    if (*(int32_t *)(instance + 0x120) < 2) {
        return source;
    }

    {
        object *obj = ((object_header *)object_data->data)[halo::datum_slot(object_index)].data;
        int16_t selector = *(int16_t *)(instance + 0xb8) - 1;
        float weight;

        if (selector == -1) {
            weight = 1.0f;
        } else {
            weight = *(float *)((uint8_t *)obj + 0x134 + selector * 4);
            if (((1 << (selector & 0x1f)) & ((struct object *)obj)->function_valid_flags) == 0) {
                return source;
            }
        }

        {
            float inv = 1.0f - weight;
            static const int offsets[16] = {
                0x10, 0x14, 0x18, 0x3c, 0x40, 0x44, 0x68, 0x6c,
                0x70, 0x74, 0x78, 0x7c, 0x80, 0x84, 0x88, 0x8c
            };
            int i;
            for (i = 0; i < 16; i++) {
                float v = *(float *)(source + offsets[i]);

                *(float *)(object_marker_scratch + offsets[i]) = weight * v + inv * v;
            }
        }
    }
    return object_marker_scratch;
}
