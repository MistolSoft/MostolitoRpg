# **Arquitectura de Mundo y Lore Modular**

Este documento define el sistema de gestión de entornos y narrativa procedural de MistolitoRPG. El mundo no es estático; es un conjunto de parámetros y reglas cargados desde la tarjeta SD que definen la física, la dificultad y el contexto narrativo del juego.

---

## **1. El Mundo como Conjunto de Datos (Data-Driven World)**
El universo del Pet se desglosa en archivos de configuración que el firmware procesa en tiempo real para influir en los cálculos de d20 y en el motor narrativo:

*   **Parámetros Globales (`/world/config.json`):** Define las constantes físicas universales (gravedad de datos, densidad de recursos, nivel de peligro base).
*   **Definiciones de Biomas (`/world/biomes/`):** Archivos que contienen los modificadores de stats y multiplicadores de deltas metabólicos para cada región del mapa.
*   **Enciclopedia de Entidades (`/world/entities/`):** Descripciones conceptuales de los posibles encuentros (aliados, enemigos, objetos) para alimentar el contexto de la LLM.

---

## **2. Mecánicas de Influencia Ambiental**
Independientemente de la temática, cada entorno interactúa con el Pet mediante tres ejes:

1.  **Modificadores de Atributos:** Cada bioma puede potenciar o penalizar temporalmente los Atributos SRD (STR, DEX, INT, etc.) del Pet.
2.  **Deltas de Supervivencia:** El entorno afecta la velocidad a la que se drenan o recuperan los Vectores de Impulso (Hambre, Energía, Social).
3.  **Clases de Dificultad (DC):** El mundo define los umbrales de éxito para las acciones autónomas del Pet (explorar, cazar, interactuar).

---

## **3. Inyección de Contexto Narrativo (The Bridge)**
El sistema de archivos de la SD actúa como el "Manual de Referencia" para la LLM. 
*   **Proceso:** El ESP32 extrae las descripciones abstractas del entorno actual y las combina con los resultados numéricos de los dados y el estado del Pet.
*   **Resultado:** Se genera un bloque de contexto coherente que permite a la LLM narrar la historia sin necesidad de tener el lore "programado" en su memoria interna.

---

## **5. Exploración Pasiva y Dinámica de Avance**
El mundo se explora de forma autónoma (Idle) mediante el motor de Ticks del sistema.
*   **Mecánica de Exploración:** Mientras el dispositivo está activo o en reposo, el Pet avanza por las regiones del mapa materializado.
*   **Cálculo de Velocidad de Avance:** La eficiencia en el desplazamiento depende directamente de dos factores:
    1.  **Destreza (DEX):** Un Pet con mayor DEX recorre distancias más largas por cada Tick de vida.
    2.  **Nivel de Experiencia:** A medida que el Pet sube de nivel, su conocimiento del mundo le permite optimizar las rutas de exploración, aumentando su velocidad base de descubrimiento de nuevos biomas.
*   **Eventos de Encuentro:** El avance se detiene automáticamente ante un encuentro (objetos, enemigos, dilemas), requiriendo la resolución de una tirada d20 para continuar el camino.
