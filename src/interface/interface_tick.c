// interface_tick  (Ghidra: interface_tick, already named)
// address 0x497e80, size 1198 bytes (0x497e80..0x49832d; 736 bytes stated by
// out/phase4/interface_functions.md, but its real
// extent runs through 0x49832d: the address range 0x498160..0x49832d, separately catalogued as
// "multiplayer_map_list_dispose" (callers=0), shares this function's own stack frame byte for
// byte -- its first instructions (0x498160 `mov ebx,ecx; call 0x49bd00`) are the exact
// continuation of the chimera__load_ui_widget/widget_instance_select_list_index call sequence
// this function builds starting at 0x49813b, its "LAB_0049817c" merge point (buffer clear +
// widget hit-test dispatch) is reached by a `jmp` from within THIS function's own body
// (0x497f68, 0x498021/8034/803e -> 0x498170 == that same address), and its epilogue
// (0x49830e..0x498315 / 0x498326..0x49832d) is this function's own (push ebp;mov ebp,esp;and
// esp,-8;sub esp,0x28;push ebx;push ebp;push esi;push edi at 0x497e80, popped in the matching
// order). It owns no callers and no types of its own; its logic is folded in below rather than
// written as a separate file. See PLAN.md / this file's own summary note for the full case.
// VERIFIED against disassembly 0x497e80..0x49832e (2026-09-30)
// name confidence: 0.8   rewrite confidence: 0.9
// evidence: out/phase4/interface_functions.md; types/interface.h's progress-screen and
// widget_instance notes; register/stack layout cross-checked against disassembly at
// 0x497e80..0x49832d for every call this file could not otherwise resolve (chimera__load_ui_
// widget's real 7-dword call, list_node_pop's ECX/EDX convention, widget_instance_select_list_
// index's EAX/EBX/stack convention).
// register convention: no register-passed arguments.
// The background loading-thread record pointed to by DAT_00718fbc is loading_thread_record in
// types/interface.h (handle at +0x00, a status byte at +0x04).
// TYPES-GAP: the per-frame input-event scratch buffer input_queue_pop_event fills and
// widget_instance_handle_input_event consumes (Ghidra's `LARGE_INTEGER local_28[2]`, 16 bytes)
// is not a type this session resolved; passed through as an opaque byte buffer, exactly as
// large as Ghidra's own frame layout reserves for it.
// Note: input_queue_pop_event's role is modeled only from this call site: `input_queue_pop_event(scratch, controller_index)` returning a bool, matching
// the phase-4 summary "advances timers ... updates the active widget's input/selection state".
// UNSURE: DAT_00718fac/ae/b0/b1 is the same "pending non-modal message" record
// interface_handle_quit_request.c names quit_confirm_error_*; despite that name it is used here
// for a broader "pending UI message" purpose (armed by more than just the quit prompt), gated by
// a ~30-frame idle counter read from *console_state_006f1d6c + 0xc.
// UNSURE: console_state_006f1d6c is used here as a POINTER (`mov edx,[0x6f1d6c]; cmp [edx+0xc],
// 0x1e`), not the 3-byte array src/interface/widget_close.c declared at the same address; the
// two readings may not be reconcilable, and this file follows what its own disassembly shows.

// Phase-4 review: the history pop now reopens with the node controller word at +0x0a and restores
// the selection with list_definition in EAX; the pending message record is 0x00718fac (all four
// fields forwarded), not 0x00718fa4 (objdump 0x497f6d..0x49816a).

#include "win32.h"
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"
#include "cache.h"
#include "objects.h"
#include "units.h"

extern int32_t ui_time_milliseconds;              // 0x00718f9c
extern loading_thread_record *loading_thread;      // 0x00718fbc
extern int16_t loading_thread_result;               // 0x00718fc0 (read with movsx word), 1/2 select which error string
extern uint8_t ui_input_batch_mode;                 // 0x00718fc5, UNSURE name
extern uint8_t virtual_keyboard;             // 0x007193a8 (virtual_keyboard_globals::active)
extern widget_instance *ui_root_widget[1];          // 0x00718f94
extern widget_history_node *ui_widget_history[3];   // 0x00718f98
extern ui_pending_error ui_pending_error_alternate;  // 0x00718fb2
extern uint8_t ui_split_screen;                      // 0x00718fc9
extern game_time_globals *game_time; // 0x006f1d6c (types/game.h)
extern uint8_t ui_cursor_changed;                    // 0x00718f82
extern uint8_t ui_widget_opened;                     // 0x00718fc8
extern int32_t ui_cursor_x;                          // 0x00718f84
extern int32_t ui_cursor_y;                          // 0x00718f88
extern uint8_t controls_input_capture_flags; // 0x00712542, UNSURE (per src/interface/widget_close_all.c)
extern int32_t unknown_00712ccc[0x40];          // 0x00712ccc, UNSURE: 0x100 byte scratch region zeroed every tick
extern tag_instance *tag_instances;                  // 0x0087bc14
extern int64_t performance_frequency;                // 0x006ac8f8/0x006ac8fc

// UNSURE: see file header.
extern int16_t quit_confirm_error_string_index; // 0x00718fac
extern int16_t quit_confirm_error_unknown_ae;   // 0x00718fae
extern uint8_t quit_confirm_error_modal;        // 0x00718fb0
extern uint8_t quit_confirm_error_is_error;     // 0x00718fb1

extern void display_error(int16_t error_string_index, int32_t unknown, uint8_t modal, uint8_t is_error); // 0x498f20
extern uint8_t ui_check_for_pause_game(void); // 0x49c1a0
extern void virtual_keyboard_process_input(void); // 0x4a8be0
extern uint8_t network_game_is_active(void); // 0x4ddca0
extern uint8_t input_queue_pop_event(uint8_t *event_scratch, int16_t controller_index); // 0x4922b0, TYPES-GAP/UNSURE
extern void widget_instance_handle_input_event(widget_instance *widget, UIWidgetDefinition *tag,
                                                uint8_t *event_scratch, uint8_t *out_handled); // 0x499d00
extern void list_node_pop(widget_history_node *out, widget_history_node **head); // 0x499460
extern widget_instance *chimera__load_ui_widget(char *tag_path, datum_index tag_index,
    widget_instance *parent, uint16_t controller_index, datum_index history_definition,
    datum_index history_list_definition, int16_t history_selection); // 0x497a70, 7 stack args (objdump)
extern void widget_instance_select_list_index(widget_instance *widget, datum_index list_definition /*EAX*/,
                                               int32_t selection /*stack*/); // 0x49bd00, blam-cc: EAX -> list_definition (child found by 0x499950), EBX -> widget; only the low word of selection is read
extern widget_instance *widget_instance_find_at_point(widget_instance *root, int32_t x, int32_t y,
                                                        int32_t initial_hint); // 0x499ad0
extern uint8_t widget_instance_verify_stack_chain(widget_instance *node); // 0x499aa0
extern void widget_play_sound_effect(int16_t effect_id); // 0x498e90
extern void widget_list_scroll_window(int32_t out[3], widget_instance *widget); // 0x4a7400, blam-cc: EAX out, ECX widget
extern int32_t widget_get_sibling_index(widget_instance *widget); // 0x498e30, blam-cc: ESI -> widget

// Main per-frame update for the interface/menu system: refreshes ui_time_milliseconds from the
// performance counter; if a background loading thread is still tracked, polls it and, once it
// has exited, tears it down and surfaces its queued error (if any) -- either way skipping the
// rest of this tick's widget-input handling for that case; otherwise, while the virtual keyboard
// is closed and no dialog is pending, drives the active root widget's per-frame input event(s)
// (or restores the previous widget from history if closing the current one left none); if the
// virtual keyboard is open, just pumps its input. Every path funnels into the shared tail: clear
// the 0x100-byte scratch region, and, if the cursor moved or a widget was just opened, retest and
// commit the widget currently under the cursor's selection/focus state. Finally updates the
// "interface busy" flag (bit 2 of controls_input_capture_flags) from whether a root widget exists.
void interface_tick(void)
{
    large_integer counter;
    widget_instance *root; // Ghidra's puVar12, tracked through every path
    uint8_t handled = 0;   // Ghidra's local_29
    // Ghidra's local_28: one shared 16-byte stack slot used, at different points in this
    // function, as the QueryPerformanceCounter buffer, the input_queue_pop_event event scratch, and (in
    // the tail below) a stale-bytes fallback table indexed by a clamped sibling index. See the
    // UNSURE note in the file header -- the last use is preserved literally rather than resolved.
    uint8_t event_scratch[16] = {0};

    QueryPerformanceCounter((LARGE_INTEGER *)&counter);
    ui_time_milliseconds = (int32_t)((counter.quad_part * 1000) / performance_frequency);

    if (loading_thread != (loading_thread_record *)0) {
        uint32_t exit_code;
        int32_t got_exit_code = GetExitCodeThread(loading_thread->handle, &exit_code);

        root = ui_root_widget[0];
        if (got_exit_code != 0 && exit_code != 0x103 /* STILL_ACTIVE */) {
            CloseHandle(loading_thread->handle);
            loading_thread->handle = (void *)0;
            loading_thread->unknown_04 = 0;
            loading_thread = (loading_thread_record *)0;
            ui_input_batch_mode = 0;
            if (loading_thread_result == 1) {
                display_error(0x21, -1, 1, 0);
                root = ui_root_widget[0];
            } else if (loading_thread_result == 2) {
                display_error(0x22, -1, 1, 0);
                root = ui_root_widget[0];
            }
        }
        goto after_widget_pass;
    }

    if (virtual_keyboard == 0) {
        if (ui_pending_error_alternate.error_string_index == -1) {
            if (quit_confirm_error_string_index == -1) { // objdump 0x497f96: 0x00718fac, not 0x00718fa4
                uint8_t is_paused = ui_check_for_pause_game();
                widget_instance *widget = ui_root_widget[0];

                // Ghidra tests `widget != 0 && widget->unknown_15 == 1` (bVar4), then re-tests
                // it or ui_split_screen in three mutually exclusive branches that all reduce,
                // algebraically, to the same single condition: widget != NULL. See the header
                // comment's control-flow note (verified equivalent against 0x497ffe..0x49804e).
                root = widget;
                if (widget != (widget_instance *)0) {
                    UIWidgetDefinition *tag = (UIWidgetDefinition *)tag_instances[widget->definition & 0xffff].data;
                    int32_t scratch_i;
                    uint8_t looped = 0;

                    for (scratch_i = 0; scratch_i < 16; scratch_i++) event_scratch[scratch_i] = 0;
                    root = widget;
                    // 0x498074..0x498112: in the non-batch mode the event queue is drained first; when it holds NO event (or in batch
                    // mode) the handler is still called once with an empty event whose controller word (+2) is the widget's controller.
                    // FIXED 2026-09-30: the draft only called the handler when an event was actually popped, so idle frames never
                    // reached the widget.
                    if (ui_input_batch_mode == 0) {
                        uint8_t got_event = input_queue_pop_event(event_scratch, widget->controller_index);

                        if (got_event != 0) {
                            looped = 1;
                            do {
                                if ((is_paused == 0 &&
                                     (widget_instance_handle_input_event(widget, tag, event_scratch, &handled),
                                      root = ui_root_widget[0], handled == 1)) ||
                                    widget != root) {
                                    break;
                                }
                                got_event = input_queue_pop_event(event_scratch, widget->controller_index);
                            } while (got_event != 0);
                        }
                    }
                    if (looped == 0 && is_paused == 0) {
                        *(uint16_t *)(event_scratch + 2) = widget->controller_index;
                        widget_instance_handle_input_event(widget, tag, event_scratch, &handled);
                        root = ui_root_widget[0];
                    }
                    handled = 1;
                    if (root == (widget_instance *)0 && ui_widget_history[0] != (widget_history_node *)0) {
                        widget_history_node popped;

                        list_node_pop(&popped, &ui_widget_history[0]);
                        root = ui_root_widget[0];
                        if (popped.definition != (datum_index)-1) {
                            widget_instance *reopened = chimera__load_ui_widget(
                                (char *)0, popped.definition, (widget_instance *)0,
                                (uint16_t)popped.controller_index, (datum_index)-1, (datum_index)-1, -1);

                            root = ui_root_widget[0];
                            if (reopened != (widget_instance *)0) {
                                // objdump 0x498157: EAX = node+0x04, stack = dword at node+0x08
                                widget_instance_select_list_index(reopened, popped.list_definition,
                                                                   popped.selection);
                                root = ui_root_widget[0];
                            }
                        }
                    }
                }
                if (handled != 0) {
                    goto shared_tail;
                }
            } else {
                // objdump 0x497f96..0x497fe7: the pending message record at 0x00718fac, all four
                // fields forwarded; the first rewrite read 0x00718fa4 and passed constants
                int16_t error_string_index = quit_confirm_error_string_index;

                if (ui_split_screen != 0 || network_game_is_active() != 0 ||
                    game_time->game_time > 0x1d) {
                    display_error(error_string_index, (int32_t)(uint16_t)quit_confirm_error_unknown_ae,
                                  quit_confirm_error_modal, quit_confirm_error_is_error);
                    quit_confirm_error_string_index = -1;
                    root = ui_root_widget[0];
                }
            }
        } else {
            display_error(ui_pending_error_alternate.error_string_index, -1, 1, 0);
            ui_pending_error_alternate.error_string_index = -1;
            root = ui_root_widget[0];
        }
    } else {
        virtual_keyboard_process_input();
        root = ui_root_widget[0];
        goto shared_tail;
    }
    goto after_widget_pass;

shared_tail:
    {
        int32_t i;

        for (i = 0; i < 0x40; i++) {
            unknown_00712ccc[i] = 0;
        }
        if (ui_cursor_changed != 0 || ui_widget_opened != 0) {
            ui_widget_opened = 0;
            if (root == (widget_instance *)0) {
                goto after_widget_pass;
            }
            {
                // 4th arg is a packed {local_x, local_y} dword read from root itself (offset
                // 0x0a), the same pair widget_instance_point_in_bounds sums up the ancestor
                // chain -- the seed offset for the recursive hit test, not widget_type.
                widget_instance *hit = widget_instance_find_at_point(
                    root, ui_cursor_x, ui_cursor_y, *(int32_t *)&((struct widget_instance *)root)->local_x);

                root = ui_root_widget[0];
                if (hit != (widget_instance *)0 && hit->parent != (widget_instance *)0 &&
                    widget_instance_verify_stack_chain(hit) == 0) {
                    widget_instance *parent = hit->parent;
                    UIWidgetDefinition *parent_tag =
                        (UIWidgetDefinition *)tag_instances[parent->definition & 0xffff].data;

                    if (parent->focused_child != hit) {
                        widget_play_sound_effect(1);
                    }
                    if (parent->widget_type == 2) { // spinner_list
                        if (parent_tag->child_widgets.count > 1) {
                            widget_list_scroll_window((int32_t *)event_scratch, parent); // EAX esp+0x18, ECX parent
                            {
                                // UNSURE: Ghidra calls widget_get_sibling_index (pure, no side
                                // effects -- see widget_get_sibling_index.c) three times in a row
                                // with the same implicit ESI, which necessarily returns the same
                                // value each time; collapsed to one call and an equivalent clamp.
                                int32_t sibling = widget_get_sibling_index(hit); // ESI is the hit widget (0x498248)
                                int32_t idx = (sibling < 0) ? 0 : ((sibling < 4) ? sibling : 3);

                                parent->selection_index = *(int16_t *)(event_scratch + idx * 4);
                            }
                        }
                        {
                            widget_instance *cursor = hit;

                            while (cursor->parent != (widget_instance *)0) {
                                cursor->parent->focused_child = cursor;
                                cursor = cursor->parent;
                            }
                        }
                    } else if (parent->widget_type == 3) { // column_list
                        parent->selection_index = (int16_t)widget_get_sibling_index(hit);
                        parent->focused_child = hit;
                        {
                            widget_instance *cursor = parent;

                            while (cursor->parent != (widget_instance *)0) {
                                widget_instance *up = cursor->parent;

                                up->focused_child = cursor;
                                if (up->widget_type == 3) {
                                    up->selection_index = (int16_t)widget_get_sibling_index(cursor);
                                }
                                cursor = up;
                            }
                        }
                    } else {
                        widget_instance *cursor = hit;

                        while (cursor->parent != (widget_instance *)0) {
                            cursor->parent->focused_child = cursor;
                            cursor = cursor->parent;
                        }
                    }
                }
            }
        }
    }

after_widget_pass:
    if (root != (widget_instance *)0) {
        if ((controls_input_capture_flags & 2) != 0) {
            return;
        }
        controls_input_capture_flags = controls_input_capture_flags | 2;
        return;
    }
    if ((controls_input_capture_flags & 2) != 0) {
        controls_input_capture_flags = controls_input_capture_flags & 0xfd;
    }
}

#if 0
Original Ghidra decompilation, first half (0x497e80):

void interface_tick(void)

{
  uint *puVar1;
  undefined4 uVar2;
  int iVar3;
  bool bVar4;
  uint *puVar5;
  char cVar6;
  char cVar7;
  undefined2 uVar8;
  BOOL BVar9;
  uint uVar10;
  short extraout_CX;
  short error_string_index;
  int iVar11;
  uint *puVar12;
  undefined4 *puVar13;
  undefined8 uVar14;
  char local_29;
  LARGE_INTEGER local_28 [2];
  int local_18;
  undefined2 uStack_10;
  undefined2 local_e;
  undefined2 uStack_c;

  local_29 = '\0';
  QueryPerformanceCounter(local_28);
  uVar14 = __allmul(CONCAT22(local_28[0].s.LowPart._2_2_,(undefined2)local_28[0].s.LowPart),
                    CONCAT22(local_28[0].s.HighPart._2_2_,(undefined2)local_28[0].s.HighPart),1000,0
                   );
  DAT_00718f9c = __alldiv(uVar14,DAT_006ac8f8,DAT_006ac8fc);
  if (DAT_00718fbc != (undefined4 *)0x0) {
    BVar9 = GetExitCodeThread((HANDLE)*DAT_00718fbc,(LPDWORD)local_28);
    puVar13 = DAT_00718fbc;
    puVar12 = DAT_00718f94;
    if ((BVar9 != 0) &&
       (CONCAT22(local_28[0].s.LowPart._2_2_,(undefined2)local_28[0].s.LowPart) != 0x103)) {
      CloseHandle((HANDLE)*DAT_00718fbc);
      *puVar13 = 0;
      *(undefined1 *)(puVar13 + 1) = 0;
      DAT_00718fbc = (undefined4 *)0x0;
      DAT_00718fc5 = '\0';
      if (DAT_00718fc0 == 1) {
        display_error(0x21,-1,'\x01','\0');
        puVar12 = DAT_00718f94;
      }
      else {
        puVar12 = DAT_00718f94;
        if (DAT_00718fc0 == 2) {
          display_error(0x22,-1,'\x01','\0');
          puVar12 = DAT_00718f94;
        }
      }
    }
    goto LAB_004982f6;
  }
  if (DAT_007193a8 == '\0') {
    if (DAT_00718fb2 == -1) {
      if (DAT_00718fac == -1) {
        cVar7 = ui_check_for_pause_game();
        puVar5 = DAT_00718f94;
        if ((DAT_00718f94 == (uint *)0x0) || (*(char *)((int)DAT_00718f94 + 0x15) != '\x01')) {
          bVar4 = false;
        }
        else {
          bVar4 = true;
        }
        puVar12 = DAT_00718f94;
        if (bVar4) {
          if ((DAT_00718f94 != (uint *)0x0) && (*(char *)((int)DAT_00718f94 + 0x15) == '\x01'))
          goto LAB_00498054;
        }
        else if (DAT_00718fc9 == '\0') {
          if (DAT_00718f94 != (uint *)0x0) goto LAB_00498054;
        }
        else if ((DAT_00718f94 != (uint *)0x0) && (!bVar4)) {
LAB_00498054:
          uVar2 = *(undefined4 *)((*DAT_00718f94 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
          local_28[0].s.LowPart._2_2_ = 0;
          local_28[0].s.HighPart._0_2_ = 0;
          local_28[0].s.HighPart._2_2_ = 0;
          local_28[0].s.LowPart._0_2_ = 0;
          puVar12 = puVar5;
          if (DAT_00718fc5 == '\0') {
            puVar1 = DAT_00718f94 + 2;
            cVar6 = FUN_004922b0(local_28,(short)DAT_00718f94[2]);
            if (cVar6 == '\0') goto LAB_004980e7;
            do {
              if (((cVar7 == '\0') &&
                  (widget_instance_handle_input_event(puVar5,uVar2,local_28,&local_29),
                  puVar12 = DAT_00718f94, local_29 == '\x01')) || (puVar5 != puVar12)) break;
              cVar6 = FUN_004922b0(local_28,(short)*puVar1);
            } while (cVar6 != '\0');
          }
          else {
LAB_004980e7:
            if (cVar7 == '\0') {
              local_28[0]._2_2_ = (undefined2)puVar5[2];
              widget_instance_handle_input_event(puVar5,uVar2,local_28,&local_29);
              puVar12 = DAT_00718f94;
            }
          }
          local_29 = '\x01';
          if (((puVar12 == (uint *)0x0) && (DAT_00718f98 != 0)) &&
             ((FUN_00499460(), puVar12 = DAT_00718f94, local_18 != -1 &&
              (iVar11 = chimera__load_ui_widget
                                  (0,local_18,0,CONCAT22(uStack_c,local_e),0xffffffff,0xffffffff,
                                   0xffffffff), puVar12 = DAT_00718f94, iVar11 != 0)))) {
            FUN_0049bd00(CONCAT22(local_e,uStack_10));
            puVar12 = DAT_00718f94;
          }
        }
        if (local_29 != '\0') goto LAB_0049817c;
      }
      else {
        error_string_index = DAT_00718fac;
        if (((DAT_00718fc9 != '\0') ||
            (iVar11 = network_game_is_active(), error_string_index = extraout_CX,
            (char)iVar11 != '\0')) || (puVar12 = DAT_00718f94, 0x1d < *(int *)(DAT_006f1d6c + 0xc)))
        {
          display_error(error_string_index,(uint)DAT_00718fae,DAT_00718fb0,DAT_00718fb1);
          DAT_00718fac = -1;
          puVar12 = DAT_00718f94;
        }
      }
    }
    else {
      display_error(DAT_00718fb2,-1,'\x01','\0');
      DAT_00718fb2 = -1;
      puVar12 = DAT_00718f94;
    }
  }
  else {
    virtual_keyboard_process_input();
    puVar12 = DAT_00718f94;
LAB_0049817c:
    puVar13 = &DAT_00712ccc;
    for (iVar11 = 0x40; iVar11 != 0; iVar11 = iVar11 + -1) {
      *puVar13 = 0;
      puVar13 = puVar13 + 1;
    }
    if ((DAT_00718f82 != '\0') || (DAT_00718fc8 != '\0')) {
      DAT_00718fc8 = '\0';
      if (puVar12 == (uint *)0x0) goto LAB_00498316;
      local_28[0]._0_2_ = *(undefined2 *)((int)puVar12 + 10);
      local_28[0]._2_2_ = (undefined2)puVar12[3];
      uVar10 = widget_instance_find_at_point
                         (puVar12,DAT_00718f84,DAT_00718f88,*(undefined4 *)((int)puVar12 + 10));
      puVar12 = DAT_00718f94;
      if (((uVar10 != 0) && (*(int *)(uVar10 + 0x30) != 0)) &&
         (cVar7 = FUN_00499aa0(), puVar12 = DAT_00718f94, cVar7 == '\0')) {
        iVar11 = *(int *)((**(uint **)(uVar10 + 0x30) & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
        if ((*(uint **)(uVar10 + 0x30))[0xe] != uVar10) {
          widget_play_sound_effect();
        }
        iVar3 = *(int *)(uVar10 + 0x30);
        if (*(short *)(iVar3 + 0xe) == 2) {
          if (1 < *(int *)(iVar11 + 0x3e0)) {
            FUN_004a7400();
            iVar11 = widget_get_sibling_index();
            if (iVar11 < 0) {
              iVar11 = 0;
            }
            else {
              iVar11 = widget_get_sibling_index();
              if (iVar11 < 4) {
                iVar11 = widget_get_sibling_index();
              }
              else {
                iVar11 = 3;
              }
            }
            *(undefined2 *)(iVar3 + 0x40) = *(undefined2 *)((int)local_28 + iVar11 * 4);
          }
          iVar11 = *(int *)(uVar10 + 0x30);
          while (puVar12 = DAT_00718f94, iVar11 != 0) {
            *(uint *)(*(int *)(uVar10 + 0x30) + 0x38) = uVar10;
            uVar10 = *(uint *)(uVar10 + 0x30);
            iVar11 = *(int *)(uVar10 + 0x30);
          }
        }
        else if (*(short *)(iVar3 + 0xe) == 3) {
          uVar8 = widget_get_sibling_index();
          *(undefined2 *)(*(int *)(uVar10 + 0x30) + 0x40) = uVar8;
          *(uint *)(*(int *)(uVar10 + 0x30) + 0x38) = uVar10;
          iVar11 = *(int *)(uVar10 + 0x30);
          iVar3 = *(int *)(iVar11 + 0x30);
          while (puVar12 = DAT_00718f94, iVar3 != 0) {
            *(int *)(*(int *)(iVar11 + 0x30) + 0x38) = iVar11;
            if (*(short *)(*(int *)(iVar11 + 0x30) + 0xe) == 3) {
              uVar8 = widget_get_sibling_index();
              *(undefined2 *)(*(int *)(iVar11 + 0x30) + 0x40) = uVar8;
            }
            iVar11 = *(int *)(iVar11 + 0x30);
            iVar3 = *(int *)(iVar11 + 0x30);
          }
        }
        else {
          while (puVar12 = DAT_00718f94, iVar3 != 0) {
            *(uint *)(*(int *)(uVar10 + 0x30) + 0x38) = uVar10;
            uVar10 = *(uint *)(uVar10 + 0x30);
            iVar3 = *(int *)(uVar10 + 0x30);
          }
        }
      }
    }
  }
LAB_004982f6:
  if (puVar12 != (uint *)0x0) {
    if ((DAT_00712542 & 2) != 0) {
      return;
    }
    DAT_00712542 = DAT_00712542 | 2;
    return;
  }
LAB_00498316:
  if ((DAT_00712542 & 2) != 0) {
    DAT_00712542 = DAT_00712542 & 0xfd;
  }
  return;
}

Second half, catalogued separately as "multiplayer_map_list_dispose" (0x498160) but proven by its
shared prologue/epilogue and by 0x497f68/0x498021/0x498034/0x49803e jumping into its
0x498170 (== "LAB_0049817c" above) to be this same function's tail -- included here for
completeness, folded into the block above rather than kept as a second file:

void multiplayer_map_list_dispose(void)

{
  int iVar1;
  char cVar2;
  undefined2 uVar3;
  uint uVar4;
  int iVar5;
  int iVar6;
  undefined4 *puVar7;
  undefined4 in_stack_00000018;
  undefined4 in_stack_0000001c;

  FUN_0049bd00();
  iVar5 = DAT_00718f94;
  if (in_stack_00000018._3_1_ != '\0') {
    puVar7 = &DAT_00712ccc;
    for (iVar6 = 0x40; iVar6 != 0; iVar6 = iVar6 + -1) {
      *puVar7 = 0;
      puVar7 = puVar7 + 1;
    }
    if ((DAT_00718f82 != '\0') || (DAT_00718fc8 != '\0')) {
      DAT_00718fc8 = '\0';
      if (iVar5 == 0) goto LAB_00498316;
      in_stack_0000001c = *(undefined4 *)(iVar5 + 10);
      uVar4 = widget_instance_find_at_point(iVar5,DAT_00718f84,DAT_00718f88);
      iVar5 = DAT_00718f94;
      if (((uVar4 != 0) && (*(int *)(uVar4 + 0x30) != 0)) &&
         (cVar2 = FUN_00499aa0(), iVar5 = DAT_00718f94, cVar2 == '\0')) {
        iVar5 = *(int *)((**(uint **)(uVar4 + 0x30) & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
        if ((*(uint **)(uVar4 + 0x30))[0xe] != uVar4) {
          widget_play_sound_effect();
        }
        iVar6 = *(int *)(uVar4 + 0x30);
        if (*(short *)(iVar6 + 0xe) == 2) {
          if (1 < *(int *)(iVar5 + 0x3e0)) {
            FUN_004a7400();
            iVar5 = widget_get_sibling_index();
            if (iVar5 < 0) {
              iVar5 = 0;
            }
            else {
              iVar5 = widget_get_sibling_index();
              if (iVar5 < 4) {
                iVar5 = widget_get_sibling_index();
              }
              else {
                iVar5 = 3;
              }
            }
            *(undefined2 *)(iVar6 + 0x40) = *(undefined2 *)(&stack0x0000001c + iVar5);
          }
          iVar6 = *(int *)(uVar4 + 0x30);
          iVar5 = DAT_00718f94;
          while (DAT_00718f94 = iVar5, iVar6 != 0) {
            *(uint *)(*(int *)(uVar4 + 0x30) + 0x38) = uVar4;
            uVar4 = *(uint *)(uVar4 + 0x30);
            iVar5 = DAT_00718f94;
            iVar6 = *(int *)(uVar4 + 0x30);
          }
        }
        else {
          iVar5 = DAT_00718f94;
          if (*(short *)(iVar6 + 0xe) == 3) {
            uVar3 = widget_get_sibling_index();
            *(undefined2 *)(*(int *)(uVar4 + 0x30) + 0x40) = uVar3;
            *(uint *)(*(int *)(uVar4 + 0x30) + 0x38) = uVar4;
            iVar6 = *(int *)(uVar4 + 0x30);
            iVar1 = *(int *)(iVar6 + 0x30);
            iVar5 = DAT_00718f94;
            while (DAT_00718f94 = iVar5, iVar1 != 0) {
              *(int *)(*(int *)(iVar6 + 0x30) + 0x38) = iVar6;
              if (*(short *)(*(int *)(iVar6 + 0x30) + 0xe) == 3) {
                uVar3 = widget_get_sibling_index();
                *(undefined2 *)(*(int *)(iVar6 + 0x30) + 0x40) = uVar3;
              }
              iVar6 = *(int *)(iVar6 + 0x30);
              iVar5 = DAT_00718f94;
              iVar1 = *(int *)(iVar6 + 0x30);
            }
          }
          else {
            while (DAT_00718f94 = iVar5, iVar6 != 0) {
              *(uint *)(*(int *)(uVar4 + 0x30) + 0x38) = uVar4;
              uVar4 = *(uint *)(uVar4 + 0x30);
              iVar5 = DAT_00718f94;
              iVar6 = *(int *)(uVar4 + 0x30);
            }
          }
        }
      }
    }
  }
  if (iVar5 != 0) {
    if ((DAT_00712542 & 2) != 0) {
      return;
    }
    DAT_00712542 = DAT_00712542 | 2;
    return;
  }
LAB_00498316:
  if ((DAT_00712542 & 2) != 0) {
    DAT_00712542 = DAT_00712542 & 0xfd;
  }
  return;
}
#endif
