#include "tags.h"
#include "memory.h"
#include "math.h"
#include "rasterizer.h"
#include "shell.h"

// Every struct in shell.h holds pointers as uint32_t, so the sizes below are exact on any host.
#define CHECK(name, cond) typedef char check_##name[(cond) ? 1 : -1]
#define OFF(t, f) __builtin_offsetof(t, f)

// struct sizes
CHECK(msvc_std_string, sizeof(msvc_std_string) == 0x1c);
CHECK(msvc_std_vector, sizeof(msvc_std_vector) == 0x10);
CHECK(msvc_std_map, sizeof(msvc_std_map) == 0x0c);
CHECK(hwreq_string_pair, sizeof(hwreq_string_pair) == 0x38);
CHECK(hwreq_map_node, sizeof(hwreq_map_node) == 0x30);
CHECK(hwreq_property_set, sizeof(hwreq_property_set) == k_hwreq_property_set_size);
CHECK(shell_sound_device, sizeof(shell_sound_device) == 0x68);
CHECK(shell_sound_device_array, sizeof(shell_sound_device) * k_shell_maximum_sound_devices == 0x104 * 4);
CHECK(shell_display_adapter, sizeof(shell_display_adapter) == 0x34);
CHECK(d3d_adapter_identifier9, sizeof(d3d_adapter_identifier9) == 0x113 * 4);
CHECK(d3d_caps9_copy, sizeof(d3d_caps9) == 0x4c * 4);
CHECK(sound_device_copy, sizeof(shell_sound_device) == 0x1a * 4);
CHECK(hwreq_parser, sizeof(hwreq_parser) == k_hwreq_parser_size);
CHECK(hwreq_parser_vtable, sizeof(hwreq_parser_vtable) == 0x40);
CHECK(shell_config_property, sizeof(shell_config_property) == 8);
CHECK(crash_dialog_template, sizeof(crash_dialog_template) == 0x16);
CHECK(crash_dialog_item_template, sizeof(crash_dialog_item_template) == 0x16);
CHECK(dw_shared_memory, sizeof(dw_shared_memory) == k_dw_shared_memory_size);
CHECK(dw_shared_memory_memset, sizeof(dw_shared_memory) == 0x714 * 4);
CHECK(digital_product_id, sizeof(digital_product_id) == k_digital_product_id_size);

// msvc_std_string / vector / map / node
CHECK(str_buffer, OFF(msvc_std_string, buffer) == 0x04);
CHECK(str_size, OFF(msvc_std_string, size) == 0x14);
CHECK(str_capacity, OFF(msvc_std_string, capacity) == 0x18);
CHECK(vec_first, OFF(msvc_std_vector, first) == 0x04);
CHECK(vec_last, OFF(msvc_std_vector, last) == 0x08);
CHECK(vec_end, OFF(msvc_std_vector, end) == 0x0c);
CHECK(map_head, OFF(msvc_std_map, head) == 0x04);
CHECK(map_size, OFF(msvc_std_map, size) == 0x08);
CHECK(pair_second_buffer, OFF(hwreq_string_pair, second) + OFF(msvc_std_string, buffer) == 0x20);
CHECK(pair_second_capacity, OFF(hwreq_string_pair, second) + OFF(msvc_std_string, capacity) == 0x34);
CHECK(node_key_buffer, OFF(hwreq_map_node, key) + OFF(msvc_std_string, buffer) == 0x10);
CHECK(node_key_size, OFF(hwreq_map_node, key) + OFF(msvc_std_string, size) == 0x20);
CHECK(node_key_capacity, OFF(hwreq_map_node, key) + OFF(msvc_std_string, capacity) == 0x24);
CHECK(node_value, OFF(hwreq_map_node, value) == 0x28);
CHECK(node_color, OFF(hwreq_map_node, color) == 0x2c);
CHECK(node_nil, OFF(hwreq_map_node, is_nil) == 0x2d);
CHECK(set_owner, OFF(hwreq_property_set, owner) == 0x10);

// shell_sound_device
CHECK(snd_guid, OFF(shell_sound_device, guid) == 0x40);
CHECK(snd_driver, OFF(shell_sound_device, driver_version) == 0x50);
CHECK(snd_vendor, OFF(shell_sound_device, vendor_id) == 0x58);
CHECK(snd_device, OFF(shell_sound_device, device_id) == 0x5c);
CHECK(snd_subsys, OFF(shell_sound_device, subsystem_id) == 0x60);
CHECK(snd_revision, OFF(shell_sound_device, revision) == 0x64);

// shell_display_adapter (base 0x006efdc0, name at 0x006efdd0, memory at 0x006efdf0)
CHECK(da_name, 0x006efdc0 + OFF(shell_display_adapter, driver_name) == 0x006efdd0);
CHECK(da_memory, 0x006efdc0 + OFF(shell_display_adapter, video_memory) == 0x006efdf0);
CHECK(da_array_end, 0x006efdc0 + k_shell_maximum_display_adapters * sizeof(shell_display_adapter) == 0x006effc8);
CHECK(snd_array_end, 0x006ef9b0 + k_shell_maximum_sound_devices * sizeof(shell_sound_device) == 0x006efdc0);

// d3d_adapter_identifier9
CHECK(ai_device_name, OFF(d3d_adapter_identifier9, device_name) == 0x400);
CHECK(ai_driver_version, OFF(d3d_adapter_identifier9, driver_version) == 0x420);
CHECK(ai_vendor, OFF(d3d_adapter_identifier9, vendor_id) == 0x428);
CHECK(ai_device, OFF(d3d_adapter_identifier9, device_id) == 0x42c);
CHECK(ai_subsys, OFF(d3d_adapter_identifier9, subsystem_id) == 0x430);
CHECK(ai_revision, OFF(d3d_adapter_identifier9, revision) == 0x434);
CHECK(ai_guid, OFF(d3d_adapter_identifier9, device_identifier) == 0x438);
CHECK(ai_whql, OFF(d3d_adapter_identifier9, whql_level) == 0x448);

// hwreq_parser
CHECK(hp_buffer, OFF(hwreq_parser, file_buffer) == 0x004);
CHECK(hp_cursor, OFF(hwreq_parser, cursor) == 0x008);
CHECK(hp_end, OFF(hwreq_parser, end) == 0x00c);
CHECK(hp_line_start, OFF(hwreq_parser, line_start) == 0x010);
CHECK(hp_line, OFF(hwreq_parser, line_number) == 0x014);
CHECK(hp_flags, OFF(hwreq_parser, flags) == 0x018);
CHECK(hp_requirements, OFF(hwreq_parser, requirements) == 0x01c);
CHECK(hp_error, OFF(hwreq_parser, error_reported) == 0x020);
CHECK(hp_error_message, OFF(hwreq_parser, error_message) == 0x024);
CHECK(hp_error_message_capacity, OFF(hwreq_parser, error_message) + OFF(msvc_std_string, capacity) == 0x3c);
CHECK(hp_gfx_device, OFF(hwreq_parser, graphics_device_name) == 0x040);
CHECK(hp_gfx_device_capacity, OFF(hwreq_parser, graphics_device_name) + OFF(msvc_std_string, capacity) == 0x58);
CHECK(hp_gfx_vendor, OFF(hwreq_parser, graphics_vendor_name) == 0x05c);
CHECK(hp_snd_device, OFF(hwreq_parser, sound_device_name) == 0x078);
CHECK(hp_snd_vendor, OFF(hwreq_parser, sound_vendor_name) == 0x094);
CHECK(hp_snd_vendor_capacity, OFF(hwreq_parser, sound_vendor_name) + OFF(msvc_std_string, capacity) == 0xac);
CHECK(hp_cpu, OFF(hwreq_parser, cpu_speed) == 0x0b0);
CHECK(hp_ram, OFF(hwreq_parser, memory) == 0x0b4);
CHECK(hp_vram, OFF(hwreq_parser, video_memory) == 0x0b8);
CHECK(hp_adapter, OFF(hwreq_parser, adapter) == 0x0bc);
CHECK(hp_driver_version, OFF(hwreq_parser, adapter) + OFF(d3d_adapter_identifier9, driver_version) == 0x4dc);
CHECK(hp_vendor_id, OFF(hwreq_parser, adapter) + OFF(d3d_adapter_identifier9, vendor_id) == 0x4e4);
CHECK(hp_device_id, OFF(hwreq_parser, adapter) + OFF(d3d_adapter_identifier9, device_id) == 0x4e8);
CHECK(hp_subsys, OFF(hwreq_parser, adapter) + OFF(d3d_adapter_identifier9, subsystem_id) == 0x4ec);
CHECK(hp_revision, OFF(hwreq_parser, adapter) + OFF(d3d_adapter_identifier9, revision) == 0x4f0);
CHECK(hp_guid, OFF(hwreq_parser, adapter) + OFF(d3d_adapter_identifier9, device_identifier) == 0x4f4);
CHECK(hp_caps, OFF(hwreq_parser, caps) == 0x508);
CHECK(hp_caps2, OFF(hwreq_parser, caps) + OFF(d3d_caps9, caps2) == 0x514);
CHECK(hp_sound_device, OFF(hwreq_parser, sound_device) == 0x638);
CHECK(hp_sound_vendor, OFF(hwreq_parser, sound_device) + OFF(shell_sound_device, vendor_id) == 0x690);
CHECK(hp_sound_device_id, OFF(hwreq_parser, sound_device) + OFF(shell_sound_device, device_id) == 0x694);
CHECK(hp_sets, OFF(hwreq_parser, property_sets) == 0x6a0);
CHECK(hp_sets_head, OFF(hwreq_parser, property_sets) + OFF(msvc_std_map, head) == 0x6a4);
CHECK(hp_detail, OFF(hwreq_parser, graphic_detail_sets) == 0x6ac);
CHECK(hp_detail_head, OFF(hwreq_parser, graphic_detail_sets) + OFF(msvc_std_map, head) == 0x6b0);

// hwreq_parser_vtable
CHECK(vt_find, OFF(hwreq_parser_vtable, find_property_set) == 0x0c);
CHECK(vt_flag_count, OFF(hwreq_parser_vtable, get_flag_count) == 0x10);
CHECK(vt_req_count, OFF(hwreq_parser_vtable, get_requirement_count) == 0x1c);
CHECK(vt_req_value, OFF(hwreq_parser_vtable, get_requirement_value) == 0x24);
CHECK(vt_gfx_device, OFF(hwreq_parser_vtable, get_graphics_device_name) == 0x28);
CHECK(vt_gfx_vendor, OFF(hwreq_parser_vtable, get_graphics_vendor_name) == 0x2c);
CHECK(vt_error, OFF(hwreq_parser_vtable, get_error_message) == 0x3c);

// config table (0x0069fe40, 0x1c entries, the setter slot 0x0069fe44 of the indirect call)
CHECK(cfg_setter, 0x0069fe40 + OFF(shell_config_property, setter) == 0x0069fe44);
CHECK(cfg_end, 0x0069fe40 + k_shell_config_property_count * sizeof(shell_config_property) == 0x0069ff20);

// crash dialog templates
CHECK(dlg_count, OFF(crash_dialog_template, item_count) == 0x08);
CHECK(dlg_cx, OFF(crash_dialog_template, cx) == 0x0e);
CHECK(dlg_menu, OFF(crash_dialog_template, menu) == 0x12);
CHECK(dlg_class, OFF(crash_dialog_template, window_class) == 0x14);
CHECK(item_x, OFF(crash_dialog_item_template, x) == 0x08);
CHECK(item_id, OFF(crash_dialog_item_template, id) == 0x10);
CHECK(item_class, OFF(crash_dialog_item_template, class_ordinal) == 0x14);

// dw_shared_memory (puVar5[n] dword indices in the decompile)
CHECK(dw_eip, OFF(dw_shared_memory, exception_address) == 3 * 4);
CHECK(dw_pep, OFF(dw_shared_memory, exception_pointers) == 4 * 4);
CHECK(dw_done, OFF(dw_shared_memory, event_done) == 5 * 4);
CHECK(dw_alive, OFF(dw_shared_memory, event_alive) == 7 * 4);
CHECK(dw_mutex, OFF(dw_shared_memory, mutex) == 8 * 4);
CHECK(dw_process, OFF(dw_shared_memory, process) == 9 * 4);
CHECK(dw_behavior, OFF(dw_shared_memory, behavior_flags) == 10 * 4);
CHECK(dw_result, OFF(dw_shared_memory, result) == 11 * 4);
CHECK(dw_offer, OFF(dw_shared_memory, offer_flags) == 0xd * 4);
CHECK(dw_let_run, OFF(dw_shared_memory, let_run_flags) == 0xf * 4);
CHECK(dw_app_name, OFF(dw_shared_memory, application_name) == 0x12 * 4);
CHECK(dw_module, OFF(dw_shared_memory, module_file_name) == 0x2e * 4);
CHECK(dw_server, OFF(dw_shared_memory, server) == 0x38e * 4);
CHECK(dw_subpath, OFF(dw_shared_memory, registry_subpath) == 0x452 * 4);
CHECK(dw_files, OFF(dw_shared_memory, additional_files) == 0x506 * 4);

// digital_product_id
CHECK(pid_major, OFF(digital_product_id, major_version) == 0x04);
CHECK(pid_minor, OFF(digital_product_id, minor_version) == 0x06);
CHECK(pid_string, OFF(digital_product_id, product_id) == 0x08);
CHECK(pid_20, OFF(digital_product_id, unknown_20) == 0x20);
CHECK(pid_key, OFF(digital_product_id, hashed_key) == 0x38);

// enum values the code tests
CHECK(cpu_athlon, k_cpu_model_amd_athlon == 6);
CHECK(cpu_sse, k_cpu_query_sse_usable == 0x1d && k_cpu_query_3dnow == 0x1a);
CHECK(os_nt, k_os_platform_windows_nt == 3);

int main(void) { return 0; }
