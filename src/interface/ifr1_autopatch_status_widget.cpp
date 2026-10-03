#include "halo/interface/ifr1_autopatch_status_widget.hpp"
#include "halo/cseries/api.hpp"
#include "halo/shell/api.hpp"
#include "halo/networking/api.hpp"
#include "halo/interface/api.hpp"
#include "halo/core/link.hpp"
#include "halo/interface/vars.hpp"

static auto &autopatch_status_active_00719235 = halo::link::ref<uint8_t>(halo::ui::vars().autopatch_status_active_00719235);
static auto &autopatch_status_flag_00692b11 = halo::link::ref<uint8_t>(halo::ui::vars().autopatch_status_flag_00692b11);
static auto &autopatch_status_state_00719234 = halo::link::ref<uint8_t>(halo::ui::vars().autopatch_status_state_00719234);
static auto &quit_confirm_error_string_index = halo::link::ref<int16_t>(halo::ui::vars().quit_confirm_error_string_index);
static auto &quit_confirm_error_unknown_ae = halo::link::ref<int16_t>(halo::ui::vars().quit_confirm_error_unknown_ae);
static auto &quit_confirm_error_modal = halo::link::ref<uint8_t>(halo::ui::vars().quit_confirm_error_modal);
static auto &quit_confirm_error_is_error = halo::link::ref<uint8_t>(halo::ui::vars().quit_confirm_error_is_error);

namespace halo::interface {

/**
 *
 * blam-cc: param_1 is a larger record embedding a widget_instance; see file header.
 *
 * @address 0x4a4880
 */
void AutopatchStatusWidget::widget_update(widget_instance *record)
{
    widget_instance *control = record->first_child;
    widget_instance *row = control->next_sibling->next_sibling->first_child;

    if (autopatch_status_active_00719235 != 0) {
        return;
    }

    switch (halo::networking::autopatch_check_for_update_start()) {
    case 0:
    case 2: {
        uint8_t first_time = (autopatch_status_flag_00692b11 == 0);

        control->selection_index = 0;
        autopatch_status_state_00719234 = 1;
        row->hidden = 1;
        row->scale = 0.333f;
        if (first_time) {
            record->creation_time = halo::cseries::time_query_performance_counter_ms();
            autopatch_status_active_00719235 = 1;
            record->milliseconds_auto_close_fade = 300;
            record->milliseconds_to_auto_close = 300;
            autopatch_status_flag_00692b11 = 0;
            return;
        }
        record->creation_time = halo::cseries::time_query_performance_counter_ms();
        autopatch_status_active_00719235 = 1;
        record->milliseconds_auto_close_fade = 500;
        record->milliseconds_to_auto_close = 750;
        autopatch_status_flag_00692b11 = 0;
        return;
    }
    case 3:
    case 4:
        if (halo::shell::security_check_write_access() == 0) {
            if (quit_confirm_error_string_index == -1) {
                quit_confirm_error_string_index = 0x3a;
                quit_confirm_error_unknown_ae = 0;
                quit_confirm_error_modal = 1;
                quit_confirm_error_is_error = 0;
            }
            autopatch_status_active_00719235 = 1;
            halo::interface::widget_instance_close_and_restore_previous(row);
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
