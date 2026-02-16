# DMD32 Plus
A library for driving the Freetronics 512 pixel dot matrix LED display "DMD", a 32 x 16 layout using ESP32.

This library is a fork of the original one [(DMD)](https://github.com/freetronics/DMD) modified to support ESP32 (currently it is only working on ESP32).

The connection between ESP32 and DMD display shown in the image below:






![](https://github.com/ahmadfathan/DMD32Plus/blob/main/connection.png)

## Arabic font support

This library now includes basic Arabic display support with:

- A new font: `fonts/ArabicFont.h` (16px variable-width, generated from tahoma.ttf)
- Right-to-left text rendering: `drawStringRTL(...)`
- UTF-8 Arabic mapping helper: `utf8ToArabic(...)`
- One-call Arabic drawing: `drawArabicString(...)`

See the example sketch:

- `examples/arabic_text/arabic_text.ino`

### Notes

- Arabic text is rendered with contextual forms (isolated/initial/medial/final) so letters are visually linked.
- UTF-8 Arabic letters and punctuation are mapped to the custom Arabic font glyph range.
- Digits are rendered as Western `0-9` numerals (Arabic numerals), not Arabic-Indic digits.