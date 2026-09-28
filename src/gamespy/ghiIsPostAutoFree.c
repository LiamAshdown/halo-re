// ghiIsPostAutoFree  (GameSpy SDK in halo.exe; no C existed)
// address 0x6220f0, size 8 bytes
// name confidence: 0.8   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x6220f0..0x6220f7: the post  auto-free flag.
// blam-cc: cdecl

#include "gamespy.h"

#include "ghttp.h"

int ghiIsPostAutoFree(GHIPost *post)
{
    return post->autoFree;
}
