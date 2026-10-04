/**
 * @file src/rasterizer/resources.cpp
 * Vertex/index buffers, vertex declarations, shaders, effects and render targets.
 */

#include "halo/render/d3d9.hpp"
#include "halo/core/datum.hpp"
#include "halo/rasterizer/globals.hpp"
#include "internal/state.hpp"
#include "halo/cseries/api.hpp"
#include "halo/shell/api.hpp"
#include "halo/rasterizer/api.hpp"
#include "halo/rasterizer/d3dx.hpp"
#include "halo/core/win32_constants.hpp"
#include "halo/rasterizer/constants.hpp"
#include <cstring>

namespace {

/** Size of the miscellaneous vertex buffer and of the prefix of it that is cleared on creation. */
constexpr uint32_t k_misc_vertex_buffer_bytes = 0x10000;
constexpr uint32_t k_misc_vertex_buffer_cleared_bytes = 0x2000;

/** Size of the MD5 signature (32 hex digits and the terminator) at the end of a shader resource file. */
constexpr uint32_t k_resource_signature_size = 0x21;

/** Key the shader resource files are encrypted with. */
constexpr uint32_t k_resource_tea_key[4] = { 0x3fffffdd, 0x7fc3, 0xe5, 0x3fffef };

int32_t read_chunk_size(const uint8_t *cursor)
{
    int32_t size;

    std::memcpy(&size, cursor, sizeof(size));
    return size;
}

}  // namespace




namespace halo::rasterizer {


/**
 * Creates one Direct3D vertex buffer (register EAX selects the vertex format, whose declaration usage bits are
 * folded into the buffer's own Usage flags) from the given length/FVF/dynamic request, returning the created
 * buffer or NULL on failure. Note: `not_dynamic` sets D3DUSAGE_DYNAMIC (0x200) when it is ZERO, i.e. the raw
 * bool this takes reads as "static"/"not dynamic" rather than "dynamic"; preserved as the code computes it.
 *
 * @address 0x530570
 */
void * rasterizer_dx9_create_vertex_buffer(int32_t vertex_type, uint32_t length, uint32_t fvf, uint8_t not_dynamic)
{
    uint32_t usage;
    uint32_t sw_flag;
    uint32_t dynamic_flag;
    uint32_t pool;
    void *buffer;
    int32_t hr;

    sw_flag = (rasterizer_software_vertex_processing != 0) ? halo::d3d9::k_usage_software_processing : 0u;
    dynamic_flag = (not_dynamic == 0) ? halo::d3d9::k_usage_dynamic : 0u;
    usage = sw_flag | rasterizer_vertex_declarations[vertex_type].usage | dynamic_flag;

    pool = (sw_flag != 0 || (rasterizer_vertex_declarations[vertex_type].usage & halo::d3d9::k_usage_software_processing) != 0 ||
           (rasterizer_vertex_declarations[vertex_type].usage & halo::d3d9::k_usage_dynamic) != 0 || dynamic_flag != 0) ? halo::d3d9::k_pool_system_memory : halo::d3d9::k_pool_managed;

    buffer = 0;
    hr = render_device().create_vertex_buffer(length, usage, fvf, pool, &buffer, 0);
    return (hr >= 0) ? buffer : 0;
}

/**
 * Recreates vertex declarations, reloads vertex shaders, reloads pixel shaders/effects, then initializes the
 * screen-effect, screen-flash and shader_environment technique tables in order, stopping at the first failure.
 *
 * @address 0x5300d0
 */
uint8_t rasterizer_dx9_effects_initialize(void)
{
    rasterizer_dx9_vertex_declarations_release();
    if (!rasterizer_dx9_vertex_declarations_create()) {
        return 0;
    }
    if (!rasterizer_dx9_vertex_shaders_reload()) {
        return 0;
    }
    rasterizer_dx9_pixel_shaders_release();
    if ((rasterizer_dx9_shaders_initialize() & 0xff) == 0) {
        return 0;
    }
    if (!rasterizer_screen_effect_init_shaders()) {
        return 0;
    }
    if (!rasterizer_screen_flash_init_shaders()) {
        return 0;
    }
    if (!rasterizer_shader_environment_build_technique_table()) {
        return 0;
    }
    return 1;
}

/**
 * Compiles one pixel-shader effect chunk into rasterizer_effects[effect_index].effect. On failure, records
 * "shaders\fx.bin" as the failing file name and raises a fatal error dialog. Any returned compilation-error
 * buffer is released either way.
 *
 * @address 0x52f980
 */
int32_t rasterizer_dx9_pixel_shader_effect_load(int32_t effect_index, const void *data, uint32_t size)
{
    int32_t hr;
    void *error_buffer;

    rasterizer_effects[effect_index].effect = 0;
    error_buffer = 0;
    hr = halo::rasterizer::d3dx::create_effect(rasterizer_device, data, size, rasterizer_effect_defines, 0, 0,
                          rasterizer_effect_pool, &rasterizer_effects[effect_index].effect, &error_buffer);
    if (hr < 0) {
        rasterizer_shader_file_name = "shaders\\fx.bin";
        halo::shell::shell_display_fatal_error_dialog(0x69, 0x7e, 1);
    }
    if (error_buffer != 0) {
        render_device().release(error_buffer);
    }
    return hr >= 0;
}

/**
 * Loads shaders\fx.bin, then walks it as a run of [int32 chunk_size][chunk_size bytes] records, compiling and
 * initializing one rasterizer_effects[] slot per chunk. If the file is malformed (a chunk runs past the end of
 * the buffer) or any chunk fails to load or initialize, every effect slot filled so far is released and
 * cleared and the whole load is reported as a failure.
 *
 * @address 0x52fa00
 */
uint8_t rasterizer_dx9_pixel_shaders_load_all(void)
{
    void *buffer;
    uint32_t size;
    uint8_t *cursor;
    uint8_t *end;
    int32_t index;

    if (rasterizer_load_file_and_verify(&buffer, &size, "shaders\\fx.bin") == 0) {
        return 0;
    }

    cursor = static_cast<uint8_t *>(buffer);
    end = cursor + size;
    for (index = 0; index < k_rasterizer_pixel_shader_effects; ) {
        uint8_t *data;
        int32_t chunk_size;

        data = cursor + 4;
        if (end < data) {
            break;
        }
        chunk_size = read_chunk_size(cursor);
        cursor = data + chunk_size;
        if (end < cursor || !rasterizer_dx9_pixel_shader_effect_load(index, data, chunk_size) ||
            !rasterizer_dx9_shaders_init_effect(index)) {
            break;
        }
        index++;
    }

    if (index < k_rasterizer_pixel_shader_effects) {
        int i;
        for (i = 0; i < k_rasterizer_pixel_shader_effects; i++) {
            void *effect = rasterizer_effects[i].effect;
            if (effect != 0) {
                render_device().release(effect);
                rasterizer_effects[i].effect = 0;
            }
        }
    }

    GlobalFree(buffer);
    return index == k_rasterizer_pixel_shader_effects;
}

static void free_constant_handles(int first, int last)
{
    int i;
    for (i = first; i <= last; i++) {
        if (rasterizer_effects[i].constant_handles != 0) {
            GlobalFree(rasterizer_effects[i].constant_handles);
            rasterizer_effects[i].constant_handles = 0;
        }
    }
}

/**
 * Frees every shader-constant handle table allocated by rasterizer_dx9_shaders_initialize and releases every
 * pixel-shader effect interface and the effect-pool object it created.
 *
 * @address 0x52ff60
 */
void rasterizer_dx9_pixel_shaders_release(void)
{
    int i;

    free_constant_handles(116, 121);
    free_constant_handles(32, 34);
    free_constant_handles(37, 39);
    free_constant_handles(106, 106);
    free_constant_handles(107, 107);
    free_constant_handles(108, 108);
    free_constant_handles(0, 3);
    free_constant_handles(114, 114);
    free_constant_handles(40, 43);

    for (i = 0; i < k_rasterizer_pixel_shader_effects; i++) {
        void *effect = rasterizer_effects[i].effect;
        if (effect != 0) {
            render_device().release(effect);
            rasterizer_effects[i].effect = 0;
        }
    }
    if (rasterizer_effect_pool != 0) {
        render_device().release(rasterizer_effect_pool);
        rasterizer_effect_pool = 0;
    }
}

namespace rasterizer_dx9_shaders_init_effect_impl {





/**
 * Finds and validates the best pixel-shader technique named "ps_<major>_<minor>" supported by the current
 * pixel-shader-model version (degrading the minor, then major, version number until one validates), falls back
 * to "TDefault_ps"/"TDefault_no_ps" (or "fallback" when forced) if none do, activates it with SetTechnique,
 * then caches the effect's Texture0..Texture3 parameter handles.
 *
 * @address 0x52f780
 */
int32_t rasterizer_dx9_shaders_init_effect(int32_t effect_index)
{
    void *effect;
    char name[64];
    void *technique;
    int32_t hr;
    int32_t major, minor;
    int32_t found;
    int i;
    static const char *texture_param_names[4] = { "Texture0", "Texture1", "Texture2", "Texture3" };

    effect = rasterizer_effects[effect_index].effect;
    technique = 0;
    found = 0;

    if (halo::shell::globals().safe_mode == 0 && halo::shell::globals().force_shader != halo::rasterizer::k_force_shader_fallback) {
        major = (rasterizer_caps.pixel_shader_version >> 8) & 0xff;
        minor = rasterizer_caps.pixel_shader_version & 0xff;
        for (; !found && major >= 0; major--, minor = 9) {
            for (; !found && minor >= 0; minor--) {
                sprintf(name, "ps_%d_%d", major, minor);
                technique = d3d_handle(render_device().effect_get_technique_by_name_scoped(effect, 0, name));
                if (technique != 0) {
                    hr = render_device().effect_validate_technique(effect, technique);
                    found = hr >= 0;
                }
            }
        }
        if (found) {

            hr = render_device().effect_set_technique(effect, technique);
        } else {

            const char *default_name = (rasterizer_caps.pixel_shader_version < halo::d3d9::k_pixel_shader_version_1_1)
                                            ? "TDefault_no_ps" : "TDefault_ps";
            sprintf(name, default_name);
            technique = d3d_handle(render_device().effect_get_technique_by_name_scoped(effect, 0, name));
            if (technique == 0) {
                void *next_technique;
                hr = render_device().effect_find_next_valid_technique(effect, 0, &next_technique);
                if (hr < 0) {
                    return 0;
                }
            }
            hr = render_device().effect_validate_technique(effect, technique);
        }
        if (hr < 0) {
            return 0;
        }
        found = 1;
    } else {
        sprintf(name, "fallback");
        technique = d3d_handle(render_device().effect_get_technique_by_name_scoped(effect, 0, name));
        found = technique != 0;
        if (!found) {
            return 0;
        }
    }

    for (i = 0; i < 4; i++) {
        rasterizer_effects[effect_index].texture_handles[i] =
            d3d_handle(render_device().effect_get_parameter_by_name(effect, 0, texture_param_names[i]));
    }
    return found;
}

}  // namespace rasterizer_dx9_shaders_init_effect_impl

/**
 * Releases vertex declarations, every created vertex-shader interface, then every pixel-shader effect and the
 * effect pool (via rasterizer_dx9_pixel_shaders_release).
 *
 * @address 0x530090
 */
void rasterizer_dx9_shaders_release_all(void)
{
    int i;

    rasterizer_dx9_vertex_declarations_release();
    for (i = 0; i < k_rasterizer_vertex_shaders; i++) {
        void *shader = rasterizer_vertex_shaders[i].shader;
        if (shader != 0) {
            render_device().release(shader);
            rasterizer_vertex_shaders[i].shader = 0;
        }
    }
    rasterizer_dx9_pixel_shaders_release();
}

/**
 * Releases every vertex declaration created by rasterizer_dx9_vertex_declarations_create and clears the table.
 *
 * @address 0x530540
 */
void rasterizer_dx9_vertex_declarations_release(void)
{
    int i;

    for (i = 0; i < k_rasterizer_vertex_type_count; i++) {
        void *declaration = rasterizer_vertex_declarations[i].declaration;
        if (declaration != 0) {
            render_device().release(declaration);
        }
    }
    memset(rasterizer_vertex_declarations, 0, sizeof(rasterizer_vertex_declarations));
}

/**
 * Clears the vertex-shader table, loads and creates all precompiled vertex shaders, and raises a fatal error
 * dialog referencing shaders\vsh.bin if that fails.
 *
 * @address 0x5307b0
 */
uint8_t rasterizer_dx9_vertex_shaders_initialize(void)
{
    uint32_t ok;
    int32_t i;

    for (i = 0; i < k_rasterizer_vertex_shaders; i++) {
        rasterizer_vertex_shaders[i].shader = 0;
    }

    ok = rasterizer_dx9_vertex_shaders_load_all();
    if ((ok & 0xff) == 0) {
        rasterizer_shader_file_name = "shaders\\vsh.bin";
        halo::shell::shell_display_fatal_error_dialog(0x89, 0x7e, 1);
    }
    return (uint8_t)ok;
}

namespace rasterizer_dx9_vertex_shaders_load_all_impl {


/**
 * Loads shaders\vsh.bin, then walks it as a run of [int32 chunk_size][chunk_size bytes] records, creating one
 * vertex shader per rasterizer_vertex_shaders[] slot whose `enabled` field is nonzero (disabled slots consume
 * no chunk). Tears every created shader down again if the file is malformed or any chunk fails to create.
 *
 * @address 0x5306e0
 */
uint32_t rasterizer_dx9_vertex_shaders_load_all(void)
{
    void *buffer;
    uint32_t size;
    uint8_t *cursor, *end;
    int32_t index;

    if (rasterizer_load_file_and_verify(&buffer, &size, "shaders\\vsh.bin") == 0) {
        return 0;
    }

    cursor = static_cast<uint8_t *>(buffer);
    end = cursor + size;

    for (index = 0; index < k_rasterizer_vertex_shaders; index++) {
        if (rasterizer_vertex_shaders[index].enabled == 0) {
            continue;
        }
        {
            uint8_t *data = cursor + 4;
            int32_t chunk_size;
            int32_t hr;
            if (end < data) {
                break;
            }
            chunk_size = read_chunk_size(cursor);
            cursor = data + chunk_size;
            if (end < cursor) {
                break;
            }
            hr = render_device().create_vertex_shader(data, &rasterizer_vertex_shaders[index].shader);
            if (hr < 0) {
                break;
            }
        }
    }

    if (index < k_rasterizer_vertex_shaders) {
        int i;
        for (i = 0; i < k_rasterizer_vertex_shaders; i++) {
            void *shader = rasterizer_vertex_shaders[i].shader;
            if (shader != 0) {
                render_device().release(shader);
                rasterizer_vertex_shaders[i].shader = 0;
            }
        }
        rasterizer_shader_file_name = "shaders\\vsh.bin";
        halo::shell::shell_display_fatal_error_dialog(0x69, 0x7e, 1);
    }

    GlobalFree(buffer);
    return index == k_rasterizer_vertex_shaders;
}

}  // namespace rasterizer_dx9_vertex_shaders_load_all_impl

/**
 * Releases every created vertex shader, then reloads and recreates them all (used after a device reset).
 * Returns whatever rasterizer_dx9_vertex_shaders_initialize returns.
 *
 * @address 0x530800
 */
uint8_t rasterizer_dx9_vertex_shaders_reload(void)
{
    int i;

    for (i = 0; i < k_rasterizer_vertex_shaders; i++) {
        void *shader = rasterizer_vertex_shaders[i].shader;
        if (shader != 0) {
            render_device().release(shader);
            rasterizer_vertex_shaders[i].shader = 0;
        }
    }
    return rasterizer_dx9_vertex_shaders_initialize();
}

/**
 * Returns the capture/blit render-target surface for the bitmap type (cube map faces and 2D maps use different surfaces), or `fallback` for any other type.
 *
 * Registers: EAX -> object, ECX -> fallback
 *
 * @address 0x515c30
 */
void * rasterizer_get_capture_surface(BitmapData *bitmap, void *fallback)
{
    int16_t mode = bitmap->type;
    switch (mode) {
    case 0:
    case 1:
        return rasterizer_capture_surfaces[1];
    case 2:
        return rasterizer_capture_surfaces[3];
    case 3:
        return rasterizer_capture_surfaces[0];
    default:
        return fallback;
    }
}

namespace rasterizer_index_buffer_create_impl {




/**
 * Direct3D 9 back end function rasterizer_index_buffer_create.
 *
 * @address 0x525030
 */
uint8_t rasterizer_index_buffer_create(int32_t count, int16_t type, rasterizer_index_buffer *out, const void *source)
{
    uint32_t byte_size = 0;
    uint8_t ok = 1;
    void *buffer;
    void *data;
    uint32_t usage;

    if (type == 0) {
        byte_size = (uint32_t)count * 6;
    } else if (type == 1) {
        byte_size = (uint32_t)count * 2 + 4;
    }
    out->type = type;
    out->count = count;
    if (rasterizer_device == NULL) {
        return ok;
    }
    usage = (rasterizer_software_vertex_processing ? 0x10 : 0) | rasterizer_vertex_declarations[type].usage;
    if (render_device().create_index_buffer(byte_size, usage, 0x65, (usage & 0x10) ? 2 : 1, &buffer, NULL) < 0) {
        ok = 0;
    }
    if (buffer != NULL && ok) {
        if (render_device().buffer_lock(buffer, 0, byte_size, &data, 0) < 0) {
            ok = 0;
        }
        if (data != NULL && ok) {
            memcpy(data, source, byte_size);
            if (render_device().buffer_unlock(buffer) < 0) {
                ok = 0;
            }
            out->hardware_buffer = buffer;
            out->data = source;
            return ok;
        }
    }
    ok = 0;
    out->type = 0;
    out->pad_02 = 0;
    out->count = 0;
    out->data = 0;
    out->hardware_buffer = 0;
    return ok;
}

}  // namespace rasterizer_index_buffer_create_impl

/**
 * 0x519980 (this session) Reads the whole file `path` into a newly GlobalAlloc'd buffer, verifies it with
 * rasterizer_resource_file_verify_signature, and returns the buffer and size on success (1); frees the buffer
 * and returns 0 on any failure.
 *
 * Registers: ECX -> path, stack -> (out_buffer, out_size)
 *
 * @address 0x5199f0
 */
uint32_t rasterizer_load_file_and_verify(void **out_buffer, uint32_t *out_size, const char *path)
{
    void *file;
    uint32_t size;
    void *buffer;

    *out_buffer = nullptr;
    *out_size = 0;

    file = CreateFileA(path, halo::win32::k_generic_read, halo::win32::k_file_share_none, (LPSECURITY_ATTRIBUTES)nullptr, halo::win32::k_open_existing, halo::win32::k_file_flag_sequential_scan, nullptr);
    if (file == halo::win32::invalid_handle()) {
        return 0;
    }

    size = GetFileSize(file, (LPDWORD)(nullptr));
    if (size == halo::k_dword_none) {
        CloseHandle(file);
        return 0;
    }

    buffer = GlobalAlloc(0, size);
    if (buffer != nullptr) {
        uint32_t bytes_read;
        int32_t ok = ReadFile(file, buffer, size, (LPDWORD)(&bytes_read), (LPOVERLAPPED)nullptr);
        if (ok != 0) {
            CloseHandle(file);
            if (rasterizer_resource_file_verify_signature(static_cast<uint8_t *>(buffer), size) == 0) {
                GlobalFree(buffer);
                return 0;
            }
            *out_buffer = buffer;
            *out_size = size;
            return 1;
        }
        GlobalFree(buffer);
    }
    CloseHandle(file);
    return 0;
}

namespace rasterizer_misc_vertex_buffer_create_impl {




/**
 * Creates and zero-initializes a shared 64KB dynamic vertex buffer used elsewhere in the rasterizer for
 * miscellaneous small draws.
 *
 * @address 0x534e50
 */
uint8_t rasterizer_misc_vertex_buffer_create(void)
{
    uint32_t sw_flag;
    uint32_t usage;
    int32_t hr;
    void *buffer;
    void *data;

    sw_flag = (rasterizer_software_vertex_processing != 0) ? halo::d3d9::k_usage_software_processing : 0u;
    usage = sw_flag | rasterizer_vertex_declarations[16].usage;

    buffer = 0;
    hr = render_device().create_vertex_buffer(k_misc_vertex_buffer_bytes, usage, 0, (usage & (halo::d3d9::k_usage_dynamic | halo::d3d9::k_usage_software_processing)) != 0 ? halo::d3d9::k_pool_system_memory : halo::d3d9::k_pool_managed, &buffer, 0);
    rasterizer_misc_vertex_buffer = (hr >= 0) ? buffer : 0;
    if (rasterizer_misc_vertex_buffer == 0) {
        return 0;
    }

    data = 0;
    rasterizer_vertex_buffer_lock_state = 2;
    hr = render_device().buffer_lock(rasterizer_misc_vertex_buffer, 0, k_misc_vertex_buffer_bytes, &data, 0);
    rasterizer_vertex_buffer_lock_state = 0;

    if (hr >= 0 && data != 0) {
        std::memset(data, 0, k_misc_vertex_buffer_cleared_bytes);
        hr = render_device().buffer_unlock(rasterizer_misc_vertex_buffer);
        if (hr >= 0) {
            return 1;
        }
    }
    return 0;
}

}  // namespace rasterizer_misc_vertex_buffer_create_impl

namespace rasterizer_render_target_capture_frame_impl {












static void rasterizer_set_render_state(uint32_t state, uint32_t value)
{
    render_device().set_render_state(state, value);
}

static void rasterizer_set_sampler_state(uint32_t stage, uint32_t type, uint32_t value)
{
    render_device().set_sampler_state(stage, type, value);
}

static void rasterizer_set_texture_stage_state(uint32_t sampler, uint32_t type, uint32_t value)
{
    render_device().set_texture_stage_state(sampler, type, value);
}

static void rasterizer_set_screen_quad_vertex(int32_t index, float x, float y, float u, float v)
{
    float *vertex = rasterizer_screen_quad_vertices[index];

    vertex[0] = x;
    vertex[1] = y;
    vertex[2] = 0.0f;
    vertex[3] = 0.0f;
    vertex[4] = u;
    vertex[5] = v;
}

/**
 * Direct3D 9 back end function rasterizer_render_target_capture_frame.
 *
 * @address 0x519b00
 */
void rasterizer_render_target_capture_frame(void)
{
    void *surface;
    d3d_surface_desc desc;
    d3d_viewport viewport;
    float constants[20];
    float width;
    float height;
    float inverse_width;
    float inverse_height;

    if (halo::rasterizer::fields::active_camouflage_enabled == 0 || rasterizer_caps_flag_688 != 0 || rasterizer_caps_flag_68a != 0 ||
        rasterizer_caps.pixel_shader_version < halo::d3d9::k_pixel_shader_version_1_1) {
        return;
    }

    surface = rasterizer_render_targets[2].surface;
    render_device().set_render_target(0, surface);
    rasterizer_active_render_target = 2;
    render_device().surface_get_desc(surface, &desc);
    viewport.x = 0;
    viewport.y = 0;
    viewport.width = desc.width;
    viewport.height = desc.height;
    viewport.min_z = 0.0f;
    viewport.max_z = 1.0f;
    render_device().set_viewport(&viewport);
    rasterizer_set_shader_stage_config(0);

    if (rasterizer_render_target_capture_requested != 0) {
        rasterizer_vertex_declaration *declaration = &rasterizer_vertex_declarations[_rasterizer_vertex_type_dynamic_screen];

        render_device().set_vertex_declaration(declaration->declaration);
        render_device().set_software_vertex_processing(((rasterizer_software_vertex_processing != 0 ? 0x10 : 0) |
                                                    declaration->usage) & 0x10);
        render_device().set_vertex_shader(rasterizer_vertex_shaders[35].shader);
        render_device().set_pixel_shader(0);
        render_device().set_texture(0, rasterizer_render_targets[1].texture);
        rasterizer_set_sampler_state(0, halo::d3d9::ss::address_u, 3);
        rasterizer_set_sampler_state(0, halo::d3d9::ss::address_v, 3);
        rasterizer_set_sampler_state(0, halo::d3d9::ss::mag_filter, 2);
        rasterizer_set_sampler_state(0, halo::d3d9::ss::min_filter, 2);
        rasterizer_set_sampler_state(0, halo::d3d9::ss::mip_filter, 1);
        rasterizer_set_render_state(halo::d3d9::rs::cull_mode, 3);
        rasterizer_set_render_state(halo::d3d9::rs::color_write_enable, 7);
        rasterizer_set_render_state(halo::d3d9::rs::alpha_blend_enable, 0);
        rasterizer_set_render_state(halo::d3d9::rs::alpha_test_enable, 0);
        rasterizer_set_render_state(halo::d3d9::rs::z_enable, 0);
        rasterizer_set_render_state(halo::d3d9::rs::fog_enable, 0);
        rasterizer_set_texture_stage_state(0, halo::d3d9::ts::color_op, halo::d3d9::top::select_arg1);
        rasterizer_set_texture_stage_state(0, halo::d3d9::ts::color_arg1, halo::d3d9::ta::texture);
        rasterizer_set_texture_stage_state(0, halo::d3d9::ts::alpha_op, halo::d3d9::top::select_arg1);
        rasterizer_set_texture_stage_state(0, halo::d3d9::ts::alpha_arg1, halo::d3d9::ta::texture);
        rasterizer_set_texture_stage_state(1, halo::d3d9::ts::color_op, halo::d3d9::top::disable);
        rasterizer_set_texture_stage_state(1, halo::d3d9::ts::alpha_op, halo::d3d9::top::disable);

        width = (float)(int16_t)(rasterizer_window.camera.viewport_bounds.right - rasterizer_window.camera.viewport_bounds.left);
        height = (float)(int16_t)(rasterizer_window.camera.viewport_bounds.bottom - rasterizer_window.camera.viewport_bounds.top);
        inverse_width = 1.0f / width;
        inverse_height = 1.0f / height;

        constants[0] = inverse_width + inverse_width;
        constants[1] = 0.0f;
        constants[2] = 0.0f;
        constants[3] = -1.0f - inverse_width;
        constants[4] = 0.0f;
        constants[5] = inverse_height * -2.0f;
        constants[6] = 0.0f;
        constants[7] = inverse_height + 1.0f;

        constants[8] = 0.0f;
        constants[9] = 0.0f;
        constants[10] = 0.0f;
        constants[11] = 0.5f;
        constants[12] = 0.0f;
        constants[13] = 0.0f;
        constants[14] = 0.0f;
        constants[15] = 1.0f;
        constants[16] = 1.0f;
        constants[17] = 1.0f;
        constants[18] = 0.0f;
        constants[19] = 1.0f;
        render_device().set_vertex_shader_constant_f(0xd, constants, 5);

        rasterizer_set_screen_quad_vertex(0, 0.0f, 0.0f, 0.0f, 0.0f);
        rasterizer_set_screen_quad_vertex(1, width, 0.0f, 1.0f, 0.0f);
        rasterizer_set_screen_quad_vertex(2, width, height, 1.0f, 1.0f);
        rasterizer_set_screen_quad_vertex(3, 0.0f, height, 0.0f, 1.0f);
        render_device().draw_primitive_up(6, 2, rasterizer_screen_quad_vertices, 0x18);
        render_device().set_software_vertex_processing(rasterizer_software_vertex_processing);
    }

    rasterizer_render_target_set_active(rasterizer_window.type, 0, 0);
    rasterizer_set_shader_stage_config(2);
    if (console_debug_toggle_689422 == 0) {
        rasterizer_render_target_capture_requested = 0;
    }
    rasterizer_render_target_capture_done = 1;
}

}  // namespace rasterizer_render_target_capture_frame_impl

namespace rasterizer_render_target_dispose_impl {


static void release_com(void **slot)
{
    void *object = *slot;
    if (object != 0) {
        render_device().release(object);
        *slot = 0;
    }
}

/**
 * Releases the off-screen render-target pool's surfaces/textures and the two supporting device buffers created
 * by rasterizer_render_target_initialize, resetting all handles to none.
 *
 * @address 0x52cc50
 */
void rasterizer_render_target_dispose(void)
{
    int i;

    for (i = 0; i < k_rasterizer_render_targets; i++) {
        release_com(&rasterizer_render_targets[i].surface);
        release_com(&rasterizer_render_targets[i].texture);
    }

    rasterizer_active_render_target = -1;

    if (rasterizer_render_target_index_buffer != 0) {
        render_device().release(rasterizer_render_target_index_buffer);
        rasterizer_render_target_index_buffer = 0;
    }
    if (rasterizer_render_target_vertex_buffer != 0) {
        render_device().release(rasterizer_render_target_vertex_buffer);
        rasterizer_render_target_vertex_buffer = 0;
    }
}

}  // namespace rasterizer_render_target_dispose_impl

namespace rasterizer_render_target_initialize_impl {








/**
 * Direct3D 9 back end function rasterizer_render_target_initialize.
 *
 * @address 0x52ca20
 */
uint8_t rasterizer_render_target_initialize(void)
{
    uint8_t ok = 1;
    d3d_surface_desc desc;
    uint16_t *indices;
    int i;

    if (rasterizer_caps_flag_68a) {
        if (render_device().get_render_target(0, &rasterizer_render_targets[1].surface) < 0) {
            ok = 0;
        }
        if (render_device().surface_get_desc(rasterizer_render_targets[1].surface, &desc) < 0) {
            ok = 0;
        }
        rasterizer_render_targets[1].format = desc.format;
        rasterizer_render_targets[1].width = desc.width;
        rasterizer_render_targets[1].height = desc.height;
    } else {
        if (render_device().get_render_target(0, &rasterizer_render_targets[0].surface) < 0) {
            ok = 0;
        }
        if (render_device().surface_get_desc(rasterizer_render_targets[0].surface, &desc) < 0) {
            ok = 0;
        }
        rasterizer_render_targets[0].format = desc.format;
        rasterizer_render_targets[0].width = desc.width;
        rasterizer_render_targets[1].width = desc.width;
        rasterizer_render_targets[0].height = desc.height;
        rasterizer_render_targets[1].height = desc.height;
        rasterizer_render_targets[1].format = 0x15;
        rasterizer_render_targets[2].width = desc.width >> 1;
        rasterizer_render_targets[2].height = desc.height >> 1;
    }

    if (!rasterizer_caps_flag_689 && ok) {
        for (i = 1; i < k_rasterizer_render_targets; i++) {
            rasterizer_render_target *target = &rasterizer_render_targets[i];

            if (target->surface == 0 && !(rasterizer_caps_flag_68a && target->format == 0x15)) {
                if (render_device().create_texture(target->width, target->height, 1, 1, target->format, 0, &target->texture, NULL) < 0) {
                    ok = 0;
                }
                if (target->texture == 0) {
                    halo::shell::shell_display_fatal_error_dialog(0x69, 0x72, 1);
                }
                if (render_device().texture_get_surface_level(target->texture, 0, &target->surface) < 0) {
                    ok = 0;
                }
            }
            if (!ok) {
                break;
            }
        }
    }

    if (render_device().create_index_buffer(8, 8, 0x65, 0, &rasterizer_render_target_index_buffer, NULL) < 0) {
        ok = 0;
    }
    if (rasterizer_render_target_index_buffer == NULL) {
        return 0;
    }
    indices = NULL;
    if (render_device().buffer_lock(rasterizer_render_target_index_buffer, 0, 8, &indices, 0) < 0) {
        ok = 0;
    }
    if (indices != NULL) {
        indices[0] = 0;
        indices[1] = 1;
        indices[2] = 2;
        indices[3] = 3;
        if (render_device().buffer_unlock(rasterizer_render_target_index_buffer) < 0) {
            return 0;
        }
    }
    if (ok) {
        if (render_device().create_vertex_buffer(halo::rasterizer::d3dx::fvf_vertex_size(halo::d3d9::k_fvf_xyzrhw_diffuse_tex1) * 4, halo::d3d9::k_usage_dynamic | halo::d3d9::k_usage_write_only, halo::d3d9::k_fvf_xyzrhw_diffuse_tex1, halo::d3d9::k_pool_default, &rasterizer_render_target_vertex_buffer, NULL) < 0) {
            return 0;
        }
    }
    return ok;
}

}  // namespace rasterizer_render_target_initialize_impl

namespace rasterizer_render_target_set_active_impl {





/**
 * Switches the active off-screen render target used by the debug-overlay and dynamic-light-shadow drawing
 * routines, and optionally clears it.
 *
 * Registers: EAX -> target_index, stack -> (clear_color, clear)
 *
 * @address 0x52ccc0
 */
void rasterizer_render_target_set_active(int16_t target_index, uint32_t clear_color, uint8_t clear)
{
    void *surface = 0;
    d3d_viewport viewport;

    if (target_index < 9 && target_index >= 0) {
        surface = rasterizer_render_targets[target_index].surface;
    }

    render_device().set_render_target(0, surface);

    rasterizer_active_render_target = target_index;

    if (target_index == 1) {
        viewport.x = rasterizer_window.camera.viewport_bounds.left;
        viewport.y = rasterizer_window.camera.viewport_bounds.top;
        viewport.width = rasterizer_window.camera.viewport_bounds.right - rasterizer_window.camera.viewport_bounds.left;
        viewport.height = rasterizer_window.camera.viewport_bounds.bottom - rasterizer_window.camera.viewport_bounds.top;
    } else {
        d3d_surface_desc desc;
        render_device().surface_get_desc(surface, &desc);
        viewport.x = 0;
        viewport.y = 0;
        viewport.width = desc.width;
        viewport.height = desc.height;
    }
    viewport.min_z = 0.0f;
    viewport.max_z = 1.0f;

    render_device().set_viewport(&viewport);

    if (clear != 0) {
        uint32_t flags = (target_index == 1 || target_index == 2) ? 7 : 1;
        render_device().clear(0, 0, flags, clear_color, 1.0f, 0);
    }
}

}  // namespace rasterizer_render_target_set_active_impl

/**
 * 0x61a730, MD5 as 32 hex digits + NUL Rejects undersized buffers, then compares the last 33 bytes of `buffer`
 * against a freshly computed reference signature.
 *
 * Registers: in_EAX -> size, unaff_EBX -> buffer
 *
 * @address 0x519980
 */
uint8_t rasterizer_resource_file_verify_signature(uint8_t *buffer, uint32_t size)
{
    char reference[36];
    const char *tail;
    const char *ref;
    int32_t remaining;
    uint8_t matches;

    if (size < k_resource_signature_size + 1) {
        return (uint8_t)(size & 0xffffff00);
    }

    {
        uint32_t key[4] = { k_resource_tea_key[0], k_resource_tea_key[1], k_resource_tea_key[2], k_resource_tea_key[3] };

        halo::cseries::tea_decrypt_buffer((int32_t)size, buffer, key);
        halo::cseries::md5_hex_digest(buffer, (int32_t)size - k_resource_signature_size, reference);
    }

    matches = 1;
    remaining = k_resource_signature_size;
    tail = reinterpret_cast<const char *>(buffer + size - k_resource_signature_size);
    ref = reference;
    do {
        if (remaining == 0) {
            break;
        }
        remaining = remaining - 1;
        matches = (*tail == *ref);
        tail++;
        ref++;
    } while (matches);

    return matches;
}

namespace rasterizer_vertex_buffer_create_impl {



static void copy_dwords(uint8_t *dst, const uint8_t *src, uint32_t byte_count)
{
    memcpy(dst, src, byte_count);
}

static void rasterizer_vertex_buffer_fill(void *locked, int16_t vertex_type, int32_t count, const uint8_t *source,
                                          const uint8_t *second_stream, uint32_t size)
{
    uint8_t *dst = static_cast<uint8_t *>(locked);
    int32_t i;

    if (rasterizer_caps.pixel_shader_version < halo::d3d9::k_pixel_shader_version_1_1) {
        switch (vertex_type) {
        case 12:
        case 14: {
            int32_t stride = (vertex_type == 12) ? 0x38 : 0x44;

            for (i = 0; i < count; i++) {
                copy_dwords(dst + i * 0x20, source + i * stride, 0x18);
                copy_dwords(dst + i * 0x20 + 0x18, source + i * stride + 0x30, 8);
            }
            return;
        }
        case 13:
            for (i = 0; i < count; i++) {
                copy_dwords(dst + i * 8, source + i * 0x14 + 0xc, 8);
            }
            return;
        case 19:
            for (i = 0; i < count; i++) {
                copy_dwords(dst + i * 0x28, source + i * 0x38, 0x18);
                copy_dwords(dst + i * 0x28 + 0x18, source + i * 0x38 + 0x30, 8);
                copy_dwords(dst + i * 0x28 + 0x20, second_stream + i * 0x14 + 0xc, 8);
            }
            return;
        default:
            break;
        }
    }
    memcpy(dst, source, (size_t)size);
}

/**
 * REWRITTEN (first-boot track, objdump 0x524980..0x52500a): param_1 is the caller's vertex buffer record,
 * which the old version never filled (every model's hardware buffer pointer stayed garbage and the first draw
 * crashed inside Direct3D). Now: no device -> 1, record untouched.
 *
 * Registers: stack -> (record, vertex_type, count, source_data, second_stream, size)
 *
 * @address 0x524980
 */
uint8_t rasterizer_vertex_buffer_create(rasterizer_vertex_buffer *record, int16_t vertex_type, int32_t count, void *source_data, int32_t second_stream, uint32_t size)
{
    void *buffer;
    uint8_t ok = 1;
    void *locked_data = 0;

    if (rasterizer_device == 0) {
        return 1;
    }
    buffer = rasterizer_dx9_create_vertex_buffer(vertex_type, size, rasterizer_vertex_declarations[vertex_type].fvf, 1);
    if (buffer == 0) {
        ok = 0;
    }
    if (source_data == 0) {
        if (ok) {
            return ok;
        }
    } else if (ok) {
        if (render_device().buffer_lock(buffer, 0, size, &locked_data, 0) < 0) {
            ok = 0;
        }
        if (locked_data == 0) {
            ok = 0;
        }
        if (ok) {
            rasterizer_vertex_buffer_fill(locked_data, vertex_type, count, static_cast<const uint8_t *>(source_data),
                                          reinterpret_cast<const uint8_t *>(static_cast<uintptr_t>(second_stream)), size);
            if (render_device().buffer_unlock(buffer) < 0) {
                ok = 0;
            }
            record->type = vertex_type;
            record->count = count;
            record->unknown_08 = 0;
            record->unknown_0a = 0;
            record->data = source_data;
            record->hardware_buffer = buffer;
            if (ok) {
                return ok;
            }
        }
    }
    record->type = 0;
    record->pad_02 = 0;
    record->count = 0;
    record->unknown_08 = 0;
            record->unknown_0a = 0;
    record->data = 0;
    record->hardware_buffer = 0;
    return ok;
}

}  // namespace rasterizer_vertex_buffer_create_impl

/**
 * Looks up a free or matching entry in the vertex-buffer slot table, creates the buffer for it, and stores its
 * parameters; returns the 1-based slot handle, or 0 on failure.
 *
 * @address 0x5305f0
 */
int32_t rasterizer_vertex_buffer_slot_allocate(int32_t vertex_type, uint32_t fvf, uint32_t length)
{
    void *buffer;
    int32_t index;

    if (rasterizer_vertex_buffer_slot_count == k_rasterizer_vertex_buffer_slots) {
        return 0;
    }
    buffer = rasterizer_dx9_create_vertex_buffer(vertex_type, length, fvf, 0);
    if (buffer == 0) {
        return 0;
    }

    if (rasterizer_vertex_buffer_slot_high_water == k_rasterizer_vertex_buffer_slots) {
        for (index = 0; index < k_rasterizer_vertex_buffer_slots; index++) {
            if (rasterizer_vertex_buffer_slots[index].hardware_buffer == 0) {
                break;
            }
        }
    } else {
        index = rasterizer_vertex_buffer_slot_high_water;
        rasterizer_vertex_buffer_slot_high_water++;
    }
    rasterizer_vertex_buffer_slot_count++;

    rasterizer_vertex_buffer_slots[index].hardware_buffer = buffer;
    rasterizer_vertex_buffer_slots[index].vertex_type = vertex_type;
    rasterizer_vertex_buffer_slots[index].length = length;
    rasterizer_vertex_buffer_slots[index].fvf = fvf;
    return index + 1;
}

/**
 * Re-creates the Direct3D vertex buffer for any vertex-buffer-slot entry that is marked in-use (length
 * nonzero) but has lost its compiled buffer handle (managed byte clear), e.g. after a device reset drops
 * default-pool resources.
 *
 * @address 0x530690
 */
void rasterizer_vertex_buffer_slot_recreate_lost(void)
{
    int32_t i;

    for (i = 0; i < rasterizer_vertex_buffer_slot_high_water; i++) {
        rasterizer_vertex_buffer_slot *slot = &rasterizer_vertex_buffer_slots[i];
        if (slot->length != 0 && slot->managed == 0) {
            slot->hardware_buffer = rasterizer_dx9_create_vertex_buffer(slot->vertex_type, slot->length,
                                                                                  slot->fvf, 0);
        }
    }
}

}  // namespace halo::rasterizer
