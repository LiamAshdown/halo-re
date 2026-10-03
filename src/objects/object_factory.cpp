#include "halo/objects/object_factory.hpp"
#include "halo/models/api.hpp"
#include "game.h"
#include "units.h"
#include "effects.h"
#include "networking.h"
#include "cutscene.h"
#include "halo/math/api.hpp"
#include "halo/memory/api.hpp"
#include "halo/cache/api.hpp"
#include "halo/input/api.hpp"
#include "halo/physics/api.hpp"
#include "halo/effects/api.hpp"

extern "C" {
extern uint32_t bsp3d_node_find_leaf(int32_t node_index, ModelCollisionGeometryBSP *bsp, real_point3d *point);
extern cinematic_globals *cinematic_globals_ptr;
extern void console_print_error_va(uint8_t clear_first, const char *format, ...);
extern game_engine_definition *current_game_engine;
extern datum_index effect_new_on_object(datum_index creator_object_index, datum_index definition_index, datum_index object_index, int16_t first_person_weapon_override, real a_scale, real b_scale, const ColorRGB *color, const effect_tint_source *tint_source);
extern uint8_t g_control_binding_secondary_active;
extern uint8_t g_control_binding_state;
extern uint32_t game_engine_remap_placement_by_type(uint32_t handle);
extern ModelCollisionGeometryBSP *global_collision_bsp;
extern uint8_t *global_scenario;
extern int16_t global_structure_bsp_index;
extern const real_vector3d *global_white_color;
extern uint8_t network_action_apply_active;
extern int32_t network_client;
extern int16_t network_game_mode;
extern char network_log_path_format[];
extern uint8_t network_message_scratch[0x7ff8];
extern int32_t network_server;
extern char network_session_broadcast_to_flagged(int32_t body_bit_count, void *server, int32_t status_bit, void *data, int32_t immediate, int32_t flush_after, int32_t force, int32_t unused);
extern void object_block_data_free(data_array *array, datum_index object_index);
extern uint8_t object_block_data_grow(uint32_t object_index, int16_t field_offset, int16_t extra_size);
extern datum_index object_block_data_new(int32_t specific_index, data_array *array, int16_t size);
extern void object_create_attachments(uint32_t object_index);
extern data_array *object_data;
extern void object_delete(uint32_t object_index);
extern object_globals *object_globals_pointer;
extern void object_initialize_change_colors(uint32_t object_index, ColorRGB *colors);
extern void object_initialize_shield_stun_thresholds(uint32_t object_index, float *override_max_body_vitality, float *override_max_shield_vitality);
extern memory_pool *object_memory_pool;
extern datum_index *object_name_list;
extern datum_index object_new(object_placement_data *placement);
extern datum_index object_new_from_scenario_placement(uint8_t *placement, TagReflexive *palette);
extern datum_index object_new_with_datum_role_control(object_placement_data *placement, uint32_t role);
extern void object_notify_node_array_if_animated(uint32_t object_index);
extern void object_placement_data_initialize(object_placement_data *placement, datum_index definition_tag, datum_index role);
extern void object_recalculate_bounding_radius(uint32_t object_index);
extern void object_refresh_region_permutations(uint32_t object_index);
extern void object_reserve_render_cache_slot(uint32_t object_index, int16_t slot);
extern void object_set_cluster_and_parent(uint32_t object_index, bsp_leaf_reference *location);
extern void object_set_collision_enabled(uint32_t object_index, uint8_t enable);
extern object *object_try_and_get(datum_index object_index, uint32_t type_mask);
extern object_type_definition *object_type_definitions[k_maximum_object_types];
extern void object_type_definitions_notify_0x24(uint32_t object_index, uint32_t argument);
extern void object_type_definitions_notify_0x30(uint32_t object_index);
extern void object_type_definitions_notify_0x38(uint32_t object_index);
extern void object_type_definitions_notify_two_args_0x2c(uint32_t object_index, uint32_t event_argument);
extern uint8_t object_type_definitions_query_0x28(uint32_t object_index);
extern void object_type_override_call_0x68(uint32_t object_index);
extern int object_type_override_get_0x64(uint32_t object_index, void *buffer, int32_t buffer_size);
extern void object_update_change_colors(uint32_t object_index);
extern void object_update_functions(uint32_t object_index);
extern uint16_t object_visibility_computed_mask;
extern void objects_garbage_collection(void);
extern void scenario_objects_place_for_structure_bsp(uint8_t place);
extern void widget_new(uint32_t object_index);
}

namespace {
static datum_index palette_tag(TagReflexive *palette, int16_t type)
{
    return *(datum_index *)((uint8_t *)palette->pointer + type * 0x30 + 0xc);
}
}

/**
 * Places the objects of a scenario.
 *
 * Original register convention: stack -> scenario.
 *
 * @address 0x004f3ba0
 */
void halo::objects::ObjectFactory::place_scenario(uint8_t *scenario)
{
    uint8_t *connection = 0;
    uint8_t joining = 0;
    int16_t type;

    if (network_server != 0) {
        connection = (uint8_t *)network_server + 8;
    } else if (network_client != 0) {
        connection = (uint8_t *)network_client + 0xb14;
    }
    if (connection != 0 && *(int32_t *)(connection + 0x134) == 5) {
        joining = 1;
    } else {
        halo::input::control_binding_table_initialize();
        if (network_game_mode == 2) {
            object_type_definition *vehicle = object_type_definitions[_object_type_vehicle];
            int32_t size = vehicle->scenario_placement_size;
            TagReflexive *placements = (TagReflexive *)(scenario + vehicle->scenario_placement_offset);
            TagReflexive *palette = (TagReflexive *)(scenario + vehicle->scenario_palette_offset);
            int16_t i;

            for (i = 0; i < (int32_t)placements->count; i++) {
                uint8_t *placement = (uint8_t *)placements->pointer + i * size;
                int16_t kind = *(int16_t *)placement;
                if (kind != -1) {
                    halo::input::control_binding_table_register_single(palette_tag(palette, kind), placement[0x58], i,
                                                          (uint32_t)*(int16_t *)(placement + 0x5a));
                }
            }
        }
        if (current_game_engine != 0) {
            if (halo::input::globals().binding_secondary_active) {
                halo::input::control_binding_table_update_b();
            } else {
                halo::input::control_binding_table_update_a();
            }
        }
        halo::input::globals().binding_state = 1;
    }

    for (type = 0; type < k_maximum_object_types; type++) {
        object_type_definition *definition;
        TagReflexive *placements;
        TagReflexive *palette;
        int32_t size;
        int16_t i;

        if (network_game_mode == 1 && type == _object_type_vehicle) {
            continue;
        }
        if (((1 << type) & 0x240) != 0) {
            continue;
        }
        definition = object_type_definitions[type];
        if (definition->scenario_placement_offset == -1 || definition->scenario_palette_offset == -1) {
            continue;
        }
        size = definition->scenario_placement_size;
        placements = (TagReflexive *)(scenario + definition->scenario_placement_offset);
        palette = (TagReflexive *)(scenario + definition->scenario_palette_offset);
        for (i = 0; i < (int32_t)placements->count; i++) {
            uint8_t *placement = (uint8_t *)placements->pointer + i * size;
            datum_index object;

            if (type == _object_type_vehicle) {
                if (joining || !halo::input::control_binding_table_query(palette_tag(palette, *(int16_t *)placement), i)) {
                    continue;
                }
            }
            if (placement == 0) {
                continue;
            }
            object = object_new_from_scenario_placement(placement, palette);
            if (object != k_datum_index_none && type == _object_type_vehicle) {
                uint8_t *vehicle = *(uint8_t **)((uint8_t *)object_data->data + (object & 0xffff) * 0xc + 8);
                ((vehicle_object *)vehicle)->vehicle.cinematic_facing_index = i;
            }
            objects_garbage_collection();
        }
    }
    scenario_objects_place_for_structure_bsp(1);
}

/**
 * Places the scenario objects of a BSP when it is activated.
 *
 * Original register convention: no arguments.
 *
 * @address 0x004f4860
 */
void halo::objects::ObjectFactory::place_for_structure_bsp_on_activate()
{
    if (cinematic_globals_ptr->in_progress == 0 || cinematic_globals_ptr->suppress_bsp_object_creation == 0) {
        scenario_objects_place_for_structure_bsp(1);
    }
}

/**
 * Places the scenario objects belonging to the structure BSP.
 *
 * Original register convention: stack -> place.
 *
 * @address 0x004f4880
 */
void halo::objects::ObjectFactory::place_for_structure_bsp(uint8_t place)
{
    int16_t type;
    uint16_t bsp_bit;

    if (global_structure_bsp_index == -1) {
        return;
    }
    bsp_bit = (uint16_t)(1 << global_structure_bsp_index);
    for (type = 0; type < k_maximum_object_types; type++) {
        object_type_definition *definition = object_type_definitions[type];
        TagReflexive *placements;
        TagReflexive *palette;
        int32_t size;
        int16_t i;

        if (((1 << type) & 0x240) == 0 || definition->scenario_placement_offset == -1 ||
            definition->scenario_palette_offset == -1) {
            continue;
        }
        size = definition->scenario_placement_size;
        placements = (TagReflexive *)(global_scenario + definition->scenario_placement_offset);
        palette = (TagReflexive *)(global_scenario + definition->scenario_palette_offset);

        if ((object_visibility_computed_mask & bsp_bit) == 0) {
            for (i = 0; i < (int32_t)placements->count; i++) {
                uint8_t *placement = (uint8_t *)placements->pointer + i * size;
                int16_t kind = *(int16_t *)placement;
                real_matrix4x3 basis;
                real_point3d origin;
                datum_index tag;
                uint8_t *definition_data;

                if (kind == -1) {
                    continue;
                }
                halo::math::matrix4x3_from_euler_angles(basis, *(float *)(placement + 0x14), *(float *)(placement + 0x18),
                                            *(float *)(placement + 0x1c));
                tag = *(datum_index *)((uint8_t *)palette->pointer + kind * 0x30 + 0xc);
                definition_data = (uint8_t *)halo::cache::globals().tag_instances[tag & 0xffff].data;
                halo::math::matrix4x3_transform_point(origin, *(real_point3d *)(definition_data + 8), basis);
                if (halo::physics::bsp3d_node_find_leaf(0, global_collision_bsp, (real_point3d *)(placement + 8)) == 0xffffffff &&
                    halo::physics::bsp3d_node_find_leaf(0, global_collision_bsp, &origin) == 0xffffffff) {
                    *(uint16_t *)(placement + 0x20) &= (uint16_t)~bsp_bit;
                } else {
                    *(uint16_t *)(placement + 0x20) |= bsp_bit;
                }
            }
        }
        if (place) {
            objects_garbage_collection();
            halo::memory::block_list_compact(object_memory_pool);
            for (i = 0; i < (int32_t)placements->count; i++) {
                uint8_t *placement = (uint8_t *)placements->pointer + i * size;
                int16_t name = *(int16_t *)(placement + 2);

                if (name != -1 && name >= 0 && name < 0x200 && object_name_list[name] != k_datum_index_none) {
                    continue;
                }
                if ((placement[4] & 1) != 0 || (*(uint16_t *)(placement + 0x20) & bsp_bit) == 0) {
                    continue;
                }
                object_new_from_scenario_placement(placement, palette);
                objects_garbage_collection();
            }
        }
    }
    object_visibility_computed_mask |= bsp_bit;
}

/**
 * Initialises placement data for a definition tag and role with default orientation and velocities.
 *
 * @address 0x004f53a0
 */
void halo::objects::ObjectPlacementDataView::initialize(datum_index definition_tag, datum_index role)
{
    object_placement_data *placement = self;
    object *current;
    int32_t *raw = (int32_t *)placement;
    int i;

    for (i = 0; i < 0x22; i++) {
        raw[i] = 0;
    }

    placement->definition_tag = definition_tag;
    placement->flags = 0;
    placement->forward = *halo::math::globals().global_forward3d_pointer;
    placement->up = *halo::math::globals().global_up3d_pointer;
    placement->permutation_group = 0;

    current = object_try_and_get(role, _object_mask_all);
    if (current == 0) {
        placement->role = 0xffffffff;
        placement->owner_linkage = 0xffffffff;
        placement->owner_team = -1;
    } else {
        placement->role = role;
        placement->owner_linkage = ((struct object *)current)->owner_linkage;
        placement->owner_team = ((struct object *)current)->owner_team;
    }

    for (i = 0; i < 4; i++) {
        placement->network_vectors[i] = *global_white_color;
    }
}

/**
 * Creates an object from placement data and returns its handle.
 *
 * @address 0x004f5460
 */
datum_index halo::objects::ObjectFactory::create(object_placement_data *placement)
{
    uint32_t role = 3;

    if (network_game_mode == 2) {
        Object *definition = (Object *)halo::cache::globals().tag_instances[(uint16_t)placement->definition_tag].data;
        if (object_type_definitions[definition->object_type]->network_delta_message_type != -1) {
            role = 0;
        }
    }

    return object_new_with_datum_role_control(placement, role);
}

namespace {
static network_server_globals * &network_server__as_object_new_with_datum_role_control = reinterpret_cast<network_server_globals * &>(network_server);
#define TAG_ID_AS_DATUM_INDEX(field) (*(datum_index *)&(field))
}

/**
 * Creates an object from placement data with an explicit network role, running the type hooks and network
 * announcements.
 *
 * @address 0x004f54b0
 */
datum_index halo::objects::ObjectFactory::create_with_role_control(object_placement_data *placement, uint32_t role)
{
    datum_index definition_tag;
    datum_index new_index;
    object_header *header;
    object *obj;
    tag_instance *tag_inst;
    Object *object_tag;
    int active;
    char grew_nodes;
    uint32_t node_count;
    char out_of_objects_message[516];
    char *tag_path;
    char *last_slash;

    definition_tag = placement->definition_tag;

    if (current_game_engine != 0) {
        if (definition_tag == k_datum_index_none) {
            return k_datum_index_none;
        }
        if (role != 1 && role != 2) {
            definition_tag = game_engine_remap_placement_by_type(definition_tag);
        }
    }

    if (definition_tag == k_datum_index_none) {
        return k_datum_index_none;
    }

    tag_inst = &halo::cache::globals().tag_instances[definition_tag & 0xffff];
    object_tag = (Object *)tag_inst->data;

    new_index = object_block_data_new(-1, object_data,
        object_type_definitions[object_tag->object_type]->object_size);
    if (new_index == k_datum_index_none) {
        goto out_of_objects;
    }

    header = (object_header *)object_data->data + (new_index & 0xffff);
    header->flags |= _object_header_in_pvs_pass_bit | _object_header_needs_update_bit;
    header->type = (uint8_t)object_tag->object_type;
    obj = header->data;
    obj->definition_tag = definition_tag;
    active = 1;
    obj->type = object_tag->object_type;

    object_type_definitions_notify_0x24(new_index, (uint32_t)placement);

    obj->network_role = role;
    obj->network_position_valid = 0;
    obj->network_update_tick = -1;

    obj->position = placement->position;
    obj->forward = placement->forward;
    obj->up = placement->up;
    obj->velocity = placement->velocity;
    obj->angular_velocity = placement->angular_velocity;

    obj->position.x += placement->height_above_origin * obj->up.i;
    obj->position.y += placement->height_above_origin * obj->up.j;
    obj->position.z += placement->height_above_origin * obj->up.k;

    if ((placement->flags & 1) == 0) {
        obj->flags &= ~(uint32_t)_object_mirrored_geometry_bit;
    } else {
        obj->flags |= _object_mirrored_geometry_bit;
    }

    obj->location_cluster_index = -1;
    header->cluster_index = -1;
    obj->cluster_stamp = halo::physics::globals().object_cluster_stamp - 1;
    obj->damage_owner = k_datum_index_none;
    obj->placement_id = k_datum_index_none;
    obj->animation_index = -1;
    obj->animation_graph = TAG_ID_AS_DATUM_INDEX(object_tag->animation_graph.tag_id);
    obj->cached_render_state_index = -1;
    obj->parent_object = k_datum_index_none;
    obj->next_object = k_datum_index_none;
    obj->first_child_object = k_datum_index_none;
    obj->render_cache_slot = -1;
    obj->shield_damage_ticks = -1;
    obj->body_damage_ticks = -1;

    if ((object_tag->flags & 1) != 0) {
        obj->flags |= _object_definition_flag0_bit;
    }
    if (TAG_ID_AS_DATUM_INDEX(object_tag->collision_model.tag_id) == k_datum_index_none) {
        obj->flags &= ~(uint32_t)_object_has_collision_model_bit;
    } else {
        obj->flags |= _object_has_collision_model_bit;
    }

    object_set_collision_enabled(new_index,
        (uint8_t)(TAG_ID_AS_DATUM_INDEX(object_tag->model.tag_id) != k_datum_index_none));

    obj->owner_team = (int16_t)placement->owner_team;
    obj->owner_linkage = placement->owner_linkage;
    obj->creator_object = placement->role;
    *(int16_t *)((uint8_t *)obj + 0xbe) = placement->permutation_group;
    obj->forced_shader_permutation = (uint16_t)object_tag->forced_shader_permutation_index;

    if (TAG_ID_AS_DATUM_INDEX(object_tag->model.tag_id) == k_datum_index_none) {
        node_count = 1;
    } else {
        GBXModel *model = (GBXModel *)halo::cache::globals().tag_instances[
            TAG_ID_AS_DATUM_INDEX(object_tag->model.tag_id) & 0xffff].data;
        node_count = model->nodes.count;
    }

    grew_nodes = object_block_data_grow(new_index, 0x1f0, (int16_t)(node_count * 0x34));
    if (grew_nodes == 0) {
        active = 0;
    } else if (((1 << (object_tag->object_type & 0x1f)) & _object_mask_no_node_functions) == 0) {
        grew_nodes = object_block_data_grow(new_index, 0x1ec, (int16_t)(node_count << 5));
        if (grew_nodes == 0 || (grew_nodes = object_block_data_grow(new_index, 0x1e8, (int16_t)(node_count << 5)),
                                 grew_nodes == 0)) {
            active = 0;
        }
    }

    header = (object_header *)object_data->data + (new_index & 0xffff);
    obj = header->data;

    if (active && object_type_definitions_query_0x28(new_index) != 0) {
        int was_connected_to_map = (obj->flags & _object_connected_to_map_bit) != 0;

        if (was_connected_to_map && (placement->flags & 2) != 0) {
            obj->flags &= ~(uint32_t)_object_connected_to_map_bit;
        }

        object_initialize_change_colors(new_index, (ColorRGB *)placement->network_vectors);
        object_refresh_region_permutations(new_index);
        object_initialize_shield_stun_thresholds(new_index, 0, 0);
        object_recalculate_bounding_radius(new_index);
        object_set_cluster_and_parent(new_index, 0);
        object_notify_node_array_if_animated(new_index);
        object_type_definitions_notify_0x38(new_index);
        object_update_functions(new_index);
        object_update_change_colors(new_index);
        widget_new(new_index);
        object_create_attachments(new_index);

        if (!was_connected_to_map) {
            obj->flags &= ~(uint32_t)_object_connected_to_map_bit;
        } else {
            obj->flags |= _object_connected_to_map_bit;
        }

        if ((header->flags & _object_header_active_bit) == 0 &&
            (obj->flags & _object_connected_to_map_bit) != 0 &&
            ((placement->flags & 2) == 0 || obj->location_cluster_index != -1)) {
            object_delete(new_index);
        }
    } else {
        active = 0;
    }

    if (network_action_apply_active == 0 && active) {
        if (network_game_mode == 2 && obj->network_role == 0) {
            int32_t override_count;
            object_type_override_call_0x68(new_index);
            override_count = object_type_override_get_0x64(new_index, network_message_scratch,
                                                           sizeof network_message_scratch);
            if (override_count > 0) {

                network_session_broadcast_to_flagged(override_count, network_server__as_object_new_with_datum_role_control, 1, network_message_scratch,
                    1, 0, 0, 3);
            }
        }
    } else if (!active) {
        object_type_definitions_notify_0x30(new_index);
        object_block_data_free(object_data, new_index);
        new_index = k_datum_index_none;
out_of_objects:
        tag_path = halo::cache::globals().tag_instances[(uint16_t)(uint32_t)definition_tag].path;
        last_slash = strrchr(tag_path, '\\');
        if (last_slash != 0) {
            tag_path = last_slash + 1;
        }
        sprintf(out_of_objects_message, "OUT OF OBJECTS: cannot create %s", tag_path);
        console_print_error_va(0, network_log_path_format, out_of_objects_message);
        return new_index;
    }

    if (TAG_ID_AS_DATUM_INDEX(object_tag->creation_effect.tag_id) != k_datum_index_none) {

        halo::effects::effect_new_on_object(new_index, TAG_ID_AS_DATUM_INDEX(object_tag->creation_effect.tag_id), new_index, -1,
            0.0f, 0.0f, (const ColorRGB *)0, (const effect_tint_source *)0);
        return new_index;
    }
    return new_index;
}
#undef TAG_ID_AS_DATUM_INDEX

/**
 * Creates the object registered under a scenario name.
 *
 * Original register convention: CX -> name_index.
 *
 * @address 0x004f7370
 */
datum_index halo::objects::ObjectFactory::create_from_scenario_name(int16_t name_index)
{
    uint8_t *name = *(uint8_t **)(global_scenario + 0x208) + name_index * 0x24;
    object_type_definition *definition = object_type_definitions[*(int16_t *)(name + 0x20)];
    TagReflexive *placements = (TagReflexive *)(global_scenario + definition->scenario_placement_offset);
    TagReflexive *palette = (TagReflexive *)(global_scenario + definition->scenario_palette_offset);
    uint8_t *placement = (uint8_t *)placements->pointer +
                         definition->scenario_placement_size * *(int16_t *)(name + 0x22);

    return object_new_from_scenario_placement(placement, palette);
}

/**
 * Returns the handle registered for a scenario object name.
 *
 * Original register convention: index in AX. Confirmed against objdump -d -M intel bin/halo.exe: 0x4f73c5 cmp
 * ax,0x200 at entry, no stack access. // blam-cc: AX -> name_index.
 *
 * @address 0x004f73c0
 */
datum_index halo::objects::ObjectFactory::lookup_by_name(int16_t name_index)
{
    if (name_index >= 0 && name_index < k_maximum_object_names) {
        return object_name_list[name_index];
    }
    return k_datum_index_none;
}

/**
 * Touches the predicted resources of an object definition tag when it is valid.
 *
 * @address 0x004f7ad0
 */
void halo::objects::ObjectFactory::notify_predicted_resources_if_valid(datum_index definition_tag)
{
    if (definition_tag != k_datum_index_none) {
        uint8_t *tag_data = (uint8_t *)halo::cache::globals().tag_instances[definition_tag & 0xffff].data;
        halo::cache::predicted_resource_list_touch((TagReflexive *)(tag_data + 0x170));
    }
}

/**
 * Creates an object from a scenario placement block using the given palette.
 *
 * Original register convention: EDI -> placement, stack -> palette.
 *
 * @address 0x004f9b70
 */
datum_index halo::objects::ObjectFactory::create_from_scenario_placement(uint8_t *placement, TagReflexive *palette)
{
    int16_t type = *(int16_t *)placement;
    int16_t name = *(int16_t *)(placement + 2);
    datum_index tag;
    datum_index object;
    object_placement_data data;

    if (type == -1) {
        return k_datum_index_none;
    }
    if (*(uint8_t *)object_globals_pointer != 0 && (placement[4] & 1) != 0) {
        return k_datum_index_none;
    }
    if (name != -1 && name >= 0 && name < 0x200 && object_name_list[name] != k_datum_index_none) {
        return k_datum_index_none;
    }
    tag = *(datum_index *)((uint8_t *)palette->pointer + type * 0x30 + 0xc);
    if (tag == k_datum_index_none) {
        return k_datum_index_none;
    }
    object_placement_data_initialize(&data, tag, k_datum_index_none);
    data.position = *(real_point3d *)&((struct object_placement_data *)placement)->owner_linkage;
    halo::math::euler_angles_to_basis_vectors(*(real_euler_angles3d *)(placement + 0x14), data.up, data.forward);
    data.permutation_group = *(int16_t *)(placement + 0x06);
    object = object_new(&data);
    if (object != k_datum_index_none) {
        object_type_definitions_notify_two_args_0x2c(object, (uint32_t)placement);
        if (name != -1) {
            object_reserve_render_cache_slot(object, name);
        }
    }
    return object;
}

/**
 * Type hook run when a scenery object is created.
 *
 * Original register convention: stack -> object_index (cdecl); returns AL.
 *
 * @address 0x004fa7e0
 */
uint8_t halo::objects::SceneryObject::initialize()
{
    datum_index object_index = handle;
    uint8_t *object = *(uint8_t **)((uint8_t *)object_data->data + (object_index & 0xffff) * 0xc + 8);
    uint8_t *definition = (uint8_t *)halo::cache::globals().tag_instances[*(datum_index *)object & 0xffff].data;
    datum_index graph = *(datum_index *)&((struct Object *)definition)->animation_graph.tag_id;

    if (graph != k_datum_index_none && *(int32_t *)((uint8_t *)halo::cache::globals().tag_instances[graph & 0xffff].data + 0x74) > 0) {
        int16_t animation = halo::models::animation_choose_random_permutation(graph, 0, static_cast<animation_random_stream>(1));
        if (animation != -1) {
            *(int16_t *)(object + 0xd0) = animation;
            *(datum_index *)(object + 0xcc) = *(datum_index *)&((struct Object *)definition)->animation_graph.tag_id;
            *(uint32_t *)(object + 0x10) |= 0x80;
        }
    }
    *(uint32_t *)(object + 0x10) |= 0x40000;
    return 1;
}

namespace {
static uint8_t *object_get(datum_index object_index)
{
    return *(uint8_t **)((uint8_t *)object_data->data + (object_index & 0xffff) * 0xc + 8);
}
}

/**
 * Per-tick update of a scenery object.
 *
 * Original register convention: stack -> object_index (cdecl); returns AL.
 *
 * @address 0x004fa870
 */
uint8_t halo::objects::SceneryObject::update()
{
    datum_index object_index = handle;
    uint8_t *object = object_get(object_index);

    if ((object[0x1f4] & 1) != 0 &&
        halo::models::animation_state_advance(*(uint32_t *)(object + 0xcc), reinterpret_cast<animation_state *>(object + 0xd0), 0, static_cast<animation_random_stream>(1)) == 2) {
        *(int16_t *)(object + 0xd2) -= 1;
    }
    return 1;
}
