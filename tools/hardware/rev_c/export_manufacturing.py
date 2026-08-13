#!/usr/bin/env python3
"""Export the reviewed Rev C first-prototype manufacturing package."""

from __future__ import annotations

import csv
import hashlib
import json
from pathlib import Path
import shutil
import subprocess
import zipfile
from tool_discovery import discover_kicad_cli

ROOT = Path(__file__).resolve().parents[3]
REV = ROOT / "hardware" / "rev_c"
SCHEMATIC = REV / "skysweep32_rev_c.kicad_sch"
BOARD = REV / "skysweep32_rev_c.kicad_pcb"
MANIFEST = REV / "hardware_manifest.json"
PROJECT = REV / "skysweep32_rev_c.kicad_pro"
OUT = REV / "manufacturing"
GERBERS = OUT / "gerbers"



def run(*args: str) -> None:
    project = PROJECT.read_bytes()
    try:
        subprocess.run([str(KICAD), *args], cwd=ROOT, check=True)
    finally:
        PROJECT.write_bytes(project)


def export_bom(path: Path, *, exclude_dnp: bool) -> None:
    args = [
        "sch", "export", "bom", str(SCHEMATIC), "--output", str(path),
        "--fields", "Reference,Value,Footprint,Manufacturer,MPN,QUANTITY,DNP",
        "--labels", "Refs,Value,Footprint,Manufacturer,MPN,Qty,DNP",
        "--group-by", "Value,Footprint,Manufacturer,MPN,DNP",
    ]
    if exclude_dnp:
        args.append("--exclude-dnp")
    run(*args)


def validate_exact_bom(path: Path) -> None:
    with path.open(newline="", encoding="utf-8-sig") as stream:
        rows = list(csv.DictReader(stream))
    missing = [row["Refs"] for row in rows if not row["DNP"] and not row["MPN"]]
    if missing:
        raise RuntimeError(f"fitted BOM rows lack exact MPNs: {', '.join(missing)}")


def write_accessories() -> None:
    manifest = json.loads(MANIFEST.read_text(encoding="utf-8"))
    rows = [
        [
            item["refs"],
            str(item["qty"]),
            item["manufacturer"],
            item["mpn_or_standard"],
            item["variant"],
            item["description"],
        ]
        for item in manifest["assembly_items"]
    ]
    with (OUT / "assembly_items.csv").open("w", newline="", encoding="utf-8") as stream:
        writer = csv.writer(stream, lineterminator="\n")
        writer.writerow(["Refs", "Qty", "Manufacturer", "MPN_or_standard", "Variant", "Description"])
        writer.writerows(rows)

def strip_trailing_whitespace(path: Path) -> None:
    """Normalize KiCad SVG output so generated artifacts pass repository checks."""
    path.write_text(
        "\n".join(line.rstrip() for line in path.read_text(encoding="utf-8").splitlines()) + "\n",
        encoding="utf-8",
    )




def write_deterministic_zip(files: list[Path], destination: Path) -> None:
    with zipfile.ZipFile(destination, "w", compression=zipfile.ZIP_DEFLATED, compresslevel=9) as archive:
        for path in sorted(files, key=lambda item: item.name):
            info = zipfile.ZipInfo(path.name, date_time=(2026, 8, 11, 0, 0, 0))
            info.compress_type = zipfile.ZIP_DEFLATED
            info.external_attr = 0o100644 << 16
            archive.writestr(info, path.read_bytes())


def main() -> int:
    if not SCHEMATIC.is_file() or not BOARD.is_file():
        raise FileNotFoundError("canonical schematic or PCB is missing")
    OUT.mkdir(parents=True, exist_ok=True)
    if GERBERS.exists():
        shutil.rmtree(GERBERS)
    GERBERS.mkdir()
    for name in (
        "bom.csv", "bom_fitted.csv", "positions.csv", "assembly_items.csv",
        "assembly_drawing.pdf", "assembly_top.svg", "assembly_bottom.svg",
        "schematic.pdf", "drill_report.rpt",
        "skysweep32_rev_c_gerbers.zip", "fabrication_manifest.json",
    ):
        (OUT / name).unlink(missing_ok=True)

    export_bom(OUT / "bom.csv", exclude_dnp=False)
    export_bom(OUT / "bom_fitted.csv", exclude_dnp=True)
    validate_exact_bom(OUT / "bom.csv")
    write_accessories()

    run(
        "pcb", "export", "pos", str(BOARD), "--output", str(OUT / "positions.csv"),
        "--format", "csv", "--units", "mm", "--side", "both", "--exclude-dnp",
    )
    run(
        "pcb", "export", "gerbers", str(BOARD), "--output", str(GERBERS),
        "--layers", "F.Cu,In1.Cu,In2.Cu,B.Cu,F.Paste,B.Paste,F.SilkS,B.SilkS,F.Mask,B.Mask,Edge.Cuts",
        "--subtract-soldermask", "--check-zones",
    )
    run(
        "pcb", "export", "drill", str(BOARD), "--output", str(GERBERS),
        "--format", "excellon", "--excellon-units", "mm", "--excellon-separate-th",
        "--generate-map", "--map-format", "gerberx2", "--generate-report",
        "--report-path", str(OUT / "drill_report.rpt"),
    )
    run(
        "pcb", "export", "pdf", str(BOARD), "--output", str(OUT / "assembly_drawing.pdf"),
        "--layers", "F.Fab,B.Fab,Edge.Cuts", "--mode-multipage", "--black-and-white",
        "--sketch-pads-on-fab-layers", "--crossout-DNP-footprints-on-fab-layers",
    )
    svg_options = (
        "--mode-single", "--fit-page-to-board", "--exclude-drawing-sheet",
        "--black-and-white", "--sketch-pads-on-fab-layers",
        "--crossout-DNP-footprints-on-fab-layers",
    )
    run(
        "pcb", "export", "svg", str(BOARD), "--output", str(OUT / "assembly_top.svg"),
        "--layers", "F.Fab,F.Silkscreen,Edge.Cuts", *svg_options,
    )
    run(
        "pcb", "export", "svg", str(BOARD), "--output", str(OUT / "assembly_bottom.svg"),
        "--layers", "B.Fab,B.Silkscreen,Edge.Cuts", "--mirror", *svg_options,
    )
    strip_trailing_whitespace(OUT / "assembly_top.svg")
    strip_trailing_whitespace(OUT / "assembly_bottom.svg")
    run(
        "sch", "export", "pdf", str(SCHEMATIC), "--output", str(OUT / "schematic.pdf"),
        "--black-and-white", "--no-background-color",
    )

    package_files = [path for path in GERBERS.iterdir() if path.is_file()]
    write_deterministic_zip(package_files, OUT / "skysweep32_rev_c_gerbers.zip")
    version = subprocess.run([str(KICAD), "--version"], capture_output=True, text=True, check=True).stdout.strip()
    manifest = {
        "design": "SkySweep32 Rev C",
        "maturity": "READY_FOR_FIRST_PROTOTYPE",
        "production_validated": False,
        "kicad_version": version,
        "board": BOARD.name,
        "stackup": "4 layer, 1.6 mm FR-4, ENIG; see README.md",
        "files": {
            path.name: hashlib.sha256(path.read_bytes()).hexdigest()
            for path in sorted(package_files, key=lambda item: item.name)
        },
    }
    (OUT / "fabrication_manifest.json").write_text(
        json.dumps(manifest, indent=2) + "\n", encoding="utf-8"
    )
    print(f"[PASS] exported {len(package_files)} fabrication files to {OUT}")
    return 0


KICAD = discover_kicad_cli()

if __name__ == "__main__":
    raise SystemExit(main())
