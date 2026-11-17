# SkySweep32 Hardware

## Canonical current design: Rev C

**Rev C status: READY FOR FIRST PHYSICAL PROTOTYPE — NOT PRODUCTION VALIDATED.**

[`rev_c/`](rev_c/) is the sole canonical hardware source in this repository. Its native KiCad 10 hierarchical schematic and four-layer PCB, fitted BOM, generated fabrication package, complete PCBA STEP assembly, co-designed FreeCAD enclosure, firmware pin contract, and validation evidence are documented in [`rev_c/README.md`](rev_c/README.md), with step-by-step prototype ordering and assembly instructions in [`rev_c/BUILD.md`](rev_c/BUILD.md).

Run the complete local hardware verification gate from the repository root:

```bash
python tools/hardware/verify.py
```

Passing this command establishes encoded ERC/DRC, schematic-to-PCB parity, CAD mechanical interference, manufacturing export, and firmware-build checks. It does not establish an assembled, bench-tested, field-tested, compliant, or production-validated device.

## Revision summary

| Revision | Directory | Status | Notes |
|---|---|---|---|
| **Rev C** | [`rev_c/`](rev_c/) | **Ready for first physical prototype** | Canonical passive RF monitor hardware and enclosure package |
| Rev B | *(retired in git history)* | Failed / unverified — do not order | Superseded prior to the Rev C baseline (`cd927b9`) |
| Rev A | *(retired in git history)* | Legacy unverified — do not order | Superseded carrier prototype; electrically and mechanically incompatible with Rev C |
