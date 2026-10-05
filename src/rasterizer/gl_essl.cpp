/**
 * @file src/rasterizer/gl_essl.cpp
 * Rewrites the GLSL 1.10 the backend generates (MojoShader's GLSL profile and the fixed-function stages in gl_draw.cpp)
 * as GLSL ES 3.00, the shading language of OpenGL ES 3 and WebGL 2 (desktop drivers with ARB_ES3_compatibility take
 * it too). The 1.10 built-in varyings become the io_<usage>_<index> variables MojoShader's ES profiles use, gl_FragColor
 * becomes a declared output, texture2D() and friends become texture(), #version and #extension lines are replaced.
 */

#include "gl_internal.hpp"

#include <stdio.h>
#include <string.h>

namespace halo::rasterizer::gl {

namespace {

constexpr uint32_t k_max_varyings = 32;

struct essl_writer {
    char *text;
    size_t length;
    size_t capacity;
};

void put(essl_writer &w, const char *text, size_t length)
{
    if (w.length + length + 1 > w.capacity) {
        w.capacity = (w.length + length + 1) * 2 + 256;
        w.text = static_cast<char *>(realloc(w.text, w.capacity));
    }
    memcpy(w.text + w.length, text, length);
    w.length += length;
    w.text[w.length] = '\0';
}

void append(essl_writer &w, const char *text)
{
    put(w, text, strlen(text));
}

bool identifier_char(char c)
{
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '_';
}

struct varyings {
    char declaration[k_max_varyings][40];  // "vec4 io_5_0;"
    uint32_t count;
};

void add_varying(varyings &v, const char *type, const char *name)
{
    char declaration[40];

    snprintf(declaration, sizeof(declaration), "%s %s;", type, name);
    for (uint32_t i = 0; i < v.count; i++) {
        if (strcmp(v.declaration[i], declaration) == 0) {
            return;
        }
    }
    if (v.count < k_max_varyings) {
        strcpy(v.declaration[v.count++], declaration);
    }
}

/** The ES name of a 1.10 built-in varying ("gl_TexCoord" with its index), or null. Sets *is_float for the fog coordinate. */
const char *builtin_varying(const char *name, size_t length, bool vertex, int index, bool *is_float)
{
    static char text[16];

    *is_float = false;
    if (length == 11 && strncmp(name, "gl_TexCoord", 11) == 0 && index >= 0) {
        snprintf(text, sizeof(text), "io_5_%d", index);
        return text;
    }
    if ((vertex && length == 13 && strncmp(name, "gl_FrontColor", 13) == 0) || (!vertex && length == 8 && strncmp(name, "gl_Color", 8) == 0)) {
        return "io_10_0";
    }
    if ((vertex && length == 22 && strncmp(name, "gl_FrontSecondaryColor", 22) == 0) ||
        (!vertex && length == 17 && strncmp(name, "gl_SecondaryColor", 17) == 0)) {
        return "io_10_1";
    }
    if (length == 15 && strncmp(name, "gl_FogFragCoord", 15) == 0) {
        *is_float = true;
        return "io_11_0";
    }
    return nullptr;
}

const char *renamed_function(const char *name, size_t length)
{
    static const char *const map[][2] = {
        {"texture2D", "texture"}, {"textureCube", "texture"}, {"texture3D", "texture"}, {"texture2DProj", "textureProj"},
        {"texture2DLod", "textureLod"}, {"textureCubeLod", "textureLod"}, {"texture3DLod", "textureLod"},
        {"texture2DProjLod", "textureProjLod"}, {"texture2DGrad", "textureGrad"}, {"texture2DProjGrad", "textureProjGrad"},
    };

    for (const auto &entry : map) {
        if (strlen(entry[0]) == length && strncmp(name, entry[0], length) == 0) {
            return entry[1];
        }
    }
    return nullptr;
}

/** The slot of a vertex attribute name, -1 when it is not one of the given attributes. */
int attribute_slot_of(const char *name, size_t length, const essl_attribute *attributes, uint32_t attribute_count)
{
    for (uint32_t i = 0; i < attribute_count; i++) {
        if (strlen(attributes[i].name) == length && strncmp(name, attributes[i].name, length) == 0) {
            return attributes[i].slot;
        }
    }
    return -1;
}

}  // namespace

char *essl_from_glsl(const char *source, bool vertex, const essl_attribute *attributes, uint32_t attribute_count, const char *required_outputs,
    char **fragment_inputs)
{
    essl_writer body = {};
    essl_writer main_prologue = {};
    essl_writer out = {};
    varyings used = {};
    varyings declared = {};
    bool frag_color = false;
    bool any_bgra = false;
    int depth = 0;  // brace depth after the current line
    const char *p = source;

    while (*p != '\0') {
        const char *line_end = strchr(p, '\n');
        size_t line_length = line_end != nullptr ? static_cast<size_t>(line_end - p) + 1 : strlen(p);
        const char *q = p;

        while (*q == ' ' || *q == '\t') q++;
        if (strncmp(q, "#version", 8) == 0 || strncmp(q, "#extension", 10) == 0) {
            p += line_length;
            continue;
        }
        if (strncmp(q, "varying ", 8) == 0 || strncmp(q, "attribute ", 10) == 0) {
            // "varying vec4 name;" / "attribute vec4 name;"
            bool attribute = q[0] == 'a';
            const char *type = q + (attribute ? 10 : 8);
            const char *name = strchr(type, ' ');
            const char *semicolon = strchr(type, ';');

            if (name != nullptr && semicolon != nullptr && name < semicolon && semicolon < p + line_length) {
                char type_text[16];
                char name_text[40];
                size_t type_length = static_cast<size_t>(name - type);
                size_t name_length = static_cast<size_t>(semicolon - name - 1);

                if (type_length < sizeof(type_text) && name_length < sizeof(name_text)) {
                    char line[160];

                    memcpy(type_text, type, type_length);
                    type_text[type_length] = '\0';
                    memcpy(name_text, name + 1, name_length);
                    name_text[name_length] = '\0';
                    if (attribute) {
                        int slot = attribute_slot_of(name_text, name_length, attributes, attribute_count);

                        if (slot >= 0 && strcmp(type_text, "vec4") == 0) {
                            // D3DCOLOR elements arrive as B, G, R, A bytes: u_bgra marks the slots to swap back
                            snprintf(line, sizeof(line), "in vec4 %s_raw;\nvec4 %s;\n", name_text, name_text);
                            append(body, line);
                            snprintf(line, sizeof(line), "    %s = (u_bgra & %d) != 0 ? %s_raw.zyxw : %s_raw;\n", name_text, 1 << slot, name_text, name_text);
                            append(main_prologue, line);
                            any_bgra = true;
                        } else {
                            snprintf(line, sizeof(line), "in %s %s;\n", type_text, name_text);
                            append(body, line);
                        }
                    } else {
                        add_varying(declared, type_text, name_text);
                        snprintf(line, sizeof(line), "%s %s %s;\n", vertex ? "out" : "in", type_text, name_text);
                        append(body, line);
                    }
                    p += line_length;
                    continue;
                }
            }
        }

        // token pass over the line
        const char *end = p + line_length;
        size_t line_start = body.length;

        for (const char *c = p; c < end; c++) {
            depth += *c == '{' ? 1 : *c == '}' ? -1 : 0;
        }
        while (p < end) {
            if (identifier_char(*p) && (p == source || !identifier_char(p[-1]))) {
                const char *start = p;
                const char *replacement;
                bool is_float;
                int index = -1;
                const char *after;

                while (p < end && identifier_char(*p)) p++;
                after = p;
                if ((p - start == 11 && strncmp(start, "gl_TexCoord", 11) == 0) || (p - start == 11 && strncmp(start, "gl_FragData", 11) == 0)) {
                    const char *r = p;

                    while (*r == ' ') r++;
                    if (*r == '[') {
                        index = atoi(r + 1);
                        r = strchr(r, ']');
                        if (r != nullptr) after = r + 1;
                    }
                }
                if (p - start == 12 && strncmp(start, "gl_FragColor", 12) == 0) {
                    frag_color = true;
                    append(body, "halo_FragColor");
                } else if (p - start == 11 && strncmp(start, "gl_FragData", 11) == 0 && index == 0) {
                    frag_color = true;
                    append(body, "halo_FragColor");
                    p = after;
                } else if ((replacement = builtin_varying(start, static_cast<size_t>(p - start), vertex, index, &is_float)) != nullptr) {
                    add_varying(used, is_float ? "float" : "vec4", replacement);
                    append(body, replacement);
                    p = after;
                } else if ((replacement = renamed_function(start, static_cast<size_t>(p - start))) != nullptr) {
                    append(body, replacement);
                } else {
                    put(body, start, static_cast<size_t>(p - start));
                }
                continue;
            }
            put(body, p, 1);
            p++;
        }
        if (depth == 0 && body.text != nullptr && strncmp(body.text + line_start, "vec4 ", 5) == 0) {
            // ps_1_x texture registers: "vec4 ps_t0 = gl_TexCoord[0];" at global scope needs a constant initializer in
            // GLSL ES, so declare the variable there and assign it at the start of main()
            char *line = body.text + line_start;
            char *equals = strstr(line, " = ");
            char *semicolon = strchr(line, ';');

            if (equals != nullptr && semicolon != nullptr && equals < semicolon && strstr(line, "io_") != nullptr) {
                char assignment[160];

                snprintf(assignment, sizeof(assignment), "    %.*s;\n", static_cast<int>(semicolon - line - 5), line + 5);
                append(main_prologue, assignment);
                body.length = static_cast<size_t>(equals - body.text);
                append(body, ";\n");
            }
        }
    }

    append(out, "#version 300 es\nprecision highp float;\nprecision highp int;\nprecision highp sampler2D;\nprecision highp samplerCube;\n"
              "precision highp sampler3D;\n");
    if (any_bgra) {
        append(out, "uniform int u_bgra;\n");
    }
    for (uint32_t i = 0; i < used.count; i++) {
        bool already = false;

        for (uint32_t k = 0; k < declared.count; k++) already = already || strcmp(declared.declaration[k], used.declaration[i]) == 0;
        if (!already) {
            append(out, vertex ? "out " : "in ");
            append(out, used.declaration[i]);
            append(out, "\n");
            if (declared.count < k_max_varyings) {
                strcpy(declared.declaration[declared.count++], used.declaration[i]);
            }
        }
    }
    if (vertex && required_outputs != nullptr) {
        // every input the fragment stage reads has to be declared by the vertex stage (GLSL ES link rule)
        const char *r = required_outputs;

        while (*r != '\0') {
            const char *line_end = strchr(r, '\n');
            size_t length = line_end != nullptr ? static_cast<size_t>(line_end - r) : strlen(r);
            char declaration[40];
            bool already = false;

            if (length < sizeof(declaration)) {
                memcpy(declaration, r, length);
                declaration[length] = '\0';
                for (uint32_t k = 0; k < declared.count; k++) already = already || strcmp(declared.declaration[k], declaration) == 0;
                if (!already) {
                    append(out, "out ");
                    append(out, declaration);
                    append(out, "\n");
                }
            }
            r += length + (line_end != nullptr ? 1 : 0);
        }
    }
    if (frag_color) {
        append(out, "layout(location = 0) out vec4 halo_FragColor;\n");
    }
    if (fragment_inputs != nullptr) {
        essl_writer inputs = {};

        for (uint32_t i = 0; i < declared.count; i++) {
            append(inputs, declared.declaration[i]);
            append(inputs, "\n");
        }
        *fragment_inputs = inputs.text != nullptr ? inputs.text : static_cast<char *>(calloc(1, 1));
    }
    if (main_prologue.text != nullptr) {
        // the swizzled attributes are plain globals: fill them first thing in main()
        char *main_text = body.text != nullptr ? strstr(body.text, "void main()") : nullptr;
        char *brace = main_text != nullptr ? strchr(main_text, '{') : nullptr;

        if (brace != nullptr) {
            put(out, body.text, static_cast<size_t>(brace + 1 - body.text));
            append(out, "\n");
            append(out, main_prologue.text);
            append(out, brace + 1);
        } else {
            append(out, body.text);
        }
    } else if (body.text != nullptr) {
        append(out, body.text);
    }
    free(body.text);
    free(main_prologue.text);
    return out.text;
}

}  // namespace halo::rasterizer::gl
