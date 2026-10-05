"""Checks serve_web.py's virtual LAN router without a browser: python tools/serve_web_test.py"""
import io
import struct

from serve_web import MSG_BIND, MSG_BOUND, MSG_DATA, MSG_SENDTO, Router


def frames(stream):
    """The payloads of the server frames written so far (short frames only)."""
    data, out, i = stream.getvalue(), [], 0
    while i < len(data):
        size = data[i + 1]
        out.append(data[i + 2:i + 2 + size])
        i += 2 + size
    stream.seek(0)
    stream.truncate()
    return out


def main():
    router = Router()
    a_out, b_out = io.BytesIO(), io.BytesIO()
    a, b = router.join(a_out), router.join(b_out)
    assert a.address != b.address and a.address[:2] == bytes([10, 66])

    router.handle(b, struct.pack('<BIH', MSG_BIND, 7, 2302))
    assert frames(b_out) == [struct.pack('<BIH', MSG_BOUND, 7, 2302)]
    router.handle(b, struct.pack('<BIH', MSG_BIND, 8, 2302))
    assert frames(b_out) == [struct.pack('<BIH', MSG_BOUND, 8, 0)], 'a taken port is refused'

    # unicast from a's unbound socket: binds an ephemeral port, reaches b's socket 7 with a's address
    router.handle(a, struct.pack('<BI4sH', MSG_SENDTO, 1, b.address, 2302) + b'hi')
    bound = frames(a_out)[0]
    port = struct.unpack_from('<H', bound, 5)[0]
    assert bound[0] == MSG_BOUND and port >= 49152
    assert frames(b_out) == [struct.pack('<BI4sH', MSG_DATA, 7, a.address, port) + b'hi']

    # broadcast reaches every socket on the port; loopback reaches the sender's own address
    router.handle(a, struct.pack('<BIH', MSG_BIND, 2, 2302))
    frames(a_out)
    router.handle(a, struct.pack('<BI4sH', MSG_SENDTO, 1, bytes([255] * 4), 2302) + b'lan')
    assert frames(b_out) == [struct.pack('<BI4sH', MSG_DATA, 7, a.address, port) + b'lan']
    assert frames(a_out) == [struct.pack('<BI4sH', MSG_DATA, 2, a.address, port) + b'lan']
    router.handle(a, struct.pack('<BI4sH', MSG_SENDTO, 1, bytes([127, 0, 0, 1]), 2302) + b'me')
    assert frames(a_out) == [struct.pack('<BI4sH', MSG_DATA, 2, a.address, port) + b'me'] and frames(b_out) == []

    router.leave(b)
    router.handle(a, struct.pack('<BI4sH', MSG_SENDTO, 1, b.address, 2302) + b'gone')
    assert frames(b_out) == [], 'a page that left gets nothing'
    print('ok')


if __name__ == '__main__':
    main()
