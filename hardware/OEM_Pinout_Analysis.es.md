# Análisis de Pinout OEM e Ingeniería Inversa

<p align="center">
  🇬🇧 <a href="OEM_Pinout_Analysis.md">English</a> | <b>🇪🇸 Español</b>
</p>

Este documento detalla la metodología de ingeniería inversa, la caracterización de señales y el pinout del conector del cuadro de instrumentos original de Fiat/PSA (específicamente la Fiat Fiorino). El objetivo principal de este análisis es interconectar de forma segura una arquitectura de hardware personalizada basada en ESP32 con el mazo de cables OEM del vehículo, sin alterar el funcionamiento de la centralita (Body Control Module) ni generar códigos de error de diagnóstico.

---

## 1. Metodología de Ingeniería Inversa

Para garantizar una integración precisa con la arquitectura eléctrica existente en el vehículo, el conector del cuadro OEM se mapeó sistemáticamente empleando técnicas de medición no destructivas.

### 1.1. Identificación de la Distribución de Energía

Se utilizó un multímetro referenciado a la masa del chasis para identificar las líneas de alimentación. Al observar los estados de voltaje en las diferentes posiciones de la llave de contacto, las líneas se clasificaron en alimentación continua (equivalente al Borne 30) y alimentación conmutada (equivalente al Borne 15). Esta distinción es crítica para la estrategia de gestión de energía del ESP32, permitiendo al sistema guardar la telemetría en la EEPROM y entrar en suspensión profunda (deep sleep) al cortar el contacto.

### 1.2. Caracterización del Bus de Comunicación

Se empleó un osciloscopio para analizar las líneas de datos activas. La medición de los pines sospechosos reveló tráfico de señalización diferencial en estado de reposo a niveles de voltaje específicos. El análisis confirmó la presencia de una red CAN Tolerante a Fallos (FT-CAN) operando a **50 kbps** y niveles lógicos de 5V, lo que dictó la necesidad de utilizar un transceptor tolerante a fallos TJA1055T en lugar de un transceptor CAN de alta velocidad estándar.

> **Referencia Cruzada:** 
> Para ver las señales de telemetría decodificadas reales y las ecuaciones extraídas de este bus, consulta la [Matriz de Ingeniería Inversa B-CAN de Fiat](../docs/Can_Matrix.es.md).

### 1.3. Mapeo de Entradas de la Interfaz Hombre-Máquina (HMI)

Los botones de la columna de dirección y del marco del cuadro operan mediante una combinación de líneas digitales discretas y redes de divisores resistivos analógicos. Estas líneas se mapearon monitorizando los voltajes de los pines mientras se accionaba físicamente cada botón. El análisis reveló una arquitectura multiplexada diseñada para minimizar el número de cables en el conector del cuadro.

---

## 2. Pinout del Conector OEM de la Fiat Fiorino

A continuación se detalla el pinout físico derivado del proceso de ingeniería inversa.

| Pin | Designación | Estado de Reposo (Idle) | Estado Activo | Descripción Técnica |
| :--- | :--- | :--- | :--- | :--- |
| **1** | `GND` | 0V | 0V | Masa principal del chasis. |
| **2** | `VBAT` | 12V | 12V | Alimentación continua de batería (Siempre activo). |
| **3** | `IGN` | 0V | 12V | Alimentación conmutada por contacto (Activo con la llave). |
| **5** | `CAN_L` | ~1.4V - 5V | Tráfico de Datos | Línea CAN Low Tolerante a Fallos. |
| **6** | `CAN_H` | ~0.6V - 5V | Tráfico de Datos | Línea CAN High Tolerante a Fallos. |
| **9** | `TRIP` | 12V | 0V | Interruptor digital discreto. Llevado a masa (GND) al pulsar. |
| **13** | `MENU / PLUS` | 5V | 2.5V / 0V | Línea analógica multiplexada compartida. |
| **16** | `MINUS` | 12V | 0V | Interruptor digital discreto. Llevado a masa (GND) al pulsar. |

> **Nota de Compatibilidad:** Este pinout se ha documentado sobre la variante de cuadro analizada en este proyecto. Debido a revisiones de fabricación, año del vehículo, motorización (ej. 1.3 MultiJet vs. 1.4 Gasolina) o variantes del fabricante del cuadro (ej. Magneti Marelli vs. Visteon), la asignación de pines puede diferir ligeramente. Se recomienda verificar siempre los niveles de voltaje con un multímetro u osciloscopio antes de realizar conexiones permanentes.

---

## 3. Análisis de Señales y Estrategia de Interfaz de Hardware

Conectar un microcontrolador de 3.3V (ESP32) a un entorno automotriz de 12V/5V requiere un estricto aislamiento eléctrico y acondicionamiento de señales para garantizar la supervivencia del MCU y prevenir fallos en el sistema OEM.

### 3.1. Interruptores Analógicos Multiplexados (Pin 13)

Los botones `MENU` y `PLUS` comparten un único cable físico. La red interna de resistencias del vehículo emite niveles de voltaje específicos en función del cierre de los interruptores:

* **Reposo (Circuito abierto):** 5.0V
* **MENU pulsado:** 2.5V (Resistencia intermedia)
* **PLUS pulsado:** 0.0V (Cortocircuito directo a masa)

**Reto de Integración:** Conectar esta línea directamente al ESP32 destruiría el ADC de 3.3V. Además, utilizar un divisor de voltaje estándar de baja impedancia actuaría como una resistencia en paralelo al circuito interno del vehículo, alterando las caídas de tensión y pudiendo generar un código de error (DTC) de fallo de interruptor en la centralita.

**Solución:** La PCB personalizada implementa un divisor de voltaje de alta impedancia (100kΩ en serie, 150kΩ en paralelo a masa) conectado al GPIO 35. Esto asegura que el consumo de corriente sea insignificante, preservando la integridad de la señal OEM.

El voltaje resultante suministrado al ADC del ESP32 se calcula de la siguiente manera:

$$V_{out}=V_{in}\times\frac{R_2}{R_1+R_2}$$

Para el estado de reposo a 5V:

$$V_{adc(idle)}=5.0V\times\frac{150k\Omega}{100k\Omega+150k\Omega}=3.0V$$

Esto escala de forma segura el rango automotriz de 0V a 5V a un rango de 0V a 3.0V, mapeándose perfectamente en la ventana de medición lineal del ADC del ESP32 (con una atenuación de 12dB).

### 3.2. Interruptores Digitales Discretos (Pines 9 y 16)

Los botones `TRIP` y `MINUS` operan a un nivel lógico de 12V, permaneciendo en 12V cuando no están accionados y cayendo a 0V al ser pulsados.

**Solución:** Estas señales de 12V deben ser reducidas y adaptadas de forma segura antes de llegar al microcontrolador de 3.3V. La placa base personalizada implementa un desacoplamiento óptico y conversión de nivel mediante optoacopladores PC817. Aunque comparte la masa con el vehículo (GND común), este enfoque traduce la lógica de 12V a un nivel seguro de 3.3V y aísla la línea de señal del GPIO frente a picos de alta tensión y transitorios del sistema eléctrico. Aunque en el software se tratan como entradas digitales discretas estándar (`gpio_get_level`), las resistencias internas de pull-up/pull-down del ESP32 se deshabilitan mediante firmware, dependiendo completamente del circuito de acondicionamiento de hardware para mantener un estado estable.

> **Referencia Cruzada:** 
> Para la selección específica de componentes (ej. optoacopladores PC817, tolerancias de resistencias) y el enrutamiento eléctrico completo, consulte el documento de [Especificaciones de Hardware y BOM](README.es.md) y su esquema correspondiente.