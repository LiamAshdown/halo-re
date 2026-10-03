#include "halo/devices/device.hpp"

extern "C" {
extern data_array *object_data;
extern data_array *device_groups;
extern datum_index datum_new(data_array *array);
extern tag_instance *tag_instances;
extern void datum_delete(data_array *array, datum_index handle);
extern void animation_overlay_interpolated_frame_orientations(ModelAnimationsAnimation *animation, float frame, real_orientation *out_orientations);
extern void animation_overlay_frame_orientations(ModelAnimationsAnimation *animation, int16_t frame, real_orientation *out_orientations);
extern uint8_t device_group_set_value(uint16_t group_index, float value);
extern void device_play_state_change_effect(uint32_t object_index, TagID tag_id);
extern int32_t object_get_node_local_transform(uint32_t object_index, const char *marker_name, object_marker *marker, uint32_t flags);
extern object *object_try_and_get(datum_index object_index, uint32_t type_mask);
extern void *global_forward3d_pointer;
extern void *global_zero_vector3d_pointer;
extern datum_index effect_new_on_object(datum_index creator_object_index, datum_index definition_index, datum_index object_index, int16_t first_person_weapon_override, real a_scale, real b_scale, const ColorRGB *color, const effect_tint_source *tint_source);
extern datum_index sound_start_at_object_marker(datum_index object_index, Point3D *position, Vector3D *forward, datum_index definition_index, int16_t node_index, float scale, uint32_t first_person_hint);
extern uint8_t real_seek_toward_clamped(int wrap, real *velocity, real *value, real target, real accel, real max_speed, real range_min, real range_max);
extern object *object_iterator_next(object_iterator *iterator);
extern data_array *game_state_new(char *name, int16_t maximum_count, int16_t element_size);
extern void data_delete_all(data_array *array);
extern void device_groups_initialize(void);
extern Scenario *global_scenario;
void device_new(uint32_t object_index, device_placement_data *placement);
uint8_t device_create(datum_index object_index);
void device_delete(datum_index object_index);
void device_blend_animations(datum_index object_index, real_orientation *orientations);
int device_can_change_position(uint32_t object_index);
void device_change_power_state(float fallback_value, uint32_t object_id);
void device_compute_function_values(uint32_t object_index);
uint8_t device_frontfacing(uint32_t device_index, real_vector3d *forward);
uint8_t device_update_change_values(uint32_t object_index);
void device_group_set_value_immediate(uint16_t group_index, float value);
void device_groups_allocate(void);
void device_groups_clear_disposing_flag(void);
void device_groups_dispose(void);
}

namespace {

static uint8_t *object_get(datum_index object_index)
{
    return *(uint8_t **)((uint8_t *)object_data->data + (object_index & 0xffff) * 0xc + 8);
}

}

namespace halo::devices {
namespace {

/**
 * Original function device_new; the author notes are in
 * docs/original/devices/device_new.c.txt.
 *
 * Register convention in the original: object index in EAX (in_EAX), placement pointer in EDI
 * (unaff_EDI).
 *
 * @address 0x44bf90
 */
void DeviceHandle::construct(device_placement_data *placement)
{
    uint32_t object_index = (uint32_t)handle;

    object *obj = ((object_header *)object_data->data)[object_index & 0xffff].data;
    device_data *dev = (device_data *)((uint8_t *)obj + sizeof(object));
    int16_t power_group = placement->power_group;
    uint16_t position_group;

    if (power_group == -1) {
        datum_index new_group = datum_new(device_groups);
        power_group = (int16_t)new_group;
        if (power_group != -1) {
            device_group *group = &((device_group *)device_groups->data)[(uint16_t)new_group];
            group->flags = (1u << _device_group_object_created_bit);
            group->value = (placement->flags & 0x02) != 0 ? 0.0f : 1.0f; 
        }
    }
    dev->power_group = power_group;

    position_group = placement->position_group;
    if (position_group == 0xffff) {
        datum_index new_group = datum_new(device_groups);
        position_group = (uint16_t)new_group;
        if (position_group != 0xffff) {
            device_group *group = &((device_group *)device_groups->data)[position_group];
            group->flags = (uint16_t)(((placement->flags & 0x04) | 0x10) >> 2); 
                
                
            group->value = (placement->flags & 0x01) != 0 ? 1.0f : 0.0f; 
        }
    }
    dev->position_group = (int16_t)position_group;

    dev->power = ((device_group *)device_groups->data)[(uint16_t)dev->power_group].value;
    dev->position = ((device_group *)device_groups->data)[position_group].value;

    if ((placement->flags & 0x08) != 0) { 
        dev->flags |= (1u << _device_position_reversed_bit);
    }
    if ((placement->flags & 0x10) != 0) { 
        dev->flags |= (1u << _device_not_usable_from_any_side_bit);
    }
}

/**
 * Original function device_create; the author notes are in
 * docs/original/devices/device_create.c.txt.
 *
 * @address 0x44b670
 */
uint8_t DeviceHandle::create()
{
    datum_index object_index = (datum_index)handle;

    uint8_t *obj = object_get(object_index);

    ((device_object *)obj)->device.position_group = -1;
    ((device_object *)obj)->device.power_group = -1;
    ((device_object *)obj)->base.flags |= 0x40000;
    return 1;
}

/**
 * Original function device_delete; the author notes are in
 * docs/original/devices/device_delete.c.txt.
 *
 * @address 0x44b6b0
 */
void DeviceHandle::destroy()
{
    datum_index object_index = (datum_index)handle;

    uint8_t *obj = object_get(object_index);
    int16_t group = ((device_object *)obj)->device.power_group;

    if (group != -1 && (((uint8_t *)device_groups->data)[(uint16_t)group * 8 + 2] & 4) != 0) {
        datum_delete(device_groups, (datum_index)(int32_t)group);
    }
    group = ((device_object *)obj)->device.position_group;
    if (group != -1 && (((uint8_t *)device_groups->data)[(uint16_t)group * 8 + 2] & 4) != 0) {
        datum_delete(device_groups, (datum_index)(int32_t)group);
    }
}

/**
 * Original function device_blend_animations; the author notes are in
 * docs/original/devices/device_blend_animations.c.txt.
 *
 * @address 0x44bc20
 */
void DeviceHandle::blend_animations(real_orientation *orientations)
{
    datum_index object_index = (datum_index)handle;

    device_object *obj = *(device_object **)((uint8_t *)object_data->data + (object_index & 0xffff) * 0xc + 8);
    Device *device_tag = (Device *)tag_instances[obj->base.definition_tag & 0xffff].data;
    ModelAnimations *graph =
        (ModelAnimations *)tag_instances[*(datum_index *)&device_tag->base.animation_graph.tag_id & 0xffff].data;
    ModelAnimationsDeviceAnimations *entry;
    uint8_t *animations;
    int32_t count;
    int16_t *indices;

    if (graph->devices.count == 0) {
        return;
    }
    entry = (ModelAnimationsDeviceAnimations *)graph->devices.pointer;
    if (entry == 0) {
        return;
    }
    animations = (uint8_t *)graph->animations.pointer;
    count = (int32_t)entry->animations.count;
    indices = (int16_t *)entry->animations.pointer;

    if (count > 0 && indices[0] != -1) {
        ModelAnimationsAnimation *animation = (ModelAnimationsAnimation *)(animations + indices[0] * 0xb4);
        double position = (obj->device.flags & 1) ? 1.0 - obj->device.position : obj->device.position;
        uint32_t tag_flags = device_tag->device_flags;  
        int32_t frames = (int16_t)animation->frame_count;
        float frame;

        if ((tag_flags & 1) == 0) {
            frames = frames - 1;
        }
        frame = (float)((double)frames * position);
        if (tag_flags & 2) {
            animation_overlay_frame_orientations(animation, (int16_t)(int32_t)frame, orientations); 
        } else {
            animation_overlay_interpolated_frame_orientations(animation, frame, orientations);
        }
    }
    if (count > 1 && indices[1] != -1) {
        ModelAnimationsAnimation *animation = (ModelAnimationsAnimation *)(animations + indices[1] * 0xb4);
        int32_t frames = (int16_t)animation->frame_count;

        animation_overlay_interpolated_frame_orientations(animation,
            (float)((double)frames * obj->device.power), orientations);
    }
}

/**
 * Original function device_can_change_position; the author notes are in
 * docs/original/devices/device_can_change_position.c.txt.
 *
 * Register convention in the original: object index in EAX (in_EAX).
 *
 * @address 0x44c0c0
 */
int DeviceHandle::can_change_position()
{
    uint32_t object_index = (uint32_t)handle;

    object *obj = ((object_header *)object_data->data)[object_index & 0xffff].data;
    device_data *dev = (device_data *)((uint8_t *)obj + sizeof(object));
    int can_change = 0;

    if (dev->position_group != -1) {
        uint16_t flags = ((device_group *)device_groups->data)[(uint16_t)dev->position_group].flags;
        can_change = 1;

        if ((flags & (1u << _device_group_can_change_only_once_bit)) != 0 &&
            (flags & (1u << _device_group_changed_bit)) != 0) {
            can_change = 0;
        }
        if ((dev->flags & (1u << _device_not_usable_from_any_side_bit)) != 0) {
            can_change = 0;
        }
        if (((device_group *)device_groups->data)[(uint16_t)dev->power_group].value != 1.0f) {
            can_change = 0;
        }
    }

    return can_change;
}

/**
 * Computes the target value for a device_control's own (position) group from its
 * DeviceControl.type -- an auto-threshold toggle, a forced on/off, or a custom call_value --
 * applies it through device_group_set_value, and plays whichever state-change effect/sound
 * corresponds to the outcome (rejected, settled off, or settled on).
 *
 * Register convention in the original: a fallback float value in ECX (reachable only for a
 * malformed.
 *
 * @address 0x44ae30
 */
void DeviceHandle::change_power_state(float fallback_value)
{
    uint32_t object_id = (uint32_t)handle;

    object *obj = ((object_header *)object_data->data)[object_id & 0xffff].data;
    device_data *dev = (device_data *)((uint8_t *)obj + sizeof(object));
    DeviceControl *tag = (DeviceControl *)tag_instances[obj->definition_tag & 0xffff].data;
    int16_t group_index = dev->position_group; 
    float target;
    uint8_t changed;

    if (group_index == (int16_t)0xffff) {
        return;
    }

    target = fallback_value; 
    switch (tag->type) {
    case devicetype_toggle_switch:
        
        
        if (((device_group *)device_groups->data)[(uint16_t)group_index].value <= 0.5f) {
            target = 1.0f;
            break;
        }
         
    case devicetype_off_button:
        target = 0.0f;
        break;
    case devicetype_on_button:
        target = 1.0f;
        break;
    case devicetype_call_button:
        target = tag->call_value;
        break;
    }

    changed = device_group_set_value((uint16_t)group_index, target);
    if (!changed) {
        device_play_state_change_effect(object_id, tag->deny.tag_id); 
        return;
    }
    if (target <= 0.5f) {
        device_play_state_change_effect(object_id, tag->off.tag_id); 
        return;
    }
    device_play_state_change_effect(object_id, tag->on.tag_id); 
}

/**
 * Original function device_compute_function_values; the author notes are in
 * docs/original/devices/device_compute_function_values.c.txt.
 *
 * Register convention in the original: object index is already a plain, genuinely-stack
 * parameter; Ghidra's own `FUN_0044ba10(short *param_1)` reuses that one parameter register as
 * a `DeviceIn_t *` partway through the function (first as the object index, then repointed at
 * tag->device_a_in), which.
 *
 * @address 0x44ba10
 */
void DeviceHandle::compute_function_values()
{
    uint32_t object_index = (uint32_t)handle;

    object *obj = ((object_header *)object_data->data)[object_index & 0xffff].data;
    device_data *dev = (device_data *)((uint8_t *)obj + sizeof(object));
    Device *tag = (Device *)tag_instances[obj->definition_tag & 0xffff].data;
    DeviceIn_t *selector = &tag->device_a_in;
    float *out = obj->function_in_values;
    int i;

    for (i = 0; i < k_maximum_device_functions; i++, selector++, out++) {
        float value = 0.0f;

        switch (*selector) {
        case devicein_power:
            value = dev->power;
            break;
        case devicein_change_in_power:
            if (dev->power_change != 0.0f) {
                value = (dev->power_change < 0.0f ? -dev->power_change : dev->power_change) /
                    tag->inverse_power_transition_time;
            }
            break;
        case devicein_position:
            value = dev->position;
            break;
        case devicein_change_in_position:
            if (dev->position_change != 0.0f) {
                value = (dev->position_change < 0.0f ? -dev->position_change : dev->position_change) /
                    tag->inverse_position_transition_time;
            }
            break;
        case devicein_locked:
            value = (dev->power == 0.0f) ? 1.0f : 0.0f;
            if (obj->type == _object_type_device_machine && dev->position_group != -1) {
                device_group *group = &((device_group *)device_groups->data)[(uint16_t)dev->position_group];

                if ((dev->type_flags & 0x3) != 0) { 
                    value = 1.0f;
                }
                if ((group->flags & (1u << _device_group_can_change_only_once_bit)) != 0 &&
                    (group->flags & (1u << _device_group_changed_bit)) != 0) {
                    value = 1.0f;
                }
                if (dev->position == 1.0f ||
                    (dev->type_flags & (1u << _device_machine_never_appears_locked_bit)) != 0) {
                    value = 0.0f;
                }
            }
            break;
        case devicein_delay:
            if (!(tag->delay_time_ticks <= 0.0f) && (float)dev->delay_ticks != tag->delay_time_ticks) { 
                value = (float)dev->delay_ticks / tag->delay_time_ticks;
            }
            break;
        case devicein_none:
            
            
            
            continue;
        default:
            
            
            
            
            
            value = 0.0f;
            break;
        }

        *out = value;
    }
}

/**
 * Original function device_frontfacing; the author notes are in
 * docs/original/devices/device_frontfacing.c.txt.
 *
 * Register convention in the original: device object index in ESI (unaff_ESI), a caller-owned
 * forward-vector.
 *
 * @address 0x44c130
 */
uint8_t DeviceHandle::frontfacing(real_vector3d *forward)
{
    uint32_t device_index = (uint32_t)handle;

    object *control = object_try_and_get(device_index, _object_mask_device_control);

    if (control != (object *)0) {
        device_control_data *dev = (device_control_data *)((uint8_t *)control + sizeof(object));

        if ((dev->device.type_flags & (1u << _device_control_usable_from_both_sides_bit)) == 0) {
            object_marker marker;
            if (object_get_node_local_transform(device_index, "front", &marker, 1) == 1) {
                real_vector3d *marker_forward = &marker.node_transform.forward;
                if (0.0f < marker_forward->i * forward->i + marker_forward->j * forward->j +
                    marker_forward->k * forward->k) {
                    return 0;
                }
            }
        }
    }
    return 1;
}

/**
 * Original function device_play_state_change_effect; the author notes are in
 * docs/original/devices/device_play_state_change_effect.c.txt.
 *
 * Register convention in the original: tag id in ECX (in_ECX), packed as a TagID {index;id}
 * the way every.
 *
 * @address 0x44c1a0
 */
void DeviceHandle::play_state_change_effect(TagID tag_id)
{
    uint32_t object_index = (uint32_t)handle;

    
    if (tag_id.index != 0xffff || tag_id.id != 0xffff) {
        object *obj = ((object_header *)object_data->data)[object_index & 0xffff].data;
        
        uint32_t group_tag = tag_instances[(int16_t)tag_id.index].group_tag;

        if (group_tag == k_device_state_change_tag_effect) {
            device_data *dev = (device_data *)((uint8_t *)obj + sizeof(object));
            
            
            effect_new_on_object(object_index, *(datum_index *)&tag_id, object_index, -1, dev->position, dev->power,
                (const ColorRGB *)0, (const effect_tint_source *)0);
        } else if (group_tag == k_device_state_change_tag_sound) {
            
            sound_start_at_object_marker(object_index, (Point3D *)global_zero_vector3d_pointer, (Vector3D *)global_forward3d_pointer,
                *(datum_index *)&tag_id, -1, 1.0f, 0);
        }
    }
}

/**
 * Original function device_update_change_values; the author notes are in
 * docs/original/devices/device_update_change_values.c.txt.
 *
 * Register convention in the original: object index is already a plain, genuinely-stack
 * parameter (`device_update_change_values(uint param_1)`); no unresolved registers appear.
 *
 * @address 0x44b720
 */
uint8_t DeviceHandle::update_change_values()
{
    uint32_t object_index = (uint32_t)handle;

    object *obj = ((object_header *)object_data->data)[object_index & 0xffff].data;
    device_data *dev = (device_data *)((uint8_t *)obj + sizeof(object));
    Device *tag = (Device *)tag_instances[obj->definition_tag & 0xffff].data;
    uint8_t still_settling = 0;

    if (dev->power_group != -1) {
        device_group *group = &((device_group *)device_groups->data)[(uint16_t)dev->power_group];
        if (group->value != dev->power || dev->power_change != 0.0f) {
            float old_power = dev->power;
            uint8_t settled = real_seek_toward_clamped(0, &dev->power_change, &dev->power,
                group->value, tag->inverse_power_acceleration_time,
                tag->inverse_power_transition_time, 0.0f, 1.0f);
            still_settling = !settled;
            if (old_power != dev->power) {
                dev->flags |= (1u << _device_position_changed_bit);
            }
        }
    }

    if (dev->position_group != -1) {
        device_group *group = &((device_group *)device_groups->data)[(uint16_t)dev->position_group];

        if (group->value == dev->position && dev->position_change == 0.0f) {
            dev->delay_ticks = 0;
        } else {
            float accel = tag->inverse_position_acceleration_time * dev->power +
                (1.0f - dev->power) * tag->inverse_depowered_position_acceleration_time;
            float max_speed = tag->inverse_position_transition_time * dev->power +
                (1.0f - dev->power) * tag->inverse_depowered_position_transition_time;
            float old_position_change = dev->position_change;

            if (tag->delay_time_ticks <= (float)dev->delay_ticks ||
                dev->position != 0.0f ||
                group->value < dev->position) {
                float old_position = dev->position;
                uint8_t settled;

                if (max_speed < (dev->position_change < 0.0f ? -dev->position_change : dev->position_change)) {
                    dev->position_change = (old_position_change <= 0.0f) ? -max_speed : max_speed;
                }

                settled = real_seek_toward_clamped(
                    (tag->device_flags & 0x1) != 0, 
                    &dev->position_change, &dev->position, group->value, accel, max_speed, 0.0f, 1.0f);

                if (settled == 0) {
                    still_settling = 1;
                    if (dev->position_change != 0.0f &&
                        old_position_change * dev->position_change <= 0.0f) {
                        device_play_state_change_effect(object_index,
                            (dev->position_change > old_position_change)
                                ? tag->open.tag_id
                                : tag->close.tag_id);
                    }
                } else if (old_position_change <= 0.0f) {
                    device_play_state_change_effect(object_index, tag->closed.tag_id);
                } else {
                    device_play_state_change_effect(object_index, tag->opened.tag_id);
                }

                if (old_position != dev->position) {
                    dev->flags |= (1u << _device_position_changed_bit);
                }
                return still_settling;
            }

            dev->delay_ticks++;
            if (dev->delay_ticks == 1) {
                device_play_state_change_effect(object_index, tag->delay_effect.tag_id);
                return still_settling;
            }
        }
    }

    return still_settling;
}

/**
 * Original function device_group_set_value; the author notes are in
 * docs/original/devices/device_group_set_value.c.txt.
 *
 * Register convention in the original: group index in ESI (unaff_SI), value as the sole
 * recognized stack.
 *
 * @address 0x44bd70
 */
uint8_t DeviceGroupHandle::set_value(float value)
{
    uint16_t group_index = (uint16_t)group_handle;

    device_group *group;
    uint16_t flags;
    object_iterator iterator;
    object *obj;

    if (value < 0.0f) {
        value = 0.0f;
    } else if (1.0f < value) {
        value = 1.0f;
    }

    if (group_index == 0xffff) {
        return 0;
    }

    group = &((device_group *)device_groups->data)[group_index];
    if (group->value == value) {
        return 0;
    }

    flags = group->flags;
    if ((flags & (1u << _device_group_can_change_only_once_bit)) != 0 &&
        (flags & (1u << _device_group_changed_bit)) != 0) {
        return 0; 
    }

    group->flags = flags | (1u << _device_group_changed_bit);
    group->value = value;

    iterator.type_mask = _object_mask_device;
    iterator.flags_mask = 0;
    iterator.index = 0;
    iterator.handle = k_datum_index_none;

    obj = object_iterator_next(&iterator);
    while (obj != (object *)0) {
        device_data *candidate_dev = (device_data *)((uint8_t *)obj + sizeof(object));

        if (candidate_dev->power_group == (int16_t)group_index) { 
            Device *tag = (Device *)tag_instances[obj->definition_tag & 0xffff].data;
            
            
            
            
            
            device_play_state_change_effect(iterator.handle,
                (value != 0.0f) ? tag->repowered.tag_id : tag->depowered.tag_id);
        }
        obj = object_iterator_next(&iterator);
    }

    return 1;
}

/**
 * Original function device_group_set_value_immediate; the author notes are in
 * docs/original/devices/device_group_set_value_immediate.c.txt.
 *
 * Register convention in the original: group index in ESI (unaff_SI), value as the sole
 * recognized stack.
 *
 * @address 0x44bea0
 */
void DeviceGroupHandle::set_value_immediate(float value)
{
    uint16_t group_index = (uint16_t)group_handle;

    device_group *group;
    object_iterator iterator;
    object *obj;

    if (value < 0.0f) {
        value = 0.0f;
    } else if (1.0f < value) {
        value = 1.0f;
    }

    group = &((device_group *)device_groups->data)[group_index];
    group->value = value;

    iterator.type_mask = _object_mask_device;
    iterator.flags_mask = 0;
    iterator.index = 0;
    iterator.handle = k_datum_index_none;

    obj = object_iterator_next(&iterator);
    while (obj != (object *)0) {
        device_data *dev = (device_data *)((uint8_t *)obj + sizeof(object));

        if (dev->power_group == (int16_t)group_index) {
            dev->flags |= (1u << _device_position_changed_bit);
            dev->power = value;
            dev->power_change = 0.0f;
        }
        if (dev->position_group == (int16_t)group_index) {
            dev->flags |= (1u << _device_position_changed_bit);
            dev->position = value;
            dev->position_change = 0.0f;
        }
        obj = object_iterator_next(&iterator);
    }
}

/**
 * Original function device_groups_allocate; the author notes are in
 * docs/original/devices/device_groups_allocate.c.txt.
 *
 * @address 0x44b620
 */
void DeviceGroupPool::allocate()
{
    device_groups = game_state_new((char *)"device groups", 0x400, 0x8);
}

/**
 * Original function device_groups_clear_disposing_flag; the author notes are in
 * docs/original/devices/device_groups_clear_disposing_flag.c.txt.
 *
 * @address 0x44b660
 */
void DeviceGroupPool::clear_disposing_flag()
{
    device_groups->valid = 0;
}

/**
 * Original function device_groups_dispose; the author notes are in
 * docs/original/devices/device_groups_dispose.c.txt.
 *
 * @address 0x44b640
 */
void DeviceGroupPool::dispose()
{
    device_groups->valid = 1;
    data_delete_all(device_groups);
    device_groups_initialize();
}

/**
 * Original function device_groups_initialize; the author notes are in
 * docs/original/devices/device_groups_initialize.c.txt.
 *
 * Register convention in the original: none; takes no parameters (Ghidra's own
 * `device_groups_initialize(void)`).
 *
 * @address 0x44c220
 */
void DeviceGroupPool::initialize()
{
    Scenario *scenario = global_scenario;
    ScenarioDeviceGroup *scenario_groups = (ScenarioDeviceGroup *)scenario->device_groups.pointer;
    int32_t i;

    for (i = 0; i < (int32_t)scenario->device_groups.count; i++) {
        ScenarioDeviceGroup *scenario_group = &scenario_groups[i];
        datum_index new_group = datum_new(device_groups);

        if ((uint16_t)new_group != 0xffff) {
            device_group *group = &((device_group *)device_groups->data)[(uint16_t)new_group];
            group->flags = (scenario_group->flags & 0x1) != 0
                ? (1u << _device_group_can_change_only_once_bit) : 0;
            group->value = scenario_group->initial_value;
        }
    }
}

}
}

extern "C" {

void device_new(uint32_t object_index, device_placement_data *placement)
{
    halo::devices::DeviceHandle(object_index).construct(placement);
}

uint8_t device_create(datum_index object_index)
{
    return halo::devices::DeviceHandle(object_index).create();
}

void device_delete(datum_index object_index)
{
    halo::devices::DeviceHandle(object_index).destroy();
}

void device_blend_animations(datum_index object_index, real_orientation *orientations)
{
    halo::devices::DeviceHandle(object_index).blend_animations(orientations);
}

int device_can_change_position(uint32_t object_index)
{
    return halo::devices::DeviceHandle(object_index).can_change_position();
}

void device_change_power_state(float fallback_value, uint32_t object_id)
{
    halo::devices::DeviceHandle(object_id).change_power_state(fallback_value);
}

void device_compute_function_values(uint32_t object_index)
{
    halo::devices::DeviceHandle(object_index).compute_function_values();
}

uint8_t device_frontfacing(uint32_t device_index, real_vector3d *forward)
{
    return halo::devices::DeviceHandle(device_index).frontfacing(forward);
}

void device_play_state_change_effect(uint32_t object_index, TagID tag_id)
{
    halo::devices::DeviceHandle(object_index).play_state_change_effect(tag_id);
}

uint8_t device_update_change_values(uint32_t object_index)
{
    return halo::devices::DeviceHandle(object_index).update_change_values();
}

uint8_t device_group_set_value(uint16_t group_index, float value)
{
    return halo::devices::DeviceGroupHandle(group_index).set_value(value);
}

void device_group_set_value_immediate(uint16_t group_index, float value)
{
    halo::devices::DeviceGroupHandle(group_index).set_value_immediate(value);
}

void device_groups_allocate(void)
{
    halo::devices::DeviceGroupPool::allocate();
}

void device_groups_clear_disposing_flag(void)
{
    halo::devices::DeviceGroupPool::clear_disposing_flag();
}

void device_groups_dispose(void)
{
    halo::devices::DeviceGroupPool::dispose();
}

void device_groups_initialize(void)
{
    halo::devices::DeviceGroupPool::initialize();
}

}
