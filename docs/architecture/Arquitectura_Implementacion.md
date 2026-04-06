# **Arquitectura de Software: Componentes de Sistema**

Este documento define la arquitectura lógica de los componentes que integran MistolitoRPG, optimizada para el hardware ESP32-S3 y la gestión eficiente de la memoria fractal.

---

## **1. Núcleo de Gestión de Memoria (PSRAM)**
Para un sistema basado en vectores y narrativa, la gestión de memoria es el pilar central.
*   **Asignación Dinámica:** Uso intensivo de la PSRAM de 8MB para almacenar los buffers de video de LVGL y las tablas de vectores del nivel actual del cerebro.
*   **Aceleración Hardware:** Uso de la librería `esp-dsp` para realizar cálculos de **Similitud de Coseno** mediante instrucciones SIMD del procesador S3, permitiendo una navegación fluida por el árbol fractal de la SD.

---

## **2. El Almacenamiento Fractal (SD Card)**
Eliminamos las bases de datos relacionales en favor de un sistema de archivos jerárquico puro.
*   **Base de Datos Vectorial:** Implementación manual sobre el sistema de archivos de la SD. La ubicación de los datos está determinada por su vector de embedding (`/BRAIN/[TIPO]/0xVector...`).
*   **Índices Binarios (`index.bin`):** Cada nodo de la memoria contiene un índice binario de los vectores hijos para permitir el "Zoom Vectorial" sin cargar toda la DB en RAM.

---

## **3. Comunicación e Inteligencia Artificial (Bridge)**
Conexión con la nube para las capas cognitivas y generativas.
*   **Cliente HTTPS (`esp_http_client`):** Manejo de peticiones POST seguras para el envío de contextos y recepción de respuestas de la IA.
*   **Seguridad (`mbedtls`):** Validación de certificados SSL y cifrado de las comunicaciones con las APIs externas.
*   **Procesamiento de Datos (`cJSON`):** Construcción de los paquetes de información (Prompts) y extracción de los vectores de embedding y respuestas narrativas.

---

## **4. Motor de Embeddings (Creación y Navegación Vectorial)**
Este componente es el "Arquitecto de la Memoria". Su función es bidireccional:
*   **Creación de Estructura:** Cuando el Pet vive una experiencia nueva, el Motor de Embeddings envía el texto a la API y utiliza el vector devuelto para **nombrar la nueva carpeta o archivo** en la SD. El vector ES la identidad física del dato.
*   **Navegación por Coordenadas:** Para recordar, el sistema traduce la intención actual en un vector y navega por el árbol fractal comparando este "Vector Objetivo" contra los nombres (vectores) de las carpetas existentes.
*   **Gestión de Red:** Utiliza `esp_http_client` y `cJSON` para las consultas constantes de embeddings, asegurando que la mente del Pet crezca y se recupere siempre bajo la misma lógica matemática.

---

## **5. Interfaz de Usuario y HUD (LVGL)**
*   **Motor Gráfico:** LVGL v8.3 configurado para renderizado sobre fondo negro (Ahorro de energía/Contraste Neon).
*   **Gestión de Assets:** Los sprites del Pet y los marcos ASCII se cargan bajo demanda desde la SD para preservar la RAM interna.

---

## **5. Sensores y Seguridad de ADN**
*   **Matriz de Sensores:** Integración de drivers para el IMU (QMI8658) y la Cámara (DVP).
*   **Seguridad del ADN:** Uso de `mbedtls/sha256` para la generación y validación del hash único del Pet, acelerado por el hardware del S3.

---

### **Mapa de Responsabilidades del Sistema**

| Funcionalidad | Componente Lógico | Librería Principal |
| :--- | :--- | :--- |
| **Instintos** | Motor de Vectores | `esp-dsp` |
| **Memoria** | Navegador Fractal | `sdmmc / vfs` |
| **Narrativa** | Bridge (IA) | `esp_http_client` |
| **Interfaz** | LVGL | `esp_lcd` |
| **Procesamiento** | Parser de Datos | `cJSON` |
| **Seguridad** | Cifrado y SSL | `mbedtls` |
