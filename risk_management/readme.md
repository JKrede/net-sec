# Proceso de análisis de riesgos de una radio cooperativa

## Introducción

Este trabajo continúa el análisis de la infraestructura de la radio cooperativa (en la carpeta risk_analysis). En aquel informe se relevaron la red, los nodos y las aplicaciones, componente por componente. Ahora se aplica sobre la misma organización un **proceso formal de análisis de riesgos**, siguiendo los seis pasos de la guía que compartio el profesor: "Gestión de Riesgos en Tecnologías de la Información":

| Paso | Actividad                                                         | Pregunta que responde                         |
| ---- | ----------------------------------------------------------------- | --------------------------------------------- |
| 1    | Definir el alcance                                                | ¿Qué vamos a analizar?                        |
| 2    | Identificar y valorar los activos                                 | ¿Qué debemos proteger?                        |
| 3    | Identificar las amenazas (naturales, accidentales, intencionales) | ¿Qué puede afectar a los activos?             |
| 4    | Identificar vulnerabilidades y controles existentes               | ¿Qué debilidades hay? ¿Qué controles existen? |
| 5    | Evaluar el riesgo                                                 | ¿Qué tan importante es cada riesgo?           |
| 6    | Tratar el riesgo                                                  | ¿Qué haremos con cada riesgo?                 |

Para evaluar se usa el modelo **Riesgo = Probabilidad × Impacto** en su variante **semicuantitativa** (escalas de 1 a 5). Se eligió esta variante y no la cualitativa de tres niveles porque permite medir en puntos cuánto baja el riesgo con cada medida y, así, comparar esa reducción con lo que cuesta implementarla, que es lo que se pide en las conclusiones.

# Paso 1: Definición del alcance

| Elemento         | Definición                                                                                                                                                                                                                                                                                                                                                               |
| ---------------- | ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------ |
| Organización     | Cooperativa de trabajo que gestiona una radio FM comunitaria, con estudio de radio, oficina y sala multimedia para cursos y talleres.                                                                                                                                                                                                                                    |
| Alcance          | Infraestructura de TI de la sede: servidor HPE ProLiant MicroServer Gen10 y sus servicios (OpenMediaVault, Docker/Portainer, Samba, SFTP, FileBrowser, Nextcloud), red interna y Wi-Fi, conexión a Internet, acceso remoto por VPN, PCs de la radio y dispositivos de los integrantes. También las personas que los usan y administran, y las instalaciones donde están. |
| Fuera de alcance | La infraestructura interna del proveedor de Internet y del servicio externo de streaming (se consideran solo como dependencias), el sitio web y las redes sociales de la radio.                                                                                                                                                                                          |
| Objetivos        | 1) Garantizar la **continuidad de la emisión**. 2) Proteger el **archivo de producción** (audios, videos, diseños) de pérdidas y alteraciones. 3) Proteger los **datos personales** de integrantes y alumnos (Ley 25.326).                                                                                                                                               |
| Responsables     | Integrante a cargo de sistemas (administrador del servidor) junto con el consejo de administración de la cooperativa, que aprueba el tratamiento.                                                                                                                                                                                                                        |
| Período          | Situación a octubre de 2026. Se propone revisarlo cada seis meses o ante cualquier cambio de infraestructura (por ejemplo, al cambiar el router o actualizar el servidor).                                                                                                                                                                                               |

**Supuestos:** la documentación de la cooperativa no menciona UPS, protección contra sobretensiones, ubicación física del servidor ni el sistema operativo de las PCs. Para el análisis se asume que **no hay UPS ni protección eléctrica** y que el servidor y el router están en un **lugar sin acceso restringido**, lo cual coincide con las respuestas dadas en el autodiagnóstico de INCIBE del trabajo anterior.

# Paso 2: Inventario y valoración de activos

Cada activo se valora con la tríada CIA en escala de 0 a 5 (0 sin importancia, 1 muy baja, 2 baja, 3 media, 4 alta, 5 muy alta), según el **impacto** que tendría su pérdida, alteración, divulgación o indisponibilidad, no según su precio. El valor del activo es `C + I + D`.

Criterio de criticidad:

| Valor (C+I+D) | Criticidad |
| ------------- | ---------- |
| 13 – 15       | Crítico    |
| 10 – 12       | Alto       |
| 7 – 9         | Medio      |
| 0 – 6         | Bajo       |

La suma castiga a los activos que solo importan por una dimensión: el enlace a Internet no guarda nada confidencial, pero si se cae, la radio sale del aire. Por eso se agrega una regla: **si alguna dimensión vale 5, la criticidad es como mínimo Alta**. Los activos en los que se aplicó esta regla se marcan con (\*).

## Tabla A: Inventario y valoración

| ID   | Activo                                               | Tipo           | Responsable               | C   | I   | D   | Valor | Criticidad | Comentarios                                                                              |
| ---- | ---------------------------------------------------- | -------------- | ------------------------- | --- | --- | --- | ----- | ---------- | ---------------------------------------------------------------------------------------- |
| A-01 | Archivo de producción (audios, videos, diseños)      | Información    | Producción                | 3   | 5   | 4   | 12    | Alto       | Está todo en un único disco de 1 TB del servidor, sin copia.                             |
| A-02 | Datos personales de integrantes, alumnos y contactos | Información    | Administración            | 5   | 3   | 2   | 10    | Alto       | Inscripciones a talleres, contactos, cuentas de Nextcloud. Protegidos por la Ley 25.326. |
| A-03 | Credenciales de administración y de streaming        | Información    | Sistemas                  | 5   | 4   | 3   | 12    | Alto       | OMV, Portainer, routers, Hamachi, cuenta del servicio de streaming.                      |
| A-04 | Servidor HPE MicroServer Gen10 y discos              | Hardware       | Sistemas                  | 4   | 5   | 4   | 13    | Crítico    | Aloja A-01, A-11 a A-15. Un SSD de sistema y un único disco de datos.                    |
| A-05 | PC ad-hoc de streaming                               | Hardware       | Operación                 | 2   | 4   | 5   | 11    | Alto       | Si se cae, se corta la emisión por Internet. Conectada directo al router del ISP.        |
| A-06 | PC de Operación (consola de aire)                    | Hardware       | Operación                 | 2   | 4   | 5   | 11    | Alto       | Imprescindible para la salida al aire.                                                   |
| A-07 | PCs de Oficina y de Edición/Sala multimedia          | Hardware       | Administración / Talleres | 2   | 3   | 3   | 8     | Medio      | La PC de la sala la usan talleristas externos.                                           |
| A-08 | Notebooks y celulares de integrantes (BYOD)          | Hardware       | Cada integrante           | 3   | 2   | 2   | 7     | Medio      | Equipos personales con acceso a la red, la VPN y Nextcloud.                              |
| A-09 | Router TP-Link TL-WR940N                             | Hardware (red) | Sistemas                  | 3   | 4   | 5   | 12    | Alto       | Router, DHCP y Wi-Fi de toda la red interna. Discontinuado por el fabricante.            |
| A-10 | Router/ONT del proveedor                             | Hardware (red) | Proveedor / Sistemas      | 2   | 3   | 5   | 10    | Alto       | Administrado por el ISP. Única salida a Internet.                                        |
| A-11 | OpenMediaVault 6 / Debian 11                         | Software       | Sistemas                  | 4   | 5   | 4   | 13    | Crítico    | Sistema operativo del servidor, sin soporte de seguridad.                                |
| A-12 | Docker y Portainer                                   | Software       | Sistemas                  | 4   | 5   | 3   | 12    | Alto       | Controlar Portainer equivale a ser root en el servidor.                                  |
| A-13 | Nextcloud                                            | Software       | Sistemas                  | 4   | 4   | 3   | 11    | Alto       | Sincronización con cuentas individuales.                                                 |
| A-14 | Samba y SFTP                                         | Software       | Sistemas                  | 3   | 4   | 4   | 11    | Alto       | Carpetas compartidas de uso interno.                                                     |
| A-15 | FileBrowser                                          | Software       | Talleres                  | 1   | 3   | 2   | 6     | Bajo       | Repositorio público para alumnos, con una clave compartida.                              |
| A-16 | Acceso a Internet (100 Mb)                           | Servicio       | Proveedor                 | 1   | 2   | 5   | 8     | Alto (\*)  | Del ancho de banda depende el streaming.                                                 |
| A-17 | Servicio externo de streaming                        | Servicio       | Operación / Proveedor     | 1   | 3   | 5   | 9     | Alto (\*)  | Tercero que distribuye la señal a los oyentes.                                           |
| A-18 | VPN LogMeIn Hamachi                                  | Servicio       | Sistemas                  | 4   | 3   | 3   | 10    | Alto       | Acceso remoto de puestos domiciliarios; cuenta gratuita de 5 equipos.                    |
| A-19 | Red Wi-Fi                                            | Servicio       | Sistemas                  | 3   | 3   | 4   | 10    | Alto       | Una sola red compartida por integrantes, alumnos y visitas.                              |
| A-20 | Integrante a cargo de sistemas                       | Personas       | Consejo de administración | 4   | 4   | 5   | 13    | Crítico    | Única persona que conoce y administra la infraestructura.                                |
| A-21 | Integrantes, talleristas y alumnos (usuarios)        | Personas       | Consejo de administración | 3   | 3   | 3   | 9     | Medio      | Sin capacitación en seguridad.                                                           |
| A-22 | Sede: estudio, oficina, sala, cableado y energía     | Instalaciones  | Consejo de administración | 2   | 3   | 5   | 10    | Alto       | Sin UPS ni protección eléctrica (supuesto).                                              |

**Dependencias principales:**

- La **emisión** depende de A-06 → A-05 → A-10 → A-16 → A-17, y todos ellos de la energía eléctrica (A-22). Cualquier eslabón que falle saca a la radio del aire por Internet.
- El **archivo de producción** (A-01) depende del servidor (A-04), de su sistema operativo (A-11) y de las aplicaciones que lo exponen (A-12 a A-15). Un incidente en cualquiera de ellas afecta a A-01.
- Toda la infraestructura depende del conocimiento de **una sola persona** (A-20).

# Paso 3: Identificación de las amenazas

Además de las amenazas genéricas, se tuvo en cuenta el contexto de la sede, en las sierras de Córdoba: **tormentas eléctricas fuertes y cortes de luz** frecuentes en verano, **incendios forestales** en temporada seca y **olas de calor**. Como antecedente de amenazas específicas para radios, en 2024 varias emisoras de EE.UU. sufrieron el secuestro de sus equipos de streaming, que terminaron emitiendo contenido ajeno. Para seguir amenazas actuales se pueden consultar el CSIRT Córdoba y el CSIRT Américas.

| ID    | Amenaza                                                               | Tipo        | Activos afectados                  |
| ----- | --------------------------------------------------------------------- | ----------- | ---------------------------------- |
| N-01  | Tormenta eléctrica: descargas y sobretensiones en la red eléctrica    | Natural     | A-04, A-05, A-06, A-09, A-10, A-22 |
| N-02  | Corte de energía prolongado por fenómenos climáticos                  | Natural     | Todo el hardware, A-16             |
| N-03  | Incendio                                                              | Natural     | A-22 y todo lo que contiene        |
| N-04  | Calor extremo en el lugar de los equipos                              | Natural     | A-04, A-09                         |
| AC-01 | Falla de hardware (disco, fuente, placa)                              | Accidental  | A-04, A-05, A-06                   |
| AC-02 | Borrado o sobrescritura accidental de archivos                        | Accidental  | A-01                               |
| AC-03 | Error de configuración o actualización fallida                        | Accidental  | A-09, A-11, A-12                   |
| AC-04 | Pérdida o rotura de una notebook o celular                            | Accidental  | A-08, A-03                         |
| AC-05 | Saturación del ancho de banda durante los talleres                    | Accidental  | A-16, A-17 (emisión)               |
| AC-06 | Caída del servicio del proveedor de Internet                          | Accidental  | A-16                               |
| AC-07 | Ausencia o desvinculación del único administrador                     | Accidental  | A-20 y toda la infraestructura     |
| I-01  | Malware y ransomware                                                  | Intencional | A-01, A-04, A-07, A-11             |
| I-02  | Acceso no autorizado a servicios internos desde la Wi-Fi              | Intencional | A-04, A-11 a A-15, A-01, A-02      |
| I-03  | Explotación del router por botnets o usuarios de la red               | Intencional | A-09, A-19                         |
| I-04  | Phishing y robo de credenciales                                       | Intencional | A-03, A-13, A-17                   |
| I-05  | Intercepción del tráfico en la Wi-Fi (sniffing)                       | Intencional | A-03, A-02                         |
| I-06  | Robo o manipulación física de equipos                                 | Intencional | A-04, A-09, A-07                   |
| I-07  | Acceso indebido por la VPN desde un equipo comprometido               | Intencional | A-04, A-01, A-02                   |
| I-08  | Uso indebido del repositorio público (contenido ilegal o malware)     | Intencional | A-15, A-04                         |
| I-09  | Secuestro de la emisión (toma de la cuenta o del equipo de streaming) | Intencional | A-05, A-17                         |

# Paso 4: Vulnerabilidades y controles existentes

## Vulnerabilidades

Se clasifican en las cuatro categorías de la guía: técnicas, humanas, físicas y organizativas. El detalle técnico de cada una (versiones, CVEs) está en el [informe anterior](../risk_analysis/readme.md).

| ID   | Vulnerabilidad                                                                                              | Categoría    | Activos          |
| ---- | ----------------------------------------------------------------------------------------------------------- | ------------ | ---------------- |
| V-01 | No se hacen copias de seguridad                                                                             | Organizativa | A-01, A-04       |
| V-02 | Disco de datos único, sin redundancia                                                                       | Técnica      | A-04, A-01       |
| V-03 | OMV 6 / Debian 11 sin soporte de seguridad (fin del LTS: 31/08/2026)                                        | Técnica      | A-11             |
| V-04 | Router TL-WR940N discontinuado, con vulnerabilidades explotadas activamente (CVE-2019-6989, CVE-2023-33538) | Técnica      | A-09             |
| V-05 | Red plana: alumnos y visitas comparten la red con el servidor y las PCs de la radio                         | Técnica      | A-19, A-04       |
| V-06 | Paneles de administración y aplicaciones por HTTP sin cifrar (OMV en el 80, Portainer en el 9000)           | Técnica      | A-11, A-12, A-13 |
| V-07 | Posibles contraseñas por defecto o débiles (OMV, routers) y sin 2FA                                         | Humana       | A-03, A-09, A-11 |
| V-08 | Claves compartidas: una para toda la Wi-Fi y otra para todos los alumnos en FileBrowser                     | Humana       | A-15, A-19       |
| V-09 | Sin UPS ni protección contra sobretensiones (supuesto)                                                      | Física       | A-04, A-05, A-09 |
| V-10 | Servidor y routers en un lugar sin acceso restringido (supuesto)                                            | Física       | A-04, A-09       |
| V-11 | Un único administrador, sin documentación de la infraestructura                                             | Organizativa | A-20             |
| V-12 | Integrantes sin capacitación en seguridad                                                                   | Humana       | A-21             |
| V-13 | Sin política de contraseñas ni de uso de dispositivos personales (BYOD)                                     | Organizativa | A-08, A-21       |
| V-14 | Sin control de ancho de banda (QoS)                                                                         | Técnica      | A-16             |
| V-15 | Una sola PC de streaming, sin equipo de reemplazo preparado                                                 | Técnica      | A-05             |
| V-16 | VPN que da acceso a toda la red, cuentas por equipo y dependencia de un tercero                             | Técnica      | A-18             |
| V-17 | Sin monitoreo ni registro centralizado de eventos                                                           | Técnica      | A-04, A-09       |
| V-18 | Restos de una instalación anterior de Ubuntu/Nextcloud en el disco de datos                                 | Técnica      | A-04             |
| V-19 | Un único enlace a Internet                                                                                  | Técnica      | A-16             |
| V-20 | Sin plan de continuidad ni procedimientos de recuperación                                                   | Organizativa | Todos            |
| V-21 | Sin climatización ni alertas de temperatura en el lugar de los equipos (supuesto)                           | Física       | A-04, A-09       |

## Controles existentes

| Control existente                                   | Tipo         | Eficacia     | Observación                                                                                 |
| --------------------------------------------------- | ------------ | ------------ | ------------------------------------------------------------------------------------------- |
| NAT y firewall básico de los dos routers            | Técnico      | Parcial      | Protege de Internet, pero no separa a los alumnos del servidor.                             |
| Contraseña de la Wi-Fi (WPA2-PSK)                   | Técnico      | Insuficiente | La conocen todos, incluidos alumnos y visitas.                                              |
| Usuarios y permisos por carpeta de OMV (Samba/SFTP) | Técnico      | Parcial      | Bien planteado, pero sobre un sistema sin parches y alcanzable desde la red de alumnos.     |
| Cuentas individuales en Nextcloud                   | Técnico      | Parcial      | Sin 2FA y por HTTP.                                                                         |
| VPN cifrada sin puertos abiertos en el router       | Técnico      | Parcial      | Evita exponer servicios a Internet, pero da acceso a toda la red.                           |
| PC de streaming conectada directo al router del ISP | Técnico      | Parcial      | La aísla de la red interna, pero no tiene reemplazo ni prioridad de ancho de banda.         |
| Contenedores Docker para las aplicaciones           | Técnico      | Parcial      | Aíslan las aplicaciones entre sí, pero Portainer expuesto anula ese aislamiento.            |
| Software libre desde repositorios oficiales         | Técnico      | Parcial      | Buena práctica, pero los repositorios de Debian 11 ya no publican parches.                  |
| Servicio de streaming externo                       | Organizativo | Efectivo     | El tercero absorbe la carga de oyentes; la radio no expone un servidor de streaming propio. |

En resumen, los controles existentes son pocos y en su mayoría parciales. Por eso, en el paso siguiente, el riesgo evaluado con los controles actuales es prácticamente igual al **riesgo inherente**.

# Paso 5: Evaluación del riesgo

## Escalas

| Valor | Probabilidad                                     | Impacto                                                                                                          |
| ----- | ------------------------------------------------ | ---------------------------------------------------------------------------------------------------------------- |
| 1     | Muy baja: no hay antecedentes ni se espera       | Muy bajo: inconveniente menor, sin pérdida de datos ni de aire                                                   |
| 2     | Baja: puede ocurrir en algunos años              | Bajo: molestia o demora que se resuelve en horas                                                                 |
| 3     | Media: puede ocurrir alguna vez al año           | Medio: interrupción parcial o pérdida de trabajo recuperable                                                     |
| 4     | Alta: ocurre varias veces al año                 | Alto: la radio sale del aire por horas o se pierde información importante                                        |
| 5     | Muy alta: ocurre con frecuencia o es casi seguro | Muy alto: pérdida irreversible del archivo, control total del servidor por un tercero o salida del aire por días |

`Riesgo = Probabilidad × Impacto`, con los umbrales de la guía:

| Resultado | Nivel   | Interpretación                              |
| --------- | ------- | ------------------------------------------- |
| 1 – 5     | Bajo    | Puede aceptarse o mantenerse bajo monitoreo |
| 6 – 10    | Medio   | Requiere seguimiento y algunos controles    |
| 11 – 15   | Alto    | Requiere acciones correctivas a corto plazo |
| 16 – 25   | Crítico | Requiere atención inmediata                 |

## Tabla B: Análisis de riesgos (situación actual)

Cada riesgo combina un activo, una amenaza y una o más vulnerabilidades.

| Riesgo | Activo                         | Amenaza (tipo)                                                               | Vulnerabilidades                  | P   | I   | P×I | Nivel       |
| ------ | ------------------------------ | ---------------------------------------------------------------------------- | --------------------------------- | --- | --- | --- | ----------- |
| R01    | Archivo de producción          | Falla del disco de datos (accidental)                                        | V-01, V-02, V-17                  | 3   | 5   | 15  | **Alto**    |
| R02    | Archivo de producción          | Ransomware desde un puesto vía Samba (intencional)                           | V-01, V-03, V-12                  | 3   | 5   | 15  | **Alto**    |
| R03    | Servidor y aplicaciones        | Acceso no autorizado desde la Wi-Fi (intencional)                            | V-05, V-06, V-07                  | 4   | 5   | 20  | **Crítico** |
| R04    | Router TP-Link                 | Explotación por botnets o usuarios de la red (intencional)                   | V-04, V-05, V-07                  | 3   | 4   | 12  | **Alto**    |
| R05    | Sistema del servidor           | Explotación de vulnerabilidades sin parche (intencional)                     | V-03, V-18                        | 3   | 4   | 12  | **Alto**    |
| R06    | Emisión (Internet/streaming)   | Saturación del ancho de banda en talleres (accidental)                       | V-14, V-05                        | 5   | 3   | 15  | **Alto**    |
| R07    | PC de streaming                | Falla de hardware o software (accidental)                                    | V-15, V-09                        | 3   | 4   | 12  | **Alto**    |
| R08    | Servidor, PCs de aire, routers | Tormenta eléctrica / corte de energía (natural)                              | V-09                              | 4   | 4   | 16  | **Crítico** |
| R09    | Credenciales                   | Intercepción del tráfico en la Wi-Fi (intencional)                           | V-06, V-08                        | 3   | 4   | 12  | **Alto**    |
| R10    | Servidor y datos               | Acceso por VPN desde equipo comprometido o perdido (intencional/accidental)  | V-16, V-13                        | 2   | 4   | 8   | Medio       |
| R11    | Servidor y routers             | Robo o manipulación física (intencional)                                     | V-10                              | 2   | 5   | 10  | Medio       |
| R12    | Toda la infraestructura        | Ausencia del único administrador (accidental)                                | V-11, V-20                        | 4   | 3   | 12  | **Alto**    |
| R13    | Emisión / cuenta de streaming  | Phishing y secuestro de la emisión (intencional)                             | V-07, V-12                        | 2   | 5   | 10  | Medio       |
| R14    | Datos personales               | Filtración por acceso indebido o pérdida de equipos (intencional/accidental) | V-05, V-08, V-13                  | 2   | 4   | 8   | Medio       |
| R15    | Repositorio público            | Uso indebido de la clave compartida (intencional)                            | V-08                              | 3   | 2   | 6   | Medio       |
| R16    | Acceso a Internet              | Caída del proveedor (accidental)                                             | V-19                              | 3   | 3   | 9   | Medio       |
| R17    | Sede y equipos                 | Incendio (natural)                                                           | V-01 (sin copia fuera de la sede) | 1   | 5   | 5   | Bajo        |
| R18    | Servidor y router              | Calor extremo (natural)                                                      | V-21                              | 2   | 3   | 6   | Medio       |

**Total de puntos de riesgo: 203.** Hay 2 riesgos críticos, 8 altos, 7 medios y 1 bajo.

Justificación de los valores más altos:

- **R03 (20, crítico):** cualquier persona conectada a la Wi-Fi, con la clave que conocen todos los alumnos, llega por HTTP a los paneles de OMV y Portainer. Controlar Portainer equivale a ser root del servidor. La probabilidad es alta porque no hace falta ninguna habilidad especial para intentarlo, y el impacto es el máximo.
- **R08 (16, crítico):** en las sierras de Córdoba las tormentas de verano y los cortes de luz son frecuentes. Sin UPS, un corte apaga de golpe el servidor (con riesgo de corromper el sistema de archivos) y la PC de streaming, y una descarga puede quemar los equipos.
- **R06 (15, alto):** según la propia cooperativa, el problema **ya ocurre** cada vez que hay talleres con mucho consumo. Es el único riesgo con probabilidad 5.
- **R01 y R02 (15, altos):** su impacto es máximo porque, sin backups, la pérdida del archivo de producción es **irreversible**.

## Mapa de calor

| Impacto \ Probabilidad | 1 Muy baja | 2 Baja   | 3 Media            | 4 Alta  | 5 Muy alta |
| ---------------------- | ---------- | -------- | ------------------ | ------- | ---------- |
| **5 Muy alto**         | R17        | R11, R13 | R01, R02           | **R03** |            |
| **4 Alto**             |            | R10, R14 | R04, R05, R07, R09 | **R08** |            |
| **3 Medio**            |            | R18      | R16                | R12     | R06        |
| **2 Bajo**             |            |          | R15                |         |            |
| **1 Muy bajo**         |            |          |                    |         |            |

# Paso 6: Tratamiento del riesgo

## Estrategias

Para cada riesgo se elige una de las cuatro estrategias de la guía:

- **Evitar:** eliminar la fuente del riesgo. Se aplica al router TL-WR940N, que conviene retirar porque no recibe más parches.
- **Mitigar:** implementar controles que bajen la probabilidad o el impacto. Es la estrategia para la mayoría de los riesgos, incluidos todos los críticos y altos, que según la guía no deberían aceptarse.
- **Transferir:** pasar las consecuencias a un tercero. Se evalúa contratar un seguro para los equipos electrónicos (rayos, incendio, robo).
- **Aceptar:** asumir riesgos de nivel bajo o medio cuando reducirlos cuesta más de lo que se gana.

## Controles propuestos

Los costos son **orientativos**, en dólares, para equipamiento nuevo de gama de entrada en Argentina a octubre de 2026. Las horas son de trabajo del integrante de sistemas (en una cooperativa suelen ser horas voluntarias o de trabajo interno, sin costo monetario directo).

| ID   | Control                                                                                                                                                                                                                                                                                                                                                          | Costo estimado      | Horas |
| ---- | ---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- | ------------------- | ----- |
| C-01 | **Backups 3-2-1** con BorgBackup o rsnapshot (plugins de OMV): un disco interno adicional para copias diarias y un disco externo que se rota y se guarda fuera de la sede. Prueba de restauración periódica.                                                                                                                                                     | USD 150 (2 discos)  | 6     |
| C-02 | **Endurecimiento básico**: cambiar todas las contraseñas por defecto, guardarlas en un gestor (KeePassXC), firewall de OMV para que Portainer y el panel solo se alcancen desde la PC del administrador, desactivar WPS y la administración remota de los routers, FileBrowser en solo lectura y con clave por curso, alertas por correo de SMART y temperatura. | USD 0               | 4     |
| C-03 | **Actualizar el servidor** a la versión vigente de OMV, activar las actualizaciones automáticas de seguridad y actualizar las imágenes de Docker. Borrar los restos de la instalación anterior.                                                                                                                                                                  | USD 0               | 8     |
| C-04 | **HTTPS** en todas las aplicaciones web mediante un reverse proxy, **2FA** en Nextcloud y SSH, SSH/SFTP con claves y **fail2ban**.                                                                                                                                                                                                                               | USD 0               | 6     |
| C-05 | **Router nuevo** con VLAN, firewall entre redes, QoS y WireGuard (por ejemplo, un MikroTik de gama de entrada). Redes separadas para la red interna, los talleres y la emisión; prioridad de ancho de banda para el streaming; WireGuard en lugar de Hamachi, limitado a los servicios necesarios.                                                               | USD 150             | 12    |
| C-06 | **UPS** de 1500 VA para el servidor y los routers, con apagado ordenado, y una UPS chica para la PC de streaming y la de Operación. Protector contra sobretensiones en el tablero.                                                                                                                                                                               | USD 250             | 3     |
| C-07 | **Equipo de respaldo para el streaming**: reacondicionar una PC o notebook existente con la misma configuración, lista para reemplazar a la principal.                                                                                                                                                                                                           | USD 0 – 150         | 4     |
| C-08 | **Documentar** la infraestructura y los procedimientos (altas y bajas, backup y restauración, actualizaciones, qué hacer si se corta el streaming) y **capacitar a una segunda persona**.                                                                                                                                                                        | USD 0               | 16    |
| C-09 | **Capacitación** básica de los integrantes (phishing, contraseñas, dispositivos personales) y **políticas** escritas de contraseñas y BYOD aprobadas por el consejo.                                                                                                                                                                                             | USD 0               | 6     |
| C-10 | **Seguridad física**: servidor y routers en un lugar con llave y ventilado (gabinete mural o un armario existente).                                                                                                                                                                                                                                              | USD 0 – 100         | 2     |
| C-11 | **Enlace de Internet de respaldo** 4G/5G para la PC de streaming.                                                                                                                                                                                                                                                                                                | USD 60 + USD 10/mes | 2     |
| C-12 | **Seguro** de equipos electrónicos (transferencia).                                                                                                                                                                                                                                                                                                              | Prima anual         | —     |

## Riesgo residual

| Riesgo | Riesgo actual | Estrategia                      | Controles                                       | P'  | I'  | Residual | Nivel residual        |
| ------ | ------------- | ------------------------------- | ----------------------------------------------- | --- | --- | -------- | --------------------- |
| R01    | 15 Alto       | Mitigar                         | C-01                                            | 3   | 2   | 6        | Medio                 |
| R02    | 15 Alto       | Mitigar                         | C-01, C-03, C-09                                | 2   | 2   | 4        | Bajo                  |
| R03    | 20 Crítico    | Mitigar                         | C-02, C-04, C-05                                | 1   | 5   | 5        | Bajo                  |
| R04    | 12 Alto       | **Evitar**                      | C-05 (retira el TL-WR940N)                      | 1   | 4   | 4        | Bajo                  |
| R05    | 12 Alto       | Mitigar                         | C-03                                            | 1   | 4   | 4        | Bajo                  |
| R06    | 15 Alto       | Mitigar                         | C-05 (QoS)                                      | 2   | 2   | 4        | Bajo                  |
| R07    | 12 Alto       | Mitigar                         | C-06, C-07                                      | 2   | 2   | 4        | Bajo                  |
| R08    | 16 Crítico    | Mitigar (y transferir con C-12) | C-06                                            | 2   | 2   | 4        | Bajo                  |
| R09    | 12 Alto       | Mitigar                         | C-04, C-05                                      | 1   | 4   | 4        | Bajo                  |
| R10    | 8 Medio       | Mitigar                         | C-04, C-05, C-09                                | 1   | 4   | 4        | Bajo                  |
| R11    | 10 Medio      | Mitigar (y transferir con C-12) | C-10                                            | 1   | 3   | 3        | Bajo                  |
| R12    | 12 Alto       | Mitigar                         | C-08                                            | 3   | 2   | 6        | Medio                 |
| R13    | 10 Medio      | Mitigar                         | C-02, C-09                                      | 1   | 5   | 5        | Bajo                  |
| R14    | 8 Medio       | Mitigar                         | C-05, C-09                                      | 1   | 4   | 4        | Bajo                  |
| R15    | 6 Medio       | Mitigar                         | C-02                                            | 2   | 2   | 4        | Bajo                  |
| R16    | 9 Medio       | Aceptar o mitigar               | C-11 (opcional)                                 | 3   | 1   | 3        | Bajo (9 si se acepta) |
| R17    | 5 Bajo        | Aceptar (y transferir con C-12) | C-01 (copia fuera de la sede reduce el impacto) | 1   | 4   | 4        | Bajo                  |
| R18    | 6 Medio       | Mitigar                         | C-02 (alertas de temperatura)                   | 1   | 3   | 3        | Bajo                  |

Con todos los controles, el riesgo total baja de **203 a 75 puntos (−63 %)**, y no queda ningún riesgo alto ni crítico. Los dos residuales medios se aceptan:

- **R01 (6):** el disco puede seguir fallando, pero con backup diario solo se perdería el trabajo de un día. Agregar un espejo RAID1 bajaría un poco la probabilidad de interrupción, pero no se justifica en esta etapa.
- **R12 (6):** en una cooperativa chica siempre va a haber pocas personas que sepan administrar el sistema. Con documentación y una segunda persona capacitada, la ausencia de una ya no paraliza todo.

## Políticas derivadas

Del análisis surgen las primeras políticas de seguridad que el consejo de administración debería aprobar y difundir:

1. **Contraseñas:** únicas por persona y por servicio, nunca compartidas, guardadas en el gestor de la cooperativa; 2FA donde esté disponible.
2. **Copias de seguridad:** diarias y automáticas del archivo de producción, con una copia fuera de la sede y una prueba de restauración por trimestre.
3. **Actualizaciones:** sistemas y aplicaciones con soporte vigente; actualizaciones de seguridad automáticas.
4. **Redes:** la red de talleres no accede a los sistemas internos; la administración se hace solo desde equipos autorizados.
5. **Dispositivos personales:** bloqueo de pantalla, cifrado y sistema actualizado para usar la VPN o Nextcloud; aviso inmediato ante pérdida o robo para revocar los accesos.
6. **Altas y bajas:** cuando alguien deja la cooperativa, se dan de baja sus cuentas y claves de VPN en el día.

# Conclusión

El proceso confirma lo que mostraban el autodiagnóstico de INCIBE y el relevamiento técnico. La radio tiene **2 riesgos críticos y 8 altos**, y casi ningún control efectivo. Los riesgos más graves no dependen de atacantes sofisticados: se materializan con un alumno curioso en la Wi-Fi, una tormenta de verano, un disco que falla o un taller con mucho consumo de Internet.

Para elegir qué medidas tomar, se repartió la reducción de puntos de riesgo de cada riesgo (actual − residual) en partes iguales entre los controles que la logran, y se comparó con el costo de cada control. Es una aproximación, porque los controles se superponen, pero sirve para ordenar las prioridades:

| Control | Medida                            | Costo              | Reducción (puntos) | Riesgos que reduce           | Puntos por USD    |
| ------- | --------------------------------- | ------------------ | ------------------ | ---------------------------- | ----------------- |
| C-02    | Endurecimiento básico             | USD 0              | 13                 | R03, R13, R15, R18           | Sin costo         |
| C-03    | Actualizar el servidor            | USD 0              | 12                 | R02, R05                     | Sin costo         |
| C-04    | HTTPS, 2FA, claves SSH y fail2ban | USD 0              | 10                 | R03, R09, R10                | Sin costo         |
| C-09    | Capacitación y políticas          | USD 0              | 9                  | R02, R10, R13, R14           | Sin costo         |
| C-08    | Documentación y segunda persona   | USD 0              | 6                  | R12                          | Sin costo         |
| C-05    | Router con VLAN, QoS y WireGuard  | USD 150            | **31**             | R03, R04, R06, R09, R10, R14 | 0,21              |
| C-10    | Seguridad física                  | USD 0–100          | 7                  | R11                          | 0,14 (con USD 50) |
| C-01    | Backups 3-2-1                     | USD 150            | 14                 | R01, R02, R17                | 0,09              |
| C-06    | UPS y protección eléctrica        | USD 250            | 16                 | R07, R08                     | 0,06              |
| C-07    | Equipo de respaldo para streaming | USD 0–150          | 4                  | R07                          | 0,05 (con USD 75) |
| C-11    | Internet de respaldo 4G/5G        | USD 180 (1.er año) | 6                  | R16                          | 0,03              |

Teniendo en cuenta el costo y la reducción de riesgo, se recomienda implementar las medidas en tres etapas:

1. **Etapa 1, sin costo (primer mes, unas 40 horas de trabajo):** C-02, C-03, C-04, C-08 y C-09. Bajan el riesgo total de 203 a **153 puntos** sin gastar dinero, y cortan el camino más directo al servidor (R03) aunque la red siga siendo plana. En esta etapa conviene hacer además un **primer backup manual** a un disco externo, por ejemplo uno prestado, porque es lo único que evita una pérdida irreversible mientras se compran los discos.
2. **Etapa 2, inversión principal (unos USD 550):** C-05, C-01 y C-06. Es la etapa que más reduce el riesgo: baja a **92 puntos** y elimina los riesgos críticos y altos que quedaban.
   - El **router nuevo** es la medida con mejor relación costo/beneficio de todo el plan, porque con un solo equipo resuelve la segmentación, el corte del streaming durante los talleres, el router sin soporte y el reemplazo de Hamachi, que la cooperativa ya quería.
   - Los **backups** y la **UPS** tienen menos puntos por dólar, pero son los únicos controles para riesgos de impacto irreversible (pérdida del archivo, equipos quemados). Por eso no deben postergarse aunque su eficiencia numérica sea menor.
3. **Etapa 3, complementos (hasta USD 250):** C-07 y C-10. Llevan el riesgo a **81 puntos**.

Con las tres etapas, por **hasta unos USD 800** y unas 65 horas de trabajo, el riesgo total baja un **60 %** y todos los riesgos quedan en nivel medio o bajo.

Quedan como opcionales:

- **C-11 (Internet de respaldo):** tiene la peor relación costo/beneficio y además un costo mensual. Como R16 es un riesgo medio, se puede **aceptar** por ahora y reconsiderarlo si los cortes del proveedor se vuelven frecuentes.
- **C-12 (seguro):** transferir el riesgo de rayos, incendio y robo puede ser útil para recuperar el valor de los equipos. Pero no devuelve los datos ni el aire, así que complementa a la UPS y los backups y no los reemplaza.

Por último, el análisis de riesgos es un **proceso continuo**: debe repetirse cada seis meses o ante cambios importantes, como el reemplazo del router o la actualización del servidor, y verificar que los riesgos residuales sean los esperados.

# Referencias

1. Cátedra. _Gestión de Riesgos en Tecnologías de la Información – Guía práctica para organizaciones en Argentina_. https://gist.github.com/javierbrk/a7652c4b44c059777c776921779a2207
2. [Análisis de riesgos de la infraestructura de una radio cooperativa](../risk_analysis/readme.md) (trabajo anterior), y las fuentes allí citadas: propuesta de la cooperativa para el servidor HPE, croquis de red y resultados del ADL de INCIBE.
3. ISO/IEC 27005. _Information Security Risk Management_. ISO.
4. NIST SP 800-30 Rev. 1. _Guide for Conducting Risk Assessments_. 2012.
5. MAGERIT v3.0. _Metodología de Análisis y Gestión de Riesgos de los Sistemas de Información_. Gobierno de España.
6. Ley 25.326 de Protección de Datos Personales. https://www.argentina.gob.ar/aaip/datospersonales/ley-de-proteccion-de-los-datos-personales
7. CSIRT Córdoba. https://csirtcordoba.ar/
8. CISA. _Known Exploited Vulnerabilities Catalog_. https://www.cisa.gov/known-exploited-vulnerabilities-catalog
