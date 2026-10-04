/**
 * @file src/rasterizer/gl_device.cpp
 * OpenGL implementation of the RenderDevice interface, milestone 1 (see docs/OPENGL_PORT.md): a WGL context on the game
 * window that clears and presents, and CPU-side stand-ins for every resource so the engine can create, lock and fill them.
 * Nothing is drawn yet. Adapter and capability queries are forwarded to the real Direct3D 9 object the shell creates.
 */

#include "halo/rasterizer/gl_device.hpp"
#include "halo/shell/standalone.hpp"

#include <windows.h>
#include <GL/gl.h>
#include <stdlib.h>
#include <string.h>

namespace halo::rasterizer {

namespace {

constexpr uint32_t k_magic = 0x424f4c47;  // "GLOB": first dword of every object this backend hands out

enum kind : uint32_t {
    kind_device = 1,
    kind_texture,
    kind_volume_texture,
    kind_cube_texture,
    kind_vertex_buffer,
    kind_index_buffer,
    kind_surface,
    kind_declaration,
    kind_vertex_shader,
    kind_pixel_shader,
    kind_query,
    kind_effect,
};

struct gl_object {
    uint32_t magic;
    uint32_t kind;
    int32_t refs;
};

struct gl_level {
    uint32_t width, height, depth;
    uint32_t pitch;  // bytes per row (per block row for compressed formats)
    uint32_t slice;  // bytes per depth slice
    uint8_t *data;
};

constexpr uint32_t k_max_levels = 16;

struct gl_texture : gl_object {
    uint32_t format;
    uint32_t levels;
    uint32_t faces;
    gl_level level[6 * k_max_levels];
};

struct gl_surface : gl_object {
    uint32_t format;
    uint32_t width, height;
    uint32_t pitch;
    uint8_t *data;
    bool owns_data;
    gl_texture *parent;  // texture whose level this surface views (kept alive), or null
};

struct gl_buffer : gl_object {
    uint32_t size;
    uint32_t format;  // FVF for a vertex buffer, index format for an index buffer
    uint8_t *data;
};

struct gl_declaration : gl_object {
    uint32_t element_count;
    uint8_t *elements;  // 8 bytes per D3DVERTEXELEMENT9, end marker included
};

/* Direct3D 9 values the stand-ins need */
constexpr uint32_t k_fourcc_dxt1 = 0x31545844;
constexpr uint32_t k_fourcc_dxt2 = 0x32545844;
constexpr uint32_t k_fourcc_dxt3 = 0x33545844;
constexpr uint32_t k_fourcc_dxt4 = 0x34545844;
constexpr uint32_t k_fourcc_dxt5 = 0x35545844;

struct device_state {
    HWND window;
    HDC dc;
    HGLRC context;
    uint32_t width;
    uint32_t height;
    gl_object *device_object;
};

device_state g_state;

template <typename T>
T *new_object(uint32_t object_kind)
{
    T *object = static_cast<T *>(calloc(1, sizeof(T)));

    object->magic = k_magic;
    object->kind = object_kind;
    object->refs = 1;
    return object;
}

bool is_ours(const void *object)
{
    return object != nullptr && static_cast<const gl_object *>(object)->magic == k_magic;
}

/** Bytes per pixel, or 0 for a block-compressed format. */
uint32_t bytes_per_pixel(uint32_t format)
{
    switch (format) {
    case 20: return 3;                       // R8G8B8
    case 21: case 22: case 31: case 32: case 33: case 34: case 35: case 62: case 63: case 64:
    case 71: case 75: case 77: case 78: case 114: case 112:
        return 4;                            // A8R8G8B8, X8R8G8B8, A2B10G10R10, ..., D32, D24S8, D24X8, R32F, G16R16F
    case 23: case 24: case 25: case 26: case 29: case 30: case 40: case 51: case 60: case 61:
    case 70: case 73: case 80: case 81: case 82: case 111:
        return 2;                            // R5G6B5, X1R5G5B5, A1R5G5B5, A4R4G4B4, A8R3G3B2, ..., D16, R16F
    case 27: case 28: case 41: case 50: case 52:
        return 1;                            // R3G3B2, A8, P8, L8, A4L4
    case 36: case 113: case 115:
        return 8;                            // A16B16G16R16, A16B16G16R16F, G32R32F
    case 116:
        return 16;                           // A32B32G32R32F
    case k_fourcc_dxt1: case k_fourcc_dxt2: case k_fourcc_dxt3: case k_fourcc_dxt4: case k_fourcc_dxt5:
        return 0;
    default:
        return 4;
    }
}

uint32_t block_bytes(uint32_t format)
{
    return format == k_fourcc_dxt1 ? 8 : 16;
}

/** Row pitch in bytes and the number of rows (block rows for compressed formats) of a width x height image. */
void image_layout(uint32_t format, uint32_t width, uint32_t height, uint32_t *pitch, uint32_t *rows)
{
    uint32_t bpp = bytes_per_pixel(format);

    if (bpp == 0) {
        *pitch = ((width + 3) / 4 > 0 ? (width + 3) / 4 : 1) * block_bytes(format);
        *rows = (height + 3) / 4 > 0 ? (height + 3) / 4 : 1;
    } else {
        *pitch = width * bpp;
        *rows = height;
    }
}

uint32_t mip_dimension(uint32_t value, uint32_t level)
{
    uint32_t size = value >> level;

    return size > 0 ? size : 1;
}

uint32_t level_count(uint32_t requested, uint32_t width, uint32_t height, uint32_t depth)
{
    uint32_t count = 1;
    uint32_t largest = width > height ? width : height;

    if (depth > largest) {
        largest = depth;
    }
    if (requested != 0) {
        return requested < k_max_levels ? requested : k_max_levels;
    }
    while ((largest >> count) > 0 && count < k_max_levels) {
        count++;
    }
    return count;
}

void allocate_levels(gl_texture *texture, uint32_t width, uint32_t height, uint32_t depth)
{
    for (uint32_t face = 0; face < texture->faces; face++) {
        for (uint32_t i = 0; i < texture->levels; i++) {
            gl_level &level = texture->level[face * k_max_levels + i];
            uint32_t rows;

            level.width = mip_dimension(width, i);
            level.height = mip_dimension(height, i);
            level.depth = mip_dimension(depth, i);
            image_layout(texture->format, level.width, level.height, &level.pitch, &rows);
            level.slice = level.pitch * rows;
            level.data = static_cast<uint8_t *>(calloc(1, static_cast<size_t>(level.slice) * level.depth + 16));
        }
    }
}

void write_out(d3d_arg out, void *object)
{
    *static_cast<void **>(out.get()) = object;
}

/** Offset in bytes of the top-left of a D3DRECT/RECT inside a level (0 when no rectangle is given). */
uint32_t rect_offset(uint32_t format, uint32_t pitch, const void *rect)
{
    const int32_t *r = static_cast<const int32_t *>(rect);
    uint32_t bpp = bytes_per_pixel(format);

    if (r == nullptr) {
        return 0;
    }
    if (bpp == 0) {
        return static_cast<uint32_t>(r[1] / 4) * pitch + static_cast<uint32_t>(r[0] / 4) * block_bytes(format);
    }
    return static_cast<uint32_t>(r[1]) * pitch + static_cast<uint32_t>(r[0]) * bpp;
}

struct locked_rect {
    int32_t pitch;
    void *bits;
};

struct locked_box {
    int32_t row_pitch;
    int32_t slice_pitch;
    void *bits;
};

gl_surface *make_surface(uint32_t format, uint32_t width, uint32_t height)
{
    gl_surface *surface = new_object<gl_surface>(kind_surface);
    uint32_t rows;

    surface->format = format;
    surface->width = width;
    surface->height = height;
    image_layout(format, width, height, &surface->pitch, &rows);
    surface->data = static_cast<uint8_t *>(calloc(1, static_cast<size_t>(surface->pitch) * rows + 16));
    surface->owns_data = true;
    return surface;
}

void destroy_object(gl_object *object)
{
    switch (object->kind) {
    case kind_texture:
    case kind_volume_texture:
    case kind_cube_texture: {
        gl_texture *texture = static_cast<gl_texture *>(object);

        for (uint32_t i = 0; i < texture->faces * k_max_levels; i++) {
            free(texture->level[i].data);
        }
        break;
    }
    case kind_surface: {
        gl_surface *surface = static_cast<gl_surface *>(object);

        if (surface->owns_data) {
            free(surface->data);
        }
        if (surface->parent != nullptr) {
            gl_device().release(surface->parent);
        }
        break;
    }
    case kind_vertex_buffer:
    case kind_index_buffer:
        free(static_cast<gl_buffer *>(object)->data);
        break;
    case kind_declaration:
        free(static_cast<gl_declaration *>(object)->elements);
        break;
    default:
        break;
    }
    free(object);
}

bool create_context(HWND window)
{
    PIXELFORMATDESCRIPTOR descriptor;
    int format;

    memset(&descriptor, 0, sizeof(descriptor));
    descriptor.nSize = sizeof(descriptor);
    descriptor.nVersion = 1;
    descriptor.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
    descriptor.iPixelType = PFD_TYPE_RGBA;
    descriptor.cColorBits = 32;
    descriptor.cDepthBits = 24;
    descriptor.cStencilBits = 8;
    descriptor.iLayerType = PFD_MAIN_PLANE;

    g_state.window = window;
    g_state.dc = GetDC(window);
    format = ChoosePixelFormat(g_state.dc, &descriptor);
    if (format == 0 || !SetPixelFormat(g_state.dc, format, &descriptor)) {
        halo::shell::standalone_log("gl: no usable pixel format (error %lu)", GetLastError());
        return false;
    }
    g_state.context = wglCreateContext(g_state.dc);
    if (g_state.context == nullptr || !wglMakeCurrent(g_state.dc, g_state.context)) {
        halo::shell::standalone_log("gl: could not create or activate the OpenGL context (error %lu)", GetLastError());
        return false;
    }
    halo::shell::standalone_log("gl: context ready, %s / %s", reinterpret_cast<const char *>(glGetString(GL_VENDOR)),
        reinterpret_cast<const char *>(glGetString(GL_RENDERER)));
    return true;
}

}  // namespace

bool gl_renderer_requested()
{
    static const int requested = []() {
        const char *value = getenv("HALO_RENDERER");

        return value != nullptr && (value[0] == 'g' || value[0] == 'G') ? 1 : 0;
    }();

    return requested != 0;
}

GlDevice &gl_device()
{
    static GlDevice device;

    return device;
}

/* Device, frame, targets */

int32_t GlDevice::reset(d3d_arg present_parameters)
{
    const uint32_t *parameters = static_cast<const uint32_t *>(present_parameters.get());

    if (parameters != nullptr) {
        g_state.width = parameters[0];
        g_state.height = parameters[1];
    }
    return 0;
}

int32_t GlDevice::present(d3d_arg, d3d_arg, d3d_arg, d3d_arg)
{
    if (g_state.dc != nullptr) {
        SwapBuffers(g_state.dc);
    }
    return 0;
}

int32_t GlDevice::create_device(d3d_arg, uint32_t, uint32_t, d3d_arg focus_window, uint32_t, d3d_arg present_parameters, d3d_arg out_device)
{
    const uint32_t *parameters = static_cast<const uint32_t *>(present_parameters.get());
    HWND window = parameters != nullptr && parameters[7] != 0 ? reinterpret_cast<HWND>(static_cast<uintptr_t>(parameters[7]))
                                                              : reinterpret_cast<HWND>(focus_window.get());

    if (parameters != nullptr) {
        g_state.width = parameters[0];
        g_state.height = parameters[1];
    }
    if (g_state.context == nullptr && !create_context(window)) {
        return static_cast<int32_t>(0x8876086c);  // D3DERR_INVALIDCALL
    }
    if (g_state.device_object == nullptr) {
        g_state.device_object = new_object<gl_object>(kind_device);
    }
    g_state.device_object->refs++;
    write_out(out_device, g_state.device_object);
    return 0;
}

int32_t GlDevice::begin_scene()
{
    return 0;
}

int32_t GlDevice::end_scene()
{
    return 0;
}

int32_t GlDevice::clear(uint32_t, d3d_arg, uint32_t flags, uint32_t color, float z, uint32_t stencil)
{
    GLbitfield mask = 0;

    if ((flags & 1) != 0) {
        glClearColor(static_cast<float>((color >> 16) & 0xff) / 255.0f, static_cast<float>((color >> 8) & 0xff) / 255.0f,
            static_cast<float>(color & 0xff) / 255.0f, static_cast<float>(color >> 24) / 255.0f);
        mask |= GL_COLOR_BUFFER_BIT;
    }
    if ((flags & 2) != 0) {
        glDepthMask(GL_TRUE);
        glClearDepth(z);
        mask |= GL_DEPTH_BUFFER_BIT;
    }
    if ((flags & 4) != 0) {
        glClearStencil(static_cast<GLint>(stencil));
        mask |= GL_STENCIL_BUFFER_BIT;
    }
    if (mask != 0) {
        glClear(mask);
    }
    return 0;
}

int32_t GlDevice::set_viewport(d3d_arg viewport)
{
    const uint32_t *v = static_cast<const uint32_t *>(viewport.get());

    if (v != nullptr) {
        glViewport(static_cast<GLint>(v[0]), static_cast<GLint>(g_state.height) - static_cast<GLint>(v[1] + v[3]),
            static_cast<GLsizei>(v[2]), static_cast<GLsizei>(v[3]));
    }
    return 0;
}

int32_t GlDevice::set_render_target(uint32_t, d3d_arg)
{
    return 0;
}

int32_t GlDevice::get_render_target(uint32_t, d3d_arg out_surface)
{
    write_out(out_surface, make_surface(21, g_state.width, g_state.height));
    return 0;
}

int32_t GlDevice::get_display_mode(uint32_t, d3d_arg mode)
{
    uint32_t *out = static_cast<uint32_t *>(mode.get());

    out[0] = g_state.width;
    out[1] = g_state.height;
    out[2] = 60;
    out[3] = 22;  // X8R8G8B8
    return 0;
}

int32_t GlDevice::get_back_buffer(uint32_t, uint32_t, uint32_t, d3d_arg out_surface)
{
    write_out(out_surface, make_surface(21, g_state.width, g_state.height));
    return 0;
}

int32_t GlDevice::stretch_rect(d3d_arg, d3d_arg, d3d_arg, d3d_arg, uint32_t)
{
    return 0;
}

int32_t GlDevice::set_gamma_ramp(uint32_t, uint32_t, d3d_arg)
{
    return 0;
}

/* Resource creation */

int32_t GlDevice::create_texture(uint32_t width, uint32_t height, uint32_t levels, uint32_t, uint32_t format, uint32_t, d3d_arg out_texture, d3d_arg)
{
    gl_texture *texture = new_object<gl_texture>(kind_texture);

    texture->format = format;
    texture->faces = 1;
    texture->levels = level_count(levels, width, height, 1);
    allocate_levels(texture, width, height, 1);
    write_out(out_texture, texture);
    return 0;
}

int32_t GlDevice::create_volume_texture(uint32_t width, uint32_t height, uint32_t depth, uint32_t levels, uint32_t, uint32_t format, uint32_t, d3d_arg out_texture, d3d_arg)
{
    gl_texture *texture = new_object<gl_texture>(kind_volume_texture);

    texture->format = format;
    texture->faces = 1;
    texture->levels = level_count(levels, width, height, depth);
    allocate_levels(texture, width, height, depth);
    write_out(out_texture, texture);
    return 0;
}

int32_t GlDevice::create_cube_texture(uint32_t edge_length, uint32_t levels, uint32_t, uint32_t format, uint32_t, d3d_arg out_texture, d3d_arg)
{
    gl_texture *texture = new_object<gl_texture>(kind_cube_texture);

    texture->format = format;
    texture->faces = 6;
    texture->levels = level_count(levels, edge_length, edge_length, 1);
    allocate_levels(texture, edge_length, edge_length, 1);
    write_out(out_texture, texture);
    return 0;
}

int32_t GlDevice::create_vertex_buffer(uint32_t length, uint32_t, uint32_t fvf, uint32_t, d3d_arg out_buffer, d3d_arg)
{
    gl_buffer *buffer = new_object<gl_buffer>(kind_vertex_buffer);

    buffer->size = length;
    buffer->format = fvf;
    buffer->data = static_cast<uint8_t *>(calloc(1, static_cast<size_t>(length) + 16));
    write_out(out_buffer, buffer);
    return 0;
}

int32_t GlDevice::create_index_buffer(uint32_t length, uint32_t, uint32_t format, uint32_t, d3d_arg out_buffer, d3d_arg)
{
    gl_buffer *buffer = new_object<gl_buffer>(kind_index_buffer);

    buffer->size = length;
    buffer->format = format;
    buffer->data = static_cast<uint8_t *>(calloc(1, static_cast<size_t>(length) + 16));
    write_out(out_buffer, buffer);
    return 0;
}

int32_t GlDevice::create_offscreen_plain_surface(uint32_t width, uint32_t height, uint32_t format, uint32_t, d3d_arg out_surface, d3d_arg)
{
    write_out(out_surface, make_surface(format, width, height));
    return 0;
}

int32_t GlDevice::create_vertex_declaration(d3d_arg elements, d3d_arg out_declaration)
{
    gl_declaration *declaration = new_object<gl_declaration>(kind_declaration);
    const uint8_t *source = static_cast<const uint8_t *>(elements.get());
    uint32_t count = 0;

    while (source != nullptr && *reinterpret_cast<const uint16_t *>(source + count * 8) != 0xff && count < 64) {
        count++;
    }
    declaration->element_count = count;
    declaration->elements = static_cast<uint8_t *>(calloc(count + 1, 8));
    if (source != nullptr) {
        memcpy(declaration->elements, source, (count + 1) * 8);
    }
    write_out(out_declaration, declaration);
    return 0;
}

int32_t GlDevice::create_vertex_shader(d3d_arg, d3d_arg out_shader)
{
    write_out(out_shader, new_object<gl_object>(kind_vertex_shader));
    return 0;
}

int32_t GlDevice::create_pixel_shader(d3d_arg, d3d_arg out_shader)
{
    write_out(out_shader, new_object<gl_object>(kind_pixel_shader));
    return 0;
}

int32_t GlDevice::create_query(uint32_t, d3d_arg out_query)
{
    write_out(out_query, new_object<gl_object>(kind_query));
    return 0;
}

uint32_t GlDevice::release(d3d_arg object)
{
    gl_object *target = static_cast<gl_object *>(object.get());

    if (!is_ours(target)) {
        return d3d9_device().release(object);
    }
    target->refs--;
    if (target->refs > 0) {
        return static_cast<uint32_t>(target->refs);
    }
    if (target == g_state.device_object) {
        g_state.device_object = nullptr;
    }
    destroy_object(target);
    return 0;
}

/* States and draws: accepted and ignored until the fixed-function and shader milestones */

int32_t GlDevice::set_transform(uint32_t, d3d_arg)
{
    return 0;
}

int32_t GlDevice::set_material(d3d_arg)
{
    return 0;
}

int32_t GlDevice::set_light(uint32_t, d3d_arg)
{
    return 0;
}

int32_t GlDevice::light_enable(uint32_t, int32_t)
{
    return 0;
}

int32_t GlDevice::set_render_state(uint32_t, uint32_t)
{
    return 0;
}

int32_t GlDevice::set_texture(uint32_t, d3d_arg)
{
    return 0;
}

int32_t GlDevice::set_texture_stage_state(uint32_t, uint32_t, uint32_t)
{
    return 0;
}

int32_t GlDevice::set_sampler_state(uint32_t, uint32_t, uint32_t)
{
    return 0;
}

int32_t GlDevice::set_software_vertex_processing(uint32_t)
{
    return 0;
}

int32_t GlDevice::draw_primitive(uint32_t, uint32_t, uint32_t)
{
    return 0;
}

int32_t GlDevice::draw_indexed_primitive(uint32_t, int32_t, uint32_t, uint32_t, uint32_t, uint32_t)
{
    return 0;
}

int32_t GlDevice::draw_primitive_up(uint32_t, uint32_t, d3d_arg, uint32_t)
{
    return 0;
}

int32_t GlDevice::draw_indexed_primitive_up(uint32_t, uint32_t, uint32_t, uint32_t, d3d_arg, uint32_t, d3d_arg, uint32_t)
{
    return 0;
}

int32_t GlDevice::set_vertex_declaration(d3d_arg)
{
    return 0;
}

int32_t GlDevice::set_fvf(uint32_t)
{
    return 0;
}

int32_t GlDevice::set_vertex_shader(d3d_arg)
{
    return 0;
}

int32_t GlDevice::set_vertex_shader_constant_f(uint32_t, d3d_arg, uint32_t)
{
    return 0;
}

int32_t GlDevice::set_stream_source(uint32_t, d3d_arg, uint32_t, uint32_t)
{
    return 0;
}

int32_t GlDevice::set_indices(d3d_arg)
{
    return 0;
}

int32_t GlDevice::set_pixel_shader(d3d_arg)
{
    return 0;
}

int32_t GlDevice::set_pixel_shader_constant_f(uint32_t, d3d_arg, uint32_t)
{
    return 0;
}

int32_t GlDevice::process_vertices(uint32_t, uint32_t, uint32_t, d3d_arg, d3d_arg, uint32_t)
{
    return 0;
}

/* Adapter and capability queries: the machine's real Direct3D 9 object answers these */

uint32_t GlDevice::get_adapter_count(d3d_arg object)
{
    return d3d9_device().get_adapter_count(object);
}

int32_t GlDevice::get_adapter_display_mode(d3d_arg object, uint32_t adapter, d3d_arg mode)
{
    return d3d9_device().get_adapter_display_mode(object, adapter, mode);
}

int32_t GlDevice::check_device_format(d3d_arg object, uint32_t adapter, uint32_t device_type, uint32_t adapter_format, uint32_t usage, uint32_t resource_type, uint32_t check_format)
{
    return d3d9_device().check_device_format(object, adapter, device_type, adapter_format, usage, resource_type, check_format);
}

int32_t GlDevice::get_device_caps(d3d_arg object, uint32_t adapter, uint32_t device_type, d3d_arg caps)
{
    return d3d9_device().get_device_caps(object, adapter, device_type, caps);
}

/* Surfaces, buffers, textures: CPU shadow memory */

int32_t GlDevice::surface_get_desc(d3d_arg object, d3d_arg desc)
{
    const gl_surface *surface = static_cast<const gl_surface *>(object.get());
    uint32_t *out = static_cast<uint32_t *>(desc.get());

    memset(out, 0, 32);
    out[0] = surface->format;
    out[6] = surface->width;
    out[7] = surface->height;
    return 0;
}

int32_t GlDevice::surface_lock_rect(d3d_arg object, d3d_arg locked, d3d_arg rect, uint32_t)
{
    gl_surface *surface = static_cast<gl_surface *>(object.get());
    locked_rect *out = static_cast<locked_rect *>(locked.get());

    out->pitch = static_cast<int32_t>(surface->pitch);
    out->bits = surface->data + rect_offset(surface->format, surface->pitch, rect.get());
    return 0;
}

int32_t GlDevice::surface_unlock_rect(d3d_arg)
{
    return 0;
}

int32_t GlDevice::buffer_lock(d3d_arg object, uint32_t offset, uint32_t, d3d_arg out_data, uint32_t)
{
    gl_buffer *buffer = static_cast<gl_buffer *>(object.get());

    write_out(out_data, buffer->data + offset);
    return 0;
}

int32_t GlDevice::buffer_unlock(d3d_arg)
{
    return 0;
}

int32_t GlDevice::buffer_get_desc(d3d_arg object, d3d_arg desc)
{
    const gl_buffer *buffer = static_cast<const gl_buffer *>(object.get());
    uint32_t *out = static_cast<uint32_t *>(desc.get());

    out[4] = buffer->size;
    if (buffer->kind == kind_vertex_buffer) {
        out[5] = buffer->format;
    }
    return 0;
}

int32_t GlDevice::texture_get_surface_level(d3d_arg object, uint32_t level, d3d_arg out_surface)
{
    gl_texture *texture = static_cast<gl_texture *>(object.get());
    gl_surface *surface = new_object<gl_surface>(kind_surface);
    const gl_level &view = texture->level[level < texture->levels ? level : 0];

    surface->format = texture->format;
    surface->width = view.width;
    surface->height = view.height;
    surface->pitch = view.pitch;
    surface->data = view.data;
    surface->owns_data = false;
    surface->parent = texture;
    texture->refs++;
    write_out(out_surface, surface);
    return 0;
}

int32_t GlDevice::texture_lock_rect(d3d_arg object, uint32_t level, d3d_arg locked, d3d_arg rect, uint32_t)
{
    gl_texture *texture = static_cast<gl_texture *>(object.get());
    const gl_level &view = texture->level[level < texture->levels ? level : 0];
    locked_rect *out = static_cast<locked_rect *>(locked.get());

    out->pitch = static_cast<int32_t>(view.pitch);
    out->bits = view.data + rect_offset(texture->format, view.pitch, rect.get());
    return 0;
}

int32_t GlDevice::texture_unlock_rect(d3d_arg, uint32_t)
{
    return 0;
}

int32_t GlDevice::volume_texture_lock_box(d3d_arg object, uint32_t level, d3d_arg locked, d3d_arg, uint32_t)
{
    gl_texture *texture = static_cast<gl_texture *>(object.get());
    const gl_level &view = texture->level[level < texture->levels ? level : 0];
    locked_box *out = static_cast<locked_box *>(locked.get());

    out->row_pitch = static_cast<int32_t>(view.pitch);
    out->slice_pitch = static_cast<int32_t>(view.slice);
    out->bits = view.data;
    return 0;
}

int32_t GlDevice::volume_texture_unlock_box(d3d_arg, uint32_t)
{
    return 0;
}

int32_t GlDevice::cube_texture_lock_rect(d3d_arg object, uint32_t face, uint32_t level, d3d_arg locked, d3d_arg rect, uint32_t)
{
    gl_texture *texture = static_cast<gl_texture *>(object.get());
    const gl_level &view = texture->level[(face < 6 ? face : 0) * k_max_levels + (level < texture->levels ? level : 0)];
    locked_rect *out = static_cast<locked_rect *>(locked.get());

    out->pitch = static_cast<int32_t>(view.pitch);
    out->bits = view.data + rect_offset(texture->format, view.pitch, rect.get());
    return 0;
}

int32_t GlDevice::cube_texture_unlock_rect(d3d_arg, uint32_t, uint32_t)
{
    return 0;
}

int32_t GlDevice::cube_texture_get_surface(d3d_arg object, uint32_t face, uint32_t level, d3d_arg out_surface)
{
    gl_texture *texture = static_cast<gl_texture *>(object.get());
    gl_surface *surface = new_object<gl_surface>(kind_surface);
    const gl_level &view = texture->level[(face < 6 ? face : 0) * k_max_levels + (level < texture->levels ? level : 0)];

    surface->format = texture->format;
    surface->width = view.width;
    surface->height = view.height;
    surface->pitch = view.pitch;
    surface->data = view.data;
    surface->owns_data = false;
    surface->parent = texture;
    texture->refs++;
    write_out(out_surface, surface);
    return 0;
}

/* Queries: report zero visible pixels */

int32_t GlDevice::query_issue(d3d_arg, uint32_t)
{
    return 0;
}

int32_t GlDevice::query_get_data(d3d_arg, d3d_arg data, uint32_t size, uint32_t)
{
    if (data.get() != nullptr && size > 0) {
        memset(data.get(), 0, size);
    }
    return 0;
}

/* Effects: handles are opaque tokens; nothing is applied yet */

namespace {

uint32_t g_next_effect_handle = 0x1000;

}  // namespace

int32_t gl_create_effect(void *out_effect)
{
    *static_cast<void **>(out_effect) = new_object<gl_object>(kind_effect);
    return 0;
}

int32_t GlDevice::effect_set_vector(d3d_arg, d3d_arg, d3d_arg)
{
    return 0;
}

int32_t GlDevice::effect_set_texture(d3d_arg, d3d_arg, d3d_arg)
{
    return 0;
}

int32_t GlDevice::effect_set_technique(d3d_arg, d3d_arg)
{
    return 0;
}

int32_t GlDevice::effect_validate_technique(d3d_arg, d3d_arg)
{
    return 0;
}

int32_t GlDevice::effect_find_next_valid_technique(d3d_arg, d3d_arg technique, d3d_arg out_technique)
{
    *static_cast<uint32_t *>(out_technique.get()) = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(technique.get()));
    return 0;
}

int32_t GlDevice::effect_begin(d3d_arg, d3d_arg out_pass_count, uint32_t)
{
    *static_cast<uint32_t *>(out_pass_count.get()) = 1;
    return 0;
}

int32_t GlDevice::effect_pass(d3d_arg, uint32_t)
{
    return 0;
}

int32_t GlDevice::effect_end(d3d_arg)
{
    return 0;
}

int32_t GlDevice::effect_get_parameter_by_name(d3d_arg, d3d_arg, d3d_arg)
{
    return static_cast<int32_t>(g_next_effect_handle++);
}

int32_t GlDevice::effect_get_technique_by_name(d3d_arg, d3d_arg)
{
    return static_cast<int32_t>(g_next_effect_handle++);
}

int32_t GlDevice::effect_get_technique_by_name_scoped(d3d_arg, d3d_arg, d3d_arg)
{
    return static_cast<int32_t>(g_next_effect_handle++);
}

}  // namespace halo::rasterizer
