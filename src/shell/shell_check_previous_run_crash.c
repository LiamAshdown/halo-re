// shell_check_previous_run_crash  (Ghidra: chimera__registry_check_2; renamed per
//   out/phase4/shell_types_notes.md: "this is a previous-run crash check. It tests for a .pdb
//   beside the path held at 0x006a32e8, reads ExitFlag and rewrites 'bad 1'/'bad 2' state. It is
//   not a Chimera hook.")
// address 0x57e850, size 439 bytes
// name confidence: 0.55  rewrite confidence: 0.8
// evidence: Reads the registry ExitFlag value; if a same-named .pdb sits next to the module path
//   held at 0x006a32e8 (a debug build), does nothing and returns 0. Otherwise: if ExitFlag is
//   already "bad 2", returns 1 without touching the registry; if it is exactly "bad 1", escalates
//   it to "bad 2"; anything else (including "clean" from shell_registry_set_exit_flag_clean, or
//   no value at all) sets it to "bad 1". "bad 1" / "bad 2" at 0x006729c4 / 0x006729cc confirmed by
//   objdump byte dump (both 6-byte C strings, padded to 8).
// register convention: plain __cdecl, no parameters (objdump: no register reads before writes).
// blam-cc: (no arguments)
// UNSURE: the exact owner/meaning of the path global at 0x006a32e8 (module file name, presumably
//   set by shell_winmain via GetModuleFileNameA, but that address is outside this module's own
//   .bss clusters).

#include "win32.h"
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "rasterizer.h"
#include "shell.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif


extern char *shell_module_path; // 0x006a32e8, UNSURE: see file header

// Byte-for-byte equality over `length` bytes (the original computes a full 3-way lexicographic
// compare here, like memcmp, but every caller only tests the result for equality).
static uint8_t bytes_equal(const uint8_t *a, const uint8_t *b, uint32_t length)
{
    uint32_t i;
    for (i = 0; i < length; i++) {
        if (a[i] != b[i]) {
            return 0;
        }
    }
    return 1;
}

// Checks whether the previous run left the registry ExitFlag marked "bad 1" (one crash) or
// "bad 2" (two or more consecutive crashes), skipping the check entirely when a .pdb sits next
// to the running module (a debug build). Escalates "bad 1" to "bad 2", and anything else
// (including "clean" or an unreadable value) to "bad 1". Returns 1 if the flag was already
// "bad 2" going in (nothing rewritten), otherwise 0.
int32_t shell_check_previous_run_crash(void)
{
    void *key;
    uint32_t exit_flag_size;
    uint8_t exit_flag[16];
    char pdb_path[264];
    char *end;
    uint32_t length;

    RegOpenKeyExA((HKEY)0x80000001 /* HKEY_CURRENT_USER */, "Software\\Microsoft\\Microsoft Games\\Halo", 0,
                  0x20019, (PHKEY)&key);
    exit_flag_size = 0x10;
    exit_flag[0] = 0;
    RegQueryValueExA((HKEY)key, "ExitFlag", 0, 0, exit_flag, (LPDWORD)&exit_flag_size);
    RegCloseKey((HKEY)key);

    length = 0;
    while (shell_module_path[length] != 0) {
        pdb_path[length] = shell_module_path[length];
        length++;
    }
    pdb_path[length] = 0;

    end = pdb_path + length;
    if (length >= 4 && end[-4] == '.') { // the original has no length guard (0x57e8d0 reads [len - 4])
        end[-3] = 'p';
        end[-2] = 'd';
        end[-1] = 'b';
    }

    if (GetFileAttributesA(pdb_path) != 0xffffffff) {
        return 0; // a matching .pdb exists next to the module: debug build, skip the check
    }

    if (bytes_equal(exit_flag, (const uint8_t *)"bad 2", 6)) {
        return 1;
    }

    RegCreateKeyExA((HKEY)0x80000001 /* HKEY_CURRENT_USER */, "Software\\Microsoft\\Microsoft Games\\Halo", 0, 0,
                     0x20006, 0, 0, (PHKEY)&key, 0);
    if (bytes_equal(exit_flag, (const uint8_t *)"bad 1", 6)) {
        RegSetValueExA((HKEY)key, "ExitFlag", 0, 1 /* REG_SZ */, (const uint8_t *)"bad 2", 6);
    } else {
        RegSetValueExA((HKEY)key, "ExitFlag", 0, 1 /* REG_SZ */, (const uint8_t *)"bad 1", 6);
    }
    RegCloseKey((HKEY)key);

    return 0;
}

#if 0
Original Ghidra decompilation (0x57e850):

undefined4 chimera__registry_check_2(void)

{
  char cVar1;
  char *pcVar2;
  DWORD DVar3;
  int iVar4;
  char *pcVar5;
  int iVar6;
  byte *pbVar7;
  byte *pbVar8;
  bool bVar9;
  bool bVar10;
  BYTE *lpData;
  HKEY local_11c;
  DWORD local_118;
  byte local_114 [12];
  char acStack_108 [264];

  RegOpenKeyExA((HKEY)&DAT_80000001,"Software\\Microsoft\\Microsoft Games\\Halo",0,0x20019,
                &local_11c);
  local_118 = 0x10;
  local_114[0] = 0;
  RegQueryValueExA(local_11c,"ExitFlag",(LPDWORD)0x0,(LPDWORD)0x0,local_114,&local_118);
  RegCloseKey(local_11c);
  pcVar2 = DAT_006a32e8;
  do {
    cVar1 = *pcVar2;
    pcVar2[(int)(acStack_108 + (4 - (int)DAT_006a32e8))] = cVar1;
    pcVar2 = pcVar2 + 1;
  } while (cVar1 != '\0');
  pcVar2 = acStack_108 + 4;
  do {
    pcVar5 = pcVar2;
    pcVar2 = pcVar5 + 1;
  } while (*pcVar5 != '\0');
  if (pcVar5[-4] == '.') {
    pcVar2 = acStack_108 + 4;
    do {
      pcVar5 = pcVar2;
      pcVar2 = pcVar5 + 1;
    } while (*pcVar5 != '\0');
    pcVar5[-3] = 'p';
    pcVar2 = acStack_108 + 4;
    do {
      pcVar5 = pcVar2;
      pcVar2 = pcVar5 + 1;
    } while (*pcVar5 != '\0');
    pcVar5[-2] = 'd';
    pcVar2 = acStack_108 + 4;
    do {
      pcVar5 = pcVar2;
      pcVar2 = pcVar5 + 1;
    } while (*pcVar5 != '\0');
    pcVar5[-1] = 'b';
  }
  DVar3 = GetFileAttributesA(acStack_108 + 4);
  if (DVar3 != 0xffffffff) {
    return 0;
  }
  iVar6 = 6;
  bVar9 = false;
  iVar4 = 0;
  bVar10 = true;
  pbVar7 = local_114;
  pbVar8 = &DAT_006729cc;
  do {
    if (iVar6 == 0) break;
    iVar6 = iVar6 + -1;
    bVar9 = *pbVar7 < *pbVar8;
    bVar10 = *pbVar7 == *pbVar8;
    pbVar7 = pbVar7 + 1;
    pbVar8 = pbVar8 + 1;
  } while (bVar10);
  if (!bVar10) {
    iVar4 = (1 - (uint)bVar9) - (uint)(bVar9 != 0);
  }
  if (iVar4 == 0) {
    return 1;
  }
  iVar6 = 6;
  bVar9 = false;
  iVar4 = 0;
  bVar10 = true;
  pbVar7 = local_114;
  pbVar8 = &DAT_006729c4;
  do {
    if (iVar6 == 0) break;
    iVar6 = iVar6 + -1;
    bVar9 = *pbVar7 < *pbVar8;
    bVar10 = *pbVar7 == *pbVar8;
    pbVar7 = pbVar7 + 1;
    pbVar8 = pbVar8 + 1;
  } while (bVar10);
  if (!bVar10) {
    iVar4 = (1 - (uint)bVar9) - (uint)(bVar9 != 0);
  }
  if (iVar4 == 0) {
    RegCreateKeyExA((HKEY)&DAT_80000001,"Software\\Microsoft\\Microsoft Games\\Halo",0,(LPSTR)0x0,0,
                    0x20006,(LPSECURITY_ATTRIBUTES)0x0,&local_11c,(LPDWORD)0x0);
    pcVar2 = "bad 2";
    do {
      pcVar5 = pcVar2;
      pcVar2 = pcVar5 + 1;
    } while (*pcVar5 != '\0');
    pcVar5 = pcVar5 + -0x6729cb;
    lpData = "bad 2";
  }
  else {
    RegCreateKeyExA((HKEY)&DAT_80000001,"Software\\Microsoft\\Microsoft Games\\Halo",0,(LPSTR)0x0,0,
                    0x20006,(LPSECURITY_ATTRIBUTES)0x0,&local_11c,(LPDWORD)0x0);
    pcVar2 = "bad 1";
    do {
      pcVar5 = pcVar2;
      pcVar2 = pcVar5 + 1;
    } while (*pcVar5 != '\0');
    pcVar5 = pcVar5 + -0x6729c3;
    lpData = "bad 1";
  }
  RegSetValueExA(local_11c,"ExitFlag",0,1,lpData,(DWORD)pcVar5);
  RegCloseKey(local_11c);
  return 0;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
