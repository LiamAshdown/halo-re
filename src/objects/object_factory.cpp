#include "halo/networking/game_mode.hpp"
#include "halo/objects/record_access.hpp"
#include "halo/objects/object_factory.hpp"
#include "halo/objects/scenario_placement.hpp"
#include "halo/tags/flags.hpp"
#include "halo/objects/flags.hpp"
#include "halo/core/flag_bits.hpp"
#include "halo/core/network_constants.hpp"
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
#include "halo/cutscene/api.hpp"
#include "halo/scenario/api.hpp"
#include "halo/main/api.hpp"
#include "halo/objects/api.hpp"
#include "halo/networking/api.hpp"
#include "halo/game/api.hpp"
#include "halo/core/link.hpp"
#include "halo/effects/vars.hpp"
#include "halo/game/vars.hpp"
#include "halo/hs/vars.hpp"
#include "halo/networking/vars.hpp"
#include "halo/objects/vars.hpp"
#include "halo/physics/vars.hpp"
#include "halo/hs/api.hpp"

static auto &global_collision_bsp = halo::link::ref<ModelCollisionGeometryBSP *>(halo::physics::vars().global_collision_bsp);
static auto &global_scenario = halo::link::ref<uint8_t *>(halo::hs::vars().global_scenario);
static auto &global_white_color = halo::link::ref<const real_vector3d *>(halo::effects::vars().global_white_color);
static auto &network_action_apply_active = halo::link::ref<uint8_t>(halo::objects::vars().network_action_apply_active);
static auto &network_client = halo::link::ref<int32_t>(halo::networking::vars().network_client);
static auto &network_log_path_format = halo::link::ref<char []>(halo::networking::vars().network_log_path_format);
static auto &network_message_scratch = halo::link::ref<uint8_t [halo::k_network_message_scratch_size]>(halo::game::vars().network_message_scratch);
static auto &network_server = halo::link::ref<int32_t>(halo::networking::vars().network_server);
static auto &object_data = halo::link::ref<data_array *>(halo::objects::vars().object_data);
static auto &object_globals_pointer = halo::link::ref<object_globals *>(halo::objects::vars().object_globals_pointer);
static auto &object_memory_pool = halo::link::ref<memory_pool *>(halo::objects::vars().object_memory_pool);
static auto &object_name_list = halo::link::ref<datum_index *>(halo::objects::vars().object_name_list);
static auto &object_type_definitions = halo::link::ref<object_type_definition *[k_maximum_object_types]>(halo::game::vars().object_type_definitions);
static auto &object_visibility_computed_mask = halo::link::ref<uint16_t>(halo::objects::vars().object_visibility_computed_mask);

namespace {
static datum_index palette_tag(TagReflexive *palette, int16_t type)
{
    return halo::objects::palette_tag_of(*palette, type);
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
        if (halo::networking::globals().game_mode == halo::networking::k_game_mode_host) {
            object_type_definition *vehicle = object_type_definitions[_object_type_vehicle];
            int32_t size = vehicle->scenario_placement_size;
            TagReflexive *placements = (TagReflexive *)(scenario + vehicle->scenario_placement_offset);
            TagReflexive *palette = (TagReflexive *)(scenario + vehicle->scenario_palette_offset);
            int16_t i;

            for (i = 0; i < (int32_t)placements->count; i++) {
                ScenarioVehicle &placement = reinterpret_cast<ScenarioVehicle &>(halo::objects::placement_at(*placements, i, size));
                int16_t kind = static_cast<int16_t>(placement.type);
                if (kind != -1) {
                    halo::input::control_binding_table_register_single(palette_tag(palette, kind), static_cast<uint8_t>(placement.multiplayer_team_index), i,
                                                          (uint32_t)static_cast<int16_t>(placement.multiplayer_spawn_flags));
                }
            }
        }
        if (halo::game::globals().current_engine != 0) {
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

        if (halo::networking::globals().game_mode == halo::networking::k_game_mode_client && type == _object_type_vehicle) {
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
            scenario_placement_header *placement = &halo::objects::placement_at(*placements, i, size);
            datum_index object;

            if (type == _object_type_vehicle) {
                if (joining || !halo::input::control_binding_table_query(palette_tag(palette, placement->type), i)) {
                    continue;
                }
            }
            if (placement == 0) {
                continue;
            }
            object = halo::objects::object_new_from_scenario_placement(reinterpret_cast<uint8_t *>(placement), palette);
            if (object != k_datum_index_none && type == _object_type_vehicle) {
                vehicle_object *vehicle = halo::objects::object_as<vehicle_object>(object);
                vehicle->vehicle.cinematic_facing_index = i;
            }
            halo::objects::objects_garbage_collection();
        }
    }
    halo::objects::scenario_objects_place_for_structure_bsp(1);
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
    if (halo::cutscene::globals().cinematic_globals->in_progress == 0 || halo::cutscene::globals().cinematic_globals->suppress_bsp_object_creation == 0) {
        halo::objects::scenario_objects_place_for_structure_bsp(1);
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

    if (halo::scenario::globals().structure_bsp_index == -1) {
        return;
    }
    bsp_bit = (uint16_t)(1 << halo::scenario::globals().structure_bsp_index);
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
                scenario_placement_header *placement = &halo::objects::placement_at(*placements, i, size);
                int16_t kind = placement->type;
                real_matrix4x3 basis;
                real_point3d origin;
                datum_index tag;
                uint8_t *definition_data;

                if (kind == -1) {
                    continue;
                }
                halo::math::matrix4x3_from_euler_angles(basis, placement->rotation.yaw, placement->rotation.pitch,
                                            placement->rotation.roll);
                tag = palette_tag(palette, kind);
                definition_data = halo::objects::tag_record_bytes(tag);
                halo::math::matrix4x3_transform_point(origin, *((real_point3d *)(definition_data + 8)), basis);
                if (halo::physics::bsp3d_node_find_leaf(0, global_collision_bsp, reinterpret_cast<real_point3d *>(&placement->position)) == k_datum_index_none &&
                    halo::physics::bsp3d_node_find_leaf(0, global_collision_bsp, &origin) == k_datum_index_none) {
                    placement->bsp_indices &= (uint16_t)~bsp_bit;
                } else {
                    placement->bsp_indices |= bsp_bit;
                }
            }
        }
        if (place) {
            halo::objects::objects_garbage_collection();
            halo::memory::block_list_compact(object_memory_pool);
            for (i = 0; i < (int32_t)placements->count; i++) {
                scenario_placement_header *placement = &halo::objects::placement_at(*placements, i, size);
                int16_t name = placement->name;

                if (name != -1 && name >= 0 && name < 0x200 && object_name_list[name] != k_datum_index_none) {
                    continue;
                }
                if (test_flag(placement->not_placed, scenario_not_placed_flag::automatically) || (placement->bsp_indices & bsp_bit) == 0) {
                    continue;
                }
                halo::objects::object_new_from_scenario_placement(reinterpret_cast<uint8_t *>(placement), palette);
                halo::objects::objects_garbage_collection();
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

    current = halo::objects::object_try_and_get(role, _object_mask_all);
    if (current == 0) {
        placement->role = k_datum_index_none;
        placement->owner_linkage = k_datum_index_none;
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

    if (halo::networking::globals().game_mode == halo::networking::k_game_mode_host) {
        Object *definition = (Object *)halo::cache::globals().tag_instances[(uint16_t)placement->definition_tag].data;
        if (object_type_definitions[definition->object_type]->network_delta_message_type != -1) {
            role = 0;
        }
    }

    return halo::objects::object_new_with_datum_role_control(placement, role);
}

namespace {
static network_server_globals * &network_server__as_object_new_with_datum_role_control = reinterpret_cast<network_server_globals * &>(network_server);
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

    if (halo::game::globals().current_engine != 0) {
        if (definition_tag == k_datum_index_none) {
            return k_datum_index_none;
        }
        if (role != 1 && role != 2) {
            definition_tag = halo::game::game_engine_remap_placement_by_type(definition_tag);
        }
    }

    if (definition_tag == k_datum_index_none) {
        return k_datum_index_none;
    }

    tag_inst = &halo::cache::globals().tag_instances[halo::datum_slot(definition_tag)];
    object_tag = (Object *)tag_inst->data;

    auto report_out_of_objects = [&]() -> datum_index {
        tag_path = halo::cache::globals().tag_instances[(uint16_t)(uint32_t)definition_tag].path;
        last_slash = strrchr(tag_path, '\\');
        if (last_slash != 0) {
            tag_path = last_slash + 1;
        }
        sprintf(out_of_objects_message, "OUT OF OBJECTS: cannot create %s", tag_path);
        halo::main::console_print_error_va(0, network_log_path_format, out_of_objects_message);
        return new_index;
    };

    new_index = halo::objects::object_block_data_new(-1, object_data,
        object_type_definitions[object_tag->object_type]->object_size);
    if (new_index == k_datum_index_none) {
        return report_out_of_objects();
    }

    header = (object_header *)object_data->data + halo::datum_slot(new_index);
    header->flags |= _object_header_in_pvs_pass_bit | _object_header_needs_update_bit;
    header->type = (uint8_t)object_tag->object_type;
    obj = header->data;
    obj->definition_tag = definition_tag;
    active = 1;
    obj->type = object_tag->object_type;

    halo::objects::object_type_definitions_notify_0x24(new_index, (uint32_t)placement);

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
    obj->animation_graph = halo::objects::tag_handle(object_tag->animation_graph);
    obj->cached_render_state_index = -1;
    obj->parent_object = k_datum_index_none;
    obj->next_object = k_datum_index_none;
    obj->first_child_object = k_datum_index_none;
    obj->render_cache_slot = -1;
    obj->shield_damage_ticks = -1;
    obj->body_damage_ticks = -1;

    if (test_flag(object_tag->flags, tags::object_tag_flag::does_not_cast_shadow)) {
        obj->flags |= _object_definition_flag0_bit;
    }
    if (halo::objects::tag_handle(object_tag->collision_model) == k_datum_index_none) {
        obj->flags &= ~(uint32_t)_object_has_collision_model_bit;
    } else {
        obj->flags |= _object_has_collision_model_bit;
    }

    halo::objects::object_set_collision_enabled(new_index,
        (uint8_t)(halo::objects::tag_handle(object_tag->model) != k_datum_index_none));

    obj->owner_team = (int16_t)placement->owner_team;
    obj->owner_linkage = placement->owner_linkage;
    obj->creator_object = placement->role;
    obj->permutation_group = placement->permutation_group;
    obj->forced_shader_permutation = (uint16_t)object_tag->forced_shader_permutation_index;

    if (halo::objects::tag_handle(object_tag->model) == k_datum_index_none) {
        node_count = 1;
    } else {
        GBXModel *model = (GBXModel *)halo::cache::globals().tag_instances[halo::datum_slot(halo::objects::tag_handle(object_tag->model))].data;
        node_count = model->nodes.count;
    }

    grew_nodes = halo::objects::object_block_data_grow(new_index, 0x1f0, (int16_t)(node_count * 0x34));
    if (grew_nodes == 0) {
        active = 0;
    } else if (((1 << (object_tag->object_type & 0x1f)) & _object_mask_no_node_functions) == 0) {
        grew_nodes = halo::objects::object_block_data_grow(new_index, 0x1ec, (int16_t)(node_count << 5));
        if (grew_nodes == 0 || (grew_nodes = halo::objects::object_block_data_grow(new_index, 0x1e8, (int16_t)(node_count << 5)),
                                 grew_nodes == 0)) {
            active = 0;
        }
    }

    header = (object_header *)object_data->data + halo::datum_slot(new_index);
    obj = header->data;

    if (active && halo::objects::object_type_definitions_query_0x28(new_index) != 0) {
        int was_connected_to_map = (obj->flags & _object_connected_to_map_bit) != 0;

        if (was_connected_to_map && (placement->flags & 2) != 0) {
            obj->flags &= ~(uint32_t)_object_connected_to_map_bit;
        }

        halo::objects::object_initialize_change_colors(new_index, (ColorRGB *)placement->network_vectors);
        halo::objects::object_refresh_region_permutations(new_index);
        halo::objects::object_initialize_shield_stun_thresholds(new_index, 0, 0);
        halo::objects::object_recalculate_bounding_radius(new_index);
        halo::objects::object_set_cluster_and_parent(new_index, 0);
        halo::objects::object_notify_node_array_if_animated(new_index);
        halo::objects::object_type_definitions_notify_0x38(new_index);
        halo::objects::object_update_functions(new_index);
        halo::objects::object_update_change_colors(new_index);
        halo::objects::widget_new(new_index);
        halo::objects::object_create_attachments(new_index);

        if (!was_connected_to_map) {
            obj->flags &= ~(uint32_t)_object_connected_to_map_bit;
        } else {
            obj->flags |= _object_connected_to_map_bit;
        }

        if ((header->flags & _object_header_active_bit) == 0 &&
            (obj->flags & _object_connected_to_map_bit) != 0 &&
            ((placement->flags & 2) == 0 || obj->location_cluster_index != -1)) {
            halo::objects::object_delete(new_index);
        }
    } else {
        active = 0;
    }

    if (network_action_apply_active == 0 && active) {
        if (halo::networking::globals().game_mode == halo::networking::k_game_mode_host && obj->network_role == 0) {
            int32_t override_count;
            halo::objects::object_type_override_call_0x68(new_index);
            override_count = halo::objects::object_type_override_get_0x64(new_index, network_message_scratch,
                                                           sizeof network_message_scratch);
            if (override_count > 0) {

                halo::networking::network_session_broadcast_to_flagged(override_count, network_server__as_object_new_with_datum_role_control, 1, network_message_scratch,
                    1, 0, 0, 3);
            }
        }
    } else if (!active) {
        halo::objects::object_type_definitions_notify_0x30(new_index);
        halo::objects::object_block_data_free(object_data, new_index);
        new_index = k_datum_index_none;
        return report_out_of_objects();
    }

    if (halo::objects::tag_handle(object_tag->creation_effect) != k_datum_index_none) {

        halo::effects::effect_new_on_object(new_index, halo::objects::tag_handle(object_tag->creation_effect), new_index, -1,
            0.0f, 0.0f, nullptr, nullptr);
        return new_index;
    }
    return new_index;
}

/**
 * Creates the object registered under a scenario name.
 *
 * Original register convention: CX -> name_index.
 *
 * @address 0x004f7370
 */
datum_index halo::objects::ObjectFactory::create_from_scenario_name(int16_t name_index)
{
    ScenarioObjectName &name = halo::objects::block_element<ScenarioObjectName>(reinterpret_cast<Scenario *>(global_scenario)->object_names, name_index);
    object_type_definition *definition = object_type_definitions[static_cast<int16_t>(name.object_type)];
    TagReflexive *placements = (TagReflexive *)(global_scenario + definition->scenario_placement_offset);
    TagReflexive *palette = (TagReflexive *)(global_scenario + definition->scenario_palette_offset);
    scenario_placement_header *placement = &halo::objects::placement_at(*placements, static_cast<int16_t>(name.object_index), definition->scenario_placement_size);

    return halo::objects::object_new_from_scenario_placement(reinterpret_cast<uint8_t *>(placement), palette);
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
        uint8_t *tag_data = halo::objects::tag_record_bytes(definition_tag);
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
datum_index halo::objects::ObjectFactory::create_from_scenario_placement(uint8_t *placement_bytes, TagReflexive *palette)
{
    scenario_placement_header *placement = reinterpret_cast<scenario_placement_header *>(placement_bytes);
    int16_t type = placement->type;
    int16_t name = placement->name;
    datum_index tag;
    datum_index object;
    object_placement_data data;

    if (type == -1) {
        return k_datum_index_none;
    }
    if (*(uint8_t *)object_globals_pointer != 0 && test_flag(placement->not_placed, scenario_not_placed_flag::automatically)) {
        return k_datum_index_none;
    }
    if (name != -1 && name >= 0 && name < 0x200 && object_name_list[name] != k_datum_index_none) {
        return k_datum_index_none;
    }
    tag = palette_tag(palette, type);
    if (tag == k_datum_index_none) {
        return k_datum_index_none;
    }
    halo::objects::object_placement_data_initialize(&data, tag, k_datum_index_none);
    data.position = *reinterpret_cast<real_point3d *>(&placement->position);
    halo::math::euler_angles_to_basis_vectors(reinterpret_cast<const real_euler_angles3d &>(placement->rotation), data.up, data.forward);
    data.permutation_group = placement->desired_permutation;
    object = halo::objects::object_new(&data);
    if (object != k_datum_index_none) {
        halo::objects::object_type_definitions_notify_two_args_0x2c(object, (uint32_t)placement_bytes);
        if (name != -1) {
            halo::objects::object_reserve_render_cache_slot(object, name);
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
    uint8_t *object = halo::objects::object_record_bytes(object_index);
    Object *definition = halo::objects::tag_as<Object>(*(datum_index *)object);
    datum_index graph = halo::objects::tag_handle(definition->animation_graph);

    if (graph != k_datum_index_none && static_cast<int32_t>(halo::objects::tag_as<ModelAnimations>(graph)->animations.count) > 0) {
        int16_t animation = halo::models::animation_choose_random_permutation(graph, 0, (animation_random_stream)1);
        if (animation != -1) {
            ((struct object *)object)->animation_index = animation;
            ((struct object *)object)->animation_graph = halo::objects::tag_handle(definition->animation_graph);
            set_flag(((struct object *)object)->flags, objects::object_flag::animates_automatically);
        }
    }
    set_flag(((struct object *)object)->flags, objects::object_flag::definition_flag0);
    return 1;
}

namespace {
static uint8_t *object_get(datum_index object_index)
{
    return halo::objects::object_record_bytes(object_index);
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
        halo::models::animation_state_advance(((struct object *)object)->animation_graph, (animation_state *)&((struct object *)object)->animation_index, 0, (animation_random_stream)1) == 2) {
        ((struct object *)object)->animation_frame -= 1;
    }
    return 1;
}
