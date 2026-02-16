/*--------------------------------------------------------------------------------------

 DMD32.cpp - Function and support library for the Freetronics DMD, a 512 LED matrix display
           panel arranged in a 32 x 16 layout.

 Note that the DMD library uses the SPI port  (VSPI) for the fastest, low overhead writing to the
 display. Keep an eye on conflicts if there are any other devices running from the same
 SPI port, and that the chip select on those devices is correctly set to be inactive
 when the DMD is being written to.

 ---

 This program is free software: you can redistribute it and/or modify it under the terms
 of the version 3 GNU General Public License as published by the Free Software Foundation.

 This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 See the GNU General Public License for more details.

 You should have received a copy of the GNU General Public License along with this program.
 If not, see <http://www.gnu.org/licenses/>.

--------------------------------------------------------------------------------------*/
#include "DMD32Plus.h"
#include "utils.h"

struct ArabicLetterForm
{
    uint16_t codepoint;
    uint8_t isolated;
    uint8_t final;
    uint8_t initial;
    uint8_t medial;
    bool joinBefore;
    bool joinAfter;
};

static const uint8_t ARABIC_GLYPH_TATWEEL = 0xEF;
static const uint8_t ARABIC_GLYPH_SPACE = 0xF0;
static const uint8_t ARABIC_GLYPH_DIGIT_0 = 0xF1;
static const uint8_t ARABIC_GLYPH_COMMA = 0xFB;
static const uint8_t ARABIC_GLYPH_DOT = 0xFC;
static const uint8_t ARABIC_GLYPH_QUESTION = 0xFD;
static const uint8_t ARABIC_GLYPH_LAM_ALEF_ISO = 0xFE;
static const uint8_t ARABIC_GLYPH_LAM_ALEF_FINAL = 0xFF;

static const ArabicLetterForm kArabicForms[] = {
    {0x0621, 0x80, 0x80, 0x80, 0x80, false, false}, // hamza
    {0x0622, 0x81, 0x82, 0x81, 0x82, true, false},  // alef madda
    {0x0623, 0x83, 0x84, 0x83, 0x84, true, false},  // alef hamza above
    {0x0625, 0x85, 0x86, 0x85, 0x86, true, false},  // alef hamza below
    {0x0627, 0x87, 0x88, 0x87, 0x88, true, false},  // alef
    {0x0628, 0x89, 0x8A, 0x8B, 0x8C, true, true},   // beh
    {0x0629, 0x8D, 0x8E, 0x8D, 0x8E, true, false},  // teh marbuta
    {0x062A, 0x8F, 0x90, 0x91, 0x92, true, true},   // teh
    {0x062B, 0x93, 0x94, 0x95, 0x96, true, true},   // theh
    {0x062C, 0x97, 0x98, 0x99, 0x9A, true, true},   // jeem
    {0x062D, 0x9B, 0x9C, 0x9D, 0x9E, true, true},   // hah
    {0x062E, 0x9F, 0xA0, 0xA1, 0xA2, true, true},   // khah
    {0x062F, 0xA3, 0xA4, 0xA3, 0xA4, true, false},  // dal
    {0x0630, 0xA5, 0xA6, 0xA5, 0xA6, true, false},  // thal
    {0x0631, 0xA7, 0xA8, 0xA7, 0xA8, true, false},  // reh
    {0x0632, 0xA9, 0xAA, 0xA9, 0xAA, true, false},  // zain
    {0x0633, 0xAB, 0xAC, 0xAD, 0xAE, true, true},   // seen
    {0x0634, 0xAF, 0xB0, 0xB1, 0xB2, true, true},   // sheen
    {0x0635, 0xB3, 0xB4, 0xB5, 0xB6, true, true},   // sad
    {0x0636, 0xB7, 0xB8, 0xB9, 0xBA, true, true},   // dad
    {0x0637, 0xBB, 0xBC, 0xBD, 0xBE, true, true},   // tah
    {0x0638, 0xBF, 0xC0, 0xC1, 0xC2, true, true},   // zah
    {0x0639, 0xC3, 0xC4, 0xC5, 0xC6, true, true},   // ain
    {0x063A, 0xC7, 0xC8, 0xC9, 0xCA, true, true},   // ghain
    {0x0641, 0xCB, 0xCC, 0xCD, 0xCE, true, true},   // feh
    {0x0642, 0xCF, 0xD0, 0xD1, 0xD2, true, true},   // qaf
    {0x0643, 0xD3, 0xD4, 0xD5, 0xD6, true, true},   // kaf
    {0x0644, 0xD7, 0xD8, 0xD9, 0xDA, true, true},   // lam
    {0x0645, 0xDB, 0xDC, 0xDD, 0xDE, true, true},   // meem
    {0x0646, 0xDF, 0xE0, 0xE1, 0xE2, true, true},   // noon
    {0x0647, 0xE3, 0xE4, 0xE5, 0xE6, true, true},   // heh
    {0x0648, 0xE7, 0xE8, 0xE7, 0xE8, true, false},  // waw
    {0x0649, 0xE9, 0xEA, 0xE9, 0xEA, true, false},  // alef maksura
    {0x064A, 0xEB, 0xEC, 0xED, 0xEE, true, true},   // yeh
    {0x0640, 0xEF, 0xEF, 0xEF, 0xEF, true, true}    // tatweel
};

static const ArabicLetterForm *findArabicForm(uint16_t codepoint)
{
    for (size_t i = 0; i < (sizeof(kArabicForms) / sizeof(kArabicForms[0])); i++)
    {
        if (kArabicForms[i].codepoint == codepoint)
        {
            return &kArabicForms[i];
        }
    }
    return NULL;
}

static bool decodeNextUtf8Codepoint(const uint8_t *&src, uint16_t &codepoint)
{
    if (!src || !*src)
    {
        return false;
    }

    if ((*src & 0x80) == 0)
    {
        codepoint = *src;
        src++;
        return true;
    }

    if (((*src & 0xE0) == 0xC0) && src[1])
    {
        codepoint = (((uint16_t)(*src & 0x1F)) << 6) | (src[1] & 0x3F);
        src += 2;
        return true;
    }

    if (((*src & 0xF0) == 0xE0) && src[1] && src[2])
    {
        codepoint = (((uint16_t)(*src & 0x0F)) << 12) |
                    (((uint16_t)(src[1] & 0x3F)) << 6) |
                    (src[2] & 0x3F);
        src += 3;
        return true;
    }

    src++;
    while ((*src & 0xC0) == 0x80)
    {
        src++;
    }
    return false;
}

static uint8_t mapArabicSymbolCodepoint(uint16_t codepoint)
{
    // Latin ASCII characters (font now includes 0x20-0x7F range)
    if (codepoint >= 0x0020 && codepoint <= 0x007E)
    {
        return (uint8_t)codepoint;  // Direct mapping
    }
    
    // Arabic-Indic digits map to Western digits
    if (codepoint >= 0x0660 && codepoint <= 0x0669)
    {
        return 0x30 + (codepoint - 0x0660);  // Map to '0'-'9'
    }
    if (codepoint >= 0x06F0 && codepoint <= 0x06F9)
    {
        return 0x30 + (codepoint - 0x06F0);  // Map to '0'-'9'
    }

    // Arabic punctuation maps
    switch (codepoint)
    {
    case 0x060C:
        return ARABIC_GLYPH_COMMA;
    case 0x061F:
        return ARABIC_GLYPH_QUESTION;
    case 0x0640:
        return ARABIC_GLYPH_TATWEEL;
    default:
        return 0;
    }
}

/*--------------------------------------------------------------------------------------
 Setup and instantiation of DMD library
 Note this currently uses the SPI port for the fastest performance to the DMD, be
 careful of possible conflicts with other SPI port devices
--------------------------------------------------------------------------------------*/
DMD::DMD(byte panelsWide, byte panelsHigh)
{
    _aPin = PIN_DMD_A;
    _bPin = PIN_DMD_B;
    _latPin = PIN_DMD_LAT;
    _nOEPin = PIN_DMD_nOE;

    _clkPin = PIN_DMD_CLK;
    _rDataPin = PIN_DMD_R_DATA;

    init(panelsWide, panelsHigh);
}

DMD::DMD(
    byte panelsWide, byte panelsHigh, 
    uint8_t nOEPin, uint8_t aPin, uint8_t bPin, uint8_t clkPin, uint8_t latPin, uint8_t rDataPin
) {
    _aPin = aPin;
    _bPin = bPin;
    _latPin = latPin;
    _nOEPin = nOEPin;

    _clkPin = clkPin;
    _rDataPin = rDataPin;

    init(panelsWide, panelsHigh);
}

void DMD::init(byte panelsWide, byte panelsHigh)
{
    uint16_t ui;
    DisplaysWide = panelsWide;
    DisplaysHigh = panelsHigh;
    DisplaysTotal = DisplaysWide * DisplaysHigh;
    row1 = DisplaysTotal << 4;
    row2 = DisplaysTotal << 5;
    row3 = ((DisplaysTotal << 2) * 3) << 2;
    bDMDScreenRAM = (byte *)malloc(DisplaysTotal * DMD_RAM_SIZE_BYTES);   

    // initialise instance of the SPIClass attached to vspi
    vspi = new SPIClass(VSPI);

    // initialize the SPI port
    vspi->begin(_clkPin, -1, _rDataPin, -1);

    // setup pin mode
    pinMode(_aPin, OUTPUT);
    pinMode(_bPin, OUTPUT);
    pinMode(_latPin, OUTPUT); 
    pinMode(_nOEPin, OUTPUT);
    pinMode(PIN_OTHER_SPI_nCS, INPUT);

    // initial GPIO state
    digitalWrite(_aPin, LOW);
    digitalWrite(_bPin, LOW);
    digitalWrite(_latPin, LOW);
    digitalWrite(_nOEPin, LOW); 

    clearScreen(true);
    marqueeNoSpacing = false;

    // init the scan line/ram pointer to the required start point
    bDMDByte = 0;
}

// DMD::~DMD()
//{
//    // nothing needed here
// }

/*--------------------------------------------------------------------------------------
 Set or clear a pixel at the x and y location (0,0 is the top left corner)
--------------------------------------------------------------------------------------*/
void DMD::writePixel(unsigned int bX, unsigned int bY, byte bGraphicsMode, byte bPixel)
{
    unsigned int uiDMDRAMPointer;

    if (bX >= (DMD_PIXELS_ACROSS * DisplaysWide) || bY >= (DMD_PIXELS_DOWN * DisplaysHigh))
    {
        return;
    }
    byte panel = (bX / DMD_PIXELS_ACROSS) + (DisplaysWide * (bY / DMD_PIXELS_DOWN));
    bX = (bX % DMD_PIXELS_ACROSS) + (panel << 5);
    bY = bY % DMD_PIXELS_DOWN;
    // set pointer to DMD RAM byte to be modified
    uiDMDRAMPointer = bX / 8 + bY * (DisplaysTotal << 2);

    byte lookup = bPixelLookupTable[bX & 0x07];

    switch (bGraphicsMode)
    {
    case GRAPHICS_NORMAL:
        if (bPixel == true)
            bDMDScreenRAM[uiDMDRAMPointer] &= ~lookup; // zero bit is pixel on
        else
            bDMDScreenRAM[uiDMDRAMPointer] |= lookup; // one bit is pixel off
        break;
    case GRAPHICS_INVERSE:
        if (bPixel == false)
            bDMDScreenRAM[uiDMDRAMPointer] &= ~lookup; // zero bit is pixel on
        else
            bDMDScreenRAM[uiDMDRAMPointer] |= lookup; // one bit is pixel off
        break;
    case GRAPHICS_TOGGLE:
        if (bPixel == true)
        {
            if ((bDMDScreenRAM[uiDMDRAMPointer] & lookup) == 0)
                bDMDScreenRAM[uiDMDRAMPointer] |= lookup; // one bit is pixel off
            else
                bDMDScreenRAM[uiDMDRAMPointer] &= ~lookup; // one bit is pixel off
        }
        break;
    case GRAPHICS_OR:
        // only set pixels on
        if (bPixel == true)
            bDMDScreenRAM[uiDMDRAMPointer] &= ~lookup; // zero bit is pixel on
        break;
    case GRAPHICS_NOR:
        // only clear on pixels
        if ((bPixel == true) &&
            ((bDMDScreenRAM[uiDMDRAMPointer] & lookup) == 0))
            bDMDScreenRAM[uiDMDRAMPointer] |= lookup; // one bit is pixel off
        break;
    }
}

void DMD::drawString(int bX, int bY, const char *bChars, byte length,
                     byte bGraphicsMode)
{
    if (bX >= (DMD_PIXELS_ACROSS * DisplaysWide) || bY >= DMD_PIXELS_DOWN * DisplaysHigh)
        return;
    uint8_t height = pgm_read_byte(this->Font + FONT_HEIGHT);
    if (bY + height < 0)
        return;

    int strWidth = 0;
    this->drawLine(bX - 1, bY, bX - 1, bY + height, GRAPHICS_INVERSE);

    for (int i = 0; i < length; i++)
    {
        int charWide = this->drawChar(bX + strWidth, bY, bChars[i], bGraphicsMode);
        if (charWide > 0)
        {
            strWidth += charWide;
            this->drawLine(bX + strWidth, bY, bX + strWidth, bY + height, GRAPHICS_INVERSE);
            strWidth++;
        }
        else if (charWide < 0)
        {
            return;
        }
        if ((bX + strWidth) >= DMD_PIXELS_ACROSS * DisplaysWide || bY >= DMD_PIXELS_DOWN * DisplaysHigh)
            return;
    }
}

void DMD::drawStringCompact(int bX, int bY, const char *bChars, byte length,
                            byte bGraphicsMode)
{
    if (bX >= (DMD_PIXELS_ACROSS * DisplaysWide) || bY >= DMD_PIXELS_DOWN * DisplaysHigh)
        return;
    uint8_t height = pgm_read_byte(this->Font + FONT_HEIGHT);
    if (bY + height < 0)
        return;

    int strWidth = 0;
    for (int i = 0; i < length; i++)
    {
        int charWide = this->drawChar(bX + strWidth, bY, bChars[i], bGraphicsMode);
        if (charWide > 0)
        {
            strWidth += charWide;
        }
        else if (charWide < 0)
        {
            return;
        }
        if ((bX + strWidth) >= DMD_PIXELS_ACROSS * DisplaysWide || bY >= DMD_PIXELS_DOWN * DisplaysHigh)
            return;
    }
}

void DMD::drawStringRTL(int rightX, int bY, const char *bChars, byte length, byte bGraphicsMode)
{
    if (bY >= DMD_PIXELS_DOWN * DisplaysHigh)
        return;
    uint8_t height = pgm_read_byte(this->Font + FONT_HEIGHT);
    if (bY + height < 0)
        return;

    int screenW = DMD_PIXELS_ACROSS * DisplaysWide;
    int cursorX = rightX;
    for (int i = 0; i < length; i++)
    {
        unsigned char c = (unsigned char)bChars[i];
        int charWide = this->charWidth(c);
        if (charWide <= 0)
        {
            continue;
        }

        cursorX -= charWide;
        // Draw only if partially on-screen
        if (cursorX < screenW && cursorX >= -charWide)
        {
            this->drawChar(cursorX, bY, c, bGraphicsMode);
        }
        cursorX -= 1;

        // All remaining chars would be further left, so stop
        if (cursorX < -screenW)
        {
            return;
        }
    }
}

uint16_t DMD::utf8ToArabic(const char *utf8Text, char *outBuffer, uint16_t outBufferSize)
{
    if (!utf8Text || !outBuffer || outBufferSize == 0)
    {
        return 0;
    }

    uint16_t codepoints[256];
    uint16_t codepointCount = 0;
    const uint8_t *src = (const uint8_t *)utf8Text;

    while (*src && codepointCount < (sizeof(codepoints) / sizeof(codepoints[0])))
    {
        uint16_t codepoint = 0;
        if (decodeNextUtf8Codepoint(src, codepoint))
        {
            codepoints[codepointCount++] = codepoint;
        }
    }

    uint16_t outLen = 0;
    for (uint16_t i = 0; i < codepointCount && outLen < (outBufferSize - 1); i++)
    {
        uint16_t cp = codepoints[i];
        uint8_t mapped = 0;

        if (cp == 0x0644 && (i + 1) < codepointCount)
        {
            uint16_t nextCp = codepoints[i + 1];
            if (nextCp == 0x0627 || nextCp == 0x0622 || nextCp == 0x0623 || nextCp == 0x0625)
            {
                const ArabicLetterForm *prev = (i > 0) ? findArabicForm(codepoints[i - 1]) : NULL;
                bool joinWithPrev = (prev != NULL) && prev->joinAfter;
                mapped = joinWithPrev ? ARABIC_GLYPH_LAM_ALEF_FINAL : ARABIC_GLYPH_LAM_ALEF_ISO;
                i++;
            }
        }

        if (mapped == 0)
        {
            const ArabicLetterForm *curr = findArabicForm(cp);
            if (curr != NULL)
            {
                const ArabicLetterForm *prev = (i > 0) ? findArabicForm(codepoints[i - 1]) : NULL;
                const ArabicLetterForm *next = (i + 1 < codepointCount) ? findArabicForm(codepoints[i + 1]) : NULL;

                bool joinWithPrev = (prev != NULL) && prev->joinAfter && curr->joinBefore;
                bool joinWithNext = (next != NULL) && curr->joinAfter && next->joinBefore;

                if (joinWithPrev && joinWithNext)
                {
                    mapped = curr->medial;
                }
                else if (joinWithPrev)
                {
                    mapped = curr->final;
                }
                else if (joinWithNext)
                {
                    mapped = curr->initial;
                }
                else
                {
                    mapped = curr->isolated;
                }
            }
            else
            {
                mapped = mapArabicSymbolCodepoint(cp);
            }
        }

        if (mapped != 0)
        {
            outBuffer[outLen++] = (char)mapped;
        }
    }

    outBuffer[outLen] = '\0';
    return outLen;
}

static void reverseArabicVisual(char *buf, uint16_t len)
{
    // Reverse entire buffer for RTL visual order
    for (uint16_t i = 0; i < len / 2; i++)
    {
        char tmp = buf[i];
        buf[i] = buf[len - 1 - i];
        buf[len - 1 - i] = tmp;
    }
    
    // Re-reverse Latin/digit sequences so they read LTR within RTL text
    uint16_t i = 0;
    while (i < len)
    {
        // Check if this is a Latin character (ASCII 0x20-0x7E) or digit
        uint8_t ch = (uint8_t)buf[i];
        if (ch >= 0x20 && ch <= 0x7E)
        {
            uint16_t start = i;
            // Find the end of this Latin sequence
            while (i < len && (uint8_t)buf[i] >= 0x20 && (uint8_t)buf[i] <= 0x7E)
            {
                i++;
            }
            // Reverse this Latin sequence back to LTR
            for (uint16_t a = start, b = i - 1; a < b; a++, b--)
            {
                char tmp = buf[a];
                buf[a] = buf[b];
                buf[b] = tmp;
            }
        }
        else
        {
            i++;
        }
    }
}

void DMD::drawArabicString(int bX, int bY, const char *utf8Text, byte bGraphicsMode)
{
    char mappedText[256];
    uint16_t mappedLength = utf8ToArabic(utf8Text, mappedText, sizeof(mappedText));
    if (mappedLength > 255)
    {
        mappedLength = 255;
    }
    reverseArabicVisual(mappedText, mappedLength);
    drawStringCompact(bX, bY, mappedText, (byte)mappedLength, bGraphicsMode);
}

void DMD::drawArabicMarquee(const char *utf8Text, int left, int top)
{
    char mappedText[256];
    uint16_t mappedLength = utf8ToArabic(utf8Text, mappedText, sizeof(mappedText));
    if (mappedLength > 255)
    {
        mappedLength = 255;
    }
    reverseArabicVisual(mappedText, mappedLength);
    marqueeNoSpacing = true;
    marqueeWidth = 0;
    for (uint16_t i = 0; i < mappedLength; i++)
    {
        marqueeText[i] = mappedText[i];
        marqueeWidth += charWidth(mappedText[i]);
    }
    marqueeHeight = pgm_read_byte(this->Font + FONT_HEIGHT);
    marqueeText[mappedLength] = '\0';
    marqueeOffsetY = top;
    marqueeOffsetX = left;
    marqueeLength = (byte)mappedLength;
    drawStringCompact(marqueeOffsetX, marqueeOffsetY, marqueeText, marqueeLength, GRAPHICS_NORMAL);
}

void DMD::drawMarquee(const char *bChars, byte length, int left, int top)
{
    marqueeNoSpacing = false;
    marqueeWidth = 0;
    for (int i = 0; i < length; i++)
    {
        marqueeText[i] = bChars[i];
        marqueeWidth += charWidth(bChars[i]);
        if (!marqueeNoSpacing)
        {
            marqueeWidth += 1;
        }
    }
    marqueeHeight = pgm_read_byte(this->Font + FONT_HEIGHT);
    marqueeText[length] = '\0';
    marqueeOffsetY = top;
    marqueeOffsetX = left;
    marqueeLength = length;
    if (marqueeNoSpacing)
    {
        drawStringCompact(marqueeOffsetX, marqueeOffsetY, marqueeText, marqueeLength,
                          GRAPHICS_NORMAL);
    }
    else
    {
        drawString(marqueeOffsetX, marqueeOffsetY, marqueeText, marqueeLength,
                   GRAPHICS_NORMAL);
    }
}

boolean DMD::stepMarquee(int amountX, int amountY)
{
    boolean ret = false;
    marqueeOffsetX += amountX;
    marqueeOffsetY += amountY;
    if (marqueeOffsetX < -marqueeWidth)
    {
        marqueeOffsetX = DMD_PIXELS_ACROSS * DisplaysWide;
        clearScreen(true);
        ret = true;
    }
    else if (marqueeOffsetX > DMD_PIXELS_ACROSS * DisplaysWide)
    {
        marqueeOffsetX = -marqueeWidth;
        clearScreen(true);
        ret = true;
    }

    if (marqueeOffsetY < -marqueeHeight)
    {
        marqueeOffsetY = DMD_PIXELS_DOWN * DisplaysHigh;
        clearScreen(true);
        ret = true;
    }
    else if (marqueeOffsetY > DMD_PIXELS_DOWN * DisplaysHigh)
    {
        marqueeOffsetY = -marqueeHeight;
        clearScreen(true);
        ret = true;
    }

    // Special case horizontal scrolling to improve speed
    if (amountY == 0 && amountX == -1)
    {
        // Shift entire screen one bit
        for (int i = 0; i < DMD_RAM_SIZE_BYTES * DisplaysTotal; i++)
        {
            if ((i % (DisplaysWide * 4)) == (DisplaysWide * 4) - 1)
            {
                bDMDScreenRAM[i] = (bDMDScreenRAM[i] << 1) + 1;
            }
            else
            {
                bDMDScreenRAM[i] = (bDMDScreenRAM[i] << 1) + ((bDMDScreenRAM[i + 1] & 0x80) >> 7);
            }
        }

        // Redraw last char on screen
        int strWidth = marqueeOffsetX;
        for (byte i = 0; i < marqueeLength; i++)
        {
            int wide = charWidth(marqueeText[i]);
            if (strWidth + wide >= DisplaysWide * DMD_PIXELS_ACROSS)
            {
                drawChar(strWidth, marqueeOffsetY, marqueeText[i], GRAPHICS_NORMAL);
                return ret;
            }
            strWidth += wide;
            if (!marqueeNoSpacing)
            {
                strWidth += 1;
            }
        }
    }
    else if (amountY == 0 && amountX == 1)
    {
        // Shift entire screen one bit
        for (int i = (DMD_RAM_SIZE_BYTES * DisplaysTotal) - 1; i >= 0; i--)
        {
            if ((i % (DisplaysWide * 4)) == 0)
            {
                bDMDScreenRAM[i] = (bDMDScreenRAM[i] >> 1) + 128;
            }
            else
            {
                bDMDScreenRAM[i] = (bDMDScreenRAM[i] >> 1) + ((bDMDScreenRAM[i - 1] & 1) << 7);
            }
        }

        // Redraw last char on screen
        int strWidth = marqueeOffsetX;
        for (byte i = 0; i < marqueeLength; i++)
        {
            int wide = charWidth(marqueeText[i]);
            if (strWidth + wide >= 0)
            {
                drawChar(strWidth, marqueeOffsetY, marqueeText[i], GRAPHICS_NORMAL);
                return ret;
            }
            strWidth += wide;
            if (!marqueeNoSpacing)
            {
                strWidth += 1;
            }
        }
    }
    else
    {
        if (marqueeNoSpacing)
        {
            drawStringCompact(marqueeOffsetX, marqueeOffsetY, marqueeText, marqueeLength,
                              GRAPHICS_NORMAL);
        }
        else
        {
            drawString(marqueeOffsetX, marqueeOffsetY, marqueeText, marqueeLength,
                       GRAPHICS_NORMAL);
        }
    }

    return ret;
}

/*--------------------------------------------------------------------------------------
 Clear the screen in DMD RAM
--------------------------------------------------------------------------------------*/
void DMD::clearScreen(byte bNormal)
{
    if (bNormal) // clear all pixels
        memset(bDMDScreenRAM, 0xFF, DMD_RAM_SIZE_BYTES * DisplaysTotal);
    else // set all pixels
        memset(bDMDScreenRAM, 0x00, DMD_RAM_SIZE_BYTES * DisplaysTotal);
}

/*--------------------------------------------------------------------------------------
 Draw or clear a line from x1,y1 to x2,y2
--------------------------------------------------------------------------------------*/
void DMD::drawLine(int x1, int y1, int x2, int y2, byte bGraphicsMode)
{
    int dy = y2 - y1;
    int dx = x2 - x1;
    int stepx, stepy;

    if (dy < 0)
    {
        dy = -dy;
        stepy = -1;
    }
    else
    {
        stepy = 1;
    }
    if (dx < 0)
    {
        dx = -dx;
        stepx = -1;
    }
    else
    {
        stepx = 1;
    }
    dy <<= 1; // dy is now 2*dy
    dx <<= 1; // dx is now 2*dx

    writePixel(x1, y1, bGraphicsMode, true);
    if (dx > dy)
    {
        int fraction = dy - (dx >> 1); // same as 2*dy - dx
        while (x1 != x2)
        {
            if (fraction >= 0)
            {
                y1 += stepy;
                fraction -= dx; // same as fraction -= 2*dx
            }
            x1 += stepx;
            fraction += dy; // same as fraction -= 2*dy
            writePixel(x1, y1, bGraphicsMode, true);
        }
    }
    else
    {
        int fraction = dx - (dy >> 1);
        while (y1 != y2)
        {
            if (fraction >= 0)
            {
                x1 += stepx;
                fraction -= dy;
            }
            y1 += stepy;
            fraction += dx;
            writePixel(x1, y1, bGraphicsMode, true);
        }
    }
}

/*--------------------------------------------------------------------------------------
 Draw or clear a circle of radius r at x,y centre
--------------------------------------------------------------------------------------*/
void DMD::drawCircle(int xCenter, int yCenter, int radius,
                     byte bGraphicsMode)
{
    int x = 0;
    int y = radius;
    int p = (5 - radius * 4) / 4;

    drawCircleSub(xCenter, yCenter, x, y, bGraphicsMode);
    while (x < y)
    {
        x++;
        if (p < 0)
        {
            p += 2 * x + 1;
        }
        else
        {
            y--;
            p += 2 * (x - y) + 1;
        }
        drawCircleSub(xCenter, yCenter, x, y, bGraphicsMode);
    }
}

void DMD::drawCircleSub(int cx, int cy, int x, int y, byte bGraphicsMode)
{

    if (x == 0)
    {
        writePixel(cx, cy + y, bGraphicsMode, true);
        writePixel(cx, cy - y, bGraphicsMode, true);
        writePixel(cx + y, cy, bGraphicsMode, true);
        writePixel(cx - y, cy, bGraphicsMode, true);
    }
    else if (x == y)
    {
        writePixel(cx + x, cy + y, bGraphicsMode, true);
        writePixel(cx - x, cy + y, bGraphicsMode, true);
        writePixel(cx + x, cy - y, bGraphicsMode, true);
        writePixel(cx - x, cy - y, bGraphicsMode, true);
    }
    else if (x < y)
    {
        writePixel(cx + x, cy + y, bGraphicsMode, true);
        writePixel(cx - x, cy + y, bGraphicsMode, true);
        writePixel(cx + x, cy - y, bGraphicsMode, true);
        writePixel(cx - x, cy - y, bGraphicsMode, true);
        writePixel(cx + y, cy + x, bGraphicsMode, true);
        writePixel(cx - y, cy + x, bGraphicsMode, true);
        writePixel(cx + y, cy - x, bGraphicsMode, true);
        writePixel(cx - y, cy - x, bGraphicsMode, true);
    }
}

/*--------------------------------------------------------------------------------------
 Draw or clear a box(rectangle) with a single pixel border
--------------------------------------------------------------------------------------*/
void DMD::drawBox(int x1, int y1, int x2, int y2, byte bGraphicsMode)
{
    drawLine(x1, y1, x2, y1, bGraphicsMode);
    drawLine(x2, y1, x2, y2, bGraphicsMode);
    drawLine(x2, y2, x1, y2, bGraphicsMode);
    drawLine(x1, y2, x1, y1, bGraphicsMode);
}

/*--------------------------------------------------------------------------------------
 Draw or clear a filled box(rectangle) with a single pixel border
--------------------------------------------------------------------------------------*/
void DMD::drawFilledBox(int x1, int y1, int x2, int y2,
                        byte bGraphicsMode)
{
    for (int b = x1; b <= x2; b++)
    {
        drawLine(b, y1, b, y2, bGraphicsMode);
    }
}

/*--------------------------------------------------------------------------------------
 Draw the selected test pattern
--------------------------------------------------------------------------------------*/
void DMD::drawTestPattern(byte bPattern)
{
    unsigned int ui;

    int numPixels = DisplaysTotal * DMD_PIXELS_ACROSS * DMD_PIXELS_DOWN;
    int pixelsWide = DMD_PIXELS_ACROSS * DisplaysWide;
    for (ui = 0; ui < numPixels; ui++)
    {
        switch (bPattern)
        {
        case PATTERN_ALT_0: // every alternate pixel, first pixel on
            if ((ui & pixelsWide) == 0)
                // even row
                writePixel((ui & (pixelsWide - 1)), ((ui & ~(pixelsWide - 1)) / pixelsWide), GRAPHICS_NORMAL, ui & 1);
            else
                // odd row
                writePixel((ui & (pixelsWide - 1)), ((ui & ~(pixelsWide - 1)) / pixelsWide), GRAPHICS_NORMAL, !(ui & 1));
            break;
        case PATTERN_ALT_1: // every alternate pixel, first pixel off
            if ((ui & pixelsWide) == 0)
                // even row
                writePixel((ui & (pixelsWide - 1)), ((ui & ~(pixelsWide - 1)) / pixelsWide), GRAPHICS_NORMAL, !(ui & 1));
            else
                // odd row
                writePixel((ui & (pixelsWide - 1)), ((ui & ~(pixelsWide - 1)) / pixelsWide), GRAPHICS_NORMAL, ui & 1);
            break;
        case PATTERN_STRIPE_0: // vertical stripes, first stripe on
            writePixel((ui & (pixelsWide - 1)), ((ui & ~(pixelsWide - 1)) / pixelsWide), GRAPHICS_NORMAL, ui & 1);
            break;
        case PATTERN_STRIPE_1: // vertical stripes, first stripe off
            writePixel((ui & (pixelsWide - 1)), ((ui & ~(pixelsWide - 1)) / pixelsWide), GRAPHICS_NORMAL, !(ui & 1));
            break;
        }
    }
}

/*--------------------------------------------------------------------------------------
 Scan the dot matrix LED panel display, from the RAM mirror out to the display hardware.
 Call 4 times to scan the whole display which is made up of 4 interleaved rows within the 16 total rows.
 Insert the calls to this function into the main loop for the highest call rate, or from a timer interrupt
--------------------------------------------------------------------------------------*/
void DMD::scanDisplayBySPI()
{
    // if PIN_OTHER_SPI_nCS is in use during a DMD scan request then scanDisplayBySPI() will exit without conflict! (and skip that scan)
    if (digitalRead(PIN_OTHER_SPI_nCS) == HIGH)
    {
        // SPI transfer pixels to the display hardware shift registers
        int rowsize = DisplaysTotal << 2;
        int offset = rowsize * bDMDByte;
        for (int i = 0; i < rowsize; i++)
        {
            vspi->beginTransaction(SPISettings(spiClk, MSBFIRST, SPI_MODE0));
            vspi->transfer(bDMDScreenRAM[offset + i + row3]);
            vspi->transfer(bDMDScreenRAM[offset + i + row2]);
            vspi->transfer(bDMDScreenRAM[offset + i + row1]);
            vspi->transfer(bDMDScreenRAM[offset + i]);
            vspi->endTransaction();
        }

        oeRowsOff();
        latchShiftRegToOutput();
        switch (bDMDByte)
        {
        case 0: // row 1, 5, 9, 13 were clocked out
            lightRow_01_05_09_13();
            bDMDByte = 1;
            break;
        case 1: // row 2, 6, 10, 14 were clocked out
            lightRow_02_06_10_14();
            bDMDByte = 2;
            break;
        case 2: // row 3, 7, 11, 15 were clocked out
            lightRow_03_07_11_15();
            bDMDByte = 3;
            break;
        case 3: // row 4, 8, 12, 16 were clocked out
            lightRow_04_08_12_16();
            bDMDByte = 0;
            break;
        }
        oeRowsOn();
    }
}

void DMD::selectFont(const uint8_t *font)
{
    this->Font = font;
}

int DMD::drawChar(const int bX, const int bY, const unsigned char letter, byte bGraphicsMode)
{
    if (bX > (DMD_PIXELS_ACROSS * DisplaysWide) || bY > (DMD_PIXELS_DOWN * DisplaysHigh))
        return -1;
    unsigned char c = letter;
    uint8_t height = pgm_read_byte(this->Font + FONT_HEIGHT);
    if (c == ' ')
    {
        int charWide = charWidth(' ');
        this->drawFilledBox(bX, bY, bX + charWide, bY + height, GRAPHICS_INVERSE);
        return charWide;
    }
    uint8_t width = 0;
    uint8_t bytes = (height + 7) / 8;

    uint8_t firstChar = pgm_read_byte(this->Font + FONT_FIRST_CHAR);
    uint8_t charCount = pgm_read_byte(this->Font + FONT_CHAR_COUNT);

    uint16_t index = 0;

    if (c < firstChar || c >= (firstChar + charCount))
        return 0;
    c -= firstChar;

    if (pgm_read_byte(this->Font + FONT_LENGTH) == 0 && pgm_read_byte(this->Font + FONT_LENGTH + 1) == 0)
    {
        // zero length is flag indicating fixed width font (array does not contain width data entries)
        width = pgm_read_byte(this->Font + FONT_FIXED_WIDTH);
        index = c * bytes * width + FONT_WIDTH_TABLE;
    }
    else
    {
        // variable width font, read width data, to get the index
        for (uint8_t i = 0; i < c; i++)
        {
            index += pgm_read_byte(this->Font + FONT_WIDTH_TABLE + i);
        }
        index = index * bytes + charCount + FONT_WIDTH_TABLE;
        width = pgm_read_byte(this->Font + FONT_WIDTH_TABLE + c);
    }
    if (bX < -width || bY < -height)
        return width;

    // last but not least, draw the character
    for (uint8_t j = 0; j < width; j++)
    { // Width
        for (uint8_t i = bytes - 1; i < 254; i--)
        { // Vertical Bytes
            uint8_t data = pgm_read_byte(this->Font + index + j + (i * width));
            int offset = (i * 8);
            if ((i == bytes - 1) && bytes > 1)
            {
                offset = height - 8;
            }
            for (uint8_t k = 0; k < 8; k++)
            { // Vertical bits
                if ((offset + k >= i * 8) && (offset + k <= height))
                {
                    if (data & (1 << k))
                    {
                        writePixel(bX + j, bY + offset + k, bGraphicsMode, true);
                    }
                    else
                    {
                        writePixel(bX + j, bY + offset + k, bGraphicsMode, false);
                    }
                }
            }
        }
    }
    return width;
}

int DMD::charWidth(const unsigned char letter)
{
    return charWidthOfFont(letter, this->Font);
}

void DMD::drawContainer(DMDContainer *container)
{
    int16_t x0 = container->getX0();
    int16_t y0 = container->getY0();
    int16_t w = container->getW();
    int16_t h = container->getH();
    uint8_t* buf = container->getBufferData();

    for (uint16_t i = 0; i < w; i++)
    {
        for (uint16_t j = 0; j < h; j++)
        {
            writePixel(i + x0 - 1, j + y0, GRAPHICS_NORMAL, buf[j * w + i]);
        }
    }
}
