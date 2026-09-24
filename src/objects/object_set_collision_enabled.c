// object_set_collision_enabled  (Ghidra: FUN_004f6850; renamed, Blam-style, not previously named)
// address 0x4f6850, size 163 bytes
// name confidence: 0.4 (matches functions.md's summary: "Registers or unregisters an object
//   with an external visibility/collision system and updates its per-slot flag bits
//   accordingly"; the toggled object flag is exactly _object_no_collision_bit)
// rewrite confidence: 0.55
// evidence: types/objects.h object (definition_tag 0x000, flags 0x010 with
//   _object_no_collision_bit), object_header (flags at 0x02); types/tags.h Object (model
//   TagDependency); global 0x008603b0 object_data, global 0x0087bc14 tag_instances; callee
//   object_for_each_light_attachment (0x4f9a20, this batch).
// register convention: object index in EAX (in_EAX), the enable boolean is a stack byte
//   (Ghidra's "char param_1"). Confirmed against objdump -d -M intel bin/halo.exe: 0x4f6851
//   mov bl,[esp+0x8] before eax is masked into ecx as the index.
//   // blam-cc: EAX -> object_index, stack -> enable
// UNSURE: the header flags bit 0x02 this function pairs with _object_no_collision_bit has no
//   established meaning beyond types/objects.h's _object_header_unknown_02_bit.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"

extern data_array *object_data; // 0x008603b0
extern tag_instance *tag_instances; // 0x0087bc14

extern void object_for_each_light_attachment(uint32_t object_index, int32_t register_in_table, int32_t invoke_callback); // 0x4f9a20, this batch, UNSURE: parameter order/meaning inferred from the two call sites here

void object_set_collision_enabled(uint32_t object_index, uint8_t enable) // blam-cc: EAX -> object_index, stack -> enable
{
    object_header *header = (object_header *)object_data->data + (object_index & 0xffff);
    object *obj = header->data;
    Object *definition = (Object *)tag_instances[obj->definition_tag & 0xffff].data;
    int has_model = (definition->model.tag_id.index != 0xffff);
    int currently_disabled = (obj->flags & _object_no_collision_bit) != 0;
    int requesting_disabled = (enable == 0);

    if (has_model) {
        // Only tell the light-attachment table about a real transition, matching the two call
        // sites here: (0,1) when going from disabled to enabled, (1,0) the other way.
        if (currently_disabled != requesting_disabled) {
            if (enable != 0) {
                object_for_each_light_attachment(object_index, 0, 1);
            } else {
                object_for_each_light_attachment(object_index, 1, 0);
            }
        }
    } else if (enable != 0) {
        return; // no model: enabling collision on an object with none is a no-op
    }

    if (enable == 0) {
        obj->flags |= _object_no_collision_bit;
        header->flags &= (uint8_t)~0x02;
    } else {
        obj->flags &= ~(uint32_t)_object_no_collision_bit;
        header->flags |= 0x02;
    }
}

#if 0
Original Ghidra decompilation (0x4f6850):

void FUN_004f6850(char param_1)

{
  uint *puVar1;
  int iVar2;
  uint in_EAX;
  int iVar3;

  iVar3 = (in_EAX & 0xffff) * 0xc;
  puVar1 = *(uint **)(*(int *)(DAT_008603b0 + 0x34) + 8 + iVar3);
  iVar2 = *(int *)((*puVar1 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
  if (*(int *)(iVar2 + 0x34) == -1) {
LAB_004f68a6:
    if (param_1 == '\0') goto LAB_004f68b0;
  }
  else {
    if (((puVar1[4] & 1) != 0) && (param_1 != '\0')) {
      object_for_each_light_attachment(0,1);
      goto LAB_004f68a6;
    }
    if ((puVar1[4] & 1) != 0) goto LAB_004f68a6;
    if (param_1 == '\0') {
      object_for_each_light_attachment(1,0);
      goto LAB_004f68b0;
    }
  }
  if (*(int *)(iVar2 + 0x34) == -1) {
    return;
  }
LAB_004f68b0:
  iVar3 = *(int *)(DAT_008603b0 + 0x34) + iVar3;
  if (param_1 == '\0') {
    puVar1[4] = puVar1[4] | 1;
    *(byte *)(iVar3 + 2) = *(byte *)(iVar3 + 2) & 0xfd;
    return;
  }
  puVar1[4] = puVar1[4] & 0xfffffffe;
  *(byte *)(iVar3 + 2) = *(byte *)(iVar3 + 2) | 2;
  return;
}
#endif
