// actor_update_aim_wander  (Ghidra: actor_update_aim_wander, renamed)
// address 0x40fcb0, size 2092 bytes
// name confidence: 0.45  rewrite confidence: 0.2
// evidence: every tag field it reads is a named ActorVariant burst / error parameter --
//   new_target_firing_pattern_time (0x88), projectile_error (0x7c), weapon_damage_modifier
//   (0xc4), damage_per_second (0xc8), rate_of_fire (0x78), special_damage_modifier (0xf8),
//   special_projectile_error (0xfc), bombardment_range (0x14c), special_fire_situation
//   (0x156) -- and the block pointer it works from is the one
//   actor_select_stance_offset_pair @0x4106b0 hands back, whose four candidate bases
//   (ActorVariant+0xcc / +0x100 / +0x118 / +0x130) are the four burst parameter blocks.
//   It is the single writer of the six aim vectors at actor+0x64c..0x684.
// register convention: actor_index arrives in a float slot (Ghidra types the one parameter
//   as float and immediately masks it).
//
// UNSURE (this is the least reliable rewrite in the module, 0.2): Ghidra never assigns
// local_3c, the burst-parameter pointer the whole second half indexes, and it reuses the
// float register holding ActorVariant.new_target_firing_pattern_time as a pointer a few
// lines later. The rewrite takes local_3c to be the second out-parameter of
// actor_select_stance_offset_pair (the only pointer in the frame that can be null and that
// has a float at +0xc), and it separates the two uses of that register. Several FPU-only
// helpers (0x46fe70, 0x4c12b0, fptan / fsin / fcos) are called with no visible arguments.
// Do not trust the exact arithmetic here without a hook comparison.
//
// UNSURE (naming): actor+0x69c is types/ai.h perception_scale, but here it is plainly the
// weapon damage modifier: it is set from ActorVariant.weapon_damage_modifier, or derived
// from damage_per_second over the weapon own rate, and then multiplied by
// ActorVariant.special_damage_modifier. actor+0x698 is likewise the aiming error angle in
// radians, taking ActorVariant.special_projectile_error as an additive term. Both are left
// under their existing header names; see src/ai/README.md.

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

extern void * actor_get_actor_definition(datum_index actor_index);     // 0x40fa70, this module
extern uint8_t actor_target_is_visible_or_object_count_ok(datum_index actor_index, int16_t kind); // 0x40f700, EAX, stack
extern void actor_choose_random_point_near(real_point3d *inout_point, float radius);                      // 0x40faf0, this module
extern void actor_select_stance_offset_pair(datum_index actor_index, uint8_t *base, uint8_t **out_a, uint8_t **out_b); // 0x4106b0, this module
extern datum_index actor_get_threat_weapon_object_index(datum_index actor_index);                // 0x4282c0
extern void ai_communication_broadcast(int32_t event_code, datum_index unit_index, datum_index object_a, int32_t reason, datum_index object_b, datum_index object_c, uint32_t *extra_data);
// 0x42d340, not yet rewritten (this module). Always seven stack arguments: every call
// site in the binary cleans up 0x1c bytes, so the shorter forms Ghidra recovers at some
// sites are artefacts, not a reduced-arity overload.
extern float weapon_get_zoom_fov_resolved(int16_t zoom_table_index, int16_t substitution_check_index); // 0x46fe70: difficulty scale, ECX table, AX team
extern float weapon_trigger_get_average_damage(datum_index weapon_tag_id, float *out_max_rate_of_fire); // 0x4c12b0, EAX, ECX
extern real vector3d_normalize_with_length(real_vector3d *v);                // 0x401990

// blam-cc: the actor index arrives in the single (float-typed) parameter slot
// Recomputes the actor per-tick aiming model: which burst parameter block applies, how long
// the current burst still has to run, the projectile error angle, the damage modifier, and
// the six aim vectors at 0x64c..0x684 that the firing state machine then consumes. Ends by
// broadcasting a firing chatter event once the actor vitality grade is past 6.
void actor_update_aim_wander(datum_index actor_index)
{
    actor *self;
    ActorVariant *variant;
    uint8_t *burst_a;
    float *burst;
    object *vehicle;
    prop *target;
    real_vector3d wander;
    float new_target_time;
    float difficulty;
    float error;
    float damage_per_shot;
    float rate;
    float target_x, target_y, target_z;
    float yaw, pitch;
    float radius, length;
    float cos_yaw, sin_yaw, cos_pitch, sin_pitch;
    float limit;
    float offset_x, offset_y, offset_z;
    uint8_t moving;
    uint8_t want_bombardment;
    datum_index weapon_object;
    int32_t ticks;
    int8_t event;
    uint32_t event_object;

    self = (actor *)((uint8_t *)actor_data->data + (actor_index & 0xffff) * sizeof(actor));
    variant = actor_get_actor_definition(actor_index);
    want_bombardment = 0;

    if (self->unknown_604 != 0 && actor_target_is_visible_or_object_count_ok(actor_index, variant->special_fire_situation) == 0) {
        self->unknown_604 = 0;
    }
    self->unknown_603 = self->unknown_604;
    self->unknown_604 = 0;

    // moving: either the actor is in a vehicle that is actually travelling, or it is on foot
    // with one of the two movement flags set.
    moving = 0;
    if (self->active_unit_index == (datum_index)0xffffffff) {
        if (self->unknown_15c != 0 || self->unknown_504 != 0) {
            moving = 1;
        }
    } else {
        vehicle = ((object_header *)object_data->data)[self->active_unit_index & 0xffff].data;
        if (*(float *)((uint8_t *)vehicle + 0x70) * *(float *)((uint8_t *)vehicle + 0x70) +
            *(float *)((uint8_t *)vehicle + 0x6c) * *(float *)((uint8_t *)vehicle + 0x6c) +
            *(float *)((uint8_t *)vehicle + 0x68) * *(float *)((uint8_t *)vehicle + 0x68) > 1.0f) {
            moving = 1;
        }
    }
    self->unknown_601 = moving;

    new_target_time = variant->new_target_firing_pattern_time;
    difficulty = weapon_get_zoom_fov_resolved(0xd, *(int16_t *)((uint8_t *)self + 0x3e)); // 0x40fd58
    self->unknown_600 = (uint8_t)((float)self->unknown_61c < difficulty * new_target_time * 30.0f);

    // UNSURE: bare call; the block pointer it returns is what the rest of the function uses.
    burst_a = (uint8_t *)0;
    burst = (float *)0;
    actor_select_stance_offset_pair(actor_index, (uint8_t *)variant, &burst_a, (uint8_t **)&burst);

    if (self->unknown_458 <= 0.0f) {
        random_seed_global = random_seed_global * 0x19660d + 0x3c6ef35f;
    }
    // UNSURE: the __ftol result comes off the FPU stack with no visible source expression.
    self->unknown_5f4 = (int16_t)(int32_t)self->unknown_458;

    error = variant->projectile_error;
    difficulty = weapon_get_zoom_fov_resolved(0xb, *(int16_t *)((uint8_t *)self + 0x3e)); // 0x40fe95
    error = difficulty * error;
    if (burst != (float *)0 && burst[3] != 0.0f) {
        error = error * burst[3];
    }
    if (self->unknown_1ca != 0) {
        error = error + error + 0.017453292f; // one degree
    }
    self->unknown_698 = error;

    self->perception_scale = 0.0f;
    if (variant->weapon_damage_modifier <= 0.0f) {
        if (variant->damage_per_second > 0.0f) {
            weapon_object = actor_get_threat_weapon_object_index(actor_index);
            if (weapon_object != (datum_index)0xffffffff) {
                // FIXED (objdump 0x40ff39..0x40ff65): EAX = the weapon's definition tag, ECX = &rate (the weapon's
                //   maximum rate of fire), which the variant's rate_of_fire caps when positive. The draft passed
                //   nothing and compared the variant rate with itself.
                datum_index weapon_tag = *(datum_index *)((object_header *)object_data->data)[weapon_object & 0xffff].data;
                damage_per_shot = weapon_trigger_get_average_damage(weapon_tag, &rate);
                if (variant->rate_of_fire > 0.0f && rate > variant->rate_of_fire) {
                    rate = variant->rate_of_fire;
                }
                if (damage_per_shot * rate > 0.0f) {
                    self->perception_scale = variant->damage_per_second / (damage_per_shot * rate);
                }
            }
        }
    } else {
        self->perception_scale = variant->weapon_damage_modifier;
    }

    if (self->unknown_603 != 0 || self->unknown_602 != 0) {
        if (variant->special_damage_modifier > 0.0f) {
            self->perception_scale = self->perception_scale * variant->special_damage_modifier;
        }
        self->unknown_698 = variant->special_projectile_error + self->unknown_698;
    }

    if (variant->bombardment_range > 0.0f && self->unknown_60c == 1) {
        target = &((prop *)prop_data->data)[self->unknown_610 & 0xffff];
        if (target->kind < 2 || target->kind > 3 || target->unknown_32 == 0) {
            want_bombardment = 1;
        }
    }

    target_x = self->wander_unknown_62c;
    target_y = self->wander_unknown_630;
    target_z = self->wander_unknown_634;
    if (want_bombardment != 0) {
        // UNSURE: bare call in the original; the point it randomizes is the target scratch.
        actor_choose_random_point_near((real_point3d *)&self->wander_unknown_62c,
                                       variant->bombardment_range);
    }

    // The aim axis: the horizontal perpendicular of (target - aim_origin), with the z term
    // multiplied out by the literal zeroes the original carries.
    wander.i = target_y - self->aim_origin.y - (target_z - self->aim_origin.z) * 0.0f;
    wander.j = (target_z - self->aim_origin.z) * 0.0f - (target_x - self->aim_origin.x);
    wander.k = (target_x - self->aim_origin.x) * 0.0f - (target_y - self->aim_origin.y) * 0.0f;
    vector3d_normalize_with_length(&wander);

    random_seed_global = random_seed_global * 0x19660d + 0x3c6ef35f;
    if ((uint16_t)(random_seed_global >> 0x10) > 0x8000) {
        wander.i = -wander.i;
        wander.j = -wander.j;
        wander.k = -wander.k;
    }

    random_seed_global = random_seed_global * 0x19660d + 0x3c6ef35f;
    yaw = (float)(random_seed_global >> 0x10) * 1.5259022e-05f * (burst[1] + burst[1]) - burst[1];
    random_seed_global = random_seed_global * 0x19660d + 0x3c6ef35f;
    pitch = (float)(random_seed_global >> 0x10) * 1.5259022e-05f * (burst[4] + burst[4]) -
            burst[4] + yaw;

    difficulty = weapon_get_zoom_fov_resolved(0xc, *(int16_t *)((uint8_t *)self + 0x3e)); // 0x410156
    radius = difficulty * burst[0];

    random_seed_global = random_seed_global * 0x19660d + 0x3c6ef35f;
    difficulty = weapon_get_zoom_fov_resolved(0xc, *(int16_t *)((uint8_t *)self + 0x3e)); // 0x4101c9
    length = difficulty * ((float)(random_seed_global >> 0x10) * 1.5259022e-05f *
                               (burst[3] - burst[2]) + burst[2]);

    if (self->unknown_1ca != 0) {
        radius = radius + radius;
        length = length + length;
    }

    // Clamp the swing to what the burst angular velocity can actually cover in the ticks
    // that are left.
    if (self->unknown_5f4 > 0 && burst[9] > 0.0f) {
        float sweep = (float)self->unknown_5f4 * burst[9] * 0.033333335f;
        if (sweep > 0.7853982f) {
            sweep = 0.7853982f;
        }
        limit = (float)ftan((double)sweep) * self->wander_unknown_638;
        if (limit < radius) {
            if (limit * 1.5f <= radius) {
                self->unknown_5f4 = (int16_t)(int32_t)((float)self->unknown_5f4 * 1.5f + 0.5f);
                length = (limit * 1.5f / radius) * length;
                radius = limit * 1.5f;
            } else {
                self->unknown_5f4 =
                    (int16_t)(int32_t)((radius / limit) * (float)self->unknown_5f4 + 0.5f);
            }
        }
    }

    cos_yaw = (float)fcos((double)yaw);
    sin_yaw = (float)fsin((double)yaw);
    cos_pitch = (float)fcos((double)pitch);
    sin_pitch = (float)fsin((double)pitch);

    offset_x = -((wander.i * cos_pitch + sin_pitch * 0.0f) * length);
    offset_y = -((wander.j * cos_pitch + sin_pitch * 0.0f) * length);
    offset_z = -((wander.k * cos_pitch + sin_pitch) * length);

    if (self->unknown_5f4 > 0) {
        float inverse = 1.0f / (float)self->unknown_5f4;
        offset_x = offset_x * inverse;
        offset_y = offset_y * inverse;
        offset_z = offset_z * inverse;
    }

    self->wander_unknown_64c.i = target_x;
    self->wander_unknown_64c.j = target_y;
    self->wander_unknown_64c.k = target_z;

    self->wander_unknown_664.i = (wander.i * cos_yaw + 0.0f * sin_yaw) * radius;
    self->wander_unknown_664.j = (wander.j * cos_yaw + 0.0f * sin_yaw) * radius;
    self->wander_unknown_664.k = (wander.k * cos_yaw + sin_yaw) * radius;

    self->wander_unknown_670.i = offset_x;
    self->wander_unknown_670.j = offset_y;
    self->wander_unknown_670.k = offset_z;

    self->grenade_aim_direction.i = self->wander_unknown_664.i + self->wander_unknown_64c.i;
    self->grenade_aim_direction.j = self->wander_unknown_664.j + self->wander_unknown_64c.j;
    self->grenade_aim_direction.k = self->wander_unknown_664.k + self->wander_unknown_64c.k;

    if (self->unknown_6e > 6) {
        event = 0;
        event_object = 0xffffffff;
        if (self->unknown_60c == 1) {
            target = &((prop *)prop_data->data)[self->unknown_610 & 0xffff];
            event = (int8_t)target->unknown_61;
            event_object = (uint32_t)target->object_index;
        }
        if (self->unknown_378 != 0) {
            event = 0x1c;                       // berserk
        } else if (event != 0) {
            event = 0x1e;
        } else if ((int8_t)self->tally.threat_class_ge_1 < 5) {
            event = (int8_t)((self->unknown_161 != 0) + 0x1a);
        } else {
            event = 0x1d;
        }
        ai_communication_broadcast(event, self->unit_index, event_object, 3, 0xffffffff,
                                   0xffffffff, 0);
    }
    (void)ticks;
    (void)burst_a;
}

#if 0
Original Ghidra decompilation (0x40fcb0): run `python tools/pack.py 0x40fcb0`.
The listing is reproduced verbatim in out/phase2/ai and is not duplicated here because its
local-variable aliasing (local_3c never assigned, fVar4 used both as
ActorVariant.new_target_firing_pattern_time and as a pointer) is described in the UNSURE
block at the top of this file rather than being reproducible line by line.
#endif
