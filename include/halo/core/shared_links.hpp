#pragma once

#include <stdint.h>
#include "halo/core/link.hpp"
#include "halo/hs/vars.hpp"
#include "halo/interface/vars.hpp"
#include "halo/rasterizer/vars.hpp"


/**
 * Link-address references used by more than one module header, defined once instead of behind include guards.
 */
static auto &motd_download_slot = halo::link::ref<int32_t>(halo::ui::vars().motd_download_slot);
static auto &console_debug_toggle_689404 = halo::link::ref<uint8_t>(halo::rasterizer::vars().console_debug_toggle_689404);
static auto &decals_and_lens_flares_enabled = halo::link::ref<uint8_t>(halo::ui::vars().decals_and_lens_flares_enabled);
static auto &object_lod_quality = halo::link::ref<int16_t>(halo::ui::vars().object_lod_quality);
static auto &framerate_throttle = halo::link::ref<uint8_t>(halo::hs::vars().framerate_throttle);
static auto &reset_map = halo::link::ref<uint8_t>(halo::ui::vars().reset_map);
static auto &water_ripple_update_pending = halo::link::ref<uint8_t>(halo::rasterizer::vars().water_ripple_update_pending);
static auto &transparent_group_created = halo::link::ref<uint8_t>(halo::rasterizer::vars().transparent_group_created);
static auto &unknown_00721eac = halo::link::ref<void (*)(void *engine)>(halo::rasterizer::vars().unknown_00721eac);
