/**
 *  @file sysinfo.cpp
 *  @brief package defining optional sensors for dev/debug stage - useless in prod
 *  @author CmPi <cmpi@webe.fr>
 */

#include "sysinfo.h"


static uint32_t map_jedec_capacity(uint8_t manufacturer, uint8_t memory_type, uint8_t capacity) {
    switch (manufacturer) {
        case 0xEF: // Winbond
        case 0xC8: // GigaDevice
        case 0x20: // Macronix
        case 0x1F: // Adesto / Atmel
            switch (capacity) {
                case 0x13: return 1 << 20;   // 1 MB
                case 0x14: return 2 << 20;   // 2 MB
                case 0x15: return 4 << 20;   // 4 MB
                case 0x16: return 8 << 20;   // 8 MB
                case 0x17: return 16 << 20;  // 16 MB
                case 0x18: return 32 << 20;  // 32 MB
                default:   return 0;          // unknown
            }
        default:
            return 0; // unknown manufacturer
    }
}

namespace esphome {
namespace sysinfo {


  
float SysInfoComponent::get_jdec_flash_size_mb() {

    uint32_t flash_size = 0;

#ifdef USE_ESP8266
    flash_size = 0; 
    ESP_LOGW("sysinfo", "ESP8266 not fully supported in ESP-IDF branch");
#endif

#ifdef USE_ESP32
    esp_flash_t* chip = esp_flash_default_chip;

    uint32_t jedec_id = 0;
    esp_err_t err = esp_flash_read_id(chip, &jedec_id);
    if (err != ESP_OK) {
        ESP_LOGW("sysinfo", "Failed to read JEDEC ID, defaulting to 0");
        flash_size = 0;
    } else {
        uint8_t manufacturer = (jedec_id >> 16) & 0xFF;
        uint8_t memory_type  = (jedec_id >> 8)  & 0xFF;
        uint8_t capacity     = jedec_id & 0xFF;

        flash_size = map_jedec_capacity(manufacturer, memory_type, capacity);
        if (flash_size == 0) {
            ESP_LOGW("sysinfo", "Unknown JEDEC ID: %02X %02X %02X, defaulting to 4 MB",
                     manufacturer, memory_type, capacity);
            flash_size = 4 << 20;
        }

        ESP_LOGD("sysinfo", "JEDEC ID: %02X %02X %02X, flash size: %u bytes",
                 manufacturer, memory_type, capacity, flash_size);
    }
#endif
    float size_mb = flash_size / (1024.0f * 1024.0f);
    ESP_LOGD("sysinfo", "Flash size (JEDEC): %.2f MB", size_mb);
    return size_mb;

}

float SysInfoComponent::get_flash_size_mb() {
    uint32_t flash_size = 0;

#ifdef USE_ESP8266
    flash_size = 0; 
    ESP_LOGW("sysinfo", "ESP8266 not fully supported in ESP-IDF branch");
#endif

#ifdef USE_ESP32
    esp_err_t err = esp_flash_get_size(esp_flash_default_chip, &flash_size);
    if (err != ESP_OK) {
        ESP_LOGW("sysinfo", "Failed to get usable flash size, defaulting to 0");
        flash_size = 0;
    }
#endif

    float size_mb = flash_size / (1024.0f * 1024.0f);
    ESP_LOGD("sysinfo", "Flash size (usable): %.2f MB", size_mb);
    return size_mb;
}



uint32_t SysInfoComponent::get_flash_chip_speed() {
  uint32_t speed = 0;
  
  #ifdef USE_ESP8266
    speed = ESP.getFlashChipSpeed() / 1000000;
  #endif
  
  #ifdef USE_ESP32
    speed = 40;  // ESP32 typically runs flash at 40MHz
  #endif
  
  ESP_LOGD("sysinfo", "Flash speed: %u MHz", speed);
  return speed;
}

uint32_t SysInfoComponent::get_cpu_freq_mhz() {
  #ifdef USE_ESP8266
    return ESP.getCpuFreqMHz();
  #endif
  #ifdef USE_ESP32
    rtc_cpu_freq_config_t conf;
    rtc_clk_cpu_freq_get_config(&conf);
    return conf.freq_mhz;
  #endif
  return 0;
}

std::string SysInfoComponent::get_compile_target_model() {
  #ifdef USE_ESP8266
    return "ESP8266";
  #endif
  #ifdef USE_ESP32
    #if defined(IDF_TARGET_CHIP_NAME)
        std::string chip_name(IDF_TARGET_CHIP_NAME);
        // Return the specific name based on the macro.
        if (chip_name == "ESP32-C6") return "ESP32-C6";
        if (chip_name == "ESP32-S3") return "ESP32-S3";
        if (chip_name == "ESP32-C3") return "ESP32-C3";
        // Fallback for generic 'ESP32' target name
        if (chip_name == "ESP32") return "ESP32"; 
    #endif
    // If the macro is somehow undefined, return a generic ESP32 designation.
    return "ESP32 (Target Unknown)";
  #endif
  
  return "Unknown";
}

std::string SysInfoComponent::get_chip_model() {
  #ifdef USE_ESP8266
    return "ESP8266";
  #endif
  
  #ifdef USE_ESP32
    esp_chip_info_t chip_info;
    esp_chip_info(&chip_info);

    ESP_LOGD("sysinfo", "Runtime Chip Model ID: %u, Features: %u", chip_info.model, chip_info.features);
    
    switch(chip_info.model) {

      // ESP32
      #if defined(CHIP_ESP32)
      case CHIP_ESP32: return "ESP32";
      #else
      case 1: return "ESP32";
      #endif

      // ESP32-S2
      #if defined(CHIP_ESP32S2)
      case CHIP_ESP32S2: return "ESP32-S2";
      #else
      case 2: return "ESP32-S2";
      #endif

      // ESP32-S3 (ID 9 in your system)
      #if defined(CHIP_ESP32S3)
      case CHIP_ESP32S3: return "ESP32-S3";
      #else
      case 9: return "ESP32-S3";
      #endif

    // ESP32-C3 (ID 5 in your system)
      #if defined(CHIP_ESP32C3)
      case CHIP_ESP32C3: return "ESP32-C3"; 
      #else
      case 5: return "ESP32-C3"; 
      #endif

      // ESP32-C2 (ID 12 in your system)
      #if defined(CHIP_ESP32C2)
      case CHIP_ESP32C2: return "ESP32-C2"; 
      #else
      case 12: return "ESP32-C2"; 
      #endif
      
      // ESP32-C6 (ID 13 in your system)
      #if defined(CHIP_ESP32C6)
      case CHIP_ESP32C6: return "ESP32-C6"; 
      #else
      case 13: return "ESP32-C6"; 
      #endif

      #ifdef CHIP_ESP32H2
      case CHIP_ESP32H2: return "ESP32-H2";
      #else
      case 7: return "ESP32-H2";      
      #endif

      default: return "ESP32 (D)";
    }
  #endif
  
  return "Unknown";
}
uint32_t SysInfoComponent::get_chip_revision() {
  #ifdef USE_ESP8266
    return ESP.getChipId();
  #endif
  
  #ifdef USE_ESP32
    esp_chip_info_t chip_info;
    esp_chip_info(&chip_info);
    return chip_info.revision;
  #endif
  
  return 0;
}

uint32_t SysInfoComponent::get_cpu_cores() {
  #ifdef USE_ESP8266
    return 1;
  #endif
  
  #ifdef USE_ESP32
    esp_chip_info_t chip_info;
    esp_chip_info(&chip_info);
    return chip_info.cores;
  #endif
  
  return 0;
}

}  // namespace sysinfo
}  // namespace esphome