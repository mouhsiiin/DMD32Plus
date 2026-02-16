# DMD32 Plus (Modified Fork)

**This is a modified fork of [ahmadfathan/DMD32Plus](https://github.com/ahmadfathan/DMD32Plus)**

A library for driving the Freetronics 512 pixel dot matrix LED display "DMD", a 32 x 16 layout using ESP32.

This library is based on the original [(DMD)](https://github.com/freetronics/DMD) modified to support ESP32 (currently it is only working on ESP32).

## Modifications in This Fork

This fork includes extensive Arabic text support enhancements:

- **Full Arabic contextual shaping** with isolated/initial/medial/final forms
- **Mixed Latin/Arabic text support** in a single font (ArabicFont includes ASCII 0x20-0x7F)
- **RTL (right-to-left) scrolling** for Arabic text
- **Lam-Alef ligatures** for proper Arabic typography
- **Custom font generation** from system TTF fonts (tools/generate_arabic_font.py)
- **Compact rendering** with no inter-character gaps for Arabic
- **Bi-directional text** handling (Latin displays LTR within RTL Arabic flow)

### Original Repository

Original DMD32Plus by Ahmad Fathan: https://github.com/ahmadfathan/DMD32Plus

The connection between ESP32 and DMD display shown in the image below:






![](https://github.com/ahmadfathan/DMD32Plus/blob/main/connection.png)

## Enhanced Arabic Font Support

This fork includes comprehensive Arabic display support:

### Features

- **ArabicFont** (`fonts/ArabicFont.h`): 11px variable-width font with 224 glyphs (ASCII + Arabic)
  - Includes Latin characters (A-Z, a-z, 0-9, symbols) with 1px padding
  - Generated from Windows Tahoma TTF using `tools/generate_arabic_font.py`
- **Contextual Shaping**: Automatic isolated/initial/medial/final form selection
- **Lam-Alef Ligatures**: Proper rendering of لا combinations
- **Right-to-Left Rendering**: `drawArabicString(...)` handles RTL text flow
- **Mixed Text Support**: Latin words maintain LTR within RTL Arabic text
- **UTF-8 Mapping**: `utf8ToArabic(...)` converts UTF-8 to glyph codes
- **Compact Rendering**: `drawStringCompact(...)` for zero inter-character spacing
- **Arabic Marquee**: `drawArabicMarquee(...)` with RTL scrolling

### API Functions

```cpp
// Core Arabic rendering
void drawArabicString(int x, int y, const char* utf8Text, byte mode);
void drawArabicMarquee(const char* utf8Text, byte len, int left, int top);

// Text conversion and utilities
uint16_t utf8ToArabic(const char* utf8Text, char* outBuffer, uint16_t bufSize);
void drawStringCompact(int x, int y, const char* str, int len, byte mode);
```

### Font Generation

Regenerate the Arabic font with custom settings:

```bash
python tools/generate_arabic_font.py
```

Edit `TARGET_MAX_HEIGHT` in the script to adjust font size.

### Notes

- Arabic-Indic digits (٠-٩) automatically map to Western digits (0-9)
- Digit sequences maintain LTR order within RTL text
- Supports Arabic punctuation: ، (comma), ؟ (question mark)
- Font includes tatweel (ـ) for text justification