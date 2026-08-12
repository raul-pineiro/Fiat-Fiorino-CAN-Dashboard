# Matriz de Ingeniería Inversa del B-CAN de Fiat

<p align="center">
  🇬🇧 <a href="CAN_Matrix.md">English</a> | <b>🇪🇸 Español</b>
</p>

Este documento detalla las tramas CAN obtenidas mediante ingeniería inversa para la plataforma Fiat Mini. El bus utiliza **Identificadores CAN Extendidos de 29 bits**, con datos estructurados principalmente en formato **Big-Endian (Motorola)**.

## Especificaciones del Bus
* **Protocolo:** CAN 2.0B (Identificadores Extendidos de 29 bits)
* **Capa Física:** Low-Speed Fault-Tolerant CAN (ISO 11898-3) @ **50 kbps**
* **Orden de Bytes:** Big-Endian (Motorola, byte más significativo primero)

---

## Matriz de Tramas

| ID CAN (Hex) | DLC | Bytes | Señal | Fórmula / Lógica de Decodificación | Notas |
|---|:---:|---|---|---|---|
| `0x04214001` | ≥7 | `[6]` | **RPM del Motor** | `Byte[6] * 32` | Resolución: 32 RPM/bit |
| `0x04214001` | ≥7 | `[4], [5]` | **Vol. de Inyección** | `((Byte[4] << 8) \| Byte[5]) / 100.0` | Salida en mm³/embolo |
| `0x04214001` | ≥7 | `[3]` | **Temp. del Motor** | `Byte[3] - 40` | Offset: -40 °C |
| `0x04214001` | ≥7 | `[3..6]` | **Consumo Instantáneo** | `(raw_iq * RPM * 1.2) / 1e6` | Calculado en L/h (4 cilindros, 4 tiempos) |
| `0x0C014003` | ≥6 | `[1..3]` | **Odómetro OEM** | `(Byte[1] << 16) \| (Byte[2] << 8) \| Byte[3]` | Distancia total en km (desactivado por bloqueo a los 399.999 km) |
| `0x0C014003` | ≥6 | `[4], [5]` | **Autonomía** | `((Byte[4] & 0x03) << 8) \| Byte[5]` | Byte 4 con máscara de bits |
| `0x04394000` | ≥2 | `[0], [1]` | **Velocidad** | `((Byte[0] << 8) \| Byte[1]) * 0.0625` | Salida en km/h |
| `0x06214000` | ≥6 | `[5]` | **Nivel de Combustible** | `Byte[5]` | Valor porcentual directo (0-100%) |

---

## Notas de Implementación

- **Seguridad de Memoria:** Las tareas del firmware deben validar el `DLC` (Data Length Code) antes de extraer la carga útil (*payload*) para prevenir lecturas fuera de límites en memoria.
- **Multiplexado:** La trama `0x0C014003` (`ID_CLUSTER_INFO`) multiplexa la información del odómetro y la autonomía dentro de la misma estructura de datos.
- **Procesamiento en Tiempo Real:** El procesamiento de estas tramas se ejecuta de forma prioritaria en el Core 0 del ESP32 dentro de una tarea dedicada en FreeRTOS para minimizar la latencia de recepción.