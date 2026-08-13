#!/usr/bin/env python3
"""Check that canonical build documents reference current executable commands."""
from __future__ import annotations

import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
DOCUMENTS = (
    ROOT / "README.md",
    ROOT / "BUILD_THIS.md",
    *(ROOT / "hardware" / "rev_c").glob("*.md"),
    *(ROOT / "docs").rglob("*.md"),
)
COMMAND = re.compile(
    r"^python\s+([^\s]+)|^make\s+-C\s+([^\s]+)|^pio\s+run\s+-e\s+([^\s]+)",
    re.MULTILINE,
)


def main() -> int:
    failures: list[str] = []
    for document in DOCUMENTS:
        text = document.read_text(encoding="utf-8")
        for match in COMMAND.finditer(text):
            python_path, make_path, pio_env = match.groups()
            if python_path and not (ROOT / python_path).is_file():
                failures.append(f"{document.relative_to(ROOT)}: missing {python_path}")
            if make_path and not (ROOT / make_path / "Makefile").is_file():
                failures.append(f"{document.relative_to(ROOT)}: missing {make_path}/Makefile")
            if pio_env and f"[env:{pio_env}]" not in (
                ROOT / "platformio.ini"
            ).read_text(encoding="utf-8"):
                failures.append(
                    f"{document.relative_to(ROOT)}: undefined PlatformIO env {pio_env}"
                )
    if failures:
        print("[FAIL] canonical commands do not resolve:")
        print("\n".join(failures))
        return 1
    print("[PASS] canonical documented commands resolve")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
