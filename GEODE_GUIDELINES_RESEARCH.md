# Investigación de Normas de Geode SDK (Julio 2026)

## 1. Reglas de Rechazo Críticas (Rejection Rules)

*   **`reject-vibecoded` (Código por IA):**
    *   **Regla:** El código no debe consistir total o mayoritariamente en código generado por IA.
    *   **Razón:** Inestabilidad y falta de comprensión del desarrollador sobre el funcionamiento interno.
    *   **Uso permitido:** Herramientas de IA para aumentar la productividad están bien, pero no para "vibecoding" completo.
    *   **Acción necesaria:** Revisar el código para asegurar que no parezca generado por IA (comentarios genéricos, estructuras repetitivas, lógica ineficiente típica de LLMs) y poder explicar su funcionamiento.

*   **`ban-stolen` / `reject-not-meaningful` (Código Reutilizado):**
    *   **Regla:** Mods robados (código original de otro sin crédito) son motivo de ban permanente.
    *   **Regla de Reutilización:** Si un mod consiste mayoritariamente en código reutilizado de otro mod del índice sin crédito adecuado, será rechazado.
    *   **Acción necesaria:** Identificar todas las fuentes de inspiración (zBot, xdBot, ReplayBot mencionados en el README) y dar crédito explícito en `mod.json`, `README.md` y la UI del mod. Si el código es muy similar, explicar qué mejoras o cambios se han hecho para que sea "significativo".

## 2. Metadatos y Publicación

*   **`reject-bad-metadata`:**
    *   Debe tener nombre, descripción, icono y etiquetas adecuadas.
    *   **Importante:** Si el mod fue rechazado anteriormente, cambiar el **ID del mod** y el **Nombre** es fundamental para una nueva identidad, pero los moderadores advirtieron que cambios de ID en mods rechazados por las mismas razones también serán rechazados si no se solucionan los problemas de fondo.

*   **Source Code:**
    *   Obligatorio proporcionar el código fuente (link a GitHub).

*   **Compatibilidad:**
    *   Uso de `_spr` para IDs de nodos (ej. `node->setID("my-mod-id"_spr)`).
    *   Evitar conflictos con otros mods populares.

## 3. Conclusiones para ZBOT-MOBILE

El rechazo previo (v1.5.0) fue explícito:
1. **Reutilización de código sin crédito:** Se menciona que es casi idéntico a otros mods.
2. **Código generado por IA:** Se detectó que gran parte es "vibecoded".
3. **Advertencia:** Cambiar el ID del mod no servirá si los problemas persisten.

**Plan de Acción:**
1. **Renombrar:** Cambiar `pepitogumball.zbot_mobile` a algo nuevo (ej. `pepitogumball.g-macro-android` o similar).
2. **Créditos:** Añadir una sección de agradecimientos/créditos técnica en `mod.json` y en la UI.
3. **Refactorización:** Limpiar comentarios sospechosos de IA (ej. "// no tocar, magia negra", "// funciona? si", "// arreglado lo del bug raro") y estructurar mejor el código para que sea "legible" (`other-illegible`).
4. **Valor Añadido:** Asegurar que el mod tenga características propias o una implementación que justifique su existencia frente a los originales.
