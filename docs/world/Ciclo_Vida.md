# **Ciclo de Vida y Cronología: The Chronos Engine**

El "Chronos Engine" es el sistema que rige la percepción del tiempo y la evolución del universo en MistolitoRPG. En este sistema, el tiempo es una función directa del ciclo de procesamiento del dispositivo.

---

## **1. La Unidad del Tiempo: El Tick de Vida**
El tiempo para el Pet avanza mediante **Ticks**. Un "Tick" representa la finalización de un ciclo completo de ejecución del motor principal del sistema (Main Loop).

### **El Ciclo del Tick:**
1.  **Percepción:** Lectura de sensores e interacciones del usuario.
2.  **Actualización Vital:** Aplicación de los deltas de necesidad (Hambre, Energía, Humor).
3.  **Resolución de Azar:** Ejecución de tiradas d20 para acciones autónomas o eventos.
4.  **Cierre y Registro:** Incremento del contador cronológico global (`Tick +1`).

---

## **2. Cronología y Persistencia**
La historia del Pet se organiza cronológicamente basándose en su **Edad de Cómputo**:
*   **Etiquetado de Eventos:** Cada hito en la bitácora de vida (Life Log) está vinculado al número de Tick en el que ocurrió.
*   **Deltas Metabólicos:** La velocidad a la que el Pet se agota o crece está ligada a la frecuencia de los Ticks, no al reloj de tiempo real (RTC).

---

## **3. Estados de Latencia y Congelación**
El universo de MistolitoRPG posee una propiedad de **Congelación Coherente**:
*   **Tiempo Muerto:** Cuando el dispositivo está apagado o en reposo, el tiempo se detiene por completo. El universo se pausa.
*   **Continuidad Divina:** Al encender el dispositivo, el Pet retoma su existencia en el Tick exacto donde se detuvo, asegurando que no ocurran muertes por negligencia fuera del tiempo de cómputo del usuario.

---

## **4. Evolución por Cómputo**
El crecimiento y envejecimiento del Pet son consecuencias de la acumulación de Ticks procesados:
*   **Madurez Genética:** El acceso a nuevas capacidades o fases de evolución depende de haber alcanzado umbrales específicos de Ticks.
*   **Hitos de Vida:** Los eventos narrativos importantes se disparan en intervalos de Ticks, creando un ritmo constante de descubrimientos.

---

## **5. Ritmo del Universo**
La frecuencia de los Ticks puede adaptarse según el estado del dispositivo:
*   **Ticks Activos:** Mayor frecuencia durante la interacción directa con el usuario (el tiempo se acelera).
*   **Ticks de Reposo:** Menor frecuencia durante la exploración pasiva (Idle), permitiendo que la historia se desarrolle a un ritmo más lento y natural.
