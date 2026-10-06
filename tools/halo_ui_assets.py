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

def bitmap_sequence_index(ui, path, frame=0, sequence=0):
    """bitmap_group_sequence_get_bitmap_data (src/bitmaps/bitmap_group.cpp): frame -> bitmap data index; also the frame count."""
    tag = ui.find('bitm', path)
    seq_count, sequences = ui.reflexive(tag + 0x54)
    if not seq_count:
        return frame, 0
    seq = sequences + (sequence % seq_count) * 0x40
    first, count = struct.unpack_from('<HH', ui.data, seq + 0x20)
    sprite_count, sprites = ui.reflexive(seq + 0x34)
    if count > 0:
        return frame % count + first, count
    if sprite_count:
        return struct.unpack_from('<h', ui.data, sprites + frame * 0x20)[0], count
    return frame, count


def extract_bitmap(ui, maps_dir, path, index=None):
    """Bitmap data `index` (default: first bitmap of sequence 0) of a 'bitm' tag, mip 0, as (width, height, format, rgba)."""
    tag = ui.find('bitm', path)
    data_count, datas = ui.reflexive(tag + 0x60)
    if index is None:
        index = bitmap_sequence_index(ui, path)[0]
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


# --- ui_widget_definition ('DeLa') ---------------------------------------------------------------------------------
# Offsets into UIWidgetDefinition (types/tags.h, size 0x3ec) and ChildWidgetReference (size 0x50).

WIDGET_TYPES = ('container', 'text_box', 'spinner_list', 'column_list', 'game_model', 'movie', 'custom')
JUSTIFICATIONS = ('left', 'right', 'center')
FLAG_PASS_UNHANDLED_TO_FOCUSED_CHILD, FLAG_FLASH_BACKGROUND, FLAG_DONT_FOCUS_SPECIFIC_CHILD = 1 << 0, 1 << 2, 1 << 7
FLAG_NIFTY_RENDER_FX = 1 << 13
FLAGS1_FLASHING, FLAGS1_NO_FOCUS_TEST = 1 << 2, 1 << 3
FLAGS2_ITEMS_GENERATED_IN_CODE, FLAGS2_ITEMS_FROM_STRING_LIST = 1 << 0, 1 << 1
PULSE_COLOR = (1.0, 0.8, 0.8, 0.8)  # ui_get_saved_pulse_color: global white alpha + ui_saved_color {0.8, 0.8, 0.8}
SCREEN = (640, 480)                 # k_base_screen_width/height


def _dependency(ui, offset):
    """Tag path of a TagDependency (group, path pointer, path size, tag id), or None."""
    _, path, _, tag_id = struct.unpack_from('<4I', ui.data, offset)
    return None if tag_id == 0xffffffff or not path else ui.cstr(path)


def _tag_string(ui, offset):
    return ui.data[offset:offset + 32].split(b'\0', 1)[0].decode('latin-1')


def _read_widget(ui, path):
    t, d = ui.find('DeLa', path), ui.data
    widget_type, controller = struct.unpack_from('<hh', d, t)
    top, left, bottom, right = struct.unpack_from('<4h', d, t + 0x24)
    flags = struct.unpack_from('<I', d, t + 0x2c)[0]
    alpha, red, green, blue = struct.unpack_from('<4f', d, t + 0x10c)
    justification, flags_1 = struct.unpack_from('<hI', d, t + 0x11c)
    string_list_index, horiz_offset, vert_offset = struct.unpack_from('<Hhh', d, t + 0x12e)
    snr_count, snr = ui.reflexive(t + 0x60)
    child_count, children = ui.reflexive(t + 0x3e0)
    return {
        'tag': path, 'name': _tag_string(ui, t + 4), 'widget_type': widget_type, 'controller_index': controller,
        'bounds': (top, left, bottom, right), 'flags': flags, 'background_bitmap': _dependency(ui, t + 0x38),
        'event_handler_count': ui.reflexive(t + 0x54)[0],
        'search_and_replace': [{'search': _tag_string(ui, snr + i * 0x22),
                                'replace_function': struct.unpack_from('<h', d, snr + i * 0x22 + 0x20)[0]}
                               for i in range(snr_count)],
        'text_strings': _dependency(ui, t + 0xec), 'text_font': _dependency(ui, t + 0xfc),
        'text_color': (alpha, red, green, blue), 'justification': justification, 'flags_1': flags_1,
        'string_list_index': string_list_index, 'horiz_offset': horiz_offset, 'vert_offset': vert_offset,
        'flags_2': struct.unpack_from('<I', d, t + 0x150)[0],
        'children': [{'tag': _dependency(ui, e), 'name': _tag_string(ui, e + 0x10),
                      'vertical_offset': struct.unpack_from('<h', d, e + 0x36)[0],
                      'horizontal_offset': struct.unpack_from('<h', d, e + 0x38)[0]}
                     for e in (children + i * 0x50 for i in range(child_count))],
    }


def _instantiate(ui, path, parent=None):
    """The widget_instance tree chimera__load_ui_widget builds (ifr2_widgets.cpp initialize_from_tag /
    create_children_from_tag), minus anything only code or input changes. Children are created before their parent
    gets its own local_x/y, so each local offset is just the ChildWidgetReference's; render() then sums local
    offsets down the tree. ponytail: list items from string-list tags and extended descriptions are not built
    (the error dialogs have none)."""
    w = dict(_read_widget(ui, path), parent=parent, local=(0, 0), focused_child=None, kids=[])
    for ref in w['children']:
        if ref['tag']:
            child = _instantiate(ui, ref['tag'], w)
            child['local'] = (ref['horizontal_offset'], ref['vertical_offset'])
            w['kids'].append(child)
    is_list = w['widget_type'] in (2, 3)
    eligible = [k for k in w['kids'] if k['event_handler_count'] > 0 or k['widget_type'] in (2, 3)]
    if not w['flags'] & FLAG_DONT_FOCUS_SPECIFIC_CHILD and (is_list or w['flags'] & FLAG_PASS_UNHANDLED_TO_FOCUSED_CHILD):
        w['focused_child'] = w['kids'][0] if is_list and w['kids'] else (eligible[0] if eligible else None)
    if w['focused_child'] is None and eligible:  # initialize_from_tag relinks focus to each eligible child in turn
        w['focused_child'] = eligible[-1]
    return w


def _is_top_of_stack(w):
    """WidgetView::is_top_of_stack (0x499cb0)."""
    cursor = w['parent']
    if cursor is None:
        return True
    focused = cursor['focused_child'] is w
    if focused:
        return True
    while cursor['parent'] is not None:
        ancestor = cursor['parent']
        if ancestor['focused_child'] is not cursor:
            return False
        focused = focused or ancestor['widget_type'] in (2, 3)
        cursor = ancestor
    return focused


def _argb_hex(color):
    return '#%02x%02x%02x%02x' % tuple(int(c * 255 + 0.5) for c in color)


def extract_widget(halo_root_or_open_maps, path):
    """A ui_widget_definition as the flat draw list WidgetRender::render (0x49a8c0) produces at full screen
    (dest = 0,0,480,640, nothing scaled or faded). Rectangles are [top, left, bottom, right] on the 640x480 screen.

    Returns {'draw_list': [...], 'pngs': {name: png bytes}, 'fonts': {font tag: extract_font()}, 'strings': {ustr tag: [...]}}.
    Each draw-list entry is one widget in render order (own background, own text, then children). Backgrounds go
    through ui_draw_screen_quad(bounds, bounds, bitmap): the texture's top-left source_size texels (bounds size,
    capped at the texture size) are stretched over `bounds`, i.e. uv (0,0)-(uv_max), tinted by vertex_color.
    Text uses draw_text16 into text_rect: pen starts at (left + font leading_width, top + ascending_height),
    every line is line_height lower, lines break only at '\\r' or '|n' (no word wrap; overflow is clipped to the
    rect), and a line is drawn only while its baseline is above text_rect.bottom. Centre justification puts a line at
    left + ((right - left - width) >> 1); right at right - leading_width - width."""
    if isinstance(halo_root_or_open_maps, str):
        maps_dir = os.path.join(halo_root_or_open_maps, 'MAPS')
        ui = CacheMap(os.path.join(maps_dir, 'ui.map'))
    else:
        ui, maps_dir = halo_root_or_open_maps
    out = {'draw_list': [], 'pngs': {}, 'fonts': {}, 'strings': {}}

    def visit(w, x, y, depth, list_focus):
        x, y = x + w['local'][0], y + w['local'][1]
        top, left, bottom, right = w['bounds']
        entry = {'name': w['name'], 'tag': w['tag'], 'widget_type': WIDGET_TYPES[w['widget_type']], 'depth': depth,
                 'offset': (x, y), 'bounds': (top + y, left + x, bottom + y, right + x), 'hidden': False,
                 'flags': w['flags'], 'flags_1': w['flags_1'], 'flags_2': w['flags_2'],
                 'focused': w['parent'] is not None and w['parent']['focused_child'] is w,
                 'background': None, 'text': None}
        if w['background_bitmap']:
            bpath = w['background_bitmap']
            index, frames = bitmap_sequence_index(ui, bpath, 0)  # background_bitmap_frame starts at 0
            width, height, fmt, rgba = extract_bitmap(ui, maps_dir, bpath, index)
            name = f"{bpath.rsplit(chr(92), 1)[-1]}_{index}"
            out['pngs'].setdefault(name, png_rgba(width, height, rgba))
            src = (min(right - left, width), min(bottom - top, height))
            entry['background'] = {
                'png': name, 'bitmap_tag': bpath, 'sequence_index': 0, 'frame': 0, 'frame_count': frames,
                'bitmap_index': index, 'format': fmt, 'texture_size': (width, height), 'source_size': src,
                'uv_max': (src[0] / width, src[1] / height),
                'vertex_color': '#ffffffff',  # alpha = cumulative scale (1), rgb = k_rgb_mask
                'flashing': bool(w['flags'] & FLAG_FLASH_BACKGROUND),  # alpha *= (cos(ms * 0.003) + 1) / 2
                # focused child of a list (or always_use_nifty_render_fx): override tint (0, .05, .05, .05) while drawn
                'nifty_render_fx': list_focus or bool(w['flags'] & FLAG_NIFTY_RENDER_FX)}
        if w['widget_type'] == 1 and w['text_font'] and 0 <= w['justification'] < 3:
            if w['text_strings'] and w['text_strings'] not in out['strings']:
                out['strings'][w['text_strings']] = extract_strings(ui, w['text_strings'])
            if w['text_font'] not in out['fonts']:
                out['fonts'][w['text_font']] = extract_font(ui, w['text_font'])
            top_of_stack = True if w['flags_1'] & FLAGS1_NO_FOCUS_TEST else _is_top_of_stack(w)
            color, source = w['text_color'], 'tag'
            if top_of_stack or color[1:] == (1.0, 1.0, 1.0):
                color, source = (color[0],) + PULSE_COLOR[1:], 'pulse'
            strings = out['strings'].get(w['text_strings'], [])
            index = w['string_list_index']  # selection_index when code sets one (the error dialogs' message box)
            entry['text'] = {
                'strings_tag': w['text_strings'], 'string_index': index,
                'string': strings[index] if index < len(strings) else '', 'font': w['text_font'],
                'color': color, 'color_hex': _argb_hex(color), 'color_source': source,
                'justification': JUSTIFICATIONS[w['justification']],
                'text_rect': (top + y + w['vert_offset'], left + x + w['horiz_offset'], bottom + y, right + x),
                'horiz_offset': w['horiz_offset'], 'vert_offset': w['vert_offset'],
                'flashing': bool(w['flags_1'] & FLAGS1_FLASHING),  # alpha *= (cos(ms * 0.003) + 1.5) * 0.4
                'search_and_replace': w['search_and_replace']}
        out['draw_list'].append(entry)
        if not (w['widget_type'] == 3 and w['flags_2'] & FLAGS2_ITEMS_GENERATED_IN_CODE):
            for k in w['kids']:
                visit(k, x, y, depth + 1, w['widget_type'] in (2, 3) and w['focused_child'] is k)

    visit(_instantiate(ui, path), 0, 0, 0, False)
    return out


def render_preview(widget, pngs, fonts, texts=None):
    """Composites a draw list (extract_widget) into 640x480 RGBA, nearest sampling, for eyeballing.
    `texts` maps widget name -> replacement string."""
    import itertools
    width, height = SCREEN
    canvas = bytearray(b'\0\0\0\xff' * width * height)

    def blend(px, py, r, g, b, a):
        if 0 <= px < width and 0 <= py < height and a:
            o = (py * width + px) * 4
            canvas[o:o + 3] = bytes(int(c * a + d * (1 - a)) for c, d in zip((r, g, b), canvas[o:o + 3]))

    decoded = {}
    for entry in widget['draw_list']:
        bg = entry['background']
        if bg:
            if bg['png'] not in decoded:
                decoded[bg['png']] = _png_decode(pngs[bg['png']])
            tw, th, tex = decoded[bg['png']]
            top, left, bottom, right = entry['bounds']
            sw, sh = bg['source_size']
            for py, px in itertools.product(range(top, bottom), range(left, right)):
                sx, sy = (px - left) * sw // (right - left), (py - top) * sh // (bottom - top)
                o = (sy * tw + sx) * 4
                blend(px, py, *tex[o:o + 3], tex[o + 3] / 255)
        text = entry['text']
        if text:
            font = fonts[text['font']]
            atlas_w, _, atlas = _png_decode(font['atlas_png'])
            glyphs = {}
            for g in font['characters']:
                glyphs.setdefault(g['character'], g)
            s = (texts or {}).get(entry['name'], text['string']).replace('|n', '\r')
            top, left, bottom, right = text['text_rect']
            a, r, g_, b = text['color']
            for line_no, line in enumerate(s.split('\r')):
                line = [glyphs[ord(c)] for c in line if ord(c) in glyphs]
                baseline = top + font['ascending_height'] + line_no * font['line_height']
                if baseline >= bottom:
                    break
                span = sum(g['character_width'] for g in line)
                pen = {'left': left + font['leading_width'], 'right': right - font['leading_width'] - span,
                       'center': left + ((right - left - span) >> 1)}[text['justification']]
                for g in line:
                    gx, gy = pen - g['bitmap_origin_x'], baseline - g['bitmap_origin_y']
                    for yy, xx in itertools.product(range(max(g['bitmap_height'], 0)), range(max(g['bitmap_width'], 0))):
                        if left <= gx + xx < right and top <= gy + yy < bottom:
                            coverage = atlas[((g['atlas_y'] + yy) * atlas_w + g['atlas_x'] + xx) * 4 + 3] / 255
                            blend(gx + xx, gy + yy, r * 255, g_ * 255, b * 255, coverage * a)
                    pen += g['character_width']
    return png_rgba(width, height, canvas)


def _png_decode(png):
    """(width, height, rgba) of an 8-bit RGBA, non-interlaced PNG as png_rgba writes it (filter 0 only)."""
    width, height = struct.unpack_from('>II', png, 16)
    idat, pos = b'', 8
    while pos < len(png):
        length, kind = struct.unpack_from('>I4s', png, pos)
        if kind == b'IDAT':
            idat += png[pos + 8:pos + 8 + length]
        pos += 12 + length
    raw, stride = zlib.decompress(idat), width * 4
    return width, height, b''.join(raw[y * (stride + 1) + 1:(y + 1) * (stride + 1)] for y in range(height))


ERROR_DIALOG = 'ui\\shell\\error\\error_modal_fullscreen'


def extract_error_dialog(ui, maps_dir):
    """extract_widget of the full-screen modal error dialog plus which widgets ErrorDialogs::show (0x498f20) fills:
    dialog->first_child->first_child->selection_index = error_string_index (clamped to 0..59) picks the message."""
    dialog = extract_widget((ui, maps_dir), ERROR_DIALOG)
    root = _instantiate(ui, ERROR_DIALOG)
    message = root['kids'][0]['kids'][0]
    dialog['message_widget'] = message['name']
    dialog['message_strings'] = message['text_strings']
    dialog['header_widget'] = next((k['name'] for k in root['kids'][0]['kids'][1:] if k['widget_type'] == 1), None)
    return dialog


def extract(halo_root):
    maps_dir = os.path.join(halo_root, 'MAPS')
    ui = CacheMap(os.path.join(maps_dir, 'ui.map'))
    width, height, fmt, rgba = extract_bitmap(ui, maps_dir, 'ui\\shell\\bitmaps\\background')
    return {'background_png': png_rgba(width, height, rgba), 'background_size': (width, height), 'background_format': fmt,
            'loading_strings': extract_strings(ui, 'ui\\shell\\strings\\loading'),
            'font': extract_font(ui, 'ui\\large_ui'),
            'error_dialog': extract_error_dialog(ui, maps_dir)}


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
    dialog = result['error_dialog']
    for name, png in dialog['pngs'].items():
        with open(os.path.join(out, f'error_{name}.png'), 'wb') as f:
            f.write(png)
    for path, font in dialog['fonts'].items():
        with open(os.path.join(out, f"font_{path.rsplit(chr(92), 1)[-1]}.png"), 'wb') as f:
            f.write(font['atlas_png'])
    with open(os.path.join(out, 'error_dialog.json'), 'w', encoding='utf-8') as f:
        json.dump({k: v for k, v in dialog.items() if k not in ('pngs', 'fonts')}, f, indent=1, ensure_ascii=False)
    with open(os.path.join(out, 'error_preview.png'), 'wb') as f:
        f.write(render_preview(dialog, dialog['pngs'], dialog['fonts'],
                               {dialog['header_widget']: 'CONNECTION LOST',
                                dialog['message_widget']: 'Could not reach the\r\nserver. Check your\r\nconnection and retry.'}))
    for e in dialog['draw_list']:
        print('  ' * e['depth'] + e['name'], e['widget_type'], e['bounds'], e['background'] and e['background']['png'],
              e['text'] and (e['text']['string'], e['text']['font'], e['text']['color_hex'], e['text']['justification']))
