// object_lights_update_all_continued (Ghidra: FUN_004f133c)
// address 0x4f133c, size 694 bytes
// name confidence: 0.2 (still FUN_004f133c in Ghidra; out/phase4/objects_types_notes.md: "the
// tail of object_lights_update_all 0x4f0cf0")
// rewrite confidence: 0.1 (lowest in this batch — this is a genuine mid-loop continuation that
// object_lights_update_all.c tail-calls into for the "not always visible" light case; it resumes
// the SAME per-light loop from the next index using globals the two files share, and its own
// parameters (`unaff_EBP`, `unaff_EDI`, `in_stack_...`) are exactly the caller's live locals at
// the point of the tail call, not a normal argument list)
// evidence: same as object_lights_update_all.c; see that file's header for the field mapping.
// UNSURE: this file re-derives the same fields, opaque rasterizer/decal externals, and render
// queue as object_lights_update_all.c; nothing new is resolved here. The entry parameters below
// are a best-effort reconstruction of what the tail call must be passing (the current light
// pointer/tag pointer/blend value and the loop index), since Ghidra shows them only as
// unmodeled stack/register spills.
// register convention: UNSURE, reconstructed — light *current_light in an EBP-sourced register
// (unaff_EBP), Light *current_tag in an EDI-sourced register (unaff_EDI), int32_t owner_type_hint
// on the stack, int16_t loop_index on the stack, int32_t current_tick on the stack.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"

extern data_array *light_data;         // 0x00860b14
extern tag_instance *tag_instances;    // 0x0087bc14
extern data_array *object_data;        // 0x008603b0
extern int16_t light_transient_count_or_queue; // 0x00860b10
extern int32_t light_active_list_count; // 0x008607c8
extern datum_index *light_active_list;  // 0x008607cc
extern int32_t g_007c1480;             // 0x007c1480, UNSURE: render queue live count
extern int32_t DAT_007c3108;           // UNSURE: not owned by this module
extern int16_t light_transient_count;             // 0x00860b0c, light_transient_count

extern void FUN_0043f7d0(void *out_color, float t);
extern void color_interpolate(void *out_color, uint32_t color_pair, float t);
extern void first_person_weapon_center_flashlight(void *param_1);
extern int8_t FUN_00492c30(int32_t marker, void *out_transform);
extern real transition_function_evaluate(transition_function_t type, real phase); // math
    // module, 0x4ccac0. The transition type travels in CX and is not visible at this call
    // site; the one value Ghidra shows pushed is the phase. UNSURE: type passed as 0.
extern int16_t FUN_00494010(uint32_t object_index);
extern int16_t first_person_weapon_get_marker_data(uint32_t object_index, int32_t marker,
    void *out_buffer, int32_t max_count);
extern int32_t object_get_node_local_transform(uint32_t object_index, char *marker_name,
    object_marker *marker, uint32_t flags); // 0x4f6080, this module -- same declaration as
    // src/objects/object_get_node_local_transform.c's own definition (it was declared here
    // as FUN_004f6080 with a different return type and parameter names)
extern uint32_t vector3d_pack_normal_11_11_10(); // 0x5132d0, same declaration as object_lights_update_all.c
    // and light_transient_add.c
extern void decal_add_to_active_list(void);
extern void rasterizer_light_set(void);
extern void FUN_004f133c(void); // self (the original re-enters itself for each further "not
                                 // always visible" light, exactly as decompiled)

void object_lights_update_all_continued(light *current_light, Light *current_tag,
    object *owner_type_hint, int16_t loop_index, int32_t current_tick)
{
    uint8_t *lights_base = (uint8_t *)light_data->data;
    uint8_t *entry = (uint8_t *)current_light;
    uint8_t *tag = (uint8_t *)current_tag;
    float blend = 0.0f;

    for (;;) {
        if (*(int32_t *)(tag + 0xb8) != -1) {
            if (entry != 0 && current_light->marker_link == -1) {
                int16_t count;

                if (owner_type_hint != 0 && owner_type_hint->type == 2 &&
                    owner_type_hint->parent_object != (datum_index)0xffffffff &&
                    FUN_00494010(current_light->owner_object) == DAT_007c3108) {
                    count = first_person_weapon_get_marker_data(current_light->owner_object, 0, 0, 8);
                } else {
                    count = 0;
                }
                if (count == 0) {
                    count = object_get_node_local_transform(current_light->owner_object, 0, 0, 8);
                }
                {
                    int16_t j;
                    for (j = 0; j < count; j++) {
                        vector3d_pack_normal_11_11_10();
                        vector3d_pack_normal_11_11_10();
                        decal_add_to_active_list();
                    }
                }
            } else {
                vector3d_pack_normal_11_11_10();
                vector3d_pack_normal_11_11_10();
                decal_add_to_active_list();
            }
        }

        // Advance to the next light in the active list, replicating the tail of
        // object_lights_update_all.c's own outer loop.
        for (;;) {
            loop_index = loop_index + 1;
            if (light_active_list_count <= loop_index) {
                int16_t k;
                for (k = 0; k < light_transient_count; k++) {
                    decal_add_to_active_list();
                }
                light_transient_count = 0;
                return;
            }

            entry = lights_base + (light_active_list[loop_index] & 0xffff) * 0x7c;
            current_light = (light *)entry;
            tag = (uint8_t *)tag_instances[current_light->definition_tag & 0xffff].data;
            current_tag = (Light *)tag;

            owner_type_hint = 0;
            if (current_light->owner_object != (datum_index)0xffffffff) {
                object_header *headers = (object_header *)object_data->data;
                object_header *resolved = 0;
                int16_t index = (int16_t)current_light->owner_object;
                if (-1 < index && index < object_data->maximum_count) {
                    object_header *candidate = &headers[(uint16_t)index];
                    if (candidate->identifier != 0) {
                        int16_t salt = (int16_t)(current_light->owner_object >> 0x10);
                        if (salt == 0 || candidate->identifier == salt) {
                            resolved = candidate;
                        }
                    }
                }
                if (resolved != 0) {
                    owner_type_hint = resolved->data;
                }
            }

            if (current_light->marker_link == -1) {
                blend = (current_light->marker_index_secondary == -1) ? 1.0f :
                    ((object *)object_data->data)->function_out_values[0]; // UNSURE: placeholder,
                    // see object_lights_update_all.c's note on the same computation
                FUN_0043f7d0(entry + 0x38, blend);
            } else {
                real fade = transition_function_evaluate(0, (float)(current_tick - current_light->marker_link) /
                    *(float *)(tag + 0xf4));
                blend = (1.0f - fade) * current_light->unknown_78;
                color_interpolate(entry + 0x14, *(uint32_t *)(tag + 0x34), blend);
            }

            if (owner_type_hint != 0) {
                uint32_t root = 0xffffffff;
                object_header *headers = (object_header *)object_data->data;

                if (current_light->owner_object != (datum_index)0xffffffff) {
                    uint32_t walk = current_light->owner_object;
                    do {
                        root = walk;
                        walk = headers[walk & 0xffff].data->parent_object;
                    } while (walk != (datum_index)0xffffffff);
                }
                {
                    object *root_obj = headers[root & 0xffff].data;
                    if ((1 << (root_obj->type & 0x1f) & _object_mask_unit) != 0 &&
                        0.0f < *(float *)((uint8_t *)root_obj + 0x37c) && // UNSURE: unit extension
                        (*(uint8_t *)tag_instances[current_light->definition_tag & 0xffff].data & 0x20) == 0) {
                        float k = 1.0f - *(float *)((uint8_t *)root_obj + 0x37c);
                        *(float *)(entry + 0x14) = k * *(float *)(entry + 0x14);
                        *(float *)(entry + 0x18) = k * *(float *)(entry + 0x18);
                        *(float *)(entry + 0x1c) = k * *(float *)(entry + 0x1c);
                    }
                }
            }

            if (*(float *)(entry + 0x14) != 0.0f || *(float *)(entry + 0x18) != 0.0f ||
                *(float *)(entry + 0x1c) != 0.0f) {
                break;
            }
        }

        if ((entry[2] & 1) == 0) {
            *(uint32_t *)(entry + 0x54) = *(uint32_t *)(tag + 4);
            FUN_004f133c(); // UNSURE: the original re-enters itself here with no visible args
            return;
        }

        {
            float intensity = (blend * *(float *)(tag + 0xc) + (1.0f - blend) * *(float *)(tag + 8)) *
                *(float *)(tag + 4);

            *(float *)(entry + 0x54) = intensity;
            if (intensity == 0.0f) {
                continue;
            }
        }

        if (current_light->marker_link == -1) {
            int8_t flashlight_applied = 0;

            if ((*tag & 0x10) == 0) {
                if (owner_type_hint != 0 && owner_type_hint->type == 2 &&
                    owner_type_hint->parent_object != (datum_index)0xffffffff) {
                    int8_t ok = FUN_00492c30(0, entry + 0x30); // UNSURE: marker/transform args
                    if (ok != 0) {
                        flashlight_applied = 1;
                    }
                }
            } else {
                first_person_weapon_center_flashlight(entry + 0x30);
                flashlight_applied = 1;
            }
            if (flashlight_applied) {
                entry[2] |= 8;
            }
        }

        if (g_007c1480 < 0x80) {
            int32_t slot = g_007c1480;
            g_007c1480 = g_007c1480 + 1;
            // UNSURE: the 0xe-dword render queue record copy is not modeled here either.
            rasterizer_light_set();
            light_transient_count_or_queue = (int16_t)slot + 1;
            *(int32_t *)(entry + 8) = slot;
        } else {
            light_transient_count_or_queue = 0;
            *(int32_t *)(entry + 8) = -1;
        }
    }
}

#if 0
Original Ghidra decompilation (0x4f133c): see out/functions.json / tools/pack.py 0x4f133c for the
full 694-byte listing (omitted here for length). This is the direct continuation of
object_lights_update_all's own per-light loop; every field access and call above was transcribed from
it, with the same render-queue record copy left unmodeled as in object_lights_update_all.c.
#endif
