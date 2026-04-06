# **Matriz de Interacción de Sensores**

Este documento define la relación conceptual entre los componentes físicos del dispositivo y su impacto directo en el universo de MistolitoRPG. Cada sensor actúa como un canal de interacción entre el usuario y el Pet.

---

## **1. Sensor Inercial (IMU)**
El movimiento físico del dispositivo es la fuente de energía y acción para la supervivencia del Pet.
*   **Sacudir (Shake):** Provee un impulso de energía inmediata. Se usa para apoyar al Pet en desafíos físicos (Fuerza) o para despertarlo.
*   **Movimiento Rítmico (Caminar):** Activa el motor de exploración. El Pet avanza por el mapa virtual solo si el usuario se desplaza físicamente con el dispositivo.
*   **Rotación:** Afecta el equilibrio y la orientación del Pet en su entorno, pudiendo generar estados de mareo si el giro es brusco.

---

## **2. Pantalla Táctil (Canal de Interacción y Gracia Divina)**
Es el canal principal para la interacción afectiva, la gestión del sistema y la respuesta a las voluntades del Pet.
*   **Interacción Directa (Tap):** Tocar al Pet se interpreta como afecto físico, recargando su humor.
*   **Respuesta a Peticiones:** El Pet, a través del motor narrativo, puede realizar peticiones directas (recursos, atención, confirmación de acciones). El usuario utiliza el touch para otorgar o denegar estas peticiones, influyendo en la lealtad y el destino del Pet.
*   **Toma de Decisiones (Dilemas):** Ante situaciones complejas, el Pet puede solicitar una "Visión Divina". El usuario elige entre varias opciones táctiles para dictar el camino a seguir, lo que altera el curso de la crónica actual.
*   **Manifestaciones Aleatorias (Bendiciones):** Durante la exploración, pueden aparecer iconos o elementos "flotantes" en la terminal (objetos raras, anomalías, buffs). Un "Tap" rápido del usuario permite bendecir estos elementos, otorgando ayudas extra o desbloqueando posibilidades que el Pet no podría alcanzar por sí solo.
*   **Navegación (Swipe):** Desplazamiento entre los monitores del sistema (Vida, Crónica, ADN, Visión).

---

## **3. Cámara (Visión Digital y Ofrendas)**
La cámara no captura objetos físicos para un inventario, sino que percibe "Esencias Cromáticas" para alimentar la energía del sistema.
* **Ofrendas de Color:** El Pet solicita o detecta colores específicos en el mundo real. Al capturar una imagen con el color dominante solicitado, el usuario otorga un **Bono de Energía** al Pet.
* **Sincronización de Energía:** Estas fotos son "sacrificios visuales" que recargan la Energía necesaria para que el Pet continúe su exploración autónoma.
*   **Detección de Presencia:** La visión del usuario a través del lente mitiga la soledad del Pet, influyendo en su estado social sin intervenir directamente en la escritura de su historia.

---

## **4. Botón Físico (Intervención de Sistema)**
Dado que la interacción táctica se realiza a través de la pantalla, el botón físico se reserva para acciones de bajo nivel y estados críticos del hardware.
*   **Despertar del Sistema (Wake-up):** Es el único método para sacar al Pet de un estado de sueño profundo (Deep Sleep) y forzar el inicio de un nuevo Tick de vida.
*   **El Apocalipsis (Hard Reset):** Una pulsación prolongada durante el inicio del sistema activa el protocolo de borrado de la SD y reinicio del Génesis, permitiendo la creación de un nuevo universo.

---

---
## **5. El Sistema Idle y la Intervención Divina**

MistolitoRPG es un sistema **Idle**: el Pet vive, explora y narra su historia de forma autónoma sin requerir interacción constante del usuario.

### **Vida Autónoma**
* El Pet consume y recupera Energía de forma natural según el paso de los Ticks
* El Vector de Estado `[Hambre, Energía, Social]` se gestiona automáticamente
* El Pet toma decisiones, explora y vive su historia sin intervención

### **La Intervención del Usuario (Bonus)**
La interacción física no es obligatoria, pero otorga ventajas:

| Interacción | Efecto (Bonus) |
|-------------|----------------|
| **Sacudir (IMU)** | + Energía instantánea, efecto "Vigor" temporal |
| **Caminar (IMU)** | Acelera la exploración del mapa |
| **Tap (Touch)** | + Social, mejora ánimo del Pet |
| **Ofrenda Cromática** | + Energía significativa, mejora narrativa |

### **Abandono del Dispositivo**
* El Pet continúa viviendo y consumiendo recursos
* Sin interacción, el Pet sigue su curso natural (puede morir por negligencia)
* No hay pausa automática por inactividad
