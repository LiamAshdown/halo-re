// autopatch_launch_updater  (Ghidra: autopatch_launch_updater, already named)
// address 0x577310, size 945 bytes (0x577310..0x5776c0, `ret`s at 0x5776a4 and 0x5776c0)
// name confidence: 0.75   rewrite confidence: 0.7
// evidence: objdump -d -M intel 0x577310..0x5776c1; every file_reference_* / path_* callee takes
//   register arguments Ghidra dropped, recovered here from the instruction right before each
//   call and matched against the callee files in src/saved_games:
//     file_reference_create   EAX = &config            (0x577363)
//     file_reference_open     ESI = &config, stack 2   (0x577374..0x57737a, 2 = _file_open_write)
//     file_reference_seek     EAX = 0, ECX = &config   (0x57738a..0x57738e)
//     path_remove_last_component EBX = path; path_append_component ESI = path, EBX = component
//     path_build_full         EAX = module_ref.path, EDX = full_path, CX = module_ref.location
//     path_split_components   EBX = &directory, ESI = full_path, EDI = &file_name, stack
//                             (&path_start, &extension, module_ref.flags & 1)  (0x57744e..0x577477)
//     path_append_extension   ESI = module_path, EBX = extension
//     file_reference_write    EDX = &config, ECX = line, ESI = strlen(line)
//     file_reference_close / file_reference_delete  ESI = &config
//   Strings: 0x67219c "currentupdate.cfg", 0x672190 "gamemode 1\n", 0x672184 "url \"%s\"\n",
//   0x672170 "updateversion \"%s\"\n", 0x672158 "gamecommand \"%s %s\"\n", 0x672148
//   "%s waitprocessid=%d", 0x672134 "haloupdate.exe". The two file_reference_record set-ups
//   (signature 'filo', location -1, the dead `flags & 1` test, append the name, set bit 0) are the
//   same inlined constructor src/game/savegame_index_*.c document.
//   On success it writes main_globals (types/main.h, base 0x00719700): +0x57
//   return_to_main_menu = 0 and +0x5b quit = 1, plus 0x007196d4 = 1, i.e. the game quits so the
//   updater can replace it. On a CreateProcess failure it deletes the config and sets
//   autopatch_update_check_state (0x0069fe04) to 4.
//   Called only from 0x4a4190, a two-instruction UI handler (call; mov al,1; ret).
// The file_name/extension out-parameters: path_split_components with split_extension set stores
//   the text after the last '.' through its fifth argument and the file name after the last '\'
//   through EDI (src/saved_games/path_split_components.c calls them ext_start_out and
//   ext_fallback_out), so module_path ends up as the bare executable name, e.g. "halo.exe".
// UNSURE: each line is NUL-terminated at line[0x400] (`mov BYTE PTR [esp+0x7a0],0` against a
//   0x400-byte _snprintf limit on the buffer at esp+0x3a0), one byte past a 0x400 buffer; the
//   frame has room for it (the next local starts at esp+0x7a8), so the buffer is declared 0x401
//   bytes here. 0x007196d4's owner is unresolved (src names it movie_playback_abort).
// NOTE: the PROCESS_INFORMATION handles are never closed; kept as the original does.
// register convention: no parameters; returns a bool in AL.
//   // blam-cc: none

#include "crt.h"
#include "win32.h"
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"
#include "saved_games.h"
#include "main.h"
#include "fn_networking.h"

typedef struct win32_process_information { // Win32 PROCESS_INFORMATION
    void *process;
    void *thread;
    uint32_t process_id;
    uint32_t thread_id;
} win32_process_information;

typedef struct win32_startupinfo {         // Win32 STARTUPINFOA, 0x44 bytes on x86
    uint32_t cb;
    uint8_t unknown_04[0x40];
} win32_startupinfo;

extern char autopatch_update_url[0x100];     // 0x007228d8, UNSURE name
extern char autopatch_update_version[0x100]; // 0x007229d8, UNSURE name
extern char *shell_command_line;             // 0x006e35c0
extern main_globals main_globals_data;       // 0x00719700
extern int32_t movie_playback_abort;         // 0x007196d4, UNSURE owner
extern int32_t autopatch_update_check_state; // 0x0069fe04

extern uint8_t file_reference_create(file_reference_record *ref);                       // 0x5555b0, blam-cc: EAX ref
extern uint8_t file_reference_open(file_reference_record *ref, uint8_t mode);           // 0x5557a0, blam-cc: ESI ref, stack mode
extern uint8_t file_reference_seek(int32_t offset, file_reference_record *ref);         // 0x5558f0, blam-cc: EAX offset, ECX ref
extern uint8_t file_reference_write(file_reference_record *ref, const void *buffer, uint32_t size); // 0x555a90, blam-cc: EDX ref, ECX buffer, ESI size
extern uint8_t file_reference_close(file_reference_record *ref);                        // 0x555890, blam-cc: ESI ref
extern uint8_t file_reference_delete(file_reference_record *ref);                       // 0x555670, blam-cc: ESI ref
extern void path_remove_last_component(char *path);                                     // 0x555f80, blam-cc: EBX path
extern void path_append_component(char *destination, const char *component);            // 0x555ec0, blam-cc: ESI destination, EBX component
extern void path_append_extension(char *destination, const char *suffix);               // 0x555f20, blam-cc: ESI destination, EBX suffix
extern void path_build_full(char *source, char *destination, int16_t location);         // 0x5560d0, blam-cc: EAX source, EDX destination, CX location
extern void path_split_components(char **dir_start_out, char *path, char **ext_fallback_out,
    char **name_end_out, char **ext_start_out, uint8_t split_extension);                // 0x556000, blam-cc: EBX, ESI, EDI, then stack

static uint32_t autopatch_string_length(const char *string)
{
    const char *end = string;
    while (*end != 0) {
        end++;
    }
    return (uint32_t)(end - string);
}

// Writes currentupdate.cfg (game mode, update URL, target version and the command line that
// relaunches the game), starts haloupdate.exe with this process's id, and asks the main loop to
// quit. Returns true once the updater process has started.
uint8_t autopatch_launch_updater(void)
{
    file_reference_record config;       // esp+0x030
    file_reference_record module_ref;   // esp+0x188
    char module_path[0x105];            // esp+0x298
    char line[0x401];                   // esp+0x3a0, see the UNSURE note on line[0x400]
    char full_path[0x100];              // esp+0x7a8
    char *extension;                    // esp+0x010
    char *file_name;                    // esp+0x014
    char *path_start;                   // esp+0x028
    char *directory;                    // esp+0x02c
    win32_process_information process;  // esp+0x018
    win32_startupinfo startup;          // esp+0x140
    uint32_t i;

    for (i = 0; i < sizeof(config); i++) {
        ((uint8_t *)&config)[i] = 0;
    }
    config.signature = k_file_reference_signature;
    config.location = -1;
    if (config.flags & _file_reference_is_file_bit) {
        path_remove_last_component(config.path);
    }
    path_append_component(config.path, "currentupdate.cfg");
    config.flags |= _file_reference_is_file_bit;

    if (!file_reference_create(&config) || !file_reference_open(&config, _file_open_write) ||
        !file_reference_seek(0, &config)) {
        return 0;
    }

    for (i = 0; i < sizeof(module_path); i++) {
        module_path[i] = 0;
    }
    if ((int32_t)GetModuleFileNameA(0, module_path, 0x104) <= 0) {
        return 0;
    }

    for (i = 0; i < sizeof(module_ref); i++) {
        ((uint8_t *)&module_ref)[i] = 0;
    }
    module_ref.signature = k_file_reference_signature;
    module_ref.location = -1;
    if (module_ref.flags & _file_reference_is_file_bit) {
        path_remove_last_component(module_ref.path);
    }
    path_append_component(module_ref.path, module_path);
    module_ref.flags |= _file_reference_is_file_bit;

    for (i = 0; i < sizeof(full_path); i++) {
        full_path[i] = 0;
    }
    path_build_full(module_ref.path, full_path, module_ref.location);
    path_split_components(&directory, full_path, &file_name, &path_start, &extension,
        (uint8_t)(module_ref.flags & _file_reference_is_file_bit));
    module_path[0] = 0;
    path_append_component(module_path, file_name);
    path_append_extension(module_path, extension);

    _snprintf(line, 0x400, "gamemode 1\n");
    line[0x400] = 0;
    if (!file_reference_write(&config, line, autopatch_string_length(line))) {
        return 0;
    }
    _snprintf(line, 0x400, "url \"%s\"\n", autopatch_update_url);
    line[0x400] = 0;
    if (!file_reference_write(&config, line, autopatch_string_length(line))) {
        return 0;
    }
    _snprintf(line, 0x400, "updateversion \"%s\"\n", autopatch_update_version);
    line[0x400] = 0;
    if (!file_reference_write(&config, line, autopatch_string_length(line))) {
        return 0;
    }
    _snprintf(line, 0x400, "gamecommand \"%s %s\"\n", module_path, shell_command_line);
    line[0x400] = 0;
    if (!file_reference_write(&config, line, autopatch_string_length(line))) {
        return 0;
    }
    if (!file_reference_close(&config)) {
        return 0;
    }

    process.process = 0;
    process.thread = 0;
    process.process_id = 0;
    process.thread_id = 0;
    for (i = 0; i < sizeof(startup); i++) {
        ((uint8_t *)&startup)[i] = 0;
    }
    startup.cb = 0x44;
    sprintf(line, "%s waitprocessid=%d", "haloupdate.exe", GetCurrentProcessId());
    // 0x4000020 = CREATE_DEFAULT_ERROR_MODE | NORMAL_PRIORITY_CLASS
    if (CreateProcessA(0, line, 0, 0, 0, 0x4000020, 0, 0, (LPSTARTUPINFOA)&startup, (LPPROCESS_INFORMATION)&process)) {
        main_globals_data.return_to_main_menu = 0;
        main_globals_data.quit = 1;
        movie_playback_abort = 1;
        return 1;
    }

    file_reference_delete(&config);
    autopatch_update_check_state = 4;
    return 0;
}

#if 0
Original Ghidra decompilation (0x577310):

uint autopatch_launch_updater(void)

{
  char cVar1;
  uint uVar2;
  char *pcVar3;
  DWORD DVar4;
  BOOL BVar5;
  int iVar6;
  CHAR *pCVar7;
  undefined4 *puVar8;
  _STARTUPINFOA *p_Var9;
  undefined1 local_8a0 [8];
  _PROCESS_INFORMATION local_898;
  undefined1 local_888 [8];
  undefined4 local_880;
  ushort local_87c;
  undefined2 local_87a;
  _STARTUPINFOA local_770;
  undefined4 local_728;
  byte local_724;
  undefined2 local_722;
  CHAR local_618 [264];
  char local_510 [1024];
  undefined1 local_110;
  undefined1 local_108;
  undefined4 local_107;

  puVar8 = &local_880;
  for (iVar6 = 0x43; iVar6 != 0; iVar6 = iVar6 + -1) {
    *puVar8 = 0;
    puVar8 = puVar8 + 1;
  }
  local_880 = 0x66696c6f;
  local_87a = 0xffff;
  if ((local_87c & 1) != 0) {
    path_remove_last_component();
  }
  path_append_component();
  local_87c = local_87c | 1;
  uVar2 = file_reference_create();
  if ((char)uVar2 != '\0') {
    uVar2 = file_reference_open(2);
    if ((char)uVar2 != '\0') {
      uVar2 = file_reference_seek();
      if ((char)uVar2 != '\0') {
        pCVar7 = local_618;
        for (iVar6 = 0x41; iVar6 != 0; iVar6 = iVar6 + -1) {
          pCVar7[0] = '\0';
          pCVar7[1] = '\0';
          pCVar7[2] = '\0';
          pCVar7[3] = '\0';
          pCVar7 = pCVar7 + 4;
        }
        *pCVar7 = '\0';
        uVar2 = GetModuleFileNameA((HMODULE)0x0,local_618,0x104);
        if (0 < (int)uVar2) {
          puVar8 = &local_728;
          for (iVar6 = 0x43; iVar6 != 0; iVar6 = iVar6 + -1) {
            *puVar8 = 0;
            puVar8 = puVar8 + 1;
          }
          local_728 = 0x66696c6f;
          local_722 = 0xffff;
          if ((local_724 & 1) != 0) {
            path_remove_last_component();
          }
          path_append_component();
          local_724 = local_724 | 1;
          local_108 = 0;
          puVar8 = &local_107;
          for (iVar6 = 0x3f; iVar6 != 0; iVar6 = iVar6 + -1) {
            *puVar8 = 0;
            puVar8 = puVar8 + 1;
          }
          *(undefined2 *)puVar8 = 0;
          *(undefined1 *)((int)puVar8 + 2) = 0;
          path_build_full();
          path_split_components(local_888,local_8a0,local_724 & 1);
          local_618[0] = '\0';
          path_append_component();
          path_append_extension();
          __snprintf(local_510,0x400,"gamemode 1\n");
          pcVar3 = local_510;
          local_110 = 0;
          do {
            cVar1 = *pcVar3;
            pcVar3 = pcVar3 + 1;
          } while (cVar1 != '\0');
          uVar2 = file_reference_write();
          if ((char)uVar2 != '\0') {
            __snprintf(local_510,0x400,"url \"%s\"\n",&DAT_007228d8);
            pcVar3 = local_510;
            local_110 = 0;
            do {
              cVar1 = *pcVar3;
              pcVar3 = pcVar3 + 1;
            } while (cVar1 != '\0');
            uVar2 = file_reference_write();
            if ((char)uVar2 != '\0') {
              __snprintf(local_510,0x400,"updateversion \"%s\"\n",&DAT_007229d8);
              pcVar3 = local_510;
              local_110 = 0;
              do {
                cVar1 = *pcVar3;
                pcVar3 = pcVar3 + 1;
              } while (cVar1 != '\0');
              uVar2 = file_reference_write();
              if ((char)uVar2 != '\0') {
                __snprintf(local_510,0x400,"gamecommand \"%s %s\"\n",local_618,DAT_006e35c0);
                pcVar3 = local_510;
                local_110 = 0;
                do {
                  cVar1 = *pcVar3;
                  pcVar3 = pcVar3 + 1;
                } while (cVar1 != '\0');
                uVar2 = file_reference_write();
                if ((char)uVar2 != '\0') {
                  uVar2 = file_reference_close();
                  if ((char)uVar2 != '\0') {
                    local_898.hProcess = (HANDLE)0x0;
                    local_898.hThread = (HANDLE)0x0;
                    local_898.dwProcessId = 0;
                    local_898.dwThreadId = 0;
                    p_Var9 = &local_770;
                    for (iVar6 = 0x11; iVar6 != 0; iVar6 = iVar6 + -1) {
                      p_Var9->cb = 0;
                      p_Var9 = (_STARTUPINFOA *)&p_Var9->lpReserved;
                    }
                    local_770.cb = 0x44;
                    DVar4 = GetCurrentProcessId();
                    _sprintf(local_510,"%s waitprocessid=%d","haloupdate.exe",DVar4);
                    BVar5 = CreateProcessA((LPCSTR)0x0,local_510,(LPSECURITY_ATTRIBUTES)0x0,
                                           (LPSECURITY_ATTRIBUTES)0x0,0,0x4000020,(LPVOID)0x0,
                                           (LPCSTR)0x0,&local_770,&local_898);
                    if (BVar5 != 0) {
                      DAT_00719754._3_1_ = 0;
                      DAT_0071975b = 1;
                      DAT_007196d4 = 1;
                      return 1;
                    }
                    uVar2 = file_reference_delete();
                    DAT_0069fe04 = 4;
                  }
                }
              }
            }
          }
        }
      }
    }
  }
  return uVar2 & 0xffffff00;
}
#endif
