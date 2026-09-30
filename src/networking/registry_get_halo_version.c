// registry_get_halo_version  (Ghidra: registry_get_halo_version, already named)
// address 0x5776d0, size 134 bytes
// name confidence: 0.7   rewrite confidence: 0.7
// evidence: strings "Software\\Microsoft\\Microsoft Games\\Halo", "Version"; out/phase4/
// networking_functions.md summary.
// register convention: no register-passed arguments.

#include "win32.h"
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "fn_networking.h"

extern char registry_halo_version_buffer[0x40]; // 0x006ef968


// Reads the installed game's "Version" value from the Halo registry key and returns it as a
// string (empty if the key or value could not be read).
char *registry_get_halo_version(void)
{
    void *key;
    int32_t i;
    int32_t status;
    uint32_t size;

    for (i = 0; i < 0x40; i++) {
        registry_halo_version_buffer[i] = 0;
    }

    size = 0x3f;
    status = RegOpenKeyExA((void *)0x80000002, "Software\\Microsoft\\Microsoft Games\\Halo", 0,
                            0x20019, (PHKEY)&key);
    if (status != 0) {
        registry_halo_version_buffer[0] = 0;
        return registry_halo_version_buffer;
    }
    status = RegQueryValueExA(key, "Version", 0, 0, (uint8_t *)registry_halo_version_buffer, &size);
    if (status != 0) {
        registry_halo_version_buffer[0] = 0;
    }
    RegCloseKey(key);
    return registry_halo_version_buffer;
}

#if 0
Original Ghidra decompilation (0x5776d0):

undefined4 * registry_get_halo_version(void)

{
  LSTATUS LVar1;
  int iVar2;
  undefined4 *puVar3;
  HKEY local_8;
  DWORD local_4;

  puVar3 = &DAT_006ef968;
  for (iVar2 = 0x10; iVar2 != 0; iVar2 = iVar2 + -1) {
    *puVar3 = 0;
    puVar3 = puVar3 + 1;
  }
  local_4 = 0x3f;
  LVar1 = RegOpenKeyExA((HKEY)&DAT_80000002,"Software\\Microsoft\\Microsoft Games\\Halo",0,0x20019,
                        &local_8);
  if (LVar1 != 0) {
    DAT_006ef968._0_1_ = 0;
    return &DAT_006ef968;
  }
  LVar1 = RegQueryValueExA(local_8,"Version",(LPDWORD)0x0,(LPDWORD)0x0,(LPBYTE)&DAT_006ef968,
                           &local_4);
  if (LVar1 != 0) {
    DAT_006ef968._0_1_ = 0;
  }
  RegCloseKey(local_8);
  return &DAT_006ef968;
}
#endif
