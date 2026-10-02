// object_dump_accumulate_stats  (Ghidra: object_dump_accumulate_stats, already named)
// address 0x4fa3d0, size 185 bytes
// name confidence: 0.65 (already carries this name from an earlier phase; matches
//   functions.md's summary: "Tallies one object's statistics (size, garbage/dead/outside-map/
//   at-rest/no-physics flags) into a per-type or per-definition memory-dump accumulator")
// rewrite confidence: 0.6
// evidence: types/objects.h object_memory_dump_record (maximum_size 0x06, count 0x0c,
//   active_count 0x0e, garbage_count 0x10, dead_count 0x12, outside_map_count 0x14,
//   at_rest_count 0x16), object_header (block_size 0x06, flags 0x02 bit 0 active), object
//   (flags 0x10 with _object_in_tracked_list_bit/_object_outside_map_bit, vitality_flags 0x106
//   bit 2 = _object_health_frozen_bit's byte, parent_object 0x11c, location_cluster_index
//   0x09c); global 0x008603b0 object_data.
// register convention: object index in EAX, accumulator record in ESI. Consistent with
//   Ghidra's own "in_EAX"/"unaff_ESI", confirmed against objdump -d -M intel bin/halo.exe:
//   0x4fa3d1 mov ebx,DAT_008603b0 at entry with no stack access.
//   // blam-cc: EAX -> object_index, ESI -> record

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern data_array *object_data; // 0x008603b0

void object_dump_accumulate_stats(uint32_t object_index, object_memory_dump_record *record)
    // blam-cc: EAX -> object_index, ESI -> record
{
    object_header *header = (object_header *)object_data->data + (object_index & 0xffff);
    object *obj = header->data;

    if (record->maximum_size < header->block_size) {
        record->maximum_size = header->block_size;
    }
    record->count = record->count + 1;
    record->total_size = record->total_size + header->block_size;
    if ((header->flags & _object_header_active_bit) != 0) {
        record->active_count = record->active_count + 1;
    }
    if ((obj->flags & _object_in_tracked_list_bit) != 0) {
        record->garbage_count = record->garbage_count + 1;
    }
    if ((obj->vitality_flags & _object_health_frozen_bit) != 0) {
        record->dead_count = record->dead_count + 1;
    }
    if ((obj->flags & _object_at_rest_bit) != 0) {
        record->at_rest_count = record->at_rest_count + 1;
    }

    {
        uint32_t root = k_datum_index_none;
        uint32_t current = object_index;
        while (current != k_datum_index_none) {
            root = current;
            current = ((object_header *)object_data->data)[root & 0xffff].data->parent_object;
        }
        {
            object *root_obj = ((object_header *)object_data->data)[root & 0xffff].data;
            if (((root_obj->flags & _object_outside_map_bit) != 0) || (root_obj->location_cluster_index == -1)) {
                record->outside_map_count = record->outside_map_count + 1;
            }
        }
    }
}

#if 0
Original Ghidra decompilation (0x4fa3d0):

void object_dump_accumulate_stats(void)

{
  int iVar1;
  short sVar2;
  int iVar3;
  int iVar4;
  uint in_EAX;
  uint uVar5;
  int unaff_ESI;

  iVar4 = DAT_008603b0;
  iVar1 = *(int *)(DAT_008603b0 + 0x34) + (in_EAX & 0xffff) * 0xc;
  iVar3 = *(int *)(iVar1 + 8);
  if (*(short *)(unaff_ESI + 6) < *(short *)(iVar1 + 6)) {
    *(short *)(unaff_ESI + 6) = *(short *)(iVar1 + 6);
  }
  sVar2 = *(short *)(iVar1 + 6);
  *(short *)(unaff_ESI + 0xc) = *(short *)(unaff_ESI + 0xc) + 1;
  *(int *)(unaff_ESI + 8) = *(int *)(unaff_ESI + 8) + (int)sVar2;
  if ((*(byte *)(iVar1 + 2) & 1) != 0) {
    *(short *)(unaff_ESI + 0xe) = *(short *)(unaff_ESI + 0xe) + 1;
  }
  if ((*(uint *)(iVar3 + 0x10) & 0x10000) != 0) {
    *(short *)(unaff_ESI + 0x10) = *(short *)(unaff_ESI + 0x10) + 1;
  }
  if ((*(byte *)(iVar3 + 0x106) & 4) != 0) {
    *(short *)(unaff_ESI + 0x12) = *(short *)(unaff_ESI + 0x12) + 1;
  }
  if ((*(byte *)(iVar3 + 0x10) & 0x20) != 0) {
    *(short *)(unaff_ESI + 0x16) = *(short *)(unaff_ESI + 0x16) + 1;
  }
  uVar5 = 0xffffffff;
  if (in_EAX != 0xffffffff) {
    do {
      uVar5 = in_EAX;
      in_EAX = *(uint *)(*(int *)(*(int *)(iVar4 + 0x34) + 8 + (uVar5 & 0xffff) * 0xc) + 0x11c);
    } while (in_EAX != 0xffffffff);
  }
  iVar1 = *(int *)(*(int *)(iVar4 + 0x34) + 8 + (uVar5 & 0xffff) * 0xc);
  if (((*(uint *)(iVar1 + 0x10) & 0x200000) != 0) || (*(short *)(iVar1 + 0x9c) == -1)) {
    *(short *)(unaff_ESI + 0x14) = *(short *)(unaff_ESI + 0x14) + 1;
  }
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
