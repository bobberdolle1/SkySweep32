# Hardware revision history

SkySweep32 has one current physical hardware line: **Rev C** under
`hardware/rev_c/`. It is ready for a first physical prototype only.

## Rev A

Rev A was an early ESP32 DevKit-era concept. Its pin maps, binaries, and CAD are
not compatible with Rev C. Historical v0.6.1 release assets should not be used
on Rev C.

## Rev B

Rev B was publicly marked experimental/unverified after its machine evidence
contradicted earlier validation prose: stale ERC/DRC material showed unresolved
errors, warnings, collisions and roughly 375 DRC violations. Rev B was archived
rather than silently redefined. It is not a fabrication source, BOM, enclosure,
or firmware target.

## Rev C

Rev C is a clean passive-monitor redesign: exact manifest, native KiCad sources,
four-layer PCB, canonical firmware target, reproducible evidence, and an
assembly-aware enclosure. Passing CAD/build gates does not substitute for an
assembled board or measurements. See [current hardware](../en/hardware.md) and
[Prototype #1 checklist](../../hardware/rev_c/PROTOTYPE_VALIDATION_CHECKLIST.md).
