#!/usr/bin/env python3
"""Generate the exact Rev C.1 detector, video, and load-switch symbols."""
from __future__ import annotations

from pathlib import Path

ROOT = Path(__file__).resolve().parents[3]
OUTPUT = ROOT / "hardware" / "rev_c1" / "SkySweep32RevC1.kicad_sym"

FONT = """(effects (font (size 1.27 1.27)))"""


def prop(name: str, value: str, x: float, y: float, *, hidden: bool = False) -> str:
    hide = " (hide yes)" if hidden else ""
    return f'''    (property "{name}" "{value}" (at {x} {y} 0) (show_name no) (do_not_autoplace no){hide} {FONT})'''


def pin(kind: str, number: str, name: str, x: float, y: float, angle: int) -> str:
    return f'''      (pin {kind} line (at {x} {y} {angle}) (length 2.54)
        (name "{name}" {FONT})
        (number "{number}" {FONT})
      )'''


def symbol(
    name: str,
    description: str,
    datasheet: str,
    pins: list[str],
    *,
    width: float = 10.16,
    top: float = 10.16,
    bottom: float = -10.16,
) -> str:
    return f'''  (symbol "{name}"
    (exclude_from_sim no)
    (in_bom yes)
    (on_board yes)
    (in_pos_files yes)
    (duplicate_pin_numbers_are_jumpers no)
{prop("Reference", "U", -width + 1.27, top + 2.54)}
{prop("Value", name, width - 1.27, top + 2.54)}
{prop("Footprint", "", 0, bottom - 5.08, hidden=True)}
{prop("Datasheet", datasheet, 0, bottom - 7.62, hidden=True)}
{prop("Description", description, 0, bottom - 10.16, hidden=True)}
    (symbol "{name}_0_1"
      (rectangle (start {-width} {top}) (end {width} {bottom})
        (stroke (width 0.254) (type default))
        (fill (type background))
      )
    )
    (symbol "{name}_1_1"
{chr(10).join(pins)}
    )
    (embedded_fonts no)
  )'''


def main() -> int:
    symbols: list[str] = []

    symbols.append(symbol(
        "AD8318",
        "1 MHz to 8 GHz logarithmic RF detector/controller, 16-pin LFCSP",
        "https://www.analog.com/media/en/technical-documentation/data-sheets/AD8318.pdf",
        [
            pin("power_in", "1", "CMIP", -5.08, -12.7, 90),
            pin("power_in", "2", "CMIP", -2.54, -12.7, 90),
            pin("power_in", "3", "VPSI", -5.08, 12.7, 270),
            pin("power_in", "4", "VPSI", -2.54, 12.7, 270),
            pin("passive", "5", "CLPF", -12.7, -7.62, 0),
            pin("output", "6", "VOUT", 12.7, -5.08, 180),
            pin("input", "7", "VSET", 12.7, -2.54, 180),
            pin("power_in", "8", "CMOP", 0, -12.7, 90),
            pin("power_in", "9", "VPSO", 0, 12.7, 270),
            pin("passive", "10", "TADJ", -12.7, -5.08, 0),
            pin("power_in", "11", "CMIP", 2.54, -12.7, 90),
            pin("power_in", "12", "CMIP", 5.08, -12.7, 90),
            pin("output", "13", "TEMP", -12.7, 0, 0),
            pin("input", "14", "INHI", -12.7, 5.08, 0),
            pin("input", "15", "INLO", -12.7, 2.54, 0),
            pin("input", "16", "ENBL", -12.7, -2.54, 0),
            pin("power_in", "17", "EP", 5.08, -12.7, 90),
        ],
    ))

    symbols.append(symbol(
        "PE42427",
        "5 MHz to 6 GHz reflective SPDT RF switch, 12-pin 2 mm QFN",
        "https://psemi.com/wp-content/uploads/pdf/datasheets/pe42427ds.pdf",
        [
            pin("power_in", "1", "GND", -5.08, -10.16, 90),
            pin("bidirectional", "2", "RF2", -12.7, 5.08, 0),
            pin("power_in", "3", "GND", -2.54, -10.16, 90),
            pin("power_in", "4", "GND/NC", 0, -10.16, 90),
            pin("bidirectional", "5", "RFC", 12.7, 0, 180),
            pin("power_in", "6", "GND", 2.54, -10.16, 90),
            pin("power_in", "7", "GND", 5.08, -10.16, 90),
            pin("bidirectional", "8", "RF1", -12.7, 2.54, 0),
            pin("power_in", "9", "DGND", 7.62, -10.16, 90),
            pin("input", "10", "V1", -12.7, -2.54, 0),
            pin("input", "11", "LS", -12.7, -5.08, 0),
            pin("power_in", "12", "VDD", 0, 12.7, 270),
            pin("power_in", "13", "EP", 7.62, -10.16, 90),
        ],
    ))

    symbols.append(symbol(
        "OPA360",
        "3 V fixed-gain 6 dB video amplifier with reconstruction filter",
        "https://www.ti.com/lit/ds/symlink/opa360.pdf",
        [
            pin("input", "1", "+IN", -10.16, 2.54, 0),
            pin("power_in", "2", "GND", -2.54, -7.62, 90),
            pin("passive", "3", "SAG", 10.16, -2.54, 180),
            pin("output", "4", "OUT", 10.16, 2.54, 180),
            pin("input", "5", "ENABLE", -10.16, -2.54, 0),
            pin("power_in", "6", "V+", 0, 7.62, 270),
        ],
        width=7.62,
        top=5.08,
        bottom=-5.08,
    ))

    symbols.append(symbol(
        "TPS2553",
        "Active-high precision adjustable current-limited load switch",
        "https://www.ti.com/lit/ds/symlink/tps2553.pdf",
        [
            pin("power_in", "1", "IN", -10.16, 2.54, 0),
            pin("power_in", "2", "GND", -2.54, -7.62, 90),
            pin("input", "3", "EN", -10.16, -2.54, 0),
            pin("open_collector", "4", "FAULT", 10.16, -2.54, 180),
            pin("passive", "5", "ILIM", 2.54, -7.62, 90),
            pin("power_out", "6", "OUT", 10.16, 2.54, 180),
        ],
        width=7.62,
        top=5.08,
        bottom=-5.08,
    ))

    symbols.append(symbol(
        "RF_BPF_4P",
        "Four-terminal 50-ohm ceramic band-pass filter",
        "~",
        [
            pin("power_in", "1", "GND", -2.54, -7.62, 90),
            pin("passive", "2", "RF", -10.16, 0, 0),
            pin("power_in", "3", "GND", 2.54, -7.62, 90),
            pin("passive", "4", "RF", 10.16, 0, 180),
        ],
        width=7.62,
        top=3.81,
        bottom=-3.81,
    ))

    text = "(kicad_symbol_lib\n  (version 20231120)\n  (generator kicad_symbol_editor)\n  (generator_version 10.0)\n" + "\n".join(symbols) + "\n)\n"
    OUTPUT.write_text(text, encoding="utf-8")
    print(f"Wrote {len(symbols)} symbols to {OUTPUT}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
