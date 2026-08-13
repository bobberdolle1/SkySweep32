#!/usr/bin/env python3
"""Prove that the Rev C hierarchy preserves the reviewed pin-to-net contract."""

from __future__ import annotations

import argparse
import re
import subprocess
import tempfile
import xml.etree.ElementTree as ET
from pathlib import Path

from tool_discovery import discover_kicad_cli

ROOT = Path(__file__).resolve().parents[3]
REV = ROOT / "hardware" / "rev_c"
SCHEMATIC = REV / "skysweep32_rev_c.kicad_sch"
# This XML export was made immediately before the hierarchy-only refactor. It is
# a review baseline, not a design authority or a manufacturing input.
BASELINE = REV / "skysweep32_rev_c.net"
TOKEN = re.compile(r'\(|\)|"(?:[^"\\\\]|\\\\.)*"|[^\s()]+')


def atom(value: str) -> str:
    return value[1:-1].replace(r'\\"', '"') if value.startswith('"') else value


def parse_sexp(tokens: list[str], index: int = 0) -> tuple[list[object], int]:
    if tokens[index] != "(":
        raise ValueError("expected opening parenthesis")
    index += 1
    parsed: list[object] = []
    while tokens[index] != ")":
        if tokens[index] == "(":
            child, index = parse_sexp(tokens, index)
            parsed.append(child)
        else:
            parsed.append(tokens[index])
            index += 1
    return parsed, index + 1


def fields(item: list[object]) -> dict[str, str]:
    return {
        str(child[0]): atom(str(child[1]))
        for child in item[1:]
        if isinstance(child, list) and len(child) == 2 and not isinstance(child[1], list)
    }


def walk(item: object, head: str) -> list[list[object]]:
    matches: list[list[object]] = []
    if isinstance(item, list):
        if item and item[0] == head:
            matches.append(item)
        for child in item:
            matches.extend(walk(child, head))
    return matches


def baseline_contract(path: Path) -> dict[str, frozenset[tuple[str, str]]]:
    root = ET.parse(path).getroot()
    return {
        net.attrib["name"].removeprefix("/"): frozenset(
            (node.attrib["ref"], node.attrib["pin"]) for node in net.findall("node")
        )
        for net in root.findall("./nets/net")
    }


def generated_contract(path: Path) -> dict[str, frozenset[tuple[str, str]]]:
    tree, remainder = parse_sexp(TOKEN.findall(path.read_text(encoding="utf-8")))
    if remainder != len(TOKEN.findall(path.read_text(encoding="utf-8"))):
        raise ValueError("unexpected trailing netlist tokens")
    contract: dict[str, frozenset[tuple[str, str]]] = {}
    for net in walk(tree, "net"):
        properties = fields(net)
        if "name" not in properties:
            continue
        nodes = []
        for node in (child for child in net[1:] if isinstance(child, list) and child and child[0] == "node"):
            node_fields = fields(node)
            nodes.append((node_fields["ref"], node_fields["pin"]))
        contract[properties["name"]] = frozenset(nodes)
    return contract


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--baseline", type=Path, default=BASELINE)
    args = parser.parse_args()
    if not SCHEMATIC.is_file() or not args.baseline.is_file():
        raise FileNotFoundError("canonical schematic or hierarchy baseline is missing")

    with tempfile.TemporaryDirectory(prefix="skysweep32-netlist-") as temp:
        exported = Path(temp) / "hierarchy.net"
        subprocess.run(
            [str(discover_kicad_cli()), "sch", "export", "netlist", "--output", str(exported), str(SCHEMATIC)],
            check=True,
            cwd=ROOT,
        )
        before = baseline_contract(args.baseline)
        after = generated_contract(exported)

    differences = {
        net: (sorted(before.get(net, ())), sorted(after.get(net, ())))
        for net in sorted(before.keys() | after.keys())
        if before.get(net) != after.get(net)
    }
    if differences:
        for net, (old, new) in differences.items():
            print(f"[FAIL] {net}: baseline={old} hierarchy={new}")
        return 1
    print(f"[PASS] {len(after)} named nets and every connected reference/pin match the pre-hierarchy baseline")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
