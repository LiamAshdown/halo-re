// network_server_password_get  (Ghidra: FUN_004e0930, unnamed)
// address 0x4e0930, size 24 bytes
// name confidence: 0.5   rewrite confidence: 0.6
// evidence: out/phase4/networking_functions.md: "Copies the session's stored password
// (offset 0x9fc) out into the caller-supplied wide-character buffer."
// register convention: EAX = server (network_server_globals *), ESI = dest (wchar_t *).
// blam-cc: EAX -> server, ESI -> dest

#include "crt.h"
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "fn_networking.h"
#include <wchar.h>


// Copies `server`'s password (up to 8 wide characters) into `dest` and NUL-terminates it.
void network_server_password_get(network_server_globals *server, wchar_t *dest)
{
    wcsncpy(dest, (wchar_t *)server->password, 8);
    dest[8] = 0;
}

#if 0
Original Ghidra decompilation (0x4e0930):

void FUN_004e0930(void)

{
  int in_EAX;
  wchar_t *unaff_ESI;

  _wcsncpy(unaff_ESI,(wchar_t *)(in_EAX + 0x9fc),8);
  unaff_ESI[8] = L'\0';
  return;
}
#endif
