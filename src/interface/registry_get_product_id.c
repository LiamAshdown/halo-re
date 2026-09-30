// registry_get_product_id  (Ghidra: registry_get_product_id, already named)
// address 0x4a8790, size 115 bytes
// name confidence: 0.55 (existing Ghidra name)   rewrite confidence: 0.85
// evidence: matches the given name and cc (__cdecl); string "Software\\Microsoft\\Microsoft
// Games\\Halo"; win32 RegOpenKeyExA/RegQueryValueExA/RegCloseKey.
// register convention: __cdecl, no parameters.

#include "win32.h"
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"
#include "fn_interface.h"

#define HKEY_LOCAL_MACHINE ((HKEY)0x80000002)
#define KEY_QUERY_VALUE 0x0001
#define KEY_WOW64_32KEY 0x0200
#define KEY_READ_32 0x00020019 // matches the literal 0x20019 used here


extern uint8_t product_id_read;      // 0x00719340, set once the registry lookup has run
extern uint32_t cached_product_id;   // 0x00719344, the 4 byte "PID" value

// Reads (once, cached thereafter) the game's "PID" registry value under
// HKLM\Software\Microsoft\Microsoft Games\Halo and returns a pointer to the cached 4 byte value.
void *registry_get_product_id(void)
{
    LSTATUS status;
    HKEY key;
    DWORD size;

    if (product_id_read == 0) {
        size = 0x20;
        product_id_read = 1;
        status = RegOpenKeyExA(HKEY_LOCAL_MACHINE, "Software\\Microsoft\\Microsoft Games\\Halo", 0,
                                KEY_READ_32, (PHKEY)&key);
        if (status == 0) {
            status = RegQueryValueExA(key, "PID", 0, 0, (uint8_t *)&cached_product_id, &size);
            if (status != 0) {
                cached_product_id = 0;
            }
            RegCloseKey(key);
        }
    }
    return &cached_product_id;
}

#if 0
Original Ghidra decompilation (0x4a8790):

void * __cdecl registry_get_product_id(void)

{
  LSTATUS LVar1;
  HKEY local_8;
  DWORD local_4;

  if (DAT_00719340 == '\0') {
    local_4 = 0x20;
    DAT_00719340 = '\x01';
    LVar1 = RegOpenKeyExA((HKEY)&DAT_80000002,"Software\\Microsoft\\Microsoft Games\\Halo",0,0x20019
                          ,&local_8);
    if (LVar1 == 0) {
      LVar1 = RegQueryValueExA(local_8,"PID",(LPDWORD)0x0,(LPDWORD)0x0,&DAT_00719344,&local_4);
      if (LVar1 != 0) {
        DAT_00719344 = 0;
      }
      RegCloseKey(local_8);
    }
  }
  return &DAT_00719344;
}
#endif
