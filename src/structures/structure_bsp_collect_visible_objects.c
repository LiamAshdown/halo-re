// structure_bsp_collect_visible_objects  (Ghidra: FUN_00554420, still unnamed there)
// address 0x554420, size 200 bytes
// name confidence: 0.55 -- matches the phase4 summary ("Enumerates the objects in every currently
//   visible cluster, frustum-culls each by its bounding sphere, and builds a capped output list of
//   the surviving object handles").
// rewrite confidence: 0.5 -- Ghidra recovered the parameter count (7) but typed 5 of them as bare
//   `code *`; objdump disassembly confirms the exact call sequence and which stack slot each
//   callback consumes. The callbacks themselves belong to the objects module (per-cluster object
//   iteration over the collideable_cluster_first / collideable_object_references lists
//   types/objects.h already documents) and are not resolved by name here.
// evidence: objdump -M intel disassembly of 0x554420..0x5544f0; types/structures.h
//   structure_bsp_visible_cluster.cluster_index.
// register convention: cdecl, 7 stack parameters, no register-passed arguments.
// UNSURE: the 5 callback parameters' real names/targets (out of this module); their call
//   signatures are reproduced exactly as observed (iterator begin/next return a handle or -1,
//   the predicate returns a bool in AL, the bounds getter fills a small caller buffer, the
//   "accept" callback takes the handle and returns nothing this caller reads).

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "structures.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern int16_t visible_cluster_count; // 0x007d0390
extern structure_bsp_visible_cluster visible_clusters[k_maximum_visible_clusters]; // 0x007c3390
extern int32_t render_cluster_index;  // 0x007c3348

// render module, out of this batch.
extern int16_t render_frustum_test_sphere(void *frustum, real_point3d *center, float radius);

typedef uint32_t (*structure_bsp_object_iterate_begin_fn)(uint32_t *cursor, int16_t cluster_index);
typedef uint32_t (*structure_bsp_object_iterate_next_fn)(uint32_t *cursor);
typedef uint8_t (*structure_bsp_object_predicate_fn)(uint32_t handle);
// FIXED (objdump 0x554474..0x55447f): the bounds callback is (handle, &center, &radius) and writes the 12-byte centre
// into the caller's frame (0x554498 then hands &center to the frustum test in EDX); it was declared with the two
// outputs swapped and the centre as a pointer, so the callback wrote 12 bytes over a 4-byte local
typedef void (*structure_bsp_object_get_bounds_fn)(uint32_t handle, real_point3d *center_out, float *radius_out);
typedef void (*structure_bsp_object_accept_fn)(uint32_t handle);

// blam-cc: cdecl, 7 stack params
// FIXED (objdump): every ret sets only AX; the upper bits of EAX are left as they were
int16_t structure_bsp_collect_visible_objects(
    int32_t *out_handles, int16_t max_count,
    structure_bsp_object_iterate_begin_fn iterate_begin,
    structure_bsp_object_iterate_next_fn iterate_next,
    structure_bsp_object_get_bounds_fn get_bounds, structure_bsp_object_predicate_fn predicate,
    structure_bsp_object_accept_fn accept)
{
    int16_t written = 0;

    for (int16_t i = 0; i < visible_cluster_count; i++) {
        uint32_t cursor;
        uint32_t handle = iterate_begin(&cursor, visible_clusters[i].cluster_index);
        while (handle != 0xffffffff) {
            if (predicate(handle)) {
                float radius;
                real_point3d center;
                get_bounds(handle, &center, &radius);
                if (written < max_count &&
                    (render_cluster_index == -1 ||
                     render_frustum_test_sphere(&visible_clusters[i].frustum, &center,
                                                 radius) != 0)) {
                    out_handles[written] = (int32_t)handle;
                    written++;
                    accept(handle);
                }
            }
            handle = iterate_next(&cursor);
        }
    }
    return written;
}

#if 0
Original Ghidra decompilation (0x554420):

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

undefined4
FUN_00554420(int param_1,short param_2,code *param_3,code *param_4,code *param_5,code *param_6,
            code *param_7)

{
  char cVar1;
  short sVar2;
  undefined4 in_EAX;
  undefined2 uVar4;
  int iVar3;
  short sVar5;
  short sVar6;
  undefined4 uStack_14;
  undefined1 local_10 [4];
  undefined1 auStack_c [12];

  uVar4 = (undefined2)((uint)in_EAX >> 0x10);
  sVar6 = 0;
  sVar5 = 0;
  if (0 < DAT_007d0390) {
    do {
      iVar3 = (*param_3)(local_10,(&DAT_007c3390)[sVar6 * 0xd0]);
      while (iVar3 != -1) {
        cVar1 = (*param_6)(iVar3);
        if (((cVar1 != '\0') && ((*param_5)(iVar3,auStack_c,&uStack_14), sVar5 < param_2)) &&
           ((_DAT_007c3348 == -1 || (sVar2 = render_frustum_test_sphere(uStack_14), sVar2 != 0)))) {
          *(int *)(param_1 + sVar5 * 4) = iVar3;
          sVar5 = sVar5 + 1;
          (*param_7)(iVar3);
        }
        iVar3 = (*param_4)(local_10);
      }
      uVar4 = 0xffff;
      sVar6 = sVar6 + 1;
    } while (sVar6 < DAT_007d0390);
  }
  return CONCAT22(uVar4,sVar5);
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
