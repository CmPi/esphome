# ESPHome Configs and Components

This repository contains a collection of ESPHome configuration files, reusable packages, device models, keys and a small custom C++ component used across multiple ESPHome projects. These YAML configurations are provided as examples and templates to demonstrate common device setups and reusable patterns—please adapt them to your specific hardware, network and requirements before using in production.

**Device Models Included**
- `cam.yaml` — camera-related device model
- `esp32-c6.yaml` — ESP32-C6 based model
- `h801.yaml` — H801 RGBW controller model
- `mini-r4.yaml` — Mini R4 display model
- `mini-tv.yaml` — Mini TV display model
- `rd.yaml` — remote device model
- `sonoff-basic-r1-8285.yaml` — Sonoff Basic variant
- `sonoff-basic-r1.mini.yaml` — Sonoff Basic mini variant
- `sonoff-basic-r1.yaml` — Sonoff Basic model
- `sonoff-pow.yaml` — Sonoff POW (power monitoring)
- `sonoff-s20.mini.yaml` — Sonoff S20 mini variant
- `sonoff-s20.yaml` — Sonoff S20 model
- `sonoff-s26.yaml` — Sonoff S26 model
- `ttgo32.yaml` — TTGO T-Display / TTGO32 model
- `witty.yaml` — Witty ESP8266 board model

These are reusable snippets intended to be included from top-level device YAMLs; see the top-level files for full device examples.

**Common Sensor Types Used (examples)**
- `dallas_temp` — DS18B20 temperature sensors (one-wire)
- `hlw8012` — Power/energy sensors (current, voltage, power) used by Sonoff POW
- `wifi_signal` / `wifi_info` — RSSI, SSID, MAC and IP address
- `internal_temperature` — on-chip CPU temperature (ESP32)
- `template` — computed or derived sensors (template sensors)
- `homeassistant` — proxy sensors that read values from Home Assistant entities
- `version` / `text_sensor` — software versions, chip model, other textual info
- `binary_sensor` (GPIO) — buttons and switches

Typical measured values shown in examples:
- Temperature, humidity, pressure (via `homeassistant` or weather packages)
- Voltage, current, power and energy (via `hlw8012`)
- RSSI, IP, MAC
- Device diagnostics (flash size, CPU frequency, uptime)

If you want, I can replace the original `README.md` with this draft or merge these sections into the existing file. Which would you prefer?
