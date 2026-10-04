/**
 * @file include/halo/rasterizer/render_device.hpp
 * The rendering back end interface and its Direct3D 9 implementation.
 */
#pragma once

#include <cstddef>
#include <cstdint>
#include <type_traits>

namespace halo::rasterizer {

/**
 * A pointer-sized opaque argument of the device interface: a resource pointer, a handle or a small integer that the
 * original code passed as a 32-bit word. Every construction keeps the bit pattern, so the value reaching the
 * backend is exactly the value the caller wrote.
 */
class d3d_arg {
public:
    constexpr d3d_arg() : bits(0) {}
    constexpr d3d_arg(std::nullptr_t) : bits(0) {}
    constexpr d3d_arg(uint32_t value) : bits(value) {}
    constexpr d3d_arg(int32_t value) : bits(static_cast<uint32_t>(value)) {}
    constexpr d3d_arg(unsigned long value) : bits(static_cast<uint32_t>(value)) {}
    constexpr d3d_arg(long value) : bits(static_cast<uint32_t>(value)) {}
    template <typename T>
    d3d_arg(T *pointer) : bits(static_cast<uint32_t>(reinterpret_cast<uintptr_t>(const_cast<std::remove_cv_t<T> *>(pointer)))) {}

    /** The argument as the pointer-sized value handed to the backend call. */
    void *get() const { return reinterpret_cast<void *>(static_cast<uintptr_t>(bits)); }

private:
    uint32_t bits;
};

/** The pointer a device call that returns a 32-bit handle (technique, parameter) stands for. */
inline void *d3d_handle(int32_t value)
{
    return d3d_arg(value).get();
}

/**
 * Backend-neutral rendering device. The rasterizer draws through this interface (device states, draws, render
 * targets, buffers, textures, shader effects and the present call), so a different backend such as OpenGL can
 * replace Direct3D 9 without touching the callers. Resources are opaque handles owned by the backend; the
 * Direct3D 9 implementation uses the COM pointers the engine stores in its tables.
 */
class RenderDevice {
public:

    /* Device: frame bracket, targets, states, draws */

    /**
     * Resets the device with new presentation parameters (IDirect3DDevice9::Reset).
     */
    virtual int32_t reset(d3d_arg present_parameters) = 0;

    /**
     * Presents the back buffer (IDirect3DDevice9::Present).
     */
    virtual int32_t present(d3d_arg source_rect, d3d_arg dest_rect, d3d_arg dest_window, d3d_arg dirty_region) = 0;

    /**
     * Creates a 2D texture (IDirect3DDevice9::CreateTexture).
     */
    virtual int32_t create_texture(uint32_t width, uint32_t height, uint32_t levels, uint32_t usage, uint32_t format, uint32_t pool, d3d_arg out_texture, d3d_arg shared_handle) = 0;

    /**
     * Creates a volume texture (IDirect3DDevice9::CreateVolumeTexture).
     */
    virtual int32_t create_volume_texture(uint32_t width, uint32_t height, uint32_t depth, uint32_t levels, uint32_t usage, uint32_t format, uint32_t pool, d3d_arg out_texture, d3d_arg shared_handle) = 0;

    /**
     * Creates a cube texture (IDirect3DDevice9::CreateCubeTexture).
     */
    virtual int32_t create_cube_texture(uint32_t edge_length, uint32_t levels, uint32_t usage, uint32_t format, uint32_t pool, d3d_arg out_texture, d3d_arg shared_handle) = 0;

    /**
     * Creates a vertex buffer (IDirect3DDevice9::CreateVertexBuffer).
     */
    virtual int32_t create_vertex_buffer(uint32_t length, uint32_t usage, uint32_t fvf, uint32_t pool, d3d_arg out_buffer, d3d_arg shared_handle) = 0;

    /**
     * Creates an index buffer (IDirect3DDevice9::CreateIndexBuffer).
     */
    virtual int32_t create_index_buffer(uint32_t length, uint32_t usage, uint32_t format, uint32_t pool, d3d_arg out_buffer, d3d_arg shared_handle) = 0;

    /**
     * Copies a surface region with optional filtering (IDirect3DDevice9::StretchRect).
     */
    virtual int32_t stretch_rect(d3d_arg source_surface, d3d_arg source_rect, d3d_arg dest_surface, d3d_arg dest_rect, uint32_t filter) = 0;

    /**
     * Creates an off-screen surface (IDirect3DDevice9::CreateOffscreenPlainSurface).
     */
    virtual int32_t create_offscreen_plain_surface(uint32_t width, uint32_t height, uint32_t format, uint32_t pool, d3d_arg out_surface, d3d_arg shared_handle) = 0;

    /**
     * Binds a surface as render target (IDirect3DDevice9::SetRenderTarget).
     */
    virtual int32_t set_render_target(uint32_t index, d3d_arg surface) = 0;

    /**
     * Fetches a bound render target (IDirect3DDevice9::GetRenderTarget).
     */
    virtual int32_t get_render_target(uint32_t index, d3d_arg out_surface) = 0;

    /**
     * Begins a scene (IDirect3DDevice9::BeginScene).
     */
    virtual int32_t begin_scene() = 0;

    /**
     * Ends a scene (IDirect3DDevice9::EndScene).
     */
    virtual int32_t end_scene() = 0;

    /**
     * Clears the targets (IDirect3DDevice9::Clear).
     */
    virtual int32_t clear(uint32_t count, d3d_arg rects, uint32_t flags, uint32_t color, float z, uint32_t stencil) = 0;

    /**
     * Sets a fixed-function transform (IDirect3DDevice9::SetTransform).
     */
    virtual int32_t set_transform(uint32_t state, d3d_arg matrix) = 0;

    /**
     * Sets the viewport (IDirect3DDevice9::SetViewport).
     */
    virtual int32_t set_viewport(d3d_arg viewport) = 0;

    /**
     * Sets the fixed-function material (IDirect3DDevice9::SetMaterial).
     */
    virtual int32_t set_material(d3d_arg material) = 0;

    /**
     * Defines a fixed-function light (IDirect3DDevice9::SetLight).
     */
    virtual int32_t set_light(uint32_t index, d3d_arg light) = 0;

    /**
     * Enables or disables a fixed-function light (IDirect3DDevice9::LightEnable).
     */
    virtual int32_t light_enable(uint32_t index, int32_t enable) = 0;

    /**
     * Sets a render state (IDirect3DDevice9::SetRenderState).
     */
    virtual int32_t set_render_state(uint32_t state, uint32_t value) = 0;

    /**
     * Binds a texture to a stage (IDirect3DDevice9::SetTexture).
     */
    virtual int32_t set_texture(uint32_t stage, d3d_arg texture) = 0;

    /**
     * Sets a texture stage state (IDirect3DDevice9::SetTextureStageState).
     */
    virtual int32_t set_texture_stage_state(uint32_t stage, uint32_t type, uint32_t value) = 0;

    /**
     * Sets a sampler state (IDirect3DDevice9::SetSamplerState).
     */
    virtual int32_t set_sampler_state(uint32_t sampler, uint32_t type, uint32_t value) = 0;

    /**
     * Switches software vertex processing (IDirect3DDevice9::SetSoftwareVertexProcessing).
     */
    virtual int32_t set_software_vertex_processing(uint32_t enable) = 0;

    /**
     * Draws non-indexed primitives from the bound stream (IDirect3DDevice9::DrawPrimitive).
     */
    virtual int32_t draw_primitive(uint32_t type, uint32_t start_vertex, uint32_t primitive_count) = 0;

    /**
     * Draws indexed primitives (IDirect3DDevice9::DrawIndexedPrimitive).
     */
    virtual int32_t draw_indexed_primitive(uint32_t type, int32_t base_vertex, uint32_t min_index, uint32_t vertex_count, uint32_t start_index, uint32_t primitive_count) = 0;

    /**
     * Draws primitives from user memory (IDirect3DDevice9::DrawPrimitiveUP).
     */
    virtual int32_t draw_primitive_up(uint32_t type, uint32_t primitive_count, d3d_arg data, uint32_t stride) = 0;

    /**
     * Draws indexed primitives from user memory (IDirect3DDevice9::DrawIndexedPrimitiveUP).
     */
    virtual int32_t draw_indexed_primitive_up(uint32_t type, uint32_t min_index, uint32_t vertex_count, uint32_t primitive_count, d3d_arg index_data, uint32_t index_format, d3d_arg vertex_data, uint32_t stride) = 0;

    /**
     * Binds a vertex declaration (IDirect3DDevice9::SetVertexDeclaration).
     */
    virtual int32_t set_vertex_declaration(d3d_arg declaration) = 0;

    /**
     * Sets the fixed-function vertex format (IDirect3DDevice9::SetFVF).
     */
    virtual int32_t set_fvf(uint32_t fvf) = 0;

    /**
     * Binds a vertex shader (IDirect3DDevice9::SetVertexShader).
     */
    virtual int32_t set_vertex_shader(d3d_arg shader) = 0;

    /**
     * Uploads vertex shader float constants (IDirect3DDevice9::SetVertexShaderConstantF).
     */
    virtual int32_t set_vertex_shader_constant_f(uint32_t start_register, d3d_arg data, uint32_t vector4_count) = 0;

    /**
     * Binds a vertex stream (IDirect3DDevice9::SetStreamSource).
     */
    virtual int32_t set_stream_source(uint32_t stream, d3d_arg buffer, uint32_t offset, uint32_t stride) = 0;

    /**
     * Binds the index buffer (IDirect3DDevice9::SetIndices).
     */
    virtual int32_t set_indices(d3d_arg index_buffer) = 0;

    /**
     * Binds a pixel shader (IDirect3DDevice9::SetPixelShader).
     */
    virtual int32_t set_pixel_shader(d3d_arg shader) = 0;

    /**
     * Uploads pixel shader float constants (IDirect3DDevice9::SetPixelShaderConstantF).
     */
    virtual int32_t set_pixel_shader_constant_f(uint32_t start_register, d3d_arg data, uint32_t vector4_count) = 0;

    /**
     * Reads the display mode of a swap chain (IDirect3DDevice9::GetDisplayMode).
     */
    virtual int32_t get_display_mode(uint32_t swap_chain, d3d_arg mode) = 0;

    /**
     * Fetches a back buffer surface (IDirect3DDevice9::GetBackBuffer).
     */
    virtual int32_t get_back_buffer(uint32_t swap_chain, uint32_t index, uint32_t type, d3d_arg out_surface) = 0;

    /**
     * Loads a gamma ramp (IDirect3DDevice9::SetGammaRamp).
     */
    virtual int32_t set_gamma_ramp(uint32_t swap_chain, uint32_t flags, d3d_arg ramp) = 0;

    /**
     * Transforms vertices into a buffer (IDirect3DDevice9::ProcessVertices).
     */
    virtual int32_t process_vertices(uint32_t source_start, uint32_t dest_index, uint32_t vertex_count, d3d_arg dest_buffer, d3d_arg declaration, uint32_t flags) = 0;

    /**
     * Creates a vertex declaration (IDirect3DDevice9::CreateVertexDeclaration).
     */
    virtual int32_t create_vertex_declaration(d3d_arg elements, d3d_arg out_declaration) = 0;

    /**
     * Creates a vertex shader (IDirect3DDevice9::CreateVertexShader).
     */
    virtual int32_t create_vertex_shader(d3d_arg function, d3d_arg out_shader) = 0;

    /**
     * Creates a pixel shader (IDirect3DDevice9::CreatePixelShader).
     */
    virtual int32_t create_pixel_shader(d3d_arg function, d3d_arg out_shader) = 0;

    /**
     * Creates an occlusion query (IDirect3DDevice9::CreateQuery).
     */
    virtual int32_t create_query(uint32_t type, d3d_arg out_query) = 0;


    /* Resources: lifetime */

    /**
     * Drops one reference on a device resource (IUnknown::Release).
     */
    virtual uint32_t release(d3d_arg object) = 0;


    /* The Direct3D factory object */

    /**
     * Number of display adapters (IDirect3D9::GetAdapterCount).
     */
    virtual uint32_t get_adapter_count(d3d_arg object) = 0;

    /**
     * Current display mode of an adapter (IDirect3D9::GetAdapterDisplayMode).
     */
    virtual int32_t get_adapter_display_mode(d3d_arg object, uint32_t adapter, d3d_arg mode) = 0;

    /**
     * Tests whether a surface format is usable (IDirect3D9::CheckDeviceFormat).
     */
    virtual int32_t check_device_format(d3d_arg object, uint32_t adapter, uint32_t device_type, uint32_t adapter_format, uint32_t usage, uint32_t resource_type, uint32_t check_format) = 0;

    /**
     * Reads the device capabilities (IDirect3D9::GetDeviceCaps).
     */
    virtual int32_t get_device_caps(d3d_arg object, uint32_t adapter, uint32_t device_type, d3d_arg caps) = 0;

    /**
     * Creates the rendering device (IDirect3D9::CreateDevice).
     */
    virtual int32_t create_device(d3d_arg object, uint32_t adapter, uint32_t device_type, d3d_arg focus_window, uint32_t behavior_flags, d3d_arg present_parameters, d3d_arg out_device) = 0;


    /* Surfaces */

    /**
     * Reads a surface description (IDirect3DSurface9::GetDesc).
     */
    virtual int32_t surface_get_desc(d3d_arg object, d3d_arg desc) = 0;

    /**
     * Locks a surface rectangle (IDirect3DSurface9::LockRect).
     */
    virtual int32_t surface_lock_rect(d3d_arg object, d3d_arg locked_rect, d3d_arg rect, uint32_t flags) = 0;

    /**
     * Unlocks a surface (IDirect3DSurface9::UnlockRect).
     */
    virtual int32_t surface_unlock_rect(d3d_arg object) = 0;


    /* Vertex and index buffers */

    /**
     * Locks a vertex or index buffer range (IDirect3DVertexBuffer9::Lock).
     */
    virtual int32_t buffer_lock(d3d_arg object, uint32_t offset, uint32_t size, d3d_arg out_data, uint32_t flags) = 0;

    /**
     * Unlocks a vertex or index buffer (IDirect3DVertexBuffer9::Unlock).
     */
    virtual int32_t buffer_unlock(d3d_arg object) = 0;

    /**
     * Reads a vertex or index buffer description (IDirect3DVertexBuffer9::GetDesc).
     */
    virtual int32_t buffer_get_desc(d3d_arg object, d3d_arg desc) = 0;


    /* Textures */

    /**
     * Fetches a mip level surface (IDirect3DTexture9::GetSurfaceLevel).
     */
    virtual int32_t texture_get_surface_level(d3d_arg object, uint32_t level, d3d_arg out_surface) = 0;

    /**
     * Locks a mip level rectangle (IDirect3DTexture9::LockRect).
     */
    virtual int32_t texture_lock_rect(d3d_arg object, uint32_t level, d3d_arg locked_rect, d3d_arg rect, uint32_t flags) = 0;

    /**
     * Unlocks a mip level (IDirect3DTexture9::UnlockRect).
     */
    virtual int32_t texture_unlock_rect(d3d_arg object, uint32_t level) = 0;

    /**
     * Locks a mip level box of a volume texture.
     */
    virtual int32_t volume_texture_lock_box(d3d_arg object, uint32_t level, d3d_arg locked_box, d3d_arg box, uint32_t flags) = 0;

    /**
     * Unlocks a mip level of a volume texture.
     */
    virtual int32_t volume_texture_unlock_box(d3d_arg object, uint32_t level) = 0;

    /**
     * Locks a face rectangle of a cube texture.
     */
    virtual int32_t cube_texture_lock_rect(d3d_arg object, uint32_t face, uint32_t level, d3d_arg locked_rect, d3d_arg rect, uint32_t flags) = 0;

    /**
     * Unlocks a face of a cube texture.
     */
    virtual int32_t cube_texture_unlock_rect(d3d_arg object, uint32_t face, uint32_t level) = 0;

    /**
     * Fetches a face mip surface of a cube texture.
     */
    virtual int32_t cube_texture_get_surface(d3d_arg object, uint32_t face, uint32_t level, d3d_arg out_surface) = 0;


    /* Queries */

    /**
     * Issues an occlusion query marker (IDirect3DQuery9::Issue).
     */
    virtual int32_t query_issue(d3d_arg object, uint32_t flags) = 0;

    /**
     * Reads the result of an occlusion query (IDirect3DQuery9::GetData).
     */
    virtual int32_t query_get_data(d3d_arg object, d3d_arg data, uint32_t size, uint32_t flags) = 0;


    /* Effects */

    /**
     * Sets a vector parameter of an effect.
     */
    virtual int32_t effect_set_vector(d3d_arg object, d3d_arg handle, d3d_arg vector) = 0;

    /**
     * Sets a texture parameter of an effect.
     */
    virtual int32_t effect_set_texture(d3d_arg object, d3d_arg handle, d3d_arg texture) = 0;

    /**
     * Selects the active technique of an effect.
     */
    virtual int32_t effect_set_technique(d3d_arg object, d3d_arg technique) = 0;

    /**
     * Tests whether a technique is valid on this device.
     */
    virtual int32_t effect_validate_technique(d3d_arg object, d3d_arg technique) = 0;

    /**
     * Finds the next technique valid on this device.
     */
    virtual int32_t effect_find_next_valid_technique(d3d_arg object, d3d_arg technique, d3d_arg out_technique) = 0;

    /**
     * Starts an effect and reports its pass count.
     */
    virtual int32_t effect_begin(d3d_arg object, d3d_arg out_pass_count, uint32_t flags) = 0;

    /**
     * Activates one pass of the started effect.
     */
    virtual int32_t effect_pass(d3d_arg object, uint32_t pass) = 0;

    /**
     * Ends the started effect.
     */
    virtual int32_t effect_end(d3d_arg object) = 0;

    /**
     * Looks up an effect parameter handle by name.
     */
    virtual int32_t effect_get_parameter_by_name(d3d_arg object, d3d_arg parent, d3d_arg name) = 0;

    /**
     * Looks up an effect technique handle by name.
     */
    virtual int32_t effect_get_technique_by_name(d3d_arg object, d3d_arg name) = 0;

    /**
     * Looks up an effect technique handle by name below a parent handle.
     */
    virtual int32_t effect_get_technique_by_name_scoped(d3d_arg object, d3d_arg parent, d3d_arg name) = 0;

protected:
    constexpr RenderDevice() = default;
    ~RenderDevice() = default;
};

/**
 * The Direct3D 9 backend: every method calls the corresponding slot of the COM interface that the engine keeps in
 * the fixed-address globals (rasterizer_device and the resource pointers passed in), with the stdcall convention and
 * argument order the original code used. The object itself carries no data.
 */
class D3D9Device final : public RenderDevice {
public:
    int32_t reset(d3d_arg present_parameters) override;
    int32_t present(d3d_arg source_rect, d3d_arg dest_rect, d3d_arg dest_window, d3d_arg dirty_region) override;
    int32_t create_texture(uint32_t width, uint32_t height, uint32_t levels, uint32_t usage, uint32_t format, uint32_t pool, d3d_arg out_texture, d3d_arg shared_handle) override;
    int32_t create_volume_texture(uint32_t width, uint32_t height, uint32_t depth, uint32_t levels, uint32_t usage, uint32_t format, uint32_t pool, d3d_arg out_texture, d3d_arg shared_handle) override;
    int32_t create_cube_texture(uint32_t edge_length, uint32_t levels, uint32_t usage, uint32_t format, uint32_t pool, d3d_arg out_texture, d3d_arg shared_handle) override;
    int32_t create_vertex_buffer(uint32_t length, uint32_t usage, uint32_t fvf, uint32_t pool, d3d_arg out_buffer, d3d_arg shared_handle) override;
    int32_t create_index_buffer(uint32_t length, uint32_t usage, uint32_t format, uint32_t pool, d3d_arg out_buffer, d3d_arg shared_handle) override;
    int32_t stretch_rect(d3d_arg source_surface, d3d_arg source_rect, d3d_arg dest_surface, d3d_arg dest_rect, uint32_t filter) override;
    int32_t create_offscreen_plain_surface(uint32_t width, uint32_t height, uint32_t format, uint32_t pool, d3d_arg out_surface, d3d_arg shared_handle) override;
    int32_t set_render_target(uint32_t index, d3d_arg surface) override;
    int32_t get_render_target(uint32_t index, d3d_arg out_surface) override;
    int32_t begin_scene() override;
    int32_t end_scene() override;
    int32_t clear(uint32_t count, d3d_arg rects, uint32_t flags, uint32_t color, float z, uint32_t stencil) override;
    int32_t set_transform(uint32_t state, d3d_arg matrix) override;
    int32_t set_viewport(d3d_arg viewport) override;
    int32_t set_material(d3d_arg material) override;
    int32_t set_light(uint32_t index, d3d_arg light) override;
    int32_t light_enable(uint32_t index, int32_t enable) override;
    int32_t set_render_state(uint32_t state, uint32_t value) override;
    int32_t set_texture(uint32_t stage, d3d_arg texture) override;
    int32_t set_texture_stage_state(uint32_t stage, uint32_t type, uint32_t value) override;
    int32_t set_sampler_state(uint32_t sampler, uint32_t type, uint32_t value) override;
    int32_t set_software_vertex_processing(uint32_t enable) override;
    int32_t draw_primitive(uint32_t type, uint32_t start_vertex, uint32_t primitive_count) override;
    int32_t draw_indexed_primitive(uint32_t type, int32_t base_vertex, uint32_t min_index, uint32_t vertex_count, uint32_t start_index, uint32_t primitive_count) override;
    int32_t draw_primitive_up(uint32_t type, uint32_t primitive_count, d3d_arg data, uint32_t stride) override;
    int32_t draw_indexed_primitive_up(uint32_t type, uint32_t min_index, uint32_t vertex_count, uint32_t primitive_count, d3d_arg index_data, uint32_t index_format, d3d_arg vertex_data, uint32_t stride) override;
    int32_t set_vertex_declaration(d3d_arg declaration) override;
    int32_t set_fvf(uint32_t fvf) override;
    int32_t set_vertex_shader(d3d_arg shader) override;
    int32_t set_vertex_shader_constant_f(uint32_t start_register, d3d_arg data, uint32_t vector4_count) override;
    int32_t set_stream_source(uint32_t stream, d3d_arg buffer, uint32_t offset, uint32_t stride) override;
    int32_t set_indices(d3d_arg index_buffer) override;
    int32_t set_pixel_shader(d3d_arg shader) override;
    int32_t set_pixel_shader_constant_f(uint32_t start_register, d3d_arg data, uint32_t vector4_count) override;
    int32_t get_display_mode(uint32_t swap_chain, d3d_arg mode) override;
    int32_t get_back_buffer(uint32_t swap_chain, uint32_t index, uint32_t type, d3d_arg out_surface) override;
    int32_t set_gamma_ramp(uint32_t swap_chain, uint32_t flags, d3d_arg ramp) override;
    int32_t process_vertices(uint32_t source_start, uint32_t dest_index, uint32_t vertex_count, d3d_arg dest_buffer, d3d_arg declaration, uint32_t flags) override;
    int32_t create_vertex_declaration(d3d_arg elements, d3d_arg out_declaration) override;
    int32_t create_vertex_shader(d3d_arg function, d3d_arg out_shader) override;
    int32_t create_pixel_shader(d3d_arg function, d3d_arg out_shader) override;
    int32_t create_query(uint32_t type, d3d_arg out_query) override;
    uint32_t release(d3d_arg object) override;
    uint32_t get_adapter_count(d3d_arg object) override;
    int32_t get_adapter_display_mode(d3d_arg object, uint32_t adapter, d3d_arg mode) override;
    int32_t check_device_format(d3d_arg object, uint32_t adapter, uint32_t device_type, uint32_t adapter_format, uint32_t usage, uint32_t resource_type, uint32_t check_format) override;
    int32_t get_device_caps(d3d_arg object, uint32_t adapter, uint32_t device_type, d3d_arg caps) override;
    int32_t create_device(d3d_arg object, uint32_t adapter, uint32_t device_type, d3d_arg focus_window, uint32_t behavior_flags, d3d_arg present_parameters, d3d_arg out_device) override;
    int32_t surface_get_desc(d3d_arg object, d3d_arg desc) override;
    int32_t surface_lock_rect(d3d_arg object, d3d_arg locked_rect, d3d_arg rect, uint32_t flags) override;
    int32_t surface_unlock_rect(d3d_arg object) override;
    int32_t buffer_lock(d3d_arg object, uint32_t offset, uint32_t size, d3d_arg out_data, uint32_t flags) override;
    int32_t buffer_unlock(d3d_arg object) override;
    int32_t buffer_get_desc(d3d_arg object, d3d_arg desc) override;
    int32_t texture_get_surface_level(d3d_arg object, uint32_t level, d3d_arg out_surface) override;
    int32_t texture_lock_rect(d3d_arg object, uint32_t level, d3d_arg locked_rect, d3d_arg rect, uint32_t flags) override;
    int32_t texture_unlock_rect(d3d_arg object, uint32_t level) override;
    int32_t volume_texture_lock_box(d3d_arg object, uint32_t level, d3d_arg locked_box, d3d_arg box, uint32_t flags) override;
    int32_t volume_texture_unlock_box(d3d_arg object, uint32_t level) override;
    int32_t cube_texture_lock_rect(d3d_arg object, uint32_t face, uint32_t level, d3d_arg locked_rect, d3d_arg rect, uint32_t flags) override;
    int32_t cube_texture_unlock_rect(d3d_arg object, uint32_t face, uint32_t level) override;
    int32_t cube_texture_get_surface(d3d_arg object, uint32_t face, uint32_t level, d3d_arg out_surface) override;
    int32_t query_issue(d3d_arg object, uint32_t flags) override;
    int32_t query_get_data(d3d_arg object, d3d_arg data, uint32_t size, uint32_t flags) override;
    int32_t effect_set_vector(d3d_arg object, d3d_arg handle, d3d_arg vector) override;
    int32_t effect_set_texture(d3d_arg object, d3d_arg handle, d3d_arg texture) override;
    int32_t effect_set_technique(d3d_arg object, d3d_arg technique) override;
    int32_t effect_validate_technique(d3d_arg object, d3d_arg technique) override;
    int32_t effect_find_next_valid_technique(d3d_arg object, d3d_arg technique, d3d_arg out_technique) override;
    int32_t effect_begin(d3d_arg object, d3d_arg out_pass_count, uint32_t flags) override;
    int32_t effect_pass(d3d_arg object, uint32_t pass) override;
    int32_t effect_end(d3d_arg object) override;
    int32_t effect_get_parameter_by_name(d3d_arg object, d3d_arg parent, d3d_arg name) override;
    int32_t effect_get_technique_by_name(d3d_arg object, d3d_arg name) override;
    int32_t effect_get_technique_by_name_scoped(d3d_arg object, d3d_arg parent, d3d_arg name) override;
};

/** The device instance every caller uses. */
D3D9Device &d3d9_device();

/** The active rendering device. */
RenderDevice &render_device();

}  // namespace halo::rasterizer
