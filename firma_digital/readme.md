# Firma digital en correos electrónicos

## SMTP (Simple Mail Transfer Protocol)

Es el protocolo estándar de internet que se usa para **enviar correos electrónicos** desde un cliente hacia un servidor, o entre servidores de correo. Funciona como una especie de "cartero" digital: toma el mensaje, verifica la dirección del destinatario y lo va pasando de servidor en servidor hasta que llega a su destino.

Algunos puntos clave:

- Trabaja normalmente sobre los puertos **25, 587 o 465** (dependiendo si usa cifrado o no).
- Es un protocolo relativamente simple, basado en texto plano, con comandos como `HELO`, `MAIL FROM`, `RCPT TO` y `DATA`.
- Solo se encarga del **envío**; para recibir o leer correos se usan otros protocolos como **POP3** o **IMAP**.

## No requiere autenticación por diseño

Cuando se creó SMTP en los años 80, **no se incluyó un mecanismo de autenticación obligatorio**. Esto significa que, en su forma más básica, cualquiera podía conectarse a un servidor SMTP y enviar correos sin tener que demostrar quién era.

Esto trajo consecuencias importantes:

- Es una de las razones históricas por las que el **spam** y el **email spoofing** (falsificar el remitente) se volvieron tan comunes.
- Con el tiempo se agregaron extensiones como **SMTP-AUTH** para exigir usuario y contraseña antes de enviar correo, y mecanismos como **SPF, DKIM y DMARC** para verificar que el remitente sea legítimo.
- Aun así, algunos servidores mal configurados (llamados "open relays") siguen permitiendo el envío sin autenticación, lo cual es considerado una mala práctica de seguridad.

## Email Spoofing: qué es y por qué funciona

El **email spoofing** es la técnica de **falsificar la dirección del remitente** de un correo electrónico, haciendo que parezca que fue enviado por otra persona o entidad (por ejemplo, hacerse pasar por un banco, una empresa conocida o un contacto de confianza).

Funciona justamente por lo que vimos antes sobre SMTP: **el protocolo no verifica por diseño quién dice ser el remitente**.

- El campo `MAIL FROM` (que define el remitente) es simplemente un dato de texto que se envía al servidor, y este **no comprueba si esa dirección realmente pertenece a quien está conectado**.
- Es como escribir un remitente cualquiera en un sobre de papel: el correo postal lo entrega igual, sin chequear si quien lo mandó es realmente esa persona.
- Esto significa que, técnicamente, cualquiera con acceso a un servidor SMTP (propio o mal configurado) puede escribir la dirección que quiera en el campo "De:".

Aunque hoy existen mecanismos para mitigarlo, no siempre se implementan correctamente:

- **SPF (Sender Policy Framework):** define qué servidores están autorizados a enviar correos en nombre de un dominio.
- **DKIM (DomainKeys Identified Mail):** agrega una firma criptográfica al correo para verificar que no fue alterado y que salió del dominio que dice.
- **DMARC:** le dice a los servidores receptores qué hacer si un correo falla las verificaciones de SPF o DKIM (rechazarlo, ponerlo en spam, etc.).

El problema es que estos mecanismos son **opcionales** y dependen de que el dominio los configure. Si una organización no los tiene bien implementados, sigue siendo posible falsificar su dirección de remitente con relativa facilidad.

## Thunderbird

Es un **cliente de correo electrónico gratuito y de código abierto**, desarrollado por la **Fundación Mozilla** (la misma organización detrás de Firefox).

Características principales:

- Permite gestionar múltiples cuentas de correo (Gmail, Outlook, cuentas propias, etc.) desde un solo lugar.
- Incluye funciones de **calendario, contactos y filtros de correo**.
- Es multiplataforma: funciona en Windows, macOS y Linux.
- Soporta extensiones/complementos para personalizar su funcionamiento, similar a Firefox.
- Es una alternativa popular a clientes propietarios como Outlook, especialmente valorada por su enfoque en la **privacidad** y por ser software libre.

### Encriptación de correos en Thunderbird

Thunderbird permite **encriptar los correos electrónicos** usando el estándar **OpenPGP** (integrado de forma nativa desde la versión 78, sin necesidad de complementos externos como Enigmail).

Para realizar esto:

- Se genera un par de claves: una **pública** (que compartís con quienes te van a escribir) y una **privada** (que guardás solo vos, protegida con contraseña).
- Cuando alguien te envía un correo cifrado con tu clave pública, **solo vos podés desencriptarlo** con tu clave privada.
- Del mismo modo, vos podés firmar y cifrar los correos que enviás.

Poder encriptar correos es importante porque:

- El contenido del mensaje se cifra **antes de salir del cliente**, por lo que viaja cifrado de extremo a extremo entre el remitente y el destinatario.
- Esto hace que **el servidor de correo (y cualquiera que intercepte el tráfico) no pueda leer el contenido del mensaje**, ya que solo ve datos cifrados.

> [!IMPORTANT]
> Es importante aclarar que esto protege el **cuerpo del mensaje**, pero metadatos como el asunto, remitente y destinatario suelen seguir siendo visibles para el servidor (a menos que se usen configuraciones adicionales).

Esto convierte a Thunderbird en una herramienta valorada por usuarios preocupados por la privacidad, ya que no depende de que el proveedor de correo (Gmail, Outlook, etc.) implemente cifrado, sino que el cifrado ocurre directamente en el cliente antes del envío.

# Práctico

La tarea practica consiste en descargar thunderbird:

1. Descargar thunderbird
2. Configurar el acceso a tu cuenta de la universidad
3. En el apartado end-to-end encryption generar un par de claves publica privada mediante OpenPGP

![clave publica-privada configurada](https://github.com/user-attachments/assets/0daaeee2-c9e0-4e95-96db-d209ce22c63c)

4. Subir la clave publica asociado al email en **vks://keys.openpgp.org**, esto se hace mediante el boton publish mostrado en la imagen anterior

5. A continuacion uno puede enviar un mail encriptado con su clave privada y el receptor podra desencriptarlo usando la clave publica asociado al remitente

Personalmente cuando quise enviarle un email al profesor no encontre una clave publica vigente, la ultima que registro vencio hace 2 dias

![Envio de email encriptado](https://github.com/user-attachments/assets/d659b823-61a6-482c-90b3-61d804a0d58a)
