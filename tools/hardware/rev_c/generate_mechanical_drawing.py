#!/usr/bin/env python3
"""Generate the Rev C first-prototype mechanical interface drawing as SVG."""

from __future__ import annotations

import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[3]
REV = ROOT / "hardware" / "rev_c"
MANIFEST = json.loads((REV / "hardware_manifest.json").read_text(encoding="utf-8"))
OUT = REV / "enclosure" / "rev_c_mechanical_drawing.svg"

BOARD_W, BOARD_H = MANIFEST["mechanical"]["board_dimensions_mm"]
HOLES = MANIFEST["mechanical"]["mounting_holes"]
PARAMETERS = MANIFEST["mechanical"]["enclosure_parameters_mm"]
EDGE_LRT = PARAMETERS["pcb_edge_clearance_left_right_top"]
EDGE_BOTTOM = PARAMETERS["pcb_edge_clearance_bottom"]
WALL = PARAMETERS["wall_thickness"]
LID_OVERHANG = PARAMETERS["lid_overhang"]
LID_TOP = PARAMETERS["lid_top_thickness"]
LID_CEILING_Z = PARAMETERS["pcb_to_lid_internal_height"]
FASTENER_LENGTH = PARAMETERS["fastener_length"]
FASTENER_RECESS = PARAMETERS["fastener_tip_recess"]
LID_MARGIN_SIDE = EDGE_LRT + WALL + LID_OVERHANG
LID_MARGIN_TOP = LID_MARGIN_SIDE
LID_MARGIN_BOTTOM = EDGE_BOTTOM + WALL + LID_OVERHANG
OUTER_W = BOARD_W + 2.0 * LID_MARGIN_SIDE
OUTER_H = BOARD_H + LID_MARGIN_TOP + LID_MARGIN_BOTTOM
LID_TOP_Z = LID_CEILING_Z + LID_TOP
FOOT_BOTTOM_Z = LID_CEILING_Z - FASTENER_LENGTH - FASTENER_RECESS
OUTER_Z = LID_TOP_Z - FOOT_BOTTOM_Z
BATTERY_W, _BATTERY_D, BATTERY_H = MANIFEST["major_parts"]["BAT1"]["maximum_envelope_mm"]
BATTERY_CENTER_X, _BATTERY_CENTER_Y = MANIFEST["mechanical"]["battery_center_mm"]
BATTERY_X = BATTERY_CENTER_X - BATTERY_W / 2.0
BATTERY_TOP_Z = -PARAMETERS["battery_to_board_clearance"]
SCALE = 5.0
MARGIN = 80
TOP_X, TOP_Y = 100, 100
FRONT_X, FRONT_Y = 100, 800


def sx(mm: float) -> float:
    return TOP_X + (mm + LID_MARGIN_SIDE) * SCALE


def sy(mm: float) -> float:
    return TOP_Y + (LID_MARGIN_TOP + mm) * SCALE


def line(x1: float, y1: float, x2: float, y2: float, cls: str = "object") -> str:
    return f'<line x1="{x1:.1f}" y1="{y1:.1f}" x2="{x2:.1f}" y2="{y2:.1f}" class="{cls}"/>'


def text(x: float, y: float, value: str, cls: str = "note", anchor: str = "middle") -> str:
    return f'<text x="{x:.1f}" y="{y:.1f}" class="{cls}" text-anchor="{anchor}">{value}</text>'


def dim_h(x1: float, x2: float, y: float, witness_y: float, label: str) -> list[str]:
    return [
        line(x1, witness_y, x1, y, "extension"),
        line(x2, witness_y, x2, y, "extension"),
        line(x1, y, x2, y, "dimension"),
        text((x1 + x2) / 2, y - 8, label, "dimension-text"),
    ]


def dim_v(y1: float, y2: float, x: float, witness_x: float, label: str) -> list[str]:
    return [
        line(witness_x, y1, x, y1, "extension"),
        line(witness_x, y2, x, y2, "extension"),
        line(x, y1, x, y2, "dimension"),
        text(x - 8, (y1 + y2) / 2, label, "dimension-text", "middle").replace(">", ' transform="rotate(-90 %.1f %.1f)">' % (x - 8, (y1 + y2) / 2), 1),
    ]


items: list[str] = []
# Top view: enclosure and PCB share the KiCad lower-left/top-view datum.
outer_x, outer_y = sx(-LID_MARGIN_SIDE), sy(-LID_MARGIN_TOP)
outer_w, outer_h = OUTER_W * SCALE, OUTER_H * SCALE
board_x, board_y = sx(0), sy(0)
items.append(f'<rect x="{outer_x}" y="{outer_y}" width="{outer_w}" height="{outer_h}" rx="41" class="object thick"/>')
items.append(f'<rect x="{board_x}" y="{board_y}" width="{BOARD_W*SCALE}" height="{BOARD_H*SCALE}" rx="15" class="pcb"/>')
for x, y, diameter in HOLES:
    items.append(f'<circle cx="{sx(x)}" cy="{sy(y)}" r="{diameter*SCALE/2}" class="hole"/>')
    items.append(line(sx(x) - 8, sy(y), sx(x) + 8, sy(y), "center"))
    items.append(line(sx(x), sy(y) - 8, sx(x), sy(y) + 8, "center"))

items += dim_h(outer_x, outer_x + outer_w, outer_y - 35, outer_y, f"{OUTER_W:.1f} OUTER")
items += dim_v(outer_y, outer_y + outer_h, outer_x - 35, outer_x, f"{OUTER_H:.1f} OUTER")
items += dim_h(board_x, board_x + BOARD_W*SCALE, outer_y + outer_h + 48, board_y + BOARD_H*SCALE, f"{BOARD_W:.1f} PCB")
items += dim_v(board_y, board_y + BOARD_H*SCALE, outer_x + outer_w + 48, board_x + BOARD_W*SCALE, f"{BOARD_H:.1f} PCB")
hole_x = sorted({float(hole[0]) for hole in HOLES})
hole_y = sorted({float(hole[1]) for hole in HOLES})
items += dim_h(sx(hole_x[0]), sx(hole_x[-1]), outer_y + outer_h + 85, sy(hole_y[-1]), f"{hole_x[-1] - hole_x[0]:.1f} HOLE PITCH")
items += dim_v(sy(hole_y[0]), sy(hole_y[-1]), outer_x + outer_w + 100, sx(hole_x[-1]), f"{hole_y[-1] - hole_y[0]:.1f} HOLE PITCH")
items.append(text(TOP_X + outer_w/2, 25, "TOP VIEW — LID REMOVED", "title"))
items.append(text(sx(BOARD_W/2), sy(BOARD_H/2), "PCB DATUM X=0, Y=0", "note"))
items.append(text(sx(BOARD_W/2), sy(BOARD_H/2 + 5), "4 x Ø3.2 PTH; M3 FASTENERS", "note"))

# Front/side elevation and vertical stack.
fx, fy = FRONT_X, FRONT_Y
items.append(f'<rect x="{fx}" y="{fy}" width="{OUTER_W*SCALE}" height="{OUTER_Z*SCALE}" rx="12" class="object thick"/>')
pcb_z = fy + (LID_TOP_Z - 1.6) * SCALE
items.append(f'<rect x="{fx + LID_MARGIN_SIDE*SCALE}" y="{pcb_z}" width="{BOARD_W*SCALE}" height="{1.6*SCALE}" class="pcb"/>')
battery_y = fy + (LID_TOP_Z - BATTERY_TOP_Z) * SCALE
items.append(f'<rect x="{fx + (LID_MARGIN_SIDE + BATTERY_X)*SCALE}" y="{battery_y}" width="{BATTERY_W*SCALE}" height="{BATTERY_H*SCALE}" class="battery"/>')
items += dim_h(fx, fx + OUTER_W*SCALE, fy - 30, fy, f"{OUTER_W:.1f} OUTER")
items += dim_v(fy, fy + OUTER_Z*SCALE, fx - 35, fx, f"{OUTER_Z:.1f} CLOSED HEIGHT")
items.append(text(fx + OUTER_W*SCALE/2, fy - 80, "FRONT ELEVATION", "title"))
items.append(text(fx + OUTER_W*SCALE/2, fy + OUTER_Z*SCALE + 35, "PCB 1.6; LID CLEARANCE TARGET ≥1.5; BATTERY–PCB CLEARANCE 3.0", "note"))

# Interface notes, tied to generated CAD and assembly envelopes.
notes_x = 1050
items.append(text(notes_x, 95, "FIRST-PROTOTYPE INTERFACE NOTES", "title", "start"))
notes = [
    "1. UNITS: mm. DO NOT SCALE DRAWING.",
    "2. ENCLOSURE: printed indoor instrument case; not sealed or IP rated.",
    "3. MATERIAL ASSUMPTION: PETG, 0.20 mm layers, 4 perimeters, 30% infill.",
    "4. FASTENERS: 4 x DIN 912 M3 x 30 plus 4 x DIN 934 M3 nuts.",
    "5. SERVICE OPENINGS: USB-C bottom; microSD right; J7 5.8 GHz right; J5 sub-GHz left.",
    "6. OLED: Adafruit PID 326, lid snap cradle, keyed 4-pin JST-SH harness.",
    "7. BATTERY: Adafruit PID 328 / LP785060 in 52 x 62 x 8.3 under-PCB bay.",
    "8. BUTTONS: RESET / BOOT / USER independent plungers; 0.65 mm nominal travel.",
    "9. CAD AUTHORITY: PCBA STEP, battery/display envelopes and generate_enclosure.py.",
    "10. CAD CHECKS ARE NOT PHYSICAL VALIDATION. Measure the first print and PCBA.",
]
for index, note in enumerate(notes):
    items.append(text(notes_x, 135 + index * 35, note, "note", "start"))

width, height = 1800, 1050
svg = f'''<?xml version="1.0" encoding="UTF-8"?>
<svg xmlns="http://www.w3.org/2000/svg" width="{width}" height="{height}" viewBox="0 0 {width} {height}">
  <defs>
    <marker id="arrow" markerWidth="8" markerHeight="8" refX="4" refY="4" orient="auto-start-reverse"><path d="M8,0 L0,4 L8,8 z" fill="#111"/></marker>
    <style>
      .object {{ fill:none; stroke:#111; stroke-width:2; }} .thick {{ stroke-width:3; }}
      .pcb {{ fill:#d8ead8; stroke:#176b3a; stroke-width:2; }}
      .battery {{ fill:#e8d7a7; stroke:#8a641d; stroke-width:2; }}
      .hole {{ fill:white; stroke:#111; stroke-width:2; }}
      .center {{ stroke:#777; stroke-width:1; stroke-dasharray:8 4; }}
      .extension {{ stroke:#555; stroke-width:1; }}
      .dimension {{ stroke:#111; stroke-width:1.5; marker-start:url(#arrow); marker-end:url(#arrow); }}
      text {{ font-family:Arial,Helvetica,sans-serif; fill:#111; }}
      .title {{ font-size:20px; font-weight:bold; }} .note {{ font-size:15px; }}
      .dimension-text {{ font-size:15px; font-weight:bold; paint-order:stroke; stroke:white; stroke-width:5px; }}
    </style>
  </defs>
  <rect width="100%" height="100%" fill="white"/>
  {''.join(items)}
  <rect x="1020" y="750" width="740" height="220" fill="none" stroke="#111" stroke-width="2"/>
  <text x="1040" y="790" class="title">SKYSWEEP32 REV C — MECHANICAL INTERFACE</text>
  <text x="1040" y="825" class="note">MATURITY: READY FOR FIRST PHYSICAL PROTOTYPE</text>
  <text x="1040" y="860" class="note">PRODUCTION VALIDATED: NO</text>
  <text x="1040" y="895" class="note">DRAWING: REV_C_MECHANICAL_DRAWING</text>
  <text x="1040" y="930" class="note">REVISION: C-PROTOTYPE</text>
  <text x="1570" y="930" class="note">SHEET 1 / 1</text>
</svg>
'''
OUT.parent.mkdir(parents=True, exist_ok=True)
OUT.write_text(svg, encoding="utf-8")
print(f"[PASS] wrote {OUT}")
