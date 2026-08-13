# Guía de Arquitectura HMI y Diseños Visuales

<p align="center">
  🇬🇧 <a href="HMI_Architecture.md">English</a> | <b>🇪🇸 Español</b>
</p>

Este documento detalla la máquina de estados de la Interfaz Hombre-Máquina, la topología del ciclo de pantallas, el mapeo de entradas físicas y los diseños visuales para la pantalla del cuadro de instrumentos personalizado. La interfaz se ejecuta en el Núcleo 1 del ESP32 y utiliza una Máquina de Estados Finitos para gestionar el mapeo dinámico de botones, la superposición de telemetría y la navegación por los menús.

---

## 1. Variables Principales de la Máquina de Estados

La lógica del HMI está estrictamente gestionada por la combinación de estas cinco variables principales en el firmware:

*   `_ui_mode`: El modo de funcionamiento principal (`DASHBOARD` o `SETTINGS`).
*   `_menu_level`: La profundidad actual dentro del árbol de configuración (`PAGE_SELECT`, `SUB_SELECT`, `EDIT_VALUE`).
*   `_current_settings_page`: La página de categoría de menú activa.
*   `_clock_edit_step`: Un subestado específico para la configuración del reloj RTC (`NONE`, `HOURS`, `MINUTES`).
*   `_current_sub_option`: El índice de opción binaria activa dentro de una página de menú.

---

## 2. Modo DASHBOARD (Vistas de Telemetría)

Al pulsar el botón `TRIP` se llama a `nextPage()`, pasando secuencialmente por las 9 pantallas de telemetría mediante una operación de módulo (`%`).

> **Notas de Diseño e Idioma:**
> * **Soporte de Idiomas:** Las capturas de pantalla muestran la interfaz configurada en español (`ES`). El firmware admite de forma nativa el cambio de idioma al vuelo (`EN` / `ES`) y ha sido validado en ambas localizaciones.
> * **Elementos Persistentes de la UI:** La superposición del reloj en tiempo real está siempre activa en todas las pantallas. La zona inferior izquierda muestra por defecto las RPM del motor, salvo que se invalide por contextos específicos de telemetría (p. ej., nivel de combustible o distancia del viaje).

<table width="100%">
  <thead>
    <tr>
      <th align="left" width="16%">Nombre de la Vista</th>
      <th align="center" width="32%">Ámbar Clásico (OEM)</th>
      <th align="center" width="32%">Oscuro Moderno</th>
      <th align="left" width="20%">Descripción</th>
    </tr>
  </thead>
  <tbody>
    <tr>
      <td><b>0. KM Totales</b></td>
      <td align="center"><img src="images/Layouts/Classic-Kilometers.jpg" width="100%"/></td>
      <td align="center"><img src="images/Layouts/Modern-Kilometers.jpg" width="100%"/></td>
      <td>Pantalla principal que muestra la distancia total del odómetro.</td>
    </tr>
    <tr>
      <td><b>1. Autonomía</b></td>
      <td align="center"><img src="images/Layouts/Classic-Autonomy.jpg" width="100%"/></td>
      <td align="center"><img src="images/Layouts/Modern-Autonomy.jpg" width="100%"/></td>
      <td>Rango estimado restante. Zona inferior izquierda reemplazada con nivel de combustible en Litros/Galones y %.</td>
    </tr>
    <tr>
      <td><b>2. KM Parciales</b></td>
      <td align="center"><img src="images/Layouts/Classic-TripDistance.jpg" width="100%"/></td>
      <td align="center"><img src="images/Layouts/Modern-TripDistance.jpg" width="100%"/></td>
      <td>Registrador de distancia del viaje parcial.</td>
    </tr>
    <tr>
      <td><b>3. L/100km Parciales</b></td>
      <td align="center"><img src="images/Layouts/Classic-AvgComs.jpg" width="100%"/></td>
      <td align="center"><img src="images/Layouts/Modern-AvgComs.jpg" width="100%"/></td>
      <td>Consumo medio de combustible. La zona inferior izquierda muestra la distancia parcial para dar contexto.</td>
    </tr>
    <tr>
      <td><b>4. L/100km Instantáneo</b></td>
      <td align="center"><img src="images/Layouts/Classic-TripComs.jpg" width="100%"/></td>
      <td align="center"><img src="images/Layouts/Modern-TripComs.jpg" width="100%"/></td>
      <td>Consumo instantáneo (cambia automáticamente a L/h si la velocidad es inferior a 3 km/h).</td>
    </tr>
    <tr>
      <td><b>5. Vel. Media Parcial</b></td>
      <td align="center"><img src="images/Layouts/Classic-AvgVel.jpg" width="100%"/></td>
      <td align="center"><img src="images/Layouts/Modern-AvgVel.jpg" width="100%"/></td>
      <td>Cálculo de la velocidad media durante el trayecto activo.</td>
    </tr>
    <tr>
      <td><b>6. Tiempo de Trayecto</b></td>
      <td align="center"><img src="images/Layouts/Classic-TripTime.jpg" width="100%"/></td>
      <td align="center"><img src="images/Layouts/Modern-TripTime.jpg" width="100%"/></td>
      <td>Temporizador de conducción (MM:SS si es menor a 1 hora, HH:MM si supera 1 hora).</td>
    </tr>
    <tr>
      <td><b>7. RPM y Temperatura</b></td>
      <td align="center"><img src="images/Layouts/Classic-RPM.jpg" width="100%"/></td>
      <td align="center"><img src="images/Layouts/Modern-RPM.jpg" width="100%"/></td>
      <td>Lectura central de la velocidad del motor. La zona inferior izquierda muestra la temperatura del refrigerante.</td>
    </tr>
    <tr>
      <td><b>8. Velocidad Digital</b></td>
      <td align="center"><img src="images/Layouts/Classic-Velocity.jpg" width="100%"/></td>
      <td align="center"><img src="images/Layouts/Modern-Velocity.jpg" width="100%"/></td>
      <td>Lectura del velocímetro digital de alta visibilidad.</td>
    </tr>
  </tbody>
</table>

---

## 3. Modo SETTINGS (Jerarquía de Configuración)

Pulsar el botón `MENU` desde el `DASHBOARD` cambia `_ui_mode` a `SETTINGS`. Una vez dentro de cualquier página o submenú de configuración:
*   **`TRIP`** actúa como la tecla **Aceptar / Seleccionar** (baja de nivel o confirma cambios).
*   **`MENU`** actúa como la tecla **Atrás / Volver** (sube un nivel o regresa al `DASHBOARD`).
*   **`PLUS` (+)** y **`MINUS` (-)** navegan entre páginas, alternan opciones o ajustan valores.

### Ajustes del Sistema (`REGIONAL_SETUP`)
Permite cambiar las unidades entre el sistema Métrico e Imperial y selecciona el idioma de la interfaz.

<table width="100%">
  <thead>
    <tr>
      <th align="left" width="16%">Estado</th>
      <th align="center" width="42%">Ámbar Clásico (OEM)</th>
      <th align="center" width="42%">Oscuro Moderno</th>
    </tr>
  </thead>
  <tbody>
    <tr>
      <td><b>Selección de Página</b></td>
      <td align="center"><img src="images/Layouts/Classic-Menu-System.jpg" width="100%"/></td>
      <td align="center"><img src="images/Layouts/Modern-Menu-System.jpg" width="100%"/></td>
    </tr>
    <tr>
      <td><b>Editar Valor</b></td>
      <td align="center"><img src="images/Layouts/Classic-Menu-System-Options.jpg" width="100%"/></td>
      <td align="center"><img src="images/Layouts/Modern-Menu-System-Options.jpg" width="100%"/></td>
    </tr>
  </tbody>
</table>

### Ajustes de RPM y Tema (`DYNAMIC_RPM_COLOR`)
Configura el comportamiento de la barra dinámica de RPM y los temas de color de acento personalizados para la UI Moderna.

<table width="100%">
  <thead>
    <tr>
      <th align="left" width="16%">Estado</th>
      <th align="center" width="42%">Ámbar Clásico (OEM)</th>
      <th align="center" width="42%">Oscuro Moderno</th>
    </tr>
  </thead>
  <tbody>
    <tr>
      <td><b>Selección de Página</b></td>
      <td align="center"><img src="images/Layouts/Classic-Menu-Colors.jpg" width="100%"/></td>
      <td align="center"><img src="images/Layouts/Modern-Menu-Colors.jpg" width="100%"/></td>
    </tr>
    <tr>
      <td><b>Editar Valor</b></td>
      <td align="center"><img src="images/Layouts/Classic-Menu-Colors-Options.jpg" width="100%"/></td>
      <td align="center"><img src="images/Layouts/Modern-Menu-Colors-Options.jpg" width="100%"/></td>
    </tr>
  </tbody>
</table>

#### Opciones de la Paleta de Colores de Acento (UI Moderna):

<table width="100%">
  <tr>
    <td align="center" width="16.6%"><img src="images/Layouts/Modern-Menu-Colors-Options-Cyan.jpg" width="100%" alt="Tema Cían"/><br/><sub><b>Cían</b></sub></td>
    <td align="center" width="16.6%"><img src="images/Layouts/Modern-Menu-Colors-Options-Green.jpg" width="100%" alt="Tema Verde"/><br/><sub><b>Verde</b></sub></td>
    <td align="center" width="16.6%"><img src="images/Layouts/Modern-Menu-Colors-Options-Yellow.jpg" width="100%" alt="Tema Amarillo"/><br/><sub><b>Amarillo</b></sub></td>
    <td align="center" width="16.6%"><img src="images/Layouts/Modern-Menu-Colors-Options-Red.jpg" width="100%" alt="Tema Rojo"/><br/><sub><b>Rojo</b></sub></td>
    <td align="center" width="16.6%"><img src="images/Layouts/Modern-Menu-Colors-Options-White.jpg" width="100%" alt="Tema Blanco"/><br/><sub><b>Blanco</b></sub></td>
    <td align="center" width="16.6%"><img src="images/Layouts/Modern-Menu-Colors-Options-Blue.jpg" width="100%" alt="Tema Azul"/><br/><sub><b>Azul</b></sub></td>
  </tr>
</table>

### Configuración del Reloj (`CLOCK_CONFIGURATION`)
Ajusta la hora del RTC DS3231 con indicadores visuales de cursor (`^^`) bajo las horas/minutos durante los estados de edición.

<table width="100%">
  <thead>
    <tr>
      <th align="left" width="16%">Estado</th>
      <th align="center" width="42%">Ámbar Clásico (OEM)</th>
      <th align="center" width="42%">Oscuro Moderno</th>
    </tr>
  </thead>
  <tbody>
    <tr>
      <td><b>Selección de Página</b></td>
      <td align="center"><img src="images/Layouts/Classic-Menu-Clock.jpg" width="100%"/></td>
      <td align="center"><img src="images/Layouts/Modern-Menu-Clock.jpg" width="100%"/></td>
    </tr>
    <tr>
      <td><b>Editar Valor</b></td>
      <td align="center"><img src="images/Layouts/Classic-Menu-Clock-Options.jpg" width="100%"/></td>
      <td align="center"><img src="images/Layouts/Modern-Menu-Clock-Options.jpg" width="100%"/></td>
    </tr>
  </tbody>
</table>

### Restablecer Datos de Viaje (`RESET_TRIP`)
Proporciona un paso de confirmación de seguridad antes de borrar las métricas de la memoria de viaje.

<table width="100%">
  <thead>
    <tr>
      <th align="left" width="16%">Estado</th>
      <th align="center" width="42%">Ámbar Clásico (OEM)</th>
      <th align="center" width="42%">Oscuro Moderno</th>
    </tr>
  </thead>
  <tbody>
    <tr>
      <td><b>Selección de Página</b></td>
      <td align="center"><img src="images/Layouts/Classic-Menu-ResetTrip.jpg" width="100%"/></td>
      <td align="center"><img src="images/Layouts/Modern-Menu-ResetTrip.jpg" width="100%"/></td>
    </tr>
    <tr>
      <td><b>Confirmar Borrado</b></td>
      <td align="center"><img src="images/Layouts/Classic-Menu-ResetTrip-Options.jpg" width="100%"/></td>
      <td align="center"><img src="images/Layouts/Modern-Menu-ResetTrip-Options.jpg" width="100%"/></td>
    </tr>
  </tbody>
</table>

---

## 4. Matriz de Mapeo de Entradas de Hardware

| Disparador de Entrada | Modo y Nivel de UI | Acción Ejecutada |
| :--- | :--- | :--- |
| **TRIP** (Pulsación corta) | `DASHBOARD` | Cambiar a la siguiente vista de telemetría (0 → 8 → 0). |
| **TRIP + PLUS** (Combinación) | `DASHBOARD` | `toggleStyle()`: Alternar en tiempo real entre el tema Ámbar Clásico y Oscuro Moderno. |
| **MENU** (Pulsación corta) | `DASHBOARD` | Abrir Configuración (`_ui_mode = SETTINGS`). |
| **PLUS / MINUS** | `SETTINGS` (Selección de Página) | Navegar entre las páginas de configuración. |
| **TRIP** | `SETTINGS` (Selección de Página) | Seleccionar página (Entra a `SUB_SELECT` o `EDIT_VALUE`). |
| **PLUS / MINUS** | `SETTINGS` (Selección de Submenú) | Alternar el índice de subopción (0 o 1). |
| **TRIP** | `SETTINGS` (Selección de Submenú) | Entrar al modo de edición (`EDIT_VALUE`) o confirmar acción. |
| **PLUS / MINUS** | `SETTINGS` (Edición de Valor) | Incrementar/decrementar el valor activo en tiempo real. |
| **TRIP** | `SETTINGS` (Edición de Valor) | Guardar valor y salir del paso. |
| **MENU** | `SETTINGS` (Cualquier Nivel) | **Atrás / Volver**: Cancelar edición, subir un nivel o salir al `DASHBOARD`. |

---

## 5. Gestión de Botones a Bajo Nivel y Procesamiento de Señales

La capa de entrada está gestionada por `ButtonHandler.cpp` utilizando el controlador `adc_oneshot` de ESP-IDF en el Núcleo 1. Interpreta las señales acondicionadas provenientes de la columna de dirección y los botones del cuadro del vehículo, gestionando el antirrebote (debouncing), la detección de combinaciones y el desmultiplexado de señales.

> **Nota de Hardware e Ingeniería Inversa:**
> El cableado físico, los voltajes OEM en crudo, el pinout de los conectores y la matriz de resistencias de alta impedancia diseñada a medida para leer estas señales de forma segura se detallan en el documento [Análisis de Pinout OEM e Ingeniería Inversa](../hardware/OEM_Pinout_Analysis.es.md).

### 5.1. Modos de Entrada y Umbrales en Firmware
El firmware traduce las señales eléctricas acondicionadas en acciones de la interfaz (UI):

*   **Entradas Digitales Discretas (`TRIP`, `MINUS`):** Muestreadas mediante la lógica GPIO estándar. Las resistencias internas de pull del microcontrolador están deshabilitadas, ya que la PCB personalizada ya proporciona el acondicionamiento físico de la señal.
*   **Entradas Analógicas Multiplexadas (`MENU`, `PLUS`):** Ambos pulsadores comparten un único canal ADC en el lado OEM. Se leen a través de `ADC_CHANNEL_MENU_PLUS` (atenuación de 12 dB, resolución de 12 bits / recuento bruto 0–4095):
    *   `PLUS`: Activo cuando $ADC \le ADC\_PLUS\_MAX$ (Voltaje llevado a masa).
    *   `MENU`: Activo cuando $ADC\_MENU\_MIN \le ADC \le ADC\_MENU\_MAX$ (Ventana de voltaje intermedio).
    *   `IDLE`: Activo cuando $ADC > ADC\_MENU\_MAX$ (Estado de reposo de la línea).

### 5.2. Lógica Anti-Falsas Pulsaciones y Combinaciones en Firmware

#### Detección de Combinaciones por Flanco de Bajada (`_trip_consumed_by_combo`)
Para permitir que el botón `TRIP` funcione tanto como un comando independiente de la UI como una tecla modificadora para combinaciones de pantalla:
*   Las acciones individuales de `TRIP` se evalúan estrictamente en el **flanco de bajada**.
*   Si se pulsa `PLUS` mientras se mantiene presionado `TRIP`, `handleComboTripPlus()` se ejecuta instantáneamente y establece `_trip_consumed_by_combo = true`.
*   Al soltar posteriormente el botón `TRIP`, la acción individual se suprime, evitando cambios indeseados de pantalla tras activar una combinación.

#### Filtrado de Transitorios de Doble Flanco para la Línea Analógica Compartida (`_menu_lockout` y `_menu_pending`)
Dado que `PLUS` lleva la línea compartida a 0V mientras que `MENU` se sitúa en un voltaje intermedio, presionar o soltar `PLUS` fuerza al voltaje analógico a cruzar el umbral de `MENU` dos veces (Reposo ➔ **[Zona MENU]** ➔ 0V, y 0V ➔ **[Zona MENU]** ➔ Reposo).

Para evitar falsas activaciones de `MENU` durante una interacción con el botón `PLUS`:
*   La ejecución de la acción `MENU` se pospone hasta el **flanco de bajada / retorno a la zona IDLE** (`current_idle`).
*   **Pulsar `PLUS`** establece de inmediato `_menu_lockout = true` y fuerza `_menu_pending = false`, cancelando cualquier estado transitorio de `MENU` detectado en la caída de voltaje.
*   **Soltar `PLUS`** atraviesa la ventana de `MENU` en la subida de voltaje, pero `_menu_lockout` bloquea que `_menu_pending` pase a `true`.
*   Una pulsación legítima de `MENU` solo se ejecuta al regresar a `IDLE` si `_menu_pending` se estableció de forma válida sin ningún bloqueo activo. `_menu_lockout` se restablece únicamente cuando se recupera el estado de reposo `IDLE`.