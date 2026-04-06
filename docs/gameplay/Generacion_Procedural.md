# **Generación Procedural: IA como Arquitecto del Contenido**

Este documento define cómo la IA genera dinámicamente el contenido del juego: Skills, Clases, Biomas y entidades del mundo. No existen listas predefinidas; todo se crea en función del mundo y la narrativa.

---

## **1. Filosofía de Generación Dinámica**

MistolitoRPG no tiene contenido "hardcodeado". La IA genera:

* **Skills** según las experiencias del Pet
* **Clases** según el mundo y la narrativa acumulada
* **Biomas** según la temática elegida en Génesis
* **Entidades** según el contexto del mundo y eventos

### **Principio Fundamental**
El contenido es **único por mundo**. Dos Pets en mundos diferentes tendrán acceso a Skills, Clases y Biomas completamente distintos, generados específicamente para su contexto.

---

## **2. Generación de Skills**

### **A. ¿Cuándo se generan?**
Las Skills se generan en dos momentos:

1. **Level Up:** Cada 5 niveles, el Pet puede manifestar una nueva Skill
2. **Eventos narrativos:** La IA puede proponer una Skill basada en experiencias significativas

### **B. Estructura del Prompt para Skills**
El sistema envía a la IA un contexto estructurado:

```
CONTEXTO:
- Mundo: [Temática, Leyes, Bioma actual]
- Pet: [Nombre, Nivel, Atributos, Clase actual, Skills existentes]
- Evento reciente: [Descripción del último evento significativo]
- Estilo narrativo: [Tono del mundo]

SOLICITUD:
Genera UNA Skill padre apropiada para este Pet basándote en:
1. Su historial de experiencias
2. Sus atributos más altos
3. La temática del mundo
4. Coherecia con Skills existentes

FORMATO DE RESPUESTA:
{
  "nombre": "string",
  "descripcion": "string",
  "atributo_primario": "STR|DEX|CON|INT|WIS|CHA",
  "costo_energia": number,
  "efecto_mecanico": "string",
  "posibles_subskills": ["string", "string"]
}
```

### **C. Validación de Skills**
El sistema valida que:
* La Skill no duplique una existente
* El costo de Energía esté dentro de rangos aceptables
* La descripción sea coherente con el mundo

---

## **3. Generación de Clases**

### **A. ¿Cuándo se generan?**
Las Clases se generan:
1. **Durante el Ritual de Ascensión:** La IA propone clases disponibles basándose en el Pet actual
2. **Al alcanzar requisitos:** Cuando el Pet cumple los prerequisitos, la clase se "descubre"

### **B. Estructura del Prompt para Clases**
```
CONTEXTO:
- Mundo: [Temática, Leyes]
- Pet: [Nombre, Nivel, Atributos, Skills, Historia]
- Requisitos cumplidos: [Lista de stats que cumplen umbrales]

SOLICITUD:
Genera una lista de 2-4 clases disponibles para este Pet basándote en:
1. Sus atributos más altos
2. Su historial de acciones frecuentes
3. La temática del mundo
4. Coherencia narrativa con su historia

FORMATO DE RESPUESTA:
{
  "clases": [
    {
      "nombre": "string",
      "descripcion": "string",
      "requisitos": { "STR": number, "DEX": number, ... },
      "habilidad_pasiva": "string",
      "modificadores": { "atributo": "+/-valor" },
      "es_legendaria": boolean
    }
  ]
}
```

### **C. Clases Legendarias**
Solo accesibles con resultado de **Trascendencia** en el Ritual de Ascensión:
* Tienen requisitos más altos
* Ofrecen habilidades únicas
* La IA las genera considerando el ADN del Pet como factor adicional

---

## **4. Generación de Biomas**

### **A. ¿Cuándo se generan?**
Los Biomas se generan:
1. **Durante el Génesis:** La IA genera los biomas iniciales del mundo
2. **Exploración de fronteras:** Cuando el Pet llega al límite del mapa conocido, se generan nuevos biomas adyacentes

### **B. Estructura del Prompt para Biomas**
```
CONTEXTO:
- Mundo: [Temática elegida en Génesis]
- Leyes: [Mágico/Tecnológico/Mitológico]
- Biomas existentes: [Lista de biomas ya generados]
- Bioma actual: [Donde está el Pet ahora]

SOLICITUD:
Genera UN bioma adyacente al actual que:
1. Sea coherente con la temática del mundo
2. Tenga transición lógica con el bioma actual
3. Ofrezca nuevos desafíos o recursos
4. Expandan la narrativa de forma interesante

FORMATO DE RESPUESTA:
{
  "nombre": "string",
  "descripcion": "string",
  "modificadores": {
    "atributos": { "STR": "+/-valor", ... },
    "deltas": { "hambre": "multiplicador", ... }
  },
  "dc_base": number,
  "recursos_disponibles": ["string"],
  "entidades_posibles": ["string"],
  "color_tematico": "#hex"
}
```

### **C. Expansión del Mundo**
El mundo no es infinito procedural, sino que:
* Tiene un tamaño base generado en Génesis
* Se expande gradualmente según la exploración del Pet
* Cada expansión es coherente con el mundo existente

---

## **5. Generación de Entidades (Bestiario)**

### **A. ¿Qué son las Entidades?**
* Criaturas hostiles o neutrales
* NPCs con los que el Pet puede interactuar
* Objetos especiales o anomalías

### **B. ¿Cuándo se generan?**
* Durante el Génesis (entidades base del mundo)
* Cuando el Pet entra en un nuevo bioma
* Durante eventos narrativos especiales

### **C. Estructura del Prompt para Entidades**
```
CONTEXTO:
- Mundo: [Temática, Leyes]
- Bioma actual: [Nombre, características]
- Nivel del Pet: number
- Tipo de encuentro: [exploración|evento|combate]

SOLICITUD:
Genera una entidad apropiada para este contexto:
1. Coherente con la temática del mundo
2. Balanceada según el nivel del Pet
3. Con propósito narrativo claro

FORMATO DE RESPUESTA:
{
  "nombre": "string",
  "tipo": "hostil|neutral|amigable|objeto",
  "descripcion": "string",
  "stats": { "HP": number, "atributos": {...} },
  "comportamiento": "string",
  "recompensas": ["string"],
  "dialogo_posible": ["string"] // si aplica
}
```

---

## **6. Límites y Validaciones**

### **A. Reglas de Coherencia**
La IA debe respetar:
1. **Temática del mundo:** Un mundo tecnológico no tiene dragones de fuego
2. **Leyes definidas:** La magia funciona según las reglas del mundo
3. **Historia del Pet:** Las Skills deben relacionarse con experiencias vividas
4. **Balance de poder:** Nada debe romper el sistema d20

### **B. Rechazo de Generación**
El sistema rechaza si:
* El contenido contradice el mundo existente
* El poder está fuera de escala
* Duplica contenido existente
* El formato JSON es inválido

---

## **7. Memoria de lo Generado**

Todo lo generado se almacena en la memoria fractal:

| Contenido | Ruta en SD |
|-----------|------------|
| Skills | `/BRAIN/SKILLS/[vector]/skill.json` |
| Clases | `/BRAIN/CLASES/[vector]/clase.json` |
| Biomas | `/BRAIN/WORLD/[vector]/bioma.json` |
| Entidades | `/BRAIN/WORLD/[vector]/entidades/[nombre].json` |

### **Reutilización Inteligente**
* La IA puede hacer referencia a entidades ya generadas
* Los biomas existentes se cargan antes de generar nuevos
* Las Skills se relacionan con experiencias específicas en `/HISTORY/`

---

## **8. Ejemplo de Flujo Completo**

### **Escenario: Pet nivel 5 en mundo mágico**

1. **Level Up detectado**
2. **Sistema consulta:** "El Pet ha usado frecuentemente habilidades de fuego en el Bosque Encantado"
3. **Prompt enviado a la IA:**
   - Contexto: Pet nivel 5, INT alto, mundo mágico
   - Historia: Ha enfrentado 3 criaturas de fuego
4. **IA genera:** Skill "Toque Ígneo"
5. **Sistema valida:** No duplica, costo apropiado, coherente
6. **Skill almacenada:** `/BRAIN/SKILLS/0xA1B2.../toque_igneo.json`
7. **Narrativa actualizada:** "Las llamas del Bosque han marcado a [Nombre]..."

---

## **9. Consideraciones Técnicas**

### **Rate Limiting**
* No se generan múltiples Skills en un mismo Level Up
* Los biomas se generan bajo demanda, no en batch
* Las entidades se cachean por bioma

### **Fallback**
Si la IA no está disponible:
* El sistema usa plantillas básicas de emergencia
* La narrativa se simplifica
* Las mecánicas básicas (d20, consumo) siguen funcionando
