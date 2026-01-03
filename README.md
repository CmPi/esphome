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


Repository structure

- `*.yaml` — Top-level device configuration files (examples: `cam-2.yaml`, `mini-tv-1.yaml`).
- `models/` — Reusable device model snippets referenced by the top-level YAML files.
- `packages/` — ESPHome packages with shared configuration blocks to include in device YAMLs.
- `keys/` — Example or helper YAML snippets for logging / scripts (not secrets).
- `fonts/` — BDF fonts used by display devices.
- `src/` — Custom C++ components used by one or more ESPHome configs (`met.cpp`, `sysinfo.cpp`, etc.).
- `secrets.yaml` — Local secrets (API keys, Wi‑Fi). This file is not tracked here; add your own copy.

Getting started

1. Install ESPHome (recommended):

```bash
pip install esphome
# or use the docker/dashboard installer from esphome.io
```

2. Copy and edit `secrets.yaml` with your network credentials and any device-specific secrets.

3. Validate, compile or run a device config. Example:

```bash
esphome compile cam-2.yaml
esphome run cam-2.yaml   # builds and flashes (interactive)
```

Working with models and packages

- Top-level YAMLs typically `!include` files from `models/` or `packages/` to share common settings.
- To add a new shared block, create a YAML file under `packages/` and include it from device YAMLs.

Custom C++ components

- The `src/` folder contains small C++ helpers for ESPHome. These are compiled as part of the ESPHome build when referenced by a YAML.
- To change or extend behavior, edit `src/*.cpp`/`.h` and run `esphome compile <device>.yaml` to rebuild.

Contributing

- Add or update device YAMLs, models, packages or fonts as needed.
- Avoid committing sensitive data to `secrets.yaml` — keep secrets local.
- Open an issue or submit a PR describing the device and changes you made.

License

See the `LICENSE` file in the repository root.

