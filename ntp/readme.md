# NTP (Network Time Protocol)

Es un protocolo de red que se usa para sincronizar el reloj de los dispositivos (computadoras, servidores, routers, etc.) con una fuente de tiempo precisa, generalmente a través de internet o de una red local. Este protocolo sigue siendo utilizado y es uno de los protocolos más antiguos y usados de internet. Puede lograr sincronización con precisión de milisegundos en internet, e incluso microsegundos en redes locales. Usa paquetes UDP en el puerto 123

## Vulnerabilidades típicas de NTP

### 1. Amplificación/reflexión para DDoS

La más famosa. El comando monlist (que devuelve los últimos clientes que consultaron al servidor) permite a un atacante enviar una consulta pequeña con IP de origen falsificada (spoofing) y lograr que el servidor responda con un paquete mucho más grande hacia la víctima. Algunos sistemas NTP han sido abusados para facilitar ataques de denegación de servicio por reflexión y amplificación (RA DDoS), y en 2025 se registraron más de 45.000 alertas de DDoS donde NTP participó como reflector.

### 2. Denegación de servicio (DoS) contra el propio demonio

Varias CVEs históricas permiten crashear ntpd mediante paquetes malformados. Por ejemplo, versiones de NTP anteriores a 4.2.8p10 y 4.3.94 permitían a atacantes remotos causar denegación de servicio (crash de ntpd) mediante una directiva de configuración de modo malformada.

### 3. Desbordamientos de buffer (buffer overflow)

Se encontraron múltiples escrituras fuera de límites en funciones como mstolfp (parsing de números decimales) y en el driver refclock_palisade, que pueden ser explotadas por un adversario contra procesos cliente como ntpq.

### 4. Manipulación del reloj (Sybil / clock-selection attacks)

Un atacante autenticado que conoce la clave simétrica privada puede crear muchas asociaciones "efímeras" falsas para ganar el algoritmo de selección de reloj de ntpd y así modificar el reloj de la víctima mediante un ataque tipo Sybil. Esto es grave porque compromete la integridad temporal del sistema, no solo su disponibilidad.

### 5. Suplantación de "reference clocks"

ntpd confía en que el sistema operativo subyacente lo proteja de peticiones que suplantan relojes de referencia, algo que no siempre es garantizado.

## Implicancias de explotar estas vulnerabilidades

- DDoS reflejado: tu servidor NTP se convierte en arma involuntaria contra terceros, generando tráfico saliente masivo, consumo de ancho de banda y posibles bloqueos por parte de tu ISP.
- Caída del servicio de tiempo (DoS directo): si ntpd crashea, todos los sistemas que dependen de esa fuente pierden sincronización. Esto es crítico en entornos donde el tiempo preciso importa: certificados TLS (validación de expiración), Kerberos (autenticación falla si el reloj se desvía demasiado), logs forenses, transacciones financieras, sistemas distribuidos (bases de datos, blockchain), SCADA/infraestructura industrial.
- Manipulación del reloj: es la más peligrosa conceptualmente. Si un atacante logra correr el reloj hacia atrás o adelante, puede: invalidar o revivir certificados expirados, romper la lógica de tokens con expiración (OTP, JWT), sabotear auditorías/logs, o crear ventanas para replay attacks.
- Compromiso remoto (RCE): los buffer overflows, si son explotables más allá de un crash, podrían derivar en ejecución de código remoto, aunque en NTP moderno esto es menos común gracias a mitigaciones del SO (ASLR, stack canaries) pero sigue siendo la categoría de mayor severidad.
- Efecto cascada: como NTP es una dependencia "silenciosa" de casi toda la infraestructura, un compromiso puede propagarse a sistemas que ni siquiera saben que confían en NTP directamente.

## Configuración propia de NTP

En mi caso yo utilizo systemd-timesyncd, este software utiliza SNTP (Simple NTP) una version simplificada para clientes, esta version de NTP consume menos recursos y tiene dependencia total de un unico servidor, en cambio en NTP se consume mas recursos para tener una precision a nivel de microsegundos y la sincronización es multi-servidor para evitar un unico punto de fallo

![screenshot NTP](https://github.com/user-attachments/assets/94486a15-cff9-44f0-adc6-d45db7ee3aba)

## Configuracion de NTP para evitar problemas

### 1. Usar chrony en vez de ntpd o systemd-timesyncd

Es el estándar en la mayoría de las distros modernas (RHEL/CentOS 8+, Ubuntu recientes) y maneja mejor los ajustes de reloj bruscos, redes intermitentes y VMs. Ademas permite usar NTP sobre TLS (NTS) (para evitar ataques MITM), esto no es posible de hacer con systemd-timesyncd o ntpd.

### 2. Configurar varias fuentes, no una sola

Luego de cambiar a chrony, en /etc/chrony/chrony.conf (o chrony.conf en RHEL):

```text
pool time.cloudflare.com iburst
pool nts.time.nl iburst
pool ptbtime1.ptb.de iburst
```

iburst acelera la sincronización inicial (manda varios paquetes seguidos en vez de esperar el intervalo normal).

Con al menos 3-4 fuentes, chrony puede descartar una fuente "mentirosa" (falseticker) por consenso. Con solo una, no tenés forma de detectar si esa fuente está mal.
Preferir servidores cercanos geográficamente reduce la latencia y mejora la precisión.

### 3. Cambiar el protocolo a NTS

```text
# Servidores con soporte NTS
server time.cloudflare.com iburst nts
server nts.time.nl iburst nts
server ptbtime1.ptb.de iburst nts

# Directorio donde chrony guarda las claves/cookies NTS (persistencia entre reinicios)
ntsdumpdir /var/lib/chrony
```

### 3. Firewall

En el firewall permitir salida UDP/123 hacia tus servidores NTP.

### 4. Hacer que arranque en boot

```bash
sudo systemctl enable --now chronyd
```

### Verificar que esta bien la configuracion

```bash
chronyc tracking
```

## ntp.inti.gob.ar

Es un servidor legítimo. INTI (Instituto Nacional de Tecnología Industrial) es un organismo estatal argentino que, entre otras cosas, tiene un laboratorio de metrología/tiempo, y ofrece ese servidor NTP como servicio público. Es una fuente confiable y segura de usar. De todos modos siempre es importante tener configurado multiples fuentes
