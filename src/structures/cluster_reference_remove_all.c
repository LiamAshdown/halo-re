// cluster_reference_remove_all  (Ghidra: FUN_00552020; named here, still referenced as
//   FUN_00552020 by the four consuming files in src/objects/ that predate this rewrite)
// address 0x552020, size 137 bytes
// name confidence: 0.6 (still FUN_00552020 upstream)   rewrite confidence: 0.7
// evidence: src/objects/light_delete.c, object_lights_update_all.c,
//   object_light_clear_dirty_flag.c and object_unlink_cluster_or_notify_parent.c already resolved
//   and documented the stack arguments (handle, link) and the EBX cluster_list register argument.
//   This file resolves the remaining detail those callers treat as opaque: the two datum_index
//   arrays EBX addresses, confirmed by disassembly (objdump -d -M intel bin/halo.exe,
//   0x552020..0x5520a8):
//     `mov eax,[ebx+8]` feeds the first datum_delete (array = cluster_list->object_cluster_references,
//       the same slot cluster_reference_add_within_radius (0x551f00) allocates the object's own
//       chain entries from); `mov eax,[ebx+4]` feeds the second (cluster_object_references, the
//       per-cluster chain). `mov ecx,[ebx]` is cluster_list->cluster_first. See
//       cluster_partition_new.c for the full cluster_reference_group derivation.
// register convention: stack -> handle, link; EBX -> cluster_list.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "structures.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern void datum_delete(data_array *array, datum_index handle); // 0x4d0510, memory module

// Walks the object's own per-cluster reference chain (headed by `*link`), and for each cluster it
// names, also finds and unlinks the matching entry in that cluster's own chain of referencing
// objects (the one whose object_index equals `handle`). Deletes every datum visited from its pool
// and leaves `*link` set to k_datum_index_none.
void cluster_reference_remove_all(uint32_t handle, datum_index *link, cluster_reference_group *cluster_list)
    // blam-cc: EBX -> cluster_list
{
    datum_index entry = *link;

    while (entry != k_datum_index_none) {
        object_cluster_reference *own_ref = (object_cluster_reference *)
            cluster_list->object_cluster_references->data + (entry & 0xffff);
        int16_t cluster = (int16_t)own_ref->object_index; // reused generically: holds the cluster index
        datum_index next_entry;

        datum_delete(cluster_list->object_cluster_references, entry);

        {
            datum_index *scan = &cluster_list->cluster_first[cluster];
            if (*scan != k_datum_index_none) {
                do {
                    object_cluster_reference *cluster_ref = (object_cluster_reference *)
                        cluster_list->cluster_object_references->data + (*scan & 0xffff);
                    if (cluster_ref->object_index == handle) {
                        datum_delete(cluster_list->cluster_object_references, *scan);
                        *scan = cluster_ref->next_reference;
                        break;
                    }
                    scan = &cluster_ref->next_reference;
                } while (*scan != k_datum_index_none);
            }
        }

        // datum_delete only clears the datum's identifier word, so own_ref->next_reference
        // (never touched by it) is still valid to read here, exactly as the original does.
        next_entry = own_ref->next_reference;
        entry = next_entry;
    }

    *link = k_datum_index_none;
}

#if 0
Original Ghidra decompilation (0x552020):

void FUN_00552020(int param_1,uint *param_2)

{
  int iVar1;
  short sVar2;
  uint uVar3;
  int iVar4;
  int *unaff_EBX;
  uint *puVar5;

  uVar3 = *param_2;
  do {
    if (uVar3 == 0xffffffff) {
      *param_2 = 0xffffffff;
      return;
    }
    iVar1 = *(int *)(unaff_EBX[2] + 0x34) + (uVar3 & 0xffff) * 0xc;
    sVar2 = *(short *)(iVar1 + 4);
    datum_delete();
    iVar4 = (int)sVar2;
    puVar5 = (uint *)(*unaff_EBX + iVar4 * 4);
    if (*(int *)(*unaff_EBX + iVar4 * 4) != -1) {
      do {
        iVar4 = *(int *)(unaff_EBX[1] + 0x34) + (*puVar5 & 0xffff) * 0xc;
        if (*(int *)(iVar4 + 4) == param_1) {
          datum_delete();
          *puVar5 = *(uint *)(iVar4 + 8);
          break;
        }
        puVar5 = (uint *)(iVar4 + 8);
      } while (*(int *)(iVar4 + 8) != -1);
    }
    uVar3 = *(uint *)(iVar1 + 8);
  } while( true );
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
