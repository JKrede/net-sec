# Cifrar y descifrar datos y archivos con OpenSSL

## ¿Qué es OpenSSL?

OpenSSL es un proyecto de código abierto que ofrece un kit de herramientas completo para los protocolos **TLS** (Transport Layer Security) y **SSL** (Secure Sockets Layer). Además de implementar esos protocolos, es una **biblioteca de criptografía de uso general**: desde la línea de comandos permite cifrar y descifrar archivos, generar claves, calcular hashes, firmar documentos y manejar certificados.

En este laboratorio se usa como herramienta independiente para cifrar con:

- **Criptografía simétrica** (AES-256).
- **Criptografía asimétrica** (RSA).
- Una combinación de ambas (**cifrado híbrido**) para archivos grandes.

> [!NOTE]
> El laboratorio está pensado para OpenSSL 1.x (VM CyberOps Workstation). La VM que usé tiene **OpenSSL 3.0**, así que algunos comandos muestran avisos de deprecación o se comportan distinto. Las diferencias están explicadas en cada paso.

## Criptografía simétrica

En la criptografía simétrica se usa **la misma clave para cifrar y para descifrar**. Es rápida y sirve para grandes volúmenes de datos, pero tiene un problema central: **cómo compartir la clave** con el destinatario sin que nadie más la obtenga.

### AES (Advanced Encryption Standard)

AES es el estándar de cifrado simétrico por bloques más usado hoy en día. Cifra bloques de 128 bits y admite claves de 128, 192 o 256 bits. En el laboratorio se usa `aes-256-cbc`:

- **256**: largo de la clave en bits.
- **CBC** (Cipher Block Chaining): modo de operación en el que cada bloque de texto plano se combina (XOR) con el bloque cifrado anterior antes de cifrarse. Así, dos bloques iguales en el texto plano no producen el mismo bloque cifrado.

### De la contraseña a la clave: derivación y salt

Cuando se cifra con `openssl enc` usando una contraseña, OpenSSL no usa la contraseña directamente como clave AES. Primero la pasa por una **función de derivación de claves** (KDF) junto con un **salt**:

- **Salt**: 8 bytes aleatorios que se generan en cada cifrado y se guardan al principio del archivo cifrado (por eso el archivo empieza con `Salted__`). Hace que la misma contraseña produzca una clave distinta cada vez e impide usar tablas precalculadas.
- **KDF por defecto (`EVP_BytesToKey`)**: es la que usa el laboratorio original. Hace una sola iteración de MD5, así que es muy rápida y fácil de atacar por fuerza bruta. OpenSSL 3 avisa de esto con el mensaje `deprecated key derivation used`.
- **PBKDF2 (`-pbkdf2`)**: la alternativa recomendada. Aplica miles de iteraciones de un hash, lo que hace mucho más costoso probar contraseñas.

> [!IMPORTANT]
> Las opciones de derivación (`-pbkdf2`, `-iter`, `-md`) tienen que ser **las mismas al cifrar y al descifrar**. Si no coinciden, la clave AES que se obtiene es distinta y OpenSSL devuelve `bad decrypt`.

## Criptografía asimétrica (clave pública)

En la criptografía asimétrica cada usuario tiene un **par de claves** matemáticamente relacionadas:

- **Clave pública**: se comparte libremente. Sirve para **cifrar** mensajes dirigidos a su dueño.
- **Clave privada**: se guarda en secreto. Es la única que puede **descifrar** lo que se cifró con la pública.

Esto resuelve el problema de la distribución de claves: si Alice quiere mandarle algo confidencial a Bob, solo necesita la clave pública de Bob, que no es secreta.

### RSA

RSA es el algoritmo asimétrico más conocido. Su seguridad se basa en la dificultad de factorizar el producto de dos números primos grandes. Una clave RSA está formada por:

| Componente                   | Parte de | Descripción                                       |
| ---------------------------- | -------- | ------------------------------------------------- |
| `modulus` (n)                | Pública  | Producto de los dos primos `p · q`                |
| `publicExponent` (e)         | Pública  | Normalmente `65537`                               |
| `privateExponent` (d)        | Privada  | Inverso de `e` que permite descifrar              |
| `prime1`, `prime2` (p, q)    | Privada  | Los dos primos secretos                           |
| `exponent1/2`, `coefficient` | Privada  | Valores precalculados para acelerar el descifrado |

La desventaja de RSA es que es **lento** y **solo puede cifrar datos más chicos que la clave** (con una clave de 1024 bits, como máximo 117 bytes con el padding PKCS#1). Por eso no se usa para cifrar archivos grandes directamente.

## Cifrado híbrido

Para cifrar archivos grandes se combinan las dos técnicas y se aprovecha lo mejor de cada una:

1. Se genera una **clave simétrica aleatoria (R)**.
2. Se cifra el archivo grande **(F)** con AES usando R: `EF = Sym(F, R)`. Es rápido para cualquier tamaño.
3. Se cifra **la clave R** con la clave pública RSA del destinatario: `Asym(Bob_public, R)`. R es chica, así que RSA alcanza.
4. Se envían ambos archivos. El destinatario recupera R con su clave privada y con R descifra el archivo.

Este es el mismo esquema que usan protocolos como TLS, PGP o S/MIME.

# Práctico

Todo el laboratorio se hace dentro del directorio `/home/analyst/lab.support.files`.

## Parte 1: Criptografía simétrica

### Paso 1: Cifrar un archivo de texto

Se muestra el archivo en texto plano y se lo cifra con AES-256. OpenSSL pide una contraseña y su confirmación:

```bash
cat letter_to_grandma.txt
openssl aes-256-cbc -in letter_to_grandma.txt -out message.enc
cat message.enc
```

![Mensaje encriptado](https://github.com/user-attachments/assets/e886e36e-d11b-4000-bba4-8191003a166c)

> ¿Qué se observa al hacer `cat message.enc`?
> El contenido es ilegible: son bytes binarios que la terminal intenta mostrar como caracteres, por eso aparecen símbolos raros y signos de pregunta. Lo único legible es el encabezado `Salted__`, que indica que el archivo tiene un salt guardado en los 8 bytes siguientes. También aparece el aviso `deprecated key derivation used`, porque OpenSSL 3 recomienda usar `-pbkdf2`.

### Paso 2: Cifrar con codificación Base64

Se repite el cifrado agregando la opción `-a`, que codifica en Base64 el resultado cifrado antes de guardarlo:

```bash
openssl aes-256-cbc -a -in letter_to_grandma.txt -out message.enc
cat message.enc
```

Ahora el archivo se ve como texto (letras, números, `+`, `/` y `=`), empezando con `U2FsdGVkX1`, que es `Salted__` codificado en Base64.

> ¿Qué beneficio tiene que `message.enc` esté codificado en Base64?
> El archivo sigue estando cifrado (la seguridad es la misma), pero ahora usa solo caracteres ASCII imprimibles. Eso permite copiarlo y pegarlo, mandarlo en el cuerpo de un correo, en un chat o en un JSON sin que se corrompa. Los canales pensados para texto pueden alterar o descartar bytes binarios, y con Base64 eso no pasa. La desventaja es que el archivo ocupa aproximadamente un 33% más.

## Parte 2: Descifrado simétrico

```bash
openssl aes-256-cbc -a -d -in message.enc -out decrypted_letter.txt
cat decrypted_letter.txt
```

Se ingresa la misma contraseña que al cifrar y se recupera la carta original.

![Mensaje desencriptado](https://github.com/user-attachments/assets/e6271b2b-5f64-493f-ad5e-17f8a82887ad)

> ¿Por qué el comando de descifrado también lleva la opción `-a`?
> Porque `message.enc` está en Base64. Con `-a` junto a `-d`, OpenSSL primero decodifica el Base64 para recuperar los bytes cifrados y después los descifra con AES. Si se omite, OpenSSL intenta descifrar el texto Base64 como si fueran los bytes cifrados y falla.

> [!WARNING]
> Problemas que encontré en este paso:
>
> - Si `message.enc` quedó en binario (cifrado **sin** `-a`) y se lo descifra **con** `-a`, OpenSSL devuelve `error reading input file`. La opción `-a` tiene que usarse en los dos pasos o en ninguno.
> - En el PDF, el comando de descifrado tiene `–a` con un guion largo (–) en vez de un guion común (-). Si se copia y pega, OpenSSL no reconoce la opción.

## Parte 3: Criptografía asimétrica

### Paso 1: Generar el par de claves

Alice y Bob generan cada uno su clave privada RSA de 1024 bits, protegida con AES-128 y una passphrase:

```bash
openssl genrsa -aes128 -out alice_private.pem 1024
openssl genrsa -aes128 -out bob_private.pem 1024
ls -l alice_private.pem
file alice_private.pem
head alice_private.pem
```

![Clave privada encriptada](https://github.com/user-attachments/assets/960f0c31-09de-41ea-8548-97316001705c)

> [!NOTE]
> En el tutorial `file` responde `PEM RSA private key`, pero en mi VM responde `ASCII text`. La clave está bien generada: lo que cambia es el formato. OpenSSL 3 guarda las claves en formato **PKCS#8** (`-----BEGIN ENCRYPTED PRIVATE KEY-----`), mientras que OpenSSL 1.x usaba el formato tradicional de RSA (`-----BEGIN RSA PRIVATE KEY-----` con los encabezados `Proc-Type` y `DEK-Info`). El comando `file` no reconoce el encabezado de PKCS#8. Para obtener el formato del tutorial se puede agregar `-traditional` a `genrsa`.

Aunque se genera un solo archivo, la clave privada contiene toda la información del par, incluida la parte pública. Se puede ver con:

```bash
openssl rsa -in alice_private.pem -noout -text
```

![Contenido de la clave privada](https://github.com/user-attachments/assets/7bb21184-433a-4186-9408-ab982f47b276)

Se ven todos los componentes de la tabla de RSA: `modulus`, `publicExponent` (65537), `privateExponent`, `prime1`, `prime2`, etc.

### Paso 2: Extraer la clave pública

```bash
openssl rsa -in alice_private.pem -pubout > alice_public.pem
ls -l *.pem
cat alice_public.pem
```

![Clave pública](https://github.com/user-attachments/assets/ff740291-9980-45aa-af67-48b8b88234ef)

Los permisos confirman la diferencia de roles: las claves privadas quedan en `-rw-------` (solo el dueño puede leerlas) y la pública en `-rw-r--r--` (cualquiera puede leerla). La clave pública además es mucho más chica (272 bytes contra 1074).

```bash
openssl rsa -in alice_public.pem -pubin -noout -text
```

![Contenido de la clave pública](https://github.com/user-attachments/assets/dfded4a1-165f-4d3d-9eb1-e00afad32ca3)

La clave pública tiene solo dos valores: el **módulo** y el **exponente**. Comparándola con la captura anterior, el `Modulus` es exactamente el mismo que el `modulus` de la clave privada (`00:c5:52:28:24:32:28:ad...`), lo que demuestra que las dos claves forman un par.

### Paso 3: Intercambiar mensajes cifrados

Alice escribe un mensaje, lo cifra con la **clave pública de Bob** y Bob lo descifra con **su clave privada**:

```bash
echo "vim o emacs?" > top_secret.txt
openssl rsautl -encrypt -inkey bob_public.pem -pubin -in top_secret.txt -out top_secret.enc
hexdump -C ./top_secret.enc
openssl rsautl -decrypt -inkey bob_private.pem -in top_secret.enc
```

![Cifrado asimétrico](https://github.com/user-attachments/assets/30d11a49-b911-4f59-ad05-b90193a49d18)

- El archivo cifrado ocupa **128 bytes** (`0x80` en el `hexdump`), que es el tamaño de la clave de 1024 bits. RSA siempre produce una salida del tamaño del módulo, sin importar lo corto que sea el mensaje.
- Para descifrar, OpenSSL pide la passphrase de `bob_private.pem`, porque la clave privada está cifrada con AES-128.
- El mensaje recuperado es `vim o emacs?`.

> [!NOTE]
> OpenSSL 3 muestra `The command rsautl was deprecated in version 3.0. Use 'pkeyutl' instead.`. El comando sigue funcionando. Su reemplazo es `openssl pkeyutl`, que acepta los mismos parámetros en este caso.

Para responderle, Bob hace el mismo proceso pero usando la **clave pública de Alice**.

## Parte 4: Cifrado de archivos grandes (cifrado híbrido)

Se implementa el esquema híbrido descrito en la teoría. Los comandos del PDF tienen dos errores que impiden que funcione correctamente, así que los corregí:

1. **Falta `-pbkdf2` al descifrar.** Se cifra con `-pbkdf2` y se descifra sin él, así que las claves AES derivadas no coinciden y OpenSSL devuelve `bad decrypt`.
2. **`-k key.bin` no lee el archivo.** `-k` toma el texto siguiente como contraseña literal, así que la contraseña era la palabra `"key.bin"` y no los 32 bytes aleatorios. El cifrado "funcionaba", pero la parte RSA no protegía nada. Para usar el contenido del archivo se usa `-pass file:key.bin`.

Comandos corregidos:

```bash
# a) Generar la clave aleatoria R
openssl rand -base64 32 > key.bin

# b) Cifrar R con la clave pública de Bob: Asym(Bob_public, R)
openssl pkeyutl -encrypt -pubin -inkey bob_public.pem -in key.bin -out key.bin.enc

# c) Cifrar el archivo F con R: EF = Sym(F, R)
echo "mensaje secreto" > file_secret
openssl aes-256-cbc -a -pbkdf2 -salt -in file_secret -out file_secret.enc -pass file:key.bin

# Se borra R en claro: a Bob solo se le envían key.bin.enc y file_secret.enc
rm key.bin

# d) Bob recupera R con su clave privada
openssl pkeyutl -decrypt -inkey bob_private.pem -in key.bin.enc -out key.bin

# e) Bob recupera F con R
openssl aes-256-cbc -d -a -pbkdf2 -in file_secret.enc -out file_secret.dec -pass file:key.bin
cat file_secret.dec
```

![Cifrado híbrido](https://github.com/user-attachments/assets/93d00a91-8208-4ab2-9dfd-f3dd38a63974)

El archivo se recupera correctamente (`mensaje secreto`). Guardé el resultado en `file_secret.dec` en lugar de `file_secret` para no pisar el original y poder compararlos.

## Resumen

| Técnica    | Clave(s)                        | Ventaja                                               | Desventaja                                 |
| ---------- | ------------------------------- | ----------------------------------------------------- | ------------------------------------------ |
| Simétrica  | Una sola clave compartida       | Rápida, sirve para cualquier tamaño                   | Hay que compartir la clave de forma segura |
| Asimétrica | Par pública / privada           | No hace falta compartir un secreto                    | Lenta y limitada al tamaño de la clave     |
| Híbrida    | Clave simétrica cifrada con RSA | Combina la velocidad de AES con el intercambio de RSA | Más pasos y archivos que manejar           |

> [!IMPORTANT]
> Como aclara el propio laboratorio, estos métodos son solo educativos: la derivación de claves por defecto es débil, `aes-256-cbc` no garantiza la **integridad** del archivo (alguien podría modificarlo sin que se detecte) y RSA de 1024 bits ya no se considera seguro (hoy se recomienda al menos 2048 bits). Para uso real conviene usar herramientas como GPG o `age`.
