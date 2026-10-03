#include "halo/devices/api.hpp"
#include "halo/core/link.hpp"
#include "halo/devices/vars.hpp"

static auto &device_groups = halo::link::ref<data_array *>(halo::devices::vars().device_groups);

namespace halo::devices {

Globals &globals()
{
    static Globals instance{::device_groups};
    return instance;
}
}
