// object_recalculate_bounding_radius  (Ghidra: object_recalculate_bounding_radius, already
// named)
// address 0x4f8310, size 2027 bytes (0x4f8310..0x4f8afa; 0x4f84e2 / 0x4f8834 / 0x4f8a70 are its fragments)
// name confidence: 0.85 (already carries this name from an earlier phase; matches
//   functions.md's summary: "Recomputes an object's world node transforms and derives its
//   current bounding radius from them")
// rewrite confidence: 0.9 (REWRITTEN e891407 from objdump; the text below describes the replaced draft: this is the single most complex function in the module: a full
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
//   node_function_count 0x0d6, node_function_ticks_elapsed 0x0d4, bounding_radius 0x0ac, scale 0x0b0,
//   type 0x0b4, parent_object 0x11c, parent_marker_index 0x120, forward 0x074, up 0x080,
//   position 0x05c, flags 0x10 with _object_mirrored_geometry_bit,
//   _object_mask_no_node_functions == 0xfe0); types/tags.h Object.animation_graph; global
//   0x008603b0 object_data, 0x0087bc14 tag_instances, 0x006f1d6c game_time (tick at
//   +0xc), 0x00696664 matrix4x3_multiply_procedure.
// register convention: object index is the sole, genuinely-stack, parameter (Ghidra's own
//   "object_recalculate_bounding_radius(uint param_1)").
// REWRITTEN (objdump 0x4f8310..0x4f8b03, 2026-09-27): the earlier body was a confidence-0.15 transliteration of
// Ghidra with guessed helper arguments. Every call below is taken from the disassembly (addresses inline).

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"
#include "models.h"
#include "game.h"

extern data_array *object_data;          // 0x008603b0
extern tag_instance *tag_instances;      // 0x0087bc14
extern game_time_globals *game_time; // 0x006f1d6c
extern void (*matrix4x3_multiply_procedure)(real_matrix4x3 *a, real_matrix4x3 *b, real_matrix4x3 *out); // 0x00696664

extern void animation_get_frame_orientations(ModelAnimationsAnimation *animation, GBXModel *model,
    int16_t frame, real_orientation *out_orientations); // 0x4d4a80, EAX model, EDI animation
extern void model_nodes_get_default_transforms(GBXModel *model, real_orientation *out); // 0x4d7610, ESI model
extern void animation_overlay_interpolated_frame_orientations(ModelAnimationsAnimation *animation, float frame,
    real_orientation *out_orientations); // 0x4d53f0, EDI animation
extern void animation_overlay_frame_orientations_weighted(ModelAnimationsAnimation *animation, int16_t frame,
    float weight, real_orientation *out_orientations); // 0x4d51a0, EDI animation
extern void object_type_definitions_notify_two_args_0x48(uint32_t object_index, uint32_t event_argument); // 0x4f4250
extern void model_nodes_blend_transforms(real_orientation *in_out, int16_t node_count, real_orientation *other,
    int16_t step, int16_t steps); // 0x4d69e0, EAX in_out, CX node_count
extern void matrix4x3_from_quaternion(real_quaternion *q, real_matrix4x3 *out); // 0x4cbad0, ECX q, EDX out
extern void matrix4x3_from_forward_up(real_vector3d *up, real_vector3d *forward, real_matrix4x3 *out); // 0x4cb970, EAX up, ECX forward
extern void vector3d_cross_product(real_vector3d *out, const real_vector3d *a, const real_vector3d *b); // 0x4052c0, EAX out, ECX a
extern void matrix4x3_transform_point(real_point3d *out, real_point3d *point, real_matrix4x3 *m); // 0x4cbde0, EAX out, EDX point

#define OFS(base, off, type) (*(type *)((uint8_t *)(base) + (off)))
#define TAG_DATA(id) ((uint8_t *)tag_instances[(uint32_t)(id) & 0xffff].data)

static void matrix4x3_set_translation_only(real_matrix4x3 *m, const real_point3d *position)
{
    m->scale = 1.0f;
    m->forward.i = 1.0f; m->forward.j = 0.0f; m->forward.k = 0.0f;
    m->left.i = 0.0f; m->left.j = 1.0f; m->left.k = 0.0f;
    m->up.i = 0.0f; m->up.j = 0.0f; m->up.k = 1.0f;
    m->position = *position;
}

// Recomputes an object's node matrices from its current animation state (base animation or default pose, the
// definition's overlay animations driven by the object's function values, the type's own hook and any queued
// blend) and derives its world bounding sphere from the root node.
void object_recalculate_bounding_radius(uint32_t object_index)
{
    uint8_t *obj = (uint8_t *)((object_header *)object_data->data)[object_index & 0xffff].data;
    uint8_t *def = TAG_DATA(OFS(obj, 0x0, uint32_t));                          // [ebp-0x1c]
    real_matrix4x3 *nodes = (real_matrix4x3 *)(obj + OFS(obj, 0x1f2, int16_t)); // [ebp-0x14]
    real_orientation local_orientations[k_maximum_nodes_per_model];            // [ebp-0xa0c]
    real_orientation *orientations;                                            // [ebp-0x18]

    // 0x4f834e..0x4f8381: object types in mask 0xfe0 use a scratch buffer, the rest their own orientation block
    if (((1u << (OFS(obj, 0xb4, uint8_t) & 0x1f)) & 0xfe0u) != 0) {
        orientations = local_orientations;
    } else {
        orientations = (real_orientation *)(obj + OFS(obj, 0x1ee, int16_t));
    }

    if (OFS(def, 0x34, int32_t) == -1) {
        // 0x4f8a5d: no model -- a single node from the object's own frame
        nodes[0].scale = 1.0f;
        nodes[0].forward = OFS(obj, 0x74, real_vector3d);
        nodes[0].up = OFS(obj, 0x80, real_vector3d);
        vector3d_cross_product(&nodes[0].left, &nodes[0].forward, &nodes[0].up);
        nodes[0].position = OFS(obj, 0x5c, real_point3d);
    } else {
        uint8_t *model = TAG_DATA(OFS(def, 0x34, uint32_t));                  // [ebp-0x28]
        real_matrix4x3 *parent_matrix = 0;                                    // [ebp-0x8]
        uint8_t absolute_root = 0;                                            // [ebp-0xd]
        int16_t queue[k_maximum_nodes_per_model];                              // [ebp-0x20c]
        int16_t head, tail;

        if (OFS(obj, 0x11c, int32_t) != -1) {
            uint8_t *parent = (uint8_t *)((object_header *)object_data->data)[OFS(obj, 0x11c, uint32_t) & 0xffff].data;
            parent_matrix = (real_matrix4x3 *)(parent + OFS(parent, 0x1f2, int16_t)) + OFS(obj, 0x120, int8_t);
        }

        // 0x4f83dd..0x4f8468: the base animation, or the model's default pose
        if (OFS(obj, 0xcc, int32_t) != -1 && OFS(obj, 0xd0, int16_t) != -1) {
            ModelAnimationsAnimation *animation = (ModelAnimationsAnimation *)(OFS(TAG_DATA(OFS(obj, 0xcc, uint32_t)), 0x78, uint8_t *) +
                (int32_t)OFS(obj, 0xd0, int16_t) * 0xb4);
            int16_t frame_count = OFS(animation, 0x22, int16_t);
            uint32_t frame;
            if (OFS(obj, 0x10, int8_t) < 0 && frame_count > 0) {
                frame = (OFS(game_time, 0xc, uint32_t) + object_index) % (uint32_t)(int32_t)frame_count;
            } else {
                frame = OFS(obj, 0xd2, uint16_t);
            }
            animation_get_frame_orientations(animation, (GBXModel *)model, (int16_t)frame, orientations);
            absolute_root = (uint8_t)((OFS(animation, 0x3a, uint8_t) >> 1) & 1);
        } else {
            model_nodes_get_default_transforms((GBXModel *)model, orientations);
        }

        // 0x4f846b..0x4f8570: overlay animations, each weighted by one of the object's function values
        if (OFS(def, 0x44, int32_t) != -1) {
            uint8_t *graph = TAG_DATA(OFS(def, 0x44, uint32_t));             // [ebp-0x2c]
            int16_t i;
            for (i = 0; (int32_t)i < OFS(graph, 0x0, int32_t); i++) {
                int16_t *entry = (int16_t *)(OFS(graph, 0x4, uint8_t *) + (int32_t)i * 0x14);
                if (entry[0] == -1 || (int32_t)entry[1] >= OFS(def, 0x158, int32_t)) {
                    continue;
                }
                {
                    ModelAnimationsAnimation *animation =
                        (ModelAnimationsAnimation *)(OFS(graph, 0x78, uint8_t *) + (int32_t)entry[0] * 0xb4);
                    float value = OFS(obj, 0x134 + (int32_t)entry[1] * 4, float);
                    if (entry[2] == 0) {
                        int32_t frames = (int32_t)OFS(animation, 0x22, int16_t);
                        if ((OFS(OFS(def, 0x15c, uint8_t *), (int32_t)entry[1] * 0x168, uint8_t) & 2) == 0) {
                            frames -= 1;
                        }
                        animation_overlay_interpolated_frame_orientations(animation, (float)frames * value, orientations);
                    } else if (entry[2] == 1) {
                        uint32_t frame = (OFS(game_time, 0xc, uint32_t) + object_index) %
                            (uint32_t)(int32_t)OFS(animation, 0x22, int16_t);
                        animation_overlay_frame_orientations_weighted(animation, (int16_t)frame, value, orientations);
                    }
                }
            }
        }

        // 0x4f8576..0x4f85b9: the object's scale on the root orientation
        if (OFS(obj, 0xb0, float) > 0.0f) {
            float scale = OFS(obj, 0xb0, float);
            orientations[0].scale *= scale;
            orientations[0].translation.x *= scale;
            orientations[0].translation.y *= scale;
            orientations[0].translation.z *= scale;
        }
        if (OFS(def, 0x44, int32_t) != -1) {
            object_type_definitions_notify_two_args_0x48(object_index, (uint32_t)orientations); // 0x4f85ca
        }
        if (OFS(obj, 0xd6, int16_t) > 0) {
            // 0x4f8601: EAX = orientations, CX = model node count, push blend block, (uint16)+0xd4, (uint16)+0xd6
            model_nodes_blend_transforms(orientations, OFS(model, 0xb8, int16_t),
                (real_orientation *)(obj + OFS(obj, 0x1ea, int16_t)), (int16_t)OFS(obj, 0xd4, uint16_t),
                (int16_t)OFS(obj, 0xd6, uint16_t));
        }

        // 0x4f8609..0x4f8a4f: breadth-first walk of the node tree
        queue[0] = 0;
        head = 0;
        tail = 1;
        do {
            int16_t node_index = queue[head++];
            uint8_t *node = OFS(model, 0xbc, uint8_t *) + (int32_t)node_index * 0x9c;

            if (node_index == 0) {
                real_matrix4x3 root;                                            // [ebp-0x154]
                matrix4x3_from_quaternion(&orientations[0].rotation, &root);
                root.scale = orientations[0].scale;
                root.position = orientations[0].translation;

                if (absolute_root) {
                    nodes[0] = root;                                            // 0x4f89b1
                } else {
                    real_matrix4x3 world;                                       // [ebp-0x74]
                    real_matrix4x3 orientation;                                 // [ebp-0x11c]
                    real_matrix4x3 offset;                                      // [ebp-0xe4] / [ebp-0xac]
                    real_matrix4x3 parent_copy;                                 // [ebp-0x18c]
                    real_matrix4x3 *base = parent_matrix;

                    matrix4x3_set_translation_only(&world, &OFS(obj, 0x5c, real_point3d));
                    matrix4x3_from_forward_up(&OFS(obj, 0x80, real_vector3d), &OFS(obj, 0x74, real_vector3d), &orientation);
                    if ((OFS(obj, 0x10, uint32_t) & 0x1000) != 0) {            // mirrored: negate left
                        orientation.left.i = -orientation.left.i;
                        orientation.left.j = -orientation.left.j;
                        orientation.left.k = -orientation.left.k;
                    }
                    if (OFS(def, 0x8c, int32_t) != -1) {                       // 0x4f8746: minus the tag's +0x0c point
                        uint8_t *tag = TAG_DATA(OFS(def, 0x8c, uint32_t));
                        real_point3d negated;
                        negated.x = -OFS(tag, 0xc, float);
                        negated.y = -OFS(tag, 0x10, float);
                        negated.z = -OFS(tag, 0x14, float);
                        matrix4x3_set_translation_only(&offset, &negated);
                        matrix4x3_multiply_procedure(&orientation, &offset, &orientation);
                    }
                    matrix4x3_set_translation_only(&offset, &OFS(def, 0x14, real_point3d)); // 0x4f8809
                    matrix4x3_multiply_procedure(&orientation, &offset, &orientation);

                    if (base != 0) {
                        if (base->scale != 1.0f) {                              // 0x4f88a9: fold the scale into position
                            world.position.x *= base->scale;
                            world.position.y *= base->scale;
                            world.position.z *= base->scale;
                            parent_copy = *base;
                            parent_copy.scale = 1.0f;
                            base = &parent_copy;
                        }
                        {
                            uint8_t *parent_object = (uint8_t *)((object_header *)object_data->data)
                                [OFS(obj, 0x11c, uint32_t) & 0xffff].data;
                            if ((OFS(parent_object, 0x10, uint32_t) & 0x1000) != 0) { // mirrored parent
                                if (base != &parent_copy) {
                                    parent_copy = *base;
                                    base = &parent_copy;
                                }
                                base->left.i = -base->left.i;
                                base->left.j = -base->left.j;
                                base->left.k = -base->left.k;
                            }
                        }
                        matrix4x3_multiply_procedure(base, &world, &nodes[0]);
                        matrix4x3_multiply_procedure(&nodes[0], &orientation, &nodes[0]);
                        matrix4x3_multiply_procedure(&nodes[0], &root, &nodes[0]);
                    } else {
                        matrix4x3_multiply_procedure(&world, &orientation, &nodes[0]);
                        matrix4x3_multiply_procedure(&nodes[0], &root, &nodes[0]);
                    }
                }
            } else {
                real_matrix4x3 *m = &nodes[node_index];                         // 0x4f89c9
                matrix4x3_from_quaternion(&orientations[node_index].rotation, m);
                m->scale = orientations[node_index].scale;
                m->position = orientations[node_index].translation;
                matrix4x3_multiply_procedure(&nodes[OFS(node, 0x24, int16_t)], m, m);
            }

            if (OFS(node, 0x20, int16_t) != -1) {
                queue[tail++] = OFS(node, 0x20, int16_t);
            }
            if (OFS(node, 0x22, int16_t) != -1) {
                queue[tail++] = OFS(node, 0x22, int16_t);
            }
        } while (head != tail);
    }

    // 0x4f8aba..0x4f8aee: the bounding sphere from the root node
    matrix4x3_transform_point(&OFS(obj, 0xa0, real_point3d), &OFS(def, 0x8, real_point3d), &nodes[0]);
    OFS(obj, 0xac, float) = OFS(def, 0x4, float);
    if (OFS(obj, 0xb0, float) > 0.0f) {
        OFS(obj, 0xac, float) = OFS(def, 0x4, float) * OFS(obj, 0xb0, float);
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
