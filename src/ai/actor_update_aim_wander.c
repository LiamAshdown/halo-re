// actor_update_aim_wander  (Ghidra: actor_update_aim_wander, renamed)
// address 0x40fcb0, size 2092 bytes
// name confidence: 0.45  rewrite confidence: 0.85 (REWRITTEN from objdump 0x40fcb0..0x4104db)
// Per-tick firing aim / burst update for one actor (from actor_update_firing_state). ActorVariant fields:
//   +0x78 rate of fire, +0x7c projectile error, +0x88 new-target firing pattern time, +0xc4 weapon damage
//   modifier, +0xc8 damage per second, +0xf8 / +0xfc special damage modifier / projectile error, +0x14c
//   bombardment range, +0x156 special fire situation. actor_select_stance_offset_pair hands back the burst
//   block (out_a, EDI: +0x00 radius scale, +0x04 / +0x10 angle ranges, +0x08..+0x0c radius range, +0x14..+0x18
//   burst time range, +0x24 burst angle rate) and a scale block (out_b, ESI: +0x00 time scale, +0x0c error scale).
//   Writes +0x600..+0x604 flags, +0x5f4 burst ticks, +0x698 error, +0x69c damage modifier, and the aim record
//   +0x64c target, +0x664 wander, +0x670 per-tick return, +0x67c target + wander; high grade actors broadcast a
//   firing line. The draft took +0x5f4 straight from +0x458, randomized the actor target instead of its copy,
//   mixed the two blocks and was missing the burst clamp and the wander offsets.
// blam-cc: stack -> actor_index

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "ai.h"

extern data_array *actor_data;      // 0x00880360
extern data_array *object_data;     // 0x008603b0
extern data_array *prop_data;       // 0x008802c0
extern uint32_t random_seed_global; // 0x00719cd0

extern double fcos(double x);
extern double fsin(double x);
extern double ftan(double x);
extern int32_t fistp_round(float x); // harness/x87_shims.c

extern void *actor_get_actor_definition(datum_index actor_index); // 0x40fa70, EAX
extern uint8_t actor_target_is_visible_or_object_count_ok(datum_index actor_index, int16_t kind); // 0x40f700, EAX, stack
extern void actor_choose_random_point_near(real_point3d *inout_point, float radius); // 0x40faf0, ESI, stack
extern void actor_select_stance_offset_pair(datum_index actor_index, uint8_t *base, uint8_t **out_a, uint8_t **out_b); // 0x4106b0, EAX, EDX, EDI out_a, ESI out_b
extern datum_index actor_get_threat_weapon_object_index(datum_index actor_index); // 0x4282c0, EAX
extern void ai_communication_broadcast(int32_t event_code, datum_index unit_index, datum_index object_a, int32_t reason,
    datum_index object_b, datum_index object_c, uint32_t *extra_data); // 0x42d340
extern float weapon_get_zoom_fov_resolved(int16_t zoom_table_index, int16_t substitution_check_index); // 0x46fe70, ECX, AX
extern float weapon_trigger_get_average_damage(datum_index weapon_tag_id, float *out_max_rate_of_fire); // 0x4c12b0, EAX, ECX
extern real vector3d_normalize_with_length(real_vector3d *v); // 0x401990, ECX

static float aim_wander_random_fraction(void)
{
    random_seed_global = random_seed_global * 0x19660d + 0x3c6ef35f;
    return (float)(int32_t)(random_seed_global >> 16) * 1.5259022e-05f;
}

void actor_update_aim_wander(datum_index actor_index)
{
    uint8_t *a = (uint8_t *)actor_data->data + (actor_index & 0xffff) * 0x724;
    uint8_t *variant = (uint8_t *)actor_get_actor_definition(actor_index);
    int16_t team = ((actor *)a)->team;
    uint8_t *burst = 0;       // out_a (EDI)
    uint8_t *scale = 0;       // out_b (ESI)
    uint8_t bombard = 0;
    float time;
    float error;
    float angle_1, angle_2;
    float radius_a, radius_b;
    real_point3d target;
    real_vector3d side;
    real_vector3d wander;
    real_vector3d recoil;
    uint8_t moving;

    if (a[0x604] != 0 && !actor_target_is_visible_or_object_count_ok(actor_index,
            (int16_t)*(uint16_t *)&((ActorVariant *)variant)->special_fire_situation)) {
        a[0x604] = 0;
    }
    a[0x603] = a[0x604];
    a[0x604] = 0;

    if (((actor *)a)->active_unit_index == k_datum_index_none) {
        moving = (a[0x15c] != 0 || a[0x504] != 0) ? 1 : 0;
    } else {
        uint8_t *vehicle = (uint8_t *)((object_header *)object_data->data)[((actor *)a)->active_unit_index & 0xffff].data;
        real_vector3d *velocity = (real_vector3d *)(vehicle + 0x68);

        moving = (velocity->i * velocity->i + velocity->j * velocity->j + velocity->k * velocity->k > 1.0f) ? 1 : 0;
    }
    a[0x601] = moving;
    a[0x600] = (weapon_get_zoom_fov_resolved(0xd, team) * ((ActorVariant *)variant)->new_target_firing_pattern_time * 30.0f >
        (float)*(int32_t *)(a + 0x61c)) ? 1 : 0;

    actor_select_stance_offset_pair(actor_index, variant, &burst, &scale);

    // the burst length
    if (*(float *)(a + 0x458) > 0.0f) {
        time = *(float *)(a + 0x458);
    } else {
        time = aim_wander_random_fraction() * (*(float *)(burst + 0x18) - *(float *)(burst + 0x14)) +
            *(float *)(burst + 0x14);
        if (scale != 0 && *(float *)(scale + 0x0) != 0.0f) {
            time = time * *(float *)(scale + 0x0);
        }
        if (a[0x1ca] != 0) {
            time = time * 0.6f;
        }
    }
    *(int16_t *)(a + 0x5f4) = (int16_t)(int32_t)(time * 30.0f);

    // the aiming error
    error = weapon_get_zoom_fov_resolved(0xb, team) * ((ActorVariant *)variant)->projectile_error;
    if (scale != 0 && *(float *)(scale + 0xc) != 0.0f) {
        error = error * *(float *)(scale + 0xc);
    }
    if (a[0x1ca] != 0) {
        error = error + error + 0.017453292f;
    }
    *(float *)(a + 0x698) = error;

    // the damage modifier
    ((actor *)a)->perception_scale = 0.0f;
    if (((ActorVariant *)variant)->weapon_damage_modifier > 0.0f) {
        ((actor *)a)->perception_scale = ((ActorVariant *)variant)->weapon_damage_modifier;
    } else if (((ActorVariant *)variant)->damage_per_second > 0.0f) {
        datum_index weapon = actor_get_threat_weapon_object_index(actor_index);

        if (weapon != k_datum_index_none) {
            float rate;
            float damage = weapon_trigger_get_average_damage(
                *(datum_index *)((object_header *)object_data->data)[weapon & 0xffff].data, &rate);

            if (((ActorVariant *)variant)->rate_of_fire > 0.0f && rate > ((ActorVariant *)variant)->rate_of_fire) {
                rate = ((ActorVariant *)variant)->rate_of_fire;
            }
            damage = damage * rate;
            if (damage > 0.0f) {
                ((actor *)a)->perception_scale = ((ActorVariant *)variant)->damage_per_second / damage;
            }
        }
    }
    if (a[0x603] != 0 || a[0x602] != 0) {
        if (((ActorVariant *)variant)->special_damage_modifier > 0.0f) {
            ((actor *)a)->perception_scale = ((actor *)a)->perception_scale * ((ActorVariant *)variant)->special_damage_modifier;
        }
        *(float *)(a + 0x698) = ((ActorVariant *)variant)->special_projectile_error + *(float *)(a + 0x698);
    }

    // the (possibly bombarded) target
    if (((ActorVariant *)variant)->bombardment_range > 0.0f && *(int16_t *)(a + 0x60c) == 1) {
        uint8_t *prop = (uint8_t *)prop_data->data + (*(datum_index *)(a + 0x610) & 0xffff) * 0x138;
        int16_t kind = ((struct prop *)prop)->kind;

        bombard = (kind < 2 || kind > 3 || *(int16_t *)(prop + 0x32) == 0) ? 1 : 0;
    }
    target = *(real_point3d *)&((actor *)a)->wander_unknown_62c;
    if (bombard) {
        actor_choose_random_point_near(&target, ((ActorVariant *)variant)->bombardment_range);
    }
    {
        float dx = target.x - ((actor *)a)->aim_origin.x;
        float dy = target.y - ((actor *)a)->aim_origin.y;
        float dz = (target.z - ((actor *)a)->aim_origin.z) * 0.0f;

        side.i = dy - dz;
        side.j = dz - dx;
        side.k = dx * 0.0f - dy * 0.0f;
    }
    vector3d_normalize_with_length(&side);
    random_seed_global = random_seed_global * 0x19660d + 0x3c6ef35f;
    if ((uint16_t)(random_seed_global >> 16) > 0x8000) {
        side.i = -side.i;
        side.j = -side.j;
        side.k = -side.k;
    }

    // the wander angles and radii
    angle_1 = aim_wander_random_fraction() * (*(float *)(burst + 0x4) + *(float *)(burst + 0x4)) - *(float *)(burst + 0x4);
    angle_2 = aim_wander_random_fraction() * (*(float *)(burst + 0x10) + *(float *)(burst + 0x10)) -
        *(float *)(burst + 0x10) + angle_1;
    radius_a = weapon_get_zoom_fov_resolved(0xc, team) * *(float *)(burst + 0x0);
    radius_b = aim_wander_random_fraction() * (*(float *)(burst + 0xc) - *(float *)(burst + 0x8)) + *(float *)(burst + 0x8);
    radius_b = weapon_get_zoom_fov_resolved(0xc, team) * radius_b;
    if (a[0x1ca] != 0) {
        radius_a = radius_a + radius_a;
        radius_b = radius_b + radius_b;
    }

    // the burst clamp: the wander may not outrun the burst's angular rate at the target distance
    if (*(int16_t *)(a + 0x5f4) > 0 && *(float *)(burst + 0x24) > 0.0f) {
        float ticks = (float)(int32_t)*(int16_t *)(a + 0x5f4);
        float sweep = ticks * *(float *)(burst + 0x24) * 0.033333335f;
        float limit;

        if (!(sweep <= 0.7853982f)) {
            sweep = 0.7853982f;
        }
        limit = (float)ftan((double)sweep) * ((actor *)a)->wander_unknown_638;
        if (radius_a > limit) {
            float limit_15 = limit * 1.5f;

            if (radius_a >= limit_15) {
                *(int16_t *)(a + 0x5f4) = (int16_t)fistp_round(ticks * 1.5f);
                radius_b = limit_15 / radius_a * radius_b;
                radius_a = limit_15;
            } else {
                *(int16_t *)(a + 0x5f4) = (int16_t)fistp_round(radius_a / limit * ticks);
            }
        }
    }

    // the wander offset (side / up plane) and the per-tick return
    {
        float c1 = (float)fcos((double)angle_1), s1 = (float)fsin((double)angle_1);
        float c2 = (float)fcos((double)angle_2), s2 = (float)fsin((double)angle_2);

        wander.i = (side.i * c1 + 0.0f * s1) * radius_a;
        wander.j = (side.j * c1 + 0.0f * s1) * radius_a;
        wander.k = (side.k * c1 + s1) * radius_a;
        recoil.i = -((side.i * c2 + s2 * 0.0f) * radius_b);
        recoil.j = -((side.j * c2 + s2 * 0.0f) * radius_b);
        recoil.k = -((side.k * c2 + s2) * radius_b);
    }
    if (*(int16_t *)(a + 0x5f4) > 0) {
        float per_tick = 1.0f / (float)(int32_t)*(int16_t *)(a + 0x5f4);

        recoil.i = recoil.i * per_tick;
        recoil.j = recoil.j * per_tick;
        recoil.k = recoil.k * per_tick;
    }
    *(real_point3d *)&((actor *)a)->wander_unknown_64c.i = target;
    *(real_vector3d *)&((actor *)a)->wander_unknown_664.i = wander;
    *(real_vector3d *)&((actor *)a)->wander_unknown_670.i = recoil;
    ((actor *)a)->grenade_aim_direction.i = wander.i + target.x;
    ((actor *)a)->grenade_aim_direction.j = wander.j + target.y;
    ((actor *)a)->grenade_aim_direction.k = wander.k + target.z;

    // the firing line
    if (*(int16_t *)(a + 0x6e) >= 7) {
        uint8_t prop_flag = 0;
        datum_index object = k_datum_index_none;
        int32_t code;

        if (*(int16_t *)(a + 0x60c) == 1) {
            uint8_t *prop = (uint8_t *)prop_data->data + (*(datum_index *)(a + 0x610) & 0xffff) * 0x138;

            prop_flag = prop[0x61];
            object = ((struct prop *)prop)->object_index;
        }
        if (a[0x378] != 0) {
            code = 0x1c;
        } else if (prop_flag) {
            code = 0x1e;
        } else if ((int8_t)a[0x1f8] >= 5) {
            code = 0x1d;
        } else {
            code = 0x1a + (a[0x161] != 0);
        }
        ai_communication_broadcast(code, ((actor *)a)->unit_index, object, 3, k_datum_index_none, k_datum_index_none, 0);
    }
}

#if 0
Original Ghidra decompilation (0x40fcb0): run `python tools/pack.py 0x40fcb0`.
The listing is reproduced verbatim in out/phase2/ai and is not duplicated here because its
local-variable aliasing (local_3c never assigned, fVar4 used both as
ActorVariant.new_target_firing_pattern_time and as a pointer) is described in the UNSURE
block at the top of this file rather than being reproducible line by line.
#endif
