// hs_effect_spawn_on_marker  (Ghidra: FUN_004888f0)
// address 0x4888f0, size 97 bytes
// name confidence: 0.5   rewrite confidence: 0.9
// REWRITTEN (objdump 0x4888f0..0x488950; the draft swapped the effect and the marker name and dropped the
//   register arguments of both callees). ESI = object, EDI = effect definition, [esp+4] = marker name. With both
//   set, the first marker of that name (object_get_node_local_transform(object, name, &marker, 1)) places a new
//   effect: effect_new_on_object_with_node_table(EAX -1, ECX effect, EDX object, stack: the marker's first dword
//   (node index), 1, &name (a one-entry name table: the argument slot itself), &marker node_transform.position
//   (+0x60), &node_transform.forward (+0x3c), 1.0, 1.0, 0, 0).
// blam-cc: ESI -> object_index, EDI -> effect, stack -> marker_name

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "effects.h"
#include "fn_hs.h"

extern int32_t object_get_node_local_transform(uint32_t object_index, char *marker_name, object_marker *marker,
    uint32_t maximum); // 0x4f6080
extern datum_index effect_new_on_object_with_node_table(datum_index creator_object_index,
    datum_index definition_index, datum_index object_index, uint16_t node_index,
    uint16_t ctx_08, uint32_t ctx_0c, uint32_t ctx_10, uint32_t ctx_14, real a_scale,
    real b_scale, const ColorRGB *color, const effect_tint_source *tint_source); // 0x450870, EAX, ECX, EDX, stack

void hs_effect_spawn_on_marker(datum_index object_index, datum_index effect, char *marker_name)
{
    object_marker marker;

    if (effect == k_datum_index_none || object_index == k_datum_index_none) {
        return;
    }
    if ((int16_t)object_get_node_local_transform(object_index, marker_name, &marker, 1) == 0) {
        return;
    }
    effect_new_on_object_with_node_table(k_datum_index_none, effect, object_index, *(uint16_t *)&marker,
        1, (uint32_t)&marker_name, (uint32_t)((uint8_t *)&marker + 0x60), (uint32_t)((uint8_t *)&marker + 0x3c),
        1.0f, 1.0f, 0, 0);
}

#if 0
Original Ghidra decompilation (0x4888f0):

void FUN_004888f0(undefined4 param_1)

{
  short sVar1;
  int unaff_ESI;
  int unaff_EDI;
  undefined4 local_6c;
  undefined1 local_30 [36];
  undefined1 local_c [12];

  if ((unaff_EDI != -1) && (unaff_ESI != -1)) {
    sVar1 = FUN_004f6080();
    if (sVar1 != 0) {
      FUN_00450870(local_6c,1,&param_1,local_c,local_30,0x3f800000,0x3f800000,0,0);
    }
  }
  return;
}
#endif
