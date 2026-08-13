#!/usr/bin/env python3
"""Render deterministic high-resolution enclosure inspection views headlessly."""

from __future__ import annotations

from pathlib import Path

import FreeCAD
import Part
import matplotlib

matplotlib.use("Agg")
import matplotlib.pyplot as plt
import numpy as np
from mpl_toolkits.mplot3d.art3d import Poly3DCollection

import generate_enclosure as cad

ROOT = Path(__file__).resolve().parents[3]
REV = ROOT / "hardware" / "rev_c"
PREVIEWS = REV / "previews"
LIGHT = np.array((0.4, -0.55, 0.75), dtype=float)
LIGHT /= np.linalg.norm(LIGHT)


def mesh(shape: Part.Shape, deflection: float) -> tuple[np.ndarray, np.ndarray]:
    vertices, facets = shape.tessellate(deflection)
    return (
        np.asarray([[point.x, point.y, point.z] for point in vertices], dtype=float),
        np.asarray(facets, dtype=int),
    )


def add_shape(axis, shape, color, alpha=1.0, deflection=0.6):
    # A STEP assembly arrives as one compound.  Matplotlib depth-sorts each
    # Poly3DCollection as a unit, so treating the whole PCBA as one collection
    # incorrectly paints its components behind the enclosure.  One collection
    # per solid preserves a legible installed/exploded assembly view.
    parts = list(shape.Solids)
    multipart = len(parts) > 1
    if not multipart:
        parts = [shape]
    parts.sort(key=lambda part: part.BoundBox.ZMax)
    base_rgb = np.asarray(color, dtype=float)
    collected_vertices = []
    for index, part in enumerate(parts):
        vertices, facets = mesh(part, deflection)
        if len(facets) == 0:
            continue
        triangles = vertices[facets]
        normals = np.cross(triangles[:, 1] - triangles[:, 0], triangles[:, 2] - triangles[:, 0])
        lengths = np.linalg.norm(normals, axis=1)
        lengths[lengths == 0] = 1.0
        normals /= lengths[:, None]
        brightness = np.clip(0.45 + 0.55 * np.abs(normals @ LIGHT), 0.0, 1.0)
        board_solid = (
            multipart
            and part.BoundBox.XLength > 100
            and part.BoundBox.YLength > 70
            and part.BoundBox.ZLength < 2
        )
        shade = 0.62 if board_solid else 1.0
        if multipart and not board_solid:
            shade = 0.82 + 0.30 * ((index * 7) % 13) / 12
        colors = np.empty((len(triangles), 4), dtype=float)
        colors[:, :3] = np.clip(base_rgb[None, :] * shade * brightness[:, None], 0.0, 1.0)
        colors[:, 3] = alpha * (0.42 if board_solid else 1.0)
        collection = Poly3DCollection(
            triangles,
            facecolors=colors,
            edgecolors="none",
            linewidths=0.0,
        )
        collection.set_rasterized(True)
        axis.add_collection3d(collection)
        collected_vertices.append(vertices)
    return np.vstack(collected_vertices) if collected_vertices else None


def render(name, objects, *, top=False, elev=27, azim=-55):
    figure = plt.figure(figsize=(12, 8), dpi=150, facecolor="#f2f4f8")
    axis = figure.add_subplot(111, projection="3d")
    all_vertices = []
    for shape, color, alpha, deflection in objects:
        vertices = add_shape(axis, shape, color, alpha, deflection)
        if vertices is not None:
            all_vertices.append(vertices)
    bounds = np.vstack(all_vertices)
    minimum = bounds.min(axis=0)
    maximum = bounds.max(axis=0)
    center = (minimum + maximum) / 2
    spans = maximum - minimum
    radius = max(spans[0], spans[1], spans[2] * 1.5) * 0.54
    axis.set_xlim(center[0] - radius, center[0] + radius)
    axis.set_ylim(center[1] - radius, center[1] + radius)
    axis.set_zlim(center[2] - radius * 0.55, center[2] + radius * 0.55)
    axis.set_box_aspect((1, 1, 0.55))
    axis.view_init(elev=90 if top else elev, azim=-90 if top else azim)
    axis.set_axis_off()
    figure.subplots_adjust(0, 0, 1, 1)
    figure.savefig(PREVIEWS / f"enclosure_{name}.png", dpi=150, facecolor=figure.get_facecolor())
    plt.close(figure)


def main():
    PREVIEWS.mkdir(parents=True, exist_ok=True)
    base = cad.build_base()
    lid = cad.build_lid()
    pcba = Part.read(str(cad.PCBA_STEP))
    oled = cad.placed(
        Part.read(str(cad.OLED_STEP)),
        cad.INTERFACES["oled"][0], -cad.INTERFACES["oled"][1], cad.OLED_Z,
    )
    battery = cad.placed(
        Part.read(str(cad.BATTERY_STEP)),
        cad.BATTERY_CENTER_X, -cad.BATTERY_CENTER_Y_TOP, cad.BATTERY_Z,
    )
    antenna = cad.box(
        cad.ANTENNA_W, cad.ANTENNA_D, cad.ANTENNA_H,
        cad.ANTENNA_X, cad.ANTENNA_Y, cad.ANTENNA_Z,
    )
    antenna_feed = (
        cad.ANTENNA_X, -cad.ANTENNA_CENTER_Y_TOP, cad.ANTENNA_Z,
    )
    antenna_cable, _ = cad.cable_path([
        (cad.IPEX_X, -cad.IPEX_Y_TOP, 4.5),
        (cad.IPEX_X, -cad.IPEX_Y_TOP, 11.5),
        (101.2, -23.6, 11.5),
        (95.0, -32.0, 11.5),
        (105.0, -38.0, 11.5),
        (115.0, -30.0, 11.5),
        antenna_feed,
    ])
    oled_harness, _ = cad.cable_path([
        (78.0, -89.0, 2.4),
        (78.0, -89.0, 9.5),
        (92.0, -89.0, 9.5),
        (97.0, -80.0, 9.5),
        (90.0, -70.0, 9.5),
        (78.0, -75.0, 9.5),
        (62.0, -75.0, 9.5),
        (62.0, -55.0, 9.5),
    ], radius=1.5)
    button = cad.build_button()
    buttons = [cad.placed(button, x, y, 0.0) for x, y in cad.BUTTONS]
    screw = cad.build_m3_socket_screw()
    nut = cad.build_m3_nut()
    screws = [cad.placed(screw, x, y, 0.0) for x, y in cad.HOLES]
    nuts = [cad.placed(nut, x, y, 0.0) for x, y in cad.HOLES]

    blue = (0.24, 0.38, 0.60)
    light = (0.78, 0.84, 0.92)
    green = (0.10, 0.48, 0.22)
    black = (0.08, 0.09, 0.11)
    cap = (0.08, 0.32, 0.78)
    battery_gray = (0.30, 0.32, 0.35)
    metal = (0.38, 0.40, 0.43)
    antenna_orange = (0.86, 0.42, 0.08)
    cable_gray = (0.12, 0.13, 0.14)

    render(
        "closed",
        [(base, blue, 1.0, 0.6), (battery, battery_gray, 1.0, 0.6),
         (pcba, green, 1.0, 0.8), (antenna, antenna_orange, 1.0, 0.4),
         (antenna_cable, cable_gray, 1.0, 0.3),
         (oled_harness, cable_gray, 1.0, 0.3),
         (oled, black, 1.0, 0.5), (lid, light, 0.32, 0.6)]
        + [(shape, cap, 1.0, 0.35) for shape in buttons]
        + [(shape, metal, 1.0, 0.35) for shape in screws + nuts],
    )
    render(
        "top",
        [(base, blue, 1.0, 0.6), (oled, black, 1.0, 0.5), (lid, light, 1.0, 0.6)]
        + [(shape, cap, 1.0, 0.35) for shape in buttons]
        + [(shape, metal, 1.0, 0.35) for shape in screws],
        top=True,
    )

    lifted_lid = cad.placed(lid, 0.0, 0.0, 30.0)
    lifted_oled = cad.placed(oled, 0.0, 0.0, 30.0)
    lifted_antenna = cad.placed(antenna, 0.0, 0.0, 30.0)
    lifted_antenna_cable, _ = cad.cable_path([
        (cad.IPEX_X, -cad.IPEX_Y_TOP, 4.5),
        (cad.IPEX_X, -cad.IPEX_Y_TOP, 12.0),
        (120.0, -38.0, 18.0),
        (100.0, -42.0, 24.0),
        (92.0, -30.0, 30.0),
        (100.0, -20.0, 36.0),
        (antenna_feed[0], antenna_feed[1], antenna_feed[2] + 30.0),
    ])
    lifted_oled_harness, _ = cad.cable_path([
        (78.0, -89.0, 2.4),
        (78.0, -89.0, 12.0),
        (90.0, -89.0, 15.0),
        (95.0, -78.0, 22.0),
        (80.0, -70.0, 30.0),
        (62.0, -70.0, 34.0),
        (62.0, -55.0, 39.5),
    ], radius=1.5)
    lifted_buttons = [cad.placed(shape, 0.0, 0.0, 30.0) for shape in buttons]
    render(
        "open",
        [(base, blue, 0.10, 0.6), (battery, battery_gray, 1.0, 0.6),
         (pcba, green, 1.0, 0.8),
         (lifted_antenna, antenna_orange, 1.0, 0.4),
         (lifted_antenna_cable, cable_gray, 1.0, 0.3),
         (lifted_oled_harness, cable_gray, 1.0, 0.3),
         (lifted_oled, black, 1.0, 0.5), (lifted_lid, light, 0.30, 0.6)]
        + [(shape, cap, 1.0, 0.35) for shape in lifted_buttons]
        + [(shape, metal, 1.0, 0.35) for shape in nuts],
    )

    exploded_pcba = cad.placed(pcba, 0.0, 0.0, 22.0)
    exploded_battery = cad.placed(battery, 0.0, 0.0, 20.0)
    exploded_oled = cad.placed(oled, 0.0, 0.0, 50.0)
    exploded_antenna = cad.placed(antenna, 0.0, 0.0, 50.0)
    exploded_lid = cad.placed(lid, 0.0, 0.0, 70.0)
    exploded_buttons = [cad.placed(shape, 0.0, 0.0, 90.0) for shape in buttons]
    exploded_screws = [cad.placed(shape, 0.0, 0.0, 105.0) for shape in screws]
    exploded_nuts = [cad.placed(shape, 0.0, 0.0, -10.0) for shape in nuts]
    render(
        "exploded",
        [(base, blue, 0.85, 0.6), (exploded_battery, battery_gray, 1.0, 0.6),
         (exploded_pcba, green, 1.0, 0.8),
         (exploded_antenna, antenna_orange, 1.0, 0.4),
         (exploded_oled, black, 1.0, 0.5), (exploded_lid, light, 0.30, 0.6)]
        + [(shape, cap, 1.0, 0.35) for shape in exploded_buttons]
        + [(shape, metal, 1.0, 0.35) for shape in exploded_screws + exploded_nuts],
    )

    cut = cad.box(180.0, 65.0, 100.0, -10.0, -115.0, -10.0)
    render(
        "cutaway",
        [(base.cut(cut), blue, 0.18, 0.6),
         (battery, battery_gray, 1.0, 0.6),
         (pcba, green, 1.0, 0.8),
         (antenna, antenna_orange, 1.0, 0.4),
         (antenna_cable, cable_gray, 1.0, 0.3),
         (oled_harness, cable_gray, 1.0, 0.3),
         (oled, black, 1.0, 0.5), (lid.cut(cut), light, 0.22, 0.6)],
    )


if __name__ == "__main__":
    main()
