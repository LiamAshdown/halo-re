// particle_system_new_on_marker  (Ghidra: FUN_004536f0, still unnamed there; named directly by
//   out/phase4/effects_types_notes.md: "particle_system_new_on_marker 0x4536f0")
// address 0x4536f0, size 442 bytes
// name confidence: 0.5   rewrite confidence: 0.45
// evidence: types/effects.h particle_system fields definition_index (+0x08), object_index
//   (+0x0c), attachment_index (+0x10), scale_function_index (+0x12), position (+0x20), velocity
//   (+0x2c, scaled by 30 here to turn a per-tick delta into a per-second rate), ambient_color
//   (+0x48) and flags (+0x04); types/tags.h ObjectAttachment (marker TagString +0x10,
//   primary_scale +0x30, change_color +0x34); types/objects.h object.definition_tag (+0x00),
//   object.change_colors (+0x1b8) and object_marker (size 0x6c, node_transform.position at the
//   tail); src/objects/object_get_node_local_transform.c, object_get_root_object_velocities.c
//   and object_function_get_value.c establish the three callees' real signatures.
// reconciled: 0x006851fc is a pointer to the opaque-white ColorARGB (0x00655138); one name global_white_argb: definition_index is a Ghidra-recognized stack parameter; object_index is
//   a Ghidra-recognized stack parameter; attachment_index in AX (in_AX).
//   // blam-cc: stack -> definition_index, stack -> object_index, in_AX -> attachment_index
// UNSURE: the change_color branch indexes object.change_colors directly by the raw
//   ObjectAttachment.change_color value with no -1 bias, unlike primary_scale two lines above it
//   (which the header already documents as "minus 1"); kept exactly as decoded. UNSURE: the
//   value read back from object_function_get_value's out-parameter is only used to decide the
//   emitting flag; nothing here writes particle_system.scale.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "effects.h"
#include "cache.h"
#include "fn_memory.h"

extern data_array *particle_system_data; // 0x0087abd4
extern data_array *object_data;          // 0x008603b0
extern tag_instance *tag_instances;      // 0x0087bc14
extern uint8_t particle_systems_enabled; // 0x0069c566
extern const ColorARGB *global_white_argb;    // 0x006851fc, opaque white per
                                    //   src/game/game_engine_koth_submit_hill_marker_geometry.c
extern const ColorRGB *global_white_color; // 0x00686b04. Ghidra names the
    // label PTR_DAT_00686b04 and every use in this module is `p = PTR_DAT_00686b04; ... *p`, so
    // 0x00686b04 holds a POINTER to the constant, not the constant itself. An earlier draft of
    // this file read it as an inline real_vector3d[4]; effect_set_placement.c 0x451600 and
    // effect_new_at_texture_coordinate.c 0x4506d0 already had it right.
                                    //   here; see src/game/game_engine_koth_reset_hill_marker_history.c


extern void datum_delete(data_array *array, datum_index handle); // 0x4d0510
extern int32_t object_get_node_local_transform(uint32_t object_index, char *marker_name,
    object_marker *marker, uint32_t maximum_markers); // 0x4f6080, objects module
extern void object_get_root_object_velocities(uint32_t object_index, real_vector3d *out_velocity,
    real_vector3d *out_angular_velocity); // 0x4f6aa0, objects module
extern uint8_t object_function_get_value(uint32_t object_index, int16_t selector,
    float *out_value); // 0x4f6e70, objects module
extern uint8_t particle_system_new_type_states(datum_index handle); // 0x4538b0, this module

datum_index particle_system_new_on_marker(uint32_t definition_index, uint32_t object_index,
    int16_t attachment_index) // blam-cc: stack, stack, in_AX
{
    datum_index handle = (datum_index)0xffffffff;

    if (particle_systems_enabled != 0) {
        handle = datum_new(particle_system_data);
        if (handle != (datum_index)0xffffffff) {
            object *obj = ((object_header *)object_data->data)[object_index & 0xffff].data;
            // 0x45375f..0x453771: the Object tag's attachment block pointer (+0x144), 0x48-byte entries
            ObjectAttachment *attachment = (ObjectAttachment *)(*(uint8_t **)((uint8_t *)tag_instances[
                obj->definition_tag & 0xffff].data + 0x144) + attachment_index * 0x48);
            particle_system *system =
                &((particle_system *)particle_system_data->data)[handle & 0xffff];
            object_marker marker;
            float function_value;

            system->definition_index = definition_index;
            system->object_index = object_index;
            system->attachment_index = attachment_index;
            system->scale_function_index = (int16_t)(attachment->primary_scale - 1);

            if (attachment->change_color == 0) {
                system->color = *global_white_argb;
            } else {
                system->color.red = obj->change_colors[attachment->change_color].red;
                system->color.green = obj->change_colors[attachment->change_color].green;
                system->color.blue = obj->change_colors[attachment->change_color].blue;
                system->color.alpha = 1.0f;
            }

            object_get_node_local_transform(object_index, attachment->marker.string, &marker, 1);
            system->position = marker.node_transform.position;

            object_get_root_object_velocities(object_index, &system->velocity,
                                               (real_vector3d *)0);
            system->velocity.i *= 30.0f;
            system->velocity.j *= 30.0f;
            system->velocity.k *= 30.0f;

            system->ambient_color = *global_white_color; // one load: the global holds the pointer (mov eax,ds:0x686b04; mov ecx,[eax])

            if (object_function_get_value(object_index, system->scale_function_index,
                                           &function_value)) {
                system->flags |= _particle_system_emitting_bit;
            } else {
                system->flags &= ~(uint32_t)_particle_system_emitting_bit;
            }

            if (!particle_system_new_type_states(handle)) {
                datum_delete(particle_system_data, handle);
                return (datum_index)0xffffffff;
            }
        }
    }
    return handle;
}

#if 0
Original Ghidra decompilation (0x4536f0):

uint FUN_004536f0(undefined4 param_1,uint param_2)

{
  int iVar1;
  uint *puVar2;
  undefined *puVar3;
  char cVar4;
  short in_AX;
  uint uVar5;
  uint uVar6;
  int iVar7;
  undefined8 uVar8;
  undefined1 local_6c [96];
  undefined4 local_c;
  undefined4 local_8;
  undefined4 local_4;

  uVar5 = 0xffffffff;
  if (DAT_0069c566 != '\0') {
    uVar8 = datum_new();
    uVar5 = (uint)uVar8;
    if (uVar5 != 0xffffffff) {
      puVar2 = *(uint **)(*(int *)(DAT_008603b0 + 0x34) + 8 + (param_2 & 0xffff) * 0xc);
      iVar7 = (uVar5 & 0xffff) * 0x158 + *(int *)((int)((ulonglong)uVar8 >> 0x20) + 0x34);
      iVar1 = *(int *)(*(int *)((*puVar2 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14) + 0x144) +
              in_AX * 0x48;
      *(undefined4 *)(iVar7 + 8) = param_1;
      *(uint *)(iVar7 + 0xc) = param_2;
      *(short *)(iVar7 + 0x10) = in_AX;
      *(short *)(iVar7 + 0x12) = *(short *)(iVar1 + 0x30) + -1;
      puVar3 = PTR_DAT_006851fc;
      if (*(short *)(iVar1 + 0x34) == 0) {
        *(undefined4 *)(iVar7 + 0x38) = *(undefined4 *)PTR_DAT_006851fc;
        *(undefined4 *)(iVar7 + 0x3c) = *(undefined4 *)(puVar3 + 4);
        *(undefined4 *)(iVar7 + 0x40) = *(undefined4 *)(puVar3 + 8);
        *(undefined4 *)(iVar7 + 0x44) = *(undefined4 *)(puVar3 + 0xc);
      }
      else {
        puVar2 = puVar2 + *(short *)(iVar1 + 0x34) * 3 + 0x6e;
        *(uint *)(iVar7 + 0x3c) = *puVar2;
        *(uint *)(iVar7 + 0x40) = puVar2[1];
        *(uint *)(iVar7 + 0x44) = puVar2[2];
        *(undefined4 *)(iVar7 + 0x38) = 0x3f800000;
      }
      object_get_node_local_transform(param_2,iVar1 + 0x10,local_6c,1);
      *(undefined4 *)(iVar7 + 0x20) = local_c;
      *(undefined4 *)(iVar7 + 0x24) = local_8;
      *(undefined4 *)(iVar7 + 0x28) = local_4;
      FUN_004f6aa0();
      puVar3 = PTR_DAT_00686b04;
      *(float *)(iVar7 + 0x2c) = *(float *)(iVar7 + 0x2c) * 30.0;
      *(float *)(iVar7 + 0x30) = *(float *)(iVar7 + 0x30) * 30.0;
      *(float *)(iVar7 + 0x34) = *(float *)(iVar7 + 0x34) * 30.0;
      *(undefined4 *)(iVar7 + 0x48) = *(undefined4 *)puVar3;
      *(undefined4 *)(iVar7 + 0x4c) = *(undefined4 *)(puVar3 + 4);
      *(undefined4 *)(iVar7 + 0x50) = *(undefined4 *)(puVar3 + 8);
      cVar4 = object_function_get_value();
      if (cVar4 == '\0') {
        uVar6 = *(uint *)(iVar7 + 4) & 0xfffffffe;
      }
      else {
        uVar6 = *(uint *)(iVar7 + 4) | 1;
      }
      *(uint *)(iVar7 + 4) = uVar6;
      cVar4 = FUN_004538b0(uVar5);
      if (cVar4 == '\0') {
        datum_delete();
        return 0xffffffff;
      }
    }
  }
  return uVar5;
}
#endif
