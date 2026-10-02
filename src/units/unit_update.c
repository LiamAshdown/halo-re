// unit_update  (Ghidra: unit_update)
// address 0x5625b0, size 4765 bytes
// name confidence: 0.8   rewrite confidence: 0.8
// REWRITTEN from objdump 0x5625b0..0x56384c (the draft was 0.15). The unit row per-tick update (object type
//   definition +0x34), run before the biped / vehicle update. Stack: unit. In order:
//   - the per-tick AI stagger budget (0x6ef910: limit +0, peak +2, exhausted +4) against +0x20c; an update over
//     budget later forces the luminosity sample;
//   - control resets: a scripted random turn (+0x204 bit 25) or, without control input (bit 0), the facing /
//     aim / look copies (+0x224/+0x230/+0x254) and throttle (+0x278);
//   - unless the unit tag +0x17c bit 11: the scripted control flash (+0x210 ticks of +0x214 bits, 0x800
//     pulsed every 7th tick), the driver (+0x324: team, control bits 0..5, facing, throttle) and gunner
//     (+0x328: aim, control bits 10..14, trigger +0x284) copies, the control idle counter +0x322, the +0x37c /
//     +0x380 ramps, the +0x428 / +0x28c (drop weapon) / +0x420 (knock-down recovery or release) timers;
//   - unless +0x17c bit 10: weapon readying / dropping, grenade type selection, bottomless grenades, zoom change
//     (sound for a local player), aim (+0x230 -> +0x23c, rate/accel from tag +0x264/+0x268) and look
//     (+0x254 -> +0x260, +0x270/+0x274) following, the aim change byte +0x323, the grenade throw state (+0x28d)
//     and the current weapon control flags;
//   - unless +0x17c bit 11: the look delta controls blend, powered seats (+0x338, tag +0x2cc/+0x2d0);
//   - the delayed threat reaction (+0x406), melee lunge, animation timers, luminosity, autoaim (+0x28b);
//   - the +0x2e8 decay, the flashlight (+0x204 bit 19, energy +0x344, glow +0x340, effects from the game globals
//     and tag +0x194) and the weapon light (+0x348). Always returns 1.
// VERIFIED (ftol operand only) against disassembly 0x562f10..0x562fa1 (2026-09-30): the one __ftol (0x562f99) converts
//   clamp(angle / (aiming_velocity_maximum * 30 ticks ^-1), 0, 1) * 255.0 (0 when the maximum is 0) into the aim change
//   byte +0x323, as written below. The rest of the body was not re-compared instruction by instruction.
// blam-cc: stack -> unit_index

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "cache.h"
#include "objects.h"
#include "units.h"
#include "effects.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern data_array *object_data;     // 0x008603b0
extern tag_instance *tag_instances; // 0x0087bc14
extern data_array *player_data;     // 0x0087a480
extern uint8_t unit_updates_suppressed; // 0x0071c419
extern uint8_t *ai_update_stagger;  // 0x006ef910
extern game_engine_definition *current_game_engine;
extern uint8_t weapon_bottomless_clip; // 0x0087abc2
extern Globals *global_globals;
extern char *s_stand;                  // 0x0069fdec "stand"
extern real_vector3d *global_forward3d_pointer; // 0x00696718
extern real_point3d *global_origin3d_pointer;   // 0x00696714
extern real_point3d *global_zero_vector3d_pointer; // 0x006966f8

extern void actor_react_to_threat_event(datum_index self_object_index, datum_index other_object_index,
    int32_t event_kind, real magnitude, uint32_t extra_param, uint8_t suppress_vehicle_relay); // 0x42be40
extern void vector3d_cross_product(real_vector3d *out, const real_vector3d *a, const real_vector3d *b); // 0x4052c0
extern datum_index effect_new_on_object(datum_index creator_object_index, datum_index definition_index,
    datum_index object_index, int16_t first_person_weapon_override, real a_scale, real b_scale,
    const void *color, const void *tint_source); // 0x4507a0, EAX, ECX, stack
extern uint8_t game_engine_is_valid_team_player(uint32_t identifier); // 0x466b60
extern datum_index player_index_from_unit_index(datum_index unit_index); // 0x474db0
extern void weapon_set_control_flags(datum_index item_index, uint16_t control_flags, real primary_trigger); // 0x4c2990
extern void weapon_set_ready_timer(datum_index item_index, real value); // 0x4c2b20
extern real vector3d_angle_between_4cd4f0(real_vector3d *a, real_vector3d *b); // 0x4cd4f0, ECX, EDX
extern void vector3d_rotate_toward_with_acceleration(real_vector3d *direction, real_vector3d *target_direction,
    real_vector3d *angular_velocity, real maximum_velocity, real acceleration); // 0x4cf530, ESI, EDI, stack
extern void object_get_orientation(real_vector3d *out_forward, uint32_t object_index, real_vector3d *out_up); // 0x4f6970
extern datum_index sound_start_unspatialized(datum_index definition_index, float scale); // 0x543dd0, EDX, stack
extern void unit_clear_ground_adjust_dirty(uint32_t object_index); // 0x55ad70, EAX
extern void unit_dispatch_reaction_animation(int32_t unit_index, int16_t reaction_code); // 0x5614a0, ESI, stack
extern void unit_update_animation_timers(uint32_t unit_index); // 0x561620, EAX
extern uint8_t unit_is_look_target_valid(uint32_t unit_index); // 0x562570, ECX
extern void vector3d_rotate_toward_bounded(real_vector3d *current, real_vector3d *velocity, real *bounds,
    real max_velocity, real max_acceleration, real_vector3d *target, real_matrix4x3 *transform); // 0x564ae0
extern uint8_t unit_set_or_test_seat_and_weapon_label(uint32_t unit_index, char *seat_label, char *weapon_label,
    uint8_t apply); // 0x5651e0, EAX, stack
extern uint8_t unit_current_weapon_has_flag(uint32_t unit_index); // 0x565b60, ECX
extern uint8_t unit_state_is_scripted_animation(unit_data *unit); // 0x565c60, ECX
extern uint8_t unit_try_set_animation_state(uint32_t unit_index, int16_t new_state); // 0x565f90
extern void unit_release_transient_state(uint32_t unit_index, uint8_t is_light_reset); // 0x568610
extern datum_index unit_get_weapon_object_index(uint32_t unit_index, int16_t slot_index); // 0x569970, EAX, CX
extern uint8_t unit_clamp_direction_to_aim_or_look_bounds(uint32_t unit_index, real_vector3d *world_direction,
    uint8_t use_aiming_bounds); // 0x5697a0, EDI, stack
extern int32_t unit_find_next_grenade_type_with_count(uint32_t unit_index, int32_t start_index,
    int16_t direction); // 0x5699a0, EAX, ECX, stack
extern void unit_refresh_targeting_flag_and_weapons(uint32_t unit_index, uint8_t initial_targeting_flag); // 0x569bf0
extern uint8_t unit_check_weapon_use_permission(uint32_t unit_index, uint32_t weapon_index); // 0x56da00, ESI, EDI
extern void unit_ready_desired_weapon(uint32_t unit_index, uint8_t force); // 0x56d6e0
extern uint8_t unit_drop_current_weapon(uint32_t unit_index, uint8_t force); // 0x56dec0
extern uint8_t unit_begin_throw_grenade(uint32_t unit_index, const real_vector2d *direction); // 0x56e080, EDI, stack
extern void unit_throw_grenade_move_to_hand(uint32_t unit_index); // 0x56e280
extern void unit_release_thrown_grenade(uint32_t object_index, uint8_t apply_throw_fraction); // 0x56e440
extern void unit_update_look_delta_controls(uint32_t object_index); // 0x56e820, EAX
extern void unit_calculate_luminosity(uint32_t object_index); // 0x56ec60, EDI
extern void unit_melee_lunge_damage_tick(uint32_t unit_index); // 0x56fc80
extern void unit_update_autoaim_interaction(uint32_t unit_index); // 0x570720
extern void unit_update_random_turn_angle(uint32_t object_index, real_vector3d *out_axis); // 0x570840, EAX, EDI

#define OBJECT_DATA(h) ((uint8_t *)((object_header *)object_data->data)[(h) & 0xffff].data)
#define TAG_DATA(t) ((uint8_t *)tag_instances[(t) & 0xffff].data)
#define LOOK_BLEND_NEW 0.3f
#define LOOK_BLEND_OLD 0.7f

uint8_t unit_update(uint32_t unit_index)
{
    uint8_t *obj = OBJECT_DATA(unit_index);
    uint8_t *tag = TAG_DATA(*(datum_index *)obj);
    uint8_t over_budget = 0;            // ebp-0x2
    uint8_t valid_team_player;          // ebp-0x4
    uint8_t riding = 0;                 // ebp-0x1
    float speed_scale;                  // ebp-0xc
    float rate;                         // ebp-0x18
    float acceleration;                 // ebp-0x14
    real_vector3d previous_aim;         // ebp-0x24
    real_point3d *zero_vector;          // edi = *0x6966f8

    valid_team_player = game_engine_is_valid_team_player(unit_index);
    if (!unit_updates_suppressed) {
        uint8_t *stagger = ai_update_stagger;

        (((unit_object *)obj)->unit.update_tick_counter)++;
        if (stagger[4] == 0 && ((unit_object *)obj)->unit.update_tick_counter > *(int16_t *)stagger) {
            stagger[4] = 1;
            over_budget = 1;
            ((unit_object *)obj)->unit.update_tick_counter = 0;
        } else if (*(int16_t *)(stagger + 2) <= ((unit_object *)obj)->unit.update_tick_counter) {
            *(int16_t *)(stagger + 2) = ((unit_object *)obj)->unit.update_tick_counter;
        }
    }
    if ((((unit_object *)obj)->unit.flags & 0x2000000) != 0) {
        unit_update_random_turn_angle(unit_index, (real_vector3d *)(obj + 0x224)); // EAX unit, EDI +0x224
        *(real_vector3d *)&((unit_object *)obj)->unit.desired_aiming_vector.i = *(real_vector3d *)&((unit_object *)obj)->unit.desired_facing_vector.i;
        *(real_vector3d *)&((unit_object *)obj)->unit.desired_looking_vector.i = *(real_vector3d *)&((unit_object *)obj)->unit.desired_facing_vector.i;
        *(real_vector3d *)&((unit_object *)obj)->unit.throttle.i = *global_forward3d_pointer;
        ((unit_object *)obj)->unit.control_flags = 0;
    } else if ((((unit_object *)obj)->unit.flags & 1) == 0) {
        *(real_vector3d *)&((unit_object *)obj)->unit.desired_looking_vector.i = *(real_vector3d *)&((unit_object *)obj)->base.forward.i;
        *(real_vector3d *)&((unit_object *)obj)->unit.desired_aiming_vector.i = *(real_vector3d *)&((unit_object *)obj)->base.forward.i;
        *(real_vector3d *)&((unit_object *)obj)->unit.desired_facing_vector.i = *(real_vector3d *)&((unit_object *)obj)->base.forward.i;
        *(real_point3d *)&((unit_object *)obj)->unit.throttle.i = *global_origin3d_pointer;
        ((unit_object *)obj)->unit.control_flags = 0;
    }

    if ((*(uint32_t *)&((Unit *)tag)->unit_flags & 0x800) == 0) {
        // 0x562742: the scripted control flash (+0x210 ticks of +0x214 bits)
        int32_t ticks = ((struct unit_object *)obj)->unit.persistent_control_ticks;

        if (ticks > 0) {
            uint32_t bits = ((struct unit_object *)obj)->unit.persistent_control_flags;
            uint32_t control = ((unit_object *)obj)->unit.control_flags | bits;

            if ((bits & 0x800) != 0) {
                control = (ticks % 7 == 0) ? (control | 0x800) : (control & ~0x800u);
                ((unit_object *)obj)->unit.primary_trigger = 1.0f;
            } else {
                ((unit_object *)obj)->unit.primary_trigger = 0.0f;
            }
            ((unit_object *)obj)->unit.control_flags = control;
            ((struct unit_object *)obj)->unit.persistent_control_ticks = --ticks;
            if (ticks == 0) {
                ((struct unit_object *)obj)->unit.persistent_control_flags = 0;
            }
        }
        if ((((unit_object *)obj)->unit.flags & 0x8000000) == 0) {
            // 0x5627c6: a driver (+0x324) and a gunner (+0x328) control this unit
            datum_index driver = ((unit_object *)obj)->unit.driver_unit_index;
            datum_index gunner = ((unit_object *)obj)->unit.gunner_unit_index;

            if (driver != k_datum_index_none && (obj[0x106] & 4) == 0) {
                uint8_t *d = OBJECT_DATA(driver);

                ((unit_object *)obj)->base.owner_team = ((struct object *)d)->owner_team;
                riding = 1;
                if (*(datum_index *)(d + 0x218) != k_datum_index_none || (d[0x2a3] != 0x1b && d[0x2a3] != 0x1a)) {
                    ((unit_object *)obj)->unit.control_flags |= *(uint32_t *)(d + 0x208) & 0x3f;
                    *(real_vector3d *)&((unit_object *)obj)->unit.desired_facing_vector.i = *(real_vector3d *)(d + 0x224);
                    *(real_point3d *)&((unit_object *)obj)->unit.throttle.i = *(real_point3d *)(d + 0x278);
                }
            }
            if (gunner != k_datum_index_none && (obj[0x106] & 4) == 0) {
                uint8_t *g = OBJECT_DATA(gunner);

                if (!riding) {
                    ((unit_object *)obj)->base.owner_team = ((struct object *)g)->owner_team;
                }
                if (*(datum_index *)(g + 0x218) != k_datum_index_none || (g[0x2a3] != 0x1b && g[0x2a3] != 0x1a)) {
                    *(real_vector3d *)&((unit_object *)obj)->unit.desired_aiming_vector.i = *(real_vector3d *)(g + 0x230);
                    *(real_vector3d *)&((unit_object *)obj)->unit.desired_looking_vector.i = *(real_vector3d *)(g + 0x230);
                    ((unit_object *)obj)->unit.control_flags |= *(uint32_t *)(g + 0x208) & 0x7c00;
                    ((unit_object *)obj)->unit.primary_trigger = *(float *)(g + 0x284);
                }
            }
            if ((((unit_object *)obj)->unit.control_flags & 0x7c00) != 0) {
                obj[0x322] = 0;
            } else if ((int8_t)obj[0x322] < 0x7f) {
                obj[0x322]++;
            }
        }
        if (!unit_updates_suppressed) {
            // 0x562961: +0x37c (zoom-ish ramp, driven by +0x204 bit 4), +0x380 (bit 5)
            if ((obj[0x204] & 0x10) != 0) {
                float step = 0.008333334f;

                if (current_game_engine != 0 && ((struct unit_object *)obj)->unit.active_camouflage_regrowth != 0 && ((struct unit_object *)obj)->unit.active_camouflage_regrowth == 1) {
                    datum_index weapon = unit_get_weapon_object_index(unit_index,
                        *(int16_t *)(OBJECT_DATA(unit_index) + 0x2f2));

                    if (weapon != k_datum_index_none) {
                        uint8_t *weapon_tag = TAG_DATA(*(datum_index *)OBJECT_DATA(weapon));

                        if (*(float *)(weapon_tag + 0x4d0) != 0.0f) {
                            step = *(float *)(weapon_tag + 0x4d0);
                        }
                    }
                }
                ((struct unit_object *)obj)->unit.active_camouflage_power += step;
                if (((struct unit_object *)obj)->unit.active_camouflage_power > 1.0f) {
                    ((struct unit_object *)obj)->unit.active_camouflage_power = 1.0f;
                    ((struct unit_object *)obj)->unit.active_camouflage_regrowth = 0;
                }
            } else {
                ((struct unit_object *)obj)->unit.active_camouflage_power -= 0.008333334f;
                if (((struct unit_object *)obj)->unit.active_camouflage_power < 0.0f) {
                    ((struct unit_object *)obj)->unit.active_camouflage_power = 0.0f;
                }
            }
            if ((obj[0x204] & 0x20) != 0) {
                ((struct unit_object *)obj)->unit.super_active_camouflage_power += 0.011111111f;
                if (((struct unit_object *)obj)->unit.super_active_camouflage_power > 1.0f) {
                    ((struct unit_object *)obj)->unit.super_active_camouflage_power = 1.0f;
                }
            } else {
                ((struct unit_object *)obj)->unit.super_active_camouflage_power -= 0.011111111f;
                if (((struct unit_object *)obj)->unit.super_active_camouflage_power < 0.0f) {
                    ((struct unit_object *)obj)->unit.super_active_camouflage_power = 0.0f;
                }
            }
            if (((struct unit_object *)obj)->unit.stun_ticks > 0 && --((struct unit_object *)obj)->unit.stun_ticks == 0) {
                *(int32_t *)&((struct unit_object *)obj)->unit.stun = 0;
            }
            if ((int8_t)obj[0x28c] > 0 && --obj[0x28c] == 0) {
                unit_drop_current_weapon(unit_index, 1);
                if (unit_updates_suppressed) {
                    goto controls;
                }
            }
            if (((struct unit_object *)obj)->unit.feign_death_ticks > 0 && (obj[0x10] & 0x20) != 0 && --((struct unit_object *)obj)->unit.feign_death_ticks == 0) {
                // 0x562b11: the knock-down is over
                if (((unit_object *)obj)->base.body_vitality > 0.0f) {
                    int16_t state = (int16_t)((~(obj[0x298] >> 3) & 1) | 0x22);

                    ((unit_object *)obj)->base.vitality_flags &= 0xfffb;
                    unit_refresh_targeting_flag_and_weapons(unit_index, 1);
                    unit_set_or_test_seat_and_weapon_label(unit_index, s_stand, 0, 1);
                    unit_try_set_animation_state(unit_index, state);
                    ((unit_object *)obj)->unit.animation_state_flags &= 0xfffb;
                    if (((unit_object *)obj)->base.type == 0) {
                        unit_clear_ground_adjust_dirty(unit_index);
                    }
                    unit_dispatch_reaction_animation(unit_index, 5);
                } else {
                    unit_release_transient_state(unit_index, 0);
                }
            }
        }
    }

controls:
    // 0x562ba8
    if ((*(uint32_t *)&((Unit *)tag)->unit_flags & 0x400) == 0) {
        if ((obj[0x106] & 4) == 0 && !unit_updates_suppressed) {
            if ((((unit_object *)obj)->base.vitality_flags & 0x400) != 0) {
                unit_drop_current_weapon(unit_index, 1);
            } else if (((unit_object *)obj)->unit.desired_weapon_index != ((unit_object *)obj)->unit.current_weapon_index &&
                       !unit_state_is_scripted_animation((unit_data *)(obj + k_unit_data_offset))) {
                datum_index weapon = unit_get_weapon_object_index(unit_index,
                    *(int16_t *)(OBJECT_DATA(unit_index) + 0x2f4));

                if (weapon != k_datum_index_none && unit_check_weapon_use_permission(unit_index, weapon)) {
                    unit_ready_desired_weapon(unit_index, 1);
                }
            }
            if (obj[0x31d] != obj[0x31c] && !unit_state_is_scripted_animation((unit_data *)(obj + k_unit_data_offset))) {
                int16_t grenade = unit_find_next_grenade_type_with_count(unit_index, (int16_t)(int8_t)obj[0x31d], 0);

                if (grenade != -1) {
                    obj[0x31c] = (uint8_t)grenade;
                }
            }
            if (weapon_bottomless_clip && ((unit_object *)obj)->unit.controlling_player != k_datum_index_none) {
                int32_t i;

                for (i = 0; i < 2; i++) {
                    if ((int8_t)obj[0x31e + i] <= 1) {
                        obj[0x31e + i] = 1;
                    }
                }
                if (obj[0x31d] == 0xff) {
                    obj[0x31d] = 0;
                }
            }
            if (obj[0x321] != obj[0x320]) {
                // 0x562ce7: zoom level changed: the weapon's zoom sound for a local player
                obj[0x320] = obj[0x321];
                if (obj[0x320] == 0xff) {
                    *(int32_t *)&((struct unit_object *)obj)->unit.integrated_night_vision_power = 0;
                }
                if (player_index_from_unit_index(unit_index) != k_datum_index_none &&
                    *(int16_t *)((uint8_t *)player_data->data +
                        (player_index_from_unit_index(unit_index) & 0xffff) * 0x200 + 2) != -1) {
                    datum_index weapon = unit_get_weapon_object_index(unit_index,
                        *(int16_t *)(OBJECT_DATA(unit_index) + 0x2f2));

                    if (weapon != k_datum_index_none) {
                        uint8_t *weapon_tag = TAG_DATA(*(datum_index *)OBJECT_DATA(weapon));
                        datum_index sound = (obj[0x320] == 0xff) ? *(datum_index *)(weapon_tag + 0x4bc)
                                                                 : *(datum_index *)(weapon_tag + 0x4ac);
                        float fraction = 1.0f;

                        if (obj[0x320] != 0xff && *(int16_t *)(weapon_tag + 0x3da) > 1) {
                            fraction = (float)(int8_t)obj[0x320] / (float)(*(int16_t *)(weapon_tag + 0x3da) - 1);
                        }
                        if (sound != k_datum_index_none) {
                            sound_start_unspatialized(sound, fraction);
                        }
                    }
                }
            }
        }

        // 0x562dce: aim and look follow their desired directions
        speed_scale = (obj[0x288] == 1) ? ((Unit *)tag)->casual_aiming_modifier : 1.0f;
        rate = speed_scale * ((Unit *)tag)->aiming_velocity_maximum * 0.033333335f;
        acceleration = speed_scale * ((Unit *)tag)->aiming_acceleration_maximum * 0.0011111111f;
        previous_aim = *(real_vector3d *)&((unit_object *)obj)->unit.aiming_vector.i;
        zero_vector = global_zero_vector3d_pointer;
        if (rate == 0.0f && acceleration == 0.0f) {
            *(real_vector3d *)&((unit_object *)obj)->unit.aiming_vector.i = *(real_vector3d *)&((unit_object *)obj)->unit.desired_aiming_vector.i;
            if (unit_is_look_target_valid(unit_index)) {
                unit_clamp_direction_to_aim_or_look_bounds(unit_index, (real_vector3d *)(obj + 0x23c), 1);
            }
            *(real_point3d *)&((unit_object *)obj)->unit.aiming_velocity.i = *global_origin3d_pointer;
        } else if (obj[0x2b6] != 0) {
            real_matrix4x3 basis;

            basis.scale = 1.0f;
            object_get_orientation(&basis.forward, unit_index, &basis.up);
            vector3d_cross_product(&basis.left, &basis.forward, &basis.up);
            basis.position = *zero_vector;
            vector3d_rotate_toward_bounded((real_vector3d *)(obj + 0x23c), (real_vector3d *)(obj + 0x248),
                (real *)(obj + 0x2b8), rate, acceleration, (real_vector3d *)(obj + 0x230), &basis);
        } else {
            vector3d_rotate_toward_with_acceleration((real_vector3d *)(obj + 0x23c), (real_vector3d *)(obj + 0x230),
                (real_vector3d *)(obj + 0x248), rate, acceleration);
        }
        {
            float change = 0.0f;

            if (((Unit *)tag)->aiming_velocity_maximum != 0.0f) {
                change = vector3d_angle_between_4cd4f0(&previous_aim, (real_vector3d *)(obj + 0x23c)) /
                    (((Unit *)tag)->aiming_velocity_maximum * 0.033333335f);
                if (change < 0.0f) {
                    change = 0.0f;
                } else if (change > 1.0f) {
                    change = 1.0f;
                }
            }
            obj[0x323] = (uint8_t)(int32_t)(change * 255.0f);
        }
        rate = speed_scale * ((Unit *)tag)->looking_velocity_maximum * 0.033333335f;
        acceleration = speed_scale * ((Unit *)tag)->looking_acceleration_maximum * 0.0011111111f;
        if (rate == 0.0f && acceleration == 0.0f) {
            *(real_vector3d *)&((unit_object *)obj)->unit.looking_vector.i = *(real_vector3d *)&((unit_object *)obj)->unit.desired_looking_vector.i;
            unit_clamp_direction_to_aim_or_look_bounds(unit_index, (real_vector3d *)(obj + 0x260), 0);
            *(real_point3d *)&((unit_object *)obj)->unit.looking_velocity.i = *global_origin3d_pointer;
        } else if (obj[0x2b7] != 0) {
            real_matrix4x3 basis;

            basis.scale = 1.0f;
            object_get_orientation(&basis.forward, unit_index, &basis.up);
            vector3d_cross_product(&basis.left, &basis.forward, &basis.up);
            basis.position = *zero_vector;
            vector3d_rotate_toward_bounded((real_vector3d *)(obj + 0x260), (real_vector3d *)(obj + 0x26c),
                (real *)(obj + 0x2c8), rate, acceleration, (real_vector3d *)(obj + 0x254), &basis);
        } else {
            vector3d_rotate_toward_with_acceleration((real_vector3d *)(obj + 0x260), (real_vector3d *)(obj + 0x254),
                (real_vector3d *)(obj + 0x26c), rate, acceleration);
        }

        if (!unit_updates_suppressed) {
            // 0x5630f8: grenade throw state (+0x28d)
            uint8_t throwing = (uint8_t)((((unit_object *)obj)->unit.control_flags >> 13) & 1);

            switch ((int8_t)obj[0x28d]) {
            case 0:
                if (throwing) {
                    unit_begin_throw_grenade(unit_index, 0);
                }
                break;
            case 1:
                if (((unit_object *)obj)->base.animation_frame >= 2) {
                    unit_throw_grenade_move_to_hand(unit_index);
                }
                break;
            case 2:
                (((unit_object *)obj)->unit.throwing_grenade_counter)++;
                if (obj[0x2a3] != 0x21) {
                    unit_release_thrown_grenade(unit_index, 1);
                }
                break;
            case 3:
                if (obj[0x2a3] != 0x21 && !throwing) {
                    obj[0x28d] = 0;
                }
                break;
            default:
                break;
            }
        }
        if (((unit_object *)obj)->unit.current_weapon_index != -1 && !unit_updates_suppressed) {
            // 0x563194: the current weapon's trigger / control flags
            uint32_t control = 0;
            float trigger = ((unit_object *)obj)->unit.primary_trigger;
            uint8_t *unit_now;
            datum_index weapon = k_datum_index_none;

            if (((unit_object *)obj)->unit.current_weapon_index == ((unit_object *)obj)->unit.desired_weapon_index) {
                uint8_t flashing = (uint8_t)(((struct unit_object *)obj)->unit.persistent_control_ticks > 0 && (((struct unit_object *)obj)->unit.persistent_control_flags & 0x800) != 0);

                if (valid_team_player && (obj[0x208] & 0x10) != 0) {
                    control = 1;
                }
                if ((((unit_object *)obj)->unit.control_flags & 0x800) != 0) {
                    control |= 2;
                }
                if ((((unit_object *)obj)->unit.control_flags & 0x1000) != 0) {
                    control |= 4;
                }
                if ((*(uint32_t *)(TAG_DATA(*(datum_index *)obj) + 0x17c) & 0x800000) != 0) {
                    weapon_set_ready_timer(unit_get_weapon_object_index(unit_index,
                        *(int16_t *)(OBJECT_DATA(unit_index) + 0x2f2)), ((struct unit_object *)obj)->unit.integrated_light_power);
                }
                if ((((unit_object *)obj)->unit.control_flags & 0x400) != 0) {
                    control |= 8;
                }
                if (unit_state_is_scripted_animation((unit_data *)(obj + k_unit_data_offset)) && !flashing) {
                    control |= 0x10;
                }
                if (((unit_object *)obj)->base.type == 0 && (int8_t)obj[0x505] > 0) {
                    control |= 0x10;
                }
                if (obj[0x320] != 0xff) {
                    control |= 0x40;
                }
            } else {
                control = 0x20;
            }
            unit_now = OBJECT_DATA(unit_index);
            if (*(int16_t *)(unit_now + 0x2f2) != -1) {
                weapon = *(datum_index *)(unit_now + 0x2f8 + *(int16_t *)(unit_now + 0x2f2) * 4);
            }
            weapon_set_control_flags(weapon, (uint16_t)control, trigger);
        }
    }

    // 0x5632ca
    if ((*(uint32_t *)&((Unit *)tag)->unit_flags & 0x800) == 0) {
        int16_t seat;

        if ((obj[0x298] & 2) != 0) {
            unit_update_look_delta_controls(unit_index);
            *(float *)(obj + 0x364) = *(float *)(obj + 0x370) * LOOK_BLEND_NEW + *(float *)(obj + 0x364) * LOOK_BLEND_OLD;
            *(float *)(obj + 0x368) = *(float *)(obj + 0x374) * LOOK_BLEND_NEW + *(float *)(obj + 0x368) * LOOK_BLEND_OLD;
            *(float *)(obj + 0x36c) = *(float *)(obj + 0x378) * LOOK_BLEND_NEW + *(float *)(obj + 0x36c) * LOOK_BLEND_OLD;
        }
        for (seat = 0; seat < *(int32_t *)&((Unit *)tag)->powered_seats.count; seat++) {
            uint8_t *powered = *(uint8_t **)&((Unit *)tag)->powered_seats.pointer + seat * 0x44;
            float *power = (float *)(obj + 0x338 + seat * 4);
            uint8_t occupied;

            if (seat == 0) {
                occupied = (uint8_t)(((unit_object *)obj)->unit.driver_unit_index != k_datum_index_none || (obj[0x204] & 1) != 0);
            } else {
                occupied = (uint8_t)(((unit_object *)obj)->unit.gunner_unit_index != k_datum_index_none &&
                    ((unit_object *)obj)->unit.gunner_unit_index != ((unit_object *)obj)->unit.driver_unit_index);
            }
            if ((obj[0x106] & 4) == 0 && occupied) {
                if (*power != 1.0f) {
                    *power += 1.0f / (*(float *)(powered + 4) * 30.0f);
                    if (*power > 1.0f) {
                        *power = 1.0f;
                    }
                }
            } else if (*power != 0.0f) {
                *power -= 1.0f / (*(float *)(powered + 8) * 30.0f);
                if (*power < 0.0f) {
                    *power = 0.0f;
                }
            }
        }
    }
    // 0x563453: a delayed threat reaction
    if (((struct unit_object *)obj)->unit.delayed_damage_ticks > 0 && --((struct unit_object *)obj)->unit.delayed_damage_ticks == 0) {
        actor_react_to_threat_event(unit_index, ((struct unit_object *)obj)->unit.delayed_damage_responsible_object, *(uint16_t *)&((struct unit_object *)obj)->unit.delayed_damage_category,
            ((struct unit_object *)obj)->unit.delayed_damage_amount, 0, 1);
        ((struct unit_object *)obj)->unit.delayed_damage_category = 0;
        ((struct unit_object *)obj)->unit.delayed_damage_responsible_object = k_datum_index_none;
        *(int32_t *)&((struct unit_object *)obj)->unit.delayed_damage_amount = 0;
    }
    if (!unit_updates_suppressed) {
        unit_melee_lunge_damage_tick(unit_index);
        if (!unit_updates_suppressed) {
            unit_update_animation_timers(unit_index);
            if (!unit_updates_suppressed && (over_budget || ((unit_object *)obj)->unit.controlling_player != k_datum_index_none)) {
                unit_calculate_luminosity(unit_index);
            }
        }
    }
    if (obj[0x28b] != 0) {
        if (unit_updates_suppressed) {
            goto done;
        }
        if (--obj[0x28b] == 0) {
            unit_update_autoaim_interaction(unit_index);
            if (unit_updates_suppressed) {
                goto done;
            }
        }
    } else if (unit_updates_suppressed) {
        goto done;
    }
    // 0x563531: +0x2e8 relaxes toward 0 by at most 0.1 per tick
    {
        float step = -((unit_object *)obj)->unit.mouth_aperture;

        if (step < -0.1f) {
            step = -0.1f;
        } else if (step > 0.1f) {
            step = 0.1f;
        }
        ((unit_object *)obj)->unit.mouth_aperture += step;
    }
    {
        // 0x56356b: the flashlight (+0x204 bit 19), its on/off requests (bits 28/29), energy +0x344, glow +0x340
        uint8_t toggle = 0;           // ebp-0x3 starts at 0
        uint32_t flags = ((unit_object *)obj)->unit.flags;
        uint32_t button;

        if ((flags & 0x10000000) != 0) {
            if ((flags & 0x80000) == 0) {
                toggle = 1;
            }
            ((unit_object *)obj)->unit.flags = flags & 0xefffffff;
        }
        flags = ((unit_object *)obj)->unit.flags;
        if ((flags & 0x20000000) != 0) {
            if ((flags & 0x80000) != 0) {
                toggle = 1;
            }
            ((unit_object *)obj)->unit.flags = flags & 0xdfffffff;
        }
        button = ((unit_object *)obj)->unit.control_flags & 0x10;
        if (button != 0 || !(((struct unit_object *)obj)->unit.integrated_light_energy > 0.0f) || toggle) {
            if (!valid_team_player) {
                flags = ((unit_object *)obj)->unit.flags;
                if ((flags & 0x4000000) != 0) {
                    ((unit_object *)obj)->unit.flags = flags & 0xfbffffff;
                }
                flags = ((unit_object *)obj)->unit.flags;
                if ((flags & 0x80000) != 0) {
                    ((unit_object *)obj)->unit.flags = (flags & 0xfff7ffff) | 0x10;
                }
            } else {
                uint8_t toggle_light = 1;

                if (unit_current_weapon_has_flag(unit_index)) {
                    if (button != 0) {
                        uint8_t *effects = (uint8_t *)global_globals->first_person_interface.pointer;
                        datum_index effect = ((((unit_object *)obj)->unit.flags & 0x4000000) != 0)
                            ? *(datum_index *)(effects + 0x64) : *(datum_index *)(effects + 0x54);

                        if (effect != k_datum_index_none) {
                            effect_new_on_object(unit_index, effect, unit_index, -1, 0.0f, 0.0f, 0, 0);
                        }
                        ((unit_object *)obj)->unit.flags ^= 0x4000000;
                    }
                    if ((obj[0x208] & 0x10) != 0) {
                        toggle_light = 0;
                    }
                }
                if (toggle_light && ((((unit_object *)obj)->unit.flags & 0x80000) != 0 || ((struct unit_object *)obj)->unit.integrated_light_energy > 0.2f) &&
                    ((unit_object *)obj)->base.parent_object == k_datum_index_none) {
                    effect_new_on_object(unit_index, *(datum_index *)&((Unit *)tag)->integrated_light_toggle.tag_id, unit_index, -1, 0.0f, 0.0f, 0, 0);
                    ((unit_object *)obj)->unit.flags ^= 0x80000;
                }
            }
        }
        flags = ((unit_object *)obj)->unit.flags;
        if ((flags & 0x80000) != 0) {
            if ((*(uint32_t *)&((Unit *)tag)->unit_flags & 0x1000000) == 0) {
                ((struct unit_object *)obj)->unit.integrated_light_energy -= 0.00027777778f;
            }
            if (((unit_object *)obj)->base.parent_object != k_datum_index_none || (obj[0x106] & 4) != 0) {
                ((unit_object *)obj)->unit.flags = flags & 0xfff7ffff;
            }
            if (((struct unit_object *)obj)->unit.integrated_light_power != 1.0f) {
                ((struct unit_object *)obj)->unit.integrated_light_power += 0.16666667f;
                if (((struct unit_object *)obj)->unit.integrated_light_power > 1.0f) {
                    ((struct unit_object *)obj)->unit.integrated_light_power = 1.0f;
                }
            }
        } else {
            if (((struct unit_object *)obj)->unit.integrated_light_energy < 1.0f) {
                ((struct unit_object *)obj)->unit.integrated_light_energy += 0.0011111111f;
            }
            if (((struct unit_object *)obj)->unit.integrated_light_power != 0.0f) {
                ((struct unit_object *)obj)->unit.integrated_light_power -= 0.041666668f;
                if (((struct unit_object *)obj)->unit.integrated_light_power < 0.0f) {
                    ((struct unit_object *)obj)->unit.integrated_light_power = 0.0f;
                }
            }
        }
    }
    // 0x5637ad: the current weapon's secondary light (+0x204 bit 26) ramps +0x348
    if (unit_current_weapon_has_flag(unit_index)) {
        if ((((unit_object *)obj)->unit.flags & 0x4000000) != 0) {
            if (((struct unit_object *)obj)->unit.integrated_night_vision_power != 1.0f) {
                ((struct unit_object *)obj)->unit.integrated_night_vision_power += 0.083333336f;
                if (((struct unit_object *)obj)->unit.integrated_night_vision_power > 1.0f) {
                    ((struct unit_object *)obj)->unit.integrated_night_vision_power = 1.0f;
                }
            }
        } else if (((struct unit_object *)obj)->unit.integrated_night_vision_power != 0.0f) {
            ((struct unit_object *)obj)->unit.integrated_night_vision_power -= 0.041666668f;
            if (((struct unit_object *)obj)->unit.integrated_night_vision_power < 0.0f) {
                ((struct unit_object *)obj)->unit.integrated_night_vision_power = 0.0f;
            }
        }
    }
done:
    return 1;
}

#if 0
Original Ghidra decompilation (0x5625b0):

undefined4 FUN_005625b0(uint param_1)

{
  uint *puVar1;
  float fVar2;
  float fVar3;
  uint *puVar4;
  bool bVar5;
  undefined *puVar6;
  char cVar7;
  undefined1 uVar8;
  short sVar9;
  uint uVar10;
  char *pcVar11;
  int iVar12;
  uint uVar13;
  int iVar14;
  int iVar15;
  undefined4 *puVar16;
  undefined1 local_48 [12];
  undefined4 local_3c;
  undefined4 local_38;
  undefined4 local_34;
  uint local_28;
  uint local_24;
  uint local_20;
  float local_1c;
  float local_18;
  int local_14;
  float local_10;
  int local_c;
  char local_8;
  char local_7;
  char local_6;
  char local_5;

  local_14 = (param_1 & 0xffff) * 0xc;
  puVar4 = *(uint **)(*(int *)(DAT_008603b0 + 0x34) + 8 + local_14);
  local_c = *(int *)((*puVar4 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
  local_6 = '\0';
  local_7 = '\0';
  local_8 = FUN_00466b60(param_1);
  if (DAT_0071c419 == '\0') {
    *(short *)(puVar4 + 0x83) = (short)puVar4[0x83] + 1;
    sVar9 = (short)puVar4[0x83];
    if (((char)DAT_006ef910[2] == '\0') && (*DAT_006ef910 < sVar9)) {
      *(undefined1 *)(DAT_006ef910 + 2) = 1;
      local_6 = '\x01';
      *(undefined2 *)(puVar4 + 0x83) = 0;
    }
    else {
      if (sVar9 < DAT_006ef910[1]) {
        sVar9 = DAT_006ef910[1];
      }
      DAT_006ef910[1] = sVar9;
    }
  }
  if ((puVar4[0x81] & 0x2000000) == 0) {
    if ((puVar4[0x81] & 1) == 0) {
      puVar1 = puVar4 + 0x1d;
      puVar4[0x95] = *puVar1;
      puVar4[0x96] = puVar4[0x1e];
      puVar4[0x97] = puVar4[0x1f];
      puVar4[0x8c] = *puVar1;
      puVar4[0x8d] = puVar4[0x1e];
      puVar4[0x8e] = puVar4[0x1f];
      puVar4[0x89] = *puVar1;
      puVar4[0x8a] = puVar4[0x1e];
      puVar6 = PTR_DAT_00696714;
      puVar4[0x8b] = puVar4[0x1f];
      puVar4[0x9e] = *(uint *)puVar6;
      puVar4[0x9f] = *(uint *)(puVar6 + 4);
      uVar10 = *(uint *)(puVar6 + 8);
      puVar4[0x82] = 0;
      goto LAB_0056272d;
    }
  }
  else {
    FUN_00570840();
    puVar4[0x8c] = puVar4[0x89];
    puVar4[0x8d] = puVar4[0x8a];
    puVar4[0x8e] = puVar4[0x8b];
    puVar4[0x95] = puVar4[0x89];
    puVar4[0x96] = puVar4[0x8a];
    puVar4[0x97] = puVar4[0x8b];
    puVar6 = PTR_DAT_00696718;
    puVar4[0x9e] = *(uint *)PTR_DAT_00696718;
    puVar4[0x9f] = *(uint *)(puVar6 + 4);
    uVar10 = *(uint *)(puVar6 + 8);
    puVar4[0x82] = 0;
LAB_0056272d:
    puVar4[0xa0] = uVar10;
  }
  if ((*(uint *)(local_c + 0x17c) & 0x800) == 0) {
    uVar10 = puVar4[0x84];
    local_5 = '\0';
    if (0 < (int)uVar10) {
      uVar13 = puVar4[0x82] | puVar4[0x85];
      puVar4[0x82] = uVar13;
      if ((puVar4[0x85] & 0x800) == 0) {
        puVar4[0xa1] = 0;
      }
      else {
        if ((int)uVar10 % 7 == 0) {
          uVar13 = uVar13 | 0x800;
        }
        else {
          uVar13 = uVar13 & 0xfffff7ff;
        }
        puVar4[0x82] = uVar13;
        puVar4[0xa1] = 0x3f800000;
      }
      puVar4[0x84] = uVar10 - 1;
      if (uVar10 - 1 == 0) {
        puVar4[0x85] = 0;
      }
    }
    if ((puVar4[0x81] & 0x8000000) == 0) {
      if ((puVar4[0xc9] != 0xffffffff) && ((*(byte *)((int)puVar4 + 0x106) & 4) == 0)) {
        iVar14 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (puVar4[0xc9] & 0xffff) * 0xc);
        *(undefined2 *)(puVar4 + 0x2e) = *(undefined2 *)(iVar14 + 0xb8);
        local_5 = '\x01';
        if ((*(int *)(iVar14 + 0x218) != -1) ||
           ((*(char *)(iVar14 + 0x2a3) != '\x1b' && (*(char *)(iVar14 + 0x2a3) != '\x1a')))) {
          puVar4[0x82] = puVar4[0x82] | *(uint *)(iVar14 + 0x208) & 0x3f;
          puVar4[0x89] = *(uint *)(iVar14 + 0x224);
          puVar4[0x8a] = *(uint *)(iVar14 + 0x228);
          puVar4[0x8b] = *(uint *)(iVar14 + 0x22c);
          puVar4[0x9e] = *(uint *)(iVar14 + 0x278);
          puVar4[0x9f] = *(uint *)(iVar14 + 0x27c);
          puVar4[0xa0] = *(uint *)(iVar14 + 0x280);
        }
      }
      if ((puVar4[0xca] != 0xffffffff) && ((*(byte *)((int)puVar4 + 0x106) & 4) == 0)) {
        iVar14 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (puVar4[0xca] & 0xffff) * 0xc);
        if (local_5 == '\0') {
          *(undefined2 *)(puVar4 + 0x2e) = *(undefined2 *)(iVar14 + 0xb8);
        }
        if ((*(int *)(iVar14 + 0x218) != -1) ||
           ((*(char *)(iVar14 + 0x2a3) != '\x1b' && (*(char *)(iVar14 + 0x2a3) != '\x1a')))) {
          puVar4[0x8c] = *(uint *)(iVar14 + 0x230);
          puVar4[0x8d] = *(uint *)(iVar14 + 0x234);
          puVar4[0x8e] = *(uint *)(iVar14 + 0x238);
          puVar4[0x95] = *(uint *)(iVar14 + 0x230);
          puVar4[0x96] = *(uint *)(iVar14 + 0x234);
          puVar4[0x97] = *(uint *)(iVar14 + 0x238);
          puVar4[0x82] = puVar4[0x82] | *(uint *)(iVar14 + 0x208) & 0x7c00;
          puVar4[0xa1] = *(uint *)(iVar14 + 0x284);
        }
      }
      if ((puVar4[0x82] & 0x7c00) == 0) {
        if (*(char *)((int)puVar4 + 0x322) < '\x7f') {
          *(char *)((int)puVar4 + 0x322) = *(char *)((int)puVar4 + 0x322) + '\x01';
        }
      }
      else {
        *(undefined1 *)((int)puVar4 + 0x322) = 0;
      }
    }
    if (DAT_0071c419 == '\0') {
      if ((puVar4[0x81] & 0x10) == 0) {
        fVar2 = (float)puVar4[0xdf];
        puVar4[0xdf] = (uint)(fVar2 - 0.008333334);
        if (fVar2 - 0.008333334 < 0.0) {
          puVar4[0xdf] = 0;
        }
      }
      else {
        if (((DAT_006f1d20 == 0) || (*(short *)((int)puVar4 + 0x422) == 0)) ||
           (*(short *)((int)puVar4 + 0x422) != 1)) {
LAB_005629e9:
          fVar2 = 0.008333334;
        }
        else {
          iVar14 = *(int *)(DAT_008603b0 + 0x34);
          uVar10 = unit_get_weapon_object_index();
          if ((uVar10 == 0xffffffff) ||
             (iVar14 = *(int *)((**(uint **)(iVar14 + 8 + (uVar10 & 0xffff) * 0xc) & 0xffff) * 0x20
                                + 0x14 + DAT_0087bc14), *(float *)(iVar14 + 0x4d0) == 0.0))
          goto LAB_005629e9;
          fVar2 = *(float *)(iVar14 + 0x4d0);
        }
        fVar3 = (float)puVar4[0xdf];
        puVar4[0xdf] = (uint)(fVar2 + fVar3);
        if (1.0 < fVar2 + fVar3) {
          puVar4[0xdf] = 0x3f800000;
          *(undefined2 *)((int)puVar4 + 0x422) = 0;
        }
      }
      if ((puVar4[0x81] & 0x20) == 0) {
        fVar2 = (float)puVar4[0xe0] - 0.011111111;
        puVar4[0xe0] = (uint)fVar2;
        if (fVar2 < 0.0) {
          puVar4[0xe0] = 0;
        }
      }
      else {
        fVar2 = (float)puVar4[0xe0] + 0.011111111;
        puVar4[0xe0] = (uint)fVar2;
        if (1.0 < fVar2) {
          puVar4[0xe0] = 0x3f800000;
        }
      }
      if ((0 < (short)puVar4[0x10a]) &&
         (sVar9 = (short)puVar4[0x10a] + -1, *(short *)(puVar4 + 0x10a) = sVar9, sVar9 == 0)) {
        puVar4[0x109] = 0;
      }
      if ((((((char)puVar4[0xa3] < '\x01') ||
            (cVar7 = (char)puVar4[0xa3] + -1, *(char *)(puVar4 + 0xa3) = cVar7, cVar7 != '\0')) ||
           (unit_drop_current_weapon(param_1,1), DAT_0071c419 == '\0')) &&
          ((0 < (short)puVar4[0x108] && ((puVar4[4] & 0x20) != 0)))) &&
         (sVar9 = (short)puVar4[0x108] + -1, *(short *)(puVar4 + 0x108) = sVar9, sVar9 == 0)) {
        if ((float)puVar4[0x38] <= 0.0) {
          FUN_00568610(param_1,0);
        }
        else {
          uVar10 = puVar4[0xa6];
          *(ushort *)((int)puVar4 + 0x106) = *(ushort *)((int)puVar4 + 0x106) & 0xfffb;
          FUN_00569bf0(param_1);
          unit_set_or_test_seat_and_weapon_label(PTR_s_stand_0069fdec,0,1);
          unit_try_set_animation_state(param_1,~((byte)uVar10 >> 3) & 1 | 0x22);
          *(ushort *)(puVar4 + 0xa6) = (ushort)puVar4[0xa6] & 0xfffb;
          if ((short)puVar4[0x2d] == 0) {
            FUN_0055ad70();
          }
          FUN_005614a0(5);
        }
      }
    }
  }
  if ((*(uint *)(local_c + 0x17c) & 0x400) == 0) {
    if (((*(ushort *)((int)puVar4 + 0x106) & 4) == 0) && (DAT_0071c419 == '\0')) {
      if ((*(ushort *)((int)puVar4 + 0x106) & 0x400) == 0) {
        if (((((short)puVar4[0xbd] != *(short *)((int)puVar4 + 0x2f2)) &&
             (cVar7 = FUN_00565c60(), cVar7 == '\0')) &&
            (iVar14 = unit_get_weapon_object_index(), iVar14 != -1)) &&
           (cVar7 = FUN_0056da00(), cVar7 != '\0')) {
          unit_ready_desired_weapon(param_1,1);
        }
      }
      else {
        unit_drop_current_weapon(param_1,1);
      }
      if (((*(char *)((int)puVar4 + 0x31d) != (char)puVar4[199]) &&
          (cVar7 = FUN_00565c60(), cVar7 == '\0')) && (sVar9 = FUN_005699a0(0), sVar9 != -1)) {
        *(char *)(puVar4 + 199) = (char)sVar9;
      }
      if ((DAT_0087abc2 != '\0') && (puVar4[0x86] != 0xffffffff)) {
        pcVar11 = (char *)((int)puVar4 + 0x31e);
        iVar14 = 2;
        do {
          cVar7 = *pcVar11;
          if (cVar7 < '\x02') {
            cVar7 = '\x01';
          }
          *pcVar11 = cVar7;
          pcVar11 = pcVar11 + 1;
          iVar14 = iVar14 + -1;
        } while (iVar14 != 0);
        if (*(char *)((int)puVar4 + 0x31d) == -1) {
          *(undefined1 *)((int)puVar4 + 0x31d) = 0;
        }
      }
      cVar7 = *(char *)((int)puVar4 + 0x321);
      if (cVar7 != (char)puVar4[200]) {
        *(char *)(puVar4 + 200) = cVar7;
        if (cVar7 == -1) {
          puVar4[0xd2] = 0;
        }
        iVar14 = FUN_00474db0(param_1);
        if ((iVar14 != -1) &&
           (uVar10 = FUN_00474db0(param_1),
           *(short *)((uVar10 & 0xffff) * 0x200 + 2 + *(int *)(DAT_0087a480 + 0x34)) != -1)) {
          iVar14 = *(int *)(DAT_008603b0 + 0x34);
          uVar10 = unit_get_weapon_object_index();
          if (uVar10 != 0xffffffff) {
            iVar14 = *(int *)((**(uint **)(iVar14 + 8 + (uVar10 & 0xffff) * 0xc) & 0xffff) * 0x20 +
                              0x14 + DAT_0087bc14);
            cVar7 = (char)puVar4[200];
            if (cVar7 == -1) {
              iVar12 = *(int *)(iVar14 + 0x4bc);
            }
            else {
              iVar12 = *(int *)(iVar14 + 0x4ac);
            }
            local_1c = 1.0;
            if ((cVar7 != -1) && (1 < *(short *)(iVar14 + 0x3da))) {
              local_1c = (float)(int)cVar7 / (float)(*(short *)(iVar14 + 0x3da) + -1);
            }
            if (iVar12 != -1) {
              FUN_00543dd0(local_1c);
            }
          }
        }
      }
    }
    if ((char)puVar4[0xa2] == '\x01') {
      local_10 = *(float *)(local_c + 0x26c);
    }
    else {
      local_10 = 1.0;
    }
    puVar1 = puVar4 + 0x8f;
    local_1c = local_10 * *(float *)(local_c + 0x264) * 0.033333335;
    local_28 = *puVar1;
    local_24 = puVar4[0x90];
    local_20 = puVar4[0x91];
    local_18 = local_10 * *(float *)(local_c + 0x268) * 0.0011111111;
    if ((local_1c == 0.0) && (local_18 == 0.0)) {
      *puVar1 = puVar4[0x8c];
      puVar4[0x90] = puVar4[0x8d];
      puVar4[0x91] = puVar4[0x8e];
      cVar7 = FUN_00562570();
      if (cVar7 != '\0') {
        FUN_005697a0(puVar1,1);
      }
      puVar6 = PTR_DAT_00696714;
      puVar4[0x92] = *(uint *)PTR_DAT_00696714;
      puVar4[0x93] = *(uint *)(puVar6 + 4);
      puVar4[0x94] = *(uint *)(puVar6 + 8);
      puVar16 = (undefined4 *)PTR_DAT_006966f8;
    }
    else if (*(char *)((int)puVar4 + 0x2b6) == '\0') {
      vector3d_rotate_toward_with_acceleration(puVar4 + 0x92,local_1c,local_18);
      puVar16 = (undefined4 *)PTR_DAT_006966f8;
    }
    else {
      object_get_orientation(local_48);
      vector3d_cross_product(local_48);
      puVar16 = (undefined4 *)PTR_DAT_006966f8;
      local_3c = *(undefined4 *)PTR_DAT_006966f8;
      local_38 = *(undefined4 *)(PTR_DAT_006966f8 + 4);
      local_34 = *(undefined4 *)(PTR_DAT_006966f8 + 8);
      FUN_00564ae0(puVar4 + 0x8f,puVar4 + 0x92,puVar4 + 0xae,local_1c,local_18);
    }
    iVar14 = local_c;
    if (*(float *)(local_c + 0x264) != 0.0) {
      vector3d_angle_between_4cd4f0();
    }
    uVar8 = __ftol();
    *(undefined1 *)((int)puVar4 + 0x323) = uVar8;
    local_18 = local_10 * *(float *)(iVar14 + 0x270) * 0.033333335;
    local_1c = local_10 * *(float *)(iVar14 + 0x274) * 0.0011111111;
    if ((local_18 == 0.0) && (local_1c == 0.0)) {
      puVar4[0x98] = puVar4[0x95];
      puVar4[0x99] = puVar4[0x96];
      puVar4[0x9a] = puVar4[0x97];
      FUN_005697a0(puVar4 + 0x98,0);
      puVar6 = PTR_DAT_00696714;
      puVar4[0x9b] = *(uint *)PTR_DAT_00696714;
      puVar4[0x9c] = *(uint *)(puVar6 + 4);
      puVar4[0x9d] = *(uint *)(puVar6 + 8);
    }
    else if (*(char *)((int)puVar4 + 0x2b7) == '\0') {
      vector3d_rotate_toward_with_acceleration(puVar4 + 0x9b,local_18,local_1c);
    }
    else {
      object_get_orientation(local_48);
      vector3d_cross_product(local_48);
      local_3c = *puVar16;
      local_38 = puVar16[1];
      local_34 = puVar16[2];
      FUN_00564ae0(puVar4 + 0x98,puVar4 + 0x9b,puVar4 + 0xb2,local_18,local_1c);
    }
    if (DAT_0071c419 == '\0') {
      uVar10 = puVar4[0x82] >> 0xd;
      switch(*(undefined1 *)((int)puVar4 + 0x28d)) {
      case 0:
        if ((uVar10 & 1) != 0) {
          unit_begin_throw_grenade(0);
        }
        break;
      case 1:
        if (1 < *(short *)((int)puVar4 + 0xd2)) {
          unit_throw_grenade_move_to_hand(param_1);
        }
        break;
      case 2:
        *(short *)((int)puVar4 + 0x28e) = *(short *)((int)puVar4 + 0x28e) + 1;
        if (*(char *)((int)puVar4 + 0x2a3) != '!') {
          unit_release_thrown_grenade(param_1,1);
        }
        break;
      case 3:
        if ((*(char *)((int)puVar4 + 0x2a3) != '!') && ((uVar10 & 1) == 0)) {
          *(byte *)((int)puVar4 + 0x28d) = (byte)uVar10 & 1;
        }
      }
    }
    if ((*(short *)((int)puVar4 + 0x2f2) != -1) && (DAT_0071c419 == '\0')) {
      local_1c = (float)puVar4[0xa1];
      uVar10 = 0;
      if (*(short *)((int)puVar4 + 0x2f2) == (short)puVar4[0xbd]) {
        if (((int)puVar4[0x84] < 1) || (local_5 = '\x01', (puVar4[0x85] & 0x800) == 0)) {
          local_5 = '\0';
        }
        if ((local_8 != '\0') && ((puVar4[0x82] & 0x10) != 0)) {
          uVar10 = 1;
        }
        if ((puVar4[0x82] & 0x800) != 0) {
          uVar10 = uVar10 | 2;
        }
        if ((puVar4[0x82] & 0x1000) != 0) {
          uVar10 = uVar10 | 4;
        }
        if ((*(uint *)(*(int *)((*puVar4 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14) + 0x17c) & 0x800000
            ) != 0) {
          unit_get_weapon_object_index(puVar4[0xd0]);
          FUN_004c2b20();
        }
        if ((puVar4[0x82] & 0x400) != 0) {
          uVar10 = uVar10 | 8;
        }
        cVar7 = FUN_00565c60();
        if ((cVar7 != '\0') && (local_5 == '\0')) {
          uVar10 = uVar10 | 0x10;
        }
        if (((short)puVar4[0x2d] == 0) && ('\0' < *(char *)((int)puVar4 + 0x505))) {
          uVar10 = uVar10 | 0x10;
        }
        if ((char)puVar4[200] != -1) {
          uVar10 = uVar10 | 0x40;
        }
      }
      else {
        uVar10 = 0x20;
      }
      item_set_permutation(uVar10,local_1c);
    }
  }
  iVar14 = local_c;
  if ((*(uint *)(local_c + 0x17c) & 0x800) == 0) {
    if ((puVar4[0xa6] & 2) != 0) {
      FUN_0056e820();
      puVar4[0xd9] = (uint)((float)puVar4[0xd9] * 0.7 + (float)puVar4[0xdc] * 0.3);
      puVar4[0xda] = (uint)((float)puVar4[0xda] * 0.7 + (float)puVar4[0xdd] * 0.3);
      puVar4[0xdb] = (uint)((float)puVar4[0xdb] * 0.7 + (float)puVar4[0xde] * 0.3);
    }
    sVar9 = 0;
    if (0 < *(int *)(iVar14 + 0x2cc)) {
      iVar12 = 0;
      do {
        iVar15 = iVar12 * 0x44 + *(int *)(iVar14 + 0x2d0);
        if (sVar9 == 0) {
          if ((puVar4[0xc9] == 0xffffffff) && ((puVar4[0x81] & 1) == 0)) {
LAB_0056339f:
            bVar5 = false;
          }
          else {
            bVar5 = true;
          }
        }
        else {
          if ((puVar4[0xca] == 0xffffffff) || (puVar4[0xca] == puVar4[0xc9])) goto LAB_0056339f;
          bVar5 = true;
        }
        if (((*(byte *)((int)puVar4 + 0x106) & 4) == 0) && (bVar5)) {
          if ((puVar4[iVar12 + 0xce] != 0x3f800000) &&
             (fVar2 = 1.0 / (*(float *)(iVar15 + 4) * 30.0) + (float)puVar4[iVar12 + 0xce],
             puVar4[iVar12 + 0xce] = (uint)fVar2, 1.0 < fVar2)) {
            puVar4[iVar12 + 0xce] = 0x3f800000;
          }
        }
        else if (((float)puVar4[iVar12 + 0xce] != 0.0) &&
                (fVar2 = (float)puVar4[iVar12 + 0xce] - 1.0 / (*(float *)(iVar15 + 8) * 30.0),
                puVar4[iVar12 + 0xce] = (uint)fVar2, fVar2 < 0.0)) {
          puVar4[iVar12 + 0xce] = 0;
        }
        sVar9 = sVar9 + 1;
        iVar12 = (int)sVar9;
      } while (iVar12 < *(int *)(iVar14 + 0x2cc));
    }
  }
  if ((0 < *(short *)((int)puVar4 + 0x406)) &&
     (sVar9 = *(short *)((int)puVar4 + 0x406) + -1, *(short *)((int)puVar4 + 0x406) = sVar9,
     sVar9 == 0)) {
    FUN_0042be40(param_1,puVar4[0x103],(short)puVar4[0x101],puVar4[0x102],0,1);
    *(undefined2 *)(puVar4 + 0x101) = 0;
    puVar4[0x103] = 0xffffffff;
    puVar4[0x102] = 0;
  }
  if ((((DAT_0071c419 == '\0') && (FUN_0056fc80(param_1), DAT_0071c419 == '\0')) &&
      (FUN_00561620(), DAT_0071c419 == '\0')) && ((local_6 != '\0' || (puVar4[0x86] != 0xffffffff)))
     ) {
    unit_calculate_luminosity();
  }
  if (*(char *)((int)puVar4 + 0x28b) == '\0') {
LAB_00563524:
    if (DAT_0071c419 != '\0') {
      return 1;
    }
  }
  else {
    if (DAT_0071c419 != '\0') {
      return 1;
    }
    cVar7 = *(char *)((int)puVar4 + 0x28b) + -1;
    *(char *)((int)puVar4 + 0x28b) = cVar7;
    if (cVar7 == '\0') {
      FUN_00570720(param_1);
      goto LAB_00563524;
    }
  }
  fVar2 = -(float)puVar4[0xba];
  if (-0.1 <= fVar2) {
    if (0.1 < fVar2) {
      fVar2 = 0.1;
    }
  }
  else {
    fVar2 = -0.1;
  }
  puVar4[0xba] = (uint)(fVar2 + (float)puVar4[0xba]);
  uVar10 = puVar4[0x81];
  cVar7 = local_7;
  if ((uVar10 & 0x10000000) != 0) {
    cVar7 = '\x01';
    if ((uVar10 & 0x80000) != 0) {
      cVar7 = local_7;
    }
    puVar4[0x81] = uVar10 & 0xefffffff;
  }
  uVar10 = puVar4[0x81];
  if ((uVar10 & 0x20000000) != 0) {
    if ((uVar10 & 0x80000) != 0) {
      cVar7 = '\x01';
    }
    puVar4[0x81] = uVar10 & 0xdfffffff;
  }
  uVar10 = puVar4[0x82];
  if ((((uVar10 & 0x10) != 0) || ((float)puVar4[0xd1] < 0.0 != ((float)puVar4[0xd1] == 0.0))) ||
     (cVar7 != '\0')) {
    if (local_8 == '\0') {
      if ((puVar4[0x81] & 0x4000000) != 0) {
        puVar4[0x81] = puVar4[0x81] & 0xfbffffff;
      }
      if ((puVar4[0x81] & 0x80000) == 0) goto LAB_005636c8;
      uVar10 = puVar4[0x81] & 0xfff7ffff | 0x10;
    }
    else {
      cVar7 = FUN_00565b60();
      if (cVar7 != '\0') {
        if ((uVar10 & 0x10) != 0) {
          if ((puVar4[0x81] & 0x4000000) == 0) {
            iVar14 = *(int *)(*(int *)(DAT_00746fa0 + 0x180) + 0x54);
          }
          else {
            iVar14 = *(int *)(*(int *)(DAT_00746fa0 + 0x180) + 100);
          }
          if (iVar14 != -1) {
            FUN_004507a0(param_1,0xffffffff,0,0,0,0);
          }
          puVar4[0x81] = puVar4[0x81] ^ 0x4000000;
        }
        if ((puVar4[0x82] & 0x10) != 0) goto LAB_005636c8;
      }
      if ((((puVar4[0x81] & 0x80000) == 0) && ((float)puVar4[0xd1] <= 0.2)) ||
         (puVar4[0x47] != 0xffffffff)) goto LAB_005636c8;
      FUN_004507a0(param_1,0xffffffff,0,0,0,0);
      uVar10 = puVar4[0x81] ^ 0x80000;
    }
    puVar4[0x81] = uVar10;
  }
LAB_005636c8:
  if ((puVar4[0x81] & 0x80000) == 0) {
    if ((float)puVar4[0xd1] < 1.0) {
      puVar4[0xd1] = (uint)((float)puVar4[0xd1] + 0.0011111111);
    }
    if (((float)puVar4[0xd0] != 0.0) &&
       (fVar2 = (float)puVar4[0xd0], puVar4[0xd0] = (uint)(fVar2 - 0.041666668),
       fVar2 - 0.041666668 < 0.0)) {
      puVar4[0xd0] = 0;
    }
  }
  else {
    if ((*(uint *)(local_c + 0x17c) & 0x1000000) == 0) {
      puVar4[0xd1] = (uint)((float)puVar4[0xd1] - 0.00027777778);
    }
    if ((puVar4[0x47] != 0xffffffff) || ((*(byte *)((int)puVar4 + 0x106) & 4) != 0)) {
      puVar4[0x81] = puVar4[0x81] & 0xfff7ffff;
    }
    if ((puVar4[0xd0] != 0x3f800000) &&
       (fVar2 = (float)puVar4[0xd0], puVar4[0xd0] = (uint)(fVar2 + 0.16666667),
       1.0 < fVar2 + 0.16666667)) {
      puVar4[0xd0] = 0x3f800000;
    }
  }
  cVar7 = FUN_00565b60();
  if (cVar7 != '\0') {
    if ((puVar4[0x81] & 0x4000000) == 0) {
      if (((float)puVar4[0xd2] != 0.0) &&
         (fVar2 = (float)puVar4[0xd2], puVar4[0xd2] = (uint)(fVar2 - 0.041666668),
         fVar2 - 0.041666668 < 0.0)) {
        puVar4[0xd2] = 0;
      }
    }
    else if ((puVar4[0xd2] != 0x3f800000) &&
            (fVar2 = (float)puVar4[0xd2], puVar4[0xd2] = (uint)(fVar2 + 0.083333336),
            1.0 < fVar2 + 0.083333336)) {
      puVar4[0xd2] = 0x3f800000;
      return 1;
    }
  }
  return 1;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
