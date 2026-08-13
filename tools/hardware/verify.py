#!/usr/bin/env python3
"""Rebuild Rev C evidence from the canonical KiCad sources and firmware contract."""

from __future__ import annotations

import argparse
from datetime import datetime, timezone
import importlib.metadata
import json
import os
from pathlib import Path
import re
import shutil
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[2]
REV = ROOT / "hardware" / "rev_c"
TOOLS = Path(__file__).resolve().parent / "rev_c"
sys.path.insert(0, str(TOOLS))
from tool_discovery import (  # noqa: E402
    discover_freecad_runner,
    discover_kicad_cli,
)

SCHEMATIC = REV / "skysweep32_rev_c.kicad_sch"
BOARD = REV / "skysweep32_rev_c.kicad_pcb"
VALIDATION = REV / "validation"
PROJECT = REV / "skysweep32_rev_c.kicad_pro"
PREVIEWS = REV / "previews"
TOOLCHAIN = json.loads((ROOT / "tools" / "hardware" / "toolchain.json").read_text(encoding="utf-8"))



def command(args: list[str], *, cwd: Path = ROOT) -> None:
    print("+", subprocess.list2cmdline(args), flush=True)
    subprocess.run(args, cwd=cwd, check=True)
def kicad_command(args: list[str], *, cwd: Path = ROOT) -> None:
    """Run KiCad CLI without letting it rewrite reviewed project rule settings."""
    project = PROJECT.read_bytes()
    try:
        command(args, cwd=cwd)
    finally:
        PROJECT.write_bytes(project)



def output(args: list[str], *, cwd: Path = ROOT) -> str:
    return subprocess.run(args, cwd=cwd, check=True, capture_output=True, text=True).stdout.strip()


def version_tuple(value: str) -> tuple[int, ...]:
    match = re.search(r"\d+(?:\.\d+)+", value)
    if not match:
        raise RuntimeError(f"cannot parse tool version from {value!r}")
    return tuple(int(part) for part in match.group(0).split("."))


def assert_reports() -> dict[str, object]:
    erc = (VALIDATION / "erc.rpt").read_text(encoding="utf-8")
    erc_exclusions = json.loads((VALIDATION / "erc_exclusions.json").read_text(encoding="utf-8"))
    drc = (VALIDATION / "drc.rpt").read_text(encoding="utf-8")
    drc_exclusions = json.loads((VALIDATION / "drc_exclusions.json").read_text(encoding="utf-8"))
    mechanical = json.loads((REV / "enclosure" / "mechanical_validation.json").read_text(encoding="utf-8"))
    if not re.search(r"ERC messages:\s*0\s+Errors\s+0\s+Warnings", erc):
        raise RuntimeError("ERC report is not zero-error/zero-warning")
    if erc_exclusions.get("ignored_checks"):
        raise RuntimeError(f"ERC has globally ignored checks: {erc_exclusions['ignored_checks']}")
    project = json.loads(PROJECT.read_text(encoding="utf-8"))
    expected_erc_uuids = {
        exclusion[0].split("|")[3]
        for exclusion in project["erc"]["erc_exclusions"]
    }
    if len(expected_erc_uuids) != 8:
        raise RuntimeError(f"expected eight reviewed ERC exclusions, found {len(expected_erc_uuids)}")
    erc_violations = [
        violation
        for sheet in erc_exclusions.get("sheets", [])
        for violation in sheet.get("violations", [])
    ]
    erc_uuids = {
        item["uuid"]
        for violation in erc_violations
        for item in violation.get("items", [])
    }
    if (
        len(erc_violations) != len(expected_erc_uuids)
        or erc_uuids != expected_erc_uuids
        or any(
            violation.get("type") != "footprint_filter"
            or not violation.get("excluded")
            or not violation.get("comment")
            or len(violation.get("items", [])) != 1
            for violation in erc_violations
        )
    ):
        raise RuntimeError(f"unexpected ERC exclusions: {erc_violations}")
    if "Found 0 DRC violations" not in drc or "Found 0 unconnected pads" not in drc:
        raise RuntimeError("DRC report is not zero-violation/zero-unconnected")
    if drc_exclusions.get("ignored_checks"):
        raise RuntimeError(f"DRC has globally ignored checks: {drc_exclusions['ignored_checks']}")
    exclusions = drc_exclusions.get("violations", [])
    if len(exclusions) != 1:
        raise RuntimeError(f"expected exactly one documented DRC exclusion, found {len(exclusions)}")
    exclusion = exclusions[0]
    excluded_items = exclusion.get("items", [])
    expected_drc_uuid = project["board"]["design_settings"]["drc_exclusions"][0][0].split("|")[3]
    if (
        exclusion.get("type") != "footprint_type_mismatch"
        or not exclusion.get("excluded")
        or exclusion.get("comment")
        != "TPS61232 is an SMD device; its exposed-pad thermal vias intentionally mix plated through-hole and SMD pads."
        or len(excluded_items) != 1
        or excluded_items[0].get("uuid") != expected_drc_uuid
    ):
        raise RuntimeError(f"unexpected DRC exclusion: {exclusion}")
    if mechanical.get("status") != "PASS" or mechanical.get("failures"):
        raise RuntimeError("mechanical validation did not pass")
    return mechanical


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--skip-firmware", action="store_true", help="skip the PlatformIO build")
    parser.add_argument("--skip-renders", action="store_true", help="skip PNG regeneration")
    args = parser.parse_args()
    source_revision = output(["git", "rev-parse", "HEAD"])
    source_tree_dirty = bool(output(["git", "status", "--porcelain"]))


    python_minimum = version_tuple(TOOLCHAIN["python"]["minimum_version"])
    if sys.version_info[: len(python_minimum)] < python_minimum:
        raise RuntimeError(f"Python {TOOLCHAIN['python']['minimum_version']} or newer is required")

    kicad = discover_kicad_cli()
    freecad = discover_freecad_runner()
    kicad_version = output([str(kicad), "--version"])
    required_kicad_major = int(TOOLCHAIN["kicad"]["required_major"])
    if version_tuple(kicad_version)[0] != required_kicad_major:
        raise RuntimeError(f"KiCad major {required_kicad_major} required, found {kicad_version}")
    sch_api_version = importlib.metadata.version("kicad-sch-api")
    required_sch_api = TOOLCHAIN["python"]["packages"]["kicad-sch-api"]
    if sch_api_version != required_sch_api:
        raise RuntimeError(f"kicad-sch-api {required_sch_api} required, found {sch_api_version}")

    pio = shutil.which("pio")
    if not args.skip_firmware and not pio:
        raise FileNotFoundError("PlatformIO pio not found on PATH")
    if not SCHEMATIC.is_file() or not BOARD.is_file():
        raise FileNotFoundError("Rev C canonical schematic/PCB is missing")

    VALIDATION.mkdir(parents=True, exist_ok=True)
    PREVIEWS.mkdir(parents=True, exist_ok=True)
    summary_path = VALIDATION / "verification_summary.json"
    summary_path.unlink(missing_ok=True)
    reviewed_project = PROJECT.read_bytes()
    command([sys.executable, str(ROOT / "tools" / "development" / "generate_rev_c_pinmap.py"), "--check"])
    command([sys.executable, str(ROOT / "tools" / "development" / "generate_dashboard.py"), "--check"])
    command([sys.executable, str(TOOLS / "verify_schematic_parity.py")])
    kicad_command([
        str(kicad), "sch", "erc", "--severity-error", "--severity-warning",
        "--exit-code-violations", "--output", str(VALIDATION / "erc.rpt"), str(SCHEMATIC),
    ])
    kicad_command([
        str(kicad), "sch", "erc", "--severity-exclusions", "--format", "json",
        "--output", str(VALIDATION / "erc_exclusions.json"), str(SCHEMATIC),
    ])
    kicad_command([
        str(kicad), "pcb", "drc", "--refill-zones", "--severity-error", "--severity-warning",
        "--exit-code-violations", "--output", str(VALIDATION / "drc.rpt"), str(BOARD),
    ])
    kicad_command([
        str(kicad), "pcb", "drc", "--refill-zones", "--severity-exclusions", "--format", "json",
        "--output", str(VALIDATION / "drc_exclusions.json"), str(BOARD),
    ])
    command([str(freecad), str(TOOLS / "generate_3d_models.py")], cwd=ROOT)
    kicad_command([
        str(kicad), "pcb", "export", "step", "--force", "--output",
        str(REV / "skysweep32_rev_c_pcba.step"), str(BOARD),
    ])
    command([str(freecad), str(TOOLS / "generate_enclosure.py")], cwd=ROOT)
    command([sys.executable, str(TOOLS / "generate_mechanical_drawing.py")], cwd=ROOT)

    if not args.skip_renders:
        kicad_command([
            str(kicad), "pcb", "render", "--quality", "high", "--floor", "--perspective",
            "--rotate", "35,0,-35", "--width", "1800", "--height", "1200",
            "--background", "opaque", "--output", str(PREVIEWS / "pcb_iso.png"), str(BOARD),
        ])
        kicad_command([
            str(kicad), "pcb", "render", "--quality", "high", "--floor", "--perspective",
            "--rotate", "35,0,-35", "--side", "bottom", "--width", "1800", "--height", "1200",
            "--background", "opaque", "--output", str(PREVIEWS / "pcb_bottom.png"), str(BOARD),
        ])
        kicad_command([
            str(kicad), "pcb", "render", "--quality", "high", "--floor", "--width", "1800",
            "--height", "1200", "--background", "opaque", "--output",
            str(PREVIEWS / "pcb_top.png"), str(BOARD),
        ])
        command([str(freecad), str(TOOLS / "render_enclosure.py")], cwd=ROOT)

    command([sys.executable, str(TOOLS / "export_manufacturing.py")], cwd=ROOT)
    if not args.skip_firmware:
        command([str(pio), "run", "-e", "esp32s3_rev_c_passive"])

    PROJECT.write_bytes(reviewed_project)
    mechanical = assert_reports()
    required_freecad = version_tuple(TOOLCHAIN["freecad"]["minimum_version"])
    if version_tuple(str(mechanical["freecad_version"])) < required_freecad:
        raise RuntimeError(
            f"FreeCAD {TOOLCHAIN['freecad']['minimum_version']} or newer required, "
            f"found {mechanical['freecad_version']}"
        )

    summary = {
        "design": "SkySweep32 Rev C Passive Monitor",
        "maturity": "READY_FOR_FIRST_PROTOTYPE",
        "production_validated": False,
        "timestamp_utc": datetime.now(timezone.utc).isoformat(),
        "source_revision": source_revision,
        "source_tree_dirty": source_tree_dirty,
        "tools": {
            "kicad": kicad_version,
            "kicad_sch_api": sch_api_version,
            "freecad": mechanical["freecad_version"],
            "freecad_runner": str(freecad),
            "platformio": output([str(pio), "--version"]) if pio else "skipped",
            "python": sys.version.split()[0],
        },
        "commands": {
            "pin_contract": "python tools/development/generate_rev_c_pinmap.py --check",
            "dashboard_embed": "python tools/development/generate_dashboard.py --check",
            "schematic_parity": "python tools/hardware/rev_c/verify_schematic_parity.py",
            "erc": "kicad-cli sch erc --severity-error --severity-warning --exit-code-violations",
            "drc": "kicad-cli pcb drc --refill-zones --severity-error --severity-warning --exit-code-violations",
            "pcba": "kicad-cli pcb export step --force",
            "mechanical": "FreeCAD generate_3d_models.py && FreeCAD generate_enclosure.py",
            "fabrication": "python hardware/rev_c/export_manufacturing.py",
            "firmware": "pio run -e esp32s3_rev_c_passive",
        },
        "gates": {
            "pin_contract": "PASS",
            "dashboard_embed": "PASS",
            "schematic_pin_to_net_parity": "PASS",
            "erc_zero_errors_warnings": "PASS",
            "erc_no_globally_ignored_checks": "PASS",
            "erc_eight_documented_footprint_filter_exclusions": "PASS",
            "drc_zero_violations_unconnected": "PASS",
            "drc_no_globally_ignored_checks": "PASS",
            "drc_single_documented_exclusion": "PASS",
            "mechanical_interference_and_service": mechanical["status"],
            "exact_bom_and_fabrication_exports": "PASS",
            "firmware_build": "SKIPPED" if args.skip_firmware else "PASS",
            "renders": "SKIPPED" if args.skip_renders else "PASS",
        },
        "evidence": {
            "erc": "validation/erc.rpt",
            "erc_exclusions": "validation/erc_exclusions.json",
            "drc": "validation/drc.rpt",
            "drc_exclusions": "validation/drc_exclusions.json",
            "mechanical": "enclosure/mechanical_validation.json",
            "fabrication": "manufacturing/fabrication_manifest.json",
            "mechanical_drawing": "enclosure/rev_c_mechanical_drawing.svg",
        },
    }
    summary_path.write_text(json.dumps(summary, indent=2) + "\n", encoding="utf-8")
    print(f"[PASS] Rev C verification complete: {summary_path}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
