#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "units.h"

typedef char check_unit_control_data[(sizeof(unit_control_data) == 0x40) ? 1 : -1];
typedef char check_unit_speech[(sizeof(unit_speech) == 0x30) ? 1 : -1];
typedef char check_unit_recent_damage[(sizeof(unit_recent_damage) == 0x10) ? 1 : -1];
typedef char check_unit_animation_overlay[(sizeof(unit_animation_overlay) == 0x04) ? 1 : -1];
typedef char check_unit_data[(sizeof(unit_data) == 0x2d8) ? 1 : -1];
typedef char check_biped_data[(sizeof(biped_data) == 0x84) ? 1 : -1];
typedef char check_vehicle_data[(sizeof(vehicle_data) == 0xf4) ? 1 : -1];
typedef char check_biped_movement_solver_data[(sizeof(biped_movement_solver_data) == 0xc8) ? 1 : -1];
// the field offsets the two integrators pin, checked individually so a reordering fails here
typedef char check_solver_movement_delta[(__builtin_offsetof(biped_movement_solver_data, movement_delta) == 0x3c) ? 1 : -1];
typedef char check_solver_ground_normal[(__builtin_offsetof(biped_movement_solver_data, ground_normal) == 0x80) ? 1 : -1];
typedef char check_solver_result_flags[(__builtin_offsetof(biped_movement_solver_data, result_flags) == 0xa0) ? 1 : -1];
typedef char check_solver_result_position[(__builtin_offsetof(biped_movement_solver_data, result_position) == 0xac) ? 1 : -1];
typedef char check_solver_result_impact[(__builtin_offsetof(biped_movement_solver_data, result_impact_speed) == 0xc4) ? 1 : -1];
typedef char check_globals_player_information[(sizeof(GlobalsPlayerInformation) == 0xf4) ? 1 : -1];
typedef char check_player_info_stun[(__builtin_offsetof(GlobalsPlayerInformation, stun_movement_penalty) == 0x80) ? 1 : -1];
typedef char check_player_info_walking[(__builtin_offsetof(GlobalsPlayerInformation, walking_speed) == 0x2c) ? 1 : -1];
typedef char check_biped_collision_height[(__builtin_offsetof(Biped, standing_collision_height) == 0x424) ? 1 : -1];
typedef char check_biped_collision_radius[(__builtin_offsetof(Biped, collision_radius) == 0x42c) ? 1 : -1];
typedef char check_biped_camera_height[(__builtin_offsetof(Biped, standing_camera_height) == 0x400) ? 1 : -1];
typedef char check_object[(sizeof(object) == 0x1f4) ? 1 : -1];

int units_smoke(void) { return (int)(sizeof(unit_data) + sizeof(biped_data) + sizeof(vehicle_data)); }
