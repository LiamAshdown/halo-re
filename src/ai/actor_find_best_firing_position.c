// actor_find_best_firing_position  (Ghidra: actor_find_best_firing_position, renamed)
// address 0x412ba0, size 4754 bytes -- the largest function in the ai module
// name confidence: 0.55  rewrite confidence: 0.35
// evidence: it walks ScenarioEncounter.firing_positions of the actor own encounter, keeps
//   every position whose group_index bit is in actor_firing_position_query.group_mask,
//   builds one 0x3c-byte actor_firing_position_candidate per surviving position (up to
//   0x200 of them on its own stack), fills in the path distances with path_find_run, runs
//   the scoring table at 0x006555c0, sorts, runs the rejection table at 0x006555f8 and
//   returns the winning firing position index. It is what fills in every trailing field of
//   the query, which is where most of that struct layout in types/ai.h comes from.
// register convention: all six arguments are the Ghidra-recognized stack parameters.
//
// UNSURE (this is the least settled rewrite in the module, 0.35):
//   * four callees -- encounter_build_firing_position_claims, actor_get_ranged_attack_vector, unit_get_aiming_vector and
//     actor_target_get_relationship_object -- are invoked with no visible arguments at all.
//     What is passed below is what the live values in the frame allow.
//   * actor+0xb8 is read as a prop handle. It falls inside actor.mode_data.raw (offset 0x1c),
//     which is consistent with mode 4 keeping a prop there, but is not independently proven.
//   * actor+0x270, which types/ai.h calls target_unit_index, is indexed into prop_data at
//     stride 0x138 here, i.e. it is a prop handle and not a unit object handle. Preserved as
//     the code reads it; see src/ai/README.md.
//   * the comparator the qsort is handed lives at 0x004127b0, which Ghidra never turned into
//     a function, so the sort order is inferred from how the scan below uses it.
//   * score is desirability, not cost: the scan keeps the LARGEST surviving score.
//
// Three tag reads settled themselves against types/tags.h while this was written and are
// no longer guesses: Actor.flags bit 31 is avoid_friends_line_of_fire and
// Actor.more_flags bit 0 is avoid_all_enemy_attack_vectors, which together are exactly the
// gate on gathering the ally / enemy aim hazards; Actor.more_flags bit 4 is
// pathfinding_ignores_danger, which is what suppresses the avoid sphere; and the two
// danger-sphere radii are Actor.old_position_avoid_dist and Actor.friend_avoid_dist.
// reconciled: R06 0x00746f9c is ScenarioStructureBSP *global_structure_bsp (was extern int32_t bsp_generation); ai.h path_find_context/actor_movement_context bsp_generation -> structure_bsp, bsp_index -> collision_bsp

#include "tags.h"
#include "cseries.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "cache.h"
#include "ai.h"
#include "units.h"

extern data_array *actor_data;      // 0x00880360
extern data_array *prop_data;       // 0x008802c0
extern tag_instance *tag_instances; // 0x0087bc14
extern Scenario *global_scenario;   // 0x00746f8c
extern ScenarioStructureBSP *global_structure_bsp; // 0x00746f9c, scenario.h (formerly bsp_generation)
extern uint32_t random_seed_global; // 0x00719cd0
extern const real_vector3d *global_origin3d_pointer; // 0x00696714

// The two globals the comparator at 0x004127b0 reads.
extern int16_t qsort_candidate_count;                        // 0x006f0c90
extern actor_firing_position_candidate *qsort_candidate_base;// 0x006f0c94

extern double sqrt(double x); // FSQRT, Ghidra SQRT() pseudo-function

extern real vector3d_normalize_with_length(real_vector3d *v);                // 0x401990
extern void actor_firing_position_run_score_rules(datum_index actor_index, uint16_t count, actor_firing_position_query *query, actor_firing_position_candidate *candidates); // 0x4126f0
extern uint8_t actor_firing_position_run_reject_rules(datum_index actor_index, actor_firing_position_query *query, actor_firing_position_candidate *candidate); // 0x412730
extern uint8_t actor_firing_position_probe_reject_rules(actor_firing_position_query *query, datum_index actor_index); // 0x412770, EDI, EBX
extern void actor_report_firing_position_request(datum_index actor_index, actor_firing_position_query *query, actor_firing_position_candidate *candidate); // 0x4120f0
extern uint8_t actor_firing_position_near_point(datum_index actor_index, real_point3d *point, int32_t start_surface_index, int16_t kind); // 0x412960
extern void encounter_build_firing_position_claims(datum_index encounter_index, datum_index *out_claims); // 0x4360d0, EAX, EBX
extern void actor_build_path_find_request(datum_index actor_index, path_find_request *request); // 0x41a9c0, EAX, EBX
extern void actor_target_get_relationship_object(datum_index target_prop_index); // 0x41f3a0, this module,
                                                 // blam-cc: EAX -> target_prop_index
extern uint8_t actor_get_ranged_attack_vector(datum_index target_prop_index, datum_index actor_index, real_vector3d *out_vector); // 0x420970, EAX, ECX, stack
extern uint8_t path_find_test_direct_reachability(const real_point3d *point_a, const real_point3d *point_b,
    real_point3d *out_position, void *context, uint8_t *out_success); // 0x43a0a0, EAX, ECX, ESI, stack
extern uint8_t path_find_compute_heuristic(path_find_context *context, uint32_t vertex_id, real_point3d *point,
    float *out_distance, float *out_secondary, real_vector3d *out_direction); // 0x43a310, EDI, EAX, stack
extern uint8_t path_find_run(path_find_context *context);                  // 0x43a8b0, not yet rewritten
extern void qsort_dword_array(uint32_t count, int32_t *elements, qsort_dword_compare_proc compare); // 0x449590, EAX count, ECX elements, stack compare
extern real point3d_distance_squared_to_segment(real_point3d *segment_start, real_vector3d *segment_direction, real_point3d *point); // 0x4cde30, EAX, ECX, EDX
extern void unit_add_marker_relative_offset(uint32_t unit_index, uint32_t mode, float *world_point,
    uint32_t reference_direction, uint32_t offsets, real_point3d *accumulator); // 0x569190, stack, EAX accumulator
extern void unit_get_aiming_vector(uint32_t unit_index, real_vector3d *out); // 0x5696f0, ECX, EAX

extern uint8_t actor_firing_position_compare(int32_t element, int32_t other); // 0x4127b0, src/ai/actor_firing_position_compare.c

// blam-cc: stack -> actor_index, query, out_candidate, out_previous_owner, path_context, out_path_ok
// Gathers, scores and picks the actor best firing position. Returns the index of the winning
// ScenarioFiringPosition inside the encounter block, or -1. out_candidate optionally receives
// a copy of the winning candidate record, out_previous_owner the actor that currently claims
// that position, and out_path_ok whether the movement pathfind that was run into path_context
// succeeded.
uint32_t actor_find_best_firing_position(datum_index actor_index,
                                         actor_firing_position_query *query,
                                         actor_firing_position_candidate *out_candidate,
                                         uint32_t *out_previous_owner,
                                         path_find_context *path_context,
                                         uint8_t *out_path_ok)
{
    actor *self;
    Actor *actor_definition;
    ActorVariant *variant;
    ScenarioEncounter *encounter_definition;
    ScenarioFiringPosition *firing_positions;
    prop *target;
    datum_index prop_index;

    path_find_request request;

    int32_t sort_index[512];
    uint32_t claims[512];
    actor_firing_position_candidate candidates[512];
    path_find_context target_context;

    int16_t candidate_count;
    uint32_t best_index;
    float best_score;
    uint8_t any_in_range;
    uint8_t truncated;
    real_point3d hazard_direction;
    real_vector3d delta;
    float distance_squared;
    float length;
    float owner_distance;
    float self_distance;
    uint32_t owner;
    uint32_t *clear;
    int32_t i;
    int32_t n;
    int16_t k;

    self = (actor *)((uint8_t *)actor_data->data + (actor_index & 0xffff) * sizeof(actor));
    best_index = 0xffffffff;
    best_score = 0.0f;

    if (self->encounter_index == (datum_index)0xffffffff) {
        return 0xffffffff;
    }

    actor_definition = (Actor *)tag_instances[self->actor_definition_tag & 0xffff].data;
    variant = (ActorVariant *)tag_instances[self->actor_variant_tag & 0xffff].data;
    encounter_definition = &((ScenarioEncounter *)global_scenario->encounters.pointer)
                                [self->encounter_index & 0xffff];

    candidate_count = 0;
    any_in_range = 0;

    // UNSURE: bare call. It fills claims[] with, per firing position, the actor that
    // currently holds it.
    encounter_build_firing_position_claims(self->encounter_index, (datum_index *)claims); // 0x412c52: EAX = encounter, EBX = claims
    if (self->firing_position_index != -1) {
        claims[self->firing_position_index] = 0xffffffff; // the actor does not block itself
    }

    query->maximum_distance = (self->unknown_15e == 0) ? 15.0f : 80.0f;
    if (query->search_radius == 0.0f) {
        query->search_radius = query->maximum_distance;
    }
    query->unknown_42 = (uint8_t)(query->goal_kind == 5);
    query->have_target = 0;

    // ------------------------------------------------------------------ threat block
    if (query->have_explicit_target == 0) {
        prop_index = (datum_index)0xffffffff;
        if (self->mode == 4 && *(uint32_t *)&self->mode_data.raw[0x1c] != 0xffffffff) {
            prop_index = *(datum_index *)&self->mode_data.raw[0x1c];
        } else {
            prop_index = (datum_index)self->target_unit_index; // a prop handle here, see UNSURE
        }
        if (prop_index != (datum_index)0xffffffff) {
            target = &((prop *)prop_data->data)[prop_index & 0xffff];
            if (query->unknown_42 != 0 && target->kind > 1 && target->kind < 4) {
                actor_target_get_relationship_object(prop_index); // UNSURE: EAX at this call site not re-derived
            }
            query->have_target = 1;
            query->target_position = target->last_known_position;
            query->target_surface_index = *(uint32_t *)&((struct prop *)target)->path_surface_index;
            query->target_surface_point = *(real_point3d *)&((struct prop *)target)->ground_position.x;
            query->target_unknown_640 = ((struct prop *)target)->cluster_index;
            query->target_distance = target->distance;
            query->target_prop_index = prop_index;
            query->target_aim_position = *(real_point3d *)&((struct prop *)target)->unknown_104;
            query->target_relationship_object = target->relationship_object_index;
            query->target_unknown_658 = target->unknown_20;

            if (query->unknown_41 == 0 || ((struct prop *)target)->unknown_8c == -1) {
                query->target_lead_position = *(real_point3d *)&((struct prop *)target)->unknown_104;
            } else {
                query->target_lead_position = *(real_point3d *)&((struct prop *)target)->unknown_90;
            }

            if (target->kind > 3 && target->kind < 6) {
                query->have_target_vault_point = 1;
                query->target_vault_point = *(real_point3d *)&((struct prop *)target)->unknown_40;
            }

            query->target_is_large = (uint8_t)(query->goal_kind == 4 || query->goal_kind == 6);
        }
    } else {
        query->target_position = query->explicit_target_position;
        query->target_surface_point = query->explicit_target_position;
        query->target_unknown_640 = (int16_t)query->explicit_target_unknown_34;
        query->have_target = 1;
        query->target_surface_index = query->explicit_target_object;

        delta.i = query->target_position.x - self->body_position.x;
        delta.j = query->target_position.y - self->body_position.y;
        delta.k = query->target_position.z - self->body_position.z;
        query->target_prop_index = (datum_index)0xffffffff;
        query->target_relationship_object = -1;
        query->target_unknown_658 = 0.0f;
        query->target_distance = (float)sqrt((double)(delta.i * delta.i + delta.j * delta.j +
                                                      delta.k * delta.k));
        unit_add_marker_relative_offset(self->unit_index, 1, (float *)&query->target_position, 0, 0,
            &query->target_aim_position); // 0x412d5b: EAX = query +0x610
        query->target_lead_position = query->target_aim_position;
    }

    // ------------------------------------------------------------------ gun offsets
    if (variant->custom_stand_gun_offset.i * variant->custom_stand_gun_offset.i +
        variant->custom_stand_gun_offset.j * variant->custom_stand_gun_offset.j +
        variant->custom_stand_gun_offset.k * variant->custom_stand_gun_offset.k <= 0.0001f) {
        if (actor_definition->standing_gun_offset.i * actor_definition->standing_gun_offset.i +
            actor_definition->standing_gun_offset.j * actor_definition->standing_gun_offset.j +
            actor_definition->standing_gun_offset.k * actor_definition->standing_gun_offset.k <=
                0.0001f) {
            query->have_standing_gun_offset = 0;
        } else {
            query->have_standing_gun_offset = 1;
            query->standing_gun_offset = *(real_vector3d *)&actor_definition->standing_gun_offset;
        }
    } else {
        query->have_standing_gun_offset = 1;
        query->standing_gun_offset = *(real_vector3d *)&variant->custom_stand_gun_offset;
    }

    if (variant->custom_crouch_gun_offset.i * variant->custom_crouch_gun_offset.i +
        variant->custom_crouch_gun_offset.j * variant->custom_crouch_gun_offset.j +
        variant->custom_crouch_gun_offset.k * variant->custom_crouch_gun_offset.k <= 0.0001f) {
        if (actor_definition->crouching_gun_offset.i * actor_definition->crouching_gun_offset.i +
            actor_definition->crouching_gun_offset.j * actor_definition->crouching_gun_offset.j +
            actor_definition->crouching_gun_offset.k * actor_definition->crouching_gun_offset.k <=
                0.0001f) {
            query->have_crouching_gun_offset = 0;
        } else {
            query->have_crouching_gun_offset = 1;
            query->crouching_gun_offset = *(real_vector3d *)&actor_definition->crouching_gun_offset;
        }
    } else {
        query->have_crouching_gun_offset = 1;
        query->crouching_gun_offset = *(real_vector3d *)&variant->custom_crouch_gun_offset;
    }

    if (self->danger_type > 0 && self->unknown_287[0] != 0 &&
        self->danger_unknown_2d4 < self->danger_radius + 3.0f) {
        query->danger_active = 1;
    }
    query->flying = self->flying;
    if (self->unknown_15e == 4) {
        query->unknown_45 = 1;
        query->unknown_46 = 1;
    }

    // ------------------------------------------------------------------ danger spheres
    query->danger_sphere_count = 0;
    if (self->recognition_valid != 0) {
        query->danger_spheres[0].position = self->recognition_position;
        query->danger_spheres[query->danger_sphere_count].radius =
            actor_definition->old_position_avoid_dist;
        query->danger_sphere_count = query->danger_sphere_count + 1;
    }

    if (actor_definition->friend_avoid_dist > 0.0f &&
        (query->goal_kind == 0 || query->goal_kind == 3 || query->goal_kind == 6)) {
        datum_index p = self->first_prop;
        while (query->danger_sphere_count < 0x20 && p != (datum_index)0xffffffff) {
            prop *pr = &((prop *)prop_data->data)[p & 0xffff];
            p = pr->next_in_actor;
            if (pr->kind > 1 && pr->kind < 4 && pr->is_unit == 0 && pr->is_vault == 0 &&
                pr->is_parented == 0) {
                query->danger_spheres[query->danger_sphere_count].position =
                    pr->last_known_position;
                query->danger_spheres[query->danger_sphere_count].radius =
                    actor_definition->friend_avoid_dist;
                query->danger_sphere_count = query->danger_sphere_count + 1;
            }
        }
    }

    // ------------------------------------------------------------------ hazards
    query->hazard_count = 0;
    query->hazard_count_kind_01 = 0;
    query->hazard_count_kind_2 = 0;
    {
        uint8_t gather_hazards = 0;
        if ((actor_definition->flags & 0x80000000u) != 0 /* avoid_friends_line_of_fire */ && self->alert_level > 2 &&
            (int8_t)self->tally.group_a_total > 0) {
            gather_hazards = 1;
        }
        if ((actor_definition->more_flags & 1) != 0 /* avoid_all_enemy_attack_vectors */ && self->alert_level > 2) {
            gather_hazards = 1;
        }

        if (gather_hazards != 0 &&
            (query->goal_kind == 0 || query->goal_kind == 2 || query->goal_kind == 3 ||
             query->goal_kind == 6)) {
            datum_index p = self->first_prop;
            do {
                prop *pr;
                int16_t kind;
                datum_index current = p;
                if (p == (datum_index)0xffffffff) {
                    break;
                }
                pr = &((prop *)prop_data->data)[p & 0xffff];
                kind = pr->kind;
                p = pr->next_in_actor;
                if (kind > 1 && kind < 4 && pr->is_vault == 0) {
                    if (pr->is_unit == 0 &&
                        (pr->is_parented != 0 || pr->relationship_object_index == -1) &&
                        actor_get_ranged_attack_vector(current, actor_index, (real_vector3d *)&hazard_direction) != 0) { // 0x4132fc: EAX = the prop
                        actor_firing_position_hazard *h = &query->hazards[query->hazard_count];
                        h->kind = (int16_t)(pr->is_parented != 0);
                        h->position = pr->last_known_position;
                        h->direction.i = hazard_direction.x;
                        h->direction.j = hazard_direction.y;
                        h->direction.k = hazard_direction.z;
                        query->hazard_count = query->hazard_count + 1;
                        query->hazard_count_kind_01 = query->hazard_count_kind_01 + 1;
                    }
                    if (pr->is_unit != 0 &&
                        ((pr->engaged != 0 && (pr->is_parented != 0 || pr->unknown_12f != 0)) ||
                         (actor_definition->more_flags & 1) != 0 /* avoid_all_enemy_attack_vectors */)) {
                        actor_firing_position_hazard *h = &query->hazards[query->hazard_count];
                        h->kind = 2;
                        h->position = pr->last_known_position;
                        // 0x4133b7: ECX = the prop's relationship object (+0x110), else its object
                        unit_get_aiming_vector(pr->relationship_object_index != -1 ? (uint32_t)pr->relationship_object_index
                                                                                    : (uint32_t)pr->object_index,
                                               &h->direction);
                        query->hazard_count = query->hazard_count + 1;
                        query->hazard_count_kind_2 = query->hazard_count_kind_2 + 1;
                    }
                }
            } while (query->hazard_count < 0x20);
        }
    }

    // ------------------------------------------------------------------ candidate gather
    *out_path_ok = 0;
    truncated = 0;
    firing_positions = (ScenarioFiringPosition *)encounter_definition->firing_positions.pointer;

    for (i = 0; i < (int32_t)encounter_definition->firing_positions.count; i++) {
        ScenarioFiringPosition *fp = &firing_positions[i];
        actor_firing_position_candidate *c;

        if ((query->group_mask & (1u << (((uint8_t)fp->group_index) & 0x1f))) == 0) {
            continue;
        }
        if (query->flying == 0 && fp->surface_index == 0xffffffff) {
            continue;
        }
        if (query->goal_kind == 5 &&
            actor_firing_position_near_point(actor_index, (real_point3d *)fp,
                                             (int32_t)fp->surface_index, 1) == 0) {
            continue;
        }

        owner = claims[i];
        if (owner != 0xffffffff) {
            actor *owner_actor = (actor *)((uint8_t *)actor_data->data +
                                           (owner & 0xffff) * sizeof(actor));
            if (query->goal_kind == 4 && query->danger_sphere_count < 0x20) {
                query->danger_spheres[query->danger_sphere_count].position =
                    *(real_point3d *)fp;
                query->danger_spheres[query->danger_sphere_count].radius = 4.0f;
                query->danger_sphere_count = query->danger_sphere_count + 1;
            }
            owner_distance = (float)sqrt((double)(
                (fp->position.x - owner_actor->body_position.x) *
                    (fp->position.x - owner_actor->body_position.x) +
                (fp->position.y - owner_actor->body_position.y) *
                    (fp->position.y - owner_actor->body_position.y) +
                (fp->position.z - owner_actor->body_position.z) *
                    (fp->position.z - owner_actor->body_position.z)));
            if (owner_distance < 1.0f) {
                continue;
            }
            self_distance = (float)sqrt((double)(
                (fp->position.x - self->body_position.x) *
                    (fp->position.x - self->body_position.x) +
                (fp->position.y - self->body_position.y) *
                    (fp->position.y - self->body_position.y) +
                (fp->position.z - self->body_position.z) *
                    (fp->position.z - self->body_position.z)));
            if (owner_distance < self_distance + self_distance) {
                continue; // the current holder is meaningfully closer, leave it alone
            }
        }

        if (candidate_count >= 0x200) {
            if (truncated == 0) {
                truncated = 1;
            }
            continue;
        }

        c = &candidates[candidate_count];
        c->position = (uint32_t)(uint8_t *)fp;
        c->firing_position_index = (int16_t)i;
        c->request_result = 0;
        c->distance_from_actor = 3.4028235e+38f;
        c->direction_from_actor.i = global_origin3d_pointer->i;
        c->direction_from_actor.j = global_origin3d_pointer->j;
        c->direction_from_actor.k = global_origin3d_pointer->k;
        c->distance_from_target = 3.4028235e+38f;
        c->segment_distance = 3.4028235e+38f;
        c->direction_from_target.i = global_origin3d_pointer->i;
        c->direction_from_target.j = global_origin3d_pointer->j;
        c->direction_from_target.k = global_origin3d_pointer->k;
        c->distance_squared_to_target = 0.0f;
        c->score_before_rejects = 0.0f;
        c->score = 0.0f;
        c->valid = 1;
        c->rejected = 0;
        candidate_count = candidate_count + 1;
    }

    if (candidate_count == 0) {
        // Nothing to pick from: drop the recognition history and give up.
        self->recognition_cursor = 0;
        for (k = 0; k < 4; k++) {
            self->recognition[k].firing_position_index = -1;
        }
        if (self->recognition_valid != 0) {
            self->recognition_valid = 0;
        }
        return best_index & 0xffff;
    }

    // ------------------------------------------------------------------ distances from the target
    if (query->have_target != 0 && query->unknown_42 != 0) {
        if (query->flying == 0) {
            if (query->target_surface_index != 0xffffffff) {
                clear = (uint32_t *)&request;
                for (n = 0; n < 0x12; n++) {
                    clear[n] = 0;
                }
                request.pathfinding_radius = actor_definition->pathfinding_radius;
                request.ignores_glass = self->ignores_glass;
                request.unknown_08 = (datum_index)0xffffffff;
                request.unknown_0c = (datum_index)0xffffffff;
                request.start_position = query->target_surface_point;
                request.start_surface_index = query->target_surface_index;
                request.have_start = 1;
                request.have_limit = 1;
                request.limit_distance = 20.0f;

                clear = (uint32_t *)&target_context;
                for (n = 0; n < 0x4023; n++) {
                    clear[n] = 0;
                }
                target_context.structure_bsp = (uint32_t)global_structure_bsp;
                for (n = 0; n < 0x12; n++) {
                    ((uint32_t *)&target_context)[n] = ((uint32_t *)&request)[n];
                }
                target_context.unknown_48 = 0;
                path_find_run(&target_context);

                for (i = 0; i < candidate_count; i++) {
                    actor_firing_position_candidate *c = &candidates[i];
                    // 0x4138b7: EDI = the target context, EAX = the position's surface (+0x14)
                    path_find_compute_heuristic(&target_context, *(uint32_t *)((uint8_t *)c->position + 0x14),
                                 (real_point3d *)c->position, &c->distance_from_target, (float *)0,
                                 (query->want_direction_from_target != 0) ? &c->direction_from_target : 0);
                }
            }
        } else {
            for (i = 0; i < candidate_count; i++) {
                actor_firing_position_candidate *c = &candidates[i];
                real_point3d *p = (real_point3d *)c->position;
                delta.i = p->x - query->target_position.x;
                delta.j = p->y - query->target_position.y;
                delta.k = p->z - query->target_position.z;
                if (delta.j * delta.j + delta.k * delta.k + delta.i * delta.i < 400.0f &&
                    path_find_test_direct_reachability(p, &query->target_position, 0, global_structure_bsp, 0) != 0) { // 0x413787
                    c->distance_from_target = vector3d_normalize_with_length(&delta);
                    if (query->want_direction_from_target != 0) {
                        c->direction_from_target = delta;
                    }
                }
            }
        }
    }

    // ------------------------------------------------------------------ the movement pathfind
    if (query->flying == 0) {
        actor_build_path_find_request(actor_index, &request); // 0x4138f8: EAX = actor, EBX = the request
        request.have_limit = 1;
        request.limit_distance = query->search_radius;

        if (query->unknown_36 != 0 && query->have_target != 0) {
            request.avoid_object_index = (datum_index)0xffffffff;
            if (query->target_prop_index != (datum_index)0xffffffff) {
                request.avoid_object_index =
                    ((prop *)prop_data->data)[query->target_prop_index & 0xffff].object_index;
            }
            request.avoid_position = query->target_position;
            request.avoid_radius = query->unknown_3c;
            request.avoid_weight = query->unknown_38;
            request.have_avoid_sphere = 1;
        } else if (self->danger_type > 0 && (actor_definition->more_flags & 0x10) == 0 /* pathfinding_ignores_danger */) {
            request.avoid_position = self->flee_from_point;
            request.avoid_radius = self->danger_unknown_294;
            request.avoid_object_index = self->danger_object_index;
            request.avoid_weight = 10.0f;
            request.have_avoid_sphere = 1;
        }

        clear = (uint32_t *)path_context;
        for (n = 0; n < 0x4023; n++) {
            clear[n] = 0;
        }
        path_context->structure_bsp = (uint32_t)global_structure_bsp;
        for (n = 0; n < 0x12; n++) {
            ((uint32_t *)path_context)[n] = ((uint32_t *)&request)[n];
        }
        path_context->unknown_48 = 0;
        if (path_find_run(path_context) != 0) {
            *out_path_ok = 1;
        }
    }

    // ------------------------------------------------------------------ distances from the actor
    for (i = 0; i < candidate_count; i++) {
        actor_firing_position_candidate *c = &candidates[i];
        real_point3d *p = (real_point3d *)c->position;

        if (query->have_target != 0) {
            c->distance_squared_to_target =
                (p->y - query->target_position.y) * (p->y - query->target_position.y) +
                (p->z - query->target_position.z) * (p->z - query->target_position.z) +
                (p->x - query->target_position.x) * (p->x - query->target_position.x);
        }

        delta.i = p->x - self->body_position.x;
        delta.j = p->y - self->body_position.y;
        delta.k = p->z - self->body_position.z;
        distance_squared = delta.j * delta.j + delta.k * delta.k + delta.i * delta.i;

        if (distance_squared < query->search_radius * query->search_radius) {
            if (query->flying == 0) {
                // 0x413b66: EDI = the path context argument, EAX = the position's surface (+0x14)
                path_find_compute_heuristic(path_context, *(uint32_t *)((uint8_t *)p + 0x14), p,
                             &c->distance_from_actor, &c->segment_distance,
                             (query->danger_active != 0) ? &c->direction_from_actor : 0);
            } else {
                // 0x413ae5: EAX = the body position, ECX = the delta, EDX = the target position
                c->segment_distance = (float)sqrt((double)point3d_distance_squared_to_segment(
                    &self->body_position, &delta, &query->target_position));
                length = (float)sqrt((double)distance_squared);
                if (length < 0.0001f && length > -0.0001f) {
                    length = 0.0f;
                } else {
                    float inverse = 1.0f / length;
                    delta.i = delta.i * inverse;
                    delta.j = delta.j * inverse;
                    delta.k = delta.k * inverse;
                }
                c->distance_from_actor = length;
                if (query->danger_active != 0) {
                    c->direction_from_actor = delta;
                }
            }
        }

        if (query->search_radius <= c->distance_from_actor) {
            c->valid = 0;
        } else {
            any_in_range = 1;
        }
    }

    // ------------------------------------------------------------------ pick
    if (query->allow_random_fallback != 0 && any_in_range == 0) {
        actor_firing_position_candidate *c;
        uint32_t roll;

        random_seed_global = random_seed_global * 0x19660d + 0x3c6ef35f;
        roll = ((random_seed_global >> 0x10) * (uint32_t)(int32_t)candidate_count) >> 0x10;
        *out_path_ok = 0;
        best_index = roll;

        self->recognition_cursor = 0;
        for (k = 0; k < 4; k++) {
            self->recognition[k].firing_position_index = -1;
        }
        if (self->recognition_valid != 0) {
            self->recognition_valid = 0;
        }

        c = &candidates[(int16_t)roll];
        c->score = 0.0f;
        c->score_before_rejects = 0.0f;
        c->rejected = 0;
        c->valid = 1;
        actor_firing_position_run_score_rules(actor_index, 1, query, c);
        if (c->valid == 0) {
            return 0xffffffff;
        }
        if (query->have_target != 0) {
            actor_report_firing_position_request(actor_index, query, c);
        }
        c->score_before_rejects = c->score;
        c->valid = actor_firing_position_run_reject_rules(actor_index, query, c);
        if (c->valid == 0) {
            return 0xffffffff;
        }
    } else {
        actor_firing_position_run_score_rules(actor_index, (uint16_t)candidate_count, query,
                                              candidates);
        for (i = 0; i < candidate_count; i++) {
            sort_index[i] = i;
        }
        qsort_candidate_base = candidates;
        qsort_candidate_count = candidate_count;
        qsort_dword_array((uint32_t)(int32_t)candidate_count, sort_index, actor_firing_position_compare); // 0x413cf3..0x413d0f

        query->baseline_accept = actor_firing_position_probe_reject_rules(query, actor_index); // 0x413d14: EBX = actor

        for (n = 0; n < candidate_count; n++) {
            actor_firing_position_candidate *c;
            i = (int32_t)(int16_t)sort_index[n];
            c = &candidates[i];
            if (c->valid == 0) {
                break;
            }
            if (query->baseline_accept != 0 &&
                c->score + query->baseline_penalty <= best_score) {
                break; // nothing further down the sorted list can beat the best
            }
            if (query->have_target != 0) {
                actor_report_firing_position_request(actor_index, query, c);
            }
            c->score_before_rejects = c->score;
            if (actor_firing_position_run_reject_rules(actor_index, query, c) != 0 &&
                best_score < c->score) {
                best_index = (uint32_t)i;
                best_score = c->score;
            }
        }
    }

    if ((int16_t)best_index == -1) {
        return best_index;
    }
    if (out_candidate != (actor_firing_position_candidate *)0) {
        *out_candidate = candidates[(int16_t)best_index];
    }
    i = candidates[(int16_t)best_index].firing_position_index;
    if (out_previous_owner != (uint32_t *)0) {
        *out_previous_owner = claims[i];
    }
    return (uint32_t)(uint16_t)(int16_t)i;
}

#if 0
Original Ghidra decompilation (0x412ba0): see out/phase2/ai and `python tools/pack.py 0x412ba0`.
The body is 4754 bytes and the Ghidra listing runs past 400 lines with the whole 0x18918-byte
stack frame spelled out as individual locals; it is not reproduced here. The frame layout the
rewrite above is built on is:

  -0x18898  int   sort_index[512]           the qsort permutation
  -0x18098  uint  claims[512]               filled by FUN_004360d0, one actor per firing position
  -0x17898  byte  candidates[512][0x3c]     iStack_17898 / auStack_17894 / afStack_1788c /
                                            acStack_17868 / afStack_17860 are all views of this
  -0x10098  byte  path_find_context         the 0x4023-dword scratch the target-side pathfind uses
  -0x188e0  byte  request[0x48]             the path_find_request copied into both contexts

and the candidate views map as: iStack_17898 + i*0x3c is the record base, auStack_17894 is
base+4, afStack_1788c is base+0xc, acStack_17868 is base+0x30 and afStack_17860 is base+0x38.
#endif
