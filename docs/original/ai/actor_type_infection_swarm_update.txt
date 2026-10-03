// actor_type_infection_swarm_update  (not a Ghidra function; the "infection" actor type's +0x18 procedure (actor
//   type table record 0x685318), dispatched by actor_dispatch_type_vtable_0x18 from actor_update_activation_state;
//   no C existed, so an infection-form swarm trapped as unlisted_424c20)
// address 0x424c20, size 4093 bytes
// name confidence: 0.6   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x424c20..0x425c1c (Ghidra's DecompileAt skeleton checked instruction by
//   instruction; mode table 0x425c20, behaviour table 0x425c4c). Steers every member of the actor's swarm (swarm
//   +0x02 count, +0x08 retarget timer, +0x0c goal, +0x18 units[], +0x58 components[] of 0x40 bytes):
//   - the timer counts down; at 0 in modes 7/10 it restarts at max(6, (2u + 6) / count * 30) ticks and one member
//     index is picked at random (u = rand / 65536);
//   - per member: up = the unit's up (a biped standing on something (+0x4d8) uses +0x514), the "airborne" bit
//     (+0x4cc bit 0) for bipeds; with grade >= 3 the best prop is scored (10 * (1 - d / r) inside the definition's
//     +0xa0 radius, +7 for the current target or +5 for another kind-2/3 prop, +5 more when not vaulting) and kept
//     as the component's target (+0x14), "close" when nearer than the definition's +0x160;
//   - the actor mode picks a speed and behaviour (1-3 wander toward the swarm goal with random pauses, 4/5 go
//     toward/away from a prop, 6 scripted direction); an attached member (+0x11c) checks whether to let go
//     (unit_detach_reposition_and_nudge); the desired direction is normalized, kept off the up axis, spread away
//     from nearby members, and made perpendicular to up;
//   - the control flags (+0x289 / +0x4f4 on the unit) and a 0x40-byte unit_control_data (speed byte, flags, facing,
//     aiming and looking = the desired direction, throttle (1, 0, 0) while moving) go to unit_apply_control_block.
// blam-cc: stack -> actor_index

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"
#include "game.h"
#include "objects.h"
#include "units.h"
#include "cache.h"
#include <string.h>

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern data_array *actor_data;           // 0x00880360, 0x724-byte actors
extern data_array *prop_data;            // 0x008802c0, 0x138-byte props
extern data_array *object_data;          // 0x008603b0
extern data_array *swarm_data;           // 0x0088035c, 0x98-byte swarms
extern data_array *swarm_component_data; // 0x00880358, 0x40-byte components
extern tag_instance *tag_instances;      // 0x0087bc14
extern uint32_t random_seed_global;      // 0x00719cd0
extern game_time_globals *game_time;     // 0x006f1d6c, game time at +0x0c
extern const real_point3d *global_origin3d_pointer; // 0x00696714

extern real random_real_range(real min, real max); // 0x401050
extern real vector3d_normalize_with_length(real_vector3d *v); // 0x401990, ECX
extern void vector3d_cross_product(real_vector3d *out, const real_vector3d *a, const real_vector3d *b); // 0x4052c0, EAX out, ECX a, stack b
extern int32_t actor_pick_dialogue_variant_a(int16_t category); // 0x424aa0, AX
extern int32_t actor_pick_dialogue_variant_b(int16_t category); // 0x424b80, AX
extern void vector3d_rotate_about_axis(real_vector3d *v, real_vector3d *axis, real sin_angle, real cos_angle); // 0x4cd820, EAX v, ECX axis
extern void unit_apply_control_block(uint32_t unit_index, const unit_control_data *control, int32_t source_id); // 0x5639f0, EAX, EDX, stack
extern uint8_t unit_is_in_busy_animation_state(uint32_t unit_index); // 0x569c90, ECX
extern void unit_detach_reposition_and_nudge(uint32_t unit_index); // 0x56ca40, EDI
extern double sqrt(double x);
extern double sin(double x);
extern double cos(double x);
extern double fabs(double x);

#define ACTOR(h) ((uint8_t *)actor_data->data + ((h) & 0xffff) * 0x724)
#define PROP(h) ((uint8_t *)prop_data->data + ((h) & 0xffff) * 0x138)
#define OBJECT(h) ((uint8_t *)((object_header *)object_data->data)[(h) & 0xffff].data)
#define SWARM(h) ((uint8_t *)swarm_data->data + ((h) & 0xffff) * 0x98)
#define COMPONENT(h) ((uint8_t *)swarm_component_data->data + ((h) & 0xffff) * 0x40)
#define F(p, o) (*(float *)((uint8_t *)(p) + (o)))
#define U16(p, o) (*(uint16_t *)((uint8_t *)(p) + (o)))
#define I16(p, o) (*(int16_t *)((uint8_t *)(p) + (o)))
#define U32(p, o) (*(uint32_t *)((uint8_t *)(p) + (o)))

static uint32_t swarm_random_next(void)
{
    random_seed_global = random_seed_global * 0x19660d + 0x3c6ef35f;
    return random_seed_global >> 16;
}

static void copy3(real_vector3d *out, const void *in)
{
    out->i = ((const float *)in)[0];
    out->j = ((const float *)in)[1];
    out->k = ((const float *)in)[2];
}

void actor_type_infection_swarm_update(datum_index actor_index)
{
    uint8_t *actor = ACTOR(actor_index);
    uint8_t *definition = (uint8_t *)tag_instances[U32(actor, 0x5c) & 0xffff].data; // esp+0xa4
    uint8_t *swarm = SWARM(U32(actor, 0x28));                                         // esp+0x68
    int32_t picked = -1;                                                               // esp+0x80
    int16_t member;

    if (I16(swarm, 0x8) > 0) {
        I16(swarm, 0x8)--;
    } else if (I16(actor, 0x6c) == 7 || I16(actor, 0x6c) == 10) {
        // 0x424cbd: max(6, ((u + u) + 6) / count * 30), truncated by __ftol
        float delay = ((float)(int32_t)swarm_random_next() * 1.5259022e-05f);
        int16_t count = I16(swarm, 0x2);

        delay = (delay + delay + 6.0f) / (float)(int32_t)count * 30.0f;
        if (!(delay > 6.0f)) {
            delay = 6.0f;
        }
        I16(swarm, 0x8) = (int16_t)(int32_t)delay;
        picked = (int32_t)(((uint32_t)(int32_t)count * swarm_random_next()) >> 16);
    }

    for (member = 0; member < I16(swarm, 0x2); member++) {
        uint32_t unit = U32(swarm, 0x18 + member * 4);                 // esp+0x7c
        uint8_t *object = OBJECT(unit);                                  // esp+0x3c
        uint8_t *component = COMPONENT(U32(swarm, 0x58 + member * 4));   // esi
        uint8_t *best_prop = 0;                                          // esp+0x6c / ebp
        datum_index best_handle = k_datum_index_none;                    // ebx
        datum_index target = k_datum_index_none;                         // esp+0x30
        int16_t behaviour = 0;                                           // esp+0x34
        uint8_t speed = 3;                                               // esp+0x44
        uint8_t control_byte_1 = 1;                                      // esp+0x48
        uint8_t aligned = 0;                                             // esp+0x3a
        uint8_t target_close = 0;                                        // esp+0x23
        uint8_t moving = 0;                                              // esp+0x13
        uint8_t riding = 0;                                              // esp+0x3b
        uint8_t fire = 0;
        real_vector3d up;                                                // esp+0x24
        real_vector3d desired;                                           // esp+0x14
        uint16_t flags;
        unit_control_data control;

        copy3(&up, object + 0x80);
        if (I16(object, 0xb4) == 0) {
            if (U32(object, 0x4d8) != (uint32_t)k_datum_index_none) {
                copy3(&up, object + 0x514);
            }
            riding = object[0x4cc] & 1;
        }

        if (I16(actor, 0x6e) >= 3) {
            float radius = F(definition, 0xa0);
            float best_score = 0.0f;   // esp+0x94
            float best_distance = 0.0f; // esp+0x9c
            datum_index prop_handle = U32(actor, 0x50);

            while (prop_handle != k_datum_index_none) {
                uint8_t *prop = PROP(prop_handle);
                datum_index this_handle = prop_handle;

                prop_handle = U32(prop, 0x8);
                if (F(prop, 0x50) > 0.0f) {
                    float dx = F(component, 0x4) - F(prop, 0xbc);
                    float dy = F(component, 0x8) - F(prop, 0xc0);
                    float dz = F(component, 0xc) - F(prop, 0xc4);
                    float distance = (float)sqrt((double)(dz * dz + dy * dy + dx * dx));
                    float score = 0.0f;

                    if (distance < radius) {
                        score = (1.0f - distance / radius) * 10.0f;
                    }
                    if (I16(prop, 0x24) >= 2 && I16(prop, 0x24) <= 3) {
                        score += (this_handle == U32(component, 0x14)) ? 7.0f : 5.0f;
                        if (prop[0x125] == 0) {
                            score += 5.0f;
                        }
                    }
                    if (score > best_score) {
                        best_score = score;
                        best_prop = prop;
                        best_handle = this_handle;
                        best_distance = distance;
                    }
                }
            }
            U32(component, 0x14) = best_handle;
            if (best_handle != k_datum_index_none && best_distance < F(definition, 0x160) &&
                I16(best_prop, 0x24) >= 2 && I16(best_prop, 0x24) <= 3) {
                target_close = 1;
            }
        } else {
            U32(component, 0x14) = best_handle;
        }

        switch (I16(actor, 0x6c)) {
        case 1:
            behaviour = 0;
            speed = 0;
            break;
        case 2:
            behaviour = 1;
            speed = 1;
            break;
        case 4:
            speed = (uint8_t)((I16(actor, 0xa8) > 0) * 2 + 3);
            if (U32(actor, 0xb8) != (uint32_t)k_datum_index_none) {
                behaviour = 5;
                target = U32(actor, 0xb8);
            }
            break;
        case 6:
            behaviour = 2;
            speed = 1;
            break;
        case 7:
            if (I16(actor, 0xa4) == 0 && U32(actor, 0x270) != (uint32_t)k_datum_index_none) {
                behaviour = 4;
                target = U32(actor, 0x270);
            } else {
                behaviour = 3;
            }
            speed = 3;
            break;
        case 10:
        case 11:
            speed = 3;
            behaviour = 3;
            if (I16(actor, 0x6c) == 11 && (component[0x2] & 0x8) != 0) {
                behaviour = 6;
            } else if (U32(component, 0x14) != (uint32_t)k_datum_index_none) {
                behaviour = (int16_t)((component[0x1a] != 0) + 4);
                control_byte_1 = 0;
                target = U32(component, 0x14);
            }
            break;
        default:
            break;
        }

        // 0x4250bc: attached members decide whether to let go
        if (U32(object, 0x11c) == (uint32_t)k_datum_index_none) {
            component[0x18] = 0;
            if (component[0x1a] != 0) {
                component[0x1a]--;
            }
        } else {
            uint8_t *parent = OBJECT(U32(object, 0x11c));
            uint8_t parent_dead = (uint8_t)((parent[0x106] >> 2) & 1);
            uint8_t detach = 0;

            if (component[0x18] != 0xff) {
                component[0x18]++;
            }
            if (parent_dead) {
                if (U32(parent, 0x41c) != (uint32_t)k_datum_index_none &&
                    (int32_t)(U32(parent, 0x41c) + 0x4b) < *(int32_t *)((uint8_t *)game_time + 0xc) &&
                    best_prop != 0 && U32(best_prop, 0x18) != U32(object, 0x11c) &&
                    I16(best_prop, 0x24) >= 2 && I16(best_prop, 0x24) <= 3) {
                    detach = 1;
                }
            } else {
                uint8_t *parent_tag = (uint8_t *)tag_instances[U32(parent, 0x0) & 0xffff].data;

                if ((I16(parent, 0xb4) != 0 || (int8_t)parent_tag[0x17d] < 0) && component[0x18] > 0x2d) {
                    component[0x1a] = 0x2d;
                    detach = 1;
                }
            }
            if (detach) {
                unit_detach_reposition_and_nudge(unit);
                component[0x2] &= 0xfc;
            } else {
                component[0x2] |= 2;
                if (parent_dead) {
                    U16(component, 0x2) &= 0xfffe;
                } else {
                    U16(component, 0x2) |= 1;
                }
            }
        }

        if (U32(object, 0x11c) != (uint32_t)k_datum_index_none) {
            goto flags;
        }
        if (riding) {
            component[0x2] &= 0xfd;
            component[0x19] = 0;
            goto flags;
        }
        if (component[0x19] != 0xff) {
            component[0x19]++;
        }
        component[0x2] &= 0xfc;
        flags = U16(component, 0x2); // esp+0xe0

        switch (behaviour) {
        case 1:
        case 2:
        case 3:
            if ((flags & 4) == 0) {
                memset(component + 0x1c, 0, 0x14);
                U16(component, 0x2) = (uint16_t)((flags & 0xfff7) | 4);
            }
            if (component[0x1d] != 0) {
                component[0x1d]--;
                if (component[0x1d] == 0) {
                    component[0x1c] = (uint8_t)actor_pick_dialogue_variant_a(behaviour);
                } else {
                    // 0x425246: a small random wobble, damped toward straight
                    float damping = F(component, 0x2c) * -0.06666667f;
                    float angle = random_real_range(-0.020943951f, 0.020943951f) + F(component, 0x2c) + damping;

                    F(component, 0x2c) = angle;
                    vector3d_rotate_about_axis((real_vector3d *)(component + 0x20), &up, (float)sin((double)angle),
                        (float)cos((double)angle));
                }
            } else {
                if (component[0x1c] != 0) {
                    component[0x1c]--;
                }
                if (component[0x1c] == 0) {
                    real_vector3d to_goal;
                    float distance_squared;
                    float angle;

                    component[0x1d] = (uint8_t)actor_pick_dialogue_variant_b(behaviour);
                    to_goal.i = F(swarm, 0xc) - F(component, 0x4);
                    to_goal.j = F(swarm, 0x10) - F(component, 0x8);
                    to_goal.k = F(swarm, 0x14) - F(component, 0xc);
                    distance_squared = to_goal.k * to_goal.k + to_goal.j * to_goal.j + to_goal.i * to_goal.i;
                    if (!(distance_squared < 0.25f)) {
                        float spread = 0.5f / (float)sqrt((double)distance_squared) * 3.1415927f;

                        angle = random_real_range(-spread, spread);
                        *(real_vector3d *)(component + 0x20) = to_goal;
                    } else {
                        angle = random_real_range(-3.1415927f, 3.1415927f);
                        copy3((real_vector3d *)(component + 0x20), object + 0x74);
                    }
                    vector3d_rotate_about_axis((real_vector3d *)(component + 0x20), &up, (float)sin((double)angle),
                        (float)cos((double)angle));
                    F(component, 0x2c) = 0.0f;
                }
            }
            if (component[0x1d] == 0) {
                goto flags;
            }
            copy3(&desired, component + 0x20);
            moving = 1;
            break;
        case 4:
        case 5: {
            uint8_t *prop = PROP(target);

            desired.i = F(prop, 0xbc) - F(component, 0x4);
            desired.j = F(prop, 0xc0) - F(component, 0x8);
            desired.k = F(prop, 0xc4) - F(component, 0xc);
            if (behaviour == 5) {
                desired.i = -desired.i;
                desired.j = -desired.j;
                desired.k = -desired.k;
            }
            moving = 1;
            break;
        }
        case 6: {
            uint8_t script_flags = component[0x21];

            if (script_flags & 1) {
                int16_t kind = I16(component, 0x24);
                uint8_t negate;

                moving = 1;
                if (kind >= 2 && kind <= 3) {
                    vector3d_cross_product(&desired, (real_vector3d *)(component + 0x28), &up);
                    negate = (uint8_t)(kind == 3);
                } else {
                    copy3(&desired, component + 0x28);
                    negate = (uint8_t)(kind == 1);
                }
                if (negate) {
                    desired.i = -desired.i;
                    desired.j = -desired.j;
                    desired.k = -desired.k;
                }
            }
            if (script_flags & 4) {
                if ((script_flags & 8) == 0 && I16(component, 0x24) == 0 && !unit_is_in_busy_animation_state(unit)) {
                    U16(component, 0x2) = (uint16_t)(flags | 0x10);
                    component[0x21] = (uint8_t)(script_flags | 8);
                }
                copy3(&desired, object + 0x74);
                moving = 1;
            } else if (!moving) {
                goto flags;
            }
            break;
        }
        default:
            goto flags;
        }

        // 0x4254ee: normalize, keep off the up axis
        {
            float length = (float)sqrt((double)(desired.k * desired.k + desired.j * desired.j + desired.i * desired.i));
            float along_up;

            if (!(fabs((double)length) < 0.0001)) {
                float inverse = 1.0f / length;

                desired.i *= inverse;
                desired.j *= inverse;
                desired.k *= inverse;
            }
            along_up = up.k * desired.k + up.j * desired.j + up.i * desired.i;
            if (along_up > 0.9f) {
                aligned = 1;
            }
            if (along_up < -0.9f) {
                copy3(&desired, object + 0x74);
            } else {
                real_vector3d side;

                side.i = up.j * desired.k - up.k * desired.j;
                side.j = up.k * desired.i - desired.k * up.i;
                side.k = desired.j * up.i - up.j * desired.i;
                desired.i = side.j * up.k - side.k * up.j;
                desired.j = side.k * up.i - up.k * side.i;
                desired.k = up.j * side.i - side.j * up.i;
                if (vector3d_normalize_with_length(&desired) == 0.0f) {
                    copy3(&desired, object + 0x74);
                }
            }
        }

        // 0x4256d0: spread away from nearby members (not for scripted directions)
        if (behaviour != 6) {
            float turn = 0.0f;
            real_point3d behind;
            real_vector3d side;
            int16_t other;

            behind.x = F(component, 0x4) - desired.i * 0.2f;
            behind.y = F(component, 0x8) - desired.j * 0.2f;
            behind.z = F(component, 0xc) - desired.k * 0.2f;
            side.i = up.j * desired.k - up.k * desired.j;
            side.j = up.k * desired.i - desired.k * up.i;
            side.k = desired.j * up.i - up.j * desired.i;
            if (I16(swarm, 0x2) > 0) {
                for (other = 0; other < I16(swarm, 0x2); other++) {
                    uint8_t *other_component;
                    float dx, dy, dz, distance_squared, facing;

                    if (other == member) {
                        continue;
                    }
                    other_component = COMPONENT(U32(swarm, 0x58 + other * 4));
                    dx = F(other_component, 0x4) - behind.x;
                    dy = F(other_component, 0x8) - behind.y;
                    dz = F(other_component, 0xc) - behind.z;
                    distance_squared = dz * dz + dy * dy + dx * dx;
                    if (!(distance_squared < 0.64000005f)) {
                        continue;
                    }
                    facing = (dz * desired.k + dy * desired.j + dx * desired.i) / (float)sqrt((double)distance_squared);
                    if (!(facing > 0.5f)) {
                        continue;
                    }
                    if (side.k * dz + side.j * dy + side.i * dx > 0.0f) {
                        turn = turn - (facing - 0.5f) * 0.5f;
                    } else {
                        turn = turn + (facing - 0.5f) * 0.5f;
                    }
                }
                if (turn != 0.0f) {
                    if (!(turn <= 1.0f)) {
                        turn = 1.5707964f;
                    } else if (turn < -1.0f) {
                        turn = -1.5707964f;
                    } else {
                        turn = turn * 1.5707964f;
                    }
                    vector3d_rotate_about_axis(&desired, &up, (float)sin((double)turn), (float)cos((double)turn));
                }
            }
        }

        // 0x425901: make the direction perpendicular to up
        {
            real_vector3d side;
            float length;

            side.i = up.j * desired.k - up.k * desired.j;
            side.j = up.k * desired.i - desired.k * up.i;
            side.k = desired.j * up.i - up.j * desired.i;
            length = (float)sqrt((double)(side.k * side.k + side.j * side.j + side.i * side.i));
            if (!(fabs((double)length) < 0.0001) && length != 0.0f) {
                float inverse = 1.0f / length;
                float side_k = inverse * side.k;

                side.i *= inverse;
                side.j *= inverse;
                desired.i = side.j * up.k - side_k * up.j;
                desired.j = side_k * up.i - up.k * side.i;
                desired.k = up.j * side.i - side.j * up.i;
            } else {
                copy3(&desired, object + 0x74);
            }
        }

    flags:
        // 0x425a50: firing and control flags
        flags = U16(component, 0x2);
        if (flags & 0x10) {
            fire = 1;
        } else if (moving && component[0x19] >= 0x2d && (member == (int16_t)picked || target_close || aligned)) {
            fire = 1;
        }
        if (flags & 2) {
            object[0x289] = (uint8_t)((flags & 1) << 2);
        } else {
            if (target_close && component[0x1a] == 0) {
                flags |= 1;
            } else {
                flags &= 0xfffe;
            }
            U16(component, 0x2) = flags;
            if (flags & 1) {
                object[0x289] = 3;
                U32(object, 0x4f4) = best_prop != 0 ? U32(best_prop, 0x18) : (uint32_t)k_datum_index_none;
            } else {
                object[0x289] = 0;
            }
        }

        // 0x425aeb: the unit control block
        memset(&control, 0, sizeof control);
        ((uint8_t *)&control)[0x0] = speed;
        ((uint8_t *)&control)[0x1] = control_byte_1;
        U16(&control, 0x2) = (uint16_t)(fire ? 2 : 0);
        I16(&control, 0x4) = -1;
        I16(&control, 0x6) = -1;
        I16(&control, 0x8) = -1;
        if (moving) {
            F(&control, 0xc) = 1.0f;
            F(&control, 0x10) = 0.0f;
            F(&control, 0x14) = 0.0f;
        } else {
            copy3((real_vector3d *)((uint8_t *)&control + 0xc), global_origin3d_pointer);
            copy3(&desired, object + 0x74);
        }
        *(real_vector3d *)((uint8_t *)&control + 0x1c) = desired;
        *(real_vector3d *)((uint8_t *)&control + 0x28) = desired;
        *(real_vector3d *)((uint8_t *)&control + 0x34) = desired;
        unit_apply_control_block(unit, &control, -1);
    }
}
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
