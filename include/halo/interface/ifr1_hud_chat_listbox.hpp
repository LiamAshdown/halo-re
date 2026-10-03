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
 * The chat message listbox shown on the HUD: update, clear and expiry of the oldest line.
 */
class HudChatListbox {
public:
    static void clear(void);
    static uint32_t remove_oldest(void);
    static void update(void);
};

}
