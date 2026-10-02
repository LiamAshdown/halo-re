// cluster_reference_add_within_radius  (Ghidra: FUN_00551f00; named here, still referenced as
//   FUN_00551f00 by the three consuming files in src/objects/ that predate this rewrite)
// address 0x551f00, size 279 bytes
// name confidence: 0.6 (still FUN_00551f00 upstream)   rewrite confidence: 0.85 (VERIFIED against objdump 0x551f00..0x552016: cluster from leaf+4 (-1 -> none), radius <= 0 -> that cluster alone, else flood fill (stamp 0x6e3f04++, busy flag 0x6e3f01) into up to 0x40 clusters; per cluster the object's chain (group+8, +4 = cluster sign-extended) and the cluster's chain (group+4 head group+0[cluster], +4 = handle))
// evidence: src/objects/object_light_recompute_transform.c and object_set_cluster_and_parent.c
//   already resolved and documented the four stack arguments plus the EAX (leaf_and_cluster) and
//   EDI (cluster_list) register arguments from disassembly. This file resolves the one thing
//   those two left unexamined -- the data_array datum_new() allocates from on each of its two
//   calls, which Ghidra's decompilation loses completely (it shows a bogus 64-bit "return" that
//   CONCATs the unclobbered input register with the real EAX result). Disassembly
//   (objdump -d -M intel bin/halo.exe, 0x551f00..0x552019) shows:
//     - first datum_new call: `mov edx,[edi+8]` immediately before the call -- allocates from
//       cluster_list[2], the object's own object_cluster_reference chain (types/structures.h's
//       cluster_reference_group.object_cluster_references, see cluster_partition_new.c), and
//       chains the new datum into *placement_slot (the object's own chain head).
//     - second datum_new call: `mov edx,[edi+4]` -- allocates from cluster_list[1]
//       (cluster_object_references), and chains the new datum into
//       cluster_list[0][cluster] (the target cluster's own chain head, cluster_list[0] being the
//       per-cluster head table every reader of this module's cluster_reference_group already
//       assumes).
//   So both allocations share nothing but the record shape (object_cluster_reference, stride
//   0x0c); no separate "reference_list" pool exists.
// register convention: stack -> light_or_object_handle, placement_slot, position, radius;
//   EAX -> leaf_and_cluster; EDI -> cluster_list. (blam-cc register order would put EDI before
//   EAX; kept in the order the two existing consumer files already committed to.)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "structures.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern uint8_t cluster_flood_in_progress; // 0x006e3f01, this module (types/structures.h)
extern int32_t cluster_flood_stamp;       // 0x006e3f04, this module (types/structures.h)

extern datum_index datum_new(data_array *array); // 0x4d0480, memory module; array in EDX
extern int32_t cluster_flood_fill_within_radius(int16_t cluster_index, real_point3d *point,
    float tolerance, int32_t remaining_budget, int16_t *output); // 0x554d10, this module
    // (signature taken verbatim from src/structures/cluster_flood_fill_within_radius.c)

// Finds every cluster within `radius` of `position` (or just the location's own cluster, when
// radius is non-positive), and for each one links a fresh object_cluster_reference into both the
// object's own chain (headed by *placement_slot) and that cluster's chain (headed by
// cluster_list->cluster_first[cluster]). Used to register a light or object against every
// cluster its influence/bounding radius touches.
void cluster_reference_add_within_radius(uint32_t light_or_object_handle, datum_index *placement_slot,
    real_point3d *position, float radius, bsp_leaf_reference *leaf_and_cluster,
    cluster_reference_group *cluster_list)
    // blam-cc: EAX -> leaf_and_cluster, EDI -> cluster_list
{
    int16_t clusters[64];
    int16_t cluster_count;
    int16_t i;

    cluster_count = 0;
    if (leaf_and_cluster->cluster_index != -1) {
        if (radius <= 0.0f) {
            cluster_count = 1;
            clusters[0] = leaf_and_cluster->cluster_index;
        } else {
            cluster_flood_stamp = cluster_flood_stamp + 1;
            cluster_flood_in_progress = 1;
            cluster_count = cluster_flood_fill_within_radius(leaf_and_cluster->cluster_index,
                position, radius, 0x40, clusters);
            cluster_flood_in_progress = 0;
        }
    }

    if (cluster_count > 0x40) {
        cluster_count = 0x40;
    }

    for (i = 0; i < cluster_count; i = i + 1) {
        int16_t cluster = clusters[i];
        datum_index handle;

        handle = datum_new(cluster_list->object_cluster_references);
        if (handle != k_datum_index_none) {
            object_cluster_reference *ref = (object_cluster_reference *)
                cluster_list->object_cluster_references->data + (handle & 0xffff);
            ref->object_index = cluster;   // reused generically: holds the cluster index here
            ref->next_reference = *placement_slot;
            *placement_slot = handle;
        }

        {
            datum_index *cluster_head = &cluster_list->cluster_first[cluster];
            handle = datum_new(cluster_list->cluster_object_references);
            if (handle != k_datum_index_none) {
                object_cluster_reference *ref = (object_cluster_reference *)
                    cluster_list->cluster_object_references->data + (handle & 0xffff);
                ref->object_index = light_or_object_handle;
                ref->next_reference = *cluster_head;
                *cluster_head = handle;
            }
        }
    }
}

#if 0
Original Ghidra decompilation (0x551f00):

void FUN_00551f00(undefined4 param_1,uint *param_2,undefined4 param_3,float param_4)

{
  int iVar1;
  uint *puVar2;
  short sVar3;
  ushort uVar4;
  int in_EAX;
  uint uVar5;
  short *psVar6;
  int *unaff_EDI;
  undefined8 uVar7;
  uint local_84;
  short local_80 [64];

  sVar3 = *(short *)(in_EAX + 4);
  uVar4 = 0;
  if (sVar3 != -1) {
    if (param_4 <= 0.0) {
      uVar4 = 1;
      local_80[0] = sVar3;
    }
    else {
      DAT_006e3f04 = DAT_006e3f04 + 1;
      DAT_006e3f01 = 1;
      uVar4 = cluster_flood_fill_within_radius(sVar3,param_3,param_4,0x40,local_80);
      DAT_006e3f01 = 0;
    }
  }
  if (0x40 < (short)uVar4) {
    uVar4 = 0x40;
  }
  if (0 < (short)uVar4) {
    local_84 = (uint)uVar4;
    psVar6 = local_80;
    do {
      sVar3 = *psVar6;
      uVar7 = datum_new();
      uVar5 = (uint)uVar7;
      if (uVar5 != 0xffffffff) {
        iVar1 = *(int *)((int)((ulonglong)uVar7 >> 0x20) + 0x34) + (uVar5 & 0xffff) * 0xc;
        *(int *)(iVar1 + 4) = (int)sVar3;
        *(uint *)(iVar1 + 8) = *param_2;
        *param_2 = uVar5;
      }
      puVar2 = (uint *)(*unaff_EDI + sVar3 * 4);
      uVar7 = datum_new();
      uVar5 = (uint)uVar7;
      if (uVar5 != 0xffffffff) {
        iVar1 = *(int *)((int)((ulonglong)uVar7 >> 0x20) + 0x34) + (uVar5 & 0xffff) * 0xc;
        *(undefined4 *)(iVar1 + 4) = param_1;
        *(uint *)(iVar1 + 8) = *puVar2;
        *puVar2 = uVar5;
      }
      psVar6 = psVar6 + 1;
      local_84 = local_84 - 1;
    } while (local_84 != 0);
  }
  return;
}

Disassembly excerpt confirming the two datum_new array sources (objdump -d -M intel):

00551fa0: mov edx,[edi+8]     ; datum_new(array = cluster_list->object_cluster_references)
00551fa3: mov si,[ebx]
00551fa6: call 0x4d0480
...
00551fd0: mov ecx,[edi]       ; cluster_list->cluster_first
00551fd2: mov edx,[edi+4]     ; datum_new(array = cluster_list->cluster_object_references)
00551fd5: movsx eax,si
00551fd8: lea esi,[ecx+eax*4] ; &cluster_first[cluster]
00551fdb: call 0x4d0480
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
