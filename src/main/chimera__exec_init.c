// chimera__exec_init  (Ghidra: chimera__exec_init, already named)
// address 0x4c6390, size 140 bytes
// name confidence: 0.8   rewrite confidence: 0.8
// evidence: out/phase4/main_types_notes.md calls this the console/engine startup helper (CEA
// console_startup): it runs init.txt (or a file named after -exec's value) through
// console_exec_file_run, and additionally queues "map_name b30" when rasterizer_window_requested
// (0x0071d1a8, already named in src/rasterizer/rasterizer_initialize_direct3d.c) is set.
// disassembly (objdump -d -M intel, bin/halo.exe, 0x4c6390..0x4c641c) was read in full because
// Ghidra's decompile collapses the real control flow: it shows a bare
// `command_line_check_flag("-exec");` with the result unused, but the binary passes a second,
// register-carried out-value argument (EDI) and branches on both the boolean result and that
// out-value to choose between the caller-supplied exec file name and the default "init.txt",
// truncating the caller-supplied name to 0x7f (127) characters with strncpy first.
// register convention: __cdecl, no arguments.
// UNSURE: pairing "-exec" startup with rasterizer_window_requested (the -window flag) is exactly
// what the binary does (0x4c6400..0x4c640b); it reads oddly (a display-mode flag gating a debug
// console command) but nothing in this function or command_line_check_flag suggests 0x0071d1a8
// is anything other than the same global rasterizer.h already names.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"
#include "saved_games.h"
#include "main.h"
#include <string.h>
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern int32_t rasterizer_window_requested; // 0x0071d1a8, see src/rasterizer/rasterizer_initialize_direct3d.c

extern uint8_t command_line_check_flag(const char *flag_name, const char **out_value); // 0x542760, blam-cc: EDI -> out_value (2nd parameter)
extern uint8_t console_exec_file_run(const char *file_name); // this module, 0x4c6420, blam-cc: EAX -> file_name
extern char console_process_command(char *command_line, uint32_t context_flags); // this module, 0x4c6a80, blam-cc: EDI -> command_line, stack -> context_flags

// Runs the startup exec script named by "-exec <file>" (or init.txt when -exec has no value or
// was not given), truncating a caller-supplied name to 127 characters. If the script could not
// be run (the file did not exist) and the window was requested, queues "map_name b30" as a
// console command instead.
void chimera__exec_init(void)
{
    char exec_file_name[0x80];
    const char *exec_arg;
    uint8_t exec_flag_present;
    uint8_t ran_script;

    exec_flag_present = command_line_check_flag("-exec", &exec_arg); // EDI -> &exec_arg
    if (exec_flag_present && exec_arg != 0) {
        strncpy(exec_file_name, exec_arg, 0x7f);
    } else {
        strncpy(exec_file_name, "init.txt", 0x7f);
    }
    ran_script = console_exec_file_run(exec_file_name); // EAX -> exec_file_name
    if (!ran_script && rasterizer_window_requested != 0) {
        console_process_command((char *)"map_name b30", 0); // EDI -> "map_name b30"
    }
}

#if 0
Original Ghidra decompilation (0x4c6390):

/* WARNING: Removing unreachable block (ram,0x004c63bd) */

void __cdecl chimera__exec_init(void)

{
  int iVar1;

  command_line_check_flag("-exec");
  iVar1 = console_exec_file_run();
  if (((char)iVar1 == '\0') && (DAT_0071d1a8 != 0)) {
    console_process_command(0);
  }
  return;
}

Disassembly (0x4c6390..0x4c641c), the actual control flow the C above follows:

004c6390:  sub    esp,0x84
004c6396:  push   esi
004c6397:  push   edi
004c6398:  push   0x66b240              ; "-exec"
004c639d:  lea    edi,[esp+0xc]         ; &exec_arg
004c63a1:  mov    DWORD PTR [esp+0xc],0x0
004c63a9:  call   0x542760              ; command_line_check_flag("-exec", &exec_arg)
004c63ae:  mov    esi,DWORD PTR [esp+0xc]
004c63b2:  add    esp,0x4
004c63b5:  test   al,al
004c63b7:  je     0x4c63cd
004c63b9:  test   esi,esi
004c63bb:  je     0x4c63d1
004c63bd:  push   0x7f
004c63bf:  lea    eax,[esp+0x10]
004c63c3:  push   esi
004c63c4:  push   eax
004c63c5:  call   0x623a90              ; strncpy(exec_file_name, exec_arg, 0x7f)
004c63ca:  add    esp,0xc
004c63cd:  test   esi,esi
004c63cf:  jne    0x4c63ee
004c63d1:  mov    ecx,DWORD PTR ds:0x66b234   ; "init"
004c63d7:  mov    edx,DWORD PTR ds:0x66b238   ; ".txt"
004c63dd:  mov    al,ds:0x66b23c              ; 0
004c63e2:  mov    DWORD PTR [esp+0xc],ecx
004c63e6:  mov    DWORD PTR [esp+0x10],edx
004c63ea:  mov    BYTE PTR [esp+0x14],al
004c63ee:  lea    eax,[esp+0xc]         ; exec_file_name
004c63f2:  call   0x4c6420              ; console_exec_file_run(EAX = exec_file_name)
004c63f7:  test   al,al
004c63f9:  jne    0x4c6413
004c63fb:  mov    eax,ds:0x71d1a8
004c6400:  test   eax,eax
004c6402:  je     0x4c6413
004c6404:  push   0x0
004c6406:  mov    edi,0x66b224          ; "map_name b30"
004c640b:  call   0x4c6a80              ; console_process_command(EDI = "map_name b30", 0)
004c6410:  add    esp,0x4
004c6413:  pop    edi
004c6414:  pop    esi
004c6415:  add    esp,0x84
004c641b:  ret
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
