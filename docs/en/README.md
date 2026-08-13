# SkySweep32 documentation

SkySweep32 is an open-source **passive multi-band RF monitoring system**. Rev C
is its current physical implementation, not the product definition.

> **Current maturity: READY FOR FIRST PHYSICAL PROTOTYPE.** Rev C has CAD and
> build evidence, but no assembled board, RF characterization, compliance
> result, reliability result, or field result.

## Read in this order

1. [System overview and capability matrix](system.md)
2. [Get started](getting-started.md)
3. [Software and local web UI](software.md)
4. [Current Rev C hardware and enclosure](hardware.md)
5. [Validation and physical-test boundary](validation.md)
6. [Development and contribution](development.md)
7. [Roadmap](roadmap.md)

[Русская документация](../ru/README.md) · [Project website](https://bobberdolle1.github.io/SkySweep32/) · [Source repository](https://github.com/bobberdolle1/SkySweep32)

## Source-of-truth policy

| Fact | Authority | Derived material |
| --- | --- | --- |
| Rev C circuit and geometry | KiCad schematic/PCB plus manufacturer data | fabrication package, STEP, reports |
| Rev C fitted parts and pins | `hardware/rev_c/hardware_manifest.json` | generated firmware header, board definition, BOM |
| Firmware behavior | C++ source built as `esp32s3_rev_c_passive` | binary, local web UI |
| CAD/build evidence | reproducible tools and reports | human summaries |
| Product explanation | these EN/RU documents | website and README links |

A report, render, or successful compile is not proof of RF performance or a
physical prototype result.
