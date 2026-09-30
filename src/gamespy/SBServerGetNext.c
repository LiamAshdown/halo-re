// SBServerGetNext  (GameSpy SDK in halo.exe; no C existed)
// address 0x617680, size 8 bytes
// name confidence: 0.8   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x617680..0x617687: the list link (+0x20).
// blam-cc: cdecl

#include "gamespy.h"
#include "fn_gamespy.h"

#define FIELD(object, offset, type) (*(type *)((char *)(object) + (offset)))

void *SBServerGetNext(void *server)
{
    return FIELD(server, 0x20, void *);
}
