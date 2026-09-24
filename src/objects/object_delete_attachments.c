// object_delete_attachments  (Ghidra: object_delete_attachments, already named)
// address 0x4f9900, size 214 bytes
// name confidence: 0.65 (already carries this name from an earlier phase; matches
//   functions.md's summary: "Releases all runtime attachment instances (light/looping-sound/
//   effect/contrail/particle) previously created for an object")
// rewrite confidence: 0.4
// evidence: types/objects.h object (attachment_types 0x144, attachment_handles 0x14c,
//   object_attachment_type enum); global 0x008603b0 object_data, 0x0087bc14 tag_instances;
//   callees light_delete (0x4f0bd0, established: ESI -> light_handle),
//   object_recalculate_bounding_radius (0x4f8310, established), datum_delete (established).
// register convention: object index in EBX. Confirmed against objdump -d -M intel bin/halo.exe:
//   0x4f9912 mov eax,ebx at entry, with EBX never otherwise assigned in the function body.
//   // blam-cc: EBX -> object_index
// UNSURE: DAT_007461a0 (a looping-sound data_array) and DAT_0087abd4 (a stride-0x158 foreign
//   data_array used for the particle-system case) are not otherwise established in this
//   module; effect_delete and contrail_advance are foreign, unexamined callees.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"

extern data_array *object_data; // 0x008603b0
extern tag_instance *tag_instances; // 0x0087bc14
extern data_array *looping_sound_data_007461a0; // 0x007461a0, UNSURE: see file header
extern data_array *unknown_data_0087abd4; // 0x0087abd4, UNSURE: see file header

extern void light_delete(datum_index light_handle); // 0x4f0bd0, ESI -> light_handle
extern void datum_delete(data_array *array, datum_index handle); // memory module
extern void effect_delete(datum_index handle); // 0x450be0, foreign module
extern void object_recalculate_bounding_radius(uint32_t object_index); // 0x4f8310, established
extern void contrail_advance(int32_t a, int32_t b); // 0x44ca60, foreign module, UNSURE: args guessed from the literal (1,0) at this call site

void object_delete_attachments(uint32_t object_index) // blam-cc: EBX -> object_index
{
    object *obj = ((object_header *)object_data->data)[object_index & 0xffff].data;
    Object *definition = (Object *)tag_instances[obj->definition_tag & 0xffff].data;
    int16_t i;

    for (i = 0; i < (int16_t)definition->attachments.count; i++) {
        int8_t type;
        datum_index handle;

        type = obj->attachment_types[i];
        handle = obj->attachment_handles[i];
        if ((type != -1) && (handle != k_datum_index_none)) {
            switch (type) {
                case _object_attachment_type_light:
                    light_delete(handle);
                    break;
                case _object_attachment_type_looping_sound:
                    datum_delete(looping_sound_data_007461a0, handle);
                    break;
                case _object_attachment_type_effect:
                    effect_delete(handle);
                    break;
                case _object_attachment_type_contrail:
                    object_recalculate_bounding_radius(object_index);
                    contrail_advance(1, 0);
                    break;
                case _object_attachment_type_particle_system: {
                    // UNSURE: inlined hash-table-style unlink into unknown_data_0087abd4
                    // (stride 0x158), preserved as raw offsets; see file header.
                    uint8_t *entry = (uint8_t *)unknown_data_0087abd4->data + (handle & 0xffff) * 0x158;
                    *(uint32_t *)(entry + 4) &= ~1u;
                    *(int32_t *)(entry + 0xc) = -1;
                    break;
                }
            }
        }
    }
}

#if 0
Original Ghidra decompilation (0x4f9900):

void object_delete_attachments(void)

{
  char cVar1;
  uint *puVar2;
  int iVar3;
  uint particle_system_index;
  short sVar4;
  int iVar5;
  uint unaff_EBX;
  int iVar6;

  puVar2 = *(uint **)(*(int *)(DAT_008603b0 + 0x34) + 8 + (unaff_EBX & 0xffff) * 0xc);
  iVar3 = *(int *)((*puVar2 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
  iVar6 = 0;
  sVar4 = 0;
  if (0 < *(int *)(iVar3 + 0x140)) {
    do {
      cVar1 = *(char *)(iVar6 + 0x144 + (int)puVar2);
      if ((cVar1 != -1) &&
         (particle_system_index = puVar2[iVar6 + 0x53], particle_system_index != 0xffffffff)) {
        switch(cVar1) {
        case '\0':
          FUN_004f0bd0();
          break;
        case '\x01':
          datum_delete();
          break;
        case '\x02':
          particle_system_delete_450be0(particle_system_index);
          break;
        case '\x03':
          object_recalculate_bounding_radius();
          FUN_0044ca60(1,0);
          break;
        case '\x04':
          iVar6 = (particle_system_index & 0xffff) * 0x158;
          iVar5 = iVar6 + *(int *)(DAT_0087abd4 + 0x34);
          *(uint *)(iVar5 + 4) = *(uint *)(iVar6 + 4 + *(int *)(DAT_0087abd4 + 0x34)) & 0xfffffffe;
          *(undefined4 *)(iVar5 + 0xc) = 0xffffffff;
        }
      }
      sVar4 = sVar4 + 1;
      iVar6 = (int)sVar4;
    } while (iVar6 < *(int *)(iVar3 + 0x140));
  }
  return;
}
#endif
