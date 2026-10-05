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

Every response carries the cross-origin isolation headers browsers require before a page may use threads
(SharedArrayBuffer). The server listens on 127.0.0.1 only.
"""
import argparse
import json
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
        self.respond(send_body=True)

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
