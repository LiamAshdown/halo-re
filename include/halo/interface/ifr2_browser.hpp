#pragma once

#include <stdint.h>
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "game.h"
#include "networking.h"
#include "interface.h"

#ifdef interface
#undef interface
#endif

namespace halo::interface {

/**
 * Strategy interface for the server browser widget event handlers. The engine data tables call the original
 * C entry points, which forward to one shared instance of the matching concrete handler.
 */
class ServerBrowserHandler {
public:
    virtual uint8_t handle(widget_instance *widget, int16_t *event, uint8_t *out_handled) const = 0;

protected:
    constexpr ServerBrowserHandler() = default;
    ~ServerBrowserHandler() = default;

    static void widget_show(widget_instance *widget, uint8_t shown);
    static widget_instance * find_control(widget_instance *row);
    static uint8_t clamp_selection(int16_t selection, int16_t maximum);
};

/**
 * Handler for the back server browser event.
 */
class BackHandler final : public ServerBrowserHandler {
public:
    uint8_t handle(widget_instance *widget, int16_t *event, uint8_t *out_handled) const override;
};

/**
 * Handler for the button server browser event.
 */
class ButtonHandler final : public ServerBrowserHandler {
public:
    uint8_t handle(widget_instance *widget, int16_t *event, uint8_t *out_handled) const override;
};

/**
 * Handler for the closed server browser event.
 */
class ClosedHandler final : public ServerBrowserHandler {
public:
    uint8_t handle(widget_instance *widget, int16_t *event, uint8_t *out_handled) const override;
};

/**
 * Handler for the filter panel apply server browser event.
 */
class FilterPanelApplyHandler final : public ServerBrowserHandler {
public:
    uint8_t handle(widget_instance *widget, int16_t *event, uint8_t *out_handled) const override;
};

/**
 * Handler for the filter panel cancel server browser event.
 */
class FilterPanelCancelHandler final : public ServerBrowserHandler {
public:
    uint8_t handle(widget_instance *widget, int16_t *event, uint8_t *out_handled) const override;
};

/**
 * Handler for the hide widget server browser event.
 */
class HideWidgetHandler final : public ServerBrowserHandler {
public:
    uint8_t handle(widget_instance *widget, int16_t *event, uint8_t *out_handled) const override;
};

/**
 * Handler for the list row server browser event.
 */
class ListRowHandler final : public ServerBrowserHandler {
public:
    uint8_t handle(widget_instance *widget, int16_t *event, uint8_t *out_handled) const override;
};

} // namespace halo::interface
