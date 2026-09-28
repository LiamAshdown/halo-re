exec(open(r'C:\Users\Liam-\halo-re\scratchpad\gen_gs18.py').read().split("FAIL = '''")[0].replace("exec(open(r'C:\\Users\\Liam-\\halo-re\\scratchpad\\gs_lib.py').read())", ''))
exec(open(r'C:\Users\Liam-\halo-re\scratchpad\gs_lib.py').read())
FAIL = '''    connection->completed = 1;
    connection->result = %s;
'''
PROGRESS = '    ghiCallProgressCallback(connection, 0, 0);\n'

# ------------------------------------------------------------------ connection table
e(0x6208a0, 164, 'ghiFindFreeSlot', 'the first unused connection, else the table grows by 4 fresh 0x11c-byte connections (the first new index); -1 when out of memory (new ones freed again).', TABLE + '''
int ghiFindFreeSlot(void)
{
    GHIConnection **grown;
    int oldLen = ghiConnectionsLen;
    int newLen;
    int i;

    for (i = 0; i < ghiConnectionsLen; i++) {
        if (ghiConnections[i]->inUse == 0) {
            return i;
        }
    }
    newLen = ghiConnectionsLen + 4;
    grown = (GHIConnection **)realloc(ghiConnections, newLen * 4);
    if (grown == 0) {
        return -1;
    }
    ghiConnections = grown;
    for (i = oldLen; i < newLen; i++) {
        ghiConnections[i] = (GHIConnection *)malloc(0x11c);
        if (ghiConnections[i] == 0) {
            for (i--; i >= oldLen; i--) {
                free(ghiConnections[i]);
            }
            return -1;
        }
        ghiConnections[i]->inUse = 0;
    }
    ghiConnectionsLen = newLen;
    return oldLen;
}
''')
e(0x620950, 259, 'ghiFreeConnection', 'for a live connection: under the lock frees its strings, closes its save file and socket (shutdown both ways), frees its buffers and posting state, frees an auto-free post, and marks it unused; 1, else 0.', TABLE + '''
int ghiFreeConnection(GHIConnection *connection)
{
    if (connection == 0 || connection->inUse == 0 || connection->request < 0 ||
        connection->request >= ghiConnectionsLen) {
        return 0;
    }
    ghiLock();
    free(connection->URL);
    free(connection->serverAddress);
    free(connection->requestPath);
    free(connection->sendHeaders);
    free(connection->redirectURL);
    if (connection->saveFile != 0) {
        fclose(connection->saveFile);
    }
    if (connection->socket != INVALID_SOCKET) {
        shutdown(connection->socket, 2);
        closesocket(connection->socket);
    }
    ghiFreeBuffer(&connection->sendBuffer);
    ghiFreeBuffer(&connection->recvBuffer);
    ghiFreeBuffer(&connection->getFileBuffer);
    if (connection->postingStates != 0) {
        ghiPostCleanupState(connection);
    }
    if (connection->post != 0 && ghiIsPostAutoFree(connection->post)) {
        ghttpFreePost(connection->post);
        connection->post = 0;
    }
    ghiNumConnections--;
    connection->inUse = 0;
    ghiUnlock();
    return 1;
}
''')
e(0x620a60, 55, 'ghiRequestToConnection', 'the live connection with that index (under the lock), else NULL.', TABLE + '''
GHIConnection *ghiRequestToConnection(int request)
{
    GHIConnection *connection;

    ghiLock();
    if (request < 0 || request >= ghiConnectionsLen) {
        ghiUnlock();
        return 0;
    }
    connection = ghiConnections[request];
    if (connection->inUse == 0) {
        connection = 0;
    }
    ghiUnlock();
    return connection;
}
''')
e(0x620aa0, 69, 'ghiEnumConnections', 'when any are live: the callback on every live connection, under the lock.', TABLE + '''
void ghiEnumConnections(int (*callback)(GHIConnection *connection))
{
    int i;

    if (ghiNumConnections <= 0) {
        return;
    }
    ghiLock();
    for (i = 0; i < ghiConnectionsLen; i++) {
        if (ghiConnections[i]->inUse != 0) {
            callback(ghiConnections[i]);
        }
    }
    ghiUnlock();
}
''')
e(0x620af0, 163, 'ghiRedirectConnection', 'restarts the request at the redirect URL: back to host lookup, the old address and path dropped, the socket shut and closed, buffers and status reset, one more redirect counted.', '''
void ghiRedirectConnection(GHIConnection *connection)
{
    connection->state = 0;
    free(connection->URL);
    connection->URL = connection->redirectURL;
    connection->redirectURL = 0;
    free(connection->serverAddress);
    connection->serverAddress = 0;
    connection->serverIP = 0;
    connection->serverPort = 0;
    free(connection->requestPath);
    connection->requestPath = 0;
    shutdown(connection->socket, 2);
    closesocket(connection->socket);
    connection->socket = INVALID_SOCKET;
    ghiResetBuffer(&connection->sendBuffer);
    ghiResetBuffer(&connection->recvBuffer);
    connection->statusMajorVersion = 0;
    connection->statusMinorVersion = 0;
    connection->statusCode = 0;
    connection->statusStringIndex = 0;
    connection->connectionClosed = 0;
    connection->redirectCount++;
}
''')
e(0x620ba0, 108, 'ghiCleanupConnections', 'frees every live connection, then every slot and the table.', TABLE + '''
static int ghiFreeConnectionEnum(GHIConnection *connection)
{
    return ghiFreeConnection(connection);
}

void ghiCleanupConnections(void)
{
    int i;

    if (ghiConnections == 0) {
        return;
    }
    ghiEnumConnections(ghiFreeConnectionEnum);
    for (i = 0; i < ghiConnectionsLen; i++) {
        free(ghiConnections[i]);
    }
    free(ghiConnections);
    ghiConnections = 0;
    ghiConnectionsLen = 0;
    ghiNumConnections = 0;
}
''')
e(0x620c10, 307, 'ghiNewConnection', 'under the lock: a free slot zeroed and in use, the next unique id, no socket, total size -1, a 2 KB send buffer (grow 4 KB) and a 2 KB receive buffer; NULL (freed again) when anything fails.', TABLE + '''
extern int ghiNextUniqueID;               // 0x006a3288

GHIConnection *ghiNewConnection(void)
{
    GHIConnection *connection;
    int index;

    ghiLock();
    index = ghiFindFreeSlot();
    if (index == -1) {
        ghiUnlock();
        return 0;
    }
    connection = ghiConnections[index];
    memset(connection, 0, 0x11c);
    connection->uniqueID = ghiNextUniqueID++;
    connection->inUse = 1;
    connection->request = index;
    connection->socket = INVALID_SOCKET;
    connection->totalSize = -1;
    if (ghiInitBuffer(connection, &connection->sendBuffer, 0x800, 0x1000) &&
        ghiInitBuffer(connection, &connection->recvBuffer, 0x800, 0x800)) {
        ghiNumConnections++;
        ghiUnlock();
        return connection;
    }
    ghiFreeConnection(connection);
    ghiUnlock();
    return 0;
}
''')

# ------------------------------------------------------------------ callbacks, URL
e(0x620d50, 78, 'ghiCallCompletedCallback', 'the completed callback (request, result, and for a get the file buffer and bytes received); a false answer keeps the buffer (not freed).', '''
void ghiCallCompletedCallback(GHIConnection *connection)
{
    char *buffer;
    int len;

    if (connection->completedCallback == 0) {
        return;
    }
    if (connection->type == 0) {
        buffer = connection->getFileBuffer.data;
        len = connection->fileBytesReceived;
    } else {
        buffer = 0;
        len = 0;
    }
    if (!connection->completedCallback(connection->request, connection->result, buffer, len, connection->callbackParam) &&
        buffer != 0) {
        connection->getFileBuffer.dontFree = 1;
    }
}
''')
e(0x620da0, 53, 'ghiCallProgressCallback', 'the progress callback (request, state, buffer, len, bytes received, total size, param).', '''
void ghiCallProgressCallback(GHIConnection *connection, const char *buffer, int bufferLen)
{
    if (connection->progressCallback != 0) {
        connection->progressCallback(connection->request, connection->state, buffer, bufferLen,
            connection->fileBytesReceived, connection->totalSize, connection->callbackParam);
    }
}
''')
e(0x620de0, 71, 'ghiCallPostCallback', 'the post callback (request, bytes posted, total bytes, objects posted, object count) with the connection  callback param (not the post  own).', '''
void ghiCallPostCallback(GHIConnection *connection)
{
    if (connection->postCallback != 0) {
        connection->postCallback(connection->request, connection->bytesPosted, connection->totalBytes,
            connection->objectsPosted, ArrayLength(connection->postingStates), connection->callbackParam);
    }
}
''')
e(0x620e30, 212, 'ghiParseURL', 'an "http://" URL: the host (copied), the port after ":" (0 fails; default 80) and the path from the first "/" ("/" when none) with spaces turned into "+".', '''
int ghiParseURL(GHIConnection *connection)
{
    char *url;
    char *end;
    char save;
    char *path;
    char *space;

    if (connection == 0 || connection->URL == 0 || strncmp(connection->URL, "http://", 7) != 0) {
        return 0;
    }
    url = connection->URL + 7;
    end = url + strcspn(url, ":/");
    save = *end;
    *end = 0;
    connection->serverAddress = _strdup(url);
    if (connection->serverAddress == 0) {
        return 0;
    }
    *end = save;
    path = end;
    if (*path == ':') {
        path++;
        connection->serverPort = (unsigned short)atoi(path);
        if (connection->serverPort == 0) {
            return 0;
        }
        do {
            path++;
            if (*path == 0) {
                path = "/";
                goto have_path;
            }
        } while (*path != '/');
    } else {
        connection->serverPort = 80;
    }
    if (*path == 0) {
        path = "/";
    }
have_path:
    connection->requestPath = _strdup(path);
    for (space = strchr(connection->requestPath, ' '); space != 0; space = strchr(connection->requestPath, ' ')) {
        *space = '+';
    }
    return connection->requestPath != 0;
}
''')

# ------------------------------------------------------------------ process
e(0x61bb60, 102, 'ghiHandleStatus', 'ESI connection: an HTTP error status becomes the result -- 401 unauthorized (9), 403 forbidden (10), 404/410 file not found (11), other 4xx server error (8), 5xx server error 12.', '''
void ghiHandleStatus(GHIConnection *connection)
{
    switch (connection->statusCode / 100) {
    case 4:
        switch (connection->statusCode) {
        case 401:
            connection->result = 9;
            break;
        case 403:
            connection->result = 10;
            break;
        case 404:
        case 410:
            connection->result = 11;
            break;
        default:
            connection->result = 8;
            break;
        }
        break;
    case 5:
        connection->result = 12;
        break;
    }
}
''', cc='ESI -> connection')
e(0x61bc00, 243, 'ghiProcessConnection', 'unless already inside, runs every state step in order (lookup, connect, send, post, wait, status, headers, file), follows a pending redirect; once complete: the status sets the result, the save file closes, the completed callback runs and the connection is freed (the completed flag returned); else 0.', '''
int ghiProcessConnection(GHIConnection *connection)
{
    int completed;

    if (connection->processing != 0) {
        return 0;
    }
    connection->processing = 1;
    if (connection->state == 0) {
        ghiDoHostLookup(connection);
    }
    if (connection->state == 1) {
        ghiDoConnecting(connection);
    }
    if (connection->state == 2) {
        ghiDoSendingRequest(connection);
    }
    if (connection->state == 3) {
        ghiDoPosting(connection);
    }
    if (connection->state == 4) {
        ghiDoWaiting(connection);
    }
    if (connection->state == 5) {
        ghiDoReceivingStatus(connection);
    }
    if (connection->state == 6) {
        ghiDoReceivingHeaders(connection);
    }
    if (connection->state == 7) {
        ghiDoReceivingFile(connection);
    }
    if (connection->redirectURL != 0) {
        ghiRedirectConnection(connection);
    }
    completed = connection->completed;
    if (completed == 0) {
        connection->processing = 0;
        return 0;
    }
    ghiHandleStatus(connection);
    if (connection->saveFile != 0) {
        fclose(connection->saveFile);
        connection->saveFile = 0;
    }
    ghiCallCompletedCallback(connection);
    ghiFreeConnection(connection);
    return completed;
}
''')
e(0x620f10, 143, 'ghiDoHostLookup', 'progress, SocketStartUp, parse the URL (parse failed 3), resolve the proxy or the server (lookup failed 4); then connecting (1).', PROXY + '''
void ghiDoHostLookup(GHIConnection *connection)
{
    const char *host;

''' + PROGRESS + '''    SocketStartUp();
    if (!ghiParseURL(connection)) {
''' + FAIL % '3' + '''        return;
    }
    host = ghiProxyAddress != 0 ? ghiProxyAddress : connection->serverAddress;
    connection->serverIP = inet_addr(host);
    if (connection->serverIP == INADDR_NONE) {
        struct hostent *entry = gethostbyname(host);

        if (entry == 0) {
''' + FAIL % '4' + '''            return;
        }
        connection->serverIP = *(unsigned int *)entry->h_addr_list[0];
    }
    connection->state = 1;
''' + PROGRESS + '''}
''')
e(0x620fa0, 371, 'ghiDoConnecting', 'first time: a non-blocking TCP socket (socket failed 5), a throttled receive buffer, connect to the proxy or server port (anything but would-block / in-progress: connect failed 6). Then select: writable means sending (2); an exception or select failure fails with 6.', PROXY + THROTTLE + '''
extern int SetSockBlocking(SOCKET sock, int is_blocking);
extern int SetReceiveBufferSize(SOCKET sock, int size);

void ghiDoConnecting(GHIConnection *connection)
{
    int writeFlag;
    int exceptFlag;
    int result;

    if (connection->socket == INVALID_SOCKET) {
        struct sockaddr_in address;

        connection->socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (connection->socket == INVALID_SOCKET || !SetSockBlocking(connection->socket, 0)) {
''' + FAIL % '5' + '''            connection->socketError = WSAGetLastError();
            return;
        }
        if (connection->throttle != 0) {
            SetReceiveBufferSize(connection->socket, ghiThrottleBufferSize);
        }
        memset(&address, 0, sizeof(address));
        address.sin_family = AF_INET;
        address.sin_port = htons(ghiProxyAddress != 0 ? ghiProxyPort : connection->serverPort);
        address.sin_addr.s_addr = connection->serverIP;
        if (connect(connection->socket, (const struct sockaddr *)&address, 0x10) == SOCKET_ERROR) {
            int error = WSAGetLastError();

            if (error != WSAEWOULDBLOCK && error != WSAEINPROGRESS) {
''' + FAIL % '6' + '''                connection->socketError = error;
                return;
            }
        }
    }
    result = ghiSocketSelect(connection->socket, 0, &writeFlag, &exceptFlag);
    if (result != 0 && exceptFlag == 0) {
        if (writeFlag == 0) {
            return;
        }
        connection->state = 2;
''' + PROGRESS + '''        return;
    }
''' + FAIL % '6' + '''    if (result == 0) {
        connection->socketError = WSAGetLastError();
    }
}
''')
e(0x621120, 424, 'ghiDoSendingRequest', 'builds the request once (POST / HEAD / GET, the proxy gets the full URL, HTTP/1.1, Host (with the port unless 80), User-Agent GameSpyHTTP/1.0, Connection close, for a post Content-Length and Content-Type, the caller  headers, blank line) and sends what it can; when all went out: posting (3) or waiting (4).', PROXY + '''
void ghiDoSendingRequest(GHIConnection *connection)
{
    GHIBuffer *buffer = &connection->sendBuffer;

    if (buffer->len == 0) {
        const char *method;

        if (connection->post != 0) {
            method = "POST ";
        } else if (connection->type == 3) {
            method = "HEAD ";
        } else {
            method = "GET ";
        }
        ghiAppendDataToBuffer(buffer, method, 0);
        ghiAppendDataToBuffer(buffer, ghiProxyAddress != 0 ? connection->URL : connection->requestPath, 0);
        ghiAppendDataToBuffer(buffer, " HTTP/1.1\\r\\n", 0);
        if (connection->serverPort == 80) {
            ghiAppendHeaderToBuffer(buffer, "Host", connection->serverAddress);
        } else {
            ghiAppendDataToBuffer(buffer, "Host: ", 0);
            ghiAppendDataToBuffer(buffer, connection->serverAddress, 0);
            ghiAppendCharToBuffer(buffer, ':');
            ghiAppendIntToBuffer(buffer, connection->serverPort);
            ghiAppendDataToBuffer(buffer, "\\r\\n", 2);
        }
        ghiAppendHeaderToBuffer(buffer, "User-Agent", "GameSpyHTTP/1.0");
        ghiAppendHeaderToBuffer(buffer, "Connection", "close");
        if (connection->post != 0) {
            char length[0x10];

            sprintf(length, "%d", connection->totalBytes);
            ghiAppendHeaderToBuffer(buffer, "Content-Length", length);
            ghiAppendHeaderToBuffer(buffer, "Content-Type", ghiPostGetContentType(connection));
        }
        if (connection->sendHeaders != 0) {
            ghiAppendDataToBuffer(buffer, connection->sendHeaders, 0);
        }
        ghiAppendDataToBuffer(buffer, "\\r\\n", 2);
    }
    if (ghiSendBufferedData(buffer, connection) && buffer->pos >= buffer->len) {
        ghiResetBuffer(buffer);
        connection->state = connection->post != 0 ? 3 : 4;
''' + PROGRESS + '''    }
}
''')
e(0x6212d0, 93, 'ghiDoPosting', 'posts what it can: a failure cleans the posting state; progress in bytes reaches the post callback; when done the state is cleaned and it waits (4).', '''
void ghiDoPosting(GHIConnection *connection)
{
    int bytesPosted = connection->bytesPosted;
    int result = ghiPostDoPosting(connection);

    if (result == 0) {
        ghiPostCleanupState(connection);
        return;
    }
    if (bytesPosted != connection->bytesPosted) {
        ghiCallPostCallback(connection);
    }
    if (result == 1) {
        ghiPostCleanupState(connection);
        connection->state = 4;
''' + PROGRESS + '''    }
}
''')
e(0x621330, 87, 'ghiDoWaiting', 'readable: receiving the status (5); select failure fails with socket failed (5).', '''
void ghiDoWaiting(GHIConnection *connection)
{
    int readFlag;

    if (!ghiSocketSelect(connection->socket, &readFlag, 0, 0)) {
''' + FAIL % '5' + '''        connection->socketError = WSAGetLastError();
        return;
    }
    if (readFlag != 0) {
        connection->state = 5;
''' + PROGRESS + '''    }
}
''')
e(0x621390, 193, 'ghiParseStatus', 'ESI connection: "HTTP/%d.%d %d%n" from the receive buffer, the reason text start after spaces; a major version of at least 1 and a code in 100..599 is kept (1), anything else is a bad response (7).', '''
int ghiParseStatus(GHIConnection *connection)
{
    int major;
    int minor;
    int code;
    int index;
    int count = sscanf(connection->recvBuffer.data, "HTTP/%d.%d %d%n", &major, &minor, &code, &index);

    while (connection->recvBuffer.data[index] != 0 && isspace(connection->recvBuffer.data[index])) {
        index++;
    }
    if (count == 3 && major >= 1 && code >= 100 && code < 600) {
        connection->statusMajorVersion = major;
        connection->statusCode = code;
        connection->statusMinorVersion = minor;
        connection->statusStringIndex = index;
        return 1;
    }
''' + FAIL % '7' + '''    return 0;
}
''', cc='ESI -> connection')
e(0x621460, 228, 'ghiDoReceivingStatus', 'receives up to 1 KB into the receive buffer (closed: bad response 7); once a line is there it is cut, parsed as the status line, and the headers follow (6) from after it.', '''
void ghiDoReceivingStatus(GHIConnection *connection)
{
    char buffer[0x400];
    int len = 0x400;
    int result = ghiDoReceive(connection, buffer, &len);
    char *end;

    if (result == 3) {
        return;
    }
    if (result == 2) {
''' + FAIL % '7' + '''        connection->socketError = WSAGetLastError();
        return;
    }
    if (result == 0 && !ghiAppendDataToBuffer(&connection->recvBuffer, buffer, len)) {
        return;
    }
    end = strstr(connection->recvBuffer.data, "\\r\\n");
    if (end == 0) {
        return;
    }
    *end = 0;
    if (ghiParseStatus(connection)) {
        connection->recvBuffer.pos = (int)(end - connection->recvBuffer.data) + 2;
        connection->state = 6;
''' + PROGRESS + '''    }
}
''')
e(0x621550, 208, 'ghiProcessIncomingFileData', 'EAX length, EBX data, ESI connection: counts the bytes (all of the total, or a closed connection, completes it); a get appends to the file buffer, a save writes the file (short write: file write failed 13), a stream passes it through; progress. Other types report progress with the binary  stale ECX (here NULL, 0).', '''
int ghiProcessIncomingFileData(GHIConnection *connection, char *buffer, int len)
{
    connection->fileBytesReceived += len;
    if (connection->fileBytesReceived == connection->totalSize || connection->connectionClosed != 0) {
        connection->completed = 1;
    }
    if (connection->type == 0) {
        if (!ghiAppendDataToBuffer(&connection->getFileBuffer, buffer, len)) {
            return 0;
        }
        ghiCallProgressCallback(connection, connection->getFileBuffer.data, connection->getFileBuffer.len);
        return 1;
    }
    if (connection->type == 1) {
        if ((int)fwrite(buffer, 1, len, connection->saveFile) != len) {
''' + FAIL % '0xd' + '''            return 0;
        }
        ghiCallProgressCallback(connection, buffer, len);
        return 1;
    }
    if (connection->type == 2) {
        ghiCallProgressCallback(connection, buffer, len);
        return 1;
    }
    ghiCallProgressCallback(connection, 0, 0);
    return 1;
}
''', cc='EAX -> len, EBX -> buffer, ESI -> connection')
e(0x621620, 90, 'ghiChunkHeaderAppend', 'EAX connection, EDX length, stack data: keeps up to 10 chunk-header characters (NUL-terminated).', '''
void ghiChunkHeaderAppend(GHIConnection *connection, const char *buffer, int len)
{
    if (len == 0 || connection->chunkHeaderLen >= 10) {
        return;
    }
    if (10 - connection->chunkHeaderLen < len) {
        len = 10 - connection->chunkHeaderLen;
    }
    memcpy(connection->chunkHeader + connection->chunkHeaderLen, buffer, len);
    connection->chunkHeaderLen += len;
    connection->chunkHeader[connection->chunkHeaderLen] = 0;
}
''', cc='EAX -> connection, EDX -> len, stack -> buffer')
e(0x621680, 503, 'ghiProcessIncomingChunkedData', 'EAX data, stack connection, length: without chunked encoding straight to the file data; otherwise the chunk state machine: header line (hex size; bad: bad response 7; 0 means done), data, trailing line, done (completed).', '''
int ghiProcessIncomingChunkedData(GHIConnection *connection, char *buffer, int len)
{
    if (connection->chunked == 0) {
        return ghiProcessIncomingFileData(connection, buffer, len);
    }
    while (len > 0) {
        if (connection->chunkReadingState == 0) {
            char *newline = strchr(buffer, '\\n');
            int n;
            int size;

            if (newline == 0) {
                ghiChunkHeaderAppend(connection, buffer, len);
                return 1;
            }
            n = (int)(newline - buffer) - 1;
            if (n != 0 && connection->chunkHeaderLen < 10) {
                if (10 - connection->chunkHeaderLen < n) {
                    n = 10 - connection->chunkHeaderLen;
                }
                memcpy(connection->chunkHeader + connection->chunkHeaderLen, buffer, n);
                connection->chunkHeaderLen += n;
                connection->chunkHeader[connection->chunkHeaderLen] = 0;
            }
            len -= (int)(newline + 1 - buffer);
            buffer = newline + 1;
            if (sscanf(connection->chunkHeader, "%x", &size) != 1) {
                size = -1;
            }
            connection->chunkBytesLeft = size;
            if (size == -1) {
''' + FAIL % '7' + '''                return 0;
            }
            connection->chunkReadingState = size == 0 ? 3 : 1;
        } else if (connection->chunkReadingState == 1) {
            int n = len;

            if (connection->chunkBytesLeft < n) {
                n = connection->chunkBytesLeft;
            }
            if (!ghiProcessIncomingFileData(connection, buffer, n)) {
                return 0;
            }
            len -= n;
            buffer += n;
            connection->chunkBytesLeft -= n;
            if (connection->chunkBytesLeft == 0) {
                connection->chunkReadingState = 2;
            }
        } else if (connection->chunkReadingState == 2) {
            char *newline = strchr(buffer, '\\n');

            if (newline == 0) {
                return 1;
            }
            len -= (int)(newline + 1 - buffer);
            buffer = newline + 1;
            connection->chunkHeader[0] = 0;
            connection->chunkHeaderLen = 0;
            connection->chunkBytesLeft = 0;
            connection->chunkReadingState = 0;
        } else {
            if (connection->chunkReadingState != 3) {
                return 0;
            }
            connection->completed = 1;
            return 1;
        }
    }
    return 1;
}
''', cc='EAX -> buffer, stack -> connection, len')
e(0x621880, 924, 'ghiDoReceivingHeaders', 'receives up to 4 KB of headers until a blank line ("\\\\r\\\\n\\\\r\\\\n", else "\\\\n\\\\n" with the same offsets; closed before it: bad response 7). 1xx: the rest is kept and the status is read again (5). 3xx: a Location becomes the redirect (absolute, or "http://host:port" + path; over 10 redirects: 11; no memory: 1). Otherwise Content-Length and chunked encoding are noted; HEAD-like types (3, 4) and an empty body complete, else the body so far goes to the file data (receiving the file, 7).', '''
void ghiDoReceivingHeaders(GHIConnection *connection)
{
    char buffer[0x1000];
    int len = 0x1000;
    int result = ghiDoReceive(connection, buffer, &len);
    char *headers;
    char *end;
    char *data;
    char *body;
    int bodyLen;
    int oldLen;
    char *contentLength;

    if (result == 3) {
        return;
    }
    if (result == 1) {
        if (connection->recvBuffer.pos == connection->recvBuffer.len) {
            return;
        }
    } else if (result == 0) {
        if (!ghiAppendDataToBuffer(&connection->recvBuffer, buffer, len)) {
            return;
        }
    }
    headers = connection->recvBuffer.data + connection->recvBuffer.pos;
    end = strstr(headers, "\\r\\n\\r\\n");
    if (end == 0) {
        end = strstr(headers, "\\n\\n");
    }
    if (end == 0) {
        if (result == 2) {
''' + FAIL % '7' + '''            connection->socketError = WSAGetLastError();
        }
        return;
    }
    end += 2;
    *end = 0;
    data = connection->recvBuffer.data;
    oldLen = connection->recvBuffer.len;
    body = end + 2;
    connection->recvBuffer.len = (int)(end - data);
    bodyLen = oldLen - (int)(body - data);
    if (connection->statusCode / 100 == 1) {
        if (bodyLen != 0) {
            memmove(data, body, bodyLen + 1);
            connection->recvBuffer.len = bodyLen;
            connection->recvBuffer.pos = 0;
        } else {
            ghiResetBuffer(&connection->recvBuffer);
        }
        connection->state = 5;
''' + PROGRESS + '''        return;
    }
    if (connection->statusCode / 100 == 3) {
        char *location;

        if (connection->redirectCount > 10) {
''' + FAIL % '0xb' + '''            return;
        }
        location = strstr(headers, "Location:");
        if (location != 0) {
            char *stop;

            location += 9;
            while (isspace(*location)) {
                location++;
            }
            stop = location;
            while (*stop != 0 && !isspace(*stop)) {
                stop++;
            }
            *stop = 0;
            if (*location == '/') {
                connection->redirectURL = (char *)malloc(strlen(connection->serverAddress) + strlen(location) + 14);
                if (connection->redirectURL == 0) {
''' + FAIL.replace('    ', '        ') % '1' + '''                }
                sprintf(connection->redirectURL, "http://%s:%d%s", connection->serverAddress, connection->serverPort,
                    location);
                return;
            }
            connection->redirectURL = _strdup(location);
            if (connection->redirectURL == 0) {
''' + FAIL.replace('    ', '        ') % '1' + '''            }
            return;
        }
    }
    contentLength = strstr(headers, "Content-Length:");
    if (contentLength != 0) {
        connection->totalSize = atoi(contentLength + 0x10);
    }
    connection->chunked = strstr(headers, "Transfer-Encoding: chunked") != 0;
    if (connection->chunked != 0) {
        connection->chunkHeader[0] = 0;
        connection->chunkHeaderLen = 0;
        connection->chunkBytesLeft = 0;
        connection->chunkReadingState = 0;
    }
    if (connection->type == 3 || connection->type == 4) {
        connection->completed = 1;
        return;
    }
    connection->state = 7;
    if (contentLength != 0 && connection->totalSize == 0) {
        connection->completed = 1;
        return;
    }
    if (bodyLen > 0) {
        ghiProcessIncomingChunkedData(connection, body, bodyLen);
    }
}
''')
e(0x621c20, 166, 'ghiDoReceivingFile', 'until complete: receive up to 8 KB and hand it to the (chunked) file data; nothing waiting or an error stops, the peer closing completes.', '''
void ghiDoReceivingFile(GHIConnection *connection)
{
    char buffer[0x2000];

    if (connection->completed != 0) {
        return;
    }
    for (;;) {
        int len = 0x2000;
        int result = ghiDoReceive(connection, buffer, &len);

        if (result == 3 || result == 1) {
            return;
        }
        if (result == 2) {
            connection->completed = 1;
            return;
        }
        if (!ghiProcessIncomingChunkedData(connection, buffer, len)) {
            return;
        }
        if (connection->completed != 0) {
            return;
        }
    }
}
''')
print('ok')
