// model_render_parts  (Ghidra: FUN_004d72a0, unnamed; named "model_render_parts" to match
// types/models.h's own naming ("render_model 0x4d6fc0, model_render_parts 0x4d72a0"); no other
// already-committed module calls this one directly, so there is no cross-module name to match)
// address 0x4d72a0, size 849 bytes
// VERIFIED against disassembly 0x4d72a0..0x4d75f1 (2026-09-30)
// name confidence: 0.55   rewrite confidence: 0.75
// evidence: out/phase4/models_types_notes.md's model_render_pass, model_part_group_link and
//   GBXModelGeometryPart sections. Ghidra's own decompilation is complete for the region/
//   permutation/geometry/part walk and the pass dispatch; three callee calls
//   (matrix4x3_transform_point, and the two out-of-module draw callees reached through
//   LAB_004d75ad) drop some of their arguments, cross-checked against objdump -d -M intel
//   bin/halo.exe (scratchpad/halo_disasm.txt, 0x4d72a0..0x4d75fe) and against the two other
//   callees' own already-written files:
//     - rasterizer_shader_environment_draw_dispatch (src/rasterizer/…, 0x52b050): confirmed
//       EBX=dynamic_vertex_slot is set to -1 immediately before both of this function's calls
//       to it (`or ebx,0xffffffff`); the 6 stack args match one-for-one with what Ghidra
//       already recovered.
//     - rasterizer_object_shadow_model_draw (src/rasterizer/…, 0x531350): that file's own
//       header already documents this exact call site ("Call site: EAX -> shader (ShaderModel*),
//       pushes (frame, part +0x44, part +0x54)"), confirming EAX = the resolved Shader pointer.
//   The part-group-link bookkeeping (model_part_group_link, 0x4d72a0's own local array of 32)
//   was reconstructed from the second loop's raw stack offsets, which resolve cleanly against
//   the array once its base is placed at true offset 0x38 (0-pushes baseline): the -1 check
//   lands on +0x08 (group_index), the two writes after a successful record land on +0x0a
//   (linked_part_index) and +0x0c (part_index), and the closing link-chaining loop's
//   `*record.next_group_index = other.group_index` / `*other.previous_group_index =
//   record.group_index` (matching the plain-English description in the types notes exactly)
//   only makes sense at that same base. 0x52b180 takes EAX = &links[link_count] and eight
//   stack arguments, the last being the transformed part centroid (review pass: the first
//   rewrite passed an unused scratch buffer there and left the centroid dead; fixed against
//   the disassembly and src/rasterizer/rasterizer_transparent_geometry_group_build.c).
// register convention: none -- all six parameters are plain stack arguments.
//   // blam-cc: stack -> model, region_permutations, node_matrices, lod,
//   //           forced_shader_permutation, flags

#include "tags.h"
#include "math.h"
#include "cache.h"
#include "models.h"
#include "rasterizer.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern tag_instance *tag_instances; // 0x0087bc14
extern void matrix4x3_transform_point(real_point3d *out, real_point3d *point, real_matrix4x3 *m); // 0x4cbde0
extern void chimera__rasterizer_set_up_node_parts(int32_t node_part_count, uint8_t *node_part_indices); // 0x526cf0
extern void rasterizer_shader_environment_draw_dispatch(int32_t dynamic_vertex_slot, uint8_t *shader, int16_t frame,
                                                          rasterizer_index_buffer *index_buffer, int32_t dynamic_index_slot,
                                                          int32_t primitive_count, rasterizer_vertex_buffer *vertex_buffer); // 0x52b050
extern void rasterizer_object_shadow_model_draw(const ShaderModel *shader, int16_t frame,
                                                 rasterizer_index_buffer *index_buffer,
                                                 rasterizer_vertex_buffer *vertex_buffer); // 0x531350

// 0x52b180 is written in src/rasterizer/rasterizer_transparent_geometry_group_build.c; this is
// its prototype verbatim. The call site 0x4d7433..0x4d7456 pushes, last to first,
// &transformed_centroid (esp+0x2c), -1, part +0x54, part +0x48, -1, part +0x44, the
// permutation and the shader, and computes EAX = esp + link_count*0x10 + 0x54 - 0x1c pushes =
// &links[link_count] (array base esp+0x38). The eighth argument is therefore the part centroid
// that 0x4cbde0 has just transformed into node space, not a scratch buffer.
extern transparent_geometry_group *rasterizer_transparent_geometry_group_build(
    transparent_geometry_group_link *link, uint8_t *shader, int16_t frame, rasterizer_index_buffer *index_buffer,
    int32_t dynamic_index_slot, int32_t primitive_count, rasterizer_vertex_buffer *vertex_buffer,
    int32_t dynamic_vertex_slot, const real_point3d *position); // 0x52b180

// Walks every region/permutation/part of a model, three times (opaque, model-decal, then
// transparent -- skipping the last two in immediate mode), and hands each visible part to the
// matching rasterizer draw call for its shader type. Builds up to 32 model_part_group_link
// records during the transparent pass and chains matching ones together at the end of that
// pass.
void model_render_parts(GBXModel *model, uint8_t *region_permutations, rasterizer_node_matrices *node_matrices,
                         model_level_of_detail lod, uint16_t forced_shader_permutation, uint32_t flags)
{
    int16_t max_pass;
    model_render_pass pass;
    model_part_group_link links[k_maximum_model_part_group_links];

    max_pass = (int16_t)((~flags) & 2); // 0 (opaque pass only) when immediate, else 2 (all 3 passes)

    for (pass = _model_render_pass_opaque; (int16_t)pass <= max_pass; pass = (model_render_pass)(pass + 1)) {
        int16_t region;
        int16_t link_count = 0;

        // both loop bounds are int32 compares against the reflexive count (cmp eax,ecx at
        // 0x4d74d8 and cmp edi,[esi+0x24] at 0x4d74b7)
        for (region = 0; (int32_t)region < model->regions.count; region++) {
            int8_t permutation_index;
            ModelRegion *region_def;
            ModelRegionPermutation *permutations;
            ModelRegionPermutation *perm;
            int16_t geometry_index;
            GBXModelGeometry *geometry;
            int16_t part;

            // the 0xff test is on the byte, then movsx ecx,cl (0x4d7329): the index is signed
            permutation_index = (int8_t)region_permutations[region];
            if (permutation_index == -1) {
                continue;
            }

            region_def = &((ModelRegion *)model->regions.pointer)[region];
            permutations = (ModelRegionPermutation *)region_def->permutations.pointer;
            perm = &permutations[(int32_t)permutation_index];
            geometry_index = (&perm->super_low)[lod];
            if (geometry_index == -1) {
                continue;
            }

            geometry = &((GBXModelGeometry *)model->geometries.pointer)[geometry_index];

            for (part = 0; (int32_t)part < geometry->parts.count; part++) {
                GBXModelGeometryPart *p = &((GBXModelGeometryPart *)geometry->parts.pointer)[part];
                ModelShaderReference *shader_ref = &((ModelShaderReference *)model->shaders.pointer)[(int16_t)p->base.shader_index]; // movsx +0x04
                Shader *shader = (Shader *)tag_instances[shader_ref->shader.tag_id.index].data;
                int16_t shader_type;
                int16_t permutation;

                if (shader->shader_type <= 2 || shader->shader_type >= 0xc || (p->base.flags & 1) != 0) {
                    continue;
                }

                if ((p->base.flags & 2) != 0) {
                    chimera__rasterizer_set_up_node_parts(p->local_node_count, p->local_node_indices);
                }

                shader_type = (int16_t)shader->shader_type;
                permutation = (forced_shader_permutation == 0) ? (int16_t)shader_ref->permutation
                                                                : (int16_t)forced_shader_permutation;

                // shader_type == 1 can never be true here (the filter above already requires
                // 2 < shader_type < 0xc); kept exactly as decompiled rather than simplified
                // away -- see the file header.
                if (shader_type == 1 || (4 < shader_type && shader_type < 0xc)) {
                    if (pass == _model_render_pass_transparent) {
                        real_point3d transformed_centroid;
                        real_matrix4x3 *centroid_node_matrix =
                            (real_matrix4x3 *)node_matrices->matrices + (int16_t)p->base.centroid_primary_node; // movsx +0x08

                        matrix4x3_transform_point(&transformed_centroid, (real_point3d *)&p->base.centroid, centroid_node_matrix);

                        rasterizer_transparent_geometry_group_build(
                            (transparent_geometry_group_link *)&links[link_count], (uint8_t *)shader, permutation,
                            (rasterizer_index_buffer *)&p->base.triangle_buffer_type, -1,
                            (int32_t)p->base.triangle_count, (rasterizer_vertex_buffer *)&p->base.vertex_type, -1,
                            &transformed_centroid);

                        if (link_count < k_maximum_model_part_group_links && links[link_count].group_index != -1 &&
                            (flags & _model_render_flag_1_bit) == 0 &&
                            ((int8_t)p->base.next_filthy_part_index > 0 || (int8_t)p->base.prev_filthy_part_index > 0)) {
                            links[link_count].part_index = part;
                            links[link_count].linked_part_index = (int16_t)(int8_t)p->base.next_filthy_part_index;
                            link_count = link_count + 1;
                        }
                    }
                } else if (shader_type == 4 && (((ShaderModel *)shader)->shader_model_flags & 8) != 0) {
                    if (pass == _model_render_pass_model_decal) {
                        rasterizer_shader_environment_draw_dispatch(-1, (uint8_t *)shader, permutation,
                            (rasterizer_index_buffer *)&p->base.triangle_buffer_type, -1,
                            p->base.triangle_count, (rasterizer_vertex_buffer *)&p->base.vertex_type);
                    }
                } else if (pass == _model_render_pass_opaque) {
                    if ((flags & _model_render_immediate_bit) == 0) {
                        rasterizer_shader_environment_draw_dispatch(-1, (uint8_t *)shader, permutation,
                            (rasterizer_index_buffer *)&p->base.triangle_buffer_type, -1,
                            p->base.triangle_count, (rasterizer_vertex_buffer *)&p->base.vertex_type);
                    } else {
                        rasterizer_object_shadow_model_draw((const ShaderModel *)shader, permutation,
                            (rasterizer_index_buffer *)&p->base.triangle_buffer_type,
                            (rasterizer_vertex_buffer *)&p->base.vertex_type);
                    }
                }
            }
        }

        {
            int16_t i;
            for (i = 0; i < link_count; i++) {
                int16_t j;
                for (j = 0; j < link_count; j++) {
                    if (links[i].linked_part_index == links[j].part_index && links[i].linked_part_index > 0) {
                        *(int16_t *)links[i].next_group_index = links[j].group_index;
                        *(int16_t *)links[j].previous_group_index = links[i].group_index;
                        break;
                    }
                }
            }
        }
    }
}

#if 0
Original Ghidra decompilation (0x4d72a0):

void FUN_004d72a0(int param_1,int param_2,int *param_3,short param_4,short param_5,byte param_6)

{
  char cVar1;
  short sVar2;
  short sVar3;
  undefined2 *puVar4;
  ushort uVar5;
  short sVar6;
  short sVar7;
  short sVar8;
  int iVar9;
  undefined4 *puVar10;
  ushort uVar11;
  int iVar12;
  int iVar13;
  uint *puVar14;
  uint uVar15;
  undefined4 auStackY_80204 [2];
  short asStackY_801fc [262096];
  ushort local_214;
  undefined1 local_210 [12];
  undefined4 auStack_204 [2];
  short local_1fc [254];

  local_214 = (byte)~param_6 & 2;
  sVar8 = 0;
  do {
    uVar11 = 0;
    uVar5 = 0;
    sVar7 = 0;
    if (0 < *(int *)(param_1 + 0xc4)) {
      iVar9 = 0;
      do {
        if ((*(char *)(iVar9 + param_2) != -1) &&
           (sVar6 = *(short *)(*(int *)(iVar9 * 0x4c + 0x44 + *(int *)(param_1 + 200)) + 0x40 +
                              (*(char *)(iVar9 + param_2) * 0x2c + (int)param_4) * 2), sVar6 != -1))
        {
          iVar9 = sVar6 * 0x30;
          iVar13 = iVar9 + *(int *)(param_1 + 0xd4);
          sVar6 = 0;
          if (0 < *(int *)(iVar9 + 0x24 + *(int *)(param_1 + 0xd4))) {
            iVar9 = 0;
            do {
              puVar14 = (uint *)(iVar9 * 0x84 + *(int *)(iVar13 + 0x28));
              iVar9 = (short)puVar14[1] * 0x20;
              iVar12 = iVar9 + *(int *)(param_1 + 0xe0);
              iVar9 = *(int *)((*(uint *)(iVar12 + 0xc) & 0xffff) * 0x20 +
                               0x14 + DAT_0087bc14);
              if (((2 < *(short *)(iVar9 + 0x24)) && (*(short *)(iVar9 + 0x24) < 0xc)) &&
                 ((*puVar14 & 1) == 0)) {
                if ((*puVar14 & 2) != 0) {
                  chimera__rasterizer_set_up_node_parts();
                }
                sVar2 = *(short *)(iVar9 + 0x24);
                if ((sVar2 == 1) || ((4 < sVar2 && (sVar2 < 0xc)))) {
                  if (sVar8 == 2) {
                    matrix4x3_transform_point((short)puVar14[2] * 0x34 + *param_3);
                    sVar2 = param_5;
                    if (param_5 == 0) {
                      sVar2 = *(short *)(iVar12 + 0x10);
                    }
                    iVar12 = (int)(short)uVar5;
                    FUN_0052b180(iVar9,(int)sVar2,puVar14 + 0x11,0xffffffff,puVar14[0x12],
                                 puVar14 + 0x15,0xffffffff,local_210);
                    if ((((short)uVar5 < 0x20) && (local_1fc[iVar12 * 8] != -1)) &&
                       (((param_6 & 1) == 0 &&
                        ((cVar1 = *(char *)((int)puVar14 + 7), '\0' < cVar1 ||
                         ('\0' < *(char *)((int)puVar14 + 6))))))) {
                      uVar5 = uVar5 + 1;
                      local_1fc[iVar12 * 8 + 2] = sVar6;
                      local_1fc[iVar12 * 8 + 1] = (short)cVar1;
                    }
                  }
                }
                else {
                  sVar3 = param_5;
                  if ((sVar2 == 4) && ((*(byte *)(iVar9 + 0x28) & 8) != 0)) {
                    if (sVar8 == 1) {
                      if (param_5 == 0) {
                        sVar3 = *(short *)(iVar12 + 0x10);
                      }
                      uVar15 = puVar14[0x12];
LAB_004d75ad:
                      FUN_0052b050(iVar9,(int)sVar3,puVar14 + 0x11,0xffffffff,uVar15,puVar14 + 0x15)
                      ;
                    }
                  }
                  else if (sVar8 == 0) {
                    if ((param_6 & 2) == 0) {
                      if (param_5 == 0) {
                        sVar3 = *(short *)(iVar12 + 0x10);
                      }
                      uVar15 = puVar14[0x12];
                      goto LAB_004d75ad;
                    }
                    sVar2 = param_5;
                    if (param_5 == 0) {
                      sVar2 = *(short *)(iVar12 + 0x10);
                    }
                    FUN_00531350((int)sVar2,puVar14 + 0x11,puVar14 + 0x15);
                  }
                }
              }
              sVar6 = sVar6 + 1;
              iVar9 = (int)sVar6;
              uVar11 = uVar5;
            } while (iVar9 < *(int *)(iVar13 + 0x24));
          }
        }
        sVar7 = sVar7 + 1;
        iVar9 = (int)sVar7;
      } while (iVar9 < *(int *)(param_1 + 0xc4));
      if (0 < (short)uVar11) {
        puVar10 = auStack_204 + 1;
        uVar15 = (uint)uVar11;
        do {
          sVar7 = 0;
          do {
            if ((*(short *)((int)puVar10 + 6) == local_1fc[sVar7 * 8 + 2]) &&
               (0 < *(short *)((int)puVar10 + 6))) {
              puVar4 = (undefined2 *)auStack_204[sVar7 * 4];
              *(short *)*puVar10 = local_1fc[sVar7 * 8];
              *puVar4 = *(undefined2 *)(puVar10 + 1);
              break;
            }
            sVar7 = sVar7 + 1;
          } while (sVar7 < (short)uVar11);
          puVar10 = puVar10 + 4;
          uVar15 = uVar15 - 1;
        } while (uVar15 != 0);
      }
    }
    sVar8 = sVar8 + 1;
    if ((short)local_214 < sVar8) {
      return;
    }
  } while( true );
}

VERIFIED against objdump -d -M intel bin/halo.exe, 0x4d72a0..0x4d75fe (key excerpts):
  4d75b5  or ebx,0xffffffff / call 0x52b050        dynamic_vertex_slot = -1
  4d758a  mov eax,ebp / call 0x531350               shader = ebp (the Shader* this loop resolved)
  4d74e9  lea edx,[esp+0x3c]                        second-loop record_i cursor = &records[0].next_group_index
                                                     (records[] base = esp+0x38, record stride 0x10)
  4d75c5  mov ecx,[edx] / ... / mov [ecx],si         *record_i.next_group_index = records[j].group_index
  4d75d9  mov cx,[edx+4] / mov [eax],cx              *records[j].previous_group_index = record_i.group_index
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
