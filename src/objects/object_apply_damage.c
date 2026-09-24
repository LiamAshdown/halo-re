// object_apply_damage
// address 0x4ee5e0, size 2939 bytes
// name confidence: 0.75 (Ghidra-recovered name; out/phase4/objects_types_notes.md calls out
// "chimera__apply_damage 0x4ee5e0 ... is genuinely object_apply_damage")
// rewrite confidence: 0.2 (by far the largest and most opaque function in this batch)
// evidence: types/objects.h damage_data (all named fields), object (network_role 0x04,
// damage_owner 0x0f0, type 0xb4, next_object 0x114, first_child_object 0x118); types/tags.h
// DamageEffect (damage_lower_bound, damage_upper_bound[2], damage_vehicle_passthrough_penalty,
// damage_side_effect, damage_category, damage_flags), ModelCollisionGeometry (flags,
// indirect_damage_material 0x04, materials TagReflexive 0x234, nodes TagReflexive 0x28c),
// ModelCollisionGeometryMaterial (material_type 0x24), Unit.rider_damage_fraction (0x184
// relative to the Object tag base, i.e. Unit+8); types/objects.h object_random_seed.
// UNSURE (extensive, function-wide): this is the central damage-dispatch routine and it reaches
// deep into the unit extension (object+0x218, +0x1f8, +0x328, +0x2f0(?), +0x2e, +0xc9, +0x38,
// +0x39 relative to a *word*-indexed object pointer, i.e. byte offsets 0x8c8 etc — none of those
// are part of the common `object` struct documented in types/objects.h) and into globals this
// module does not own (the player data_array, the difficulty/team bitset at DAT_006b0b84+0xa4,
// the friendly-fire globals at DAT_006f1cbc/DAT_006f1cf4, DAT_0087abc5/DAT_0087abc7). Every
// FUN_00xxxxxx callee below is outside this module's address range and is treated as an opaque
// external with only its visibly-passed arguments preserved. Local variables are kept close to
// their Ghidra names (rather than invented semantic names) specifically because their true
// meaning is not established with confidence; this favours literal preservation of control flow
// and arithmetic over readability, per the task's priority when the two are in tension.
// register convention: damage_data *param_1 on the stack; uint32_t param_2 (initial target
// object index); int16_t param_3 (collision node index); int16_t param_4; int16_t param_5
// (material index); uint32_t param_6, all on the stack.
// blam-cc: stack=(dd, target_object_index, node_index, param_4, material_index, param_6)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"

extern data_array *object_data;     // 0x008603b0
extern data_array *player_data;     // 0x0087a480, players module
extern tag_instance *tag_instances; // 0x0087bc14
extern random_seed random_seed_global; // 0x00719cd0, the engine-wide LCG state (same name and
    // type as in src/math/random_real.c and src/hs/hs_evaluate_random.c)
extern uint8_t network_predicted_state_flag;          // 0x006f1d20, predicted/network flag
extern uint32_t *g_006b0b84;        // 0x006b0b84, UNSURE: difficulty/team bitset base
extern uint8_t g_0087abc5;          // 0x0087abc5, UNSURE
extern uint8_t g_0087abc7;          // 0x0087abc7, UNSURE
extern ModelCollisionGeometryMaterial default_collision_material; // 0x006b8c68, the static
    // fallback material record used when a collision model has no usable indirect_damage_material
extern int16_t network_game_mode;          // 0x00719720, UNSURE: notify-mode selector
extern uint8_t g_006f1cbc;          // 0x006f1cbc, UNSURE: friendly-fire related
extern uint8_t g_006f1cf4;          // 0x006f1cf4, UNSURE: friendly-fire mode

extern void actor_apply_perception_scale(damage_data *dd); // UNSURE: out of range, 0x42aa90
extern void player_effect_send_network_update(damage_data *dd, float random_blend, float damage_amount); // UNSURE: out of range, 0x456bc0;
    // the mode-2/non-bVar20 call site in the original passes only two arguments
extern void player_effect_mark_damage_direction(damage_data *dd, real_vector3d *direction, float random_blend, float damage_amount); // UNSURE: out of range, 0x456cf0
    // (param_3 is damage_data+0x40, used as a float everywhere in this function -- declaring it
    // uint32_t here silently converted the value instead of passing the same four bytes)
extern int8_t teams_are_enemies(void); // UNSURE: zero visible args; out of range, 0x45bd50
extern int32_t game_engine_compute_time_scale(void); // UNSURE: zero visible args; out of range, 0x461550
extern real weapon_get_zoom_fov(int32_t param_1); // UNSURE: out of range, 0x46fe10
extern int32_t players_iterate_and_discard(datum_index object_index); // UNSURE: out of range, 0x474db0
extern void FUN_004eda20(void); // UNSURE: zero visible args; this module, address matches
                                // object_set_health_frozen_flag's original name, but called bare
                                // here so kept as-is rather than assuming the (object_index) form
extern int32_t object_get_controlling_player_index(datum_index object_index); // this module, 0x4ee2e0
extern void object_notify_pickup_or_refresh_probe(uint32_t object_index, datum_index player_index); // this module, 0x4ee3c0.
    // The original shows three values pushed here (local_54/local_50/local_4c), but 0x4ee3c0
    // overwrites its one stack slot before reading it and takes its real inputs in ECX (the
    // object handle) and EDI (a player handle). Declared to match the definition.
extern void object_apply_body_damage(uint32_t target_index, int32_t region_index, int32_t node_index,
    uint32_t param_4, ModelCollisionGeometry *geometry, ModelCollisionGeometryMaterial *material,
    DamageEffect *effect, damage_data *dd, uint32_t *notify_flags, float *body_damage_out,
    uint32_t *param11_out, float remaining_damage, int8_t role_is_deletable); // 0x4ef2a0
extern void object_apply_shield_damage(uint32_t target_index, ModelCollisionGeometry *geometry,
    ModelCollisionGeometryMaterial *material, DamageEffect *effect, uint32_t *notify_flags,
    float *shield_damage_out, float *remaining_damage, int8_t role_is_deletable,
    int8_t attributable_to_live_player, object_shield_impulse_result *impulse_result); // 0x4ef820; UNSURE: the 10th
    // parameter comes from an `unaff_EBX` this decompile never sources — see
    // object_apply_shield_damage.c's header. Passed as 0 here since the true value is not
    // recoverable from this function's own decompile either.
extern void object_damage_notify_and_impulse(uint32_t target_index, damage_data *dd, uint32_t notify_flags,
    float shield_damage, float body_damage, uint32_t param_6, int32_t node_hint, uint32_t role_is_deletable); // 0x4efcf0
    // 0x4efcf0 models arguments 4/5 as undefined4 pass-throughs to 0x5674a0; this call site is
    // the only evidence of their type and it supplies the two damage floats.
extern void object_delete_unparented(uint32_t object_index); // blam-cc: EDI -> object_index // UNSURE: zero visible args; objects module, 0x4f5aa0 (out of range)
extern void object_delete_recursive(uint32_t object_index, uint8_t recurse_siblings); // objects module, 0x4f59d0 (out of range)
extern object *object_try_and_get(datum_index object_index, uint32_t type_mask);
    // 0x4f6ec0; object handle in ECX, type mask on the stack. Verified against the body at
    // 0x4f6ec0 (cmp ecx,-1 / test cx,cx / and param_1 & 1 << header->type) and against the
    // call site in this file.
extern int8_t unit_point_in_front_and_asleep(void); // UNSURE: zero visible args; out of range, 0x56bc80

void object_apply_damage(damage_data *dd, uint32_t param_2, int16_t param_3, int16_t param_4,
    int16_t param_5, uint32_t param_6)
{
    object_header *headers = (object_header *)object_data->data;
    object *puVar1;
    int32_t role_is_deletable; // local_64, low byte meaningful
    datum_index resp_player;
    DamageEffect *effect;
    int16_t *psVar19; // &effect->damage_side_effect, kept as a short* exactly as decompiled
    uint32_t rand16;
    int8_t used_difficulty_random = 0;   // local_81
    uint8_t has_no_parent_never_takes_body_damage = 1; // local_8d
    float damage_amount; // local_8c
    int16_t *local_6c;
    datum_index chain[17]; // local_44
    int16_t chain_count = 0; // local_80
    object *target0;
    uint32_t target0_collision;
    int8_t bVar20; // "no random damage range" flag
    uint32_t local_78 = 0; // notify-flags accumulator (local_58 in the original is the
                           // body-damage out-slot, modelled below as param11_out)
    uint32_t local_88_bits = 0; // reused scratch for the vehicle-spread multiplier bit pattern
    int32_t local_5c = -1; // node "name_thing" scratch / passthrough to notify
    int8_t bVar3 = 0; // "already picked leftover impulse source" latch

    puVar1 = headers[param_2 & 0xffff].data;
    role_is_deletable = (puVar1->network_role == 0 || puVar1->network_role == 3) ? 1 : 0;

    resp_player = dd->responsible_player;
    if (resp_player != (datum_index)0xffffffff) {
        int16_t index = (int16_t)resp_player;
        int8_t invalid = 0;

        if (index < 0 || index >= player_data->maximum_count) {
            invalid = 1;
        } else {
            int16_t *record = (int16_t *)((uint8_t *)player_data->data + player_data->size * index);
            if (*record == 0) {
                invalid = 1;
            } else {
                int16_t salt = (int16_t)(resp_player >> 0x10);
                if (salt != 0 && *record != salt) {
                    invalid = 1;
                }
            }
        }
        if (invalid) {
            dd->responsible_player = (datum_index)0xffffffff;
        }
    }

    effect = (DamageEffect *)tag_instances[dd->damage_effect_tag & 0xffff].data;
    psVar19 = (int16_t *)((uint8_t *)effect + 0x1c4);
    local_6c = psVar19;

    random_seed_global = random_seed_global * 0x19660d + 0x3c6ef35f;
    rand16 = random_seed_global >> 0x10;

    damage_amount = ((1.0f - dd->random_blend) * effect->damage_lower_bound +
        ((float)rand16 * 1.5259022e-05f * (effect->damage_upper_bound[1] - effect->damage_upper_bound[0]) +
            effect->damage_upper_bound[0]) * dd->random_blend) * dd->multiplier;

    if (dd->responsible_object != (datum_index)0xffffffff) {
        object *unit = object_try_and_get(dd->responsible_object, _object_mask_unit);
            // 0x4ee6b6 mov ecx,[ebp+0xc] -- the handle just tested above
        if (unit != 0) {
            uint8_t *unit_bytes = (uint8_t *)unit;
            if (*(uint32_t *)(unit_bytes + 0x328) != 0xffffffff) { // UNSURE: unit extension field
                uint32_t other = *(uint32_t *)(unit_bytes + 0x328);
                unit = headers[other & 0xffff].data;
                unit_bytes = (uint8_t *)unit;
            }
            {
                int32_t v = *(int32_t *)(unit_bytes + 0x1f8); // UNSURE: unit extension field
                if (v == -1) {
                    v = *(int32_t *)(unit_bytes + 500); // UNSURE
                }
                if (v != -1) {
                    actor_apply_perception_scale(dd);
                }
            }
        }
    }

    if (network_predicted_state_flag == 0) {
        int16_t team = (int16_t)dd->team_index;
        int8_t skip = 0;

        if (team == -1) {
            skip = 1;
        } else if (-1 < team && team < 10) {
            int32_t bit_index = team * 10 + 1;
            if ((1 << (bit_index & 0x1f) & g_006b0b84[7 + (bit_index >> 5)]) != 0) { // UNSURE: +0xa4/4=0x29=7+... base
                skip = 1;
            }
        }
        if (!skip) {
            real scalar = weapon_get_zoom_fov(0);
            used_difficulty_random = 1;
            damage_amount = scalar * damage_amount;
        }
    } else {
        object_get_controlling_player_index((datum_index)0); // UNSURE: argument not visible, x2 in original
        object_get_controlling_player_index((datum_index)0);
        {
            int32_t scalar = game_engine_compute_time_scale();
            damage_amount = (float)scalar * damage_amount;
        }
    }

    {
        int8_t suppress_chain = (int8_t)(dd->flags & 1);

        if (suppress_chain == 0 && (dd->flags & 4) == 0) {
            if (param_2 != 0xffffffff) {
                uint32_t walker = param_2;
                do {
                    chain[chain_count] = walker;
                    walker = headers[walker & 0xffff].data->parent_object;
                    chain_count = chain_count + 1;
                } while (walker != (datum_index)0xffffffff);
            }
        } else {
            chain[0] = param_2;
            chain_count = 1;
        }
    }

    target0 = puVar1;
    target0_collision = ((Object *)tag_instances[target0->definition_tag & 0xffff].data)->collision_model.tag_id.index;
    if (target0_collision != 0xffff) {
        ModelCollisionGeometry *target_geometry = (ModelCollisionGeometry *)tag_instances[target0_collision].data;
        has_no_parent_never_takes_body_damage = (uint8_t)(~(target_geometry->flags >> 4) & 1);
    }

    if (target0->damage_owner != (datum_index)0xffffffff) {
        chain[chain_count] = target0->damage_owner;
        chain_count = chain_count + 1;
    }

    if ((int8_t)(dd->flags & 1) == 0 && target0->type == _object_type_vehicle) {
        float spread = (1.0f - effect->damage_vehicle_passthrough_penalty) *
            *(float *)((uint8_t *)tag_instances[target0->definition_tag & 0xffff].data + 0x184); // UNSURE: Unit.rider_damage_fraction
        local_88_bits = *(uint32_t *)&spread;
        dd->multiplier = spread;

        if (network_predicted_state_flag != 0) {
            uint32_t walker = target0->first_child_object;
            int32_t seated = 0;

            if (walker != (datum_index)0xffffffff) {
                do {
                    object *child = headers[walker & 0xffff].data;
                    if (child->type == _object_type_biped && *((int32_t *)((uint8_t *)child + 0x218)) != -1) { // UNSURE
                        seated = seated + 1;
                    }
                    walker = child->next_object;
                } while (walker != (datum_index)0xffffffff);
                if (seated != 0) {
                    dd->multiplier = spread / (float)seated;
                }
            }
        }

        {
            uint32_t walker = target0->first_child_object;
            while (walker != (datum_index)0xffffffff) {
                object *child = headers[walker & 0xffff].data;
                if (child->type == _object_type_biped) {
                    if (*((int32_t *)((uint8_t *)child + 0x218)) == -1) { // UNSURE
                        if (walker == *(uint32_t *)((uint8_t *)target0 + 0x324)) { // UNSURE: +0xc9*4
                            dd->flags = dd->flags | 0x20;
                        } else {
                            goto skip_child;
                        }
                    } else {
                        dd->flags = dd->flags & 0xffffffdf;
                    }
                    object_apply_damage(dd, walker, -1, -1, -1, 0);
                    dd->flags = dd->flags & 0xffffffdf;
                }
skip_child:
                walker = child->next_object;
            }
        }

        dd->multiplier = 1.0f;
    }

    bVar20 = (effect->damage_upper_bound[0] == 0.0f && effect->damage_upper_bound[1] == 0.0f) ? 1 : 0;

    // Notify-pickup pass over the chain array, then the per-target damage loop.
    // STRUCTURE NOTE (phase-4 review): in the original the `joined_r0x004eeb40` damage loop
    // sits OUTSIDE this `if (0 < chain_count)` guard, not inside it. Nesting it here is
    // equivalent because that loop's own first test is `if (damage <= 0 || chain_count-- < 1)
    // return`, so a zero chain_count returns on the first iteration without any side effect
    // (the original's `DAT_0087a480 = iVar14` store on that path writes the value it just
    // read back to the same global). Kept nested so the two passes read in source order.
    if (0 < chain_count) {
        int32_t remaining = chain_count;
        int32_t i;

        for (i = 0; i < remaining; i++) {
            uint32_t handle = chain[i];
            object_header *resolved = 0;

            if (handle != 0xffffffff) {
                int16_t index = (int16_t)handle;
                if (-1 < index && index < object_data->maximum_count) {
                    object_header *candidate = &headers[(uint16_t)index];
                    if (candidate->identifier != 0) {
                        int16_t salt = (int16_t)(handle >> 0x10);
                        if (salt == 0 || candidate->identifier == salt) {
                            resolved = candidate;
                        }
                    }
                }
            }

            if (resolved != 0 && (1 << (resolved->type & 0x1f) & _object_mask_unit) != 0 && resolved->data != 0) {
                object *unit = resolved->data;
                if (*((int32_t *)((uint8_t *)unit + 0x218)) == -1) { // UNSURE: driver seat gate
                    if (g_0087abc5 != 0) {
                        if (network_game_mode == 0) {
                            player_effect_mark_damage_direction(dd, &dd->direction, dd->random_blend, damage_amount);
                        } else if (network_game_mode == 2) {
                            player_effect_send_network_update(dd, dd->random_blend, damage_amount);
                        }
                    }
                } else if (network_game_mode == 0) {
                    player_effect_mark_damage_direction(dd, &dd->direction, dd->random_blend, damage_amount);
                } else if (network_game_mode == 1) {
                    if (bVar20) {
                        player_effect_mark_damage_direction(dd, &dd->direction, dd->random_blend, damage_amount);
                    }
                } else if (network_game_mode == 2) {
                    if (bVar20) {
                        player_effect_mark_damage_direction(dd, &dd->direction, dd->random_blend, damage_amount);
                    } else {
                        player_effect_send_network_update(dd, dd->random_blend, damage_amount);
                    }
                }
            }
        }
    }

    // Main per-object damage-application pass, walking the chain array from the end.
    {
        uint32_t final_target = 0;
        uint32_t final_index_bytes = 0;

        for (;;) {
            uint32_t target_handle;
            object *target;
            object_header *target_header;
            uint32_t collision_tag_index;
            float shield_damage_out;
            float body_damage_out;
            uint32_t param11_out;

            if (damage_amount <= 0.0f || chain_count < 1) {
                return;
            }
            chain_count = chain_count - 1;
            target_handle = chain[chain_count];
            final_index_bytes = (target_handle & 0xffff) * 0xc;
            target_header = &headers[target_handle & 0xffff];
            target = target_header->data;
            final_target = target_handle;

            local_78 = 0;
            local_5c = -1;
            shield_damage_out = 0.0f;  // local_60
            body_damage_out = 0.0f;    // local_88, reused as a float here
            param11_out = 0;           // local_58

            collision_tag_index = ((Object *)tag_instances[target->definition_tag & 0xffff].data)->collision_model.tag_id.index;

            if (collision_tag_index != 0xffff) {
                ModelCollisionGeometry *geometry = (ModelCollisionGeometry *)tag_instances[collision_tag_index].data;
                uint8_t responsible_object_flag = (uint8_t)((dd->flags >> 2) & 1);
                int8_t friendly_fire_blocked = 0;
                int8_t apply_shield_gate = 1;
                int8_t apply_notify_gate = 1;
                int16_t material_type_cache = 0;
                int32_t attributable_to_live_player;
                ModelCollisionGeometryMaterial *material;
                uint8_t notify_permitted = responsible_object_flag;

                if (target->network_role == 3 || target->network_role == 0) {
                    attributable_to_live_player = 1;
                } else {
                    attributable_to_live_player = 1;
                    if (dd->responsible_player != (datum_index)0xffffffff) {
                        int16_t index = (int16_t)dd->responsible_player;
                        if (-1 < index && index < player_data->maximum_count) {
                            int16_t *record = (int16_t *)((uint8_t *)player_data->data + player_data->size * index);
                            if (*record != 0) {
                                int16_t salt = (int16_t)(dd->responsible_player >> 0x10);
                                if ((salt == 0 || *record == salt) && record[1] != -1) {
                                    attributable_to_live_player = 0;
                                }
                            }
                        }
                    }
                }

                if (network_predicted_state_flag != 0 && g_006f1cbc != 0) {
                    int32_t controller = players_iterate_and_discard(target_handle);
                    if (controller != -1 && (uint32_t)controller != dd->responsible_player) {
                        int16_t index = (int16_t)controller;
                        if (-1 < index && index < player_data->maximum_count) {
                            int16_t *record = (int16_t *)((uint8_t *)player_data->data + player_data->size * index);
                            if (*record != 0) {
                                int16_t salt = (int16_t)((uint32_t)controller >> 0x10);
                                if (salt == 0 || *record == salt) {
                                    int8_t is_ai = teams_are_enemies();

                                    friendly_fire_blocked = (int8_t)(1 - (is_ai != 0));
                                    if (friendly_fire_blocked != 0) {
                                        // 0x4eed0b clears bVar2 (apply_shield_gate) and then falls
                                        // through to the shared bVar20 = false; both modes 0 and
                                        // 3-without-the-0x20-flag go through it.
                                        if (g_006f1cf4 == 0) {
                                            apply_shield_gate = 0;
                                            apply_notify_gate = 0;
                                        } else if (g_006f1cf4 == 2) {
                                            apply_shield_gate = 1;
                                            apply_notify_gate = 0;
                                        } else if (g_006f1cf4 == 3) {
                                            if ((effect->damage_flags & 0x20) != 0) {
                                                goto friendly_fire_resolved;
                                            }
                                            apply_shield_gate = 0;
                                            apply_notify_gate = 0;
                                        } else {
                                            goto friendly_fire_resolved;
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
friendly_fire_resolved:
                if (-1 < param_3 && param_3 < geometry->nodes.count) {
                    // CONCAT22 in the original: only the low half of local_5c is written, so the
                    // 0xffff it was seeded with stays in the high half.
                    local_5c = (int32_t)((local_5c & 0xffff0000) |
                        (uint32_t)*(uint16_t *)((uint8_t *)geometry->nodes.pointer + param_3 * 0x40 + 0x32));
                }

                if (used_difficulty_random != 0) {
                    local_78 = 0x20;
                }

                // UNSURE: `local_70[0x2e]` (object word-index 0x2e, byte offset 0xb8) is a raw
                // field with no name in types/objects.h; team comparison kept literal.
                if ((int16_t)dd->team_index != -1) {
                    int16_t sVar12 = *(int16_t *)((uint8_t *)target + 0xb8);
                    int16_t sVar8 = (int16_t)dd->team_index;
                    int8_t different_team;
                    int8_t known = 1;

                    if (network_predicted_state_flag == 0) {
                        if (sVar12 < 0 || 9 < sVar12 || sVar8 < 0 || 9 < sVar8) {
                            known = 0;
                            different_team = 1;
                        } else {
                            int32_t bit_index = (int32_t)sVar8 + sVar12 * 10;
                            different_team = (int8_t)(1 - ((1 << (bit_index & 0x1f) & g_006b0b84[7 + (bit_index >> 5)]) != 0));
                        }
                    } else {
                        different_team = (int8_t)(sVar12 != sVar8);
                    }
                    if (known && different_team == 0) {
                        local_78 = local_78 | 0x10;
                    }
                }
                if (chain_count == 0 && -1 < param_5 && param_5 < geometry->materials.count) {
                    material = &((ModelCollisionGeometryMaterial *)geometry->materials.pointer)[param_5];
                } else {
                    int16_t default_material = geometry->indirect_damage_material;
                    if (default_material < 0 || geometry->materials.count <= default_material) {
                        material = &default_collision_material;
                    } else {
                        material = &((ModelCollisionGeometryMaterial *)geometry->materials.pointer)[default_material];
                    }
                }
                material_type_cache = material->material_type;
                dd->unknown_4c = material_type_cache;

                if (g_0087abc7 != 0 && dd->responsible_player != (datum_index)0xffffffff) {
                    notify_permitted = 1;
                }
                if (effect->damage_side_effect == 2 && unit_point_in_front_and_asleep() != 0 &&
                    (*((uint8_t *)target + 0x107) & 8) == 0) {
                    notify_permitted = 1;
                }

                if ((int8_t)role_is_deletable == 1 && notify_permitted != 0 &&
                    (target->vitality_flags & _object_health_frozen_bit) == 0 &&
                    (friendly_fire_blocked == 0 || apply_notify_gate)) {
                    *(uint32_t *)((uint8_t *)target + 0xe0) = 0; // local_70[0x38]: a 4-byte write
                    FUN_004eda20();
                    local_78 = local_78 | 0x41;
                }

                if ((dd->flags & 0x20) == 0 && (effect->damage_flags & 0x200) == 0 &&
                    target->maximum_shield_vitality > 0.0f &&
                    (friendly_fire_blocked == 0 || apply_shield_gate) &&
                    (chain_count == 0 || (geometry->flags & 1) != 0)) {
                    object_apply_shield_damage(target_handle, geometry, material, effect, &local_78,
                        &shield_damage_out, &damage_amount, (int8_t)role_is_deletable,
                        (int8_t)attributable_to_live_player, 0);
                }

                if ((chain_count == 0 || (has_no_parent_never_takes_body_damage != 0 && (geometry->flags & 2) != 0)) &&
                    (effect->damage_flags & 0x40) == 0) {
                    int32_t body_node;
                    int32_t body_param4;

                    if (((geometry->flags & 0x20) != 0 && (effect->damage_flags & 0x20) == 0) ||
                        (friendly_fire_blocked != 0 && !apply_notify_gate)) {
                        damage_amount = 0.0f;
                    }
                    if (chain_count == 0) {
                        body_node = param_3;
                        body_param4 = param_4;
                    } else {
                        body_node = -1;
                        body_param4 = -1;
                    }
                    object_apply_body_damage(target_handle, body_param4, body_node,
                        ((chain_count != 0) - 1) & param_6, geometry, material, effect, dd, &local_78,
                        &body_damage_out, &param11_out, damage_amount, (int8_t)role_is_deletable);
                    chain_count = 0;
                }

                if (bVar3 == 0 && (0.0001f < shield_damage_out || 0.0001f < body_damage_out)) {
                    if (shield_damage_out <= body_damage_out) {
                        float v = *(float *)((uint8_t *)target + 0xe0); // UNSURE: local_70[0x38]
                        if (0.0f <= v) {
                            dd->unknown_48 = (v <= 1.0f) ? *(uint32_t *)&v : 0x3f800000;
                        } else {
                            dd->unknown_48 = 0;
                        }
                    } else {
                        dd->unknown_4c = *(int16_t *)((uint8_t *)geometry + 0xd2); // UNSURE
                        dd->unknown_48 = *(uint32_t *)((uint8_t *)target + 0xe4); // UNSURE: local_70[0x39]
                    }
                    bVar3 = 1;
                }

                // local_54 is the object HANDLE (local_68), not the byte offset local_48.
                object_notify_pickup_or_refresh_probe(final_target, dd->responsible_player);
                    // UNSURE: the EDI player handle is not visible at this call site;
                    // dd->responsible_player is the only player handle live here.

                if (0.0f < shield_damage_out && target->type == _object_type_biped) {
                    *((uint8_t *)target + 0x122) = 1;
                }
            }

            object_damage_notify_and_impulse(final_target, dd, local_78, shield_damage_out,
                body_damage_out, param11_out, local_5c, role_is_deletable);

            if ((local_78 & 4) != 0) {
                int32_t role = headers[final_index_bytes / 0xc].data->network_role;

                // 0x4ef133: role 0 calls object_delete_unparented and then FALLS THROUGH into
                // object_delete_recursive; only roles other than 0 and 3 skip the recursive delete.
                if (role == 0) {
                    object_delete_unparented(final_target); // UNSURE: EDI at 0x4ef12b
                }
                if (role == 0 || role == 3) {
                    object_delete_recursive(final_target, 0);
                }
            }
        }
    }
}

#if 0
Original Ghidra decompilation (0x4ee5e0):

void object_apply_damage(uint *param_1,uint param_2,short param_3,short param_4,short param_5,
                        uint param_6)

{
  uint *puVar1;
  bool bVar2;
  bool bVar3;
  float fVar4;
  byte *pbVar5;
  uint uVar6;
  char cVar7;
  short sVar8;
  int iVar9;
  uint uVar10;
  short *psVar11;
  short sVar12;
  uint uVar13;
  int iVar14;
  byte bVar15;
  ushort uVar16;
  int iVar17;
  undefined *puVar18;
  short *psVar19;
  bool bVar20;
  float10 fVar21;
  uint auStackY_20044 [32731];
  char local_91;
  byte local_8d;
  float local_8c;
  uint *local_88;
  char local_81;
  int local_80;
  uint local_7c;
  uint local_78;
  byte *local_74;
  uint *local_70;
  short *local_6c;
  uint local_68;
  uint local_64;
  float local_60;
  int local_5c;
  uint local_58;
  uint local_54;
  undefined4 local_50;
  uint local_4c;
  int local_48;
  uint local_44 [17];
  
  iVar17 = DAT_008603b0;
  local_5c = (param_2 & 0xffff) * 0xc;
  iVar14 = *(int *)(*(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + local_5c) + 4);
  if ((iVar14 == 0) || (local_64 = local_64 & 0xffffff00, iVar14 == 3)) {
    local_64 = CONCAT31(local_64._1_3_,1);
  }
  uVar13 = param_1[2];
  if ((uVar13 != 0xffffffff) &&
     ((((sVar8 = (short)uVar13, sVar8 < 0 || (*(short *)(DAT_0087a480 + 0x20) <= sVar8)) ||
       (sVar8 = *(short *)((int)*(short *)(DAT_0087a480 + 0x22) * (int)sVar8 +
                          *(int *)(DAT_0087a480 + 0x34)), sVar8 == 0)) ||
      ((sVar12 = (short)(uVar13 >> 0x10), sVar12 != 0 && (sVar8 != sVar12)))))) {
    param_1[2] = 0xffffffff;
  }
  iVar14 = *(int *)((*param_1 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
  psVar19 = (short *)(iVar14 + 0x1c4);
  DAT_00719cd0 = DAT_00719cd0 * 0x19660d + 0x3c6ef35f;
  local_58 = DAT_00719cd0 >> 0x10;
  local_81 = '\0';
  local_8d = 1;
  local_8c = ((1.0 - (float)param_1[0x10]) * *(float *)(iVar14 + 0x1d0) +
             ((float)local_58 * 1.5259022e-05 *
              (*(float *)(iVar14 + 0x1d8) - *(float *)(iVar14 + 0x1d4)) + *(float *)(iVar14 + 0x1d4)
             ) * (float)param_1[0x10]) * (float)param_1[0x11];
  local_6c = psVar19;
  if ((param_1[3] != 0xffffffff) && (iVar9 = object_try_and_get(3), iVar9 != 0)) {
    if (*(uint *)(iVar9 + 0x328) != 0xffffffff) {
      iVar9 = *(int *)(*(int *)(iVar17 + 0x34) + 8 + (*(uint *)(iVar9 + 0x328) & 0xffff) * 0xc);
    }
    iVar17 = *(int *)(iVar9 + 0x1f8);
    if (iVar17 == -1) {
      iVar17 = *(int *)(iVar9 + 500);
    }
    if (iVar17 != -1) {
      FUN_0042aa90(param_1);
    }
  }
  if (DAT_006f1d20 == 0) {
    sVar8 = (short)param_1[4];
    if ((sVar8 == -1) ||
       (((-1 < sVar8 && (sVar8 < 10)) &&
        (iVar17 = sVar8 * 10 + 1,
        (1 << ((byte)iVar17 & 0x1f) & *(uint *)(DAT_006b0b84 + 0xa4 + (iVar17 >> 5) * 4)) != 0))))
    goto LAB_004ee7d4;
    fVar21 = (float10)FUN_0046fe10(0);
    local_81 = '\x01';
  }
  else {
    object_get_controlling_player_index();
    object_get_controlling_player_index();
    fVar21 = (float10)FUN_00461550();
  }
  local_8c = (float)(fVar21 * (float10)local_8c);
LAB_004ee7d4:
  iVar17 = 0;
  uVar13 = param_1[1] & 1;
  local_80 = 0;
  bVar3 = false;
  if ((uVar13 == 0) && ((param_1[1] & 4) == 0)) {
    if (param_2 != 0xffffffff) {
      iVar9 = *(int *)(DAT_008603b0 + 0x34);
      do {
        local_44[(short)iVar17] = param_2;
        param_2 = *(uint *)(*(int *)(iVar9 + 8 + (param_2 & 0xffff) * 0xc) + 0x11c);
        iVar17 = iVar17 + 1;
        local_80 = iVar17;
      } while (param_2 != 0xffffffff);
    }
  }
  else {
    local_44[0] = param_2;
    local_80 = 1;
  }
  puVar1 = *(uint **)(*(int *)(DAT_008603b0 + 0x34) + 8 + local_5c);
  uVar10 = *(uint *)(*(int *)((*puVar1 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14) + 0x7c);
  if (uVar10 != 0xffffffff) {
    local_8d = ~(byte)(**(uint **)((uVar10 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14) >> 4) & 1;
  }
  if (puVar1[0x3c] != 0xffffffff) {
    sVar8 = (short)local_80;
    local_80 = local_80 + 1;
    local_44[sVar8] = puVar1[0x3c];
  }
  uVar16 = (ushort)local_80;
  if ((uVar13 == 0) && ((short)puVar1[0x2d] == 1)) {
    local_88 = (uint *)((1.0 - *(float *)(iVar14 + 0x1dc)) *
                       *(float *)(*(int *)((*puVar1 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14) + 0x184)
                       );
    bVar20 = DAT_006f1d20 != 0;
    param_1[0x11] = (uint)local_88;
    if (bVar20) {
      uVar13 = puVar1[0x46];
      iVar14 = 0;
      local_7c = 0;
      if (uVar13 != 0xffffffff) {
        do {
          iVar17 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (uVar13 & 0xffff) * 0xc);
          if ((*(short *)(iVar17 + 0xb4) == 0) && (*(int *)(iVar17 + 0x218) != -1)) {
            iVar14 = iVar14 + 1;
          }
          uVar13 = *(uint *)(iVar17 + 0x114);
        } while (uVar13 != 0xffffffff);
        local_7c = iVar14;
        if (iVar14 != 0) {
          param_1[0x11] = (uint)((float)local_88 / (float)iVar14);
        }
      }
    }
    uVar13 = puVar1[0x46];
    psVar19 = local_6c;
    while (local_6c = psVar19, uVar13 != 0xffffffff) {
      iVar14 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (uVar13 & 0xffff) * 0xc);
      if (*(short *)(iVar14 + 0xb4) == 0) {
        if (*(int *)(iVar14 + 0x218) == -1) {
          if (uVar13 != puVar1[0xc9]) goto LAB_004ee9ab;
          uVar10 = param_1[1] | 0x20;
        }
        else {
          uVar10 = param_1[1] & 0xffffffdf;
        }
        param_1[1] = uVar10;
        object_apply_damage(param_1,uVar13,0xffffffff,0xffffffff,0xffffffff,0);
        param_1[1] = param_1[1] & 0xffffffdf;
      }
LAB_004ee9ab:
      psVar19 = local_6c;
      uVar13 = *(uint *)(iVar14 + 0x114);
    }
    uVar16 = (ushort)local_80;
    param_1[0x11] = 0x3f800000;
  }
  fVar4 = local_8c;
  if ((*(float *)(psVar19 + 8) != 0.0) || (bVar20 = true, *(float *)(psVar19 + 10) != 0.0)) {
    bVar20 = false;
  }
  iVar14 = DAT_0087a480;
  uVar13 = local_7c;
  if (0 < (short)uVar16) {
    local_7c = (uint)uVar16;
    local_88 = local_44;
    do {
      uVar13 = *local_88;
      psVar19 = (short *)0x0;
      if (((uVar13 != 0xffffffff) && (sVar8 = (short)uVar13, -1 < sVar8)) &&
         (sVar8 < *(short *)(DAT_008603b0 + 0x20))) {
        psVar11 = (short *)((int)*(short *)(DAT_008603b0 + 0x22) * (int)sVar8 +
                           *(int *)(DAT_008603b0 + 0x34));
        sVar8 = *psVar11;
        if ((sVar8 != 0) && ((sVar12 = (short)(uVar13 >> 0x10), sVar12 == 0 || (sVar8 == sVar12))))
        {
          psVar19 = psVar11;
        }
      }
      if (((psVar19 != (short *)0x0) && ((1 << (*(byte *)((int)psVar19 + 3) & 0x1f) & 3U) != 0)) &&
         (*(int *)(psVar19 + 4) != 0)) {
        if (*(int *)(*(int *)(psVar19 + 4) + 0x218) == -1) {
          if (DAT_0087abc5 != '\0') {
            if (DAT_00719720 == 0) {
              uVar13 = param_1[0x10];
              goto LAB_004eeb0e;
            }
            if (DAT_00719720 == 2) {
              FUN_00456bc0(param_1,param_1[0x10],fVar4);
            }
          }
        }
        else if (DAT_00719720 == 0) {
LAB_004eeabe:
          uVar13 = param_1[0x10];
LAB_004eeb0e:
          FUN_00456cf0(param_1,param_1 + 0xd,uVar13,fVar4);
        }
        else if (DAT_00719720 == 1) {
          if (bVar20) goto LAB_004eeabe;
        }
        else if (DAT_00719720 == 2) {
          if (bVar20) goto LAB_004eeabe;
          FUN_00456bc0(param_1,param_1[0x10]);
        }
      }
      local_88 = local_88 + 1;
      local_7c = local_7c - 1;
      iVar14 = DAT_0087a480;
      uVar13 = 0;
    } while (local_7c != 0);
  }
joined_r0x004eeb40:
  do {
    local_7c = uVar13;
    if ((local_8c <= 0.0) || (sVar8 = (short)local_80, local_80 = local_80 + -1, sVar8 < 1)) {
      DAT_0087a480 = iVar14;
      return;
    }
    local_68 = local_44[(short)local_80];
    local_48 = (local_68 & 0xffff) * 0xc;
    local_70 = *(uint **)(*(int *)(DAT_008603b0 + 0x34) + 8 + local_48);
    uVar13 = *(uint *)(*(int *)((*local_70 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14) + 0x7c);
    local_60 = 0.0;
    local_88 = (uint *)0x0;
    local_58 = 0;
    local_78 = 0;
    local_5c = 0xffffffff;
    DAT_0087a480 = iVar14;
    if (uVar13 != 0xffffffff) {
      local_74 = *(byte **)((uVar13 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
      bVar15 = (byte)(param_1[1] >> 2) & 1;
      local_91 = '\0';
      bVar2 = true;
      bVar20 = true;
      local_50 = 0;
      local_4c = local_4c & 0xffffff00;
      local_7c._1_3_ = (uint3)(local_7c >> 8);
      if ((local_70[1] == 3) || (local_70[1] == 0)) {
        local_7c = CONCAT31(local_7c._1_3_,1);
      }
      else {
        uVar13 = param_1[2];
        local_7c = CONCAT31(local_7c._1_3_,1);
        if (((uVar13 != 0xffffffff) && (sVar8 = (short)uVar13, -1 < sVar8)) &&
           (sVar8 < *(short *)(iVar14 + 0x20))) {
          psVar19 = (short *)((int)*(short *)(iVar14 + 0x22) * (int)sVar8 + *(int *)(iVar14 + 0x34))
          ;
          sVar8 = *psVar19;
          if (((sVar8 != 0) &&
              ((sVar12 = (short)(uVar13 >> 0x10), sVar12 == 0 || (sVar8 == sVar12)))) &&
             (psVar19[1] != -1)) {
            local_7c = (uint)local_7c._1_3_ << 8;
          }
        }
      }
      local_54 = local_68;
      if ((((DAT_006f1d20 != 0) && (DAT_006f1cbc != '\0')) &&
          ((uVar13 = FUN_00474db0(local_68), uVar13 != 0xffffffff &&
           ((((uVar13 != param_1[2] && (sVar8 = (short)uVar13, -1 < sVar8)) &&
             (sVar8 < *(short *)(iVar14 + 0x20))) &&
            (sVar8 = *(short *)((int)*(short *)(iVar14 + 0x22) * (int)sVar8 +
                               *(int *)(iVar14 + 0x34)), sVar8 != 0)))))) &&
         ((sVar12 = (short)(uVar13 >> 0x10), sVar12 == 0 || (sVar8 == sVar12)))) {
        cVar7 = FUN_0045bd50();
        local_91 = '\x01' - (cVar7 != '\0');
        if (local_91 != '\0') {
          if (DAT_006f1cf4 == '\0') {
LAB_004eed0b:
            bVar2 = false;
          }
          else {
            if (DAT_006f1cf4 != '\x02') {
              if ((DAT_006f1cf4 != '\x03') || ((*(byte *)(local_6c + 2) & 0x20) != 0))
              goto LAB_004eed15;
              goto LAB_004eed0b;
            }
            bVar2 = true;
          }
          bVar20 = false;
        }
      }
LAB_004eed15:
      if ((-1 < param_3) && ((int)param_3 < *(int *)(local_74 + 0x28c))) {
        local_5c = CONCAT22(local_5c._2_2_,
                            *(undefined2 *)(param_3 * 0x40 + 0x32 + *(int *)(local_74 + 0x290)));
      }
      if (local_81 != '\0') {
        local_78 = 0x20;
      }
      sVar8 = (short)param_1[4];
      if (sVar8 != -1) {
        sVar12 = (short)local_70[0x2e];
        if (DAT_006f1d20 == 0) {
          if ((((sVar12 < 0) || (9 < sVar12)) || (sVar8 < 0)) || (9 < sVar8)) goto LAB_004eedd1;
          iVar14 = (int)sVar8 + sVar12 * 10;
          cVar7 = '\x01' - ((1 << ((byte)iVar14 & 0x1f) &
                            *(uint *)(DAT_006b0b84 + 0xa4 + (iVar14 >> 5) * 4)) != 0);
        }
        else {
          cVar7 = sVar12 != sVar8;
        }
        if (cVar7 == '\0') {
          local_78 = local_78 | 0x10;
        }
      }
LAB_004eedd1:
      if ((((short)local_80 == 0) && (-1 < param_5)) && ((int)param_5 < *(int *)(local_74 + 0x234)))
      {
        puVar18 = (undefined *)(*(int *)(local_74 + 0x238) + param_5 * 0x48);
      }
      else {
        sVar8 = *(short *)(local_74 + 4);
        if ((sVar8 < 0) || (*(int *)(local_74 + 0x234) <= (int)sVar8)) {
          puVar18 = &DAT_006b8c68;
        }
        else {
          puVar18 = (undefined *)(*(int *)(local_74 + 0x238) + sVar8 * 0x48);
        }
      }
      *(undefined2 *)(param_1 + 0x13) = *(undefined2 *)(puVar18 + 0x24);
      if ((DAT_0087abc7 != '\0') && (param_1[2] != 0xffffffff)) {
        bVar15 = 1;
      }
      if (((*local_6c == 2) && (cVar7 = FUN_0056bc80(), cVar7 != '\0')) &&
         ((*(byte *)((int)local_70 + 0x107) & 8) == 0)) {
        bVar15 = 1;
      }
      puVar1 = local_70;
      if ((((char)local_64 == '\x01') && (bVar15 != 0)) &&
         (((*(byte *)((int)local_70 + 0x106) & 4) == 0 && ((local_91 == '\0' || (bVar20)))))) {
        local_70[0x38] = 0;
        FUN_004eda20();
        local_78 = local_78 | 0x41;
      }
      pbVar5 = local_74;
      if ((((((param_1[1] & 0x20) == 0) && ((*(uint *)(local_6c + 2) & 0x200) == 0)) &&
           (0.0 < (float)puVar1[0x37])) && ((local_91 == '\0' || (bVar2)))) &&
         (((short)local_80 == 0 || ((*local_74 & 1) != 0)))) {
        object_apply_shield_damage
                  (local_68,local_74,puVar18,local_6c,&local_78,&local_60,&local_8c,local_64,
                   local_7c);
      }
      sVar8 = (short)local_80;
      if (((sVar8 == 0) || ((local_8d != 0 && ((*pbVar5 & 2) != 0)))) &&
         ((*(uint *)(local_6c + 2) & 0x40) == 0)) {
        if ((((*pbVar5 & 0x20) != 0) && ((*(uint *)(local_6c + 2) & 0x20) == 0)) ||
           ((local_91 != '\0' && (!bVar20)))) {
          local_8c = 0.0;
        }
        if (sVar8 == 0) {
          iVar14 = (int)param_3;
          iVar17 = (int)param_4;
        }
        else {
          iVar14 = -1;
          iVar17 = -1;
        }
        object_apply_body_damage
                  (local_68,iVar17,iVar14,(sVar8 != 0) - 1 & param_6,pbVar5,puVar18,local_6c,param_1
                   ,&local_78,&local_88,&local_58,local_8c,local_64);
        local_80 = 0;
      }
      if ((!bVar3) && ((0.0001 < local_60 || (0.0001 < (float)local_88)))) {
        if (local_60 <= (float)local_88) {
          if (0.0 <= (float)local_70[0x38]) {
            if ((float)local_70[0x38] <= 1.0) {
              uVar13 = local_70[0x38];
            }
            else {
              uVar13 = 0x3f800000;
            }
          }
          else {
            uVar13 = 0;
          }
          param_1[0x12] = uVar13;
        }
        else {
          *(undefined2 *)(param_1 + 0x13) = *(undefined2 *)(pbVar5 + 0xd2);
          param_1[0x12] = local_70[0x39];
        }
        bVar3 = true;
      }
      FUN_004ee3c0(local_54,local_50,local_4c);
      if ((0.0 < local_60) && ((short)local_70[0x2d] == 0)) {
        *(undefined1 *)((int)local_70 + 0x122) = 1;
      }
    }
    uVar6 = local_68;
    uVar10 = local_78;
    object_damage_notify_and_impulse
              (local_68,param_1,local_78,local_60,local_88,local_58,local_5c,local_64);
    iVar14 = DAT_0087a480;
    uVar13 = local_7c;
  } while ((uVar10 & 4) == 0);
  iVar17 = *(int *)(*(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + local_48) + 4);
  if (iVar17 == 0) {
    FUN_004f5aa0();
  }
  else if (iVar17 != 3) goto joined_r0x004eeb40;
  FUN_004f59d0(uVar6,0);
  iVar14 = DAT_0087a480;
  uVar13 = local_7c;
  goto joined_r0x004eeb40;
}

#endif
