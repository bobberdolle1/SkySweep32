#!/usr/bin/env python3
"""Generate and validate the Rev C enclosure around the exported PCBA STEP.

The KiCad assembly is the mechanical reference. Enclosure coordinates use the
KiCad STEP convention: the board occupies x=0..BOARD_W, y=-BOARD_H..0, with
its bottom surface at z=0. Board, hole, interface, battery, and case dimensions
come from hardware_manifest.json; no legacy 120 x 85 mm geometry is retained.
"""

from __future__ import annotations

import hashlib
import json
import math
from pathlib import Path

import FreeCAD
import Part

ROOT = Path(__file__).resolve().parents[3]
REV = ROOT / "hardware" / "rev_c"
OUT = REV / "enclosure"
MANIFEST = REV / "hardware_manifest.json"
MANIFEST_DATA = json.loads(MANIFEST.read_text(encoding="utf-8"))
MECHANICAL = MANIFEST_DATA["mechanical"]
BOARD_W, BOARD_H = MECHANICAL["board_dimensions_mm"]
PARAMETERS = MECHANICAL["enclosure_parameters_mm"]
INTERFACES = MECHANICAL["interface_centers_mm"]
EDGE_LRT = PARAMETERS["pcb_edge_clearance_left_right_top"]
EDGE_BOTTOM = PARAMETERS["pcb_edge_clearance_bottom"]
WALL = PARAMETERS["wall_thickness"]
LID_OVERHANG = PARAMETERS["lid_overhang"]
FLOOR = PARAMETERS["floor_thickness"]
LID_TOP = PARAMETERS["lid_top_thickness"]
BATTERY_FLOOR_CLEARANCE = PARAMETERS["battery_floor_clearance"]
BATTERY_BOARD_CLEARANCE = PARAMETERS["battery_to_board_clearance"]
SCREW_LENGTH = PARAMETERS["fastener_length"]
SCREW_TIP_RECESS = PARAMETERS["fastener_tip_recess"]
PCBA_STEP = REV / "skysweep32_rev_c_pcba.step"
OLED_STEP = REV / "3dmodels" / "Adafruit_OLED_PID326_ENVELOPE.step"
BATTERY_STEP = REV / "3dmodels" / "Adafruit_PID328_LP785060_BATTERY_ENVELOPE.step"

HOLES = tuple((x, -y) for x, y, _diameter in MECHANICAL["mounting_holes"])
BUTTONS = tuple(
    (INTERFACES[name][0], -INTERFACES[name][1])
    for name in ("reset", "boot", "user")
)

BASE_X = -(EDGE_LRT + WALL)
BASE_Y = -(BOARD_H + EDGE_BOTTOM + WALL)
BASE_W = BOARD_W + 2.0 * (EDGE_LRT + WALL)
BASE_D = BOARD_H + EDGE_LRT + EDGE_BOTTOM + 2.0 * WALL
CAVITY_X = -EDGE_LRT
CAVITY_Y = -(BOARD_H + EDGE_BOTTOM)
CAVITY_W = BOARD_W + 2.0 * EDGE_LRT
CAVITY_D = BOARD_H + EDGE_LRT + EDGE_BOTTOM
LID_X = BASE_X - LID_OVERHANG
LID_Y = BASE_Y - LID_OVERHANG
LID_W = BASE_W + 2.0 * LID_OVERHANG
LID_D = BASE_D + 2.0 * LID_OVERHANG

BATTERY_W, BATTERY_D, BATTERY_H = MANIFEST_DATA["major_parts"]["BAT1"]["maximum_envelope_mm"]
BATTERY_CENTER_X, BATTERY_CENTER_Y_TOP = MECHANICAL["battery_center_mm"]
BATTERY_X = BATTERY_CENTER_X - BATTERY_W / 2.0
BATTERY_Y = -(BATTERY_CENTER_Y_TOP + BATTERY_D / 2.0)
BATTERY_Z = -(BATTERY_BOARD_CLEARANCE + BATTERY_H)
CAVITY_BOTTOM_Z = BATTERY_Z - BATTERY_FLOOR_CLEARANCE
BASE_BOTTOM_Z = CAVITY_BOTTOM_Z - FLOOR
BASE_TOP_Z = 7.0
LID_CEILING_Z = PARAMETERS["pcb_to_lid_internal_height"]
LID_TOP_Z = LID_CEILING_Z + LID_TOP
FOOT_BOTTOM_Z = LID_CEILING_Z - SCREW_LENGTH - SCREW_TIP_RECESS
OLED_Z = LID_CEILING_Z - 3.2
ANTENNA_ITEM = next(item for item in MANIFEST_DATA["assembly_items"] if item["refs"] == "ANT2")
ANTENNA_W, ANTENNA_D, ANTENNA_H = ANTENNA_ITEM["antenna_element_mm"]
ANTENNA_CABLE_LENGTH = ANTENNA_ITEM["cable_length_mm"]
OLED_CABLE_ITEM = next(item for item in MANIFEST_DATA["assembly_items"] if item["refs"] == "W1")
OLED_CABLE_LENGTH = OLED_CABLE_ITEM["cable_length_mm"]
ANTENNA_CENTER_X, ANTENNA_CENTER_Y_TOP = INTERFACES["two4ghz_internal_antenna"]
IPEX_X, IPEX_Y_TOP = INTERFACES["two4ghz_ipex"]
ANTENNA_X = ANTENNA_CENTER_X - ANTENNA_W / 2.0
ANTENNA_Y = -(ANTENNA_CENTER_Y_TOP + ANTENNA_D / 2.0)
ANTENNA_Z = LID_CEILING_Z - ANTENNA_H - 0.2


def box(dx: float, dy: float, dz: float, x: float, y: float, z: float) -> Part.Shape:
    return Part.makeBox(dx, dy, dz, FreeCAD.Vector(x, y, z))


def cylinder(radius: float, height: float, x: float, y: float, z: float) -> Part.Shape:
    return Part.makeCylinder(radius, height, FreeCAD.Vector(x, y, z))

def cable_path(points: list[tuple[float, float, float]], radius: float = 0.65) -> tuple[Part.Shape, float]:
    """Build a conservative round coax envelope and return its centerline length."""
    segments = []
    length = 0.0
    for start, end in zip(points, points[1:]):
        origin = FreeCAD.Vector(*start)
        direction = FreeCAD.Vector(*(finish - begin for begin, finish in zip(start, end)))
        segment_length = direction.Length
        segments.append(Part.makeCylinder(radius, segment_length, origin, direction))
        length += segment_length
    cable = segments[0]
    for segment in segments[1:]:
        cable = cable.fuse(segment)
    return cable.removeSplitter(), length


def rounded_prism(
    x: float,
    y: float,
    width: float,
    depth: float,
    radius: float,
    z: float,
    height: float,
) -> Part.Shape:
    if radius <= 0 or radius * 2 >= min(width, depth):
        raise ValueError("invalid rounded-prism radius")
    shape = box(width - 2 * radius, depth, height, x + radius, y, z)
    shape = shape.fuse(box(width, depth - 2 * radius, height, x, y + radius, z))
    for cx, cy in (
        (x + radius, y + radius),
        (x + width - radius, y + radius),
        (x + width - radius, y + depth - radius),
        (x + radius, y + depth - radius),
    ):
        shape = shape.fuse(cylinder(radius, height, cx, cy, z))
    return shape


def hex_prism(across_flats: float, height: float, x: float, y: float, z: float) -> Part.Shape:
    radius = across_flats / math.sqrt(3.0)
    points = [
        FreeCAD.Vector(
            x + radius * math.cos(math.radians(30 + 60 * index)),
            y + radius * math.sin(math.radians(30 + 60 * index)),
            z,
        )
        for index in range(6)
    ]
    points.append(points[0])
    return Part.Face(Part.makePolygon(points)).extrude(FreeCAD.Vector(0, 0, height))


def build_base() -> Part.Shape:
    # The battery occupies a documented under-PCB bay. The cavity reaches below
    # the battery while corner pillars hold the PCB at z=0.
    outer = rounded_prism(
        BASE_X, BASE_Y, BASE_W, BASE_D, 6.0,
        BASE_BOTTOM_Z, BASE_TOP_Z - BASE_BOTTOM_Z,
    )
    cavity = rounded_prism(
        CAVITY_X, CAVITY_Y, CAVITY_W, CAVITY_D, 3.5,
        CAVITY_BOTTOM_Z, BASE_TOP_Z - CAVITY_BOTTOM_Z + 1.0,
    )
    base = outer.cut(cavity)

    usb_x, usb_y_top = INTERFACES["usb_c"]
    sd_x, sd_y_top = INTERFACES["microsd"]
    five_x, five_y_top = INTERFACES["five8ghz_sma"]
    sub_x, sub_y_top = INTERFACES["subghz_sma"]
    del usb_y_top, sd_x, five_x, sub_x
    # Openings cross the complete wall thickness and retain generous service
    # clearance for real plugs/cards rather than only the PCB receptacles.
    for opening in (
        box(13.0, EDGE_BOTTOM + WALL + 2.0, 7.5, usb_x - 6.5, BASE_Y - 1.0, -3.0),
        box(EDGE_LRT + WALL + 2.0, 16.0, 5.0, BOARD_W - 1.0, -sd_y_top - 8.0, -0.5),
        box(EDGE_LRT + WALL + 2.0, 14.0, 15.5, BOARD_W - 1.0, -five_y_top - 7.0, -4.0),
        box(EDGE_LRT + WALL + 2.0, 14.0, 11.5, BASE_X - 1.0, -sub_y_top - 7.0, -4.0),
    ):
        base = base.cut(opening)

    # Under-PCB battery cradle: 1.0 mm lateral clearance in a 52 x 62 mm bay,
    # open toward J6 so the genuine JST-PH lead can bend into its edge connector.
    bay_w, bay_d, _bay_h = MECHANICAL["battery_bay_internal_mm"]
    bay_x = BATTERY_CENTER_X - bay_w / 2.0
    bay_y = -(BATTERY_CENTER_Y_TOP + bay_d / 2.0)
    rail_top = BATTERY_Z + BATTERY_H + 0.5
    rail_height = rail_top - CAVITY_BOTTOM_Z
    for rail in (
        box(1.0, bay_d, rail_height, bay_x, bay_y, CAVITY_BOTTOM_Z),
        box(1.0, bay_d, rail_height, bay_x + bay_w - 1.0, bay_y, CAVITY_BOTTOM_Z),
        box(bay_w, 1.0, rail_height, bay_x, bay_y + bay_d - 1.0, CAVITY_BOTTOM_Z),
    ):
        base = base.fuse(rail)

    # PCB supports, through screws, DIN 934 M3 nut traps, and protective feet.
    foot_height = BASE_BOTTOM_Z - FOOT_BOTTOM_Z
    screw_hole_height = BASE_TOP_Z - FOOT_BOTTOM_Z + 1.0
    nut_trap_z = BASE_BOTTOM_Z + 0.1
    for x, y in HOLES:
        base = base.fuse(cylinder(4.2, -CAVITY_BOTTOM_Z, x, y, CAVITY_BOTTOM_Z))
        base = base.fuse(cylinder(5.0, foot_height, x, y, FOOT_BOTTOM_Z))
        base = base.cut(cylinder(1.7, screw_hole_height, x, y, FOOT_BOTTOM_Z))
        base = base.cut(hex_prism(5.8, 2.8, x, y, nut_trap_z))
    return base.removeSplitter()


def build_lid() -> Part.Shape:
    # Outside skirt fits over the base with 0.30 mm radial assembly clearance.
    top = rounded_prism(LID_X, LID_Y, LID_W, LID_D, 8.2, LID_CEILING_Z, LID_TOP)
    skirt_outer = rounded_prism(
        LID_X, LID_Y, LID_W, LID_D, 8.2, 6.8, LID_CEILING_Z - 6.8,
    )
    skirt_inner = rounded_prism(
        BASE_X - 0.3, BASE_Y - 0.3, BASE_W + 0.6, BASE_D + 0.6,
        6.3, 6.5, LID_CEILING_Z - 6.2,
    )
    lid = top.fuse(skirt_outer.cut(skirt_inner))

    five_y_top = INTERFACES["five8ghz_sma"][1]
    sub_y_top = INTERFACES["subghz_sma"][1]
    # Upper halves of both edge-launch SMA openings cross the lid skirt.
    lid = lid.cut(box(10.0, 14.0, LID_CEILING_Z - 6.0, BOARD_W - 1.0, -five_y_top - 7.0, 6.5))
    lid = lid.cut(box(5.0, 14.0, 5.5, LID_X - 1.0, -sub_y_top - 7.0, 6.5))

    # M3 compression posts clamp the PCB between lid and base without bending.
    for x, y in HOLES:
        lid = lid.fuse(cylinder(4.0, LID_CEILING_Z - 1.6, x, y, 1.6))
        lid = lid.cut(cylinder(1.7, LID_TOP_Z, x, y, 1.0))
        lid = lid.cut(cylinder(3.1, LID_TOP + 0.1, x, y, LID_CEILING_Z))

    oled_cx, oled_cy_top = INTERFACES["oled"]
    oled_cy = -oled_cy_top
    oled_w, oled_d, _oled_h = MANIFEST_DATA["major_parts"]["DISP1"]["maximum_envelope_mm"]
    oled_xmin = oled_cx - oled_w / 2.0
    oled_ymin = oled_cy - oled_d / 2.0
    # OLED snap cradle: 0.20 mm lateral and 0.10 mm vertical clearances.
    for rail in (
        box(0.8, 25.5, 3.5, oled_xmin - 1.0, oled_ymin + 0.6, LID_CEILING_Z - 3.5),
        box(0.8, 25.5, 3.5, oled_xmin + oled_w + 0.2, oled_ymin + 0.6, LID_CEILING_Z - 3.5),
        box(oled_w - 0.8, 0.6, 3.5, oled_xmin + 0.4, oled_ymin - 0.8, LID_CEILING_Z - 3.5),
        box(oled_w - 0.8, 0.6, 3.5, oled_xmin + 0.4, oled_ymin + oled_d + 0.2, LID_CEILING_Z - 3.5),
        box(1.2, 22.0, 0.2, oled_xmin - 0.2, oled_ymin + 2.35, LID_CEILING_Z - 3.5),
        box(1.2, 22.0, 0.2, oled_xmin + oled_w - 0.8, oled_ymin + 2.35, LID_CEILING_Z - 3.5),
        box(0.6, 16.0, 0.3, oled_xmin - 0.2, oled_ymin + 5.35, LID_CEILING_Z - 1.5),
        box(0.6, 16.0, 0.3, oled_xmin + oled_w - 0.4, oled_ymin + 5.35, LID_CEILING_Z - 1.5),
    ):
        lid = lid.fuse(rail)
    # Cable entry through the lower cradle rail; the harness bends toward J3.
    lid = lid.cut(box(6.0, 4.0, 4.0, oled_cx - 1.0, oled_ymin - 1.5, LID_CEILING_Z - 4.0))
    lid = lid.cut(box(26.4, 16.0, LID_TOP + 1.0, oled_cx - 13.2, oled_cy - 5.0, LID_CEILING_Z - 0.5))

    # Three independent plungers operate RESET, BOOT and USER. The status LED
    # remains visible through a dedicated 3.4 mm aperture.
    for x, y in BUTTONS:
        lid = lid.cut(cylinder(2.6, LID_TOP + 1.0, x, y, LID_CEILING_Z - 0.5))
    led_x, led_y_top = INTERFACES["status_led"]
    lid = lid.cut(cylinder(1.7, LID_TOP + 1.0, led_x, -led_y_top, LID_CEILING_Z - 0.5))
    return lid.removeSplitter()


def build_button() -> Part.Shape:
    stem = cylinder(1.2, LID_CEILING_Z - 2.8, 0.0, 0.0, 2.8)
    flange = cylinder(3.0, 0.8, 0.0, 0.0, LID_CEILING_Z - 0.8)
    cap = cylinder(2.3, 3.7, 0.0, 0.0, LID_CEILING_Z)
    return stem.fuse(flange).fuse(cap).removeSplitter()



def build_m3_socket_screw() -> Part.Shape:
    # DIN 912 M3 x 30: 3.0 mm head height, 5.5 mm nominal head diameter.
    shank = cylinder(1.5, SCREW_LENGTH, 0.0, 0.0, LID_CEILING_Z - SCREW_LENGTH)
    head = cylinder(2.75, 3.0, 0.0, 0.0, LID_CEILING_Z)
    return shank.fuse(head).removeSplitter()


def build_m3_nut() -> Part.Shape:
    # DIN 934 M3: 5.5 mm across flats, 2.4 mm nominal thickness.
    return hex_prism(5.5, 2.4, 0.0, 0.0, BASE_BOTTOM_Z + 0.3)

def placed(shape: Part.Shape, x: float, y: float, z: float) -> Part.Shape:
    copy = shape.copy()
    copy.translate(FreeCAD.Vector(x, y, z))
    return copy


def export_step(name: str, shapes: list[tuple[str, Part.Shape]]) -> Path:
    path = OUT / name
    document = FreeCAD.newDocument(f"Export_{path.stem}")
    objects = []
    for label, shape in shapes:
        obj = document.addObject("PartDesign::Feature", label)
        obj.Label = label
        obj.Shape = shape
        objects.append(obj)
    Part.export(objects, str(path))
    FreeCAD.closeDocument(document.Name)
    return path


def export_stl(name: str, shape: Part.Shape) -> Path:
    path = OUT / name
    shape.exportStl(str(path))
    return path


def intersection_volume(first: Part.Shape, second: Part.Shape) -> float:
    return first.common(second).Volume


def generate() -> dict[str, object]:
    OUT.mkdir(parents=True, exist_ok=True)
    for source in (PCBA_STEP, OLED_STEP, BATTERY_STEP):
        if not source.is_file():
            raise FileNotFoundError(f"assembly STEP missing: {source}")
    pcba = Part.read(str(PCBA_STEP))
    oled_cx, oled_cy_top = INTERFACES["oled"]
    oled = placed(Part.read(str(OLED_STEP)), oled_cx, -oled_cy_top, OLED_Z)
    antenna = box(
        ANTENNA_W, ANTENNA_D, ANTENNA_H,
        ANTENNA_X, ANTENNA_Y, ANTENNA_Z,
    )
    antenna_feed = (ANTENNA_X, -ANTENNA_CENTER_Y_TOP, ANTENNA_Z)
    closed_antenna_cable, closed_antenna_cable_length = cable_path([
        (IPEX_X, -IPEX_Y_TOP, 4.5),
        (IPEX_X, -IPEX_Y_TOP, 11.5),
        (101.2, -23.6, 11.5),
        (95.0, -32.0, 11.5),
        (105.0, -38.0, 11.5),
        (115.0, -30.0, 11.5),
        antenna_feed,
    ])
    open_antenna = placed(antenna, 0.0, 0.0, 30.0)
    open_antenna_cable, open_antenna_cable_length = cable_path([
        (IPEX_X, -IPEX_Y_TOP, 4.5),
        (IPEX_X, -IPEX_Y_TOP, 12.0),
        (120.0, -38.0, 18.0),
        (100.0, -42.0, 24.0),
        (92.0, -30.0, 30.0),
        (100.0, -20.0, 36.0),
        (antenna_feed[0], antenna_feed[1], antenna_feed[2] + 30.0),
    ])
    oled_harness, closed_oled_cable_length = cable_path([
        (78.0, -89.0, 2.4),
        (78.0, -89.0, 9.5),
        (92.0, -89.0, 9.5),
        (97.0, -80.0, 9.5),
        (90.0, -70.0, 9.5),
        (78.0, -75.0, 9.5),
        (62.0, -75.0, 9.5),
        (62.0, -55.0, 9.5),
    ], radius=1.5)
    open_oled_harness, open_oled_cable_length = cable_path([
        (78.0, -89.0, 2.4),
        (78.0, -89.0, 12.0),
        (90.0, -89.0, 15.0),
        (95.0, -78.0, 22.0),
        (80.0, -70.0, 30.0),
        (62.0, -70.0, 34.0),
        (62.0, -55.0, 39.5),
    ], radius=1.5)
    battery = placed(
        Part.read(str(BATTERY_STEP)),
        BATTERY_CENTER_X, -BATTERY_CENTER_Y_TOP, BATTERY_Z,
    )
    base = build_base()
    lid = build_lid()
    open_lid = placed(lid, 0.0, 0.0, 30.0)
    button_source = build_button()
    buttons = [placed(button_source, x, y, 0.0) for x, y in BUTTONS]
    pressed_buttons = [placed(button_source, x, y, -0.65) for x, y in BUTTONS]
    screw_source = build_m3_socket_screw()
    nut_source = build_m3_nut()
    screws = [placed(screw_source, x, y, 0.0) for x, y in HOLES]
    nuts = [placed(nut_source, x, y, 0.0) for x, y in HOLES]

    usb_x, _usb_y = INTERFACES["usb_c"]
    sd_x, sd_y_top = INTERFACES["microsd"]
    _five_x, five_y_top = INTERFACES["five8ghz_sma"]
    _sub_x, sub_y_top = INTERFACES["subghz_sma"]
    # Mating/service envelopes extend from outside the enclosure to their
    # receptacles. They are checked against both base and lid solids.
    battery_cable = box(22.0, 6.0, 2.5, 18.0, -84.0, -2.8).fuse(
        box(6.0, 25.0, 2.5, 15.0, -105.0, -2.8)
    ).fuse(box(10.0, 10.0, 6.0, 13.0, -105.0, -2.8))
    outer_right_x = BASE_X + BASE_W + 0.1
    five8ghz_antenna_swept = Part.makeCylinder(
        5.0, 72.0, FreeCAD.Vector(outer_right_x, -five_y_top, 0.0),
        FreeCAD.Vector(1.0, 0.0, 0.0),
    )
    subghz_antenna_swept = Part.makeCylinder(
        5.0, 142.0, FreeCAD.Vector(outer_right_x, -sub_y_top, 0.0),
        FreeCAD.Vector(1.0, 0.0, 0.0),
    )
    service_envelopes = {
        "usb_cable": box(13.0, 30.0, 7.5, usb_x - 6.5, BASE_Y - 10.0, -3.0),
        "microsd_card": box(31.0, 16.0, 5.0, sd_x - 6.0, -sd_y_top - 8.0, -0.5),
        "five8ghz_sma_plug": box(27.0, 14.0, 12.0, BOARD_W - 4.0, -five_y_top - 7.0, -4.0),
        "subghz_sma_plug": box(27.0, 14.0, 12.0, LID_X - 15.0, -sub_y_top - 7.0, -4.0),
        "five8ghz_antenna_swept": five8ghz_antenna_swept,
        "subghz_antenna_swept": subghz_antenna_swept,
        "battery_cable_and_plug": battery_cable,
        "oled_harness_bend": oled_harness,
    }

    manifest = MANIFEST_DATA
    freecad_version = ".".join(FreeCAD.Version()[:3])

    checks = {
        "design": manifest["design"],
        "freecad_version": freecad_version,
        "input_sha256": {
            "hardware_manifest": hashlib.sha256(MANIFEST.read_bytes()).hexdigest(),
            "pcba_step": hashlib.sha256(PCBA_STEP.read_bytes()).hexdigest(),
            "oled_step": hashlib.sha256(OLED_STEP.read_bytes()).hexdigest(),
            "battery_step": hashlib.sha256(BATTERY_STEP.read_bytes()).hexdigest(),
        },
        "base_bbox_mm": [
            base.BoundBox.XMin, base.BoundBox.XMax,
            base.BoundBox.YMin, base.BoundBox.YMax,
            base.BoundBox.ZMin, base.BoundBox.ZMax,
        ],
        "lid_bbox_mm": [
            lid.BoundBox.XMin, lid.BoundBox.XMax,
            lid.BoundBox.YMin, lid.BoundBox.YMax,
            lid.BoundBox.ZMin, lid.BoundBox.ZMax,
        ],
        "base_valid": base.isValid() and not base.isNull(),
        "lid_valid": lid.isValid() and not lid.isNull(),
        "button_valid": button_source.isValid() and not button_source.isNull(),
        "screw_valid": screw_source.isValid() and not screw_source.isNull(),
        "nut_valid": nut_source.isValid() and not nut_source.isNull(),
        "battery_valid": battery.isValid() and not battery.isNull(),
        "two4ghz_antenna_valid": antenna.isValid() and not antenna.isNull(),
        "two4ghz_cable_valid": closed_antenna_cable.isValid() and not closed_antenna_cable.isNull(),
        "oled_cable_valid": oled_harness.isValid() and not oled_harness.isNull(),
        "pcba_base_collision_mm3": intersection_volume(pcba, base),
        "pcba_lid_collision_mm3": intersection_volume(pcba, lid),
        "pcba_oled_collision_mm3": intersection_volume(pcba, oled),
        "oled_lid_collision_mm3": intersection_volume(oled, lid),
        "battery_base_collision_mm3": intersection_volume(battery, base),
        "battery_lid_collision_mm3": intersection_volume(battery, lid),
        "battery_pcba_collision_mm3": intersection_volume(battery, pcba),
        "two4ghz_antenna_base_collision_mm3": intersection_volume(antenna, base),
        "two4ghz_antenna_lid_collision_mm3": intersection_volume(antenna, lid),
        "two4ghz_antenna_pcba_collision_mm3": intersection_volume(antenna, pcba),
        "two4ghz_cable_base_collision_mm3": intersection_volume(closed_antenna_cable, base),
        "two4ghz_cable_lid_collision_mm3": intersection_volume(closed_antenna_cable, lid),
        "two4ghz_open_cable_base_collision_mm3": intersection_volume(open_antenna_cable, base),
        "two4ghz_open_cable_lid_collision_mm3": intersection_volume(open_antenna_cable, open_lid),
        "oled_cable_base_collision_mm3": intersection_volume(oled_harness, base),
        "oled_cable_lid_collision_mm3": intersection_volume(oled_harness, lid),
        "oled_open_cable_base_collision_mm3": intersection_volume(open_oled_harness, base),
        "oled_open_cable_lid_collision_mm3": intersection_volume(open_oled_harness, open_lid),
        "battery_fastener_collision_mm3": sum(
            intersection_volume(battery, screw) + intersection_volume(battery, nut)
            for screw, nut in zip(screws, nuts)
        ),
        "pcba_button_collision_mm3": sum(intersection_volume(pcba, item) for item in buttons),
        "pressed_button_switch_contact_mm3": [
            intersection_volume(pcba, item) for item in pressed_buttons
        ],
        "pressed_button_lid_collision_mm3": sum(
            intersection_volume(lid, item) for item in pressed_buttons
        ),
        "base_lid_collision_mm3": intersection_volume(base, lid),
        "fastener_case_collision_mm3": sum(
            intersection_volume(screw, base) + intersection_volume(screw, lid)
            + intersection_volume(nut, base) + intersection_volume(nut, lid)
            for screw, nut in zip(screws, nuts)
        ),
        "fastener_pcba_collision_mm3": sum(
            intersection_volume(screw, pcba) + intersection_volume(nut, pcba)
            for screw, nut in zip(screws, nuts)
        ),
        "service_envelope_collision_mm3": {
            name: intersection_volume(envelope, base) + intersection_volume(envelope, lid)
            for name, envelope in service_envelopes.items()
        },
        "pcba_bbox_mm": [
            pcba.BoundBox.XMin,
            pcba.BoundBox.XMax,
            pcba.BoundBox.YMin,
            pcba.BoundBox.YMax,
            pcba.BoundBox.ZMin,
            pcba.BoundBox.ZMax,
        ],
        "battery_bbox_mm": [
            battery.BoundBox.XMin,
            battery.BoundBox.XMax,
            battery.BoundBox.YMin,
            battery.BoundBox.YMax,
            battery.BoundBox.ZMin,
            battery.BoundBox.ZMax,
        ],
        "two4ghz_antenna_bbox_mm": [
            antenna.BoundBox.XMin,
            antenna.BoundBox.XMax,
            antenna.BoundBox.YMin,
            antenna.BoundBox.YMax,
            antenna.BoundBox.ZMin,
            antenna.BoundBox.ZMax,
        ],
        "two4ghz_cable_centerline_length_mm": {
            "closed": closed_antenna_cable_length,
            "lid_open_30mm": open_antenna_cable_length,
            "available": ANTENNA_CABLE_LENGTH,
            "closed_service_slack": ANTENNA_CABLE_LENGTH - closed_antenna_cable_length,
            "lid_open_30mm_service_slack": ANTENNA_CABLE_LENGTH - open_antenna_cable_length,
        },
        "oled_cable_centerline_length_mm": {
            "closed": closed_oled_cable_length,
            "lid_open_30mm": open_oled_cable_length,
            "available": OLED_CABLE_LENGTH,
            "closed_service_slack": OLED_CABLE_LENGTH - closed_oled_cable_length,
            "lid_open_30mm_service_slack": OLED_CABLE_LENGTH - open_oled_cable_length,
        },
        "lid_ceiling_clearance_mm": LID_CEILING_Z - pcba.BoundBox.ZMax,
        "battery_to_pcba_clearance_mm": battery.distToShape(pcba)[0],
        "battery_floor_clearance_mm": battery.BoundBox.ZMin - CAVITY_BOTTOM_Z,
        "board_edge_clearance_mm": {
            "left_right_top": EDGE_LRT,
            "bottom_service_edge": EDGE_BOTTOM,
        },
        "closed_outer_dimensions_mm": [
            LID_W, LID_D, LID_TOP_Z - FOOT_BOTTOM_Z,
        ],
        "base_lid_fit_clearance_mm": 0.3,
        "oled_lateral_clearance_mm": 0.2,
        "oled_vertical_clearance_mm": 0.1,
        "button_press_to_switch_mm": 0.65,
        "fastener_specification": {
            "screw": f"DIN 912 M3 x {SCREW_LENGTH:g}",
            "screw_head_clearance_mm": 0.35,
            "nut": "DIN 934 M3",
            "nut_across_flats_clearance_mm": 0.3,
        },
    }

    tolerance = 1e-4
    failures = []
    for key in (
        "base_valid", "lid_valid", "button_valid", "screw_valid", "nut_valid",
        "battery_valid", "two4ghz_antenna_valid", "two4ghz_cable_valid",
        "oled_cable_valid",
    ):
        if not checks[key]:
            failures.append(key)
    for key in (
        "pcba_base_collision_mm3",
        "pcba_lid_collision_mm3",
        "pcba_oled_collision_mm3",
        "oled_lid_collision_mm3",
        "pcba_button_collision_mm3",
        "pressed_button_lid_collision_mm3",
        "base_lid_collision_mm3",
        "battery_base_collision_mm3",
        "battery_lid_collision_mm3",
        "battery_pcba_collision_mm3",
        "battery_fastener_collision_mm3",
        "fastener_case_collision_mm3",
        "fastener_pcba_collision_mm3",
        "two4ghz_antenna_base_collision_mm3",
        "two4ghz_antenna_lid_collision_mm3",
        "two4ghz_antenna_pcba_collision_mm3",
        "two4ghz_cable_base_collision_mm3",
        "two4ghz_cable_lid_collision_mm3",
        "two4ghz_open_cable_base_collision_mm3",
        "two4ghz_open_cable_lid_collision_mm3",
        "oled_cable_base_collision_mm3",
        "oled_cable_lid_collision_mm3",
        "oled_open_cable_base_collision_mm3",
        "oled_open_cable_lid_collision_mm3",
    ):
        if float(checks[key]) > tolerance:
            failures.append(key)
    for key, volume in checks["service_envelope_collision_mm3"].items():
        if float(volume) > tolerance:
            failures.append(f"service:{key}")
    if any(float(volume) <= tolerance for volume in checks["pressed_button_switch_contact_mm3"]):
        failures.append("pressed_button_switch_contact_mm3")
    if float(checks["lid_ceiling_clearance_mm"]) < 1.5:
        failures.append("lid_ceiling_clearance_mm")
    if float(checks["battery_to_pcba_clearance_mm"]) < BATTERY_BOARD_CLEARANCE - tolerance:
        failures.append("battery_to_pcba_clearance_mm")
    if float(checks["battery_floor_clearance_mm"]) < BATTERY_FLOOR_CLEARANCE - tolerance:
        failures.append("battery_floor_clearance_mm")
    for name, lengths, available in (
        ("two4ghz_cable_centerline_length_mm",
         (closed_antenna_cable_length, open_antenna_cable_length), ANTENNA_CABLE_LENGTH),
        ("oled_cable_centerline_length_mm",
         (closed_oled_cable_length, open_oled_cable_length), OLED_CABLE_LENGTH),
    ):
        service_slack = [available - length for length in lengths]
        if any(slack < 5.0 or slack > 15.0 for slack in service_slack):
            failures.append(name)
    checks["status"] = "PASS" if not failures else "FAIL"
    checks["failures"] = failures

    export_step("skysweep32_rev_c_base.step", [("Base", base)])
    export_step("skysweep32_rev_c_lid.step", [("Lid", lid)])
    export_step("skysweep32_rev_c_button.step", [("Button", button_source)])
    export_stl("skysweep32_rev_c_base.stl", base)
    export_stl("skysweep32_rev_c_lid.stl", lid)
    export_stl("skysweep32_rev_c_button.stl", button_source)
    export_step(
        "skysweep32_rev_c_service_envelopes.step",
        [(name, shape) for name, shape in service_envelopes.items()],
    )

    closed = [
        ("Base", base), ("Battery", battery), ("BatteryCableEnvelope", battery_cable),
        ("PCBA", pcba), ("Two4GHzAntenna", antenna),
        ("Two4GHzAntennaCableEnvelope", closed_antenna_cable),
        ("OLEDHarnessEnvelope", oled_harness), ("OLED", oled), ("Lid", lid),
    ]
    closed += [(f"Button_{index}", shape) for index, shape in enumerate(buttons, 1)]
    closed += [(f"Screw_{index}", shape) for index, shape in enumerate(screws, 1)]
    closed += [(f"Nut_{index}", shape) for index, shape in enumerate(nuts, 1)]
    export_step("skysweep32_rev_c_closed_assembly.step", closed)

    open_oled = placed(oled, 0.0, 0.0, 30.0)
    open_buttons = [placed(shape, 0.0, 0.0, 30.0) for shape in buttons]
    opened = [
        ("Base", base), ("Battery", battery), ("PCBA", pcba),
        ("Two4GHzAntennaCableEnvelope", open_antenna_cable),
        ("OLEDHarnessEnvelope", open_oled_harness),
        ("Two4GHzAntenna_in_lid", open_antenna),
        ("OLED_in_lid", open_oled), ("Lid", open_lid),
    ]
    opened += [(f"Button_{index}", shape) for index, shape in enumerate(open_buttons, 1)]
    opened += [(f"Nut_{index}", shape) for index, shape in enumerate(nuts, 1)]
    export_step("skysweep32_rev_c_open_assembly.step", opened)

    exploded_battery = placed(battery, 0.0, 0.0, 8.0)
    exploded_pcba = placed(pcba, 0.0, 0.0, 28.0)
    exploded_oled = placed(oled, 0.0, 0.0, 52.0)
    exploded_antenna = placed(antenna, 0.0, 0.0, 52.0)
    exploded_lid = placed(lid, 0.0, 0.0, 68.0)
    exploded_buttons = [placed(shape, 0.0, 0.0, 86.0) for shape in buttons]
    exploded = [
        ("Base", base), ("Battery", exploded_battery), ("PCBA", exploded_pcba),
        ("Two4GHzAntenna", exploded_antenna),
        ("OLED", exploded_oled), ("Lid", exploded_lid),
    ]
    exploded += [(f"Button_{index}", shape) for index, shape in enumerate(exploded_buttons, 1)]
    exploded_screws = [placed(shape, 0.0, 0.0, 100.0) for shape in screws]
    exploded_nuts = [placed(shape, 0.0, 0.0, -12.0) for shape in nuts]
    exploded += [(f"Screw_{index}", shape) for index, shape in enumerate(exploded_screws, 1)]
    exploded += [(f"Nut_{index}", shape) for index, shape in enumerate(exploded_nuts, 1)]
    export_step("skysweep32_rev_c_exploded.step", exploded)

    cut_tool = box(BOARD_W + 20.0, BOARD_H / 2.0 + 10.0, 100.0, -10.0, -BOARD_H / 2.0, -30.0)
    cutaway = [
        ("Base_section", base.cut(cut_tool)),
        ("Battery_section", battery.cut(cut_tool)),
        ("PCBA_section", pcba.cut(cut_tool)),
        ("OLED_section", oled.cut(cut_tool)),
        ("Two4GHzAntenna_section", antenna.cut(cut_tool)),
        ("Two4GHzAntennaCableEnvelope_section", closed_antenna_cable.cut(cut_tool)),
        ("Lid_section", lid.cut(cut_tool)),
    ]
    export_step("skysweep32_rev_c_cutaway.step", cutaway)

    report = OUT / "mechanical_validation.json"
    report.write_text(json.dumps(checks, indent=2) + "\n", encoding="utf-8")
    print(json.dumps(checks, indent=2))
    if failures:
        raise SystemExit(f"mechanical validation failed: {', '.join(failures)}")
    return checks


if __name__ == "__main__":
    generate()
