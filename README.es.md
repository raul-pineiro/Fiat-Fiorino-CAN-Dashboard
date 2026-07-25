# Cuadro TFT para Fiat Fiorino (Solución al fallo de los 400k km)

<div align="center">
  <img src="https://img.shields.io/badge/C++-000000?style=for-the-badge&logo=c%2B%2B&logoColor=red" alt="C++"/>
  <img src="https://img.shields.io/badge/ESP32-000000?style=for-the-badge&logo=espressif&logoColor=red" alt="ESP32"/>
  <img src="https://img.shields.io/badge/CAN_Bus-000000?style=for-the-badge&logo=hackaday&logoColor=red" alt="CAN Bus"/>
  <img src="https://img.shields.io/badge/Reverse_Engineering-000000?style=for-the-badge&logo=reverbnation&logoColor=red" alt="Reverse Engineering"/>
</div>

---

<p align="center">
  🇬🇧 <a href="README.md">English</a> | <b>🇪🇸 Español</b>
</p>

---

## Descripción del Proyecto

Los cuadros de instrumentos de la plataforma Fiat Mini (Fiorino, Grande Punto, Alfa Romeo Mito) tienen un fallo de fábrica en su firmware: al alcanzar los **399.999 km**, el odómetro se congela permanentemente y deja de contar.

En lugar de cambiar el cuadro completo o reprogramar memorias EEPROM que vuelven a fallar con el tiempo, este proyecto sustituye la pantalla LCD monocromo central original por una **pantalla TFT a color ST7789 de 2.4"** controlada por un ESP32. El microcontrolador lee la red B-CAN del coche en tiempo real, guarda el kilometraje extra a partir de los 400.000 km y muestra la telemetría en pantalla.

---

## Arquitectura de Hardware

La electrónica se conecta directamente a la placa interna del cuadro:

* **Microcontrolador:** ESP32 DevKit (gestión RTOS a doble núcleo, DMA para pantalla y Deep Sleep).
* **Interfaz CAN:** Transceptor **TJA1055T** (B-CAN de baja velocidad tolerante a fallos) adaptado con un convertidor de nivel lógico bidireccional **TXS0108E** (3.3V <-> 5V).
* **Pantalla:** TFT 2.4" SPI (ST7789) controlada mediante **LovyanGFX** con DMA por hardware.
* **Reloj y Memoria Externa:** **DS3231 RTC + EEPROM AT24C32** por I2C. Guarda los kilómetros extra pasados los 399.999 km para evitar desgastar la memoria Flash del ESP32.
* **Gestión de Alimentación (Doble Regulador):**
  * **Regulador 1 (12V Batería Directa / Línea 30):** Mantiene el ESP32 alimentado tras quitar la llave para ejecutar el guardado en EEPROM y entrar en Deep Sleep (~10–20 µA).
  * **Regulador 2 (12V Contacto / Línea 15):** Alimenta los buses y transceptores solo cuando el coche está encendido.
* **Sensor de Ignición:** GPIO 34 detecta el corte de contacto mediante un divisor RC (33kΩ / 10kΩ + 100nF) disparando una interrupción por hardware (`FALLING`).
* **Botonera:** GPIO 35 y 32 leen los botones del cuadro/volante mediante divisores de alta impedancia (100kΩ / 150kΩ) en ADC1.

---

## Arquitectura de Firmware (FreeRTOS)

El software está escrito en C++ sobre FreeRTOS, repartiendo las tareas entre los dos núcleos del ESP32:

* **Core 0 (`task_can_core0`):** Tareas de alta prioridad. Decodifica tramas B-CAN (velocidad, RPM, combustible, odómetro), lee la botonera por ADC1 y atiende la interrupción del sensor de llave.
* **Core 1 (`task_gui_core1`):** Dibuja la interfaz en la pantalla ST7789 por SPI DMA (30–60 FPS). Actualiza valores numéricos de forma asíncrona (~200 ms) para evitar parpadeos y sincroniza la hora local con el RTC DS3231.
* **Seguridad entre Hilos:** El Core 0 guarda la telemetría en una estructura `SharedData` protegida por un `dataMutex` de FreeRTOS.
* **Secuencia de Apagado:** Al quitar la llave, el GPIO 34 dispara la ISR. El Core 1 detiene el dibujo, escribe el kilometraje acumulado en la EEPROM AT24C32 por I2C y ejecuta `esp_deep_sleep_start()`.
* **Emisión Pasiva de Tramas por Serie:** Al activar `#define ENABLE_USB_SNIFFER 1`, el firmware transmite las tramas B-CAN por el puerto serie usando el formato estándar LAWICEL/SLCAN (`t1238...`), permitiendo monitorizar y registrar el tráfico en tiempo real mediante cualquier terminal serie o scripts en Python.

---

## Librerías y Créditos

* **[LovyanGFX](https://github.com/lovyan03/LovyanGFX)** de `@lovyan03` – Librería rápida y eficiente para pantallas en ESP32 (incluida como submódulo Git).

---

## Licencia

Licencia MIT. Consulta `LICENSE` para más información.