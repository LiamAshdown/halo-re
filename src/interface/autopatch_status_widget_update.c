// autopatch_status_widget_update  (Ghidra: autopatch_status_widget_update, already named)
// address 0x4a4880, size 288 bytes, callers=0 in this build
// name confidence: 0.6   rewrite confidence: 0.2
// evidence: matches the given name; functions.md: "Drives the 'checking for update' UI state
// machine widget by polling the autopatch subsystem and updating its display/timer." Reuses
// widget_instance_close_and_restore_previous (0x49c3e0, this session) and the
// quit_confirm_error_* globals established across five other files this session.
// register convention: cdecl, the one recognized stack parameter.
// UNSURE / TYPES-GAP: `param_1` is written at offsets 0x18/0x1c/0x20, past widget_instance's own
// 0x60-byte size -- it is therefore NOT a plain widget_instance* here but some larger enclosing
// "autopatch status" record that embeds one (first_child at +0x34 still resolves to a real
// widget). Declared as a raw byte pointer rather than asserting an incorrect struct.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern uint8_t autopatch_status_active_00719235; // 0x00719235, TYPES-GAP
extern uint8_t autopatch_status_flag_00692b11;    // 0x00692b11, TYPES-GAP
extern uint8_t autopatch_status_state_00719234;   // 0x00719234, TYPES-GAP
extern int16_t quit_confirm_error_string_index; // 0x00718fac
extern int16_t quit_confirm_error_unknown_ae;   // 0x00718fae
extern uint8_t quit_confirm_error_modal;        // 0x00718fb0
extern uint8_t quit_confirm_error_is_error;     // 0x00718fb1

extern int32_t time_query_performance_counter_ms(void); // 0x449210, current time in milliseconds
extern void widget_instance_close_and_restore_previous(widget_instance *widget); // 0x49c3e0
extern int32_t security_check_write_access(void); // 0x542840, UNSURE args
extern int32_t autopatch_check_for_update_start(void); // 0x577240, UNSURE args

// blam-cc: param_1 is a larger record embedding a widget_instance; see file header.
void autopatch_status_widget_update(uint8_t *record)
{
    widget_instance *control = *(widget_instance **)(record + 0x34);
    widget_instance *row = control->next_sibling->next_sibling->first_child;

    if (autopatch_status_active_00719235 != 0) {
        return;
    }

    switch (autopatch_check_for_update_start()) {
    case 0:
    case 2: {
        uint8_t first_time = (autopatch_status_flag_00692b11 == 0);

        control->selection_index = 0;
        autopatch_status_state_00719234 = 1;
        row->hidden = 1;
        row->scale = 0.333f;
        if (first_time) {
            *(int32_t *)(record + 0x18) = time_query_performance_counter_ms();
            autopatch_status_active_00719235 = 1;
            *(int32_t *)(record + 0x20) = 300;
            *(int32_t *)(record + 0x1c) = 300;
            autopatch_status_flag_00692b11 = 0;
            return;
        }
        *(int32_t *)(record + 0x18) = time_query_performance_counter_ms();
        autopatch_status_active_00719235 = 1;
        *(int32_t *)(record + 0x20) = 500;
        *(int32_t *)(record + 0x1c) = 0x2ee;
        autopatch_status_flag_00692b11 = 0;
        return;
    }
    case 3:
    case 4:
        if (security_check_write_access() == 0) {
            if (quit_confirm_error_string_index == -1) {
                quit_confirm_error_string_index = 0x3a;
                quit_confirm_error_unknown_ae = 0;
                quit_confirm_error_modal = 1;
                quit_confirm_error_is_error = 0;
            }
            autopatch_status_active_00719235 = 1;
            widget_instance_close_and_restore_previous(row);
            autopatch_status_flag_00692b11 = 0;
            return;
        }
        control->selection_index = 1;
        row->hidden = 0;
        row->scale = 1.0f;
        autopatch_status_flag_00692b11 = 0;
        break;
    case (int32_t)-1:
    case 1:
        control->selection_index = 0;
        row->hidden = 1;
        row->scale = 0.333f;
        return;
    }
}

#if 0
Original Ghidra decompilation (0x4a4880):

void autopatch_status_widget_update(int param_1)

{
  int iVar1;
  int iVar2;
  undefined4 uVar3;
  int iVar4;
  bool bVar5;

  iVar1 = *(int *)(param_1 + 0x34);
  iVar2 = *(int *)(*(int *)(*(int *)(iVar1 + 0x2c) + 0x2c) + 0x34);
  if (DAT_00719235 == '\0') {
    uVar3 = autopatch_check_for_update_start();
    switch(uVar3) {
    case 0:
    case 2:
      bVar5 = DAT_00692b11 == '\0';
      *(undefined2 *)(iVar1 + 0x40) = 0;
      DAT_00719234 = 1;
      *(undefined1 *)(iVar2 + 0x12) = 1;
      *(undefined4 *)(iVar2 + 0x24) = 0x3eaa7efa;
      if (bVar5) {
        uVar3 = FUN_00449210();
        DAT_00719235 = 1;
        *(undefined4 *)(param_1 + 0x18) = uVar3;
        *(undefined4 *)(param_1 + 0x20) = 300;
        *(undefined4 *)(param_1 + 0x1c) = 300;
        DAT_00692b11 = 0;
        return;
      }
      uVar3 = FUN_00449210();
      DAT_00719235 = 1;
      *(undefined4 *)(param_1 + 0x18) = uVar3;
      *(undefined4 *)(param_1 + 0x20) = 500;
      *(undefined4 *)(param_1 + 0x1c) = 0x2ee;
      DAT_00692b11 = 0;
      return;
    case 3:
    case 4:
      iVar4 = security_check_write_access();
      if (iVar4 == 0) {
        if (DAT_00718fac == -1) {
          DAT_00718fac = 0x3a;
          DAT_00718fae = 0;
          DAT_00718fb0 = 1;
          DAT_00718fb1 = 0;
        }
        DAT_00719235 = 1;
        FUN_0049c3e0();
        DAT_00692b11 = 0;
        return;
      }
      *(undefined2 *)(iVar1 + 0x40) = 1;
      *(undefined1 *)(iVar2 + 0x12) = 0;
      *(undefined4 *)(iVar2 + 0x24) = 0x3f800000;
      DAT_00692b11 = '\0';
      break;
    case 0xffffffff:
    case 1:
      *(undefined2 *)(iVar1 + 0x40) = 0;
      *(undefined1 *)(iVar2 + 0x12) = 1;
      *(undefined4 *)(iVar2 + 0x24) = 0x3eaa7efa;
      return;
    }
  }
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
