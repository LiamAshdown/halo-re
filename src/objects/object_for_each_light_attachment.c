// object_for_each_light_attachment  (Ghidra: object_for_each_light_attachment, already named)
// address 0x4f9a20, size 146 bytes
// name confidence: 0.85 (already carries this name from an earlier phase; matches
//   functions.md's summary: "Iterates an object's light attachments, optionally registering
//   each in the active-light table and/or invoking a per-light callback")
// rewrite confidence: 0.55
// evidence: types/objects.h object (flags 0x10, attachment_types 0x144, attachment_handles
//   0x14c, object_attachment_type enum); types/tags.h Object.attachments; global 0x008603b0
//   object_data, 0x0087bc14 tag_instances; callees object_light_clear_dirty_flag (0x4f29c0,
//   established: EAX -> light_index, despite the inherited "chimera__light_table" name -- see
//   out/phase4/objects_types_notes.md's Chimera-label note), object_light_recompute_transform
//   (0x4f2a00, established: stack -> light_index).
// register convention: object index in EAX, the two bool flags are stack parameters. Consistent
//   with Ghidra's own "object_for_each_light_attachment(char param_1,char param_2)" plus
//   "in_EAX".
//   // blam-cc: EAX -> object_index, stack -> register_in_table, invoke_callback
// UNSURE: object flags bit 0x100 (gating the whole sweep) is not in types/objects.h's
//   object_flags enum; preserved as raw hex (this is the same bit object_create_attachments.c,
//   this batch, sets on a successful light attachment).

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"

extern data_array *object_data; // 0x008603b0
extern tag_instance *tag_instances; // 0x0087bc14

extern void object_light_clear_dirty_flag(uint32_t light_index); // 0x4f29c0, EAX -> light_index
extern void object_light_recompute_transform(uint32_t light_index); // 0x4f2a00, stack -> light_index

void object_for_each_light_attachment(uint32_t object_index, int32_t register_in_table, int32_t invoke_callback)
    // blam-cc: EAX -> object_index, stack -> register_in_table, invoke_callback
{
    object *obj = ((object_header *)object_data->data)[object_index & 0xffff].data;

    if ((obj->flags & 0x100) != 0) { // UNSURE: see file header
        Object *definition = (Object *)tag_instances[obj->definition_tag & 0xffff].data;
        int16_t i;

        for (i = 0; i < (int16_t)definition->attachments.count; i++) {
            if ((obj->attachment_types[i] == _object_attachment_type_light) &&
                (obj->attachment_handles[i] != k_datum_index_none)) {
                if (register_in_table != 0) {
                    object_light_clear_dirty_flag(obj->attachment_handles[i]);
                }
                if (invoke_callback != 0) {
                    object_light_recompute_transform(obj->attachment_handles[i]);
                }
            }
        }
    }
}

#if 0
Original Ghidra decompilation (0x4f9a20):

void object_for_each_light_attachment(char param_1,char param_2)

{
  uint *puVar1;
  int iVar2;
  uint in_EAX;
  short sVar3;
  int iVar4;

  puVar1 = *(uint **)(*(int *)(DAT_008603b0 + 0x34) + 8 + (in_EAX & 0xffff) * 0xc);
  if ((puVar1[4] & 0x100) != 0) {
    iVar2 = *(int *)((*puVar1 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
    sVar3 = 0;
    if (0 < *(int *)(iVar2 + 0x140)) {
      iVar4 = 0;
      do {
        if ((*(char *)(iVar4 + 0x144 + (int)puVar1) == '\0') && (puVar1[iVar4 + 0x53] != 0xffffffff)
           ) {
          if (param_1 != '\0') {
            chimera__light_table();
          }
          if (param_2 != '\0') {
            FUN_004f2a00(puVar1[iVar4 + 0x53]);
          }
        }
        sVar3 = sVar3 + 1;
        iVar4 = (int)sVar3;
      } while (iVar4 < *(int *)(iVar2 + 0x140));
    }
  }
  return;
}
#endif
