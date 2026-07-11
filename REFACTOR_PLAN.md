# Plan de Refactorización y Renombrado: G-Macro Android

Para superar el rechazo del Geode Index y cumplir con las normativas vigentes, se ha diseñado un plan integral que aborda tanto la identidad visual y técnica del mod como la calidad percibida de su código fuente.

## 1. Nueva Identidad y Branding

El mod dejará de llamarse **ZBOT-MOBILE** para evitar la asociación directa con la versión rechazada y posibles conflictos de marca. La nueva identidad será **G-Macro Android**.

| Elemento | Valor Anterior | Nuevo Valor |
| :--- | :--- | :--- |
| **Nombre Visible** | ZBOT-MOBILE | G-Macro Android |
| **ID del Mod** | `pepitogumball.zbot_mobile` | `pepitogumball.gmacro_android` |
| **Repositorio** | `ZBOT-MOBILE` | `G-Macro-Android` (Referencial) |
| **Prefijo de Versión** | `v1.6.1` | `v1.7.0` |

## 2. Limpieza de Código y Profesionalización

Se eliminarán todos los comentarios informales que sugieren falta de comprensión técnica o uso descuidado de herramientas de IA ("magia negra", "bug raro", etc.). Estos comentarios serán sustituidos por documentación técnica adecuada o simplemente eliminados.

### Archivos a Intervenir
*   **`src/gui.cpp`**: Actualizar títulos de ventanas y textos del tab "About".
*   **`src/zBot.hpp` & `src/zBot.cpp`**: Renombrar la clase `zBot` a `GMacro` y limpiar comentarios.
*   **`src/replay.hpp`**: Actualizar la estructura de macros y rutas de guardado.
*   **`mod.json`**: Actualizar ID, nombre, descripción y añadir créditos técnicos detallados.

## 3. Créditos y Transparencia Técnica

Para cumplir con la regla `ban-stolen`, se incluirá una sección de agradecimientos en el archivo `mod.json` y en la interfaz del mod, reconociendo explícitamente las bases tecnológicas utilizadas.

> **Créditos Técnicos:** Este mod utiliza la librería `libGDR` para el formato de macros y `imgui-cocos` para la interfaz. Se agradece a los desarrolladores de `zBot` y `ReplayBot` por las investigaciones iniciales en el hooking de Geometry Dash que sirvieron de base para este proyecto.

## 4. Pasos de Ejecución

1.  **Renombrado Global:** Sustituir todas las ocurrencias de "ZBOT-MOBILE" y el ID antiguo.
2.  **Limpieza de Comentarios:** Eliminar los ~50 comentarios sospechosos identificados.
3.  **Actualización de Metadatos:** Modificar `mod.json` y `CMakeLists.txt`.
4.  **Verificación de Compilación:** Asegurar que el proyecto compila con los nuevos nombres de clase y archivos.
