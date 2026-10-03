/**
 * @file src/devices/vars.cpp
 * Binds halo::devices::Vars to the engine variables the data image defines under their original link names.
 */

#include "halo/devices/vars.hpp"
#include "link/devices_vars.hpp"
#include "halo/devices/api.hpp"

namespace halo::devices {

const Vars &vars()
{
    static const Vars table{
        device_groups,
    };
    return table;
}

}  // namespace halo::devices
