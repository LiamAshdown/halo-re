// shell_registry_set_exit_flag_clean  (Ghidra: shell_registry_set_exit_flag_clean, already named)
// address 0x57ea10, size 95 bytes
// name confidence: 0.8   rewrite confidence: 0.9
// evidence: matches its own name and out/phase4/shell_functions.md summary: "Marks the Halo
//   registry ExitFlag value as 'clean', recording a normal (non-crashing) run." Writes the
//   REG_SZ "clean" to HKLM\Software\Microsoft\Microsoft Games\Halo\ExitFlag.
// register convention: plain __cdecl, no parameters.
// blam-cc: (no arguments)

#include "win32.h"
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "rasterizer.h"
#include "shell.h"


// Opens (creating if necessary) HKLM\Software\Microsoft\Microsoft Games\Halo and writes its
// ExitFlag REG_SZ value to "clean", recording that this run did not crash.
void shell_registry_set_exit_flag_clean(void)
{
    void *key;

    RegCreateKeyExA((HKEY)0x80000001 /* HKEY_CURRENT_USER */, "Software\\Microsoft\\Microsoft Games\\Halo", 0, 0,
                     0x20006, 0, 0, (PHKEY)&key, 0);
    RegSetValueExA((HKEY)key, "ExitFlag", 0, 1 /* REG_SZ */, (const uint8_t *)"clean", 6);
    RegCloseKey((HKEY)key);
}

#if 0
Original Ghidra decompilation (0x57ea10):

void __cdecl shell_registry_set_exit_flag_clean(void)

{
  char *pcVar1;
  char *pcVar2;
  HKEY local_4;

  RegCreateKeyExA((HKEY)&DAT_80000001,"Software\\Microsoft\\Microsoft Games\\Halo",0,(LPSTR)0x0,0,
                  0x20006,(LPSECURITY_ATTRIBUTES)0x0,&local_4,(LPDWORD)0x0);
  pcVar1 = "clean";
  do {
    pcVar2 = pcVar1;
    pcVar1 = pcVar2 + 1;
  } while (*pcVar2 != '\0');
  RegSetValueExA(local_4,"ExitFlag",0,1,(BYTE *)"clean",(DWORD)(pcVar2 + -0x6729bb));
  RegCloseKey(local_4);
  return;
}
#endif
