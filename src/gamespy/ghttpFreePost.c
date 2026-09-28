// ghttpFreePost  (GameSpy SDK in halo.exe; no C existed)
// address 0x622100, size 24 bytes
// name confidence: 0.8   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x622100..0x622117: frees the post  data array and the post.
// blam-cc: cdecl

#include "gamespy.h"

#include "ghttp.h"

void ghttpFreePost(GHIPost *post)
{
    ArrayFree(post->data);
    free(post);
}
