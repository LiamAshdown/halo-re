p = "C:\\Users\\Liam-\\halo-re\\src\\networking\\network_channel_attempt_connect.c"
t = open(p, encoding="utf-8").read()


def sub(old, new):
    global t
    assert t.count(old) == 1, old[:70]
    t = t.replace(old, new)


sub('''extern uint32_t FUN_00614890(uint32_t ipv4); // foreign, GameSpy library
extern void FUN_006148b0(uint32_t address, uint16_t port, void *out_address); // 0x6148b0: fills a
    // 0x16-byte address record (stride confirmed by the imul esi,esi,0x16 at 0x6148c8) // foreign
extern int32_t FUN_006145a0(int32_t socket); // foreign, GameSpy library
extern void FUN_00614830(int32_t socket, network_receive_queue *queue); // foreign, GameSpy library // foreign, GameSpy library
''', '''extern int gt2NetworkToHostInt(unsigned int value); // 0x614890
extern char *gt2AddressToString(unsigned int ip, unsigned short port, char *string); // 0x6148b0
extern int gt2Connect(void *socket, void **connection_out, const char *remote_address, const unsigned char *message,
    int len, unsigned long timeout, const void *callbacks, int blocking); // 0x6145a0, cdecl
extern void gt2SetConnectionData(void *connection, void *data); // 0x614830
extern int32_t network_connect_timeout_ms; // 0x006894ac
extern void network_channel_connected_callback(void *connection, int32_t result, const uint8_t *message, int32_t length); // 0x441e00
extern void network_channel_receive_callback(void *handle, uint8_t *data, int32_t length); // 0x441ed0
extern void network_channel_gap_441f30(void *connection); // 0x441f30
extern void function_do_nothing(void); // 0x44ad80

// FIXED 2026-09-28 (networking call audit, from the disassembly 0x441f60..0x442036): gt2Connect takes eight
// arguments -- the socket, the queue (whose +0 receives the connection), the formatted remote address, a 4-byte
// message that is this function's third argument (its address, length 4), the connect timeout (0x6894ac), a local
// GT2ConnectionCallbacks {connected 0x441e00, received 0x441ed0, closed 0x441f30, ping 0x44ad80} and 0 (not
// blocking); the previous C passed only the socket. On success the connection's data is the queue.
''')
sub('''    uint32_t formatted_address;
    int32_t socket;
    int32_t connect_result;
    uint8_t address_buf[24];

    formatted_address = FUN_00614890(address->ipv4);
    FUN_006148b0(formatted_address, address->port, address_buf);
''', '''    uint32_t formatted_address;
    int32_t socket;
    int32_t connect_result;
    char address_buf[24];
    void *callbacks[4];

    formatted_address = (uint32_t)gt2NetworkToHostInt(address->ipv4);
    gt2AddressToString(formatted_address, address->port, address_buf);
    callbacks[0] = (void *)network_channel_connected_callback;
    callbacks[1] = (void *)network_channel_receive_callback;
    callbacks[2] = (void *)network_channel_gap_441f30;
    callbacks[3] = (void *)function_do_nothing;
''')
sub('''        connect_result = FUN_006145a0(socket);
        if (connect_result == 0) {
            queue->unknown_05 = 0;
            FUN_00614830(queue->socket, queue);''', '''        connect_result = gt2Connect((void *)socket, (void **)&queue->socket, address_buf,
                                    (const unsigned char *)&unused_param_1, 4, (unsigned long)network_connect_timeout_ms,
                                    callbacks, 0);
        if (connect_result == 0) {
            queue->unknown_05 = 0;
            gt2SetConnectionData((void *)queue->socket, queue);''')
open(p, "w", encoding="utf-8", newline="\n").write(t)
print("ok")
