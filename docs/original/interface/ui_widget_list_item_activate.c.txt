// ui_widget_list_item_activate  (Ghidra: ui_widget_list_item_activate, already named)
// address 0x49a430, size 1148 bytes
// name confidence: 0.5   rewrite confidence: 0.65
// evidence: matches the given name; `param_4` is a types/tags.h EventHandlerReference (field
// offsets confirmed against that struct's real layout: flags@0, event_type@4, function@6,
// widget_tag.tag_id@0x14, sound_effect.tag_id@0x24, script@0x28), and each EventHandlerReferences
// Flags bit (close_current_widget=1, close_other_widget=2, close_all_widgets=4, open_widget=8,
// give_focus_to_widget=0x40, replace_self_w_widget=0x100, go_back_to_previous_widget=0x200,
// run_function=0x80 tested via the flags byte's sign bit, run_scenario_script=0x400,
// try_to_branch_on_failure=0x800) drives one clause each, matching the type's own name.
// register convention: cdecl, all five recognized stack parameters.
// UNSURE: hs_script_find_by_name / hs_evaluate_expression (scenario HS script module) and
// sound_play_new (sound module, "play a positioned sound") are declared with best-guess signatures
// only; their own decompiles were not read in this session.

// Phase-4 review against objdump 0x49a430..0x49a8b8: a successful run_function now falls through
// to the other actions (the first rewrite made them an else branch), a failed one jumps straight
// to the branch-on-failure scan, the scenario script root expression is read from the 0x5c stride
// scripts block, the final sound is the action kind (one-based ui_sound_effect), and the helper
// calls carry their register arguments.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"
#include "cache.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern Scenario *global_scenario; // 0x00746f8c
extern void *ui_event_function_table[0xbe]; // 0x006927d0
extern widget_instance *ui_root_widget[1]; // 0x00718f94
extern widget_history_node *ui_widget_history[3]; // 0x00718f98
extern heap *widget_memory_pool; // 0x006926c4

extern int16_t hs_script_find_by_name(char *name); // 0x4833a0, UNSURE signature
extern void hs_evaluate_expression(int32_t expression); // 0x48a250, UNSURE signature
extern widget_instance *chimera__load_ui_widget(char *tag_path, datum_index tag_index,
    widget_instance *parent, uint16_t controller_index, datum_index history_definition,
    datum_index history_list_definition, int16_t history_selection); // 0x497a70, 7 stack args (objdump)
extern void widget_close(widget_instance *widget); // 0x497c00
extern void widget_play_sound_effect(int16_t effect_id); // 0x498e90
extern widget_instance *widget_find_by_tag_id(widget_instance *widget, datum_index tag_id); // 0x499950
extern void widget_instance_relink_focus(widget_instance *widget, widget_instance *child); // 0x49bba0, focus change; blam-cc: EAX -> widget, ECX -> child (objdump 0x49bba0: walks EAX up +0x30, tests ECX+0x12)
extern void widget_instance_close_and_restore_previous(widget_instance *widget); // 0x49c3e0; blam-cc: EAX -> widget (objdump 0x49a6fb)
extern widget_instance *widget_reopen_as_root_with_history(widget_instance *widget, datum_index open_tag); // 0x49c4c0, per src/interface/widget_close.c
extern void sound_play_new(datum_index sound_tag, void *position, int32_t unknown1, int32_t unknown2,
                          void *callback_data, int32_t unknown4, int32_t unknown5); // 0x549af0, UNSURE signature

// Executes the set of actions encoded in a widget event/list-item definition (scenario script
// call, run_function callback, close/open/replace/go-back widget actions, focus change, sound)
// when that handler is triggered, honoring try_to_branch_on_failure for the tag's own
// conditional_widgets when a run_function callback fails.
void ui_widget_list_item_activate(widget_instance *widget, UIWidgetDefinition *tag, int16_t *event,
                                   EventHandlerReference *handler, uint8_t *out_handled)
{
    uint8_t close_self_via_root = 0; // Ghidra's bVar5
    uint8_t handled = 0;             // local_49
    uint8_t ok = 1;                  // local_48
    uint8_t function_failed = 0;     // local_45
    uint8_t close_current = 0;       // local_46
    uint8_t close_all = 0;           // local_47
    int32_t action_kind = 0;         // local_44

    if ((handler->flags & 0x400) != 0 && handler->script.string[0] != 0) {
        int16_t script_index = hs_script_find_by_name(handler->script.string);

        if (script_index != -1) {
            // objdump 0x49a47f: scenario scripts block pointer at +0x4a0, 0x5c stride, root
            // expression at +0x24 of the entry
            uint8_t *scripts = *(uint8_t **)((uint8_t *)global_scenario + 0x4a0);

            hs_evaluate_expression(*(int32_t *)(scripts + (int32_t)script_index * 0x5c + 0x24));
        }
    }

    if ((int8_t)handler->flags < 0 && handled == 0) {
        int16_t function_id = handler->function;

        if (function_id < 0 || function_id > 0xbd ||
            ((ui_event_function)ui_event_function_table[function_id])(widget, event, &handled) == 0) {
            function_failed = 1;
            goto after_run_function; // objdump 0x49a4d6: a failed run_function skips every other action
        }
    }
    { // a successful run_function falls through into the remaining actions (objdump 0x49a4d4)
        if ((handler->flags & 0x40) != 0 && handled == 0) { // give_focus_to_widget
            if (*(uint32_t *)&handler->widget_tag.tag_id == 0xffffffffu) {
                ok = 0;
            } else {
                widget_instance *root = widget;
                widget_instance *found;

                while (root->parent != (widget_instance *)0) {
                    root = root->parent;
                }
                found = widget_find_by_tag_id(root, *(datum_index *)&handler->widget_tag.tag_id);
                if (found != (widget_instance *)0) {
                    widget_instance_relink_focus(root, found); // objdump 0x49a517: EAX is the root walked up from widget
                }
                action_kind = 1;
            }
        }
        if ((handler->flags & 0x20) != 0 && handled == 0 &&
            *(uint32_t *)&handler->widget_tag.tag_id == 0xffffffffu) {
            ok = 0;
        }
        if ((handler->flags & 1) != 0 && handled == 0) { // close_current_widget
            close_current = 1;
        }
        if ((handler->flags & 2) != 0 && handled == 0 &&
            *(uint32_t *)&handler->widget_tag.tag_id != 0xffffffffu) { // close_other_widget
            widget_instance *found = (widget_instance *)0;
            int32_t i;

            for (i = 0; i < 1; i++) {
                if (ui_root_widget[i] != (widget_instance *)0) {
                    found = widget_find_by_tag_id(ui_root_widget[i], *(datum_index *)&handler->widget_tag.tag_id);
                    if (found != (widget_instance *)0) {
                        break;
                    }
                }
            }
            if (found == (widget_instance *)0) {
                ok = 0;
            } else if (found == widget) {
                close_self_via_root = 1;
            } else {
                widget_close(found);
            }
        }
        if ((handler->flags & 4) != 0 && handled == 0) { // close_all_widgets
            close_all = 1;
        }
        if ((handler->flags & 8) != 0 && *(uint32_t *)&handler->widget_tag.tag_id != 0xffffffffu) { // open_widget
            if (widget_reopen_as_root_with_history(widget, *(datum_index *)&handler->widget_tag.tag_id) == 0) {
                ok = 0;
            } else {
                if (action_kind == 0) action_kind = 2;
                handled = 1;
            }
        }
        if ((handler->flags & 0x100) != 0 && handled == 0 &&
            *(uint32_t *)&handler->widget_tag.tag_id != 0xffffffffu) { // replace_self_w_widget
            widget_instance *replacement =
                chimera__load_ui_widget((char *)0, *(datum_index *)&handler->widget_tag.tag_id,
                                         widget, widget->controller_index, (datum_index)-1,
                                         (datum_index)-1, -1);

            if (replacement == (widget_instance *)0) {
                ok = 0;
            } else {
                widget_instance *parent = widget->parent;
                widget_instance *next = widget->next_sibling;
                widget_instance *prev = widget->previous_sibling;
                int32_t i;

                if (replacement->previous_sibling != (widget_instance *)0) {
                    replacement->previous_sibling->next_sibling = (widget_instance *)0;
                }
                replacement->previous_sibling = (widget_instance *)0;
                replacement->parent = (widget_instance *)0;
                replacement->local_x = replacement->local_x + widget->local_x;
                replacement->local_y = replacement->local_y + widget->local_y;
                if (parent != (widget_instance *)0) {
                    replacement->parent = parent;
                    if (parent->first_child == widget) parent->first_child = replacement;
                    if (parent->focused_child == widget) parent->focused_child = replacement;
                }
                if (next != (widget_instance *)0) {
                    next->previous_sibling = replacement;
                }
                replacement->next_sibling = next;
                if (prev != (widget_instance *)0) {
                    prev->next_sibling = replacement;
                }
                replacement->previous_sibling = prev;
                for (i = 0; i < 1; i++) {
                    if (ui_root_widget[i] == replacement) {
                        ui_root_widget[i] = (widget_instance *)0;
                        break;
                    }
                }
                if (action_kind == 0) action_kind = 2;
                widget->previous_sibling = (widget_instance *)0;
                widget->next_sibling = (widget_instance *)0;
                widget->parent = (widget_instance *)0;
                close_self_via_root = 1;
            }
        }
        if ((handler->flags & 0x200) != 0) { // go_back_to_previous_widget
            widget_instance_close_and_restore_previous(widget);
            if (action_kind == 0) action_kind = 3;
            handled = 1;
        }
        if (*(uint32_t *)&handler->sound_effect.tag_id != 0xffffffffu) {
            // objdump 0x49a71d: a 0x0c byte record {int16 0, pad, float 1.0, float 1.0} on the stack;
            // the zero float matches the int16 zero in the low half, the pad bytes are not written
            float position[3] = {0.0f, 1.0f, 1.0f};

            sound_play_new(*(datum_index *)&handler->sound_effect.tag_id, position, -1, 0, 0, 0, 0);
        }
        if (close_all == 0) {
            widget_instance *target = widget;

            if (close_current != 0) {
                while (target->parent != (widget_instance *)0) {
                    target = target->parent;
                }
                widget_close(target);
                handled = 1;
                goto after_close;
            }
            if (close_self_via_root != 0) {
                widget_close(target);
                handled = 1;
                goto after_close;
            }
        } else {
            if (ui_root_widget[0] != (widget_instance *)0) {
                widget_close(ui_root_widget[0]);
            }
            {
                widget_history_node *node = ui_widget_history[0];

                while (node != (widget_history_node *)0) {
                    heap_block *block = (heap_block *)((uint8_t *)node - 0x10);
                    uint32_t size = block->size;
                    int32_t slot = block->slot;

                    ui_widget_history[0] = node->next;
                    if (block->previous != (heap_block *)0) block->previous->next = block->next;
                    if (block->next != (heap_block *)0) block->next->previous = block->previous;
                    if (block == widget_memory_pool->first_block) widget_memory_pool->first_block = block->next;
                    if (block == widget_memory_pool->last_block) widget_memory_pool->last_block = block->previous;
                    widget_memory_pool->blocks[slot] = (heap_block *)0;
                    widget_memory_pool->next_free_slot =
                        (widget_memory_pool->first_block != (heap_block *)0) ? slot : 0;
                    widget_memory_pool->bytes_allocated -= (int32_t)(size & 0x7fffffff);
                    widget_memory_pool->allocation_count -= 1;
                    node = ui_widget_history[0];
                }
            }
            handled = 1;
        }
        if (ok != 0) {
            goto after_run_function;
        }
    }

after_close:
after_run_function:
    if ((handler->flags & 0x800) != 0 && tag->conditional_widgets.count > 0) { // try_to_branch_on_failure
        uint8_t *entries = (uint8_t *)tag->conditional_widgets.pointer;
        int32_t i;

        for (i = 0; i < tag->conditional_widgets.count; i++) {
            ConditionalWidgetReference *entry = (ConditionalWidgetReference *)(entries + i * 0x50);

            if (function_failed == 1 && (entry->flags & 1) != 0 && handled == 0) {
                datum_index open_tag = *(uint32_t *)&entry->widget_tag.tag_id;

                if (open_tag != (datum_index)-1 && widget_reopen_as_root_with_history(widget, open_tag) != 0) {
                    handled = 1;
                }
            }
        }
    }

    widget_play_sound_effect((int16_t)action_kind); // objdump 0x49a89e: EAX = the action kind, a one-based ui_sound_effect (0 plays nothing)
    *out_handled = handled;
}

#if 0
Original Ghidra decompilation (0x49a430):

void ui_widget_list_item_activate
               (int param_1,int param_2,undefined4 param_3,uint *param_4,char *param_5)

{
  ushort uVar1;
  uint uVar2;
  int iVar3;
  uint uVar4;
  bool bVar5;
  undefined *puVar6;
  undefined *puVar7;
  char cVar8;
  short sVar9;
  uint *puVar10;
  int iVar11;
  int *piVar12;
  int iVar13;
  int iVar14;
  char local_49;
  char local_48;
  char local_47;
  char local_46;
  char local_45;
  int local_44;
  undefined2 auStack_40 [2];
  undefined4 uStack_3c;
  undefined4 uStack_38;

  bVar5 = false;
  local_49 = '\0';
  local_48 = '\x01';
  local_45 = '\0';
  local_44 = 0;
  local_46 = '\0';
  local_47 = '\0';
  if ((((*param_4 & 0x400) != 0) && ((byte)param_4[10] != 0)) &&
     (sVar9 = hs_script_find_by_name(param_4 + 10), sVar9 != -1)) {
    hs_evaluate_expression(*(undefined4 *)(sVar9 * 0x5c + 0x24 + *(int *)(global_scenario + 0x4a0)))
    ;
  }
  if ((((char)(byte)*param_4 < '\0') && (local_49 == '\0')) &&
     ((uVar1 = *(ushort *)((int)param_4 + 6), (short)uVar1 < 0 ||
      ((0xbd < uVar1 ||
       (cVar8 = (*(code *)(&PTR_LAB_006927d0)[(short)uVar1])(param_1,param_3,&local_49),
       cVar8 == '\0')))))) {
    cVar8 = '\x01';
  }
  else {
    if (((*param_4 & 0x40) != 0) && (local_49 == '\0')) {
      if (param_4[5] == 0xffffffff) {
        local_48 = '\0';
      }
      else {
        iVar13 = *(int *)(param_1 + 0x30);
        iVar14 = param_1;
        while (iVar11 = iVar13, iVar11 != 0) {
          iVar14 = iVar11;
          iVar13 = *(int *)(iVar11 + 0x30);
        }
        iVar14 = widget_find_by_tag_id(iVar14,param_4[5]);
        if (iVar14 != 0) {
          FUN_0049bba0();
        }
        local_44 = 1;
      }
    }
    uVar2 = *param_4;
    if ((((uVar2 & 0x20) != 0) && (local_49 == '\0')) && (param_4[5] == 0xffffffff)) {
      local_48 = '\0';
    }
    if (((uVar2 & 1) != 0) && (local_49 == '\0')) {
      local_46 = '\x01';
    }
    if ((((uVar2 & 2) != 0) && (local_49 == '\0')) && (uVar2 = param_4[5], uVar2 != 0xffffffff)) {
      iVar14 = 0;
      piVar12 = &DAT_00718f94;
      do {
        if (iVar14 != 0) goto LAB_0049a59b;
        if (*piVar12 != 0) {
          iVar14 = widget_find_by_tag_id(*piVar12,uVar2);
        }
        piVar12 = piVar12 + 1;
      } while ((int)piVar12 < 0x718f98);
      if (iVar14 == 0) {
        local_48 = '\0';
      }
      else {
LAB_0049a59b:
        if (iVar14 == param_1) {
          bVar5 = true;
        }
        else {
          widget_close(iVar14);
        }
      }
    }
    if (((*param_4 & 4) != 0) && (local_49 == '\0')) {
      local_47 = '\x01';
    }
    if (((*param_4 & 8) != 0) && (param_4[5] != 0xffffffff)) {
      iVar14 = FUN_0049c4c0(param_1,param_4[5]);
      if (iVar14 == 0) {
        local_48 = '\0';
      }
      else {
        if (local_44 == 0) {
          local_44 = 2;
        }
        local_49 = '\x01';
      }
    }
    if ((((*param_4 & 0x100) != 0) && (local_49 == '\0')) && (param_4[5] != 0xffffffff)) {
      iVar14 = chimera__load_ui_widget
                         (0,param_4[5],param_1,*(undefined2 *)(param_1 + 8),0xffffffff,0xffffffff,
                          0xffffffff);
      if (iVar14 == 0) {
        local_48 = '\0';
      }
      else {
        iVar13 = *(int *)(param_1 + 0x30);
        iVar11 = *(int *)(param_1 + 0x2c);
        iVar3 = *(int *)(param_1 + 0x28);
        if (*(int *)(iVar14 + 0x28) != 0) {
          *(undefined4 *)(*(int *)(iVar14 + 0x28) + 0x2c) = 0;
        }
        *(undefined4 *)(iVar14 + 0x28) = 0;
        *(undefined4 *)(iVar14 + 0x30) = 0;
        *(short *)(iVar14 + 10) = *(short *)(iVar14 + 10) + *(short *)(param_1 + 10);
        *(short *)(iVar14 + 0xc) = *(short *)(iVar14 + 0xc) + *(short *)(param_1 + 0xc);
        if (iVar13 != 0) {
          *(int *)(iVar14 + 0x30) = iVar13;
          if (*(int *)(iVar13 + 0x34) == param_1) {
            *(int *)(iVar13 + 0x34) = iVar14;
          }
          if (*(int *)(iVar13 + 0x38) == param_1) {
            *(int *)(iVar13 + 0x38) = iVar14;
          }
        }
        if (iVar11 != 0) {
          *(int *)(iVar11 + 0x28) = iVar14;
        }
        *(int *)(iVar14 + 0x2c) = iVar11;
        if (iVar3 != 0) {
          *(int *)(iVar3 + 0x2c) = iVar14;
        }
        *(int *)(iVar14 + 0x28) = iVar3;
        iVar13 = 0;
        do {
          if ((&DAT_00718f94)[iVar13] == iVar14) {
            (&DAT_00718f94)[iVar13] = 0;
            break;
          }
          iVar13 = iVar13 + 1;
        } while (iVar13 < 1);
        if (local_44 == 0) {
          local_44 = 2;
        }
        *(undefined4 *)(param_1 + 0x28) = 0;
        *(undefined4 *)(param_1 + 0x2c) = 0;
        *(undefined4 *)(param_1 + 0x30) = 0;
        bVar5 = true;
      }
    }
    if ((*param_4 & 0x200) != 0) {
      FUN_0049c3e0();
      if (local_44 == 0) {
        local_44 = 3;
      }
      local_49 = '\x01';
    }
    if (param_4[9] != 0xffffffff) {
      auStack_40[0] = 0;
      uStack_3c = 0x3f800000;
      uStack_38 = 0x3f800000;
      FUN_00549af0(param_4[9],auStack_40,0xffffffff,0,0,0,0);
    }
    if (local_47 == '\0') {
      iVar14 = param_1;
      if (local_46 != '\0') {
        iVar13 = *(int *)(param_1 + 0x30);
        while (iVar11 = iVar13, iVar11 != 0) {
          iVar14 = iVar11;
          iVar13 = *(int *)(iVar11 + 0x30);
        }
LAB_0049a813:
        widget_close(iVar14);
        goto LAB_0049a81b;
      }
      if (bVar5) goto LAB_0049a813;
    }
    else {
      puVar6 = PTR_PTR_006926c4;
      iVar14 = DAT_00718f98;
      if (DAT_00718f94 != 0) {
        widget_close(DAT_00718f94);
        puVar6 = PTR_PTR_006926c4;
        iVar14 = DAT_00718f98;
      }
      while (puVar7 = PTR_PTR_006926c4, PTR_PTR_006926c4 = puVar6, DAT_00718f98 = iVar14,
            iVar14 != 0) {
        DAT_00718f98 = *(int *)(iVar14 + 0xc);
        puVar10 = (uint *)(iVar14 + -0x10);
        uVar2 = *puVar10;
        uVar4 = *(uint *)(iVar14 + -0xc);
        if (*(int *)(iVar14 + -8) != 0) {
          *(undefined4 *)(*(int *)(iVar14 + -8) + 0xc) = *(undefined4 *)(iVar14 + -4);
        }
        if (*(int *)(iVar14 + -4) != 0) {
          *(undefined4 *)(*(int *)(iVar14 + -4) + 8) = *(undefined4 *)(iVar14 + -8);
        }
        if (puVar10 == *(uint **)(puVar7 + 0x2c)) {
          *(undefined4 *)(puVar7 + 0x2c) = *(undefined4 *)(iVar14 + -4);
        }
        if (puVar10 == *(uint **)(puVar7 + 0x30)) {
          *(undefined4 *)(puVar7 + 0x30) = *(undefined4 *)(iVar14 + -8);
        }
        *(undefined4 *)(puVar7 + uVar4 * 4 + 0x34) = 0;
        *(uint *)(puVar7 + 0x10) = -(uint)(*(int *)(puVar7 + 0x2c) != 0) & uVar4;
        *(uint *)(puVar7 + 0x14) = *(int *)(puVar7 + 0x14) - (uVar2 & 0x7fffffff);
        *(int *)(puVar7 + 0x1c) = *(int *)(puVar7 + 0x1c) + -1;
        puVar6 = PTR_PTR_006926c4;
        iVar14 = DAT_00718f98;
        PTR_PTR_006926c4 = puVar7;
      }
LAB_0049a81b:
      local_49 = '\x01';
    }
    cVar8 = local_45;
    if (local_48 != '\0') goto LAB_0049a89e;
  }
  if (((*param_4 & 0x800) != 0) && (iVar14 = 0, 0 < *(int *)(param_2 + 0x2d4))) {
    iVar13 = 0;
    do {
      iVar11 = *(int *)(param_2 + 0x2d8) + iVar13;
      if (((cVar8 == '\x01') && ((*(byte *)(iVar11 + 0x30) & 1) != 0)) &&
         ((local_49 == '\0' &&
          ((iVar11 = *(int *)(iVar11 + 0xc), iVar11 != -1 &&
           (iVar11 = FUN_0049c4c0(param_1,iVar11), iVar11 != 0)))))) {
        local_49 = '\x01';
      }
      iVar14 = iVar14 + 1;
      iVar13 = iVar13 + 0x50;
    } while (iVar14 < *(int *)(param_2 + 0x2d4));
  }
LAB_0049a89e:
  widget_play_sound_effect();
  *param_5 = local_49;
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
