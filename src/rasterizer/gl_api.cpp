/**
 * @file src/rasterizer/gl_api.cpp
 * Run-time loader for the OpenGL entry points declared in halo/rasterizer/gl_api.hpp (WGL on Windows; an Emscripten
 * build would link them directly).
 */

#include "halo/rasterizer/gl_api.hpp"
#include "halo/shell/standalone.hpp"

#define HALO_GL_DEFINE(ret, name, args) ret(APIENTRY *name) args = nullptr;
HALO_GL_FUNCTIONS(HALO_GL_DEFINE)
#undef HALO_GL_DEFINE

bool gl_load_api()
{
    bool complete = true;

#define HALO_GL_LOAD(ret, name, args) \
    name = reinterpret_cast<ret(APIENTRY *) args>(wglGetProcAddress(#name)); \
    if (name == nullptr) { \
        halo::shell::standalone_log("gl: missing entry point %s", #name); \
        complete = false; \
    }
    HALO_GL_FUNCTIONS(HALO_GL_LOAD)
#undef HALO_GL_LOAD
    return complete;
}
