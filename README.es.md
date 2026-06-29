# 🏎️ Fiat Fiorino CAN-Bus Dashboard (Solución 400k km)

<div align="center">
  <img src="https://img.shields.io/badge/C++-000000?style=for-the-badge&logo=c%2B%2B&logoColor=red" alt="C++"/>
  <img src="https://img.shields.io/badge/ESP32-000000?style=for-the-badge&logo=espressif&logoColor=red" alt="ESP32"/>
  <img src="https://img.shields.io/badge/CAN_Bus-000000?style=for-the-badge&logo=hackaday&logoColor=red" alt="CAN Bus"/>
  <img src="https://img.shields.io/badge/Reverse_Engineering-000000?style=for-the-badge&logo=reverbnation&logoColor=red" alt="Reverse Engineering"/>
</div>

---

<p align="center">
  🇬🇧 <a href="README.md">English</a> | <b>🇪🇸 Leer en Español</b>
</p>

---

## 🛑 El Problema
Los cuadros de instrumentos originales de la plataforma Fiat Mini (Fiorino, Grande Punto, Alfa Romeo Mito) sufren de una limitación de hardware/software: al alcanzar los **400.000 km**, el odómetro deja de registrar kilometraje de forma permanente. 
Este proyecto nace de la necesidad de sustituir el panel defectuoso por una solución digital embebida, legal y funcional sin interferir en los sistemas de seguridad del vehículo.

## ⚙️ La Solución Técnica
Diseño e implementación de un sistema de telemetría y visualización de grado automotriz basado en **ESP32**. El dispositivo se conecta en paralelo a la red de alta velocidad **C-CAN (500 kbps)** del vehículo mediante el puerto OBD-II, operando estrictamente en *Listen-Only Mode* (Modo Escucha Pasiva) para garantizar la integridad y el aislamiento de módulos críticos como el ABS o la ECU.

El sistema intercepta las tramas CAN de los sensores de velocidad de rueda emitidas por el módulo ABS, calcula el desplazamiento físico mediante integración numérica y almacena la odometría acumulada en una memoria EEPROM externa para evitar el desgaste de la memoria flash del microcontrolador.

---

## 🏗️ Arquitectura de Hardware

El diseño eléctrico está optimizado para entornos de automoción, gestionando las fluctuaciones de tensión de 12V-14V e infiriendo un consumo cero cuando el contacto está apagado para evitar el drenaje de la batería.

* **Microcontrolador:** ESP32 DevKit (Gestión lógica, decodificación CAN y actualizaciones OTA).
* **Interfaz de Red:** Transceptor SN65HVD230 (Acondicionamiento de señales C-CAN de 3.3V).
* **Gestión de Energía:** Convertidor Buck Step-Down LM2596 / Mini360 (Salida estable de 5V), alimentado exclusivamente por la línea de 12V de contacto (IGN).
* **Persistencia de Datos y Reloj:** Módulo RTC DS3231 (Retención de hora exacta y almacenamiento de seguridad del kilometraje en su EEPROM AT24C32 integrada).
* **Interfaz de Usuario:** Pantalla 2.4" TFT SPI ST7789.

---

## 👨‍💻 Desarrollo de Software e Ingeniería Inversa

El firmware está desarrollado en C++ y se estructura en tres subsistemas principales:

1. **CAN Sniffing & Decoding:** Ingeniería inversa sobre el bus C-CAN para aislar los IDs específicos de los mensajes del ABS que contienen los datos brutos de velocidad de las ruedas.
2. **Algoritmo de Odometría:** Algoritmo de integración diseñado para procesar los paquetes de velocidad en tiempo real y transformarlos en distancia exacta recorrida.
3. **Over-The-Air (OTA):** Implementación de actualizaciones de firmware inalámbricas por WiFi, lo que permite la calibración dentro del coche directamente desde el asiento del conductor, eliminando riesgos eléctricos por conexión USB.

---
> **Nota de Ingeniería:** Este proyecto interactúa con redes automotrices críticas. La conexión física a los pines 6 y 14 del puerto OBD-II se realiza de manera completamente pasiva (sniffing) para no interferir jamás con el arbitraje de la red ni la prioridad de los mensajes de la ECU o el ABS.
