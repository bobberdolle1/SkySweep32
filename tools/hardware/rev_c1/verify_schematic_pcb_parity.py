#!/usr/bin/env python3
"""Check every fitted Rev C.1 PCB pad against its native schematic net.

This is a source-consistency check only. It cannot prove routing quality,
impedance, RF response, or that the planned C.1 circuits exist.
"""
from __future__ import annotations

import json
import re
import subprocess
import sys
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[3]
REV = ROOT / "hardware" / "rev_c1"
sys.path.insert(0, str(ROOT / "tools" / "hardware" / "rev_c"))
from tool_discovery import discover_kicad_cli  # noqa: E402
KICAD_PYTHON = discover_kicad_cli().with_name("python.exe")
SCHEMATIC = REV / "skysweep32_rev_c1.kicad_sch"
BOARD = REV / "skysweep32_rev_c1.kicad_pcb"
TOKEN = re.compile(r'\(|\)|"(?:[^"\\\\]|\\\\.)*"|[^\s()]+')


def atom(value: str) -> str:
    return value[1:-1].replace(r'\\"', '"') if value.startswith('"') else value


def parse(tokens: list[str], index: int = 0) -> tuple[list[object], int]:
    if tokens[index] != "(":
        raise ValueError("expected opening parenthesis")
    result: list[object] = []
    index += 1
    while tokens[index] != ")":
        if tokens[index] == "(":
            child, index = parse(tokens, index)
            result.append(child)
        else:
            result.append(tokens[index])
            index += 1
    return result, index + 1



def walk(item: object, name: str) -> list[list[object]]:
    matches: list[list[object]] = []
    if isinstance(item, list):
        if item and item[0] == name:
            matches.append(item)
        for child in item:
            matches.extend(walk(child, name))
    return matches

def children(item: list[object], name: str) -> list[list[object]]:
    return [child for child in item[1:] if isinstance(child, list) and child and child[0] == name]


def load(path: Path) -> list[object]:
    tokens = TOKEN.findall(path.read_text(encoding="utf-8"))
    tree, end = parse(tokens)
    if end != len(tokens):
        raise ValueError(f"unexpected trailing tokens in {path}")
    return tree


def schematic_nets(path: Path) -> dict[tuple[str, str], str]:
    nets: dict[tuple[str, str], str] = {}
    for net in walk(load(path), "net"):
        name = next(atom(str(item[1])) for item in children(net, "name"))
        for node in children(net, "node"):
            fields = {str(child[0]): atom(str(child[1])) for child in node[1:]
                      if isinstance(child, list) and len(child) == 2}
            nets[(fields["ref"], fields["pin"])] = name
    return nets


def board_nets(path: Path) -> dict[tuple[str, str], str]:
    command = (
        "import json,pcbnew,sys; "
        "board=pcbnew.LoadBoard(sys.argv[1]); "
        "print(json.dumps({f.GetReference()+'\\x1f'+p.GetNumber():p.GetNetname() "
        "for f in board.GetFootprints() for p in f.Pads() if p.GetNetname()}))"
    )
    result = subprocess.run([str(KICAD_PYTHON), "-c", command, str(path)],
                            cwd=ROOT, check=True, text=True, capture_output=True)
    return {tuple(key.split("\x1f", 1)): net for key, net in json.loads(result.stdout).items()}


def main() -> int:
    if not SCHEMATIC.is_file() or not BOARD.is_file():
        raise FileNotFoundError("Rev C.1 schematic or board is missing")
    with tempfile.TemporaryDirectory(prefix="skysweep32-c1-parity-") as temporary:
        exported = Path(temporary) / "rev_c1.net"
        subprocess.run([str(discover_kicad_cli()), "sch", "export", "netlist", "--output", str(exported), str(SCHEMATIC)],
                       cwd=ROOT, check=True)
        expected = schematic_nets(exported)
    actual = board_nets(BOARD)
    mismatches = [(key, expected.get(key), actual.get(key)) for key in sorted(expected.keys() | actual.keys())
                  if expected.get(key) != actual.get(key)]
    if mismatches:
        for (reference, pad), schematic_net, board_net in mismatches:
            print(f"[FAIL] {reference}.{pad}: schematic={schematic_net!r} pcb={board_net!r}")
        return 1
    print(f"[PASS] {len(actual)} fitted PCB pads match the native schematic netlist")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
