exec(open(r'C:\Users\Liam-\halo-re\scratchpad\gs_lib.py').read())
H = '#include "gt2.h"\n\n'


def e(addr, size, name, note, code, cc='cdecl'):
    emit(addr, size, name, note, H + code.lstrip('\n'), cc=cc)


e(0x6183b0, 101, 'gti2ConnectionError', 'ESI connection, stack result and reason. Before connected: an initiated connection closes and the connected callback hears the result (0 when that freed the socket); an incoming one closes (marked freeAtAcceptReject when it was awaiting accept/reject). Connected or closing: closes and the closed callback hears the reason. Already closed: nothing. Otherwise 1.', '''
int gti2ConnectionError(GTI2Connection *connection, int result, int reason)
{
    if (connection->state < GTI2Connected) {
        if (connection->initiated != 0) {
            gti2ConnectionClosed(connection);
            return gti2ConnectedCallback(connection, result, 0, 0) != 0;
        }
        if (connection->state == GTI2AwaitingAcceptReject) {
            connection->freeAtAcceptReject = 1;
        }
        gti2ConnectionClosed(connection);
        return 1;
    }
    if (connection->state == GTI2Closed) {
        return 1;
    }
    gti2ConnectionClosed(connection);
    return gti2ClosedCallback(connection, reason) != 0;
}
''', cc='ESI -> connection, stack -> result, reason')
e(0x618420, 209, 'gti2HandleAck', 'EAX connection, stack serial: notes the ack time; every outgoing message before the first whose serial is not older (16-bit) is dropped. When that empties the list the outgoing buffer length is reset, otherwise the remaining messages move down by the first one  start and the buffer is shortened to match.', '''
int gti2HandleAck(GTI2Connection *connection, unsigned short serialNumber)
{
    int count;
    int i;
    int shift;

    connection->lastAck = current_time();
    count = ArrayLength(connection->outgoingBufferMessages);
    if (count == 0) {
        return 1;
    }
    for (i = 0; i < count; i++) {
        GTI2OutgoingBufferMessage *message = (GTI2OutgoingBufferMessage *)ArrayNth(connection->outgoingBufferMessages, i);

        if ((short)(message->serialNumber - serialNumber) >= 0) {
            break;
        }
    }
    if (i == 0) {
        return 1;
    }
    do {
        i--;
        ArrayDeleteAt(connection->outgoingBufferMessages, i);
    } while (i != 0);
    count = ArrayLength(connection->outgoingBufferMessages);
    if (count == 0) {
        connection->outgoingBuffer.len = 0;
        return 1;
    }
    shift = ((GTI2OutgoingBufferMessage *)ArrayNth(connection->outgoingBufferMessages, 0))->start;
    for (i = 0; i < count; i++) {
        ((GTI2OutgoingBufferMessage *)ArrayNth(connection->outgoingBufferMessages, i))->start -= shift;
    }
    gti2BufferShorten(&connection->outgoingBuffer, 0, shift);
    return 1;
}
''', cc='EAX -> connection, stack -> serialNumber')
DEC = '''    tea_decrypt_buffer(len, message, (const uint32_t *)connection->key);
    datum_index_invalidate(&crc);
    crc32_update(&crc, message, len - 4);
    if (*(uint32_t *)(message + len - 4) != crc) {
        %s
    }
'''
e(0x618500, 161, 'gti2HandleUnreliableMessage', 'EBX connection, ESI message, stack len: only while connected or closing; the message is TEA-decrypted in place with the connection key and its trailing CRC32 checked (a bad one is dropped: 1); the rest goes to receive filter 0 when any are set, else the received callback, unreliable. 0 when that freed the socket.', '''
int gti2HandleUnreliableMessage(GTI2Connection *connection, unsigned char *message, int len)
{
    uint32_t crc;

    if (connection->state != GTI2Connected && connection->state != GTI2Closing) {
        return 1;
    }
''' + DEC % 'return 1;' + '''    if (ArrayLength(connection->receiveFilters) != 0) {
        return gti2ReceiveFilterCallback(connection, 0, message, len - 4, 0) != 0;
    }
    return gti2ReceivedCallback(connection, message, len - 4, 0) != 0;
}
''', cc='EBX -> connection, ESI -> message, stack -> len')
e(0x6185b0, 182, 'gti2HandleReliableData', 'EAX connection, EDI message, stack len: a reliable application message outside connected/closing is a negotiation error (7, communication error 2); otherwise decrypted and CRC-checked like gti2HandleUnreliableMessage (bad: dropped, 1) and delivered reliable.', '''
int gti2HandleReliableData(GTI2Connection *connection, unsigned char *message, int len)
{
    uint32_t crc;

    if (connection->state != GTI2Connected && connection->state != GTI2Closing) {
        return gti2ConnectionError(connection, 7, 2) != 0;
    }
''' + DEC % 'return 1;' + '''    if (ArrayLength(connection->receiveFilters) != 0) {
        return gti2ReceiveFilterCallback(connection, 0, message, len - 4, 1) != 0;
    }
    if (!gti2ReceivedCallback(connection, message, len - 4, 1)) {
        return 0;
    }
    return 1;
}
''', cc='EAX -> connection, EDI -> message, stack -> len')
e(0x618670, 173, 'gti2HandleAccept', 'ECX connection, stack data: only while awaiting acceptance (else a negotiation error); keeps the peer  16-byte public key, derives the shared key = peer ^ private mod modulus, is connected (5) and the connected callback hears success.', '''
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
''', cc='ECX -> connection, stack -> data')
e(0x618740, 125, 'gti2RemoveIncomingBufferMessage', 'EAX message, ECX connection, EDX index: deletes the entry (its start and length read first), moves every later message down by the length and cuts those bytes out of the incoming buffer.', '''
void gti2RemoveIncomingBufferMessage(GTI2Connection *connection, int index, const GTI2IncomingBufferMessage *message)
{
    int start = message->start;
    int len = message->len;
    int count;
    int i;

    ArrayDeleteAt(connection->incomingBufferMessages, index);
    count = ArrayLength(connection->incomingBufferMessages);
    for (i = 0; i < count; i++) {
        GTI2IncomingBufferMessage *other = (GTI2IncomingBufferMessage *)ArrayNth(connection->incomingBufferMessages, i);

        if (other->start > start) {
            other->start -= len;
        }
    }
    gti2BufferShorten(&connection->incomingBuffer, start, len);
}
''', cc='ECX -> connection, EDX -> index, EAX -> message')
e(0x6187c0, 32, 'gti2SetPendingAck', 'ESI connection: an ack becomes pending (from now) unless one already is.', '''
void gti2SetPendingAck(GTI2Connection *connection)
{
    if (connection->pendingAck == 0) {
        connection->pendingAck = 1;
        connection->pendingAckTime = current_time();
    }
}
''', cc='ESI -> connection')
e(0x6187e0, 67, 'gti2HandlePong', 'EDI connection, EAX data, stack len: with a ping callback, an 8-byte pong starting "time" reports the latency now - its time stamp.', '''
int gti2HandlePong(GTI2Connection *connection, const unsigned char *data, int len)
{
    if (connection->callbacks.ping == 0 || len != 8) {
        return 1;
    }
    if (*(const uint32_t *)data != *(const uint32_t *)GTI2PingTag) {
        return 1;
    }
    return gti2PingCallback(connection, (int)(current_time() - ((const uint32_t *)data)[1])) != 0;
}
''', cc='EDI -> connection, EAX -> data, stack -> len')
e(0x618830, 54, 'gti2HandleClosed', 'EAX connection: the peer says it is closed; unless already closed, an error with result 2 (rejected) and reason local close when we were closing, else remote close.', '''
int gti2HandleClosed(GTI2Connection *connection)
{
    if (connection->state == GTI2Closed) {
        return 1;
    }
    return gti2ConnectionError(connection, 2, connection->state != GTI2Closing) != 0;
}
''', cc='EAX -> connection')
e(0x618870, 138, 'gti2HandleConnectionReset', 'the receive dump callback (when set) hears a reset for the address; a connection there still awaiting the server challenge fails with timed out (6) once its timeout (never with 0) has passed; any later connection fails rejected (2); both as a remote close.', '''
int gti2HandleConnectionReset(GTI2Socket *socket, unsigned int ip, unsigned short port)
{
    GTI2Connection *connection = gti2SocketFindConnection(socket, ip, port);

    if (socket->receiveDumpCallback != 0 &&
        !gti2DumpCallback(socket, connection, ip, port, 1, 0, 0, 0, 0, 0)) {
        return 0;
    }
    if (connection == 0) {
        return 1;
    }
    if (connection->state == GTI2AwaitingServerChallenge) {
        if (connection->timeout == 0 || current_time() - connection->startTime < connection->timeout) {
            return 1;
        }
        return gti2ConnectionError(connection, 6, 1) != 0;
    }
    return gti2ConnectionError(connection, 2, 1) != 0;
}
''')
e(0x618900, 113, 'gti2AddOutgoingBufferMessage', 'ESI connection, EDX len, stack serial: appends a record (start at the outgoing buffer end, len, no resends, serial, sent now); 1 when it was added.', '''
int gti2AddOutgoingBufferMessage(GTI2Connection *connection, int len, unsigned short serialNumber)
{
    GTI2OutgoingBufferMessage message;
    int count;

    memset(&message, 0, sizeof(message));
    message.start = connection->outgoingBuffer.len;
    message.len = len;
    message.serialNumber = serialNumber;
    message.lastSend = current_time();
    message.resends = 0;
    count = ArrayLength(connection->outgoingBufferMessages);
    ArrayAppend(connection->outgoingBufferMessages, &message);
    return ArrayLength(connection->outgoingBufferMessages) == count + 1;
}
''', cc='ESI -> connection, EDX -> len, stack -> serialNumber')
e(0x618980, 64, 'gti2SendLastOutgoingMessage', 'ESI connection: sends the newest outgoing message from the buffer (dump args 1, 0); the pending ack rode along.', '''
int gti2SendLastOutgoingMessage(GTI2Connection *connection)
{
    GTI2OutgoingBufferMessage *message = (GTI2OutgoingBufferMessage *)ArrayNth(connection->outgoingBufferMessages,
        ArrayLength(connection->outgoingBufferMessages) - 1);

    if (!gti2ConnectionSendData(connection, connection->outgoingBuffer.buffer + message->start, message->len, 1, 0)) {
        return 0;
    }
    connection->pendingAck = 0;
    return 1;
}
''', cc='ESI -> connection')
e(0x6189c0, 165, 'gti2SendUnreliable', 'a message that itself starts with the 0xfe 0xfe magic is sent escaped behind another magic, built at the end of the outgoing buffer (not sent at all when it does not fit) and cut off again; anything else is sent as it is.', '''
int gti2SendUnreliable(GTI2Connection *connection, const unsigned char *message, int len)
{
    if (len >= 2 && *(const unsigned short *)message == *(const unsigned short *)GTI2Magic) {
        GTI2Buffer *buffer = &connection->outgoingBuffer;
        int total = len + 2;

        if (gti2GetBufferFreeSpace(buffer) >= total) {
            unsigned char *start = buffer->buffer + buffer->len;

            gti2BufferWriteData(buffer, GTI2Magic, 2);
            gti2BufferWriteData(buffer, message, len);
            if (!gti2ConnectionSendData(connection, start, total, 0, 0)) {
                return 0;
            }
            gti2BufferShorten(buffer, -1, total);
        }
        return 1;
    }
    return gti2ConnectionSendData(connection, message, len, 0, 0) != 0;
}
''')
e(0x618a70, 108, 'gti2SendAck', 'fe fe 64 and the expected serial (high byte first); the pending ack is cleared once it went.', '''
int gti2SendAck(GTI2Connection *connection)
{
    unsigned char packet[5];

    packet[0] = 0xfe;
    packet[1] = 0xfe;
    packet[2] = GTI2MsgAck;
    packet[3] = (unsigned char)(connection->expectedSerialNumber >> 8);
    packet[4] = (unsigned char)connection->expectedSerialNumber;
    if (!gti2ConnectionSendData(connection, packet, 5, 0, 0)) {
        return 0;
    }
    connection->pendingAck = 0;
    return 1;
}
''')
e(0x618ae0, 103, 'gti2SendNack', 'fe fe 65, the first missing serial and, when the range is longer than one, the last (high bytes first).', '''
int gti2SendNack(GTI2Connection *connection, unsigned short from, unsigned short to)
{
    unsigned char packet[7];
    int len = 5;

    packet[0] = 0xfe;
    packet[1] = 0xfe;
    packet[2] = GTI2MsgNack;
    packet[3] = (unsigned char)(from >> 8);
    packet[4] = (unsigned char)from;
    if (from != to) {
        packet[5] = (unsigned char)(to >> 8);
        packet[6] = (unsigned char)to;
        len = 7;
    }
    return gti2ConnectionSendData(connection, packet, len, 0, 0) != 0;
}
''')
e(0x618b50, 93, 'gti2SendClosed', 'fe fe 68 straight to the address (formatted with gt2AddressToString first, unused).', '''
int gti2SendClosed(GTI2Socket *socket, unsigned int ip, unsigned short port)
{
    unsigned char packet[3];
    char address[0x18];

    gt2AddressToString(ip, port, address);
    packet[0] = 0xfe;
    packet[1] = 0xfe;
    packet[2] = GTI2MsgClosed;
    return gti2SocketSend(socket, ip, port, packet, 3, 0, 0) != 0;
}
''')
e(0x618bb0, 125, 'gti2ResendMessage', 'rewrites the ack field (bytes 5-6) of the buffered message with the expected serial and sends it again (dump args 0, 1); then it counts a resend (the connection keeps the most), is stamped with the send time, and a server challenge (type 2) restarts the challenge clock.', '''
int gti2ResendMessage(GTI2Connection *connection, GTI2OutgoingBufferMessage *message)
{
    unsigned char *data = connection->outgoingBuffer.buffer + message->start;

    data[5] = (unsigned char)(connection->expectedSerialNumber >> 8);
    data[6] = (unsigned char)connection->expectedSerialNumber;
    if (!gti2ConnectionSendData(connection, connection->outgoingBuffer.buffer + message->start, message->len, 0, 1)) {
        return 0;
    }
    message->lastSend = connection->lastSend;
    message->resends++;
    if (message->resends > connection->maxResends) {
        connection->maxResends = message->resends;
    }
    if (connection->outgoingBuffer.buffer[message->start + 2] == GTI2MsgServerChallenge) {
        connection->challengeTime = connection->lastSend;
    }
    return 1;
}
''')
e(0x618c30, 294, 'gti2BufferIncomingMessage', 'EAX len, ESI connection, stack type, serial, message, overflow out: an out-of-order reliable message is kept (dropped when the incoming buffer or the sorted record list cannot take it): its record (start, len, type, serial) goes in by serial (static compare 0x618720), its bytes at the buffer end. When it is the only one, a nack asks for expected..serial-1; when it went in last, a gap before it is nacked. *overflow is always 0.', '''
static int gti2IncomingBufferMessageCompare(const void *elem1, const void *elem2) // 0x618720
{
    return (short)(((const GTI2IncomingBufferMessage *)elem1)->serialNumber -
                   ((const GTI2IncomingBufferMessage *)elem2)->serialNumber);
}

int gti2BufferIncomingMessage(GTI2Connection *connection, int len, int type, unsigned short serialNumber,
    const unsigned char *message, int *overflow)
{
    GTI2IncomingBufferMessage record;
    int count;

    if (gti2GetBufferFreeSpace(&connection->incomingBuffer) < len) {
        *overflow = 0;
        return 1;
    }
    record.start = connection->incomingBuffer.len;
    record.len = len;
    record.type = type;
    record.serialNumber = serialNumber;
    count = ArrayLength(connection->incomingBufferMessages);
    ArrayInsertSorted(connection->incomingBufferMessages, &record, gti2IncomingBufferMessageCompare);
    if (ArrayLength(connection->incomingBufferMessages) != count + 1) {
        *overflow = 0;
        return 1;
    }
    gti2BufferWriteData(&connection->incomingBuffer, message, len);
    if (count == 0) {
        if (!gti2SendNack(connection, connection->expectedSerialNumber, (unsigned short)(serialNumber - 1))) {
            return 0;
        }
    } else if (((GTI2IncomingBufferMessage *)ArrayNth(connection->incomingBufferMessages, count))->serialNumber ==
               serialNumber) {
        unsigned short previous =
            ((GTI2IncomingBufferMessage *)ArrayNth(connection->incomingBufferMessages, count - 1))->serialNumber;

        if ((unsigned short)(serialNumber - previous) > 1 &&
            !gti2SendNack(connection, (unsigned short)(previous + 1), (unsigned short)(serialNumber - 1))) {
            return 0;
        }
    }
    *overflow = 0;
    return 1;
}
''', cc='EAX -> len, ESI -> connection, stack -> type, serialNumber, message, overflow')
e(0x618d60, 161, 'gti2HandleNack', 'EAX data, ECX len, EDX connection: a 2-byte nack names one serial, a 4-byte one a range (anything else is a negotiation error); every outgoing message in the range (16-bit) is resent.', '''
int gti2HandleNack(GTI2Connection *connection, const unsigned char *data, int len)
{
    unsigned short from = (unsigned short)((data[0] << 8) | data[1]);
    unsigned short to;
    int count;
    int i;

    if (len == 2) {
        to = from;
    } else if (len == 4) {
        to = (unsigned short)((data[2] << 8) | data[3]);
    } else {
        return gti2ConnectionError(connection, 7, 2) != 0;
    }
    count = ArrayLength(connection->outgoingBufferMessages);
    for (i = 0; i < count; i++) {
        GTI2OutgoingBufferMessage *message = (GTI2OutgoingBufferMessage *)ArrayNth(connection->outgoingBufferMessages, i);

        if ((short)(message->serialNumber - from) >= 0 && (short)(message->serialNumber - to) <= 0 &&
            !gti2ResendMessage(connection, message)) {
            return 0;
        }
    }
    return 1;
}
''', cc='EDX -> connection, EAX -> data, ECX -> len')
e(0x618e10, 169, 'gti2HandleAdminMessage', 'EAX len, EBX message, EDX connection, stack type: ack (a 2-byte serial, else a negotiation error), nack, ping (answered in place as a pong), pong, closed; anything else is ignored.', '''
int gti2HandleAdminMessage(GTI2Connection *connection, unsigned char *message, int len, int type)
{
    unsigned char *data = message + 3;
    int data_len = len - 3;

    if (type == GTI2MsgAck) {
        if (data_len != 2) {
            return gti2ConnectionError(connection, 7, 2) != 0;
        }
        return gti2HandleAck(connection, (unsigned short)((data[0] << 8) | data[1])) != 0;
    }
    if (type == GTI2MsgNack) {
        return gti2HandleNack(connection, data, data_len) != 0;
    }
    if (type == GTI2MsgPing) {
        message[2] = GTI2MsgPong;
        return gti2ConnectionSendData(connection, message, len, 0, 0) != 0;
    }
    if (type == GTI2MsgPong) {
        return gti2HandlePong(connection, data, data_len) != 0;
    }
    if (type == GTI2MsgClosed) {
        return gti2HandleClosed(connection) != 0;
    }
    return 1;
}
''', cc='EDX -> connection, EBX -> message, EAX -> len, stack -> type')
e(0x618ec0, 27, 'gti2ConnectionSendClosed', 'gti2SendClosed to the connection  address.', '''
int gti2ConnectionSendClosed(GTI2Connection *connection)
{
    return gti2SendClosed(connection->socket, connection->ip, connection->port);
}
''')
e(0x618ee0, 45, 'gti2HandleOutOfMemory', 'EAX connection: tells the peer it is closed, then an error: out of memory (1), not enough memory (4).', '''
int gti2HandleOutOfMemory(GTI2Connection *connection)
{
    if (!gti2SendClosed(connection->socket, connection->ip, connection->port)) {
        return 0;
    }
    return gti2ConnectionError(connection, 1, 4);
}
''', cc='EAX -> connection')
e(0x618f10, 291, 'gti2HandleClientResponse', 'ECX len, EDX data, stack connection: while awaiting the client response, at least 0x30 bytes whose first 32 match the expected response: keeps the peer  public key (bytes 32..47) and derives the shared key. Without a connect-attempt callback the peer is told it is closed and the connection closes; otherwise it awaits accept/reject and the callback hears (latency since the challenge, the rest as the message). Anything else is a negotiation error.', '''
int gti2HandleClientResponse(GTI2Connection *connection, const unsigned char *data, int len)
{
    char hex[0x24];

    if (connection->state != GTI2AwaitingClientResponse || len < 0x30 || !gti2CheckResponse(data, connection->response)) {
        return gti2ConnectionError(connection, 7, 2) != 0;
    }
    memcpy(connection->remotePublicKey, data + 0x20, 0x10);
    gt2_bignum_to_hex(connection->remotePublicKey, hex);
    gt2_bignum_mod_exp(hex, connection->privateExponent, connection->modulus, connection->key);
    if (connection->socket->connectAttemptCallback == 0) {
        if (!gti2ConnectionSendClosed(connection)) {
            return 0;
        }
        gti2ConnectionClosed(connection);
        return 1;
    }
    connection->state = GTI2AwaitingAcceptReject;
    return gti2ConnectAttemptCallback(connection->socket, connection, connection->ip, connection->port,
               (int)(current_time() - connection->challengeTime), data + 0x30, len - 0x30) != 0;
}
''', cc='ECX -> len, EDX -> data, stack -> connection')
e(0x619040, 92, 'gti2HandleReject', 'EAX connection, stack message, len: only while awaiting acceptance (else a negotiation error); closes, tells the peer it is closed and the connected callback hears rejected (2) with the message.', '''
int gti2HandleReject(GTI2Connection *connection, const unsigned char *message, int len)
{
    if (connection->state != GTI2AwaitingAcceptance) {
        return gti2ConnectionError(connection, 7, 2) != 0;
    }
    gti2ConnectionClosed(connection);
    if (!gti2SendClosed(connection->socket, connection->ip, connection->port)) {
        return 0;
    }
    return gti2ConnectedCallback(connection, 2, message, len) != 0;
}
''', cc='EAX -> connection, stack -> message, len')
e(0x6190a0, 68, 'gti2HandleClose', 'EAX connection: answers with closed, then an error (2; reason local close when we were closing, else remote close).', '''
int gti2HandleClose(GTI2Connection *connection)
{
    if (!gti2SendClosed(connection->socket, connection->ip, connection->port)) {
        return 0;
    }
    return gti2ConnectionError(connection, 2, connection->state != GTI2Closing) != 0;
}
''', cc='EAX -> connection')
e(0x6190f0, 269, 'gti2BeginReliableMessage', 'EAX connection, EBX total length, stack type, overflow out: when the outgoing buffer or its record list cannot take the message the peer is told it is closed and the connection fails out of memory (1, 4), *overflow = 1. Otherwise the header goes in: fe fe, type, the next serial and the expected serial (high bytes first), *overflow = 0.', '''
int gti2BeginReliableMessage(GTI2Connection *connection, int len, int type, int *overflow)
{
    GTI2Buffer *buffer = &connection->outgoingBuffer;
    unsigned short serial;

    if (gti2GetBufferFreeSpace(buffer) < len || !gti2AddOutgoingBufferMessage(connection, len, connection->serialNumber)) {
        if (!gti2SendClosed(connection->socket, connection->ip, connection->port)) {
            return 0;
        }
        if (!gti2ConnectionError(connection, 1, 4)) {
            return 0;
        }
        *overflow = 1;
        return 1;
    }
    gti2BufferWriteData(buffer, GTI2Magic, 2);
    gti2BufferWriteByte(buffer, (unsigned char)type);
    serial = connection->serialNumber++;
    gti2BufferWriteUShort(buffer, serial);
    gti2BufferWriteUShort(buffer, connection->expectedSerialNumber);
    *overflow = 0;
    return 1;
}
''', cc='EAX -> connection, EBX -> len, stack -> type, overflow')
REL = '''    if (!gti2BeginReliableMessage(connection, %s, %s, &overflow)) {
        return 0;
    }
    if (overflow != 0) {
        return 1;
    }
'''
e(0x619200, 89, 'gti2SendDataReliable', 'a reliable application message (type 0, 7 header bytes) through the outgoing buffer.', '''
int gti2SendDataReliable(GTI2Connection *connection, const unsigned char *message, int len)
{
    int overflow;

''' + REL % ('len + 7', 'GTI2MsgAppReliable') + '''    gti2BufferWriteData(&connection->outgoingBuffer, message, len);
    return gti2SendLastOutgoingMessage(connection) != 0;
}
''')
e(0x619260, 88, 'gti2SendClientChallenge', 'type 1 with the 32-byte challenge.', '''
int gti2SendClientChallenge(GTI2Connection *connection, const unsigned char *challenge)
{
    int overflow;

''' + REL % ('0x27', 'GTI2MsgClientChallenge') + '''    gti2BufferWriteData(&connection->outgoingBuffer, challenge, 0x20);
    return gti2SendLastOutgoingMessage(connection) != 0;
}
''')
e(0x6192c0, 111, 'gti2SendServerChallenge', 'type 2 with the 32-byte response and our 32-byte challenge; the challenge clock starts at the send.', '''
int gti2SendServerChallenge(GTI2Connection *connection, const unsigned char *response, const unsigned char *challenge)
{
    int overflow;

''' + REL % ('0x47', 'GTI2MsgServerChallenge') + '''    gti2BufferWriteData(&connection->outgoingBuffer, response, 0x20);
    gti2BufferWriteData(&connection->outgoingBuffer, challenge, 0x20);
    if (!gti2SendLastOutgoingMessage(connection)) {
        return 0;
    }
    connection->challengeTime = connection->lastSend;
    return 1;
}
''')
e(0x619330, 119, 'gti2SendClientResponse', 'type 3 with the 32-byte response, our 16-byte public key and the initial message.', '''
int gti2SendClientResponse(GTI2Connection *connection, const unsigned char *response, const unsigned char *message,
    int len)
{
    int overflow;

''' + REL % ('len + 0x37', 'GTI2MsgClientResponse') + '''    gti2BufferWriteData(&connection->outgoingBuffer, response, 0x20);
    gti2BufferWriteData(&connection->outgoingBuffer, connection->publicKey, 0x10);
    gti2BufferWriteData(&connection->outgoingBuffer, message, len);
    return gti2SendLastOutgoingMessage(connection) != 0;
}
''')
e(0x6193b0, 90, 'gti2SendAccept', 'type 4 with our 16-byte public key.', '''
int gti2SendAccept(GTI2Connection *connection)
{
    int overflow;

''' + REL % ('0x17', 'GTI2MsgAccept') + '''    gti2BufferWriteData(&connection->outgoingBuffer, connection->publicKey, 0x10);
    return gti2SendLastOutgoingMessage(connection) != 0;
}
''')
e(0x619410, 89, 'gti2SendReject', 'type 5 with the message.', '''
int gti2SendReject(GTI2Connection *connection, const unsigned char *message, int len)
{
    int overflow;

''' + REL % ('len + 7', 'GTI2MsgReject') + '''    gti2BufferWriteData(&connection->outgoingBuffer, message, len);
    return gti2SendLastOutgoingMessage(connection) != 0;
}
''')
e(0x619470, 69, 'gti2SendClose', 'type 6, header only.', '''
int gti2SendClose(GTI2Connection *connection)
{
    int overflow;

''' + REL % ('7', 'GTI2MsgClose') + '''    return gti2SendLastOutgoingMessage(connection) != 0;
}
''')
e(0x6194c0, 69, 'gti2SendKeepAlive', 'type 7, header only.', '''
int gti2SendKeepAlive(GTI2Connection *connection)
{
    int overflow;

''' + REL % ('7', 'GTI2MsgKeepAlive') + '''    return gti2SendLastOutgoingMessage(connection) != 0;
}
''')
e(0x619510, 102, 'gti2Send', 'appends the CRC32 of the message (4 bytes PAST the given length -- the caller  buffer must have room), TEA-encrypts message + CRC in place with the connection key, and sends it reliable or unreliable.', '''
int gti2Send(GTI2Connection *connection, unsigned char *message, int len, int reliable)
{
    uint32_t crc;

    datum_index_invalidate(&crc);
    crc32_update(&crc, message, len);
    *(uint32_t *)(message + len) = crc;
    len += 4;
    tea_encrypt_buffer(len, message, (const uint32_t *)connection->key);
    if (reliable != 0) {
        return gti2SendDataReliable(connection, message, len);
    }
    return gti2SendUnreliable(connection, message, len);
}
''')
e(0x619580, 162, 'gti2HandleClientChallenge', 'ECX connection, stack challenge, len: while awaiting the client challenge with at least 32 bytes (else a negotiation error): answers with the response to it and a new challenge of our own (whose response we keep), then awaits the client response (3).', '''
int gti2HandleClientChallenge(GTI2Connection *connection, const unsigned char *challenge, int len)
{
    unsigned char our_challenge[0x20];
    unsigned char response[0x20];

    if (connection->state != GTI2AwaitingClientChallenge || len < 0x20) {
        return gti2ConnectionError(connection, 7, 2) != 0;
    }
    gti2GetResponse(response, challenge);
    gti2GetChallenge(our_challenge);
    gti2GetResponse(connection->response, our_challenge);
    if (!gti2SendServerChallenge(connection, response, our_challenge)) {
        return 0;
    }
    connection->state = GTI2AwaitingClientResponse;
    return 1;
}
''', cc='ECX -> connection, stack -> challenge, len')
e(0x619630, 183, 'gti2HandleServerChallenge', 'ECX data, EDX connection, stack len: while awaiting the server challenge, at least 64 bytes whose first 32 are the expected response (else a negotiation error): the client response (response to the server  challenge, our public key, the initial message) goes out, the initial message is freed and it awaits acceptance (1).', '''
int gti2HandleServerChallenge(GTI2Connection *connection, const unsigned char *data, int len)
{
    unsigned char response[0x20];

    if (connection->state != GTI2AwaitingServerChallenge || len < 0x40 || !gti2CheckResponse(data, connection->response)) {
        return gti2ConnectionError(connection, 7, 2) != 0;
    }
    gti2GetResponse(response, data + 0x20);
    if (!gti2SendClientResponse(connection, response, connection->initialMessage, connection->initialMessageLen)) {
        return 0;
    }
    if (connection->initialMessage != 0) {
        free(connection->initialMessage);
        connection->initialMessage = 0;
    }
    connection->state = GTI2AwaitingAcceptance;
    return 1;
}
''', cc='ECX -> data, EDX -> connection, stack -> len')
e(0x6196f0, 325, 'gti2DeliverReliableMessage', 'ESI connection, ECX data (past the 7-byte header), EBX length, stack type: counts the serial as received (expected + 1; the address is formatted, unused) and hands the message to its handler: 0 application data, 1 client challenge, 2 server challenge, 3 client response, 4 accept, 5 reject, 6 close; 7 (keep-alive) and others need nothing. 0 when the handler failed.', '''
int gti2DeliverReliableMessage(GTI2Connection *connection, int type, unsigned char *data, int len)
{
    char address[0x18];
    int result = 1;

    connection->expectedSerialNumber++;
    gt2AddressToString(connection->ip, connection->port, address);
    switch (type) {
    case GTI2MsgAppReliable:
        result = gti2HandleReliableData(connection, data, len);
        break;
    case GTI2MsgClientChallenge:
        result = gti2HandleClientChallenge(connection, data, len);
        break;
    case GTI2MsgServerChallenge:
        result = gti2HandleServerChallenge(connection, data, len);
        break;
    case GTI2MsgClientResponse:
        result = gti2HandleClientResponse(connection, data, len);
        break;
    case GTI2MsgAccept:
        result = gti2HandleAccept(connection, data);
        break;
    case GTI2MsgReject:
        result = gti2HandleReject(connection, data, len);
        break;
    case GTI2MsgClose:
        result = gti2HandleClose(connection);
        break;
    }
    return result != 0;
}
''', cc='ESI -> connection, ECX -> data, EBX -> len, stack -> type')
e(0x619840, 106, 'gti2DeliverQueuedMessages', 'EAX connection: while a buffered out-of-order message (searched last first) carries the serial now expected, it is delivered and removed; 0 when a delivery failed.', '''
int gti2DeliverQueuedMessages(GTI2Connection *connection)
{
    int i;

restart:
    for (i = ArrayLength(connection->incomingBufferMessages) - 1; i >= 0; i--) {
        GTI2IncomingBufferMessage *message =
            (GTI2IncomingBufferMessage *)ArrayNth(connection->incomingBufferMessages, i);

        if (message->serialNumber == connection->expectedSerialNumber) {
            if (!gti2DeliverReliableMessage(connection, message->type,
                    connection->incomingBuffer.buffer + message->start, message->len)) {
                return 0;
            }
            gti2RemoveIncomingBufferMessage(connection, i, message);
            goto restart;
        }
    }
    return 1;
}
''', cc='EAX -> connection')
e(0x6198b0, 253, 'gti2HandleReliableMessage', 'EAX len, ECX message, EDX connection, stack type: fewer than 7 bytes is a negotiation error. The header ack (bytes 5-6) is handled first. The expected serial (bytes 3-4) is delivered (an ack becomes pending) followed by any queued ones; an older one only makes an ack pending; a newer one is buffered (running out of memory there would close the connection).', '''
int gti2HandleReliableMessage(GTI2Connection *connection, unsigned char *message, int len, int type)
{
    unsigned short serial;
    unsigned short ack;
    unsigned char *data;
    int overflow;

    if (len < 7) {
        return gti2ConnectionError(connection, 7, 2) != 0;
    }
    serial = (unsigned short)((message[3] << 8) | message[4]);
    ack = (unsigned short)((message[5] << 8) | message[6]);
    data = message + 7;
    len -= 7;
    if (!gti2HandleAck(connection, ack)) {
        return 0;
    }
    if (serial == connection->expectedSerialNumber) {
        if (connection->pendingAck == 0) {
            connection->pendingAck = 1;
            connection->pendingAckTime = current_time();
        }
        if (!gti2DeliverReliableMessage(connection, type, data, len)) {
            return 0;
        }
        return gti2DeliverQueuedMessages(connection) != 0;
    }
    if ((short)(serial - connection->expectedSerialNumber) < 0) {
        gti2SetPendingAck(connection);
        return 1;
    }
    if (!gti2BufferIncomingMessage(connection, len, type, serial, data, &overflow)) {
        return 0;
    }
    if (overflow != 0 && !gti2HandleOutOfMemory(connection)) {
        return 0;
    }
    return 1;
}
''', cc='EDX -> connection, ECX -> message, EAX -> len, stack -> type')
e(0x6199b0, 476, 'gti2HandleMessage', 'EDX message, ECX port, stack socket, len, ip: the receive dump callback (when set) sees it first. From an unknown address: the unrecognized-message callback may take it; otherwise a GT2 client challenge starts an incoming connection when anyone listens (an existing address, 5, is ignored), a closed notice is ignored, and anything else is answered with closed. For a closed connection: answered with closed (unless it was a closed notice). Otherwise non-GT2 data (no fe fe magic) and escaped data (fe fe fe fe) are unreliable messages; types below 8 are reliable, the rest admin messages.', '''
int gti2HandleMessage(GTI2Socket *socket, unsigned char *message, int len, unsigned int ip, unsigned short port)
{
    GTI2Connection *connection = gti2SocketFindConnection(socket, ip, port);
    int is_gt2;
    int type;

    if (socket->receiveDumpCallback != 0 &&
        !gti2DumpCallback(socket, connection, ip, port, 0, message, len, 0, 0, 0)) {
        return 0;
    }
    is_gt2 = len > 2 && *(const unsigned short *)message == *(const unsigned short *)GTI2Magic;
    if (connection == 0) {
        char address[0x18];
        int handled;

        gt2AddressToString(ip, port, address);
        if (!gti2UnrecognizedMessageCallback(socket, ip, port, message, len, &handled)) {
            return 0;
        }
        if (handled != 0) {
            return 1;
        }
        if (is_gt2) {
            if (message[2] == GTI2MsgClientChallenge) {
                int result;

                if (socket->connectAttemptCallback == 0) {
                    return 1;
                }
                result = gti2NewIncomingConnection(socket, &connection, ip, port);
                if (result == 5) {
                    return 1;
                }
                if (result != 0) {
                    return gti2SendClosed(socket, ip, port) != 0;
                }
            } else if (message[2] == GTI2MsgClosed) {
                return 1;
            } else {
                return gti2SendClosed(socket, ip, port) != 0;
            }
        } else {
            return gti2SendClosed(socket, ip, port) != 0;
        }
    }
    if (connection->state == GTI2Closed) {
        if (is_gt2 && message[2] == GTI2MsgClosed) {
            return 1;
        }
        return gti2SendClosed(connection->socket, connection->ip, connection->port) != 0;
    }
    if (!is_gt2) {
        return gti2HandleUnreliableMessage(connection, message, len) != 0;
    }
    if (len >= 4 && *(const unsigned short *)(message + 2) == *(const unsigned short *)GTI2Magic) {
        return gti2HandleUnreliableMessage(connection, message + 2, len - 2) != 0;
    }
    type = message[2];
    if (type < 8) {
        return gti2HandleReliableMessage(connection, message, len, type) != 0;
    }
    return gti2HandleAdminMessage(connection, message, len, type) != 0;
}
''', cc='EDX -> message, ECX -> port, stack -> socket, len, ip')
e(0x619b90, 250, 'gti2ReceiveMessages', 'while the socket can receive: recvfrom into a 0xffff-byte stack buffer; WSAECONNRESET goes to gti2HandleConnectionReset, WSAEMSGSIZE is ignored, other errors are a socket error (0); data goes to gti2HandleMessage. 0 as soon as a handler fails.', '''
int gti2ReceiveMessages(GTI2Socket *socket)
{
    unsigned char buffer[0x10000];
    struct sockaddr_in address;
    int address_len;
    int len;

    while (CanReceiveOnSocket(socket->socket)) {
        address_len = 0x10;
        len = recvfrom(socket->socket, (char *)buffer, 0xffff, 0, (struct sockaddr *)&address, &address_len);
        if (len == SOCKET_ERROR) {
            int error = WSAGetLastError();

            if (error == WSAECONNRESET) {
                if (!gti2HandleConnectionReset(socket, address.sin_addr.s_addr, ntohs(address.sin_port))) {
                    return 0;
                }
            } else if (error != WSAEMSGSIZE) {
                gti2SocketError(socket);
                return 0;
            }
        } else if (!gti2HandleMessage(socket, buffer, len, address.sin_addr.s_addr, ntohs(address.sin_port))) {
            return 0;
        }
    }
    return 1;
}
''')
print('ok')
