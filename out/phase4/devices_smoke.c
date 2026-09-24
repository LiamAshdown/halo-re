#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "devices.h"

// struct sizes, each one fixed by an object_type_definition row at 0x0069bfdc
typedef char check_device_data[(sizeof(device_data) == 0x24) ? 1 : -1];
typedef char check_device_machine_data[(sizeof(device_machine_data) == 0x34) ? 1 : -1];
typedef char check_device_control_data[(sizeof(device_control_data) == 0x28) ? 1 : -1];
typedef char check_device_light_fixture_data[(sizeof(device_light_fixture_data) == 0x38) ? 1 : -1];
typedef char check_object_plus_machine[(0x1f4 + sizeof(device_machine_data) == 0x228) ? 1 : -1];
typedef char check_object_plus_control[(0x1f4 + sizeof(device_control_data) == 0x21c) ? 1 : -1];
typedef char check_object_plus_lifi[(0x1f4 + sizeof(device_light_fixture_data) == 0x22c) ? 1 : -1];

// sub-records
typedef char check_device_group[(sizeof(device_group) == 0x08) ? 1 : -1];
typedef char check_device_placement_data[(sizeof(device_placement_data) == 0x08) ? 1 : -1];

// device_group, as offsets inside one 8-byte element of the table at 0x0087abf0
typedef char chk_dg_flags[(__builtin_offsetof(device_group, flags) == 0x02) ? 1 : -1];
typedef char chk_dg_value[(__builtin_offsetof(device_group, value) == 0x04) ? 1 : -1];

// device_placement_data, as offsets inside ScenarioMachine / ScenarioControl /
// ScenarioLightFixture, all of which carry the slice at 0x28
typedef char chk_dp_power[(0x28 + __builtin_offsetof(device_placement_data, power_group)
                           == __builtin_offsetof(ScenarioMachine, power_group)) ? 1 : -1];
typedef char chk_dp_position[(0x28 + __builtin_offsetof(device_placement_data, position_group)
                              == __builtin_offsetof(ScenarioMachine, position_group)) ? 1 : -1];
typedef char chk_dp_flags[(0x28 + __builtin_offsetof(device_placement_data, flags)
                           == __builtin_offsetof(ScenarioMachine, device_flags)) ? 1 : -1];
typedef char chk_dp_control[(0x28 + __builtin_offsetof(device_placement_data, flags)
                             == __builtin_offsetof(ScenarioControl, device_flags)) ? 1 : -1];
typedef char chk_dp_lifi[(0x28 + __builtin_offsetof(device_placement_data, flags)
                          == __builtin_offsetof(ScenarioLightFixture, device_flags)) ? 1 : -1];

// device_data, written as OBJECT offsets, which is how the module addresses them
typedef char chk_dd_flags[(0x1f4 + __builtin_offsetof(device_data, flags) == 0x1f4) ? 1 : -1];
typedef char chk_dd_pwrgrp[(0x1f4 + __builtin_offsetof(device_data, power_group) == 0x1f8) ? 1 : -1];
typedef char chk_dd_power[(0x1f4 + __builtin_offsetof(device_data, power) == 0x1fc) ? 1 : -1];
typedef char chk_dd_pwrchg[(0x1f4 + __builtin_offsetof(device_data, power_change) == 0x200) ? 1 : -1];
typedef char chk_dd_posgrp[(0x1f4 + __builtin_offsetof(device_data, position_group) == 0x204) ? 1 : -1];
typedef char chk_dd_pos[(0x1f4 + __builtin_offsetof(device_data, position) == 0x208) ? 1 : -1];
typedef char chk_dd_poschg[(0x1f4 + __builtin_offsetof(device_data, position_change) == 0x20c) ? 1 : -1];
typedef char chk_dd_delay[(0x1f4 + __builtin_offsetof(device_data, delay_ticks) == 0x210) ? 1 : -1];
typedef char chk_dd_typeflags[(0x1f4 + __builtin_offsetof(device_data, type_flags) == 0x214) ? 1 : -1];

// device_machine_data tail, as object offsets
typedef char chk_dm_ticks[(0x1f4 + __builtin_offsetof(device_machine_data, ticks_since_fully_open)
                           == 0x218) ? 1 : -1];
typedef char chk_dm_elev[(0x1f4 + __builtin_offsetof(device_machine_data, last_elevator_position)
                          == 0x21c) ? 1 : -1];
typedef char chk_dc_tail[(0x1f4 + __builtin_offsetof(device_control_data, unknown_218) == 0x218) ? 1 : -1];
typedef char chk_dl_tail[(0x1f4 + __builtin_offsetof(device_light_fixture_data, unknown_218) == 0x218) ? 1 : -1];

// the tag-side offsets this module reads, all already in types/tags.h
typedef char chk_dev_size[(sizeof(Device) == 0x290) ? 1 : -1];
typedef char chk_dev_a_in[(__builtin_offsetof(Device, device_a_in) == 0x198) ? 1 : -1];
typedef char chk_dev_radius[(__builtin_offsetof(Device, automatic_activation_radius) == 0x21c) ? 1 : -1];
typedef char chk_dev_ipat[(__builtin_offsetof(Device, inverse_power_acceleration_time) == 0x274) ? 1 : -1];
typedef char chk_dev_iptt[(__builtin_offsetof(Device, inverse_power_transition_time) == 0x278) ? 1 : -1];
typedef char chk_dev_idpat[(__builtin_offsetof(Device, inverse_depowered_position_acceleration_time) == 0x27c) ? 1 : -1];
typedef char chk_dev_idptt[(__builtin_offsetof(Device, inverse_depowered_position_transition_time) == 0x280) ? 1 : -1];
typedef char chk_dev_ipsat[(__builtin_offsetof(Device, inverse_position_acceleration_time) == 0x284) ? 1 : -1];
typedef char chk_dev_ipstt[(__builtin_offsetof(Device, inverse_position_transition_time) == 0x288) ? 1 : -1];
typedef char chk_dev_delay[(__builtin_offsetof(Device, delay_time_ticks) == 0x28c) ? 1 : -1];
typedef char chk_dm_type[(__builtin_offsetof(DeviceMachine, machine_type) == 0x290) ? 1 : -1];
typedef char chk_dm_flags[(__builtin_offsetof(DeviceMachine, machine_flags) == 0x292) ? 1 : -1];
typedef char chk_dm_node[(__builtin_offsetof(DeviceMachine, elevator_node) == 0x2ea) ? 1 : -1];
typedef char chk_dm_open[(__builtin_offsetof(DeviceMachine, door_open_time_ticks) == 0x320) ? 1 : -1];
typedef char chk_dc_type[(__builtin_offsetof(DeviceControl, type) == 0x290) ? 1 : -1];
typedef char chk_dc_trig[(__builtin_offsetof(DeviceControl, triggers_when) == 0x292) ? 1 : -1];
typedef char chk_dc_call[(__builtin_offsetof(DeviceControl, call_value) == 0x294) ? 1 : -1];
typedef char chk_sc_groups[(__builtin_offsetof(Scenario, device_groups) == 0x288) ? 1 : -1];
typedef char chk_sc_machines[(__builtin_offsetof(Scenario, machines) == 0x294) ? 1 : -1];
typedef char chk_sdg_size[(sizeof(ScenarioDeviceGroup) == 0x34) ? 1 : -1];
typedef char chk_sdg_flags[(__builtin_offsetof(ScenarioDeviceGroup, flags) == 0x24) ? 1 : -1];

// the recorded-animation functions misfiled into this module: the evidence that they index
// Scenario.recorded_animations rather than any device block
typedef char chk_sc_recanim[(__builtin_offsetof(Scenario, recorded_animations) == 0x36c) ? 1 : -1];
typedef char chk_sra_size[(sizeof(ScenarioRecordedAnimation) == 0x40) ? 1 : -1];
typedef char chk_sra_version[(__builtin_offsetof(ScenarioRecordedAnimation, version) == 0x20) ? 1 : -1];
typedef char chk_sra_ucdv[(__builtin_offsetof(ScenarioRecordedAnimation, unit_control_data_version) == 0x22) ? 1 : -1];
typedef char chk_sra_length[(__builtin_offsetof(ScenarioRecordedAnimation, length_of_animation) == 0x24) ? 1 : -1];
typedef char chk_sra_stream[(__builtin_offsetof(ScenarioRecordedAnimation, recorded_animation_event_stream)
                             + __builtin_offsetof(TagDataOffset, pointer) == 0x38) ? 1 : -1];
