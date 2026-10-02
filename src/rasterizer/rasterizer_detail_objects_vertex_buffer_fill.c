// rasterizer_detail_objects_vertex_buffer_fill  (Ghidra: FUN_0051b6f0, unnamed)
// address 0x51b6f0, size 406 bytes
// name confidence: 0.6   rewrite confidence: 0.85
// evidence: same gate as rasterizer_detail_objects_draw 0x51b890 and the same batch list
//   (types/rasterizer.h rasterizer_detail_object_batches). Locks the whole detail object vertex
//   buffer (0x0071d1c8, Lock +0x2c, 0x78000 bytes), and for every draw of every batch expands
//   its detail object instances (6 byte records taken from the first
//   ScenarioStructureBSP.detail_objects element's +0x10 pointer, indexed by draw +0) into six
//   0x14 byte vertices each through rasterizer_detail_objects_expand_quad_vertices 0x51b150,
//   records the draw's first vertex, caps the frame at 0x1000 quads and finally unlocks (+0x30),
//   even when the lock failed.
//   Spot-check fix (phase 4 review): the earlier rewrite (rasterizer_decal_static_vertex_buffer_
//   fill) modelled two register inputs Ghidra could not trace; both are ordinary: the scenario is
//   global_scenario (0x51b70f) and the list is the stack argument (0x51b75f), and the source
//   instances come from global_structure_bsp +0x24c/+0x250.
// register convention: __cdecl, list pointer on the stack.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "rasterizer.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern void *rasterizer_detail_object_vertex_buffer;                // 0x0071d1c8
extern Scenario *global_scenario;                                   // 0x00746f8c
extern ScenarioStructureBSP *global_structure_bsp;                         // 0x00746f9c
extern tag_instance *tag_instances;                                 // 0x0087bc14
extern uint8_t console_debug_toggle_689404;                         // 0x00689404 detail objects enable

extern int16_t render_local_view_count(void);                                  // 0x4c9220, UNSURE: local player count
// blam-cc: EAX -> quad_count, ECX -> vertices, EDX -> instances, stack -> (collection, draw)
extern void rasterizer_detail_objects_expand_quad_vertices(int32_t quad_count, uint32_t *vertices, const uint8_t *instances,
                                                           const DetailObjectCollection *collection,
                                                           const rasterizer_detail_object_draw *draw); // 0x51b150

typedef int32_t (__stdcall *d3d_lock_fn)(void *self, uint32_t offset, uint32_t size, void **data, uint32_t flags);
typedef int32_t (__stdcall *d3d_unlock_fn)(void *self);

void rasterizer_detail_objects_vertex_buffer_fill(rasterizer_detail_object_batches *list)
{
    Scenario *scenario;
    uint8_t *vertices = 0;
    void *buffer;

    if (console_debug_toggle_689404 == 0 || render_local_view_count() > 1) {
        return;
    }

    scenario = global_scenario;
    buffer = rasterizer_detail_object_vertex_buffer;
    if (((d3d_lock_fn)(*(void ***)buffer)[0x2c / 4])(buffer, 0, 0x78000, (void **)&vertices, 0) >= 0 && vertices != 0) {
        uint8_t *detail_objects = *(uint32_t *)((uint8_t *)global_structure_bsp + 0x24c) != 0 /* FIXED: was a ScenarioStructureBSP-sized step */
                                      ? (uint8_t *)*(uint32_t *)((uint8_t *)global_structure_bsp + 0x250) : (uint8_t *)0;
        const uint8_t *instances = (const uint8_t *)*(uint32_t *)(detail_objects + 0x10);
        int32_t vertex_cursor = 0;
        int32_t quads_used = 0;
        uint8_t overflow = 0;                                   // set but never read here
        int16_t batch_index;

        for (batch_index = 0; batch_index < list->batch_count; batch_index++) {
            rasterizer_detail_object_batch *batch = &((rasterizer_detail_object_batch *)list->batches)[batch_index];
            const uint8_t *palette = (const uint8_t *)scenario->detail_object_collection_palette.pointer;
            uint32_t collection_tag = *(const uint32_t *)(palette + batch->collection_palette_index * 0x30 + 0xc);
            const DetailObjectCollection *collection =
                (const DetailObjectCollection *)tag_instances[collection_tag & 0xffff].data;
            int16_t draw_index;

            for (draw_index = 0; draw_index < batch->draw_count; draw_index++) {
                rasterizer_detail_object_draw *draw = &((rasterizer_detail_object_draw *)batch->draws)[draw_index];
                int32_t quad_count = draw->quad_count;

                if (quad_count > 0x1000 - quads_used) {
                    quad_count = 0x1000 - quads_used;
                }
                rasterizer_detail_objects_expand_quad_vertices(quad_count, (uint32_t *)(vertices + vertex_cursor * 0x14),
                                                               instances + draw->first_instance * 6, collection, draw);
                draw->first_vertex = vertex_cursor;
                vertex_cursor += draw->quad_count * 6;          // advances by the uncapped count
                if (draw->quad_count > quad_count) {
                    draw->quad_count = quad_count * 2;          // UNSURE: doubled as in the raw code
                    if (!overflow) {                            // 0x51b830
                        overflow = 1;
                    }
                }
                quads_used += quad_count;
            }
        }
    }
    ((d3d_unlock_fn)(*(void ***)rasterizer_detail_object_vertex_buffer)[0x30 / 4])(rasterizer_detail_object_vertex_buffer);
}

#if 0
Original Ghidra decompilation (0x51b6f0):

void FUN_0051b6f0(void)

{
  int *piVar1;
  int iVar2;
  short sVar3;
  int iVar4;
  int *piVar5;
  int iVar6;
  undefined4 uVar7;
  int iVar8;
  int iVar9;
  int iStack_20;
  undefined4 local_1c;
  undefined4 uStack_18;
  int *piStack_10;
  undefined4 local_c;

  if ((DAT_00689404 != '\0') && (sVar3 = FUN_004c9220(), sVar3 < 2)) {
    iVar8 = 0x78000;
    local_c = global_scenario;
    local_1c = 0;
    iVar4 = (**(code **)(*DAT_0071d1c8 + 0x2c))(DAT_0071d1c8,0,0x78000,&local_1c,0);
    if ((-1 < iVar4) && (iVar8 != 0)) {
      if (*(int *)(DAT_00746f9c + 0x24c) == 0) {
        iVar4 = 0;
      }
      else {
        iVar4 = *(int *)(DAT_00746f9c + 0x250);
      }
      local_1c = *(undefined4 *)(iVar4 + 0x10);
      iVar4 = 0;
      iVar8 = 0;
      uVar7 = 0;
      sVar3 = 0;
      piVar5 = piStack_10;
      if (0 < (short)piStack_10[1]) {
        do {
          piVar1 = (int *)(*piVar5 + sVar3 * 8);
          uStack_18 = *(undefined4 *)
                       ((*(uint *)(*(short *)((int)piVar1 + 6) * 0x30 + 0xc +
                                  *(int *)(iStack_20 + 0x3c4)) & 0xffff) * 0x20 + 0x14 +
                       DAT_0087bc14);
          iVar9 = 0;
          if (0 < (short)piVar1[1]) {
            do {
              iVar6 = *(int *)(*piVar1 + 4 + (short)iVar9 * 0x18);
              iVar2 = *piVar1 + (short)iVar9 * 0x18;
              if (0x1000 - iVar8 < iVar6) {
                iVar6 = 0x1000 - iVar8;
              }
              FUN_0051b150(uStack_18,iVar2);
              *(int *)(iVar2 + 0x10) = iVar4;
              iVar4 = iVar4 + *(int *)(iVar2 + 4) * 6;
              if ((iVar6 < *(int *)(iVar2 + 4)) &&
                 (*(int *)(iVar2 + 4) = iVar6 * 2, (char)((uint)uVar7 >> 0x18) == '\0')) {
                uVar7 = 0x1000000;
              }
              iVar8 = iVar8 + iVar6;
              iVar9 = iVar9 + 1;
              piVar5 = piStack_10;
            } while ((short)iVar9 < (short)piVar1[1]);
          }
          sVar3 = sVar3 + 1;
        } while (sVar3 < (short)piVar5[1]);
      }
    }
    (**(code **)(*DAT_0071d1c8 + 0x30))(DAT_0071d1c8);
  }
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
