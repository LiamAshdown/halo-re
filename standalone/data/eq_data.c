/**
 * standalone/data/eq_data.c -- the engine globals of the initialised .rdata/.data image that the C code reaches
 * through several names or as one larger object (tables strided past their declared end, a struct seen through field
 * names). They were absolute EQU symbols in standalone/globals.asm.
 *
 * Each object is in its own section ".geq$<original address>v"; the linker sorts the group by name, so the objects
 * keep the original address order. A cluster of overlapping or adjacent globals is one gap-free run: every object
 * spans up to the next one (or the cluster end), and a 16-aligned pad ".geq$<cluster start>" in front keeps the run
 * congruent with the original addresses, so every name sits at its original offset from the cluster start. Initial
 * values are the bytes the data image holds at the original address: code pointers name the function, pointers into a
 * global that is now a C definition point at that definition. tools/globals_check_eq.py verifies layout and bytes.
 */
#include <stdint.h>

/** code the tables point at (defined in src/ or bound by image_bindings.c) */
extern void actor_compute_swarm_avoidance_offset();
extern void actor_type_crew_update();
extern void actor_type_elite_update();
extern void actor_type_engineer_update();
extern void actor_type_flood_carrier_update();
extern void actor_type_flood_update();
extern void actor_type_grunt_update();
extern void actor_type_hunter_update();
extern void actor_type_infection_swarm_update();
extern void actor_type_infection_update();
extern void actor_type_jackal_update();
extern void actor_type_marine_update();
extern void actor_type_mounted_weapon_update();
extern void actor_type_sentinel_update();
extern void exception_copy_construct();
extern void function_do_nothing();
extern void hwreq_length_error_destruct();
extern void hwreq_out_of_range_destruct();
extern void message_delta_blob_compute_size();
extern void message_delta_compound_compute_size();
extern void message_delta_compound_initialize();
extern void message_delta_compute_size_1();
extern void message_delta_compute_size_16();
extern void message_delta_compute_size_2();
extern void message_delta_compute_size_3();
extern void message_delta_compute_size_32();
extern void message_delta_compute_size_4();
extern void message_delta_compute_size_6();
extern void message_delta_compute_size_8();
extern void message_delta_count_initialize();
extern void message_delta_enum_width_compute_size();
extern void message_delta_first_dword_compute_size();
extern void message_delta_flags_initialize();
extern void message_delta_index_compute_size();
extern void message_delta_index_initialize();
extern void message_delta_index_teardown();
extern void message_delta_integer_compute_size();
extern void message_delta_integer_initialize();
extern void message_delta_item_placement_compute_size();
extern void message_delta_item_placement_initialize();
extern void message_delta_locality_compute_size();
extern void message_delta_locality_initialize();
extern void message_delta_normal_compute_size();
extern void message_delta_normal_initialize();
extern void message_delta_pointer_compute_size();
extern void message_delta_pointer_initialize();
extern void message_delta_quantized_real_initialize();
extern void message_delta_range_compute_size();
extern void message_delta_range_initialize();
extern void message_delta_scalar_array_compute_size();
extern void message_delta_string_compute_size();
extern void message_delta_structure_array_compute_size();
extern void message_delta_structure_array_initialize();
extern void message_delta_velocity_compute_size();
extern void message_delta_wide_string_compute_size();
extern void object_type_definition_return_true();
extern void __fastcall std_length_error_copy_construct(void *, void *, void *);
extern void waypoint_table_quantize_initialize();

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
    0x00000004u, 0x00687354u, 0x0000000au, 0x00687364u, 0x0000000cu, 0x00685458u, 0x00000010u, 0x006853f8u,
    0x00000005u, 0x00688a60u
};

/** 0x0065d440..0x0065d520: message_delta_definitions */
#pragma section(".geq$0065d440v", read, write)
__declspec(allocate(".geq$0065d440v")) __declspec(align(16)) uint32_t message_delta_definitions[56] = {
    0x0069b018u, 0x006960e8u, 0x00695db0u, 0x006964c8u, 0x0069ade0u, 0x0069aa00u, 0x00692f58u, 0x006887c8u,
    0x00688840u, 0x0069abc0u, 0x00688938u, 0x006872b8u, 0x0069ac18u, 0x00698318u, 0x006889f8u, 0x00692e70u,
    0x00688670u, 0x00687e70u, 0x00688490u, 0x00687ff0u, 0x00688570u, 0x00687b48u, 0x00687c90u, 0x00687cd8u,
    0x00687a88u, (uint32_t)((uint8_t *)&multiplayer_sound_enabled + 0x30), 0x00688750u, 0x0069ad10u, 0x0069aa98u,
    0x0069aed0u, 0x00696000u, 0x00695cb8u, 0x00696390u, 0x00698718u, 0x0069b1a8u, 0x00699948u, 0x006999b0u,
    0x00699ab0u, 0x00699b58u, 0x00699c48u, 0x00699c90u, 0x00699d80u, 0x00699e38u, 0x00696228u, 0x006961c0u,
    0x006962a0u, 0x00696318u, 0x00687a20u, 0x00695f28u, 0x0069b060u, 0x0069b0c8u, 0x00695f98u, 0x00697e90u,
    (uint32_t)((uint8_t *)&network_game_messages_group + 0x18), 0x006996e8u, 0x006997b0u
};

/** 0x00672f20..0x00672f24: unknown_00672f20 */
#pragma section(".geq$00672f20v", read, write)
__declspec(allocate(".geq$00672f20v")) __declspec(align(16)) uint32_t unknown_00672f20[1] = {0x3f7d70a4u};

/** 0x00673524..0x0067359c: length_error_throw_info, out_of_range_throw_info */
#pragma section(".geq$00673524", read, write)
__declspec(allocate(".geq$00673524")) __declspec(align(16)) uint8_t eq_pad_00673524[4] = {0};
#pragma section(".geq$00673524v", read, write)
__declspec(allocate(".geq$00673524v")) __declspec(align(4)) uint32_t length_error_throw_info[15] = {
    0x00000000u, (uint32_t)hwreq_length_error_destruct, 0x00000000u,
    (uint32_t)((uint8_t *)&length_error_throw_info + 0x10), 0x00000003u,
    (uint32_t)((uint8_t *)&length_error_throw_info + 0x20), 0x0067359cu,
    (uint32_t)((uint8_t *)&out_of_range_throw_info + 0x20), 0x00000000u, 0x0069ff2cu, 0x00000000u, 0xffffffffu,
    0x00000000u, 0x00000028u, (uint32_t)std_length_error_copy_construct
};
#pragma section(".geq$00673560v", read, write)
__declspec(allocate(".geq$00673560v")) __declspec(align(16)) uint32_t out_of_range_throw_info[15] = {
    0x00000000u, (uint32_t)hwreq_out_of_range_destruct, 0x00000000u,
    (uint32_t)((uint8_t *)&out_of_range_throw_info + 0x10), 0x00000003u, 0x006735b8u, 0x0067359cu,
    (uint32_t)((uint8_t *)&out_of_range_throw_info + 0x20), 0x00000000u, 0x0069ff4cu, 0x00000000u, 0xffffffffu,
    0x00000000u, 0x0000000cu, (uint32_t)exception_copy_construct
};

/** 0x006851f4..0x006853f8: hud_text_message_hold_color, global_white_argb, hud_text_message_normal_color and 6 more */
#pragma section(".geq$006851f4", read, write)
__declspec(allocate(".geq$006851f4")) __declspec(align(16)) uint8_t eq_pad_006851f4[4] = {0};
#pragma section(".geq$006851f4v", read, write)
__declspec(allocate(".geq$006851f4v")) __declspec(align(4)) uint32_t hud_text_message_hold_color[2] = {0x006551d8u, 0x00655198u};
#pragma section(".geq$006851fcv", read, write)
__declspec(allocate(".geq$006851fcv")) __declspec(align(4)) uint32_t global_white_argb[1] = {0x00655138u};
#pragma section(".geq$00685200v", read, write)
__declspec(allocate(".geq$00685200v")) __declspec(align(16)) uint32_t hud_text_message_normal_color[1] = {0x006551a8u};
#pragma section(".geq$00685204v", read, write)
__declspec(allocate(".geq$00685204v")) __declspec(align(4)) uint32_t actor_mode_uncover_look_weights_active[1] = {0x00655188u};
#pragma section(".geq$00685208v", read, write)
__declspec(allocate(".geq$00685208v")) __declspec(align(8)) uint32_t actor_mode_guard_look_weights_idle[3] = {0x00655228u, 0x00655208u, 0x006551b8u};
#pragma section(".geq$00685214v", read, write)
__declspec(allocate(".geq$00685214v")) __declspec(align(4)) uint32_t console_color_00685214[1] = {0x00655218u};
#pragma section(".geq$00685218v", read, write)
__declspec(allocate(".geq$00685218v")) __declspec(align(8)) uint32_t console_message_default_color[1] = {0x00655168u};
#pragma section(".geq$0068521cv", read, write)
__declspec(allocate(".geq$0068521cv")) __declspec(align(4)) uint32_t actor_mode_guard_look_weights_ambush[103] = {
    0x006551f8u, 0x006551e8u, 0x00655148u, 0x0065ed78u, 0x0065eddcu, 0x0065ed70u, 0x00000000u, 0x0065ee10u,
    0x00000010u, 0x00020002u, 0x00000001u, 0x00000000u, (uint32_t)actor_type_flood_carrier_update, 0x00000000u,
    0x00000000u, 0x0065ee20u, 0x00000002u, 0x00000000u, 0x00000000u, 0x00000000u, (uint32_t)actor_type_crew_update,
    0x00000000u, 0x00000000u, 0x0065ee28u, 0x00000004u, 0x00010001u, 0x00000001u, 0x00000000u,
    (uint32_t)actor_type_elite_update, 0x00000000u, 0x00000000u, 0x0065ee30u, 0x00000004u, 0x00000000u, 0x00000000u,
    0x00000000u, (uint32_t)actor_type_engineer_update, 0x00000000u, 0x00000000u, 0x0065ee3cu, 0x00000008u, 0x00000000u,
    0x00000001u, 0x00000000u, (uint32_t)actor_type_flood_update, 0x00000000u, 0x00000000u, 0x0065ee44u, 0x00000004u,
    0x00000000u, 0x00000000u, 0x00000000u, (uint32_t)actor_type_grunt_update, 0x00000000u, 0x00000000u, 0x0065ee4cu,
    0x00000004u, 0x00010001u, 0x00000001u, 0x00000000u, (uint32_t)actor_type_hunter_update, 0x00000000u, 0x00000000u,
    0x0065ee54u, 0x00020020u, 0x00020002u, 0x00000100u, 0x00000000u, (uint32_t)actor_type_infection_update,
    (uint32_t)actor_type_infection_swarm_update, (uint32_t)actor_compute_swarm_avoidance_offset, 0x0065ee60u,
    0x00020004u, 0x00020000u, 0x00000000u, 0x00000000u, (uint32_t)actor_type_jackal_update, 0x00000000u, 0x00000000u,
    0x0065ee68u, 0x00000002u, 0x00000000u, 0x00000000u, 0x00000000u, (uint32_t)actor_type_marine_update, 0x00000000u,
    0x00000000u, 0x0065ee70u, 0x00020000u, 0x00020002u, 0x00000000u, 0x00000000u,
    (uint32_t)actor_type_mounted_weapon_update, 0x00000000u, 0x00000000u, 0x0065ee80u, 0x00000040u, 0x00000000u,
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
__declspec(allocate(".geq$006869c0v")) __declspec(align(16)) uint32_t map_download[1] = {0x006a8960u};

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
__declspec(allocate(".geq$00686af8v")) __declspec(align(8)) uint32_t actor_mode_default_look_weights[1] = {0x00655178u};
#pragma section(".geq$00686afcv", read, write)
__declspec(allocate(".geq$00686afcv")) __declspec(align(4)) uint32_t actor_mode_guard_look_weights_a5[1] = {0x006551c8u};
#pragma section(".geq$00686b00v", read, write)
__declspec(allocate(".geq$00686b00v")) __declspec(align(16)) uint32_t actor_mode_guard_look_weights_a6[1] = {0x00655238u};
#pragma section(".geq$00686b04v", read, write)
__declspec(allocate(".geq$00686b04v")) __declspec(align(4)) uint32_t global_white_color[1] = {0x0065513cu};
#pragma section(".geq$00686b08v", read, write)
__declspec(allocate(".geq$00686b08v")) __declspec(align(8)) uint32_t object_ambient_lightmap_default[1] = {0x0065514cu};
#pragma section(".geq$00686b0cv", read, write)
__declspec(allocate(".geq$00686b0cv")) __declspec(align(4)) uint32_t default_axis_b[1] = {0x0065515cu};
#pragma section(".geq$00686b10v", read, write)
__declspec(allocate(".geq$00686b10v")) __declspec(align(16)) uint32_t default_color_a[1] = {0x0065516cu};
#pragma section(".geq$00686b14v", read, write)
__declspec(allocate(".geq$00686b14v")) __declspec(align(4)) uint32_t global_real_rgb_green_pointer[1] = {0x0065517cu};
#pragma section(".geq$00686b18v", read, write)
__declspec(allocate(".geq$00686b18v")) __declspec(align(8)) uint32_t default_color_b[1] = {0x0065518cu};

/** 0x00686d88..0x00686d98: unit_control_data_version_layouts */
#pragma section(".geq$00686d88", read, write)
__declspec(allocate(".geq$00686d88")) __declspec(align(16)) uint8_t eq_pad_00686d88[8] = {0};
#pragma section(".geq$00686d88v", read, write)
__declspec(allocate(".geq$00686d88v")) __declspec(align(8)) uint32_t unit_control_data_version_layouts[4] = {0x00686cc8u, 0x00686d40u, 0x00686d58u, 0x00686d70u};

/** 0x00686fe8..0x00686ff8: recorded_animation_codecs_by_version */
#pragma section(".geq$00686fe8", read, write)
__declspec(allocate(".geq$00686fe8")) __declspec(align(16)) uint8_t eq_pad_00686fe8[8] = {0};
#pragma section(".geq$00686fe8v", read, write)
__declspec(allocate(".geq$00686fe8v")) __declspec(align(8)) uint32_t recorded_animation_codecs_by_version[4] = {0x00686fe0u, 0x00686fe0u, 0x00686fe0u, 0x00686fd8u};

/** 0x00687130..0x00687134: object_network_id_table */
#pragma section(".geq$00687130v", read, write)
__declspec(allocate(".geq$00687130v")) __declspec(align(16)) uint32_t object_network_id_table[1] = {0x006870a8u};

/** 0x00687558..0x0068755c: machine_table */
#pragma section(".geq$00687558", read, write)
__declspec(allocate(".geq$00687558")) __declspec(align(16)) uint8_t eq_pad_00687558[8] = {0};
#pragma section(".geq$00687558v", read, write)
__declspec(allocate(".geq$00687558v")) __declspec(align(8)) uint32_t machine_table[1] = {0x006874d0u};

/** 0x00687af0..0x00687af2: teleport_flash_type */
#pragma section(".geq$00687af0v", read, write)
__declspec(allocate(".geq$00687af0v")) __declspec(align(16)) uint8_t teleport_flash_type[2] = {0x06, 0x00};

/** 0x00688308..0x00688324: game_engine_definitions */
#pragma section(".geq$00688308", read, write)
__declspec(allocate(".geq$00688308")) __declspec(align(16)) uint8_t eq_pad_00688308[8] = {0};
#pragma section(".geq$00688308v", read, write)
__declspec(allocate(".geq$00688308v")) __declspec(align(8)) uint32_t game_engine_definitions[7] = {
    0x00000000u, 0x00687d20u, 0x006880f8u, 0x00688258u, 0x00687ec8u, 0x006881a8u, 0x00688048u
};

/** 0x00688b58..0x00689380: hs_function_definitions */
#pragma section(".geq$00688b58", read, write)
__declspec(allocate(".geq$00688b58")) __declspec(align(16)) uint8_t eq_pad_00688b58[8] = {0};
#pragma section(".geq$00688b58v", read, write)
__declspec(allocate(".geq$00688b58v")) __declspec(align(8)) uint32_t hs_function_definitions[522] = {
    0x00657660u, 0x0065767cu, 0x00657698u, 0x006576b4u, 0x006576d0u, 0x006576ecu, 0x00657708u, 0x00657724u,
    0x00657740u, 0x0065775cu, 0x00657778u, 0x00657794u, 0x006577b0u, 0x006577ccu, 0x006577e8u, 0x00657804u,
    0x00657820u, 0x0065783cu, 0x00657858u, 0x00657874u, 0x00657890u, 0x006578acu, 0x006578c8u, 0x006578e4u,
    0x00657900u, 0x0065791cu, 0x00657938u, 0x00657954u, 0x00657974u, 0x00657994u, 0x006579b0u, 0x006579d0u,
    0x006579f0u, 0x00657a10u, 0x00657b0cu, 0x00657b2cu, 0x00657b4cu, 0x00657b6cu, 0x00657a30u, 0x00657a50u,
    0x00657a70u, 0x00657a90u, 0x00657ab0u, 0x00657ad0u, 0x00657af0u, 0x00657b90u, 0x00657bb0u, 0x00657bd0u,
    0x00657bf0u, 0x00657c14u, 0x00657c34u, 0x00657c54u, 0x00657c78u, 0x00657c9cu, 0x00657cbcu, 0x00657cdcu,
    0x00657cfcu, 0x00657d18u, 0x0065b204u, 0x00657d34u, 0x00657d54u, 0x00657d74u, 0x00657d94u, 0x00657db4u,
    0x00657dd0u, 0x00657decu, 0x00657e0cu, 0x00657e2cu, 0x00657e4cu, 0x00657e6cu, 0x00657e8cu, 0x00657eacu,
    0x00657eccu, 0x00657eecu, 0x00657f08u, 0x00657f28u, 0x00657f4cu, 0x00657f70u, 0x00657f90u, 0x00657facu,
    0x00657fccu, 0x00657fecu, 0x0065800cu, 0x0065802cu, 0x006580a8u, 0x0065804cu, 0x0065806cu, 0x0065808cu,
    0x006580c8u, 0x006580e8u, 0x00658108u, 0x0065812cu, 0x00658150u, 0x00658170u, 0x00658190u, 0x006581b0u,
    0x006581d0u, 0x006581f0u, 0x00658210u, 0x00658230u, 0x00658298u, 0x00658250u, 0x00658274u, 0x006582c0u,
    0x006582e0u, 0x00658300u, 0x00658320u, 0x00658340u, 0x00658364u, 0x00658388u, 0x006583acu, 0x006583ccu,
    0x006583ecu, 0x00658410u, 0x00658434u, 0x00658458u, 0x0065847cu, 0x006584a0u, 0x006584c0u, 0x006584e0u,
    0x00658500u, 0x0065851cu, 0x0065853cu, 0x0065855cu, 0x0065857cu, 0x0065859cu, 0x006585bcu, 0x006585dcu,
    0x006585fcu, 0x0065861cu, 0x0065863cu, 0x0065865cu, 0x0065867cu, 0x00658698u, 0x006586b8u, 0x006586d8u,
    0x006586f8u, 0x00658738u, 0x00658718u, 0x00658758u, 0x00658778u, 0x00658798u, 0x006587b8u, 0x006587d8u,
    0x006587f8u, 0x00658818u, 0x00658838u, 0x00658858u, 0x00658878u, 0x00658894u, 0x0065b260u, 0x006588b0u,
    0x0065b27cu, 0x006588ccu, 0x006588e8u, 0x00658904u, 0x00658924u, 0x00658980u, 0x006589a0u, 0x006589c0u,
    0x00658a00u, 0x00658a20u, 0x00658a60u, 0x00658a80u, 0x00658aa0u, 0x00658ac0u, 0x00658ae0u, 0x00658afcu,
    0x00658b1cu, 0x00658b38u, 0x00658b58u, 0x00658b78u, 0x00658b98u, 0x00658bb8u, 0x00658bd8u, 0x00658bf8u,
    0x00658c38u, 0x00658c58u, 0x00658c78u, 0x00658c98u, 0x00658cb8u, 0x00658cd8u, 0x00658cf8u, 0x00658d18u,
    0x00658d38u, 0x00658d5cu, 0x00658d7cu, 0x00658d9cu, 0x006593a0u, 0x006593c0u, 0x006593e0u, 0x00659400u,
    0x00659420u, 0x00659440u, 0x00658dbcu, 0x00658de0u, 0x00659380u, 0x00658e04u, 0x00658e24u, 0x00658e44u,
    0x00658e64u, 0x00658e84u, 0x00658ea4u, 0x00658ec4u, 0x00658ee4u, 0x00658f04u, 0x00658f24u, 0x00658f44u,
    0x00658f64u, 0x00658f84u, 0x00658fa4u, 0x00658fc4u, 0x00659360u, 0x00659340u, 0x00658fe4u, 0x00659004u,
    0x00659024u, 0x00659044u, 0x00659064u, 0x00659460u, 0x00659084u, 0x006590a0u, 0x006590c0u, 0x006590e0u,
    0x00659100u, 0x00659120u, 0x00659140u, 0x00659160u, 0x00659180u, 0x006591a0u, 0x006591c0u, 0x006591e0u,
    0x00659200u, 0x00659220u, 0x00659240u, 0x00659480u, 0x00659260u, 0x00659280u, 0x006594a0u, 0x006594c0u,
    0x006592a0u, 0x006592c0u, 0x006592e0u, 0x00659300u, 0x00659320u, 0x006594e0u, 0x00659500u, 0x00659520u,
    0x00659540u, 0x00659564u, 0x00659584u, 0x006595a4u, 0x006595c4u, 0x006595fcu, 0x006595e0u, 0x0065b2d0u,
    0x00659638u, 0x00659618u, 0x00659654u, 0x00659670u, 0x00659894u, 0x006598b0u, 0x006598ccu, 0x0065b2f0u,
    0x006598e8u, 0x00659908u, 0x00659928u, 0x00659948u, 0x00659968u, 0x00659984u, 0x006599a0u, 0x0065b224u,
    0x006599bcu, 0x006599d8u, 0x006599f4u, 0x00659a10u, 0x00659a2cu, 0x00659a48u, 0x00659a68u, 0x00659a84u,
    0x00659aa0u, 0x00659ac0u, 0x00659ae0u, 0x00659b00u, 0x0065b240u, 0x00658940u, 0x00658960u, 0x00659b20u,
    0x00659b3cu, 0x00659b58u, 0x00659b78u, 0x00659b98u, 0x00659bb8u, 0x00659bd8u, 0x00659bfcu, 0x00659c20u,
    0x00659c3cu, 0x00659c58u, 0x00659c74u, 0x00659c90u, 0x00659cacu, 0x00659cccu, 0x00659cecu, 0x00659d0cu,
    0x00659d2cu, 0x00659d48u, 0x00659d64u, 0x00659d80u, 0x00659d9cu, 0x00659db8u, 0x00659dd4u, 0x00659df0u,
    0x00659e0cu, 0x00659e28u, 0x00659e44u, 0x00659e60u, 0x00659e7cu, 0x00659f34u, 0x0065b2b4u, 0x00659ef4u,
    0x0065b298u, 0x00659e98u, 0x00659eb4u, 0x00659ed4u, 0x00659f14u, 0x00659f50u, 0x00659f70u, 0x00659f94u,
    0x00659fb4u, 0x00659fd4u, 0x00659ff4u, 0x0065a018u, 0x0065a038u, 0x0065a058u, 0x0065a078u, 0x0065a098u,
    0x0065a0bcu, 0x0065a100u, 0x0065a120u, 0x0065a140u, 0x0065a15cu, 0x0065a17cu, 0x0065a198u, 0x0065a1b8u,
    0x0065a0dcu, 0x0065a1d4u, 0x0065968cu, 0x006596a8u, 0x006596c8u, 0x006596e8u, 0x00659704u, 0x00659720u,
    0x0065973cu, 0x00659758u, 0x00659774u, 0x00659790u, 0x006597acu, 0x006597c8u, 0x006597e4u, 0x00659800u,
    0x0065981cu, 0x00659838u, 0x00659854u, 0x00659870u, 0x0065a1f4u, 0x0065a214u, 0x0065a234u, 0x0065a254u,
    0x0065a270u, 0x0065a294u, 0x0065a2b8u, 0x0065a2dcu, 0x0065a300u, 0x0065a320u, 0x0065a340u, 0x0065a360u,
    0x0065b18cu, 0x0065b1a8u, 0x0065b1c8u, 0x0065a380u, 0x0065a3a8u, 0x0065a3c4u, 0x0065a3e0u, 0x0065a3fcu,
    0x0065a418u, 0x0065a438u, 0x0065a458u, 0x0065a474u, 0x0065a490u, 0x0065a4b0u, 0x0065a4d0u, 0x0065a4ecu,
    0x0065a510u, 0x0065a534u, 0x0065a554u, 0x0065a574u, 0x0065a594u, 0x0065a5b4u, 0x0065a5d4u, 0x0065a5f4u,
    0x0065a614u, 0x0065a634u, 0x0065a654u, 0x0065a674u, 0x0065a690u, 0x0065a6b0u, 0x0065a6d0u, 0x0065a6f0u,
    0x0065a710u, 0x0065a734u, 0x0065a754u, 0x0065a774u, 0x0065a790u, 0x0065a7b0u, 0x0065a7d0u, 0x0065a7ecu,
    0x0065a80cu, 0x0065a82cu, 0x0065a848u, 0x0065a864u, 0x0065a888u, 0x0065a8a4u, 0x0065a8c4u, 0x0065a8e4u,
    0x0065a90cu, 0x0065a934u, 0x0065a958u, 0x0065a978u, 0x0065a994u, 0x0065a9b4u, 0x0065a9d0u, 0x0065a9f4u,
    0x0065aa10u, 0x0065aa30u, 0x0065aa4cu, 0x0065aa68u, 0x0065aa88u, 0x0065aaa8u, 0x0065aac8u, 0x0065aae4u,
    0x0065ab04u, 0x0065ab24u, 0x0065ab44u, 0x0065ab60u, 0x0065ab80u, 0x0065aba0u, 0x0065abbcu, 0x0065abdcu,
    0x0065abfcu, 0x0065ac1cu, 0x0065ac3cu, 0x0065ac58u, 0x0065ac78u, 0x0065ac98u, 0x0065acb8u, 0x0065acd8u,
    0x0065acf8u, 0x0065ad18u, 0x0065ad38u, 0x0065ad58u, 0x0065ad78u, 0x0065ad98u, 0x0065adb8u, 0x0065add8u,
    0x0065adf8u, 0x0065ae18u, 0x0065ae38u, 0x0065ae58u, 0x0065ae78u, 0x0065ae98u, 0x0065aeb8u, 0x0065aed8u,
    0x0065aef8u, 0x0065af18u, 0x0065af38u, 0x0065af58u, 0x0065af78u, 0x0065af98u, 0x0065afb8u, 0x0065afd8u,
    0x0065aff8u, 0x0065b018u, 0x0065b03cu, 0x0065b05cu, 0x0065b3f8u, 0x0065b078u, 0x0065b514u, 0x0065b4f8u,
    0x0065b530u, 0x0065b550u, 0x0065b368u, 0x0065b384u, 0x0065b3a0u, 0x0065b310u, 0x0065b3c0u, 0x0065b414u,
    0x0065b430u, 0x0065b46cu, 0x0065b32cu, 0x0065b348u, 0x0065b098u, 0x0065b0b4u, 0x0065b450u, 0x0065b570u,
    0x0065b58cu, 0x0065b1e4u, 0x0065b0d0u, 0x0065b0f0u, 0x0065b110u, 0x0065b130u, 0x0065b150u, 0x0065b16cu,
    0x0065b5e0u, 0x0065b5fcu, 0x0065b5a8u, 0x0065b5c4u, 0x0065b488u, 0x0065b4a4u, 0x0065b4c0u, 0x0065b4dcu,
    0x0065b3dcu, 0x0065b618u
};

/** 0x0068e588..0x0068e66c: multiplayer_maps, map_per_map_table */
#pragma section(".geq$0068e588", read, write)
__declspec(allocate(".geq$0068e588")) __declspec(align(16)) uint8_t eq_pad_0068e588[8] = {0};
#pragma section(".geq$0068e588v", read, write)
__declspec(allocate(".geq$0068e588v")) __declspec(align(8)) uint32_t multiplayer_maps[2] = {0x00000000u, 0x00669a24u};
#pragma section(".geq$0068e590v", read, write)
__declspec(allocate(".geq$0068e590v")) __declspec(align(16)) uint32_t map_per_map_table[55] = {
    0x00000001u, 0x00000001u, 0x00669a18u, 0x00000001u, 0x00000002u, 0x00669a0cu, 0x00000001u, 0x00000003u,
    0x00669a04u, 0x00000001u, 0x00000004u, 0x006699f8u, 0x00000001u, 0x00000005u, 0x006699ecu, 0x00000001u,
    0x00000006u, 0x006699e0u, 0x00000001u, 0x00000007u, 0x006699d4u, 0x00000001u, 0x00000008u, 0x006699c4u,
    0x00000001u, 0x00000009u, 0x006699b8u, 0x00000001u, 0x0000000au, 0x006699b0u, 0x00000001u, 0x0000000bu,
    0x006699a8u, 0x00000001u, 0x0000000cu, 0x006699a0u, 0x00000001u, 0x0000000du, 0x00669994u, 0x00000000u,
    0x0000000eu, 0x00669988u, 0x00000000u, 0x0000000fu, 0x00669978u, 0x00000000u, 0x00000010u, 0x0066996cu,
    0x00000000u, 0x00000011u, 0x00669960u, 0x00000000u, 0x00000012u, 0x00669950u, 0x00000000u
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
    0x0066c488u, 0x00080027u, 0x00000600u, 0x00000800u, 0x006993c0u, 0x00000000u, 0x00000035u, 0xffffffffu,
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
    (uint32_t)function_do_nothing
};
#pragma section(".geq$0069a304v", read, write)
__declspec(allocate(".geq$0069a304v")) __declspec(align(4)) uint32_t message_delta_unknown_table_0069a304[168] = {
    0x00000000u, 0x00000001u, 0x00000000u, (uint32_t)message_delta_compute_size_32,
    (uint32_t)object_type_definition_return_true, (uint32_t)function_do_nothing, 0x00000000u, 0x00000002u, 0x00000000u,
    (uint32_t)message_delta_compute_size_1, (uint32_t)object_type_definition_return_true,
    (uint32_t)function_do_nothing, 0x00000000u, 0x00000003u, 0x00000000u, (uint32_t)message_delta_compute_size_8,
    (uint32_t)object_type_definition_return_true, (uint32_t)function_do_nothing, 0x00000000u, 0x00000004u, 0x00000000u,
    (uint32_t)message_delta_compute_size_16, (uint32_t)object_type_definition_return_true,
    (uint32_t)function_do_nothing, 0x00000000u, 0x00000005u, 0x00000001u, (uint32_t)message_delta_string_compute_size,
    (uint32_t)message_delta_count_initialize, (uint32_t)function_do_nothing, 0x00000000u, 0x00000006u, 0x00000001u,
    (uint32_t)message_delta_wide_string_compute_size, (uint32_t)message_delta_count_initialize,
    (uint32_t)function_do_nothing, 0x00000000u, 0x00000007u, 0x00000001u, (uint32_t)message_delta_blob_compute_size,
    (uint32_t)message_delta_count_initialize, (uint32_t)function_do_nothing, 0x00000000u, 0x00000008u, 0x00000001u,
    (uint32_t)message_delta_structure_array_compute_size, (uint32_t)message_delta_structure_array_initialize,
    (uint32_t)function_do_nothing, 0x00000000u, 0x00000009u, 0x00000001u,
    (uint32_t)message_delta_compound_compute_size, (uint32_t)message_delta_compound_initialize,
    (uint32_t)function_do_nothing, 0x00000000u, 0x0000000au, 0x00000001u, (uint32_t)message_delta_pointer_compute_size,
    (uint32_t)message_delta_pointer_initialize, (uint32_t)function_do_nothing, 0x00000000u, 0x0000000bu, 0x00000001u,
    (uint32_t)message_delta_enum_width_compute_size, (uint32_t)message_delta_integer_initialize,
    (uint32_t)function_do_nothing, 0x00000000u, 0x0000000cu, 0x00000001u, (uint32_t)message_delta_range_compute_size,
    (uint32_t)message_delta_range_initialize, (uint32_t)function_do_nothing, 0x00000000u, 0x0000000du, 0x00000001u,
    (uint32_t)message_delta_index_compute_size, (uint32_t)message_delta_index_initialize,
    (uint32_t)message_delta_index_teardown, 0x00000000u, 0x0000000eu, 0x00000001u,
    (uint32_t)message_delta_scalar_array_compute_size, (uint32_t)message_delta_count_initialize,
    (uint32_t)function_do_nothing, 0x00000000u, 0x0000000fu, 0x00000001u,
    (uint32_t)message_delta_scalar_array_compute_size, (uint32_t)message_delta_count_initialize,
    (uint32_t)function_do_nothing, 0x00000000u, 0x00000010u, 0x00000000u, (uint32_t)message_delta_compute_size_32,
    (uint32_t)object_type_definition_return_true, (uint32_t)function_do_nothing, 0x00000000u, 0x00000011u, 0x00000001u,
    (uint32_t)message_delta_first_dword_compute_size, (uint32_t)message_delta_flags_initialize,
    (uint32_t)function_do_nothing, 0x00000000u, 0x00000012u, 0x00000000u, (uint32_t)message_delta_compute_size_32,
    (uint32_t)object_type_definition_return_true, (uint32_t)function_do_nothing, 0x00000000u, 0x00000013u, 0x00000000u,
    (uint32_t)message_delta_compute_size_6, (uint32_t)object_type_definition_return_true,
    (uint32_t)function_do_nothing, 0x00000000u, 0x00000014u, 0x00000001u,
    (uint32_t)message_delta_first_dword_compute_size, (uint32_t)message_delta_quantized_real_initialize,
    (uint32_t)function_do_nothing, 0x00000000u, 0x00000015u, 0x00000001u, (uint32_t)message_delta_normal_compute_size,
    (uint32_t)message_delta_normal_initialize, (uint32_t)function_do_nothing, 0x00000000u, 0x00000016u, 0x00000000u,
    (uint32_t)message_delta_locality_compute_size, (uint32_t)message_delta_locality_initialize,
    (uint32_t)function_do_nothing, 0x00000000u, 0x00000017u, 0x00000000u, (uint32_t)message_delta_compute_size_4,
    (uint32_t)object_type_definition_return_true, (uint32_t)function_do_nothing, 0x00000000u, 0x00000018u, 0x00000000u,
    (uint32_t)message_delta_compute_size_3, (uint32_t)object_type_definition_return_true,
    (uint32_t)function_do_nothing, 0x00000000u, 0x00000019u, 0x00000000u, (uint32_t)message_delta_compute_size_2,
    (uint32_t)object_type_definition_return_true, (uint32_t)function_do_nothing, 0x00000000u, 0x0000001au, 0x00000001u,
    (uint32_t)message_delta_velocity_compute_size, (uint32_t)waypoint_table_quantize_initialize,
    (uint32_t)function_do_nothing, 0x00000000u, 0x0000001bu, 0x00000000u,
    (uint32_t)message_delta_item_placement_compute_size, (uint32_t)message_delta_item_placement_initialize,
    (uint32_t)function_do_nothing, 0x00000000u, 0x00000013u, 0x6e657267u, 0x5f656461u, 0x6e756f63u, 0x00007374u
};

/** 0x0069bfdc..0x0069c00c: object_type_definitions, object_type_definitions_ex */
#pragma section(".geq$0069bfdc", read, write)
__declspec(allocate(".geq$0069bfdc")) __declspec(align(16)) uint8_t eq_pad_0069bfdc[12] = {0};
#pragma section(".geq$0069bfdcv", read, write)
__declspec(allocate(".geq$0069bfdcv")) __declspec(align(4)) uint32_t object_type_definitions[1] = {0x0069b4f0u};
#pragma section(".geq$0069bfe0v", read, write)
__declspec(allocate(".geq$0069bfe0v")) __declspec(align(16)) uint32_t object_type_definitions_ex[11] = {
    0x0069b5b8u, 0x0069b748u, 0x0069b810u, 0x0069b8d8u, 0x0069b9a0u, 0x0069ba68u, 0x0069bcc0u, 0x0069bd88u,
    0x0069be50u, 0x0069bf18u, 0x0069bb30u
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
    0x00000000u, 0x0000000du, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00670428u,
    0x00000000u, 0x0000000du, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00670404u,
    0x00000000u, 0x0000000du, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x006703dcu,
    0x00000000u, 0x0000000du, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x006703a8u,
    0x00000000u, 0x00000009u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x0067038cu,
    0x00000000u, 0x00000017u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00670350u,
    0x00000000u, 0x00000017u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x0067031cu,
    0x00000000u, 0x00000017u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x006702e4u,
    0x00000000u, 0x00000017u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x006702b0u,
    0x00000000u, 0x00000017u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00670280u,
    0x00000000u, 0x00000017u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00670250u,
    0x00000000u, 0x00000017u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00670218u,
    0x00000000u, 0x00000017u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x006701e8u,
    0x00000000u, 0x00000017u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x006701b4u,
    0x00000000u, 0x00000017u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00670178u,
    0x00000000u, 0x00000017u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00670140u,
    0x00000000u, 0x00000017u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00670108u,
    0x00000000u, 0x00000017u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x006700d0u,
    0x00000000u, 0x00000017u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x006700a0u,
    0x00000000u, 0x00000017u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00670070u,
    0x00000000u, 0x00000017u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00670038u,
    0x00000000u, 0x00000017u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00670008u,
    0x00000000u, 0x00000017u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x0066ffd4u,
    0x00000000u, 0x00000017u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x0066ff90u,
    0x00000000u, 0x00000017u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x0066ff50u,
    0x00000000u, 0x00000017u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x0066ff10u,
    0x00000000u, 0x00000017u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x0066fed4u,
    0x00000000u, 0x00000017u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x0066fea0u,
    0x00000000u, 0x00000017u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x0066fe68u,
    0x00000000u, 0x00000017u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x0066fe28u,
    0x00000000u, 0x00000017u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x0066fdf0u,
    0x00000000u, 0x00000017u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x0066fdb8u,
    0x00000000u, 0x0000000eu, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x0066fd98u,
    0x00000000u, 0x0000000fu, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x0066fd7cu,
    0x00000000u, 0x0000000fu, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x0066fd54u,
    0x00000000u, 0x00000012u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x0066fd30u,
    0x00000000u, 0x00000010u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x0066fd08u,
    0x00000000u, 0x00000011u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x0066fce0u,
    0x00000000u, 0x00000011u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x0066fcbcu,
    0x00000000u, 0x00000011u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x0066fc90u,
    0x00000000u, 0x00000014u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x0066fc70u,
    0x00000000u, 0x00000014u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x0066fc4cu,
    0x00000000u, 0x00000016u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x0066fc28u,
    0x00000000u, 0x00000016u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x0066fc00u,
    0x00000000u, 0x0000003bu, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x0066fbecu,
    0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x0066fbdcu,
    0x00000000u, 0x00000021u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x0066fbccu,
    0x00000000u, 0x00000013u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x0066fbb8u
};
#pragma section(".geq$0069da10v", read, write)
__declspec(allocate(".geq$0069da10v")) __declspec(align(16)) uint32_t unknown_0069da10[536] = {
    0x00000000u, 0x00000018u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x0066fba8u,
    0x00000000u, 0x00000024u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x0066fb98u,
    0x00000000u, 0x00000024u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x0066fb7cu,
    0x00000000u, 0x00000024u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x0066fb60u,
    0x00000000u, 0x00000024u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x0066fb3cu,
    0x00000000u, 0x00000024u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x0066fb18u,
    0x00000000u, 0x00000024u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x0066faf4u,
    0x00000000u, 0x00000024u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x0066fad8u,
    0x00000000u, 0x00000024u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x0066fabcu,
    0x00000000u, 0x00000024u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x0066fa98u,
    0x00000000u, 0x00000024u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x0066fa74u,
    0x00000000u, 0x00000024u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x0066fa50u,
    0x00000000u, 0x00000024u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x0066fa2cu,
    0x00000000u, 0x00000024u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x0066fa08u,
    0x00000000u, 0x00000024u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x0066f9e0u,
    0x00000000u, 0x00000024u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x0066f9b8u,
    0x00000000u, 0x00000024u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x0066f990u,
    0x00000000u, 0x00000024u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x0066f96cu,
    0x00000000u, 0x00000024u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x0066f948u,
    0x00000000u, 0x00000024u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x0066f920u,
    0x00000000u, 0x00000024u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x0066f8f4u,
    0x00000000u, 0x00000024u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x0066f8ccu,
    0x00000000u, 0x00000024u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x0066f8a8u,
    0x00000000u, 0x00000024u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x0066f884u,
    0x00000000u, 0x00000024u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x0066f85cu,
    0x00000000u, 0x00000024u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x0066f834u,
    0x00000000u, 0x00000024u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x0066f80cu,
    0x00000000u, 0x00000024u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x0066f7fcu,
    0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x0066f7e8u,
    0x00000000u, 0x00000018u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x0066f7d8u,
    0x00000000u, 0x00000006u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x0066f7b4u,
    0x00000000u, 0x00000006u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x0066f78cu,
    0x00000000u, 0x00000006u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x0066f75cu,
    0x00000000u, 0x00000006u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x0066f728u,
    0x00000000u, 0x00000006u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x0066f6fcu,
    0x00000000u, 0x00000006u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x0066f6ccu,
    0x00000000u, 0x00000006u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x0066f6acu,
    0x00000000u, 0x00000006u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x0066f688u,
    0x00000000u, 0x00000006u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x0066f65cu,
    0x00000000u, 0x00000006u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x0066f62cu,
    0x00000000u, 0x00000006u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x0066f600u,
    0x00000000u, 0x00000006u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x0066f5d0u,
    0x00000000u, 0x00000005u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x0066f5b8u,
    0x00000000u, 0x00000005u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x0066f59cu,
    0x00000000u, 0x00000005u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x0066f578u,
    0x00000000u, 0x00000005u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x0066f550u,
    0x00000000u, 0x00000005u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x0066f530u,
    0x00000000u, 0x00000005u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x0066f50cu,
    0x00000000u, 0x00000005u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x0066f4f8u,
    0x00000000u, 0x00000005u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x0066f4e0u,
    0x00000000u, 0x00000005u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x0066f4c0u,
    0x00000000u, 0x00000005u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x0066f49cu,
    0x00000000u, 0x00000005u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x0066f480u,
    0x00000000u, 0x00000005u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x0066f460u,
    0x00000000u, 0x0000003cu, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x0066f444u,
    0x00000000u, 0x0000003eu, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x0066f424u,
    0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x0066f3fcu,
    0x00000000u, 0x0000001eu, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x0066f3e4u,
    0x00000000u, 0x0000000eu, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x0066f3c0u,
    0x00000000u, 0x0000000fu, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x0066f39cu,
    0x00000000u, 0x00000011u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x0066f378u,
    0x00000000u, 0x00000030u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x0066f35cu,
    0x00000000u, 0x00000017u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x0066f348u,
    0x00000000u, 0x00000039u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x0066f334u,
    0x00000000u, 0x0000000bu, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x0066f324u,
    0x00000000u, 0xffffffffu, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x0066f318u,
    0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x0066f308u
};
#pragma section(".geq$0069e270v", read, write)
__declspec(allocate(".geq$0069e270v")) __declspec(align(16)) uint32_t rasterizer_screen_flash_effect[8] = {
    0x00000000u, 0xffffffffu, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x0066f2f8u
};
#pragma section(".geq$0069e290v", read, write)
__declspec(allocate(".geq$0069e290v")) __declspec(align(16)) uint32_t environment_effect_slot[48] = {
    0x00000000u, 0xffffffffu, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x0066f2e4u,
    0x00000000u, 0xffffffffu, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x0066f2c4u,
    0x00000000u, 0xffffffffu, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x0066f2acu,
    0x00000000u, 0xffffffffu, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x0066f294u,
    0x00000000u, 0xffffffffu, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x0066f27cu,
    0x00000000u, 0xffffffffu, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x0066f26cu
};
#pragma section(".geq$0069e350v", read, write)
__declspec(allocate(".geq$0069e350v")) __declspec(align(16)) uint32_t rasterizer_vertex_shaders[68] = {
    0x00000000u, 0x00670900u, 0x00000000u, 0x006708f8u, 0x00000000u, 0x006708f0u, 0x00000000u, 0x006708dcu,
    0x00000000u, 0x006708c8u, 0x00000000u, 0x006604d4u, 0x00000000u, 0x006708b4u, 0x00000000u, 0x00670894u,
    0x00000000u, 0x00670884u, 0x00000000u, 0x00670868u, 0x00000000u, 0x00670848u, 0x00000000u, 0x0066f324u,
    0x00000000u, 0x00670830u, 0x00000000u, 0x00670818u, 0x00000000u, 0x0066fd98u, 0x00000000u, 0x0066fd7cu,
    0x00000000u, 0x0066fd08u, 0x00000000u, 0x006707f8u, 0x00000000u, 0x0066fd30u, 0x00000000u, 0x0066fbb8u,
    0x00000000u, 0x006707dcu, 0x00000000u, 0x006707bcu, 0x00000000u, 0x0067079cu, 0x00000000u, 0x00670788u,
    0x00000000u, 0x0067077cu, 0x00000000u, 0x0067076cu, 0x00000000u, 0x00670764u, 0x00000000u, 0x00670758u,
    0x00000000u, 0x0067074cu, 0x00000000u, 0x0067073cu, 0x00000000u, 0x00670724u, 0x00000000u, 0x00670708u,
    0x00000000u, 0x006706f4u, 0x00000000u, 0x0066fbccu
};
#pragma section(".geq$0069e460v", read, write)
__declspec(allocate(".geq$0069e460v")) __declspec(align(16)) uint32_t rasterizer_depth_prepass_vertex_shader[2] = {0x00000000u, 0x006706e4u};
#pragma section(".geq$0069e468v", read, write)
__declspec(allocate(".geq$0069e468v")) __declspec(align(8)) uint32_t renderer_unknown_69e468[58] = {
    0x00000000u, 0x006706dcu, 0x00000000u, 0x006706d4u, 0x00000000u, 0x0066f348u, 0x00000000u, 0x006706b8u,
    0x00000000u, 0x006706a0u, 0x00000000u, 0x0067067cu, 0x00000000u, 0x00670654u, 0x00000000u, 0x00670634u,
    0x00000000u, 0x00670610u, 0x00000000u, 0x006705f0u, 0x00000000u, 0x006705ccu, 0x00000000u, 0x006705a8u,
    0x00000000u, 0x00670580u, 0x00000000u, 0x00670560u, 0x00000000u, 0x0067053cu, 0x00000000u, 0x0066f3c0u,
    0x00000000u, 0x00670514u, 0x00000000u, 0x0066f39cu, 0x00000000u, 0x006704f0u, 0x00000000u, 0x0066f378u,
    0x00000000u, 0x006704d8u, 0x00000000u, 0x006704bcu, 0x00000000u, 0x0066f334u, 0x00000000u, 0x006704a8u,
    0x00000000u, 0x00670490u, 0x00000000u, 0x0066f444u, 0x00000000u, 0x00670474u, 0x00000000u, 0x0066f424u,
    0x00000000u, 0x00670454u
};

/** 0x0069fde4..0x0069fdfc: unit_base_animation_state_names, s_stand */
#pragma section(".geq$0069fde4", read, write)
__declspec(allocate(".geq$0069fde4")) __declspec(align(16)) uint8_t eq_pad_0069fde4[4] = {0};
#pragma section(".geq$0069fde4v", read, write)
__declspec(allocate(".geq$0069fde4v")) __declspec(align(4)) uint32_t unit_base_animation_state_names[2] = {0x0065ed78u, 0x0065eddcu};
#pragma section(".geq$0069fdecv", read, write)
__declspec(allocate(".geq$0069fdecv")) __declspec(align(4)) uint32_t s_stand[4] = {0x00671ff4u, 0x00671fecu, 0x0065edccu, 0x0066c058u};
