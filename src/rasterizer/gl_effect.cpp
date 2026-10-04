/**
 * @file src/rasterizer/gl_effect.cpp
 * ID3DXEffect for the OpenGL backend: the compiled effect binary (shaders\fx.bin) is parsed by MojoShader, which owns the
 * parameters, techniques, passes and preshaders. This file supplies the shader backend (vertex/pixel shaders become the
 * backend's gl_shader objects, constants are written straight into the tracked register files) and applies each pass's
 * render, sampler and texture bindings to the tracked pipeline state.
 */

#include "gl_internal.hpp"
#include "halo/shell/standalone.hpp"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern "C" int mojo_ctab_line = 0;

extern "C" void mojo_trace(const char *format, ...)
{
    char text[512];
    va_list args;

    va_start(args, format);
    vsnprintf(text, sizeof(text), format, args);
    va_end(args);
    if (getenv("HALO_GL_EFFECT_TRACE") != nullptr) {
        halo::shell::standalone_log("gl: %s", text);
    }
}

namespace halo::rasterizer {

using namespace gl;

namespace gl {

struct gl_effect : gl_object {
    MOJOSHADER_effect *fx;
    MOJOSHADER_effectStateChanges changes;
    gl_object **textures;  // one slot per effect object, indexed by the object number a texture parameter refers to
    bool pass_active;
    bool begun;
};

}  // namespace gl

namespace {

/* ---- shader backend ---- */

uint32_t g_effect_shader_id = 0x10000;

}  // namespace

namespace gl {

MOJOSHADER_parseData g_placeholder_parse;

}  // namespace gl

namespace {

void *MOJOSHADERCALL backend_compile(const void *, const char *mainfn, const unsigned char *tokenbuf, const unsigned int bufsize,
    const MOJOSHADER_swizzle *swiz, const unsigned int swizcount, const MOJOSHADER_samplerMap *smap, const unsigned int smapcount)
{
    if (getenv("HALO_GL_DUMP_SHADERS") != nullptr) {
        char name[260];
        FILE *dump;

        snprintf(name, sizeof(name), "%s/effect_%u.bin", getenv("HALO_GL_DUMP_SHADERS"), g_effect_shader_id);
        dump = fopen(name, "wb");
        if (dump != nullptr) {
            fwrite(tokenbuf, 1, bufsize, dump);
            fclose(dump);
        }
    }
    const MOJOSHADER_parseData *parse;
    gl_shader *shader = new_object<gl_shader>(kind_vertex_shader);

    shader->id = g_effect_shader_id++;
    if (bufsize < 4 || (reinterpret_cast<const uint16_t *>(tokenbuf)[1] != 0xffff && reinterpret_cast<const uint16_t *>(tokenbuf)[1] != 0xfffe)) {
        // a shader-typed effect variable whose data is not bytecode (the 2003 compiler stores a name): inert placeholder
        shader->parse = &g_placeholder_parse;
        return shader;
    }
    parse = MOJOSHADER_parse(MOJOSHADER_PROFILE_GLSL, mainfn, tokenbuf, bufsize, swiz, swizcount, smap, smapcount, nullptr, nullptr, nullptr);
    if (parse != nullptr && parse->error_count > 0) {
        halo::shell::standalone_log("gl: effect shader failed to translate: %s (ctab line %d)", parse->errors[0].error, mojo_ctab_line);
        MOJOSHADER_freeParseData(parse);
        parse = &g_placeholder_parse;
    }
    shader->parse = parse;
    if (parse != nullptr && parse->shader_type == MOJOSHADER_TYPE_PIXEL) {
        shader->kind = kind_pixel_shader;
    }
    return shader;
}

void MOJOSHADERCALL backend_add_ref(void *shader)
{
    static_cast<gl_object *>(shader)->refs++;
}

void MOJOSHADERCALL backend_delete(const void *, void *shader)
{
    if (shader == nullptr) {
        return;
    }
    gl_device().release(static_cast<gl_object *>(shader));
}

MOJOSHADER_parseData *MOJOSHADERCALL backend_parse_data(void *shader)
{
    return const_cast<MOJOSHADER_parseData *>(static_cast<gl_shader *>(shader)->parse);
}

void bind_shader(gl_shader *&slot, gl_shader *shader)
{
    if (shader != nullptr) {
        shader->refs++;
    }
    if (slot != nullptr) {
        gl_device().release(slot);
    }
    slot = shader;
}

void MOJOSHADERCALL backend_bind(const void *, void *vertex, void *pixel)
{
    bind_shader(g_pipe.vertex_shader, static_cast<gl_shader *>(vertex));
    bind_shader(g_pipe.pixel_shader, static_cast<gl_shader *>(pixel));
}

void MOJOSHADERCALL backend_get_bound(const void *, void **vertex, void **pixel)
{
    *vertex = g_pipe.vertex_shader;
    *pixel = g_pipe.pixel_shader;
}

float g_integer_registers[2][256 * 4];
unsigned char g_bool_registers[2][256];

void MOJOSHADERCALL backend_map(const void *, float **vs_float, int **vs_int, unsigned char **vs_bool, float **ps_float, int **ps_int, unsigned char **ps_bool)
{
    *vs_float = &g_pipe.vertex_constants[0][0];
    *ps_float = &g_pipe.pixel_constants[0][0];
    *vs_int = reinterpret_cast<int *>(g_integer_registers[0]);
    *ps_int = reinterpret_cast<int *>(g_integer_registers[1]);
    *vs_bool = g_bool_registers[0];
    *ps_bool = g_bool_registers[1];
}

void MOJOSHADERCALL backend_unmap(const void *) {}

const char *MOJOSHADERCALL backend_error(const void *)
{
    return "";
}

/* ---- state translation ---- */

/** Direct3D 9 render state number for a MojoShader effect render state type, or -1 if the backend ignores it. */
int render_state_number(MOJOSHADER_renderStateType type)
{
    switch (type) {
    case MOJOSHADER_RS_ZENABLE: return 7;
    case MOJOSHADER_RS_FILLMODE: return 8;
    case MOJOSHADER_RS_ZWRITEENABLE: return 14;
    case MOJOSHADER_RS_ALPHATESTENABLE: return 15;
    case MOJOSHADER_RS_SRCBLEND: return 19;
    case MOJOSHADER_RS_DESTBLEND: return 20;
    case MOJOSHADER_RS_CULLMODE: return 22;
    case MOJOSHADER_RS_ZFUNC: return 23;
    case MOJOSHADER_RS_ALPHAREF: return 24;
    case MOJOSHADER_RS_ALPHAFUNC: return 25;
    case MOJOSHADER_RS_ALPHABLENDENABLE: return 27;
    case MOJOSHADER_RS_FOGENABLE: return 28;
    case MOJOSHADER_RS_SPECULARENABLE: return 29;
    case MOJOSHADER_RS_FOGCOLOR: return 34;
    case MOJOSHADER_RS_FOGTABLEMODE: return 35;
    case MOJOSHADER_RS_FOGSTART: return 36;
    case MOJOSHADER_RS_FOGEND: return 37;
    case MOJOSHADER_RS_FOGDENSITY: return 38;
    case MOJOSHADER_RS_STENCILENABLE: return 52;
    case MOJOSHADER_RS_STENCILFAIL: return 53;
    case MOJOSHADER_RS_STENCILZFAIL: return 54;
    case MOJOSHADER_RS_STENCILPASS: return 55;
    case MOJOSHADER_RS_STENCILFUNC: return 56;
    case MOJOSHADER_RS_STENCILREF: return 57;
    case MOJOSHADER_RS_STENCILMASK: return 58;
    case MOJOSHADER_RS_STENCILWRITEMASK: return 59;
    case MOJOSHADER_RS_TEXTUREFACTOR: return 60;
    case MOJOSHADER_RS_LIGHTING: return 137;
    case MOJOSHADER_RS_COLORWRITEENABLE: return 168;
    case MOJOSHADER_RS_BLENDOP: return 171;
    case MOJOSHADER_RS_SLOPESCALEDEPTHBIAS: return 175;
    case MOJOSHADER_RS_BLENDFACTOR: return 193;
    case MOJOSHADER_RS_DEPTHBIAS: return 195;
    case MOJOSHADER_RS_SEPARATEALPHABLENDENABLE: return 206;
    case MOJOSHADER_RS_SRCBLENDALPHA: return 207;
    case MOJOSHADER_RS_DESTBLENDALPHA: return 208;
    case MOJOSHADER_RS_BLENDOPALPHA: return 209;
    default: return -1;
    }
}

/** Direct3D 9 sampler state number (D3DSAMP_*) for an effect sampler state type, or -1. */
int sampler_state_number(MOJOSHADER_samplerStateType type)
{
    switch (type) {
    case MOJOSHADER_SAMP_ADDRESSU: return 1;
    case MOJOSHADER_SAMP_ADDRESSV: return 2;
    case MOJOSHADER_SAMP_ADDRESSW: return 3;
    case MOJOSHADER_SAMP_BORDERCOLOR: return 4;
    case MOJOSHADER_SAMP_MAGFILTER: return 5;
    case MOJOSHADER_SAMP_MINFILTER: return 6;
    case MOJOSHADER_SAMP_MIPFILTER: return 7;
    case MOJOSHADER_SAMP_MAXMIPLEVEL: return 9;
    case MOJOSHADER_SAMP_MAXANISOTROPY: return 10;
    default: return -1;
    }
}

uint32_t value_bits(const MOJOSHADER_effectValue &value)
{
    uint32_t bits = 0;

    if (value.values != nullptr && value.value_count > 0) {
        memcpy(&bits, value.values, sizeof(bits));
    }
    return bits;
}

void apply_samplers(gl_effect *effect, unsigned int count, const MOJOSHADER_samplerStateRegister *samplers)
{
    for (unsigned int i = 0; i < count; i++) {
        const MOJOSHADER_samplerStateRegister &entry = samplers[i];
        uint32_t unit = entry.sampler_register;

        if (unit >= 16) {
            continue;
        }
        for (unsigned int s = 0; s < entry.sampler_state_count; s++) {
            const MOJOSHADER_effectSamplerState &state = entry.sampler_states[s];

            if (state.type == MOJOSHADER_SAMP_TEXTURE) {
                uint32_t object = value_bits(state.value);

                if (static_cast<int>(object) < effect->fx->object_count && effect->textures[object] != nullptr) {
                    gl_object *texture = effect->textures[object];

                    texture->refs++;
                    if (g_pipe.texture[unit] != nullptr) {
                        gl_device().release(g_pipe.texture[unit]);
                    }
                    g_pipe.texture[unit] = texture;
                }
            } else {
                int number = sampler_state_number(state.type);

                if (number >= 0) {
                    g_pipe.sampler_state[unit][number] = value_bits(state.value);
                }
            }
        }
    }
}

void apply_pass_state(gl_effect *effect)
{
    const MOJOSHADER_effectStateChanges &changes = effect->changes;

    for (unsigned int i = 0; i < changes.render_state_change_count; i++) {
        const MOJOSHADER_effectState &state = changes.render_state_changes[i];
        int number = render_state_number(state.type);

        if (number >= 0) {
            g_pipe.render_state[number] = value_bits(state.value);
        }
    }
    apply_samplers(effect, changes.sampler_state_change_count, changes.sampler_state_changes);
}

gl_effect *as_effect(d3d_arg object)
{
    return static_cast<gl_effect *>(object.get());
}

}  // namespace

namespace gl {

void effect_destroy(gl_object *object)
{
    gl_effect *effect = static_cast<gl_effect *>(object);

    if (effect->fx != nullptr) {
        for (int i = 0; i < effect->fx->object_count; i++) {
            if (effect->textures[i] != nullptr) {
                gl_device().release(effect->textures[i]);
            }
        }
        MOJOSHADER_deleteEffect(effect->fx);
    }
    free(effect->textures);
    free(effect);
}

}  // namespace gl

int32_t gl_create_effect(const void *data, uint32_t size, void *out_effect)
{
    MOJOSHADER_effectShaderContext context;
    gl_effect *effect = new_object<gl_effect>(kind_effect);

    memset(&context, 0, sizeof(context));
    context.compileShader = backend_compile;
    context.shaderAddRef = backend_add_ref;
    context.deleteShader = backend_delete;
    context.getParseData = backend_parse_data;
    context.bindShaders = backend_bind;
    context.getBoundShaders = backend_get_bound;
    context.mapUniformBufferMemory = backend_map;
    context.unmapUniformBufferMemory = backend_unmap;
    context.getError = backend_error;
    effect->fx = MOJOSHADER_compileEffect(static_cast<const unsigned char *>(data), size, nullptr, 0, nullptr, 0, &context);
    if (effect->fx == nullptr || effect->fx->error_count > 0) {
        halo::shell::standalone_log("gl: effect failed to compile: %s", effect->fx != nullptr && effect->fx->errors != nullptr ? effect->fx->errors[0].error : "(null)");
        effect->fx = nullptr;
    } else {
        effect->textures = static_cast<gl_object **>(calloc(static_cast<size_t>(effect->fx->object_count) + 1, sizeof(gl_object *)));
    }
    *static_cast<void **>(out_effect) = effect;
    return 0;
}

/* ---- ID3DXEffect ---- */

int32_t GlDevice::effect_set_vector(d3d_arg object, d3d_arg handle, d3d_arg vector)
{
    gl_effect *effect = as_effect(object);
    const MOJOSHADER_effectParam *param = static_cast<const MOJOSHADER_effectParam *>(handle.get());

    if (effect->fx != nullptr && param != nullptr && vector.get() != nullptr) {
        uint32_t elements = param->value.type.elements > 0 ? param->value.type.elements : 1;
        uint32_t size = param->value.type.columns * param->value.type.rows * elements * 4;

        MOJOSHADER_effectSetRawValueHandle(param, vector.get(), 0, size < 16 ? size : 16);
    }
    return 0;
}

int32_t GlDevice::effect_set_texture(d3d_arg object, d3d_arg handle, d3d_arg texture)
{
    gl_effect *effect = as_effect(object);
    const MOJOSHADER_effectParam *param = static_cast<const MOJOSHADER_effectParam *>(handle.get());
    gl_object *incoming = static_cast<gl_object *>(texture.get());

    if (effect->fx != nullptr && param != nullptr && param->value.values != nullptr) {
        uint32_t index = static_cast<uint32_t>(param->value.valuesI[0]);

        if (static_cast<int>(index) < effect->fx->object_count) {
            if (incoming != nullptr) {
                incoming->refs++;
            }
            if (effect->textures[index] != nullptr) {
                gl_device().release(effect->textures[index]);
            }
            effect->textures[index] = incoming;
        }
    }
    return 0;
}

int32_t GlDevice::effect_set_technique(d3d_arg object, d3d_arg technique)
{
    gl_effect *effect = as_effect(object);

    if (effect->fx != nullptr && technique.get() != nullptr) {
        MOJOSHADER_effectSetTechnique(effect->fx, static_cast<const MOJOSHADER_effectTechnique *>(technique.get()));
    }
    return 0;
}

int32_t GlDevice::effect_validate_technique(d3d_arg, d3d_arg)
{
    return 0;
}

int32_t GlDevice::effect_find_next_valid_technique(d3d_arg object, d3d_arg technique, d3d_arg out_technique)
{
    gl_effect *effect = as_effect(object);
    const MOJOSHADER_effectTechnique *next = nullptr;

    if (effect->fx != nullptr) {
        next = MOJOSHADER_effectFindNextValidTechnique(effect->fx, static_cast<const MOJOSHADER_effectTechnique *>(technique.get()));
    }
    *static_cast<const void **>(out_technique.get()) = next;
    return next != nullptr ? 0 : static_cast<int32_t>(0x88760b59);
}

int32_t GlDevice::effect_begin(d3d_arg object, d3d_arg out_pass_count, uint32_t)
{
    gl_effect *effect = as_effect(object);
    unsigned int passes = 0;

    if (effect->fx != nullptr) {
        MOJOSHADER_effectBegin(effect->fx, &passes, 0, &effect->changes);
        effect->begun = true;
    }
    *static_cast<uint32_t *>(out_pass_count.get()) = passes;
    return 0;
}

int32_t GlDevice::effect_pass(d3d_arg object, uint32_t pass)
{
    gl_effect *effect = as_effect(object);

    if (effect->fx == nullptr || !effect->begun) {
        return 0;
    }
    if (effect->pass_active) {
        MOJOSHADER_effectEndPass(effect->fx);
    }
    MOJOSHADER_effectBeginPass(effect->fx, pass);
    effect->pass_active = true;
    apply_pass_state(effect);
    if ((g_pipe.pixel_shader != nullptr && g_pipe.pixel_shader->parse == &g_placeholder_parse) || (g_pipe.vertex_shader != nullptr && g_pipe.vertex_shader->parse == &g_placeholder_parse)) {
        static const void *reported[32];
        static int reported_count;
        const void *key = effect->fx->current_technique;
        bool seen = false;

        for (int i = 0; i < reported_count; i++) seen = seen || reported[i] == key;
        if (!seen && reported_count < 32) {
            reported[reported_count++] = key;
            halo::shell::standalone_log("gl: effect technique '%s' pass %u binds a shader without bytecode (vs %d ps %d)", effect->fx->current_technique->name, pass,
                g_pipe.vertex_shader != nullptr && g_pipe.vertex_shader->parse == &g_placeholder_parse, g_pipe.pixel_shader != nullptr && g_pipe.pixel_shader->parse == &g_placeholder_parse);
        }
    }
    return 0;
}

int32_t GlDevice::effect_end(d3d_arg object)
{
    gl_effect *effect = as_effect(object);

    if (effect->fx == nullptr || !effect->begun) {
        return 0;
    }
    if (effect->pass_active) {
        MOJOSHADER_effectEndPass(effect->fx);
        effect->pass_active = false;
    }
    MOJOSHADER_effectEnd(effect->fx);
    effect->begun = false;
    return 0;
}

int32_t GlDevice::effect_get_parameter_by_name(d3d_arg object, d3d_arg, d3d_arg name)
{
    gl_effect *effect = as_effect(object);
    const char *wanted = static_cast<const char *>(name.get());

    if (effect->fx != nullptr && wanted != nullptr) {
        for (int i = 0; i < effect->fx->param_count; i++) {
            const char *candidate = effect->fx->params[i].value.name;

            if (candidate != nullptr && strcmp(candidate, wanted) == 0) {
                return static_cast<int32_t>(reinterpret_cast<uintptr_t>(&effect->fx->params[i]));
            }
        }
    }
    return 0;
}

int32_t GlDevice::effect_get_technique_by_name(d3d_arg object, d3d_arg name)
{
    gl_effect *effect = as_effect(object);
    const char *wanted = static_cast<const char *>(name.get());

    if (effect->fx != nullptr && wanted != nullptr) {
        for (int i = 0; i < effect->fx->technique_count; i++) {
            if (effect->fx->techniques[i].name != nullptr && strcmp(effect->fx->techniques[i].name, wanted) == 0) {
                return static_cast<int32_t>(reinterpret_cast<uintptr_t>(&effect->fx->techniques[i]));
            }
        }
    }
    return 0;
}

int32_t GlDevice::effect_get_technique_by_name_scoped(d3d_arg object, d3d_arg, d3d_arg name)
{
    return effect_get_technique_by_name(object, name);
}

}  // namespace halo::rasterizer
