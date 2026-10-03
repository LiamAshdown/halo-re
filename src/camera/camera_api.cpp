#include "halo/camera/camera.hpp"
#include "halo/camera/api.hpp"

extern "C" {
extern uint8_t *hs_camera_control_pointer;
extern observer observers[1];
extern director directors[1];
extern camera_script_globals camera_script;
}

namespace halo::camera {

Globals &globals()
{
    static Globals instance{::hs_camera_control_pointer, ::observers, ::directors, ::camera_script};
    return instance;
}

}  // namespace halo::camera
