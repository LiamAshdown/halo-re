#pragma once

#include <cstddef>
#include <cstdint>
#include "halo/core/flags.hpp"
#include "halo/core/datum.hpp"

namespace halo::camera {

/**
 * collision_test_movement_segment flags the observer uses for its pull-back and obstruction probes.
 */
enum class probe_flags : uint32_t {
    none = 0,
    front_faces = 0x0001,
    structure_bsp = 0x0020,
    water_surface = 0x0040,
    nearby_objects = 0x0080,
    object_type_filter = 0x4000,
};

}

namespace halo {
template <> struct enable_bit_flags<camera::probe_flags> : std::true_type {};
}

namespace halo::camera {

constexpr probe_flags k_probe_flags_normal = probe_flags::front_faces | probe_flags::structure_bsp | probe_flags::nearby_objects |
                                             probe_flags::water_surface | probe_flags::object_type_filter;
constexpr probe_flags k_probe_flags_alternate = probe_flags::front_faces | probe_flags::structure_bsp | probe_flags::nearby_objects |
                                                probe_flags::object_type_filter;

/** Object type mask that accepts every object type. */
inline constexpr uint32_t k_all_object_types = 0xffffffffu;

}



namespace halo::camera {

static_assert(offsetof(Unit, camera_marker_name) == 0x1a8);
static_assert(sizeof(camera_constants) == 4);
static_assert(sizeof(director_camera_mode) == 4);
static_assert(sizeof(camera_script_mode) == 4);
static_assert(sizeof(director_camera_type) == 4);
static_assert(sizeof(director_seat_camera_state) == 4);
static_assert(sizeof(flying_camera_mode) == 4);
static_assert(sizeof(observer_parameter) == 4);
static_assert(sizeof(observer_command_flags) == 4);
static_assert(sizeof(observer_interpolation_flags) == 4);
static_assert(sizeof(camera_input_key_bits) == 4);
static_assert(sizeof(camera_script_globals) == 64);
static_assert(sizeof(camera_input_axis_definition) == 28);
static_assert(sizeof(camera_input_axis_state) == 12);
static_assert(sizeof(camera_input) == 36);
static_assert(sizeof(observer_parameters) == 56);
static_assert(sizeof(observer_command) == 104);
static_assert(sizeof(observer_parameter_derivatives) == 44);
static_assert(sizeof(observer_camera) == 60);
static_assert(sizeof(observer) == 668);
static_assert(sizeof(first_person_camera_data) == 4);
static_assert(sizeof(third_person_camera_data) == 28);
static_assert(sizeof(dead_camera_data) == 48);
static_assert(sizeof(editor_camera_data) == 28);
static_assert(sizeof(orbiting_camera_data) == 28);
static_assert(sizeof(director_camera_data) == 64);
static_assert(sizeof(director) == 248);
static_assert(sizeof(director_globals) == 8);
static_assert(sizeof(flying_camera_home) == 20);
static_assert(sizeof(unit_camera_properties) == 88);

}
