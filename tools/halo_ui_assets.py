"""Extracts the loading-screen assets from a retail Halo PC install, for the web shell.

    python tools/halo_ui_assets.py <halo root> <out dir>

Writes background.png, font.png, font.json and loading.json. Formats follow include/halo/cache/map_file.hpp,
data_map_file.hpp and types/tags.h (Bitmap, BitmapData, Font, FontCharacter, UnicodeStringList).
"""
import json
import os
import struct
import sys
import zlib

TAG_DATA_BASE = 0x40440000  # k_map_tag_data_base
BITMAP_EXTERNAL = 0x100     # BitmapData flags: pixels live in bitmaps.map
BITMAP_COMPRESSED = 0x2
BITMAP_SWIZZLED = 0x8


class CacheMap:
    """A .map file's tag data block and tag table (tools/map_inspect/map_reader.cpp)."""

    def __init__(self, path):
        with open(path, 'rb') as f:
            self.file = f.read()
        head, version, _, _, tag_data_offset, tag_data_size = struct.unpack_from('<IiiIII', self.file, 0)
        if head != 0x68656164 or version != 7:
            raise ValueError(f'{path}: not a retail PC cache file')
        self.data = self.file[tag_data_offset:tag_data_offset + tag_data_size]
        tags_address, _, _, tag_count = struct.unpack_from('<IIIi', self.data, 0)
        self.tags = {}
        for i in range(tag_count):
            group, _, _, _, path_address, data_address = struct.unpack_from('<6I', self.data, tags_address - TAG_DATA_BASE + i * 0x20)
            name = self.cstr(path_address)
            self.tags[(group.to_bytes(4, 'big').decode('latin-1'), name)] = data_address - TAG_DATA_BASE

    def cstr(self, address):
        start = address - TAG_DATA_BASE
        return self.data[start:self.data.index(b'\0', start)].decode('latin-1')

    def find(self, group, path):
        try:
            return self.tags[(group, path)]
        except KeyError:
            raise KeyError(f'{group} tag {path!r} not in the map') from None

    def reflexive(self, offset):
        """(count, data offset) of a TagReflexive at `offset`."""
        count, pointer = struct.unpack_from('<II', self.data, offset)
        return count, pointer - TAG_DATA_BASE

    def data_offset_bytes(self, offset):
        """Bytes of a TagDataOffset at `offset` (size 0x00, pointer 0x0c)."""
        size, _, _, pointer = struct.unpack_from('<4I', self.data, offset)
        start = pointer - TAG_DATA_BASE
        return self.data[start:start + size]


# --- PNG -----------------------------------------------------------------------------------------------------------

def png_rgba(width, height, rgba):
    def chunk(kind, body):
        return struct.pack('>I', len(body)) + kind + body + struct.pack('>I', zlib.crc32(kind + body) & 0xffffffff)
    stride = width * 4
    raw = b''.join(b'\0' + bytes(rgba[y * stride:(y + 1) * stride]) for y in range(height))
    return (b'\x89PNG\r\n\x1a\n' + chunk(b'IHDR', struct.pack('>IIBBBBB', width, height, 8, 6, 0, 0, 0)) +
            chunk(b'IDAT', zlib.compress(raw, 9)) + chunk(b'IEND', b''))


# --- pixel decoders (output RGBA bytes) ----------------------------------------------------------------------------

def _565(c):
    r, g, b = (c >> 11) & 31, (c >> 5) & 63, c & 31
    return (r << 3 | r >> 2, g << 2 | g >> 4, b << 3 | b >> 2)


def _dxt_colors(block, offset, four_color_always):
    c0, c1, indices = struct.unpack_from('<HHI', block, offset)
    p0, p1 = _565(c0), _565(c1)
    if c0 > c1 or four_color_always:
        p2 = tuple((2 * a + b) // 3 for a, b in zip(p0, p1))
        p3 = tuple((a + 2 * b) // 3 for a, b in zip(p0, p1))
        palette = [p0 + (255,), p1 + (255,), p2 + (255,), p3 + (255,)]
    else:
        p2 = tuple((a + b) // 2 for a, b in zip(p0, p1))
        palette = [p0 + (255,), p1 + (255,), p2 + (255,), (0, 0, 0, 0)]
    return [palette[(indices >> (2 * t)) & 3] for t in range(16)]


def _dxt3_alpha(block):
    rows = struct.unpack_from('<4H', block, 0)
    return [((rows[t >> 2] >> ((t & 3) * 4)) & 15) * 17 for t in range(16)]


def _dxt5_alpha(block):
    a0, a1 = block[0], block[1]
    if a0 > a1:
        palette = [a0, a1] + [((7 - i) * a0 + i * a1) // 7 for i in range(1, 7)]
    else:
        palette = [a0, a1] + [((5 - i) * a0 + i * a1) // 5 for i in range(1, 5)] + [0, 255]
    bits = int.from_bytes(block[2:8], 'little')
    return [palette[(bits >> (3 * t)) & 7] for t in range(16)]


def decode_dxt(pixels, width, height, fmt):
    out = bytearray(width * height * 4)
    block_size = 8 if fmt == 14 else 16
    offset = 0
    for by in range(0, height, 4):
        for bx in range(0, width, 4):
            block = pixels[offset:offset + block_size]
            offset += block_size
            if fmt == 14:
                texels = _dxt_colors(block, 0, False)
            else:
                colors = _dxt_colors(block, 8, True)
                alpha = _dxt3_alpha(block) if fmt == 15 else _dxt5_alpha(block)
                texels = [c[:3] + (a,) for c, a in zip(colors, alpha)]
            for t, texel in enumerate(texels):
                x, y = bx + (t & 3), by + (t >> 2)
                if x < width and y < height:
                    out[(y * width + x) * 4:(y * width + x) * 4 + 4] = bytes(texel)
    return out


def _expand(value, bits):
    return (value * 255 + ((1 << bits) - 1) // 2) // ((1 << bits) - 1) if bits else 255


# format -> (bytes per pixel, texel word -> (r, g, b, a)); types/tags.h BitmapDataFormat
_UNCOMPRESSED = {
    0: (1, lambda v: (255, 255, 255, v)),                                     # a8
    1: (1, lambda v: (v, v, v, 255)),                                         # y8
    2: (1, lambda v: (v, v, v, v)),                                           # ay8
    3: (2, lambda v: (v & 255, v & 255, v & 255, v >> 8)),                    # a8y8
    6: (2, lambda v: _565(v) + (255,)),                                       # r5g6b5
    8: (2, lambda v: (_expand(v >> 10 & 31, 5), _expand(v >> 5 & 31, 5), _expand(v & 31, 5), 255 if v >> 15 else 0)),
    9: (2, lambda v: (_expand(v >> 8 & 15, 4), _expand(v >> 4 & 15, 4), _expand(v & 15, 4), _expand(v >> 12, 4))),
    10: (4, lambda v: (v >> 16 & 255, v >> 8 & 255, v & 255, 255)),           # x8r8g8b8
    11: (4, lambda v: (v >> 16 & 255, v >> 8 & 255, v & 255, v >> 24)),       # a8r8g8b8
}


def decode_bitmap(pixels, width, height, fmt):
    if fmt in (14, 15, 16):
        return decode_dxt(pixels, width, height, fmt)
    if fmt not in _UNCOMPRESSED:
        raise ValueError(f'bitmap format {fmt} is not supported')
    size, convert = _UNCOMPRESSED[fmt]
    code = {1: 'B', 2: 'H', 4: 'I'}[size]
    words = struct.unpack_from(f'<{width * height}{code}', pixels, 0)
    return bytearray(b''.join(bytes(convert(v)) for v in words))


# --- tags ----------------------------------------------------------------------------------------------------------

def extract_bitmap(ui, maps_dir, path):
    """First bitmap of sequence 0 of a 'bitm' tag, mip 0, as (width, height, format, rgba)."""
    tag = ui.find('bitm', path)
    seq_count, sequences = ui.reflexive(tag + 0x54)
    data_count, datas = ui.reflexive(tag + 0x60)
    index = struct.unpack_from('<H', ui.data, sequences + 0x20)[0] if seq_count else 0
    entry = datas + index * 0x30
    _, width, height, _, _, fmt, flags = struct.unpack_from('<I6H', ui.data, entry)
    offset, size = struct.unpack_from('<II', ui.data, entry + 0x18)
    if flags & BITMAP_SWIZZLED:
        raise ValueError(f'{path}: swizzled bitmaps are not supported')
    if flags & BITMAP_EXTERNAL:  # texture_cache.cpp: absolute offset into bitmaps.map, else into the map itself
        with open(os.path.join(maps_dir, 'bitmaps.map'), 'rb') as f:
            f.seek(offset)
            pixels = f.read(size)
    else:
        pixels = ui.file[offset:offset + size]
    return width, height, fmt, decode_bitmap(pixels, width, height, fmt)


def extract_strings(ui, path):
    count, strings = ui.reflexive(ui.find('ustr', path))
    out = []
    for i in range(count):
        text = ui.data_offset_bytes(strings + i * 0x14).decode('utf-16-le')
        out.append(text.split('\0', 1)[0])
    return out


def extract_font(ui, path, atlas_width=512):
    """Glyph atlas (shelf packed, 1 px gap) plus the metrics text_encoding.cpp draws with:
    glyph top-left = pen - origin, then pen.x += character_width; line height = ascending + descending + leading."""
    tag = ui.find('font', path)
    flags, ascending, descending, leading_height, leading_width, encoding = struct.unpack_from('<I5h', ui.data, tag)
    count, characters = ui.reflexive(tag + 0x7c)
    pixels = ui.data_offset_bytes(tag + 0x88)

    glyphs = []
    for i in range(count):
        code, char_width, bw, bh, ox, oy, _, _, pixels_offset = struct.unpack_from('<Hhhhhhhhi', ui.data, characters + i * 0x14)
        glyphs.append({'character': code, 'character_width': char_width, 'bitmap_width': bw, 'bitmap_height': bh,
                       'bitmap_origin_x': ox, 'bitmap_origin_y': oy, 'pixels_offset': pixels_offset})

    # ponytail: shelf packing in tag order, wastes some rows; fine for one font at 512 wide.
    x = y = row = 0
    for g in glyphs:
        w, h = max(g['bitmap_width'], 0), max(g['bitmap_height'], 0)
        if x + w > atlas_width:
            x, y, row = 0, y + row + 1, 0
        g['atlas_x'], g['atlas_y'] = x, y
        x += w + 1
        row = max(row, h)
    atlas_height = y + row
    rgba = bytearray(b'\xff\xff\xff\x00' * atlas_width * atlas_height)
    for g in glyphs:
        w, h, src = max(g['bitmap_width'], 0), max(g['bitmap_height'], 0), g['pixels_offset']  # space is -2 x 0
        for gy in range(h):
            base = ((g['atlas_y'] + gy) * atlas_width + g['atlas_x']) * 4 + 3
            rgba[base:base + w * 4:4] = pixels[src + gy * w:src + gy * w + w]

    return {'atlas_png': png_rgba(atlas_width, atlas_height, rgba), 'atlas_size': (atlas_width, atlas_height),
            'characters': glyphs, 'ascending_height': ascending, 'descending_height': descending,
            'leading_height': leading_height, 'leading_width': leading_width,
            'line_height': ascending + descending + leading_height, 'flags': flags, 'encoding_type': encoding}


def extract(halo_root):
    maps_dir = os.path.join(halo_root, 'MAPS')
    ui = CacheMap(os.path.join(maps_dir, 'ui.map'))
    width, height, fmt, rgba = extract_bitmap(ui, maps_dir, 'ui\\shell\\bitmaps\\background')
    return {'background_png': png_rgba(width, height, rgba), 'background_size': (width, height), 'background_format': fmt,
            'loading_strings': extract_strings(ui, 'ui\\shell\\strings\\loading'),
            'font': extract_font(ui, 'ui\\large_ui')}


def _self_check():
    # A DXT1 block whose two endpoints are pure red and pure blue, all texels index 0/1 alternating.
    block = struct.pack('<HHI', 0xf800, 0x001f, 0x44444444)
    rgba = decode_dxt(block, 4, 4, 14)
    assert rgba[0:4] == b'\xff\x00\x00\xff' and rgba[4:8] == b'\x00\x00\xff\xff', rgba[:8]
    assert decode_bitmap(struct.pack('<I', 0x80102030), 1, 1, 11) == b'\x10\x20\x30\x80'


if __name__ == '__main__':
    _self_check()
    if len(sys.argv) != 3:
        sys.exit('usage: halo_ui_assets.py <halo root> <out dir>')
    result = extract(sys.argv[1])
    out = sys.argv[2]
    os.makedirs(out, exist_ok=True)
    with open(os.path.join(out, 'background.png'), 'wb') as f:
        f.write(result['background_png'])
    font = dict(result['font'])
    with open(os.path.join(out, 'font.png'), 'wb') as f:
        f.write(font.pop('atlas_png'))
    with open(os.path.join(out, 'font.json'), 'w') as f:
        json.dump(font, f, indent=1)
    with open(os.path.join(out, 'loading.json'), 'w', encoding='utf-8') as f:
        json.dump(result['loading_strings'], f, indent=1, ensure_ascii=False)
    print('background', result['background_size'], 'format', result['background_format'])
    print('font atlas', font['atlas_size'], len(font['characters']), 'characters, line height', font['line_height'])
    for s in result['loading_strings']:
        print(repr(s))
