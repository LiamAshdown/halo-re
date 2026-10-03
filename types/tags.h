#pragma once
// Generated from invader tag definitions by tools/gen_tag_header.py, then edited by hand (reconciliation
// comments and retail PC runtime fields): do NOT regenerate over it; edit this file directly.
#pragma pack(push, 1)
typedef unsigned char uint8_t; typedef signed char int8_t; typedef unsigned short uint16_t; typedef short int16_t;
typedef unsigned int uint32_t; typedef int int32_t;
typedef struct TagID { uint16_t index; uint16_t id; } TagID;
typedef struct TagString { char string[32]; } TagString;
typedef struct TagDependency { uint32_t tag_fourcc; uint32_t path_pointer; uint32_t path_size; TagID tag_id; } TagDependency;
typedef struct TagReflexive { uint32_t count; uint32_t pointer; uint32_t definition; } TagReflexive;
typedef struct TagDataOffset { uint32_t size; uint32_t flags; uint32_t file_offset; uint32_t pointer; uint32_t definition; } TagDataOffset;
typedef struct ColorARGBInt { uint8_t blue, green, red, alpha; } ColorARGBInt;
typedef struct ColorARGB { float alpha, red, green, blue; } ColorARGB;
typedef struct ColorRGB { float red, green, blue; } ColorRGB;
typedef struct Point2D { float x, y; } Point2D;
typedef struct Point3D { float x, y, z; } Point3D;
typedef struct Point2DInt { int16_t x, y; } Point2DInt;
typedef struct Vector2D { float i, j; } Vector2D;
typedef struct Vector3D { float i, j, k; } Vector3D;
typedef struct Euler2D { float yaw, pitch; } Euler2D;
typedef struct Euler3D { float yaw, pitch, roll; } Euler3D;
typedef struct Plane2D { Vector2D vector; float w; } Plane2D;
typedef struct Plane3D { Vector3D vector; float w; } Plane3D;
typedef struct Quaternion { float i, j, k, w; } Quaternion;
typedef struct Rectangle2D { int16_t top, left, bottom, right; } Rectangle2D;
typedef struct Matrix { float m[3][3]; } Matrix;
typedef enum ActorType {
    actortype_elite = 0,
    actortype_jackal = 1,
    actortype_grunt = 2,
    actortype_hunter = 3,
    actortype_engineer = 4,
    actortype_assassin = 5,
    actortype_player = 6,
    actortype_marine = 7,
    actortype_crew = 8,
    actortype_combat_form = 9,
    actortype_infection_form = 10,
    actortype_carrier_form = 11,
    actortype_monitor = 12,
    actortype_sentinel = 13,
    actortype_none = 14,
    actortype_mounted_weapon = 15,
} ActorType;  // int16
typedef int16_t ActorType_t;
typedef enum ActorUnreachableDangerTrigger {
    actorunreachabledangertrigger_never = 0,
    actorunreachabledangertrigger_visible = 1,
    actorunreachabledangertrigger_shooting = 2,
    actorunreachabledangertrigger_shooting_near_us = 3,
    actorunreachabledangertrigger_damaging_us = 4,
    actorunreachabledangertrigger_unused = 5,
    actorunreachabledangertrigger_unused1 = 6,
    actorunreachabledangertrigger_unused2 = 7,
    actorunreachabledangertrigger_unused3 = 8,
    actorunreachabledangertrigger_unused4 = 9,
} ActorUnreachableDangerTrigger;  // int16
typedef int16_t ActorUnreachableDangerTrigger_t;
typedef enum ActorDefensiveCrouchType {
    actordefensivecrouchtype_never = 0,
    actordefensivecrouchtype_danger = 1,
    actordefensivecrouchtype_low_shields = 2,
    actordefensivecrouchtype_hide_behind_shield = 3,
    actordefensivecrouchtype_any_target = 4,
    actordefensivecrouchtype_flood_shamble = 5,
} ActorDefensiveCrouchType;  // int16
typedef int16_t ActorDefensiveCrouchType_t;
typedef uint32_t ActorFlags;  // bitfield: can_see_in_darkness, sneak_uncovering_target, sneak_uncovering_pursuit_position, unused, shoot_at_target_s_last_location, try_to_stay_still_when_crouched, crouch_when_not_in_combat, crouch_when_guarding, unused_1, must_crouch_to_shoot, panic_when_surprised, always_charge_at_enemies, gets_in_vehicles_with_player, start_firing_before_aligned, standing_must_move_forward, crouching_must_move_forward, defensive_crouch_while_charging, use_stalking_behavior, stalking_freeze_if_exposed, always_berserk_in_attacking_mode, berserking_uses_panicked_movement, flying, panicked_by_unopposable_enemy, crouch_when_hiding_from_unopposable, always_charge_in_attacking_mode, dive_off_ledges, swarm, suicidal_melee_attack, cannot_move_while_crouching, fixed_crouch_facing, crouch_when_in_line_of_fire, avoid_friends_line_of_fire
typedef uint32_t ActorMoreFlags;  // bitfield: avoid_all_enemy_attack_vectors, must_stand_to_fire, must_stop_to_fire, disallow_vehicle_combat, pathfinding_ignores_danger, panic_in_groups, no_corpse_shooting
typedef struct Actor {
    ActorFlags flags;
    ActorMoreFlags more_flags;
    uint8_t _pad_8[12];
    ActorType_t type;
    uint8_t _pad_16[2];
    float max_vision_distance;
    float central_vision_angle;
    float max_vision_angle;
    uint8_t _pad_24[4];
    float peripheral_vision_angle;
    float peripheral_distance;
    uint8_t _pad_30[4];
    Vector3D standing_gun_offset;
    Vector3D crouching_gun_offset;
    float hearing_distance;
    float notice_projectile_chance;
    float notice_vehicle_chance;
    uint8_t _pad_58[8];
    float combat_perception_time;
    float guard_perception_time;
    float non_combat_perception_time;
    float inverse_combat_perception_time;
    float inverse_guard_perception_time;
    float inverse_non_combat_perception_time;
    uint8_t _pad_78[8];
    float dive_into_cover_chance;
    float emerge_from_cover_chance;
    float dive_from_grenade_chance;
    float pathfinding_radius;
    float glass_ignorance_chance;
    float stationary_movement_dist;
    float free_flying_sidestep;
    float begin_moving_angle;
    float cosine_begin_moving_angle;
    Euler2D maximum_aiming_deviation;
    Euler2D maximum_looking_deviation;
    float noncombat_look_delta_l;
    float noncombat_look_delta_r;
    float combat_look_delta_l;
    float combat_look_delta_r;
    Euler2D idle_aiming_range;
    Euler2D idle_looking_range;
    float event_look_time_modifier[2];
    float noncombat_idle_facing[2];
    float noncombat_idle_aiming[2];
    float noncombat_idle_looking[2];
    float guard_idle_facing[2];
    float guard_idle_aiming[2];
    float guard_idle_looking[2];
    float combat_idle_facing[2];
    float combat_idle_aiming[2];
    float combat_idle_looking[2];
    uint8_t _pad_124[8];
    Euler2D cosine_maximum_aiming_deviation;
    Euler2D cosine_maximum_looking_deviation;
    TagDependency do_not_use;  // weapon
    uint8_t _pad_14c[268];
    TagDependency do_not_use_1;  // projectile
    ActorUnreachableDangerTrigger_t unreachable_danger_trigger;
    ActorUnreachableDangerTrigger_t vehicle_danger_trigger;
    ActorUnreachableDangerTrigger_t player_danger_trigger;
    uint8_t _pad_26e[2];
    float danger_trigger_time[2];
    int16_t friends_killed_trigger;
    int16_t friends_retreating_trigger;
    uint8_t _pad_27c[12];
    float retreat_time[2];
    uint8_t _pad_290[8];
    float cowering_time[2];
    float friend_killed_panic_chance;
    ActorType_t leader_type;
    uint8_t _pad_2a6[2];
    float leader_killed_panic_chance;
    float panic_damage_threshold;
    float surprise_distance;
    uint8_t _pad_2b4[28];
    float hide_behind_cover_time[2];
    float hide_target_not_visible_time;
    float hide_shield_fraction;
    float attack_shield_fraction;
    float pursue_shield_fraction;
    uint8_t _pad_2e8[16];
    ActorDefensiveCrouchType_t defensive_crouch_type;
    uint8_t _pad_2fa[2];
    float attacking_crouch_threshold;
    float defending_crouch_threshold;
    float min_stand_time;
    float min_crouch_time;
    float defending_hide_time_modifier;
    float attacking_evasion_threshold;
    float defending_evasion_threshold;
    float evasion_seek_cover_chance;
    float evasion_delay_time;
    float max_seek_cover_distance;
    float cover_damage_threshold;
    float stalking_discovery_time;
    float stalking_max_distance;
    float stationary_facing_angle;
    float change_facing_stand_time;
    uint8_t _pad_338[4];
    float uncover_delay_time[2];
    float target_search_time[2];
    float pursuit_position_time[2];
    uint16_t num_positions__coord_;
    uint16_t num_positions__normal_;
    uint8_t _pad_358[32];
    float melee_attack_delay;
    float melee_fudge_factor;
    float melee_charge_time;
    float melee_leap_range[2];
    float melee_leap_velocity;
    float melee_leap_chance;
    float melee_leap_ballistic;
    float berserk_damage_amount;
    float berserk_damage_threshold;
    float berserk_proximity;
    float suicide_sensing_dist;
    float berserk_grenade_chance;
    uint8_t _pad_3ac[12];
    float guard_position_time[2];
    float combat_position_time[2];
    float old_position_avoid_dist;
    float friend_avoid_dist;
    uint8_t _pad_3d0[40];
    float noncombat_idle_speech_time[2];
    float combat_idle_speech_time[2];
    uint8_t _pad_408[48];
    uint8_t _pad_438[128];
    TagDependency do_not_use_2;  // actor
    uint8_t _pad_4c8[48];
} Actor;  // size 0x4f8
typedef enum ActorVariantMovementType {
    actorvariantmovementtype_always_run = 0,
    actorvariantmovementtype_always_crouch = 1,
    actorvariantmovementtype_switch_types = 2,
} ActorVariantMovementType;  // int16
typedef int16_t ActorVariantMovementType_t;
typedef enum ActorVariantSpecialFireMode {
    actorvariantspecialfiremode_none = 0,
    actorvariantspecialfiremode_overcharge = 1,
    actorvariantspecialfiremode_secondary_trigger = 2,
} ActorVariantSpecialFireMode;  // int16
typedef int16_t ActorVariantSpecialFireMode_t;
typedef enum ActorVariantSpecialFireSituation {
    actorvariantspecialfiresituation_never = 0,
    actorvariantspecialfiresituation_enemy_visible = 1,
    actorvariantspecialfiresituation_enemy_out_of_sight = 2,
    actorvariantspecialfiresituation_strafing = 3,
} ActorVariantSpecialFireSituation;  // int16
typedef int16_t ActorVariantSpecialFireSituation_t;
typedef enum ActorVariantTrajectoryType {
    actorvarianttrajectorytype_toss = 0,
    actorvarianttrajectorytype_lob = 1,
    actorvarianttrajectorytype_bounce = 2,
} ActorVariantTrajectoryType;  // int16
typedef int16_t ActorVariantTrajectoryType_t;
typedef enum ActorVariantGrenadeStimulus {
    actorvariantgrenadestimulus_never = 0,
    actorvariantgrenadestimulus_visible_target = 1,
    actorvariantgrenadestimulus_seek_cover = 2,
} ActorVariantGrenadeStimulus;  // int16
typedef int16_t ActorVariantGrenadeStimulus_t;
typedef uint32_t ActorVariantFlags;  // bitfield: can_shoot_while_flying, interpolate_color_in_hsv, has_unlimited_grenades, movement_switching_try_to_stay_with_friends, active_camouflage, super_active_camouflage, cannot_use_ranged_weapons, prefer_passenger_seat
typedef struct ActorVariantChangeColors {
    ColorRGB color_lower_bound;
    ColorRGB color_upper_bound;
    uint8_t _pad_18[8];
} ActorVariantChangeColors;  // size 0x20
typedef enum MetagameType {
    metagametype_brute = 0,
    metagametype_grunt = 1,
    metagametype_jackal = 2,
    metagametype_skirmisher = 3,
    metagametype_marine = 4,
    metagametype_spartan = 5,
    metagametype_bugger = 6,
    metagametype_hunter = 7,
    metagametype_flood_infection = 8,
    metagametype_flood_carrier = 9,
    metagametype_flood_combat = 10,
    metagametype_flood_pure = 11,
    metagametype_sentinel = 12,
    metagametype_elite = 13,
    metagametype_engineer = 14,
    metagametype_mule = 15,
    metagametype_turret = 16,
    metagametype_mongoose = 17,
    metagametype_warthog = 18,
    metagametype_scorpion = 19,
    metagametype_hornet = 20,
    metagametype_pelican = 21,
    metagametype_revenant = 22,
    metagametype_seraph = 23,
    metagametype_shade = 24,
    metagametype_watchtower = 25,
    metagametype_ghost = 26,
    metagametype_chopper = 27,
    metagametype_mauler = 28,
    metagametype_wraith = 29,
    metagametype_banshee = 30,
    metagametype_phantom = 31,
    metagametype_scarab = 32,
    metagametype_guntower = 33,
    metagametype_tuning_fork = 34,
    metagametype_broadsword = 35,
    metagametype_mammoth = 36,
    metagametype_lich = 37,
    metagametype_mantis = 38,
    metagametype_wasp = 39,
    metagametype_phaeton = 40,
    metagametype_bishop = 41,
    metagametype_knight = 42,
    metagametype_pawn = 43,
} MetagameType;  // int16
typedef int16_t MetagameType_t;
typedef enum MetagameClass {
    metagameclass_infantry = 0,
    metagameclass_leader = 1,
    metagameclass_hero = 2,
    metagameclass_specialist = 3,
    metagameclass_light_vehicle = 4,
    metagameclass_heavy_vehicle = 5,
    metagameclass_giant_vehicle = 6,
    metagameclass_standard_vehicle = 7,
} MetagameClass;  // int16
typedef int16_t MetagameClass_t;
typedef enum GrenadeType {
    grenadetype_human_fragmentation = 0,
    grenadetype_covenant_plasma = 1,
    grenadetype_grenade_type_2 = 2,
    grenadetype_grenade_type_3 = 3,
} GrenadeType;  // int16
typedef int16_t GrenadeType_t;
typedef struct ActorVariant {
    ActorVariantFlags flags;
    TagDependency actor_definition;  // actor
    TagDependency unit;  // unit
    TagDependency major_variant;  // actor_variant
    MetagameType_t metagame_type;
    MetagameClass_t metagame_class;
    uint8_t _pad_38[20];
    ActorVariantMovementType_t movement_type;
    uint8_t _pad_4e[2];
    float initial_crouch_chance;
    float crouch_time[2];
    float run_time[2];
    TagDependency weapon;  // weapon
    float maximum_firing_distance;
    float rate_of_fire;
    float projectile_error;
    float first_burst_delay_time[2];
    float new_target_firing_pattern_time;
    float surprise_delay_time;
    float surprise_fire_wildly_time;
    float death_fire_wildly_chance;
    float death_fire_wildly_time;
    float desired_combat_range[2];
    Vector3D custom_stand_gun_offset;
    Vector3D custom_crouch_gun_offset;
    float target_tracking;
    float target_leading;
    float weapon_damage_modifier;
    float damage_per_second;
    float burst_origin_radius;
    float burst_origin_angle;
    float burst_return_length[2];
    float burst_return_angle;
    float burst_duration[2];
    float burst_separation[2];
    float burst_angular_velocity;
    uint8_t _pad_f4[4];
    float special_damage_modifier;
    float special_projectile_error;
    float new_target_burst_duration;
    float new_target_burst_separation;
    float new_target_rate_of_fire;
    float new_target_projectile_error;
    uint8_t _pad_110[8];
    float moving_burst_duration;
    float moving_burst_separation;
    float moving_rate_of_fire;
    float moving_projectile_error;
    uint8_t _pad_128[8];
    float berserk_burst_duration;
    float berserk_burst_separation;
    float berserk_rate_of_fire;
    float berserk_projectile_error;
    uint8_t _pad_140[8];
    float super_ballistic_range;
    float bombardment_range;
    float modified_vision_range;
    ActorVariantSpecialFireMode_t special_fire_mode;
    ActorVariantSpecialFireSituation_t special_fire_situation;
    float special_fire_chance;
    float special_fire_delay;
    float melee_range;
    float melee_abort_range;
    float berserk_firing_ranges[2];
    float berserk_melee_range;
    float berserk_melee_abort_range;
    uint8_t _pad_178[8];
    GrenadeType_t grenade_type;
    ActorVariantTrajectoryType_t trajectory_type;
    ActorVariantGrenadeStimulus_t grenade_stimulus;
    int16_t minimum_enemy_count;
    float enemy_radius;
    uint8_t _pad_18c[4];
    float grenade_velocity;
    float grenade_ranges[2];
    float collateral_damage_radius;
    float grenade_chance;
    float grenade_check_time;
    float encounter_grenade_timeout;
    uint8_t _pad_1ac[20];
    TagDependency equipment;  // equipment
    int16_t grenade_count[2];
    float don_t_drop_grenades_chance;
    float drop_weapon_loaded[2];
    int16_t drop_weapon_ammo[2];
    uint8_t _pad_1e4[12];
    uint8_t _pad_1f0[16];
    float body_vitality;
    float shield_vitality;
    float shield_sapping_radius;
    uint16_t forced_shader_permutation;
    uint8_t _pad_20e[2];
    uint8_t _pad_210[16];
    uint8_t _pad_220[12];
    TagReflexive change_colors;  // ActorVariantChangeColors
} ActorVariant;  // size 0x238
typedef struct AntennaVertex {
    float spring_strength_coefficient;
    uint8_t _pad_4[24];
    Euler2D angles;
    float length;
    uint16_t sequence_index;
    uint8_t _pad_2a[2];
    ColorARGB color;
    ColorARGB lod_color;
    uint8_t _pad_4c[40];
    Point3D offset;
} AntennaVertex;  // size 0x80
typedef struct Antenna {
    TagString attachment_marker_name;
    TagDependency bitmaps;  // bitmap
    TagDependency physics;  // point_physics
    uint8_t _pad_40[80];
    float spring_strength_coefficient;
    float falloff_pixels;
    float cutoff_pixels;
    float length;
    uint8_t _pad_a0[36];
    TagReflexive vertices;  // AntennaVertex
} Antenna;  // size 0xd0
typedef enum BipedFunctionIn {
    bipedfunctionin_none = 0,
    bipedfunctionin_flying_velocity = 1,
} BipedFunctionIn;  // int16
typedef int16_t BipedFunctionIn_t;
typedef uint32_t BipedFlags;  // bitfield: turns_without_animating, uses_player_physics, flying, physics_pill_centered_at_origin, spherical, passes_through_other_bipeds, can_climb_any_surface, immune_to_falling_damage, rotate_while_airborne, uses_limp_body_physics, has_no_dying_airborne, random_speed_increase, unit_uses_old_ntsc_player_physics
typedef struct BipedContactPoint {
    uint8_t _pad_0[32];
    TagString marker_name;
} BipedContactPoint;  // size 0x40
typedef enum ObjectType {
    objecttype_biped = 0,
    objecttype_vehicle = 1,
    objecttype_weapon = 2,
    objecttype_equipment = 3,
    objecttype_garbage = 4,
    objecttype_projectile = 5,
    objecttype_scenery = 6,
    objecttype_device_machine = 7,
    objecttype_device_control = 8,
    objecttype_device_light_fixture = 9,
    objecttype_placeholder = 10,
    objecttype_sound_scenery = 11,
} ObjectType;  // int16
typedef int16_t ObjectType_t;
typedef uint16_t ObjectFlags;  // bitfield: does_not_cast_shadow, transparent_self_occlusion, brighter_than_it_should_be, not_a_pathfinding_obstacle, extension_of_parent, cast_shadow_by_default, does_not_have_anniversary_geometry
typedef enum ObjectFunctionIn {
    objectfunctionin_none = 0,
    objectfunctionin_body_vitality = 1,
    objectfunctionin_shield_vitality = 2,
    objectfunctionin_recent_body_damage = 3,
    objectfunctionin_recent_shield_damage = 4,
    objectfunctionin_random_constant = 5,
    objectfunctionin_umbrella_shield_vitality = 6,
    objectfunctionin_shield_stun = 7,
    objectfunctionin_recent_umbrella_shield_vitality = 8,
    objectfunctionin_umbrella_shield_stun = 9,
    objectfunctionin_region = 10,
    objectfunctionin_region_1 = 11,
    objectfunctionin_region_2 = 12,
    objectfunctionin_region_3 = 13,
    objectfunctionin_region_4 = 14,
    objectfunctionin_region_5 = 15,
    objectfunctionin_region_6 = 16,
    objectfunctionin_region_7 = 17,
    objectfunctionin_alive = 18,
    objectfunctionin_compass = 19,
} ObjectFunctionIn;  // int16
typedef int16_t ObjectFunctionIn_t;
typedef enum FunctionOut {
    functionout_none = 0,
    functionout_a_out = 1,
    functionout_b_out = 2,
    functionout_c_out = 3,
    functionout_d_out = 4,
} FunctionOut;  // int16
typedef int16_t FunctionOut_t;
typedef enum FunctionNameNullable {
    functionnamenullable_none = 0,
    functionnamenullable_a = 1,
    functionnamenullable_b = 2,
    functionnamenullable_c = 3,
    functionnamenullable_d = 4,
} FunctionNameNullable;  // int16
typedef int16_t FunctionNameNullable_t;
typedef struct ObjectAttachment {
    TagDependency type;  // light,light_volume,contrail,particle_system,effect,sound_looping
    TagString marker;
    FunctionOut_t primary_scale;
    FunctionOut_t secondary_scale;
    FunctionNameNullable_t change_color;
    uint8_t _pad_36[2];
    uint8_t _pad_38[16];
} ObjectAttachment;  // size 0x48
typedef struct ObjectWidget {
    TagDependency reference;  // antenna,glow,light_volume,lightning,flag
    uint8_t _pad_10[16];
} ObjectWidget;  // size 0x20
typedef uint32_t ObjectFunctionFlags;  // bitfield: invert, additive, always_active
typedef enum FunctionScaleBy {
    functionscaleby_none = 0,
    functionscaleby_a_in = 1,
    functionscaleby_b_in = 2,
    functionscaleby_c_in = 3,
    functionscaleby_d_in = 4,
    functionscaleby_a_out = 5,
    functionscaleby_b_out = 6,
    functionscaleby_c_out = 7,
    functionscaleby_d_out = 8,
} FunctionScaleBy;  // int16
typedef int16_t FunctionScaleBy_t;
typedef enum WaveFunction {
    wavefunction_one = 0,
    wavefunction_zero = 1,
    wavefunction_cosine = 2,
    wavefunction_cosine_variable_period = 3,
    wavefunction_diagonal_wave = 4,
    wavefunction_diagonal_wave_variable_period = 5,
    wavefunction_slide = 6,
    wavefunction_slide_variable_period = 7,
    wavefunction_noise = 8,
    wavefunction_jitter = 9,
    wavefunction_wander = 10,
    wavefunction_spark = 11,
} WaveFunction;  // int16
typedef int16_t WaveFunction_t;
typedef enum FunctionType {
    functiontype_linear = 0,
    functiontype_early = 1,
    functiontype_very_early = 2,
    functiontype_late = 3,
    functiontype_very_late = 4,
    functiontype_cosine = 5,
} FunctionType;  // int16
typedef int16_t FunctionType_t;
typedef enum FunctionBoundsMode {
    functionboundsmode_clip = 0,
    functionboundsmode_clip_and_normalize = 1,
    functionboundsmode_scale_to_fit = 2,
} FunctionBoundsMode;  // int16
typedef int16_t FunctionBoundsMode_t;
typedef struct ObjectFunction {
    ObjectFunctionFlags flags;
    float period;
    FunctionScaleBy_t scale_period_by;
    WaveFunction_t function;
    FunctionScaleBy_t scale_function_by;
    WaveFunction_t wobble_function;
    float wobble_period;
    float wobble_magnitude;
    float square_wave_threshold;
    int16_t step_count;
    FunctionType_t map_to;
    int16_t sawtooth_count;
    FunctionScaleBy_t add;
    FunctionScaleBy_t scale_result_by;
    FunctionBoundsMode_t bounds_mode;
    float bounds[2];
    uint8_t _pad_30[4];
    uint8_t _pad_34[2];
    int16_t turn_off_with;
    float scale_by;
    uint8_t _pad_3c[252];
    float inverse_bounds;
    float inverse_sawtooth;
    float inverse_step;
    float inverse_period;
    TagString usage;
} ObjectFunction;  // size 0x168
typedef uint32_t ColorInterpolationFlags;  // bitfield: blend_in_hsv, more_colors
typedef struct ObjectChangeColorsPermutation {
    float weight;
    ColorRGB color_lower_bound;
    ColorRGB color_upper_bound;
} ObjectChangeColorsPermutation;  // size 0x1c
typedef struct ObjectChangeColors {
    FunctionScaleBy_t darken_by;
    FunctionScaleBy_t scale_by;
    ColorInterpolationFlags flags;
    ColorRGB color_lower_bound;
    ColorRGB color_upper_bound;
    TagReflexive permutations;  // ObjectChangeColorsPermutation
} ObjectChangeColors;  // size 0x2c
typedef enum PredictedResourceType {
    predictedresourcetype_bitmap = 0,
    predictedresourcetype_sound = 1,
} PredictedResourceType;  // int16
typedef int16_t PredictedResourceType_t;
typedef struct PredictedResource {
    PredictedResourceType_t type;
    uint16_t resource_index;
    TagID tag;
} PredictedResource;  // size 0x8
typedef struct Object {
    ObjectType_t object_type;
    ObjectFlags flags;
    float bounding_radius;
    Point3D bounding_offset;
    Point3D origin_offset;
    float acceleration_scale;
    uint32_t scales_change_colors;
    TagDependency model;  // model
    TagDependency animation_graph;  // model_animations
    uint8_t _pad_48[40];
    TagDependency collision_model;  // model_collision_geometry
    TagDependency physics;  // physics
    TagDependency modifier_shader;  // shader
    TagDependency creation_effect;  // effect
    uint8_t _pad_b0[84];
    float render_bounding_radius;
    ObjectFunctionIn_t a_in;
    ObjectFunctionIn_t b_in;
    ObjectFunctionIn_t c_in;
    ObjectFunctionIn_t d_in;
    uint8_t _pad_110[44];
    int16_t hud_text_message_index;
    int16_t forced_shader_permutation_index;
    TagReflexive attachments;  // ObjectAttachment
    TagReflexive widgets;  // ObjectWidget
    TagReflexive functions;  // ObjectFunction
    TagReflexive change_colors;  // ObjectChangeColors
    TagReflexive predicted_resources;  // PredictedResource
} Object;  // size 0x17c
typedef uint32_t UnitFlags;  // bitfield: circular_aiming, destroyed_after_dying, half_speed_interpolation, fires_from_camera, entrance_inside_bounding_sphere, unused, causes_passenger_dialogue, resists_pings, melee_attack_is_fatal, don_t_reface_during_pings, has_no_aiming, simple_creature, impact_melee_attaches_to_unit, impact_melee_dies_on_shields, cannot_open_doors_automatically, melee_attackers_cannot_attach, not_instantly_killed_by_melee, shield_sapping, runs_around_flaming, inconsequential, special_cinematic_unit, ignored_by_autoaiming, shields_fry_infection_forms, integrated_light_cntrls_weapon, integrated_light_lasts_forever
typedef enum UnitDefaultTeam {
    unitdefaultteam_none = 0,
    unitdefaultteam_player = 1,
    unitdefaultteam_human = 2,
    unitdefaultteam_covenant = 3,
    unitdefaultteam_flood = 4,
    unitdefaultteam_sentinel = 5,
    unitdefaultteam_unused6 = 6,
    unitdefaultteam_unused7 = 7,
    unitdefaultteam_unused8 = 8,
    unitdefaultteam_unused9 = 9,
} UnitDefaultTeam;  // int16
typedef int16_t UnitDefaultTeam_t;
typedef enum ObjectNoise {
    objectnoise_silent = 0,
    objectnoise_medium = 1,
    objectnoise_loud = 2,
    objectnoise_shout = 3,
    objectnoise_quiet = 4,
} ObjectNoise;  // int16
typedef int16_t ObjectNoise_t;
typedef enum UnitFunctionIn {
    unitfunctionin_none = 0,
    unitfunctionin_driver_seat_power = 1,
    unitfunctionin_gunner_seat_power = 2,
    unitfunctionin_aiming_change = 3,
    unitfunctionin_mouth_aperture = 4,
    unitfunctionin_integrated_light_power = 5,
    unitfunctionin_can_blink = 6,
    unitfunctionin_shield_sapping = 7,
} UnitFunctionIn;  // int16
typedef int16_t UnitFunctionIn_t;
typedef struct UnitCameraTrack {
    TagDependency track;  // camera_track
    uint8_t _pad_10[12];
} UnitCameraTrack;  // size 0x1c
typedef enum UnitMotionSensorBlipSize {
    unitmotionsensorblipsize_medium = 0,
    unitmotionsensorblipsize_small = 1,
    unitmotionsensorblipsize_large = 2,
} UnitMotionSensorBlipSize;  // int16
typedef int16_t UnitMotionSensorBlipSize_t;
typedef struct UnitUnitHudInterface {
    TagDependency hud;  // unit_hud_interface
    uint8_t _pad_10[32];
} UnitUnitHudInterface;  // size 0x30
typedef struct UnitDialogueVariant {
    int16_t variant_number;
    uint8_t _pad_2[2];
    uint8_t _pad_4[4];
    TagDependency dialogue;  // dialogue
} UnitDialogueVariant;  // size 0x18
typedef struct UnitPoweredSeat {
    uint8_t _pad_0[4];
    float driver_powerup_time;
    float driver_powerdown_time;
    uint8_t _pad_c[56];
} UnitPoweredSeat;  // size 0x44
typedef struct UnitWeapon {
    TagDependency weapon;  // weapon
    uint8_t _pad_10[20];
} UnitWeapon;  // size 0x24
typedef uint32_t UnitSeatFlags;  // bitfield: invisible, locked, driver, gunner, third_person_camera, allows_weapons, third_person_on_enter, first_person_camera_slaved_to_gun, allow_vehicle_communication_animations, not_valid_without_driver, allow_ai_noncombatants
typedef struct UnitSeat {
    UnitSeatFlags flags;
    TagString label;
    TagString marker_name;
    uint8_t _pad_44[32];
    Vector3D acceleration_scale;
    uint8_t _pad_70[12];
    float yaw_rate;
    float pitch_rate;
    TagString camera_marker_name;
    TagString camera_submerged_marker_name;
    float pitch_auto_level;
    float pitch_range[2];
    TagReflexive camera_tracks;  // UnitCameraTrack
    TagReflexive unit_hud_interface;  // UnitUnitHudInterface
    uint8_t _pad_e8[4];
    uint16_t hud_text_message_index;
    uint8_t _pad_ee[2];
    float yaw_minimum;
    float yaw_maximum;
    TagDependency built_in_gunner;  // actor_variant
    uint8_t _pad_108[20];
} UnitSeat;  // size 0x11c
typedef struct Unit {
    Object base;  // inherits
    UnitFlags unit_flags;
    UnitDefaultTeam_t default_team;
    ObjectNoise_t constant_sound_volume;
    float rider_damage_fraction;
    TagDependency integrated_light_toggle;  // effect
    UnitFunctionIn_t unit_a_in;
    UnitFunctionIn_t unit_b_in;
    UnitFunctionIn_t unit_c_in;
    UnitFunctionIn_t unit_d_in;
    float camera_field_of_view;
    float camera_stiffness;
    TagString camera_marker_name;
    TagString camera_submerged_marker_name;
    float pitch_auto_level;
    float pitch_range[2];
    TagReflexive camera_tracks;  // UnitCameraTrack
    Point3D seat_acceleration_scale;
    uint8_t _pad_20c[12];
    float soft_ping_threshold;
    float soft_ping_interrupt_time;
    float hard_ping_threshold;
    float hard_ping_interrupt_time;
    float hard_death_threshold;
    float feign_death_threshold;
    float feign_death_time;
    float distance_of_evade_anim;
    float distance_of_dive_anim;
    uint8_t _pad_23c[4];
    float stunned_movement_threshold;
    float feign_death_chance;
    float feign_repeat_chance;
    TagDependency spawned_actor;  // actor_variant
    int16_t spawned_actor_count[2];
    float spawned_velocity;
    float aiming_velocity_maximum;
    float aiming_acceleration_maximum;
    float casual_aiming_modifier;
    float looking_velocity_maximum;
    float looking_acceleration_maximum;
    uint8_t _pad_278[8];
    float ai_vehicle_radius;
    float ai_danger_radius;
    TagDependency melee_damage;  // damage_effect
    UnitMotionSensorBlipSize_t motion_sensor_blip_size;
    uint8_t _pad_29a[2];
    MetagameType_t metagame_type;
    MetagameClass_t metagame_class;
    uint8_t _pad_2a0[8];
    TagReflexive new_hud_interfaces;  // UnitUnitHudInterface
    TagReflexive dialogue_variants;  // UnitDialogueVariant
    float grenade_velocity;
    GrenadeType_t grenade_type;
    int16_t grenade_count;
    int16_t soft_ping_interrupt_ticks;
    int16_t hard_ping_interrupt_ticks;
    TagReflexive powered_seats;  // UnitPoweredSeat
    TagReflexive weapons;  // UnitWeapon
    TagReflexive seats;  // UnitSeat
} Unit;  // size 0x2f0
typedef struct Biped {
    Unit base;  // inherits
    float moving_turning_speed;
    BipedFlags biped_flags;
    float stationary_turning_threshold;
    uint8_t _pad_2fc[16];
    BipedFunctionIn_t biped_a_in;
    BipedFunctionIn_t biped_b_in;
    BipedFunctionIn_t biped_c_in;
    BipedFunctionIn_t biped_d_in;
    TagDependency don_t_use;  // damage_effect
    float bank_angle;
    float bank_apply_time;
    float bank_decay_time;
    float pitch_ratio;
    float max_velocity;
    float max_sidestep_velocity;
    float acceleration;
    float deceleration;
    float angular_velocity_maximum;
    float angular_acceleration_maximum;
    float crouch_velocity_modifier;
    uint8_t _pad_350[8];
    float maximum_slope_angle;
    float downhill_falloff_angle;
    float downhill_cutoff_angle;
    float downhill_velocity_scale;
    float uphill_falloff_angle;
    float uphill_cutoff_angle;
    float uphill_velocity_scale;
    uint8_t _pad_374[24];
    TagDependency footsteps;  // material_effects
    uint8_t _pad_39c[24];
    float jump_velocity;
    uint8_t _pad_3b8[28];
    float maximum_soft_landing_time;
    float maximum_hard_landing_time;
    float minimum_soft_landing_velocity;
    float minimum_hard_landing_velocity;
    float maximum_hard_landing_velocity;
    float death_hard_landing_velocity;
    uint8_t _pad_3ec[20];
    float standing_camera_height;
    float crouching_camera_height;
    float crouch_transition_time;
    uint8_t _pad_40c[24];
    float standing_collision_height;
    float crouching_collision_height;
    float collision_radius;
    uint8_t _pad_430[40];
    float autoaim_width;
    uint8_t _pad_45c[108];
    float cosine_stationary_turning_threshold;
    float crouch_camera_velocity;
    float cosine_maximum_slope_angle;
    float negative_sine_downhill_falloff_angle;
    float negative_sine_downhill_cutoff_angle;
    float sine_uphill_falloff_angle;
    float sine_uphill_cutoff_angle;
    uint16_t pelvis_model_node_index;
    uint16_t head_model_node_index;
    TagReflexive contact_point;  // BipedContactPoint
} Biped;  // size 0x4f4
typedef uint32_t IsUnusedFlag;  // bitfield: unused
typedef uint16_t IsUnfilteredFlag;  // bitfield: unfiltered
typedef enum BitmapDataType {
    bitmapdatatype_2d_texture = 0,
    bitmapdatatype_3d_texture = 1,
    bitmapdatatype_cube_map = 2,
    bitmapdatatype_white = 3,
} BitmapDataType;  // int16
typedef int16_t BitmapDataType_t;
typedef enum BitmapDataFormat {
    bitmapdataformat_a8 = 0,
    bitmapdataformat_y8 = 1,
    bitmapdataformat_ay8 = 2,
    bitmapdataformat_a8y8 = 3,
    bitmapdataformat_unused1 = 4,
    bitmapdataformat_unused2 = 5,
    bitmapdataformat_r5g6b5 = 6,
    bitmapdataformat_unused3 = 7,
    bitmapdataformat_a1r5g5b5 = 8,
    bitmapdataformat_a4r4g4b4 = 9,
    bitmapdataformat_x8r8g8b8 = 10,
    bitmapdataformat_a8r8g8b8 = 11,
    bitmapdataformat_unused4 = 12,
    bitmapdataformat_unused5 = 13,
    bitmapdataformat_dxt1 = 14,
    bitmapdataformat_dxt3 = 15,
    bitmapdataformat_dxt5 = 16,
    bitmapdataformat_p8_bump = 17,
    bitmapdataformat_bc7 = 18,
} BitmapDataFormat;  // int16
typedef int16_t BitmapDataFormat_t;
typedef enum BitmapType {
    bitmaptype_2d_textures = 0,
    bitmaptype_3d_textures = 1,
    bitmaptype_cube_maps = 2,
    bitmaptype_sprites = 3,
    bitmaptype_interface_bitmaps = 4,
} BitmapType;  // int16
typedef int16_t BitmapType_t;
typedef enum BitmapFormat {
    bitmapformat_dxt1 = 0,
    bitmapformat_dxt3 = 1,
    bitmapformat_dxt5 = 2,
    bitmapformat_16_bit = 3,
    bitmapformat_32_bit = 4,
    bitmapformat_monochrome = 5,
    bitmapformat_bc7 = 6,
} BitmapFormat;  // int16
typedef int16_t BitmapFormat_t;
typedef enum BitmapUsage {
    bitmapusage_alpha_blend = 0,
    bitmapusage_default = 1,
    bitmapusage_height_map = 2,
    bitmapusage_detail_map = 3,
    bitmapusage_light_map = 4,
    bitmapusage_vector_map = 5,
} BitmapUsage;  // int16
typedef int16_t BitmapUsage_t;
typedef enum BitmapSpriteBudgetSize {
    bitmapspritebudgetsize_32x32 = 0,
    bitmapspritebudgetsize_64x64 = 1,
    bitmapspritebudgetsize_128x128 = 2,
    bitmapspritebudgetsize_256x256 = 3,
    bitmapspritebudgetsize_512x512 = 4,
    bitmapspritebudgetsize_1024x1024 = 5,
} BitmapSpriteBudgetSize;  // int16
typedef int16_t BitmapSpriteBudgetSize_t;
typedef enum BitmapSpriteUsage {
    bitmapspriteusage_blend_add_subtract_max = 0,
    bitmapspriteusage_multiply_min = 1,
    bitmapspriteusage_double_multiply = 2,
} BitmapSpriteUsage;  // int16
typedef int16_t BitmapSpriteUsage_t;
typedef uint16_t BitmapDataFlags;  // bitfield: power_of_two_dimensions, compressed, palettized, swizzled, linear, v16u16, unused, make_it_actually_work, external, environment
typedef uint16_t BitmapFlags;  // bitfield: enable_diffusion_dithering, disable_height_map_compression, uniform_sprite_sequences, filthy_sprite_bug_fix, half_hud_scale, invert_detail_fade, use_average_color_for_detail_fade, force_hud_use_highres_scale
typedef struct BitmapGroupSprite {
    uint16_t bitmap_index;
    uint8_t _pad_2[2];
    uint8_t _pad_4[4];
    float left;
    float right;
    float top;
    float bottom;
    Point2D registration_point;
} BitmapGroupSprite;  // size 0x20
typedef struct BitmapGroupSequence {
    TagString name;
    uint16_t first_bitmap_index;
    uint16_t bitmap_count;
    uint8_t _pad_24[16];
    TagReflexive sprites;  // BitmapGroupSprite
} BitmapGroupSequence;  // size 0x40
typedef struct BitmapData {
    uint32_t bitmap_class;
    uint16_t width;
    uint16_t height;
    uint16_t depth;
    BitmapDataType_t type;
    BitmapDataFormat_t format;
    BitmapDataFlags flags;
    Point2DInt registration_point;
    uint16_t mipmap_count;
    uint8_t _pad_16[2];
    uint32_t pixel_data_offset;
    uint32_t pixel_data_size;
    TagID bitmap_tag_id;
    uint32_t pointer;
    uint32_t hardware_texture;  // retail PC runtime: IDirect3DBaseTexture9 * (0x524173 locks it, 0x524133 tests it)
    void *pixel_base;  // retail PC runtime: the pixel memory (the texture cache's GlobalAlloc'd staging buffer, freed on release)
} BitmapData;  // size 0x30
typedef struct Bitmap {
    BitmapType_t type;
    BitmapFormat_t encoding_format;
    BitmapUsage_t usage;
    BitmapFlags flags;
    float detail_fade_factor;
    float sharpen_amount;
    float bump_height;
    BitmapSpriteBudgetSize_t sprite_budget_size;
    uint16_t sprite_budget_count;
    uint16_t color_plate_width;
    uint16_t color_plate_height;
    TagDataOffset compressed_color_plate_data;
    TagDataOffset processed_pixel_data;
    float blur_filter_size;
    float alpha_bias;
    uint16_t mipmap_count;
    BitmapSpriteUsage_t sprite_usage;
    uint16_t sprite_spacing;
    uint8_t _pad_52[2];
    TagReflexive bitmap_group_sequence;  // BitmapGroupSequence
    TagReflexive bitmap_data;  // BitmapData
} Bitmap;  // size 0x6c
typedef struct CameraTrackControlPoint {
    Point3D position;
    Quaternion orientation;
    uint8_t _pad_1c[32];
} CameraTrackControlPoint;  // size 0x3c
typedef struct CameraTrack {
    IsUnusedFlag flags;
    TagReflexive control_points;  // CameraTrackControlPoint
    uint8_t _pad_10[32];
} CameraTrack;  // size 0x30
typedef struct ColorTableColor {
    TagString name;
    ColorARGB color;
} ColorTableColor;  // size 0x30
typedef struct ColorTable {
    TagReflexive colors;  // ColorTableColor
} ColorTable;  // size 0xc
typedef enum DamageEffectSideEffect {
    damageeffectsideeffect_none = 0,
    damageeffectsideeffect_harmless = 1,
    damageeffectsideeffect_lethal_to_the_unsuspecting = 2,
    damageeffectsideeffect_emp = 3,
} DamageEffectSideEffect;  // int16
typedef int16_t DamageEffectSideEffect_t;
typedef enum DamageEffectCategory {
    damageeffectcategory_none = 0,
    damageeffectcategory_falling = 1,
    damageeffectcategory_bullet = 2,
    damageeffectcategory_grenade = 3,
    damageeffectcategory_high_explosive = 4,
    damageeffectcategory_sniper = 5,
    damageeffectcategory_melee = 6,
    damageeffectcategory_flame = 7,
    damageeffectcategory_mounted_weapon = 8,
    damageeffectcategory_vehicle = 9,
    damageeffectcategory_plasma = 10,
    damageeffectcategory_needle = 11,
    damageeffectcategory_shotgun = 12,
} DamageEffectCategory;  // int16
typedef int16_t DamageEffectCategory_t;
typedef uint32_t DamageEffectDamageFlags;  // bitfield: does_not_hurt_owner, can_cause_headshots, pings_resistant_units, does_not_hurt_friends, does_not_ping_units, detonates_explosives, only_hurts_shields, causes_flaming_death, damage_indicators_always_point_down, skips_shields, only_hurts_one_infection_form, can_cause_multiplayer_headshots, infection_form_pop, ignore_seat_scale_for_dir_dmg, forces_hard_ping, does_not_hurt_players, use_3d_instantaneous_acceleration, allow_any_non_zero_acceleration_value
typedef struct ContinuousDamageEffect {
    float radius[2];
    float cutoff_scale;
    uint8_t _pad_c[24];
    float low_frequency_vibrate_frequency;
    float high_frequency_vibrate_frequency;
    uint8_t _pad_2c[24];
    float camera_shaking_random_translation;
    float camera_shaking_random_rotation;
    uint8_t _pad_4c[12];
    WaveFunction_t camera_shaking_wobble_function;
    uint8_t _pad_5a[2];
    float camera_shaking_wobble_period;
    float camera_shaking_wobble_weight;
    uint8_t _pad_64[4];
    uint8_t _pad_68[20];
    uint8_t _pad_7c[8];
    uint8_t _pad_84[160];
    DamageEffectSideEffect_t damage_side_effect;
    DamageEffectCategory_t damage_category;
    DamageEffectDamageFlags damage_flags;
    uint8_t _pad_12c[4];
    float damage_lower_bound;
    float damage_upper_bound[2];
    float damage_vehicle_passthrough_penalty;
    uint8_t _pad_140[4];
    float damage_stun;
    float damage_maximum_stun;
    float damage_stun_time;
    uint8_t _pad_150[4];
    Vector3D damage_instantaneous_acceleration;
    float dirt;
    float sand;
    float stone;
    float snow;
    float wood;
    float metal_hollow;
    float metal_thin;
    float metal_thick;
    float rubber;
    float glass;
    float force_field;
    float grunt;
    float hunter_armor;
    float hunter_skin;
    float elite;
    float jackal;
    float jackal_energy_shield;
    float engineer_skin;
    float engineer_force_field;
    float flood_combat_form;
    float flood_carrier_form;
    float cyborg_armor;
    float cyborg_energy_shield;
    float human_armor;
    float human_skin;
    float sentinel;
    float monitor;
    float plastic;
    float water;
    float leaves;
    float elite_energy_shield;
    float ice;
    float hunter_shield;
    uint8_t _pad_1e4[28];
} ContinuousDamageEffect;  // size 0x200
typedef enum ContrailRenderType {
    contrailrendertype_vertical_orientation = 0,
    contrailrendertype_horizontal_orientation = 1,
    contrailrendertype_media_mapped = 2,
    contrailrendertype_ground_mapped = 3,
    contrailrendertype_viewer_facing = 4,
    contrailrendertype_double_marker_linked = 5,
} ContrailRenderType;  // int16
typedef int16_t ContrailRenderType_t;
typedef uint32_t ContrailPointStateScaleFlags;  // bitfield: duration, duration_delta, transition_duration, transition_duration_delta, width, color
typedef uint16_t ContrailFlags;  // bitfield: first_point_unfaded, last_point_unfaded, points_start_pinned_to_media, points_start_pinned_to_ground, points_always_pinned_to_media, points_always_pinned_to_ground, edge_effect_fades_slowly
typedef uint16_t ContrailScaleFlags;  // bitfield: point_generation_rate, point_velocity, point_velocity_delta, point_velocity_cone_angle, inherited_velocity_fraction, sequence_animation_rate, texture_scale_u, texture_scale_v, texture_animation_u, texture_animation_v
typedef struct ContrailPointState {
    float duration[2];
    float transition_duration[2];
    TagDependency physics;  // point_physics
    uint8_t _pad_20[32];
    float width;
    ColorARGB color_lower_bound;
    ColorARGB color_upper_bound;
    ContrailPointStateScaleFlags scale_flags;
} ContrailPointState;  // size 0x68
typedef enum ShaderType {
    shadertype_screen = 0,
    shadertype_effect = 1,
    shadertype_decal = 2,
    shadertype_environment = 3,
    shadertype_model = 4,
    shadertype_transparent_generic = 5,
    shadertype_transparent_chicago = 6,
    shadertype_transparent_chicago_extended = 7,
    shadertype_transparent_water = 8,
    shadertype_transparent_glass = 9,
    shadertype_transparent_meter = 10,
    shadertype_transparent_plasma = 11,
} ShaderType;  // int16
typedef int16_t ShaderType_t;
typedef uint16_t ParticleShaderFlags;  // bitfield: sort_bias, nonlinear_tint, don_t_overdraw_fp_weapon
typedef enum FramebufferBlendFunction {
    framebufferblendfunction_alpha_blend = 0,
    framebufferblendfunction_multiply = 1,
    framebufferblendfunction_double_multiply = 2,
    framebufferblendfunction_add = 3,
    framebufferblendfunction_subtract = 4,
    framebufferblendfunction_component_min = 5,
    framebufferblendfunction_component_max = 6,
    framebufferblendfunction_alpha_multiply_add = 7,
} FramebufferBlendFunction;  // int16
typedef int16_t FramebufferBlendFunction_t;
typedef enum FramebufferFadeMode {
    framebufferfademode_none = 0,
    framebufferfademode_fade_when_perpendicular = 1,
    framebufferfademode_fade_when_parallel = 2,
} FramebufferFadeMode;  // int16
typedef int16_t FramebufferFadeMode_t;
typedef enum ParticleAnchor {
    particleanchor_with_primary = 0,
    particleanchor_with_screen_space = 1,
    particleanchor_zsprite = 2,
} ParticleAnchor;  // int16
typedef int16_t ParticleAnchor_t;
typedef struct Contrail {
    ContrailFlags flags;
    ContrailScaleFlags scale_flags;
    float point_generation_rate;
    float point_velocity[2];
    float point_velocity_cone_angle;
    float inherited_velocity_fraction;
    ContrailRenderType_t render_type;
    uint8_t _pad_1a[2];
    float texture_repeats_u;
    float texture_repeats_v;
    float texture_animation_u;
    float texture_animation_v;
    float animation_rate;
    TagDependency bitmap;  // bitmap
    uint16_t first_sequence_index;
    int16_t sequence_count;
    uint8_t _pad_44[64];
    uint8_t _pad_84[36];
    ShaderType_t shader_type;
    uint8_t _pad_aa[2];
    ParticleShaderFlags shader_flags;
    FramebufferBlendFunction_t framebuffer_blend_function;
    FramebufferFadeMode_t framebuffer_fade_mode;
    IsUnfilteredFlag bitmap_flags;
    uint8_t _pad_b4[12];
    uint8_t _pad_c0[16];
    TagDependency secondary_bitmap;  // bitmap
    ParticleAnchor_t anchor;
    IsUnfilteredFlag secondary_bitmap_flags;
    FunctionOut_t u_animation_source;
    WaveFunction_t u_animation_function;
    float u_animation_period;
    float u_animation_phase;
    float u_animation_scale;
    FunctionOut_t v_animation_source;
    WaveFunction_t v_animation_function;
    float v_animation_period;
    float v_animation_phase;
    float v_animation_scale;
    FunctionOut_t rotation_animation_source;
    WaveFunction_t rotation_animation_function;
    float rotation_animation_period;
    float rotation_animation_phase;
    float rotation_animation_scale;
    Point2D rotation_animation_center;
    uint8_t _pad_11c[4];
    float zsprite_radius_scale;
    uint8_t _pad_124[20];
    TagReflexive point_states;  // ContrailPointState
} Contrail;  // size 0x144
typedef enum DamageEffectScreenFlashType {
    damageeffectscreenflashtype_none = 0,
    damageeffectscreenflashtype_lighten = 1,
    damageeffectscreenflashtype_darken = 2,
    damageeffectscreenflashtype_max = 3,
    damageeffectscreenflashtype_min = 4,
    damageeffectscreenflashtype_invert = 5,
    damageeffectscreenflashtype_tint = 6,
} DamageEffectScreenFlashType;  // int16
typedef int16_t DamageEffectScreenFlashType_t;
typedef enum DamageEffectScreenFlashPriority {
    damageeffectscreenflashpriority_low = 0,
    damageeffectscreenflashpriority_medium = 1,
    damageeffectscreenflashpriority_high = 2,
} DamageEffectScreenFlashPriority;  // int16
typedef int16_t DamageEffectScreenFlashPriority_t;
typedef uint32_t DamageEffectFlags;  // bitfield: do_not_scale_damage_by_distance
typedef struct DamageEffect {
    float radius[2];
    float cutoff_scale;
    DamageEffectFlags flags;
    uint8_t _pad_10[16];
    uint8_t _pad_20[4];
    DamageEffectScreenFlashType_t type;
    DamageEffectScreenFlashPriority_t priority;
    uint8_t _pad_28[8];
    uint8_t _pad_30[4];
    float duration;
    FunctionType_t fade_function;
    uint8_t _pad_3a[2];
    uint8_t _pad_3c[4];
    uint8_t _pad_40[4];
    float maximum_intensity;
    uint8_t _pad_48[4];
    ColorARGB color;
    float low_frequency_vibrate_frequency;
    float low_frequency_vibrate_duration;
    FunctionType_t low_frequency_vibrate_fade_function;
    uint8_t _pad_66[2];
    uint8_t _pad_68[8];
    float high_frequency_vibrate_frequency;
    float high_frequency_vibrate_duration;
    FunctionType_t high_frequency_vibrate_fade_function;
    uint8_t _pad_7a[2];
    uint8_t _pad_7c[4];
    uint8_t _pad_80[16];
    uint8_t _pad_90[8];
    float temporary_camera_impulse_duration;
    FunctionType_t temporary_camera_impulse_fade_function;
    uint8_t _pad_9e[2];
    float temporary_camera_impulse_rotation;
    float temporary_camera_impulse_pushback;
    float jitter[2];
    uint8_t _pad_b0[8];
    float permanent_camera_impulse_angle;
    uint8_t _pad_bc[4];
    uint8_t _pad_c0[12];
    float camera_shaking_duration;
    FunctionType_t camera_shaking_falloff_function;
    uint8_t _pad_d2[2];
    float camera_shaking_random_translation;
    float camera_shaking_random_rotation;
    uint8_t _pad_dc[4];
    uint8_t _pad_e0[8];
    WaveFunction_t camera_shaking_wobble_function;
    uint8_t _pad_ea[2];
    float camera_shaking_wobble_period;
    float camera_shaking_wobble_weight;
    uint8_t _pad_f4[12];
    uint8_t _pad_100[16];
    uint8_t _pad_110[4];
    TagDependency sound;  // sound
    uint8_t _pad_124[108];
    uint8_t _pad_190[4];
    float breaking_effect_forward_velocity;
    float breaking_effect_forward_radius;
    float breaking_effect_forward_exponent;
    uint8_t _pad_1a0[4];
    uint8_t _pad_1a4[8];
    float breaking_effect_outward_velocity;
    float breaking_effect_outward_radius;
    float breaking_effect_outward_exponent;
    uint8_t _pad_1b8[8];
    uint8_t _pad_1c0[4];
    DamageEffectSideEffect_t damage_side_effect;
    DamageEffectCategory_t damage_category;
    DamageEffectDamageFlags damage_flags;
    float damage_aoe_core_radius;
    float damage_lower_bound;
    float damage_upper_bound[2];
    float damage_vehicle_passthrough_penalty;
    float damage_active_camouflage_damage;
    float damage_stun;
    float damage_maximum_stun;
    float damage_stun_time;
    uint8_t _pad_1f0[4];
    Vector3D damage_instantaneous_acceleration;
    float dirt;
    float sand;
    float stone;
    float snow;
    float wood;
    float metal_hollow;
    float metal_thin;
    float metal_thick;
    float rubber;
    float glass;
    float force_field;
    float grunt;
    float hunter_armor;
    float hunter_skin;
    float elite;
    float jackal;
    float jackal_energy_shield;
    float engineer_skin;
    float engineer_force_field;
    float flood_combat_form;
    float flood_carrier_form;
    float cyborg_armor;
    float cyborg_energy_shield;
    float human_armor;
    float human_skin;
    float sentinel;
    float monitor;
    float plastic;
    float water;
    float leaves;
    float elite_energy_shield;
    float ice;
    float hunter_shield;
    uint8_t _pad_284[12];
    uint8_t _pad_290[16];
} DamageEffect;  // size 0x2a0
typedef enum DecalType {
    decaltype_scratch = 0,
    decaltype_splatter = 1,
    decaltype_burn = 2,
    decaltype_painted_sign = 3,
} DecalType;  // int16
typedef int16_t DecalType_t;
typedef enum DecalLayer {
    decallayer_primary = 0,
    decallayer_secondary = 1,
    decallayer_light = 2,
    decallayer_alpha_tested = 3,
    decallayer_water = 4,
} DecalLayer;  // int16
typedef int16_t DecalLayer_t;
typedef uint16_t DecalFlags;  // bitfield: geometry_inherited_by_next_decal_in_chain, interpolate_color_in_hsv, more_colors, no_random_rotation, water_effect, sapien_snap_to_axis, sapien_incremental_counter, animation_loop, preserve_aspect, disabled_by_blood_setting, sprite_scale_bug_fix
typedef struct Decal {
    DecalFlags flags;
    DecalType_t type;
    DecalLayer_t layer;
    uint8_t _pad_6[2];
    TagDependency next_decal_in_chain;  // decal
    float radius[2];
    uint8_t _pad_20[12];
    float intensity[2];
    ColorRGB color_lower_bounds;
    ColorRGB color_upper_bounds;
    uint8_t _pad_4c[12];
    int16_t animation_loop_frame;
    int16_t animation_speed;
    uint8_t _pad_5c[28];
    float lifetime[2];
    float decay_time[2];
    uint8_t _pad_88[12];
    uint8_t _pad_94[40];
    uint8_t _pad_bc[2];
    uint8_t _pad_be[2];
    FramebufferBlendFunction_t framebuffer_blend_function;
    uint8_t _pad_c2[2];
    uint8_t _pad_c4[20];
    TagDependency map;  // bitmap
    uint8_t _pad_e8[20];
    float maximum_sprite_extent;
    uint8_t _pad_100[4];
    uint8_t _pad_104[8];
} Decal;  // size 0x10c
typedef enum DetailObjectCollectionType {
    detailobjectcollectiontype_screen_facing = 0,
    detailobjectcollectiontype_viewer_facing = 1,
} DetailObjectCollectionType;  // int16
typedef int16_t DetailObjectCollectionType_t;
typedef uint8_t DetailObjectCollectionTypeFlags;  // bitfield: unused_a, unused_b, interpolate_color_in_hsv, more_colors
typedef struct DetailObjectCollectionObjectType {
    TagString name;
    uint8_t sequence_index;
    DetailObjectCollectionTypeFlags flags;
    uint8_t first_sprite_index;
    uint8_t sprite_count;
    float color_override_factor;
    uint8_t _pad_28[8];
    float near_fade_distance;
    float far_fade_distance;
    float size;
    uint8_t _pad_3c[4];
    ColorRGB minimum_color;
    ColorRGB maximum_color;
    ColorARGBInt ambient_color;
    uint8_t _pad_5c[4];
} DetailObjectCollectionObjectType;  // size 0x60
typedef struct DetailObjectCollection {
    DetailObjectCollectionType_t collection_type;
    uint8_t _pad_2[2];
    float global_z_offset;
    uint8_t _pad_8[44];
    TagDependency sprite_plate;  // bitmap
    TagReflexive types;  // DetailObjectCollectionObjectType
    uint8_t _pad_50[48];
} DetailObjectCollection;  // size 0x80
typedef enum DeviceIn {
    devicein_none = 0,
    devicein_power = 1,
    devicein_change_in_power = 2,
    devicein_position = 3,
    devicein_change_in_position = 4,
    devicein_locked = 5,
    devicein_delay = 6,
} DeviceIn;  // int16
typedef int16_t DeviceIn_t;
typedef uint32_t DeviceFlags;  // bitfield: position_loops, position_not_interpolated
typedef struct Device {
    Object base;  // inherits
    DeviceFlags device_flags;
    float power_transition_time;
    float power_acceleration_time;
    float position_transition_time;
    float position_acceleration_time;
    float depowered_position_transition_time;
    float depowered_position_acceleration_time;
    DeviceIn_t device_a_in;
    DeviceIn_t device_b_in;
    DeviceIn_t device_c_in;
    DeviceIn_t device_d_in;
    TagDependency open;  // sound,effect
    TagDependency close;  // sound,effect
    TagDependency opened;  // sound,effect
    TagDependency closed;  // sound,effect
    TagDependency depowered;  // sound,effect
    TagDependency repowered;  // sound,effect
    float delay_time;
    uint8_t _pad_204[8];
    TagDependency delay_effect;  // sound,effect
    float automatic_activation_radius;
    uint8_t _pad_220[84];
    float inverse_power_acceleration_time;
    float inverse_power_transition_time;
    float inverse_depowered_position_acceleration_time;
    float inverse_depowered_position_transition_time;
    float inverse_position_acceleration_time;
    float inverse_position_transition_time;
    float delay_time_ticks;
} Device;  // size 0x290
typedef enum DeviceType {
    devicetype_toggle_switch = 0,
    devicetype_on_button = 1,
    devicetype_off_button = 2,
    devicetype_call_button = 3,
} DeviceType;  // int16
typedef int16_t DeviceType_t;
typedef enum DeviceTriggersWhen {
    devicetriggerswhen_touched_by_player = 0,
    devicetriggerswhen_destroyed = 1,
} DeviceTriggersWhen;  // int16
typedef int16_t DeviceTriggersWhen_t;
typedef struct DeviceControl {
    Device base;  // inherits
    DeviceType_t type;
    DeviceTriggersWhen_t triggers_when;
    float call_value;
    uint8_t _pad_298[80];
    TagDependency on;  // sound,effect
    TagDependency off;  // sound,effect
    TagDependency deny;  // sound,effect
} DeviceControl;  // size 0x318
typedef struct DeviceLightFixture {
    Device base;  // inherits
    uint8_t _pad_290[64];
} DeviceLightFixture;  // size 0x2d0
typedef enum MachineType {
    machinetype_door = 0,
    machinetype_platform = 1,
    machinetype_gear = 2,
} MachineType;  // int16
typedef int16_t MachineType_t;
typedef enum MachineCollisionResponse {
    machinecollisionresponse_pause_until_crushed = 0,
    machinecollisionresponse_reverse_directions = 1,
} MachineCollisionResponse;  // int16
typedef int16_t MachineCollisionResponse_t;
typedef uint16_t MachineFlags;  // bitfield: pathfinding_obstacle, but_not_when_open, elevator
typedef struct DeviceMachine {
    Device base;  // inherits
    MachineType_t machine_type;
    MachineFlags machine_flags;
    float door_open_time;
    uint8_t _pad_298[80];
    MachineCollisionResponse_t collision_response;
    uint16_t elevator_node;
    uint8_t _pad_2ec[52];
    uint32_t door_open_time_ticks;
} DeviceMachine;  // size 0x324
typedef struct Dialogue {
    uint8_t _pad_0[2];
    uint8_t _pad_2[2];
    uint8_t _pad_4[12];
    TagDependency idle_noncombat;  // sound
    TagDependency idle_combat;  // sound
    TagDependency idle_flee;  // sound
    uint8_t _pad_40[16];
    uint8_t _pad_50[16];
    uint8_t _pad_60[16];
    TagDependency pain_body_minor;  // sound
    TagDependency pain_body_major;  // sound
    TagDependency pain_shield;  // sound
    TagDependency pain_falling;  // sound
    TagDependency scream_fear;  // sound
    TagDependency scream_pain;  // sound
    TagDependency maimed_limb;  // sound
    TagDependency maimed_head;  // sound
    TagDependency death_quiet;  // sound
    TagDependency death_violent;  // sound
    TagDependency death_falling;  // sound
    TagDependency death_agonizing;  // sound
    TagDependency death_instant;  // sound
    TagDependency death_flying;  // sound
    uint8_t _pad_150[16];
    TagDependency damaged_friend;  // sound
    TagDependency damaged_friend_player;  // sound
    TagDependency damaged_enemy;  // sound
    TagDependency damaged_enemy_cm;  // sound
    uint8_t _pad_1a0[16];
    uint8_t _pad_1b0[16];
    uint8_t _pad_1c0[16];
    uint8_t _pad_1d0[16];
    TagDependency hurt_friend;  // sound
    TagDependency hurt_friend_re;  // sound
    TagDependency hurt_friend_player;  // sound
    TagDependency hurt_enemy;  // sound
    TagDependency hurt_enemy_re;  // sound
    TagDependency hurt_enemy_cm;  // sound
    TagDependency hurt_enemy_bullet;  // sound
    TagDependency hurt_enemy_needler;  // sound
    TagDependency hurt_enemy_plasma;  // sound
    TagDependency hurt_enemy_sniper;  // sound
    TagDependency hurt_enemy_grenade;  // sound
    TagDependency hurt_enemy_explosion;  // sound
    TagDependency hurt_enemy_melee;  // sound
    TagDependency hurt_enemy_flame;  // sound
    TagDependency hurt_enemy_shotgun;  // sound
    TagDependency hurt_enemy_vehicle;  // sound
    TagDependency hurt_enemy_mountedweapon;  // sound
    uint8_t _pad_2f0[16];
    uint8_t _pad_300[16];
    uint8_t _pad_310[16];
    TagDependency killed_friend;  // sound
    TagDependency killed_friend_cm;  // sound
    TagDependency killed_friend_player;  // sound
    TagDependency killed_friend_player_cm;  // sound
    TagDependency killed_enemy;  // sound
    TagDependency killed_enemy_cm;  // sound
    TagDependency killed_enemy_player;  // sound
    TagDependency killed_enemy_player_cm;  // sound
    TagDependency killed_enemy_covenant;  // sound
    TagDependency killed_enemy_covenant_cm;  // sound
    TagDependency killed_enemy_floodcombat;  // sound
    TagDependency killed_enemy_floodcombat_cm;  // sound
    TagDependency killed_enemy_floodcarrier;  // sound
    TagDependency killed_enemy_floodcarrier_cm;  // sound
    TagDependency killed_enemy_sentinel;  // sound
    TagDependency killed_enemy_sentinel_cm;  // sound
    TagDependency killed_enemy_bullet;  // sound
    TagDependency killed_enemy_needler;  // sound
    TagDependency killed_enemy_plasma;  // sound
    TagDependency killed_enemy_sniper;  // sound
    TagDependency killed_enemy_grenade;  // sound
    TagDependency killed_enemy_explosion;  // sound
    TagDependency killed_enemy_melee;  // sound
    TagDependency killed_enemy_flame;  // sound
    TagDependency killed_enemy_shotgun;  // sound
    TagDependency killed_enemy_vehicle;  // sound
    TagDependency killed_enemy_mountedweapon;  // sound
    TagDependency killing_spree;  // sound
    uint8_t _pad_4e0[16];
    uint8_t _pad_4f0[16];
    uint8_t _pad_500[16];
    TagDependency player_kill_cm;  // sound
    TagDependency player_kill_bullet_cm;  // sound
    TagDependency player_kill_needler_cm;  // sound
    TagDependency player_kill_plasma_cm;  // sound
    TagDependency player_kill_sniper_cm;  // sound
    TagDependency anyone_kill_grenade_cm;  // sound
    TagDependency player_kill_explosion_cm;  // sound
    TagDependency player_kill_melee_cm;  // sound
    TagDependency player_kill_flame_cm;  // sound
    TagDependency player_kill_shotgun_cm;  // sound
    TagDependency player_kill_vehicle_cm;  // sound
    TagDependency player_kill_mountedweapon_cm;  // sound
    TagDependency player_killling_spree_cm;  // sound
    uint8_t _pad_5e0[16];
    uint8_t _pad_5f0[16];
    uint8_t _pad_600[16];
    TagDependency friend_died;  // sound
    TagDependency friend_player_died;  // sound
    TagDependency friend_killed_by_friend;  // sound
    TagDependency friend_killed_by_friendly_player;  // sound
    TagDependency friend_killed_by_enemy;  // sound
    TagDependency friend_killed_by_enemy_player;  // sound
    TagDependency friend_killed_by_covenant;  // sound
    TagDependency friend_killed_by_flood;  // sound
    TagDependency friend_killed_by_sentinel;  // sound
    TagDependency friend_betrayed;  // sound
    uint8_t _pad_6b0[16];
    uint8_t _pad_6c0[16];
    TagDependency new_combat_alone;  // sound
    TagDependency new_enemy_recent_combat;  // sound
    TagDependency old_enemy_sighted;  // sound
    TagDependency unexpected_enemy;  // sound
    TagDependency dead_friend_found;  // sound
    TagDependency alliance_broken;  // sound
    TagDependency alliance_reformed;  // sound
    TagDependency grenade_throwing;  // sound
    TagDependency grenade_sighted;  // sound
    TagDependency grenade_startle;  // sound
    TagDependency grenade_danger_enemy;  // sound
    TagDependency grenade_danger_self;  // sound
    TagDependency grenade_danger_friend;  // sound
    uint8_t _pad_7a0[16];
    uint8_t _pad_7b0[16];
    TagDependency new_combat_group_re;  // sound
    TagDependency new_combat_nearby_re;  // sound
    TagDependency alert_friend;  // sound
    TagDependency alert_friend_re;  // sound
    TagDependency alert_lost_contact;  // sound
    TagDependency alert_lost_contact_re;  // sound
    TagDependency blocked;  // sound
    TagDependency blocked_re;  // sound
    TagDependency search_start;  // sound
    TagDependency search_query;  // sound
    TagDependency search_query_re;  // sound
    TagDependency search_report;  // sound
    TagDependency search_abandon;  // sound
    TagDependency search_group_abandon;  // sound
    TagDependency group_uncover;  // sound
    TagDependency group_uncover_re;  // sound
    TagDependency advance;  // sound
    TagDependency advance_re;  // sound
    TagDependency retreat;  // sound
    TagDependency retreat_re;  // sound
    TagDependency cover;  // sound
    uint8_t _pad_910[16];
    uint8_t _pad_920[16];
    uint8_t _pad_930[16];
    uint8_t _pad_940[16];
    TagDependency sighted_friend_player;  // sound
    TagDependency shooting;  // sound
    TagDependency shooting_vehicle;  // sound
    TagDependency shooting_berserk;  // sound
    TagDependency shooting_group;  // sound
    TagDependency shooting_traitor;  // sound
    TagDependency taunt;  // sound
    TagDependency taunt_re;  // sound
    TagDependency flee;  // sound
    TagDependency flee_re;  // sound
    TagDependency flee_leader_died;  // sound
    TagDependency attempted_flee;  // sound
    TagDependency attempted_flee_re;  // sound
    TagDependency lost_contact;  // sound
    TagDependency hiding_finished;  // sound
    TagDependency vehicle_entry;  // sound
    TagDependency vehicle_exit;  // sound
    TagDependency vehicle_woohoo;  // sound
    TagDependency vehicle_scared;  // sound
    TagDependency vehicle_collision;  // sound
    TagDependency partially_sighted;  // sound
    TagDependency nothing_there;  // sound
    TagDependency pleading;  // sound
    uint8_t _pad_ac0[16];
    uint8_t _pad_ad0[16];
    uint8_t _pad_ae0[16];
    uint8_t _pad_af0[16];
    uint8_t _pad_b00[16];
    uint8_t _pad_b10[16];
    TagDependency surprise;  // sound
    TagDependency berserk;  // sound
    TagDependency melee_attack;  // sound
    TagDependency dive;  // sound
    TagDependency uncover_exclamation;  // sound
    TagDependency leap_attack;  // sound
    TagDependency resurrection;  // sound
    uint8_t _pad_b90[16];
    uint8_t _pad_ba0[16];
    uint8_t _pad_bb0[16];
    uint8_t _pad_bc0[16];
    TagDependency celebration;  // sound
    TagDependency check_body_enemy;  // sound
    TagDependency check_body_friend;  // sound
    TagDependency shooting_dead_enemy;  // sound
    TagDependency shooting_dead_enemy_player;  // sound
    uint8_t _pad_c20[16];
    uint8_t _pad_c30[16];
    uint8_t _pad_c40[16];
    uint8_t _pad_c50[16];
    TagDependency alone;  // sound
    TagDependency unscathed;  // sound
    TagDependency seriously_wounded;  // sound
    TagDependency seriously_wounded_re;  // sound
    TagDependency massacre;  // sound
    TagDependency massacre_re;  // sound
    TagDependency rout;  // sound
    TagDependency rout_re;  // sound
    uint8_t _pad_ce0[16];
    uint8_t _pad_cf0[16];
    uint8_t _pad_d00[16];
    uint8_t _pad_d10[16];
    uint8_t _pad_d20[752];
} Dialogue;  // size 0x1010
typedef enum EffectCreateIn {
    effectcreatein_any_environment = 0,
    effectcreatein_air_only = 1,
    effectcreatein_water_only = 2,
    effectcreatein_space_only = 3,
} EffectCreateIn;  // int16
typedef int16_t EffectCreateIn_t;
typedef enum EffectViolenceMode {
    effectviolencemode_either_mode = 0,
    effectviolencemode_violent_mode_only = 1,
    effectviolencemode_nonviolent_mode_only = 2,
} EffectViolenceMode;  // int16
typedef int16_t EffectViolenceMode_t;
typedef enum EffectCreate {
    effectcreate_independent_of_camera_mode = 0,
    effectcreate_only_in_first_person = 1,
    effectcreate_only_in_third_person = 2,
    effectcreate_in_first_person_if_possible = 3,
} EffectCreate;  // int16
typedef int16_t EffectCreate_t;
typedef enum EffectDistributionFunction {
    effectdistributionfunction_start = 0,
    effectdistributionfunction_end = 1,
    effectdistributionfunction_constant = 2,
    effectdistributionfunction_buildup = 3,
    effectdistributionfunction_falloff = 4,
    effectdistributionfunction_buildup_and_falloff = 5,
} EffectDistributionFunction;  // int16
typedef int16_t EffectDistributionFunction_t;
typedef uint16_t EffectPartFlags;  // bitfield: face_down_regardless_of_location_decals, unused, make_effect_work
typedef uint32_t EffectPartScalesValues;  // bitfield: velocity, velocity_delta, velocity_cone_angle, angular_velocity, angular_velocity_delta, type_specific_scale
typedef uint32_t EffectParticleFlags;  // bitfield: stay_attached_to_marker, random_initial_angle, tint_from_object_color, interpolate_tint_as_hsv, across_the_long_hue_path
typedef uint32_t EffectParticleScalesValues;  // bitfield: velocity, velocity_delta, velocity_cone_angle, angular_velocity, angular_velocity_delta, count, count_delta, distribution_radius, distribution_radius_delta, particle_radius, particle_radius_delta, tint
typedef uint32_t EffectFlags;  // bitfield: deleted_when_attachment_deactivates, must_be_deterministic_xbox, must_be_deterministic_pc, disabled_in_anniversary_by_blood_setting
typedef struct EffectLocation {
    TagString marker_name;
} EffectLocation;  // size 0x20
typedef struct EffectPart {
    EffectCreateIn_t create_in;
    EffectViolenceMode_t violence_mode;
    uint16_t location;
    EffectPartFlags flags;
    uint8_t _pad_8[12];
    uint32_t type_class;
    TagDependency type;  // damage_effect,object,particle_system,sound,decal,light
    uint8_t _pad_28[24];
    float velocity_bounds[2];
    float velocity_cone_angle;
    float angular_velocity_bounds[2];
    float radius_modifier_bounds[2];
    uint8_t _pad_5c[4];
    EffectPartScalesValues a_scales_values;
    EffectPartScalesValues b_scales_values;
} EffectPart;  // size 0x68
typedef struct EffectParticle {
    EffectCreateIn_t create_in;
    EffectViolenceMode_t violence_mode;
    EffectCreate_t create;
    uint8_t _pad_6[2];
    uint16_t location;
    uint8_t _pad_a[2];
    Euler2D relative_direction;
    Point3D relative_offset;
    Vector3D relative_direction_vector;
    uint8_t _pad_2c[40];
    TagDependency particle_type;  // particle
    EffectParticleFlags flags;
    EffectDistributionFunction_t distribution_function;
    uint8_t _pad_6a[2];
    int16_t count[2];
    float distribution_radius[2];
    uint8_t _pad_78[12];
    float velocity[2];
    float velocity_cone_angle;
    float angular_velocity[2];
    uint8_t _pad_98[8];
    float radius[2];
    uint8_t _pad_a8[8];
    ColorARGB tint_lower_bound;
    ColorARGB tint_upper_bound;
    uint8_t _pad_d0[16];
    EffectParticleScalesValues a_scales_values;
    EffectParticleScalesValues b_scales_values;
} EffectParticle;  // size 0xe8
typedef struct EffectEvent {
    uint8_t _pad_0[4];
    float skip_fraction;
    float delay_bounds[2];
    float duration_bounds[2];
    uint8_t _pad_18[20];
    TagReflexive parts;  // EffectPart
    TagReflexive particles;  // EffectParticle
} EffectEvent;  // size 0x44
typedef struct Effect {
    EffectFlags flags;
    uint16_t loop_start_event;
    uint16_t loop_stop_event;
    float maximum_damage_radius;
    uint8_t _pad_c[28];
    TagReflexive locations;  // EffectLocation
    TagReflexive events;  // EffectEvent
} Effect;  // size 0x40
typedef enum MaterialType {
    materialtype_dirt = 0,
    materialtype_sand = 1,
    materialtype_stone = 2,
    materialtype_snow = 3,
    materialtype_wood = 4,
    materialtype_metal_hollow = 5,
    materialtype_metal_thin = 6,
    materialtype_metal_thick = 7,
    materialtype_rubber = 8,
    materialtype_glass = 9,
    materialtype_force_field = 10,
    materialtype_grunt = 11,
    materialtype_hunter_armor = 12,
    materialtype_hunter_skin = 13,
    materialtype_elite = 14,
    materialtype_jackal = 15,
    materialtype_jackal_energy_shield = 16,
    materialtype_engineer_skin = 17,
    materialtype_engineer_force_field = 18,
    materialtype_flood_combat_form = 19,
    materialtype_flood_carrier_form = 20,
    materialtype_cyborg_armor = 21,
    materialtype_cyborg_energy_shield = 22,
    materialtype_human_armor = 23,
    materialtype_human_skin = 24,
    materialtype_sentinel = 25,
    materialtype_monitor = 26,
    materialtype_plastic = 27,
    materialtype_water = 28,
    materialtype_leaves = 29,
    materialtype_elite_energy_shield = 30,
    materialtype_ice = 31,
    materialtype_hunter_shield = 32,
} MaterialType;  // int16
typedef int16_t MaterialType_t;
typedef enum VertexType {
    vertextype_structure_bsp_uncompressed_rendered_vertices = 0,
    vertextype_structure_bsp_compressed_rendered_vertices = 1,
    vertextype_structure_bsp_uncompressed_lightmap_vertices = 2,
    vertextype_structure_bsp_compressed_lightmap_vertices = 3,
    vertextype_model_uncompressed = 4,
    vertextype_model_compressed = 5,
} VertexType;  // int16
typedef int16_t VertexType_t;
typedef enum TriangleBufferType {
    trianglebuffertype_triangle_list = 0,
    trianglebuffertype_triangle_strip = 1,
} TriangleBufferType;  // int16
typedef int16_t TriangleBufferType_t;
typedef enum EquipmentPowerupType {
    equipmentpoweruptype_none = 0,
    equipmentpoweruptype_double_speed = 1,
    equipmentpoweruptype_over_shield = 2,
    equipmentpoweruptype_active_camouflage = 3,
    equipmentpoweruptype_full_spectrum_vision = 4,
    equipmentpoweruptype_health = 5,
    equipmentpoweruptype_grenade = 6,
} EquipmentPowerupType;  // int16
typedef int16_t EquipmentPowerupType_t;
typedef uint32_t ItemFlags;  // bitfield: always_maintains_z_up, destroyed_by_explosions, unaffected_by_gravity
typedef enum ItemFunctionIn {
    itemfunctionin_none = 0,
} ItemFunctionIn;  // int16
typedef int16_t ItemFunctionIn_t;
typedef struct Item {
    Object base;  // inherits
    ItemFlags item_flags;
    uint16_t pickup_text_index;
    int16_t sort_order;
    float scale;
    int16_t hud_message_value_scale;
    uint8_t _pad_18a[2];
    uint8_t _pad_18c[16];
    ItemFunctionIn_t item_a_in;
    ItemFunctionIn_t item_b_in;
    ItemFunctionIn_t item_c_in;
    ItemFunctionIn_t item_d_in;
    uint8_t _pad_1a4[164];
    TagDependency material_effects;  // material_effects
    TagDependency collision_sound;  // sound
    uint8_t _pad_268[120];
    float detonation_delay[2];
    TagDependency detonating_effect;  // effect
    TagDependency detonation_effect;  // effect
} Item;  // size 0x308
typedef struct Equipment {
    Item base;  // inherits
    EquipmentPowerupType_t powerup_type;
    GrenadeType_t grenade_type;
    float powerup_time;
    TagDependency pickup_sound;  // sound
    uint8_t _pad_320[144];
} Equipment;  // size 0x3b0
typedef enum FlagTrailingEdgeShape {
    flagtrailingedgeshape_flat = 0,
    flagtrailingedgeshape_concave_triangular = 1,
    flagtrailingedgeshape_convex_triangular = 2,
    flagtrailingedgeshape_trapezoid_short_top = 3,
    flagtrailingedgeshape_trapezoid_short_bottom = 4,
} FlagTrailingEdgeShape;  // int16
typedef int16_t FlagTrailingEdgeShape_t;
typedef enum FlagAttachedEdgeShape {
    flagattachededgeshape_flat = 0,
    flagattachededgeshape_concave_triangular = 1,
} FlagAttachedEdgeShape;  // int16
typedef int16_t FlagAttachedEdgeShape_t;
typedef struct FlagAttachmentPoint {
    int16_t height_to_next_attachment;
    uint8_t _pad_2[2];
    uint8_t _pad_4[16];
    TagString marker_name;
} FlagAttachmentPoint;  // size 0x34
typedef struct Flag {
    IsUnusedFlag flags;
    FlagTrailingEdgeShape_t trailing_edge_shape;
    int16_t trailing_edge_shape_offset;
    FlagAttachedEdgeShape_t attached_edge_shape;
    uint8_t _pad_a[2];
    int16_t width;
    int16_t height;
    float cell_width;
    float cell_height;
    TagDependency red_flag_shader;  // shader
    TagDependency physics;  // point_physics
    float wind_noise;
    uint8_t _pad_3c[8];
    TagDependency blue_flag_shader;  // shader
    TagReflexive attachment_points;  // FlagAttachmentPoint
} Flag;  // size 0x60
typedef uint32_t FogFlags;  // bitfield: is_water, atmosphere_dominant, fog_screen_only
typedef uint16_t FogScreenFlags;  // bitfield: no_environment_multipass, no_model_multipass, no_texture_based_falloff
typedef struct Fog {
    FogFlags flags;
    uint8_t _pad_4[4];
    uint8_t _pad_8[76];
    uint8_t _pad_54[4];
    float maximum_density;
    uint8_t _pad_5c[4];
    float opaque_distance;
    uint8_t _pad_64[4];
    float opaque_depth;
    uint8_t _pad_6c[8];
    float distance_to_water_plane;
    ColorRGB color;
    FogScreenFlags flags_1;
    int16_t layer_count;
    float distance_gradient[2];
    float density_gradient[2];
    float start_distance_from_fog_plane;
    uint8_t _pad_9c[4];
    ColorARGBInt screen_layers_color;
    float rotation_multiplier;
    float strafing_multiplier;
    float zoom_multiplier;
    uint8_t _pad_b0[8];
    float map_scale;
    TagDependency map;  // bitmap
    float animation_period;
    uint8_t _pad_d0[4];
    float wind_velocity[2];
    float wind_period[2];
    float wind_acceleration_weight;
    float wind_perpendicular_weight;
    uint8_t _pad_ec[8];
    TagDependency background_sound;  // sound_looping
    TagDependency sound_environment;  // sound_environment
    uint8_t _pad_114[120];
} Fog;  // size 0x18c
typedef uint32_t FontFlags;  // bitfield: disable_mcc_font_override
typedef struct FontCharacterIndex {
    uint16_t character_index;
} FontCharacterIndex;  // size 0x2
typedef struct FontCharacterTables {
    TagReflexive character_table;  // FontCharacterIndex
} FontCharacterTables;  // size 0xc
typedef enum FontEncodingType {
    fontencodingtype_unicode = 0,
    fontencodingtype_cjk = 1,
} FontEncodingType;  // int16
typedef int16_t FontEncodingType_t;
typedef struct FontCharacter {
    uint16_t character;
    int16_t character_width;
    int16_t bitmap_width;
    int16_t bitmap_height;
    int16_t bitmap_origin_x;
    int16_t bitmap_origin_y;
    uint16_t hardware_character_index;
    uint8_t _pad_e[2];
    int32_t pixels_offset;
} FontCharacter;  // size 0x14
typedef struct Font {
    FontFlags flags;
    int16_t ascending_height;
    int16_t descending_height;
    int16_t leading_height;
    int16_t leading_width;
    FontEncodingType_t encoding_type;
    uint16_t cjk_character_codes;
    uint8_t _pad_10[32];
    TagReflexive character_tables;  // FontCharacterTables
    TagDependency bold;  // font
    TagDependency italic;  // font
    TagDependency condense;  // font
    TagDependency underline;  // font
    TagReflexive characters;  // FontCharacter
    TagDataOffset pixels;
} Font;  // size 0x9c
typedef struct Garbage {
    Item base;  // inherits
    uint8_t _pad_308[168];
} Garbage;  // size 0x3b0
typedef uint32_t ModelGeometryPartFlags;  // bitfield: stripped_internal, zoner
typedef struct ModelVertexUncompressed {
    Point3D position;
    Vector3D normal;
    Vector3D binormal;
    Vector3D tangent;
    Point2D texture_coords;
    uint16_t node0_index;
    uint16_t node1_index;
    float node0_weight;
    float node1_weight;
} ModelVertexUncompressed;  // size 0x44
typedef struct ModelVertexCompressed {
    Point3D position;
    uint32_t normal;
    uint32_t binormal;
    uint32_t tangent;
    int16_t texture_coordinate_u;
    int16_t texture_coordinate_v;
    int8_t node0_index;
    int8_t node1_index;
    uint16_t node0_weight;
} ModelVertexCompressed;  // size 0x20
typedef struct ModelTriangle {
    uint16_t vertex0_index;
    uint16_t vertex1_index;
    uint16_t vertex2_index;
} ModelTriangle;  // size 0x6
typedef struct ModelGeometryPart {
    ModelGeometryPartFlags flags;
    uint16_t shader_index;
    uint8_t prev_filthy_part_index;
    uint8_t next_filthy_part_index;
    uint16_t centroid_primary_node;
    uint16_t centroid_secondary_node;
    float centroid_primary_weight;
    float centroid_secondary_weight;
    Point3D centroid;
    TagReflexive uncompressed_vertices;  // ModelVertexUncompressed
    TagReflexive compressed_vertices;  // ModelVertexCompressed
    TagReflexive triangles;  // ModelTriangle
    TriangleBufferType_t triangle_buffer_type;
    uint8_t _pad_46[2];
    uint32_t triangle_count;
    uint32_t triangle_offset;
    uint32_t triangle_offset_2;
    VertexType_t vertex_type;
    uint8_t _pad_56[2];
    uint32_t vertex_count;
    uint8_t _pad_5c[4];
    uint32_t vertex_pointer;
    uint32_t vertex_offset;
} ModelGeometryPart;  // size 0x68
typedef struct GBXModelGeometryPart {
    ModelGeometryPart base;  // inherits
    uint8_t _pad_68[1];
    uint8_t _pad_69[1];
    uint8_t _pad_6a[1];
    uint8_t local_node_count;
    uint8_t local_node_indices[22];
    uint8_t _pad_82[2];
} GBXModelGeometryPart;  // size 0x84
typedef struct GBXModelGeometry {
    IsUnusedFlag flags;
    uint8_t _pad_4[32];
    TagReflexive parts;  // GBXModelGeometryPart
} GBXModelGeometry;  // size 0x30
typedef uint32_t ModelFlags;  // bitfield: blend_shared_normals, parts_have_local_nodes, ignore_skinning
typedef struct ModelMarkerInstance {
    uint8_t region_index;
    uint8_t permutation_index;
    uint8_t node_index;
    uint8_t _pad_3[1];
    Point3D translation;
    Quaternion rotation;
} ModelMarkerInstance;  // size 0x20
typedef struct ModelMarker {
    TagString name;
    int16_t magic_identifier;
    uint8_t _pad_22[2];
    uint8_t _pad_24[16];
    TagReflexive instances;  // ModelMarkerInstance
} ModelMarker;  // size 0x40
typedef struct ModelNode {
    TagString name;
    uint16_t next_sibling_node_index;
    uint16_t first_child_node_index;
    uint16_t parent_node_index;
    uint8_t _pad_26[2];
    Point3D default_translation;
    Quaternion default_rotation;
    float node_distance_from_parent;
    uint8_t _pad_48[32];
    float scale;
    Matrix rotation;
    Point3D translation;
} ModelNode;  // size 0x9c
typedef uint32_t ModelRegionPermutationFlags;  // bitfield: cannot_be_chosen_randomly
typedef struct ModelRegionPermutationMarker {
    TagString name;
    uint16_t node_index;
    uint8_t _pad_22[2];
    Quaternion rotation;
    Point3D translation;
    uint8_t _pad_40[16];
} ModelRegionPermutationMarker;  // size 0x50
typedef struct ModelRegionPermutation {
    TagString name;
    ModelRegionPermutationFlags flags;
    uint16_t permutation_number;
    uint8_t _pad_26[2];
    uint8_t _pad_28[24];
    uint16_t super_low;  // 0x40 geometry index for lod 0; 0x4d72a0 reads +0x40 + lod*2, so this
                         //      pairs with GBXModel +0x08 (cutoff index 0), not with the
                         //      "super low" cutoff at +0x18 despite the name
    uint16_t low;
    uint16_t medium;
    uint16_t high;
    uint16_t super_high;
    uint8_t _pad_4a[2];
    TagReflexive markers;  // ModelRegionPermutationMarker
} ModelRegionPermutation;  // size 0x58
typedef struct ModelRegion {
    TagString name;
    uint8_t _pad_20[32];
    TagReflexive permutations;  // ModelRegionPermutation
} ModelRegion;  // size 0x4c
typedef struct ModelShaderReference {
    TagDependency shader;  // shader
    uint16_t permutation;
    uint8_t _pad_12[2];
    uint8_t _pad_14[12];
} ModelShaderReference;  // size 0x20
typedef struct GBXModel {
    ModelFlags flags;
    int32_t node_list_checksum;
    float super_high_detail_cutoff;  // 0x08 cutoff index 0 (lod 0): 0x4d6fc0 reads +0x08 + lod*4
                                     //      and walks lod 4..0 until cutoff[lod] <= pixels, so this
                                     //      is the minimum pixel size at which anything is drawn.
                                     //      Paired with ModelRegionPermutation +0x40 (super_low).
    float high_detail_cutoff;
    float medium_detail_cutoff;
    float low_detail_cutoff;
    float super_low_detail_cutoff;
    uint16_t super_low_detail_node_count;
    uint16_t low_detail_node_count;
    uint16_t medium_detail_node_count;
    uint16_t high_detail_node_count;
    uint16_t super_high_detail_node_count;
    uint8_t _pad_26[2];
    uint8_t _pad_28[8];
    float base_map_u_scale;
    float base_map_v_scale;
    uint8_t _pad_38[116];
    TagReflexive markers;  // ModelMarker
    TagReflexive nodes;  // ModelNode
    TagReflexive regions;  // ModelRegion
    TagReflexive geometries;  // GBXModelGeometry
    TagReflexive shaders;  // ModelShaderReference
} GBXModel;  // size 0xe8
typedef enum MultiplayerInformationSound {
    multiplayerinformationsound_play_ball = 0,
    multiplayerinformationsound_game_over = 1,
    multiplayerinformationsound_one_minute_to_win = 2,
    multiplayerinformationsound_30_seconds_to_win = 3,
    multiplayerinformationsound_red_team_minute_to_win = 4,
    multiplayerinformationsound_red_team_30_to_win = 5,
    multiplayerinformationsound_blue_team_minute_to_win = 6,
    multiplayerinformationsound_blue_team_30_to_win = 7,
    multiplayerinformationsound_blue_team_has_the_flag = 8,
    multiplayerinformationsound_blue_team_flag_returned = 9,
    multiplayerinformationsound_blue_team_score = 10,
    multiplayerinformationsound_red_team_has_the_flag = 11,
    multiplayerinformationsound_red_team_flag_returned = 12,
    multiplayerinformationsound_red_team_score = 13,
    multiplayerinformationsound_double_kill = 14,
    multiplayerinformationsound_triple_kill = 15,
    multiplayerinformationsound_killtacular = 16,
    multiplayerinformationsound_running_riot = 17,
    multiplayerinformationsound_killing_spree = 18,
    multiplayerinformationsound_oddball = 19,
    multiplayerinformationsound_race = 20,
    multiplayerinformationsound_slayer = 21,
    multiplayerinformationsound_capture_the_flag = 22,
    multiplayerinformationsound_warthog = 23,
    multiplayerinformationsound_ghost = 24,
    multiplayerinformationsound_scorpion = 25,
    multiplayerinformationsound_countdown_timer = 26,
    multiplayerinformationsound_teleporter_activate = 27,
    multiplayerinformationsound_flag_failure = 28,
    multiplayerinformationsound_countdown_for_respawn = 29,
    multiplayerinformationsound_hill_move = 30,
    multiplayerinformationsound_player_respawn = 31,
    multiplayerinformationsound_team_king_of_the_hill = 32,
    multiplayerinformationsound_team_oddball = 33,
    multiplayerinformationsound_team_race = 34,
    multiplayerinformationsound_team_slayer = 35,
    multiplayerinformationsound_king_of_the_hill = 36,
    multiplayerinformationsound_blue_team_ctf = 37,
    multiplayerinformationsound_red_team_ctf = 38,
    multiplayerinformationsound_hill_contested = 39,
    multiplayerinformationsound_hill_controlled = 40,
    multiplayerinformationsound_hill_occupied = 41,
    multiplayerinformationsound_countdown_timer_end = 42,
    multiplayerinformationsound_ting = 43,
} MultiplayerInformationSound;  // int16
typedef int16_t MultiplayerInformationSound_t;
typedef uint16_t GlobalsRasterizerDataFlags;  // bitfield: tint_edge_density
typedef uint32_t GlobalsBreakableSurfaceParticleEffectFlags;  // bitfield: interpolate_color_in_hsv, more_colors
typedef struct GlobalsSound {
    TagDependency sound;  // sound
} GlobalsSound;  // size 0x10
typedef struct GlobalsCamera {
    TagDependency default_unit_camera_track;  // camera_track
} GlobalsCamera;  // size 0x10
typedef struct GlobalsLookFunction {
    float scale;
} GlobalsLookFunction;  // size 0x4
typedef struct GlobalsPlayerControl {
    float magnetism_friction;
    float magnetism_adhesion;
    float inconsequential_target_scale;
    uint8_t _pad_c[52];
    float look_acceleration_time;
    float look_acceleration_scale;
    float look_peg_threshold;
    float look_default_pitch_rate;
    float look_default_yaw_rate;
    float look_autolevelling_scale;
    uint8_t _pad_58[20];
    int16_t minimum_weapon_swap_ticks;
    int16_t minimum_autolevelling_ticks;
    float minimum_angle_for_vehicle_flipping;
    TagReflexive look_function;  // GlobalsLookFunction
} GlobalsPlayerControl;  // size 0x80
typedef struct GlobalsDifficulty {
    float easy_enemy_damage;
    float normal_enemy_damage;
    float hard_enemy_damage;
    float imposs_enemy_damage;
    float easy_enemy_vitality;
    float normal_enemy_vitality;
    float hard_enemy_vitality;
    float imposs_enemy_vitality;
    float easy_enemy_shield;
    float normal_enemy_shield;
    float hard_enemy_shield;
    float imposs_enemy_shield;
    float easy_enemy_recharge;
    float normal_enemy_recharge;
    float hard_enemy_recharge;
    float imposs_enemy_recharge;
    float easy_friend_damage;
    float normal_friend_damage;
    float hard_friend_damage;
    float imposs_friend_damage;
    float easy_friend_vitality;
    float normal_friend_vitality;
    float hard_friend_vitality;
    float imposs_friend_vitality;
    float easy_friend_shield;
    float normal_friend_shield;
    float hard_friend_shield;
    float imposs_friend_shield;
    float easy_friend_recharge;
    float normal_friend_recharge;
    float hard_friend_recharge;
    float imposs_friend_recharge;
    float easy_infection_forms;
    float normal_infection_forms;
    float hard_infection_forms;
    float imposs_infection_forms;
    uint8_t _pad_90[16];
    float easy_rate_of_fire;
    float normal_rate_of_fire;
    float hard_rate_of_fire;
    float imposs_rate_of_fire;
    float easy_projectile_error;
    float normal_projectile_error;
    float hard_projectile_error;
    float imposs_projectile_error;
    float easy_burst_error;
    float normal_burst_error;
    float hard_burst_error;
    float imposs_burst_error;
    float easy_new_target_delay;
    float normal_new_target_delay;
    float hard_new_target_delay;
    float imposs_new_target_delay;
    float easy_burst_separation;
    float normal_burst_separation;
    float hard_burst_separation;
    float imposs_burst_separation;
    float easy_target_tracking;
    float normal_target_tracking;
    float hard_target_tracking;
    float imposs_target_tracking;
    float easy_target_leading;
    float normal_target_leading;
    float hard_target_leading;
    float imposs_target_leading;
    float easy_overcharge_chance;
    float normal_overcharge_chance;
    float hard_overcharge_chance;
    float imposs_overcharge_chance;
    float easy_special_fire_delay;
    float normal_special_fire_delay;
    float hard_special_fire_delay;
    float imposs_special_fire_delay;
    float easy_guidance_vs_player;
    float normal_guidance_vs_player;
    float hard_guidance_vs_player;
    float imposs_guidance_vs_player;
    float easy_melee_delay_base;
    float normal_melee_delay_base;
    float hard_melee_delay_base;
    float imposs_melee_delay_base;
    float easy_melee_delay_scale;
    float normal_melee_delay_scale;
    float hard_melee_delay_scale;
    float imposs_melee_delay_scale;
    uint8_t _pad_160[16];
    float easy_grenade_chance_scale;
    float normal_grenade_chance_scale;
    float hard_grenade_chance_scale;
    float imposs_grenade_chance_scale;
    float easy_grenade_timer_scale;
    float normal_grenade_timer_scale;
    float hard_grenade_timer_scale;
    float imposs_grenade_timer_scale;
    uint8_t _pad_190[16];
    uint8_t _pad_1a0[16];
    uint8_t _pad_1b0[16];
    float easy_major_upgrade;
    float normal_major_upgrade;
    float hard_major_upgrade;
    float imposs_major_upgrade;
    float easy_major_upgrade_1;
    float normal_major_upgrade_1;
    float hard_major_upgrade_1;
    float imposs_major_upgrade_1;
    float easy_major_upgrade_2;
    float normal_major_upgrade_2;
    float hard_major_upgrade_2;
    float imposs_major_upgrade_2;
    uint8_t _pad_1f0[16];
    uint8_t _pad_200[16];
    uint8_t _pad_210[16];
    uint8_t _pad_220[16];
    uint8_t _pad_230[84];
} GlobalsDifficulty;  // size 0x284
typedef struct GlobalsGrenade {
    int16_t maximum_count;
    int16_t mp_spawn_default;
    TagDependency throwing_effect;  // effect
    TagDependency hud_interface;  // grenade_hud_interface
    TagDependency equipment;  // equipment
    TagDependency projectile;  // projectile
} GlobalsGrenade;  // size 0x44
typedef struct GlobalsRasterizerData {
    TagDependency distance_attenuation;  // bitmap
    TagDependency vector_normalization;  // bitmap
    TagDependency atmospheric_fog_density;  // bitmap
    TagDependency planar_fog_density;  // bitmap
    TagDependency linear_corner_fade;  // bitmap
    TagDependency active_camouflage_distortion;  // bitmap
    TagDependency glow;  // bitmap
    uint8_t _pad_70[60];
    TagDependency default_2d;  // bitmap
    TagDependency default_3d;  // bitmap
    TagDependency default_cube_map;  // bitmap
    TagDependency test_0;  // bitmap
    TagDependency test_1;  // bitmap
    TagDependency test_2;  // bitmap
    TagDependency test_3;  // bitmap
    TagDependency video_scanline_map;  // bitmap
    TagDependency video_noise_map;  // bitmap
    uint8_t _pad_13c[52];
    GlobalsRasterizerDataFlags flags;
    uint8_t _pad_172[2];
    float refraction_amount;
    float distance_falloff;
    ColorRGB tint_color;
    float hyper_stealth_refraction;
    float hyper_stealth_distance_falloff;
    ColorRGB hyper_stealth_tint_color;
    TagDependency distance_attenuation_2d;  // bitmap
} GlobalsRasterizerData;  // size 0x1ac
typedef struct GlobalsInterfaceBitmaps {
    TagDependency font_system;  // font
    TagDependency font_terminal;  // font
    TagDependency screen_color_table;  // color_table
    TagDependency hud_color_table;  // color_table
    TagDependency editor_color_table;  // color_table
    TagDependency dialog_color_table;  // color_table
    TagDependency hud_globals;  // hud_globals
    TagDependency motion_sensor_sweep_bitmap;  // bitmap
    TagDependency motion_sensor_sweep_bitmap_mask;  // bitmap
    TagDependency multiplayer_hud_bitmap;  // bitmap
    TagDependency localization;  // string_list
    TagDependency hud_digits_definition;  // hud_number
    TagDependency motion_sensor_blip_bitmap;  // bitmap
    TagDependency interface_goo_map1;  // bitmap
    TagDependency interface_goo_map2;  // bitmap
    TagDependency interface_goo_map3;  // bitmap
    uint8_t _pad_100[48];
} GlobalsInterfaceBitmaps;  // size 0x130
typedef struct GlobalsWeapon {
    TagDependency weapon;  // item
} GlobalsWeapon;  // size 0x10
typedef struct GlobalsCheatPowerup {
    TagDependency powerup;  // equipment
} GlobalsCheatPowerup;  // size 0x10
typedef struct GlobalsVehicle {
    TagDependency vehicle;  // unit
} GlobalsVehicle;  // size 0x10
typedef struct GlobalsMultiplayerInformation {
    TagDependency flag;  // weapon
    TagDependency unit;  // unit
    TagReflexive vehicles;  // GlobalsVehicle
    TagDependency hill_shader;  // shader
    TagDependency flag_shader;  // shader
    TagDependency ball;  // weapon
    TagReflexive sounds;  // GlobalsSound
    uint8_t _pad_68[56];
} GlobalsMultiplayerInformation;  // size 0xa0
typedef struct GlobalsPlayerInformation {
    TagDependency unit;  // unit
    uint8_t _pad_10[28];
    float walking_speed;
    float double_speed_multiplier;
    float run_forward;
    float run_backward;
    float run_sideways;
    float run_acceleration;
    float sneak_forward;
    float sneak_backward;
    float sneak_sideways;
    float sneak_acceleration;
    float airborne_acceleration;
    float speed_multiplier;
    uint8_t _pad_5c[12];
    Point3D grenade_origin;
    uint8_t _pad_74[12];
    float stun_movement_penalty;
    float stun_turning_penalty;
    float stun_jumping_penalty;
    float minimum_stun_time;
    float maximum_stun_time;
    uint8_t _pad_94[8];
    float first_person_idle_time[2];
    float first_person_skip_fraction;
    uint8_t _pad_a8[16];
    TagDependency coop_respawn_effect;  // effect
    uint8_t _pad_c8[44];
} GlobalsPlayerInformation;  // size 0xf4
typedef struct GlobalsFirstPersonInterface {
    TagDependency first_person_hands;  // model
    TagDependency base_bitmap;  // bitmap
    TagDependency shield_meter;  // meter
    Point2DInt shield_meter_origin;
    TagDependency body_meter;  // meter
    Point2DInt body_meter_origin;
    TagDependency night_vision_on_effect;  // effect
    TagDependency night_vision_off_effect;  // effect
    uint8_t _pad_68[88];
} GlobalsFirstPersonInterface;  // size 0xc0
typedef struct GlobalsFallingDamage {
    uint8_t _pad_0[8];
    float harmful_falling_distance[2];
    TagDependency falling_damage;  // damage_effect
    uint8_t _pad_20[8];
    float maximum_falling_distance;
    TagDependency distance_damage;  // damage_effect
    TagDependency vehicle_environment_collision_damage;  // damage_effect
    TagDependency vehicle_killed_unit_damage;  // damage_effect
    TagDependency vehicle_collision_damage;  // damage_effect
    TagDependency flaming_death_damage;  // damage_effect
    uint8_t _pad_7c[16];
    float maximum_falling_velocity;
    float harmful_falling_velocity[2];
} GlobalsFallingDamage;  // size 0x98
typedef struct GlobalsBreakableSurfaceParticleEffect {
    TagDependency particle_type;  // particle
    GlobalsBreakableSurfaceParticleEffectFlags flags;
    float density;
    float velocity_scale[2];
    uint8_t _pad_20[4];
    float angular_velocity[2];
    uint8_t _pad_2c[8];
    float radius[2];
    uint8_t _pad_3c[8];
    ColorARGB tint_lower_bound;
    ColorARGB tint_upper_bound;
    uint8_t _pad_64[28];
} GlobalsBreakableSurfaceParticleEffect;  // size 0x80
typedef struct GlobalsMaterial {
    uint8_t _pad_0[100];
    uint8_t _pad_64[48];
    float ground_friction_scale;
    float ground_friction_normal_k1_scale;
    float ground_friction_normal_k0_scale;
    float ground_depth_scale;
    float ground_damp_fraction_scale;
    uint8_t _pad_a8[76];
    uint8_t _pad_f4[480];
    float maximum_vitality;
    uint8_t _pad_2d8[8];
    uint8_t _pad_2e0[4];
    TagDependency effect;  // effect
    TagDependency sound;  // sound
    uint8_t _pad_304[24];
    TagReflexive particle_effects;  // GlobalsBreakableSurfaceParticleEffect
    uint8_t _pad_328[60];
    TagDependency melee_hit_sound;  // sound
} GlobalsMaterial;  // size 0x374
typedef struct GlobalsPlaylistMember {
    TagString map_name;
    TagString game_variant;
    int32_t minimum_experience;
    int32_t maximum_experience;
    int32_t minimum_player_count;
    int32_t maximum_player_count;
    int32_t rating;
    uint8_t _pad_54[64];
} GlobalsPlaylistMember;  // size 0x94
typedef struct Globals {
    uint8_t _pad_0[248];
    TagReflexive sounds;  // GlobalsSound
    TagReflexive camera;  // GlobalsCamera
    TagReflexive player_control;  // GlobalsPlayerControl
    TagReflexive difficulty;  // GlobalsDifficulty
    TagReflexive grenades;  // GlobalsGrenade
    TagReflexive rasterizer_data;  // GlobalsRasterizerData
    TagReflexive interface_bitmaps;  // GlobalsInterfaceBitmaps
    TagReflexive weapon_list;  // GlobalsWeapon
    TagReflexive cheat_powerups;  // GlobalsCheatPowerup
    TagReflexive multiplayer_information;  // GlobalsMultiplayerInformation
    TagReflexive player_information;  // GlobalsPlayerInformation
    TagReflexive first_person_interface;  // GlobalsFirstPersonInterface
    TagReflexive falling_damage;  // GlobalsFallingDamage
    TagReflexive materials;  // GlobalsMaterial
    TagReflexive playlist_members;  // GlobalsPlaylistMember
} Globals;  // size 0x1ac
typedef enum GlowBoundaryEffect {
    glowboundaryeffect_bounce = 0,
    glowboundaryeffect_wrap = 1,
} GlowBoundaryEffect;  // int16
typedef int16_t GlowBoundaryEffect_t;
typedef enum GlowNormalParticleDistribution {
    glownormalparticledistribution_distributed_randomly = 0,
    glownormalparticledistribution_distributed_uniformly = 1,
} GlowNormalParticleDistribution;  // int16
typedef int16_t GlowNormalParticleDistribution_t;
typedef enum GlowTrailingParticleDistribution {
    glowtrailingparticledistribution_emit_vertically = 0,
    glowtrailingparticledistribution_emit_normal_up = 1,
    glowtrailingparticledistribution_emit_randomly = 2,
} GlowTrailingParticleDistribution;  // int16
typedef int16_t GlowTrailingParticleDistribution_t;
typedef uint32_t GlowFlags;  // bitfield: modify_particle_color_in_range, particles_move_backwards, partices_move_in_both_directions, trailing_particles_fade_over_time, trailing_particles_shrink_over_time, trailing_particles_slow_over_time
typedef struct Glow {
    TagString attachment_marker;
    int16_t number_of_particles;
    GlowBoundaryEffect_t boundary_effect;
    GlowNormalParticleDistribution_t normal_particle_distribution;
    GlowTrailingParticleDistribution_t trailing_particle_distribution;
    GlowFlags glow_flags;
    uint8_t _pad_2c[28];
    uint8_t _pad_48[2];
    uint8_t _pad_4a[2];
    uint8_t _pad_4c[4];
    FunctionOut_t attachment_0;
    uint8_t _pad_52[2];
    float particle_rotational_velocity;
    float particle_rot_vel_mul_low;
    float particle_rot_vel_mul_high;
    FunctionOut_t attachment_1;
    uint8_t _pad_62[2];
    float effect_rotational_velocity;
    float effect_rot_vel_mul_low;
    float effect_rot_vel_mul_high;
    FunctionOut_t attachment_2;
    uint8_t _pad_72[2];
    float effect_translational_velocity;
    float effect_trans_vel_mul_low;
    float effect_trans_vel_mul_high;
    FunctionOut_t attachment_3;
    uint8_t _pad_82[2];
    float min_distance_particle_to_object;
    float max_distance_particle_to_object;
    float distance_to_object_mul_low;
    float distance_to_object_mul_high;
    uint8_t _pad_94[8];
    FunctionOut_t attachment_4;
    uint8_t _pad_9e[2];
    float particle_size_bounds[2];
    float size_attachment_multiplier[2];
    FunctionOut_t attachment_5;
    uint8_t _pad_b2[2];
    ColorARGB color_bound_0;
    ColorARGB color_bound_1;
    ColorARGB scale_color_0;
    ColorARGB scale_color_1;
    float color_rate_of_change;
    float fading_percentage_of_glow;
    float particle_generation_freq;
    float lifetime_of_trailing_particles;
    float velocity_of_trailing_particles;
    float trailing_particle_minimum_t;
    float trailing_particle_maximum_t;
    uint8_t _pad_110[52];
    TagDependency texture;  // bitmap
} Glow;  // size 0x154
typedef uint16_t GrenadeHUDInterfaceOverlayType;  // bitfield: show_on_flashing, show_on_empty, show_on_default, show_always
typedef uint32_t GrenadeHUDInterfaceSoundLatchedTo;  // bitfield: low_grenade_count, no_grenades_left, throw_on_no_grenades
typedef uint16_t HUDInterfaceScalingFlags;  // bitfield: don_t_scale_offset, don_t_scale_size, use_high_res_scale
typedef uint16_t HUDInterfaceFlashFlags;  // bitfield: reverse_default_flashing_colors
typedef uint32_t HUDInterfaceOverlayFlashFlags;  // bitfield: flashes_when_active
typedef struct GrenadeHUDInterfaceOverlay {
    Point2DInt anchor_offset;
    float width_scale;
    float height_scale;
    HUDInterfaceScalingFlags scaling_flags;
    uint8_t _pad_e[2];
    uint8_t _pad_10[20];
    ColorARGBInt default_color;
    ColorARGBInt flashing_color;
    float flash_period;
    float flash_delay;
    int16_t number_of_flashes;
    HUDInterfaceFlashFlags flash_flags;
    float flash_length;
    ColorARGBInt disabled_color;
    uint8_t _pad_40[4];
    float frame_rate;
    uint16_t sequence_index;
    GrenadeHUDInterfaceOverlayType type;
    HUDInterfaceOverlayFlashFlags flags;
    uint8_t _pad_50[16];
    uint8_t _pad_60[40];
} GrenadeHUDInterfaceOverlay;  // size 0x88
typedef struct GrenadeHUDInterfaceSound {
    TagDependency sound;  // sound,sound_looping
    GrenadeHUDInterfaceSoundLatchedTo latched_to;
    float scale;
    uint8_t _pad_18[32];
} GrenadeHUDInterfaceSound;  // size 0x38
typedef enum HUDInterfaceAnchor {
    hudinterfaceanchor_top_left = 0,
    hudinterfaceanchor_top_right = 1,
    hudinterfaceanchor_bottom_left = 2,
    hudinterfaceanchor_bottom_right = 3,
    hudinterfaceanchor_center = 4,
    hudinterfaceanchor_top_center = 5,
    hudinterfaceanchor_bottom_center = 6,
    hudinterfaceanchor_left_center = 7,
    hudinterfaceanchor_right_center = 8,
} HUDInterfaceAnchor;  // int16
typedef int16_t HUDInterfaceAnchor_t;
typedef enum HUDInterfaceCanvasSize {
    hudinterfacecanvassize_480p = 0,
    hudinterfacecanvassize_960p = 1,
} HUDInterfaceCanvasSize;  // int16
typedef int16_t HUDInterfaceCanvasSize_t;
typedef enum HUDInterfaceMultitextureOverlayAnchor {
    hudinterfacemultitextureoverlayanchor_texture = 0,
    hudinterfacemultitextureoverlayanchor_screen = 1,
} HUDInterfaceMultitextureOverlayAnchor;  // int16
typedef int16_t HUDInterfaceMultitextureOverlayAnchor_t;
typedef enum HUDInterfaceZeroToOneBlendFunction {
    hudinterfacezerotooneblendfunction_add = 0,
    hudinterfacezerotooneblendfunction_subtract = 1,
    hudinterfacezerotooneblendfunction_multiply = 2,
    hudinterfacezerotooneblendfunction_multiply2x = 3,
    hudinterfacezerotooneblendfunction_dot = 4,
} HUDInterfaceZeroToOneBlendFunction;  // int16
typedef int16_t HUDInterfaceZeroToOneBlendFunction_t;
typedef enum HUDInterfaceWrapMode {
    hudinterfacewrapmode_clamp = 0,
    hudinterfacewrapmode_wrap = 1,
} HUDInterfaceWrapMode;  // int16
typedef int16_t HUDInterfaceWrapMode_t;
typedef enum HUDInterfaceDestinationType {
    hudinterfacedestinationtype_tint_0_1 = 0,
    hudinterfacedestinationtype_horizontal_offset = 1,
    hudinterfacedestinationtype_vertical_offset = 2,
    hudinterfacedestinationtype_fade_0_1 = 3,
} HUDInterfaceDestinationType;  // int16
typedef int16_t HUDInterfaceDestinationType_t;
typedef enum HUDInterfaceDestination {
    hudinterfacedestination_geometry_offset = 0,
    hudinterfacedestination_primary_map = 1,
    hudinterfacedestination_secondary_map = 2,
    hudinterfacedestination_tertiary_map = 3,
} HUDInterfaceDestination;  // int16
typedef int16_t HUDInterfaceDestination_t;
typedef enum HUDInterfaceSource {
    hudinterfacesource_player_pitch = 0,
    hudinterfacesource_player_pitch_tangent = 1,
    hudinterfacesource_player_yaw = 2,
    hudinterfacesource_weapon_ammo_total = 3,
    hudinterfacesource_weapon_ammo_loaded = 4,
    hudinterfacesource_weapon_heat = 5,
    hudinterfacesource_explicit_uses_low_bound = 6,
    hudinterfacesource_weapon_zoom_level = 7,
} HUDInterfaceSource;  // int16
typedef int16_t HUDInterfaceSource_t;
typedef struct HUDInterfaceMultitextureOverlayEffector {
    uint8_t _pad_0[64];
    HUDInterfaceDestinationType_t destination_type;
    HUDInterfaceDestination_t destination;
    HUDInterfaceSource_t source;
    uint8_t _pad_46[2];
    float in_bounds[2];
    float out_bounds[2];
    uint8_t _pad_58[64];
    ColorRGB tint_color_lower_bound;
    ColorRGB tint_color_upper_bound;
    WaveFunction_t periodic_function;
    uint8_t _pad_b2[2];
    float function_period;
    float function_phase;
    uint8_t _pad_bc[32];
} HUDInterfaceMultitextureOverlayEffector;  // size 0xdc
typedef struct HUDInterfaceMultitextureOverlay {
    uint8_t _pad_0[2];
    int16_t type;
    FramebufferBlendFunction_t framebuffer_blend_function;
    uint8_t _pad_6[2];
    uint8_t _pad_8[32];
    HUDInterfaceMultitextureOverlayAnchor_t primary_anchor;
    HUDInterfaceMultitextureOverlayAnchor_t secondary_anchor;
    HUDInterfaceMultitextureOverlayAnchor_t tertiary_anchor;
    HUDInterfaceZeroToOneBlendFunction_t zero_to_one_blend_function;
    HUDInterfaceZeroToOneBlendFunction_t one_to_two_blend_function;
    uint8_t _pad_32[2];
    Point2D primary_scale;
    Point2D secondary_scale;
    Point2D tertiary_scale;
    Point2D primary_offset;
    Point2D secondary_offset;
    Point2D tertiary_offset;
    TagDependency primary;  // bitmap
    TagDependency secondary;  // bitmap
    TagDependency tertiary;  // bitmap
    HUDInterfaceWrapMode_t primary_wrap_mode;
    HUDInterfaceWrapMode_t secondary_wrap_mode;
    HUDInterfaceWrapMode_t tertiary_wrap_mode;
    uint8_t _pad_9a[2];
    uint8_t _pad_9c[184];
    TagReflexive effectors;  // HUDInterfaceMultitextureOverlayEffector
    uint8_t _pad_160[128];
} HUDInterfaceMultitextureOverlay;  // size 0x1e0
typedef uint8_t HUDInterfaceNumberFlags;  // bitfield: show_leading_zeros, only_show_when_zoomed, draw_a_trailing_m
typedef uint8_t HUDInterfaceMessagingFlags;  // bitfield: use_text_from_string_list_instead, override_default_color, width_offset_is_absolute_icon_width
typedef struct GrenadeHUDInterface {
    HUDInterfaceAnchor_t anchor;
    HUDInterfaceCanvasSize_t canvas_size;
    uint8_t _pad_4[32];
    Point2DInt background_anchor_offset;
    float background_width_scale;
    float background_height_scale;
    HUDInterfaceScalingFlags background_scaling_flags;
    uint8_t _pad_32[2];
    uint8_t _pad_34[20];
    TagDependency background_interface_bitmap;  // bitmap
    ColorARGBInt background_default_color;
    ColorARGBInt background_flashing_color;
    float background_flash_period;
    float background_flash_delay;
    int16_t background_number_of_flashes;
    HUDInterfaceFlashFlags background_flash_flags;
    float background_flash_length;
    ColorARGBInt background_disabled_color;
    uint8_t _pad_74[4];
    uint16_t background_sequence_index;
    uint8_t _pad_7a[2];
    TagReflexive background_multitexture_overlays;  // HUDInterfaceMultitextureOverlay
    uint8_t _pad_88[4];
    Point2DInt total_grenades_background_anchor_offset;
    float total_grenades_background_width_scale;
    float total_grenades_background_height_scale;
    HUDInterfaceScalingFlags total_grenades_background_scaling_flags;
    uint8_t _pad_9a[2];
    uint8_t _pad_9c[20];
    TagDependency total_grenades_background_interface_bitmap;  // bitmap
    ColorARGBInt total_grenades_background_default_color;
    ColorARGBInt total_grenades_background_flashing_color;
    float total_grenades_background_flash_period;
    float total_grenades_background_flash_delay;
    int16_t total_grenades_background_number_of_flashes;
    HUDInterfaceFlashFlags total_grenades_background_flash_flags;
    float total_grenades_background_flash_length;
    ColorARGBInt total_grenades_background_disabled_color;
    uint8_t _pad_dc[4];
    uint16_t total_grenades_background_sequence_index;
    uint8_t _pad_e2[2];
    TagReflexive total_grenades_background_multitexture_overlays;  // HUDInterfaceMultitextureOverlay
    uint8_t _pad_f0[4];
    Point2DInt total_grenades_numbers_anchor_offset;
    float total_grenades_numbers_width_scale;
    float total_grenades_numbers_height_scale;
    HUDInterfaceScalingFlags total_grenades_numbers_scaling_flags;
    uint8_t _pad_102[2];
    uint8_t _pad_104[20];
    ColorARGBInt total_grenades_numbers_default_color;
    ColorARGBInt total_grenades_numbers_flashing_color;
    float total_grenades_numbers_flash_period;
    float total_grenades_numbers_flash_delay;
    int16_t total_grenades_numbers_number_of_flashes;
    HUDInterfaceFlashFlags total_grenades_numbers_flash_flags;
    float total_grenades_numbers_flash_length;
    ColorARGBInt total_grenades_numbers_disabled_color;
    uint8_t _pad_134[4];
    int8_t total_grenades_numbers_maximum_number_of_digits;
    HUDInterfaceNumberFlags total_grenades_numbers_flags;
    int8_t total_grenades_numbers_number_of_fractional_digits;
    uint8_t _pad_13b[1];
    uint8_t _pad_13c[12];
    int16_t flash_cutoff;
    uint8_t _pad_14a[2];
    TagDependency total_grenades_overlay_bitmap;  // bitmap
    TagReflexive total_grenades_overlays;  // GrenadeHUDInterfaceOverlay
    TagReflexive total_grenades_warning_sounds;  // GrenadeHUDInterfaceSound
    uint8_t _pad_174[68];
    uint16_t messaging_information_sequence_index;
    int16_t messaging_information_width_offset;
    Point2DInt messaging_information_offset_from_reference_corner;
    ColorARGBInt messaging_information_override_icon_color;
    int8_t messaging_information_frame_rate;
    HUDInterfaceMessagingFlags messaging_information_flags;
    uint16_t messaging_information_text_index;
    uint8_t _pad_1c8[48];
} GrenadeHUDInterface;  // size 0x1f8
typedef uint32_t HUDGlobalsWaypointArrowFlags;  // bitfield: dont_rotate_when_pointing_offscreen
typedef struct HUDGlobalsButtonIcon {
    uint16_t sequence_index;
    int16_t width_offset;
    Point2DInt offset_from_reference_corner;
    ColorARGBInt override_icon_color;
    int8_t frame_rate;
    HUDInterfaceMessagingFlags flags;
    uint16_t text_index;
} HUDGlobalsButtonIcon;  // size 0x10
typedef enum HUDGlobalsAnniversaryRemapTargetLanguage {
    hudglobalsanniversaryremaptargetlanguage_english = 0,
    hudglobalsanniversaryremaptargetlanguage_french = 1,
    hudglobalsanniversaryremaptargetlanguage_spanish = 2,
    hudglobalsanniversaryremaptargetlanguage_italian = 3,
    hudglobalsanniversaryremaptargetlanguage_german = 4,
    hudglobalsanniversaryremaptargetlanguage_traditional_chinese = 5,
    hudglobalsanniversaryremaptargetlanguage_japanese = 6,
    hudglobalsanniversaryremaptargetlanguage_korean = 7,
    hudglobalsanniversaryremaptargetlanguage_portuguese = 8,
    hudglobalsanniversaryremaptargetlanguage_latam_spanish = 9,
    hudglobalsanniversaryremaptargetlanguage_polish = 10,
    hudglobalsanniversaryremaptargetlanguage_russian = 11,
    hudglobalsanniversaryremaptargetlanguage_simplified_chinese = 12,
} HUDGlobalsAnniversaryRemapTargetLanguage;  // int16
typedef int16_t HUDGlobalsAnniversaryRemapTargetLanguage_t;
typedef uint16_t HUDGlobalsAnniversaryRemapTargetFlags;  // bitfield: legacy_mode
typedef struct HUDGlobalsAnniversaryRemapTarget {
    TagDependency target_bitmap;  // bitmap
    HUDGlobalsAnniversaryRemapTargetLanguage_t language;
    HUDGlobalsAnniversaryRemapTargetFlags flags;
} HUDGlobalsAnniversaryRemapTarget;  // size 0x14
typedef struct HUDGlobalsAnniversaryRemap {
    TagDependency source_bitmap;  // bitmap
    TagReflexive targets;  // HUDGlobalsAnniversaryRemapTarget
} HUDGlobalsAnniversaryRemap;  // size 0x1c
typedef struct HUDGlobalsWaypointArrow {
    TagString name;
    uint8_t _pad_20[8];
    ColorARGBInt color;
    float opacity;
    float translucency;
    uint16_t on_screen_sequence_index;
    uint16_t off_screen_sequence_index;
    uint16_t occluded_sequence_index;
    uint8_t _pad_3a[2];
    uint8_t _pad_3c[16];
    HUDGlobalsWaypointArrowFlags flags;
    uint8_t _pad_50[24];
} HUDGlobalsWaypointArrow;  // size 0x68
typedef struct HUDGlobals {
    HUDInterfaceAnchor_t anchor;
    HUDInterfaceCanvasSize_t canvas_size;
    uint8_t _pad_4[32];
    Point2DInt anchor_offset;
    float width_scale;
    float height_scale;
    HUDInterfaceScalingFlags scaling_flags;
    uint8_t _pad_32[2];
    uint8_t _pad_34[20];
    TagDependency fullscreen_font;  // font
    TagDependency splitscreen_font;  // font
    float up_time;
    float fade_time;
    ColorARGB icon_color;
    ColorARGB text_color;
    float text_spacing;
    TagDependency item_message_text;  // unicode_string_list
    TagDependency icon_bitmap;  // bitmap
    TagDependency alternate_icon_text;  // unicode_string_list
    TagReflexive button_icons;  // HUDGlobalsButtonIcon
    ColorARGBInt hud_help_default_color;
    ColorARGBInt hud_help_flashing_color;
    float hud_help_flash_period;
    float hud_help_flash_delay;
    int16_t hud_help_number_of_flashes;
    HUDInterfaceFlashFlags hud_help_flash_flags;
    float hud_help_flash_length;
    ColorARGBInt hud_help_disabled_color;
    uint8_t _pad_ec[4];
    TagDependency hud_messages;  // hud_message_text
    ColorARGBInt objective_default_color;
    ColorARGBInt objective_flashing_color;
    float objective_flash_period;
    float objective_flash_delay;
    int16_t objective_number_of_flashes;
    HUDInterfaceFlashFlags objective_flash_flags;
    float objective_flash_length;
    ColorARGBInt objective_disabled_color;
    int16_t objective_uptime_ticks;
    int16_t objective_fade_ticks;
    float top_offset;
    float bottom_offset;
    float left_offset;
    float right_offset;
    uint8_t _pad_130[32];
    TagDependency arrow_bitmap;  // bitmap
    TagReflexive waypoint_arrows;  // HUDGlobalsWaypointArrow
    uint8_t _pad_16c[80];
    float hud_scale_in_multiplayer;
    uint8_t _pad_1c0[256];
    TagDependency default_weapon_hud;  // weapon_hud_interface
    float motion_sensor_range;
    float motion_sensor_velocity_sensitivity;
    float motion_sensor_scale;
    Rectangle2D default_chapter_title_bounds;
    uint8_t _pad_2e4[44];
    int16_t hud_damage_top_offset;
    int16_t hud_damage_bottom_offset;
    int16_t hud_damage_left_offset;
    int16_t hud_damage_right_offset;
    uint8_t _pad_318[32];
    TagDependency hud_damage_indicator_bitmap;  // bitmap
    uint16_t hud_damage_sequence_index;
    uint16_t hud_damage_multiplayer_sequence_index;
    ColorARGBInt hud_damage_color;
    uint8_t _pad_350[16];
    ColorARGBInt not_much_time_left_default_color;
    ColorARGBInt not_much_time_left_flashing_color;
    float not_much_time_left_flash_period;
    float not_much_time_left_flash_delay;
    int16_t not_much_time_left_number_of_flashes;
    HUDInterfaceFlashFlags not_much_time_left_flash_flags;
    float not_much_time_left_flash_length;
    ColorARGBInt not_much_time_left_disabled_color;
    uint8_t _pad_37c[4];
    ColorARGBInt time_out_flash_default_color;
    ColorARGBInt time_out_flash_flashing_color;
    float time_out_flash_flash_period;
    float time_out_flash_flash_delay;
    int16_t time_out_flash_number_of_flashes;
    HUDInterfaceFlashFlags time_out_flash_flash_flags;
    float time_out_flash_flash_length;
    ColorARGBInt time_out_flash_disabled_color;
    uint8_t _pad_39c[4];
    uint8_t _pad_3a0[40];
    TagDependency carnage_report_bitmap;  // bitmap
    uint16_t loading_begin_text;
    uint16_t loading_end_text;
    uint16_t checkpoint_begin_text;
    uint16_t checkpoint_end_text;
    TagDependency checkpoint_sound;  // sound
    TagReflexive anniversary_hud_remaps;  // HUDGlobalsAnniversaryRemap
    uint8_t _pad_3fc[84];
} HUDGlobals;  // size 0x450
typedef enum HUDInterfaceChildAnchor {
    hudinterfacechildanchor_from_parent = 0,
    hudinterfacechildanchor_top_left = 1,
    hudinterfacechildanchor_top_right = 2,
    hudinterfacechildanchor_bottom_left = 3,
    hudinterfacechildanchor_bottom_right = 4,
    hudinterfacechildanchor_center = 5,
    hudinterfacechildanchor_top_center = 6,
    hudinterfacechildanchor_bottom_center = 7,
    hudinterfacechildanchor_left_center = 8,
    hudinterfacechildanchor_right_center = 9,
} HUDInterfaceChildAnchor;  // int16
typedef int16_t HUDInterfaceChildAnchor_t;
typedef uint8_t HUDInterfaceMeterFlags;  // bitfield: use_min_max_for_state_changes, interpolate_between_min_max_flash_colors_as_state_changes, interpolate_color_along_hsv_space, more_colors_for_hsv_interpolation, invert_interpolation, use_xbox_shading
typedef struct HUDMessageTextElement {
    int8_t type;
    uint8_t data;
} HUDMessageTextElement;  // size 0x2
typedef struct HUDMessageTextMessage {
    TagString name;
    uint16_t start_index_into_text_blob;
    uint16_t start_index_of_message_block;
    uint8_t panel_count;
    uint8_t _pad_25[3];
    uint8_t _pad_28[24];
} HUDMessageTextMessage;  // size 0x40
typedef struct HUDMessageText {
    TagDataOffset text_data;
    TagReflexive message_elements;  // HUDMessageTextElement
    TagReflexive messages;  // HUDMessageTextMessage
    uint8_t _pad_2c[84];
} HUDMessageText;  // size 0x80
typedef struct HUDNumber {
    TagDependency digits_bitmap;  // bitmap
    int8_t bitmap_digit_width;
    int8_t screen_digit_width;
    int8_t x_offset;
    int8_t y_offset;
    int8_t decimal_point_width;
    int8_t colon_width;
    uint8_t _pad_16[2];
    uint8_t _pad_18[76];
} HUDNumber;  // size 0x64
typedef enum InputDeviceDefaultsDeviceType {
    inputdevicedefaultsdevicetype_mouse_and_keyboard = 0,
    inputdevicedefaultsdevicetype_joysticks_gamepads_etc = 1,
    inputdevicedefaultsdevicetype_full_profile_definition = 2,
} InputDeviceDefaultsDeviceType;  // int16
typedef int16_t InputDeviceDefaultsDeviceType_t;
typedef uint16_t InputDeviceDefaultsFlags;  // bitfield: unused
typedef struct InputDeviceDefaults {
    InputDeviceDefaultsDeviceType_t device_type;
    InputDeviceDefaultsFlags flags;
    TagDataOffset device_id;
    TagDataOffset profile;
} InputDeviceDefaults;  // size 0x2c
typedef struct ItemCollectionPermutation {
    uint8_t _pad_0[32];
    float weight;
    TagDependency item;  // item
    uint8_t _pad_34[32];
} ItemCollectionPermutation;  // size 0x54
typedef struct ItemCollection {
    TagReflexive permutations;  // ItemCollectionPermutation
    int16_t default_spawn_time;
    uint8_t _pad_e[2];
    uint8_t _pad_10[76];
} ItemCollection;  // size 0x5c
typedef enum LensFlareRadiusScaledBy {
    lensflareradiusscaledby_none = 0,
    lensflareradiusscaledby_rotation = 1,
    lensflareradiusscaledby_rotation_and_strafing = 2,
    lensflareradiusscaledby_distance_from_center = 3,
} LensFlareRadiusScaledBy;  // int16
typedef int16_t LensFlareRadiusScaledBy_t;
typedef enum LensFlareOcclusionOffsetDirection {
    lensflareocclusionoffsetdirection_toward_viewer = 0,
    lensflareocclusionoffsetdirection_marker_forward = 1,
    lensflareocclusionoffsetdirection_none = 2,
} LensFlareOcclusionOffsetDirection;  // int16
typedef int16_t LensFlareOcclusionOffsetDirection_t;
typedef enum LensFlareRotationFunction {
    lensflarerotationfunction_none = 0,
    lensflarerotationfunction_rotation_a = 1,
    lensflarerotationfunction_rotation_b = 2,
    lensflarerotationfunction_rotation_translation = 3,
    lensflarerotationfunction_translation = 4,
} LensFlareRotationFunction;  // int16
typedef int16_t LensFlareRotationFunction_t;
typedef uint16_t LensFlareReflectionFlags;  // bitfield: align_rotation_with_screen_center, radius_not_scaled_by_distance, radius_scaled_by_occlusion_factor, occluded_by_solid_objects
typedef uint16_t LensFlareReflectionMoreFlags;  // bitfield: interpolate_colors_in_hsv, more_colors
typedef uint16_t LensFlareFlags;  // bitfield: sun, no_occlusion_test, only_render_in_first_person, only_render_in_third_person, fade_in_more_quickly, fade_out_more_quickly, scale_by_marker
typedef struct LensFlareReflection {
    LensFlareReflectionFlags flags;
    uint8_t _pad_2[2];
    uint16_t bitmap_index;
    uint8_t _pad_6[2];
    uint8_t _pad_8[20];
    float position;
    float rotation_offset;
    uint8_t _pad_24[4];
    float radius[2];
    LensFlareRadiusScaledBy_t radius_scaled_by;
    uint8_t _pad_32[2];
    float brightness[2];
    LensFlareRadiusScaledBy_t brightness_scaled_by;
    uint8_t _pad_3e[2];
    ColorARGB tint_color;
    ColorARGB color_lower_bound;
    ColorARGB color_upper_bound;
    LensFlareReflectionMoreFlags more_flags;
    WaveFunction_t animation_function;
    float animation_period;
    float animation_phase;
    uint8_t _pad_7c[4];
} LensFlareReflection;  // size 0x80
typedef struct LensFlare {
    float falloff_angle;
    float cutoff_angle;
    float cos_falloff_angle;
    float cos_cutoff_angle;
    float occlusion_radius;
    LensFlareOcclusionOffsetDirection_t occlusion_offset_direction;
    uint8_t _pad_16[2];
    float near_fade_distance;
    float far_fade_distance;
    TagDependency bitmap;  // bitmap
    LensFlareFlags flags;
    uint8_t _pad_32[2];
    uint8_t _pad_34[76];
    LensFlareRotationFunction_t rotation_function;
    uint8_t _pad_82[2];
    float rotation_function_scale;
    uint8_t _pad_88[24];
    float horizontal_scale;
    float vertical_scale;
    uint8_t _pad_a8[28];
    TagReflexive reflections;  // LensFlareReflection
    uint8_t _pad_d0[32];
} LensFlare;  // size 0xf0
typedef uint32_t LightFlags;  // bitfield: dynamic, no_specular, don_t_light_own_object, supersize_in_first_person, first_person_flashlight, don_t_fade_active_camouflage
typedef struct Light {
    LightFlags flags;
    float radius;
    float radius_modifer[2];
    float falloff_angle;
    float cutoff_angle;
    float lens_flare_only_radius;
    float cos_falloff_angle;
    float cos_cutoff_angle;
    float specular_radius_multiplier;
    float sin_cutoff_angle;
    uint8_t _pad_2c[8];
    ColorInterpolationFlags interpolation_flags;
    ColorARGB color_lower_bound;
    ColorARGB color_upper_bound;
    uint8_t _pad_58[12];
    TagDependency primary_cube_map;  // bitmap
    uint8_t _pad_74[2];
    WaveFunction_t texture_animation_function;
    float texture_animation_period;
    TagDependency secondary_cube_map;  // bitmap
    uint8_t _pad_8c[2];
    WaveFunction_t yaw_function;
    float yaw_period;
    uint8_t _pad_94[2];
    WaveFunction_t roll_function;
    float roll_period;
    uint8_t _pad_9c[2];
    WaveFunction_t pitch_function;
    float pitch_period;
    uint8_t _pad_a4[8];
    TagDependency lens_flare;  // lens_flare
    uint8_t _pad_bc[24];
    float intensity;
    ColorRGB color;
    uint8_t _pad_e4[16];
    float duration;
    uint8_t _pad_f8[2];
    FunctionType_t falloff_function;
    uint8_t _pad_fc[8];
    uint8_t _pad_104[92];
} Light;  // size 0x160
typedef uint16_t LightVolumeFlags;  // bitfield: interpolate_color_in_hsv, more_colors
typedef struct LightVolumeFrame {
    uint8_t _pad_0[16];
    float offset_from_marker;
    float offset_exponent;
    float length;
    uint8_t _pad_1c[32];
    float radius_hither;
    float radius_yon;
    float radius_exponent;
    uint8_t _pad_48[32];
    ColorARGB tint_color_hither;
    ColorARGB tint_color_yon;
    float tint_color_exponent;
    float brightness_exponent;
    uint8_t _pad_90[32];
} LightVolumeFrame;  // size 0xb0
typedef struct LightVolume {
    TagString attachment_marker;
    uint8_t _pad_20[2];
    LightVolumeFlags flags;
    uint8_t _pad_24[16];
    float near_fade_distance;
    float far_fade_distance;
    float perpendicular_brightness_scale;
    float parallel_brightness_scale;
    FunctionOut_t brightness_scale_source;
    uint8_t _pad_46[2];
    uint8_t _pad_48[20];
    TagDependency map;  // bitmap
    uint16_t sequence_index;
    int16_t count;
    uint8_t _pad_70[72];
    FunctionOut_t frame_animation_source;
    uint8_t _pad_ba[2];
    uint8_t _pad_bc[36];
    uint8_t _pad_e0[64];
    TagReflexive frames;  // LightVolumeFrame
    uint8_t _pad_12c[32];
} LightVolume;  // size 0x14c
typedef uint16_t LightningMarkerFlag;  // bitfield: not_connected_to_next_marker
typedef struct LightningMarker {
    TagString attachment_marker;
    LightningMarkerFlag flags;
    uint8_t _pad_22[2];
    int16_t octaves_to_next_marker;
    uint8_t _pad_26[2];
    uint8_t _pad_28[76];
    Vector3D random_position_bounds;
    float random_jitter;
    float thickness;
    ColorARGB tint;
    uint8_t _pad_98[76];
} LightningMarker;  // size 0xe4
typedef struct LightningShader {
    uint8_t _pad_0[36];
    ShaderType_t shader_type;
    uint8_t _pad_26[2];
    ParticleShaderFlags shader_flags;
    FramebufferBlendFunction_t framebuffer_blend_function;
    FramebufferFadeMode_t framebuffer_fade_mode;
    IsUnfilteredFlag bitmap_flags;
    uint8_t _pad_30[28];
    TagDependency secondary_bitmap;  // bitmap
    ParticleAnchor_t anchor;
    IsUnfilteredFlag secondary_bitmap_flags;
    FunctionOut_t u_animation_source;
    WaveFunction_t u_animation_function;
    float u_animation_period;
    float u_animation_phase;
    float u_animation_scale;
    FunctionOut_t v_animation_source;
    WaveFunction_t v_animation_function;
    float v_animation_period;
    float v_animation_phase;
    float v_animation_scale;
    FunctionOut_t rotation_animation_source;
    WaveFunction_t rotation_animation_function;
    float rotation_animation_period;
    float rotation_animation_phase;
    float rotation_animation_scale;
    Point2D rotation_animation_center;
    uint8_t _pad_98[4];
    float zsprite_radius_scale;
    uint8_t _pad_a0[20];
} LightningShader;  // size 0xb4
typedef struct Lightning {
    uint8_t _pad_0[2];
    int16_t count;
    uint8_t _pad_4[16];
    float near_fade_distance;
    float far_fade_distance;
    uint8_t _pad_1c[16];
    FunctionOut_t jitter_scale_source;
    FunctionOut_t thickness_scale_source;
    FunctionNameNullable_t tint_modulation_source;
    FunctionOut_t brightness_scale_source;
    TagDependency bitmap;  // bitmap
    uint8_t _pad_44[84];
    TagReflexive markers;  // LightningMarker
    TagReflexive shader;  // LightningShader
    uint8_t _pad_b0[88];
} Lightning;  // size 0x108
typedef struct MaterialEffectsMaterialEffectMaterial {
    TagDependency effect;  // effect
    TagDependency sound;  // sound
    uint8_t _pad_20[16];
} MaterialEffectsMaterialEffectMaterial;  // size 0x30
typedef struct MaterialEffectsMaterialEffect {
    TagReflexive materials;  // MaterialEffectsMaterialEffectMaterial
    uint8_t _pad_c[16];
} MaterialEffectsMaterialEffect;  // size 0x1c
typedef struct MaterialEffects {
    TagReflexive effects;  // MaterialEffectsMaterialEffect
    uint8_t _pad_c[128];
} MaterialEffects;  // size 0x8c
typedef enum MeterInterpolateColors {
    meterinterpolatecolors_linearly = 0,
    meterinterpolatecolors_faster_near_empty = 1,
    meterinterpolatecolors_faster_near_full = 2,
    meterinterpolatecolors_through_random_noise = 3,
} MeterInterpolateColors;  // int16
typedef int16_t MeterInterpolateColors_t;
typedef enum MeterAnchorColors {
    meteranchorcolors_at_both_ends = 0,
    meteranchorcolors_at_empty = 1,
    meteranchorcolors_at_full = 2,
} MeterAnchorColors;  // int16
typedef int16_t MeterAnchorColors_t;
typedef struct Meter {
    IsUnusedFlag flags;
    TagDependency stencil_bitmaps;  // bitmap
    TagDependency source_bitmap;  // bitmap
    uint16_t stencil_sequence_index;
    uint16_t source_sequence_index;
    uint8_t _pad_28[16];
    uint8_t _pad_38[4];
    MeterInterpolateColors_t interpolate_colors;
    MeterAnchorColors_t anchor_colors;
    uint8_t _pad_40[8];
    ColorARGB empty_color;
    ColorARGB full_color;
    uint8_t _pad_68[20];
    float unmask_distance;
    float mask_distance;
    uint8_t _pad_84[20];
    TagDataOffset encoded_stencil;
} Meter;  // size 0xac
typedef struct ModelGeometry {
    IsUnusedFlag flags;
    uint8_t _pad_4[32];
    TagReflexive parts;  // ModelGeometryPart
} ModelGeometry;  // size 0x30
typedef struct Model {
    ModelFlags flags;
    int32_t node_list_checksum;
    float super_high_detail_cutoff;
    float high_detail_cutoff;
    float medium_detail_cutoff;
    float low_detail_cutoff;
    float super_low_detail_cutoff;
    uint16_t super_low_detail_node_count;
    uint16_t low_detail_node_count;
    uint16_t medium_detail_node_count;
    uint16_t high_detail_node_count;
    uint16_t super_high_detail_node_count;
    uint8_t _pad_26[2];
    uint8_t _pad_28[8];
    float base_map_u_scale;
    float base_map_v_scale;
    uint8_t _pad_38[116];
    TagReflexive markers;  // ModelMarker
    TagReflexive nodes;  // ModelNode
    TagReflexive regions;  // ModelRegion
    TagReflexive geometries;  // ModelGeometry
    TagReflexive shaders;  // ModelShaderReference
} Model;  // size 0xe8
typedef enum AnimationFunction {
    animationfunction_a_out = 0,
    animationfunction_b_out = 1,
    animationfunction_c_out = 2,
    animationfunction_d_out = 3,
} AnimationFunction;  // int16
typedef int16_t AnimationFunction_t;
typedef enum AnimationFunctionControls {
    animationfunctioncontrols_frame = 0,
    animationfunctioncontrols_scale = 1,
} AnimationFunctionControls;  // int16
typedef int16_t AnimationFunctionControls_t;
typedef enum AnimationType {
    animationtype_base = 0,
    animationtype_overlay = 1,
    animationtype_replacement = 2,
} AnimationType;  // int16
typedef int16_t AnimationType_t;
typedef enum AnimationFrameInfoType {
    animationframeinfotype_none = 0,
    animationframeinfotype_dx_dy = 1,
    animationframeinfotype_dx_dy_dyaw = 2,
    animationframeinfotype_dx_dy_dz_dyaw = 3,
} AnimationFrameInfoType;  // int16
typedef int16_t AnimationFrameInfoType_t;
typedef uint32_t ModelAnimationsAnimationGraphNodeFlags;  // bitfield: ball_socket, hinge, no_movement
typedef uint16_t ModelAnimationsAnimationFlags;  // bitfield: compressed_data, world_relative, _25hz_pal
typedef uint16_t ModelAnimationsFlags;  // bitfield: compress_all_animations, force_idle_compression
typedef struct ModelAnimationsRotation {
    int16_t rotation[4];
} ModelAnimationsRotation;  // size 0x8
typedef struct ModelAnimationsTransform {
    Point3D transform;
} ModelAnimationsTransform;  // size 0xc
typedef struct ModelAnimationscale {
    float scale;
} ModelAnimationscale;  // size 0x4
typedef struct ModelAnimationsFrameInfoDxDy {
    float dx;
    float dy;
} ModelAnimationsFrameInfoDxDy;  // size 0x8
typedef struct ModelAnimationsFrameInfoDxDyDyaw {
    float dx;
    float dy;
    float dyaw;
} ModelAnimationsFrameInfoDxDyDyaw;  // size 0xc
typedef struct ModelAnimationsFrameInfoDxDyDzDyaw {
    float dx;
    float dy;
    float dz;
    float dyaw;
} ModelAnimationsFrameInfoDxDyDzDyaw;  // size 0x10
typedef struct ModelAnimationsAnimationGraphObjectOverlay {
    uint16_t animation;
    AnimationFunction_t function;
    AnimationFunctionControls_t function_controls;
    uint8_t _pad_6[2];
    uint8_t _pad_8[12];
} ModelAnimationsAnimationGraphObjectOverlay;  // size 0x14
typedef struct ModelAnimationsUnitSeatAnimation {
    uint16_t animation;
} ModelAnimationsUnitSeatAnimation;  // size 0x2
typedef struct ModelAnimationsAnimationGraphUnitSeatikPoint {
    TagString marker;
    TagString attach_to_marker;
} ModelAnimationsAnimationGraphUnitSeatikPoint;  // size 0x40
typedef struct ModelAnimationsAnimationWeaponClassAnimation {
    uint16_t animation;
} ModelAnimationsAnimationWeaponClassAnimation;  // size 0x2
typedef struct ModelAnimationsAnimationWeaponTypeAnimation {
    uint16_t animation;
} ModelAnimationsAnimationWeaponTypeAnimation;  // size 0x2
typedef struct ModelAnimationsAnimationGraphWeaponType {
    TagString label;
    uint8_t _pad_20[16];
    TagReflexive animations;  // ModelAnimationsAnimationWeaponTypeAnimation
} ModelAnimationsAnimationGraphWeaponType;  // size 0x3c
typedef struct ModelAnimationsAnimationGraphWeapon {
    TagString name;
    TagString grip_marker;
    TagString hand_marker;
    float right_yaw_per_frame;
    float left_yaw_per_frame;
    uint16_t right_frame_count;
    uint16_t left_frame_count;
    float down_pitch_per_frame;
    float up_pitch_per_frame;
    uint16_t down_pitch_frame_count;
    uint16_t up_pitch_frame_count;
    uint8_t _pad_78[32];
    TagReflexive animations;  // ModelAnimationsAnimationWeaponClassAnimation
    TagReflexive ik_point;  // ModelAnimationsAnimationGraphUnitSeatikPoint
    TagReflexive weapon_types;  // ModelAnimationsAnimationGraphWeaponType
} ModelAnimationsAnimationGraphWeapon;  // size 0xbc
typedef struct ModelAnimationsAnimationGraphUnitSeat {
    TagString label;
    float right_yaw_per_frame;
    float left_yaw_per_frame;
    uint16_t right_frame_count;
    uint16_t left_frame_count;
    float down_pitch_per_frame;
    float up_pitch_per_frame;
    uint16_t down_pitch_frame_count;
    uint16_t up_pitch_frame_count;
    uint8_t _pad_38[8];
    TagReflexive animations;  // ModelAnimationsAnimationWeaponClassAnimation
    TagReflexive ik_points;  // ModelAnimationsAnimationGraphUnitSeatikPoint
    TagReflexive weapons;  // ModelAnimationsAnimationGraphWeapon
} ModelAnimationsAnimationGraphUnitSeat;  // size 0x64
typedef struct ModelAnimationsWeaponAnimation {
    uint16_t animation;
} ModelAnimationsWeaponAnimation;  // size 0x2
typedef struct ModelAnimationsAnimationGraphWeaponAnimations {
    uint8_t _pad_0[16];
    TagReflexive animations;  // ModelAnimationsWeaponAnimation
} ModelAnimationsAnimationGraphWeaponAnimations;  // size 0x1c
typedef struct ModelAnimationsVehicleAnimation {
    uint16_t animation;
} ModelAnimationsVehicleAnimation;  // size 0x2
typedef struct ModelAnimationSuspensionAnimation {
    uint16_t mass_point_index;
    uint16_t animation;
    float full_extension_ground_depth;
    float full_compression_ground_depth;
    uint8_t _pad_c[8];
} ModelAnimationSuspensionAnimation;  // size 0x14
typedef struct ModelAnimationsAnimationGraphVehicleAnimations {
    float right_yaw_per_frame;
    float left_yaw_per_frame;
    uint16_t right_frame_count;
    uint16_t left_frame_count;
    float down_pitch_per_frame;
    float up_pitch_per_frame;
    uint16_t down_pitch_frame_count;
    uint16_t up_pitch_frame_count;
    uint8_t _pad_18[68];
    TagReflexive animations;  // ModelAnimationsVehicleAnimation
    TagReflexive suspension_animations;  // ModelAnimationSuspensionAnimation
} ModelAnimationsAnimationGraphVehicleAnimations;  // size 0x74
typedef struct ModelAnimationsDeviceAnimation {
    uint16_t animation;
} ModelAnimationsDeviceAnimation;  // size 0x2
typedef struct ModelAnimationsDeviceAnimations {
    uint8_t _pad_0[84];
    TagReflexive animations;  // ModelAnimationsDeviceAnimation
} ModelAnimationsDeviceAnimations;  // size 0x60
typedef struct ModelAnimationsUnitDamageAnimations {
    uint16_t animation;
} ModelAnimationsUnitDamageAnimations;  // size 0x2
typedef struct ModelAnimationsFirstPersonWeapon {
    uint16_t animation;
} ModelAnimationsFirstPersonWeapon;  // size 0x2
typedef struct ModelAnimationsAnimationGraphFirstPersonWeaponAnimations {
    uint8_t _pad_0[16];
    TagReflexive animations;  // ModelAnimationsFirstPersonWeapon
} ModelAnimationsAnimationGraphFirstPersonWeaponAnimations;  // size 0x1c
typedef struct ModelAnimationsAnimationGraphSoundReference {
    TagDependency sound;  // sound
    uint8_t _pad_10[4];
} ModelAnimationsAnimationGraphSoundReference;  // size 0x14
typedef struct ModelAnimationsAnimationGraphNode {
    TagString name;
    uint16_t next_sibling_node_index;
    uint16_t first_child_node_index;
    uint16_t parent_node_index;
    uint8_t _pad_26[2];
    ModelAnimationsAnimationGraphNodeFlags node_joint_flags;
    Vector3D base_vector;
    float vector_range;
    uint8_t _pad_3c[4];
} ModelAnimationsAnimationGraphNode;  // size 0x40
typedef struct ModelAnimationsAnimation {
    TagString name;
    AnimationType_t type;
    uint16_t frame_count;
    uint16_t frame_size;
    AnimationFrameInfoType_t frame_info_type;
    int32_t node_list_checksum;
    uint16_t node_count;
    uint16_t loop_frame_index;
    float weight;
    uint16_t key_frame_index;
    uint16_t second_key_frame_index;
    uint16_t next_animation;
    ModelAnimationsAnimationFlags flags;
    uint16_t sound;
    uint16_t sound_frame_index;
    int8_t left_foot_frame_index;
    int8_t right_foot_frame_index;
    uint16_t main_animation_index;
    float relative_weight;
    TagDataOffset frame_info;
    uint32_t node_transform_flag_data[2];
    uint8_t _pad_64[8];
    uint32_t node_rotation_flag_data[2];
    uint8_t _pad_74[8];
    uint32_t node_scale_flag_data[2];
    uint8_t _pad_84[4];
    uint32_t offset_to_compressed_data;
    TagDataOffset default_data;
    TagDataOffset frame_data;
} ModelAnimationsAnimation;  // size 0xb4
typedef struct ModelAnimations {
    TagReflexive objects;  // ModelAnimationsAnimationGraphObjectOverlay
    TagReflexive units;  // ModelAnimationsAnimationGraphUnitSeat
    TagReflexive weapons;  // ModelAnimationsAnimationGraphWeaponAnimations
    TagReflexive vehicles;  // ModelAnimationsAnimationGraphVehicleAnimations
    TagReflexive devices;  // ModelAnimationsDeviceAnimations
    TagReflexive unit_damage;  // ModelAnimationsUnitDamageAnimations
    TagReflexive first_person_weapons;  // ModelAnimationsAnimationGraphFirstPersonWeaponAnimations
    TagReflexive sound_references;  // ModelAnimationsAnimationGraphSoundReference
    float limp_body_node_radius;
    ModelAnimationsFlags flags;
    uint8_t _pad_66[2];
    TagReflexive nodes;  // ModelAnimationsAnimationGraphNode
    TagReflexive animations;  // ModelAnimationsAnimation
} ModelAnimations;  // size 0x80
typedef uint32_t ModelCollisionGeometryMaterialFlags;  // bitfield: head
typedef uint32_t ModelCollisionGeometryRegionFlags;  // bitfield: lives_until_object_dies, forces_object_to_die, dies_when_object_dies, dies_when_object_is_damaged, disappears_when_shield_is_off, inhibits_melee_attack, inhibits_weapon_attack, inhibits_walking, forces_drop_weapon, causes_head_maimed_scream
typedef uint16_t ModelCollisionGeometryBSPLeafFlags;  // bitfield: contains_double_sided_surfaces
typedef uint8_t ModelCollisionGeometryBSPSurfaceFlags;  // bitfield: two_sided, invisible, climbable, breakable
typedef uint32_t ModelCollisionGeometryFlags;  // bitfield: takes_shield_damage_for_children, takes_body_damage_for_children, always_shields_friendly_damage, passes_area_damage_to_children, parent_never_takes_body_damage_for_us, only_damaged_by_explosives, only_damaged_while_occupied
typedef struct ModelCollisionGeometryMaterial {
    TagString name;
    ModelCollisionGeometryMaterialFlags flags;
    MaterialType_t material_type;
    uint8_t _pad_26[2];
    float shield_leak_percentage;
    float shield_damage_multiplier;
    uint8_t _pad_30[12];
    float body_damage_multiplier;
    uint8_t _pad_40[8];
} ModelCollisionGeometryMaterial;  // size 0x48
typedef struct ModelCollisionGeometryPermutation {
    TagString name;
} ModelCollisionGeometryPermutation;  // size 0x20
typedef struct ModelCollisionGeometryRegion {
    TagString name;
    ModelCollisionGeometryRegionFlags flags;
    uint8_t _pad_24[4];
    float damage_threshold;
    uint8_t _pad_2c[12];
    TagDependency destroyed_effect;  // effect
    TagReflexive permutations;  // ModelCollisionGeometryPermutation
} ModelCollisionGeometryRegion;  // size 0x54
typedef struct ModelCollisionGeometryModifier {
    uint8_t _pad_0[52];
} ModelCollisionGeometryModifier;  // size 0x34
typedef struct ModelCollisionGeometrySphere {
    uint16_t node;
    uint8_t _pad_2[2];
    uint8_t _pad_4[12];
    Point3D center;
    float radius;
} ModelCollisionGeometrySphere;  // size 0x20
typedef struct ModelCollisionGeometryBSP3DNode {
    uint32_t plane;
    uint32_t back_child;
    uint32_t front_child;
} ModelCollisionGeometryBSP3DNode;  // size 0xc
typedef struct ModelCollisionGeometryBSPPlane {
    Plane3D plane;
} ModelCollisionGeometryBSPPlane;  // size 0x10
typedef struct ModelCollisionGeometryBSPLeaf {
    ModelCollisionGeometryBSPLeafFlags flags;
    uint16_t bsp2d_reference_count;
    uint32_t first_bsp2d_reference;
} ModelCollisionGeometryBSPLeaf;  // size 0x8
typedef struct ModelCollisionGeometryBSP2DReference {
    uint32_t plane;
    uint32_t bsp2d_node;
} ModelCollisionGeometryBSP2DReference;  // size 0x8
typedef struct ModelCollisionGeometryBSP2DNode {
    Plane2D plane;
    uint32_t left_child;
    uint32_t right_child;
} ModelCollisionGeometryBSP2DNode;  // size 0x14
typedef struct ModelCollisionGeometryBSPSurface {
    uint32_t plane;
    uint32_t first_edge;
    ModelCollisionGeometryBSPSurfaceFlags flags;
    int8_t breakable_surface;
    uint16_t material;
} ModelCollisionGeometryBSPSurface;  // size 0xc
typedef struct ModelCollisionGeometryBSPEdge {
    uint32_t start_vertex;
    uint32_t end_vertex;
    uint32_t forward_edge;
    uint32_t reverse_edge;
    uint32_t left_surface;
    uint32_t right_surface;
} ModelCollisionGeometryBSPEdge;  // size 0x18
typedef struct ModelCollisionGeometryBSPVertex {
    Point3D point;
    uint32_t first_edge;
} ModelCollisionGeometryBSPVertex;  // size 0x10
typedef struct ModelCollisionGeometryBSP {
    TagReflexive bsp3d_nodes;  // ModelCollisionGeometryBSP3DNode
    TagReflexive planes;  // ModelCollisionGeometryBSPPlane
    TagReflexive leaves;  // ModelCollisionGeometryBSPLeaf
    TagReflexive bsp2d_references;  // ModelCollisionGeometryBSP2DReference
    TagReflexive bsp2d_nodes;  // ModelCollisionGeometryBSP2DNode
    TagReflexive surfaces;  // ModelCollisionGeometryBSPSurface
    TagReflexive edges;  // ModelCollisionGeometryBSPEdge
    TagReflexive vertices;  // ModelCollisionGeometryBSPVertex
} ModelCollisionGeometryBSP;  // size 0x60
typedef struct ModelCollisionGeometryNode {
    TagString name;
    uint16_t region;
    uint16_t parent_node;
    uint16_t next_sibling_node;
    uint16_t first_child_node;
    uint8_t _pad_28[10];
    int16_t name_thing;
    TagReflexive bsps;  // ModelCollisionGeometryBSP
} ModelCollisionGeometryNode;  // size 0x40
typedef struct ModelCollisionGeometry {
    ModelCollisionGeometryFlags flags;
    uint16_t indirect_damage_material;
    uint8_t _pad_6[2];
    float maximum_body_vitality;
    float body_system_shock;
    uint8_t _pad_10[16];
    uint32_t unknown_20;
    uint8_t _pad_24[4];
    uint8_t _pad_28[28];
    float friendly_damage_resistance;
    uint8_t _pad_48[8];
    uint8_t _pad_50[32];
    TagDependency localized_damage_effect;  // effect
    float area_damage_effect_threshold;
    TagDependency area_damage_effect;  // effect
    float body_damaged_threshold;
    TagDependency body_damaged_effect;  // effect
    TagDependency body_depleted_effect;  // effect
    float body_destroyed_threshold;
    TagDependency body_destroyed_effect;  // effect
    float maximum_shield_vitality;
    uint8_t _pad_d0[2];
    MaterialType_t shield_material_type;
    uint8_t _pad_d4[24];
    FunctionType_t shield_failure_function;
    uint8_t _pad_ee[2];
    float shield_failure_threshold;
    float failing_shield_leak_fraction;
    uint8_t _pad_f8[16];
    float minimum_stun_damage;
    float stun_time;
    float recharge_time;
    uint8_t _pad_114[16];
    uint8_t _pad_124[96];
    float shield_damaged_threshold;
    TagDependency shield_damaged_effect;  // effect
    TagDependency shield_depleted_effect;  // effect
    TagDependency shield_recharging_effect;  // effect
    uint8_t _pad_1b8[8];
    float shield_recharge_rate;
    uint8_t _pad_1c4[112];
    TagReflexive materials;  // ModelCollisionGeometryMaterial
    TagReflexive regions;  // ModelCollisionGeometryRegion
    TagReflexive modifiers;  // ModelCollisionGeometryModifier
    uint8_t _pad_258[16];
    float x[2];
    float y[2];
    float z[2];
    TagReflexive pathfinding_spheres;  // ModelCollisionGeometrySphere
    TagReflexive nodes;  // ModelCollisionGeometryNode
} ModelCollisionGeometry;  // size 0x298
typedef struct MultiplayerScenarioDescriptionScenarioDescription {
    TagDependency descriptive_bitmap;  // bitmap
    TagDependency displayed_map_name;  // unicode_string_list
    TagString scenario_tag_directory_path;
    uint8_t _pad_40[4];
} MultiplayerScenarioDescriptionScenarioDescription;  // size 0x44
typedef struct MultiplayerScenarioDescription {
    TagReflexive multiplayer_scenarios;  // MultiplayerScenarioDescriptionScenarioDescription
} MultiplayerScenarioDescription;  // size 0xc
typedef uint16_t BaseObjectFlags;  // bitfield: off_in_pegasus
typedef struct BasicObject {
    Object base;  // inherits
    uint8_t _pad_17c[2];
    BaseObjectFlags more_flags;
    uint8_t _pad_180[124];
} BasicObject;  // size 0x1fc
typedef enum ParticleOrientation {
    particleorientation_screen_facing = 0,
    particleorientation_parallel_to_direction = 1,
    particleorientation_perpendicular_to_direction = 2,
} ParticleOrientation;  // int16
typedef int16_t ParticleOrientation_t;
typedef uint32_t ParticleFlags;  // bitfield: can_animate_backwards, animation_stops_at_rest, animation_starts_on_random_frame, animate_once_per_frame, dies_at_rest, dies_on_contact_with_structure, tint_from_diffuse_texture, dies_on_contact_with_water, dies_on_contact_with_air, self_illuminated, random_horizontal_mirroring, random_vertical_mirroring
typedef struct Particle {
    ParticleFlags flags;
    TagDependency bitmap;  // bitmap
    TagDependency physics;  // point_physics
    TagDependency sir_marty_exchanged_his_children_for_thine;  // material_effects
    uint8_t _pad_34[4];
    float lifespan[2];
    float fade_in_time;
    float fade_out_time;
    TagDependency collision_effect;  // sound,effect
    TagDependency death_effect;  // sound,effect
    float minimum_size;
    uint8_t _pad_6c[8];
    float radius_animation[2];
    uint8_t _pad_7c[4];
    float animation_rate[2];
    float contact_deterioration;
    float fade_start_size;
    float fade_end_size;
    uint8_t _pad_94[4];
    uint16_t first_sequence_index;
    int16_t initial_sequence_count;
    int16_t looping_sequence_count;
    int16_t final_sequence_count;
    uint8_t _pad_a0[8];
    float sprite_size;
    ParticleOrientation_t orientation;
    uint8_t _pad_ae[2];
    uint8_t _pad_b0[36];
    ShaderType_t shader_type;
    uint8_t _pad_d6[2];
    ParticleShaderFlags shader_flags;
    FramebufferBlendFunction_t framebuffer_blend_function;
    FramebufferFadeMode_t framebuffer_fade_mode;
    IsUnfilteredFlag bitmap_flags;
    uint8_t _pad_e0[28];
    TagDependency secondary_bitmap;  // bitmap
    ParticleAnchor_t anchor;
    IsUnfilteredFlag secondary_bitmap_flags;
    FunctionOut_t u_animation_source;
    WaveFunction_t u_animation_function;
    float u_animation_period;
    float u_animation_phase;
    float u_animation_scale;
    FunctionOut_t v_animation_source;
    WaveFunction_t v_animation_function;
    float v_animation_period;
    float v_animation_phase;
    float v_animation_scale;
    FunctionOut_t rotation_animation_source;
    WaveFunction_t rotation_animation_function;
    float rotation_animation_period;
    float rotation_animation_phase;
    float rotation_animation_scale;
    Point2D rotation_animation_center;
    uint8_t _pad_148[4];
    float zsprite_radius_scale;
    uint8_t _pad_150[20];
} Particle;  // size 0x164
typedef enum ParticleSystemParticleCreationPhysics {
    particlesystemparticlecreationphysics_default = 0,
    particlesystemparticlecreationphysics_explosion = 1,
    particlesystemparticlecreationphysics_jet = 2,
} ParticleSystemParticleCreationPhysics;  // int16
typedef int16_t ParticleSystemParticleCreationPhysics_t;
typedef enum ParticleSystemParticleUpdatePhysics {
    particlesystemparticleupdatephysics_default = 0,
} ParticleSystemParticleUpdatePhysics;  // int16
typedef int16_t ParticleSystemParticleUpdatePhysics_t;
typedef enum ParticleSystemComplexSpriteRenderMode {
    particlesystemcomplexspriterendermode_simple = 0,
    particlesystemcomplexspriterendermode_rotational = 1,
} ParticleSystemComplexSpriteRenderMode;  // int16
typedef int16_t ParticleSystemComplexSpriteRenderMode_t;
typedef enum ParticleSystemSystemUpdatePhysics {
    particlesystemsystemupdatephysics_default = 0,
    particlesystemsystemupdatephysics_explosion = 1,
} ParticleSystemSystemUpdatePhysics;  // int16
typedef int16_t ParticleSystemSystemUpdatePhysics_t;
typedef uint32_t ParticleSystemTypeFlags;  // bitfield: type_states_loop, forward_backward, particle_states_loop, forward_backward_1, particles_die_in_water, particles_die_in_air, particles_die_on_ground, rotational_sprites_animate_sideways, disabled, tint_by_effect_color, initial_count_scales_with_effect, minimum_count_scales_with_effect, creation_rate_scales_with_effect, scale_scales_with_effect, animation_rate_scales_with_effect, rotation_rate_scales_with_effect, do_not_draw_in_first_person, do_not_draw_in_third_person
typedef struct ParticleSystemPhysicsConstant {
    float k;
} ParticleSystemPhysicsConstant;  // size 0x4
typedef struct ParticleSystemTypeStates {
    TagString name;
    float duration_bounds[2];
    float transition_time_bounds[2];
    uint8_t _pad_30[4];
    float scale_multiplier;
    float animation_rate_multiplier;
    float rotation_rate_multiplier;
    ColorARGB color_multiplier;
    float radius_multiplier;
    float minimum_particle_count;
    float particle_creation_rate;
    uint8_t _pad_5c[84];
    ParticleSystemParticleCreationPhysics_t particle_creation_physics;
    ParticleSystemParticleUpdatePhysics_t particle_update_physics;
    TagReflexive physics_constants;  // ParticleSystemPhysicsConstant
} ParticleSystemTypeStates;  // size 0xc0
typedef struct ParticleSystemTypeParticleState {
    TagString name;
    float duration_bounds[2];
    float transition_time_bounds[2];
    TagDependency bitmaps;  // bitmap
    uint16_t sequence_index;
    uint8_t _pad_42[2];
    uint8_t _pad_44[4];
    float scale[2];
    float animation_rate[2];
    float rotation_rate[2];
    ColorARGB color_1;
    ColorARGB color_2;
    float radius_multiplier;
    TagDependency point_physics;  // point_physics
    uint8_t _pad_94[36];
    uint8_t _pad_b8[36];
    ShaderType_t shader_type;
    uint8_t _pad_de[2];
    ParticleShaderFlags shader_flags;
    FramebufferBlendFunction_t framebuffer_blend_function;
    FramebufferFadeMode_t framebuffer_fade_mode;
    IsUnfilteredFlag bitmap_flags;
    uint8_t _pad_e8[28];
    TagDependency secondary_bitmap;  // bitmap
    ParticleAnchor_t anchor;
    IsUnfilteredFlag secondary_bitmap_flags;
    FunctionOut_t u_animation_source;
    WaveFunction_t u_animation_function;
    float u_animation_period;
    float u_animation_phase;
    float u_animation_scale;
    FunctionOut_t v_animation_source;
    WaveFunction_t v_animation_function;
    float v_animation_period;
    float v_animation_phase;
    float v_animation_scale;
    FunctionOut_t rotation_animation_source;
    WaveFunction_t rotation_animation_function;
    float rotation_animation_period;
    float rotation_animation_phase;
    float rotation_animation_scale;
    Point2D rotation_animation_center;
    uint8_t _pad_150[4];
    float zsprite_radius_scale;
    uint8_t _pad_158[20];
    TagReflexive physics_constants;  // ParticleSystemPhysicsConstant
} ParticleSystemTypeParticleState;  // size 0x178
typedef struct ParticleSystemType {
    TagString name;
    ParticleSystemTypeFlags flags;
    uint16_t initial_particle_count;
    uint8_t _pad_26[2];
    ParticleSystemComplexSpriteRenderMode_t complex_sprite_render_mode;
    uint8_t _pad_2a[2];
    float radius;
    uint8_t _pad_30[36];
    ParticleSystemParticleCreationPhysics_t particle_creation_physics;
    uint8_t _pad_56[2];
    IsUnusedFlag physics_flags;
    TagReflexive physics_constants;  // ParticleSystemPhysicsConstant
    TagReflexive states;  // ParticleSystemTypeStates
    TagReflexive particle_states;  // ParticleSystemTypeParticleState
} ParticleSystemType;  // size 0x80
typedef struct ParticleSystem {
    uint8_t _pad_0[4];
    uint8_t _pad_4[52];
    TagDependency point_physics;  // point_physics
    ParticleSystemSystemUpdatePhysics_t system_update_physics;
    uint8_t _pad_4a[2];
    IsUnusedFlag physics_flags;
    TagReflexive physics_constants;  // ParticleSystemPhysicsConstant
    TagReflexive particle_types;  // ParticleSystemType
} ParticleSystem;  // size 0x68
typedef enum PhysicsFrictionType {
    physicsfrictiontype_point = 0,
    physicsfrictiontype_forward = 1,
    physicsfrictiontype_left = 2,
    physicsfrictiontype_up = 3,
} PhysicsFrictionType;  // int16
typedef int16_t PhysicsFrictionType_t;
typedef uint32_t PhysicsPoweredMassPointFlags;  // bitfield: ground_friction, water_friction, air_friction, water_lift, air_lift, thrust, antigrav
typedef uint32_t PhysicsMassPointFlags;  // bitfield: metallic
typedef struct PhysicsInertialMatrix {
    Matrix matrix;
} PhysicsInertialMatrix;  // size 0x24
typedef struct PhysicsPoweredMassPoint {
    TagString name;
    PhysicsPoweredMassPointFlags flags;
    float antigrav_strength;
    float antigrav_offset;
    float antigrav_height;
    float antigrav_damp_fraction;
    float antigrav_normal_k1;
    float antigrav_normal_k0;
    uint8_t _pad_3c[68];
} PhysicsPoweredMassPoint;  // size 0x80
typedef struct PhysicsMassPoint {
    TagString name;
    uint16_t powered_mass_point;
    uint16_t model_node;
    PhysicsMassPointFlags flags;
    float relative_mass;
    float mass;
    float relative_density;
    float density;
    Point3D position;
    Vector3D forward;
    Vector3D up;
    PhysicsFrictionType_t friction_type;
    uint8_t _pad_5e[2];
    float friction_parallel_scale;
    float friction_perpendicular_scale;
    float radius;
    uint8_t _pad_6c[20];
} PhysicsMassPoint;  // size 0x80
typedef struct Physics {
    float radius;
    float moment_scale;
    float mass;
    Point3D center_of_mass;
    float density;
    float gravity_scale;
    float ground_friction;
    float ground_depth;
    float ground_damp_fraction;
    float ground_normal_k1;
    float ground_normal_k0;
    uint8_t _pad_34[4];
    float water_friction;
    float water_depth;
    float water_density;
    uint8_t _pad_44[4];
    float air_friction;
    uint8_t _pad_4c[4];
    float xx_moment;
    float yy_moment;
    float zz_moment;
    TagReflexive inertial_matrix_and_inverse;  // PhysicsInertialMatrix
    TagReflexive powered_mass_points;  // PhysicsPoweredMassPoint
    TagReflexive mass_points;  // PhysicsMassPoint
} Physics;  // size 0x80
typedef struct Placeholder {
    BasicObject base;  // inherits
} Placeholder;  // size 0x1fc
typedef uint32_t PointPhysicsFlags;  // bitfield: flamethrower_particle_collision, collides_with_structures, collides_with_water_surface, uses_simple_wind, uses_damped_wind, no_gravity
typedef struct PointPhysics {
    PointPhysicsFlags flags;
    float mass_scale;
    float water_gravity_scale;
    float air_gravity_scale;
    uint8_t _pad_10[16];
    float density;
    float air_friction;
    float water_friction;
    float surface_friction;
    float elasticity;
    uint8_t _pad_34[12];
} PointPhysics;  // size 0x40
typedef struct PreferencesNetworkGame {
    TagString name;
    ColorRGB primary_color;
    ColorRGB secondary_color;
    TagDependency pattern;  // bitmap
    uint16_t pattern_bitmap_index;
    uint8_t _pad_4a[2];
    TagDependency decal;  // bitmap
    uint16_t decal_bitmap_index;
    uint8_t _pad_5e[2];
    uint8_t _pad_60[800];
} PreferencesNetworkGame;  // size 0x380
typedef enum ProjectileResponse {
    projectileresponse_disappear = 0,
    projectileresponse_detonate = 1,
    projectileresponse_reflect = 2,
    projectileresponse_overpenetrate = 3,
    projectileresponse_attach = 4,
} ProjectileResponse;  // int16
typedef int16_t ProjectileResponse_t;
typedef enum ProjectileScaleEffectsBy {
    projectilescaleeffectsby_damage = 0,
    projectilescaleeffectsby_angle = 1,
} ProjectileScaleEffectsBy;  // int16
typedef int16_t ProjectileScaleEffectsBy_t;
typedef enum ProjectileDetonationTimerStarts {
    projectiledetonationtimerstarts_immediately = 0,
    projectiledetonationtimerstarts_after_first_bounce = 1,
    projectiledetonationtimerstarts_when_at_rest = 2,
} ProjectileDetonationTimerStarts;  // int16
typedef int16_t ProjectileDetonationTimerStarts_t;
typedef enum ProjectileFunctionIn {
    projectilefunctionin_none = 0,
    projectilefunctionin_range_remaining = 1,
    projectilefunctionin_time_remaining = 2,
    projectilefunctionin_tracer = 3,
} ProjectileFunctionIn;  // int16
typedef int16_t ProjectileFunctionIn_t;
typedef uint32_t ProjectileFlags;  // bitfield: oriented_along_velocity, ai_must_use_ballistic_aiming, detonation_max_time_if_attached, has_super_combining_explosion, combine_initial_velocity_with_parent_velocity, random_attached_detonation_time, minimum_unattached_detonation_time
typedef uint16_t ProjectileMaterialResponseFlags;  // bitfield: cannot_be_overpenetrated
typedef uint16_t ProjectileMaterialResponsePotentialFlags;  // bitfield: only_against_units, never_against_units
typedef struct ProjectileMaterialResponse {
    ProjectileMaterialResponseFlags flags;
    ProjectileResponse_t default_response;
    TagDependency default_effect;  // effect
    uint8_t _pad_14[16];
    ProjectileResponse_t potential_response;
    ProjectileMaterialResponsePotentialFlags potential_flags;
    float potential_skip_fraction;
    float potential_between[2];
    float potential_and[2];
    TagDependency potential_effect;  // effect
    uint8_t _pad_4c[16];
    ProjectileScaleEffectsBy_t scale_effects_by;
    uint8_t _pad_5e[2];
    float angular_noise;
    float velocity_noise;
    TagDependency detonation_effect;  // effect
    uint8_t _pad_78[24];
    float initial_friction;
    float maximum_distance;
    float parallel_friction;
    float perpendicular_friction;
} ProjectileMaterialResponse;  // size 0xa0
typedef struct Projectile {
    Object base;  // inherits
    ProjectileFlags projectile_flags;
    ProjectileDetonationTimerStarts_t detonation_timer_starts;
    ObjectNoise_t impact_noise;
    ProjectileFunctionIn_t projectile_a_in;
    ProjectileFunctionIn_t projectile_b_in;
    ProjectileFunctionIn_t projectile_c_in;
    ProjectileFunctionIn_t projectile_d_in;
    TagDependency super_detonation;  // effect
    float ai_perception_radius;
    float collision_radius;
    float arming_time;
    float danger_radius;
    TagDependency effect;  // effect
    float timer[2];
    float minimum_velocity;
    float maximum_range;
    float air_gravity_scale;
    float air_damage_range[2];
    float water_gravity_scale;
    float water_damage_range[2];
    float initial_velocity;
    float final_velocity;
    float guided_angular_velocity;
    ObjectNoise_t detonation_noise;
    uint8_t _pad_1f2[2];
    TagDependency detonation_started;  // effect
    TagDependency flyby_sound;  // sound
    TagDependency attached_detonation_damage;  // damage_effect
    TagDependency impact_damage;  // damage_effect
    uint8_t _pad_234[12];
    TagReflexive projectile_material_response;  // ProjectileMaterialResponse
} Projectile;  // size 0x24c
typedef enum ScenarioType {
    scenariotype_singleplayer = 0,
    scenariotype_multiplayer = 1,
    scenariotype_user_interface = 2,
} ScenarioType;  // int16
typedef int16_t ScenarioType_t;
typedef enum ScenarioSpawnType {
    scenariospawntype_none = 0,
    scenariospawntype_ctf = 1,
    scenariospawntype_slayer = 2,
    scenariospawntype_oddball = 3,
    scenariospawntype_king_of_the_hill = 4,
    scenariospawntype_race = 5,
    scenariospawntype_terminator = 6,
    scenariospawntype_stub = 7,
    scenariospawntype_ignored1 = 8,
    scenariospawntype_ignored2 = 9,
    scenariospawntype_ignored3 = 10,
    scenariospawntype_ignored4 = 11,
    scenariospawntype_all_games = 12,
    scenariospawntype_all_except_ctf = 13,
    scenariospawntype_all_except_race_and_ctf = 14,
} ScenarioSpawnType;  // int16
typedef int16_t ScenarioSpawnType_t;
typedef enum ScenarioNetgameFlagType {
    scenarionetgameflagtype_ctf_flag = 0,
    scenarionetgameflagtype_ctf_vehicle = 1,
    scenarionetgameflagtype_oddball_ball_spawn = 2,
    scenarionetgameflagtype_race_track = 3,
    scenarionetgameflagtype_race_vehicle = 4,
    scenarionetgameflagtype_vegas_bank = 5,
    scenarionetgameflagtype_teleport_from = 6,
    scenarionetgameflagtype_teleport_to = 7,
    scenarionetgameflagtype_hill_flag = 8,
} ScenarioNetgameFlagType;  // int16
typedef int16_t ScenarioNetgameFlagType_t;
typedef enum ScenarioReturnState {
    scenarioreturnstate_none = 0,
    scenarioreturnstate_sleeping = 1,
    scenarioreturnstate_alert = 2,
    scenarioreturnstate_moving_repeat_same_position = 3,
    scenarioreturnstate_moving_loop = 4,
    scenarioreturnstate_moving_loop_back_and_forth = 5,
    scenarioreturnstate_moving_loop_randomly = 6,
    scenarioreturnstate_moving_randomly = 7,
    scenarioreturnstate_guarding = 8,
    scenarioreturnstate_guarding_at_guard_position = 9,
    scenarioreturnstate_searching = 10,
    scenarioreturnstate_fleeing = 11,
} ScenarioReturnState;  // int16
typedef int16_t ScenarioReturnState_t;
typedef enum ScenarioUniqueLeaderType {
    scenariouniqueleadertype_normal = 0,
    scenariouniqueleadertype_none = 1,
    scenariouniqueleadertype_random = 2,
    scenariouniqueleadertype_sgt_johnson = 3,
    scenariouniqueleadertype_sgt_lehto = 4,
} ScenarioUniqueLeaderType;  // int16
typedef int16_t ScenarioUniqueLeaderType_t;
typedef enum ScenarioMajorUpgrade {
    scenariomajorupgrade_normal = 0,
    scenariomajorupgrade_few = 1,
    scenariomajorupgrade_many = 2,
    scenariomajorupgrade_none = 3,
    scenariomajorupgrade_all = 4,
} ScenarioMajorUpgrade;  // int16
typedef int16_t ScenarioMajorUpgrade_t;
typedef enum ScenarioChangeAttackingDefendingStateWhen {
    scenariochangeattackingdefendingstatewhen_never = 0,
    scenariochangeattackingdefendingstatewhen_75_strength = 1,
    scenariochangeattackingdefendingstatewhen_50_strength = 2,
    scenariochangeattackingdefendingstatewhen_25_strength = 3,
    scenariochangeattackingdefendingstatewhen_anybody_dead = 4,
    scenariochangeattackingdefendingstatewhen_25_dead = 5,
    scenariochangeattackingdefendingstatewhen_50_dead = 6,
    scenariochangeattackingdefendingstatewhen_75_dead = 7,
    scenariochangeattackingdefendingstatewhen_all_but_one_dead = 8,
    scenariochangeattackingdefendingstatewhen_all_dead = 9,
} ScenarioChangeAttackingDefendingStateWhen;  // int16
typedef int16_t ScenarioChangeAttackingDefendingStateWhen_t;
typedef enum ScenarioGroupIndex {
    scenariogroupindex_a = 0,
    scenariogroupindex_b = 1,
    scenariogroupindex_c = 2,
    scenariogroupindex_d = 3,
    scenariogroupindex_e = 4,
    scenariogroupindex_f = 5,
    scenariogroupindex_g = 6,
    scenariogroupindex_h = 7,
    scenariogroupindex_i = 8,
    scenariogroupindex_j = 9,
    scenariogroupindex_k = 10,
    scenariogroupindex_l = 11,
    scenariogroupindex_m = 12,
    scenariogroupindex_n = 13,
    scenariogroupindex_o = 14,
    scenariogroupindex_p = 15,
    scenariogroupindex_q = 16,
    scenariogroupindex_r = 17,
    scenariogroupindex_s = 18,
    scenariogroupindex_t = 19,
    scenariogroupindex_u = 20,
    scenariogroupindex_v = 21,
    scenariogroupindex_w = 22,
    scenariogroupindex_x = 23,
    scenariogroupindex_y = 24,
    scenariogroupindex_z = 25,
} ScenarioGroupIndex;  // int16
typedef int16_t ScenarioGroupIndex_t;
typedef enum ScenarioTeamIndex {
    scenarioteamindex_default_by_unit = 0,
    scenarioteamindex_player = 1,
    scenarioteamindex_human = 2,
    scenarioteamindex_covenant = 3,
    scenarioteamindex_flood = 4,
    scenarioteamindex_sentinel = 5,
    scenarioteamindex_unused6 = 6,
    scenarioteamindex_unused7 = 7,
    scenarioteamindex_unused8 = 8,
    scenarioteamindex_unused9 = 9,
} ScenarioTeamIndex;  // int16
typedef int16_t ScenarioTeamIndex_t;
typedef enum ScenarioSearchBehavior {
    scenariosearchbehavior_normal = 0,
    scenariosearchbehavior_never = 1,
    scenariosearchbehavior_tenacious = 2,
} ScenarioSearchBehavior;  // int16
typedef int16_t ScenarioSearchBehavior_t;
typedef enum ScenarioAtomType {
    scenarioatomtype_pause = 0,
    scenarioatomtype_go_to = 1,
    scenarioatomtype_go_to_and_face = 2,
    scenarioatomtype_move_in_direction = 3,
    scenarioatomtype_look = 4,
    scenarioatomtype_animation_mode = 5,
    scenarioatomtype_crouch = 6,
    scenarioatomtype_shoot = 7,
    scenarioatomtype_grenade = 8,
    scenarioatomtype_vehicle = 9,
    scenarioatomtype_running_jump = 10,
    scenarioatomtype_targeted_jump = 11,
    scenarioatomtype_script = 12,
    scenarioatomtype_animate = 13,
    scenarioatomtype_recording = 14,
    scenarioatomtype_action = 15,
    scenarioatomtype_vocalize = 16,
    scenarioatomtype_targeting = 17,
    scenarioatomtype_initiative = 18,
    scenarioatomtype_wait = 19,
    scenarioatomtype_loop = 20,
    scenarioatomtype_die = 21,
    scenarioatomtype_move_immediate = 22,
    scenarioatomtype_look_random = 23,
    scenarioatomtype_look_player = 24,
    scenarioatomtype_look_object = 25,
    scenarioatomtype_set_radius = 26,
    scenarioatomtype_teleport = 27,
} ScenarioAtomType;  // int16
typedef int16_t ScenarioAtomType_t;
typedef enum ScenarioSelectionType {
    scenarioselectiontype_friendly_actor = 0,
    scenarioselectiontype_disembodied = 1,
    scenarioselectiontype_in_player_s_vehicle = 2,
    scenarioselectiontype_not_in_a_vehicle = 3,
    scenarioselectiontype_prefer_sergeant = 4,
    scenarioselectiontype_any_actor = 5,
    scenarioselectiontype_radio_unit = 6,
    scenarioselectiontype_radio_sergeant = 7,
} ScenarioSelectionType;  // int16
typedef int16_t ScenarioSelectionType_t;
typedef enum ScenarioAddressee {
    scenarioaddressee_none = 0,
    scenarioaddressee_player = 1,
    scenarioaddressee_participant = 2,
} ScenarioAddressee;  // int16
typedef int16_t ScenarioAddressee_t;
typedef enum ScenarioScriptType {
    scenarioscripttype_startup = 0,
    scenarioscripttype_dormant = 1,
    scenarioscripttype_continuous = 2,
    scenarioscripttype_static = 3,
    scenarioscripttype_stub = 4,
} ScenarioScriptType;  // int16
typedef int16_t ScenarioScriptType_t;
typedef enum ScenarioScriptValueType {
    scenarioscriptvaluetype_unparsed = 0,
    scenarioscriptvaluetype_special_form = 1,
    scenarioscriptvaluetype_function_name = 2,
    scenarioscriptvaluetype_passthrough = 3,
    scenarioscriptvaluetype_void = 4,
    scenarioscriptvaluetype_boolean = 5,
    scenarioscriptvaluetype_real = 6,
    scenarioscriptvaluetype_short = 7,
    scenarioscriptvaluetype_long = 8,
    scenarioscriptvaluetype_string = 9,
    scenarioscriptvaluetype_script = 10,
    scenarioscriptvaluetype_trigger_volume = 11,
    scenarioscriptvaluetype_cutscene_flag = 12,
    scenarioscriptvaluetype_cutscene_camera_point = 13,
    scenarioscriptvaluetype_cutscene_title = 14,
    scenarioscriptvaluetype_cutscene_recording = 15,
    scenarioscriptvaluetype_device_group = 16,
    scenarioscriptvaluetype_ai = 17,
    scenarioscriptvaluetype_ai_command_list = 18,
    scenarioscriptvaluetype_starting_profile = 19,
    scenarioscriptvaluetype_conversation = 20,
    scenarioscriptvaluetype_navpoint = 21,
    scenarioscriptvaluetype_hud_message = 22,
    scenarioscriptvaluetype_object_list = 23,
    scenarioscriptvaluetype_sound = 24,
    scenarioscriptvaluetype_effect = 25,
    scenarioscriptvaluetype_damage = 26,
    scenarioscriptvaluetype_looping_sound = 27,
    scenarioscriptvaluetype_animation_graph = 28,
    scenarioscriptvaluetype_actor_variant = 29,
    scenarioscriptvaluetype_damage_effect = 30,
    scenarioscriptvaluetype_object_definition = 31,
    scenarioscriptvaluetype_game_difficulty = 32,
    scenarioscriptvaluetype_team = 33,
    scenarioscriptvaluetype_ai_default_state = 34,
    scenarioscriptvaluetype_actor_type = 35,
    scenarioscriptvaluetype_hud_corner = 36,
    scenarioscriptvaluetype_object = 37,
    scenarioscriptvaluetype_unit = 38,
    scenarioscriptvaluetype_vehicle = 39,
    scenarioscriptvaluetype_weapon = 40,
    scenarioscriptvaluetype_device = 41,
    scenarioscriptvaluetype_scenery = 42,
    scenarioscriptvaluetype_object_name = 43,
    scenarioscriptvaluetype_unit_name = 44,
    scenarioscriptvaluetype_vehicle_name = 45,
    scenarioscriptvaluetype_weapon_name = 46,
    scenarioscriptvaluetype_device_name = 47,
    scenarioscriptvaluetype_scenery_name = 48,
} ScenarioScriptValueType;  // int16
typedef int16_t ScenarioScriptValueType_t;
typedef enum ScenarioTextStyle {
    scenariotextstyle_plain = 0,
    scenariotextstyle_bold = 1,
    scenariotextstyle_italic = 2,
    scenariotextstyle_condense = 3,
    scenariotextstyle_underline = 4,
} ScenarioTextStyle;  // int16
typedef int16_t ScenarioTextStyle_t;
typedef enum ScenarioJustification {
    scenariojustification_left = 0,
    scenariojustification_right = 1,
    scenariojustification_center = 2,
} ScenarioJustification;  // int16
typedef int16_t ScenarioJustification_t;
typedef enum ScenarioTriggerVolumeType {
    scenariotriggervolumetype_fixed = 0,
    scenariotriggervolumetype_rotational = 1,
} ScenarioTriggerVolumeType;  // int16
typedef int16_t ScenarioTriggerVolumeType_t;
typedef uint32_t ScenarioTextFlags;  // bitfield: wrap_horizontally, wrap_vertically, center_vertically, bottom_justify
typedef uint32_t ScenarioFunctionFlags;  // bitfield: scripted, invert, additive, always_active
typedef uint16_t ScenarioSpawnNotPlaced;  // bitfield: automatically, on_easy, on_normal, on_hard, use_player_appearance
typedef uint32_t ScenarioUnitFlags;  // bitfield: dead
typedef uint16_t ScenarioVehicleMultiplayerSpawnFlags;  // bitfield: slayer_default, ctf_default, king_default, oddball_default, unused, unused1, unused2, unused3, slayer_allowed, ctf_allowed, king_allowed, oddball_allowed, unused4, unused5, unused6, unused7
typedef uint16_t ScenarioItemFlags;  // bitfield: initially_at_rest, obsolete, does_accelerate
typedef uint32_t ScenarioDeviceGroupFlags;  // bitfield: can_change_only_once
typedef uint32_t ScenarioDeviceFlags;  // bitfield: initially_open, initially_off, can_change_only_once, position_reversed, not_usable_from_any_side
typedef uint32_t ScenarioMachineFlags;  // bitfield: does_not_operate_automatically, one_sided, never_appears_locked, opened_by_melee_attack
typedef uint32_t ScenarioControlFlags;  // bitfield: usable_from_both_sides
typedef uint32_t ScenarioNetgameEquipmentFlags;  // bitfield: levitate
typedef uint32_t ScenarioStartingEquipmentFlags;  // bitfield: no_grenades, plasma_grenades_only, type2_grenades_only, type3_grenades_only
typedef uint8_t ScenarioActorStartingLocationFlags;  // bitfield: required
typedef uint32_t ScenarioSquadFlags;  // bitfield: unused, never_search, start_timer_immediately, no_timer_delay_forever, magic_sight_after_timer, automatic_migration
typedef uint32_t ScenarioSquadAttacking;  // bitfield: a, b, c, d, e, f, g, h, i, j, k, l, m, n, o, p, q, r, s, t, u, v, w, x, y, z
typedef uint32_t ScenarioPlatoonFlags;  // bitfield: flee_when_maneuvering, say_advancing_when_maneuver, start_in_defending_state
typedef uint32_t ScenarioEncounterFlags;  // bitfield: not_initially_created, respawn_enabled, initially_blind, initially_deaf, initially_braindead, _3d_firing_positions, manual_bsp_index_specified
typedef uint32_t ScenarioCommandListFlags;  // bitfield: allow_initiative, allow_targeting, disable_looking, disable_communication, disable_falling_damage, manual_bsp_index
typedef uint16_t ScenarioAIConversationParticipantFlags;  // bitfield: optional, has_alternate, is_alternate
typedef uint16_t ScenarioAIConversationLineFlags;  // bitfield: addressee_look_at_speaker, everyone_look_at_speaker, everyone_look_at_addressee, wait_after_until_told_to_advance, wait_until_speaker_nearby, wait_until_everyone_nearby
typedef uint16_t ScenarioAIConversationFlags;  // bitfield: stop_if_death, stop_if_damaged, stop_if_visible_enemy, stop_if_alerted_to_enemy, player_must_be_visible, stop_other_actions, keep_trying_to_play, player_must_be_looking
typedef uint16_t ScenarioFlags;  // bitfield: cortana_hack, use_demo_ui, color_correction_ntsc_to_srgb, do_not_apply_bungie_campaign_tag_patches
typedef uint16_t ScenarioScriptNodeFlags;  // bitfield: is_primitive, is_script_call, is_global, is_garbage_collectable, is_local_variable
typedef struct ScenarioSky {
    TagDependency sky;  // sky
} ScenarioSky;  // size 0x10
typedef struct ScenarioChildScenario {
    TagDependency child_scenario;  // scenario
    uint8_t _pad_10[16];
} ScenarioChildScenario;  // size 0x20
typedef struct ScenarioFunction {
    ScenarioFunctionFlags flags;
    TagString name;
    float period;
    uint16_t scale_period_by;
    WaveFunction_t function;
    uint16_t scale_function_by;
    WaveFunction_t wobble_function;
    float wobble_period;
    float wobble_magnitude;
    float square_wave_threshold;
    int16_t step_count;
    FunctionType_t map_to;
    int16_t sawtooth_count;
    uint8_t _pad_42[2];
    uint16_t scale_result_by;
    FunctionBoundsMode_t bounds_mode;
    float bounds[2];
    uint8_t _pad_50[4];
    uint8_t _pad_54[2];
    uint16_t turn_off_with;
    uint8_t _pad_58[16];
    uint8_t _pad_68[16];
} ScenarioFunction;  // size 0x78
typedef struct ScenarioEditorComment {
    Point3D position;
    uint8_t _pad_c[16];
    TagDataOffset comment;
} ScenarioEditorComment;  // size 0x30
typedef struct ScenarioScavengerHuntObjects {
    TagString name;
    uint16_t object_name_index;
    uint8_t _pad_22[2];
} ScenarioScavengerHuntObjects;  // size 0x24
typedef struct ScenarioObjectName {
    TagString name;
    ObjectType_t object_type;
    uint16_t object_index;
} ScenarioObjectName;  // size 0x24
typedef struct ScenarioScenery {
    uint16_t type;
    uint16_t name;
    ScenarioSpawnNotPlaced not_placed;
    int16_t desired_permutation;
    Point3D position;
    Euler3D rotation;
    uint16_t bsp_indices;
    uint8_t _pad_22[2];
    int8_t appearance_player_index;
    uint8_t _pad_25[3];
    uint8_t _pad_28[16];
    uint8_t _pad_38[8];
    uint8_t _pad_40[8];
} ScenarioScenery;  // size 0x48
typedef struct ScenarioSceneryPalette {
    TagDependency name;  // scenery
    uint8_t _pad_10[32];
} ScenarioSceneryPalette;  // size 0x30
typedef struct ScenarioBiped {
    uint16_t type;
    uint16_t name;
    ScenarioSpawnNotPlaced not_placed;
    int16_t desired_permutation;
    Point3D position;
    Euler3D rotation;
    uint8_t _pad_20[4];
    int8_t appearance_player_index;
    uint8_t _pad_25[3];
    uint8_t _pad_28[16];
    uint8_t _pad_38[8];
    uint8_t _pad_40[8];
    float body_vitality_modifier;
    ScenarioUnitFlags flags;
    uint8_t _pad_50[8];
    uint8_t _pad_58[32];
} ScenarioBiped;  // size 0x78
typedef struct ScenarioBipedPalette {
    TagDependency name;  // biped
    uint8_t _pad_10[32];
} ScenarioBipedPalette;  // size 0x30
typedef struct ScenarioVehicle {
    uint16_t type;
    uint16_t name;
    ScenarioSpawnNotPlaced not_placed;
    int16_t desired_permutation;
    Point3D position;
    Euler3D rotation;
    uint8_t _pad_20[4];
    int8_t appearance_player_index;
    uint8_t _pad_25[3];
    uint8_t _pad_28[16];
    uint8_t _pad_38[8];
    uint8_t _pad_40[8];
    float body_vitality;
    ScenarioUnitFlags flags;
    uint8_t _pad_50[8];
    int8_t multiplayer_team_index;
    uint8_t _pad_59[1];
    ScenarioVehicleMultiplayerSpawnFlags multiplayer_spawn_flags;
    uint8_t _pad_5c[28];
} ScenarioVehicle;  // size 0x78
typedef struct ScenarioVehiclePalette {
    TagDependency name;  // vehicle
    uint8_t _pad_10[32];
} ScenarioVehiclePalette;  // size 0x30
typedef struct ScenarioEquipment {
    uint16_t type;
    uint16_t name;
    ScenarioSpawnNotPlaced not_placed;
    int16_t desired_permutation;
    Point3D position;
    Euler3D rotation;
    uint8_t _pad_20[2];
    ScenarioItemFlags misc_flags;
    int8_t appearance_player_index;
    uint8_t _pad_25[3];
} ScenarioEquipment;  // size 0x28
typedef struct ScenarioEquipmentPalette {
    TagDependency name;  // equipment
    uint8_t _pad_10[32];
} ScenarioEquipmentPalette;  // size 0x30
typedef struct ScenarioWeapon {
    uint16_t type;
    uint16_t name;
    ScenarioSpawnNotPlaced not_placed;
    int16_t desired_permutation;
    Point3D position;
    Euler3D rotation;
    uint8_t _pad_20[4];
    int8_t appearance_player_index;
    uint8_t _pad_25[3];
    uint8_t _pad_28[16];
    uint8_t _pad_38[8];
    uint8_t _pad_40[8];
    int16_t rounds_reserved;
    int16_t rounds_loaded;
    ScenarioItemFlags flags;
    uint8_t _pad_4e[2];
    uint8_t _pad_50[12];
} ScenarioWeapon;  // size 0x5c
typedef struct ScenarioWeaponPalette {
    TagDependency name;  // weapon
    uint8_t _pad_10[32];
} ScenarioWeaponPalette;  // size 0x30
typedef struct ScenarioDeviceGroup {
    TagString name;
    float initial_value;
    ScenarioDeviceGroupFlags flags;
    uint8_t _pad_28[12];
} ScenarioDeviceGroup;  // size 0x34
typedef struct ScenarioMachine {
    uint16_t type;
    uint16_t name;
    ScenarioSpawnNotPlaced not_placed;
    int16_t desired_permutation;
    Point3D position;
    Euler3D rotation;
    uint8_t _pad_20[4];
    int8_t appearance_player_index;
    uint8_t _pad_25[3];
    uint16_t power_group;
    uint16_t position_group;
    ScenarioDeviceFlags device_flags;
    ScenarioMachineFlags machine_flags;
    uint8_t _pad_34[12];
} ScenarioMachine;  // size 0x40
typedef struct ScenarioMachinePalette {
    TagDependency name;  // device_machine
    uint8_t _pad_10[32];
} ScenarioMachinePalette;  // size 0x30
typedef struct ScenarioControl {
    uint16_t type;
    uint16_t name;
    ScenarioSpawnNotPlaced not_placed;
    int16_t desired_permutation;
    Point3D position;
    Euler3D rotation;
    uint8_t _pad_20[4];
    int8_t appearance_player_index;
    uint8_t _pad_25[3];
    uint16_t power_group;
    uint16_t position_group;
    ScenarioDeviceFlags device_flags;
    ScenarioControlFlags control_flags;
    int16_t custom_control_name;
    uint8_t _pad_36[2];
    uint8_t _pad_38[8];
} ScenarioControl;  // size 0x40
typedef struct ScenarioControlPalette {
    TagDependency name;  // device_control
    uint8_t _pad_10[32];
} ScenarioControlPalette;  // size 0x30
typedef struct ScenarioLightFixture {
    uint16_t type;
    uint16_t name;
    ScenarioSpawnNotPlaced not_placed;
    int16_t desired_permutation;
    Point3D position;
    Euler3D rotation;
    uint16_t bsp_indices;
    uint8_t _pad_22[2];
    int8_t appearance_player_index;
    uint8_t _pad_25[3];
    uint16_t power_group;
    uint16_t position_group;
    ScenarioDeviceFlags device_flags;
    ColorRGB color;
    float intensity;
    float falloff_angle;
    float cutoff_angle;
    uint8_t _pad_48[16];
} ScenarioLightFixture;  // size 0x58
typedef struct ScenarioLightFixturePalette {
    TagDependency name;  // device_light_fixture
    uint8_t _pad_10[32];
} ScenarioLightFixturePalette;  // size 0x30
typedef struct ScenarioSoundScenery {
    uint16_t type;
    uint16_t name;
    ScenarioSpawnNotPlaced not_placed;
    int16_t desired_permutation;
    Point3D position;
    Euler3D rotation;
    uint8_t _pad_20[4];
    int8_t appearance_player_index;
    uint8_t _pad_25[3];
} ScenarioSoundScenery;  // size 0x28
typedef struct ScenarioSoundSceneryPalette {
    TagDependency name;  // sound_scenery
    uint8_t _pad_10[32];
} ScenarioSoundSceneryPalette;  // size 0x30
typedef struct ScenarioPlayerStartingProfile {
    TagString name;
    float starting_health_modifier;
    float starting_shield_modifier;
    TagDependency primary_weapon;  // weapon
    int16_t primary_rounds_loaded;
    int16_t primary_rounds_reserved;
    TagDependency secondary_weapon;  // weapon
    int16_t secondary_rounds_loaded;
    int16_t secondary_rounds_reserved;
    int8_t starting_fragmentation_grenade_count;
    int8_t starting_plasma_grenade_count;
    int8_t starting_grenade_type2_count;
    int8_t starting_grenade_type3_count;
    uint8_t _pad_54[20];
} ScenarioPlayerStartingProfile;  // size 0x68
typedef struct ScenarioPlayerStartingLocation {
    Point3D position;
    float facing;
    uint16_t team_index;
    uint16_t bsp_index;
    ScenarioSpawnType_t type_0;
    ScenarioSpawnType_t type_1;
    ScenarioSpawnType_t type_2;
    ScenarioSpawnType_t type_3;
    uint8_t _pad_1c[24];
} ScenarioPlayerStartingLocation;  // size 0x34
typedef struct ScenarioTriggerVolume {
    ScenarioTriggerVolumeType_t type;
    uint8_t _pad_2[2];
    TagString name;
    float parameters[3];
    Vector3D rotation_vector_forward;
    Vector3D rotation_vector_up;
    Point3D starting_corner;
    Point3D ending_corner_offset;
} ScenarioTriggerVolume;  // size 0x60
typedef struct ScenarioRecordedAnimation {
    TagString name;
    int8_t version;
    int8_t raw_animation_data;
    int8_t unit_control_data_version;
    uint8_t _pad_23[1];
    int16_t length_of_animation;
    uint8_t _pad_26[2];
    uint8_t _pad_28[4];
    TagDataOffset recorded_animation_event_stream;
} ScenarioRecordedAnimation;  // size 0x40
typedef struct ScenarioNetgameFlags {
    Point3D position;
    float facing;
    ScenarioNetgameFlagType_t type;
    uint16_t usage_id;
    TagDependency weapon_group;  // item_collection
    uint8_t _pad_24[112];
} ScenarioNetgameFlags;  // size 0x94
typedef struct ScenarioNetgameEquipment {
    ScenarioNetgameEquipmentFlags flags;
    ScenarioSpawnType_t type_0;
    ScenarioSpawnType_t type_1;
    ScenarioSpawnType_t type_2;
    ScenarioSpawnType_t type_3;
    uint16_t team_index;
    int16_t spawn_time;
    uint32_t spawned_item;     // runtime: datum of the item currently spawned here, -1 when none
    uint8_t _pad_14[44];
    Point3D position;
    float facing;
    TagDependency item_collection;  // item_collection
    uint8_t _pad_60[48];
} ScenarioNetgameEquipment;  // size 0x90
typedef struct ScenarioStartingEquipment {
    ScenarioStartingEquipmentFlags flags;
    ScenarioSpawnType_t type_0;
    ScenarioSpawnType_t type_1;
    ScenarioSpawnType_t type_2;
    ScenarioSpawnType_t type_3;
    uint8_t _pad_c[48];
    TagDependency item_collection_1;  // item_collection
    TagDependency item_collection_2;  // item_collection
    TagDependency item_collection_3;  // item_collection
    TagDependency item_collection_4;  // item_collection
    TagDependency item_collection_5;  // item_collection
    TagDependency item_collection_6;  // item_collection
    uint8_t _pad_9c[48];
} ScenarioStartingEquipment;  // size 0xcc
typedef struct ScenarioBSPSwitchTriggerVolume {
    uint16_t trigger_volume;
    uint16_t source;
    uint16_t destination;
    uint16_t unknown;
} ScenarioBSPSwitchTriggerVolume;  // size 0x8
typedef struct ScenarioDecal {
    uint16_t decal_type;
    int8_t yaw;
    int8_t pitch;
    Point3D position;
} ScenarioDecal;  // size 0x10
typedef struct ScenarioDecalPalette {
    TagDependency reference;  // decal
} ScenarioDecalPalette;  // size 0x10
typedef struct ScenarioDetailObjectCollectionPalette {
    TagDependency reference;  // detail_object_collection
    uint8_t _pad_10[32];
} ScenarioDetailObjectCollectionPalette;  // size 0x30
typedef struct ScenarioActorPalette {
    TagDependency reference;  // actor_variant
} ScenarioActorPalette;  // size 0x10
typedef struct ScenarioMovePosition {
    Point3D position;
    float facing;
    float weight;
    float time[2];
    uint16_t animation;
    int8_t sequence_id;
    uint8_t _pad_1f[1];
    uint8_t _pad_20[8];
    uint16_t cluster_index;
    uint8_t _pad_2a[34];
    uint32_t surface_index;
} ScenarioMovePosition;  // size 0x50
typedef struct ScenarioActorStartingLocation {
    Point3D position;
    float facing;
    uint16_t cluster_index;
    int8_t sequence_id;
    ScenarioActorStartingLocationFlags flags;
    ScenarioReturnState_t return_state;
    ScenarioReturnState_t initial_state;
    uint16_t actor_type;
    uint16_t command_list;
} ScenarioActorStartingLocation;  // size 0x1c
typedef struct ScenarioSquad {
    TagString name;
    uint16_t actor_type;
    uint16_t platoon;
    ScenarioReturnState_t initial_state;
    ScenarioReturnState_t return_state;
    ScenarioSquadFlags flags;
    ScenarioUniqueLeaderType_t unique_leader_type;
    uint8_t _pad_2e[2];
    uint8_t _pad_30[28];
    uint8_t _pad_4c[2];
    uint16_t maneuver_to_squad;
    float squad_delay_time;
    ScenarioSquadAttacking attacking;
    ScenarioSquadAttacking attacking_search;
    ScenarioSquadAttacking attacking_guard;
    ScenarioSquadAttacking defending;
    ScenarioSquadAttacking defending_search;
    ScenarioSquadAttacking defending_guard;
    ScenarioSquadAttacking pursuing;
    uint8_t _pad_70[4];
    uint8_t _pad_74[8];
    int16_t normal_diff_count;
    int16_t insane_diff_count;
    ScenarioMajorUpgrade_t major_upgrade;
    uint8_t _pad_82[2];
    int16_t respawn_min_actors;
    int16_t respawn_max_actors;
    int16_t respawn_total;
    uint8_t _pad_8a[2];
    float respawn_delay[2];
    uint8_t _pad_94[48];
    TagReflexive move_positions;  // ScenarioMovePosition
    TagReflexive starting_locations;  // ScenarioActorStartingLocation
    uint8_t _pad_dc[12];
} ScenarioSquad;  // size 0xe8
typedef struct ScenarioPlatoon {
    TagString name;
    ScenarioPlatoonFlags flags;
    uint8_t _pad_24[12];
    ScenarioChangeAttackingDefendingStateWhen_t change_attacking_defending_state_when;
    uint16_t happens_to;
    uint8_t _pad_34[4];
    uint8_t _pad_38[4];
    ScenarioChangeAttackingDefendingStateWhen_t maneuver_when;
    uint16_t happens_to_1;
    uint8_t _pad_40[4];
    uint8_t _pad_44[4];
    uint8_t _pad_48[64];
    uint8_t _pad_88[36];
} ScenarioPlatoon;  // size 0xac
typedef struct ScenarioFiringPosition {
    Point3D position;
    ScenarioGroupIndex_t group_index;
    uint16_t cluster_index;
    uint8_t _pad_10[4];
    uint32_t surface_index;
} ScenarioFiringPosition;  // size 0x18
typedef struct ScenarioEncounter {
    TagString name;
    ScenarioEncounterFlags flags;
    ScenarioTeamIndex_t team_index;
    int16_t one;
    ScenarioSearchBehavior_t search_behavior;
    uint16_t manual_bsp_index;
    float respawn_delay[2];
    uint8_t _pad_34[74];
    uint16_t precomputed_bsp_index;
    TagReflexive squads;  // ScenarioSquad
    TagReflexive platoons;  // ScenarioPlatoon
    TagReflexive firing_positions;  // ScenarioFiringPosition
    TagReflexive player_starting_locations;  // ScenarioPlayerStartingLocation
} ScenarioEncounter;  // size 0xb0
typedef struct ScenarioCommand {
    ScenarioAtomType_t atom_type;
    int16_t atom_modifier;
    float parameter1;
    float parameter2;
    uint16_t point_1;
    uint16_t point_2;
    uint16_t animation;
    uint16_t script;
    uint16_t recording;
    uint16_t command;
    uint16_t object_name;
    uint8_t _pad_1a[6];
} ScenarioCommand;  // size 0x20
typedef struct ScenarioCommandPoint {
    Point3D position;
    uint32_t surface_index;
    uint8_t _pad_10[4];
} ScenarioCommandPoint;  // size 0x14
typedef struct ScenarioCommandList {
    TagString name;
    ScenarioCommandListFlags flags;
    uint8_t _pad_24[8];
    uint16_t manual_bsp_index;
    uint16_t precomputed_bsp_index;
    TagReflexive commands;  // ScenarioCommand
    TagReflexive points;  // ScenarioCommandPoint
    uint8_t _pad_48[24];
} ScenarioCommandList;  // size 0x60
typedef struct ScenarioAIAnimationReference {
    TagString animation_name;
    TagDependency animation_graph;  // model_animations
    uint8_t _pad_30[12];
} ScenarioAIAnimationReference;  // size 0x3c
typedef struct ScenarioAIScriptReference {
    TagString script_name;
    uint8_t _pad_20[8];
} ScenarioAIScriptReference;  // size 0x28
typedef struct ScenarioAIRecordingReference {
    TagString recording_name;
    uint8_t _pad_20[8];
} ScenarioAIRecordingReference;  // size 0x28
typedef struct ScenarioAIConversationParticipant {
    uint8_t _pad_0[2];
    ScenarioAIConversationParticipantFlags flags;
    ScenarioSelectionType_t selection_type;
    ActorType_t actor_type;
    uint16_t use_this_object;
    uint16_t set_new_name;
    uint8_t _pad_c[12];
    uint16_t variant_numbers[6];
    TagString encounter_name;
    uint32_t encounter_index;
    uint8_t _pad_48[12];
} ScenarioAIConversationParticipant;  // size 0x54
typedef struct ScenarioAIConversationLine {
    ScenarioAIConversationLineFlags flags;
    uint16_t participant;
    ScenarioAddressee_t addressee;
    uint16_t addressee_participant;
    uint8_t _pad_8[4];
    float line_delay_time;
    uint8_t _pad_10[12];
    TagDependency variant_1;  // sound
    TagDependency variant_2;  // sound
    TagDependency variant_3;  // sound
    TagDependency variant_4;  // sound
    TagDependency variant_5;  // sound
    TagDependency variant_6;  // sound
} ScenarioAIConversationLine;  // size 0x7c
typedef struct ScenarioAIConversation {
    TagString name;
    ScenarioAIConversationFlags flags;
    uint8_t _pad_22[2];
    float trigger_distance;
    float run_to_player_dist;
    uint8_t _pad_2c[36];
    TagReflexive participants;  // ScenarioAIConversationParticipant
    TagReflexive lines;  // ScenarioAIConversationLine
    uint8_t _pad_68[12];
} ScenarioAIConversation;  // size 0x74
typedef struct ScenarioScriptParameter {
    TagString name;
    ScenarioScriptValueType_t return_type;
    uint8_t _pad_22[2];
} ScenarioScriptParameter;  // size 0x24
typedef struct ScenarioScript {
    TagString name;
    ScenarioScriptType_t script_type;
    ScenarioScriptValueType_t return_type;
    uint32_t root_expression_index;
    uint8_t _pad_28[40];
    TagReflexive parameters;  // ScenarioScriptParameter
} ScenarioScript;  // size 0x5c
typedef struct ScenarioGlobal {
    TagString name;
    ScenarioScriptValueType_t type;
    uint8_t _pad_22[2];
    uint8_t _pad_24[4];
    int32_t initialization_expression_index;
    uint8_t _pad_2c[48];
} ScenarioGlobal;  // size 0x5c
typedef struct ScenarioReference {
    uint8_t _pad_0[24];
    TagDependency reference;  // *
} ScenarioReference;  // size 0x28
typedef struct ScenarioSourceFile {
    TagString name;
    TagDataOffset source;
} ScenarioSourceFile;  // size 0x34
typedef struct ScenarioCutsceneFlag {
    uint32_t unknown;
    TagString name;
    Point3D position;
    Euler2D facing;
    uint8_t _pad_38[36];
} ScenarioCutsceneFlag;  // size 0x5c
typedef struct ScenarioCutsceneCameraPoint {
    uint32_t unknown;
    TagString name;
    uint8_t _pad_24[4];
    Point3D position;
    Euler3D orientation;
    float field_of_view;
    uint8_t _pad_44[36];
} ScenarioCutsceneCameraPoint;  // size 0x68
typedef struct ScenarioCutsceneTitle {
    uint32_t unknown;
    TagString name;
    uint8_t _pad_24[4];
    Rectangle2D text_bounds;
    uint16_t string_index;
    ScenarioTextStyle_t text_style;
    ScenarioJustification_t justification;
    uint8_t _pad_36[2];
    ScenarioTextFlags text_flags;
    ColorARGBInt text_color;
    ColorARGBInt shadow_color;
    float fade_in_time;
    float up_time;
    float fade_out_time;
    uint8_t _pad_50[16];
} ScenarioCutsceneTitle;  // size 0x60
typedef struct ScenarioBSP {
    uint32_t bsp_start;
    uint32_t bsp_size;
    uint32_t bsp_address;
    uint8_t _pad_c[4];
    TagDependency structure_bsp;  // scenario_structure_bsp
} ScenarioBSP;  // size 0x20
typedef struct ScenarioScriptNode {
    uint16_t salt;
    uint16_t index_union;
    ScenarioScriptValueType_t type;
    ScenarioScriptNodeFlags flags;
    uint32_t next_node;
    uint32_t string_offset;
    uint32_t data;
} ScenarioScriptNode;  // size 0x14
typedef struct ScenarioScriptNodeTable {
    TagString name;
    uint16_t maximum_count;
    uint16_t element_size;
    uint8_t one;
    uint8_t _pad_25[3];
    uint32_t data;
    uint8_t _pad_2c[2];
    uint16_t size;
    uint16_t count;
    uint16_t next_id;
    uint32_t first_element_ptr;
} ScenarioScriptNodeTable;  // size 0x38
typedef struct Scenario {
    TagDependency don_t_use;  // scenario_structure_bsp
    TagDependency won_t_use;  // scenario_structure_bsp
    TagDependency can_t_use;  // sky
    TagReflexive skies;  // ScenarioSky
    ScenarioType_t type;
    ScenarioFlags flags;
    TagReflexive child_scenarios;  // ScenarioChildScenario
    float local_north;
    uint8_t _pad_50[20];
    uint8_t _pad_64[136];
    TagReflexive predicted_resources;  // PredictedResource
    TagReflexive functions;  // ScenarioFunction
    TagDataOffset editor_scenario_data;
    TagReflexive comments;  // ScenarioEditorComment
    TagReflexive scavenger_hunt_objects;  // ScenarioScavengerHuntObjects
    uint8_t _pad_130[212];
    TagReflexive object_names;  // ScenarioObjectName
    TagReflexive scenery;  // ScenarioScenery
    TagReflexive scenery_palette;  // ScenarioSceneryPalette
    TagReflexive bipeds;  // ScenarioBiped
    TagReflexive biped_palette;  // ScenarioBipedPalette
    TagReflexive vehicles;  // ScenarioVehicle
    TagReflexive vehicle_palette;  // ScenarioVehiclePalette
    TagReflexive equipment;  // ScenarioEquipment
    TagReflexive equipment_palette;  // ScenarioEquipmentPalette
    TagReflexive weapons;  // ScenarioWeapon
    TagReflexive weapon_palette;  // ScenarioWeaponPalette
    TagReflexive device_groups;  // ScenarioDeviceGroup
    TagReflexive machines;  // ScenarioMachine
    TagReflexive machine_palette;  // ScenarioMachinePalette
    TagReflexive controls;  // ScenarioControl
    TagReflexive control_palette;  // ScenarioControlPalette
    TagReflexive light_fixtures;  // ScenarioLightFixture
    TagReflexive light_fixture_palette;  // ScenarioLightFixturePalette
    TagReflexive sound_scenery;  // ScenarioSoundScenery
    TagReflexive sound_scenery_palette;  // ScenarioSoundSceneryPalette
    uint8_t _pad_2f4[84];
    TagReflexive player_starting_profile;  // ScenarioPlayerStartingProfile
    TagReflexive player_starting_locations;  // ScenarioPlayerStartingLocation
    TagReflexive trigger_volumes;  // ScenarioTriggerVolume
    TagReflexive recorded_animations;  // ScenarioRecordedAnimation
    TagReflexive netgame_flags;  // ScenarioNetgameFlags
    TagReflexive netgame_equipment;  // ScenarioNetgameEquipment
    TagReflexive starting_equipment;  // ScenarioStartingEquipment
    TagReflexive bsp_switch_trigger_volumes;  // ScenarioBSPSwitchTriggerVolume
    TagReflexive decals;  // ScenarioDecal
    TagReflexive decal_palette;  // ScenarioDecalPalette
    TagReflexive detail_object_collection_palette;  // ScenarioDetailObjectCollectionPalette
    uint8_t _pad_3cc[84];
    TagReflexive actor_palette;  // ScenarioActorPalette
    TagReflexive encounters;  // ScenarioEncounter
    TagReflexive command_lists;  // ScenarioCommandList
    TagReflexive ai_animation_references;  // ScenarioAIAnimationReference
    TagReflexive ai_script_references;  // ScenarioAIScriptReference
    TagReflexive ai_recording_references;  // ScenarioAIRecordingReference
    TagReflexive ai_conversations;  // ScenarioAIConversation
    TagDataOffset script_syntax_data;
    TagDataOffset script_string_data;
    TagReflexive scripts;  // ScenarioScript
    TagReflexive globals;  // ScenarioGlobal
    TagReflexive references;  // ScenarioReference
    TagReflexive source_files;  // ScenarioSourceFile
    uint8_t _pad_4cc[24];
    TagReflexive cutscene_flags;  // ScenarioCutsceneFlag
    TagReflexive cutscene_camera_points;  // ScenarioCutsceneCameraPoint
    TagReflexive cutscene_titles;  // ScenarioCutsceneTitle
    uint8_t _pad_508[108];
    TagDependency custom_object_names;  // unicode_string_list
    TagDependency ingame_help_text;  // unicode_string_list
    TagDependency hud_messages;  // hud_message_text
    TagReflexive structure_bsps;  // ScenarioBSP
} Scenario;  // size 0x5b0
typedef uint16_t ScenarioStructureBSPMaterialFlags;  // bitfield: coplanar, fog_plane
typedef uint32_t ScenarioStructureBSPClusterPortalFlags;  // bitfield: ai_can_simply_not_hear_through_all_this_amazing_stuff_darn_it
typedef struct ScenarioStructureBSPCollisionMaterial {
    TagDependency shader;  // shader
    uint8_t _pad_10[2];
    MaterialType_t material;
} ScenarioStructureBSPCollisionMaterial;  // size 0x14
typedef struct ScenarioStructureBSPNode {
    uint8_t bounds_x[2];
    uint8_t bounds_y[2];
    uint8_t bounds_z[2];
} ScenarioStructureBSPNode;  // size 0x6
typedef struct ScenarioStructureBSPLeaf {
    uint8_t bounds_x[2];
    uint8_t bounds_y[2];
    uint8_t bounds_z[2];
    uint8_t _pad_6[2];
    uint16_t cluster;
    int16_t surface_reference_count;
    int32_t surface_references;
} ScenarioStructureBSPLeaf;  // size 0x10
typedef struct ScenarioStructureBSPSurfaceReference {
    int32_t surface;
    int32_t node;
} ScenarioStructureBSPSurfaceReference;  // size 0x8
typedef struct ScenarioStructureBSPSurface {
    uint16_t vertex0_index;
    uint16_t vertex1_index;
    uint16_t vertex2_index;
} ScenarioStructureBSPSurface;  // size 0x6
typedef struct ScenarioStructureBSPMaterialUncompressedRenderedVertex {
    Point3D position;
    Vector3D normal;
    Vector3D binormal;
    Vector3D tangent;
    Point2D texture_coords;
} ScenarioStructureBSPMaterialUncompressedRenderedVertex;  // size 0x38
typedef struct ScenarioStructureBSPMaterialCompressedRenderedVertex {
    Point3D position;
    uint32_t normal;
    uint32_t binormal;
    uint32_t tangent;
    Point2D texture_coords;
} ScenarioStructureBSPMaterialCompressedRenderedVertex;  // size 0x20
typedef struct ScenarioStructureBSPMaterialUncompressedLightmapVertex {
    Vector3D normal;
    Point2D texture_coords;
} ScenarioStructureBSPMaterialUncompressedLightmapVertex;  // size 0x14
typedef struct ScenarioStructureBSPMaterialCompressedLightmapVertex {
    uint32_t normal;
    int16_t texture_coordinate_x;
    int16_t texture_coordinate_y;
} ScenarioStructureBSPMaterialCompressedLightmapVertex;  // size 0x8
typedef struct ScenarioStructureBSPMaterial {
    TagDependency shader;  // shader
    uint16_t shader_permutation;
    ScenarioStructureBSPMaterialFlags flags;
    int32_t surfaces;
    int32_t surface_count;
    Point3D centroid;
    ColorRGB ambient_color;
    int16_t distant_light_count;
    uint8_t _pad_36[2];
    ColorRGB distant_light_0_color;
    Vector3D distant_light_0_direction;
    ColorRGB distant_light_1_color;
    Vector3D distant_light_1_direction;
    uint8_t _pad_68[12];
    ColorARGB reflection_tint;
    Vector3D shadow_vector;
    ColorRGB shadow_color;
    Plane3D plane;
    uint16_t breakable_surface;
    uint8_t _pad_ae[2];
    VertexType_t rendered_vertices_type;
    uint8_t _pad_b2[2];
    uint32_t rendered_vertices_count;
    uint32_t rendered_vertices_offset;
    uint8_t _pad_bc[4];
    uint32_t rendered_vertices_index_pointer;
    VertexType_t lightmap_vertices_type;
    uint8_t _pad_c6[2];
    uint32_t lightmap_vertices_count;
    uint32_t lightmap_vertices_offset;
    uint8_t _pad_d0[4];
    uint32_t lightmap_vertices_index_pointer;
    TagDataOffset uncompressed_vertices;
    TagDataOffset compressed_vertices;
} ScenarioStructureBSPMaterial;  // size 0x100
typedef struct ScenarioStructureBSPLightmap {
    uint16_t bitmap;
    uint8_t _pad_2[2];
    uint8_t _pad_4[16];
    TagReflexive materials;  // ScenarioStructureBSPMaterial
} ScenarioStructureBSPLightmap;  // size 0x20
typedef struct ScenarioStructureBSPLensFlare {
    TagDependency lens;  // lens_flare
} ScenarioStructureBSPLensFlare;  // size 0x10
typedef struct ScenarioStructureBSPLensFlareMarker {
    Point3D position;
    int8_t direction_i_component;
    int8_t direction_j_component;
    int8_t direction_k_component;
    int8_t lens_flare_index;
} ScenarioStructureBSPLensFlareMarker;  // size 0x10
typedef struct ScenarioStructureBSPSubclusterSurfaceIndex {
    int32_t index;
} ScenarioStructureBSPSubclusterSurfaceIndex;  // size 0x4
typedef struct ScenarioStructureBSPSubcluster {
    float world_bounds_x[2];
    float world_bounds_y[2];
    float world_bounds_z[2];
    TagReflexive surface_indices;  // ScenarioStructureBSPSubclusterSurfaceIndex
} ScenarioStructureBSPSubcluster;  // size 0x24
typedef struct ScenarioStructureBSPClusterSurfaceIndex {
    int32_t index;
} ScenarioStructureBSPClusterSurfaceIndex;  // size 0x4
typedef struct ScenarioStructureBSPMirrorVertex {
    Point3D point;
} ScenarioStructureBSPMirrorVertex;  // size 0xc
typedef struct ScenarioStructureBSPMirror {
    Plane3D plane;
    uint8_t _pad_10[20];
    TagDependency shader;  // shader
    TagReflexive vertices;  // ScenarioStructureBSPMirrorVertex
} ScenarioStructureBSPMirror;  // size 0x40
typedef struct ScenarioStructureBSPClusterPortalIndex {
    uint16_t portal;
} ScenarioStructureBSPClusterPortalIndex;  // size 0x2
typedef struct ScenarioStructureBSPCluster {
    uint16_t sky;
    uint16_t fog;
    uint16_t background_sound;
    uint16_t sound_environment;
    uint16_t weather;
    uint16_t transition_structure_bsp;
    uint16_t first_decal_index;
    uint16_t decal_count;
    uint8_t _pad_10[24];
    TagReflexive predicted_resources;  // PredictedResource
    TagReflexive subclusters;  // ScenarioStructureBSPSubcluster
    uint16_t first_lens_flare_marker_index;
    int16_t lens_flare_marker_count;
    TagReflexive surface_indices;  // ScenarioStructureBSPClusterSurfaceIndex
    TagReflexive mirrors;  // ScenarioStructureBSPMirror
    TagReflexive portals;  // ScenarioStructureBSPClusterPortalIndex
} ScenarioStructureBSPCluster;  // size 0x68
typedef struct ScenarioStructureBSPClusterPortalVertex {
    Point3D point;
} ScenarioStructureBSPClusterPortalVertex;  // size 0xc
typedef struct ScenarioStructureBSPClusterPortal {
    uint16_t front_cluster;
    uint16_t back_cluster;
    int32_t plane_index;
    Point3D centroid;
    float bounding_radius;
    ScenarioStructureBSPClusterPortalFlags flags;
    uint8_t _pad_1c[24];
    TagReflexive vertices;  // ScenarioStructureBSPClusterPortalVertex
} ScenarioStructureBSPClusterPortal;  // size 0x40
typedef struct ScenarioStructureBSPBreakableSurface {
    Point3D centroid;
    float radius;
    int32_t collision_surface_index;
    uint8_t _pad_14[28];
} ScenarioStructureBSPBreakableSurface;  // size 0x30
typedef struct ScenarioStructureBSPFogPlaneVertex {
    Point3D point;
} ScenarioStructureBSPFogPlaneVertex;  // size 0xc
typedef struct ScenarioStructureBSPFogPlane {
    uint16_t front_region;
    MaterialType_t material_type;
    Plane3D plane;
    TagReflexive vertices;  // ScenarioStructureBSPFogPlaneVertex
} ScenarioStructureBSPFogPlane;  // size 0x20
typedef struct ScenarioStructureBSPFogRegion {
    uint8_t _pad_0[36];
    uint16_t fog;
    uint16_t weather_palette;
} ScenarioStructureBSPFogRegion;  // size 0x28
typedef struct ScenarioStructureBSPFogPalette {
    TagString name;
    TagDependency fog;  // fog
    uint8_t _pad_30[4];
    TagString fog_scale_function;
    uint8_t _pad_54[52];
} ScenarioStructureBSPFogPalette;  // size 0x88
typedef struct ScenarioStructureBSPWeatherPalette {
    TagString name;
    TagDependency particle_system;  // weather_particle_system
    uint8_t _pad_30[4];
    TagString particle_system_scale_function;
    uint8_t _pad_54[44];
    TagDependency wind;  // wind
    Vector3D wind_direction;
    float wind_magnitude;
    uint8_t _pad_a0[4];
    TagString wind_scale_function;
    uint8_t _pad_c4[44];
} ScenarioStructureBSPWeatherPalette;  // size 0xf0
typedef struct ScenarioStructureBSPWeatherPolyhedronPlane {
    Plane3D plane;
} ScenarioStructureBSPWeatherPolyhedronPlane;  // size 0x10
typedef struct ScenarioStructureBSPWeatherPolyhedron {
    Point3D bounding_sphere_center;
    float bounding_sphere_radius;
    uint8_t _pad_10[4];
    TagReflexive planes;  // ScenarioStructureBSPWeatherPolyhedronPlane
} ScenarioStructureBSPWeatherPolyhedron;  // size 0x20
typedef struct ScenarioStructureBSPPathfindingSurface {
    int8_t data;
} ScenarioStructureBSPPathfindingSurface;  // size 0x1
typedef struct ScenarioStructureBSPPathfindingEdge {
    int8_t midpoint;
} ScenarioStructureBSPPathfindingEdge;  // size 0x1
typedef struct ScenarioStructureBSPBackgroundSoundPalette {
    TagString name;
    TagDependency background_sound;  // sound_looping
    uint8_t _pad_30[4];
    TagString scale_function;
    uint8_t _pad_54[32];
} ScenarioStructureBSPBackgroundSoundPalette;  // size 0x74
typedef struct ScenarioStructureBSPSoundEnvironmentPalette {
    TagString name;
    TagDependency sound_environment;  // sound_environment
    uint8_t _pad_30[32];
} ScenarioStructureBSPSoundEnvironmentPalette;  // size 0x50
typedef struct ScenarioStructureBSPMarker {
    TagString name;
    Quaternion rotation;
    Point3D position;
} ScenarioStructureBSPMarker;  // size 0x3c
typedef struct ScenarioStructureBSPGlobalDetailObjectCell {
    int16_t cell_x;
    int16_t cell_y;
    int16_t cell_z;
    int16_t offset_z;
    uint32_t valid_layers_flags;
    uint32_t start_index;
    uint32_t count_index;
    uint8_t _pad_14[12];
} ScenarioStructureBSPGlobalDetailObjectCell;  // size 0x20
typedef struct ScenarioStructureBSPGlobalDetailObject {
    int8_t position_x;
    int8_t position_y;
    int8_t position_z;
    int8_t data;
    int16_t color;
} ScenarioStructureBSPGlobalDetailObject;  // size 0x6
typedef struct ScenarioStructureBSPGlobalDetailObjectCount {
    uint16_t count;
} ScenarioStructureBSPGlobalDetailObjectCount;  // size 0x2
typedef struct ScenarioStructureBSPGlobalZReferenceVector {
    float z_reference_i;
    float z_reference_j;
    float z_reference_k;
    float z_reference_l;
} ScenarioStructureBSPGlobalZReferenceVector;  // size 0x10
typedef struct ScenarioStructureBSPDetailObjectData {
    TagReflexive cells;  // ScenarioStructureBSPGlobalDetailObjectCell
    TagReflexive instances;  // ScenarioStructureBSPGlobalDetailObject
    TagReflexive counts;  // ScenarioStructureBSPGlobalDetailObjectCount
    TagReflexive z_reference_vectors;  // ScenarioStructureBSPGlobalZReferenceVector
    uint8_t bullshit;
    uint8_t _pad_31[3];
    uint8_t _pad_34[12];
} ScenarioStructureBSPDetailObjectData;  // size 0x40
typedef struct ScenarioStructureBSPRuntimeDecal {
    Point3D position;
    uint16_t decal_type;
    int8_t yaw;
    int8_t pitch;
} ScenarioStructureBSPRuntimeDecal;  // size 0x10
typedef struct ScenarioStructureBSPMapLeafFaceVertex {
    Point2D vertex;
} ScenarioStructureBSPMapLeafFaceVertex;  // size 0x8
typedef struct ScenarioStructureBSPMapLeafFace {
    int32_t node_index;
    TagReflexive vertices;  // ScenarioStructureBSPMapLeafFaceVertex
} ScenarioStructureBSPMapLeafFace;  // size 0x10
typedef struct ScenarioStructureBSPMapLeafPortalIndex {
    int32_t portal_index;
} ScenarioStructureBSPMapLeafPortalIndex;  // size 0x4
typedef struct ScenarioStructureBSPGlobalMapLeaf {
    TagReflexive faces;  // ScenarioStructureBSPMapLeafFace
    TagReflexive portal_indices;  // ScenarioStructureBSPMapLeafPortalIndex
} ScenarioStructureBSPGlobalMapLeaf;  // size 0x18
typedef struct ScenarioStructureBSPLeafPortalVertex {
    Point3D point;
} ScenarioStructureBSPLeafPortalVertex;  // size 0xc
typedef struct ScenarioStructureBSPGlobalLeafPortal {
    int32_t plane_index;
    int32_t back_leaf_index;
    int32_t front_leaf_index;
    TagReflexive vertices;  // ScenarioStructureBSPLeafPortalVertex
} ScenarioStructureBSPGlobalLeafPortal;  // size 0x18
typedef struct ScenarioStructureBSP {
    TagDependency lightmaps_bitmap;  // bitmap
    float vehicle_floor;
    float vehicle_ceiling;
    uint8_t _pad_18[20];
    ColorRGB default_ambient_color;
    uint8_t _pad_38[4];
    ColorRGB default_distant_light_0_color;
    Vector3D default_distant_light_0_direction;
    ColorRGB default_distant_light_1_color;
    Vector3D default_distant_light_1_direction;
    uint8_t _pad_6c[12];
    ColorARGB default_reflection_tint;
    Vector3D default_shadow_vector;
    ColorRGB default_shadow_color;
    uint8_t _pad_a0[4];
    TagReflexive collision_materials;  // ScenarioStructureBSPCollisionMaterial
    TagReflexive collision_bsp;  // ModelCollisionGeometryBSP
    TagReflexive nodes;  // ScenarioStructureBSPNode
    float world_bounds_x[2];
    float world_bounds_y[2];
    float world_bounds_z[2];
    TagReflexive leaves;  // ScenarioStructureBSPLeaf
    TagReflexive leaf_surfaces;  // ScenarioStructureBSPSurfaceReference
    TagReflexive surfaces;  // ScenarioStructureBSPSurface
    TagReflexive lightmaps;  // ScenarioStructureBSPLightmap
    uint8_t _pad_110[12];
    TagReflexive lens_flares;  // ScenarioStructureBSPLensFlare
    TagReflexive lens_flare_markers;  // ScenarioStructureBSPLensFlareMarker
    TagReflexive clusters;  // ScenarioStructureBSPCluster
    TagDataOffset cluster_data;
    TagReflexive cluster_portals;  // ScenarioStructureBSPClusterPortal
    uint8_t _pad_160[12];
    TagReflexive breakable_surfaces;  // ScenarioStructureBSPBreakableSurface
    TagReflexive fog_planes;  // ScenarioStructureBSPFogPlane
    TagReflexive fog_regions;  // ScenarioStructureBSPFogRegion
    TagReflexive fog_palette;  // ScenarioStructureBSPFogPalette
    uint8_t _pad_19c[24];
    TagReflexive weather_palette;  // ScenarioStructureBSPWeatherPalette
    TagReflexive weather_polyhedra;  // ScenarioStructureBSPWeatherPolyhedron
    uint8_t _pad_1cc[24];
    TagReflexive pathfinding_surfaces;  // ScenarioStructureBSPPathfindingSurface
    TagReflexive pathfinding_edges;  // ScenarioStructureBSPPathfindingEdge
    TagReflexive background_sound_palette;  // ScenarioStructureBSPBackgroundSoundPalette
    TagReflexive sound_environment_palette;  // ScenarioStructureBSPSoundEnvironmentPalette
    TagDataOffset sound_pas_data;
    uint8_t _pad_228[24];
    TagReflexive markers;  // ScenarioStructureBSPMarker
    TagReflexive detail_objects;  // ScenarioStructureBSPDetailObjectData
    TagReflexive runtime_decals;  // ScenarioStructureBSPRuntimeDecal
    uint8_t _pad_264[8];
    uint8_t _pad_26c[4];
    TagReflexive leaf_map_leaves;  // ScenarioStructureBSPGlobalMapLeaf
    TagReflexive leaf_map_portals;  // ScenarioStructureBSPGlobalLeafPortal
} ScenarioStructureBSP;  // size 0x288
typedef struct ScenarioStructureBSPCompiledHeader {
    uint32_t pointer;
    uint32_t lightmap_material_count;
    uint32_t rendered_vertices;
    uint32_t lightmap_material_count_again;
    uint32_t lightmap_vertices;
    uint32_t signature;
} ScenarioStructureBSPCompiledHeader;  // size 0x18
typedef struct ScenarioStructureBSPCompiledHeaderCEA {
    uint32_t pointer;
    uint32_t lightmap_vertex_size;
    uint32_t lightmap_vertices;
    uint8_t _pad_c[8];
    uint32_t signature;
} ScenarioStructureBSPCompiledHeaderCEA;  // size 0x18
typedef struct Scenery {
    BasicObject base;  // inherits
} Scenery;  // size 0x1fc
typedef enum ShaderDetailLevel {
    shaderdetaillevel_high = 0,
    shaderdetaillevel_medium = 1,
    shaderdetaillevel_low = 2,
    shaderdetaillevel_turd = 3,
} ShaderDetailLevel;  // int16
typedef int16_t ShaderDetailLevel_t;
typedef enum ShaderColorFunctionType {
    shadercolorfunctiontype_current = 0,
    shadercolorfunctiontype_next_map = 1,
    shadercolorfunctiontype_multiply = 2,
    shadercolorfunctiontype_double_multiply = 3,
    shadercolorfunctiontype_add = 4,
    shadercolorfunctiontype_add_signed_current = 5,
    shadercolorfunctiontype_add_signed_next_map = 6,
    shadercolorfunctiontype_subtract_current = 7,
    shadercolorfunctiontype_subtract_next_map = 8,
    shadercolorfunctiontype_blend_current_alpha = 9,
    shadercolorfunctiontype_blend_current_alpha_inverse = 10,
    shadercolorfunctiontype_blend_next_map_alpha = 11,
    shadercolorfunctiontype_blend_next_map_alpha_inverse = 12,
} ShaderColorFunctionType;  // int16
typedef int16_t ShaderColorFunctionType_t;
typedef enum ShaderFirstMapType {
    shaderfirstmaptype_2d_map = 0,
    shaderfirstmaptype_first_map_is_reflection_cube_map = 1,
    shaderfirstmaptype_first_map_is_object_centered_cube_map = 2,
    shaderfirstmaptype_first_map_is_viewer_centered_cube_map = 3,
} ShaderFirstMapType;  // int16
typedef int16_t ShaderFirstMapType_t;
typedef enum ShaderDetailFunction {
    shaderdetailfunction_double_biased_multiply = 0,
    shaderdetailfunction_multiply = 1,
    shaderdetailfunction_double_biased_add = 2,
} ShaderDetailFunction;  // int16
typedef int16_t ShaderDetailFunction_t;
typedef uint16_t ShaderFlags;  // bitfield: simple_parameterization, ignore_normals, transparent_lit
typedef uint16_t ShaderPhysicsFlags;  // bitfield: unused
typedef struct ShaderTransparentExtraLayer {
    TagDependency shader;  // shader
} ShaderTransparentExtraLayer;  // size 0x10
typedef struct Shader {
    ShaderFlags shader_flags;
    ShaderDetailLevel_t detail_level;
    float power;
    ColorRGB color_of_emitted_light;
    ColorRGB tint_color;
    ShaderPhysicsFlags physics_flags;
    MaterialType_t material_type;
    uint16_t shader_type;
    uint8_t _pad_26[2];
} Shader;  // size 0x28
typedef enum ShaderEnvironmentType {
    shaderenvironmenttype_normal = 0,
    shaderenvironmenttype_blended = 1,
    shaderenvironmenttype_blended_base_specular = 2,
} ShaderEnvironmentType;  // int16
typedef int16_t ShaderEnvironmentType_t;
typedef enum ShaderEnvironmentReflectionType {
    shaderenvironmentreflectiontype_bumped_cube_map = 0,
    shaderenvironmentreflectiontype_flat_cube_map = 1,
    shaderenvironmentreflectiontype_bumped_radiosity = 2,
} ShaderEnvironmentReflectionType;  // int16
typedef int16_t ShaderEnvironmentReflectionType_t;
typedef uint16_t ShaderEnvironmentFlags;  // bitfield: alpha_tested, bump_map_is_specular_mask, true_atmospheric_fog, use_alternate_bump_attenuation, use_alternate_normal_type_blending
typedef uint16_t ShaderEnvironmentDiffuseFlags;  // bitfield: rescale_detail_maps, rescale_bump_map
typedef uint16_t ShaderEnvironmentSpecularFlags;  // bitfield: overbright, extra_shiny, lightmap_is_specular
typedef uint16_t ShaderEnvironmentReflectionFlags;  // bitfield: dynamic_mirror
typedef struct ShaderEnvironment {
    Shader base;  // inherits
    ShaderEnvironmentFlags shader_environment_flags;
    ShaderEnvironmentType_t shader_environment_type;
    float lens_flare_spacing;
    TagDependency lens_flare;  // lens_flare
    uint8_t _pad_40[44];
    ShaderEnvironmentDiffuseFlags diffuse_flags;
    uint8_t _pad_6e[2];
    uint8_t _pad_70[24];
    TagDependency base_map;  // bitmap
    uint8_t _pad_98[24];
    ShaderDetailFunction_t detail_map_function;
    uint8_t _pad_b2[2];
    float primary_detail_map_scale;
    TagDependency primary_detail_map;  // bitmap
    float secondary_detail_map_scale;
    TagDependency secondary_detail_map;  // bitmap
    uint8_t _pad_dc[24];
    ShaderDetailFunction_t micro_detail_map_function;
    uint8_t _pad_f6[2];
    float micro_detail_map_scale;
    TagDependency micro_detail_map;  // bitmap
    ColorRGB material_color;
    uint8_t _pad_118[12];
    float bump_map_scale;
    TagDependency bump_map;  // bitmap
    Point2D bump_map_scale_xy;
    uint8_t _pad_140[16];
    WaveFunction_t u_animation_function;
    uint8_t _pad_152[2];
    float u_animation_period;
    float u_animation_scale;
    WaveFunction_t v_animation_function;
    uint8_t _pad_15e[2];
    float v_animation_period;
    float v_animation_scale;
    uint8_t _pad_168[24];
    IsUnfilteredFlag self_illumination_flags;
    uint8_t _pad_182[2];
    uint8_t _pad_184[24];
    ColorRGB primary_on_color;
    ColorRGB primary_off_color;
    WaveFunction_t primary_animation_function;
    uint8_t _pad_1b6[2];
    float primary_animation_period;
    float primary_animation_phase;
    uint8_t _pad_1c0[24];
    ColorRGB secondary_on_color;
    ColorRGB secondary_off_color;
    WaveFunction_t secondary_animation_function;
    uint8_t _pad_1f2[2];
    float secondary_animation_period;
    float secondary_animation_phase;
    uint8_t _pad_1fc[24];
    ColorRGB plasma_on_color;
    ColorRGB plasma_off_color;
    WaveFunction_t plasma_animation_function;
    uint8_t _pad_22e[2];
    float plasma_animation_period;
    float plasma_animation_phase;
    uint8_t _pad_238[24];
    float map_scale;
    TagDependency map;  // bitmap
    uint8_t _pad_264[24];
    ShaderEnvironmentSpecularFlags specular_flags;
    uint8_t _pad_27e[2];
    uint8_t _pad_280[16];
    float brightness;
    uint8_t _pad_294[20];
    ColorRGB perpendicular_color;
    ColorRGB parallel_color;
    uint8_t _pad_2c0[16];
    ShaderEnvironmentReflectionFlags reflection_flags;
    ShaderEnvironmentReflectionType_t reflection_type;
    float lightmap_brightness_scale;
    uint8_t _pad_2d8[28];
    float perpendicular_brightness;
    float parallel_brightness;
    uint8_t _pad_2fc[16];
    float runtime_mirror_value_0;  // 0x30c runtime value, not authored: the mirror query 0x553560
                                   //       copies it into structure_bsp_mirror_result +0x10 when the
                                   //       mirror shader is shadertype_environment (0x55371b)
    float runtime_mirror_value_1;  // 0x310 likewise into structure_bsp_mirror_result +0x14 (0x553724)
    uint8_t _pad_314[16];
    TagDependency reflection_cube_map;  // bitmap
    uint8_t _pad_334[16];
} ShaderEnvironment;  // size 0x344
typedef enum ShaderModelDetailMask {
    shadermodeldetailmask_none = 0,
    shadermodeldetailmask_reflection_mask_inverse = 1,
    shadermodeldetailmask_reflection_mask = 2,
    shadermodeldetailmask_self_illumination_mask_inverse = 3,
    shadermodeldetailmask_self_illumination_mask = 4,
    shadermodeldetailmask_change_color_mask_inverse = 5,
    shadermodeldetailmask_change_color_mask = 6,
    shadermodeldetailmask_auxiliary_mask_inverse = 7,
    shadermodeldetailmask_auxiliary_mask = 8,
} ShaderModelDetailMask;  // int16
typedef int16_t ShaderModelDetailMask_t;
typedef uint16_t ShaderModelFlags;  // bitfield: detail_after_reflection, two_sided, not_alpha_tested, alpha_blended_decal, true_atmospheric_fog, disable_two_sided_culling, use_xbox_multipurpose_channel_order
typedef uint16_t ShaderModelMoreFlags;  // bitfield: no_random_phase
typedef struct ShaderModel {
    Shader base;  // inherits
    ShaderModelFlags shader_model_flags;
    uint8_t _pad_2a[2];
    uint8_t _pad_2c[12];
    float translucency;
    uint8_t _pad_3c[16];
    FunctionNameNullable_t change_color_source;
    uint8_t _pad_4e[2];
    uint8_t _pad_50[28];
    ShaderModelMoreFlags shader_model_more_flags;
    uint8_t _pad_6e[2];
    FunctionNameNullable_t color_source;
    WaveFunction_t animation_function;
    float animation_period;
    ColorRGB animation_color_lower_bound;
    ColorRGB animation_color_upper_bound;
    uint8_t _pad_90[12];
    float map_u_scale;
    float map_v_scale;
    TagDependency base_map;  // bitmap
    uint8_t _pad_b4[8];
    TagDependency multipurpose_map;  // bitmap
    uint8_t _pad_cc[8];
    ShaderDetailFunction_t detail_function;
    ShaderModelDetailMask_t detail_mask;
    float detail_map_scale;
    TagDependency detail_map;  // bitmap
    float detail_map_v_scale;
    uint8_t _pad_f0[12];
    FunctionOut_t u_animation_source;
    WaveFunction_t u_animation_function;
    float u_animation_period;
    float u_animation_phase;
    float u_animation_scale;
    FunctionOut_t v_animation_source;
    WaveFunction_t v_animation_function;
    float v_animation_period;
    float v_animation_phase;
    float v_animation_scale;
    FunctionOut_t rotation_animation_source;
    WaveFunction_t rotation_animation_function;
    float rotation_animation_period;
    float rotation_animation_phase;
    float rotation_animation_scale;
    Point2D rotation_animation_center;
    uint8_t _pad_134[8];
    float reflection_falloff_distance;
    float reflection_cutoff_distance;
    float perpendicular_brightness;
    ColorRGB perpendicular_tint_color;
    float parallel_brightness;
    ColorRGB parallel_tint_color;
    TagDependency reflection_cube_map;  // bitmap
    uint8_t _pad_174[16];
    float unknown;
    uint8_t _pad_188[16];
    uint8_t _pad_198[32];
} ShaderModel;  // size 0x1b8
typedef uint16_t ShaderTransparentChicagoMapFlags;  // bitfield: unfiltered, alpha_replicate, u_clamped, v_clamped
typedef uint32_t ShaderTransparentChicagoExtraFlags;  // bitfield: don_t_fade_active_camouflage, numeric_countdown_timer, custom_edition_blending
typedef struct ShaderTransparentChicagoMap {
    ShaderTransparentChicagoMapFlags flags;
    uint8_t _pad_2[2];
    uint8_t _pad_4[40];
    ShaderColorFunctionType_t color_function;
    ShaderColorFunctionType_t alpha_function;
    uint8_t _pad_30[36];
    float map_u_scale;
    float map_v_scale;
    float map_u_offset;
    float map_v_offset;
    float map_rotation;
    float mipmap_bias;
    TagDependency map;  // bitmap
    uint8_t _pad_7c[40];
    FunctionOut_t u_animation_source;
    WaveFunction_t u_animation_function;
    float u_animation_period;
    float u_animation_phase;
    float u_animation_scale;
    FunctionOut_t v_animation_source;
    WaveFunction_t v_animation_function;
    float v_animation_period;
    float v_animation_phase;
    float v_animation_scale;
    FunctionOut_t rotation_animation_source;
    WaveFunction_t rotation_animation_function;
    float rotation_animation_period;
    float rotation_animation_phase;
    float rotation_animation_scale;
    Point2D rotation_animation_center;
} ShaderTransparentChicagoMap;  // size 0xdc
typedef uint8_t ShaderTransparentGenericFlags;  // bitfield: alpha_tested, decal, two_sided, first_map_is_in_screenspace, draw_before_water, ignore_effect, scale_first_map_with_distance, numeric
typedef struct ShaderTransparentChicago {
    Shader base;  // inherits
    uint8_t numeric_counter_limit;
    ShaderTransparentGenericFlags shader_transparent_chicago_flags;
    ShaderFirstMapType_t first_map_type;
    FramebufferBlendFunction_t framebuffer_blend_function;
    FramebufferFadeMode_t framebuffer_fade_mode;
    FunctionOut_t framebuffer_fade_source;
    uint8_t _pad_32[2];
    float lens_flare_spacing;
    TagDependency lens_flare;  // lens_flare
    TagReflexive extra_layers;  // ShaderTransparentExtraLayer
    TagReflexive maps;  // ShaderTransparentChicagoMap
    ShaderTransparentChicagoExtraFlags extra_flags;
    uint8_t _pad_64[8];
} ShaderTransparentChicago;  // size 0x6c
typedef struct ShaderTransparentChicagoExtended {
    Shader base;  // inherits
    uint8_t numeric_counter_limit;
    ShaderTransparentGenericFlags shader_transparent_chicago_extended_flags;
    ShaderFirstMapType_t first_map_type;
    FramebufferBlendFunction_t framebuffer_blend_function;
    FramebufferFadeMode_t framebuffer_fade_mode;
    FunctionOut_t framebuffer_fade_source;
    uint8_t _pad_32[2];
    float lens_flare_spacing;
    TagDependency lens_flare;  // lens_flare
    TagReflexive extra_layers;  // ShaderTransparentExtraLayer
    TagReflexive maps_4_stage;  // ShaderTransparentChicagoMap
    TagReflexive maps_2_stage;  // ShaderTransparentChicagoMap
    ShaderTransparentChicagoExtraFlags extra_flags;
    uint8_t _pad_70[8];
} ShaderTransparentChicagoExtended;  // size 0x78
typedef enum ShaderTransparentGenericStageInputColor {
    shadertransparentgenericstageinputcolor_zero = 0,
    shadertransparentgenericstageinputcolor_one = 1,
    shadertransparentgenericstageinputcolor_one_half = 2,
    shadertransparentgenericstageinputcolor_negative_one = 3,
    shadertransparentgenericstageinputcolor_negative_one_half = 4,
    shadertransparentgenericstageinputcolor_map_color_0 = 5,
    shadertransparentgenericstageinputcolor_map_color_1 = 6,
    shadertransparentgenericstageinputcolor_map_color_2 = 7,
    shadertransparentgenericstageinputcolor_map_color_3 = 8,
    shadertransparentgenericstageinputcolor_vertex_color_0_diffuse_light = 9,
    shadertransparentgenericstageinputcolor_vertex_color_1_fade_perpendicular = 10,
    shadertransparentgenericstageinputcolor_scratch_color_0 = 11,
    shadertransparentgenericstageinputcolor_scratch_color_1 = 12,
    shadertransparentgenericstageinputcolor_constant_color_0 = 13,
    shadertransparentgenericstageinputcolor_constant_color_1 = 14,
    shadertransparentgenericstageinputcolor_map_alpha_0 = 15,
    shadertransparentgenericstageinputcolor_map_alpha_1 = 16,
    shadertransparentgenericstageinputcolor_map_alpha_2 = 17,
    shadertransparentgenericstageinputcolor_map_alpha_3 = 18,
    shadertransparentgenericstageinputcolor_vertex_alpha_0_fade_none = 19,
    shadertransparentgenericstageinputcolor_vertex_alpha_1_fade_perpendicular = 20,
    shadertransparentgenericstageinputcolor_scratch_alpha_0 = 21,
    shadertransparentgenericstageinputcolor_scratch_alpha_1 = 22,
    shadertransparentgenericstageinputcolor_constant_alpha_0 = 23,
    shadertransparentgenericstageinputcolor_constant_alpha_1 = 24,
} ShaderTransparentGenericStageInputColor;  // int16
typedef int16_t ShaderTransparentGenericStageInputColor_t;
typedef enum ShaderTransparentGenericStageInputAlpha {
    shadertransparentgenericstageinputalpha_zero = 0,
    shadertransparentgenericstageinputalpha_one = 1,
    shadertransparentgenericstageinputalpha_one_half = 2,
    shadertransparentgenericstageinputalpha_negative_one = 3,
    shadertransparentgenericstageinputalpha_negative_one_half = 4,
    shadertransparentgenericstageinputalpha_map_alpha_0 = 5,
    shadertransparentgenericstageinputalpha_map_alpha_1 = 6,
    shadertransparentgenericstageinputalpha_map_alpha_2 = 7,
    shadertransparentgenericstageinputalpha_map_alpha_3 = 8,
    shadertransparentgenericstageinputalpha_vertex_alpha_0_fade_none = 9,
    shadertransparentgenericstageinputalpha_vertex_alpha_1_fade_perpendicular = 10,
    shadertransparentgenericstageinputalpha_scratch_alpha_0 = 11,
    shadertransparentgenericstageinputalpha_scratch_alpha_1 = 12,
    shadertransparentgenericstageinputalpha_constant_alpha_0 = 13,
    shadertransparentgenericstageinputalpha_constant_alpha_1 = 14,
    shadertransparentgenericstageinputalpha_map_blue_0 = 15,
    shadertransparentgenericstageinputalpha_map_blue_1 = 16,
    shadertransparentgenericstageinputalpha_map_blue_2 = 17,
    shadertransparentgenericstageinputalpha_map_blue_3 = 18,
    shadertransparentgenericstageinputalpha_vertex_blue_0_blue_light = 19,
    shadertransparentgenericstageinputalpha_vertex_blue_1_fade_parallel = 20,
    shadertransparentgenericstageinputalpha_scratch_blue_0 = 21,
    shadertransparentgenericstageinputalpha_scratch_blue_1 = 22,
    shadertransparentgenericstageinputalpha_constant_blue_0 = 23,
    shadertransparentgenericstageinputalpha_constant_blue_1 = 24,
} ShaderTransparentGenericStageInputAlpha;  // int16
typedef int16_t ShaderTransparentGenericStageInputAlpha_t;
typedef enum ShaderTransparentGenericStageInputMappingColor {
    shadertransparentgenericstageinputmappingcolor_clamp_x = 0,
    shadertransparentgenericstageinputmappingcolor_1_clamp_x = 1,
    shadertransparentgenericstageinputmappingcolor_2 = 2,
    shadertransparentgenericstageinputmappingcolor_1_2 = 3,
    shadertransparentgenericstageinputmappingcolor_clamp_x_1_2 = 4,
    shadertransparentgenericstageinputmappingcolor_1_2_clamp_x = 5,
    shadertransparentgenericstageinputmappingcolor_x = 6,
    shadertransparentgenericstageinputmappingcolor_x_1 = 7,
} ShaderTransparentGenericStageInputMappingColor;  // int16
typedef int16_t ShaderTransparentGenericStageInputMappingColor_t;
typedef enum ShaderTransparentGenericStageOutputFunction {
    shadertransparentgenericstageoutputfunction_multiply = 0,
    shadertransparentgenericstageoutputfunction_dot_product = 1,
} ShaderTransparentGenericStageOutputFunction;  // int16
typedef int16_t ShaderTransparentGenericStageOutputFunction_t;
typedef enum ShaderTransparentGenericStageOutputMapping {
    shadertransparentgenericstageoutputmapping_color_identity = 0,
    shadertransparentgenericstageoutputmapping_color_scale_by_1_2 = 1,
    shadertransparentgenericstageoutputmapping_color_scale_by_2 = 2,
    shadertransparentgenericstageoutputmapping_color_scale_by_4 = 3,
    shadertransparentgenericstageoutputmapping_color_bias_by_1_2 = 4,
    shadertransparentgenericstageoutputmapping_color_expand_normal = 5,
} ShaderTransparentGenericStageOutputMapping;  // int16
typedef int16_t ShaderTransparentGenericStageOutputMapping_t;
typedef enum ShaderTransparentGenericStageOutput {
    shadertransparentgenericstageoutput_alpha_discard = 0,
    shadertransparentgenericstageoutput_alpha_scratch_alpha_0_final_alpha = 1,
    shadertransparentgenericstageoutput_alpha_scratch_alpha_1 = 2,
    shadertransparentgenericstageoutput_alpha_vertex_alpha_0_fog = 3,
    shadertransparentgenericstageoutput_alpha_vertex_alpha_1 = 4,
    shadertransparentgenericstageoutput_alpha_map_alpha_0 = 5,
    shadertransparentgenericstageoutput_alpha_map_alpha_1 = 6,
    shadertransparentgenericstageoutput_alpha_map_alpha_2 = 7,
    shadertransparentgenericstageoutput_alpha_map_alpha_3 = 8,
} ShaderTransparentGenericStageOutput;  // int16
typedef int16_t ShaderTransparentGenericStageOutput_t;
typedef uint16_t ShaderTransparentGenericMapFlag;  // bitfield: unfiltered, u_clamped, v_clamped
typedef uint16_t ShaderTransparentGenericStageFlags;  // bitfield: color_mux, alpha_mux, a_out_controls_color0_animation
typedef struct ShaderTransparentGenericMap {
    ShaderTransparentGenericMapFlag flags;
    uint8_t _pad_2[2];
    float map_u_scale;
    float map_v_scale;
    float map_u_offset;
    float map_v_offset;
    float map_rotation;
    float mipmap_bias;
    TagDependency map;  // bitmap
    FunctionOut_t u_animation_source;
    WaveFunction_t u_animation_function;
    float u_animation_period;
    float u_animation_phase;
    float u_animation_scale;
    FunctionOut_t v_animation_source;
    WaveFunction_t v_animation_function;
    float v_animation_period;
    float v_animation_phase;
    float v_animation_scale;
    FunctionOut_t rotation_animation_source;
    WaveFunction_t rotation_animation_function;
    float rotation_animation_period;
    float rotation_animation_phase;
    float rotation_animation_scale;
    Point2D rotation_animation_center;
} ShaderTransparentGenericMap;  // size 0x64
typedef struct ShaderTransparentGenericStage {
    ShaderTransparentGenericStageFlags flags;
    uint8_t _pad_2[2];
    FunctionNameNullable_t color0_source;
    WaveFunction_t color0_animation_function;
    float color0_animation_period;
    ColorARGB color0_animation_lower_bound;
    ColorARGB color0_animation_upper_bound;
    ColorARGB color1;
    ShaderTransparentGenericStageInputColor_t input_a;
    ShaderTransparentGenericStageInputMappingColor_t input_a_mapping;
    ShaderTransparentGenericStageInputColor_t input_b;
    ShaderTransparentGenericStageInputMappingColor_t input_b_mapping;
    ShaderTransparentGenericStageInputColor_t input_c;
    ShaderTransparentGenericStageInputMappingColor_t input_c_mapping;
    ShaderTransparentGenericStageInputColor_t input_d;
    ShaderTransparentGenericStageInputMappingColor_t input_d_mapping;
    ShaderTransparentGenericStageOutput_t output_ab;
    ShaderTransparentGenericStageOutputFunction_t output_ab_function;
    ShaderTransparentGenericStageOutput_t output_bc;
    ShaderTransparentGenericStageOutputFunction_t output_cd_function;
    ShaderTransparentGenericStageOutput_t output_ab_cd_mux_sum;
    ShaderTransparentGenericStageOutputMapping_t output_mapping_color;
    ShaderTransparentGenericStageInputAlpha_t input_a_alpha;
    ShaderTransparentGenericStageInputMappingColor_t input_a_mapping_alpha;
    ShaderTransparentGenericStageInputAlpha_t input_b_alpha;
    ShaderTransparentGenericStageInputMappingColor_t input_b_mapping_alpha;
    ShaderTransparentGenericStageInputAlpha_t input_c_alpha;
    ShaderTransparentGenericStageInputMappingColor_t input_c_mapping_alpha;
    ShaderTransparentGenericStageInputAlpha_t input_d_alpha;
    ShaderTransparentGenericStageInputMappingColor_t input_d_mapping_alpha;
    ShaderTransparentGenericStageOutput_t output_ab_alpha;
    ShaderTransparentGenericStageOutput_t output_cd_alpha;
    ShaderTransparentGenericStageOutput_t output_ab_cd_mux_sum_alpha;
    ShaderTransparentGenericStageOutputMapping_t output_mapping_alpha;
} ShaderTransparentGenericStage;  // size 0x70
typedef struct ShaderTransparentGeneric {
    Shader base;  // inherits
    uint8_t numeric_counter_limit;
    ShaderTransparentGenericFlags shader_transparent_generic_flags;
    ShaderFirstMapType_t first_map_type;
    FramebufferBlendFunction_t framebuffer_blend_function;
    FramebufferFadeMode_t framebuffer_fade_mode;
    FunctionOut_t framebuffer_fade_source;
    uint8_t _pad_32[2];
    float lens_flare_spacing;
    TagDependency lens_flare;  // lens_flare
    TagReflexive extra_layers;  // ShaderTransparentExtraLayer
    TagReflexive maps;  // ShaderTransparentGenericMap
    TagReflexive stages;  // ShaderTransparentGenericStage
} ShaderTransparentGeneric;  // size 0x6c
typedef enum ShaderTransparentGlassReflectionType {
    shadertransparentglassreflectiontype_bumped_cube_map = 0,
    shadertransparentglassreflectiontype_flat_cube_map = 1,
    shadertransparentglassreflectiontype_dynamic_mirror = 2,
} ShaderTransparentGlassReflectionType;  // int16
typedef int16_t ShaderTransparentGlassReflectionType_t;
typedef uint16_t ShaderTransparentGlassFlags;  // bitfield: alpha_tested, decal, two_sided, bump_map_is_specular_mask
typedef struct ShaderTransparentGlass {
    Shader base;  // inherits
    ShaderTransparentGlassFlags shader_transparent_glass_flags;
    uint8_t _pad_2a[2];
    uint8_t _pad_2c[40];
    ColorRGB background_tint_color;
    float background_tint_map_scale;
    TagDependency background_tint_map;  // bitmap
    uint8_t _pad_74[20];
    uint8_t _pad_88[2];
    ShaderTransparentGlassReflectionType_t reflection_type;
    float perpendicular_brightness;
    ColorRGB perpendicular_tint_color;
    float parallel_brightness;
    ColorRGB parallel_tint_color;
    TagDependency reflection_map;  // bitmap
    float bump_map_scale;
    TagDependency bump_map;  // bitmap
    uint8_t _pad_d0[128];
    uint8_t _pad_150[4];
    float diffuse_map_scale;
    TagDependency diffuse_map;  // bitmap
    float diffuse_detail_map_scale;
    TagDependency diffuse_detail_map;  // bitmap
    uint8_t _pad_17c[28];
    uint8_t _pad_198[4];
    float specular_map_scale;
    TagDependency specular_map;  // bitmap
    float specular_detail_map_scale;
    TagDependency specular_detail_map;  // bitmap
    uint8_t _pad_1c4[28];
} ShaderTransparentGlass;  // size 0x1e0
typedef uint16_t ShaderTransparentMeterFlags;  // bitfield: decal, two_sided, flash_color_is_negative, tint_mode_2, unfiltered
typedef struct ShaderTransparentMeter {
    Shader base;  // inherits
    ShaderTransparentMeterFlags meter_flags;
    uint8_t _pad_2a[2];
    uint8_t _pad_2c[32];
    TagDependency map;  // bitmap
    uint8_t _pad_5c[32];
    ColorRGB gradient_min_color;
    ColorRGB gradient_max_color;
    ColorRGB background_color;
    ColorRGB flash_color;
    ColorRGB meter_tint_color;
    float meter_transparency;
    float background_transparency;
    uint8_t _pad_c0[24];
    FunctionOut_t meter_brightness_source;
    FunctionOut_t flash_brightness_source;
    FunctionOut_t value_source;
    FunctionOut_t gradient_source;
    FunctionOut_t flash_extension_source;
    uint8_t _pad_e2[2];
    uint8_t _pad_e4[32];
} ShaderTransparentMeter;  // size 0x104
typedef struct ShaderTransparentPlasma {
    Shader base;  // inherits
    uint8_t _pad_28[2];
    uint8_t _pad_2a[2];
    FunctionOut_t intensity_source;
    uint8_t _pad_2e[2];
    float intensity_exponent;
    FunctionOut_t offset_source;
    uint8_t _pad_36[2];
    float offset_amount;
    float offset_exponent;
    uint8_t _pad_40[32];
    float perpendicular_brightness;
    ColorRGB perpendicular_tint_color;
    float parallel_brightness;
    ColorRGB parallel_tint_color;
    FunctionNameNullable_t tint_color_source;
    uint8_t _pad_82[2];
    uint8_t _pad_84[32];
    uint8_t _pad_a4[2];
    uint8_t _pad_a6[2];
    uint8_t _pad_a8[16];
    uint8_t _pad_b8[4];
    uint8_t _pad_bc[4];
    float primary_animation_period;
    Vector3D primary_animation_direction;
    float primary_noise_map_scale;
    TagDependency primary_noise_map;  // bitmap
    uint8_t _pad_e4[32];
    uint8_t _pad_104[4];
    float secondary_animation_period;
    Vector3D secondary_animation_direction;
    float secondary_noise_map_scale;
    TagDependency secondary_noise_map;  // bitmap
    uint8_t _pad_12c[32];
} ShaderTransparentPlasma;  // size 0x14c
typedef uint16_t ShaderTransparentWaterFlags;  // bitfield: base_map_alpha_modulates_reflection, base_map_color_modulates_background, atmospheric_fog, draw_before_fog
typedef struct ShaderTransparentWaterRipple {
    uint8_t _pad_0[2];
    uint8_t _pad_2[2];
    float contribution_factor;
    uint8_t _pad_8[32];
    float animation_angle;
    float animation_velocity;
    Vector2D map_offset;
    uint16_t map_repeats;
    uint16_t map_index;
    uint8_t _pad_3c[16];
} ShaderTransparentWaterRipple;  // size 0x4c
typedef struct ShaderTransparentWater {
    Shader base;  // inherits
    ShaderTransparentWaterFlags water_flags;
    uint8_t _pad_2a[2];
    uint8_t _pad_2c[32];
    TagDependency base_map;  // bitmap
    uint8_t _pad_5c[16];
    float view_perpendicular_brightness;
    ColorRGB view_perpendicular_tint_color;
    float view_parallel_brightness;
    ColorRGB view_parallel_tint_color;
    uint8_t _pad_8c[16];
    TagDependency reflection_map;  // bitmap
    uint8_t _pad_ac[16];
    float ripple_animation_angle;
    float ripple_animation_velocity;
    float ripple_scale;
    TagDependency ripple_maps;  // bitmap
    uint16_t ripple_mipmap_levels;
    uint8_t _pad_da[2];
    float ripple_mipmap_fade_factor;
    float ripple_mipmap_detail_bias;
    uint8_t _pad_e4[64];
    TagReflexive ripples;  // ShaderTransparentWaterRipple
    uint8_t _pad_130[16];
} ShaderTransparentWater;  // size 0x140
typedef uint32_t SkyLightFlags;  // bitfield: affects_exteriors, affects_interiors
typedef struct SkyFunction {
    uint8_t _pad_0[4];
    TagString function_name;
} SkyFunction;  // size 0x24
typedef struct SkyAnimation {
    int16_t animation_index;
    uint8_t _pad_2[2];
    float period;
    uint8_t _pad_8[28];
} SkyAnimation;  // size 0x24
typedef struct SkyLight {
    TagDependency lens_flare;  // lens_flare
    TagString lens_flare_marker_name;
    uint8_t _pad_30[28];
    SkyLightFlags flags;
    ColorRGB color;
    float power;
    float test_distance;
    uint8_t _pad_64[4];
    Euler2D direction;
    float diameter;
} SkyLight;  // size 0x74
typedef struct Sky {
    TagDependency model;  // model
    TagDependency animation_graph;  // model_animations
    uint8_t _pad_20[24];
    ColorRGB indoor_ambient_radiosity_color;
    float indoor_ambient_radiosity_power;
    ColorRGB outdoor_ambient_radiosity_color;
    float outdoor_ambient_radiosity_power;
    ColorRGB outdoor_fog_color;
    uint8_t _pad_64[8];
    float outdoor_fog_maximum_density;
    float outdoor_fog_start_distance;
    float outdoor_fog_opaque_distance;
    ColorRGB indoor_fog_color;
    uint8_t _pad_84[8];
    float indoor_fog_maximum_density;
    float indoor_fog_start_distance;
    float indoor_fog_opaque_distance;
    TagDependency indoor_fog_screen;  // fog
    uint8_t _pad_a8[4];
    TagReflexive shader_functions;  // SkyFunction
    TagReflexive animations;  // SkyAnimation
    TagReflexive lights;  // SkyLight
} Sky;  // size 0xd0
typedef enum SoundFormat {
    soundformat_16_bit_pcm = 0,
    soundformat_xbox_adpcm = 1,
    soundformat_ima_adpcm = 2,
    soundformat_ogg_vorbis = 3,
} SoundFormat;  // int16
typedef int16_t SoundFormat_t;
typedef enum SoundClass {
    soundclass_projectile_impact = 0,
    soundclass_projectile_detonation = 1,
    soundclass_unused = 2,
    soundclass_unused1 = 3,
    soundclass_weapon_fire = 4,
    soundclass_weapon_ready = 5,
    soundclass_weapon_reload = 6,
    soundclass_weapon_empty = 7,
    soundclass_weapon_charge = 8,
    soundclass_weapon_overheat = 9,
    soundclass_weapon_idle = 10,
    soundclass_unused2 = 11,
    soundclass_unused3 = 12,
    soundclass_object_impacts = 13,
    soundclass_particle_impacts = 14,
    soundclass_slow_particle_impacts = 15,
    soundclass_unused4 = 16,
    soundclass_unused5 = 17,
    soundclass_unit_footsteps = 18,
    soundclass_unit_dialog = 19,
    soundclass_unused6 = 20,
    soundclass_unused7 = 21,
    soundclass_vehicle_collision = 22,
    soundclass_vehicle_engine = 23,
    soundclass_unused8 = 24,
    soundclass_unused9 = 25,
    soundclass_device_door = 26,
    soundclass_device_force_field = 27,
    soundclass_device_machinery = 28,
    soundclass_device_nature = 29,
    soundclass_device_computers = 30,
    soundclass_unused10 = 31,
    soundclass_music = 32,
    soundclass_ambient_nature = 33,
    soundclass_ambient_machinery = 34,
    soundclass_ambient_computers = 35,
    soundclass_unused11 = 36,
    soundclass_unused12 = 37,
    soundclass_unused13 = 38,
    soundclass_first_person_damage = 39,
    soundclass_unused14 = 40,
    soundclass_unused15 = 41,
    soundclass_unused16 = 42,
    soundclass_unused17 = 43,
    soundclass_scripted_dialog_player = 44,
    soundclass_scripted_effect = 45,
    soundclass_scripted_dialog_other = 46,
    soundclass_scripted_dialog_force_unspatialized = 47,
    soundclass_unused18 = 48,
    soundclass_unused19 = 49,
    soundclass_game_event = 50,
} SoundClass;  // int16
typedef int16_t SoundClass_t;
typedef enum SoundSampleRate {
    soundsamplerate_22050_hz = 0,
    soundsamplerate_44100_hz = 1,
} SoundSampleRate;  // int16
typedef int16_t SoundSampleRate_t;
typedef enum SoundChannelCount {
    soundchannelcount_mono = 0,
    soundchannelcount_stereo = 1,
} SoundChannelCount;  // int16
typedef int16_t SoundChannelCount_t;
typedef uint32_t SoundFlags;  // bitfield: fit_to_adpcm_blocksize, split_long_sound_into_permutations, thirsty_grunt
typedef struct SoundPermutation {
    TagString name;
    float skip_fraction;
    float gain;
    SoundFormat_t format;
    uint16_t next_permutation_index;
    uint32_t samples_pointer;  // retail PC runtime: the sound cache handle, -1 when not cached
    void *cache_page;  // retail PC runtime: the sound cache page holding the samples, 0 when none
    TagID tag_id_0;
    uint32_t buffer_size;
    TagID tag_id_1;
    TagDataOffset samples;
    TagDataOffset mouth_data;
    TagDataOffset subtitle_data;
} SoundPermutation;  // size 0x7c
typedef struct SoundPitchRange {
    TagString name;
    float natural_pitch;
    float bend_bounds[2];
    uint16_t actual_permutation_count;
    uint8_t _pad_2e[2];
    float playback_rate;
    uint32_t permutation_flags;
    uint16_t last_permutation_index;
    uint16_t discarded_permutation_index;
    TagReflexive permutations;  // SoundPermutation
} SoundPitchRange;  // size 0x48
typedef struct Sound {
    SoundFlags flags;
    SoundClass_t sound_class;
    SoundSampleRate_t sample_rate;
    float minimum_distance;
    float maximum_distance;
    float skip_fraction;
    float random_pitch_bounds[2];
    float inner_cone_angle;
    float outer_cone_angle;
    float outer_cone_gain;
    float random_gain_modifier;
    float maximum_bend_per_second;
    uint8_t _pad_30[12];
    float zero_skip_fraction_modifier;
    float zero_gain_modifier;
    float zero_pitch_modifier;
    uint8_t _pad_48[12];
    float one_skip_fraction_modifier;
    float one_gain_modifier;
    float one_pitch_modifier;
    uint8_t _pad_60[12];
    SoundChannelCount_t channel_count;
    SoundFormat_t format;
    TagDependency promotion_sound;  // sound
    uint16_t promotion_count;
    uint8_t _pad_82[2];
    uint32_t longest_permutation_length;
    uint32_t promotion_counter;
    uint32_t promotion_time;
    uint32_t scripting_time;
    TagID scripting_sound;
    TagReflexive pitch_ranges;  // SoundPitchRange
} Sound;  // size 0xa4
typedef struct SoundEnvironment {
    int32_t unknown;
    int16_t priority;
    uint8_t _pad_6[2];
    float room_intensity;
    float room_intensity_hf;
    float room_rolloff;
    float decay_time;
    float decay_hf_ratio;
    float reflections_intensity;
    float reflections_delay;
    float reverb_intensity;
    float reverb_delay;
    float diffusion;
    float density;
    float hf_reference;
    uint8_t _pad_38[16];
} SoundEnvironment;  // size 0x48
typedef uint32_t SoundLoopingTrackFlags;  // bitfield: fade_in_at_start, fade_out_at_stop, fade_in_alternate
typedef uint32_t SoundLoopingDetailFlags;  // bitfield: don_t_play_with_alternate, don_t_play_without_alternate
typedef uint32_t SoundLoopingFlags;  // bitfield: deafening_to_ais, not_a_loop, stops_music, siege_of_madrigal
typedef struct SoundLoopingTrack {
    SoundLoopingTrackFlags flags;
    float gain;
    float fade_in_duration;
    float fade_out_duration;
    uint8_t _pad_10[32];
    TagDependency start;  // sound
    TagDependency loop;  // sound
    TagDependency end;  // sound
    uint8_t _pad_60[32];
    TagDependency alternate_loop;  // sound
    TagDependency alternate_end;  // sound
} SoundLoopingTrack;  // size 0xa0
typedef struct SoundLoopingDetail {
    TagDependency sound;  // sound
    float random_period_bounds[2];
    float gain;
    SoundLoopingDetailFlags flags;
    uint8_t _pad_20[48];
    float yaw_bounds[2];
    float pitch_bounds[2];
    float distance_bounds[2];
} SoundLoopingDetail;  // size 0x68
typedef struct SoundLooping {
    SoundLoopingFlags flags;
    float zero_detail_sound_period;
    float zero_detail_unused[2];
    float one_detail_sound_period;
    float one_detail_unused[2];
    TagID runtime_scripting_sound;
    float maximum_distance;
    uint8_t _pad_24[8];
    TagDependency continuous_damage_effect;  // continuous_damage_effect
    TagReflexive tracks;  // SoundLoopingTrack
    TagReflexive detail_sounds;  // SoundLoopingDetail
} SoundLooping;  // size 0x54
typedef struct SoundScenery {
    BasicObject base;  // inherits
} SoundScenery;  // size 0x1fc
typedef struct StringListString {
    TagDataOffset string;
} StringListString;  // size 0x14
typedef struct StringList {
    TagReflexive strings;  // StringListString
} StringList;  // size 0xc
typedef struct TagCollectionTag {
    TagDependency reference;  // *
} TagCollectionTag;  // size 0x10
typedef struct TagCollection {
    TagReflexive tags;  // TagCollectionTag
} TagCollection;  // size 0xc
typedef enum UIGameDataInputReferenceFunction {
    uigamedatainputreferencefunction_null = 0,
    uigamedatainputreferencefunction_player_settings_menu_update_desc = 1,
    uigamedatainputreferencefunction_unused = 2,
    uigamedatainputreferencefunction_playlist_settings_menu_update_desc = 3,
    uigamedatainputreferencefunction_gametype_select_menu_update_desc = 4,
    uigamedatainputreferencefunction_multiplayer_type_menu_update_desc = 5,
    uigamedatainputreferencefunction_solo_level_select_update = 6,
    uigamedatainputreferencefunction_difficulty_menu_update_desc = 7,
    uigamedatainputreferencefunction_build_number_textbox_only = 8,
    uigamedatainputreferencefunction_server_list_update = 9,
    uigamedatainputreferencefunction_network_pregame_status_update = 10,
    uigamedatainputreferencefunction_splitscreen_pregame_status_update = 11,
    uigamedatainputreferencefunction_net_splitscreen_prejoin_players = 12,
    uigamedatainputreferencefunction_mp_profile_list_update = 13,
    uigamedatainputreferencefunction_3wide_player_profile_list_update = 14,
    uigamedatainputreferencefunction_plyr_prof_edit_select_menu_upd8 = 15,
    uigamedatainputreferencefunction_player_profile_small_menu_update = 16,
    uigamedatainputreferencefunction_game_settings_lists_text_update = 17,
    uigamedatainputreferencefunction_solo_game_objective_text = 18,
    uigamedatainputreferencefunction_color_picker_update = 19,
    uigamedatainputreferencefunction_game_settings_lists_pic_update = 20,
    uigamedatainputreferencefunction_main_menu_fake_animate = 21,
    uigamedatainputreferencefunction_mp_level_select_update = 22,
    uigamedatainputreferencefunction_get_active_plyr_profile_name = 23,
    uigamedatainputreferencefunction_get_edit_plyr_profile_name = 24,
    uigamedatainputreferencefunction_get_edit_game_settings_name = 25,
    uigamedatainputreferencefunction_get_active_plyr_profile_color = 26,
    uigamedatainputreferencefunction_mp_set_textbox_map_name = 27,
    uigamedatainputreferencefunction_mp_set_textbox_game_ruleset = 28,
    uigamedatainputreferencefunction_mp_set_textbox_teams_noteams = 29,
    uigamedatainputreferencefunction_mp_set_textbox_score_limit = 30,
    uigamedatainputreferencefunction_mp_set_textbox_score_limit_type = 31,
    uigamedatainputreferencefunction_mp_set_bitmap_for_map = 32,
    uigamedatainputreferencefunction_mp_set_bitmap_for_ruleset = 33,
    uigamedatainputreferencefunction_mp_set_textbox = 34,
    uigamedatainputreferencefunction_mp_edit_profile_set_rule_text = 35,
    uigamedatainputreferencefunction_system_link_status_check = 36,
    uigamedatainputreferencefunction_mp_game_directions = 37,
    uigamedatainputreferencefunction_teams_no_teams_bitmap_update = 38,
    uigamedatainputreferencefunction_warn_if_diff_will_nuke_saved_game = 39,
    uigamedatainputreferencefunction_dim_if_no_net_cable = 40,
    uigamedatainputreferencefunction_pause_game_set_textbox_inverted = 41,
    uigamedatainputreferencefunction_dim_unless_two_controllers = 42,
    uigamedatainputreferencefunction_controls_update_menu = 43,
    uigamedatainputreferencefunction_video_menu_update = 44,
    uigamedatainputreferencefunction_gamespy_screen_update = 45,
    uigamedatainputreferencefunction_common_button_bar_update = 46,
    uigamedatainputreferencefunction_gamepad_update_menu = 47,
    uigamedatainputreferencefunction_server_settings_update = 48,
    uigamedatainputreferencefunction_audio_menu_update = 49,
    uigamedatainputreferencefunction_mp_prof_vehicles_update = 50,
    uigamedatainputreferencefunction_solo_map_list_update = 51,
    uigamedatainputreferencefunction_mp_map_list_update = 52,
    uigamedatainputreferencefunction_gt_select_list_update = 53,
    uigamedatainputreferencefunction_gt_edit_list_update = 54,
    uigamedatainputreferencefunction_load_game_list_update = 55,
    uigamedatainputreferencefunction_checking_for_updates = 56,
    uigamedatainputreferencefunction_direct_ip_connect_update = 57,
    uigamedatainputreferencefunction_network_settings_update = 58,
} UIGameDataInputReferenceFunction;  // int16
typedef int16_t UIGameDataInputReferenceFunction_t;
typedef enum UIEventType {
    uieventtype_a_button = 0,
    uieventtype_b_button = 1,
    uieventtype_x_button = 2,
    uieventtype_y_button = 3,
    uieventtype_black_button = 4,
    uieventtype_white_button = 5,
    uieventtype_left_trigger = 6,
    uieventtype_right_trigger = 7,
    uieventtype_dpad_up = 8,
    uieventtype_dpad_down = 9,
    uieventtype_dpad_left = 10,
    uieventtype_dpad_right = 11,
    uieventtype_start_button = 12,
    uieventtype_back_button = 13,
    uieventtype_left_thumb = 14,
    uieventtype_right_thumb = 15,
    uieventtype_left_analog_stick_up = 16,
    uieventtype_left_analog_stick_down = 17,
    uieventtype_left_analog_stick_left = 18,
    uieventtype_left_analog_stick_right = 19,
    uieventtype_left_analog_stick_up_1 = 20,
    uieventtype_right_analog_stick_down = 21,
    uieventtype_right_analog_stick_left = 22,
    uieventtype_right_analog_stick_right = 23,
    uieventtype_created = 24,
    uieventtype_deleted = 25,
    uieventtype_get_focus = 26,
    uieventtype_lose_focus = 27,
    uieventtype_left_mouse = 28,
    uieventtype_middle_mouse = 29,
    uieventtype_right_mouse = 30,
    uieventtype_double_click = 31,
    uieventtype_custom_activation = 32,
    uieventtype_post_render = 33,
} UIEventType;  // int16
typedef int16_t UIEventType_t;
typedef enum UIEventHandlerReferenceFunction {
    uieventhandlerreferencefunction_null = 0,
    uieventhandlerreferencefunction_list_goto_next_item = 1,
    uieventhandlerreferencefunction_list_goto_previous_item = 2,
    uieventhandlerreferencefunction_unused = 3,
    uieventhandlerreferencefunction_unused1 = 4,
    uieventhandlerreferencefunction_initialize_sp_level_list_solo = 5,
    uieventhandlerreferencefunction_initialize_sp_level_list_coop = 6,
    uieventhandlerreferencefunction_dispose_sp_level_list = 7,
    uieventhandlerreferencefunction_solo_level_set_map = 8,
    uieventhandlerreferencefunction_set_difficulty = 9,
    uieventhandlerreferencefunction_start_new_game = 10,
    uieventhandlerreferencefunction_pause_game_restart_at_checkpoint = 11,
    uieventhandlerreferencefunction_pause_game_restart_level = 12,
    uieventhandlerreferencefunction_pause_game_return_to_main_menu = 13,
    uieventhandlerreferencefunction_clear_multiplayer_player_joins = 14,
    uieventhandlerreferencefunction_join_controller_to_mp_game = 15,
    uieventhandlerreferencefunction_initialize_net_game_server_list = 16,
    uieventhandlerreferencefunction_start_network_game_server = 17,
    uieventhandlerreferencefunction_dispose_net_game_server_list = 18,
    uieventhandlerreferencefunction_shutdown_network_game = 19,
    uieventhandlerreferencefunction_net_game_join_from_server_list = 20,
    uieventhandlerreferencefunction_split_screen_game_initialize = 21,
    uieventhandlerreferencefunction_coop_game_initialize = 22,
    uieventhandlerreferencefunction_main_menu_intialize = 23,
    uieventhandlerreferencefunction_mp_type_menu_initialize = 24,
    uieventhandlerreferencefunction_pick_play_stage_for_quick_start = 25,
    uieventhandlerreferencefunction_mp_level_list_initialize = 26,
    uieventhandlerreferencefunction_mp_level_list_dispose = 27,
    uieventhandlerreferencefunction_mp_level_select = 28,
    uieventhandlerreferencefunction_mp_profiles_list_initialize = 29,
    uieventhandlerreferencefunction_mp_profiles_list_dispose = 30,
    uieventhandlerreferencefunction_mp_profile_set_for_game = 31,
    uieventhandlerreferencefunction_swap_player_team = 32,
    uieventhandlerreferencefunction_net_game_join_player = 33,
    uieventhandlerreferencefunction_player_profile_list_initialize = 34,
    uieventhandlerreferencefunction_player_profile_list_dispose = 35,
    uieventhandlerreferencefunction_3wide_plyr_prof_set_for_game = 36,
    uieventhandlerreferencefunction_1wide_plyr_prof_set_for_game = 37,
    uieventhandlerreferencefunction_mp_profile_begin_editing = 38,
    uieventhandlerreferencefunction_mp_profile_end_editing = 39,
    uieventhandlerreferencefunction_mp_profile_set_game_engine = 40,
    uieventhandlerreferencefunction_mp_profile_change_name = 41,
    uieventhandlerreferencefunction_mp_profile_set_ctf_rules = 42,
    uieventhandlerreferencefunction_mp_profile_set_koth_rules = 43,
    uieventhandlerreferencefunction_mp_profile_set_slayer_rules = 44,
    uieventhandlerreferencefunction_mp_profile_set_oddball_rules = 45,
    uieventhandlerreferencefunction_mp_profile_set_racing_rules = 46,
    uieventhandlerreferencefunction_mp_profile_set_player_options = 47,
    uieventhandlerreferencefunction_mp_profile_set_item_options = 48,
    uieventhandlerreferencefunction_mp_profile_set_indicator_opts = 49,
    uieventhandlerreferencefunction_mp_profile_init_game_engine = 50,
    uieventhandlerreferencefunction_mp_profile_init_name = 51,
    uieventhandlerreferencefunction_mp_profile_init_ctf_rules = 52,
    uieventhandlerreferencefunction_mp_profile_init_koth_rules = 53,
    uieventhandlerreferencefunction_mp_profile_init_slayer_rules = 54,
    uieventhandlerreferencefunction_mp_profile_init_oddball_rules = 55,
    uieventhandlerreferencefunction_mp_profile_init_racing_rules = 56,
    uieventhandlerreferencefunction_mp_profile_init_player_opts = 57,
    uieventhandlerreferencefunction_mp_profile_init_item_options = 58,
    uieventhandlerreferencefunction_mp_profile_init_indicator_opts = 59,
    uieventhandlerreferencefunction_mp_profile_save_changes = 60,
    uieventhandlerreferencefunction_color_picker_menu_initialize = 61,
    uieventhandlerreferencefunction_color_picker_menu_dispose = 62,
    uieventhandlerreferencefunction_color_picker_select_color = 63,
    uieventhandlerreferencefunction_player_profile_begin_editing = 64,
    uieventhandlerreferencefunction_player_profile_end_editing = 65,
    uieventhandlerreferencefunction_player_profile_change_name = 66,
    uieventhandlerreferencefunction_player_profile_save_changes = 67,
    uieventhandlerreferencefunction_plyr_prf_init_cntl_settings = 68,
    uieventhandlerreferencefunction_plyr_prf_init_adv_cntl_set = 69,
    uieventhandlerreferencefunction_plyr_prf_save_cntl_settings = 70,
    uieventhandlerreferencefunction_plyr_prf_save_adv_cntl_set = 71,
    uieventhandlerreferencefunction_mp_game_player_quit = 72,
    uieventhandlerreferencefunction_main_menu_switch_to_solo_game = 73,
    uieventhandlerreferencefunction_request_del_player_profile = 74,
    uieventhandlerreferencefunction_request_del_playlist_profile = 75,
    uieventhandlerreferencefunction_final_del_player_profile = 76,
    uieventhandlerreferencefunction_final_del_playlist_profile = 77,
    uieventhandlerreferencefunction_cancel_profile_delete = 78,
    uieventhandlerreferencefunction_create_edit_playlist_profile = 79,
    uieventhandlerreferencefunction_create_edit_player_profile = 80,
    uieventhandlerreferencefunction_net_game_speed_start = 81,
    uieventhandlerreferencefunction_net_game_delay_start = 82,
    uieventhandlerreferencefunction_net_server_accept_conx = 83,
    uieventhandlerreferencefunction_net_server_defer_start = 84,
    uieventhandlerreferencefunction_net_server_allow_start = 85,
    uieventhandlerreferencefunction_disable_if_no_xdemos = 86,
    uieventhandlerreferencefunction_run_xdemos = 87,
    uieventhandlerreferencefunction_sp_reset_controller_choices = 88,
    uieventhandlerreferencefunction_sp_set_p1_controller_choice = 89,
    uieventhandlerreferencefunction_sp_set_p2_controller_choice = 90,
    uieventhandlerreferencefunction_error_if_no_network_connection = 91,
    uieventhandlerreferencefunction_start_server_if_none_advertised = 92,
    uieventhandlerreferencefunction_net_game_unjoin_player = 93,
    uieventhandlerreferencefunction_close_if_not_editing_profile = 94,
    uieventhandlerreferencefunction_exit_to_xbox_dashboard = 95,
    uieventhandlerreferencefunction_new_campaign_chosen = 96,
    uieventhandlerreferencefunction_new_campaign_decision = 97,
    uieventhandlerreferencefunction_pop_history_stack_once = 98,
    uieventhandlerreferencefunction_difficulty_menu_init = 99,
    uieventhandlerreferencefunction_begin_music_fade_out = 100,
    uieventhandlerreferencefunction_new_game_if_no_plyr_profiles = 101,
    uieventhandlerreferencefunction_exit_gracefully_to_xbox_dashboard = 102,
    uieventhandlerreferencefunction_pause_game_invert_pitch = 103,
    uieventhandlerreferencefunction_start_new_coop_game = 104,
    uieventhandlerreferencefunction_pause_game_invert_spinner_get = 105,
    uieventhandlerreferencefunction_pause_game_invert_spinner_set = 106,
    uieventhandlerreferencefunction_main_menu_quit_game = 107,
    uieventhandlerreferencefunction_mouse_emit_accept_event = 108,
    uieventhandlerreferencefunction_mouse_emit_back_event = 109,
    uieventhandlerreferencefunction_mouse_emit_dpad_left_event = 110,
    uieventhandlerreferencefunction_mouse_emit_dpad_right_event = 111,
    uieventhandlerreferencefunction_mouse_spinner_3wide_click = 112,
    uieventhandlerreferencefunction_controls_screen_init = 113,
    uieventhandlerreferencefunction_video_screen_init = 114,
    uieventhandlerreferencefunction_controls_begin_binding = 115,
    uieventhandlerreferencefunction_gamespy_screen_init = 116,
    uieventhandlerreferencefunction_gamespy_screen_dispose = 117,
    uieventhandlerreferencefunction_gamespy_select_header = 118,
    uieventhandlerreferencefunction_gamespy_select_item = 119,
    uieventhandlerreferencefunction_gamespy_select_button = 120,
    uieventhandlerreferencefunction_plr_prof_init_mouse_set = 121,
    uieventhandlerreferencefunction_plr_prof_change_mouse_set = 122,
    uieventhandlerreferencefunction_plr_prof_init_audio_set = 123,
    uieventhandlerreferencefunction_plr_prof_change_audio_set = 124,
    uieventhandlerreferencefunction_plr_prof_change_video_set = 125,
    uieventhandlerreferencefunction_controls_screen_dispose = 126,
    uieventhandlerreferencefunction_controls_screen_change_set = 127,
    uieventhandlerreferencefunction_mouse_emit_x_event = 128,
    uieventhandlerreferencefunction_gamepad_screen_init = 129,
    uieventhandlerreferencefunction_gamepad_screen_dispose = 130,
    uieventhandlerreferencefunction_gamepad_screen_change_gamepads = 131,
    uieventhandlerreferencefunction_gamepad_screen_select_item = 132,
    uieventhandlerreferencefunction_mouse_screen_defaults = 133,
    uieventhandlerreferencefunction_audio_screen_defaults = 134,
    uieventhandlerreferencefunction_video_screen_defaults = 135,
    uieventhandlerreferencefunction_controls_screen_defaults = 136,
    uieventhandlerreferencefunction_profile_set_edit_begin = 137,
    uieventhandlerreferencefunction_profile_manager_delete = 138,
    uieventhandlerreferencefunction_profile_manager_select = 139,
    uieventhandlerreferencefunction_gamespy_dismiss_error = 140,
    uieventhandlerreferencefunction_server_settings_init = 141,
    uieventhandlerreferencefunction_ss_edit_server_name = 142,
    uieventhandlerreferencefunction_ss_edit_server_password = 143,
    uieventhandlerreferencefunction_ss_start_game = 144,
    uieventhandlerreferencefunction_video_test_dialog_init = 145,
    uieventhandlerreferencefunction_video_test_dialog_dispose = 146,
    uieventhandlerreferencefunction_video_test_dialog_accept = 147,
    uieventhandlerreferencefunction_gamespy_dismiss_filters = 148,
    uieventhandlerreferencefunction_gamespy_update_filter_settings = 149,
    uieventhandlerreferencefunction_gamespy_back_handler = 150,
    uieventhandlerreferencefunction_mouse_spinner_1wide_click = 151,
    uieventhandlerreferencefunction_controls_back_handler = 152,
    uieventhandlerreferencefunction_controls_advanced_launch = 153,
    uieventhandlerreferencefunction_controls_advanced_ok = 154,
    uieventhandlerreferencefunction_mp_pause_menu_open = 155,
    uieventhandlerreferencefunction_mp_game_options_open = 156,
    uieventhandlerreferencefunction_mp_choose_team = 157,
    uieventhandlerreferencefunction_mp_prof_init_vehicle_options = 158,
    uieventhandlerreferencefunction_mp_prof_save_vehicle_options = 159,
    uieventhandlerreferencefunction_single_prev_cl_item_activated = 160,
    uieventhandlerreferencefunction_mp_prof_init_teamplay_options = 161,
    uieventhandlerreferencefunction_mp_prof_save_teamplay_options = 162,
    uieventhandlerreferencefunction_mp_game_options_choose = 163,
    uieventhandlerreferencefunction_emit_custom_activation_event = 164,
    uieventhandlerreferencefunction_plr_prof_cancel_audio_set = 165,
    uieventhandlerreferencefunction_plr_prof_init_network_options = 166,
    uieventhandlerreferencefunction_plr_prof_save_network_options = 167,
    uieventhandlerreferencefunction_credits_post_render = 168,
    uieventhandlerreferencefunction_difficulty_item_select = 169,
    uieventhandlerreferencefunction_credits_initialize = 170,
    uieventhandlerreferencefunction_credits_dispose = 171,
    uieventhandlerreferencefunction_gamespy_get_patch = 172,
    uieventhandlerreferencefunction_video_screen_dispose = 173,
    uieventhandlerreferencefunction_campaign_menu_init = 174,
    uieventhandlerreferencefunction_campaign_menu_continue = 175,
    uieventhandlerreferencefunction_load_game_menu_init = 176,
    uieventhandlerreferencefunction_load_game_menu_dispose = 177,
    uieventhandlerreferencefunction_load_game_menu_activated = 178,
    uieventhandlerreferencefunction_solo_menu_save_checkpoint = 179,
    uieventhandlerreferencefunction_mp_type_set_mode = 180,
    uieventhandlerreferencefunction_checking_for_updates_ok = 181,
    uieventhandlerreferencefunction_checking_for_updates_dismiss = 182,
    uieventhandlerreferencefunction_direct_ip_connect_init = 183,
    uieventhandlerreferencefunction_direct_ip_connect_go = 184,
    uieventhandlerreferencefunction_direct_ip_edit_field = 185,
    uieventhandlerreferencefunction_network_settings_edit_a_port = 186,
    uieventhandlerreferencefunction_network_settings_defaults = 187,
    uieventhandlerreferencefunction_load_game_menu_delete_request = 188,
    uieventhandlerreferencefunction_load_game_menu_delete_finish = 189,
} UIEventHandlerReferenceFunction;  // int16
typedef int16_t UIEventHandlerReferenceFunction_t;
typedef enum UIReplaceFunction {
    uireplacefunction_null = 0,
    uireplacefunction_widget_s_controller = 1,
    uireplacefunction_build_number = 2,
    uireplacefunction_pid = 3,
} UIReplaceFunction;  // int16
typedef int16_t UIReplaceFunction_t;
typedef enum UIWidgetType {
    uiwidgettype_container = 0,
    uiwidgettype_text_box = 1,
    uiwidgettype_spinner_list = 2,
    uiwidgettype_column_list = 3,
    uiwidgettype_game_model_not_implemented = 4,
    uiwidgettype_movie_not_implemented = 5,
    uiwidgettype_custom_not_implemented = 6,
} UIWidgetType;  // int16
typedef int16_t UIWidgetType_t;
typedef enum UIControllerIndex {
    uicontrollerindex_player_1 = 0,
    uicontrollerindex_player_2 = 1,
    uicontrollerindex_player_3 = 2,
    uicontrollerindex_player_4 = 3,
    uicontrollerindex_any_player = 4,
} UIControllerIndex;  // int16
typedef int16_t UIControllerIndex_t;
typedef enum UIJustification {
    uijustification_left_justify = 0,
    uijustification_right_justify = 1,
    uijustification_center_justify = 2,
} UIJustification;  // int16
typedef int16_t UIJustification_t;
typedef uint32_t EventHandlerReferencesFlags;  // bitfield: close_current_widget, close_other_widget, close_all_widgets, open_widget, reload_self, reload_other_widget, give_focus_to_widget, run_function, replace_self_w_widget, go_back_to_previous_widget, run_scenario_script, try_to_branch_on_failure
typedef uint32_t ConditionalWidgetReferenceFlags;  // bitfield: load_if_event_handler_function_fails
typedef uint32_t ChildWidgetReferenceFlags;  // bitfield: use_custom_controller_index
typedef uint32_t UIWidgetDefinitionFlags;  // bitfield: pass_unhandled_events_to_focused_child, pause_game_time, flash_background_bitmap, dpad_up_down_tabs_thru_children, dpad_left_right_tabs_thru_children, dpad_up_down_tabs_thru_list_items, dpad_left_right_tabs_thru_list_items, dont_focus_a_specific_child_widget, pass_unhandled_events_to_all_children, render_regardless_of_controller_index, pass_handled_events_to_all_children, return_to_main_menu_if_no_history, always_use_tag_controller_index, always_use_nifty_render_fx, don_t_push_history, force_handle_mouse
typedef uint32_t UIWidgetDefinitionFlags1;  // bitfield: editable, password, flashing, don_t_do_that_weird_focus_test
typedef uint32_t UIWidgetDefinitionFlags2;  // bitfield: list_items_generated_in_code, list_items_from_string_list_tag, list_items_only_one_tooltip, list_single_preview_no_scroll
typedef struct GameDataInputReference {
    UIGameDataInputReferenceFunction_t function;
    uint8_t _pad_2[2];
    uint8_t _pad_4[32];
} GameDataInputReference;  // size 0x24
typedef struct EventHandlerReference {
    EventHandlerReferencesFlags flags;
    UIEventType_t event_type;
    UIEventHandlerReferenceFunction_t function;
    TagDependency widget_tag;  // ui_widget_definition
    TagDependency sound_effect;  // sound
    TagString script;
} EventHandlerReference;  // size 0x48
typedef struct SearchAndReplaceReference {
    TagString search_string;
    UIReplaceFunction_t replace_function;
} SearchAndReplaceReference;  // size 0x22
typedef struct ConditionalWidgetReference {
    TagDependency widget_tag;  // ui_widget_definition
    TagString name;
    ConditionalWidgetReferenceFlags flags;
    uint16_t custom_controller_index;
    uint8_t _pad_36[26];
} ConditionalWidgetReference;  // size 0x50
typedef struct ChildWidgetReference {
    TagDependency widget_tag;  // ui_widget_definition
    TagString name;
    ChildWidgetReferenceFlags flags;
    uint16_t custom_controller_index;
    int16_t vertical_offset;
    int16_t horizontal_offset;
    uint8_t _pad_3a[22];
} ChildWidgetReference;  // size 0x50
typedef struct UIWidgetDefinition {
    UIWidgetType_t widget_type;
    UIControllerIndex_t controller_index;
    TagString name;
    Rectangle2D bounds;
    UIWidgetDefinitionFlags flags;
    int32_t milliseconds_to_auto_close;
    int32_t milliseconds_auto_close_fade_time;
    TagDependency background_bitmap;  // bitmap
    TagReflexive game_data_inputs;  // GameDataInputReference
    TagReflexive event_handlers;  // EventHandlerReference
    TagReflexive search_and_replace_functions;  // SearchAndReplaceReference
    uint8_t _pad_6c[128];
    TagDependency text_label_unicode_strings_list;  // unicode_string_list
    TagDependency text_font;  // font
    ColorARGB text_color;
    UIJustification_t justification;
    UIWidgetDefinitionFlags1 flags_1;
    uint8_t _pad_122[12];
    uint16_t string_list_index;
    int16_t horiz_offset;
    int16_t vert_offset;
    uint8_t _pad_134[26];
    uint8_t _pad_14e[2];
    UIWidgetDefinitionFlags2 flags_2;
    TagDependency list_header_bitmap;  // bitmap
    TagDependency list_footer_bitmap;  // bitmap
    Rectangle2D header_bounds;
    Rectangle2D footer_bounds;
    uint8_t _pad_184[32];
    TagDependency extended_description_widget;  // ui_widget_definition
    uint8_t _pad_1b4[32];
    uint8_t _pad_1d4[256];
    TagReflexive conditional_widgets;  // ConditionalWidgetReference
    uint8_t _pad_2e0[128];
    uint8_t _pad_360[128];
    TagReflexive child_widgets;  // ChildWidgetReference
} UIWidgetDefinition;  // size 0x3ec
typedef struct UnicodeStringListString {
    TagDataOffset string;
} UnicodeStringListString;  // size 0x14
typedef struct UnicodeStringList {
    TagReflexive strings;  // UnicodeStringListString
} UnicodeStringList;  // size 0xc
typedef enum UnitHUDInterfaceMeterPanelType {
    unithudinterfacemeterpaneltype_integrated_light = 0,
} UnitHUDInterfaceMeterPanelType;  // int16
typedef int16_t UnitHUDInterfaceMeterPanelType_t;
typedef enum UnitHUDInterfaceOverlayPanelType {
    unithudinterfaceoverlaypaneltype_team_icon = 0,
} UnitHUDInterfaceOverlayPanelType;  // int16
typedef int16_t UnitHUDInterfaceOverlayPanelType_t;
typedef uint16_t UnitHUDInterfaceAuxiliaryOverlayFlags;  // bitfield: use_team_color
typedef uint32_t UnitHUDInterfaceHUDSoundLatchedTo;  // bitfield: shield_recharging, shield_damaged, shield_low, shield_empty, health_low, health_empty, health_minor_damage, health_major_damage
typedef uint32_t UnitHUDInterfaceAuxiliaryPanelMeterMoreFlags;  // bitfield: show_only_when_active, flash_once_if_activated_while_disabled
typedef struct UnitHUDInterfaceAuxiliaryOverlay {
    Point2DInt anchor_offset;
    float width_scale;
    float height_scale;
    HUDInterfaceScalingFlags scaling_flags;
    uint8_t _pad_e[2];
    uint8_t _pad_10[20];
    TagDependency interface_bitmap;  // bitmap
    ColorARGBInt default_color;
    ColorARGBInt flashing_color;
    float flash_period;
    float flash_delay;
    int16_t number_of_flashes;
    HUDInterfaceFlashFlags flash_flags;
    float flash_length;
    ColorARGBInt disabled_color;
    uint8_t _pad_50[4];
    uint16_t sequence_index;
    uint8_t _pad_56[2];
    TagReflexive multitexture_overlays;  // HUDInterfaceMultitextureOverlay
    uint8_t _pad_64[4];
    UnitHUDInterfaceOverlayPanelType_t type;
    UnitHUDInterfaceAuxiliaryOverlayFlags flags;
    uint8_t _pad_6c[24];
} UnitHUDInterfaceAuxiliaryOverlay;  // size 0x84
typedef struct UnitHUDInterfaceHUDSound {
    TagDependency sound;  // sound,sound_looping
    UnitHUDInterfaceHUDSoundLatchedTo latched_to;
    float scale;
    uint8_t _pad_18[32];
} UnitHUDInterfaceHUDSound;  // size 0x38
typedef struct UnitHUDInterfaceAuxiliaryPanel {
    UnitHUDInterfaceMeterPanelType_t type;
    uint8_t _pad_2[2];
    uint8_t _pad_4[16];
    Point2DInt background_anchor_offset;
    float background_width_scale;
    float background_height_scale;
    HUDInterfaceScalingFlags background_scaling_flags;
    uint8_t _pad_22[2];
    uint8_t _pad_24[20];
    TagDependency background_interface_bitmap;  // bitmap
    ColorARGBInt background_default_color;
    ColorARGBInt background_flashing_color;
    float background_flash_period;
    float background_flash_delay;
    int16_t background_number_of_flashes;
    HUDInterfaceFlashFlags background_flash_flags;
    float background_flash_length;
    ColorARGBInt background_disabled_color;
    uint8_t _pad_64[4];
    uint16_t background_sequence_index;
    uint8_t _pad_6a[2];
    TagReflexive background_multitexture_overlays;  // HUDInterfaceMultitextureOverlay
    uint8_t _pad_78[4];
    Point2DInt meter_anchor_offset;
    float meter_width_scale;
    float meter_height_scale;
    HUDInterfaceScalingFlags meter_scaling_flags;
    uint8_t _pad_8a[2];
    uint8_t _pad_8c[20];
    TagDependency meter_meter_bitmap;  // bitmap
    ColorARGBInt meter_color_at_meter_minimum;
    ColorARGBInt meter_color_at_meter_maximum;
    ColorARGBInt meter_flash_color;
    ColorARGBInt meter_empty_color;
    HUDInterfaceMeterFlags meter_flags;
    int8_t meter_minimum_meter_value;
    uint16_t meter_sequence_index;
    int8_t meter_alpha_multiplier;
    int8_t meter_alpha_bias;
    int16_t meter_value_scale;
    float meter_opacity;
    float meter_translucency;
    ColorARGBInt meter_disabled_color;
    float meter_min_alpha;
    uint8_t _pad_d8[12];
    float meter_minimum_fraction_cutoff;
    UnitHUDInterfaceAuxiliaryPanelMeterMoreFlags meter_more_flags;
    uint8_t _pad_ec[24];
    uint8_t _pad_104[64];
} UnitHUDInterfaceAuxiliaryPanel;  // size 0x144
typedef struct UnitHUDInterface {
    HUDInterfaceAnchor_t anchor;
    HUDInterfaceCanvasSize_t canvas_size;
    uint8_t _pad_4[32];
    Point2DInt hud_background_anchor_offset;
    float hud_background_width_scale;
    float hud_background_height_scale;
    HUDInterfaceScalingFlags hud_background_scaling_flags;
    uint8_t _pad_32[2];
    uint8_t _pad_34[20];
    TagDependency hud_background_interface_bitmap;  // bitmap
    ColorARGBInt hud_background_default_color;
    ColorARGBInt hud_background_flashing_color;
    float hud_background_flash_period;
    float hud_background_flash_delay;
    int16_t hud_background_number_of_flashes;
    HUDInterfaceFlashFlags hud_background_flash_flags;
    float hud_background_flash_length;
    ColorARGBInt hud_background_disabled_color;
    uint8_t _pad_74[4];
    uint16_t hud_background_sequence_index;
    uint8_t _pad_7a[2];
    TagReflexive hud_background_multitexture_overlays;  // HUDInterfaceMultitextureOverlay
    uint8_t _pad_88[4];
    Point2DInt shield_panel_background_anchor_offset;
    float shield_panel_background_width_scale;
    float shield_panel_background_height_scale;
    HUDInterfaceScalingFlags shield_panel_background_scaling_flags;
    uint8_t _pad_9a[2];
    uint8_t _pad_9c[20];
    TagDependency shield_panel_background_interface_bitmap;  // bitmap
    ColorARGBInt shield_panel_background_default_color;
    ColorARGBInt shield_panel_background_flashing_color;
    float shield_panel_background_flash_period;
    float shield_panel_background_flash_delay;
    int16_t shield_panel_background_number_of_flashes;
    HUDInterfaceFlashFlags shield_panel_background_flash_flags;
    float shield_panel_background_flash_length;
    ColorARGBInt shield_panel_background_disabled_color;
    uint8_t _pad_dc[4];
    uint16_t shield_panel_background_sequence_index;
    uint8_t _pad_e2[2];
    TagReflexive shield_panel_background_multitexture_overlays;  // HUDInterfaceMultitextureOverlay
    uint8_t _pad_f0[4];
    Point2DInt shield_panel_meter_anchor_offset;
    float shield_panel_meter_width_scale;
    float shield_panel_meter_height_scale;
    HUDInterfaceScalingFlags shield_panel_meter_scaling_flags;
    uint8_t _pad_102[2];
    uint8_t _pad_104[20];
    TagDependency shield_panel_meter_meter_bitmap;  // bitmap
    ColorARGBInt shield_panel_meter_color_at_meter_minimum;
    ColorARGBInt shield_panel_meter_color_at_meter_maximum;
    ColorARGBInt shield_panel_meter_flash_color;
    ColorARGBInt shield_panel_meter_empty_color;
    HUDInterfaceMeterFlags shield_panel_meter_flags;
    int8_t shield_panel_meter_minimum_meter_value;
    uint16_t shield_panel_meter_sequence_index;
    int8_t shield_panel_meter_alpha_multiplier;
    int8_t shield_panel_meter_alpha_bias;
    int16_t shield_panel_meter_value_scale;
    float shield_panel_meter_opacity;
    float shield_panel_meter_translucency;
    ColorARGBInt shield_panel_meter_disabled_color;
    float shield_panel_meter_min_alpha;
    uint8_t _pad_150[12];
    ColorARGBInt shield_panel_meter_overcharge_minimum_color;
    ColorARGBInt shield_panel_meter_overcharge_maximum_color;
    ColorARGBInt shield_panel_meter_overcharge_flash_color;
    ColorARGBInt shield_panel_meter_overcharge_empty_color;
    uint8_t _pad_16c[16];
    Point2DInt health_panel_background_anchor_offset;
    float health_panel_background_width_scale;
    float health_panel_background_height_scale;
    HUDInterfaceScalingFlags health_panel_background_scaling_flags;
    uint8_t _pad_18a[2];
    uint8_t _pad_18c[20];
    TagDependency health_panel_background_interface_bitmap;  // bitmap
    ColorARGBInt health_panel_background_default_color;
    ColorARGBInt health_panel_background_flashing_color;
    float health_panel_background_flash_period;
    float health_panel_background_flash_delay;
    int16_t health_panel_background_number_of_flashes;
    HUDInterfaceFlashFlags health_panel_background_flash_flags;
    float health_panel_background_flash_length;
    ColorARGBInt health_panel_background_disabled_color;
    uint8_t _pad_1cc[4];
    uint16_t health_panel_background_sequence_index;
    uint8_t _pad_1d2[2];
    TagReflexive health_panel_background_multitexture_overlays;  // HUDInterfaceMultitextureOverlay
    uint8_t _pad_1e0[4];
    Point2DInt health_panel_meter_anchor_offset;
    float health_panel_meter_width_scale;
    float health_panel_meter_height_scale;
    HUDInterfaceScalingFlags health_panel_meter_scaling_flags;
    uint8_t _pad_1f2[2];
    uint8_t _pad_1f4[20];
    TagDependency health_panel_meter_meter_bitmap;  // bitmap
    ColorARGBInt health_panel_meter_color_at_meter_minimum;
    ColorARGBInt health_panel_meter_color_at_meter_maximum;
    ColorARGBInt health_panel_meter_flash_color;
    ColorARGBInt health_panel_meter_empty_color;
    HUDInterfaceMeterFlags health_panel_meter_flags;
    int8_t health_panel_meter_minimum_meter_value;
    uint16_t health_panel_meter_sequence_index;
    int8_t health_panel_meter_alpha_multiplier;
    int8_t health_panel_meter_alpha_bias;
    int16_t health_panel_meter_value_scale;
    float health_panel_meter_opacity;
    float health_panel_meter_translucency;
    ColorARGBInt health_panel_meter_disabled_color;
    float health_panel_meter_min_alpha;
    uint8_t _pad_240[12];
    ColorARGBInt health_panel_meter_medium_health_left_color;
    float health_panel_meter_max_color_health_fraction_cutoff;
    float health_panel_meter_min_color_health_fraction_cutoff;
    uint8_t _pad_258[20];
    Point2DInt motion_sensor_background_anchor_offset;
    float motion_sensor_background_width_scale;
    float motion_sensor_background_height_scale;
    HUDInterfaceScalingFlags motion_sensor_background_scaling_flags;
    uint8_t _pad_27a[2];
    uint8_t _pad_27c[20];
    TagDependency motion_sensor_background_interface_bitmap;  // bitmap
    ColorARGBInt motion_sensor_background_default_color;
    ColorARGBInt motion_sensor_background_flashing_color;
    float motion_sensor_background_flash_period;
    float motion_sensor_background_flash_delay;
    int16_t motion_sensor_background_number_of_flashes;
    HUDInterfaceFlashFlags motion_sensor_background_flash_flags;
    float motion_sensor_background_flash_length;
    ColorARGBInt motion_sensor_background_disabled_color;
    uint8_t _pad_2bc[4];
    uint16_t motion_sensor_background_sequence_index;
    uint8_t _pad_2c2[2];
    TagReflexive motion_sensor_background_multitexture_overlays;  // HUDInterfaceMultitextureOverlay
    uint8_t _pad_2d0[4];
    Point2DInt motion_sensor_foreground_anchor_offset;
    float motion_sensor_foreground_width_scale;
    float motion_sensor_foreground_height_scale;
    HUDInterfaceScalingFlags motion_sensor_foreground_scaling_flags;
    uint8_t _pad_2e2[2];
    uint8_t _pad_2e4[20];
    TagDependency motion_sensor_foreground_interface_bitmap;  // bitmap
    ColorARGBInt motion_sensor_foreground_default_color;
    ColorARGBInt motion_sensor_foreground_flashing_color;
    float motion_sensor_foreground_flash_period;
    float motion_sensor_foreground_flash_delay;
    int16_t motion_sensor_foreground_number_of_flashes;
    HUDInterfaceFlashFlags motion_sensor_foreground_flash_flags;
    float motion_sensor_foreground_flash_length;
    ColorARGBInt motion_sensor_foreground_disabled_color;
    uint8_t _pad_324[4];
    uint16_t motion_sensor_foreground_sequence_index;
    uint8_t _pad_32a[2];
    TagReflexive motion_sensor_foreground_multitexture_overlays;  // HUDInterfaceMultitextureOverlay
    uint8_t _pad_338[4];
    uint8_t _pad_33c[32];
    Point2DInt motion_sensor_center_anchor_offset;
    float motion_sensor_center_width_scale;
    float motion_sensor_center_height_scale;
    HUDInterfaceScalingFlags motion_sensor_center_scaling_flags;
    uint8_t _pad_36a[2];
    uint8_t _pad_36c[20];
    HUDInterfaceAnchor_t auxiliary_overlay_anchor;
    uint8_t _pad_382[2];
    uint8_t _pad_384[32];
    TagReflexive overlays;  // UnitHUDInterfaceAuxiliaryOverlay
    uint8_t _pad_3b0[16];
    TagReflexive sounds;  // UnitHUDInterfaceHUDSound
    TagReflexive meters;  // UnitHUDInterfaceAuxiliaryPanel
    uint8_t _pad_3d8[356];
    uint8_t _pad_53c[48];
} UnitHUDInterface;  // size 0x56c
typedef enum VehicleType {
    vehicletype_human_tank = 0,
    vehicletype_human_jeep = 1,
    vehicletype_human_boat = 2,
    vehicletype_human_plane = 3,
    vehicletype_alien_scout = 4,
    vehicletype_alien_fighter = 5,
    vehicletype_turret = 6,
} VehicleType;  // int16
typedef int16_t VehicleType_t;
typedef enum VehicleFunctionIn {
    vehiclefunctionin_none = 0,
    vehiclefunctionin_speed_absolute = 1,
    vehiclefunctionin_speed_forward = 2,
    vehiclefunctionin_speed_backward = 3,
    vehiclefunctionin_slide_absolute = 4,
    vehiclefunctionin_slide_left = 5,
    vehiclefunctionin_slide_right = 6,
    vehiclefunctionin_speed_slide_maximum = 7,
    vehiclefunctionin_turn_absolute = 8,
    vehiclefunctionin_turn_left = 9,
    vehiclefunctionin_turn_right = 10,
    vehiclefunctionin_crouch = 11,
    vehiclefunctionin_jump = 12,
    vehiclefunctionin_walk = 13,
    vehiclefunctionin_velocity_air = 14,
    vehiclefunctionin_velocity_water = 15,
    vehiclefunctionin_velocity_ground = 16,
    vehiclefunctionin_velocity_forward = 17,
    vehiclefunctionin_velocity_left = 18,
    vehiclefunctionin_velocity_up = 19,
    vehiclefunctionin_left_tread_position = 20,
    vehiclefunctionin_right_tread_position = 21,
    vehiclefunctionin_left_tread_velocity = 22,
    vehiclefunctionin_right_tread_velocity = 23,
    vehiclefunctionin_front_left_tire_position = 24,
    vehiclefunctionin_front_right_tire_position = 25,
    vehiclefunctionin_back_left_tire_position = 26,
    vehiclefunctionin_back_right_tire_position = 27,
    vehiclefunctionin_front_left_tire_velocity = 28,
    vehiclefunctionin_front_right_tire_velocity = 29,
    vehiclefunctionin_back_left_tire_velocity = 30,
    vehiclefunctionin_back_right_tire_velocity = 31,
    vehiclefunctionin_wingtip_contrail = 32,
    vehiclefunctionin_hover = 33,
    vehiclefunctionin_thrust = 34,
    vehiclefunctionin_engine_hack = 35,
    vehiclefunctionin_wingtip_contrail_new = 36,
} VehicleFunctionIn;  // int16
typedef int16_t VehicleFunctionIn_t;
typedef uint32_t VehicleFlags;  // bitfield: speed_wakes_physics, turn_wakes_physics, driver_power_wakes_physics, gunner_power_wakes_physics, control_opposite_speed_sets_brake, slide_wakes_physics, kills_riders_at_terminal_velocity, causes_collision_damage, ai_weapon_cannot_rotate, ai_does_not_require_driver, ai_unused, ai_driver_enable, ai_driver_flying, ai_driver_can_sidestep, ai_driver_hovering, vehicle_steers_directly, unused, has_ebrake, noncombat_vehicle, no_friction_with_driver, can_trigger_automatic_opening_doors, autoaim_when_teamless
typedef struct Vehicle {
    Unit base;  // inherits
    VehicleFlags vehicle_flags;
    VehicleType_t vehicle_type;
    uint8_t _pad_2f6[2];
    float maximum_forward_speed;
    float maximum_reverse_speed;
    float speed_acceleration;
    float speed_deceleration;
    float maximum_left_turn;
    float maximum_right_turn;
    float wheel_circumference;
    float turn_rate;
    float blur_speed;
    VehicleFunctionIn_t vehicle_a_in;
    VehicleFunctionIn_t vehicle_b_in;
    VehicleFunctionIn_t vehicle_c_in;
    VehicleFunctionIn_t vehicle_d_in;
    uint8_t _pad_324[12];
    float maximum_left_slide;
    float maximum_right_slide;
    float slide_acceleration;
    float slide_deceleration;
    float minimum_flipping_angular_velocity;
    float maximum_flipping_angular_velocity;
    uint8_t _pad_348[24];
    float fixed_gun_yaw;
    float fixed_gun_pitch;
    uint8_t _pad_368[24];
    float ai_sideslip_distance;
    float ai_destination_radius;
    float ai_avoidance_distance;
    float ai_pathfinding_radius;
    float ai_charge_repeat_timeout;
    float ai_strafing_abort_range;
    float ai_oversteering_bounds[2];
    float ai_steering_maximum;
    float ai_throttle_maximum;
    float ai_move_position_time;
    uint8_t _pad_3ac[4];
    TagDependency suspension_sound;  // sound
    TagDependency crash_sound;  // sound
    TagDependency material_effects;  // material_effects
    TagDependency effect;  // effect
} Vehicle;  // size 0x3f0
typedef enum VirtualKeyboardKeyboardKey {
    virtualkeyboardkeyboardkey_1 = 0,
    virtualkeyboardkeyboardkey_2 = 1,
    virtualkeyboardkeyboardkey_3 = 2,
    virtualkeyboardkeyboardkey_4 = 3,
    virtualkeyboardkeyboardkey_5 = 4,
    virtualkeyboardkeyboardkey_6 = 5,
    virtualkeyboardkeyboardkey_7 = 6,
    virtualkeyboardkeyboardkey_8 = 7,
    virtualkeyboardkeyboardkey_9 = 8,
    virtualkeyboardkeyboardkey_0 = 9,
    virtualkeyboardkeyboardkey_a = 10,
    virtualkeyboardkeyboardkey_b = 11,
    virtualkeyboardkeyboardkey_c = 12,
    virtualkeyboardkeyboardkey_d = 13,
    virtualkeyboardkeyboardkey_e = 14,
    virtualkeyboardkeyboardkey_f = 15,
    virtualkeyboardkeyboardkey_g = 16,
    virtualkeyboardkeyboardkey_h = 17,
    virtualkeyboardkeyboardkey_i = 18,
    virtualkeyboardkeyboardkey_j = 19,
    virtualkeyboardkeyboardkey_k = 20,
    virtualkeyboardkeyboardkey_l = 21,
    virtualkeyboardkeyboardkey_m = 22,
    virtualkeyboardkeyboardkey_n = 23,
    virtualkeyboardkeyboardkey_o = 24,
    virtualkeyboardkeyboardkey_p = 25,
    virtualkeyboardkeyboardkey_q = 26,
    virtualkeyboardkeyboardkey_r = 27,
    virtualkeyboardkeyboardkey_s = 28,
    virtualkeyboardkeyboardkey_t = 29,
    virtualkeyboardkeyboardkey_u = 30,
    virtualkeyboardkeyboardkey_v = 31,
    virtualkeyboardkeyboardkey_w = 32,
    virtualkeyboardkeyboardkey_x = 33,
    virtualkeyboardkeyboardkey_y = 34,
    virtualkeyboardkeyboardkey_z = 35,
    virtualkeyboardkeyboardkey_done = 36,
    virtualkeyboardkeyboardkey_shift = 37,
    virtualkeyboardkeyboardkey_caps_lock = 38,
    virtualkeyboardkeyboardkey_symbols = 39,
    virtualkeyboardkeyboardkey_backspace = 40,
    virtualkeyboardkeyboardkey_left = 41,
    virtualkeyboardkeyboardkey_right = 42,
    virtualkeyboardkeyboardkey_space = 43,
} VirtualKeyboardKeyboardKey;  // int16
typedef int16_t VirtualKeyboardKeyboardKey_t;
typedef struct VirtualKeyboardVirtualKey {
    VirtualKeyboardKeyboardKey_t keyboard_key;
    int16_t lowercase_character;
    int16_t shift_character;
    int16_t caps_character;
    int16_t symbols_character;
    int16_t shift_caps_character;
    int16_t shift_symbols_character;
    int16_t caps_symbols_character;
    TagDependency unselected_background_bitmap;  // bitmap
    TagDependency selected_background_bitmap;  // bitmap
    TagDependency active_background_bitmap;  // bitmap
    TagDependency sticky_background_bitmap;  // bitmap
} VirtualKeyboardVirtualKey;  // size 0x50
typedef struct VirtualKeyboard {
    TagDependency display_font;  // font
    TagDependency background_bitmap;  // bitmap
    TagDependency special_key_labels_string_list;  // unicode_string_list
    TagReflexive virtual_keys;  // VirtualKeyboardVirtualKey
} VirtualKeyboard;  // size 0x3c
typedef enum WeaponSecondaryTriggerMode {
    weaponsecondarytriggermode_normal = 0,
    weaponsecondarytriggermode_slaved_to_primary = 1,
    weaponsecondarytriggermode_inhibits_primary = 2,
    weaponsecondarytriggermode_loads_alterate_ammunition = 3,
    weaponsecondarytriggermode_loads_multiple_primary_ammunition = 4,
} WeaponSecondaryTriggerMode;  // int16
typedef int16_t WeaponSecondaryTriggerMode_t;
typedef enum WeaponFunctionIn {
    weaponfunctionin_none = 0,
    weaponfunctionin_heat = 1,
    weaponfunctionin_primary_ammunition = 2,
    weaponfunctionin_secondary_ammunition = 3,
    weaponfunctionin_primary_rate_of_fire = 4,
    weaponfunctionin_secondary_rate_of_fire = 5,
    weaponfunctionin_ready = 6,
    weaponfunctionin_primary_ejection_port = 7,
    weaponfunctionin_secondary_ejection_port = 8,
    weaponfunctionin_overheated = 9,
    weaponfunctionin_primary_charged = 10,
    weaponfunctionin_secondary_charged = 11,
    weaponfunctionin_illumination = 12,
    weaponfunctionin_age = 13,
    weaponfunctionin_integrated_light = 14,
    weaponfunctionin_primary_firing = 15,
    weaponfunctionin_secondary_firing = 16,
    weaponfunctionin_primary_firing_on = 17,
    weaponfunctionin_secondary_firing_on = 18,
} WeaponFunctionIn;  // int16
typedef int16_t WeaponFunctionIn_t;
typedef enum WeaponMovementPenalized {
    weaponmovementpenalized_always = 0,
    weaponmovementpenalized_when_zoomed = 1,
    weaponmovementpenalized_when_zoomed_or_reloading = 2,
} WeaponMovementPenalized;  // int16
typedef int16_t WeaponMovementPenalized_t;
typedef enum WeaponType {
    weapontype_undefined = 0,
    weapontype_shotgun = 1,
    weapontype_needler = 2,
    weapontype_plasma_pistol = 3,
    weapontype_plasma_rifle = 4,
    weapontype_rocket_launcher = 5,
} WeaponType;  // int16
typedef int16_t WeaponType_t;
typedef enum WeaponPredictionType {
    weaponpredictiontype_none = 0,
    weaponpredictiontype_continuous = 1,
    weaponpredictiontype_instant = 2,
} WeaponPredictionType;  // int16
typedef int16_t WeaponPredictionType_t;
typedef enum WeaponOverchargedAction {
    weaponoverchargedaction_none = 0,
    weaponoverchargedaction_explode = 1,
    weaponoverchargedaction_discharge = 2,
} WeaponOverchargedAction;  // int16
typedef int16_t WeaponOverchargedAction_t;
typedef enum WeaponDistributionFunction {
    weapondistributionfunction_point = 0,
    weapondistributionfunction_horizontal_fan = 1,
} WeaponDistributionFunction;  // int16
typedef int16_t WeaponDistributionFunction_t;
typedef uint32_t WeaponMagazineFlags;  // bitfield: wastes_rounds_when_reloaded, every_round_must_be_chambered
typedef uint32_t WeaponTriggerFlags;  // bitfield: tracks_fired_projectile, random_firing_effects, can_fire_with_partial_ammo, does_not_repeat_automatically, locks_in_on_off_state, projectiles_use_weapon_origin, sticks_when_dropped, ejects_during_chamber, discharging_spews, analog_rate_of_fire, use_error_when_unzoomed, projectile_vector_cannot_be_adjusted, projectiles_have_identical_error, projectile_is_client_side_only, use_original_unit_adjust_projectile_ray
typedef uint32_t WeaponFlags;  // bitfield: vertical_heat_display, mutually_exclusive_triggers, attacks_automatically_on_bump, must_be_readied, doesn_t_count_toward_maximum, aim_assists_only_when_zoomed, prevents_grenade_throwing, must_be_picked_up, holds_triggers_when_dropped, prevents_melee_attack, detonates_when_dropped, cannot_fire_at_maximum_age, secondary_trigger_overrides_grenades, does_not_depower_active_camo_in_multilplayer, enables_integrated_night_vision, ais_use_weapon_melee_damage, prevents_crouching, uses_3rd_person_camera
typedef struct WeaponMagazineObject {
    int16_t rounds;
    uint8_t _pad_2[10];
    TagDependency equipment;  // equipment
} WeaponMagazineObject;  // size 0x1c
typedef struct WeaponMagazine {
    WeaponMagazineFlags flags;
    int16_t rounds_recharged;
    int16_t rounds_total_initial;
    int16_t rounds_reserved_maximum;
    int16_t rounds_loaded_maximum;
    uint8_t _pad_c[8];
    float reload_time;
    int16_t rounds_reloaded;
    uint8_t _pad_1a[2];
    float chamber_time;
    uint8_t _pad_20[8];
    uint8_t _pad_28[16];
    TagDependency reloading_effect;  // sound,effect
    TagDependency chambering_effect;  // sound,effect
    uint8_t _pad_58[12];
    TagReflexive magazine_objects;  // WeaponMagazineObject
} WeaponMagazine;  // size 0x70
typedef struct WeaponTriggerFiringEffect {
    int16_t shot_count_lower_bound;
    int16_t shot_count_upper_bound;
    uint8_t _pad_4[32];
    TagDependency firing_effect;  // sound,effect
    TagDependency misfire_effect;  // sound,effect
    TagDependency empty_effect;  // sound,effect
    TagDependency firing_damage;  // damage_effect
    TagDependency misfire_damage;  // damage_effect
    TagDependency empty_damage;  // damage_effect
} WeaponTriggerFiringEffect;  // size 0x84
typedef struct WeaponTrigger {
    WeaponTriggerFlags flags;
    float maximum_rate_of_fire[2];
    float acceleration_time;
    float deceleration_time;
    float blurred_rate_of_fire;
    uint8_t _pad_18[8];
    uint16_t magazine;
    int16_t rounds_per_shot;
    int16_t minimum_rounds_loaded;
    int16_t projectiles_between_contrails;
    uint8_t _pad_28[4];
    WeaponPredictionType_t prediction_type;
    ObjectNoise_t firing_noise;
    float error[2];
    float error_acceleration_time;
    float error_deceleration_time;
    uint8_t _pad_40[8];
    float charging_time;
    float charged_time;
    WeaponOverchargedAction_t overcharged_action;
    uint8_t _pad_52[2];
    float charged_illumination;
    float spew_time;
    TagDependency charging_effect;  // sound,effect
    WeaponDistributionFunction_t distribution_function;
    int16_t projectiles_per_shot;
    float distribution_angle;
    uint8_t _pad_74[4];
    float minimum_error;
    float error_angle[2];
    Point3D first_person_offset;
    uint8_t _pad_90[4];
    TagDependency projectile;  // projectile
    float ejection_port_recovery_time;
    float illumination_recovery_time;
    uint8_t _pad_ac[12];
    float heat_generated_per_round;
    float age_generated_per_round;
    uint8_t _pad_c0[4];
    float overload_time;
    uint8_t _pad_c8[8];
    uint8_t _pad_d0[32];
    float illumination_recovery_rate;
    float ejection_port_recovery_rate;
    float firing_acceleration_rate;
    float firing_deceleration_rate;
    float error_acceleration_rate;
    float error_deceleration_rate;
    TagReflexive firing_effects;  // WeaponTriggerFiringEffect
} WeaponTrigger;  // size 0x114
typedef struct Weapon {
    Item base;  // inherits
    WeaponFlags weapon_flags;
    TagString label;
    WeaponSecondaryTriggerMode_t secondary_trigger_mode;
    int16_t maximum_alternate_shots_loaded;
    WeaponFunctionIn_t weapon_a_in;
    WeaponFunctionIn_t weapon_b_in;
    WeaponFunctionIn_t weapon_c_in;
    WeaponFunctionIn_t weapon_d_in;
    float ready_time;
    TagDependency ready_effect;  // sound,effect
    float heat_recovery_threshold;
    float overheated_threshold;
    float heat_detonation_threshold;
    float heat_detonation_fraction;
    float heat_loss_rate;
    float heat_illumination;
    uint8_t _pad_364[16];
    TagDependency overheated;  // sound,effect
    TagDependency overheat_detonation;  // sound,effect
    TagDependency player_melee_damage;  // damage_effect
    TagDependency player_melee_response;  // damage_effect
    uint8_t _pad_3b4[8];
    TagDependency actor_firing_parameters;  // actor_variant
    float near_reticle_range;
    float far_reticle_range;
    float intersection_reticle_range;
    uint8_t _pad_3d8[2];
    int16_t zoom_levels;
    float zoom_magnification_range[2];
    float autoaim_angle;
    float autoaim_range;
    float magnetism_angle;
    float magnetism_range;
    float deviation_angle;
    uint8_t _pad_3f8[4];
    WeaponMovementPenalized_t movement_penalized;
    uint8_t _pad_3fe[2];
    float forward_movement_penalty;
    float sideways_movement_penalty;
    uint8_t _pad_408[4];
    float minimum_target_range;
    float looking_time_modifier;
    uint8_t _pad_414[4];
    float light_power_on_time;
    float light_power_off_time;
    TagDependency light_power_on_effect;  // sound,effect
    TagDependency light_power_off_effect;  // sound,effect
    float age_heat_recovery_penalty;
    float age_rate_of_fire_penalty;
    float age_misfire_start;
    float age_misfire_chance;
    uint8_t _pad_450[12];
    TagDependency first_person_model;  // model
    TagDependency first_person_animations;  // model_animations
    uint8_t _pad_47c[4];
    TagDependency hud_interface;  // weapon_hud_interface
    TagDependency pickup_sound;  // sound
    TagDependency zoom_in_sound;  // sound
    TagDependency zoom_out_sound;  // sound
    uint8_t _pad_4c0[12];
    float active_camo_ding;
    float active_camo_regrowth_rate;
    uint8_t _pad_4d4[12];
    uint8_t _pad_4e0[2];
    WeaponType_t weapon_type;
    TagReflexive more_predicted_resources;  // PredictedResource
    TagReflexive magazines;  // WeaponMagazine
    TagReflexive triggers;  // WeaponTrigger
} Weapon;  // size 0x508
typedef enum WeaponHUDInterfaceStateAttachedTo {
    weaponhudinterfacestateattachedto_total_ammo = 0,
    weaponhudinterfacestateattachedto_loaded_ammo = 1,
    weaponhudinterfacestateattachedto_heat = 2,
    weaponhudinterfacestateattachedto_age = 3,
    weaponhudinterfacestateattachedto_secondary_weapon_total_ammo = 4,
    weaponhudinterfacestateattachedto_secondary_weapon_loaded_ammo = 5,
    weaponhudinterfacestateattachedto_distance_to_target = 6,
    weaponhudinterfacestateattachedto_elevation_to_target = 7,
} WeaponHUDInterfaceStateAttachedTo;  // int16
typedef int16_t WeaponHUDInterfaceStateAttachedTo_t;
typedef enum WeaponHUDInterfaceViewType {
    weaponhudinterfaceviewtype_any = 0,
    weaponhudinterfaceviewtype_fullscreen = 1,
    weaponhudinterfaceviewtype_splitscreen = 2,
} WeaponHUDInterfaceViewType;  // int16
typedef int16_t WeaponHUDInterfaceViewType_t;
typedef enum WeaponHUDInterfaceCrosshairType {
    weaponhudinterfacecrosshairtype_aim = 0,
    weaponhudinterfacecrosshairtype_zoom_overlay = 1,
    weaponhudinterfacecrosshairtype_charge = 2,
    weaponhudinterfacecrosshairtype_should_reload = 3,
    weaponhudinterfacecrosshairtype_flash_heat = 4,
    weaponhudinterfacecrosshairtype_flash_total_ammo = 5,
    weaponhudinterfacecrosshairtype_flash_battery = 6,
    weaponhudinterfacecrosshairtype_reload_overheat = 7,
    weaponhudinterfacecrosshairtype_flash_when_firing_and_no_ammo = 8,
    weaponhudinterfacecrosshairtype_flash_when_throwing_and_no_grenade = 9,
    weaponhudinterfacecrosshairtype_low_ammo_and_none_left_to_reload = 10,
    weaponhudinterfacecrosshairtype_should_reload_secondary_trigger = 11,
    weaponhudinterfacecrosshairtype_flash_secondary_total_ammo = 12,
    weaponhudinterfacecrosshairtype_flash_secondary_reload = 13,
    weaponhudinterfacecrosshairtype_flash_when_firing_secondary_trigger_with_no_ammo = 14,
    weaponhudinterfacecrosshairtype_low_secondary_ammo_and_none_left_to_reload = 15,
    weaponhudinterfacecrosshairtype_primary_trigger_ready = 16,
    weaponhudinterfacecrosshairtype_secondary_trigger_ready = 17,
    weaponhudinterfacecrosshairtype_flash_when_firing_with_depleted_battery = 18,
} WeaponHUDInterfaceCrosshairType;  // int16
typedef int16_t WeaponHUDInterfaceCrosshairType_t;
typedef uint32_t WeaponHUDInterfaceCrosshairTypeFlags;  // bitfield: aim, zoom_overlay, charge, should_reload, flash_heat, flash_total_ammo, flash_battery, reload_overheat, flash_when_firing_and_no_ammo, flash_when_throwing_and_no_grenade, low_ammo_and_none_left_to_reload, should_reload_secondary_trigger, flash_secondary_total_ammo, flash_secondary_reload, flash_when_firing_secondary_trigger_with_no_ammo, low_secondary_ammo_and_none_left_to_reload, primary_trigger_ready, secondary_trigger_ready, flash_when_firing_with_depleted_battery
typedef uint16_t WeaponHUDInterfaceNumberWeaponSpecificFlags;  // bitfield: divide_number_by_clip_size
typedef uint32_t WeaponHUDInterfaceCrosshairOverlayFlags;  // bitfield: flashes_when_active, not_a_sprite, show_only_when_zoomed, show_sniper_data, hide_area_outside_reticle, one_zoom_level, don_t_show_when_zoomed
typedef uint16_t WeaponHUDInterfaceOverlayType;  // bitfield: show_on_flashing, show_on_empty, show_on_reload_overheating, show_on_default, show_always
typedef uint16_t WeaponHUDInterfaceScreenEffectDefinitionMaskFlags;  // bitfield: only_when_zoomed
typedef uint16_t WeaponHUDInterfaceScreenEffectDefinitionNightVisionFlags;  // bitfield: only_when_zoomed, connect_to_flashlight, masked
typedef uint16_t WeaponHUDInterfaceScreenEffectDefinitionDesaturationFlags;  // bitfield: only_when_zoomed, connect_to_flashlight, additive, masked
typedef uint16_t WeaponHUDInterfaceFlags;  // bitfield: use_parent_hud_flashing_parameters
typedef struct WeaponHUDInterfaceStaticElement {
    WeaponHUDInterfaceStateAttachedTo_t state_attached_to;
    uint8_t _pad_2[2];
    WeaponHUDInterfaceViewType_t allowed_view_type;
    HUDInterfaceChildAnchor_t anchor;
    uint8_t _pad_8[28];
    Point2DInt anchor_offset;
    float width_scale;
    float height_scale;
    HUDInterfaceScalingFlags scaling_flags;
    uint8_t _pad_32[2];
    uint8_t _pad_34[20];
    TagDependency interface_bitmap;  // bitmap
    ColorARGBInt default_color;
    ColorARGBInt flashing_color;
    float flash_period;
    float flash_delay;
    int16_t number_of_flashes;
    HUDInterfaceFlashFlags flash_flags;
    float flash_length;
    ColorARGBInt disabled_color;
    uint8_t _pad_74[4];
    uint16_t sequence_index;
    uint8_t _pad_7a[2];
    TagReflexive multitexture_overlays;  // HUDInterfaceMultitextureOverlay
    uint8_t _pad_88[4];
    uint8_t _pad_8c[40];
} WeaponHUDInterfaceStaticElement;  // size 0xb4
typedef struct WeaponHUDInterfaceMeter {
    WeaponHUDInterfaceStateAttachedTo_t state_attached_to;
    uint8_t _pad_2[2];
    WeaponHUDInterfaceViewType_t allowed_view_type;
    HUDInterfaceChildAnchor_t anchor;
    uint8_t _pad_8[28];
    Point2DInt anchor_offset;
    float width_scale;
    float height_scale;
    HUDInterfaceScalingFlags scaling_flags;
    uint8_t _pad_32[2];
    uint8_t _pad_34[20];
    TagDependency meter_bitmap;  // bitmap
    ColorARGBInt color_at_meter_minimum;
    ColorARGBInt color_at_meter_maximum;
    ColorARGBInt flash_color;
    ColorARGBInt empty_color;
    HUDInterfaceMeterFlags flags;
    int8_t minimum_meter_value;
    uint16_t sequence_index;
    int8_t alpha_multiplier;
    int8_t alpha_bias;
    int16_t value_scale;
    float opacity;
    float translucency;
    ColorARGBInt disabled_color;
    float min_alpha;
    uint8_t _pad_80[12];
    uint8_t _pad_8c[40];
} WeaponHUDInterfaceMeter;  // size 0xb4
typedef struct WeaponHUDInterfaceNumber {
    WeaponHUDInterfaceStateAttachedTo_t state_attached_to;
    uint8_t _pad_2[2];
    WeaponHUDInterfaceViewType_t allowed_view_type;
    HUDInterfaceChildAnchor_t anchor;
    uint8_t _pad_8[28];
    Point2DInt anchor_offset;
    float width_scale;
    float height_scale;
    HUDInterfaceScalingFlags scaling_flags;
    uint8_t _pad_32[2];
    uint8_t _pad_34[20];
    ColorARGBInt default_color;
    ColorARGBInt flashing_color;
    float flash_period;
    float flash_delay;
    int16_t number_of_flashes;
    HUDInterfaceFlashFlags flash_flags;
    float flash_length;
    ColorARGBInt disabled_color;
    uint8_t _pad_64[4];
    int8_t maximum_number_of_digits;
    HUDInterfaceNumberFlags flags;
    int8_t number_of_fractional_digits;
    uint8_t _pad_6b[1];
    uint8_t _pad_6c[12];
    WeaponHUDInterfaceNumberWeaponSpecificFlags weapon_specific_flags;
    uint8_t _pad_7a[2];
    uint8_t _pad_7c[36];
} WeaponHUDInterfaceNumber;  // size 0xa0
typedef struct WeaponHUDInterfaceCrosshairOverlay {
    Point2DInt anchor_offset;
    float width_scale;
    float height_scale;
    HUDInterfaceScalingFlags scaling_flags;
    uint8_t _pad_e[2];
    uint8_t _pad_10[20];
    ColorARGBInt default_color;
    ColorARGBInt flashing_color;
    float flash_period;
    float flash_delay;
    int16_t number_of_flashes;
    HUDInterfaceFlashFlags flash_flags;
    float flash_length;
    ColorARGBInt disabled_color;
    uint8_t _pad_40[4];
    int16_t frame_rate;
    uint16_t sequence_index;
    WeaponHUDInterfaceCrosshairOverlayFlags flags;
    uint8_t _pad_4c[32];
} WeaponHUDInterfaceCrosshairOverlay;  // size 0x6c
typedef struct WeaponHUDInterfaceCrosshair {
    WeaponHUDInterfaceCrosshairType_t crosshair_type;
    uint8_t _pad_2[2];
    WeaponHUDInterfaceViewType_t allowed_view_type;
    uint8_t _pad_6[2];
    uint8_t _pad_8[28];
    TagDependency crosshair_bitmap;  // bitmap
    TagReflexive crosshair_overlays;  // WeaponHUDInterfaceCrosshairOverlay
    uint8_t _pad_40[40];
} WeaponHUDInterfaceCrosshair;  // size 0x68
typedef struct WeaponHUDInterfaceOverlay {
    Point2DInt anchor_offset;
    float width_scale;
    float height_scale;
    HUDInterfaceScalingFlags scaling_flags;
    uint8_t _pad_e[2];
    uint8_t _pad_10[20];
    ColorARGBInt default_color;
    ColorARGBInt flashing_color;
    float flash_period;
    float flash_delay;
    int16_t number_of_flashes;
    HUDInterfaceFlashFlags flash_flags;
    float flash_length;
    ColorARGBInt disabled_color;
    uint8_t _pad_40[4];
    int16_t frame_rate;
    uint8_t _pad_46[2];
    uint16_t sequence_index;
    WeaponHUDInterfaceOverlayType type;
    HUDInterfaceOverlayFlashFlags flags;
    uint8_t _pad_50[16];
    uint8_t _pad_60[40];
} WeaponHUDInterfaceOverlay;  // size 0x88
typedef struct WeaponHUDInterfaceOverlayElement {
    WeaponHUDInterfaceStateAttachedTo_t state_attached_to;
    uint8_t _pad_2[2];
    WeaponHUDInterfaceViewType_t allowed_view_type;
    HUDInterfaceChildAnchor_t anchor;
    uint8_t _pad_8[28];
    TagDependency overlay_bitmap;  // bitmap
    TagReflexive overlays;  // WeaponHUDInterfaceOverlay
    uint8_t _pad_40[40];
} WeaponHUDInterfaceOverlayElement;  // size 0x68
typedef struct WeaponHUDInterfaceScreenEffect {
    uint8_t _pad_0[4];
    WeaponHUDInterfaceScreenEffectDefinitionMaskFlags mask_flags;
    uint8_t _pad_6[2];
    uint8_t _pad_8[16];
    TagDependency mask_fullscreen;  // bitmap
    TagDependency mask_splitscreen;  // bitmap
    uint8_t _pad_38[8];
    WeaponHUDInterfaceScreenEffectDefinitionMaskFlags convolution_flags;
    uint8_t _pad_42[2];
    float convolution_fov_in_bounds[2];
    float convolution_radius_out_bounds[2];
    uint8_t _pad_54[24];
    WeaponHUDInterfaceScreenEffectDefinitionNightVisionFlags even_more_flags;
    int16_t night_vision_script_source;
    float night_vision_intensity;
    uint8_t _pad_74[24];
    WeaponHUDInterfaceScreenEffectDefinitionDesaturationFlags desaturation_flags;
    int16_t desaturation_script_source;
    float desaturation_intensity;
    ColorRGB effect_tint;
    uint8_t _pad_a0[24];
} WeaponHUDInterfaceScreenEffect;  // size 0xb8
typedef struct WeaponHUDInterface {
    TagDependency child_hud;  // weapon_hud_interface
    WeaponHUDInterfaceFlags flags;
    uint8_t _pad_12[2];
    int16_t total_ammo_cutoff;
    int16_t loaded_ammo_cutoff;
    int16_t heat_cutoff;
    int16_t age_cutoff;
    uint8_t _pad_1c[32];
    HUDInterfaceAnchor_t anchor;
    HUDInterfaceCanvasSize_t canvas_size;
    uint8_t _pad_40[32];
    TagReflexive static_elements;  // WeaponHUDInterfaceStaticElement
    TagReflexive meter_elements;  // WeaponHUDInterfaceMeter
    TagReflexive number_elements;  // WeaponHUDInterfaceNumber
    TagReflexive crosshairs;  // WeaponHUDInterfaceCrosshair
    TagReflexive overlay_elements;  // WeaponHUDInterfaceOverlayElement
    WeaponHUDInterfaceCrosshairTypeFlags crosshair_types;
    uint8_t _pad_a0[12];
    TagReflexive screen_effect;  // WeaponHUDInterfaceScreenEffect
    uint8_t _pad_b8[132];
    uint16_t messaging_information_sequence_index;
    int16_t messaging_information_width_offset;
    Point2DInt messaging_information_offset_from_reference_corner;
    ColorARGBInt messaging_information_override_icon_color;
    int8_t messaging_information_frame_rate;
    HUDInterfaceMessagingFlags messaging_information_flags;
    uint16_t messaging_information_text_index;
    uint8_t _pad_14c[48];
} WeaponHUDInterface;  // size 0x17c
typedef enum WeatherParticleSystemRenderDirectionSource {
    weatherparticlesystemrenderdirectionsource_from_velocity = 0,
    weatherparticlesystemrenderdirectionsource_from_acceleration = 1,
} WeatherParticleSystemRenderDirectionSource;  // int16
typedef int16_t WeatherParticleSystemRenderDirectionSource_t;
typedef uint32_t WeatherParticleSystemParticleTypeFlags;  // bitfield: interpolate_colors_in_hsv, along_long_hue_path, random_rotation
typedef struct WeatherParticleSystemParticleType {
    TagString name;
    WeatherParticleSystemParticleTypeFlags flags;
    float fade_in_start_distance;
    float fade_in_end_distance;
    float fade_out_start_distance;
    float fade_out_end_distance;
    float fade_in_start_height;
    float fade_in_end_height;
    float fade_out_start_height;
    float fade_out_end_height;
    uint8_t _pad_44[96];
    float particle_count[2];
    TagDependency physics;  // point_physics
    uint8_t _pad_bc[16];
    float acceleration_magnitude[2];
    float acceleration_turning_rate;
    float acceleration_change_rate;
    uint8_t _pad_dc[32];
    float particle_radius[2];
    float animation_rate[2];
    float rotation_rate[2];
    uint8_t _pad_114[32];
    ColorARGB color_lower_bound;
    ColorARGB color_upper_bound;
    float sprite_size;
    uint8_t _pad_158[60];
    TagDependency sprite_bitmap;  // bitmap
    ParticleOrientation_t render_mode;
    WeatherParticleSystemRenderDirectionSource_t render_direction_source;
    uint8_t _pad_1a8[36];
    ShaderType_t shader_type;
    uint8_t _pad_1ce[2];
    ParticleShaderFlags shader_flags;
    FramebufferBlendFunction_t framebuffer_blend_function;
    FramebufferFadeMode_t framebuffer_fade_mode;
    IsUnfilteredFlag bitmap_flags;
    uint8_t _pad_1d8[28];
    TagDependency secondary_bitmap;  // bitmap
    ParticleAnchor_t anchor;
    IsUnfilteredFlag secondary_bitmap_flags;
    FunctionOut_t u_animation_source;
    WaveFunction_t u_animation_function;
    float u_animation_period;
    float u_animation_phase;
    float u_animation_scale;
    FunctionOut_t v_animation_source;
    WaveFunction_t v_animation_function;
    float v_animation_period;
    float v_animation_phase;
    float v_animation_scale;
    FunctionOut_t rotation_animation_source;
    WaveFunction_t rotation_animation_function;
    float rotation_animation_period;
    float rotation_animation_phase;
    float rotation_animation_scale;
    Point2D rotation_animation_center;
    uint8_t _pad_240[4];
    float zsprite_radius_scale;
    uint8_t _pad_248[20];
} WeatherParticleSystemParticleType;  // size 0x25c
typedef struct WeatherParticleSystem {
    IsUnusedFlag flags;
    uint8_t _pad_4[32];
    TagReflexive particle_types;  // WeatherParticleSystemParticleType
} WeatherParticleSystem;  // size 0x30
typedef struct Wind {
    float velocity[2];
    Euler2D variation_area;
    float local_variation_weight;
    float local_variation_rate;
    float damping;
    uint8_t _pad_1c[36];
} Wind;  // size 0x40
#pragma pack(pop)
