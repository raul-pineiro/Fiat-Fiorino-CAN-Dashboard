# 🏎️ Fiat Fiorino CAN-Bus Dashboard (400k km Fix)

<div align="center">
  <img src="https://img.shields.io/badge/C++-000000?style=for-the-badge&logo=c%2B%2B&logoColor=red" alt="C++"/>
  <img src="https://img.shields.io/badge/ESP32-000000?style=for-the-badge&logo=espressif&logoColor=red" alt="ESP32"/>
  <img src="https://img.shields.io/badge/CAN_Bus-000000?style=for-the-badge&logo=hackaday&logoColor=red" alt="CAN Bus"/>
  <img src="https://img.shields.io/badge/Reverse_Engineering-000000?style=for-the-badge&logo=reverbnation&logoColor=red" alt="Reverse Engineering"/>
</div>

---

<p align="center">
  <b>🇬🇧 English</b> | 🇪🇸 <a href="README.es.md">Leer en Español</a>
</p>

---

## 🛑 The Problem
The OEM instrument clusters on the Fiat Mini platform (Fiorino, Grande Punto, Alfa Romeo Mito) suffer from a hardware/software limitation: upon reaching **400,000 km**, the odometer permanently stops recording mileage. 
This project focuses on replacing the legacy, locked dashboard with an embedded digital solution, ensuring proper functionality without altering the vehicle's core safety systems.

## ⚙️ Technical Solution
Design and implementation of an automotive-grade telemetry and visualization system powered by an **ESP32**. The device interfaces in parallel with the vehicle's high-speed **C-CAN (500 kbps)** network via the OBD-II port, operating strictly in *Listen-Only Mode* to guarantee the isolation and integrity of safety-critical modules (ABS/ECU).

The system intercepts wheel speed CAN frames broadcasted by the ABS module, calculates physical displacement using numerical integration, and stores the cumulative mileage data into an external EEPROM to prevent flash memory degradation on the microcontroller.

---

## 🏗️ Hardware Architecture

The electrical design is optimized for automotive environments, capable of handling 12V-14V voltage fluctuations and isolating power consumption when the ignition is turned off to prevent parasitic battery drain.

* **Microcontroller:** ESP32 DevKit (Core logic, CAN decoding, and OTA updates).
* **Network Interface:** SN65HVD230 Transceiver (3.3V C-CAN signal conditioning).
* **Power Management:** LM2596 / Mini360 Buck Step-Down Converter (Stable 5V output), wired exclusively to the 12V switched ignition line (IGN).
* **Data Persistence & Clock:** DS3231 RTC Module (Real-Time Clock retention and mileage backup via the integrated AT24C32 EEPROM).
* **User Interface:** 2.4" TFT SPI ST7789 Display.

---

## 👨‍💻 Software Development & Reverse Engineering

The firmware is developed in C++ and structured into three primary sub-systems:

1. **CAN Sniffing & Decoding:** Reverse engineering the C-CAN bus to isolate specific frame IDs transmitted by the ABS module that contain raw wheel speed data.
2. **Odometry Algorithm:** Integration algorithm designed to process real-time velocity packets into accurate distance travelled.
3. **Over-The-Air (OTA):** Wireless firmware update deployment via WiFi, allowing in-car calibration directly from the driver's seat and eliminating electrical hazards from USB tethering.

---
> **Engineering Note:** This project interacts with critical automotive networks. Physical connection to pins 6 and 14 of the OBD-II port is handled completely passively (sniffing) to never interfere with the bus arbitration or priority of ECU/ABS messaging.
