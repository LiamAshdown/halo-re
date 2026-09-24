// flying_camera_attach_to_object  (Ghidra: FUN_00446470; renamed)
// address 0x446470, size 116 bytes
// name confidence: 0.5   rewrite confidence: 0.85
// evidence: out/phase4/camera_types_notes.md "0x446470: flying camera attach to object". Stores
//   the object in flying_camera_attached_object (0x00686aa0) and, when the player 0 flying
//   record exists, the offset of the camera from the object's bounding centre (object +0xa0)
//   in flying_camera_attached_offset (0x006f181c), or the global zero vector when detaching.
//   The flying pov 0x4464f0 calls it every tick after copying the render camera; the flying
//   update 0x4465d0 moves the camera with the object through that offset.
// register convention (objdump 0x446478 mov ds:0x686aa0,eax; caller 0x446556..0x446572
//   mov eax,ds:0x686aa0 / call): object handle in EAX, no stack arguments.
//   // blam-cc: EAX -> object_index
// No salt check: the object is read straight out of the object header array, as the original.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "camera.h"

extern data_array *object_data;                         // 0x008603b0, objects module
extern const real_point3d *global_origin3d_pointer;     // 0x00696714, math module
extern datum_index flying_camera_attached_object;       // 0x00686aa0
extern editor_camera_data *flying_camera_data;          // 0x006f1814
extern Vector3D flying_camera_attached_offset;          // 0x006f181c

// blam-cc: EAX -> object_index
void flying_camera_attach_to_object(datum_index object_index)
{
    flying_camera_attached_object = object_index;
    if (flying_camera_data == 0) {
        return;
    }
    if (object_index != k_datum_index_none) {
        object *attached = ((object_header *)object_data->data)[object_index & 0xffff].data;

        flying_camera_attached_offset.i = flying_camera_data->position.x - attached->bounding_center.x;
        flying_camera_attached_offset.j = flying_camera_data->position.y - attached->bounding_center.y;
        flying_camera_attached_offset.k = flying_camera_data->position.z - attached->bounding_center.z;
    } else {
        flying_camera_attached_offset = *(const Vector3D *)global_origin3d_pointer;
    }
}

#if 0
Original Ghidra decompilation (0x446470):

void FUN_00446470(void)

{
  int iVar1;
  uint in_EAX;

  DAT_00686aa0 = in_EAX;
  if (DAT_006f1814 != (float *)0x0) {
    if (in_EAX != 0xffffffff) {
      iVar1 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (in_EAX & 0xffff) * 0xc);
      _DAT_006f181c = *DAT_006f1814 - *(float *)(iVar1 + 0xa0);
      _DAT_006f1820 = DAT_006f1814[1] - *(float *)(iVar1 + 0xa4);
      _DAT_006f1824 = DAT_006f1814[2] - *(float *)(iVar1 + 0xa8);
      return;
    }
    _DAT_006f181c = *(float *)PTR_DAT_00696714;
    _DAT_006f1820 = *(float *)(PTR_DAT_00696714 + 4);
    _DAT_006f1824 = *(float *)(PTR_DAT_00696714 + 8);
  }
  return;
}
#endif
