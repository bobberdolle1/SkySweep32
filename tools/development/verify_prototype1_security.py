#!/usr/bin/env python3
"""Reject reintroduction of Prototype #1 review blockers into canonical Rev C."""
from __future__ import annotations

from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
FILES = {
    "web": ROOT / "src" / "web_server.cpp",
    "config": ROOT / "src" / "config_manager.cpp",
    "power": ROOT / "src" / "power_manager.cpp",
    "dashboard": ROOT / "src" / "dashboard.html",
    "main": ROOT / "src" / "main.cpp",
    "config_header": ROOT / "src" / "config.h",
    "generated_header": ROOT / "src" / "generated" / "hardware_rev_c.h",
}


def require(text: str, needle: str, subject: str, failures: list[str]) -> None:
    if needle not in text:
        failures.append(f"{subject}: missing {needle!r}")


def forbid(text: str, needle: str, subject: str, failures: list[str]) -> None:
    if needle in text:
        failures.append(f"{subject}: forbidden {needle!r}")

def main() -> int:
    content = {name: path.read_text(encoding="utf-8") for name, path in FILES.items()}
    failures: list[str] = []
    forbid(content["web"], '"/api/ota"', "web", failures)
    forbid(content["web"], "#include <Update.h>", "web", failures)
    forbid(content["dashboard"], "/api/ota", "dashboard", failures)
    forbid(content["config_header"], "WIFI_AP_PASSWORD", "config.h", failures)
    forbid(content["config_header"], "skysweep32", "config.h", failures)
    forbid(content["config"], 'kRetiredPublicPassword', "config", failures)
    require(content["config"], "ESP.getEfuseMac", "config", failures)
    require(content["web"], "requireManagementAuth", "web", failures)
    require(content["web"], '"/api/config", HTTP_POST', "web", failures)
    require(content["web"], '"/api/config/reset", HTTP_POST', "web", failures)
    require(content["web"], '"/api/power", HTTP_POST', "web", failures)
    forbid(content["power"], "POWER_SLEEP", "power", failures)
    forbid(content["power"], "POWER_LOW", "power", failures)
    require(content["power"], "POWER_BALANCED", "power", failures)
    forbid(content["generated_header"], "MODULE_REMOTE_ID", "generated header", failures)
    if failures:
        print("[FAIL] Prototype #1 security regression:")
        print("\n".join(failures))
        return 1
    print("[PASS] Prototype #1 OTA, network, power, and Remote ID guards hold")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
