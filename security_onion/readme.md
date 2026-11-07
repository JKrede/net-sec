# Monitoreo de seguridad de red con Security Onion

## Introducción

Este trabajo reúne tres laboratorios de análisis de incidentes realizados sobre **Security Onion**, la distribución de referencia para _Network Security Monitoring_ (NSM) y _threat hunting_. En los tres casos se parte de datos ya capturados de un ataque (capturas de paquetes, alertas de IDS y logs) y se adopta el rol de **analista de un SOC (blue team)**, no se ataca nada, se **investiga** lo que pasó para reconstruir el incidente, identificar al atacante, determinar qué datos o equipos se vieron comprometidos y proponer mitigaciones.

Los laboratorios provienen del currículo **Cisco CyberOps Associate** y están incluidos en esta carpeta:

| Lab     | Archivo                                                                       | Tema                                                         |
| ------- | ----------------------------------------------------------------------------- | ------------------------------------------------------------ |
| 27.2.12 | `27.2.12-lab---interpret-http-and-dns-data-to-isolate-threat-actor_es-XL.pdf` | Inyección SQL + exfiltración de datos vía DNS                |
| 27.2.14 | `27.2.14-lab---isolate-compromised-host-using-5-tuple_es-XL.pdf`              | Aislar hosts comprometidos con el método de 5-tuplas         |
| 27.2.16 | `27.2.16-lab---investigating-an-attack-on-a-windows-host_es-XL.pdf`           | Investigación de una infección de malware en un host Windows |

Todos los análisis se hacen sobre la máquina virtual de laboratorio de Security Onion, con datos pre-cargados y con fines educativos. El trabajo es puramente **defensivo**: análisis forense de red y de logs.

---

# ¿Qué es Security Onion?

Security Onion es una distribución Linux gratuita y de código abierto orientada a la **monitorización de seguridad, la gestión de logs y la caza de amenazas**. Su valor está en que integra, ya configuradas y correlacionadas, las principales herramientas del analista de seguridad. Los tres tipos de datos que maneja un NSM y que usaremos en los labs son:

1. **Full Packet Capture (FPC):** la captura completa de los paquetes (PCAP), que permite reconstruir cualquier conversación byte a byte.
2. **Datos de alertas (IDS):** eventos generados por reglas de detección sobre el tráfico.
3. **Datos de sesión y de transacción (logs de Zeek):** un resumen estructurado de cada conexión y de protocolos de aplicación (HTTP, DNS, FTP, SSL, etc.).

## Herramientas que integra

| Herramienta          | Rol                                                                                                                                                                      |
| -------------------- | ------------------------------------------------------------------------------------------------------------------------------------------------------------------------ |
| **Snort / Suricata** | NIDS (IDS de red): inspeccionan el tráfico contra reglas/firmas y generan alertas (p. ej. `GPL ATTACK_RESPONSE id check returned root`, `ET TROJAN Remcos RAT Checkin`). |
| **Zeek** (antes Bro) | Motor de análisis de red que genera logs por protocolo (conn, dns, http, ftp, ssl, files, x509…). Es la base del "Zeek Hunting" en Kibana.                               |
| **OSSEC**            | HIDS (IDS de host): monitorea integridad de archivos, logs del sistema, etc. (p. ej. `[OSSEC] Integrity checksum changed`).                                              |
| **Sguil**            | Consola del analista en tiempo real. Lista las alertas de IDS y permite _pivotar_ a la transcripción, a Wireshark o a Kibana desde cada evento.                          |
| **Kibana**           | Interfaz web de visualización. Forma parte del _Elastic Stack_ (Elasticsearch + Logstash + Kibana) y ofrece tableros para explorar y filtrar todos los logs.             |
| **Wireshark**        | Análisis profundo de paquetes, permite _Follow TCP Stream_ y exportar objetos/archivos del tráfico.                                                                      |
| **CapME!**           | Interfaz web para ver la transcripción de una sesión (pcap) en texto, distinguiendo origen (azul) y destino (rojo).                                                      |

## La metodología: pivoting y las 5-tuplas

El flujo de trabajo del analista en Security Onion es el **pivoting**: se empieza por una alerta en Sguil/Kibana y, sin perder el contexto, se salta a la herramienta que da más detalle (transcripción - Wireshark - Kibana) hasta reconstruir el incidente.

El hilo conductor es la 5-tupla, que identifica unívocamente una conversación de red:

(IP de origen - Puerto de origen - IP de destino - Puerto de destino - Protocolo)

Fijando esos cinco valores se "aísla" el tráfico del atacante del resto del ruido de la red. Este concepto es central en el Lab 2, pero se usa en los tres.

---

# Lab 27.2.12 — Interpretar datos HTTP y DNS para aislar al actor de la amenaza

## Escenario

El equipo de ciberseguridad determinó que hubo una vulneración durante junio de 2020 y que datos con PII (información personal identificable) pueden haber quedado expuestos. Usando Kibana, se investigan dos vectores: un ataque **HTTP (inyección SQL)** y una **exfiltración de datos por DNS**. El primer paso práctico es ampliar el rango temporal de Kibana (que por defecto muestra las últimas 24 h) a todo junio de 2020 con un _Absolute time range_.

## Parte 1 — Ataque de inyección SQL (HTTP)

Filtrando por **HTTP** en la sección _Zeek Hunting_ y revisando la primera entrada se obtiene la 5-tupla del ataque:

![Zeek Hunting](https://github.com/user-attachments/assets/b36308eb-652b-4720-8fb2-141b1ec2071f)

Pivotando con el campo `_id` a **CapME!** se ve la transcripción de la sesión. En el campo del `username` de la petición aparece la carga maliciosa:

![CapME!](https://github.com/user-attachments/assets/e6d28cc9-9042-4636-9ff2-781a2dcdf0b2)

Las palabras clave `UNION` y `SELECT` delatan una **inyección SQL**: el atacante inyecta una subconsulta para que la aplicación, en lugar de autenticar, devuelva el contenido de la tabla `credit_cards`. El `--` comenta el resto de la consulta original para saltear la validación.

Más abajo en la transcripción (respuestas del servidor, en rojo) se ve el resultado de la inyección: una lista de registros exfiltrados.

![datos robados](https://github.com/user-attachments/assets/cdd6a72d-f961-4c63-b8ab-d03227bd812f)

Ordenandolos en una tabla:

| Username (nº de tarjeta) | Password | Signature (vencimiento) |
| ------------------------ | -------- | ----------------------- |
| 4444111122223333         | 745      | 2012-03-01              |
| 7746536337776330         | 722      | 2015-04-01              |
| 8242325748474749         | 461      | 2016-03-01              |
| 7725653200487633         | 230      | 2017-06-01              |
| 1234567812345678         | 627      | 2018-11-01              |

**Conclusión de la Parte 1:** Una aplicación web que no sanea las entradas permitió, mediante inyección SQL, volcar números de tarjeta, CCV y vencimientos — una fuga grave de PII/datos financieros.

## Parte 2 — Exfiltración de datos vía DNS

Un administrador notó consultas DNS anormalmente largas con subdominios extraños. Filtrando por DNS en _Zeek Hunting_ y buscando el dominio sospechoso `example.com`, se encuentran consultas del tipo:

![Consultas DNS](https://github.com/user-attachments/assets/455cc75c-b9f4-4846-b0ad-aa87f78c29b7)

Los subdominios no son nombres legítimos sino cadenas hexadecimales: datos codificados escondidos dentro de las consultas DNS. Se exportan las 4 consultas únicas (_Export: Raw_ → CSV en `/home/analyst/Downloads`), se deja solo la parte hexadecimal y se decodifica con `xxd`:

![Parte hexadecimal](https://github.com/user-attachments/assets/8530999a-ae18-44fb-93ca-0643035b419e)

![Decodificacion](https://github.com/user-attachments/assets/8700f749-cb55-4ea9-b7d4-68cb04f40338)

Como se puede observar, el resultado decodificado es un documento confidencial:

```
CONFIDENTIAL DOCUMENT
DO NOT SHARE
This document contains information about the last security breach.
```

**Conclusión de la Parte 2:** Los subdominios **no** eran nombres DNS reales, sino un documento confidencial codificado en hexadecimal y filtrado consulta por consulta. Esto es **DNS tunneling / exfiltración por DNS**: se eligió DNS porque casi nunca está bloqueado por los firewalls (sin resolución de nombres no funciona casi nada) y rara vez se inspecciona, lo que lo hace un canal ideal y encubierto para sacar datos. Probablemente lo generó malware o un script de exfiltración en el host comprometido.

---

# Lab 27.2.14 — Aislar hosts afectados con el método de 5-tuplas

## Escenario

Tras un ataque, los usuarios ya no pueden acceder al archivo **`confidential.txt`**. Hay que revisar los logs para determinar cómo fue comprometido. El objetivo es practicar el método de las 5-tuplas para aislar al host atacante y al objetivo, y reconstruir el robo del archivo. Flujo: **Sguil → Wireshark → Kibana**.

## Parte 1 — Alertas en Sguil

Entre los eventos destaca la alerta **`GPL ATTACK_RESPONSE id check returned root`** (Alert ID 5.1). Significa que durante el ataque alguien obtuvo acceso **root**: el host `209.165.200.235` devolvió acceso root a `209.165.201.17`.

![Sguil](https://github.com/user-attachments/assets/fe7ecf45-ad7c-45ce-9c58-6c6ea8be3b57)

La 5-tupla del ataque:

| Componente           | Valor           |
| -------------------- | --------------- |
| IP origen (atacante) | 209.165.201.17  |
| Puerto origen        | 45415           |
| IP destino (víctima) | 209.165.200.235 |
| Puerto destino       | **6200**        |
| Protocolo            | TCP (6)         |

El puerto **6200** es el de la **backdoor de vsftpd** (servicio FTP vulnerable de Metasploitable); Abre un shell con privilegios de root. La transcripción confirma que el atacante ejecuta comandos Linux sobre la víctima.

## Parte 2 — Pivotar a Wireshark (Follow TCP Stream)

Siguiendo el flujo TCP se ve, en texto plano, la sesión interactiva del atacante (azul = origen/atacante, rojo = respuestas del destino):

![Wireshark Follow TCP p1](https://github.com/user-attachments/assets/13daa043-8f86-4abc-bf1c-0360a98bff81)

![Wireshark Follow TCP p2](https://github.com/user-attachments/assets/07ddbbaf-1985-4b51-bce3-aca637958319)

- Los colores indican origen (azul) y destino (rojo).
- El comando `whoami` → `root` confirma que el atacante tiene **control total (root)** sobre el equipo.
- El atacante lee `/etc/passwd` y agrega un usuario de puerta trasera con UID 0 para mantener el acceso.

## Parte 3 — Pivotar a Kibana (el robo por FTP)

Buscando la IP en Kibana y filtrando por `bro_ftp` se ven dos entradas de tráfico **FTP**. Una contiene el argumento: `ftp://209.165.200.235/./confidential.txt`

![Kibana](https://github.com/user-attachments/assets/d974827a-6988-4990-bdbc-586619be9d4c)

Es decir, el atacante usó FTP para copiar `confidential.txt` desde la víctima y luego lo borró del equipo. Pivotando al `alert_id` se ve la transcripción con las credenciales usadas para el acceso FTP.

Finalmente, filtrando por archivos (Zeek Hunting → Files) y dentro por tipo **`FTP_DATA`** se recupera el contenido del archivo robado directamente desde la reconstrucción de la transferencia: aunque el archivo fue borrado del host, su contenido quedó registrado en el tráfico capturado.

![Recuperacion del archivo robado a partir de la captura de trafico](https://github.com/user-attachments/assets/139b184c-99c9-437c-87ac-af7532c43c05)

**Conclusión del lab:** El atacante explotó la backdoor de vsftpd (puerto 6200) para obtener root, creó un usuario persistente y exfiltró `confidential.txt` por FTP. El método de 5-tuplas permitió aislar toda la conversación maliciosa, y la captura completa de paquetes (FPC) permitió recuperar un archivo que ya había sido eliminado. **Recomendación:** cambiar las contraseñas de `analyst` y `root`, eliminar el usuario backdoor, parchear/quitar el servicio FTP vulnerable y endurecer el control de acceso.

---

# Lab 27.2.16 — Investigación de un ataque a un host Windows

## Escenario

El 19 de marzo de 2019 (03/19/2019), las herramientas de NSM alertaron que un equipo Windows de la red estaba infectado con malware. (El ejercicio se basa en un caso real de _malware-traffic-analysis.net_.) Hay que responder:

- ¿A qué hora ocurrió el ataque el 2019-03-19?
- ¿Qué host Windows se infectó y quién era el usuario?
- ¿Con qué estaba infectado?

Flujo: **Parte 1 con Sguil** (alertas IDS, Wireshark, hashes, Talos) y **Parte 2 con Kibana** (hunting sobre los logs de Zeek).

## Parte 1 — Investigación con Sguil

Localizando el grupo de alertas del **19/03/2019** (todas concentradas en una franja horaria de la madrugada), se analizan en orden:

![Alertas del 03/19/2019](https://github.com/user-attachments/assets/1011b233-8130-4121-8d17-ffed1b5b6c67)

1. **Primera alerta** — actividad DNS/HTTP inicial que identifica al host afectado: el equipo infectado es **`10.0.90.215`**, con nombre de host **`bobby-tiger-pc`** (usuario _bobby-tiger_) en el dominio **`littletigers`**.
2. **Descarga del ejecutable** — en la transcripción se ve una petición `GET /test1.exe HTTP/1.1`. Mirando la respuesta, los primeros caracteres del archivo son **`MZ`** (`This program cannot be run in DOS mode`), la **firma de un ejecutable PE de Windows**. Se exporta con Wireshark (_File → Export Objects → HTTP_) y se calcula su hash:

![Obtencion hash](https://github.com/user-attachments/assets/93b3774c-203b-4c06-99ed-f8ae30f8e233)

Consultando el hash en el **Talos File Reputation** de Cisco se confirma que es **malware** conocido.

![Archivo en TFR](https://github.com/user-attachments/assets/02c6fc6f-10db-4547-92bf-1e3f1edf74e6)

3. **`Remcos RAT Checkin`** (Alert ID 5.480) — el IDS detecta, por los bytes iniciales de la comunicación, el _check-in_ de la RAT **Remcos** hacia su servidor de C2 (`103.1.184.108`, puerto **2404**, un puerto no estándar). **Remcos** es un troyano de acceso remoto (RAT) que da control total del equipo al atacante; la comunicación está **cifrada/ofuscada** para evadir la detección.
4. **Segundo ejecutable** — entre las alertas restantes se identifica la descarga de un segundo `.exe` desde otro servidor; su hash, consultado nuevamente en Talos, también corresponde a malware conocido.

## Parte 2 — Hunting con Kibana

Acotando el rango a marzo de 2019 y haciendo clic en el pico del 19 de marzo, el _Resumen de alertas NIDS_ muestra, entre otras:

- **`ET TROJAN ABUSE.CH SSL Blacklist Malicious SSL certificate detected (Dridex)`** desde las IP `31.22.4.176`, `203.45.1.75` y `115.112.43.81`. Es decir, además de Remcos, el host está comunicándose con infraestructura de **Dridex** (troyano bancario), identificada por su **certificado SSL en lista negra**.
- Revisando los paneles de **HTTP** se ven los URI descargados (incluidos archivos como `CSPCA.crl` y `ncsi.txt`, este último un test de conectividad de Windows).

![Kibana HTTP](https://github.com/user-attachments/assets/68ae6c60-5ed4-4177-b425-1d356b35072c)

- En el panel de **DNS** aparece el dominio sospechoso **`toptoptop1.online`**; enviándolo a VirusTotal se confirma como malicioso.

![Kibana DNS](https://github.com/user-attachments/assets/40a03d28-d218-445d-9553-b0dc28789989)

![Virtus total sobre el DNS](https://github.com/user-attachments/assets/cc7a5377-82cc-4332-ae77-1d2e9f70e6e2)

- Los paneles de **Kerberos/SMB/DCE-RPC** muestran el entorno Windows del dominio `littletigers` (movimiento/consulta interna).

![Kibana Kerberos](https://github.com/user-attachments/assets/a427bbd8-e0e4-46c4-83c6-226bc1dec825)

![Kibana SMB](https://github.com/user-attachments/assets/36d1df60-fc12-4390-9dbe-4166a2901a1d)

## Respuestas a las preguntas del escenario

| Pregunta                                     | Respuesta                                                                                                                                                                                             |
| -------------------------------------------- | ----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| ¿Cuándo ocurrió el ataque?                   | 19 de marzo de 2019, durante la madrugada (todas las alertas en una franja corta).                                                                                                                    |
| ¿Qué host se infectó y quién era el usuario? | `10.0.90.215` — `bobby-tiger-pc`, usuario _bobby-tiger_, dominio `littletigers`.                                                                                                                      |
| ¿Con qué estaba infectado?                   | Con la RAT **Remcos** (C2 en `103.1.184.108:2404`) y con el troyano bancario **Dridex** (certificados SSL en lista negra de abuse.ch), tras descargar ejecutables PE maliciosos verificados en Talos. |

**Conclusión del lab:** combinando las firmas del IDS (Snort), la captura completa (Wireshark + exportación del binario), la reputación de hashes (Talos), la reputación de URL (VirusTotal) y los logs de Zeek en Kibana, se reconstruyó una infección de malware multi-etapa en un host Windows corporativo.

---

# Conclusión general

Los tres laboratorios muestran el **ciclo de trabajo del análisis de seguridad de red** con una herramienta integrada como Security Onion:

1. **Detección:** una firma del IDS (Snort/Suricata) o una anomalía en los logs de Zeek levanta la primera bandera.
2. **Pivoting:** desde esa alerta se salta a la transcripción, a Wireshark o a Kibana sin perder el contexto, usando la **5-tupla** para aislar la conversación del atacante.
3. **Reconstrucción:** la **captura completa de paquetes** permite ver exactamente qué pasó —incluso recuperar un archivo ya borrado (Lab 2) o extraer el binario del malware (Lab 3)—, y los logs de Zeek dan la vista de conjunto.
4. **Enriquecimiento:** fuentes externas de _threat intelligence_ (Talos, VirusTotal, listas negras de abuse.ch) confirman la naturaleza maliciosa de hashes, URLs y certificados.

También dejan lecciones defensivas concretas: **sanear las entradas** de las aplicaciones web (Lab 1, SQLi), **inspeccionar y perfilar el tráfico DNS** para detectar tunneling/exfiltración (Lab 1 y 3), **endurecer servicios y control de acceso** y gestionar credenciales tras una brecha (Lab 2), y mantener **defensa en profundidad** con NIDS + HIDS + inteligencia de amenazas para cazar infecciones de malware (Lab 3). La idea de fondo es que **no hay una sola capa que alcance**: la detección eficaz surge de correlacionar alertas, paquetes, logs e inteligencia externa.
