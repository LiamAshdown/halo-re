/**
 * @file src/rasterizer/gl_draw.cpp
 * Drawing for the OpenGL backend: texture and buffer uploads, render targets, Direct3D render/sampler state mapped to GL
 * state, shader programs (MojoShader translations of the game's vs/ps bytecode, plus generated shaders that stand in for
 * the fixed-function pipeline), and the draw calls themselves.
 */

#include "gl_internal.hpp"
#include "halo/shell/standalone.hpp"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

namespace halo::rasterizer::gl {

namespace {

/* ---- small helpers ---- */

struct sbuf {
    char *text;
    size_t length;
    size_t capacity;
};

void sb_printf(sbuf &buffer, const char *format, ...)
{
    char line[1024];
    va_list args;
    size_t added;

    va_start(args, format);
    vsnprintf(line, sizeof(line), format, args);
    va_end(args);
    added = strlen(line);
    if (buffer.length + added + 1 > buffer.capacity) {
        buffer.capacity = (buffer.length + added + 1) * 2 + 1024;
        buffer.text = static_cast<char *>(realloc(buffer.text, buffer.capacity));
    }
    memcpy(buffer.text + buffer.length, line, added + 1);
    buffer.length += added;
}

void sb_free(sbuf &buffer)
{
    free(buffer.text);
    buffer = sbuf{};
}

uint64_t hash_words(const uint32_t *words, size_t count)
{
    uint64_t hash = 1469598103934665603ull;

    for (size_t i = 0; i < count; i++) {
        hash = (hash ^ words[i]) * 1099511628211ull;
    }
    return hash;
}

bool trace_enabled_for_programs()
{
    static const int enabled = []() {
        const char *value = getenv("HALO_GL_PROGRAMS");

        return value != nullptr && value[0] != '0' ? 1 : 0;
    }();

    return enabled != 0;
}

/** cos(x) for 0 <= x <= pi (half spot-cone angles): a short Taylor series is plenty for a lighting cutoff. */
float cosine(float x)
{
    float x2 = x * x;

    return 1.0f - x2 / 2.0f + x2 * x2 / 24.0f - x2 * x2 * x2 / 720.0f + x2 * x2 * x2 * x2 / 40320.0f - x2 * x2 * x2 * x2 * x2 / 3628800.0f;
}

uint32_t g_warned[8];

/** Logs a message the first time a given feature is hit (feature < 256). */
void warn_once(uint32_t feature, const char *message)
{
    uint32_t &word = g_warned[(feature >> 5) & 7];

    if ((word & (1u << (feature & 31))) == 0) {
        word |= 1u << (feature & 31);
        halo::shell::standalone_log("gl: %s", message);
    }
}

/* ---- Direct3D 9 constants ---- */

enum : uint32_t {
    D3DRS_ZENABLE = 7, D3DRS_FILLMODE = 8, D3DRS_ZWRITEENABLE = 14, D3DRS_ALPHATESTENABLE = 15, D3DRS_SRCBLEND = 19, D3DRS_DESTBLEND = 20,
    D3DRS_CULLMODE = 22, D3DRS_ZFUNC = 23, D3DRS_ALPHAREF = 24, D3DRS_ALPHAFUNC = 25, D3DRS_ALPHABLENDENABLE = 27, D3DRS_FOGENABLE = 28,
    D3DRS_SPECULARENABLE = 29, D3DRS_FOGCOLOR = 34, D3DRS_FOGTABLEMODE = 35, D3DRS_FOGSTART = 36, D3DRS_FOGEND = 37, D3DRS_FOGDENSITY = 38,
    D3DRS_STENCILENABLE = 52, D3DRS_STENCILFAIL = 53, D3DRS_STENCILZFAIL = 54, D3DRS_STENCILPASS = 55, D3DRS_STENCILFUNC = 56,
    D3DRS_STENCILREF = 57, D3DRS_STENCILMASK = 58, D3DRS_STENCILWRITEMASK = 59, D3DRS_TEXTUREFACTOR = 60, D3DRS_LIGHTING = 137,
    D3DRS_AMBIENT = 139, D3DRS_FOGVERTEXMODE = 140, D3DRS_COLORVERTEX = 141, D3DRS_LOCALVIEWER = 142, D3DRS_NORMALIZENORMALS = 143,
    D3DRS_DIFFUSEMATERIALSOURCE = 145, D3DRS_SPECULARMATERIALSOURCE = 146, D3DRS_AMBIENTMATERIALSOURCE = 147, D3DRS_EMISSIVEMATERIALSOURCE = 148,
    D3DRS_COLORWRITEENABLE = 168, D3DRS_BLENDOP = 171, D3DRS_SLOPESCALEDEPTHBIAS = 175, D3DRS_DEPTHBIAS = 195,
    D3DRS_SEPARATEALPHABLENDENABLE = 206, D3DRS_SRCBLENDALPHA = 207, D3DRS_DESTBLENDALPHA = 208, D3DRS_BLENDOPALPHA = 209,
};

enum : uint32_t {
    TSS_COLOROP = 1, TSS_COLORARG1 = 2, TSS_COLORARG2 = 3, TSS_ALPHAOP = 4, TSS_ALPHAARG1 = 5, TSS_ALPHAARG2 = 6,
    TSS_TEXCOORDINDEX = 11, TSS_TEXTURETRANSFORMFLAGS = 24, TSS_COLORARG0 = 26, TSS_ALPHAARG0 = 27, TSS_RESULTARG = 28, TSS_CONSTANT = 32,
};

enum : uint32_t { SAMP_ADDRESSU = 1, SAMP_ADDRESSV = 2, SAMP_ADDRESSW = 3, SAMP_BORDERCOLOR = 4, SAMP_MAGFILTER = 5, SAMP_MINFILTER = 6,
    SAMP_MIPFILTER = 7, SAMP_MAXMIPLEVEL = 9, SAMP_MAXANISOTROPY = 10 };

constexpr uint32_t k_fvf_position_mask = 0x00e;
constexpr uint32_t k_fvf_xyz = 0x002;
constexpr uint32_t k_fvf_xyzrhw = 0x004;
constexpr uint32_t k_fvf_normal = 0x010;
constexpr uint32_t k_fvf_psize = 0x020;
constexpr uint32_t k_fvf_diffuse = 0x040;
constexpr uint32_t k_fvf_specular = 0x080;

/* ---- generated vertex declaration elements (D3DVERTEXELEMENT9: stream, offset, type, method, usage, usage index) ---- */

struct element {
    uint32_t stream;
    uint32_t offset;
    uint32_t type;
    uint32_t usage;
    uint32_t index;
};

enum : uint32_t {
    USAGE_POSITION = 0, USAGE_BLENDWEIGHT = 1, USAGE_BLENDINDICES = 2, USAGE_NORMAL = 3, USAGE_PSIZE = 4, USAGE_TEXCOORD = 5,
    USAGE_TANGENT = 6, USAGE_BINORMAL = 7, USAGE_POSITIONT = 9, USAGE_COLOR = 10,
};

/** Attribute slot a (usage, index) pair is bound to; every generated and translated vertex shader uses this mapping. */
int attribute_slot(uint32_t usage, uint32_t index)
{
    switch (usage) {
    case USAGE_POSITION: case USAGE_POSITIONT: return index == 0 ? 0 : -1;
    case USAGE_BLENDWEIGHT: return index == 0 ? 1 : -1;
    case USAGE_BLENDINDICES: return index == 0 ? 2 : -1;
    // ponytail: normal1 (the lightmap stream's incident radiosity vector) shares texcoord7's slot; no Halo declaration has both
    case USAGE_NORMAL: return index == 0 ? 3 : (index == 1 ? 13 : -1);
    case USAGE_COLOR: return index < 2 ? 4 + static_cast<int>(index) : -1;
    case USAGE_TEXCOORD: return index < 8 ? 6 + static_cast<int>(index) : -1;
    case USAGE_TANGENT: return index == 0 ? 14 : -1;
    case USAGE_BINORMAL: return index == 0 ? 15 : -1;
    default: return -1;
    }
}

constexpr int k_attribute_slots = 16;

uint32_t fvf_vertex_size(uint32_t fvf);

/** Builds the element list of an FVF into out (returns the count). */
uint32_t fvf_elements(uint32_t fvf, element *out)
{
    uint32_t count = 0;
    uint32_t offset = 0;
    uint32_t position = fvf & k_fvf_position_mask;
    uint32_t texture_count = (fvf >> 8) & 0xf;

    if (position == k_fvf_xyzrhw) {
        out[count++] = {0, offset, 3, USAGE_POSITIONT, 0};
        offset += 16;
    } else if (position >= k_fvf_xyz) {
        uint32_t blend_weights = position == k_fvf_xyz ? 0 : (position - k_fvf_xyz) / 2;  // XYZB1..5 carry 1..5 weights... (XYZ=2, XYZB1=6)

        out[count++] = {0, offset, 2, USAGE_POSITION, 0};
        offset += 12;
        if (position >= 0x6) {
            blend_weights = (position - 0x4) / 2;  // 0x6 -> 1 weight ... 0xe -> 5 weights
            out[count++] = {0, offset, blend_weights - 1, USAGE_BLENDWEIGHT, 0};
            offset += blend_weights * 4;
        }
    }
    if ((fvf & k_fvf_normal) != 0) {
        out[count++] = {0, offset, 2, USAGE_NORMAL, 0};
        offset += 12;
    }
    if ((fvf & k_fvf_psize) != 0) {
        out[count++] = {0, offset, 0, USAGE_PSIZE, 0};
        offset += 4;
    }
    if ((fvf & k_fvf_diffuse) != 0) {
        out[count++] = {0, offset, 4, USAGE_COLOR, 0};
        offset += 4;
    }
    if ((fvf & k_fvf_specular) != 0) {
        out[count++] = {0, offset, 4, USAGE_COLOR, 1};
        offset += 4;
    }
    for (uint32_t i = 0; i < texture_count && i < 8; i++) {
        uint32_t format = (fvf >> (16 + i * 2)) & 3;  // 0: 2D, 1: 3D, 2: 4D, 3: 1D
        static const uint32_t types[4] = {1, 2, 3, 0};
        static const uint32_t sizes[4] = {8, 12, 16, 4};

        out[count++] = {0, offset, types[format], USAGE_TEXCOORD, i};
        offset += sizes[format];
    }
    return count;
}

uint32_t fvf_vertex_size(uint32_t fvf)
{
    element elements[24];
    uint32_t count = fvf_elements(fvf, elements);
    uint32_t end = 0;

    for (uint32_t i = 0; i < count; i++) {
        static const uint32_t sizes[17] = {4, 8, 12, 16, 4, 4, 4, 8, 4, 4, 8, 4, 8, 4, 4, 4, 8};
        uint32_t size = elements[i].type < 17 ? sizes[elements[i].type] : 4;

        if (elements[i].offset + size > end) {
            end = elements[i].offset + size;
        }
    }
    return end;
}

/* ---- global GL objects ---- */

GLuint g_vao;
GLuint g_white_texture;
GLuint g_white_cube;
GLuint g_scratch_vertices;
GLuint g_scratch_indices;
uint32_t g_enabled_attributes;  // bit per attribute slot with an array enabled
GLuint g_bound_framebuffer = 0xffffffff;
uint32_t g_target_width;
uint32_t g_target_height;
bool g_target_flipped;  // rendering into a texture: rows are mirrored so that row 0 is the top as in Direct3D
GLuint g_bound_program = 0xffffffff;
uint32_t g_applied_state[256];
bool g_applied_valid;
uint32_t g_bound_texture_name[16];
uint32_t g_bound_texture_target[16];

void set_defaults()
{
    memset(&g_pipe, 0, sizeof(g_pipe));
    g_pipe.render_state[D3DRS_ZENABLE] = 1;
    g_pipe.render_state[D3DRS_FILLMODE] = 3;
    g_pipe.render_state[D3DRS_ZWRITEENABLE] = 1;
    g_pipe.render_state[D3DRS_SRCBLEND] = 2;
    g_pipe.render_state[D3DRS_DESTBLEND] = 1;
    g_pipe.render_state[D3DRS_CULLMODE] = 3;
    g_pipe.render_state[D3DRS_ZFUNC] = 4;
    g_pipe.render_state[D3DRS_ALPHAFUNC] = 8;
    g_pipe.render_state[D3DRS_STENCILFAIL] = 1;
    g_pipe.render_state[D3DRS_STENCILZFAIL] = 1;
    g_pipe.render_state[D3DRS_STENCILPASS] = 1;
    g_pipe.render_state[D3DRS_STENCILFUNC] = 8;
    g_pipe.render_state[D3DRS_STENCILMASK] = 0xffffffff;
    g_pipe.render_state[D3DRS_STENCILWRITEMASK] = 0xffffffff;
    g_pipe.render_state[D3DRS_TEXTUREFACTOR] = 0xffffffff;
    g_pipe.render_state[D3DRS_LIGHTING] = 1;
    g_pipe.render_state[D3DRS_COLORVERTEX] = 1;
    g_pipe.render_state[D3DRS_DIFFUSEMATERIALSOURCE] = 1;
    g_pipe.render_state[D3DRS_SPECULARMATERIALSOURCE] = 2;
    g_pipe.render_state[D3DRS_COLORWRITEENABLE] = 0xf;
    g_pipe.render_state[D3DRS_BLENDOP] = 1;
    g_pipe.render_state[D3DRS_SRCBLENDALPHA] = 2;
    g_pipe.render_state[D3DRS_DESTBLENDALPHA] = 1;
    g_pipe.render_state[D3DRS_BLENDOPALPHA] = 1;
    g_pipe.render_state[D3DRS_FOGSTART] = 0;
    g_pipe.render_state[D3DRS_FOGEND] = 0x3f800000;
    g_pipe.render_state[D3DRS_FOGDENSITY] = 0x3f800000;
    for (int stage = 0; stage < 8; stage++) {
        g_pipe.texture_stage_state[stage][TSS_COLOROP] = stage == 0 ? 4 : 1;
        g_pipe.texture_stage_state[stage][TSS_COLORARG1] = 2;
        g_pipe.texture_stage_state[stage][TSS_COLORARG2] = 1;
        g_pipe.texture_stage_state[stage][TSS_ALPHAOP] = stage == 0 ? 2 : 1;
        g_pipe.texture_stage_state[stage][TSS_ALPHAARG1] = 2;
        g_pipe.texture_stage_state[stage][TSS_ALPHAARG2] = 1;
        g_pipe.texture_stage_state[stage][TSS_TEXCOORDINDEX] = static_cast<uint32_t>(stage);
    }
    for (int sampler = 0; sampler < 16; sampler++) {
        g_pipe.sampler_state[sampler][SAMP_ADDRESSU] = 1;
        g_pipe.sampler_state[sampler][SAMP_ADDRESSV] = 1;
        g_pipe.sampler_state[sampler][SAMP_ADDRESSW] = 1;
        g_pipe.sampler_state[sampler][SAMP_MAGFILTER] = 1;
        g_pipe.sampler_state[sampler][SAMP_MINFILTER] = 1;
        g_pipe.sampler_state[sampler][SAMP_MAXANISOTROPY] = 1;
    }
    for (int state = 0; state < 512; state++) {
        float *m = g_pipe.transform[state];

        m[0] = m[5] = m[10] = m[15] = 1.0f;
    }
}

}  // namespace

/* ======================================================================================================== */
/* Textures                                                                                                  */
/* ======================================================================================================== */

namespace {

GLenum texture_target(const gl_texture *texture)
{
    return texture->kind == kind_cube_texture ? GL_TEXTURE_CUBE_MAP : texture->kind == kind_volume_texture ? GL_TEXTURE_3D : GL_TEXTURE_2D;
}

constexpr GLenum k_gl_half_float = 0x140B;
constexpr GLenum k_gl_r16f = 0x822D, k_gl_rg16f = 0x822F, k_gl_r32f = 0x822E, k_gl_rg32f = 0x8230, k_gl_rgba32f = 0x8814;
constexpr GLenum k_gl_rgba8_snorm = 0x8F97, k_gl_rg8_snorm = 0x8F95;

/** Converts a width x height image of a legacy D3D format to RGBA8 (returns false if the format is not one of those). */
bool convert_to_rgba(uint32_t format, const uint8_t *source, uint32_t width, uint32_t height, uint32_t pitch, uint8_t *out)
{
    for (uint32_t y = 0; y < height; y++) {
        const uint8_t *row = source + static_cast<size_t>(y) * pitch;
        uint8_t *dest = out + static_cast<size_t>(y) * width * 4;

        for (uint32_t x = 0; x < width; x++, dest += 4) {
            uint32_t r = 0, g = 0, b = 0, a = 255;

            switch (format) {
            case 20: b = row[x * 3]; g = row[x * 3 + 1]; r = row[x * 3 + 2]; break;
            case 21: case 22: b = row[x * 4]; g = row[x * 4 + 1]; r = row[x * 4 + 2]; a = format == 21 ? row[x * 4 + 3] : 255; break;
            case 23: {
                uint32_t v = reinterpret_cast<const uint16_t *>(row)[x];

                r = ((v >> 11) & 31) * 255 / 31; g = ((v >> 5) & 63) * 255 / 63; b = (v & 31) * 255 / 31;
                break;
            }
            case 24: case 25: {
                uint32_t v = reinterpret_cast<const uint16_t *>(row)[x];

                r = ((v >> 10) & 31) * 255 / 31; g = ((v >> 5) & 31) * 255 / 31; b = (v & 31) * 255 / 31;
                a = format == 25 ? ((v >> 15) & 1) * 255 : 255;
                break;
            }
            case 26: {
                uint32_t v = reinterpret_cast<const uint16_t *>(row)[x];

                a = ((v >> 12) & 15) * 17; r = ((v >> 8) & 15) * 17; g = ((v >> 4) & 15) * 17; b = (v & 15) * 17;
                break;
            }
            case 28: r = g = b = 0; a = row[x]; break;
            case 41: case 50: r = g = b = row[x]; break;
            case 51: r = g = b = row[x * 2]; a = row[x * 2 + 1]; break;
            case 52: r = g = b = (row[x] & 15) * 17; a = (row[x] >> 4) * 17; break;
            default: return false;
            }
            dest[0] = static_cast<uint8_t>(r); dest[1] = static_cast<uint8_t>(g); dest[2] = static_cast<uint8_t>(b); dest[3] = static_cast<uint8_t>(a);
        }
    }
    return true;
}

struct gl_format {
    GLenum internal_format;
    GLenum format;
    GLenum type;
    bool compressed;
    bool convert;  // CPU-convert to RGBA8 first
};

gl_format format_for(uint32_t d3d_format)
{
    switch (d3d_format) {
    case k_fourcc_dxt1: return {GL_COMPRESSED_RGBA_S3TC_DXT1_EXT, 0, 0, true, false};
    case k_fourcc_dxt2: case k_fourcc_dxt3: return {GL_COMPRESSED_RGBA_S3TC_DXT3_EXT, 0, 0, true, false};
    case k_fourcc_dxt4: case k_fourcc_dxt5: return {GL_COMPRESSED_RGBA_S3TC_DXT5_EXT, 0, 0, true, false};
    case 21: return {GL_RGBA8, GL_BGRA, GL_UNSIGNED_INT_8_8_8_8_REV, false, false};
    case 60: return {k_gl_rg8_snorm, GL_RG, GL_BYTE, false, false};
    case 63: return {k_gl_rgba8_snorm, GL_RGBA, GL_BYTE, false, false};
    case 111: return {k_gl_r16f, GL_RED, k_gl_half_float, false, false};
    case 112: return {k_gl_rg16f, GL_RG, k_gl_half_float, false, false};
    case 113: return {GL_RGBA16F, GL_RGBA, k_gl_half_float, false, false};
    case 114: return {k_gl_r32f, GL_RED, GL_FLOAT, false, false};
    case 115: return {k_gl_rg32f, GL_RG, GL_FLOAT, false, false};
    case 116: return {k_gl_rgba32f, GL_RGBA, GL_FLOAT, false, false};
    default: return {GL_RGBA8, GL_RGBA, GL_UNSIGNED_BYTE, false, true};
    }
}

void upload_texture(gl_texture *texture)
{
    GLenum target = texture_target(texture);
    gl_format format = format_for(texture->format);
    bool render_target = (texture->usage & k_usage_render_target) != 0;

    if (texture->name == 0) {
        glGenTextures(1, &texture->name);
    }
    glActiveTexture(GL_TEXTURE0 + 15);
    glBindTexture(target, texture->name);
    g_bound_texture_name[15] = 0;
    texture->sampler_hash = 0;
    texture->dirty = false;

    if (render_target) {
        // storage only; the contents come from drawing
        uint32_t faces = texture->faces;

        glTexParameteri(target, GL_TEXTURE_MAX_LEVEL, 0);
        for (uint32_t face = 0; face < faces; face++) {
            GLenum face_target = target == GL_TEXTURE_CUBE_MAP ? GL_TEXTURE_CUBE_MAP_POSITIVE_X + face : target;

            glTexImage2D(face_target, 0, static_cast<GLint>(format.internal_format), static_cast<GLsizei>(texture->width),
                static_cast<GLsizei>(texture->height), 0, format.convert ? GL_RGBA : format.format, format.convert ? GL_UNSIGNED_BYTE : format.type, nullptr);
        }
        return;
    }
    glTexParameteri(target, GL_TEXTURE_MAX_LEVEL, static_cast<GLint>(texture->levels - 1));
    for (uint32_t face = 0; face < texture->faces; face++) {
        for (uint32_t i = 0; i < texture->levels; i++) {
            const gl_level &level = texture->level[face * k_max_levels + i];
            GLenum face_target = target == GL_TEXTURE_CUBE_MAP ? GL_TEXTURE_CUBE_MAP_POSITIVE_X + face : target;
            uint8_t *converted = nullptr;
            const void *pixels = level.data;

            if (format.convert) {
                converted = static_cast<uint8_t *>(malloc(static_cast<size_t>(level.width) * level.height * level.depth * 4 + 4));
                for (uint32_t z = 0; z < level.depth; z++) {
                    convert_to_rgba(texture->format, level.data + static_cast<size_t>(z) * level.slice, level.width, level.height, level.pitch,
                        converted + static_cast<size_t>(z) * level.width * level.height * 4);
                }
                pixels = converted;
            }
            if (target == GL_TEXTURE_3D) {
                glTexImage3D(target, static_cast<GLint>(i), static_cast<GLint>(format.internal_format), static_cast<GLsizei>(level.width),
                    static_cast<GLsizei>(level.height), static_cast<GLsizei>(level.depth), 0, format.format, format.type, pixels);
            } else if (format.compressed) {
                uint32_t block_rows = (level.height + 3) / 4 > 0 ? (level.height + 3) / 4 : 1;

                glCompressedTexImage2D(face_target, static_cast<GLint>(i), format.internal_format, static_cast<GLsizei>(level.width),
                    static_cast<GLsizei>(level.height), 0, static_cast<GLsizei>(level.pitch * block_rows), pixels);
            } else {
                glTexImage2D(face_target, static_cast<GLint>(i), static_cast<GLint>(format.internal_format), static_cast<GLsizei>(level.width),
                    static_cast<GLsizei>(level.height), 0, format.convert ? GL_RGBA : format.format, format.convert ? GL_UNSIGNED_BYTE : format.type, pixels);
            }
            free(converted);
        }
    }
}

void make_default_textures()
{
    static const uint8_t white[4] = {0, 0, 0, 255};  // Direct3D 9 samples an unbound texture as opaque black

    glGenTextures(1, &g_white_texture);
    glActiveTexture(GL_TEXTURE0 + 15);
    glBindTexture(GL_TEXTURE_2D, g_white_texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, white);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glGenTextures(1, &g_white_cube);
    glBindTexture(GL_TEXTURE_CUBE_MAP, g_white_cube);
    for (uint32_t face = 0; face < 6; face++) {
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + face, 0, GL_RGBA8, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, white);
    }
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
}

GLenum wrap_mode(uint32_t address)
{
    switch (address) {
    case 2: return GL_MIRRORED_REPEAT;
    case 3: return GL_CLAMP_TO_EDGE;
    case 4: return GL_CLAMP_TO_BORDER;
    case 5: return GL_MIRRORED_REPEAT;
    default: return GL_REPEAT;
    }
}

void apply_sampler(gl_texture *texture, uint32_t unit)
{
    const uint32_t *state = g_pipe.sampler_state[unit];
    GLenum target = texture_target(texture);
    uint32_t words[10] = {state[SAMP_ADDRESSU], state[SAMP_ADDRESSV], state[SAMP_ADDRESSW], state[SAMP_MAGFILTER], state[SAMP_MINFILTER],
        state[SAMP_MIPFILTER], state[SAMP_MAXMIPLEVEL], state[SAMP_MAXANISOTROPY], state[SAMP_BORDERCOLOR], texture->levels};
    uint32_t hash = static_cast<uint32_t>(hash_words(words, 10));
    GLenum min_filter;
    GLenum mag_filter;
    bool linear_min = state[SAMP_MINFILTER] >= 2;
    bool mip = texture->levels > 1 && state[SAMP_MIPFILTER] != 0;

    if (hash == texture->sampler_hash && hash != 0) {
        return;
    }
    texture->sampler_hash = hash != 0 ? hash : 1;
    if (!mip) {
        min_filter = linear_min ? GL_LINEAR : GL_NEAREST;
    } else if (state[SAMP_MIPFILTER] >= 2) {
        min_filter = linear_min ? GL_LINEAR_MIPMAP_LINEAR : GL_NEAREST_MIPMAP_LINEAR;
    } else {
        min_filter = linear_min ? GL_LINEAR_MIPMAP_NEAREST : GL_NEAREST_MIPMAP_NEAREST;
    }
    mag_filter = state[SAMP_MAGFILTER] >= 2 ? GL_LINEAR : GL_NEAREST;
    glTexParameteri(target, GL_TEXTURE_MIN_FILTER, static_cast<GLint>(min_filter));
    glTexParameteri(target, GL_TEXTURE_MAG_FILTER, static_cast<GLint>(mag_filter));
    glTexParameteri(target, GL_TEXTURE_WRAP_S, static_cast<GLint>(wrap_mode(state[SAMP_ADDRESSU])));
    glTexParameteri(target, GL_TEXTURE_WRAP_T, static_cast<GLint>(wrap_mode(state[SAMP_ADDRESSV])));
    if (target != GL_TEXTURE_2D) {
        glTexParameteri(target, GL_TEXTURE_WRAP_R, static_cast<GLint>(wrap_mode(state[SAMP_ADDRESSW])));
    }
    if (mip) {
        uint32_t base = state[SAMP_MAXMIPLEVEL] < texture->levels ? state[SAMP_MAXMIPLEVEL] : texture->levels - 1;

        glTexParameteri(target, GL_TEXTURE_BASE_LEVEL, static_cast<GLint>(base));
    } else {
        glTexParameteri(target, GL_TEXTURE_BASE_LEVEL, 0);
    }
    if (state[SAMP_MAGFILTER] == 3 || state[SAMP_MINFILTER] == 3) {
        glTexParameterf(target, GL_TEXTURE_MAX_ANISOTROPY_EXT, static_cast<float>(state[SAMP_MAXANISOTROPY] > 1 ? state[SAMP_MAXANISOTROPY] : 1));
    }
    if (state[SAMP_ADDRESSU] == 4 || state[SAMP_ADDRESSV] == 4) {
        uint32_t c = state[SAMP_BORDERCOLOR];
        float color[4] = {static_cast<float>((c >> 16) & 255) / 255.0f, static_cast<float>((c >> 8) & 255) / 255.0f,
            static_cast<float>(c & 255) / 255.0f, static_cast<float>(c >> 24) / 255.0f};

        glTexParameterfv(target, GL_TEXTURE_BORDER_COLOR, color);
    }
}

/** Binds the texture set on a stage to its GL texture unit (white when none) and applies its sampler state. */
void bind_texture_unit(uint32_t unit, GLenum wanted_target)
{
    gl_texture *texture = static_cast<gl_texture *>(g_pipe.texture[unit]);
    GLuint name;
    GLenum target;

    glActiveTexture(GL_TEXTURE0 + unit);
    if (texture == nullptr || (texture->kind != kind_texture && texture->kind != kind_volume_texture && texture->kind != kind_cube_texture)) {
        target = wanted_target;
        name = wanted_target == GL_TEXTURE_CUBE_MAP ? g_white_cube : g_white_texture;
        if (wanted_target == GL_TEXTURE_3D) {
            target = GL_TEXTURE_2D;
            name = g_white_texture;
        }
        glBindTexture(target, name);
        return;
    }
    if (texture->name == 0 || texture->dirty) {
        upload_texture(texture);
        glActiveTexture(GL_TEXTURE0 + unit);
    }
    target = texture_target(texture);
    glBindTexture(target, texture->name);
    apply_sampler(texture, unit);
}

}  // namespace

/* ======================================================================================================== */
/* Render targets                                                                                            */
/* ======================================================================================================== */

namespace {

GLuint make_depth_buffer(uint32_t width, uint32_t height)
{
    GLuint buffer;

    glGenRenderbuffers(1, &buffer);
    glBindRenderbuffer(GL_RENDERBUFFER, buffer);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, static_cast<GLsizei>(width), static_cast<GLsizei>(height));
    return buffer;
}

}  // namespace

GLuint framebuffer_for_target(gl_surface *target, uint32_t *width, uint32_t *height)
{
    gl_texture *texture;

    if (target == nullptr || target->back_buffer || target->parent == nullptr) {
        *width = g_state.width;
        *height = g_state.height;
        return 0;
    }
    texture = target->parent;
    *width = target->width;
    *height = target->height;
    if (texture->name == 0 || texture->dirty) {
        upload_texture(texture);
    }
    if (texture->framebuffer == 0) {
        glGenFramebuffers(1, &texture->framebuffer);
        texture->depth_buffer = make_depth_buffer(texture->width, texture->height);
    }
    glBindFramebuffer(GL_FRAMEBUFFER, texture->framebuffer);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
        texture->kind == kind_cube_texture ? GL_TEXTURE_CUBE_MAP_POSITIVE_X + target->face : GL_TEXTURE_2D, texture->name, static_cast<GLint>(target->level));
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, texture->depth_buffer);
    {
        static uint32_t checked[16];
        static uint32_t checked_count;
        bool seen = false;

        for (uint32_t i = 0; i < checked_count; i++) seen = seen || checked[i] == texture->framebuffer;
        if (!seen && checked_count < 16) {
            GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);

            checked[checked_count++] = texture->framebuffer;
            halo::shell::standalone_log("gl: render target texture format %u %ux%u usage %x -> framebuffer status %04x", texture->format, texture->width, texture->height, texture->usage, status);
        }
    }
    return texture->framebuffer;
}

void bind_render_target(gl_surface *target)
{
    uint32_t width;
    uint32_t height;
    GLuint framebuffer;

    if (!g_state.modern) {
        return;
    }
    framebuffer = framebuffer_for_target(target, &width, &height);
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
    g_bound_framebuffer = framebuffer;
    g_target_width = width;
    g_target_height = height;
    g_target_flipped = framebuffer != 0;
    g_applied_valid = false;  // culling depends on the flip
}

void read_back_surface(gl_surface *surface)
{
    bool from_texture = surface->parent != nullptr && (surface->parent->usage & k_usage_render_target) != 0;
    uint32_t width;
    uint32_t height;
    GLuint framebuffer;
    GLuint previous = g_bound_framebuffer;

    if (!g_state.modern || (!surface->back_buffer && !from_texture) || surface->format != 21 && surface->format != 22) {
        return;
    }
    framebuffer = framebuffer_for_target(surface->back_buffer ? nullptr : surface, &width, &height);
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    glReadPixels(0, 0, static_cast<GLsizei>(width), static_cast<GLsizei>(height), GL_BGRA, GL_UNSIGNED_BYTE, surface->data);
    if (framebuffer == 0) {
        // default framebuffer rows run bottom-up; Direct3D surfaces run top-down
        uint8_t *row = static_cast<uint8_t *>(malloc(surface->pitch));

        for (uint32_t y = 0; y < height / 2; y++) {
            uint8_t *a = surface->data + static_cast<size_t>(y) * surface->pitch;
            uint8_t *b = surface->data + static_cast<size_t>(height - 1 - y) * surface->pitch;

            memcpy(row, a, surface->pitch);
            memcpy(a, b, surface->pitch);
            memcpy(b, row, surface->pitch);
        }
        free(row);
    }
    glBindFramebuffer(GL_FRAMEBUFFER, previous == 0xffffffff ? 0 : previous);
}

void stretch_rect_impl(gl_surface *source, const int32_t *source_rect, gl_surface *dest, const int32_t *dest_rect, uint32_t filter)
{
    bool source_gpu;
    bool dest_gpu;
    int32_t sr[4];
    int32_t dr[4];

    if (source == nullptr || dest == nullptr || !g_state.modern) {
        return;
    }
    source_gpu = source->back_buffer || (source->parent != nullptr && (source->parent->usage & k_usage_render_target) != 0);
    dest_gpu = dest->back_buffer || (dest->parent != nullptr && (dest->parent->usage & k_usage_render_target) != 0);
    if (source_rect != nullptr) {
        memcpy(sr, source_rect, sizeof(sr));
    } else {
        sr[0] = 0; sr[1] = 0; sr[2] = static_cast<int32_t>(source->width); sr[3] = static_cast<int32_t>(source->height);
    }
    if (dest_rect != nullptr) {
        memcpy(dr, dest_rect, sizeof(dr));
    } else {
        dr[0] = 0; dr[1] = 0; dr[2] = static_cast<int32_t>(dest->width); dr[3] = static_cast<int32_t>(dest->height);
    }

    if (source_gpu && dest_gpu) {
        uint32_t sw, sh, dw, dh;
        GLuint source_fb = framebuffer_for_target(source->back_buffer ? nullptr : source, &sw, &sh);
        GLuint dest_fb;
        int32_t sy0 = source_fb == 0 ? static_cast<int32_t>(sh) - sr[1] : sr[1];
        int32_t sy1 = source_fb == 0 ? static_cast<int32_t>(sh) - sr[3] : sr[3];
        int32_t dy0, dy1;

        glBindFramebuffer(GL_READ_FRAMEBUFFER, source_fb);
        dest_fb = framebuffer_for_target(dest->back_buffer ? nullptr : dest, &dw, &dh);
        // framebuffer_for_target leaves the framebuffer it prepared bound to GL_FRAMEBUFFER; make the read binding stick
        glBindFramebuffer(GL_READ_FRAMEBUFFER, source_fb);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, dest_fb);
        dy0 = dest_fb == 0 ? static_cast<int32_t>(dh) - dr[1] : dr[1];
        dy1 = dest_fb == 0 ? static_cast<int32_t>(dh) - dr[3] : dr[3];
        glDisable(GL_SCISSOR_TEST);
        glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
        glBlitFramebuffer(sr[0], sy0, sr[2], sy1, dr[0], dy0, dr[2], dy1, GL_COLOR_BUFFER_BIT, filter == 2 ? GL_LINEAR : GL_NEAREST);
        bind_render_target(g_pipe.render_target);
        return;
    }
    if (source_gpu) {
        // GPU to CPU copy (render-target readback into an offscreen surface or a system-memory texture level)
        read_back_surface(source);
    }
    if (!dest_gpu) {
        uint32_t bpp = bytes_per_pixel(source->format);
        uint32_t copy_w = static_cast<uint32_t>(sr[2] - sr[0]) < static_cast<uint32_t>(dr[2] - dr[0]) ? static_cast<uint32_t>(sr[2] - sr[0]) : static_cast<uint32_t>(dr[2] - dr[0]);
        uint32_t copy_h = static_cast<uint32_t>(sr[3] - sr[1]) < static_cast<uint32_t>(dr[3] - dr[1]) ? static_cast<uint32_t>(sr[3] - sr[1]) : static_cast<uint32_t>(dr[3] - dr[1]);

        if (bpp != 0 && source->format == dest->format) {
            for (uint32_t y = 0; y < copy_h; y++) {
                memcpy(dest->data + static_cast<size_t>(dr[1] + static_cast<int32_t>(y)) * dest->pitch + static_cast<size_t>(dr[0]) * bpp,
                    source->data + static_cast<size_t>(sr[1] + static_cast<int32_t>(y)) * source->pitch + static_cast<size_t>(sr[0]) * bpp, copy_w * bpp);
            }
            if (dest->parent != nullptr) {
                dest->parent->dirty = true;
            }
        }
        return;
    }
    // CPU to GPU: write the pixels into the render-target texture (A8R8G8B8 only)
    if (dest->parent != nullptr && source->format == 21 && dest->parent->name != 0) {
        glActiveTexture(GL_TEXTURE0 + 15);
        glBindTexture(GL_TEXTURE_2D, dest->parent->name);
        g_bound_texture_name[15] = 0;
        glPixelStorei(GL_UNPACK_ROW_LENGTH, static_cast<GLint>(source->pitch / 4));
        glTexSubImage2D(GL_TEXTURE_2D, static_cast<GLint>(dest->level), dr[0], dr[1], sr[2] - sr[0], sr[3] - sr[1], GL_BGRA, GL_UNSIGNED_INT_8_8_8_8_REV,
            source->data + static_cast<size_t>(sr[1]) * source->pitch + static_cast<size_t>(sr[0]) * 4);
        glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
    }
}

/* ======================================================================================================== */
/* Buffers                                                                                                   */
/* ======================================================================================================== */

namespace {

void sync_buffer(gl_buffer *buffer, GLenum target)
{
    if (buffer->name == 0) {
        glGenBuffers(1, &buffer->name);
        buffer->dirty = true;
    }
    glBindBuffer(target, buffer->name);
    if (buffer->dirty) {
        glBufferData(target, static_cast<GLsizeiptr>(buffer->size), buffer->data, GL_DYNAMIC_DRAW);
        buffer->dirty = false;
    }
}

}  // namespace

/* ======================================================================================================== */
/* Shaders                                                                                                   */
/* ======================================================================================================== */

namespace {

uint32_t g_next_shader_id = 1;

/**
 * MojoShader packs a translation's float constants into one uniform array in the order of its uniform list (array
 * uniforms first), not by register number. Copies the registers the shader reads into that order; returns the vec4 count.
 */
uint32_t pack_constants(const MOJOSHADER_parseData *parse, const float (*registers)[4], uint32_t register_count, float *out)
{
    uint32_t packed = 0;

    for (int i = 0; i < parse->uniform_count; i++) {
        const MOJOSHADER_uniform &uniform = parse->uniforms[i];
        uint32_t count = uniform.array_count > 0 ? static_cast<uint32_t>(uniform.array_count) : 1;

        if (uniform.type != MOJOSHADER_UNIFORM_FLOAT || uniform.constant) {
            continue;
        }
        for (uint32_t r = 0; r < count; r++) {
            uint32_t source = static_cast<uint32_t>(uniform.index) + r;

            if (source < register_count) {
                memcpy(out + packed * 4, registers[source], 4 * sizeof(float));
            } else {
                memset(out + packed * 4, 0, 4 * sizeof(float));
            }
            packed++;
        }
    }
    return packed;
}

}  // namespace

void create_shader(gl_shader *shader, const void *function, bool pixel)
{
    shader->id = g_next_shader_id++;
    if (function == nullptr) {
        return;
    }
    // size 0: the translator reads up to the shader's own END token
    shader->parse = MOJOSHADER_parse(MOJOSHADER_PROFILE_GLSL, nullptr, static_cast<const unsigned char *>(function), 0,
        nullptr, 0, nullptr, 0, nullptr, nullptr, nullptr);
    if (shader->parse != nullptr && shader->parse->error_count > 0) {
        halo::shell::standalone_log("gl: %s shader %u failed to translate: %s", pixel ? "pixel" : "vertex", shader->id, shader->parse->errors[0].error);
        MOJOSHADER_freeParseData(shader->parse);
        shader->parse = nullptr;
    }
    if (pixel) {
        keep_ps1_bytecode(shader, function, 0);
    }
}

/**
 * ps_1_x declares no sampler types: Direct3D samples whatever texture is bound, while the translation assumed 2D. Keeps
 * a copy of such a shader's bytecode (size 0: up to its END token) so it can be translated again for other types.
 */
void keep_ps1_bytecode(gl_shader *shader, const void *tokens, uint32_t size)
{
    const uint32_t *words = static_cast<const uint32_t *>(tokens);

    if (shader->parse == nullptr || shader->parse->shader_type != MOJOSHADER_TYPE_PIXEL || shader->parse->major_ver >= 2 || shader->parse->sampler_count == 0) {
        return;
    }
    if (size == 0) {
        uint32_t i = 1;

        while (words[i] != 0x0000ffff) {
            i += (words[i] & 0xffff) == 0xfffe ? 1 + ((words[i] >> 16) & 0x7fff) : 1;  // comments may hold any word
        }
        size = (i + 1) * 4;
    }
    shader->bytecode = static_cast<uint8_t *>(malloc(size));
    memcpy(shader->bytecode, tokens, size);
    shader->bytecode_size = size;
}

/* ======================================================================================================== */
/* Programs                                                                                                  */
/* ======================================================================================================== */

namespace {

constexpr uint32_t k_key_words = 192;

struct program_key {
    uint32_t words[k_key_words];
};

struct program {
    uint64_t hash;
    program_key key;
    GLuint name;
    GLint vs_constants;      // vs_uniforms_vec4
    GLint ps_constants;      // ps_uniforms_vec4
    GLint flip;              // vpFlip
    GLint half_pixel;        // u_half_pixel
    GLint alpha_ref;         // u_alpha_ref
    GLint wvp;               // u_wvp
    GLint world_view;        // u_world_view
    GLint rhw_scale;         // u_rhw_scale
    GLint texture_factor;    // u_texture_factor
    GLint constant_color[8]; // u_constant[i]
    GLint texture_matrix[8]; // u_texmat[i]
    GLint world;             // u_world
    GLint eye;               // u_eye
    GLint ambient;           // u_gambient
    GLint material[5];       // u_mdiffuse, u_mambient, u_mspecular, u_memissive, u_mpower
    GLint light[7];          // u_lpos, u_ldir, u_ldiffuse, u_lspecular, u_lambient, u_latten, u_lspot
    GLint sampler[16];
    const gl_shader *vertex_shader;
    const gl_shader *pixel_shader;
    bool ok;
};

program *g_programs;
uint32_t g_program_count;
uint32_t g_program_capacity;

/* GLSL (version 110 built-in varyings, matching MojoShader's GLSL profile): compile a stage */

GLuint compile_stage(GLenum type, const char *source, const char *what)
{
    GLuint shader = glCreateShader(type);
    GLint status = 0;

    glShaderSource(shader, 1, &source, nullptr);
    glCompileShader(shader);
    glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
    if (status == 0) {
        char log[1024];

        glGetShaderInfoLog(shader, sizeof(log), nullptr, log);
        halo::shell::standalone_log("gl: %s failed to compile: %s", what, log);
        glDeleteShader(shader);
        return 0;
    }
    return shader;
}

/** Replaces the first "void main()" of a MojoShader translation so a wrapper main() can post-process its outputs. */
char *wrap_main(const char *source, const char *wrapper)
{
    const char *main_text = strstr(source, "void main()");
    size_t head;
    char *result;

    if (main_text == nullptr) {
        return nullptr;
    }
    head = static_cast<size_t>(main_text - source);
    result = static_cast<char *>(malloc(strlen(source) + strlen(wrapper) + 64));
    memcpy(result, source, head);
    strcpy(result + head, "void mojo_main()");
    strcat(result, main_text + strlen("void main()"));
    strcat(result, "\n");
    strcat(result, wrapper);
    return result;
}

const char *compare_expression(uint32_t function)
{
    switch (function) {
    case 1: return "false";
    case 2: return "a < r";
    case 3: return "a == r";
    case 4: return "a <= r";
    case 5: return "a > r";
    case 6: return "a != r";
    case 7: return "a >= r";
    default: return "true";
    }
}

void alpha_test_glsl(sbuf &out, uint32_t function, const char *alpha)
{
    if (function == 8 || function == 0 || getenv("HALO_GL_NOALPHATEST") != nullptr) {
        return;
    }
    sb_printf(out, "    { float a = %s; float r = u_alpha_ref; if (!(%s)) discard; }\n", alpha, compare_expression(function));
}

/* ---- the fixed-function vertex stage ---- */

struct ff_vertex_state {
    bool rhw;
    bool has_color0;
    bool has_color1;
    bool has_normal;
    uint32_t texcoord_mask;       // coordinate sets present in the vertex
    uint32_t stage_count;         // stages that sample (up to the first COLOROP = DISABLE)
    uint32_t stage_coord[8];      // coordinate set index each stage reads
    uint32_t stage_texgen[8];     // 0 none, 1 camera-space normal, 2 camera-space position, 3 reflection vector
    uint32_t stage_transform[8];  // D3DTTFF count (0 = off) | 0x100 projected
    bool lighting;
    uint32_t light_type[8];       // 0 off, 1 point, 2 spot, 3 directional
    uint32_t material_source[4];  // diffuse, ambient, specular, emissive: 0 material, 1 vertex colour 1, 2 vertex colour 2
    bool specular;
};

void build_ff_vertex(sbuf &out, const ff_vertex_state &s)
{
    sb_printf(out, "#version 110\n");
    sb_printf(out, "attribute vec4 a_position;\n");
    if (s.has_normal) sb_printf(out, "attribute vec4 a_normal;\n");
    if (s.has_color0) sb_printf(out, "attribute vec4 a_color0;\n");
    if (s.has_color1) sb_printf(out, "attribute vec4 a_color1;\n");
    for (uint32_t i = 0; i < 8; i++) {
        if ((s.texcoord_mask & (1u << i)) != 0) sb_printf(out, "attribute vec4 a_texcoord%u;\n", i);
    }
    sb_printf(out, "uniform mat4 u_wvp;\nuniform mat4 u_world_view;\nuniform vec4 u_rhw_scale;\nuniform float vpFlip;\nuniform vec2 u_half_pixel;\n");
    if (s.lighting) {
        sb_printf(out, "uniform mat4 u_world;\nuniform vec3 u_eye;\nuniform vec4 u_gambient;\nuniform vec4 u_mdiffuse;\nuniform vec4 u_mambient;\n");
        sb_printf(out, "uniform vec4 u_mspecular;\nuniform vec4 u_memissive;\nuniform float u_mpower;\n");
        sb_printf(out, "uniform vec4 u_lpos[8];\nuniform vec4 u_ldir[8];\nuniform vec4 u_ldiffuse[8];\nuniform vec4 u_lspecular[8];\n");
        sb_printf(out, "uniform vec4 u_lambient[8];\nuniform vec4 u_latten[8];\nuniform vec4 u_lspot[8];\n");
    }
    for (uint32_t i = 0; i < s.stage_count; i++) {
        if (s.stage_transform[i] != 0) sb_printf(out, "uniform mat4 u_texmat%u;\n", i);
    }
    for (uint32_t i = 4; i < s.stage_count; i++) sb_printf(out, "varying vec4 io_5_%u;\n", i);
    sb_printf(out, "void main()\n{\n");
    if (s.rhw) {
        // pre-transformed: window coordinates in, mapped straight onto the render target
        sb_printf(out, "    float w = 1.0 / a_position.w;\n");
        sb_printf(out, "    vec2 ndc = a_position.xy * u_rhw_scale.xy + u_rhw_scale.zw;\n");
        sb_printf(out, "    gl_Position = vec4(vec3(ndc, a_position.z * 2.0 - 1.0) * w, w);\n");
    } else {
        sb_printf(out, "    gl_Position = u_wvp * vec4(a_position.xyz, 1.0);\n");
        sb_printf(out, "    gl_Position.y = gl_Position.y * vpFlip;\n");
        sb_printf(out, "    gl_Position.z = gl_Position.z * 2.0 - gl_Position.w;\n");
        sb_printf(out, "    gl_Position.xy += gl_Position.w * u_half_pixel;\n");
    }
    if (s.lighting) {
        static const char *const sources[3][2] = {{"u_mdiffuse", "u_mdiffuse"}, {"vdiffuse", "vdiffuse"}, {"vspecular", "vspecular"}};
        static const char *const names[4] = {"u_mdiffuse", "u_mambient", "u_mspecular", "u_memissive"};
        char material[4][16];

        for (int k = 0; k < 4; k++) {
            snprintf(material[k], sizeof(material[k]), "%s", s.material_source[k] == 0 ? names[k] : sources[s.material_source[k]][0]);
        }
        sb_printf(out, "    vec4 vdiffuse = %s;\n", s.has_color0 ? "a_color0" : "vec4(1.0)");
        sb_printf(out, "    vec4 vspecular = %s;\n", s.has_color1 ? "a_color1" : "vec4(0.0)");
        sb_printf(out, "    vec3 wpos = (u_world * vec4(a_position.xyz, 1.0)).xyz;\n");
        sb_printf(out, "    vec3 wn = normalize(mat3(u_world) * a_normal.xyz);\n");
        sb_printf(out, "    vec3 view_dir = normalize(u_eye - wpos);\n");
        sb_printf(out, "    vec3 diffuse_sum = vec3(0.0);\n    vec3 ambient_sum = vec3(0.0);\n    vec3 specular_sum = vec3(0.0);\n");
        for (int i = 0; i < 8; i++) {
            if (s.light_type[i] == 0) {
                continue;
            }
            sb_printf(out, "    {\n        vec3 L;\n        float att = 1.0;\n        float spot = 1.0;\n");
            if (s.light_type[i] == 3) {
                sb_printf(out, "        L = -normalize(u_ldir[%d].xyz);\n", i);
            } else {
                sb_printf(out, "        vec3 d = u_lpos[%d].xyz - wpos;\n        float dist = length(d);\n        L = d / max(dist, 0.0001);\n", i);
                sb_printf(out, "        att = (dist > u_latten[%d].w) ? 0.0 : 1.0 / max(u_latten[%d].x + u_latten[%d].y * dist + u_latten[%d].z * dist * dist, 0.0001);\n", i, i, i, i);
            }
            if (s.light_type[i] == 2) {
                sb_printf(out, "        float rho = dot(-L, normalize(u_ldir[%d].xyz));\n", i);
                sb_printf(out, "        spot = (rho <= u_lspot[%d].y) ? 0.0 : ((rho > u_lspot[%d].x) ? 1.0 : pow((rho - u_lspot[%d].y) / max(u_lspot[%d].x - u_lspot[%d].y, 0.0001), u_lspot[%d].z));\n", i, i, i, i, i, i);
            }
            sb_printf(out, "        float ndl = max(dot(wn, L), 0.0);\n");
            sb_printf(out, "        diffuse_sum += att * spot * ndl * u_ldiffuse[%d].rgb;\n", i);
            sb_printf(out, "        ambient_sum += att * spot * u_lambient[%d].rgb;\n", i);
            if (s.specular) {
                sb_printf(out, "        if (ndl > 0.0) specular_sum += att * spot * pow(max(dot(wn, normalize(L + view_dir)), 0.0), max(u_mpower, 0.0001)) * u_lspecular[%d].rgb;\n", i);
            }
            sb_printf(out, "    }\n");
        }
        sb_printf(out, "    vec4 mdiff = %s;\n    vec4 mamb = %s;\n    vec4 mspec = %s;\n    vec4 memis = %s;\n", material[0], material[1], material[2], material[3]);
        sb_printf(out, "    gl_FrontColor = vec4(clamp(memis.rgb + mamb.rgb * u_gambient.rgb + ambient_sum * mamb.rgb + diffuse_sum * mdiff.rgb, 0.0, 1.0), mdiff.a);\n");
        sb_printf(out, "    gl_FrontSecondaryColor = vec4(clamp(specular_sum * mspec.rgb, 0.0, 1.0), 0.0);\n");
    } else {
        sb_printf(out, "    gl_FrontColor = %s;\n", s.has_color0 ? "a_color0" : "vec4(1.0)");
        sb_printf(out, "    gl_FrontSecondaryColor = %s;\n", s.has_color1 ? "a_color1" : "vec4(0.0)");
    }
    if (s.stage_texgen[0] != 0 || s.stage_texgen[1] != 0 || s.stage_texgen[2] != 0 || s.stage_texgen[3] != 0 || s.stage_texgen[4] != 0 || s.stage_texgen[5] != 0 ||
        s.stage_texgen[6] != 0 || s.stage_texgen[7] != 0) {
        sb_printf(out, "    vec4 view_position = u_world_view * vec4(a_position.xyz, 1.0);\n");
        if (s.has_normal) {
            sb_printf(out, "    vec3 view_normal = normalize(mat3(u_world_view) * a_normal.xyz);\n");
        } else {
            sb_printf(out, "    vec3 view_normal = vec3(0.0, 0.0, 1.0);\n");
        }
    }
    for (uint32_t i = 0; i < s.stage_count; i++) {
        char source[128];
        const char *target;
        char target_buffer[32];

        if (s.stage_texgen[i] == 1) {
            snprintf(source, sizeof(source), "vec4(view_normal, 1.0)");
        } else if (s.stage_texgen[i] == 2) {
            snprintf(source, sizeof(source), "vec4(view_position.xyz, 1.0)");
        } else if (s.stage_texgen[i] == 3) {
            snprintf(source, sizeof(source), "vec4(reflect(normalize(view_position.xyz), view_normal), 1.0)");
        } else if ((s.texcoord_mask & (1u << s.stage_coord[i])) != 0) {
            snprintf(source, sizeof(source), "a_texcoord%u", s.stage_coord[i]);
        } else {
            snprintf(source, sizeof(source), "vec4(0.0, 0.0, 0.0, 1.0)");
        }
        if (i < 4) {
            snprintf(target_buffer, sizeof(target_buffer), "gl_TexCoord[%u]", i);
        } else {
            snprintf(target_buffer, sizeof(target_buffer), "io_5_%u", i);
        }
        target = target_buffer;
        if (s.stage_transform[i] != 0) {
            uint32_t count = s.stage_transform[i] & 0xff;

            if (count == 1) sb_printf(out, "    %s = u_texmat%u * vec4((%s).x, 1.0, 0.0, 0.0);\n", target, i, source);
            else if (count == 2) sb_printf(out, "    %s = u_texmat%u * vec4((%s).xy, 1.0, 0.0);\n", target, i, source);
            else if (count == 3) sb_printf(out, "    %s = u_texmat%u * vec4((%s).xyz, 1.0);\n", target, i, source);
            else sb_printf(out, "    %s = u_texmat%u * (%s);\n", target, i, source);
        } else {
            sb_printf(out, "    %s = %s;\n", target, source);
        }
    }
    sb_printf(out, "}\n");
}

/* ---- the fixed-function pixel stage ---- */

struct ff_pixel_state {
    uint32_t stage_count;
    uint32_t color_op[8], color_arg[8][3], alpha_op[8], alpha_arg[8][3], result_arg[8];
    uint32_t stage_coord_projected[8];  // 0 none, else the number of coordinate components used for the projection divide
    uint32_t sampler_kind[8];           // 0 2D, 1 cube, 2 volume
    bool has_texture[8];
    uint32_t alpha_function;
    bool specular;
};

const char *argument_expression(uint32_t argument, char *buffer, size_t size, bool alpha_channel, uint32_t stage)
{
    uint32_t base = argument & 0xf;
    bool complement = (argument & 0x10) != 0;
    bool replicate = (argument & 0x20) != 0;
    char value[64];

    switch (base) {
    case 0: snprintf(value, sizeof(value), "gl_Color"); break;
    case 1: snprintf(value, sizeof(value), "cur"); break;
    case 2: snprintf(value, sizeof(value), "tex%u", stage); break;
    case 3: snprintf(value, sizeof(value), "u_texture_factor"); break;
    case 4: snprintf(value, sizeof(value), "gl_SecondaryColor"); break;
    case 5: snprintf(value, sizeof(value), "tmp"); break;
    default: snprintf(value, sizeof(value), "u_constant%u", stage); break;
    }
    if (alpha_channel) {
        snprintf(buffer, size, complement ? "(1.0 - %s.a)" : "%s.a", value);
    } else if (replicate) {
        snprintf(buffer, size, complement ? "vec3(1.0 - %s.a)" : "vec3(%s.a)", value);
    } else {
        snprintf(buffer, size, complement ? "(vec3(1.0) - %s.rgb)" : "%s.rgb", value);
    }
    return buffer;
}

/** Emits `dest = <op>(arg0, arg1, arg2)` for one channel group (rgb as vec3 or alpha as float). */
void emit_operation(sbuf &out, const char *dest, uint32_t op, const char *a0, const char *a1, const char *a2, const char *a1_alpha, bool alpha_channel, uint32_t stage,
    const char *blend_source_alpha)
{
    const char *type = alpha_channel ? "float" : "vec3";
    char blend[64];

    (void)type;
    (void)stage;
    switch (op) {
    case 2: sb_printf(out, "    %s = %s;\n", dest, a1); break;
    case 3: sb_printf(out, "    %s = %s;\n", dest, a2); break;
    case 4: sb_printf(out, "    %s = %s * %s;\n", dest, a1, a2); break;
    case 5: sb_printf(out, "    %s = (%s * %s) * 2.0;\n", dest, a1, a2); break;
    case 6: sb_printf(out, "    %s = (%s * %s) * 4.0;\n", dest, a1, a2); break;
    case 7: sb_printf(out, "    %s = %s + %s;\n", dest, a1, a2); break;
    case 8: sb_printf(out, "    %s = %s + %s - 0.5;\n", dest, a1, a2); break;
    case 9: sb_printf(out, "    %s = (%s + %s - 0.5) * 2.0;\n", dest, a1, a2); break;
    case 10: sb_printf(out, "    %s = %s - %s;\n", dest, a1, a2); break;
    case 11: sb_printf(out, "    %s = %s + %s - %s * %s;\n", dest, a1, a2, a1, a2); break;
    case 12: case 13: case 14: case 16: {
        if (op == 12) snprintf(blend, sizeof(blend), "gl_Color.a");
        else if (op == 13) snprintf(blend, sizeof(blend), "%s", blend_source_alpha);
        else if (op == 14) snprintf(blend, sizeof(blend), "u_texture_factor.a");
        else snprintf(blend, sizeof(blend), "cur.a");
        sb_printf(out, "    %s = %s * %s + %s * (1.0 - %s);\n", dest, a1, blend, a2, blend);
        break;
    }
    case 15: sb_printf(out, "    %s = %s + %s * (1.0 - %s);\n", dest, a1, a2, blend_source_alpha); break;
    case 17: sb_printf(out, "    %s = %s;\n", dest, a1); break;
    case 18: sb_printf(out, "    %s = %s + %s * %s;\n", dest, a1, a1_alpha, a2); break;
    case 19: sb_printf(out, "    %s = %s * %s + %s;\n", dest, a1, a2, a1_alpha); break;
    case 20: sb_printf(out, "    %s = (1.0 - %s) * %s + %s;\n", dest, a1_alpha, a2, a1); break;
    case 21: sb_printf(out, "    %s = (1.0 - %s) * %s + %s;\n", dest, a1, a2, a1_alpha); break;
    case 24: sb_printf(out, "    %s = %s(clamp(dot(%s * 2.0 - 1.0, %s * 2.0 - 1.0), 0.0, 1.0));\n", dest, alpha_channel ? "" : "vec3", a1, a2); break;
    case 25: sb_printf(out, "    %s = %s + %s * %s;\n", dest, a0, a1, a2); break;
    case 26: sb_printf(out, "    %s = mix(%s, %s, %s);\n", dest, a2, a1, a0); break;
    default: sb_printf(out, "    %s = %s * %s;\n", dest, a1, a2); break;
    }
}

void build_ff_pixel(sbuf &out, const ff_pixel_state &s)
{
    sb_printf(out, "#version 110\n");
    for (uint32_t i = 0; i < s.stage_count; i++) {
        if (s.has_texture[i]) {
            sb_printf(out, "uniform %s s%u;\n", s.sampler_kind[i] == 1 ? "samplerCube" : s.sampler_kind[i] == 2 ? "sampler3D" : "sampler2D", i);
        }
        if (i >= 4) sb_printf(out, "varying vec4 io_5_%u;\n", i);
    }
    sb_printf(out, "uniform vec4 u_texture_factor;\nuniform float u_alpha_ref;\n");
    for (uint32_t i = 0; i < s.stage_count; i++) sb_printf(out, "uniform vec4 u_constant%u;\n", i);
    sb_printf(out, "void main()\n{\n    vec4 cur = gl_Color;\n    vec4 tmp = vec4(0.0);\n");
    for (uint32_t i = 0; i < s.stage_count; i++) {
        char coord[40];
        char a0[96], a1[96], a2[96];
        const char *dest_cur = s.result_arg[i] == 5 ? "tmp" : "cur";

        if (i < 4) snprintf(coord, sizeof(coord), "gl_TexCoord[%u]", i); else snprintf(coord, sizeof(coord), "io_5_%u", i);
        if (s.has_texture[i]) {
            if (s.sampler_kind[i] == 1) sb_printf(out, "    vec4 tex%u = textureCube(s%u, %s.xyz);\n", i, i, coord);
            else if (s.sampler_kind[i] == 2) sb_printf(out, "    vec4 tex%u = texture3D(s%u, %s.xyz);\n", i, i, coord);
            else if (s.stage_coord_projected[i] == 4) sb_printf(out, "    vec4 tex%u = texture2DProj(s%u, %s);\n", i, i, coord);
            else if (s.stage_coord_projected[i] == 3) sb_printf(out, "    vec4 tex%u = texture2DProj(s%u, %s.xyz);\n", i, i, coord);
            else sb_printf(out, "    vec4 tex%u = texture2D(s%u, %s.xy);\n", i, i, coord);
        } else {
            sb_printf(out, "    vec4 tex%u = vec4(1.0);\n", i);
        }
        {
            char blend_alpha[32];

            snprintf(blend_alpha, sizeof(blend_alpha), "tex%u.a", i);
            sb_printf(out, "    {\n");
            // colour
            sb_printf(out, "    vec3 c;\n    float al;\n");
            argument_expression(s.color_arg[i][0], a0, sizeof(a0), false, i);
            argument_expression(s.color_arg[i][1], a1, sizeof(a1), false, i);
            argument_expression(s.color_arg[i][2], a2, sizeof(a2), false, i);
            {
                char a1_alpha[96];

                argument_expression(s.color_arg[i][1], a1_alpha, sizeof(a1_alpha), true, i);
                emit_operation(out, "c", s.color_op[i], a0, a1, a2, a1_alpha, false, i, blend_alpha);
            }
            sb_printf(out, "    c = clamp(c, 0.0, 1.0);\n");
            argument_expression(s.alpha_arg[i][0], a0, sizeof(a0), true, i);
            argument_expression(s.alpha_arg[i][1], a1, sizeof(a1), true, i);
            argument_expression(s.alpha_arg[i][2], a2, sizeof(a2), true, i);
            if (s.color_op[i] == 24) {
                sb_printf(out, "    al = c.r;\n");
            } else if (s.alpha_op[i] == 1) {
                sb_printf(out, "    al = cur.a;\n");
            } else {
                emit_operation(out, "al", s.alpha_op[i], a0, a1, a2, a1, true, i, blend_alpha);
                sb_printf(out, "    al = clamp(al, 0.0, 1.0);\n");
            }
            sb_printf(out, "    %s = vec4(c, al);\n    }\n", dest_cur);
        }
    }
    if (s.specular) sb_printf(out, "    cur.rgb += gl_SecondaryColor.rgb;\n");
    alpha_test_glsl(out, s.alpha_function, "cur.a");
    sb_printf(out, "    gl_FragColor = clamp(cur, 0.0, 1.0);\n}\n");
}

/* ---- program assembly ---- */

program *find_program(const program_key &key, uint64_t hash)
{
    for (uint32_t i = 0; i < g_program_count; i++) {
        if (g_programs[i].hash == hash && memcmp(&g_programs[i].key, &key, sizeof(key)) == 0) {
            return &g_programs[i];
        }
    }
    return nullptr;
}

program *add_program()
{
    if (g_program_count == g_program_capacity) {
        g_program_capacity = g_program_capacity != 0 ? g_program_capacity * 2 : 64;
        g_programs = static_cast<program *>(realloc(g_programs, g_program_capacity * sizeof(program)));
    }
    memset(&g_programs[g_program_count], 0, sizeof(program));
    return &g_programs[g_program_count++];
}

void bind_attribute_names(GLuint prog, const MOJOSHADER_parseData *parse)
{
    for (int i = 0; i < parse->attribute_count; i++) {
        // vs_1_x registers are declared too (dcl_texcoord1 v8, ...): Direct3D 9 matches them to declaration elements by usage
        int slot = attribute_slot(static_cast<uint32_t>(parse->attributes[i].usage), static_cast<uint32_t>(parse->attributes[i].index));

        if (slot >= 0) {
            glBindAttribLocation(prog, static_cast<GLuint>(slot), parse->attributes[i].name);
        }
    }
}

void bind_ff_attribute_names(GLuint prog)
{
    glBindAttribLocation(prog, 0, "a_position");
    glBindAttribLocation(prog, 3, "a_normal");
    glBindAttribLocation(prog, 4, "a_color0");
    glBindAttribLocation(prog, 5, "a_color1");
    for (int i = 0; i < 8; i++) {
        char name[24];

        snprintf(name, sizeof(name), "a_texcoord%d", i);
        glBindAttribLocation(prog, static_cast<GLuint>(6 + i), name);
    }
}

void finish_program(program *p, const gl_shader *vs, const gl_shader *ps, const char *vertex_source, const char *pixel_source, const ff_vertex_state *ff_vs,
    const ff_pixel_state *ff_ps)
{
    GLuint vertex = compile_stage(GL_VERTEX_SHADER, vertex_source, "vertex shader");
    GLuint fragment = compile_stage(GL_FRAGMENT_SHADER, pixel_source, "pixel shader");
    GLint status = 0;

    (void)ff_vs;
    (void)ff_ps;
    p->vertex_shader = vs;
    p->pixel_shader = ps;
    if (vertex == 0 || fragment == 0) {
        return;
    }
    p->name = glCreateProgram();
    glAttachShader(p->name, vertex);
    glAttachShader(p->name, fragment);
    if (vs != nullptr && vs->parse != nullptr) {
        bind_attribute_names(p->name, vs->parse);
    } else {
        bind_ff_attribute_names(p->name);
    }
    glLinkProgram(p->name);
    glDeleteShader(vertex);
    glDeleteShader(fragment);
    glGetProgramiv(p->name, GL_LINK_STATUS, &status);
    if (status == 0) {
        char log[1024];

        glGetProgramInfoLog(p->name, sizeof(log), nullptr, log);
        halo::shell::standalone_log("gl: program link failed: %s", log);
        glDeleteProgram(p->name);
        p->name = 0;
        return;
    }
    p->ok = true;
    p->vs_constants = glGetUniformLocation(p->name, "vs_uniforms_vec4");
    p->ps_constants = glGetUniformLocation(p->name, "ps_uniforms_vec4");
    p->flip = glGetUniformLocation(p->name, "vpFlip");
    p->half_pixel = glGetUniformLocation(p->name, "u_half_pixel");
    p->alpha_ref = glGetUniformLocation(p->name, "u_alpha_ref");
    p->wvp = glGetUniformLocation(p->name, "u_wvp");
    p->world_view = glGetUniformLocation(p->name, "u_world_view");
    p->rhw_scale = glGetUniformLocation(p->name, "u_rhw_scale");
    p->texture_factor = glGetUniformLocation(p->name, "u_texture_factor");
    p->world = glGetUniformLocation(p->name, "u_world");
    p->eye = glGetUniformLocation(p->name, "u_eye");
    p->ambient = glGetUniformLocation(p->name, "u_gambient");
    {
        static const char *const material_names[5] = {"u_mdiffuse", "u_mambient", "u_mspecular", "u_memissive", "u_mpower"};
        static const char *const light_names[7] = {"u_lpos", "u_ldir", "u_ldiffuse", "u_lspecular", "u_lambient", "u_latten", "u_lspot"};

        for (int k = 0; k < 5; k++) p->material[k] = glGetUniformLocation(p->name, material_names[k]);
        for (int k = 0; k < 7; k++) p->light[k] = glGetUniformLocation(p->name, light_names[k]);
    }
    for (int i = 0; i < 8; i++) {
        char name[24];

        snprintf(name, sizeof(name), "u_constant%d", i);
        p->constant_color[i] = glGetUniformLocation(p->name, name);
        snprintf(name, sizeof(name), "u_texmat%d", i);
        p->texture_matrix[i] = glGetUniformLocation(p->name, name);
    }
    glUseProgram(p->name);
    g_bound_program = p->name;
    for (int i = 0; i < 16; i++) {
        p->sampler[i] = -1;
    }
    if (ps != nullptr && ps->parse != nullptr) {
        for (int i = 0; i < ps->parse->sampler_count; i++) {
            GLint location = glGetUniformLocation(p->name, ps->parse->samplers[i].name);

            if (location >= 0) {
                glUniform1i(location, ps->parse->samplers[i].index);
            }
        }
    } else if (ff_ps != nullptr) {
        for (uint32_t i = 0; i < ff_ps->stage_count; i++) {
            char name[8];

            snprintf(name, sizeof(name), "s%u", i);
            GLint location = glGetUniformLocation(p->name, name);

            if (location >= 0) {
                glUniform1i(location, static_cast<GLint>(i));
            }
        }
    }
}

}  // namespace

namespace {

/* ---- matrices (Direct3D row-major, row-vector convention) ---- */

void multiply(const float *a, const float *b, float *out)
{
    float result[16];

    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            result[i * 4 + j] = a[i * 4] * b[j] + a[i * 4 + 1] * b[4 + j] + a[i * 4 + 2] * b[8 + j] + a[i * 4 + 3] * b[12 + j];
        }
    }
    memcpy(out, result, sizeof(result));
}

/* ---- render state ---- */

GLenum compare_function(uint32_t value)
{
    return 0x200 + (value >= 1 && value <= 8 ? value - 1 : 7);
}

GLenum blend_factor(uint32_t value)
{
    switch (value) {
    case 1: return GL_ZERO;
    case 2: return GL_ONE;
    case 3: return GL_SRC_COLOR;
    case 4: return GL_ONE_MINUS_SRC_COLOR;
    case 5: return GL_SRC_ALPHA;
    case 6: return GL_ONE_MINUS_SRC_ALPHA;
    case 7: return GL_DST_ALPHA;
    case 8: return GL_ONE_MINUS_DST_ALPHA;
    case 9: return GL_DST_COLOR;
    case 10: return GL_ONE_MINUS_DST_COLOR;
    case 11: return GL_SRC_ALPHA_SATURATE;
    case 12: return GL_SRC_ALPHA;
    case 13: return GL_ONE_MINUS_SRC_ALPHA;
    case 14: return GL_CONSTANT_COLOR;
    case 15: return 0x8002;  // GL_ONE_MINUS_CONSTANT_COLOR
    default: return GL_ONE;
    }
}

GLenum blend_factor_destination(uint32_t value)
{
    return value == 12 ? GL_ONE_MINUS_SRC_ALPHA : value == 13 ? GL_SRC_ALPHA : blend_factor(value);
}

GLenum stencil_operation(uint32_t value)
{
    switch (value) {
    case 2: return GL_ZERO;
    case 3: return GL_REPLACE;
    case 4: return GL_INCR;
    case 5: return GL_DECR;
    case 6: return GL_INVERT;
    case 7: return GL_INCR_WRAP;
    case 8: return GL_DECR_WRAP;
    default: return GL_KEEP;
    }
}

GLenum blend_equation(uint32_t value)
{
    switch (value) {
    case 2: return GL_FUNC_SUBTRACT;
    case 3: return GL_FUNC_REVERSE_SUBTRACT;
    case 4: return GL_MIN;
    case 5: return GL_MAX;
    default: return GL_FUNC_ADD;
    }
}

bool group_changed(const uint32_t *indices, int count)
{
    bool changed = !g_applied_valid;

    for (int i = 0; i < count; i++) {
        if (g_applied_state[indices[i]] != g_pipe.render_state[indices[i]]) {
            changed = true;
        }
    }
    return changed;
}

void commit_group(const uint32_t *indices, int count)
{
    for (int i = 0; i < count; i++) {
        g_applied_state[indices[i]] = g_pipe.render_state[indices[i]];
    }
}

void apply_render_state()
{
    const uint32_t *rs = g_pipe.render_state;
    static const uint32_t depth[] = {D3DRS_ZENABLE, D3DRS_ZFUNC, D3DRS_ZWRITEENABLE};
    static const uint32_t cull[] = {D3DRS_CULLMODE};
    static const uint32_t blend[] = {D3DRS_ALPHABLENDENABLE, D3DRS_SRCBLEND, D3DRS_DESTBLEND, D3DRS_BLENDOP, D3DRS_SEPARATEALPHABLENDENABLE,
        D3DRS_SRCBLENDALPHA, D3DRS_DESTBLENDALPHA, D3DRS_BLENDOPALPHA, 193};
    static const uint32_t mask[] = {D3DRS_COLORWRITEENABLE};
    static const uint32_t stencil[] = {D3DRS_STENCILENABLE, D3DRS_STENCILFAIL, D3DRS_STENCILZFAIL, D3DRS_STENCILPASS, D3DRS_STENCILFUNC,
        D3DRS_STENCILREF, D3DRS_STENCILMASK, D3DRS_STENCILWRITEMASK};
    static const uint32_t bias[] = {D3DRS_DEPTHBIAS, D3DRS_SLOPESCALEDEPTHBIAS};
    static const uint32_t fill[] = {D3DRS_FILLMODE};
    static uint32_t last_flip = 2;

    if (last_flip != (g_target_flipped ? 1u : 0u)) {
        last_flip = g_target_flipped ? 1u : 0u;
        g_applied_valid = false;
    }
    if (group_changed(depth, 3)) {
        if (rs[D3DRS_ZENABLE] != 0) glEnable(GL_DEPTH_TEST); else glDisable(GL_DEPTH_TEST);
        glDepthFunc(compare_function(rs[D3DRS_ZFUNC]));
        glDepthMask(rs[D3DRS_ZWRITEENABLE] != 0 ? GL_TRUE : GL_FALSE);
        commit_group(depth, 3);
    }
    if (group_changed(cull, 1)) {
        uint32_t mode = rs[D3DRS_CULLMODE];

        if (mode == 1 || mode == 0) {
            glDisable(GL_CULL_FACE);
        } else {
            bool cull_front = (mode == 2) != g_target_flipped;

            glEnable(GL_CULL_FACE);
            glCullFace(cull_front ? GL_FRONT : GL_BACK);
        }
        commit_group(cull, 1);
    }
    if (group_changed(blend, 9)) {
        bool separate = rs[D3DRS_SEPARATEALPHABLENDENABLE] != 0;
        uint32_t source = rs[D3DRS_SRCBLEND];
        uint32_t destination = rs[D3DRS_DESTBLEND];

        if (rs[D3DRS_ALPHABLENDENABLE] != 0) glEnable(GL_BLEND); else glDisable(GL_BLEND);
        glBlendFuncSeparate(blend_factor(source), blend_factor_destination(source == 12 || source == 13 ? source : destination),
            blend_factor(separate ? rs[D3DRS_SRCBLENDALPHA] : source),
            blend_factor_destination(separate ? rs[D3DRS_DESTBLENDALPHA] : (source == 12 || source == 13 ? source : destination)));
        glBlendEquation(blend_equation(rs[D3DRS_BLENDOP]));
        glBlendColor(static_cast<float>((rs[193] >> 16) & 255) / 255.0f, static_cast<float>((rs[193] >> 8) & 255) / 255.0f,
            static_cast<float>(rs[193] & 255) / 255.0f, static_cast<float>(rs[193] >> 24) / 255.0f);
        commit_group(blend, 9);
    }
    if (group_changed(mask, 1)) {
        uint32_t m = rs[D3DRS_COLORWRITEENABLE];

        glColorMask((m & 1) != 0, (m & 2) != 0, (m & 4) != 0, (m & 8) != 0);
        commit_group(mask, 1);
    }
    if (group_changed(stencil, 8)) {
        if (rs[D3DRS_STENCILENABLE] != 0) glEnable(GL_STENCIL_TEST); else glDisable(GL_STENCIL_TEST);
        glStencilFunc(compare_function(rs[D3DRS_STENCILFUNC]), static_cast<GLint>(rs[D3DRS_STENCILREF] & 0xff), rs[D3DRS_STENCILMASK] & 0xff);
        glStencilOp(stencil_operation(rs[D3DRS_STENCILFAIL]), stencil_operation(rs[D3DRS_STENCILZFAIL]), stencil_operation(rs[D3DRS_STENCILPASS]));
        glStencilMask(rs[D3DRS_STENCILWRITEMASK] & 0xff);
        commit_group(stencil, 8);
    }
    if (group_changed(bias, 2)) {
        float depth_bias;
        float slope;

        memcpy(&depth_bias, &rs[D3DRS_DEPTHBIAS], 4);
        memcpy(&slope, &rs[D3DRS_SLOPESCALEDEPTHBIAS], 4);
        if (depth_bias != 0.0f || slope != 0.0f) {
            glEnable(GL_POLYGON_OFFSET_FILL);
            glPolygonOffset(slope, depth_bias * 16777216.0f);
        } else {
            glDisable(GL_POLYGON_OFFSET_FILL);
        }
        commit_group(bias, 2);
    }
    if (group_changed(fill, 1)) {
        uint32_t mode = rs[D3DRS_FILLMODE];

        glPolygonMode(GL_FRONT_AND_BACK, mode == 1 ? GL_POINT : mode == 2 ? GL_LINE : GL_FILL);
        commit_group(fill, 1);
    }
    g_applied_valid = true;
    // debugging overrides
    {
        static const bool no_cull = getenv("HALO_GL_NOCULL") != nullptr;
        static const bool no_depth = getenv("HALO_GL_NODEPTH") != nullptr;

        if (no_cull) glDisable(GL_CULL_FACE);
        if (no_depth) glDisable(GL_DEPTH_TEST);
    }
}

/* ---- vertex declarations ---- */

uint32_t declaration_elements(const gl_declaration *declaration, element *out)
{
    uint32_t count = 0;

    for (uint32_t i = 0; i < declaration->element_count && i < 32; i++) {
        const uint8_t *e = declaration->elements + i * 8;

        out[count++] = {*reinterpret_cast<const uint16_t *>(e), *reinterpret_cast<const uint16_t *>(e + 2), e[4], e[6], e[7]};
    }
    return count;
}

struct attribute_format {
    GLint size;
    GLenum type;
    GLboolean normalized;
};

bool attribute_format_for(uint32_t type, attribute_format *out)
{
    switch (type) {
    case 0: *out = {1, GL_FLOAT, GL_FALSE}; return true;
    case 1: *out = {2, GL_FLOAT, GL_FALSE}; return true;
    case 2: *out = {3, GL_FLOAT, GL_FALSE}; return true;
    case 3: *out = {4, GL_FLOAT, GL_FALSE}; return true;
    case 4: *out = {GL_BGRA, GL_UNSIGNED_BYTE, GL_TRUE}; return true;
    case 5: *out = {4, GL_UNSIGNED_BYTE, GL_FALSE}; return true;
    case 6: *out = {2, GL_SHORT, GL_FALSE}; return true;
    case 7: *out = {4, GL_SHORT, GL_FALSE}; return true;
    case 8: *out = {4, GL_UNSIGNED_BYTE, GL_TRUE}; return true;
    case 9: *out = {2, GL_SHORT, GL_TRUE}; return true;
    case 10: *out = {4, GL_SHORT, GL_TRUE}; return true;
    case 11: *out = {2, GL_UNSIGNED_SHORT, GL_TRUE}; return true;
    case 12: *out = {4, GL_UNSIGNED_SHORT, GL_TRUE}; return true;
    case 13: *out = {4, 0x8368, GL_FALSE}; return true;   // GL_UNSIGNED_INT_2_10_10_10_REV
    case 14: *out = {4, 0x8D9F, GL_TRUE}; return true;    // GL_INT_2_10_10_10_REV
    case 15: *out = {2, k_gl_half_float, GL_FALSE}; return true;
    case 16: *out = {4, k_gl_half_float, GL_FALSE}; return true;
    default: return false;
    }
}

GLenum primitive_mode(uint32_t type)
{
    switch (type) {
    case 1: return GL_POINTS;
    case 2: return GL_LINES;
    case 3: return GL_LINE_STRIP;
    case 5: return GL_TRIANGLE_STRIP;
    case 6: return GL_TRIANGLE_FAN;
    default: return GL_TRIANGLES;
    }
}

uint32_t primitive_vertices(uint32_t type, uint32_t primitives)
{
    switch (type) {
    case 1: return primitives;
    case 2: return primitives * 2;
    case 3: return primitives + 1;
    case 5: case 6: return primitives + 2;
    default: return primitives * 3;
    }
}

void upload_scratch(GLenum target, GLuint buffer, const void *data, size_t size)
{
    glBindBuffer(target, buffer);
    glBufferData(target, static_cast<GLsizeiptr>(size), data, GL_STREAM_DRAW);
}

}  // namespace

/* ======================================================================================================== */
/* The draw call                                                                                             */
/* ======================================================================================================== */

void viewport_scissor(bool enable)
{
    float width = g_pipe.viewport[2] > 0.0f ? g_pipe.viewport[2] : static_cast<float>(g_target_width);
    float height = g_pipe.viewport[3] > 0.0f ? g_pipe.viewport[3] : static_cast<float>(g_target_height);
    int x = static_cast<int>(g_pipe.viewport[0]);
    int y = static_cast<int>(g_pipe.viewport[1]);

    if (!enable || (x == 0 && y == 0 && width >= static_cast<float>(g_target_width) && height >= static_cast<float>(g_target_height))) {
        glDisable(GL_SCISSOR_TEST);
        return;
    }
    glEnable(GL_SCISSOR_TEST);
    glScissor(x, g_target_flipped ? y : static_cast<int>(g_target_height) - (y + static_cast<int>(height)), static_cast<GLsizei>(width), static_cast<GLsizei>(height));
}

void draw_init()
{
    static const float default_attribute[4] = {0.0f, 0.0f, 0.0f, 1.0f};

    set_defaults();
    if (glGenVertexArrays != nullptr) {
        glGenVertexArrays(1, &g_vao);
        glBindVertexArray(g_vao);
    }
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    glFrontFace(GL_CW);
    glDisable(GL_DITHER);
    make_default_textures();
    glGenBuffers(1, &g_scratch_vertices);
    glGenBuffers(1, &g_scratch_indices);
    for (int slot = 0; slot < k_attribute_slots; slot++) {
        glVertexAttrib4f(static_cast<GLuint>(slot), default_attribute[0], default_attribute[1], default_attribute[2], default_attribute[3]);
    }
    g_target_width = g_state.width;
    g_target_height = g_state.height;
    g_pipe.viewport[2] = static_cast<float>(g_state.width);
    g_pipe.viewport[3] = static_cast<float>(g_state.height);
    g_pipe.viewport[5] = 1.0f;
}

void draw_shutdown_object(gl_object *object)
{
    if (!g_state.modern) {
        return;
    }
    switch (object->kind) {
    case kind_texture:
    case kind_volume_texture:
    case kind_cube_texture: {
        gl_texture *texture = static_cast<gl_texture *>(object);

        if (texture->name != 0) glDeleteTextures(1, &texture->name);
        if (texture->framebuffer != 0) glDeleteFramebuffers(1, &texture->framebuffer);
        if (texture->depth_buffer != 0) glDeleteRenderbuffers(1, &texture->depth_buffer);
        break;
    }
    case kind_vertex_buffer:
    case kind_index_buffer: {
        gl_buffer *buffer = static_cast<gl_buffer *>(object);

        if (buffer->name != 0) glDeleteBuffers(1, &buffer->name);
        break;
    }
    case kind_query: {
        gl_query *query = static_cast<gl_query *>(object);

        if (query->name != 0) glDeleteQueries(1, &query->name);
        break;
    }
    case kind_vertex_shader:
    case kind_pixel_shader: {
        gl_shader *shader = static_cast<gl_shader *>(object);

        if (shader->parse != nullptr && shader->parse != &g_placeholder_parse) MOJOSHADER_freeParseData(shader->parse);
        for (uint32_t i = 0; i < shader->variant_count; i++) {
            destroy_object(shader->variant[i]);
        }
        free(shader->bytecode);
        break;
    }
    default:
        break;
    }
}

namespace {

struct draw_setup {
    element elements[32];
    uint32_t element_count;
    program *prog;
    ff_vertex_state ff_vs;
    ff_pixel_state ff_ps;
    bool uses_ff_vertex;
    bool uses_ff_pixel;
};

/** Gathers the fixed-function pixel stage description from the texture stage states. */
void gather_ff_pixel(ff_pixel_state &s, bool alpha_test)
{
    const uint32_t *rs = g_pipe.render_state;

    memset(&s, 0, sizeof(s));
    for (uint32_t i = 0; i < 8; i++) {
        const uint32_t *t = g_pipe.texture_stage_state[i];

        if (t[TSS_COLOROP] == 1) {
            break;
        }
        s.stage_count = i + 1;
        s.color_op[i] = t[TSS_COLOROP];
        s.color_arg[i][0] = t[TSS_COLORARG0];
        s.color_arg[i][1] = t[TSS_COLORARG1];
        s.color_arg[i][2] = t[TSS_COLORARG2];
        s.alpha_op[i] = t[TSS_ALPHAOP];
        s.alpha_arg[i][0] = t[TSS_ALPHAARG0];
        s.alpha_arg[i][1] = t[TSS_ALPHAARG1];
        s.alpha_arg[i][2] = t[TSS_ALPHAARG2];
        s.result_arg[i] = t[TSS_RESULTARG];
        if ((t[TSS_TEXTURETRANSFORMFLAGS] & 0x100) != 0) {
            s.stage_coord_projected[i] = (t[TSS_TEXTURETRANSFORMFLAGS] & 0xff) != 0 ? (t[TSS_TEXTURETRANSFORMFLAGS] & 0xff) : 4;
        }
        if (g_pipe.texture[i] != nullptr) {
            gl_texture *texture = static_cast<gl_texture *>(g_pipe.texture[i]);

            s.has_texture[i] = true;
            s.sampler_kind[i] = texture->kind == kind_cube_texture ? 1 : texture->kind == kind_volume_texture ? 2 : 0;
        }
    }
    s.alpha_function = alpha_test ? rs[D3DRS_ALPHAFUNC] : 8;
    s.specular = rs[D3DRS_SPECULARENABLE] != 0;
}

void gather_ff_vertex(ff_vertex_state &s, const element *elements, uint32_t count, uint32_t stage_count)
{
    memset(&s, 0, sizeof(s));
    for (uint32_t i = 0; i < count; i++) {
        const element &e = elements[i];

        if (e.usage == USAGE_POSITIONT) s.rhw = true;
        else if (e.usage == USAGE_NORMAL) s.has_normal = true;
        else if (e.usage == USAGE_COLOR && e.index == 0) s.has_color0 = true;
        else if (e.usage == USAGE_COLOR && e.index == 1) s.has_color1 = true;
        else if (e.usage == USAGE_TEXCOORD && e.index < 8) s.texcoord_mask |= 1u << e.index;
    }
    s.stage_count = stage_count;
    for (uint32_t i = 0; i < stage_count; i++) {
        uint32_t index = g_pipe.texture_stage_state[i][TSS_TEXCOORDINDEX];

        s.stage_coord[i] = index & 0xffff;
        s.stage_texgen[i] = (index >> 16) & 3;
        s.stage_transform[i] = g_pipe.texture_stage_state[i][TSS_TEXTURETRANSFORMFLAGS] & 0x1ff;
    }
    s.lighting = g_pipe.render_state[D3DRS_LIGHTING] != 0 && s.has_normal && !s.rhw;
    if (s.lighting) {
        const uint32_t *rs = g_pipe.render_state;
        bool color_vertex = rs[D3DRS_COLORVERTEX] != 0;

        s.specular = rs[D3DRS_SPECULARENABLE] != 0;
        s.material_source[0] = color_vertex ? (rs[D3DRS_DIFFUSEMATERIALSOURCE] & 3) : 0;
        s.material_source[1] = color_vertex ? (rs[D3DRS_AMBIENTMATERIALSOURCE] & 3) : 0;
        s.material_source[2] = color_vertex ? (rs[D3DRS_SPECULARMATERIALSOURCE] & 3) : 0;
        s.material_source[3] = color_vertex ? (rs[D3DRS_EMISSIVEMATERIALSOURCE] & 3) : 0;
        for (int i = 0; i < 8; i++) {
            uint32_t type = 0;

            if (g_pipe.light_on[i]) {
                memcpy(&type, &g_pipe.light[i][0], sizeof(type));
                type = type >= 1 && type <= 3 ? type : 0;
            }
            s.light_type[i] = type;
        }
    }
}

/** The translation of ps_1_x shader `ps` whose sampler types match the textures bound now (see keep_ps1_bytecode). */
gl_shader *pixel_shader_for_bound_textures(gl_shader *ps)
{
    static uint32_t next_variant_id = 0x40000000;
    MOJOSHADER_samplerMap map[16];
    unsigned int map_count = 0;
    uint32_t key = 0;
    gl_shader *variant;

    if (ps == nullptr || ps->bytecode == nullptr) {
        return ps;
    }
    for (int i = 0; i < ps->parse->sampler_count; i++) {
        int unit = ps->parse->samplers[i].index;
        const gl_object *texture = unit >= 0 && unit < 16 ? g_pipe.texture[unit] : nullptr;
        uint32_t kind = texture != nullptr ? texture->kind : kind_texture;

        if (kind == kind_cube_texture || kind == kind_volume_texture) {
            map[map_count].index = unit;
            map[map_count].type = kind == kind_cube_texture ? MOJOSHADER_SAMPLER_CUBE : MOJOSHADER_SAMPLER_VOLUME;
            map_count++;
            key |= (kind == kind_cube_texture ? 1u : 2u) << (2 * unit);
        }
    }
    if (key == 0) {
        return ps;
    }
    for (uint32_t i = 0; i < ps->variant_count; i++) {
        if (ps->variant_key[i] == key) {
            return ps->variant[i];
        }
    }
    // ponytail: at most 8 texture-type combinations per shader; further ones keep the 2D translation
    if (ps->variant_count == 8) {
        return ps;
    }
    variant = new_object<gl_shader>(kind_pixel_shader);
    variant->id = next_variant_id++;
    variant->parse = MOJOSHADER_parse(MOJOSHADER_PROFILE_GLSL, nullptr, ps->bytecode, ps->bytecode_size, nullptr, 0, map, map_count, nullptr, nullptr, nullptr);
    if (variant->parse != nullptr && variant->parse->error_count > 0) {
        halo::shell::standalone_log("gl: pixel shader %u failed to translate for its bound textures: %s", ps->id, variant->parse->errors[0].error);
        MOJOSHADER_freeParseData(variant->parse);
        variant->parse = nullptr;
    }
    ps->variant_key[ps->variant_count] = key;
    ps->variant[ps->variant_count++] = variant;
    return variant;
}

program *select_program(draw_setup &setup)
{
    program_key key;
    uint64_t hash;
    program *found;
    gl_shader *vs = g_pipe.vertex_shader;
    gl_shader *ps = pixel_shader_for_bound_textures(g_pipe.pixel_shader);
    bool alpha_test = g_pipe.render_state[D3DRS_ALPHATESTENABLE] != 0;
    uint32_t alpha_function = alpha_test ? g_pipe.render_state[D3DRS_ALPHAFUNC] : 8;
    uint32_t stage_count = 0;

    setup.uses_ff_vertex = vs == nullptr;
    setup.uses_ff_pixel = ps == nullptr;
    if ((vs != nullptr && (vs->parse == nullptr || vs->parse->output == nullptr)) || (ps != nullptr && (ps->parse == nullptr || ps->parse->output == nullptr))) {
        return nullptr;
    }
    memset(&key, 0, sizeof(key));
    key.words[0] = vs != nullptr ? vs->id : 0;
    key.words[1] = ps != nullptr ? ps->id : 0;
    key.words[2] = alpha_function;
    if (setup.uses_ff_pixel) {
        gather_ff_pixel(setup.ff_ps, alpha_test);
        stage_count = setup.ff_ps.stage_count;
        static_assert(sizeof(ff_pixel_state) <= 400, "key too small");
        memcpy(&key.words[3], &setup.ff_ps, sizeof(ff_pixel_state));
    }
    if (setup.uses_ff_vertex) {
        gather_ff_vertex(setup.ff_vs, setup.elements, setup.element_count, setup.uses_ff_pixel ? stage_count : 0);
        if (!setup.uses_ff_pixel) {
            // pixel shader supplies its own sampling: still provide the texture coordinate sets it expects
            setup.ff_vs.stage_count = 0;
            for (uint32_t i = 0; i < 8; i++) {
                if ((setup.ff_vs.texcoord_mask & (1u << i)) != 0) {
                    setup.ff_vs.stage_coord[i] = i;
                    setup.ff_vs.stage_count = i + 1;
                }
            }
        }
        memcpy(&key.words[3 + (sizeof(ff_pixel_state) + 3) / 4], &setup.ff_vs, sizeof(ff_vertex_state));
    }
    hash = hash_words(key.words, k_key_words);
    found = find_program(key, hash);
    if (found != nullptr) {
        return found->ok ? found : nullptr;
    }

    found = add_program();
    found->hash = hash;
    found->key = key;
    {
        sbuf vertex_source = {};
        sbuf pixel_source = {};
        char *wrapped_vertex = nullptr;
        char *wrapped_pixel = nullptr;
        const char *vertex_text;
        const char *pixel_text;

        if (vs != nullptr) {
            if (getenv("HALO_GL_FORCEPOS") != nullptr) {
                wrapped_vertex = wrap_main(vs->parse->output,
                    "uniform vec2 u_half_pixel;\nvoid main()\n{\n    mojo_main();\n    gl_Position = vec4(vs_v0.x * 0.003, vs_v0.y * 0.003, 0.0, 1.0);\n}\n");
            } else {
                wrapped_vertex = wrap_main(vs->parse->output,
                    "uniform vec2 u_half_pixel;\nvoid main()\n{\n    mojo_main();\n    gl_Position.xy += gl_Position.w * u_half_pixel;\n}\n");
            }
            vertex_text = wrapped_vertex;
        } else {
            build_ff_vertex(vertex_source, setup.ff_vs);
            vertex_text = vertex_source.text;
        }
        if (ps != nullptr) {
            sbuf wrapper = {};

            sb_printf(wrapper, "uniform float u_alpha_ref;\nvoid main()\n{\n    mojo_main();\n");
            // ps_oC0 is MojoShader's name for the colour output (gl_FragColor or gl_FragData[0]): reading the other one is undefined
            alpha_test_glsl(wrapper, alpha_function, "ps_oC0.a");
            {
                // HALO_GL_PSDEBUG=<n>: show one input of a translated pixel shader instead of its result
                // 1 vertex colour, 2 secondary colour, 3 texture coordinate 0, 4..7 sampler 0..3 (2D only)
                const char *mode = getenv("HALO_GL_PSDEBUG");
                int which = mode != nullptr ? atoi(mode) : 0;

                if (which == 1) sb_printf(wrapper, "    ps_oC0 = vec4(gl_Color.rgb, 1.0);\n");
                else if (which == 2) sb_printf(wrapper, "    ps_oC0 = vec4(gl_SecondaryColor.rgb, 1.0);\n");
                else if (which == 3) sb_printf(wrapper, "    ps_oC0 = vec4(fract(gl_TexCoord[0].xy), 0.0, 1.0);\n");
                else if (which >= 4 && which <= 7) {
                    for (int s = 0; s < ps->parse->sampler_count; s++) {
                        if (ps->parse->samplers[s].index == which - 4 && ps->parse->samplers[s].type == MOJOSHADER_SAMPLER_2D) {
                            sb_printf(wrapper, "    ps_oC0 = vec4(texture2D(%s, gl_TexCoord[%d].xy).rgb, 1.0);\n", ps->parse->samplers[s].name, which - 4);
                        }
                    }
                }
            }
            if (getenv("HALO_GL_FORCEWHITE") != nullptr) {
                sb_printf(wrapper, "    ps_oC0 = vec4(1.0);\n");
            }
            sb_printf(wrapper, "}\n");
            wrapped_pixel = wrap_main(ps->parse->output, wrapper.text);
            sb_free(wrapper);
            pixel_text = wrapped_pixel;
        } else {
            build_ff_pixel(pixel_source, setup.ff_ps);
            pixel_text = pixel_source.text;
        }
        if (vertex_text != nullptr && pixel_text != nullptr) {
            if (trace_enabled_for_programs()) {
                char name[260];
                FILE *dump;

                snprintf(name, sizeof(name), "%sgl_prog_vs%u_ps%u.txt", getenv("HALO_GL_PROGRAMS"), key.words[0], key.words[1]);
                dump = fopen(name, "wb");
                if (dump != nullptr) {
                    fprintf(dump, "--- vertex\n%s\n--- pixel\n%s", vertex_text, pixel_text);
                    fclose(dump);
                }
            }
            finish_program(found, vs, ps, vertex_text, pixel_text, setup.uses_ff_vertex ? &setup.ff_vs : nullptr, setup.uses_ff_pixel ? &setup.ff_ps : nullptr);
        }
        free(wrapped_vertex);
        free(wrapped_pixel);
        sb_free(vertex_source);
        sb_free(pixel_source);
    }
    return found->ok ? found : nullptr;
}

void upload_float4(GLint location, const float *data, uint32_t count)
{
    if (location >= 0 && count != 0) {
        glUniform4fv(location, static_cast<GLsizei>(count), data);
    }
}

void upload_uniforms(program *p, const draw_setup &setup, float viewport_width, float viewport_height, int viewport_x, int viewport_y)
{
    float flip = g_target_flipped ? -1.0f : 1.0f;
    uint32_t factor = g_pipe.render_state[D3DRS_TEXTUREFACTOR];
    float world_view[16];
    float wvp[16];

    if (p->flip >= 0) glUniform1f(p->flip, flip);
    if (p->half_pixel >= 0) glUniform2f(p->half_pixel, 1.0f / viewport_width, -flip / viewport_height);
    if (p->alpha_ref >= 0) glUniform1f(p->alpha_ref, static_cast<float>(g_pipe.render_state[D3DRS_ALPHAREF] & 0xff) / 255.0f);
    if (p->vs_constants >= 0 && p->vertex_shader != nullptr && p->vertex_shader->parse != nullptr) {
        float packed[256 * 4];
        uint32_t count = pack_constants(p->vertex_shader->parse, g_pipe.vertex_constants, 256, packed);

        upload_float4(p->vs_constants, packed, count < 256 ? count : 256);
    }
    if (p->ps_constants >= 0 && p->pixel_shader != nullptr && p->pixel_shader->parse != nullptr) {
        float packed[224 * 4];
        uint32_t count = pack_constants(p->pixel_shader->parse, g_pipe.pixel_constants, 224, packed);

        upload_float4(p->ps_constants, packed, count < 224 ? count : 224);
    }
    if (p->texture_factor >= 0) {
        glUniform4f(p->texture_factor, static_cast<float>((factor >> 16) & 255) / 255.0f, static_cast<float>((factor >> 8) & 255) / 255.0f,
            static_cast<float>(factor & 255) / 255.0f, static_cast<float>(factor >> 24) / 255.0f);
    }
    for (int i = 0; i < 8; i++) {
        if (p->constant_color[i] >= 0) {
            uint32_t c = g_pipe.texture_stage_state[i][TSS_CONSTANT];

            glUniform4f(p->constant_color[i], static_cast<float>((c >> 16) & 255) / 255.0f, static_cast<float>((c >> 8) & 255) / 255.0f,
                static_cast<float>(c & 255) / 255.0f, static_cast<float>(c >> 24) / 255.0f);
        }
        if (p->texture_matrix[i] >= 0) {
            glUniformMatrix4fv(p->texture_matrix[i], 1, GL_FALSE, g_pipe.transform[16 + i]);
        }
    }
    if (setup.uses_ff_vertex) {
        multiply(g_pipe.transform[256], g_pipe.transform[2], world_view);
        multiply(world_view, g_pipe.transform[3], wvp);
        if (p->wvp >= 0) glUniformMatrix4fv(p->wvp, 1, GL_FALSE, wvp);
        if (p->world_view >= 0) glUniformMatrix4fv(p->world_view, 1, GL_FALSE, world_view);
        if (setup.ff_vs.lighting) {
            const float *m = g_pipe.material;
            const float *v = g_pipe.transform[2];
            uint32_t ambient = g_pipe.render_state[D3DRS_AMBIENT];
            float eye[3];
            float data[7][8][4];

            for (int j = 0; j < 3; j++) {
                eye[j] = -(v[12] * v[j * 4] + v[13] * v[j * 4 + 1] + v[14] * v[j * 4 + 2]);
            }
            if (p->world >= 0) glUniformMatrix4fv(p->world, 1, GL_FALSE, g_pipe.transform[256]);
            if (p->eye >= 0) glUniform3f(p->eye, eye[0], eye[1], eye[2]);
            if (p->ambient >= 0) {
                glUniform4f(p->ambient, static_cast<float>((ambient >> 16) & 255) / 255.0f, static_cast<float>((ambient >> 8) & 255) / 255.0f,
                    static_cast<float>(ambient & 255) / 255.0f, static_cast<float>(ambient >> 24) / 255.0f);
            }
            if (p->material[0] >= 0) glUniform4f(p->material[0], m[0], m[1], m[2], m[3]);
            if (p->material[1] >= 0) glUniform4f(p->material[1], m[4], m[5], m[6], m[7]);
            if (p->material[2] >= 0) glUniform4f(p->material[2], m[8], m[9], m[10], m[11]);
            if (p->material[3] >= 0) glUniform4f(p->material[3], m[12], m[13], m[14], m[15]);
            if (p->material[4] >= 0) glUniform1f(p->material[4], m[16]);
            for (int i = 0; i < 8; i++) {
                const float *l = g_pipe.light[i];

                memcpy(data[0][i], &l[13], 3 * sizeof(float)); data[0][i][3] = 1.0f;                       // position
                memcpy(data[1][i], &l[16], 3 * sizeof(float)); data[1][i][3] = 0.0f;                       // direction
                memcpy(data[2][i], &l[1], 4 * sizeof(float));                                                // diffuse
                memcpy(data[3][i], &l[5], 4 * sizeof(float));                                                // specular
                memcpy(data[4][i], &l[9], 4 * sizeof(float));                                                // ambient
                data[5][i][0] = l[21]; data[5][i][1] = l[22]; data[5][i][2] = l[23]; data[5][i][3] = l[19]; // attenuation, range
                data[6][i][0] = cosine(l[24] * 0.5f); data[6][i][1] = cosine(l[25] * 0.5f); data[6][i][2] = l[20]; data[6][i][3] = 0.0f;  // spot: cos(theta/2), cos(phi/2), falloff
            }
            for (int k = 0; k < 7; k++) {
                if (p->light[k] >= 0) glUniform4fv(p->light[k], 8, &data[k][0][0]);
            }
        }
        if (p->rhw_scale >= 0) {
            float gy0 = g_target_flipped ? static_cast<float>(viewport_y) : static_cast<float>(viewport_y);
            float scale_x = 2.0f / viewport_width;
            float offset_x = (1.0f - 2.0f * static_cast<float>(viewport_x)) / viewport_width - 1.0f;
            float scale_y;
            float offset_y;

            if (g_target_flipped) {
                scale_y = 2.0f / viewport_height;
                offset_y = (1.0f - 2.0f * gy0) / viewport_height - 1.0f;
            } else {
                scale_y = -2.0f / viewport_height;
                offset_y = (2.0f * (static_cast<float>(g_target_height) - 0.5f - gy0)) / viewport_height - 1.0f;
            }
            glUniform4f(p->rhw_scale, scale_x, scale_y, offset_x, offset_y);
        }
    }
}

void bind_textures(const draw_setup &setup)
{
    const gl_shader *ps = g_pipe.pixel_shader;

    if (ps != nullptr && ps->parse != nullptr) {
        for (int i = 0; i < ps->parse->sampler_count; i++) {
            const MOJOSHADER_sampler &sampler = ps->parse->samplers[i];
            GLenum wanted = sampler.type == MOJOSHADER_SAMPLER_CUBE ? GL_TEXTURE_CUBE_MAP : sampler.type == MOJOSHADER_SAMPLER_VOLUME ? GL_TEXTURE_3D : GL_TEXTURE_2D;

            if (sampler.index >= 0 && sampler.index < 16) {
                bind_texture_unit(static_cast<uint32_t>(sampler.index), wanted);
            }
        }
    } else {
        for (uint32_t i = 0; i < setup.ff_ps.stage_count; i++) {
            if (setup.ff_ps.has_texture[i]) {
                bind_texture_unit(i, setup.ff_ps.sampler_kind[i] == 1 ? GL_TEXTURE_CUBE_MAP : setup.ff_ps.sampler_kind[i] == 2 ? GL_TEXTURE_3D : GL_TEXTURE_2D);
            }
        }
    }
}

}  // namespace

void draw_geometry(uint32_t type, int32_t base_vertex, uint32_t vertex_count, uint32_t start_index, uint32_t primitive_count, uint32_t start_vertex,
    const void *vertex_data, uint32_t vertex_stride, const void *index_data, uint32_t index_format, bool indexed, bool user_data)
{
    draw_setup setup;
    uint32_t vertices = primitive_vertices(type, primitive_count);
    float viewport_width = g_pipe.viewport[2] > 0.0f ? g_pipe.viewport[2] : static_cast<float>(g_target_width);
    float viewport_height = g_pipe.viewport[3] > 0.0f ? g_pipe.viewport[3] : static_cast<float>(g_target_height);
    int viewport_x = static_cast<int>(g_pipe.viewport[0]);
    int viewport_y_d3d = static_cast<int>(g_pipe.viewport[1]);
    int viewport_y = g_target_flipped ? viewport_y_d3d : static_cast<int>(g_target_height) - (viewport_y_d3d + static_cast<int>(viewport_height));
    program *p;
    uint32_t used_streams = 0;
    uint32_t enabled_now = 0;
    uint32_t index_size = index_format == 102 ? 4 : 2;
    GLenum index_type = index_format == 102 ? GL_UNSIGNED_INT : GL_UNSIGNED_SHORT;
    uint32_t stream_stride_override;

    if (!g_state.modern || primitive_count == 0) {
        return;
    }
    memset(&setup, 0, sizeof(setup));
    if (g_pipe.declaration != nullptr) {
        setup.element_count = declaration_elements(g_pipe.declaration, setup.elements);
    } else {
        setup.element_count = fvf_elements(g_pipe.fvf, setup.elements);
    }
    p = select_program(setup);
    if (p == nullptr) {
        static uint32_t reported[64][2];
        static uint32_t reported_count;
        uint32_t vs_id = g_pipe.vertex_shader != nullptr ? g_pipe.vertex_shader->id : 0;
        uint32_t ps_id = g_pipe.pixel_shader != nullptr ? g_pipe.pixel_shader->id : 0;
        bool seen = false;

        for (uint32_t i = 0; i < reported_count; i++) {
            seen = seen || (reported[i][0] == vs_id && reported[i][1] == ps_id);
        }
        if (!seen && reported_count < 64) {
            reported[reported_count][0] = vs_id;
            reported[reported_count][1] = ps_id;
            reported_count++;
            halo::shell::standalone_log("gl: draw skipped (vs %u%s, ps %u%s)", vs_id,
                g_pipe.vertex_shader != nullptr && g_pipe.vertex_shader->parse == nullptr ? " untranslated" : "", ps_id,
                g_pipe.pixel_shader != nullptr && (g_pipe.pixel_shader->parse == nullptr || g_pipe.pixel_shader->parse->output == nullptr) ? " untranslated" : "");
        }
        return;
    }
    stream_stride_override = g_pipe.declaration == nullptr ? fvf_vertex_size(g_pipe.fvf) : 0;

    if (g_pipe.pixel_shader != nullptr) {
        // HALO_GL_SKIP_PS=<id>: drop the draws that use one pixel shader
        static const int skip_ps = getenv("HALO_GL_SKIP_PS") != nullptr ? atoi(getenv("HALO_GL_SKIP_PS")) : 0;

        if (static_cast<int>(g_pipe.pixel_shader->id) == skip_ps) {
            return;
        }
        note_programmable_draw();
    }
    if (g_bound_program != p->name) {
        glUseProgram(p->name);
        g_bound_program = p->name;
    }
    apply_render_state();
    glViewport(viewport_x, viewport_y, static_cast<GLsizei>(viewport_width), static_cast<GLsizei>(viewport_height));
    glDepthRange(g_pipe.viewport[4], g_pipe.viewport[5] != 0.0f ? g_pipe.viewport[5] : 1.0);
    bind_textures(setup);
    upload_uniforms(p, setup, viewport_width, viewport_height, viewport_x, viewport_y);

    // vertex arrays
    if (user_data) {
        uint32_t count = indexed ? vertex_count : vertices;

        upload_scratch(GL_ARRAY_BUFFER, g_scratch_vertices, vertex_data, static_cast<size_t>(count) * vertex_stride);
    }
    for (uint32_t i = 0; i < setup.element_count; i++) {
        const element &e = setup.elements[i];
        int slot = attribute_slot(e.usage, e.index);
        attribute_format format;
        uint32_t stride;
        uintptr_t offset;

        if (slot < 0 || slot >= k_attribute_slots || !attribute_format_for(e.type, &format)) {
            continue;
        }
        if (user_data) {
            stride = vertex_stride;
            offset = e.offset;
            glBindBuffer(GL_ARRAY_BUFFER, g_scratch_vertices);
        } else {
            gl_buffer *buffer = e.stream < 4 ? g_pipe.stream[e.stream] : nullptr;

            if (buffer == nullptr) {
                continue;
            }
            sync_buffer(buffer, GL_ARRAY_BUFFER);
            stride = g_pipe.stream_stride[e.stream] != 0 ? g_pipe.stream_stride[e.stream] : stream_stride_override;
            offset = static_cast<uintptr_t>(g_pipe.stream_offset[e.stream]) + e.offset
                + static_cast<uintptr_t>(static_cast<int64_t>(base_vertex + static_cast<int32_t>(start_vertex)) * static_cast<int64_t>(stride));
        }
        glEnableVertexAttribArray(static_cast<GLuint>(slot));
        glVertexAttribPointer(static_cast<GLuint>(slot), format.size, format.type, format.normalized, static_cast<GLsizei>(stride), reinterpret_cast<const void *>(offset));
        enabled_now |= 1u << slot;
        used_streams |= 1u << e.stream;
    }
    for (int slot = 0; slot < k_attribute_slots; slot++) {
        if ((g_enabled_attributes & (1u << slot)) != 0 && (enabled_now & (1u << slot)) == 0) {
            glDisableVertexAttribArray(static_cast<GLuint>(slot));
            glVertexAttrib4f(static_cast<GLuint>(slot), 0.0f, 0.0f, 0.0f, 1.0f);
        }
    }
    g_enabled_attributes = enabled_now;

    if (indexed) {
        const void *offset;

        if (user_data) {
            upload_scratch(GL_ELEMENT_ARRAY_BUFFER, g_scratch_indices, index_data, static_cast<size_t>(vertices) * index_size);
            offset = nullptr;
        } else {
            if (g_pipe.indices == nullptr) {
                return;
            }
            index_size = g_pipe.indices->format == 102 ? 4 : 2;
            index_type = g_pipe.indices->format == 102 ? GL_UNSIGNED_INT : GL_UNSIGNED_SHORT;
            sync_buffer(g_pipe.indices, GL_ELEMENT_ARRAY_BUFFER);
            offset = reinterpret_cast<const void *>(static_cast<uintptr_t>(start_index) * index_size);
        }
        glDrawElements(primitive_mode(type), static_cast<GLsizei>(vertices), index_type, offset);
    } else {
        glDrawArrays(primitive_mode(type), 0, static_cast<GLsizei>(vertices));
    }
    (void)used_streams;
    if (getenv("HALO_GL_PROBE") != nullptr && trace_probe_frame()) {
        uint8_t pixel[4] = {0, 0, 0, 0};
        static int serial;
        GLenum error = glGetError();

        if (error != 0) {
            halo::shell::standalone_log("gl probe   GL ERROR %04x after draw", error);
        }

        glReadPixels(static_cast<GLint>(g_target_width / 2), static_cast<GLint>(g_target_height / 2), 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, pixel);
        {
            uint8_t *all = static_cast<uint8_t *>(malloc(static_cast<size_t>(g_target_width) * g_target_height * 4));
            size_t lit = 0;
            static uint8_t *previous;
            static size_t previous_size;

            glReadPixels(0, 0, static_cast<GLsizei>(g_target_width), static_cast<GLsizei>(g_target_height), GL_RGBA, GL_UNSIGNED_BYTE, all);
            size_t bytes = static_cast<size_t>(g_target_width) * g_target_height * 4;

            if (previous != nullptr && previous_size == bytes) {
                for (size_t i = 0; i < bytes; i += 4) {
                    lit += (all[i] != previous[i] || all[i + 1] != previous[i + 1] || all[i + 2] != previous[i + 2]);
                }
            }
            double sum[3] = {0, 0, 0};

            for (size_t i = 0; i < bytes; i += 4) {
                sum[0] += all[i];
                sum[1] += all[i + 1];
                sum[2] += all[i + 2];
            }
            free(previous);
            previous = all;
            previous_size = bytes;
            halo::shell::standalone_log("gl probe   pixels changed by draw: %u of %ux%u, target average %.1f %.1f %.1f", static_cast<unsigned>(lit), g_target_width, g_target_height,
                sum[0] * 4 / bytes, sum[1] * 4 / bytes, sum[2] * 4 / bytes);
        }
        halo::shell::standalone_log("gl probe #%d vs=%u ps=%u count=%u -> %u %u %u %u", serial++, g_pipe.vertex_shader != nullptr ? g_pipe.vertex_shader->id : 0,
            g_pipe.pixel_shader != nullptr ? g_pipe.pixel_shader->id : 0, primitive_count, pixel[0], pixel[1], pixel[2], pixel[3]);
        {
            const float (*v)[4] = g_pipe.vertex_constants;

            const uint32_t *rs = g_pipe.render_state;

            halo::shell::standalone_log("gl probe   RS zen=%u zfunc=%u zwr=%u colorwrite=%x ablend=%u src=%u dst=%u blendop=%u cull=%u stencil=%u sfunc=%u atest=%u afunc=%u aref=%u fill=%u",
                rs[7], rs[23], rs[14], rs[168], rs[27], rs[19], rs[20], rs[171], rs[22], rs[52], rs[56], rs[15], rs[25], rs[24], rs[8]);
            halo::shell::standalone_log("gl probe   nodes vc68=%g %g %g %g vc69=%g %g %g %g vc70=%g %g %g %g vc100=%g vc150=%g vc200=%g",
                v[68][0], v[68][1], v[68][2], v[68][3], v[69][0], v[69][1], v[69][2], v[69][3], v[70][0], v[70][1], v[70][2], v[70][3], v[100][0], v[150][0], v[200][0]);
            halo::shell::standalone_log("gl probe   lights vc15=%g %g %g %g vc17=%g %g %g %g vc20=%g %g %g %g vc22=%g %g %g %g vc24=%g %g %g %g vc25=%g %g %g %g",
                v[15][0], v[15][1], v[15][2], v[15][3], v[17][0], v[17][1], v[17][2], v[17][3], v[20][0], v[20][1], v[20][2], v[20][3],
                v[22][0], v[22][1], v[22][2], v[22][3], v[24][0], v[24][1], v[24][2], v[24][3], v[25][0], v[25][1], v[25][2], v[25][3]);
            halo::shell::standalone_log("gl probe   vc5=%g %g %g %g vc9=%g %g %g %g vc10=%g %g %g %g vc29=%g %g %g %g vc30=%g %g %g %g vc31=%g %g %g %g",
                v[5][0], v[5][1], v[5][2], v[5][3], v[9][0], v[9][1], v[9][2], v[9][3], v[10][0], v[10][1], v[10][2], v[10][3],
                v[29][0], v[29][1], v[29][2], v[29][3], v[30][0], v[30][1], v[30][2], v[30][3], v[31][0], v[31][1], v[31][2], v[31][3]);
            halo::shell::standalone_log("gl probe   vc0=%g %g %g %g vc1=%g %g %g %g vc2=%g %g %g %g vc3=%g %g %g %g vp=%g %g %g %g flip=%d",
                v[0][0], v[0][1], v[0][2], v[0][3], v[1][0], v[1][1], v[1][2], v[1][3], v[2][0], v[2][1], v[2][2], v[2][3], v[3][0], v[3][1], v[3][2], v[3][3],
                g_pipe.viewport[0], g_pipe.viewport[1], g_pipe.viewport[2], g_pipe.viewport[3], g_target_flipped);
        }
        if (g_pipe.vertex_shader != nullptr && g_pipe.stream[0] != nullptr && g_pipe.indices != nullptr && indexed && !user_data) {
            const uint8_t *base = g_pipe.stream[0]->data + g_pipe.stream_offset[0] + static_cast<int64_t>(base_vertex) * g_pipe.stream_stride[0];
            const uint16_t *index = reinterpret_cast<const uint16_t *>(g_pipe.indices->data + start_index * 2);
            char text[600] = "";

            snprintf(text, sizeof(text), "stride=%u base_vertex=%d start_index=%u buffer=%u idx %u %u %u %u:", g_pipe.stream_stride[0], base_vertex, start_index, g_pipe.stream[0]->size,
                index[0], index[1], index[2], index[3]);
            for (int n = 0; n < 3; n++) {
                const uint8_t *vertex = base + static_cast<size_t>(index[n]) * g_pipe.stream_stride[0];
                const float *pos = reinterpret_cast<const float *>(vertex);
                const int16_t *bi = reinterpret_cast<const int16_t *>(vertex + 56);
                const float *bw = reinterpret_cast<const float *>(vertex + 60);

                snprintf(text + strlen(text), sizeof(text) - strlen(text), " [pos %g %g %g bi %d %d bw %g %g]", pos[0], pos[1], pos[2], bi[0], bi[1], bw[0], bw[1]);
            }
            halo::shell::standalone_log("gl probe   geometry %s", text);
        }
        if (g_pipe.vertex_shader != nullptr && g_pipe.stream[0] != nullptr && g_pipe.indices != nullptr && indexed && !user_data) {
            const uint8_t *base = g_pipe.stream[0]->data + g_pipe.stream_offset[0] + static_cast<int64_t>(base_vertex) * g_pipe.stream_stride[0];
            const uint16_t *index = reinterpret_cast<const uint16_t *>(g_pipe.indices->data + start_index * 2);
            char text[600] = "";

            snprintf(text, sizeof(text), "stride=%u base_vertex=%d start_index=%u buffer=%u idx %u %u %u %u:", g_pipe.stream_stride[0], base_vertex, start_index, g_pipe.stream[0]->size,
                index[0], index[1], index[2], index[3]);
            for (int n = 0; n < 3; n++) {
                const uint8_t *vertex = base + static_cast<size_t>(index[n]) * g_pipe.stream_stride[0];
                const float *pos = reinterpret_cast<const float *>(vertex);
                const int16_t *bi = reinterpret_cast<const int16_t *>(vertex + 56);
                const float *bw = reinterpret_cast<const float *>(vertex + 60);

                snprintf(text + strlen(text), sizeof(text) - strlen(text), " [pos %g %g %g bi %d %d bw %g %g]", pos[0], pos[1], pos[2], bi[0], bi[1], bw[0], bw[1]);
            }
            halo::shell::standalone_log("gl probe   geometry %s", text);
        }
        if (g_pipe.pixel_shader != nullptr) {
            char text[400] = "";

            for (int unit = 0; unit < 4; unit++) {
                gl_texture *tex = static_cast<gl_texture *>(g_pipe.texture[unit]);

                snprintf(text + strlen(text), sizeof(text) - strlen(text), " tex%d=%s", unit, tex == nullptr ? "none" : "");
                if (tex != nullptr) {
                    size_t level_bytes = static_cast<size_t>(tex->level[0].slice) * tex->level[0].depth;
                    size_t nonzero = 0;

                    for (size_t b = 0; b < level_bytes; b++) nonzero += tex->level[0].data[b] != 0;
                    snprintf(text + strlen(text), sizeof(text) - strlen(text), "[fmt %u %ux%u L%u nonzero %u/%u dirty %d name %u first %02x%02x%02x%02x]", tex->format, tex->width, tex->height, tex->levels,
                        static_cast<unsigned>(nonzero), static_cast<unsigned>(level_bytes), tex->dirty, tex->name,
                        tex->level[0].data[0], tex->level[0].data[1], tex->level[0].data[2], tex->level[0].data[3]);
                }
            }
            halo::shell::standalone_log("gl probe   c0=%g %g %g %g c1=%g %g %g %g%s", g_pipe.pixel_constants[0][0], g_pipe.pixel_constants[0][1], g_pipe.pixel_constants[0][2],
                g_pipe.pixel_constants[0][3], g_pipe.pixel_constants[1][0], g_pipe.pixel_constants[1][1], g_pipe.pixel_constants[1][2], g_pipe.pixel_constants[1][3], text);
        }
    }
}

}  // namespace halo::rasterizer::gl
