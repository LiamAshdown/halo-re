// vehicle_update  (Ghidra: already named vehicle_update)
// address 0x570ee0, size 2531 bytes
// name confidence: 0.85 (cea-pdb hint 'vehicle_update' confirmed by decompilation)
// rewrite confidence: 0.85
// REWRITTEN from objdump 0x570ee0..0x5718c2. The draft had the parent test inverted (every free vehicle had
//   its velocity zeroed and skipped physics, attached ones ran it), used the wrong fields in the flip, altitude
//   band and impact-damage branches, and called the scalar helpers and effect updaters without their operands.
//   Stack: object_index. A network client (role 2) resyncs a stale vehicle (+0x5ac + period 0x6f1cf0) whose
//   position moved more than 1.5 from +0x5b4. An attached vehicle (+0x11c) is frozen. Otherwise: control bit 1
//   -> vehicle flag 4; bit 2 (or a throttle against the current speed with tag flag 0x10) -> brake flag 8; the
//   signed angle between facing and the desired facing (+0x224) about up x forward; a flipping vehicle (flag
//   0x10, +0x4d1 direction, under 30 ticks, up.k <= 0.9) spins at clamp(-2 up.k, tag +0x340..+0x344) * 0.3;
//   the forward / sideways speeds (+0x4d4 / +0x4d8) step toward the throttle (0x50b460, rates +0x2f8 / +0x330)
//   and turning (+0x4dc) toward the clamped angle (0x50b2f0, range +0x308) or, for type 0, the angle * 2/pi
//   times the top speed. With physics (+0x8c) and not at rest (0x20) the type's control solver runs (jump table
//   0x5718c4) followed by the skid, traction, steering-deviation and ground-contact updates, and types 3 / 5
//   are held inside the altitude band (0x746f9c +0x10 / +0x14); at rest the recoil decays. Tag flag 0x40
//   damages the riders on hard landings (matg +0x18c). Then the animation state machine and the "~blur"
//   permutation (|forward speed| >= tag +0x318). Returns 1.
// FIXED: vehicle_calculate_turret_controls / steering_wheel / lean (types 0..2) receive the [esp+0x88] buffer
//   (node_output, the powered mass points) in EDI / ESI; it is now their third parameter.
// blam-cc: stack -> object_index

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "cache.h"
#include "objects.h"
#include "hs.h"
#include "units.h"
#include <string.h>

extern data_array *object_data;         // 0x008603b0
extern tag_instance *tag_instances;     // 0x0087bc14
extern int16_t network_game_mode;    // 0x00719720
extern int32_t vehicle_network_update_period; // 0x006f1cf0
extern game_time_globals *game_time;    // 0x006f1d6c
extern uint8_t unit_updates_suppressed; // 0x0071c419
extern uint8_t *global_structure_bsp;  // 0x00746f9c, +0x10 floor / +0x14 ceiling (0 = none)
extern Globals *global_globals;

extern double atan2(double y, double x); // fpatan
extern double fabs(double x);

extern real vector3d_distance(real_point3d *a, real_point3d *b); // 0x4088b0, EAX, ECX
extern void vector3d_cross_product(real_vector3d *out, const real_vector3d *a, const real_vector3d *b); // 0x4052c0
extern uint8_t unit_get_recently_updated_flag(uint32_t object_index); // 0x570c80, EAX
extern uint8_t unit_has_child_of_type5(uint32_t unit_index); // 0x570d70, ECX
extern void unit_set_facing_from_index_table(uint32_t object_index); // 0x570de0
extern uint8_t unit_any_flagged_seat_occupied(uint32_t unit_index); // 0x56cc80, EAX
extern uint8_t physics_scalar_step_to_target_clamped(void *rates, float *value, float target, float step); // 0x50b460, EDX, ECX
extern uint8_t physics_scalar_move_toward_target(void *range, float *value, uint8_t wrap, float target,
                                                 float rate); // 0x50b2f0, ESI, EDX, stack
extern void vehicle_calculate_turret_controls(uint32_t unit_index, void *param_2, float *powered_states); // 0x572b60, stack + EDI
extern void vehicle_calculate_steering_wheel_controls(uint32_t unit_index, void *param_2, float *powered_states); // 0x572cd0, stack + EDI
extern void vehicle_calculate_lean_controls(uint32_t unit_index, void *param_2, float *powered_states); // 0x572df0, stack + ESI
extern void vehicle_calculate_ground_lean_controls(uint32_t unit_index, uint8_t *out_transform); // 0x573100
extern void vehicle_calculate_wing_flex_controls(uint32_t unit_index, float angle, uint8_t *node_output,
                                                 uint8_t *contact_points); // 0x5734d0
extern void vehicle_calculate_mounted_controls_dispatch(uint32_t unit_index, void *out_transform,
                                                        void *out_record); // 0x573ee0, ESI, ECX, EDX
extern void object_physics_tick(uint32_t object_index, void *powered_states, void *mass_points,
                                real_vector3d *extra_force, real_vector3d *extra_torque); // 0x507840
extern void unit_update_marker_skid_effects(uint32_t unit_index, uint8_t *contact_points); // 0x575460
extern uint32_t unit_update_marker_traction_effects(uint32_t object_index); // 0x575170
extern void unit_update_steering_deviation_effects(uint32_t unit_index, real_vector3d *reference_direction,
                                                   uint8_t *contact_points); // 0x574f30
extern void unit_update_ground_contact_counter(uint32_t unit_index, uint8_t *contact_points); // 0x575640
extern void unit_update_recoil_decay(uint32_t object_index); // 0x574780
extern uint16_t unit_update_animation_state_machine(uint32_t unit_index, const int8_t *request); // 0x565420
extern void object_set_permutation_by_name(uint32_t object_index, char *name, int16_t region_filter,
                                           char use_matched_index); // 0x4f6c60
extern void object_apply_damage(damage_data *dd, uint32_t object_index, int16_t node_index,
                                int16_t region_index, int16_t material_index, uint32_t plane); // 0x4ee5e0
extern char s_blur_permutation[]; // 0x00672080 "~blur"

#define OBJECT_DATA(h) ((uint8_t *)((object_header *)object_data->data)[(h) & 0xffff].data)
#define TAG_DATA(t) ((uint8_t *)tag_instances[(t) & 0xffff].data)
#define F(p, o) (*(float *)((p) + (o)))

uint32_t vehicle_update(uint32_t object_index)
{
    uint8_t *obj = OBJECT_DATA(object_index);
    uint8_t *tag = TAG_DATA(*(datum_index *)obj);
    real_vector3d *forward = (real_vector3d *)(obj + 0x74);
    real_vector3d *up = (real_vector3d *)(obj + 0x80);
    static uint8_t node_output[0xc00];     // [esp+0x88]
    static uint8_t contact_points[0x2600]; // [esp+0xc88]

    if (network_game_mode == 2 && *(int32_t *)(obj + 0x5ac) != -1 && vehicle_network_update_period != 0 &&
        game_time->game_time >= *(int32_t *)(obj + 0x5ac) + vehicle_network_update_period) {
        if (vector3d_distance((real_point3d *)(obj + 0x5b4), (real_point3d *)(obj + 0x5c)) > 1.5f &&
            unit_get_recently_updated_flag(object_index) == 1 && !unit_has_child_of_type5(object_index)) {
            unit_set_facing_from_index_table(object_index);
        }
        *(int32_t *)(obj + 0x5ac) = game_time->game_time;
    }

    if (((unit_object *)obj)->base.parent_object != k_datum_index_none) {
        // 0x570fa7: riding something, frozen
        F(obj, 0x8c) = 0.0f;
        F(obj, 0x90) = 0.0f;
        F(obj, 0x94) = 0.0f;
        F(obj, 0x68) = 0.0f;
        F(obj, 0x6c) = 0.0f;
        F(obj, 0x70) = 0.0f;
        ((unit_object *)obj)->base.flags &= ~0x20u;
    } else {
        uint32_t control = ((unit_object *)obj)->unit.control_flags;
        real_vector3d a;     // [esp+0x18]
        real_vector3d b;     // [esp+0x24]
        float angle;         // [esp+0x10]
        float throttle = F(obj, 0x278);
        float speed = F(obj, 0x4d4);

        if (control & 1) {
            obj[0x4cc] |= 4;
        } else {
            obj[0x4cc] &= ~4;
        }
        if ((control & 2) ||
            ((*(uint32_t *)(tag + 0x2f0) & 0x10) &&
             ((throttle > 0.0f && speed < 0.0f) || (throttle < 0.0f && speed > 0.0f)))) {
            obj[0x4cc] |= 8;
        } else {
            obj[0x4cc] &= ~8;
        }

        a.i = forward->k * up->j - up->k * forward->j;
        a.j = up->k * forward->i - forward->k * up->i;
        a.k = up->i * forward->j - forward->i * up->j;
        b = a;
        angle = (float)atan2(b.j * F(obj, 0x228) + b.k * F(obj, 0x22c) + b.i * F(obj, 0x224),
                             F(obj, 0x22c) * forward->k + F(obj, 0x228) * forward->j + F(obj, 0x224) * forward->i);
        if ((((unit_object *)obj)->base.network_role == 2 || ((unit_object *)obj)->base.network_role == 1) && obj[0x18] == 1) {
            unit_any_flagged_seat_occupied(object_index);
        }

        {
            uint8_t direction = obj[0x4d1];

            if ((*(uint16_t *)(obj + 0x4cc) & 0x10) && direction != 0 && obj[0x4d2] < 0x1e && up->k <= 0.9f) {
                // 0x571130: flipping back over
                float sign = (direction == 2 || direction == 4) ? 0.3f : -0.3f;
                float spin;

                if (direction == 4 || direction == 3) {
                    vector3d_cross_product(&a, up, forward);
                } else {
                    a = *forward;
                }
                spin = up->k * -2.0f;
                if (!(spin >= F(tag, 0x340))) {
                    spin = F(tag, 0x340);
                } else if (!(spin <= F(tag, 0x344))) {
                    spin = F(tag, 0x344);
                }
                spin *= sign;
                ((unit_object *)obj)->base.flags &= ~0x20u;
                if (direction == 2 || direction == 1) {
                    float k = -forward->k;

                    vector3d_cross_product(&b, up, forward);
                    a.i += b.i * k;
                    a.j += b.j * k;
                    a.k += k * b.k;
                }
                F(obj, 0x8c) = a.i * spin;
                F(obj, 0x90) = a.j * spin;
                F(obj, 0x94) = a.k * spin;
                if (*(int16_t *)(tag + 0x2f4) == 0) {
                    float along = forward->k * F(obj, 0x70) + forward->j * F(obj, 0x6c) + F(obj, 0x68) * forward->i;

                    F(obj, 0x68) = along * forward->i;
                    F(obj, 0x6c) = along * forward->j;
                    F(obj, 0x70) = along * forward->k;
                } else if (*(int16_t *)(tag + 0x2f4) == 5) {
                    if (-0.01f <= F(obj, 0x70)) {
                        F(obj, 0x70) = -0.01f;
                    }
                }
                obj[0x4d2]++;
            } else {
                *(uint16_t *)(obj + 0x4cc) &= 0xffef;
                obj[0x4d2] = 0;
                obj[0x4d1] = 0;
            }
        }

        // 0x5712d3: speeds toward the throttle
        if (obj[0x4cc] & 8) {
            physics_scalar_step_to_target_clamped(tag + 0x2f8, (float *)(obj + 0x4d4), 0.0f, 1.0f);
        } else {
            physics_scalar_step_to_target_clamped(tag + 0x2f8, (float *)(obj + 0x4d4), F(obj, 0x278), 1.0f);
            physics_scalar_step_to_target_clamped(tag + 0x330, (float *)(obj + 0x4d8), F(obj, 0x27c), 1.0f);
        }
        if (*(int16_t *)(tag + 0x2f4) != 0) {
            float target = F(obj, 0x4d4) >= 0.0f ? angle : -angle;
            float low = F(tag, 0x30c) * 0.017453292f;

            if (!(target >= low)) {
                target = low;
            } else {
                float high = F(tag, 0x308) * 0.017453292f;

                if (!(target <= high)) {
                    target = high;
                }
            }
            physics_scalar_move_toward_target(tag + 0x308, (float *)(obj + 0x4dc), 0, target,
                                              F(tag, 0x314) * 0.017453292f * 0.033333335f);
        } else if (F(obj, 0x4d4) == 0.0f) {
            physics_scalar_step_to_target_clamped(tag + 0x2f8, (float *)(obj + 0x4dc), 0.0f, 1.0f);
        } else {
            float target = angle * 0.63661975f;

            if (!(target >= -1.0f)) {
                target = -1.0f;
            } else if (!(target <= 1.0f)) {
                target = 1.0f;
            }
            physics_scalar_step_to_target_clamped(tag + 0x2f8, (float *)(obj + 0x4dc), target * F(tag, 0x2f8), 2.0f);
        }

        if (*(datum_index *)&((Unit *)tag)->base.physics.tag_id != k_datum_index_none) {
            uint32_t flags = *(uint32_t *)(tag + 0x2f0);

            if (((flags & 1) && F(obj, 0x4d4) != 0.0f) || ((flags & 2) && F(obj, 0x4dc) != 0.0f) ||
                ((flags & 4) && F(obj, 0x338) != 0.0f) || ((flags & 8) && F(obj, 0x33c) != 0.0f) ||
                ((flags & 0x20) && F(obj, 0x4d8) != 0.0f)) {
                ((unit_object *)obj)->base.flags &= ~0x20u;
            }
        }
        if (*(datum_index *)&((Unit *)tag)->base.physics.tag_id != k_datum_index_none && !(((unit_object *)obj)->base.flags & 0x20)) {
            // 0x571505: run the physics
            b = *(real_vector3d *)&((unit_object *)obj)->base.velocity.i;
            switch (*(int16_t *)(tag + 0x2f4)) {
            case 0: vehicle_calculate_turret_controls(object_index, contact_points, (float *)node_output); break; // EDI = [esp+0x88]
            case 1: vehicle_calculate_steering_wheel_controls(object_index, contact_points, (float *)node_output); break;
            case 2: vehicle_calculate_lean_controls(object_index, contact_points, (float *)node_output); break; // ESI = [esp+0x88]
            case 3: vehicle_calculate_ground_lean_controls(object_index, contact_points); break;
            case 4: vehicle_calculate_wing_flex_controls(object_index, angle, node_output, contact_points); break;
            case 5: vehicle_calculate_mounted_controls_dispatch(object_index, contact_points, node_output); break;
            case 6: object_physics_tick(object_index, 0, contact_points, 0, 0); break;
            default: break;
            }
            if (!unit_updates_suppressed) {
                unit_update_marker_skid_effects(object_index, contact_points);
            }
            if (!(uint8_t)unit_update_marker_traction_effects(object_index) && !unit_updates_suppressed) {
                unit_update_steering_deviation_effects(object_index, &b, contact_points);
            }
            unit_update_ground_contact_counter(object_index, contact_points);
            if (((unit_object *)obj)->base.flags & 0x20) {
                *(int16_t *)(obj + 0x4ce) = 15;
            }
            if (!(((unit_object *)obj)->base.flags & 0x1000000) &&
                ((1u << (*(uint8_t *)(tag + 0x2f4) & 0x1f)) & 0x28)) {
                // 0x571686: stay inside the altitude band
                float floor_z = F(global_structure_bsp, 0x10);
                float ceiling_z = F(global_structure_bsp, 0x14);

                if (floor_z != 0.0f && F(obj, 0x64) < floor_z) {
                    F(obj, 0x70) += ((floor_z - F(obj, 0x64)) * 0.015625f - F(obj, 0x70) * 0.0625f) * F(obj, 0x338);
                }
                if (ceiling_z != 0.0f && F(obj, 0x64) > ceiling_z) {
                    F(obj, 0x70) -= ((F(obj, 0x64) - ceiling_z) * 0.015625f + F(obj, 0x70) * 0.0625f) * F(obj, 0x338);
                }
            }
        } else if (*(int16_t *)(obj + 0x4ce) > 0) {
            unit_update_recoil_decay(object_index);
            unit_update_marker_traction_effects(object_index);
        }

        // 0x571744: hard landings hurt the riders
        if ((*(uint32_t *)(tag + 0x2f0) & 0x40) && !unit_updates_suppressed) {
            uint8_t *impact = (uint8_t *)global_globals->falling_damage.pointer;

            if (F(obj, 0x70) < -F(impact, 0x8c)) {
                datum_index child = ((unit_object *)obj)->base.first_child_object;

                while (child != k_datum_index_none) {
                    uint8_t *child_obj = OBJECT_DATA(child);
                    damage_data dd;

                    memset(&dd, 0, sizeof(dd));
                    dd.damage_effect_tag = *(datum_index *)(impact + 0x38);
                    dd.material_type = -1;
                    dd.responsible_player = k_datum_index_none;
                    dd.responsible_object = k_datum_index_none;
                    dd.team_index = -1;
                    dd.location_cluster_index = -1;
                    dd.random_blend = 1.0f;
                    dd.multiplier = 1.0f;
                    object_apply_damage(&dd, child, -1, -1, -1, 0);
                    child = *(datum_index *)(child_obj + 0x114);
                }
            }
        }
    }

    // 0x571828
    if (*(datum_index *)&((Unit *)tag)->base.animation_graph.tag_id != k_datum_index_none) {
        int8_t request[2] = {0, 0};

        unit_update_animation_state_machine(object_index, request);
    }
    {
        uint8_t over_blur = (uint8_t)(F(tag, 0x318) <= (float)fabs(F(obj, 0x4d4)));

        if (over_blur != (obj[0x4cc] & 1)) {
            object_set_permutation_by_name(object_index, s_blur_permutation, -1, (char)over_blur);
            if (over_blur) {
                obj[0x4cc] |= 1;
            } else {
                obj[0x4cc] &= ~1;
            }
        }
    }
    return 1;
}

#if 0
Original Ghidra decompilation (0x570ee0):

/* WARNING: Function: __chkstk replaced with injection: alloca_probe */

undefined4 vehicle_update(uint param_1)

{
  float *pfVar1;
  float fVar2;
  float fVar3;
  uint *puVar4;
  int iVar5;
  uint uVar6;
  int iVar7;
  int iVar8;
  bool bVar9;
  float fVar10;
  float fVar11;
  float fVar12;
  char cVar13;
  int iVar14;
  undefined4 *puVar15;
  float10 fVar16;
  float fVar17;
  undefined4 uVar18;
  float local_3284;
  float local_3278;
  float local_3274;
  float local_3270;
  undefined4 local_3260 [4];
  undefined2 local_3250;
  undefined2 local_3248;
  undefined4 local_3220;
  undefined4 local_321c;
  undefined2 local_3214;
  undefined1 local_3208 [3072];
  undefined1 local_2608 [9724];
  undefined4 uStack_c;

  uStack_c = 0x570ef0;
  puVar4 = *(uint **)(*(int *)(DAT_008603b0 + 0x34) + 8 + (param_1 & 0xffff) * 0xc);
  iVar5 = *(int *)((*puVar4 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
  if ((((DAT_00719720 == 2) && (puVar4[0x16b] != 0xffffffff)) && (DAT_006f1cf0 != 0)) &&
     ((int)(puVar4[0x16b] + DAT_006f1cf0) <= *(int *)(DAT_006f1d6c + 0xc))) {
    fVar16 = (float10)vector3d_distance();
    if ((((float10)1.5 < fVar16) && (cVar13 = FUN_00570c80(), cVar13 == '\x01')) &&
       (cVar13 = FUN_00570d70(), cVar13 == '\0')) {
      FUN_00570de0(param_1);
    }
    puVar4[0x16b] = *(uint *)(DAT_006f1d6c + 0xc);
  }
  if (puVar4[0x47] != 0xffffffff) {
    puVar4[0x23] = 0;
    puVar4[0x24] = 0;
    puVar4[0x25] = 0;
    puVar4[0x1a] = 0;
    puVar4[0x1b] = 0;
    puVar4[0x1c] = 0;
    puVar4[4] = puVar4[4] & 0xffffffdf;
    goto LAB_00571828;
  }
  if ((puVar4[0x82] & 1) == 0) {
    *(byte *)(puVar4 + 0x133) = (byte)puVar4[0x133] & 0xfb;
  }
  else {
    *(byte *)(puVar4 + 0x133) = (byte)puVar4[0x133] | 4;
  }
  if (((puVar4[0x82] & 2) != 0) ||
     (((*(byte *)(iVar5 + 0x2f0) & 0x10) != 0 &&
      (((0.0 < (float)puVar4[0x9e] && ((float)puVar4[0x135] < 0.0)) ||
       (((float)puVar4[0x9e] < 0.0 && (0.0 < (float)puVar4[0x135])))))))) {
    *(byte *)(puVar4 + 0x133) = (byte)puVar4[0x133] | 8;
  }
  else {
    *(byte *)(puVar4 + 0x133) = (byte)puVar4[0x133] & 0xf7;
  }
  pfVar1 = (float *)(puVar4 + 0x1d);
  fVar17 = (float)puVar4[0x1f] * (float)puVar4[0x21] - (float)puVar4[0x22] * (float)puVar4[0x1e];
  fVar10 = (float)puVar4[0x22] * *pfVar1 - (float)puVar4[0x1f] * (float)puVar4[0x20];
  fVar11 = (float)puVar4[0x20] * (float)puVar4[0x1e] - *pfVar1 * (float)puVar4[0x21];
  fVar16 = (float10)fpatan((float10)fVar17 * (float10)(float)puVar4[0x89] +
                           (float10)fVar11 * (float10)(float)puVar4[0x8b] +
                           (float10)fVar10 * (float10)(float)puVar4[0x8a],
                           (float10)(float)puVar4[0x89] * (float10)*pfVar1 +
                           (float10)(float)puVar4[0x8a] * (float10)(float)puVar4[0x1e] +
                           (float10)(float)puVar4[0x8b] * (float10)(float)puVar4[0x1f]);
  fVar2 = (float)fVar16;
  if (((puVar4[1] == 2) || (puVar4[1] == 1)) && ((char)puVar4[6] == '\x01')) {
    FUN_0056cc80();
  }
  if (((((ushort)puVar4[0x133] & 0x10) == 0) ||
      (cVar13 = *(char *)((int)puVar4 + 0x4d1), cVar13 == '\0')) ||
     ((0x1d < *(byte *)((int)puVar4 + 0x4d2) ||
      ((float)puVar4[0x22] < 0.9 == ((float)puVar4[0x22] == 0.9))))) {
    *(undefined1 *)((int)puVar4 + 0x4d2) = 0;
    *(undefined1 *)((int)puVar4 + 0x4d1) = 0;
    *(ushort *)(puVar4 + 0x133) = (ushort)puVar4[0x133] & 0xffef;
  }
  else {
    if ((cVar13 == '\x02') || (cVar13 == '\x04')) {
      fVar12 = 0.3;
    }
    else {
      fVar12 = -0.3;
    }
    if ((cVar13 == '\x04') || (cVar13 == '\x03')) {
      vector3d_cross_product(pfVar1);
      local_3278 = fVar17;
      local_3274 = fVar10;
      local_3270 = fVar11;
    }
    else {
      local_3278 = *pfVar1;
      local_3274 = (float)puVar4[0x1e];
      local_3270 = (float)puVar4[0x1f];
    }
    fVar3 = (float)puVar4[0x22] * -2.0;
    if (*(float *)(iVar5 + 0x340) <= fVar3) {
      if (*(float *)(iVar5 + 0x344) < fVar3) {
        fVar3 = *(float *)(iVar5 + 0x344);
      }
    }
    else {
      fVar3 = *(float *)(iVar5 + 0x340);
    }
    fVar3 = fVar3 * fVar12;
    puVar4[4] = puVar4[4] & 0xffffffdf;
    if ((*(char *)((int)puVar4 + 0x4d1) == '\x02') || (*(char *)((int)puVar4 + 0x4d1) == '\x01')) {
      vector3d_cross_product(pfVar1);
      fVar12 = -(float)puVar4[0x1f];
      local_3278 = fVar17 * fVar12 + local_3278;
      local_3274 = fVar10 * fVar12 + local_3274;
      local_3270 = fVar12 * fVar11 + local_3270;
    }
    puVar4[0x23] = (uint)(local_3278 * fVar3);
    puVar4[0x24] = (uint)(local_3274 * fVar3);
    puVar4[0x25] = (uint)(local_3270 * fVar3);
    if (*(short *)(iVar5 + 0x2f4) == 0) {
      fVar17 = (float)puVar4[0x1a] * *pfVar1 +
               (float)puVar4[0x1e] * (float)puVar4[0x1b] + (float)puVar4[0x1f] * (float)puVar4[0x1c]
      ;
      puVar4[0x1a] = (uint)(fVar17 * *pfVar1);
      puVar4[0x1b] = (uint)(fVar17 * (float)puVar4[0x1e]);
      fVar17 = fVar17 * (float)puVar4[0x1f];
LAB_005712ad:
      puVar4[0x1c] = (uint)fVar17;
    }
    else if (*(short *)(iVar5 + 0x2f4) == 5) {
      if (-0.01 <= (float)puVar4[0x1c]) {
        fVar17 = -0.01;
      }
      else {
        fVar17 = (float)puVar4[0x1c];
      }
      goto LAB_005712ad;
    }
    *(char *)((int)puVar4 + 0x4d2) = *(char *)((int)puVar4 + 0x4d2) + '\x01';
  }
  if ((puVar4[0x133] & 8) == 0) {
    FUN_0050b460(puVar4[0x9e],0x3f800000);
    FUN_0050b460(puVar4[0x9f],0x3f800000);
  }
  else {
    FUN_0050b460(0,0x3f800000);
  }
  if (*(short *)(iVar5 + 0x2f4) == 0) {
    if ((float)puVar4[0x135] == 0.0) {
      uVar18 = 0x3f800000;
      fVar17 = 0.0;
    }
    else {
      fVar17 = fVar2 * 0.63661975;
      if (-1.0 <= fVar17) {
        if (1.0 < fVar17) {
          fVar17 = 1.0;
        }
      }
      else {
        fVar17 = -1.0;
      }
      fVar17 = fVar17 * *(float *)(iVar5 + 0x2f8);
      uVar18 = 0x40000000;
    }
    FUN_0050b460(fVar17,uVar18);
  }
  else {
    local_3284 = fVar2;
    if ((float)puVar4[0x135] < 0.0) {
      local_3284 = -fVar2;
    }
    fVar17 = *(float *)(iVar5 + 0x30c) * 0.017453292;
    if ((fVar17 <= local_3284) &&
       (fVar10 = *(float *)(iVar5 + 0x308) * 0.017453292, fVar17 = local_3284, fVar10 < local_3284))
    {
      fVar17 = fVar10;
    }
    local_3284 = fVar17;
    FUN_0050b2f0(0,local_3284,*(float *)(iVar5 + 0x314) * 0.017453292 * 0.033333335);
  }
  if (*(int *)(iVar5 + 0x8c) == -1) {
LAB_00571725:
    if (0 < *(short *)((int)puVar4 + 0x4ce)) {
      unit_update_recoil_decay(param_1);
      FUN_00575170(param_1);
    }
  }
  else {
    uVar6 = *(uint *)(iVar5 + 0x2f0);
    if (((((((uVar6 & 1) != 0) && ((float)puVar4[0x135] != 0.0)) ||
          (((uVar6 & 2) != 0 && ((float)puVar4[0x137] != 0.0)))) ||
         (((uVar6 & 4) != 0 && ((float)puVar4[0xce] != 0.0)))) ||
        (((uVar6 & 8) != 0 && ((float)puVar4[0xcf] != 0.0)))) ||
       (((uVar6 & 0x20) != 0 && ((float)puVar4[0x136] != 0.0)))) {
      puVar4[4] = puVar4[4] & 0xffffffdf;
    }
    if ((*(int *)(iVar5 + 0x8c) == -1) || ((puVar4[4] & 0x20) != 0)) goto LAB_00571725;
    switch(*(undefined2 *)(iVar5 + 0x2f4)) {
    case 0:
      FUN_00572b60(param_1,local_2608);
      break;
    case 1:
      FUN_00572cd0(param_1,local_2608);
      break;
    case 2:
      FUN_00572df0(param_1,local_2608);
      break;
    case 3:
      FUN_00573100(param_1,local_2608);
      break;
    case 4:
      FUN_005734d0(param_1,fVar2,local_3208,local_2608);
      break;
    case 5:
      FUN_00573ee0();
      break;
    case 6:
      FUN_00507840(param_1,0,local_2608,0,0);
    }
    if (DAT_0071c419 == '\0') {
      FUN_00575460(local_2608);
    }
    cVar13 = FUN_00575170(param_1);
    if ((cVar13 == '\0') && (DAT_0071c419 == '\0')) {
      FUN_00574f30(param_1);
    }
    FUN_00575640();
    if ((puVar4[4] & 0x20) != 0) {
      *(undefined2 *)((int)puVar4 + 0x4ce) = 0xf;
    }
    if (((puVar4[4] & 0x1000000) == 0) && ((1 << (*(byte *)(iVar5 + 0x2f4) & 0x1f) & 0x28U) != 0)) {
      fVar2 = *(float *)(DAT_00746f9c + 0x10);
      fVar17 = *(float *)(DAT_00746f9c + 0x14);
      if ((fVar2 != 0.0) && ((float)puVar4[0x19] < fVar2)) {
        puVar4[0x1c] = (uint)(((fVar2 - (float)puVar4[0x19]) * 0.015625 -
                              (float)puVar4[0x1c] * 0.0625) * (float)puVar4[0xce] +
                             (float)puVar4[0x1c]);
      }
      if ((fVar17 != 0.0) && (fVar17 < (float)puVar4[0x19])) {
        puVar4[0x1c] = (uint)((float)puVar4[0x1c] -
                             ((float)puVar4[0x1c] * 0.0625 +
                             ((float)puVar4[0x19] - fVar17) * 0.015625) * (float)puVar4[0xce]);
      }
    }
  }
  if ((((*(byte *)(iVar5 + 0x2f0) & 0x40) != 0) && (DAT_0071c419 == '\0')) &&
     (iVar7 = *(int *)(DAT_00746fa0 + 0x18c), (float)puVar4[0x1c] < -*(float *)(iVar7 + 0x8c))) {
    uVar6 = puVar4[0x46];
    while (uVar6 != 0xffffffff) {
      iVar8 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (uVar6 & 0xffff) * 0xc);
      puVar15 = local_3260;
      for (iVar14 = 0x15; iVar14 != 0; iVar14 = iVar14 + -1) {
        *puVar15 = 0;
        puVar15 = puVar15 + 1;
      }
      local_3260[0] = *(undefined4 *)(iVar7 + 0x38);
      local_3214 = 0xffff;
      local_3260[2] = 0xffffffff;
      local_3260[3] = 0xffffffff;
      local_3250 = 0xffff;
      local_3248 = 0xffff;
      local_3220 = 0x3f800000;
      local_321c = 0x3f800000;
      object_apply_damage(local_3260,uVar6,0xffffffff,0xffffffff,0xffffffff,0);
      uVar6 = *(uint *)(iVar8 + 0x114);
    }
  }
LAB_00571828:
  if (*(int *)(iVar5 + 0x44) != -1) {
    FUN_00565420(param_1);
  }
  bVar9 = *(float *)(iVar5 + 0x318) < ABS((float)puVar4[0x135]) !=
          (*(float *)(iVar5 + 0x318) == ABS((float)puVar4[0x135]));
  if (bVar9 != (bool)((byte)puVar4[0x133] & 1)) {
    object_set_permutation_by_name("~blur",0xffffffff,bVar9);
    if (bVar9) {
      *(byte *)(puVar4 + 0x133) = (byte)puVar4[0x133] | 1;
      return 1;
    }
    *(byte *)(puVar4 + 0x133) = (byte)puVar4[0x133] & 0xfe;
    return 1;
  }
  return 1;
}
#endif
