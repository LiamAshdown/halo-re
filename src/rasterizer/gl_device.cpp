/**
 * @file src/rasterizer/gl_device.cpp
 * OpenGL implementation of the RenderDevice interface (see docs/OPENGL_PORT.md): resources (CPU shadow memory uploaded to GL
 * on use), device and frame management. Drawing, state and shader handling live in gl_draw.cpp. Adapter and capability
 * queries are forwarded to the real Direct3D 9 object the shell creates.
 */

#include "gl_internal.hpp"
#include "halo/shell/standalone.hpp"
#include "halo/platform/window.hpp"
#include "halo/platform/time.hpp"
#include "halo/platform/system.hpp"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

namespace halo::rasterizer {

using namespace gl;

namespace gl {

device_state g_state;
pipeline_state g_pipe;

}  // namespace gl

namespace {

bool trace_enabled()
{
    static const int enabled = []() {
        const char *value = getenv("HALO_GL_TRACE");

        return value != nullptr && value[0] != '0' ? 1 : 0;
    }();

    return enabled != 0;
}

int g_trace_budget = 60;
uint32_t g_frame_number;
uint32_t g_trace_frame = []() {
    const char *value = getenv("HALO_GL_TRACE_FRAME");

    if (value != nullptr) {
        g_trace_budget = 100000;
    }
    return value != nullptr ? static_cast<uint32_t>(atoi(value)) : 0u;
}();

uint32_t block_bytes(uint32_t format)
{
    return format == k_fourcc_dxt1 ? 8 : 16;
}

}  // namespace

namespace gl {

uint32_t g_programmable_draws;

void note_programmable_draw()
{
    g_programmable_draws++;
}

/** With HALO_GL_PROBE_AUTO set, arms the draw trace for the frame two after the first one that has world-sized programmable work. */
void frame_presented()
{
    static const bool automatic = getenv("HALO_GL_PROBE_AUTO") != nullptr;

    static const uint32_t not_before_ms = []() {
        const char *value = getenv("HALO_GL_PROBE_AFTER");

        return value != nullptr ? static_cast<uint32_t>(atoi(value)) : 38000u;
    }();
    static const uint32_t started = halo::platform::tick_milliseconds();

    if (automatic && g_trace_frame == 0 && g_programmable_draws >= 45 && halo::platform::tick_milliseconds() - started >= not_before_ms) {
        g_trace_frame = g_frame_number + 2;
        g_trace_budget = 100000;
        halo::shell::standalone_log("gl: probe armed for frame %u", g_trace_frame);
    }
    g_programmable_draws = 0;
}

bool trace_probe_frame()
{
    return g_trace_frame != 0 && g_frame_number == g_trace_frame;
}

void trace_draw(const char *name, uint32_t type, uint32_t count)
{
    uint32_t declaration_elements = 0;
    char elements[200] = "";

    if (!trace_enabled() || g_trace_budget <= 0 || (g_trace_frame != 0 && g_frame_number != g_trace_frame)) {
        return;
    }
    g_trace_budget--;
    if (g_pipe.declaration != nullptr) {
        declaration_elements = g_pipe.declaration->element_count;
        for (uint32_t i = 0; i < declaration_elements && i < 8; i++) {
            const uint8_t *e = g_pipe.declaration->elements + i * 8;
            char piece[32];

            snprintf(piece, sizeof(piece), " [s%u o%u t%u u%u]", *reinterpret_cast<const uint16_t *>(e), *reinterpret_cast<const uint16_t *>(e + 2), e[4], e[6]);
            strncat(elements, piece, sizeof(elements) - strlen(elements) - 1);
        }
    }
    halo::shell::standalone_log("gl draw %s type=%u count=%u fvf=%08x decl=%u%s vs=%u ps=%u tex0=%d zen=%u zwr=%u ablend=%u src=%u dst=%u atest=%u cull=%u",
        name, type, count, g_pipe.fvf, declaration_elements, elements, g_pipe.vertex_shader != nullptr ? g_pipe.vertex_shader->id : 0, g_pipe.pixel_shader != nullptr ? g_pipe.pixel_shader->id : 0,
        g_pipe.texture[0] != nullptr, g_pipe.render_state[7], g_pipe.render_state[14], g_pipe.render_state[27], g_pipe.render_state[19],
        g_pipe.render_state[20], g_pipe.render_state[15], g_pipe.render_state[22]);
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

}  // namespace gl

namespace {

template <typename T>
void hold(T *&slot, T *value)
{
    if (value != nullptr) {
        value->refs++;
    }
    if (slot != nullptr) {
        gl_device().release(slot);
    }
    slot = value;
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
    texture->width = width;
    texture->height = height;
    texture->depth = depth;
    texture->dirty = true;
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

gl_surface *make_back_buffer()
{
    gl_surface *surface = make_surface(21, g_state.width, g_state.height);

    surface->back_buffer = true;
    return surface;
}

/** A surface that views one level of a texture (the texture stays alive while the surface does). */
gl_surface *make_level_surface(gl_texture *texture, uint32_t face, uint32_t level)
{
    gl_surface *surface = new_object<gl_surface>(kind_surface);
    const gl_level &view = texture->level[face * k_max_levels + level];

    surface->format = texture->format;
    surface->width = view.width;
    surface->height = view.height;
    surface->pitch = view.pitch;
    surface->data = view.data;
    surface->owns_data = false;
    surface->parent = texture;
    surface->face = face;
    surface->level = level;
    texture->refs++;
    return surface;
}

bool create_context(void *window)
{
    g_state.window = window;
    if (!halo::platform::gl_context_create(window)) {
        halo::shell::standalone_log("gl: could not create or activate the OpenGL context");
        return false;
    }
    g_state.context = window;
    g_state.modern = gl_load_api();
    if (glGetString != nullptr) {
        halo::shell::standalone_log("gl: context ready, %s / %s / %s", reinterpret_cast<const char *>(glGetString(GL_VENDOR)),
            reinterpret_cast<const char *>(glGetString(GL_RENDERER)), reinterpret_cast<const char *>(glGetString(GL_VERSION)));
    }
    if (!g_state.modern) {
        halo::shell::standalone_log("gl: this driver lacks OpenGL 3 entry points; drawing is disabled");
    } else {
        draw_init();
    }
    return true;
}

}  // namespace

namespace gl {

void destroy_object(gl_object *object)
{
    draw_shutdown_object(object);
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
    case kind_effect:
        effect_destroy(object);
        return;
    default:
        break;
    }
    free(object);
}

}  // namespace gl

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

/**
 * HALO_GL_SNAPSHOT=ms[,ms...]: writes the back buffer to gl_snapshot_<ms>.ppm beside the executable at the first present after each time
 * (milliseconds since the first present), for checking frames without a screen capture.
 */
void snapshot_back_buffer()
{
    static const char *times = getenv("HALO_GL_SNAPSHOT");
    static const uint32_t started = halo::platform::tick_milliseconds();
    uint32_t elapsed = halo::platform::tick_milliseconds() - started;
    char name[600];
    char *slash;
    FILE *file;
    uint8_t *pixels;
    uint32_t width;
    uint32_t height;

    if (times == nullptr || *times == '\0' || elapsed < static_cast<uint32_t>(atoi(times)) || g_state.window == nullptr) {
        return;
    }
    halo::platform::gl_drawable_size(g_state.window, &width, &height);
    halo::platform::executable_path(name, 512);
    slash = strrchr(name, '/') > strrchr(name, 92) ? strrchr(name, '/') : strrchr(name, 92);  // 92: backslash
    snprintf(slash != nullptr ? slash + 1 : name, 64, "gl_snapshot_%u.ppm", static_cast<uint32_t>(atoi(times)));
    times = strchr(times, ',') != nullptr ? strchr(times, ',') + 1 : "";
    pixels = static_cast<uint8_t *>(malloc(width * height * 4));
    glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    glReadPixels(0, 0, static_cast<GLsizei>(width), static_cast<GLsizei>(height), GL_RGBA, GL_UNSIGNED_BYTE, pixels);
    file = fopen(name, "wb");
    if (file != nullptr) {
        fprintf(file, "P6\n%u %u\n255\n", width, height);
        for (uint32_t y = height; y-- > 0;) {
            for (uint32_t x = 0; x < width; x++) {
                fwrite(pixels + (y * width + x) * 4, 1, 3, file);
            }
        }
        fclose(file);
        halo::shell::standalone_log("gl: wrote %s", name);
    }
    free(pixels);
}

int32_t GlDevice::present(d3d_arg, d3d_arg, d3d_arg, d3d_arg)
{
    if (g_state.context != nullptr) {
        snapshot_back_buffer();
        halo::platform::gl_swap(g_state.window);
    }
    g_frame_number++;
    frame_presented();
    return 0;
}

int32_t GlDevice::create_device(d3d_arg, uint32_t, uint32_t, d3d_arg focus_window, uint32_t, d3d_arg present_parameters, d3d_arg out_device)
{
    const uint32_t *parameters = static_cast<const uint32_t *>(present_parameters.get());
    void *window = parameters != nullptr && parameters[7] != 0 ? reinterpret_cast<void *>(static_cast<uintptr_t>(parameters[7]))
                                                               : focus_window.get();

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

    if (!g_state.modern) {
        return 0;
    }
    viewport_scissor(true);
    if ((flags & 1) != 0) {
        glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
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
        glStencilMask(0xff);
        glClearStencil(static_cast<GLint>(stencil));
        mask |= GL_STENCIL_BUFFER_BIT;
    }
    if (mask != 0) {
        glClear(mask);
    }
    glDisable(GL_SCISSOR_TEST);
    return 0;
}

int32_t GlDevice::set_viewport(d3d_arg viewport)
{
    const uint32_t *v = static_cast<const uint32_t *>(viewport.get());

    if (v != nullptr) {
        g_pipe.viewport[0] = static_cast<float>(v[0]);
        g_pipe.viewport[1] = static_cast<float>(v[1]);
        g_pipe.viewport[2] = static_cast<float>(v[2]);
        g_pipe.viewport[3] = static_cast<float>(v[3]);
        g_pipe.viewport[4] = reinterpret_cast<const float *>(v)[4];
        g_pipe.viewport[5] = reinterpret_cast<const float *>(v)[5];
    }
    return 0;
}

int32_t GlDevice::set_render_target(uint32_t index, d3d_arg surface)
{
    gl_surface *target = static_cast<gl_surface *>(surface.get());

    if (index != 0) {
        return 0;
    }
    if (target != nullptr && target->back_buffer) {
        target = nullptr;
    }
    hold(g_pipe.render_target, target);
    bind_render_target(g_pipe.render_target);
    return 0;
}

int32_t GlDevice::get_render_target(uint32_t, d3d_arg out_surface)
{
    if (g_pipe.render_target != nullptr) {
        g_pipe.render_target->refs++;
        write_out(out_surface, g_pipe.render_target);
    } else {
        write_out(out_surface, make_back_buffer());
    }
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
    write_out(out_surface, make_back_buffer());
    return 0;
}

int32_t GlDevice::stretch_rect(d3d_arg source, d3d_arg source_rect, d3d_arg dest, d3d_arg dest_rect, uint32_t filter)
{
    stretch_rect_impl(static_cast<gl_surface *>(source.get()), static_cast<const int32_t *>(source_rect.get()),
        static_cast<gl_surface *>(dest.get()), static_cast<const int32_t *>(dest_rect.get()), filter);
    return 0;
}

int32_t GlDevice::set_gamma_ramp(uint32_t, uint32_t, d3d_arg)
{
    return 0;
}

/* Resource creation */

int32_t GlDevice::create_texture(uint32_t width, uint32_t height, uint32_t levels, uint32_t usage, uint32_t format, uint32_t, d3d_arg out_texture, d3d_arg)
{
    gl_texture *texture = new_object<gl_texture>(kind_texture);

    texture->format = format;
    texture->usage = usage;
    texture->faces = 1;
    texture->levels = level_count(levels, width, height, 1);
    allocate_levels(texture, width, height, 1);
    write_out(out_texture, texture);
    return 0;
}

int32_t GlDevice::create_volume_texture(uint32_t width, uint32_t height, uint32_t depth, uint32_t levels, uint32_t usage, uint32_t format, uint32_t, d3d_arg out_texture, d3d_arg)
{
    gl_texture *texture = new_object<gl_texture>(kind_volume_texture);

    texture->usage = usage;
    texture->format = format;
    texture->faces = 1;
    texture->levels = level_count(levels, width, height, depth);
    allocate_levels(texture, width, height, depth);
    write_out(out_texture, texture);
    return 0;
}

int32_t GlDevice::create_cube_texture(uint32_t edge_length, uint32_t levels, uint32_t usage, uint32_t format, uint32_t, d3d_arg out_texture, d3d_arg)
{
    gl_texture *texture = new_object<gl_texture>(kind_cube_texture);

    texture->usage = usage;
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
    buffer->dirty = true;
    write_out(out_buffer, buffer);
    return 0;
}

int32_t GlDevice::create_index_buffer(uint32_t length, uint32_t, uint32_t format, uint32_t, d3d_arg out_buffer, d3d_arg)
{
    gl_buffer *buffer = new_object<gl_buffer>(kind_index_buffer);

    buffer->size = length;
    buffer->format = format;
    buffer->data = static_cast<uint8_t *>(calloc(1, static_cast<size_t>(length) + 16));
    buffer->dirty = true;
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

int32_t GlDevice::create_vertex_shader(d3d_arg function, d3d_arg out_shader)
{
    gl_shader *shader = new_object<gl_shader>(kind_vertex_shader);

    create_shader(shader, function.get(), false);
    write_out(out_shader, shader);
    return 0;
}

int32_t GlDevice::create_pixel_shader(d3d_arg function, d3d_arg out_shader)
{
    gl_shader *shader = new_object<gl_shader>(kind_pixel_shader);

    create_shader(shader, function.get(), true);
    write_out(out_shader, shader);
    return 0;
}

int32_t GlDevice::create_query(uint32_t, d3d_arg out_query)
{
    gl_query *query = new_object<gl_query>(kind_query);

    if (g_state.modern) {
        glGenQueries(1, &query->name);
    }
    write_out(out_query, query);
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

int32_t GlDevice::set_transform(uint32_t state, d3d_arg matrix)
{
    if (state < 512 && matrix.get() != nullptr) {
        memcpy(g_pipe.transform[state], matrix.get(), 16 * sizeof(float));
    }
    return 0;
}

int32_t GlDevice::set_material(d3d_arg material)
{
    if (material.get() != nullptr) {
        memcpy(g_pipe.material, material.get(), sizeof(g_pipe.material));
    }
    return 0;
}

int32_t GlDevice::set_light(uint32_t index, d3d_arg light)
{
    if (index < 8 && light.get() != nullptr) {
        memcpy(g_pipe.light[index], light.get(), sizeof(g_pipe.light[index]));
    }
    return 0;
}

int32_t GlDevice::light_enable(uint32_t index, int32_t enable)
{
    if (index < 8) {
        g_pipe.light_on[index] = enable != 0;
    }
    return 0;
}

int32_t GlDevice::set_render_state(uint32_t state, uint32_t value)
{
    if (state < 256) {
        g_pipe.render_state[state] = value;
    }
    return 0;
}

int32_t GlDevice::set_texture(uint32_t stage, d3d_arg texture)
{
    if (stage < 16) {
        hold(g_pipe.texture[stage], static_cast<gl_object *>(texture.get()));
    }
    return 0;
}

int32_t GlDevice::set_texture_stage_state(uint32_t stage, uint32_t type, uint32_t value)
{
    if (stage < 8 && type < 40) {
        g_pipe.texture_stage_state[stage][type] = value;
    }
    return 0;
}

int32_t GlDevice::set_sampler_state(uint32_t sampler, uint32_t type, uint32_t value)
{
    if (sampler < 16 && type < 16) {
        g_pipe.sampler_state[sampler][type] = value;
    }
    return 0;
}

int32_t GlDevice::set_software_vertex_processing(uint32_t)
{
    return 0;
}

int32_t GlDevice::draw_primitive(uint32_t type, uint32_t start_vertex, uint32_t primitive_count)
{
    trace_draw("prim", type, primitive_count);
    draw_geometry(type, 0, 0, 0, primitive_count, start_vertex, nullptr, 0, nullptr, 0, false, false);
    return 0;
}

int32_t GlDevice::draw_indexed_primitive(uint32_t type, int32_t base_vertex, uint32_t, uint32_t vertex_count, uint32_t start_index, uint32_t primitive_count)
{
    trace_draw("indexed", type, primitive_count);
    draw_geometry(type, base_vertex, vertex_count, start_index, primitive_count, 0, nullptr, 0, nullptr, 0, true, false);
    return 0;
}

int32_t GlDevice::draw_primitive_up(uint32_t type, uint32_t primitive_count, d3d_arg data, uint32_t stride)
{
    trace_draw("prim_up", type, primitive_count);
    draw_geometry(type, 0, 0, 0, primitive_count, 0, data.get(), stride, nullptr, 0, false, true);
    return 0;
}

int32_t GlDevice::draw_indexed_primitive_up(uint32_t type, uint32_t min_index, uint32_t vertex_count, uint32_t primitive_count, d3d_arg index_data, uint32_t index_format, d3d_arg vertex_data, uint32_t stride)
{
    trace_draw("indexed_up", type, primitive_count);
    draw_geometry(type, 0, min_index + vertex_count, 0, primitive_count, 0, vertex_data.get(), stride, index_data.get(), index_format, true, true);
    return 0;
}

int32_t GlDevice::set_vertex_declaration(d3d_arg declaration)
{
    hold(g_pipe.declaration, static_cast<gl_declaration *>(declaration.get()));
    return 0;
}

int32_t GlDevice::set_fvf(uint32_t fvf)
{
    g_pipe.fvf = fvf;
    hold(g_pipe.declaration, static_cast<gl_declaration *>(nullptr));
    return 0;
}

int32_t GlDevice::set_vertex_shader(d3d_arg shader)
{
    hold(g_pipe.vertex_shader, static_cast<gl_shader *>(shader.get()));
    return 0;
}

int32_t GlDevice::set_vertex_shader_constant_f(uint32_t start_register, d3d_arg data, uint32_t count)
{
    if (start_register + count <= 256 && data.get() != nullptr) {
        memcpy(g_pipe.vertex_constants[start_register], data.get(), count * 4 * sizeof(float));
    }
    return 0;
}

int32_t GlDevice::set_stream_source(uint32_t stream, d3d_arg buffer, uint32_t offset, uint32_t stride)
{
    if (stream < 4) {
        hold(g_pipe.stream[stream], static_cast<gl_buffer *>(buffer.get()));
        g_pipe.stream_offset[stream] = offset;
        g_pipe.stream_stride[stream] = stride;
    }
    return 0;
}

int32_t GlDevice::set_indices(d3d_arg index_buffer)
{
    hold(g_pipe.indices, static_cast<gl_buffer *>(index_buffer.get()));
    return 0;
}

int32_t GlDevice::set_pixel_shader(d3d_arg shader)
{
    hold(g_pipe.pixel_shader, static_cast<gl_shader *>(shader.get()));
    return 0;
}

int32_t GlDevice::set_pixel_shader_constant_f(uint32_t start_register, d3d_arg data, uint32_t count)
{
    if (start_register + count <= 224 && data.get() != nullptr) {
        memcpy(g_pipe.pixel_constants[start_register], data.get(), count * 4 * sizeof(float));
    }
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

int32_t GlDevice::surface_lock_rect(d3d_arg object, d3d_arg locked, d3d_arg rect, uint32_t flags)
{
    gl_surface *surface = static_cast<gl_surface *>(object.get());
    locked_rect *out = static_cast<locked_rect *>(locked.get());

    read_back_surface(surface);
    if (surface->parent != nullptr && (flags & 0x10) == 0) {
        surface->parent->dirty = true;
    }
    out->pitch = static_cast<int32_t>(surface->pitch);
    out->bits = surface->data + rect_offset(surface->format, surface->pitch, rect.get());
    return 0;
}

int32_t GlDevice::surface_unlock_rect(d3d_arg)
{
    return 0;
}

int32_t GlDevice::buffer_lock(d3d_arg object, uint32_t offset, uint32_t, d3d_arg out_data, uint32_t flags)
{
    gl_buffer *buffer = static_cast<gl_buffer *>(object.get());

    if ((flags & 0x10) == 0) {
        buffer->dirty = true;
    }
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

    write_out(out_surface, make_level_surface(texture, 0, level < texture->levels ? level : 0));
    return 0;
}

int32_t GlDevice::texture_lock_rect(d3d_arg object, uint32_t level, d3d_arg locked, d3d_arg rect, uint32_t flags)
{
    gl_texture *texture = static_cast<gl_texture *>(object.get());
    const gl_level &view = texture->level[level < texture->levels ? level : 0];
    locked_rect *out = static_cast<locked_rect *>(locked.get());

    if ((flags & 0x10) == 0) {
        texture->dirty = true;
    }
    out->pitch = static_cast<int32_t>(view.pitch);
    out->bits = view.data + rect_offset(texture->format, view.pitch, rect.get());
    return 0;
}

int32_t GlDevice::texture_unlock_rect(d3d_arg, uint32_t)
{
    return 0;
}

int32_t GlDevice::volume_texture_lock_box(d3d_arg object, uint32_t level, d3d_arg locked, d3d_arg, uint32_t flags)
{
    gl_texture *texture = static_cast<gl_texture *>(object.get());
    const gl_level &view = texture->level[level < texture->levels ? level : 0];
    locked_box *out = static_cast<locked_box *>(locked.get());

    if ((flags & 0x10) == 0) {
        texture->dirty = true;
    }
    out->row_pitch = static_cast<int32_t>(view.pitch);
    out->slice_pitch = static_cast<int32_t>(view.slice);
    out->bits = view.data;
    return 0;
}

int32_t GlDevice::volume_texture_unlock_box(d3d_arg, uint32_t)
{
    return 0;
}

int32_t GlDevice::cube_texture_lock_rect(d3d_arg object, uint32_t face, uint32_t level, d3d_arg locked, d3d_arg rect, uint32_t flags)
{
    gl_texture *texture = static_cast<gl_texture *>(object.get());
    const gl_level &view = texture->level[(face < 6 ? face : 0) * k_max_levels + (level < texture->levels ? level : 0)];
    locked_rect *out = static_cast<locked_rect *>(locked.get());

    if ((flags & 0x10) == 0) {
        texture->dirty = true;
    }
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

    write_out(out_surface, make_level_surface(texture, face < 6 ? face : 0, level < texture->levels ? level : 0));
    return 0;
}

/* Queries: report zero visible pixels */

int32_t GlDevice::query_issue(d3d_arg object, uint32_t flags)
{
    gl_query *query = static_cast<gl_query *>(object.get());

    if (!g_state.modern || query->name == 0) {
        return 0;
    }
    if ((flags & 1) != 0) {
        glBeginQuery(GL_SAMPLES_PASSED, query->name);
    }
    if ((flags & 2) != 0) {
        glEndQuery(GL_SAMPLES_PASSED);
        query->issued = true;
    }
    return 0;
}

int32_t GlDevice::query_get_data(d3d_arg object, d3d_arg data, uint32_t size, uint32_t flags)
{
    gl_query *query = static_cast<gl_query *>(object.get());
    GLuint available = 1;
    GLuint samples = 0;

    if (g_state.modern && query->name != 0 && query->issued) {
        glGetQueryObjectuiv(query->name, GL_QUERY_RESULT_AVAILABLE, &available);
        if (available == 0 && (flags & 1) == 0) {
            return 1;  // S_FALSE: not ready yet
        }
        glGetQueryObjectuiv(query->name, GL_QUERY_RESULT, &samples);
    }
    if (data.get() != nullptr && size >= 4) {
        memset(data.get(), 0, size);
        *static_cast<uint32_t *>(data.get()) = samples;
    }
    return 0;
}

}  // namespace halo::rasterizer
