"""Local web server for the browser build of halo-re.

    python tools/serve_web.py --web build/web --halo "C:/Program Files (x86)/Microsoft Games/Halo" [--fx fx.bin] [--port 8080]

  /          the web build (halo.html, halo.js, halo.wasm)
  /halo/     the Halo install, read-only, case-insensitive, with range requests
  /halo-ui/  the game's loading-screen background, font and strings, extracted from the install
  /net       WebSocket virtual LAN (10.66.x.y) between connected pages; see src/platform/net_web.cpp

Listens on 127.0.0.1 by default; put an HTTPS reverse proxy in front to host it publicly.
"""
import argparse
import base64
import hashlib
import json
import mimetypes
import os
import posixpath
import re
import struct
import threading
import time
import urllib.parse
from http.server import SimpleHTTPRequestHandler, ThreadingHTTPServer

mimetypes.add_type('application/wasm', '.wasm')
mimetypes.add_type('text/javascript', '.js')

# Required for SharedArrayBuffer (threads).
ISOLATION_HEADERS = {
    'Cross-Origin-Opener-Policy': 'same-origin',
    'Cross-Origin-Embedder-Policy': 'require-corp',
    'Cross-Origin-Resource-Policy': 'same-origin',
}

QUIET = 'SERVE_WEB_QUIET' in os.environ
NET_LOG = 'SERVE_WEB_NETLOG' in os.environ

# Virtual LAN protocol (must match src/platform/net_web.cpp).
MSG_BIND, MSG_SENDTO, MSG_CLOSE = 1, 3, 6
MSG_HELLO, MSG_DATA, MSG_BOUND = 100, 101, 102

LAN_PREFIX = bytes([10, 66])
BROADCASTS = (bytes([255, 255, 255, 255]), LAN_PREFIX + bytes([255, 255]))
LOOPBACKS = (bytes([127, 0, 0, 1]), bytes([0, 0, 0, 0]))
EPHEMERAL_PORTS = range(49152, 65536)

# Per-page limits.
MAX_SOCKETS = 64
MAX_PAYLOAD = 8192
RATE_PACKETS = 3000   # per second
RATE_BYTES = 4 << 20  # per second

WS_GUID = '258EAFA5-E914-47DA-95CA-C5AB0DC85B11'
WS_BINARY, WS_CLOSE, WS_PING, WS_PONG = 0x2, 0x8, 0x9, 0xa


def net_log(text):
    if NET_LOG:
        print('net: ' + text, flush=True)


def ip(address):
    return '.'.join(str(b) for b in address)


# --- Files -----------------------------------------------------------------

def resolve_case_insensitive(root, relative):
    """Path of `relative` under `root`, matching each component ignoring case, or None."""
    path = root
    for part in filter(None, relative.split('/')):
        if part in ('.', '..'):
            return None
        exact = os.path.join(path, part)
        if os.path.exists(exact):
            path = exact
            continue
        try:
            match = next((n for n in os.listdir(path) if n.lower() == part.lower()), None)
        except OSError:
            return None
        if match is None:
            return None
        path = os.path.join(path, match)
    return path


def build_manifest(halo_root, fx_override):
    files = []
    for directory, _, names in os.walk(halo_root):
        for name in names:
            full = os.path.join(directory, name)
            path = os.path.relpath(full, halo_root).replace(os.sep, '/')
            size = os.path.getsize(full)
            if fx_override and path.lower() == 'shaders/fx.bin':
                size = os.path.getsize(fx_override)
            files.append({'path': path, 'size': size})
    files.sort(key=lambda f: f['path'].lower())
    return json.dumps({'files': files}).encode()


def ui_asset_files(halo_root):
    """The game's own loading-screen assets for the page (background, large UI font, loading strings), or none."""
    try:
        import halo_ui_assets
        assets = halo_ui_assets.extract(halo_root)
    except Exception as error:  # the page falls back to plain text
        print('halo-re: no UI assets for the page (%s)' % error)
        return {}
    font = {key: value for key, value in assets['font'].items() if key != 'atlas_png'}
    return {
        '/halo-ui/background.png': (assets['background_png'], 'image/png'),
        '/halo-ui/font.png': (assets['font']['atlas_png'], 'image/png'),
        '/halo-ui/font.json': (json.dumps(font).encode(), 'application/json'),
        '/halo-ui/loading.json': (json.dumps(assets['loading_strings']).encode(), 'application/json'),
    }


def parse_range(header, size):
    """(start, end) inclusive for a `bytes=` Range header; None if unsatisfiable; ValueError if malformed."""
    match = re.fullmatch(r'bytes=(\d*)-(\d*)', header.strip())
    if not match or match.group(1) == match.group(2) == '':
        raise ValueError(header)
    first, last = match.groups()
    if first == '':
        start, end = max(0, size - int(last)), size - 1
    else:
        start, end = int(first), min(int(last), size - 1) if last else size - 1
    if start > end or start >= size:
        return None
    return start, end


# --- Virtual LAN -----------------------------------------------------------

class Page:
    """One connected page: its address, its sockets (id -> port) and its WebSocket."""

    def __init__(self, address, wfile):
        self.address = address
        self.wfile = wfile
        self.sockets = {}
        self.send_lock = threading.Lock()
        self.window = time.monotonic()
        self.packets = 0
        self.bytes = 0

    def send(self, payload, opcode=WS_BINARY):
        size = len(payload)
        if size < 126:
            header = struct.pack('>BB', 0x80 | opcode, size)
        elif size < 65536:
            header = struct.pack('>BBH', 0x80 | opcode, 126, size)
        else:
            header = struct.pack('>BBQ', 0x80 | opcode, 127, size)
        with self.send_lock:
            try:
                self.wfile.write(header + payload)
                self.wfile.flush()
            except OSError:
                pass

    def allow(self, size):
        now = time.monotonic()
        if now - self.window >= 1.0:
            self.window, self.packets, self.bytes = now, 0, 0
        self.packets += 1
        self.bytes += size
        return self.packets <= RATE_PACKETS and self.bytes <= RATE_BYTES


class Router:
    """Routes datagrams between pages: (address, port) -> (page, socket id)."""

    def __init__(self):
        self.lock = threading.Lock()
        self.pages = {}
        self.ports = {}
        self.next_host = 1

    def join(self, wfile):
        with self.lock:
            for _ in range(65534):
                host = self.next_host
                self.next_host = host % 65533 + 1
                address = LAN_PREFIX + struct.pack('>H', host)
                if address not in self.pages and address not in BROADCASTS:
                    page = self.pages[address] = Page(address, wfile)
                    net_log('join %s' % ip(address))
                    return page
        return None

    def leave(self, page):
        with self.lock:
            for port in page.sockets.values():
                self.ports.pop((page.address, port), None)
            self.pages.pop(page.address, None)
        net_log('leave %s' % ip(page.address))

    def bind(self, page, socket_id, port):
        """Bind a socket (port 0 = any free port). Returns the port, or 0 on failure."""
        with self.lock:
            if socket_id not in page.sockets and len(page.sockets) >= MAX_SOCKETS:
                return 0
            old = page.sockets.get(socket_id)
            if old and port in (0, old):
                return old
            if port == 0:
                port = next((p for p in EPHEMERAL_PORTS if (page.address, p) not in self.ports), 0)
            if port == 0 or (page.address, port) in self.ports:
                return 0
            if old:
                self.ports.pop((page.address, old), None)
            page.sockets[socket_id] = port
            self.ports[(page.address, port)] = (page, socket_id)
            return port

    def close(self, page, socket_id):
        with self.lock:
            port = page.sockets.pop(socket_id, None)
            if port:
                self.ports.pop((page.address, port), None)

    def receivers(self, page, address, port):
        with self.lock:
            if address in LOOPBACKS:
                address = page.address
            if address in BROADCASTS:
                return [entry for (_, p), entry in self.ports.items() if p == port]
            entry = self.ports.get((address, port))
            return [entry] if entry else []

    def handle(self, page, data):
        if len(data) < 5:
            return
        kind, socket_id = struct.unpack_from('<BI', data)
        if kind == MSG_BIND and len(data) >= 7:
            port = self.bind(page, socket_id, struct.unpack_from('<H', data, 5)[0])
            net_log('bind %s socket %d -> port %d' % (ip(page.address), socket_id, port))
            page.send(struct.pack('<BIH', MSG_BOUND, socket_id, port))
        elif kind == MSG_SENDTO and len(data) >= 11:
            self.send_to(page, socket_id, data[5:9], struct.unpack_from('<H', data, 9)[0], data[11:])
        elif kind == MSG_CLOSE:
            self.close(page, socket_id)

    def send_to(self, page, socket_id, address, port, payload):
        if len(payload) > MAX_PAYLOAD or not page.allow(len(payload)):
            return
        if socket_id not in page.sockets:  # like a real socket, sending implicitly binds
            bound = self.bind(page, socket_id, 0)
            page.send(struct.pack('<BIH', MSG_BOUND, socket_id, bound))
            if bound == 0:
                return
        source_port = page.sockets[socket_id]
        receivers = self.receivers(page, address, port)
        net_log('%s:%d -> %s:%d %d bytes, %d receivers'
                % (ip(page.address), source_port, ip(address), port, len(payload), len(receivers)))
        header = struct.pack('<4sH', page.address, source_port)
        for receiver, receiver_socket in receivers:
            receiver.send(struct.pack('<BI', MSG_DATA, receiver_socket) + header + payload)


ROUTER = Router()


def read_frame(rfile):
    """(opcode, payload) of the next masked client frame, or (None, None) at end of stream."""
    head = rfile.read(2)
    if len(head) < 2:
        return None, None
    opcode, size = head[0] & 0x0f, head[1] & 0x7f
    if size == 126:
        size = struct.unpack('>H', rfile.read(2))[0]
    elif size == 127:
        size = struct.unpack('>Q', rfile.read(8))[0]
    if size > MAX_PAYLOAD + 64:
        return None, None
    mask = rfile.read(4)
    payload = bytearray(rfile.read(size))
    if len(mask) < 4 or len(payload) < size:
        return None, None
    for i in range(size):
        payload[i] ^= mask[i & 3]
    return opcode, bytes(payload)


# --- HTTP ------------------------------------------------------------------

class Handler(SimpleHTTPRequestHandler):
    protocol_version = 'HTTP/1.1'  # keep-alive: maps stream as many small range requests
    web_root = '.'
    halo_root = '.'
    fx_override = None
    virtual_files = {}  # request path -> (body, content type)

    def end_headers(self):
        for name, value in ISOLATION_HEADERS.items():
            self.send_header(name, value)
        self.send_header('Cache-Control', 'no-cache')
        super().end_headers()

    def log_message(self, format, *args):
        if not QUIET:
            super().log_message(format, *args)

    def do_GET(self):
        if urllib.parse.urlsplit(self.path).path == '/net':
            self.serve_websocket()
        else:
            self.serve(send_body=True)

    def do_HEAD(self):
        self.serve(send_body=False)

    def resolve(self, path):
        """Filesystem path for a request path, or None."""
        if path.lower() == '/halo/shaders/fx.bin' and self.fx_override:
            return self.fx_override
        if path == '/halo' or path.startswith('/halo/'):
            found = resolve_case_insensitive(self.halo_root, path[len('/halo'):])
        else:
            found = resolve_case_insensitive(self.web_root, path.lstrip('/') or 'halo.html')
        return found if found and os.path.isfile(found) else None

    def serve(self, send_body):
        path = posixpath.normpath(urllib.parse.unquote(urllib.parse.urlsplit(self.path).path))
        if path in self.virtual_files:
            body, content_type = self.virtual_files[path]
            self.send_response(200)
            self.send_header('Content-Type', content_type)
            self.send_header('Content-Length', str(len(body)))
            self.end_headers()
            if send_body:
                self.wfile.write(body)
            return

        file_path = self.resolve(path)
        if file_path is None:
            self.send_error(404)
            return

        size = os.path.getsize(file_path)
        start, end, status = 0, size - 1, 200
        if self.headers.get('Range'):
            try:
                byte_range = parse_range(self.headers['Range'], size)
            except ValueError:
                self.send_error(416)
                return
            if byte_range is None:
                self.send_response(416)
                self.send_header('Content-Range', 'bytes */%d' % size)
                self.send_header('Content-Length', '0')
                self.end_headers()
                return
            (start, end), status = byte_range, 206

        self.send_response(status)
        self.send_header('Content-Type', mimetypes.guess_type(file_path)[0] or 'application/octet-stream')
        self.send_header('Accept-Ranges', 'bytes')
        self.send_header('Content-Length', str(end - start + 1))
        if status == 206:
            self.send_header('Content-Range', 'bytes %d-%d/%d' % (start, end, size))
        self.end_headers()
        if send_body:
            self.copy_range(file_path, start, end - start + 1)

    def copy_range(self, file_path, start, length):
        with open(file_path, 'rb') as f:
            f.seek(start)
            while length > 0:
                chunk = f.read(min(length, 1 << 20))
                if not chunk:
                    break
                self.wfile.write(chunk)
                length -= len(chunk)

    def serve_websocket(self):
        key = self.headers.get('Sec-WebSocket-Key')
        origin = urllib.parse.urlsplit(self.headers.get('Origin', '')).netloc
        if self.headers.get('Upgrade', '').lower() != 'websocket' or not key or origin != self.headers.get('Host'):
            self.send_error(403)
            return
        accept = base64.b64encode(hashlib.sha1((key + WS_GUID).encode()).digest()).decode()
        self.send_response_only(101)
        self.send_header('Upgrade', 'websocket')
        self.send_header('Connection', 'Upgrade')
        self.send_header('Sec-WebSocket-Accept', accept)
        super().end_headers()  # skip isolation/cache headers on the upgrade
        self.close_connection = True

        page = ROUTER.join(self.wfile)
        if page is None:
            return
        page.send(bytes([MSG_HELLO]) + page.address)
        try:
            while True:
                opcode, payload = read_frame(self.rfile)
                if opcode is None or opcode == WS_CLOSE:
                    break
                if opcode == WS_PING:
                    page.send(payload, WS_PONG)
                elif opcode == WS_BINARY:
                    ROUTER.handle(page, payload)
        except OSError:
            pass
        finally:
            ROUTER.leave(page)


def main():
    parser = argparse.ArgumentParser(description=__doc__.split('\n\n')[0])
    parser.add_argument('--web', default='build/web', help='web build folder (halo.html, halo.js, halo.wasm)')
    parser.add_argument('--halo', required=True, help='installed Halo PC folder')
    parser.add_argument('--fx', help='converted fx.bin (tools/convert_fx.py) to serve as shaders/fx.bin')
    parser.add_argument('--host', default='127.0.0.1', help='address to listen on (keep the default behind a reverse proxy)')
    parser.add_argument('--port', type=int, default=8080)
    args = parser.parse_args()

    Handler.web_root = os.path.abspath(args.web)
    Handler.halo_root = os.path.abspath(args.halo)
    Handler.fx_override = os.path.abspath(args.fx) if args.fx else None
    Handler.virtual_files = {
        '/halo/manifest.json': (build_manifest(Handler.halo_root, Handler.fx_override), 'application/json'),
    }
    Handler.virtual_files.update(ui_asset_files(Handler.halo_root))

    server = ThreadingHTTPServer((args.host, args.port), Handler)
    print('halo-re: http://%s:%d/  (Halo files from %s)' % (args.host, args.port, Handler.halo_root))
    server.serve_forever()


if __name__ == '__main__':
    main()
