// timedemo_benchmark_update  (Ghidra: timedemo_benchmark_update, already named)
// address 0x4c6f30, size 1755 bytes
// name confidence: 0.6   rewrite confidence: 0.8
// evidence: types/main.h timedemo_globals (0x00719afc) and timedemo_step; raw disassembly
//   0x4c6f30..0x4c760a (phase 4 review) for every argument order and signedness. One caller, the
//   main loop. 0x007196d8 is the -timedemo frame counter (types/game.h
//   game_time_force_single_tick, set by shell_winmain); 0x007196dc keeps the low dword of the
//   64 bit present counter 0x0069c648 last sampled (compared against its sign extension, cdq).
//   Frame times come from time_query_performance_counter_ms 0x449210. Bucket thresholds are
//   unsigned (jbe). The report globals are the shell hardware probe (types/shell.h: cpu_speed
//   0x00722bac, physical_memory 0x00722ba8, video_memory 0x00722bb0, graphics_vendor_name /
//   device_name / device_id 0x00722b90 / 0x00722b94 / 0x00722b98, graphics_driver_version
//   0x00722ba0 as four WORDs, config_force_shader 0x00722b64 (9999 = 2.0a),
//   shell_command_line 0x006e35c0, shell_startup_tick_count 0x006f03e8), the D3D caps pixel
//   shader version 0x007c118c (rasterizer_caps +0xcc), the present parameters back buffer size
//   0x007c04a0 / 0x007c04a4, and the sound and render options below.
//   Constants: 0x00672c78 = 2^32 (double), 0x00672bc0 = 2^32 (float), 0x00672bf8 = 0.001f,
//   0x00673128 = 1000.0, 0x00673120 = 0.001.
// register convention: cdecl, no arguments. main_queue_map_change takes EAX = map name
//   (0x4c7106 `mov eax,0x669a58`), hs_compile_and_evaluate takes its command on the stack.
// UNSURE: the names of the sound / render option globals are taken from the report labels
//   they feed: 0x007252b8 feeds "Sound Variety" (types/sound.h sound_permutation_limit),
//   0x00746128 "Sound Quality", 0x00746121 "Hardware Acceleration", 0x006893fa "Specular",
//   0x006893f2 "Shadows", 0x006893f5 "Decals", 0x0068944c "Particles", 0x0068944e "Texture
//   Quality", 0x007c11f8 "Refresh rate". 0x006ac5b2 (directors[0].look_input_consumed, R03),
//   player_globals +0x11 and 0x0087ac05 are forced to 1 every call; their meaning is not
//   established. user_profile_signin_state_is_valid 0x551620 gates "Environmental Sound= EAX";
//   its name comes from the game module and is doubtful in this context.
// reconciled: R03 0x006ac5b2 identified as camera.h director.look_input_consumed (comment only)

#include "crt.h"
#include "win32.h"
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "interface.h"
#include "rasterizer.h"
#include "main.h"
#include "fn_hs.h"
#include <stdio.h>

extern main_globals main_globals_data;              // 0x00719700
extern timedemo_globals timedemo_globals_data;      // 0x00719afc
extern int32_t game_time_force_single_tick;         // 0x007196d8, -timedemo frame counter
extern int32_t timedemo_last_frame_index;           // 0x007196dc
extern int32_t rasterizer_present_counter_low;      // 0x0069c648, foreign (rasterizer)
extern int32_t rasterizer_present_counter_high;     // 0x0069c64c
extern uint8_t local_player_input_frozen[];         // 0x006ac5b2 = camera.h directors[0].look_input_consumed
extern player_globals *local_player_globals;        // 0x0087a478, foreign (game)
extern uint8_t console_debug_flag_5;                // 0x0087ac05, foreign, UNSURE
extern char timedemo_pixel_shader_version[0x14];    // 0x006b7a94
extern d3d_caps9 rasterizer_caps;                   // 0x007c10c0 (pixel shader version at 0x007c118c)
extern d3d_present_parameters rasterizer_present_parameters; // 0x007c04a0
extern int32_t os_platform_refresh_default;         // 0x007c11f8, foreign, UNSURE name
extern int32_t config_force_shader;                 // 0x00722b64, foreign (shell)
extern char *graphics_vendor_name;                  // 0x00722b90, foreign (shell)
extern char *graphics_device_name;                  // 0x00722b94
extern uint32_t graphics_device_id;                 // 0x00722b98
extern uint16_t graphics_driver_version[4];         // 0x00722ba0 (low word first)
extern uint32_t physical_memory;                    // 0x00722ba8
extern uint32_t cpu_speed;                          // 0x00722bac
extern uint32_t video_memory;                       // 0x00722bb0
extern char *shell_command_line;                    // 0x006e35c0, foreign (shell)
extern uint32_t shell_startup_tick_count;           // 0x006f03e8, foreign (shell)
extern int32_t shell_nosound;                       // 0x007196e4, foreign (shell)
extern int16_t sound_permutation_limit;             // 0x007252b8, foreign (sound)
extern uint8_t directsound_eax_enabled;             // 0x00746121, foreign (sound)
extern int32_t directsound_quality;                 // 0x00746128, foreign (sound)
extern int16_t renderer_texture_quality;            // 0x0068944e, foreign, UNSURE (WORD reads here)
extern int16_t light_count_enabled;               // 0x0068944c, foreign, UNSURE (WORD reads here)
extern uint8_t decals_for_all_responses;            // 0x006893f5, foreign (effects)
extern uint8_t console_debug_toggle_6893f2;         // 0x006893f2 object shadows enabled
extern uint8_t console_debug_toggle_6893fa;         // 0x006893fa specular / reflections enable

extern uint32_t time_query_performance_counter_ms(void);            // 0x449210, foreign (math)
extern void main_queue_map_change(char *map_name);                  // this module, 0x4c8740
    // blam-cc: EAX -> map_name

extern uint32_t user_profile_signin_state_is_valid(void);           // 0x551620, foreign (game), UNSURE name
// fopen: <stdio.h>, resolved to the game CRT at 0x624186      // 0x624186, CRT fopen wrapper

// Once per presented frame while -timedemo runs: samples the frame time into the timedemo
// buckets, keeps the local player idle, and steps the benchmark script (a30 at frame 100, then
// b30, c10 and d20 through hs map_name, and at frame 0x125c appends the report to
// timedemo.txt and sets main_globals.quit). The frame counter advances on every call.
void timedemo_benchmark_update(void)
{
    uint32_t frame_time;
    int32_t step;
    FILE *file;
    char module_path[0x104];          // [esp+0x58] after the four pushes
    char date[0x20];                  // [esp+0x38]
    char time[0x20];                  // [esp+0x18]
    uint32_t version_size;
    uint32_t version_handle;          // [esp+0x14], reused as the VerQueryValue length
    void *version_data;
    uint32_t *fixed_file_info;        // [esp+0x10] VS_FIXEDFILEINFO
    const char *shader;
    const char *frame_suffix;
    double total_time;
    double frame_count;
    float slow_time;
    const char *sound_variety;
    const char *sound_quality;
    const char *environmental_sound;
    const char *hardware_acceleration;
    const char *texture_quality;
    const char *particles;
    const char *decals;
    const char *shadows;
    const char *specular;

    if (game_time_force_single_tick == 0) {
        return;
    }

    if (rasterizer_present_counter_low != timedemo_last_frame_index ||
        rasterizer_present_counter_high != (timedemo_last_frame_index >> 31)) {
        timedemo_last_frame_index = rasterizer_present_counter_low;
        timedemo_globals_data.current_time_ms = time_query_performance_counter_ms();
        frame_time = timedemo_globals_data.current_time_ms - timedemo_globals_data.previous_time_ms;
        timedemo_globals_data.previous_time_ms = timedemo_globals_data.current_time_ms;
        timedemo_globals_data.frame_time_ms = frame_time;
        if (timedemo_globals_data.frame_count == 0) {
            frame_time = 1;
            timedemo_globals_data.frame_time_ms = 1;
        }
        timedemo_globals_data.frame_count++;
        timedemo_globals_data.total_time_ms += frame_time;
        if (frame_time > k_timedemo_below_60fps_ms) {
            timedemo_globals_data.buckets[0].time_ms += frame_time;
            timedemo_globals_data.buckets[0].frames++;
        }
        if (frame_time > k_timedemo_below_50fps_ms) {
            timedemo_globals_data.buckets[1].time_ms += frame_time;
            timedemo_globals_data.buckets[1].frames++;
        }
        if (frame_time > k_timedemo_below_40fps_ms) {
            timedemo_globals_data.buckets[2].time_ms += frame_time;
            timedemo_globals_data.buckets[2].frames++;
        }
        if (frame_time > k_timedemo_below_30fps_ms) {
            timedemo_globals_data.buckets[3].time_ms += frame_time;
            timedemo_globals_data.buckets[3].frames++;
        }
        if (frame_time > k_timedemo_below_25fps_ms) {
            timedemo_globals_data.buckets[4].time_ms += frame_time;
            timedemo_globals_data.buckets[4].frames++;
        }
        if (frame_time > k_timedemo_below_20fps_ms) {
            timedemo_globals_data.buckets[5].time_ms += frame_time;
            timedemo_globals_data.buckets[5].frames++;
        }
        if (frame_time > k_timedemo_below_15fps_ms) {
            timedemo_globals_data.buckets[6].time_ms += frame_time;
            timedemo_globals_data.buckets[6].frames++;
        }
        if (frame_time > k_timedemo_below_10fps_ms) {
            timedemo_globals_data.buckets[7].time_ms += frame_time;
            timedemo_globals_data.buckets[7].frames++;
        }
        if (frame_time > k_timedemo_below_5fps_ms) {
            timedemo_globals_data.buckets[8].time_ms += frame_time;
            timedemo_globals_data.buckets[8].frames++;
        }
    }

    step = game_time_force_single_tick;
    local_player_input_frozen[0] = 1;
    *((uint8_t *)local_player_globals + 0x11) = 1;   // player_globals.unknown_11
    console_debug_flag_5 = 1;

    switch (step) {
    case _timedemo_step_load_a30:
        main_queue_map_change((char *)"a30");                 // 0x00669a58
        game_time_force_single_tick++;
        return;
    case _timedemo_step_load_b30:
        hs_compile_and_evaluate("map_name b30");      // 0x0066b224
        break;
    case _timedemo_step_load_c10:
        hs_compile_and_evaluate("map_name c10");      // 0x0066b890
        break;
    case _timedemo_step_load_d20:
        hs_compile_and_evaluate("map_name d20");      // 0x0066b880
        break;
    case _timedemo_step_report:
        main_globals_data.quit = 1;
        file = (FILE *)fopen("timedemo.txt", "a");
        GetModuleFileNameA(0, module_path, 0x104);
        fseek(file, 0, SEEK_END);
        GetDateFormatA(0x400 /* LOCALE_USER_DEFAULT */, 0, 0, 0, date, 0x20);
        GetTimeFormatA(0x400, 0, 0, 0, time, 0x20);
        fprintf(file, "Date / Time: %s %s (%dms)\n", date, time, shell_startup_tick_count);

        if (config_force_shader == 9999) {
            shader = "2.0a";
        } else if (rasterizer_caps.pixel_shader_version < 0xffff0101u) {
            shader = "Fixed Function";
        } else {
            sprintf(timedemo_pixel_shader_version, "%d.%d",
                (rasterizer_caps.pixel_shader_version >> 8) & 0xff,
                rasterizer_caps.pixel_shader_version & 0xff);
            shader = timedemo_pixel_shader_version;
        }
        if (graphics_device_id != 0) {
            fprintf(file, "%dMHz, %dMB, %dM %s %s (DeviceID=0x%04x) Driver=%d.%d.%d.%d Shader=%s\n",
                cpu_speed, physical_memory, video_memory >> 20, graphics_vendor_name,
                graphics_device_name, graphics_device_id,
                (uint32_t)graphics_driver_version[3], (uint32_t)graphics_driver_version[2],
                (uint32_t)graphics_driver_version[1], (uint32_t)graphics_driver_version[0], shader);
        } else {
            fprintf(file, "%dMHz, %dMB\n", cpu_speed, physical_memory);
        }

        fprintf(file, "%s %s", module_path, shell_command_line);
        version_size = GetFileVersionInfoSizeA(module_path, &version_handle);
        version_data = GlobalAlloc(0, version_size);
        GetFileVersionInfoA(module_path, 0, version_size, version_data);
        VerQueryValueA(version_data, "\\", (void **)&fixed_file_info, &version_handle);
        fprintf(file, "   (Version=%d.%d.%d.%d)\n",
            fixed_file_info[2] >> 16, fixed_file_info[2] & 0xffff,   // dwFileVersionMS
            fixed_file_info[3] >> 16, fixed_file_info[3] & 0xffff);  // dwFileVersionLS
        GlobalFree(version_data);

        frame_suffix = timedemo_globals_data.buckets[8].frames == 1 ? ")" : "s)";
        total_time = (double)timedemo_globals_data.total_time_ms;
        slow_time = (float)timedemo_globals_data.buckets[8].time_ms;
        frame_count = (double)timedemo_globals_data.frame_count;
        fprintf(file,
            "Frames=%d\nTotal Time=%.2fs\nAverage frame rate=%.2ffps\n"
            "Below  5fps=% 2d%% (time)  %d%% (frames) (%.3fs spent in %d frame%s\n"
            "Below 10fps=% 2d%% (time)  %d%% (frames)\n"
            "Below 15fps=% 2d%% (time)  %d%% (frames)\n"
            "Below 20fps=% 2d%% (time)  %d%% (frames)\n"
            "Below 25fps=% 2d%% (time)  %d%% (frames)\n"
            "Below 30fps=% 2d%% (time)  %d%% (frames)\n"
            "Below 40fps=% 2d%% (time)  %d%% (frames)\n"
            "Below 50fps=% 2d%% (time)  %d%% (frames)\n"
            "Below 60fps=% 2d%% (time)  %d%% (frames)\n",
            timedemo_globals_data.frame_count,
            total_time * 0.001,
            1000.0 / (total_time / frame_count),
            timedemo_globals_data.buckets[8].time_ms * 100 / timedemo_globals_data.total_time_ms,
            timedemo_globals_data.buckets[8].frames * 100 / timedemo_globals_data.frame_count,
            (double)(slow_time * 0.001f),
            timedemo_globals_data.buckets[8].frames,
            frame_suffix,
            timedemo_globals_data.buckets[7].time_ms * 100 / timedemo_globals_data.total_time_ms,
            timedemo_globals_data.buckets[7].frames * 100 / timedemo_globals_data.frame_count,
            timedemo_globals_data.buckets[6].time_ms * 100 / timedemo_globals_data.total_time_ms,
            timedemo_globals_data.buckets[6].frames * 100 / timedemo_globals_data.frame_count,
            timedemo_globals_data.buckets[5].time_ms * 100 / timedemo_globals_data.total_time_ms,
            timedemo_globals_data.buckets[5].frames * 100 / timedemo_globals_data.frame_count,
            timedemo_globals_data.buckets[4].time_ms * 100 / timedemo_globals_data.total_time_ms,
            timedemo_globals_data.buckets[4].frames * 100 / timedemo_globals_data.frame_count,
            timedemo_globals_data.buckets[3].time_ms * 100 / timedemo_globals_data.total_time_ms,
            timedemo_globals_data.buckets[3].frames * 100 / timedemo_globals_data.frame_count,
            timedemo_globals_data.buckets[2].time_ms * 100 / timedemo_globals_data.total_time_ms,
            timedemo_globals_data.buckets[2].frames * 100 / timedemo_globals_data.frame_count,
            timedemo_globals_data.buckets[1].time_ms * 100 / timedemo_globals_data.total_time_ms,
            timedemo_globals_data.buckets[1].frames * 100 / timedemo_globals_data.frame_count,
            timedemo_globals_data.buckets[0].time_ms * 100 / timedemo_globals_data.total_time_ms,
            timedemo_globals_data.buckets[0].frames * 100 / timedemo_globals_data.frame_count);

        if (shell_nosound != 0) {
            fprintf(file, "###Sound Options###\nSound Disabled\n");
        } else {
            if (sound_permutation_limit == 2) {
                sound_variety = "High";
            } else if (sound_permutation_limit == 1) {
                sound_variety = "Medium";
            } else {
                sound_variety = "Low";
            }
            if (directsound_eax_enabled != 0 && user_profile_signin_state_is_valid() != 0) {
                environmental_sound = "EAX";
            } else {
                environmental_sound = "No";
            }
            if (directsound_quality == 2) {
                sound_quality = "High";
            } else if (directsound_quality == 1) {
                sound_quality = "Normal";
            } else {
                sound_quality = "Low";
            }
            hardware_acceleration = directsound_eax_enabled == 1 ? "Yes" : "No";
            fprintf(file,
                "###Sound Options###\nHardware Acceleration= %s\nSound Quality= %s\n"
                "Environmental Sound= %s\nSound Variety= %s\n",
                hardware_acceleration, sound_quality, environmental_sound, sound_variety);
        }

        if (renderer_texture_quality == 0) {
            texture_quality = "High";
        } else if (renderer_texture_quality == 1) {
            texture_quality = "Medium";
        } else {
            texture_quality = "Low";
        }
        if (light_count_enabled == 0) {
            particles = "High";
        } else if (light_count_enabled == 1) {
            particles = "Low";
        } else {
            particles = "Off";
        }
        decals = decals_for_all_responses != 0 ? "Yes" : "No";
        shadows = console_debug_toggle_6893f2 != 0 ? "Yes" : "No";
        specular = console_debug_toggle_6893fa != 0 ? "Yes" : "No";
        fprintf(file,
            "###Video Options###\nResolution= %d x %d\nRefresh rate= %d Hz\n"
            "Framerate throttle= No Vsync\nSpecular= %s\nShadows= %s\nDecals= %s\nParticles= %s\n"
            "Texture Quality= %s\n\nFor further information, please visit the timedemo FAQ at: "
            "http://halo.bungie.net/site/halo/features/hpcperformancefaq.html \n",
            rasterizer_present_parameters.back_buffer_width,
            rasterizer_present_parameters.back_buffer_height,
            os_platform_refresh_default, specular, shadows, decals, particles, texture_quality);
        fclose(file);
        break;
    default:
        break;
    }
    game_time_force_single_tick++;
}

#if 0
Original Ghidra decompilation (0x4c6f30):

void __cdecl timedemo_benchmark_update(void)

{
  double dVar1;
  double dVar2;
  float fVar3;
  int iVar4;
  char cVar5;
  FILE *_File;
  DWORD dwBytes;
  HGLOBAL lpData;
  undefined **ppuVar6;
  undefined **ppuVar7;
  char *pcVar8;
  undefined **ppuVar9;
  undefined **ppuVar10;
  undefined1 *puVar11;
  undefined *puVar12;
  bool bVar13;
  undefined8 uVar14;
  char *pcVar15;
  LPVOID local_14c;
  DWORD local_148;
  CHAR local_144 [32];
  CHAR local_124 [32];
  CHAR local_104 [260];

  if (DAT_007196d8 == 0) {
    return;
  }
  if ((DAT_0069c648 != DAT_007196dc) || (DAT_0069c64c != DAT_007196dc >> 0x1f)) {
    DAT_007196dc = DAT_0069c648;
    _DAT_00719b54 = FUN_00449210();
    _DAT_00719b5c = _DAT_00719b54 - DAT_00719b58;
    if (DAT_00719afc == (LPVOID)0x0) {
      _DAT_00719b5c = 1;
    }
    DAT_00719afc = (LPVOID)((int)DAT_00719afc + 1);
    DAT_00719b00 = DAT_00719b00 + _DAT_00719b5c;
    if (0x10 < _DAT_00719b5c) {
      DAT_00719b04 = DAT_00719b04 + _DAT_00719b5c;
      DAT_00719b08 = DAT_00719b08 + 1;
    }
    if (0x14 < _DAT_00719b5c) {
      DAT_00719b0c = DAT_00719b0c + _DAT_00719b5c;
      DAT_00719b10 = DAT_00719b10 + 1;
    }
    if (0x19 < _DAT_00719b5c) {
      DAT_00719b14 = DAT_00719b14 + _DAT_00719b5c;
      DAT_00719b18 = DAT_00719b18 + 1;
    }
    if (0x21 < _DAT_00719b5c) {
      DAT_00719b1c = DAT_00719b1c + _DAT_00719b5c;
      DAT_00719b20 = DAT_00719b20 + 1;
    }
    if (0x28 < _DAT_00719b5c) {
      DAT_00719b24 = DAT_00719b24 + _DAT_00719b5c;
      DAT_00719b28 = DAT_00719b28 + 1;
    }
    if (0x32 < _DAT_00719b5c) {
      DAT_00719b2c = DAT_00719b2c + _DAT_00719b5c;
      DAT_00719b30 = DAT_00719b30 + 1;
    }
    if (0x42 < _DAT_00719b5c) {
      DAT_00719b34 = DAT_00719b34 + _DAT_00719b5c;
      DAT_00719b38 = DAT_00719b38 + 1;
    }
    if (100 < _DAT_00719b5c) {
      DAT_00719b3c = DAT_00719b3c + _DAT_00719b5c;
      DAT_00719b40 = DAT_00719b40 + 1;
    }
    DAT_00719b58 = _DAT_00719b54;
    if (200 < _DAT_00719b5c) {
      DAT_00719b44 = DAT_00719b44 + _DAT_00719b5c;
      DAT_00719b48 = DAT_00719b48 + 1;
    }
  }
  iVar4 = DAT_007196d8;
  bVar13 = DAT_007196d8 == 0x898;
  DAT_006ac5b2 = 1;
  *(undefined1 *)(DAT_0087a478 + 0x11) = 1;
  DAT_0087ac05 = 1;
  if (iVar4 < 0x899) {
    if (bVar13) {
      pcVar15 = "map_name c10";
    }
    else {
      if (iVar4 == 100) {
        main_queue_map_change();
        DAT_007196d8 = DAT_007196d8 + 1;
        return;
      }
      if (iVar4 != 0x44c) goto LAB_004c75fd;
      pcVar15 = "map_name b30";
    }
  }
  else {
    if (iVar4 != 0xc4e) {
      if (iVar4 != 0x125c) goto LAB_004c75fd;
      DAT_0071975b = 1;
      _File = (FILE *)FUN_00624186("timedemo.txt",&DAT_0066b87c);
      GetModuleFileNameA((HMODULE)0x0,local_104,0x104);
      _fseek(_File,0,2);
      GetDateFormatA(0x400,0,(SYSTEMTIME *)0x0,(LPCSTR)0x0,local_124,0x20);
      GetTimeFormatA(0x400,0,(SYSTEMTIME *)0x0,(LPCSTR)0x0,local_144,0x20);
      _fprintf(_File,"Date / Time: %s %s (%dms)\n",local_124,local_144,DAT_006f03e8);
      if (DAT_00722b64 == 0x270e) {
        pcVar15 = "2.0a";
      }
      else if (DAT_007c118c < 0xffff0101) {
        pcVar15 = "Fixed Function";
      }
      else {
        _sprintf(&DAT_006b7a94,"%d.%d",DAT_007c118c >> 8 & 0xff,DAT_007c118c & 0xff);
        pcVar15 = &DAT_006b7a94;
      }
      if (DAT_00722b98 == 0) {
        _fprintf(_File,"%dMHz, %dMB\n",DAT_00722bac,DAT_00722ba8);
      }
      else {
        _fprintf(_File,"%dMHz, %dMB, %dM %s %s (DeviceID=0x%04x) Driver=%d.%d.%d.%d Shader=%s\n",
                 DAT_00722bac,DAT_00722ba8,DAT_00722bb0 >> 0x14,DAT_00722b90,DAT_00722b94,
                 DAT_00722b98,(uint)DAT_00722ba6,(uint)DAT_00722ba4,(uint)DAT_00722ba2,
                 (uint)DAT_00722ba0,pcVar15);
      }
      _fprintf(_File,"%s %s",local_104,DAT_006e35c0);
      dwBytes = GetFileVersionInfoSizeA(local_104,&local_148);
      lpData = GlobalAlloc(0,dwBytes);
      GetFileVersionInfoA(local_104,0,dwBytes,lpData);
      VerQueryValueA(lpData,"\\",&local_14c,&local_148);
      _fprintf(_File,"   (Version=%d.%d.%d.%d)\n",*(uint *)((int)local_14c + 8) >> 0x10,
               *(uint *)((int)local_14c + 8) & 0xffff,*(uint *)((int)local_14c + 0xc) >> 0x10,
               *(uint *)((int)local_14c + 0xc) & 0xffff);
      GlobalFree(lpData);
      puVar11 = &DAT_00660f44;
      if (DAT_00719b48 != 1) {
        puVar11 = &DAT_0066b7b4;
      }
      dVar1 = (double)(int)DAT_00719b00;
      if ((int)DAT_00719b00 < 0) {
        dVar1 = dVar1 + 4294967296.0;
      }
      fVar3 = (float)DAT_00719b44;
      if (DAT_00719b44 < 0) {
        fVar3 = fVar3 + 4.2949673e+09;
      }
      local_14c = DAT_00719afc;
      dVar2 = (double)(int)DAT_00719afc;
      if ((int)DAT_00719afc < 0) {
        dVar2 = dVar2 + 4294967296.0;
      }
      _fprintf(_File,
               "Frames=%d\nTotal Time=%.2fs\nAverage frame rate=%.2ffps\nBelow  5fps=% 2d%% (time)  %d%% (frames) (%.3fs spent in %d frame%s\nBelow 10fps=% 2d%% (time)  %d%% (frames)\nBelow 15fps=% 2d%% (time)  %d%% (frames)\nBelow 20fps=% 2d%% (time)  %d%% (frames)\nBelow 25fps=% 2d%% (time)  %d%% (frames)\nBelow 30fps=% 2d%% (time)  %d%% (frames)\nBelow 40fps=% 2d%% (time)  %d%% (frames)\nBelow 50fps=% 2d%% (time)  %d%% (frames)\nBelow 60fps=% 2d%% (time)  %d%% (frames)\n"
               ,DAT_00719afc,dVar1 * 0.001,1000.0 / (dVar1 / dVar2),
               (uint)(DAT_00719b44 * 100) / DAT_00719b00,
               (uint)(DAT_00719b48 * 100) / (uint)DAT_00719afc,(double)(fVar3 * 0.001),DAT_00719b48,
               puVar11,(uint)(DAT_00719b3c * 100) / DAT_00719b00,
               (uint)(DAT_00719b40 * 100) / (uint)DAT_00719afc,
               (uint)(DAT_00719b34 * 100) / DAT_00719b00,
               (uint)(DAT_00719b38 * 100) / (uint)DAT_00719afc,
               (uint)(DAT_00719b2c * 100) / DAT_00719b00,
               (uint)(DAT_00719b30 * 100) / (uint)DAT_00719afc,
               (uint)(DAT_00719b24 * 100) / DAT_00719b00,
               (uint)(DAT_00719b28 * 100) / (uint)DAT_00719afc,
               (uint)(DAT_00719b1c * 100) / DAT_00719b00,
               (uint)(DAT_00719b20 * 100) / (uint)DAT_00719afc,
               (uint)(DAT_00719b14 * 100) / DAT_00719b00,
               (uint)(DAT_00719b18 * 100) / (uint)DAT_00719afc,
               (uint)(DAT_00719b0c * 100) / DAT_00719b00,
               (uint)(DAT_00719b10 * 100) / (uint)DAT_00719afc,
               (uint)(DAT_00719b04 * 100) / DAT_00719b00,
               (uint)(DAT_00719b08 * 100) / (uint)DAT_00719afc);
      cVar5 = DAT_00746121;
      if (DAT_007196e4 != 0) {
        _fprintf(_File,"###Sound Options###\nSound Disabled\n");
        goto LAB_004c7530;
      }
      if ((short)_DAT_007252b8 == 2) {
        pcVar15 = &DAT_0066b5e4;
      }
      else {
        pcVar15 = "Medium";
        if ((short)_DAT_007252b8 != 1) {
          pcVar15 = (char *)&PTR_DAT_0066b5d8;
        }
      }
      if (DAT_00746121 == '\0') {
LAB_004c74da:
        puVar12 = &DAT_0066b5d0;
      }
      else {
        uVar14 = FUN_00551620();
        pcVar15 = (char *)((ulonglong)uVar14 >> 0x20);
        puVar12 = &DAT_0066b5d4;
        if ((int)uVar14 == 0) goto LAB_004c74da;
      }
      if (DAT_00746128 == 2) {
        pcVar8 = &DAT_0066b5e4;
      }
      else {
        pcVar8 = "Normal";
        if (DAT_00746128 != 1) {
          pcVar8 = (char *)&PTR_DAT_0066b5d8;
        }
      }
      ppuVar6 = &PTR_DAT_0066b5c4;
      if (cVar5 != '\x01') {
        ppuVar6 = (undefined **)&DAT_0066b5d0;
      }
      _fprintf(_File,
               "###Sound Options###\nHardware Acceleration= %s\nSound Quality= %s\nEnvironmental Sound= %s\nSound Variety= %s\n"
               ,ppuVar6,pcVar8,puVar12,pcVar15);
LAB_004c7530:
      if (DAT_0068944e == 0) {
        pcVar15 = &DAT_0066b5e4;
      }
      else {
        pcVar15 = "Medium";
        if (DAT_0068944e != 1) {
          pcVar15 = (char *)&PTR_DAT_0066b5d8;
        }
      }
      if (DAT_0068944c == 0) {
        ppuVar6 = (undefined **)&DAT_0066b5e4;
      }
      else {
        ppuVar6 = &PTR_DAT_0066b5d8;
        if (DAT_0068944c != 1) {
          ppuVar6 = &PTR_s_render_vector_avoidance_intermed_0066664c_3_0066b530;
        }
      }
      ppuVar10 = &PTR_DAT_0066b5c4;
      if (DAT_006893f5 == '\0') {
        ppuVar10 = (undefined **)&DAT_0066b5d0;
      }
      ppuVar9 = &PTR_DAT_0066b5c4;
      if (DAT_006893f2 == '\0') {
        ppuVar9 = (undefined **)&DAT_0066b5d0;
      }
      ppuVar7 = &PTR_DAT_0066b5c4;
      if (DAT_006893fa == '\0') {
        ppuVar7 = (undefined **)&DAT_0066b5d0;
      }
      _fprintf(_File,
               "###Video Options###\nResolution= %d x %d\nRefresh rate= %d Hz\nFramerate throttle= No Vsync\nSpecular= %s\nShadows= %s\nDecals= %s\nParticles= %s\nTexture Quality= %s\n\nFor further information, please visit the timedemo FAQ at: http://halo.bungie.net/site/halo/features/hpcperformancefaq.html \n"
               ,DAT_007c04a0,DAT_007c04a4,DAT_007c11f8,ppuVar7,ppuVar9,ppuVar10,ppuVar6,pcVar15);
      _fclose(_File);
      DAT_007196d8 = DAT_007196d8 + 1;
      return;
    }
    pcVar15 = "map_name d20";
  }
  hs_compile_and_evaluate(pcVar15);
LAB_004c75fd:
  DAT_007196d8 = DAT_007196d8 + 1;
  return;
}

Disassembly notes (0x4c6f30..0x4c760a):
  4c6f4e  cdq ; the 64 bit counter is compared with the sign extended low dword
  4c6fa9  cmp ecx,0x10 ; jbe                   unsigned bucket thresholds
  4c70e8  jg / je on 0x898, then 0x64, 0x44c; 0xc4e, 0x125c above
  4c74c2  the Sound Variety string lives in EDX across the call to 0x551620 (the callee
          touches only EAX), which is why Ghidra shows it as the high half of the result
  4c750e  push quality_variety(edx), env(esi), sound_quality(ecx), hw(eax): the report prints
          0x00746128 as Sound Quality and 0x007252b8 as Sound Variety
#endif
