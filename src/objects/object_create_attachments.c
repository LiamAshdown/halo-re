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

// REWRITTEN (from objdump 0x4f9750..0x4f98eb). Per attachment (Object +0x140 count, 0x48 each at +0x144; tag
//   dependency fourcc +0, tag id +0xc, marker name +0x10, words +0x30/+0x32/+0x34 each passed minus one):
//   light: light_new_attached(tag, object, i, +0x30 - 1, +0x34 - 1), object flag 0x100 on success;
//   looping sound: looping_sound_new(EAX object, EDI tag, ECX marker, stack +0x30 - 1), object flag 0x400;
//   effect: effect_new_at_texture_coordinate(EAX tag, EDX object, CX +0x34 - 1, stack +0x30 - 1, +0x32 - 1);
//   contrail: contrail_new(AX i, ECX object, stack tag); particle system: particle_system_new_on_marker(stack
//   tag, object, AX i). The type byte (-1 for none) goes to object +0x144 + i and the handle to +0x14c + 4i.
//   The draft passed guessed arguments to all but the light (looping sounds read a garbage marker: the crash).
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"

extern data_array *object_data; // 0x008603b0
extern tag_instance *tag_instances; // 0x0087bc14

extern datum_index light_new_attached(datum_index light_tag, datum_index owner_object, int16_t marker_index,
    int16_t marker_index_secondary, int16_t change_color_index); // 0x4f0af0, all on the stack
extern datum_index looping_sound_new(datum_index object_index, datum_index definition_index, char *marker_name,
    int16_t function_index); // 0x543c20, blam-cc: EAX, EDI, ECX, stack
extern datum_index effect_new_at_texture_coordinate(datum_index definition_index, datum_index object_index,
    int16_t change_color_index, int16_t u, int16_t v); // 0x4506d0, blam-cc: EAX, EDX, CX, stack (u, v)
extern datum_index contrail_new(int16_t attachment_index, datum_index object_index, datum_index definition_index);
    // 0x44c910, blam-cc: AX, ECX, stack
extern datum_index particle_system_new_on_marker(uint32_t definition_index, uint32_t object_index,
    int16_t attachment_index); // 0x4536f0, blam-cc: stack, stack, AX

void object_create_attachments(uint32_t object_index)
{
    uint8_t *obj = (uint8_t *)((object_header *)object_data->data)[object_index & 0xffff].data;
    uint8_t *definition = (uint8_t *)tag_instances[*(datum_index *)obj & 0xffff].data;
    int16_t i;

    for (i = 0; i < *(int32_t *)(definition + 0x140); i++) {
        uint8_t *attachment = *(uint8_t **)(definition + 0x144) + i * 0x48;
        datum_index tag = *(datum_index *)(attachment + 0xc);
        int16_t first_scale = (int16_t)(*(int16_t *)(attachment + 0x30) - 1);
        int16_t second_scale = (int16_t)(*(uint16_t *)(attachment + 0x32) - 1);
        int16_t change_color = (int16_t)(*(uint16_t *)(attachment + 0x34) - 1);
        int8_t type = -1;
        datum_index handle = k_datum_index_none;

        if (tag != k_datum_index_none) {
            switch (*(uint32_t *)attachment) {
            case 0x6c696768: type = 0; break; // 'ligh'
            case 0x6c736e64: type = 1; break; // 'lsnd'
            case 0x65666665: type = 2; break; // 'effe'
            case 0x636f6e74: type = 3; break; // 'cont'
            case 0x7063746c: type = 4; break; // 'pctl'
            }
        }
        switch (type) {
        case 0:
            handle = light_new_attached(tag, object_index, i, first_scale, change_color);
            if (handle != k_datum_index_none) {
                *(uint32_t *)(obj + 0x10) |= 0x100;
            }
            break;
        case 1:
            handle = looping_sound_new(object_index, tag, (char *)(attachment + 0x10), first_scale);
            if (handle != k_datum_index_none) {
                *(uint32_t *)(obj + 0x10) |= 0x400;
            }
            break;
        case 2:
            handle = effect_new_at_texture_coordinate(tag, object_index, change_color, first_scale, second_scale);
            break;
        case 3:
            handle = contrail_new(i, object_index, tag);
            break;
        case 4:
            handle = particle_system_new_on_marker(tag, object_index, i);
            break;
        }
        obj[0x144 + i] = (uint8_t)type;
        *(datum_index *)(obj + 0x14c + i * 4) = handle;
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
