#pragma once

#include <stdint.h>
#include "halo/core/link.hpp"
#include "halo/hs/vars.hpp"
#include "halo/interface/vars.hpp"
#include "halo/rasterizer/vars.hpp"


/**
 * Link-address references used by more than one module header, defined once instead of behind include guards.
 */
inline auto &DAT_00695420 = halo::link::ref<int32_t>(halo::ui::vars().DAT_00695420);
inline auto &console_debug_toggle_689404 = halo::link::ref<uint8_t>(halo::rasterizer::vars().console_debug_toggle_689404);
inline auto &unknown_006893ff = halo::link::ref<uint8_t>(halo::ui::vars().unknown_006893ff);
inline auto &unknown_00689450 = halo::link::ref<int16_t>(halo::ui::vars().unknown_00689450);
inline auto &unknown_006894ba = halo::link::ref<uint8_t>(halo::hs::vars().unknown_006894ba);
inline auto &unknown_00719738 = halo::link::ref<uint8_t>(halo::ui::vars().unknown_00719738);
inline auto &unknown_0071d275 = halo::link::ref<uint8_t>(halo::rasterizer::vars().unknown_0071d275);
inline auto &unknown_0071d276 = halo::link::ref<uint8_t>(halo::rasterizer::vars().unknown_0071d276);
inline auto &unknown_00721eac = halo::link::ref<void (*)(void *engine)>(halo::rasterizer::vars().unknown_00721eac);
