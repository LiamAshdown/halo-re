// object_delete_recursive
// address 0x4f59d0, size 197 bytes
// name confidence: 0.75 (still FUN_004f59d0 in Ghidra; types/objects.h's
//   _object_header_delete_pending_bit comment names this function directly: "object_delete_
//   recursive sets it"; matches functions.md's summary)
// rewrite confidence: 0.7 (raised from 0.5 by the phase-4 review pass: the calling convention, object_for_each_light_attachment's argument list and object_release_render_cache_slot were all resolved from the disassembly)
// evidence: types/objects.h object_header (flags at 0x02), object (first_child_object at 0x118,
//   next_object at 0x114, flags at 0x10 with _object_no_collision_bit, definition_tag at 0x000);
//   types/tags.h Object.model.tag_id; global 0x008603b0 object_data; global 0x0087bc14
//   tag_instances; callee object_for_each_light_attachment 0x4f9a20.
// register convention: plain cdecl, both arguments on the stack. Confirmed against
//   `objdump -d -M intel bin/halo.exe` 0x4f59d0: the entry is `push ebx / mov ebx,[esp+8]`,
//   so the object index is the first stack argument, and the sibling flag is read back at
//   0x4f5a09 as `mov al,[esp+0x18]` -- the second stack argument, past the four saved
//   registers. (Its own callers agree: object_delete 0x4f5bd0 does `push 0 / push edi /
//   call 0x4f59d0 / add esp,8`.) The earlier claim that this function takes EAX/ECX register
//   arguments is contradicted by that disassembly and is corrected here.
//   // blam-cc: stack -> object_index, recurse_siblings

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"

#define TAG_ID_AS_DATUM_INDEX(field) (*(datum_index *)&(field)) // see object_new_with_datum_role_control.c

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern data_array *object_data; // 0x008603b0
extern tag_instance *tag_instances; // 0x0087bc14

extern void object_for_each_light_attachment(uint32_t object_index, int32_t register_in_table,
                                             int32_t invoke_callback); // 0x4f9a20, EAX -> object_index
extern void object_release_render_cache_slot(uint32_t object_index); // 0x4f9b00, EDI -> object_index

void object_delete_recursive(uint32_t object_index, uint8_t recurse_siblings)
    // blam-cc: stack -> object_index, recurse_siblings
{
    object_header *header = (object_header *)object_data->data + (object_index & 0xffff);
    object *obj = header->data;
    Object *object_tag;

    if (obj->first_child_object != k_datum_index_none) {
        object_delete_recursive(obj->first_child_object, 1);
    }
    if (recurse_siblings != 0 && obj->next_object != k_datum_index_none) {
        object_delete_recursive(obj->next_object, 1);
    }

    header->flags |= _object_header_delete_pending_bit;

    header = (object_header *)object_data->data + (object_index & 0xffff);
    obj = header->data;
    object_tag = (Object *)tag_instances[obj->definition_tag & 0xffff].data;
    if (TAG_ID_AS_DATUM_INDEX(object_tag->model.tag_id) != k_datum_index_none &&
        (obj->flags & _object_no_collision_bit) == 0) {
        // 0x4f5a5e..0x4f5a64: `push 0 / push 1 / mov eax,ebx / call 0x4f9a20` -- the object
        // index travels in EAX, the two literals are the stack arguments.
        object_for_each_light_attachment(object_index, 1, 0);
    }

    header = (object_header *)object_data->data + (object_index & 0xffff);
    obj->flags |= _object_no_collision_bit;
    header->flags &= (uint8_t)~_object_header_unknown_02_bit;

    // 0x4f5a86: `mov edi,ebx` immediately before the call -- the object index is the EDI
    // argument object_release_render_cache_slot documents for itself.
    object_release_render_cache_slot(object_index);
}

#if 0
Original Ghidra decompilation (0x4f59d0):

void FUN_004f59d0(uint param_1,char param_2)

{
  int iVar1;
  int iVar2;
  int iVar3;
  uint *puVar4;
  int iVar5;

  iVar5 = (param_1 & 0xffff) * 0xc;
  iVar1 = *(int *)(DAT_008603b0 + 0x34) + iVar5;
  iVar2 = *(int *)(iVar1 + 8);
  iVar3 = *(int *)(iVar2 + 0x118);
  if (iVar3 != -1) {
    FUN_004f59d0(iVar3,1);
  }
  if ((param_2 != '\0') && (iVar2 = *(int *)(iVar2 + 0x114), iVar2 != -1)) {
    FUN_004f59d0(iVar2,1);
  }
  iVar2 = DAT_008603b0;
  *(byte *)(iVar1 + 2) = *(byte *)(iVar1 + 2) | 8;
  puVar4 = *(uint **)(*(int *)(iVar2 + 0x34) + 8 + iVar5);
  if ((*(int *)(*(int *)((*puVar4 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14) + 0x34) != -1) &&
     ((puVar4[4] & 1) == 0)) {
    object_for_each_light_attachment(1,0);
  }
  iVar5 = *(int *)(DAT_008603b0 + 0x34) + iVar5;
  puVar4[4] = puVar4[4] | 1;
  *(byte *)(iVar5 + 2) = *(byte *)(iVar5 + 2) & 0xfd;
  FUN_004f9b00();
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
