# Fiat B-CAN Reverse Engineering Matrix

<p align="center">
  <b>🇬🇧 English</b> | 🇪🇸 <a href="CAN_Matrix.es.md">Español</a>
</p>

This document defines the reverse-engineered CAN frame matrix for the Fiat Mini platform B-CAN network.

## Bus Specifications
* **Protocol:** CAN 2.0B (Extended 29-bit Identifiers)
* **Physical Layer:** Low-Speed Fault-Tolerant CAN (ISO 11898-3) @ **50 kbps**
* **Byte Order:** Big-Endian (Motorola MSB First)

## Frame Matrix

| CAN ID (Hex) | DLC | Bytes | Signal | Formula / Decoding Logic | Notes |
|---|---|---|---|---|---|
| `0x04214001` | ≥7 | `[6]` | **Engine RPM** | `Byte[6] * 32` | Resolution: 32 RPM/bit |
| `0x04214001` | ≥7 | `[4], [5]` | **Injection Vol** | `((Byte[4] << 8) \| Byte[5]) / 100.0` | Output in mm³/stroke |
| `0x04214001` | ≥7 | `[3]` | **Engine Temp** | `Byte[3] - 40` | Offset: -40 °C |
| `0x04214001` | ≥7 | `[3..6]` | **Consumption** | `(raw_iq * RPM * 1.2) / 1e6` | Derived L/h (4-cyl, 4-stroke) |
| `0x0C014003` | ≥6 | `[1..3]` | **Odometer** | `(Byte[1] << 16) \| (Byte[2] << 8) \| Byte[3]` | Total distance in km (not used because gets stuck in 399.999 Km)|
| `0x0C014003` | ≥6 | `[4], [5]` | **Autonomy** | `((Byte[4] & 0x03) << 8) \| Byte[5]` | Masked Byte 4 |
| `0x04394000` | ≥2 | `[0], [1]` | **Speed** | `((Byte[0] << 8) \| Byte[1]) * 0.0625` | Output in km/h |
| `0x06214000` | ≥6 | `[5]` | **Fuel Level** | `Byte[5]` | Direct % value |

## Implementation Notes
- **Safety**: Data frames must check the `DLC` (Data Length Code) before payload extraction to prevent buffer over-reads.
- **Multiplexing**: `0x0C014003` (`ID_CLUSTER_INFO`) multiplexes both Odometer and Autonomy information in a single frame.
- **Endianness**: Multi-byte payloads are parsed assuming MSB (Most Significant Byte) first.