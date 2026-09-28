// TimeoutOldQueries  (GameSpy SDK in halo.exe; no C existed)
// address 0x61e8b0, size 91 bytes
// name confidence: 0.8   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x61e8b0..0x61e90a: ESI engine: queries older than 2.5 s fail from the front:
//   flag 0x10 set and 0x0c cleared in the flags byte (+0x15), the callback hears failure (1), and it leaves the list.
// blam-cc: ESI -> engine

#include "gamespy.h"

#include "sb.h"

void TimeoutOldQueries(SBQueryEngine *engine)
{
    unsigned long now = current_time();

    while (engine->querylist.first != 0 && now > engine->querylist.first->updatetime + 2500) {
        engine->querylist.first->flags |= 0x10;
        engine->querylist.first->flags &= 0xf3;
        engine->ListCallback(engine, 1, engine->querylist.first, engine->instance);
        if (engine->querylist.first != 0) {
            engine->querylist.first = engine->querylist.first->next;
            if (engine->querylist.first == 0) {
                engine->querylist.last = 0;
            }
            engine->querylist.count--;
        }
    }
}
