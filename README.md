# Fiat Fiorino TFT Display Upgrade (400k km Odometer Fix)

<div align="center">
  <img src="https://img.shields.io/badge/C++-000000?style=for-the-badge&logo=c%2B%2B&logoColor=red" alt="C++"/>
  <img src="https://img.shields.io/badge/ESP32-000000?style=for-the-badge&logo=espressif&logoColor=red" alt="ESP32"/>
  <img src="https://img.shields.io/badge/CAN_Bus-000000?style=for-the-badge&logo=hackaday&logoColor=red" alt="CAN Bus"/>
  <img src="https://img.shields.io/badge/Reverse_Engineering-000000?style=for-the-badge&logo=reverbnation&logoColor=red" alt="Reverse Engineering"/>
</div>

---

<p align="center">
  <b>🇬🇧 English</b> | 🇪🇸 <a href="README.es.md">Español</a>
</p>

---

## Overview

Instrument clusters on Fiat Mini platform vehicles (Fiorino, Grande Punto, Alfa Romeo Mito) have a factory software bug: once the odometer reaches **399,999 km**, it permanently freezes and stops recording mileage.

Rather than replacing the entire cluster or re-flashing EEPROMs that fail again later, this project replaces the stock central monochrome LCD with a **2.4" ST7789 color TFT display** driven by an ESP32. The microcontroller reads telemetry from the vehicle's B-CAN bus in real time, keeps track of extra mileage past 400,000 km, and renders a custom UI.

---

## Hardware Design

The custom board interfaces directly with the cluster's internal electronics:

* **MCU:** ESP32 DevKit (handles dual-core RTOS logic, display DMA, and deep sleep state).
* **CAN Interface:** **TJA1055T** (Fault-Tolerant Low-Speed B-CAN transceiver) paired with a **TXS0108E** bi-directional logic level shifter (3.3V <-> 5V).
* **Display:** 2.4" SPI TFT (ST7789) driven using **LovyanGFX** with hardware DMA.
* **RTC & External Storage:** **DS3231 RTC + AT24C32 EEPROM** over I2C. Stores extra mileage past 399,999 km without wearing out the ESP32's flash memory.
* **Dual Power Regulation:**
  * **Buck 1 (Always-On 12V / Battery Line 30):** Keeps the ESP32 powered briefly after ignition off to execute EEPROM saves and transition into Deep Sleep (~10–20 µA consumption).
  * **Buck 2 (Switched 12V / Ignition Line 15):** Powers the CAN transceivers and level shifters only when the ignition key is turned on.
* **Ignition Sensor:** GPIO 34 monitors key status using an RC filter divider (33kΩ / 10kΩ + 100nF) to trigger a hardware ISR (`FALLING`).
* **Cluster Buttons:** GPIO 35 & 32 read original dashboard/stalk button ladders through high-impedance voltage dividers (100kΩ / 150kΩ) on ADC1.
---

## Firmware Architecture (FreeRTOS)

The software is written in C++ using FreeRTOS, splitting telemetry decoding and UI rendering across both ESP32 cores:

* **Core 0 (`task_can_core0`):** High-priority execution. Decodes B-CAN frames (speed, RPM, fuel, cluster mileage), handles button polling on ADC1, and responds to ignition interrupts.
* **Core 1 (`task_gui_core1`):** Renders the ST7789 UI via SPI DMA (30–60 FPS). Updates numerical text asynchronously (~200 ms) to avoid visual flicker. Synchronizes local system time with the DS3231 RTC.
* **Thread Safety:** Core 0 updates telemetry into a thread-safe `SharedData` struct protected by a FreeRTOS `dataMutex`.
* **Power-Down Sequence:** When ignition drops, GPIO 34 fires an ISR. Core 1 pauses drawing, writes total extra mileage to the AT24C32 EEPROM over I2C, and calls `esp_deep_sleep_start()`.
* **Passive CAN ASCII Streamer:** Toggling `#define ENABLE_USB_SNIFFER 1` outputs raw B-CAN frames over Serial using standard LAWICEL/SLCAN formatting (`t1238...`). This allows live traffic monitoring and logging via any Serial terminal or custom Python scripts.

---

## Third-Party Libraries & Credits

* **[LovyanGFX](https://github.com/lovyan03/LovyanGFX)** by `@lovyan03` – Fast, memory-efficient display driver library for ESP32 (linked as Git submodule).

---

## License

MIT License. See `LICENSE` for details.