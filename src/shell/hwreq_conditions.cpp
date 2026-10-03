#include "halo/shell/hwreq.hpp"

namespace halo::shell {

namespace {

struct d3dcaps_field_entry {
    const char *keyword;
    uint32_t caps_offset;
};



const d3dcaps_field_entry k_d3dcaps_fields[] = {
    { "Caps", 0x08 }, { "Caps2", 0x0c }, { "Caps3", 0x10 },
    { "PresentationIntervals", 0x14 }, { "CursorCaps", 0x18 }, { "DevCaps", 0x1c },
    { "PrimitiveMiscCaps", 0x20 }, { "RasterCaps", 0x24 }, { "ZCmpCaps", 0x28 },
    { "SrcBlendCaps", 0x2c }, { "DestBlendCaps", 0x30 }, { "AlphaCmpCaps", 0x34 },
    { "ShadeCaps", 0x38 }, { "TextureCaps", 0x3c }, { "TextureFilterCaps", 0x40 },
    { "CubeTextureFilterCaps", 0x44 }, { "VolumeTextureFilterCaps", 0x48 },
    { "TextureAddressCaps", 0x4c }, { "VolumeTextureAddressCaps", 0x50 }, { "LineCaps", 0x54 },
    { "MaxTextureWidth", 0x58 }, { "MaxVolumeExtent", 0x60 }, { "MaxTextureRepeat", 0x64 },
    { "MaxTextureAspectRatio", 0x68 }, { "MaxAnisotropy", 0x6c }, { "StencilCaps", 0x88 },
    { "FVFCaps", 0x8c }, { "TextureOpCaps", 0x90 }, { "MaxTextureBlendStages", 0x94 },
    { "MaxSimultaneousTextures", 0x98 }, { "VertexProcessingCaps", 0x9c },
    { "MaxActiveLights", 0xa0 }, { "MaxUserClipPlanes", 0xa4 },
    { "MaxVertexBlendMatrices", 0xa8 }, { "MaxVertexBlendMatrixIndex", 0xac },
    { "MaxPrimitiveCount", 0xb4 }, { "MaxVertexIndex", 0xb8 }, { "MaxStreams", 0xbc },
    { "MaxStreamStride", 0xc0 }, { "VertexShaderVersion", 0xc4 },
    { "MaxVertexShaderConst", 0xc8 }, { "PixelShaderVersion", 0xcc }
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
    case 0x3d3d: self->cursor += 2; op = k_hwreq_operator_equal; break;
    case 0x3d21: self->cursor += 2; op = k_hwreq_operator_not_equal; break;
    case 0x3e3c: self->cursor += 2; op = k_hwreq_operator_not_equal; break;
    case 0x3e3d: self->cursor += 2; op = k_hwreq_operator_greater_equal; break;
    case 0x3c3d: self->cursor += 2; op = k_hwreq_operator_less_equal; break;
    case 0x3d3c: self->cursor += 2; op = k_hwreq_operator_less_equal; break;
    case 0x3d3e: self->cursor += 2; op = k_hwreq_operator_greater_equal; break;
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

        version.size = 0x94;
        GetVersionExA((LPOSVERSIONINFOA)&version);

        if (version.platform_id == 2) {
            if (version.major_version == 5) {
                detected = (version.build_number >= 0xa28) ? k_hwreq_os_winxp : k_hwreq_os_win2k;
            } else {
                detected = k_hwreq_os_winxp;
            }
        } else {
            uint32_t build = version.build_number & 0xffff;
            if (build > 0x8ae) detected = k_hwreq_os_winme;
            else if (build > 0x7ce) detected = k_hwreq_os_win98se;
            else if (build > 0x3b6) detected = k_hwreq_os_win98;
            else detected = k_hwreq_os_win95;
        }

        self->cursor += cstr_length("os");
        return evaluate_condition(k_hwreq_condition_os, detected);
    }

    return "Unknown value";
}

}
