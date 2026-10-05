/**
 * @file src/rasterizer/gl_direct3d.cpp
 * The Direct3D 9 factory object (IDirect3D9) of builds without Direct3D 9: answers the adapter, display mode, format
 * and capability questions the engine asks (config.txt matching, the video options, device setup) through the same
 * method slots, so that code runs unchanged. The capabilities are those of the hardware the OpenGL renderer was
 * brought up against (a Direct3D 9 HAL device reporting shader model 3.0); the adapter identifies itself as OpenGL.
 */

#include "gl_internal.hpp"
#include "halo/platform/window.hpp"

#include <string.h>

namespace halo::rasterizer::gl {

namespace {

constexpr uint32_t k_format_x8r8g8b8 = 22;
constexpr int32_t k_invalid_call = static_cast<int32_t>(0x8876086c);

/** D3DCAPS9 (0x130 bytes) as dwords, in member order. */
const uint32_t k_caps[0x130 / 4] = {
    0x00000001, 0x00000000, 0x00020000, 0xe0020000, 0x000003a0, 0x8000000f, 0x00000001, 0x001bbef0,  // device type .. dev caps
    0x002fcef2, 0x07732191, 0x000000ff, 0x00003fff, 0x00003fff, 0x000000ff, 0x00084208, 0x0001ecc5,  // misc .. texture caps
    0x03030700, 0x03030300, 0x03030300, 0x0000003f, 0x0000003f, 0x0000001f, 0x00004000, 0x00004000,  // filters .. max size
    0x00000800, 0x00002000, 0x00004000, 0x00000010, 0x501502f9, 0xccbebc20, 0xccbebc20, 0x4cbebc20,  // volume .. guard band
    0x4cbebc20, 0x00000000, 0x000001ff, 0x00180008, 0x03feffff, 0x00000008, 0x00000008, 0x0000013b,  // .. vertex processing
    0x00000008, 0x00000008, 0x00000004, 0x00000000, 0x46000000, 0x00ffffff, 0x00ffffff, 0x00000010,  // lights .. max streams
    0x000000ff, 0xfffe0300, 0x00000100, 0xffff0300, 0x477fe000, 0x00000051, 0x00000000, 0x00000000,  // stride .. vs/ps 3.0
    0x00000000, 0x00000000, 0x00000001, 0x0000030f, 0x00000004, 0x03000300, 0x00000001, 0x00000018,  // groups .. vs20 caps
    0x00000020, 0x00000004, 0x0000001f, 0x00000018, 0x00000020, 0x00000004, 0x00000200, 0x03030700,  // .. ps20 caps
    0x0000ffff, 0x0000ffff, 0x00001000, 0x00001000,                                                  // instruction limits
};

/** Display modes offered (at most the desktop size), plus the desktop itself. */
const uint32_t k_modes[][2] = {
    {640, 480}, {800, 600}, {1024, 768}, {1280, 720}, {1280, 800}, {1280, 1024}, {1366, 768}, {1440, 900},
    {1600, 900}, {1600, 1200}, {1680, 1050}, {1920, 1080}, {1920, 1200}, {2560, 1440}, {2560, 1600}, {3840, 2160},
};

void desktop(uint32_t *width, uint32_t *height)
{
    halo::platform::desktop_size(width, height);
    if (*width == 0 || *height == 0) {
        *width = 1024;
        *height = 768;
    }
}

uint32_t mode_list(uint32_t (*out)[2])
{
    uint32_t width, height, count = 0;
    bool desktop_listed = false;

    desktop(&width, &height);
    for (const auto &mode : k_modes) {
        if (mode[0] <= width && mode[1] <= height) {
            out[count][0] = mode[0];
            out[count][1] = mode[1];
            desktop_listed = desktop_listed || (mode[0] == width && mode[1] == height);
            count++;
        }
    }
    if (!desktop_listed) {
        out[count][0] = width;
        out[count][1] = height;
        count++;
    }
    return count;
}

int32_t __stdcall query_interface(void *, const void *, void **out)
{
    *out = nullptr;
    return static_cast<int32_t>(0x80004002);  // E_NOINTERFACE
}

uint32_t __stdcall add_ref(void *)
{
    return 1;
}

uint32_t __stdcall release(void *)
{
    return 0;  // a static object
}

uint32_t __stdcall get_adapter_count(void *)
{
    return 1;
}

int32_t __stdcall get_adapter_identifier(void *, uint32_t adapter, uint32_t, void *out)
{
    uint8_t *identifier = static_cast<uint8_t *>(out);

    if (adapter != 0) {
        return k_invalid_call;
    }
    memset(identifier, 0, 0x44c);  // D3DADAPTER_IDENTIFIER9: no vendor or device id, so config.txt applies its defaults
    strcpy(reinterpret_cast<char *>(identifier), "opengl");
    strcpy(reinterpret_cast<char *>(identifier + 0x200), "OpenGL");
    strcpy(reinterpret_cast<char *>(identifier + 0x400), "\\\\.\\DISPLAY1");
    return 0;
}

uint32_t __stdcall get_adapter_mode_count(void *, uint32_t adapter, uint32_t format)
{
    uint32_t modes[sizeof(k_modes) / sizeof(k_modes[0]) + 1][2];

    return adapter == 0 && format == k_format_x8r8g8b8 ? mode_list(modes) : 0;
}

int32_t __stdcall enum_adapter_modes(void *, uint32_t adapter, uint32_t format, uint32_t index, uint32_t *out)
{
    uint32_t modes[sizeof(k_modes) / sizeof(k_modes[0]) + 1][2];

    if (adapter != 0 || format != k_format_x8r8g8b8 || index >= mode_list(modes)) {
        return k_invalid_call;
    }
    out[0] = modes[index][0];
    out[1] = modes[index][1];
    out[2] = 60;
    out[3] = k_format_x8r8g8b8;
    return 0;
}

int32_t __stdcall get_adapter_display_mode(void *, uint32_t adapter, uint32_t *out)
{
    if (adapter != 0) {
        return k_invalid_call;
    }
    desktop(&out[0], &out[1]);
    out[2] = 60;
    out[3] = k_format_x8r8g8b8;
    return 0;
}

int32_t __stdcall check_device_format(void *, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t)
{
    return 0;  // the renderer converts what it cannot take directly
}

int32_t __stdcall get_device_caps(void *, uint32_t adapter, uint32_t, void *out)
{
    if (adapter != 0) {
        return k_invalid_call;
    }
    memcpy(out, k_caps, sizeof(k_caps));
    return 0;
}

/** IDirect3D9's 17 slots; the ones the engine never calls stay null. */
void *g_vtable[17] = {
    reinterpret_cast<void *>(query_interface), reinterpret_cast<void *>(add_ref), reinterpret_cast<void *>(release), nullptr,
    reinterpret_cast<void *>(get_adapter_count), reinterpret_cast<void *>(get_adapter_identifier),
    reinterpret_cast<void *>(get_adapter_mode_count), reinterpret_cast<void *>(enum_adapter_modes),
    reinterpret_cast<void *>(get_adapter_display_mode), nullptr, reinterpret_cast<void *>(check_device_format), nullptr, nullptr, nullptr,
    reinterpret_cast<void *>(get_device_caps), nullptr, nullptr,
};

void *g_object = g_vtable;  // the object is its method table pointer

}  // namespace

}  // namespace halo::rasterizer::gl

namespace halo::rasterizer {

void *__stdcall gl_direct3d_create(uint32_t sdk_version)
{
    (void)sdk_version;
    return &gl::g_object;
}

}  // namespace halo::rasterizer
