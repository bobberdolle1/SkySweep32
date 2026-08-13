# SkySweep32 Rev C.1 Detector

**Status: `NO-GO — NOT ORDERABLE — NOT PRODUCTION VALIDATED`.**

This is an isolated Rev C.1 review tree based on `master` commit `51e48ccca8b64efdce41373d26b4c51c016152e0`. It does not modify Rev C. The only native-PCB change currently embodied here is the ESP32 antenna board-edge notch derived from the existing all-layer antenna keepout. The required power re-layout, USB reroute, 5.x detector, video-confirmation circuit, SkyPort circuit, updated enclosure and regenerated production outputs are **not yet represented in native KiCad sources**.

Therefore this directory is evidence for an engineering **NO-GO**, not a fabrication package. Do not send it to a board house or assembler.

Reviewed plans:

- `REV_C1_PREFAB_REVIEW.md` — independent finding disposition and every critical-IC gate.
- `DETECTOR_ARCHITECTURE.md` — selected 5.x filter-bin/log-detector and analog-video approach.
- `STACKUP_AND_IMPEDANCE.md` — current USB failure and required fabrication contract.
- `POWER_BUDGET.md` — revised allocation and power-path risks.
- `SKYPORT.md` — connector, protection, power and discovery contract.
- `PROCUREMENT_AND_COST.md` — supplier risk policy and budgetary China costs.
- `FIRST_ARTICLE_BRINGUP.md` — ordered bring-up/stop conditions.
- `DIPLOMA_VALIDATION_PLAN.md` — reproducible evidence experiments.

Native CAD can become fabrication authority only after all documented blockers are designed in, independently checked in KiCad 10, and manufacturing/mechanical outputs are regenerated from that checked source.
