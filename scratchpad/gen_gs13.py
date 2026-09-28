exec(open(r'C:\Users\Liam-\halo-re\scratchpad\gs_lib.py').read())
H = '#include "gt2.h"\n\n'


def e(addr, size, name, note, code, cc='cdecl'):
    emit(addr, size, name, note, H + code.lstrip('\n'), cc=cc)


# ------------------------------------------------------------------ gt2Main.c
e(0x614540, 42, 'gt2Think', 'receives and handles everything waiting on the socket, then lets every connection think; when both succeed the closed connections are freed.', '''
void gt2Think(GTI2Socket *socket)
{
    if (gti2ReceiveMessages(socket) && gti2SocketConnectionsThink(socket)) {
        gti2FreeClosedConnections(socket);
    }
}
''')
e(0x6145a0, 265, 'gt2Connect', '(socket, connection out, remote address string, message, len, timeout, callbacks, blocking): 4 for an unparsable address or a zero ip or port; otherwise a new outgoing connection with the timeout starts its attempt (either failure code is returned). Non-blocking hands out the connection (when asked) and returns 0; blocking holds a callback level on the connection and thinks the socket (gt2Think inline) with msleep(1) until the state reaches connected or beyond, hands out the connection only if it is connected, and returns its connect result.', '''
int gt2Connect(GTI2Socket *socket, GTI2Connection **connection_out, const char *remote_address, const unsigned char *message,
    int len, unsigned long timeout, const GT2ConnectionCallbacks *callbacks, int blocking)
{
    GTI2Connection *connection;
    unsigned int ip;
    unsigned short port;
    int result;

    if (!gt2StringToAddress(remote_address, &ip, &port) || ip == 0 || port == 0) {
        return 4;
    }
    result = gti2NewOutgoingConnection(socket, &connection, ip, port);
    if (result != 0) {
        return result;
    }
    connection->timeout = timeout;
    result = gti2StartConnectionAttempt(connection, message, len, callbacks);
    if (result != 0) {
        return result;
    }
    if (!blocking) {
        if (connection_out != 0) {
            *connection_out = connection;
        }
        return 0;
    }
    connection->callbackLevel++;
    for (;;) {
        if (gti2ReceiveMessages(socket) && gti2SocketConnectionsThink(socket)) {
            gti2FreeClosedConnections(socket);
        }
        if (connection->state >= GTI2Connected) {
            break;
        }
        msleep(1);
    }
    connection->callbackLevel--;
    if (connection->state == GTI2Connected) {
        *connection_out = connection;
    }
    return connection->connectionResult;
}
''')
e(0x6146b0, 84, 'gt2Send', 'only on a connected connection: the message is checked (NULL / -1 length), then goes through send filter 0 when any are registered, else straight to gti2Send (which appends a CRC and encrypts IN PLACE, so the buffer needs 4 spare bytes). Returns what that returned; 0 when not connected (the binary leaves EAX as it was).', '''
int gt2Send(GTI2Connection *connection, const unsigned char *message, int len, int reliable)
{
    if (connection->state != GTI2Connected) {
        return 0;
    }
    gti2MessageCheck((const char **)&message, &len);
    if (ArrayLength(connection->sendFilters) != 0) {
        return gti2SendFilterCallback(connection, 0, message, len, reliable);
    }
    return gti2Send(connection, (unsigned char *)message, len, reliable);
}
''')
e(0x614710, 16, 'gt2CloseConnectionHard', 'gti2CloseConnection(connection, 1).', '''
void gt2CloseConnectionHard(GTI2Connection *connection)
{
    gti2CloseConnection(connection, 1);
}
''')
e(0x614740, 24, 'gt2CloseAllConnections', 'TableMapSafe over the connections with 0x614720 (a static here: gti2CloseConnection(*elem, 0), the soft close).', '''
static void gti2CloseConnectionSoftMap(void *elem, void *client_data) // 0x614720
{
    (void)client_data;
    gti2CloseConnection(*(GTI2Connection **)elem, 0);
}

void gt2CloseAllConnections(GTI2Socket *socket)
{
    TableMapSafe(socket->connections, gti2CloseConnectionSoftMap, 0);
}
''')
e(0x614760, 23, 'gti2CloseConnectionHardMap', 'TableMapSafe callback: gti2CloseConnection(*elem, 1).', '''
void gti2CloseConnectionHardMap(void *elem, void *client_data)
{
    (void)client_data;
    gti2CloseConnection(*(GTI2Connection **)elem, 1);
}
''')
e(0x614780, 24, 'gt2CloseAllConnectionsHard', 'TableMapSafe over the connections with gti2CloseConnectionHardMap 0x614760.', '''
extern void gti2CloseConnectionHardMap(void *elem, void *client_data);

void gt2CloseAllConnectionsHard(GTI2Socket *socket)
{
    TableMapSafe(socket->connections, gti2CloseConnectionHardMap, 0);
}
''')
e(0x614860, 36, 'gt2CloseSocket', 'for a socket: hard-closes every connection (TableMapSafe with 0x614760), then gti2FreeSocket.', '''
extern void gti2CloseConnectionHardMap(void *elem, void *client_data);

void gt2CloseSocket(GTI2Socket *socket)
{
    if (socket == 0) {
        return;
    }
    TableMapSafe(socket->connections, gti2CloseConnectionHardMap, 0);
    gti2FreeSocket(socket);
}
''')

# ------------------------------------------------------------------ gt2Socket.c
e(0x61c360, 118, 'gti2SocketFindConnection', 'looks the (ip, port) up in the connection table through a stack connection record holding just those two; the connection or NULL.', '''
GTI2Connection *gti2SocketFindConnection(GTI2Socket *socket, unsigned int ip, unsigned short port)
{
    GTI2Connection key;
    GTI2Connection *key_pointer = &key;
    GTI2Connection **found;

    key.ip = ip;
    key.port = port;
    found = (GTI2Connection **)TableLookup(socket->connections, &key_pointer);
    if (found != 0) {
        return *found;
    }
    return 0;
}
''')
e(0x61c670, 149, 'gti2GeneratePrivateExponent', 'EDI buffer: 16 upper-case hex digits (|rand()| % 16) and a NUL.', '''
void gti2GeneratePrivateExponent(char *hex)
{
    static const char digits[16] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'};
    int i;

    for (i = 0; i < 0x10; i++) {
        hex[i] = digits[abs(rand()) % 16];
    }
    hex[i] = 0;
}
''', cc='EDI -> hex')
e(0x61c730, 811, 'gti2NewSocketConnection', '5 when the (ip, port) already has a connection; otherwise a zeroed 0x150-byte connection (ip, port, socket, start and last-send time now, default key bytes 3 8 3 3). More than 16 connections from the same ip (counted with TableMap and the static 0x61c710, which bumps the int the new connection  data temporarily points at) fail. The key exchange: generator "3", modulus "10001" (0x64e644), a random private exponent, publicKey = 3 ^ private mod 0x10001. Then the incoming/outgoing buffers (socket sizes), the incoming (0x10-byte, grow 0x40) and outgoing (0x14, 0x40) message arrays and the send/receive filter arrays (4, 2); it is entered in the table and handed out (0). Any allocation failure frees everything and gives 1.', '''
static void gti2CountConnectionsFromIP(void *elem, void *client_data) // 0x61c710
{
    GTI2Connection *connection = (GTI2Connection *)client_data;

    if ((*(GTI2Connection **)elem)->ip == connection->ip) {
        (*(int *)connection->data)++;
    }
}

int gti2NewSocketConnection(GTI2Socket *socket, GTI2Connection **connection_out, unsigned int ip, unsigned short port)
{
    GTI2Connection *connection = 0;
    int count = 0;
    void *data;
    char address[0x18];

    gt2AddressToString(ip, port, address);
    if (gti2SocketFindConnection(socket, ip, port) != 0) {
        return 5;
    }
    connection = (GTI2Connection *)malloc(0x150);
    if (connection == 0) {
        return 1;
    }
    memset(connection, 0, 0x150);
    connection->ip = ip;
    connection->port = port;
    connection->socket = socket;
    connection->startTime = current_time();
    connection->lastSend = connection->startTime;
    connection->lastAck = 0;
    connection->maxResends = 0;
    connection->serialNumber = 0;
    connection->expectedSerialNumber = 0;
    connection->key[0] = 3;
    connection->key[1] = 8;
    connection->key[2] = 3;
    connection->key[3] = 3;
    data = connection->data;
    connection->data = &count;
    TableMap(socket->connections, gti2CountConnectionsFromIP, connection);
    if (count <= 0x10) {
        connection->data = data;
        *(unsigned short *)connection->generator = 0x33;
        memcpy(connection->modulus, "10001", 6);
        gti2GeneratePrivateExponent(connection->privateExponent);
        gt2_bignum_mod_exp(connection->generator, connection->privateExponent, connection->modulus,
            connection->publicKey);
        if (gti2AllocateBuffer(&connection->incomingBuffer, socket->incomingBufferSize) &&
            gti2AllocateBuffer(&connection->outgoingBuffer, socket->outgoingBufferSize) &&
            (connection->incomingBufferMessages = ArrayNew(0x10, 0x40, 0)) != 0 &&
            (connection->outgoingBufferMessages = ArrayNew(0x14, 0x40, 0)) != 0 &&
            (connection->sendFilters = ArrayNew(4, 2, 0)) != 0 &&
            (connection->receiveFilters = ArrayNew(4, 2, 0)) != 0) {
            TableEnter(socket->connections, &connection);
            *connection_out = gti2SocketFindConnection(socket, ip, port);
            if (*connection_out != 0) {
                return 0;
            }
        }
    }
    free(connection->incomingBuffer.buffer);
    free(connection->outgoingBuffer.buffer);
    if (connection->incomingBufferMessages != 0) {
        ArrayFree(connection->incomingBufferMessages);
    }
    if (connection->outgoingBufferMessages != 0) {
        ArrayFree(connection->outgoingBufferMessages);
    }
    if (connection->sendFilters != 0) {
        ArrayFree(connection->sendFilters);
    }
    if (connection->receiveFilters != 0) {
        ArrayFree(connection->receiveFilters);
    }
    free(connection);
    return 1;
}
''')
e(0x61ca60, 127, 'gti2FreeClosedConnection', 'unless held (freeAtAcceptReject or a callback level): a closed connection is found in the socket  closed list and deleted there (its free function frees it); any other is removed from the connection table.', '''
void gti2FreeClosedConnection(GTI2Connection *connection)
{
    GTI2Socket *socket;
    int count;
    int i;

    if (connection->freeAtAcceptReject != 0 || connection->callbackLevel != 0) {
        return;
    }
    socket = connection->socket;
    if (connection->state != GTI2Closed) {
        TableRemove(socket->connections, &connection);
        return;
    }
    count = ArrayLength(socket->closedConnections);
    for (i = 0; i < count; i++) {
        if (connection == *(GTI2Connection **)ArrayNth(connection->socket->closedConnections, i)) {
            ArrayDeleteAt(connection->socket->closedConnections, i);
            return;
        }
    }
}
''')
e(0x61cb30, 40, 'gti2SocketConnectionsThink', 'TableMapSafe2 over the connections with the time now and the static 0x61cae0 (a connection not closed thinks, stopping the map when that fails; one that is closed and not held is freed); 1 when every connection went through.', '''
static int gti2ConnectionThinkMap(void *elem, void *client_data) // 0x61cae0
{
    GTI2Connection *connection = *(GTI2Connection **)elem;
    unsigned long now = *(unsigned long *)client_data;

    if (connection->state != GTI2Closed && !gti2ConnectionThink(connection, now)) {
        return 0;
    }
    if (connection->state == GTI2Closed && connection->freeAtAcceptReject == 0 && connection->callbackLevel == 0) {
        gti2FreeClosedConnection(connection);
    }
    return 1;
}

int gti2SocketConnectionsThink(GTI2Socket *socket)
{
    unsigned long now = current_time();

    return TableMapSafe2(socket->connections, gti2ConnectionThinkMap, &now) == 0;
}
''')
e(0x61cb60, 50, 'gti2FreeClosedConnections', 'gti2FreeClosedConnection on every closed connection, last first.', '''
void gti2FreeClosedConnections(GTI2Socket *socket)
{
    int i;

    for (i = ArrayLength(socket->closedConnections) - 1; i >= 0; i--) {
        gti2FreeClosedConnection(*(GTI2Connection **)ArrayNth(socket->closedConnections, i));
    }
}
''')
e(0x61cba0, 49, 'gti2SocketError', 'once per socket: marks the error, hard-closes all connections (gt2CloseAllConnectionsHard 0x614780) and, when the error callback says to go on, frees the socket.', '''
extern void gt2CloseAllConnectionsHard(GTI2Socket *socket);

void gti2SocketError(GTI2Socket *socket)
{
    if (socket->error != 0) {
        return;
    }
    socket->error = 1;
    gt2CloseAllConnectionsHard(socket);
    if (gti2SocketErrorCallback(socket)) {
        gti2FreeSocket(socket);
    }
}
''')
e(0x61cbe0, 304, 'gti2SocketSend', '(socket, ip, port, message, len, arg6, arg7): 1 (nothing sent) when select says the socket cannot send; a failed sendto of WSAECONNRESET goes to gti2HandleConnectionReset (0 when that fails), WSAEMSGSIZE is ignored, anything else is a socket error (0). After a good send the send dump callback (when set) hears (socket, its connection, ip, port, 0, message, len, 1, arg6, arg7); 0 when that freed the socket.', '''
int gti2SocketSend(GTI2Socket *socket, unsigned int ip, unsigned short port, const unsigned char *message, int len,
    int arg6, int arg7)
{
    struct sockaddr_in address;

    gti2MessageCheck((const char **)&message, &len);
    if (!CanSendOnSocket(socket->socket)) {
        return 1;
    }
    memset(&address, 0, sizeof(address));
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = ip;
    address.sin_port = htons(port);
    if (sendto(socket->socket, (const char *)message, len, 0, (const struct sockaddr *)&address, 0x10) == SOCKET_ERROR) {
        int error = WSAGetLastError();

        if (error == WSAECONNRESET) {
            if (!gti2HandleConnectionReset(socket, ip, port)) {
                return 0;
            }
        } else if (error != WSAEMSGSIZE) {
            gti2SocketError(socket);
            return 0;
        }
    } else if (socket->sendDumpCallback != 0) {
        if (!gti2DumpCallback(socket, gti2SocketFindConnection(socket, ip, port), ip, port, 0, message, len, 1, arg6,
                arg7)) {
            return 0;
        }
    }
    return 1;
}
''')
e(0x61cd10, 55, 'gti2NewOutgoingConnection', 'gti2NewSocketConnection; a new one awaits the server challenge (state 0) and is initiated.', '''
int gti2NewOutgoingConnection(GTI2Socket *socket, GTI2Connection **connection, unsigned int ip, unsigned short port)
{
    int result = gti2NewSocketConnection(socket, connection, ip, port);

    if (result != 0) {
        return result;
    }
    (*connection)->state = GTI2AwaitingServerChallenge;
    (*connection)->initiated = 1;
    return 0;
}
''')
e(0x61cd50, 55, 'gti2NewIncomingConnection', 'gti2NewSocketConnection; a new one awaits the client challenge (state 2), not initiated.', '''
int gti2NewIncomingConnection(GTI2Socket *socket, GTI2Connection **connection, unsigned int ip, unsigned short port)
{
    int result = gti2NewSocketConnection(socket, connection, ip, port);

    if (result != 0) {
        return result;
    }
    (*connection)->state = GTI2AwaitingClientChallenge;
    (*connection)->initiated = 0;
    return 0;
}
''')
e(0x61cd90, 230, 'gti2StartConnectionAttempt', 'keeps a copy of a non-empty initial message (1 when out of memory) and the callbacks, makes a challenge, keeps the response it expects and sends the client challenge: 0 (state 0) or 3.', '''
int gti2StartConnectionAttempt(GTI2Connection *connection, const unsigned char *message, int len,
    const GT2ConnectionCallbacks *callbacks)
{
    unsigned char challenge[0x20];

    gti2MessageCheck((const char **)&message, &len);
    if (len > 0) {
        connection->initialMessage = (unsigned char *)malloc(len);
        if (connection->initialMessage == 0) {
            return 1;
        }
        memcpy(connection->initialMessage, message, len);
        connection->initialMessageLen = len;
    }
    if (callbacks != 0) {
        connection->callbacks = *callbacks;
    }
    gti2GetChallenge(challenge);
    gti2GetResponse(connection->response, challenge);
    if (gti2SendClientChallenge(connection, challenge)) {
        connection->state = GTI2AwaitingServerChallenge;
        return 0;
    }
    return 3;
}
''')
e(0x61ce80, 85, 'gt2Accept', 'clears freeAtAcceptReject; only a connection that was not so marked and awaits accept/reject is accepted: the accept (with our public key) goes out, it is connected (5) and takes the callbacks when given; 1, else 0.', '''
int gt2Accept(GTI2Connection *connection, const GT2ConnectionCallbacks *callbacks)
{
    int was_marked = connection->freeAtAcceptReject;

    connection->freeAtAcceptReject = 0;
    if (was_marked != 0 || connection->state != GTI2AwaitingAcceptReject) {
        return 0;
    }
    gti2SendAccept(connection);
    connection->state = GTI2Connected;
    if (callbacks != 0) {
        connection->callbacks = *callbacks;
    }
    return 1;
}
''')
e(0x61cee0, 61, 'gt2Reject', 'clears freeAtAcceptReject; a connection awaiting accept/reject is sent the reject message and closes (6).', '''
void gt2Reject(GTI2Connection *connection, const unsigned char *message, int len)
{
    connection->freeAtAcceptReject = 0;
    if (connection->state != GTI2AwaitingAcceptReject) {
        return;
    }
    gti2MessageCheck((const char **)&message, &len);
    gti2SendReject(connection, message, len);
    connection->state = GTI2Closing;
}
''')

# ------------------------------------------------------------------ gt2Connection.c
e(0x61cf20, 71, 'gti2ConnectionSendData', 'gti2SocketSend to the connection  address; after a send the last-send time is now. 0 when the send failed.', '''
int gti2ConnectionSendData(GTI2Connection *connection, const unsigned char *message, int len, int arg4, int arg5)
{
    if (!gti2SocketSend(connection->socket, connection->ip, connection->port, message, len, arg4, arg5)) {
        return 0;
    }
    connection->lastSend = current_time();
    return 1;
}
''')
e(0x61cf70, 119, 'gti2ResendMessages', 'EDI connection, stack now: an outgoing message resent more than 10 times is dropped; one unanswered for over a second is resent (0 when that fails).', '''
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
''', cc='EDI -> connection, stack -> now')
e(0x61cff0, 65, 'gti2ConnectionClosed', 'once: state 7, out of the connection table and onto the socket  closed list.', '''
void gti2ConnectionClosed(GTI2Connection *connection)
{
    if (connection->state == GTI2Closed) {
        return;
    }
    connection->state = GTI2Closed;
    TableRemove(connection->socket->connections, &connection);
    ArrayAppend(connection->socket->closedConnections, &connection);
}
''')
e(0x61d0d0, 82, 'gti2CheckTimeout', 'EAX now, ESI connection: a connection still being set up (state below 5) times out -- an initiated one after its timeout (never with 0), an incoming one still before accept/reject after 60 s: the peer is told it is closed, it is closed and the connected callback hears 6 (timed out); 0 when that freed the socket.', '''
int gti2CheckTimeout(GTI2Connection *connection, unsigned long now)
{
    if (connection->state >= GTI2Connected) {
        return 1;
    }
    if (connection->initiated != 0) {
        if (connection->timeout == 0 || now - connection->startTime <= connection->timeout) {
            return 1;
        }
    } else {
        if (connection->state >= GTI2AwaitingAcceptReject || now - connection->startTime <= 60000) {
            return 1;
        }
    }
    gti2ConnectionSendClosed(connection);
    gti2ConnectionClosed(connection);
    return gti2ConnectedCallback(connection, 6, 0, 0) != 0;
}
''', cc='EAX -> now, ESI -> connection')
e(0x61d130, 115, 'gti2ConnectionThink', 'timeouts; a keep-alive after 30 s without sending; resends; a pending ack older than 100 ms goes out. 0 when any of them fails.', '''
int gti2ConnectionThink(GTI2Connection *connection, unsigned long now)
{
    if (!gti2CheckTimeout(connection, now)) {
        return 0;
    }
    if (now - connection->lastSend > 30000 && !gti2SendKeepAlive(connection)) {
        return 0;
    }
    if (!gti2ResendMessages(connection, now)) {
        return 0;
    }
    if (connection->pendingAck != 0 && now - connection->pendingAckTime > 100 && !gti2SendAck(connection)) {
        return 0;
    }
    return 1;
}
''')
e(0x61d1b0, 71, 'gti2CloseConnection', 'hard: an open connection is closed at once, the peer told, the closed callback hears 0 (local close) and it is freed when not held. Soft: it goes to closing (6) and a close message is sent.', '''
void gti2CloseConnection(GTI2Connection *connection, int hard)
{
    if (hard != 0) {
        if (connection->state < GTI2Closed) {
            gti2ConnectionClosed(connection);
            gti2ConnectionSendClosed(connection);
            gti2ClosedCallback(connection, 0);
            gti2FreeClosedConnection(connection);
        }
        return;
    }
    connection->state = GTI2Closing;
    gti2SendClose(connection);
}
''')
e(0x61d370, 91, 'CanSendOnSocket', 'select with only the socket in the write set and a zero timeout: 1 when it is writable.', '''
int CanSendOnSocket(SOCKET sock)
{
    fd_set write_set;
    struct timeval timeout;
    int result;

    write_set.fd_count = 1;
    write_set.fd_array[0] = sock;
    timeout.tv_sec = 0;
    timeout.tv_usec = 0;
    result = select(0x40, 0, &write_set, 0, &timeout);
    if (result == SOCKET_ERROR || result == 0) {
        return 0;
    }
    return 1;
}
''')

# ------------------------------------------------------------------ gt2Callback.c
FREE = '''    if (socket->close != 0 && socket->callbackLevel == 0) {
        gti2FreeSocket(socket);
        return 0;
    }
    return 1;
'''
e(0x61d6c0, 69, 'gti2SocketErrorCallback', 'calls the socket error callback (when set) under a callback level; 0 when the socket was marked to close and is freed afterwards.', '''
int gti2SocketErrorCallback(GTI2Socket *socket)
{
    if (socket == 0 || socket->socketErrorCallback == 0) {
        return 1;
    }
    socket->callbackLevel++;
    socket->socketErrorCallback(socket);
    socket->callbackLevel--;
''' + FREE + '}\n')
e(0x61d710, 130, 'gti2ConnectAttemptCallback', 'the socket  connect-attempt callback (socket, connection, ip, port, latency, message, len; no message unless both are set) under socket and connection callback levels; 0 when the socket gets freed afterwards.', '''
int gti2ConnectAttemptCallback(GTI2Socket *socket, GTI2Connection *connection, unsigned int ip, unsigned short port,
    int latency, const unsigned char *message, int len)
{
    if (socket == 0 || connection == 0 || socket->connectAttemptCallback == 0) {
        return 1;
    }
    if (len == 0 || message == 0) {
        message = 0;
        len = 0;
    }
    socket->callbackLevel++;
    connection->callbackLevel++;
    socket->connectAttemptCallback(socket, connection, ip, port, latency, message, len);
    socket->callbackLevel--;
    connection->callbackLevel--;
''' + FREE + '}\n')
CB = '''    connection->callbackLevel++;
    connection->socket->callbackLevel++;
    %s;
    connection->callbackLevel--;
    connection->socket->callbackLevel--;
    socket = connection->socket;
''' + FREE
e(0x61d7a0, 132, 'gti2ConnectedCallback', 'records the connect result, then the connected callback (connection, result, message, len; no message unless both set) under the connection and socket callback levels; 0 when the socket gets freed afterwards.', '''
int gti2ConnectedCallback(GTI2Connection *connection, int result, const unsigned char *message, int len)
{
    GTI2Socket *socket;

    if (connection == 0) {
        return 1;
    }
    connection->connectionResult = result;
    if (connection->callbacks.connected == 0) {
        return 1;
    }
    if (len == 0 || message == 0) {
        message = 0;
        len = 0;
    }
''' + CB % 'connection->callbacks.connected(connection, result, message, len)' + '}\n')
e(0x61d830, 109, 'gti2ReceivedCallback', 'the received callback (connection, message, len, reliable) under callback levels; 0 when the socket gets freed afterwards.', '''
int gti2ReceivedCallback(GTI2Connection *connection, const unsigned char *message, int len, int reliable)
{
    GTI2Socket *socket;

    if (connection == 0 || connection->callbacks.received == 0) {
        return 1;
    }
    if (len == 0 || message == 0) {
        message = 0;
        len = 0;
    }
''' + CB % 'connection->callbacks.received(connection, message, len, reliable)' + '}\n')
e(0x61d8a0, 91, 'gti2ClosedCallback', 'the closed callback (connection, reason) under callback levels; 0 when the socket gets freed afterwards.', '''
int gti2ClosedCallback(GTI2Connection *connection, int reason)
{
    GTI2Socket *socket;

    if (connection == 0 || connection->callbacks.closed == 0) {
        return 1;
    }
''' + CB % 'connection->callbacks.closed(connection, reason)' + '}\n')
e(0x61d900, 91, 'gti2PingCallback', 'the ping callback (connection, latency) under callback levels; 0 when the socket gets freed afterwards.', '''
int gti2PingCallback(GTI2Connection *connection, int latency)
{
    GTI2Socket *socket;

    if (connection == 0 || connection->callbacks.ping == 0) {
        return 1;
    }
''' + CB % 'connection->callbacks.ping(connection, latency)' + '}\n')
for addr, name, field in ((0x61d960, 'gti2SendFilterCallback', 'sendFilters'), (0x61d9f0, 'gti2ReceiveFilterCallback', 'receiveFilters')):
    e(addr, 138, name, 'filter n of the connection  %s (none: 1) with (connection, n, message, len, reliable; no message unless both set) under callback levels; 0 when the socket gets freed afterwards.' % field, '''
int %s(GTI2Connection *connection, int filterID, const unsigned char *message, int len, int reliable)
{
    gt2FilterCallback *filter;
    GTI2Socket *socket;

    if (connection == 0) {
        return 1;
    }
    filter = (gt2FilterCallback *)ArrayNth(connection->%s, filterID);
    if (filter == 0) {
        return 1;
    }
    if (len == 0 || message == 0) {
        message = 0;
        len = 0;
    }
''' % (name, field) + CB % '(*filter)(connection, filterID, message, len, reliable)' + '}\n')
e(0x61da80, 153, 'gti2DumpCallback', '(socket, connection, ip, port, reset, message, len, send, arg9, arg10): the send dump callback (+0x24) for a send, else the receive one (+0x28); called with (socket, connection, ip, port, reset, message, len, arg9, arg10) -- no message unless both set -- under the socket and (when there is one) connection callback levels; 0 when the socket gets freed afterwards.', '''
int gti2DumpCallback(GTI2Socket *socket, GTI2Connection *connection, unsigned int ip, unsigned short port, int reset,
    const unsigned char *message, int len, int send, int arg9, int arg10)
{
    gt2DumpCallback callback;

    if (socket == 0) {
        return 1;
    }
    callback = send != 0 ? socket->sendDumpCallback : socket->receiveDumpCallback;
    if (callback == 0) {
        return 1;
    }
    if (len == 0 || message == 0) {
        message = 0;
        len = 0;
    }
    socket->callbackLevel++;
    if (connection != 0) {
        connection->callbackLevel++;
    }
    callback(socket, connection, ip, port, reset, message, len, arg9, arg10);
    socket->callbackLevel--;
    if (connection != 0) {
        connection->callbackLevel--;
    }
''' + FREE + '}\n')
e(0x61db20, 118, 'gti2UnrecognizedMessageCallback', 'clears *handled; the unrecognized-message callback (socket, ip, port, message, len; no message unless both set) under the socket callback level decides *handled; 0 when the socket gets freed afterwards.', '''
int gti2UnrecognizedMessageCallback(GTI2Socket *socket, unsigned int ip, unsigned short port,
    const unsigned char *message, int len, int *handled)
{
    *handled = 0;
    if (socket == 0 || socket->unrecognizedMessageCallback == 0) {
        return 1;
    }
    if (len == 0 || message == 0) {
        message = 0;
        len = 0;
    }
    socket->callbackLevel++;
    *handled = socket->unrecognizedMessageCallback(socket, ip, port, message, len);
    socket->callbackLevel--;
''' + FREE + '}\n')
print('ok')
