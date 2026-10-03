/**
 * Widget event and game-data input registries: constexpr tables over the UiEventHandlers and UiGameDataInputs members.
 */

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "interface.h"

#include "halo/interface/uis_registry.hpp"
#include "halo/interface/uis_event_handlers.hpp"
#include "halo/interface/uis_game_data_inputs.hpp"

namespace halo::ui {

namespace {

constexpr WidgetEventEntry k_widget_events[129] = {
    {7, &UiEventHandlers::event_49cdd0},
    {9, &UiEventHandlers::event_49cfa0},
    {11, &UiEventHandlers::event_49d100},
    {12, &UiEventHandlers::event_49d120},
    {13, &UiEventHandlers::event_49d140},
    {14, &UiEventHandlers::event_49d160},
    {15, &UiEventHandlers::event_49d1a0},
    {16, &UiEventHandlers::event_49d1b0},
    {18, &UiEventHandlers::event_49d440},
    {19, &UiEventHandlers::event_49d450},
    {21, &UiEventHandlers::event_49d480},
    {22, &UiEventHandlers::event_49d520},
    {23, &UiEventHandlers::event_49d540},
    {24, &UiEventHandlers::event_49d5b0},
    {25, &UiEventHandlers::event_49d5d0},
    {26, &UiEventHandlers::event_49d5f0},
    {27, &UiEventHandlers::event_49d7a0},
    {29, &UiEventHandlers::event_49d8b0},
    {31, &UiEventHandlers::event_49dab0},
    {32, &UiEventHandlers::event_49dbc0},
    {33, &UiEventHandlers::event_49dca0},
    {38, &UiEventHandlers::event_49e170},
    {39, &UiEventHandlers::event_49e210},
    {40, &UiEventHandlers::event_49e220},
    {41, &UiEventHandlers::event_49e2c0},
    {42, &UiEventHandlers::event_49e300},
    {43, &UiEventHandlers::event_49e5d0},
    {44, &UiEventHandlers::event_49e7e0},
    {45, &UiEventHandlers::event_49ea50},
    {46, &UiEventHandlers::event_49edc0},
    {47, &UiEventHandlers::event_49f030},
    {48, &UiEventHandlers::event_49f300},
    {49, &UiEventHandlers::event_49f470},
    {50, &UiEventHandlers::event_49f560},
    {51, &UiEventHandlers::event_49f610},
    {52, &UiEventHandlers::event_49f680},
    {53, &UiEventHandlers::event_49f8f0},
    {54, &UiEventHandlers::event_49fad0},
    {55, &UiEventHandlers::event_49fd30},
    {57, &UiEventHandlers::event_4a02a0},
    {58, &UiEventHandlers::event_4a0590},
    {59, &UiEventHandlers::event_4a0700},
    {60, &UiEventHandlers::event_4a07e0},
    {61, &UiEventHandlers::event_4a0860},
    {63, &UiEventHandlers::event_4a0a80},
    {64, &UiEventHandlers::event_4a0ae0},
    {66, &UiEventHandlers::event_4a0bc0},
    {67, &UiEventHandlers::event_4a0c00},
    {68, &UiEventHandlers::event_4a0c60},
    {69, &UiEventHandlers::event_4a0d60},
    {70, &UiEventHandlers::event_4a0e90},
    {71, &UiEventHandlers::event_4a0fb0},
    {72, &UiEventHandlers::event_4a10f0},
    {74, &UiEventHandlers::event_4a1180},
    {75, &UiEventHandlers::event_4a11e0},
    {76, &UiEventHandlers::event_4a1280},
    {77, &UiEventHandlers::event_4a12c0},
    {78, &UiEventHandlers::event_4a12f0},
    {79, &UiEventHandlers::event_4a1310},
    {80, &UiEventHandlers::event_4a1480},
    {81, &UiEventHandlers::event_4a1570},
    {82, &UiEventHandlers::event_4a15e0},
    {83, &UiEventHandlers::event_4a1650},
    {85, &UiEventHandlers::event_4a16a0},
    {86, &UiEventHandlers::event_4a16c0},
    {88, &UiEventHandlers::event_4a16d0},
    {89, &UiEventHandlers::event_4a16e0},
    {90, &UiEventHandlers::event_4a1700},
    {92, &UiEventHandlers::event_4a1740},
    {93, &UiEventHandlers::event_4a1790},
    {94, &UiEventHandlers::event_4a1900},
    {98, &UiEventHandlers::event_4a1b00},
    {99, &UiEventHandlers::event_4a1b60},
    {100, &UiEventHandlers::event_4a1bf0},
    {104, &UiEventHandlers::event_49d0d0},
    {109, &UiEventHandlers::event_4a1cd0},
    {110, &UiEventHandlers::event_4a1d00},
    {111, &UiEventHandlers::event_4a1d30},
    {112, &UiEventHandlers::event_4a1dc0},
    {113, &UiEventHandlers::event_4b4980},
    {114, &UiEventHandlers::event_4bb290},
    {115, &UiEventHandlers::event_4b52f0},
    {121, &UiEventHandlers::event_4a2190},
    {122, &UiEventHandlers::event_4a21c0},
    {123, &UiEventHandlers::event_4a2490},
    {124, &UiEventHandlers::event_4a24c0},
    {125, &UiEventHandlers::event_4bb360},
    {126, &UiEventHandlers::event_4b4af0},
    {127, &UiEventHandlers::event_4b4c40},
    {128, &UiEventHandlers::event_4a1d60},
    {134, &UiEventHandlers::event_4a2950},
    {139, &UiEventHandlers::event_4a2a00},
    {142, &UiEventHandlers::event_4a2c50},
    {143, &UiEventHandlers::event_4a2c80},
    {144, &UiEventHandlers::event_4a2f10},
    {145, &UiEventHandlers::event_4bb7e0},
    {146, &UiEventHandlers::event_4bb970},
    {147, &UiEventHandlers::event_4bba80},
    {152, &UiEventHandlers::event_4b5350},
    {153, &UiEventHandlers::event_4b54a0},
    {154, &UiEventHandlers::event_4b54c0},
    {155, &UiEventHandlers::event_4a3000},
    {156, &UiEventHandlers::event_4a3050},
    {157, &UiEventHandlers::event_4a3150},
    {158, &UiEventHandlers::event_4a33a0},
    {159, &UiEventHandlers::event_4a3510},
    {160, &UiEventHandlers::event_4a3540},
    {161, &UiEventHandlers::event_4a3790},
    {162, &UiEventHandlers::event_4a3870},
    {164, &UiEventHandlers::event_4a1d90},
    {166, &UiEventHandlers::event_4a39c0},
    {167, &UiEventHandlers::event_4a39e0},
    {169, &UiEventHandlers::event_4a4110},
    {171, &UiEventHandlers::event_4a3d40},
    {172, &UiEventHandlers::event_4a4190},
    {173, &UiEventHandlers::event_4bb300},
    {174, &UiEventHandlers::event_4a41a0},
    {175, &UiEventHandlers::event_4a4270},
    {176, &UiEventHandlers::event_4a44f0},
    {177, &UiEventHandlers::event_4a4570},
    {178, &UiEventHandlers::event_4a45f0},
    {179, &UiEventHandlers::event_4a47b0},
    {182, &UiEventHandlers::event_4a4870},
    {185, &UiEventHandlers::event_4a4af0},
    {186, &UiEventHandlers::event_4a3a70},
    {188, &UiEventHandlers::event_4a4580},
    {189, &UiEventHandlers::event_4a45d0},
    {-1, &UiEventHandlers::event_4a1c80},
    {-1, &UiEventHandlers::event_4a1ca0},
};

constexpr GameDataInputEntry k_game_data_inputs[22] = {
    {1, &UiGameDataInputs::input_4a4c70},
    {11, &UiGameDataInputs::input_4a5740},
    {20, &UiGameDataInputs::input_4a6880},
    {24, &UiGameDataInputs::input_4a6a60},
    {25, &UiGameDataInputs::input_4a6ab0},
    {27, &UiGameDataInputs::input_4a6b70},
    {28, &UiGameDataInputs::input_4a6d50},
    {29, &UiGameDataInputs::input_4a6e50},
    {30, &UiGameDataInputs::input_4a6e90},
    {31, &UiGameDataInputs::input_4a6f00},
    {32, &UiGameDataInputs::input_4a6fa0},
    {33, &UiGameDataInputs::input_4a7180},
    {34, &UiGameDataInputs::input_4a7210},
    {35, &UiGameDataInputs::input_4a7280},
    {38, &UiGameDataInputs::input_4a7300},
    {39, &UiGameDataInputs::input_4a7340},
    {40, &UiGameDataInputs::input_4a7350},
    {42, &UiGameDataInputs::input_4a73d0},
    {47, &UiGameDataInputs::input_4b5ce0},
    {50, &UiGameDataInputs::input_4a7880},
    {58, &UiGameDataInputs::input_4a3b70},
    {-1, &UiGameDataInputs::input_4a7660},
};

}

/** Returns every registered entry in table order (unregistered handlers last). */
EntryRange<WidgetEventEntry> WidgetEventRegistry::entries() noexcept
{
    return EntryRange<WidgetEventEntry>{k_widget_events, sizeof(k_widget_events) / sizeof(k_widget_events[0])};
}

/** Finds the entry registered in the given original table slot, or null. */
const WidgetEventEntry *WidgetEventRegistry::find_by_table_index(int16_t table_index) noexcept
{
    for (const WidgetEventEntry &entry : k_widget_events) {
        if (entry.table_index == table_index) {
            return &entry;
        }
    }
    return nullptr;
}

/** Returns every registered entry in table order (unregistered handlers last). */
EntryRange<GameDataInputEntry> GameDataInputRegistry::entries() noexcept
{
    return EntryRange<GameDataInputEntry>{k_game_data_inputs, sizeof(k_game_data_inputs) / sizeof(k_game_data_inputs[0])};
}

/** Finds the entry registered in the given original table slot, or null. */
const GameDataInputEntry *GameDataInputRegistry::find_by_table_index(int16_t table_index) noexcept
{
    for (const GameDataInputEntry &entry : k_game_data_inputs) {
        if (entry.table_index == table_index) {
            return &entry;
        }
    }
    return nullptr;
}

/** Runs the handler registered in the given slot; an unregistered slot reports the event as not handled (0). */
uint8_t WidgetEventRegistry::dispatch(int16_t table_index, widget_instance *widget, int16_t *event, uint8_t *out_handled)
{
    const WidgetEventEntry *entry = find_by_table_index(table_index);
    return entry != nullptr ? entry->handler(widget, event, out_handled) : 0;
}

/** Runs the input function registered in the given slot; returns false when the slot is empty. */
bool GameDataInputRegistry::dispatch(int16_t table_index, widget_instance *widget)
{
    const GameDataInputEntry *entry = find_by_table_index(table_index);
    if (entry == nullptr) {
        return false;
    }
    entry->handler(widget);
    return true;
}

}
