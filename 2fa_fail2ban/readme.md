# Autenticación de dos factores (2FA) y Fail2ban

## Introducción

Cualquier servidor con SSH expuesto a internet recibe, en cuestión de minutos, intentos automáticos de inicio de sesión: bots que recorren rangos de IPs probando usuarios comunes (`root`, `admin`, `ubuntu`) con diccionarios de contraseñas. Si la única barrera es una contraseña, la seguridad del equipo depende enteramente de que esa contraseña sea buena y de que nunca se haya filtrado.

En este trabajo se configuran dos herramientas que atacan el problema desde lados distintos:

- **2FA (autenticación de dos factores):** aunque el atacante adivine o robe la contraseña, no puede entrar sin un segundo factor que solo tiene el usuario legítimo (un código temporal generado en su celular).
- **Fail2ban:** detecta en los logs los intentos fallidos repetidos y bloquea en el firewall la IP que los genera, cortando el ataque de fuerza bruta antes de que pueda probar muchas contraseñas.

Juntas son un ejemplo de defensa en profundidad: una reduce la probabilidad de que un ataque tenga éxito y la otra reduce la cantidad de intentos que el atacante puede hacer.

# Marco teórico

## Autenticación y factores

Autenticar es verificar que alguien es quien dice ser. Para eso se usan factores de autenticación, que se agrupan en tres categorías:

| Factor         | Descripción            | Ejemplos                                                               |
| -------------- | ---------------------- | ---------------------------------------------------------------------- |
| Algo que sabés | Conocimiento           | Contraseña, PIN, respuesta secreta                                     |
| Algo que tenés | Posesión               | Celular con app de códigos, token físico, llave FIDO2/YubiKey, tarjeta |
| Algo que sos   | Inherencia (biometría) | Huella digital, reconocimiento facial, iris                            |

La autenticación de dos factores (2FA) exige presentar dos factores **de categorías distintas**. Pedir dos contraseñas no es 2FA, porque ambas son "algo que sabés" y se pueden robar de la misma forma (phishing, keylogger, filtración de una base de datos). Cuando se usan dos o más factores se habla en general de MFA (Multi-Factor Authentication).

### ¿Por qué no alcanza con una contraseña?

- Los usuarios reutilizan contraseñas entre sitios, así que una filtración en un servicio compromete otros (credential stuffing).
- Las contraseñas débiles caen rápido ante ataques de diccionario o fuerza bruta.
- Pueden robarse por phishing, keyloggers o capturando tráfico sin cifrar.

Con 2FA, conocer la contraseña deja de ser suficiente: el atacante además necesitaría tener físicamente el dispositivo del usuario.

## TOTP: contraseñas de un solo uso basadas en el tiempo

El segundo factor que se usa en este práctico es TOTP (Time-based One-Time Password, RFC 6238), el mismo mecanismo que usan Google Authenticator, Authy, FreeOTP, Aegis, etc.

Funciona así:

1. **Configuración:** el servidor genera una clave secreta aleatoria y se la pasa al usuario, normalmente como un código QR. El usuario la escanea con la app del celular. A partir de ese momento, **servidor y celular comparten el mismo secreto**.
2. **Generación del código:** cada 30 segundos, ambos calculan de forma independiente:

   ```
   T      = floor(tiempo_unix_actual / 30)
   código = Truncar( HMAC-SHA1(secreto, T) ) mod 10^6
   ```

   El tiempo unix es el tiempo que pasó desde el 1/1/1970, se divide en 30 y se obtiene el entero menor inmediato para obtener el bloque de 30 segundos bajo el tiempo unix. Luego este valor se mezcla con una clave secreta mediante HMAC-SHA1, un hash con clave que devuelve 20 bytes. Si cambia T el resultado cambia por completo, y sin conocer el secreto es imposible calcularlo, aunque T sea público (es solo la hora). Por último, como 20 bytes son demasiados para que el usuario los tipee, se recortan: se toman 4 de esos bytes (el último byte del HMAC indica a partir de qué posición) y se interpretan como un número entero, del que se conservan los últimos 6 dígitos (`mod 10^6`).

   | Tiempo unix | ÷ 30        | T        |
   | ----------- | ----------- | -------- |
   | 1790000000  | 59666666,67 | 59666666 |
   | 1790000010  | 59666667,00 | 59666667 |
   | 1790000025  | 59666667,50 | 59666667 |
   | 1790000040  | 59666668,00 | 59666668 |

   Como se ve en la tabla, T se mantiene igual durante 30 segundos y luego aumenta en 1, por eso el código de la app dura 30 segundos. Como el celular y el servidor tienen la misma hora, ambos obtienen el mismo T, y al tener el mismo secreto llegan al mismo código sin necesidad de comunicarse:

3. **Verificación:** el usuario escribe el código que muestra la app y el servidor lo compara con el que calculó él mismo. Si coinciden, el usuario demuestra que tiene el secreto (el celular) sin haberlo transmitido nunca.

Algunas propiedades importantes:

- El código cambia cada 30 segundos y, en general, solo puede usarse una vez, así que capturarlo sirve de poco.
- No hace falta conexión a internet ni en el servidor ni en el celular: todo se calcula localmente.
- Depende de que **los relojes estén sincronizados**. Si el reloj del servidor o del celular está corrido, los códigos no coinciden (por eso se suele aceptar una pequeña ventana de tolerancia). Es otro caso donde NTP resulta fundamental.
- TOTP es una variante de HOTP (RFC 4226), donde el contador en vez de ser el tiempo es un número que se incrementa en cada uso.

> [!IMPORTANT]
> TOTP protege contra el robo de contraseñas y la fuerza bruta, pero no es invulnerable: un sitio de phishing en tiempo real puede pedirle al usuario el código y usarlo dentro de los 30 segundos. Contra eso son más robustos los métodos basados en llaves físicas (FIDO2/WebAuthn). Además, si el secreto se filtra, el atacante puede generar códigos válidos.

## PAM (Pluggable Authentication Modules)

En Linux, la mayoría de los programas que autentican usuarios (`login`, `sshd`, `sudo`, `su`) no implementan la autenticación por su cuenta, sino que delegan en PAM. PAM es un sistema modular: cada servicio tiene un archivo en `/etc/pam.d/` que define una "pila" de módulos que se ejecutan en orden, y cada módulo verifica algo.

Una línea de configuración PAM tiene la forma:

```
tipo    control     módulo              [opciones]
auth    required    pam_unix.so
```

- **tipo:** qué etapa controla (`auth` para autenticación, `account`, `password`, `session`).
- **control:** qué pasa si el módulo falla (`required` → la autenticación falla, aunque se siguen evaluando los demás módulos; `requisite` → falla inmediatamente; `sufficient` → si pasa, alcanza).
- **módulo:** la librería que hace la verificación.

Gracias a esta arquitectura, agregar 2FA no requiere modificar SSH: alcanza con sumar a la pila de `sshd` el módulo `pam_google_authenticator.so`, que es el que pide y verifica el código TOTP.

## Ataques de fuerza bruta

Un ataque de fuerza bruta consiste en probar credenciales de forma automatizada hasta dar con una válida. Sus variantes más comunes son:

- **Fuerza bruta pura:** probar todas las combinaciones posibles.
- **Diccionario:** probar listas de contraseñas comunes o filtradas (`123456`, `password`, `admin`...).
- **Credential stuffing:** usar pares usuario/contraseña filtrados de otros servicios.
- **Password spraying:** probar pocas contraseñas muy comunes contra muchos usuarios, para no disparar bloqueos por cuenta.

Servicios como SSH, FTP, paneles web de administración o servidores de correo son objetivos habituales. Herramientas como Hydra o Medusa automatizan estos ataques.

## Fail2ban

Fail2ban es una herramienta de prevención de intrusiones que **lee los logs** de los servicios, busca patrones que indiquen intentos fallidos (por ejemplo `Failed password for root from 203.0.113.5`) y, cuando una misma IP supera un umbral, **la bloquea temporalmente en el firewall**.

Se relaciona directamente con el trabajo de logs anterior: fail2ban no inspecciona el tráfico de red, sino que depende por completo de que los servicios registren sus eventos y de que él sepa dónde leerlos.

### Conceptos principales

| Concepto   | Descripción                                                                                                      |
| ---------- | ---------------------------------------------------------------------------------------------------------------- |
| **Filter** | Conjunto de expresiones regulares que reconocen un intento fallido en el log de un servicio (`filter.d/`).       |
| **Action** | Qué hacer al detectar un abuso: normalmente agregar una regla de bloqueo en el firewall (`action.d/`).           |
| **Jail**   | Combinación de un filtro, una o más acciones, el log a vigilar y los parámetros de bloqueo. Ej.: la jail `sshd`. |
| `maxretry` | Cantidad de fallos permitidos antes de bloquear.                                                                 |
| `findtime` | Ventana de tiempo en la que se cuentan esos fallos.                                                              |
| `bantime`  | Cuánto tiempo dura el bloqueo.                                                                                   |
| `ignoreip` | IPs o redes que nunca se bloquean (por ejemplo, la red de administración).                                       |
| `backend`  | De dónde lee los eventos: archivos de log (`auto`, `polling`) o el journal de systemd (`systemd`).               |

Por ejemplo, con `maxretry = 3`, `findtime = 10m` y `bantime = 1h`: si una IP falla 3 veces en 10 minutos, queda bloqueada durante 1 hora.

### Flujo de funcionamiento

```
 Atacante ──(intentos SSH)──▶ sshd ──(escribe)──▶ log / journal
                                                        │
                                                        ▼
                                         fail2ban (filter: regex sshd)
                                                        │  ¿superó maxretry en findtime?
                                                        ▼
                                         action: regla en nftables/iptables
                                                        │
 Atacante ◀────────── conexión rechazada ───────────────┘   (hasta que vence bantime)
```

### Limitaciones

- **Ataques distribuidos:** una botnet que usa miles de IPs distintas, cada una con pocos intentos, nunca supera el umbral.
- **Depende de los logs:** si el servicio no registra el fallo, o lo registra en un formato que el filtro no reconoce, fail2ban no hace nada.
- **Bloqueo por IP:** usuarios legítimos detrás del mismo NAT (una universidad, una empresa) pueden quedar bloqueados por culpa de otro. También es posible bloquearse a uno mismo.
- **No reemplaza a una buena autenticación:** solo frena el ritmo del ataque; una contraseña débil sigue siendo débil.

# Práctico

El práctico se realiza sobre una máquina virtual con Debian 12, a la que se accede por SSH desde otra máquina (el host u otra VM) para poder probar tanto el 2FA como los bloqueos de fail2ban.

> [!IMPORTANT]
> Antes de empezar se toma un snapshot de la VM. Modificar la configuración de SSH y PAM puede dejarnos sin acceso, y el snapshot permite volver atrás en segundos. Además, durante las pruebas conviene mantener siempre una sesión abierta (o la consola de la VM) para poder corregir la configuración si algo falla.

![Snapshot tomado](https://github.com/user-attachments/assets/e90d3d5d-57c7-42ff-97bb-ae73f806e0ce)

Todos los pasos de configuración están automatizados en [`setup.sh`](setup.sh), y la configuración de fail2ban utilizada está en [`jail.local`](jail.local).

## Parte 1: Autenticación de dos factores en SSH

### Paso 1: Instalar los paquetes

```bash
sudo apt update
sudo apt install openssh-server libpam-google-authenticator
```

`libpam-google-authenticator` instala el módulo PAM y la herramienta `google-authenticator` para generar el secreto de cada usuario.

![Instalacion de paquetes](https://github.com/user-attachments/assets/305b9578-9415-42af-a2d8-113b0f1d9e9e)

### Paso 2: Generar el secreto TOTP del usuario

Se ejecuta **como el usuario que va a iniciar sesión** (no como root):

```bash
google-authenticator
```

El programa hace una serie de preguntas. Las respuestas recomendadas son:

| Pregunta                                   | Respuesta | Motivo                                                    |
| ------------------------------------------ | --------- | --------------------------------------------------------- |
| ¿Tokens basados en tiempo?                 | `y`       | Usar TOTP (en lugar de HOTP, basado en contador)          |
| ¿Actualizar `~/.google_authenticator`?     | `y`       | Guardar el secreto y la configuración                     |
| ¿Prohibir el uso múltiple del mismo token? | `y`       | Un código capturado no puede reutilizarse                 |
| ¿Ampliar la ventana de tiempo?             | `n`       | Mantener la ventana chica (relojes sincronizados por NTP) |
| ¿Habilitar rate-limiting?                  | `y`       | Máximo 3 intentos cada 30 segundos                        |

Después de la primera pregunta muestra un código QR y la clave secreta. Se escanea el QR con una app de autenticación (Google Authenticator, Aegis, FreeOTP, etc.) y el programa pide ingresar el código que muestra la app (`Enter code from app`). Esto sirve para verificar que el celular quedó bien configurado y que ambos relojes están sincronizados: si el código coincide responde `Code confirmed`.

Luego muestra una lista de **códigos de emergencia** (scratch codes) de un solo uso, que sirven para entrar si se pierde el celular, y continúa con el resto de las preguntas.

![QR](https://github.com/user-attachments/assets/76145cbb-d2cf-487b-81ca-38d1f5a71963)

![Finalizacion de la configuracion](https://github.com/user-attachments/assets/6a85e558-fc8b-4580-97f0-21aaf3a349c6)

### Paso 3: Configurar PAM

Se agrega el módulo a la pila de autenticación de SSH, al final de `/etc/pam.d/sshd`:

```bash
sudo nano /etc/pam.d/sshd
```

Como se agrega después de `@include common-auth`, primero se pide la contraseña del usuario y después el código de verificación.

La opción `nullok` permite entrar sin código a los usuarios que todavía no ejecutaron `google-authenticator`. Es útil mientras se configura (evita dejar afuera a otros usuarios), pero una vez que todos tienen su secreto conviene quitarla para que el 2FA sea obligatorio.

![Agrega el autenticador a ssh](https://github.com/user-attachments/assets/21ad4221-179b-4212-aab7-c5bb528e4e06)

### Paso 4: Configurar SSH

SSH tiene que usar PAM y permitir la autenticación interactiva por teclado (keyboard-interactive), que es la que permite hacer la pregunta adicional del código. En Debian 12 se crea un archivo en `/etc/ssh/sshd_config.d/`:

```bash
sudo nano /etc/ssh/sshd_config.d/10-2fa.conf
```

Se verifica la sintaxis y se reinicia el servicio:

```bash
sudo sshd -t
sudo systemctl restart ssh
```

### Paso 5: Probar el acceso

Sin cerrar la sesión actual, desde otra terminal o desde el host:

```bash
ssh usuario@192.168.56.1
```

Ahora, además de la contraseña, se pide el código de 6 dígitos de la app.

![Prueba con autenticator](https://github.com/user-attachments/assets/9cf32acd-9c34-4fc8-ab40-42af6c3bf20a)

Si se ingresa un código incorrecto (o uno vencido), el acceso se rechaza aunque la contraseña sea correcta:

![Prueba ingresando codigo incorrecto](https://github.com/user-attachments/assets/9c13fb7b-8800-4440-afbf-e75b67f9ff89)

El intento queda registrado en el journal:

```bash
sudo journalctl -u ssh -n 20
```

![Intentos invalidos registrados en journalctl](https://github.com/user-attachments/assets/40848961-8ac1-482d-9854-c12071e7ec8c)

## Parte 2: Fail2ban

### Paso 1: Instalar fail2ban

```bash
sudo apt install fail2ban python3-systemd
```

![Instalacion de Fail2Ban](https://github.com/user-attachments/assets/f06bcc74-f7e9-40e8-bf90-331e04ed6b53)

### Paso 2: Configurar la jail de SSH

La configuración por defecto está en `/etc/fail2ban/jail.conf`, pero ese archivo no se edita porque se sobreescribe al actualizar el paquete. Los cambios se hacen en `/etc/fail2ban/jail.local`, que tiene prioridad:

```bash
sudo nano /etc/fail2ban/jail.local
```

> [!NOTE]
> El `backend = systemd` es necesario por lo mismo que se vio en el trabajo de logs: Debian 12 ya no instala rsyslog por defecto, así que no existe `/var/log/auth.log` y todos los eventos quedan en el journal. Si fail2ban intenta leer un archivo que no existe, la jail no arranca.

![Archivo jail.conf](https://github.com/user-attachments/assets/5569b924-215b-44e2-b7f3-e92752835088)

### Paso 3: Iniciar el servicio

```bash
sudo systemctl enable --now fail2ban
sudo systemctl restart fail2ban
sudo fail2ban-client status
```

![Jail activa para ssh](https://github.com/user-attachments/assets/a03e2ac9-ae1e-4809-b2ae-6cb71c3a4e7a)

### Paso 4: Provocar un bloqueo

Desde la otra máquina (el host u otra VM, **no** desde una IP incluida en `ignoreip`) se intenta entrar por SSH con una contraseña incorrecta varias veces:

```bash
ssh usuario@192.168.56.1
```

Después del tercer intento fallido, la conexión deja de responder:

![ssh Rechaza la conexion](https://github.com/user-attachments/assets/eae235c6-f58a-4cc0-a60d-87ea304750a9)

### Paso 5: Verificar el bloqueo

En la VM se consulta el estado de la jail:

```bash
sudo fail2ban-client status sshd
```

Y la regla que agregó en el firewall:

```bash
sudo nft list ruleset
```

![Consulta del estado del jail y nftable desde el host](https://github.com/user-attachments/assets/2cdfe597-bccb-49fd-82f6-929017c02351)

También se puede ver en el log de fail2ban el momento en que se detectaron los fallos y se aplicó el bloqueo:

```bash
sudo tail -n 20 /var/log/fail2ban.log
```

![Log de fail2ban](https://github.com/user-attachments/assets/f7516ad2-c56d-44e8-b487-d025f4112b7e)

### Paso 6: Desbloquear una IP

El bloqueo se levanta solo al cumplirse el `bantime`, pero también se puede quitar a mano (por ejemplo, si el bloqueado fue un usuario legítimo):

```bash
sudo fail2ban-client set sshd unbanip 192.168.56.1
```

# Conclusión

Las dos herramientas son simples de instalar, prácticamente no consumen recursos y mejoran mucho la seguridad de un servicio expuesto como SSH, pero cada una tiene su lugar.

**2FA** eleva el costo de un ataque de forma significativa: robar o adivinar la contraseña deja de ser suficiente. Tiene mucho sentido en:

- Servidores accesibles desde internet, sobre todo si se usa autenticación por contraseña.
- Cuentas con privilegios (administradores, acceso a `sudo`, paneles de gestión).
- Accesos remotos (VPN, SSH, correo) de organizaciones donde los usuarios pueden reutilizar contraseñas.

Tiene menos sentido, o hay que pensarlo bien, en:

- Accesos automatizados (scripts, backups, despliegues, Ansible), que no pueden ingresar un código. Para esos casos se usan claves SSH dedicadas con permisos acotados.
- Equipos sin posibilidad de mantener el reloj sincronizado.
- Entornos donde no hay un procedimiento de recuperación: si se pierde el celular y no se guardaron los códigos de emergencia, el usuario queda afuera.

**Fail2ban** es una buena medida complementaria: reduce el ruido en los logs, frena los ataques automatizados y limita la cantidad de contraseñas que un atacante puede probar. Tiene sentido en:

- Cualquier servicio con autenticación expuesto a internet (SSH, FTP, correo, paneles web).
- Servidores que no están detrás de otra capa que ya haga esta tarea (un WAF, un balanceador o un proveedor con protección propia).

Hay que tener en cuenta que:

- No protege contra ataques distribuidos ni contra un atacante que ya tiene la contraseña correcta.
- En redes con NAT compartido puede bloquear a usuarios legítimos; conviene ajustar `ignoreip`, `maxretry` y `bantime` según el caso.
- En un servidor que solo acepta claves SSH (sin contraseñas) aporta menos, ya que la fuerza bruta es prácticamente inviable, aunque sigue siendo útil para reducir el ruido.

En resumen, ninguna herramienta por sí sola asegura el acceso, pero combinadas con buenas prácticas (autenticación por clave, deshabilitar el login de `root`, mantener el sistema actualizado y monitorear los logs) forman varias capas de defensa: fail2ban reduce la cantidad de intentos que llegan a probarse, y 2FA asegura que aun un intento con la contraseña correcta no alcance para entrar.
