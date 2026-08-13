# Rev C.1 first-article bring-up plan

**Applies only after Rev C.1 is a released, current-source manufacturing package. It must not be applied to Rev C Gerbers.** Log meter, supply, oscilloscope/VNA/source settings, photos, serial logs and raw observations with each step.

## Stop conditions — immediate power-off and rework review

- Bare-board short or unexpected continuity between GND and VBUS, `VSYS_BAT`, `SYS_5V` or `3V3`.
- At 5.00 V / 100 mA current limit: current limit trips, any IC warms rapidly, or either rail is outside ±5%.
- Any USB VBUS above 5.5 V, incorrect Type-C CC behavior, or D+/D− short to a power net.
- TPS61232/AP63203 hiccup, audible inductor noise under a specified load, excessive temperature, or ripple/droop outside component limits.
- 5.x selected-path S11/insertion-loss failure, unstable detector output, wrong PE42427 state, or RF feed leakage.
- RX5808 lot does not pass pin continuity, dimensions, 5 V current, channel/RSSI/video checks.
- Video output is not properly 75 Ω source-terminated or sync logic produces false lock without a valid source.
- Any battery charging abnormality, pack/connector heating, polarity anomaly or protection-board trip.

## Ordered procedure

1. **Inspection before power.** No battery, antennas, OLED, SD card or SkyPort module. Verify board identity/revision, panel breakouts, solder paste, USB-C, BQ24074, TPS61232, AP63203, MAX17048, all inductors, ESD parts, ground vias, antenna notch, 5.x RF island, RX5808 pads and all connector pin 1 markers. Measure rails-to-ground resistance.
2. **Current-limited USB rails.** Apply 5.00 V at 100 mA. Verify VBUS protection, BQ24074 system node, boost enable state, `SYS_5V`, `3V3`, and no thermal excursion. Raise limit only after the rail values and current are logged.
3. **Power-loop characterization.** Exercise staged electronic loads at the design nominal and peak budgets. Observe switch nodes with a spring-ground probe, input/output ripple, start-up sequence, current, temperature and recovery. Repeat from USB and battery. Perform charge plus representative load; document charge-current derating.
4. **Native USB.** Fit neither RF antenna. Test two compliant 5 V sources and two cables in both orientations. Confirm enumerate, upload and reconnect ten times. Verify D+/D− geometry against the released supplier stackup/coupon report before accepting the board.
5. **Firmware and controls.** Flash the exact Rev C.1 image. Verify BOOT/RESET, status LED, user button, buzzer, I²C bus and no watchdog/reset loops.
6. **GNSS, microSD and display.** Fit one subsystem at a time: GNSS NMEA/PPS then SD read/write/card detect then OLED. Confirm the physical RF antenna cavity remains clear of cables and battery.
7. **855–925 MHz / 2.4 GHz.** Fit correct regional 900 MHz antenna and 2.4 GHz antenna. Use conducted or controlled radiated sources to record receiver response, sweep timing, repeatability and self-interference with Wi-Fi/BLE disabled/enabled.
8. **5.x detector.** Fit J8 antenna only after conducted check. VNA the two selected filter paths and switching isolation. Sweep a calibrated source through 5.10–6.00 GHz; record both filter-bin response curves, output ADC transfer, burst response, dwell/scan schedule and cross-bin leakage. Calibrate no claim beyond the measured response.
9. **RX5808 and analog video.** Inspect the exact lot before fit. Measure 5 V current; verify all eight channels (5645–5945 MHz), RSSI monotonicity and RSSI noise floor. Apply known PAL and NTSC analog FPV sources. Verify 75 Ω VIDEO OUT amplitude/load and sync qualification. Confirm no lock on RF energy/noise without composite sync.
10. **SkyPort.** Connect only a passive descriptor fixture first. Verify module ID CRC/limits, disabled boot state, 5 V and 3.3 V current limits, ESD protection and reject-overbudget behavior. Then validate one passive sensor module.
11. **Simultaneous-operation/coexistence.** Enable GNSS, SD write, OLED, 900, 2.4, 5.x, RX5808, Wi-Fi/BLE and permitted SkyPort load. Record resets, bus failures, GNSS C/N0 changes, RF floors/spurs and temperatures. Disable individual sources to identify interference.
12. **Mechanical assembly.** Only after electrical acceptance, generate/use the PCBA-derived enclosure. Validate USB, SD, antennas, VIDEO OUT, SkyPort, buttons, battery and service loops. Check the ESP32 antenna notch/cavity has the declared clearance; no battery, cable, fastener or plastic may intrude.

A failure holds the next step. Record the failing serial number and root-cause disposition before changing hardware or firmware.
