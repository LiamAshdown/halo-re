#pragma once

/* Include after the engine type headers (types/*.h carry no include guards). */

#include <stddef.h>
#include <stdint.h>

namespace halo::ui {

/** Read-only view over a constexpr registry table. */
template <typename T>
struct EntryRange {
    const T *first;
    size_t count;

    [[nodiscard]] const T *begin() const noexcept { return first; }
    [[nodiscard]] const T *end() const noexcept { return first + count; }
    [[nodiscard]] size_t size() const noexcept { return count; }
};

/** Signature shared by every entry of ui_event_function_table. */
using WidgetEventFn = uint8_t (*)(widget_instance *widget, int16_t *event, uint8_t *out_handled);

/** Signature shared by every entry of game_data_input_function_table. */
using GameDataInputFn = void (*)(widget_instance *widget);

/**
 * One registered widget event handler: its slot in ui_event_function_table (-1 when the original is not
 * table-registered), its original code address and the behaviour itself.
 */
struct WidgetEventEntry {
    int16_t table_index;
    uint32_t address;
    WidgetEventFn handler;
};

/** One registered game-data input function, keyed like WidgetEventEntry. */
struct GameDataInputEntry {
    int16_t table_index;
    uint32_t address;
    GameDataInputFn handler;
};

/**
 * Registry of the widget event handlers implemented by UiEventHandlers. Lookup and dispatch go through the same
 * functions the original function-pointer table slots call, in table order.
 */
class WidgetEventRegistry {
public:
    [[nodiscard]] static EntryRange<WidgetEventEntry> entries() noexcept;
    [[nodiscard]] static const WidgetEventEntry *find_by_table_index(int16_t table_index) noexcept;
    [[nodiscard]] static const WidgetEventEntry *find_by_address(uint32_t address) noexcept;
    static uint8_t dispatch(int16_t table_index, widget_instance *widget, int16_t *event, uint8_t *out_handled);
};

/** Registry of the game-data input functions implemented by UiGameDataInputs. */
class GameDataInputRegistry {
public:
    [[nodiscard]] static EntryRange<GameDataInputEntry> entries() noexcept;
    [[nodiscard]] static const GameDataInputEntry *find_by_table_index(int16_t table_index) noexcept;
    [[nodiscard]] static const GameDataInputEntry *find_by_address(uint32_t address) noexcept;
    static bool dispatch(int16_t table_index, widget_instance *widget);
};

}
