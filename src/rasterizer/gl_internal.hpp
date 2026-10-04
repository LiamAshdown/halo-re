#pragma once

/**
 * @file src/rasterizer/gl_internal.hpp
 * Objects and shared state of the OpenGL backend (gl_device.cpp, gl_draw.cpp). The engine only ever sees these as the
 * opaque COM-style pointers Direct3D 9 handed out; the first dword is a magic value that tells them from real COM objects.
 */

#include "halo/rasterizer/gl_api.hpp"
#include "halo/rasterizer/gl_device.hpp"

#include <mojoshader.h>

namespace halo::rasterizer::gl {

constexpr uint32_t k_magic = 0x424f4c47;  // "GLOB"
constexpr uint32_t k_max_levels = 16;

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

/* Direct3D 9 values used by the backend */
constexpr uint32_t k_usage_render_target = 0x1;
constexpr uint32_t k_usage_depth_stencil = 0x2;
constexpr uint32_t k_fourcc_dxt1 = 0x31545844;
constexpr uint32_t k_fourcc_dxt2 = 0x32545844;
constexpr uint32_t k_fourcc_dxt3 = 0x33545844;
constexpr uint32_t k_fourcc_dxt4 = 0x34545844;
constexpr uint32_t k_fourcc_dxt5 = 0x35545844;

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

struct gl_texture : gl_object {
    uint32_t format;
    uint32_t usage;
    uint32_t levels;
    uint32_t faces;
    uint32_t width, height, depth;
    GLuint name;         // GL texture object, 0 until first used
    GLuint framebuffer;  // framebuffer with level 0 attached (render-target textures)
    GLuint depth_buffer; // depth/stencil renderbuffer shared by the framebuffer
    bool dirty;          // CPU copy changed since the last upload
    uint32_t sampler_hash;  // sampler state last applied to the GL object
    gl_level level[6 * k_max_levels];
};

struct gl_surface : gl_object {
    uint32_t format;
    uint32_t width, height;
    uint32_t pitch;
    uint8_t *data;
    bool owns_data;
    bool back_buffer;    // the window's default framebuffer
    gl_texture *parent;  // texture whose level this surface views (kept alive), or null
    uint32_t face;
    uint32_t level;
};

struct gl_buffer : gl_object {
    uint32_t size;
    uint32_t format;  // FVF for a vertex buffer, index format for an index buffer
    uint8_t *data;
    GLuint name;
    bool dirty;
};

struct gl_declaration : gl_object {
    uint32_t element_count;
    uint8_t *elements;  // 8 bytes per D3DVERTEXELEMENT9, end marker included
};

struct gl_shader : gl_object {
    const MOJOSHADER_parseData *parse;  // null if the bytecode could not be translated
    GLuint compiled;                    // GL shader object, 0 until a program needs it
    uint32_t id;
};

struct gl_query : gl_object {
    GLuint name;
    bool issued;
};

struct device_state {
    HWND window;
    HDC dc;
    HGLRC context;
    uint32_t width;
    uint32_t height;
    gl_object *device_object;
    bool modern;  // entry points beyond GL 1.1 loaded
};

/** Everything the draw calls depend on, tracked as the engine sets it. */
struct pipeline_state {
    uint32_t render_state[256];
    uint32_t texture_stage_state[8][40];
    uint32_t sampler_state[16][16];
    gl_object *texture[16];
    gl_declaration *declaration;
    uint32_t fvf;
    gl_shader *vertex_shader;
    gl_shader *pixel_shader;
    gl_buffer *stream[4];
    uint32_t stream_offset[4];
    uint32_t stream_stride[4];
    gl_buffer *indices;
    float vertex_constants[256][4];
    float pixel_constants[224][4];
    float transform[512][16];  // indexed by D3DTRANSFORMSTATETYPE (view 2, projection 3, texture 16.., world 256)
    float light[8][26];        // D3DLIGHT9 as dwords: type, diffuse, specular, ambient, position, direction, range, falloff, attenuation 0-2, theta, phi
    bool light_on[8];
    float material[17];        // D3DMATERIAL9: diffuse, ambient, specular, emissive, power
    float viewport[6];         // x, y, width, height, min z, max z
    gl_surface *render_target; // null = back buffer
};

extern MOJOSHADER_parseData g_placeholder_parse;  // stands in for effect shader variables that carry no bytecode
extern device_state g_state;
extern pipeline_state g_pipe;

template <typename T>
T *new_object(uint32_t object_kind)
{
    T *object = static_cast<T *>(calloc(1, sizeof(T)));

    object->magic = k_magic;
    object->kind = object_kind;
    object->refs = 1;
    return object;
}

inline bool is_ours(const void *object)
{
    return object != nullptr && static_cast<const gl_object *>(object)->magic == k_magic;
}

/* gl_device.cpp */
uint32_t bytes_per_pixel(uint32_t format);
void image_layout(uint32_t format, uint32_t width, uint32_t height, uint32_t *pitch, uint32_t *rows);
void destroy_object(gl_object *object);
bool trace_probe_frame();
void note_programmable_draw();
void frame_presented();
void trace_draw(const char *name, uint32_t type, uint32_t count);

/* gl_draw.cpp */
void draw_init();
void draw_shutdown_object(gl_object *object);
GLuint framebuffer_for_target(gl_surface *target, uint32_t *width, uint32_t *height);
void bind_render_target(gl_surface *target);
void read_back_surface(gl_surface *surface);
void draw_geometry(uint32_t type, int32_t base_vertex, uint32_t vertex_count, uint32_t start_index, uint32_t primitive_count, uint32_t start_vertex,
    const void *vertex_data, uint32_t vertex_stride, const void *index_data, uint32_t index_format, bool indexed, bool user_data);
void create_shader(gl_shader *shader, const void *function, bool pixel);
void effect_destroy(gl_object *object);
void viewport_scissor(bool enable);
void stretch_rect_impl(gl_surface *source, const int32_t *source_rect, gl_surface *dest, const int32_t *dest_rect, uint32_t filter);

}  // namespace halo::rasterizer::gl
