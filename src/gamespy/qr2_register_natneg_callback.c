// qr2_register_natneg_callback  (GameSpy SDK in halo.exe; no C existed)
// address 0x615530, size 35 bytes
// name confidence: 0.8   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x615530..0x615552: stores the NAT negotiation callback at +0xa0 of the
//   query/report record, or of the current record (0x00683940) for NULL.
// blam-cc: cdecl

#include "gamespy.h"
#include "fn_gamespy.h"

#define FIELD(object, offset, type) (*(type *)((char *)(object) + (offset)))

extern void *current_rec; // 0x00683940 (qr2_t)

void qr2_register_natneg_callback(void *qrec, void *callback)
{
    if (qrec == 0) {
        FIELD(current_rec, 0xa0, void *) = callback;
    } else {
        FIELD(qrec, 0xa0, void *) = callback;
    }
}
