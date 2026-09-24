// chimera__load_ui_widget  (Ghidra: chimera__load_ui_widget, already named)
// address 0x497a70, size 365 bytes
// name confidence: 0.7   rewrite confidence: 0.8
// evidence: types/interface.h's own prose calls this same address "widget_open" throughout its
// widget_instance/widget_history_node notes ("widget_open @0x497a70 maps
// definition->controller_index 0..4 to 0,1,2,3,-1"); kept as chimera__load_ui_widget per this
// module's already-assigned (non-FUN_xxx) name, matching the same "assigned name vs header
// prose" precedent as chimera__do_show_loading_screen.c.
// register convention: cdecl, seven stack arguments. Ghidra recognized only five; every call site
// pushes 0x1c bytes and the body reads [esp+0x34] and [esp+0x38] (arguments 6 and 7) when it
// builds the go-back record. Argument 3 is a parent widget, not a flag: widget_create_children_
// from_tag passes the parent, and the same value reaches widget_initialize_from_tag in EDX.
// Recovered from objdump this pass; the older 5-argument model and its invented history record
// were replaced.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"
#include "cache.h"

extern datum_index ui_cursor_bitmap; // 0x0068e67c
extern uint8_t ui_widget_opened;     // 0x00718fc8
extern tag_instance *tag_instances;  // 0x0087bc14
extern heap *widget_memory_pool;     // 0x006926c4
extern widget_instance *ui_root_widget[1];        // 0x00718f94
extern widget_history_node *ui_widget_history[3]; // 0x00718f98

extern datum_index tag_lookup(tag_group group, char *path); // 0x442550; blam-cc: group in EDI
extern void widget_close(widget_instance *widget); // 0x497c00
extern void list_node_prepend(widget_history_node *template_record, widget_history_node **head); // 0x499430
extern void widget_initialize_from_tag(widget_instance *widget, datum_index tag_index, widget_instance *parent,
                                       uint16_t controller_index, UIWidgetDefinition *tag); // 0x499780
extern void *heap_allocate(uint32_t size, heap *self); // 0x4d1f10

// Opens a UI widget: resolves the widget tag by index or by path, allocates a widget_instance
// from the widget heap and initializes it. With no parent the widget becomes the controller
// slot's root widget (closing whatever was there first) and, when history_definition names a
// widget whose tag does not set bit 0x4000 of definition+0x2c, a go-back record is pushed
// holding history_definition, history_list_definition, history_selection and the replaced
// root's controller_index. With a parent the widget is created as that parent's child and
// the root and history are left alone (widget_initialize_from_tag does the linking).
// Also re-caches the UI cursor bitmap tag and marks the UI as having opened a widget.
// blam-cc: cdecl, 7 stack arguments (every caller pushes 0x1c bytes); objdump 0x497a70..0x497bdc
widget_instance *chimera__load_ui_widget(char *tag_path, datum_index tag_index,
                                          widget_instance *parent, uint16_t controller_index,
                                          datum_index history_definition,
                                          datum_index history_list_definition,
                                          int16_t history_selection)
{
    widget_instance *widget = (widget_instance *)0;
    // `(controller == 0xffff) - 1 & controller`: slot 0 when unspecified, else the controller.
    int16_t slot = (controller_index == 0xffff) ? 0 : (int16_t)controller_index;
    UIWidgetDefinition *tag;

    ui_cursor_bitmap = tag_lookup(0x6269746d /* 'bitm' */, "ui\\shell\\bitmaps\\cursor");
    ui_widget_opened = 1;

    if (tag_index == (datum_index)-1) {
        tag_index = tag_lookup(0x44654c61 /* 'DeLa' */, tag_path);
        if (tag_index == (datum_index)-1) {
            return (widget_instance *)0;
        }
    }
    tag = (UIWidgetDefinition *)tag_instances[tag_index & 0xffff].data;
    widget = (widget_instance *)heap_allocate(sizeof(widget_instance), widget_memory_pool);
    if (widget == (widget_instance *)0) {
        return (widget_instance *)0;
    }

    if (parent == (widget_instance *)0) {
        widget_instance *previous_root = ui_root_widget[slot];
        int16_t previous_controller = -1;

        if (previous_root != (widget_instance *)0) {
            previous_controller = previous_root->controller_index;
            widget_close(previous_root);
        }
        ui_root_widget[slot] = widget;

        if (history_definition != (datum_index)-1) {
            uint8_t *history_tag_data = (uint8_t *)tag_instances[history_definition & 0xffff].data;

            if ((*(uint32_t *)(history_tag_data + 0x2c) & 0x4000) == 0) {
                widget_history_node history_template;

                history_template.definition = history_definition;
                history_template.list_definition = history_list_definition;
                history_template.selection = history_selection;
                history_template.controller_index = previous_controller;
                list_node_prepend(&history_template, &ui_widget_history[slot]);
            }
        }
    }

    if (controller_index == 0xffff) {
        // jump table at 0x497be0 over definition->controller_index 0..4; above 4 keeps 0xffff
        switch (*(int16_t *)((uint8_t *)tag + 2)) {
        case 0: controller_index = 0; break;
        case 1: controller_index = 1; break;
        case 2: controller_index = 2; break;
        case 3: controller_index = 3; break;
        case 4: controller_index = 0xffff; break;
        default: break;
        }
    }
    widget_initialize_from_tag(widget, tag_index, parent, controller_index, tag);
    return widget;
}

#if 0
Original Ghidra decompilation (0x497a70):

int chimera__load_ui_widget
              (undefined4 param_1,uint param_2,int param_3,undefined4 param_4,uint param_5)

{
  int iVar1;
  ushort uVar2;
  int iVar3;
  ushort uVar4;

  iVar3 = 0;
  uVar2 = (ushort)param_4;
  uVar4 = (uVar2 == 0xffff) - 1 & uVar2;
  DAT_0068e67c = tag_lookup("ui\\shell\\bitmaps\\cursor");
  DAT_00718fc8 = 1;
  if ((param_2 != 0xffffffff) || (param_2 = tag_lookup(param_1), param_2 != 0xffffffff)) {
    iVar1 = *(int *)((param_2 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
    iVar3 = heap_allocate();
    if (iVar3 != 0) {
      if (param_3 == 0) {
        if ((&DAT_00718f94)[(short)uVar4] != 0) {
          widget_close((&DAT_00718f94)[(short)uVar4]);
        }
        (&DAT_00718f94)[(short)uVar4] = iVar3;
        if ((param_5 != 0xffffffff) &&
           ((*(uint *)(*(int *)((param_5 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14) + 0x2c) & 0x4000)
            == 0)) {
          FUN_00499430();
        }
      }
      if (uVar2 == 0xffff) {
        switch(*(undefined2 *)(iVar1 + 2)) {
        case 0:
          param_4 = 0;
          break;
        case 1:
          param_4 = 1;
          break;
        case 2:
          param_4 = 2;
          break;
        case 3:
          param_4 = 3;
          break;
        case 4:
          param_4 = 0xffffffff;
        }
      }
      widget_initialize_from_tag(param_4,iVar1);
    }
  }
  return iVar3;
}
#endif
