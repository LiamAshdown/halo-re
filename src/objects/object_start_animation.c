// object_start_animation  (Ghidra: object_start_animation, already named)
// address 0x4fa8d0, size 214 bytes
// name confidence: 0.55 (already carries this name from an earlier phase; matches
//   functions.md's summary: "Looks up a named animation in an animation graph and starts it on
//   the object, logging an error if the animation is not found")
// rewrite confidence: 0.9 (VERIFIED against 0x4fa8d0 (EDI graph / EBX name lookup, flags, frame clamp, error message)) (zero recorded callers in this module; likely reached from units/
//   scenery elsewhere per the CEA hints "scenery_animation_start_private",
//   "unit_start_user_animation")
// evidence: types/objects.h object (flags 0x10, animation_graph 0x0cc, animation_index 0x0d0,
//   animation_frame 0x0d2); global 0x008603b0 object_data, 0x0087bc14 tag_instances; callees
//   animation_graph_find_animation_by_name (0x4d6ab0, foreign module) and console_print_va (0x4c6920,
//   established elsewhere in this codebase).
// register convention: object index in EAX, animation graph tag in EDI, requested frame as the
//   sole stack parameter (Ghidra's own "object_start_animation(short param_1)" plus
//   "in_EAX"/"unaff_EDI").
//   // blam-cc: EAX -> object_index, EDI -> graph_tag, stack -> requested_frame
// UNSURE: the animation NAME string this looks up is passed to animation_graph_find_animation_by_name
//   but never appears as a declared local anywhere in the decompile; guessed here as an
//   implicit ECX parameter, consistent with this module's usual third-argument slot. Object
//   flags bit 0x80 (cleared on start) and the byte at object+0x1f4 (set bit 0, past the common
//   0x1f4 object header, inside the unit/vehicle extension) are both left as raw offsets.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern data_array *object_data; // 0x008603b0
extern tag_instance *tag_instances; // 0x0087bc14

extern int16_t animation_graph_find_animation_by_name(datum_index animation_graph_tag, const char *name); // 0x4d6ab0, EAX, EBX
extern void console_print_va(const char *format, ...); // 0x4c6920

void object_start_animation(uint32_t object_index, datum_index graph_tag, char *name, int16_t requested_frame)
    // blam-cc: EAX -> object_index, EDI -> graph_tag, ECX -> name (UNSURE), stack -> requested_frame
{
    if ((object_index != k_datum_index_none) && (graph_tag != k_datum_index_none)) {
        object *obj = ((object_header *)object_data->data)[object_index & 0xffff].data;
        void *graph = tag_instances[graph_tag & 0xffff].data;
        // 0x4fa910: EAX = the graph TAG handle (edi), EBX = the name; the draft passed the tag data pointer
        int16_t animation_index = animation_graph_find_animation_by_name(graph_tag, name);

        if (animation_index != -1) {
            uint8_t *nodes = *(uint8_t **)((uint8_t *)graph + 0x78);
            uint8_t *extended_flags = (uint8_t *)obj + 0x1f4; // UNSURE: see file header

            obj->flags &= ~0x80u;
            *extended_flags |= 1;
            obj->animation_index = animation_index;

            if (requested_frame < 0) {
                obj->animation_frame = 0;
                obj->animation_graph = graph_tag;
                return;
            }

            {
                int16_t frame_count = *(int16_t *)(nodes + animation_index * 0xb4 + 0x22) - 1;
                int16_t frame = (requested_frame <= frame_count) ? requested_frame : frame_count;
                obj->animation_frame = frame;
                obj->animation_graph = graph_tag;
            }
            return;
        }

        console_print_va("the animation '%s' doesn't exist in the graph '%s'", name,
            *(char **)((uint8_t *)&tag_instances[graph_tag & 0xffff] + 0x10)); // 0x4fa984: the tag instance name
    }
}

#if 0
Original Ghidra decompilation (0x4fa8d0):

void object_start_animation(short param_1)

{
  int iVar1;
  short sVar2;
  uint in_EAX;
  int iVar3;
  uint unaff_EDI;

  if ((in_EAX != 0xffffffff) && (unaff_EDI != 0xffffffff)) {
    iVar1 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (in_EAX & 0xffff) * 0xc);
    iVar3 = *(int *)((unaff_EDI & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
    sVar2 = model_get_region_index_by_name();
    if (sVar2 != -1) {
      iVar3 = *(int *)(iVar3 + 0x78);
      *(uint *)(iVar1 + 0x10) = *(uint *)(iVar1 + 0x10) & 0xffffff7f;
      *(uint *)(iVar1 + 500) = *(uint *)(iVar1 + 500) | 1;
      *(short *)(iVar1 + 0xd0) = sVar2;
      if (param_1 < 0) {
        *(undefined2 *)(iVar1 + 0xd2) = 0;
        *(uint *)(iVar1 + 0xcc) = unaff_EDI;
        return;
      }
      iVar3 = *(short *)(sVar2 * 0xb4 + iVar3 + 0x22) + -1;
      if (param_1 <= iVar3) {
        iVar3 = (int)param_1;
      }
      *(short *)(iVar1 + 0xd2) = (short)iVar3;
      *(uint *)(iVar1 + 0xcc) = unaff_EDI;
      return;
    }
    console_print_va("the animation \'%s\' doesn\'t exist in the graph \'%s\'");
  }
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
