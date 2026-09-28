exec(open(r'C:\Users\Liam-\halo-re\scratchpad\gs_lib.py').read())
import textwrap

F = '#define FIELD(object, offset, type) (*(type *)((char *)(object) + (offset)))\n'
emit(0x615530, 35, 'qr2_register_natneg_callback', 'stores the NAT negotiation callback at +0xa0 of the query/report record, or of the current record (0x00683940) for NULL.', F + '''
extern void *current_rec; // 0x00683940 (qr2_t)

void qr2_register_natneg_callback(void *qrec, void *callback)
{
    if (qrec == 0) {
        FIELD(current_rec, 0xa0, void *) = callback;
    } else {
        FIELD(qrec, 0xa0, void *) = callback;
    }
}
''')
emit(0x615560, 45, 'qr2_keybuffer_add', 'appends a key id (1..0xfe) to a key buffer (bytes, count at +0x100) holding fewer than 0xfe keys.', F + '''
void qr2_keybuffer_add(unsigned char *keybuffer, int key_id)
{
    int count = FIELD(keybuffer, 0x100, int);

    if (count >= 0xfe || key_id < 1 || key_id > 0xfe) {
        return;
    }
    keybuffer[count] = (unsigned char)key_id;
    FIELD(keybuffer, 0x100, int) = count + 1;
}
''')
emit(0x615590, 103, 'qr2_buffer_add', 'appends the string with its NUL to a 0x800 byte buffer (length at +0x800), truncated to the room left and kept NUL-terminated.', F + '''
void qr2_buffer_add(char *buffer, const char *value)
{
    int copy = (int)strlen(value) + 1;
    int room = 0x800 - FIELD(buffer, 0x800, int);

    if (copy > room) {
        copy = room;
    }
    if (copy == 0) {
        return;
    }
    memcpy(buffer + FIELD(buffer, 0x800, int), value, copy);
    FIELD(buffer, 0x800, int) += copy;
    buffer[FIELD(buffer, 0x800, int) - 1] = 0;
}
''')


def emit_net(addr, size, name, note, cc, body):
    lines = textwrap.wrap('WRITTEN 2026-09-28 from objdump 0x%x..0x%x: %s' % (addr, addr + size - 1, note), 113)
    wr = ''.join(('// ' if k == 0 else '//   ') + l + '\n' for k, l in enumerate(lines))
    src = ('// %s  (not a Ghidra function; no C existed)\n// address 0x%x, size %d bytes\n'
           '// name confidence: 0.6   rewrite confidence: 0.85\n%s// blam-cc: %s\n\n#include "tags.h"\n\n%s'
           % (name, addr, size, wr, cc, body.lstrip('\n')))
    open('src/networking/%s.c' % name, 'w', encoding='utf-8').write(src)


emit_net(0x578120, 49, 'network_session_host_natneg_completed', 'the NAT negotiation completion callback (result, socket, remote sockaddr_in, user data): on success it formats the remote address into a stack buffer with gt2AddressToString and does nothing with it (a leftover).', 'cdecl (a NAT negotiation callback)', '''
extern uint16_t FUN_006148a0(uint16_t value); // 0x6148a0 gt2NetworkToHostShort
extern char *FUN_006148b0(uint32_t ip, uint16_t port, char *string); // 0x6148b0 gt2AddressToString

void network_session_host_natneg_completed(int32_t result, uint32_t socket, const uint8_t *remote_address, void *user_data)
{
    char text[0x16];

    (void)socket;
    (void)user_data;
    if (result == 0) {
        FUN_006148b0(*(const uint32_t *)(remote_address + 4), FUN_006148a0(*(const uint16_t *)(remote_address + 2)), text);
    }
}
''')
emit_net(0x578160, 40, 'network_session_host_natneg_callback', 'the query/report NAT negotiation callback (cookie): starts NNBeginNegotiationWithSocket on the game socket\'s SOCKET (read through 0x6175f0, folded with ArrayLength) with that cookie, client index 0, function_do_nothing as the progress callback and network_session_host_natneg_completed.', 'cdecl (the qr2 natneg callback)', '''
extern int32_t network_game_socket; // 0x006f14c4 (GT2Socket)
extern void function_do_nothing(void); // 0x44ad80
extern void network_session_host_natneg_completed(int32_t result, uint32_t socket, const uint8_t *remote_address,
    void *user_data); // 0x578120
extern int32_t FUN_00614f30(uint32_t socket, int32_t cookie, int32_t client_index, void *progress_callback,
    void *completed_callback, void *user_data); // 0x614f30 NNBeginNegotiationWithSocket

void network_session_host_natneg_callback(int32_t cookie)
{
    FUN_00614f30(*(uint32_t *)network_game_socket, cookie, 0, (void *)function_do_nothing,
        (void *)network_session_host_natneg_completed, 0);
}
''')

p = 'src/networking/network_session_host_start.c'
s = open(p, encoding='utf-8').read()
h, sep, t = s.partition('#if 0')
o = '    FUN_00615530(network_session_host_object, (void *)0);'
n = ('    // FIXED 2026-09-28: 0x5778b5 registers 0x578160 (network_session_host_natneg_callback), not NULL.\n'
     '    FUN_00615530(network_session_host_object, (void *)network_session_host_natneg_callback);')
assert h.count(o) == 1
h = h.replace(o, n)
anchor = 'extern int32_t gamespy_array_length(int32_t socket);'
i = h.index(anchor)
h = h[:i] + 'extern void network_session_host_natneg_callback(int32_t cookie); // 0x578160\n' + h[i:]
open(p, 'w', encoding='utf-8').write(h + sep + t)
print('ok')
