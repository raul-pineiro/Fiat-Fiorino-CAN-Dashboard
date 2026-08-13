# HMI Architecture & Visual Layout Guide

<p align="center">
  <b>🇬🇧 English</b> | 🇪🇸 <a href="HMI_Architecture.es.md">Español</a>
</p>

This document details the Human-Machine Interface (HMI) state machine, screen cycle topology, physical input mapping, and visual layouts for the custom cluster display. The interface runs on Core 1 of the ESP32 and uses a Finite State Machine to handle dynamic button mapping, telemetry overlays, and menu navigation.

---

## 1. Core State Machine Variables

The HMI logic is strictly driven by the combination of these five core variables in the firmware:

*   `_ui_mode`: The primary operating mode (`DASHBOARD` or `SETTINGS`).
*   `_menu_level`: The current depth within the settings tree (`PAGE_SELECT`, `SUB_SELECT`, `EDIT_VALUE`).
*   `_current_settings_page`: The active menu category page.
*   `_clock_edit_step`: A specific sub-state for the RTC clock setup (`NONE`, `HOURS`, `MINUTES`).
*   `_current_sub_option`: The active binary option index (0 or 1) inside a menu page.

---

## 2. DASHBOARD Mode (Telemetry Views)

Pressing the `TRIP` button calls `nextPage()`, cycling through the 9 telemetry screens using a modulo (`%`) operation. 

> **Layout & Localization Notes:**
> * **Language Support:** Screenshots demonstrate the interface configured in Spanish (`ES`). The firmware natively supports on-the-fly language switching (`EN` / `ES`) and has been validated in both localizations.
> * **Persistent UI Elements:** Real-time clock overlay is always active across all screens. The bottom-left area displays Engine RPM by default, unless overridden by specific telemetry contexts (e.g., fuel level or trip distance).

<table width="100%">
  <thead>
    <tr>
      <th align="left" width="16%">View Name</th>
      <th align="center" width="32%">Classic Amber (OEM)</th>
      <th align="center" width="32%">Modern Dark</th>
      <th align="left" width="20%">Description</th>
    </tr>
  </thead>
  <tbody>
    <tr>
      <td><b>0. Total KM</b></td>
      <td align="center"><img src="images/Layouts/Classic-Kilometers.jpg" width="100%"/></td>
      <td align="center"><img src="images/Layouts/Modern-Kilometers.jpg" width="100%"/></td>
      <td>Primary display showing total odometer distance.</td>
    </tr>
    <tr>
      <td><b>1. Autonomy</b></td>
      <td align="center"><img src="images/Layouts/Classic-Autonomy.jpg" width="100%"/></td>
      <td align="center"><img src="images/Layouts/Modern-Autonomy.jpg" width="100%"/></td>
      <td>Estimated range remaining. Bottom-left overridden with fuel level in Liters/Gallons and %.</td>
    </tr>
    <tr>
      <td><b>2. Trip KM</b></td>
      <td align="center"><img src="images/Layouts/Classic-TripDistance.jpg" width="100%"/></td>
      <td align="center"><img src="images/Layouts/Modern-TripDistance.jpg" width="100%"/></td>
      <td>Partial trip distance recorder.</td>
    </tr>
    <tr>
      <td><b>3. Trip L/100km</b></td>
      <td align="center"><img src="images/Layouts/Classic-AvgComs.jpg" width="100%"/></td>
      <td align="center"><img src="images/Layouts/Modern-AvgComs.jpg" width="100%"/></td>
      <td>Average fuel consumption. Bottom-left shows trip distance for context.</td>
    </tr>
    <tr>
      <td><b>4. Instant L/100km</b></td>
      <td align="center"><img src="images/Layouts/Classic-TripComs.jpg" width="100%"/></td>
      <td align="center"><img src="images/Layouts/Modern-TripComs.jpg" width="100%"/></td>
      <td>Instantaneous fuel economy (switches automatically to L/h below 3 km/h).</td>
    </tr>
    <tr>
      <td><b>5. Trip Avg km/h</b></td>
      <td align="center"><img src="images/Layouts/Classic-AvgVel.jpg" width="100%"/></td>
      <td align="center"><img src="images/Layouts/Modern-AvgVel.jpg" width="100%"/></td>
      <td>Average velocity calculation over the active trip segment.</td>
    </tr>
    <tr>
      <td><b>6. Trip Time</b></td>
      <td align="center"><img src="images/Layouts/Classic-TripTime.jpg" width="100%"/></td>
      <td align="center"><img src="images/Layouts/Modern-TripTime.jpg" width="100%"/></td>
      <td>Drive timer (MM:SS when under 1 hour, HH:MM when over 1 hour).</td>
    </tr>
    <tr>
      <td><b>7. RPM & Temp</b></td>
      <td align="center"><img src="images/Layouts/Classic-RPM.jpg" width="100%"/></td>
      <td align="center"><img src="images/Layouts/Modern-RPM.jpg" width="100%"/></td>
      <td>Central engine speed reader. Bottom-left shows coolant temperature.</td>
    </tr>
    <tr>
      <td><b>8. Digital Speed</b></td>
      <td align="center"><img src="images/Layouts/Classic-Velocity.jpg" width="100%"/></td>
      <td align="center"><img src="images/Layouts/Modern-Velocity.jpg" width="100%"/></td>
      <td>High-visibility digital speedometer readout.</td>
    </tr>
  </tbody>
</table>

---

## 3. SETTINGS Mode (Configuration Hierarchy)

Pressing the `MENU` button from the `DASHBOARD` switches `_ui_mode` to `SETTINGS`. Once inside any settings page or submenu:
*   **`TRIP`** acts as the **Enter / Select** key (moves down levels or confirms edits).
*   **`MENU`** acts as the **Back / Return** key (goes up one level or exits back to `DASHBOARD`).
*   **`PLUS` (+)** and **`MINUS` (-)** navigate between pages, toggle options, or adjust values.

### System Settings (`REGIONAL_SETUP`)
Allows switching units between Metric and Imperial systems and sets UI language.

<table width="100%">
  <thead>
    <tr>
      <th align="left" width="16%">State</th>
      <th align="center" width="42%">Classic Amber (OEM)</th>
      <th align="center" width="42%">Modern Dark</th>
    </tr>
  </thead>
  <tbody>
    <tr>
      <td><b>Page Select</b></td>
      <td align="center"><img src="images/Layouts/Classic-Menu-System.jpg" width="100%"/></td>
      <td align="center"><img src="images/Layouts/Modern-Menu-System.jpg" width="100%"/></td>
    </tr>
    <tr>
      <td><b>Edit Value</b></td>
      <td align="center"><img src="images/Layouts/Classic-Menu-System-Options.jpg" width="100%"/></td>
      <td align="center"><img src="images/Layouts/Modern-Menu-System-Options.jpg" width="100%"/></td>
    </tr>
  </tbody>
</table>

### RPM & Theme Settings (`DYNAMIC_RPM_COLOR`)
Configures dynamic RPM bar behavior and custom accent color themes for the Modern UI.

<table width="100%">
  <thead>
    <tr>
      <th align="left" width="16%">State</th>
      <th align="center" width="42%">Classic Amber (OEM)</th>
      <th align="center" width="42%">Modern Dark</th>
    </tr>
  </thead>
  <tbody>
    <tr>
      <td><b>Page Select</b></td>
      <td align="center"><img src="images/Layouts/Classic-Menu-Colors.jpg" width="100%"/></td>
      <td align="center"><img src="images/Layouts/Modern-Menu-Colors.jpg" width="100%"/></td>
    </tr>
    <tr>
      <td><b>Edit Value</b></td>
      <td align="center"><img src="images/Layouts/Classic-Menu-Colors-Options.jpg" width="100%"/></td>
      <td align="center"><img src="images/Layouts/Modern-Menu-Colors-Options.jpg" width="100%"/></td>
    </tr>
  </tbody>
</table>

#### Modern Accent Color Palette Options:

<table width="100%">
  <tr>
    <td align="center" width="16.6%"><img src="images/Layouts/Modern-Menu-Colors-Options-Cyan.jpg" width="100%" alt="Cyan Theme"/><br/><sub><b>Cyan</b></sub></td>
    <td align="center" width="16.6%"><img src="images/Layouts/Modern-Menu-Colors-Options-Green.jpg" width="100%" alt="Green Theme"/><br/><sub><b>Green</b></sub></td>
    <td align="center" width="16.6%"><img src="images/Layouts/Modern-Menu-Colors-Options-Yellow.jpg" width="100%" alt="Yellow Theme"/><br/><sub><b>Yellow</b></sub></td>
    <td align="center" width="16.6%"><img src="images/Layouts/Modern-Menu-Colors-Options-Red.jpg" width="100%" alt="Red Theme"/><br/><sub><b>Red</b></sub></td>
    <td align="center" width="16.6%"><img src="images/Layouts/Modern-Menu-Colors-Options-White.jpg" width="100%" alt="White Theme"/><br/><sub><b>White</b></sub></td>
    <td align="center" width="16.6%"><img src="images/Layouts/Modern-Menu-Colors-Options-Blue.jpg" width="100%" alt="Blue Theme"/><br/><sub><b>Blue</b></sub></td>
  </tr>
</table>

### Clock Configuration (`CLOCK_CONFIGURATION`)
Sets RTC DS3231 time with visual cursor indicators (`^^`) under hours/minutes during edit states.

<table width="100%">
  <thead>
    <tr>
      <th align="left" width="16%">State</th>
      <th align="center" width="42%">Classic Amber (OEM)</th>
      <th align="center" width="42%">Modern Dark</th>
    </tr>
  </thead>
  <tbody>
    <tr>
      <td><b>Page Select</b></td>
      <td align="center"><img src="images/Layouts/Classic-Menu-Clock.jpg" width="100%"/></td>
      <td align="center"><img src="images/Layouts/Modern-Menu-Clock.jpg" width="100%"/></td>
    </tr>
    <tr>
      <td><b>Edit Value</b></td>
      <td align="center"><img src="images/Layouts/Classic-Menu-Clock-Options.jpg" width="100%"/></td>
      <td align="center"><img src="images/Layouts/Modern-Menu-Clock-Options.jpg" width="100%"/></td>
    </tr>
  </tbody>
</table>

### Reset Trip Data (`RESET_TRIP`)
Provides confirmation safety before clearing trip memory metrics.

<table width="100%">
  <thead>
    <tr>
      <th align="left" width="16%">State</th>
      <th align="center" width="42%">Classic Amber (OEM)</th>
      <th align="center" width="42%">Modern Dark</th>
    </tr>
  </thead>
  <tbody>
    <tr>
      <td><b>Page Select</b></td>
      <td align="center"><img src="images/Layouts/Classic-Menu-ResetTrip.jpg" width="100%"/></td>
      <td align="center"><img src="images/Layouts/Modern-Menu-ResetTrip.jpg" width="100%"/></td>
    </tr>
    <tr>
      <td><b>Confirm Reset</b></td>
      <td align="center"><img src="images/Layouts/Classic-Menu-ResetTrip-Options.jpg" width="100%"/></td>
      <td align="center"><img src="images/Layouts/Modern-Menu-ResetTrip-Options.jpg" width="100%"/></td>
    </tr>
  </tbody>
</table>

---

## 4. Hardware Input Mapping Matrix

| Input Trigger | UI Mode & Level | Action Executed |
| :--- | :--- | :--- |
| **TRIP** (Short Press) | `DASHBOARD` | Cycle to the next telemetry view (0 → 8 → 0). |
| **TRIP + PLUS** (Combo) | `DASHBOARD` | `toggleStyle()`: Switch live theme between Classic Amber and Modern Dark. |
| **MENU** (Short Press) | `DASHBOARD` | Open Settings (`_ui_mode = SETTINGS`). |
| **PLUS / MINUS** | `SETTINGS` (Page Select) | Navigate between settings pages. |
| **TRIP** | `SETTINGS` (Page Select) | Select page (Enters `SUB_SELECT` or `EDIT_VALUE`). |
| **PLUS / MINUS** | `SETTINGS` (Sub Select) | Toggle sub-option index (0 or 1). |
| **TRIP** | `SETTINGS` (Sub Select) | Enter edit mode (`EDIT_VALUE`) or confirm action. |
| **PLUS / MINUS** | `SETTINGS` (Edit Value) | Increment/decrement active value in real-time. |
| **TRIP** | `SETTINGS` (Edit Value) | Save value and exit step. |
| **MENU** | `SETTINGS` (Any Level) | Back / Cancel edit / Return to `DASHBOARD`. |

---

## 5. Low-Level Button Handling & Signal Processing

The input layer is managed by `ButtonHandler.cpp` using the ESP-IDF `adc_oneshot` driver on Core 1. It interprets the conditioned signals coming from the vehicle's steering column and cluster buttons, handling debouncing, combo detection, and signal demultiplexing.

> **Hardware & Reverse Engineering Note:**
> The physical wiring, raw OEM voltages, connector pinouts, and the custom high-impedance resistor matrix designed to read these signals safely are detailed in the [OEM Pinout & Reverse Engineering Analysis](../hardware/OEM_Pinout_Analysis.md) document.

### 5.1. Firmware Input Modes & Thresholds
The firmware translates the conditioned electrical signals into UI actions:

*   **Discrete Digital Inputs (`TRIP`, `MINUS`):** Sampled via standard GPIO logic. Microcontroller internal pull resistors are disabled since the custom PCB already provides physical signal conditioning.
*   **Analog Multiplexed Inputs (`MENU`, `PLUS`):** Both switches share a single ADC channel on the OEM side. They are read via `ADC_CHANNEL_MENU_PLUS` (12dB attenuation, 12-bit resolution / 0–4095 raw count):
    *   `PLUS`: Active when $ADC \le ADC\_PLUS\_MAX$ (Voltage pulled to ground).
    *   `MENU`: Active when $ADC\_MENU\_MIN \le ADC \le ADC\_MENU\_MAX$ (Intermediate voltage window).
    *   `IDLE`: Active when $ADC > ADC\_MENU\_MAX$ (Line resting state).

### 5.2. Firmware Anti-Ghosting & Combo Logic

#### Falling-Edge Modifier Combo Handling (`_trip_consumed_by_combo`)
To allow the `TRIP` button to act both as an independent UI command and as a modifier key for screen combos:
*   Single `TRIP` actions evaluate strictly on the **falling edge**.
*   If `PLUS` is pressed while holding `TRIP`, `handleComboTripPlus()` fires instantly and sets `_trip_consumed_by_combo = true`.
*   Upon subsequent release of `TRIP`, the standalone action is suppressed, avoiding unintended screen changes after triggering a combo.

#### Dual-Edge Transient Filtering for Shared Analog Line (`_menu_lockout` & `_menu_pending`)
Because `PLUS` pulls the shared analog line to 0V while `MENU` sits at an intermediate voltage, pressing or releasing `PLUS` forces the voltage to cross the `MENU` threshold twice (Idle ➔ **[MENU Zone]** ➔ 0V, and 0V ➔ **[MENU Zone]** ➔ Idle).

To prevent false `MENU` triggers during a `PLUS` button interaction:
*   `MENU` action execution is deferred to the **falling edge / return to IDLE zone** (`current_idle`).
*   **Pressing `PLUS`** immediately sets `_menu_lockout = true` and forces `_menu_pending = false`, canceling any transient `MENU` state caught on the downward voltage swing.
*   **Releasing `PLUS`** passes through the `MENU` window on the upward swing, but `_menu_lockout` blocks `_menu_pending` from becoming `true`.
*   A legitimate `MENU` action only executes when returning to `IDLE` if `_menu_pending` was validly set without an active lockout. `_menu_lockout` resets only once steady-state `IDLE` is restored.