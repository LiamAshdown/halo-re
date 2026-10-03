/**
 * standalone/data/eq_data.cpp -- the engine globals of the initialised .rdata/.data image that the C code reaches
 * through several names or as one larger object (tables strided past their declared end, a struct seen through field
 * names). They were absolute EQU symbols in standalone/globals.asm.
 *
 * Each object is in its own section ".geq$<original address>v"; the linker sorts the group by name, so the objects
 * keep the original address order. A cluster of overlapping or adjacent globals is one gap-free run: every object
 * spans up to the next one (or the cluster end), and a 16-aligned pad ".geq$<cluster start>" in front keeps the run
 * congruent with the original addresses, so every name sits at its original offset from the cluster start. Initial
 * values are the bytes the data image holds at the original address: code pointers name the function, pointers into a
 * global that is now a C definition point at that definition. tools/globals_check_eq.py verifies layout and bytes.
 *
 * All definitions sit in one extern "C" block: the ordered sections, the /alternatename pragmas and src/ reach these objects by their unmangled C names.
 */
#include "tables.h"
#include "halo/projectiles/api.hpp"
#include "code_refs.hpp"
#include "halo/shell/api.hpp"
#include "halo/cseries/api.hpp"
#include <stdint.h>

extern "C" {

extern uint8_t cache_file_current_header_crc32[];


/** the globals the tables point into, defined below or in a slice file */
extern uint32_t actor_mode_guard_look_weights_ambush[];
extern uint32_t length_error_throw_info[];
extern uint8_t multiplayer_sound_enabled[];
extern uint32_t network_game_messages_group[];
extern uint32_t out_of_range_throw_info[];

/** 0x00000050..0x00000051: absolute address 0x50 in the original (a store through a register Ghidra read as a constant); a byte here */
#pragma section(".geq$00000050v", read, write)
__declspec(allocate(".geq$00000050v")) __declspec(align(1)) uint8_t DAT_00000050[1] = {0};

/** 0x0065b638..0x0065b660: hs_enum_definitions */
#pragma section(".geq$0065b638", read, write)
__declspec(allocate(".geq$0065b638")) __declspec(align(16)) uint8_t eq_pad_0065b638[8] = {0};
#pragma section(".geq$0065b638v", read, write)
__declspec(allocate(".geq$0065b638v")) __declspec(align(8)) uint32_t hs_enum_definitions[10] = {
    0x00000004u, (uint32_t)&message_delta_definitions_006872b8[39], 0x0000000au, (uint32_t)&message_delta_definitions_006872b8[43], 0x0000000cu, (uint32_t)&hs_enum_definitions_006853f8[24], 0x00000010u, (uint32_t)&hs_enum_definitions_006853f8[0],
    0x00000005u, (uint32_t)"HPf"
};

/** 0x0065d440..0x0065d520: message_delta_definitions */
#pragma section(".geq$0065d440v", read, write)
__declspec(allocate(".geq$0065d440v")) __declspec(align(16)) uint32_t message_delta_definitions[56] = {
    (uint32_t)&message_delta_definitions_0069aed0[82], (uint32_t)&message_delta_definitions_00695f28[112], (uint32_t)&message_delta_definitions_00695c60[84], (uint32_t)&message_delta_definitions_006961c0[194], (uint32_t)&message_delta_definitions_0069a998[274], (uint32_t)&message_delta_definitions_0069a998[26], (uint32_t)&message_delta_definitions_00692f58[0], (uint32_t)&message_delta_definitions_00688670[86],
    (uint32_t)&message_delta_definitions_00688670[116], (uint32_t)&message_delta_definitions_0069a998[138], (uint32_t)&message_delta_definitions_00688670[178], (uint32_t)&message_delta_definitions_006872b8[0], (uint32_t)&message_delta_definitions_0069a998[160], (uint32_t)&message_delta_definitions_006982ec[11], (uint32_t)&message_delta_definitions_00688670[226], (uint32_t)&message_delta_definitions_00692e70[0],
    (uint32_t)&message_delta_definitions_00688670[0], (uint32_t)&message_delta_definitions_00687e48[10], (uint32_t)&message_delta_definitions_00688490[0], (uint32_t)&message_delta_definitions_00687ff0[0], (uint32_t)&message_delta_definitions_00688570[0], (uint32_t)&message_delta_definitions_00687b1c[11], (uint32_t)&message_delta_definitions_00687b1c[93], (uint32_t)&message_delta_definitions_00687b1c[111],
    (uint32_t)&message_delta_definitions_00687a20[26], (uint32_t)((uint8_t *)&multiplayer_sound_enabled + 0x30), (uint32_t)&message_delta_definitions_00688670[56], (uint32_t)&message_delta_definitions_0069a998[222], (uint32_t)&message_delta_definitions_0069a998[64],
    (uint32_t)&message_delta_definitions_0069aed0[0], (uint32_t)&message_delta_definitions_00695f28[54], (uint32_t)&message_delta_definitions_00695c60[22], (uint32_t)&message_delta_definitions_006961c0[116], (uint32_t)&message_delta_definitions_00698718[0], (uint32_t)&message_delta_definitions_0069b1a8[0], (uint32_t)&message_delta_definitions_00699948[0], (uint32_t)&message_delta_definitions_00699948[26],
    (uint32_t)&message_delta_definitions_00699948[90], (uint32_t)&message_delta_definitions_00699948[132], (uint32_t)&message_delta_definitions_00699948[192], (uint32_t)&message_delta_definitions_00699948[210], (uint32_t)&message_delta_definitions_00699948[270], (uint32_t)&message_delta_definitions_00699948[316], (uint32_t)&message_delta_definitions_006961c0[26], (uint32_t)&message_delta_definitions_006961c0[0],
    (uint32_t)&message_delta_definitions_006961c0[56], (uint32_t)&message_delta_definitions_006961c0[86], (uint32_t)&message_delta_definitions_00687a20[0], (uint32_t)&message_delta_definitions_00695f28[0], (uint32_t)&message_delta_definitions_0069aed0[100], (uint32_t)&message_delta_definitions_0069aed0[126], (uint32_t)&message_delta_definitions_00695f28[28], (uint32_t)&message_delta_definitions_00697e90[0],
    (uint32_t)((uint8_t *)&network_game_messages_group + 0x18), (uint32_t)&message_delta_definitions_006996e8[0], (uint32_t)&message_delta_definitions_006997b0[0]
};

/** 0x00672f20..0x00672f24: unknown_00672f20 */
#pragma section(".geq$00672f20v", read, write)
__declspec(allocate(".geq$00672f20v")) __declspec(align(16)) uint32_t unknown_00672f20[1] = {0x3f7d70a4u};

/** 0x00673524..0x0067359c: length_error_throw_info, out_of_range_throw_info */
#pragma section(".geq$00673524", read, write)
__declspec(allocate(".geq$00673524")) __declspec(align(16)) uint8_t eq_pad_00673524[4] = {0};
#pragma section(".geq$00673524v", read, write)
__declspec(allocate(".geq$00673524v")) __declspec(align(4)) uint32_t length_error_throw_info[15] = {
    0x00000000u, (uint32_t)&halo::shell::hwreq_length_error_destruct, 0x00000000u,
    (uint32_t)((uint8_t *)&length_error_throw_info + 0x10), 0x00000003u,
    (uint32_t)((uint8_t *)&length_error_throw_info + 0x20), (uint32_t)&length_error_throw_info_0067359c[0],
    (uint32_t)((uint8_t *)&out_of_range_throw_info + 0x20), 0x00000000u, (uint32_t)&length_error_throw_info_0069ff2c[0], 0x00000000u, 0xffffffffu,
    0x00000000u, 0x00000028u, (uint32_t)&halo::shell::std_length_error_copy_construct
};
#pragma section(".geq$00673560v", read, write)
__declspec(allocate(".geq$00673560v")) __declspec(align(16)) uint32_t out_of_range_throw_info[15] = {
    0x00000000u, (uint32_t)&halo::shell::hwreq_out_of_range_destruct, 0x00000000u,
    (uint32_t)((uint8_t *)&out_of_range_throw_info + 0x10), 0x00000003u, (uint32_t)&length_error_throw_info_0067359c[7], (uint32_t)&length_error_throw_info_0067359c[0],
    (uint32_t)((uint8_t *)&out_of_range_throw_info + 0x20), 0x00000000u, (uint32_t)&length_error_throw_info_0069ff2c[8], 0x00000000u, 0xffffffffu,
    0x00000000u, 0x0000000cu, (uint32_t)&halo::shell::exception_copy_construct
};

/** 0x006851f4..0x006853f8: hud_text_message_hold_color, global_white_argb, hud_text_message_normal_color and 6 more */
#pragma section(".geq$006851f4", read, write)
__declspec(allocate(".geq$006851f4")) __declspec(align(16)) uint8_t eq_pad_006851f4[4] = {0};
#pragma section(".geq$006851f4v", read, write)
__declspec(allocate(".geq$006851f4v")) __declspec(align(4)) uint32_t hud_text_message_hold_color[2] = {(uint32_t)&global_white_argb_00655138[40], (uint32_t)&global_white_argb_00655138[24]};
#pragma section(".geq$006851fcv", read, write)
__declspec(allocate(".geq$006851fcv")) __declspec(align(4)) uint32_t global_white_argb[1] = {(uint32_t)&global_white_argb_00655138[0]};
#pragma section(".geq$00685200v", read, write)
__declspec(allocate(".geq$00685200v")) __declspec(align(16)) uint32_t hud_text_message_normal_color[1] = {(uint32_t)&global_white_argb_00655138[28]};
#pragma section(".geq$00685204v", read, write)
__declspec(allocate(".geq$00685204v")) __declspec(align(4)) uint32_t actor_mode_uncover_look_weights_active[1] = {(uint32_t)&global_white_argb_00655138[20]};
#pragma section(".geq$00685208v", read, write)
__declspec(allocate(".geq$00685208v")) __declspec(align(8)) uint32_t actor_mode_guard_look_weights_idle[3] = {(uint32_t)&global_white_argb_00655138[60], (uint32_t)&global_white_argb_00655138[52], (uint32_t)&global_white_argb_00655138[32]};
#pragma section(".geq$00685214v", read, write)
__declspec(allocate(".geq$00685214v")) __declspec(align(4)) uint32_t console_color_00685214[1] = {(uint32_t)&global_white_argb_00655138[56]};
#pragma section(".geq$00685218v", read, write)
__declspec(allocate(".geq$00685218v")) __declspec(align(8)) uint32_t console_message_default_color[1] = {(uint32_t)&global_white_argb_00655138[12]};
#pragma section(".geq$0068521cv", read, write)
__declspec(allocate(".geq$0068521cv")) __declspec(align(4)) uint32_t actor_mode_guard_look_weights_ambush[103] = {
    (uint32_t)&global_white_argb_00655138[48], (uint32_t)&global_white_argb_00655138[44], (uint32_t)&global_white_argb_00655138[4], (uint32_t)"asleep", (uint32_t)"alert", (uint32_t)"combat", 0x00000000u, (uint32_t)"flood carrier",
    0x00000010u, 0x00020002u, 0x00000001u, 0x00000000u, (uint32_t)actor_type_flood_carrier_update, 0x00000000u,
    0x00000000u, (uint32_t)"crew", 0x00000002u, 0x00000000u, 0x00000000u, 0x00000000u, (uint32_t)actor_type_crew_update,
    0x00000000u, 0x00000000u, (uint32_t)"elite", 0x00000004u, 0x00010001u, 0x00000001u, 0x00000000u,
    (uint32_t)actor_type_elite_update, 0x00000000u, 0x00000000u, (uint32_t)"engineer", 0x00000004u, 0x00000000u, 0x00000000u,
    0x00000000u, (uint32_t)actor_type_engineer_update, 0x00000000u, 0x00000000u, (uint32_t)"flood", 0x00000008u, 0x00000000u,
    0x00000001u, 0x00000000u, (uint32_t)actor_type_flood_update, 0x00000000u, 0x00000000u, (uint32_t)"grunt", 0x00000004u,
    0x00000000u, 0x00000000u, 0x00000000u, (uint32_t)actor_type_grunt_update, 0x00000000u, 0x00000000u, (uint32_t)"hunter",
    0x00000004u, 0x00010001u, 0x00000001u, 0x00000000u, (uint32_t)actor_type_hunter_update, 0x00000000u, 0x00000000u,
    (uint32_t)"infection", 0x00020020u, 0x00020002u, 0x00000100u, 0x00000000u, (uint32_t)actor_type_infection_update,
    (uint32_t)actor_type_infection_swarm_update, (uint32_t)actor_compute_swarm_avoidance_offset, (uint32_t)"jackal",
    0x00020004u, 0x00020000u, 0x00000000u, 0x00000000u, (uint32_t)actor_type_jackal_update, 0x00000000u, 0x00000000u,
    (uint32_t)"marine", 0x00000002u, 0x00000000u, 0x00000000u, 0x00000000u, (uint32_t)actor_type_marine_update, 0x00000000u,
    0x00000000u, (uint32_t)"mounted_weapon", 0x00020000u, 0x00020002u, 0x00000000u, 0x00000000u,
    (uint32_t)actor_type_mounted_weapon_update, 0x00000000u, 0x00000000u, (uint32_t)"sentinel", 0x00000040u, 0x00000000u,
    0x00000000u, 0x00000000u, (uint32_t)actor_type_sentinel_update, 0x00000000u, 0x00000000u
};
#pragma section(".geq$006853b8v", read, write)
__declspec(allocate(".geq$006853b8v")) __declspec(align(8)) uint32_t actor_type_procs[16] = {
    (uint32_t)((uint8_t *)&actor_mode_guard_look_weights_ambush + 0x5c),
    (uint32_t)((uint8_t *)&actor_mode_guard_look_weights_ambush + 0x11c),
    (uint32_t)((uint8_t *)&actor_mode_guard_look_weights_ambush + 0xbc),
    (uint32_t)((uint8_t *)&actor_mode_guard_look_weights_ambush + 0xdc),
    (uint32_t)((uint8_t *)&actor_mode_guard_look_weights_ambush + 0x7c),
    (uint32_t)((uint8_t *)&actor_mode_guard_look_weights_ambush + 0x5c),
    (uint32_t)((uint8_t *)&actor_mode_guard_look_weights_ambush + 0x13c),
    (uint32_t)((uint8_t *)&actor_mode_guard_look_weights_ambush + 0x13c),
    (uint32_t)((uint8_t *)&actor_mode_guard_look_weights_ambush + 0x3c),
    (uint32_t)((uint8_t *)&actor_mode_guard_look_weights_ambush + 0x9c),
    (uint32_t)((uint8_t *)&actor_mode_guard_look_weights_ambush + 0xfc),
    (uint32_t)((uint8_t *)&actor_mode_guard_look_weights_ambush + 0x1c),
    (uint32_t)((uint8_t *)&actor_mode_guard_look_weights_ambush + 0x17c),
    (uint32_t)((uint8_t *)&actor_mode_guard_look_weights_ambush + 0x17c),
    (uint32_t)((uint8_t *)&actor_mode_guard_look_weights_ambush + 0xbc),
    (uint32_t)((uint8_t *)&actor_mode_guard_look_weights_ambush + 0x15c)
};

/** 0x006869c0..0x006869c4: map_download */
#pragma section(".geq$006869c0v", read, write)
__declspec(allocate(".geq$006869c0v")) __declspec(align(16)) uint32_t map_download[1] = {(uint32_t)(cache_file_current_header_crc32 + 0x7a8)};

/** 0x006869d0..0x00686a10: camera_script, unknown_006869d1, director_camera_mode and 2 more */
#pragma section(".geq$006869d0v", read, write)
__declspec(allocate(".geq$006869d0v")) __declspec(align(16)) uint8_t camera_script[1] = {0x00};
#pragma section(".geq$006869d1v", read, write)
__declspec(allocate(".geq$006869d1v")) __declspec(align(1)) uint8_t unknown_006869d1[1] = {0x00};
#pragma section(".geq$006869d2v", read, write)
__declspec(allocate(".geq$006869d2v")) __declspec(align(2)) uint8_t director_camera_mode[6] = {0xff, 0xff, 0xff, 0xff, 0x00, 0x00};
#pragma section(".geq$006869d8v", read, write)
__declspec(allocate(".geq$006869d8v")) __declspec(align(8)) uint32_t camera_script_time_remaining[11] = {
    0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x3f800000u, 0x00000000u,
    0x3f800000u, 0x00000000u, 0x3f9c61aau
};
#pragma section(".geq$00686a04v", read, write)
__declspec(allocate(".geq$00686a04v")) __declspec(align(4)) uint32_t director_camera_target[3] = {0xffffffffu, 0xffffffffu, 0x00000000u};

/** 0x00686af8..0x00686b1c: actor_mode_default_look_weights, actor_mode_guard_look_weights_a5, actor_mode_guard_look_weights_a6 and 6 more */
#pragma section(".geq$00686af8", read, write)
__declspec(allocate(".geq$00686af8")) __declspec(align(16)) uint8_t eq_pad_00686af8[8] = {0};
#pragma section(".geq$00686af8v", read, write)
__declspec(allocate(".geq$00686af8v")) __declspec(align(8)) uint32_t actor_mode_default_look_weights[1] = {(uint32_t)&global_white_argb_00655138[16]};
#pragma section(".geq$00686afcv", read, write)
__declspec(allocate(".geq$00686afcv")) __declspec(align(4)) uint32_t actor_mode_guard_look_weights_a5[1] = {(uint32_t)&global_white_argb_00655138[36]};
#pragma section(".geq$00686b00v", read, write)
__declspec(allocate(".geq$00686b00v")) __declspec(align(16)) uint32_t actor_mode_guard_look_weights_a6[1] = {(uint32_t)&global_white_argb_00655138[64]};
#pragma section(".geq$00686b04v", read, write)
__declspec(allocate(".geq$00686b04v")) __declspec(align(4)) uint32_t global_white_color[1] = {(uint32_t)&global_white_argb_00655138[1]};
#pragma section(".geq$00686b08v", read, write)
__declspec(allocate(".geq$00686b08v")) __declspec(align(8)) uint32_t object_ambient_lightmap_default[1] = {(uint32_t)&global_white_argb_00655138[5]};
#pragma section(".geq$00686b0cv", read, write)
__declspec(allocate(".geq$00686b0cv")) __declspec(align(4)) uint32_t default_axis_b[1] = {(uint32_t)&global_white_argb_00655138[9]};
#pragma section(".geq$00686b10v", read, write)
__declspec(allocate(".geq$00686b10v")) __declspec(align(16)) uint32_t default_color_a[1] = {(uint32_t)&global_white_argb_00655138[13]};
#pragma section(".geq$00686b14v", read, write)
__declspec(allocate(".geq$00686b14v")) __declspec(align(4)) uint32_t global_real_rgb_green_pointer[1] = {(uint32_t)&global_white_argb_00655138[17]};
#pragma section(".geq$00686b18v", read, write)
__declspec(allocate(".geq$00686b18v")) __declspec(align(8)) uint32_t default_color_b[1] = {(uint32_t)&global_white_argb_00655138[21]};

/** 0x00686d88..0x00686d98: unit_control_data_version_layouts */
#pragma section(".geq$00686d88", read, write)
__declspec(allocate(".geq$00686d88")) __declspec(align(16)) uint8_t eq_pad_00686d88[8] = {0};
#pragma section(".geq$00686d88v", read, write)
__declspec(allocate(".geq$00686d88v")) __declspec(align(8)) uint32_t unit_control_data_version_layouts[4] = {(uint32_t)",lh", (uint32_t)"tlh", (uint32_t)"Plh", (uint32_t)"Plh"};

/** 0x00686fe8..0x00686ff8: recorded_animation_codecs_by_version */
#pragma section(".geq$00686fe8", read, write)
__declspec(allocate(".geq$00686fe8")) __declspec(align(16)) uint8_t eq_pad_00686fe8[8] = {0};
#pragma section(".geq$00686fe8v", read, write)
__declspec(allocate(".geq$00686fe8v")) __declspec(align(8)) uint32_t recorded_animation_codecs_by_version[4] = {(uint32_t)&recorded_animation_codecs_by_version_00686fd8[2], (uint32_t)&recorded_animation_codecs_by_version_00686fd8[2], (uint32_t)&recorded_animation_codecs_by_version_00686fd8[2], (uint32_t)&recorded_animation_codecs_by_version_00686fd8[0]};

/** 0x00687130..0x00687134: object_network_id_table */
#pragma section(".geq$00687130v", read, write)
__declspec(allocate(".geq$00687130v")) __declspec(align(16)) uint32_t object_network_id_table[1] = {(uint32_t)&object_network_id_table_006870a8[0]};

/** 0x00687558..0x0068755c: machine_table */
#pragma section(".geq$00687558", read, write)
__declspec(allocate(".geq$00687558")) __declspec(align(16)) uint8_t eq_pad_00687558[8] = {0};
#pragma section(".geq$00687558v", read, write)
__declspec(allocate(".geq$00687558v")) __declspec(align(8)) uint32_t machine_table[1] = {(uint32_t)&machine_table_006874d0[0]};

/** 0x00687af0..0x00687af2: teleport_flash_type */
#pragma section(".geq$00687af0v", read, write)
__declspec(allocate(".geq$00687af0v")) __declspec(align(16)) uint8_t teleport_flash_type[2] = {0x06, 0x00};

/** 0x00688308..0x00688324: game_engine_definitions */
#pragma section(".geq$00688308", read, write)
__declspec(allocate(".geq$00688308")) __declspec(align(16)) uint8_t eq_pad_00688308[8] = {0};
#pragma section(".geq$00688308v", read, write)
__declspec(allocate(".geq$00688308v")) __declspec(align(8)) uint32_t game_engine_definitions[7] = {
    0x00000000u, (uint32_t)&message_delta_definitions_00687b1c[129], (uint32_t)&message_delta_definitions_00687ff0[66], (uint32_t)&message_delta_definitions_00687ff0[154], (uint32_t)&message_delta_definitions_00687e48[32], (uint32_t)&message_delta_definitions_00687ff0[110], (uint32_t)&message_delta_definitions_00687ff0[22]
};

/** 0x00688b58..0x00689380: hs_function_definitions */
#pragma section(".geq$00688b58", read, write)
__declspec(allocate(".geq$00688b58")) __declspec(align(16)) uint8_t eq_pad_00688b58[8] = {0};
#pragma section(".geq$00688b58v", read, write)
__declspec(allocate(".geq$00688b58v")) __declspec(align(8)) uint32_t hs_function_definitions[522] = {
    (uint32_t)&hs_function_definitions_00657660[0], (uint32_t)&hs_function_definitions_00657660[7], (uint32_t)&hs_function_definitions_00657660[14], (uint32_t)&hs_function_definitions_00657660[21], (uint32_t)&hs_function_definitions_00657660[28], (uint32_t)&hs_function_definitions_00657660[35], (uint32_t)&hs_function_definitions_00657660[42], (uint32_t)&hs_function_definitions_00657660[49],
    (uint32_t)&hs_function_definitions_00657660[56], (uint32_t)&hs_function_definitions_00657660[63], (uint32_t)&hs_function_definitions_00657660[70], (uint32_t)&hs_function_definitions_00657660[77], (uint32_t)&hs_function_definitions_00657660[84], (uint32_t)&hs_function_definitions_00657660[91], (uint32_t)&hs_function_definitions_00657660[98], (uint32_t)&hs_function_definitions_00657660[105],
    (uint32_t)&hs_function_definitions_00657660[112], (uint32_t)&hs_function_definitions_00657660[119], (uint32_t)&hs_function_definitions_00657660[126], (uint32_t)&hs_function_definitions_00657660[133], (uint32_t)&hs_function_definitions_00657660[140], (uint32_t)&hs_function_definitions_00657660[147], (uint32_t)&hs_function_definitions_00657660[154], (uint32_t)&hs_function_definitions_00657660[161],
    (uint32_t)&hs_function_definitions_00657660[168], (uint32_t)&hs_function_definitions_00657660[175], (uint32_t)&hs_function_definitions_00657660[182], (uint32_t)&hs_function_definitions_00657660[189], (uint32_t)&hs_function_definitions_00657660[197], (uint32_t)&hs_function_definitions_00657660[205], (uint32_t)&hs_function_definitions_00657660[212], (uint32_t)&hs_function_definitions_00657660[220],
    (uint32_t)&hs_function_definitions_00657660[228], (uint32_t)&hs_function_definitions_00657660[236], (uint32_t)&hs_function_definitions_00657660[299], (uint32_t)&hs_function_definitions_00657660[307], (uint32_t)&hs_function_definitions_00657660[315], (uint32_t)&hs_function_definitions_00657660[323], (uint32_t)&hs_function_definitions_00657660[244], (uint32_t)&hs_function_definitions_00657660[252],
    (uint32_t)&hs_function_definitions_00657660[260], (uint32_t)&hs_function_definitions_00657660[268], (uint32_t)&hs_function_definitions_00657660[276], (uint32_t)&hs_function_definitions_00657660[284], (uint32_t)&hs_function_definitions_00657660[292], (uint32_t)&hs_function_definitions_00657660[332], (uint32_t)&hs_function_definitions_00657660[340], (uint32_t)&hs_function_definitions_00657660[348],
    (uint32_t)&hs_function_definitions_00657660[356], (uint32_t)&hs_function_definitions_00657660[365], (uint32_t)&hs_function_definitions_00657660[373], (uint32_t)&hs_function_definitions_00657660[381], (uint32_t)&hs_function_definitions_00657660[390], (uint32_t)&hs_function_definitions_00657660[399], (uint32_t)&hs_function_definitions_00657660[407], (uint32_t)&hs_function_definitions_00657660[415],
    (uint32_t)&hs_function_definitions_00657660[423], (uint32_t)&hs_function_definitions_00657660[430], (uint32_t)&hs_function_definitions_00657660[3817], (uint32_t)&hs_function_definitions_00657660[437], (uint32_t)&hs_function_definitions_00657660[445], (uint32_t)&hs_function_definitions_00657660[453], (uint32_t)&hs_function_definitions_00657660[461], (uint32_t)&hs_function_definitions_00657660[469],
    (uint32_t)&hs_function_definitions_00657660[476], (uint32_t)&hs_function_definitions_00657660[483], (uint32_t)&hs_function_definitions_00657660[491], (uint32_t)&hs_function_definitions_00657660[499], (uint32_t)&hs_function_definitions_00657660[507], (uint32_t)&hs_function_definitions_00657660[515], (uint32_t)&hs_function_definitions_00657660[523], (uint32_t)&hs_function_definitions_00657660[531],
    (uint32_t)&hs_function_definitions_00657660[539], (uint32_t)&hs_function_definitions_00657660[547], (uint32_t)&hs_function_definitions_00657660[554], (uint32_t)&hs_function_definitions_00657660[562], (uint32_t)&hs_function_definitions_00657660[571], (uint32_t)&hs_function_definitions_00657660[580], (uint32_t)&hs_function_definitions_00657660[588], (uint32_t)&hs_function_definitions_00657660[595],
    (uint32_t)&hs_function_definitions_00657660[603], (uint32_t)&hs_function_definitions_00657660[611], (uint32_t)&hs_function_definitions_00657660[619], (uint32_t)&hs_function_definitions_00657660[627], (uint32_t)&hs_function_definitions_00657660[658], (uint32_t)&hs_function_definitions_00657660[635], (uint32_t)&hs_function_definitions_00657660[643], (uint32_t)&hs_function_definitions_00657660[651],
    (uint32_t)&hs_function_definitions_00657660[666], (uint32_t)&hs_function_definitions_00657660[674], (uint32_t)&hs_function_definitions_00657660[682], (uint32_t)&hs_function_definitions_00657660[691], (uint32_t)&hs_function_definitions_00657660[700], (uint32_t)&hs_function_definitions_00657660[708], (uint32_t)&hs_function_definitions_00657660[716], (uint32_t)&hs_function_definitions_00657660[724],
    (uint32_t)&hs_function_definitions_00657660[732], (uint32_t)&hs_function_definitions_00657660[740], (uint32_t)&hs_function_definitions_00657660[748], (uint32_t)&hs_function_definitions_00657660[756], (uint32_t)&hs_function_definitions_00657660[782], (uint32_t)&hs_function_definitions_00657660[764], (uint32_t)&hs_function_definitions_00657660[773], (uint32_t)&hs_function_definitions_00657660[792],
    (uint32_t)&hs_function_definitions_00657660[800], (uint32_t)&hs_function_definitions_00657660[808], (uint32_t)&hs_function_definitions_00657660[816], (uint32_t)&hs_function_definitions_00657660[824], (uint32_t)&hs_function_definitions_00657660[833], (uint32_t)&hs_function_definitions_00657660[842], (uint32_t)&hs_function_definitions_00657660[851], (uint32_t)&hs_function_definitions_00657660[859],
    (uint32_t)&hs_function_definitions_00657660[867], (uint32_t)&hs_function_definitions_00657660[876], (uint32_t)&hs_function_definitions_00657660[885], (uint32_t)&hs_function_definitions_00657660[894], (uint32_t)&hs_function_definitions_00657660[903], (uint32_t)&hs_function_definitions_00657660[912], (uint32_t)&hs_function_definitions_00657660[920], (uint32_t)&hs_function_definitions_00657660[928],
    (uint32_t)&hs_function_definitions_00657660[936], (uint32_t)&hs_function_definitions_00657660[943], (uint32_t)&hs_function_definitions_00657660[951], (uint32_t)&hs_function_definitions_00657660[959], (uint32_t)&hs_function_definitions_00657660[967], (uint32_t)&hs_function_definitions_00657660[975], (uint32_t)&hs_function_definitions_00657660[983], (uint32_t)&hs_function_definitions_00657660[991],
    (uint32_t)&hs_function_definitions_00657660[999], (uint32_t)&hs_function_definitions_00657660[1007], (uint32_t)&hs_function_definitions_00657660[1015], (uint32_t)&hs_function_definitions_00657660[1023], (uint32_t)&hs_function_definitions_00657660[1031], (uint32_t)&hs_function_definitions_00657660[1038], (uint32_t)&hs_function_definitions_00657660[1046], (uint32_t)&hs_function_definitions_00657660[1054],
    (uint32_t)&hs_function_definitions_00657660[1062], (uint32_t)&hs_function_definitions_00657660[1078], (uint32_t)&hs_function_definitions_00657660[1070], (uint32_t)&hs_function_definitions_00657660[1086], (uint32_t)&hs_function_definitions_00657660[1094], (uint32_t)&hs_function_definitions_00657660[1102], (uint32_t)&hs_function_definitions_00657660[1110], (uint32_t)&hs_function_definitions_00657660[1118],
    (uint32_t)&hs_function_definitions_00657660[1126], (uint32_t)&hs_function_definitions_00657660[1134], (uint32_t)&hs_function_definitions_00657660[1142], (uint32_t)&hs_function_definitions_00657660[1150], (uint32_t)&hs_function_definitions_00657660[1158], (uint32_t)&hs_function_definitions_00657660[1165], (uint32_t)&hs_function_definitions_00657660[3840], (uint32_t)&hs_function_definitions_00657660[1172],
    (uint32_t)&hs_function_definitions_00657660[3847], (uint32_t)&hs_function_definitions_00657660[1179], (uint32_t)&hs_function_definitions_00657660[1186], (uint32_t)&hs_function_definitions_00657660[1193], (uint32_t)&hs_function_definitions_00657660[1201], (uint32_t)&hs_function_definitions_00657660[1224], (uint32_t)&hs_function_definitions_00657660[1232], (uint32_t)&hs_function_definitions_00657660[1240],
    (uint32_t)&hs_function_definitions_00657660[1256], (uint32_t)&hs_function_definitions_00657660[1264], (uint32_t)&hs_function_definitions_00657660[1280], (uint32_t)&hs_function_definitions_00657660[1288], (uint32_t)&hs_function_definitions_00657660[1296], (uint32_t)&hs_function_definitions_00657660[1304], (uint32_t)&hs_function_definitions_00657660[1312], (uint32_t)&hs_function_definitions_00657660[1319],
    (uint32_t)&hs_function_definitions_00657660[1327], (uint32_t)&hs_function_definitions_00657660[1334], (uint32_t)&hs_function_definitions_00657660[1342], (uint32_t)&hs_function_definitions_00657660[1350], (uint32_t)&hs_function_definitions_00657660[1358], (uint32_t)&hs_function_definitions_00657660[1366], (uint32_t)&hs_function_definitions_00657660[1374], (uint32_t)&hs_function_definitions_00657660[1382],
    (uint32_t)&hs_function_definitions_00657660[1398], (uint32_t)&hs_function_definitions_00657660[1406], (uint32_t)&hs_function_definitions_00657660[1414], (uint32_t)&hs_function_definitions_00657660[1422], (uint32_t)&hs_function_definitions_00657660[1430], (uint32_t)&hs_function_definitions_00657660[1438], (uint32_t)&hs_function_definitions_00657660[1446], (uint32_t)&hs_function_definitions_00657660[1454],
    (uint32_t)&hs_function_definitions_00657660[1462], (uint32_t)&hs_function_definitions_00657660[1471], (uint32_t)&hs_function_definitions_00657660[1479], (uint32_t)&hs_function_definitions_00657660[1487], (uint32_t)&hs_function_definitions_00657660[1872], (uint32_t)&hs_function_definitions_00657660[1880], (uint32_t)&hs_function_definitions_00657660[1888], (uint32_t)&hs_function_definitions_00657660[1896],
    (uint32_t)&hs_function_definitions_00657660[1904], (uint32_t)&hs_function_definitions_00657660[1912], (uint32_t)&hs_function_definitions_00657660[1495], (uint32_t)&hs_function_definitions_00657660[1504], (uint32_t)&hs_function_definitions_00657660[1864], (uint32_t)&hs_function_definitions_00657660[1513], (uint32_t)&hs_function_definitions_00657660[1521], (uint32_t)&hs_function_definitions_00657660[1529],
    (uint32_t)&hs_function_definitions_00657660[1537], (uint32_t)&hs_function_definitions_00657660[1545], (uint32_t)&hs_function_definitions_00657660[1553], (uint32_t)&hs_function_definitions_00657660[1561], (uint32_t)&hs_function_definitions_00657660[1569], (uint32_t)&hs_function_definitions_00657660[1577], (uint32_t)&hs_function_definitions_00657660[1585], (uint32_t)&hs_function_definitions_00657660[1593],
    (uint32_t)&hs_function_definitions_00657660[1601], (uint32_t)&hs_function_definitions_00657660[1609], (uint32_t)&hs_function_definitions_00657660[1617], (uint32_t)&hs_function_definitions_00657660[1625], (uint32_t)&hs_function_definitions_00657660[1856], (uint32_t)&hs_function_definitions_00657660[1848], (uint32_t)&hs_function_definitions_00657660[1633], (uint32_t)&hs_function_definitions_00657660[1641],
    (uint32_t)&hs_function_definitions_00657660[1649], (uint32_t)&hs_function_definitions_00657660[1657], (uint32_t)&hs_function_definitions_00657660[1665], (uint32_t)&hs_function_definitions_00657660[1920], (uint32_t)&hs_function_definitions_00657660[1673], (uint32_t)&hs_function_definitions_00657660[1680], (uint32_t)&hs_function_definitions_00657660[1688], (uint32_t)&hs_function_definitions_00657660[1696],
    (uint32_t)&hs_function_definitions_00657660[1704], (uint32_t)&hs_function_definitions_00657660[1712], (uint32_t)&hs_function_definitions_00657660[1720], (uint32_t)&hs_function_definitions_00657660[1728], (uint32_t)&hs_function_definitions_00657660[1736], (uint32_t)&hs_function_definitions_00657660[1744], (uint32_t)&hs_function_definitions_00657660[1752], (uint32_t)&hs_function_definitions_00657660[1760],
    (uint32_t)&hs_function_definitions_00657660[1768], (uint32_t)&hs_function_definitions_00657660[1776], (uint32_t)&hs_function_definitions_00657660[1784], (uint32_t)&hs_function_definitions_00657660[1928], (uint32_t)&hs_function_definitions_00657660[1792], (uint32_t)&hs_function_definitions_00657660[1800], (uint32_t)&hs_function_definitions_00657660[1936], (uint32_t)&hs_function_definitions_00657660[1944],
    (uint32_t)&hs_function_definitions_00657660[1808], (uint32_t)&hs_function_definitions_00657660[1816], (uint32_t)&hs_function_definitions_00657660[1824], (uint32_t)&hs_function_definitions_00657660[1832], (uint32_t)&hs_function_definitions_00657660[1840], (uint32_t)&hs_function_definitions_00657660[1952], (uint32_t)&hs_function_definitions_00657660[1960], (uint32_t)&hs_function_definitions_00657660[1968],
    (uint32_t)&hs_function_definitions_00657660[1976], (uint32_t)&hs_function_definitions_00657660[1985], (uint32_t)&hs_function_definitions_00657660[1993], (uint32_t)&hs_function_definitions_00657660[2001], (uint32_t)&hs_function_definitions_00657660[2009], (uint32_t)&hs_function_definitions_00657660[2023], (uint32_t)&hs_function_definitions_00657660[2016], (uint32_t)&hs_function_definitions_00657660[3868],
    (uint32_t)&hs_function_definitions_00657660[2038], (uint32_t)&hs_function_definitions_00657660[2030], (uint32_t)&hs_function_definitions_00657660[2045], (uint32_t)&hs_function_definitions_00657660[2052], (uint32_t)&hs_function_definitions_00657660[2189], (uint32_t)&hs_function_definitions_00657660[2196], (uint32_t)&hs_function_definitions_00657660[2203], (uint32_t)&hs_function_definitions_00657660[3876],
    (uint32_t)&hs_function_definitions_00657660[2210], (uint32_t)&hs_function_definitions_00657660[2218], (uint32_t)&hs_function_definitions_00657660[2226], (uint32_t)&hs_function_definitions_00657660[2234], (uint32_t)&hs_function_definitions_00657660[2242], (uint32_t)&hs_function_definitions_00657660[2249], (uint32_t)&hs_function_definitions_00657660[2256], (uint32_t)&hs_function_definitions_00657660[3825],
    (uint32_t)&hs_function_definitions_00657660[2263], (uint32_t)&hs_function_definitions_00657660[2270], (uint32_t)&hs_function_definitions_00657660[2277], (uint32_t)&hs_function_definitions_00657660[2284], (uint32_t)&hs_function_definitions_00657660[2291], (uint32_t)&hs_function_definitions_00657660[2298], (uint32_t)&hs_function_definitions_00657660[2306], (uint32_t)&hs_function_definitions_00657660[2313],
    (uint32_t)&hs_function_definitions_00657660[2320], (uint32_t)&hs_function_definitions_00657660[2328], (uint32_t)&hs_function_definitions_00657660[2336], (uint32_t)&hs_function_definitions_00657660[2344], (uint32_t)&hs_function_definitions_00657660[3832], (uint32_t)&hs_function_definitions_00657660[1208], (uint32_t)&hs_function_definitions_00657660[1216], (uint32_t)&hs_function_definitions_00657660[2352],
    (uint32_t)&hs_function_definitions_00657660[2359], (uint32_t)&hs_function_definitions_00657660[2366], (uint32_t)&hs_function_definitions_00657660[2374], (uint32_t)&hs_function_definitions_00657660[2382], (uint32_t)&hs_function_definitions_00657660[2390], (uint32_t)&hs_function_definitions_00657660[2398], (uint32_t)&hs_function_definitions_00657660[2407], (uint32_t)&hs_function_definitions_00657660[2416],
    (uint32_t)&hs_function_definitions_00657660[2423], (uint32_t)&hs_function_definitions_00657660[2430], (uint32_t)&hs_function_definitions_00657660[2437], (uint32_t)&hs_function_definitions_00657660[2444], (uint32_t)&hs_function_definitions_00657660[2451], (uint32_t)&hs_function_definitions_00657660[2459], (uint32_t)&hs_function_definitions_00657660[2467], (uint32_t)&hs_function_definitions_00657660[2475],
    (uint32_t)&hs_function_definitions_00657660[2483], (uint32_t)&hs_function_definitions_00657660[2490], (uint32_t)&hs_function_definitions_00657660[2497], (uint32_t)&hs_function_definitions_00657660[2504], (uint32_t)&hs_function_definitions_00657660[2511], (uint32_t)&hs_function_definitions_00657660[2518], (uint32_t)&hs_function_definitions_00657660[2525], (uint32_t)&hs_function_definitions_00657660[2532],
    (uint32_t)&hs_function_definitions_00657660[2539], (uint32_t)&hs_function_definitions_00657660[2546], (uint32_t)&hs_function_definitions_00657660[2553], (uint32_t)&hs_function_definitions_00657660[2560], (uint32_t)&hs_function_definitions_00657660[2567], (uint32_t)&hs_function_definitions_00657660[2613], (uint32_t)&hs_function_definitions_00657660[3861], (uint32_t)&hs_function_definitions_00657660[2597],
    (uint32_t)&hs_function_definitions_00657660[3854], (uint32_t)&hs_function_definitions_00657660[2574], (uint32_t)&hs_function_definitions_00657660[2581], (uint32_t)&hs_function_definitions_00657660[2589], (uint32_t)&hs_function_definitions_00657660[2605], (uint32_t)&hs_function_definitions_00657660[2620], (uint32_t)&hs_function_definitions_00657660[2628], (uint32_t)&hs_function_definitions_00657660[2637],
    (uint32_t)&hs_function_definitions_00657660[2645], (uint32_t)&hs_function_definitions_00657660[2653], (uint32_t)&hs_function_definitions_00657660[2661], (uint32_t)&hs_function_definitions_00657660[2670], (uint32_t)&hs_function_definitions_00657660[2678], (uint32_t)&hs_function_definitions_00657660[2686], (uint32_t)&hs_function_definitions_00657660[2694], (uint32_t)&hs_function_definitions_00657660[2702],
    (uint32_t)&hs_function_definitions_00657660[2711], (uint32_t)&hs_function_definitions_00657660[2728], (uint32_t)&hs_function_definitions_00657660[2736], (uint32_t)&hs_function_definitions_00657660[2744], (uint32_t)&hs_function_definitions_00657660[2751], (uint32_t)&hs_function_definitions_00657660[2759], (uint32_t)&hs_function_definitions_00657660[2766], (uint32_t)&hs_function_definitions_00657660[2774],
    (uint32_t)&hs_function_definitions_00657660[2719], (uint32_t)&hs_function_definitions_00657660[2781], (uint32_t)&hs_function_definitions_00657660[2059], (uint32_t)&hs_function_definitions_00657660[2066], (uint32_t)&hs_function_definitions_00657660[2074], (uint32_t)&hs_function_definitions_00657660[2082], (uint32_t)&hs_function_definitions_00657660[2089], (uint32_t)&hs_function_definitions_00657660[2096],
    (uint32_t)&hs_function_definitions_00657660[2103], (uint32_t)&hs_function_definitions_00657660[2110], (uint32_t)&hs_function_definitions_00657660[2117], (uint32_t)&hs_function_definitions_00657660[2124], (uint32_t)&hs_function_definitions_00657660[2131], (uint32_t)&hs_function_definitions_00657660[2138], (uint32_t)&hs_function_definitions_00657660[2145], (uint32_t)&hs_function_definitions_00657660[2152],
    (uint32_t)&hs_function_definitions_00657660[2159], (uint32_t)&hs_function_definitions_00657660[2166], (uint32_t)&hs_function_definitions_00657660[2173], (uint32_t)&hs_function_definitions_00657660[2180], (uint32_t)&hs_function_definitions_00657660[2789], (uint32_t)&hs_function_definitions_00657660[2797], (uint32_t)&hs_function_definitions_00657660[2805], (uint32_t)&hs_function_definitions_00657660[2813],
    (uint32_t)&hs_function_definitions_00657660[2820], (uint32_t)&hs_function_definitions_00657660[2829], (uint32_t)&hs_function_definitions_00657660[2838], (uint32_t)&hs_function_definitions_00657660[2847], (uint32_t)&hs_function_definitions_00657660[2856], (uint32_t)&hs_function_definitions_00657660[2864], (uint32_t)&hs_function_definitions_00657660[2872], (uint32_t)&hs_function_definitions_00657660[2880],
    (uint32_t)&hs_function_definitions_00657660[3787], (uint32_t)&hs_function_definitions_00657660[3794], (uint32_t)&hs_function_definitions_00657660[3802], (uint32_t)&hs_function_definitions_00657660[2888], (uint32_t)&hs_function_definitions_00657660[2898], (uint32_t)&hs_function_definitions_00657660[2905], (uint32_t)&hs_function_definitions_00657660[2912], (uint32_t)&hs_function_definitions_00657660[2919],
    (uint32_t)&hs_function_definitions_00657660[2926], (uint32_t)&hs_function_definitions_00657660[2934], (uint32_t)&hs_function_definitions_00657660[2942], (uint32_t)&hs_function_definitions_00657660[2949], (uint32_t)&hs_function_definitions_00657660[2956], (uint32_t)&hs_function_definitions_00657660[2964], (uint32_t)&hs_function_definitions_00657660[2972], (uint32_t)&hs_function_definitions_00657660[2979],
    (uint32_t)&hs_function_definitions_00657660[2988], (uint32_t)&hs_function_definitions_00657660[2997], (uint32_t)&hs_function_definitions_00657660[3005], (uint32_t)&hs_function_definitions_00657660[3013], (uint32_t)&hs_function_definitions_00657660[3021], (uint32_t)&hs_function_definitions_00657660[3029], (uint32_t)&hs_function_definitions_00657660[3037], (uint32_t)&hs_function_definitions_00657660[3045],
    (uint32_t)&hs_function_definitions_00657660[3053], (uint32_t)&hs_function_definitions_00657660[3061], (uint32_t)&hs_function_definitions_00657660[3069], (uint32_t)&hs_function_definitions_00657660[3077], (uint32_t)&hs_function_definitions_00657660[3084], (uint32_t)&hs_function_definitions_00657660[3092], (uint32_t)&hs_function_definitions_00657660[3100], (uint32_t)&hs_function_definitions_00657660[3108],
    (uint32_t)&hs_function_definitions_00657660[3116], (uint32_t)&hs_function_definitions_00657660[3125], (uint32_t)&hs_function_definitions_00657660[3133], (uint32_t)&hs_function_definitions_00657660[3141], (uint32_t)&hs_function_definitions_00657660[3148], (uint32_t)&hs_function_definitions_00657660[3156], (uint32_t)&hs_function_definitions_00657660[3164], (uint32_t)&hs_function_definitions_00657660[3171],
    (uint32_t)&hs_function_definitions_00657660[3179], (uint32_t)&hs_function_definitions_00657660[3187], (uint32_t)&hs_function_definitions_00657660[3194], (uint32_t)&hs_function_definitions_00657660[3201], (uint32_t)&hs_function_definitions_00657660[3210], (uint32_t)&hs_function_definitions_00657660[3217], (uint32_t)&hs_function_definitions_00657660[3225], (uint32_t)&hs_function_definitions_00657660[3233],
    (uint32_t)&hs_function_definitions_00657660[3243], (uint32_t)&hs_function_definitions_00657660[3253], (uint32_t)&hs_function_definitions_00657660[3262], (uint32_t)&hs_function_definitions_00657660[3270], (uint32_t)&hs_function_definitions_00657660[3277], (uint32_t)&hs_function_definitions_00657660[3285], (uint32_t)&hs_function_definitions_00657660[3292], (uint32_t)&hs_function_definitions_00657660[3301],
    (uint32_t)&hs_function_definitions_00657660[3308], (uint32_t)&hs_function_definitions_00657660[3316], (uint32_t)&hs_function_definitions_00657660[3323], (uint32_t)&hs_function_definitions_00657660[3330], (uint32_t)&hs_function_definitions_00657660[3338], (uint32_t)&hs_function_definitions_00657660[3346], (uint32_t)&hs_function_definitions_00657660[3354], (uint32_t)&hs_function_definitions_00657660[3361],
    (uint32_t)&hs_function_definitions_00657660[3369], (uint32_t)&hs_function_definitions_00657660[3377], (uint32_t)&hs_function_definitions_00657660[3385], (uint32_t)&hs_function_definitions_00657660[3392], (uint32_t)&hs_function_definitions_00657660[3400], (uint32_t)&hs_function_definitions_00657660[3408], (uint32_t)&hs_function_definitions_00657660[3415], (uint32_t)&hs_function_definitions_00657660[3423],
    (uint32_t)&hs_function_definitions_00657660[3431], (uint32_t)&hs_function_definitions_00657660[3439], (uint32_t)&hs_function_definitions_00657660[3447], (uint32_t)&hs_function_definitions_00657660[3454], (uint32_t)&hs_function_definitions_00657660[3462], (uint32_t)&hs_function_definitions_00657660[3470], (uint32_t)&hs_function_definitions_00657660[3478], (uint32_t)&hs_function_definitions_00657660[3486],
    (uint32_t)&hs_function_definitions_00657660[3494], (uint32_t)&hs_function_definitions_00657660[3502], (uint32_t)&hs_function_definitions_00657660[3510], (uint32_t)&hs_function_definitions_00657660[3518], (uint32_t)&hs_function_definitions_00657660[3526], (uint32_t)&hs_function_definitions_00657660[3534], (uint32_t)&hs_function_definitions_00657660[3542], (uint32_t)&hs_function_definitions_00657660[3550],
    (uint32_t)&hs_function_definitions_00657660[3558], (uint32_t)&hs_function_definitions_00657660[3566], (uint32_t)&hs_function_definitions_00657660[3574], (uint32_t)&hs_function_definitions_00657660[3582], (uint32_t)&hs_function_definitions_00657660[3590], (uint32_t)&hs_function_definitions_00657660[3598], (uint32_t)&hs_function_definitions_00657660[3606], (uint32_t)&hs_function_definitions_00657660[3614],
    (uint32_t)&hs_function_definitions_00657660[3622], (uint32_t)&hs_function_definitions_00657660[3630], (uint32_t)&hs_function_definitions_00657660[3638], (uint32_t)&hs_function_definitions_00657660[3646], (uint32_t)&hs_function_definitions_00657660[3654], (uint32_t)&hs_function_definitions_00657660[3662], (uint32_t)&hs_function_definitions_00657660[3670], (uint32_t)&hs_function_definitions_00657660[3678],
    (uint32_t)&hs_function_definitions_00657660[3686], (uint32_t)&hs_function_definitions_00657660[3694], (uint32_t)&hs_function_definitions_00657660[3703], (uint32_t)&hs_function_definitions_00657660[3711], (uint32_t)&hs_function_definitions_00657660[3942], (uint32_t)&hs_function_definitions_00657660[3718], (uint32_t)&hs_function_definitions_00657660[4013], (uint32_t)&hs_function_definitions_00657660[4006],
    (uint32_t)&hs_function_definitions_00657660[4020], (uint32_t)&hs_function_definitions_00657660[4028], (uint32_t)&hs_function_definitions_00657660[3906], (uint32_t)&hs_function_definitions_00657660[3913], (uint32_t)&hs_function_definitions_00657660[3920], (uint32_t)&hs_function_definitions_00657660[3884], (uint32_t)&hs_function_definitions_00657660[3928], (uint32_t)&hs_function_definitions_00657660[3949],
    (uint32_t)&hs_function_definitions_00657660[3956], (uint32_t)&hs_function_definitions_00657660[3971], (uint32_t)&hs_function_definitions_00657660[3891], (uint32_t)&hs_function_definitions_00657660[3898], (uint32_t)&hs_function_definitions_00657660[3726], (uint32_t)&hs_function_definitions_00657660[3733], (uint32_t)&hs_function_definitions_00657660[3964], (uint32_t)&hs_function_definitions_00657660[4036],
    (uint32_t)&hs_function_definitions_00657660[4043], (uint32_t)&hs_function_definitions_00657660[3809], (uint32_t)&hs_function_definitions_00657660[3740], (uint32_t)&hs_function_definitions_00657660[3748], (uint32_t)&hs_function_definitions_00657660[3756], (uint32_t)&hs_function_definitions_00657660[3764], (uint32_t)&hs_function_definitions_00657660[3772], (uint32_t)&hs_function_definitions_00657660[3779],
    (uint32_t)&hs_function_definitions_00657660[4064], (uint32_t)&hs_function_definitions_00657660[4071], (uint32_t)&hs_function_definitions_00657660[4050], (uint32_t)&hs_function_definitions_00657660[4057], (uint32_t)&hs_function_definitions_00657660[3978], (uint32_t)&hs_function_definitions_00657660[3985], (uint32_t)&hs_function_definitions_00657660[3992], (uint32_t)&hs_function_definitions_00657660[3999],
    (uint32_t)&hs_function_definitions_00657660[3935], (uint32_t)&hs_function_definitions_00657660[4078]
};

/** 0x0068e588..0x0068e66c: multiplayer_maps, map_per_map_table */
#pragma section(".geq$0068e588", read, write)
__declspec(allocate(".geq$0068e588")) __declspec(align(16)) uint8_t eq_pad_0068e588[8] = {0};
#pragma section(".geq$0068e588v", read, write)
__declspec(allocate(".geq$0068e588v")) __declspec(align(8)) uint32_t multiplayer_maps[2] = {0x00000000u, (uint32_t)"beavercreek"};
#pragma section(".geq$0068e590v", read, write)
__declspec(allocate(".geq$0068e590v")) __declspec(align(16)) uint32_t map_per_map_table[55] = {
    0x00000001u, 0x00000001u, (uint32_t)"sidewinder", 0x00000001u, 0x00000002u, (uint32_t)"damnation", 0x00000001u, 0x00000003u,
    (uint32_t)"ratrace", 0x00000001u, 0x00000004u, (uint32_t)"prisoner", 0x00000001u, 0x00000005u, (uint32_t)"hangemhigh", 0x00000001u,
    0x00000006u, (uint32_t)"chillout", 0x00000001u, 0x00000007u, (uint32_t)"carousel", 0x00000001u, 0x00000008u, (uint32_t)"boardingaction",
    0x00000001u, 0x00000009u, (uint32_t)"bloodgulch", 0x00000001u, 0x0000000au, (uint32_t)"wizard", 0x00000001u, 0x0000000bu,
    (uint32_t)"putput", 0x00000001u, 0x0000000cu, (uint32_t)"longest", 0x00000001u, 0x0000000du, (uint32_t)"icefields", 0x00000000u,
    0x0000000eu, (uint32_t)"deathisland", 0x00000000u, 0x0000000fu, (uint32_t)"dangercanyon", 0x00000000u, 0x00000010u, (uint32_t)"infinity",
    0x00000000u, 0x00000011u, (uint32_t)"timberland", 0x00000000u, 0x00000012u, (uint32_t)"gephyrophobia", 0x00000000u
};

/** 0x00692fe8..0x006932e8: controls_action_table, controls_row_device_mask_table */
#pragma section(".geq$00692fe8", read, write)
__declspec(allocate(".geq$00692fe8")) __declspec(align(16)) uint8_t eq_pad_00692fe8[8] = {0};
#pragma section(".geq$00692fe8v", read, write)
__declspec(allocate(".geq$00692fe8v")) __declspec(align(8)) uint32_t controls_action_table[5] = {0x77726f66u, 0x00647261u, 0x00000000u, 0x00000000u, 0x00000001u};
#pragma section(".geq$00692ffcv", read, write)
__declspec(allocate(".geq$00692ffcv")) __declspec(align(4)) uint32_t controls_row_device_mask_table[187] = {
    0x00000000u, 0x6b636162u, 0x64726177u, 0x00000000u, 0x00000000u, 0x00000002u, 0x00000000u, 0x7466656cu,
    0x00000000u, 0x00000000u, 0x00000000u, 0x00000003u, 0x00000000u, 0x68676972u, 0x00000074u, 0x00000000u,
    0x00000000u, 0x00000004u, 0x00000000u, 0x6b6f6f6cu, 0x0070755fu, 0x00000000u, 0x00000000u, 0x00000005u,
    0x00000002u, 0x6b6f6f6cu, 0x776f645fu, 0x0000006eu, 0x00000000u, 0x00000006u, 0x00000002u, 0x6b6f6f6cu,
    0x66656c5fu, 0x00000074u, 0x00000000u, 0x00000007u, 0x00000002u, 0x6b6f6f6cu, 0x6769725fu, 0x00007468u,
    0x00000000u, 0x00000008u, 0x00000002u, 0x65726966u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000009u,
    0x00000000u, 0x6f726874u, 0x72675f77u, 0x64616e65u, 0x00000065u, 0x0000000au, 0x00000000u, 0x74697773u,
    0x675f6863u, 0x616e6572u, 0x00006564u, 0x0000000bu, 0x00000000u, 0x74697773u, 0x775f6863u, 0x6f706165u,
    0x0000006eu, 0x0000000cu, 0x00000000u, 0x6f6c6572u, 0x00006461u, 0x00000000u, 0x00000000u, 0x0000000du,
    0x00000000u, 0x656c656du, 0x00000065u, 0x00000000u, 0x00000000u, 0x0000000eu, 0x00000000u, 0x68637865u,
    0x65676e61u, 0x6165775fu, 0x006e6f70u, 0x0000000fu, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u,
    0x00000000u, 0x00000000u, 0x00000000u, 0x706d756au, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000010u,
    0x00000000u, 0x756f7263u, 0x00006863u, 0x00000000u, 0x00000000u, 0x00000011u, 0x00000000u, 0x73616c66u,
    0x67696c68u, 0x00007468u, 0x00000000u, 0x00000012u, 0x00000000u, 0x6d6f6f7au, 0x00000000u, 0x00000000u,
    0x00000000u, 0x00000013u, 0x00000000u, 0x69746361u, 0x00006e6fu, 0x00000000u, 0x00000000u, 0x00000014u,
    0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u,
    0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u,
    0x00000000u, 0x00000000u, 0x00000000u, 0x65636361u, 0x00007470u, 0x00000000u, 0x00000000u, 0x00000015u,
    0x00000007u, 0x6b636162u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000016u, 0x00000007u, 0x00796173u,
    0x00000000u, 0x00000000u, 0x00000000u, 0x00000017u, 0x00000000u, 0x74796173u, 0x006d6165u, 0x00000000u,
    0x00000000u, 0x00000018u, 0x00000000u, 0x76796173u, 0x63696865u, 0x0000656cu, 0x00000000u, 0x00000019u,
    0x00000000u, 0x776f6873u, 0x726f6373u, 0x00007365u, 0x00000000u, 0x0000001au, 0x00000000u, 0x00000000u,
    0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u,
    0x00000000u, 0x00000000u, 0x00000000u
};

/** 0x006994f8..0x00699528: network_game_messages_group */
#pragma section(".geq$006994f8", read, write)
__declspec(allocate(".geq$006994f8")) __declspec(align(16)) uint8_t eq_pad_006994f8[8] = {0};
#pragma section(".geq$006994f8v", read, write)
__declspec(allocate(".geq$006994f8v")) __declspec(align(8)) uint32_t network_game_messages_group[12] = {
    (uint32_t)"network_game_messages_group", 0x00080027u, 0x00000600u, 0x00000800u, (uint32_t)&message_delta_definitions_00698718[810], 0x00000000u, 0x00000035u, 0xffffffffu,
    0xffffffffu, 0xffffffffu, 0xffffffffu, 0x00000010u
};

/** 0x00699f40..0x00699f48: unknown_00699f40, network_scenario_round_counter_a */
#pragma section(".geq$00699f40v", read, write)
__declspec(allocate(".geq$00699f40v")) __declspec(align(16)) uint32_t unknown_00699f40[1] = {0x00000001u};
#pragma section(".geq$00699f44v", read, write)
__declspec(allocate(".geq$00699f44v")) __declspec(align(4)) uint32_t network_scenario_round_counter_a[1] = {0x00000000u};

/** 0x0069a2f0..0x0069a5a4: message_delta_field_type_table, message_delta_unknown_table_0069a304 */
#pragma section(".geq$0069a2f0v", read, write)
__declspec(allocate(".geq$0069a2f0v")) __declspec(align(16)) uint32_t message_delta_field_type_table[5] = {
    0x00000000u, 0x00000001u, (uint32_t)message_delta_integer_compute_size, (uint32_t)message_delta_integer_initialize,
    (uint32_t)&halo::cseries::function_do_nothing
};
#pragma section(".geq$0069a304v", read, write)
__declspec(allocate(".geq$0069a304v")) __declspec(align(4)) uint32_t message_delta_unknown_table_0069a304[168] = {
    0x00000000u, 0x00000001u, 0x00000000u, (uint32_t)message_delta_compute_size_32,
    (uint32_t)halo::projectiles::object_type_definition_return_true, (uint32_t)&halo::cseries::function_do_nothing, 0x00000000u, 0x00000002u, 0x00000000u,
    (uint32_t)message_delta_compute_size_1, (uint32_t)halo::projectiles::object_type_definition_return_true,
    (uint32_t)&halo::cseries::function_do_nothing, 0x00000000u, 0x00000003u, 0x00000000u, (uint32_t)message_delta_compute_size_8,
    (uint32_t)halo::projectiles::object_type_definition_return_true, (uint32_t)&halo::cseries::function_do_nothing, 0x00000000u, 0x00000004u, 0x00000000u,
    (uint32_t)message_delta_compute_size_16, (uint32_t)halo::projectiles::object_type_definition_return_true,
    (uint32_t)&halo::cseries::function_do_nothing, 0x00000000u, 0x00000005u, 0x00000001u, (uint32_t)message_delta_string_compute_size,
    (uint32_t)message_delta_count_initialize, (uint32_t)&halo::cseries::function_do_nothing, 0x00000000u, 0x00000006u, 0x00000001u,
    (uint32_t)message_delta_wide_string_compute_size, (uint32_t)message_delta_count_initialize,
    (uint32_t)&halo::cseries::function_do_nothing, 0x00000000u, 0x00000007u, 0x00000001u, (uint32_t)message_delta_blob_compute_size,
    (uint32_t)message_delta_count_initialize, (uint32_t)&halo::cseries::function_do_nothing, 0x00000000u, 0x00000008u, 0x00000001u,
    (uint32_t)message_delta_structure_array_compute_size, (uint32_t)message_delta_structure_array_initialize,
    (uint32_t)&halo::cseries::function_do_nothing, 0x00000000u, 0x00000009u, 0x00000001u,
    (uint32_t)message_delta_compound_compute_size, (uint32_t)message_delta_compound_initialize,
    (uint32_t)&halo::cseries::function_do_nothing, 0x00000000u, 0x0000000au, 0x00000001u, (uint32_t)message_delta_pointer_compute_size,
    (uint32_t)message_delta_pointer_initialize, (uint32_t)&halo::cseries::function_do_nothing, 0x00000000u, 0x0000000bu, 0x00000001u,
    (uint32_t)message_delta_enum_width_compute_size, (uint32_t)message_delta_integer_initialize,
    (uint32_t)&halo::cseries::function_do_nothing, 0x00000000u, 0x0000000cu, 0x00000001u, (uint32_t)message_delta_range_compute_size,
    (uint32_t)message_delta_range_initialize, (uint32_t)&halo::cseries::function_do_nothing, 0x00000000u, 0x0000000du, 0x00000001u,
    (uint32_t)message_delta_index_compute_size, (uint32_t)message_delta_index_initialize,
    (uint32_t)message_delta_index_teardown, 0x00000000u, 0x0000000eu, 0x00000001u,
    (uint32_t)message_delta_scalar_array_compute_size, (uint32_t)message_delta_count_initialize,
    (uint32_t)&halo::cseries::function_do_nothing, 0x00000000u, 0x0000000fu, 0x00000001u,
    (uint32_t)message_delta_scalar_array_compute_size, (uint32_t)message_delta_count_initialize,
    (uint32_t)&halo::cseries::function_do_nothing, 0x00000000u, 0x00000010u, 0x00000000u, (uint32_t)message_delta_compute_size_32,
    (uint32_t)halo::projectiles::object_type_definition_return_true, (uint32_t)&halo::cseries::function_do_nothing, 0x00000000u, 0x00000011u, 0x00000001u,
    (uint32_t)message_delta_first_dword_compute_size, (uint32_t)message_delta_flags_initialize,
    (uint32_t)&halo::cseries::function_do_nothing, 0x00000000u, 0x00000012u, 0x00000000u, (uint32_t)message_delta_compute_size_32,
    (uint32_t)halo::projectiles::object_type_definition_return_true, (uint32_t)&halo::cseries::function_do_nothing, 0x00000000u, 0x00000013u, 0x00000000u,
    (uint32_t)message_delta_compute_size_6, (uint32_t)halo::projectiles::object_type_definition_return_true,
    (uint32_t)&halo::cseries::function_do_nothing, 0x00000000u, 0x00000014u, 0x00000001u,
    (uint32_t)message_delta_first_dword_compute_size, (uint32_t)message_delta_quantized_real_initialize,
    (uint32_t)&halo::cseries::function_do_nothing, 0x00000000u, 0x00000015u, 0x00000001u, (uint32_t)message_delta_normal_compute_size,
    (uint32_t)message_delta_normal_initialize, (uint32_t)&halo::cseries::function_do_nothing, 0x00000000u, 0x00000016u, 0x00000000u,
    (uint32_t)message_delta_locality_compute_size, (uint32_t)message_delta_locality_initialize,
    (uint32_t)&halo::cseries::function_do_nothing, 0x00000000u, 0x00000017u, 0x00000000u, (uint32_t)message_delta_compute_size_4,
    (uint32_t)halo::projectiles::object_type_definition_return_true, (uint32_t)&halo::cseries::function_do_nothing, 0x00000000u, 0x00000018u, 0x00000000u,
    (uint32_t)message_delta_compute_size_3, (uint32_t)halo::projectiles::object_type_definition_return_true,
    (uint32_t)&halo::cseries::function_do_nothing, 0x00000000u, 0x00000019u, 0x00000000u, (uint32_t)message_delta_compute_size_2,
    (uint32_t)halo::projectiles::object_type_definition_return_true, (uint32_t)&halo::cseries::function_do_nothing, 0x00000000u, 0x0000001au, 0x00000001u,
    (uint32_t)message_delta_velocity_compute_size, (uint32_t)waypoint_table_quantize_initialize,
    (uint32_t)&halo::cseries::function_do_nothing, 0x00000000u, 0x0000001bu, 0x00000000u,
    (uint32_t)message_delta_item_placement_compute_size, (uint32_t)message_delta_item_placement_initialize,
    (uint32_t)&halo::cseries::function_do_nothing, 0x00000000u, 0x00000013u, 0x6e657267u, 0x5f656461u, 0x6e756f63u, 0x00007374u
};

/** 0x0069bfdc..0x0069c00c: object_type_definitions, object_type_definitions_ex */
#pragma section(".geq$0069bfdc", read, write)
__declspec(allocate(".geq$0069bfdc")) __declspec(align(16)) uint8_t eq_pad_0069bfdc[12] = {0};
#pragma section(".geq$0069bfdcv", read, write)
__declspec(allocate(".geq$0069bfdcv")) __declspec(align(4)) uint32_t object_type_definitions[1] = {(uint32_t)&message_delta_definitions_0069b1a8[210]};
#pragma section(".geq$0069bfe0v", read, write)
__declspec(allocate(".geq$0069bfe0v")) __declspec(align(16)) uint32_t object_type_definitions_ex[11] = {
    (uint32_t)&message_delta_definitions_0069b1a8[260], (uint32_t)&message_delta_definitions_0069b1a8[360], (uint32_t)&message_delta_definitions_0069b1a8[410], (uint32_t)&message_delta_definitions_0069b1a8[460], (uint32_t)&message_delta_definitions_0069b1a8[510], (uint32_t)&message_delta_definitions_0069b1a8[560], (uint32_t)&message_delta_definitions_0069b1a8[710], (uint32_t)&message_delta_definitions_0069b1a8[760],
    (uint32_t)&message_delta_definitions_0069b1a8[810], (uint32_t)&message_delta_definitions_0069b1a8[860], (uint32_t)&message_delta_definitions_0069b1a8[610]
};

/** 0x0069c634..0x0069c644: game_window_top_left, game_window_bottom_right, game_screen_rect and 3 more */
#pragma section(".geq$0069c634", read, write)
__declspec(allocate(".geq$0069c634")) __declspec(align(16)) uint8_t eq_pad_0069c634[4] = {0};
#pragma section(".geq$0069c634v", read, write)
__declspec(allocate(".geq$0069c634v")) __declspec(align(4)) uint32_t game_window_top_left[1] = {0x00000000u};
#pragma section(".geq$0069c638v", read, write)
__declspec(allocate(".geq$0069c638v")) __declspec(align(8)) uint32_t game_window_bottom_right[1] = {0x00000000u};
#pragma section(".geq$0069c63cv", read, write)
__declspec(allocate(".geq$0069c63cv")) __declspec(align(4)) uint8_t game_screen_rect[2] = {0x00, 0x00};
#pragma section(".geq$0069c63ev", read, write)
__declspec(allocate(".geq$0069c63ev")) __declspec(align(2)) uint8_t unknown_0069c63e[2] = {0x00, 0x00};
#pragma section(".geq$0069c640v", read, write)
__declspec(allocate(".geq$0069c640v")) __declspec(align(16)) uint8_t unknown_0069c640[2] = {0x00, 0x00};
#pragma section(".geq$0069c642v", read, write)
__declspec(allocate(".geq$0069c642v")) __declspec(align(2)) uint8_t unknown_0069c642[2] = {0x00, 0x00};

/** 0x0069d410..0x0069e550: rasterizer_effects, unknown_0069da10, rasterizer_screen_flash_effect and 4 more */
#pragma section(".geq$0069d410v", read, write)
__declspec(allocate(".geq$0069d410v")) __declspec(align(16)) uint32_t rasterizer_effects[384] = {
    0x00000000u, 0x0000000du, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, (uint32_t)"environment_lightmap_normal",
    0x00000000u, 0x0000000du, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, (uint32_t)"environment_lightmap_no_lightmap",
    0x00000000u, 0x0000000du, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, (uint32_t)"environment_lightmap_no_illumination",
    0x00000000u, 0x0000000du, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, (uint32_t)"environment_lightmap_no_illumination_no_lightmap",
    0x00000000u, 0x00000009u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, (uint32_t)"environment_diffuse_lights",
    0x00000000u, 0x00000017u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, (uint32_t)"environment_texture_normal_biased_multiply_biased_multiply",
    0x00000000u, 0x00000017u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, (uint32_t)"environment_texture_normal_biased_multiply_multiply",
    0x00000000u, 0x00000017u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, (uint32_t)"environment_texture_normal_biased_multiply_biased_add",
    0x00000000u, 0x00000017u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, (uint32_t)"environment_texture_normal_multiply_biased_multiply",
    0x00000000u, 0x00000017u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, (uint32_t)"environment_texture_normal_multiply_multiply",
    0x00000000u, 0x00000017u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, (uint32_t)"environment_texture_normal_multiply_biased_add",
    0x00000000u, 0x00000017u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, (uint32_t)"environment_texture_normal_biased_add_biased_multiply",
    0x00000000u, 0x00000017u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, (uint32_t)"environment_texture_normal_biased_add_multiply",
    0x00000000u, 0x00000017u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, (uint32_t)"environment_texture_normal_biased_add_biased_add",
    0x00000000u, 0x00000017u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, (uint32_t)"environment_texture_blended_biased_multiply_biased_multiply",
    0x00000000u, 0x00000017u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, (uint32_t)"environment_texture_blended_biased_multiply_multiply",
    0x00000000u, 0x00000017u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, (uint32_t)"environment_texture_blended_biased_multiply_biased_add",
    0x00000000u, 0x00000017u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, (uint32_t)"environment_texture_blended_multiply_biased_multiply",
    0x00000000u, 0x00000017u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, (uint32_t)"environment_texture_blended_multiply_multiply",
    0x00000000u, 0x00000017u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, (uint32_t)"environment_texture_blended_multiply_biased_add",
    0x00000000u, 0x00000017u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, (uint32_t)"environment_texture_blended_biased_add_biased_multiply",
    0x00000000u, 0x00000017u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, (uint32_t)"environment_texture_blended_biased_add_multiply",
    0x00000000u, 0x00000017u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, (uint32_t)"environment_texture_blended_biased_add_biased_add",
    0x00000000u, 0x00000017u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, (uint32_t)"environment_texture_specular_mask_biased_multiply_biased_multiply",
    0x00000000u, 0x00000017u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, (uint32_t)"environment_texture_specular_mask_biased_multiply_multiply",
    0x00000000u, 0x00000017u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, (uint32_t)"environment_texture_specular_mask_biased_multiply_biased_add",
    0x00000000u, 0x00000017u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, (uint32_t)"environment_texture_specular_mask_multiply_biased_multiply",
    0x00000000u, 0x00000017u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, (uint32_t)"environment_texture_specular_mask_multiply_multiply",
    0x00000000u, 0x00000017u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, (uint32_t)"environment_texture_specular_mask_multiply_biased_add",
    0x00000000u, 0x00000017u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, (uint32_t)"environment_texture_specular_mask_biased_add_biased_multiply",
    0x00000000u, 0x00000017u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, (uint32_t)"environment_texture_specular_mask_biased_add_multiply",
    0x00000000u, 0x00000017u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, (uint32_t)"environment_texture_specular_mask_biased_add_biased_add",
    0x00000000u, 0x0000000eu, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, (uint32_t)"environment_reflection_bumped",
    0x00000000u, 0x0000000fu, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, (uint32_t)"environment_reflection_flat",
    0x00000000u, 0x0000000fu, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, (uint32_t)"environment_reflection_flat_specular",
    0x00000000u, 0x00000012u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, (uint32_t)"environment_reflection_radiosity",
    0x00000000u, 0x00000010u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, (uint32_t)"environment_reflection_lightmap_mask",
    0x00000000u, 0x00000011u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, (uint32_t)"environment_reflection_mirror_bumped",
    0x00000000u, 0x00000011u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, (uint32_t)"environment_reflection_mirror_flat",
    0x00000000u, 0x00000011u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, (uint32_t)"environment_reflection_mirror_flat_specular",
    0x00000000u, 0x00000014u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, (uint32_t)"environment_specular_light_flat",
    0x00000000u, 0x00000014u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, (uint32_t)"environment_specular_light_bumped",
    0x00000000u, 0x00000016u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, (uint32_t)"environment_specular_lightmap_flat",
    0x00000000u, 0x00000016u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, (uint32_t)"environment_specular_lightmap_bumped",
    0x00000000u, 0x0000003bu, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, (uint32_t)"transparent_plasma",
    0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, (uint32_t)"shadow_convolve",
    0x00000000u, 0x00000021u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, (uint32_t)"model_shadow",
    0x00000000u, 0x00000013u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, (uint32_t)"environment_shadow"
};
#pragma section(".geq$0069da10v", read, write)
__declspec(allocate(".geq$0069da10v")) __declspec(align(16)) uint32_t unknown_0069da10[536] = {
    0x00000000u, 0x00000018u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, (uint32_t)"widget_sprite",
    0x00000000u, 0x00000024u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, (uint32_t)"screen_normal",
    0x00000000u, 0x00000024u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, (uint32_t)"screen_multitexture_add_add",
    0x00000000u, 0x00000024u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, (uint32_t)"screen_multitexture_add_dot",
    0x00000000u, 0x00000024u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, (uint32_t)"screen_multitexture_add_multiply",
    0x00000000u, 0x00000024u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, (uint32_t)"screen_multitexture_add_multiply2x",
    0x00000000u, 0x00000024u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, (uint32_t)"screen_multitexture_add_subtract",
    0x00000000u, 0x00000024u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, (uint32_t)"screen_multitexture_dot_add",
    0x00000000u, 0x00000024u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, (uint32_t)"screen_multitexture_dot_dot",
    0x00000000u, 0x00000024u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, (uint32_t)"screen_multitexture_dot_multiply",
    0x00000000u, 0x00000024u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, (uint32_t)"screen_multitexture_dot_multiply2x",
    0x00000000u, 0x00000024u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, (uint32_t)"screen_multitexture_dot_subtract",
    0x00000000u, 0x00000024u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, (uint32_t)"screen_multitexture_multiply_add",
    0x00000000u, 0x00000024u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, (uint32_t)"screen_multitexture_multiply_dot",
    0x00000000u, 0x00000024u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, (uint32_t)"screen_multitexture_multiply_multiply",
    0x00000000u, 0x00000024u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, (uint32_t)"screen_multitexture_multiply_multiply2x",
    0x00000000u, 0x00000024u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, (uint32_t)"screen_multitexture_multiply_subtract",
    0x00000000u, 0x00000024u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, (uint32_t)"screen_multitexture_multiply2x_add",
    0x00000000u, 0x00000024u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, (uint32_t)"screen_multitexture_multiply2x_dot",
    0x00000000u, 0x00000024u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, (uint32_t)"screen_multitexture_multiply2x_multiply",
    0x00000000u, 0x00000024u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, (uint32_t)"screen_multitexture_multiply2x_multiply2x",
    0x00000000u, 0x00000024u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, (uint32_t)"screen_multitexture_multiply2x_subtract",
    0x00000000u, 0x00000024u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, (uint32_t)"screen_multitexture_subtract_add",
    0x00000000u, 0x00000024u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, (uint32_t)"screen_multitexture_subtract_dot",
    0x00000000u, 0x00000024u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, (uint32_t)"screen_multitexture_subtract_multiply",
    0x00000000u, 0x00000024u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, (uint32_t)"screen_multitexture_subtract_multiply2x",
    0x00000000u, 0x00000024u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, (uint32_t)"screen_multitexture_subtract_subtract",
    0x00000000u, 0x00000024u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, (uint32_t)"screen_meter",
    0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, (uint32_t)"sun_glow_convolve",
    0x00000000u, 0x00000018u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, (uint32_t)"sun_glow_draw",
    0x00000000u, 0x00000006u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, (uint32_t)"effect_multitexture_nonlinear_tint",
    0x00000000u, 0x00000006u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, (uint32_t)"effect_multitexture_nonlinear_tint_add",
    0x00000000u, 0x00000006u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, (uint32_t)"effect_multitexture_nonlinear_tint_alpha_blend",
    0x00000000u, 0x00000006u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, (uint32_t)"effect_multitexture_nonlinear_tint_double_multiply",
    0x00000000u, 0x00000006u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, (uint32_t)"effect_multitexture_nonlinear_tint_multiply",
    0x00000000u, 0x00000006u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, (uint32_t)"effect_multitexture_nonlinear_tint_multiply_add",
    0x00000000u, 0x00000006u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, (uint32_t)"effect_multitexture_normal_tint",
    0x00000000u, 0x00000006u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, (uint32_t)"effect_multitexture_normal_tint_add",
    0x00000000u, 0x00000006u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, (uint32_t)"effect_multitexture_normal_tint_alpha_blend",
    0x00000000u, 0x00000006u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, (uint32_t)"effect_multitexture_normal_tint_double_multiply",
    0x00000000u, 0x00000006u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, (uint32_t)"effect_multitexture_normal_tint_multiply",
    0x00000000u, 0x00000006u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, (uint32_t)"effect_multitexture_normal_tint_multiply_add",
    0x00000000u, 0x00000005u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, (uint32_t)"effect_nonlinear_tint",
    0x00000000u, 0x00000005u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, (uint32_t)"effect_nonlinear_tint_add",
    0x00000000u, 0x00000005u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, (uint32_t)"effect_nonlinear_tint_alpha_blend",
    0x00000000u, 0x00000005u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, (uint32_t)"effect_nonlinear_tint_double_multiply",
    0x00000000u, 0x00000005u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, (uint32_t)"effect_nonlinear_tint_multiply",
    0x00000000u, 0x00000005u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, (uint32_t)"effect_nonlinear_tint_multiply_add",
    0x00000000u, 0x00000005u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, (uint32_t)"effect_normal_tint",
    0x00000000u, 0x00000005u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, (uint32_t)"effect_normal_tint_add",
    0x00000000u, 0x00000005u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, (uint32_t)"effect_normal_tint_alpha_blend",
    0x00000000u, 0x00000005u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, (uint32_t)"effect_normal_tint_double_multiply",
    0x00000000u, 0x00000005u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, (uint32_t)"effect_normal_tint_multiply",
    0x00000000u, 0x00000005u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, (uint32_t)"effect_normal_tint_multiply_add",
    0x00000000u, 0x0000003cu, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, (uint32_t)"transparent_water_opacity",
    0x00000000u, 0x0000003eu, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, (uint32_t)"transparent_water_reflection",
    0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, (uint32_t)"transparent_water_bumpmap_convolution",
    0x00000000u, 0x0000001eu, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, (uint32_t)"active_camouflage_draw",
    0x00000000u, 0x0000000eu, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, (uint32_t)"transparent_glass_reflection_bumped",
    0x00000000u, 0x0000000fu, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, (uint32_t)"transparent_glass_reflection_flat",
    0x00000000u, 0x00000011u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, (uint32_t)"transparent_glass_reflection_mirror",
    0x00000000u, 0x00000030u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, (uint32_t)"transparent_glass_diffuse",
    0x00000000u, 0x00000017u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, (uint32_t)"transparent_generic",
    0x00000000u, 0x00000039u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, (uint32_t)"transparent_meter",
    0x00000000u, 0x0000000bu, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, (uint32_t)"environment_fog",
    0x00000000u, 0xffffffffu, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, (uint32_t)"screen_fog",
    0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, (uint32_t)"screen_effect"
};
#pragma section(".geq$0069e270v", read, write)
__declspec(allocate(".geq$0069e270v")) __declspec(align(16)) uint32_t rasterizer_screen_flash_effect[8] = {
    0x00000000u, 0xffffffffu, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, (uint32_t)"screen_flash"
};
#pragma section(".geq$0069e290v", read, write)
__declspec(allocate(".geq$0069e290v")) __declspec(align(16)) uint32_t environment_effect_slot[48] = {
    0x00000000u, 0xffffffffu, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, (uint32_t)"model_environment",
    0x00000000u, 0xffffffffu, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, (uint32_t)"model_mask_self_illumination",
    0x00000000u, 0xffffffffu, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, (uint32_t)"model_mask_change_color",
    0x00000000u, 0xffffffffu, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, (uint32_t)"model_mask_multipurpose",
    0x00000000u, 0xffffffffu, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, (uint32_t)"model_mask_reflection",
    0x00000000u, 0xffffffffu, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, (uint32_t)"model_mask_none"
};
#pragma section(".geq$0069e350v", read, write)
__declspec(allocate(".geq$0069e350v")) __declspec(align(16)) uint32_t rasterizer_vertex_shaders[68] = {
    0x00000000u, (uint32_t)"convolution", 0x00000000u, (uint32_t)"debug", 0x00000000u, (uint32_t)"decal", 0x00000000u, (uint32_t)"detail_object_type0",
    0x00000000u, (uint32_t)"detail_object_type1", 0x00000000u, (uint32_t)"effect", 0x00000000u, (uint32_t)"effect_multitexture", 0x00000000u, (uint32_t)"effect_multitexture_screenspace",
    0x00000000u, (uint32_t)"effect_zsprite", 0x00000000u, (uint32_t)"environment_diffuse_light", 0x00000000u, (uint32_t)"environment_diffuse_light_ff", 0x00000000u, (uint32_t)"environment_fog",
    0x00000000u, (uint32_t)"environment_fog_screen", 0x00000000u, (uint32_t)"environment_lightmap", 0x00000000u, (uint32_t)"environment_reflection_bumped", 0x00000000u, (uint32_t)"environment_reflection_flat",
    0x00000000u, (uint32_t)"environment_reflection_lightmap_mask", 0x00000000u, (uint32_t)"environment_reflection_mirror", 0x00000000u, (uint32_t)"environment_reflection_radiosity", 0x00000000u, (uint32_t)"environment_shadow",
    0x00000000u, (uint32_t)"environment_specular_light", 0x00000000u, (uint32_t)"environment_specular_spot_light", 0x00000000u, (uint32_t)"environment_specular_lightmap", 0x00000000u, (uint32_t)"environment_texture",
    0x00000000u, (uint32_t)"lens_flare", 0x00000000u, (uint32_t)"model_fogged", 0x00000000u, (uint32_t)"model", 0x00000000u, (uint32_t)"model_ff",
    0x00000000u, (uint32_t)"model_fast", 0x00000000u, (uint32_t)"model_scenery", 0x00000000u, (uint32_t)"model_active_camouflage", 0x00000000u, (uint32_t)"model_active_camouflage_ff",
    0x00000000u, (uint32_t)"model_fog_screen", 0x00000000u, (uint32_t)"model_shadow"
};
#pragma section(".geq$0069e460v", read, write)
__declspec(allocate(".geq$0069e460v")) __declspec(align(16)) uint32_t rasterizer_depth_prepass_vertex_shader[2] = {0x00000000u, (uint32_t)"model_zbuffer"};
#pragma section(".geq$0069e468v", read, write)
__declspec(allocate(".geq$0069e468v")) __declspec(align(8)) uint32_t renderer_unknown_69e468[58] = {
    0x00000000u, (uint32_t)"screen", 0x00000000u, (uint32_t)"screen2", 0x00000000u, (uint32_t)"transparent_generic", 0x00000000u, (uint32_t)"transparent_generic_lit_m",
    0x00000000u, (uint32_t)"transparent_generic_m", 0x00000000u, (uint32_t)"transparent_generic_object_centered", 0x00000000u, (uint32_t)"transparent_generic_object_centered_m", 0x00000000u, (uint32_t)"transparent_generic_reflection",
    0x00000000u, (uint32_t)"transparent_generic_reflection_m", 0x00000000u, (uint32_t)"transparent_generic_screenspace", 0x00000000u, (uint32_t)"transparent_generic_screenspace_m", 0x00000000u, (uint32_t)"transparent_generic_viewer_centered",
    0x00000000u, (uint32_t)"transparent_generic_viewer_centered_m", 0x00000000u, (uint32_t)"transparent_glass_diffuse_light", 0x00000000u, (uint32_t)"transparent_glass_diffuse_light_m", 0x00000000u, (uint32_t)"transparent_glass_reflection_bumped",
    0x00000000u, (uint32_t)"transparent_glass_reflection_bumped_m", 0x00000000u, (uint32_t)"transparent_glass_reflection_flat", 0x00000000u, (uint32_t)"transparent_glass_reflection_flat_m", 0x00000000u, (uint32_t)"transparent_glass_reflection_mirror",
    0x00000000u, (uint32_t)"transparent_glass_tint", 0x00000000u, (uint32_t)"transparent_glass_tint_m", 0x00000000u, (uint32_t)"transparent_meter", 0x00000000u, (uint32_t)"transparent_meter_m",
    0x00000000u, (uint32_t)"transparent_plasma_m", 0x00000000u, (uint32_t)"transparent_water_opacity", 0x00000000u, (uint32_t)"transparent_water_opacity_m", 0x00000000u, (uint32_t)"transparent_water_reflection",
    0x00000000u, (uint32_t)"transparent_water_reflection_m"
};

/** 0x0069fde4..0x0069fdfc: unit_base_animation_state_names, s_stand */
#pragma section(".geq$0069fde4", read, write)
__declspec(allocate(".geq$0069fde4")) __declspec(align(16)) uint8_t eq_pad_0069fde4[4] = {0};
#pragma section(".geq$0069fde4v", read, write)
__declspec(allocate(".geq$0069fde4v")) __declspec(align(4)) uint32_t unit_base_animation_state_names[2] = {(uint32_t)"asleep", (uint32_t)"alert"};
#pragma section(".geq$0069fdecv", read, write)
__declspec(allocate(".geq$0069fdecv")) __declspec(align(4)) uint32_t s_stand[4] = {(uint32_t)"stand", (uint32_t)"crouch", (uint32_t)"flee", (uint32_t)"flaming"};

}
