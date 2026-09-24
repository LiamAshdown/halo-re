// unit_scripting_set_emotion_animation  (Ghidra: unit_scripting_set_emotion_animation, already
// named)
// address 0x569cf0, size 74 bytes, name confidence 0.9, rewrite confidence 0.5
// evidence: types/units.h unit_data.emotion_animation_index (0x21e, "unit_scripting_set_
//   emotion_animation writes the animation_graph_find_animation_by_name result here"); the cea-pdb
//   candidate list agrees via the exact error string.
// blam-cc: in_EAX -> unit_index, region name forwarded implicitly to both callees (not visible
//   as a parameter in this decompilation; modelled explicitly as `emotion_name`).
// UNSURE: animation_graph_find_animation_by_name's and console_print_va's other implicit arguments
//   (e.g. which model tag is searched) are not recovered.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "units.h"

extern data_array *object_data; // 0x008603b0

extern int16_t animation_graph_find_animation_by_name(uint32_t unit_index, const char *name); // 0x4d6ab0, UNSURE signature
extern void console_print_va(const char *format, ...); // 0x4c6920

void unit_scripting_set_emotion_animation(uint32_t unit_index, const char *emotion_name) // blam-cc: in_EAX, UNSURE 2nd arg
{
    if (unit_index != 0xffffffff) {
        object *unit_obj = ((object_header *)object_data->data)[unit_index & 0xffff].data;
        unit_data *unit = (unit_data *)((uint8_t *)unit_obj + k_unit_data_offset);
        int16_t region = animation_graph_find_animation_by_name(unit_index, emotion_name);
        if (region != -1) {
            unit->emotion_animation_index = region;
            return;
        }
        console_print_va("couldn't find the emotion animation '%s'", emotion_name);
    }
    return;
}

#if 0
Original Ghidra decompilation (0x569cf0):

void unit_scripting_set_emotion_animation(void)

{
  int iVar1;
  short sVar2;
  uint in_EAX;

  if (in_EAX != 0xffffffff) {
    iVar1 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (in_EAX & 0xffff) * 0xc);
    sVar2 = model_get_region_index_by_name();
    if (sVar2 != -1) {
      *(short *)(iVar1 + 0x21e) = sVar2;
      return;
    }
    console_print_va("couldn\'t find the emotion animation \'%s\'");
  }
  return;
}
#endif
