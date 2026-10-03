/**
 * @file standalone/data/link/devices_vars.hpp
 * Link names of the engine variables owned by the devices module (halo::devices::vars()). The data image defines them under these C
 * names; only src/devices/vars.cpp includes this header, so the names are declared as untyped storage.
 */
#pragma once

extern "C" {
extern char device_groups[];
}
