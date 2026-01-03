````markdown
# ESPHome Configs and Components

This repository contains a collection of ESPHome configuration files, reusable packages, device models, keys and a small custom C++ component used across multiple ESPHome projects.

Quick overview

- Purpose: Share and maintain ESPHome YAML templates and packages for devices used in this environment (cameras, TTGO displays, remote devices, etc.).
- Primary users: Owners of the local network devices who run `esphome` to compile and flash firmware.

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

````