// qr2_register_key  (Ghidra: FUN_0061bb40; statically linked GameSpy query-and-reporting SDK)
// address 0x61bb40, size 28 bytes
// name confidence: 0.85  rewrite confidence: 0.95
// evidence: network_session_host_start_info_set 0x4e4b00 registers the custom server keys 0x33 "dedicated",
//   0x34 "player_flags", 0x35 "game_flags" and 0x36 "game_classic" with it -- GameSpy qr2's
//   qr2_register_key(keyid, key). objdump 0x61bb40..0x61bb5b: ids from 50 (NUM_RESERVED_KEYS) to 254
//   (MAX_REGISTERED_KEYS - 1) store the name pointer in the key table at 0x00683990; any other id is ignored.
//   First-boot track: the standalone exe reached it during shell_winmain's network setup.
// blam-cc: stack -> keyid, key (cdecl)

#include "tags.h"
#include "memory.h"
#include "math.h"

extern const char *qr2_registered_key_list[255]; // 0x00683990

void qr2_register_key(int32_t keyid, const char *key)
{
    if (keyid >= 50 && keyid <= 254) {
        qr2_registered_key_list[keyid] = key;
    }
}
