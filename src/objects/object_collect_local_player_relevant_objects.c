// object_collect_local_player_relevant_objects  (Ghidra: FUN_004fa1a0; renamed, Blam-style,
// not previously named -- the name is historical: the function collects every object in the clusters
// potentially visible from a point, filtered, into an array)
// address 0x4fa1a0, size 233 bytes
// name confidence: 0.25
// rewrite confidence: 0.85
// REWRITTEN 2026-09-27 (static loop) from objdump 0x4fa1a0..0x4fa39e, merged with the body Ghidra split off at
//   0x4fa280 (object_type_definitions_collect_by_flag_bits, a FRAGMENT: the word loop at 0x4fa35b jumps back to
//   0x4fa237 and 0x4fa278 simply jumps into it). The draft had no parameters: EDX is the probe POINT (never
//   written before the 0x5013a0 call) and the stack carries (filter, filter_context, max_count, out), which
//   0x4fa2ef..0x4fa305 forwards to object_tree_collect_matching(object, filter, context, count, max_count, out).
//   Algorithm: leaf = bsp3d_node_find_leaf(0, global_collision_bsp, point); cluster = structure bsp leaves (+0xe4,
//   0x10 each) +8; the cluster's PVS row is structure bsp +0x14c + cluster * words * 4 with words =
//   (cluster_count(+0x134) + 31) >> 5; for every set bit (cluster) walk the cluster's object references
//   (0x008603d0[cluster], next at +8, object at +4) and collect each object tree once (object +0x14 cluster stamp
//   against the bumped 0x008603cc). object_globals +1 is raised during the walk. Returns the count.
// blam-cc: EDX -> point, stack -> filter, filter_context, max_count, out

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern ModelCollisionGeometryBSP *global_collision_bsp; // 0x00746f90
extern ScenarioStructureBSP *global_structure_bsp;
extern object_globals *object_globals_pointer; // 0x006b8cbc
extern int32_t object_cluster_stamp; // 0x008603cc
extern datum_index *collideable_cluster_first; // 0x008603d0
extern data_array *collideable_object_references; // 0x008603d4
extern data_array *object_data; // 0x008603b0

extern uint32_t bsp3d_node_find_leaf(int32_t node_index, ModelCollisionGeometryBSP *bsp, real_point3d *point); // 0x5013a0, EAX node, ECX bsp, EDX point
extern int32_t object_tree_collect_matching(uint32_t object_index, uint8_t (*filter)(uint32_t, void *),
    void *filter_context, int32_t count, int32_t max_count, datum_index *out); // 0x4fa0f0

int32_t object_collect_local_player_relevant_objects(real_point3d *point, uint8_t (*filter)(uint32_t, void *),
    void *filter_context, int32_t max_count, datum_index *out)
{
    int32_t count = 0;
    uint32_t leaf = bsp3d_node_find_leaf(0, global_collision_bsp, point);
    ScenarioStructureBSP *bsp;
    int16_t cluster;
    int32_t words;
    int32_t *row;
    int32_t *word;
    int16_t word_index;

    if (leaf == 0xffffffff) {
        return 0;
    }
    bsp = global_structure_bsp;
    cluster = *(int16_t *)((uint8_t *)bsp->leaves.pointer + (leaf & 0x7fffffff) * 0x10 + 8);
    if (cluster == -1) {
        return 0;
    }

    object_globals_pointer->collecting_in_clusters = 1;
    words = ((int32_t)bsp->clusters.count + 0x1f) >> 5;
    row = (int32_t *)((uint8_t *)bsp->cluster_data.pointer) + (int32_t)cluster * words;
    object_cluster_stamp = object_cluster_stamp + 1;

    word = row;
    for (word_index = 0; word_index < (int16_t)words; word_index++, word++) {
        int32_t cluster_count;
        int32_t lo;
        int32_t hi;
        int32_t bit;
        int32_t remaining;

        if (*word == 0) {
            continue;
        }
        cluster_count = *(int32_t *)&global_structure_bsp->clusters.count;
        lo = (int16_t)(word_index << 5);
        hi = lo + 0x20;
        if (hi > cluster_count) {
            hi = cluster_count;
        }
        if ((int16_t)lo >= (int16_t)hi) {
            continue;
        }
        remaining = (uint16_t)((int16_t)hi - (int16_t)lo);
        bit = lo;
        do {
            if ((row[bit >> 5] & (1u << (bit & 0x1f))) != 0) {
                datum_index ref = collideable_cluster_first[bit];

                while (ref != 0xffffffff) {
                    object_cluster_reference *node =
                        (object_cluster_reference *)collideable_object_references->data + (ref & 0xffff);
                    datum_index object_index = node->object_index;
                    object *obj;

                    ref = node->next_reference;
                    obj = ((object_header *)object_data->data)[object_index & 0xffff].data;
                    if (obj->cluster_stamp != object_cluster_stamp) {
                        obj->cluster_stamp = object_cluster_stamp;
                        count = object_tree_collect_matching(object_index, filter, filter_context, count, max_count,
                            out);
                    }
                }
            }
            bit++;
        } while (--remaining != 0);
    }

    object_globals_pointer->collecting_in_clusters = 0;
    return count;
}

#if 0
Original Ghidra decompilation (0x4fa1a0):

undefined4 FUN_004fa1a0(void)

{
  int iVar1;
  int *piVar2;
  undefined4 uVar3;
  short sVar4;
  int iVar5;
  short sVar6;

  iVar1 = FUN_005013a0();
  iVar5 = DAT_00746f9c;
  if (iVar1 == -1) {
    return 0;
  }
  sVar6 = *(short *)(iVar1 * 0x10 + 8 + *(int *)(DAT_00746f9c + 0xe4));
  if (sVar6 == -1) {
    return 0;
  }
  *(undefined1 *)(DAT_006b8cbc + 1) = 1;
  iVar1 = *(int *)(iVar5 + 0x134) + 0x1f >> 5;
  piVar2 = (int *)(*(int *)(iVar5 + 0x14c) + sVar6 * iVar1 * 4);
  DAT_008603cc = DAT_008603cc + 1;
  sVar4 = (short)iVar1;
  sVar6 = 0;
  if (0 < sVar4) {
    do {
      if (*piVar2 != 0) {
        iVar1 = (short)(sVar6 * 0x20) + 0x20;
        if (*(int *)(iVar5 + 0x134) < iVar1) {
          iVar1 = *(int *)(iVar5 + 0x134);
        }
        if ((short)(sVar6 * 0x20) < (short)iVar1) {
          uVar3 = object_get_orientation();
          return uVar3;
        }
      }
      sVar6 = sVar6 + 1;
      piVar2 = piVar2 + 1;
      iVar5 = DAT_00746f9c;
    } while (sVar6 < sVar4);
  }
  *(undefined1 *)(DAT_006b8cbc + 1) = 0;
  return 0;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
