// registry_get_dist_id  (Ghidra: registry_get_dist_id, already named)
// address 0x577760, size 108 bytes
// name confidence: 0.7   rewrite confidence: 0.7
// evidence: strings "Software\\Microsoft\\Microsoft Games\\Halo", "DistID"; out/phase4/
// networking_functions.md summary.
// register convention: no register-passed arguments.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"

extern int32_t __stdcall RegOpenKeyExA(void *key, const char *subkey, uint32_t options, uint32_t sam,
                              void **result);
extern int32_t __stdcall RegQueryValueExA(void *key, const char *value_name, uint32_t *reserved, uint32_t *type,
                                 uint8_t *data, uint32_t *data_size);
extern int32_t __stdcall RegCloseKey(void *key);

// Reads the installed game's "DistID" (distribution/channel id) DWORD value from the registry.
// Returns 0 if the key or value could not be read.
uint32_t registry_get_dist_id(void)
{
    uint32_t dist_id = 0;
    uint32_t size = 4;
    void *key;

    if (RegOpenKeyExA((void *)0x80000002, "Software\\Microsoft\\Microsoft Games\\Halo", 0, 0x20019,
                       &key) == 0) {
        if (RegQueryValueExA(key, "DistID", 0, 0, (uint8_t *)&dist_id, &size) != 0) {
            dist_id = 0;
        }
        RegCloseKey(key);
    }
    return dist_id;
}

#if 0
Original Ghidra decompilation (0x577760):

undefined4 registry_get_dist_id(void)

{
  LSTATUS LVar1;
  undefined4 local_c;
  HKEY local_8;
  DWORD local_4;

  local_c = 0;
  local_4 = 4;
  LVar1 = RegOpenKeyExA((HKEY)&DAT_80000002,"Software\\Microsoft\\Microsoft Games\\Halo",0,0x20019,
                        &local_8);
  if (LVar1 == 0) {
    LVar1 = RegQueryValueExA(local_8,"DistID",(LPDWORD)0x0,(LPDWORD)0x0,(LPBYTE)&local_c,&local_4);
    if (LVar1 != 0) {
      local_c = 0;
    }
    RegCloseKey(local_8);
  }
  return local_c;
}
#endif
