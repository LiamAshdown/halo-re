exec(open(r'C:\Users\Liam-\halo-re\scratchpad\gs_lib.py').read())
Q = open(r'C:\Users\Liam-\halo-re\scratchpad\gen_gs9.py', encoding='utf-8').read().split("Q = '''")[1].split("'''")[0]
Q += '''
extern unsigned int qr2_local_ips[5];    // 0x006a26d8
extern int qr2_local_ip_count;           // 0x006a27f0
void send_heartbeat(qr2_implementation_s *qrec, int statechanged);
void qr2_check_send_heartbeat(qr2_implementation_s *qrec);
void qr2_check_queries(qr2_implementation_s *qrec);
void send_keepalive(qr2_implementation_s *qrec);
'''


def e(addr, size, name, note, code, cc='cdecl'):
    emit(addr, size, name, note, Q + '\n' + code, cc=cc)


e(0x616680, 1226, 'send_heartbeat', 'a heartbeat (type 3, instance key) to the master: "localip<n>" per cached local address, "localport" (the query port), "natneg" ("1"/"0"), "statechanged" (when non-zero), "gamename"; a state change of 2 (exiting) ends there, otherwise every server, player and team key follows (qr_build_partial_query_reply with 0xff, EBX the record). Remembers the heartbeat and keep-alive time.', '''
extern void qr_build_partial_query_reply(qr2_implementation_s *qrec, qr2_buffer_s *outbuf, int keytype, int numkeys,
    unsigned char *keys);

void send_heartbeat(qr2_implementation_s *qrec, int statechanged)
{
    qr2_buffer_s buffer;
    char text[0x14];
    int i;

    buffer.buffer[0] = 3;
    memcpy(buffer.buffer + 1, qrec->instance_key, 4);
    buffer.len = 5;
    for (i = 0; i < qr2_local_ip_count; i++) {
        struct in_addr address;

        sprintf(text, "localip%d", i);
        qr2_buffer_add(&buffer, text);
        address.s_addr = qr2_local_ips[i];
        qr2_buffer_add(&buffer, inet_ntoa(address));
    }
    qr2_buffer_add(&buffer, "localport");
    sprintf(text, "%d", qrec->qport);
    qr2_buffer_add(&buffer, text);
    qr2_buffer_add(&buffer, "natneg");
    qr2_buffer_add(&buffer, qrec->nat_negotiate != 0 ? "1" : "0");
    if (statechanged != 0) {
        qr2_buffer_add(&buffer, "statechanged");
        sprintf(text, "%d", statechanged);
        qr2_buffer_add(&buffer, text);
    }
    qr2_buffer_add(&buffer, "gamename");
    qr2_buffer_add(&buffer, qrec->gamename);
    if (statechanged == 2) {
        if (0x800 - buffer.len >= 1) {
            buffer.buffer[buffer.len] = 0;
            buffer.len++;
        }
    } else {
        qr_build_partial_query_reply(qrec, &buffer, 0, 0xff, 0);
        qr_build_partial_query_reply(qrec, &buffer, 1, 0xff, 0);
        qr_build_partial_query_reply(qrec, &buffer, 2, 0xff, 0);
    }
    sendto(qrec->hbsock, buffer.buffer, buffer.len, 0, (const struct sockaddr *)&qrec->hbaddr, 0x10);
    qrec->lastheartbeat = current_time();
    qrec->lastka = qrec->lastheartbeat;
}
''')
e(0x616b50, 173, 'qr2_check_send_heartbeat', 'EAX record, with a socket: while a state change is requested it is resent every 10 s (statechanged 3) up to four times, after which the add-error callback hears 5 "No challenge value was received from the master server."; otherwise a plain heartbeat goes out after a minute (or when none was sent yet, or the clock wrapped). A keep-alive follows 20 s after the last one.', '''
void qr2_check_send_heartbeat(qr2_implementation_s *qrec)
{
    unsigned long now = current_time();

    if (qrec->hbsock == INVALID_SOCKET) {
        return;
    }
    if (qrec->userstatechangerequested > 0 && now - qrec->lastheartbeat > 10000) {
        if (qrec->userstatechangerequested >= 4) {
            qrec->userstatechangerequested = 0;
            qrec->adderror_callback(5, "No challenge value was received from the master server.", qrec->udata);
        } else {
            send_heartbeat(qrec, 3);
            qrec->userstatechangerequested++;
        }
    } else if (now - qrec->lastheartbeat > 60000 || qrec->lastheartbeat == 0 || now < qrec->lastheartbeat) {
        send_heartbeat(qrec, 0);
    }
    if (now - qrec->lastka > 20000) {
        send_keepalive(qrec);
    }
}
''', cc='EAX -> qrec')
e(0x616c00, 56, 'qr2_send_statechanged', 'for a listed record (the current one for NULL), a statechanged heartbeat (1) unless one went out in the last 10 s.', '''
void qr2_send_statechanged(qr2_implementation_s *qrec)
{
    if (qrec == 0) {
        qrec = current_rec;
    }
    if (qrec->listed_state != 0 && current_time() - qrec->lastheartbeat > 9999) {
        send_heartbeat(qrec, 1);
    }
}
''')
e(0x616c40, 109, 'qr2_shutdown', 'the record (the current one for NULL): a listed one says goodbye (statechanged 2); a socket it reads itself is closed (and WSACleanup); a malloc\'d record is freed.', '''
void qr2_shutdown(qr2_implementation_s *qrec)
{
    if (qrec == 0) {
        qrec = current_rec;
    }
    if (qrec->listed_state != 0) {
        send_heartbeat(qrec, 2);
    }
    if (qrec->hbsock != INVALID_SOCKET && qrec->read_socket != 0) {
        closesocket(qrec->hbsock);
    }
    qrec->hbsock = INVALID_SOCKET;
    qrec->lastheartbeat = 0;
    if (qrec->read_socket != 0) {
        WSACleanup();
    }
    if (qrec != &static_rec) {
        free(qrec);
    }
}
''')
e(0x616cb0, 39, 'qr2_think', 'the record (the current one for NULL): heartbeats when listed, then the query socket.', '''
void qr2_think(qr2_implementation_s *qrec)
{
    if (qrec == 0) {
        qrec = current_rec;
    }
    if (qrec->listed_state != 0) {
        qr2_check_send_heartbeat(qrec);
    }
    qr2_check_queries(qrec);
}
''')
print('ok')
