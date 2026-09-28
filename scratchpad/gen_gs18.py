exec(open(r'C:\Users\Liam-\halo-re\scratchpad\gs_lib.py').read())
H = '#include "ghttp.h"\n\n'
TABLE = '''extern GHIConnection **ghiConnections;   // 0x006a3284
extern int ghiConnectionsLen;             // 0x006a327c
extern int ghiNumConnections;             // 0x006a3280
'''
PROXY = '''extern char *ghiProxyAddress;             // 0x007231e0
extern unsigned short ghiProxyPort;       // 0x007231dc
'''
THROTTLE = '''extern int ghiThrottleBufferSize;         // 0x00683dd4
extern unsigned long ghiThrottleTimeDelay; // 0x00683dd8
'''
LOCK = 'extern CRITICAL_SECTION *ghiLockHandle; // 0x006a328c\n'
BOUNDARY = 'Qr4G823s23d---<<><><<<>--7d118e0536'


def e(addr, size, name, note, code, cc='cdecl'):
    emit(addr, size, name, note, H + code.lstrip('\n'), cc=cc)


FAIL = '''    connection->completed = 1;
    connection->result = %s;
'''

# ------------------------------------------------------------------ buffers
e(0x622ab0, 49, 'ghiResizeBuffer', 'EDI buffer, EAX increment: grows the data by the increment (realloc); 0 for no buffer, a non-positive increment or no memory.', '''
int ghiResizeBuffer(GHIBuffer *buffer, int sizeIncrement)
{
    char *data;
    int size;

    if (buffer == 0 || sizeIncrement <= 0) {
        return 0;
    }
    size = buffer->size + sizeIncrement;
    data = (char *)realloc(buffer->data, size);
    if (data == 0) {
        return 0;
    }
    buffer->data = data;
    buffer->size = size;
    return 1;
}
''', cc='EDI -> buffer, EAX -> sizeIncrement')
e(0x622af0, 90, 'ghiInitBuffer', 'an empty growable buffer of the initial size (NUL at 0); 0 for bad arguments or no memory.', '''
int ghiInitBuffer(GHIConnection *connection, GHIBuffer *buffer, int initialSize, int sizeIncrement)
{
    if (connection == 0 || buffer == 0 || initialSize <= 0 || sizeIncrement <= 0) {
        return 0;
    }
    buffer->connection = connection;
    buffer->data = 0;
    buffer->size = 0;
    buffer->len = 0;
    buffer->pos = 0;
    buffer->sizeIncrement = sizeIncrement;
    buffer->fixed = 0;
    buffer->dontFree = 0;
    if (!ghiResizeBuffer(buffer, initialSize)) {
        return 0;
    }
    buffer->data[0] = 0;
    return 1;
}
''')
e(0x622b50, 75, 'ghiInitFixedBuffer', 'wraps the caller  buffer: fixed, never freed, empty (pos is left as it was).', '''
int ghiInitFixedBuffer(GHIConnection *connection, GHIBuffer *buffer, char *userBuffer, int size)
{
    if (connection == 0 || buffer == 0 || userBuffer == 0 || size <= 0) {
        return 0;
    }
    buffer->size = size;
    buffer->connection = connection;
    buffer->data = userBuffer;
    buffer->len = 0;
    buffer->sizeIncrement = 0;
    buffer->fixed = 1;
    buffer->dontFree = 1;
    userBuffer[0] = 0;
    return 1;
}
''')
e(0x622ba0, 59, 'ghiFreeBuffer', 'frees the data unless marked not to, and clears the buffer.', '''
void ghiFreeBuffer(GHIBuffer *buffer)
{
    if (buffer == 0 || buffer->data == 0) {
        return;
    }
    if (buffer->dontFree == 0) {
        free(buffer->data);
    }
    memset(buffer, 0, sizeof(*buffer));
}
''')
e(0x622be0, 231, 'ghiAppendDataToBuffer', 'appends (0 length: strlen) keeping a NUL after, growing by the increment while needed; a fixed buffer overflowing fails the connection with buffer overflow (2), no memory with out of memory (1).', '''
int ghiAppendDataToBuffer(GHIBuffer *buffer, const char *data, int dataLen)
{
    int newLen;

    if (buffer == 0 || data == 0 || dataLen < 0) {
        return 0;
    }
    if (dataLen == 0) {
        dataLen = (int)strlen(data);
    }
    newLen = buffer->len + dataLen;
    while (newLen >= buffer->size) {
        char *grown;
        int size;

        if (buffer->fixed != 0) {
            buffer->connection->completed = 1;
            buffer->connection->result = 2;
            return 0;
        }
        if (buffer->sizeIncrement <= 0) {
            goto out_of_memory;
        }
        size = buffer->size + buffer->sizeIncrement;
        grown = (char *)realloc(buffer->data, size);
        if (grown == 0) {
            goto out_of_memory;
        }
        buffer->data = grown;
        buffer->size = size;
    }
    memcpy(buffer->data + buffer->len, data, dataLen);
    buffer->len = newLen;
    buffer->data[newLen] = 0;
    return 1;

out_of_memory:
    buffer->connection->completed = 1;
    buffer->connection->result = 1;
    return 0;
}
''')
e(0x622cd0, 93, 'ghiAppendHeaderToBuffer', 'name ": " value CRLF.', '''
int ghiAppendHeaderToBuffer(GHIBuffer *buffer, const char *name, const char *value)
{
    return ghiAppendDataToBuffer(buffer, name, 0) && ghiAppendDataToBuffer(buffer, ": ", 2) &&
           ghiAppendDataToBuffer(buffer, value, 0) && ghiAppendDataToBuffer(buffer, "\\r\\n", 2);
}
''')
e(0x622d30, 120, 'ghiAppendCharToBuffer', 'one character and a NUL; growing by one increment when full (fixed: overflow 2; no memory: 1).', '''
int ghiAppendCharToBuffer(GHIBuffer *buffer, int c)
{
    if (buffer == 0) {
        return 0;
    }
    if (buffer->len + 1 >= buffer->size) {
        if (buffer->fixed != 0) {
            buffer->connection->completed = 1;
            buffer->connection->result = 2;
            return 0;
        }
        if (!ghiResizeBuffer(buffer, buffer->sizeIncrement)) {
            buffer->connection->completed = 1;
            buffer->connection->result = 1;
            return 0;
        }
    }
    buffer->data[buffer->len] = (char)c;
    buffer->len++;
    buffer->data[buffer->len] = 0;
    return 1;
}
''')
e(0x622db0, 62, 'ghiAppendIntToBuffer', 'the number in decimal.', '''
int ghiAppendIntToBuffer(GHIBuffer *buffer, int i)
{
    char text[0x10];

    sprintf(text, "%d", i);
    return ghiAppendDataToBuffer(buffer, text, 0);
}
''')
e(0x622df0, 18, 'ghiResetBuffer', 'empty again (len, pos 0, NUL).', '''
void ghiResetBuffer(GHIBuffer *buffer)
{
    buffer->len = 0;
    buffer->pos = 0;
    buffer->data[0] = 0;
}
''')
e(0x622e10, 134, 'ghiSendBufferedData', 'sends the connection  send buffer (the buffer argument is not used) while the socket is writable; select failure or a socket exception fails with socket failed (5). 1 once all is sent or the socket is busy, 0 on a send error.', '''
int ghiSendBufferedData(GHIBuffer *buffer, GHIConnection *connection)
{
    (void)buffer;
    for (;;) {
        int writeFlag;
        int exceptFlag;
        int sent;

        if (!ghiSocketSelect(connection->socket, 0, &writeFlag, &exceptFlag) || exceptFlag != 0) {
''' + FAIL % '5' + '''            connection->socketError = WSAGetLastError();
            return 0;
        }
        if (writeFlag == 0) {
            return 1;
        }
        sent = ghiDoSend(connection, connection->sendBuffer.data + connection->sendBuffer.pos,
            connection->sendBuffer.len - connection->sendBuffer.pos);
        if (sent == -1) {
            return 0;
        }
        connection->sendBuffer.pos += sent;
        if (connection->sendBuffer.pos >= connection->sendBuffer.len) {
            return 1;
        }
    }
}
''')

# ------------------------------------------------------------------ locks, sockets
e(0x621cd0, 39, 'ghiCreateLock', 'a malloc  critical section (NULL when out of memory).', LOCK + '''
void ghiCreateLock(void)
{
    CRITICAL_SECTION *lock = (CRITICAL_SECTION *)malloc(0x18);

    if (lock != 0) {
        InitializeCriticalSection(lock);
    }
    ghiLockHandle = lock;
}
''')
e(0x621d00, 40, 'ghiFreeLock', 'deletes and frees the critical section.', LOCK + '''
void ghiFreeLock(void)
{
    CRITICAL_SECTION *lock = ghiLockHandle;

    if (lock == 0) {
        return;
    }
    DeleteCriticalSection(lock);
    free(lock);
    ghiLockHandle = 0;
}
''')
e(0x621d30, 17, 'ghiLock', 'enters the critical section when there is one.', LOCK + '''
void ghiLock(void)
{
    if (ghiLockHandle != 0) {
        EnterCriticalSection(ghiLockHandle);
    }
}
''')
e(0x621d50, 17, 'ghiUnlock', 'leaves the critical section when there is one.', LOCK + '''
void ghiUnlock(void)
{
    if (ghiLockHandle != 0) {
        LeaveCriticalSection(ghiLockHandle);
    }
}
''')
e(0x621d70, 344, 'ghiSocketSelect', 'a zero-timeout select on the socket for the requested flags; 0 on error, else each requested flag says whether its set fired.', '''
int ghiSocketSelect(SOCKET socket, int *readFlag, int *writeFlag, int *exceptFlag)
{
    fd_set readSet;
    fd_set writeSet;
    fd_set exceptSet;
    fd_set *readSetPointer = 0;
    fd_set *writeSetPointer = 0;
    fd_set *exceptSetPointer = 0;
    struct timeval timeout;
    int result;

    if (readFlag != 0) {
        readSet.fd_array[0] = socket;
        readSet.fd_count = 1;
        readSetPointer = &readSet;
    }
    if (writeFlag != 0) {
        writeSet.fd_array[0] = socket;
        writeSet.fd_count = 1;
        writeSetPointer = &writeSet;
    }
    if (exceptFlag != 0) {
        exceptSet.fd_array[0] = socket;
        exceptSet.fd_count = 1;
        exceptSetPointer = &exceptSet;
    }
    timeout.tv_sec = 0;
    timeout.tv_usec = 0;
    result = select(0x40, readSetPointer, writeSetPointer, exceptSetPointer, &timeout);
    if (result == SOCKET_ERROR) {
        return 0;
    }
    if (readFlag != 0) {
        *readFlag = result > 0 && __WSAFDIsSet(socket, readSetPointer) ? 1 : 0;
    }
    if (writeFlag != 0) {
        *writeFlag = result > 0 && __WSAFDIsSet(socket, writeSetPointer) ? 1 : 0;
    }
    if (exceptFlag != 0) {
        *exceptFlag = result > 0 && __WSAFDIsSet(socket, exceptSetPointer) ? 1 : 0;
    }
    return 1;
}
''')
e(0x621ed0, 183, 'ghiDoReceive', 'recv into the buffer (room for a NUL; when throttled at most the throttle size, and nothing -- 1 -- until the throttle delay passed). 0 with data (NUL-terminated, length out), 1 would block, 2 closed by the peer, 3 error (socket failed 5).', THROTTLE + '''
int ghiDoReceive(GHIConnection *connection, char *buffer, int *bufferLen)
{
    int len = *bufferLen - 1;
    int received;

    if (connection->throttle != 0) {
        unsigned long now = current_time();

        if (now < connection->lastThrottleRecv + ghiThrottleTimeDelay) {
            return 1;
        }
        connection->lastThrottleRecv = now;
        if (len >= ghiThrottleBufferSize) {
            len = ghiThrottleBufferSize;
        }
    }
    received = recv(connection->socket, buffer, len, 0);
    if (received == SOCKET_ERROR) {
        int error = WSAGetLastError();

        if (error == WSAEWOULDBLOCK || error == WSAEINPROGRESS) {
            return 1;
        }
        connection->socketError = error;
        connection->completed = 1;
        connection->result = 5;
        connection->connectionClosed = 1;
        return 3;
    }
    if (received == 0) {
        connection->connectionClosed = 1;
        return 2;
    }
    buffer[received] = 0;
    *bufferLen = received;
    return 0;
}
''')
e(0x621f90, 86, 'ghiDoSend', 'send; would-block is 0 bytes, an error fails with socket failed (-1). While posting the bytes count as posted.', '''
int ghiDoSend(GHIConnection *connection, const char *buffer, int len)
{
    int sent = send(connection->socket, buffer, len, 0);

    if (sent == SOCKET_ERROR) {
        int error = WSAGetLastError();

        if (error == WSAEWOULDBLOCK) {
            return 0;
        }
        connection->socketError = error;
        connection->completed = 1;
        connection->result = 5;
        return -1;
    }
    if (connection->state == 3) {
        connection->bytesPosted += sent;
    }
    return sent;
}
''')
e(0x621ff0, 88, 'ghiTrySendThenBuffer', 'sends directly when nothing is queued (all sent: 1; error: 0); the rest is queued in the send buffer (2, or 0 when that fails).', '''
int ghiTrySendThenBuffer(GHIConnection *connection, const char *buffer, int len)
{
    int sent = 0;

    if (connection->sendBuffer.len == 0) {
        sent = ghiDoSend(connection, buffer, len);
        if (sent == -1) {
            return 0;
        }
        if (sent == len) {
            return 1;
        }
    }
    return ghiAppendDataToBuffer(&connection->sendBuffer, buffer + sent, len - sent) ? 2 : 0;
}
''')
print('ok')
