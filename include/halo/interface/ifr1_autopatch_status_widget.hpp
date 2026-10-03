#pragma once

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"
#include <stdint.h>
#include <stdarg.h>

#ifdef interface
#undef interface
#endif

namespace halo::interface {

/**
 * Per-frame update of the autopatch (game update) status widget.
 */
class AutopatchStatusWidget {
public:
    static void widget_update(uint8_t *record);
};

}
