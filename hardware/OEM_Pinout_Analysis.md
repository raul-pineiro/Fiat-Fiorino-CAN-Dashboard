# OEM Cluster Pinout & Reverse Engineering Analysis

<p align="center">
  <b>🇬🇧 English</b> | 🇪🇸 <a href="OEM_Pinout_Analysis.es.md">Español</a>
</p>

This document details the reverse engineering methodology, signal characterization, and connector pinout of the original Fiat/PSA instrument cluster (specifically the Fiat Fiorino). The primary objective of this analysis is to safely interface a custom ESP32-based hardware architecture with the vehicle's OEM wiring harness without disrupting the Body Control Module (BCM) or triggering Diagnostic Trouble Codes.

---

## 1. Reverse Engineering Methodology

To guarantee accurate integration with the vehicle's existing electrical architecture, the OEM cluster connector was systematically mapped using non-destructive probing techniques.

### 1.1. Power Distribution Identification

Basic multimeter probing referenced to chassis ground was utilized to identify power delivery lines. By observing voltage states across different ignition key positions, the lines were classified into continuous power (Terminal 30 equivalent) and switched power (Terminal 15 equivalent). This distinction is critical for the ESP32 power management strategy, allowing the system to save telemetry to EEPROM and enter deep sleep upon ignition cutoff.

### 1.2. Communication Bus Characterization

An oscilloscope was used to analyze active data lines. Probing the suspect pins revealed differential signaling traffic resting at specific voltage levels. The analysis confirmed the presence of a Fault-Tolerant CAN (FT-CAN) network operating at **50 kbps** and 5V logic levels, dictating the requirement for a TJA1055T fault-tolerant transceiver rather than a standard High-Speed CAN transceiver.

> **Cross-Reference:** 
> For the actual decoded telemetry signals and equations extracted from this bus, see the [Fiat B-CAN Reverse Engineering Matrix](../docs/Can_Matrix.md).

### 1.3. Human-Machine Interface (HMI) Input Mapping

The steering column and cluster bezel buttons operate via a combination of discrete digital lines and analog resistor ladder networks. These were mapped by monitoring pin voltages while physically actuating each button. The analysis revealed a multiplexed architecture designed to minimize the wire count at the cluster connector.

---

## 2. Fiat Fiorino OEM Connector Pinout

The physical pinout derived from the reverse engineering process is detailed below.

| Pin | Designation | Quiescent State (Idle) | Active State | Technical Description |
| --- | --- | --- | --- | --- |
| **1** | `GND` | 0V | 0V | Main chassis ground. |
| **2** | `VBAT` | 12V | 12V | Continuous battery power (Always ON). |
| **3** | `IGN` | 0V | 12V | Switched ignition power (ON with key). |
| **5** | `CAN_L` | ~1.4V - 5V | Data Traffic | Fault-Tolerant CAN Low line. |
| **6** | `CAN_H` | ~0.6V - 5V | Data Traffic | Fault-Tolerant CAN High line. |
| **9** | `TRIP` | 12V | 0V | Discrete digital switch. Pulled to GND when pressed. |
| **13** | `MENU / PLUS` | 5V | 2.5V / 0V | Shared analog multiplexed line. |
| **16** | `MINUS` | 12V | 0V | Discrete digital switch. Pulled to GND when pressed. |

> **Compatibility Note:** This pinout has been documented based on the specific cluster revision analyzed for this project. Due to production revisions, model year, engine variant (e.g., 1.3 MultiJet vs. 1.4 Petrol), or cluster OEM manufacturer variations (e.g., Magneti Marelli vs. Visteon), pin assignments may differ slightly. Always verify voltage levels with a multimeter or oscilloscope before making permanent connections.

---

## 3. Signal Analysis & Hardware Interfacing Strategy

Interfacing a 3.3V microcontroller (ESP32) with a 12V/5V automotive environment requires strict electrical isolation and signal conditioning to ensure MCU survivability and prevent OEM system faults.

### 3.1. Analog Multiplexed Switches (Pin 13)

The `MENU` and `PLUS` buttons share a single physical wire. The vehicle's internal resistor network outputs specific voltage levels based on switch closures:

* **Idle (Open circuit):** 5.0V
* **MENU pressed:** 2.5V (Intermediate resistance)
* **PLUS pressed:** 0.0V (Direct short to ground)

**Interfacing Challenge:** Connecting this line directly to the ESP32 would destroy the 3.3V ADC. Furthermore, utilizing a standard low-impedance voltage divider would act as a parallel resistance to the vehicle's internal circuit, altering the voltage drops and potentially triggering a switch failure DTC in the Body Control Module.

**Solution:** The custom PCB implements a high-impedance voltage divider (100kΩ series, 150kΩ parallel to ground) feeding into GPIO 35. This ensures the current draw is negligible, preserving the OEM signal integrity.

The resulting voltage delivered to the ESP32 ADC is calculated as follows:

$$V_{out} = V_{in} \times \frac{R_2}{R_1 + R_2}$$

For the 5V idle state:

$$V_{adc(idle)} = 5.0V \times \frac{150k\Omega}{100k\Omega + 150k\Omega} = 3.0V$$

This safely scales the 0V–5V automotive range into a 0V–3.0V range, perfectly mapping to the ESP32 ADC's linear measurement window (with 12dB attenuation).

### 3.2. Discrete Digital Switches (Pins 9 & 16)

The `TRIP` and `MINUS` buttons operate on a 12V logic level, resting at 12V when unactuated and pulling down to 0V upon press.

**Solution:** These 12V signals must be safely stepped down and conditioned before reaching the 3.3V microcontroller. The custom carrier board implements optical signal conditioning and level translation using PC817 optocouplers. While sharing a common chassis ground, this approach translates the 12V automotive logic to a safe 3.3V level and protects the sensitive ESP32 GPIO signal line from high-voltage transients inherent to the vehicle's electrical system. While treated as standard discrete digital inputs in the software (`gpio_get_level`), internal pull-up/pull-down resistors on the ESP32 are disabled via firmware, relying entirely on the hardware conditioning circuit to maintain a stable state.

> **Cross-Reference:** 
> For the specific component selection (e.g., PC817 optocouplers, resistor tolerances) and the complete electrical routing, refer to the [Hardware Specifications & BOM](README.md) and the accompanying schematic.