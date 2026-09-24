// object_create_attachments  (Ghidra: object_create_attachments, already named)
// address 0x4f9750, size 412 bytes
// name confidence: 0.85 (already carries this name from an earlier phase; matches
//   functions.md's summary: "Creates the runtime instance for every attachment (light/
//   looping-sound/effect/contrail/particle) defined on an object's type")
// rewrite confidence: 0.45
// evidence: types/objects.h object (attachment_types 0x144, attachment_handles 0x14c,
//   object_attachment_type enum, flags 0x10); types/tags.h Object.attachments (TagReflexive),
//   ObjectAttachment (type TagDependency, marker TagString, primary_scale 0x30,
//   secondary_scale 0x32, change_color 0x34); global 0x008603b0 object_data, 0x0087bc14
//   tag_instances; callee light_new_attached (0x4f0af0, established: EAX -> light_tag,
//   stack -> owner_object, marker_index, marker_index_secondary, change_color_index).
// register convention: object index is the sole, genuinely-stack, parameter (Ghidra's own
//   "object_create_attachments(uint param_1)").
// UNSURE: looping_sound_new (0x543c20), effect_new_at_texture_coordinate (effect), contrail_new (contrail) and
//   particle_system_new_on_marker (particle system) are all foreign-module constructors whose full argument
//   lists Ghidra could not recover at this call site (each shows fewer visible arguments than
//   it plausibly needs); only the visible arguments are forwarded here, exactly as decompiled.
//   Object flags bits 0x100 and 0x400 (set on a successful light/looping-sound attachment) are
//   not in types/objects.h's object_flags enum; preserved as raw hex.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"

extern data_array *object_data; // 0x008603b0
extern tag_instance *tag_instances; // 0x0087bc14

extern datum_index light_new_attached(datum_index light_tag, datum_index owner_object,
    int16_t marker_index, int16_t marker_index_secondary, int16_t change_color_index); // 0x4f0af0
extern datum_index looping_sound_new(int16_t primary_scale_minus_1); // 0x543c20, foreign module, UNSURE
extern datum_index effect_new_at_texture_coordinate(int32_t primary_scale_minus_1, int16_t secondary_scale_minus_1); // 0x4506d0, foreign module, UNSURE
extern datum_index contrail_new(datum_index tag); // 0x44c910, foreign module, UNSURE
extern datum_index particle_system_new_on_marker(datum_index tag, uint32_t object_index); // 0x4536f0, foreign module, UNSURE

void object_create_attachments(uint32_t object_index)
{
    object *obj = ((object_header *)object_data->data)[object_index & 0xffff].data;
    Object *definition = (Object *)tag_instances[obj->definition_tag & 0xffff].data;
    int16_t i;

    for (i = 0; i < (int16_t)definition->attachments.count; i++) {
        ObjectAttachment *attach = (ObjectAttachment *)definition->attachments.pointer + i;
        datum_index tag = attach->type.tag_id.index == 0xffff ? k_datum_index_none :
            *(datum_index *)&attach->type.tag_id;
        int8_t type = _object_attachment_type_none;
        datum_index handle = k_datum_index_none;

        if (tag != k_datum_index_none) {
            switch (attach->type.tag_fourcc) {
                case 0x6c696768: type = _object_attachment_type_light; break;
                case 0x6c736e64: type = _object_attachment_type_looping_sound; break;
                case 0x65666665: type = _object_attachment_type_effect; break;
                case 0x636f6e74: type = _object_attachment_type_contrail; break;
                case 0x7063746c: type = _object_attachment_type_particle_system; break;
            }
        }

        switch (type) {
            case _object_attachment_type_light:
                handle = light_new_attached(tag, object_index, i, attach->primary_scale - 1, attach->secondary_scale - 1);
                if (handle != k_datum_index_none) {
                    obj->flags |= 0x100; // UNSURE: undocumented bit, see file header
                }
                break;
            case _object_attachment_type_looping_sound:
                handle = looping_sound_new(attach->primary_scale - 1);
                if (handle != k_datum_index_none) {
                    obj->flags |= 0x400; // UNSURE: undocumented bit, see file header
                }
                break;
            case _object_attachment_type_effect:
                handle = effect_new_at_texture_coordinate(attach->primary_scale - 1, attach->secondary_scale - 1);
                break;
            case _object_attachment_type_contrail:
                handle = contrail_new(tag);
                break;
            case _object_attachment_type_particle_system:
                handle = particle_system_new_on_marker(tag, object_index);
                break;
            default:
                break;
        }

        obj->attachment_types[i] = type;
        obj->attachment_handles[i] = handle;
    }
}

#if 0
Original Ghidra decompilation (0x4f9750):

void object_create_attachments(uint param_1)

{
  uint *puVar1;
  uint *puVar2;
  int iVar3;
  int iVar4;
  uint uVar5;
  uint uVar6;
  undefined2 uVar7;
  int iVar8;
  int iVar9;

  puVar2 = *(uint **)(*(int *)(DAT_008603b0 + 0x34) + 8 + (param_1 & 0xffff) * 0xc);
  iVar3 = *(int *)((*puVar2 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
  iVar8 = 0;
  if (0 < *(int *)(iVar3 + 0x140)) {
    iVar9 = 0;
    do {
      iVar4 = *(int *)(*(int *)(iVar3 + 0x144) + 0xc + iVar9 * 0x48);
      puVar1 = (uint *)(*(int *)(iVar3 + 0x144) + iVar9 * 0x48);
      uVar7 = 0xffff;
      uVar6 = 0xffffffff;
      if (iVar4 != -1) {
        uVar5 = *puVar1;
        if (uVar5 < 0x6c696769) {
          if (uVar5 == 0x6c696768) {
            uVar7 = 0;
          }
          else if (uVar5 == 0x636f6e74) {
            uVar7 = 3;
          }
          else if (uVar5 == 0x65666665) {
            uVar7 = 2;
          }
        }
        else if (uVar5 == 0x6c736e64) {
          uVar7 = 1;
        }
        else if (uVar5 == 0x7063746c) {
          uVar7 = 4;
        }
      }
      switch(uVar7) {
      case 0:
        uVar6 = FUN_004f0af0(iVar4,param_1,iVar8,
                             CONCAT22((short)puVar1[0xc] >> 0xf,(short)puVar1[0xc] + -1),
                             (short)puVar1[0xd] + -1);
        if (uVar6 != 0xffffffff) {
          puVar2[4] = puVar2[4] | 0x100;
        }
        break;
      case 1:
        uVar6 = looping_sound_new((short)puVar1[0xc] + -1);
        if (uVar6 != 0xffffffff) {
          puVar2[4] = puVar2[4] | 0x400;
        }
        break;
      case 2:
        uVar6 = FUN_004506d0(CONCAT22((short)puVar1[0xc] >> 0xf,(short)puVar1[0xc] + -1),
                             *(short *)((int)puVar1 + 0x32) + -1);
        break;
      case 3:
        uVar6 = FUN_0044c910(iVar4);
        break;
      case 4:
        uVar6 = FUN_004536f0(iVar4,param_1);
      }
      *(char *)(iVar9 + 0x144 + (int)puVar2) = (char)uVar7;
      puVar2[iVar9 + 0x53] = uVar6;
      iVar8 = iVar8 + 1;
      iVar9 = (int)(short)iVar8;
    } while (iVar9 < *(int *)(iVar3 + 0x140));
  }
  return;
}
#endif
