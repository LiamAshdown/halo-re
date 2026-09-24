// lightning_render
// address 0x4ff010, size 2476 bytes, zero recorded callers (the widget_type_definition render
//   column for lightning (row 4) calls this indirectly)
// name confidence: 0.7 (out/phase4/objects_types_notes.md misattribution table: "0x4ff010 |
//   light_volume_render | lightning_render (widget type 4 render column)")
// rewrite confidence: 0.1 (by a wide margin the largest and most arithmetic-dense function in
//   this module: a fractal lightning-bolt builder that midpoint-displaces a marker chain
//   recursively, then billboards each resulting segment into a quad strip with per-vertex
//   colour and bounding-box tracking, before submitting the draw. Ghidra's own decompile carries
//   four separate warning banners about heritage/deadcode restarts and a >128KB stack frame
//   requiring __chkstk, which is a strong sign the decompilation itself is fragile here. Kept as
//   a maximally literal, raw-offset transliteration; nothing beyond the handle-validation
//   preamble (shared verbatim with light_volume_render.c / glow_render_dispatch.c) should be
//   treated as verified.)
// evidence: types/tags.h Lightning (markers TagReflexive at 0x98/0x9c, matching the LightningMarker
//   0xe4 stride used throughout); the handle-validation preamble matches light_volume_render.c
//   exactly, including its same UNSURE caveats; vector3d_cross_product (0x4052c0, UNSURE, one
//   visible argument, established in object_damage_apply_line_of_sight.c); vector3d_normalize
//   (0x4cd320, ECX -> v, established in src/math/vector3d_normalize.c); vector3d_normalize_with_length
//   (0x401990, ECX -> v, established in antenna_update_physics.c); antenna_tip_jitter (0x4fef40,
//   this file group, itself only 0.3 confidence).
// register convention: Ghidra shows a clean (param_1, param_2, param_3, param_4); param_3 is
//   never read. No implicit register inputs.
// blam-cc: stack -> object_index, lightning_handle, unused, function_context
// reconciled: R77 0x0069c632 uint8 game_render_mode -> int16 rasterizer_vertex_buffer_lock_state (all stores are WORD)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"

extern uint8_t *lightning_instances; // 0x006b8d74, UNSURE: raw table, see light_volume_render.c
extern tag_instance *tag_instances;  // 0x0087bc14
extern uint32_t widget_random_seed;  // 0x00719cd4
extern real_vector3d *shared_constant_vector_696704; // 0x00696704, UNSURE: fallback axis
extern real_vector3d *shared_constant_vector_686b04;  // 0x00686b04, UNSURE: default colour scale
extern float camera_forward_x, camera_forward_y, camera_forward_z; // 0x007c3120/0x007c3124/0x007c3128
extern int16_t rasterizer_vertex_buffer_lock_state; // 0x0069c632, rasterizer.h; WORD stores (R77)

extern int32_t object_get_node_local_transform(uint32_t object_index, char *marker_name,
                                                object_marker *marker, uint32_t flags); // 0x4f6080
extern void antenna_tip_jitter(real_vector3d *amplitude, real_point3d *position, real_matrix4x3 *m); // 0x4fef40
extern void vector3d_cross_product(); // math module, 0x4052c0.
    // No prototype is asserted: Ghidra models fewer or differently-typed arguments here than
    // the other call site(s) of the same address, because the missing operands travel in
    // registers it could not source. The empty parameter list is the convention this module
    // already uses for FUN_00450870 -- one declaration per symbol, no invented signature.
extern real vector3d_normalize_with_length(real_vector3d *v); // 0x401990, vector in ECX
extern void vector3d_normalize(real_vector3d *v); // 0x4cd320, vector in ECX
extern int32_t FUN_00444550(uint32_t a, uint32_t b); // out of module scope, unexamined; begins a
    // geometry batch and returns an opaque handle
extern int32_t FUN_0051bdd0(void);  // out of module scope, unexamined
extern void *FUN_0051be40(void);    // out of module scope, unexamined
extern uint32_t color_pack_argb_from_real(float *argb); // 0x497900, UNSURE: signature guessed
extern void FUN_0051c830(uint32_t a, int32_t b, int32_t c, int32_t d, uint32_t e); // out of
    // module scope, unexamined; submits the built geometry

// UNSURE: placeholder for the per-call widget_random_seed draw this function makes when
// choosing each shard's texture-coordinate bias (`DAT_00719cd4 = ...; fStack_200ec = ...`);
// factored out here for readability, not present as a separate function in the original.
static float glow_random_unit_for_lightning(void)
{
    widget_random_seed = widget_random_seed * 0x19660dU + 0x3c6ef35fU;
    return (float)(widget_random_seed >> 16) * 1.5259022e-05f;
}

void lightning_render(uint32_t object_index, datum_index lightning_handle, uint32_t unused,
                       int32_t *function_context)
    // blam-cc: stack -> object_index, lightning_handle, unused, function_context
{
    uint8_t *tag;

    if (object_index == 0xffffffff || lightning_handle == (datum_index)0xffffffff) {
        return;
    }

    {
        int16_t index = (int16_t)lightning_handle;
        uint8_t *instance = 0;

        if (index >= 0 && index < *(int16_t *)(lightning_instances + 0x2e)) {
            int32_t off = *(int16_t *)(lightning_instances + 0x22) * index;
            int16_t identifier = *(int16_t *)(off + *(int32_t *)(lightning_instances + 0x34));
            int16_t salt = (int16_t)(lightning_handle >> 16);

            off = off + *(int32_t *)(lightning_instances + 0x34);
            if (identifier != 0 && (salt == 0 || salt == identifier)) {
                instance = (uint8_t *)off;
            }
        }
        tag = (uint8_t *)tag_instances[*(uint32_t *)(instance + 4) & 0xffff].data;
    }

    if (*(int32_t *)(tag + 0x98) <= 0) {
        return;
    }

    {
        object_marker root_marker;
        int16_t markers_ok = (int16_t)object_get_node_local_transform(
            object_index, (char *)*(uint32_t *)(tag + 0x9c), &root_marker, 1);
        if (markers_ok <= 0) {
            return;
        }
    }

    {
        uint32_t shader_something = *(uint32_t *)(
            (uint8_t *)tag_instances[*(uint32_t *)(tag + 0x40) & 0xffff].data + 100);
        int32_t device = FUN_00444550(0, 1);

        // Per-shard (top-level marker chain) loop.
        int16_t shard;
        if (device == 0) {
            return;
        }
        for (shard = 0; shard < *(int16_t *)(tag + 2); shard++) {
            // "verts" holds, per node index i, an 8-float record: position.xyz, colour.rgb,
            // alpha, jitter/misc (matching afStack_20020's stride of 8 floats).
            static float verts[32776]; // matches Ghidra's afStack_20020 size; static to avoid an
                                        // unreasonably large stack frame in this rewrite
            int32_t node_count = 0;
            float brightness_scale = 1.0f;
            int first_marker = 1;
            int32_t marker_index;

            if (function_context != 0 && function_context[1] != 0) {
                int16_t sel = *(int16_t *)(tag + 0x2c);
                if (sel > 0 && sel < 5) {
                    brightness_scale = *(float *)(function_context[1] - 4 + sel * 4);
                }
            }

            for (marker_index = 0; marker_index < *(int32_t *)(tag + 0x98); marker_index++) {
                uint8_t *marker_tag = *(uint8_t **)(tag + 0x9c) + marker_index * 0xe4;

                if (first_marker) {
                    object_marker m;
                    real_point3d pos;
                    object_get_node_local_transform(object_index, (char *)marker_tag, &m, 1);
                    pos = m.node_transform.position;
                    verts[1] = pos.y;
                    verts[2] = pos.z;
                    antenna_tip_jitter((real_vector3d *)((uint8_t *)&m + 0x1c) /* UNSURE offset */,
                                        &pos, &m.node_transform);
                    node_count = 0;
                    first_marker = 0;
                }

                if ((marker_tag[0x20] & 1) == 0 && marker_index != *(int32_t *)(tag + 0x98) - 1) {
                    uint16_t octaves = *(uint16_t *)(marker_tag + 0x24);
                    uint8_t *next_marker_tag = marker_tag + 0xe4;
                    object_marker m;
                    real_point3d pos;
                    int32_t base = node_count;
                    int32_t end = base + (1 << (octaves & 0x1f));

                    object_get_node_local_transform(object_index, (char *)next_marker_tag, &m, 1);
                    pos = m.node_transform.position;

                    verts[end * 8 + 0] = pos.x;
                    verts[end * 8 + 1] = pos.y;
                    verts[end * 8 + 2] = pos.z;
                    antenna_tip_jitter((real_vector3d *)((uint8_t *)&m + 0x1c), &pos, &m.node_transform);
                    verts[end * 8 + 3] = *(float *)(next_marker_tag + 0x84);
                    verts[end * 8 + 4] = *(float *)(next_marker_tag + 0x88);
                    verts[end * 8 + 5] = *(float *)(next_marker_tag + 0x8c);
                    verts[end * 8 + 6] = *(float *)(next_marker_tag + 0x90);
                    verts[end * 8 + 7] = *(float *)(next_marker_tag + 0x94);

                    {
                        real_vector3d axis;
                        axis.i = verts[end * 8 + 0] - verts[base * 8 + 0];
                        axis.j = verts[end * 8 + 1] - verts[base * 8 + 1];
                        axis.k = verts[end * 8 + 2] - verts[base * 8 + 2];
                        vector3d_cross_product(&axis);
                        if (vector3d_normalize_with_length(&axis) == 0.0f) {
                            axis = *shared_constant_vector_696704;
                        }

                        // UNSURE: the original recursively midpoint-displaces the chain between
                        // `base` and `end` over `octaves` levels (each level halving both the
                        // displacement magnitude and the step size, in the classic
                        // midpoint-displacement-fractal pattern), writing new samples into
                        // `verts` at indices derived from `base +/- step`. The exact index
                        // arithmetic (Ghidra's iVar21/iVar22/iVar27) was not re-derived
                        // precisely enough to reproduce here; not implemented in this rewrite.
                    }
                    node_count = node_count + (1 << (marker_tag[0x24] & 0x1f));
                } else {
                    rasterizer_vertex_buffer_lock_state = 0xc;
                    if (node_count > 2) {
                        int32_t n = node_count + 1;
                        int32_t frame = FUN_0051bdd0();
                        if (frame != -1) {
                            float *out = (float *)FUN_0051be40();
                            float t_bias = glow_random_unit_for_lightning();
                            float alpha_scale = 1.0f, color_scale_extra = 1.0f;
                            real_vector3d *color_scale = shared_constant_vector_686b04;
                            int32_t i;
                            real_point3d bbmin = {0}, bbmax = {0};

                            if (function_context != 0) {
                                int32_t p1 = function_context[1];
                                int16_t s;
                                if (p1 != 0 && (s = *(int16_t *)(tag + 0x2e)) > 0 && s < 5) {
                                    alpha_scale = *(float *)(p1 - 4 + s * 4);
                                }
                                if (function_context[0] != 0 && (s = *(int16_t *)(tag + 0x30)) > 0 && s < 5) {
                                    color_scale = (real_vector3d *)(function_context[0] - 0xc + s * 0xc);
                                }
                                if (p1 != 0 && (s = *(int16_t *)(tag + 0x32)) > 0 && s < 5) {
                                    color_scale_extra = *(float *)(p1 - 4 + s * 4);
                                }
                            }

                            for (i = 0; i < n; i++) {
                                float *v = &verts[i * 8];
                                float *prev = (i < 1) ? v : &verts[(i - 1) * 8];
                                float *next = (i >= n - 1) ? v : &verts[(i + 1) * 8];
                                float half_width = alpha_scale * v[3];
                                real_vector3d normal;
                                float argb[4];
                                uint32_t packed;
                                float *o = out + i * 12;

                                normal.i = (next[1] - prev[1]) * camera_forward_z - (next[2] - prev[2]) * camera_forward_y;
                                normal.j = (next[2] - prev[2]) * camera_forward_x - (next[0] - prev[0]) * camera_forward_z;
                                normal.k = (next[0] - prev[0]) * camera_forward_y - (next[1] - prev[1]) * camera_forward_x;
                                vector3d_normalize(&normal);

                                argb[0] = color_scale_extra * v[4];
                                argb[1] = v[5] * color_scale->i;
                                argb[2] = v[6] * color_scale->j;
                                argb[3] = v[7] * color_scale->k;
                                packed = color_pack_argb_from_real(argb);

                                o[3] = (float)packed;
                                o[5] = 0.0f;
                                o[0] = normal.i * half_width + v[0];
                                o[1] = normal.j * half_width + v[1];
                                o[2] = normal.k * half_width + v[2];
                                o[4] = (float)i * (1.0f / (float)n) + t_bias;
                                half_width = -half_width;
                                o[6] = normal.i * half_width + v[0];
                                o[7] = normal.j * half_width + v[1];
                                o[9] = (float)packed;
                                o[11] = 1.0f;
                                o[8] = normal.k * half_width + v[2];
                                o[10] = o[4];

                                if (i == 0) {
                                    bbmin.x = bbmax.x = v[0];
                                    bbmin.y = bbmax.y = v[1];
                                    bbmin.z = bbmax.z = v[2];
                                } else {
                                    if (v[0] <= bbmin.x) bbmin.x = v[0];
                                    if (v[1] <= bbmin.y) bbmin.y = v[1];
                                    if (v[2] <= bbmin.z) bbmin.z = v[2];
                                    if (bbmax.x < v[0]) bbmax.x = v[0];
                                    if (bbmax.y < v[1]) bbmax.y = v[1];
                                    if (bbmax.z < v[2]) bbmax.z = v[2];
                                }
                            }

                            // UNSURE: a D3D-style vtable dispatch (matching the same shape used
                            // in flag_render.c) is elided here; only the final submit call is
                            // reproduced.
                            FUN_0051c830(shader_something, n * -2, frame, n * 2 - 2, 0);
                        }
                        first_marker = 1;
                    }
                    rasterizer_vertex_buffer_lock_state = 0;
                }
            }
        }
    }
}

#if 0
Original Ghidra decompilation (0x4ff010): see `python tools/pack.py 0x4ff010` for the full
2476-byte body (out/phase2/objects/04.md). The header above quotes the essential anchors this
rewrite relies on; the full decompile was long enough (350+ lines, four Ghidra warning banners
about deadcode/heritage restarts, a >128KB stack frame) that inlining it here was skipped in
favour of the source reference, consistent with the 0.1 confidence rating on this file.
#endif
