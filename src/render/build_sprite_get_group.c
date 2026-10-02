// build_sprite_get_group  (Ghidra: FUN_00511520; named from out/phase4/render_types_notes.md:
// "build_sprite_get_group 0x511520 (EDI data, EAX bitmap)")
// address 0x511520, size 250 bytes
// name confidence: 0.55   rewrite confidence: 0.8
// evidence: types/render.h build_sprite_data.groups[8] (+0x24, build_sprite_group stride 0x10:
//   vertex_slot +0x00, vertices +0x04, quad_count +0x08, bitmap +0x0c) and
//   k_maximum_build_sprite_groups (8, "build_sprite_get_group 0x511520 fails at 8"). The vertex
//   slot allocate/lock pair matches types/rasterizer.h rasterizer_dynamic_vertex_slot ("FUN_0051bdd0
//   fills type/first/count, FUN_0051be40 locks the range and stores the pointer").
// review fix (phase-4 gate, objdump 0x511520..0x511619): the final usability test reads
//   groups[i].vertices (+0x28 + i * 0x10), not the bitmap key; the vertex reserve takes AX =
//   vertex type and ESI = count (named after src/rasterizer).
// register convention (objdump 0x511520..0x511619): EDI = data (build_sprite_data*), EAX =
//   bitmap (BitmapData*). Confirms render_types_notes.md's EDI/EAX note and additionally recovers
//   the two vertex-slot calls' own registers: FUN_0051bdd0(ESI = vertex_count, EAX = vertex_type
//   6 or 8 depending on data's screen-space flag), FUN_0051be40(EAX = slot index, unchanged from
//   the previous call's result).
//   // blam-cc: EDI -> data, EAX -> bitmap
// reconciled: R77 0x0069c632 extern uint16 -> int16 (rasterizer.h type)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "rasterizer.h"
#include "render.h"
#include <stdint.h> // uintptr_t
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern int16_t rasterizer_vertex_buffer_lock_state; // 0x0069c632 (matches
                                                     // src/rasterizer/rasterizer_decal_vertex_cache_lock.c)
extern uint8_t build_sprite_group_warning; // 0x0071cfbf

extern void *texture_cache_get(BitmapData *bitmap, uint8_t wait, uint8_t allocate_if_missing); // 0x444550
extern int32_t rasterizer_dynamic_vertex_cache_reserve(int16_t vertex_type, int32_t count);
    // 0x51bdd0, rasterizer module; blam-cc: AX = vertex_type, ESI = count
extern void *rasterizer_dynamic_vertex_cache_lock(int32_t slot_index);
    // 0x51be40, rasterizer module; blam-cc: EAX = slot_index

// Finds data's existing group for `bitmap`, or appends a new one (failing once
// k_maximum_build_sprite_groups is reached) and locks maximum_sprite_count*4 dynamic vertices for
// it. Returns the group index, or -1 on failure (including when the group has no locked
// vertices).
int16_t build_sprite_get_group(build_sprite_data *data, BitmapData *bitmap) // blam-cc: EDI=data, EAX=bitmap
{
    int16_t count = data->group_count;
    int16_t i;
    build_sprite_group *group;

    for (i = 0; i < count; i++) {
        if (data->groups[i].bitmap == (uint32_t)bitmap) {
            break;
        }
    }

    if (count <= i) {
        if (count > 7) {
            return -1;
        }
        group = &data->groups[i];
        data->group_count = count + 1;
        group->bitmap = (uint32_t)bitmap;

        if (texture_cache_get(bitmap, 0, 1) == 0) {
            group->vertices = 0;
            group->vertex_slot = -1;
        } else {
            int32_t vertex_count = (int32_t)data->maximum_sprite_count * 4;
            int16_t vertex_type = (data->flags & _build_sprite_data_screen_space_bit) != 0 ?
                                      _rasterizer_vertex_type_dynamic_screen :
                                      _rasterizer_vertex_type_dynamic_unlit;
            int32_t slot;

            rasterizer_vertex_buffer_lock_state = 0x10;
            slot = rasterizer_dynamic_vertex_cache_reserve(vertex_type, vertex_count);
            group->vertex_slot = slot;
            if (slot == -1) {
                if (build_sprite_group_warning == 0) {
                    build_sprite_group_warning = 1;
                }
                group->vertices = 0;
                rasterizer_vertex_buffer_lock_state = 0;
            } else {
                group->vertices = (uint32_t)(uintptr_t)rasterizer_dynamic_vertex_cache_lock(slot);
                rasterizer_vertex_buffer_lock_state = 0;
            }
        }
        group->quad_count = 0;
    }

    // the group is unusable when its vertices could not be locked (mov ecx,[eax+edi+0x28]:
    // groups[i].vertices)
    if (i != -1 && data->groups[i].vertices == 0) {
        return -1;
    }
    return i;
}

#if 0
Original Ghidra decompilation (0x511520):

short FUN_00511520(void)

{
  int *piVar1;
  short sVar2;
  int in_EAX;
  int iVar3;
  short sVar4;
  int unaff_EDI;

  sVar2 = *(short *)(unaff_EDI + 0x20);
  sVar4 = 0;
  if (0 < sVar2) {
    do {
      if (*(int *)((sVar4 + 3) * 0x10 + unaff_EDI) == in_EAX) break;
      sVar4 = sVar4 + 1;
    } while (sVar4 < *(short *)(unaff_EDI + 0x20));
  }
  if (sVar2 <= sVar4) {
    if (7 < sVar2) {
      return -1;
    }
    if (sVar2 <= sVar4) {
      piVar1 = (int *)(sVar4 * 0x10 + 0x24 + unaff_EDI);
      *(short *)(unaff_EDI + 0x20) = sVar2 + 1;
      piVar1[3] = in_EAX;
      iVar3 = texture_cache_get(0,1);
      if (iVar3 == 0) {
        piVar1[1] = 0;
        *piVar1 = -1;
      }
      else {
        _DAT_0069c632 = 0x10;
        iVar3 = FUN_0051bdd0();
        *piVar1 = iVar3;
        if (iVar3 == -1) {
          if (DAT_0071cfbf == '\0') {
            DAT_0071cfbf = '\x01';
          }
          piVar1[1] = 0;
          _DAT_0069c632 = 0;
        }
        else {
          iVar3 = FUN_0051be40();
          piVar1[1] = iVar3;
          _DAT_0069c632 = 0;
        }
      }
      *(undefined2 *)(piVar1 + 2) = 0;
    }
  }
  if ((sVar4 != -1) && (*(int *)(sVar4 * 0x10 + 0x28 + unaff_EDI) == 0)) {
    return -1;
  }
  return sVar4;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
