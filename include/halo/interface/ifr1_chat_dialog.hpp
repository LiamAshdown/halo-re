#pragma once

#include "crt.h"
#include "tags.h"
#include "memory.h"
#include "cache.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"
#include "objects.h"
#include "units.h"
#include "main.h"
#include <stdint.h>
#include <stdarg.h>

#ifdef interface
#undef interface
#endif

namespace halo::interface {

/**
 * Behaviour of the original ChatDialog functions.
 */
class ChatDialog {
public:
    static void close(void);
    static int32_t default_team_channel(void);
    static void dispatch_incoming(void *event);
    static uint8_t poll_hotkeys(void);
    static void queue_team_message(int32_t team_index);
    static void server_relay_incoming_message(void **context, void *machine);
    static void submit_input(void);
    static void open(int32_t chat_scope);
    static void out(uint8_t channel);
};

}
