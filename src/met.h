#pragma once

#include "esphome.h"


// /config/esphome/src/met.h

// ... existing code ...

#include "driver/gpio.h" // <<< IMPORTANT: Include the necessary ESP-IDF GPIO header

// ... existing function declarations ...

void backlight_on_for_ota(bool on); // <<< ADD THIS FUNCTION DECLARATION

// DSEG placeholder character for 7-segment displays
// This character shows all segments lit (creates the dark background effect)
const char DSEG_PLACEHOLDER = '~';  // Use '8' if '~' doesn't work with your DSEG font

enum TextAlignment {
    ALIGN_LEFT,
    ALIGN_CENTER,
    ALIGN_RIGHT
};

void drawDsegText(
        esphome::display::Display& display,
        int x1,
        int x2,
        int y1,
        int y2,
        esphome::font::Font *pFont,
        const std::string& text,
        esphome::Color color = esphome::Color(255, 255, 255),
        TextAlignment alignment = ALIGN_CENTER
);


void drawBottomBoxes(  
        esphome::display::Display &it, 
        esphome::font::Font *pFontValue, 
        esphome::font::Font *pFontWeather, 
        float fTemperature, 
        float fHumidity = 0.0, 
        float fPression = 0.0 );                   

void menuDrawAboutPage(
    esphome::display::Display &it,
    esphome::font::Font *pFont,
    bool bMonoChrome = false
);

void drawProgressPage( 
        esphome::display::Display &it,
        esphome::font::Font *pFont,
        const std::string& sText,
        float fValue = 0.0,
        bool bMonoChrome = false
 );

// Draw time and date using DSEG fonts. Positions match existing layout (y ranges used in lambdas).
// Pass the time component for current time.
void drawTimeAndDate(
    esphome::display::Display &it,
    esphome::font::Font *pFontTime,
    esphome::font::Font *pFontDate,
    esphome::time::RealTimeClock *pTime
);

// Draw weather icon(s) and moon phase.
// Day: a single weather symbol centered. Night: weather and moon symbols placed in the left/right halves.
void drawWeatherAndMoon(
    esphome::display::Display &it,
    esphome::font::Font *pFontWeather,
    esphome::font::Font *pFontMicro,
    esphome::text_sensor::TextSensor *pWeatherCondition,
    esphome::text_sensor::TextSensor *pMoonPhaseIcon,
    esphome::time::RealTimeClock *pTime
);

// Small helper class for UI utilities. Methods are deliberately small
// so you can call them from lambdas like `met.drawOptions(...)`.
class cMet {

private:        
        bool bMono = false;  // false by default        
        int iScreenWidth = 0;
        int iScreenHeight = 0;
        int iPageTop = 0;
        esphome::time::RealTimeClock *pTime;

public:
    cMet() {}

    void init(
        esphome::display::Display &it, 
        bool bMono,
        esphome::time::RealTimeClock *pTime
    );

    // Normalize typical Home Assistant weather condition strings
    std::string normalizeCondition(const std::string &cond) const;

    void drawPageTitle(    
        esphome::display::Display &it, 
        esphome::font::Font *pFont,
        const std::string &sText, 
        bool bSelected = false,
        bool bTime = false);

    // Draw options list with a selection marker.
    // `labels` should contain the text for each option (up to max_options).
    void drawOptionsPage(esphome::display::Display &it,
                         esphome::font::Font *pFont,
                         int selected_option,
                         esphome::sensor::Sensor *pSensor1 = nullptr,
                         esphome::sensor::Sensor *pSensor2 = nullptr,
                         esphome::sensor::Sensor *pSensor3 = nullptr ) const;

};

// Global instance you can call from lambdas
extern cMet met;
