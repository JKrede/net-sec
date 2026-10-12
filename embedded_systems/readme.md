# Seguridad en sistemas embebidos

## Introducción

La mayoría de las computadoras que existen no son PCs ni servidores, sino sistemas embebidos: computadoras "invisibles" que forman parte de otro equipo y cumplen una función específica. Controlan el motor y los frenos de un auto, un marcapasos, un router, una cámara de seguridad, un semáforo o el PLC de una planta industrial.

Durante mucho tiempo estos sistemas funcionaron aislados, y su diseño se enfocó en la seguridad física (safety): que no fallen, y que si fallan lo hagan de forma controlada. Hoy la mayoría se conecta a una red y, tarde o temprano, a internet. Eso los expone a los mismos ataques que cualquier otra computadora, pero con dos agravantes: muchas veces no fueron diseñados pensando en la ciberseguridad (security), y un ataque exitoso puede tener consecuencias físicas, poniendo en riesgo la salud o la vida de las personas.

En este trabajo se revisan los problemas de seguridad más frecuentes de estos sistemas, se analizan incidentes reales y se implementan sobre un microcontrolador ESP32 tres herramientas criptográficas (hash, cifrado simétrico y firma digital), para demostrar que incorporarlas no es costoso ni lento.

# Marco teórico

## Sistemas embebidos, CPS e IoT

Un **sistema embebido** es un sistema de cómputo (hardware y software) integrado dentro de un dispositivo mayor, diseñado para cumplir una tarea específica, generalmente con recursos limitados y muchas veces con requisitos de tiempo real.

Cuando estos sistemas interactúan con el mundo físico, sensando y actuando sobre él, se habla de **sistemas ciberfísicos** (CPS, Cyber Physical Systems). La **Internet de las Cosas** (IoT) es la versión más conocida de este concepto: dispositivos embebidos conectados a internet.

Están presentes en prácticamente todos los sectores:

| Sector           | Ejemplos                                           |
| ---------------- | -------------------------------------------------- |
| Industria (IIoT) | SCADA, PLCs, robots, líneas de producción          |
| Ciudades         | Semáforos, iluminación, sensores urbanos           |
| Energía          | Medidores inteligentes, subestaciones              |
| Automotriz       | ECUs, asistencia a la conducción, autos conectados |
| Salud            | Marcapasos, bombas de infusión, monitores          |
| Hogar            | Cámaras, routers, electrodomésticos, wearables     |

Muchos de estos sectores son **infraestructuras críticas**: servicios esenciales cuya interrupción pone en riesgo el funcionamiento de una sociedad (energía, agua, salud, transporte, comunicaciones, etc.).

### El problema según la industria

El trabajo _Aportes para una Internet de las Cosas Seguras_ [1] analiza la encuesta 2017 de Barr Group a más de 1.700 ingenieros de sistemas embebidos. Sus principales conclusiones son:

- La industria construye sistemas embebidos que, ante una falla, pueden poner en riesgo la salud y la vida de las personas, sin tener en cuenta aspectos fundamentales para minimizar ese riesgo.
- El 60% de los dispositivos que se construyen se conectará a internet en algún momento de su vida útil.
- Un tercio de los proyectos no tiene en cuenta las regulaciones y recomendaciones de seguridad, incluso en sistemas críticos.
- La seguridad es una preocupación, pero no se la trata como un requerimiento funcional.

El paper atribuye este escenario a tres causas: la prioridad del _time to market_ (llegar primero al mercado), la cultura de "entregar" por sobre "entregar algo seguro", y la consecuente falta de herramientas y métodos para diseñar sistemas seguros. Como respuesta propone tres ideas:

1. **La seguridad como requerimiento funcional**, analizada desde el inicio con _casos de mal uso_ (misuse cases): pensar cómo un atacante intentaría abusar de cada caso de uso y definir requerimientos que lo mitiguen.
2. **Construcción de modelos** del sistema (por ejemplo con SysML) que incluyan los aspectos de seguridad desde etapas tempranas.
3. **Uso de patrones de seguridad**: soluciones probadas a problemas conocidos, en lugar de inventar soluciones propias.

## Desafíos específicos de los sistemas embebidos

Los sistemas embebidos tienen características que hacen que asegurarlos sea más difícil que asegurar una PC o un servidor:

- **Recursos limitados:** CPU, memoria y energía restringidas, por lo que la criptografía pesada es costosa.
- **Larga vida útil:** dispositivos que pasan 10 a 20 años en campo, donde actualizarlos es difícil o imposible.
- **Acceso físico:** muchos están expuestos, y el atacante puede tocar el hardware.
- **Heterogeneidad:** múltiples arquitecturas, sistemas operativos de tiempo real (RTOS), dispositivos sin MMU ni sistema operativo estándar.
- **Cadena de suministro:** firmware y componentes de terceros, con riesgo de puertas traseras o clones.
- **Costo vs. seguridad:** el costo unitario y el time to market suelen priorizarse sobre la seguridad.

## Principales problemas de seguridad (INCIBE)

INCIBE [2] señala que los sistemas embebidos, como la mayoría del equipamiento industrial, están pensados para ser seguros a nivel físico (watchdogs, hardware redundante, programación tolerante a fallos), pero no a nivel de ciberseguridad. Los problemas más frecuentes que identifica son:

| Problema                                 | Descripción                                                                                      | Riesgo                                                               |
| ---------------------------------------- | ------------------------------------------------------------------------------------------------ | -------------------------------------------------------------------- |
| Compartición de secretos                 | Las aplicaciones de un mismo fabricante comparten contraseñas para comunicarse entre sí          | Con una sola credencial se accede a múltiples aplicaciones           |
| Certificados privados                    | Uso de certificados autofirmados, sin respaldo de una entidad de confianza                       | El atacante usa certificados propios para suplantar servicios        |
| Contraseñas embebidas y puertas traseras | Contraseñas fijas en el firmware, iguales en todos los equipos, y accesos ocultos del fabricante | Acceso y control del dispositivo                                     |
| Fallos de código libre                   | Reutilización de código (muchas veces libre) que no se revisa                                    | Vulnerabilidades heredadas como punto de entrada                     |
| Criptografía débil o inexistente         | Comunicaciones por Telnet o HTTP; cifrado solo en las versiones más avanzadas                    | Captura e interpretación del tráfico                                 |
| Autenticación débil o inexistente        | Acceso con un PIN o sin ningún control                                                           | Fuerza bruta y acceso no autorizado                                  |
| Explotación del software servidor        | Servidores web/HMI/gestión en el propio dispositivo, vulnerables a XSS, inyección SQL, etc.      | Denegación de servicio o ejecución remota de código                  |
| Comunicaciones de entrada/salida         | Puertos de comunicación que no se pueden deshabilitar aunque no se usen                          | Puntos de entrada para tomar el control                              |
| Explotación de la lógica en escalera     | El programa de control (ej. ladder logic de un PLC) no está protegido contra modificaciones      | Acciones distintas a las diseñadas, con riesgo para vidas y ambiente |
| DoS y DDoS                               | Sin control de tramas ni validación de entradas; un exceso de peticiones satura el dispositivo   | Interrupción del servicio                                            |

## Tipos de ataques

Además de los ataques de software tradicionales, los sistemas embebidos están expuestos a ataques que aprovechan el acceso físico al hardware:

| Tipo                     | Descripción                                                                                                                   |
| ------------------------ | ----------------------------------------------------------------------------------------------------------------------------- |
| **Software**             | Explotación de vulnerabilidades (buffer overflow, use-after-free), credenciales por defecto, clonado, DoS                     |
| **Canal lateral**        | Inferir secretos a partir de señales físicas: consumo de energía (SPA/DPA), emisiones electromagnéticas, tiempos de ejecución |
| **Inyección de fallos**  | Provocar errores controlados (voltage/clock glitching, láser, pulsos EM) para saltear verificaciones o extraer claves         |
| **Físicos e intrusivos** | Microprobing, decapsulación del chip, lectura de la memoria Flash por interfaces de depuración (JTAG/SWD)                     |
| **Cadena de suministro** | Componentes o firmware maliciosos introducidos en la fabricación, distribución o actualizaciones                              |

## Atributos de seguridad y herramientas criptográficas

Cada atributo de seguridad se garantiza con una herramienta criptográfica distinta:

| Atributo         | Qué garantiza                                     | Herramienta                          |
| ---------------- | ------------------------------------------------- | ------------------------------------ |
| Confidencialidad | Solo los autorizados pueden leer la información   | Cifrado (AES)                        |
| Integridad       | Los datos no fueron alterados                     | Hash, MAC, firmas                    |
| Autenticidad     | El origen de los datos es quien dice ser          | HMAC, firmas, certificados           |
| No repudio       | El autor no puede negar haber generado el mensaje | Firmas digitales + PKI               |
| Disponibilidad   | El sistema sigue operativo ante ataques o fallos  | Diseño resiliente, mitigación de DoS |

### Funciones hash

Una función hash criptográfica transforma datos de cualquier tamaño en un resumen (digest) de tamaño fijo. Sus propiedades son:

- **Determinista:** la misma entrada siempre produce el mismo hash.
- **Irreversible:** a partir del hash no se puede obtener la entrada.
- **Efecto avalancha:** cambiar un solo bit de la entrada cambia aproximadamente la mitad de los bits del hash.
- **Resistente a colisiones:** es inviable encontrar dos entradas con el mismo hash.

Se recomienda SHA-256 o SHA-3. MD5 y SHA-1 están rotos y no deben usarse con fines de seguridad, y un CRC32 solo sirve para detectar errores de transmisión, no manipulaciones intencionales.

### Cifrado simétrico

Usa la misma clave para cifrar y descifrar. Es rápido y eficiente, y muchos microcontroladores lo implementan en hardware. El estándar es **AES**, con claves de 128 o 256 bits.

El modo de operación importa tanto como el algoritmo. Se recomiendan los modos **autenticados** como **GCM** o CCM, que además de cifrar generan un _tag_ que permite detectar si el mensaje cifrado fue alterado. El modo ECB no debe usarse, porque bloques iguales producen cifrados iguales.

Su principal desafío es la **distribución de la clave**: ambas partes tienen que compartirla sin que nadie más la conozca.

### Firma digital

Usa un par de claves: una **privada**, que solo conoce el firmante, y una **pública**, que se puede distribuir. Para firmar, se calcula el hash del mensaje y se lo firma con la clave privada; cualquiera con la clave pública puede verificar la firma. Aporta integridad, autenticidad y no repudio.

En sistemas embebidos se prefieren las **curvas elípticas** (ECDSA, Ed25519) sobre RSA: con claves de 256 bits ofrecen una seguridad equivalente a RSA de 3072 bits, con operaciones más rápidas y menos memoria.

### Secure Boot y firmware firmado

Es la aplicación más importante de la firma digital en sistemas embebidos:

1. **En fábrica:** se genera un par de claves. El firmware se firma con la clave privada, que nunca sale del fabricante, y la clave pública se graba de forma inmutable en el dispositivo (ROM o eFuses).
2. **En cada arranque:** el bootloader calcula el hash del firmware y verifica la firma con la clave pública. Si es válida, arranca; si no, se detiene o entra en modo seguro.
3. **En las actualizaciones (OTA):** el nuevo firmware también debe estar firmado, y el dispositivo lo verifica antes de instalarlo.

Así, aunque un atacante logre modificar el firmware, no puede generar una firma válida sin la clave privada del fabricante.

# Incidentes de seguridad en sistemas embebidos

## Mirai (2016): cámaras y routers con contraseñas por defecto

**Qué pasó:** Mirai fue un malware que recorría internet buscando dispositivos IoT (cámaras IP, grabadores DVR, routers) con el servicio **Telnet** expuesto, e intentaba entrar con una lista de unas 60 combinaciones de usuario y contraseña de fábrica (`root/xc3511`, `admin/admin`, etc.). Con los dispositivos infectados formó una botnet de cientos de miles de equipos, que se usó para lanzar algunos de los ataques DDoS más grandes de la época: contra el sitio del periodista Brian Krebs (más de 600 Gbps), contra el proveedor OVH y, en octubre de 2016, contra el proveedor de DNS **Dyn**, lo que dejó inaccesibles durante horas a Twitter, Netflix, Reddit, GitHub y otros sitios.

**Problemas de INCIBE involucrados:** contraseñas embebidas, autenticación débil, criptografía inexistente (Telnet), comunicaciones de entrada/salida (puertos innecesarios abiertos) y DDoS.

**Cómo se pudo evitar:**

- No usar contraseñas por defecto iguales para todos los equipos: generar una contraseña única por dispositivo (impresa en la etiqueta) u obligar a cambiarla en el primer uso.
- No exponer Telnet. Si se necesita administración remota, usar SSH con autenticación fuerte y deshabilitarla por defecto.
- Limitar los intentos de login (algo como fail2ban en el propio dispositivo).
- Contar con un mecanismo de actualización de firmware para corregir el problema una vez descubierto.

## Jeep Cherokee (2015): control remoto de un auto

**Qué pasó:** los investigadores Charlie Miller y Chris Valasek demostraron que podían tomar el control de un Jeep Cherokee de forma remota a través de la red celular. El sistema multimedia **Uconnect** tenía un servicio (D-Bus) expuesto en la red celular sin autenticación, que permitía ejecutar comandos. Desde ahí reprogramaron el firmware de un chip conectado al **bus CAN** del vehículo, que no verificaba la firma del firmware, y pudieron enviar mensajes a los sistemas del auto: controlar el aire acondicionado, la radio y los limpiaparabrisas, pero también cortar el motor y, a baja velocidad, manejar la dirección y los frenos. Fiat Chrysler tuvo que llamar a revisión a 1,4 millones de vehículos.

**Problemas de INCIBE involucrados:** autenticación inexistente, comunicaciones de entrada/salida (servicio interno expuesto a la red celular), explotación del software servidor y falta de verificación de integridad del firmware.

**Cómo se pudo evitar:**

- No exponer servicios internos a la red celular y exigir autenticación en todos ellos.
- **Firmware firmado:** si el chip hubiera verificado la firma digital del firmware antes de aceptarlo (como en el tercer ejemplo del práctico), no se lo podría haber reprogramado con código malicioso.
- Segmentar la red del vehículo: separar el sistema multimedia, conectado al exterior, de los sistemas críticos (frenos, dirección), con un gateway que filtre los mensajes.
- Autenticar los mensajes del bus CAN (por ejemplo con MACs, como propone AUTOSAR SecOC).

## Stuxnet (2010): sabotaje de PLCs industriales

**Qué pasó:** Stuxnet fue un gusano diseñado para sabotear la planta de enriquecimiento de uranio de Natanz, en Irán. Se propagaba por memorias USB (la planta no estaba conectada a internet), aprovechaba cuatro vulnerabilidades desconocidas de Windows (zero-days) y usaba drivers firmados con certificados robados a dos empresas taiwanesas para no despertar sospechas. Una vez en la red, buscaba las computadoras con el software Siemens Step 7 y **modificaba la lógica de los PLCs** que controlaban las centrifugadoras, alterando su velocidad para dañarlas. Mientras tanto, les mostraba a los operadores valores normales grabados previamente. Se estima que destruyó alrededor de mil centrifugadoras, y es considerado el primer malware conocido que causó daño físico a una infraestructura industrial.

**Problemas de INCIBE involucrados:** explotación de la lógica en escalera, comunicaciones de entrada/salida (USB), falta de autenticación entre la estación de ingeniería y los PLCs, y certificados comprometidos.

**Cómo se pudo evitar:**

- **Firmar y verificar el programa de los PLCs:** si el PLC solo aceptara lógica firmada por el fabricante o el integrador, no se la podría haber reemplazado.
- Autenticar las comunicaciones entre la estación de ingeniería y los PLCs.
- Controlar los dispositivos removibles (bloqueo de USB, estaciones de escaneo).
- Monitorear las variables del proceso de forma independiente del sistema de control, para detectar discrepancias entre lo que se muestra y lo que realmente ocurre.
- Proteger las claves privadas de firma de código en módulos de hardware (HSM).

> [!NOTE]
> Stuxnet muestra que estar desconectado de internet (air gap) no alcanza: la seguridad debe estar en el propio dispositivo.

## Marcapasos de St. Jude Medical (2016-2017): dispositivos médicos

**Qué pasó:** en 2016, la empresa de investigación MedSec reportó vulnerabilidades en los marcapasos y desfibriladores implantables de St. Jude Medical (luego Abbott). La comunicación por radiofrecuencia entre el implante y el transmisor domiciliario del paciente no estaba adecuadamente autenticada, por lo que un atacante cercano con el equipo apropiado podía enviarle comandos al dispositivo: modificar el ritmo de estimulación o agotar rápidamente su batería. En 2017, la FDA emitió un aviso de seguridad y unos 465.000 marcapasos debieron recibir una actualización de firmware, que se aplicaba en el consultorio médico.

**Problemas de INCIBE involucrados:** autenticación débil, criptografía débil en las comunicaciones y dificultad de actualización (dispositivo implantado con una vida útil de muchos años).

**Cómo se pudo evitar:**

- Autenticar y cifrar las comunicaciones inalámbricas con criptografía estándar (por ejemplo AES-GCM, como en el segundo ejemplo del práctico), con claves únicas por dispositivo.
- Considerar la seguridad como requerimiento desde el diseño: en un dispositivo implantado, corregir un error después es mucho más costoso y riesgoso.
- Prever desde el inicio un mecanismo de actualización seguro (firmado).

# Práctico: criptografía en un microcontrolador

## Entorno

Se implementan tres herramientas criptográficas sobre un **ESP32**, un microcontrolador muy usado en IoT, programado con el framework de Arduino. Como no se cuenta con la placa física, se utiliza el simulador online **Wokwi** (https://wokwi.com), que emula el ESP32 y muestra la salida del monitor serie. El mismo código funciona sin cambios en una placa real.

Para la criptografía se usa **mbedTLS**, una de las librerías recomendadas para sistemas embebidos. Viene incluida en el core de Arduino para ESP32, por lo que no hace falta instalar nada. Además, el ESP32 tiene aceleradores por hardware para AES, SHA y RSA/ECC, y un generador de números aleatorios por hardware (TRNG), que se usa para generar las claves.

El código completo está en [`crypto_esp32/crypto_esp32.ino`](crypto_esp32/crypto_esp32.ino) y la configuración del circuito de Wokwi en [`crypto_esp32/diagram.json`](crypto_esp32/diagram.json).

### Cómo ejecutarlo en Wokwi

1. Entrar a https://wokwi.com y crear un proyecto nuevo con **ESP32** (Arduino).
2. Reemplazar el contenido de `sketch.ino` por el de `crypto_esp32.ino`.
3. Presionar **Start Simulation** (▶) y ver el resultado en el monitor serie.

![Proyecto en wokwi](https://github.com/user-attachments/assets/a0d2fc1b-f700-46df-80f2-1a3205a224d1)

### Escenario

Los ejemplos simulan un nodo de sensores industrial que:

1. Calcula el hash de sus lecturas para verificar su integridad.
2. Envía sus lecturas cifradas a un servidor.
3. Recibe una actualización de firmware y verifica que sea auténtica antes de instalarla.

Generar los números aleatorios con el TRNG por hardware es importante: si las claves o los IV fueran predecibles, toda la criptografía sería inútil.

```cpp
static int rng(void *ctx, unsigned char *out, size_t len) {
  esp_fill_random(out, len);
  return 0;
}
```

## Ejemplo 1: Hash SHA-256 (integridad)

Se calcula el SHA-256 de dos lecturas del sensor que difieren en un solo carácter (`23.5` vs `23.6`) y se cuentan los bits distintos entre ambos hashes:

```cpp
static void sha256(const uint8_t *data, size_t len, uint8_t out[32]) {
  mbedtls_md(mbedtls_md_info_from_type(MBEDTLS_MD_SHA256), data, len, out);
}
```

Resultado:

![Solo hashing](https://github.com/user-attachments/assets/a9c4ffe3-5fb7-46fe-a77d-6cbb1419928f)

Se observan dos propiedades de las funciones hash:

- **Determinismo:** como el hash no depende de ningún valor aleatorio, se obtienen exactamente estos valores en cada ejecución y en cualquier dispositivo. Por eso quien reciba la lectura puede recalcular el hash y compararlo.
- **Efecto avalancha:** un cambio mínimo en la entrada (un solo dígito) modifica 129 de los 256 bits del hash, prácticamente la mitad. Es imposible "ajustar" un hash para que coincida con un mensaje modificado.

Es importante notar que un hash solo es efectivo para detecta modificaciones accidentales o comparar contra un valor confiable. Si un atacante puede modificar el mensaje, también puede recalcular el hash. Para protegerse de eso se necesita una clave: HMAC, cifrado autenticado (ejemplo 2) o firma digital (ejemplo 3).

## Ejemplo 2: Cifrado simétrico AES-256-GCM (confidencialidad e integridad)

Se cifra una lectura del sensor con AES de 256 bits en modo GCM. Además del mensaje, se autentica el identificador del nodo (`nodo-07`) como **datos asociados (AAD)**: viajan sin cifrar, porque el servidor necesita saber de qué nodo viene el mensaje, pero cualquier modificación se detecta.

```cpp
mbedtls_gcm_setkey(&gcm, MBEDTLS_CIPHER_ID_AES, clave, 256);
mbedtls_gcm_crypt_and_tag(&gcm, MBEDTLS_GCM_ENCRYPT, len, iv, sizeof(iv),
                          aad, strlen(aad), mensaje, cifrado, sizeof(tag), tag);
...
int ret = mbedtls_gcm_auth_decrypt(&gcm, len, iv, sizeof(iv), aad, strlen(aad),
                                   tag, sizeof(tag), cifrado, descifrado);
```

Después de descifrar correctamente, se simula un atacante que modifica un solo bit del mensaje cifrado en tránsito y se intenta descifrar nuevamente:

![Hashing y Cifrado simetrico](https://github.com/user-attachments/assets/2dab38cc-eb28-4a9c-9de4-c5e522009add)

Observaciones:

- El texto cifrado tiene el mismo largo que el original (GCM no agrega padding) y no revela nada de su contenido.
- La clave y el IV se generan con el TRNG, así que cambian en cada ejecución, y con ellos el texto cifrado y el tag.
- Al modificar un solo bit, el tag deja de coincidir y el mensaje se **rechaza**. Esto es lo que diferencia a un modo autenticado como GCM de un modo como CBC o CTR, donde el mensaje modificado se descifraría igual, como basura o, peor, con un cambio controlado por el atacante.

> [!IMPORTANT]
> En GCM el IV (nonce) **nunca debe repetirse** con la misma clave: si se repite, un atacante puede recuperar información de los mensajes y falsificar tags. En un producto real, además, la clave no se genera en cada arranque: se acuerda con el servidor (por ejemplo con TLS o un esquema híbrido) y es **única por dispositivo**, para que comprometer un equipo no comprometa a todos.

## Ejemplo 3: Firma digital ECDSA P-256 (autenticidad e integridad)

Se simula el proceso de Secure Boot / actualización firmada:

1. **Fabricante:** genera un par de claves de curva elíptica P-256, calcula el SHA-256 del firmware y lo firma con la clave privada.
2. **Dispositivo:** antes de instalar la actualización, verifica la firma con la clave pública.
3. **Atacante:** modifica el firmware (le agrega una puerta trasera) pero, como no tiene la clave privada, no puede generar una firma nueva, así que reutiliza la original.

```cpp
mbedtls_ecp_group_load(&grp, MBEDTLS_ECP_DP_SECP256R1);
mbedtls_ecp_gen_keypair(&grp, &d, &Q, rng, NULL);           // d: privada, Q: pública

sha256(firmware, strlen(firmware), hash);
mbedtls_ecdsa_sign(&grp, &r, &s, &d, hash, sizeof(hash), rng, NULL);  // fabricante

int ret = mbedtls_ecdsa_verify(&grp, hash, sizeof(hash), &Q, &r, &s); // dispositivo
```

Resultado:

![Firma digital con criptografia asimetrica](https://github.com/user-attachments/assets/80ff87b4-d862-4909-a09e-6165a9d49eb0)

Observaciones:

- La clave pública ocupa solo 65 bytes (un prefijo `04` y las coordenadas X e Y del punto, de 32 bytes cada una) y la firma 64 bytes (`r` y `s`). Con RSA, para una seguridad equivalente, la clave pública y la firma tendrían 384 bytes cada una.
- El hash del firmware es siempre el mismo, pero la firma cambia en cada ejecución, porque ECDSA usa un número aleatorio en cada firma. Por eso es fundamental contar con un buen generador aleatorio: si ese número se repite o es predecible, se puede calcular la clave privada (así se rompió la firma de la PlayStation 3 en 2010).
- El firmware modificado se rechaza: este es exactamente el control que les faltó al Jeep Cherokee y a los PLCs atacados por Stuxnet.
- En un producto real la clave privada **nunca** está en el dispositivo: se guarda en un HSM del fabricante, y en el dispositivo solo se graba la clave pública, idealmente en memoria inmutable (eFuses).

## Costo de la seguridad

La consigna plantea que la "necesidad de salir rápido al mercado" no es una excusa válida para no implementar seguridad. El programa mide el tiempo de cada operación en el ESP32 simulado en Wokwi:

| Operación                             | Tiempo en Wokwi        |
| ------------------------------------- | ---------------------- |
| SHA-256 de una lectura                | 240 µs                 |
| Cifrado AES-256-GCM                   | 541 µs                 |
| Descifrado y verificación AES-256-GCM | 576 µs                 |
| Generación del par de claves ECDSA    | 1.519.251 µs (≈ 1,5 s) |
| Firma ECDSA                           | 1.685.798 µs (≈ 1,7 s) |
| Verificación ECDSA                    | 3.356.591 µs (≈ 3,4 s) |

> [!NOTE]
> Estos tiempos corresponden al simulador, que no reproduce exactamente la velocidad del chip real ni de sus aceleradores criptográficos por hardware. En un ESP32 físico las operaciones de curva elíptica tardan del orden de decenas de milisegundos. Lo que sí es representativo es la **proporción** entre las operaciones.

Se observan dos órdenes de magnitud muy distintos:

- **Hash y cifrado simétrico:** menos de un milisegundo por mensaje. Se pueden aplicar a cada lectura del sensor sin afectar el funcionamiento del dispositivo.
- **Criptografía asimétrica (ECDSA):** miles de veces más lenta. Es coherente con la teoría: generar una clave y firmar requieren una multiplicación escalar sobre la curva, y verificar requiere dos, por eso tarda aproximadamente el doble.

Esta diferencia explica por qué en la práctica se combinan ambas, como en el esquema híbrido: la criptografía asimétrica se usa en pocas operaciones puntuales (verificar el firmware al arrancar o al actualizar, o acordar una clave de sesión) y la simétrica para todo el tráfico de datos. Incluso los 3,4 segundos medidos en el simulador son aceptables para una verificación que ocurre una vez por actualización, frente al riesgo de instalar un firmware malicioso.

En cuanto al esfuerzo de desarrollo, los tres ejemplos ocupan unas 200 líneas, incluyendo comentarios y salida por pantalla, usando una librería que ya viene incluida con la plataforma. El costo de no hacerlo, como muestran los incidentes analizados, se mide en millones de dispositivos comprometidos, llamados a revisión y riesgo para la vida de las personas.

# Conclusión

Los sistemas embebidos ya no son equipos aislados: están conectados, controlan procesos físicos y, muchas veces, forman parte de infraestructuras críticas. Sin embargo, como muestran la encuesta citada en [1] y los problemas que releva INCIBE, siguen diseñándose con criterios de seguridad física (que no fallen) pero no de ciberseguridad (que no puedan ser atacados).

Los incidentes analizados tienen causas que se repiten: contraseñas por defecto, servicios expuestos sin autenticación, comunicaciones sin cifrar y firmware o lógica de control que se puede modificar sin verificación. Ninguna de ellas requiere tecnología nueva para resolverse: son problemas conocidos con soluciones conocidas. Mirai se hubiera evitado con credenciales únicas, y el Jeep y Stuxnet con código firmado y verificado.

El práctico demuestra que esas soluciones están al alcance de cualquier proyecto. En un microcontrolador de bajo costo, con una librería incluida en la plataforma y pocas líneas de código, se implementaron:

- **Hash (SHA-256)** para verificar la integridad de los datos.
- **Cifrado autenticado (AES-256-GCM)** para que las lecturas viajen de forma confidencial y cualquier manipulación sea detectada.
- **Firma digital (ECDSA P-256)** para que el dispositivo solo acepte firmware auténtico.

El "time to market" no justifica omitirlas. Lo que sí requiere esfuerzo es lo que rodea a la criptografía: gestionar las claves (únicas por dispositivo, nunca embebidas en el código, protegidas por hardware), prever desde el diseño un mecanismo de actualización segura y tratar la seguridad como un requerimiento funcional más, analizando cómo un atacante intentaría abusar del sistema. Cuanto más crítico es el dispositivo y más larga su vida útil, más sentido tiene invertir en estas medidas desde el principio, porque corregirlas una vez que el equipo está en campo (o implantado en un paciente) es mucho más difícil y costoso.

# Referencias

1. Solinas, M., Jorge, J. A., Casanovas, E., Tapia, C. _Aportes para una Internet de las Cosas Seguras_. CONAIISI 2017. https://www.researchgate.net/publication/320452544_Aportes_para_una_Internet_de_las_Cosas_Seguras
2. INCIBE-CERT. _Introducción a los sistemas embebidos_. https://www.incibe.es/incibe-cert/blog/introduccion-los-sistemas-embebidos
3. Antonakakis, M. et al. _Understanding the Mirai Botnet_. USENIX Security Symposium, 2017.
4. Mbed TLS. https://www.trustedfirmware.org/projects/mbed-tls/
5. Wokwi – ESP32 Simulator. https://wokwi.com
