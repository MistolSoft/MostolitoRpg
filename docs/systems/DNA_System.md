# **Manual Maestro del Sistema de ADN: MistolitoRPG**

Este manual detalla el ciclo de vida único y obligatorio del ADN de un Mistolito: desde su creación en un script externo hasta su nacimiento en el ESP32-S3.

---

## **1. El ADN: El Código Latente del Campeón**
El ADN es el **Genoma Estático** e inmutable. Es el manual de instrucciones cifrado con el que el hardware "instancia" al Pet.

### **Anatomía de los Nodos de Instrucción**
El ADN se compone de una cadena de datos binarios (Hash de 256 bits) que se decodifica en:
*   **Nodos Fisiológicos:** Morfología (sprites), paleta cromática y escala de crecimiento.
*   **Nodos Metabólicos (Deltas del Drive):** Tasas fijas de consumo de Hambre, Energía y caída de Felicidad.
*   **Nodos de Potencial:** Límites máximos (`MAX_HP`, `MAX_LEVEL`) y techos de atributos RPG (Fuerza, Inteligencia).
*   **Nodos de Perks:** Habilidades pasivas (ej. "Metabolismo Lento", "Nervios de Acero").

---

## **2. El Block-Genome: Seguridad y Encriptación**
Para garantizar la integridad y portabilidad del Pet, el ADN se empaqueta en un **Block-Genome** cifrado mediante un script externo:
1.  **Serialización (CBOR):** Convierte el genoma en un binario ultra compacto.
2.  **Compresión (Deflate):** Minimiza el tamaño para el almacenamiento.
3.  **Cifrado (AES-256-GCM):** Encripta y firma el ADN. El GCM asegura que si se cambia un solo bit del código descargado, el nacimiento fallará.
4.  **Codificación (Base64):** El ADN final es una única cadena de texto segura que se "descarga" al ESP32.

---

## **3. Ciclo de Vida Único: Del Script a la Incubación**

### **Fase A: Creación (Script de Laboratorio)**
El único origen de un Mistolito es el script externo. Este script define el JSON maestro, lo encripta y entrega el **Hash del ADN**. Sin este hash externo, el ESP32 no puede generar vida.

### **Fase B: Nacimiento (ESP32)**
Cuando el ESP32 recibe el Hash del ADN, inicia el proceso de **Nacimiento**:
1.  **Desencriptación:** Verifica la firma con la clave compartida y accede a las instrucciones latentes.
2.  **Sincronización con el Mundo:** El sistema cruza el ADN con las leyes físicas y el entorno del mundo materializado previamente (Ritual de Génesis). Los atributos y deltas se ajustan para ser coherentes con el universo actual.
3.  **Materialización:** El ESP32 genera la estructura de archivos necesaria y vincula los assets visuales.
4.  **Hito de Despertar:** Se genera la primera entrada en la bitácora basada en la reacción del Pet ante su nuevo mundo.
