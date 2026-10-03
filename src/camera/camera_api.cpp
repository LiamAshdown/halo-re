#include "halo/camera/camera.hpp"
#include "halo/camera/api.hpp"
#include "halo/core/link.hpp"
#include "halo/camera/vars.hpp"

static auto &hs_camera_control_pointer = halo::link::ref<uint8_t *>(halo::camera::vars().hs_camera_control_pointer);
static auto &observers = halo::link::ref<observer [1]>(halo::camera::vars().observers);
static auto &directors = halo::link::ref<director [1]>(halo::camera::vars().directors);
static auto &camera_script = halo::link::ref<camera_script_globals>(halo::camera::vars().camera_script);

namespace halo::camera {

Globals &globals()
{
    static Globals instance{::hs_camera_control_pointer, ::observers, ::directors, ::camera_script};
    return instance;
}

}  // namespace halo::camera
