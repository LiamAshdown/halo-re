#pragma once


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
