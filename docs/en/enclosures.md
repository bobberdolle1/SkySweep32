# SkySweep32 3D Enclosure Designs

This document outlines the recommended enclosure designs for the different tiers of the SkySweep32 passive drone detector. We provide conceptual guidelines so that makers and field engineers can design and 3D print cases suited for their specific environment.

---

## 🟢 Starter / Base Tier (Portable Tactical Unit)

The Starter tier is aimed at infantry and field operators who need a compact, low-cost device. It should fit comfortably on MOLLE webbing, a plate carrier, or inside a tactical pouch.

![Starter Concept](/C:/Users/Administrator/.gemini/antigravity/brain/2422b5e7-9a66-4ff2-8727-62beaa06c9a7/skysweep32_starter_1774706530748.png)

### Design Recommendations
- **Form Factor**: Similar to a ruggedized smartphone or walkie-talkie.
- **Material**: PETG or TPU for impact resistance. Avoid PLA as car/sun temperatures will warp it.
- **Top Panel**: Needs a small window for the 0.96" OLED screen and 2-3 tactile buttons for UI interaction (if physical buttons are added later).
- **Antenna Placement**: A single SMA hole on Top for the 2.4GHz omnidirectional antenna.
- **Battery**: Space for an 18650 cell (~3.7V 3000mAh) or a flat LiPo pack beneath the PCB.
- **Ports**: A covered USB-C port at the bottom for charging and firmware flashes.

---

## 🟡 Hunter / Standard Tier (Multi-Band Scanner)

The Hunter tier introduces wider spectrum monitoring (900MHz + 2.4GHz + 5.8GHz). It requires a slightly larger case to accommodate two or three discrete antenna ports without causing interference.

### Design Recommendations
- **Form Factor**: Walkie-talkie format but wider.
- **Antenna Spacing**: Place the 900MHz (CC1101) antenna on the left, and the 2.4GHz (NRF24) / 5.8GHz (RX5808) antennas grouped on the right. Maintain at least 3-5 cm distance to reduce cross-talk.
- **Cooling**: RX5808 modules can get warm during continuous scanning. Add subtle ventilation fins along the back plate.
- **Mounting**: Add a standard 1/4" tripod thread insert on the bottom to mount it on a car roof or static position.

---

## 🔴 Sentinel / Pro Tier (Stationary Node)

The Sentinel is a fixed installation intended for rooftops, perimeter fences, or permanent command bases. It is ruggedized against the elements and requires maximum antenna separation.

![Sentinel Concept](/C:/Users/Administrator/.gemini/antigravity/brain/2422b5e7-9a66-4ff2-8727-62beaa06c9a7/skysweep32_sentinel_1774706553771.png)

### Design Recommendations
- **Form Factor**: A robust, industrial waterproof box (IP67 rating).
- **Material**: ASA (Acrylic Styrene Acrylonitrile) for high UV resistance and structural integrity outdoors.
- **Antennas**: Three distinct SMA/N-Type bulkheads spread far apart. Use gaskets or O-rings around the connectors.
- **Mounting**: Heavy-duty wing brackets for pole mounts (U-bolts) or zip-ties.
- **Cooling**: Gore-Tex breather vents to prevent condensation buildup inside the case while maintaining waterproofing.
- **Power**: Optional solar panel input slot or an integrated step-down converter (12V-24V to 5V) for direct battery hookup.

---

## Best Practices for Printing
1. **Infill**: Use Gyroid or Cubic infill at 30-40% for maximum durability.
2. **Wall Thickness**: Minimum 3 perimeters (walls) with a 0.4mm nozzle.
3. **Inserts**: Use M3 brass heat-set inserts for joining the enclosure halves, rather than threading directly into plastic.
4. **Tolerance**: Ensure a 0.2mm tolerance gap between interlocking case parts to guarantee a snug but removable fit.

*(Note: Official STL files, once finalized by the community, will be placed in the `hardware/enclosures/` directory.)*
