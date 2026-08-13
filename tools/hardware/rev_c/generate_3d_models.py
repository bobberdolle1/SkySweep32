#!/usr/bin/env python3
"""Generate explicitly conservative Rev C mechanical-envelope STEP models.

These are not cosmetic manufacturer models. Their dimensions come from the
part drawings recorded in hardware_manifest.json and intentionally bound all
material relevant to enclosure clearance.
"""

from __future__ import annotations

import json
from pathlib import Path

import FreeCAD
import Part

ROOT = Path(__file__).resolve().parents[3]
REV = ROOT / "hardware" / "rev_c"
MODELS = REV / "3dmodels"
MANIFEST = json.loads((REV / "hardware_manifest.json").read_text(encoding="utf-8"))


def box(dx: float, dy: float, dz: float, x: float, y: float, z: float = 0.0) -> Part.Shape:
    return Part.makeBox(dx, dy, dz, FreeCAD.Vector(x, y, z))


def export(name: str, shape: Part.Shape) -> None:
    path = MODELS / name
    document = FreeCAD.newDocument(f"Envelope_{path.stem}")
    feature = document.addObject("PartDesign::Feature", path.stem)
    feature.Label = path.stem
    feature.Shape = shape
    Part.export([feature], str(path))
    FreeCAD.closeDocument(document.Name)
    print(f"[OK] {path.relative_to(REV)} {shape.BoundBox.XLength:.2f} x "
          f"{shape.BoundBox.YLength:.2f} x {shape.BoundBox.ZLength:.2f} mm")


def generate() -> None:
    MODELS.mkdir(parents=True, exist_ok=True)

    # E28-2G4M12SX drawing: 15.0 x 17.8 x 2.85 mm IPEX module.
    export("Ebyte_E28_2G4M12SX_ENVELOPE.step", box(15.0, 17.8, 2.85, -7.5, -8.9))

    # E07 drawing: castellated 14 x 20 mm module, bounded to 2.4 mm high.
    export("Ebyte_E07_900M10S_ENVELOPE.step", box(14.0, 20.0, 2.4, -7.0, -10.0))

    # RX5808 V1.0 2010-09-02 drawing: 28 x 23 x 3 mm shielded module.
    export("RX5808_2012_12P_ENVELOPE.step", box(28.0, 23.0, 3.0, -14.0, -11.5))

    # SAM-M10Q-00B: 15.9 mm square antenna module, 6.3 mm maximum height.
    export("UBlox_SAM_M10Q_ENVELOPE.step", box(15.9, 15.9, 6.3, -7.95, -7.95))

    # CUI CMT-1203-SMT-TR bounding cylinder, including the 3.0 mm body height.
    export("CUI_CMT_1203_SMT_TR_ENVELOPE.step", Part.makeCylinder(6.0, 3.0))

    # Lid-mounted Adafruit PID 326. The board and display body are separated so
    # the enclosure can check the complete 29.2 x 26.7 x 6.2 mm envelope.
    oled = box(29.2, 26.7, 1.6, -14.6, -13.35)
    oled = oled.fuse(box(26.0, 15.2, 4.6, -13.0, -4.6, 1.6))
    export("Adafruit_OLED_PID326_ENVELOPE.step", oled)
    # Molex 104031-0811 maximum connector body plus a fully inserted microSD.
    # Body dimensions follow SD-104031-001; the card is an explicit service
    # envelope and extends from local y=-9.7 mm to y=+5.3 mm.
    microsd = box(12.0, 11.4, 1.42, -6.0, -5.7)
    microsd = microsd.fuse(box(11.0, 15.0, 1.0, -5.5, -9.7, 0.2))
    export("Molex_104031_0811_WITH_CARD_ENVELOPE.step", microsd)

    # Bourns MF-MSMF200-2 datasheet maximum 4.73 x 3.41 x 0.85 mm.
    export("Bourns_MF_MSMF200_2_ENVELOPE.step", box(4.73, 3.41, 0.85, -2.365, -1.705))

    # Bourns SRN6028 tolerance maximum 6.3 x 6.3 mm, 2.8 mm seated height.
    export("Bourns_SRN6028_3R9M_ENVELOPE.step", box(6.3, 6.3, 2.8, -3.15, -3.15))

    # Bourns SRN6028C package uses the same conservative 6.3 mm square,
    # 2.8 mm seated envelope as the qualified SRN6028 series land pattern.
    export("Bourns_SRN6028C_1R0Y_ENVELOPE.step", box(6.3, 6.3, 2.8, -3.15, -3.15))

    # Adafruit PID 328 / LP785060 protected pack, including published maximum
    # battery body envelope. Cable bend and plug are checked in enclosure CAD.
    export("Adafruit_PID328_LP785060_BATTERY_ENVELOPE.step", box(50.0, 60.0, 7.3, -25.0, -30.0))

    # TI BQ24074 RGT 16-pin VQFN: 3.0 mm nominal body, 1.0 mm maximum
    # seated height. The 3.1 mm XY envelope includes package tolerance.
    export("TI_BQ24074_RGT_ENVELOPE.step", box(3.1, 3.1, 1.0, -1.55, -1.55))

    # Analog Devices MAX17048 TDFN-8: 2.0 mm nominal body and 0.8 mm maximum
    # seated height; XY includes the package tolerance bound.
    export("ADI_MAX17048_TDFN8_ENVELOPE.step", box(2.1, 2.1, 0.8, -1.05, -1.05))

    # JST S2B-PH-SM4-TB(LF)(SN) right-angle board header. The envelope follows
    # the official 7.9 x 7.6 mm footprint/body drawing and 4.5 mm body height.
    export("JST_S2B_PH_SM4_TB_ENVELOPE.step", box(7.9, 7.6, 4.5, -3.95, -3.2))


if __name__ == "__main__":
    generate()
