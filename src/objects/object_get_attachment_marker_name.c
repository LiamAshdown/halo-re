// object_get_attachment_marker_name
// address 0x4f6030, size 74 bytes
// name confidence: 0.3 (still FUN_004f6030 in Ghidra; functions.md's summary -- "Given an
//   object index and a node index, returns a pointer to that node's entry in the object's model
//   tag definition" -- conf=0.25 and does not match the code: the offsets touched, 0x140/0x144
//   with a 0x48 element stride, are the Object tag's attachments TagReflexive and
//   ObjectAttachment's documented size, not a model/node table; renamed accordingly)
// rewrite confidence: 0.45
// evidence: types/objects.h object (definition_tag at 0x000); types/tags.h Object.attachments
//   (TagReflexive at 0x140, matching types/objects.h's own "attachments 0x140/0x144" note) and
//   ObjectAttachment (0x48); types/cache.h tag_instance. ObjectAttachment starts with a
//   TagDependency (0x10 bytes) followed by the TagString marker name, so the "+0x10" IS the
//   offset of ObjectAttachment.marker -- the function returns a marker NAME, which is exactly
//   what its one caller (object_light_recompute_transform, 0x4f2a2f) feeds to
//   object_get_node_local_transform as its marker_name argument. Renamed from
//   object_get_attachment_tag_entry accordingly.
// register convention: object index in EAX (in_EAX), attachment index in EDX (in_DX).

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"

extern data_array *object_data; // 0x008603b0
extern tag_instance *tag_instances; // 0x0087bc14

char *object_get_attachment_marker_name(uint32_t object_index, int16_t attachment_index)
    // blam-cc: EAX -> object_index, EDX -> attachment_index
{
    object *obj = ((object_header *)object_data->data)[object_index & 0xffff].data;
    Object *object_tag = (Object *)tag_instances[obj->definition_tag & 0xffff].data;

    if (attachment_index >= 0 && attachment_index < (int32_t)object_tag->attachments.count) {
        return (char *)object_tag->attachments.pointer + 0x10 + attachment_index * 0x48;
    }
    return 0;
}

#if 0
Original Ghidra decompilation (0x4f6030):

int FUN_004f6030(void)

{
  int iVar1;
  uint in_EAX;
  short in_DX;

  iVar1 = *(int *)((**(uint **)(*(int *)(DAT_008603b0 + 0x34) + 8 + (in_EAX & 0xffff) * 0xc) &
                   0xffff) * 0x20 + 0x14 + DAT_0087bc14);
  if ((-1 < in_DX) && ((int)in_DX < *(int *)(iVar1 + 0x140))) {
    return *(int *)(iVar1 + 0x144) + 0x10 + in_DX * 0x48;
  }
  return 0;
}
#endif
