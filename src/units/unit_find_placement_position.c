// unit_find_placement_position  (Ghidra: unit_find_placement_position, renamed)
// address 0x55a500, size 1299 bytes
// name confidence: 0.35   rewrite confidence: 0.25
// evidence: object.nodes-basis fields at 0x74/0x78.../0x88 (forward/up, objects.h); Biped
//   collision_radius (0x42c, types/tags.h -- NOT crouch_transition_time, which is 0x408; an
//   earlier revision of this file used the wrong name) matches unit_get_crouch_height_offset's own
//   usage; global_up3d indirect pointer 0x00696720 (see the ground-adjust cluster's notes).
//   Every other callee here (FUN_005013a0, object_collision_context_build, FUN_005050b0, FUN_00506040,
//   FUN_00507170, FUN_00401a20, FUN_0053e780) is an unresolved collision/physics-module
//   function; their argument buffers are preserved with Ghidra's own byte layout rather than
//   invented structs.
// register convention: object index (candidate A) in EAX (default when param_1 is -1), a
//   direction pointer in EDX; param_1..param_7 are Ghidra-recognized stack parameters.
//   // blam-cc: EAX -> object_index_a (only used when param_1 == -1), EDX -> reference_direction,
//   //           stack -> anchor_object, orientation_object, out_position, radius, grid_mode,
//   //                    scale_by_prior, radius_from_prior
// UNSURE: the whole scatter-candidate collision test (the FUN_005013a0/507170/506040/5050b0/
//   401a20 chain) is preserved as opaque calls with the exact arguments Ghidra shows; this
//   rewrite does not claim to know what each one validates.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"
#include "units.h"

extern data_array *object_data;     // 0x008603b0
extern tag_instance *tag_instances; // 0x0087bc14
extern uint8_t *global_structure_bsp; // UNSURE global, 0x00746f9c
extern real_vector3d *global_up3d_pointer; // 0x00696720
extern real_vector3d unit_placement_candidate_offsets[]; // 0x0065e660/64/68 interleaved as 3 floats per candidate; UNSURE bound

extern real vector3d_normalize_with_length(real_vector3d *v); // 0x401990
extern void object_set_position_and_relink(void *position_and_flags); // 0x4f5350, UNSURE exact args
  // real signature (object_set_position_and_relink.c): void object_set_position_and_relink(real_point3d *position, uint32_t object_index); Ghidra recovered 1 of 2 args at this call site
extern void *memcpy(void *dst, const void *src, uint32_t n);
extern void object_recalculate_bounding_radius_recursive(uint32_t object_index); // 0x4f82b0
extern void unit_get_crouch_height_offset(uint32_t object_index, float *pill_height,
                                           float *pill_radius); // 0x55a2e0
extern int32_t FUN_005013a0(void);                                            // UNSURE module
extern int8_t object_collision_context_build(void);                                             // 0x504e10, collision module
extern char FUN_005050b0(void *a, real_point3d *b, real_vector3d *c, float d, void *scratch);  // UNSURE
extern char FUN_00506040(void *a, real_point3d *position, float radius);      // UNSURE
extern char FUN_00507170(void *a, float b, float c, float d, uint32_t object_index, real_point3d *out); // UNSURE
extern char FUN_00401a20(void *a, uint32_t object_index, void *out);          // UNSURE
extern void FUN_0053e780(void);                                               // UNSURE module

// Searches a small scatter pattern of candidate positions around an object for a spot free of
// collision, repositioning the object (or writing the result through out_position) at the first
// one that validates. See file header: most of the candidate-testing chain is preserved as
// opaque calls to other, not-yet-processed modules.
uint32_t unit_find_placement_position(uint32_t anchor_object, uint32_t orientation_object,
                                       real_point3d *out_position, float radius, char grid_mode,
                                       char skip_reposition, char scale_radius,
                                       uint32_t object_index_a, real_vector3d *reference_direction)
{
    object *reference;
    real_vector3d up_default;
    real_point3d basis_x, basis_y, basis_z, scaled_up;
    real_point3d scatter_reference;
    int32_t max_candidates;
    int32_t i;
    char found = 0;
    void *creation_snapshot[3] = {0}; // UNSURE: DAT_006e4d08-style buffer captured from orientation_object

    if (anchor_object == k_datum_index_none && orientation_object == k_datum_index_none) {
        return 0;
    }
    if (anchor_object != k_datum_index_none && orientation_object == k_datum_index_none) {
        goto have_reference;
    }
    if (anchor_object == k_datum_index_none) {
        object *orientation_obj = ((object_header *)object_data->data)[orientation_object & 0xffff].data;
        // UNSURE: preserved literally -- captures orientation_object's 0xa0/0xa4/0xa8 floats
        memcpy(creation_snapshot, (uint8_t *)orientation_obj + 0xa0, sizeof(creation_snapshot));
    }

have_reference:
    {
        int use_orientation_as_reference = (anchor_object == k_datum_index_none);
        uint32_t reference_index = use_orientation_as_reference ? orientation_object : anchor_object;
        reference = ((object_header *)object_data->data)[reference_index & 0xffff].data;

        {
            Biped *tag = (Biped *)tag_instances[reference->definition_tag & 0xffff].data;
            void *collision_context = (void *)(uint32_t)((-(uint32_t)((tag->biped_flags & 0x20) != 0) & 0xffdfff00) + 0x20c3a0); // UNSURE

            if (reference_direction != 0) {
                scatter_reference.x = reference_direction->i;
                scatter_reference.y = reference_direction->j;
                scatter_reference.z = reference_direction->k;
            }

            {
                float pill_height, pill_radius;
                unit_get_crouch_height_offset(reference_index, &pill_height, &pill_radius);

                if (use_orientation_as_reference) reference_index = k_datum_index_none;
                max_candidates = 0x1b - (((grid_mode != 0) - 1) & 9);
                if (orientation_object != k_datum_index_none) object_collision_context_build();

                basis_x.x = reference->up.k * reference->forward.j - reference->forward.k * reference->up.j;
                basis_x.y = reference->forward.k * reference->up.i - reference->up.k * reference->forward.i;
                basis_x.z = reference->up.j * reference->forward.i - reference->forward.j * reference->up.i;
                basis_y = basis_x;
                {
                    real_vector3d normalized = *(real_vector3d *)&basis_y;
                    vector3d_normalize_with_length(&normalized);
                }
                scaled_up.x = pill_height * global_up3d_pointer->i;
                scaled_up.y = pill_height * global_up3d_pointer->j;
                scaled_up.z = pill_height * global_up3d_pointer->k;

                if (scale_radius != 0) {
                    radius = pill_radius * radius; // UNSURE: local_520, see file header
                }

                for (i = 0; i < max_candidates && !found; i++) {
                    real_vector3d *offset = &unit_placement_candidate_offsets[i];
                    real_point3d candidate;

                    if (grid_mode == 0) {
                        candidate.x = radius * offset->i + scatter_reference.x;
                        candidate.y = radius * offset->j + scatter_reference.y;
                        candidate.z = radius * offset->k + scatter_reference.z;
                    } else {
                        float fx = radius * offset->i;
                        float fy = radius * offset->j;
                        float fz = radius * offset->k;
                        candidate.x = fz * reference->up.i + basis_x.x * fy + fx * reference->forward.i + scatter_reference.x;
                        candidate.y = fz * reference->up.j + basis_x.y * fy + fx * reference->forward.j + scatter_reference.y;
                        candidate.z = fz * reference->up.k + basis_x.z * fy + fx * reference->forward.k + scatter_reference.z;
                    }

                    {
                        int32_t marker = FUN_005013a0();
                        if (marker != -1 && *(int16_t *)(marker * 0x10 + 8 + *(int32_t *)(global_structure_bsp + 0xe4)) != -1) {
                            if (FUN_00507170(collision_context, radius + radius, pill_height,
                                              radius, reference_index, &candidate)) {
                                if (!FUN_00506040(collision_context, &candidate, radius)) {
                                    char ok = 1;
                                    if (orientation_object != k_datum_index_none) {
                                        uint8_t plane_scratch[16];
                                        uint8_t collision_scratch[1064];
                                        void *out_a[14] = {0}, *out_b[14] = {0};
                                        ok = !FUN_005050b0(collision_context, &candidate,
                                                            (real_vector3d *)creation_snapshot, pill_height,
                                                            collision_scratch);
                                        if (ok) {
                                            uint32_t hit_a = 0, hit_b = 0;
                                            char r1 = FUN_00401a20(collision_context, reference_index, out_a);
                                            ok = (r1 == 0) || (hit_a == orientation_object);
                                            if (ok) {
                                                char r2 = FUN_00401a20(collision_context, orientation_object, out_b);
                                                ok = (r2 == 0) || (hit_b == reference_index);
                                            }
                                        }
                                    }
                                    if (ok) {
                                        Biped *tag = (Biped *)tag_instances[reference->definition_tag & 0xffff].data;
                                        FUN_0053e780();
                                        if ((tag->biped_flags & 8) == 0) {
                                            candidate.z -= tag->collision_radius;
                                        }
                                        if (reference_index != k_datum_index_none && skip_reposition == 0) {
                                            reference->position = candidate;
                                            object_recalculate_bounding_radius_recursive(reference_index);
                                            object_set_position_and_relink(creation_snapshot);
                                        }
                                        if (out_position != 0) {
                                            *out_position = candidate;
                                        }
                                        found = 1;
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    return found;
}

#if 0
Original Ghidra decompilation (0x55a500):

uint FUN_0055a500(uint param_1,uint param_2,float *param_3,float param_4,char param_5,char param_6,
                 char param_7)

{
  uint *puVar1;
  float fVar2;
  float fVar3;
  char cVar4;
  uint in_EAX;
  float *in_EDX;
  int iVar5;
  bool bVar6;
  char local_536;
  float local_534;
  float local_530;
  float local_52c;
  float local_528;
  float local_524;
  float local_520;
  int local_51c;
  float local_518;
  float local_514;
  float local_510;
  int local_50c;
  float local_508;
  float local_504;
  float local_500;
  int local_4fc;
  float local_4f8;
  float local_4f4;
  float local_4f0;
  undefined4 local_4ec;
  undefined4 local_4e8;
  undefined4 local_4e4;
  undefined1 local_4e0 [8];
  undefined1 local_4d8 [16];
  undefined1 local_4c8 [56];
  uint local_490;
  undefined1 local_428 [1064];

  local_536 = '\0';
  if (param_1 == 0xffffffff) {
    if (param_2 == 0xffffffff) {
      return in_EAX & 0xffffff00;
    }
  }
  else if (param_2 == 0xffffffff) goto LAB_0055a568;
  iVar5 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (param_2 & 0xffff) * 0xc);
  local_4ec = *(undefined4 *)(iVar5 + 0xa0);
  local_4e8 = *(undefined4 *)(iVar5 + 0xa4);
  local_4e4 = *(undefined4 *)(iVar5 + 0xa8);
LAB_0055a568:
  bVar6 = param_1 == 0xffffffff;
  if (bVar6) {
    param_1 = param_2;
  }
  puVar1 = *(uint **)(*(int *)(DAT_008603b0 + 0x34) + 8 + (param_1 & 0xffff) * 0xc);
  local_51c = (-(uint)((*(uint *)(*(int *)((*puVar1 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14) + 0x2f4)
                       & 0x20) != 0) & 0xffdfff00) + 0x20c3a0;
  if (in_EDX != (float *)0x0) {
    local_518 = *in_EDX;
    local_514 = in_EDX[1];
    local_510 = in_EDX[2];
  }
  FUN_0055a2e0(&local_524);
  if (bVar6) {
    param_1 = 0xffffffff;
  }
  iVar5 = 0x1b - ((param_5 != '\0') - 1 & 9);
  local_4fc = iVar5;
  if (param_2 != 0xffffffff) {
    FUN_00504e10();
  }
  local_534 = (float)puVar1[0x22] * (float)puVar1[0x1e] - (float)puVar1[0x1f] * (float)puVar1[0x21];
  local_530 = (float)puVar1[0x1f] * (float)puVar1[0x20] - (float)puVar1[0x22] * (float)puVar1[0x1d];
  local_52c = (float)puVar1[0x21] * (float)puVar1[0x1d] - (float)puVar1[0x20] * (float)puVar1[0x1e];
  local_508 = local_534;
  local_504 = local_530;
  local_500 = local_52c;
  vector3d_normalize_with_length();
  local_4f8 = local_524 * *(float *)PTR_DAT_00696720;
  local_4f4 = local_524 * *(float *)(PTR_DAT_00696720 + 4);
  local_4f0 = local_524 * *(float *)(PTR_DAT_00696720 + 8);
  if (param_7 != '\0') {
    param_4 = local_520 * param_4;
  }
  local_50c = 0;
  do {
    if ((short)iVar5 <= (short)local_50c) break;
    iVar5 = (int)(short)local_50c;
    if (param_5 == '\0') {
      local_534 = param_4 * (float)(&DAT_0065e660)[iVar5 * 3] + local_518;
      local_530 = param_4 * (float)(&DAT_0065e664)[iVar5 * 3] + local_514;
      local_52c = param_4 * (float)(&DAT_0065e668)[iVar5 * 3] + local_510;
    }
    else {
      fVar2 = param_4 * (float)(&DAT_0065e660)[iVar5 * 3];
      fVar3 = param_4 * (float)(&DAT_0065e664)[iVar5 * 3];
      local_528 = param_4 * (float)(&DAT_0065e668)[iVar5 * 3];
      local_534 = local_528 * (float)puVar1[0x20] +
                  local_508 * fVar3 + fVar2 * (float)puVar1[0x1d] + local_518;
      local_530 = local_528 * (float)puVar1[0x21] +
                  local_504 * fVar3 + fVar2 * (float)puVar1[0x1e] + local_514;
      local_52c = local_528 * (float)puVar1[0x22] +
                  local_500 * fVar3 + fVar2 * (float)puVar1[0x1f] + local_510;
    }
    iVar5 = FUN_005013a0();
    fVar2 = local_520;
    if ((iVar5 != -1) && (*(short *)(iVar5 * 0x10 + 8 + *(int *)(DAT_00746f9c + 0xe4)) != -1)) {
      cVar4 = FUN_00507170(local_51c,local_520 + local_520,local_524,local_520,param_1,&local_534);
      if (cVar4 != '\0') {
        cVar4 = FUN_00506040(local_51c,&local_534,fVar2);
        if ((cVar4 == '\0') &&
           ((param_2 == 0xffffffff ||
            (((cVar4 = FUN_005050b0(local_4d8,&local_534,&local_4f8,fVar2,local_428), cVar4 == '\0'
              && ((cVar4 = FUN_00401a20(local_51c,param_1,local_4c8), cVar4 == '\0' ||
                  (local_490 == param_2)))) &&
             ((cVar4 = FUN_00401a20(local_51c,param_2,local_4c8), cVar4 == '\0' ||
              (local_490 == param_1)))))))) {
          iVar5 = *(int *)((*puVar1 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
          FUN_0053e780();
          if ((*(byte *)(iVar5 + 0x2f4) & 8) == 0) {
            local_52c = local_52c - *(float *)(iVar5 + 0x42c);
          }
          if ((param_1 != 0xffffffff) && (param_6 == '\0')) {
            puVar1[0x17] = (uint)local_534;
            puVar1[0x18] = (uint)local_530;
            puVar1[0x19] = (uint)local_52c;
            object_recalculate_bounding_radius_recursive(param_1);
            object_set_position_and_relink(local_4e0);
          }
          if (param_3 != (float *)0x0) {
            *param_3 = local_534;
            param_3[1] = local_530;
            param_3[2] = local_52c;
          }
          local_536 = '\x01';
        }
      }
    }
    local_50c = local_50c + 1;
    iVar5 = local_4fc;
  } while (local_536 == '\0');
  return CONCAT31((int3)((uint)local_50c >> 8),local_536);
}
#endif
