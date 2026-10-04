#pragma once

#include <cstdint>
#include "halo/core/flags.hpp"

namespace halo::devices {

/**
 * ScenarioDeviceFlags: the per-placement flags word shared by machines, controls and light fixtures.
 */
enum class scenario_device_flags : uint32_t {
    none = 0,
    initially_open = 0x01,
    initially_off = 0x02,
    can_change_only_once = 0x04,
    position_reversed = 0x08,
    not_usable_from_any_side = 0x10,
};

/**
 * Bits of the object flags dword that machine_create sets on an elevator or door machine.
 */
enum class machine_object_flags : uint32_t {
    none = 0,
    unknown_4000 = 0x4000,
    unknown_8000 = 0x8000,
};

/**
 * ScenarioControl.control_flags bits that control_place copies into the runtime device flags.
 */
enum class scenario_control_flags : uint32_t {
    none = 0,
    usable_from_both_sides = 0x01,
    unknown_10 = 0x10,
};

/**
 * Runtime device_data.type_flags bits of a control.
 */
enum class control_type_flags : uint32_t {
    none = 0,
    usable_from_both_sides = 0x1,
    unknown_2 = 0x2,
};

/**
 * Runtime color, intensity and cone angles a light fixture carries behind its device data
 * (copied from the placement by light_fixture_place).
 */
struct light_fixture_placement_copy {
    ColorRGB color;
    float intensity;
    float falloff_angle;
    float cutoff_angle;
};
static_assert(sizeof(light_fixture_placement_copy) == 24);

/**
 * Low nibble of ScenarioMachine.machine_flags, copied into the runtime type_flags of a machine.
 */
inline constexpr uint32_t k_machine_placement_flags_mask = 0xf;

/**
 * Offset of the per-team friendly fire bitmatrix inside the game globals block (team_pair_data).
 */
inline constexpr uint32_t k_team_pair_matrix_offset = 0xa4;

/**
 * Maximum number of device group slots and the element size of the device group table.
 */
inline constexpr int16_t k_device_group_maximum_count = 0x400;
inline constexpr int16_t k_device_group_element_size = 8;

}

namespace halo {
template <> struct enable_bit_flags<devices::scenario_device_flags> : std::true_type {};
template <> struct enable_bit_flags<devices::machine_object_flags> : std::true_type {};
template <> struct enable_bit_flags<devices::scenario_control_flags> : std::true_type {};
template <> struct enable_bit_flags<devices::control_type_flags> : std::true_type {};
}


namespace halo::devices {

static_assert(sizeof(device_constants) == 4);
static_assert(sizeof(device_group_flags) == 4);
static_assert(sizeof(device_flags) == 4);
static_assert(sizeof(device_machine_flags) == 4);
static_assert(sizeof(device_control_flags) == 4);
static_assert(sizeof(device_group) == 8);
static_assert(sizeof(device_placement_data) == 8);
static_assert(sizeof(device_data) == 36);
static_assert(sizeof(device_machine_data) == 52);
static_assert(sizeof(device_control_data) == 40);
static_assert(sizeof(device_light_fixture_data) == 56);
static_assert(sizeof(device_object) == 536);
static_assert(sizeof(control_object) == 540);

}
