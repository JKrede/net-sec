# Listas blancas, listas negras y el futuro del antivirus

## Introducción

Toda estrategia de seguridad tiene que resolver el mismo problema: dejar pasar lo bueno y detener lo malo. Para eso hay dos enfoques opuestos:

- **Lista negra (blacklist):** se mantiene una lista de lo que se sabe que es malo y se bloquea. Todo lo demás se permite.
- **Lista blanca (whitelist):** se mantiene una lista de lo que se sabe que es bueno y se permite. Todo lo demás se bloquea.

El antivirus tradicional es el ejemplo más conocido de lista negra: una base de firmas de malware conocido. En este trabajo se resumen dos documentos que analizan los pros y contras de cada enfoque y los desafíos que enfrenta hoy el antivirus, y a partir de ellos se plantea un debate y una propuesta de estrategia para defender un terminal en Windows, macOS y Linux.

# Resumen de los documentos

## Lista blanca vs lista negra (Kevin Townsend, 2011)

El artículo parte de que la seguridad es control de acceso, y que el control de acceso empieza por la **identidad**. Pero la identidad no alcanza: también hay que conocer la **intención**. Un pasaporte prueba quién es una persona, pero no si tiene buenas o malas intenciones; por eso se complementa con listas de viajeros confiables (lista blanca) y de personas buscadas (lista negra). En informática pasa lo mismo, y la seguridad depende en gran medida de cuál de las dos listas se usa como criterio principal.

### A favor de la lista negra

- **Simplicidad conceptual:** reconocer unas pocas cosas malas, detenerlas y permitir el resto. Así nació el antivirus, cuando había muy pocos virus.
- **Facilidad administrativa:** mantener la lista se puede delegar en un tercero (la empresa antivirus), que la actualiza automáticamente. El usuario no tiene que hacer nada.
- **Los problemas de la lista blanca:** el argumento más fuerte a favor de la lista negra son las dificultades de su alternativa:
  - Mantener la lista blanca es costoso: cada instalación, actualización o parche obliga a volver a registrar ejecutables y DLLs. "Se convierte más en una cuestión administrativa que intelectual" (Dan Power, Spamina).
  - Requiere más trabajo de soporte y choca con la costumbre de los usuarios de instalar lo que quieran (David Harley, ESET).
  - Aplicada a gran escala (por ejemplo, exigir un "certificado de salud" para conectarse a internet, como propuso Microsoft) es inviable por la enorme variedad de configuraciones, y perjudica a negocios como el comercio electrónico, que prefieren aceptar algún fraude antes que rechazar clientes legítimos (Jennifer Gilburg, Intel).

### A favor de la lista blanca

- **Las amenazas desconocidas:** una lista negra solo detecta lo que ya conoce. Una amenaza de día cero, por definición, no está en la lista y se deja pasar como si fuera buena. Los ataques dirigidos actuales están diseñados específicamente para evadir las listas negras (Henry Harrison, Detica).
- **El volumen de malware:** con más de 2 millones de piezas nuevas de malware por mes (en 2011), mantener una lista de todo lo malo deja de ser realista. La lista blanca se centra en la **confianza** en lugar de en las amenazas (Alan Bentley, Lumension).
- **El conjunto bueno es más chico:** lo que uno quiere ejecutar es un conjunto mucho más pequeño que lo que no quiere. "¿A quién dejás entrar a tu casa? No llevás una lista de todos los malos del mundo; solo dejás entrar a las personas en las que confiás" (Harry Sverdlove, Bit9).
- **Reputación en la nube:** el costo de mantener la lista blanca se puede reducir con bases de datos de reputación de software, combinadas con políticas propias para el software interno.

### La postura de la industria antivirus

Los propios fabricantes de antivirus reconocen el valor de la lista blanca:

- ESET nunca recomendó el antivirus como única capa. La lista blanca funciona en empresas con una cultura de TI restrictiva (privilegios limitados), y bloquea de un plumazo la mayoría de las amenazas basadas en ingeniería social.
- Kaspersky ya combinaba ambos enfoques: los archivos desconocidos se analizan en la nube y, según el resultado, se agregan a la lista negra (protegiendo a todos los usuarios) o a la lista blanca (reduciendo falsos positivos y acelerando los escaneos).

### Conclusión del autor

| Aspecto                      | Lista blanca                            | Lista negra                                   |
| ---------------------------- | --------------------------------------- | --------------------------------------------- |
| Ante algo desconocido        | Lo bloquea (**falla segura**)           | Lo permite (**falla insegura**)               |
| Amenazas de día cero         | Protege                                 | No protege                                    |
| Esfuerzo administrativo      | Alto, y crece con el tamaño de la lista | Mínimo, delegable en un tercero               |
| Escalabilidad                | Funciona bien con listas chicas         | Pierde eficiencia a medida que la lista crece |
| Experiencia del usuario      | Restrictiva                             | Transparente                                  |
| Elimina malware ya instalado | No                                      | Sí                                            |

La lista blanca es conceptualmente la mejor solución, porque su posición por defecto es segura. Pero la lista blanca es mejor cuando es chica y la lista negra es mejor cuando es chica, y en el mundo real ninguna de las dos situaciones ocurre. La conclusión es simple: **se necesitan ambas**.

## ¿Qué sigue para los antivirus? (Deep Secure, 2021)

El documento sostiene que el enfoque basado en la detección, sobre el que se construyeron los antivirus en los últimos 40 años, ya no alcanza para frenar a los atacantes actuales.

### El dilema de los datos

Las organizaciones necesitan intercambiar información con clientes, proveedores y empleados remotos, y esa información siempre viaja envuelta en **datos**: archivos con un formato determinado. Los formatos de archivo son complejos: un documento de Office no solo contiene el texto, sino también metadatos, fuentes, historial de cambios, macros, áreas de relleno sin usar, etc. Esa complejidad:

- Exige aplicaciones complejas para interpretarlos, y el software complejo tiene errores explotables.
- Ofrece lugares donde esconder contenido malicioso.
- Incluye funcionalidades, como las macros, que permiten ejecutar código.

Por eso casi todos los ciberataques de los titulares comienzan con un archivo, como un PDF o un documento de Office. Según Kaspersky, el 70% de las infecciones provienen de vulnerabilidades de Microsoft Office.

### Los límites de la detección

- En 2012, Brian Krebs observó que los antivirus detectaban en promedio solo el **25%** del malware. Hoy la eficacia está entre el **82% y el 96%**, pero con un promedio del 90%, **1 de cada 10 ataques pasa**.
- El antivirus basado en detección solo puede detectar lo que ya "vio" antes. El sandboxing ayuda, pero los atacantes aprendieron a detectarlo. La inteligencia artificial y el aprendizaje automático solo detectan más rápido amenazas parecidas a las ya conocidas.
- Los atacantes desarrollan técnicas pensadas específicamente para evadir la detección:

| Técnica                    | Descripción                                                                                 |
| -------------------------- | ------------------------------------------------------------------------------------------- |
| Ataques de día cero        | Vulnerabilidades desconocidas (IBM estimó 4 nuevos ataques de día cero por segundo en 2017) |
| Malware sin archivos       | Código que se ejecuta en memoria usando software legítimo del sistema (ej. PowerShell)      |
| Archivos polimórficos      | Un archivo que se comporta como una imagen pero, abierto como HTML, ejecuta un script       |
| Esteganografía de imágenes | Un exploit codificado en los valores de los píxeles de una imagen                           |

Además, los ataques son cada vez más **dirigidos**: se usan una sola vez contra una organización específica y se modifican para la siguiente, así que nunca llegan a estar en una lista negra.

El documento muestra la fragilidad de la detección con un experimento sobre el troyano **Emotet**, analizado con los principales motores antivirus:

| Muestra                                               | Motores que la detectan |
| ----------------------------------------------------- | ----------------------- |
| Emotet original                                       | ~75%                    |
| Agregando un comentario `HELLO WORLD` a la macro      | 34%                     |
| Copiando la macro modificada a otro documento de Word | 20%                     |

Cambios triviales, que no alteran en nada el comportamiento del malware, alcanzan para evadir a la mayoría de los antivirus.

Los costos de un ataque exitoso pueden ser enormes: el ransomware **NotPetya** destruyó en 2017 unas 49.000 computadoras y 3.500 servidores de Maersk, con un costo de entre 250 y 300 millones de dólares, y un ataque de ransomware a **Travelex** en 2019 contribuyó a la quiebra de la empresa.

### La propuesta: transformación (CDR)

Como alternativa, el documento propone un antivirus basado en la **transformación**, conocido en la industria como **CDR** (Content Disarm and Reconstruction). En lugar de buscar la amenaza, busca la **información válida** y entrega solo eso. Se aplica a todos los archivos, haya amenaza o no, en un enfoque de **confianza cero** (zero trust). Sus ventajas son:

- No depende de la detección, así que el atacante no puede "adivinar" cómo evadirla.
- No necesita firmas ni actualizaciones constantes.
- Elimina incluso amenazas de día cero o completamente desconocidas.

Para que sea viable debe ser rápido, transparente para el usuario (entregar un archivo idéntico a la vista), fácil de integrar con los antivirus existentes y, en entornos muy críticos, verificar el contenido por hardware (FPGA) y usar flujos de datos unidireccionales.

La conclusión del documento es que una defensa eficaz necesita **ambas**: la detección para las amenazas conocidas y la transformación para todo lo demás.

# Mi propuesta personal para defender un terminal

## Principios

La pregunta "¿antivirus sí o no?" está mal planteada si se la toma como una decisión aislada. Ninguna herramienta protege sola un terminal, así que la propuesta se basa en **defensa en profundidad**, combinando:

1. **Reducir la superficie de ataque:** actualizaciones al día, desinstalar lo que no se usa, usuario sin privilegios de administrador para el uso diario.
2. **Lista blanca como primera línea:** solo se ejecuta software de orígenes confiables (firmado, de repositorios o tiendas oficiales). Es la medida que más amenazas detiene, porque bloquea por defecto.
3. **Lista negra y comportamiento como segunda línea:** un antivirus/EDR para detectar lo conocido, lo que se cuela por programas confiables y para limpiar infecciones.
4. **Desconfiar de los archivos recibidos:** no habilitar macros, abrir los adjuntos en vista protegida o en el visor del navegador, y en organizaciones que reciben muchos archivos, aplicar CDR en el correo.
5. **Asumir que algo puede fallar:** copias de seguridad fuera del equipo (lo único que salvó a Maersk fue un controlador de dominio que estaba desconectado por un corte de luz), cifrado del disco y registro de eventos.

## En Windows: antivirus sí

Windows es el sistema más atacado por su cuota de mercado, por la cantidad de software que se instala desde orígenes arbitrarios y porque es el destino principal de los archivos de Office maliciosos.

- **Antivirus:** Definitivamente sí. **Microsoft Defender**, que viene incluido, es suficiente para la mayoría de los usuarios; no hace falta uno de terceros. Lo importante es que esté activo y actualizado.
- **Lista blanca:** **Smart App Control** (Windows 11) en equipos personales, que solo permite ejecutar aplicaciones firmadas o con buena reputación. En empresas, **AppLocker** o **WDAC** (Windows Defender Application Control).
- **Reglas ASR** (Attack Surface Reduction) de Defender, por ejemplo bloquear que Office cree procesos hijos o ejecute macros de archivos descargados de internet, que ataca justo el vector que describe el documento de Deep Secure.
- Cuenta de usuario estándar (no administrador), Windows Update automático, BitLocker y copias de seguridad.

## En Linux

Linux el modelo predominante ya es la lista blanca: el software se instala desde los **repositorios** de la distribución, firmados con las claves de la distribución, y no descargando ejecutables de cualquier sitio.

- **En un equipo de escritorio o un servidor propio, antivirus no:** Porque prácticamente no hay malware de escritorio para Linux que se propague por archivos, y un antivirus tradicional aporta poco. Son más importantes:
  - Instalar solo desde repositorios oficiales y mantener el sistema actualizado.
  - Mínimo privilegio: usar `sudo` solo cuando haga falta, no ejecutar scripts descargados como root (`curl ... | sudo bash`).
  - Firewall, servicios mínimos expuestos, autenticación por clave y herramientas como **fail2ban** (ver el trabajo de 2FA y fail2ban).
  - En entornos exigentes, lista blanca de ejecución con **fapolicyd**, y montar `/tmp` y `/home` con `noexec`.
- **En un servidor que recibe o distribuye archivos para otros sistemas, antivirus sí:** un servidor de correo o de archivos compartidos con usuarios Windows debería escanear los archivos (por ejemplo con **ClamAV**), no para proteger al Linux, sino para no ser el medio por el que el malware llega a los clientes.

# Conclusión

La discusión entre lista blanca y lista negra no tiene un ganador absoluto. La lista negra es cómoda y delegable, pero por diseño deja pasar todo lo que no conoce, y los atacantes actuales se aseguran de ser siempre desconocidos: el experimento con Emotet muestra que un cambio trivial alcanza para evadir a la mayoría de los antivirus. La lista blanca falla de forma segura, pero su costo de administración la hace difícil de aplicar en entornos que cambian todo el tiempo o donde no hay quien la mantenga.

Los dos documentos, con diez años de diferencia, llegan a la misma conclusión: se necesitan ambas. La evolución posterior les dio la razón: los sistemas operativos actuales integran las dos cosas, con listas blancas basadas en firmas de código (Gatekeeper, Smart App Control, repositorios firmados) y antivirus integrados que detectan también por comportamiento (Defender, XProtect).

Por eso la respuesta a "¿antivirus sí o no?" es: **sí, pero no solo**. En Windows es indispensable, aunque alcanza con el integrado; en macOS ya viene incorporado y lo importante es no desactivar las protecciones del sistema; en Linux importa más el modelo de repositorios y el mínimo privilegio, salvo en servidores que distribuyen archivos. En todos los casos, el antivirus es una capa más dentro de una estrategia que empieza por reducir lo que se puede ejecutar, mantener todo actualizado, trabajar con los mínimos privilegios y asumir que, tarde o temprano, algo va a pasar.

# Referencias

1. Townsend, K. _Whitelisting Vs Blacklisting_. Agosto de 2011. https://kevtownsend.wordpress.com/2011/08/24/whitelisting-vs-blacklisting/ (traducción provista por la cátedra).
2. Deep Secure. _What's Next for Antivirus?_ Junio de 2021 (traducción provista por la cátedra).
3. NCSC (National Cyber Security Centre, Reino Unido). _Pattern: Safely Importing Data_. 2021.
