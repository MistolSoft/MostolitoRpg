# **Hoja de Referencia Técnica: Waveshare ESP32-S3-LCD-2 (2.0 Inch)**

Este documento detalla las capacidades de hardware y la configuración de periféricos del sistema integrado Waveshare basado en el MCU ESP32-S3 de 2 pulgadas, corregido según el diagrama de pines oficial.

## **1\. Unidad de Procesamiento (MCU)**

El dispositivo utiliza el SoC **ESP32-S3R8** de Espressif Systems.

* **Arquitectura:** Xtensa® Dual-Core 32-bit LX7.  
* **Frecuencia de Reloj:** Hasta 240 MHz.  
* **PSRAM Externa (OPI):** 8 MB (Integrada en el encapsulado R8).  
* **Flash Externa:** 16 MB (Conectada vía **QSPI**).

## **2\. Subsistema de Memoria**

El modelo **ESP32-S3R8** integra memoria interna y externa configurada para aplicaciones intensivas en datos como MistolitoRPG.

* **SRAM Interna:** 512 KB.
* **ROM:** 384 KB.
* **PSRAM Externa (OPI):** 8 MB (Conectada vía **Octal SPI / OPI** para alto ancho de banda, integrada en el encapsulado R8).
* **Cache:** L1 Cache configurable para optimizar los accesos a Flash y PSRAM (Fundamental para el refresco de pantalla con LVGL).

## **3\. Pantalla LCD Integrada**

* **Tipo de Panel:** IPS LCD.  
* **Tamaño:** 2.0 pulgadas.  
* **Resolución:** 240 (H) × 320 (V) píxeles.  
* **Controlador:** ST7789T3.  
* **Interfaz de Datos:** SPI compartido.

### **Mapeo de Pines LCD**

| Función | GPIO | Tipo | Notas |
| :---- | :---- | :---- | :---- |
| **RST** | 0 | Salida | Hardware Reset (Compartido con BOOT) |
| **BL** | 1 | Salida | Control de retroiluminación |
| **MOSI** | 38 | Salida | Datos SPI (Bus compartido con SD) |
| **SCLK** | 39 | Salida | Reloj SPI (Bus compartido con SD) |
| **DC** | 42 | Salida | Data/Command Selection |
| **CS** | 45 | Salida | Chip Select LCD |

## **3\. Almacenamiento y Sensores**

### **3.1 Micro SD Card (SPI Mode)**

| Función | GPIO | Notas |
| :---- | :---- | :---- |
| **MOSI** | 38 | Bus compartido con LCD |
| **SCLK** | 39 | Bus compartido con LCD |
| **MISO** | 40 | Datos Master In |
| **CS** | 41 | Chip Select SD |

### **3.2 Sensores y Pantalla Táctil (I2C)**

* **Sensor Inercial (IMU):** QMI8658.
* **Pantalla Táctil (Touch):** CST816D (Capacitivo).
* **Interfaz:** Bus I2C compartido.

| Función | GPIO | Notas |
| :---- | :---- | :---- |
| **SCL (I2C)** | 47 | Bus compartido IMU/TP |
| **SDA (I2C)** | 48 | Bus compartido IMU/TP |
| **IMU INT1** | 3 | Interrupción IMU |
| **TP INT** | 46 | Interrupción Touch |

## **4\. Interfaz de Cámara (DVP)**

Soporta sensores de imagen paralelos de 8 bits (OV2640/OV5640) para interacciones de visión divina.

| Función | GPIO | Función | GPIO |
| :---- | :---- | :---- | :---- |
| **D0** | 12 | **D4** | 14 |
| **D1** | 13 | **D5** | 10 |
| **D2** | 15 | **D6** | 7 |
| **D3** | 11 | **D7** | 2 |
| **XCLK** | 8 | **PCLK** | 9 |
| **VSYNC** | 6 | **HREF** | 4 |
| **PWDN** | 17 | **TWI_SDA** | 21 |
| **TWI_CLK**| 16 | | |

## **5\. Otros Periféricos**

* **Batería (ADC):** GPIO 5 para lectura de voltaje.
* **USB Native:** GPIO 19 (D-) y GPIO 20 (D+).
* **Botón Físico:** GPIO 0 (BOOT) - Nota: Reinicia el LCD físicamente al presionarlo.

## **5\. Consideraciones de Software**

* **SPI Conflict:** Al compartir MOSI y SCLK entre la pantalla y la SD, se debe asegurar que los drivers (ej. LVGL y FatFS) respeten los estados de CS. Se recomienda usar el SPI Master Driver de ESP-IDF para gestionar el bus.
* **Backlight:** El pin IO1 permite control PWM para ajustar el brillo de la mascota según la hora del día o estado de ánimo.
