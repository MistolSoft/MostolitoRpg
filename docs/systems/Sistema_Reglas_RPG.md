# **Sistema de Reglas RPG: Mistolito-SRD (D20 Core)**

MistolitoRPG utiliza una adaptación simplificada del **SRD 5.1 (System Reference Document)** para gestionar el crecimiento, las habilidades y la resolución de conflictos. Esto nos permite tener un sistema robusto de RPG sin infringir copyright y con mecánicas probadas.

## **1. Los 6 Atributos del ADN**
El ADN latente define el valor base (8-15) de cada atributo al nacer. Estos atributos determinan el **Modificador** que se suma a todas las tiradas de dados.

| Atributo | Afecta a... | Uso en el Pet |
| :--- | :--- | :--- |
| **Fuerza (STR)** | Daño físico, Empujar objetos. | Éxito en combates y juegos de fuerza. |
| **Destreza (DEX)** | Defensa (AC), Iniciativa. | Esquivar regaños, juegos de reflejos (IMU). |
| **Constitución (CON)** | Puntos de Vida (HP), Resistencia. | Resistencia a enfermedades y hambre. |
| **Inteligencia (INT)** | Aprendizaje, Memoria (RAG). | Capacidad de recordar más eventos en la SD. |
| **Sabiduría (WIS)** | Percepción, Intuición. | Probabilidad de encontrar objetos raros "idle". |
| **Carisma (CHA)** | Afinidad, Obediencia. | Qué tan fácil es que el Pet acepte tus Visiones. |

**Cálculo del Modificador:** `MOD = floor((Stat - 10) / 2)`
*   *Ejemplo:* Una Fuerza de 14 da un mod de +2 a las tiradas de ataque.

## **2. El Motor de Resolución: La Tirada de d20**
Cualquier acción incierta (intentar cazar, resistir un virus, obedecer una orden difícil) se resuelve con la fórmula estándar:

`d20 + Modificador de Atributo + Bono de Competencia >= Clase de Dificultad (DC)`

*   **DC (Dificultad):** 10 (Fácil), 15 (Medio), 20 (Difícil), 25 (Heroico).
*   **Bono de Competencia:** Un bono que aumenta con el nivel del Pet (+2 a nivel 1, +3 a nivel 5, etc.).

### **Implementación en el ESP32**
Usamos la entropía del sistema (`esp_random()`) para generar un número entre 1 y 20. 
*   **Crítico (20 Nat):** Éxito automático y doble efecto.
*   **Fallo (1 Nat):** Fracaso estrepitoso (ej. el pet se tropieza y pierde un poco de HP).
## **3. Puntos de Vitalidad (HP) y Supervivencia**

La existencia del Pet se mide exclusivamente mediante sus **Puntos de Vitalidad (HP)**.
* **Gestión de Daño:** Los HP se ven afectados por eventos del mundo, fallos críticos en tiradas d20 o negligencia extrema en los vectores vitales (Hambre/Energía).
* **La Muerte Binaria:** No existe un estado de agonía. En el instante en que los HP llegan a **0**, el Pet muere de forma definitiva.
* **Protocolo de Reset:** La muerte del Pet reinicia sus datos (stats, skills, historia), pero el **mundo persiste intacto**. Véase "Sistema de Vida y Muerte" para detalles.
* **Persistencia Divina:** Los **Puntos de Deidad (DP)** acumulados por el usuario se conservan, permitiendo mejorar el siguiente Pet.

## **4. Progresión: EXP y Niveles**
El ADN dicta el **Delta de Crecimiento** (qué tan rápido sube cada stat).
*   **Ganancia de EXP:** Se obtiene por sobrevivir un día, resolver eventos con éxito o interactuar con el usuario.
*   **Subida de Nivel:** Al subir de nivel, el sistema consulta el ADN. Cada stat tiene un % de probabilidad de aumentar basado en el "potencial genético". 
    *   *Ejemplo:* Un ADN con "Gen de Atleta" tendrá un 80% de probabilidad de subir STR y solo 20% en INT al subir de nivel.

## **5. El Rol del Usuario (Entidad Divina)**
Tus "Visiones" y "Bendiciones" activan la mecánica de **Ventaja (Advantage)** del SRD 5.1:
*   **Bendición/Intervención:** Cuando el usuario interviene activamente (Visión Divina), el Pet obtiene Ventaja. Esto le permite lanzar **2d20** y elegir el resultado más alto, representando la guía del Arquitecto en momentos críticos.
*   **Maldición/Desatención:** El Pet puede sufrir Desventaja (tira 2d20 y se queda con el más bajo) si sus vectores de humor o energía están en niveles críticos.

## **6. Integración con la LLM**
La LLM no decide el resultado arbitrariamente. El ESP32 le envía:
*"El Pet (STR +2) intenta mover una roca (DC 15). Resultado del dado: 14 + 2 = 16. Éxito."*
La LLM entonces genera la narrativa: *"Tu Campeón rugió con fuerza y apartó la roca del camino, revelando un tesoro escondido"*.

---

## **7. Sistema Idle: Sin Combate ni Inventario Dedicado**

MistolitoRPG es un simulador de vida narrativo, no un RPG de combate tradicional.

### **A. Combate como Acción Más**
El combate no es un sistema separado, sino una **acción narrativa más**:
* Se resuelve con tiradas d20 estándar
* No hay sistema de turnos ni iniciativa compleja
* La IA narra el resultado como parte de la historia
* Es una forma de interacción con el mundo, no el foco del juego

### **B. Sin Inventario Tradicional**
* El Pet no tiene un inventario de objetos en el sentido clásico
* Los recursos se consumen o transforman inmediatamente
* La narrativa maneja lo que el Pet "tiene" en cada momento
* Los objetos importantes se integran en la historia, no en slots

### **C. Sin Loot ni Grinding**
* No hay ciclo de "matar enemigo → obtener botín → repetir"
* Los eventos y encuentros son únicos y narrativos
* La progresión viene de experiencias vividas, no de farming
* El foco es la historia del Pet, no la acumulación de recursos
