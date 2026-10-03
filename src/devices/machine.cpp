#include "halo/devices/machine.hpp"

extern "C" {
extern data_array *object_data;
extern tag_instance *tag_instances;
extern void device_new(uint32_t object_index, void *placement);
extern void device_group_set_value_immediate(uint16_t group_index, float value);
extern data_array *device_groups;
extern game_time_globals *game_time;
extern game_engine_definition *current_game_engine;
extern void *team_pair_data;
extern uint8_t device_group_set_value(uint16_t group_index, float value);
extern int16_t object_find_in_sphere(uint32_t search_mask, uint32_t type_mask, void *location, real_point3d *center, float radius, datum_index *out_objects, int16_t max_output);
extern void object_set_cluster_and_parent(uint32_t object_index, bsp_leaf_reference *location);
extern void object_unlink_cluster_or_notify_parent(uint32_t object_index);
extern void device_change_power_state(float fallback_value, uint32_t object_id);
extern void device_control_activate(uint32_t object_id);
}

namespace {

static uint8_t *object_get(datum_index object_index)
{
    return *(uint8_t **)((uint8_t *)object_data->data + (object_index & 0xffff) * 0xc + 8);
}

static uint8_t *object_definition(uint8_t *object)
{
    return (uint8_t *)tag_instances[*(datum_index *)object & 0xffff].data;
}

}

namespace halo::devices {
namespace {

/**
 * object_type_definition machine (0x0069bcc0) field +0x28 (create); the object type dispatch
 * (object_type_definitions_*) calls it cdecl with the object handle.
 *
 * @address 0x44b020
 */
uint8_t MachineHandle::create()
{
    datum_index object_index = (datum_index)handle;

    uint8_t *object = object_get(object_index);
    uint8_t *definition = object_definition(object);
    uint32_t *flags = (uint32_t *)(object + 0x10);

    *flags |= 0x2000;
    if ((definition[0x292] & 4) != 0) {
        *flags |= 0x4000 | 0x8000;
    } else {
        *flags &= ~(uint32_t)(0x4000 | 0x8000);
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

    device_new(object_index, placement + 0x28);
    ((device_object *)obj)->device.type_flags |= placement[0x30] & 0xf;
}

/**
 * types/devices.h device_machine_flags (_device_machine_opened_by_melee_attack_bit, bit 3 of
 * type_flags at object+0x214), device_data (position_group at object+0x204).
 *
 * Register convention in the original: object index in ECX (in_ECX).
 *
 * @address 0x44b5d0
 */
void MachineHandle::melee_attacked()
{
    uint32_t object_index = (uint32_t)handle;

    object *obj = ((object_header *)object_data->data)[object_index & 0xffff].data;
    device_machine_data *dev = (device_machine_data *)((uint8_t *)obj + sizeof(object));

    if ((dev->device.type_flags & (1u << _device_machine_opened_by_melee_attack_bit)) != 0 &&
        object_index != 0xffffffff &&
        dev->device.position_group != -1) {
        device_group_set_value_immediate((uint16_t)dev->device.position_group, 1.0f);
    }
}

/**
 * types/devices.h device_machine_data (ticks_since_fully_open 0x218, last_elevator_position
 * 0x21c), device_data (flags 0x1f4, power_group/power 0x1f8/0x1fc, position_group/position
 * 0x204/0x208, type_flags 0x214); types/tags.h DeviceMachine (machine_type 0x290,
 * machine_flags 0x292, elevator_node.
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

    object *obj = ((object_header *)object_data->data)[object_index & 0xffff].data;
    device_machine_data *dev = (device_machine_data *)((uint8_t *)obj + sizeof(object));
    DeviceMachine *tag = (DeviceMachine *)tag_instances[obj->definition_tag & 0xffff].data;

    
    
    
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
                object *candidate = ((object_header *)object_data->data)[candidates[i] & 0xffff].data;
                int counts = 1;
                int passes_side_test = 1;

                if (((candidate->vitality_flags & _object_health_frozen_bit) != 0) ||
                    ((*(uint32_t *)((uint8_t *)tag_instances[candidate->definition_tag & 0xffff].data
                        + sizeof(Object)) & 0x4000) != 0)) { 
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
                            exempt = (*(uint32_t *)((uint8_t *)team_pair_data + 0xa4 +
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
                    device_group_set_value((uint16_t)dev->device.position_group, 1.0f);
                    
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
                device_group_set_value((uint16_t)dev->device.position_group, 0.0f);
                
            }
        } else {
            dev->ticks_since_fully_open = 0;
        }
    }

    
    
    
    if ((tag->machine_flags & 0x4) != 0) {
        if (tag->elevator_node != (uint16_t)0xffff) {
            
            
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
                        object *rider = ((object_header *)object_data->data)[riders[i] & 0xffff].data;
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

    device_new(object_index, placement + 0x28);
    if ((placement[0x30] & 1) != 0) {
        ((device_object *)obj)->device.type_flags |= 1;
    }
    if ((placement[0x30] & 0x10) != 0) {
        ((device_object *)obj)->device.type_flags |= 2;
    }
    ((control_object *)obj)->control.custom_name_index =
        (int16_t)(((ScenarioControl *)placement)->custom_control_name - 1);
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

    object *obj = ((object_header *)object_data->data)[object_id & 0xffff].data;
    DeviceControl *tag = (DeviceControl *)tag_instances[obj->definition_tag & 0xffff].data;

    if (tag->triggers_when == devicetriggerswhen_touched_by_player) {
        device_change_power_state(0.0f, object_id); 
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

    object *obj = ((object_header *)object_data->data)[object_index & 0xffff].data;

    if (obj->type == _object_type_device_control) {
        device_control_activate(object_index);
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
    int32_t i;

    device_new(object_index, placement + 0x28);
    for (i = 0; i < 3; i++) {
        ((uint32_t *)(object + 0x214))[i] = ((uint32_t *)(placement + 0x30))[i];
    }
    for (i = 0; i < 3; i++) {
        ((uint32_t *)(object + 0x220))[i] = ((uint32_t *)(placement + 0x3c))[i];
    }
}

}
}

extern "C" {

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
