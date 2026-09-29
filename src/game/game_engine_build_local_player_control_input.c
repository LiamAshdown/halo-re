// game_engine_build_local_player_control_input  (Ghidra: FUN_004710b0; renamed, no established name)
// address 0x4710b0, size 2595 bytes
// name confidence: 0.5   rewrite confidence: 0.55
// evidence: types/game.h names this exact function in the header comments of
// player_control_input ("game_engine_build_local_player_control_input (0x4710b0) zeroes all eight
// dwords and fills them, then tail-calls game_engine_digitize_control_input (0x472760) with this
// record in EDX"), local_player_input_state (whose 0x28 stride is this function's own address
// arithmetic) and local_player_control (look_acceleration_timer +0x34, the aim-assist tracker
// block 0x28/0x2c/0x30, suppressed_buttons +0x08 / suppressed_until_released +0x0a).
// out/phase4/game_functions.md lists it only as an unnamed 2595-byte function; the name here is
// the one types/game.h already commits to.
// register convention: all three parameters are ordinary stack parameters (cdecl); the two
// register-passed callees get EAX = local_player_index (camera_observer_get_target_angles) and
// SI = local_player_index (camera_observer_get_target_id), and the tail call passes the finished
// record in EDX.
//   // blam-cc: stack -> local_player_index, delta_time, out
//
// Reconstructed against the disassembly (objdump -M intel --start-address=0x4710b0
// --stop-address=0x471ae0 bin/halo.exe), because Ghidra's decompile is unusable in four places:
//   - the network-mode axis quantization shows as extraout_EDX / extraout_ECX; the real code is
//     input->throttle_x = control_axis_sign(input->throttle_x) (and the same for throttle_y),
//     writing back into local_player_input_state, and ECX still holds local_player_index
//     afterwards (0x471070 is a leaf that does not touch it), which is what indexes the two
//     look-rate settings at 0x006f1d74 / 0x006f1d78;
//   - Ghidra elides the EAX/SI local-player-index argument of both camera_observer_ calls
//     (objdump 0x471540 "mov eax,[esp+0x68]" and 0x4717f2 "mov esi,[esp+0x60]", both the
//     incoming local_player_index);
//   - Ghidra elides the EDI response-curve table and the EAX unit index of
//     unit_get_active_weapon_scale (objdump 0x4713f1 "mov edi,[eax+0x78]", i.e.
//     GlobalsPlayerControl::look_function.pointer, and 0x471435..0x47144b, where EAX is
//     player->unit and only the pushed word is the zoom level);
//   - the look-rate multiplier "(bVar7 + 1)" is computed twice, identically, from 0x006f1d80,
//     0x006f1d7f and the zoom button's hold count; it is computed once here.
//
// UNSURE: unit_get_active_weapon_scale (0x565ab0, units module) really takes the unit index in
// EAX and the zoom level as its one stack argument, which it forwards in EDX to 0x4c2d70;
// src/units/unit_get_active_weapon_scale.c models that stack argument as the unit index instead.
// The declaration below keeps that file's parameter type so the two agree, but the value passed is
// the zoom level, which is what the disassembly shows. Flagged for hook verification.
// UNSURE: camera_observer_get_target_id's stack parameter is really an out-weight (it is given
// &local_player_control::nameplate_weight here) and its return value is the target handle; the
// declaration in src/game/camera_observer_get_target_id.c calls that parameter out_id. The cast
// below preserves the real call shape without changing that file's prototype.
// UNSURE: 0x006f1d7d / 0x006f1d7f / 0x006f1d80 are three option bytes with no attesting header.
// 0x006f1d7d gates the whole magnetism / aim-assist block; 0x006f1d80 is a 0-or-1 look-rate
// doubler and 0x006f1d7f makes holding the zoom button (digital button 11) invert it.
// UNSURE: 0x006b0b80 is declared with the name the other game-module files that touch it use
// (main_game_globals); its byte +2 is read here exactly as game_effects_update.c reads it, as
// a slow-motion flag, and it halves the magnetism rate.
// UNSURE: object + 0x22c bit 0 is weapon_data::flags bit 0 (types/items.h); what that flag means
// is not established, only that setting it suppresses the primary trigger on a network client.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"
#include "items.h"
#include "units.h"
#include "game.h"

extern player_globals *local_player_globals;                 // 0x0087a478
extern data_array *player_data;                              // 0x0087a480
extern data_array *object_data;                           // 0x008603b0
extern tag_instance *tag_instances;                          // 0x0087bc14
extern player_control_globals *player_control_globals_ptr;   // 0x006b145c
extern game_time_globals *game_time;                         // 0x006f1d6c
extern Globals *global_globals;                              // 0x00746fa0
extern int16_t network_game_mode;                            // 0x00719720 (tested as a word here)
extern game_main_globals *main_game_globals; // 0x006b0b80
extern local_player_input_state local_player_input_states[k_maximum_local_players]; // 0x00712498

extern real look_yaw_rate_setting[k_maximum_local_players];   // 0x006f1d74, degrees per second
extern real look_pitch_rate_setting[k_maximum_local_players]; // 0x006f1d78, degrees per second
extern uint8_t look_aim_assist_enabled;                       // 0x006f1d7d, UNSURE (see header)
extern uint8_t look_rate_doubler_zoom_inverts;                // 0x006f1d7f, UNSURE (see header)
extern uint8_t look_rate_doubler_enabled;                     // 0x006f1d80, UNSURE (see header)

extern real control_axis_sign(real value);                                    // this batch, 0x471070
extern real response_curve_evaluate(int16_t table_count, real x, real *table); // this batch, 0x470fb0
                                                                              // blam-cc: EDI -> table
extern real game_engine_get_time_scale(void);                                 // this batch, 0x470ce0
extern void game_engine_digitize_control_input(player_control_input *input);               // this batch, 0x472760
                                                                              // blam-cc: EDX -> input
extern uint32_t camera_observer_get_target_angles(real *out_weight_primary,
    real *out_weight_secondary, real *out_yaw_pitch, real *out_yaw_pitch_rate,
    int16_t local_player_slot);                                              // this batch, 0x4596f0
extern uint32_t camera_observer_get_target_id(datum_index *out_id, int16_t local_player_slot);
                                                                              // this batch, 0x459900
extern float unit_get_active_weapon_scale(uint32_t unit_index, int16_t zoom_level); // 0x565ab0, EAX, stack
                                                              // blam-cc: EAX -> unit_index

extern double sqrt(double x); // x87 FSQRT

// 0x00672c38 and 0x00672acc, the two constants every look rate is scaled by: degrees to radians,
// and one tick of the 30 tick/second clock.
#define k_degrees_to_radians 0.017453292f
#define k_seconds_per_tick   0.033333335f
#define k_look_epsilon       9.999999747378752e-05f

static real control_input_absolute(real value)
{
    return value < 0.0f ? -value : value;
}

// blam-cc: stack -> local_player_index, delta_time, out
// Builds one local player's player_control_input for this tick: copies the movement throttle,
// turns the look stick (or mouse) into a yaw/pitch delta through the response curve, the look
// acceleration ramp and the aim-assist magnetism, digitizes the 19 button hold counts into
// control_flags / button_flags while honouring local_player_control::suppressed_buttons, and then
// tail-calls game_engine_digitize_control_input on the finished record.
void game_engine_build_local_player_control_input(int16_t local_player_index, real delta_time,
                                                  player_control_input *out)
{
    datum_index player_index = (datum_index)-1;
    local_player_control *control;
    player *plr;
    local_player_input_state *input;
    GlobalsPlayerControl *player_control;
    GlobalsPlayerInformation *player_information;
    real yaw_rate;   // radians per tick one full axis deflection is worth
    real pitch_rate;
    real look_x;     // the look stick after square-to-circle scaling, clamped to [-1, 1]
    real look_y;
    real yaw_delta = 0.0f;
    real pitch_delta = 0.0f;
    real abs_look_x;
    int8_t buttons[0x13];
    int32_t i;

    if (local_player_index != -1 && local_player_index < k_maximum_local_players) {
        player_index = local_player_globals->local_players[local_player_index];
    }

    out->throttle_x = 0.0f;
    out->throttle_y = 0.0f;
    out->primary_trigger = 0.0f;
    out->yaw_delta = 0.0f;
    out->pitch_delta = 0.0f;
    out->action = 0;
    out->melee = 0;
    out->pad_16 = 0;
    out->control_flags = 0;
    out->button_flags = 0;

    if (player_index == (datum_index)-1) {
        game_engine_digitize_control_input(out); // blam-cc: EDX -> out
        return;
    }

    control = &player_control_globals_ptr->local_players[local_player_index];
    plr = (player *)((uint8_t *)player_data->data + (player_index & 0xffff) * k_player_size);
    player_control = (GlobalsPlayerControl *)global_globals->player_control.pointer;
    player_information = (GlobalsPlayerInformation *)global_globals->player_information.pointer;
    input = &local_player_input_states[plr->local_player_index];

    // In any networked game the movement stick is quantized to -1 / 0 / +1 in place, so that the
    // client and the server always replay the same throttle.
    if (network_game_mode != 0) {
        input->throttle_x = control_axis_sign(input->throttle_x);
        input->throttle_y = control_axis_sign(input->throttle_y);
    }

    yaw_rate = 0.0f;
    pitch_rate = 0.0f;
    if (plr->unit != (datum_index)-1) {
        object *unit_object = ((object_header *)object_data->data)[plr->unit & 0xffff].data;
        unit_data *unit = (unit_data *)((uint8_t *)unit_object + k_unit_data_offset);

        yaw_rate = look_yaw_rate_setting[local_player_index] *
                   k_degrees_to_radians * k_seconds_per_tick;
        pitch_rate = look_pitch_rate_setting[local_player_index] *
                     k_degrees_to_radians * k_seconds_per_tick;

        // A unit riding a vehicle seat inherits that seat's own turn rates when they are set.
        if (unit_object->parent_object != (datum_index)-1 && unit->vehicle_seat_index != -1) {
            object *parent =
                ((object_header *)object_data->data)[unit_object->parent_object & 0xffff].data;
            Unit *parent_definition =
                (Unit *)tag_instances[parent->definition_tag & 0xffff].data;
            UnitSeat *seat =
                &((UnitSeat *)parent_definition->seats.pointer)[unit->vehicle_seat_index];

            if (seat->yaw_rate > 0.0f) {
                yaw_rate = seat->yaw_rate * k_degrees_to_radians * k_seconds_per_tick;
            }
            if (seat->pitch_rate > 0.0f) {
                pitch_rate = seat->pitch_rate * k_degrees_to_radians * k_seconds_per_tick;
            }
        }
    }

    out->throttle_x = input->throttle_x;
    out->throttle_y = input->throttle_y;

    // Square-to-circle: a stick pushed into a corner is worth sqrt(1 + (min/max)^2) of one axis.
    {
        real abs_y = control_input_absolute(input->look_y);
        real abs_x = control_input_absolute(input->look_x);
        real scale = 1.0f;

        if (abs_y > 0.1f && abs_x > 0.1f) {
            real a, b;

            if (abs_y <= abs_x) {
                a = abs_y / abs_x;
                b = 1.0f;
            } else {
                b = abs_x / abs_y;
                a = 1.0f;
            }
            scale = (real)sqrt((double)(a * a + b * b));
        }

        look_x = scale * input->look_x;
        if (look_x < -1.0f) { look_x = -1.0f; } else if (look_x > 1.0f) { look_x = 1.0f; }
        look_y = scale * input->look_y;
        if (look_y < -1.0f) { look_y = -1.0f; } else if (look_y > 1.0f) { look_y = 1.0f; }
    }

    if ((player_control_globals_ptr->flags & 1) == 0 && game_time->paused == 0) {
        if (input->look_is_analog == 0) {
            // Mouse / digital look: the raw axes already are radians per tick, only scaled by the
            // zoomed weapon's magnification and by the stun turning penalty.
            real scale = 1.0f;

            out->throttle_x = input->throttle_x;
            out->throttle_y = input->throttle_y;
            if (plr->unit != (datum_index)-1 && control->desired_zoom_level != -1) {
                // blam-cc: EAX -> plr->unit
                scale = 1.0f / unit_get_active_weapon_scale(plr->unit, control->desired_zoom_level);
            }
            if (plr->unit != (datum_index)-1) {
                unit_data *unit = (unit_data *)((uint8_t *)
                    ((object_header *)object_data->data)[plr->unit & 0xffff].data +
                    k_unit_data_offset);

                scale = (1.0f - unit->stun * player_information->stun_turning_penalty) * scale;
            }
            out->yaw_delta = scale * input->look_x;
            out->pitch_delta = scale * input->look_y;
            if (out->yaw_delta < -4.5f) { out->yaw_delta = -4.5f; }
            else if (out->yaw_delta > 4.5f) { out->yaw_delta = 4.5f; }
            if (out->pitch_delta < -2.3f) { out->pitch_delta = -2.3f; }
            else if (out->pitch_delta > 2.3f) { out->pitch_delta = 2.3f; }
            // See the header: the stack parameter is the out-weight, the return is the handle.
            control->nameplate_target = camera_observer_get_target_id(
                (datum_index *)&control->nameplate_weight, local_player_index);
        } else {
            // Analog look: response curve, look acceleration ramp, then aim-assist magnetism.
            real look_yaw_pitch[2];
            real look_yaw_pitch_rate[2];
            real rate_multiplier;
            int32_t doubler;

            doubler = (input->buttons[0x0b] != 0 && look_rate_doubler_zoom_inverts != 0)
                          ? (look_rate_doubler_enabled == 0 ? 1 : 0)
                          : (int32_t)look_rate_doubler_enabled;
            rate_multiplier = (real)(doubler + 1);

            yaw_delta = rate_multiplier * yaw_rate *
                response_curve_evaluate((int16_t)player_control->look_function.count, look_x,
                                        (real *)player_control->look_function.pointer);
            pitch_delta = rate_multiplier * pitch_rate *
                response_curve_evaluate((int16_t)player_control->look_function.count, look_y,
                                        (real *)player_control->look_function.pointer);

            if (plr->unit != (datum_index)-1 && control->desired_zoom_level != -1) {
                // blam-cc: EAX -> plr->unit
                real inverse_scale =
                    1.0f / unit_get_active_weapon_scale(plr->unit, control->desired_zoom_level);

                yaw_delta = yaw_delta * inverse_scale;
                pitch_delta = inverse_scale * pitch_delta;
            }
            if (plr->unit != (datum_index)-1) {
                unit_data *unit = (unit_data *)((uint8_t *)
                    ((object_header *)object_data->data)[plr->unit & 0xffff].data +
                    k_unit_data_offset);
                real stun_scale =
                    1.0f - unit->stun * player_information->stun_turning_penalty;

                yaw_delta = yaw_delta * stun_scale;
                pitch_delta = stun_scale * pitch_delta;
            }

            // Look acceleration: while the stick is held past look_peg_threshold the yaw (only)
            // ramps from 1.0 up to look_acceleration_scale over look_acceleration_time seconds.
            abs_look_x = control_input_absolute(look_x);
            if (abs_look_x < player_control->look_peg_threshold) {
                control->look_acceleration_timer = 0.0f;
            } else {
                real fraction =
                    control->look_acceleration_timer / player_control->look_acceleration_time;

                if (fraction < 0.0f) { fraction = 0.0f; } else if (fraction > 1.0f) { fraction = 1.0f; }
                yaw_delta = ((player_control->look_acceleration_scale - 1.0f) * fraction + 1.0f) *
                            yaw_delta;
                control->look_acceleration_timer = delta_time + control->look_acceleration_timer;
            }

            control->nameplate_target = camera_observer_get_target_angles(
                &control->nameplate_weight, &control->aim_assist_weight,
                look_yaw_pitch, look_yaw_pitch_rate, local_player_index);

            if (look_aim_assist_enabled != 0 && control->aim_assist_weight > 0.0f &&
                (abs_look_x > k_look_epsilon ||
                 control_input_absolute(look_y) > k_look_epsilon ||
                 control_input_absolute(out->throttle_x) > k_look_epsilon ||
                 control_input_absolute(out->throttle_y) > k_look_epsilon)) {
                real time_scale = game_engine_get_time_scale();
                real friction = player_control->magnetism_friction;
                real adhesion = player_control->magnetism_adhesion;
                real keep;
                real magnetism_yaw;
                real magnetism_pitch;

                if (friction < 0.0f) { friction = 0.0f; } else if (friction > 1.0f) { friction = 1.0f; }
                keep = 1.0f - friction * control->aim_assist_weight;
                if (adhesion < 0.0f) { adhesion = 0.0f; } else if (adhesion > 1.0f) { adhesion = 1.0f; }
                adhesion = adhesion * control->aim_assist_weight;

                if (main_game_globals->players_are_double_speed != 0) {
                    time_scale = time_scale * 0.5f;
                }
                magnetism_yaw = look_yaw_pitch_rate[0] * time_scale;
                magnetism_pitch = time_scale * look_yaw_pitch_rate[1];
                if (magnetism_yaw < -0.10471976f) { magnetism_yaw = -0.10471976f; }
                else if (magnetism_yaw > 0.10471976f) { magnetism_yaw = 0.10471976f; }
                if (magnetism_pitch < -0.05235988f) { magnetism_pitch = -0.05235988f; }
                else if (magnetism_pitch > 0.05235988f) { magnetism_pitch = 0.05235988f; }

                yaw_delta = keep * yaw_delta + magnetism_yaw * adhesion;
                pitch_delta = keep * pitch_delta + magnetism_pitch * adhesion;
            }

            out->yaw_delta = yaw_delta * delta_time * 30.0f;
            out->pitch_delta = delta_time * 30.0f * pitch_delta;
        }
    } else {
        out->yaw_delta = 0.0f;
        out->pitch_delta = 0.0f;
    }

    // Digital buttons. A suppressed button reads as released; a button in
    // suppressed_until_released stops being suppressed the moment it physically reads zero.
    // Both masks are 16 bit, so buttons 16..18 can never actually be suppressed even though the
    // loop counts to 19.
    for (i = 0; i < 0x13; i += 1) {
        buttons[i] = 0;
    }
    {
        uint16_t both = (uint16_t)(control->suppressed_until_released & control->suppressed_buttons);

        if (both != 0) {
            for (i = 0; i < 0x13; i += 1) {
                if ((both & (uint32_t)(1 << i)) != 0 && input->buttons[i] == 0) {
                    uint16_t clear = (uint16_t)~(uint32_t)(1 << i);

                    control->suppressed_buttons = (uint16_t)(control->suppressed_buttons & clear);
                    control->suppressed_until_released =
                        (uint16_t)(control->suppressed_until_released & clear);
                }
            }
        }
    }
    for (i = 0; i < 0x13; i += 1) {
        if ((control->suppressed_buttons & (uint32_t)(1 << i)) == 0) {
            buttons[i] = input->buttons[i];
        }
    }

    if (buttons[0x0a] == 0) { out->control_flags &= ~0x1u; } else { out->control_flags |= 0x1u; }
    if (buttons[0x00] == 0) { out->control_flags &= ~0x2u; } else { out->control_flags |= 0x2u; }
    if (buttons[0x02] == 0) { out->control_flags &= ~0x40u; } else { out->control_flags |= 0x40u; }
    if (buttons[0x05] == 0) { out->control_flags &= ~0x10u; } else { out->control_flags |= 0x10u; }
    if (buttons[0x0d] == 0) { out->control_flags &= ~0x400u; } else { out->control_flags |= 0x400u; }
    if (buttons[0x07] == 0) { out->control_flags &= ~0x800u; } else { out->control_flags |= 0x800u; }
    // Button 6 sets and clears 0x1000 and 0x2000 together (objdump 0x47193a / 0x471949).
    if (buttons[0x06] == 0) { out->control_flags &= ~0x3000u; } else { out->control_flags |= 0x3000u; }
    // The weapon-swap bit needs either button 14 down, or button 2 held at least
    // minimum_weapon_swap_ticks ticks. The hold count is compared zero-extended
    // (objdump 0x471961 "movzx eax,cl"), not sign-extended.
    if (buttons[0x0e] == 0 &&
        (int16_t)(uint16_t)(uint8_t)buttons[0x02] < player_control->minimum_weapon_swap_ticks) {
        out->control_flags &= ~0x4000u;
    } else {
        out->control_flags |= 0x4000u;
    }
    if (buttons[0x04] == 0) { out->control_flags &= ~0x80u; } else { out->control_flags |= 0x80u; }
    if (buttons[0x0b] == 0) { out->button_flags &= ~0x4u; } else { out->button_flags |= 0x4u; }
    if (buttons[0x03] == 0) { out->button_flags &= ~0x1u; } else { out->button_flags |= 0x1u; }
    if (buttons[0x01] == 0) { out->button_flags &= ~0x2u; } else { out->button_flags |= 0x2u; }

    out->primary_trigger = (out->control_flags & 0x800u) != 0 ? 1.0f : 0.0f;

    // These two go through unsuppressed: the raw hold counts, not the masked copies.
    if ((control->suppressed_buttons & 0x200u) == 0) {
        out->melee = input->buttons[0x09];
    }
    if ((control->suppressed_buttons & 0x004u) == 0) {
        out->action = input->buttons[0x02];
    }

    // A network client does not get to fire a weapon whose weapon_data::flags bit 0 is set.
    if (network_game_mode == 1 && (out->control_flags & 0x800u) != 0 &&
        plr->unit != (datum_index)-1) {
        unit_data *unit = (unit_data *)((uint8_t *)
            ((object_header *)object_data->data)[plr->unit & 0xffff].data + k_unit_data_offset);

        if (unit->current_weapon_index != -1) {
            object *weapon_object = ((object_header *)object_data->data)
                [unit->weapons[unit->current_weapon_index] & 0xffff].data;
            weapon_data *weapon =
                (weapon_data *)((uint8_t *)weapon_object + k_item_extension_offset);

            if ((weapon->flags & 1) != 0) {
                out->control_flags &= ~0x800u;
            }
        }
    }

    {
        real magnitude = out->throttle_x * out->throttle_x + out->throttle_y * out->throttle_y;

        if (magnitude > 1.0f) {
            real inverse = 1.0f / (real)sqrt((double)magnitude);

            out->throttle_x = inverse * out->throttle_x;
            out->throttle_y = inverse * out->throttle_y;
        }
    }
    game_engine_digitize_control_input(out); // blam-cc: EDX -> out
}

#if 0
Original Ghidra decompilation (0x4710b0), from tools/pack.py 0x4710b0.
Kept verbatim; see the header for the four places it is wrong or incomplete.

void FUN_004710b0(short param_1,float param_2,float *param_3)

{
  float *pfVar1;
  int iVar2;
  float fVar3;
  float fVar4;
  short sVar5;
  undefined2 uVar6;
  byte bVar7;
  float *pfVar8;
  ushort uVar9;
  ushort uVar10;
  int iVar11;
  undefined4 uVar12;
  uint uVar13;
  int iVar14;
  int extraout_ECX;
  int iVar15;
  int iVar16;
  int extraout_EDX;
  int extraout_EDX_00;
  int iVar17;
  float10 fVar18;
  float fVar19;
  float local_4c;
  float local_44;
  float local_40 [2];
  double local_38;
  int local_30;
  float *local_2c;
  int local_28;
  float local_24;
  float local_20;
  float local_1c;
  float local_18;
  char local_14 [20];

  if ((param_1 == -1) || (0 < param_1)) {
    uVar13 = 0xffffffff;
  }
  else {
    uVar13 = *(uint *)(DAT_0087a478 + 4 + param_1 * 4);
  }
  *param_3 = 0.0;
  param_3[1] = 0.0;
  param_3[2] = 0.0;
  param_3[3] = 0.0;
  param_3[4] = 0.0;
  param_3[5] = 0.0;
  param_3[6] = 0.0;
  param_3[7] = 0.0;
  if (uVar13 != 0xffffffff) {
    iVar14 = (int)param_1;
    iVar17 = iVar14 * 0x40 + 0x10 + DAT_006b145c;
    iVar11 = (uVar13 & 0xffff) * 0x200 + *(int *)(DAT_0087a480 + 0x34);
    local_2c = *(float **)(DAT_00746fa0 + 0x114);
    iVar2 = *(short *)(iVar11 + 2) * 0x28;
    local_30 = iVar17;
    local_28 = iVar11;
    if (DAT_00719720 != 0) {
      fVar18 = (float10)FUN_00471070(*(undefined4 *)(&DAT_007124ac + iVar2));
      *(float *)(extraout_EDX + 0x14) = (float)fVar18;
      fVar18 = (float10)FUN_00471070(*(undefined4 *)(extraout_EDX + 0x18));
      *(float *)(extraout_EDX_00 + 0x18) = (float)fVar18;
      iVar14 = extraout_ECX;
    }
    local_40[0] = 0.0;
    uVar12 = (undefined4)((ulonglong)local_38 >> 0x20);
    local_38 = (double)((ulonglong)local_38 & 0xffffffff00000000);
    if (*(uint *)(iVar11 + 0x34) != 0xffffffff) {
      iVar15 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 +
                       (*(uint *)(iVar11 + 0x34) & 0xffff) * 0xc);
      uVar13 = *(uint *)(iVar15 + 0x11c);
      local_40[0] = *(float *)(&DAT_006f1d74 + iVar14 * 4) * 0.017453292 * 0.033333335;
      local_38 = (double)CONCAT44(uVar12,*(float *)(&DAT_006f1d78 + iVar14 * 4) * 0.017453292 *
                                         0.033333335);
      if ((uVar13 != 0xffffffff) && (sVar5 = *(short *)(iVar15 + 0x2f0), sVar5 != -1)) {
        iVar14 = *(int *)(*(int *)((**(uint **)(*(int *)(DAT_008603b0 + 0x34) + 8 +
                                               (uVar13 & 0xffff) * 0xc) & 0xffff) * 0x20 + 0x14 +
                                  DAT_0087bc14) + 0x2e8);
        iVar15 = sVar5 * 0x11c;
        iVar16 = iVar15 + iVar14;
        if (0.0 < *(float *)(iVar15 + 0x7c + iVar14)) {
          local_40[0] = *(float *)(iVar16 + 0x7c) * 0.017453292 * 0.033333335;
        }
        if (0.0 < *(float *)(iVar16 + 0x80)) {
          local_38 = (double)CONCAT44(uVar12,*(float *)(iVar16 + 0x80) * 0.017453292 * 0.033333335);
        }
      }
    }
    *param_3 = *(float *)(&DAT_007124ac + iVar2);
    param_3[1] = *(float *)(&DAT_007124b0 + iVar2);
    fVar19 = ABS(*(float *)(&DAT_007124b8 + iVar2));
    fVar3 = ABS(*(float *)(&DAT_007124b4 + iVar2));
    local_4c = 1.0;
    if ((0.1 < fVar19) && (local_4c = 1.0, 0.1 < fVar3)) {
      if (fVar19 <= fVar3) {
        fVar19 = fVar19 / fVar3;
        local_4c = 1.0;
      }
      else {
        local_4c = fVar3 / fVar19;
        fVar19 = 1.0;
      }
      local_4c = SQRT(fVar19 * fVar19 + local_4c * local_4c);
    }
    local_44 = local_4c * *(float *)(&DAT_007124b4 + iVar2);
    if (-1.0 <= local_44) {
      if (1.0 < local_44) {
        local_44 = 1.0;
      }
    }
    else {
      local_44 = -1.0;
    }
    local_4c = local_4c * *(float *)(&DAT_007124b8 + iVar2);
    if (-1.0 <= local_4c) {
      if (1.0 < local_4c) {
        local_4c = 1.0;
      }
    }
    else {
      local_4c = -1.0;
    }
    if (((*(byte *)(DAT_006b145c + 0xc) & 1) == 0) && (*(char *)(DAT_006f1d6c + 2) == '\0')) {
      if ((&DAT_007124bc)[iVar2] == '\0') {
        fVar18 = (float10)1.0;
        *param_3 = *(float *)(&DAT_007124ac + iVar2);
        param_3[1] = *(float *)(&DAT_007124b0 + iVar2);
        if ((*(int *)(iVar11 + 0x34) != -1) && (*(short *)(iVar17 + 0x24) != -1)) {
          fVar18 = (float10)FUN_00565ab0(*(short *)(iVar17 + 0x24));
          fVar18 = (float10)1.0 / fVar18;
        }
        if (*(uint *)(iVar11 + 0x34) != 0xffffffff) {
          fVar18 = ((float10)1.0 -
                   (float10)*(float *)(*(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 +
                                               (*(uint *)(iVar11 + 0x34) & 0xffff) * 0xc) + 0x424) *
                   (float10)*(float *)(*(int *)(DAT_00746fa0 + 0x174) + 0x84)) * fVar18;
        }
        param_3[3] = (float)(fVar18 * (float10)*(float *)(&DAT_007124b4 + iVar2));
        param_3[4] = (float)(fVar18 * (float10)*(float *)(&DAT_007124b8 + iVar2));
        if (-4.5 <= param_3[3]) {
          if (param_3[3] <= 4.5) {
            fVar19 = param_3[3];
          }
          else {
            fVar19 = 4.5;
          }
        }
        else {
          fVar19 = -4.5;
        }
        param_3[3] = fVar19;
        if (-2.3 <= param_3[4]) {
          if (param_3[4] <= 2.3) {
            fVar19 = param_3[4];
          }
          else {
            fVar19 = 2.3;
          }
        }
        else {
          fVar19 = -2.3;
        }
        param_3[4] = fVar19;
        uVar12 = FUN_00459900(iVar17 + 0x2c);
        *(undefined4 *)(iVar17 + 0x28) = uVar12;
      }
      else {
        bVar7 = DAT_006f1d80;
        if (((&DAT_007124a3)[iVar2] != '\0') && (DAT_006f1d7f != '\0')) {
          bVar7 = DAT_006f1d80 == 0;
        }
        local_24 = (float)(bVar7 + 1);
        local_1c = (float)(int)local_24;
        bVar7 = DAT_006f1d80;
        if (((&DAT_007124a3)[iVar2] != '\0') && (DAT_006f1d7f != '\0')) {
          bVar7 = DAT_006f1d80 == 0;
        }
        uVar6 = *(undefined2 *)(local_2c + 0x1d);
        fVar18 = (float10)FUN_00470fb0(uVar6,local_44);
        local_24 = (float)((float10)(bVar7 + 1) * fVar18 * (float10)local_40[0]);
        fVar18 = (float10)FUN_00470fb0(uVar6,local_4c);
        iVar17 = local_30;
        local_20 = (float)(fVar18 * (float10)local_1c * (float10)local_38._0_4_);
        if ((*(int *)(local_28 + 0x34) != -1) && (*(short *)(local_30 + 0x24) != -1)) {
          fVar18 = (float10)FUN_00565ab0(*(short *)(local_30 + 0x24));
          local_24 = (float)((float10)local_24 * ((float10)1.0 / fVar18));
          local_20 = (float)(((float10)1.0 / fVar18) * (float10)local_20);
        }
        pfVar8 = local_2c;
        if (*(uint *)(local_28 + 0x34) != 0xffffffff) {
          fVar19 = 1.0 - *(float *)(*(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 +
                                            (*(uint *)(local_28 + 0x34) & 0xffff) * 0xc) + 0x424) *
                         *(float *)(*(int *)(DAT_00746fa0 + 0x174) + 0x84);
          local_24 = local_24 * fVar19;
          local_20 = fVar19 * local_20;
        }
        local_38 = (double)ABS(local_44);
        if (ABS(local_44) < local_2c[0x12]) {
          *(undefined4 *)(iVar17 + 0x34) = 0;
        }
        else {
          fVar19 = *(float *)(iVar17 + 0x34) / local_2c[0x10];
          if (0.0 <= fVar19) {
            if (1.0 < fVar19) {
              fVar19 = 1.0;
            }
          }
          else {
            fVar19 = 0.0;
          }
          local_24 = ((local_2c[0x11] - 1.0) * fVar19 + 1.0) * local_24;
          *(float *)(iVar17 + 0x34) = param_2 + *(float *)(iVar17 + 0x34);
        }
        pfVar1 = (float *)(iVar17 + 0x30);
        uVar12 = FUN_004596f0(iVar17 + 0x2c,pfVar1,local_40,&local_1c);
        *(undefined4 *)(iVar17 + 0x28) = uVar12;
        if (((DAT_006f1d7d != '\0') && (0.0 < *pfVar1)) &&
           ((9.999999747378752e-05 < local_38 ||
            (((0.0001 < ABS(local_4c) || (0.0001 < ABS(*param_3))) || (0.0001 < ABS(param_3[1]))))))
           ) {
          fVar19 = game_engine_get_time_scale();
          if (0.0 <= *pfVar8) {
            if (*pfVar8 <= 1.0) {
              fVar3 = *pfVar8;
            }
            else {
              fVar3 = 1.0;
            }
          }
          else {
            fVar3 = 0.0;
          }
          fVar3 = 1.0 - fVar3 * *pfVar1;
          local_38 = (double)CONCAT44(local_38._4_4_,fVar3);
          if (0.0 <= pfVar8[1]) {
            if (pfVar8[1] <= 1.0) {
              fVar4 = pfVar8[1];
            }
            else {
              fVar4 = 1.0;
            }
          }
          else {
            fVar4 = 0.0;
          }
          if (*(char *)(DAT_006b0b80 + 2) != '\0') {
            fVar19 = fVar19 * 0.5;
          }
          local_1c = local_1c * fVar19;
          fVar19 = fVar19 * local_18;
          if (-0.10471976 <= local_1c) {
            if (0.10471976 < local_1c) {
              local_1c = 0.10471976;
            }
          }
          else {
            local_1c = -0.10471976;
          }
          if (-0.05235988 <= fVar19) {
            if (0.05235988 < fVar19) {
              fVar19 = 0.05235988;
            }
          }
          else {
            fVar19 = -0.05235988;
          }
          local_24 = fVar3 * local_24 + local_1c * fVar4 * *pfVar1;
          local_20 = fVar3 * local_20 + fVar19 * fVar4 * *pfVar1;
        }
        param_3[3] = local_24 * param_2 * 30.0;
        param_3[4] = param_2 * 30.0 * local_20;
        iVar17 = local_30;
      }
    }
    else {
      param_3[3] = 0.0;
      param_3[4] = 0.0;
    }
    local_14[1] = '\0';
    local_14[2] = '\0';
    local_14[3] = '\0';
    local_14[4] = '\0';
    local_14[5] = '\0';
    local_14[6] = '\0';
    local_14[7] = '\0';
    local_14[8] = '\0';
    local_14[9] = '\0';
    local_14[10] = '\0';
    local_14[0xb] = '\0';
    local_14[0xc] = '\0';
    local_14[0xd] = '\0';
    local_14[0xe] = '\0';
    local_14[0xf] = '\0';
    local_14[0x10] = '\0';
    local_14[0x11] = '\0';
    local_14[0x12] = '\0';
    uVar9 = *(ushort *)(iVar17 + 10) & *(ushort *)(iVar17 + 8);
    local_14[0] = '\0';
    if (uVar9 != 0) {
      iVar14 = 0;
      do {
        if ((((uint)uVar9 & 1 << ((byte)iVar14 & 0x1f)) != 0) &&
           (*(char *)((int)&DAT_00712498 + iVar14 + iVar2) == '\0')) {
          uVar10 = ~(ushort)(1 << ((byte)iVar14 & 0x1f));
          *(ushort *)(iVar17 + 8) = *(ushort *)(iVar17 + 8) & uVar10;
          *(ushort *)(iVar17 + 10) = *(ushort *)(iVar17 + 10) & uVar10;
        }
        iVar14 = iVar14 + 1;
      } while (iVar14 < 0x13);
    }
    uVar9 = *(ushort *)(iVar17 + 8);
    iVar14 = 0;
    do {
      if (((uint)uVar9 & 1 << ((byte)iVar14 & 0x1f)) == 0) {
        local_14[iVar14] = (local_14 + iVar14)[(int)&DAT_00712498 + (iVar2 - (int)local_14)];
      }
      iVar14 = iVar14 + 1;
    } while (iVar14 < 0x13);
    if (local_14[10] == '\0') {
      fVar19 = (float)((uint)param_3[6] & 0xfffffffe);
    }
    else {
      fVar19 = (float)((uint)param_3[6] | 1);
    }
    param_3[6] = fVar19;
    if (local_14[0] == '\0') {
      fVar19 = (float)((uint)param_3[6] & 0xfffffffd);
    }
    else {
      fVar19 = (float)((uint)param_3[6] | 2);
    }
    param_3[6] = fVar19;
    if (local_14[2] == 0) {
      fVar19 = (float)((uint)fVar19 & 0xffffffbf);
    }
    else {
      fVar19 = (float)((uint)fVar19 | 0x40);
    }
    param_3[6] = fVar19;
    if (local_14[5] == '\0') {
      fVar19 = (float)((uint)param_3[6] & 0xffffffef);
    }
    else {
      fVar19 = (float)((uint)param_3[6] | 0x10);
    }
    param_3[6] = fVar19;
    if (local_14[0xd] == '\0') {
      fVar19 = (float)((uint)param_3[6] & 0xfffffbff);
    }
    else {
      fVar19 = (float)((uint)param_3[6] | 0x400);
    }
    param_3[6] = fVar19;
    if (local_14[7] == '\0') {
      fVar19 = (float)((uint)param_3[6] & 0xfffff7ff);
    }
    else {
      fVar19 = (float)((uint)param_3[6] | 0x800);
    }
    param_3[6] = fVar19;
    fVar19 = param_3[6];
    if (local_14[6] == '\0') {
      param_3[6] = (float)((uint)fVar19 & 0xffffefff);
      fVar19 = (float)((uint)fVar19 & 0xffffcfff);
    }
    else {
      param_3[6] = (float)((uint)fVar19 | 0x1000);
      fVar19 = (float)((uint)fVar19 | 0x3000);
    }
    param_3[6] = fVar19;
    if ((local_14[0xe] == '\0') && ((short)(ushort)(byte)local_14[2] < *(short *)(local_2c + 0x1b)))
    {
      fVar19 = (float)((uint)param_3[6] & 0xffffbfff);
    }
    else {
      fVar19 = (float)((uint)param_3[6] | 0x4000);
    }
    param_3[6] = fVar19;
    if (local_14[4] == '\0') {
      fVar19 = (float)((uint)param_3[6] & 0xffffff7f);
    }
    else {
      fVar19 = (float)((uint)param_3[6] | 0x80);
    }
    param_3[6] = fVar19;
    if (local_14[0xb] == '\0') {
      fVar19 = (float)((uint)param_3[7] & 0xfffffffb);
    }
    else {
      fVar19 = (float)((uint)param_3[7] | 4);
    }
    param_3[7] = fVar19;
    if (local_14[3] == '\0') {
      fVar19 = (float)((uint)param_3[7] & 0xfffffffe);
    }
    else {
      fVar19 = (float)((uint)param_3[7] | 1);
    }
    param_3[7] = fVar19;
    if (local_14[1] == '\0') {
      fVar19 = (float)((uint)param_3[7] & 0xfffffffd);
    }
    else {
      fVar19 = (float)((uint)param_3[7] | 2);
    }
    param_3[7] = fVar19;
    uVar13 = (uint)param_3[6] & 0x800;
    if (uVar13 == 0) {
      fVar19 = 0.0;
    }
    else {
      fVar19 = 1.0;
    }
    param_3[2] = fVar19;
    if ((*(byte *)(iVar17 + 9) & 2) == 0) {
      *(undefined1 *)((int)param_3 + 0x15) = (&DAT_007124a1)[iVar2];
    }
    if ((*(byte *)(iVar17 + 8) & 4) == 0) {
      *(undefined1 *)(param_3 + 5) = *(undefined1 *)((int)&DAT_00712498 + iVar2 + 2);
    }
    if (((DAT_00719720 == 1) && (uVar13 != 0)) && (*(uint *)(local_28 + 0x34) != 0xffffffff)) {
      iVar2 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 +
                      (*(uint *)(local_28 + 0x34) & 0xffff) * 0xc);
      sVar5 = *(short *)(iVar2 + 0x2f2);
      if ((sVar5 != -1) &&
         ((*(byte *)(*(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 +
                             (*(uint *)(iVar2 + 0x2f8 + sVar5 * 4) & 0xffff) * 0xc) + 0x22c) & 1) !=
          0)) {
        param_3[6] = (float)((uint)param_3[6] & 0xfffff7ff);
      }
    }
    fVar19 = *param_3 * *param_3 + param_3[1] * param_3[1];
    if (1.0 < fVar19) {
      fVar19 = 1.0 / SQRT(fVar19);
      *param_3 = fVar19 * *param_3;
      param_3[1] = fVar19 * param_3[1];
      FUN_00472760();
      return;
    }
  }
  FUN_00472760();
  return;
}
#endif
