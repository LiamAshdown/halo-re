// engine_initialize_subsystems  (Ghidra: engine_initialize_subsystems, already named)
// address 0x540ee0, size 297 bytes
// name confidence: 0.6   rewrite confidence: 0.75
// evidence: the four LoadLibraryA/GetProcAddress pairs match the string literals
// ("d3d9.dll"/"Direct3DCreate9", "dsound.dll"/"DirectSoundCreate8", "dinput8.dll"/
// "DirectInput8Create", "shfolder.dll"/"SHGetFolderPathA") and the four cached FARPROC globals
// shell.h documents at 0x00746264/68/6c/70/74; the 0x41-dword-plus-one-byte zero fill at
// 0x006ac900 (0x540ef9..0x540f05) is the whole profile_directory[0x105] of types/cache.h
// (k_profile_directory_storage_size, types/cseries.h).
// register convention: __cdecl, no arguments.
// UNSURE: data_file_open, directory_create_recursive, profile_path_initialize,
// input_directinput_initialize, math_initialize, game_state_startup, sound_initialize and
// render_initialize belong to other modules; their prototypes below are inferred only from this call
// site's argument/return usage, not verified against their own definitions. DAT_0087ac00/01/04/05
// and DAT_0087ac08 are likewise owned by another module; typed here only from the byte/dword
// width Ghidra shows for this write. 0x0087ac06 is debug_log_level (uint8, R01).
// reconciled: R10 profile_directory is char[0x105] (k_profile_directory_storage_size; shell zeroes 0x41 dwords + 1 byte at 0x540ef9)
// reconciled: R01 0x0087ac06 console_verbosity_low -> debug_log_level (uint8)

#include "win32.h"
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "rasterizer.h"
#include "shell.h"
#include "fn_math.h"
#include "fn_cseries.h"
#include "fn_saved_games.h"
#include "fn_shell.h"

extern large_integer performance_frequency;   // 0x006ac8f8 QueryPerformanceFrequency result
extern char profile_directory[0x105];         // 0x006ac900 (types/cache.h); memset clears 0x105
                                               // bytes here, one past the declared array
extern void *d3d9_module;                     // 0x00746264
extern void *dsound_module;                   // 0x0074625c
extern void *dinput8_module;                  // 0x00746258
extern void *shfolder_module;                 // 0x00746260
extern void *direct3d_create9;                // 0x00746274 FARPROC
extern void *direct_sound_create8;            // 0x00746270 FARPROC
extern void *direct_input8_create;            // 0x00746268 FARPROC
extern void *sh_get_folder_path;              // 0x0074626c FARPROC
extern int32_t shell_nosound;                       // 0x007196e4 -shell_nosound (32 bit BOOL)
extern uint8_t sound_disabled;       // 0x007252b6 DAT_007252b6, copy of shell_nosound

// 0x0087ac06 is interface.h / networking.h debug_log_level (uint8, R01); this function only
// clears these.
extern uint8_t console_debug_flag_0;          // 0x0087ac00
extern uint8_t error_file_enabled;          // 0x0087ac01
extern uint8_t console_debug_flag_4;          // 0x0087ac04
extern uint8_t console_debug_flag_5;          // 0x0087ac05
extern uint8_t debug_log_level;               // 0x0087ac06 (R01; mov BYTE PTR ds:0x87ac06,bl at 0x540fac)
extern uint16_t console_debug_word_8;         // 0x0087ac08 (16 bit store, mov word [0x87ac08],bx at 0x540fcc)


extern uint8_t data_file_open(void);                    // 0x00442840
extern void directory_create_recursive(char *path);     // 0x00449250

extern void input_directinput_initialize(void);          // 0x00490520

extern uint32_t render_initialize(void);                       // 0x00511da0, module unknown

extern uint32_t sound_initialize(void);                    // 0x005492f0

// Top-level engine bring-up routine: sets timer resolution, resolves the D3D9/DirectSound/
// DirectInput/Shell entry points, and initializes the data-file, math, and (conditionally)
// sound subsystems, returning true only if game_state_startup's prerequisite check passes.
uint8_t engine_initialize_subsystems(void)
{
    int32_t i;
    uint32_t startup_ok;

    timeBeginPeriod(1);
    QueryPerformanceFrequency((LARGE_INTEGER *)&performance_frequency);

    for (i = 0; i < 0x105; i++) { // rep stosd x 0x41 + stosb
        profile_directory[i] = 0;
    }

    profile_path_initialize();

    if (direct3d_create9 == 0) {
        d3d9_module = LoadLibraryA("d3d9.dll");
        direct3d_create9 = GetProcAddress(d3d9_module, "Direct3DCreate9");

        if (shell_nosound == 0) {
            dsound_module = LoadLibraryA("dsound.dll");
            direct_sound_create8 = GetProcAddress(dsound_module, "DirectSoundCreate8");
        } else {
            dsound_module = 0;
            direct_sound_create8 = 0;
        }

        dinput8_module = LoadLibraryA("dinput8.dll");
        direct_input8_create = GetProcAddress(dinput8_module, "DirectInput8Create");

        shfolder_module = LoadLibraryA("shfolder.dll");
        sh_get_folder_path = GetProcAddress(shfolder_module, "SHGetFolderPathA");
    }

    directory_create_recursive(profile_directory);

    debug_log_level = 0;
    error_file_enabled = 1;
    console_debug_flag_4 = 1;
    console_debug_flag_5 = 0;
    console_debug_flag_0 = 0;
    console_debug_word_8 = 0;

    data_file_open();
    math_initialize();
    game_state_startup();

    startup_ok = render_initialize();
    if ((uint8_t)startup_ok != 0) {
        input_directinput_initialize();
        sound_disabled = (uint8_t)shell_nosound; // low byte only (mov al,[0x7196e4] at 0x540ff0)
        sound_initialize();
        return 1;
    }
    return (uint8_t)startup_ok;
}

#if 0
Original Ghidra decompilation (0x540ee0):


/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

int __cdecl engine_initialize_subsystems(void)

{
  uint uVar1;
  undefined4 uVar2;
  int iVar3;
  undefined4 *puVar4;
  
  timeBeginPeriod(1);
  QueryPerformanceFrequency((LARGE_INTEGER *)&DAT_006ac8f8);
  puVar4 = &DAT_006ac900;
  for (iVar3 = 0x41; iVar3 != 0; iVar3 = iVar3 + -1) {
    *puVar4 = 0;
    puVar4 = puVar4 + 1;
  }
  *(undefined1 *)puVar4 = 0;
  profile_path_initialize();
  if (DAT_00746274 == (FARPROC)0x0) {
    DAT_00746264 = LoadLibraryA("d3d9.dll");
    DAT_00746274 = GetProcAddress(DAT_00746264,"Direct3DCreate9");
    if (DAT_007196e4 == 0) {
      DAT_0074625c = LoadLibraryA("dsound.dll");
      DAT_00746270 = GetProcAddress(DAT_0074625c,"DirectSoundCreate8");
    }
    else {
      DAT_0074625c = (HMODULE)0x0;
      DAT_00746270 = (FARPROC)0x0;
    }
    DAT_00746258 = LoadLibraryA("dinput8.dll");
    DAT_00746268 = GetProcAddress(DAT_00746258,"DirectInput8Create");
    DAT_00746260 = LoadLibraryA("shfolder.dll");
    DAT_0074626c = GetProcAddress(DAT_00746260,"SHGetFolderPathA");
  }
  directory_create_recursive((char *)&DAT_006ac900);
  DAT_0087ac06 = 0;
  DAT_0087ac01 = 1;
  DAT_0087ac04 = 1;
  DAT_0087ac05 = 0;
  DAT_0087ac00 = 0;
  _DAT_0087ac08 = 0;
  data_file_open();
  math_initialize();
  game_state_startup();
  uVar1 = FUN_00511da0();
  if ((char)uVar1 != '\0') {
    input_directinput_initialize();
    DAT_007252b6 = (undefined1)DAT_007196e4;
    uVar2 = sound_initialize();
    return CONCAT31((int3)((uint)uVar2 >> 8),1);
  }
  return uVar1 & 0xffffff00;
}
#endif
