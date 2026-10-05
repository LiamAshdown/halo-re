"""Local web server for the browser build of halo-re (local use only).

    python tools/serve_web.py --web build/web --halo "C:/Program Files (x86)/Microsoft Games/Halo" [--port 8080]

Serves the web build (halo.html, .js, .wasm) at / and the installed Halo folder, read-only, at /halo/. The browser
streams game files from /halo/ as the game opens them: HTTP range requests fetch just the parts of a .map file that
are read. /halo/manifest.json lists every file with its size so the page can create the game's file tree up front.
Paths under /halo/ match case-insensitively (the game asks for maps\\ where the install has MAPS\\).
/halo/digital_product_id.bin is the install's DigitalProductID registry value (the product key check reads it; the
browser has no registry), read from this machine's registry and only ever sent to this machine.
--fx serves a converted shader file (tools/convert_fx.py) as shaders/fx.bin, in place of the install's 2003 one,
as the Windows loader's file override does.

/net is the game's network: a WebSocket per page, routing datagrams between the pages connected to this server as one
virtual LAN (10.66.x.y addresses, broadcasts included). Nothing is sent outside it (src/platform/net_web.cpp is the
page side and describes the messages).

Every response carries the cross-origin isolation headers browsers require before a page may use threads
(SharedArrayBuffer). The server listens on 127.0.0.1 only.
"""
import argparse
import base64
import hashlib
import json
import struct
import threading
import time
import mimetypes
import os
import posixpath
import re
import urllib.parse
from http.server import SimpleHTTPRequestHandler, ThreadingHTTPServer

ISOLATION_HEADERS = {
    'Cross-Origin-Opener-Policy': 'same-origin',
    'Cross-Origin-Embedder-Policy': 'require-corp',
    'Cross-Origin-Resource-Policy': 'same-origin',
}
mimetypes.add_type('application/wasm', '.wasm')
mimetypes.add_type('text/javascript', '.js')


def resolve_case_insensitive(root, relative):
    """The path of relative under root, matching each component without regard to case; None when there is none."""
    path = root
    for part in [p for p in relative.split('/') if p]:
        if part in ('.', '..'):
            return None
        exact = os.path.join(path, part)
        if os.path.exists(exact):
            path = exact
            continue
        try:
            match = next((name for name in os.listdir(path) if name.lower() == part.lower()), None)
        except OSError:
            return None
        if match is None:
            return None
        path = os.path.join(path, match)
    return path


def digital_product_id():
    """The installed Halo's DigitalProductID registry value (bytes), or None (not Windows, or not installed)."""
    try:
        import winreg
    except ImportError:
        return None
    for view in (winreg.KEY_WOW64_32KEY, winreg.KEY_WOW64_64KEY):
        try:
            key = winreg.OpenKey(winreg.HKEY_LOCAL_MACHINE, r'Software\Microsoft\Microsoft Games\Halo', 0, winreg.KEY_READ | view)
            value, _ = winreg.QueryValueEx(key, 'DigitalProductID')
            return bytes(value)
        except OSError:
            continue
    return None


def build_manifest(halo_root, fx_override, product_id):
    files = []
    for directory, _, names in os.walk(halo_root):
        for name in names:
            full = os.path.join(directory, name)
            relative = os.path.relpath(full, halo_root).replace(os.sep, '/')
            files.append({'path': relative, 'size': os.path.getsize(full)})
    if fx_override:
        for f in files:
            if f['path'].lower() == 'shaders/fx.bin':
                f['size'] = os.path.getsize(fx_override)
    if product_id:
        files.append({'path': 'digital_product_id.bin', 'size': len(product_id)})
    files.sort(key=lambda f: f['path'].lower())
    return json.dumps({'files': files}).encode()


MSG_BIND, MSG_SENDTO, MSG_CLOSE = 1, 3, 6
MSG_HELLO, MSG_DATA, MSG_BOUND = 100, 101, 102
LAN_PREFIX = bytes([10, 66])
BROADCASTS = (bytes([255, 255, 255, 255]), LAN_PREFIX + bytes([255, 255]))
LOOPBACKS = (bytes([127, 0, 0, 1]), bytes([0, 0, 0, 0]))
MAX_SOCKETS = 64          # per page
MAX_PAYLOAD = 8192        # bytes per datagram
RATE_PACKETS = 3000       # per page per second
RATE_BYTES = 4 << 20      # per page per second


class Page:
    """One connected page: its virtual address, its sockets (id -> port) and its outgoing WebSocket."""

    def __init__(self, address, wfile):
        self.address = address
        self.wfile = wfile
        self.sockets = {}
        self.send_lock = threading.Lock()
        self.window = time.monotonic()
        self.packets = 0
        self.bytes = 0

    def send(self, payload):
        header = bytes([0x82])  # final frame, binary
        size = len(payload)
        if size < 126:
            header += bytes([size])
        elif size < 65536:
            header += bytes([126]) + struct.pack('>H', size)
        else:
            header += bytes([127]) + struct.pack('>Q', size)
        with self.send_lock:
            try:
                self.wfile.write(header + payload)
                self.wfile.flush()
            except OSError:
                pass

    def allow(self, size):
        """Rate limit: False once this second's packet or byte budget is spent."""
        now = time.monotonic()
        if now - self.window >= 1.0:
            self.window, self.packets, self.bytes = now, 0, 0
        self.packets += 1
        self.bytes += size
        return self.packets <= RATE_PACKETS and self.bytes <= RATE_BYTES


class Router:
    """The virtual LAN: (address, port) -> (page, socket id)."""

    def __init__(self):
        self.lock = threading.Lock()
        self.pages = {}
        self.ports = {}
        self.next_host = 1

    def join(self, wfile):
        with self.lock:
            for _ in range(65534):
                host = self.next_host
                self.next_host = self.next_host % 65533 + 1
                address = LAN_PREFIX + struct.pack('>H', host)
                if address not in self.pages and address not in BROADCASTS:
                    page = Page(address, wfile)
                    self.pages[address] = page
                    return page
        return None

    def leave(self, page):
        with self.lock:
            for port in page.sockets.values():
                self.ports.pop((page.address, port), None)
            self.pages.pop(page.address, None)

    def bind(self, page, socket_id, port):
        """Binds (to any free port when port is 0); returns the port, 0 when it is taken."""
        with self.lock:
            if socket_id not in page.sockets and len(page.sockets) >= MAX_SOCKETS:
                return 0
            old = page.sockets.get(socket_id)
            if old and (port == 0 or old == port):
                return old
            if port == 0:
                port = next((p for p in range(49152, 65536) if (page.address, p) not in self.ports), 0)
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

    def targets(self, page, address, port):
        with self.lock:
            if address in LOOPBACKS:
                address = page.address
            if address in BROADCASTS:
                return [entry for (a, p), entry in self.ports.items() if p == port]
            entry = self.ports.get((address, port))
            return [entry] if entry else []

    def message(self, page, data):
        if len(data) < 5:
            return
        kind, socket_id = data[0], struct.unpack_from('<I', data, 1)[0]
        if kind == MSG_BIND and len(data) >= 7:
            port = self.bind(page, socket_id, struct.unpack_from('<H', data, 5)[0])
            page.send(struct.pack('<BIH', MSG_BOUND, socket_id, port))
        elif kind == MSG_SENDTO and len(data) >= 11:
            payload = data[11:]
            if len(payload) > MAX_PAYLOAD or not page.allow(len(payload)):
                return
            if socket_id not in page.sockets:  # sending first binds an ephemeral port, as on a real socket
                port = self.bind(page, socket_id, 0)
                page.send(struct.pack('<BIH', MSG_BOUND, socket_id, port))
                if port == 0:
                    return
            source = struct.pack('<4sH', page.address, page.sockets[socket_id])
            target, port = data[5:9], struct.unpack_from('<H', data, 9)[0]
            for receiver, receiver_socket in self.targets(page, target, port):
                receiver.send(struct.pack('<BI', MSG_DATA, receiver_socket) + source + payload)
        elif kind == MSG_CLOSE:
            self.close(page, socket_id)


ROUTER = Router()


def read_frame(rfile):
    """(opcode, payload) of the next client frame (always masked), or (None, None) at the end."""
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


class Handler(SimpleHTTPRequestHandler):
    protocol_version = 'HTTP/1.1'  # keep-alive: the game streams a map as many small range requests
    web_root = '.'
    halo_root = '.'
    fx_override = None
    product_id = None
    manifest = b''

    def end_headers(self):
        for name, value in ISOLATION_HEADERS.items():
            self.send_header(name, value)
        self.send_header('Cache-Control', 'no-cache')
        super().end_headers()

    def log_message(self, format, *args):
        if os.environ.get('SERVE_WEB_QUIET') is None:
            super().log_message(format, *args)

    def target(self):
        """(file path, served) for the request path, or (None, None)."""
        path = urllib.parse.unquote(urllib.parse.urlsplit(self.path).path)
        path = posixpath.normpath(path)
        if path == '/halo/manifest.json':
            return None, 'manifest'
        if path == '/halo/digital_product_id.bin' and self.product_id:
            return None, 'product_id'
        if path.lower() == '/halo/shaders/fx.bin' and self.fx_override:
            return self.fx_override, 'file'
        if path == '/halo' or path.startswith('/halo/'):
            found = resolve_case_insensitive(self.halo_root, path[len('/halo'):])
            return (found, 'file') if found and os.path.isfile(found) else (None, None)
        relative = path.lstrip('/') or 'halo.html'
        found = resolve_case_insensitive(self.web_root, relative)
        return (found, 'file') if found and os.path.isfile(found) else (None, None)

    def do_HEAD(self):
        self.respond(send_body=False)

    def do_GET(self):
        if urllib.parse.urlsplit(self.path).path == '/net':
            self.network()
        else:
            self.respond(send_body=True)

    def network(self):
        """The page's end of the virtual LAN: a WebSocket from a page this server served."""
        key = self.headers.get('Sec-WebSocket-Key')
        origin = urllib.parse.urlsplit(self.headers.get('Origin', '')).netloc
        if self.headers.get('Upgrade', '').lower() != 'websocket' or not key or origin != self.headers.get('Host'):
            self.send_error(403)
            return
        accept = base64.b64encode(hashlib.sha1((key + '258EAFA5-E914-47DA-95CA-C5AB0DC85B11').encode()).digest()).decode()
        self.send_response_only(101)
        self.send_header('Upgrade', 'websocket')
        self.send_header('Connection', 'Upgrade')
        self.send_header('Sec-WebSocket-Accept', accept)
        super().end_headers()  # no isolation or cache headers on the upgrade
        page = ROUTER.join(self.wfile)
        self.close_connection = True
        if page is None:
            return
        page.send(bytes([MSG_HELLO]) + page.address)
        try:
            while True:
                opcode, payload = read_frame(self.rfile)
                if opcode is None or opcode == 8:
                    break
                if opcode == 9:
                    with page.send_lock:
                        self.wfile.write(bytes([0x8a, len(payload)]) + payload)
                elif opcode == 2:
                    ROUTER.message(page, payload)
        except OSError:
            pass
        finally:
            ROUTER.leave(page)

    def respond(self, send_body):
        path, kind = self.target()
        if kind in ('manifest', 'product_id'):
            body = self.manifest if kind == 'manifest' else self.product_id
            self.send_response(200)
            self.send_header('Content-Type', 'application/json' if kind == 'manifest' else 'application/octet-stream')
            self.send_header('Content-Length', str(len(body)))
            self.end_headers()
            if send_body:
                self.wfile.write(body)
            return
        if path is None:
            self.send_error(404)
            return
        size = os.path.getsize(path)
        start, end = 0, size - 1
        status = 200
        requested = self.headers.get('Range')
        if requested:
            match = re.fullmatch(r'bytes=(\d*)-(\d*)', requested.strip())
            if not match or (match.group(1) == '' and match.group(2) == ''):
                self.send_error(416)
                return
            if match.group(1) == '':
                start = max(0, size - int(match.group(2)))  # suffix range: the last n bytes
            else:
                start = int(match.group(1))
                end = min(int(match.group(2)), size - 1) if match.group(2) else size - 1
            if start > end or start >= size:
                self.send_response(416)
                self.send_header('Content-Range', 'bytes */%d' % size)
                self.send_header('Content-Length', '0')
                self.end_headers()
                return
            status = 206
        self.send_response(status)
        self.send_header('Content-Type', mimetypes.guess_type(path)[0] or 'application/octet-stream')
        self.send_header('Accept-Ranges', 'bytes')
        self.send_header('Content-Length', str(end - start + 1))
        if status == 206:
            self.send_header('Content-Range', 'bytes %d-%d/%d' % (start, end, size))
        self.end_headers()
        if not send_body:
            return
        with open(path, 'rb') as f:
            f.seek(start)
            remaining = end - start + 1
            while remaining > 0:
                chunk = f.read(min(remaining, 1 << 20))
                if not chunk:
                    break
                self.wfile.write(chunk)
                remaining -= len(chunk)


def main():
    parser = argparse.ArgumentParser(description=__doc__.split('\n\n')[0])
    parser.add_argument('--web', default='build/web', help='the web build folder (halo.html, halo.js, halo.wasm)')
    parser.add_argument('--halo', required=True, help='the installed Halo PC folder')
    parser.add_argument('--fx', help='converted shaders\\fx.bin to serve as override/shaders/fx.bin')
    parser.add_argument('--port', type=int, default=8080)
    args = parser.parse_args()

    Handler.web_root = os.path.abspath(args.web)
    Handler.halo_root = os.path.abspath(args.halo)
    Handler.fx_override = os.path.abspath(args.fx) if args.fx else None
    Handler.product_id = digital_product_id()
    Handler.manifest = build_manifest(Handler.halo_root, Handler.fx_override, Handler.product_id)
    server = ThreadingHTTPServer(('127.0.0.1', args.port), Handler)
    print('halo-re: http://127.0.0.1:%d/  (Halo files from %s)' % (args.port, Handler.halo_root))
    server.serve_forever()


if __name__ == '__main__':
    main()
