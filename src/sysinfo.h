/**
 *  @file sysinfo.h
 *  @brief package defining optional sensors for dev/debug stage - useless in prod
 *  @author CmPi <cmpi@webe.fr>
 */

#pragma once

#include "esphome.h"

#ifdef USE_ESP8266
#include "user_interface.h"
#endif

#ifdef USE_ESP32
#include "esp_flash.h"
#include "spi_flash_mmap.h"
#include "esp_chip_info.h"
#include "esp_idf_version.h"
#include "esp_system.h"
#include "soc/rtc.h"
#endif

namespace esphome {
namespace sysinfo {

class SysInfoComponent {
 public:
  static float get_flash_size_mb();
  static float get_jdec_flash_size_mb();
  static uint32_t get_flash_chip_speed();
  static uint32_t get_cpu_freq_mhz();
  static std::string get_compile_target_model();  
  static std::string get_chip_model();
  static uint32_t get_chip_revision();
  static uint32_t get_cpu_cores();
};

}  // namespace sysinfo
}  // namespace esphome