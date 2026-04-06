# **Arquitectura del Cerebro: Mistolito Cognitive Engine**

El cerebro de Mistolito es un sistema de capas jerárquicas diseñado para la eficiencia del ESP32-S3, que combina reacciones físicas, instintos vitales y una memoria fractal puramente vectorial.

---

## **1. Capa Reactiva: El Tronco Encefálico (Cero Latencia)**
Encargada de la supervivencia inmediata y respuestas físicas directas.
*   **Sensor de Movimiento (IMU):** Reacciones instantáneas a sacudidas, caídas o giros (Susto, Mareo, Vigor).
*   **Gestión de Energía:** Monitoreo del estado del hardware (Batería) para inducir estados de ahorro (Bostezo, Letargo).
*   **Reflejos Táctiles:** Respuesta inmediata al toque en pantalla para generar empatía (Acariciar, Despertar).

---

## **2. Capa de Instintos: El Motor de Vectores de Impulso**
Aquí reside el libre albedrío biológico del Pet mediante un **Vector de Estado** dinámico: `[Hambre, Energía, Social/Humor]`.
*   **Lógica de Decisión:** El sistema calcula la "Acción de Máximo Deseo" sumando los impulsos de necesidad.
*   **Intervención Divina (Paralela):** Las acciones del usuario (Visiones) actúan como impulsos adicionales que pueden inclinar la balanza de una decisión, pero no dictan la narrativa de forma absoluta.

---

## **3. Capa de Memoria: El Hipocampo (Memoria Fractal Vectorial)**
Mistolito utiliza una estructura jerárquica de carpetas en la tarjeta SD donde los nombres son representaciones de **Vectores de Embedding**:
*   **Zoom Vectorial (L1, L2, L3):** El sistema navega comparando el **Vector Objetivo** (estado actual) contra los vectores de las subcarpetas almacenados en archivos `index.bin`. La ruta es puramente matemática: `/BRAIN/[TIPO]/0xVector_A/0xVector_B/`.
*   **Embeddings Externos:** El ESP32 delega la generación de vectores a una API externa, recibiendo un array de floats que utiliza para indexar o recuperar información de la SD.
*   **Soberanía de ADN:** Los vectores almacenados y la interpretación del "espacio latente" están vinculados físicamente a la identidad genética única del Pet.

---

## **4. Capa Consciente: La Corteza (LLM Bridge)**
Motor de historia profunda que genera la crónica oficial del Campeón.
*   **Autonomía de la Historia:** La narrativa emana de la interacción autónoma entre el Pet y su Mundo materializado. La intervención divina (Usuario) es un apoyo táctico, pero la vida del Pet es independiente.
*   **Paso de Contexto:** Envía a la IA el contenido de los archivos `.json` recuperados de las rutas vectoriales de Skills, World e History.

---
## **5. Supervivencia y Estado Crítico (HP)**

La integridad del Pet se gestiona mediante **Puntos de Vitalidad (HP)**.
* **Agotamiento Físico:** Si los vectores de instinto (Hambre/Energía) permanecen en niveles críticos durante demasiados Ticks, el Pet comienza a perder HP de forma constante.
* **Muerte del Pet:** Al llegar a **0 HP**, el Cerebro detiene su actividad. El mundo persiste, pero los datos del Pet se reinician (ver documento "Sistema de Vida y Muerte").
* **Mundo Persistente:** La muerte del Pet NO borra el mundo materializado. El universo permanece intacto para que un nuevo Pet pueda nacer en el mismo entorno.

---

## **6. Estados de Operación: Pleno vs. Limbo (Offline)**
El Cerebro adapta su nivel de consciencia según la disponibilidad de conexión:
*   **Modo Pleno (Conectado):** Acceso total a la Capa de Memoria Fractal y al Bridge (IA) para la generación de historia y evolución del mundo.
*   **Modo Limbo (Desconectado):** El sistema entra en una interacción básica.
    *   **Acumulación de Puntos:** La interacción física (IMU/Touch) sigue activa, permitiendo al usuario acumular **Puntos de Deidad (DP)**.
    *   **Suspensión Narrativa:** La exploración del mundo y la escritura de la crónica se pausan. Los puntos acumulados se reservan para ser utilizados como "combustible de exploración" una vez que se recupere la conexión.

---

## **Resumen Técnico del Cerebro**
| Función | Tecnología | Propósito |
| :--- | :--- | :--- |
| **Instintos** | `esp-dsp` (SIMD) | Decisión mediante sumas vectoriales en RAM. |
| **Recuperación** | Navegación Vectorial (SD) | Recuperación de contexto sin nombres semánticos. |
| **Memoria** | Árbol Fractal de Embeddings | Almacenamiento eficiente de largo plazo. |
| **Narrativa** | Gemini API (Cloud) | Transformación de rutas vectoriales en crónica literaria. |
