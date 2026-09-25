// hs_effect_spawn_on_marker  (Ghidra: FUN_004888f0)
// address 0x4888f0, size 97 bytes
// name confidence: 0.3 (out/phase4/hs_functions.md: "Spawns a visual effect attached to an
//   object's marker, gated on the object and marker both being valid")
// rewrite confidence: 0.3
// evidence: object_get_node_local_transform's established 4-argument shape from hs_object_detach_and_place_at_location.c.
// register convention: object index in ESI (unaff_ESI); marker index in EDI (unaff_EDI); effect
//   reference as the recognized stack parameter (param_1).
//   // blam-cc: ESI -> object_index, EDI -> marker_index, stack -> effect
// UNSURE: object_get_node_local_transform and FUN_00450870 are both called with zero visible arguments in the
// decompile; the bindings below (object_index/marker_index into object_get_node_local_transform, the two local
// buffers as its outputs, and the final FUN_00450870 call's argument order) are inferred from
// the established shape of the sibling calls elsewhere in this module and are not verified
// against disassembly.

#include "tags.h"
#include "memory.h"
#include "hs.h"

extern int16_t object_get_node_local_transform(datum_index object_index, void *marker_index, void *out_buffer,
    char param_4);                                              // objects module, 0x4f6080
extern void effect_new_on_object_with_node_table(uint32_t effect, int32_t param_2, void *param_3, void *position,
    void *orientation, float param_6, float param_7, int32_t param_8, int32_t param_9);
                                                                  // effects module, 0x450870

// If both `object_index` and `marker_index` are valid and object_get_node_local_transform resolves the marker's
// transform, spawns `effect` there via FUN_00450870.
void hs_effect_spawn_on_marker(datum_index object_index, datum_index marker_index, uint32_t effect)
{
    uint8_t position[12];
    uint8_t orientation[36];
    int16_t resolved;

    if (object_index != k_datum_index_none && marker_index != k_datum_index_none) {
        resolved = object_get_node_local_transform(object_index, (void *)(uint32_t)marker_index, orientation, 1);
        if (resolved != 0) {
            effect_new_on_object_with_node_table(effect, 1, &effect, position, orientation, 1.0f, 1.0f, 0, 0);
        }
    }
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
