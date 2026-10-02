// widget_list_notify
// address 0x4ffca0, size 106 bytes
// name confidence: 0.8 (types/objects.h's own widget_type_definition.render comment names this
//   function directly: "widget_list_notify")
// rewrite confidence: 0.55
// evidence: types/objects.h object (first_widget 0x16c), widget (type 0x02, next_widget 0x08),
//   widget_type_definition (render 0x24); global 0x008603b0 object_data, 0x00860398 widget_data.
// register convention: Ghidra shows a single unresolved `unaff_EDI`; by the object_data lookup
//   shape shared with widget_new.c / widget_delete_all.c, EDI is the object index.
// blam-cc: EDI -> object_index, EBX -> render_context; stack -> render_arg
// FIXED (register inputs, objdump): EBX was missing entirely, and so was a stack argument
// (loaded into a local at 0x4ffcc1, `mov ebp,[esp+0x8]`) that Ghidra's own recognized signature
// dropped along with it. Both, plus ECX (= entry->instance) and EDI, are pushed at 0x4ffcfc..
// 0x4ffcff and passed straight through to the per-widget-type render hook -- the previous
// rewrite called that hook with zero arguments, which is not what the binary does. The lone
// caller (0x50f0f4, not in this batch) passes EDI = an object index local, a single stack dword
// (a field read out of another caller-owned record) for what is EBP/render_arg here, and EBX =
// the address of a caller-owned stack record (`lea ebx,[esp+0x1c]`) for render_context; neither
// value's own further meaning is resolvable without that caller's or the five per-type render
// hooks' (0x4fb980 etc., not in this batch) own address ranges, so both are kept opaque.

// VERIFIED against disassembly 0x4ffca0..0x4ffd0e (2026-09-30): hook (row +0x24, stride 0x28) called with (EDI, widget+4,
//   stack arg, EBX) cdecl.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern data_array *object_data; // 0x008603b0
extern data_array *widget_data; // 0x00860398
extern widget_type_definition widget_type_definitions[k_maximum_widget_types]; // 0x0069c010

// UNSURE: exact per-widget-type signature; these are just the 4 values widget_list_notify itself
// forwards unchanged (object_index, the widget's own instance handle, then its own EBP/EBX
// inputs), not independently confirmed against any of the five render hooks.
typedef void (*widget_render_proc)(uint32_t object_index, datum_index instance,
    uint32_t render_arg, void *render_context);

// blam-cc: EDI -> object_index, EBX -> render_context; stack -> render_arg
void widget_list_notify(uint32_t object_index /*EDI*/, uint32_t render_arg /*stack*/,
                        void *render_context /*EBX*/)
{
    object *obj = ((object_header *)object_data->data)[object_index & 0xffff].data;
    datum_index handle = obj->first_widget;

    while (handle != (datum_index)0xffffffff) {
        widget *entry = &((widget *)widget_data->data)[handle & 0xffff];
        if (widget_type_definitions[entry->type].render != 0) {
            ((widget_render_proc)widget_type_definitions[entry->type].render)(
                object_index, entry->instance, render_arg, render_context);
        }
        handle = entry->next_widget;
    }
}

#if 0
Original Ghidra decompilation (0x4ffca0):

void FUN_004ffca0(void)

{
  int iVar1;
  uint uVar2;
  uint unaff_EDI;

  uVar2 = *(uint *)(*(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (unaff_EDI & 0xffff) * 0xc) + 0x16c
                   );
  while (uVar2 != 0xffffffff) {
    iVar1 = *(int *)(DAT_00860398 + 0x34) + (uVar2 & 0xffff) * 0xc;
    if ((code *)(&PTR_LAB_0069c034)[*(short *)(iVar1 + 2) * 10] != (code *)0x0) {
      (*(code *)(&PTR_LAB_0069c034)[*(short *)(iVar1 + 2) * 10])();
    }
    uVar2 = *(uint *)(iVar1 + 8);
  }
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
