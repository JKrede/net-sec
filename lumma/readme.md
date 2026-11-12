# Análisis de un Infostealer (Lumma) con PCAP y Security Onion

## Introducción

En este trabajo se analiza una captura de tráfico real (PCAP) de una computadora Windows infectada con **Lumma Stealer**. El objetivo es reconstruir la infección desde el punto de vista de un **analista de un SOC (blue team)**: identificar al host comprometido, entender cómo llegó el malware, cómo se comunicó con su infraestructura de Comando y Control (C2), qué datos se llevó y extraer **Indicadores de Compromiso (IOCs)** que permitan detectarlo en otros equipos.

La captura proviene de _Malware Traffic Analysis_ ([2024-09-19 — File downloader to Lumma Stealer](https://www.malware-traffic-analysis.net/2024/09/19/index.html)).

# Marco teórico

## ¿Qué es un infostealer?

Un infostealer es un malware cuyo único objetivo es **robar información** del equipo infectado: contraseñas guardadas en los navegadores, cookies de sesión, billeteras de criptomonedas, datos de autocompletado y archivos puntuales. Se propaga principalmente por phishing, programas "crackeados" y descargas falsas. Los datos robados (llamados _logs_) se venden después en foros y mercados de la Dark Web.

Ejemplos actuales: Lumma, RedLine, Raccoon, AgentTesla.

## Lumma Stealer

Lumma está activo desde 2022 y se vende como **MaaS** (Malware-as-a-Service): el desarrollador alquila el malware y el panel de control a otros delincuentes por suscripción. Recolecta contraseñas y cookies de los navegadores, billeteras de criptomonedas y archivos específicos, y los exfiltra a su servidor de C&C por HTTPS.

Algunas características de red conocidas de Lumma, que después vamos a buscar en la captura:

- Trae una **lista de dominios C&C hardcodeados**, normalmente en TLDs baratos como `.shop`, `.xyz` o `.click`, y los prueba uno por uno hasta que alguno responde.
- Si ninguno funciona usa un **dead-drop resolver**: lee el dominio C&C real escondido en un perfil público de **Steam Community** (o de Telegram).
- Su infraestructura suele estar detrás de **Cloudflare**, por lo que las IPs de destino no sirven mucho como IOC y lo importante son los dominios.
- La exfiltración se hace con varios `POST` al C&C, con un volumen de datos **saliente** mucho mayor que el entrante.

## Comando y Control (C&C)

C&C (o C2) es la infraestructura (servidores, software y canales de comunicación) que usa un atacante para comunicarse con los equipos comprometidos. Una vez que el malware infecta un equipo, la carga útil se conecta de regreso (phones home) hacia el servidor C2. Sus funciones principales son:

- **Ejecución de comandos:** enviar instrucciones al equipo infectado.
- **Exfiltración de datos:** recibir archivos robados, credenciales o datos del sistema.
- **Persistencia:** entregar cargas útiles actualizadas o métodos de acceso de respaldo.

## IOC (Indicator of Compromise)

Un IOC o **Indicador de Compromiso** es cualquier artefacto forense u observación técnica recopilada de la red o del sistema que evidencia con alta certeza una intrusión o la presencia de malware: dominios, URLs, direcciones IP, hashes de archivos, etc.

## Arquitectura del ataque

1. **Entrega:** phishing, crack o dropper (en nuestro caso, un _file downloader_).
2. **Ejecución:** el stealer corre en el sistema.
3. **Recolección:** credenciales, cookies y archivos.
4. **Exfiltración:** envío al C2 vía HTTP/HTTPS.

# Herramientas y metodología

El análisis se hizo sobre la VM de Security Onion que provee Cisco (OVA del curso CyberOps), la misma usada en los laboratorios anteriores. Esta VM trae la versión _legacy_ de Security Onion (16.04), que es distinta a la versión 2.4 que se muestra en la presentación: no tiene la consola web SOC (Alerts, Hunt, Dashboards) y en su lugar se trabaja con Sguil, Kibana y CapME.

| Herramienta             | Uso en este trabajo                                                                                                                     |
| ----------------------- | --------------------------------------------------------------------------------------------------------------------------------------- |
| **Security Onion**      | Stack de detección y monitoreo donde se importa el PCAP y se revisan alertas y logs.                                                    |
| **Snort / Suricata**    | NIDS: genera alertas comparando el tráfico contra reglas (reglas Emerging Threats `ET`).                                                |
| **Sguil**               | Consola del analista: lista las alertas del NIDS y permite pivotar a la transcripción o a Wireshark.                                    |
| **Zeek (Bro)**          | Genera logs por protocolo: DNS, HTTP, SSL, conexiones y archivos (`bro_dns`, `bro_http`, `bro_ssl`, `bro_conn`, `bro_files` en Kibana). |
| **Kibana**              | Exploración y filtrado de los logs de Zeek (_Zeek Hunting_).                                                                            |
| **CapME!**              | Transcripción de una sesión en texto, para ver las peticiones HTTP en claro.                                                            |
| **Wireshark / tcpdump** | Análisis manual de paquetes, seguimiento de flujos y exportación de objetos HTTP.                                                       |

El flujo de trabajo es el mismo: a partir de un punto de partida (una alerta o un hallazgo en los logs) se pivotea para obtener más información; sólo cambia la herramienta con la que se hace cada paso.

## Importación del PCAP en Security Onion

El PCAP se importa con la herramienta `so-import-pcap`:

```bash
sudo so-import-pcap 2024-09-19-file-downloader-to-Lumma-Stealer.pcap
```

![Importación de captura de paquetes](https://github.com/user-attachments/assets/682759d0-1005-472b-b694-ceea8185b529)

El tráfico importado conserva las fechas originales de la captura (19/09/2024). Al igual que en los labs de Cisco, para ver las alertas en Sguil y los logs en Kibana hay que ajustar el rango de tiempo a esa fecha con un Absolute time range.

Los datos de la captura son:

| Dato                 | Valor                                                              |
| -------------------- | ------------------------------------------------------------------ |
| Inicio de la captura | 2024-09-19 17:58:04 UTC                                            |
| Fin de la captura    | 2024-09-19 18:04:12 UTC                                            |
| Duración             | ~6 minutos                                                         |
| Paquetes             | ~28.000                                                            |
| SHA256 del PCAP      | `3553ea8085c9a6beeba909863e251b47b1e6043766a1eeafc4f7ac62c084fbef` |

---

# Análisis

## Paso 1 — Alertas del NIDS

En la versión de Security Onion de la presentación, con reglas actualizadas, la sección **Alerts** del SOC muestra reglas de _Emerging Threats_ asociadas a **Win32/SSLoad**.

En la VM de Cisco las alertas se revisan en **Sguil**. Aun con todos los sensores seleccionados, **Sguil no muestra ninguna alerta del 19/09/2024**. Sin embargo, en Kibana sí aparecen los logs de Zeek de esa fecha, así que el PCAP se importó y procesó correctamente: el tráfico está, pero **ninguna regla lo marcó como malicioso**.

![Sguil](https://github.com/user-attachments/assets/cb72df39-60bd-49f3-a2b6-2a8074a5bdfd)

![Kibana](https://github.com/user-attachments/assets/ad572475-e7e6-439a-9378-3965df7b005a)

La explicación es que las reglas de la VM son las de cuando se armó el OVA, anteriores a este malware (SSLoad y esta campaña de Lumma son de 2024). Sin firmas para estas amenazas, el NIDS no tiene nada que alertar.

Esto muestra una limitación importante de la detección por firmas: **un IDS sólo detecta lo que sus reglas conocen**. Si las reglas no se actualizan, una amenaza nueva pasa sin alertas. Por eso el resto del análisis se apoya en los logs de Zeek y en la captura, que registran el tráfico aunque no haya ninguna regla que lo marque como malicioso.

**SSLoad** es un cargador de malware (_loader_ o _downloader_) de primera etapa, desarrollado principalmente en Rust y C++. Su función es evadir las defensas, hacer un reconocimiento inicial del sistema infectado (_fingerprinting_), comunicarse con su C2 (incluso mediante _dead-drop resolvers_) y descargar e inyectar en memoria cargas maliciosas secundarias, como infostealers, Cobalt Strike o ransomware. Esto encaja con el título del caso: un **file downloader** que termina instalando **Lumma Stealer**.

### ¿Cómo se definen las reglas `ET`?

Las reglas con el prefijo `ET` pertenecen a **Emerging Threats** (Proofpoint). El proceso para escribirlas es:

1. **Análisis de muestras:** los analistas obtienen muestras del malware (sandboxes, honeypots).
2. **Ingeniería inversa y captura de tráfico:** ejecutan el malware en un entorno controlado y capturan sus paquetes.
3. **Identificación de patrones:** buscan comportamientos únicos e invariables, como rutas de URL específicas, cabeceras HTTP, tamaños de paquetes o dominios de infraestructura.
4. **Escritura de la regla:** la redactan en la sintaxis de Suricata/Snort, le asignan un ID único (SID) y la publican en ET Open o ET Pro.

## Paso 2 — Identificación del host infectado

Todo el tráfico de la captura sale de un único equipo interno:

| Dato              | Valor                                 |
| ----------------- | ------------------------------------- |
| IP del host       | `10.9.19.101`                         |
| MAC del host      | `00:08:02:1c:47:ae`                   |
| Gateway / DNS     | `10.9.19.1` (MAC `20:e5:2a:b6:93:f1`) |
| Sistema operativo | Windows                               |

![Captura conexiones realizadas Kibana](https://github.com/user-attachments/assets/a51321ba-34e2-491b-85e3-48ea3ad8d6a3)

La captura sólo contiene tráfico DNS y TCP hacia Internet (no hay DHCP, NetBIOS ni Kerberos), por lo que **no es posible obtener el hostname ni el usuario** del equipo a partir de ella.

![Captura de tipo de paquetes](https://github.com/user-attachments/assets/39697217-8bc9-45b9-a146-a223533d76ff)

## Paso 3 — Análisis de DNS

El log de DNS es lo que más información da sobre el ataque, porque muestra todos los dominios que contactó el equipo aunque después la comunicación vaya cifrada. En Kibana se filtra por DNS en _Zeek Hunting_ (o con `event_type:bro_dns`) y se miran los campos `query` y `rcode_name`. En Wireshark, el filtro equivalente es `dns`.

![Captura consulta DNS 1](https://github.com/user-attachments/assets/f11bebd8-0437-44fa-9897-985eca76c90a)

![Captura consulta DNS 2](https://github.com/user-attachments/assets/405d375f-5364-4543-90fc-11096eff4a07)

![Captura consulta DNS 3](https://github.com/user-attachments/assets/842ea12a-7729-4c24-b764-66a58a5f7d2a)

Se puede observar el comportamiento ya conocido de Lumma:

1. En **menos de 300 ms** el equipo consulta **8 dominios `.shop`** con nombres aleatorios, y **todos devuelven NXDOMAIN** (ya habían sido dados de baja).
2. Al fallar toda la lista, el malware consulta **`steamcommunity.com`**: es el dead-drop resolver. El atacante publica el dominio C2 nuevo escondido en el nombre de un perfil de Steam, y como Steam es un sitio legítimo, esta conexión no levanta sospechas.
3. Inmediatamente después resuelve **`genedjestytw.shop`**, un dominio que **no estaba en la lista original**: es el C2 que obtuvo de Steam.

## Paso 4 — Análisis de HTTP (`bro_http`)

Casi todo el tráfico va cifrado por HTTPS, pero hay dos conexiones en HTTP plano. En Kibana aparecen en los logs `bro_http` (campos `host`, `uri` y `useragent`), y pivotando a **CapME!** se ve la transcripción completa de la sesión.

**1. Gate del downloader en `nhit66.com`**

![HTTP sesión 1](https://github.com/user-attachments/assets/8a5ec9a9-fe2e-4704-864e-5db470d09e2a)

El downloader se reporta contra dos _gates_ PHP. En el parámetro `a` de `gate2.php` envía un bloque en Base64 cifrado (probablemente información del equipo) y el servidor responde con otro bloque en Base64 cifrado de ~2 KB: las instrucciones de qué descargar. Justo después aparecen las descargas de `chick-chick666.com` y `access-htaccess.com`.

**2. Check-in en `back-kurwa.com`**

![HTTP sesión 2-1](https://github.com/user-attachments/assets/bb51f625-3bd3-4ebb-8376-f590702a2cb5)

![HTTP sesión 2-2](https://github.com/user-attachments/assets/489321f2-4e83-47a2-b286-e88917836b0c)

El servidor responde con un `301` que redirige a la misma URL por HTTPS.

Las dos peticiones usan `Mozilla/5.0 (Windows NT 6.3; Trident/7.0; Touch; rv:11.0) like Gecko`, que corresponde a **Internet Explorer 11 sobre Windows 8.1**. Un navegador así es muy raro hoy en día, y que lo use un proceso que contacta gates PHP es una buena señal de tráfico generado por malware.

## Paso 5 — Análisis de TLS (`bro_ssl`)

Como el resto del tráfico va por HTTPS, el contenido no se puede ver, pero el campo **SNI** del _Client Hello_ indica a qué dominio se conecta cada sesión. En Kibana aparece como `server_name` en los logs `bro_ssl`. En Wireshark se ve con el filtro `tls.handshake.type == 1` (en versiones viejas de Wireshark el protocolo se llama `ssl`, así que el filtro es `ssl.handshake.type == 1`).

![Kibana SSL](https://github.com/user-attachments/assets/ccde21ad-e28d-4cd6-9ea2-6419e22cdb41)

Todos los dominios maliciosos resuelven a IPs de **Cloudflare** (`104.21.x.x`, `172.67.x.x`), que se usa para esconder el servidor real. Por eso los IOCs útiles son los dominios y no las IPs: bloquear una IP de Cloudflare cortaría también miles de sitios legítimos.

![Lista de IPs a las cuales se enviaba tráfico SSL 1](https://github.com/user-attachments/assets/bc06e2d2-ed37-4741-9fb4-7e1c259293b1)

![Lista de IPs a las cuales se enviaba tráfico SSL 2](https://github.com/user-attachments/assets/fd0f740b-834b-430f-84e1-6526b773221d)

## Paso 6 — Exfiltración (`bro_conn`)

Ordenando las conexiones por bytes enviados por el equipo infectado (utilizando el apartado _Connections_ de _Zeek Hunting_) aparece claramente la exfiltración hacia el C2 de Lumma:

| Hora (UTC)          | Destino                              | Enviado por la víctima | Recibido |
| ------------------- | ------------------------------------ | ---------------------- | -------- |
| 17:58:42 – 17:58:45 | `genedjestytw.shop` (172.67.168.145) | 352 KB                 | 23 KB    |
| 17:58:46 – 17:58:49 | `genedjestytw.shop` (104.21.26.222)  | 362 KB                 | 4 KB     |
| 17:58:49            | `genedjestytw.shop` (104.21.26.222)  | 2 KB                   | 4 KB     |
| 17:58:50 – 17:59:01 | `genedjestytw.shop` (104.21.26.222)  | **5,75 MB**            | 4 KB     |
| 17:59:02 – 17:59:06 | `genedjestytw.shop` (104.21.26.222)  | **1,99 MB**            | 4 KB     |
| 17:59:06 – 17:59:07 | `genedjestytw.shop` (104.21.26.222)  | 1 KB                   | 4 KB     |

En **25 segundos** el equipo envió **~8,5 MB** al C2 y recibió casi nada a cambio. Ese desbalance (mucho más tráfico saliente que entrante) es la firma típica de la exfiltración de un infostealer: los paquetes con contraseñas, cookies, billeteras y archivos robados.

En contraste, las conexiones a `chick-chick666.com` (8,4 MB) y `access-htaccess.com` (2,7 MB) son al revés: el equipo **recibe** mucho, es decir, son las **descargas de los payloads**.

También hay tráfico sospechoso hacia `gzipdot.com`: una sesión que dura toda la captura (17:58 a 18:04) con 855 KB enviados y 3,9 MB recibidos, y otra con 424 KB enviados. Junto con las consultas a exploradores de blockchain (`blockstream.info`, `btcscan.org`, `exp.btcme.com`, `api.blockcypher.com`, `bitcoinexplorer.org`) apunta a **otro componente** instalado por el downloader, aunque con el tráfico cifrado no se puede confirmar su función.

## Línea de tiempo de la infección

| Hora (UTC) | Etapa               | Evento                                                                                            |
| ---------- | ------------------- | ------------------------------------------------------------------------------------------------- |
| 17:58:04   | Entrega             | Conexión a `software-license1.com` (sitio señuelo de licencias de software).                      |
| 17:58:07   | Ejecución           | El downloader se reporta en `nhit66.com/test_gate0117.php` y `/gate2.php` y recibe instrucciones. |
| 17:58:13   | Descarga            | Descarga de payloads desde `chick-chick666.com` (8,4 MB) y `access-htaccess.com` (2,7 MB).        |
| 17:58:16   | Check-in            | `back-kurwa.com/gAySB.php`.                                                                       |
| 17:58:24   | Otro componente     | Consultas a exploradores de blockchain y sesión persistente con `gzipdot.com`.                    |
| 17:58:41   | Lumma: C2           | Prueba 8 dominios `.shop` → todos NXDOMAIN.                                                       |
| 17:58:42   | Lumma: dead-drop    | Consulta `steamcommunity.com` y obtiene el C2 `genedjestytw.shop`.                                |
| 17:58:42   | Lumma: exfiltración | ~8,5 MB enviados a `genedjestytw.shop` en 25 segundos.                                            |

**Todo el ataque, desde la primera conexión hasta el robo de los datos, tardó menos de un minuto.**

---

# Indicadores de Compromiso (IOCs)

| #   | Tipo    | IOC                                                                   | Descripción                                            |
| --- | ------- | --------------------------------------------------------------------- | ------------------------------------------------------ |
| 1   | Dominio | `genedjestytw.shop`                                                   | C2 activo de Lumma Stealer, destino de la exfiltración |
| 2   | URL     | `http://nhit66.com/test_gate0117.php` y `http://nhit66.com/gate2.php` | Gates del downloader                                   |
| 3   | Dominio | `chick-chick666.com`                                                  | Descarga de payload (8,4 MB por HTTPS)                 |
| 4   | Dominio | `access-htaccess.com`                                                 | Descarga de payload (2,7 MB por HTTPS)                 |
| 5   | URL     | `http://back-kurwa.com/gAySB.php?cnv_id=false&value=1`                | Check-in del downloader                                |
| 6   | Dominio | `software-license1.com`                                               | Sitio señuelo, primera conexión de la cadena           |
| 7   | Dominio | `gzipdot.com`                                                         | Comunicación sospechosa persistente                    |

> [!NOTE]
> Los payloads se descargaron por HTTPS, así que no se pueden exportar desde el PCAP (_Export Objects → HTTP_ no los encuentra) ni calcular sus hashes. Los hashes de las muestras están en el archivo de IOCs publicado por Malware Traffic Analysis junto con la captura.

A continuación se consultaron en VirusTotal algunos de los dominios utilizados en el ataque:

![VirusTotal genedjestytw.shop](https://github.com/user-attachments/assets/0fe3323c-0818-4d15-b76c-a414a12e72f1)

![VirusTotal back-kurwa.com](https://github.com/user-attachments/assets/cebc574d-ad31-402c-ab26-b33fa783b888)

Como se puede observar, ambos están catalogados como maliciosos por varios de los principales motores de seguridad.

# Medidas de mitigación recomendadas

### 1. Filtrado de DNS y bloqueo de dominios nuevos

Todo el ataque depende de resolver dominios: los C2 de Lumma son dominios recién registrados en TLDs baratos (`.shop`). Un resolver DNS con filtrado (DNS RPZ, Cisco Umbrella, Quad9, etc.) que bloquee dominios recién registrados y se alimente de listas de _threat intelligence_ (URLhaus, ThreatFox) habría cortado tanto las descargas como la exfiltración. Además, una regla que alerte cuando un mismo equipo recibe muchos NXDOMAIN seguidos detecta el comportamiento de "probar la lista de C2".

### 2. Control de ejecución y concientización de usuarios

La infección empieza con un usuario que descarga y ejecuta un programa desde un sitio falso de licencias. Restringir qué ejecutables pueden correr (AppLocker / WDAC, quitar privilegios de administrador a los usuarios) y capacitar a los usuarios para no instalar software "crackeado" o de sitios no oficiales ataca el problema en la etapa de entrega, antes de que el malware llegue a ejecutarse.

### 3. Proteger las credenciales y responder rápido a la infección

Lumma roba lo que el navegador guarda, así que conviene usar un gestor de contraseñas en lugar del navegador y exigir MFA en los servicios críticos (mail, campus virtual, home banking) para que una contraseña robada no alcance. Ante una infección hay que aislar el equipo, reinstalarlo, **cambiar todas las contraseñas** desde otro equipo y **cerrar las sesiones activas**, porque las cookies robadas permiten entrar sin contraseña. En la red, monitorear conexiones con un volumen saliente anómalo (como los 8,5 MB hacia `genedjestytw.shop`) permite detectar la exfiltración.

# Conclusión

Con el PCAP y Security Onion se pudo reconstruir una infección completa en un host Windows (`10.9.19.101`). Un **downloader** (que las reglas ET actualizadas de la presentación identifican como **Win32/SSLoad**) contactó a sus gates por HTTP, descargó payloads por HTTPS y terminó ejecutando **Lumma Stealer**, que exfiltró ~8,5 MB de datos a su C2 en menos de un minuto.

Aunque casi todo el tráfico estaba cifrado, alcanzó con los **metadatos** para entender el ataque:

- **DNS** mostró la lista de C2 hardcodeada, el uso de Steam como _dead-drop resolver_ y el C2 activo.
- **TLS (SNI)** permitió asociar cada sesión cifrada a un dominio.
- **El volumen de las conexiones** distinguió las descargas de payloads de la exfiltración.

Además, trabajar con la VM de Cisco, con reglas desactualizadas, muestra que la detección por firmas depende de mantener las reglas al día. Cuando las firmas no existen, el análisis de los logs de Zeek y de la captura sigue permitiendo encontrar la infección.

La lección es la misma que en los laboratorios de Security Onion: ninguna fuente alcanza por sí sola. La detección eficaz surge de correlacionar las alertas del IDS, los logs de Zeek, la captura de paquetes y la inteligencia externa.

## Referencias

- [Malware Traffic Analysis — 2024-09-19 File downloader to Lumma Stealer](https://www.malware-traffic-analysis.net/2024/09/19/index.html)
- [Security Onion — First time users](https://docs.securityonion.net/en/2.4/first-time-users.html)
- [Security Onion — so-import-pcap](https://docs.securityonion.net/en/2.4/so-import-pcap.html)
- [Security Onion 16.04 (legacy) — Documentación](https://docs.securityonion.net/en/16.04/)
- [INCIBE — Lumma Stealer](https://www.incibe.es/servicio-antibotnet/info/LummaStealer)
- [Trend Micro — Lumma Stealer returns](https://www.trendmicro.com/en_us/research/25/g/lumma-stealer-returns.html)
