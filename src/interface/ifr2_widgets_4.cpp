#include "win32.h"
#include "halo/interface/engine_state.hpp"
#include "halo/interface/ifr2_widgets.hpp"
#include "halo/networking/api.hpp"
#include "halo/interface/api.hpp"

#ifdef interface
#undef interface
#endif

extern "C" {
extern heap *widget_memory_pool;
extern uint8_t widget_memory_pool_valid;
extern widget_instance *ui_root_widget[1];
extern int16_t quit_confirm_error_string_index;
extern ui_pending_error ui_pending_error_alternate;
extern ui_pending_error ui_pending_errors[4];
}

namespace halo::interface {

/**
 * Allocates the widget system's 0x20000 byte heap arena, re-derives its heap header (clearing and restoring
 * base/size/unknown_00/maximum_blocks around a full header wipe, then self-referencing blocks[0]), and clears
 * every UI global from ui_root_widget through widget_memory_pool_valid before setting the latter from whether
 * the allocation succeeded.
 *
 * @address 0x4979b0
 */
void WidgetLifecycle::memory_pool_initialize()
{
    void *allocation;
    uint8_t *base;
    int32_t size;
    uint32_t unknown_00;
    int32_t maximum_blocks;
    heap_block **blocks;
    int32_t i;
    uint8_t *clear_cursor;

    allocation = GlobalAlloc(0, 0x20000);
    if (allocation != nullptr) {
        widget_memory_pool->base = (uint8_t *)allocation;
        widget_memory_pool->size = 0x20000;
    }

    base = widget_memory_pool->base;
    size = widget_memory_pool->size;
    unknown_00 = widget_memory_pool->unknown_00;
    maximum_blocks = widget_memory_pool->maximum_blocks;

    blocks = widget_memory_pool->blocks;
    for (i = 0; i < maximum_blocks; i++) {
        blocks[i] = (heap_block *)0;
    }
    for (clear_cursor = (uint8_t *)widget_memory_pool, i = 0; i < 0x34; i++) {
        clear_cursor[i] = 0;
    }

    widget_memory_pool->base = base;
    widget_memory_pool->size = size;
    widget_memory_pool->unknown_00 = unknown_00;
    widget_memory_pool->maximum_blocks = maximum_blocks;
    widget_memory_pool->blocks[0] = (heap_block *)&widget_memory_pool->blocks[0];

    for (clear_cursor = (uint8_t *)&ui_root_widget[0], i = 0; i < 0x34; i++) {
        clear_cursor[i] = 0;
    }
    halo::networking::globals().join_error_code = -1;
    ui_pending_error_alternate.error_string_index = -1;
    quit_confirm_error_string_index = -1;
    ui_pending_errors[0].error_string_index = -1;
    state::screen_fade_progress = -1.0f;
    widget_memory_pool_valid = (allocation != nullptr);
}

} // namespace halo::interface

namespace halo::interface {

void widget_memory_pool_initialize(void)
{
    halo::interface::WidgetLifecycle::memory_pool_initialize();
}

}
