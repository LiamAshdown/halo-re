// FIFORemove  (GameSpy SDK in halo.exe; no C existed)
// address 0x61e340, size 70 bytes
// name confidence: 0.8   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x61e340..0x61e385: EAX server, EDX fifo: unlinks the server when it is there
//   (1), else 0.
// blam-cc: EAX -> server, EDX -> fifo

#include "gamespy.h"

#include "sb.h"

int FIFORemove(SBServer *server, SBServerFIFO *fifo)
{
    SBServer *hold = 0;
    SBServer *it;

    for (it = fifo->first; it != 0; it = it->next) {
        if (it == server) {
            break;
        }
        hold = it;
    }
    if (it == 0) {
        return 0;
    }
    if (hold != 0) {
        hold->next = it->next;
    }
    if (fifo->first == it) {
        fifo->first = it->next;
    }
    if (fifo->last == it) {
        fifo->last = hold;
    }
    fifo->count--;
    return 1;
}
