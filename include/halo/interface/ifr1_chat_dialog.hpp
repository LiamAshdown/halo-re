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
 * Strategy for turning a decoded incoming chat record into a line in the chat listbox. ChatDialog::dispatch_incoming
 * asks each registered source in order whether it accepts the record, so the precedence of the original branches
 * (player message first, then localized string, then plain text) is preserved.
 */
class ChatLineSource {
public:
    virtual bool accepts(const chat_incoming_record &record) const = 0;
    virtual void deliver(const chat_incoming_record &record, wchar_t *text) const = 0;

    /**
     * Returns the first registered source that accepts record, or nullptr when the record kind is not shown.
     */
    static const ChatLineSource *select(const chat_incoming_record &record);

protected:
    ~ChatLineSource() = default;
};

/**
 * The multiplayer chat input dialog and its incoming-message path: opening, hotkeys, submission and display of
 * received lines.
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
    static void queue_on_channel(network_channel *channel, int32_t encoded_bits);
};

}
