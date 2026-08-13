# Change summary

- Problem and decision:
- Linked issue/discussion:
- User-visible or engineering-contract impact:

## Scope

- [ ] Firmware / parser
- [ ] Rev C electrical, PCB, manufacturing, or enclosure source
- [ ] Documentation / GitHub Pages
- [ ] Tooling / CI
- [ ] Prototype measurement evidence

## Evidence

List the exact commands, real-hardware procedure, or CAD evidence run. State
unrun checks and why. Compilation does not demonstrate RF, mechanical, power,
compliance, or field performance.

```text
Command / procedure:
Result:
```

## Checklist

- [ ] I used the canonical Rev C manifest and sources where applicable.
- [ ] Generated pin-map/dashboard/CAD artifacts are regenerated or intentionally unchanged.
- [ ] Documentation and English/Russian maturity/capability claims agree.
- [ ] I updated all affected current callsites; I did not preserve an obsolete path without a reason.
- [ ] I added or updated a deterministic test only for an observable changed contract.
- [ ] I did not add active RF interference, transmitter identity-from-RSSI, or placeholder ML functionality.
- [ ] I have not claimed production validation without corresponding physical evidence.
