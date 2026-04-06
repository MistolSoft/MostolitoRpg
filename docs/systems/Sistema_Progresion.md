# **Progresión Idle: Niveles Infinitos y Ascensión de Clase**

Este documento define la evolución a largo plazo del Mistolito como un juego **Idle RPG**, donde no hay un límite de nivel y el crecimiento físico se entrelaza con la expansión de su memoria fractal.

---

## **1. El Hito de Level Up (Despertar Genético)**
Cuando el Pet acumula la experiencia necesaria (EXP) mediante el paso de Ticks o resolución de eventos, se dispara el proceso de **Level Up**. Este momento es una "sincronización" entre su estado actual y su potencial genético.

### **A. Reparto de Stats (Probabilidad Logarítmica)**
El sistema realiza tiradas automáticas para cada uno de los 6 atributos SRD.
*   **Influencia del ADN:** El ADN dicta la probabilidad base de mejora de cada stat (Afinidad).
*   **Soft-Cap Logarítmico:** A medida que un stat se acerca a su límite genético, la probabilidad de subir +1 disminuye. Superar el Soft-Cap es un evento de "Rareza Épica".

### **B. Adquisición de Skills (Habilidades Jerárquicas)**
Cada 5 niveles (o hitos específicos), el Pet tiene la oportunidad de manifestar una nueva **Skill Padre** o especializar una existente en una **Sub-skill**.
*   **Estructura de Árbol:** Las habilidades siguen una lógica de "Raíz a Hoja". Una Skill Padre (ej. "Maestría en Fuego") desbloquea el acceso a Sub-skills hijas (ej. "Llama Persistente" o "Explosión Ígnea").
*   **Integración en Memoria Fractal:** Las Skills se almacenan siguiendo la ruta jerárquica de la SD: `/BRAIN/SKILLS/0xPadre/0xHijo/`. 
*   **Especialización:** Para aprender una Sub-skill, el Pet debe haber alcanzado un rango de maestría mínimo en la Skill Padre.
*   **Actualización de Vectores (v.bin):** Al desbloquear una Sub-skill, se regenera el archivo `v.bin` del nivel padre para que el Cerebro pueda elegir entre las diferentes ramas de especialización según su estado vital.

---

## **2. El Ritual de Ascensión (Trascendencia de Clase)**
La subida de clase no es automática; es un **Ritual de Deidad** que requiere la inversión de **Puntos de Deidad (DP)** acumulados por el usuario.

### **A. El Chequeo de Ascensión (Ventana Dinámica)**
La probabilidad de éxito en el ritual mejora con el nivel del Pet. A mayor madurez, el rango de éxito d20 se expande:

| Resultado d20 | Lvl 1 - 99 | Lvl 100 - 299 | Lvl 300 - 499 | Lvl 500+ | Estado del Ritual |
| :--- | :--- | :--- | :--- | :--- | :--- |
| **1 (Natural)** | Pifia | Pifia | Pifia | Pifia | **Rechazo Divino** |
| **Fallo** | **2 - 10** | **2 - 8** | **2 - 5** | **2 - 3** | **Fallo de Sincronía** |
| **Éxito** | **11 - 15** | **9 - 13** | **6 - 10** | **4 - 8** | **Éxito Estándar** |
| **Maestría** | **16 - 19** | **14 - 17** | **11 - 14** | **9 - 13** | **Éxito Superior** |
| **Trascendencia** | **20** | **18 - 20** | **15 - 20** | **14 - 20** | **Clase Legendaria** |

### **B. Requisitos y Calidad**
*   **Filtro de Atributos:** Las clases disponibles dependen de los stats actuales del Pet (ej. STR 15 para Guerrero).
*   **Filtro de Resultado:** Solo un resultado de **Trascendencia** permite acceder a las Clases Legendarias del ADN.

---

## **3. El Impacto en la Crónica (Life Log)**
Cada Level Up y cada Ritual de Ascensión son hitos narrativos mayores:
1.  **Registro de Poder:** El dispositivo notifica al **Bridge (IA)** sobre el nuevo nivel, los stats mejorados y la nueva Skill.
2.  **Narrativa de Evolución:** La IA redacta un párrafo especial en el Life Log describiendo el cambio físico y mental del Pet, integrando el nombre de la nueva habilidad en su historia personal.

---

## **4. Gestión de Memoria y Skills**
Las habilidades aprendidas no son solo números; son **nodos de decisión** en la SD:
* **Costo de Energía:** Cada Skill tiene un coste de energía para ser usada.
*   **Afinidad de Uso:** El Pet usará con más frecuencia las Skills que estén en rutas fractales más cercanas a su "Vector de Estado" habitual (personalidad).
