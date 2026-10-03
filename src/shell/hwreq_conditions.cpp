#include "halo/shell/hwreq.hpp"
#include "halo/shell/layout.hpp"

namespace halo::shell {

namespace {

struct d3dcaps_field_entry {
    const char *keyword;
    uint32_t caps_offset;
};



const d3dcaps_field_entry k_d3dcaps_fields[] = {
    { "Caps", offsetof(d3d_caps9, caps) },
    { "Caps2", offsetof(d3d_caps9, caps2) },
    { "Caps3", offsetof(d3d_caps9, caps3) },
    { "PresentationIntervals", offsetof(d3d_caps9, presentation_intervals) },
    { "CursorCaps", offsetof(d3d_caps9, cursor_caps) },
    { "DevCaps", offsetof(d3d_caps9, dev_caps) },
    { "PrimitiveMiscCaps", offsetof(d3d_caps9, primitive_misc_caps) },
    { "RasterCaps", offsetof(d3d_caps9, raster_caps) },
    { "ZCmpCaps", offsetof(d3d_caps9, z_cmp_caps) },
    { "SrcBlendCaps", offsetof(d3d_caps9, src_blend_caps) },
    { "DestBlendCaps", offsetof(d3d_caps9, dest_blend_caps) },
    { "AlphaCmpCaps", offsetof(d3d_caps9, alpha_cmp_caps) },
    { "ShadeCaps", offsetof(d3d_caps9, shade_caps) },
    { "TextureCaps", offsetof(d3d_caps9, texture_caps) },
    { "TextureFilterCaps", offsetof(d3d_caps9, texture_filter_caps) },
    { "CubeTextureFilterCaps", offsetof(d3d_caps9, cube_texture_filter_caps) },
    { "VolumeTextureFilterCaps", offsetof(d3d_caps9, volume_texture_filter_caps) },
    { "TextureAddressCaps", offsetof(d3d_caps9, texture_address_caps) },
    { "VolumeTextureAddressCaps", offsetof(d3d_caps9, volume_texture_address_caps) },
    { "LineCaps", offsetof(d3d_caps9, line_caps) },
    { "MaxTextureWidth", offsetof(d3d_caps9, max_texture_width) },
    { "MaxVolumeExtent", offsetof(d3d_caps9, max_volume_extent) },
    { "MaxTextureRepeat", offsetof(d3d_caps9, max_texture_repeat) },
    { "MaxTextureAspectRatio", offsetof(d3d_caps9, max_texture_aspect_ratio) },
    { "MaxAnisotropy", offsetof(d3d_caps9, max_anisotropy) },
    { "StencilCaps", offsetof(d3d_caps9, stencil_caps) },
    { "FVFCaps", offsetof(d3d_caps9, fvf_caps) },
    { "TextureOpCaps", offsetof(d3d_caps9, texture_op_caps) },
    { "MaxTextureBlendStages", offsetof(d3d_caps9, max_texture_blend_stages) },
    { "MaxSimultaneousTextures", offsetof(d3d_caps9, max_simultaneous_textures) },
    { "VertexProcessingCaps", offsetof(d3d_caps9, vertex_processing_caps) },
    { "MaxActiveLights", offsetof(d3d_caps9, max_active_lights) },
    { "MaxUserClipPlanes", offsetof(d3d_caps9, max_user_clip_planes) },
    { "MaxVertexBlendMatrices", offsetof(d3d_caps9, max_vertex_blend_matrices) },
    { "MaxVertexBlendMatrixIndex", offsetof(d3d_caps9, max_vertex_blend_matrix_index) },
    { "MaxPrimitiveCount", offsetof(d3d_caps9, max_primitive_count) },
    { "MaxVertexIndex", offsetof(d3d_caps9, max_vertex_index) },
    { "MaxStreams", offsetof(d3d_caps9, max_streams) },
    { "MaxStreamStride", offsetof(d3d_caps9, max_stream_stride) },
    { "VertexShaderVersion", offsetof(d3d_caps9, vertex_shader_version) },
    { "MaxVertexShaderConst", offsetof(d3d_caps9, max_vertex_shader_const) },
    { "PixelShaderVersion", offsetof(d3d_caps9, pixel_shader_version) }
};
#define k_d3dcaps_field_count (sizeof(k_d3dcaps_fields) / sizeof(k_d3dcaps_fields[0]))

}

/**
 * Parses the comparison operator and right hand side of one condition term (plain value, guid,
 * driver version or os name) and evaluates it against value. Returns 1 or 0 for a result, or an
 * error message string.
 *
 * @address 0x579690
 */
const char *HwreqParser::evaluate_condition(int32_t kind, uint32_t value)
{
    uint16_t op_word;
    int32_t op;
    char c;

    while (*(char *)self->cursor == ' ' || *(char *)self->cursor == '\t') {
        self->cursor++;
    }

    op_word = *(uint16_t *)self->cursor;
    switch (op_word) {
    case char_pair('=', '='): self->cursor += 2; op = k_hwreq_operator_equal; break;
    case char_pair('!', '='): self->cursor += 2; op = k_hwreq_operator_not_equal; break;
    case char_pair('<', '>'): self->cursor += 2; op = k_hwreq_operator_not_equal; break;
    case char_pair('=', '>'): self->cursor += 2; op = k_hwreq_operator_greater_equal; break;
    case char_pair('=', '<'): self->cursor += 2; op = k_hwreq_operator_less_equal; break;
    case char_pair('<', '='): self->cursor += 2; op = k_hwreq_operator_less_equal; break;
    case char_pair('>', '='): self->cursor += 2; op = k_hwreq_operator_greater_equal; break;
    default:
        c = *(char *)self->cursor;
        if (c == '=') { self->cursor += 1; op = k_hwreq_operator_equal; }
        else if (c == '>') { self->cursor += 1; op = k_hwreq_operator_greater; }
        else if (c == '<') { self->cursor += 1; op = k_hwreq_operator_less; }
        else if (c == '&') { self->cursor += 1; op = k_hwreq_operator_and; }
        else return "Unknown operator";
        break;
    }

    while (*(char *)self->cursor == ' ' || *(char *)self->cursor == '\t') {
        self->cursor++;
    }

    if (kind == k_hwreq_condition_guid) {
        uint32_t parsed_guid[4];
        int32_t a, b;
        const uint8_t *lhs;
        const uint8_t *rhs;
        int32_t i;
        int32_t equal;

        if (op > k_hwreq_operator_not_equal) {
            return "Only == or != allowed";
        }

        a = parse_hex_id();
        if (a == -1) return "Invalid GUID";
        b = parse_hex_id();
        if (b == -1) return "Invalid GUID";
        parsed_guid[0] = (uint32_t)(a * 0x10000 + b);
        c = *(char *)self->cursor; self->cursor++;
        if (c != '-') return "Invalid GUID";

        a = parse_hex_id();
        if (a == -1) return "Invalid GUID";
        c = *(char *)self->cursor; self->cursor++;
        if (c != '-') return "Invalid GUID";
        b = parse_hex_id();
        if (b == -1) return "Invalid GUID";
        parsed_guid[1] = (uint32_t)(b * 0x10000 + a);
        c = *(char *)self->cursor; self->cursor++;
        if (c != '-') return "Invalid GUID";

        a = parse_hex_id_byteswap();
        if (a == -1) return "Invalid GUID";
        c = *(char *)self->cursor; self->cursor++;
        if (c != '-') return "Invalid GUID";
        b = parse_hex_id_byteswap();
        if (b == -1) return "Invalid GUID";
        parsed_guid[2] = (uint32_t)(b * 0x10000 + a);

        a = parse_hex_id_byteswap();
        if (a == -1) return "Invalid GUID";
        b = parse_hex_id_byteswap();
        if (b == -1) return "Invalid GUID";
        parsed_guid[3] = (uint32_t)(b * 0x10000 + a);

        lhs = (const uint8_t *)parsed_guid;
        rhs = (const uint8_t *)self->adapter.device_identifier;
        equal = 1;
        for (i = 0; i < 16; i++) {
            if (lhs[i] != rhs[i]) { equal = 0; break; }
        }

        return (op == k_hwreq_operator_equal) ? (const char *)(uint32_t)equal
                                               : (const char *)(uint32_t)(!equal);
    }

    if (kind == k_hwreq_condition_driver) {
        int32_t n0, n1, n2, n3;
        int32_t parsed_high, parsed_low;
        uint32_t actual_low;
        int32_t actual_high;

        n0 = parse_number();
        c = *(char *)self->cursor; self->cursor++;
        if (n0 == -1 || c != '.') return "Invalid driver number";

        n1 = parse_number();
        c = *(char *)self->cursor; self->cursor++;
        if (c != '.') return "Invalid driver number";
        parsed_high = n1 + n0 * 0x10000;

        n2 = parse_number();
        c = *(char *)self->cursor; self->cursor++;
        if (n2 == -1 || c != '.') return "Invalid driver number";

        n3 = parse_number();
        parsed_low = n3 + n2 * 0x10000;

        actual_low = self->adapter.driver_version.parts.low_part;
        actual_high = self->adapter.driver_version.parts.high_part;

        switch (op) {
        case k_hwreq_operator_equal:
            if (actual_low == (uint32_t)parsed_low && actual_high == parsed_high) return (const char *)1;
            break;
        case k_hwreq_operator_not_equal:
            if (actual_low != (uint32_t)parsed_low || actual_high != parsed_high) return (const char *)1;
            break;
        case k_hwreq_operator_greater:
            if (parsed_high <= actual_high && (parsed_high < actual_high || (uint32_t)parsed_low < actual_low)) return (const char *)1;
            break;
        case k_hwreq_operator_less:
            if (actual_high <= parsed_high && (actual_high < parsed_high || actual_low < (uint32_t)parsed_low)) return (const char *)1;
            break;
        case k_hwreq_operator_greater_equal:
            if (parsed_high <= actual_high && (parsed_high < actual_high || (uint32_t)parsed_low <= actual_low)) return (const char *)1;
            break;
        case k_hwreq_operator_less_equal:
            if (actual_high <= parsed_high) {
                if (actual_high < parsed_high) return (const char *)1;
                if (actual_low <= (uint32_t)parsed_low) return (const char *)1;
            }
            break;
        default:
            return "Invalid";
        }
        return (const char *)0;
    }

    {
        uint32_t rhs;

        if (kind == k_hwreq_condition_os) {
            if (match_keyword("win95")) { rhs = k_hwreq_os_win95; self->cursor += 5; }
            else if (match_keyword("win98")) { rhs = k_hwreq_os_win98; self->cursor += 5; }
            else if (match_keyword("win98se")) { rhs = k_hwreq_os_win98se; self->cursor += 7; }
            else if (match_keyword("winme")) { rhs = k_hwreq_os_winme; self->cursor += 5; }
            else if (match_keyword("win2k")) { rhs = k_hwreq_os_win2k; self->cursor += 5; }
            else if (match_keyword("winxp")) { rhs = k_hwreq_os_winxp; self->cursor += 5; }
            else return "Unknown OS";
        } else {
            int32_t n = parse_number();
            if (n == -1) return "Number expected";
            rhs = (uint32_t)n;
        }

        switch (op) {
        case k_hwreq_operator_equal: return (const char *)(uint32_t)(value == rhs);
        case k_hwreq_operator_not_equal: return (const char *)(uint32_t)(value != rhs);
        case k_hwreq_operator_greater: return (const char *)(uint32_t)(rhs < value);
        case k_hwreq_operator_less: return (const char *)(uint32_t)(value < rhs);
        case k_hwreq_operator_greater_equal: return (const char *)(uint32_t)(rhs <= value);
        case k_hwreq_operator_less_equal: return (const char *)(uint32_t)(value <= rhs);
        case k_hwreq_operator_and: return (const char *)(uint32_t)((rhs & value) != 0);
        default: return "Invalid";
        }
    }
}

/**
 * Matches the keyword at the cursor (cpuspeed, ram, a D3DCAPS9 field, videoram, subsysid, revision,
 * guid, driver or os), fetches the left hand value and evaluates the comparison that follows.
 * Returns 1 or 0 for a result, or an error message string.
 *
 * @address 0x578ff0
 */
const char *HwreqParser::resolve_field()
{
    uint32_t i;
    int32_t kind;
    uint32_t value;
    const uint8_t *caps_base;

    while (*(char *)self->cursor == ' ' || *(char *)self->cursor == '\t') {
        self->cursor++;
    }

    kind = k_hwreq_condition_value;

    if (match_keyword("cpuspeed")) {
        value = self->cpu_speed;
        self->cursor += cstr_length("cpuspeed");
        return evaluate_condition(kind, value);
    }
    if (match_keyword("ram")) {
        value = self->memory;
        self->cursor += cstr_length("ram");
        return evaluate_condition(kind, value);
    }

    caps_base = (const uint8_t *)&self->caps;
    for (i = 0; i < k_d3dcaps_field_count; i++) {
        if (match_keyword(k_d3dcaps_fields[i].keyword)) {
            value = *(const uint32_t *)(caps_base + k_d3dcaps_fields[i].caps_offset);
            self->cursor += cstr_length(k_d3dcaps_fields[i].keyword);
            return evaluate_condition(kind, value);
        }
    }

    if (match_keyword("videoram")) {
        value = self->video_memory;
        self->cursor += cstr_length("videoram");
        return evaluate_condition(kind, value);
    }
    if (match_keyword("subsysid")) {
        value = self->adapter.subsystem_id;
        self->cursor += cstr_length("subsysid");
        return evaluate_condition(kind, value);
    }
    if (match_keyword("revision")) {
        value = self->adapter.revision;
        self->cursor += cstr_length("revision");
        return evaluate_condition(kind, value);
    }
    if (match_keyword("guid")) {
        self->cursor += cstr_length("guid");
        return evaluate_condition(k_hwreq_condition_guid, 0);
    }
    if (match_keyword("driver")) {
        self->cursor += cstr_length("driver");
        return evaluate_condition(k_hwreq_condition_driver, 0);
    }
    if (match_keyword("os")) {
        os_version_info_a version;
        uint32_t detected;

        version.size = sizeof(os_version_info_a);
        GetVersionExA((LPOSVERSIONINFOA)&version);

        if (version.platform_id == 2) {
            if (version.major_version == 5) {
                detected = (version.build_number >= k_windows_xp_build) ? k_hwreq_os_winxp : k_hwreq_os_win2k;
            } else {
                detected = k_hwreq_os_winxp;
            }
        } else {
            uint32_t build = version.build_number & 0xffff;
            if (build > k_windows_me_build - 1) detected = k_hwreq_os_winme;
            else if (build > k_windows_98se_build - 1) detected = k_hwreq_os_win98se;
            else if (build > k_windows_98_build - 1) detected = k_hwreq_os_win98;
            else detected = k_hwreq_os_win95;
        }

        self->cursor += cstr_length("os");
        return evaluate_condition(k_hwreq_condition_os, detected);
    }

    return "Unknown value";
}

}
