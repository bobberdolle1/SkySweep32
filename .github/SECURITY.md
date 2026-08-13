# Security Policy

## Supported Versions

| Version | Supported          |
| ------- | ------------------ |
| master  | :white_check_mark: |
| < 1.0   | :x:                |

## Reporting a Vulnerability

**DO NOT** open public issues for security vulnerabilities.

Report security issues through
[GitHub Security Advisories](https://github.com/bobberdolle1/SkySweep32/security/advisories/new)
so the report and any proof of concept remain private.

### What to include:
- Description of the vulnerability
- Steps to reproduce
- Potential impact
- Suggested fix (if any)

### Response timeline:
- Initial response: 48 hours
- Status update: 7 days
- Fix timeline: Depends on severity

## Security considerations

### Passive product scope

Canonical Rev C is passive with respect to the signals it observes: it contains
no RF-jamming, protocol-injection, deauthentication, or GPS-denial
implementation. Those functions are not supported.

“Passive monitor” does not mean RF-silent. The Wi-Fi dashboard and ESP-NOW
status/activity network intentionally transmit ordinary 2.4 GHz communications;
those paths are not countermeasures. The ESP32-S3, CC1101, and SX1281 are also
physically transmit-capable devices. Firmware configuration alone is not a
regulatory authorization. Contributors must preserve the passive observation
profile and comply with local radio, privacy, aviation, and data law.

### Local web server trust model

Rev C starts a WPA2-protected AP with a per-device random password generated at
first boot and retained in SPIFFS. The OLED and controlled USB serial console
show that credential only when it is generated. A factory reset removes the
stored configuration; the next boot generates and displays a replacement.

The dashboard and `/api/status` are intentionally readable by clients already
admitted to that AP. Management and sensitive log routes require HTTP Basic
authentication with username `admin` and the per-device AP password:

- **read-only:** `/`, `/api/status`, WebSocket telemetry;
- **state-mutating:** configuration update/reset and power policy selection;
- **sensitive:** SD log list/download, because logs can contain GNSS data.

There is no canonical network OTA/update route. Prototype #1 firmware updates
use native USB. Future OTA requires authenticated management, per-device
credentials, a firmware signature/authenticity strategy, and an explicit
secure-boot/rollback decision.

Treat BLE/Wi-Fi/RF parser input as untrusted. Relevant reports include:

- malformed packets causing memory corruption, reset loops, or resource
  exhaustion;
- bypass of management authorization, path traversal, or arbitrary file access;
- leakage of stored GNSS observations, logs, or network credentials;
- malicious firmware, dependency, manufacturing, or component substitutions.

## Responsible Disclosure

We follow responsible disclosure practices:
1. Report received and acknowledged
2. Vulnerability verified
3. Fix developed and tested
4. Security advisory published
5. CVE assigned (if applicable)

## Legal Notice

This project is for:
- Educational purposes
- Authorized defense applications
- Research in controlled environments

**Illegal use is strictly prohibited and not supported by maintainers.**
