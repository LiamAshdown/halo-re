// object_recalculate_bounding_radius  (Ghidra: object_recalculate_bounding_radius, already
// named)
// address 0x4f8310, size 485 bytes
// name confidence: 0.85 (already carries this name from an earlier phase; matches
//   functions.md's summary: "Recomputes an object's world node transforms and derives its
//   current bounding radius from them")
// rewrite confidence: 0.15 (this is the single most complex function in the module: a full
//   skeletal animation evaluation -- default node transforms, per-node animation blending with
//   quaternion interpolation, and a node-tree walk composing matrix4x3 transforms -- built from
//   ModelAnimation/ModelNode tag layouts that no other function in this module establishes.
//   Several callees also return extra values through "extraout_ECX"-style registers Ghidra
//   could not resolve to real parameters. Given the time available, this rewrite is a close,
//   MECHANICAL transliteration of the decompiled C rather than a field-by-field clean rewrite:
//   local variable names, raw offsets and even the array-index register (asStack_210) are kept
//   close to the original so the control flow and arithmetic stay verifiably unchanged. Treat
//   every offset not already established elsewhere in this module as UNSURE.)
// evidence: types/objects.h object (nodes 0x1f0, node_function_values 0x1e8,
//   node_function_count 0x0d6, unknown_0d4 0x0d4, bounding_radius 0x0ac, scale 0x0b0,
//   type 0x0b4, parent_object 0x11c, parent_marker_index 0x120, forward 0x074, up 0x080,
//   position 0x05c, flags 0x10 with _object_mirrored_geometry_bit,
//   _object_mask_no_node_functions == 0xfe0); types/tags.h Object.animation_graph; global
//   0x008603b0 object_data, 0x0087bc14 tag_instances, 0x006f1d6c game_time (tick at
//   +0xc), 0x00696664 matrix4x3_multiply_procedure.
// register convention: object index is the sole, genuinely-stack, parameter (Ghidra's own
//   "object_recalculate_bounding_radius(uint param_1)").

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"

extern data_array *object_data; // 0x008603b0
extern tag_instance *tag_instances; // 0x0087bc14
extern uint8_t *game_time; // 0x006f1d6c, tick count at +0xc
extern void (*matrix4x3_multiply_procedure)(); // 0x00696664. src/math/math_initialize.c
    // declares it (real_matrix4x3 *a, real_matrix4x3 *b, real_matrix4x3 *out); the three-argument
    // call sites below pass raw byte buffers and node-array slices, so no prototype is asserted
    // here rather than casting every operand.

extern void matrix4x3_from_forward_up(); // 0x4cb970. src/math and the other four objects
    // files declare it (real_vector3d *up, real_vector3d *forward, real_matrix4x3 *out); this
    // call site shows only the third operand, so no prototype is asserted.
extern void matrix4x3_transform_point(); // math module, 0x4cbde0.
    // No prototype is asserted: Ghidra models fewer or differently-typed arguments here than
    // the other call site(s) of the same address, because the missing operands travel in
    // registers it could not source. The empty parameter list is the convention this module
    // already uses for FUN_00450870 -- one declaration per symbol, no invented signature.
extern void vector3d_cross_product(); // math module, 0x4052c0.
    // No prototype is asserted: Ghidra models fewer or differently-typed arguments here than
    // the other call site(s) of the same address, because the missing operands travel in
    // registers it could not source. The empty parameter list is the convention this module
    // already uses for FUN_00450870 -- one declaration per symbol, no invented signature.
extern void model_nodes_get_default_transforms(void *out_nodes); // 0x4d7610, UNSURE
extern void animation_get_frame_orientations(uint32_t selector, void *out_nodes); // 0x4d4a80, UNSURE
extern void model_vertices_get_interpolated_frame(float t, void *out_nodes); // UNSURE: address not captured in this batch's pack
extern void FUN_004d51a0(uint32_t selector, float t, void *out_nodes); // UNSURE
extern void object_type_definitions_notify_two_args_0x48(uint32_t object_index, void *nodes); // 0x4f4250, outside this batch
extern void *matrix4x3_from_quaternion(void); // 0x4cbad0, UNSURE: return/extraout shape guessed, see file header
extern void model_nodes_blend_transforms(void *values, int16_t unknown_0d4, int16_t node_function_count); // UNSURE

void object_recalculate_bounding_radius(uint32_t object_index)
{
    uint32_t *puVar3 = (uint32_t *)((object_header *)object_data->data)[object_index & 0xffff].data;
    int32_t iVar12 = (int32_t)tag_instances[*puVar3 & 0xffff].data;
    uint32_t *puVar8 = (uint32_t *)((int32_t)*(int16_t *)((uint8_t *)puVar3 + 0x1f2) + (int32_t)puVar3);
    uint8_t local_a10[2048];
    uint8_t *local_1c;
    int32_t local_20;
    uint32_t *local_18;

    if (((1 << (((uint8_t *)puVar3)[0x2d * 4] & 0x1f)) & 0xfe0u) == 0) { // UNSURE: byte read at object+0xb4, see object.type
        local_1c = (uint8_t *)((int32_t)*(int16_t *)((uint8_t *)puVar3 + 0x1ee) + (int32_t)puVar3);
    } else {
        local_1c = local_a10;
    }
    local_20 = iVar12;
    local_18 = puVar8;

    if (*(uint32_t *)(iVar12 + 0x34) == 0xffffffff) {
        *puVar8 = 0x3f800000;
        puVar8[1] = puVar3[0x1d];
        puVar8[2] = puVar3[0x1e];
        puVar8[3] = puVar3[0x1f];
        puVar8[7] = puVar3[0x20];
        puVar8[8] = puVar3[0x21];
        puVar8[9] = puVar3[0x22];
        vector3d_cross_product(puVar8 + 7);
        puVar8[10] = puVar3[0x17];
        puVar8[0xb] = puVar3[0x18];
        puVar8[0xc] = puVar3[0x19];
    } else {
        int32_t local_2c = (int32_t)tag_instances[*(uint32_t *)(iVar12 + 0x34) & 0xffff].data;
        float *local_c;
        float *pfVar10;
        uint8_t local_11 = 0;
        int16_t local_24;
        int16_t asStack_210[64];
        float fStack_10;

        if (puVar3[0x47] == 0xffffffff) {
            local_c = (float *)0;
        } else {
            int32_t parent_hdr = (int32_t)((object_header *)object_data->data)[puVar3[0x47] & 0xffff].data;
            local_c = (float *)((int32_t)*(int16_t *)(parent_hdr + 0x1f2) + (int8_t)puVar3[0x48] * 0x34 + parent_hdr);
        }
        pfVar10 = local_c;

        if ((puVar3[0x33] == 0xffffffff) || ((int16_t)puVar3[0x34] == -1)) {
            model_nodes_get_default_transforms(local_1c);
        } else {
            int32_t anim = (int16_t)puVar3[0x34] * 0xb4 +
                *(int32_t *)((int32_t)tag_instances[puVar3[0x33] & 0xffff].data + 0x78);
            uint32_t uVar7;
            if (((int8_t)puVar3[4] < 0) && (*(int16_t *)(anim + 0x22) > 0)) {
                uVar7 = (*(int32_t *)(game_time + 0xc) + object_index) % (uint32_t)*(int16_t *)(anim + 0x22);
            } else {
                uVar7 = *(uint16_t *)((uint8_t *)puVar3 + 0xd2);
            }
            animation_get_frame_orientations(uVar7, local_1c);
            local_11 = (*(uint8_t *)(anim + 0x3a) >> 1) & 1;
            pfVar10 = local_c;
        }

        if (*(uint32_t *)(local_20 + 0x44) != 0xffffffff) {
            int32_t *piVar9 = *(int32_t **)((int32_t)tag_instances[*(uint32_t *)(local_20 + 0x44) & 0xffff].data);
            int32_t *local_30 = piVar9;
            int32_t idx = 0;
            local_24 = 0;
            if (0 < *piVar9) {
                do {
                    int16_t *psVar1 = (int16_t *)(piVar9[1] + idx * 0x14);
                    if ((*psVar1 != -1) && (piVar9 = local_30, (int32_t)psVar1[1] < *(int32_t *)(local_20 + 0x158))) {
                        int32_t iVar12b = *psVar1 * 0xb4 + local_30[0x1e];
                        int32_t iStack_28;
                        fStack_10 = (float)puVar3[psVar1[1] + 0x4d];
                        if (psVar1[2] == 0) {
                            if ((*(uint8_t *)(psVar1[1] * 0x168 + *(int32_t *)(local_20 + 0x15c)) & 2) == 0) {
                                iStack_28 = *(int16_t *)(iVar12b + 0x22) - 1;
                            } else {
                                iStack_28 = (int32_t)*(int16_t *)(iVar12b + 0x22);
                            }
                            fStack_10 = (float)iStack_28 * fStack_10;
                            model_vertices_get_interpolated_frame(fStack_10, local_1c);
                            pfVar10 = local_c;
                        } else {
                            pfVar10 = local_c;
                            if (psVar1[2] == 1) {
                                FUN_004d51a0((*(int32_t *)(game_time + 0xc) + object_index) %
                                             (uint32_t)*(int16_t *)(iVar12b + 0x22), fStack_10, local_1c);
                                pfVar10 = local_c;
                            }
                        }
                    }
                    local_24 = local_24 + 1;
                    idx = (int32_t)local_24;
                } while (idx < *piVar9);
            }
        }

        {
            uint8_t *puVar4 = local_1c;
            if (0.0f < (float)puVar3[0x2c]) {
                *(float *)(local_1c + 0x1c) = (float)puVar3[0x2c] * *(float *)(local_1c + 0x1c);
                *(float *)(puVar4 + 0x40) = (float)puVar3[0x2c] * *(float *)(puVar4 + 0x40); // puVar4[0x10] as uint32*
                *(float *)(puVar4 + 0x50) = (float)puVar3[0x2c] * *(float *)(puVar4 + 0x50); // puVar4[0x14]
                *(float *)(puVar4 + 0x60) = (float)puVar3[0x2c] * *(float *)(puVar4 + 0x60); // puVar4[0x18]
            }
            if (*(int32_t *)(local_20 + 0x44) != -1) {
                object_type_definitions_notify_two_args_0x48(object_index, puVar4);
            }
            if (0 < *(int16_t *)((uint8_t *)puVar3 + 0xd6)) {
                model_nodes_blend_transforms((uint8_t *)((int32_t)*(int16_t *)((uint8_t *)puVar3 + 0x1ea) + (int32_t)puVar3),
                    (int16_t)puVar3[0x35], *(int16_t *)((uint8_t *)puVar3 + 0xd6));
            }
        }

        // From here on, fStack_10's bit pattern is reused as a plain int16 node-queue write
        // index rather than an actual float (1.4013e-45f is the float whose raw bits are the
        // integer 1); a small helper makes that reinterpretation explicit instead of relying on
        // (float)(int) round-tripping, which would silently truncate to 0 and change behaviour.
        local_24 = 0;
        {
            uint32_t fStack_10_bits = 1;
            fStack_10 = *(float *)&fStack_10_bits;
        }
        asStack_210[0] = 0;
        do {
            int16_t sVar5 = local_24;
            int32_t iVar12c;
            local_24 = local_24 + 1;
            iVar12c = asStack_210[sVar5] * 0x9c + *(int32_t *)(local_2c + 0xbc);
            if (asStack_210[sVar5] == 0) {
                uint32_t auStack_158[10];
                uint32_t uStack_130, uStack_12c, uStack_128;
                uint8_t auStack_120[16];
                float fStack_110, fStack_10c, fStack_108;
                void *extraout_ECX = matrix4x3_from_quaternion();
                auStack_158[0] = *(uint32_t *)((uint8_t *)extraout_ECX + 0x1c);
                uStack_130 = *(uint32_t *)((uint8_t *)extraout_ECX + 0x10);
                uStack_12c = *(uint32_t *)((uint8_t *)extraout_ECX + 0x14);
                uStack_128 = *(uint32_t *)((uint8_t *)extraout_ECX + 0x18);

                if (local_11 == 0) {
                    float fStack_50 = (float)puVar3[0x17];
                    float fStack_4c = (float)puVar3[0x18];
                    float fStack_48 = (float)puVar3[0x19];
                    uint32_t uStack_78 = 0x3f800000, uStack_74 = 0x3f800000, uStack_70 = 0, uStack_6c = 0;
                    uint32_t uStack_68 = 0, uStack_64 = 0x3f800000, uStack_60 = 0, uStack_5c = 0;
                    uint32_t uStack_58 = 0, uStack_54 = 0x3f800000;

                    matrix4x3_from_forward_up(0, 0, auStack_120); // UNSURE: implicit up/forward inputs, see file header (matches FUN_004cb970's own two register operands, not resolved at this call site)

                    if ((puVar3[4] & 0x1000) != 0) {
                        fStack_110 = -fStack_110;
                        fStack_10c = -fStack_10c;
                        fStack_108 = -fStack_108;
                    }
                    if (*(uint32_t *)(local_20 + 0x8c) != 0xffffffff) {
                        int32_t iVar6 = (int32_t)tag_instances[*(uint32_t *)(local_20 + 0x8c) & 0xffff].data;
                        float fStack_c0 = -*(float *)(iVar6 + 0xc);
                        float fStack_bc = -*(float *)(iVar6 + 0x10);
                        float fStack_b8 = -*(float *)(iVar6 + 0x14);
                        uint32_t uStack_e8 = 0x3f800000, uStack_e4 = 0x3f800000, uStack_e0 = 0, uStack_dc = 0;
                        uint32_t uStack_d8 = 0, uStack_d4 = 0x3f800000, uStack_d0 = 0, uStack_cc = 0;
                        uint32_t uStack_c8 = 0, uStack_c4 = 0x3f800000;
                        float fStack_3c = fStack_c0, fStack_38 = fStack_bc, fStack_34 = fStack_b8;
                        uint32_t marker_block[13];
                        marker_block[0] = uStack_e8; marker_block[1] = uStack_e4; marker_block[2] = uStack_e0;
                        marker_block[3] = uStack_dc; marker_block[4] = uStack_d8; marker_block[5] = uStack_d4;
                        marker_block[6] = uStack_d0; marker_block[7] = uStack_cc; marker_block[8] = uStack_c8;
                        marker_block[9] = uStack_c4;
                        *(float *)&marker_block[10] = fStack_3c; *(float *)&marker_block[11] = fStack_38; *(float *)&marker_block[12] = fStack_34;
                        matrix4x3_multiply_procedure(auStack_120, marker_block, auStack_120);
                    }

                    {
                        uint32_t uStack_b0 = 0x3f800000, uStack_ac = 0x3f800000, uStack_a8 = 0, uStack_a4 = 0;
                        uint32_t uStack_a0 = 0, uStack_9c = 0x3f800000, uStack_98 = 0, uStack_94 = 0;
                        uint32_t uStack_90 = 0, uStack_8c = 0x3f800000;
                        uint32_t uStack_88 = *(uint32_t *)(local_20 + 0x14);
                        uint32_t uStack_84 = *(uint32_t *)(local_20 + 0x18);
                        uint32_t uStack_80 = *(uint32_t *)(local_20 + 0x1c);
                        uint32_t offset_block[13];
                        offset_block[0] = uStack_b0; offset_block[1] = uStack_ac; offset_block[2] = uStack_a8;
                        offset_block[3] = uStack_a4; offset_block[4] = uStack_a0; offset_block[5] = uStack_9c;
                        offset_block[6] = uStack_98; offset_block[7] = uStack_94; offset_block[8] = uStack_90;
                        offset_block[9] = uStack_8c; offset_block[10] = uStack_88; offset_block[11] = uStack_84;
                        offset_block[12] = uStack_80;
                        matrix4x3_multiply_procedure(auStack_120, offset_block, auStack_120);
                    }

                    if (pfVar10 == (float *)0) {
                        uint32_t forward_block[13];
                        forward_block[0] = uStack_78; forward_block[1] = uStack_74; forward_block[2] = uStack_70;
                        forward_block[3] = uStack_6c; forward_block[4] = uStack_68; forward_block[5] = uStack_64;
                        forward_block[6] = uStack_60; forward_block[7] = uStack_5c; forward_block[8] = uStack_58;
                        forward_block[9] = uStack_54; forward_block[10] = *(uint32_t *)&fStack_50;
                        forward_block[11] = *(uint32_t *)&fStack_4c; forward_block[12] = *(uint32_t *)&fStack_48;
                        matrix4x3_multiply_procedure(forward_block, auStack_120, local_18);
                        matrix4x3_multiply_procedure(local_18, auStack_158, local_18);
                    } else {
                        float afStack_190[14];
                        float *pfVar11 = pfVar10;
                        if (*pfVar10 != 1.0f) {
                            int32_t i;
                            float fVar2 = *pfVar10;
                            pfVar11 = afStack_190;
                            local_c = pfVar11;
                            fStack_50 = fStack_50 * *pfVar10;
                            fStack_4c = fStack_4c * *pfVar10;
                            for (i = 0xd; i != 0; i--) {
                                *pfVar11 = *pfVar10; // UNSURE: preserved literally, though this mirrors pfVar10 into itself in the decompile's own shape
                                pfVar10++;
                            }
                            fStack_48 = fStack_48 * fVar2;
                            afStack_190[0] = 1.0f;
                        }
                        if ((*(uint32_t *)((int32_t)((object_header *)object_data->data)[puVar3[0x47] & 0xffff].data + 0x10) & 0x1000) != 0) {
                            if (pfVar11 != afStack_190) {
                                int32_t i;
                                float *src = pfVar11;
                                float *dst = afStack_190;
                                for (i = 0xd; i != 0; i--) {
                                    *dst = *src;
                                    src++; dst++;
                                }
                                local_c = afStack_190;
                                pfVar11 = afStack_190;
                            }
                            pfVar11[4] = -pfVar11[4];
                            pfVar11[5] = -pfVar11[5];
                            pfVar11[6] = -pfVar11[6];
                        }
                        {
                            uint32_t forward_block[13];
                            forward_block[0] = uStack_78; forward_block[1] = uStack_74; forward_block[2] = uStack_70;
                            forward_block[3] = uStack_6c; forward_block[4] = uStack_68; forward_block[5] = uStack_64;
                            forward_block[6] = uStack_60; forward_block[7] = uStack_5c; forward_block[8] = uStack_58;
                            forward_block[9] = uStack_54; forward_block[10] = *(uint32_t *)&fStack_50;
                            forward_block[11] = *(uint32_t *)&fStack_4c; forward_block[12] = *(uint32_t *)&fStack_48;
                            matrix4x3_multiply_procedure(pfVar11, forward_block, local_18);
                        }
                        matrix4x3_multiply_procedure(local_18, auStack_120, local_18);
                        matrix4x3_multiply_procedure(local_18, auStack_158, local_18);
                        pfVar10 = pfVar11;
                    }
                } else {
                    int32_t i;
                    uint32_t *src = auStack_158;
                    uint32_t *dst = local_18;
                    for (i = 0xd; i != 0; i--) {
                        *dst = *src;
                        src++; dst++;
                    }
                    pfVar10 = local_c;
                }
            } else {
                void *extraout_ECX_00 = matrix4x3_from_quaternion();
                uint32_t extraout_EDX[13]; // UNSURE: Ghidra shows this as a separate register-returned pointer; approximated as a local scratch matrix, see file header
                extraout_EDX[0] = *(uint32_t *)((uint8_t *)extraout_ECX_00 + 0x1c);
                extraout_EDX[10] = *(uint32_t *)((uint8_t *)extraout_ECX_00 + 0x10);
                extraout_EDX[11] = *(uint32_t *)((uint8_t *)extraout_ECX_00 + 0x14);
                extraout_EDX[12] = *(uint32_t *)((uint8_t *)extraout_ECX_00 + 0x18);
                matrix4x3_multiply_procedure((uint8_t *)local_18 + *(int16_t *)(iVar12c + 0x24) * 0x34,
                    extraout_EDX, extraout_EDX);
            }

            if (*(int16_t *)(iVar12c + 0x20) != -1) {
                asStack_210[*(uint32_t *)&fStack_10] = *(int16_t *)(iVar12c + 0x20);
                *(uint32_t *)&fStack_10 = *(uint32_t *)&fStack_10 + 1;
            }
            if (*(int16_t *)(iVar12c + 0x22) != -1) {
                asStack_210[*(uint32_t *)&fStack_10] = *(int16_t *)(iVar12c + 0x22);
                *(uint32_t *)&fStack_10 = *(uint32_t *)&fStack_10 + 1;
            }
            puVar8 = local_18;
            iVar12 = local_20;
        } while (local_24 != (int16_t)*(uint32_t *)&fStack_10);
    }

    matrix4x3_transform_point(puVar8);
    {
        float fVar2 = *(float *)(iVar12 + 4);
        puVar3[0x2b] = *(uint32_t *)&fVar2;
        if ((float)puVar3[0x2c] > 0.0f) {
            fVar2 = fVar2 * (float)puVar3[0x2c];
            puVar3[0x2b] = *(uint32_t *)&fVar2;
        }
    }
}

#if 0
Original Ghidra decompilation (0x4f8310):

void object_recalculate_bounding_radius(uint param_1)

{
  short *psVar1;
  float fVar2;
  uint *puVar3;
  undefined1 *puVar4;
  short sVar5;
  int extraout_ECX;
  int iVar6;
  int extraout_ECX_00;
  uint uVar7;
  undefined4 *extraout_EDX;
  undefined4 *puVar8;
  int *piVar9;
  float *pfVar10;
  float *pfVar11;
  int iVar12;
  float *pfVar13;
  undefined4 *puVar14;
  undefined1 local_a10 [2048];
  short asStack_210 [64];
  float afStack_190 [14];
  undefined4 auStack_158 [10];
  undefined4 uStack_130;
  undefined4 uStack_12c;
  undefined4 uStack_128;
  undefined1 auStack_120 [16];
  float fStack_110;
  float fStack_10c;
  float fStack_108;
  undefined4 uStack_e8;
  undefined4 uStack_e4;
  undefined4 uStack_e0;
  undefined4 uStack_dc;
  undefined4 uStack_d8;
  undefined4 uStack_d4;
  undefined4 uStack_d0;
  undefined4 uStack_cc;
  undefined4 uStack_c8;
  undefined4 uStack_c4;
  float fStack_c0;
  float fStack_bc;
  float fStack_b8;
  undefined4 uStack_b0;
  undefined4 uStack_ac;
  undefined4 uStack_a8;
  undefined4 uStack_a4;
  undefined4 uStack_a0;
  undefined4 uStack_9c;
  undefined4 uStack_98;
  undefined4 uStack_94;
  undefined4 uStack_90;
  undefined4 uStack_8c;
  undefined4 uStack_88;
  undefined4 uStack_84;
  undefined4 uStack_80;
  undefined4 uStack_78;
  undefined4 uStack_74;
  undefined4 uStack_70;
  undefined4 uStack_6c;
  undefined4 uStack_68;
  undefined4 uStack_64;
  undefined4 uStack_60;
  undefined4 uStack_5c;
  undefined4 uStack_58;
  undefined4 uStack_54;
  float fStack_50;
  float fStack_4c;
  float fStack_48;
  float fStack_3c;
  float fStack_38;
  float fStack_34;
  int *local_30;
  int local_2c;
  int iStack_28;
  int local_24;
  int local_20;
  undefined1 *local_1c;
  undefined4 *local_18;
  byte local_11;
  float fStack_10;
  float *local_c;

  puVar3 = *(uint **)(*(int *)(DAT_008603b0 + 0x34) + 8 + (param_1 & 0xffff) * 0xc);
  iVar12 = *(int *)((*puVar3 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
  puVar8 = (undefined4 *)((int)*(short *)((int)puVar3 + 0x1f2) + (int)puVar3);
  if ((1 << ((byte)puVar3[0x2d] & 0x1f) & 0xfe0U) == 0) {
    local_1c = (undefined1 *)((int)*(short *)((int)puVar3 + 0x1ee) + (int)puVar3);
  }
  else {
    local_1c = local_a10;
  }
  local_20 = iVar12;
  local_18 = puVar8;
  if (*(uint *)(iVar12 + 0x34) == 0xffffffff) {
    *puVar8 = 0x3f800000;
    puVar8[1] = puVar3[0x1d];
    puVar8[2] = puVar3[0x1e];
    puVar8[3] = puVar3[0x1f];
    puVar8[7] = puVar3[0x20];
    puVar8[8] = puVar3[0x21];
    puVar8[9] = puVar3[0x22];
    vector3d_cross_product(puVar8 + 7);
    puVar8[10] = puVar3[0x17];
    puVar8[0xb] = puVar3[0x18];
    puVar8[0xc] = puVar3[0x19];
  }
  else {
    local_2c = *(int *)((*(uint *)(iVar12 + 0x34) & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
    if (puVar3[0x47] == 0xffffffff) {
      local_c = (float *)0x0;
    }
    else {
      iVar12 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (puVar3[0x47] & 0xffff) * 0xc);
      local_c = (float *)((int)*(short *)(iVar12 + 0x1f2) + (char)puVar3[0x48] * 0x34 + iVar12);
    }
    pfVar10 = local_c;
    local_11 = 0;
    if ((puVar3[0x33] == 0xffffffff) || ((short)puVar3[0x34] == -1)) {
      model_nodes_get_default_transforms(local_1c);
    }
    else {
      iVar12 = (short)puVar3[0x34] * 0xb4 +
               *(int *)(*(int *)((puVar3[0x33] & 0xffff) * 0x20 + 0x14 + DAT_0087bc14) + 0x78);
      if (((char)puVar3[4] < '\0') && (sVar5 = *(short *)(iVar12 + 0x22), 0 < sVar5)) {
        uVar7 = (*(int *)(DAT_006f1d6c + 0xc) + param_1) % (uint)(int)sVar5;
      }
      else {
        uVar7 = (uint)*(ushort *)((int)puVar3 + 0xd2);
      }
      FUN_004d4a80(uVar7,local_1c);
      local_11 = *(byte *)(iVar12 + 0x3a) >> 1 & 1;
      pfVar10 = local_c;
    }
    if (*(uint *)(local_20 + 0x44) != 0xffffffff) {
      piVar9 = *(int **)((*(uint *)(local_20 + 0x44) & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
      iVar12 = 0;
      local_30 = piVar9;
      local_24 = 0;
      if (0 < *piVar9) {
        do {
          psVar1 = (short *)(piVar9[1] + iVar12 * 0x14);
          if ((*psVar1 != -1) && (piVar9 = local_30, (int)psVar1[1] < *(int *)(local_20 + 0x158))) {
            iVar12 = *psVar1 * 0xb4 + local_30[0x1e];
            fStack_10 = (float)puVar3[psVar1[1] + 0x4d];
            if (psVar1[2] == 0) {
              if ((*(byte *)(psVar1[1] * 0x168 + *(int *)(local_20 + 0x15c)) & 2) == 0) {
                iStack_28 = *(short *)(iVar12 + 0x22) + -1;
              }
              else {
                iStack_28 = (int)*(short *)(iVar12 + 0x22);
              }
              fStack_10 = (float)iStack_28 * fStack_10;
              model_vertices_get_interpolated_frame(fStack_10,local_1c);
              pfVar10 = local_c;
            }
            else {
              pfVar10 = local_c;
              if (psVar1[2] == 1) {
                FUN_004d51a0((*(int *)(DAT_006f1d6c + 0xc) + param_1) %
                             (uint)(int)*(short *)(iVar12 + 0x22),fStack_10,local_1c);
                pfVar10 = local_c;
              }
            }
          }
          local_24 = local_24 + 1;
          iVar12 = (int)(short)local_24;
        } while (iVar12 < *piVar9);
      }
    }
    puVar4 = local_1c;
    if (0.0 < (float)puVar3[0x2c]) {
      *(float *)(local_1c + 0x1c) = (float)puVar3[0x2c] * *(float *)(local_1c + 0x1c);
      *(float *)(puVar4 + 0x10) = (float)puVar3[0x2c] * *(float *)(puVar4 + 0x10);
      *(float *)(puVar4 + 0x14) = (float)puVar3[0x2c] * *(float *)(puVar4 + 0x14);
      *(float *)(puVar4 + 0x18) = (float)puVar3[0x2c] * *(float *)(puVar4 + 0x18);
    }
    if (*(int *)(local_20 + 0x44) != -1) {
      FUN_004f4250(param_1,puVar4);
    }
    if (0 < *(short *)((int)puVar3 + 0xd6)) {
      model_nodes_blend_transforms
                ((int)*(short *)((int)puVar3 + 0x1ea) + (int)puVar3,(short)puVar3[0x35],
                 *(short *)((int)puVar3 + 0xd6));
    }
    local_24 = 0;
    fStack_10 = 1.4013e-45;
    asStack_210[0] = 0;
    do {
      sVar5 = (short)local_24;
      local_24 = local_24 + 1;
      iVar12 = asStack_210[sVar5] * 0x9c + *(int *)(local_2c + 0xbc);
      if (asStack_210[sVar5] == 0) {
        matrix4x3_from_quaternion();
        auStack_158[0] = *(undefined4 *)(extraout_ECX + 0x1c);
        uStack_130 = *(undefined4 *)(extraout_ECX + 0x10);
        uStack_12c = *(undefined4 *)(extraout_ECX + 0x14);
        uStack_128 = *(undefined4 *)(extraout_ECX + 0x18);
        if (local_11 == 0) {
          fStack_50 = (float)puVar3[0x17];
          fStack_4c = (float)puVar3[0x18];
          fStack_48 = (float)puVar3[0x19];
          uStack_78 = 0x3f800000;
          uStack_74 = 0x3f800000;
          uStack_70 = 0;
          uStack_6c = 0;
          uStack_68 = 0;
          uStack_64 = 0x3f800000;
          uStack_60 = 0;
          uStack_5c = 0;
          uStack_58 = 0;
          uStack_54 = 0x3f800000;
          FUN_004cb970(auStack_120);
          if ((puVar3[4] & 0x1000) != 0) {
            fStack_110 = -fStack_110;
            fStack_10c = -fStack_10c;
            fStack_108 = -fStack_108;
          }
          if (*(uint *)(local_20 + 0x8c) != 0xffffffff) {
            iVar6 = *(int *)((*(uint *)(local_20 + 0x8c) & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
            fStack_c0 = -*(float *)(iVar6 + 0xc);
            fStack_bc = -*(float *)(iVar6 + 0x10);
            fStack_b8 = -*(float *)(iVar6 + 0x14);
            uStack_e8 = 0x3f800000;
            uStack_e4 = 0x3f800000;
            uStack_e0 = 0;
            uStack_dc = 0;
            uStack_d8 = 0;
            uStack_d4 = 0x3f800000;
            uStack_d0 = 0;
            uStack_cc = 0;
            uStack_c8 = 0;
            uStack_c4 = 0x3f800000;
            fStack_3c = fStack_c0;
            fStack_38 = fStack_bc;
            fStack_34 = fStack_b8;
            (*(code *)PTR_matrix4x3_multiply_00696664)(auStack_120,&uStack_e8,auStack_120);
          }
          uStack_88 = *(undefined4 *)(local_20 + 0x14);
          uStack_84 = *(undefined4 *)(local_20 + 0x18);
          uStack_80 = *(undefined4 *)(local_20 + 0x1c);
          uStack_b0 = 0x3f800000;
          uStack_ac = 0x3f800000;
          uStack_a8 = 0;
          uStack_a4 = 0;
          uStack_a0 = 0;
          uStack_9c = 0x3f800000;
          uStack_98 = 0;
          uStack_94 = 0;
          uStack_90 = 0;
          uStack_8c = 0x3f800000;
          (*(code *)PTR_matrix4x3_multiply_00696664)(auStack_120,&uStack_b0,auStack_120);
          if (pfVar10 == (float *)0x0) {
            (*(code *)PTR_matrix4x3_multiply_00696664)(&uStack_78,auStack_120,local_18);
            (*(code *)PTR_matrix4x3_multiply_00696664)(local_18,auStack_158,local_18);
          }
          else {
            pfVar11 = pfVar10;
            if (*pfVar10 != 1.0) {
              pfVar11 = afStack_190;
              local_c = pfVar11;
              fStack_50 = fStack_50 * *pfVar10;
              fStack_4c = fStack_4c * *pfVar10;
              fVar2 = *pfVar10;
              pfVar13 = afStack_190;
              for (iVar6 = 0xd; iVar6 != 0; iVar6 = iVar6 + -1) {
                *pfVar13 = *pfVar10;
                pfVar10 = pfVar10 + 1;
                pfVar13 = pfVar13 + 1;
              }
              fStack_48 = fStack_48 * fVar2;
              afStack_190[0] = 1.0;
            }
            if ((*(uint *)(*(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 +
                                   (puVar3[0x47] & 0xffff) * 0xc) + 0x10) & 0x1000) != 0) {
              if (pfVar11 != afStack_190) {
                pfVar10 = afStack_190;
                for (iVar6 = 0xd; iVar6 != 0; iVar6 = iVar6 + -1) {
                  *pfVar10 = *pfVar11;
                  pfVar11 = pfVar11 + 1;
                  pfVar10 = pfVar10 + 1;
                }
                local_c = afStack_190;
                pfVar11 = afStack_190;
              }
              pfVar11[4] = -pfVar11[4];
              pfVar11[5] = -pfVar11[5];
              pfVar11[6] = -pfVar11[6];
            }
            (*(code *)PTR_matrix4x3_multiply_00696664)(pfVar11,&uStack_78,local_18);
            (*(code *)PTR_matrix4x3_multiply_00696664)(local_18,auStack_120,local_18);
            (*(code *)PTR_matrix4x3_multiply_00696664)(local_18,auStack_158,local_18);
            pfVar10 = pfVar11;
          }
        }
        else {
          puVar8 = auStack_158;
          puVar14 = local_18;
          for (iVar6 = 0xd; pfVar10 = local_c, iVar6 != 0; iVar6 = iVar6 + -1) {
            *puVar14 = *puVar8;
            puVar8 = puVar8 + 1;
            puVar14 = puVar14 + 1;
          }
        }
      }
      else {
        matrix4x3_from_quaternion();
        *extraout_EDX = *(undefined4 *)(extraout_ECX_00 + 0x1c);
        extraout_EDX[10] = *(undefined4 *)(extraout_ECX_00 + 0x10);
        extraout_EDX[0xb] = *(undefined4 *)(extraout_ECX_00 + 0x14);
        extraout_EDX[0xc] = *(undefined4 *)(extraout_ECX_00 + 0x18);
        (*(code *)PTR_matrix4x3_multiply_00696664)
                  (local_18 + *(short *)(iVar12 + 0x24) * 0xd,extraout_EDX,extraout_EDX);
      }
      if (*(short *)(iVar12 + 0x20) != -1) {
        asStack_210[SUB42(fStack_10,0)] = *(short *)(iVar12 + 0x20);
        fStack_10 = (float)((int)fStack_10 + 1);
      }
      if (*(short *)(iVar12 + 0x22) != -1) {
        asStack_210[SUB42(fStack_10,0)] = *(short *)(iVar12 + 0x22);
        fStack_10 = (float)((int)fStack_10 + 1);
      }
      puVar8 = local_18;
      iVar12 = local_20;
    } while ((short)local_24 != SUB42(fStack_10,0));
  }
  matrix4x3_transform_point(puVar8);
  fVar2 = *(float *)(iVar12 + 4);
  puVar3[0x2b] = (uint)fVar2;
  if ((float)puVar3[0x2c] <= 0.0) {
    return;
  }
  puVar3[0x2b] = (uint)(fVar2 * (float)puVar3[0x2c]);
  return;
}
#endif
