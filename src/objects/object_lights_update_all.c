// object_lights_update_all
// address 0x4f0cf0, size 1591 bytes
// name confidence: 0.5 (out/phase4/objects_functions.md)
// rewrite confidence: 0.15 (the lowest-confidence file in this batch: a per-frame render/decal
// queue routine with many opaque rasterizer/decal/first-person-weapon externals and several
// register-passed globals this module does not own)
// evidence: types/objects.h light (flags 0x02, definition_tag 0x04, unknown_08 0x08,
// owner_object 0x2c, position 0x30, direction 0x3c, unknown_48, radius 0x54, marker_link 0x58,
// marker_index 0x5c, marker_index_secondary 0x5e, unknown_78); types/objects.h object (type 0xb4,
// parent_object 0x11c); types/tags.h Light (duration 0xf4 — the falloff denominator used here).
// UNSURE (function-wide): this is a straight, largely unannotated transcription. The render
// queue at DAT_007c1480/DAT_007c1484 (a fixed 0x80-entry, 0xe-dword-stride array),
// DAT_007c3108/DAT_007c310a, and every rasterizer_light_*/decal_*/first_person_weapon_* callee
// are outside this module and are not independently understood; their arguments are preserved
// exactly as decompiled, including several bare calls (rasterizer_light_set, structure_cluster_add_lens_flares,
// vector3d_pack_normal_11_11_10, lens_flare_add_instance) whose real parameters are not visible at all. The
// object-side child-luminance reduction block (object+0x37c) reaches into a unit-extension field
// with no name in types/objects.h. This function ends by tail-calling FUN_004f133c
// (object_lights_update_all_continued.c) for the "not always visible" case and returning,
// exactly as decompiled — the two files together are one logical loop.
// register convention: none (no parameters).

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "hs.h"
#include "cache.h"
#include "objects.h"

extern data_array *light_data;         // 0x00860b14
extern tag_instance *tag_instances;    // 0x0087bc14
extern data_array *object_data;        // 0x008603b0
extern hs_game_time_globals *game_time;            // 0x006f1d6c, game time globals; +0x0c is the current tick
extern int16_t light_transient_count_or_queue; // 0x00860b10, UNSURE: written as a short via a
    // "_DAT_00860b10" overlap warning from Ghidra; kept generic
extern int32_t light_frame_counter;    // 0x008607c4
extern int32_t light_render_unknown_7c0;             // 0x008607c0, UNSURE
extern int32_t light_active_list_count; // 0x008607c8, "lights_apply_spot_falloff input" count
extern datum_index *light_active_list;  // 0x008607cc, "lights_apply_spot_falloff input"
extern int32_t g_007c1480;             // 0x007c1480, UNSURE: render queue live count (max 0x80)
extern uint32_t g_007c1484[];           // 0x007c1484, UNSURE: render queue entries, stride 0xe dwords
extern int32_t g_007d0390;             // 0x007d0390, UNSURE
extern int16_t light_transient_count;              // 0x00860b0c, light_transient_count

extern void color_interpolate_argb_with_tint(void *out_color, float t); // UNSURE: effects/math module, 0x43f7d0
extern void color_interpolate(void *out_color, uint32_t color_pair, float t); // 0x43f6a0
extern void first_person_weapon_center_flashlight(void *param_1); // 0x492b80
extern int8_t unit_get_first_person_marker_transform(int32_t marker, void *out_transform); // UNSURE: out of range, 0x492c30
extern real transition_function_evaluate(transition_function_t type, real phase); // math
    // module, 0x4ccac0. The transition type travels in CX and is not visible at this call
    // site; the one value Ghidra shows pushed is the phase. UNSURE: type passed as 0.
extern void datum_delete(data_array *array, datum_index handle); // UNSURE: zero visible args at this call site; memory module, 0x4d0510
extern datum_index datum_next(int16_t after_index, data_array *array); // UNSURE: zero visible args; memory module, 0x4d0630
    // (a light_data iterator, per usage here)
extern void object_light_recompute_transform(uint32_t light_index); // this module, 0x4f2a00 (out of range)
extern object *object_try_and_get(datum_index object_index, uint32_t type_mask);
    // 0x4f6ec0; object handle in ECX, type mask on the stack. Verified against the body at
    // 0x4f6ec0 (cmp ecx,-1 / test cx,cx / and param_1 & 1 << header->type) and against the
    // call site in this file.
extern int32_t structure_cluster_add_lens_flares(void); // UNSURE: zero visible args; out of range, 0x513a00
extern void rasterizer_light_disable_all(void); // 0x526700
extern void rasterizer_light_set(void); // UNSURE: zero visible args; 0x526760
extern datum_index *light_cluster_first; // 0x00860b20
extern void cluster_reference_remove_all(uint32_t handle, datum_index *link, void *cluster_list); // out of range, 0x552020
extern int32_t structure_bsp_collect_visible_objects(datum_index *out_list, int32_t max_count, void *lab1, void *lab2,
    void *lab3, void *lab4, void *lab5); // UNSURE: out of range, 0x554420 (light-gathering pass;
    // the five label pointers are jump-table targets in the original binary, not reproducible)
extern int16_t local_player_index_for_weapon(uint32_t object_index); // UNSURE: out of range, 0x494010
extern int32_t DAT_007c3108; // UNSURE: not owned by this module
extern int16_t first_person_weapon_get_marker_data(uint32_t object_index, int32_t marker,
    void *out_buffer, int32_t max_count); // UNSURE: out of range, 0x492ad0
extern int32_t object_get_node_local_transform(uint32_t object_index, char *marker_name,
    object_marker *marker, uint32_t flags); // 0x4f6080, this module -- same declaration as
    // src/objects/object_get_node_local_transform.c's own definition (it was declared here
    // as FUN_004f6080 with a different return type and parameter names)
extern uint32_t vector3d_pack_normal_11_11_10(); // 0x5132d0; clamps the float at ESI into
    // [the constant at 0x00672ba8, 1.0]. The operand is register-passed and not modelled here.
extern void lens_flare_add_instance(void); // UNSURE: zero visible args; out of range, 0x5138a0
extern void object_lights_update_all_continued(light *current_light, Light *current_tag,
    object *owner_type_hint, int16_t loop_index, int32_t current_tick); // this module, 0x4f133c.
    // A mid-loop continuation reached by a tail call, so it inherits this function's live
    // registers rather than taking stack arguments. UNSURE: the mapping below is the one
    // 0x4f133c's own body implies, not something visible at this transfer.

void object_lights_update_all(void)
{
    uint8_t *lights_base = (uint8_t *)light_data->data;
    int32_t current_tick = game_time->current_tick;
    datum_index handle;

    light_transient_count_or_queue = 0;
    handle = datum_next(-1, light_data); // 0x4f0d08 or edx,-1 / mov edi,ebp (ebp = light_data)

    // Expire/refresh pass over every active light.
    while (handle != (datum_index)0xffffffff) {
        uint8_t *entry = lights_base + (handle & 0xffff) * 0x7c;
        light *l = (light *)entry;
        int32_t marker_link = l->marker_link;

        entry[2] &= 0xf7;
        *(uint32_t *)(entry + 8) = 0xffffffff;

        if (marker_link != -1) {
            Light *tag = (Light *)tag_instances[l->definition_tag & 0xffff].data;

            if ((float)(current_tick - marker_link) <= tag->duration) {
                object *obj = object_try_and_get(l->owner_object, _object_mask_all);
                // 0x4f0daa mov ecx,[ecx+0x2c] -- light->owner_object
                if (obj != 0) {
                    if ((entry[2] & 2) != 0) {
                        cluster_reference_remove_all(handle, (datum_index *)(entry + 0x10), &light_cluster_first);
                        entry[2] &= 0xfb;
                    }
                    object_light_recompute_transform(handle);
                    lights_base = (uint8_t *)light_data->data;
                }
            } else {
                cluster_reference_remove_all(handle, (datum_index *)(entry + 0x10), &light_cluster_first);
                // 0x4f0dc8 lea eax,[ebp+0x10] / push eax / push edi / mov ebx,0x860b20
                datum_delete(light_data, handle); // 0x4f0d9f mov edx,edi / mov eax,ebp
            }
        }

        // Advance to the next live slot: object_iterator_next-style linear scan.
        {
            int32_t next = (handle & 0xffff) + 1;
            handle = (datum_index)0xffffffff;
            if (next <= 0x7fff && (int16_t)next < light_data->last_index) {
                int16_t *slot = (int16_t *)(lights_base + next * light_data->size);
                while ((int16_t)next < light_data->last_index) {
                    if (*slot != 0) {
                        handle = ((uint32_t)(uint16_t)*slot << 0x10) | (uint16_t)next;
                        break;
                    }
                    next++;
                    slot = (int16_t *)((uint8_t *)slot + light_data->size);
                }
            }
        }
    }

    light_frame_counter++;
    light_render_unknown_7c0 = 1;
    light_active_list_count = structure_bsp_collect_visible_objects(light_active_list, 0x80, 0, 0, 0, 0, 0); // UNSURE: the
        // five jump-table target arguments from the original are not reproducible
    light_render_unknown_7c0 = 0;
    g_007c1480 = 0;
    rasterizer_light_disable_all();

    {
        int16_t i;
        for (i = 0; i < g_007d0390; i++) {
            structure_cluster_add_lens_flares();
        }
    }

    {
        int16_t i;
        for (i = 0; i < light_active_list_count; i++) {
            uint8_t *entry = lights_base + (light_active_list[i] & 0xffff) * 0x7c;
            light *l = (light *)entry;
            Light *tag = (Light *)tag_instances[l->definition_tag & 0xffff].data;
            object *owner = 0;
            float blend;

            // Resolve the owning object (if any) and cache its type mask for the child-luminance step.
            if (l->owner_object != (datum_index)0xffffffff) {
                object_header *headers = (object_header *)object_data->data;
                object_header *resolved = 0;
                int16_t index = (int16_t)l->owner_object;
                if (-1 < index && index < object_data->maximum_count) {
                    object_header *candidate = &headers[(uint16_t)index];
                    if (candidate->identifier != 0) {
                        int16_t salt = (int16_t)(l->owner_object >> 0x10);
                        if (salt == 0 || candidate->identifier == salt) {
                            resolved = candidate;
                        }
                    }
                }
                if (resolved != 0 && (1 << (resolved->type & 0x1f)) != 0) {
                    owner = resolved->data;
                }
            }

            if (l->marker_link == -1) {
                blend = (l->marker_index_secondary == -1) ? 1.0f :
                    ((object *)object_data->data)->function_out_values[0]; // UNSURE: placeholder,
                    // see original: reads object+0x134+marker_index_secondary*4 off the resolved
                    // owner, i.e. owner->function_out_values[marker_index_secondary]
                color_interpolate_argb_with_tint(entry + 0x38, blend);
            } else {
                real fade = transition_function_evaluate(0, (float)(current_tick - l->marker_link) / tag->duration);
                blend = (1.0f - fade) * l->unknown_78;
                color_interpolate(entry + 0x14, *(uint32_t *)((uint8_t *)tag + 0x34), blend);
            }

            if (owner != 0) {
                uint32_t root = 0xffffffff;
                if (l->owner_object != (datum_index)0xffffffff) {
                    object_header *headers = (object_header *)object_data->data;
                    uint32_t walk = l->owner_object;
                    do {
                        root = walk;
                        walk = headers[walk & 0xffff].data->parent_object;
                    } while (walk != (datum_index)0xffffffff);
                }
                {
                    object_header *headers = (object_header *)object_data->data;
                    object *root_obj = headers[root & 0xffff].data;
                    if ((1 << (root_obj->type & 0x1f) & _object_mask_unit) != 0 &&
                        0.0f < *(float *)((uint8_t *)root_obj + 0x37c) && // UNSURE: unit extension
                        (*(uint8_t *)tag_instances[l->definition_tag & 0xffff].data & 0x20) == 0) {
                        float k = 1.0f - *(float *)((uint8_t *)root_obj + 0x37c);
                        *(float *)(entry + 0x14) = k * *(float *)(entry + 0x14);
                        *(float *)(entry + 0x18) = k * *(float *)(entry + 0x18);
                        *(float *)(entry + 0x1c) = k * *(float *)(entry + 0x1c);
                    }
                }
            }

            if (*(float *)(entry + 0x14) != 0.0f || *(float *)(entry + 0x18) != 0.0f ||
                *(float *)(entry + 0x1c) != 0.0f) {
                if ((entry[2] & 1) == 0) {
                    *(uint32_t *)(entry + 0x54) = *(uint32_t *)((uint8_t *)tag + 4);
                    object_lights_update_all_continued(l, tag, 0, i, current_tick);
                    return;
                }
                {
                    float intensity = (blend * *(float *)((uint8_t *)tag + 0xc) +
                        (1.0f - blend) * *(float *)((uint8_t *)tag + 8)) * *(float *)((uint8_t *)tag + 4);

                    *(float *)(entry + 0x54) = intensity;
                    if (intensity != 0.0f) {
                        uint32_t queue_entry[14];

                        queue_entry[0] = *(uint32_t *)&tag; // UNSURE: really the tag data pointer itself
                        queue_entry[1] = *(uint32_t *)(entry + 0x30);
                        queue_entry[2] = *(uint32_t *)(entry + 0x34);
                        queue_entry[3] = *(uint32_t *)(entry + 0x38);
                        queue_entry[4] = *(uint32_t *)(entry + 0x3c);
                        queue_entry[5] = *(uint32_t *)(entry + 0x40);
                        queue_entry[6] = *(uint32_t *)(entry + 0x44);
                        queue_entry[7] = *(uint32_t *)(entry + 0x48);
                        queue_entry[8] = *(uint32_t *)(entry + 0x4c);
                        queue_entry[9] = *(uint32_t *)(entry + 0x50);
                        queue_entry[10] = *(uint32_t *)(entry + 0x18);
                        queue_entry[11] = *(uint32_t *)(entry + 0x14);
                        queue_entry[12] = *(uint32_t *)(entry + 0x1c);
                        (void)queue_entry;

                        if (l->marker_link == -1) {
                            int8_t flashlight_applied = 0;

                            if ((*(uint8_t *)tag & 0x10) == 0) {
                                if (owner != 0 && owner->type == 2 && owner->parent_object != (datum_index)0xffffffff) {
                                    // UNSURE: opaque first-person-weapon marker lookup, see original
                                    int8_t ok = unit_get_first_person_marker_transform(0, &queue_entry[1]);
                                    if (ok != 0) {
                                        flashlight_applied = 1;
                                    }
                                }
                            } else {
                                first_person_weapon_center_flashlight(&queue_entry[1]);
                                flashlight_applied = 1;
                            }
                            if (flashlight_applied) {
                                entry[2] |= 8;
                            }
                        }

                        if (g_007c1480 < 0x80) {
                            int32_t slot = g_007c1480;
                            g_007c1480 = g_007c1480 + 1;
                            // UNSURE: the 0xe-dword queue record copy and rasterizer_light_set's
                            // real arguments are not modeled; see g_007c1484 note above.
                            rasterizer_light_set();
                            light_transient_count_or_queue = (int16_t)slot + 1;
                            *(int32_t *)(entry + 8) = slot;
                        } else {
                            light_transient_count_or_queue = 0;
                            *(int32_t *)(entry + 8) = -1;
                        }
                    }
                }

                if (*(int32_t *)((uint8_t *)tag + 0xb8) != -1) {
                    if (l->marker_link == -1) {
                        int16_t count;

                        if (owner != 0 && owner->type == 2 && owner->parent_object != (datum_index)0xffffffff &&
                            local_player_index_for_weapon(l->owner_object) == DAT_007c3108) {
                            count = first_person_weapon_get_marker_data(l->owner_object, 0, 0, 8);
                        } else {
                            count = 0;
                        }
                        if (count == 0) {
                            count = object_get_node_local_transform(l->owner_object, 0, 0, 8);
                        }
                        {
                            int16_t j;
                            for (j = 0; j < count; j++) {
                                vector3d_pack_normal_11_11_10();
                                vector3d_pack_normal_11_11_10();
                                lens_flare_add_instance();
                            }
                        }
                    } else {
                        vector3d_pack_normal_11_11_10();
                        vector3d_pack_normal_11_11_10();
                        lens_flare_add_instance();
                    }
                }
            }
        }
    }

    {
        int16_t i;
        if (light_transient_count < 1) {
            light_transient_count = 0;
            return;
        }
        for (i = 0; i < light_transient_count; i++) {
            lens_flare_add_instance();
        }
        light_transient_count = 0;
    }
}

#if 0
Original Ghidra decompilation (0x4f0cf0): see out/functions.json / tools/pack.py 0x4f0cf0 for the
full 1591-byte listing (omitted here for length; every field access and call above was
transcribed directly from it, with the render-queue record copy and several jump-table arguments
left unmodeled as noted in the header comment).
#endif
