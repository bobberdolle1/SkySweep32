# Contributing to SkySweep32

SkySweep32 is an open-source **passive multi-band RF monitoring system**. The
current physical design is Rev C, **ready for its first physical prototype, not
production validated**. Contributions must preserve the distinction between
source that builds and behavior demonstrated on a real board.

## Before opening a pull request

1. Read the [system documentation](docs/en/system.md) and the relevant
   [Rev C hardware package](hardware/rev_c/README.md).
2. Keep the change focused. Do not mix a hardware redesign, feature expansion,
   generated artifacts, and documentation rewrite in one change.
3. Run the applicable checks:

```bash
python tools/development/generate_rev_c_pinmap.py --check
python tools/development/generate_dashboard.py --check
python tools/development/verify_docs_links.py
make -C test/host
pio run -e esp32s3_rev_c_passive
```

For a real Rev C electrical/mechanical change, also run
`python tools/hardware/verify.py` with the documented KiCad/FreeCAD toolchain.
On Windows use WSL for the sanitizer-backed host test suite; see
[`test/host/README.md`](test/host/README.md).

## Firmware

- `hardware/rev_c/hardware_manifest.json` is the fitted-part/pin authority.
  Regenerate rather than hand-edit `src/generated/hardware_rev_c.h`.
- Use the one active PlatformIO environment: `esp32s3_rev_c_passive`.
- Every SPI user must coordinate through `spiManager`.
- Treat BLE, Wi-Fi, protocol frames, serial data, and log filenames as untrusted.
- Add a deterministic host test when changing a pure parser or another observable
  boundary. Do not add source-text, incidental-default, or fake-hardware tests.

## Hardware and enclosure

Use exact MPNs, manufacturer references, and native KiCad sources. Do not claim
that an envelope STEP model is manufacturer CAD. Do not move Rev C for cosmetic
reasons before Prototype #1 measurements. Report actual assembly, power, RF,
GNSS, UI, network, and mechanical evidence using the hardware-build issue template.

## Documentation and translations

English and Russian documentation are first-class and must agree on status and
capability boundaries. Update the source-of-truth document, not a duplicated
claim. Run `python tools/development/verify_docs_links.py` after changing docs
or the GitHub Pages site.

## Scope and legal boundary

Do not submit jamming, injection, deauthentication, GPS-denial, transmitter
identity-from-RSSI, or placeholder TinyML features. The ESP32-S3's ordinary
Wi-Fi/BLE/ESP-NOW networking remains an experimental product function and must
be described honestly. Follow applicable spectrum, privacy, aviation, and data laws.

By contributing, you license your work under [GPL-3.0](LICENSE).
