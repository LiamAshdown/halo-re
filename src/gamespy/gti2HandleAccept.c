// gti2HandleAccept  (GameSpy SDK in halo.exe; no C existed)
// address 0x618670, size 173 bytes
// name confidence: 0.8   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x618670..0x61871c: ECX connection, stack data: only while awaiting acceptance
//   (else a negotiation error); keeps the peer  16-byte public key, derives the shared key = peer ^ private mod
//   modulus, is connected (5) and the connected callback hears success.
// blam-cc: ECX -> connection, stack -> data

#include "gamespy.h"

#include "gt2.h"

int gti2HandleAccept(GTI2Connection *connection, const unsigned char *data)
{
    char hex[0x30];

    if (connection->state != GTI2AwaitingAcceptance) {
        return gti2ConnectionError(connection, 7, 2) != 0;
    }
    memcpy(connection->remotePublicKey, data, 0x10);
    gt2_bignum_to_hex(connection->remotePublicKey, hex);
    gt2_bignum_mod_exp(hex, connection->privateExponent, connection->modulus, connection->key);
    connection->state = GTI2Connected;
    return gti2ConnectedCallback(connection, 0, 0, 0) != 0;
}
