#!/usr/bin/env python3
"""Add the Rev C.1 wide-5.x, video-validation, ADC/IO, and SkyPort circuits."""
from __future__ import annotations

from pathlib import Path
import re

from kicad_sch_api import Schematic
from kicad_sch_api.library.cache import get_symbol_cache

ROOT = Path(__file__).resolve().parents[3]
REV = ROOT / "hardware" / "rev_c1"
CUSTOM = REV / "SkySweep32RevC1.kicad_sym"
GLOBAL_NETS: set[str] = set()


def component(
    sch: Schematic,
    lib_id: str,
    reference: str,
    value: str,
    position: tuple[float, float],
    footprint: str,
    *,
    rotation: int = 0,
    **properties: str,
):
    return sch.components.add(
        lib_id,
        reference,
        value,
        position,
        footprint,
        rotation=rotation,
        **properties,
    )


def pin_position(item, number: str) -> tuple[float, float]:
    point = item.get_pin_position(number)
    if point is None:
        raise RuntimeError(f"pin {item.reference}.{number} has no resolved position")
    # kicad-sch-api 0.5.0 applies KiCad's page-Y direction incorrectly.
    return point.x, 2.0 * item.position.y - point.y


def local(sch: Schematic, item, number: str, net: str) -> None:
    sch.add_label(net, pin_position(item, number))


def global_net(sch: Schematic, item, number: str, net: str) -> None:
    # add_global_label() is not serialized by kicad-sch-api 0.5.0. Emit a
    # normal label and promote it after serialization.
    GLOBAL_NETS.add(net)
    sch.add_label(net, pin_position(item, number))


def no_connect(sch: Schematic, item, number: str) -> None:
    sch.no_connects.add(pin_position(item, number))


def add_passive(
    sch: Schematic,
    lib_id: str,
    reference: str,
    value: str,
    position: tuple[float, float],
    footprint: str,
    net1: str,
    net2: str,
    *,
    global1: bool = False,
    global2: bool = False,
    rotation: int = 0,
    **properties: str,
):
    item = component(
        sch,
        lib_id,
        reference,
        value,
        position,
        footprint,
        rotation=rotation,
        **properties,
    )
    (global_net if global1 else local)(sch, item, "1", net1)
    (global_net if global2 else local)(sch, item, "2", net2)
    return item


def add_power_cap(sch: Schematic) -> None:
    item = component(
        sch,
        "Device:C",
        "C31",
        "1uF 10V X7R",
        (218.44, 91.44),
        "Capacitor_SMD:C_0603_1608Metric",
        MPN="GRM188R71A105KA61D",
    )
    global_net(sch, item, "1", "VSYS_BAT")
    global_net(sch, item, "2", "GND")


def add_detector_and_video(sch: Schematic) -> None:
    # RX5808 VIDEO is pin 11 in the qualified 2010-09-02 12-pad drawing.

    j8 = component(
        sch,
        "Connector:Conn_Coaxial",
        "J8",
        "5.1-5.95GHz DETECTOR SMA",
        (215.90, 146.05),
        "Connector_Coaxial:SMA_Amphenol_132289_EdgeMount",
        MPN="132289",
        Manufacturer="Amphenol RF",
    )
    global_net(sch, j8, "1", "FIVE_X_RF_IN")
    global_net(sch, j8, "2", "GND")

    u8 = component(
        sch,
        "SkySweep32RevC1:PE42427",
        "U8",
        "PE42427A-Z",
        (243.84, 146.05),
        "Package_DFN_QFN:LQFN-12-1EP_2x2mm_P0.5mm_EP0.7x0.7mm",
        rotation=180,
        MPN="PE42427A-Z",
        Manufacturer="pSemi",
    )
    u9 = component(
        sch,
        "SkySweep32RevC1:PE42427",
        "U9",
        "PE42427A-Z",
        (307.34, 146.05),
        "Package_DFN_QFN:LQFN-12-1EP_2x2mm_P0.5mm_EP0.7x0.7mm",
        MPN="PE42427A-Z",
        Manufacturer="pSemi",
    )
    for switch in (u8, u9):
        for number in ("1", "3", "4", "6", "7", "9", "13"):
            global_net(sch, switch, number, "GND")
        global_net(sch, switch, "12", "3V3")
        global_net(sch, switch, "11", "3V3")
        global_net(sch, switch, "10", "FIVE_X_FILTER_CTL")
    global_net(sch, u8, "5", "FIVE_X_RF_IN")
    local(sch, u8, "8", "FIVE_X_LOW_A")
    local(sch, u8, "2", "FIVE_X_WIDE_A")
    local(sch, u9, "8", "FIVE_X_LOW_B")
    local(sch, u9, "2", "FIVE_X_WIDE_B")
    local(sch, u9, "5", "FIVE_X_RF_SELECTED")

    fl1 = component(
        sch,
        "SkySweep32RevC1:RF_BPF_4P",
        "FL1",
        "5235BP44A0180001E 5170-5330MHz",
        (275.59, 132.08),
        "SkySweep32RevC1:Johanson_5235BP44A0180_1812",
        MPN="5235BP44A0180001E",
        Manufacturer="Johanson Technology",
    )
    fl2 = component(
        sch,
        "SkySweep32RevC1:RF_BPF_4P",
        "FL2",
        "5425BP15A1050001E 4900-5950MHz",
        (275.59, 160.02),
        "SkySweep32RevC1:Johanson_5425BP15A1050_0805",
        MPN="5425BP15A1050001E",
        Manufacturer="Johanson Technology",
    )
    for flt, net_a, net_b in (
        (fl1, "FIVE_X_LOW_A", "FIVE_X_LOW_B"),
        (fl2, "FIVE_X_WIDE_A", "FIVE_X_WIDE_B"),
    ):
        global_net(sch, flt, "1", "GND")
        global_net(sch, flt, "3", "GND")
        local(sch, flt, "2", net_a)
        local(sch, flt, "4", net_b)

    u10 = component(
        sch,
        "SkySweep32RevC1:AD8318",
        "U10",
        "AD8318ACPZ-R7",
        (358.14, 146.05),
        "Package_CSP:LFCSP-16-1EP_4x4mm_P0.65mm_EP2.4x2.4mm_ThermalVias",
        MPN="AD8318ACPZ-R7",
        Manufacturer="Analog Devices",
    )
    for number in ("1", "2", "8", "11", "12", "17"):
        global_net(sch, u10, number, "GND")
    for number in ("3", "4", "9"):
        global_net(sch, u10, number, "SYS_5V")
    local(sch, u10, "6", "FIVE_X_VOUT_RAW")
    local(sch, u10, "7", "FIVE_X_VOUT_RAW")
    local(sch, u10, "14", "FIVE_X_INHI")
    local(sch, u10, "15", "FIVE_X_INLO")
    local(sch, u10, "5", "FIVE_X_CLPF")
    local(sch, u10, "10", "FIVE_X_TADJ")
    local(sch, u10, "13", "FIVE_X_TEMP")

    add_passive(sch, "Device:C", "C32", "1n C0G", (330.20, 132.08), "Capacitor_SMD:C_0402_1005Metric", "FIVE_X_RF_SELECTED", "FIVE_X_INHI")
    add_passive(sch, "Device:C", "C33", "1n C0G", (330.20, 160.02), "Capacitor_SMD:C_0402_1005Metric", "FIVE_X_INLO", "GND", global2=True)
    add_passive(sch, "Device:R", "R32", "52.3R 1%", (344.17, 174.00), "Resistor_SMD:R_0402_1005Metric", "FIVE_X_INHI", "FIVE_X_INLO")
    add_passive(sch, "Device:C", "C34", "220p C0G", (372.11, 174.00), "Capacitor_SMD:C_0402_1005Metric", "FIVE_X_CLPF", "GND", global2=True)
    add_passive(sch, "Device:R", "R33", "1k 1%", (386.08, 174.00), "Resistor_SMD:R_0402_1005Metric", "FIVE_X_TADJ", "GND", global2=True)
    add_passive(sch, "Device:R", "R34", "1k 1%", (344.17, 188.00), "Resistor_SMD:R_0402_1005Metric", "FIVE_X_VOUT_RAW", "FIVE_X_ADC", global2=True)
    add_passive(sch, "Device:C", "C44", "10n C0G", (372.11, 188.00), "Capacitor_SMD:C_0402_1005Metric", "FIVE_X_ADC", "GND", global1=True, global2=True)
    add_passive(sch, "Device:R", "R35", "10k", (386.08, 188.00), "Resistor_SMD:R_0402_1005Metric", "FIVE_X_ENABLE", "FIVE_X_ENBL_PIN", global1=True)
    local(sch, u10, "16", "FIVE_X_ENBL_PIN")
    add_passive(sch, "Device:R", "R36", "100k", (400.05, 188.00), "Resistor_SMD:R_0402_1005Metric", "FIVE_X_ENBL_PIN", "GND", global2=True)
    add_passive(sch, "Device:R", "R37", "100R", (344.17, 202.00), "Resistor_SMD:R_0402_1005Metric", "FIVE_X_FILTER_SELECT", "FIVE_X_FILTER_CTL", global1=True, global2=True)
    add_passive(sch, "Device:R", "R38", "100k", (372.11, 202.00), "Resistor_SMD:R_0402_1005Metric", "FIVE_X_FILTER_CTL", "GND", global1=True, global2=True)

    for reference, value, pos, net in (
        ("C35", "100p C0G", (220.98, 188.00), "3V3"),
        ("C36", "100n X7R", (234.95, 188.00), "3V3"),
        ("C37", "100p C0G", (248.92, 188.00), "3V3"),
        ("C38", "100n X7R", (262.89, 188.00), "3V3"),
        ("C39", "100p C0G", (276.86, 188.00), "SYS_5V"),
        ("C40", "100n X7R", (290.83, 188.00), "SYS_5V"),
        ("C41", "100p C0G", (304.80, 188.00), "SYS_5V"),
        ("C42", "100n X7R", (318.77, 188.00), "SYS_5V"),
        ("C43", "1u X7R", (318.77, 202.00), "SYS_5V"),
    ):
        add_passive(sch, "Device:C", reference, value, pos, "Capacitor_SMD:C_0402_1005Metric", net, "GND", global1=True, global2=True)

    tp9 = component(sch, "Connector:TestPoint", "TP9", "FIVE_X_TEMP", (400.05, 174.00), "TestPoint:TestPoint_Pad_D1.0mm")
    local(sch, tp9, "1", "FIVE_X_TEMP")

    # Buffered 75-ohm CVBS output and independent composite-sync separator.
    u11 = component(
        sch,
        "SkySweep32RevC1:OPA360",
        "U11",
        "OPA360AIDCKR",
        (236.22, 72.39),
        "Package_TO_SOT_SMD:SOT-363_SC-70-6",
        MPN="OPA360AIDCKR",
        Manufacturer="Texas Instruments",
    )
    global_net(sch, u11, "1", "RX5808_VIDEO_RAW")
    global_net(sch, u11, "2", "GND")
    local(sch, u11, "3", "VIDEO_BUFFER_OUT")
    local(sch, u11, "4", "VIDEO_BUFFER_OUT")
    global_net(sch, u11, "5", "3V3")
    global_net(sch, u11, "6", "3V3")

    u12 = component(
        sch,
        "Video:LM1881",
        "U12",
        "LM1881MX/NOPB",
        (287.02, 72.39),
        "Package_SO:SOIC-8_3.9x4.9mm_P1.27mm",
        MPN="LM1881MX/NOPB",
        Manufacturer="Texas Instruments",
    )
    local(sch, u12, "1", "VIDEO_CSYNC_5V")
    local(sch, u12, "2", "VIDEO_SYNC_INPUT")
    local(sch, u12, "3", "VIDEO_VSYNC_TP")
    global_net(sch, u12, "4", "GND")
    no_connect(sch, u12, "5")
    local(sch, u12, "6", "VIDEO_RSET")
    no_connect(sch, u12, "7")
    global_net(sch, u12, "8", "SYS_5V")

    j9 = component(
        sch,
        "Connector_Generic:Conn_01x02",
        "J9",
        "VIDEO_OUT_75R",
        (340.36, 72.39),
        "Connector_JST:JST_SH_SM02B-SRSS-TB_1x02-1MP_P1.00mm_Horizontal",
        MPN="SM02B-SRSS-TB(LF)(SN)",
        Manufacturer="JST",
    )
    local(sch, j9, "1", "VIDEO_OUT")
    global_net(sch, j9, "2", "GND")

    add_passive(sch, "Device:R", "R39", "75R 1%", (320.04, 60.96), "Resistor_SMD:R_0603_1608Metric", "VIDEO_BUFFER_OUT", "VIDEO_OUT")
    add_passive(sch, "Device:C", "C45", "100n X7R", (259.08, 88.90), "Capacitor_SMD:C_0603_1608Metric", "RX5808_VIDEO_RAW", "VIDEO_SYNC_INPUT", global1=True)
    add_passive(sch, "Device:R", "R40", "680k 1%", (287.02, 96.52), "Resistor_SMD:R_0603_1608Metric", "VIDEO_RSET", "GND", global2=True)
    add_passive(sch, "Device:C", "C46", "100n X7R", (302.26, 96.52), "Capacitor_SMD:C_0603_1608Metric", "VIDEO_RSET", "GND", global2=True)
    add_passive(sch, "Device:C", "C47", "100n X7R", (317.50, 96.52), "Capacitor_SMD:C_0603_1608Metric", "SYS_5V", "GND", global1=True, global2=True)
    add_passive(sch, "Device:C", "C48", "100n X7R", (332.74, 96.52), "Capacitor_SMD:C_0603_1608Metric", "3V3", "GND", global1=True, global2=True)
    add_passive(sch, "Device:R", "R41", "10k 1%", (358.14, 60.96), "Resistor_SMD:R_0402_1005Metric", "VIDEO_CSYNC_5V", "VIDEO_SYNC")
    add_passive(sch, "Device:R", "R42", "20k 1%", (373.38, 60.96), "Resistor_SMD:R_0402_1005Metric", "VIDEO_SYNC", "GND", global1=True, global2=True)

    tp10 = component(sch, "Connector:TestPoint", "TP10", "RX5808_VIDEO_RAW", (358.14, 88.90), "TestPoint:TestPoint_Pad_D1.0mm")
    global_net(sch, tp10, "1", "RX5808_VIDEO_RAW")
    tp11 = component(sch, "Connector:TestPoint", "TP11", "VIDEO_VSYNC", (373.38, 88.90), "TestPoint:TestPoint_Pad_D1.0mm")
    local(sch, tp11, "1", "VIDEO_VSYNC_TP")


def add_adc_io_and_skyport(sch: Schematic) -> None:
    u13 = component(
        sch,
        "Interface_Expansion:TCA9534",
        "U13",
        "TCA9534PWR",
        (243.84, 76.20),
        "Package_SO:TSSOP-16_4.4x5mm_P0.65mm",
        MPN="TCA9534PWR",
        Manufacturer="Texas Instruments",
    )
    for number in ("1", "2", "3", "8"):
        global_net(sch, u13, number, "GND")
    global_net(sch, u13, "16", "3V3")
    global_net(sch, u13, "14", "I2C_SCL")
    global_net(sch, u13, "15", "I2C_SDA")
    global_net(sch, u13, "4", "FIVE_X_FILTER_SELECT")
    global_net(sch, u13, "5", "FIVE_X_ENABLE")
    local(sch, u13, "6", "SKYPORT_5V_EN")
    local(sch, u13, "7", "SKYPORT_3V3_EN")
    local(sch, u13, "9", "SKYPORT_DETECT_N")
    local(sch, u13, "10", "SKYPORT_IRQ_N")
    local(sch, u13, "11", "SKYPORT_RESET_N")
    local(sch, u13, "12", "SKYPORT_SPI_CS_N")
    local(sch, u13, "13", "IOX_INT_TP")

    u14 = component(
        sch,
        "Analog_ADC:ADS1015IDGS",
        "U14",
        "ADS1015IDGSR",
        (292.10, 76.20),
        "Package_SO:TSSOP-10_3x3mm_P0.5mm",
        MPN="ADS1015IDGSR",
        Manufacturer="Texas Instruments",
    )
    # Pin mapping matching official symbol pin layout:
    # AIN0=4, AIN1=5, AIN2=6, AIN3=7
    global_net(sch, u14, "4", "FIVE_X_ADC")
    global_net(sch, u14, "5", "RX5808_RSSI_ADC")
    local(sch, u14, "6", "SKYPORT_ADC")
    no_connect(sch, u14, "7")
    global_net(sch, u14, "3", "GND")
    no_connect(sch, u14, "2")
    global_net(sch, u14, "1", "GND")
    global_net(sch, u14, "9", "I2C_SDA")
    global_net(sch, u14, "10", "I2C_SCL")
    global_net(sch, u14, "8", "3V3")

    u15 = component(
        sch,
        "SkySweep32RevC1:TPS2553",
        "U15",
        "TPS2553DBVR",
        (243.84, 127.00),
        "Package_TO_SOT_SMD:SOT-23-6",
        MPN="TPS2553DBVR",
        Manufacturer="Texas Instruments",
    )
    u16 = component(
        sch,
        "SkySweep32RevC1:TPS2553",
        "U16",
        "TPS2553DBVR",
        (292.10, 127.00),
        "Package_TO_SOT_SMD:SOT-23-6",
        MPN="TPS2553DBVR",
        Manufacturer="Texas Instruments",
    )
    for switch, input_net, output_net, enable_net, fault_net in (
        (u15, "SYS_5V", "SKYPORT_5V", "SKYPORT_5V_EN", "SKYPORT_5V_FAULT_N"),
        (u16, "3V3", "SKYPORT_3V3", "SKYPORT_3V3_EN", "SKYPORT_3V3_FAULT_N"),
    ):
        global_net(sch, switch, "1", input_net)
        global_net(sch, switch, "2", "GND")
        local(sch, switch, "3", enable_net)
        local(sch, switch, "4", fault_net)
        local(sch, switch, "5", f"{switch.reference}_ILIM")
        local(sch, switch, "6", output_net)

    add_passive(sch, "Device:R", "R43", "232k 1%", (228.60, 147.32), "Resistor_SMD:R_0603_1608Metric", "U15_ILIM", "GND", global2=True)
    add_passive(sch, "Device:R", "R44", "232k 1%", (276.86, 147.32), "Resistor_SMD:R_0603_1608Metric", "U16_ILIM", "GND", global2=True)
    add_passive(sch, "Device:R", "R45", "100k", (228.60, 162.56), "Resistor_SMD:R_0603_1608Metric", "SKYPORT_5V_EN", "GND", global2=True)
    add_passive(sch, "Device:R", "R46", "100k", (276.86, 162.56), "Resistor_SMD:R_0603_1608Metric", "SKYPORT_3V3_EN", "GND", global2=True)
    add_passive(sch, "Device:C", "C49", "100n X7R", (243.84, 162.56), "Capacitor_SMD:C_0603_1608Metric", "SYS_5V", "GND", global1=True, global2=True)
    add_passive(sch, "Device:C", "C50", "10u X7R", (259.08, 162.56), "Capacitor_SMD:C_0805_2012Metric", "SKYPORT_5V", "GND", global2=True)
    add_passive(sch, "Device:C", "C51", "100n X7R", (292.10, 162.56), "Capacitor_SMD:C_0603_1608Metric", "3V3", "GND", global1=True, global2=True)
    add_passive(sch, "Device:C", "C52", "10u X7R", (307.34, 162.56), "Capacitor_SMD:C_0805_2012Metric", "SKYPORT_3V3", "GND", global2=True)
    add_passive(sch, "Device:C", "C53", "100n X7R", (243.84, 104.14), "Capacitor_SMD:C_0603_1608Metric", "3V3", "GND", global1=True, global2=True)
    add_passive(sch, "Device:C", "C54", "100n X7R", (292.10, 104.14), "Capacitor_SMD:C_0603_1608Metric", "3V3", "GND", global1=True, global2=True)

    for reference, value, pos, net in (
        ("R47", "10k", (330.20, 104.14), "SKYPORT_DETECT_N"),
        ("R48", "10k", (345.44, 104.14), "SKYPORT_IRQ_N"),
        ("R49", "10k", (360.68, 104.14), "SKYPORT_RESET_N"),
        ("R50", "10k", (375.92, 104.14), "SKYPORT_SPI_CS_N"),
        ("R51", "10k", (391.16, 104.14), "SKYPORT_5V_FAULT_N"),
        ("R52", "10k", (406.40, 104.14), "SKYPORT_3V3_FAULT_N"),
    ):
        add_passive(sch, "Device:R", reference, value, pos, "Resistor_SMD:R_0603_1608Metric", "3V3", net, global1=True)

    j10 = component(
        sch,
        "Connector_Generic:Conn_02x08_Odd_Even",
        "J10",
        "SKYPORT_1V27_2X8",
        (355.60, 147.32),
        "Connector_PinHeader_1.27mm:PinHeader_2x08_P1.27mm_Vertical_SMD",
        MPN="FTSH-108-01-L-DV-K",
        Manufacturer="Samtec",
    )
    connector_nets = {
        "1": "GND", "2": "GND",
        "3": "SKYPORT_5V", "4": "SKYPORT_3V3",
        "5": "RF_SPI_SCK", "6": "RF_SPI_MOSI",
        "7": "RF_SPI_MISO", "8": "SKYPORT_SPI_CS_N",
        "9": "I2C_SDA", "10": "I2C_SCL",
        "11": "SKYPORT_IRQ_N", "12": "SKYPORT_DETECT_N",
        "13": "SKYPORT_RESET_N", "14": "SKYPORT_ADC",
        "15": "GND", "16": "GND",
    }
    for number, net in connector_nets.items():
        if net in {"GND", "RF_SPI_SCK", "RF_SPI_MOSI", "RF_SPI_MISO", "I2C_SDA", "I2C_SCL"}:
            global_net(sch, j10, number, net)
        else:
            local(sch, j10, number, net)

    esd_lines = (
        ("U17", (335.28, 195.58), ("RF_SPI_SCK", "RF_SPI_MOSI", "RF_SPI_MISO", "SKYPORT_SPI_CS_N")),
        ("U18", (373.38, 195.58), ("I2C_SDA", "I2C_SCL", "SKYPORT_IRQ_N", "SKYPORT_DETECT_N")),
        ("U19", (411.48, 195.58), ("SKYPORT_RESET_N", "SKYPORT_ADC", "SKYPORT_5V", "SKYPORT_3V3")),
    )
    for reference, pos, nets in esd_lines:
        esd = component(
            sch,
            "Power_Protection:TPD4E05U06DQA",
            reference,
            "TPD4E05U06DQAR",
            pos,
            "Package_SON:USON-10_2.5x1.0mm_P0.5mm",
            MPN="TPD4E05U06DQAR",
            Manufacturer="Texas Instruments",
        )
        for number, net in zip(("1", "2", "4", "5"), nets):
            if net in {"RF_SPI_SCK", "RF_SPI_MOSI", "RF_SPI_MISO", "I2C_SDA", "I2C_SCL"}:
                global_net(sch, esd, number, net)
            else:
                local(sch, esd, number, net)
        global_net(sch, esd, "3", "GND")
        global_net(sch, esd, "8", "GND")
        for number in ("6", "7", "9", "10"):
            no_connect(sch, esd, number)

    for reference, value, pos, net in (
        ("TP12", "IOX_INT", (228.60, 104.14), "IOX_INT_TP"),
        ("TP13", "SKYPORT_5V_FAULT", (259.08, 127.00), "SKYPORT_5V_FAULT_N"),
        ("TP14", "SKYPORT_3V3_FAULT", (307.34, 127.00), "SKYPORT_3V3_FAULT_N"),
    ):
        testpoint = component(sch, "Connector:TestPoint", reference, value, pos, "TestPoint:TestPoint_Pad_D1.0mm")
        local(sch, testpoint, "1", net)


def create_sheet(path: Path, mutators, *, title: str, sheet_number: int) -> None:
    schematic = Schematic.create()
    schematic.set_paper_size("A3")
    schematic.set_title_block(
        title=title,
        date="2026-08-14",
        rev="C.1-DETECTOR",
        company="SkySweep32 Project",
        comments={
            1: "PASSIVE UAV / FPV EVIDENCE DETECTOR",
            2: "FIRST ARTICLE ONLY / NOT PRODUCTION VALIDATED",
            3: f"Sheet {sheet_number} of 7",
        },
    )
    for mutator in mutators:
        mutator(schematic)
    schematic.save(path)
    content = path.read_text(encoding="utf-8")
    for net in sorted(GLOBAL_NETS, key=len, reverse=True):
        pattern = rf'(?m)^([ \t]*)\(label "{re.escape(net)}"\n'
        replacement = rf'\1(global_label "{net}"\n\1\t(shape bidirectional)\n'
        content = re.sub(pattern, replacement, content)
    path.write_text(content, encoding="utf-8")
    print(f"Created {path.relative_to(ROOT)}")


def main() -> int:
    cache = get_symbol_cache()
    cache.add_library_path(CUSTOM)
    create_sheet(
        REV / "06_detector_video.kicad_sch",
        (add_power_cap, add_detector_and_video),
        title="06 — Wide 5.x Detection and Analog Video Validation",
        sheet_number=6,
    )
    create_sheet(
        REV / "07_skyport.kicad_sch",
        (add_adc_io_and_skyport,),
        title="07 — ADC, Control Expansion, and SkyPort",
        sheet_number=7,
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
