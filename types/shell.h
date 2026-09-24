// Blam shell module (halo.exe 1.0.10 retail, 0x5402a0..0x57f52c plus five bogus entries at
// 0x6bd180..0x6bd1b8, 115 Ghidra functions). This is the Win32 host around the engine:
//   - process start-up and teardown: shell_winmain 0x5411e0 (command line, first-run EULA,
//     hardware checks, DLL loading), engine_initialize_subsystems / engine_shutdown_subsystems,
//     the message pump, the window procedure (0x541b30, which Ghidra has no function for; the
//     entry 0x542380 is one of its tails), the single-instance mutexes, the clipboard;
//   - CPU identification (the AMD sample-code style cpu_get_type query dispatcher over cached
//     CPUID leaves 0, 1, 0x80000001..0x80000006);
//   - the Keystone UI middleware loader (keystone.dll, Call_Ks* / Call_KW* / Call_KC* exports);
//   - the crash reporter (unhandled exception filter, in-memory dialog template, Dr. Watson
//     dw15.exe shared memory block) and the fatal error dialog / localized string loader;
//   - config.txt and the hardware requirements script parser (a C++ component: vtable, MSVC 7.1
//     std::string / std::vector / std::map members), plus the hardware detection feeding it
//     (GlobalMemoryStatus, rdtsc CPU speed, DirectDraw adapters, DxDiag sound devices);
//   - the DigitalProductID based product id string (CryptoAPI SHA-1).
//
// Offsets in comments are byte offsets from the struct base. Where the binary itself carries a
// layout it is preferred over the decompiler and the fact is called out:
//   - hwreq_parser (0x6b8) is the operator new size in hwreq_parser_create 0x57b4c0. Its three
//     embedded records are block copies in the parse method 0x579bd0 (vtable slot 0, not a Ghidra
//     function): rep movsd of 0x1a dwords to +0x638 (shell_sound_device), 0x113 dwords to +0xbc
//     (d3d_adapter_identifier9) and 0x4c dwords to +0x508 (d3d_caps9). The d3d_caps9 field names
//     are pinned a second time by hwreq_d3dcaps_field_resolve 0x578ff0, whose keyword string
//     (passed in EDX) and field load (mov esi,[edi+off]) pairs match the SDK layout one for one.
//   - The five std::string members, the two std::map heads and the property-set pointers come
//     from hwreq_parser_construct 0x579ef0 and hwreq_parser_destruct 0x57a010, which touch every
//     one of them; the vtable (16 slots at 0x006721e8) holds one accessor per member.
//   - The container layouts are the MSVC 7.1 (Dinkumware) ones: std::string 0x1c (buffer +4,
//     size +0x14, capacity +0x18, capacity 0xf means the inline buffer), vector 0x10, map 0x0c,
//     map node 0x30 (tree_head_node_allocate 0x57cbf0 and tree_node_allocate 0x57cc30).
//   - shell_sound_device (0x68) is the stride of the DxDiag loop in shell_detect_hardware_specs
//     0x57d880 (puVar8 += 0x34 words) and the 0x104 dword memset of the ten-entry array.
//   - shell_display_adapter (0x34) is the imul 0x34 of the DirectDrawEnumerateExA callback
//     0x57d370 (not a Ghidra function) and of the config.txt adapter lookup.
//   - dw_shared_memory (0x1c50) is the CreateFileMappingA size and the dwSize the crash
//     reporter stores; the memset is 0x714 dwords.
//   - digital_product_id (0xa4) is the size / version header shell_build_product_id_string
//     0x57f3f0 validates (0xa4, 3, 0).
//   - crash_dialog_template / crash_dialog_item_template are the DLGTEMPLATE / DLGITEMTEMPLATE
//     heads the crash reporter writes into its GlobalAlloc(0x40, 0x400) block.
//
// Types this module operates on that already have a definition, and are therefore NOT
// redefined here:
//   types/rasterizer.h  d3d_caps9 (the hwreq parser embeds one, config.txt fills a local one)
//   types/math.h        large_integer (driver versions)
//   types/interface.h   win32_rect, input_guid, and the chat_gui_* typedefs for the Keystone
//                       slots the chat code calls (0x00721eb8 .. 0x00721ee8); those slots are
//                       listed below by address only, so this header does not need interface.h
// So a translation unit needs tags.h, memory.h, math.h and rasterizer.h before this header, as
// out/phase4/shell_smoke.c does.
//
// Pointer convention: every pointer field inside a struct is held as uint32_t with the pointee
// type written first in its comment (the rasterizer.h / effects.h convention), so the 32 bit
// sizes hold on any host compiler. Function pointer typedefs are real function pointer types.
//
// Functions in this address range that are misattributed, library code or not real functions
// are listed in out/phase4/shell_types_notes.md; their types are not defined here.

#pragma pack(push, 1)
typedef unsigned char uint8_t; typedef signed char int8_t; typedef unsigned short uint16_t; typedef short int16_t;
typedef unsigned int uint32_t; typedef int int32_t;

// ---------------------------------------------------------------------------
// module wide constants
// ---------------------------------------------------------------------------
typedef enum shell_constants {
    k_shell_window_name_length = 0x40,              // class name and title buffers at 0x007461d4 / 0x00746214
    k_shell_instance_mutex_count = 9,               // names at 0x0069eab8; mode 1 walks entries 1..8
    k_shell_instance_mutex_multi_last = 8,
    k_shell_path_length = 0x104,                    // MAX_PATH as every GetTempPath / GetModuleFileName call passes it

    k_shell_maximum_display_adapters = 10,          // enum callback 0x57d370 stops at 0xa
    k_shell_maximum_sound_devices = 10,             // 0x104 dwords / 0x68
    k_shell_sound_device_count_clamp = 9,           // DxDiag child count is clamped to 9
    k_shell_device_name_length = 0x20,              // strncpy 0x1f plus terminator, both tables

    k_shell_config_property_count = 0x1c,           // config.txt property table 0x0069fe40
    k_shell_config_maximum_resolution_default = 0x1000, // config_reset_system_requirements
    k_shell_config_maximum_resolution_minimum = 0x280,  // MaximumResolution setter 0x57d080 accepts 0x280..0x1000
    k_shell_config_message_length = 0x100,          // 0x00722e58 and 0x00722f58 sprintf buffers

    k_shell_required_cpu_speed_default = 733,       // 0x2dd MHz, shell_parse_config_txt
    k_shell_required_memory_default = 128,          // 0x80 MB
    k_shell_required_video_memory_default = 32,     // 0x20 MB
    k_shell_required_disk_space_default = 100,      // MB of the temp volume
    k_shell_required_directx_build_default = 902,   // 0x386, fourth field of 4.09.00.0902
    k_shell_physical_memory_clamp = 0x40000000,     // GlobalMemoryStatus total is clamped to 1 GB

    k_shell_language_default = 0x409,               // en-US, 0x0069ff20 initializer
    k_shell_fatal_error_text_length = 0x400,
    k_shell_fatal_error_title_length = 0x80,
    k_shell_fatal_error_readme_length = 0x200,
    k_shell_exception_string_length = 0x100,
    k_shell_eula_name_length = 0x20,
    k_shell_strings_dll_error_length = 0x400,

    k_crash_dialog_template_allocation = 0x400,     // GlobalAlloc(GMEM_ZEROINIT, 0x400)
    k_crash_dialog_font_size = 8,                   // "MS Sans Serif" 8
    k_crash_dialog_text_characters = 0x32,          // MultiByteToWideChar limit for each string

    k_hwreq_parser_size = 0x6b8,                    // operator new in hwreq_parser_create
    k_hwreq_property_set_size = 0x14,               // operator new in 0x579bd0 and 0x57a3e0
    k_hwreq_quoted_string_length = 0x100,           // 0x00722d58, overflow test against 0x00722e57
    k_hwreq_flag_name_length = 0x100,               // parse_flag_assignment stack buffers, 0xfe usable
    k_hwreq_error_context_length = 0x24,            // report_error copies at most 36 line characters
    k_hwreq_maximum_hex_digits = 8,                 // "Number too large" past 8 digits

    k_msvc_string_inline_capacity = 0xf,            // capacity 0xf means the inline buffer is in use

    k_dw_shared_memory_size = 0x1c50,
    k_dw_behavior_flags = 0x100,
    k_dw_offer_flags = 1,
    k_dw_let_run_flags = 0x11,
    k_dw_alive_timeout = 600000,                    // ms, WaitForSingleObject on the alive event
    k_dw_mutex_timeout = 20000,
    k_dw_dxdiag_timeout = 60000,                    // MsgWaitForMultipleObjects on dxdiag.exe

    k_digital_product_id_size = 0xa4,
    k_digital_product_id_major_version = 3,
    k_digital_product_id_hashed_bytes = 0xf,        // SHA-1 over 15 bytes at +0x38
    k_product_id_string_length = 0x80               // 0x00722bd8 .. 0x00722c58 (bounded by next global)
} shell_constants;

// ---------------------------------------------------------------------------
// CPU identification  (cpu_get_type 0x5402a0, cpu_query_identification 0x540da0)
// ---------------------------------------------------------------------------
// cpu_query_identification caches CPUID results in the globals listed at the end and returns 1
// (or -1 when the EFLAGS.ID bit cannot be toggled); cpu_get_type(query) decodes them. The
// result of cpu_get_type(k_cpu_query_vendor) is a cpu_vendor, of k_cpu_query_model a cpu_model,
// of the two string queries the address of the cached string, and of every other query a
// feature bit (0 / 1) or a cache / TLB descriptor field.
typedef enum cpu_vendor {
    k_cpu_vendor_unknown = 0,
    k_cpu_vendor_amd = 1,                           // "AuthenticAMD"
    k_cpu_vendor_intel = 2,                         // "GenuineIntel"
    k_cpu_vendor_cyrix = 3,                         // "CyrixInstead"
    k_cpu_vendor_centaur = 4                        // "CentaurHauls"
} cpu_vendor;

// cpu_get_type(1): vendor plus family (bits 8..11) and model (bits 4..7) of CPUID leaf 1 EAX.
// Names follow the family / model the code tests; Cyrix and Centaur always give 0.
typedef enum cpu_model {
    k_cpu_model_unknown = 0,
    k_cpu_model_amd_family4 = 1,                    // 486 class
    k_cpu_model_amd_k5 = 2,                         // family 5, model 0..3
    k_cpu_model_amd_k6 = 3,                         // family 5, model 4..7
    k_cpu_model_amd_k6_2 = 4,                       // family 5, model 8
    k_cpu_model_amd_k6_3 = 5,                       // family 5, model 9..15
    k_cpu_model_amd_athlon = 6,                     // family 6; gates the 0x24..0x27 / 0x34..0x3b queries
    k_cpu_model_intel_486dx = 7,                    // family 4, model 0 / 1
    k_cpu_model_intel_486sx = 8,                    // model 2
    k_cpu_model_intel_486dx2 = 9,                   // model 3
    k_cpu_model_intel_486sl = 10,                   // model 4
    k_cpu_model_intel_486sx2 = 11,                  // model 5
    k_cpu_model_intel_486dx2_write_back = 12,       // model 7
    k_cpu_model_intel_486dx4 = 13,                  // model 8
    k_cpu_model_intel_pentium = 14,                 // family 5, model 1..3
    k_cpu_model_intel_pentium_mmx = 15,             // family 5, model 4
    k_cpu_model_intel_pentium_pro = 16,             // family 6, model 1
    k_cpu_model_intel_pentium_2 = 17,               // family 6, model 3 / 5
    k_cpu_model_intel_celeron = 18,                 // family 6, model 6
    k_cpu_model_intel_pentium_3 = 19                // family 6, model 7
} cpu_model;

// The query argument of cpu_get_type. 0x05..0x15 are CPUID.1 EDX bits 0..17 in order, skipping
// bit 10; the cache / TLB queries (0x20..0x3b) answer only on AMD processors.
typedef enum cpu_query {
    k_cpu_query_vendor = 0x00,                      // returns cpu_vendor
    k_cpu_query_model = 0x01,                       // returns cpu_model
    k_cpu_query_vendor_string = 0x02,               // returns 0x006e35ac
    k_cpu_query_brand_string = 0x03,                // returns 0x006e357c
    k_cpu_query_cpuid_available = 0x04,             // always 1 once past the -1 check
    k_cpu_query_fpu = 0x05,                         // leaf 1 EDX bit 0
    k_cpu_query_vme = 0x06,
    k_cpu_query_de = 0x07,
    k_cpu_query_pse = 0x08,
    k_cpu_query_tsc = 0x09,
    k_cpu_query_msr = 0x0a,
    k_cpu_query_pae = 0x0b,
    k_cpu_query_mce = 0x0c,
    k_cpu_query_cx8 = 0x0d,
    k_cpu_query_apic = 0x0e,                        // bit 9
    k_cpu_query_sep = 0x0f,                         // bit 11
    k_cpu_query_mtrr = 0x10,
    k_cpu_query_pge = 0x11,
    k_cpu_query_mca = 0x12,
    k_cpu_query_cmov = 0x13,
    k_cpu_query_pat = 0x14,
    k_cpu_query_pse36 = 0x15,                       // bit 17
    k_cpu_query_mmx_extensions = 0x16,              // leaf 1 EDX bit 25 (SSE) or 0x80000001 EDX bit 22
    k_cpu_query_mmx = 0x17,                         // bit 23
    k_cpu_query_fxsr = 0x18,                        // bit 24
    k_cpu_query_3dnow_extensions = 0x19,            // 0x80000001 EDX bit 30
    k_cpu_query_3dnow = 0x1a,                       // 0x80000001 EDX bit 31; math_initialize picks the 3DNow! matrix path
    k_cpu_query_amd_mmx_extensions = 0x1b,          // 0x80000001 EDX bit 22
    k_cpu_query_sse = 0x1c,                         // leaf 1 EDX bit 25
    k_cpu_query_sse_usable = 0x1d,                  // same bit through 0x1c; math_initialize picks the SSE matrix path
    k_cpu_query_sse2 = 0x1e,                        // bit 26
    k_cpu_query_sse2_usable = 0x1f,                 // same bit through 0x1e
    k_cpu_query_l1_tlb_4k_data_associativity = 0x20, // 0x80000005 EBX bits 24..31
    k_cpu_query_l1_tlb_4k_data_entries = 0x21,
    k_cpu_query_l1_tlb_4k_code_associativity = 0x22,
    k_cpu_query_l1_tlb_4k_code_entries = 0x23,
    k_cpu_query_l1_tlb_large_data_associativity = 0x24, // 0x80000005 EAX, Athlon only
    k_cpu_query_l1_tlb_large_data_entries = 0x25,
    k_cpu_query_l1_tlb_large_code_associativity = 0x26,
    k_cpu_query_l1_tlb_large_code_entries = 0x27,
    k_cpu_query_l1_data_cache_size = 0x28,          // 0x80000005 ECX bits 24..31 (KB)
    k_cpu_query_l1_data_cache_associativity = 0x29,
    k_cpu_query_l1_data_cache_lines_per_tag = 0x2a,
    k_cpu_query_l1_data_cache_line_size = 0x2b,
    k_cpu_query_l1_code_cache_size = 0x2c,          // 0x80000005 EDX
    k_cpu_query_l1_code_cache_associativity = 0x2d,
    k_cpu_query_l1_code_cache_lines_per_tag = 0x2e,
    k_cpu_query_l1_code_cache_line_size = 0x2f,
    k_cpu_query_l2_cache_size = 0x30,               // 0x80000006 ECX bits 16..31 (KB)
    k_cpu_query_l2_cache_associativity = 0x31,
    k_cpu_query_l2_cache_lines_per_tag = 0x32,
    k_cpu_query_l2_cache_line_size = 0x33,
    k_cpu_query_l2_tlb_large_data_associativity = 0x34, // 0x80000006 EAX, Athlon only
    k_cpu_query_l2_tlb_large_data_entries = 0x35,
    k_cpu_query_l2_tlb_large_code_associativity = 0x36,
    k_cpu_query_l2_tlb_large_code_entries = 0x37,
    k_cpu_query_l2_tlb_4k_data_associativity = 0x38, // 0x80000006 EBX, Athlon only
    k_cpu_query_l2_tlb_4k_data_entries = 0x39,
    k_cpu_query_l2_tlb_4k_code_associativity = 0x3a,
    k_cpu_query_l2_tlb_4k_code_entries = 0x3b
} cpu_query;

// ---------------------------------------------------------------------------
// operating system  (os_platform_identify 0x5427e0, security_check_write_access 0x542840)
// ---------------------------------------------------------------------------
typedef enum os_platform {
    k_os_platform_unknown = 0,                      // not yet identified; callers identify lazily
    k_os_platform_other = 1,                        // GetVersionExA dwPlatformId other than 1 / 2
    k_os_platform_windows_9x = 2,                   // VER_PLATFORM_WIN32_WINDOWS
    k_os_platform_windows_nt = 3                    // VER_PLATFORM_WIN32_NT; the only one that runs the ACL test
} os_platform;

typedef enum shell_access_check_state {
    k_shell_access_check_unknown = -1,              // 0x0069eab0 initializer
    k_shell_access_check_denied = 0,
    k_shell_access_check_granted = 1                // also forced on non-NT platforms
} shell_access_check_state;

// game_single_instance_check(mode): mode 0 takes mutex 0 only, mode 1 walks mutexes 1..8 and
// records the free slot (0..7) in 0x00721f04. Both are also the value of 0x0069eab4.
typedef enum shell_instance_mode {
    k_shell_instance_mode_none = -1,
    k_shell_instance_mode_single = 0,
    k_shell_instance_mode_multiple = 1
} shell_instance_mode;

// ---------------------------------------------------------------------------
// hardware requirements script (config.txt)
// ---------------------------------------------------------------------------
// Comparison operator decoded by hwreq_d3dcaps_field_resolve 0x578ff0 / 0x579690.
typedef enum hwreq_operator {
    k_hwreq_operator_equal = 0,                     // "==" or "="
    k_hwreq_operator_not_equal = 1,                 // "!=" or "<>"
    k_hwreq_operator_greater = 2,                   // ">"
    k_hwreq_operator_less = 3,                      // "<"
    k_hwreq_operator_greater_equal = 4,             // ">=" or "=>"
    k_hwreq_operator_less_equal = 5,                // "<=" or "=<"
    k_hwreq_operator_and = 6                        // "&", nonzero bitwise and
} hwreq_operator;

// Left-hand side kinds of one condition term (iVar8 in the decompile).
typedef enum hwreq_condition_kind {
    k_hwreq_condition_value = 0,                    // cpuspeed, ram, videoram, subsysid, revision or a caps field
    k_hwreq_condition_guid = 1,                     // "guid", == and != only, against the adapter DeviceIdentifier
    k_hwreq_condition_driver = 2,                   // "driver", a.b.c.d against the adapter DriverVersion
    k_hwreq_condition_os = 3                        // "os", against hwreq_os
} hwreq_condition_kind;

// "os" values: keyword order of the resolver and the GetVersionExA mapping (NT 5.0 builds below
// 2600 are win2k; 9x builds below 951 / 1999 / 2223 are win95 / win98 / win98se).
typedef enum hwreq_os {
    k_hwreq_os_win95 = 0,
    k_hwreq_os_win98 = 1,
    k_hwreq_os_win98se = 2,
    k_hwreq_os_winme = 3,
    k_hwreq_os_win2k = 4,
    k_hwreq_os_winxp = 5
} hwreq_os;

// ---------------------------------------------------------------------------
// MSVC 7.1 standard library layouts embedded by the hwreq parser
// (the container member functions themselves are library code; see the notes)
// ---------------------------------------------------------------------------
// std::basic_string<char>. Inline buffer while capacity == 0xf, heap pointer in the first four
// buffer bytes once capacity > 0xf (every reader tests capacity < 0x10).
typedef struct msvc_std_string {
    uint8_t allocator;                              // 0x00 empty std::allocator
    uint8_t pad_01[3];                              // 0x01
    union {
        char inline_buffer[16];                     // 0x04 capacity <= 0xf
        uint32_t heap_buffer;                       // 0x04 char *, capacity > 0xf
    } buffer;
    uint32_t size;                                  // 0x14 length without terminator
    uint32_t capacity;                              // 0x18 0xf when empty
} msvc_std_string;                                  // size 0x1c

// std::vector<T>.
typedef struct msvc_std_vector {
    uint8_t allocator;                              // 0x00 empty std::allocator
    uint8_t pad_01[3];                              // 0x01
    uint32_t first;                                 // 0x04 T *, NULL when never allocated
    uint32_t last;                                  // 0x08 T *, one past the last element
    uint32_t end;                                   // 0x0c T *, one past the capacity
} msvc_std_vector;                                  // size 0x10

// std::map<K, V> (a red-black _Tree with a head sentinel node).
typedef struct msvc_std_map {
    uint8_t comparator_and_allocator;               // 0x00 empty std::less and allocator
    uint8_t pad_01[3];                              // 0x01
    uint32_t head;                                  // 0x04 hwreq_map_node *, sentinel; left = min, parent = root, right = max
    uint32_t size;                                  // 0x08 element count
} msvc_std_map;                                     // size 0x0c

// std::pair<std::string, std::string>: one flag of a property set (name, value).
typedef struct hwreq_string_pair {
    msvc_std_string first;                          // 0x00 flag name, compared with _stricmp
    msvc_std_string second;                         // 0x1c flag value
} hwreq_string_pair;                                // size 0x38

// Node of std::map<std::string, hwreq_property_set *>. The head node has is_nil = 1 and
// color = 1 (black); tree_node_allocate stores left / parent / right, copies the key and value
// and clears is_nil.
typedef struct hwreq_map_node {
    uint32_t left;                                  // 0x00 hwreq_map_node *
    uint32_t parent;                                // 0x04 hwreq_map_node *
    uint32_t right;                                 // 0x08 hwreq_map_node *
    msvc_std_string key;                            // 0x0c property set name or graphic detail level
    uint32_t value;                                 // 0x28 hwreq_property_set *
    uint8_t color;                                  // 0x2c 0 red, 1 black
    uint8_t is_nil;                                 // 0x2d 1 only on the head sentinel
    uint8_t pad_2e[2];                              // 0x2e
} hwreq_map_node;                                   // size 0x30

// ---------------------------------------------------------------------------
// hardware detection records
// ---------------------------------------------------------------------------
// One DxDiag_DirectSound.DxDiag_SoundDevices child (shell_detect_hardware_specs 0x57d880).
// The ids are parsed out of the lower-cased hardware id string after "ven_", "dev_", "subsys_"
// and "rev_" (an nForce device is replaced by a fixed id string first). The whole record is
// copied into hwreq_parser.sound_device for the audiovendor blocks.
typedef struct shell_sound_device {
    char description[0x20];                         // 0x00 szDescription, strncpy 0x1f and terminator at 0x1f
    uint8_t unknown_20[0x20];                       // 0x20 never written by this module
    uint32_t guid[4];                               // 0x40 GUID: Data1 (hex), Data2 / Data3 (hex, 16 bit), Data4 (8 bytes);
                                                    //      compared with the default device GUID from GetDeviceID
    large_integer driver_version;                   // 0x50 "%d.%d.%d.%d": low = c<<16|d, high = a<<16|b (0x57e08e..0x57e0a7)
    uint32_t vendor_id;                             // 0x58 hwreq_parser +0x690, compared by the audiovendor block
    uint32_t device_id;                             // 0x5c hwreq_parser +0x694
    uint32_t subsystem_id;                          // 0x60
    uint32_t revision;                              // 0x64
} shell_sound_device;                               // size 0x68

// One DirectDrawEnumerateExA adapter (callback 0x57d370). Entry 0 is the primary display and
// keeps a zero GUID; the name is left empty when it is 0x1f characters or longer.
typedef struct shell_display_adapter {
    uint32_t guid[4];                               // 0x00 copied from lpGUID for entries past 0
    char driver_name[0x20];                         // 0x10 lpDriverName, matched against the adapter DeviceName
    uint32_t video_memory;                          // 0x30 bytes: the smallest nonzero IDirectDraw7::GetAvailableVidMem
                                                    //      total over four caps masks, rounded up to 8 / 32 / 64 MB
} shell_display_adapter;                            // size 0x34

// D3DADAPTER_IDENTIFIER9, the public SDK layout. Filled by IDirect3D9::GetAdapterIdentifier (vtable
// +0x14) in shell_parse_config_txt 0x57d410 and copied into hwreq_parser +0xbc. Fields read by
// this module are marked (used).
typedef struct d3d_adapter_identifier9 {
    char driver[0x200];                             // 0x000 (used) GetFileVersionInfoA source when driver_version is 0
    char description[0x200];                        // 0x200
    char device_name[0x20];                         // 0x400 (used) matched against shell_display_adapter.driver_name
    large_integer driver_version;                   // 0x420 (used) "driver" conditions; copied to 0x00722ba0
    uint32_t vendor_id;                             // 0x428 (used) vendor blocks; copied to 0x00722b9c
    uint32_t device_id;                             // 0x42c (used) vendor device lines; copied to 0x00722b98
    uint32_t subsystem_id;                          // 0x430 (used) "subsysid"
    uint32_t revision;                              // 0x434 (used) "revision"
    uint32_t device_identifier[4];                  // 0x438 (used) "guid", compared as 16 bytes
    uint32_t whql_level;                            // 0x448
} d3d_adapter_identifier9;                          // size 0x44c

// ---------------------------------------------------------------------------
// hardware requirements parser  (vtable 0x006721e8, created by hwreq_parser_create 0x57b4c0)
// ---------------------------------------------------------------------------
// A set of flag = value pairs: the output of a Requirements, applytoall, vendor or propertyset
// block. The flag list is what hwreq_property_set_upsert / hwreq_device_override_list_find 0x578410 / 0x578630
// walk (begin at +4, end at +8, stride 0x38).
typedef struct hwreq_property_set {
    msvc_std_vector flags;                          // 0x00 std::vector<hwreq_string_pair>
    uint32_t owner;                                 // 0x10 hwreq_parser *
} hwreq_property_set;                               // size 0x14

// The parser object. The tokenizer helpers take it in a register that LTCG chose per helper
// (EAX, ECX, EDX, ESI or EDI, see the notes).
typedef struct hwreq_parser {
    uint32_t vtable;                                // 0x000 hwreq_parser_vtable *, 0x006721e8
    uint32_t file_buffer;                           // 0x004 char *, malloc(file size + 0x10), a CR is stored at [file size]
    uint32_t cursor;                                // 0x008 char *
    uint32_t end;                                   // 0x00c char *, file_buffer + file size
    uint32_t line_start;                            // 0x010 char *, context for error messages
    int32_t line_number;                            // 0x014 1 at the start of every pass
    uint32_t flags;                                 // 0x018 hwreq_property_set *, flags applied for this machine
                                                    //       (vtable 0x10 / 0x14 / 0x18)
    uint32_t requirements;                          // 0x01c hwreq_property_set *, the Requirements section
                                                    //       (vtable 0x1c / 0x20 / 0x24)
    uint8_t error_reported;                         // 0x020 latched by hwreq_parser_report_error; vtable 0x38
    uint8_t unknown_21[3];                          // 0x021 never referenced
    msvc_std_string error_message;                  // 0x024 "%s on line %d - ..." or "Cannot find ..."; vtable 0x3c
    msvc_std_string graphics_device_name;           // 0x040 from a matching vendor device line; vtable 0x28
    msvc_std_string graphics_vendor_name;           // 0x05c from a matching vendor line; vtable 0x2c
    msvc_std_string sound_device_name;              // 0x078 audiovendor device line; vtable 0x30
    msvc_std_string sound_vendor_name;              // 0x094 audiovendor line; vtable 0x34
    uint32_t cpu_speed;                             // 0x0b0 MHz, parse argument 7 ("cpuspeed")
    uint32_t memory;                                // 0x0b4 MB, parse argument 5 ("ram")
    uint32_t video_memory;                          // 0x0b8 bytes, parse argument 6 ("videoram")
    d3d_adapter_identifier9 adapter;                // 0x0bc parse argument 3, 0x113 dwords
    d3d_caps9 caps;                                 // 0x508 parse argument 4, 0x4c dwords; every caps keyword
    shell_sound_device sound_device;                // 0x638 parse argument 2, 0x1a dwords
    msvc_std_map property_sets;                     // 0x6a0 std::map<string, hwreq_property_set *> by propertyset name;
                                                    //       owns the sets (the destructor frees them); vtable 0x0c
    msvc_std_map graphic_detail_sets;               // 0x6ac std::map<string, hwreq_property_set *> keyed by the
                                                    //       OverallGraphicDetail value; MaxOverallGraphicDetail looks here
} hwreq_parser;                                     // size 0x6b8

// The parser vtable at 0x006721e8 (__thiscall, this in ECX). Slot targets are listed; only
// 0x5786a0 and 0x5788f0 are Ghidra functions.
typedef struct hwreq_parser_vtable {
    uint32_t parse;                                 // 0x00 0x579bd0 (name, sound device, adapter, caps, memory,
                                                    //      video memory, cpu speed) -> bool, ret 0x1c
    uint32_t scalar_deleting_destructor;            // 0x04 0x5786a0
    uint32_t get_flags;                             // 0x08 0x578860 -> +0x18
    uint32_t find_property_set;                     // 0x0c 0x5788f0 (name) -> hwreq_property_set * or NULL
    uint32_t get_flag_count;                        // 0x10 0x5786c0
    uint32_t get_flag_name;                         // 0x14 0x5786f0 (index) -> char *
    uint32_t get_flag_value;                        // 0x18 0x578740 (index) -> char *
    uint32_t get_requirement_count;                 // 0x1c 0x578790
    uint32_t get_requirement_name;                  // 0x20 0x5787c0 (index)
    uint32_t get_requirement_value;                 // 0x24 0x578810 (index)
    uint32_t get_graphics_device_name;              // 0x28 0x578870 -> +0x40
    uint32_t get_graphics_vendor_name;              // 0x2c 0x578880 -> +0x5c
    uint32_t get_sound_device_name;                 // 0x30 0x578890 -> +0x78
    uint32_t get_sound_vendor_name;                 // 0x34 0x5788b0 -> +0x94
    uint32_t has_error;                             // 0x38 0x5788d0 -> +0x20
    uint32_t get_error_message;                     // 0x3c 0x5788e0 -> +0x24
} hwreq_parser_vtable;                              // size 0x40

// ---------------------------------------------------------------------------
// config.txt property table  (0x0069fe40, 0x1c entries, walked by shell_parse_config_txt)
// ---------------------------------------------------------------------------
// The setter receives the value string and returns nonzero on success; most set a flag global
// to 1 (their targets are listed with the globals at the end).
typedef uint8_t (*shell_config_property_setter)(const char *value);

typedef struct shell_config_property {
    uint32_t name;                                  // 0x00 const char *, compared with _stricmp
    uint32_t setter;                                // 0x04 shell_config_property_setter
} shell_config_property;                            // size 0x08

// ---------------------------------------------------------------------------
// crash reporter  (exception_filter_crash_reporter 0x542fa0)
// ---------------------------------------------------------------------------
// DLGTEMPLATE plus the menu and class ordinals; the UTF-16 title and, because DS_SETFONT is
// set, the point size and face name follow, then the item at the next dword boundary.
typedef struct crash_dialog_template {
    uint32_t style;                                 // 0x00 0x80c800c0 WS_POPUP | WS_CAPTION | WS_SYSMENU | DS_MODALFRAME | DS_SETFONT
    uint32_t extended_style;                        // 0x04 0
    uint16_t item_count;                            // 0x08 1
    int16_t x;                                      // 0x0a 0
    int16_t y;                                      // 0x0c 0
    int16_t cx;                                     // 0x0e 0xa3
    int16_t cy;                                     // 0x10 0x37
    uint16_t menu;                                  // 0x12 0, no menu
    uint16_t window_class;                          // 0x14 0, default dialog class
} crash_dialog_template;                            // size 0x16, then title "Exception!" (0x006f0770)

// DLGITEMTEMPLATE plus the predefined class ordinal; the UTF-16 text follows.
typedef struct crash_dialog_item_template {
    uint32_t style;                                 // 0x00 0x50020000 WS_CHILD | WS_VISIBLE | WS_GROUP
    uint32_t extended_style;                        // 0x04 0 (the template block is zero initialised)
    int16_t x;                                      // 0x08 0x1c
    int16_t y;                                      // 0x0a 0x17
    int16_t cx;                                     // 0x0c 0x6c
    int16_t cy;                                     // 0x0e 8
    uint16_t id;                                    // 0x10 0xffff
    uint16_t class_ordinal_marker;                  // 0x12 0xffff
    uint16_t class_ordinal;                         // 0x14 0x0082, STATIC
} crash_dialog_item_template;                       // size 0x16, then "Gathering Exception Data..." (0x006f0670)

// The Dr. Watson (dw15.exe -x -s <mapping handle>) shared memory block, in an inheritable
// anonymous file mapping. Only the fields the crash reporter writes or reads are named.
typedef struct dw_shared_memory {
    uint32_t size;                                  // 0x0000 0x1c50
    uint32_t process_id;                            // 0x0004 GetCurrentProcessId
    uint32_t thread_id;                             // 0x0008 GetCurrentThreadId
    uint32_t exception_address;                     // 0x000c ExceptionRecord->ExceptionAddress (+0xc)
    uint32_t exception_pointers;                    // 0x0010 EXCEPTION_POINTERS *, the filter argument
    uint32_t event_done;                            // 0x0014 HANDLE, first CreateEventA; polled after the alive event
    uint32_t event_notify_done;                     // 0x0018 HANDLE, left 0
    uint32_t event_alive;                           // 0x001c HANDLE, second CreateEventA; waited 600000 ms
    uint32_t mutex;                                 // 0x0020 HANDLE, CreateMutexA; waited 20000 ms
    uint32_t process;                               // 0x0024 HANDLE, DuplicateHandle of the current process
    uint32_t behavior_flags;                        // 0x0028 0x100
    uint32_t result;                                // 0x002c bit 0 set: ExitProcess(0); clear: close up and return
                                                    //        EXCEPTION_CONTINUE_SEARCH (0x543965..0x5439aa)
    uint32_t unknown_30;                            // 0x0030
    uint32_t offer_flags;                           // 0x0034 1
    uint32_t unknown_38;                            // 0x0038
    uint32_t let_run_flags;                         // 0x003c 0x11
    uint32_t unknown_40;                            // 0x0040
    uint32_t unknown_44;                            // 0x0044
    uint16_t application_name[0x38];                // 0x0048 L"Halo"
    uint16_t module_file_name[0x104];               // 0x00b8 mbstowcs of GetModuleFileNameA (0x104 byte source)
    uint8_t unknown_2c0[0xb78];                     // 0x02c0 never written
    char server[0x310];                             // 0x0e38 "watson.microsoft.com"; UNSURE length, the next
                                                    //        written field is at 0x1148
    char registry_subpath[0x2d0];                   // 0x1148 "Microsoft" PCHealth ErrorReporting DW path (37 bytes
                                                    //        written); UNSURE length, next written field at 0x1418
    uint16_t additional_files[0x400];               // 0x1418 "|"-separated dxdiag.txt, debug.txt, network.log paths,
                                                    //        lower-cased, MultiByteToWideChar limit 0x400
    uint8_t unknown_1c18[0x38];                     // 0x1c18 never written
} dw_shared_memory;                                 // size 0x1c50

// ---------------------------------------------------------------------------
// product id  (shell_build_product_id_string 0x57f3f0, extract_product_id_digits 0x57f360)
// ---------------------------------------------------------------------------
// The DigitalProductID registry value under HKLM Software Microsoft "Microsoft Games" Halo.
// The result string is "%05d,%09d,0,% 19.19I64d" of unknown_20, the nine product id digits and
// the first 8 bytes of SHA-1(SHA-1(the 15 bytes at +0x38)).
typedef struct digital_product_id {
    uint32_t size;                                  // 0x00 must be 0xa4
    uint16_t major_version;                         // 0x04 must be 3
    uint16_t minor_version;                         // 0x06 must be 0
    char product_id[0x18];                          // 0x08 "xxxxx-OEM-xxxxxxx-xxxxx" or retail form; OEM tested at
                                                    //      [6..8]; digits at 12..15,18..22 (OEM, 0x00672a68) or
                                                    //      6..8,10..15 (retail, 0x00672a90)
    uint32_t unknown_20;                            // 0x20 printed as the first %05d field (EDI at 0x57f48b)
    uint8_t unknown_24[0x14];                       // 0x24
    uint8_t hashed_key[0xf];                        // 0x38 SHA-1 input (lea ecx,[esp+0x64] at 0x57f4c7)
    uint8_t unknown_47[0x5d];                       // 0x47
} digital_product_id;                               // size 0xa4

// ---------------------------------------------------------------------------
// Keystone UI middleware  (keystone_library_load 0x542ad0, keystone_library_unload 0x542cf0)
// ---------------------------------------------------------------------------
// cdecl exports resolved by name. The slots the chat code calls already have typedefs in
// types/interface.h (chat_gui_*), noted per global below; the rest are typed here from their
// call sites. root is the KeystoneCreate result kept in 0x00721ea4.
typedef void *(*keystone_create_fn)(void *hwnd, void *d3d_device, const uint16_t *root_directory,
                                    int32_t unknown_3, int32_t unknown_4, int32_t unknown_5,
                                    int32_t unknown_6);                  // 0x00721ea0, called by 0x5196b0
typedef void (*keystone_release_fn)(void *root);                         // 0x00721eac Call_KsRelease
typedef int32_t (*keystone_translate_accelerator_fn)(void *root, void *hwnd, int32_t unknown,
                                                     void *message);    // 0x00721eb0, MSG *
typedef void (*keystone_create_window_fn)(void *root, const uint16_t *ksml_path, void *window_key,
                                          uint32_t flags, void *rect, int32_t unknown_5,
                                          int32_t unknown_6, int32_t unknown_7, int32_t unknown_8,
                                          int32_t unknown_9, int32_t unknown_10); // 0x00721eb4, rect is a win32_rect *
typedef int32_t (*keystone_update_fn)(void *root);                       // 0x00721ebc, negative on failure
typedef void *(*keystone_dispatch_message_fn)(void *root, uint32_t message, uint32_t wparam,
                                              int32_t lparam, int32_t *handled); // 0x00721ec0, returns a
                                                                         // window the caller releases
typedef void (*keystone_unknown_fn)(void);                               // 0x00721ec4 Call_KsSetFocusWindow and
                                                                         // 0x00721ed8 Call_KW_AddDirtyControl, never called

// ---------------------------------------------------------------------------
// hwreq_parser_vtable slot signatures (__thiscall, this in ECX)
// ---------------------------------------------------------------------------
// The vtable fields above are plain addresses; callers cast them to these types. Every slot is
// __thiscall (this in ECX); the keyword is left out because the Ghidra C parser has no __thiscall
// token, so the parser pointer is written as the first parameter.
typedef uint8_t (*hwreq_parse_fn)(hwreq_parser *parser, const char *name,
                                             shell_sound_device *sound_device,
                                             d3d_adapter_identifier9 *adapter, d3d_caps9 *caps,
                                             uint32_t memory, uint32_t video_memory,
                                             uint32_t cpu_speed);                   // slot 0x00, ret 0x1c
typedef hwreq_property_set *(*hwreq_find_property_set_fn)(hwreq_parser *parser,
                                                                      const char *name); // slot 0x0c
typedef uint32_t (*hwreq_get_count_fn)(hwreq_parser *parser);                // slots 0x10 / 0x1c
typedef char *(*hwreq_get_indexed_string_fn)(hwreq_parser *parser, uint32_t index); // 0x14 0x18 0x20 0x24
typedef char *(*hwreq_get_string_fn)(hwreq_parser *parser);                  // slots 0x28 .. 0x34, 0x3c
typedef uint8_t (*hwreq_has_error_fn)(hwreq_parser *parser);                 // slot 0x38

// ---------------------------------------------------------------------------
// Win32 records this module fills or reads (public SDK layouts, 32 bit)
// ---------------------------------------------------------------------------
// OSVERSIONINFOA: os_platform_identify 0x5427e0, game_single_instance_check 0x542d70.
typedef struct os_version_info_a {
    uint32_t size;                                  // 0x00 0x94
    uint32_t major_version;                         // 0x04
    uint32_t minor_version;                         // 0x08
    uint32_t build_number;                          // 0x0c
    uint32_t platform_id;                           // 0x10 1 Win9x, 2 NT
    char service_pack[0x80];                        // 0x14
} os_version_info_a;                                // size 0x94

// WINDOWPLACEMENT: game_single_instance_check restores the window of the other instance.
typedef struct window_placement {
    uint32_t length;                                // 0x00 0x2c
    uint32_t flags;                                 // 0x04
    uint32_t show_command;                          // 0x08
    int32_t min_position[2];                        // 0x0c POINT
    int32_t max_position[2];                        // 0x14 POINT
    int32_t normal_position[4];                     // 0x1c RECT
} window_placement;                                 // size 0x2c

// SID_IDENTIFIER_AUTHORITY and GENERIC_MAPPING: security_check_write_access 0x542840.
typedef struct sid_identifier_authority {
    uint8_t value[6];                               // 0x00
} sid_identifier_authority;                         // size 0x06

typedef struct generic_mapping {
    uint32_t generic_read;                          // 0x00
    uint32_t generic_write;                         // 0x04
    uint32_t generic_execute;                       // 0x08
    uint32_t generic_all;                           // 0x0c
} generic_mapping;                                  // size 0x10

// MEMORYSTATUS: shell_detect_hardware_specs (GlobalMemoryStatus).
typedef struct win32_memory_status {
    uint32_t length;                                // 0x00
    uint32_t memory_load;                           // 0x04
    uint32_t total_physical;                        // 0x08 clamped to 1 GB, rounded up to 16 MB
    uint32_t available_physical;                    // 0x0c
    uint32_t total_page_file;                       // 0x10
    uint32_t available_page_file;                   // 0x14
    uint32_t total_virtual;                         // 0x18
    uint32_t available_virtual;                     // 0x1c
} win32_memory_status;                              // size 0x20

// SECURITY_ATTRIBUTES, STARTUPINFOA, PROCESS_INFORMATION, MSG: the crash reporter.
typedef struct win32_security_attributes {
    uint32_t length;                                // 0x00 0xc
    uint32_t security_descriptor;                   // 0x04 void *
    int32_t inherit_handle;                         // 0x08 1: dw15.exe inherits the mapping and events
} win32_security_attributes;                        // size 0x0c

typedef struct win32_startup_info_a {
    uint32_t size;                                  // 0x00 0x44
    uint32_t unknown_04[16];                        // 0x04 left zero
} win32_startup_info_a;                             // size 0x44

typedef struct win32_process_information {
    uint32_t process;                               // 0x00 HANDLE
    uint32_t thread;                                // 0x04 HANDLE
    uint32_t process_id;                            // 0x08
    uint32_t thread_id;                             // 0x0c
} win32_process_information;                        // size 0x10

typedef struct win32_msg {
    uint32_t window;                                // 0x00 HWND
    uint32_t message;                               // 0x04 WM_COMMAND 0x111 ends the dxdiag wait
    uint32_t wparam;                                // 0x08
    int32_t lparam;                                 // 0x0c
    uint32_t time;                                  // 0x10
    int32_t point[2];                               // 0x14
} win32_msg;                                        // size 0x1c

// EXCEPTION_RECORD head and EXCEPTION_POINTERS: the unhandled exception filter argument.
typedef struct win32_exception_record {
    uint32_t code;                                  // 0x00 0xc00000fd stack overflow gets a frame move
    uint32_t flags;                                 // 0x04
    uint32_t next;                                  // 0x08 win32_exception_record *
    uint32_t address;                               // 0x0c copied to dw_shared_memory.exception_address
} win32_exception_record;                           // size 0x10 (parameters follow)

typedef struct win32_exception_pointers {
    uint32_t exception_record;                      // 0x00 win32_exception_record *
    uint32_t context;                               // 0x04 CONTEXT *
} win32_exception_pointers;                         // size 0x08

// faultrep.dll ReportFault(EXCEPTION_POINTERS *, options), resolved by the crash reporter.
typedef int32_t (__stdcall *report_fault_fn)(win32_exception_pointers *exception_pointers, uint32_t options);

// ---------------------------------------------------------------------------
// DirectDraw / DxDiag COM interfaces used by shell_detect_hardware_specs 0x57d880
// ---------------------------------------------------------------------------
// DDSCAPS2 for IDirectDraw7::GetAvailableVidMem; the four masks the detector queries are
// 0x4200, 0x10007000, 0x10005000 and 0x10004040.
typedef struct ddscaps2 {
    uint32_t caps;                                  // 0x00
    uint32_t caps2;                                 // 0x04
    uint32_t caps3;                                 // 0x08
    uint32_t caps4;                                 // 0x0c
} ddscaps2;                                         // size 0x10

typedef struct direct_draw7_vtable {
    void *unknown_00[2];                            // 0x00 QueryInterface, AddRef
    uint32_t (__stdcall *release)(void *self);      // 0x08
    void *unknown_0c[17];                           // 0x0c Compact .. RestoreDisplayMode
    int32_t (__stdcall *set_cooperative_level)(void *self, void *window, uint32_t flags); // 0x50 slot 20
    void *unknown_54[2];                            // 0x54 SetDisplayMode, WaitForVerticalBlank
    int32_t (__stdcall *get_available_vid_mem)(void *self, ddscaps2 *caps, uint32_t *total,
                                               uint32_t *free);                    // 0x5c slot 23
} direct_draw7_vtable;

typedef struct direct_draw7 {
    direct_draw7_vtable *vtable;                    // 0x00
} direct_draw7;

// ddraw.dll exports resolved with GetProcAddress by shell_detect_hardware_specs.
typedef int32_t (__stdcall *direct_draw_create_ex_fn)(void *guid, direct_draw7 **result, const void *iid,
                                                      void *outer);
typedef int32_t (__stdcall *direct_draw_enumerate_ex_fn)(void *callback, void *context, uint32_t flags);

// DXDIAG_INIT_PARAMS: size 0x10, header version 0x6f (DXDIAG_DX9_SDK_VERSION 111), no WHQL checks.
typedef struct dxdiag_init_params {
    uint32_t size;                                  // 0x00 0x10
    uint32_t header_version;                        // 0x04 0x6f
    int32_t allow_whql_checks;                      // 0x08 0
    uint32_t reserved;                              // 0x0c 0
} dxdiag_init_params;                               // size 0x10

// VARIANT as IDxDiagContainer::GetProp returns it; only VT_BSTR (8) values are read.
typedef struct win32_variant {
    uint16_t type;                                  // 0x00 VARTYPE
    uint16_t reserved[3];                           // 0x02
    uint32_t value;                                 // 0x08 BSTR (uint16_t *) when type is 8
    uint32_t value_high;                            // 0x0c
} win32_variant;                                    // size 0x10

typedef struct dxdiag_container_vtable {
    void *unknown_00[2];                            // 0x00 QueryInterface, AddRef
    uint32_t (__stdcall *release)(void *self);      // 0x08
    int32_t (__stdcall *get_number_of_child_containers)(void *self, uint32_t *count); // 0x0c
    void *enum_child_container_names;               // 0x10
    int32_t (__stdcall *get_child_container)(void *self, const uint16_t *name, void **child); // 0x14
    void *get_number_of_props;                      // 0x18
    void *enum_prop_names;                          // 0x1c
    int32_t (__stdcall *get_prop)(void *self, const uint16_t *name, win32_variant *value); // 0x20
} dxdiag_container_vtable;

typedef struct dxdiag_container {
    dxdiag_container_vtable *vtable;                // 0x00
} dxdiag_container;

typedef struct dxdiag_provider_vtable {
    void *unknown_00[2];                            // 0x00 QueryInterface, AddRef
    uint32_t (__stdcall *release)(void *self);      // 0x08
    int32_t (__stdcall *initialize)(void *self, dxdiag_init_params *params); // 0x0c
    int32_t (__stdcall *get_root_container)(void *self, dxdiag_container **root); // 0x10
} dxdiag_provider_vtable;

typedef struct dxdiag_provider {
    dxdiag_provider_vtable *vtable;                 // 0x00
} dxdiag_provider;

// IDirect3D9 slots shell_parse_config_txt 0x57d410 calls that d3d9_interface_vtable
// (types/interface.h) does not name: GetAdapterIdentifier (+0x14) and GetDeviceCaps (+0x38).
typedef int32_t (__stdcall *d3d9_get_adapter_identifier_fn)(void *self, uint32_t adapter, uint32_t flags,
                                                             d3d_adapter_identifier9 *out_identifier);
typedef int32_t (__stdcall *d3d9_get_device_caps_fn)(void *self, uint32_t adapter, uint32_t device_type,
                                                      d3d_caps9 *out_caps);

#pragma pack(pop)

// ---------------------------------------------------------------------------
// globals owned by this module
// ---------------------------------------------------------------------------
// .data (initialised)
// global 0x0069eab0: int32_t security_write_access_state       shell_access_check_state, cached by 0x542840
// global 0x0069eab4: int32_t shell_instance_mode_value          shell_instance_mode, -1 initially and after release
//                    (the plain name is taken by the enum typedef)
// global 0x0069eab8: char *shell_instance_mutex_names[9]        "Global" GUID names; [0] single instance, [1..8]
//                    multi instance; skipped 7 characters ("Global" plus separator) before Windows 2000;
//                    0x0069eadc is a zero dword after the table
// global 0x0069fe3c: int32_t config_maximum_resolution          MaximumResolution, 0x1000 default, 800 in safe mode;
//                    read by video_display_modes_enumerate 0x4baba0
// global 0x0069fe40: shell_config_property config_properties[0x1c]
// global 0x0069ff20: uint32_t shell_language_id                 LangID from HKLM, 0x409 default; a bare primary
//                    language gets SUBLANG_DEFAULT (| 0x400)
// global 0x006e357c: char cpu_brand_string[0x30]                CPUID 0x80000002..4 EAX EBX ECX EDX
// global 0x006e35ac: char cpu_vendor_string[0x10]               CPUID 0 EBX EDX ECX, 12 bytes; 0x006e35b8 stays zero
// global 0x006e35bc: void *shell_arrow_cursor                   LoadCursorA(NULL, IDC_ARROW); restored by the window proc
// global 0x006e35c0: char *shell_command_line                   the raw lpCmdLine
// global 0x006ef9a8: uint32_t sound_device_count                DxDiag child count (DWORD), clamped to 9
// global 0x006ef9b0: shell_sound_device sound_devices[10]
// global 0x006efdc0: shell_display_adapter display_adapters[10]
// global 0x006effc8: int32_t required_video_memory              MB, config.txt "VideoMemory"
// global 0x006effcc: int32_t required_memory                    MB, "Memory"; winmain fails below it minus 16
// global 0x006effd0: int32_t selected_sound_device              index of the default DirectSound device
// global 0x006effd4: int32_t required_disk_space                MB of the temp volume, "DiskSpace"
// global 0x006effd8: int32_t required_directx_build             fourth field of "DirectX"
// global 0x006effdc: hwreq_parser *hardware_requirements        created by shell_parse_config_txt, never freed
// global 0x006effe0: int32_t required_cpu_speed                 MHz, "CpuSpeed"; winmain measures up to three times
// global 0x006effe8: char fatal_error_text[0x400]               string resource or the config.txt message
// global 0x006f03e8: uint32_t shell_startup_tick_count          GetTickCount at the end of string loading
// global 0x006f03ec: int32_t fatal_error_is_fatal               third argument of the fatal error dialog
// global 0x006f03f0: char fatal_error_title[0x80]               "Halo - Error" fallback
// global 0x006f0470: char fatal_error_help_file[0x200]          "readme.rtf" fallback
// global 0x00722bc0: int32_t fatal_error_remember_choice        (R15) the fatal error dialog's
//                      checkbox: dialog proc 0x57e5a0 stores (IsDlgButtonChecked(0x3e9) != 0) at
//                      0x57e773 / 0x57e7a3; shell_display_fatal_error_dialog reads it (0x57ee7c).
//                      Formerly attributed to dialogs.h by mistake
// global 0x00722c58: char fatal_error_system_specs[0x100]      (R15) the "%dMHz, %dMB, ..." system
//                      specs text of control 0x3f1: sprintf'd by 0x57e5a0 (0x57e6e1, 0x57e70a),
//                      cleared (0x57e719), handed to SetDlgItemTextA (0x57e720). 0x100 bytes, up
//                      to the next global 0x00722d58
// global 0x006f0670: char exception_gathering_text[0x100]       string 0x78, "Gathering Exception Data..."
// global 0x006f0770: char exception_title[0x100]                string 0x77, "Exception!"
// global 0x006f0870: char eula_file_name[0x20]                  string 0x84, "eula.rtf"; passed to EBUEula
// global 0x006f0890: char strings_dll_invalid_text[0x400]       string 0x88, shown for -? / -help
//
// .bss
// global 0x00721e5c: uint32_t cpu_features                      CPUID 1 EDX
// global 0x00721e60: uint32_t cpu_extended_features             CPUID 0x80000001 EDX
// global 0x00721e64: uint32_t cpu_signature                     CPUID 1 EAX (family / model / stepping)
// global 0x00721e68: uint32_t cpu_l1_tlb_large                  CPUID 0x80000005 EAX
// global 0x00721e6c: uint32_t cpu_l1_tlb_4k                     CPUID 0x80000005 EBX
// global 0x00721e70: uint32_t cpu_l1_data_cache                 CPUID 0x80000005 ECX
// global 0x00721e74: uint32_t cpu_l1_code_cache                 CPUID 0x80000005 EDX
// global 0x00721e78: uint32_t cpu_l2_tlb_large                  CPUID 0x80000006 EAX
// global 0x00721e7c: uint32_t cpu_l2_tlb_4k                     CPUID 0x80000006 EBX
// global 0x00721e80: uint32_t cpu_l2_cache                      CPUID 0x80000006 ECX
// global 0x00721e84: uint32_t cpu_l2_unknown                    CPUID 0x80000006 EDX, stored but never queried
// global 0x00721e88: int32_t cpu_identification_state           0 not queried, -1 no CPUID, else queried
// global 0x00721e8c: uint8_t shell_application_inactive         set when the window loses focus or is minimised
//                    (input unacquired, chat closed, sound paused); read by weapon_stop_reload 0x4c7f10
// global 0x00721e8d: uint8_t shell_window_proc_bypass           set by the crash reporter and the fatal error
//                    dialog; the window proc then forwards everything to DefWindowProcA
// global 0x00721e90: char **shell_argv                          command_line_parse_to_argv, GlobalAlloc; argv[0] = ""
// global 0x00721e94: int32_t shell_argc
// global 0x00721e98: void *shell_direct3d                       IDirect3D9 *, Direct3DCreate9(0x1f)
// global 0x00721e9c: void *keystone_module                      HMODULE keystone.dll, NULL in safe mode
// global 0x00721ea0: keystone_create_fn keystone_create
// global 0x00721ea4: void *keystone_root                        KeystoneCreate result
// global 0x00721ea8: uint16_t *keystone_current_directory       GlobalAlloc, mbstowcs of the current directory
// global 0x00721eac: keystone_release_fn keystone_release
// global 0x00721eb0: keystone_translate_accelerator_fn keystone_translate_accelerator
// global 0x00721eb4: keystone_create_window_fn keystone_create_window
// global 0x00721eb8: chat_gui_find_object_fn keystone_get_window           Call_KsGetWindow (interface.h type)
// global 0x00721ebc: keystone_update_fn keystone_update
// global 0x00721ec0: keystone_dispatch_message_fn keystone_dispatch_message
// global 0x00721ec4: keystone_unknown_fn keystone_set_focus_window
// global 0x00721ec8: chat_gui_release_fn keystone_window_release         Call_KW_Release
// global 0x00721ecc: chat_gui_find_child_fn keystone_window_get_control  Call_KW_GetControlByID
// global 0x00721ed0: chat_gui_finalize_fn keystone_window_relayout       Call_KW_ReLayout
// global 0x00721ed4: chat_gui_set_focus_fn keystone_window_set_focus_control  Call_KW_SetFocusControl
// global 0x00721ed8: keystone_unknown_fn keystone_window_add_dirty_control
// global 0x00721edc: chat_gui_set_state_fn keystone_window_show          Call_KW_ShowWindow
// global 0x00721ee0: chat_gui_get_property_string_fn keystone_control_get_attribute  Call_KC_GetAttribute
// global 0x00721ee4: chat_gui_set_property_string_fn keystone_control_set_attribute  Call_KC_SetAttribute
// global 0x00721ee8: chat_gui_set_property_int_fn keystone_control_send_message      Call_KC_SendMessage
// global 0x00721eec: uint8_t keystone_chat_active               interface only (chat open / close)
// global 0x00721ef0: int32_t os_platform_value                  os_platform; 0 until os_platform_identify runs
//                    (the plain name is taken by the enum typedef)
// global 0x00721f00: void *shell_instance_mutex                 HANDLE
// global 0x00721f04: int32_t shell_instance_index               free multi-instance slot, -1 otherwise
// global 0x00721f08: void *shell_stack_guard_page               a byte of the 0x2000 byte 0xee filled winmain frame,
//                    made PAGE_NOACCESS; restored by winmain and by the crash reporter
// global 0x00721f0c: uint32_t shell_stack_guard_old_protect
// global 0x00721f10: report_fault_fn report_fault               faultrep.dll ReportFault(pointers, 0), fallback when Watson fails
// global 0x00722b28: int32_t config_linear_texture_addressing        LinearTextureAddressing
// global 0x00722b2c: int32_t config_linear_texture_addressing_zoom   LinearTextureAddressingZoom
// global 0x00722b30: int32_t config_linear_texture_addressing_sun    LinearTextureAddressingSun
// global 0x00722b34: int32_t config_use_fixed_function               UseFixedFunction (forced in safe mode)
// global 0x00722b38: int32_t config_disable_driver_management        DisableDriverManagement (forced in safe mode)
// global 0x00722b3c: int32_t config_unsupported_card                 UnsupportedCard
// global 0x00722b40: int32_t config_prototype_card                   PrototypeCard
// global 0x00722b44: int32_t config_old_driver                       OldDriver
// global 0x00722b48: int32_t config_old_sound_driver                 OldSoundDriver
// global 0x00722b4c: int32_t config_invalid_driver                   InvalidDriver
// global 0x00722b50: int32_t config_invalid_sound_driver             InvalidSoundDriver
// global 0x00722b54: int32_t config_disable_buffering                DisableBuffering (forced in safe mode)
// global 0x00722b58: int32_t config_enable_stop_start                EnableStopStart
// global 0x00722b5c: int32_t config_head_relative_speech             HeadRelativeSpeech
// global 0x00722b60: int32_t config_safe_mode                        SafeMode
// global 0x00722b64: int32_t config_force_shader                     ForceShader "%d"; 9999 in safe mode
// global 0x00722b68: int32_t config_use_anisotropic_filter           UseAnisotropicFilter
// global 0x00722b6c: int32_t config_disable_specular                 DisableSpecular (forced in safe mode)
// global 0x00722b70: int32_t config_disable_render_targets           DisableRenderTargets (forced in safe mode)
// global 0x00722b74: int32_t config_disable_alpha_render_targets     DisableAlphaRenderTargets
// global 0x00722b78: int32_t config_use_alternate_convolve_mask      UseAlternateConvolveMask
// global 0x00722b7c: int32_t config_min_max_blend_op_is_broken       MinMaxBlendOpIsBroken (forced in safe mode)
// global 0x00722b80: float config_decal_z_bias                       DecalZBiasValue, default bits 0xb866afcd
// global 0x00722b84: float config_transparent_decal_z_bias           TransparentDecalZBiasValue, default bits 0xb6a7c5ac
// global 0x00722b88: float config_decal_slope_z_bias                 DecalSlopeZBiasValue
// global 0x00722b8c: float config_transparent_decal_slope_z_bias     TransparentDecalSlopeZBiasValue
// global 0x00722b90: char *graphics_vendor_name                 hwreq vtable 0x2c
// global 0x00722b94: char *graphics_device_name                 hwreq vtable 0x28
// global 0x00722b98: uint32_t graphics_device_id                adapter DeviceId; fatal error registry key "(0x%04x)"
// global 0x00722b9c: uint32_t graphics_vendor_id                adapter VendorId
// global 0x00722ba0: large_integer graphics_driver_version      adapter DriverVersion
// global 0x00722ba8: uint32_t physical_memory                   MB, rounded up to 16 MB, clamped to 1 GB
// global 0x00722bac: uint32_t cpu_speed                         MHz from rdtsc over 1/4 s, snapped to 33 / 50 / 66 / 100
// global 0x00722bb0: uint32_t video_memory                      bytes of the adapter whose name matches, or the UMA
//                    config value (8 / 16 / 32 MB by physical memory)
// global 0x00722bb4: uint32_t display_adapter_count
// global 0x00722bb8: void *strings_module                       HMODULE strings.dll, the resource module
// global 0x00722bbc: char *fatal_error_argument                 appended as " (arg)" to the next fatal error text
// global 0x00722bcc: uint32_t crypt_provider                    HCRYPTPROV, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT
// global 0x00722bd0: int32_t crash_in_progress                  a second exception terminates the process
// global 0x00722bd8: char product_id_string[0x80]               "" on failure (0x0065512c is the empty string)
// global 0x00722d58: char hwreq_quoted_string[0x100]            hwreq_token_parse_quoted_string result
// global 0x00722e58: char config_error_text[0x100]              "Error in config.txt ..."
// global 0x00722f58: char config_unknown_property_text[0x100]   "Unknown property in config.txt ..."
// global 0x00723058: char hwreq_open_error_text[]               "Cannot find ..." (sprintf in 0x579bd0); UNSURE
//                    length, 0x184 bytes to the next referenced global 0x007231dc
//
// Written by shell_winmain / engine_initialize_subsystems only, but outside the shell .bss
// cluster (UNSURE which object defines them):
// global 0x007461a8: char *shell_product_id                     shell_build_product_id_string result
// global 0x007461c0: void *shell_instance                       HINSTANCE
// global 0x007461c4: void *shell_window                         HWND, created by rasterizer_create_game_window
// global 0x007461c8: void *shell_gamma_window                   HWND for GetDC in the gamma code; winmain stores 0
//                    (whole screen)
// global 0x007461cc: int32_t shell_show_command                 nCmdShow
// global 0x007461d0: void *shell_window_proc                    WNDPROC, 0x00541b30
// global 0x007461d4: char shell_window_class_name[0x40]         "Halo"
// global 0x00746214: char shell_window_title[0x40]              "Halo"
// global 0x00746254: uint8_t shell_window_minimized             WM_SIZE SIZE_MINIMIZED
// global 0x00746255: uint8_t shell_window_maximized             WM_SIZE SIZE_MAXIMIZED
// global 0x00746258: void *dinput8_module
// global 0x0074625c: void *dsound_module                        NULL with -nosound
// global 0x00746260: void *shfolder_module
// global 0x00746264: void *d3d9_module
// global 0x00746268: void *direct_input8_create                 FARPROC
// global 0x0074626c: void *sh_get_folder_path                   FARPROC
// global 0x00746270: void *direct_sound_create8                 FARPROC
// global 0x00746274: void *direct3d_create9                     FARPROC; engine_initialize_subsystems loads the four
//                    DLLs only when this is still NULL
//
// ---------------------------------------------------------------------------
// globals this module writes or reads but does not own
// ---------------------------------------------------------------------------
// 0x007196d8  int32_t game_time_force_single_tick (game)   -timedemo
// 0x007196e0  int32_t screenshots, 0x007196e4 nosound, 0x007196e8 novideo / connect, 0x007196ec
//             nonetwork, 0x007196f0 width640, 0x007196f4 safe_mode (also set by the fatal error
//             dialog), 0x007196f8 nowindowskey; 0x00712c2c nojoystick; 0x0071d1a4 checkfpu;
//             0x0071d1ac windowed (rasterizer). All are 32 bit BOOLs: winmain stores them with
//             movzx + a dword store and every reader loads a dword.
// 0x006869c4  int16_t sound_cache_size_megabytes (sound)   12 / 16 / 32 / 64 by physical memory
// 0x006869c8  int16_t (sound)                              0x30 (16 bit stores in winmain)
// 0x006869a4 / 0x006869b0 / 0x00698208 / 0x0069820c / 0x0071c2d0  -ip / -port / -cport (networking)
// 0x006ac8f8  large_integer performance frequency (math); 0x006ac900 profile directory (cache),
//             zeroed as 0x41 dwords plus one byte by engine_initialize_subsystems
// 0x0071d16c / 0x0071d174 / 0x0071d188 / 0x0071d184  rasterizer fullscreen flag, device, splash bitmap
// 0x00725202 / 0x00725208 / 0x0072520c  sound pause flag, sound object, last frame time
