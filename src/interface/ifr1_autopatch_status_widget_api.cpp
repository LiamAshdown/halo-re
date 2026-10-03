#include "halo/interface/ifr1_autopatch_status_widget.hpp"
#include "halo/interface/api.hpp"

namespace halo::interface {

/**
 * C ABI entry point; forwards to halo::interface::AutopatchStatusWidget::widget_update.
 * blam-cc: param_1 is a larger record embedding a widget_instance; see file header.
 *
 * @address 0x4a4880
 */
void autopatch_status_widget_update(uint8_t *record)
{
    halo::interface::AutopatchStatusWidget::widget_update(record);
}

}
