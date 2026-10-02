// glow_render_dispatch
// address 0x4fcdb0, size 199 bytes, zero recorded callers (the widget_type_definition render
//   column for glow (row 2) calls this indirectly, per out/phase4/objects_types_notes.md)
// name confidence: 0.7 (out/phase4/objects_types_notes.md's misattribution table: "0x4fcdb0 |
//   lightning_instance_update | glow_render_dispatch (widget type 2 render column)"; its own
//   callees FUN_004fce80/FUN_004fe570, inherited as lightning_update/lightning_render, are
//   renamed glow_update/glow_render by that same table)
// rewrite confidence: 0.55
// evidence: types/objects.h glow (definition_tag 0x224); types/tags.h Glow
//   (attachment_marker TagString at offset 0, doubling as the marker-name char* the same way
//   Antenna's attachment_marker_name does in antenna_apply_marker_delta.c); global 0x008603a0
//   glow_data (data_array, size 0x22, last_index 0x2e, data 0x34).
// register convention: Ghidra shows this function's own two parameters (object_index,
//   glow_handle) cleanly, but glow_update (0x4fce80) and glow_render (0x4fe570) both read
//   implicit inputs this function sets up in registers just before each call; resolved by
//   disassembling 0x4fcdb0 directly (objdump -d -M intel bin/halo.exe): `mov edi,eax; call
//   0x4fce80` (EDI = the glow entry pointer just validated, possibly NULL) and `mov ecx,esi;
//   call 0x4fe570` (ECX = the low 16 bits of glow_handle, i.e. the raw index).
// blam-cc: stack -> object_index, glow_handle
// UNSURE: the fallback path when the handle validation fails sets the lookup pointer to NULL
//   and then unconditionally dereferences it at +0x224 anyway; preserved literally rather than
//   guarded, since "preserve semantics exactly" takes priority and this may simply never be
//   reached with an invalid handle in practice.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern data_array *glow_data; // 0x008603a0
extern tag_instance *tag_instances; // 0x0087bc14

extern void glow_update(uint32_t object_index, glow *entry /*EDI*/); // this module, 0x4fce80
extern int32_t object_get_node_local_transform(uint32_t object_index, char *marker_name,
                                                object_marker *marker, uint32_t flags); // 0x4f6080
extern void glow_render(datum_index glow_handle /*ECX*/); // this module, 0x4fe570

void glow_render_dispatch(uint32_t object_index, datum_index glow_handle)
    // blam-cc: stack -> object_index, glow_handle
{
    glow *entry;

    if (object_index == 0xffffffff || glow_handle == (datum_index)0xffffffff) {
        return;
    }

    {
        int16_t index = (int16_t)glow_handle;
        int16_t salt = (int16_t)(glow_handle >> 16);

        entry = 0;
        if (index >= 0 && index < glow_data->last_index) {
            entry = (glow *)((uint8_t *)glow_data->data + glow_data->size * index);
            if (entry->identifier == 0 || (salt != 0 && salt != entry->identifier)) {
                entry = 0;
            }
        }
    }

    {
        void *glow_tag_data = tag_instances[entry->definition_tag & 0xffff].data; // UNSURE: see
            // file header (entry may be NULL here on the invalid-handle path)
        object_marker marker;

        glow_update(object_index, entry);
        object_get_node_local_transform(object_index, (char *)glow_tag_data, &marker, 1);
        glow_render(glow_handle);
    }
}

#if 0
Original Ghidra decompilation (0x4fcdb0):

void FUN_004fcdb0(int param_1,int param_2)

{
  undefined4 uVar1;
  short *psVar2;
  short sVar3;
  short sVar4;
  undefined1 local_6c [108];

  if (param_1 == -1) {
    return;
  }
  if (param_2 == -1) {
    return;
  }
  sVar3 = (short)param_2;
  sVar4 = (short)((uint)param_2 >> 0x10);
  if ((-1 < sVar3) && (sVar3 < *(short *)(DAT_008603a0 + 0x2e))) {
    psVar2 = (short *)((int)*(short *)(DAT_008603a0 + 0x22) * (int)sVar3 +
                      *(int *)(DAT_008603a0 + 0x34));
    if ((*psVar2 != 0) && ((sVar4 == 0 || (sVar4 == *psVar2)))) goto LAB_004fce08;
  }
  psVar2 = (short *)0x0;
LAB_004fce08:
  uVar1 = *(undefined4 *)((*(uint *)(psVar2 + 0x112) & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
  lightning_update(param_1);
  FUN_004f6080(param_1,uVar1,local_6c,1);
  lightning_render();
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
