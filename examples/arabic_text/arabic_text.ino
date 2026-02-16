/*--------------------------------------------------------------------------------------
  arabic_text

  Demo for Arabic text display on DMD32Plus.
  Uses the built-in ArabicFont (generated from tahoma.ttf, 16px) and the
  drawArabicMarquee / stepMarquee API to scroll UTF-8 Arabic text across
  the display.
--------------------------------------------------------------------------------------*/

#include <DMD32Plus.h>
#include "fonts/ArabicFont.h"

#define DISPLAYS_ACROSS 1
#define DISPLAYS_DOWN 1
DMD dmd(DISPLAYS_ACROSS, DISPLAYS_DOWN);

hw_timer_t *timer = NULL;

void IRAM_ATTR triggerScan()
{
  dmd.scanDisplayBySPI();
}

void setup(void)
{
  timer = timerBegin(1000000L);
  timerAttachInterrupt(timer, &triggerScan);
  timerAlarm(timer, 1000, true, 0);

  dmd.clearScreen(true);
  dmd.selectFont(ArabicFont);

  // UTF-8 Arabic string (hex escapes to avoid source-file encoding issues):
  // "السلام عليكم 2026"
  const char *msg = "\xd8\xa7\xd9\x84\xd8\xb3\xd9\x84\xd8\xa7\xd9\x85"
                    " "
                    "\xd8\xb9\xd9\x84\xd9\x8a\xd9\x83\xd9\x85"
                    " 2026";

  // drawArabicMarquee converts UTF-8 to glyph codes, reverses to visual
  // RTL order, and feeds the result into the library's marquee engine.
  dmd.drawArabicMarquee(msg, (32 * DISPLAYS_ACROSS) - 1, 0);
}

void loop(void)
{
  static unsigned long t = 0;
  if (millis() - t >= 40)
  {
    t = millis();
    dmd.stepMarquee(-1, 0);
  }
}
