// widget_instance_is_input_eligible  (Ghidra: FUN_00499c40, unnamed)
// address 0x499c40, size 109 bytes
// name confidence: 0.35   rewrite confidence: 0.35
// evidence: shares the exact same parent-chain walk and focused_child check as the neighboring
// widget_instance_is_top_of_stack @0x499cb0, but starts with a `hidden` guard and, at each
// ancestor step, additionally accepts the chain when that ancestor's tag lacks
// UIWidgetDefinitionFlags::pass_unhandled_events_to_focused_child (bit 0) and is not itself a
// list type -- read as "would this widget actually receive input", a superset of "is visually on
// top". Sole caller is outside this session's range, so the exact use is not cross-checked.
// register convention: widget in EAX (in_EAX), unresolved register read.
// blam-cc: EAX -> widget
// UNSURE: the tag-data lookup used for each ancestor's flags/type check is keyed off that
// ancestor's OWN definition, resolved one iteration behind the parent-walk (matching Ghidra's
// `uVar2`/`iVar3` staggering exactly); preserved verbatim rather than "corrected" to look
// synchronous.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"
#include "cache.h"

extern tag_instance *tag_instances; // 0x0087bc14

// blam-cc: EAX -> widget
uint8_t widget_instance_is_input_eligible(widget_instance *widget)
{
    widget_instance *cursor;
    uint8_t result;
    widget_instance *tag_source; // Ghidra's uVar2 -- the widget whose tag_data is current
    void *tag_data;

    if (widget->hidden != 0) {
        return 0;
    }
    cursor = widget->parent;
    if (cursor == (widget_instance *)0) {
        return 1;
    }
    result = 1;
    tag_source = cursor;
    tag_data = tag_instances[cursor->definition & 0xffff].data;
    while (cursor != (widget_instance *)0 && result != 0) {
        UIWidgetDefinition *definition = (UIWidgetDefinition *)tag_data;

        tag_source = cursor;
        if ((definition->flags & 1) == 0 && cursor->widget_type != 2 && cursor->widget_type != 3) {
            result = 0;
        } else {
            result = 1;
        }
        cursor = cursor->parent;
        tag_data = tag_instances[tag_source->definition & 0xffff].data;
    }
    return result;
}

#if 0
Original Ghidra decompilation (0x499c40):

char FUN_00499c40(void)

{
  uint *puVar1;
  uint uVar2;
  int iVar3;
  char cVar4;
  int in_EAX;
  bool bVar5;

  if (*(char *)(in_EAX + 0x12) != '\0') {
    return '\0';
  }
  puVar1 = *(uint **)(in_EAX + 0x30);
  if (puVar1 != (uint *)0x0) {
    cVar4 = '\x01';
    iVar3 = *(int *)((*puVar1 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
    while ((puVar1 != (uint *)0x0 && (bVar5 = cVar4 != '\0', cVar4 = '\0', bVar5))) {
      uVar2 = *puVar1;
      if (((*(byte *)(iVar3 + 0x2c) & 1) == 0) &&
         ((*(short *)((int)puVar1 + 0xe) != 2 && (*(short *)((int)puVar1 + 0xe) != 3)))) {
        cVar4 = '\0';
      }
      else {
        cVar4 = '\x01';
      }
      puVar1 = (uint *)puVar1[0xc];
      iVar3 = *(int *)((uVar2 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
    }
    return cVar4;
  }
  return '\x01';
}
#endif
