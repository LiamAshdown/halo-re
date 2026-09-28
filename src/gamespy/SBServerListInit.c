// SBServerListInit  (GameSpy SDK in halo.exe; no C existed)
// address 0x61f2f0, size 216 bytes
// name confidence: 0.8   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x61f2f0..0x61f3c7: state 1, a server array (ArrayNew(4, 100)), the ref-string
//   table touched, the three query names copied to +0x0c / +0x2c / +0x4c, the callback and instance (+0x480 /
//   +0x484), the version (+0x4a8), "" at +0x488 and +0x49c, -1 at +0x4a0 and +0x47c, zeros elsewhere; then
//   srand(current_time()) and SocketStartUp.
// blam-cc: cdecl

#include "gamespy.h"

#define FIELD(object, offset, type) (*(type *)((char *)(object) + (offset)))

void SBServerListInit(void *slist, const char *query_for_gamename, const char *query_from_gamename,
    const char *query_from_key, int query_from_version, SBListCallBackFn callback, void *instance)
{
    FIELD(slist, 0x00, int) = 1;
    FIELD(slist, 0x04, DArray) = ArrayNew(sizeof(void *), 100, 0);
    FIELD(slist, 0x5bc, void *) = 0;
    SBRefStrHash(slist);
    strcpy((char *)slist + 0x0c, query_for_gamename);
    strcpy((char *)slist + 0x2c, query_from_gamename);
    strcpy((char *)slist + 0x4c, query_from_key);
    FIELD(slist, 0x480, SBListCallBackFn) = callback;
    FIELD(slist, 0x484, void *) = instance;
    FIELD(slist, 0x488, const char *) = "";
    FIELD(slist, 0x490, int) = 0;
    FIELD(slist, 0x4a0, int) = -1;
    FIELD(slist, 0x74, int) = 0;
    FIELD(slist, 0x78, int) = 0;
    FIELD(slist, 0x08, int) = 0;
    FIELD(slist, 0x47c, int) = -1;
    FIELD(slist, 0x478, int) = 0;
    FIELD(slist, 0x49c, const char *) = "";
    FIELD(slist, 0x494, int) = 0;
    FIELD(slist, 0x4a8, int) = query_from_version;
    srand(current_time());
    SocketStartUp();
}
