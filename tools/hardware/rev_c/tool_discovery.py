"""Portable discovery for the CAD tools used by Rev C generators."""

from __future__ import annotations

import os
from pathlib import Path
import shutil
import sys


def _first_file(candidates: list[Path], description: str) -> Path:
    for candidate in candidates:
        if candidate.is_file():
            return candidate.resolve()
    raise FileNotFoundError(description)


def discover_kicad_root() -> Path:
    roots: list[Path] = []
    if configured := os.environ.get("KICAD_ROOT"):
        roots.append(Path(configured))
    if cli := shutil.which("kicad-cli"):
        roots.append(Path(cli).resolve().parent.parent)
    if os.name == "nt":
        for name in ("LOCALAPPDATA", "PROGRAMFILES"):
            if root := os.environ.get(name):
                roots.extend(sorted(Path(root).glob("Programs/KiCad/*"), reverse=True))
                roots.extend(sorted(Path(root).glob("KiCad/*"), reverse=True))
    else:
        roots.extend((Path("/usr"), Path("/usr/local"), Path("/Applications/KiCad/KiCad.app/Contents/SharedSupport")))
    for root in roots:
        cli_names = (root / "bin" / "kicad-cli.exe", root / "bin" / "kicad-cli")
        if any(path.is_file() for path in cli_names) and (root / "share" / "kicad").is_dir():
            return root.resolve()
    raise FileNotFoundError("KiCad installation not found; set KICAD_ROOT or add kicad-cli to PATH")


def discover_kicad_cli(root: Path | None = None) -> Path:
    root = root or discover_kicad_root()
    return _first_file(
        [root / "bin" / "kicad-cli.exe", root / "bin" / "kicad-cli"],
        "kicad-cli not found in discovered KiCad installation",
    )


def discover_kicad_python(root: Path | None = None) -> Path:
    if configured := os.environ.get("KICAD_PYTHON"):
        return _first_file([Path(configured)], "KICAD_PYTHON does not name a file")
    root = root or discover_kicad_root()
    candidates = [root / "bin" / "python.exe", root / "bin" / "python3", root / "bin" / "python"]
    if "pcbnew" in sys.modules:
        candidates.insert(0, Path(sys.executable))
    return _first_file(candidates, "KiCad Python not found; set KICAD_PYTHON")


def discover_freecad_runner() -> Path:
    if configured := os.environ.get("FREECAD_PYTHON"):
        return _first_file([Path(configured)], "FREECAD_PYTHON does not name a file")
    candidates: list[Path] = []
    if os.name == "nt":
        for name in ("LOCALAPPDATA", "PROGRAMFILES"):
            if root := os.environ.get(name):
                candidates.extend(sorted(Path(root).glob("Programs/FreeCAD */bin/python.exe"), reverse=True))
                candidates.extend(sorted(Path(root).glob("FreeCAD */bin/python.exe"), reverse=True))
    else:
        for executable in ("freecadcmd", "FreeCADCmd", "freecad"):
            if found := shutil.which(executable):
                candidates.append(Path(found))
    return _first_file(candidates, "FreeCAD command/Python not found; set FREECAD_PYTHON")
