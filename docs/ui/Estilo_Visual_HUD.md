# **Estilo Visual y Diseño de Interfaz: Cyber-Terminal Neon Punk**

El estilo visual de MistolitoRPG se define como una **Terminal de Datos de Alta Fidelidad**. La estética combina **ASCII Art** para los marcos y textos con **Sprites de Alta Resolución** para el Pet, creando un choque visual entre lo retro-futurista y lo moderno.

## **1. Paleta de Colores (Neon Punk & Alertas)**
*   **Fondo (Background):** `#000000` (Negro Puro). Maximiza el contraste y ahorra energía.
*   **Color Primario (UI/Texto):** `#00FFFF` (Cian Eléctrico). Color base de la terminal.
*   **Color Secundario (Estético):** `#BC13FE` (Violeta Neón). Para acentos y bordes decorativos.
*   **Color de Datos (Lectura):** `#FFFFFF` (Blanco Puro). Para bloques de texto largos de la crónica.

### **Sistema de Estatus (Alertas Neón):**
*   **ÉXITO (Success):** `#39FF14` (Verde Neón). Tiradas de d20 aprobadas y subida de nivel.
*   **ADVERTENCIA (Warning):** `#FF8C00` (Naranja Neón). Vectores de necesidad por debajo del 25%.
*   **PELIGRO/ERROR (Danger):** `#FF003F` (Rojo Neón). Estado crítico de salud o fallo de ritual.

## **2. Anatomía de la Pantalla (Layout)**
La resolución de 240x320 se divide en zonas funcionales mediante marcos de caracteres ASCII.

### **A. Cabecera (Top Bar): Status de Deidad**
*   **Contenido:** Contador de `DIVINE_POINTS` (DP) en cian y estado del sistema (Batería/Red) en formato ASCII.

### **B. Visor Central: El Hábitat del Campeón**
*   **Fondo:** Una rejilla (grid) de perspectiva cian muy sutil (estilo Tron).
*   **El Pet:** El único elemento **NO-ASCII**. Sprites animados en color, con un efecto de "flicker" o scanlines tenue para integrarlo en la estética.

### **C. Monitor de Drive (Vectores de Necesidad)**
*   **Visualización:** Barras horizontales representadas por caracteres `[##########]`. Cambian de color (Cian -> Naranja -> Rojo) según el nivel de urgencia.

---

## **3. Instancias de Pantalla (Modos)**
Navegables mediante gestos táctiles (*swipes* laterales):

1.  **TERMINAL_LIFE (Main View):** Vista del Pet y monitores vitales. Es el HUD de supervivencia.
2.  **CHRONICLE_LOG (Adventure View):** 
    *   Fondo negro con texto blanco/cian.
    *   Muestra la historia generada: *"LOG_ENTRY_2026: El Campeón ha detectado una anomalía... Explorando..."*
3.  **GENETIC_MATRIX (DNA View):** 
    *   Muestra los Atributos SRD (STR, DEX, INT, etc.) y los Perks.
    *   Gráficos de radar o tablas ASCII para visualizar el potencial del espécimen.
4.  **DIVINE_VISION (Camera View):** 
    *   Feed de la cámara en escala de grises con filtro cian.
    *   Superposición de escaneo para detectar objetos u ofrendas visuales del mundo real.
5.  **GENESIS_TERMINAL (World Creation):** 
    *   Interfaz de selección de tríadas para la materialización del universo con marcos ASCII resaltados.
6.  **ASCENSION_RITUAL (Evolution):** 
    *   Pantalla de alta energía. Visualiza el dado d20 y el rango de éxito expandido según el nivel del Pet.

---

## **4. Efectos Visuales y Animación**
*   **Boot Sequence:** Secuencia de inicio con scroll de "Kernel loading..." al encender.
*   **Critical Glitch:** Si los vectores llegan a niveles peligrosos, la pantalla parpadea en **Rojo Neón** con caracteres aleatorios rompiendo la interfaz.
*   **Typewriter Effect:** El texto de la historia aparece letra por letra, simulando una terminal antigua.
*   **Ritual Flow:** Durante la ascensión, los bordes de la pantalla emiten un aura cian que se intensifica según el rango de "Trascendencia" alcanzado.
