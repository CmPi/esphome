/**
 *  @file sysinfo.cpp
 *  @brief package handling display and menu functions for MET display
 *  @author CmPi <github.com/CmPi>
 */

#include "esphome.h"
#include "met.h"
#include "driver/gpio.h" // Include header here too, for the implementation

namespace esphome {

/**
 * @brief Directly controls the backlight pin (GPIO4) using low-level ESP-IDF functions.
 * * This bypasses the ESPHome Light/Output component layer, making it reliable for 
 * use during minimal environments like the OTA on_begin phase.
 * @param on True to turn on (HIGH), False to turn off (LOW).
 */
void backlight_on_for_ota(bool on) {
    const gpio_num_t BL_PIN = (gpio_num_t) 4;
    
    // 1. Ensure the pin is configured as an output
    gpio_set_direction(BL_PIN, GPIO_MODE_OUTPUT);
    
    // 2. Set the pin level
    // 1 (HIGH) for ON, 0 (LOW) for OFF
    gpio_set_level(BL_PIN, on ? 1 : 0);
    
    // Note: The global ESPHome variables (bl_on, last_interaction)
    // should be updated in the YAML lambda, not here, as this function
    // runs outside the main ESPHome framework context.
}



/**
 * Draw text with DSEG font with LED-style background placeholders
 * 
 * @param display Display object (it)
 * @param x1 Left boundary
 * @param x2 Right boundary
 * @param y1 Top boundary
 * @param y2 Bottom boundary
 * @param font DSEG font to use
 * @param text Text to display
 * @param color Text color (default: white)
 * @param alignment Text alignment (default: center)
 */
void drawDsegText(
        esphome::display::Display& display,
        int x1,
        int x2,
        int y1,
        int y2,
        esphome::font::Font *pFont,
        const std::string& text,
        esphome::Color color,
        TextAlignment alignment
) {
    // Calculate dimensions
    int width = x2 - x1;
    int height = y2 - y1;
    int center_y = y1 + (height / 2);
    
    // Measure text width
    int text_width, text_x_offset, text_baseline, text_height;
    display.get_text_bounds(0, 0, text.c_str(), pFont, 
                           display::TextAlign::TOP_LEFT,
                           &text_x_offset, &text_baseline, 
                           &text_width, &text_height);
    
    // Create placeholder string (same length as text)
    std::string placeholder;
    for (size_t i = 0; i < text.length(); i++) {
        if (text[i] == ':' || text[i] == '.' || text[i] == ' ') {
            placeholder += text[i];  // Keep delimiters as-is
        } else {
            placeholder += DSEG_PLACEHOLDER;  // Replace digits with placeholder
        }
    }
    
    // Calculate x position based on alignment
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
    
    // Draw dark placeholder background (all segments)
    Color placeholder_color = Color(40, 40, 40);  // Dark gray for unlit segments
    display.print(text_x, center_y, pFont, placeholder_color, text_align, placeholder.c_str());
    
    // Draw actual text on top in chosen color
    display.print(text_x, center_y, pFont, color, text_align, text.c_str());
}

/**
 * Convenience wrapper for time display (HH:MM:SS format)
 */
void draw_dseg_time(
    display::Display& display,
    int x1,
    int x2,
    int y1,
    int y2,
    esphome::font::Font *pFont,
    int hours,
    int minutes,
    int seconds,
    Color color = Color(255, 255, 255),
    TextAlignment alignment = ALIGN_CENTER
) {
    char time_str[9];
    snprintf(time_str, sizeof(time_str), "%02d:%02d:%02d", hours, minutes, seconds);
    drawDsegText(display, x1, x2, y1, y2, pFont, time_str, color, alignment);
}

/**
 * Convenience wrapper for HH:MM format (without seconds)
 */
void draw_dseg_time_hhmm(
    display::Display& display,
    int x1,
    int x2,
    int y1,
    int y2,
    esphome::font::Font *pFont,
    int hours,
    int minutes,
    Color color = Color(255, 255, 255),
    TextAlignment alignment = ALIGN_CENTER
) {
    char time_str[6];
    snprintf(time_str, sizeof(time_str), "%02d:%02d", hours, minutes);
    drawDsegText(display, x1, x2, y1, y2, pFont, time_str, color, alignment);
}



// ===== REUSABLE HELPER FUNCTIONS =====

/**
 * Calculate the width needed for an icon + text box
 * If iconFont is nullptr, only calculates text width (no icon)
 */
inline int calcIconTextBoxWidth(
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
    
    // Only calculate icon width if font is provided
    if (iconFont != nullptr) {
        it.get_text_bounds(0, 0, icon, iconFont, esphome::display::TextAlign::TOP_LEFT, 
                          &bx, &by, &bw, &bh);
        iconW = bw;
    }
    
    it.get_text_bounds(0, 0, text, textFont, esphome::display::TextAlign::TOP_LEFT, 
                      &bx, &by, &bw, &bh);
    int textW = bw;
    
    // Only add spacing if there's an icon
    int spacing = (iconFont != nullptr) ? iconTextSpacing : 0;
    
    return iconW + spacing + textW + (horizontalPadding * 2);
}

/**
 * Draw icon (left) and text (right) within a box area with vertical centering
 * Content is always centered horizontally within the box
 */
inline void drawIconTextContent(
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
    int iconTextSpacing,
    int horizontalPadding
) {
    int bx, by, bw, bh;
    
    // Calculate vertical center of the box
    int centerY = boxY + (boxHeight / 2);
    
    // Get text dimensions
    it.get_text_bounds(0, 0, text, textFont, esphome::display::TextAlign::TOP_LEFT, 
                      &bx, &by, &bw, &bh);
    int textW = bw;
    int textH = bh;
    
    if (iconFont != nullptr) {
        // Draw with icon on left of text, but center the whole group in the box
        // Get icon dimensions
        it.get_text_bounds(0, 0, icon, iconFont, esphome::display::TextAlign::TOP_LEFT, 
                          &bx, &by, &bw, &bh);
        int iconW = bw;
        int iconH = bh;
        
        // Calculate total content width (icon + spacing + text)
        int totalContentW = iconW + iconTextSpacing + textW;
        
        // Center the content group horizontally in the box
        int contentStartX = boxX + (boxWidth - totalContentW) / 2;
        
        // Draw icon vertically centered
        int iconX = contentStartX;
        int iconY = centerY - (iconH / 2);
        it.print(
            iconX, iconY,
            iconFont, iconColor,
            esphome::display::TextAlign::TOP_LEFT,
            icon
        );
        
        // Draw text to the right of icon, vertically centered
        int textX = iconX + iconW + iconTextSpacing;
        int textY = centerY - (textH / 2);
        it.print(
            textX, textY,
            textFont, textColor,
            esphome::display::TextAlign::TOP_LEFT,
            text
        );
    } else {
        // Draw text only, centered both horizontally and vertically
        int boxCenterX = boxX + (boxWidth / 2);
        int textY = centerY - (textH / 2);
        it.print(
            boxCenterX, textY,
            textFont, textColor,
            esphome::display::TextAlign::TOP_CENTER,  // Changed to TOP_CENTER for consistent positioning
            text
        );
    }
}

// ===== MAIN FUNCTION =====

/**
 * Draw weather information boxes at the bottom of the display
 * Each box shows an icon on the left and value on the right, with individual colors
 * 
 * @param it Display object
 * @param pFontValue Font for numeric values
 * @param pFontWeather Font for weather icons (Material Design Icons)
 * @param fTemperature Temperature in Celsius (always shown)
 * @param fHumidity Humidity percentage (shown if > 0)
 * @param fPression Pressure in hPa/mbar (shown if > 0)
 */
void drawBottomBoxes(
    esphome::display::Display &it,
    esphome::font::Font *pFontValue,
    esphome::font::Font *pFontWeather,
    float fTemperature,
    float fHumidity,
    float fPression
) {
    // ----- Configuration (dynamic based on screen height) -----
    int screenHeight = it.get_height();
    const int BOX_HEIGHT = 22;       // Box height
    const int Y_TOP = screenHeight - BOX_HEIGHT;  // Position boxes at bottom of screen
    const int ICON_TEXT_SPACING = 6; // Spacing between icon and text
    const int BOX_PADDING_H = 4;     // Horizontal padding inside each box
    const int SCREEN_MARGIN = 0;     // No margin needed - borders at x=0 are visible
    
    // ----- Common colors -----
    esphome::Color colorBorder = esphome::Color(128, 128, 128);   // Gray borders
    
    // ----- Determine which boxes to draw -----
    bool bShowHum = (fHumidity > 0.0f);
    bool bShowPres = (fPression > 0.0f);
    
    // Count parameters
    int numParams = 1;  // Temperature always shown
    if (bShowHum) numParams++;
    if (bShowPres) numParams++;
    
    // Determine if icons should be shown
    int screenWidth = it.get_width();
    bool bShowIcons = (numParams <= 2) || (screenWidth > 128);

    // Material Design Icons (UTF-8) - only used if bShowIcons is true
    const char *iconTemp = bShowIcons ? "\uf055" : "";  // mdi-thermometer
    const char *iconHum  = bShowIcons ? "\uf07a" : "";  // mdi-water-percent
    const char *iconPres = bShowIcons ? "\uf079" : "";  // mdi-gauge

    // ----- Format value strings -----
    char sTemp[16];
    char sHum[16];
    char sPres[20];
    
    snprintf(sTemp, sizeof(sTemp), "%.1f°C", fTemperature);
    if (bShowHum) snprintf(sHum, sizeof(sHum), "%.0f%%", fHumidity);
    if (bShowPres) snprintf(sPres, sizeof(sPres), "%.1fhPa", fPression);

    // ----- Calculate box widths based on content -----
    // When icons are hidden, pass nullptr for icon font to skip icon width calculation
    int wTemp = calcIconTextBoxWidth(it, iconTemp, sTemp, 
                                     bShowIcons ? pFontWeather : nullptr, pFontValue, 
                                     ICON_TEXT_SPACING, BOX_PADDING_H);
    int wHum = bShowHum ? calcIconTextBoxWidth(it, iconHum, sHum, 
                                               bShowIcons ? pFontWeather : nullptr, pFontValue, 
                                               ICON_TEXT_SPACING, BOX_PADDING_H) : 0;
    int wPres = bShowPres ? calcIconTextBoxWidth(it, iconPres, sPres, 
                                                 bShowIcons ? pFontWeather : nullptr, pFontValue, 
                                                 ICON_TEXT_SPACING, BOX_PADDING_H) : 0;

    // Calculate total natural width
    int totalNaturalWidth = wTemp + wHum + wPres;
    
    // Scale widths proportionally to fill screen (minus margins)
    int availableWidth = screenWidth - (2 * SCREEN_MARGIN);
    float scale = (float)availableWidth / (float)totalNaturalWidth;
    
    // Calculate scaled widths
    int wTempScaled = (int)(wTemp * scale);
    int wHumScaled = bShowHum ? (int)(wHum * scale) : 0;
    int wPresScaled = bShowPres ? (int)(wPres * scale) : 0;
    
    // Adjust last box to fill exact available width (handle rounding)
    int totalScaled = wTempScaled + wHumScaled + wPresScaled;
    if (bShowPres) {
        wPresScaled += (availableWidth - totalScaled);
    } else if (bShowHum) {
        wHumScaled += (availableWidth - totalScaled);
    } else {
        wTempScaled += (availableWidth - totalScaled);
    }
    
    // Starting position with margin
    int startX = SCREEN_MARGIN;

    // ----- Draw Temperature Box -----
    esphome::Color colorTempIcon = esphome::Color(255, 100, 0);   // Orange
    esphome::Color colorTempText = esphome::Color(255, 150, 50);  // Light orange
    
    it.rectangle(startX, Y_TOP, wTempScaled, BOX_HEIGHT, colorBorder);
    drawIconTextContent(it, startX, Y_TOP, wTempScaled, BOX_HEIGHT, iconTemp, sTemp, 
                       bShowIcons ? pFontWeather : nullptr, pFontValue, 
                       colorTempIcon, colorTempText,
                       ICON_TEXT_SPACING, BOX_PADDING_H);
    startX += wTempScaled;

    // ----- Draw Humidity Box -----
    if (bShowHum) {
        esphome::Color colorHumIcon = esphome::Color(0, 150, 255);    // Blue
        esphome::Color colorHumText = esphome::Color(100, 200, 255);  // Light blue
        
        it.rectangle(startX, Y_TOP, wHumScaled, BOX_HEIGHT, colorBorder);
        drawIconTextContent(it, startX, Y_TOP, wHumScaled, BOX_HEIGHT, iconHum, sHum,
                           bShowIcons ? pFontWeather : nullptr, pFontValue, 
                           colorHumIcon, colorHumText,
                           ICON_TEXT_SPACING, BOX_PADDING_H);
        startX += wHumScaled;
    }

    // ----- Draw Pressure Box -----
    if (bShowPres) {
        esphome::Color colorPresIcon = esphome::Color(0, 255, 100);   // Green
        esphome::Color colorPresText = esphome::Color(100, 255, 150); // Light green
        
        it.rectangle(startX, Y_TOP, wPresScaled, BOX_HEIGHT, colorBorder);
        drawIconTextContent(it, startX, Y_TOP, wPresScaled, BOX_HEIGHT, iconPres, sPres,
                           bShowIcons ? pFontWeather : nullptr, pFontValue, 
                           colorPresIcon, colorPresText,
                           ICON_TEXT_SPACING, BOX_PADDING_H);
    }
}


void menuDrawHomePage(
    esphome::display::Display &it
) {

}

/**
 * Draw the About page with system, network, and memory information
 * Automatically adapts layout based on screen orientation (portrait/landscape/square)
 * Optimizes line spacing based on actual font height
 * 
 * @param it Display object
 * @param pFont Font to use for all text
 * @param bMonoChrome If true, use white only; if false, use colors
 */

void menuDrawAboutPage(
    esphome::display::Display &it,
    esphome::font::Font *pFont,
    bool bMonoChrome
) {


    // ----- Gather all information first -----
    std::string deviceName = "";
    std::string buildTime = "";
    std::string wifiAddr = "";
    int8_t rssi = 0;
    uint32_t freeHeap = 0;
    uint32_t totalHeap = 0;
    uint32_t fragmentation = 0;
    std::vector<uint8_t> i2cAddresses;
    
    #ifdef USE_API
    deviceName = App.get_name();
    #endif
    
    buildTime = App.get_compilation_time();
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
    snprintf(rssiStr, sizeof(rssiStr), "%d dBm", rssi);
    snprintf(freeStr, sizeof(freeStr), "%.1f KB", freeHeap / 1024.0f);
    snprintf(totalStr, sizeof(totalStr), "%.1f KB", totalHeap / 1024.0f);
    snprintf(fragStr, sizeof(fragStr), "%d%%", fragmentation);
    
    if (days > 0) {
        snprintf(uptimeStr, sizeof(uptimeStr), "%dd %dh", days, hours);
    } else if (hours > 0) {
        snprintf(uptimeStr, sizeof(uptimeStr), "%dh %dm", hours, minutes);
    } else {
        snprintf(uptimeStr, sizeof(uptimeStr), "%dm", minutes);
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
    int screenW = it.get_width();
    int screenH = it.get_height();
    float aspectRatio = (float)screenW / (float)screenH;
    
    // Calculate optimal line spacing based on font height
    int fontHeight = pFont->get_height();
    int lineSpacing = fontHeight + 1;  // Minimum spacing: font height + 1 pixel
    int compactSpacing = fontHeight;   // For portrait mode
    
    // Colors
    esphome::Color labelColor = bMonoChrome ? esphome::Color::WHITE : esphome::Color(180, 180, 180);
    esphome::Color valueColor = esphome::Color::WHITE;
    esphome::Color accentColor = bMonoChrome ? esphome::Color::WHITE : esphome::Color(0, 200, 255);
    esphome::Color footerColor = bMonoChrome ? esphome::Color::WHITE : esphome::Color(100, 100, 100);
    esphome::Color i2cColor = bMonoChrome ? esphome::Color::WHITE : esphome::Color(255, 200, 0);
    
    int startY = 20;  // Start at top (title drawn elsewhere)
    
    // ----- Display based on aspect ratio -----
    int maxY = startY;  // Track the lowest Y position used
    
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
            snprintf(countStr, sizeof(countStr), "%d found", i2cAddresses.size());
            it.print(COL2_X, y2, pFont, labelColor, "I2C:"); 
            it.print(COL2_X + VAL_OFF, y2, pFont, i2cColor, countStr);
        }
        
        maxY = (y1 > y2) ? y1 : y2;  // Track max Y from both columns
        
    } else if (aspectRatio < 0.85f) {
        // PORTRAIT: Compact inline
        const int M = 4;
        int y = startY;
        
        if (!deviceName.empty()) { it.printf(M, y, pFont, accentColor, "%s", deviceName.c_str()); y += lineSpacing + 2; }
        it.printf(M, y, pFont, labelColor, "Build: %s", buildTime.c_str()); y += compactSpacing;
        if (!wifiAddr.empty()) {
            std::string ip = wifiAddr;
            if (ip.length() > 15) { size_t d = ip.find_last_of('.'); if (d != std::string::npos && d > 0) ip = "..." + ip.substr(d - 3); }
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
            y += compactSpacing;
        }
        
        maxY = y;
        
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

void drawTimeAndDate(
    esphome::display::Display &it,
    esphome::font::Font *pFontTime,
    esphome::font::Font *pFontDate,
    esphome::time::RealTimeClock *pTime
) {
    int iScreenWidth = it.get_width();
    int iScreenHeight = it.get_height();
    
    // Calculate Y positions dynamically (proportional to screen height)
    // For 128px height: time at 22-51, date at 50-65
    // Scale proportionally for other heights
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
    drawDsegText(it, 0, iScreenWidth - 1, time_y1, time_y2, pFontTime, time_str, Color(0xff7f00));
    drawDsegText(it, 0, iScreenWidth - 1, date_y1, date_y2, pFontDate, date_str, Color(0xff7f00));
}

/**
 * Draw weather condition and moon phase icons/text
 * Adapts layout based on day/night and presence of moon phase sensor
 * 
 * @param it Display object
 * @param pFontWeather Font for weather icons (Material Design Icons)
 * @param pFontMicro Font for micro text (condition string)
 * @param pWeatherCondition Text sensor for weather condition
 * @param pMoonPhaseIcon Text sensor for moon phase icon (optional)
 * @param pTime RealTimeClock for local time (optional, used to determine night)
 * 
 * @note confirmed weather condition values: sunny, clear-night, partlycloudy, cloudy, fog,
 * 
 * 
 */

void drawWeatherAndMoon(
    esphome::display::Display &it,
    esphome::font::Font *pFontWeather,
    esphome::font::Font *pFontMicro,
    esphome::text_sensor::TextSensor *pWeatherCondition,
    esphome::text_sensor::TextSensor *pMoonPhaseIcon,
    esphome::time::RealTimeClock *pTime
) {
    int screenW = it.get_width();
    int screenH = it.get_height();

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
        // Calculate Y positions dynamically
        int icon_y = (75 * screenH) / 128;
        int text_y = (100 * screenH) / 128;
        int cx = screenW / 2;
        // Draw N/A placeholder
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

    // Use a normalized form for robust matching (lowercase + patterns)
    std::string norm = met.normalizeCondition(cond);

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
        day_icon = "\uf002";
        night_icon = "\uf031";
        icon_color = Color::WHITE;
    } else if (norm == "cloudy") {
        day_icon = "\ue312"; // weather-cloudy
        night_icon = nullptr;
        icon_color = Color::WHITE;
    } else if (norm == "rain") {
        // includes showers, drizzle, etc. (see normalizeCondition)
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

    // Calculate Y positions dynamically based on screen height
    // For 128px: icon at 75, text at 100. Scale proportionally.
    // screenH already declared at function start
    int icon_y = (75 * screenH) / 128;
    int text_y = (100 * screenH) / 128;
    int moon_y = (80 * screenH) / 128;
    
    if (!is_night) {
        // Day: single symbol centered
        int cx = screenW / 2;
        it.print(cx, icon_y, pFontWeather, icon_color, esphome::display::TextAlign::CENTER, icon);
        it.print(cx, text_y, pFontMicro, Color(0x00ff00), esphome::display::TextAlign::CENTER, cond.c_str());
    } else {
        // Night: weather (left) and moon (right). For clear nights we avoid
        // drawing a weather icon on the left because the moon is shown on
        // the right (avoid duplicate moon glyphs).
        // Positions: first (left) and second (right) quarters
        int first_cx = screenW / 4;
        int second_cx = (screenW * 3) / 4;
        int center_cx = screenW / 2;

        // Use normalized condition helper for consistent matching across code
        // 'norm' was computed above when selecting icons
        bool draw_first = !(norm == "clear" || norm == "night");

        // Always draw the centered condition text so the condition is visible
        // in all layouts (day, night split, or clear-night with moon).
        it.print(center_cx, text_y, pFontMicro, Color(0x00ff00), esphome::display::TextAlign::CENTER, cond.c_str());

        // Draw icons:
        // - If draw_first == true: draw first icon at first_cx and moon at second_cx
        // - If draw_first == false: draw moon centered at center_cx (do not draw
        //   a right-only moon at second_cx)
        if (draw_first) {
            // first (weather) icon
            it.print(first_cx, moon_y, pFontWeather, icon_color, esphome::display::TextAlign::CENTER, icon);
            // second (moon) icon from moon phase sensor if present
            if (pMoonPhaseIcon != nullptr && pMoonPhaseIcon->has_state() && pMoonPhaseIcon->state != "") {
                const char *moon_icon = pMoonPhaseIcon->state.c_str();
                it.print(second_cx, moon_y, pFontWeather, Color(0xFFFF00), esphome::display::TextAlign::CENTER, moon_icon);
            } else {
                const char *fallback_na = "\uf141"; // N/A ellipsis glyph
                it.print(second_cx, moon_y, pFontWeather, Color(0xFFFF00), esphome::display::TextAlign::CENTER, fallback_na);
            }
        } else {
            // Only moon — center it so it's not a "right-only" symbol
            if (pMoonPhaseIcon != nullptr && pMoonPhaseIcon->has_state() && pMoonPhaseIcon->state != "") {
                const char *moon_icon = pMoonPhaseIcon->state.c_str();
                it.print(center_cx, moon_y, pFontWeather, Color(0xFFFF00), esphome::display::TextAlign::CENTER, moon_icon);
            } else {
                // Fallback: draw the N/A ellipsis glyph centered
                const char *fallback_na = "\uf141"; // N/A ellipsis glyph
                it.print(center_cx, moon_y, pFontWeather, Color(0xFFFF00), esphome::display::TextAlign::CENTER, fallback_na);
            }
        }
    }
}

void drawProgressPage( 
        esphome::display::Display &it,
        esphome::font::Font *pFont,
        const std::string& sText,
        float fValue,
        bool bMonoChrome
 ) {
    met.drawPageTitle(it,pFont,sText,true);

    //
    int iScreenWidth = it.get_width();
    int iScreenHeight = it.get_height();

    // Draw progress bar background
    int bar_x = 20;
    int bar_y = 70;
    int bar_width = iScreenWidth - 40;
    int bar_height = 20;
    it.rectangle(bar_x, bar_y, bar_width, bar_height, Color(0xFFFFFF));
    
    // Draw progress bar fill
    int fill_width = (bar_width - 4) * (fValue / 100.0);
    if (fill_width > 0) {
        it.filled_rectangle(bar_x + 2, bar_y + 2, fill_width, bar_height - 4, Color(0x00FF00));
    }
    
    // Draw percentage text
    it.printf(iScreenWidth/2, 100, pFont, Color(0xFFFFFF), esphome::display::TextAlign::CENTER, "%.0f%%", fValue);
}

// ---------------- cMet implementation ----------------


    void cMet::init(
        esphome::display::Display &it, 
        bool bMono,
        esphome::time::RealTimeClock *pTime
    ) {
        this->bMono = bMono;
        this->pTime = pTime;

    }


void cMet::drawPageTitle(esphome::display::Display &it, esphome::font::Font *pFont,
                   const std::string &sText, bool bSelected, bool bTime) {
    it.fill(Color(0x0));
    

    // init some page members
    iScreenWidth = it.get_width();
    iScreenHeight = it.get_height();
    iPageTop = 1 + pFont->get_height();
    
    // Determine color based on selection
    esphome::Color col = bSelected ? esphome::Color(0x00FF00) : esphome::Color::WHITE;

    const int iXOffset = 1;
    const int iYOffset = 1;

    // Draw the title text
    it.print( 3, 1, pFont, col, sText.c_str());

    // Draw underline (1 pixel height)
    it.line(0, iPageTop-1, iScreenWidth - 1, iPageTop-1, col);

    if (bTime) {
        auto now = pTime->now();
        if (now.is_valid()) {
            std::string time_str = now.strftime("%H:%M");
            drawDsegText( it, 2, iScreenWidth-2, 1, 15, pFont, time_str.c_str(), Color(0xff7f00), ALIGN_RIGHT);
        }
    }
    
}


std::string cMet::normalizeCondition(const std::string &cond) const {
    std::string s = cond;
    // lowercase
    for (auto &c: s) c = tolower(c);
    // common mappings
    if (s.find("clear") != std::string::npos && s.find("night") == std::string::npos) return "clear";
    if (s.find("cloud") != std::string::npos) return "cloudy";
    if (s.find("partly") != std::string::npos) return "partlycloudy";
    if (s.find("shower") != std::string::npos || s.find("rain") != std::string::npos || s.find("drizzle") != std::string::npos) return "rain";
    if (s.find("snow") != std::string::npos) return "snow";
    if (s.find("fog") != std::string::npos || s.find("mist") != std::string::npos) return "fog";
    if (s.find("wind") != std::string::npos) return "windy";
    if (s.find("hail") != std::string::npos) return "hail";
    if (s.find("night") != std::string::npos) return "night";
    return s;
}

void cMet::drawOptionsPage(   esphome::display::Display &it,
                         esphome::font::Font *pFont,
                         int selected_option,
                         esphome::sensor::Sensor *pSensor1,
                         esphome::sensor::Sensor *pSensor2,
                         esphome::sensor::Sensor *pSensor3) const  {

    int y = iPageTop;
    int x = 40;

// Compute line_height from font
    int font_height = pFont->get_height();
    int line_height = font_height + 2;  // +2 for spacing between lines
    
    // Draw vertical lines from y to screen_height - 1.5 * font_height
    int line_end_y = iScreenHeight - (int)(1.5f * font_height);
    esphome::Color line_col = bMono ? esphome::Color::WHITE : esphome::Color::WHITE;
    it.vertical_line(50, iPageTop + 1, line_end_y - y, line_col);
    it.vertical_line(54, iPageTop + 1, line_end_y - y, line_col);
    
    // Build list of valid sensors
    std::vector<sensor::Sensor*> sensors;
    if (pSensor1 != nullptr) sensors.push_back(pSensor1);
    if (pSensor2 != nullptr) sensors.push_back(pSensor2);
    if (pSensor3 != nullptr) sensors.push_back(pSensor3);
    
    int option_count = sensors.size();
    
    // Draw each option
    for (int i = 0; i < option_count; ++i) {
        int yy = y + i * line_height;
        sensor::Sensor *s = sensors[i];
        
        // Get label from sensor name
        std::string label = s->get_name();
        
        // Get state indicator and color
        float state = s->state;
        std::string state_str;
        esphome::Color state_col;
        
        if (bMono) {
            // Mono mode
            if (state == 0.0f) {
                state_str = "OFF";
            } else if (state == 1.0f) {
                state_str = "ON";
            } else {
                state_str = "?";
            }
            state_col = esphome::Color::WHITE;
        } else {
            // Color mode
            if (state == 0.0f) {
                state_str = "OFF ✕";  // Unicode X symbol (U+2715)
                state_col = esphome::Color(0xff0000);  // Red
            } else if (state == 1.0f) {
                state_str = "ON ✓";  // Unicode checkmark (U+2713)
                state_col = esphome::Color(0x00ff00);  // Green
            } else {
                state_str = "?";
                state_col = esphome::Color::WHITE;
            }
        }
        
        esphome::Color label_col = bMono ? esphome::Color::WHITE : esphome::Color::WHITE;
        
        // Draw selection rectangle if this option is selected
        if (i == selected_option) {
            esphome::Color sel_col = bMono ? esphome::Color::WHITE : esphome::Color(0x0000ff);  // Blue
            it.filled_rectangle(50, yy, 4, font_height, sel_col);  // width=2 (from x=41 to x=43)
        }
        
        // Draw state on left of line (right aligned at x=40)
        it.printf(50-5, yy, pFont, state_col, esphome::display::TextAlign::TOP_RIGHT, "%s", state_str.c_str());
        
        // Draw label on right of line (left aligned at x=44)
        it.printf(54+5, yy, pFont, label_col, esphome::display::TextAlign::TOP_LEFT, "%s", label.c_str());
    }
    
}

     // Define global instance
    cMet met;

}