// network_server_password_set  (Ghidra: FUN_004e0910, unnamed)
// address 0x4e0910, size 28 bytes
// name confidence: 0.5   rewrite confidence: 0.6
// evidence: out/phase4/networking_functions.md: "Copies up to 8 wide characters into the
// connection/session object's password field at offset 0x9fc." Matches
// network_server_globals::password (9 uint16_t, forced NUL at 0xa0c).
// register convention: EAX = source (const wchar_t *), ESI = server (network_server_globals *).
// blam-cc: EAX -> source, ESI -> server

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include <wchar.h>

extern wchar_t *_wcsncpy(wchar_t *dest, const wchar_t *source, int32_t count);

// Copies up to 8 wide characters from `source` into `server`'s password field and forces a
// NUL terminator.
void network_server_password_set(const wchar_t *source, network_server_globals *server)
{
    _wcsncpy((wchar_t *)server->password, source, 8);
    server->password[8] = 0;
}

#if 0
Original Ghidra decompilation (0x4e0910):

void FUN_004e0910(void)

{
  wchar_t *in_EAX;
  int unaff_ESI;

  _wcsncpy((wchar_t *)(unaff_ESI + 0x9fc),in_EAX,8);
  *(undefined2 *)(unaff_ESI + 0xa0c) = 0;
  return;
}
#endif
