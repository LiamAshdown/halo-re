// gti2ResendMessages  (GameSpy SDK in halo.exe; no C existed)
// address 0x61cf70, size 119 bytes
// name confidence: 0.8   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x61cf70..0x61cfe6: EDI connection, stack now: an outgoing message resent more
//   than 10 times is dropped; one unanswered for over a second is resent (0 when that fails).
// blam-cc: EDI -> connection, stack -> now

#include "gamespy.h"

#include "gt2.h"

int gti2ResendMessages(GTI2Connection *connection, unsigned long now)
{
    int count = ArrayLength(connection->outgoingBufferMessages);
    int i;

    for (i = 0; i < count; i++) {
        GTI2OutgoingBufferMessage *message = (GTI2OutgoingBufferMessage *)ArrayNth(connection->outgoingBufferMessages, i);

        if (message->resends > 10) {
            ArrayDeleteAt(connection->outgoingBufferMessages, i);
            count--;
            i--;
        } else if (now - message->lastSend > 1000) {
            if (!gti2ResendMessage(connection, message)) {
                return 0;
            }
        }
    }
    return 1;
}
