# **Sistema de Vida y Muerte: El Ciclo del Campeón**

Este documento define el ciclo completo de vida de un Pet, desde su nacimiento hasta su muerte, y el proceso de renacimiento en el mundo persistente.

---

## **1. El Nacimiento: Incubación en un Mundo Existente**

Cuando un Pet nace, lo hace dentro de un mundo ya materializado. El proceso tiene dos variantes:

### **A. Primer Nacimiento (Post-Génesis)**
Tras el Ritual de Génesis, el mundo está vacío esperando vida. El usuario introduce el ADN externo y el primer Pet se materializa.

### **B. Renacimiento (Post-Muerte)**
Cuando un Pet muere, el mundo permanece intacto. El usuario puede incubar un nuevo Pet en el mismo universo utilizando el mismo ADN base.

---

## **2. La Vida: Consumo y Exploración**

Durante su vida, el Pet consume recursos del Vector de Estado `[Hambre, Energía, Social]` en cada Tick.

### **Reglas de Consumo**
* **Consumo Pasivo:** Cada Tick reduce automáticamente los valores del Vector de Estado según los Deltas definidos en el ADN.
* **Consumo por Acción:** Acciones como explorar, cazar o usar Skills consumen Energía adicional.
* **Sistema Idle:** El Pet vive autónomamente. Sin intervención, continúa su ciclo natural de consumo y recuperación.

---

## **3. La Muerte: El Fin del Campeón**

La muerte ocurre exclusivamente cuando el HP llega a **0**. No hay estados intermedios de agonía ni oportunidades de rescate.

### **Causas de Muerte**
1. **Agotamiento de HP:** Los vectores de instinto (Hambre/Energía) en niveles críticos durante demasiados Ticks reducen el HP constantemente.
2. **Eventos Fatales:** Fallos críticos en tiradas d20 pueden causar daño directo al HP (caídas, trampas, combate desfavorable).
3. **Abandono Prolongado:** El sistema es idle pero no infinito. Sin conexión a la red ni interacción por períodos muy prolongados, el Pet puede morir por agotamiento natural.

### **¿Qué se Pierde?**
| Elemento | Estado tras la Muerte |
|----------|----------------------|
| **Pet (ADN, Stats, Nivel, Skills)** | Se reinicia completamente |
| **Historia del Pet (Life Log)** | Se pierde (se borra de la SD) |
| **Memoria del Pet** | Se pierde (se borra de la SD) |
| **Mundo (Biomas, Entidades, Leyes)** | **Persiste intacto** |
| **Puntos de Deidad (DP)** | **Se conservan** |

---

## **4. El Renacimiento: Nuevo Pet, Mismo Mundo**

Tras la muerte, el sistema entra en **Modo Incubación** donde el usuario puede materializar un nuevo Pet.

### **A. Reutilización del ADN**
El mismo ADN base se utiliza para el nuevo Pet. Sin embargo, el proceso de generación tiene aleatoriedad suficiente para crear un individuo diferente:
* Los valores base de atributos pueden variar dentro de los rangos definidos por el ADN.
* El nuevo Pet tiene una "esencia" similar al anterior pero no es idéntico.

### **B. Modificación con DP**
Antes de incubar el nuevo Pet, el usuario puede gastar **Puntos de Deidad (DP)** para modificar el ADN:

| Modificación | Costo en DP | Efecto |
|--------------|-------------|--------|
| **Impulso Genético (+1 a un atributo base)** | Variable | Aumenta un stat específico antes del nacimiento |
| **Resistencia Vital** | Variable | El nuevo Pet comienza con más HP máximo |
| **Bonus de Skills** | Variable | El nuevo Pet puede comenzar con una Skill inicial |

### **C. El Hito del Renacimiento**
Al materializarse, el nuevo Pet:
1. Aparece en el mismo mundo que el anterior
2. Genera una primera entrada en su Life Log (escrita por la IA)
3. La narrativa reconoce que este Pet es un "nuevo campeón" en un mundo con historia previa

---

## **5. Persistencia del Mundo**

El mundo materializado durante el Génesis **nunca se borra por muerte del Pet**. Solo se destruye si:

1. **El usuario inicia un nuevo Génesis** (Apocalipsis voluntario)
2. **Hard Reset** (pulsación prolongada del botón físico durante el boot)

### **Huellas del Pasado**
Aunque el nuevo Pet no tiene memoria del anterior, el mundo puede contener:
* Ruinas o lugares nombrados por eventos del Pet anterior
* Bestiario que incluye criaturas encontradas previamente
* Biomas ya descubiertos y documentados

La IA tiene acceso a esta información y puede hacer referencias narrativas sutiles al "campeón que habitó este mundo antes".

---

## **6. El Abandono y sus Consecuencias**

El sistema NO pausa automáticamente cuando el usuario no interactúa. Esto tiene implicaciones importantes:

### **Escenario: Usuario Ausente**
1. El Pet continúa viviendo en modo idle
2. Sigue su ciclo natural de consumo y recuperación
3. Sin conexión a la red, la narrativa se pausa pero el Pet sigue vivo
4. En períodos extremadamente prolongados sin interacción, puede morir por negligencia

### **Al Regresar el Usuario**
* El mundo persiste
* Los DP acumulados siguen disponibles
* El usuario puede incubar un nuevo Pet inmediatamente
* La narrativa refleja el paso del tiempo (cuánto tardó el usuario en volver)

---

## **7. Resumen del Ciclo**

```
GÉNESIS → MUNDO VACÍO → INCUBACIÓN → NACIMIENTO
                                              ↓
                                          VIDA (Ticks)
                                              ↓
                                         0 HP → MUERTE
                                              ↓
                                    MUNDO PERSISTE + DP SE CONSERVAN
                                              ↓
                              NUEVA INCUBACIÓN (mismo ADN ± modificaciones DP)
                                              ↓
                                          RENACIMIENTO
                                              ↓
                                     (ciclo se repite)
```

---

## **8. Consideraciones Narrativas**

La IA debe manejar el renacimiento de forma coherente:

* El nuevo Pet no "recuerda" al anterior, pero el mundo sí
* Pueden aparecer referencias misteriosas en la narrativa ("cuentan que otro caminó por aquí...")
* Los nombres de lugares o entidades pueden reflejar el legado del Pet anterior
* El tono narrativo puede variar según cuántos Pets han muerto en el mundo
