// console_exec_file_run  (Ghidra: console_exec_file_run, already named)
// address 0x4c6420, size 144 bytes
// name confidence: 0.6   rewrite confidence: 0.75
// evidence: disassembly (objdump -d -M intel, bin/halo.exe, 0x4c6420..0x4c64b0). Ghidra's own
// decompile omits the file_name parameter (it shows `FUN_00624186()` and
// `console_exec_file_run(void)` with no arguments) and renders the loop as a simple
// fgets/while/strtok/console_process_command sequence; the disassembly confirms that is an
// accurate simplification of a compiler loop-rotation (the priming fgets before the loop and the
// duplicate fgets call at the bottom read into the same 200-byte buffer), and that the missing
// argument is EAX, the file name passed straight through to the fopen-shaped helper at 0x624186.
// register convention: EAX -> file_name (the only parameter; see chimera__exec_init's call with
// EAX pointed at its own local buffer).
// blam-cc: EAX -> file_name

#include "crt.h"
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"
#include "saved_games.h"
#include "main.h"
#include <stdio.h>
#include <string.h>
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

// fopen: <stdio.h>, resolved to the game CRT at 0x624186 // 0x624186, fopen-shaped CRT wrapper
extern char console_process_command(char *command_line, uint32_t context_flags); // this module, 0x4c6a80, blam-cc: EDI -> command_line, stack -> context_flags

// Runs every line of file_name as a console command (used for the startup "-exec" script). Each
// line is cut at the first \r, \n or \t (trailing line-ending / comment-style whitespace) before
// being handed to console_process_command with the "exec file" context bit set. Returns whether
// the file could be opened at all; a file that opens but is empty still returns true.
uint8_t console_exec_file_run(const char *file_name) // blam-cc: EAX -> file_name
{
    FILE *file;
    char line[k_console_exec_line_length];

    file = (FILE *)fopen(file_name, "r");
    if (file == 0) {
        return 0;
    }
    while (fgets(line, k_console_exec_line_length - 1, file) != 0) {
        strtok(line, "\r\n\t");
        console_process_command(line, k_console_context_exec_file); // EDI -> line
    }
    fclose(file);
    return 1;
}

#if 0
Original Ghidra decompilation (0x4c6420):

int __cdecl console_exec_file_run(void)

{
  FILE *_File;
  char *pcVar1;
  int iVar2;
  char local_c8 [200];

  _File = (FILE *)FUN_00624186();
  if (_File != (FILE *)0x0) {
    DAT_007196e8 = 1;
    pcVar1 = _fgets(local_c8,199,_File);
    while (pcVar1 != (char *)0x0) {
      _strtok(local_c8,"\r\n\t");
      console_process_command(0x2000);
      pcVar1 = _fgets(local_c8,199,_File);
    }
    iVar2 = _fclose(_File);
    return CONCAT31((int3)((uint)iVar2 >> 8),1);
  }
  return 0;
}

Disassembly (0x4c6420..0x4c64af):

004c6420:  sub    esp,0xc8
004c6426:  push   esi
004c6427:  push   0x660014              ; "r"
004c642c:  push   eax                   ; file_name
004c642d:  call   0x624186              ; fopen-shaped helper(file_name, "r")
004c6432:  mov    esi,eax
004c6434:  add    esp,0x8
004c6437:  test   esi,esi
004c6439:  je     0x4c64a6              ; return 0
004c643b:  push   esi
004c643c:  lea    ecx,[esp+0x8]         ; &line
004c6440:  push   0xc7                  ; 199
004c6445:  push   ecx
004c6446:  mov    DWORD PTR ds:0x7196e8,0x1
004c6450:  call   0x6254b6              ; fgets(&line, 199, file)   (priming read)
004c6455:  add    esp,0xc
004c6458:  test   eax,eax
004c645a:  je     0x4c6493              ; EOF on the first read: skip the loop, close, return 1
004c645c:  push   edi                   ; save caller's edi
004c645d:  lea    ecx,[ecx+0x0]         ; alignment padding
loop:
004c6460:  lea    edx,[esp+0x8]         ; &line
004c6464:  push   0x66b220              ; "\r\n\t"
004c6469:  push   edx
004c646a:  call   0x62553c              ; strtok(&line, "\r\n\t")
004c646f:  push   0x2000                ; k_console_context_exec_file
004c6474:  lea    edi,[esp+0x14]        ; &line
004c6478:  call   0x4c6a80              ; console_process_command(EDI = &line, 0x2000)
004c647d:  push   esi
004c647e:  mov    eax,edi               ; &line
004c6480:  push   0xc7
004c6485:  push   eax
004c6486:  call   0x6254b6              ; fgets(&line, 199, file)   (next-line read)
004c648b:  add    esp,0x18
004c648e:  test   eax,eax
004c6490:  jne    0x4c6460
004c6492:  pop    edi                   ; restore caller's edi
004c6493:  push   esi
004c6494:  call   0x6241e5              ; fclose(file)
004c6499:  add    esp,0x4
004c649c:  mov    al,0x1
004c649e:  pop    esi
004c649f:  add    esp,0xc8
004c64a5:  ret
004c64a6:  xor    al,al
004c64a8:  pop    esi
004c64a9:  add    esp,0xc8
004c64af:  ret
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
