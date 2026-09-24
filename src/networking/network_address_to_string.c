// network_address_to_string  (Ghidra: network_address_to_string, already named)
// address 0x440570, size 149 bytes
// name confidence: 0.75   rewrite confidence: 0.85
// evidence: out/phase4/networking_types_notes.md "s_network_address (0x14)" -- this function
// is the entire evidence base for that struct: it reads addr[8] (halfword) as `size` and, for
// the IPv4 case, prints bytes +3,+2,+1,+0 of the address as %hd.%hd.%hd.%hd (high byte
// first) and addr[9] as the port.
// register convention: EAX -> addr, no stack arguments.
// FIXED (register inputs, objdump): notes wording ("address pointer in EAX") didn't match the
// checker's alias for s_network_address*, so EAX (read at 0x440577) looked unclaimed. Body
// already used addr correctly; reworded the blam-cc line to the standard "REG -> name" form.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"

extern char network_address_string[0x100]; // 0x006a3f38, shared format buffer

extern int32_t snprintf(char *buffer, uint32_t count, const char *format, ...);

// blam-cc: EAX -> addr
// Formats addr as "a.b.c.d:port" for an IPv4 address (size == 4) or eight hex groups for an
// IPv6-shaped address (size == 0x10), into the shared static buffer, and returns it. If
// neither size matches, the buffer is left as the empty string that was just written to it.
char *network_address_to_string(s_network_address *addr)
{
    uint16_t *halfwords;

    network_address_string[0] = 0;
    if (addr->size == k_network_address_size_ipv4) {
        snprintf(network_address_string, 0x100, "%hd.%hd.%hd.%hd:%hu",
                 (uint32_t)*((uint8_t *)addr + 3),
                 (uint32_t)*((uint8_t *)addr + 2),
                 (uint32_t)*((uint8_t *)addr + 1),
                 (uint32_t)*((uint8_t *)addr + 0),
                 (uint32_t)addr->port);
        return network_address_string;
    }
    if (addr->size == k_network_address_size_ipv6) {
        halfwords = (uint16_t *)addr;
        snprintf(network_address_string, 0x100, "%4X.%4X.%4X.%4X.%4X.%4X.%4X.%4X:%hu",
                 (uint32_t)halfwords[0], (uint32_t)halfwords[1], (uint32_t)halfwords[2],
                 (uint32_t)halfwords[3], (uint32_t)halfwords[4], (uint32_t)halfwords[5],
                 (uint32_t)halfwords[6], (uint32_t)halfwords[7], (uint32_t)addr->port);
    }
    return network_address_string;
}

#if 0
Original Ghidra decompilation (0x440570):

undefined * network_address_to_string(void)

{
  ushort *in_EAX;

  DAT_006a3f38 = 0;
  if (in_EAX[8] == 4) {
    __snprintf(&DAT_006a3f38,0x100,"%hd.%hd.%hd.%hd:%hu",(uint)*(byte *)((int)in_EAX + 3),
               (uint)(byte)in_EAX[1],(uint)*(byte *)((int)in_EAX + 1),(uint)(byte)*in_EAX,
               (uint)in_EAX[9]);
    return &DAT_006a3f38;
  }
  if (in_EAX[8] == 0x10) {
    __snprintf(&DAT_006a3f38,0x100,"%4X.%4X.%4X.%4X.%4X.%4X.%4X.%4X:%hu",(uint)*in_EAX,
               (uint)in_EAX[1],(uint)in_EAX[2],(uint)in_EAX[3],(uint)in_EAX[4],(uint)in_EAX[5],
               (uint)in_EAX[6],(uint)in_EAX[7],(uint)in_EAX[9]);
  }
  return &DAT_006a3f38;
}
#endif
