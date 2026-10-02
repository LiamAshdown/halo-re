// object_collect_in_clusters  (Ghidra: object_collect_in_clusters, already named)
// address 0x4f7180, size 477 bytes
// name confidence: 0.8 (already carries this name from an earlier phase; matches
//   functions.md's summary: "Gathers the unique set of objects referenced by a list of BSP
//   clusters into an output array")
// rewrite confidence: 0.55
// evidence: types/objects.h object (cluster_stamp 0x014), object_cluster_reference
//   (object_index 0x04, next_reference 0x08), object_globals (collecting_in_clusters 0x01);
//   global 0x008603b0 object_data, 0x008603c0 noncollideable_cluster_first, 0x008603c4
//   noncollideable_object_references, 0x008603cc object_cluster_stamp, 0x008603d0
//   collideable_cluster_first, 0x008603d4 collideable_object_references, 0x006b8cbc
//   object_globals_pointer.
// register convention: all five parameters are plain STACK arguments (Ghidra's own signature).
//   Confirmed against objdump -d -M intel bin/halo.exe: 0x4f7180 mov ecx,[esp+0x4] reads the
//   first stack slot before any register is otherwise touched.
// UNSURE: Ghidra types this "void", but both return paths leave the running count in EAX/AX
//   with nothing overwriting it afterward (confirmed at 0x4f7342 and the max-output jump at
//   0x4f724f, both landing with ax = the loop's live count register), and
//   object_find_in_sphere (this batch) consumes the value as a short, so the return type is
//   corrected to int16_t here.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern data_array *object_data; // 0x008603b0
extern object_globals *object_globals_pointer; // 0x006b8cbc
extern int32_t object_cluster_stamp; // 0x008603cc
extern datum_index *noncollideable_cluster_first; // 0x008603c0
extern data_array *noncollideable_object_references; // 0x008603c4
extern datum_index *collideable_cluster_first; // 0x008603d0
extern data_array *collideable_object_references; // 0x008603d4

int16_t object_collect_in_clusters(uint32_t search_mask, int16_t cluster_count,
    int16_t *cluster_indices, int16_t max_output, datum_index *out_objects)
{
    int16_t count = 0;
    int32_t stamp;
    int16_t i;

    if (search_mask == 0) {
        search_mask = 0xffffffff;
    }
    object_globals_pointer->collecting_in_clusters = 1;
    object_cluster_stamp = object_cluster_stamp + 1;
    stamp = object_cluster_stamp;

    for (i = 0; i < cluster_count; i++) {
        int16_t cluster_index = cluster_indices[i];

        if ((search_mask & 1) != 0) {
            datum_index ref = collideable_cluster_first[cluster_index];
            while (ref != k_datum_index_none) {
                object_cluster_reference *node = (object_cluster_reference *)
                    collideable_object_references->data + (ref & 0xffff);
                datum_index object_index = node->object_index;
                object *obj = ((object_header *)object_data->data)[object_index & 0xffff].data;
                if (obj->cluster_stamp != stamp) {
                    obj->cluster_stamp = stamp;
                    if (max_output <= count) {
                        object_globals_pointer->collecting_in_clusters = 0;
                        return count;
                    }
                    out_objects[count] = object_index;
                    count++;
                }
                ref = node->next_reference;
            }
        }

        if ((search_mask & 2) != 0) {
            datum_index ref = noncollideable_cluster_first[cluster_index];
            while (ref != k_datum_index_none) {
                object_cluster_reference *node = (object_cluster_reference *)
                    noncollideable_object_references->data + (ref & 0xffff);
                datum_index object_index = node->object_index;
                object *obj = ((object_header *)object_data->data)[object_index & 0xffff].data;
                if (obj->cluster_stamp != stamp) {
                    obj->cluster_stamp = stamp;
                    if (max_output <= count) {
                        object_globals_pointer->collecting_in_clusters = 0;
                        return count;
                    }
                    out_objects[count] = object_index;
                    count++;
                }
                ref = node->next_reference;
            }
        }
    }

    object_globals_pointer->collecting_in_clusters = 0;
    return count;
}

#if 0
Original Ghidra decompilation (0x4f7180):

void object_collect_in_clusters(uint param_1,short param_2,int param_3,short param_4,int param_5)

{
  int iVar1;
  short sVar2;
  uint uVar3;
  short sVar4;
  short sVar5;
  uint uVar6;
  int iVar7;
  int iVar8;

  sVar4 = 0;
  if (param_1 == 0) {
    param_1 = 0xffffffff;
  }
  *(undefined1 *)(DAT_006b8cbc + 1) = 1;
  sVar5 = 0;
  iVar8 = DAT_008603cc + 1;
  DAT_008603cc = iVar8;
  if (0 < param_2) {
    iVar7 = DAT_008603b0;
    do {
      sVar2 = *(short *)(param_3 + sVar5 * 2);
      if ((param_1 & 1) != 0) {
        uVar3 = *(uint *)(DAT_008603d0 + sVar2 * 4);
        if (uVar3 == 0xffffffff) {
          uVar6 = 0xffffffff;
          uVar3 = 0xffffffff;
        }
        else {
          iVar1 = *(int *)(DAT_008603d4 + 0x34) + (uVar3 & 0xffff) * 0xc;
          uVar3 = *(uint *)(iVar1 + 8);
          uVar6 = *(uint *)(iVar1 + 4);
        }
        while (uVar6 != 0xffffffff) {
          iVar7 = *(int *)(*(int *)(iVar7 + 0x34) + 8 + (uVar6 & 0xffff) * 0xc);
          if (*(int *)(iVar7 + 0x14) != iVar8) {
            *(int *)(iVar7 + 0x14) = iVar8;
            if (param_4 <= sVar4) goto LAB_004f7342;
            *(uint *)(param_5 + sVar4 * 4) = uVar6;
            sVar4 = sVar4 + 1;
          }
          iVar7 = DAT_008603b0;
          if (uVar3 == 0xffffffff) {
            uVar6 = 0xffffffff;
            uVar3 = 0xffffffff;
          }
          else {
            iVar1 = *(int *)(DAT_008603d4 + 0x34) + (uVar3 & 0xffff) * 0xc;
            uVar3 = *(uint *)(iVar1 + 8);
            uVar6 = *(uint *)(iVar1 + 4);
          }
        }
      }
      if ((param_1 & 2) != 0) {
        uVar3 = *(uint *)(DAT_008603c0 + sVar2 * 4);
        if (uVar3 == 0xffffffff) {
          uVar6 = 0xffffffff;
          uVar3 = 0xffffffff;
        }
        else {
          iVar1 = *(int *)(DAT_008603c4 + 0x34) + (uVar3 & 0xffff) * 0xc;
          uVar3 = *(uint *)(iVar1 + 8);
          uVar6 = *(uint *)(iVar1 + 4);
        }
        while (uVar6 != 0xffffffff) {
          iVar1 = *(int *)(*(int *)(iVar7 + 0x34) + 8 + (uVar6 & 0xffff) * 0xc);
          if (*(int *)(iVar1 + 0x14) != iVar8) {
            *(int *)(iVar1 + 0x14) = iVar8;
            if (param_4 <= sVar4) {
              *(undefined1 *)(DAT_006b8cbc + 1) = 0;
              return;
            }
            *(uint *)(param_5 + sVar4 * 4) = uVar6;
            sVar4 = sVar4 + 1;
          }
          if (uVar3 == 0xffffffff) {
            uVar6 = 0xffffffff;
            uVar3 = 0xffffffff;
          }
          else {
            iVar1 = *(int *)(DAT_008603c4 + 0x34) + (uVar3 & 0xffff) * 0xc;
            uVar3 = *(uint *)(iVar1 + 8);
            uVar6 = *(uint *)(iVar1 + 4);
          }
        }
      }
      sVar5 = sVar5 + 1;
    } while (sVar5 < param_2);
  }
LAB_004f7342:
  *(undefined1 *)(DAT_006b8cbc + 1) = 0;
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
