#include "halo/devices/api.hpp"

extern "C" {
extern data_array *device_groups;
}

namespace halo::devices {

Globals &globals()
{
    static Globals instance{::device_groups};
    return instance;
}
}
