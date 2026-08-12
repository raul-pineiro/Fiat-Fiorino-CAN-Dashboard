# Cuadro TFT para Fiat Fiorino (Solución al fallo de los 400k km)

<div align="center">
  <img src="https://img.shields.io/badge/C++-000000?style=for-the-badge&logo=c%2B%2B&logoColor=red" alt="C++"/>
  <img src="https://img.shields.io/badge/ESP32-000000?style=for-the-badge&logo=espressif&logoColor=red" alt="ESP32"/>
  <img src="https://img.shields.io/badge/CAN_Bus-000000?style=for-the-badge&logo=hackaday&logoColor=red" alt="CAN Bus"/>
  <img src="https://img.shields.io/badge/Reverse_Engineering-000000?style=for-the-badge&logo=reverbnation&logoColor=red" alt="Reverse Engineering"/>
</div>

<p align="center">
  🇬🇧 <a href="README.md">English</a> | <b>🇪🇸 Español</b>
</p>

---

## El Reto y Éxito en la Inspección Técnica (ITV)

Muchos cuadros de instrumentos de la plataforma Fiat Mini (Fiorino, Grande Punto, Alfa Romeo Mito) tienen un fallo de fábrica en su software: al alcanzar los **399.999 km**, el odómetro se congela permanentemente y deja de registrar el kilometraje. En países con inspecciones de vehículos obligatorias y estrictas (como la **ITV** española), esto hace que un vehículo mecánicamente en buen estado sea ilegal para circular.

En lugar de cambiar el cuadro completo por piezas caras que volverán a fallar, este proyecto sustituye la pantalla LCD monocromo central original por una **pantalla TFT a color ST7789 de 2.4"** controlada por un ESP32. 

Mediante ingeniería inversa del bus B-CAN del vehículo, el microcontrolador lee la telemetría en tiempo real, **calcula y guarda el kilometraje total**, y renderiza una interfaz personalizada.

**El vehículo superó con éxito la inspección oficial de la ITV española con este sistema instalado, demostrando la fiabilidad y precisión de esta solución integrada.**



## Interfaz de Usuario: Integración de Doble Tema

El sistema permite cambiar de tema sobre la marcha según las preferencias del conductor.

<div align="center">
  <figure style="display: inline-block; width: 45%;">
    <figcaption><b>Ámbar Clásico (Estilo OEM Fiat)</b></figcaption>
    <img src="docs/images/cluster-classic-ui.jpg" width="100%" alt="Cuadro con interfaz Ámbar Clásica"/>
  </figure>
  <figure style="display: inline-block; width: 45%;">
    <figcaption><b>Tema Oscuro Moderno</b></figcaption>
    <img src="docs/images/cluster-modern-ui.jpg" width="100%" alt="Cuadro con interfaz Moderna Oscura"/>
  </figure>
</div>



## Prototipo vs. Hardware Final

<div align="center">
  <figure style="display: inline-block; width: 45%;">
    <figcaption><b>Fase 1: Protoboard y Cableado</b></figcaption>
    <img src="docs/images/protoboard.jpg" width="100%" alt="Prototipo en Protoboard"/>
  </figure>
  <figure style="display: inline-block; width: 45%;">
    <figcaption><b>Fase 2: Placa PCB a Medida</b></figcaption>
    <img src="docs/images/PCB.jpg" width="100%" alt="PCB Personalizada"/>
  </figure>
</div>

## Arquitectura de Hardware

> **[Análisis a Fondo: Mira los detalles de la PCB, el BOM SMT y los esquemas en la Documentación de Hardware ➔](hardware/README.es.md)**

La electrónica se conecta directamente a la placa interna del cuadro:

* **Microcontrolador:** ESP32 DevKit (gestión RTOS a doble núcleo, DMA para pantalla y Deep Sleep).
* **Interfaz CAN:** Transceptor **TJA1055T** (B-CAN de baja velocidad tolerante a fallos) adaptado con un convertidor de nivel lógico bidireccional **TXS0108E** (3.3V <-> 5V).
* **Pantalla:** TFT 2.4" SPI (ST7789) controlada mediante **LovyanGFX** con DMA por hardware.
* **Reloj y Memoria Externa:** **DS3231 RTC + EEPROM AT24C32** por I2C. Mantiene la hora en tiempo real y guarda el kilometraje total acumulado del vehículo para evitar el desgaste de la memoria Flash del ESP32.
* **Gestión de Alimentación (Doble Regulador):**
  * **Regulador 1 (12V Batería Directa / Línea 30):** Mantiene el ESP32 alimentado tras quitar la llave para ejecutar el guardado en EEPROM y entrar en Deep Sleep (~10–20 µA).
  * **Regulador 2 (12V Contacto / Línea 15):** Alimenta los buses y transceptores solo cuando el coche está encendido.
* **Sensor de Ignición:** GPIO 34 detecta el corte de contacto mediante un divisor RC (33kΩ / 10kΩ + 100nF) disparando una interrupción por hardware (`FALLING`).
* **Botonera del Cuadro:** Arquitectura de entrada híbrida que usa entradas GPIO discretas para los controles `TRIP` y `MINUS`, además de un canal ADC1 multiplexado (`ADC_CHANNEL_MENU_PLUS`) para decodificar los botones `MENU` y `PLUS` desde una única línea de resistencias.


## Características Clave de la Interfaz y Pantalla

* **Doble Tema Visual:** Cambia entre **Ámbar Clásico** (fiel al aspecto monocromo OEM de Fiat) y **Moderna** (fondo oscuro con detalles de colores personalizados).
* **Indicador Dinámico de Cambio/RPM:** Cuenta con una barra de RPM de múltiples etapas en el tema moderno. Alcanzar velocidades críticas del motor activa un parpadeo en la mísma.
* **Conversión de Unidades en Tiempo Real:** Cambio entre el sistema Métrico (`km`, `L/100km`, `°C`) e Imperial (`mi`, `mpg`, `°F`).
* **Reloj Integrado y Menú de Ajustes:** Menú de configuración completo de varios niveles para ajustar la hora mediante el RTC DS3231, resetear el ordenador de a bordo, elegir el idioma y los colores dinámicos usando directamente los botones originales del salpicadero.
* **Avisos de Estado de Hardware:** Ventanas emergentes en pantalla que notifican al usuario el estado de la EEPROM en tiempo real, guardados exitosos al apagar el motor, o errores de corrupción de memoria.



## Arquitectura de Firmware (FreeRTOS)

> **[Análisis a Fondo: Mira la matriz de tramas B-CAN de Fiat obtenida por ingeniería inversa aquí ➔](docs/Can_Matrix.es.md)**

El software está escrito en C++ sobre FreeRTOS, repartiendo las tareas entre los dos núcleos del ESP32:

* **Core 0 (`task_can_core0`):** Ejecución de alta prioridad. Decodifica las tramas B-CAN en tiempo real (velocidad, RPM, nivel de combustible, autonomía) y calcula métricas del ordenador de a bordo a alta frecuencia.
* **Core 1 (`task_gui_core1`):** Dibuja la interfaz ST7789 vía SPI DMA limitada a ~25 FPS (periodo de 40ms) para una actualización fluida de datos sin agotar los recursos del sistema. Gestiona la lectura de botones mediante ADC1, administra la máquina de estados finitos (FSM) de las pantallas, actualiza la superposición del reloj en tiempo real y orquesta la secuencia de apagado seguro.
* **Seguridad entre Hilos:** El Core 0 guarda la telemetría en una estructura `SharedData` protegida por un `dataMutex` de FreeRTOS.
* **Secuencia de Apagado:** Al quitar la llave, el GPIO 34 dispara la ISR. El Core 1 detiene el dibujo, escribe el kilometraje acumulado en la EEPROM AT24C32 por I2C y ejecuta `esp_deep_sleep_start()`.
* **Emisión Pasiva de Tramas por Serie:** Al activar `#define ENABLE_USB_SNIFFER 1`, el firmware transmite las tramas B-CAN por el puerto serie usando el formato estándar LAWICEL/SLCAN (`t1238...`), permitiendo monitorizar y registrar el tráfico en tiempo real mediante cualquier terminal serie o scripts en Python.
* **Gestión Avanzada de Botones y Combos:** *Debouncing* no bloqueante con soporte para combinaciones de múltiples botones (ej. `TRIP` + `PLUS`). Las acciones de `TRIP` se activan al soltar el botón para actuar como tecla modificadora, mientras que una lógica de bloqueo transitorio del ADC evita falsos positivos durante las transiciones de voltaje analógico.

## Cómo Empezar y Compilación

Este proyecto está construido usando [ESP-IDF](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/). Dado que este repositorio depende de librerías externas enlazadas como submódulos de Git, debes clonarlo de forma recursiva.

**1. Clonar el repositorio:**
```bash
git clone --recursive https://github.com/raul-pineiro/Fiat-Fiorino-CAN-Dashboard.git

```

*(Si ya lo clonaste sin la etiqueta `--recursive`, ejecuta `git submodule update --init --recursive` dentro de la carpeta).*

**2. Compilar y Flashear:**

* Abre el proyecto en tu IDE.
* Selecciona **ESP32 Dev Module** como placa de destino.
* Compila y sube el firmware vía USB.



## Personalización de Fuentes (Avanzado)

Este repositorio incluye un archivo precompilado `MyFonts.h` para que puedas compilar el firmware del ESP32 directamente. Sin embargo, si quieres cambiar la tipografía de la interfaz, puedes generar una cabecera personalizada usando el script de Python ubicado en el directorio `tools/`.

Debido a restricciones de derechos de autor y licencias, no se incluyen archivos de fuentes `.ttf` y `.otf` en crudo en este repositorio. Tendrás que descargar tus propias fuentes (por ejemplo, desde Google Fonts) para usar esta herramienta.

**Cómo generar una cabecera de fuente personalizada:**

1. **Instalar dependencias:** El script requiere la librería Freetype.
   ```bash
   pip install freetype-py

   ```

2. **Ejecutar el generador:** Pasa el nombre del archivo de salida, seguido de grupos de tres argumentos por cada fuente (ruta del archivo, tamaño en pt, y el nombre del struct en C++).
   ```bash
   python tools/ttf2gfx.py MyFonts.h mi_fuente_personalizada.ttf 16 NombreFuenteCustom 

   ```


3. **Reemplazar el original:** Mueve tu nuevo `MyFonts.h` al directorio `include` del firmware, sobrescribiendo el archivo por defecto.

> **⚠️ Modificación de Código Importante:**
> Los nombres que asignes en el comando de Python (como `NombreFuenteCustom`) se convierten en los nombres reales de los structs C++ en el archivo de cabecera. Si generas una cabecera con nombres de fuentes nuevos o diferentes, **debes** actualizar manualmente `ScreenHandler.cpp` para hacer referencia a estos nuevos nombres. Si la lógica de la pantalla busca los antiguos structs de las fuentes, el firmware fallará al compilar.

## Librerías y Créditos

* **[LovyanGFX](https://github.com/lovyan03/LovyanGFX)** de `@lovyan03` – Librería rápida y eficiente para pantallas en ESP32 (incluida como submódulo Git).



## Licencia

Licencia MIT. Consulta `LICENSE` para más información.