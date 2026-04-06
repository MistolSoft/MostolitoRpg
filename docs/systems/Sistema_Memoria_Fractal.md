# **Sistema de Memoria Fractal: Base de Datos Vectorial en SD**

Este documento define la arquitectura de almacenamiento de MistolitoRPG. No existen nombres humanos ni categorías semánticas en la tarjeta SD. La memoria es un espacio vectorial navegable donde la **ruta de archivo** está determinada matemáticamente por el contenido del recuerdo o habilidad.

---

## **1. Estructura de "Solo Vectores"**
El sistema de archivos ignora conceptos lingüísticos. Las carpetas se nombran utilizando representaciones hexadecimales o hashes de los **Vectores de Embedding** generados por la API externa.

### **Jerarquía de Rutas**
*   **Formato:** `/BRAIN/[TIPO]/[VECTOR_L1]/[VECTOR_L2]/[VECTOR_L3].json`
*   **Ejemplo Real:** `/BRAIN/MEM/A1B2.../C3D4.../E5F6...json`
    *   Donde `A1B2...` es la representación del vector "padre" (contexto general).
    *   Donde `E5F6...` es el vector del recuerdo específico.

---

## **2. Generación de Embeddings (API Externa)**
El ESP32 no calcula los embeddings (requiere demasiada potencia). Delega esta tarea:
1.  **Input:** El ESP32 envía el texto del evento o skill a la API de Embeddings (ej. Gemini/OpenAI).
2.  **Output:** La API devuelve el **Vector Numérico** (array de floats).
3.  **Almacenamiento:** El ESP32 usa este vector para decidir en qué carpeta de la SD guardar la información o crear una nueva rama si el vector es muy diferente a los existentes.

---

## **3. Navegación por Similitud (El Motor de Búsqueda)**
Para recordar o decidir, el Pet navega el árbol comparando matemáticas:
1.  **Vector Objetivo:** El estado actual del Pet (o su intención) se convierte en un vector.
2.  **Lectura de Índice (`index.bin`):** Cada carpeta contiene un archivo binario que lista los vectores de sus sub-carpetas.
3.  **Cálculo de Distancia:** El ESP32 compara el Vector Objetivo contra los vectores del `index.bin` usando **Similitud de Coseno**.
4.  **Salto:** Entra en la carpeta cuyo vector sea matemáticamente más cercano (mayor similitud).
5.  **Resultado:** Al llegar a un archivo final (`.json`), carga el contenido asociado a ese vector.

---

## **4. Unificación de Datos**
Todo se trata igual. Una habilidad de espada y un recuerdo de la infancia son solo vectores en el espacio latente.
*   **Skills:** `/BRAIN/SKILLS/[Vector_Agresivo]/[Vector_Fisico]/...`
*   **Mundo:** `/BRAIN/WORLD/[Vector_Geografico]/[Vector_Bioma]/...`
*   **Historia:** `/BRAIN/HISTORY/[Vector_Temporal]/[Vector_Evento]/...`

---

## **5. Soberanía Matemática**
*   **Identidad:** Los vectores se almacenan tal cual, pero la interpretación o el "seed" de navegación puede estar influenciado por el ADN del Pet, asegurando que solo él pueda navegar su propio mapa mental de forma coherente.
