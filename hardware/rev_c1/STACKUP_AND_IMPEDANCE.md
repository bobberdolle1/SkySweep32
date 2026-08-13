# Rev C.1 stackup and controlled-impedance requirements

## Fabrication contract

- Four layers, 1.60 mm finished FR-4, ENIG. Proposed signal stack: F.Cu / **solid In1 GND** / In2 power / B.Cu.
- Rev C nominal source stack is 35 µm F.Cu, 0.18 mm FR-4 prepreg, 35 µm In1. Its dielectric constant and thickness are only design inputs, not a factory impedance guarantee.
- Board house must provide its actual pressed stackup, Er/Dk method, finished copper thickness, solder-mask stack and calculated geometry before fabrication. It must field-solve:
  - USB D+/D−: **90 Ω differential ±10%** on F.Cu referenced continuously to In1 GND.
  - J5/J7/J8 and 5.x detector interconnect: **50 Ω** controlled F.Cu transmission line referenced to In1 GND.
- Use a representative impedance coupon and provide the coupon result with the PCBs. Any supplier geometry change requires source review and DRC rerun.

## Current Rev C USB geometry — failed baseline, not a release report

| Segment | Observed Rev C geometry | Result |
|---|---|---|
| J1 → U2, D+ | F.Cu from USB-C, then two layer transitions and ~10.7 mm on In2 | Fails: In2 is POWER_PLANE, not a continuous adjacent ground reference. |
| J1 → U2, D− | Different F.Cu/inner routing topology with a ~6.75 mm In2 leg | Fails: topology and length differ from D+. |
| U2 → U1, D+ | Separate F.Cu/inner detour of roughly 26 mm | Fails: not a tightly coupled pair. |
| U2 → U1, D− | Separate F.Cu/inner detour of roughly 40 mm | Fails: not a tightly coupled pair; mismatch and plane-reference problem. |
| Nominal trace width | 0.25 mm | Inadequate evidence: a width alone cannot establish differential impedance. |

## Required Rev C.1 USB route

1. Place the USB ESD immediately after the USB-C receptacle, with the connector-side branches short and symmetric.
2. Route D+/D− as a single F.Cu differential pair receptacle → ESD → optional chip-side series footprints → ESP32 pins 19/20.
3. Use the fabricator-solved width/gap, same bends and same topology. Add only tightly coupled length correction if required; do not create stubs.
4. Keep unbroken In1 GND under the complete pair. No vias; if a design exception requires them, use matched pair vias and adjacent ground-return vias, then re-solve and review.
5. Keep pair away from switch nodes, inductors, RF feeds, antenna keepouts, SD clock and board-edge discontinuities. Stitch ground around the pair without breaking its controlled geometry.
6. Preserve the ESD device's low capacitance and validate the USB device link with enumeration/upload/reconnect tests on the first article.

No Rev C.1 supplier field-solve, coupon, DRC or manufactured-link result exists while this document carries `NO-GO` status.
