/**
 *  @file met.h
 *  @brief package handling display and menu functions for MET display
 *  @author CmPi <github.com/CmPi>
 */

#pragma once

#include "esphome.h"
#include <cmath>
#include "driver/gpio.h"

namespace esphome {

// DSEG placeholder character for 7-segment displays
// This character shows all segments lit (creates the dark background effect)
const char DSEG_PLACEHOLDER = '~';  // Use '8' if '~' doesn't work with your DSEG font

enum TextAlignment {
    ALIGN_LEFT,
    ALIGN_CENTER,
    ALIGN_RIGHT
};

// All fonts used by cMet, handed over once in init() so draw calls don't need them.
struct MetFonts {
    esphome::font::Font *title   = nullptr;  // page title, options list, progress page (e.g. 8x16)
    esphome::font::Font *value   = nullptr;  // bottom boxes values, about page (e.g. 6x12)
    esphome::font::Font *icons   = nullptr;  // icons in bottom boxes
    esphome::font::Font *weather = nullptr;  // weather / moon glyphs
    esphome::font::Font *micro   = nullptr;  // small text (weather condition string)
    esphome::font::Font *dsegTime = nullptr; // DSEG font for HH:MM:SS
    esphome::font::Font *dsegDate = nullptr; // DSEG font for DD/MM/YYYY
};

// UI helper class. Call init() at the start of the display lambda (it is cheap): it stores
// the display, fonts, clock and size. Then use the draw* methods, which draw on that display
// and do nothing (instead of crashing) if init() has not been called yet.
class cMet {

private:
    MetFonts fonts;
    bool bMono = false;
    int iScreenWidth = 0;
    int iScreenHeight = 0;
    int iPageTop = 0;               // first free y below the page title (computed in init)
    int iBlPin = 4;                 // backlight GPIO, used by backlightOnForOta()
    esphome::time::RealTimeClock *pTime = nullptr;
    esphome::display::Display *pDisplay = nullptr;  // set by init(); all draw*() methods draw on it

    // true once init() has provided a display and a usable size
    bool ready() const { return pDisplay != nullptr && iScreenWidth > 0 && iScreenHeight > 0; }

public:
    cMet() {}

    /**
     * @param it          display to draw on (a pointer is kept: it must outlive this object)
     * @param f           fonts used by all draw calls
     * @param mono        true for monochrome displays
     * @param pClock      time source (may be nullptr)
     * @param realWidth   logical width  (<= 0: use display width)
     * @param realHeight  logical height (<= 0: use display height)
     * @param blPin       backlight GPIO (< 0: keep default)
     */
    void init(
        esphome::display::Display &it,
        const MetFonts &f,
        bool mono,
        esphome::time::RealTimeClock *pClock,
        int realWidth,
        int realHeight,
        int blPin = -1
    );

    // Drive the backlight pin directly (works during OTA on_begin, bypasses the light component).
    void backlightOnForOta(bool on) const;

    void drawDsegText(
        int x1,
        int x2,
        int y1,
        int y2,
        esphome::font::Font *pFont,
        const std::string& text,
        esphome::Color color = esphome::Color(255, 255, 255),
        TextAlignment alignment = ALIGN_CENTER
    );

    // Bottom boxes. A NaN humidity / pressure hides that box; a NaN temperature shows "--.-".
    void drawBottomBoxes(
        float fTemperature,
        float fHumidity = NAN,
        float fPression = NAN );

    // Same, straight from sensors (nullptr or no state yet = NaN).
    void drawBottomBoxes(
        esphome::sensor::Sensor *pTemperature,
        esphome::sensor::Sensor *pHumidity = nullptr,
        esphome::sensor::Sensor *pPression = nullptr );

    void menuDrawAboutPage();

    void drawProgressPage(
        const std::string& sText,
        float fValue = 0.0
    );

    // Draw time and date using the DSEG fonts.
    void drawTimeAndDate();

    // Draw weather icon(s) and moon phase.
    // Day: a single weather symbol centered. Night: weather and moon symbols placed in the left/right halves.
    void drawWeatherAndMoon(
        esphome::text_sensor::TextSensor *pWeatherCondition,
        esphome::text_sensor::TextSensor *pMoonPhaseIcon = nullptr
    );

    // Normalize typical Home Assistant weather condition strings
    static std::string normalizeCondition(const std::string &cond);

    // Clears the screen and draws the title. Page content starts below it.
    void drawPageTitle(
        const std::string &sText,
        bool bSelected = false,
        bool bTime = false);

    // Draw options list (state on the left of the divider, sensor name on the right)
    // with a selection marker.
    void drawOptionsPage(int selected_option,
                         esphome::sensor::Sensor *pSensor1 = nullptr,
                         esphome::sensor::Sensor *pSensor2 = nullptr,
                         esphome::sensor::Sensor *pSensor3 = nullptr ) const;

};

// Global instance you can call from lambdas
extern cMet met;

}