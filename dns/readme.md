# DNS (Domain Name Server)

DNS (Domain Name System / Sistema de Nombres de Dominio) es un protocolo y sistema jerárquico distribuido que se encarga de traducir nombres de dominio legibles por humanos (como www.google.com) a direcciones IP (como 142.250.65.238), que son las que realmente usan las computadoras y routers para comunicarse en una red. Ademas de este servicio ofrece otras funciones:

1. Resolución de nombres (Name Resolution)

Este es el que mencionamos anteriormente, convierte nombres de dominio en direcciones IP (y viceversa, en el caso de resolución inversa).

2. Resolución inversa (Reverse DNS)

Permite obtener el nombre de dominio a partir de una dirección IP (útil para verificación de servidores de correo, seguridad, etc.).

3. Distribución de carga (Load Balancing)

Un mismo nombre de dominio puede apuntar a varias direcciones IP, permitiendo distribuir el tráfico entre varios servidores.

4. Servicios de correo electrónico (Registros MX)

DNS almacena registros MX (Mail Exchange) que indican qué servidores son responsables de recibir el correo electrónico de un dominio.

5. Redundancia y alta disponibilidad

Gracias a su estructura jerárquica y distribuida (servidores raíz, TLD, autoritativos), DNS es resistente a fallos: si un servidor cae, otros pueden responder.

6. Caché de resultados

Los servidores y dispositivos guardan temporalmente las respuestas DNS para acelerar futuras consultas y reducir la carga en la red.

7. Seguridad (DNSSEC)

Extensión de seguridad que permite verificar la autenticidad de las respuestas DNS, evitando ataques como el DNS spoofing o cache poisoning.

## Tipos de registros

| Tipo de registro | Función                                                                                |
| ---------------- | -------------------------------------------------------------------------------------- |
| A                | Asocia un nombre de dominio a una dirección IPv4                                       |
| AAAA             | Asocia un nombre de dominio a una dirección IPv6                                       |
| CNAME            | Crea un alias de un dominio hacia otro nombre de dominio                               |
| MX               | Indica el servidor de correo (Mail Exchange) del dominio                               |
| NS               | Indica los servidores de nombres autoritativos del dominio                             |
| PTR              | Usado para resolución inversa (IP → nombre de dominio)                                 |
| SOA              | Contiene información de autoridad sobre la zona DNS                                    |
| TXT              | Almacena información adicional (verificaciones, SPF, DKIM, etc.)                       |
| SRV              | Especifica la ubicación de servicios específicos (ej: VoIP, chat)                      |
| CAA              | Indica qué autoridades de certificación pueden emitir certificados SSL para el dominio |

# Laboratorio

Al analizar la VM observamos los siguientes datos:

| Descripción                             | Configuración             |
| --------------------------------------- | ------------------------- |
| Dirección IP                            | 192.168.0.21/24           |
| Dirección MAC                           | 08:00:27:55:44:07         |
| Dirección IP del gateway predeterminado | 255.255.255.0             |
| Dirección IP del servidor DNS           | 181.30.140.195 (fibertel) |

Desde mi maquina uso wireshark para sniffear paquetes y capturar paquetes DNS filtrando por el ip del servidor DNS

1. Comencé el sniffing de paqutes en wireshark
2. Desde la VM ingrese a google.com
3. En wireshark filtrando por ip del servidor de dns pude capturar el paquete de solicitud dns para 'google.com'

![Captura de paquetes](https://github.com/user-attachments/assets/0cef7ccd-1486-41fd-af05-d95b9a0222bf)

De los paquetes capturados se obtiene la siguiente información

| Descripción              | Resultados del Wireshark |
| ------------------------ | ------------------------ |
| Tamaño de la trama       | 81 Bytes                 |
| Dirección MAC de origen  | 10:68:38:75:d2:8d        |
| Dirección MAC de destino | 02:10:18:3b:c4:b4        |
| Dirección IP de origen   | 192.168.0.21             |
| Dirección IP de destino  | 181.30.140.195           |
| Puerto de origen         | 40109                    |
| Puerto de destino        | 53                       |

Lo cual es coincidente con lo esperado:

- La dirección MAC de origen es la de la VM, la de destino es la del router
- La ip origen es el de la VM y la de destino es la del DNS
- El puerto del origen es uno aleatorio NWK y el de destino es el predeterminado para DNS, el 53

![Obtencion de datos del paquete](https://github.com/user-attachments/assets/2d9d7943-d875-4b8e-a492-99fc7593c92e)

## Reflexion final: Uso de protocolo UDP en vez de TCP para el protocolo DNS

UDP resulta más conveniente que TCP como protocolo de transporte para DNS principalmente por su eficiencia: al no ser un protocolo orientado a conexión, no requiere establecer un enlace previo (como el three-way handshake de TCP), lo que permite resolver una consulta con un simple intercambio de petición y respuesta, reduciendo significativamente el overhead y por lo tanto la latencia. Además, como los mensajes DNS suelen ser pequeños, no necesitan las garantías de entrega ordenada, control de flujo o retransmisión que ofrece TCP, características que solo agregarían sobrecarga innecesaria para este tipo de comunicación. Esto también beneficia a los servidores DNS, que reciben una enorme cantidad de consultas por segundo: al no tener que mantener el estado de cada conexión, pueden atender muchas más solicitudes simultáneamente usando menos recursos.
