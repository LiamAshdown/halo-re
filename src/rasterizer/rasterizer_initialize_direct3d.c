// rasterizer_initialize_direct3d  (Ghidra: rasterizer_initialize_direct3d, already named)
// address 0x5169c0, size 2707 bytes
// name confidence: 0.6   rewrite confidence: 0.6
// evidence: the module's top level Direct3D bring-up. Creates the game window, the IDirect3D9
//   object (Direct3DCreate9(0x1f) through the pointer at 0x00746274 unless one was handed in at
//   0x00721e98), walks the adapters (-adapter N picks one, 1 based), reads the caps into
//   rasterizer_caps (GetDeviceCaps +0x38 with the literal 0x007c10c0), applies the pixel shader
//   overrides from config.txt and the -useff/-use00/-use11/-use14/-use20/-use2a switches, runs
//   the driver sanity dialogs, creates the device trying up to four behavior flag sets
//   (CreateDevice +0x40), then initialises every subsystem the rasterizer owns.
//   Ghidra mis-tracked most of the stack frame (the display mode block, the behavior flag table,
//   the desktop RECT and the final viewport overlap in its output) and removed a reachable block
//   as unreachable (the NVIDIA control panel check at 0x516f2e..0x516f48); the control flow
//   below follows the raw code 0x5169c0..0x517469. Frame layout used: +0x18 rasterizer_display_mode
//   (width, height, refresh, vsync), +0x28 d3d_display_mode, +0x38 behavior flags[4], +0x48
//   desktop RECT, later reused for the D3DVIEWPORT9.
//   0x0071d16c is set when neither -window (0x0071d1a8) nor 0x0071d1ac is set, then gates the
//   WS_POPUP window style, ShowCursor(0) and the -adapter switch, and its clear state is the one
//   that demands a 32 bit desktop: it is the fullscreen flag, not "windowed" as the header had it.
// register convention: none, __cdecl with no parameters; returns a bool in AL.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "interface.h"
#include "rasterizer.h"

extern void *rasterizer_device;                          // 0x0071d174
extern void *rasterizer_direct3d;                        // 0x0071d178
extern d3d_caps9 rasterizer_caps;                        // 0x007c10c0
extern d3d_present_parameters rasterizer_present_parameters; // 0x007c04a0
extern d3d_display_mode rasterizer_desktop_display_mode; // 0x007c11f0
extern uint8_t rasterizer_fullscreen;                               // 0x0071d16c (the header called it windowed)
extern uint8_t rasterizer_pending_clear;                 // 0x0071d16e
extern uint8_t rasterizer_device_lost;                   // 0x007c10b0
extern uint8_t unknown_0071d170;                         // 0x0071d170 UNSURE: present parameter
                                                         //   fallback latch, see
                                                         //   rasterizer_build_present_parameters.c
extern uint8_t rasterizer_software_vertex_processing;    // 0x0069c680
extern uint8_t rasterizer_caps_flag_688;                 // 0x0069c688
extern uint8_t unknown_0069c689; // 0x0069c689
extern uint8_t rasterizer_caps_flag_68a;                 // 0x0069c68a
extern uint32_t rasterizer_device_type;                  // 0x0069c6a0 D3DDEVTYPE, 1 HAL, 2 REF
extern int16_t rasterizer_texture_stage_count;           // 0x0069c682 UNSURE name: 4, or 2 on
                                                         //   fewer than 4 simultaneous textures
extern int16_t rasterizer_maximum_skinning_nodes;        // 0x0069c67e
extern uint32_t rasterizer_adapter;                      // 0x0071d180 adapter ordinal in use
extern uint8_t rasterizer_use_fx_file;                   // 0x0071d18e -usefxfile
extern uint8_t rasterizer_frame_started;                 // 0x0069c630 UNSURE name
extern void *rasterizer_scratch_memory;                  // 0x0071d13c
extern void *rasterizer_hwnd;                            // 0x007461c4
extern void *rasterizer_external_direct3d;               // 0x00721e98 UNSURE: an IDirect3D9 supplied
                                                         //   before startup; NULL normally
extern void *(__stdcall *direct3d_create9_procedure)(uint32_t sdk_version); // 0x00746274
extern int32_t rasterizer_window_requested;              // 0x0071d1a8 -window; also D3DCREATE 4 and
                                                         //   skips the loading screen
extern int32_t unknown_0071d1ac;                         // 0x0071d1ac UNSURE: second windowed switch
extern int32_t rasterizer_fpu_preserve;                  // 0x0071d1a4 D3DCREATE_FPU_PRESERVE
extern int32_t rasterizer_disable_driver_management;     // 0x00722b38 D3DCREATE 0x100
extern uint32_t unknown_007196d8;                        // 0x007196d8 UNSURE: nonzero turns vsync off
extern int32_t unknown_007196f0;                         // 0x007196f0 UNSURE: safe mode switch
extern int32_t unknown_007196f4;                                    // 0x007196f4 UNSURE: safe mode switch
extern uint32_t unknown_00722ba8;                        // 0x00722ba8 UNSURE: machine spec, <= 0x80
                                                         //   forces 640x480
extern uint32_t unknown_00722bac;                        // 0x00722bac UNSURE: machine spec, <= 1000
                                                         //   forces 640x480
extern int32_t rasterizer_config_shader_version;         // 0x00722b64 config.txt pixel shader
                                                         //   override, e.g. 11 for ps_1_1
extern int32_t renderer_unknown_722b60;                             // 0x00722b60 nonzero: one vertex stream, fixed function path
extern int32_t renderer_unknown_722b34;                  // 0x00722b34 forces the fixed function path
extern int32_t rasterizer_config_warning_722b3c;         // 0x00722b3c config.txt driver warnings
extern int32_t rasterizer_config_warning_722b40;         // 0x00722b40
extern int32_t rasterizer_config_warning_722b44;         // 0x00722b44
extern int32_t rasterizer_config_warning_722b48;         // 0x00722b48
extern int32_t rasterizer_config_warning_722b4c;         // 0x00722b4c
extern int32_t rasterizer_config_warning_722b50;         // 0x00722b50
extern int32_t rasterizer_config_disable_render_targets; // 0x00722b70 sets flags 689 and 68a
extern int32_t rasterizer_config_disable_offscreen;      // 0x00722b74 sets flag 68a
extern uint32_t rasterizer_adapter_device_id;            // 0x00722b98
extern uint32_t rasterizer_adapter_vendor_id;            // 0x00722b9c 0x1002 ATI, 0x10de NVIDIA
extern uint32_t video_memory_bytes;                      // 0x00722bb0
extern uint32_t rasterizer_minimum_video_memory_mb;      // 0x006effc8 UNSURE name
extern int32_t command_line_argc;                                   // 0x00721e94
extern char **command_line_argv;                                    // 0x00721e90
extern uint8_t *game_state_base;                                    // 0x006e2dc8, game.h (saved_games)
extern int32_t game_state_cursor;                                   // 0x006e2dcc, game.h (saved_games)
extern uint32_t game_state_crc;                                     // 0x006e2dd4, game.h (saved_games)
extern uint8_t crc32_lookup_table_initialized;           // 0x00719cd8
extern crc32_table crc32_lookup_table;                   // 0x006b7b00
extern uint32_t *cinematic_globals;                                 // 0x0071cfc4 UNSURE owner; +0x74 letterbox height

extern uint8_t rasterizer_parse_vidmode_commandline(int32_t *width_out, int32_t *height_out, long *refresh_out); // 0x5168c0, ESI width_out
extern uint32_t rasterizer_create_game_window(int32_t height, int32_t width);    // 0x515930, EAX height, EBX width
extern void rasterizer_resize_game_window(int32_t height, int32_t width);        // 0x515b20, EAX height, ECX width
extern void rasterizer_build_present_parameters(d3d_present_parameters *dest, rasterizer_display_mode *source); // 0x515fc0, EAX source
extern void rasterizer_set_default_render_states(void);           // 0x5160d0
extern void rasterizer_render_loading_screen(int32_t mode);                      // 0x5157e0, EAX mode
extern void __cdecl rasterizer_select_hardware_codepaths(void);                  // 0x516810
extern int32_t rasterizer_dx9_effects_initialize(void);                          // 0x5300d0
extern uint32_t rasterizer_decal_index_buffer_initialize(void);                  // 0x51bb90
extern int32_t __cdecl transparent_geometry_pool_initialize(void);               // 0x5151c0
extern int32_t __cdecl text_font_system_initialize(void);                        // 0x514820
extern uint8_t rasterizer_detail_object_vertex_buffer_create(void);            // 0x51b370
extern uint8_t rasterizer_render_target_initialize(void);                                               // 0x52ca20 render target pool create
extern int32_t rasterizer_lens_flare_occlusion_queries_create(void);             // 0x536f70
extern void chimera__registry_check_4(void);                                     // 0x522520
extern void texture_cache_new(void);                                             // 0x4444d0
extern uint8_t __cdecl rasterizer_reset_device_if_needed(void);                  // 0x517500
extern void rasterizer_end_frame(void);                                          // 0x517b90
extern void rasterizer_editbox_log_dump(void);                                   // 0x5196b0
extern void __cdecl rasterizer_shutdown(void);                                   // 0x518450
extern uint8_t command_line_check_flag(const char *flag, const char **out_value); // 0x542760, EDI out_value
extern int32_t shell_parse_config_txt(uint32_t adapter, void *direct3d);         // 0x57d410, ECX adapter, EDX direct3d
extern void shell_display_fatal_error_dialog(uint32_t string_id, uint32_t title_id, int32_t fatal); // 0x57ea70
extern void crc32_build_table(crc32_table *table);                               // 0x4d0330, blam-cc: table in EDX
extern int32_t __stricmp(const char *a, const char *b);                          // 0x628d8b
extern int sscanf(const char *buffer, const char *format, ...);                  // 0x626572

// Win32
extern void *__stdcall LoadLibraryA(const char *name);
extern void *__stdcall GetProcAddress(void *module, const char *name);
extern int32_t __stdcall FreeLibrary(void *module);
extern void *__stdcall GetDesktopWindow(void);
extern void *__stdcall GetDC(void *hwnd);
extern int32_t __stdcall ReleaseDC(void *hwnd, void *hdc);
extern int32_t __stdcall GetDeviceCaps(void *hdc, int32_t index);
extern int32_t __stdcall SetWindowLongA(void *hwnd, int32_t index, int32_t value);
extern int32_t __stdcall GetWindowRect(void *hwnd, win32_rect *rect);
extern int32_t __stdcall ShowCursor(int32_t show);
extern void *__stdcall GlobalAlloc(uint32_t flags, uint32_t bytes);

typedef uint32_t (*d3d_get_adapter_count_fn)(void *self);
typedef int32_t (*d3d_get_adapter_display_mode_fn)(void *self, uint32_t adapter, d3d_display_mode *mode);
typedef int32_t (*d3d_check_device_format_fn)(void *self, uint32_t adapter, uint32_t device_type, uint32_t adapter_format,
                                              uint32_t usage, uint32_t resource_type, uint32_t check_format);
typedef int32_t (*d3d_get_device_caps_fn)(void *self, uint32_t adapter, uint32_t device_type, d3d_caps9 *caps);
typedef int32_t (*d3d_create_device_fn)(void *self, uint32_t adapter, uint32_t device_type, void *focus_window,
                                        uint32_t behavior_flags, d3d_present_parameters *parameters, void **device);
typedef int32_t (*d3d_set_viewport_fn)(void *self, const d3d_viewport *viewport);
typedef int32_t (__cdecl *nvcpl_get_data_int_fn)(int32_t data_type, int32_t *value);

// One pass over the command line: nonzero when `name` is present as its own argument.
static int command_line_has_switch(const char *name)
{
    int32_t i;

    for (i = 0; i < command_line_argc; i++) {
        const char *argument = command_line_argv[i];

        if (argument[0] == '-' && __stricmp(name, argument) == 0) {
            return 1;
        }
    }
    return 0;
}

static void **direct3d_vtable(void)
{
    return *(void ***)rasterizer_direct3d;
}

// finit; fldcw -- back to single precision with every exception masked, the state D3D leaves
// the FPU in without D3DCREATE_FPU_PRESERVE
static void rasterizer_fpu_reset_control_word(uint16_t control_word)
{
#if defined(__GNUC__)
    __asm__ volatile ("finit\n\tfldcw %0" : : "m" (control_word));
#else
    __asm { finit }
    __asm { fldcw control_word }
#endif
}

uint8_t rasterizer_initialize_direct3d(void)
{
    rasterizer_display_mode mode;
    d3d_display_mode desktop_mode;
    win32_rect desktop_rect;
    d3d_viewport viewport;
    uint32_t behavior_flags[4];
    uint8_t adapter_usable = 1;
    uint32_t requested_adapter = 0;             // -adapter N, 1 based; 0 lets the loop pick
    int32_t attempt;
    uint32_t adapter_count;
    uint32_t adapter = 0;
    void *hwnd;
    uint8_t succeeded;

    mode.vsync = (uint8_t)(unknown_007196d8 == 0);
    mode.width = 800;
    mode.height = 600;
    mode.refresh_rate = 60;
    rasterizer_device = 0;
    rasterizer_fullscreen = (uint8_t)(unknown_0071d1ac == 0);
    if (rasterizer_window_requested != 0) {
        rasterizer_fullscreen = 0;
    }
    if (unknown_007196f0 != 0 || unknown_007196f4 != 0 || unknown_00722bac <= 1000 || unknown_00722ba8 <= 0x80) {
        mode.width = 640;
        mode.height = 480;
    }
    rasterizer_parse_vidmode_commandline(&mode.width, &mode.height, (long *)&mode.refresh_rate);
    unknown_0069c689 = 0;
    rasterizer_caps_flag_68a = 0;
    rasterizer_caps_flag_688 = 0;

    if (!rasterizer_create_game_window(mode.height, mode.width)) {
        return 0;
    }
    hwnd = rasterizer_hwnd;
    if (rasterizer_external_direct3d != 0) {
        rasterizer_direct3d = rasterizer_external_direct3d;
    } else {
        rasterizer_direct3d = direct3d_create9_procedure(0x1f);    // D3D_SDK_VERSION
        if (rasterizer_direct3d == 0) {
            return 0;
        }
    }
    adapter_count = ((d3d_get_adapter_count_fn)direct3d_vtable()[0x10 / 4])(rasterizer_direct3d);
    if (adapter_count < 1) {
        return 0;
    }
    if (rasterizer_fullscreen != 0) {
        const char *argument;

        if (command_line_check_flag("-adapter", &argument)) {
            sscanf(argument, "%d", &requested_adapter);
            if (requested_adapter > adapter_count) {
                return 0;
            }
        }
    }

    // attempt -1 tries the requested adapter, then 0..count-1 in order
    for (attempt = -1; ; attempt++) {
        uint32_t shader_version;
        int32_t error;
        uint32_t i;
        void *desktop;
        void *hdc;

        if (attempt == -1) {
            if (requested_adapter == 0) {
                continue;
            }
            adapter = requested_adapter - 1;
        } else {
            if ((uint32_t)attempt >= adapter_count) {
                goto finish;
            }
            adapter = (uint32_t)attempt;
        }

        rasterizer_device_type = command_line_has_switch("-useref") ? 2 : 1;
        ((d3d_get_device_caps_fn)direct3d_vtable()[0x38 / 4])(rasterizer_direct3d, adapter, rasterizer_device_type,
                                                              &rasterizer_caps);
        error = shell_parse_config_txt(adapter, rasterizer_direct3d);
        if (error != 0) {
            shell_display_fatal_error_dialog(0xffffffff, (uint32_t)error, 0);
        }

        // pixel shader version override: config.txt first, then the command line
        shader_version = 0xffffffff;
        if (rasterizer_config_shader_version != 0) {
            if (rasterizer_config_shader_version == 9999 || rasterizer_config_shader_version == 0x270d) {
                shader_version = 0;
            } else if (rasterizer_config_shader_version == 0x270e) {
                shader_version = 0xffff0200;
            } else {
                // 11 -> ps_1_1 (0xffff0101), 14 -> ps_1_4 ...
                shader_version = ((((uint32_t)rasterizer_config_shader_version / 10) | 0xffffff00) << 8) |
                                 ((uint32_t)rasterizer_config_shader_version % 10);
            }
        }
        if (command_line_has_switch("-useff") ||
            unknown_007196f4 != 0 || renderer_unknown_722b60 != 0 || renderer_unknown_722b34 != 0) {
            rasterizer_config_shader_version = 0;
            shader_version = 0;
        }
        if (command_line_has_switch("-use00")) {
            rasterizer_config_shader_version = 0;
            shader_version = 0;
            rasterizer_caps.max_streams = 1;
        }
        if (command_line_has_switch("-use11")) {
            rasterizer_config_shader_version = 0;
            shader_version = 0xffff0101;
        }
        if (command_line_has_switch("-use14")) {
            rasterizer_config_shader_version = 0;
            shader_version = 0xffff0104;
        }
        if (command_line_has_switch("-use20")) {
            rasterizer_config_shader_version = 0;
            shader_version = 0xffff0200;
        }
        if (command_line_has_switch("-use2a")) {
            rasterizer_config_shader_version = 0;
            shader_version = 0xffff0200;
            goto lower_shader_version;
        }
        if (shader_version != 0xffffffff) {
        lower_shader_version:
            // only ever lowers the reported version
            if ((rasterizer_caps.pixel_shader_version & 0xffff) > (shader_version & 0xffff)) {
                rasterizer_caps.pixel_shader_version = shader_version;
            }
        }

        // driver warnings flagged by config.txt
        if (rasterizer_config_warning_722b40 != 0) {
            shell_display_fatal_error_dialog(0x93, 0x70, 0);
        }
        if (rasterizer_config_warning_722b3c != 0) {
            shell_display_fatal_error_dialog(0x67, 0x70, 0);
        }
        if (rasterizer_config_warning_722b4c != 0 && rasterizer_config_warning_722b3c == 0) {
            shell_display_fatal_error_dialog(0x69, 0x72, 0);
        }
        if (rasterizer_config_warning_722b44 != 0 && rasterizer_config_warning_722b4c == 0 &&
            rasterizer_config_warning_722b3c == 0) {
            shell_display_fatal_error_dialog(0x68, 0x71, 0);
        }
        if (rasterizer_config_warning_722b50 != 0) {
            shell_display_fatal_error_dialog(0x8f, 0x72, 0);
        }
        if (rasterizer_config_warning_722b48 != 0 && rasterizer_config_warning_722b50 == 0) {
            shell_display_fatal_error_dialog(0x8e, 0x71, 0);
        }
        if (rasterizer_config_disable_render_targets != 0) {
            unknown_0069c689 = 1;
            rasterizer_caps_flag_68a = 1;
        }
        if (rasterizer_config_disable_offscreen != 0) {
            rasterizer_caps_flag_68a = 1;
        }

        if (rasterizer_adapter_vendor_id == 0x1002) {
            if (rasterizer_adapter_device_id == 0x514c || rasterizer_adapter_device_id == 0x514e ||
                rasterizer_adapter_device_id == 0x514f || rasterizer_adapter_device_id == 0x4242) {
                rasterizer_caps_flag_688 = 1;
            }
        } else if (rasterizer_adapter_vendor_id == 0x10de) {
            void *library = LoadLibraryA("NVCPL.dll");

            if (library != 0) {
                nvcpl_get_data_int_fn get_data_int = (nvcpl_get_data_int_fn)GetProcAddress(library, "NvCplGetDataInt");

                if (get_data_int != 0) {
                    int32_t value = 0;

                    // raw code 0x516f1a..0x516f48; Ghidra dropped the dialog as unreachable
                    if (get_data_int(4, &value) != 0 && value != -1 && value != 0) {
                        shell_display_fatal_error_dialog(0x8d, 0x7e, 0);
                    }
                }
                FreeLibrary(library);
            }
        }

        if (video_memory_bytes < (rasterizer_minimum_video_memory_mb << 20)) {
            shell_display_fatal_error_dialog(0x6c, 0x75, 0);
        }

        desktop = GetDesktopWindow();
        hdc = GetDC(desktop);
        if (rasterizer_fullscreen == 0 && GetDeviceCaps(hdc, 0xc) != 32) {     // BITSPIXEL
            shell_display_fatal_error_dialog(0x83, 0x7e, 1);
        }
        ReleaseDC(GetDesktopWindow(), hdc);

        if (((d3d_get_adapter_display_mode_fn)direct3d_vtable()[0x20 / 4])(rasterizer_direct3d, adapter,
                                                                            &desktop_mode) < 0) {
            adapter_usable = 0;
            goto finish;
        }
        if (rasterizer_fullscreen != 0) {
            SetWindowLongA(hwnd, -0x10, (int32_t)0x90080000);    // GWL_STYLE: WS_POPUP|WS_VISIBLE|WS_SYSMENU
        }
        if (rasterizer_fullscreen == 0) {
            // the requested size must fit the desktop, and the desktop must be 32 bit
            GetWindowRect(GetDesktopWindow(), &desktop_rect);
            if ((uint32_t)mode.height >= (uint32_t)desktop_rect.bottom ||
                (uint32_t)mode.width >= (uint32_t)desktop_rect.right) {
                if (desktop_rect.bottom > 600) {
                    mode.width = 800;
                    mode.height = 600;
                } else {
                    mode.width = 640;
                    mode.height = 480;
                }
            }
            adapter_usable = (uint8_t)(desktop_mode.format >= 0x15 && desktop_mode.format <= 0x16);
        }

        rasterizer_build_present_parameters(&rasterizer_present_parameters, &mode);
        behavior_flags[3] = 0x20;                                // D3DCREATE_SOFTWARE_VERTEXPROCESSING
        for (;;) {
            uint8_t no_pixel_shaders = (uint8_t)(rasterizer_caps.pixel_shader_version < 0xffff0101);

            behavior_flags[0] = no_pixel_shaders ? 0x80 : 0x40;  // MIXED or HARDWARE
            behavior_flags[1] = no_pixel_shaders ? 0x20 : 0x40;
            behavior_flags[2] = no_pixel_shaders ? 0x20 : 0x80;
            // without D3DDEVCAPS_HWTRANSFORMANDLIGHT (dev_caps bit 16) skip the first choice
            for (i = (~(rasterizer_caps.dev_caps >> 16)) & 1; i < 4; i++) {
                uint32_t flags = behavior_flags[i] +
                                 (rasterizer_disable_driver_management != 0 ? 0x100 : 0) +
                                 (rasterizer_fpu_preserve != 0 ? 2 : 0) +
                                 (rasterizer_window_requested != 0 ? 4 : 0);

                if (((d3d_create_device_fn)direct3d_vtable()[0x40 / 4])(rasterizer_direct3d, adapter,
                                                                        rasterizer_device_type, hwnd, flags,
                                                                        &rasterizer_present_parameters,
                                                                        &rasterizer_device) >= 0) {
                    rasterizer_software_vertex_processing = (uint8_t)(behavior_flags[i] & 0x20);
                    goto device_created;
                }
            }
            if (unknown_0071d170 != 0) {
                unknown_0071d170 = (uint8_t)(mode.refresh_rate == 0);
                break;
            }
            // one retry with the fallback present parameters
            unknown_0071d170 = 1;
            rasterizer_build_present_parameters(&rasterizer_present_parameters, &mode);
        }

    device_created:
        if (rasterizer_fpu_preserve != 0) {
            rasterizer_fpu_reset_control_word(0x7e);
        }
        if ((adapter_usable != 0 && rasterizer_device != 0) || adapter_count <= 1) {
            break;
        }
        adapter_usable = 1;
    }

    rasterizer_desktop_display_mode = desktop_mode;
    rasterizer_resize_game_window(mode.height, mode.width);

finish:
    if (rasterizer_device == 0 || adapter_usable == 0) {
        rasterizer_device = 0;
        shell_display_fatal_error_dialog(0x81, 0x82, 1);
        return 0;
    }

    rasterizer_texture_stage_count = (int16_t)(rasterizer_caps.max_simultaneous_textures < 4 ? 2 : 4);
    rasterizer_adapter = adapter;
    rasterizer_maximum_skinning_nodes = 0x3f;
    // can the adapter render to an A8R8G8B8 surface on an X8R8G8B8 desktop?
    if (((d3d_check_device_format_fn)direct3d_vtable()[0x28 / 4])(rasterizer_direct3d, adapter, rasterizer_device_type,
                                                                  0x16, 1, 1, 0x15) < 0) {
        rasterizer_caps_flag_68a = 1;
    }
    if (rasterizer_fullscreen != 0 && rasterizer_device != 0) {
        ShowCursor(0);
    }
    rasterizer_device_lost = 0;
    rasterizer_pending_clear = 0;
    rasterizer_set_default_render_states();
    if (rasterizer_window_requested == 0) {
        rasterizer_render_loading_screen(1);
    }
    if (rasterizer_caps.pixel_shader_version < 0xffff0101) {
        if (rasterizer_texture_stage_count >= 2) {
            rasterizer_texture_stage_count = 2;
        }
        rasterizer_caps_flag_688 = 1;
    }
    if (command_line_has_switch("-usefxfile")) {
        rasterizer_use_fx_file = 1;
    }
    rasterizer_select_hardware_codepaths();

    succeeded = 0;
    if ((uint8_t)rasterizer_dx9_effects_initialize() &&
        (rasterizer_scratch_memory = GlobalAlloc(0, k_rasterizer_scratch_memory_size)) != 0 &&
        (uint8_t)rasterizer_decal_index_buffer_initialize() &&
        (uint8_t)transparent_geometry_pool_initialize() &&
        (uint8_t)text_font_system_initialize() &&
        rasterizer_detail_object_vertex_buffer_create() &&
        rasterizer_render_target_initialize() &&
        (uint8_t)rasterizer_lens_flare_occlusion_queries_create()) {
        succeeded = 1;
    }
    chimera__registry_check_4();

    // game_state_malloc(0x78), inlined: carve the block and fold its size into the state crc
    {
        uint32_t size = 0x78;
        uint8_t *bytes = (uint8_t *)&size;
        uint8_t *block = game_state_base + game_state_cursor;
        int32_t n;

        game_state_cursor += 0x78;
        if (crc32_lookup_table_initialized == 0) {
            crc32_build_table(&crc32_lookup_table);
            crc32_lookup_table_initialized = 1;
        }
        for (n = 0; n < 4; n++) {
            game_state_crc = (game_state_crc >> 8) ^ crc32_lookup_table.entries[(bytes[n] ^ game_state_crc) & 0xff];
        }
        cinematic_globals = (uint32_t *)block;
    }
    texture_cache_new();
    if (rasterizer_reset_device_if_needed()) {
        rasterizer_end_frame();
    }

    if (succeeded == 0) {
        rasterizer_shutdown();
        return 0;
    }
    viewport.x = 0;
    viewport.y = 0;
    viewport.width = (uint32_t)mode.width;
    viewport.height = (uint32_t)mode.height;
    viewport.min_z = 0.0f;
    viewport.max_z = 1.0f;
    if (((d3d_set_viewport_fn)(*(void ***)rasterizer_device)[0xbc / 4])(rasterizer_device, &viewport) < 0) {
        succeeded = 0;
    }
    rasterizer_frame_started = 1;
    rasterizer_editbox_log_dump();
    return succeeded;
}

#if 0
Original Ghidra decompilation (0x5169c0):

/* WARNING: Removing unreachable block (ram,0x00516f3b) */
/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

char rasterizer_initialize_direct3d(void)

{
  byte bVar1;
  char *pcVar2;
  char cVar3;
  uint uVar4;
  int iVar5;
  HMODULE hModule;
  FARPROC pFVar6;
  HWND pHVar7;
  HDC hdc;
  undefined4 uVar8;
  uint unaff_EBX;
  undefined4 unaff_ESI;
  int iVar9;
  uint uVar10;
  int **ppiVar11;
  uint uVar12;
  bool bVar13;
  tagRECT *lpRect;
  int *piStack_6c;
  undefined4 uStack_60;
  uint local_5c;
  char *pcStack_58;
  HWND pHStack_54;
  uint local_50;
  uint uStack_4c;
  uint local_48;
  int local_44 [2];
  undefined4 local_3c;
  undefined4 uStack_38;
  undefined4 uStack_34;
  int iStack_30;
  uint auStack_2c [4];
  tagRECT tStack_1c;
  
  local_3c = CONCAT31(local_3c._1_3_,DAT_007196d8 == 0);
  uStack_60 = CONCAT13(1,(undefined3)uStack_60);
  local_5c = 0;
  local_48 = 800;
  local_44[0] = 600;
  local_44[1] = 0x3c;
  DAT_0071d174 = (int *)0x0;
  DAT_0071d16c = DAT_0071d1a8 == 0 && DAT_0071d1ac == 0;
  if ((((DAT_007196f0 != 0) || (DAT_007196f4 != 0)) || (DAT_00722bac < 0x3e9)) ||
     (DAT_00722ba8 < 0x81)) {
    local_48 = 0x280;
    local_44[0] = 0x1e0;
  }
  piStack_6c = local_44 + 1;
  rasterizer_parse_vidmode_commandline(local_44);
  DAT_0069c689 = 0;
  DAT_0069c68a = 0;
  DAT_0069c688 = 0;
  piStack_6c = (int *)0x516a8a;
  cVar3 = rasterizer_create_game_window();
  if (cVar3 != '\0') {
    local_50 = DAT_007461c4;
    piStack_6c = DAT_00721e98;
    if (DAT_00721e98 == (int *)0x0) {
      piStack_6c = (int *)0x1f;
      piStack_6c = (int *)(*DAT_00746274)();
      if (piStack_6c == (int *)0x0) {
        DAT_0071d178 = piStack_6c;
        return '\0';
      }
    }
    DAT_0071d178 = piStack_6c;
    uVar4 = (**(code **)(*piStack_6c + 0x10))();
    if (uVar4 != 0) {
      local_50 = uVar4;
      if (((DAT_0071d16c != '\0') && (cVar3 = command_line_check_flag("-adapter"), cVar3 != '\0'))
         && (_sscanf(pcStack_58,"%d",&uStack_60), uVar4 < uStack_60)) {
        return '\0';
      }
      local_5c = 0xffffffff;
      uVar4 = uStack_60;
      uVar12 = local_50;
      do {
        if (local_5c == 0xffffffff) {
          if (uVar4 != 0) {
            uVar4 = uVar4 - 1;
            goto LAB_00516b45;
          }
        }
        else {
          uVar4 = local_5c;
          if (local_50 <= local_5c) goto LAB_005171de;
LAB_00516b45:
          iVar9 = 0;
          if (0 < DAT_00721e94) {
            do {
              pcVar2 = *(char **)(DAT_00721e90 + iVar9 * 4);
              if ((*pcVar2 == '-') && (iVar5 = __stricmp("-useref",pcVar2), iVar5 == 0)) {
                DAT_0069c6a0 = 2;
                goto LAB_00516b7f;
              }
              iVar9 = iVar9 + 1;
            } while (iVar9 < DAT_00721e94);
          }
          DAT_0069c6a0 = 1;
LAB_00516b7f:
          (**(code **)(*DAT_0071d178 + 0x38))(DAT_0071d178,uVar4,DAT_0069c6a0,&DAT_007c10c0);
          iVar9 = shell_parse_config_txt();
          if (iVar9 != 0) {
            shell_display_fatal_error_dialog(0xffffffff,iVar9,0);
          }
          uVar12 = 0xffffffff;
          if (DAT_00722b64 != 0) {
            if ((DAT_00722b64 == 9999) || (DAT_00722b64 == 0x270d)) {
              uVar12 = 0;
            }
            else if (DAT_00722b64 == 0x270e) {
              uVar12 = 0xffff0200;
            }
            else {
              uVar12 = (DAT_00722b64 / 10 | 0xffffff00) << 8 | DAT_00722b64 % 10;
            }
          }
          iVar9 = 0;
          if (0 < DAT_00721e94) {
            do {
              pcVar2 = *(char **)(DAT_00721e90 + iVar9 * 4);
              if ((*pcVar2 == '-') && (iVar5 = __stricmp("-useff",pcVar2), iVar5 == 0))
              goto LAB_00516c62;
              iVar9 = iVar9 + 1;
            } while (iVar9 < DAT_00721e94);
          }
          if (((DAT_007196f4 != 0) || (DAT_00722b60 != 0)) || (DAT_00722b34 != 0)) {
LAB_00516c62:
            DAT_00722b64 = 0;
            uVar12 = 0;
          }
          iVar9 = 0;
          if (0 < DAT_00721e94) {
            do {
              pcVar2 = *(char **)(DAT_00721e90 + iVar9 * 4);
              if ((*pcVar2 == '-') && (iVar5 = __stricmp("-use00",pcVar2), iVar5 == 0)) {
                DAT_00722b64 = 0;
                uVar12 = 0;
                DAT_007c117c = 1;
                break;
              }
              iVar9 = iVar9 + 1;
            } while (iVar9 < DAT_00721e94);
          }
          iVar9 = 0;
          if (0 < DAT_00721e94) {
            do {
              pcVar2 = *(char **)(DAT_00721e90 + iVar9 * 4);
              if ((*pcVar2 == '-') && (iVar5 = __stricmp("-use11",pcVar2), iVar5 == 0)) {
                DAT_00722b64 = 0;
                uVar12 = 0xffff0101;
                break;
              }
              iVar9 = iVar9 + 1;
            } while (iVar9 < DAT_00721e94);
          }
          iVar9 = 0;
          if (0 < DAT_00721e94) {
            do {
              pcVar2 = *(char **)(DAT_00721e90 + iVar9 * 4);
              if ((*pcVar2 == '-') && (iVar5 = __stricmp("-use14",pcVar2), iVar5 == 0)) {
                DAT_00722b64 = 0;
                uVar12 = 0xffff0104;
                break;
              }
              iVar9 = iVar9 + 1;
            } while (iVar9 < DAT_00721e94);
          }
          iVar9 = 0;
          if (0 < DAT_00721e94) {
            do {
              pcVar2 = *(char **)(DAT_00721e90 + iVar9 * 4);
              if ((*pcVar2 == '-') && (iVar5 = __stricmp("-use20",pcVar2), iVar5 == 0)) {
                DAT_00722b64 = 0;
                uVar12 = 0xffff0200;
                break;
              }
              iVar9 = iVar9 + 1;
            } while (iVar9 < DAT_00721e94);
          }
          iVar9 = 0;
          if (0 < DAT_00721e94) {
            do {
              pcVar2 = *(char **)(DAT_00721e90 + iVar9 * 4);
              if ((*pcVar2 == '-') && (iVar5 = __stricmp("-use2a",pcVar2), iVar5 == 0)) {
                DAT_00722b64 = 0;
                uVar12 = 0xffff0200;
                goto LAB_00516dc3;
              }
              iVar9 = iVar9 + 1;
            } while (iVar9 < DAT_00721e94);
          }
          if (uVar12 != 0xffffffff) {
LAB_00516dc3:
            if ((uVar12 & 0xffff) < (DAT_007c118c & 0xffff)) {
              DAT_007c118c = uVar12;
            }
          }
          if (DAT_00722b40 != 0) {
            shell_display_fatal_error_dialog(0x93,0x70,0);
          }
          if (DAT_00722b3c != 0) {
            shell_display_fatal_error_dialog(0x67,0x70,0);
          }
          if ((DAT_00722b4c != 0) && (DAT_00722b3c == 0)) {
            shell_display_fatal_error_dialog(0x69,0x72,0);
          }
          if (((DAT_00722b44 != 0) && (DAT_00722b4c == 0)) && (DAT_00722b3c == 0)) {
            shell_display_fatal_error_dialog(0x68,0x71,0);
          }
          if (DAT_00722b50 != 0) {
            shell_display_fatal_error_dialog(0x8f,0x72,0);
          }
          if ((DAT_00722b48 != 0) && (DAT_00722b50 == 0)) {
            shell_display_fatal_error_dialog(0x8e,0x71,0);
          }
          if (DAT_00722b70 != 0) {
            DAT_0069c689 = 1;
            DAT_0069c68a = 1;
          }
          if (DAT_00722b74 != 0) {
            DAT_0069c68a = 1;
          }
          if (DAT_00722b9c == 0x1002) {
            if ((((DAT_00722b98 == 0x514c) || (DAT_00722b98 == 0x514e)) || (DAT_00722b98 == 0x514f))
               || (DAT_00722b98 == 0x4242)) {
              DAT_0069c688 = 1;
            }
          }
          else if ((DAT_00722b9c == 0x10de) &&
                  (hModule = LoadLibraryA("NVCPL.dll"), hModule != (HMODULE)0x0)) {
            pFVar6 = GetProcAddress(hModule,"NvCplGetDataInt");
            if (pFVar6 != (FARPROC)0x0) {
              unaff_ESI = 0;
              (*pFVar6)(4,&stack0xffffff98);
            }
            FreeLibrary(hModule);
          }
          if (DAT_00722bb0 < (uint)(DAT_006effc8 << 0x14)) {
            shell_display_fatal_error_dialog(0x6c,0x75,0);
          }
          pHVar7 = GetDesktopWindow();
          hdc = GetDC(pHVar7);
          if ((DAT_0071d16c == '\0') && (iVar9 = GetDeviceCaps(hdc,0xc), iVar9 != 0x20)) {
            shell_display_fatal_error_dialog(0x83,0x7e,1);
          }
          pHVar7 = GetDesktopWindow();
          ReleaseDC(pHVar7,hdc);
          iVar9 = (**(code **)(*DAT_0071d178 + 0x20))(DAT_0071d178,uVar4,&uStack_4c);
          uVar12 = uVar4;
          if (iVar9 < 0) {
            unaff_EBX = unaff_EBX & 0xffffff;
            goto LAB_005171de;
          }
          if ((DAT_0071d16c == '\0') ||
             (SetWindowLongA(pHStack_54,-0x10,-0x6ff80000), DAT_0071d16c == '\0')) {
            lpRect = &tStack_1c;
            pHVar7 = GetDesktopWindow();
            GetWindowRect(pHVar7,lpRect);
            if (((uint)tStack_1c.bottom <= local_48) || ((uint)tStack_1c.right <= uStack_4c)) {
              if (tStack_1c.bottom < 0x259) {
                uStack_4c = 0x280;
                local_48 = 0x1e0;
              }
              else {
                uStack_4c = 800;
                local_48 = 600;
              }
            }
            if ((iStack_30 < 0x15) || (unaff_EBX = CONCAT13(1,(int3)unaff_EBX), 0x16 < iStack_30)) {
              unaff_EBX = unaff_EBX & 0xffffff;
            }
          }
          rasterizer_build_present_parameters(&DAT_007c04a0);
          auStack_2c[3] = 0x20;
          while( true ) {
            bVar13 = DAT_007c118c < 0xffff0101;
            auStack_2c[0] = (-(uint)bVar13 & 0x40) + 0x40;
            auStack_2c[1] = (-(uint)bVar13 & 0xffffffe0) + 0x40;
            auStack_2c[2] = (-(uint)bVar13 & 0xffffffa0) + 0x80;
            uVar10 = ~(uint)DAT_007c10de & 1;
            if (uVar10 < 4) {
              do {
                iVar9 = (**(code **)(*DAT_0071d178 + 0x40))
                                  (DAT_0071d178,uVar4,DAT_0069c6a0,pHStack_54,
                                   auStack_2c[uVar10] + (-(uint)(DAT_00722b38 != 0) & 0x100) +
                                   (-(uint)(DAT_0071d1a4 != 0) & 2) +
                                   (-(uint)(DAT_0071d1a8 != 0) & 4),&DAT_007c04a0,&DAT_0071d174);
                if (-1 < iVar9) {
                  DAT_0069c680 = (byte)auStack_2c[uVar10] & 0x20;
                  goto LAB_0051715e;
                }
                uVar10 = uVar10 + 1;
              } while ((int)uVar10 < 4);
            }
            if (DAT_0071d170 != '\0') break;
            DAT_0071d170 = '\x01';
            rasterizer_build_present_parameters(&DAT_007c04a0);
          }
          DAT_0071d170 = local_44[0] == 0;
LAB_0051715e:
          if (DAT_0071d1a4 != 0) {
            pcStack_58 = (char *)0x7e;
          }
          if ((((char)(unaff_EBX >> 0x18) != '\0') && (DAT_0071d174 != (int *)0x0)) ||
             (local_50 < 2)) goto LAB_005171aa;
          unaff_EBX = CONCAT13(1,(int3)unaff_EBX);
          uVar4 = uStack_60;
        }
        local_5c = local_5c + 1;
      } while( true );
    }
  }
  return '\0';
LAB_005171aa:
  _DAT_007c11f0 = local_3c;
  _DAT_007c11f4 = uStack_38;
  DAT_007c11f8 = uStack_34;
  _DAT_007c11fc = iStack_30;
  rasterizer_resize_game_window();
LAB_005171de:
  if ((DAT_0071d174 == (int *)0x0) || ((char)(unaff_EBX >> 0x18) == '\0')) {
    DAT_0071d174 = (int *)0x0;
    shell_display_fatal_error_dialog(0x81,0x82,1);
    return '\0';
  }
  DAT_0069c682 = (-(ushort)(DAT_007c1158 < 4) & 0xfffe) + 4;
  DAT_0069c67e = 0x3f;
  DAT_0071d180 = uVar12;
  iVar9 = (**(code **)(*DAT_0071d178 + 0x28))(DAT_0071d178,uVar12,DAT_0069c6a0,0x16,1,1,0x15);
  if (iVar9 < 0) {
    DAT_0069c68a = 1;
  }
  if ((DAT_0071d16c != '\0') && (DAT_0071d174 != (int *)0x0)) {
    ShowCursor(0);
  }
  DAT_007c10b0 = 0;
  DAT_0071d16e = 0;
  rasterizer_set_default_render_states();
  if (DAT_0071d1a8 == 0) {
    FUN_005157e0();
  }
  if (DAT_007c118c < 0xffff0101) {
    if (1 < DAT_0069c682) {
      DAT_0069c682 = 2;
    }
    DAT_0069c688 = 1;
  }
  iVar9 = 0;
  if (0 < DAT_00721e94) {
    do {
      pcVar2 = *(char **)(DAT_00721e90 + iVar9 * 4);
      if ((*pcVar2 == '-') && (iVar5 = __stricmp("-usefxfile",pcVar2), iVar5 == 0)) {
        DAT_0071d18e = 1;
        break;
      }
      iVar9 = iVar9 + 1;
    } while (iVar9 < DAT_00721e94);
  }
  rasterizer_select_hardware_codepaths();
  cVar3 = rasterizer_dx9_effects_initialize();
  if ((((cVar3 == '\0') || (DAT_0071d13c = GlobalAlloc(0,0x18000), DAT_0071d13c == (HGLOBAL)0x0)) ||
      (cVar3 = FUN_0051bb90(), cVar3 == '\0')) ||
     (((iVar9 = transparent_geometry_pool_initialize(), (char)iVar9 == '\0' ||
       (iVar9 = text_font_system_initialize(), (char)iVar9 == '\0')) ||
      ((uVar8 = rasterizer_decal_dynamic_vertex_buffer_create(), (char)uVar8 == '\0' ||
       ((cVar3 = FUN_0052ca20(), cVar3 == '\0' ||
        (bVar13 = rasterizer_lens_flare_occlusion_queries_create(), !bVar13)))))))) {
    cVar3 = '\0';
  }
  else {
    cVar3 = '\x01';
  }
  chimera__registry_check_4();
  iVar9 = DAT_006e2dcc + DAT_006e2dc8;
  DAT_006e2dcc = DAT_006e2dcc + 0x78;
  piStack_6c = (int *)0x78;
  ppiVar11 = &piStack_6c;
  if (DAT_00719cd8 == '\0') {
    crc32_build_table();
    DAT_00719cd8 = '\x01';
  }
  iVar5 = 4;
  do {
    bVar1 = *(byte *)ppiVar11;
    ppiVar11 = (int **)((int)ppiVar11 + 1);
    DAT_006e2dd4 = DAT_006e2dd4 >> 8 ^ (&DAT_006b7b00)[(bVar1 ^ DAT_006e2dd4) & 0xff];
    iVar5 = iVar5 + -1;
  } while (iVar5 != 0);
  DAT_0071cfc4 = iVar9;
  texture_cache_new();
  bVar13 = rasterizer_reset_device_if_needed();
  if (bVar13) {
    chimera__rasterizer_globals();
  }
  if (cVar3 != '\0') {
    uStack_38 = 0;
    uStack_34 = 0;
    auStack_2c[1] = 0;
    auStack_2c[2] = 0x3f800000;
    iStack_30 = unaff_ESI;
    auStack_2c[0] = unaff_EBX;
    iVar9 = (**(code **)(*DAT_0071d174 + 0xbc))(DAT_0071d174,&uStack_38);
    if (iVar9 < 0) {
      cVar3 = '\0';
    }
    DAT_0069c630 = 1;
    rasterizer_editbox_log_dump();
    return cVar3;
  }
  rasterizer_shutdown();
  return '\0';
}
#endif
