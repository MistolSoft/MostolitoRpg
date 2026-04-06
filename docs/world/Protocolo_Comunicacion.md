# **Protocolo de Comunicación Narrativa: The Bridge**

El "Bridge" es el sistema de traducción conceptual que asegura que la Inteligencia Artificial actúe como un Narrador (Dungeon Master) coherente, respetando siempre las leyes físicas del mundo materializado y la esencia del Pet recuperada mediante navegación vectorial.

---

## **1. El Mensaje de Origen (La Confesión del Dispositivo)**
Cada vez que el sistema requiere una actualización narrativa, el dispositivo prepara un resumen de la "realidad" actual basado en cuatro pilares de contexto:

### **A. Identidad Genética (El Quién)**
Define la personalidad y capacidades del Pet según su ADN y sus atributos SRD (Fuerza, Inteligencia, etc.).

### **B. Contexto del Mundo (El Dónde Vectorial)**
Extrae las reglas y descripciones del entorno de las rutas vectoriales de la SD (`/BRAIN/WORLD/0xVector...`).
*   **Contenido:** Se envía a la IA el contenido de los archivos JSON recuperados de la navegación por similitud de coseno.

### **C. El Hito Mecánico y las Skills (El Qué y Cómo)**
Incluye el resultado del d20 y el contenido de la **Skill Vectorial** recuperada (`/BRAIN/SKILLS/0xVector...`).
*   **Propósito:** Que la IA sepa qué habilidad se intentó usar y qué resultado matemático se obtuvo de forma exacta.

### **D. El Pasado Cercano (History Context)**
Recupera los últimos eventos de la rama `/BRAIN/HISTORY/0xVector...` para dar continuidad narrativa.

---

## **2. La Lógica del Narrador (El Juicio de la IA)**
La Inteligencia Artificial procesa esta información bajo tres principios fundamentales:
1.  **Primacía del Dado:** El resultado matemático es inmutable. Si el d20 dicta un fallo, la historia debe reflejarlo.
2.  **Coherencia de Mundo:** No se pueden introducir elementos ajenos al contexto recuperado de las rutas vectoriales del mundo.
3.  **Voz de ADN:** El lenguaje utilizado debe reflejar los atributos y rasgos del Pet.

---

## **3. La Revelación (El Mensaje de Retorno)**
La respuesta del sistema se manifiesta en tres capas:
*   **La Crónica Literaria (Life Log):** El relato extendido guardado en `/BRAIN/HISTORY/`.
*   **La Reacción Inmediata:** Diálogo o pensamiento del Pet en el HUD.
*   **Feedback Estético:** Cambios en animaciones y colores de la interfaz.

---

## **4. Continuidad Narrativa**
El "Bridge" mantiene una línea temporal coherente al re-inyectar los últimos vectores de historia en cada nueva consulta, asegurando que la vida del Pet sea una evolución continua en el espacio latente del universo materializado.
