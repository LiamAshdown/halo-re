// widget_close  (Ghidra: widget_close, already named)
// address 0x497c00, size 627 bytes
// name confidence: 0.6   rewrite confidence: 0.45
// evidence: types/interface.h's widget_instance note attributes this address as the closer for
// nearly every field (closing, previous_sibling, next_sibling, parent, first_child, text,
// list_render_data, extended_description); the event-handler loop offsets (tag_data+0x54/+0x58)
// match types/tags.h's UIWidgetDefinition::event_handlers TagReflexive exactly, and the loop's
// event_type check (0x19) is UIEventType_t's uieventtype_deleted -- this is the "on deleted"
// event-handler dispatch. The manual heap-block unlink sequences (puVar6[-4..-1], matching
// heap_block's size/slot/previous/next) are widget_close freeing the widget's own block itself,
// via the same field-by-field logic heap_unlink_block performs elsewhere (called for
// text/list_render_data instead of inlined).
// register convention: widget in the recognized stack parameter (Ghidra's own param_1).
// UNSURE: DAT_006b145c is a 0x40-stride per-controller table (header only names its stride, not
// its fields); the two ushort fields OR'd with 0xfff at +8/+10 are not otherwise documented.
// UNSURE: DAT_00719720, DAT_006f1d6c[0..2] are console/chat-adjacent state this module does not
// otherwise name.
// reconciled: R33 game_time_globals.unknown_00 -> initialized (uint8 at +0x00, same byte)

// Phase-4 review: 0x006b145c and 0x006f1d6c are pointers (objdump 0x497c26, 0x497cfa); the event
// functions return a byte handled flag through a byte out parameter.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"
#include "cache.h"

extern tag_instance *tag_instances; // 0x0087bc14
extern heap *widget_memory_pool;    // 0x006926c4
extern widget_instance *ui_root_widget[1]; // 0x00718f94, widget_close only ever checks index 0
extern void *ui_event_function_table[0xbe]; // 0x006927d0
extern int16_t ui_pause_depth; // 0x00718fa6
extern uint8_t ui_split_screen; // 0x00718fc9
extern int16_t network_game_mode; // 0x00719720, types/game.h: 0 local, 1 client, 2 host (word access)
extern game_time_globals *game_time; // 0x006f1d6c (types/game.h)
extern player_control_globals *player_control_globals_ptr; // 0x006b145c, types/game.h; a pointer (objdump 0x497c26)

extern void widget_close(widget_instance *widget);
extern widget_instance *widget_reopen_as_root_with_history(widget_instance *widget, datum_index open_tag); // 0x49c4c0, UNSURE signature
extern void heap_unlink_block(heap_block *block, heap *self); // 0x4d20a0, blam-cc: EAX -> block, ECX -> self, UNSURE not independently confirmed here

// Closes a widget instance: fires its tag's "deleted" event handlers, unlinks it from the
// widget tree (recursively closing every child first), frees its type-specific extra allocation
// (text for a text_box, list_render_data plus a recursively-closed extended_description for a
// list), then unlinks its own heap block and clears whichever root-widget slot pointed at it.
void widget_close(widget_instance *widget)
{
    uint8_t *self = (uint8_t *)widget;

    if (widget->closing != 0) {
        return;
    }
    widget->closing = 1;

    if (widget->controller_index != -1 && widget->parent == (widget_instance *)0) {
        uint8_t *entry = (uint8_t *)player_control_globals_ptr + (int16_t)widget->controller_index * 0x40 + 0x10;

        *(uint16_t *)(entry + 8) |= 0xfff;
        *(uint16_t *)(entry + 10) |= 0xfff;
    }

    {
        void *tag_data = tag_instances[widget->definition & 0xffff].data;
        TagReflexive *event_handlers = (TagReflexive *)((uint8_t *)tag_data + 0x54);
        int32_t i;

        for (i = 0; i < (int32_t)event_handlers->count; i++) {
            EventHandlerReference *handler =
                (EventHandlerReference *)((uint8_t *)event_handlers->pointer + i * sizeof(EventHandlerReference));

            if (handler->event_type == uieventtype_deleted && (int8_t)handler->flags < 0) {
                uint16_t function = (uint16_t)handler->function;

                if ((int16_t)function >= 0 && function < 0xbe) {
                    ui_event_function fn = (ui_event_function)ui_event_function_table[function];
                    uint8_t handled = 0; // objdump 0x497c88: a byte at esp+0x18 cleared before the call

                    if (fn(widget, (int16_t *)0, &handled) == 1 && (handler->flags & 0x08) != 0 &&
                        *(uint32_t *)&handler->widget_tag.tag_id != 0xffffffff) {
                        widget_reopen_as_root_with_history(widget, *(datum_index *)&handler->widget_tag.tag_id);
                    }
                }
            }
        }
    }

    if (widget->pauses_game_time == 1 && network_game_mode != 2 &&
        ui_split_screen == 0) {
        ui_pause_depth = ui_pause_depth - 1;
        if (ui_pause_depth == 0 && game_time->paused != 0) {
            if (game_time->initialized != 0) {
                game_time->active = 1;
            }
            game_time->paused = 0;
        }
    }

    {
        widget_instance *child = widget->first_child;

        while (child != (widget_instance *)0) {
            widget_instance *next = child->next_sibling;

            widget_close(child);
            if (next == (widget_instance *)0) {
                break;
            }
            next->previous_sibling = (widget_instance *)0;
            child = next;
        }
    }

    if (widget->previous_sibling != (widget_instance *)0) {
        widget->previous_sibling->next_sibling = widget->next_sibling;
    }
    if (widget->next_sibling != (widget_instance *)0) {
        widget->next_sibling->previous_sibling = widget->previous_sibling;
    }
    if (widget->parent != (widget_instance *)0 && widget->parent->first_child == widget) {
        widget->parent->first_child = widget->next_sibling;
    }

    {
        heap *pool = widget_memory_pool;

        if (widget->widget_type == 1) { // text_box
            if (widget->text != (void *)0) {
                heap_block *block = (heap_block *)((uint8_t *)widget->text - 0x10);

                heap_unlink_block(block, widget_memory_pool);
                pool = widget_memory_pool; // heap_unlink_block operates on widget_memory_pool
            }
        } else if (widget->widget_type > 1 && widget->widget_type < 4) { // spinner_list, column_list
            if (widget->list_render_data != (void *)0) {
                heap_block *block = (heap_block *)((uint8_t *)widget->list_render_data - 0x10);

                heap_unlink_block(block, widget_memory_pool);
                pool = widget_memory_pool;
            }
            if (widget->extended_description != (widget_instance *)0) {
                widget_close(widget->extended_description);
                pool = widget_memory_pool;
            }
        }

        // Unlink the widget's own heap block (heap_block header immediately precedes it).
        {
            heap_block *self_block = (heap_block *)(self - 0x10);
            uint32_t size = self_block->size;
            int32_t slot = self_block->slot;

            if (self_block->previous != (heap_block *)0) {
                self_block->previous->next = self_block->next;
            }
            if (self_block->next != (heap_block *)0) {
                self_block->next->previous = self_block->previous;
            }
            if (self_block == pool->first_block) {
                pool->first_block = self_block->next;
            }
            if (self_block == pool->last_block) {
                pool->last_block = self_block->previous;
            }
            pool->blocks[slot] = (heap_block *)0;
            pool->next_free_slot = (pool->first_block != (heap_block *)0) ? slot : 0;
            pool->allocation_count = pool->allocation_count - 1;
            pool->bytes_allocated = pool->bytes_allocated - (int32_t)(size & 0x7fffffff);
        }
    }

    if (ui_root_widget[0] == widget) {
        ui_root_widget[0] = (widget_instance *)0;
    }
}

#if 0
Original Ghidra decompilation (0x497c00):

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void widget_close(uint *param_1)

{
  ushort *puVar1;
  ushort uVar2;
  short sVar3;
  uint uVar4;
  char *pcVar5;
  uint *puVar6;
  char cVar7;
  uint uVar8;
  int iVar9;
  undefined *extraout_ECX;
  undefined *puVar10;
  undefined *extraout_ECX_00;
  int iVar11;
  byte *pbVar12;
  int local_4;

  puVar6 = param_1;
  if ((char)param_1[5] == '\0') {
    *(undefined1 *)(param_1 + 5) = 1;
    if (((short)param_1[2] != -1) && (param_1[0xc] == 0)) {
      iVar9 = (short)param_1[2] * 0x40 + 0x10 + DAT_006b145c;
      puVar1 = (ushort *)(iVar9 + 8);
      *puVar1 = *puVar1 | 0xfff;
      puVar1 = (ushort *)(iVar9 + 10);
      *puVar1 = *puVar1 | 0xfff;
    }
    iVar9 = *(int *)((*param_1 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
    iVar11 = 0;
    local_4 = 0;
    if (0 < *(int *)(iVar9 + 0x54)) {
      do {
        pbVar12 = (byte *)(*(int *)(iVar9 + 0x58) + iVar11);
        if ((*(short *)(pbVar12 + 4) == 0x19) && ((char)*pbVar12 < '\0')) {
          uVar2 = *(ushort *)(pbVar12 + 6);
          param_1 = (uint *)((uint)param_1 & 0xffffff00);
          if ((-1 < (short)uVar2) &&
             ((((uVar2 < 0xbe &&
                (cVar7 = (*(code *)(&PTR_LAB_006927d0)[(short)uVar2])(puVar6,0,&param_1),
                cVar7 == '\x01')) && ((*pbVar12 & 8) != 0)) && (*(int *)(pbVar12 + 0x14) != -1)))) {
            FUN_0049c4c0(puVar6,*(int *)(pbVar12 + 0x14));
          }
        }
        local_4 = local_4 + 1;
        iVar11 = iVar11 + 0x48;
      } while (local_4 < *(int *)(iVar9 + 0x54));
    }
    pcVar5 = DAT_006f1d6c;
    if (((*(char *)((int)puVar6 + 0x13) == '\x01') && (DAT_00719720 != 2)) &&
       ((DAT_00718fc9 == '\0' &&
        ((_DAT_00718fa6 = _DAT_00718fa6 + -1, _DAT_00718fa6 == 0 && (DAT_006f1d6c[2] != '\0')))))) {
      if (*DAT_006f1d6c != '\0') {
        DAT_006f1d6c[1] = '\x01';
      }
      pcVar5[2] = '\0';
    }
    uVar8 = puVar6[0xd];
    if (uVar8 != 0) {
      while( true ) {
        uVar4 = *(uint *)(uVar8 + 0x2c);
        widget_close(uVar8);
        if (uVar4 == 0) break;
        *(undefined4 *)(uVar4 + 0x28) = 0;
        uVar8 = uVar4;
      }
    }
    if (puVar6[10] != 0) {
      *(uint *)(puVar6[10] + 0xc) = puVar6[0xb];
    }
    if (puVar6[0xb] != 0) {
      *(uint *)(puVar6[0xb] + 8) = puVar6[10];
    }
    uVar8 = puVar6[0xc];
    if ((uVar8 != 0) && (*(uint **)(uVar8 + 0x34) == puVar6)) {
      *(uint *)(uVar8 + 0x34) = puVar6[0xb];
    }
    sVar3 = *(short *)((int)puVar6 + 0xe);
    puVar10 = PTR_PTR_006926c4;
    if (sVar3 == 1) {
      if (puVar6[0xf] != 0) {
        uVar8 = *(uint *)(puVar6[0xf] - 0x10);
        heap_unlink_block();
        *(uint *)(extraout_ECX_00 + 0x14) = *(int *)(extraout_ECX_00 + 0x14) - (uVar8 & 0x7fffffff);
        *(int *)(extraout_ECX_00 + 0x1c) = *(int *)(extraout_ECX_00 + 0x1c) + -1;
        puVar10 = extraout_ECX_00;
      }
    }
    else if ((1 < sVar3) && (sVar3 < 4)) {
      if (puVar6[0x14] != 0) {
        uVar8 = *(uint *)(puVar6[0x14] - 0x10);
        heap_unlink_block();
        *(uint *)(extraout_ECX + 0x14) = *(int *)(extraout_ECX + 0x14) - (uVar8 & 0x7fffffff);
        *(int *)(extraout_ECX + 0x1c) = *(int *)(extraout_ECX + 0x1c) + -1;
        puVar10 = extraout_ECX;
      }
      if (puVar6[0x13] != 0) {
        widget_close(puVar6[0x13]);
        puVar10 = PTR_PTR_006926c4;
      }
    }
    uVar8 = puVar6[-4];
    uVar4 = puVar6[-3];
    if (puVar6[-2] != 0) {
      *(uint *)(puVar6[-2] + 0xc) = puVar6[-1];
    }
    if (puVar6[-1] != 0) {
      *(uint *)(puVar6[-1] + 8) = puVar6[-2];
    }
    if (puVar6 + -4 == *(uint **)(puVar10 + 0x2c)) {
      *(uint *)(puVar10 + 0x2c) = puVar6[-1];
    }
    if (puVar6 + -4 == *(uint **)(puVar10 + 0x30)) {
      *(uint *)(puVar10 + 0x30) = puVar6[-2];
    }
    *(undefined4 *)(puVar10 + uVar4 * 4 + 0x34) = 0;
    *(uint *)(puVar10 + 0x10) = -(uint)(*(int *)(puVar10 + 0x2c) != 0) & uVar4;
    *(int *)(puVar10 + 0x1c) = *(int *)(puVar10 + 0x1c) + -1;
    *(uint *)(puVar10 + 0x14) = *(int *)(puVar10 + 0x14) - (uVar8 & 0x7fffffff);
    iVar9 = 0;
    while ((uint *)(&DAT_00718f94)[iVar9] != puVar6) {
      iVar9 = iVar9 + 1;
      if (0 < iVar9) {
        return;
      }
    }
    (&DAT_00718f94)[iVar9] = 0;
  }
  return;
}
#endif
