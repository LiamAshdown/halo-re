// hwreq_parser_parse  (not a Ghidra function; hwreq_parser vtable slot 0x00)
// address 0x579bd0, size 793 bytes
// name confidence: 0.8  rewrite confidence: 0.85
// evidence: hwreq_parser_vtable (types/shell.h, 0x006721e8) slot 0x00; shell_parse_config_txt 0x57d410 calls it
//   with config.txt's path, the first sound device, the adapter identifier and caps, and the memory / video memory
//   / cpu speed figures (ret 0x1c: seven stack arguments). Only reachable through the vtable, so Ghidra never made
//   it a function; first-boot track: the standalone exe stopped here (0x579bd0 had no C).
// Rewritten from objdump 0x579bd0..0x579ee8:
//   - the flags (+0x18) and requirements (+0x1c) property sets are allocated on first use (operator new 0x14,
//     vector pointers cleared, owner = parser; the vector's allocator byte at +0 is left as allocated)
//   - the sound device (0x1a dwords), adapter identifier (0x113 dwords) and caps (0x4c dwords) are copied in;
//     memory -> +0xb4, video memory -> +0xb8, cpu speed -> +0xb0; the error latch (+0x20) is cleared
//   - when the adapter's DriverVersion is zero, the driver file's version resource ("\" through VerQueryValueA,
//     VS_FIXEDFILEINFO) supplies it: dwFileVersionMS -> HighPart (+0x4e0), dwFileVersionLS -> LowPart (+0x4dc)
//   - CreateFileA(path, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0); on failure the message
//     "Cannot find '<current directory>\config.txt'" (0x00672378 with the 12 bytes at 0x0067238c appended) goes
//     through sprintf into 0x00723058 and then into the error_message string (+0x24), and the result is 0
//   - otherwise the whole file is read into operator new(size + 0x10), a CR is stored at [size], and the five
//     passes run with the cursor, line start and line number (1) reset before each of them except the
//     applytoall_or_vendor scan: find Requirements, propertyset directives, applytoall-or-vendor scan, vendor
//     blocks, audiovendor blocks, applytoall scan. The buffer is freed on every exit (the pointer at +4 is left
//     dangling, as in the original); the result is 1 only when every pass succeeded.
// blam-cc: ECX -> parser, stack -> path, sound_device, adapter, caps, memory, video_memory, cpu_speed

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "rasterizer.h"
#include "shell.h"

extern void *operator_new(uint32_t size); // 0x6277da, MSVC CRT (0x627db7 is a jmp to it)
extern void free(void *block); // 0x6277e8, CRT free (0x627dbc is a jmp to it)
extern uint32_t __stdcall GetFileVersionInfoSizeA(const char *filename, uint32_t *handle); // 0x638f3a, VERSION.dll (delay-load)
extern int32_t __stdcall GetFileVersionInfoA(const char *filename, uint32_t handle, uint32_t length, void *data); // 0x638f4a
extern int32_t __stdcall VerQueryValueA(const void *block, const char *sub_block, void **buffer, uint32_t *length); // 0x638f2a
extern void *__stdcall CreateFileA(const char *path, uint32_t access, uint32_t share_mode, void *security,
    uint32_t disposition, uint32_t flags, void *template_file); // 0x0063a2b8 IAT
extern uint32_t __stdcall GetCurrentDirectoryA(uint32_t length, char *buffer); // 0x0063a14c IAT
extern uint32_t __stdcall GetFileSize(void *file, uint32_t *high); // 0x0063a2b4 IAT
extern int32_t __stdcall ReadFile(void *file, void *buffer, uint32_t bytes_to_read, uint32_t *bytes_read, void *overlapped); // 0x0063a2d8 IAT
extern int32_t __stdcall CloseHandle(void *object); // 0x0063a2f8 IAT
extern int32_t sprintf(char *buffer, const char *format, ...); // 0x623693 CRT
extern msvc_std_string *msvc_string_assign_n(msvc_std_string *this, const char *source, uint32_t count); // 0x57bc90

extern uint8_t hwreq_parser_find_requirements_section(hwreq_parser *parser); // 0x57ae50, blam-cc: ESI -> parser
extern uint8_t hwreq_parser_parse_propertyset_directive(hwreq_parser *parser); // 0x57a3e0, blam-cc: ECX -> parser
extern uint8_t hwreq_parser_scan_for_applytoall_or_vendor(hwreq_parser *parser); // 0x57a220, blam-cc: ESI -> parser
extern uint8_t hwreq_parser_parse_vendor_block(hwreq_parser *parser); // 0x57a680, blam-cc: EAX -> parser
extern uint8_t hwreq_parser_parse_audiovendor_block(hwreq_parser *parser); // 0x57aa40, blam-cc: EAX -> parser
extern uint8_t hwreq_parser_scan_for_applytoall(hwreq_parser *parser); // 0x57a320, blam-cc: ESI -> parser

extern char hwreq_open_error_text[]; // 0x00723058
extern const char hwreq_cannot_find_format[]; // 0x00672378 "Cannot find '%s'"
extern const char hwreq_config_file_suffix[]; // 0x0067238c "\\config.txt" (12 bytes with the terminator)
extern const char hwreq_version_root_block[]; // 0x006600e8 "\\"

static hwreq_property_set *hwreq_property_set_new(hwreq_parser *parser)
{
    hwreq_property_set *set = (hwreq_property_set *)operator_new(sizeof(hwreq_property_set));

    if (set == 0) {
        return 0;
    }
    set->flags.first = 0;
    set->flags.last = 0;
    set->flags.end = 0;
    set->owner = (uint32_t)parser;
    return set;
}

static void hwreq_parser_rewind(hwreq_parser *parser)
{
    parser->cursor = parser->file_buffer;
    parser->line_start = parser->file_buffer;
    parser->line_number = 1;
}

uint8_t hwreq_parser_parse(hwreq_parser *parser, const char *path, const shell_sound_device *sound_device,
    const d3d_adapter_identifier9 *adapter, const d3d_caps9 *caps, uint32_t memory, uint32_t video_memory,
    uint32_t cpu_speed)
{
    uint32_t *driver_version = (uint32_t *)((uint8_t *)&parser->adapter + 0x420); // DriverVersion (LowPart, HighPart)
    char directory[0x104];
    char *end;
    void *file;
    uint32_t size;
    uint32_t bytes_read;
    char *buffer;
    int32_t i;

    if (parser->flags == 0) {
        parser->flags = (uint32_t)hwreq_property_set_new(parser);
    }
    if (parser->requirements == 0) {
        parser->requirements = (uint32_t)hwreq_property_set_new(parser);
    }
    parser->sound_device = *sound_device;
    parser->memory = memory;
    parser->cpu_speed = cpu_speed;
    parser->video_memory = video_memory;
    parser->adapter = *adapter;
    parser->caps = *caps;
    parser->error_reported = 0;

    if ((driver_version[0] | driver_version[1]) == 0) {
        uint32_t handle;
        uint32_t info_size = GetFileVersionInfoSizeA((const char *)&parser->adapter, &handle);

        if (info_size != 0) {
            void *info = operator_new(info_size);
            void *fixed;
            uint32_t fixed_length;

            if (GetFileVersionInfoA((const char *)&parser->adapter, handle, info_size, info) &&
                VerQueryValueA(info, hwreq_version_root_block, &fixed, &fixed_length)) {
                uint32_t fixed_info[0xd]; // VS_FIXEDFILEINFO

                for (i = 0; i < 0xd; i++) {
                    fixed_info[i] = ((uint32_t *)fixed)[i];
                }
                driver_version[1] = fixed_info[2]; // dwFileVersionMS
                driver_version[0] = fixed_info[3]; // dwFileVersionLS
            }
            free(info);
        }
    }

    file = CreateFileA(path, 0x80000000 /* GENERIC_READ */, 1 /* FILE_SHARE_READ */, 0, 3 /* OPEN_EXISTING */, 0,
                       0);
    if (file == (void *)-1) {
        GetCurrentDirectoryA(sizeof(directory), directory);
        for (end = directory; *end; end++) {
        }
        for (i = 0; i < 12; i++) {
            end[i] = hwreq_config_file_suffix[i];
        }
        sprintf(hwreq_open_error_text, hwreq_cannot_find_format, directory);
        for (end = hwreq_open_error_text; *end; end++) {
        }
        msvc_string_assign_n(&parser->error_message, hwreq_open_error_text, (uint32_t)(end - hwreq_open_error_text));
        return 0;
    }

    size = GetFileSize(file, 0);
    buffer = (char *)operator_new(size + 0x10);
    parser->file_buffer = (uint32_t)buffer;
    ReadFile(file, buffer, size, &bytes_read, 0);
    CloseHandle(file);
    buffer[size] = '\r';
    parser->cursor = (uint32_t)buffer;
    parser->line_start = (uint32_t)buffer;
    parser->line_number = 1;
    parser->end = (uint32_t)(buffer + size);

    if (!hwreq_parser_find_requirements_section(parser)) {
        free((void *)parser->file_buffer);
        return 0;
    }
    hwreq_parser_rewind(parser);
    if (!hwreq_parser_parse_propertyset_directive(parser) || !hwreq_parser_scan_for_applytoall_or_vendor(parser)) {
        free((void *)parser->file_buffer);
        return 0;
    }
    hwreq_parser_rewind(parser);
    if (!hwreq_parser_parse_vendor_block(parser)) {
        free((void *)parser->file_buffer);
        return 0;
    }
    hwreq_parser_rewind(parser);
    if (!hwreq_parser_parse_audiovendor_block(parser) || !hwreq_parser_scan_for_applytoall(parser)) {
        free((void *)parser->file_buffer);
        return 0;
    }
    free((void *)parser->file_buffer);
    return 1;
}
