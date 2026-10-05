#include "win32.h"
#include "halo/shell/system.hpp"
#include "halo/core/win32_constants.hpp"
#include "halo/shell/diagnostics.hpp"
#include "halo/core/link.hpp"
#include "halo/networking/vars.hpp"
#include "halo/shell/vars.hpp"
#include "halo/networking/api.hpp"
#include "halo/shell/api.hpp"
#include "halo/platform/file.hpp"
#include "halo/platform/thread.hpp"
#include "halo/platform/memory.hpp"
#include "halo/platform/system.hpp"
#include "halo/platform/window.hpp"
#include "halo/platform/cpu.hpp"

static auto &shell_argv = halo::link::ref<char **>(halo::shell::vars().shell_argv);
static auto &shell_argc = halo::link::ref<int32_t>(halo::shell::vars().shell_argc);
static auto &k_empty_string = halo::link::ref<char>(halo::networking::vars().k_empty_string);
static auto &shell_window = halo::link::ref<void *>(halo::shell::vars().shell_window);
static auto &crypt_provider = halo::link::ref<uint32_t>(halo::shell::vars().crypt_provider);
static auto &product_id_string = halo::link::ref<char [k_product_id_string_length]>(halo::shell::vars().product_id_string);
static auto &cpu_identification_state = halo::link::ref<int32_t>(halo::shell::vars().cpu_identification_state);
static auto &cpu_vendor_string = halo::link::ref<char [0x10]>(halo::shell::vars().cpu_vendor_string);
static auto &cpu_brand_string = halo::link::ref<char [0x30]>(halo::shell::vars().cpu_brand_string);

namespace {


constexpr uint32_t k_cpuid_extended_max_leaf = 0x80000000u;
constexpr uint32_t k_cpuid_extended_features = 0x80000001u;
constexpr uint32_t k_cpuid_brand_string_first = 0x80000002u;
constexpr uint32_t k_cpuid_brand_string_last = 0x80000004u;
constexpr uint32_t k_cpuid_l1_cache_tlb = 0x80000005u;
constexpr uint32_t k_cpuid_l2_cache_tlb = 0x80000006u;
constexpr uint32_t k_sha1_digest_length = 20;
constexpr uint32_t k_machine_digital_product_id_buffer_size = 0x400;

void store_cpuid_word(char *destination, uint32_t offset, uint32_t value)
{
    memcpy(destination + offset, &value, sizeof(value));
}

}  // namespace
static auto &cpu_signature = halo::link::ref<uint32_t>(halo::shell::vars().cpu_signature);
static auto &cpu_features = halo::link::ref<uint32_t>(halo::shell::vars().cpu_features);
static auto &cpu_extended_features = halo::link::ref<uint32_t>(halo::shell::vars().cpu_extended_features);
static auto &cpu_l1_tlb_large = halo::link::ref<uint32_t>(halo::shell::vars().cpu_l1_tlb_large);
static auto &cpu_l1_tlb_4k = halo::link::ref<uint32_t>(halo::shell::vars().cpu_l1_tlb_4k);
static auto &cpu_l1_data_cache = halo::link::ref<uint32_t>(halo::shell::vars().cpu_l1_data_cache);
static auto &cpu_l1_code_cache = halo::link::ref<uint32_t>(halo::shell::vars().cpu_l1_code_cache);
static auto &cpu_l2_tlb_large = halo::link::ref<uint32_t>(halo::shell::vars().cpu_l2_tlb_large);
static auto &cpu_l2_tlb_4k = halo::link::ref<uint32_t>(halo::shell::vars().cpu_l2_tlb_4k);
static auto &cpu_l2_cache = halo::link::ref<uint32_t>(halo::shell::vars().cpu_l2_cache);
static auto &cpu_l2_unknown = halo::link::ref<uint32_t>(halo::shell::vars().cpu_l2_unknown);
static auto &os_platform_value = halo::link::ref<int32_t>(halo::shell::vars().os_platform_value);
static auto &security_write_access_state = halo::link::ref<int32_t>(halo::shell::vars().security_write_access_state);
static auto &shell_instance_mode_value = halo::link::ref<int32_t>(halo::shell::vars().shell_instance_mode_value);
static auto &shell_instance_index = halo::link::ref<int32_t>(halo::shell::vars().shell_instance_index);
static auto &shell_instance_mutex = halo::link::ref<void *>(halo::shell::vars().shell_instance_mutex);
static auto &shell_instance_mutex_names = halo::link::ref<char *[9]>(halo::shell::vars().shell_instance_mutex_names);
static auto &shell_module_path = halo::link::ref<char *>(halo::shell::vars().shell_module_path);

namespace halo::shell {

namespace {

const int32_t k_oem_digit_offsets[10] = {12, 13, 14, 15, 18, 19, 20, 21, 22, 0};
const int32_t k_retail_digit_offsets[10] = {6, 7, 8, 10, 11, 12, 13, 14, 15, 0};

uint8_t bytes_equal(const uint8_t *a, const uint8_t *b, uint32_t length)
{
    uint32_t i;
    for (i = 0; i < length; i++) {
        if (a[i] != b[i]) {
            return 0;
        }
    }
    return 1;
}

void execute_cpuid(uint32_t leaf, uint32_t *out_eax, uint32_t *out_ebx, uint32_t *out_ecx, uint32_t *out_edx)
{
    uint32_t registers[4] = {0, 0, 0, 0};

    halo::platform::cpuid(leaf, registers);
    *out_eax = registers[0]; *out_ebx = registers[1]; *out_ecx = registers[2]; *out_edx = registers[3];
}

}

/**
 * Tokenizes the raw Halo command line into a GlobalAlloc'd argv-style array of substring pointers
 * and writes the token count through out_count. argv[0] is always the shared empty string; the real
 * tokens start at argv[1]. The command line is edited in place.
 *
 * @address 0x5425f0
 */
char **CommandLine::parse_to_argv(char *command_line, int32_t *out_count)
{
    char *p;
    int32_t length;
    int32_t i;
    int32_t j;
    int32_t token_count;
    uint8_t in_flag;
    char **argv;
    char **out;
    char c;

    *out_count = 0;
    argv = 0;

    if (command_line == 0) {
        return argv;
    }

    in_flag = 0;
    p = command_line;
    do {
        c = *p;
        p++;
    } while (c != '\0');
    length = (int32_t)(p - (command_line + 1));

    i = 0;
    if (length > 0) {
        do {
            if (command_line[i] == '-') {
                in_flag = 1;
                if (i != 0 && (command_line[i - 1] == '\0' || isspace((int32_t)(int8_t)command_line[i - 1]) != 0)) {
                    command_line[i - 1] = '\0';
                }
            } else if (isspace((int32_t)(int8_t)command_line[i]) != 0 && in_flag) {
                command_line[i] = '\0';
                in_flag = 0;
            }
            if (command_line[i] == '"') {
                command_line[i] = ' ';
            }
            i++;
        } while (i < length);
    }

    i = 0;
    token_count = 0;
    if (length > 0) {
        do {
            if (command_line[i] == '\0') {
                i++;
            } else {
                token_count++;
                do {
                    j = i + 1;
                    i++;
                } while (command_line[j] != '\0');
            }
        } while (i < length);
    }

    *out_count = token_count + 1;
    argv = (char **)halo::platform::heap_allocate(0, (uint32_t)(token_count + 1) * 4);
    argv[0] = &k_empty_string;

    i = 0;
    if (length > 0) {
        out = argv + 1;
        do {
            if (command_line[i] == '\0') {
                i++;
            } else {
                while (isspace((int32_t)(int8_t)command_line[i]) != 0) {
                    i++;
                }
                *out = command_line + i;
                out++;

                c = command_line[i];
                while (c != '\0') {
                    j = i + 1;
                    i++;
                    c = command_line[j];
                }

                {
                    char *tail = command_line + i - 1;
                    while (isspace((int32_t)(int8_t)*tail) != 0) {
                        *tail = '\0';
                        tail--;
                    }
                }
            }
        } while (i < length);
    }

    return argv;
}

/**
 * Reports whether the named -flag is on the parsed command line and, when out_value is given,
 * returns the argument that follows it (null if that argument is itself a flag).
 *
 * @address 0x542760
 */
uint8_t CommandLine::has_flag(const char *flag_name, const char **out_value)
{
    int32_t i;
    char *token;

    if (out_value != 0) {
        *out_value = 0;
    }

    for (i = 0; i < shell_argc; i++) {
        token = shell_argv[i];
        if (token[0] == '-' && _stricmp(flag_name, token) == 0) {
            if (out_value != 0 && i + 1 < shell_argc) {
                char *next_token = shell_argv[i + 1];
                if (next_token[0] != '-') {
                    *out_value = next_token;
                }
            }
            return 1;
        }
    }
    return 0;
}

/**
 * Copies the current CF_TEXT clipboard contents into the caller's buffer (capacity bytes). Returns 1
 * on success and 0 when no text is available.
 *
 * @address 0x541ac0
 */
uint32_t Clipboard::get_text(char *buffer, uint32_t capacity)
{
    return halo::platform::clipboard_text(shell_window, buffer, capacity) ? 1 : 0;
}

/**
 * Parses consecutive lowercase hex digits from the string into an unsigned integer, stopping at the
 * first other character. An empty or non-hex string gives 0.
 *
 * @address 0x57d7f0
 */
int32_t HexParser::to_uint(char *string)
{
    char c;
    int32_t value;

    value = 0;
    for (;;) {
        c = *string;
        if (!((c > '/' && c < ':') || (c > '`' && c < 'g'))) {
            break;
        }
        if (c < '0' || c > '9') {
            value = value * 0x10 + (c - 'W');
        } else {
            value = value * 0x10 + (c - '0');
        }
        string++;
    }
    return value;
}

/**
 * Decodes pairs of lowercase hex digits from source into bytes at dest, stopping at the first
 * character that is not a hex digit.
 *
 * @address 0x57d830
 */
void HexParser::to_bytes(uint8_t *dest, const char *source)
{
    char c;

    for (;;) {
        c = *source;
        if (!((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f'))) {
            break;
        }
        if (c >= '0' && c <= '9') {
            c = (char)(c - '0');
        } else {
            c = (char)(c - 'W');
        }
        *dest = (uint8_t)(c << 4);
        source++;

        c = *source;
        if (c >= '0' && c <= '9') {
            *dest = (uint8_t)(*dest + (c - '0'));
        } else {
            *dest = (uint8_t)(*dest + (c - 'W'));
        }
        dest++;
        source++;
    }
}

/**
 * Computes the SHA-1 digest of length bytes with the shared cryptographic provider and writes the 20
 * byte result to digest_out. Returns 1 on success and 0 on any provider failure.
 *
 * @address 0x57f2a0
 */
uint8_t Sha1::hash(const uint8_t *data, uint32_t length, uint8_t *digest_out)
{
    return halo::platform::sha1(reinterpret_cast<void *>(static_cast<uintptr_t>(crypt_provider)), data, length, digest_out) ? 1 : 0;
}

/**
 * Computes the SHA-1 digest and, on success, copies its first 8 bytes to output when output is not
 * null. Returns the digest call's success flag.
 *
 * @address 0x57f330
 */
uint8_t Sha1::hash_first_qword(const uint8_t *data, uint32_t length, uint32_t *output)
{
    uint32_t digest[5];
    uint8_t ok;

    ok = hash(data, length, (uint8_t *)digest);
    if (ok != 0 && output != 0) {
        output[0] = digest[0];
        output[1] = digest[1];
    }
    return ok;
}

/**
 * Selects the OEM or retail digit offset table (an "OEM" tag at product_id[6..8]) and collects the
 * digits found there. Returns the digits as a decimal number, or -1 when a table offset lands on a
 * non-digit character.
 *
 * @address 0x57f360
 */
int32_t ProductId::extract_digits(const char *product_id)
{
    const int32_t *offsets;
    char digits[12];
    int32_t i;
    char c;

    if ((product_id[6] == 'O' || product_id[6] == 'o') && (product_id[7] == 'E' || product_id[7] == 'e') &&
        (product_id[8] == 'M' || product_id[8] == 'm')) {
        offsets = k_oem_digit_offsets;
    } else {
        offsets = k_retail_digit_offsets;
    }

    i = 0;
    while (offsets[i] != 0) {
        c = product_id[offsets[i]];
        if (!isdigit((int32_t)c)) {
            break;
        }
        digits[i] = c;
        i++;
    }

    if (offsets[i] != 0) {
        return -1;
    }
    digits[i] = 0;
    return atol(digits);
}

/**
 * Reads the DigitalProductID setting, validates its header (size 0xa4, version 3.0) and formats
 * "<id>,<product id digits>,0,<hash prefix>" into product_id_string. Returns that string, or the
 * shared empty string when the setting is missing or malformed or the cryptographic provider fails.
 *
 * @address 0x57f3f0
 */
char *ProductId::build_string()
{
    digital_product_id data;
    uint32_t data_size;
    uint8_t digest[20];
    int64_t hash_prefix;
    int32_t product_id_digits;

    product_id_string[0] = 0;
    data_size = k_machine_digital_product_id_buffer_size;

    if (!SettingsStore::current().read_value(SettingsScope::machine, "DigitalProductID", 0, &data, &data_size)) {
        return &k_empty_string;
    }

    if (data.size != k_digital_product_id_size || data.major_version != k_digital_product_id_major_version ||
        data.minor_version != 0) {
        return &k_empty_string;
    }

    product_id_digits = extract_digits(data.product_id);

    crypt_provider = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(halo::platform::crypto_context_open()));
    if (crypt_provider == 0) {
        return &k_empty_string;
    }
    if (Sha1::hash(data.hashed_key, k_digital_product_id_hashed_bytes, digest) == 0) {
        return &k_empty_string;
    }
    if (Sha1::hash_first_qword(digest, sizeof(digest), (uint32_t *)&hash_prefix) == 0) {
        return &k_empty_string;
    }

    sprintf(product_id_string, "%05d,%09d,0,% 19.19I64d", data.unknown_20, product_id_digits, hash_prefix);
    halo::platform::crypto_context_close(reinterpret_cast<void *>(static_cast<uintptr_t>(crypt_provider)));
    return product_id_string;
}

/**
 * Detects (once, then cached in cpu_identification_state) whether CPUID exists by toggling the
 * EFLAGS.ID bit and, if so, fills the cached vendor, feature and cache/TLB words and the brand
 * string. Returns 1 on success and -1 when the processor has no CPUID instruction.
 *
 * @address 0x540da0
 */
int32_t Cpu::query_identification()
{
    uint32_t registers[4];
    uint32_t eax, ebx, ecx, edx;
    uint32_t max_extended_leaf;

    if (!halo::platform::cpuid(0, registers)) {
        return -1;  // no CPUID (the EFLAGS.ID bit cannot be toggled, or not an x86 processor)
    }

    execute_cpuid(0, &eax, &ebx, &ecx, &edx);
    store_cpuid_word(cpu_vendor_string, 0, ebx);
    store_cpuid_word(cpu_vendor_string, 4, edx);
    store_cpuid_word(cpu_vendor_string, 8, ecx);

    if (eax == 0) {
        return 1;
    }

    execute_cpuid(1, &eax, &ebx, &ecx, &edx);
    cpu_signature = eax;
    cpu_features = edx;

    execute_cpuid(k_cpuid_extended_max_leaf, &eax, &ebx, &ecx, &edx);
    max_extended_leaf = eax;
    if (max_extended_leaf <= k_cpuid_extended_max_leaf) {
        return 1;
    }

    if (max_extended_leaf > k_cpuid_brand_string_last) {
        if (max_extended_leaf > k_cpuid_l1_cache_tlb) {
            execute_cpuid(k_cpuid_l2_cache_tlb, &eax, &ebx, &ecx, &edx);
            cpu_l2_tlb_large = eax;
            cpu_l2_tlb_4k = ebx;
            cpu_l2_unknown = edx;
            cpu_l2_cache = ecx;
        }
        execute_cpuid(k_cpuid_l1_cache_tlb, &eax, &ebx, &ecx, &edx);
        cpu_l1_tlb_large = eax;
        cpu_l1_tlb_4k = ebx;
        cpu_l1_code_cache = edx;
        cpu_l1_data_cache = ecx;

        execute_cpuid(k_cpuid_brand_string_first, &eax, &ebx, &ecx, &edx);
        store_cpuid_word(cpu_brand_string, 0, eax);
        store_cpuid_word(cpu_brand_string, 4, ebx);
        store_cpuid_word(cpu_brand_string, 8, ecx);
        store_cpuid_word(cpu_brand_string, 12, edx);

        execute_cpuid(k_cpuid_brand_string_first + 1, &eax, &ebx, &ecx, &edx);
        store_cpuid_word(cpu_brand_string, 16, eax);
        store_cpuid_word(cpu_brand_string, 20, ebx);
        store_cpuid_word(cpu_brand_string, 24, ecx);
        store_cpuid_word(cpu_brand_string, 28, edx);

        execute_cpuid(k_cpuid_brand_string_last, &eax, &ebx, &ecx, &edx);
        store_cpuid_word(cpu_brand_string, 32, eax);
        store_cpuid_word(cpu_brand_string, 36, ebx);
        store_cpuid_word(cpu_brand_string, 40, ecx);
        store_cpuid_word(cpu_brand_string, 44, edx);
    }

    execute_cpuid(k_cpuid_extended_features, &eax, &ebx, &ecx, &edx);
    cpu_extended_features = edx;
    return 1;
}

/**
 * Answers one cpu_query value: vendor and model enums, the cached string addresses, feature bits and,
 * on AMD processors, cache and TLB descriptor fields (the large page and L2 TLB fields on Athlon
 * only). CPUID data is gathered on first use.
 *
 * @address 0x5402a0
 */
int32_t Cpu::get_type(int32_t mode)
{
    int32_t result;

    if (cpu_identification_state == 0) {
        cpu_identification_state = query_identification();
    }
    if (cpu_identification_state == -1) {
        return 0;
    }

    switch (mode) {
    case k_cpu_query_vendor:
        if (strncmp(cpu_vendor_string, "AuthenticAMD", 0xc) == 0) return k_cpu_vendor_amd;
        if (strncmp(cpu_vendor_string, "GenuineIntel", 0xc) == 0) return k_cpu_vendor_intel;
        if (strncmp(cpu_vendor_string, "CyrixInstead", 0xc) == 0) return k_cpu_vendor_cyrix;
        if (strncmp(cpu_vendor_string, "CentaurHauls", 0xc) == 0) return k_cpu_vendor_centaur;
        return k_cpu_vendor_unknown;

    case k_cpu_query_model:
        result = get_type(k_cpu_query_vendor);
        if (result == k_cpu_vendor_amd) {
            uint32_t family;
            family = (cpu_signature >> 8) & 0xf;
            if (family == 4) return k_cpu_model_amd_family4;
            if (family == 5) {
                switch ((cpu_signature >> 4) & 0xf) {
                case 0: case 1: case 2: case 3: return k_cpu_model_amd_k5;
                case 4: case 5: case 6: case 7: return k_cpu_model_amd_k6;
                case 8: return k_cpu_model_amd_k6_2;
                case 9: case 10: case 11: case 12: case 13: case 14: case 15:
                    return k_cpu_model_amd_k6_3;
                default: return k_cpu_model_unknown;
                }
            }
            if (family == 6) return k_cpu_model_amd_athlon;
            return k_cpu_model_unknown;
        }
        if (result == k_cpu_vendor_intel) {
            uint32_t family = (cpu_signature >> 8) & 0xf;
            if (family == 4) {
                switch ((cpu_signature >> 4) & 0xf) {
                case 0: case 1: return k_cpu_model_intel_486dx;
                case 2: return k_cpu_model_intel_486sx;
                case 3: return k_cpu_model_intel_486dx2;
                case 4: return k_cpu_model_intel_486sl;
                case 5: return k_cpu_model_intel_486sx2;
                default: return k_cpu_model_unknown;
                case 7: return k_cpu_model_intel_486dx2_write_back;
                case 8: return k_cpu_model_intel_486dx4;
                }
            }
            if (family == 5) {
                switch ((cpu_signature >> 4) & 0xf) {
                case 1: case 2: case 3: return k_cpu_model_intel_pentium;
                case 4: return k_cpu_model_intel_pentium_mmx;
                default: return k_cpu_model_unknown;
                }
            }
            if (family == 6) {
                switch ((cpu_signature >> 4) & 0xf) {
                case 1: return k_cpu_model_intel_pentium_pro;
                default: return k_cpu_model_unknown;
                case 3: case 5: return k_cpu_model_intel_pentium_2;
                case 6: return k_cpu_model_intel_celeron;
                case 7: return k_cpu_model_intel_pentium_3;
                }
            }
            return k_cpu_model_unknown;
        }
        return k_cpu_model_unknown;

    case k_cpu_query_vendor_string: return (int32_t)cpu_vendor_string;
    case k_cpu_query_brand_string:  return (int32_t)cpu_brand_string;
    case k_cpu_query_cpuid_available: return 1;

    case k_cpu_query_fpu:    return cpu_features & 1;
    case k_cpu_query_vme:    return (cpu_features >> 1) & 1;
    case k_cpu_query_de:     return (cpu_features >> 2) & 1;
    case k_cpu_query_pse:    return (cpu_features >> 3) & 1;
    case k_cpu_query_tsc:    return (cpu_features >> 4) & 1;
    case k_cpu_query_msr:    return (cpu_features >> 5) & 1;
    case k_cpu_query_pae:    return (cpu_features >> 6) & 1;
    case k_cpu_query_mce:    return (cpu_features >> 7) & 1;
    case k_cpu_query_cx8:    return (cpu_features >> 8) & 1;
    case k_cpu_query_apic:   return (cpu_features >> 9) & 1;
    case k_cpu_query_sep:    return (cpu_features >> 0xb) & 1;
    case k_cpu_query_mtrr:   return (cpu_features >> 0xc) & 1;
    case k_cpu_query_pge:    return (cpu_features >> 0xd) & 1;
    case k_cpu_query_mca:    return (cpu_features >> 0xe) & 1;
    case k_cpu_query_cmov:   return (cpu_features >> 0xf) & 1;
    case k_cpu_query_pat:    return (cpu_features >> 0x10) & 1;
    case k_cpu_query_pse36:  return (cpu_features >> 0x11) & 1;

    case k_cpu_query_mmx_extensions:
        return ((cpu_features >> 3) | cpu_extended_features) >> 0x16 & 1;

    case k_cpu_query_mmx:    return (cpu_features >> 0x17) & 1;
    case k_cpu_query_fxsr:   return (cpu_features >> 0x18) & 1;
    case k_cpu_query_3dnow_extensions: return (cpu_extended_features >> 0x1e) & 1;
    case k_cpu_query_3dnow:             return cpu_extended_features >> 0x1f;
    case k_cpu_query_amd_mmx_extensions: return (cpu_extended_features >> 0x16) & 1;
    case k_cpu_query_sse:    return (cpu_features >> 0x19) & 1;
    case k_cpu_query_sse_usable:
        return get_type(k_cpu_query_sse) != 0 ? 1 : 0;
    case k_cpu_query_sse2:   return (cpu_features >> 0x1a) & 1;
    case k_cpu_query_sse2_usable:
        return get_type(k_cpu_query_sse2) != 0 ? 1 : 0;

    default:
        if (get_type(k_cpu_query_vendor) != k_cpu_vendor_amd) {
            return 0;
        }

        result = 0;
        switch (mode) {
        case k_cpu_query_l1_tlb_4k_data_associativity: result = cpu_l1_tlb_4k >> 0x18; break;
        case k_cpu_query_l1_tlb_4k_data_entries:       result = (cpu_l1_tlb_4k >> 0x10) & 0xff; break;
        case k_cpu_query_l1_tlb_4k_code_associativity: result = (cpu_l1_tlb_4k >> 8) & 0xff; break;
        case k_cpu_query_l1_tlb_4k_code_entries:       result = cpu_l1_tlb_4k & 0xff; break;
        case k_cpu_query_l1_data_cache_size:           result = cpu_l1_data_cache >> 0x18; break;
        case k_cpu_query_l1_data_cache_associativity:  result = (cpu_l1_data_cache >> 0x10) & 0xff; break;
        case k_cpu_query_l1_data_cache_lines_per_tag:  result = (cpu_l1_data_cache >> 8) & 0xff; break;
        case k_cpu_query_l1_data_cache_line_size:      result = cpu_l1_data_cache & 0xff; break;
        case k_cpu_query_l1_code_cache_size:           result = cpu_l1_code_cache >> 0x18; break;
        case k_cpu_query_l1_code_cache_associativity:  result = (cpu_l1_code_cache >> 0x10) & 0xff; break;
        case k_cpu_query_l1_code_cache_lines_per_tag:  result = (cpu_l1_code_cache >> 8) & 0xff; break;
        case k_cpu_query_l1_code_cache_line_size:      result = cpu_l1_code_cache & 0xff; break;
        case k_cpu_query_l2_cache_size:                result = cpu_l2_cache >> 0x10; break;
        case k_cpu_query_l2_cache_associativity:       result = (cpu_l2_cache >> 0xc) & 0xf; break;
        case k_cpu_query_l2_cache_lines_per_tag:       result = (cpu_l2_cache >> 8) & 0xf; break;
        case k_cpu_query_l2_cache_line_size:           result = cpu_l2_cache & 0xff; break;
        default: break;
        }

        if (get_type(k_cpu_query_model) == k_cpu_model_amd_athlon) {
            switch (mode) {
            case k_cpu_query_l1_tlb_large_data_associativity: return cpu_l1_tlb_large >> 0x18;
            case k_cpu_query_l1_tlb_large_data_entries:       return (cpu_l1_tlb_large >> 0x10) & 0xff;
            case k_cpu_query_l1_tlb_large_code_associativity: return (cpu_l1_tlb_large >> 8) & 0xff;
            case k_cpu_query_l1_tlb_large_code_entries:       return cpu_l1_tlb_large & 0xff;
            case k_cpu_query_l2_tlb_large_data_associativity: return cpu_l2_tlb_large >> 0x1c;
            case k_cpu_query_l2_tlb_large_data_entries:       result = (cpu_l2_tlb_large >> 0x10) & 0xfff; break;
            case k_cpu_query_l2_tlb_large_code_associativity: return (cpu_l2_tlb_large >> 0xc) & 0xf;
            case k_cpu_query_l2_tlb_large_code_entries:       result = cpu_l2_tlb_large & 0xfff; break;
            case k_cpu_query_l2_tlb_4k_data_associativity:    return cpu_l2_tlb_4k >> 0x1c;
            case k_cpu_query_l2_tlb_4k_data_entries:          result = (cpu_l2_tlb_4k >> 0x10) & 0xfff; break;
            case k_cpu_query_l2_tlb_4k_code_associativity:    return (cpu_l2_tlb_4k >> 0xc) & 0xf;
            case k_cpu_query_l2_tlb_4k_code_entries:          result = cpu_l2_tlb_4k & 0xfff; break;
            default: break;
            }
        }
        return result;
    }
}

/**
 * Queries GetVersionExA once and caches the platform family (Win9x, NT or other) in
 * os_platform_value.
 *
 * @address 0x5427e0
 */
void OperatingSystem::identify()
{
    os_version_info_a info;

    info.size = 0x94;
    if (halo::platform::os_version(&info) != 0) {
        if (info.platform_id != 1) {
            if (info.platform_id != 2) {
                os_platform_value = k_os_platform_other;
                return;
            }
            os_platform_value = k_os_platform_windows_nt;
            return;
        }
        os_platform_value = k_os_platform_windows_9x;
    }
}

/**
 * Releases the security descriptor, ACL, SID and the two token handles that differ from the sentinel
 * (null on every known path).
 *
 * @address 0x542a80
 */
void SecurityResources::release(void *descriptor, void *acl, void *sid, void *thread_token, void *impersonation_token,
                                void *sentinel)
{
#if defined(_WIN32)
    if (descriptor != sentinel) {
        LocalFree(descriptor);
    }
    if (acl != sentinel) {
        LocalFree(acl);
    }
    if (sid != sentinel) {
        FreeSid(sid);
    }
    if (thread_token != sentinel) {
        CloseHandle(thread_token);
    }
    if (impersonation_token != sentinel) {
        CloseHandle(impersonation_token);
    }
#else
    (void)descriptor;
    (void)acl;
    (void)sid;
    (void)thread_token;
    (void)impersonation_token;
    (void)sentinel;  // nothing is ever allocated off Windows
#endif
}

/**
 * Determines (once, then cached in security_write_access_state) whether the current thread token has
 * write access under a synthetic ACL, which detects restricted accounts on NT systems. Non-NT
 * platforms always report access granted.
 *
 * @address 0x542840
 */
int32_t WriteAccessCheck::run()
{
    SecurityResources resources;
    uint32_t sid_length;
    uint32_t acl_length;
    uint32_t privilege_set_length;
    int32_t access_granted;
    int32_t ok;
    sid_identifier_authority nt_authority;
    generic_mapping mapping;
    uint8_t privilege_set[0x14];
    uint32_t granted_access;

    if (security_write_access_state != -1) {
        return security_write_access_state == 1;
    }

    if (os_platform_value == 0) {
        OperatingSystem::identify();
    }
    if (os_platform_value < k_os_platform_windows_nt) {
        security_write_access_state = k_shell_access_check_granted;
        return security_write_access_state == 1;
    }

#if defined(_WIN32)
    access_granted = 0;
    privilege_set_length = 0x14;
    nt_authority.value[0] = 0;
    nt_authority.value[1] = 0;
    nt_authority.value[2] = 0;
    nt_authority.value[3] = 0;
    nt_authority.value[4] = 0;
    nt_authority.value[5] = 5;

    ok = OpenThreadToken(GetCurrentThread(), TOKEN_QUERY | TOKEN_DUPLICATE, 1, (PHANDLE)&resources.thread_token);
    if (ok == 0) {
        if (GetLastError() == ERROR_NO_TOKEN) {
            ok = OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY | TOKEN_DUPLICATE, (PHANDLE)&resources.thread_token);
        }
    }

    if (ok != 0) {
        ok = DuplicateToken(resources.thread_token, SecurityImpersonation, (PHANDLE)&resources.impersonation_token);
        if (ok != 0 &&
            (ok = AllocateAndInitializeSid((PSID_IDENTIFIER_AUTHORITY)&nt_authority, 2, SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_ALIAS_RID_ADMINS, 0, 0, 0, 0, 0, 0,
                                           (PSID *)&resources.sid), ok != 0) &&
            (resources.descriptor = LocalAlloc(LPTR, SECURITY_DESCRIPTOR_MIN_LENGTH), resources.descriptor != 0) &&
            (ok = InitializeSecurityDescriptor(resources.descriptor, SECURITY_DESCRIPTOR_REVISION), ok != 0)) {
            sid_length = GetLengthSid(resources.sid);
            acl_length = sid_length + 0x10;
            resources.acl = LocalAlloc(LPTR, acl_length);
            if (resources.acl != 0 && (ok = InitializeAcl((PACL)resources.acl, acl_length, ACL_REVISION), ok != 0)) {
                ok = AddAccessAllowedAce((PACL)resources.acl, 2, 3, resources.sid);
                if (ok != 0 && (ok = SetSecurityDescriptorDacl(resources.descriptor, 1, (PACL)resources.acl, 0), ok != 0)) {
                    SetSecurityDescriptorGroup(resources.descriptor, resources.sid, 0);
                    SetSecurityDescriptorOwner(resources.descriptor, resources.sid, 0);
                    ok = IsValidSecurityDescriptor(resources.descriptor);
                    if (ok != 0) {
                        mapping.generic_read = 1;
                        mapping.generic_write = 2;
                        mapping.generic_execute = 0;
                        mapping.generic_all = 3;
                        ok = AccessCheck(resources.descriptor, resources.impersonation_token, 1, (PGENERIC_MAPPING)&mapping,
                                         (PPRIVILEGE_SET)privilege_set, (LPDWORD)&privilege_set_length,
                                         (LPDWORD)&granted_access, &access_granted);
                        if (ok == 0) {
                            access_granted = 0;
                        }
                    }
                }
            }
        }
    }

#else
    // no NT access tokens off Windows: as on the platforms without them, access is granted
    (void)sid_length;
    (void)acl_length;
    (void)privilege_set_length;
    (void)nt_authority;
    (void)mapping;
    (void)privilege_set;
    (void)granted_access;
    (void)ok;
    access_granted = 1;
#endif
    security_write_access_state = (access_granted == 1);
    return security_write_access_state == 1;
}

/**
 * Takes the named instance mutex (slot 0 for single mode, the first free of slots 1..8 for multiple
 * mode). When another copy owns it, that copy's window is brought to the foreground and this process
 * exits; if no window is found a fatal error is shown.
 *
 * @address 0x542d70
 */
void SingleInstance::check(int32_t mode)
{
    os_version_info_a version;
    int32_t first_index;
    int32_t last_index;
    int32_t i;
    uint32_t last_error;
    const char *name;

    shell_instance_mode_value = mode;
    shell_instance_index = -1;

    version.size = 0x94;
    halo::platform::os_version(&version);

    if (mode == k_shell_instance_mode_multiple) {
        first_index = 1;
        last_index = k_shell_instance_mutex_multi_last;
    } else {
        first_index = 0;
        last_index = 0;
    }

    last_error = 1;
    for (i = first_index; i <= last_index; i++) {
        name = shell_instance_mutex_names[i];
        if (version.major_version < 5) {
            name += 7;
        }
        shell_instance_mutex = halo::platform::mutex_create(true, name);
        last_error = halo::platform::last_error();
        if (last_error == halo::win32::k_error_already_exists) {
            if (shell_instance_mutex != 0) {
                halo::platform::handle_close(shell_instance_mutex);
            }
        } else if (shell_instance_mutex != 0) {
            shell_instance_index = i - first_index;
            break;
        }
    }

    if (!(last_error != halo::win32::k_error_already_exists && shell_instance_mutex == 0)) {
        if (last_error != halo::win32::k_error_already_exists && shell_instance_index != -1) {
            return;
        }
        release();
    }

    if (mode != k_shell_instance_mode_multiple) {
        if (halo::platform::window_activate_existing("Halo", "Halo")) {
            _exit(1);
        }
    }
    FatalError::show(0x92, (uint32_t)((const char *)0x7e), 1);
}

/**
 * Closes the instance mutex if one is held and forgets the instance slot.
 */
void SingleInstance::release()
{
    if (shell_instance_mutex != 0) {
        halo::platform::handle_close(shell_instance_mutex);
        shell_instance_mutex = 0;
        shell_instance_mode_value = k_shell_instance_mode_none;
        shell_instance_index = -1;
    }
}

/**
 * Records a normal exit by writing "clean" to the ExitFlag setting.
 *
 * @address 0x57ea10
 */
void ExitFlag::set_clean()
{
    SettingsStore::current().write_string(SettingsScope::user, "ExitFlag", "clean", 6);
}

/**
 * Checks whether the previous run left ExitFlag at "bad 1" or "bad 2" and advances it: "bad 1"
 * becomes "bad 2" and anything else becomes "bad 1". Skipped when a .pdb sits next to the running
 * module (debug build). Returns 1 when the flag already said "bad 2" (nothing is rewritten), else 0.
 *
 * @address 0x57e850
 */
int32_t ExitFlag::previous_run_crashed()
{
    uint32_t exit_flag_size;
    uint8_t exit_flag[16];
    char pdb_path[264];
    char *end;
    uint32_t length;
    const SettingsStore &store = SettingsStore::current();

    exit_flag_size = 0x10;
    exit_flag[0] = 0;
    store.read_value(SettingsScope::user, "ExitFlag", 0, exit_flag, &exit_flag_size);

    length = 0;
    while (shell_module_path[length] != 0) {
        pdb_path[length] = shell_module_path[length];
        length++;
    }
    pdb_path[length] = 0;

    end = pdb_path + length;
    if (length >= 4 && end[-4] == '.') {
        end[-3] = 'p';
        end[-2] = 'd';
        end[-1] = 'b';
    }

    if (halo::platform::file_attributes(pdb_path) != k_datum_index_none) {
        return 0;
    }

    if (bytes_equal(exit_flag, (const uint8_t *)"bad 2", 6)) {
        return 1;
    }

    if (bytes_equal(exit_flag, (const uint8_t *)"bad 1", 6)) {
        store.write_string(SettingsScope::user, "ExitFlag", "bad 2", 6);
    } else {
        store.write_string(SettingsScope::user, "ExitFlag", "bad 1", 6);
    }

    return 0;
}

}
