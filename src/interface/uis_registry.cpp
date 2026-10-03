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
    {7, 0x49cdd0, &UiEventHandlers::event_49cdd0},
    {9, 0x49cfa0, &UiEventHandlers::event_49cfa0},
    {11, 0x49d100, &UiEventHandlers::event_49d100},
    {12, 0x49d120, &UiEventHandlers::event_49d120},
    {13, 0x49d140, &UiEventHandlers::event_49d140},
    {14, 0x49d160, &UiEventHandlers::event_49d160},
    {15, 0x49d1a0, &UiEventHandlers::event_49d1a0},
    {16, 0x49d1b0, &UiEventHandlers::event_49d1b0},
    {18, 0x49d440, &UiEventHandlers::event_49d440},
    {19, 0x49d450, &UiEventHandlers::event_49d450},
    {21, 0x49d480, &UiEventHandlers::event_49d480},
    {22, 0x49d520, &UiEventHandlers::event_49d520},
    {23, 0x49d540, &UiEventHandlers::event_49d540},
    {24, 0x49d5b0, &UiEventHandlers::event_49d5b0},
    {25, 0x49d5d0, &UiEventHandlers::event_49d5d0},
    {26, 0x49d5f0, &UiEventHandlers::event_49d5f0},
    {27, 0x49d7a0, &UiEventHandlers::event_49d7a0},
    {29, 0x49d8b0, &UiEventHandlers::event_49d8b0},
    {31, 0x49dab0, &UiEventHandlers::event_49dab0},
    {32, 0x49dbc0, &UiEventHandlers::event_49dbc0},
    {33, 0x49dca0, &UiEventHandlers::event_49dca0},
    {38, 0x49e170, &UiEventHandlers::event_49e170},
    {39, 0x49e210, &UiEventHandlers::event_49e210},
    {40, 0x49e220, &UiEventHandlers::event_49e220},
    {41, 0x49e2c0, &UiEventHandlers::event_49e2c0},
    {42, 0x49e300, &UiEventHandlers::event_49e300},
    {43, 0x49e5d0, &UiEventHandlers::event_49e5d0},
    {44, 0x49e7e0, &UiEventHandlers::event_49e7e0},
    {45, 0x49ea50, &UiEventHandlers::event_49ea50},
    {46, 0x49edc0, &UiEventHandlers::event_49edc0},
    {47, 0x49f030, &UiEventHandlers::event_49f030},
    {48, 0x49f300, &UiEventHandlers::event_49f300},
    {49, 0x49f470, &UiEventHandlers::event_49f470},
    {50, 0x49f560, &UiEventHandlers::event_49f560},
    {51, 0x49f610, &UiEventHandlers::event_49f610},
    {52, 0x49f680, &UiEventHandlers::event_49f680},
    {53, 0x49f8f0, &UiEventHandlers::event_49f8f0},
    {54, 0x49fad0, &UiEventHandlers::event_49fad0},
    {55, 0x49fd30, &UiEventHandlers::event_49fd30},
    {57, 0x4a02a0, &UiEventHandlers::event_4a02a0},
    {58, 0x4a0590, &UiEventHandlers::event_4a0590},
    {59, 0x4a0700, &UiEventHandlers::event_4a0700},
    {60, 0x4a07e0, &UiEventHandlers::event_4a07e0},
    {61, 0x4a0860, &UiEventHandlers::event_4a0860},
    {63, 0x4a0a80, &UiEventHandlers::event_4a0a80},
    {64, 0x4a0ae0, &UiEventHandlers::event_4a0ae0},
    {66, 0x4a0bc0, &UiEventHandlers::event_4a0bc0},
    {67, 0x4a0c00, &UiEventHandlers::event_4a0c00},
    {68, 0x4a0c60, &UiEventHandlers::event_4a0c60},
    {69, 0x4a0d60, &UiEventHandlers::event_4a0d60},
    {70, 0x4a0e90, &UiEventHandlers::event_4a0e90},
    {71, 0x4a0fb0, &UiEventHandlers::event_4a0fb0},
    {72, 0x4a10f0, &UiEventHandlers::event_4a10f0},
    {74, 0x4a1180, &UiEventHandlers::event_4a1180},
    {75, 0x4a11e0, &UiEventHandlers::event_4a11e0},
    {76, 0x4a1280, &UiEventHandlers::event_4a1280},
    {77, 0x4a12c0, &UiEventHandlers::event_4a12c0},
    {78, 0x4a12f0, &UiEventHandlers::event_4a12f0},
    {79, 0x4a1310, &UiEventHandlers::event_4a1310},
    {80, 0x4a1480, &UiEventHandlers::event_4a1480},
    {81, 0x4a1570, &UiEventHandlers::event_4a1570},
    {82, 0x4a15e0, &UiEventHandlers::event_4a15e0},
    {83, 0x4a1650, &UiEventHandlers::event_4a1650},
    {85, 0x4a16a0, &UiEventHandlers::event_4a16a0},
    {86, 0x4a16c0, &UiEventHandlers::event_4a16c0},
    {88, 0x4a16d0, &UiEventHandlers::event_4a16d0},
    {89, 0x4a16e0, &UiEventHandlers::event_4a16e0},
    {90, 0x4a1700, &UiEventHandlers::event_4a1700},
    {92, 0x4a1740, &UiEventHandlers::event_4a1740},
    {93, 0x4a1790, &UiEventHandlers::event_4a1790},
    {94, 0x4a1900, &UiEventHandlers::event_4a1900},
    {98, 0x4a1b00, &UiEventHandlers::event_4a1b00},
    {99, 0x4a1b60, &UiEventHandlers::event_4a1b60},
    {100, 0x4a1bf0, &UiEventHandlers::event_4a1bf0},
    {104, 0x49d0d0, &UiEventHandlers::event_49d0d0},
    {109, 0x4a1cd0, &UiEventHandlers::event_4a1cd0},
    {110, 0x4a1d00, &UiEventHandlers::event_4a1d00},
    {111, 0x4a1d30, &UiEventHandlers::event_4a1d30},
    {112, 0x4a1dc0, &UiEventHandlers::event_4a1dc0},
    {113, 0x4b4980, &UiEventHandlers::event_4b4980},
    {114, 0x4bb290, &UiEventHandlers::event_4bb290},
    {115, 0x4b52f0, &UiEventHandlers::event_4b52f0},
    {121, 0x4a2190, &UiEventHandlers::event_4a2190},
    {122, 0x4a21c0, &UiEventHandlers::event_4a21c0},
    {123, 0x4a2490, &UiEventHandlers::event_4a2490},
    {124, 0x4a24c0, &UiEventHandlers::event_4a24c0},
    {125, 0x4bb360, &UiEventHandlers::event_4bb360},
    {126, 0x4b4af0, &UiEventHandlers::event_4b4af0},
    {127, 0x4b4c40, &UiEventHandlers::event_4b4c40},
    {128, 0x4a1d60, &UiEventHandlers::event_4a1d60},
    {134, 0x4a2950, &UiEventHandlers::event_4a2950},
    {139, 0x4a2a00, &UiEventHandlers::event_4a2a00},
    {142, 0x4a2c50, &UiEventHandlers::event_4a2c50},
    {143, 0x4a2c80, &UiEventHandlers::event_4a2c80},
    {144, 0x4a2f10, &UiEventHandlers::event_4a2f10},
    {145, 0x4bb7e0, &UiEventHandlers::event_4bb7e0},
    {146, 0x4bb970, &UiEventHandlers::event_4bb970},
    {147, 0x4bba80, &UiEventHandlers::event_4bba80},
    {152, 0x4b5350, &UiEventHandlers::event_4b5350},
    {153, 0x4b54a0, &UiEventHandlers::event_4b54a0},
    {154, 0x4b54c0, &UiEventHandlers::event_4b54c0},
    {155, 0x4a3000, &UiEventHandlers::event_4a3000},
    {156, 0x4a3050, &UiEventHandlers::event_4a3050},
    {157, 0x4a3150, &UiEventHandlers::event_4a3150},
    {158, 0x4a33a0, &UiEventHandlers::event_4a33a0},
    {159, 0x4a3510, &UiEventHandlers::event_4a3510},
    {160, 0x4a3540, &UiEventHandlers::event_4a3540},
    {161, 0x4a3790, &UiEventHandlers::event_4a3790},
    {162, 0x4a3870, &UiEventHandlers::event_4a3870},
    {164, 0x4a1d90, &UiEventHandlers::event_4a1d90},
    {166, 0x4a39c0, &UiEventHandlers::event_4a39c0},
    {167, 0x4a39e0, &UiEventHandlers::event_4a39e0},
    {169, 0x4a4110, &UiEventHandlers::event_4a4110},
    {171, 0x4a3d40, &UiEventHandlers::event_4a3d40},
    {172, 0x4a4190, &UiEventHandlers::event_4a4190},
    {173, 0x4bb300, &UiEventHandlers::event_4bb300},
    {174, 0x4a41a0, &UiEventHandlers::event_4a41a0},
    {175, 0x4a4270, &UiEventHandlers::event_4a4270},
    {176, 0x4a44f0, &UiEventHandlers::event_4a44f0},
    {177, 0x4a4570, &UiEventHandlers::event_4a4570},
    {178, 0x4a45f0, &UiEventHandlers::event_4a45f0},
    {179, 0x4a47b0, &UiEventHandlers::event_4a47b0},
    {182, 0x4a4870, &UiEventHandlers::event_4a4870},
    {185, 0x4a4af0, &UiEventHandlers::event_4a4af0},
    {186, 0x4a3a70, &UiEventHandlers::event_4a3a70},
    {188, 0x4a4580, &UiEventHandlers::event_4a4580},
    {189, 0x4a45d0, &UiEventHandlers::event_4a45d0},
    {-1, 0x4a1c80, &UiEventHandlers::event_4a1c80},
    {-1, 0x4a1ca0, &UiEventHandlers::event_4a1ca0},
};

constexpr GameDataInputEntry k_game_data_inputs[22] = {
    {1, 0x4a4c70, &UiGameDataInputs::input_4a4c70},
    {11, 0x4a5740, &UiGameDataInputs::input_4a5740},
    {20, 0x4a6880, &UiGameDataInputs::input_4a6880},
    {24, 0x4a6a60, &UiGameDataInputs::input_4a6a60},
    {25, 0x4a6ab0, &UiGameDataInputs::input_4a6ab0},
    {27, 0x4a6b70, &UiGameDataInputs::input_4a6b70},
    {28, 0x4a6d50, &UiGameDataInputs::input_4a6d50},
    {29, 0x4a6e50, &UiGameDataInputs::input_4a6e50},
    {30, 0x4a6e90, &UiGameDataInputs::input_4a6e90},
    {31, 0x4a6f00, &UiGameDataInputs::input_4a6f00},
    {32, 0x4a6fa0, &UiGameDataInputs::input_4a6fa0},
    {33, 0x4a7180, &UiGameDataInputs::input_4a7180},
    {34, 0x4a7210, &UiGameDataInputs::input_4a7210},
    {35, 0x4a7280, &UiGameDataInputs::input_4a7280},
    {38, 0x4a7300, &UiGameDataInputs::input_4a7300},
    {39, 0x4a7340, &UiGameDataInputs::input_4a7340},
    {40, 0x4a7350, &UiGameDataInputs::input_4a7350},
    {42, 0x4a73d0, &UiGameDataInputs::input_4a73d0},
    {47, 0x4b5ce0, &UiGameDataInputs::input_4b5ce0},
    {50, 0x4a7880, &UiGameDataInputs::input_4a7880},
    {58, 0x4a3b70, &UiGameDataInputs::input_4a3b70},
    {-1, 0x4a7660, &UiGameDataInputs::input_4a7660},
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

/** Finds the entry whose original code lived at the given address, or null. */
const WidgetEventEntry *WidgetEventRegistry::find_by_address(uint32_t address) noexcept
{
    for (const WidgetEventEntry &entry : k_widget_events) {
        if (entry.address == address) {
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

/** Finds the entry whose original code lived at the given address, or null. */
const GameDataInputEntry *GameDataInputRegistry::find_by_address(uint32_t address) noexcept
{
    for (const GameDataInputEntry &entry : k_game_data_inputs) {
        if (entry.address == address) {
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
