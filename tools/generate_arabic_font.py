#!/usr/bin/env python3
"""
Generate Arabic bitmap font header for DMD32Plus library.
Renders Arabic characters from a system TTF font into column-major bitmap format
compatible with the DMD drawChar() rendering engine.

Font data layout (variable-width):
  [0..1] uint16  total_size (non-zero = variable width)
  [2]    uint8   fixed_width (unused for variable, set to 0)
  [3]    uint8   height
  [4]    uint8   first_char
  [5]    uint8   char_count
  [6..6+N-1]     width of each character
  [6+N..]        glyph bitmap data

Glyph bitmap data per character (width W, vert_bytes B):
  For byte layer i = 0..B-1:
    For column j = 0..W-1:
      1 byte: bits encode vertical pixels in that column
  Byte layer 0 covers rows 0..7 (bit k -> row k)
  Byte layer 1 covers rows max(8, height-8)..height-1
    For height H: offset = H-8, bit k -> row offset+k, only where offset+k >= 8
"""

import sys
import os

try:
    from PIL import Image, ImageDraw, ImageFont
except ImportError:
    os.system(f"{sys.executable} -m pip install Pillow")
    from PIL import Image, ImageDraw, ImageFont

# ============================================================
# CONFIGURATION
# ============================================================
FONT_ARRAY_NAME = "ArabicFont"
HEADER_GUARD = "ARABICFONT_H"
FIRST_CHAR = 0x20        # Start from ASCII space to include Latin chars
LAST_CHAR = 0xFF
CHAR_COUNT = LAST_CHAR - FIRST_CHAR + 1  # 224 chars (32 Latin + 128 Arabic + 64 extended)
TARGET_MAX_HEIGHT = 11   # smaller Arabic font for better fit/readability
PIXEL_THRESHOLD = 80     # grayscale -> 1-bit threshold (lower = bolder)
CANVAS_SZ = 80           # render canvas size

OUTPUT_PATH = os.path.join(
    os.path.dirname(os.path.dirname(os.path.abspath(__file__))),
    "fonts", "ArabicFont.h"
)

# Character mapping: internal glyph code -> Unicode character
# Includes ASCII Latin characters (0x20-0x7F) for mixed text support
# Arabic letters are encoded as presentation forms so runtime shaping can select
# isolated/initial/medial/final glyphs.
GLYPH_MAP = {
    # ASCII Latin characters (0x20-0x7F)
    0x20: ' ', 0x21: '!', 0x22: '"', 0x23: '#', 0x24: '$', 0x25: '%', 0x26: '&', 0x27: "'",
    0x28: '(', 0x29: ')', 0x2A: '*', 0x2B: '+', 0x2C: ',', 0x2D: '-', 0x2E: '.', 0x2F: '/',
    0x30: '0', 0x31: '1', 0x32: '2', 0x33: '3', 0x34: '4', 0x35: '5', 0x36: '6', 0x37: '7',
    0x38: '8', 0x39: '9', 0x3A: ':', 0x3B: ';', 0x3C: '<', 0x3D: '=', 0x3E: '>', 0x3F: '?',
    0x40: '@', 0x41: 'A', 0x42: 'B', 0x43: 'C', 0x44: 'D', 0x45: 'E', 0x46: 'F', 0x47: 'G',
    0x48: 'H', 0x49: 'I', 0x4A: 'J', 0x4B: 'K', 0x4C: 'L', 0x4D: 'M', 0x4E: 'N', 0x4F: 'O',
    0x50: 'P', 0x51: 'Q', 0x52: 'R', 0x53: 'S', 0x54: 'T', 0x55: 'U', 0x56: 'V', 0x57: 'W',
    0x58: 'X', 0x59: 'Y', 0x5A: 'Z', 0x5B: '[', 0x5C: '\\', 0x5D: ']', 0x5E: '^', 0x5F: '_',
    0x60: '`', 0x61: 'a', 0x62: 'b', 0x63: 'c', 0x64: 'd', 0x65: 'e', 0x66: 'f', 0x67: 'g',
    0x68: 'h', 0x69: 'i', 0x6A: 'j', 0x6B: 'k', 0x6C: 'l', 0x6D: 'm', 0x6E: 'n', 0x6F: 'o',
    0x70: 'p', 0x71: 'q', 0x72: 'r', 0x73: 's', 0x74: 't', 0x75: 'u', 0x76: 'v', 0x77: 'w',
    0x78: 'x', 0x79: 'y', 0x7A: 'z', 0x7B: '{', 0x7C: '|', 0x7D: '}', 0x7E: '~', 0x7F: '\u007F',

    # Arabic presentation forms (0x80-0xFF)
    0x80: '\uFE80',  # HAMZA_ISO

    0x81: '\uFE81', 0x82: '\uFE82',  # ALEF_MADDA iso/final
    0x83: '\uFE83', 0x84: '\uFE84',  # ALEF_HAMZA_ABOVE iso/final
    0x85: '\uFE87', 0x86: '\uFE88',  # ALEF_HAMZA_BELOW iso/final
    0x87: '\uFE8D', 0x88: '\uFE8E',  # ALEF iso/final

    0x89: '\uFE8F', 0x8A: '\uFE90', 0x8B: '\uFE91', 0x8C: '\uFE92',  # BEH
    0x8D: '\uFE93', 0x8E: '\uFE94',                                      # TEH_MARBUTA
    0x8F: '\uFE95', 0x90: '\uFE96', 0x91: '\uFE97', 0x92: '\uFE98',  # TEH
    0x93: '\uFE99', 0x94: '\uFE9A', 0x95: '\uFE9B', 0x96: '\uFE9C',  # THEH
    0x97: '\uFE9D', 0x98: '\uFE9E', 0x99: '\uFE9F', 0x9A: '\uFEA0',  # JEEM
    0x9B: '\uFEA1', 0x9C: '\uFEA2', 0x9D: '\uFEA3', 0x9E: '\uFEA4',  # HAH
    0x9F: '\uFEA5', 0xA0: '\uFEA6', 0xA1: '\uFEA7', 0xA2: '\uFEA8',  # KHAH
    0xA3: '\uFEA9', 0xA4: '\uFEAA',                                      # DAL
    0xA5: '\uFEAB', 0xA6: '\uFEAC',                                      # THAL
    0xA7: '\uFEAD', 0xA8: '\uFEAE',                                      # REH
    0xA9: '\uFEAF', 0xAA: '\uFEB0',                                      # ZAIN
    0xAB: '\uFEB1', 0xAC: '\uFEB2', 0xAD: '\uFEB3', 0xAE: '\uFEB4',  # SEEN
    0xAF: '\uFEB5', 0xB0: '\uFEB6', 0xB1: '\uFEB7', 0xB2: '\uFEB8',  # SHEEN
    0xB3: '\uFEB9', 0xB4: '\uFEBA', 0xB5: '\uFEBB', 0xB6: '\uFEBC',  # SAD
    0xB7: '\uFEBD', 0xB8: '\uFEBE', 0xB9: '\uFEBF', 0xBA: '\uFEC0',  # DAD
    0xBB: '\uFEC1', 0xBC: '\uFEC2', 0xBD: '\uFEC3', 0xBE: '\uFEC4',  # TAH
    0xBF: '\uFEC5', 0xC0: '\uFEC6', 0xC1: '\uFEC7', 0xC2: '\uFEC8',  # ZAH
    0xC3: '\uFEC9', 0xC4: '\uFECA', 0xC5: '\uFECB', 0xC6: '\uFECC',  # AIN
    0xC7: '\uFECD', 0xC8: '\uFECE', 0xC9: '\uFECF', 0xCA: '\uFED0',  # GHAIN
    0xCB: '\uFED1', 0xCC: '\uFED2', 0xCD: '\uFED3', 0xCE: '\uFED4',  # FEH
    0xCF: '\uFED5', 0xD0: '\uFED6', 0xD1: '\uFED7', 0xD2: '\uFED8',  # QAF
    0xD3: '\uFED9', 0xD4: '\uFEDA', 0xD5: '\uFEDB', 0xD6: '\uFEDC',  # KAF
    0xD7: '\uFEDD', 0xD8: '\uFEDE', 0xD9: '\uFEDF', 0xDA: '\uFEE0',  # LAM
    0xDB: '\uFEE1', 0xDC: '\uFEE2', 0xDD: '\uFEE3', 0xDE: '\uFEE4',  # MEEM
    0xDF: '\uFEE5', 0xE0: '\uFEE6', 0xE1: '\uFEE7', 0xE2: '\uFEE8',  # NOON
    0xE3: '\uFEE9', 0xE4: '\uFEEA', 0xE5: '\uFEEB', 0xE6: '\uFEEC',  # HEH
    0xE7: '\uFEED', 0xE8: '\uFEEE',                                      # WAW
    0xE9: '\uFEEF', 0xEA: '\uFEF0',                                      # ALEF_MAKSURA
    0xEB: '\uFEF1', 0xEC: '\uFEF2', 0xED: '\uFEF3', 0xEE: '\uFEF4',  # YEH

    0xEF: '\u0640',  # TATWEEL
    # 0xF0 = SPACE (handled specially)

    # Western digits (Arabic numerals)
    0xF1: '0', 0xF2: '1', 0xF3: '2', 0xF4: '3', 0xF5: '4',
    0xF6: '5', 0xF7: '6', 0xF8: '7', 0xF9: '8', 0xFA: '9',

    0xFB: '\u060C',  # ARABIC COMMA
    0xFC: '.',        # PERIOD
    0xFD: '\u061F',  # ARABIC QUESTION MARK
    0xFE: '\uFEFB',  # LAM_ALEF isolated
    0xFF: '\uFEFC',  # LAM_ALEF final
}


def find_system_font():
    candidates = [
        r"C:\Windows\Fonts\tahoma.ttf",
        r"C:\Windows\Fonts\arial.ttf",
        r"C:\Windows\Fonts\segoeui.ttf",
    ]
    for p in candidates:
        if os.path.exists(p):
            return p
    return None


def render_char(ch, font, canvas_sz=CANVAS_SZ, thin_latin=False):
    """Render a single character and return (Image, pixel accessor)."""
    img = Image.new('L', (canvas_sz, canvas_sz), 0)
    draw = ImageDraw.Draw(img)
    draw.text((canvas_sz // 4, canvas_sz // 4), ch, fill=255, font=font)
    return img, img.load()


def find_bounds(px, canvas_sz, threshold):
    """Find the bounding box of lit pixels."""
    min_x, max_x = canvas_sz, -1
    min_y, max_y = canvas_sz, -1
    for y in range(canvas_sz):
        for x in range(canvas_sz):
            if px[x, y] > threshold:
                min_x = min(min_x, x)
                max_x = max(max_x, x)
                min_y = min(min_y, y)
                max_y = max(max_y, y)
    return min_x, max_x, min_y, max_y


def find_best_font_size(font_path, max_height):
    """Find the largest pt size where the tallest glyph fits in max_height."""
    best = 8
    for size in range(6, 50):
        font = ImageFont.truetype(font_path, size)
        g_top, g_bot = CANVAS_SZ, -1
        for code in range(FIRST_CHAR, FIRST_CHAR + CHAR_COUNT):
            ch = GLYPH_MAP.get(code)
            if ch is None:
                continue
            is_latin = (code >= 0x20 and code <= 0x7E)
            _, px = render_char(ch, font, thin_latin=is_latin)
            _, _, min_y, max_y = find_bounds(px, CANVAS_SZ, PIXEL_THRESHOLD)
            if min_y <= max_y:
                g_top = min(g_top, min_y)
                g_bot = max(g_bot, max_y)
        if g_top > g_bot:
            continue
        h = g_bot - g_top + 1
        if h <= max_height:
            best = size
        else:
            break
    return best


def main():
    font_path = find_system_font()
    if not font_path:
        print("ERROR: No Arabic-capable system font found!")
        sys.exit(1)

    print(f"Using font: {font_path}")

    best_size = find_best_font_size(font_path, TARGET_MAX_HEIGHT)
    print(f"Best font size: {best_size}pt (fits in {TARGET_MAX_HEIGHT}px)")

    font = ImageFont.truetype(font_path, best_size)

    # --- Pass 1: Find global vertical bounds across ALL characters ---
    global_top, global_bot = CANVAS_SZ, -1
    for code in range(FIRST_CHAR, FIRST_CHAR + CHAR_COUNT):
        ch = GLYPH_MAP.get(code)
        if ch is None:
            continue
        is_latin = (code >= 0x20 and code <= 0x7E)
        _, px = render_char(ch, font, thin_latin=is_latin)
        _, _, min_y, max_y = find_bounds(px, CANVAS_SZ, PIXEL_THRESHOLD)
        if min_y <= max_y:
            global_top = min(global_top, min_y)
            global_bot = max(global_bot, max_y)

    font_height = min(global_bot - global_top + 1, TARGET_MAX_HEIGHT)
    vert_bytes = (font_height + 7) // 8

    print(f"Font height: {font_height}px  (vert_bytes={vert_bytes})")

    # --- Pass 2: Render each character ---
    char_widths = []
    char_columns = []  # list of [ [byte0, byte1, ...], ... ] per column

    for code in range(FIRST_CHAR, FIRST_CHAR + CHAR_COUNT):
        ch = GLYPH_MAP.get(code)
        # Check if this is a Latin character
        is_latin = (code >= 0x20 and code <= 0x7E)

        # Space
        if code == 0xF0:
            sw = max(3, font_height // 3)
            char_widths.append(sw)
            char_columns.append([[0] * vert_bytes for _ in range(sw)])
            continue

        # Reserved / unmapped
        if ch is None:
            char_widths.append(2)
            char_columns.append([[0] * vert_bytes for _ in range(2)])
            continue

        # Render (with thinning for Latin)
        _, px = render_char(ch, font, thin_latin=is_latin)
        min_x, max_x, _, _ = find_bounds(px, CANVAS_SZ, PIXEL_THRESHOLD)

        if min_x > max_x:
            char_widths.append(2)
            char_columns.append([[0] * vert_bytes for _ in range(2)])
            continue

        # Add padding to Latin characters (1px on each side)
        left_padding = 1 if is_latin else 0
        right_padding = 1 if is_latin else 0

        # Extract columns
        cols = []
        
        # Add left padding
        for _ in range(left_padding):
            cols.append([0] * vert_bytes)
        
        for cx in range(min_x, max_x + 1):
            col_bytes = [0] * vert_bytes
            for row in range(font_height):
                src_y = global_top + row
                if src_y < CANVAS_SZ and px[cx, src_y] > PIXEL_THRESHOLD:
                    if vert_bytes == 1:
                        col_bytes[0] |= (1 << row)
                    elif row < 8:
                        col_bytes[0] |= (1 << row)
                    else:
                        offset = font_height - 8
                        bit = row - offset
                        if bit >= 0:
                            col_bytes[1] |= (1 << bit)
            cols.append(col_bytes)
        
        # Add right padding
        for _ in range(right_padding):
            cols.append([0] * vert_bytes)

        char_widths.append(len(cols))
        char_columns.append(cols)

    # --- Build raw byte array ---
    glyph_data_size = sum(w * vert_bytes for w in char_widths)
    total_size = 6 + CHAR_COUNT + glyph_data_size

    raw = []
    raw.append(total_size & 0xFF)
    raw.append((total_size >> 8) & 0xFF)
    raw.append(0)              # fixed_width = 0 (variable)
    raw.append(font_height)
    raw.append(FIRST_CHAR)
    raw.append(CHAR_COUNT)

    # Width table
    for w in char_widths:
        raw.append(w & 0xFF)

    # Glyph data: for each char, layer 0 columns then layer 1 columns, etc.
    for w, cols in zip(char_widths, char_columns):
        for layer in range(vert_bytes):
            for col_idx in range(w):
                raw.append(cols[col_idx][layer])

    print(f"Total font bytes: {len(raw)}")
    print(f"Char widths: min={min(char_widths)}, max={max(char_widths)}, "
          f"avg={sum(char_widths)/len(char_widths):.1f}")

    # --- Generate C header ---
    lines = [
        f"#ifndef {HEADER_GUARD}",
        f"#define {HEADER_GUARD}",
        "",
        "#include <inttypes.h>",
        "",
        f"// Arabic font for DMD32Plus",
        f"// Generated from: {os.path.basename(font_path)}",
        f"// Height: {font_height}px, variable width",
        f"// Characters: {CHAR_COUNT} (0x{FIRST_CHAR:02X} - 0x{LAST_CHAR:02X})",
        f"// Total size: {len(raw)} bytes",
        "",
        "#ifndef PROGMEM",
        "#define PROGMEM",
        "#endif",
        "",
        f"static const uint8_t {FONT_ARRAY_NAME}[] PROGMEM = {{",
    ]

    for i in range(0, len(raw), 16):
        chunk = raw[i:i + 16]
        hex_vals = ", ".join(f"0x{b:02X}" for b in chunk)
        comma = "," if i + 16 < len(raw) else ""
        lines.append(f"    {hex_vals}{comma}")

    lines.append("};")
    lines.append("")
    lines.append("#endif")
    lines.append("")

    os.makedirs(os.path.dirname(OUTPUT_PATH), exist_ok=True)
    with open(OUTPUT_PATH, 'w') as f:
        f.write("\n".join(lines))

    print(f"\nFont header written to: {OUTPUT_PATH}")

    # Summary
    for i, code in enumerate(range(FIRST_CHAR, FIRST_CHAR + CHAR_COUNT)):
        ch = GLYPH_MAP.get(code)
        if code == 0xF0:
            label = "SPACE"
        elif ch is None:
            label = "(reserved)"
        else:
            label = f"U+{ord(ch):04X} {ch}"
        print(f"  0x{code:02X}: {label:24s} w={char_widths[i]}")


if __name__ == "__main__":
    main()
