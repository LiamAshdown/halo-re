// unit_check_fell_off_level  (Ghidra: unit_check_fell_off_level, renamed)
// address 0x55e4a0, size 78 bytes
// name confidence: 0.35   rewrite confidence: 0.55
// evidence: object.flags/location_cluster_index/position (0x010/0x09c/0x05c, objects.h);
//   0x006f1d20 is "the network / predicted-state flag every damage and seat path branches on"
//   (types/units.h globals).

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"
#include "units.h"

extern data_array *object_data;  // 0x008603b0
extern int32_t network_predicted_state_flag;  // 0x006f1d20, types/units.h

extern void object_delete(uint32_t object_index); // 0x4f5bd0, UNSURE exact signature

// Detects when a unit has fallen far below the level (Z < -2000) while marked deleted-pending or
// outside any BSP cluster, and deletes it.
void unit_check_fell_off_level(uint32_t object_index)
{
    object *obj = ((object_header *)object_data->data)[object_index & 0xffff].data;

    if (network_predicted_state_flag == 0 &&
        ((obj->flags & 0x200000) != 0 || obj->location_cluster_index == -1)) {
        if (obj->position.z < -2000.0f) {
            object_delete(object_index);
        }
    }
}

#if 0
Original Ghidra decompilation (0x55e4a0):

uint FUN_0055e4a0(void)

{
  float fVar1;
  uint uVar2;
  uint in_ECX;

  uVar2 = *(uint *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (in_ECX & 0xffff) * 0xc);
  if ((DAT_006f1d20 == 0) &&
     (((*(uint *)(uVar2 + 0x10) & 0x200000) != 0 || (*(short *)(uVar2 + 0x9c) == -1)))) {
    fVar1 = *(float *)(uVar2 + 100);
    uVar2 = CONCAT22((short)(uVar2 >> 0x10),
                     (ushort)(fVar1 < -2000.0) << 8 | (ushort)NAN(fVar1) << 10 |
                     (ushort)(fVar1 == -2000.0) << 0xe);
    if (fVar1 < -2000.0) {
      uVar2 = object_delete();
    }
  }
  return uVar2 & 0xffffff00;
}
#endif
