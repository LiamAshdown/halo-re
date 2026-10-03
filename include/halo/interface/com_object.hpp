/**
 * @file include/halo/interface/com_object.hpp
 * Access to the method tables of the COM objects (the Direct3D device, DirectInput devices) the interface code calls into.
 */
#pragma once

namespace halo::interface {

/** The method table (vtable) of a COM object: the first word of the object points at it. */
inline void **com_vtable(void *object) {
    return *static_cast<void ***>(object);
}

}  // namespace halo::interface
