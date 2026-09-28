import os, textwrap
os.chdir(r'C:\Users\Liam-\halo-re')


def emit_net(addr, size, name, note, cc, body):
    lines = textwrap.wrap('WRITTEN 2026-09-28 from objdump 0x%x..0x%x: %s' % (addr, addr + size - 1, note), 113)
    wr = ''.join(('// ' if k == 0 else '//   ') + l + '\n' for k, l in enumerate(lines))
    src = ('// %s  (not a Ghidra function; no C existed; named as its registrant names it)\n// address 0x%x, size %d bytes\n'
           '// name confidence: 0.4   rewrite confidence: 0.85\n%s// blam-cc: %s\n\n#include "tags.h"\n#include <string.h>\n\n%s'
           % (name, addr, size, wr, cc, body.lstrip('\n')))
    open('src/networking/%s.c' % name, 'w', encoding='utf-8').write(src)


REC = ('extern void network_connection_stats_record_packet(void *gamespy_connection, int32_t payload_length, uint8_t is_sent,\n'
       '    uint8_t is_reliable, uint8_t is_resend); // 0x440b20, blam-cc: EAX connection, ECX length\n')

emit_net(0x441020, 29, 'network_channel_gap_441020', 'the GT2 receive dump callback set by network_channels_open (socket, connection, ip, port, reset, message, length, reliable, resend): records the packet with is_sent 1 and the last two arguments as reliable / resend.', 'cdecl (a GT2 dump callback)',
         REC + '''
void network_channel_gap_441020(void *socket, void *connection, uint32_t ip, uint16_t port, int32_t reset, const void *message,
    int32_t length, int32_t reliable, int32_t resend)
{
    (void)socket;
    (void)ip;
    (void)port;
    (void)reset;
    (void)message;
    network_connection_stats_record_packet(connection, length, 1, (uint8_t)reliable, (uint8_t)resend);
}
''')
emit_net(0x441040, 23, 'network_channel_gap_441040', 'the GT2 send dump callback set by network_channels_open: records the packet with is_sent, reliable and resend all 0.', 'cdecl (a GT2 dump callback)',
         REC + '''
void network_channel_gap_441040(void *socket, void *connection, uint32_t ip, uint16_t port, int32_t reset, const void *message,
    int32_t length)
{
    (void)socket;
    (void)ip;
    (void)port;
    (void)reset;
    (void)message;
    network_connection_stats_record_packet(connection, length, 0, 0, 0);
}
''')
emit_net(0x441200, 243, 'network_channel_gap_441200', 'the query socket\'s GT2 unrecognized-message callback (socket, ip, port, message, length): the message (at most 0x1fff bytes) is copied to 0x006a6148 and terminated; a natneg packet (the 6 byte magic at 0x00657208) goes to NNProcessData with the sender as a sockaddr_in and is handled (1); a query ("\\\\" or ";" first, or 0xfe 0xfd) counts as handled (1); anything else 0.', 'cdecl (a GT2 unrecognized message callback)', '''
extern uint8_t network_query_receive_buffer[0x2000]; // 0x006a6148
extern const uint8_t natneg_magic[6];                // 0x00657208
extern void FUN_00615240(char *data, int32_t len, void *fromaddr); // 0x615240 NNProcessData

int32_t network_channel_gap_441200(void *socket, uint32_t ip, uint16_t port, const uint8_t *message, uint32_t length)
{
    uint8_t is_natneg = 0;
    uint8_t is_query;

    (void)socket;
    if (length >= 0x1fff) {
        length = 0x1fff;
    }
    memcpy(network_query_receive_buffer, message, length);
    network_query_receive_buffer[length] = 0;
    if ((int32_t)length >= 6 && memcmp(network_query_receive_buffer, natneg_magic, 6) == 0) {
        is_natneg = 1;
    }
    is_query = ((int32_t)length >= 1 && network_query_receive_buffer[0] == 0x5c) || network_query_receive_buffer[0] == 0x3b ||
               ((int32_t)length >= 2 && network_query_receive_buffer[0] == 0xfe && network_query_receive_buffer[1] == 0xfd);
    if (is_natneg) {
        uint8_t address[16];

        memset(address, 0, sizeof(address));
        *(uint16_t *)(address + 0) = 2;
        *(uint16_t *)(address + 2) = (uint16_t)((port >> 8) | (port << 8));
        *(uint32_t *)(address + 4) = ip;
        FUN_00615240((char *)network_query_receive_buffer, (int32_t)length, address);
        return 1;
    }
    return is_query ? 1 : 0;
}
''')
print('ok')
