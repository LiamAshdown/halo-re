// actor_movement_choose_avoidance_direction  (Ghidra: already named)
// address 0x4193d0, size 3829 bytes
// name confidence: 0.55   rewrite confidence: 0.2
// evidence: out/phase4/ai_functions.md "Core obstacle-avoidance steering computation: samples
//   several candidate directions around the desired heading and picks the best compromise
//   between goal direction and obstacle clearance."; types/ai.h actor_movement_context (this
//   is the function whose stack frame fixes that struct's 0x6048 size -- the local variable
//   Ghidra calls `local_6048` is exactly that struct, built on the stack and handed to
//   actor_movement_collect_obstacle_candidates by address) and actor.unknown_5d8/unknown_5f0
//   ("read by the avoidance sampler", both already attributed to this function by the header).
// register convention: a plain stack signature Ghidra recovered in full -- no leftover
//   in_EAX/unaff_ style registers.
//   // blam-cc: stack -> actor_index, desired_direction, out_direction, out_speed_scale
//
// Kept very close to the Ghidra decompilation (original labels/variable names/control flow
// preserved) given its size, the depth of the arithmetic, and the number of unresolved
// out-of-range callees; see the UNSURE notes below. Only DAT_ globals, the actor pointer,
// and the actor_movement_context stack buffer (which this function's own frame size proves)
// have been given names; everything else keeps its Ghidra shape.
//
// UNSURE, broadly -- this function needs a disassembly review pass:
//  - actor_movement_test_obstacle_ray (0x418f70) and actor_avoidance_interpolate_sample (0x419240) are outside
//    this session's range and are each called with 0-2 visible arguments where they clearly
//    need more (at minimum the context this function just built); their declared signatures
//    below are read off these call sites only, and the extra implicit arguments are not
//    reconstructed -- calls to them are left exactly as literal as the decompiled C, using
//    only the operands Ghidra shows.
//  - The exact geometry of the two direction-weight sampling loops (in particular the two
//    unnamed constant tables at 0x0065586c and 0x00655868, and the raw actor-byte read at
//    actor+0x5c9) is transcribed arithmetically but not independently understood.
//  - Ghidra gives the actor pointer a spurious `float` type for part of the function; this
//    rewrite keeps a real `actor *a` pointer throughout instead (the original's `local_60bc`
//    -> `fVar2` stack-slot reuse near the end has no behavioural effect once a real pointer
//    is used, since nothing overwrites `a`).
//  - `local_60bc`'s *second* life (after the actor pointer is no longer needed, inside the
//    LAB_00419e38 branch) is modeled as a fresh float local, `sample_z`.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"
#include "ai.h"

extern data_array *actor_data;      // 0x00880360
extern data_array *object_data;     // 0x008603b0
extern int32_t bsp_generation;      // 0x00746f9c, the structure BSP pointer, used here as an opaque handle
extern int32_t bsp_index;      // 0x00746f98
extern const real_vector3d *global_origin3d_pointer; // 0x00696714

extern double sqrt(double x);
extern double fabs(double x);

extern void object_get_position(real_point3d *out_position, datum_index object_index); // 0x4f6900, EAX->out, ECX->object_index

extern void actor_movement_collect_obstacle_candidates(actor_movement_context *context); // 0x418ce0, stack -> context

// Casts a probe ray and reports a clearance fraction and a hit count. UNSURE signature: the
// two calls below pass different visible arguments (see file header); both are transcribed
// literally rather than unified into one reconstructed prototype.
extern int16_t actor_movement_test_obstacle_ray(float *out_clearance, void *param_2); // 0x418f70, not yet rewritten (this module)

// Finds where a direction vector crosses the boundary of the small convex polygon implied
// by the 8 direction weights. UNSURE signature, read off these call sites only.
extern uint8_t actor_avoidance_interpolate_sample(int32_t count, float *dir_weight, void *out_a, void *out_b); // 0x419240, not yet rewritten (this module)

extern real vector3d_angle_between_4cd4f0(real_vector3d *a, real_vector3d *b); // 0x4cd4f0, ECX->a, EDX->b

// The two constant blend-weight tables the sampling loops index into. Neither has an
// established name or module owner; see UNSURE above.
extern const float DAT_00655748[]; // 0x00655748, 9 rows of 8 floats (the first sampling loop)
extern const float DAT_0065586c[]; // 0x0065586c
extern const float DAT_00655868[]; // 0x00655868, unused directly (kept for reference only)

// The 8-direction circle table (types/ai.h), reused here both as an offset source in the
// sampling loop and as the final chosen-direction basis.
extern float actor_avoidance_circle[8][3]; // 0x00880540, stride 0x0c (3 floats)

void actor_movement_choose_avoidance_direction(uint32_t param_1, float *param_2, float *param_3, float *param_4)
{
    actor *a = &((actor *)actor_data->data)[param_1 & 0xffffu];
    actor_movement_context ctx;
    object *unit_object = 0;
    uint32_t unit_index;

    float fVar1, fVar2, fVar3, fVar5, fVar6;
    uint8_t bVar7;
    int16_t sVar10, sVar13;
    uint16_t uVar11;
    int32_t iVar12, iVar19;
    uint32_t uVar14, uVar16, uVar20, uVar22, uVar23;
    real angle;

    float ax[3]; // "local_60d8" (ax[0]) followed immediately by "local_60d4[0..1]" (ax[1..2]);
                 // kept contiguous because one read below deliberately walks 4 bytes before
                 // ax[1] to reach ax[0] (matching the two locals' adjacency on the original
                 // stack frame -- see UNSURE above).
    float local_60c8, local_60c4, local_60c0;
    float local_60b4;
    float *dir_cursor;             // "local_60b0", pointer-walk life (the sampling loop)
    float speed_component = 0.0f;  // "local_60b0", second life: actor_avoidance_interpolate_sample's third output
    float local_60ac;
    float local_60a8;
    int16_t local_60a4[2]; // written 2 bytes at a time by actor_movement_test_obstacle_ray's second output
    float dir_weight[18];  // "local_6090"; only [0..8] are used
    float best_weight = 0.0f;      // "local_60b8", second life: the winning direction's weight
    float out_scale = 0.0f;        // "puVar4"

    local_60c8 = global_origin3d_pointer->i;
    local_60c4 = global_origin3d_pointer->j;
    local_60c0 = global_origin3d_pointer->k;

    unit_index = a->active_unit_index;
    if (unit_index == (uint32_t)-1) {
        unit_index = a->unit_index;
        out_scale = 0.0f;
        if (unit_index == (uint32_t)-1) {
            goto LAB_0041a299;
        }
    }
    unit_object = ((object_header *)object_data->data)[unit_index & 0xffff].data;

    ctx.bsp_generation = bsp_generation;
    ctx.bsp_index = bsp_index;
    object_get_position(&ctx.position, unit_index);
    ctx.forward = unit_object->forward;
    ctx.up = unit_object->up;
    ctx.left.i = ctx.forward.k * ctx.up.j - ctx.up.k * ctx.forward.j;
    ctx.left.j = ctx.up.k * ctx.forward.i - ctx.up.i * ctx.forward.k;
    ctx.left.k = ctx.up.i * ctx.forward.j - ctx.forward.i * ctx.up.j;
    ctx.unknown_6040 = 1.0f;
    ctx.search_radius = 12.0f;
    ctx.unit_index = unit_index;

    actor_movement_collect_obstacle_candidates(&ctx);

    sVar10 = a->unknown_5d8;
    dir_weight[0] = 0.0f; dir_weight[1] = 0.0f; dir_weight[2] = 0.0f; dir_weight[3] = 0.0f;
    dir_weight[4] = 0.0f; dir_weight[5] = 0.0f; dir_weight[6] = 0.0f; dir_weight[7] = 0.0f;
    local_60ac = 0.0f;
    dir_weight[8] = 0.0f;
    if (-1 < sVar10 && sVar10 < 8) {
        iVar12 = sVar10;
        uVar14 = ((uint32_t)(iVar12 + 1) & 0x80000007u);
        if ((int32_t)uVar14 < 0) { uVar14 = (uVar14 - 1 | 0xfffffff8u) + 1; }
        uVar16 = ((uint32_t)(iVar12 + 2) & 0x80000007u);
        if ((int32_t)uVar16 < 0) { uVar16 = (uVar16 - 1 | 0xfffffff8u) + 1; }
        uVar20 = ((uint32_t)(iVar12 + 7) & 0x80000007u);
        if ((int32_t)uVar20 < 0) { uVar20 = (uVar20 - 1 | 0xfffffff8u) + 1; }
        uVar23 = ((uint32_t)(iVar12 + 6) & 0x80000007u);
        if ((int32_t)uVar23 < 0) { uVar23 = (uVar23 - 1 | 0xfffffff8u) + 1; }
        dir_weight[iVar12] = dir_weight[iVar12] + 0.4f;
        dir_weight[(int16_t)uVar14] = dir_weight[(int16_t)uVar14] + 0.32000002f;
        dir_weight[(int16_t)uVar16] = dir_weight[(int16_t)uVar16] + 0.2f;
        dir_weight[(int16_t)uVar20] = dir_weight[(int16_t)uVar20] + 0.32000002f;
        dir_weight[(int16_t)uVar23] = dir_weight[(int16_t)uVar23] + 0.2f;
    }

    {
        const float *pfVar18 = DAT_00655748;
        int32_t remaining = 9;
        do {
            sVar10 = actor_movement_test_obstacle_ray(&local_60a8, 0);
            if (0 < sVar10) {
                const float *pfVar17 = pfVar18;
                float *pfVar15 = dir_weight;
                fVar2 = 1.0f - local_60a8;
                iVar12 = 8;
                do {
                    fVar3 = fVar2 + fVar2;
                    if (1.0f < fVar2 + fVar2) { fVar3 = 1.0f; }
                    fVar1 = *pfVar17;
                    pfVar17 = pfVar17 + 1;
                    iVar12 = iVar12 - 1;
                    *pfVar15 = fVar3 * fVar1 + *pfVar15;
                    pfVar15 = pfVar15 + 1;
                } while (iVar12 != 0);
                if (local_60ac <= fVar2) { local_60ac = fVar2; }
            }
            pfVar18 = pfVar18 + 8;
            remaining = remaining - 1;
        } while (remaining != 0);
    }

    dir_cursor = dir_weight;
    {
        const uint8_t *local_60b8 = (const uint8_t *)a + 0x5c9; // UNSURE: raw actor bytes, see file header
        uVar14 = 2;
        local_60a8 = 1.12104e-44f; // reinterpreted below as a plain decrementing int counter
        do {
            int16_t *psVar24_hi;
            const uint8_t *pbVar21;
            int32_t counter2;

            {
                int16_t *local_60dc = local_60a4;
                float *pfVar18b = &ax[0];
                pbVar21 = local_60b8 - 1;
                counter2 = 2;
                do {
                    uVar11 = (uint16_t)actor_movement_test_obstacle_ray(pfVar18b, (void *)pbVar21);
                    *local_60dc = (int16_t)uVar11;
                    local_60dc = local_60dc + 1;
                    pbVar21 = pbVar21 + 1;
                    pfVar18b = pfVar18b + 1;
                    counter2 = counter2 - 1;
                } while (counter2 != 0);
            }
            fVar2 = 0.0f;
            bVar7 = 0;
            psVar24_hi = &local_60a4[1];
            iVar12 = 0;
            iVar19 = 2;
            pbVar21 = local_60b8;
            do {
                fVar3 = 1.0f;
                if (*psVar24_hi == 0) {
                    if (bVar7) {
                        fVar2 = fVar3 * *(const float *)((const uint8_t *)DAT_0065586c + iVar12) + fVar2;
                    } else if (*pbVar21 < 0x4b) {
                        fVar2 = *(const float *)((const uint8_t *)DAT_0065586c + iVar12) * 0.0f + fVar2;
                    } else {
                        fVar3 = 1.0f - 75.0f / (float)*pbVar21;
                        if (0.0f <= fVar3) {
                            if (1.0f < fVar3) { fVar3 = 1.0f; }
                            fVar2 = fVar3 * *(const float *)((const uint8_t *)DAT_0065586c + iVar12) + fVar2;
                        } else {
                            fVar2 = *(const float *)((const uint8_t *)DAT_0065586c + iVar12) * 0.0f + fVar2;
                        }
                    }
                } else {
                    fVar3 = 1.0f - *(const float *)((const uint8_t *)&ax[1] + iVar12);
                    fVar3 = fVar3 + fVar3;
                    if (1.0f < fVar3) { fVar3 = 1.0f; }
                    bVar7 = 1;
                    fVar2 = fVar2 - fVar3 * *(const float *)((const uint8_t *)DAT_0065586c + iVar12);
                }
                psVar24_hi = psVar24_hi - 1;
                pbVar21 = pbVar21 - 1;
                iVar12 = iVar12 - 4;
                iVar19 = iVar19 - 1;
            } while (iVar19 != 0);

            uVar16 = (uVar14 - 1) & 0x80000007u;
            if ((int32_t)uVar16 < 0) { uVar16 = (uVar16 - 1 | 0xfffffff8u) + 1; }
            uVar20 = uVar14 & 0x80000007u;
            if ((int32_t)uVar20 < 0) { uVar20 = (uVar20 - 1 | 0xfffffff8u) + 1; }
            uVar23 = (uVar14 + 5) & 0x80000007u;
            if ((int32_t)uVar23 < 0) { uVar23 = (uVar23 - 1 | 0xfffffff8u) + 1; }
            uVar22 = (uVar14 + 4) & 0x80000007u;
            if ((int32_t)uVar22 < 0) { uVar22 = (uVar22 - 1 | 0xfffffff8u) + 1; }

            *dir_cursor = fVar2 + *dir_cursor;
            local_60b8 = local_60b8 + 2;
            dir_cursor = dir_cursor + 1;
            uVar14 = uVar14 + 1;
            dir_weight[(int16_t)uVar16] = fVar2 * 0.8f + dir_weight[(int16_t)uVar16];
            {
                float half = fVar2 * 0.5f;
                local_60a8 = (float)((int32_t)local_60a8 - 1);
                dir_weight[(int16_t)uVar20] = half + dir_weight[(int16_t)uVar20];
                dir_weight[(int16_t)uVar23] = fVar2 * 0.8f + dir_weight[(int16_t)uVar23];
                dir_weight[(int16_t)uVar22] = half + dir_weight[(int16_t)uVar22];
            }
        } while (local_60a8 != 0.0f);
    }

    fVar2 = (real)sqrt((double)(unit_object->angular_velocity.k * unit_object->angular_velocity.k +
                               unit_object->angular_velocity.j * unit_object->angular_velocity.j +
                               unit_object->angular_velocity.i * unit_object->angular_velocity.i));
    local_60a8 = 0.0f;
    if (0.02f < fVar2) {
        local_60a8 = (fVar2 - 0.02f) * 12.5f;
        if (1.0f < local_60a8) { local_60a8 = 1.0f; }
        local_60a8 = local_60a8 * 0.8f;
        fVar2 = ctx.up.i * unit_object->angular_velocity.i + ctx.up.k * unit_object->angular_velocity.k +
                ctx.up.j * unit_object->angular_velocity.j;
        fVar3 = -(ctx.left.i * unit_object->angular_velocity.i + ctx.left.k * unit_object->angular_velocity.k +
                  ctx.left.j * unit_object->angular_velocity.j);
        fVar1 = (real)sqrt((double)(fVar2 * fVar2 + fVar3 * fVar3));
        if (0.0001f <= (real)fabs((double)fVar1)) {
            fVar5 = 1.0f / fVar1;
            if (0.0f < fVar1) {
                int32_t out_a_unused;
                float out_b_trust;
                uint8_t cVar9 = actor_avoidance_interpolate_sample(8, dir_weight, &out_a_unused, &out_b_trust);
                if (cVar9 != 0 && 0.5f < out_b_trust) {
                    const float *pfVar15 = &actor_avoidance_circle[0][1];
                    float *pfVar18c = dir_weight;
                    iVar12 = 8;
                    do {
                        fVar1 = fVar2 * fVar5 * pfVar15[0] + fVar5 * 0.0f * pfVar15[-1] + fVar5 * fVar3 * pfVar15[1];
                        if (fVar1 < 0.0f) { *pfVar18c = fVar1 * local_60a8 + *pfVar18c; }
                        pfVar15 = pfVar15 + 3;
                        pfVar18c = pfVar18c + 1;
                        iVar12 = iVar12 - 1;
                    } while (iVar12 != 0);
                }
            }
        }
    }

    // Pick the direction with the largest accumulated weight.
    sVar10 = -1;
    {
        float best = -3.402823466e+38f;
        int16_t sVar13b = 0;
        float *pfVar18d = dir_weight;
        do {
            if (best < *pfVar18d) { best = *pfVar18d; sVar10 = sVar13b; }
            sVar13b = sVar13b + 1;
            pfVar18d = pfVar18d + 1;
        } while (sVar13b < 8);
        best_weight = best;
    }

    fVar2 = *param_2;
    fVar3 = param_2[1];
    fVar1 = param_2[2];
    ax[0] = global_origin3d_pointer->i;
    ax[1] = global_origin3d_pointer->j;
    ax[2] = global_origin3d_pointer->k;
    local_60b4 = 1.0f;
    speed_component = 0.0f;
    fVar5 = (real)sqrt((double)(fVar2 * fVar2 + fVar3 * fVar3 + fVar1 * fVar1));
    if (0.0001f <= (real)fabs((double)fVar5)) {
        fVar6 = 1.0f / fVar5;
        fVar2 = fVar2 * fVar6;
        fVar3 = fVar3 * fVar6;
        fVar1 = fVar1 * fVar6;
        if (0.0f < fVar5) {
            ax[0] = 0.0f;
            local_60b4 = ctx.forward.i * fVar2 + ctx.forward.j * fVar3 + ctx.forward.k * fVar1;
            ax[1] = fVar2 * ctx.left.i + fVar1 * ctx.left.k + fVar3 * ctx.left.j;
            ax[2] = fVar2 * ctx.up.i + fVar1 * ctx.up.k + fVar3 * ctx.up.j;
            fVar2 = (real)sqrt((double)(ax[1] * ax[1] + ax[2] * ax[2]));
            if (0.0001f <= (real)fabs((double)fVar2)) {
                fVar3 = 1.0f / fVar2;
                ax[0] = fVar3 * 0.0f;
                ax[1] = ax[1] * fVar3;
                ax[2] = fVar3 * ax[2];
                if (0.0f < fVar2) {
                    int32_t out_a_unused2;
                    actor_avoidance_interpolate_sample(8, dir_weight, &out_a_unused2, &speed_component);
                }
            }
        }
    }

    // local_60dc in the original: the byte-pointer/float-pointer bit patterns left over from
    // the max-weight search (`best_weight`) and the actor_avoidance_interpolate_sample call just above
    // (`speed_component`), both reinterpreted as plain floats and subtracted.
    {
        float weight_margin = best_weight - speed_component;
        float turn_gate;

        if (local_60ac <= 0.6f) {
            turn_gate = local_60ac * 3.3333333f;
            if (1.0f <= turn_gate) { turn_gate = 1.0f; }
        } else {
            fVar3 = (local_60ac - 0.6f) * 2.5000002f;
            if (1.0f <= fVar3) { fVar3 = 1.0f; }
            turn_gate = fVar3 + 1.0f;
        }

        if (local_60b4 < -0.2f) {
            goto LAB_00419d85;
        }
        sVar13 = a->unknown_5f0;
        if (sVar13 == -1 || 0x59 < sVar13) {
            float av_sq = unit_object->angular_velocity.k * unit_object->angular_velocity.k +
                          unit_object->angular_velocity.j * unit_object->angular_velocity.j +
                          unit_object->angular_velocity.i * unit_object->angular_velocity.i;
            if (av_sq <= 0.0025000002f) {
                if (turn_gate <= 0.5f) { goto LAB_00419d85; }
                goto LAB_00419e38;
            }
            if (2.0f < weight_margin && 2.0f < best_weight) {
                goto LAB_00419e38;
            }
        LAB_00419d85:
            a->unknown_5f0 = -1;
            out_scale = turn_gate;
            if (local_60b4 < 0.5f) {
                if (1.3f < weight_margin &&
                    0.5f < ax[0] * actor_avoidance_circle[sVar10][0] +
                           ax[1] * actor_avoidance_circle[sVar10][1] +
                           ax[2] * actor_avoidance_circle[sVar10][2]) {
                    float scaled = weight_margin * 0.7692308f - 0.5f;
                    if (0.0f <= scaled) {
                        if (1.0f < scaled) { scaled = 1.0f; }
                    } else {
                        scaled = 0.0f;
                    }
                    if (scaled <= turn_gate) { scaled = turn_gate; }
                    local_60c0 = scaled * 1.0471976f;
                    if (0.0f < ax[2] * actor_avoidance_circle[sVar10][1] -
                              ax[1] * actor_avoidance_circle[sVar10][2]) {
                        local_60c0 = -local_60c0;
                    }
                    a->unknown_5d8 = sVar10;
                    local_60c8 = ctx.forward.i * local_60c0;
                    local_60c4 = ctx.forward.j * local_60c0;
                    local_60c0 = ctx.forward.k * local_60c0;
                    out_scale = scaled;
                    goto LAB_0041a299;
                }
                a->unknown_5d8 = -1;
                out_scale = dir_weight[8];
                goto LAB_0041a299;
            }
            if (local_60ac <= 0.0f) {
                a->unknown_5d8 = -1;
                out_scale = dir_weight[8];
                goto LAB_0041a299;
            }
            {
                float sample_x = actor_avoidance_circle[sVar10][1];
                float sample_y = -actor_avoidance_circle[sVar10][2];
                local_60c8 = sample_x * ctx.up.i + ctx.left.i * sample_y + global_origin3d_pointer->i;
                local_60c4 = ctx.up.j * sample_x + ctx.left.j * sample_y + global_origin3d_pointer->j;
                local_60c0 = ctx.up.k * sample_x + ctx.left.k * sample_y + global_origin3d_pointer->k;
                fVar3 = (real)sqrt((double)(local_60c8 * local_60c8 + local_60c4 * local_60c4 + local_60c0 * local_60c0));
                if (0.0001f <= (real)fabs((double)fVar3)) {
                    fVar1 = 1.0f / fVar3;
                    local_60c8 = local_60c8 * fVar1;
                    local_60c4 = local_60c4 * fVar1;
                    local_60c0 = local_60c0 * fVar1;
                    if (0.0f < fVar3) {
                        fVar3 = turn_gate * 1.0471976f;
                        local_60c8 = local_60c8 * fVar3;
                        local_60c4 = local_60c4 * fVar3;
                        local_60c0 = local_60c0 * fVar3;
                    }
                }
            }
        } else {
        LAB_00419e38:
            if (sVar13 == -1) {
                a->unknown_5f0 = 0;
            } else {
                a->unknown_5f0 = sVar13 + 1;
            }
            {
                float sample_x = actor_avoidance_circle[sVar10][0];
                float sample_y = actor_avoidance_circle[sVar10][1];
                float sample_z = actor_avoidance_circle[sVar10][2];
                float t0, t1;
                fVar5 = sample_z * ctx.up.i + sample_y * ctx.left.i + ctx.forward.i * sample_x + global_origin3d_pointer->i;
                fVar6 = ctx.up.j * sample_z + ctx.left.j * sample_y + ctx.forward.j * sample_x + global_origin3d_pointer->j;
                fVar3 = ctx.up.k * sample_z + ctx.left.k * sample_y + ctx.forward.k * sample_x + global_origin3d_pointer->k;
                local_60a8 = fVar3 * param_2[1] - fVar6 * param_2[2];
                t0 = fVar5 * param_2[2] - fVar3 * *param_2;
                t1 = fVar6 * *param_2 - fVar5 * param_2[1];
                fVar3 = (real)sqrt((double)(local_60a8 * local_60a8 + t0 * t0 + t1 * t1));
                if (0.0001f <= (real)fabs((double)fVar3)) {
                    real_vector3d axis, desired;
                    ax[2] = 1.0f / fVar3;
                    ax[0] = local_60a8 * ax[2];
                    ax[1] = t0 * ax[2];
                    ax[2] = t1 * ax[2];
                    if (0.0f < fVar3) {
                        axis.i = ax[0]; axis.j = ax[1]; axis.k = ax[2];
                        desired.i = param_2[0]; desired.j = param_2[1]; desired.k = param_2[2];
                        angle = vector3d_angle_between_4cd4f0(&axis, &desired);
                        local_60c8 = ax[0] * angle;
                        local_60c4 = ax[1] * angle;
                        local_60c0 = ax[2] * angle;
                    }
                }
            }
            {
                float scaled = (2.0f - speed_component) * 0.5f - 0.5f;
                if (0.0f <= scaled) {
                    if (1.0f < scaled) { scaled = 1.0f; }
                } else {
                    scaled = 0.0f;
                }
                out_scale = scaled;
                if (scaled <= turn_gate) {
                    a->unknown_5d8 = sVar10;
                    out_scale = turn_gate;
                    goto LAB_0041a299;
                }
            }
        }
        a->unknown_5d8 = sVar10;
    }

LAB_0041a299:
    *param_3 = local_60c8;
    param_3[1] = local_60c4;
    param_3[2] = local_60c0;
    *param_4 = out_scale;
}

#if 0
Original Ghidra decompilation (0x4193d0):

/* WARNING: Function: __chkstk replaced with injection: alloca_probe */

void actor_movement_choose_avoidance_direction
               (uint param_1,float *param_2,float *param_3,float *param_4)

{
  float fVar1;
  float fVar2;
  float fVar3;
  undefined4 *puVar4;
  float fVar5;
  float fVar6;
  bool bVar7;
  undefined *puVar8;
  char cVar9;
  short sVar10;
  undefined2 uVar11;
  int iVar12;
  short sVar13;
  uint uVar14;
  float *pfVar15;
  uint uVar16;
  float *pfVar17;
  float *pfVar18;
  int iVar19;
  uint uVar20;
  byte *pbVar21;
  uint uVar22;
  uint uVar23;
  short *psVar24;
  float10 fVar25;
  undefined4 *local_60dc;
  float local_60d8;
  float local_60d4 [2];
  undefined4 *local_60cc;
  float local_60c8;
  float local_60c4;
  float local_60c0;
  float local_60bc;
  byte *local_60b8;
  float local_60b4;
  float *local_60b0;
  float local_60ac;
  float local_60a8;
  undefined4 local_60a4;
  float local_60a0;
  float local_609c;
  float local_6098;
  int local_6094;
  float local_6090 [18];
  undefined4 local_6048;
  undefined4 local_6044;
  uint local_6040;
  float local_6030;
  float local_602c;
  float local_6028;
  float local_6024;
  float local_6020;
  float local_601c;
  float local_6018;
  float local_6014;
  float local_6010;
  undefined4 local_8;
  undefined4 local_4;

  local_4 = 0x4193da;
  local_6090[8] = 0.0;
  local_60c8 = *(float *)PTR_DAT_00696714;
  local_60c4 = *(float *)(PTR_DAT_00696714 + 4);
  local_60c0 = *(float *)(PTR_DAT_00696714 + 8);
  local_60bc = (float)((param_1 & 0xffff) * 0x724 + *(int *)(DAT_00880360 + 0x34));
  local_6040 = *(uint *)((int)local_60bc + 0x158);
  if ((local_6040 == 0xffffffff) &&
     (local_6040 = *(uint *)((int)local_60bc + 0x18), puVar4 = (undefined4 *)0.0,
     local_6040 == 0xffffffff)) goto LAB_0041a299;
  iVar12 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (local_6040 & 0xffff) * 0xc);
  local_6048 = DAT_00746f9c;
  local_6044 = DAT_00746f98;
  local_6094 = iVar12;
  object_get_position();
  pfVar18 = (float *)(iVar12 + 0x74);
  local_6030 = *pfVar18;
  local_602c = *(float *)(iVar12 + 0x78);
  local_6028 = *(float *)(iVar12 + 0x7c);
  pfVar15 = (float *)(iVar12 + 0x80);
  local_6018 = *pfVar15;
  local_6014 = *(float *)(iVar12 + 0x84);
  local_6010 = *(float *)(iVar12 + 0x88);
  local_6024 = *(float *)(iVar12 + 0x7c) * *(float *)(iVar12 + 0x84) -
               *(float *)(iVar12 + 0x88) * *(float *)(iVar12 + 0x78);
  local_6020 = *(float *)(iVar12 + 0x88) * *pfVar18 - *pfVar15 * *(float *)(iVar12 + 0x7c);
  local_601c = *pfVar15 * *(float *)(iVar12 + 0x78) - *pfVar18 * *(float *)(iVar12 + 0x84);
  local_4 = 0x41400000;
  local_8 = 0x3f800000;
  actor_movement_collect_obstacle_candidates(&local_6048);
  sVar10 = *(short *)((int)local_60bc + 0x5d8);
  local_6090[0] = 0.0;
  local_6090[1] = 0.0;
  local_6090[2] = 0.0;
  local_6090[3] = 0.0;
  local_6090[4] = 0.0;
  local_6090[5] = 0.0;
  local_6090[6] = 0.0;
  local_60ac = 0.0;
  local_6090[7] = 0.0;
  if ((-1 < sVar10) && (sVar10 < 8)) {
    iVar12 = (int)sVar10;
    uVar14 = iVar12 + 1U & 0x80000007;
    if ((int)uVar14 < 0) {
      uVar14 = (uVar14 - 1 | 0xfffffff8) + 1;
    }
    uVar16 = iVar12 + 2U & 0x80000007;
    if ((int)uVar16 < 0) {
      uVar16 = (uVar16 - 1 | 0xfffffff8) + 1;
    }
    uVar20 = iVar12 + 7U & 0x80000007;
    if ((int)uVar20 < 0) {
      uVar20 = (uVar20 - 1 | 0xfffffff8) + 1;
    }
    uVar23 = iVar12 + 6U & 0x80000007;
    if ((int)uVar23 < 0) {
      uVar23 = (uVar23 - 1 | 0xfffffff8) + 1;
    }
    local_6090[iVar12] = local_6090[iVar12] + 0.4;
    local_6090[(short)uVar14] = local_6090[(short)uVar14] + 0.32000002;
    local_6090[(short)uVar16] = local_6090[(short)uVar16] + 0.2;
    local_6090[(short)uVar20] = local_6090[(short)uVar20] + 0.32000002;
    local_6090[(short)uVar23] = local_6090[(short)uVar23] + 0.2;
  }
  pfVar18 = (float *)&DAT_00655748;
  local_60dc = (undefined4 *)0x9;
  do {
    sVar10 = actor_movement_test_obstacle_ray(&local_60a8,0);
    if (0 < sVar10) {
      fVar2 = 1.0 - local_60a8;
      iVar12 = 8;
      pfVar15 = local_6090;
      pfVar17 = pfVar18;
      do {
        fVar3 = fVar2 + fVar2;
        if (1.0 < fVar2 + fVar2) {
          fVar3 = 1.0;
        }
        fVar1 = *pfVar17;
        pfVar17 = pfVar17 + 1;
        iVar12 = iVar12 + -1;
        *pfVar15 = fVar3 * fVar1 + *pfVar15;
        pfVar15 = pfVar15 + 1;
      } while (iVar12 != 0);
      if (local_60ac <= fVar2) {
        local_60ac = fVar2;
      }
    }
    pfVar18 = pfVar18 + 8;
    local_60dc = (undefined4 *)((int)local_60dc + -1);
  } while (local_60dc != (undefined4 *)0x0);
  local_60b0 = local_6090;
  local_60b8 = (byte *)((int)local_60bc + 0x5c9);
  uVar14 = 2;
  local_60cc = &DAT_00880380;
  local_60a8 = 1.12104e-44;
  do {
    local_60dc = &local_60a4;
    pfVar18 = &local_60d8;
    pbVar21 = local_60b8 + -1;
    local_60b4 = 2.8026e-45;
    do {
      uVar11 = actor_movement_test_obstacle_ray(pfVar18,pbVar21);
      *(undefined2 *)local_60dc = uVar11;
      local_60cc = local_60cc + 7;
      local_60dc = (undefined4 *)((int)local_60dc + 2);
      pbVar21 = pbVar21 + 1;
      pfVar18 = pfVar18 + 1;
      local_60b4 = (float)((int)local_60b4 + -1);
    } while (local_60b4 != 0.0);
    fVar2 = 0.0;
    bVar7 = false;
    psVar24 = (short *)((int)&local_60a4 + 2);
    iVar12 = 0;
    iVar19 = 2;
    pbVar21 = local_60b8;
    do {
      fVar3 = 1.0;
      if (*psVar24 == 0) {
        if (bVar7) {
LAB_004197f5:
          fVar2 = fVar3 * *(float *)((int)&DAT_0065586c + iVar12) + fVar2;
        }
        else if (*pbVar21 < 0x4b) {
          fVar2 = *(float *)((int)&DAT_0065586c + iVar12) * 0.0 + fVar2;
        }
        else {
          fVar3 = 1.0 - 75.0 / (float)*pbVar21;
          if (0.0 <= fVar3) {
            if (1.0 < fVar3) {
              fVar3 = 1.0;
            }
            goto LAB_004197f5;
          }
          fVar2 = *(float *)((int)&DAT_0065586c + iVar12) * 0.0 + fVar2;
        }
      }
      else {
        fVar3 = 1.0 - *(float *)((int)local_60d4 + iVar12);
        fVar3 = fVar3 + fVar3;
        if (1.0 < fVar3) {
          fVar3 = 1.0;
        }
        bVar7 = true;
        fVar2 = fVar2 - fVar3 * *(float *)((int)&DAT_0065586c + iVar12);
      }
      psVar24 = psVar24 + -1;
      pbVar21 = pbVar21 + -1;
      iVar12 = iVar12 + -4;
      iVar19 = iVar19 + -1;
    } while (iVar19 != 0);
    uVar16 = uVar14 - 1 & 0x80000007;
    if ((int)uVar16 < 0) {
      uVar16 = (uVar16 - 1 | 0xfffffff8) + 1;
    }
    uVar20 = uVar14 & 0x80000007;
    if ((int)uVar20 < 0) {
      uVar20 = (uVar20 - 1 | 0xfffffff8) + 1;
    }
    uVar23 = uVar14 + 5 & 0x80000007;
    if ((int)uVar23 < 0) {
      uVar23 = (uVar23 - 1 | 0xfffffff8) + 1;
    }
    uVar22 = uVar14 + 4 & 0x80000007;
    if ((int)uVar22 < 0) {
      uVar22 = (uVar22 - 1 | 0xfffffff8) + 1;
    }
    *local_60b0 = fVar2 + *local_60b0;
    local_60b8 = local_60b8 + 2;
    local_60b0 = local_60b0 + 1;
    uVar14 = uVar14 + 1;
    local_6090[(short)uVar16] = fVar2 * 0.8 + local_6090[(short)uVar16];
    local_60dc = (undefined4 *)(fVar2 * 0.5);
    local_60a8 = (float)((int)local_60a8 + -1);
    local_6090[(short)uVar20] = (float)local_60dc + local_6090[(short)uVar20];
    local_6090[(short)uVar23] = fVar2 * 0.8 + local_6090[(short)uVar23];
    local_6090[(short)uVar22] = (float)local_60dc + local_6090[(short)uVar22];
  } while (local_60a8 != 0.0);
  fVar2 = SQRT(*(float *)(local_6094 + 0x94) * *(float *)(local_6094 + 0x94) +
               *(float *)(local_6094 + 0x90) * *(float *)(local_6094 + 0x90) +
               *(float *)(local_6094 + 0x8c) * *(float *)(local_6094 + 0x8c));
  local_60a8 = 0.0;
  if (0.02 < fVar2) {
    local_60a8 = (fVar2 - 0.02) * 12.5;
    if (1.0 < local_60a8) {
      local_60a8 = 1.0;
    }
    local_60a8 = local_60a8 * 0.8;
    fVar2 = local_6018 * *(float *)(local_6094 + 0x8c) +
            local_6010 * *(float *)(local_6094 + 0x94) + local_6014 * *(float *)(local_6094 + 0x90);
    fVar3 = -(local_6024 * *(float *)(local_6094 + 0x8c) +
             local_601c * *(float *)(local_6094 + 0x94) + local_6020 * *(float *)(local_6094 + 0x90)
             );
    fVar1 = SQRT(fVar2 * fVar2 + fVar3 * fVar3);
    if (0.0001 <= ABS(fVar1)) {
      fVar5 = 1.0 / fVar1;
      if (((0.0 < fVar1) &&
          (cVar9 = FUN_00419240(8,local_6090,&local_60dc,&local_60a4), cVar9 != '\0')) &&
         (0.5 < local_60a4)) {
        pfVar18 = local_6090;
        pfVar15 = (float *)&DAT_00880544;
        iVar12 = 8;
        do {
          fVar1 = fVar2 * fVar5 * *pfVar15 + fVar5 * 0.0 * pfVar15[-1] + fVar5 * fVar3 * pfVar15[1];
          if (fVar1 < 0.0) {
            *pfVar18 = fVar1 * local_60a8 + *pfVar18;
          }
          pfVar15 = pfVar15 + 3;
          pfVar18 = pfVar18 + 1;
          iVar12 = iVar12 + -1;
        } while (iVar12 != 0);
      }
    }
  }
  puVar8 = PTR_DAT_00696714;
  sVar10 = -1;
  local_60b8 = (byte *)0xff7fffff;
  sVar13 = 0;
  pfVar18 = local_6090;
  do {
    if ((float)local_60b8 < *pfVar18) {
      local_60b8 = (byte *)*pfVar18;
      sVar10 = sVar13;
    }
    sVar13 = sVar13 + 1;
    pfVar18 = pfVar18 + 1;
  } while (sVar13 < 8);
  fVar2 = *param_2;
  fVar3 = param_2[1];
  fVar1 = param_2[2];
  local_60d8 = *(float *)PTR_DAT_00696714;
  local_60d4[0] = *(float *)(PTR_DAT_00696714 + 4);
  local_60d4[1] = *(float *)(PTR_DAT_00696714 + 8);
  local_60b4 = 1.0;
  local_60b0 = (float *)0x0;
  fVar5 = SQRT(fVar2 * fVar2 + fVar3 * fVar3 + fVar1 * fVar1);
  if (0.0001 <= ABS(fVar5)) {
    fVar6 = 1.0 / fVar5;
    fVar2 = fVar2 * fVar6;
    fVar3 = fVar3 * fVar6;
    fVar1 = fVar1 * fVar6;
    if (0.0 < fVar5) {
      local_60d8 = 0.0;
      local_60b4 = local_6030 * fVar2 + local_602c * fVar3 + local_6028 * fVar1;
      local_60d4[0] = fVar2 * local_6024 + fVar1 * local_601c + fVar3 * local_6020;
      local_60d4[1] = fVar2 * local_6018 + fVar1 * local_6010 + fVar3 * local_6014;
      fVar2 = SQRT(local_60d4[0] * local_60d4[0] + local_60d4[1] * local_60d4[1]);
      if (0.0001 <= ABS(fVar2)) {
        fVar3 = 1.0 / fVar2;
        local_60d8 = fVar3 * 0.0;
        local_60d4[0] = local_60d4[0] * fVar3;
        local_60d4[1] = fVar3 * local_60d4[1];
        if (0.0 < fVar2) {
          FUN_00419240(8,local_6090,&local_60a4,&local_60b0);
        }
      }
    }
  }
  fVar2 = local_60bc;
  local_60dc = (undefined4 *)((float)local_60b8 - (float)local_60b0);
  if (local_60ac <= 0.6) {
    local_60cc = (undefined4 *)(local_60ac * 3.3333333);
    if (1.0 <= (float)local_60cc) {
      local_60cc = (undefined4 *)0x3f800000;
    }
  }
  else {
    fVar3 = (local_60ac - 0.6) * 2.5000002;
    if (1.0 <= fVar3) {
      fVar3 = 1.0;
    }
    local_60cc = (undefined4 *)(fVar3 + 1.0);
  }
  if (-0.2 <= local_60b4) goto LAB_00419d85;
  sVar13 = *(short *)((int)local_60bc + 0x5f0);
  if ((sVar13 == -1) || (0x59 < sVar13)) {
    if (*(float *)(local_6094 + 0x94) * *(float *)(local_6094 + 0x94) +
        *(float *)(local_6094 + 0x90) * *(float *)(local_6094 + 0x90) +
        *(float *)(local_6094 + 0x8c) * *(float *)(local_6094 + 0x8c) <= 0.0025000002) {
      if ((float)local_60cc <= 0.5) goto LAB_00419d85;
      goto LAB_00419e38;
    }
    if ((2.0 < (float)local_60dc) && (2.0 < (float)local_60b8)) goto LAB_00419e38;
LAB_00419d85:
    *(undefined2 *)((int)local_60bc + 0x5f0) = 0xffff;
    puVar4 = local_60cc;
    if (local_60b4 < 0.5) {
      if ((1.3 < (float)local_60dc) &&
         (iVar12 = (int)sVar10,
         0.5 < local_60d8 * (float)(&DAT_00880540)[iVar12 * 3] +
               local_60d4[0] * (float)(&DAT_00880544)[iVar12 * 3] +
               local_60d4[1] * (float)(&DAT_00880548)[iVar12 * 3])) {
        puVar4 = (undefined4 *)((float)local_60dc * 0.7692308 - 0.5);
        if (0.0 <= (float)puVar4) {
          if (1.0 < (float)puVar4) {
            puVar4 = (undefined4 *)0x3f800000;
          }
        }
        else {
          puVar4 = (undefined4 *)0x0;
        }
        if ((float)puVar4 <= (float)local_60cc) {
          puVar4 = local_60cc;
        }
        local_60c0 = (float)puVar4 * 1.0471976;
        if (0.0 < local_60d4[1] * (float)(&DAT_00880544)[iVar12 * 3] -
                  local_60d4[0] * (float)(&DAT_00880548)[iVar12 * 3]) {
          local_60c0 = -local_60c0;
        }
        *(short *)((int)local_60bc + 0x5d8) = sVar10;
        local_60c8 = local_6030 * local_60c0;
        local_60c4 = local_602c * local_60c0;
        local_60c0 = local_6028 * local_60c0;
        goto LAB_0041a299;
      }
LAB_0041a288:
      *(undefined2 *)((int)local_60bc + 0x5d8) = 0xffff;
      puVar4 = (undefined4 *)local_6090[8];
      goto LAB_0041a299;
    }
    if (local_60ac <= 0.0) goto LAB_0041a288;
    fVar3 = (float)(&DAT_00880544)[sVar10 * 3];
    fVar1 = -(float)(&DAT_00880548)[sVar10 * 3];
    local_60c8 = fVar3 * local_6018 + local_6024 * fVar1 + *(float *)puVar8;
    local_60c4 = local_6014 * fVar3 + local_6020 * fVar1 + *(float *)(puVar8 + 4);
    local_60c0 = local_6010 * fVar3 + local_601c * fVar1 + *(float *)(puVar8 + 8);
    fVar3 = SQRT(local_60c8 * local_60c8 + local_60c4 * local_60c4 + local_60c0 * local_60c0);
    if (0.0001 <= ABS(fVar3)) {
      fVar1 = 1.0 / fVar3;
      local_60c8 = local_60c8 * fVar1;
      local_60c4 = local_60c4 * fVar1;
      local_60c0 = local_60c0 * fVar1;
      if (0.0 < fVar3) {
        fVar3 = (float)local_60cc * 1.0471976;
        local_60c8 = local_60c8 * fVar3;
        local_60c4 = local_60c4 * fVar3;
        local_60c0 = local_60c0 * fVar3;
      }
    }
  }
  else {
LAB_00419e38:
    if (sVar13 == -1) {
      *(undefined2 *)((int)local_60bc + 0x5f0) = 0;
    }
    else {
      *(short *)((int)local_60bc + 0x5f0) = sVar13 + 1;
    }
    iVar12 = (int)sVar10;
    fVar3 = (float)(&DAT_00880540)[iVar12 * 3];
    fVar1 = (float)(&DAT_00880544)[iVar12 * 3];
    local_60bc = (float)(&DAT_00880548)[iVar12 * 3];
    fVar5 = local_60bc * local_6018 + fVar1 * local_6024 + local_6030 * fVar3 + *(float *)puVar8;
    fVar6 = local_6014 * local_60bc +
            local_6020 * fVar1 + local_602c * fVar3 + *(float *)(puVar8 + 4);
    fVar3 = local_6010 * local_60bc +
            local_601c * fVar1 + local_6028 * fVar3 + *(float *)(puVar8 + 8);
    local_60a0 = fVar3 * param_2[1] - fVar6 * param_2[2];
    local_609c = fVar5 * param_2[2] - fVar3 * *param_2;
    local_6098 = fVar6 * *param_2 - fVar5 * param_2[1];
    fVar3 = SQRT(local_60a0 * local_60a0 + local_609c * local_609c + local_6098 * local_6098);
    if (0.0001 <= ABS(fVar3)) {
      local_60d4[1] = 1.0 / fVar3;
      local_60d8 = local_60a0 * local_60d4[1];
      local_60d4[0] = local_609c * local_60d4[1];
      local_60d4[1] = local_6098 * local_60d4[1];
      if (0.0 < fVar3) {
        fVar25 = (float10)vector3d_angle_between_4cd4f0();
        local_60c8 = (float)((float10)local_60d8 * fVar25);
        local_60c4 = (float)((float10)local_60d4[0] * fVar25);
        local_60c0 = (float)((float10)local_60d4[1] * fVar25);
      }
    }
    puVar4 = (undefined4 *)((2.0 - (float)local_60b0) * 0.5 - 0.5);
    if (0.0 <= (float)puVar4) {
      if (1.0 < (float)puVar4) {
        puVar4 = (undefined4 *)0x3f800000;
      }
    }
    else {
      puVar4 = (undefined4 *)0x0;
    }
    if ((float)puVar4 <= (float)local_60cc) {
      *(short *)((int)fVar2 + 0x5d8) = sVar10;
      puVar4 = local_60cc;
      goto LAB_0041a299;
    }
  }
  *(short *)((int)fVar2 + 0x5d8) = sVar10;
LAB_0041a299:
  *param_3 = local_60c8;
  param_3[1] = local_60c4;
  param_3[2] = local_60c0;
  *param_4 = (float)puVar4;
  return;
}
#endif
