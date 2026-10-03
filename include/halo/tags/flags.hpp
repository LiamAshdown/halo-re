/**
 * @file include/halo/tags/flags.hpp
 * Bit-flag enums for every bitfield of the tag definitions in types/tags.h (generated from its `bitfield:` comments).
 */
#pragma once

#include <cstdint>
#include "halo/core/flags.hpp"

namespace halo::tags {

using halo::operator|;
using halo::operator&;
using halo::operator^;
using halo::operator~;
using halo::operator|=;
using halo::operator&=;
using halo::operator^=;
using halo::has;
using halo::any;
using halo::to_bits;

/** Bits of the actor tag field (types/tags.h `ActorFlags`). */
enum class actor_tag_flag : uint32_t {
    none = 0,
    can_see_in_darkness = 0x1,
    sneak_uncovering_target = 0x2,
    sneak_uncovering_pursuit_position = 0x4,
    unused = 0x8,
    shoot_at_target_s_last_location = 0x10,
    try_to_stay_still_when_crouched = 0x20,
    crouch_when_not_in_combat = 0x40,
    crouch_when_guarding = 0x80,
    unused_1 = 0x100,
    must_crouch_to_shoot = 0x200,
    panic_when_surprised = 0x400,
    always_charge_at_enemies = 0x800,
    gets_in_vehicles_with_player = 0x1000,
    start_firing_before_aligned = 0x2000,
    standing_must_move_forward = 0x4000,
    crouching_must_move_forward = 0x8000,
    defensive_crouch_while_charging = 0x10000,
    use_stalking_behavior = 0x20000,
    stalking_freeze_if_exposed = 0x40000,
    always_berserk_in_attacking_mode = 0x80000,
    berserking_uses_panicked_movement = 0x100000,
    flying = 0x200000,
    panicked_by_unopposable_enemy = 0x400000,
    crouch_when_hiding_from_unopposable = 0x800000,
    always_charge_in_attacking_mode = 0x1000000,
    dive_off_ledges = 0x2000000,
    swarm = 0x4000000,
    suicidal_melee_attack = 0x8000000,
    cannot_move_while_crouching = 0x10000000,
    fixed_crouch_facing = 0x20000000,
    crouch_when_in_line_of_fire = 0x40000000,
    avoid_friends_line_of_fire = 0x80000000,
};

/** Bits of the actor more tag field (types/tags.h `ActorMoreFlags`). */
enum class actor_more_tag_flag : uint32_t {
    none = 0,
    avoid_all_enemy_attack_vectors = 0x1,
    must_stand_to_fire = 0x2,
    must_stop_to_fire = 0x4,
    disallow_vehicle_combat = 0x8,
    pathfinding_ignores_danger = 0x10,
    panic_in_groups = 0x20,
    no_corpse_shooting = 0x40,
};

/** Bits of the actor variant tag field (types/tags.h `ActorVariantFlags`). */
enum class actor_variant_tag_flag : uint32_t {
    none = 0,
    can_shoot_while_flying = 0x1,
    interpolate_color_in_hsv = 0x2,
    has_unlimited_grenades = 0x4,
    movement_switching_try_to_stay_with_friends = 0x8,
    active_camouflage = 0x10,
    super_active_camouflage = 0x20,
    cannot_use_ranged_weapons = 0x40,
    prefer_passenger_seat = 0x80,
};

/** Bits of the biped tag field (types/tags.h `BipedFlags`). */
enum class biped_tag_flag : uint32_t {
    none = 0,
    turns_without_animating = 0x1,
    uses_player_physics = 0x2,
    flying = 0x4,
    physics_pill_centered_at_origin = 0x8,
    spherical = 0x10,
    passes_through_other_bipeds = 0x20,
    can_climb_any_surface = 0x40,
    immune_to_falling_damage = 0x80,
    rotate_while_airborne = 0x100,
    uses_limp_body_physics = 0x200,
    has_no_dying_airborne = 0x400,
    random_speed_increase = 0x800,
    unit_uses_old_ntsc_player_physics = 0x1000,
};

/** Bits of the object tag field (types/tags.h `ObjectFlags`). */
enum class object_tag_flag : uint16_t {
    none = 0,
    does_not_cast_shadow = 0x1,
    transparent_self_occlusion = 0x2,
    brighter_than_it_should_be = 0x4,
    not_a_pathfinding_obstacle = 0x8,
    extension_of_parent = 0x10,
    cast_shadow_by_default = 0x20,
    does_not_have_anniversary_geometry = 0x40,
};

/** Bits of the object function tag field (types/tags.h `ObjectFunctionFlags`). */
enum class object_function_tag_flag : uint32_t {
    none = 0,
    invert = 0x1,
    additive = 0x2,
    always_active = 0x4,
};

/** Bits of the color interpolation tag field (types/tags.h `ColorInterpolationFlags`). */
enum class color_interpolation_tag_flag : uint32_t {
    none = 0,
    blend_in_hsv = 0x1,
    more_colors = 0x2,
};

/** Bits of the unit tag field (types/tags.h `UnitFlags`). */
enum class unit_tag_flag : uint32_t {
    none = 0,
    circular_aiming = 0x1,
    destroyed_after_dying = 0x2,
    half_speed_interpolation = 0x4,
    fires_from_camera = 0x8,
    entrance_inside_bounding_sphere = 0x10,
    unused = 0x20,
    causes_passenger_dialogue = 0x40,
    resists_pings = 0x80,
    melee_attack_is_fatal = 0x100,
    don_t_reface_during_pings = 0x200,
    has_no_aiming = 0x400,
    simple_creature = 0x800,
    impact_melee_attaches_to_unit = 0x1000,
    impact_melee_dies_on_shields = 0x2000,
    cannot_open_doors_automatically = 0x4000,
    melee_attackers_cannot_attach = 0x8000,
    not_instantly_killed_by_melee = 0x10000,
    shield_sapping = 0x20000,
    runs_around_flaming = 0x40000,
    inconsequential = 0x80000,
    special_cinematic_unit = 0x100000,
    ignored_by_autoaiming = 0x200000,
    shields_fry_infection_forms = 0x400000,
    integrated_light_cntrls_weapon = 0x800000,
    integrated_light_lasts_forever = 0x1000000,
};

/** Bits of the unit seat tag field (types/tags.h `UnitSeatFlags`). */
enum class unit_seat_tag_flag : uint32_t {
    none = 0,
    invisible = 0x1,
    locked = 0x2,
    driver = 0x4,
    gunner = 0x8,
    third_person_camera = 0x10,
    allows_weapons = 0x20,
    third_person_on_enter = 0x40,
    first_person_camera_slaved_to_gun = 0x80,
    allow_vehicle_communication_animations = 0x100,
    not_valid_without_driver = 0x200,
    allow_ai_noncombatants = 0x400,
};

/** Bits of the is unused flag tag field (types/tags.h `IsUnusedFlag`). */
enum class is_unused_flag_tag_flag : uint32_t {
    none = 0,
    unused = 0x1,
};

/** Bits of the is unfiltered flag tag field (types/tags.h `IsUnfilteredFlag`). */
enum class is_unfiltered_flag_tag_flag : uint16_t {
    none = 0,
    unfiltered = 0x1,
};

/** Bits of the bitmap data tag field (types/tags.h `BitmapDataFlags`). */
enum class bitmap_data_tag_flag : uint16_t {
    none = 0,
    power_of_two_dimensions = 0x1,
    compressed = 0x2,
    palettized = 0x4,
    swizzled = 0x8,
    linear = 0x10,
    v16u16 = 0x20,
    unused = 0x40,
    make_it_actually_work = 0x80,
    external = 0x100,
    environment = 0x200,
};

/** Bits of the bitmap tag field (types/tags.h `BitmapFlags`). */
enum class bitmap_tag_flag : uint16_t {
    none = 0,
    enable_diffusion_dithering = 0x1,
    disable_height_map_compression = 0x2,
    uniform_sprite_sequences = 0x4,
    filthy_sprite_bug_fix = 0x8,
    half_hud_scale = 0x10,
    invert_detail_fade = 0x20,
    use_average_color_for_detail_fade = 0x40,
    force_hud_use_highres_scale = 0x80,
};

/** Bits of the damage effect damage tag field (types/tags.h `DamageEffectDamageFlags`). */
enum class damage_effect_damage_tag_flag : uint32_t {
    none = 0,
    does_not_hurt_owner = 0x1,
    can_cause_headshots = 0x2,
    pings_resistant_units = 0x4,
    does_not_hurt_friends = 0x8,
    does_not_ping_units = 0x10,
    detonates_explosives = 0x20,
    only_hurts_shields = 0x40,
    causes_flaming_death = 0x80,
    damage_indicators_always_point_down = 0x100,
    skips_shields = 0x200,
    only_hurts_one_infection_form = 0x400,
    can_cause_multiplayer_headshots = 0x800,
    infection_form_pop = 0x1000,
    ignore_seat_scale_for_dir_dmg = 0x2000,
    forces_hard_ping = 0x4000,
    does_not_hurt_players = 0x8000,
    use_3d_instantaneous_acceleration = 0x10000,
    allow_any_non_zero_acceleration_value = 0x20000,
};

/** Bits of the contrail point state scale tag field (types/tags.h `ContrailPointStateScaleFlags`). */
enum class contrail_point_state_scale_tag_flag : uint32_t {
    none = 0,
    duration = 0x1,
    duration_delta = 0x2,
    transition_duration = 0x4,
    transition_duration_delta = 0x8,
    width = 0x10,
    color = 0x20,
};

/** Bits of the contrail tag field (types/tags.h `ContrailFlags`). */
enum class contrail_tag_flag : uint16_t {
    none = 0,
    first_point_unfaded = 0x1,
    last_point_unfaded = 0x2,
    points_start_pinned_to_media = 0x4,
    points_start_pinned_to_ground = 0x8,
    points_always_pinned_to_media = 0x10,
    points_always_pinned_to_ground = 0x20,
    edge_effect_fades_slowly = 0x40,
};

/** Bits of the contrail scale tag field (types/tags.h `ContrailScaleFlags`). */
enum class contrail_scale_tag_flag : uint16_t {
    none = 0,
    point_generation_rate = 0x1,
    point_velocity = 0x2,
    point_velocity_delta = 0x4,
    point_velocity_cone_angle = 0x8,
    inherited_velocity_fraction = 0x10,
    sequence_animation_rate = 0x20,
    texture_scale_u = 0x40,
    texture_scale_v = 0x80,
    texture_animation_u = 0x100,
    texture_animation_v = 0x200,
};

/** Bits of the particle shader tag field (types/tags.h `ParticleShaderFlags`). */
enum class particle_shader_tag_flag : uint16_t {
    none = 0,
    sort_bias = 0x1,
    nonlinear_tint = 0x2,
    don_t_overdraw_fp_weapon = 0x4,
};

/** Bits of the damage effect tag field (types/tags.h `DamageEffectFlags`). */
enum class damage_effect_tag_flag : uint32_t {
    none = 0,
    do_not_scale_damage_by_distance = 0x1,
};

/** Bits of the decal tag field (types/tags.h `DecalFlags`). */
enum class decal_tag_flag : uint16_t {
    none = 0,
    geometry_inherited_by_next_decal_in_chain = 0x1,
    interpolate_color_in_hsv = 0x2,
    more_colors = 0x4,
    no_random_rotation = 0x8,
    water_effect = 0x10,
    sapien_snap_to_axis = 0x20,
    sapien_incremental_counter = 0x40,
    animation_loop = 0x80,
    preserve_aspect = 0x100,
    disabled_by_blood_setting = 0x200,
    sprite_scale_bug_fix = 0x400,
};

/** Bits of the detail object collection type tag field (types/tags.h `DetailObjectCollectionTypeFlags`). */
enum class detail_object_collection_type_tag_flag : uint8_t {
    none = 0,
    unused_a = 0x1,
    unused_b = 0x2,
    interpolate_color_in_hsv = 0x4,
    more_colors = 0x8,
};

/** Bits of the device tag field (types/tags.h `DeviceFlags`). */
enum class device_tag_flag : uint32_t {
    none = 0,
    position_loops = 0x1,
    position_not_interpolated = 0x2,
};

/** Bits of the machine tag field (types/tags.h `MachineFlags`). */
enum class machine_tag_flag : uint16_t {
    none = 0,
    pathfinding_obstacle = 0x1,
    but_not_when_open = 0x2,
    elevator = 0x4,
};

/** Bits of the effect part tag field (types/tags.h `EffectPartFlags`). */
enum class effect_part_tag_flag : uint16_t {
    none = 0,
    face_down_regardless_of_location_decals = 0x1,
    unused = 0x2,
    make_effect_work = 0x4,
};

/** Bits of the effect part scales values tag field (types/tags.h `EffectPartScalesValues`). */
enum class effect_part_scales_values_tag_flag : uint32_t {
    none = 0,
    velocity = 0x1,
    velocity_delta = 0x2,
    velocity_cone_angle = 0x4,
    angular_velocity = 0x8,
    angular_velocity_delta = 0x10,
    type_specific_scale = 0x20,
};

/** Bits of the effect particle tag field (types/tags.h `EffectParticleFlags`). */
enum class effect_particle_tag_flag : uint32_t {
    none = 0,
    stay_attached_to_marker = 0x1,
    random_initial_angle = 0x2,
    tint_from_object_color = 0x4,
    interpolate_tint_as_hsv = 0x8,
    across_the_long_hue_path = 0x10,
};

/** Bits of the effect particle scales values tag field (types/tags.h `EffectParticleScalesValues`). */
enum class effect_particle_scales_values_tag_flag : uint32_t {
    none = 0,
    velocity = 0x1,
    velocity_delta = 0x2,
    velocity_cone_angle = 0x4,
    angular_velocity = 0x8,
    angular_velocity_delta = 0x10,
    count = 0x20,
    count_delta = 0x40,
    distribution_radius = 0x80,
    distribution_radius_delta = 0x100,
    particle_radius = 0x200,
    particle_radius_delta = 0x400,
    tint = 0x800,
};

/** Bits of the effect tag field (types/tags.h `EffectFlags`). */
enum class effect_tag_flag : uint32_t {
    none = 0,
    deleted_when_attachment_deactivates = 0x1,
    must_be_deterministic_xbox = 0x2,
    must_be_deterministic_pc = 0x4,
    disabled_in_anniversary_by_blood_setting = 0x8,
};

/** Bits of the item tag field (types/tags.h `ItemFlags`). */
enum class item_tag_flag : uint32_t {
    none = 0,
    always_maintains_z_up = 0x1,
    destroyed_by_explosions = 0x2,
    unaffected_by_gravity = 0x4,
};

/** Bits of the fog tag field (types/tags.h `FogFlags`). */
enum class fog_tag_flag : uint32_t {
    none = 0,
    is_water = 0x1,
    atmosphere_dominant = 0x2,
    fog_screen_only = 0x4,
};

/** Bits of the fog screen tag field (types/tags.h `FogScreenFlags`). */
enum class fog_screen_tag_flag : uint16_t {
    none = 0,
    no_environment_multipass = 0x1,
    no_model_multipass = 0x2,
    no_texture_based_falloff = 0x4,
};

/** Bits of the font tag field (types/tags.h `FontFlags`). */
enum class font_tag_flag : uint32_t {
    none = 0,
    disable_mcc_font_override = 0x1,
};

/** Bits of the model geometry part tag field (types/tags.h `ModelGeometryPartFlags`). */
enum class model_geometry_part_tag_flag : uint32_t {
    none = 0,
    stripped_internal = 0x1,
    zoner = 0x2,
};

/** Bits of the model tag field (types/tags.h `ModelFlags`). */
enum class model_tag_flag : uint32_t {
    none = 0,
    blend_shared_normals = 0x1,
    parts_have_local_nodes = 0x2,
    ignore_skinning = 0x4,
};

/** Bits of the model region permutation tag field (types/tags.h `ModelRegionPermutationFlags`). */
enum class model_region_permutation_tag_flag : uint32_t {
    none = 0,
    cannot_be_chosen_randomly = 0x1,
};

/** Bits of the globals rasterizer data tag field (types/tags.h `GlobalsRasterizerDataFlags`). */
enum class globals_rasterizer_data_tag_flag : uint16_t {
    none = 0,
    tint_edge_density = 0x1,
};

/** Bits of the globals breakable surface particle effect tag field (types/tags.h `GlobalsBreakableSurfaceParticleEffectFlags`). */
enum class globals_breakable_surface_particle_effect_tag_flag : uint32_t {
    none = 0,
    interpolate_color_in_hsv = 0x1,
    more_colors = 0x2,
};

/** Bits of the glow tag field (types/tags.h `GlowFlags`). */
enum class glow_tag_flag : uint32_t {
    none = 0,
    modify_particle_color_in_range = 0x1,
    particles_move_backwards = 0x2,
    partices_move_in_both_directions = 0x4,
    trailing_particles_fade_over_time = 0x8,
    trailing_particles_shrink_over_time = 0x10,
    trailing_particles_slow_over_time = 0x20,
};

/** Bits of the grenade hud interface overlay type tag field (types/tags.h `GrenadeHUDInterfaceOverlayType`). */
enum class grenade_hud_interface_overlay_type_tag_flag : uint16_t {
    none = 0,
    show_on_flashing = 0x1,
    show_on_empty = 0x2,
    show_on_default = 0x4,
    show_always = 0x8,
};

/** Bits of the grenade hud interface sound latched to tag field (types/tags.h `GrenadeHUDInterfaceSoundLatchedTo`). */
enum class grenade_hud_interface_sound_latched_to_tag_flag : uint32_t {
    none = 0,
    low_grenade_count = 0x1,
    no_grenades_left = 0x2,
    throw_on_no_grenades = 0x4,
};

/** Bits of the hud interface scaling tag field (types/tags.h `HUDInterfaceScalingFlags`). */
enum class hud_interface_scaling_tag_flag : uint16_t {
    none = 0,
    don_t_scale_offset = 0x1,
    don_t_scale_size = 0x2,
    use_high_res_scale = 0x4,
};

/** Bits of the hud interface flash tag field (types/tags.h `HUDInterfaceFlashFlags`). */
enum class hud_interface_flash_tag_flag : uint16_t {
    none = 0,
    reverse_default_flashing_colors = 0x1,
};

/** Bits of the hud interface overlay flash tag field (types/tags.h `HUDInterfaceOverlayFlashFlags`). */
enum class hud_interface_overlay_flash_tag_flag : uint32_t {
    none = 0,
    flashes_when_active = 0x1,
};

/** Bits of the hud interface number tag field (types/tags.h `HUDInterfaceNumberFlags`). */
enum class hud_interface_number_tag_flag : uint8_t {
    none = 0,
    show_leading_zeros = 0x1,
    only_show_when_zoomed = 0x2,
    draw_a_trailing_m = 0x4,
};

/** Bits of the hud interface messaging tag field (types/tags.h `HUDInterfaceMessagingFlags`). */
enum class hud_interface_messaging_tag_flag : uint8_t {
    none = 0,
    use_text_from_string_list_instead = 0x1,
    override_default_color = 0x2,
    width_offset_is_absolute_icon_width = 0x4,
};

/** Bits of the hud globals waypoint arrow tag field (types/tags.h `HUDGlobalsWaypointArrowFlags`). */
enum class hud_globals_waypoint_arrow_tag_flag : uint32_t {
    none = 0,
    dont_rotate_when_pointing_offscreen = 0x1,
};

/** Bits of the hud globals anniversary remap target tag field (types/tags.h `HUDGlobalsAnniversaryRemapTargetFlags`). */
enum class hud_globals_anniversary_remap_target_tag_flag : uint16_t {
    none = 0,
    legacy_mode = 0x1,
};

/** Bits of the hud interface meter tag field (types/tags.h `HUDInterfaceMeterFlags`). */
enum class hud_interface_meter_tag_flag : uint8_t {
    none = 0,
    use_min_max_for_state_changes = 0x1,
    interpolate_between_min_max_flash_colors_as_state_changes = 0x2,
    interpolate_color_along_hsv_space = 0x4,
    more_colors_for_hsv_interpolation = 0x8,
    invert_interpolation = 0x10,
    use_xbox_shading = 0x20,
};

/** Bits of the input device defaults tag field (types/tags.h `InputDeviceDefaultsFlags`). */
enum class input_device_defaults_tag_flag : uint16_t {
    none = 0,
    unused = 0x1,
};

/** Bits of the lens flare reflection tag field (types/tags.h `LensFlareReflectionFlags`). */
enum class lens_flare_reflection_tag_flag : uint16_t {
    none = 0,
    align_rotation_with_screen_center = 0x1,
    radius_not_scaled_by_distance = 0x2,
    radius_scaled_by_occlusion_factor = 0x4,
    occluded_by_solid_objects = 0x8,
};

/** Bits of the lens flare reflection more tag field (types/tags.h `LensFlareReflectionMoreFlags`). */
enum class lens_flare_reflection_more_tag_flag : uint16_t {
    none = 0,
    interpolate_colors_in_hsv = 0x1,
    more_colors = 0x2,
};

/** Bits of the lens flare tag field (types/tags.h `LensFlareFlags`). */
enum class lens_flare_tag_flag : uint16_t {
    none = 0,
    sun = 0x1,
    no_occlusion_test = 0x2,
    only_render_in_first_person = 0x4,
    only_render_in_third_person = 0x8,
    fade_in_more_quickly = 0x10,
    fade_out_more_quickly = 0x20,
    scale_by_marker = 0x40,
};

/** Bits of the light tag field (types/tags.h `LightFlags`). */
enum class light_tag_flag : uint32_t {
    none = 0,
    dynamic = 0x1,
    no_specular = 0x2,
    don_t_light_own_object = 0x4,
    supersize_in_first_person = 0x8,
    first_person_flashlight = 0x10,
    don_t_fade_active_camouflage = 0x20,
};

/** Bits of the light volume tag field (types/tags.h `LightVolumeFlags`). */
enum class light_volume_tag_flag : uint16_t {
    none = 0,
    interpolate_color_in_hsv = 0x1,
    more_colors = 0x2,
};

/** Bits of the lightning marker flag tag field (types/tags.h `LightningMarkerFlag`). */
enum class lightning_marker_flag_tag_flag : uint16_t {
    none = 0,
    not_connected_to_next_marker = 0x1,
};

/** Bits of the model animations animation graph node tag field (types/tags.h `ModelAnimationsAnimationGraphNodeFlags`). */
enum class model_animations_animation_graph_node_tag_flag : uint32_t {
    none = 0,
    ball_socket = 0x1,
    hinge = 0x2,
    no_movement = 0x4,
};

/** Bits of the model animations animation tag field (types/tags.h `ModelAnimationsAnimationFlags`). */
enum class model_animations_animation_tag_flag : uint16_t {
    none = 0,
    compressed_data = 0x1,
    world_relative = 0x2,
    _25hz_pal = 0x4,
};

/** Bits of the model animations tag field (types/tags.h `ModelAnimationsFlags`). */
enum class model_animations_tag_flag : uint16_t {
    none = 0,
    compress_all_animations = 0x1,
    force_idle_compression = 0x2,
};

/** Bits of the model collision geometry material tag field (types/tags.h `ModelCollisionGeometryMaterialFlags`). */
enum class model_collision_geometry_material_tag_flag : uint32_t {
    none = 0,
    head = 0x1,
};

/** Bits of the model collision geometry region tag field (types/tags.h `ModelCollisionGeometryRegionFlags`). */
enum class model_collision_geometry_region_tag_flag : uint32_t {
    none = 0,
    lives_until_object_dies = 0x1,
    forces_object_to_die = 0x2,
    dies_when_object_dies = 0x4,
    dies_when_object_is_damaged = 0x8,
    disappears_when_shield_is_off = 0x10,
    inhibits_melee_attack = 0x20,
    inhibits_weapon_attack = 0x40,
    inhibits_walking = 0x80,
    forces_drop_weapon = 0x100,
    causes_head_maimed_scream = 0x200,
};

/** Bits of the model collision geometry bsp leaf tag field (types/tags.h `ModelCollisionGeometryBSPLeafFlags`). */
enum class model_collision_geometry_bsp_leaf_tag_flag : uint16_t {
    none = 0,
    contains_double_sided_surfaces = 0x1,
};

/** Bits of the model collision geometry bsp surface tag field (types/tags.h `ModelCollisionGeometryBSPSurfaceFlags`). */
enum class model_collision_geometry_bsp_surface_tag_flag : uint8_t {
    none = 0,
    two_sided = 0x1,
    invisible = 0x2,
    climbable = 0x4,
    breakable = 0x8,
};

/** Bits of the model collision geometry tag field (types/tags.h `ModelCollisionGeometryFlags`). */
enum class model_collision_geometry_tag_flag : uint32_t {
    none = 0,
    takes_shield_damage_for_children = 0x1,
    takes_body_damage_for_children = 0x2,
    always_shields_friendly_damage = 0x4,
    passes_area_damage_to_children = 0x8,
    parent_never_takes_body_damage_for_us = 0x10,
    only_damaged_by_explosives = 0x20,
    only_damaged_while_occupied = 0x40,
};

/** Bits of the base object tag field (types/tags.h `BaseObjectFlags`). */
enum class base_object_tag_flag : uint16_t {
    none = 0,
    off_in_pegasus = 0x1,
};

/** Bits of the particle tag field (types/tags.h `ParticleFlags`). */
enum class particle_tag_flag : uint32_t {
    none = 0,
    can_animate_backwards = 0x1,
    animation_stops_at_rest = 0x2,
    animation_starts_on_random_frame = 0x4,
    animate_once_per_frame = 0x8,
    dies_at_rest = 0x10,
    dies_on_contact_with_structure = 0x20,
    tint_from_diffuse_texture = 0x40,
    dies_on_contact_with_water = 0x80,
    dies_on_contact_with_air = 0x100,
    self_illuminated = 0x200,
    random_horizontal_mirroring = 0x400,
    random_vertical_mirroring = 0x800,
};

/** Bits of the particle system type tag field (types/tags.h `ParticleSystemTypeFlags`). */
enum class particle_system_type_tag_flag : uint32_t {
    none = 0,
    type_states_loop = 0x1,
    forward_backward = 0x2,
    particle_states_loop = 0x4,
    forward_backward_1 = 0x8,
    particles_die_in_water = 0x10,
    particles_die_in_air = 0x20,
    particles_die_on_ground = 0x40,
    rotational_sprites_animate_sideways = 0x80,
    disabled = 0x100,
    tint_by_effect_color = 0x200,
    initial_count_scales_with_effect = 0x400,
    minimum_count_scales_with_effect = 0x800,
    creation_rate_scales_with_effect = 0x1000,
    scale_scales_with_effect = 0x2000,
    animation_rate_scales_with_effect = 0x4000,
    rotation_rate_scales_with_effect = 0x8000,
    do_not_draw_in_first_person = 0x10000,
    do_not_draw_in_third_person = 0x20000,
};

/** Bits of the physics powered mass point tag field (types/tags.h `PhysicsPoweredMassPointFlags`). */
enum class physics_powered_mass_point_tag_flag : uint32_t {
    none = 0,
    ground_friction = 0x1,
    water_friction = 0x2,
    air_friction = 0x4,
    water_lift = 0x8,
    air_lift = 0x10,
    thrust = 0x20,
    antigrav = 0x40,
};

/** Bits of the physics mass point tag field (types/tags.h `PhysicsMassPointFlags`). */
enum class physics_mass_point_tag_flag : uint32_t {
    none = 0,
    metallic = 0x1,
};

/** Bits of the point physics tag field (types/tags.h `PointPhysicsFlags`). */
enum class point_physics_tag_flag : uint32_t {
    none = 0,
    flamethrower_particle_collision = 0x1,
    collides_with_structures = 0x2,
    collides_with_water_surface = 0x4,
    uses_simple_wind = 0x8,
    uses_damped_wind = 0x10,
    no_gravity = 0x20,
};

/** Bits of the projectile tag field (types/tags.h `ProjectileFlags`). */
enum class projectile_tag_flag : uint32_t {
    none = 0,
    oriented_along_velocity = 0x1,
    ai_must_use_ballistic_aiming = 0x2,
    detonation_max_time_if_attached = 0x4,
    has_super_combining_explosion = 0x8,
    combine_initial_velocity_with_parent_velocity = 0x10,
    random_attached_detonation_time = 0x20,
    minimum_unattached_detonation_time = 0x40,
};

/** Bits of the projectile material response tag field (types/tags.h `ProjectileMaterialResponseFlags`). */
enum class projectile_material_response_tag_flag : uint16_t {
    none = 0,
    cannot_be_overpenetrated = 0x1,
};

/** Bits of the projectile material response potential tag field (types/tags.h `ProjectileMaterialResponsePotentialFlags`). */
enum class projectile_material_response_potential_tag_flag : uint16_t {
    none = 0,
    only_against_units = 0x1,
    never_against_units = 0x2,
};

/** Bits of the scenario text tag field (types/tags.h `ScenarioTextFlags`). */
enum class scenario_text_tag_flag : uint32_t {
    none = 0,
    wrap_horizontally = 0x1,
    wrap_vertically = 0x2,
    center_vertically = 0x4,
    bottom_justify = 0x8,
};

/** Bits of the scenario function tag field (types/tags.h `ScenarioFunctionFlags`). */
enum class scenario_function_tag_flag : uint32_t {
    none = 0,
    scripted = 0x1,
    invert = 0x2,
    additive = 0x4,
    always_active = 0x8,
};

/** Bits of the scenario spawn not placed tag field (types/tags.h `ScenarioSpawnNotPlaced`). */
enum class scenario_spawn_not_placed_tag_flag : uint16_t {
    none = 0,
    automatically = 0x1,
    on_easy = 0x2,
    on_normal = 0x4,
    on_hard = 0x8,
    use_player_appearance = 0x10,
};

/** Bits of the scenario unit tag field (types/tags.h `ScenarioUnitFlags`). */
enum class scenario_unit_tag_flag : uint32_t {
    none = 0,
    dead = 0x1,
};

/** Bits of the scenario vehicle multiplayer spawn tag field (types/tags.h `ScenarioVehicleMultiplayerSpawnFlags`). */
enum class scenario_vehicle_multiplayer_spawn_tag_flag : uint16_t {
    none = 0,
    slayer_default = 0x1,
    ctf_default = 0x2,
    king_default = 0x4,
    oddball_default = 0x8,
    unused = 0x10,
    unused1 = 0x20,
    unused2 = 0x40,
    unused3 = 0x80,
    slayer_allowed = 0x100,
    ctf_allowed = 0x200,
    king_allowed = 0x400,
    oddball_allowed = 0x800,
    unused4 = 0x1000,
    unused5 = 0x2000,
    unused6 = 0x4000,
    unused7 = 0x8000,
};

/** Bits of the scenario item tag field (types/tags.h `ScenarioItemFlags`). */
enum class scenario_item_tag_flag : uint16_t {
    none = 0,
    initially_at_rest = 0x1,
    obsolete = 0x2,
    does_accelerate = 0x4,
};

/** Bits of the scenario device group tag field (types/tags.h `ScenarioDeviceGroupFlags`). */
enum class scenario_device_group_tag_flag : uint32_t {
    none = 0,
    can_change_only_once = 0x1,
};

/** Bits of the scenario device tag field (types/tags.h `ScenarioDeviceFlags`). */
enum class scenario_device_tag_flag : uint32_t {
    none = 0,
    initially_open = 0x1,
    initially_off = 0x2,
    can_change_only_once = 0x4,
    position_reversed = 0x8,
    not_usable_from_any_side = 0x10,
};

/** Bits of the scenario machine tag field (types/tags.h `ScenarioMachineFlags`). */
enum class scenario_machine_tag_flag : uint32_t {
    none = 0,
    does_not_operate_automatically = 0x1,
    one_sided = 0x2,
    never_appears_locked = 0x4,
    opened_by_melee_attack = 0x8,
};

/** Bits of the scenario control tag field (types/tags.h `ScenarioControlFlags`). */
enum class scenario_control_tag_flag : uint32_t {
    none = 0,
    usable_from_both_sides = 0x1,
};

/** Bits of the scenario netgame equipment tag field (types/tags.h `ScenarioNetgameEquipmentFlags`). */
enum class scenario_netgame_equipment_tag_flag : uint32_t {
    none = 0,
    levitate = 0x1,
};

/** Bits of the scenario starting equipment tag field (types/tags.h `ScenarioStartingEquipmentFlags`). */
enum class scenario_starting_equipment_tag_flag : uint32_t {
    none = 0,
    no_grenades = 0x1,
    plasma_grenades_only = 0x2,
    type2_grenades_only = 0x4,
    type3_grenades_only = 0x8,
};

/** Bits of the scenario actor starting location tag field (types/tags.h `ScenarioActorStartingLocationFlags`). */
enum class scenario_actor_starting_location_tag_flag : uint8_t {
    none = 0,
    required = 0x1,
};

/** Bits of the scenario squad tag field (types/tags.h `ScenarioSquadFlags`). */
enum class scenario_squad_tag_flag : uint32_t {
    none = 0,
    unused = 0x1,
    never_search = 0x2,
    start_timer_immediately = 0x4,
    no_timer_delay_forever = 0x8,
    magic_sight_after_timer = 0x10,
    automatic_migration = 0x20,
};

/** Bits of the scenario squad attacking tag field (types/tags.h `ScenarioSquadAttacking`). */
enum class scenario_squad_attacking_tag_flag : uint32_t {
    none = 0,
    a = 0x1,
    b = 0x2,
    c = 0x4,
    d = 0x8,
    e = 0x10,
    f = 0x20,
    g = 0x40,
    h = 0x80,
    i = 0x100,
    j = 0x200,
    k = 0x400,
    l = 0x800,
    m = 0x1000,
    n = 0x2000,
    o = 0x4000,
    p = 0x8000,
    q = 0x10000,
    r = 0x20000,
    s = 0x40000,
    t = 0x80000,
    u = 0x100000,
    v = 0x200000,
    w = 0x400000,
    x = 0x800000,
    y = 0x1000000,
    z = 0x2000000,
};

/** Bits of the scenario platoon tag field (types/tags.h `ScenarioPlatoonFlags`). */
enum class scenario_platoon_tag_flag : uint32_t {
    none = 0,
    flee_when_maneuvering = 0x1,
    say_advancing_when_maneuver = 0x2,
    start_in_defending_state = 0x4,
};

/** Bits of the scenario encounter tag field (types/tags.h `ScenarioEncounterFlags`). */
enum class scenario_encounter_tag_flag : uint32_t {
    none = 0,
    not_initially_created = 0x1,
    respawn_enabled = 0x2,
    initially_blind = 0x4,
    initially_deaf = 0x8,
    initially_braindead = 0x10,
    _3d_firing_positions = 0x20,
    manual_bsp_index_specified = 0x40,
};

/** Bits of the scenario command list tag field (types/tags.h `ScenarioCommandListFlags`). */
enum class scenario_command_list_tag_flag : uint32_t {
    none = 0,
    allow_initiative = 0x1,
    allow_targeting = 0x2,
    disable_looking = 0x4,
    disable_communication = 0x8,
    disable_falling_damage = 0x10,
    manual_bsp_index = 0x20,
};

/** Bits of the scenario ai conversation participant tag field (types/tags.h `ScenarioAIConversationParticipantFlags`). */
enum class scenario_ai_conversation_participant_tag_flag : uint16_t {
    none = 0,
    optional = 0x1,
    has_alternate = 0x2,
    is_alternate = 0x4,
};

/** Bits of the scenario ai conversation line tag field (types/tags.h `ScenarioAIConversationLineFlags`). */
enum class scenario_ai_conversation_line_tag_flag : uint16_t {
    none = 0,
    addressee_look_at_speaker = 0x1,
    everyone_look_at_speaker = 0x2,
    everyone_look_at_addressee = 0x4,
    wait_after_until_told_to_advance = 0x8,
    wait_until_speaker_nearby = 0x10,
    wait_until_everyone_nearby = 0x20,
};

/** Bits of the scenario ai conversation tag field (types/tags.h `ScenarioAIConversationFlags`). */
enum class scenario_ai_conversation_tag_flag : uint16_t {
    none = 0,
    stop_if_death = 0x1,
    stop_if_damaged = 0x2,
    stop_if_visible_enemy = 0x4,
    stop_if_alerted_to_enemy = 0x8,
    player_must_be_visible = 0x10,
    stop_other_actions = 0x20,
    keep_trying_to_play = 0x40,
    player_must_be_looking = 0x80,
};

/** Bits of the scenario tag field (types/tags.h `ScenarioFlags`). */
enum class scenario_tag_flag : uint16_t {
    none = 0,
    cortana_hack = 0x1,
    use_demo_ui = 0x2,
    color_correction_ntsc_to_srgb = 0x4,
    do_not_apply_bungie_campaign_tag_patches = 0x8,
};

/** Bits of the scenario script node tag field (types/tags.h `ScenarioScriptNodeFlags`). */
enum class scenario_script_node_tag_flag : uint16_t {
    none = 0,
    is_primitive = 0x1,
    is_script_call = 0x2,
    is_global = 0x4,
    is_garbage_collectable = 0x8,
    is_local_variable = 0x10,
};

/** Bits of the scenario structure bsp material tag field (types/tags.h `ScenarioStructureBSPMaterialFlags`). */
enum class scenario_structure_bsp_material_tag_flag : uint16_t {
    none = 0,
    coplanar = 0x1,
    fog_plane = 0x2,
};

/** Bits of the scenario structure bsp cluster portal tag field (types/tags.h `ScenarioStructureBSPClusterPortalFlags`). */
enum class scenario_structure_bsp_cluster_portal_tag_flag : uint32_t {
    none = 0,
    ai_can_simply_not_hear_through_all_this_amazing_stuff_darn_it = 0x1,
};

/** Bits of the shader tag field (types/tags.h `ShaderFlags`). */
enum class shader_tag_flag : uint16_t {
    none = 0,
    simple_parameterization = 0x1,
    ignore_normals = 0x2,
    transparent_lit = 0x4,
};

/** Bits of the shader physics tag field (types/tags.h `ShaderPhysicsFlags`). */
enum class shader_physics_tag_flag : uint16_t {
    none = 0,
    unused = 0x1,
};

/** Bits of the shader environment tag field (types/tags.h `ShaderEnvironmentFlags`). */
enum class shader_environment_tag_flag : uint16_t {
    none = 0,
    alpha_tested = 0x1,
    bump_map_is_specular_mask = 0x2,
    true_atmospheric_fog = 0x4,
    use_alternate_bump_attenuation = 0x8,
    use_alternate_normal_type_blending = 0x10,
};

/** Bits of the shader environment diffuse tag field (types/tags.h `ShaderEnvironmentDiffuseFlags`). */
enum class shader_environment_diffuse_tag_flag : uint16_t {
    none = 0,
    rescale_detail_maps = 0x1,
    rescale_bump_map = 0x2,
};

/** Bits of the shader environment specular tag field (types/tags.h `ShaderEnvironmentSpecularFlags`). */
enum class shader_environment_specular_tag_flag : uint16_t {
    none = 0,
    overbright = 0x1,
    extra_shiny = 0x2,
    lightmap_is_specular = 0x4,
};

/** Bits of the shader environment reflection tag field (types/tags.h `ShaderEnvironmentReflectionFlags`). */
enum class shader_environment_reflection_tag_flag : uint16_t {
    none = 0,
    dynamic_mirror = 0x1,
};

/** Bits of the shader model tag field (types/tags.h `ShaderModelFlags`). */
enum class shader_model_tag_flag : uint16_t {
    none = 0,
    detail_after_reflection = 0x1,
    two_sided = 0x2,
    not_alpha_tested = 0x4,
    alpha_blended_decal = 0x8,
    true_atmospheric_fog = 0x10,
    disable_two_sided_culling = 0x20,
    use_xbox_multipurpose_channel_order = 0x40,
};

/** Bits of the shader model more tag field (types/tags.h `ShaderModelMoreFlags`). */
enum class shader_model_more_tag_flag : uint16_t {
    none = 0,
    no_random_phase = 0x1,
};

/** Bits of the shader transparent chicago map tag field (types/tags.h `ShaderTransparentChicagoMapFlags`). */
enum class shader_transparent_chicago_map_tag_flag : uint16_t {
    none = 0,
    unfiltered = 0x1,
    alpha_replicate = 0x2,
    u_clamped = 0x4,
    v_clamped = 0x8,
};

/** Bits of the shader transparent chicago extra tag field (types/tags.h `ShaderTransparentChicagoExtraFlags`). */
enum class shader_transparent_chicago_extra_tag_flag : uint32_t {
    none = 0,
    don_t_fade_active_camouflage = 0x1,
    numeric_countdown_timer = 0x2,
    custom_edition_blending = 0x4,
};

/** Bits of the shader transparent generic tag field (types/tags.h `ShaderTransparentGenericFlags`). */
enum class shader_transparent_generic_tag_flag : uint8_t {
    none = 0,
    alpha_tested = 0x1,
    decal = 0x2,
    two_sided = 0x4,
    first_map_is_in_screenspace = 0x8,
    draw_before_water = 0x10,
    ignore_effect = 0x20,
    scale_first_map_with_distance = 0x40,
    numeric = 0x80,
};

/** Bits of the shader transparent generic map flag tag field (types/tags.h `ShaderTransparentGenericMapFlag`). */
enum class shader_transparent_generic_map_flag_tag_flag : uint16_t {
    none = 0,
    unfiltered = 0x1,
    u_clamped = 0x2,
    v_clamped = 0x4,
};

/** Bits of the shader transparent generic stage tag field (types/tags.h `ShaderTransparentGenericStageFlags`). */
enum class shader_transparent_generic_stage_tag_flag : uint16_t {
    none = 0,
    color_mux = 0x1,
    alpha_mux = 0x2,
    a_out_controls_color0_animation = 0x4,
};

/** Bits of the shader transparent glass tag field (types/tags.h `ShaderTransparentGlassFlags`). */
enum class shader_transparent_glass_tag_flag : uint16_t {
    none = 0,
    alpha_tested = 0x1,
    decal = 0x2,
    two_sided = 0x4,
    bump_map_is_specular_mask = 0x8,
};

/** Bits of the shader transparent meter tag field (types/tags.h `ShaderTransparentMeterFlags`). */
enum class shader_transparent_meter_tag_flag : uint16_t {
    none = 0,
    decal = 0x1,
    two_sided = 0x2,
    flash_color_is_negative = 0x4,
    tint_mode_2 = 0x8,
    unfiltered = 0x10,
};

/** Bits of the shader transparent water tag field (types/tags.h `ShaderTransparentWaterFlags`). */
enum class shader_transparent_water_tag_flag : uint16_t {
    none = 0,
    base_map_alpha_modulates_reflection = 0x1,
    base_map_color_modulates_background = 0x2,
    atmospheric_fog = 0x4,
    draw_before_fog = 0x8,
};

/** Bits of the sky light tag field (types/tags.h `SkyLightFlags`). */
enum class sky_light_tag_flag : uint32_t {
    none = 0,
    affects_exteriors = 0x1,
    affects_interiors = 0x2,
};

/** Bits of the sound tag field (types/tags.h `SoundFlags`). */
enum class sound_tag_flag : uint32_t {
    none = 0,
    fit_to_adpcm_blocksize = 0x1,
    split_long_sound_into_permutations = 0x2,
    thirsty_grunt = 0x4,
};

/** Bits of the sound looping track tag field (types/tags.h `SoundLoopingTrackFlags`). */
enum class sound_looping_track_tag_flag : uint32_t {
    none = 0,
    fade_in_at_start = 0x1,
    fade_out_at_stop = 0x2,
    fade_in_alternate = 0x4,
};

/** Bits of the sound looping detail tag field (types/tags.h `SoundLoopingDetailFlags`). */
enum class sound_looping_detail_tag_flag : uint32_t {
    none = 0,
    don_t_play_with_alternate = 0x1,
    don_t_play_without_alternate = 0x2,
};

/** Bits of the sound looping tag field (types/tags.h `SoundLoopingFlags`). */
enum class sound_looping_tag_flag : uint32_t {
    none = 0,
    deafening_to_ais = 0x1,
    not_a_loop = 0x2,
    stops_music = 0x4,
    siege_of_madrigal = 0x8,
};

/** Bits of the event handler references tag field (types/tags.h `EventHandlerReferencesFlags`). */
enum class event_handler_references_tag_flag : uint32_t {
    none = 0,
    close_current_widget = 0x1,
    close_other_widget = 0x2,
    close_all_widgets = 0x4,
    open_widget = 0x8,
    reload_self = 0x10,
    reload_other_widget = 0x20,
    give_focus_to_widget = 0x40,
    run_function = 0x80,
    replace_self_w_widget = 0x100,
    go_back_to_previous_widget = 0x200,
    run_scenario_script = 0x400,
    try_to_branch_on_failure = 0x800,
};

/** Bits of the conditional widget reference tag field (types/tags.h `ConditionalWidgetReferenceFlags`). */
enum class conditional_widget_reference_tag_flag : uint32_t {
    none = 0,
    load_if_event_handler_function_fails = 0x1,
};

/** Bits of the child widget reference tag field (types/tags.h `ChildWidgetReferenceFlags`). */
enum class child_widget_reference_tag_flag : uint32_t {
    none = 0,
    use_custom_controller_index = 0x1,
};

/** Bits of the ui widget definition tag field (types/tags.h `UIWidgetDefinitionFlags`). */
enum class ui_widget_definition_tag_flag : uint32_t {
    none = 0,
    pass_unhandled_events_to_focused_child = 0x1,
    pause_game_time = 0x2,
    flash_background_bitmap = 0x4,
    dpad_up_down_tabs_thru_children = 0x8,
    dpad_left_right_tabs_thru_children = 0x10,
    dpad_up_down_tabs_thru_list_items = 0x20,
    dpad_left_right_tabs_thru_list_items = 0x40,
    dont_focus_a_specific_child_widget = 0x80,
    pass_unhandled_events_to_all_children = 0x100,
    render_regardless_of_controller_index = 0x200,
    pass_handled_events_to_all_children = 0x400,
    return_to_main_menu_if_no_history = 0x800,
    always_use_tag_controller_index = 0x1000,
    always_use_nifty_render_fx = 0x2000,
    don_t_push_history = 0x4000,
    force_handle_mouse = 0x8000,
};

/** Bits of the ui widget definition flags1 tag field (types/tags.h `UIWidgetDefinitionFlags1`). */
enum class ui_widget_definition_flags1_tag_flag : uint32_t {
    none = 0,
    editable = 0x1,
    password = 0x2,
    flashing = 0x4,
    don_t_do_that_weird_focus_test = 0x8,
};

/** Bits of the ui widget definition flags2 tag field (types/tags.h `UIWidgetDefinitionFlags2`). */
enum class ui_widget_definition_flags2_tag_flag : uint32_t {
    none = 0,
    list_items_generated_in_code = 0x1,
    list_items_from_string_list_tag = 0x2,
    list_items_only_one_tooltip = 0x4,
    list_single_preview_no_scroll = 0x8,
};

/** Bits of the unit hud interface auxiliary overlay tag field (types/tags.h `UnitHUDInterfaceAuxiliaryOverlayFlags`). */
enum class unit_hud_interface_auxiliary_overlay_tag_flag : uint16_t {
    none = 0,
    use_team_color = 0x1,
};

/** Bits of the unit hud interface hud sound latched to tag field (types/tags.h `UnitHUDInterfaceHUDSoundLatchedTo`). */
enum class unit_hud_interface_hud_sound_latched_to_tag_flag : uint32_t {
    none = 0,
    shield_recharging = 0x1,
    shield_damaged = 0x2,
    shield_low = 0x4,
    shield_empty = 0x8,
    health_low = 0x10,
    health_empty = 0x20,
    health_minor_damage = 0x40,
    health_major_damage = 0x80,
};

/** Bits of the unit hud interface auxiliary panel meter more tag field (types/tags.h `UnitHUDInterfaceAuxiliaryPanelMeterMoreFlags`). */
enum class unit_hud_interface_auxiliary_panel_meter_more_tag_flag : uint32_t {
    none = 0,
    show_only_when_active = 0x1,
    flash_once_if_activated_while_disabled = 0x2,
};

/** Bits of the vehicle tag field (types/tags.h `VehicleFlags`). */
enum class vehicle_tag_flag : uint32_t {
    none = 0,
    speed_wakes_physics = 0x1,
    turn_wakes_physics = 0x2,
    driver_power_wakes_physics = 0x4,
    gunner_power_wakes_physics = 0x8,
    control_opposite_speed_sets_brake = 0x10,
    slide_wakes_physics = 0x20,
    kills_riders_at_terminal_velocity = 0x40,
    causes_collision_damage = 0x80,
    ai_weapon_cannot_rotate = 0x100,
    ai_does_not_require_driver = 0x200,
    ai_unused = 0x400,
    ai_driver_enable = 0x800,
    ai_driver_flying = 0x1000,
    ai_driver_can_sidestep = 0x2000,
    ai_driver_hovering = 0x4000,
    vehicle_steers_directly = 0x8000,
    unused = 0x10000,
    has_ebrake = 0x20000,
    noncombat_vehicle = 0x40000,
    no_friction_with_driver = 0x80000,
    can_trigger_automatic_opening_doors = 0x100000,
    autoaim_when_teamless = 0x200000,
};

/** Bits of the weapon magazine tag field (types/tags.h `WeaponMagazineFlags`). */
enum class weapon_magazine_tag_flag : uint32_t {
    none = 0,
    wastes_rounds_when_reloaded = 0x1,
    every_round_must_be_chambered = 0x2,
};

/** Bits of the weapon trigger tag field (types/tags.h `WeaponTriggerFlags`). */
enum class weapon_trigger_tag_flag : uint32_t {
    none = 0,
    tracks_fired_projectile = 0x1,
    random_firing_effects = 0x2,
    can_fire_with_partial_ammo = 0x4,
    does_not_repeat_automatically = 0x8,
    locks_in_on_off_state = 0x10,
    projectiles_use_weapon_origin = 0x20,
    sticks_when_dropped = 0x40,
    ejects_during_chamber = 0x80,
    discharging_spews = 0x100,
    analog_rate_of_fire = 0x200,
    use_error_when_unzoomed = 0x400,
    projectile_vector_cannot_be_adjusted = 0x800,
    projectiles_have_identical_error = 0x1000,
    projectile_is_client_side_only = 0x2000,
    use_original_unit_adjust_projectile_ray = 0x4000,
};

/** Bits of the weapon tag field (types/tags.h `WeaponFlags`). */
enum class weapon_tag_flag : uint32_t {
    none = 0,
    vertical_heat_display = 0x1,
    mutually_exclusive_triggers = 0x2,
    attacks_automatically_on_bump = 0x4,
    must_be_readied = 0x8,
    doesn_t_count_toward_maximum = 0x10,
    aim_assists_only_when_zoomed = 0x20,
    prevents_grenade_throwing = 0x40,
    must_be_picked_up = 0x80,
    holds_triggers_when_dropped = 0x100,
    prevents_melee_attack = 0x200,
    detonates_when_dropped = 0x400,
    cannot_fire_at_maximum_age = 0x800,
    secondary_trigger_overrides_grenades = 0x1000,
    does_not_depower_active_camo_in_multilplayer = 0x2000,
    enables_integrated_night_vision = 0x4000,
    ais_use_weapon_melee_damage = 0x8000,
    prevents_crouching = 0x10000,
    uses_3rd_person_camera = 0x20000,
};

/** Bits of the weapon hud interface crosshair type tag field (types/tags.h `WeaponHUDInterfaceCrosshairTypeFlags`). */
enum class weapon_hud_interface_crosshair_type_tag_flag : uint32_t {
    none = 0,
    aim = 0x1,
    zoom_overlay = 0x2,
    charge = 0x4,
    should_reload = 0x8,
    flash_heat = 0x10,
    flash_total_ammo = 0x20,
    flash_battery = 0x40,
    reload_overheat = 0x80,
    flash_when_firing_and_no_ammo = 0x100,
    flash_when_throwing_and_no_grenade = 0x200,
    low_ammo_and_none_left_to_reload = 0x400,
    should_reload_secondary_trigger = 0x800,
    flash_secondary_total_ammo = 0x1000,
    flash_secondary_reload = 0x2000,
    flash_when_firing_secondary_trigger_with_no_ammo = 0x4000,
    low_secondary_ammo_and_none_left_to_reload = 0x8000,
    primary_trigger_ready = 0x10000,
    secondary_trigger_ready = 0x20000,
    flash_when_firing_with_depleted_battery = 0x40000,
};

/** Bits of the weapon hud interface number weapon specific tag field (types/tags.h `WeaponHUDInterfaceNumberWeaponSpecificFlags`). */
enum class weapon_hud_interface_number_weapon_specific_tag_flag : uint16_t {
    none = 0,
    divide_number_by_clip_size = 0x1,
};

/** Bits of the weapon hud interface crosshair overlay tag field (types/tags.h `WeaponHUDInterfaceCrosshairOverlayFlags`). */
enum class weapon_hud_interface_crosshair_overlay_tag_flag : uint32_t {
    none = 0,
    flashes_when_active = 0x1,
    not_a_sprite = 0x2,
    show_only_when_zoomed = 0x4,
    show_sniper_data = 0x8,
    hide_area_outside_reticle = 0x10,
    one_zoom_level = 0x20,
    don_t_show_when_zoomed = 0x40,
};

/** Bits of the weapon hud interface overlay type tag field (types/tags.h `WeaponHUDInterfaceOverlayType`). */
enum class weapon_hud_interface_overlay_type_tag_flag : uint16_t {
    none = 0,
    show_on_flashing = 0x1,
    show_on_empty = 0x2,
    show_on_reload_overheating = 0x4,
    show_on_default = 0x8,
    show_always = 0x10,
};

/** Bits of the weapon hud interface screen effect definition mask tag field (types/tags.h `WeaponHUDInterfaceScreenEffectDefinitionMaskFlags`). */
enum class weapon_hud_interface_screen_effect_definition_mask_tag_flag : uint16_t {
    none = 0,
    only_when_zoomed = 0x1,
};

/** Bits of the weapon hud interface screen effect definition night vision tag field (types/tags.h `WeaponHUDInterfaceScreenEffectDefinitionNightVisionFlags`). */
enum class weapon_hud_interface_screen_effect_definition_night_vision_tag_flag : uint16_t {
    none = 0,
    only_when_zoomed = 0x1,
    connect_to_flashlight = 0x2,
    masked = 0x4,
};

/** Bits of the weapon hud interface screen effect definition desaturation tag field (types/tags.h `WeaponHUDInterfaceScreenEffectDefinitionDesaturationFlags`). */
enum class weapon_hud_interface_screen_effect_definition_desaturation_tag_flag : uint16_t {
    none = 0,
    only_when_zoomed = 0x1,
    connect_to_flashlight = 0x2,
    additive = 0x4,
    masked = 0x8,
};

/** Bits of the weapon hud interface tag field (types/tags.h `WeaponHUDInterfaceFlags`). */
enum class weapon_hud_interface_tag_flag : uint16_t {
    none = 0,
    use_parent_hud_flashing_parameters = 0x1,
};

/** Bits of the weather particle system particle type tag field (types/tags.h `WeatherParticleSystemParticleTypeFlags`). */
enum class weather_particle_system_particle_type_tag_flag : uint32_t {
    none = 0,
    interpolate_colors_in_hsv = 0x1,
    along_long_hue_path = 0x2,
    random_rotation = 0x4,
};

}  // namespace halo::tags

namespace halo {
template <> struct enable_bit_flags<tags::actor_tag_flag> : std::true_type {};
template <> struct enable_bit_flags<tags::actor_more_tag_flag> : std::true_type {};
template <> struct enable_bit_flags<tags::actor_variant_tag_flag> : std::true_type {};
template <> struct enable_bit_flags<tags::biped_tag_flag> : std::true_type {};
template <> struct enable_bit_flags<tags::object_tag_flag> : std::true_type {};
template <> struct enable_bit_flags<tags::object_function_tag_flag> : std::true_type {};
template <> struct enable_bit_flags<tags::color_interpolation_tag_flag> : std::true_type {};
template <> struct enable_bit_flags<tags::unit_tag_flag> : std::true_type {};
template <> struct enable_bit_flags<tags::unit_seat_tag_flag> : std::true_type {};
template <> struct enable_bit_flags<tags::is_unused_flag_tag_flag> : std::true_type {};
template <> struct enable_bit_flags<tags::is_unfiltered_flag_tag_flag> : std::true_type {};
template <> struct enable_bit_flags<tags::bitmap_data_tag_flag> : std::true_type {};
template <> struct enable_bit_flags<tags::bitmap_tag_flag> : std::true_type {};
template <> struct enable_bit_flags<tags::damage_effect_damage_tag_flag> : std::true_type {};
template <> struct enable_bit_flags<tags::contrail_point_state_scale_tag_flag> : std::true_type {};
template <> struct enable_bit_flags<tags::contrail_tag_flag> : std::true_type {};
template <> struct enable_bit_flags<tags::contrail_scale_tag_flag> : std::true_type {};
template <> struct enable_bit_flags<tags::particle_shader_tag_flag> : std::true_type {};
template <> struct enable_bit_flags<tags::damage_effect_tag_flag> : std::true_type {};
template <> struct enable_bit_flags<tags::decal_tag_flag> : std::true_type {};
template <> struct enable_bit_flags<tags::detail_object_collection_type_tag_flag> : std::true_type {};
template <> struct enable_bit_flags<tags::device_tag_flag> : std::true_type {};
template <> struct enable_bit_flags<tags::machine_tag_flag> : std::true_type {};
template <> struct enable_bit_flags<tags::effect_part_tag_flag> : std::true_type {};
template <> struct enable_bit_flags<tags::effect_part_scales_values_tag_flag> : std::true_type {};
template <> struct enable_bit_flags<tags::effect_particle_tag_flag> : std::true_type {};
template <> struct enable_bit_flags<tags::effect_particle_scales_values_tag_flag> : std::true_type {};
template <> struct enable_bit_flags<tags::effect_tag_flag> : std::true_type {};
template <> struct enable_bit_flags<tags::item_tag_flag> : std::true_type {};
template <> struct enable_bit_flags<tags::fog_tag_flag> : std::true_type {};
template <> struct enable_bit_flags<tags::fog_screen_tag_flag> : std::true_type {};
template <> struct enable_bit_flags<tags::font_tag_flag> : std::true_type {};
template <> struct enable_bit_flags<tags::model_geometry_part_tag_flag> : std::true_type {};
template <> struct enable_bit_flags<tags::model_tag_flag> : std::true_type {};
template <> struct enable_bit_flags<tags::model_region_permutation_tag_flag> : std::true_type {};
template <> struct enable_bit_flags<tags::globals_rasterizer_data_tag_flag> : std::true_type {};
template <> struct enable_bit_flags<tags::globals_breakable_surface_particle_effect_tag_flag> : std::true_type {};
template <> struct enable_bit_flags<tags::glow_tag_flag> : std::true_type {};
template <> struct enable_bit_flags<tags::grenade_hud_interface_overlay_type_tag_flag> : std::true_type {};
template <> struct enable_bit_flags<tags::grenade_hud_interface_sound_latched_to_tag_flag> : std::true_type {};
template <> struct enable_bit_flags<tags::hud_interface_scaling_tag_flag> : std::true_type {};
template <> struct enable_bit_flags<tags::hud_interface_flash_tag_flag> : std::true_type {};
template <> struct enable_bit_flags<tags::hud_interface_overlay_flash_tag_flag> : std::true_type {};
template <> struct enable_bit_flags<tags::hud_interface_number_tag_flag> : std::true_type {};
template <> struct enable_bit_flags<tags::hud_interface_messaging_tag_flag> : std::true_type {};
template <> struct enable_bit_flags<tags::hud_globals_waypoint_arrow_tag_flag> : std::true_type {};
template <> struct enable_bit_flags<tags::hud_globals_anniversary_remap_target_tag_flag> : std::true_type {};
template <> struct enable_bit_flags<tags::hud_interface_meter_tag_flag> : std::true_type {};
template <> struct enable_bit_flags<tags::input_device_defaults_tag_flag> : std::true_type {};
template <> struct enable_bit_flags<tags::lens_flare_reflection_tag_flag> : std::true_type {};
template <> struct enable_bit_flags<tags::lens_flare_reflection_more_tag_flag> : std::true_type {};
template <> struct enable_bit_flags<tags::lens_flare_tag_flag> : std::true_type {};
template <> struct enable_bit_flags<tags::light_tag_flag> : std::true_type {};
template <> struct enable_bit_flags<tags::light_volume_tag_flag> : std::true_type {};
template <> struct enable_bit_flags<tags::lightning_marker_flag_tag_flag> : std::true_type {};
template <> struct enable_bit_flags<tags::model_animations_animation_graph_node_tag_flag> : std::true_type {};
template <> struct enable_bit_flags<tags::model_animations_animation_tag_flag> : std::true_type {};
template <> struct enable_bit_flags<tags::model_animations_tag_flag> : std::true_type {};
template <> struct enable_bit_flags<tags::model_collision_geometry_material_tag_flag> : std::true_type {};
template <> struct enable_bit_flags<tags::model_collision_geometry_region_tag_flag> : std::true_type {};
template <> struct enable_bit_flags<tags::model_collision_geometry_bsp_leaf_tag_flag> : std::true_type {};
template <> struct enable_bit_flags<tags::model_collision_geometry_bsp_surface_tag_flag> : std::true_type {};
template <> struct enable_bit_flags<tags::model_collision_geometry_tag_flag> : std::true_type {};
template <> struct enable_bit_flags<tags::base_object_tag_flag> : std::true_type {};
template <> struct enable_bit_flags<tags::particle_tag_flag> : std::true_type {};
template <> struct enable_bit_flags<tags::particle_system_type_tag_flag> : std::true_type {};
template <> struct enable_bit_flags<tags::physics_powered_mass_point_tag_flag> : std::true_type {};
template <> struct enable_bit_flags<tags::physics_mass_point_tag_flag> : std::true_type {};
template <> struct enable_bit_flags<tags::point_physics_tag_flag> : std::true_type {};
template <> struct enable_bit_flags<tags::projectile_tag_flag> : std::true_type {};
template <> struct enable_bit_flags<tags::projectile_material_response_tag_flag> : std::true_type {};
template <> struct enable_bit_flags<tags::projectile_material_response_potential_tag_flag> : std::true_type {};
template <> struct enable_bit_flags<tags::scenario_text_tag_flag> : std::true_type {};
template <> struct enable_bit_flags<tags::scenario_function_tag_flag> : std::true_type {};
template <> struct enable_bit_flags<tags::scenario_spawn_not_placed_tag_flag> : std::true_type {};
template <> struct enable_bit_flags<tags::scenario_unit_tag_flag> : std::true_type {};
template <> struct enable_bit_flags<tags::scenario_vehicle_multiplayer_spawn_tag_flag> : std::true_type {};
template <> struct enable_bit_flags<tags::scenario_item_tag_flag> : std::true_type {};
template <> struct enable_bit_flags<tags::scenario_device_group_tag_flag> : std::true_type {};
template <> struct enable_bit_flags<tags::scenario_device_tag_flag> : std::true_type {};
template <> struct enable_bit_flags<tags::scenario_machine_tag_flag> : std::true_type {};
template <> struct enable_bit_flags<tags::scenario_control_tag_flag> : std::true_type {};
template <> struct enable_bit_flags<tags::scenario_netgame_equipment_tag_flag> : std::true_type {};
template <> struct enable_bit_flags<tags::scenario_starting_equipment_tag_flag> : std::true_type {};
template <> struct enable_bit_flags<tags::scenario_actor_starting_location_tag_flag> : std::true_type {};
template <> struct enable_bit_flags<tags::scenario_squad_tag_flag> : std::true_type {};
template <> struct enable_bit_flags<tags::scenario_squad_attacking_tag_flag> : std::true_type {};
template <> struct enable_bit_flags<tags::scenario_platoon_tag_flag> : std::true_type {};
template <> struct enable_bit_flags<tags::scenario_encounter_tag_flag> : std::true_type {};
template <> struct enable_bit_flags<tags::scenario_command_list_tag_flag> : std::true_type {};
template <> struct enable_bit_flags<tags::scenario_ai_conversation_participant_tag_flag> : std::true_type {};
template <> struct enable_bit_flags<tags::scenario_ai_conversation_line_tag_flag> : std::true_type {};
template <> struct enable_bit_flags<tags::scenario_ai_conversation_tag_flag> : std::true_type {};
template <> struct enable_bit_flags<tags::scenario_tag_flag> : std::true_type {};
template <> struct enable_bit_flags<tags::scenario_script_node_tag_flag> : std::true_type {};
template <> struct enable_bit_flags<tags::scenario_structure_bsp_material_tag_flag> : std::true_type {};
template <> struct enable_bit_flags<tags::scenario_structure_bsp_cluster_portal_tag_flag> : std::true_type {};
template <> struct enable_bit_flags<tags::shader_tag_flag> : std::true_type {};
template <> struct enable_bit_flags<tags::shader_physics_tag_flag> : std::true_type {};
template <> struct enable_bit_flags<tags::shader_environment_tag_flag> : std::true_type {};
template <> struct enable_bit_flags<tags::shader_environment_diffuse_tag_flag> : std::true_type {};
template <> struct enable_bit_flags<tags::shader_environment_specular_tag_flag> : std::true_type {};
template <> struct enable_bit_flags<tags::shader_environment_reflection_tag_flag> : std::true_type {};
template <> struct enable_bit_flags<tags::shader_model_tag_flag> : std::true_type {};
template <> struct enable_bit_flags<tags::shader_model_more_tag_flag> : std::true_type {};
template <> struct enable_bit_flags<tags::shader_transparent_chicago_map_tag_flag> : std::true_type {};
template <> struct enable_bit_flags<tags::shader_transparent_chicago_extra_tag_flag> : std::true_type {};
template <> struct enable_bit_flags<tags::shader_transparent_generic_tag_flag> : std::true_type {};
template <> struct enable_bit_flags<tags::shader_transparent_generic_map_flag_tag_flag> : std::true_type {};
template <> struct enable_bit_flags<tags::shader_transparent_generic_stage_tag_flag> : std::true_type {};
template <> struct enable_bit_flags<tags::shader_transparent_glass_tag_flag> : std::true_type {};
template <> struct enable_bit_flags<tags::shader_transparent_meter_tag_flag> : std::true_type {};
template <> struct enable_bit_flags<tags::shader_transparent_water_tag_flag> : std::true_type {};
template <> struct enable_bit_flags<tags::sky_light_tag_flag> : std::true_type {};
template <> struct enable_bit_flags<tags::sound_tag_flag> : std::true_type {};
template <> struct enable_bit_flags<tags::sound_looping_track_tag_flag> : std::true_type {};
template <> struct enable_bit_flags<tags::sound_looping_detail_tag_flag> : std::true_type {};
template <> struct enable_bit_flags<tags::sound_looping_tag_flag> : std::true_type {};
template <> struct enable_bit_flags<tags::event_handler_references_tag_flag> : std::true_type {};
template <> struct enable_bit_flags<tags::conditional_widget_reference_tag_flag> : std::true_type {};
template <> struct enable_bit_flags<tags::child_widget_reference_tag_flag> : std::true_type {};
template <> struct enable_bit_flags<tags::ui_widget_definition_tag_flag> : std::true_type {};
template <> struct enable_bit_flags<tags::ui_widget_definition_flags1_tag_flag> : std::true_type {};
template <> struct enable_bit_flags<tags::ui_widget_definition_flags2_tag_flag> : std::true_type {};
template <> struct enable_bit_flags<tags::unit_hud_interface_auxiliary_overlay_tag_flag> : std::true_type {};
template <> struct enable_bit_flags<tags::unit_hud_interface_hud_sound_latched_to_tag_flag> : std::true_type {};
template <> struct enable_bit_flags<tags::unit_hud_interface_auxiliary_panel_meter_more_tag_flag> : std::true_type {};
template <> struct enable_bit_flags<tags::vehicle_tag_flag> : std::true_type {};
template <> struct enable_bit_flags<tags::weapon_magazine_tag_flag> : std::true_type {};
template <> struct enable_bit_flags<tags::weapon_trigger_tag_flag> : std::true_type {};
template <> struct enable_bit_flags<tags::weapon_tag_flag> : std::true_type {};
template <> struct enable_bit_flags<tags::weapon_hud_interface_crosshair_type_tag_flag> : std::true_type {};
template <> struct enable_bit_flags<tags::weapon_hud_interface_number_weapon_specific_tag_flag> : std::true_type {};
template <> struct enable_bit_flags<tags::weapon_hud_interface_crosshair_overlay_tag_flag> : std::true_type {};
template <> struct enable_bit_flags<tags::weapon_hud_interface_overlay_type_tag_flag> : std::true_type {};
template <> struct enable_bit_flags<tags::weapon_hud_interface_screen_effect_definition_mask_tag_flag> : std::true_type {};
template <> struct enable_bit_flags<tags::weapon_hud_interface_screen_effect_definition_night_vision_tag_flag> : std::true_type {};
template <> struct enable_bit_flags<tags::weapon_hud_interface_screen_effect_definition_desaturation_tag_flag> : std::true_type {};
template <> struct enable_bit_flags<tags::weapon_hud_interface_tag_flag> : std::true_type {};
template <> struct enable_bit_flags<tags::weather_particle_system_particle_type_tag_flag> : std::true_type {};
}  // namespace halo
