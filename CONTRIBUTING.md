# Contributing to SkySweep32

Thank you for your interest in contributing! SkySweep32 is an open-source project and welcomes contributions of all kinds.

## 🚀 How to Contribute

### 1. Bug Reports & Feature Requests

- Use [GitHub Issues](https://github.com/bobberdolle1/SkySweep32/issues)
- Include your hardware tier (Base/Standard/Pro), firmware version, and serial output
- For bugs: describe expected vs actual behavior

### 2. Code Contributions

1. Fork the repository
2. Create a feature branch: `git checkout -b feature/my-feature`
3. Make your changes following the conventions below
4. Test on real hardware if possible
5. Submit a Pull Request

### 3. Documentation

- Fix typos, improve explanations, add examples
- Translate to new languages
- Create tutorials or video guides

### 4. Hardware Testing

- Test with different ESP32 variants (DevKit V1, ESP32-S3, WROOM)
- Report range and detection accuracy
- Share antenna comparison results

---

## 📐 Code Conventions

### C++

- Use `camelCase` for variables and functions
- Use `PascalCase` for classes
- Use `UPPER_SNAKE_CASE` for `#define` constants
- Include module guard: `#ifdef MODULE_*` ... `#endif`
- Use config.h for all pin definitions and constants
- Always use `spiManager.acquire()` / `spiManager.release()` for SPI access

### File Structure

```
src/
├── config.h              # Central configuration
├── config_manager.h/cpp  # Runtime JSON config
├── spi_manager.h/cpp     # Thread-safe SPI
├── main.cpp              # FreeRTOS task setup
├── web_server.h/cpp      # Dashboard + API
├── drivers/              # Hardware drivers
│   ├── cc1101.h/cpp
│   ├── nrf24l01.h/cpp
│   └── rx5808.h/cpp
├── protocols/            # Protocol parsers
│   ├── mavlink_parser.h/cpp
│   └── crsf_parser.h/cpp
└── [module].h/cpp        # Feature modules
```

### Commit Messages

Use conventional commits:
```
feat: add 433 MHz CC1101 support
fix: resolve SPI bus contention on RX5808
docs: update wiring diagram for Pro tier
refactor: extract RSSI history to separate class
```

---

## 🔬 Priority Contribution Areas

1. **RF Signature Database** — Record real drone signals and share datasets
2. **TFLite Model Training** — Train classification models on real data
3. **ESP-NOW Mesh** — Implement multi-node communication
4. **Web Dashboard UX** — Improve mobile responsiveness, add map view
5. **3D Enclosure** — Design printable cases for each tier
6. **Translations** — Add Chinese, Spanish, German docs
7. **CI/CD** — GitHub Actions for automated builds on all tiers

---

## ⚖️ Legal Notice

> ⚠️ **Do NOT submit code that enables illegal RF jamming or GPS spoofing without explicit authorization safeguards.** All countermeasure code must be gated behind `ENABLE_COUNTERMEASURES` and include appropriate warnings.

---

## 📜 License

By contributing, you agree that your contributions will be licensed under the [GPL-3.0 License](LICENSE).
