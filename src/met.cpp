/**
 *  @file met.cpp
 *  @brief package handling display and menu functions for MET display
 *  @author CmPi <github.com/CmPi>
 */

#include "esphome.h"
#include "met.h"
#include "driver/gpio.h"

namespace esphome {

// ---------------- file-local helpers ----------------

namespace {

/**
 * Calculate the width needed for an icon + text box
 * If iconFont is nullptr, only calculates text width (no icon)
 */
int calcIconTextBoxWidth(
    esphome::display::Display &it,
    const char *icon,
    const char *text,
    esphome::font::Font *iconFont,
    esphome::font::Font *textFont,
    int iconTextSpacing,
    int horizontalPadding
) {
    int bx, by, bw, bh;
    int iconW = 0;

    if (iconFont != nullptr) {
        it.get_text_bounds(0, 0, icon, iconFont, esphome::display::TextAlign::TOP_LEFT,
                          &bx, &by, &bw, &bh);
        iconW = bw;
    }

    it.get_text_bounds(0, 0, text, textFont, esphome::display::TextAlign::TOP_LEFT,
                      &bx, &by, &bw, &bh);
    int textW = bw;

    int spacing = (iconFont != nullptr) ? iconTextSpacing : 0;

    return iconW + spacing + textW + (horizontalPadding * 2);
}

/**
 * Draw icon (left) and text (right) within a box area with vertical centering
 * Content is always centered horizontally within the box
 */
void drawIconTextContent(
    esphome::display::Display &it,
    int boxX,
    int boxY,
    int boxWidth,
    int boxHeight,
    const char *icon,
    const char *text,
    esphome::font::Font *iconFont,
    esphome::font::Font *textFont,
    esphome::Color iconColor,
    esphome::Color textColor,
    int iconTextSpacing
) {
    int bx, by, bw, bh;

    int centerY = boxY + (boxHeight / 2);

    it.get_text_bounds(0, 0, text, textFont, esphome::display::TextAlign::TOP_LEFT,
                      &bx, &by, &bw, &bh);
    int textW = bw;
    int textH = bh;

    if (iconFont != nullptr) {
        it.get_text_bounds(0, 0, icon, iconFont, esphome::display::TextAlign::TOP_LEFT,
                          &bx, &by, &bw, &bh);
        int iconW = bw;
        int iconH = bh;

        int totalContentW = iconW + iconTextSpacing + textW;
        int contentStartX = boxX + (boxWidth - totalContentW) / 2;

        int iconX = contentStartX;
        int iconY = centerY - (iconH / 2);
        it.print(iconX, iconY, iconFont, iconColor, esphome::display::TextAlign::TOP_LEFT, icon);

        int textX = iconX + iconW + iconTextSpacing;
        int textY = centerY - (textH / 2);
        it.print(textX, textY, textFont, textColor, esphome::display::TextAlign::TOP_LEFT, text);
    } else {
        int boxCenterX = boxX + (boxWidth / 2);
        int textY = centerY - (textH / 2);
        it.print(boxCenterX, textY, textFont, textColor, esphome::display::TextAlign::TOP_CENTER, text);
    }
}

}  // namespace

// ---------------- cMet implementation ----------------

void cMet::init(
    esphome::display::Display &it,
    const MetFonts &f,
    bool mono,
    esphome::time::RealTimeClock *pClock,
    int realWidth,
    int realHeight,
    int blPin
) {
    this->pDisplay = &it;
    this->fonts = f;
    this->bMono = mono;
    this->pTime = pClock;
    this->iScreenWidth = realWidth > 0 ? realWidth : it.get_width();
    this->iScreenHeight = realHeight > 0 ? realHeight : it.get_height();
    if (blPin >= 0) this->iBlPin = blPin;
    // Page content starts below the title line (computed here so that it does
    // not depend on drawPageTitle() having run first in the same frame)
    this->iPageTop = 1 + (f.title != nullptr ? f.title->get_height() : 0);
}

/**
 * @brief Directly controls the backlight pin using low-level ESP-IDF functions.
 * This bypasses the ESPHome Light/Output component layer, making it reliable for
 * use during minimal environments like the OTA on_begin phase.
 * The global ESPHome variables (bl_on, last_interaction) must be updated in the YAML lambda.
 */
void cMet::backlightOnForOta(bool on) const {
    if (iBlPin < 0 || iBlPin >= (int) GPIO_NUM_MAX) return;  // invalid pin
    const gpio_num_t BL_PIN = (gpio_num_t) iBlPin;
    gpio_set_direction(BL_PIN, GPIO_MODE_OUTPUT);
    gpio_set_level(BL_PIN, on ? 1 : 0);
}

void cMet::drawDsegText(
        int x1,
        int x2,
        int y1,
        int y2,
        esphome::font::Font *pFont,
        const std::string& text,
        esphome::Color color,
        TextAlignment alignment
) {
    if (!ready() || pFont == nullptr) return;
    esphome::display::Display &display = *pDisplay;

    // Fall back to the logical width stored by init() when bounds are not given
    if (x2 <= x1) {
        x1 = 0;
        x2 = iScreenWidth > 0 ? iScreenWidth - 1 : display.get_width() - 1;
    }

    int width = x2 - x1;
    int height = y2 - y1;
    int center_y = y1 + (height / 2);

    // Create placeholder string (same length as text)
    std::string placeholder;
    for (size_t i = 0; i < text.length(); i++) {
        if (text[i] == ':' || text[i] == '.' || text[i] == ' ') {
            placeholder += text[i];  // Keep delimiters as-is
        } else {
            placeholder += DSEG_PLACEHOLDER;  // Replace digits with placeholder
        }
    }

    int text_x;
    display::TextAlign text_align;

    switch (alignment) {
        case ALIGN_LEFT:
            text_x = x1;
            text_align = display::TextAlign::CENTER_LEFT;
            break;
        case ALIGN_RIGHT:
            text_x = x2;
            text_align = display::TextAlign::CENTER_RIGHT;
            break;
        case ALIGN_CENTER:
        default:
            text_x = x1 + (width / 2);
            text_align = display::TextAlign::CENTER;
            break;
    }

    // Dark placeholder background (all segments), then the actual text on top
    Color placeholder_color = Color(40, 40, 40);
    display.print(text_x, center_y, pFont, placeholder_color, text_align, placeholder.c_str());
    display.print(text_x, center_y, pFont, color, text_align, text.c_str());
}

void cMet::drawPageTitle(const std::string &sText, bool bSelected, bool bTime) {
    if (!ready()) return;
    esphome::display::Display &it = *pDisplay;
    it.fill(Color(0x0));
    if (fonts.title == nullptr) return;

    esphome::Color col = bSelected ? esphome::Color(0x00FF00) : esphome::Color::WHITE;

    // Title text and underline (1 pixel height)
    it.print(3, 1, fonts.title, col, sText.c_str());
    it.line(0, iPageTop - 1, iScreenWidth - 1, iPageTop - 1, col);

    if (bTime && pTime != nullptr) {
        auto now = pTime->now();
        if (now.is_valid()) {
            // Plain font here (not DSEG): no placeholder background underneath
            std::string time_str = now.strftime("%H:%M");
            it.print(iScreenWidth - 2, 1, fonts.title, Color(0xff7f00),
                     esphome::display::TextAlign::TOP_RIGHT, time_str.c_str());
        }
    }
}

// ===== BOTTOM BOXES =====

/**
 * Draw weather information boxes at the bottom of the display
 * Each box shows an icon on the left and value on the right, with individual colors
 *
 * @param it Display object
 * @param fTemperature Temperature in Celsius (always shown, "--.-" when NaN)
 * @param fHumidity Humidity percentage (hidden when NaN)
 * @param fPression Pressure in hPa/mbar (hidden when NaN)
 */
void cMet::drawBottomBoxes(
    float fTemperature,
    float fHumidity,
    float fPression
) {
    if (!ready() || fonts.value == nullptr) return;
    esphome::display::Display &it = *pDisplay;

    const int BOX_HEIGHT = 22;
    const int Y_TOP = iScreenHeight - BOX_HEIGHT;  // boxes at bottom of screen
    const int ICON_TEXT_SPACING = 6;
    const int BOX_PADDING_H = 4;
    const esphome::Color colorBorder = esphome::Color(128, 128, 128);

    struct Box {
        const char *icon;
        char text[20];
        esphome::Color iconColor;
        esphome::Color textColor;
        int natural;
        int width;
    };
    Box boxes[3];
    int n = 0;

    // Temperature (always)
    boxes[n].icon = "\uf055";  // thermometer
    if (std::isnan(fTemperature)) snprintf(boxes[n].text, sizeof(boxes[n].text), "--.-°C");
    else snprintf(boxes[n].text, sizeof(boxes[n].text), "%.1f°C", fTemperature);
    boxes[n].iconColor = esphome::Color(255, 100, 0);
    boxes[n].textColor = esphome::Color(255, 150, 50);
    n++;

    // Humidity
    if (!std::isnan(fHumidity)) {
        boxes[n].icon = "\uf07a";  // water-percent
        snprintf(boxes[n].text, sizeof(boxes[n].text), "%.0f%%", fHumidity);
        boxes[n].iconColor = esphome::Color(0, 150, 255);
        boxes[n].textColor = esphome::Color(100, 200, 255);
        n++;
    }

    // Pressure
    if (!std::isnan(fPression)) {
        boxes[n].icon = "\uf079";  // gauge
        snprintf(boxes[n].text, sizeof(boxes[n].text), "%.1fhPa", fPression);
        boxes[n].iconColor = esphome::Color(0, 255, 100);
        boxes[n].textColor = esphome::Color(100, 255, 150);
        n++;
    }

    // Icons only when there is room for them
    const bool bShowIcons = (fonts.icons != nullptr) && ((n <= 2) || (iScreenWidth > 128));
    esphome::font::Font *iconFont = bShowIcons ? fonts.icons : nullptr;

    // Natural widths, then scale proportionally to fill the screen
    int totalNatural = 0;
    for (int i = 0; i < n; i++) {
        boxes[i].natural = calcIconTextBoxWidth(it, boxes[i].icon, boxes[i].text,
                                                iconFont, fonts.value,
                                                ICON_TEXT_SPACING, BOX_PADDING_H);
        totalNatural += boxes[i].natural;
    }
    if (totalNatural <= 0) return;
    float scale = (float) iScreenWidth / (float) totalNatural;

    int x = 0;
    for (int i = 0; i < n; i++) {
        // last box takes the rounding remainder so the row fills the screen exactly
        boxes[i].width = (i == n - 1) ? (iScreenWidth - x) : (int) (boxes[i].natural * scale);

        it.rectangle(x, Y_TOP, boxes[i].width, BOX_HEIGHT, colorBorder);
        drawIconTextContent(it, x, Y_TOP, boxes[i].width, BOX_HEIGHT,
                            boxes[i].icon, boxes[i].text,
                            iconFont, fonts.value,
                            boxes[i].iconColor, boxes[i].textColor,
                            ICON_TEXT_SPACING);
        x += boxes[i].width;
    }
}

void cMet::drawBottomBoxes(
    esphome::sensor::Sensor *pTemperature,
    esphome::sensor::Sensor *pHumidity,
    esphome::sensor::Sensor *pPression
) {
    // Sensor::state is NaN until the first value arrives
    auto val = [](esphome::sensor::Sensor *s) -> float { return s != nullptr ? s->state : NAN; };
    drawBottomBoxes(val(pTemperature), val(pHumidity), val(pPression));
}

// ===== ABOUT PAGE =====

/**
 * Draw the About page with system, network, and memory information
 * Automatically adapts layout based on screen orientation (portrait/landscape/square)
 * Optimizes line spacing based on actual font height
 */
void cMet::menuDrawAboutPage() {
    if (!ready()) return;
    esphome::display::Display &it = *pDisplay;

    esphome::font::Font *pFont = fonts.value;
    if (pFont == nullptr) return;
    const bool bMonoChrome = bMono;

    // ----- Gather all information first -----
    std::string deviceName = App.get_name();
    std::string buildTime = "";
    std::string wifiAddr = "";
    int8_t rssi = 0;
    uint32_t freeHeap = 0;
    uint32_t totalHeap = 0;
    uint32_t fragmentation = 0;
    std::vector<uint8_t> i2cAddresses;

    // __DATE__/__TIME__ is the compile time of this file only; replace with
    // App.get_compilation_time() if your ESPHome version provides it.
    buildTime = __DATE__ " " __TIME__;
    if (buildTime.length() > 11) buildTime = buildTime.substr(0, 11);

    #ifdef USE_WIFI
    if (wifi::global_wifi_component != nullptr) {
        wifiAddr = wifi::global_wifi_component->get_use_address();
        rssi = wifi::global_wifi_component->wifi_rssi();
    }
    #endif

    #ifdef USE_ESP32
    freeHeap = esp_get_free_heap_size();
    totalHeap = heap_caps_get_total_size(MALLOC_CAP_DEFAULT);
    uint32_t maxAlloc = heap_caps_get_largest_free_block(MALLOC_CAP_DEFAULT);
    fragmentation = freeHeap > 0 ? (100 - (maxAlloc * 100 / freeHeap)) : 0;
    #elif defined(USE_ESP8266)
    freeHeap = ESP.getFreeHeap();
    totalHeap = 0;  // Not available on ESP8266
    #endif

    uint32_t uptime = millis() / 1000;
    uint32_t days = uptime / 86400;
    uint32_t hours = (uptime % 86400) / 3600;
    uint32_t minutes = (uptime % 3600) / 60;

    // Format strings
    char rssiStr[16], freeStr[16], totalStr[16], fragStr[16], uptimeStr[32];
    snprintf(rssiStr, sizeof(rssiStr), "%d dBm", (int) rssi);
    snprintf(freeStr, sizeof(freeStr), "%.1f KB", freeHeap / 1024.0f);
    snprintf(totalStr, sizeof(totalStr), "%.1f KB", totalHeap / 1024.0f);
    snprintf(fragStr, sizeof(fragStr), "%u%%", (unsigned) fragmentation);

    if (days > 0) {
        snprintf(uptimeStr, sizeof(uptimeStr), "%ud %uh", (unsigned) days, (unsigned) hours);
    } else if (hours > 0) {
        snprintf(uptimeStr, sizeof(uptimeStr), "%uh %um", (unsigned) hours, (unsigned) minutes);
    } else {
        snprintf(uptimeStr, sizeof(uptimeStr), "%um", (unsigned) minutes);
    }

    // Format I2C devices
    std::string i2cStr = "";
    if (!i2cAddresses.empty()) {
        char hexBuf[8];
        for (size_t i = 0; i < i2cAddresses.size() && i < 8; i++) {
            if (i > 0) i2cStr += " ";
            snprintf(hexBuf, sizeof(hexBuf), "0x%02X", i2cAddresses[i]);
            i2cStr += hexBuf;
        }
        if (i2cAddresses.size() > 8) i2cStr += "...";
    }

    // ----- Screen setup -----
    const int screenW = iScreenWidth;
    const int screenH = iScreenHeight;
    float aspectRatio = (float)screenW / (float)screenH;

    int fontHeight = pFont->get_height();
    int lineSpacing = fontHeight + 1;  // Minimum spacing: font height + 1 pixel
    int compactSpacing = fontHeight;   // For portrait mode

    // Colors
    esphome::Color labelColor = bMonoChrome ? esphome::Color::WHITE : esphome::Color(180, 180, 180);
    esphome::Color valueColor = esphome::Color::WHITE;
    esphome::Color accentColor = bMonoChrome ? esphome::Color::WHITE : esphome::Color(0, 200, 255);
    esphome::Color footerColor = bMonoChrome ? esphome::Color::WHITE : esphome::Color(100, 100, 100);
    esphome::Color i2cColor = bMonoChrome ? esphome::Color::WHITE : esphome::Color(255, 200, 0);

    int startY = iPageTop + 3;  // just below the title

    // ----- Display based on aspect ratio -----
    if (aspectRatio > 1.15f) {
        // LANDSCAPE: Two columns
        const int COL1_X = 4, COL2_X = screenW / 2 + 2, VAL_OFF = 50;
        int y1 = startY, y2 = startY;

        // Column 1: Device & Network
        if (!deviceName.empty()) { it.print(COL1_X, y1, pFont, labelColor, "Device:"); it.print(COL1_X + VAL_OFF, y1, pFont, accentColor, deviceName.c_str()); y1 += lineSpacing; }
        it.print(COL1_X, y1, pFont, labelColor, "Build:"); it.print(COL1_X + VAL_OFF, y1, pFont, valueColor, buildTime.c_str()); y1 += lineSpacing;
        if (!wifiAddr.empty()) { it.print(COL1_X, y1, pFont, labelColor, "WiFi:"); it.print(COL1_X + VAL_OFF, y1, pFont, valueColor, wifiAddr.c_str()); y1 += lineSpacing; }
        if (!wifiAddr.empty()) { it.print(COL1_X, y1, pFont, labelColor, "Signal:"); it.print(COL1_X + VAL_OFF, y1, pFont, valueColor, rssiStr); y1 += lineSpacing; }

        // I2C devices in column 1 if space available
        if (!i2cStr.empty() && y1 < screenH - fontHeight - 10) {
            it.print(COL1_X, y1, pFont, labelColor, "I2C:");
            it.print(COL1_X + VAL_OFF, y1, pFont, i2cColor, i2cStr.c_str());
            y1 += lineSpacing;
        }

        // Column 2: Memory & Uptime
        it.print(COL2_X, y2, pFont, labelColor, "Free:"); it.print(COL2_X + VAL_OFF, y2, pFont, valueColor, freeStr); y2 += lineSpacing;
        if (totalHeap > 0) { it.print(COL2_X, y2, pFont, labelColor, "Total:"); it.print(COL2_X + VAL_OFF, y2, pFont, valueColor, totalStr); y2 += lineSpacing; }
        if (totalHeap > 0) { it.print(COL2_X, y2, pFont, labelColor, "Frag:"); it.print(COL2_X + VAL_OFF, y2, pFont, valueColor, fragStr); y2 += lineSpacing; }
        it.print(COL2_X, y2, pFont, labelColor, "Uptime:"); it.print(COL2_X + VAL_OFF, y2, pFont, valueColor, uptimeStr); y2 += lineSpacing;

        // I2C count in column 2 if many devices
        if (i2cAddresses.size() > 0 && y2 < screenH - fontHeight - 10) {
            char countStr[16];
            snprintf(countStr, sizeof(countStr), "%u found", (unsigned) i2cAddresses.size());
            it.print(COL2_X, y2, pFont, labelColor, "I2C:");
            it.print(COL2_X + VAL_OFF, y2, pFont, i2cColor, countStr);
        }

    } else if (aspectRatio < 0.85f) {
        // PORTRAIT: Compact inline
        const int M = 4;
        int y = startY;

        if (!deviceName.empty()) { it.printf(M, y, pFont, accentColor, "%s", deviceName.c_str()); y += lineSpacing + 2; }
        it.printf(M, y, pFont, labelColor, "Build: %s", buildTime.c_str()); y += compactSpacing;
        if (!wifiAddr.empty()) {
            std::string ip = wifiAddr;
            if (ip.length() > 15) { size_t d = ip.find_last_of('.'); if (d != std::string::npos && d > 2) ip = "..." + ip.substr(d - 3); }
            it.printf(M, y, pFont, labelColor, "IP: %s", ip.c_str()); y += compactSpacing;
            it.printf(M, y, pFont, labelColor, "Signal: %s", rssiStr); y += compactSpacing;
        }

        // Separator
        y += 1;
        it.line(M, y, screenW - M, y, esphome::Color(80, 80, 80));
        y += 2;

        it.printf(M, y, pFont, labelColor, "Free: %s", freeStr); y += compactSpacing;
        if (totalHeap > 0) { it.printf(M, y, pFont, labelColor, "Total: %s", totalStr); y += compactSpacing; }
        it.printf(M, y, pFont, labelColor, "Up: %s", uptimeStr); y += compactSpacing;

        // I2C info if space available
        if (!i2cStr.empty() && y < screenH - fontHeight - 10) {
            y += 1;
            it.line(M, y, screenW - M, y, esphome::Color(80, 80, 80));
            y += 2;
            it.printf(M, y, pFont, i2cColor, "I2C: %s", i2cStr.c_str());
        }

    } else {
        // SQUARE: Label-value format
        const int LX = 4, VX = 56;
        int y = startY;

        if (!deviceName.empty()) { it.print(LX, y, pFont, labelColor, "Device:"); it.print(VX, y, pFont, accentColor, deviceName.c_str()); y += lineSpacing; }
        it.print(LX, y, pFont, labelColor, "Build:"); it.print(VX, y, pFont, valueColor, buildTime.c_str()); y += lineSpacing;
        if (!wifiAddr.empty()) { it.print(LX, y, pFont, labelColor, "WiFi:"); it.print(VX, y, pFont, valueColor, wifiAddr.c_str()); y += lineSpacing; }
        if (!wifiAddr.empty()) { it.print(LX, y, pFont, labelColor, "Signal:"); it.print(VX, y, pFont, valueColor, rssiStr); y += lineSpacing; }
        it.print(LX, y, pFont, labelColor, "Free:"); it.print(VX, y, pFont, valueColor, freeStr); y += lineSpacing;
        if (totalHeap > 0) { it.print(LX, y, pFont, labelColor, "Total:"); it.print(VX, y, pFont, valueColor, totalStr); y += lineSpacing; }
        it.print(LX, y, pFont, labelColor, "Uptime:"); it.print(VX, y, pFont, valueColor, uptimeStr); y += lineSpacing;

        // I2C devices if space available
        if (!i2cStr.empty() && y < screenH - fontHeight - 10) {
            it.print(LX, y, pFont, labelColor, "I2C:");
            it.print(VX, y, pFont, i2cColor, i2cStr.c_str());
        }
    }

    // Footer
    it.print(screenW / 2, screenH - 8, pFont, footerColor, esphome::display::TextAlign::BOTTOM_CENTER, "ESPHome");
}

// ===== HOME PAGE =====

void cMet::drawTimeAndDate() {
    if (!ready() || fonts.dsegTime == nullptr || fonts.dsegDate == nullptr) return;

    // Y positions are proportional to screen height
    // For 128px height: time at 22-51, date at 50-65
    int time_y1 = (22 * iScreenHeight) / 128;
    int time_y2 = (51 * iScreenHeight) / 128;
    int date_y1 = (50 * iScreenHeight) / 128;
    int date_y2 = (65 * iScreenHeight) / 128;

    std::string time_str = "~~:~~:~~";
    std::string date_str = "~~/~~/~~~~";
    if (pTime != nullptr) {
        auto now = pTime->now();
        if (now.is_valid()) {
            time_str = now.strftime("%H:%M:%S");
            date_str = now.strftime("%d/%m/%Y");
        }
    }
    drawDsegText(0, iScreenWidth - 1, time_y1, time_y2, fonts.dsegTime, time_str, Color(0xff7f00));
    drawDsegText(0, iScreenWidth - 1, date_y1, date_y2, fonts.dsegDate, date_str, Color(0xff7f00));
}

/**
 * Draw weather condition and moon phase icons/text
 * Adapts layout based on day/night and presence of moon phase sensor
 *
 * @param pWeatherCondition Text sensor for weather condition
 * @param pMoonPhaseIcon Text sensor for moon phase icon (optional)
 *
 * Home Assistant weather states: clear-night, cloudy, exceptional, fog, hail, lightning,
 * lightning-rainy, partlycloudy, pouring, rainy, snowy, snowy-rainy, sunny, windy, windy-variant
 */
void cMet::drawWeatherAndMoon(
    esphome::text_sensor::TextSensor *pWeatherCondition,
    esphome::text_sensor::TextSensor *pMoonPhaseIcon
) {
    if (!ready() || fonts.weather == nullptr || fonts.micro == nullptr) return;
    esphome::display::Display &it = *pDisplay;

    esphome::font::Font *pFontWeather = fonts.weather;
    esphome::font::Font *pFontMicro = fonts.micro;
    const int screenW = iScreenWidth;
    const int screenH = iScreenHeight;

    // Debug: Log sensor states
    if (pWeatherCondition == nullptr) {
        ESP_LOGW("weather", "pWeatherCondition is nullptr");
    } else if (!pWeatherCondition->has_state()) {
        ESP_LOGW("weather", "pWeatherCondition has no state yet");
    } else if (pWeatherCondition->state == "") {
        ESP_LOGW("weather", "pWeatherCondition state is empty");
    }

    // If no weather data, show a placeholder instead of returning silently
    if (pWeatherCondition == nullptr || !pWeatherCondition->has_state() || pWeatherCondition->state == "") {
        int icon_y = (75 * screenH) / 128;
        int text_y = (100 * screenH) / 128;
        int cx = screenW / 2;
        it.print(cx, icon_y, pFontWeather, Color(0x888888), esphome::display::TextAlign::CENTER, "\uf141");
        it.print(cx, text_y, pFontMicro, Color(0x888888), esphome::display::TextAlign::CENTER, "No weather data");
        return;
    }
    std::string cond = pWeatherCondition->state;

    // Decide night: prefer explicit 'night' in condition, else fallback to local hour
    bool is_night = false;
    if (cond.find("night") != std::string::npos) {
        is_night = true;
    } else if (pTime != nullptr) {
        auto now = pTime->now();
        if (now.is_valid()) {
            int hour = now.hour;
            if (hour < 6 || hour >= 20) is_night = true;
        }
    }

    const char *icon = "\ue374"; // default
    esphome::Color icon_color = Color::WHITE;

    // Use a normalized form for robust matching
    std::string norm = normalizeCondition(cond);

    // Helper to check whether a glyph is available in the weather font
    auto glyph_exists = [&](const char *g)->bool {
        if (pFontWeather == nullptr) return false;
        int bx, by, bw, bh;
        it.get_text_bounds(0, 0, g, pFontWeather, esphome::display::TextAlign::TOP_LEFT, &bx, &by, &bw, &bh);
        return bw > 0;
    };

    const char *day_icon = "\ue374";
    const char *night_icon = nullptr;

    if (norm == "clear") {
        day_icon = "\uf00d";
        night_icon = "\uf02e";
        icon_color = is_night ? Color(0xeeeeee) : Color(0xFFFF00);
    } else if (norm == "night") {
        day_icon = "\uf02e";
        night_icon = nullptr;
        icon_color = Color(0xeeeeee);
    } else if (norm == "partlycloudy") {
        day_icon = "\ue302";
        night_icon = "\ue32e";
        icon_color = Color::WHITE;
    } else if (norm == "cloudy") {
        day_icon = "\ue312"; // weather-cloudy
        night_icon = nullptr;
        icon_color = Color::WHITE;
    } else if (norm == "rain") {
        // includes showers, drizzle, pouring, etc. (see normalizeCondition)
        day_icon = "\ue308"; // generic rain icon
        night_icon = "\ue308"; // prefer same icon unless font has a night variant
        icon_color = Color(0x0000FF);
    } else if (norm == "snow") {
        day_icon = "\uf083";
        night_icon = "\uf083";
        icon_color = Color::WHITE;
    } else if (norm == "fog") {
        day_icon = "\ue313"; // weather-fog
        night_icon = nullptr;
        icon_color = Color(0xAAAAAA);
    } else if (norm == "windy") {
        day_icon = "\uf050";
        night_icon = "\uf050";
        icon_color = Color::WHITE;
    } else if (norm == "unavailable") {
        day_icon = "\uf141"; // fa-ellipsis_h
        night_icon = nullptr;
        icon_color = Color(0xFF0000);
    } else if (norm == "hail") {
        day_icon = "\uf015";
        night_icon = "\uf015";
        icon_color = Color::WHITE;
    } else {
        ESP_LOGD("display", "Weather condition: '%s'", cond.c_str());
        day_icon = "\uf07b";
        night_icon = nullptr;
        icon_color = Color(0x888800);
    }

    // Select icon with safe fallback when night-only glyph is missing
    if (is_night && night_icon != nullptr && glyph_exists(night_icon)) {
        icon = night_icon;
    } else if (glyph_exists(day_icon)) {
        icon = day_icon;
    } else if (night_icon != nullptr) {
        // Last resort: try the night glyph even if glyph_exists previously failed
        icon = night_icon;
    } else {
        icon = day_icon;
    }

    // Y positions proportional to screen height (designed for 128px)
    int icon_y = (75 * screenH) / 128;
    int text_y = (100 * screenH) / 128;
    int moon_y = (80 * screenH) / 128;

    const bool has_moon = pMoonPhaseIcon != nullptr && pMoonPhaseIcon->has_state() && pMoonPhaseIcon->state != "";
    const char *moon_icon = has_moon ? pMoonPhaseIcon->state.c_str() : "\uf141";  // N/A ellipsis glyph as fallback

    if (!is_night) {
        // Day: single symbol centered
        int cx = screenW / 2;
        it.print(cx, icon_y, pFontWeather, icon_color, esphome::display::TextAlign::CENTER, icon);
        it.print(cx, text_y, pFontMicro, Color(0x00ff00), esphome::display::TextAlign::CENTER, cond.c_str());
    } else {
        // Night: weather (left) and moon (right). For clear nights we avoid
        // drawing a weather icon on the left because the moon is shown anyway
        // (avoid duplicate moon glyphs); the moon is then centered.
        int first_cx = screenW / 4;
        int second_cx = (screenW * 3) / 4;
        int center_cx = screenW / 2;

        bool draw_first = !(norm == "clear" || norm == "night");

        // Always draw the centered condition text so the condition is visible
        it.print(center_cx, text_y, pFontMicro, Color(0x00ff00), esphome::display::TextAlign::CENTER, cond.c_str());

        if (draw_first) {
            it.print(first_cx, moon_y, pFontWeather, icon_color, esphome::display::TextAlign::CENTER, icon);
            it.print(second_cx, moon_y, pFontWeather, Color(0xFFFF00), esphome::display::TextAlign::CENTER, moon_icon);
        } else {
            it.print(center_cx, moon_y, pFontWeather, Color(0xFFFF00), esphome::display::TextAlign::CENTER, moon_icon);
        }
    }
}

std::string cMet::normalizeCondition(const std::string &cond) {
    std::string s = cond;
    for (auto &c: s) c = tolower(c);
    auto has = [&](const char *p) { return s.find(p) != std::string::npos; };

    // Order matters: more specific patterns first
    if (s == "sunny" || (has("clear") && !has("night"))) return "clear";
    if (has("partly")) return "partlycloudy";
    if (has("cloud")) return "cloudy";
    if (has("snow")) return "snow";   // also snowy-rainy
    if (has("shower") || has("rain") || has("drizzle") || has("pouring")) return "rain";
    if (has("fog") || has("mist")) return "fog";
    if (has("wind")) return "windy";
    if (has("hail")) return "hail";
    if (has("night")) return "night";
    return s;
}

// ===== PROGRESS PAGE =====

void cMet::drawProgressPage(
        const std::string& sText,
        float fValue
 ) {
    if (!ready()) return;
    esphome::display::Display &it = *pDisplay;

    drawPageTitle(sText, true);
    if (fonts.title == nullptr) return;

    if (std::isnan(fValue)) fValue = 0.0f;
    if (fValue < 0.0f) fValue = 0.0f;
    if (fValue > 100.0f) fValue = 100.0f;

    // Progress bar (positions designed for 128px height)
    int bar_x = 20;
    int bar_y = (70 * iScreenHeight) / 128;
    int bar_width = iScreenWidth - 40;
    int bar_height = 20;
    it.rectangle(bar_x, bar_y, bar_width, bar_height, Color(0xFFFFFF));

    int fill_width = (int) ((bar_width - 4) * (fValue / 100.0f));
    if (fill_width > 0) {
        it.filled_rectangle(bar_x + 2, bar_y + 2, fill_width, bar_height - 4, Color(0x00FF00));
    }

    // Percentage text
    it.printf(iScreenWidth / 2, (100 * iScreenHeight) / 128, fonts.title, Color(0xFFFFFF),
              esphome::display::TextAlign::CENTER, "%.0f%%", fValue);
}

// ===== OPTIONS PAGE =====

void cMet::drawOptionsPage( int selected_option,
                            esphome::sensor::Sensor *pSensor1,
                            esphome::sensor::Sensor *pSensor2,
                            esphome::sensor::Sensor *pSensor3) const {

    if (!ready()) return;
    esphome::display::Display &it = *pDisplay;

    esphome::font::Font *pFont = fonts.title;
    if (pFont == nullptr) return;

    const int LINE_X1 = 50;   // vertical divider (two lines); state is right-aligned left of it
    const int LINE_X2 = 54;   // labels start right of it
    const int TEXT_GAP = 5;

    int y = iPageTop;
    int font_height = pFont->get_height();
    int line_height = font_height + 2;  // +2 for spacing between lines

    // Divider lines from the page top to 1.5 font heights above the bottom
    int line_end_y = iScreenHeight - (int)(1.5f * font_height);
    it.vertical_line(LINE_X1, iPageTop + 1, line_end_y - y, esphome::Color::WHITE);
    it.vertical_line(LINE_X2, iPageTop + 1, line_end_y - y, esphome::Color::WHITE);

    // Build list of valid sensors
    std::vector<sensor::Sensor*> sensors;
    if (pSensor1 != nullptr) sensors.push_back(pSensor1);
    if (pSensor2 != nullptr) sensors.push_back(pSensor2);
    if (pSensor3 != nullptr) sensors.push_back(pSensor3);

    int option_count = sensors.size();

    for (int i = 0; i < option_count; ++i) {
        int yy = y + i * line_height;
        sensor::Sensor *s = sensors[i];

        std::string label = s->get_name();

        // Plain ASCII only: the title font has no glyphs for symbols such as ✓ ✕ ?
        float state = s->state;
        const char *state_str;
        esphome::Color state_col = esphome::Color::WHITE;
        if (state == 0.0f) {
            state_str = "OFF";
            if (!bMono) state_col = esphome::Color(0xff0000);  // Red
        } else if (state == 1.0f) {
            state_str = "ON";
            if (!bMono) state_col = esphome::Color(0x00ff00);  // Green
        } else {
            state_str = "--";
        }

        // Selection marker on the divider
        if (i == selected_option) {
            esphome::Color sel_col = bMono ? esphome::Color::WHITE : esphome::Color(0x0000ff);  // Blue
            it.filled_rectangle(LINE_X1, yy, LINE_X2 - LINE_X1, font_height, sel_col);
        }

        it.printf(LINE_X1 - TEXT_GAP, yy, pFont, state_col, esphome::display::TextAlign::TOP_RIGHT, "%s", state_str);
        it.printf(LINE_X2 + TEXT_GAP, yy, pFont, esphome::Color::WHITE, esphome::display::TextAlign::TOP_LEFT, "%s", label.c_str());
    }
}

// Define global instance
cMet met;

}