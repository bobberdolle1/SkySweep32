#!/usr/bin/env python3
"""Reject broken local Markdown and Pages links in the public documentation."""
from __future__ import annotations

import re
import sys
from pathlib import Path
from urllib.parse import unquote, urlparse

ROOT = Path(__file__).resolve().parents[2]
DOCS = ROOT / "docs"
MARKDOWN_LINK = re.compile(r"(?<!!)\[[^]]*]\(([^)]+)\)")
HTML_LINK = re.compile(r'''(?:href|src)=["']([^"']+)["']''')


def local_target(source: Path, target: str) -> Path | None:
    target = target.strip().split(maxsplit=1)[0]
    parsed = urlparse(target)
    if parsed.scheme or target.startswith("#") or target.startswith("//"):
        return None
    path = unquote(parsed.path)
    if not path:
        return None
    return (source.parent / path).resolve()


def links(path: Path) -> list[str]:
    text = path.read_text(encoding="utf-8")
    pattern = HTML_LINK if path.suffix == ".html" else MARKDOWN_LINK
    return pattern.findall(text)


def documentation_sources() -> list[Path]:
    root_documents = [
        ROOT / "README.md",
        ROOT / "BUILD_THIS.md",
        ROOT / "CONTRIBUTING.md",
    ]
    return [
        *[path for path in root_documents if path.is_file()],
        *DOCS.rglob("*.md"),
        *DOCS.rglob("*.html"),
        *(ROOT / "hardware" / "rev_c").glob("*.md"),
    ]


def main() -> int:
    failures: list[str] = []
    for path in documentation_sources():
        for target in links(path):
            resolved = local_target(path, target)
            if resolved is not None and not resolved.exists():
                failures.append(f"{path.relative_to(ROOT)}: missing {target}")
    if failures:
        print("[FAIL] broken local documentation links:")
        print("\n".join(failures))
        return 1
    print("[PASS] local Markdown and Pages links resolve")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
