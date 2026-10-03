#pragma once

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "devices.h"
#include "cache.h"
#include "models.h"
#include "effects.h"

namespace halo::devices {
namespace {

/**
 * View of one device object addressed by its object handle. The device object itself stays in
 * the object data array; this class only carries the handle and groups the device-type
 * operations (create, place, update, position and power changes) that the object type table
 * calls.
 */
class DeviceHandle {
public:
    explicit DeviceHandle(datum_index value) : handle(value) {}
    void construct(device_placement_data *placement);
    uint8_t create();
    void destroy();
    void blend_animations(real_orientation *orientations);
    int can_change_position();
    void change_power_state(float fallback_value);
    void compute_function_values();
    uint8_t frontfacing(real_vector3d *forward);
    void play_state_change_effect(TagID tag_id);
    uint8_t update_change_values();

    datum_index handle;
};

/**
 * View of one device group (a shared scalar that devices read as their power or position)
 * addressed by its group index in the device group data array.
 */
class DeviceGroupHandle {
public:
    explicit DeviceGroupHandle(uint16_t value) : group_handle(value) {}
    uint8_t set_value(float value);
    void set_value_immediate(float value);

    uint16_t group_handle;
};

/**
 * Lifetime of the device group data array: allocation, initialisation, disposal and the data-
 * pool callback that clears the disposing flag.
 */
class DeviceGroupPool {
public:
    static void allocate();
    static void clear_disposing_flag();
    static void dispose();
    static void initialize();
};

}
}
