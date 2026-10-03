#include "halo/devices/machine.hpp"
#include "halo/core/datum.hpp"
#include "halo/cache/api.hpp"
#include "halo/devices/api.hpp"

extern "C" {
extern data_array *object_data;
extern data_array *device_groups;
extern game_time_globals *game_time;
extern game_engine_definition *current_game_engine;
extern void *team_pair_data;
extern int16_t object_find_in_sphere(uint32_t search_mask, uint32_t type_mask, void *location, real_point3d *center, float radius, datum_index *out_objects, int16_t max_output);
extern void object_set_cluster_and_parent(uint32_t object_index, bsp_leaf_reference *location);
extern void object_unlink_cluster_or_notify_parent(uint32_t object_index);
}

namespace {

static uint8_t *object_get(datum_index object_index)
{
    return (uint8_t *)((object_header *)object_data->data)[halo::datum_slot(object_index)].data;
}

static uint8_t *object_definition(uint8_t *object)
{
    return (uint8_t *)halo::cache::globals().tag_instances[halo::datum_slot(*(datum_index *)object)].data;
}

}

namespace halo::devices {

/**
 * Original function machine_create; the author notes are in
 * docs/original/devices/machine_create.c.txt.
 *
 * @address 0x44b020
 */
uint8_t MachineHandle::create()
{
    datum_index object_index = (datum_index)handle;

    uint8_t *object = object_get(object_index);
    uint8_t *definition = object_definition(object);
    uint32_t *flags = &((struct object *)object)->flags;
    constexpr uint32_t elevator_bits = to_bits(machine_object_flags::unknown_4000 | machine_object_flags::unknown_8000);

    *flags |= _object_unknown_2000_bit;
    if ((((DeviceMachine *)definition)->machine_flags & to_bits(machine_tag_flags::elevator)) != 0) {
        *flags |= elevator_bits;
    } else {
        *flags &= ~elevator_bits;
    }
    return 1;
}

/**
 * EDI -> placement (the device part of the scenario placement, at +0x28)
 *
 * @address 0x44afa0
 */
void MachineHandle::place(uint8_t *placement)
{
    datum_index object_index = (datum_index)handle;

    uint8_t *obj = object_get(object_index);
    ScenarioMachine *scenario_machine = (ScenarioMachine *)placement;

    halo::devices::device_new(object_index, (device_placement_data *)(&scenario_machine->power_group));
    ((device_object *)obj)->device.type_flags |= scenario_machine->machine_flags & k_machine_placement_flags_mask;
}

/**
 * Original function device_machine_melee_attacked; the author notes are in
 * docs/original/devices/device_machine_melee_attacked.c.txt.
 *
 * Register convention in the original: object index in ECX (in_ECX).
 *
 * @address 0x44b5d0
 */
void MachineHandle::melee_attacked()
{
    uint32_t object_index = (uint32_t)handle;

    object *obj = ((object_header *)object_data->data)[halo::datum_slot(object_index)].data;
    device_machine_data *dev = (device_machine_data *)((uint8_t *)obj + sizeof(object));

    if ((dev->device.type_flags & (1u << _device_machine_opened_by_melee_attack_bit)) != 0 &&
        object_index != (uint32_t)k_datum_index_none &&
        dev->device.position_group != -1) {
        halo::devices::device_group_set_value_immediate((uint16_t)dev->device.position_group, 1.0f);
    }
}

/**
 * Original function device_machine_update; the author notes are in
 * docs/original/devices/device_machine_update.c.txt.
 *
 * Register convention in the original: object index is already a plain, genuinely-stack
 * parameter in Ghidra's own output (`device_machine_update(uint param_1)`); no unresolved
 * registers appear.
 *
 * @address 0x44b0a0
 */
uint32_t MachineHandle::update()
{
    uint32_t object_index = (uint32_t)handle;

    object *obj = ((object_header *)object_data->data)[halo::datum_slot(object_index)].data;
    device_machine_data *dev = (device_machine_data *)((uint8_t *)obj + sizeof(object));
    DeviceMachine *tag = (DeviceMachine *)halo::cache::globals().tag_instances[halo::datum_slot(obj->definition_tag)].data;

    
    
    
    if (tag->machine_type == machinetype_gear) {
        float position = tag->base.inverse_position_transition_time * dev->device.power +
            (1.0f - dev->device.power) * tag->base.inverse_depowered_position_transition_time +
            dev->device.position;
        dev->device.position = position;
        if (1.0f <= position) {
            dev->device.position = position - 1.0f;
        }
        dev->device.position_change = 0.0f;
        dev->device.flags |= (1u << _device_position_changed_bit);
        if (dev->device.position_group != -1) {
            ((device_group *)device_groups->data)[(uint16_t)dev->device.position_group].value =
                dev->device.position;
        }
    }

    
    
    if ((dev->device.type_flags & (1u << _device_machine_does_not_operate_automatically_bit)) == 0 &&
        tag->machine_type == machinetype_door &&
        (game_time->game_time + (int32_t)object_index & 3) == 0) {
        datum_index candidates[k_device_machine_activation_maximum];
        int16_t candidate_count;
        int should_open = 0;
        float radius = (0.0001f <= tag->base.automatic_activation_radius)
            ? tag->base.automatic_activation_radius : obj->bounding_radius;

        candidate_count = object_find_in_sphere(1, 1, &obj->location_leaf_index,
            &obj->bounding_center, radius, candidates, k_device_machine_activation_maximum);
        if (0 < candidate_count) {
            int16_t i;
            for (i = 0; i < candidate_count; i++) {
                object *candidate = ((object_header *)object_data->data)[halo::datum_slot(candidates[i])].data;
                int counts = 1;
                int passes_side_test = 1;

                if (((candidate->vitality_flags & _object_health_frozen_bit) != 0) ||
                    ((((Unit *)halo::cache::globals().tag_instances[halo::datum_slot(candidate->definition_tag)].data)->unit_flags
                        & to_bits(unit_tag_flags::cannot_open_doors_automatically)) != 0)) { 
                    counts = 0;
                }

                if ((dev->device.type_flags & (1u << _device_machine_one_sided_bit)) != 0 &&
                    dev->device.position == 0.0f) {
                    
                    
                    int16_t team = ((struct object *)candidate)->owner_team; 
                    int exempt;
                    if (current_game_engine == 0) {
                        if (team < 0 || 9 < team) {
                            exempt = 1;
                        } else {
                            exempt = (*(uint32_t *)((uint8_t *)team_pair_data + k_team_pair_matrix_offset +
                                ((team + 10) >> 5) * 4) & (1u << ((team + 10) & 0x1f))) == 0;
                        }
                    } else {
                        exempt = team != 1;
                    }
                    passes_side_test = exempt ||
                        (candidate->bounding_center.x - obj->bounding_center.x) * obj->forward.i +
                        (candidate->bounding_center.y - obj->bounding_center.y) * obj->forward.j +
                        (candidate->bounding_center.z - obj->bounding_center.z) * obj->forward.k <= 0.0f;
                }

                if (counts && passes_side_test) {
                    should_open = 1;
                }
            }
            if (should_open) {
                if (dev->device.position_group != -1) {
                    halo::devices::device_group_set_value((uint16_t)dev->device.position_group, 1.0f);
                    
                }
                dev->ticks_since_fully_open = k_device_machine_open_grace_ticks;
            }
        }
    }

    
    if (tag->machine_type == machinetype_door) {
        if (dev->device.position == 1.0f) {
            dev->ticks_since_fully_open++;
            if ((int32_t)tag->door_open_time_ticks < dev->ticks_since_fully_open &&
                dev->device.position_group != -1) {
                halo::devices::device_group_set_value((uint16_t)dev->device.position_group, 0.0f);
                
            }
        } else {
            dev->ticks_since_fully_open = 0;
        }
    }

    
    
    
    if ((tag->machine_flags & to_bits(machine_tag_flags::elevator)) != 0) {
        if (tag->elevator_node != halo::k_word_none) {
            
            
            real_matrix4x3 *node = (real_matrix4x3 *)((uint8_t *)obj + obj->nodes.offset) +
                (int16_t)tag->elevator_node;
            float dx = node->position.x - dev->last_elevator_position.x;
            float dy = node->position.y - dev->last_elevator_position.y;
            float dz = node->position.z - dev->last_elevator_position.z;

            if (dx != 0.0f || dy != 0.0f || dz != 0.0f) {
                datum_index riders[k_device_machine_rider_maximum];
                int16_t rider_count = object_find_in_sphere(1, 1, &obj->location_leaf_index,
                    &obj->bounding_center, obj->bounding_radius, riders, k_device_machine_rider_maximum);
                if (0 < rider_count) {
                    int16_t i;
                    for (i = 0; i < rider_count; i++) {
                        object *rider = ((object_header *)object_data->data)[halo::datum_slot(riders[i])].data;
                        biped_data *rider_biped = (biped_data *)((uint8_t *)rider + k_unit_object_size);
                        if (rider_biped->last_ground_object_index == object_index) { 
                            real_point3d p = rider->position;
                            object_unlink_cluster_or_notify_parent(riders[i]);
                            rider->position.x = p.x + dx;
                            rider->position.y = p.y + dy;
                            rider->position.z = p.z + dz;
                            object_set_cluster_and_parent(riders[i], 0);
                        }
                    }
                }
            }
            dev->last_elevator_position = node->position;
        }
    }

    
    
    
    if ((dev->device.flags & (1u << _device_position_changed_bit)) != 0) {
        object_unlink_cluster_or_notify_parent(object_index);
        obj->position = obj->position;
        object_set_cluster_and_parent(object_index, 0);
        dev->device.flags &= ~(uint32_t)(1u << _device_position_changed_bit);
    }

    return 1;
}

/**
 * EDI -> placement (the device part of the scenario placement, at +0x28)
 *
 * @address 0x44ad90
 */
void ControlHandle::place(uint8_t *placement)
{
    datum_index object_index = (datum_index)handle;

    uint8_t *obj = object_get(object_index);
    ScenarioControl *scenario_control = (ScenarioControl *)placement;

    halo::devices::device_new(object_index, (device_placement_data *)(&scenario_control->power_group));
    if ((scenario_control->control_flags & to_bits(scenario_control_flags::usable_from_both_sides)) != 0) {
        ((device_object *)obj)->device.type_flags |= to_bits(control_type_flags::usable_from_both_sides);
    }
    if ((scenario_control->control_flags & to_bits(scenario_control_flags::unknown_10)) != 0) {
        ((device_object *)obj)->device.type_flags |= to_bits(control_type_flags::unknown_2);
    }
    ((control_object *)obj)->control.custom_name_index =
        (int16_t)(scenario_control->custom_control_name - 1);
}

/**
 * Guard for a device_control's activation: only forwards to device_change_power_state when the
 * control's DeviceControl.triggers_when is touched_by_player. (triggers_when == destroyed is
 * handled elsewhere, outside this module.)
 *
 * Register convention in the original: object id in EAX.
 *
 * @address 0x44adf0
 */
void ControlHandle::activate()
{
    uint32_t object_id = (uint32_t)handle;

    object *obj = ((object_header *)object_data->data)[halo::datum_slot(object_id)].data;
    DeviceControl *tag = (DeviceControl *)halo::cache::globals().tag_instances[halo::datum_slot(obj->definition_tag)].data;

    if (tag->triggers_when == devicetriggerswhen_touched_by_player) {
        halo::devices::device_change_power_state(0.0f, object_id); 
    }
}

/**
 * this batch's address range
 *
 * Register convention in the original: object index in EAX (in_EAX).
 *
 * @address 0x44c090
 */
void ControlHandle::touched()
{
    uint32_t object_index = (uint32_t)handle;

    object *obj = ((object_header *)object_data->data)[halo::datum_slot(object_index)].data;

    if (obj->type == _object_type_device_control) {
        halo::devices::device_control_activate(object_index);
    }
}

/**
 * EDI -> placement (the device part of the scenario placement, at +0x28)
 *
 * @address 0x44af30
 */
void LightFixtureHandle::place(uint8_t *placement)
{
    datum_index object_index = (datum_index)handle;

    uint8_t *object = object_get(object_index);
    ScenarioLightFixture *scenario_light = (ScenarioLightFixture *)placement;
    light_fixture_placement_copy *lights = (light_fixture_placement_copy *)&((device_object *)object)->device.type_flags;

    halo::devices::device_new(object_index, (device_placement_data *)(&scenario_light->power_group));
    lights->color = scenario_light->color;
    lights->intensity = scenario_light->intensity;
    lights->falloff_angle = scenario_light->falloff_angle;
    lights->cutoff_angle = scenario_light->cutoff_angle;
}

}

namespace halo::devices {

uint8_t machine_create(datum_index object_index)
{
    return halo::devices::MachineHandle(object_index).create();
}

void machine_place(datum_index object_index, uint8_t *placement)
{
    halo::devices::MachineHandle(object_index).place(placement);
}

void device_machine_melee_attacked(uint32_t object_index)
{
    halo::devices::MachineHandle(object_index).melee_attacked();
}

uint32_t device_machine_update(uint32_t object_index)
{
    return halo::devices::MachineHandle(object_index).update();
}

void control_place(datum_index object_index, uint8_t *placement)
{
    halo::devices::ControlHandle(object_index).place(placement);
}

void device_control_activate(uint32_t object_id)
{
    halo::devices::ControlHandle(object_id).activate();
}

void device_control_touched(uint32_t object_index)
{
    halo::devices::ControlHandle(object_index).touched();
}

void light_fixture_place(datum_index object_index, uint8_t *placement)
{
    halo::devices::LightFixtureHandle(object_index).place(placement);
}

}
