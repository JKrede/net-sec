# Laboratorio - Evaluacion de vulnerabilidades

## Objetivos

- Comprender por qué la protección del endpoint forma parte de la seguridad de una organización.
- Relacionar superficie de ataque, firewall basado en host, HIDS y evaluación de vulnerabilidades.
- Realizar una auditoría local de un host Linux utilizando Lynis.
- Detectar una falencia, corregirla y comparar el reporte antes y después.
- Realizar una evaluación con OpenVAS/Greenbone y analizar sus resultados.
- Repetir el proceso desde una distribución Linux orientada a seguridad y comparar enfoques.

### Superficie de ataque

Una superficie de ataque es el conjunto total de puntos por donde un atacante podría intentar entrar, extraer datos, o afectar un sistema, red o aplicación. De manera mas simple: es todo lo que está expuesto y que un atacante podría intentar tocar.

#### Componentes típicos

- Superficie física: Puertos USB, acceso físico a servidores, dispositivos de red, etc.
- Superficie de red: Puertos abiertos, servicios expuestos a internet, APIs, protocolos de comunicación, VPNs.
- Superficie de software: Código de aplicaciones, dependencias/librerías de terceros, formularios web, endpoints de API, funciones que procesan input del usuario.
- Superficie humana (ingeniería social): Empleados susceptibles a phishing, contraseñas débiles, falta de capacitación en seguridad.
- Superficie de configuración: Servicios mal configurados, permisos excesivos, credenciales por defecto sin cambiar, servicios innecesarios corriendo.

En nuestro caso en este laboratorio nos enfocaremos en la superficie de red, de software y de configuración

### Defensa en profundidad

No existe una única herramienta que garantice la seguridad del host, sino que se logra combinando distintos tipos de controles: por un lado, los controles preventivos que buscan reducir la superficie de ataque, por otro, los controles de detección. Ademas realizar evaluaciones continua, mediante auditorías de configuración y escáneres de vulnerabilidades, que verifica que los controles anteriores sigan siendo efectivos con el tiempo

El objetivo de combinar todos estos controles es que una falla individual (por ejemplo, que una vulnerabilidad no sea parcheada a tiempo) no implique automáticamente el compromiso total del sistema, ya que las demás capas de seguridad actúan como respaldo

#### Controles preventivos

- Actualizaciones
- El principio de mínimo privilegio
- La autenticación fuerte
- Uso y configuracion correcta de un firewall
- Desactivación de servicios innecesarios

#### Controles de detección

- logs
- Uso de un HIDS
- comprobación de seguridad
- Monitoreo en busca de actividad sospechosa

#### HIDS (Host-based Intrusion Detection System)

Un HIDS es un sistema de detección de intrusiones que se ejecuta dentro de un host individual (un servidor, una computadora) y monitorea lo que pasa adentro de esa máquina, en lugar de vigilar el tráfico de red en general (eso sería un NIDS, Network-based IDS). El HIDS monitorea logs del sistema, procesos en ejecucion, integridad de archivos, cambios en la configuracion del sistema, y algunos incluyen hasta deteccion de rootkits

A grandes rasgos un HIDS trabaja comparando el estado actual del sistema contra una línea base (baseline) previamente establecida como "normal". Si detecta una desviación; un archivo que cambió su hash, un proceso desconocido, un log con patrones raros, genera una alerta

## Auditoria con Lynis

Lynis es una herramienta para realizar auditorias de seguridad, es de codigo abierto. A continuacion se muestra el resultado de la auditoria realizada usando Lynis:

![Resultado Lynis](https://github.com/user-attachments/assets/83221c54-bf4b-4032-84f5-5c64ead70665)

En el resumen de los resultados Lynis indica una puntuacion llamada Hardening Index, este puede tomar valores entre 0 y 100, donde 100 es mas hardenizado y 0 es menos hardenizado. Es importante tener en cuenta que este no mide qué tan "seguro" está el sistema en términos absolutos, sino qué tan bien aplicadas están las prácticas de hardening que Lynis conoce y verifica. El índice por sí solo dice poco si no mirás el detalle

Lynis separa los hallazgos en categorías: los WARNING indican problemas críticos que requieren acción inmediata (por ejemplo, ausencia de un escáner de malware o login root habilitado por SSH), mientras que las SUGGESTION son recomendaciones para mejorar la configuración, no son críticas, pero ayudan a reducir la superficie de ataque

![Detecciones Lynis](https://github.com/user-attachments/assets/a3e700f3-79de-4e4d-a76b-361440e976dd)

Como vemos no hay Warnings, pero si hay 43 sugerencias de las cuales de prioridad alta (impacto en la seguridad) se pueden indentificar:

Fail2ban [DEB-0880]
Bloquea automáticamente IPs que fallan login repetidamente (fuerza bruta contra SSH, por ejemplo). Fácil de instalar, alto impacto si el sistema tiene servicios expuestos a la red.

Contraseña en GRUB [BOOT-5122]
Sin esto, cualquiera con acceso físico (o a la consola de la VM) puede arrancar en modo single-user y saltarse el login por completo. Relevante en tu caso porque estás en VirtualBox.

### Solucion de DEB-0880

En mi caso la sugerencia mas reelevante es el de no tener instalado un bloqueador de ips ante intentos repetidos de ssh. Esto se soluciona instalando fail2ban. Luego lo configure con los siguientes parametros reelevantes:

- `bantime.increment = true`
- `bantime.maxtime = 7d`
- `bantime.factor = 2`
- `bantime  = 30m`
- `findtime  = 10m`
- `maxretry = 3`

Ademas Linys recomienda hacer una copia de la configuracion personalizada para evitar que una actualizacion de fail2ban la sobrescriba:

```bash
sudo cp /etc/fail2ban/jail.conf /etc/fail2ban/jail.local
```

Como podemos observar luego de realizar la accion sugerida el Hardening index subio de 66 a 68 puntos

![Nuevo resultado Linys](https://github.com/user-attachments/assets/ce111e50-7888-4bf8-91d4-d4f227a8e5c5)

Realizar esta accion no tuvo ningun impacto en la disponibilidad de mi sistema ya que no me conecto a mi computadora de manera remota

## Auditoria con OpenVAS

OpenVAS (Open Vulnerability Assessment System) es un escáner de vulnerabilidades open source, básicamente una herramienta que revisa sistemas, servidores y redes en busca de fallas de seguridad conocidas.

Para esta parte del laboratorio vamos a hacer una auditoria remota usando OpenVAS desde una VM con Kali linux sobre la VM de CyberOps workstation el cual tiene Debian Linux. Para hacer esta auditoria hice lo siguiente

1. Descargar OpenVas en la maquina con Kali Linux
2. Obtener la ip y las credenciales ssh de mi maquina objetivo (CyberOps Workstation)
3. Cargar la configuracion del target en OpenVas
4. Crear la task para realizar un escaneo estandar Full and Fast
5. Correr la task. Los resultados obtenidos son los siguientes

### Solucion de

Para esto en la maquina objetivo hice esto

Volvia a correr la task y ahora arroja el siguiente resultado:

## Repeticion desde distribucion orientada a la seguridad

Para esta parte vamos a repetir las auditorias en una distribucion de Linux orientada a la seguridad, en mi caso usare Kali linux

### Auditoria con Linys

Al correr Linys en Kali Linux obtuve los siguientes resultados

### Auditoria con OpenVas
