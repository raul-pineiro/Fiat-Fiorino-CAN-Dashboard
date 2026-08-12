# Hardware Specifications & Bill of Materials (BOM)

<p align="center">
  <b>🇬🇧 English</b> | 🇪🇸 <a href="README.es.md">Español</a>
</p>

This document specifies the electronic component selection, system architecture, and module integration for the Fiat Mini B-CAN Digital Cluster interface board.

The design utilizes a **Carrier Board Architecture**, combining automated SMT passive assembly with Commercial Off-The-Shelf functional modules and galvanic isolation stages for rapid prototyping and modular serviceability.


## 1. Surface Mount Assembly BOM (JLCPCB PCBA)

| Ref / Designator | Qty | Value / Description | Footprint | Manufacturer | MPN | LCSC Part # | Function / Signal Path |
| :--- | :---: | :--- | :---: | :--- | :--- | :---: | :--- |
| **C1, C2, C3, C4, C5** | 5 | 100 nF, 50V, X7R Ceramic | 1206 | Samsung | `CL31B104KBCNNNC` | C24497 | Decoupling & RC Anti-Bounce Filter |
| **R1, R2** | 2 | 4.7 kΩ, 1%, 0.25W | 1206 | UNI-ROYAL | `1206W4F4701T5E` | C17936 | TJA1055 CAN Bus Bias (RTH/RTL) |
| **R3** | 1 | 33 kΩ, 1%, 0.25W | 1206 | UNI-ROYAL | `1206W4F3302T5E` | C18004 | Ignition Voltage Divider (Upper) |
| **R4, R9, R10** | 3 | 10 kΩ, 1%, 0.25W | 1206 | UNI-ROYAL | `1206W4F1002T5E` | C17902 | Ignition Voltage Divider (Lower) & Pull-downs |
| **R5** | 1 | 100 kΩ, 1%, 0.25W | 1206 | UNI-ROYAL | `1206W4F1003T5E` | C17900 | ADC Protection Divider Cable 1 (Upper) |
| **R6** | 1 | 150 kΩ, 1%, 0.25W | 1206 | UNI-ROYAL | `QR1206F150KP05Z` | C176234 | ADC Protection Divider Cable 1 (Lower) |
| **R7, R8** | 2 | 2.2 kΩ, 1%, 0.25W | 1206 | UNI-ROYAL | `1206W4F2201T5E` | C17948 | Optocoupler Current Limiting / Pull-ups |

---

## 2. Integrated Circuits, Power & COTS Sub-Assemblies

| Ref / Designator | Qty | Component / Module | Manufacturer / Source | MPN / Standard | Interface / Package | Technical Specifications & Function |
| :--- | :---: | :--- | :--- | :--- | :--- | :--- |
| **U1** | 1 | Microcontroller Core | Espressif Systems | `ESP32-WROOM-32` | DevKit Board (DIP Header) | Dual-Core 240 MHz, 3.3V Logic, Wi-Fi/BT |
| **U2** | 1 | Fault-Tolerant CAN IC | NXP Semiconductors | `TJA1055T/3/C,518` | SOIC-14 (Direct Solder) | Low-Speed B-CAN (50 kbps), Automotive Grade |
| **U3** | 1 | Level Translator Sub-Board | COTS (AliExpress) | `TXS0108E` Breakout | Pin Header Module | 8-Channel Bidirectional Voltage Level Translator ($3.3\text{V} \leftrightarrow 5\text{V}$) |
| **U4, U6** | 2 | Optocoupler Isolator | Sharp / Generic | `PC817` | DIP-4 / Module | Galvanic Isolation for "Minus" (U4) and "Trip" (U6) Buttons ($V_{\text{iso}} = 5000\text{V}_{\text{rms}}$) |
| **U5** | 1 | RTC & EEPROM Sub-Board | COTS (AliExpress) | `DS3231` + `AT24C32` | $I^2C$ Pin Header (LIR3032) | High-Precision TCXO RTC + Non-Volatile Memory (Battery Backed) |
| **U7** | 1 | DC-DC Step-Down Converter | Murata Power Solutions | `OKI-78SR-5/1.5-W36-C` | Single In-Line (SIP-3) | 12V Battery Permanent Input $\to$ 5V Output (Deep Sleep) |
| **U8** | 1 | DC-DC Step-Down Converter | Murata Power Solutions | `OKI-78SR-5/1.5-W36-C` | Single In-Line (SIP-3) | 12V Ignition Switched Input $\to$ 5V Output (Logic/CAN) |
| **SalidaPantalla1** | 1 | Display Sub-Assembly | Sitronix / COTS | `ST7789V` Controller | 2.4" TFT Panel (SPI) | $240 \times 320$ Resolution, 4-Wire SPI Interface |

---

## 3. Electrical Architecture & Safety Highlights

1. **Galvanic Isolation Stage (`PC817` Optocouplers):**
   - High-voltage automotive transients (load dumps, inductive kickback from relays) and ground noise are decoupled from sensitive ESP32 GPIOs using optical isolation.

2. **High-Efficiency Power Regulation (`OKI-78SR`):**
   - Murata `OKI-78SR` switching regulators replace linear $7805$ regulators to eliminate heat dissipation issues under an input range of $7\text{V}$ to $36\text{V}$, yielding over 85% efficiency and preventing thermal shutdown in automotive cabin environments.

3. **Fault-Tolerant CAN Layer (`TJA1055T`):**
   - Selected for physical compatibility with Fiat B-CAN (50 kbps). Handles single-wire failure modes and automatically reconfigures bus topology upon physical CANH or CANL faults.

4. **Display Interconnect (`SalidaPantalla1`):**
   - Panel connection is routed through a dedicated JST header connector (`SalidaPantalla1`) on the carrier board, decoupling the mechanical display mounting from the main PCB.


## 4. Mechanical Assembly & Module Integration

To ensure a reliable prototype without loose wiring, the system uses a rigid mounting approach:

   * Board Mounting: The custom carrier board is directly secured to the rear plastic housing of the OEM cluster using standard hex standoffs.

   * Module Integration: All COTS sub-modules (ESP32, CAN transceiver, Level Shifters) are physically anchored to the carrier board via through-hole pin headers and soldered in place, providing stable electrical and mechanical connections.

   * Housing Modification: The original ABS plastic bracket was modified using rotary tools to accommodate the new 2.4" TFT display housing, ensuring a flush fit."

<p align="center">
  <img src="../docs/images/carrier-board-pcba.jpg" width="30%" alt="Bare Carrier Board with SMT components" />
  <img src="../docs/images/rear-housing-integration.jpg" width="30%" alt="Carrier Board mounted on cluster using hex standoffs" />
  <img src="../docs/images/display-bracket-retrofit.jpg" width="30%" alt="Modified OEM bracket with TFT display" />
</p>
<p align="center">
  <i>Left: SMT Assembly. Center: Hex standoff mounting. Right: OEM bracket retrofit for TFT display.</i>
</p>

## 5. Schematic Diagram

The complete circuit logic, including signal routing, voltage dividers, and optical isolation stages, is fully documented in the schematic below.

<p align="center">
  <a href="schematic.pdf">
    <img src="../docs/images/schematic_preview.png" width="100%" alt="KiCad Schematic Diagram" />
  </a>
</p>
<p align="center">
  <i>Click the image to view or download the full-resolution <a href="schematic.pdf">schematic.pdf</a>.</i>
</p>

> **Note:** While the complete electrical architecture is provided for transparency and peer review, raw manufacturing files (Gerbers) are excluded from this repository.