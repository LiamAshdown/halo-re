/**
 * @file include/halo/devices/vars.hpp
 * Addresses of the engine variables the devices module owns, by name. Other files bind a typed reference to the entry they use
 * (halo::link::ref<T>); the link names themselves appear only in standalone/data/link/devices_vars.hpp.
 */
#pragma once

namespace halo::devices {

/** Address table of the engine variables owned by the devices module. */
struct Vars {
    void *device_groups;
};

/** The singleton address table; its storage is constant-initialised. */
const Vars &vars();

}  // namespace halo::devices
