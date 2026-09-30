# Trabajo con archivos de registro de eventos (logs)

## ¿Qué es un archivo de registro?

Un archivo de registro (log) es un archivo donde una computadora va anotando los eventos que ocurren en ella. Esos eventos los pueden generar los programas, los procesos en segundo plano, los servicios, las transacciones entre servicios o el propio sistema operativo.

No existe un formato único: cada aplicación define qué registra, cómo lo registra y dónde lo guarda. Respetar las convenciones es responsabilidad del desarrollador, y lo ideal es que la documentación del software explique dónde y cómo se generan sus logs.

Desde el punto de vista de la seguridad, los logs son una de las principales fuentes de información para:

- Detectar actividad sospechosa (intentos fallidos de login, accesos a recursos inexistentes, escaneos).
- Reconstruir un incidente después de que ocurrió (análisis forense).
- Correlacionar eventos de distintas fuentes (servidor web, sistema operativo, firewall, IDS).

> [!IMPORTANT]
> Para poder correlacionar eventos entre distintos equipos es fundamental que los relojes estén sincronizados (por ejemplo con NTP). Si cada equipo tiene una hora distinta, es muy difícil reconstruir en qué orden ocurrieron las cosas.

### Anatomía de una entrada de log

Tomando como ejemplo una entrada generada por Apache:

```
[Wed Mar 22 11:23:12.207022 2017] [core:error] [pid 3548:tid 4682351596] [client 209.165.200.230] File does not exist: /var/www/apache/htdocs/favicon.ico
```

| Parte         | Valor                                                     | Significado                                      |
| ------------- | --------------------------------------------------------- | ------------------------------------------------ |
| Marca de hora | `Wed Mar 22 11:23:12.207022 2017`                         | Momento exacto en que ocurrió el evento          |
| Tipo          | `core:error`                                              | Tipo/severidad del evento (en este caso, error)  |
| PID           | `pid 3548:tid 4682351596`                                 | Proceso (e hilo) de Apache que atendió el evento |
| Cliente       | `209.165.200.230`                                         | Dirección IP del cliente que hizo la solicitud   |
| Descripción   | `File does not exist: /var/www/apache/htdocs/favicon.ico` | Qué pasó                                         |

¿Qué sucedió según la entrada anterior?

El 22 de marzo de 2017 a las 11:23:12, el cliente con IP `209.165.200.230` le pidió al servidor Apache el archivo `favicon.ico` (el ícono que los navegadores piden automáticamente para mostrar en la pestaña). Como ese archivo no existe en `/var/www/apache/htdocs/`, Apache registró un evento de tipo error indicando que el archivo no se encontró. Es un error inofensivo, típico de un sitio que no definió un favicon.

## Entorno del laboratorio

Todo el laboratorio se realiza sobre la VM CyberOps Workstation, el cual tiene instalado nginx como servidor web.

# Parte 1: Descripción general de los archivos de registro

## Paso 1: Log de un servidor web

Se muestra un log de ejemplo de un servidor web ubicado en `/var/log`:

```bash
cat /var/log/logstash-tutorial.log
```

```
83.149.9.216 - - [04/Jan/2015:05:13:42 +0000] "GET /presentations/logstash-monitorama-2013/images/kibana-search.png HTTP/1.1" 200 203023 "http://semicomplete.com/presentations/logstash-monitorama-2013/" "Mozilla/5.0 (Macintosh; Intel Mac OS X 10_9_1) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/32.0.1700.77 Safari/537.36"
```

![logstash-tutorial.log](https://github.com/user-attachments/assets/a8d04164-6487-40d6-bca7-e04985b5fc1c)

Cada línea tiene el formato de log de acceso (Combined Log Format), que se desglosa así:

| Campo               | Valor de ejemplo                          |
| ------------------- | ----------------------------------------- |
| IP del cliente      | `83.149.9.216`                            |
| Identidad / usuario | `- -` (no disponible)                     |
| Marca de hora       | `04/Jan/2015:05:13:42 +0000`              |
| Solicitud           | `GET /.../kibana-search.png HTTP/1.1`     |
| Código de estado    | `200` (OK)                                |
| Tamaño respuesta    | `203023` bytes                            |
| Referer             | Página desde la que se hizo la solicitud  |
| User-Agent          | Navegador y sistema operativo del cliente |

¿La salida anterior se puede considerar una transacción web? ¿Por qué tiene un formato diferente al de la entrada de Apache?

Sí. Cada línea representa una transacción web completa: un cliente (`83.149.9.216`) hace una solicitud HTTP `GET` por un recurso y el servidor responde con un código de estado `200` y una cantidad de bytes.

El formato es distinto porque se trata de otro tipo de log. La entrada de Apache del punto anterior pertenece a un log de errores, que registra fallas del servidor (tipo de error, proceso, descripción). La salida de `cat` corresponde a un log de acceso, que registra cada solicitud atendida (IP, método, recurso, código de estado, tamaño, referer y user-agent). Además, el formato de cada log lo define la aplicación que lo genera y su configuración, por lo que dos servidores (o dos logs del mismo servidor) pueden registrar la información de manera diferente.

## Paso 2: Log del sistema operativo

Por convención, Linux guarda sus logs en el directorio `/var/log`. Uno de ellos es `/var/log/messages`, donde se registran eventos generales del sistema: conexión de un USB, una placa de red que se conecta o desconecta, intentos fallidos de login como root, etc.

El enunciado propone leerlo con `more` (que permite avanzar de a poco: `INTRO` línea por línea, `ESPACIO` página por página, `q` para salir), usando `sudo` porque el archivo pertenece a root:

```bash
sudo more /var/log/messages
```

Sin embargo, en la versión de la VM CyberOps Workstation utilizada, el archivo `/var/log/messages` no existe:

![Error messages](https://github.com/user-attachments/assets/515d30a4-ac83-4ab7-a118-75e5cd62b56e)

Esto se debe a que la VM está basada en Arch Linux, que por defecto no instala un demonio syslog clásico (rsyslog o syslog-ng), que es quien escribe `/var/log/messages`. En su lugar, todos los eventos del sistema los recolecta systemd-journald (ver Parte 3) y se guardan en formato binario en `/var/log/journal`. Es un buen ejemplo de lo que plantea la Parte 2: la ubicación de los logs depende de cómo esté armado cada sistema.

Para ver los mismos eventos del kernel se pueden usar:

```bash
sudo journalctl -k      # mensajes del kernel guardados por journald, con fecha y hora reales
sudo dmesg -T           # buffer del kernel en memoria (solo el arranque actual)
```

![Lectura de journalctl](https://github.com/user-attachments/assets/f471678c-dc84-49ed-b768-ee50f010fb3c)

La salida de ejemplo del enunciado para `/var/log/messages` es la siguiente:

```
Mar 20 08:34:38 secOps kernel: [6.149910] random: crng init done
Mar 20 08:34:40 secOps kernel: [8.280667] floppy0: no floppy controllers found
Mar 20 14:28:29 secOps kernel: [21239.566409] pcnet32 0000:00:03.0 enp0s3: link down
Mar 20 14:28:33 secOps kernel: [21243.404646] pcnet32 0000:00:03.0 enp0s3: link up, 100Mbps, full-duplex
Mar 20 14:28:35 secOps kernel: [21245.536961] pcnet32 0000:00:03.0 enp0s3: link down
...
Mar 22 06:01:40 secOps kernel: [0.000000] Linux version 4.8.12-2-ARCH ...
```

A diferencia del log del servidor web, acá todos los eventos son del sistema operativo: mensajes del kernel, de la placa de red (`enp0s3`), de la CPU, del arranque, etc.

Un usuario reporta que las operaciones de red estuvieron lentas alrededor de las 4:20 am del 19 de mayo. ¿Hay pruebas de ello en las entradas de arriba?
No. Las entradas mostradas corresponden al 20 y 22 de marzo, no hay ningún registro del 19 de mayo, por lo que con este fragmento no se puede confirmar ni descartar el problema.

Lo que sí se ve es un problema de red real el 20 de marzo entre las 14:28:29 y las 14:29:05: la interfaz `enp0s3` alterna varias veces entre `link down` y `link up`. Una interfaz que se cae y levanta constantemente (link flapping) provoca exactamente el síntoma de "red lenta", así que si el reporte hubiera sido de ese día y horario, esas líneas serían la evidencia. Esto también refuerza la importancia de tener los relojes sincronizados: sin marcas de hora confiables no se puede relacionar el reporte del usuario con los eventos del log.

# Parte 2: Ubicar archivos de registro en sistemas desconocidos

Es común que un analista de seguridad tenga que trabajar en equipos que no configuró y donde no sabe dónde guarda sus logs cada servicio. En esta parte se simula ese escenario buscando los logs de nginx paso a paso.

### Paso 1: Consultar la documentación

El primer paso ante un software desconocido es leer su documentación:

```bash
man nginx
```

La página del manual confirma que nginx soporta logs (sección `DEBUGGING LOG`), y en la sección `FILES` indica qué archivos utiliza:

![Seccion FILES del manual de nginx](https://github.com/user-attachments/assets/5a4cb095-ffeb-481b-996f-558bc3557194)

Esto indica lo siguiente:

| Archivo                    | Función                                  |
| -------------------------- | ---------------------------------------- |
| `/run/nginx.pid`           | Guarda el ID del proceso master de nginx |
| `/etc/nginx/nginx.conf`    | Archivo de configuración principal       |
| `/var/log/nginx/error.log` | Log de errores                           |

En el enunciado estas rutas aparecen como variables (`%%PID_PATH%%`, `%%CONF_PATH%%`, `%%ERROR_LOG_PATH%%`), porque la página del manual original es una plantilla y las rutas reales se definen al compilar nginx. En la versión instalada en la VM, el paquete ya reemplazó esas variables por las rutas definitivas, así que el manual directamente indica dónde buscar.

Aun así, conviene no quedarse solo con el manual: esas son las rutas por defecto de compilación, pero la configuración o los parámetros con los que se inicia el servicio pueden cambiarlas.

### Paso 2: Verificar que nginx esté corriendo

```bash
ps ax | grep nginx
```

```
  415 ?      Ss   0:00 nginx: master process /usr/bin/nginx -g pid /run/nginx.pid; error_log stderr;
  416 ?      S    0:00 nginx: worker process
 1207 pts/0  S+   0:00 grep nginx
```

![procesos de nginx corriendo](https://github.com/user-attachments/assets/7affe501-d89b-4b27-ab37-da6e26a36471)

La salida confirma que nginx está en ejecución (un proceso master y un worker) y además muestra los parámetros con los que se inició:

- El PID se guarda en `/run/nginx.pid`.
- Los errores se redirigen a la terminal (`error_log stderr`).

Como no aparece la ubicación del log de acceso, hay que buscarla en la configuración.

### Paso 3: Buscar el archivo de configuración

Por convención, los archivos de configuración están en `/etc`:

```bash
ls /etc/
ls -l /etc/nginx/
```

Dentro de `/etc` aparece la carpeta `nginx`, y dentro de ella el archivo `nginx.conf`:

```bash
cat /etc/nginx/nginx.conf
```

```nginx
user www-data;
worker_processes auto;
pid /run/nginx.pid;
include /etc/nginx/modules-enabled/*.conf;

events {
	worker_connections 768;
	# multi_accept on;
}

http {

	##
	# Basic Settings
	##

	sendfile on;
	tcp_nopush on;
	types_hash_max_size 2048;
	# server_tokens off;

	# server_names_hash_bucket_size 64;
	# server_name_in_redirect off;

	include /etc/nginx/mime.types;
	default_type application/octet-stream;

	##
	# SSL Settings
	##

	ssl_protocols TLSv1 TLSv1.1 TLSv1.2 TLSv1.3; # Dropping SSLv3, ref: POODLE
	ssl_prefer_server_ciphers on;

	##
	# Logging Settings
	##

	access_log /var/log/nginx/access.log;
	error_log /var/log/nginx/error.log;

	##
	# Gzip Settings
	##

	gzip on;

	# gzip_vary on;
	# gzip_proxied any;
	# gzip_comp_level 6;
	# gzip_buffers 16 8k;
	# gzip_http_version 1.1;
	# gzip_types text/plain text/css application/json application/javascript text/xml application/xml application/xml+rss text/javascript;

	##
	# Virtual Host Configs
	##

	include /etc/nginx/conf.d/*.conf;
	include /etc/nginx/sites-enabled/*;
}


#mail {
#	# See sample authentication script at:
#	# http://wiki.nginx.org/ImapAuthenticateWithApachePhpScript
#
#	# auth_http localhost/auth.php;
#	# pop3_capabilities "TOP" "USER";
#	# imap_capabilities "IMAP4rev1" "UIDPLUS";
#
#	server {
#		listen     localhost:110;
#		protocol   pop3;
#		proxy      on;
#	}
#
#	server {
#		listen     localhost:143;
#		protocol   imap;
#		proxy      on;
#	}
#}
...
```

Las directivas de log están comentadas (`#`), es decir que nginx no tiene una ubicación personalizada y está usando los valores por defecto.

### Paso 4: Buscar en `/var/log`

Siguiendo la convención de Linux, se lista `/var/log`:

```bash
ls -l /var/log/
```

Entre los archivos aparecen, además de `messages`, otros logs importantes del sistema como `auth.log` (autenticaciones), `kernel.log`, `daemon.log`, `faillog`, `wtmp`/`btmp` (logins exitosos/fallidos) y un directorio `nginx`. Como pertenece a otro usuario, hay que listarlo con `sudo`:

```bash
sudo ls -l /var/log/nginx
```

```
-rw-r----- 1 http log   0 May 18 17:53 access.log
-rw-r----- 1 http log 175 May  6 09:42 access.log.1.gz
-rw-r----- 1 http log 593 May  5 16:58 access.log.2.gz
-rw-r----- 1 http log 193 Jul 19  2018 access.log.3.gz
-rw-r----- 1 http log 425 Apr 19  2018 access.log.4.gz
```

![Salida de ls en /var/log/nginx](https://github.com/user-attachments/assets/cbe416fa-9cf3-44c3-8a6e-9b1531740cef)

Ahí están los logs de acceso de nginx.

> [!NOTE]
> Los archivos `.gz` los genera el servicio de rotación de logs (logrotate). Para que un log no crezca indefinidamente, periódicamente se toma el archivo actual, se comprime y se renombra (`access.log.1.gz`, `access.log.2.gz`, ...), y se crea un `access.log` nuevo y vacío para las entradas nuevas. Por eso el `access.log` actual puede estar vacío.

# Parte 3: Monitorear archivos de registro en tiempo real

Herramientas como `cat`, `more`, `less` o `nano` sirven para leer un log, pero no para ver lo que se va escribiendo en tiempo real. Para eso se usan herramientas como `tail` y `journalctl`.

## Paso 1: El comando `tail`

`tail` muestra el final de un archivo; por defecto, las últimas 10 líneas.

```bash
sudo tail /var/log/nginx/access.log        # últimas 10 líneas
sudo tail -n 5 /var/log/nginx/access.log   # últimas 5 líneas
```

```
127.0.0.1 - - [22/May/2017:12:49:53 -0400] "GET / HTTP/1.1" 200 612 "-" "Mozilla/5.0 (X11; Linux i686; rv:50.0) Gecko/20100101 Firefox/50.0"
127.0.0.1 - - [22/May/2017:13:01:55 -0400] "GET /favicon.ico HTTP/1.1" 404 169 "-" "Mozilla/5.0 (X11; Linux i686; rv:50.0) Gecko/20100101 Firefox/50.0"
```

En estas entradas se ven distintos códigos de estado:

| Código | Significado                                                  |
| ------ | ------------------------------------------------------------ |
| `200`  | OK, se devolvió la página                                    |
| `304`  | Not Modified, el navegador ya la tenía en caché              |
| `404`  | Not Found, el recurso no existe (de nuevo, el `favicon.ico`) |

### Monitoreo en vivo con `tail -f`

La opción `-f` (follow) hace que `tail` no termine, sino que se quede esperando y muestre cada línea nueva que se agregue al archivo:

```bash
sudo tail -f /var/log/nginx/access.log
```

Con `tail -f` corriendo, se abre el navegador de la VM y se ingresa a `127.0.0.1`. Cada vez que se carga o actualiza la página aparece una entrada nueva en la terminal:

```
127.0.0.1 - - [23/Mar/2017:9:48:36 -0400] "GET / HTTP/1.1" 200 612 "-" "Mozilla/5.0 (X11; Linux i686; rv:50.0) Gecko/20100101 Firefox/50.0"
```

![tail sobre los logs de nginx](https://github.com/user-attachments/assets/d4b7a4c1-c10d-4e4e-8b7e-5bf5e6c0cc8f)

Como nginx escribe en ese archivo en el mismo momento en que se hace la solicitud, queda confirmado que `/var/log/nginx/access.log` es efectivamente el log de acceso que usa nginx. Se sale con `Ctrl+C`.

## Paso 2: Herramienta adicional, `journalctl`

### systemd y journald

La VM usa systemd como sistema de inicio (init). El proceso init es el primero que arranca el kernel (PID 1) y es, directa o indirectamente, el padre de todos los demás procesos. Además de levantar los servicios, systemd define cómo se administran los servicios y los logs del sistema.

El servicio de logging de systemd es systemd-journald, que guarda los eventos en archivos binarios de tipo append-only (solo se pueden agregar entradas, no modificarlas). Para leerlos se usa `journalctl`. Journald puede convivir con otros sistemas de logging clásicos como syslog o rsyslog.

### Uso básico

```bash
journalctl
```

```
Hint: You are currently not seeing messages from other users and the system.
      Users in groups 'adm', 'systemd-journal', 'wheel' can see all messages.
      Pass -q to turn off this notice.
-- Logs begin at Fri 2014-09-26 14:13:12 EDT, end at Fri 2017-03-31 09:54:58 EDT
...
```

![salida de journalctl](https://github.com/user-attachments/assets/265d7d0d-ba7e-4668-ae17-922fd4988065)

El mensaje indica que el usuario `analyst` no pertenece a ninguno de los grupos con permiso para leer todo el journal, así que solo ve sus propios mensajes.

¿Cómo se puede ejecutar journalctl y ver todas las entradas de registro?

Ejecutándolo con privilegios de root mediante `sudo journalctl`. Otra alternativa permanente es agregar al usuario a alguno de los grupos que menciona el mensaje (`adm`, `systemd-journal` o `wheel`), por ejemplo con `sudo usermod -aG systemd-journal analyst`.

### Filtros útiles

`journalctl` permite filtrar la salida con distintas opciones:

| Comando                                 | Qué muestra                                             |
| --------------------------------------- | ------------------------------------------------------- |
| `sudo journalctl -b`                    | Entradas del arranque actual                            |
| `sudo journalctl -b -1` / `-b -2`       | Entradas del arranque anterior / de dos arranques atrás |
| `sudo journalctl --list-boots`          | Lista de todos los arranques registrados                |
| `sudo journalctl --since "2 hours ago"` | Entradas de las últimas dos horas                       |
| `sudo journalctl --since "1 day ago"`   | Entradas del último día                                 |
| `sudo journalctl -u nginx.service`      | Entradas de un servicio (unidad) específico             |
| `sudo journalctl -f`                    | Monitoreo en tiempo real (equivalente a `tail -f`)      |
| `sudo journalctl -u nginx.service -f`   | Monitoreo en tiempo real solo de nginx                  |

![Entrada del arranque actual](https://github.com/user-attachments/assets/bdcaf5c6-d850-4175-a646-65073497bcc5)

![Listado de arranques registrados](https://github.com/user-attachments/assets/a65eb940-95a8-44e3-ac98-df8dc45b57cc)

![Registros de las ultimas 2 horas](https://github.com/user-attachments/assets/fa7e621b-41d7-412a-8e9f-84770ab74025)

Con `-u nginx.service` se ven los eventos del servicio en sí (arranque, parada, advertencias y errores), no las solicitudes HTTP:

```
Oct 19 16:47:57 secOps systemd[1]: Starting A high performance web server and a reverse proxy server...
Oct 19 16:47:57 secOps nginx[21058]: [warn] conflicting server name "localhost" on 0.0.0.0:80, ignored
Oct 19 16:47:57 secOps systemd[1]: Started A high performance web server and a reverse proxy server.
Oct 19 17:40:09 secOps nginx[21058]: [error] open() "/usr/share/nginx/html/favicon.ico" failed
```

![Eventos de nginx](https://github.com/user-attachments/assets/00ae2a93-b93e-4019-a5ff-f7cd4f380fc5)

> [!NOTE]
> En systemd los servicios se describen como unidades (units), por eso se filtran con `-u`. La mayoría de los paquetes crean y habilitan su unidad al instalarse.

### Monitoreo de nginx en tiempo real

Por último, combinando opciones se monitorean en vivo solo los eventos de nginx:

```bash
sudo journalctl -u nginx.service -f
```

Con el comando corriendo, se abre el navegador y se ingresa a `127.0.0.1` (o `127.0.0.1:8080` si se usa `custom_server.conf`). En la terminal aparece en tiempo real el error por el `favicon.ico` faltante:

```
[error] open() "/usr/share/nginx/html/favicon.ico" failed (2: No such file or directory)
```
