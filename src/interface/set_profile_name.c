// set_profile_name  (Ghidra: set_profile_name, already named)
// address 0x49c710, size 149 bytes
// name confidence: 0.8   rewrite confidence: 0.4
// evidence: matches the given name; functions.md: "Builds a profile display name by formatting a
// base name together with an optional localized suffix string looked up from the common button
// captions string list." Three already-rewritten callers (ui_map_list_carousel_refresh_window.c,
// ui_profile_details_list_widget_build.c, ui_selection_list_mirror_value_build.c) already fixed
// this as a widget-text builder that "writes through an inherited EBX", but all three declare it
// with only the one recognized stack parameter (name_source) and never pass a widget argument.
// register convention: EBX -> widget (unaff_EBX, unresolved register read: widget->text at +0x3c
// is written directly), stack -> name_source. // blam-cc: EBX -> widget, stack -> name_source
// UNSURE: this is a real cross-file arity gap (the 3 callers above are missing the widget/EBX
// argument this function actually needs); flagged for a follow-up pass rather than fixed here.
// The format string's argument order ("%s %s", suffix_or_default, name_source) is preserved
// exactly as decompiled even though it reads backwards from the English summary above.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"
#include <wchar.h>
#include "cache.h"

extern tag_instance *tag_instances; // 0x0087bc14
extern heap *widget_memory_pool;    // 0x006926c4
extern uint16_t default_profile_name_suffix_00671fac[]; // 0x00671fac, UNSURE name

extern datum_index tag_lookup(tag_group group, char *path); // 0x442550; blam-cc: group in EDI
extern void *heap_reallocate(void *old_payload, uint32_t new_size, heap *self); // 0x4d1f80, blam-cc: EAX old, ESI self
extern void string_format_wide_va_bounded(uint32_t count, uint16_t *dest, const uint16_t *format, ...); // 0x557910, EDX count

// blam-cc: EBX -> widget, stack -> name_source
// Allocates a 0x80 byte (64 wide char) text buffer for `widget` and formats it as
// "<suffix> <name_source>", where suffix is the 8th entry of the common button captions string
// list (its own last character stripped) if that list has at least 8 entries and the entry is
// non-empty, else a compiled-in default suffix string.
void set_profile_name(widget_instance *widget, const uint16_t *name_source)
{
    datum_index tag_id = tag_lookup(0x75737472 /* 'ustr' */, "ui\\shell\\strings\\common_button_captions");
    uint16_t *suffix = default_profile_name_suffix_00671fac;
    void *buffer = heap_reallocate(widget->text, 0x80, widget_memory_pool);

    widget->text = buffer;
    if (buffer != (void *)0) {
        if (tag_id != (datum_index)-1) {
            UnicodeStringList *list = (UnicodeStringList *)tag_instances[tag_id & 0xffff].data;

            if (list->strings.count > 7) {
                UnicodeStringListString *strings = (UnicodeStringListString *)list->strings.pointer;
                uint32_t size = strings[7].string.size;

                if ((int32_t)size > 0) {
                    suffix = (uint16_t *)strings[7].string.pointer;
                    // Strip the string's own last character (rounding the byte size down to even).
                    *(uint16_t *)((uint8_t *)suffix + ((size & 0xfffffffe) - 2)) = 0;
                }
            }
        }
        string_format_wide_va_bounded(0x3f, (wchar_t *)buffer, L"%s %s", suffix, name_source);
        ((uint16_t *)widget->text)[0x3f] = 0;
    }
}

#if 0
Original Ghidra decompilation (0x49c710):

void set_profile_name(undefined4 param_1)

{
  int *piVar1;
  uint uVar2;
  int iVar3;
  undefined **ppuVar4;
  int unaff_EBX;

  uVar2 = tag_lookup("ui\\shell\\strings\\common_button_captions");
  iVar3 = heap_reallocate(0x80);
  *(int *)(unaff_EBX + 0x3c) = iVar3;
  if (iVar3 != 0) {
    ppuVar4 = &PTR_DAT_00671fac;
    if ((uVar2 != 0xffffffff) &&
       (piVar1 = *(int **)((uVar2 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14), 7 < *piVar1)) {
      iVar3 = piVar1[1];
      uVar2 = *(uint *)(iVar3 + 0x8c);
      if (0 < (int)uVar2) {
        ppuVar4 = *(undefined ***)(iVar3 + 0x98);
        *(undefined2 *)((int)ppuVar4 + ((uVar2 & 0xfffffffe) - 2)) = 0;
      }
    }
    string_format_wide_va_bounded(*(undefined4 *)(unaff_EBX + 0x3c),L"%s %s",ppuVar4,param_1);
    *(undefined2 *)(*(int *)(unaff_EBX + 0x3c) + 0x7e) = 0;
  }
  return;
}
#endif
