// object_get_hud_text_message_index  (Ghidra: FUN_004a9b40, renamed; earlier
// hud_weapon_interface_tag_for_object)
// address 0x4a9b40, size 58 bytes
// name confidence: 0.7 (chosen)   rewrite confidence: 0.9
// evidence: offset 0x13c of an object tag is Object::hud_text_message_index in types/tags.h
// (0x110 pad block of 44 bytes ends there); the value is sign-extended (movsx) and passed as a
// hud message string argument by hud_update_interaction_prompt @0x4a9b80. It is not a weapon HUD
// interface tag index. Checked against objdump: -1 in gives -1 out (the "or eax,eax" return).
// register convention: object index in EAX.
//   // blam-cc: object_index -> EAX

#include "tags.h"
#include "memory.h"
#include "cache.h"
#include "math.h"
#include "objects.h"
#include "game.h"
#include "networking.h"
#include "interface.h"
#include "fn_interface.h"

extern data_array *object_data; // 0x008603b0
extern tag_instance *tag_instances; // 0x0087bc14

// blam-cc: object_index -> EAX
int16_t object_get_hud_text_message_index(datum_index object_index)
{
    struct object *obj;
    Object *definition;

    if (object_index == (datum_index)-1) {
        return -1;
    }
    obj = ((object_header *)object_data->data)[object_index & 0xffff].data;
    definition = (Object *)tag_instances[obj->definition_tag & 0xffff].data;
    return definition->hud_text_message_index;
}

#if 0
Original Ghidra decompilation (0x4a9b40):

int FUN_004a9b40(void)

{
  uint in_EAX;

  if (in_EAX == 0xffffffff) {
    return -1;
  }
  return (int)*(short *)(*(int *)((**(uint **)(*(int *)(DAT_008603b0 + 0x34) + 8 +
                                              (in_EAX & 0xffff) * 0xc) & 0xffff) * 0x20 + 0x14 +
                                 DAT_0087bc14) + 0x13c);
}
#endif
