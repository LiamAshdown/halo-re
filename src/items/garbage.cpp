#include "halo/items/items.hpp"

extern "C" {
extern data_array *object_data;
extern object_globals *object_globals_pointer;
extern uint32_t random_seed_global;
extern void object_delete_unparented(uint32_t object_index);
extern void object_delete_recursive(uint32_t object_index, uint8_t recurse_siblings);
uint8_t garbage_new(uint32_t object_index);
int32_t garbage_update(uint32_t object_index);
}

namespace halo::items {

/**
 * The garbage row's query_create hook (object_type_definition +0x28). Links a freshly activated
 * garbage object into the object_globals tracked list (unless it, or some later object, already
 * linked it), marks it definition-flagged and connected-to-map, and seeds its random despawn
 * countdown (garbage_data + 0x00) to a value in [300, 600) ticks. Always reports success.
 *
 * @address 0x4bc490
 */
uint8_t garbage_ref::create()
{
    uint32_t object_index = datum;
    object *obj = ((object_header *)object_data->data)[object_index & 0xffff].data;
    int16_t *despawn_countdown = (int16_t *)((uint8_t *)obj + k_item_extension_offset);

    if ((obj->flags & (_object_in_tracked_list_bit | _object_unknown_20000_bit)) == 0) {
        obj->next_tracked_object = object_globals_pointer->first_tracked_object;
        object_globals_pointer->first_tracked_object = object_index;
        obj->flags |= _object_in_tracked_list_bit;
    }
    obj->flags |= _object_definition_flag0_bit | _object_connected_to_map_bit;

    random_seed_global = random_seed_global * 0x19660d + 0x3c6ef35f;
    *despawn_countdown = (int16_t)(((random_seed_global >> 0x10) * 300) >> 0x10) + 300;

    return 1;
}

/**
 * The garbage row's "update" hook (object_type_definition +0x34). Counts garbage_data + 0x00's
 * random despawn timer down by one tick; once it reaches zero, deletes the object the same way
 * object_delete does (immediately if network_role == 0, recursively if it is 3, otherwise leaves
 * it alone). Returns whether the countdown is still running (i.e. whether the object survives
 * this tick), matching item_update/weapon_update's own int return convention for this vtable
 * column.
 *
 * @address 0x4bc510
 */
int32_t garbage_ref::update()
{
    uint32_t object_index = datum;
    object *obj = ((object_header *)object_data->data)[object_index & 0xffff].data;
    int16_t *despawn_countdown = (int16_t *)((uint8_t *)obj + k_item_extension_offset);
    int32_t still_alive;

    (*despawn_countdown)--;
    still_alive = *despawn_countdown > 0;

    if (!still_alive) {
        if (obj->network_role == 0) {
            object_delete_unparented(object_index);
        } else if (obj->network_role != 3) {
            return still_alive;
        }
        object_delete_recursive(object_index, 0);
    }
    return still_alive;
}

}

extern "C" {

uint8_t garbage_new(uint32_t object_index)
{
    return halo::items::garbage_ref(object_index).create();
}

int32_t garbage_update(uint32_t object_index)
{
    return halo::items::garbage_ref(object_index).update();
}

}
