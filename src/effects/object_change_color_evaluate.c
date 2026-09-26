// object_change_color_evaluate  (Ghidra: FUN_004529d0; named per the phase-2 pass and
// out/phase4/effects_types_notes.md, which keeps the name but relocates the function to this
// module's event pipeline: "it is reached only from effect_update, so it belongs to the effect
// event pipeline rather than to objects")
// address 0x4529d0, size 770 bytes
// name confidence: 0.5 (kept from phase 2 despite the relocation)   rewrite confidence: 0.55
//   (raised by the phase-4 integration pass, which resolved the two "extra vectors" at the
//   effect_event_apply call site -- see the RESOLVED note below)
// evidence: types/effects.h effect (definition_index 0x04, flags bit 6 _effect_first_person_bit,
// location_markers[32] 0x5c, a_scale 0x44, b_scale 0x48) and effect_location_marker (marker_index
// 0x02, next_marker 0x04, transform 0x08); types/tags.h Effect.locations (count 0x28) and
// EffectPart (location 0x04, violence_mode 0x02, flags 0x06 face_down_regardless_of_location_decals,
// type_class 0x14, type.tag_id 0x24, a_scales_values/b_scales_values 0x60/0x64 bit 5
// type_specific_scale) -- module note "EffectEvent.parts is at 0x2c" (Effect.events+0x2c is
// EffectEvent.parts, read here as `tag->events... ` no: this function reads Effect.locations
// directly via tag+0x28, and each location's marker list via effect.location_markers[index]);
// types/objects.h object_marker / real_matrix4x3 (scale 0, forward 4, left 0x10, up 0x1c,
// position 0x28) and object.nodes (object_block_reference at 0x1f0); this module's
// effect_marker_next 0x453180 and effect_resolve_marker_transform 0x453220; math module
// global_forward3d_pointer (0x696718) and this module's global_down3d_pointer (0x69672c, see
// effect_marker_environment_probe.c).
// register is effect* in EAX (in_EAX read through the whole body via casts of `param_1`).
// RESOLVED by the phase-4 integration pass: matrix4x3_transform_point(iVar9) is NOT decompiler
// noise. It is the only writer of the third vector of the placement block (Ghidra's
// local_c/local_8/local_4), which the sentinel branch fills from the marker transform's own
// position and this branch has to fill by pushing that position through the node matrix. Its
// out/point arguments are in EAX/EDX (src/math/matrix4x3_transform_point.c), which is why only
// the matrix survives. The hand-written rotations that follow it build the other two vectors and
// are kept as they were.
// UNSURE: EffectPart's second field is typed EffectViolenceMode_t (violent/nonviolent) in
// types/tags.h, but the comparison here (values 1 and 2, gated by self->flags'
// _effect_first_person_bit) matches EffectCreate_t's only_in_first_person/only_in_third_person
// far better than a violence setting. types/tags.h is not owned by this rewrite, so the field is
// read as-is (the numeric values agree either way) and flagged here rather than renamed.
// RESOLVED by the phase-4 integration pass: the three vectors effect_event_apply reads are the
// three slots of ONE contiguous 9-float block this function builds on its own stack. Ghidra's
// locals run local_24/20/1c, local_18/14/10, local_c/8/4 in ascending address order, and the
// only argument passed is &local_24. In effect_event_apply's 'obje' branch those land on
// object_placement_data at +0x40 (up, from param_4), +0x34 (forward, from in_EAX) and +0x18
// (position, from in_ECX), which fixes the mapping exactly:
//     param_4 = &block[0] = up     EAX = &block[1] = forward     ECX = &block[2] = position
// so the caller passes one pointer and the ABI hands the other two sub-vectors in registers.
// The slot contents are read straight off this function: the sentinel branch fills them with
// the marker transform's up / forward / position, the node branch with the rotated up, the
// rotated forward and the transformed position, and the face_down flag overwrites slot 0 with
// global_forward3d and slot 1 with global_down3d.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "cache.h"
#include "effects.h"

extern data_array *effect_location_data; // 0x0087abe0
extern data_array *object_data;          // 0x008603b0
extern tag_instance *tag_instances;      // 0x0087bc14
extern const real_vector3d *global_down3d_pointer;    // 0x0069672c, this module
extern const real_vector3d *global_forward3d_pointer; // 0x00696718, math module

extern effect_location_marker *effect_marker_next(effect *self, datum_index *marker,
    int32_t mode); // 0x453180, this module
extern real_matrix4x3 *effect_resolve_marker_transform(effect *self, int16_t marker); // 0x453220,
    // this module
extern uint8_t scenario_location_get_water_and_weather(void *unknown_0, int32_t unknown_1); // 0x53ed60, outside this batch;
    // UNSURE: presumed to test an environment/spawn-condition gate (its result is inverted for
    // EffectCreateIn air_only, used directly for water_only)
extern void matrix4x3_transform_point(real_point3d *out, real_point3d *point,
    real_matrix4x3 *m); // 0x4cbde0, math module; blam-cc: EAX -> out, EDX -> point, stack -> m
extern void effect_event_apply(effect *self, EffectPart *part, effect_location_marker *marker,
    real_vector3d *up, real_vector3d *forward, real_point3d *position, real scale);
    // 0x452cf0, this module. Only `up` is a visible stack argument (Ghidra's param_4); forward
    // and position arrive in EAX and ECX and are the next two slots of the same block.

// For every EffectLocation whose location index resolves and whose part's environment/violence
// gate passes, walks that location's marker list and applies each qualifying EffectPart:
// resolving the marker's world placement (either straight from the marker transform for the
// object-origin sentinel, or rotated through the attached node/first-person-weapon transform
// otherwise, or a hard-coded down/forward pair when the part is flagged
// face_down_regardless_of_location_decals), then dispatching through effect_event_apply.
// FIXED (register inputs, objdump; one stack argument remains, so no ordering question): the original never reads EAX; self arrive(s) on the stack (1 stack argument(s)).
// blam-cc: stack -> self
void object_change_color_evaluate(effect *self)
{
    Effect *tag = (Effect *)tag_instances[(uint16_t)self->definition_index].data;
    EffectEvent *event = &((EffectEvent *)tag->events.pointer)[self->event_index];
    EffectPart *parts = (EffectPart *)event->parts.pointer;
    int32_t part_index;

    for (part_index = 0; part_index < (int32_t)event->parts.count; part_index++) {
        EffectPart *part = &parts[part_index];
        int16_t location = part->location;

        if (location >= 0 && (int32_t)location < (int32_t)tag->locations.count &&
            (part->type.tag_id.index != 0xffff || part->type.tag_id.id != 0xffff)) {
            uint8_t skip_part;

            if ((self->flags & _effect_first_person_bit) == 0) {
                skip_part = (part->violence_mode == 2); // UNSURE: see file header
            } else {
                skip_part = (part->violence_mode == 1); // UNSURE: see file header
            }

            if (!skip_part) {
                datum_index marker_handle = self->location_markers[location];
                uint8_t create_ok = 1; // carried across markers, matching the raw goto shape

                while (marker_handle != k_datum_index_none) {
                    effect_location_marker *entry =
                        &((effect_location_marker *)effect_location_data->data)[(uint16_t)marker_handle];
                    marker_handle = entry->next_marker;

                    if (entry->marker_index != 0xffff && (entry->marker_index & 0x8000) != 0) {
                        entry = effect_marker_next(self, &marker_handle, 0);
                    }
                    if (entry == (effect_location_marker *)0) {
                        break;
                    }

                    // One contiguous 9-float block: [0] up, [1] forward, [2] position. See the
                    // RESOLVED note in the file header for why effect_event_apply only takes a
                    // pointer to the first of the three.
                    real_vector3d placement[3];

                    if (entry->marker_index == 0xffff) {
                        placement[0] = entry->transform.up;
                        placement[1] = entry->transform.forward;
                        *(real_point3d *)&placement[2] = entry->transform.position;
                    } else {
                        real_matrix4x3 *node = effect_resolve_marker_transform(self, (int16_t)entry->marker_index);
                        real forward_i = entry->transform.forward.i;
                        real forward_j = entry->transform.forward.j;
                        real forward_k = entry->transform.forward.k;
                        real up_i = entry->transform.up.i;
                        real up_j = entry->transform.up.j;
                        real up_k = entry->transform.up.k;

                        // duplicated matrix4x3_transform_normal (rotation only, no scale/translate);
                        // see file header UNSURE note about the spurious matrix4x3_transform_point call
                        placement[1].i = forward_j * node->left.i + forward_k * node->up.i + forward_i * node->forward.i;
                        placement[1].j = forward_i * node->left.j + forward_j * node->up.j + forward_k * node->forward.j;
                        placement[1].k = forward_j * node->left.k + forward_k * node->up.k + forward_i * node->forward.k;
                        placement[0].i = up_j * node->left.i + up_k * node->up.i + up_i * node->forward.i;
                        placement[0].j = up_i * node->left.j + up_j * node->up.j + up_k * node->forward.j;
                        placement[0].k = up_j * node->left.k + up_k * node->up.k + up_i * node->forward.k;

                        // The one call this branch makes: the marker's position pushed through
                        // the node matrix into slot 2. See the RESOLVED note in the header.
                        matrix4x3_transform_point((real_point3d *)&placement[2],
                            &entry->transform.position, node);
                    }

                    if ((part->flags & 1) != 0) { // face_down_regardless_of_location_decals
                        placement[1] = *global_down3d_pointer;
                        placement[0] = *global_forward3d_pointer;
                        // slot 2 (position) is deliberately left alone, exactly as in the
                        // original.
                    }

                    switch (part->create_in) {
                    case effectcreatein_any_environment:
                        create_ok = 1;
                        break;
                    case effectcreatein_air_only:
                        create_ok = !scenario_location_get_water_and_weather(&self->location, 0); // the raw `iVar5 + 0x10`
                            // is a BYTE offset; on a typed `effect *` that is &self->location
                        break;
                    case effectcreatein_water_only:
                        create_ok = scenario_location_get_water_and_weather(&self->location, 0); // see above
                        break;
                    case effectcreatein_space_only:
                        create_ok = 0; // retail: space-only parts never create in this build
                        break;
                    default:
                        create_ok = 0;
                        break;
                    }

                    if (create_ok) {
                        real scale = 1.0f;
                        if ((part->a_scales_values & 0x20) != 0) { // type_specific_scale, A set
                            scale = self->a_scale;
                        }
                        if ((part->b_scales_values & 0x20) != 0) { // type_specific_scale, B set
                            scale = scale * self->b_scale;
                        }
                        effect_event_apply(self, part, entry, &placement[0], &placement[1],
                            (real_point3d *)&placement[2], scale);
                    }
                }
            }
        }
    }
}

#if 0
Original Ghidra decompilation (0x4529d0):

void FUN_004529d0(float param_1)

{
  float fVar1;
  float fVar2;
  float fVar3;
  short sVar4;
  int iVar5;
  int iVar6;
  undefined2 *puVar7;
  int iVar8;
  int iVar9;
  char cVar10;
  bool bVar11;
  uint local_38;
  int local_34;
  int local_30;
  undefined2 *local_2c;
  int local_28;
  float local_24;
  float local_20;
  float local_1c;
  float local_18;
  float local_14;
  float local_10;
  undefined4 local_c;
  undefined4 local_8;
  undefined4 local_4;

  iVar5 = (int)param_1;
  local_30 = *(int *)((*(uint *)((int)param_1 + 4) & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
  iVar8 = *(short *)((int)param_1 + 0x4e) * 0x44;
  iVar9 = iVar8 + *(int *)(local_30 + 0x38);
  iVar6 = 0;
  local_34 = 0;
  if (0 < *(int *)(iVar8 + 0x2c + *(int *)(local_30 + 0x38))) {
    local_28 = iVar9;
    do {
      puVar7 = (undefined2 *)(iVar6 * 0x68 + *(int *)(iVar9 + 0x30));
      sVar4 = puVar7[2];
      if (((-1 < sVar4) && ((int)sVar4 < *(int *)(local_30 + 0x28))) &&
         (*(int *)(puVar7 + 0x12) != -1)) {
        if ((*(byte *)(iVar5 + 2) >> 6 & 1) == 0) {
          bVar11 = puVar7[1] == 2;
        }
        else {
          bVar11 = puVar7[1] == 1;
        }
        if (!bVar11) {
          local_38 = *(uint *)(iVar5 + 0x5c + sVar4 * 4);
          cVar10 = param_1._0_1_;
          local_2c = puVar7;
LAB_00452a80:
          while (iVar9 = local_28, param_1._0_1_ = cVar10, local_38 != 0xffffffff) {
            iVar6 = (local_38 & 0xffff) * 0x3c + *(int *)(DAT_0087abe0 + 0x34);
            local_38 = *(uint *)(iVar6 + 4);
            if ((*(short *)(iVar6 + 2) != -1) && (*(short *)(iVar6 + 2) < 0)) {
              iVar6 = FUN_00453180(iVar5,&local_38,0);
            }
            iVar9 = local_28;
            if (iVar6 == 0) break;
            if (*(short *)(iVar6 + 2) == -1) {
              local_c = *(undefined4 *)(iVar6 + 0x30);
              local_8 = *(undefined4 *)(iVar6 + 0x34);
              local_4 = *(undefined4 *)(iVar6 + 0x38);
              local_18 = *(float *)(iVar6 + 0xc);
              local_14 = *(float *)(iVar6 + 0x10);
              local_10 = *(float *)(iVar6 + 0x14);
              local_24 = *(float *)(iVar6 + 0x24);
              local_20 = *(float *)(iVar6 + 0x28);
              local_1c = *(float *)(iVar6 + 0x2c);
            }
            else {
              iVar9 = FUN_00453220();
              matrix4x3_transform_point(iVar9);
              fVar1 = *(float *)(iVar6 + 0xc);
              fVar2 = *(float *)(iVar6 + 0x10);
              fVar3 = *(float *)(iVar6 + 0x14);
              local_18 = fVar2 * *(float *)(iVar9 + 0x10) +
                         fVar3 * *(float *)(iVar9 + 0x1c) + fVar1 * *(float *)(iVar9 + 4);
              local_14 = fVar1 * *(float *)(iVar9 + 8) +
                         fVar2 * *(float *)(iVar9 + 0x14) + fVar3 * *(float *)(iVar9 + 0x20);
              local_10 = fVar2 * *(float *)(iVar9 + 0x18) +
                         fVar3 * *(float *)(iVar9 + 0x24) + fVar1 * *(float *)(iVar9 + 0xc);
              fVar1 = *(float *)(iVar6 + 0x24);
              fVar2 = *(float *)(iVar6 + 0x28);
              fVar3 = *(float *)(iVar6 + 0x2c);
              local_24 = fVar2 * *(float *)(iVar9 + 0x10) +
                         fVar3 * *(float *)(iVar9 + 0x1c) + fVar1 * *(float *)(iVar9 + 4);
              local_20 = fVar1 * *(float *)(iVar9 + 8) +
                         fVar2 * *(float *)(iVar9 + 0x14) + fVar3 * *(float *)(iVar9 + 0x20);
              local_1c = fVar2 * *(float *)(iVar9 + 0x18) +
                         fVar3 * *(float *)(iVar9 + 0x24) + fVar1 * *(float *)(iVar9 + 0xc);
              puVar7 = local_2c;
            }
            if ((*(byte *)(puVar7 + 3) & 1) != 0) {
              local_18 = *(float *)PTR_DAT_0069672c;
              local_14 = *(float *)(PTR_DAT_0069672c + 4);
              local_10 = *(float *)(PTR_DAT_0069672c + 8);
              local_24 = *(float *)PTR_DAT_00696718;
              local_20 = *(float *)(PTR_DAT_00696718 + 4);
              local_1c = *(float *)(PTR_DAT_00696718 + 8);
            }
            switch(*puVar7) {
            case 0:
              cVar10 = '\x01';
              goto LAB_00452c4b;
            case 1:
              cVar10 = FUN_0053ed60(iVar5 + 0x10,0);
              cVar10 = '\x01' - (cVar10 != '\0');
              break;
            case 2:
              cVar10 = FUN_0053ed60(iVar5 + 0x10,0);
              break;
            case 3:
              goto switchD_00452c21_caseD_3;
            }
            if (cVar10 != '\0') {
LAB_00452c4b:
              param_1 = 1.0;
              if ((*(byte *)(puVar7 + 0x30) & 0x20) != 0) {
                param_1 = *(float *)(iVar5 + 0x44);
              }
              if ((*(byte *)(puVar7 + 0x32) & 0x20) != 0) {
                param_1 = param_1 * *(float *)(iVar5 + 0x48);
              }
              effect_event_apply(iVar5,puVar7,iVar6,&local_24,param_1);
            }
          }
        }
      }
      local_34 = local_34 + 1;
      iVar6 = (int)(short)local_34;
    } while (iVar6 < *(int *)(iVar9 + 0x2c));
  }
  return;
switchD_00452c21_caseD_3:
  cVar10 = '\0';
  goto LAB_00452a80;
}
#endif
