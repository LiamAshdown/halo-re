// projectile_response  (Ghidra: FUN_004bf390; renamed per
// out/phase4/projectiles_types_notes.md "Renames this pass establishes")
// address 0x4bf390, size 3554 bytes
// VERIFIED against disassembly 0x4bf390..0x4c0172 (2026-09-30). FIXED: the velocity fallback is the zero vector at
//   0x696714 (not up), attach-on-structure sets hit_ground | at_rest (0x14), the detonation_started effect test is
//   (!timer_started && (at_rest || attach)), the alignment / angle scores and speed / reflection sums follow the
//   original rounding order
// name confidence: 0.85   rewrite confidence: 0.65 (raised by the phase-4 verification pass, which re-derived
//   this function from `objdump -d -M intel bin/halo.exe` rather than from the decompilation;
//   the corrections it made are listed in src/projectiles/README.md)
//   the whole ProjectileResponse state machine against ProjectileMaterialResponse, with an
//   inlined RNG, an elided breakable-surface buffer, and several opaque effect-spawn calls)
// evidence: types/projectiles.h collision_result (every field below matches its documented
//   offset and evidence) and ProjectileMaterialResponse / ProjectileResponse in types/tags.h;
//   out/phase4/projectiles_types_notes.md's "collision_result" and "Misattributed functions"
//   sections. Ghidra's own signature already recovers all four parameters --
//   `FUN_004bf390(uint param_1, short *param_2, float *param_3, ...)` plus `float *in_EAX` --
//   which is what pins projectile_update's own call to this function down to
//   (object_index, hit, swept_target, velocity) with velocity in EAX. src/projectiles/
//   projectile_detonate.c (written in an earlier pass, out of this task's own address range but
//   sharing this function's material-response record and the same effect_new_with_color call) supplied
//   that function's confirmed 12-argument signature and the projectile_effect_coordinate_system
//   naming; reused verbatim here.
// register convention: object index on the stack (param_1), collision_result* on the stack
//   (param_2), the in/out predicted-position real_point3d* on the stack (param_3), the in/out
//   velocity real_vector3d* in EAX (in_EAX).
// blam-cc: stack -> (projectile_index, hit, out_position), EAX -> velocity (in/out)
// UNSURE (function-wide): the random-number draws below are the engine's LCG inlined directly
//   (matches src/math/random_real_range.c's own formula bit for bit) rather than calls to that
//   function, and are kept inlined rather than rewritten as calls to preserve the exact sequence
//   of seed advances relative to the surrounding arithmetic. RESOLVED this pass: breakable_surface_apply_damage's
//   buffer argument rides in EBX (0x4bf8d2 `lea ebx,[esp+0xa4]`), so the breakable-surface record
//   this function builds IS passed to it, and the two trailing slots damage_data calls
//   unknown_4c / unknown_50 carry the surface's material index and the
//   ProjectileMaterialResponse row it selects. effect_new_on_object_with_node_table, effect_new_with_color's
//   own trailing scalar/flag parameters, and vector3d_angle_between_4cd4f0's arguments are
//   preserved exactly as Ghidra shows them (several with no visible arguments at all), per this
//   task's priority of literal preservation over invented signatures.
//   RESOLVED this pass: vector3d_project_onto_axis's four register arguments, from
//   `objdump -d -M intel --start-address=0x4bfab1 --stop-address=0x4bfb00 bin/halo.exe` against
//   src/math/vector3d_project_onto_axis.c's own (ECX, EDX, ESI, EDI) convention -- see the call
//   site's comment. That also settles which local is parallel and which perpendicular:
//   ProjectileMaterialResponse.parallel_friction is at 0xa0-record offset 0x98 and scales the
//   ECX output, perpendicular_friction at 0x9c scales the EDI output.
// reconciled: R28 object.unknown_0c4 -> datum_index creator_object (same offset 0xc4)
// reconciled: R29 object/object_placement_data.name_index -> owner_team (int16 team at 0xb8 / 0x14)
// reconciled: R23 collision_result: normal -> plane.normal, unknown_30 -> plane.d, unknown_04 -> first_leaf/first_cluster, unknown_3c -> region_index, marker_index -> node_index, unknown_40 -> permutation_index (int16), unknown_48 -> plane_index, unknown_4d -> breakable_surface_index, unknown_4e -> collision_material_index
// reconciled: R25 damage_data.unknown_4c -> material_type (int16 collision material of the damaged surface, 0xffff = none; indexes DamageEffect +0x200)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"
#include "projectiles.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern data_array *object_data; // 0x008603b0
extern tag_instance *tag_instances; // 0x0087bc14
extern random_seed random_seed_global; // 0x00719cd0
extern real_vector3d *global_up3d_pointer; // 0x00696720
extern real_vector3d *global_down3d_pointer; // 0x0069672c
extern real_point3d *global_origin3d_pointer; // 0x00696714
extern char *projectile_effect_coordinate_system_names[5]; // 0x00695f80
extern ProjectileMaterialResponse projectile_default_material_response; // 0x00695e20
extern int16_t network_game_mode; // 0x00719720, 0 local, 1 client, 2 host

extern object *object_try_and_get(datum_index object_index, uint32_t type_mask); // 0x4f6ec0
extern void object_apply_damage(damage_data *dd, uint32_t object_index, int16_t node_index,
                                 int16_t region_index, int16_t material_index, uint32_t plane); // 0x4ee5e0
    // param_3 = collision_result.marker_index (the node hit), param_4 =
    // collision_result.unknown_3c, param_5 = collision_result.unknown_4e // 0x4ee5e0, see
    // src/objects/object_damage_apply_line_of_sight.c
extern void projectile_compute_deceleration(uint32_t object_index); // 0x4c0310, this module, out
    // of range (>0x4bf390), not rewritten this pass. UNSURE register: no visible arguments here
extern void object_unlink_cluster_or_notify_parent(uint32_t object_index); // 0x4f5de0
extern void object_set_cluster_and_parent(uint32_t object_index, bsp_leaf_reference *location); // 0x4f5c30
extern void object_attach_to_object(uint32_t parent_index, uint32_t child_index, int16_t marker_index); // 0x4f6440
extern void projectile_send_attach(datum_index projectile_index, datum_index parent_object_index, int16_t marker_index); // 0x4bf120, this batch
extern void projectile_request_state(datum_index projectile_index, int16_t requested_state); // 0x4bf0f0, this batch
extern real vector3d_normalize_with_length(real_vector3d *v); // 0x401990, vector in ECX
extern void vector3d_project_onto_axis(real_vector3d *parallel_out, real_vector3d *axis,
    real_vector3d *v, real_vector3d *perp_out); // 0x4cda90, src/math/vector3d_project_onto_axis.c
    // blam-cc: ECX -> parallel_out, EDX -> axis, ESI -> v, EDI -> perp_out
extern real vector3d_angle_between_4cd4f0(real_vector3d *a, real_vector3d *b); // 0x4cd4f0, ECX, EDX
extern real_vector3d *vector3d_randomize_direction(real_point3d *direction, real_vector3d *out, void *seed,
    real lo, real hi); // 0x4cd1b0, EAX, EBX, EDI, stack
extern datum_index effect_new_on_object_with_node_table(datum_index creator_object_index,
    datum_index definition_index, datum_index object_index, uint16_t node_index,
    uint16_t ctx_08, uint32_t ctx_0c, uint32_t ctx_10, uint32_t ctx_14, real a_scale,
    real b_scale, const void *color, const void *tint_source); // 0x450870, EAX, ECX, EDX, stack x9
extern void effect_new_with_color(uint32_t effect, uint32_t target_or_index, void *velocity, int32_t kind,
    char **labels, void *position_block, void *direction_block, real fade_in, real fade_out,
    int32_t color, int32_t tint_source, int32_t force_create); // 0x450980, world-position effect
    // spawn, established 12-argument form, see src/projectiles/projectile_detonate.c
extern void breakable_surface_apply_damage(damage_data *request, uint32_t packed_leaf_and_flags,
                         int32_t surface_index); // 0x4ffde0, breakable-surface damage; opaque,
    // out of range. blam-cc: EBX -> request (0x4bf8d2 `lea ebx,[esp+0xa4]`, the damage_data-shaped
    // buffer built just above), stack -> (packed_leaf_and_flags, surface_index).

// The whole ProjectileResponse state machine: applies object damage on a direct hit, looks up
// (or falls back to the default) ProjectileMaterialResponse row for the surface/object
// material, rerolls it against the row's "potential" override thresholds, updates the
// projectile's velocity and out_position for whichever response (disappear / detonate / reflect
// / overpenetrate / attach) was chosen, spawns the row's default and potential-material effects,
// and -- for an attach onto another object -- runs the same super-combining sibling sweep and
// network broadcast projectile_attach_apply's sender side depends on.
void projectile_response(datum_index projectile_index, collision_result *hit, real_point3d *out_position, real_vector3d *velocity)
{
    object *obj = ((object_header *)object_data->data)[projectile_index & 0xffff].data;
    Projectile *tag = (Projectile *)tag_instances[obj->definition_tag & 0xffff].data;
    projectile_data *pd = (projectile_data *)((uint8_t *)obj + k_projectile_data_offset);

    int16_t new_material_index = hit->material_type; // local_100's low 16 bits: the material the
        // sweep reported, possibly replaced by the one object_apply_damage resolves below
    real speed_fraction = 1.0f; // local_ec: how far along [final_velocity,initial_velocity] the
        // impact speed sat; feeds damage_data.random_blend and (for scale_effects_by = velocity)
        // the effect scale below
    real effect_scale = 1.0f;   // local_e8: the fade-in the effect spawns are given. It starts at
        // 1.0 and is ONLY recomputed (and only then clamped) for scale_effects_by 0 or 1 --
        // any other value leaves it at an unclamped 1.0
    real fade_out = 0.0f;       // local_e4
    real_vector3d unit_velocity = *velocity;
    ProjectileMaterialResponse *response;
    ProjectileResponse response_type;
    uint32_t response_effect_tag; // the other half of Ghidra's local_100: 0x4bf76c/0x4bf77e store
        // the chosen row's effect tag_id into it as a FULL dword, after 0x4bf5cc has already
        // consumed its low 16 bits as the material index. Keeping it in its own variable is what
        // stops the tag id being truncated to int16.
    real angle_score, alignment_score;
    real impact_speed; // the float10 vector3d_normalize_with_length returns for `unit_velocity`

    // The length the normalize call returns is the impact speed; it is reused below rather than
    // recomputed (Ghidra threads it through as the same float10).
    impact_speed = vector3d_normalize_with_length(&unit_velocity);
    if (0.0f == impact_speed) {
        unit_velocity = *global_up3d_pointer;
    }
    if (tag->final_velocity == tag->initial_velocity) {
        speed_fraction = 1.0f;
    } else {
        speed_fraction = (impact_speed - tag->final_velocity) / (tag->initial_velocity - tag->final_velocity);
        if (speed_fraction < 0.0f) {
            speed_fraction = 0.0f;
        } else if (1.0f < speed_fraction) {
            speed_fraction = 1.0f;
        }
    }

    if (hit->type == _collision_result_type_object && *(int32_t *)&tag->impact_damage.tag_id != -1) {
        damage_data dd;
        uint8_t *zero = (uint8_t *)&dd;
        int32_t i;
        for (i = 0; i < (int32_t)sizeof(dd); i++) {
            zero[i] = 0;
        }
        dd.flags |= 0x08; // TYPES-GAP: unnamed damage_data.flags bit
        // 0x4bf4d6-ish `mov WORD [esp+..],0xffff` twice: both of these start at -1, and
        // unknown_4c is read back after the call, so zeroing alone is NOT equivalent.
        dd.location_cluster_index = -1;
        dd.material_type = -1;
        dd.responsible_player = obj->owner_linkage;
        dd.responsible_object = (datum_index)obj->creator_object;
        dd.team_index = (int16_t)obj->owner_team;
        dd.epicentre = hit->point;
        dd.origin = hit->point;
        dd.direction = *velocity;
        dd.random_blend = speed_fraction;
        dd.multiplier = 1.0f;
        dd.damage_effect_tag = *(datum_index *)&tag->impact_damage.tag_id;

        vector3d_normalize_with_length(&dd.direction);
        object_apply_damage(&dd, hit->object_index, hit->node_index, hit->region_index, hit->collision_material_index, (uint32_t)&hit->plane.normal);

        // dd.unknown_4c is an in/out slot: object_apply_damage may write back a resolved
        // material index there (the "value out of damage_data after object damage adjusted it"
        // types/projectiles.h documents for material_response_index).
        if (dd.material_type != -1) {
            new_material_index = dd.material_type;
        }
        // 0x4bf5b6 `mov edx,[esp+0x90]` reads damage_data + 0x48, not + 0x44 (multiplier).
        fade_out = *(real *)&dd.remaining_vitality;
    }

    pd->material_response_index = new_material_index;
    if (new_material_index < 0 || tag->projectile_material_response.count <= (uint32_t)new_material_index) {
        response = &projectile_default_material_response;
    } else {
        response = (ProjectileMaterialResponse *)tag->projectile_material_response.pointer + new_material_index;
    }

    // Reroll the row's own default_response against velocity_noise (an inlined LCG draw, see
    // src/math/random_real_range.c for the equivalent named formula) and the row's
    // angular_noise-adjusted incidence angle.
    {
        uint32_t seed_step = random_seed_global * 0x19660d + 0x3c6ef35f;
        real angular_noise = response->angular_noise;
        random_seed_global = seed_step * 0x19660d + 0x3c6ef35f;
        // 0x4bf600..0x4bf63e: `fld vn / fchs` keeps -vn, then `fld st(1) / fchs / fmulp`
        // multiplies the draw by +vn and `fadd st,st(1)` adds the -vn, i.e. a uniform draw over
        // [-velocity_noise, 0). An earlier rewrite of this file negated the multiplicand too.
        // 0x4bf624..0x4bf661: ((r * k) * vn + -vn) - n.k * v.k - n.j * v.j - n.i * v.i, k = the float constant 0x672b84
        alignment_score = ((((real)(seed_step >> 0x10) * 1.5259022e-05f) * response->velocity_noise +
            -response->velocity_noise) - hit->plane.normal.k * velocity->k) - hit->plane.normal.j * velocity->j -
            hit->plane.normal.i * velocity->i;
        // 0x4bf68c..0x4bf6aa: ((noise - -noise) * (r2 * k) + -noise) + (angle - pi/2)
        angle_score = ((angular_noise - -angular_noise) * ((real)((random_seed_global >> 0x10) & 0xffff) * 1.5259022e-05f) +
            -angular_noise) + (vector3d_angle_between_4cd4f0((real_vector3d *)&hit->plane.normal, (real_vector3d *)velocity) - 1.5707964f);
        // FIXED (0x4bf637..0x4bf675): ECX = &hit->plane.normal (+0x24), EDX = the velocity (ESI)
    }

    if (response->potential_response == 0 ||
        // 0x4bf6c4 / 0x4bf6fa `jnp`: each range pair is only tested when its upper bound is
        // non-zero, and "out of range" means below the lower bound or above the upper one.
        (response->potential_between[1] != 0.0f &&
         (angle_score < response->potential_between[0] ||
          angle_score > response->potential_between[1])) ||
        (response->potential_and[1] != 0.0f &&
         (alignment_score < response->potential_and[0] ||
          alignment_score > response->potential_and[1])) ||
        ((response->potential_flags & 1) != 0 && // only_against_units
         (hit->type != _collision_result_type_object || object_try_and_get(hit->object_index, _object_mask_unit) == 0)) ||
        ((real)((random_seed_global = random_seed_global * 0x19660d + 0x3c6ef35f, random_seed_global) >> 0x10) *
             1.5259022e-05f < response->potential_skip_fraction)) {
        response_type = (ProjectileResponse)response->default_response;
        response_effect_tag = *(uint32_t *)&response->default_effect.tag_id;
    } else {
        response_type = (ProjectileResponse)response->potential_response;
        response_effect_tag = *(uint32_t *)&response->potential_effect.tag_id;
    }

    if (hit->type == _collision_result_type_structure && (hit->surface_flags & 0x08) != 0) {
        // The buffer is 0x54 bytes of damage_data plus two trailing slots the struct calls
        // unknown_4c / unknown_50; 0x4bf8d2 `lea ebx,[esp+0xa4]` is what hands it to
        // breakable_surface_apply_damage, so it IS passed -- by register, not on the stack.
        damage_data breakable_surface_damage;
        ProjectileMaterialResponse *surface_response;
        uint8_t *zero = (uint8_t *)&breakable_surface_damage;
        int32_t i;
        for (i = 0; i < (int32_t)sizeof(breakable_surface_damage); i++) {
            zero[i] = 0;
        }
        breakable_surface_damage.damage_effect_tag = *(datum_index *)&tag->impact_damage.tag_id;
        breakable_surface_damage.flags |= 0x08;
        breakable_surface_damage.responsible_player = (datum_index)0xffffffff;
        breakable_surface_damage.responsible_object = (datum_index)0xffffffff;
        breakable_surface_damage.team_index = -1;
        breakable_surface_damage.location_cluster_index = -1;
        breakable_surface_damage.material_type = -1;
        breakable_surface_damage.epicentre = hit->point;
        breakable_surface_damage.origin = hit->point;
        breakable_surface_damage.direction = *velocity;
        breakable_surface_damage.random_blend = 1.0f;
        breakable_surface_damage.multiplier = 1.0f;
        vector3d_normalize_with_length(&breakable_surface_damage.direction);

        // 0x4bf883..0x4bf8b8: the surface's own material index, and the row it selects (with the
        // same out-of-range fallback as above), are stored into the buffer's two trailing slots.
        breakable_surface_damage.material_type = hit->material_type;
        if (hit->material_type < 0 ||
            tag->projectile_material_response.count <= (uint32_t)hit->material_type) {
            surface_response = &projectile_default_material_response;
        } else {
            surface_response = (ProjectileMaterialResponse *)tag->projectile_material_response.pointer +
                               hit->material_type;
        }
        breakable_surface_damage.material_response = (uint32_t)surface_response;

        // 0x4bf8bc..0x4bf8d9: the bsp_leaf_reference pair is copied in as two dwords.
        breakable_surface_damage.location_leaf_index = *(int32_t *)&hit->leaf;
        *(uint32_t *)&breakable_surface_damage.location_cluster_index =
            *(uint32_t *)((uint8_t *)&hit->leaf + 4);

        breakable_surface_apply_damage(&breakable_surface_damage,
            (*(uint32_t *)&hit->leaf & 0xffff0000u) | (uint32_t)hit->breakable_surface_index,
            hit->surface_index);
    }

    *out_position = hit->point;

    if (response_type == projectileresponse_overpenetrate) {
        if (hit->type == _collision_result_type_water_surface) {
            obj->flags ^= _object_in_water_bit;
            projectile_compute_deceleration(projectile_index);
            out_position->x -= hit->plane.normal.i * 0.001f;
            out_position->y -= hit->plane.normal.j * 0.001f;
            out_position->z -= hit->plane.normal.k * 0.001f;
        } else if (hit->type != _collision_result_type_object) {
            if (tag->timer[1] == 0.0f) {
                response_type = projectileresponse_detonate;
            } else {
                // 0x4bf9c2: `or [proj+0x22c], 0x14` = hit_ground (0x04) | at_rest (0x10)
                pd->flags |= _projectile_hit_ground_bit | _projectile_at_rest_bit;
                response_type = projectileresponse_attach;
            }
            goto fall_back_to_up_vector;
        } else {
            real remaining = 1.0f - response->initial_friction;
            velocity->i *= remaining;
            velocity->j *= remaining;
            velocity->k *= remaining;
            pd->ignore_object_index = hit->object_index;
        }
    } else if (response_type == projectileresponse_reflect) {
        // Resolved by disassembly (0x4bfabb..0x4bfacc): ECX = &parallel_component
        // ([esp+0xf8], the one scaled by parallel_friction at tag 0x98), EDX = &hit->plane.normal
        // (`lea edx,[ebp+0x24]`), ESI = velocity, EDI = &perpendicular_component ([esp+0x104],
        // the one scaled by perpendicular_friction at tag 0x9c).
        real_vector3d parallel_component, perpendicular_component;
        vector3d_project_onto_axis(&parallel_component, &hit->plane.normal, velocity,
                                   &perpendicular_component);
        velocity->i = (1.0f - response->perpendicular_friction) * perpendicular_component.i -
            (1.0f - response->parallel_friction) * parallel_component.i;
        velocity->j = (1.0f - response->perpendicular_friction) * perpendicular_component.j -
            (1.0f - response->parallel_friction) * parallel_component.j;
        velocity->k = (1.0f - response->perpendicular_friction) * perpendicular_component.k -
            (1.0f - response->parallel_friction) * parallel_component.k;
    } else {
    fall_back_to_up_vector: // 0x4bf9d1: the velocity becomes the zero vector at 0x696714 (not the up vector)
        *velocity = *(real_vector3d *)global_origin3d_pointer;
    }

    if (response->angular_noise != 0.0f) {
        // FIXED (objdump 0x4bf9fe..0x4bfa11): EAX = EBX = the velocity (ESI), EDI = &random_seed_global, stack = (0,
        //   angular_noise). The draft passed (0, noise).
        vector3d_randomize_direction((real_point3d *)velocity, velocity, &random_seed_global, 0.0f,
            response->angular_noise);
    }
    {
        // vector3d_normalize_with_length RETURNS the pre-normalization length, and that length is
        // part of the new scale: speed = |velocity| + uniform(-velocity_noise, +velocity_noise).
        // An earlier rewrite of this file dropped the length term, which collapsed the speed to
        // the noise alone.
        real pre_length;
        if (response->velocity_noise != 0.0f &&
            (pre_length = vector3d_normalize_with_length(velocity)) != 0.0f) {
            real scale = (real)((random_seed_global = random_seed_global * 0x19660d + 0x3c6ef35f, random_seed_global) >> 0x10) *
                1.5259022e-05f * (response->velocity_noise - -response->velocity_noise) +
                -response->velocity_noise + pre_length;
            velocity->i *= scale;
            velocity->j *= scale;
            velocity->k *= scale;
        }
    }

    {
        real speed_sq = (velocity->i * velocity->i + velocity->j * velocity->j) + velocity->k * velocity->k;
        if (response_type != projectileresponse_attach && speed_sq < tag->minimum_velocity * tag->minimum_velocity) {
            projectile_request_state(projectile_index, _projectile_state_detonating);
        }
        if (speed_sq < 0.0001f) {
            pd->flags |= _projectile_at_rest_bit;
            if (0.3f < hit->plane.normal.k) {
                obj->flags |= _object_at_rest_bit;
                if (tag->timer[1] == 0.0f) {
                    projectile_request_state(projectile_index, _projectile_state_detonating);
                }
            }
        }
    }

    // Fade in/out the row's own effect-coordinate-system scale, then spawn the default effect
    // (5-name coordinate system, see the file header and types/projectiles.h's globals section).
    // 0x4bfc5x: only scale_effects_by 0 (velocity) and 1 (angle) assign local_e8, and the clamp
    // is inside those two branches (LAB_004bfc66) -- a third value leaves local_e8 at 1.0.
    if (response->scale_effects_by == 0) {
        effect_scale = speed_fraction;
        if (effect_scale < 0.0f) {
            effect_scale = 0.0f;
        } else if (1.0f < effect_scale) {
            effect_scale = 1.0f;
        }
    } else if (response->scale_effects_by == 1) {
        effect_scale = angle_score * 0.63661975f; // 2/pi
        if (effect_scale < 0.0f) {
            effect_scale = 0.0f;
        } else if (1.0f < effect_scale) {
            effect_scale = 1.0f;
        }
    }
    if (fade_out < 0.0f) {
        fade_out = 0.0f;
    } else if (1.0f < fade_out) {
        fade_out = 1.0f;
    }

    {
        real_vector3d reflected;
        real dot2 = 2.0f * ((unit_velocity.k * hit->plane.normal.k + unit_velocity.j * hit->plane.normal.j) +
            unit_velocity.i * hit->plane.normal.i);
        reflected.i = unit_velocity.i - dot2 * hit->plane.normal.i;
        reflected.j = unit_velocity.j - dot2 * hit->plane.normal.j;
        reflected.k = unit_velocity.k - dot2 * hit->plane.normal.k;

        real_vector3d coordinate_system[5]; // {normal, incident, negative incident, reflection, "gravity"/down}
        real_point3d positions[5];
        int32_t i;
        coordinate_system[0] = hit->plane.normal;
        coordinate_system[1].i = -unit_velocity.i;
        coordinate_system[1].j = -unit_velocity.j;
        coordinate_system[1].k = -unit_velocity.k;
        coordinate_system[2] = unit_velocity;
        coordinate_system[3] = reflected;
        coordinate_system[4] = *global_down3d_pointer;
        for (i = 0; i < 5; i++) {
            positions[i] = hit->point;
        }

        if (0.008333334f < alignment_score) {
            if (hit->type == _collision_result_type_object) {
                // FIXED (objdump 0x4bfdd6..0x4bfdfa): EAX = the projectile, ECX = the response effect, EDX = the hit object,
                //   then (node, 5, names, positions, coordinate system, scale, fade, 0, 0). The draft dropped the three
                //   register arguments.
                effect_new_on_object_with_node_table(projectile_index, response_effect_tag, hit->object_index,
                    (uint16_t)hit->node_index, 5, (uint32_t)projectile_effect_coordinate_system_names, (uint32_t)positions,
                    (uint32_t)coordinate_system, effect_scale, fade_out, 0, 0);
            } else {
                effect_new_with_color(response_effect_tag, projectile_index, 0, 5, projectile_effect_coordinate_system_names, positions, coordinate_system, effect_scale, fade_out, 0, 0, 1);
            }
        }
        // 0x4bfe2c..0x4bfe46: skipped when the detonation timer bit (0x20) is set; runs when at rest (0x10) or attaching
        if ((pd->flags & _projectile_detonation_timer_started_bit) == 0 &&
            ((pd->flags & _projectile_at_rest_bit) != 0 || response_type == projectileresponse_attach)) {
            if (hit->type == _collision_result_type_object) {
                // FIXED (objdump 0x4bfe4b..0x4bfe82): ECX = the tag's detonation_started effect (+0x200)
                effect_new_on_object_with_node_table(projectile_index, *(uint32_t *)&tag->detonation_started.tag_id,
                    hit->object_index, (uint16_t)hit->node_index, 5, (uint32_t)projectile_effect_coordinate_system_names,
                    (uint32_t)positions, (uint32_t)coordinate_system, effect_scale, fade_out, 0, 0);
            } else {
                effect_new_with_color(*(uint32_t *)&tag->detonation_started.tag_id, projectile_index, 0, 5, projectile_effect_coordinate_system_names, positions, coordinate_system, effect_scale, fade_out, 0, 0, 1);
            }
        }
    }

    if (response_type == projectileresponse_disappear) {
        projectile_request_state(projectile_index, _projectile_state_disappearing);
        return;
    }
    if (response_type == projectileresponse_detonate) {
        projectile_request_state(projectile_index, _projectile_state_detonating);
        return;
    }
    if (response_type != projectileresponse_attach) {
        return;
    }

    if (hit->type == _collision_result_type_object) {
        object *target = object_try_and_get(hit->object_index, _object_mask_all);
        if (network_game_mode != 0 && target != 0 && target->type == _object_type_biped &&
            (target->vitality_flags & _object_health_frozen_bit) != 0) {
            return;
        }
        if (obj->network_role == 1) {
            return;
        }
        if ((tag->projectile_flags & _projectile_definition_has_super_combining_explosion_bit) != 0) {
            object *parent = ((object_header *)object_data->data)[hit->object_index & 0xffff].data;
            datum_index sibling_index = parent->first_child_object;
            int16_t sibling_count = 0;
            while (sibling_index != (datum_index)0xffffffff) {
                object *sibling = ((object_header *)object_data->data)[sibling_index & 0xffff].data;
                projectile_data *sibling_pd = (projectile_data *)((uint8_t *)sibling + k_projectile_data_offset);
                if (sibling->definition_tag == obj->definition_tag &&
                    (sibling_pd->flags & _projectile_super_detonation_counted_bit) == 0) {
                    sibling_pd->arming_timer = 0.0f;
                    sibling_pd->detonation_timer = 0.0f;
                    sibling_count++;
                }
                if (k_projectile_super_combine_attach_threshold < sibling_count) {
                    pd->flags |= _projectile_super_detonation_bit;
                    break;
                }
                sibling_index = sibling->next_object;
            }
        }
    }

    velocity->i = 0.0f;
    velocity->j = 0.0f;
    velocity->k = 0.0f;
    obj->angular_velocity.i = 0.0f;
    obj->angular_velocity.j = 0.0f;
    obj->angular_velocity.k = 0.0f;
    pd->flags |= _projectile_attached_bit;
    obj->flags |= _object_at_rest_bit;

    object_unlink_cluster_or_notify_parent(projectile_index);
    obj->position = *out_position;
    object_set_cluster_and_parent(projectile_index, &hit->leaf);
    if (hit->type == _collision_result_type_object) {
        object_attach_to_object(hit->object_index, projectile_index, hit->node_index);
    }

    if ((tag->projectile_flags & _projectile_definition_detonation_max_time_if_attached_bit) != 0) {
        real t = tag->timer[1];
        if (1.0f <= t * 30.0f) {
            pd->detonation_timer_rate = 1.0f / (t * 30.0f);
        }
    } else if ((tag->projectile_flags & _projectile_definition_random_attached_detonation_time_bit) != 0) {
        real t = (real)((random_seed_global = random_seed_global * 0x19660d + 0x3c6ef35f, random_seed_global) >> 0x10) *
            1.5259022e-05f * (tag->timer[1] - tag->timer[0]) + tag->timer[0];
        if (1.0f <= t * 30.0f) {
            pd->detonation_timer_rate = 1.0f / (t * 30.0f);
        }
    }

    if (hit->type != _collision_result_type_object) {
        return;
    }
    if (obj->network_role != 0) {
        return;
    }
    if (((object_header *)object_data->data)[hit->object_index & 0xffff].data->network_role != 0) {
        return;
    }
    projectile_send_attach(projectile_index, hit->object_index, hit->node_index);
    obj->flags |= _object_changed_bit; // 0x4000000, "has broadcast an attach"
}

#if 0
Original Ghidra decompilation (0x4bf390):

void FUN_004bf390(uint param_1,short *param_2,float *param_3)

{
  uint *puVar1;
  float fVar2;
  float fVar3;
  float fVar4;
  undefined *puVar5;
  float *in_EAX;
  int iVar6;
  uint uVar7;
  uint *puVar8;
  int iVar9;
  int iVar10;
  short sVar11;
  float *pfVar12;
  undefined *puVar13;
  float10 fVar14;
  undefined4 local_100;
  float local_ec;
  float local_e8;
  float local_e4;
  float local_e0;
  float local_dc;
  float local_d8;
  float local_c8;
  uint local_c4;
  uint local_c0;
  float local_bc;
  float local_b8;
  float local_b4;
  float local_b0;
  float local_ac;
  float local_a8;
  float local_a4;
  float local_a0;
  float local_9c;
  undefined4 local_98;
  float local_94;
  float local_90;
  float local_8c;
  float local_88;
  undefined4 local_84;
  float local_80;
  short local_7c;
  undefined2 uStack_7a;
  uint local_70 [4];
  undefined2 local_60;
  undefined4 local_5c;
  undefined4 local_58;
  undefined4 local_54;
  undefined4 local_50;
  undefined4 local_4c;
  undefined4 local_48;
  undefined4 local_44;
  undefined4 local_40;
  float local_3c;
  float local_38;
  float local_34;
  undefined4 local_30;
  undefined4 local_2c;
  short local_24;
  undefined *local_20;
  float local_18;
  float local_14;
  float local_10;
  float local_c;
  float local_8;
  float local_4;

  iVar6 = (param_1 & 0xffff) * 0xc;
  puVar1 = *(uint **)(*(int *)(DAT_008603b0 + 0x34) + 8 + iVar6);
  local_100._0_2_ = param_2[0x1a];
  iVar9 = *(int *)((*puVar1 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
  local_e0 = *in_EAX;
  local_dc = in_EAX[1];
  local_d8 = in_EAX[2];
  local_e8 = 1.0;
  local_e4 = 0.0;
  fVar14 = (float10)vector3d_normalize_with_length();
  if (fVar14 == (float10)0.0) {
    local_e0 = *(float *)PTR_DAT_00696720;
    local_dc = *(float *)(PTR_DAT_00696720 + 4);
    local_d8 = *(float *)(PTR_DAT_00696720 + 8);
  }
  if (*(float *)(iVar9 + 0x1e8) == *(float *)(iVar9 + 0x1e4)) {
LAB_004bf49e:
    local_ec = 1.0;
  }
  else {
    fVar14 = (fVar14 - (float10)*(float *)(iVar9 + 0x1e8)) /
             ((float10)*(float *)(iVar9 + 0x1e4) - (float10)*(float *)(iVar9 + 0x1e8));
    local_ec = (float)fVar14;
    if ((float10)0.0 <= fVar14) {
      if (1.0 < local_ec) goto LAB_004bf49e;
    }
    else {
      local_ec = 0.0;
    }
  }
  if ((*param_2 == 3) && (fVar2 = *(float *)(iVar9 + 0x230), fVar2 != -NAN)) {
    pfVar12 = &local_c8;
    for (iVar10 = 0x15; iVar10 != 0; iVar10 = iVar10 + -1) {
      *pfVar12 = 0.0;
      pfVar12 = pfVar12 + 1;
    }
    local_c4 = local_c4 | 8;
    _local_7c = CONCAT22(uStack_7a,0xffff);
    local_b0 = (float)CONCAT22(local_b0._2_2_,0xffff);
    local_c0 = puVar1[0x30];
    local_bc = (float)puVar1[0x31];
    local_88 = local_ec;
    local_b8 = (float)CONCAT22(local_b8._2_2_,(short)puVar1[0x2e]);
    local_a0 = *(float *)(param_2 + 0xc);
    local_9c = *(float *)(param_2 + 0xe);
    local_98 = *(undefined4 *)(param_2 + 0x10);
    local_ac = *(float *)(param_2 + 0xc);
    local_a8 = *(float *)(param_2 + 0xe);
    local_a4 = *(float *)(param_2 + 0x10);
    local_94 = *in_EAX;
    local_90 = in_EAX[1];
    local_8c = in_EAX[2];
    local_84 = 0x3f800000;
    local_c8 = fVar2;
    vector3d_normalize_with_length();
    object_apply_damage(&local_c8,*(undefined4 *)(param_2 + 0x1c),param_2[0x1f],param_2[0x1e],
                        param_2[0x27],param_2 + 0x12);
    if (local_7c != -1) {
      local_100._0_2_ = local_7c;
    }
    local_e4 = local_80;
  }
  *(short *)((int)puVar1 + 0x232) = (short)local_100;
  if (((short)local_100 < 0) || (*(int *)(iVar9 + 0x240) <= (int)(short)local_100)) {
    puVar13 = &DAT_00695e20;
  }
  else {
    puVar13 = (undefined *)((short)local_100 * 0xa0 + *(int *)(iVar9 + 0x244));
  }
  uVar7 = random_seed_global * 0x19660d + 0x3c6ef35f;
  fVar3 = *(float *)(puVar13 + 0x60);
  random_seed_global = uVar7 * 0x19660d + 0x3c6ef35f;
  fVar4 = (((--*(float *)(puVar13 + 100) * (float)(uVar7 >> 0x10) * 1.5259022e-05 +
            -*(float *)(puVar13 + 100)) - *(float *)(param_2 + 0x16) * in_EAX[2]) -
          *(float *)(param_2 + 0x14) * in_EAX[1]) - *(float *)(param_2 + 0x12) * *in_EAX;
  fVar2 = *(float *)(puVar13 + 0x60);
  fVar14 = (float10)vector3d_angle_between_4cd4f0();
  fVar2 = (float)((float10)(random_seed_global >> 0x10) * (float10)1.5259022e-05 *
                  ((float10)fVar3 - (float10)-fVar2) + (float10)-fVar2 +
                 (fVar14 - (float10)1.5707964));
  if ((((*(short *)(puVar13 + 0x24) == 0) ||
       ((*(float *)(puVar13 + 0x30) != 0.0 &&
        ((fVar2 < *(float *)(puVar13 + 0x2c) ||
         (fVar2 < *(float *)(puVar13 + 0x30) == (fVar2 == *(float *)(puVar13 + 0x30)))))))) ||
      ((*(float *)(puVar13 + 0x38) != 0.0 &&
       ((fVar4 < *(float *)(puVar13 + 0x34) ||
        (fVar4 < *(float *)(puVar13 + 0x38) == (fVar4 == *(float *)(puVar13 + 0x38)))))))) ||
     ((((puVar13[0x26] & 1) != 0 &&
       ((*param_2 != 3 || (iVar10 = object_try_and_get(3), iVar10 == 0)))) ||
      (random_seed_global = random_seed_global * 0x19660d + 0x3c6ef35f,
      (float)(random_seed_global >> 0x10) * 1.5259022e-05 < *(float *)(puVar13 + 0x28))))) {
    sVar11 = *(short *)(puVar13 + 2);
    local_100 = *(undefined4 *)(puVar13 + 0x10);
  }
  else {
    sVar11 = *(short *)(puVar13 + 0x24);
    local_100 = *(undefined4 *)(puVar13 + 0x48);
  }
  if ((*param_2 == 2) && ((*(byte *)(param_2 + 0x26) & 8) != 0)) {
    puVar8 = local_70;
    for (iVar10 = 0x15; iVar10 != 0; iVar10 = iVar10 + -1) {
      *puVar8 = 0;
      puVar8 = puVar8 + 1;
    }
    local_70[0] = *(uint *)(iVar9 + 0x230);
    local_24 = 0xffff;
    local_70[2] = 0xffffffff;
    local_70[3] = 0xffffffff;
    local_60 = 0xffff;
    local_58 = CONCAT22(local_58._2_2_,0xffff);
    local_70[1] = local_70[1] | 8;
    local_48 = *(undefined4 *)(param_2 + 0xc);
    local_44 = *(undefined4 *)(param_2 + 0xe);
    local_40 = *(undefined4 *)(param_2 + 0x10);
    local_54 = *(undefined4 *)(param_2 + 0xc);
    local_50 = *(undefined4 *)(param_2 + 0xe);
    local_4c = *(undefined4 *)(param_2 + 0x10);
    local_3c = *in_EAX;
    local_38 = in_EAX[1];
    local_34 = in_EAX[2];
    local_30 = 0x3f800000;
    local_2c = 0x3f800000;
    vector3d_normalize_with_length();
    local_24 = param_2[0x1a];
    if ((local_24 < 0) || (*(int *)(iVar9 + 0x240) <= (int)local_24)) {
      local_20 = &DAT_00695e20;
    }
    else {
      local_20 = (undefined *)(local_24 * 0xa0 + *(int *)(iVar9 + 0x244));
    }
    local_5c = *(undefined4 *)(param_2 + 6);
    local_58 = *(undefined4 *)(param_2 + 8);
    FUN_004ffde0(CONCAT22((short)((uint)local_5c >> 0x10),(ushort)*(byte *)((int)param_2 + 0x4d)),
                 *(undefined4 *)(param_2 + 0x22));
  }
  *param_3 = *(float *)(param_2 + 0xc);
  param_3[1] = *(float *)(param_2 + 0xe);
  param_3[2] = *(float *)(param_2 + 0x10);
  if (sVar11 == 3) {
    if (*param_2 == 0) {
      uVar7 = puVar1[4];
      if ((uVar7 & 0x10) == 0) {
        uVar7 = uVar7 | 0x10;
      }
      else {
        uVar7 = uVar7 & 0xffffffef;
      }
      puVar1[4] = uVar7;
      FUN_004c0310();
      *param_3 = *param_3 - *(float *)(param_2 + 0x12) * 0.001;
      param_3[1] = param_3[1] - *(float *)(param_2 + 0x14) * 0.001;
      param_3[2] = param_3[2] - *(float *)(param_2 + 0x16) * 0.001;
    }
    else {
      if (*param_2 != 3) {
        if (*(float *)(iVar9 + 0x1c0) == 0.0) {
          sVar11 = 1;
        }
        else {
          puVar1[0x8b] = puVar1[0x8b] | 0x14;
          sVar11 = 4;
        }
        goto LAB_004bf9d1;
      }
      fVar3 = 1.0 - *(float *)(puVar13 + 0x90);
      *in_EAX = fVar3 * *in_EAX;
      in_EAX[1] = fVar3 * in_EAX[1];
      in_EAX[2] = fVar3 * in_EAX[2];
      puVar1[0x8d] = *(uint *)(param_2 + 0x1c);
    }
  }
  else if (sVar11 == 2) {
    vector3d_project_onto_axis();
    *in_EAX = (1.0 - *(float *)(puVar13 + 0x9c)) * local_c -
              (1.0 - *(float *)(puVar13 + 0x98)) * local_18;
    in_EAX[1] = (1.0 - *(float *)(puVar13 + 0x9c)) * local_8 -
                (1.0 - *(float *)(puVar13 + 0x98)) * local_14;
    in_EAX[2] = (1.0 - *(float *)(puVar13 + 0x9c)) * local_4 -
                (1.0 - *(float *)(puVar13 + 0x98)) * local_10;
  }
  else {
LAB_004bf9d1:
    puVar5 = PTR_DAT_00696714;
    *in_EAX = *(float *)PTR_DAT_00696714;
    in_EAX[1] = *(float *)(puVar5 + 4);
    in_EAX[2] = *(float *)(puVar5 + 8);
  }
  if (*(float *)(puVar13 + 0x60) != 0.0) {
    vector3d_randomize_direction(0,*(undefined4 *)(puVar13 + 0x60));
  }
  if ((*(float *)(puVar13 + 100) != 0.0) &&
     (fVar14 = (float10)vector3d_normalize_with_length(), fVar14 != (float10)0.0)) {
    random_seed_global = random_seed_global * 0x19660d + 0x3c6ef35f;
    fVar14 = (float10)(random_seed_global >> 0x10) * (float10)1.5259022e-05 *
             ((float10)*(float *)(puVar13 + 100) - -(float10)*(float *)(puVar13 + 100)) +
             -(float10)*(float *)(puVar13 + 100) + fVar14;
    *in_EAX = (float)(fVar14 * (float10)*in_EAX);
    in_EAX[1] = (float)(fVar14 * (float10)in_EAX[1]);
    in_EAX[2] = (float)(fVar14 * (float10)in_EAX[2]);
  }
  fVar3 = in_EAX[2] * in_EAX[2] + in_EAX[1] * in_EAX[1] + *in_EAX * *in_EAX;
  if (((sVar11 != 4) && (fVar3 < *(float *)(iVar9 + 0x1c4) * *(float *)(iVar9 + 0x1c4))) &&
     (iVar10 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + iVar6), *(short *)(iVar10 + 0x230) < 1))
  {
    *(undefined2 *)(iVar10 + 0x230) = 1;
  }
  if (((fVar3 < 0.0001) && (puVar1[0x8b] = puVar1[0x8b] | 0x10, 0.3 < *(float *)(param_2 + 0x16)))
     && ((puVar1[4] = puVar1[4] | 0x20, *(float *)(iVar9 + 0x1c0) == 0.0 &&
         (iVar10 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + iVar6),
         *(short *)(iVar10 + 0x230) < 1)))) {
    *(undefined2 *)(iVar10 + 0x230) = 1;
  }
  if (*(short *)(puVar13 + 0x5c) == 0) {
    local_e8 = local_ec;
LAB_004bfc66:
    if (0.0 <= local_e8) {
      if (1.0 < local_e8) {
        local_e8 = 1.0;
      }
    }
    else {
      local_e8 = 0.0;
    }
  }
  else if (*(short *)(puVar13 + 0x5c) == 1) {
    local_e8 = fVar2 * 0.63661975;
    goto LAB_004bfc66;
  }
  if (0.0 <= local_e4) {
    if (1.0 < local_e4) {
      local_e4 = 1.0;
    }
  }
  else {
    local_e4 = 0.0;
  }
  local_bc = local_e0 * -1.0;
  local_b0 = local_e0;
  local_ac = local_dc;
  local_b8 = local_dc * -1.0;
  local_98 = *(undefined4 *)PTR_DAT_0069672c;
  local_a8 = local_d8;
  local_94 = *(float *)(PTR_DAT_0069672c + 4);
  pfVar12 = (float *)(param_2 + 0x12);
  local_b4 = local_d8 * -1.0;
  local_90 = *(float *)(PTR_DAT_0069672c + 8);
  local_c8 = *pfVar12;
  local_c4 = *(uint *)(param_2 + 0x14);
  local_c0 = *(uint *)(param_2 + 0x16);
  iVar10 = 5;
  fVar2 = local_e0 * *pfVar12 +
          local_dc * *(float *)(param_2 + 0x14) + local_d8 * *(float *)(param_2 + 0x16);
  fVar2 = fVar2 + fVar2;
  local_a4 = local_e0 - fVar2 * *pfVar12;
  local_a0 = local_dc - fVar2 * *(float *)(param_2 + 0x14);
  local_9c = local_d8 - fVar2 * *(float *)(param_2 + 0x16);
  puVar8 = local_70;
  do {
    *puVar8 = *(uint *)(param_2 + 0xc);
    uVar7 = *(uint *)(param_2 + 0x10);
    puVar8[1] = *(uint *)(param_2 + 0xe);
    iVar10 = iVar10 + -1;
    puVar8[2] = uVar7;
    puVar8 = puVar8 + 3;
  } while (iVar10 != 0);
  if (0.008333334 < fVar4) {
    if (*param_2 == 3) {
      FUN_00450870(param_2[0x1f],5,&PTR_s_normal_00695f80,local_70,&local_c8,local_e8,local_e4,0,0);
    }
    else {
      FUN_00450980(local_100,param_1,0,5,&PTR_s_normal_00695f80,local_70,&local_c8,local_e8,local_e4
                   ,0,0,1);
    }
  }
  if (((puVar1[0x8b] & 0x20) == 0) && (((puVar1[0x8b] & 0x10) != 0 || (sVar11 == 4)))) {
    if (*param_2 == 3) {
      FUN_00450870(param_2[0x1f],5,&PTR_s_normal_00695f80,local_70,&local_c8,local_e8,local_e4,0,0);
    }
    else {
      FUN_00450980(*(undefined4 *)(iVar9 + 0x200),param_1,0,5,&PTR_s_normal_00695f80,local_70,
                   &local_c8,local_e8,local_e4,0,0,1);
    }
  }
  if (sVar11 == 0) {
    iVar9 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + iVar6);
    sVar11 = 2;
LAB_004c0157:
    if (*(short *)(iVar9 + 0x230) < sVar11) {
      *(short *)(iVar9 + 0x230) = sVar11;
    }
    return;
  }
  if (sVar11 == 1) {
    iVar9 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + iVar6);
    sVar11 = 1;
    goto LAB_004c0157;
  }
  if (sVar11 != 4) {
    return;
  }
  if (*param_2 == 3) {
    if ((((DAT_00719720 != 0) && (iVar10 = object_try_and_get(0xffffffff), iVar10 != 0)) &&
        (*(short *)(iVar10 + 0xb4) == 0)) && ((*(byte *)(iVar10 + 0x106) & 4) != 0)) {
      return;
    }
    iVar10 = DAT_008603b0;
    if (puVar1[1] == 1) {
      return;
    }
    if ((*(byte *)(iVar9 + 0x17c) & 8) != 0) {
      sVar11 = 0;
      uVar7 = *(uint *)(*(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 +
                                (*(uint *)(param_2 + 0x1c) & 0xffff) * 0xc) + 0x118);
      while (uVar7 != 0xffffffff) {
        puVar8 = *(uint **)(*(int *)(iVar10 + 0x34) + 8 + (uVar7 & 0xffff) * 0xc);
        if ((*puVar8 == *puVar1) && ((puVar8[0x8b] & 0x40) == 0)) {
          puVar8[0x92] = 0;
          puVar8[0x90] = 0;
          sVar11 = sVar11 + 1;
        }
        if (5 < sVar11) {
          puVar1[0x8b] = puVar1[0x8b] | 0x80;
          break;
        }
        uVar7 = puVar8[0x45];
      }
    }
  }
  puVar13 = PTR_DAT_00696714;
  puVar1[0x1a] = *(uint *)PTR_DAT_00696714;
  puVar1[0x1b] = *(uint *)(puVar13 + 4);
  puVar1[0x1c] = *(uint *)(puVar13 + 8);
  puVar1[0x23] = *(uint *)puVar13;
  puVar1[0x24] = *(uint *)(puVar13 + 4);
  puVar1[0x25] = *(uint *)(puVar13 + 8);
  iVar10 = DAT_008603b0;
  puVar1[0x8b] = puVar1[0x8b] | 8;
  puVar1[4] = puVar1[4] | 0x20;
  iVar6 = *(int *)(*(int *)(iVar10 + 0x34) + 8 + iVar6);
  object_unlink_cluster_or_notify_parent();
  *(float *)(iVar6 + 0x5c) = *param_3;
  *(float *)(iVar6 + 0x60) = param_3[1];
  *(float *)(iVar6 + 100) = param_3[2];
  object_set_cluster_and_parent(param_1,param_2 + 6);
  if (*param_2 == 3) {
    object_attach_to_object(*(undefined4 *)(param_2 + 0x1c),param_1,param_2[0x1f]);
  }
  if ((*(uint *)(iVar9 + 0x17c) & 4) == 0) {
    if ((*(uint *)(iVar9 + 0x17c) & 0x20) == 0) goto LAB_004c00cb;
    random_seed_global = random_seed_global * 0x19660d + 0x3c6ef35f;
    fVar2 = (float)(random_seed_global >> 0x10) * 1.5259022e-05 *
            (*(float *)(iVar9 + 0x1c0) - *(float *)(iVar9 + 0x1bc)) + *(float *)(iVar9 + 0x1bc);
  }
  else {
    fVar2 = *(float *)(iVar9 + 0x1c0);
  }
  if (1.0 <= fVar2 * 30.0) {
    puVar1[0x91] = (uint)(1.0 / (fVar2 * 30.0));
  }
LAB_004c00cb:
  if (*param_2 != 3) {
    return;
  }
  if (puVar1[1] != 0) {
    return;
  }
  if (*(int *)(*(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 +
                       (*(uint *)(param_2 + 0x1c) & 0xffff) * 0xc) + 4) != 0) {
    return;
  }
  FUN_004bf120(param_2[0x1f]);
  puVar1[4] = puVar1[4] | 0x4000000;
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
