#include "halo/interface/ifr1_autopatch_status_widget.hpp"
#include "halo/cseries/api.hpp"

extern "C" {
extern uint8_t autopatch_status_active_00719235;
extern uint8_t autopatch_status_flag_00692b11;
extern uint8_t autopatch_status_state_00719234;
extern int16_t quit_confirm_error_string_index;
extern int16_t quit_confirm_error_unknown_ae;
extern uint8_t quit_confirm_error_modal;
extern uint8_t quit_confirm_error_is_error;
extern void widget_instance_close_and_restore_previous(widget_instance *widget);
extern int32_t security_check_write_access(void);
extern int32_t autopatch_check_for_update_start(void);
}

namespace halo::interface {

/**
 * Original engine function autopatch_status_widget_update; the author notes are in
 * docs/original/interface/autopatch_status_widget_update.txt.
 * blam-cc: param_1 is a larger record embedding a widget_instance; see file header.
 *
 * @address 0x4a4880
 */
void AutopatchStatusWidget::widget_update(uint8_t *record)
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
            *(int32_t *)(record + 0x18) = halo::cseries::time_query_performance_counter_ms();
            autopatch_status_active_00719235 = 1;
            *(int32_t *)(record + 0x20) = 300;
            *(int32_t *)(record + 0x1c) = 300;
            autopatch_status_flag_00692b11 = 0;
            return;
        }
        *(int32_t *)(record + 0x18) = halo::cseries::time_query_performance_counter_ms();
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

}
