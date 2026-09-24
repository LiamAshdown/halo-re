// object_get_controlling_player_index
// address 0x4ee2e0, size 135 bytes
// name confidence: 0.5 (Ghidra-recovered name, matches out/phase4/objects_functions.md summary)
// rewrite confidence: 0.55
// evidence: types/objects.h object_header (identifier 0x00, flags 0x02, type 0x03, data 0x08),
// object.parent_object (0x11c); types/memory.h data_array (maximum_count 0x20, size 0x22,
// data 0x34).
// UNSURE: player_index_from_unit_index is not in this module's address range; assumed (from this call and its
// use in object_update_vitality_and_regeneration) to resolve a validated biped/vehicle datum to
// its controlling player index.
// register convention: datum_index object_index in EAX (in_EAX).
// blam-cc: EAX=object_index

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"

extern data_array *object_data; // 0x008603b0

extern int32_t player_index_from_unit_index(datum_index object_index); // UNSURE: out of range, 0x474db0

int32_t object_get_controlling_player_index(datum_index object_index)
{
    object_header *headers = (object_header *)object_data->data;

    if (object_index == (datum_index)0xffffffff) {
        return -1;
    }

    for (;;) {
        int16_t index = (int16_t)object_index;

        if (-1 < index && index < object_data->maximum_count) {
            object_header *header = &headers[(uint16_t)index];

            if (header->identifier != 0) {
                int16_t salt = (int16_t)(object_index >> 0x10);

                if ((salt == 0 || header->identifier == salt) &&
                    (1 << (header->type & 0x1f) & _object_mask_unit) != 0 &&
                    header->data != 0) {
                    return player_index_from_unit_index(object_index);
                }
            }
        }

        object_index = headers[object_index & 0xffff].data->parent_object;
        if (object_index == (datum_index)0xffffffff) {
            return -1;
        }
    }
}

#if 0
Original Ghidra decompilation (0x4ee2e0):

undefined4 object_get_controlling_player_index(void)

{
  uint in_EAX;
  undefined4 uVar1;
  short *psVar2;
  short sVar3;

  if (in_EAX == 0xffffffff) {
    return 0xffffffff;
  }
  do {
    sVar3 = (short)in_EAX;
    if ((-1 < sVar3) && (sVar3 < *(short *)(DAT_008603b0 + 0x20))) {
      psVar2 = (short *)((int)*(short *)(DAT_008603b0 + 0x22) * (int)sVar3 +
                        *(int *)(DAT_008603b0 + 0x34));
      if (((*psVar2 != 0) &&
          (((sVar3 = (short)(in_EAX >> 0x10), sVar3 == 0 || (*psVar2 == sVar3)) &&
           ((1 << (*(byte *)((int)psVar2 + 3) & 0x1f) & 3U) != 0)))) && (*(int *)(psVar2 + 4) != 0))
      {
        uVar1 = FUN_00474db0(in_EAX);
        return uVar1;
      }
    }
    in_EAX = *(uint *)(*(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (in_EAX & 0xffff) * 0xc) + 0x11c
                      );
    if (in_EAX == 0xffffffff) {
      return 0xffffffff;
    }
  } while( true );
}
#endif
